#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define CROW_STAGE_ENTER 1 /* Fly towards the corn. */
#define CROW_STAGE_APPETITE 2 /* Stand in front of the corn, preparing to eat it. */
#define CROW_STAGE_NIBBLE 3 /* Nose down. Corn disappears at stage start. */
#define CROW_STAGE_DIGEST 4 /* Nose back up. Show pride in what you have accomplished. */
#define CROW_STAGE_LEAD 5 /* Fly towards the middle of a screen edge, and hold there. */
#define CROW_STAGE_EXIT 6 /* Fly offscreen opposite where we came in. Nothing useful to do. */

#define CROW_FLIGHT_SPEED 4.0f

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define stage sprite->bv[2]
#define satisfied sprite->bv[3]
#define clock sprite->fv[0] /* Resets at stage changes. */
#define stagelen sprite->fv[1]
#define targetx sprite->fv[2] /* Flight stages, where are we headed? <0 if undecided for ENTER */
#define targety sprite->fv[3]

/* Init.
 */
 
static void _crow_init(struct fmn_sprite *sprite) {
  sprite->imageid=3;
  sprite->tileid=0x54;
  sprite->layer=130;
  tileid0=sprite->tileid;
  stage=CROW_STAGE_ENTER;
  stagelen=999.999f; // ENTER ends explicitly, during update.
  targetx=targety=-1.0f;
  if (sprite->x>(FMN_COLC>>1)) sprite->xform=FMN_XFORM_XREV;
}

/* Enter NIBBLE stage.
 * If there is corn in my nose range, destroy it and decide which direction to lead.
 */
 
struct crow_find_reachable_corn_context {
  struct fmn_sprite *crow;
  struct fmn_sprite *corn;
};

static int crow_find_reachable_corn(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->controller!=FMN_SPRCTL_seed) return 0;
  struct crow_find_reachable_corn_context *ctx=userdata;
  float dx=sprite->x-ctx->crow->x;
  float dy=sprite->y-ctx->crow->y;
  if ((dy<-0.5f)||(dy>0.5f)) return 0;
  if (ctx->crow->xform&FMN_XFORM_XREV) {
    if ((dx<-1.0f)||(dx>0.0f)) return 0;
  } else {
    if ((dx<0.0f)||(dx>1.0f)) return 0;
  }
  ctx->corn=sprite;
  return 1;
}
 
static void crow_eat(struct fmn_sprite *sprite) {
  struct crow_find_reachable_corn_context ctx={
    .crow=sprite,
    .corn=0,
  };
  fmn_sprites_for_each(crow_find_reachable_corn,&ctx);
  if (ctx.corn) {
    fmn_sprite_kill(ctx.corn);
    satisfied=1;
    switch (fmn_secrets_get_guide_dir()) {
      case FMN_DIR_W: {
          targetx=1.0f;
          targety=FMN_ROWC>>1;
        } break;
      case FMN_DIR_E: {
          targetx=FMN_COLC-1.0f;
          targety=FMN_ROWC>>1;
        } break;
      case FMN_DIR_N: {
          targetx=FMN_COLC>>1;
          targety=1.0f;
        } break;
      case FMN_DIR_S: {
          targetx=FMN_COLC>>1;
          targety=FMN_ROWC-1.0f;
        } break;
    }
  } else {
    satisfied=0;
    targetx=(sprite->x<(FMN_COLC>>1))?(FMN_COLC+1.0f):(-1.0f);
    targety=FMN_ROWC>>1;
  }
}

/* Update, the flight stages.
 */
 
static void crow_animate_flight(struct fmn_sprite *sprite) {
  if (sprite->x<targetx) sprite->xform=0;
  else sprite->xform=FMN_XFORM_XREV;
  sprite->tileid=tileid0+((((int)(clock*3.0f))&1)?3:2);
  sprite->layer=130;
}

