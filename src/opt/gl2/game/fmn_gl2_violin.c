#include "../fmn_gl2_internal.h"
#include "app/hero/fmn_hero.h"

/* Violin chart context, generated from scratch every frame.
 */
 
#define FMN_GL2_VIOLIN_BAR_LIMIT 7
 
struct fmn_gl2_violin_context {
  int16_t x,y,w,h; // full bounds within mainfb
  struct fmn_gl2_violin_line {
    int16_t y; // absolute
    uint32_t rgba;
  } linev[4]; // top to bottom
  struct fmn_gl2_violin_bar {
    int16_t x; // absolute
    uint32_t rgba;
  } barv[FMN_GL2_VIOLIN_BAR_LIMIT];
  struct fmn_gl2_violin_note {
    int16_t x,y; // absolute
    uint8_t tileid;
  } notev[FMN_VIOLIN_SONG_LENGTH];
  uint8_t notec;
};

static void fmn_gl2_violin_begin(struct fmn_gl2_violin_context *ctx,struct bigpc_render_driver *driver) {

  int16_t ymargin=DRIVER->game.tilesize;
  ctx->x=0;
  ctx->w=DRIVER->mainfb.texture.w;
  ctx->h=DRIVER->mainfb.texture.h/3;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (heroy<FMN_ROWC*0.5f) {
    ctx->y=DRIVER->mainfb.texture.h-ymargin-ctx->h;
  } else {
    ctx->y=ymargin;
  }
  
  struct fmn_gl2_violin_line *line=ctx->linev;
  int i=0;
  for (;i<4;i++,line++) {
    line->y=ctx->y+((i+1)*ctx->h)/5;
    line->rgba=0x886644ff;
  }
  switch (fmn_global.wand_dir) {
    case FMN_DIR_N: ctx->linev[0].rgba=0xcc0000ff; break;
    case FMN_DIR_E: ctx->linev[1].rgba=0xcc0000ff; break;
    case FMN_DIR_W: ctx->linev[2].rgba=0xcc0000ff; break;
    case FMN_DIR_S: ctx->linev[3].rgba=0xcc0000ff; break;
  }
  
  int note_spacing=ctx->w/FMN_VIOLIN_SONG_LENGTH;
  int bar_spacing=note_spacing<<2;
  if (bar_spacing<1) bar_spacing=1;
  float position_in_bar=((fmn_global.violin_songp&3)+fmn_global.violin_clock)/4.0f;
  int16_t x=ctx->x-(int16_t)(position_in_bar*bar_spacing);
  int16_t leftx=x+(note_spacing>>1);
  x-=note_spacing;
  struct fmn_gl2_violin_bar *bar=ctx->barv;
  int barc=0;
  for (;(x<ctx->x+ctx->w)&&(barc<FMN_GL2_VIOLIN_BAR_LIMIT);x+=bar_spacing,bar++,barc++) {
    bar->x=x;
    bar->rgba=0x886644ff;
  }
  
  uint8_t songp=fmn_global.violin_songp;
  x=leftx+(songp&3)*note_spacing;
  ctx->notec=0;
  for (i=FMN_VIOLIN_SONG_LENGTH;i-->0;songp++,x+=note_spacing) {
    if (songp>=FMN_VIOLIN_SONG_LENGTH) songp=0;
    struct fmn_gl2_violin_note *note=ctx->notev+ctx->notec;
    switch (fmn_global.violin_song[songp]) {
      case FMN_DIR_N: note->y=ctx->linev[0].y; note->tileid=0xc4; break;
      case FMN_DIR_E: note->y=ctx->linev[1].y; note->tileid=0xc5; break;
      case FMN_DIR_W: note->y=ctx->linev[2].y; note->tileid=0xc6; break;
      case FMN_DIR_S: note->y=ctx->linev[3].y; note->tileid=0xc7; break;
      default: continue;
    }
    note->x=x;
    ctx->notec++;
  }
}

/* Background elements.
 */
 
