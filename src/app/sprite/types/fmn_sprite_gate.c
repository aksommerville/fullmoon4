#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define GATE_BLINK_TIME 0.5f
#define GATE_BLINK_RATE 15.0f

#define gsbit sprite->argv[0]
#define state sprite->bv[0] /* 0=closed */
#define tileid0 sprite->bv[1]
#define blinktime sprite->fv[0]

/* State changed.
 */
 
static void _gate_cb_changed(void *userdata,uint16_t p,uint8_t v) {
  struct fmn_sprite *sprite=userdata;
  if (v) {
    if (state) return;
    state=1;
    blinktime=GATE_BLINK_TIME;
    sprite->physics&=~FMN_PHYSICS_SPRITES;
  } else {
    if (!state) return;
    state=0;
    blinktime=GATE_BLINK_TIME;
    sprite->physics|=FMN_PHYSICS_SPRITES;
  }
}

/* Init.
 */
 
static void _gate_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (gsbit) {
    if (fmn_gs_get_bit(gsbit)) {
      state=1;
      sprite->tileid=tileid0+1;
      sprite->physics&=~FMN_PHYSICS_SPRITES;
    }
    fmn_gs_listen_bit(gsbit,_gate_cb_changed,sprite);
  }
}

/* Update.
 */
 
static void _gate_update(struct fmn_sprite *sprite,float elapsed) {
  if (blinktime>0.0f) {
    if ((blinktime-=elapsed)<=0.0f) {
      blinktime=0.0f;
      if (state) sprite->tileid=tileid0+1;
      else sprite->tileid=tileid0;
    } else if (((int)(blinktime*GATE_BLINK_RATE))&1) {
      sprite->tileid=tileid0+1;
    } else {
      sprite->tileid=tileid0;
    }
  }
}

/* Interact.
 */
 
static int16_t _gate_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_OPEN: {
            if (!state) _gate_cb_changed(sprite,gsbit,1);
          } break;
      } break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_gate={
  .init=_gate_init,
  .update=_gate_update,
  .interact=_gate_interact,
};
