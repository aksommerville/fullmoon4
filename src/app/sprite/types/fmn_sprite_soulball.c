#include "app/sprite/fmn_sprite.h"
#include "app/hero/fmn_hero.h"

// Caller should indicate how many balls in my group, and my index among them.
#define grpp sprite->argv[0]
#define grpc sprite->argv[1]
#define return_to_sender sprite->argv[2] /* nonzero to coalesce on the hero's position, instead of going out forever. (NB we say "sender" but it's always the hero) */

#define tile0 sprite->bv[0]
#define animframe sprite->bv[1]
#define animclock sprite->bv[2]

#define dx sprite->fv[0]
#define dy sprite->fv[1]
#define clock sprite->fv[2]
#define x0 sprite->fv[3]
#define y0 sprite->fv[4]

#define SOULBALL_SPEED 6.0f
#define SOULBALL_FRAME_TIME 5 /* counts in update frames, regardless of their true length */
#define SOULBALL_FRAME_COUNT 6
#define SOULBALL_TIME_LIMIT 1.0f /* in return-to-sender mode only */
#define SOULBALL_RADIUS_MAX 3.0f /* return-to-sender; SPEED is not used */

static void _soulball_init(struct fmn_sprite *sprite) {
  if (grpc<1) grpc=1;
  grpp%=grpc;
  
  float t=(grpp*M_PI*2.0f)/grpc;
  if (return_to_sender) {
    x0=sprite->x;
    y0=sprite->y;
    dx=cosf(t);
    dy=sinf(t);
  } else {
    dx=cosf(t)*SOULBALL_SPEED;
    dy=sinf(t)*SOULBALL_SPEED;
  }
  
  tile0=sprite->tileid;
  animframe=(grpp*SOULBALL_FRAME_COUNT)/grpc;
  animclock=(grpp*SOULBALL_FRAME_TIME)/grpc;
}

/* Update, returning to the hero's position.
 * Instead of tracking only my absolute position, we track a focus point and distance from it.
 */

static void soulball_update_coalescing(struct fmn_sprite *sprite,float elapsed) {
  clock+=elapsed;
  if (clock>SOULBALL_TIME_LIMIT) {
    // it's possible to outrun the soulballs. give up after a quick interval.
    fmn_sprite_kill(sprite);
    return;
  }
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  float cx=x0+((herox-x0)*clock)/SOULBALL_TIME_LIMIT;
  float cy=y0+((heroy-y0)*clock)/SOULBALL_TIME_LIMIT;
  float r;
  const float halfrange=SOULBALL_TIME_LIMIT*0.5f;
  if (clock<halfrange) {
    r=(clock*SOULBALL_RADIUS_MAX)/halfrange;
  } else {
    r=((SOULBALL_TIME_LIMIT-clock)*SOULBALL_RADIUS_MAX)/halfrange;
  }
  sprite->x=cx+r*dx;
  sprite->y=cy+r*dy;
}

/* Update, running out until offscreen. (default)
 */
 
static void soulball_update_infinite(struct fmn_sprite *sprite,float elapsed) {
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

/* Update, animation and dispatch.
 */

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
  
  if (return_to_sender) {
    soulball_update_coalescing(sprite,elapsed);
  } else {
    soulball_update_infinite(sprite,elapsed);
  }
}

const struct fmn_sprite_controller fmn_sprite_controller_soulball={
  .init=_soulball_init,
  .update=_soulball_update,
};
