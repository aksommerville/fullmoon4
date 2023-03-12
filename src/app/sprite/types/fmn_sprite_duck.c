#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define DUCK_STAGE_DECIDE 1
#define DUCK_STAGE_IDLE 2
#define DUCK_STAGE_QUACK 3
#define DUCK_STAGE_WALK 4
#define DUCK_STAGE_THROW 5

#define DUCK_THROW_RADIUS 2.0f
#define DUCK_IDLE_ODDS 100
#define DUCK_QUACK_ODDS 50
#define DUCK_WALK_ODDS 300

#define DUCK_IDLE_TIME_MIN 0.200f
#define DUCK_IDLE_TIME_MAX 1.000f
#define DUCK_QUACK_TIME 0.750f
#define DUCK_WALK_TIME_MIN 1.500f
#define DUCK_WALK_TIME_MAX 2.500f
#define DUCK_WALK_SPEED 2.000f
#define DUCK_WALK_FRAME_TIME 0.125f
#define DUCK_THROW_TIME 1.000f
#define DUCK_THROW_BLACKOUT_TIME 5.000f /* prevent ugly auto-re-throw */
#define DUCK_FLAP_PREVENTION_TIME 0.500f /* prevent turning around due to collision too fast */

#define tileid0 sprite->bv[0]
#define stage sprite->bv[1]
#define sleeping sprite->bv[2]
#define charmed sprite->bv[3]
#define clock sprite->fv[0]
#define walkdx sprite->fv[1]
#define walkdy sprite->fv[2]
#define throw_blackout sprite->fv[3]
#define flap_prevention sprite->fv[4]

/* Init.
 */
 
static void _duck_init(struct fmn_sprite *sprite) {
  stage=DUCK_STAGE_DECIDE;
  tileid0=sprite->tileid;
  sprite->xform=(rand()&1)?FMN_XFORM_XREV:0;
}

/* Create the missile and enter THROW stage.
 */
 
static void duck_throw_star(struct fmn_sprite *sprite,float herox,float heroy) {
  stage=DUCK_STAGE_THROW;
  sprite->tileid=tileid0+4;
  clock=0.0f;
  throw_blackout=DUCK_THROW_BLACKOUT_TIME;
  float x=sprite->x;
  if (sprite->xform&FMN_XFORM_XREV) x-=0.5f;
  else x+=0.5f;
  const uint8_t cmdv[]={
    0x42,FMN_SPRCTL_missile>>8,FMN_SPRCTL_missile,
    0x20,sprite->imageid,
    0x21,0x8e,
    0x23,FMN_SPRITE_STYLE_TWOFRAME,
  };
  const uint8_t argv[]={};
  struct fmn_sprite *star=fmn_sprite_spawn(x,sprite->y,0,cmdv,sizeof(cmdv),argv,sizeof(argv));
  if (star) {
    // defaults are probably ok
  }
}

/* Begin the trivial IDLE or QUACK stages.
 */
 
static void duck_begin_IDLE(struct fmn_sprite *sprite) {
  stage=DUCK_STAGE_IDLE;
  sprite->tileid=tileid0;
  clock=DUCK_IDLE_TIME_MIN+((rand()%1000)*(DUCK_IDLE_TIME_MAX-DUCK_IDLE_TIME_MIN))/1000.0f;
  if (rand()&1) sprite->xform^=FMN_XFORM_XREV;
}

static void duck_begin_QUACK(struct fmn_sprite *sprite) {
  stage=DUCK_STAGE_QUACK;
  sprite->tileid=tileid0+3;
  fmn_sound_effect(FMN_SFX_QUACK);
  clock=0.0f;
}

/* Begin walking.
 */
 
static void duck_begin_WALK(struct fmn_sprite *sprite) {
  // Since our image is oriented horizontally, restrict motion to within 45 degrees of horizontal.
  // Retain the direction we're already facing. That changes only when beginning an IDLE stage.
  float t=((rand()%1000)*M_PI*0.5f)/1000.0f-(M_PI*0.25f);
  if (sprite->xform&FMN_XFORM_XREV) t+=M_PI;
  walkdx=cosf(t)*DUCK_WALK_SPEED;
  walkdy=sinf(t)*DUCK_WALK_SPEED;
  stage=DUCK_STAGE_WALK;
  clock=DUCK_WALK_TIME_MIN+((rand()%1000)*(DUCK_WALK_TIME_MAX-DUCK_WALK_TIME_MIN))/1000.0f;
}

/* Select an appropriate stage.
 */
 
