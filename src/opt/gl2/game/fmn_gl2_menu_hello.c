int fmn_gl2_menu_hello_dummy=0;
#if 0 /*XXX move to client*/
#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"

#define HELLO_PERIOD 1200 /* frames, total time of our sequence */

/* Background stars.
 */
 
#define HELLO_STARC 500

static struct fmn_gl2_vertex_raw hello_star_vtxv[HELLO_STARC];
static struct hello_star {
  int alpha16;
  int dalpha16;
} hello_starv[HELLO_STARC];

static void hello_stars_reset(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  struct fmn_gl2_vertex_raw *vtx=hello_star_vtxv;
  struct hello_star *star=hello_starv;
  int i=HELLO_STARC;
  for (;i-->0;vtx++,star++) {
    vtx->x=rand()%DRIVER->mainfb.w;
    vtx->y=rand()%DRIVER->mainfb.h;
    uint8_t luma=0x40+(rand()&0x1f);
    vtx->r=luma+(rand()&0x1f)-0x10;
    vtx->g=luma+(rand()&0x1f)-0x10;
    vtx->b=luma+(rand()&0x1f)-0x10;
    vtx->a=0x00;
    star->alpha16=0x0000;
    star->dalpha16=20+(rand()&0x7f);
  }
}

static void hello_stars_draw(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  struct fmn_gl2_vertex_raw *vtx=hello_star_vtxv;
  struct hello_star *star=hello_starv;
  int i=HELLO_STARC;
  for (;i-->0;vtx++,star++) {
    star->alpha16+=star->dalpha16;
    if ((star->alpha16>=0xc000)&&(star->dalpha16>0)) star->dalpha16=-star->dalpha16;
    else if ((star->alpha16<=0)&&(star->dalpha16<0)) star->dalpha16=-star->dalpha16;
    vtx->a=star->alpha16>>8;
  }
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  #if FMN_USE_bcm /* can't tell why, but GL_POINTS just doesn't work on my pi. what did i do wrong... */
    for (i=HELLO_STARC>>1,vtx=hello_star_vtxv;i-->0;vtx+=2) { vtx[0].x=vtx[1].x+1; vtx[0].y=vtx[1].y; }
    fmn_gl2_draw_raw(GL_LINES,hello_star_vtxv,HELLO_STARC);
  #else
    fmn_gl2_draw_raw(GL_POINTS,hello_star_vtxv,HELLO_STARC);
  #endif
}

/* Animated title color.
 */
 
static struct hello_title_color {
  int time;
  uint8_t r,g,b,a;
} hello_title_colorv[]={
  {   0,0x00,0x00,0x00,0xff},
  {  40,0x00,0x00,0x00,0xff},
  { 120,0x00,0x00,0x00,0x00},
  { 120,0x00,0x00,0x00,0x00},
  { 100,0x00,0xff,0x00,0xff},
  { 100,0x00,0xff,0x00,0x00},
  {  20,0x00,0x00,0x00,0x00},
  { 200,0xff,0x00,0x00,0xff},
  { 190,0xff,0x00,0x00,0x00},
  {  10,0x00,0x00,0x00,0x00},
  { 300,0x00,0x00,0x00,0xff},
};

static int hello_title_colorp=1;
static int hello_title_colorc=sizeof(hello_title_colorv)/sizeof(struct hello_title_color);
static int hello_title_color_clock=0;
static struct hello_title_color *hello_title_color_prev,*hello_title_color_next;

static void hello_title_color_reset() {
  hello_title_colorp=1;
  hello_title_color_prev=hello_title_colorv;
  hello_title_color_next=hello_title_colorv+1;
  hello_title_color_clock=0;
}

