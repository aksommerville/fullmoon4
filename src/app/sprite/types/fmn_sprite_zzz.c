#include "app/sprite/fmn_sprite.h"

#define ZZZ_PERIOD 2.0f
#define ZZZ_SPEED_X 0.1f
#define ZZZ_SPEED_Y -0.6f

#define tileid0 sprite->bv[0]
#define clock sprite->fv[0]
#define x0 sprite->fv[1]
#define y0 sprite->fv[2]
#define xxtra sprite->fv[3]
#define yxtra sprite->fv[4]
#define source ((struct fmn_sprite*)sprite->pv[0])
#define source_w sprite->pv[0]

/* Init.
 */
 
static void _zzz_init(struct fmn_sprite *sprite) {
  sprite->imageid=3;
  sprite->tileid=tileid0=0x2c;
  sprite->layer=0xe0;
  x0=sprite->x;
  y0=sprite->y;
}

/* Update.
 */
 
static void _zzz_update(struct fmn_sprite *sprite,float elapsed) {

  if (source) {
    if (!source->style) {
      source_w=0;
    } else {
      x0=source->x;
      y0=source->y;
    }
  }

  clock+=elapsed;
  if (clock>=ZZZ_PERIOD) {
    clock=0.0f;
    sprite->x=x0+xxtra;
    sprite->y=y0-0.5f+yxtra;
    sprite->tileid=tileid0;
  } else {
    sprite->x+=ZZZ_SPEED_X*elapsed;
    sprite->y+=ZZZ_SPEED_Y*elapsed;
  }
  sprite->tileid=tileid0+(((int)(clock*3.0f))&1);
}

/* Interact.
 */

static int16_t _zzz_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_REVEILLE: {
            fmn_sprite_kill(sprite);
            return 0;
          }
      } break;
    case FMN_ITEM_BELL: fmn_sprite_kill(sprite); return 0;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_zzz={
  .init=_zzz_init,
  .update=_zzz_update,
  .interact=_zzz_interact,
};
