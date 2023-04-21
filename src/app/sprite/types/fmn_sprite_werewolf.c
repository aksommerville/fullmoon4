#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include <math.h>

#define WEREWOLF_STAGE_IDLE 0
#define WEREWOLF_STAGE_SLEEP 1
#define WEREWOLF_STAGE_ERECT 2
#define WEREWOLF_STAGE_WALK 3
#define WEREWOLF_STAGE_GROWL 4 /* introduces POUNCE */
#define WEREWOLF_STAGE_POUNCE 5
#define WEREWOLF_STAGE_HADOUKEN_CHARGE 6
#define WEREWOLF_STAGE_HADOUKEN_FOLLOWTHRU 7
#define WEREWOLF_STAGE_FLOORFIRE_CHARGE 8
#define WEREWOLF_STAGE_FLOORFIRE_FOLLOWTHRU 9
#define WEREWOLF_STAGE_EAT 10
#define WEREWOLF_STAGE_SHOCK 11
#define WEREWOLF_STAGE_DEAD 12

#define WEREWOLF_IDLE_TIME_MIN 0.25f /* uninterruptible */
#define WEREWOLF_IDLE_TIME_MAX 1.0f /* we may interrupt with POUNCE or WALK before this */

// Relative odds of next stage, if the idle timer expires. (can WALK or POUNCE via different triggers)
#define WEREWOLF_ODDS_WALK 30
#define WEREWOLF_ODDS_ATTACK 40

#define WEREWOLF_HADOUKEN_DISTANCE_MINIMUM_X 2.0f /* must be so far away before we'll consider hadouking */
#define WEREWOLF_HADOUKEN_CONE_SLOPE 2.0f /* 1.0f=45deg, higher is narrower */
#define WEREWOLF_HADOUKEN_SPEED 13.0f
#define WEREWOLF_HADOUKEN_CHARGE_TIME 0.5f
#define WEREWOLF_HADOUKEN_FOLLOWTHRU_TIME 1.0f

#define WEREWOLF_FLOORFIRE_CHARGE_TIME 1.0f
#define WEREWOLF_FLOORFIRE_FOLLOWTHRU_TIME 1.5f

#define WEREWOLF_WALK_MARGIN_X 3.0f /* Walk destination, min distance to horz edges. (horz is symmetric) */
#define WEREWOLF_WALK_MARGIN_TOP 2.5f
#define WEREWOLF_WALK_MARGIN_BOTTOM 1.5f
#define WEREWOLF_WALK_MINIMUM_DISTANCE 2.0f
#define WEREWOLF_WALK_SPEED 6.0f
#define WEREWOLF_WALK_PANIC_TIME 3.0f

#define WEREWOLF_GROWL_TIME 0.5f
#define WEREWOLF_POUNCE_MAXIMUM_DISTANCE_X 4.0f
#define WEREWOLF_POUNCE_MAXIMUM_DISTANCE_Y 2.0f
#define WEREWOLF_POUNCE_CONE_SLOPE 2.0f
#define WEREWOLF_POUNCE_SPEED 12.0f
#define WEREWOLF_POUNCE_TIME 0.5f
#define WEREWOLF_POUNCE_RANGE_UP -1.0f
#define WEREWOLF_POUNCE_RANGE_DOWN 1.0f
#define WEREWOLF_POUNCE_RANGE_HORZ 1.0f

// When picking a move, if you are inside the Cone of Hadouken, and between POUNCE_MAX and MANDATORY_X (4..6), no walking, always attack.
// This is an effort to create a deterministic path for speed runners.
#define WEREWOLF_MANDATORY_ATTACK_DISTANCE_X 6.0f

#define WEREWOLF_SHOCK_TIME 6.0f

#define WEREWOLF_HBW_FOURS 0.50f
#define WEREWOLF_HBE_FOURS 1.00f
#define WEREWOLF_HBN_FOURS 1.30f
#define WEREWOLF_HBS_FOURS 0.00f
#define WEREWOLF_HBW_UPRIGHT -0.20f
#define WEREWOLF_HBE_UPRIGHT 1.80f
#define WEREWOLF_HBN_UPRIGHT 1.30f
#define WEREWOLF_HBS_UPRIGHT 0.25f
#define WEREWOLF_HBW_BACK 0.50f
#define WEREWOLF_HBE_BACK 1.00f
#define WEREWOLF_HBN_BACK 0.75f
#define WEREWOLF_HBS_BACK 0.20f

