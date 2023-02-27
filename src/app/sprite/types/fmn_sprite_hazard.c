#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

static void _hazard_hero_collision(struct fmn_sprite *sprite,struct fmn_sprite *hero) {
  fmn_hero_injure(sprite->x,sprite->y,sprite);
}

static int16_t _hazard_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_PITCHER: {
        if (qualifier) {
          // If we're a bonfire, get extinguished.
          if ((sprite->imageid==3)&&(sprite->tileid==0x20)) {
            fmn_sprite_kill(sprite);
            //TODO sound effect? smoke?
            return 1;
          }
        } else {
          // Can we provide a pitcherful of fire? That sounds cool.
        }
      } break;
  }
  return 0;
}

const struct fmn_sprite_controller fmn_sprite_controller_hazard={
  .hero_collision=_hazard_hero_collision,
  .interact=_hazard_interact,
};