static uint8_t crow_approach_target(struct fmn_sprite *sprite,float elapsed) {
  float dx=targetx-sprite->x;
  float dy=targety-sprite->y;
  const float threshold=0.200f; // beware, it's possible to overshoot if this is too low. maybe we need to guard against that
  if ((dx>=-threshold)&&(dx<=threshold)&&(dy>=-threshold)&&(dy<=threshold)) return 1;
  float distance=sqrtf(dx*dx+dy*dy);
  sprite->x+=(dx*CROW_FLIGHT_SPEED*elapsed)/distance;
  sprite->y+=(dy*CROW_FLIGHT_SPEED*elapsed)/distance;
  return 0;
}

static int crow_find_any_corn(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->controller!=FMN_SPRCTL_seed) return 0;
  *(struct fmn_sprite**)userdata=sprite;
  return 1;
}
 
static void crow_update_ENTER(struct fmn_sprite *sprite,float elapsed) {
  if (targetx<0.0f) {
    // Find the corn, or fly across the screen.
    struct fmn_sprite *corn=0;
    fmn_sprites_for_each(crow_find_any_corn,&corn);
    if (corn) {
      targetx=corn->x;
      targety=corn->y;
      if (sprite->xform&FMN_XFORM_XREV) targetx+=0.5f;
      else targetx-=0.5f;
    } else {
      stage=CROW_STAGE_EXIT;
      targetx=sprite->x;
      targety=sprite->y;
      if (sprite->x<0.0f) targetx=FMN_COLC+1.0f;
      else if (sprite->x>FMN_COLC) targetx=-1.0f;
      else if (sprite->y<0.0f) targety=FMN_ROWC+1.0f;
      else if (sprite->y>FMN_ROWC) targety=-1.0f;
    }
  } else if (crow_approach_target(sprite,elapsed)) {
    stage=CROW_STAGE_APPETITE;
    stagelen=1.500f;
    clock=0;
  }
  crow_animate_flight(sprite);
}

static void crow_update_LEAD(struct fmn_sprite *sprite,float elapsed) {
  crow_animate_flight(sprite);
  crow_approach_target(sprite,elapsed); // fine if we reach it, just hang out there.
  if (!satisfied) stage=CROW_STAGE_EXIT; // we'll always auto-switch to LEAD. Bump it to EXIT here if needed. They're about the same thing.
}

static void crow_update_EXIT(struct fmn_sprite *sprite,float elapsed) {
  crow_animate_flight(sprite);
  if (crow_approach_target(sprite,elapsed)) {
    fmn_sprite_kill(sprite);
  }
}

/* Update.
 */
 
static void _crow_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+4;
    sprite->layer=128;
  } else {
    clock+=elapsed;
    if (clock>=stagelen) {
      stage++;
      clock=0.0f;
      switch (stage) {
        case CROW_STAGE_APPETITE: stagelen=0.500f; break;
        case CROW_STAGE_NIBBLE: stagelen=0.400f; crow_eat(sprite); break;
        case CROW_STAGE_DIGEST: stagelen=1.500f; break;
        default: stagelen=999.999f;
      }
    }
    switch (stage) {
      case CROW_STAGE_ENTER: crow_update_ENTER(sprite,elapsed); break;
      case CROW_STAGE_APPETITE: sprite->layer=128; sprite->tileid=tileid0; break;
      case CROW_STAGE_NIBBLE: sprite->layer=128; sprite->tileid=tileid0+1; break;
      case CROW_STAGE_DIGEST: sprite->layer=128; sprite->tileid=tileid0; break;
      case CROW_STAGE_LEAD: crow_update_LEAD(sprite,elapsed); break;
      case CROW_STAGE_EXIT: crow_update_EXIT(sprite,elapsed); break;
    }
  }
}

/* Interact.
 */
 
static int16_t _crow_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
        case FMN_SPELLID_LULLABYE: if (!sleeping) { sleeping=1; fmn_sprite_generate_zzz(sprite); } break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_crow={
  .init=_crow_init,
  .update=_crow_update,
  .interact=_crow_interact,
};
