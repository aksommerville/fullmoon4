#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

static void _hazard_hero_collision(struct fmn_sprite *sprite,struct fmn_sprite *hero) {
  fmn_hero_injure(sprite->x,sprite->y,sprite);
}

const struct fmn_sprite_controller fmn_sprite_controller_hazard={
  .hero_collision=_hazard_hero_collision,
};