#define stage sprite->bv[0]
#define floorfire_running sprite->bv[1]
#define next_attack sprite->bv[2]
#define clock sprite->fv[0]
#define stage_clock sprite->fv[1]
#define walkdstx sprite->fv[2]
#define walkdsty sprite->fv[3]
#define walkdx sprite->fv[4]
#define walkdy sprite->fv[5]

/* Reset hitbox.
 */
 
#define WEREWOLF_SET_HITBOX(tag) { \
  if (sprite->xform&FMN_XFORM_XREV) { \
    sprite->hbw=WEREWOLF_HBE_##tag; \
    sprite->hbe=WEREWOLF_HBW_##tag; \
  } else { \
    sprite->hbw=WEREWOLF_HBW_##tag; \
    sprite->hbe=WEREWOLF_HBE_##tag; \
  } \
  sprite->hbn=WEREWOLF_HBN_##tag; \
  sprite->hbs=WEREWOLF_HBS_##tag; \
}

#define WEREWOLF_FLOP_HITBOX { \
  float tmp=sprite->hbw; \
  sprite->hbw=sprite->hbe; \
  sprite->hbe=tmp; \
}

/* Init.
 */
 
static void _werewolf_init(struct fmn_sprite *sprite) {
  stage=WEREWOLF_STAGE_IDLE;
  WEREWOLF_SET_HITBOX(FOURS)
  sprite->physics=FMN_PHYSICS_EDGE|FMN_PHYSICS_SOLID|FMN_PHYSICS_HOLE|FMN_PHYSICS_SPRITES;
  sprite->invmass=0;
  next_attack=WEREWOLF_STAGE_HADOUKEN_CHARGE;
}

/* Floorfire.
 * This is not updated with the FLOORFIRE_FOLLOWTHRU stage as you might expect; it can run longer than the stage.
 */
 
static void werewolf_floorfire_restart(struct fmn_sprite *sprite) {
  float x=sprite->x;
  if (sprite->xform&FMN_XFORM_XREV) x-=0.5f;
  else x+=0.5f;
  struct fmn_sprite *floorfire=fmn_sprite_generate_noparam(FMN_SPRCTL_floorfire,sprite->x,sprite->y);
  if (!floorfire) return;
  floorfire->imageid=sprite->imageid;
}

/* Nonzero if we are currently being tickled.
 */
 
static uint8_t werewolf_is_tickling(struct fmn_sprite *sprite) {
  if (fmn_global.active_item!=FMN_ITEM_FEATHER) return 0;
  //TODO examine hero position and direction
  return 0;
}

/* Nonzero if the hero is in my Cone Of Hadouken.
 */
 
static uint8_t werewolf_in_hadouken_position(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x;
  float dy=heroy-sprite->y;
  if (dy<0.0f) dy=-dy; // the question is vertically symmetric. make it positive always for simplicity.
  if (!(sprite->xform&FMN_XFORM_XREV)) dx=-dx; // ditto dx, minding our face direction. this can be negative if she's behind us.
  if (dx<WEREWOLF_HADOUKEN_DISTANCE_MINIMUM_X) return 0;
  if (dx<dy*WEREWOLF_HADOUKEN_CONE_SLOPE) return 0;
  return 1;
}

/* Sleep or wake.
 */
 
static void werewolf_sleep(struct fmn_sprite *sprite,uint8_t newstate) {
  if (newstate) {
    if (stage==WEREWOLF_STAGE_SLEEP) return;
    if (stage==WEREWOLF_STAGE_EAT) return;
    if (stage==WEREWOLF_STAGE_DEAD) return;
    WEREWOLF_SET_HITBOX(BACK)
    stage=WEREWOLF_STAGE_SLEEP;
    stage_clock=0.0f;
    struct fmn_sprite *zzz=fmn_sprite_generate_zzz(sprite);
    if (zzz) {
      zzz->x-=0.25f;
      zzz->fv[1]-=0.25f; // x0
      zzz->y-=1.0f; // not 0.5; (y) gets corrected in weird and broken ways. whatever
      zzz->fv[2]-=0.5f; // y0
      zzz->pv[0]=0; // source; don't let it second-guess our adjustment. a sleeping werewolf can't move, so it's cool.
    }
  } else {
    if (stage!=WEREWOLF_STAGE_SLEEP) return;
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_IDLE;
    stage_clock=0.0f;
  }
}

