#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

#define TOLLTROLL_STAGE_COLLECT 0
#define TOLLTROLL_STAGE_RETREAT 1
#define TOLLTROLL_STAGE_RETIRE 2
#define TOLLTROLL_STAGE_RAGE 3

#define TOLLTROLL_RETREAT_SPEED 2.0f
#define TOLLTROLL_RETREAT_DISTANCE 3.0f
#define TOLLTROLL_RAGE_TIME 2.0f
#define TOLLTROLL_RAGE_RADIUS 1.0f
#define TOLLTROLL_RAGE_DISTANCE 0.8f
#define TOLLTROLL_DEMAND_RADIUS 0.8f
#define TOLLTROLL_DEMAND_DISTANCE 2.5f
#define TOLLTROLL_TILEID_PAYMENT 0x6c
#define TOLLTROLL_TILEID_RAGE 0x7c

#define sleeping sprite->bv[0]
#define tileid0 sprite->bv[1]
#define stage sprite->bv[2]
#define targetx sprite->fv[0]
#define clock sprite->fv[1]
#define gsbit sprite->argv[0]

static void tolltroll_begin_COLLECT(struct fmn_sprite *sprite);
static void tolltroll_begin_RETREAT(struct fmn_sprite *sprite);
static void tolltroll_begin_RETIRE(struct fmn_sprite *sprite);
static void tolltroll_begin_RAGE(struct fmn_sprite *sprite);

/* Init.
 */
 
static void _tolltroll_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  stage=TOLLTROLL_STAGE_COLLECT;
  // Ignore gsbit for now -- wait until the first update, so we're sure we move the right direction.
}

/* Payment indicator.
 * One of these will be called each frame during COLLECT stage,
 * and other stage entry points should call tolltroll_demand_nothing().
 */
 
struct tolltroll_find_dialogue_context {
  struct fmn_sprite *tolltroll;
  struct fmn_sprite *dialogue;
};

static int tolltroll_find_dialogue_1(struct fmn_sprite *sprite,void *userdata) {
  struct tolltroll_find_dialogue_context *ctx=userdata;
  if (sprite->controller) return 0;
  if (sprite->imageid!=ctx->tolltroll->imageid) return 0;
  if ((sprite->tileid!=TOLLTROLL_TILEID_PAYMENT)&&(sprite->tileid!=TOLLTROLL_TILEID_RAGE)) return 0;
  if (sprite->x<ctx->tolltroll->x-1.0f) return 0;
  if (sprite->x>ctx->tolltroll->x+1.0f) return 0;
  if (sprite->y<ctx->tolltroll->y-2.0f) return 0;
  if (sprite->y>ctx->tolltroll->y+0.0f) return 0;
  ctx->dialogue=sprite;
  return 1;
}
 
static struct fmn_sprite *tolltroll_find_dialogue(struct fmn_sprite *sprite) {
  struct tolltroll_find_dialogue_context ctx={
    .tolltroll=sprite,
    .dialogue=0,
  };
  fmn_sprites_for_each(tolltroll_find_dialogue_1,&ctx);
  return ctx.dialogue;
}
 
static void tolltroll_demand_payment(struct fmn_sprite *sprite) {
  struct fmn_sprite *dialogue=tolltroll_find_dialogue(sprite);
  if (dialogue) {
    dialogue->tileid=TOLLTROLL_TILEID_PAYMENT;
    return;
  }
  if (!(dialogue=fmn_sprite_generate_noparam(0,sprite->x,sprite->y-1.0f))) return;
  dialogue->imageid=sprite->imageid;
  dialogue->layer=150;
  dialogue->tileid=TOLLTROLL_TILEID_PAYMENT;
}

static void tolltroll_demand_nothing(struct fmn_sprite *sprite) {
  struct fmn_sprite *dialogue=tolltroll_find_dialogue(sprite);
  if (dialogue) {
    fmn_sprite_kill(dialogue);
  }
}

/* COLLECT: Default stage. Watch the hero and demand payment if she comes near.
 */
 
static void tolltroll_begin_COLLECT(struct fmn_sprite *sprite) {
  stage=TOLLTROLL_STAGE_COLLECT;
}
 
static void tolltroll_update_COLLECT(struct fmn_sprite *sprite,float elapsed) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox<sprite->x) {
    sprite->xform=0;
  } else {
    sprite->xform=FMN_XFORM_XREV;
  }
  sprite->tileid=tileid0;
  
  if (gsbit&&fmn_gs_get_bit(gsbit)) {
    tolltroll_begin_RETREAT(sprite);
    return;
  }
  
  if (
    ((heroy<=sprite->y-TOLLTROLL_RAGE_DISTANCE)||(heroy>=sprite->y+TOLLTROLL_RAGE_DISTANCE))&&
    (herox>=sprite->x-TOLLTROLL_RAGE_RADIUS)&&
    (herox<=sprite->x+TOLLTROLL_RAGE_RADIUS)
  ) {
    tolltroll_begin_RAGE(sprite);
    return;
  }
  
  if (
    (heroy>=sprite->y-TOLLTROLL_DEMAND_RADIUS)&&
    (heroy<=sprite->y+TOLLTROLL_DEMAND_RADIUS)&&
    (herox>=sprite->x-TOLLTROLL_DEMAND_DISTANCE)&&
    (herox<=sprite->x+TOLLTROLL_DEMAND_DISTANCE)
  ) {
    tolltroll_demand_payment(sprite);
    sprite->tileid=tileid0+1;
  } else {
    tolltroll_demand_nothing(sprite);
  }
}

