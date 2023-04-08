#include "../fmn_gl2_internal.h"
#include "app/hero/fmn_hero.h"

void fmn_gl2_game_render_world(struct bigpc_render_driver *driver);

/* Transition beginning. Capture "from" image.
 */
 
void fmn_gl2_game_transition_begin(struct bigpc_render_driver *driver) {
  fmn_gl2_framebuffer_use(driver,&DRIVER->game.transitionfrom);
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  driver->transition_in_progress=1;
  fmn_gl2_game_render_world(driver);
  driver->transition_in_progress=0;
  
  // Capture color and "from" position, in case we need them.
  if (fmn_global.mapdark) DRIVER->game.transitioncolor=0x22004400;
  else DRIVER->game.transitioncolor=0x00000000;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  DRIVER->game.transitionfromx=herox*DRIVER->game.tilesize;
  DRIVER->game.transitionfromy=heroy*DRIVER->game.tilesize;
}

/* Commit transition.
 * Update color and "to" position.
 */
 
void fmn_gl2_game_transition_commit(struct bigpc_render_driver *driver) {
  DRIVER->game.transitionc=FMN_GL2_TRANSITION_FRAMEC; // make it official
  driver->transition_in_progress=1;
  if (fmn_global.mapdark) DRIVER->game.transitioncolor=0x22004400; // purple if either "from" or "to" is dark.
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  DRIVER->game.transitiontox=herox*DRIVER->game.tilesize;
  DRIVER->game.transitiontoy=heroy*DRIVER->game.tilesize;
}

/* Pan.
 * (dx,dy) are the direction of the camera's apparent motion. Framebuffers slide the opposite way.
 */
 
static void fmn_gl2_pan(
  struct bigpc_render_driver *driver,
  int p,int c,
  struct fmn_gl2_framebuffer *from,struct fmn_gl2_framebuffer *to,
  int dx,int dy
) {
  if ((p<=0)||(p>=c)) {
    struct fmn_gl2_framebuffer *src=(p<=0)?from:to;
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    fmn_gl2_texture_use_object(driver,&src->texture);
    fmn_gl2_draw_decal(
      0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
      0,src->texture.h,src->texture.w,-src->texture.h
    );
    return;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  int16_t dxpx=(dx*DRIVER->mainfb.texture.w*p)/c;
  int16_t dypx=(dy*DRIVER->mainfb.texture.h*p)/c;
  int16_t dstx=-dxpx,dsty=-dypx; // where (from) goes
  fmn_gl2_texture_use_object(driver,&from->texture);
  fmn_gl2_draw_decal(
    dstx,dsty,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
    0,from->texture.h,from->texture.w,-from->texture.h
  );
  dstx+=dx*DRIVER->mainfb.texture.w;
  dsty+=dy*DRIVER->mainfb.texture.h;
  fmn_gl2_texture_use_object(driver,&to->texture);
  fmn_gl2_draw_decal(
    dstx,dsty,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
    0,to->texture.h,to->texture.w,-to->texture.h
  );
}

/* Fade to an intermediate color, then to the final.
 */
 
static void fmn_gl2_fade2(
  struct bigpc_render_driver *driver,
  int p,int c,
  struct fmn_gl2_framebuffer *from,struct fmn_gl2_framebuffer *to
) {
  struct fmn_gl2_framebuffer *src;
  int halfc=c>>1;
  if (p<halfc) {
    src=from;
    c=halfc;
  } else {
    src=to;
    p=c-p;
    c=halfc;
  }
  if (p<c) {
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    fmn_gl2_texture_use_object(driver,&src->texture);
    fmn_gl2_draw_decal(
      0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
      0,src->texture.h,src->texture.w,-src->texture.h
    );
  }
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  int a=(p*0x100)/c;
  if (a<0) a=0; else if (a>0xff) a=0xff;
  uint32_t rgba=DRIVER->game.transitioncolor|a; // TODO intermediate color like in web -- one side has lights out, use a dark purple instead of black
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,rgba);
}

/* Spotlight.
 */
 
