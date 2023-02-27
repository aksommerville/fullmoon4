#include "app/sprite/fmn_sprite.h"

#define BEE_ROTATION_SPEED 4.0f /* radians/sec */
#define BEE_EXPANSION_SPEED 3.0f /* Hz */
#define BEE_PATH_RADIUS 1.0f /* meters */

#define tileid0 sprite->bv[0]
#define t sprite->fv[0]
#define rphase sprite->fv[1]
#define animclock sprite->fv[2]
#define x0 sprite->fv[3]
#define y0 sprite->fv[4]

/* Init.
 */
 
static void _bee_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  x0=sprite->x;
  y0=sprite->y;
  sprite->layer=150;
}

/* Update.
 */
 
static void _bee_update(struct fmn_sprite *sprite,float elapsed) {
  animclock+=elapsed;
  if (animclock>=0.60f) {
    animclock-=0.60f;
    sprite->tileid=tileid0;
  } else if (animclock>=0.30f) {
    sprite->tileid=tileid0+0x10;
  }
  t+=BEE_ROTATION_SPEED*elapsed;
  if (t>=M_PI) t-=M_PI*2.0f;
  rphase+=BEE_EXPANSION_SPEED*elapsed;
  if (rphase>=M_PI) rphase-=M_PI*2.0f;
  sprite->x=x0+cosf(t)*sinf(rphase)*BEE_PATH_RADIUS;
  sprite->y=y0+sinf(t)*sinf(rphase)*BEE_PATH_RADIUS;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_bee={
  .init=_bee_init,
  .update=_bee_update,
};
