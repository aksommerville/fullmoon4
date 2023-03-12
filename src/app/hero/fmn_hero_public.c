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
  
    uint8_t col=fmn_global.herostartp%FMN_COLC;
    uint8_t row=fmn_global.herostartp/FMN_COLC;
    if (!(fmn_hero.sprite=fmn_sprite_spawn(col+0.5f,row+0.5f,0,0,0,0,0))) return -1;
    fmn_hero.sprite->update=0;
    fmn_hero.sprite->style=FMN_SPRITE_STYLE_HERO;
    fmn_hero.sprite->imageid=2;
    
    fmn_hero.sprite->physics=
      FMN_PHYSICS_MOTION|
      FMN_PHYSICS_SPRITES|
      FMN_PHYSICS_SOLID|
      ((fmn_global.active_item==FMN_ITEM_BROOM)?0:FMN_PHYSICS_HOLE)|
      FMN_PHYSICS_BLOWABLE|
    0;
    fmn_hero.sprite->radius=0.250f;
    fmn_hero.sprite->veldecay=0.0f; // We override velocity management completely; veldecay is irrelevant.
    fmn_hero.sprite->velx=fmn_hero.velx;
    fmn_hero.sprite->vely=fmn_hero.vely;
    fmn_hero.sprite->invmass=0x80;
    
    /*XXX TEMP get some items initially.
    fmn_global.itemv[FMN_ITEM_BROOM]=1;
    fmn_global.itemv[FMN_ITEM_CHEESE]=1;
    fmn_global.itemqv[FMN_ITEM_CHEESE]=20;
    fmn_global.selected_item=FMN_ITEM_BROOM;
    /**/
  }
  
  fmn_hero.cellx=-128;
  fmn_hero.celly=-128;
  
  fmn_hero.enterx=fmn_hero.sprite->x;
  fmn_hero.entery=fmn_hero.sprite->y;
  fmn_hero.recent_reset=1;
  
  if (!fmn_global.facedir) fmn_global.facedir=FMN_DIR_S;
  
  return 0;
}

/* Input.
 */
 
void fmn_hero_input(uint8_t bit,uint8_t value,uint8_t state) {

  // Spell repudiation is highly transient; drop on any keypress.
  if (state) {
    fmn_global.spell_repudiation=0;
  }
  
  if (bit&(FMN_INPUT_LEFT|FMN_INPUT_RIGHT|FMN_INPUT_UP|FMN_INPUT_DOWN)) {
    if (!fmn_hero_item_motion(bit,value)) {
      fmn_hero_motion_event(bit,value);
    }
  } else if (bit==FMN_INPUT_USE) {
    fmn_hero_item_event(value);
  } else if (bit==FMN_INPUT_MENU) {
    if (value) {
      fmn_begin_menu(FMN_MENU_PAUSE);
      fmn_hero_kill_velocity();
    }
  }
  
  fmn_hero_motion_input(state);
}

/* Update.
 */
 
void fmn_hero_update(float elapsed) {

  if (fmn_hero.recent_reset) {
    fmn_hero.enterx=fmn_hero.sprite->x;
    fmn_hero.entery=fmn_hero.sprite->y;
    fmn_hero.recent_reset=0;
  }

  fmn_hero_motion_update(elapsed);
  fmn_hero_item_update(elapsed);
  
  if (fmn_global.injury_time>0.0f) {
    if ((fmn_global.injury_time-=elapsed)<0.0f) {
      fmn_global.injury_time=0.0f;
    }
  }
  if (fmn_global.curse_time>0.0f) {
    if ((fmn_global.curse_time-=elapsed)<=0.0f) {
      fmn_global.curse_time=0.0f;
      fmn_sound_effect(FMN_SFX_UNCURSE);
    }
  }
  
  fmn_hero.velx=fmn_hero.sprite->velx;
  fmn_hero.vely=fmn_hero.sprite->vely;
  
  // Update (shovelx,shovely) regardless of selected item. eg seed also uses it.
  float x=fmn_hero.sprite->x;
  float y=fmn_hero.sprite->y;
  switch (fmn_global.facedir) {
    case FMN_DIR_W: x-=0.5f; break;
    case FMN_DIR_E: x+=0.5f; break;
    case FMN_DIR_N: y-=0.5f; break;
    case FMN_DIR_S: y+=0.5f; break;
  }
  fmn_global.shovelx=x; if (x<0.0f) fmn_global.shovelx--;
  fmn_global.shovely=y; if (y<0.0f) fmn_global.shovely--;
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

void fmn_hero_kill_velocity() {
  fmn_hero.walkforce=0.0f;
  fmn_hero.sprite->velx=0.0f;
  fmn_hero.sprite->vely=0.0f;
  fmn_global.walking=0;
}

/* Check feet.
 */
 
uint8_t fmn_hero_feet_on_ground() {
  if (fmn_global.active_item==FMN_ITEM_BROOM) return 0;
  return 1;
}
