#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"
#include "app/fmn_game.h"

#define THROWSWITCH_BLACKOUT_TIME 1.0f

#define gsbit sprite->argv[0]
#define state sprite->bv[0]
#define tileid0 sprite->bv[1]
#define blackout_time sprite->fv[0]

/* State changes.
 */
 
static void throwswitch_set_state(struct fmn_sprite *sprite,uint8_t v) {
  if (v) {
    if (state) return;
    state=1;
    sprite->tileid=tileid0+1;
  } else {
    if (!state) return;
    state=0;
    sprite->tileid=tileid0;
  }
  if (gsbit) fmn_gs_set_bit(gsbit,state);
}

static void throwswitch_cb_gsbit(void *userdata,uint16_t p,uint8_t v) {
  throwswitch_set_state(userdata,v);
}

/* Init.
 */
 
static void _throwswitch_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (gsbit) {
    if (fmn_gs_get_bit(gsbit)) throwswitch_set_state(sprite,1);
    fmn_gs_listen_bit(gsbit,throwswitch_cb_gsbit,sprite);
  }
}

/* Update.
 */
 
static int throwswitch_sprite_is_activator(struct fmn_sprite *missile) {
  if (missile->controller==FMN_SPRCTL_missile) return 1;
  if (missile->controller==FMN_SPRCTL_coin) {
    if (!(missile->physics&FMN_PHYSICS_MOTION)) return 0;
    return 1;
  }
  return 0;
}
 
static int throwswitch_check_collision(struct fmn_sprite *missile,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (!throwswitch_sprite_is_activator(missile)) return 0;
  const float radius=0.5f;
  float dx=missile->x-sprite->x;
  if ((dx>radius)||(dx<-radius)) return 0;
  float dy=missile->y-sprite->y;
  if ((dy>radius)||(dy<-radius)) return 0;
  fmn_sound_effect(FMN_SFX_TREADLE_PRESS);
  throwswitch_set_state(sprite,state?0:1);
  blackout_time=THROWSWITCH_BLACKOUT_TIME;
  if (0&&missile->static_pressure) {
    uint8_t dir=fmn_dir_from_vector_cardinal(dx,dy);
    missile->static_pressure(missile,0,dir); // null, not sprite: otherwise it might trigger interact
  }
  return 1;
}
 
static void _throwswitch_update(struct fmn_sprite *sprite,float elapsed) {
  if (blackout_time>0.0f) {
    if ((blackout_time-=elapsed)<=0.0f) blackout_time=0.0f;
  } else {
    fmn_sprites_for_each(throwswitch_check_collision,sprite);
  }
}

/* Interact.
 */
 
static int16_t _throwswitch_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_COIN: {
        if (blackout_time>0.0f) return 0;
        throwswitch_set_state(sprite,state?0:1);
        blackout_time=THROWSWITCH_BLACKOUT_TIME;
      } return 1;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_throwswitch={
  .init=_throwswitch_init,
  .update=_throwswitch_update,
  .interact=_throwswitch_interact,
};
