#include "app/sprite/fmn_sprite.h"

static void _hazard_update(struct fmn_sprite *sprite,float elapsed) {
  //TODO
}

const struct fmn_sprite_controller fmn_sprite_controller_hazard={
  .update=_hazard_update,
};