static void duck_update_DECIDE(struct fmn_sprite *sprite) {
  sprite->tileid=tileid0;
  
  // If the hero is close to my throwing range, throw a ninja star at her.
  if ((throw_blackout<=0.0f)&&!charmed) {
    float herox,heroy;
    fmn_hero_get_position(&herox,&heroy);
    float dy=heroy-sprite->y;
    if ((dy>=-DUCK_THROW_RADIUS)&&(dy<=DUCK_THROW_RADIUS)) {
      if (
        ((herox>sprite->x)&&!(sprite->xform&FMN_XFORM_XREV))||
        ((herox<sprite->x)&&(sprite->xform&FMN_XFORM_XREV))
      ) {
        duck_throw_star(sprite,herox,heroy);
        return;
      }
    }
  }
  
  // The other three stages, we select randomly with fixed weights.
  const int total_odds=DUCK_IDLE_ODDS+DUCK_QUACK_ODDS+DUCK_WALK_ODDS;
  int choice=rand()%total_odds;
  if ((choice-=DUCK_IDLE_ODDS)<=0) duck_begin_IDLE(sprite);
  else if ((choice-=DUCK_QUACK_ODDS)<=0) duck_begin_QUACK(sprite);
  else duck_begin_WALK(sprite);
}

/* Update trivial stages.
 */
 
static void duck_update_IDLE(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0;
  clock-=elapsed;
  if (clock<=0.0f) stage=DUCK_STAGE_DECIDE;
}
 
static void duck_update_QUACK(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0+3;
  clock+=elapsed;
  if (clock>=DUCK_QUACK_TIME) stage=DUCK_STAGE_DECIDE;
}
 
static void duck_update_THROW(struct fmn_sprite *sprite,float elapsed) {
  sprite->tileid=tileid0+4;
  clock+=elapsed;
  if (clock>=DUCK_THROW_TIME) stage=DUCK_STAGE_DECIDE;
}

/* Update WALK stage.
 */
 
static void duck_update_WALK(struct fmn_sprite *sprite,float elapsed) {
  clock-=elapsed;
  if (clock<=0.0f) {
    sprite->tileid=tileid0;
    stage=DUCK_STAGE_DECIDE;
    return;
  }
  switch (((int)(clock/DUCK_WALK_FRAME_TIME))&3) {
    case 0: sprite->tileid=tileid0+0; break;
    case 1: sprite->tileid=tileid0+1; break;
    case 2: sprite->tileid=tileid0+0; break;
    case 3: sprite->tileid=tileid0+2; break;
  }
  sprite->x+=walkdx*elapsed;
  sprite->y+=walkdy*elapsed;
}

/* Update.
 */
 
static void _duck_update(struct fmn_sprite *sprite,float elapsed) {
  if ((throw_blackout-=elapsed)<0.0f) throw_blackout=0.0f;
  if ((flap_prevention-=elapsed)<0.0f) flap_prevention=0.0f;
  if (sleeping) {
    sprite->tileid=tileid0+5;
    return;
  }
  switch (stage) {
    case DUCK_STAGE_DECIDE: duck_update_DECIDE(sprite); break;
    case DUCK_STAGE_IDLE: duck_update_IDLE(sprite,elapsed); break;
    case DUCK_STAGE_QUACK: duck_update_QUACK(sprite,elapsed); break;
    case DUCK_STAGE_WALK: duck_update_WALK(sprite,elapsed); break;
    case DUCK_STAGE_THROW: duck_update_THROW(sprite,elapsed); break;
  }
}

/* Interact.
 */
 
static int16_t _duck_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_WAND: switch (qualifier) {
        case FMN_SPELLID_LULLABYE: if (!sleeping) { sleeping=1; fmn_sprite_generate_zzz(sprite); } break;
        case FMN_SPELLID_REVEILLE: sleeping=0; break;
      } break;
    case FMN_ITEM_BELL: sleeping=0; break;
    case FMN_ITEM_FEATHER: if (!charmed) { charmed=1; fmn_sound_effect(FMN_SFX_ENCHANT_ANIMAL); } break;
  }
  return 0;
}

/* Pressure: If it's in the direction we're walking, turn around.
 */
 
static void _duck_pressure(struct fmn_sprite *sprite,struct fmn_sprite *obstruction,uint8_t dir) {
  if (stage!=DUCK_STAGE_WALK) return;
  if (flap_prevention>0.0f) return;
  switch (dir) {
    case FMN_DIR_E: if (walkdx<0.0f) {
        flap_prevention=DUCK_FLAP_PREVENTION_TIME;
        sprite->xform&=~FMN_XFORM_XREV;
        walkdx=-walkdx;
      } break;
    case FMN_DIR_W: if (walkdx>0.0f) {
        flap_prevention=DUCK_FLAP_PREVENTION_TIME;
        sprite->xform|=FMN_XFORM_XREV;
        walkdx=-walkdx;
     } break;
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_duck={
  .init=_duck_init,
  .update=_duck_update,
  .interact=_duck_interact,
  .pressure=_duck_pressure,
  .static_pressure=_duck_pressure,
};
