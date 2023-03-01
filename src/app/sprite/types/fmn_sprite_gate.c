#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define gsbit sprite->argv[0]
#define state sprite->bv[0] /* 0=closed */
#define tileid0 sprite->bv[1]

/* State changed.
 */
 
static void _gate_cb_changed(void *userdata,uint16_t p,uint8_t v) {
  struct fmn_sprite *sprite=userdata;
  if (v) {
    if (state) return;
    state=1;
    sprite->tileid=tileid0+1;
    sprite->physics&=~FMN_PHYSICS_SPRITES;
  } else {
    if (!state) return;
    state=0;
    sprite->tileid=tileid0;
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

/* Interact.
 */
 
static int16_t _gate_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  fmn_log("%s %d:%d",__func__,itemid,qualifier);
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
  .interact=_gate_interact,
};