/* Set XREV to make us point at the hero.
 */
 
static void werewolf_face_hero(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x;
  // Change when she's a certain threshold beyond our (x), don't let it be fast toggleable. Not that it matters.
  const float threshold=1.0f;
  if (sprite->xform&FMN_XFORM_XREV) {
    if (dx<-threshold) {
      sprite->xform&=~FMN_XFORM_XREV;
      WEREWOLF_FLOP_HITBOX
    }
  } else {
    if (dx>threshold) {
      sprite->xform|=FMN_XFORM_XREV;
      WEREWOLF_FLOP_HITBOX
    }
  }
}

/* If the hero is positioned pounceably, begin POUNCE stage and return nonzero.
 */
 
static uint8_t werewolf_pounce_if_reasonable(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x;
  if (!(sprite->xform&FMN_XFORM_XREV)) dx=-dx;
  if (dx>WEREWOLF_POUNCE_MAXIMUM_DISTANCE_X) return 0;
  float dy=heroy-sprite->y;
  if (dy<0.0f) dy=-dy;
  if (dy>WEREWOLF_POUNCE_MAXIMUM_DISTANCE_Y) return 0;
  if (dx<dy*WEREWOLF_POUNCE_CONE_SLOPE) return 0;
  WEREWOLF_SET_HITBOX(FOURS)
  stage=WEREWOLF_STAGE_GROWL;
  stage_clock=0.0f;
  return 1;
}

/* Select a destination and enter WALK stage.
 * Nonzero if we do. Zero if we judge it too short, and retain current stage.
 */
 
static uint8_t werewolf_begin_WALK(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float halfscreenw=FMN_COLC*0.5f;
  if (herox>halfscreenw) {
    walkdstx=herox-halfscreenw;
    if (walkdstx<WEREWOLF_WALK_MARGIN_X) walkdstx=WEREWOLF_WALK_MARGIN_X;
  } else {
    walkdstx=herox+halfscreenw;
    if (walkdstx>FMN_COLC-WEREWOLF_WALK_MARGIN_X) walkdstx=FMN_COLC-WEREWOLF_WALK_MARGIN_X;
  }
  walkdsty=heroy;
  if (walkdsty<WEREWOLF_WALK_MARGIN_TOP) walkdsty=WEREWOLF_WALK_MARGIN_TOP;
  else if (walkdsty>FMN_ROWC-WEREWOLF_WALK_MARGIN_BOTTOM) walkdsty=FMN_ROWC-WEREWOLF_WALK_MARGIN_BOTTOM;
  float dx=walkdstx-sprite->x;
  float dy=walkdsty-sprite->y;
  float distance=sqrtf(dx*dx+dy*dy);
  if (distance<WEREWOLF_WALK_MINIMUM_DISTANCE) return 0;
  walkdx=(dx*WEREWOLF_WALK_SPEED)/distance;
  walkdy=(dy*WEREWOLF_WALK_SPEED)/distance;
  WEREWOLF_SET_HITBOX(FOURS)
  stage=WEREWOLF_STAGE_WALK;
  stage_clock=0.0f;
  return 1;
}
 
// same idea, but always change state.
static void werewolf_begin_WALK_random(struct fmn_sprite *sprite) {

  // Select the further by manhattan distance of 3 points: hero, w/3, 2w/3.
  float herox,heroy,dx,dy;
  fmn_hero_get_position(&herox,&heroy);
  float refx=FMN_COLC/3.0f,refy=FMN_ROWC*0.5f;
  if (sprite->x<FMN_COLC*0.5f) refx*=2.0f;
  dx=herox-sprite->x; if (dx<0.0f) dx=-dx;
  dy=heroy-sprite->y; if (dy<0.0f) dy=-dy;
  float herodistance=dx+dy;
  dx=refx-sprite->x; if (refx<0.0f) refx=-refx;
  dy=refy-sprite->y; if (refy<0.0f) refy=-refy;
  float refdistance=dx+dy;
  if (herodistance>refdistance) {
    walkdstx=herox;
    walkdsty=heroy;
  } else {
    walkdstx=refx;
    walkdsty=refy;
  }
  
  dx=walkdstx-sprite->x;
  dy=walkdsty-sprite->y;
  float distance=sqrtf(dx*dx+dy*dy);
  if (distance<1.0f) distance=1.0f; // shouldn't be possible but play it safe
  walkdx=(dx*WEREWOLF_WALK_SPEED)/distance;
  walkdy=(dy*WEREWOLF_WALK_SPEED)/distance;
  WEREWOLF_SET_HITBOX(FOURS)
  stage=WEREWOLF_STAGE_WALK;
  stage_clock=0.0f;
}

