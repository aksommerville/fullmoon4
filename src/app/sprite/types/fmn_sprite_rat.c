#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define RAT_STAGE_DECIDE 0
#define RAT_STAGE_WAIT 1
#define RAT_STAGE_RUN 2

#define RAT_LINE_OF_SIGHT_RADIUS 0.5f
#define RAT_CHARGE_SPEED 3.0f

#define sleeping sprite->bv[0]
#define tileid0 sprite->bv[1]
#define stage sprite->bv[2]
#define animclock sprite->fv[0]
#define rundx sprite->fv[1]
#define rundy sprite->fv[2]
#define stageclock sprite->fv[3]

/* Init.
 */
 
static void _rat_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  stage=RAT_STAGE_DECIDE;
}

/* Select a run direction and enter WAIT stage.
 */
 
static void rat_update_DECIDE(struct fmn_sprite *sprite) {
  stage=RAT_STAGE_WAIT;
  stageclock=0.5+(rand()%1000)/500.0f;
  rundx=((rand()%1000)-500)/200.0f;
  rundy=((rand()%1000)-500)/200.0f;
  if (rundx<0.0f) sprite->xform=0;
  else sprite->xform=FMN_XFORM_XREV;
}

/* Run down the stage clock.
 */
 
static void rat_update_WAIT(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0;
  if ((stageclock-=elapsed)<=0.0f) {
    stage=RAT_STAGE_RUN;
    stageclock=1.0f+(rand()%1000)/500.0f;
  }
}

/* Run.
 */
 
static void rat_update_RUN(struct fmn_sprite *sprite,float elapsed) {
  if ((stageclock-=elapsed)<=0.0f) {
    stage=RAT_STAGE_DECIDE;
    return;
  }
  animclock+=elapsed;
  if (animclock>=0.7f) {
    animclock-=0.7f;
  }
  sprite->tileid=tileid0+((animclock>=0.35f)?0x10:0);
  sprite->x+=rundx*elapsed;
  sprite->y+=rundy*elapsed;
}

/* Charge.
 */
 
static void rat_charge(struct fmn_sprite *sprite,float elapsed) {
  animclock+=elapsed;
  if (animclock>=0.7f) {
    animclock-=0.7f;
  }
  sprite->tileid=tileid0+((animclock>=0.35f)?0x10:0);
  if (sprite->xform&FMN_XFORM_XREV) {
    sprite->x+=RAT_CHARGE_SPEED*elapsed;
  } else {
    sprite->x-=RAT_CHARGE_SPEED*elapsed;
  }
}

/* Update.
 */
 
static void _rat_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+0x20;
    animclock=0.0f;
    return;
  }
  
  // Regardless of (stage), any time we're awake, we can charge the hero.
  if (fmn_global.invisibility_time<=0.0f) {
    float herox,heroy;
    fmn_hero_get_position(&herox,&heroy);
    uint8_t sighted=1;
    if (sprite->xform&FMN_XFORM_XREV) {
      if (herox<sprite->x) sighted=0;
    } else {
      if (herox>sprite->x) sighted=0;
    }
    if (sighted) {
      float dy=heroy-sprite->y;
      if ((dy<RAT_LINE_OF_SIGHT_RADIUS)&&(dy>-RAT_LINE_OF_SIGHT_RADIUS)) {
        rat_charge(sprite,elapsed);
        return;
      }
    }
  }
  
  switch (stage) {
    case RAT_STAGE_DECIDE: rat_update_DECIDE(sprite); break;
    case RAT_STAGE_WAIT: rat_update_WAIT(sprite,elapsed); break;
    case RAT_STAGE_RUN: rat_update_RUN(sprite,elapsed); break;
  }
}

/* Interact.
 */
 
static int16_t _rat_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping) { sleeping=1; fmn_sprite_generate_zzz(sprite); } break;
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_rat={
  .init=_rat_init,
  .update=_rat_update,
  .interact=_rat_interact,
};