static void fmn_gl2_render_violin_background(struct bigpc_render_driver *driver,struct fmn_gl2_violin_context *ctx) {
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  
  // Main background color blips on alternate beeps.
  if ((fmn_global.violin_songp&1)&&(fmn_global.violin_clock<0.5f)) {
    int aweight=fmn_global.violin_clock*512.0f; if (aweight>0xff) aweight=0xff;
    int bweight=0xff-aweight;
    int r=(0xd8*aweight+0xf8*bweight)>>8; if (r<0) r=0; else if (r>0xff) r=0xff;
    int g=(0xd0*aweight+0xf4*bweight)>>8; if (g<0) g=0; else if (g>0xff) g=0xff;
    int b=(0xc0*aweight+0xe0*bweight)>>8; if (b<0) b=0; else if (b>0xff) b=0xff;
    fmn_gl2_draw_raw_rect(ctx->x,ctx->y,ctx->w,ctx->h,(r<<24)|(g<<16)|(b<<8)|0xe0);
  } else {
    fmn_gl2_draw_raw_rect(ctx->x,ctx->y,ctx->w,ctx->h,0xf8f4e0e0);
  }
  
  fmn_gl2_draw_raw_rect(ctx->x,ctx->y,ctx->w,1,0x000000ff);
  fmn_gl2_draw_raw_rect(ctx->x,ctx->y+ctx->h-1,ctx->w,1,0x000000ff);
  fmn_gl2_draw_raw_rect(ctx->x,ctx->y+ctx->h,ctx->w,1,0x00000080);
  
  struct fmn_gl2_vertex_raw lvtxv[22]; // (bar limit + line limit) * 2
  struct fmn_gl2_vertex_raw *lvtx=lvtxv;
  int i;
  const struct fmn_gl2_violin_bar *bar=ctx->barv;
  for (i=FMN_GL2_VIOLIN_BAR_LIMIT;i-->0;bar++) {
    if (!bar->rgba) continue;
    lvtx->x=bar->x;
    lvtx->y=ctx->linev[0].y;
    lvtx->r=bar->rgba>>24;
    lvtx->g=bar->rgba>>16;
    lvtx->b=bar->rgba>>8;
    lvtx->a=0xff;
    lvtx[1]=lvtx[0];
    lvtx[1].y=ctx->linev[3].y;
    lvtx+=2;
  }
  const struct fmn_gl2_violin_line *line=ctx->linev;
  for (i=4;i-->0;line++,lvtx+=2) {
    lvtx->x=ctx->x;
    lvtx->y=line->y;
    lvtx->r=line->rgba>>24;
    lvtx->g=line->rgba>>16;
    lvtx->b=line->rgba>>8;
    lvtx->a=line->rgba;
    lvtx[1]=lvtx[0];
    lvtx[1].x+=ctx->w;
  }
  fmn_gl2_draw_raw(GL_LINES,lvtxv,lvtx-lvtxv);
  
  if (ctx->notec&&(fmn_gl2_texture_use(driver,2)>=0)) {
    struct fmn_gl2_vertex_mintile tvtxv[FMN_VIOLIN_SONG_LENGTH];
    struct fmn_gl2_vertex_mintile *tvtx=tvtxv;
    const struct fmn_gl2_violin_note *note=ctx->notev;
    for (i=ctx->notec;i-->0;note++) {
      if (!note->tileid) continue;
      tvtx->x=note->x;
      tvtx->y=note->y;
      tvtx->tileid=note->tileid;
      tvtx->xform=0;
      tvtx++;
    }
    fmn_gl2_program_use(driver,&DRIVER->program_mintile);
    fmn_gl2_draw_mintile(tvtxv,tvtx-tvtxv);
  }
}

/* Render violin chart, main entry point.
 */
 
void fmn_gl2_render_violin(struct bigpc_render_driver *driver) {
  struct fmn_gl2_violin_context ctx={0};
  fmn_gl2_violin_begin(&ctx,driver);
  fmn_gl2_render_violin_background(driver,&ctx);
}