/* Enter HADOUKEN or FLOORFIRE stages.
 */
 
static void werewolf_begin_HADOUKEN(struct fmn_sprite *sprite) {
  WEREWOLF_SET_HITBOX(UPRIGHT)
  stage=WEREWOLF_STAGE_HADOUKEN_CHARGE;
  stage_clock=0.0f;
}

static void werewolf_begin_FLOORFIRE(struct fmn_sprite *sprite) {
  WEREWOLF_SET_HITBOX(UPRIGHT)
  stage=WEREWOLF_STAGE_FLOORFIRE_CHARGE;
  stage_clock=0.0f;
}

/* Enter EAT stage.
 */
 
static void werewolf_begin_EAT(struct fmn_sprite *sprite) {
  WEREWOLF_SET_HITBOX(FOURS)
  stage=WEREWOLF_STAGE_EAT;
  stage_clock=0.0f;
  fmn_hero_kill(sprite);
  
  float x=sprite->x,y=sprite->y;
  if (sprite->xform&FMN_XFORM_XREV) x+=0.7f;
  else x-=0.7f;
  struct fmn_sprite *deadwitch=fmn_sprite_generate_noparam(FMN_SPRCTL_deadwitch,x,y);
  if (deadwitch) {
    deadwitch->imageid=sprite->imageid;
    deadwitch->tileid=0x96;
    deadwitch->xform=sprite->xform&FMN_XFORM_XREV;
  }
  
  if (sprite->xform&FMN_XFORM_XREV) x=sprite->x+2.0f;
  else x=sprite->x-2.0f;
  y=sprite->y-1.5f;
  struct fmn_sprite *losthat=fmn_sprite_generate_noparam(FMN_SPRCTL_losthat,x,y);
  if (losthat) {
    losthat->imageid=sprite->imageid;
    losthat->tileid=0x86;
  }
}

/* Check for missiles.
 * If we are struck, enter SHOCK stage and return nonzero.
 */
 
static int werewolf_check_missiles_1(struct fmn_sprite *q,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (q->controller!=FMN_SPRCTL_missile) return 0;
  
  // Missile must be moving toward me. That will always be the case for a reflection, and it neatly prevents my own hadoukens hitting me on the way out.
  if (q->fv[1]<0.0f) {
    if (q->x<=sprite->x) return 0;
  } else {
    if (q->x>=sprite->x) return 0;
  }
  
  //TODO use hitbox
  float dx=q->x-sprite->x;
  if ((dx<-1.0f)||(dx>1.0f)) return 0;
  float dy=q->y-sprite->y;
  if (dy<-1.5f) return 0;
  if (dy>0.0f) return 0;
  
  // Struck!
  return 1;
}
 
static uint8_t werewolf_check_missiles(struct fmn_sprite *sprite) {
  if (!fmn_sprites_for_each(werewolf_check_missiles_1,sprite)) return 0;
  WEREWOLF_SET_HITBOX(BACK)
  stage=WEREWOLF_STAGE_SHOCK;
  stage_clock=0.0f;
  return 1;
}

/* Enter DEAD stage and trigger all the fireworks.
 */
 
static void werewolf_die(struct fmn_sprite *sprite) {
  fmn_log_event("kill-werewolf","");
  fmn_sprite_generate_soulballs(sprite->x,sprite->y,6);
  fmn_sound_effect(FMN_SFX_KILL_WEREWOLF);
  WEREWOLF_SET_HITBOX(BACK)
  stage=WEREWOLF_STAGE_DEAD;
  stage_clock=0.0f;
  fmn_global.werewolf_dead=1;
  fmn_global.terminate_time=5.0f;
}

/* Ready to attack or walk. Check the mandatory attack distance.
 */
 
static int werewolf_within_mandatory_attack_distance(struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=sprite->x-herox;
  if (dx<0.0f) dx=-dx;
  if (dx>=WEREWOLF_MANDATORY_ATTACK_DISTANCE_X) return 0;
  return 1;
}

/* Update, IDLE.
 */
 
