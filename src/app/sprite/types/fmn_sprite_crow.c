#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

#define CROW_STAGE_ENTER 1 /* Fly towards the corn. */
#define CROW_STAGE_APPETITE 2 /* Stand in front of the corn, preparing to eat it. */
#define CROW_STAGE_NIBBLE 3 /* Nose down. Corn disappears at stage start. */
#define CROW_STAGE_DIGEST 4 /* Nose back up. Show pride in what you have accomplished. */
#define CROW_STAGE_LEAD 5 /* Fly towards the middle of a screen edge, and hold there. */
#define CROW_STAGE_EXIT 6 /* Fly offscreen opposite where we came in. Nothing useful to do. */
#define CROW_STAGE_CIRCLE 7 /* Fly in a circle around the center of the screen, indicating you've got where you're going. */

#define CROW_FLIGHT_SPEED 4.0f
#define CROW_CIRCLE_RADIUS_SPEED 2.0f /* m/s */
#define CROW_CIRCLE_ANGULAR_SPEED 1.0f /* rad/s */
#define CROW_CIRCLE_RADIUS_MAX (FMN_ROWC*0.5f-1.0f)

#define CROW_FREE_BIRD_COUNT 4 /* Each feeding gets you 5 birds */

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define stage sprite->bv[2]
#define satisfied sprite->bv[3]
#define clock sprite->fv[0] /* Resets at stage changes. */
#define stagelen sprite->fv[1]
#define targetx sprite->fv[2] /* Flight stages, where are we headed? <0 if undecided for ENTER */
#define targety sprite->fv[3]
#define circle_phase sprite->fv[4]
#define circle_radius sprite->fv[5]

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

/* CIRCLE stage.
 */
 
static void crow_begin_circle(struct fmn_sprite *sprite) {
  stage=CROW_STAGE_CIRCLE;
  circle_phase=0.0f;
  circle_radius=0.0f;
  stagelen=999999.999f;
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
      case 0xff: { // it's right here! fly around in a circle to express your joy.
          satisfied=2;
        } break;
      case 0: { // No advice to offer. TODO Can we do something funny like shrug or explode?
          targetx=(sprite->x<(FMN_COLC>>1))?(FMN_COLC+1.0f):(-1.0f);
          targety=FMN_ROWC>>1;
          satisfied=0;
        } return; // no free birds either
    }
    fmn_add_free_birds(CROW_FREE_BIRD_COUNT);
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

static void crow_update_CIRCLE(struct fmn_sprite *sprite,float elapsed) {
  crow_animate_flight(sprite);
  circle_phase+=elapsed*CROW_CIRCLE_ANGULAR_SPEED;
  circle_radius+=elapsed*CROW_CIRCLE_RADIUS_SPEED;
  if (circle_phase>M_PI*2.0f) circle_phase-=M_PI*2.0f;
  if (circle_radius>CROW_CIRCLE_RADIUS_MAX) circle_radius=CROW_CIRCLE_RADIUS_MAX;
  float dstx=(FMN_COLC*0.5f)+circle_radius*cosf(circle_phase);
  float dsty=(FMN_ROWC*0.5f)+circle_radius*sinf(circle_phase);
  float dx=dstx-sprite->x;
  float dy=dsty-sprite->y;
  if (dx<0.0f) sprite->xform=FMN_XFORM_XREV; else sprite->xform=0;
  float dsq=dx*dx+dy*dy;
  float limit=CROW_FLIGHT_SPEED*elapsed;
  if (dsq>limit) {
    float d=sqrtf(dsq);
    dx=(dx*limit)/d;
    dy=(dy*limit)/d;
  }
  sprite->x+=dx;
  sprite->y+=dy;
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
        case CROW_STAGE_DIGEST: stagelen=1.500f; if (satisfied==2) crow_begin_circle(sprite); break;
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
      case CROW_STAGE_CIRCLE: crow_update_CIRCLE(sprite,elapsed); break;
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
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
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
