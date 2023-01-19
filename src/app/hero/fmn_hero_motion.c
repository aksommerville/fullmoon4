#include "fmn_hero_internal.h"
#include <math.h>

/* Begin walking.
 */
 
static void fmn_hero_walk_begin(int8_t dx,int8_t dy) {
  fmn_global.walking=1;
  fmn_hero.walkdx=dx;
  fmn_hero.walkdy=dy;
}

/* Change direction while walking.
 */
 
static void fmn_hero_walk_turn(int8_t dx,int8_t dy) {
  //TODO I think I want inertia to continue carrying the hero in the previous direction a little.
  // Currently, the full inertia redirects instantly. Doesn't feel natural.
  fmn_hero.walkdx=dx;
  fmn_hero.walkdy=dy;
}

/* End walking.
 */
 
static void fmn_hero_walk_end() {
  fmn_global.walking=0;
  fmn_hero.walkdx=0;
  fmn_hero.walkdy=0;
}

/* Input event.
 * Set facedir.
 */
 
void fmn_hero_motion_event(uint8_t bit,uint8_t value) {
  if (value) switch (bit) {
    case FMN_INPUT_LEFT:  fmn_global.facedir=FMN_DIR_W; break;
    case FMN_INPUT_RIGHT: fmn_global.facedir=FMN_DIR_E; break;
    case FMN_INPUT_UP:    fmn_global.facedir=FMN_DIR_N; break;
    case FMN_INPUT_DOWN:  fmn_global.facedir=FMN_DIR_S; break;
  } else switch (bit) {
    case FMN_INPUT_LEFT:  if (fmn_hero.walkdy<0) fmn_global.facedir=FMN_DIR_N; else if (fmn_hero.walkdy>0) fmn_global.facedir=FMN_DIR_S; break;
    case FMN_INPUT_RIGHT: if (fmn_hero.walkdy<0) fmn_global.facedir=FMN_DIR_N; else if (fmn_hero.walkdy>0) fmn_global.facedir=FMN_DIR_S; break;
    case FMN_INPUT_UP:    if (fmn_hero.walkdx<0) fmn_global.facedir=FMN_DIR_W; else if (fmn_hero.walkdx>0) fmn_global.facedir=FMN_DIR_E; break;
    case FMN_INPUT_DOWN:  if (fmn_hero.walkdx<0) fmn_global.facedir=FMN_DIR_W; else if (fmn_hero.walkdx>0) fmn_global.facedir=FMN_DIR_E; break;
  }
}

/* Input state changed.
 */
 
void fmn_hero_motion_input(uint8_t state) {

  int8_t dx=0,dy=0;
  switch (state&(FMN_INPUT_LEFT|FMN_INPUT_RIGHT)) {
    case FMN_INPUT_LEFT: dx=-1; break;
    case FMN_INPUT_RIGHT: dx=1; break;
  }
  switch (state&(FMN_INPUT_UP|FMN_INPUT_DOWN)) {
    case FMN_INPUT_UP: dy=-1; break;
    case FMN_INPUT_DOWN: dy=1; break;
  }
  
  if (dx||dy) {
    if (!fmn_hero.walkdx&&!fmn_hero.walkdy) {
      fmn_hero_walk_begin(dx,dy);
    } else if ((fmn_hero.walkdx!=dx)||(fmn_hero.walkdy!=dy)) {
      fmn_hero_walk_turn(dx,dy);
    }
  } else if (fmn_hero.walkdx||fmn_hero.walkdy) {
    fmn_hero_walk_end();
  }
}

/* Update.
 */
 
void fmn_hero_motion_update(float elapsed) {

  // We may drop (fmn_global.walking) during transitions or pauses.
  // We're updating now, so ensure it is true.
  if (fmn_hero.walkdx||fmn_hero.walkdy) fmn_global.walking=1;

  // Determine target velocity (ignoring elapsed).
  //TODO Higher target velocity when riding broom or high on nitro. And other cases?
  float tvx=fmn_hero.walkdx*FMN_HERO_WALK_SPEED_MAX;
  float tvy=fmn_hero.walkdy*FMN_HERO_WALK_SPEED_MAX;
  if (fmn_hero.walkdx&&fmn_hero.walkdy) {
    const float halfroot2=M_SQRT2/2.0f;
    tvx*=halfroot2;
    tvy*=halfroot2;
  }
  
  // Accelerate toward the target with a maximum rate per axis.
  //TODO Different acceleration constants eg when riding the broom, standing on ice, ...
  // Will we ever need to accelerate at different rates toward vs away from zero?
  float limit=FMN_HERO_ACCELERATION*elapsed;
  float dx=tvx-fmn_hero.sprite->velx;
  if (dx>limit) dx=limit; else if (dx<-limit) dx=-limit;
  float dy=tvy-fmn_hero.sprite->vely;
  if (dy>limit) dy=limit; else if (dy<-limit) dy=-limit;
  fmn_hero.sprite->velx+=dx;
  fmn_hero.sprite->vely+=dy;
}
