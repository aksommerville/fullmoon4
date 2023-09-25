#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

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
#define time_since_charm_toast sprite->fv[3]
#define snap_clock sprite->fv[4]

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

static int pushblock_physics_cb(struct fmn_sprite *q,void *userdata) {
  struct fmn_sprite *sprite=userdata;
  if (q==sprite) return 0;
  if (!(q->physics&FMN_PHYSICS_SPRITES)) return 0;
  if (!fmn_physics_check_sprites(0,0,q,sprite)) return 0;
  return 1;
}
 
static uint8_t pushblock_prepare_motion(struct fmn_sprite *sprite,uint8_t dir) {
  
  float dx,dy;
  fmn_vector_from_dir(&dx,&dy,dir);
  float dstx=sprite->x+dx;
  float dsty=sprite->y+dy;
  
  sprite->x=dstx;
  sprite->y=dsty;
  sprite->radius-=0.3f;
  uint8_t collision=0;
  if (
    fmn_physics_check_edges(0,0,sprite)||
    fmn_physics_check_grid(0,0,sprite,sprite->physics)||
    fmn_sprites_for_each(pushblock_physics_cb,sprite)
  ) collision=1;
  sprite->radius+=0.3f;
  sprite->x-=dx;
  sprite->y-=dy;
  
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
  
  return collision?0:1;
}

/* Begin push.
 */
 
static void pushblock_push(struct fmn_sprite *sprite) {
  if (pushblock_prepare_motion(sprite,pressdir)) {
    fmn_sound_effect(FMN_SFX_PUSH);
  }
  // If it doesn't look possible, try anyway. Just don't make the "fwish" sound.
  pressdir=0;
  presstime=0.0f;
}

/* Check snap.
 */
 
static void pushblock_snap(struct fmn_sprite *sprite,float *p) {
  if (snap_clock<0.0f) return;
  float whole;
  float m=modff(*p,&whole);
  float d=m-0.5f;
  if (d<0.0f) d=-d;
  if (d<1.0f/16.0f) {
    *p=whole+0.5f;
    snap_clock=0.250f;
  }
}

/* Move due to charm.
 */
 
static void pushblock_move_charm(struct fmn_sprite *sprite,float elapsed) {
  if (snap_clock>0.0f) {
    if ((snap_clock-=elapsed)<=0.0f) {
      snap_clock=-0.125f;
    } else {
      return;
    }
  } else if (snap_clock<0.0f) {
    if ((snap_clock+=elapsed)>=0.0f) {
      snap_clock=0.0f;
    }
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  int moved=0;
  #define BARREL(axis) ((hero##axis>=sprite->axis-FMN_PUSHBLOCK_CHARM_RADIUS)&&(hero##axis<=sprite->axis+FMN_PUSHBLOCK_CHARM_RADIUS))
  switch (charmdir) {
    case FMN_DIR_N: if (BARREL(x)&&(heroy<sprite->y-FMN_PUSHBLOCK_CHARM_PROXIMITY)) { sprite->y-=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; moved=1; pushblock_snap(sprite,&sprite->y); } break;
    case FMN_DIR_S: if (BARREL(x)&&(heroy>sprite->y+FMN_PUSHBLOCK_CHARM_PROXIMITY)) { sprite->y+=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; moved=1; pushblock_snap(sprite,&sprite->y); } break;
    case FMN_DIR_W: if (BARREL(y)&&(herox<sprite->x-FMN_PUSHBLOCK_CHARM_PROXIMITY)) { sprite->x-=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; moved=1; pushblock_snap(sprite,&sprite->x); } break;
    case FMN_DIR_E: if (BARREL(y)&&(herox>sprite->x+FMN_PUSHBLOCK_CHARM_PROXIMITY)) { sprite->x+=FMN_PUSHBLOCK_CHARM_SPEED*elapsed; moved=1; pushblock_snap(sprite,&sprite->x); } break;
  }
  #undef BARREL
  if (moved) {
    fmn_game_event_broadcast(FMN_GAME_EVENT_BLOCKS_MOVED,sprite);
  } else {
    snap_clock=0.0f;
  }
}

/* Nudge toward grid alignment on the minor axis, while experiencing pressure on the other.
 */

static void pushblock_nudge_minor_axis(struct fmn_sprite *sprite,float elapsed) {
  float dx=0.0f,dy=0.0f,sub,dummy;
  switch (pressdir) {
    case FMN_DIR_N:
    case FMN_DIR_S: {
        dx=1.0f;
        sub=sprite->x;
      } break;
    case FMN_DIR_W:
    case FMN_DIR_E: {
        dy=1.0f;
        sub=sprite->y;
      } break;
    default: return;
  }
  sub=modff(sub,&dummy)-0.5f;
  const float near=0.125f;
  if ((sub>=-near)&&(sub<=near)) {
    sprite->x+=dx*-sub;
    sprite->y+=dy*-sub;
  }
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
    } else {
      pushblock_nudge_minor_axis(sprite,elapsed);
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
    if (!slidedir) { // slide ended
      fmn_game_event_broadcast(FMN_GAME_EVENT_BLOCKS_MOVED,sprite);
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
  // When the enchantment begins or changes, fire the toast immediately. Even if there's already one nearby.
  fmn_sprite_kill_enchantment(sprite);
  fmn_sprite_generate_enchantment(sprite,1);
  time_since_charm_toast=0.0f;
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
