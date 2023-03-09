#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define itemid sprite->argv[0]

static void _treasure_init(struct fmn_sprite *sprite) {
  if ((itemid<1)||(itemid>=FMN_ITEM_COUNT)||(fmn_global.itemv[itemid]&&!fmn_item_default_quantities[itemid])) {
    fmn_sprite_kill(sprite);
    return;
  }
}

static void _treasure_hero_collision(struct fmn_sprite *sprite,struct fmn_sprite *hero) {
  if (!fmn_collect_item(itemid,0)) return;
  fmn_sprite_kill(sprite);
}

const struct fmn_sprite_controller fmn_sprite_controller_treasure={
  .init=_treasure_init,
  .hero_collision=_treasure_hero_collision,
};
