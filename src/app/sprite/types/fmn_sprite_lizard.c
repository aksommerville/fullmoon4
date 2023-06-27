#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define LIZARD_WALK_SPEED 3.0f
#define LIZARD_ACCEL_RATE 0.125f
#define LIZARD_DECEL_RATE 0.125f
#define LIZARD_BURN_OFFSET 1.0f
#define LIZARD_BURN_RADIUS 0.6f
#define LIZARD_TARGET_Y_OFFSET 0.5f
#define LIZARD_TARGET_X_OFFSET 0.8f

#define tileid0 sprite->bv[0]
#define sleeping sprite->bv[1]
#define stage sprite->bv[2]
#define burning sprite->bv[3]
#define invmass0 sprite->bv[4]
#define clock sprite->fv[0]
#define fireclock sprite->fv[1]
#define walkclock sprite->fv[2]

/* Init.
 */
 
static void _lizard_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  invmass0=sprite->invmass;
}

/* Begin breathing fire.
 */
 
struct lizard_find_flame_context {
  struct fmn_sprite *lizard;
  struct fmn_sprite *flame;
};

static int lizard_find_flame_1(struct fmn_sprite *sprite,void *userdata) {
  struct lizard_find_flame_context *ctx=userdata;
  if (sprite->controller) return 0;
  if (sprite->pv[0]!=ctx->lizard) return 0;
  ctx->flame=sprite;
  return 1;
}

static struct fmn_sprite *lizard_find_flame(struct fmn_sprite *sprite) {
  struct lizard_find_flame_context ctx={
    .lizard=sprite,
    .flame=0,
  };
  fmn_sprites_for_each(lizard_find_flame_1,&ctx);
  return ctx.flame;
}
 
static void lizard_burn(struct fmn_sprite *sprite) {
  burning=1;
  sprite->tileid=tileid0+3;
  sprite->velx=sprite->vely=0.0f;
  fireclock=0.0f;
  sprite->invmass=0; // can't push while burning
  sprite->physics&=~FMN_PHYSICS_BLOWABLE;
  struct fmn_sprite *flame=fmn_sprite_generate_noparam(0,sprite->x+((sprite->xform&FMN_XFORM_XREV)?-1.0f:1.0f),sprite->y);
  if (flame) {
    flame->imageid=sprite->imageid;
    flame->tileid=tileid0+4;
    flame->xform=sprite->xform;
    flame->style=FMN_SPRITE_STYLE_TWOFRAME;
    flame->pv[0]=sprite;
    if (fmn_global.illumination_time<2.0f) fmn_global.illumination_time=2.0f;
  }
}

static void lizard_burn_end(struct fmn_sprite *sprite) {
  burning=0;
  sprite->tileid=tileid0;
  fireclock=0.0f;
  walkclock=0.0f;
  sprite->invmass=invmass0;
  sprite->physics|=FMN_PHYSICS_BLOWABLE;
  struct fmn_sprite *flame=lizard_find_flame(sprite);
  if (flame) fmn_sprite_kill(flame);
}

/* We are currently burning. Check hero and injure as warranted.
 */
 
static void lizard_check_injury(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float x=sprite->x+((sprite->xform&FMN_XFORM_XREV)?-LIZARD_BURN_OFFSET:LIZARD_BURN_OFFSET);
  float y=sprite->y;
  float dx=herox-x; if (dx<0.0f) dx=-dx;
  float dy=heroy-y; if (dy<0.0f) dy=-dy;
  float radius=LIZARD_BURN_RADIUS;
  if (dx*dx+dy*dy>=radius*radius) return;
  fmn_hero_injure(x,y,sprite);
}

/* Adjust my velocity, to walk toward the hero.
 */
 
