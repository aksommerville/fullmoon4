#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"
#include "app/hero/fmn_hero.h"

#define COIN_SPEED 12.0f

/* Init.
 */
 
static void _coin_init(struct fmn_sprite *sprite) {
  sprite->imageid=2;
  sprite->tileid=0x1a;
  
  // (velx,vely) are placeholders, each indicates my flight speed.
  // Whoever instantiates us (fmn_hero_item.c) should replace these.
  sprite->velx=COIN_SPEED;
  sprite->vely=COIN_SPEED;
  
  sprite->invmass=1;
  sprite->radius=0.20f;
  sprite->physics=
    FMN_PHYSICS_MOTION|
    FMN_PHYSICS_SPRITES|
    FMN_PHYSICS_SOLID|
  0;
}

/* Update.
 */
 
static void _coin_update(struct fmn_sprite *sprite,float elapsed) {

  // Kill if offscreen.
  if ((sprite->x<-1.0f)||(sprite->y<-1.0f)||(sprite->x>FMN_COLC+1.0f)||(sprite->y>FMN_ROWC+1.0f)) {
    fmn_sprite_kill(sprite);
    return;
  }
}

/* Pressure.
 */
 
static void _coin_pressure(struct fmn_sprite *sprite,struct fmn_sprite *presser,uint8_t dir) {
  if (presser&&(presser->style==FMN_SPRITE_STYLE_HERO)) {
    fmn_sprite_kill(sprite);
    if (fmn_global.itemqv[FMN_ITEM_COIN]<0xff) {
      fmn_global.itemqv[FMN_ITEM_COIN]++;
      fmn_global.show_off_item=FMN_ITEM_COIN;
      fmn_global.show_off_item_time=0xff;
      fmn_sound_effect(FMN_SFX_ITEM_MINOR);
    }
  } else {
    uint8_t movedir=fmn_dir_from_vector(sprite->velx,sprite->vely);
    if (movedir==fmn_dir_reverse(dir)) {
      if (presser&&presser->interact&&presser->interact(presser,FMN_ITEM_COIN,0)) {
        fmn_sprite_kill(sprite);
        return;
      }
      sprite->tileid=0x2a;
      sprite->velx=0.0f;
      sprite->vely=0.0f;
      sprite->physics&=~FMN_PHYSICS_MOTION;
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_coin={
  .init=_coin_init,
  .update=_coin_update,
  .pressure=_coin_pressure,
  .static_pressure=_coin_pressure,
};
