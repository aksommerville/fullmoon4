#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"
#include "app/hero/fmn_hero.h"

#define FMN_PUSHBLOCK_PRESS_TIME 0.5f
#define FMN_PUSHBLOCK_SLIDE_SPEED 5.0f
#define FMN_PUSHBLOCK_SLIDE_TIME_LIMIT 0.400f /* more than 1/SLIDE_SPEED, time before we panic and stop */
#define FMN_PUSHBLOCK_TARGET_FUDGE 0.100f
#define FMN_PUSHBLOCK_CHARM_SPEED 1.0f
#define FMN_PUSHBLOCK_CHARM_RADIUS 0.75f
#define FMN_PUSHBLOCK_CHARM_PROXIMITY 1.0f

#define pressdir sprite->bv[0]
#define presscurrent sprite->bv[1]
#define slidedir sprite->bv[2]
#define charmdir sprite->bv[3]
#define presstime sprite->fv[0]
#define slidetime sprite->fv[1]
#define slidedst sprite->fv[2]

/* Init.
 */
 
static void _pushblock_init(struct fmn_sprite *sprite) {
}

/* Begin motion.
 */
 
struct pushblock_point {
  float x,y;
};

static int pushblock_check_solids_cb(struct fmn_sprite *q,void *userdata) {
  struct pushblock_point *point=userdata;
  if (!(q->physics&FMN_PHYSICS_SPRITES)) return 0;
  if (point->x<q->x-q->radius) return 0;
  if (point->x>q->x+q->radius) return 0;
  if (point->y<q->y-q->radius) return 0;
  if (point->y>q->y+q->radius) return 0;
  return 1;
}
 
static void pushblock_prepare_motion(struct fmn_sprite *sprite,uint8_t dir) {
  
  float dx,dy;
  fmn_vector_from_dir(&dx,&dy,dir);
  float dstx=sprite->x+dx;
  float dsty=sprite->y+dy;
  
  slidedir=dir;
  slidetime=0.0f;
  switch (dir) {
    case FMN_DIR_N: slidedst=dsty; break;
    case FMN_DIR_S: slidedst=dsty; break;
    case FMN_DIR_W: slidedst=dstx; break;
    case FMN_DIR_E: slidedst=dstx; break;
  }
  
  
  // If the target position is close to aligned, fudge it to perfect alignment.
  float whole,fract;
  fract=modff(slidedst,&whole);
  if ((fract>=0.5f-FMN_PUSHBLOCK_TARGET_FUDGE)&&(fract<=0.5+FMN_PUSHBLOCK_TARGET_FUDGE)) {
    slidedst=whole+0.5f;
  }
}

/* Begin push.
 */
 
static void pushblock_push(struct fmn_sprite *sprite) {
  pushblock_prepare_motion(sprite,pressdir);
  pressdir=0;
  presstime=0.0f;
}

/* Move due to charm.
 */
 
static void pushblock_move_charm(struct fmn_sprite *sprite,float elapsed) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  #define BARREL(axis) ((hero##axis>=sprite->axis-FMN_PUSHBLOCK_CHARM_RADIUS)&&(hero##axis<=sprite->axis+FMN_PUSHBLOCK_CHARM_RADIUS))
  switch (charmdir) {
    case FMN_DIR_N: if (BARREL(x)&&(heroy<sprite->y-FMN_PUSHBLOCK_CHARM_PROXIMITY)) sprite->y-=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; break;
    case FMN_DIR_S: if (BARREL(x)&&(heroy>sprite->y+FMN_PUSHBLOCK_CHARM_PROXIMITY)) sprite->y+=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; break;
    case FMN_DIR_W: if (BARREL(y)&&(herox<sprite->x-FMN_PUSHBLOCK_CHARM_PROXIMITY)) sprite->x-=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; break;
    case FMN_DIR_E: if (BARREL(y)&&(herox>sprite->x+FMN_PUSHBLOCK_CHARM_PROXIMITY)) sprite->x+=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; break;
  }
  #undef BARREL
}

/* Update.
 */
 
static void _pushblock_update(struct fmn_sprite *sprite,float elapsed) {

  /* Drop pressure if it didn't get noted in the last cycle.
   * update is guaranteed to happen before pressure each cycle.
   * If we are sliding, drop all pressure.
   */
  if (!presscurrent||slidedir) {
    if (pressdir) {
      pressdir=0;
      presstime=0.0f;
    }
  } else {
    presscurrent=0;
    presstime+=elapsed;
    if (presstime>=FMN_PUSHBLOCK_PRESS_TIME) {
      pushblock_push(sprite);
    }
  }

  /* Slide, if we're doing that.
   */
  if (slidedir) {
    slidetime+=elapsed;
    if (slidetime>=FMN_PUSHBLOCK_SLIDE_TIME_LIMIT) {
      slidedir=0;
    } else switch (slidedir) {
      case FMN_DIR_N: sprite->y-=FMN_PUSHBLOCK_SLIDE_SPEED*elapsed; if (sprite->y<=slidedst) { sprite->y=slidedst; slidedir=0; } break;
      case FMN_DIR_S: sprite->y+=FMN_PUSHBLOCK_SLIDE_SPEED*elapsed; if (sprite->y>=slidedst) { sprite->y=slidedst; slidedir=0; } break;
      case FMN_DIR_W: sprite->x-=FMN_PUSHBLOCK_SLIDE_SPEED*elapsed; if (sprite->x<=slidedst) { sprite->x=slidedst; slidedir=0; } break;
      case FMN_DIR_E: sprite->x+=FMN_PUSHBLOCK_SLIDE_SPEED*elapsed; if (sprite->x>=slidedst) { sprite->x=slidedst; slidedir=0; } break;
    }
  }
  
  /* Move per charm. This can run together with push-sliding.
   */
  if (charmdir) pushblock_move_charm(sprite,elapsed);
}

/* Get charmed.
 */
 
static void pushblock_charm(struct fmn_sprite *sprite,uint8_t dir) {
  if (charmdir==dir) return;
  fmn_sound_effect(FMN_SFX_PUSHBLOCK_ENCHANT);
  charmdir=dir;
}

/* Pressure.
 */
 
static void _pushblock_pressure(struct fmn_sprite *sprite,struct fmn_sprite *presser,uint8_t dir) {
  presscurrent=1;
  if (dir!=pressdir) {
    pressdir=dir;
    presstime=0.0f;
  }
}

/* Interact.
 */
 
static int16_t _pushblock_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
    case FMN_ITEM_FEATHER: pushblock_charm(sprite,fmn_dir_reverse(fmn_global.facedir)); break;
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_pushblock={
  .init=_pushblock_init,
  .update=_pushblock_update,
  .pressure=_pushblock_pressure,
  .interact=_pushblock_interact,
};