static void lizard_update_velocity(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  
  if (herox>sprite->x) sprite->xform=0;
  else sprite->xform=FMN_XFORM_XREV;
  
  float targety=heroy;
  if (fmn_global.facedir==FMN_DIR_N) targety-=LIZARD_TARGET_Y_OFFSET;
  else if (fmn_global.facedir==FMN_DIR_S) targety+=LIZARD_TARGET_Y_OFFSET;
  float targetx=herox;
  if (sprite->x<herox-0.5f) targetx-=LIZARD_TARGET_X_OFFSET;
  else if (sprite->x>herox+0.5f) targetx+=LIZARD_TARGET_X_OFFSET;
  
  float remainx=targetx-sprite->x;
  float remainy=targety-sprite->y;
  float distance=sqrtf(remainx*remainx+remainy*remainy);
  if (distance<0.5f) {
    if (sprite->velx>=LIZARD_DECEL_RATE) sprite->velx-=LIZARD_DECEL_RATE;
    else if (sprite->velx<=-LIZARD_DECEL_RATE) sprite->velx+=LIZARD_DECEL_RATE;
    else sprite->velx=0.0f;
    if (sprite->vely>=LIZARD_DECEL_RATE) sprite->vely-=LIZARD_DECEL_RATE;
    else if (sprite->vely<=-LIZARD_DECEL_RATE) sprite->vely+=LIZARD_DECEL_RATE;
    else sprite->vely=0.0f;
    return;
  }
  
  float idealx=(remainx*LIZARD_WALK_SPEED)/distance;
  float idealy=(remainy*LIZARD_WALK_SPEED)/distance;
  float adjx=idealx-sprite->velx;
  float adjy=idealy-sprite->vely;
  if (adjx>=LIZARD_ACCEL_RATE) sprite->velx+=LIZARD_ACCEL_RATE;
  else if (adjx<=-LIZARD_ACCEL_RATE) sprite->velx-=LIZARD_ACCEL_RATE;
  else sprite->velx=idealx;
  if (adjy>=LIZARD_ACCEL_RATE) sprite->vely+=LIZARD_ACCEL_RATE;
  else if (adjy<=-LIZARD_ACCEL_RATE) sprite->vely-=LIZARD_ACCEL_RATE;
  else sprite->vely=idealy;
}

/* Update.
 */
 
static void _lizard_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+6;
    fireclock=0.0f;
    return;
  }
  
  if (burning) {
    fireclock+=elapsed;
    // I didn't want this, but alas it does seem necessary: update the flame's position every time.
    struct fmn_sprite *flame=lizard_find_flame(sprite);
    if (flame) {
      flame->x=sprite->x+((sprite->xform&FMN_XFORM_XREV)?-1.0f:1.0f);
      flame->y=sprite->y;
    }
    if (fireclock>=2.0f) lizard_burn_end(sprite);
    else lizard_check_injury(sprite);
    return;
  }
  
  lizard_update_velocity(sprite);

  if ((sprite->velx<-0.1f)||(sprite->velx>0.1f)||(sprite->vely<-0.1f)||(sprite->vely>0.1f)) {
    clock+=elapsed;
    if (clock>=0.6f) clock-=0.6f;
    if (clock>=0.3f) sprite->tileid=tileid0+2;
    else sprite->tileid=tileid0+1;
    fireclock=0.0f;
    walkclock+=elapsed;
    if (walkclock>6.0f) { // we've walked a long time. let's start a fire to keep amused.
      lizard_burn(sprite);
    }
  } else {
    walkclock=0.0f;
    sprite->tileid=tileid0;
    fireclock+=elapsed;
    if (fireclock>=1.0f) lizard_burn(sprite);
  }
}

/* Sleep state.
 */
 
static void lizard_sleep(struct fmn_sprite *sprite) {
  if (sleeping) return;
  sleeping=1;
  fmn_sprite_generate_zzz(sprite);
  lizard_burn_end(sprite);
}

static void lizard_wake(struct fmn_sprite *sprite) {
  if (!sleeping) return;
  sleeping=0;
}

/* Interact.
 */
 
static int16_t _lizard_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: lizard_sleep(sprite); break;
        case FMN_SPELLID_REVEILLE: lizard_wake(sprite); break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: lizard_wake(sprite); break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_lizard={
  .init=_lizard_init,
  .update=_lizard_update,
  .interact=_lizard_interact,
};
