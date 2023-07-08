#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define SCARYDOOR_OPEN_TIME 3.0f /* s */

#define closedness sprite->fv[0]
#define opening sprite->bv[0]

/* Runs at completion, once we are fully open.
 * The bottom row of the 2x3 we occupy should be a SOLID tile with an identical VACANT one the next row up.
 */
 
static void scarydoor_make_foot_passable(struct fmn_sprite *sprite) {
  int8_t x=(int8_t)sprite->x;
  int8_t y=(int8_t)(sprite->y+2.0f);
  if ((x<0)||(y<0)||(x+1>=FMN_COLC)||(y>=FMN_ROWC)) return;
  uint8_t p=y*FMN_COLC+x;
  fmn_global.map[p++]-=0x10;
  fmn_global.map[p]-=0x10;
  // Normally that would mandate a fmn_map_dirty(); but we require that they be identical.
}

/* Init.
 */
 
static void _scarydoor_init(struct fmn_sprite *sprite) {
  closedness=1.0f;
  
  // Massively important! If the hero is on top of us, pop open instantly.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x,dy=heroy-(sprite->y+2.0f); // +2.0f: Scarydoor is positioned at the top of its 2x3 space; hero should be at the bottom of it
  if ((dx>=-2.0f)&&(dx<=2.0f)&&(dy>=-1.0f)&&(dy<=1.0f)) {
    closedness=0.0f;
    scarydoor_make_foot_passable(sprite);
    fmn_sprite_kill(sprite);
  }
}

/* Update.
 */
 
static void _scarydoor_update(struct fmn_sprite *sprite,float elapsed) {
  if (!opening) return;
  if ((closedness-=elapsed/SCARYDOOR_OPEN_TIME)<=0.0f) {
    closedness=0.0f;
    scarydoor_make_foot_passable(sprite);
    fmn_sprite_kill(sprite);
  }
}

/* Interact.
 */
 
static int16_t _scarydoor_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_OPEN: opening=1; break;
      } break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_scarydoor={
  .init=_scarydoor_init,
  .update=_scarydoor_update,
  .interact=_scarydoor_interact,
};
