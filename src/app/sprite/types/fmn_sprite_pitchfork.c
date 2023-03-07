#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define orientation sprite->argv[0]
#define extension sprite->fv[0]
#define tripped sprite->bv[0]

#define PITCHFORK_MAX_EXTENSION 3.0f
#define PITCHFORK_EXTEND_SPEED 11.0f
#define PITCHFORK_RETRACT_SPEED 2.0f
#define PITCHFORK_LOS_RADIUS 0.8f
#define PITCHFORK_HURT_RADIUS 0.4f

#define PITCHFORK_TRIPPED_EXTEND 1
#define PITCHFORK_TRIPPED_RETRACT 2

/* Init.
 */
 
static void _pitchfork_init(struct fmn_sprite *sprite) {
  if (orientation) {
    sprite->xform=FMN_XFORM_XREV;
  } else {
    sprite->xform=0;
  }
}

/* Check if hero in range.
 */
 
static uint8_t pitchfork_check_hero(const struct fmn_sprite *sprite) {
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (orientation) {
    if (herox<sprite->x) return 0;
  } else {
    if (herox>sprite->x) return 0;
  }
  if (heroy<sprite->y-PITCHFORK_LOS_RADIUS) return 0;
  if (heroy>sprite->y+PITCHFORK_LOS_RADIUS) return 0;
  return 1;
}

/* Update.
 */
 
static void _pitchfork_update(struct fmn_sprite *sprite,float elapsed) {

  uint8_t action=0;
  if (tripped==PITCHFORK_TRIPPED_EXTEND) action=PITCHFORK_TRIPPED_EXTEND;
  else if (tripped==PITCHFORK_TRIPPED_RETRACT) action=PITCHFORK_TRIPPED_RETRACT;
  else if (pitchfork_check_hero(sprite)) action=tripped=PITCHFORK_TRIPPED_EXTEND;
  else action=PITCHFORK_TRIPPED_RETRACT;
  
  if (action==PITCHFORK_TRIPPED_EXTEND) {
    if (extension<PITCHFORK_MAX_EXTENSION) {
      extension+=PITCHFORK_EXTEND_SPEED*elapsed;
      if (extension>PITCHFORK_MAX_EXTENSION) extension=PITCHFORK_MAX_EXTENSION;
    } else {
      tripped=PITCHFORK_TRIPPED_RETRACT;
    }
  } else if (action==PITCHFORK_TRIPPED_RETRACT) {
    if (extension>0.0f) { 
      extension-=PITCHFORK_RETRACT_SPEED*elapsed;
      if (extension<0.0f) extension=0.0f;
    } else {
      tripped=0;
    }
  }
  
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float hurtx=sprite->x,hurty=sprite->y;
  if (orientation) hurtx+=1.0f+extension;
  else hurtx-=1.0f+extension;
  if (
    (herox>=hurtx-PITCHFORK_HURT_RADIUS)&&
    (herox<=hurtx+PITCHFORK_HURT_RADIUS)&&
    (heroy>=hurty-PITCHFORK_HURT_RADIUS)&&
    (heroy<=hurty+PITCHFORK_HURT_RADIUS)
  ) {
    fmn_hero_injure(hurtx,hurty,sprite);
  }
}

/* Interact.
 */
 
static int16_t _pitchfork_interact(struct fmn_sprite *sprite,uint8_t itemid,uint8_t qualifier) {
  return 0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_pitchfork={
  .init=_pitchfork_init,
  .update=_pitchfork_update,
  .interact=_pitchfork_interact,
};