/* RETREAT: Walk backward until we reach (targetx).
 */
 
static void tolltroll_begin_RETREAT(struct fmn_sprite *sprite) {
  tolltroll_demand_nothing(sprite);
  stage=TOLLTROLL_STAGE_RETREAT;
  if (sprite->xform&FMN_XFORM_XREV) { // facing right. walk left
    targetx=sprite->x-TOLLTROLL_RETREAT_DISTANCE;
  } else {
    targetx=sprite->x+TOLLTROLL_RETREAT_DISTANCE;
  }
  clock=0.0f;
}
 
static void tolltroll_update_RETREAT(struct fmn_sprite *sprite,float elapsed) {
  if (sprite->x<targetx) {
    sprite->x+=TOLLTROLL_RETREAT_SPEED*elapsed;
    if (sprite->x>=targetx) {
      sprite->x=targetx;
      tolltroll_begin_RETIRE(sprite);
    }
  } else {
    sprite->x-=TOLLTROLL_RETREAT_SPEED*elapsed;
    if (sprite->x<=targetx) {
      sprite->x=targetx;
      tolltroll_begin_RETIRE(sprite);
    }
  }
  clock+=elapsed;
  if (clock>=0.6f) clock-=0.6f;
  if (clock>=0.3) sprite->tileid=tileid0+3;
  else sprite->tileid=tileid0+2;
}

/* RETIRE: Stand around and bask in the joy of your earnings.
 */
 
static void tolltroll_begin_RETIRE(struct fmn_sprite *sprite) {
  tolltroll_demand_nothing(sprite);
  stage=TOLLTROLL_STAGE_RETIRE;
}

static void tolltroll_update_RETIRE(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0;
}

/* RAGE: Temporary stage during COLLECT, when the hero has bypassed us.
 */
 
static void tolltroll_begin_RAGE(struct fmn_sprite *sprite) {
  tolltroll_demand_nothing(sprite);
  stage=TOLLTROLL_STAGE_RAGE;
  clock=TOLLTROLL_RAGE_TIME;
  struct fmn_sprite *dialogue=fmn_sprite_generate_noparam(0,sprite->x,sprite->y-1.0f);
  if (!dialogue) return;
  dialogue->imageid=sprite->imageid;
  dialogue->layer=150;
  dialogue->tileid=TOLLTROLL_TILEID_RAGE;
}

static void tolltroll_update_RAGE(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0+0xf1; // odd case of a secondary tile lower than the base; i added this behavior late.
  if ((clock-=elapsed)<=0.0f) {
    tolltroll_begin_COLLECT(sprite);
    return;
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (herox<sprite->x) {
    sprite->xform=0;
  } else {
    sprite->xform=FMN_XFORM_XREV;
  }
}

/* Update.
 */
 
static void _tolltroll_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+4;
    return;
  }
  switch (stage) {
    case TOLLTROLL_STAGE_COLLECT: tolltroll_update_COLLECT(sprite,elapsed); break;
    case TOLLTROLL_STAGE_RETREAT: tolltroll_update_RETREAT(sprite,elapsed); break;
    case TOLLTROLL_STAGE_RETIRE: tolltroll_update_RETIRE(sprite,elapsed); break;
    case TOLLTROLL_STAGE_RAGE: tolltroll_update_RAGE(sprite,elapsed); break;
  }
}

/* Coin interaction.
 * Return nonzero if we accept this payment.
 */
 
static int16_t tolltroll_consider_payment(struct fmn_sprite *sprite) {
  if (sleeping) return 0;
  fmn_sound_effect(FMN_SFX_PAYMENT);
  if ((stage==TOLLTROLL_STAGE_RETIRE)||(stage==TOLLTROLL_STAGE_RETREAT)) {
    // i'll happily take your coins, but i've already done my thing.
  } else {
    tolltroll_begin_RETREAT(sprite);
  }
  if (gsbit) {
    fmn_gs_set_bit(gsbit,1);
  }
  return 1;
}

/* Interact.
 */
 
static int16_t _tolltroll_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping) {
            tolltroll_demand_nothing(sprite);
            sleeping=1;
            fmn_sprite_generate_zzz(sprite);
          } break;
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
    case FMN_ITEM_COIN: return tolltroll_consider_payment(sprite);
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_tolltroll={
  .init=_tolltroll_init,
  .update=_tolltroll_update,
  .interact=_tolltroll_interact,
};
