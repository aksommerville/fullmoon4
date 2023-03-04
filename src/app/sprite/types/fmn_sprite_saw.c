#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

#define SAW_SPEED 6.0f
#define SAW_X_RANGE 0.5f
#define SAW_YN_RANGE -0.0f
#define SAW_YP_RANGE 0.7f

#define tileid0 sprite->bv[0]
#define animclock sprite->fv[0]
#define dx sprite->fv[1]
#define xlo sprite->fv[2]
#define xhi sprite->fv[3]

/* Find the left and right limits of my motion.
 */
 
static void saw_locate_limits(struct fmn_sprite *sprite) {
  // In the absence of any other intel, set a range of +-1 meter.
  xlo=sprite->x-1.0f;
  xhi=sprite->x+1.0f;
  int8_t col=(int8_t)sprite->x;
  int8_t row=(int8_t)sprite->y;
  if ((col<0)||(row<0)||(col>=FMN_COLC)||(row>=FMN_ROWC)) return;
  uint8_t cellp=row*FMN_COLC+col;
  // The track tiles are 0x08 on the left and 0x0a on the right.
  // If we don't find an end tile, keep the default +-1.
  uint8_t leftp=0xff,rightp=0xff;
  uint8_t i;
  for (i=0;i<=col;i++) if (fmn_global.map[cellp-i]==0x08) { leftp=cellp-i; break; }
  for (i=0;i<FMN_COLC-col;i++) if (fmn_global.map[cellp+i]==0x0a) { rightp=cellp+i; break; }
  if (leftp!=0xff) xlo=sprite->x-(cellp-leftp);
  if (rightp!=0xff) xhi=sprite->x+(rightp-cellp);
}

/* Init.
 */
 
static void _saw_init(struct fmn_sprite *sprite) {
  sprite->y-=0.5f;
  tileid0=sprite->tileid;
  dx=-1.0f;
  saw_locate_limits(sprite);
}

/* Update.
 */
 
static void _saw_update(struct fmn_sprite *sprite,float elapsed) {
  animclock+=elapsed;
  switch ((int)(animclock*20.0f)%6) {
    case 0: sprite->tileid=tileid0+0; break;
    case 1: sprite->tileid=tileid0+1; break;
    case 2: sprite->tileid=tileid0+2; break;
    case 3: sprite->tileid=tileid0+3; break;
    case 4: sprite->tileid=tileid0+2; break;
    case 5: sprite->tileid=tileid0+1; break;
  }
  sprite->xform=(dx<0.0f)?0:FMN_XFORM_XREV;
  
  sprite->x+=dx*SAW_SPEED*elapsed;
  if (sprite->x<xlo) {
    sprite->x=xlo;
    dx=1.0f;
  } else if (sprite->x>xhi) {
    sprite->x=xhi;
    dx=-1.0f;
  }
  
  float herox=0.0f,heroy=0.0f;
  fmn_hero_get_position(&herox,&heroy);
  float distx=herox-sprite->x;
  if ((distx<SAW_X_RANGE)&&(distx>-SAW_X_RANGE)) {
    float disty=heroy-sprite->y;
    if ((disty<SAW_YP_RANGE)&&(disty>SAW_YN_RANGE)) {
      fmn_hero_injure(sprite->x,sprite->y,sprite);
    }
  }
}

/* Type definition.
 */
 
const struct fmn_sprite_controller fmn_sprite_controller_saw={
  .init=_saw_init,
  .update=_saw_update,
};