static void werewolf_update_IDLE(struct fmn_sprite *sprite,float elapsed) {
  
  // If the hero is dead, stay in IDLE.
  if (fmn_global.hero_dead) return;
  
  if (werewolf_check_missiles(sprite)) return;
  
  werewolf_face_hero(sprite);
  
  // Mostly just to prevent frame-flashing, there's a brief minimum idle time, uninterruptible.
  stage_clock+=elapsed;
  if (stage_clock<WEREWOLF_IDLE_TIME_MIN) return;
  
  // If we're outside the margins, try walking.
  // (this can happen after a pounce)
  if (
    (sprite->x<WEREWOLF_WALK_MARGIN_X)||
    (sprite->x>FMN_COLC-WEREWOLF_WALK_MARGIN_X)||
    (sprite->y<WEREWOLF_WALK_MARGIN_TOP)||
    (sprite->y>FMN_ROWC-WEREWOLF_WALK_MARGIN_BOTTOM)
  ) {
    if (werewolf_begin_WALK(sprite)) return;
  }
  
  // If we're situated for a POUNCE, that's ideal.
  if (werewolf_pounce_if_reasonable(sprite)) return;
  
  // If the hero is not in my cone of Hadouken, walk to her latitude and a goodly distance horizontally.
  uint8_t hadoukenable=werewolf_in_hadouken_position(sprite);
  if (!hadoukenable) {
    if (werewolf_begin_WALK(sprite)) return;
  }
  
  // We are hadoukenable (probably), and not pounceable.
  // Wait a tasteful interval, then walk, hadouken, or floorfire, at random.
  if (stage_clock<WEREWOLF_IDLE_TIME_MAX) return;
  
  // If you're standing close, but not pounceably-close, always attack.
  // If she chickened out and left a safe distance, we may attack or walk.
  if (!werewolf_within_mandatory_attack_distance(sprite)) {
    #define oddssum (WEREWOLF_ODDS_WALK+WEREWOLF_ODDS_ATTACK)
    int16_t choice=rand()%oddssum;
    #undef oddssum
    if (choice<WEREWOLF_ODDS_WALK) {
      werewolf_begin_WALK_random(sprite);
      return;
    }
  }
  
  // Attack: Alternate between hadouken and floorfire.
  switch (next_attack) {
    case WEREWOLF_STAGE_HADOUKEN_CHARGE: {
        next_attack=WEREWOLF_STAGE_FLOORFIRE_CHARGE;
        werewolf_begin_HADOUKEN(sprite);
      } break;
    case WEREWOLF_STAGE_FLOORFIRE_CHARGE: {
        next_attack=WEREWOLF_STAGE_HADOUKEN_CHARGE;
        werewolf_begin_FLOORFIRE(sprite);
      } break;
  }
}

/* Update, ERECT.
 * This might be just a graphics test.
 * Unclear what he's doing up on his hind legs. Begging for a treat or something.
 */
 
static void werewolf_update_ERECT(struct fmn_sprite *sprite,float elapsed) {
  if (werewolf_check_missiles(sprite)) return;
  werewolf_face_hero(sprite);
  stage_clock+=elapsed;
  if (stage_clock>=3.0f) {
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_IDLE;
    stage_clock=0.0f;
  }
}

/* Update, SLEEP.
 */
 
static int werewolf_terminate_zzz(struct fmn_sprite *q,void *dummy) {
  if (q->controller!=FMN_SPRCTL_zzz) return 0;
  // It's OK to kill every zzz; the werewolf is always alone, so it could only be his.
  fmn_sprite_kill(q);
  return 0;
}
 
static void werewolf_update_SLEEP(struct fmn_sprite *sprite,float elapsed) {
  // In addition to the usual REVEILLE and BELL, the feather can wake us up.
  // In which case, eliminating the zzz is our responsibility.
  if (werewolf_is_tickling(sprite)) {
    werewolf_sleep(sprite,0);
    fmn_sprites_for_each(werewolf_terminate_zzz,0);
    return;
  }
  //TODO Set a timer and wake up automatically? I think that would be pretty scary.
}

/* Update, WALK.
 */
 
