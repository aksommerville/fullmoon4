#include "app/sprite/fmn_sprite.h"
#include "app/sprite/fmn_physics.h"

#define FMN_PUSHBLOCK_PRESS_TIME 1.0f
#define FMN_PUSHBLOCK_SPEED 5.0f
#define FMN_PUSHBLOCK_MINIMUM_TRAVEL 0.01f

#define pressflag sprite->bv[0]
#define pressdir sprite->bv[1]
#define movedir sprite->bv[2]
#define presstime sprite->fv[0]
#define recenttime sprite->fv[1]
#define pending_voyage sprite->fv[2]

static void fmn_pushblock_update_motion(struct fmn_sprite *sprite,float elapsed) {

  float distance=elapsed*FMN_PUSHBLOCK_SPEED;
  if (distance>=pending_voyage) distance=pending_voyage;
    
  float dx,dy;
  fmn_vector_from_dir(&dx,&dy,movedir);
  dx*=distance;
  dy*=distance;
  sprite->x+=dx;
  sprite->y+=dy;
    
  pending_voyage-=distance;
  if (pending_voyage<=0.0f) {
    movedir=0;
  }
}

static float fmn_pushblock_distance_to_destination(float from,int8_t d) {
  // Quantize (from) and target the center of the next cell.
  int8_t fromq=(int8_t)from;
  int8_t toq=fromq+d;
  float to=toq+0.5f;
  float distance=to-from;
  if (distance<0.0f) distance=-distance;
  return distance;
}

static void fmn_pushblock_begin_motion(struct fmn_sprite *sprite) {
  movedir=pressdir;
  presstime=0.0f;
  pressflag=0;
  pressdir=0;
  
  // Decide how far we are going to move.
  // Normally it's exactly one meter, but can be less if we are misaligned.
  switch (movedir) {
    case FMN_DIR_W: pending_voyage=fmn_pushblock_distance_to_destination(sprite->x,-1); break;
    case FMN_DIR_E: pending_voyage=fmn_pushblock_distance_to_destination(sprite->x, 1); break;
    case FMN_DIR_N: pending_voyage=fmn_pushblock_distance_to_destination(sprite->y,-1); break;
    case FMN_DIR_S: pending_voyage=fmn_pushblock_distance_to_destination(sprite->y, 1); break;
    default: {
        // oops that's not a valid direction, forget it.
        movedir=0;
        return;
      }
  }
  if (pending_voyage<=0.0f) {
    movedir=0;
    return;
  }
}

static void _pushblock_update(struct fmn_sprite *sprite,float elapsed) {
  recenttime=elapsed;
  
  if (movedir) {
    // Motion in progress. Nothing else can happen, until it's complete.
    fmn_pushblock_update_motion(sprite,elapsed);
    
  } else if (presstime>FMN_PUSHBLOCK_PRESS_TIME) {
    // We have been pressed for long enough. Begin motion.
    fmn_pushblock_begin_motion(sprite);
    
  } else if (pressflag) {
    // We are currently being pressed. Let _pushblock_pressure continue tallying the duration.
    pressflag=0;
    
  } else if (presstime>0.0f) {
    // Pressure ended and some time was on the clock. Probably doesn't mean a thing, except drop the tally.
    presstime=0.0f;
  }
}

static void _pushblock_pressure(struct fmn_sprite *sprite,struct fmn_sprite *presser,uint8_t dir) {
  if (movedir) return;
  pressflag=1;
  if (dir==pressdir) {
    presstime+=recenttime;
  } else {
    pressdir=dir;
    presstime=recenttime;
  }
}

const struct fmn_sprite_controller fmn_sprite_controller_pushblock={
  .update=_pushblock_update,
  .pressure=_pushblock_pressure,
};
