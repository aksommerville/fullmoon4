#include "fmn_game.h"
#include "sprite/fmn_sprite.h"
#include "hero/fmn_hero.h"

/* Init.
 */
 
int fmn_game_init() {
  if (fmn_game_load_map(1)<1) return -1;
  fmn_hero_set_position(FMN_COLC*0.5f,FMN_ROWC*0.5f);
  return 0;
}

/* Spawn sprite in new map.
 */
 
static void cb_spawn(
  int8_t x,int8_t y,
  uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3,
  const uint8_t *cmdv,uint16_t cmdc
) {
  //fmn_log("%s (%d,%d) #%d [%d,%d,%d,%d] cmdv=%p cmdc=%d",__func__,x,y,spriteid,arg0,arg1,arg2,arg3,cmdv,cmdc);
  uint8_t argv[]={arg0,arg1,arg2,arg3};
  struct fmn_sprite *sprite=fmn_sprite_spawn(x+0.5f,y+0.5f,spriteid,cmdv,cmdc,argv,sizeof(argv));
  if (!sprite) {
    fmn_log(
      "Failed to spawn sprite %d at (%d,%d), argv=[%d,%d,%d,%d]",
      spriteid,x,y,arg0,arg1,arg2,arg3
    );
    return;
  }
}

/* Load map.
 */
 
int fmn_game_load_map(int mapid) {
  fmn_sprites_clear();
  int err=fmn_load_map(mapid,cb_spawn);
  if (err<=0) return err;
  if (fmn_hero_reset()<0) return -1;
  return 1;
}

/* Input event.
 */
 
void fmn_game_input(uint8_t bit,uint8_t value,uint8_t state) {
  fmn_hero_input(bit,value,state);
}

/* Trigger navigation.
 */
 
static void fmn_game_navigate(int8_t dx,int8_t dy) {
  uint16_t mapid=0;
  uint8_t transition=0;
       if (dx<0) { mapid=fmn_global.neighborw; transition=FMN_TRANSITION_PAN_LEFT; }
  else if (dx>0) { mapid=fmn_global.neighbore; transition=FMN_TRANSITION_PAN_RIGHT; }
  else if (dy<0) { mapid=fmn_global.neighborn; transition=FMN_TRANSITION_PAN_UP; }
  else if (dy>0) { mapid=fmn_global.neighbors; transition=FMN_TRANSITION_PAN_DOWN; }
  if (!mapid) return;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  fmn_prepare_transition(transition);
  if (fmn_game_load_map(mapid)<1) {
    fmn_cancel_transition();
    return;
  }
  fmn_hero_set_position(herox-dx*FMN_COLC,heroy-dy*FMN_ROWC);
  // Preserve velocity on these neighbor transitions.
  fmn_commit_transition();
}

static void fmn_game_check_doors(uint8_t x,uint8_t y) {
  struct fmn_door *door=fmn_global.doorv;
  uint32_t i=fmn_global.doorc;
  for (;i-->0;door++) {
    if (door->x!=x) continue;
    if (door->y!=y) continue;
    
    float dstx=door->dstx+0.5f;
    float dsty=door->dsty+0.5f;
    fmn_prepare_transition(FMN_TRANSITION_DOOR);
    if (fmn_game_load_map(door->mapid)<1) {
      fmn_cancel_transition();
      continue;
    }
    
    // After placing the hero, pump its quantized position once to ignore whatever it landed on.
    // (typically, that's the other end of this door).
    // Also kill velocity.
    fmn_hero_set_position(dstx,dsty);
    int8_t dumx,dumy;
    fmn_hero_get_quantized_position(&dumx,&dumy);
    fmn_hero_kill_velocity();
    
    fmn_commit_transition();
    return;
  }
}

/* Update.
 */
 
void fmn_game_update(float elapsed) {

  fmn_sprites_update(elapsed);
  fmn_hero_update(elapsed);
  fmn_sprites_sort_partial();

  int8_t x,y;
  if (fmn_hero_get_quantized_position(&x,&y)) {
    //fmn_log("hero at %d,%d",x,y);
    if (x<0) fmn_game_navigate(-1,0);
    else if (x>=FMN_COLC) fmn_game_navigate(1,0);
    else if (y<0) fmn_game_navigate(0,-1);
    else if (y>=FMN_ROWC) fmn_game_navigate(0,1);
    else fmn_game_check_doors(x,y);
  }
}