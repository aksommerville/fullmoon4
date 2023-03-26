#include "app/sprite/fmn_sprite.h"

#define clock sprite->fv[0]

static void _deadwitch_init(struct fmn_sprite *sprite) {
  sprite->layer=100;
  sprite->style=FMN_SPRITE_STYLE_DEADWITCH;
}

static void _deadwitch_update(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
}

const struct fmn_sprite_controller fmn_sprite_controller_deadwitch={
  .init=_deadwitch_init,
  .update=_deadwitch_update,
};
