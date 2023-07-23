#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define gsbit sprite->bv[0]
#define tileid0 sprite->bv[1]
#define unfurled sprite->bv[2]

/* Create the two dependent sprites, and update my tileid.
 */
 
static void tb_create_dependents(struct fmn_sprite *sprite) {
  sprite->tileid=tileid0+1;
  struct fmn_sprite *mid=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x,sprite->y+1.0f);
  struct fmn_sprite *bottom=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x,sprite->y+2.0f);
  if (!mid||!bottom) return;
  mid->imageid=bottom->imageid=sprite->imageid;
  mid->style=bottom->style=FMN_SPRITE_STYLE_TILE;
  mid->tileid=sprite->tileid+0x10;
  bottom->tileid=sprite->tileid+0x20;
}

/* Reset my tileid and remove the two dependent sprites.
 */
 
static int tb_destroy_dependents_cb(struct fmn_sprite *q,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if ((q->tileid!=sprite->tileid+0x11)&&(q->tileid!=sprite->tileid+0x21)) return 0;
  fmn_sprite_kill(q);
  return 0;
}
 
static void tb_destroy_dependents(struct fmn_sprite *sprite) {
  sprite->tileid=tileid0;
  fmn_sprites_for_each(tb_destroy_dependents_cb,sprite);
}

/* Furlage.
 * When unfurled, there are 2 dependent sprites just below me.
 */
 
static void tb_unfurl(struct fmn_sprite *sprite) {
  if (unfurled) return;
  unfurled=1;
  fmn_gs_set_bit(gsbit,1);
  tb_create_dependents(sprite);
}

static void tb_refurl(struct fmn_sprite *sprite) {
  if (!unfurled) return;
  unfurled=0;
  fmn_gs_set_bit(gsbit,0);
  tb_destroy_dependents(sprite);
}

/* Interact.
 */
 
static int16_t _tb_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_FEATHER: tb_unfurl(sprite); break;
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_WIND_N: tb_refurl(sprite); break;
        case FMN_SPELLID_WIND_S: tb_unfurl(sprite); break;
        case FMN_SPELLID_OPEN: tb_unfurl(sprite); break;
      } break;
    case FMN_ITEM_SNOWGLOBE: tb_unfurl(sprite); break;
  }
  return 0;
}

/* Init.
 */
 
static void _tb_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (fmn_gs_get_bit(gsbit)) {
    tb_unfurl(sprite);
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_tallbanner={
  .init=_tb_init,
  .interact=_tb_interact,
};
