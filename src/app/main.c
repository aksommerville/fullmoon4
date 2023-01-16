#include "fmn_platform.h"

#define ELAPSED_TIME_LIMIT 1000

struct fmn_global fmn_global={0};
static uint32_t last_update_time=0;
static uint8_t pvinput=0;

static struct fmn_sprite my_sprite;
static struct fmn_sprite *my_sprite_list;
#define WALK_SPEED 7.5f /* m/s */
/* Getting a feel for "m/s" vs my usual "pixels/frame"...
 *   1.0 Very slow
 *   5.0 Reasonable walking speed, maybe on the slow side.
 *   7.5 Seems a good top speed for walking.
 *  10.0 Fast but still credible, turbo mode or something.
 *  15.0 '' pushing it
 *  20.0 Too fast to be useful.
 *  30.0 About 0.5 m/frame, ie the Nyquist point. May interfere with collision resolution.
 *  60.0 About 1 m/frame. Expect breakage like missed collisions.
 */

static void cb_spawn(int8_t x,int8_t y,uint16_t spriteid,uint8_t arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3) {
  fmn_log("%s (%d,%d) #%d [%d,%d,%d,%d]",__func__,x,y,spriteid,arg0,arg1,arg2,arg3);
}

int fmn_init() {
  if (fmn_load_map(1,cb_spawn)<1) {
    fmn_log("Failed to load initial map.");
    return -1;
  }
  
  fmn_log("neighbors: %d,%d,%d,%d",fmn_global.neighborw,fmn_global.neighbore,fmn_global.neighborn,fmn_global.neighbors);
  const struct fmn_door *door=fmn_global.doorv;
  uint32_t i=fmn_global.doorc;
  for (;i-->0;door++) {
    fmn_log("door at (%d,%d) to %d(%d,%d)",door->x,door->y,door->mapid,door->dstx,door->dsty);
  }
  
  //TEMP sprites
  my_sprite.x=5.0f;
  my_sprite.y=2.0f;
  my_sprite.imageid=2;
  my_sprite.tileid=0x02;
  my_sprite.xform=0;
  my_sprite_list=&my_sprite;
  fmn_global.spritev=&my_sprite_list;
  fmn_global.spritec=1;
  
  fmn_global.facedir=FMN_DIR_S;
  fmn_global.walking=0;
  
  return 0;
}

void fmn_update(uint32_t timems,uint8_t input) {

  uint32_t elapsedms=timems-last_update_time;
  last_update_time=timems;
  if ((elapsedms>ELAPSED_TIME_LIMIT)||(elapsedms<1)) {
    fmn_log("Clock fault! last_time=%d time=%d elapsed=%d",last_update_time,timems,elapsedms);
    elapsedms=1;
  }
  float elapsed=elapsedms/1000.0f;
    
  if (input!=pvinput) {
    //TODO input changes
    pvinput=input;
  }
  
  //TODO update game
  
  //TEMP move my sprite around.
  fmn_global.walking=0;
  switch (input&(FMN_INPUT_UP|FMN_INPUT_DOWN)) {
    case FMN_INPUT_UP: fmn_global.walking=1; my_sprite.y-=WALK_SPEED*elapsed; fmn_global.facedir=FMN_DIR_N; break;
    case FMN_INPUT_DOWN: fmn_global.walking=1; my_sprite.y+=WALK_SPEED*elapsed; fmn_global.facedir=FMN_DIR_S; break;
  }
  switch (input&(FMN_INPUT_LEFT|FMN_INPUT_RIGHT)) {
    case FMN_INPUT_LEFT: fmn_global.walking=1; my_sprite.x-=WALK_SPEED*elapsed; fmn_global.facedir=FMN_DIR_W; break;
    case FMN_INPUT_RIGHT: fmn_global.walking=1; my_sprite.x+=WALK_SPEED*elapsed; fmn_global.facedir=FMN_DIR_E; break;
  }
}
