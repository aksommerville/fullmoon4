#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

static int16_t _chalkguard_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier);

// We don't have a good way to store strings from resources.
// So we'll hold them here statically.
// That means you can't have more than one chalkguard per map.
static char chalkguard_word_a[16];
static char chalkguard_word_b[16];

#define CHALKGUARD_HIGHLIGHT_TIME 1.0f

#define stringid_a sprite->argv[0]
#define stringid_b sprite->argv[1]
#define tileid0 sprite->bv[0]
#define match_state sprite->bv[1] /* 0=none, 1=left, 2=right */
#define highlight_time sprite->fv[0]
#define full_time sprite->fv[1]

/* Init.
 */
 
static void _chalkguard_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;

  memset(chalkguard_word_a,0,sizeof(chalkguard_word_a));
  memset(chalkguard_word_b,0,sizeof(chalkguard_word_b));
  fmn_get_string(chalkguard_word_a,sizeof(chalkguard_word_a),stringid_a);
  fmn_get_string(chalkguard_word_b,sizeof(chalkguard_word_b),stringid_b);
  
  struct fmn_sprite *bl=fmn_sprite_generate_noparam(0,sprite->x-1.0f,sprite->y);
  struct fmn_sprite *br=fmn_sprite_generate_noparam(0,sprite->x+1.0f,sprite->y);
  if (!bl||!br) {
    fmn_sprite_kill(sprite);
    return;
  }
  bl->invmass=br->invmass=0;
  bl->radius=br->radius=0.5f;
  bl->imageid=br->imageid=sprite->imageid;
  bl->tileid=br->tileid=tileid0+2;
  bl->xform=FMN_XFORM_XREV;

  _chalkguard_interact(sprite,FMN_ITEM_CHALK,0); // read the initial chalk state
  highlight_time=0.0f; // don't react to initial state
  sprite->tileid=tileid0;
}

/* Raise or lower one blockade.
 */
 
struct chalkguard_find_blockade_context {
  struct fmn_sprite *chalkguard;
  struct fmn_sprite *blockade;
  float xlo,xhi,ylo,yhi;
};

static int chalkguard_find_blockade(struct fmn_sprite *sprite,void *userdata) {
  struct chalkguard_find_blockade_context *ctx=userdata;
  if (sprite->controller) return 0;
  if (sprite->x<ctx->xlo) return 0;
  if (sprite->x>ctx->xhi) return 0;
  if (sprite->y<ctx->ylo) return 0;
  if (sprite->y>ctx->yhi) return 0;
  if (sprite->imageid!=ctx->chalkguard->imageid) return 0;
  if (sprite->tileid!=ctx->chalkguard->bv[0]+2) return 0;
  ctx->blockade=sprite;
  return 1;
}
 
static void chalkguard_set_blockade(struct fmn_sprite *sprite,float dx,uint8_t blocked) {
  struct chalkguard_find_blockade_context ctx={
    .chalkguard=sprite,
    .blockade=0,
    .xlo=sprite->x+dx-0.5f,
    .xhi=sprite->x+dx+0.5f,
    .ylo=sprite->y-0.5f,
    .yhi=sprite->y+0.5f,
  };
  fmn_sprites_for_each(chalkguard_find_blockade,&ctx);
  if (!ctx.blockade) return;
  if (blocked) {
    ctx.blockade->physics=FMN_PHYSICS_SPRITES;
    if (dx<0.0f) ctx.blockade->xform=FMN_XFORM_XREV;
    else ctx.blockade->xform=0;
  } else {
    ctx.blockade->physics=0;
    if (dx<0.0f) ctx.blockade->xform=FMN_XFORM_XREV|FMN_XFORM_SWAP;
    else ctx.blockade->xform=FMN_XFORM_XREV|FMN_XFORM_YREV|FMN_XFORM_SWAP;
  }
}

/* Call one of these once after a chalk change.
 */
 
static void chalkguard_highlight(struct fmn_sprite *sprite) {
  if (full_time<0.5f) return;
  highlight_time=CHALKGUARD_HIGHLIGHT_TIME;
  struct fmn_sprite *tattle=fmn_sprite_generate_noparam(0,sprite->x,sprite->y-1.0f);
  if (tattle) {
    tattle->imageid=sprite->imageid;
    tattle->tileid=0xf7;
    tattle->style=FMN_SPRITE_STYLE_DOUBLEWIDE;
    tattle->layer=150;
  }
}

static int chalkguard_find_tattle(struct fmn_sprite *q,void *userdata) {
  if (q->tileid!=0xf7) return 0;
  if (q->style!=FMN_SPRITE_STYLE_DOUBLEWIDE) return 0;
  if (q->layer!=150) return 0;
  *(void**)userdata=q;
  return 1;
}

static void chalkguard_drop_tattle(struct fmn_sprite *sprite) {
  struct fmn_sprite *tattle=0;
  if (fmn_sprites_for_each(chalkguard_find_tattle,&tattle)) fmn_sprite_kill(tattle);
}
 
static void chalkguard_match(struct fmn_sprite *sprite,float dx) {
  chalkguard_set_blockade(sprite,dx,0);
  chalkguard_set_blockade(sprite,-dx,1);
  uint8_t nstate=(dx>0.0f)?2:1;
  if (match_state!=nstate) {
    match_state=nstate;
    sprite->tileid=tileid0+1;
    chalkguard_highlight(sprite);
  }
}

static void chalkguard_match_none(struct fmn_sprite *sprite) {
  chalkguard_set_blockade(sprite,-1.0f,1);
  chalkguard_set_blockade(sprite,1.0f,1);
  if (match_state!=0) {
    match_state=0;
    sprite->tileid=tileid0+1;
    chalkguard_highlight(sprite);
  }
}

/* Check a sketched word.
 */
 
static int8_t chalkguard_check_word(const char *src,uint8_t srcc,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (srcc<=sizeof(chalkguard_word_a)) {
    if ((srcc<sizeof(chalkguard_word_a))&&chalkguard_word_a[srcc]) ;
    else if (!memcmp(src,chalkguard_word_a,srcc)) {
      chalkguard_match(sprite,-1.0f);
      return 1;
    }
    if ((srcc<sizeof(chalkguard_word_b))&&chalkguard_word_b[srcc]) ;
    else if (!memcmp(src,chalkguard_word_b,srcc)) {
      chalkguard_match(sprite,1.0f);
      return 1;
    }
  }
  return 0;
}

/* Interact.
 */
 
static int16_t _chalkguard_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_CHALK: {
        if (!fmn_for_each_sketch_word(chalkguard_check_word,sprite)) {
          chalkguard_match_none(sprite);
        }
      } break;
  }
  return 0;
}

/* Update.
 */
 
static void _chalkguard_update(struct fmn_sprite *sprite,float elapsed) {
  full_time+=elapsed;
  if (highlight_time>0.0f) {
    if ((highlight_time-=elapsed)<=0.0f) {
      highlight_time=0.0f;
      sprite->tileid=tileid0;
      chalkguard_drop_tattle(sprite);
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_chalkguard={
  .init=_chalkguard_init,
  .interact=_chalkguard_interact,
  .update=_chalkguard_update,
};