static void werewolf_update_WALK(struct fmn_sprite *sprite,float elapsed) {
  if (werewolf_check_missiles(sprite)) return;
  werewolf_face_hero(sprite);
  stage_clock+=elapsed;
  if (stage_clock>WEREWOLF_WALK_PANIC_TIME) {
    // we've been walking for too long. Return to IDLE to reassess.
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_IDLE;
    stage_clock=0.0f;
    return;
  }
  
  // We can interrupt the walk with a pounce at any time.
  if (werewolf_pounce_if_reasonable(sprite)) return;
  
  uint8_t moved=0;
  if (sprite->x>walkdstx) {
    if ((sprite->x+=walkdx*elapsed)<=walkdstx) sprite->x=walkdstx;
    else moved=1;
  } else {
    if ((sprite->x+=walkdx*elapsed)>=walkdstx) sprite->x=walkdstx;
    else moved=1;
  }
  if (sprite->y>walkdsty) {
    if ((sprite->y+=walkdy*elapsed)<=walkdsty) sprite->y=walkdsty;
    else moved=1;
  } else {
    if ((sprite->y+=walkdy*elapsed)>=walkdsty) sprite->y=walkdsty;
    else moved=1;
  }
  if (!moved) {
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_IDLE;
    stage_clock=0.0f;
  }
}

/* Update GROWL and POUNCE.
 */
 
static void werewolf_update_GROWL(struct fmn_sprite *sprite,float elapsed) {
  if (werewolf_check_missiles(sprite)) return;
  stage_clock+=elapsed;
  if (stage_clock>=WEREWOLF_GROWL_TIME) {
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_POUNCE;
    stage_clock=0.0f;
  }
}

