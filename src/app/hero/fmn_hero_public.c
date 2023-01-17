#include "fmn_hero_internal.h"

/* Extra globals.
 */
 
struct fmn_hero fmn_hero={0};

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
    
    fmn_hero.sprite->physics_mode=FMN_SPRITE_PHYSICS_FULL;
    fmn_hero.sprite->radius=0.5f;
    fmn_hero.sprite->veldecay=0.0f; // We override velocity management completely; veldecay is irrelevant.
    fmn_hero.sprite->velx=fmn_hero.velx;
    fmn_hero.sprite->vely=fmn_hero.vely;
  }
  
  fmn_hero.cellx=-128;
  fmn_hero.celly=-128;
  
  return 0;
}

/* Input.
 */
 
void fmn_hero_input(uint8_t bit,uint8_t value,uint8_t state) {
  
  //TODO Some items (wand, violin...) need to intercept the dpad. Should that patch in right here?
  if (bit&(FMN_INPUT_LEFT|FMN_INPUT_RIGHT|FMN_INPUT_UP|FMN_INPUT_DOWN)) {
    fmn_hero_motion_event(bit,value);
  } else if (bit==FMN_INPUT_USE) {
    fmn_hero_item_event(value);
  }
  
  fmn_hero_motion_input(state);
}

/* Update.
 */
 
void fmn_hero_update(float elapsed) {
  fmn_hero_motion_update(elapsed);
  fmn_hero_item_update(elapsed);
  
  fmn_hero.velx=fmn_hero.sprite->velx;
  fmn_hero.vely=fmn_hero.sprite->vely;
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
