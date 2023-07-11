#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define magnitude sprite->argv[0]
#define tileid0 sprite->bv[0]
#define animframe sprite->bv[1]
#define motion sprite->sv[0]
#define animclock sprite->fv[0]
#define motiontime sprite->fv[1]

#define TM_X_RADIUS 0.5f
#define TM_UP_RADIUS 0.5f
#define TM_DOWN_RADIUS 1.5f
#define TM_NUDGE_SPEED_X 2.0f
#define TM_NUDGE_SPEED_Y 8.0f
#define TM_INCREMENT_INTERVAL 0.5f

/* Nudge hero towards some point, presumably the treadmill's center.
 */
 
static void tm_nudge_hero(struct fmn_sprite *sprite,float herox,float heroy,float dstx,float dsty,float elapsed) {
  float dxtotal=dstx-herox;
  float dytotal=dsty-heroy;
  float dx,dy;
  if (dxtotal<0.0f) {
    dx=-TM_NUDGE_SPEED_X*elapsed;
    if (dx<dxtotal) dx=dxtotal;
  } else {
    dx=TM_NUDGE_SPEED_X*elapsed;
    if (dx>dxtotal) dx=dxtotal;
  }
  if (dytotal<0.0f) {
    dy=-TM_NUDGE_SPEED_Y*elapsed;
    if (dy<dytotal) dy=dytotal;
  } else {
    dy=TM_NUDGE_SPEED_Y*elapsed;
    if (dy>dytotal) dy=dytotal;
  }
  fmn_hero_set_position(herox+dx,heroy+dy);
}

/* Adjust value by one, and create a sound effect.
 */
 
static void tm_increment(struct fmn_sprite *sprite,int d) {
  if (d<0) d=-1;
  else if (d>0) d=1;
  else return;
  int i=magnitude;
  while (i-->0) d*=10;
  fmn_game_event_broadcast(FMN_GAME_EVENT_TREADMILL,&d);
  fmn_sound_effect(FMN_SFX_TREADMILL_INCREMENT);
}

/* Update.
 */
 
static void _tm_update(struct fmn_sprite *sprite,float elapsed) {
  motion=0;
  if (fmn_global.active_item!=FMN_ITEM_BROOM) {
    float herox,heroy;
    fmn_hero_get_position(&herox,&heroy);
    if (
      (herox>sprite->x-TM_X_RADIUS)&&
      (herox<sprite->x+TM_X_RADIUS)&&
      (heroy>sprite->y-TM_UP_RADIUS)&&
      (heroy<sprite->y+TM_DOWN_RADIUS)
    ) {
      tm_nudge_hero(sprite,herox,heroy,sprite->x,sprite->y+0.5f,elapsed);
      if (fmn_global.walking) {
        if (fmn_global.facedir==FMN_DIR_S) motion=-1;
        else if (fmn_global.facedir==FMN_DIR_N) motion=1;
      }
    }
  }
  if (motion) {
    animclock+=elapsed;
    if (animclock>=0.075f) {
      animclock-=0.075f;
      if (motion>0) {
        animframe++;
        if (animframe>=6) animframe=0;
      } else {
        if (animframe) animframe--;
        else animframe=5;
      }
      sprite->tileid=tileid0+animframe;
    }
    motiontime+=elapsed;
    if (motiontime>=TM_INCREMENT_INTERVAL) {
      motiontime-=TM_INCREMENT_INTERVAL;
      tm_increment(sprite,motion);
    }
  } else {
    motiontime=0.0f;
  }
}

/* Interact.
 */
 
static int16_t _tm_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
  }
  return 0;
}

/* Init.
 */
 
static void _tm_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_treadmill={
  .init=_tm_init,
  .update=_tm_update,
  .interact=_tm_interact,
};
