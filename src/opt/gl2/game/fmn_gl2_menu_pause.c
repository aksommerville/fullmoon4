#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"

/* Draw one count for a depletable item.
 * Caller must bind tilesheet 2, and provide absolute (x,y) of the item icon's NW corner.
 */
 
static void fmn_gl2_pause_draw_count(struct bigpc_render_driver *driver,int16_t x,int16_t y,uint8_t c) {
  const int16_t glyphw=3;
  const int16_t glyphh=5;
  const int16_t marginx=2;
  const int16_t marginy=1;
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  int16_t glyphc=(c>=100)?3:(c>=10)?2:1;
  int16_t dstw=glyphw*glyphc+(glyphc-1)+(marginx<<1);
  int16_t dsth=glyphh+(marginy<<1);
  int16_t dstx=x+DRIVER->game.tilesize-dstw+2;
  int16_t dsty=y+DRIVER->game.tilesize-dsth+2;
  int16_t bgsrcx=c?30:37;
  int16_t bgsrcy=224;
  
  // The background image is 7x7, and 1 pixel along the edge should not be repeated.
  fmn_gl2_draw_decal(dstx,dsty,1,dsth,bgsrcx,bgsrcy,1,dsth);
  fmn_gl2_draw_decal(dstx+dstw-1,dsty,1,dsth,bgsrcx+6,bgsrcy,1,dsth);
  int xi=dstx+1;
  int bgremaining=dstw-2;
  while (bgremaining>0) {
    int cpw=bgremaining;
    if (cpw>5) cpw=5;
    fmn_gl2_draw_decal(xi,dsty,cpw,dsth,bgsrcx+1,bgsrcy,cpw,dsth);
    xi+=cpw;
    bgremaining-=cpw;
  }
  
  int yi=dsty+marginy;
  xi=dstx+marginx;
  if (c>=100) { fmn_gl2_draw_decal(xi,yi,glyphw,glyphh,(c/100)*glyphw,bgsrcy,glyphw,glyphh); xi+=glyphw+1; }
  if (c>=10) { fmn_gl2_draw_decal(xi,yi,glyphw,glyphh,((c%100)/10)*glyphw,bgsrcy,glyphw,glyphh); xi+=glyphw+1; }
  fmn_gl2_draw_decal(xi,yi,glyphw,glyphh,(c%10)*glyphw,bgsrcy,glyphw,glyphh);
}

/* Render pause menu.
 */
 
void fmn_gl2_render_menu_pause(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  const int16_t marginx=4;
  const int16_t marginy=4;
  const int16_t spacingx=4;
  const int16_t spacingy=4;
  const int16_t colc=4;
  const int16_t rowc=4;
  int16_t ts=DRIVER->game.tilesize;
  int16_t halfts=DRIVER->game.tilesize>>1;
  int16_t w=(marginx<<1)+colc*ts+spacingx*(colc-1);
  int16_t h=(marginy<<1)+rowc*ts+spacingy*(rowc-1);
  int16_t x=(DRIVER->mainfb.texture.w>>1)-(w>>1);
  int16_t y=(DRIVER->mainfb.texture.h>>1)-(h>>1);
  
  // Solid black background.
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(x,y,w,h,0x000000ff);
  
  struct fmn_gl2_vertex_mintile vtxv[FMN_ITEM_COUNT+4+1]; // 4 for the indicator, 1 for the pitcher's content
  int vtxc=0;
  
  // Indicator takes four tiles.
  {
    struct fmn_gl2_vertex_mintile *vtx=vtxv+vtxc;
    vtxc+=4;
    vtx[0].x=x+marginx+menu->selx*(ts+spacingx);
    vtx[0].y=y+marginy+menu->sely*(ts+spacingy);
    vtx[0].tileid=0xc0;
    vtx[0].xform=0;
    vtx[1]=vtx[0];
    vtx[1].x+=ts;
    vtx[1].tileid+=0x01;
    vtx[2]=vtx[0];
    vtx[2].y+=ts;
    vtx[2].tileid+=0x10;
    vtx[3]=vtx[0];
    vtx[3].x+=ts;
    vtx[3].y+=ts;
    vtx[3].tileid+=0x11;
  }
  
  // One tile for each item. Qualifiers are not drawn in this pass.
  int itemid=0;
  for (;itemid<FMN_ITEM_COUNT;itemid++) {
    if (!(fmn_global.itemv[itemid])) continue;
    int16_t col=itemid%colc;
    int16_t row=itemid/colc;
    struct fmn_gl2_vertex_mintile *vtx=vtxv+vtxc++;
    vtx->x=x+marginx+col*(ts+spacingx)+halfts;
    vtx->y=y+marginy+row*(ts+spacingy)+halfts;
    vtx->tileid=0xf0+itemid;
    vtx->xform=0;
  }
  
  // Pitcher qualifier is also a mintile, and fits right on top of the base tile.
  if (fmn_global.itemv[FMN_ITEM_PITCHER]&&fmn_global.itemqv[FMN_ITEM_PITCHER]) {
    int16_t col=FMN_ITEM_PITCHER%colc;
    int16_t row=FMN_ITEM_PITCHER/colc;
    struct fmn_gl2_vertex_mintile *vtx=vtxv+vtxc++;
    vtx->x=x+marginx+col*(ts+spacingx)+halfts;
    vtx->y=y+marginy+row*(ts+spacingy)+halfts;
    vtx->tileid=0xd1+fmn_global.itemqv[FMN_ITEM_PITCHER];
    vtx->xform=0;
  }
  
  fmn_gl2_texture_use(driver,2);
  fmn_gl2_program_use(driver,&DRIVER->program_mintile);
  fmn_gl2_draw_mintile(vtxv,vtxc);
  
  // Qualifiers for depletable items.
  #define QUALIFIER(tag) if (fmn_global.itemv[FMN_ITEM_##tag]) { \
    int16_t col=FMN_ITEM_##tag%colc; \
    int16_t row=FMN_ITEM_##tag/colc; \
    int16_t dstx=x+marginx+col*(ts+spacingx); \
    int16_t dsty=y+marginy+row*(ts+spacingy); \
    fmn_gl2_pause_draw_count(driver,dstx,dsty,fmn_global.itemqv[FMN_ITEM_##tag]); \
  }
  QUALIFIER(SEED)
  QUALIFIER(COIN)
  QUALIFIER(MATCH)
  QUALIFIER(CHEESE)
  #undef QUALIFIER
}