static void fmn_gl2_spotlight(
  struct bigpc_render_driver *driver,
  int p,int c,
  struct fmn_gl2_framebuffer *from,struct fmn_gl2_framebuffer *to
) {
  struct fmn_gl2_framebuffer *src;
  int16_t fx,fy;
  int halfc=c>>1;
  if (p<halfc) {
    src=from;
    c=halfc;
    fx=DRIVER->game.transitionfromx;
    fy=DRIVER->game.transitionfromy;
  } else {
    src=to;
    p=c-p;
    c=halfc;
    fx=DRIVER->game.transitiontox;
    fy=DRIVER->game.transitiontoy;
  }
  if (p>=c) {
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,DRIVER->game.transitioncolor|0xff);
    return;
  }
  
  // Start with the unmodified scene.
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use_object(driver,&src->texture);
  fmn_gl2_draw_decal(
    0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
    0,src->texture.h,src->texture.w,-src->texture.h
  );
  if (p<=0) return;
  
  // Spotlight's radius is the distance from focus to the furthest corner.
  int16_t fullw=DRIVER->mainfb.texture.w;
  int16_t fullh=DRIVER->mainfb.texture.h;
  float dx,dy;
  if (fx>fullw>>1) dx=fx; else dx=fullw-fx;
  if (fy>fullh>>1) dy=fy; else dy=fullh-fy;
  int16_t radius_limit=sqrtf(dx*dx+dy*dy)+0.9f;
  int16_t radius=((c-p)*radius_limit)/c;
  
  // Circle shape will be tricky. First try it as a square, to make sure it animates right.
  uint32_t bgcolor=DRIVER->game.transitioncolor|0xff;
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  int16_t ax=fx-radius,zx=fx+radius,ay=fy-radius,zy=fy+radius;
  if (ax>0) fmn_gl2_draw_raw_rect(0,0,ax,fullh,bgcolor);
  if (ay>0) fmn_gl2_draw_raw_rect(0,0,fullw,ay,bgcolor);
  if (zx<fullw) fmn_gl2_draw_raw_rect(zx,0,fullw-zx,fullh,bgcolor);
  if (zy<fullh) fmn_gl2_draw_raw_rect(0,zy,fullw,fullh-zy,bgcolor);
  
  // And finally, draw the circle part as a texture.
  if (fmn_gl2_texture_use(driver,17)>=0) {
    fmn_gl2_program_use(driver,&DRIVER->program_recal);
    fmn_gl2_draw_recal(&DRIVER->program_recal,ax,ay,zx-ax,zy-ay,0,0,DRIVER->texture->w,DRIVER->texture->h,DRIVER->game.transitioncolor|0xff);
  }
}

/* Apply transition.
 * This could be done without all the parameters, but I feel it's cleaner this way.
 * Output framebuffer is not specified -- caller must set that up in advance.
 */
 
void fmn_gl2_game_transition_apply(
  struct bigpc_render_driver *driver,
  int transition,int p,int c,
  struct fmn_gl2_framebuffer *from,struct fmn_gl2_framebuffer *to
) {
  if ((p>=0)&&(p<c)) switch (transition) {
    case FMN_TRANSITION_PAN_LEFT: fmn_gl2_pan(driver,p,c,from,to,-1,0); return;
    case FMN_TRANSITION_PAN_RIGHT: fmn_gl2_pan(driver,p,c,from,to,1,0); return;
    case FMN_TRANSITION_PAN_UP: fmn_gl2_pan(driver,p,c,from,to,0,-1); return;
    case FMN_TRANSITION_PAN_DOWN: fmn_gl2_pan(driver,p,c,from,to,0,1); return;
    case FMN_TRANSITION_FADE_BLACK: fmn_gl2_fade2(driver,p,c,from,to); return;
    case FMN_TRANSITION_DOOR: fmn_gl2_spotlight(driver,p,c,from,to); return;
    case FMN_TRANSITION_WARP: fmn_gl2_fade2(driver,p,c,from,to); return;
  }
  // Fallback: No transition, render the (to) state and unset "in progress".
  // NB This is a fallback only. Normal end-of-transition is at fmn_gl2_game_render().
  driver->transition_in_progress=0;
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  fmn_gl2_texture_use_object(driver,&DRIVER->game.transitionto.texture);
  fmn_gl2_draw_decal(
    0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,
    0,DRIVER->game.transitionto.texture.h,DRIVER->game.transitionto.texture.w,-DRIVER->game.transitionto.texture.h
  );
}
