#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define LOBSTER_STAGE_CHILL 0
#define LOBSTER_STAGE_SCURRY 1

#define LOBSTER_CHILL_TIME_MIN 0.5f
#define LOBSTER_CHILL_TIME_MAX 2.0f
#define LOBSTER_SCURRY_TIME_MIN 0.5f
#define LOBSTER_SCURRY_TIME_MAX 2.0f
#define LOBSTER_SCURRY_SPEED 3.0f

#define tileid0 sprite->bv[0]
#define stage sprite->bv[1]
#define sleeping sprite->bv[2]
#define animclock sprite->fv[0]
#define stageclock sprite->fv[1] /* counts down */
#define scurrydx sprite->fv[2]
#define scurrydy sprite->fv[3]

/* CHILL: Just sit still. Do continue animating.
 */
 
static void lobster_begin_CHILL(struct fmn_sprite *sprite) {
  stage=LOBSTER_STAGE_CHILL;
  stageclock=LOBSTER_CHILL_TIME_MIN+((rand()%1000)*(LOBSTER_CHILL_TIME_MAX-LOBSTER_CHILL_TIME_MIN))/1000.0f;
}

/* SCURRY: Walk in a straight line until time expires.
 */
 
static void lobster_begin_SCURRY(struct fmn_sprite *sprite) {
  stage=LOBSTER_STAGE_SCURRY;
  stageclock=LOBSTER_SCURRY_TIME_MIN+((rand()%1000)*(LOBSTER_SCURRY_TIME_MAX-LOBSTER_SCURRY_TIME_MIN))/1000.0f;
  
  // We don't need to be super smart about picking a direction.
  // But it's good to at least check the first adjacent tile, and only consider directions where that is walkable.
  uint8_t dir;
  if (sprite->x<0.0f) dir=FMN_DIR_E;
  else if (sprite->y<0.0f) dir=FMN_DIR_S;
  else if (sprite->x>=FMN_COLC) dir=FMN_DIR_W;
  else if (sprite->y>=FMN_ROWC) dir=FMN_DIR_N;
  else {
    uint8_t optv[4];
    uint8_t optc=0;
    int8_t col=(int8_t)sprite->x;
    int8_t row=(int8_t)sprite->y;
    #define PASSABLE(x,y) ({ \
      uint8_t v=fmn_global.map[(y)*FMN_COLC+(x)]; \
      v=fmn_global.cellphysics[v]; \
      switch (v) { \
        case FMN_CELLPHYSICS_VACANT: \
        case FMN_CELLPHYSICS_UNSHOVELLABLE: \
          v=1; \
          break; \
        default: v=0; \
      } \
      v; \
    })
    if ((col>0)&&PASSABLE(col-1,row)) optv[optc++]=FMN_DIR_W;
    if ((row>0)&&PASSABLE(col,row-1)) optv[optc++]=FMN_DIR_N;
    if ((col<FMN_COLC-1)&&PASSABLE(col+1,row)) optv[optc++]=FMN_DIR_E;
    if ((row<FMN_ROWC-1)&&PASSABLE(col,row+1)) optv[optc++]=FMN_DIR_S;
    if (!optc) { // trapped! whatever, pick any direction.
      optv[0]=FMN_DIR_N;
      optv[1]=FMN_DIR_S;
      optv[2]=FMN_DIR_W;
      optv[3]=FMN_DIR_E;
      optc=3;
    }
    dir=optv[rand()%optc];
    #undef PASSABLE
  }
  switch (dir) {
    case FMN_DIR_E: scurrydx= LOBSTER_SCURRY_SPEED; scurrydy=0.0f; break;
    case FMN_DIR_W: scurrydx=-LOBSTER_SCURRY_SPEED; scurrydy=0.0f; break;
    case FMN_DIR_S: scurrydy= LOBSTER_SCURRY_SPEED; scurrydx=0.0f; break;
    case FMN_DIR_N: scurrydy=-LOBSTER_SCURRY_SPEED; scurrydx=0.0f; break;
  }
}

static void lobster_update_SCURRY(struct fmn_sprite *sprite,float elapsed) {
  sprite->x+=scurrydx*elapsed;
  sprite->y+=scurrydy*elapsed;
}

/* Init.
 */
 
static void _lobster_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  lobster_begin_CHILL(sprite);
}

/* Update.
 */
 
static void _lobster_update(struct fmn_sprite *sprite,float elapsed) {
  if (sleeping) {
    sprite->tileid=tileid0+2;
    return;
  }
  animclock+=elapsed;
  if (animclock>=0.8f) animclock-=0.8f;
  if (animclock>=0.4f) sprite->tileid=tileid0+1;
  else sprite->tileid=tileid0;
  if ((stageclock-=elapsed)<=0.0f) switch (stage) {
    case LOBSTER_STAGE_CHILL: lobster_begin_SCURRY(sprite); break;
    case LOBSTER_STAGE_SCURRY: lobster_begin_CHILL(sprite); break;
  } else switch (stage) {
    case LOBSTER_STAGE_SCURRY: lobster_update_SCURRY(sprite,elapsed); break;
  }
}

/* Hero collision.
 */

static void _lobster_pressure(struct fmn_sprite *sprite,struct fmn_sprite *presser,uint8_t dir) {
  if (sleeping) return;
  if (!presser||(presser->style!=FMN_SPRITE_STYLE_HERO)) return;
  fmn_hero_injure(sprite->x,sprite->y,sprite);
}

/* Interact.
 */
 
static int16_t _lobster_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
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
 
const struct fmn_sprite_controller fmn_sprite_controller_lobster={
  .init=_lobster_init,
  .update=_lobster_update,
  .pressure=_lobster_pressure,
  .interact=_lobster_interact,
};
