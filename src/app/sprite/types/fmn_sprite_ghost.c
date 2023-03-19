#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"
#include "app/fmn_game.h"

/* I want the ghost to spiral in upon the hero.
 * EXPERIMENT: Do it without much state. Find the clockwise tangent with respect to hero, and tweak it clockwise by like pi/8.
 */
#define GHOST_IN_RANGE 0.40f /* Manhattan distance <this, we are upon the hero, stop moving. */

#define GHOST_STAGE_APPROACH 1
#define GHOST_STAGE_RETREAT 2
#define GHOST_STAGE_PANIC 3

#define GHOST_APPROACH_SPEED 4.0f
#define GHOST_APPROACH_ANGULAR_VELOCITY 7.0f /* radians/sec */
#define GHOST_APPROACH_PANIC_DISTANCE 8.0f /* exceed this threshold, we fly straight to the hero */
#define GHOST_APPROACH_RECOVERY_DISTANCE 4.0f /* ...until we get this close */
#define GHOST_RETREAT_SPEED 2.0f
#define GHOST_RETREAT_TIME 4.0f

#define stage sprite->bv[0]
#define retreatdx sprite->fv[0]
#define retreatdy sprite->fv[1]
#define clock sprite->fv[2]
#define approachmd sprite->fv[3]

/* Global controller.
 * The global's only job is to detect proximity among ghosts, and gently nudge them apart.
 * The nature of a single ghost's motion is that they tend to clump.
 */
 
#define GHOST_COUNT_LIMIT 16
#define GHOST_PROXIMITY_LIMIT 2.0f /* m */
#define GHOST_PROXIMITY_CORRECTION_LIMIT 4.0f /* m/s */
#define GHOST_PROXIMITY_FLOOR 0.0625f /* m */
#define GHOST_PROXIMITY_DISABLE_BY_MD 1.0f /* m manhattan distance. when so close to the hero, it doesn't participate in proximity correction */
 
static struct {
  uint8_t ghostc;
  struct fmn_sprite *spritev[GHOST_COUNT_LIMIT];
  uint8_t spritec;
} ghost_global;

static int ghost_detect(struct fmn_sprite *sprite,void *userdata) {
  if (sprite->controller!=FMN_SPRCTL_ghost) return 0;
  if (ghost_global.spritec>=GHOST_COUNT_LIMIT) return 1; // can't happen but let's be safe
  if (sprite->bv[0]==GHOST_STAGE_APPROACH) {
    if (sprite->fv[3]<=GHOST_PROXIMITY_DISABLE_BY_MD) {
      return 0;
    }
  }
  ghost_global.spritev[ghost_global.spritec++]=sprite;
  return 0;
}
 
static void _ghost_class_update(void *userdata,float elapsed) {
  if (ghost_global.ghostc<2) return;
  ghost_global.spritec=0;
  fmn_sprites_for_each(ghost_detect,0);
  uint8_t ai=ghost_global.spritec;
  while (ai-->1) {
    struct fmn_sprite *a=ghost_global.spritev[ai];
    uint8_t bi=ai;
    while (bi-->0) {
      struct fmn_sprite *b=ghost_global.spritev[bi];
      
      // Collision?
      float dx=b->x-a->x;
      if (dx>=GHOST_PROXIMITY_LIMIT) continue;
      if (dx<=-GHOST_PROXIMITY_LIMIT) continue;
      float dy=b->y-a->y;
      if (dy>=GHOST_PROXIMITY_LIMIT) continue;
      if (dy<=-GHOST_PROXIMITY_LIMIT) continue;
      float distance=sqrtf(dx*dx+dy*dy);
      if (distance>=GHOST_PROXIMITY_LIMIT) continue;
      
      // If they are too close, eg exactly upon each other, pretend they're just a reasonable distance apart horizontally.
      if (distance<GHOST_PROXIMITY_FLOOR) {
        dx=GHOST_PROXIMITY_FLOOR;
        dy=0.0f;
        distance=GHOST_PROXIMITY_FLOOR;
      }
      
      // Correct (b) positively and (a) negatively by the same amount, inversely proportionate to (distance).
      float correction=((GHOST_PROXIMITY_LIMIT-distance)*GHOST_PROXIMITY_CORRECTION_LIMIT)/GHOST_PROXIMITY_LIMIT;
      correction*=elapsed/distance;
      b->x+=dx*correction;
      b->y+=dy*correction;
      a->x-=dx*correction;
      a->y-=dy*correction;
    }
  }
}

static void _ghost_class_cleanup(void *userdata) {
  ghost_global.ghostc=0;
}

