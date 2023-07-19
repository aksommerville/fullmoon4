#include "app/fmn_game.h"
#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

/* Pumpkin in the NW corner, find the topmost pushblock and kill it.
 * This is no longer needed; we now take a more aggressive approach re pumpkin control.
 */

static int _find_top_pushblock(struct fmn_sprite *sprite,void *userdata) {
  struct fmn_sprite **found=userdata;
  if (sprite->controller!=FMN_SPRCTL_pushblock) return 0;
  if (*found&&((*found)->y<sprite->y)) return 0;
  *found=sprite;
  return 0;
}

void fmn_map_callback_kill_top_pushblock_if_pumpkin_at_nw(uint8_t param,void *userdata) {
  if (fmn_global.transmogrification!=1) return;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox>1.0f) return;
  struct fmn_sprite *pushblock=0;
  fmn_sprites_for_each(_find_top_pushblock,&pushblock);
  if (!pushblock) return;
  if (heroy>=pushblock->y) return;
  fmn_sprite_kill(pushblock);
}

/* Generic, set a gsbit on map events.
 */

void fmn_map_callback_set_gsbit_00xx(uint8_t param,void *userdata) {
  fmn_gs_set_bit(0x0000|param,1);
}

/* Prepare the next step of the swamp maze.
 * Start, end, and intermediate maps should all trigger this event alike.
 */

void fmn_map_callback_swamp_maze(uint8_t param,void *userdata) {
  
  // If the hero is near the north edge, eliminate the door stored in 3 bits of gs at (param).
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (heroy<1.0f) {
    uint8_t doorp=fmn_gs_get_word(param,3);
    if (doorp>=fmn_global.doorc) {
    } else {
      struct fmn_door *door=fmn_global.doorv+doorp;
      door->mapid=0;
      door->dstx=0x20; // event trigger
      door->extra=0; // event zero, should be undefined
    }
  
  // Hero not at the north edge, eliminate all doors.
  // Swamp-maze maps are not allowed to have any other doors.
  } else {
    fmn_global.doorc=0;
  }
  
  // Identify south exits.
  float exitv[8]; // x
  uint8_t exitc=0;
  uint8_t x=0;
  const uint8_t *mapp=fmn_global.map+(FMN_ROWC-1)*FMN_COLC;
  while (x<FMN_COLC) {
    if (fmn_global.cellphysics[*mapp]==FMN_CELLPHYSICS_VACANT) {
      uint8_t c=1;
      while ((x+c<FMN_COLC)&&(fmn_global.cellphysics[mapp[c]]==FMN_CELLPHYSICS_VACANT)) c++;
      exitv[exitc++]=x+c*0.5f;
      if (exitc>=sizeof(exitv)/sizeof(exitv[0])) break;
      x+=c;
      mapp+=c;
    } else {
      x++;
      mapp++;
    }
  }
  
  // Pick a door randomly and store in 3 bits of gs at (param).
  uint8_t choice=0;
  if (exitc>0) choice=rand()%exitc;
  fmn_gs_set_word(param,3,choice);
  
  // Record as compass target.
  if (choice<exitc) {
    fmn_global.compassx=exitv[choice];
    fmn_global.compassy=FMN_ROWC;
  }
}
