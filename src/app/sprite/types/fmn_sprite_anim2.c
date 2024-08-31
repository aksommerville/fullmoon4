/* fmn_sprite_anim2.c
 * Alternative to the TWOFRAME render style, for the demon sprite, which animates very slowly.
 */

#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define FRAME_TIME 1.0f

#define tileid0 sprite->bv[0]
#define animframe sprite->bv[3]
#define animclock sprite->fv[1]

/* Update.
 */
 
static void _anim2_update(struct fmn_sprite *sprite,float elapsed) {
  if ((animclock-=elapsed)<=0.0f) {
    animclock+=FRAME_TIME;
    animframe^=1;
    sprite->tileid=tileid0+animframe;
  }
}

/* Interact.
 */
 
static int16_t _anim2_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
  }
  return 0;
}

/* Init.
 */
 
static void _anim2_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_anim2={
  .init=_anim2_init,
  .interact=_anim2_interact,
  .update=_anim2_update,
};
