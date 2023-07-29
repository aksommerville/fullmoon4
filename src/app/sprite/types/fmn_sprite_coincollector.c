#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define tileid0 sprite->bv[0]
#define threshold sprite->argv[0]
#define gsbit sprite->argv[1]
#define reset sprite->argv[2]

/* Dependent sprites.
 * One for the arm, two for the digits.
 */
 
struct cc_search_context {
  struct fmn_sprite *sprite;
  int v;
};

static int cc_open_arm_cb(struct fmn_sprite *q,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if (q->tileid!=tileid0+1) return 0;
  float dx=q->x-sprite->x,dy=q->y-sprite->y;
  if ((dx<0.75f)||(dx>1.25f)||(dy<-0.25f)||(dy>0.25f)) return 0;
  q->physics=0;
  q->xform=FMN_XFORM_XREV|FMN_XFORM_YREV|FMN_XFORM_SWAP;
  return 1;
}
 
static void cc_open_arm(struct fmn_sprite *sprite) {
  fmn_sprites_for_each(cc_open_arm_cb,sprite);
}

static int cc_update_digits_cb(struct fmn_sprite *q,void *userdata) {
  struct cc_search_context *ctx=userdata;
  struct fmn_sprite *sprite=ctx->sprite;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if ((q->tileid&0xf0)!=(tileid0&0xf0)) return 0;
  if (q->tileid<tileid0+2) return 0;
  float dx=q->x-sprite->x,dy=q->y-sprite->y;
  if ((dx<-0.5f)||(dx>0.5f)||(dy<-0.5f)||(dy>0.5f)) return 0;
  switch (q->bv[0]) {
    case 1: q->tileid=tileid0+2+(ctx->v%10); break;
    case 10: q->tileid=tileid0+2+((ctx->v/10)%10); break;
  }
  return 0;
}

static void cc_update_digits(struct fmn_sprite *sprite,int v) {
  struct cc_search_context ctx={.sprite=sprite,.v=v};
  fmn_sprites_for_each(cc_update_digits_cb,&ctx);
}
 
static void cc_create_dependents(struct fmn_sprite *sprite) {
  struct fmn_sprite *arm=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x+1.0f,sprite->y);
  if (arm) {
    arm->imageid=sprite->imageid;
    arm->tileid=tileid0+1;
    arm->style=FMN_SPRITE_STYLE_TILE;
    arm->physics=FMN_PHYSICS_SPRITES;
    arm->radius=0.5f;
    arm->invmass=0;
  }
  struct fmn_sprite *dig10=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x,sprite->y);
  if (dig10) {
    dig10->imageid=sprite->imageid;
    dig10->tileid=tileid0+2;
    dig10->style=FMN_SPRITE_STYLE_TILE;
    dig10->layer=sprite->layer+1;
    dig10->bv[0]=10;
  }
  struct fmn_sprite *dig01=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x+0.25f,sprite->y);
  if (dig01) {
    dig01->imageid=sprite->imageid;
    dig01->tileid=tileid0+2;
    dig01->style=FMN_SPRITE_STYLE_TILE;
    dig01->layer=sprite->layer+1;
    dig01->bv[0]=1;
  }
}

/* Accept coin.
 * Return nonzero if accepted (same as interact)
 */
 
static int16_t cc_accept_coin(struct fmn_sprite *sprite) {
  int amount=fmn_gs_get_word(gsbit,7);
  if (amount>=99) return 0;
  amount++;
  fmn_gs_set_word(gsbit,7,amount);
  if (amount==threshold) {
    cc_open_arm(sprite);
    if (reset) {
      amount=0;
      fmn_gs_set_word(gsbit,7,amount);
    }
  }
  cc_update_digits(sprite,amount);
  return 1;
}

/* Interact.
 */
 
static int16_t _cc_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_COIN: return cc_accept_coin(sprite);
  }
  return 0;
}

/* Init.
 */
 
static void _cc_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  cc_create_dependents(sprite);
  int v=fmn_gs_get_word(gsbit,7);
  if (v>=99) {
    v=99;
    fmn_gs_set_word(gsbit,7,v);
  }
  if (v>=threshold) cc_open_arm(sprite);
  if (v) cc_update_digits(sprite,v);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_coincollector={
  .init=_cc_init,
  .interact=_cc_interact,
};
