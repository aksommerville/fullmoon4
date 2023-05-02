#include "fmn_menu_internal.h"

/* Dismiss.
 */
 
static void pause_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Move selection.
 */
 
static void pause_move(struct fmn_menu *menu,int dx,int dy) {
  int x=fmn_global.selected_item&3;
  int y=fmn_global.selected_item>>2;
  x+=dx; x&=3;
  y+=dy; y&=3;
  fmn_global.selected_item=(y<<2)|x;
}

/* Update.
 */
 
static void _pause_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) { pause_dismiss(menu); return; }
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { pause_dismiss(menu); return; }
    if ((input&FMN_INPUT_LEFT)&&!(menu->pvinput&FMN_INPUT_LEFT)) pause_move(menu,-1,0);
    if ((input&FMN_INPUT_RIGHT)&&!(menu->pvinput&FMN_INPUT_RIGHT)) pause_move(menu,1,0);
    if ((input&FMN_INPUT_UP)&&!(menu->pvinput&FMN_INPUT_UP)) pause_move(menu,0,-1);
    if ((input&FMN_INPUT_DOWN)&&!(menu->pvinput&FMN_INPUT_DOWN)) pause_move(menu,0,1);
    menu->pvinput=input;
  }
}

/* Draw one count for a depletable item.
 * Caller must provide absolute (x,y) of the item icon's NW corner.
 */
 
static void pause_draw_count(struct fmn_menu *menu,int16_t x,int16_t y,uint8_t c) {
  int16_t tilesize=menu->fbw/FMN_COLC;
  const int16_t glyphw=3;
  const int16_t glyphh=5;
  const int16_t marginx=2;
  const int16_t marginy=1;
  int16_t glyphc=(c>=100)?3:(c>=10)?2:1;
  int16_t dstw=glyphw*glyphc+(glyphc-1)+(marginx<<1);
  int16_t dsth=glyphh+(marginy<<1);
  int16_t dstx=x+tilesize-dstw+2;
  int16_t dsty=y+tilesize-dsth+2;
  int16_t bgsrcx=c?30:37;
  int16_t bgsrcy=224;
  
  struct fmn_draw_decal vtxv[16];
  int vtxc=0;
  #define ADD_DECAL(dstx,dsty,srcx,srcy,w,h) { \
    if (vtxc<16) vtxv[vtxc++]=(struct fmn_draw_decal){dstx,dsty,w,h,srcx,srcy,w,h}; \
  }
  
  // The background image is 7x7, and 1 pixel along the edge should not be repeated.
  ADD_DECAL(dstx,dsty,bgsrcx,bgsrcy,1,dsth);
  ADD_DECAL(dstx+dstw-1,dsty,bgsrcx+6,bgsrcy,1,dsth);
  int xi=dstx+1;
  int bgremaining=dstw-2;
  while (bgremaining>0) {
    int cpw=bgremaining;
    if (cpw>5) cpw=5;
    ADD_DECAL(xi,dsty,bgsrcx+1,bgsrcy,cpw,dsth);
    xi+=cpw;
    bgremaining-=cpw;
  }
  
  int yi=dsty+marginy;
  xi=dstx+marginx;
  if (c>=100) { ADD_DECAL(xi,yi,(c/100)*glyphw,bgsrcy,glyphw,glyphh); xi+=glyphw+1; }
  if (c>=10) { ADD_DECAL(xi,yi,((c%100)/10)*glyphw,bgsrcy,glyphw,glyphh); xi+=glyphw+1; }
  ADD_DECAL(xi,yi,(c%10)*glyphw,bgsrcy,glyphw,glyphh);
  
  #undef ADD_DECAL
  fmn_draw_decal(vtxv,vtxc,2);
}

/* Render.
 */
 
static void _pause_render(struct fmn_menu *menu) {
  
  const int16_t marginx=4;
  const int16_t marginy=4;
  const int16_t spacingx=4;
  const int16_t spacingy=4;
  const int16_t colc=4;
  const int16_t rowc=4;
  int16_t ts=menu->fbw/FMN_COLC;
  int16_t halfts=ts>>1;
  int16_t w=(marginx<<1)+colc*ts+spacingx*(colc-1);
  int16_t h=(marginy<<1)+rowc*ts+spacingy*(rowc-1);
  int16_t x=(menu->fbw>>1)-(w>>1);
  int16_t y=(menu->fbh>>1)-(h>>1);
  
  // Solid black background.
  {
    struct fmn_draw_rect vtx={x,y,w,h,0x000000ff};
    fmn_draw_rect(&vtx,1);
  }
  
  struct fmn_draw_mintile vtxv[FMN_ITEM_COUNT+4+1]; // 4 for the indicator, 1 for the pitcher's content
  int vtxc=0;
  
  // Indicator takes four tiles.
  {
    int selx=fmn_global.selected_item&3;
    int sely=fmn_global.selected_item>>2;
    struct fmn_draw_mintile *vtx=vtxv+vtxc;
    vtxc+=4;
    vtx[0].x=x+marginx+selx*(ts+spacingx);
    vtx[0].y=y+marginy+sely*(ts+spacingy);
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
    struct fmn_draw_mintile *vtx=vtxv+vtxc++;
    vtx->x=x+marginx+col*(ts+spacingx)+halfts;
    vtx->y=y+marginy+row*(ts+spacingy)+halfts;
    vtx->tileid=0xf0+itemid;
    vtx->xform=0;
  }
  
  // Pitcher qualifier is also a mintile, and fits right on top of the base tile.
  if (fmn_global.itemv[FMN_ITEM_PITCHER]&&fmn_global.itemqv[FMN_ITEM_PITCHER]) {
    int16_t col=FMN_ITEM_PITCHER%colc;
    int16_t row=FMN_ITEM_PITCHER/colc;
    struct fmn_draw_mintile *vtx=vtxv+vtxc++;
    vtx->x=x+marginx+col*(ts+spacingx)+halfts;
    vtx->y=y+marginy+row*(ts+spacingy)+halfts;
    vtx->tileid=0xd1+fmn_global.itemqv[FMN_ITEM_PITCHER];
    vtx->xform=0;
  }
  
  fmn_draw_mintile(vtxv,vtxc,2);
  
  // Qualifiers for depletable items.
  #define QUALIFIER(tag) if (fmn_global.itemv[FMN_ITEM_##tag]) { \
    int16_t col=FMN_ITEM_##tag%colc; \
    int16_t row=FMN_ITEM_##tag/colc; \
    int16_t dstx=x+marginx+col*(ts+spacingx); \
    int16_t dsty=y+marginy+row*(ts+spacingy); \
    pause_draw_count(menu,dstx,dsty,fmn_global.itemqv[FMN_ITEM_##tag]); \
  }
  QUALIFIER(SEED)
  QUALIFIER(COIN)
  QUALIFIER(MATCH)
  QUALIFIER(CHEESE)
  #undef QUALIFIER
}

/* Init.
 */
 
void fmn_menu_init_PAUSE(struct fmn_menu *menu) {
  menu->update=_pause_update;
  menu->render=_pause_render;
}