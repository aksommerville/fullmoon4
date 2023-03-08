#include "app/sprite/fmn_sprite.h"
#include "app/fmn_game.h"

/* If OPPOSITE_SPEED<=SPEED, it will be impossible to walk against the flow.
 * That's not necessarily a bad idea, consider it.
 */
#define CONVEYOR_SPEED 2.0f
#define CONVEYOR_LIMIT_OPPOSITE_SPEED 3.0f

#define col sprite->bv[0]
#define row sprite->bv[1]
#define dir sprite->argv[0]
#define gsbit sprite->argv[1]

static uint8_t conveyor_dirv[FMN_COLC*FMN_ROWC]={0};
static uint8_t conveyor_gsbitv[FMN_COLC*FMN_ROWC]={0};

static void _conveyor_cb_gsbit(void *userdata,uint16_t p,uint8_t v) {
  struct fmn_sprite *sprite=userdata;
  if (v) {
    sprite->style=FMN_SPRITE_STYLE_EIGHTFRAME;
    conveyor_dirv[row*FMN_COLC+col]=dir;
  } else {
    sprite->style=FMN_SPRITE_STYLE_TILE;
    conveyor_dirv[row*FMN_COLC+col]=0;
  }
}

struct conveyor_convey_context {
  float elapsed;
};

static int conveyor_convey_1(struct fmn_sprite *pumpkin,void *userdata) {
  struct conveyor_convey_context *ctx=userdata;
  if (!(pumpkin->physics&FMN_PHYSICS_BLOWABLE)) return 0;
  if (pumpkin->x<0.0f) return 0;
  if (pumpkin->y<0.0f) return 0;
  if (pumpkin->x>=FMN_COLC) return 0;
  if (pumpkin->y>=FMN_ROWC) return 0;
  uint8_t xx=pumpkin->x;
  uint8_t yy=pumpkin->y;
  uint8_t p=yy*FMN_COLC+xx;
  if (!conveyor_dirv[p]) return 0;
  if (pumpkin->style==FMN_SPRITE_STYLE_HERO) { // TODO might need a "feet on ground" physics flag? This situation isn't unique to the broom.
    if (fmn_global.active_item==FMN_ITEM_BROOM) return 0;
  }
  /* We apply CONVEYOR_SPEED directly to the pumpkin.
   * That's 2.0 m/s, which tracks close, but not exact, to the animation.
   * Works great, except the hero's speed is considerably greater.
   * When she walks opposite the conveyor flow, you barely feel it.
   * So we add an artificial limit on the pumpkin's opposite velocity, in addition to the direct motion.
   */
  switch (conveyor_dirv[p]) {
    case FMN_DIR_W: {
        pumpkin->x-=CONVEYOR_SPEED*ctx->elapsed;
        if (pumpkin->velx>CONVEYOR_LIMIT_OPPOSITE_SPEED) pumpkin->velx=CONVEYOR_LIMIT_OPPOSITE_SPEED;
      } break;
    case FMN_DIR_E: {
        pumpkin->x+=CONVEYOR_SPEED*ctx->elapsed;
        if (pumpkin->velx<-CONVEYOR_LIMIT_OPPOSITE_SPEED) pumpkin->velx=-CONVEYOR_LIMIT_OPPOSITE_SPEED;
      } break;
    case FMN_DIR_N: {
        pumpkin->y-=CONVEYOR_SPEED*ctx->elapsed;
        if (pumpkin->vely>CONVEYOR_LIMIT_OPPOSITE_SPEED) pumpkin->vely=CONVEYOR_LIMIT_OPPOSITE_SPEED;
      } break;
    case FMN_DIR_S: {
        pumpkin->y+=CONVEYOR_SPEED*ctx->elapsed;
        if (pumpkin->vely<-CONVEYOR_LIMIT_OPPOSITE_SPEED) pumpkin->vely=-CONVEYOR_LIMIT_OPPOSITE_SPEED;
      } break;
  }
  return 0;
}

static void _conveyor_update(void *userdata,float elapsed) {
  struct conveyor_convey_context ctx={
    .elapsed=elapsed,
  };
  fmn_sprites_for_each(conveyor_convey_1,&ctx);
}

static void _conveyor_cleanup(void *userdata) {
  memset(conveyor_dirv,0,sizeof(conveyor_dirv));
  memset(conveyor_gsbitv,0,sizeof(conveyor_gsbitv));
}

static void _conveyor_init(struct fmn_sprite *sprite) {
  switch (dir) {
    case FMN_DIR_W: break;
    case FMN_DIR_E: sprite->xform=FMN_XFORM_XREV; break;
    case FMN_DIR_N: sprite->xform=FMN_XFORM_SWAP; break;
    case FMN_DIR_S: sprite->xform=FMN_XFORM_SWAP|FMN_XFORM_XREV; break;
    default: fmn_sprite_kill(sprite); return;
  }
  if ((sprite->x<0.0f)||(sprite->y<0.0f)||(sprite->x>=FMN_COLC)||(sprite->y>=FMN_ROWC)) {
    fmn_sprite_kill(sprite);
    return;
  }
  col=(uint8_t)sprite->x;
  row=(uint8_t)sprite->y;
  if (conveyor_dirv[row*FMN_COLC+col]||conveyor_gsbitv[row*FMN_COLC+col]) {
    fmn_sprite_kill(sprite);
    return;
  }
  conveyor_dirv[row*FMN_COLC+col]=dir;
  if (gsbit) {
    conveyor_dirv[row*FMN_COLC+col]=0;
    conveyor_gsbitv[row*FMN_COLC+col]=gsbit;
    fmn_gs_listen_bit(gsbit,_conveyor_cb_gsbit,sprite);
    if (!fmn_gs_get_bit(gsbit)) {
      sprite->style=FMN_SPRITE_STYLE_TILE;
    }
  }
  fmn_game_register_map_singleton(
    _conveyor_init,
    _conveyor_update,
    _conveyor_cleanup,
    0
  );
}

const struct fmn_sprite_controller fmn_sprite_controller_conveyor={
  .init=_conveyor_init,
};