/* Init.
 */
 
static void _ghost_init(struct fmn_sprite *sprite) {
  stage=GHOST_STAGE_APPROACH;
  if (fmn_game_register_map_singleton(
    _ghost_init,
    _ghost_class_update,
    _ghost_class_cleanup,
    0
  )) {
    ghost_global.ghostc=1;
  } else if (ghost_global.ghostc>=GHOST_COUNT_LIMIT) {
    fmn_sprite_kill(sprite);
    return;
  } else {
    ghost_global.ghostc++;
  }
}

/* Begin RETREAT stage.
 * We'll fly at constant speed in a straight line from wherever we are through the center of the screen.
 */
 
static void ghost_begin_RETREAT(struct fmn_sprite *sprite) {
  clock=0.0f;
  stage=GHOST_STAGE_RETREAT;
  float dx=(FMN_COLC*0.5f)-sprite->x;
  float dy=(FMN_ROWC*0.5f)-sprite->y;
  
  // If we're very close to the center already, any direction is fine.
  // Carve out a special case for it, to mitigate rounding error.
  if ((dx>-0.01f)&&(dx<0.01f)&&(dy>-0.01f)&&(dy<0.01f)) {
    retreatdx=GHOST_RETREAT_SPEED;
    retreatdy=0.0f;
    return;
  }
  
  float distance=sqrtf(dx*dx+dy*dy);
  retreatdx=(dx*GHOST_RETREAT_SPEED)/distance;
  retreatdy=(dy*GHOST_RETREAT_SPEED)/distance;
}

/* Close to hero. Curse her.
 */
 
static void ghost_apply_curse(struct fmn_sprite *sprite) {
  if (!fmn_hero_curse(sprite)) return;
  ghost_begin_RETREAT(sprite);
}

/* Update, PANIC.
 */
 
static void ghost_update_PANIC(struct fmn_sprite *sprite,float elapsed) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x;
  float dy=heroy-sprite->y;
  float distance=sqrtf(dx*dx+dy*dy);
  if (distance<GHOST_APPROACH_RECOVERY_DISTANCE) {
    stage=GHOST_STAGE_APPROACH;
    // but don't skip this update...
  }
  sprite->x+=(dx*GHOST_APPROACH_SPEED*elapsed)/distance;
  sprite->y+=(dy*GHOST_APPROACH_SPEED*elapsed)/distance;
}

/* Update, APPROACH.
 */
 
static void ghost_update_APPROACH(struct fmn_sprite *sprite,float elapsed) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float dx=herox-sprite->x;
  float dy=heroy-sprite->y;
  
  // If we're close enough, curse her and run away.
  // Or too far, enter PANIC stage to fly straight at her for a while.
  float md=((dx<0.0f)?-dx:dx)+((dy<0.0f)?-dy:dy);
  approachmd=md;
  if (md<=GHOST_IN_RANGE) {
    ghost_apply_curse(sprite);
    return;
  }
  if (md>=GHOST_APPROACH_PANIC_DISTANCE) {
    stage=GHOST_STAGE_PANIC;
    ghost_update_PANIC(sprite,elapsed);
    return;
  }
  
  float tandx=dy;
  float tandy=-dx;
  float tant=atan2f(tandy,tandx);
  float movet=tant+GHOST_APPROACH_ANGULAR_VELOCITY*elapsed;
  sprite->x+=cosf(movet)*GHOST_APPROACH_SPEED*elapsed;
  sprite->y+=sinf(movet)*GHOST_APPROACH_SPEED*elapsed;
}

/* Update, RETREAT.
 */
 
static void ghost_update_RETREAT(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  if (clock>=GHOST_RETREAT_TIME) {
    clock=0.0f;
    stage=GHOST_STAGE_APPROACH;
    return;
  }
  sprite->x+=retreatdx*elapsed;
  sprite->y+=retreatdy*elapsed;
}

/* Update.
 */
 
static void _ghost_update(struct fmn_sprite *sprite,float elapsed) {
  switch (stage) {
    case GHOST_STAGE_APPROACH: ghost_update_APPROACH(sprite,elapsed); break;
    case GHOST_STAGE_RETREAT: ghost_update_RETREAT(sprite,elapsed); break;
    case GHOST_STAGE_PANIC: ghost_update_PANIC(sprite,elapsed); break;
  }
}

/* Interact.
 */
 
static int16_t _ghost_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  switch (itemid) {
  }
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_ghost={
  .init=_ghost_init,
  .update=_ghost_update,
  .interact=_ghost_interact,
};
