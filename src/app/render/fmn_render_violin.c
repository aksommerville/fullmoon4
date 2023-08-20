#include "fmn_render_internal.h"
#include "app/hero/fmn_hero.h"

/* Violin chart context, generated from scratch every frame.
 */
 
#define FMN_VIOLIN_BAR_LIMIT 7
 
struct fmn_violin_context {
  int16_t x,y,w,h; // full bounds within mainfb
  struct fmn_violin_line {
    int16_t y; // absolute
    uint32_t pixel;
  } linev[4]; // top to bottom
  struct fmn_violin_bar {
    int16_t x; // absolute
    uint32_t pixel;
  } barv[FMN_VIOLIN_BAR_LIMIT];
  struct fmn_violin_note {
    int16_t x,y; // absolute
    uint8_t tileid;
  } notev[FMN_VIOLIN_SONG_LENGTH*2]; // *2 because it includes shadow notes
  uint8_t notec;
};

static void fmn_violin_begin(struct fmn_violin_context *ctx) {

  int16_t ymargin=fmn_render_global.tilesize;
  ctx->x=0;
  ctx->w=fmn_render_global.fbw;
  ctx->h=fmn_render_global.fbh/3;
  float herox,heroy;
  fmn_hero_get_position(&herox,&heroy);
  if (heroy<FMN_ROWC*0.5f) {
    ctx->y=fmn_render_global.fbh-ymargin-ctx->h;
  } else {
    ctx->y=ymargin;
  }
  
  struct fmn_violin_line *line=ctx->linev;
  int i=0;
  for (;i<4;i++,line++) {
    line->y=ctx->y+((i+1)*ctx->h)/5;
    line->pixel=fmn_render_global.violin_line_color;
  }
  switch (fmn_global.wand_dir) {
    case FMN_DIR_N: ctx->linev[0].pixel=fmn_render_global.violin_highlight_line_color; break;
    case FMN_DIR_E: ctx->linev[1].pixel=fmn_render_global.violin_highlight_line_color; break;
    case FMN_DIR_W: ctx->linev[2].pixel=fmn_render_global.violin_highlight_line_color; break;
    case FMN_DIR_S: ctx->linev[3].pixel=fmn_render_global.violin_highlight_line_color; break;
  }
  
  int note_spacing=ctx->w/FMN_VIOLIN_SONG_LENGTH;
  int bar_spacing=note_spacing<<1;
  if (bar_spacing<1) bar_spacing=1;
  float position_in_bar=((fmn_global.violin_songp&1)+fmn_global.violin_clock)/2.0f;
  int16_t x=ctx->x-(int16_t)(position_in_bar*bar_spacing);
  int16_t leftx=x+(note_spacing>>1);
  x+=note_spacing;
  struct fmn_violin_bar *bar=ctx->barv;
  int barc=0;
  for (;(x<ctx->x+ctx->w)&&(barc<FMN_VIOLIN_BAR_LIMIT);x+=bar_spacing,bar++,barc++) {
    bar->x=x;
    bar->pixel=fmn_render_global.violin_line_color;
  }
  
  // Shadow notes...
  uint8_t songp=fmn_global.violin_songp;
  x=leftx+(songp&1)*note_spacing;
  ctx->notec=0;
  for (i=FMN_VIOLIN_SONG_LENGTH;i-->0;songp++,x+=note_spacing) {
    if (songp>=FMN_VIOLIN_SONG_LENGTH) songp=0;
    struct fmn_violin_note *note=ctx->notev+ctx->notec;
    switch (fmn_global.violin_shadow[songp]) {
      case FMN_DIR_N: note->y=ctx->linev[0].y; note->tileid=0xc8; break;
      case FMN_DIR_E: note->y=ctx->linev[1].y; note->tileid=0xc9; break;
      case FMN_DIR_W: note->y=ctx->linev[2].y; note->tileid=0xca; break;
      case FMN_DIR_S: note->y=ctx->linev[3].y; note->tileid=0xcb; break;
      default: continue;
    }
    note->x=x;
    ctx->notec++;
  }
  
  // Live notes...
  songp=fmn_global.violin_songp;
  x=leftx+(songp&1)*note_spacing;
  for (i=FMN_VIOLIN_SONG_LENGTH;i-->0;songp++,x+=note_spacing) {
    if (songp>=FMN_VIOLIN_SONG_LENGTH) songp=0;
    struct fmn_violin_note *note=ctx->notev+ctx->notec;
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

/* Render.
 */
 
static void fmn_render_violin_context(struct fmn_violin_context *ctx) {

  // Background, top and bottom lines, bottom shadow.
  struct fmn_draw_rect rectv[4];
  if (fmn_global.violin_clock<0.5f) {
    int aweight=fmn_global.violin_clock*512.0f; if (aweight>0xff) aweight=0xff;
    int bweight=0xff-aweight;
    int r=(0xd8*aweight+0xf8*bweight)>>8; if (r<0) r=0; else if (r>0xff) r=0xff;
    int g=(0xd0*aweight+0xf4*bweight)>>8; if (g<0) g=0; else if (g>0xff) g=0xff;
    int b=(0xc0*aweight+0xe0*bweight)>>8; if (b<0) b=0; else if (b>0xff) b=0xff;
    rectv[0]=(struct fmn_draw_rect){ctx->x,ctx->y,ctx->w,ctx->h,(r<<24)|(g<<16)|(b<<8)|0xe0};
  } else {
    rectv[0]=(struct fmn_draw_rect){ctx->x,ctx->y,ctx->w,ctx->h,0xf8f4e0e0};
  }
  rectv[1]=(struct fmn_draw_rect){ctx->x,ctx->y,ctx->w,1,0x000000ff};
  rectv[2]=(struct fmn_draw_rect){ctx->x,ctx->y+ctx->h-1,ctx->w,1,0x000000ff};
  rectv[3]=(struct fmn_draw_rect){ctx->x,ctx->y+ctx->h,ctx->w,1,0x00000080};
  fmn_draw_rect(rectv,4);
  
  // Lines horz and vert.
  struct fmn_draw_line linev[11]; // bar limit + line limit
  struct fmn_draw_line *lvtx=linev;
  int i;
  const struct fmn_violin_bar *bar=ctx->barv;
  for (i=FMN_VIOLIN_BAR_LIMIT;i-->0;bar++) {
    if (!bar->pixel) continue;
    lvtx->ax=bar->x;
    lvtx->ay=ctx->linev[0].y;
    lvtx->bx=bar->x;
    lvtx->by=ctx->linev[3].y;
    lvtx->pixel=bar->pixel;
    lvtx++;
  }
  const struct fmn_violin_line *line=ctx->linev;
  for (i=4;i-->0;line++,lvtx++) {
    lvtx->ax=ctx->x;
    lvtx->ay=line->y;
    lvtx->bx=ctx->x+ctx->w;
    lvtx->by=line->y;
    lvtx->pixel=line->pixel;
  }
  fmn_draw_line(linev,lvtx-linev);

  // Notes.  
  if (ctx->notec) {
    struct fmn_draw_mintile tvtxv[FMN_VIOLIN_SONG_LENGTH];
    struct fmn_draw_mintile *tvtx=tvtxv;
    const struct fmn_violin_note *note=ctx->notev;
    for (i=ctx->notec;i-->0;note++) {
      if (!note->tileid) continue;
      tvtx->x=note->x;
      tvtx->y=note->y;
      tvtx->tileid=note->tileid;
      tvtx->xform=0;
      tvtx++;
    }
    fmn_draw_mintile(tvtxv,tvtx-tvtxv,2);
  }
}

/* Render violin chart, main entry point.
 */
 
void fmn_render_violin() {
  struct fmn_violin_context ctx={0};
  fmn_violin_begin(&ctx);
  fmn_render_violin_context(&ctx);
}
