#include "app/sprite/fmn_sprite.h"

#define tileid0 sprite->bv[0]
#define style0 sprite->bv[1]
#define sleeping sprite->bv[2]
#define xoffset ((int8_t)sprite->argv[0])
#define yoffset ((int8_t)sprite->argv[1])

/* Init.
 */
 
static void _decoanimal_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  style0=sprite->style;
  sprite->x+=xoffset*0.0625f;
  sprite->y+=yoffset*0.0625f;
}

/* Sleep state.
 */
 
static void decoanimal_sleep(struct fmn_sprite *sprite,uint8_t to_state) {
  if (to_state) {
    if (sleeping) return;
    sleeping=1;
    sprite->tileid=tileid0+2;
    sprite->style=FMN_SPRITE_STYLE_TILE;
    fmn_sprite_generate_zzz(sprite);
  } else {
    if (!sleeping) return;
    sleeping=0;
    sprite->tileid=tileid0;
    sprite->style=style0;
  }
}

/* Interact.
 */
 
static int16_t _decoanimal_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: decoanimal_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: decoanimal_sleep(sprite,0); break;
      } break;
    case FMN_ITEM_BELL: decoanimal_sleep(sprite,0); break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_decoanimal={
  .init=_decoanimal_init,
  .interact=_decoanimal_interact,
};
