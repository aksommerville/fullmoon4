#include "app/sprite/fmn_sprite.h"

#define BEE_ROTATION_SPEED 4.0f /* radians/sec */
#define BEE_EXPANSION_SPEED 3.0f /* Hz */
#define BEE_PATH_RADIUS 1.0f /* meters */
#define BEE_RETURN_SPEED 1.0f

#define tileid0 sprite->bv[0]
#define windtrack sprite->bv[1]
#define t sprite->fv[0]
#define rphase sprite->fv[1]
#define animclock sprite->fv[2]
#define x0 sprite->fv[3]
#define y0 sprite->fv[4]
#define x00 sprite->fv[5]
#define y00 sprite->fv[6]

/* Init.
 */
 
static void _bee_init(struct fmn_sprite *sprite) {
  tileid0=sprite->tileid;
  x0=x00=sprite->x;
  y0=y00=sprite->y;
  sprite->layer=150;
  sprite->physics=FMN_PHYSICS_BLOWABLE;
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
  
  if (windtrack>=20) {
    if (x0>x00) {
      if ((x0-=BEE_RETURN_SPEED*elapsed)<x00) x0=x00;
    } else if (x0<x00) {
      if ((x0+=BEE_RETURN_SPEED*elapsed)>x00) x0=x00;
    }
    if (y0>y00) {
      if ((y0-=BEE_RETURN_SPEED*elapsed)<y00) y0=y00;
    } else if (y0<y00) {
      if ((y0+=BEE_RETURN_SPEED*elapsed)>y00) y0=y00;
    }
  } else {
    windtrack++;
  }
}

/* Receive wind.
 * (it modifies (x0,y0); modifying regular (x,y) would get overwritten.
 */
 
static void _bee_wind(struct fmn_sprite *sprite,float dx,float dy) {
  x0+=dx;
  y0+=dy;
  windtrack=0;
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_bee={
  .init=_bee_init,
  .update=_bee_update,
  .wind=_bee_wind,
};
