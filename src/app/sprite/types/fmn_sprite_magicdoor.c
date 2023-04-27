#include "app/sprite/fmn_sprite.h"

#define clock sprite->fv[0]

static void _magicdoor_update(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  if (clock>=1.0f) {
    sprite->update=0;
    sprite->style=FMN_SPRITE_STYLE_TWOFRAME;
    sprite->tileid=0xce;
  } else if (clock>=0.75f) {
    sprite->tileid=0xdf;
  } else if (clock>=0.50f) {
    sprite->tileid=0xde;
  } else {
    sprite->tileid=0xdd;
  }
}

static void _magicdoor_init(struct fmn_sprite *sprite) {
  sprite->imageid=3;
  sprite->tileid=0xdd;
  sprite->style=FMN_SPRITE_STYLE_TILE;
}

const struct fmn_sprite_controller fmn_sprite_controller_magicdoor={
  .init=_magicdoor_init,
  .update=_magicdoor_update,
};
