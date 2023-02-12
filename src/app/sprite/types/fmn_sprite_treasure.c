#include "app/sprite/fmn_sprite.h"

#define itemid sprite->argv[0]

// A nonzero value here means the chest will re-appear on each map load, and add to the item's qualifier.
static const uint8_t fmn_initial_qualifier_per_itemid[FMN_ITEM_COUNT]={
  [FMN_ITEM_CORN]=5,
  [FMN_ITEM_SEED]=5,
  [FMN_ITEM_COIN]=5,
  [FMN_ITEM_MATCH]=5,
  [FMN_ITEM_CHEESE]=5,
};

static void _treasure_init(struct fmn_sprite *sprite) {
  if ((itemid<1)||(itemid>=FMN_ITEM_COUNT)||(fmn_global.itemv[itemid]&&!fmn_initial_qualifier_per_itemid[itemid])) {
    fmn_sprite_kill(sprite);
    return;
  }
}

static void cb_modal() {
}

static void _treasure_hero_collision(struct fmn_sprite *sprite,struct fmn_sprite *hero) {
  if (!fmn_global.itemv[itemid]) {
    // If we didn't already have this, begin some fanfare and select it.
    fmn_global.itemv[itemid]=1;
    fmn_global.selected_item=itemid;
    fmn_begin_menu(FMN_MENU_TREASURE,itemid,cb_modal);
  } else {
    // Something we already have, so do the passive "show_off_item" instead of the modal.
    // Do not auto-select it.
    fmn_global.show_off_item=itemid;
    fmn_global.show_off_item_time=0xff;
  }
  uint8_t q=fmn_initial_qualifier_per_itemid[itemid];
  if (q) {
    if (fmn_global.itemqv[itemid]>0xff-q) fmn_global.itemqv[itemid]=0xff;
    else fmn_global.itemqv[itemid]+=q;
  }
  fmn_sprite_kill(sprite);
}

const struct fmn_sprite_controller fmn_sprite_controller_treasure={
  .init=_treasure_init,
  .hero_collision=_treasure_hero_collision,
};
