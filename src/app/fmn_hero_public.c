#include "fmn_hero.h"
#include "fmn_sprite.h"

#define FMN_HERO_WALK_ACCELERATION 0.800f
#define FMN_HERO_WALK_SPEED_MAX    8.000f
#define FMN_HERO_WALK_DECELERATION 0.600f

/* Extra globals.
 */
 
static struct fmn_hero {
  struct fmn_sprite *sprite;
  int8_t walkdx,walkdy; // current state of dpad
  int8_t pvdx,pvdy; // most recent nonzero state of dpad
  float walkspeed;
  float walkspeed_target;
  float walkaccel; // always positive
  int8_t cellx,celly; // quantized position
} fmn_hero={0};

/* Reset.
 */
 
static int fmn_hero_cb_find(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->style==FMN_SPRITE_STYLE_HERO) {
    fmn_hero.sprite=sprite;
    return 1;
  }
  return 0;
}
 
int fmn_hero_reset() {
  fmn_hero.sprite=0;
  if (fmn_sprites_for_each(fmn_hero_cb_find,0)<1) {
  
    if (!(fmn_hero.sprite=fmn_sprite_spawn(FMN_COLC*0.5f,FMN_ROWC*0.5f,0,0,0))) return -1;
    fmn_hero.sprite->update=0;
    fmn_hero.sprite->style=FMN_SPRITE_STYLE_HERO;
    fmn_hero.sprite->imageid=2;
  }
  
  fmn_hero.cellx=-128;
  fmn_hero.celly=-128;
  
  return 0;
}

/* Item.
 */
 
static void fmn_hero_use_begin() {
  //TODO
}

static void fmn_hero_use_end() {
  //TODO
}

/* Input.
 */
 
void fmn_hero_input(uint8_t bit,uint8_t value,uint8_t state) {

  if (value) switch (bit) {
    case FMN_INPUT_LEFT:  fmn_global.facedir=FMN_DIR_W; break;
    case FMN_INPUT_RIGHT: fmn_global.facedir=FMN_DIR_E; break;
    case FMN_INPUT_UP:    fmn_global.facedir=FMN_DIR_N; break;
    case FMN_INPUT_DOWN:  fmn_global.facedir=FMN_DIR_S; break;
    case FMN_INPUT_USE: fmn_hero_use_begin(); break;
  } else switch (bit) {
    case FMN_INPUT_LEFT:  if (fmn_hero.walkdy<0) fmn_global.facedir=FMN_DIR_N; else if (fmn_hero.walkdy>0) fmn_global.facedir=FMN_DIR_S; break;
    case FMN_INPUT_RIGHT: if (fmn_hero.walkdy<0) fmn_global.facedir=FMN_DIR_N; else if (fmn_hero.walkdy>0) fmn_global.facedir=FMN_DIR_S; break;
    case FMN_INPUT_UP:    if (fmn_hero.walkdx<0) fmn_global.facedir=FMN_DIR_W; else if (fmn_hero.walkdx>0) fmn_global.facedir=FMN_DIR_E; break;
    case FMN_INPUT_DOWN:  if (fmn_hero.walkdx<0) fmn_global.facedir=FMN_DIR_W; else if (fmn_hero.walkdx>0) fmn_global.facedir=FMN_DIR_E; break;
    case FMN_INPUT_USE: fmn_hero_use_end(); break;
  }

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
      fmn_hero.walkaccel=FMN_HERO_WALK_ACCELERATION;
      fmn_hero.walkspeed_target=FMN_HERO_WALK_SPEED_MAX;
    }
    fmn_global.walking=1;
    fmn_hero.pvdx=fmn_hero.walkdx=dx;
    fmn_hero.pvdy=fmn_hero.walkdy=dy;
  } else if (fmn_hero.walkdx||fmn_hero.walkdy) {
    fmn_hero.walkaccel=FMN_HERO_WALK_DECELERATION;
    fmn_hero.walkspeed_target=0.0f;
    fmn_global.walking=0;
    fmn_hero.walkdx=0;
    fmn_hero.walkdy=0;
  }
}

/* Update.
 */
 
void fmn_hero_update(float elapsed) {

  if (fmn_hero.walkspeed<fmn_hero.walkspeed_target) {
    fmn_hero.walkspeed+=fmn_hero.walkaccel;
    if (fmn_hero.walkspeed>fmn_hero.walkspeed_target) {
      fmn_hero.walkspeed=fmn_hero.walkspeed_target;
    }
  } else if (fmn_hero.walkspeed>fmn_hero.walkspeed_target) {
    fmn_hero.walkspeed-=fmn_hero.walkaccel;
    if (fmn_hero.walkspeed<fmn_hero.walkspeed_target) {
      fmn_hero.walkspeed=fmn_hero.walkspeed_target;
    }
  }

  if (fmn_hero.walkspeed>0.0f) {
    fmn_hero.sprite->x+=fmn_hero.pvdx*fmn_hero.walkspeed*elapsed;
    fmn_hero.sprite->y+=fmn_hero.pvdy*fmn_hero.walkspeed*elapsed;
  }
}

/* Quantize position.
 */
 
uint8_t fmn_hero_get_quantized_position(int8_t *x,int8_t *y) {
  *x=fmn_hero.sprite->x; if (fmn_hero.sprite->x<0.0f) (*x)--;
  *y=fmn_hero.sprite->y; if (fmn_hero.sprite->y<0.0f) (*y)--;
  if ((*x!=fmn_hero.cellx)||(*y!=fmn_hero.celly)) {
    fmn_hero.cellx=*x;
    fmn_hero.celly=*y;
    return 1;
  }
  return 0;
}

/* Modify position directly and artificially.
 * Called during map transitions.
 */
 
void fmn_hero_get_position(float *x,float *y) {
  *x=fmn_hero.sprite->x;
  *y=fmn_hero.sprite->y;
}

void fmn_hero_set_position(float x,float y) {
  fmn_hero.sprite->x=x;
  fmn_hero.sprite->y=y;
}
