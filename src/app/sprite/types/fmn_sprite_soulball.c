#include "app/sprite/fmn_sprite.h"

// Caller should indicate how many balls in my group, and my index among them.
#define grpp sprite->argv[0]
#define grpc sprite->argv[1]

#define tile0 sprite->bv[0]
#define animframe sprite->bv[1]
#define animclock sprite->bv[2]

#define dx sprite->fv[0]
#define dy sprite->fv[1]

#define SOULBALL_SPEED 4.0f
#define SOULBALL_FRAME_TIME 5 /* counts in update frames, regardless of their true length */
#define SOULBALL_FRAME_COUNT 6

static void _soulball_init(struct fmn_sprite *sprite) {
  if (grpc<1) grpc=1;
  grpp%=grpc;
  
  float t=(grpp*M_PI*2.0f)/grpc;
  dx=cosf(t)*SOULBALL_SPEED;
  dy=sinf(t)*SOULBALL_SPEED;
  
  tile0=sprite->tileid;
  animframe=(grpp*SOULBALL_FRAME_COUNT)/grpc;
  animclock=(grpp*SOULBALL_FRAME_TIME)/grpc;
}

static void _soulball_update(struct fmn_sprite *sprite,float elapsed) {
  
  if (animclock) {
    animclock--;
  } else {
    animclock=SOULBALL_FRAME_TIME;
    animframe++;
    if (animframe>=SOULBALL_FRAME_COUNT) animframe=0;
    switch (animframe) {
      case 0: sprite->tileid=tile0+0; break;
      case 1: sprite->tileid=tile0+1; break;
      case 2: sprite->tileid=tile0+2; break;
      case 3: sprite->tileid=tile0+3; break;
      case 4: sprite->tileid=tile0+2; break;
      case 5: sprite->tileid=tile0+1; break;
    }
  }
  
  sprite->x+=dx*elapsed;
  sprite->y+=dy*elapsed;
  if (
    (sprite->x<-1.0f)||
    (sprite->y<-1.0f)||
    (sprite->x>FMN_COLC+1.0f)||
    (sprite->y>FMN_ROWC+1.0f)
  ) {
    fmn_sprite_kill(sprite);
  }
}

const struct fmn_sprite_controller fmn_sprite_controller_soulball={
  .init=_soulball_init,
  .update=_soulball_update,
};