static uint32_t hello_title_color_update() {
  hello_title_color_clock++;
  int leglen=hello_title_color_next->time;
  if (hello_title_color_clock>=leglen) {
    hello_title_color_clock=0;
    hello_title_colorp++;
    if (hello_title_colorp>=hello_title_colorc) hello_title_colorp=1;
    hello_title_color_next=hello_title_colorv+hello_title_colorp;
    hello_title_color_prev=hello_title_color_next-1;
  }
  uint8_t bweight=(hello_title_color_clock*0xff)/leglen;
  uint8_t aweight=0xff-bweight;
  uint8_t r=(hello_title_color_prev->r*aweight+hello_title_color_next->r*bweight)>>8;
  uint8_t g=(hello_title_color_prev->g*aweight+hello_title_color_next->g*bweight)>>8;
  uint8_t b=(hello_title_color_prev->b*aweight+hello_title_color_next->b*bweight)>>8;
  uint8_t a=(hello_title_color_prev->a*aweight+hello_title_color_next->a*bweight)>>8;
  return (r<<24)|(g<<16)|(b<<8)|a;
}

/* Render hello menu.
 */
 
void fmn_gl2_render_menu_hello(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  if (menu->framec==1) {
    hello_title_color_reset();
    hello_stars_reset(driver,menu);
  }
  
  int16_t fullw=DRIVER->mainfb.w,fullh=DRIVER->mainfb.h;
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,fullw,fullh,0x000000ff);
  int seqp=menu->framec%HELLO_PERIOD;
  
  hello_stars_draw(driver,menu);
  
  if (fmn_gl2_texture_use_imageid(driver,14)>=0) {
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    
    // The Moon.
    int16_t moony=0;
    {
      int16_t srcx=0,srcy=DRIVER->game.tilesize*5;
      int16_t w=DRIVER->game.tilesize*7,h=DRIVER->game.tilesize*7;
      int16_t dstx=(fullw>>1)-(w>>1);
      int16_t trackh=fullh+h*2; // starts one moonheight below, travels to just off the top edge
      int16_t dsty=fullh+h-(seqp*trackh)/HELLO_PERIOD;
      fmn_gl2_program_use(driver,&DRIVER->program_decal);
      fmn_gl2_draw_decal(dstx,dsty,w,h,srcx,srcy,w,h);
      moony=dsty+(h>>1);
    }
  
    // Dot flying by on a broom. Timing and position derived from Moon's position.
    // ("start" and "end" here run backward in time, they're sorted vertically).
    const int16_t moony_start=fullh>>2;
    const int16_t moony_end=(fullh*3)>>2;
    if ((moony>moony_start)&&(moony<moony_end)) {
      int16_t flyp=moony-moony_start;
      int16_t flyc=moony_end-moony_start;
      int16_t srcx=DRIVER->game.tilesize*8,srcy=DRIVER->game.tilesize*5;
      int16_t w=DRIVER->game.tilesize*5,h=DRIVER->game.tilesize*5;
      int16_t trackw=fullw+w;
      int16_t trackh=fullh+h;
      int16_t dstx=(flyp*trackw)/flyc-w;
      int16_t dsty=(flyp*trackh)/flyc-h;
      fmn_gl2_program_use(driver,&DRIVER->program_decal);
      fmn_gl2_draw_decal(dstx,dsty,w,h,srcx,srcy,w,h);
    }
    
  }
  
  // "Full Moon"
  if (fmn_gl2_texture_use_imageid(driver,18)>=0) {
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    int16_t w=DRIVER->texture->w,h=DRIVER->texture->h;
    int16_t dstx=(fullw>>1)-(w>>1);
    int16_t dsty=(fullh>>1)-(h>>1);
    fmn_gl2_draw_decal(dstx,dsty,w,h,0,0,w,h);
    if (fmn_gl2_texture_use_imageid(driver,19)>=0) {
      fmn_gl2_program_use(driver,&DRIVER->program_recal);
      fmn_gl2_draw_recal(&DRIVER->program_recal,dstx,dsty,w,h,0,0,w,h,hello_title_color_update());
    }
  }
}
#endif
