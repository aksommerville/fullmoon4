#include "app/sprite/fmn_sprite.h"

#define LOSTHAT_RADIUS_LIMIT 1.5f /* meters, we end so far down from where we started */
#define LOSTHAT_ANGLE_LIMIT 1.0f /* radians */
#define LOSTHAT_RADIUS_SPEED 0.500f /* m/s, rate of fall basically */
#define LOSTHAT_ANGLE_SPEED_MAX 3.000f /* r/s, rate of lateral sweep at the bottom */
#define LOSTHAT_ANGLE_SPEED_MIN 1.000f /* r/s, rate of lateral sweep at the ends */

#define radius sprite->fv[0]
#define angle sprite->fv[1]
#define sweepdir sprite->fv[2] /* 1.0 or -1.0 */
#define x0 sprite->fv[3]
#define y0 sprite->fv[4]
#define radiusend sprite->bv[0]
#define sweepend sprite->bv[1]

static void _losthat_init(struct fmn_sprite *sprite) {
  sprite->style=FMN_SPRITE_STYLE_TILE;
  radius=0.0f;
  angle=0.0f;
  sweepdir=1.0f;
  x0=sprite->x;
  y0=sprite->y;
}

static void _losthat_update(struct fmn_sprite *sprite,float elapsed) {

  if (!radiusend) {
    radius+=LOSTHAT_RADIUS_SPEED*elapsed;
    if (radius>=LOSTHAT_RADIUS_LIMIT) {
      radius=LOSTHAT_RADIUS_LIMIT;
      radiusend=1;
    }
  }
  
  if (!sweepend) {
    float pvangle=angle;
    float v=LOSTHAT_ANGLE_SPEED_MIN+(LOSTHAT_ANGLE_LIMIT-((angle<0.0f)?-angle:angle))*LOSTHAT_ANGLE_SPEED_MAX;
    v*=sweepdir;
    angle+=v*elapsed;
    if (sweepdir<0.0f) {
      if (angle<=-LOSTHAT_ANGLE_LIMIT) sweepdir=1.0f;
    } else {
      if (angle>=LOSTHAT_ANGLE_LIMIT) sweepdir=-1.0f;
    }
    if (radiusend) {
      if ((sweepdir<0.0f)&&(pvangle>=0.0f)&&(angle<=0.0f)) {
        angle=0.0f;
        sweepend=1;
      } else if ((sweepdir>0.0f)&&(pvangle<=0.0f)&&(angle>=0.0f)) {
        angle=0.0f;
        sweepend=1;
      }
    }
  }
  
  sprite->x=x0+radius*sinf(angle);
  sprite->y=y0+radius*cosf(angle);
  
  if (radiusend&&sweepend) {
    sprite->update=0;
  }
}

const struct fmn_sprite_controller fmn_sprite_controller_losthat={
  .init=_losthat_init,
  .update=_losthat_update,
};
