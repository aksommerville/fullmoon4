#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include <math.h>

// Don't change stages; renderer depends on them!
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

#define WEREWOLF_HADOUKEN_CONE_SLOPE 2.0f /* 1.0f=45deg, higher is narrower */
#define WEREWOLF_HADOUKEN_SPEED 13.0f

#define stage sprite->bv[0]
#define next_move sprite->bv[1] /* 0,1,2,3 = hadouken,walk,floorfire,walk */
#define stageclock sprite->fv[1]
#define herox sprite->fv[2] /* we use it a lot */
#define heroy sprite->fv[3]
#define walkdx sprite->fv[4]
#define walkdy sprite->fv[5]
#define walktime sprite->fv[6]

/* Reset hitbox.
 */

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

#define WEREWOLF_HBW_MAX (0.50f*2.0f)
#define WEREWOLF_HBE_MAX (1.80f*2.0f)
#define WEREWOLF_HBN_MAX (1.30f*2.0f)
#define WEREWOLF_HBS_MAX (0.25f*2.0f)
 
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

/* Generate attacks.
 */
 
static void werewolf_generate_hadouken(struct fmn_sprite *sprite) {
  float x=sprite->x,y=sprite->y-1.0f;
  struct fmn_sprite *missile=fmn_sprite_generate_noparam(FMN_SPRCTL_missile,x,y);
  if (!missile) return;
  
  // Don't trust FMN_SPRCTL_missile's targetting.
  // We want to enforce our own cone-of-hadouken, and it's ok if it's going to miss.
  float dx=herox-missile->x;
  float dy=heroy-missile->y;
  float adx=(dx<0.0f)?-dx:dx;
  float ady=(dy<0.0f)?-dy:dy;
  if (adx+ady<1.0f) dx=adx=1.0f; // don't let it be very small
  if (adx<ady*WEREWOLF_HADOUKEN_CONE_SLOPE) adx=dx=ady*WEREWOLF_HADOUKEN_CONE_SLOPE;
  float distance=sqrtf(dx*dx+dy*dy);
  if (sprite->xform&FMN_XFORM_XREV) {
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

static void werewolf_generate_floorfire(struct fmn_sprite *sprite) {
  float x=sprite->x;
  if (sprite->xform&FMN_XFORM_XREV) x-=0.5f;
  else x+=0.5f;
  struct fmn_sprite *floorfire=fmn_sprite_generate_noparam(FMN_SPRCTL_floorfire,sprite->x,sprite->y);
  if (!floorfire) return;
  floorfire->imageid=sprite->imageid;
}

/* Flop to face the hero.
 */
 
static void werewolf_face_hero(struct fmn_sprite *sprite) {
  if (sprite->x<herox) {
    if (!(sprite->xform&FMN_XFORM_XREV)) {
      sprite->xform|=FMN_XFORM_XREV;
      WEREWOLF_FLOP_HITBOX
    }
  } else {
    if (sprite->xform&FMN_XFORM_XREV) {
      sprite->xform&=~FMN_XFORM_XREV;
      WEREWOLF_FLOP_HITBOX
    }
  }
}

/* Begin idle stage, ie reconsider surroundings.
 */
 
static void werewolf_enter_idle(struct fmn_sprite *sprite) {
  stage=WEREWOLF_STAGE_IDLE;
  stageclock=0.0f;
  WEREWOLF_SET_HITBOX(FOURS)
  werewolf_face_hero(sprite);
}

/* Sleep or wake.
 */
 
static void werewolf_sleep(struct fmn_sprite *sprite,int sleep) {
  if (sleep) {
    if ((stage==WEREWOLF_STAGE_EAT)||(stage==WEREWOLF_STAGE_DEAD)) return;
    stage=WEREWOLF_STAGE_SLEEP;
    stageclock=0.0f;
    WEREWOLF_SET_HITBOX(BACK)
    struct fmn_sprite *zzz=fmn_sprite_generate_zzz(sprite);
    if (zzz) {
      zzz->x-=0.25f;
      zzz->fv[1]-=0.25f; // x0
      zzz->y-=1.0f; // not 0.5; (y) gets corrected in weird and broken ways. whatever
      zzz->fv[2]-=0.5f; // y0
      zzz->pv[0]=0; // source; don't let it second-guess our adjustment. a sleeping werewolf can't move, so it's cool.
    }
  } else if ((stage==WEREWOLF_STAGE_SLEEP)||(stage==WEREWOLF_STAGE_SHOCK)) {
    werewolf_enter_idle(sprite);
  }
}

/* Check tickle.
 */
 
static void werewolf_die(struct fmn_sprite *sprite) {
  fmn_log_event("kill-werewolf","");
  fmn_sprite_generate_soulballs(sprite->x,sprite->y,6);
  fmn_sound_effect(FMN_SFX_KILL_WEREWOLF);
  fmn_play_song(0xff);
  WEREWOLF_SET_HITBOX(BACK)
  stage=WEREWOLF_STAGE_DEAD;
  stageclock=0.0f;
  fmn_global.werewolf_dead=1;
  fmn_global.terminate_time=3.0f;
}

static void werewolf_feather(struct fmn_sprite *sprite) {
  switch (stage) {
    case WEREWOLF_STAGE_SHOCK:
    case WEREWOLF_STAGE_SLEEP: werewolf_die(sprite); break;
  }
}

/* Eat the witch.
 */
 
static void werewolf_eat(struct fmn_sprite *sprite) {
  WEREWOLF_SET_HITBOX(FOURS)
  stage=WEREWOLF_STAGE_EAT;
  stageclock=0.0f;
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

/* Interrupting pounce: If this returns nonzero, we've entered GROWL stage.
 */
 
static uint8_t werewolf_check_pounce(struct fmn_sprite *sprite) {
  if ((heroy<sprite->y-sprite->hbn)||(heroy>=sprite->y+sprite->hbs)) {
    return 0;
  }
  float dx=herox-sprite->x;
  if (!(sprite->xform&FMN_XFORM_XREV)) dx=-dx;
  if ((dx<0.0f)||(dx>4.0f)) {
    return 0;
  }
  fmn_sound_effect(FMN_SFX_GROWL);
  stage=WEREWOLF_STAGE_GROWL;
  stageclock=0.0f;
  WEREWOLF_SET_HITBOX(FOURS)
  return 1;
}

/* Missile: If we're hit, enter SHOCK stage and return nonzero.
 */
 
static int werewolf_check_missile_1(struct fmn_sprite *q,void *userdata) {
  if (q->controller!=FMN_SPRCTL_missile) return 0;
  if (!q->bv[1]) return 0; // reflected
  // Current design, there can't be more than one missile on screen with a werewolf. If that changes, we'd have to do the bounds check here too.
  *(struct fmn_sprite**)userdata=q;
  return 1;
}
 
static uint8_t werewolf_check_missile(struct fmn_sprite *sprite) {
  struct fmn_sprite *missile=0;
  if (!fmn_sprites_for_each(werewolf_check_missile_1,&missile)) return 0;
  if (missile->x+missile->radius<sprite->x-sprite->hbw) return 0;
  if (missile->x-missile->radius>sprite->x+sprite->hbe) return 0;
  if (missile->y+missile->radius<sprite->y-sprite->hbn) return 0;
  if (missile->y-missile->radius>sprite->y+sprite->hbs) return 0;
  stage=WEREWOLF_STAGE_SHOCK;
  stageclock=0.0f;
  WEREWOLF_SET_HITBOX(BACK)
  return 1;
}

/* Select a destination and enter WALK stage.
 */
 
static void werewolf_begin_walk(struct fmn_sprite *sprite) {
  
  // Pick cells at random until we find a good one.
  uint8_t panic=200;
  uint8_t cellp;
  while (panic-->0) {
    cellp=rand()%(FMN_COLC*FMN_ROWC);
    switch (fmn_global.cellphysics[fmn_global.map[cellp]]) {
      case FMN_CELLPHYSICS_VACANT:
      case FMN_CELLPHYSICS_UNSHOVELLABLE: panic=0; break;
    }
  }
  float dstx=(cellp%FMN_COLC)+0.5f;
  float dsty=(cellp/FMN_COLC)+0.5f;
  
  // If it happens to be really close to me, bump by two tiles on each axis.
  float dx=dstx-sprite->x,dy=dsty-sprite->y;
  float adx=dx; if (adx<0.0f) adx=-adx;
  float ady=dy; if (ady<0.0f) ady=-ady;
  if (adx+ady<2.0f) {
    const float correction=2.0f;
    float halfw=FMN_COLC*0.5f,halfh=FMN_ROWC*0.5f;
    if (dstx>halfw) dstx-=correction; else dstx+=correction;
    if (dsty>halfh) dsty-=correction; else dsty+=correction;
    dx=dstx-sprite->x;
    dy=dsty-sprite->y;
  }
  
  const float walkspeed=6.0f;
  float distance=sqrtf(dx*dx+dy*dy);
  walkdx=(dx*walkspeed)/distance;
  walkdy=(dy*walkspeed)/distance;
  walktime=distance/walkspeed;
  
  stage=WEREWOLF_STAGE_WALK;
  stageclock=0.0f;
  WEREWOLF_SET_HITBOX(FOURS)
}

/* Select my next move, after a brief period IDLE.
 */
 
static void werewolf_select_move(struct fmn_sprite *sprite) {
  float dx=sprite->x-herox;
  if (!(sprite->xform&FMN_XFORM_XREV)) dx=-dx;
  float dy=sprite->y-heroy;
  
  // If she's standing in the money spot, reset the sequence and do a hadouken.
  // The determined speed runner can force hadoukens this way.
  if ((dx>=4.0f)&&(dx<6.0f)&&(dy>=-1.0f)&&(dy<=1.0f)) {
    next_move=1;
    stage=WEREWOLF_STAGE_HADOUKEN_CHARGE;
    stageclock=0.0f;
    WEREWOLF_SET_HITBOX(UPRIGHT)
    return;
  }
  
  switch (next_move++) {
    case 0: {
        stage=WEREWOLF_STAGE_HADOUKEN_CHARGE;
        stageclock=0.0f;
        WEREWOLF_SET_HITBOX(UPRIGHT)
      } break;
    case 1: {
        werewolf_begin_walk(sprite);
      } break;
    case 2: {
        stage=WEREWOLF_STAGE_FLOORFIRE_CHARGE;
        stageclock=0.0f;
        WEREWOLF_SET_HITBOX(UPRIGHT)
      } break;
    default: {
        next_move=0;
        werewolf_begin_walk(sprite);
      } break;
  }
}

/* Walk.
 */
 
static void werewolf_update_WALK(struct fmn_sprite *sprite,float elapsed) {
  if (werewolf_check_missile(sprite)) return;
  werewolf_face_hero(sprite);
  if (werewolf_check_pounce(sprite)) return;
  if (stageclock>walktime) {
    werewolf_enter_idle(sprite);
    return;
  }
  sprite->x+=walkdx*elapsed;
  sprite->y+=walkdy*elapsed;
}

/* Pounce.
 */
 
static void werewolf_update_POUNCE(struct fmn_sprite *sprite,float elapsed) {
  if (werewolf_check_missile(sprite)) return;
  if (stageclock>0.5f) {
    werewolf_enter_idle(sprite);
    return;
  }
  const float speed=12.0f;
  if (sprite->xform&FMN_XFORM_XREV) {
    sprite->x+=speed*elapsed;
  } else {
    sprite->x-=speed*elapsed;
  }
}

/* Simple timed stages.
 */
 
static void werewolf_update_IDLE(struct fmn_sprite *sprite) {
  if (werewolf_check_missile(sprite)) return;
  werewolf_face_hero(sprite);
  if (werewolf_check_pounce(sprite)) return;
  if (stageclock>=0.5f) {
    werewolf_select_move(sprite);
  }
}
 
static void werewolf_update_GROWL(struct fmn_sprite *sprite) {
  if (werewolf_check_missile(sprite)) return;
  if (stageclock>=0.5f) {
    fmn_sound_effect(FMN_SFX_BARK);
    stage=WEREWOLF_STAGE_POUNCE;
    stageclock=0.0f;
  }
}

static void werewolf_update_HADOUKEN_CHARGE(struct fmn_sprite *sprite) {
  if (werewolf_check_missile(sprite)) return;
  if (stageclock>=0.5f) {
    fmn_sound_effect(FMN_SFX_HADOUKEN);
    stage=WEREWOLF_STAGE_HADOUKEN_FOLLOWTHRU;
    stageclock=0.0f;
    werewolf_generate_hadouken(sprite);
  }
}

static void werewolf_update_HADOUKEN_FOLLOWTHRU(struct fmn_sprite *sprite) {
  if (werewolf_check_missile(sprite)) return;
  if (stageclock>=1.0f) {
    werewolf_enter_idle(sprite);
  }
}

static void werewolf_update_FLOORFIRE_CHARGE(struct fmn_sprite *sprite) {
  if (werewolf_check_missile(sprite)) return;
  if (stageclock>=1.0f) {
    stage=WEREWOLF_STAGE_FLOORFIRE_FOLLOWTHRU;
    stageclock=0.0f;
    werewolf_generate_floorfire(sprite);
  }
}

static void werewolf_update_FLOORFIRE_FOLLOWTHRU(struct fmn_sprite *sprite) {
  if (werewolf_check_missile(sprite)) return;
  if (stageclock>=1.0f) {
    werewolf_enter_idle(sprite);
  }
}

static void werewolf_update_SHOCK(struct fmn_sprite *sprite) {
  if (stageclock>=6.0f) {
    werewolf_enter_idle(sprite);
  }
}

/* Update.
 */

static void _werewolf_update(struct fmn_sprite *sprite,float elapsed) {
  fmn_hero_get_position(&herox,&heroy);
  stageclock+=elapsed;
  
  // A wee cudgel. If the hero is dead, let EAT run as usual, but everything else, freeze in IDLE state.
  // Without this, he turns left to face her (default position, zero), and throws hadoukens at nothing.
  if (fmn_global.hero_dead) {
    switch (stage) {
      case WEREWOLF_STAGE_EAT: break;
      default: stage=WEREWOLF_STAGE_IDLE; break;
    }
    return;
  }
  
  switch (stage) {
    case WEREWOLF_STAGE_IDLE: werewolf_update_IDLE(sprite); break;
    case WEREWOLF_STAGE_SLEEP: break;
    case WEREWOLF_STAGE_ERECT: break; // not a real stage
    case WEREWOLF_STAGE_WALK: werewolf_update_WALK(sprite,elapsed); break;
    case WEREWOLF_STAGE_GROWL: werewolf_update_GROWL(sprite); break;
    case WEREWOLF_STAGE_POUNCE: werewolf_update_POUNCE(sprite,elapsed); break;
    case WEREWOLF_STAGE_HADOUKEN_CHARGE: werewolf_update_HADOUKEN_CHARGE(sprite); break;
    case WEREWOLF_STAGE_HADOUKEN_FOLLOWTHRU: werewolf_update_HADOUKEN_FOLLOWTHRU(sprite); break;
    case WEREWOLF_STAGE_FLOORFIRE_CHARGE: werewolf_update_FLOORFIRE_CHARGE(sprite); break;
    case WEREWOLF_STAGE_FLOORFIRE_FOLLOWTHRU: werewolf_update_FLOORFIRE_FOLLOWTHRU(sprite); break;
    case WEREWOLF_STAGE_EAT: break;
    case WEREWOLF_STAGE_SHOCK: werewolf_update_SHOCK(sprite); break;
    case WEREWOLF_STAGE_DEAD: break;
  }
}

/* Interact.
 */

static int16_t _werewolf_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: werewolf_sleep(sprite,1); break;
        case FMN_SPELLID_REVEILLE: werewolf_sleep(sprite,0); break;
        case FMN_SPELLID_PUMPKIN: fmn_sprite_pumpkinize(sprite); break;
      } break;
    case FMN_ITEM_BELL: werewolf_sleep(sprite,0); break;
    case FMN_ITEM_FEATHER: werewolf_feather(sprite); break;
  }
  return 0;
}

/* Pressure.
 */
 
static void _werewolf_pressure(struct fmn_sprite *sprite,struct fmn_sprite *hero,uint8_t dir) {
  if (stage!=WEREWOLF_STAGE_POUNCE) return;
  if (!hero||(hero->style!=FMN_SPRITE_STYLE_HERO)) return;
  if (sprite->xform&FMN_XFORM_XREV) {
    if (dir!=0x10) return;
  } else {
    if (dir!=0x08) return;
  }
  werewolf_eat(sprite);
}

/* Init.
 */
 
static void _werewolf_init(struct fmn_sprite *sprite) {
  stage=WEREWOLF_STAGE_IDLE;
  stageclock=0.0f;
  WEREWOLF_SET_HITBOX(FOURS)
  sprite->physics=FMN_PHYSICS_EDGE|FMN_PHYSICS_SOLID|FMN_PHYSICS_HOLE|FMN_PHYSICS_SPRITES;
  sprite->invmass=0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_werewolf={
  .init=_werewolf_init,
  .update=_werewolf_update,
  .interact=_werewolf_interact,
  .pressure=_werewolf_pressure,
};
