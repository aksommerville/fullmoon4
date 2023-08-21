#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define gsbit_clothed sprite->argv[0]

#define tileid0 sprite->bv[0]
#define clothed sprite->bv[1] /* orphans can't be both clothed and asleep -- clothed only happens in the non-interactive outtro sequence */
#define asleep sprite->bv[2]
#define animclock sprite->fv[0]

#define ORPHAN_BLINK_DURATION 0.4f

/* Snowflake: An extra sprite above the orphan's head that indicates she is cold.
 */
 
struct orphan_snowflake_context {
  struct fmn_sprite *sprite;
  struct fmn_sprite *snowflake;
};

static int orphan_find_snowflake_cb(struct fmn_sprite *q,void *userdata) {
  struct orphan_snowflake_context *ctx=userdata;
  struct fmn_sprite *sprite=ctx->sprite;
  if (q->controller!=FMN_SPRCTL_dummy) return 0;
  if (q->imageid!=sprite->imageid) return 0;
  if (q->tileid!=0x68) return 0;
  float dx=q->x-sprite->x;
  if ((dx<-0.125f)||(dx>0.125f)) return 0;
  float dy=q->y-sprite->y;
  if ((dy<-1.125f)||(dy>-0.875f)) return 0;
  ctx->snowflake=q;
  return 1;
}

static struct fmn_sprite *orphan_find_snowflake(struct fmn_sprite *sprite) {
  struct orphan_snowflake_context ctx={.sprite=sprite};
  fmn_sprites_for_each(orphan_find_snowflake_cb,&ctx);
  return ctx.snowflake;
}
 
static void orphan_create_snowflake(struct fmn_sprite *sprite) {
  if (asleep||clothed) return;
  struct fmn_sprite *snowflake=fmn_sprite_generate_noparam(FMN_SPRCTL_dummy,sprite->x,sprite->y-1.0f);
  if (!snowflake) return;
  snowflake->imageid=sprite->imageid;
  snowflake->tileid=0x68;
  snowflake->style=FMN_SPRITE_STYLE_TILE;
  snowflake->layer=150;
}

static void orphan_drop_snowflake(struct fmn_sprite *sprite) {
  struct fmn_sprite *snowflake=orphan_find_snowflake(sprite);
  if (!snowflake) return;
  fmn_sprite_kill(snowflake);
}

static void orphan_update_snowflake(struct fmn_sprite *sprite,float elapsed) {
  if (asleep||clothed) return;
  struct fmn_sprite *snowflake=orphan_find_snowflake(sprite);
  if (!snowflake) return;
  snowflake->x=sprite->x;
  snowflake->y=sprite->y-1.0f;
}

/* Set animation time.
 */
 
static void orphan_reset_animation(struct fmn_sprite *sprite) {
  animclock=2.0f+((rand()&0xffff)*2.0f)/65536.0f;
}

/* Update.
 */
 
static void _orphan_update(struct fmn_sprite *sprite,float elapsed) {
  animclock-=elapsed;
  if (animclock<=0.0f) {
    orphan_reset_animation(sprite);
  }
  if ((animclock<=ORPHAN_BLINK_DURATION)&&!asleep) {
    sprite->tileid=tileid0+(clothed?0x14:0x10);
  } else {
    sprite->tileid=tileid0+(clothed?4:asleep?2:0);
  }
  orphan_update_snowflake(sprite,elapsed);
}

/* Sleep/wake.
 */
 
static void orphan_sleep(struct fmn_sprite *sprite,int sleep) {
  if (sleep) {
    if (asleep) return;
    if (clothed) return;
    asleep=1;
    fmn_sprite_generate_zzz(sprite);
    orphan_drop_snowflake(sprite);
  } else {
    if (!asleep) return;
    asleep=0;
    orphan_create_snowflake(sprite);
  }
}

/* Interact.
 */
 
static int16_t _orphan_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: orphan_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: orphan_sleep(sprite,0); break;
        case FMN_SPELLID_PUMPKIN: orphan_drop_snowflake(sprite); fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: orphan_sleep(sprite,0); break;
    case 0xf0: { // secret interaction, to put the wolf clothes on
        orphan_drop_snowflake(sprite);
        clothed=1;
        asleep=0;
      } break;
  }
  return 0;
}

/* Init.
 */
 
static void _orphan_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  if (gsbit_clothed&&fmn_gs_get_bit(gsbit_clothed)) {
    clothed=1;
    sprite->tileid=tileid0+4;
  }
  orphan_reset_animation(sprite);
  orphan_create_snowflake(sprite);
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_orphan={
  .init=_orphan_init,
  .interact=_orphan_interact,
  .update=_orphan_update,
};
