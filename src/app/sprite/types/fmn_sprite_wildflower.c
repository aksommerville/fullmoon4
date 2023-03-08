#include "app/sprite/fmn_sprite.h"

#define varietal sprite->argv[0]

static void _wildflower_init(struct fmn_sprite *sprite) {
  sprite->tileid+=(varietal&3)*2;
}

static int16_t _wildflower_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_BLOOM: sprite->tileid|=1; break;
      } break;
  }
  return 0;
}

const struct fmn_sprite_controller fmn_sprite_controller_wildflower={
  .init=_wildflower_init,
  .interact=_wildflower_interact,
};