static void werewolf_update_POUNCE(struct fmn_sprite *sprite,float elapsed) {
  if (werewolf_check_missiles(sprite)) return;
  stage_clock+=elapsed;
  if (stage_clock>=WEREWOLF_POUNCE_TIME) {
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_IDLE;
    stage_clock=0.0f;
    return;
  }
  if (sprite->xform&FMN_XFORM_XREV) {
    sprite->x+=WEREWOLF_POUNCE_SPEED*elapsed;
  } else {
    sprite->x-=WEREWOLF_POUNCE_SPEED*elapsed;
  }
  if ((sprite->x<1.0f)||(sprite->x>=FMN_COLC-1.0f)) {
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_IDLE;
    stage_clock=0.0f;
    return;
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x;
  float dy=heroy-sprite->y;
  if (dy<WEREWOLF_POUNCE_RANGE_UP) return;
  if (dy>WEREWOLF_POUNCE_RANGE_DOWN) return;
  if (dx<-WEREWOLF_POUNCE_RANGE_HORZ) return;
  if (dx>WEREWOLF_POUNCE_RANGE_HORZ) return;
  werewolf_begin_EAT(sprite);
}

/* Toss the Hadouken.
 * Up to this point, it was a decorative part of our sprite.
 * Here we make its own sprite and set it free.
 */
 
static void werewolf_hadouken_commit(struct fmn_sprite *sprite) {
  float x=sprite->x,y=sprite->y-1.0f;
  struct fmn_sprite *missile=fmn_sprite_generate_noparam(FMN_SPRCTL_missile,x,y);
  if (!missile) return;
  
  // Don't trust FMN_SPRCTL_missile's targetting.
  // We want to enforce our own cone-of-hadouken, and it's ok if it's going to miss.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-missile->x;
  float dy=heroy-missile->y;
  float adx=(dx<0.0f)?-dx:dx;
  float ady=(dy<0.0f)?-dy:dy;
  if (adx+ady<1.0f) dx=adx=1.0f; // don't let it be very small
  if (adx<ady*WEREWOLF_HADOUKEN_CONE_SLOPE) adx=dx=ady*WEREWOLF_HADOUKEN_CONE_SLOPE;
  float distance=sqrtf(dx*dx+dy*dy);
  if (herox>sprite->x) {
    if (dx<0.0f) dx=-dx;
  } else {
    if (dx>0.0f) dx=-dx;
  }
  
  missile->fv[0]=WEREWOLF_HADOUKEN_SPEED;
  missile->fv[1]=(dx*WEREWOLF_HADOUKEN_SPEED)/distance;
  missile->fv[2]=(dy*WEREWOLF_HADOUKEN_SPEED)/distance;
  missile->bv[0]=1; // ready
  missile->imageid=sprite->imageid;
  missile->tileid=0x74;
}

/* Begin floor fire.
 */
 
static void werewolf_floorfire_commit(struct fmn_sprite *sprite) {
  werewolf_floorfire_restart(sprite);
}

/* Update, HADOUKEN and FLOORFIRE.
 * Same idea with both. When the CHARGE timer lapses, advance to the next stage and call (commit).
 * When the FOLLOWTHRU timer lapses, return to idle.
 */
 
static void werewolf_update_CHARGE(struct fmn_sprite *sprite,float elapsed,float duration,void (*commit)(struct fmn_sprite *sprite)) {
  if (werewolf_check_missiles(sprite)) return;
  stage_clock+=elapsed;
  if (stage_clock<duration) return;
  commit(sprite);
  stage++;
  stage_clock=0.0f;
}

static void werewolf_update_FOLLOWTHRU(struct fmn_sprite *sprite,float elapsed,float duration) {
  if (werewolf_check_missiles(sprite)) return;
  stage_clock+=elapsed;
  if (stage_clock<duration) return;
  WEREWOLF_SET_HITBOX(FOURS)
  stage=WEREWOLF_STAGE_IDLE;
  stage_clock=0.0f;
}

/* Update, final stages: EAT and DEAD.
 */
 
static void werewolf_update_EAT(struct fmn_sprite *sprite,float elapsed) {
  stage_clock+=elapsed;
}

static void werewolf_update_DEAD(struct fmn_sprite *sprite,float elapsed) {
}

/* Update, SHOCK stage.
 */
 
static void werewolf_update_SHOCK(struct fmn_sprite *sprite,float elapsed) {
  stage_clock+=elapsed;
  if (stage_clock>=WEREWOLF_SHOCK_TIME) {
    WEREWOLF_SET_HITBOX(FOURS)
    stage=WEREWOLF_STAGE_IDLE;
    stage_clock=0.0f;
    return;
  }
}

/* Update.
 */
 
static void _werewolf_update(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  switch (stage) {
    case WEREWOLF_STAGE_IDLE: werewolf_update_IDLE(sprite,elapsed); break;
    case WEREWOLF_STAGE_SLEEP: werewolf_update_SLEEP(sprite,elapsed); break;
    case WEREWOLF_STAGE_ERECT: werewolf_update_ERECT(sprite,elapsed); break;
    case WEREWOLF_STAGE_WALK: werewolf_update_WALK(sprite,elapsed); break;
    case WEREWOLF_STAGE_GROWL: werewolf_update_GROWL(sprite,elapsed); break;
    case WEREWOLF_STAGE_POUNCE: werewolf_update_POUNCE(sprite,elapsed); break;
    case WEREWOLF_STAGE_HADOUKEN_CHARGE: werewolf_update_CHARGE(sprite,elapsed,WEREWOLF_HADOUKEN_CHARGE_TIME,werewolf_hadouken_commit); break;
    case WEREWOLF_STAGE_FLOORFIRE_CHARGE: werewolf_update_CHARGE(sprite,elapsed,WEREWOLF_FLOORFIRE_CHARGE_TIME,werewolf_floorfire_commit); break;
    case WEREWOLF_STAGE_HADOUKEN_FOLLOWTHRU: werewolf_update_FOLLOWTHRU(sprite,elapsed,WEREWOLF_HADOUKEN_FOLLOWTHRU_TIME); break;
    case WEREWOLF_STAGE_FLOORFIRE_FOLLOWTHRU: werewolf_update_FOLLOWTHRU(sprite,elapsed,WEREWOLF_FLOORFIRE_FOLLOWTHRU_TIME); break;
    case WEREWOLF_STAGE_EAT: werewolf_update_EAT(sprite,elapsed); break;
    case WEREWOLF_STAGE_SHOCK: werewolf_update_SHOCK(sprite,elapsed); break;
    case WEREWOLF_STAGE_DEAD: werewolf_update_DEAD(sprite,elapsed); break;
  }
}

/* Feather.
 */

static void werewolf_feather(struct fmn_sprite *sprite) {
  switch (stage) {
    case WEREWOLF_STAGE_SHOCK: werewolf_die(sprite); break;
    case WEREWOLF_STAGE_SLEEP: werewolf_sleep(sprite,0); break;
  }
}

/* Interact.
 */
 
static int16_t _werewolf_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: werewolf_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: werewolf_sleep(sprite,0); break;
      } break;
    case FMN_ITEM_BELL: werewolf_sleep(sprite,0); break;
    case FMN_ITEM_FEATHER: werewolf_feather(sprite); break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_werewolf={
  .init=_werewolf_init,
  .update=_werewolf_update,
  .interact=_werewolf_interact,
};
