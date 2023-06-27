#include "fmn_menu_internal.h"
#include "app/fmn_game.h"

#define bitpic menu->argv[0]
#define handx menu->argv[1]
#define handy menu->argv[2]
#define anchorx menu->argv[3] /* <0 if not dragging */
#define anchory menu->argv[4]
#define line_color menu->argv[5]
#define highlight_color menu->argv[6]
#define bgcolor menu->argv[7]

/* Dismiss.
 */
 
static void chalk_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Move selection.
 */
 
static void chalk_move(struct fmn_menu *menu,int dx,int dy) {
  handx+=dx;
  handy+=dy;
  if ((handx==3)&&(handy==0)) {
    // "clear all"
  } else {
    if (handx<0) handx=handy?2:3; else if (handx>2) handx=0;
    if (handy<0) handy=2; else if (handy>2) handy=0;
  }
  if ((anchorx==0)&&(handx==2)) handx=1;
  if ((anchorx==2)&&(handx==0)) handx=1;
  if ((anchory==0)&&(handy==2)) handy=1;
  if ((anchory==2)&&(handy==0)) handy=1;
}

/* Press or release.
 */
 
static void chalk_press(struct fmn_menu *menu) {
  if (handx==3) {
    bitpic=0;
    if (menu->cb) menu->cb(menu,FMN_MENU_MESSAGE_CHANGED);
  } else {
    anchorx=handx;
    anchory=handy;
  }
}

static void chalk_release(struct fmn_menu *menu) {
  if (handx<3) {
    if ((anchorx!=handx)||(anchory!=handy)) {
      uint32_t bit=fmn_chalk_bit_from_points((anchorx<<12)|(anchory<<8)|(handx<<4)|handy);
      if (bit) {
        bitpic^=bit;
        if (menu->cb) menu->cb(menu,FMN_MENU_MESSAGE_CHANGED);
      }
    }
  }
  anchorx=anchory=-1;
}

/* Update.
 */
 
static void _chalk_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) chalk_press(menu);
    else if (!(input&FMN_INPUT_USE)&&(menu->pvinput&FMN_INPUT_USE)) chalk_release(menu);
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { chalk_dismiss(menu); return; }
    if ((input&FMN_INPUT_LEFT)&&!(menu->pvinput&FMN_INPUT_LEFT)) chalk_move(menu,-1,0);
    if ((input&FMN_INPUT_RIGHT)&&!(menu->pvinput&FMN_INPUT_RIGHT)) chalk_move(menu,1,0);
    if ((input&FMN_INPUT_UP)&&!(menu->pvinput&FMN_INPUT_UP)) chalk_move(menu,0,-1);
    if ((input&FMN_INPUT_DOWN)&&!(menu->pvinput&FMN_INPUT_DOWN)) chalk_move(menu,0,1);
    menu->pvinput=input;
  }
}

/* Render.
 */
 
static void _chalk_render(struct fmn_menu *menu) {
  int16_t tilesize=menu->fbw/FMN_COLC;
  int16_t margin=tilesize;
  int16_t spacing=tilesize*3;
  int16_t bw=spacing*2+margin*2;
  int16_t bh=bw;
  int16_t bx=(menu->fbw>>1)-(bw>>1);
  int16_t by=(menu->fbh>>1)-(bh>>1);
  int16_t clearw=tilesize<<1;
  int16_t clearh=tilesize<<1;
  int16_t clearx=bx+bw+1;
  int16_t cleary=by;
  
  { // Black box.
    struct fmn_draw_rect vtxv[]={
      {bx,by,bw,bh,bgcolor},
      {clearx,cleary,clearw,clearh,bgcolor},
    };
    fmn_draw_rect(vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  // Lines.
  struct fmn_draw_line vtxv[21]; // 20 possible lines, plus the focus line.
  int vtxc=0;
  uint32_t bits=bitpic;
  if (bits) {
    uint32_t mask=0x80000;
    for (;mask;mask>>=1) {
      if (!(bits&mask)) continue;
      uint32_t points=fmn_chalk_points_from_bit(mask);
      if (!points) continue;
      struct fmn_draw_line *vtx=vtxv+vtxc;
      vtx->ax=bx+margin+spacing*((points>>12)&15);
      vtx->ay=by+margin+spacing*((points>> 8)&15);
      vtx->bx=bx+margin+spacing*((points>> 4)&15);
      vtx->by=by+margin+spacing*((points    )&15);
      vtx->pixel=line_color;
      vtxc++;
    }
  }
  if (anchorx>=0) {
    struct fmn_draw_line *vtx=vtxv+vtxc;
    vtx->ax=bx+margin+spacing*anchorx;
    vtx->ay=by+margin+spacing*anchory;
    vtx->bx=bx+margin+spacing*handx;
    vtx->by=by+margin+spacing*handy;
    vtx->pixel=highlight_color;
    vtxc++;
  }
  if (vtxc) fmn_draw_line(vtxv,vtxc);
  
  // Anchor points and hand.
  {
    struct fmn_draw_mintile mvtxv[10];
    struct fmn_draw_mintile *mvtx=mvtxv;
    int col=0; for (;col<3;col++) {
      int row=0; for (;row<3;row++) {
        mvtx->x=bx+margin+spacing*col;
        mvtx->y=by+margin+spacing*row;
        if ((col==anchorx)&&(row==anchory)) mvtx->tileid=0xb4;
        else if ((anchorx>=0)&&((col==handx)&&(row==handy))) mvtx->tileid=0xb4;
        else mvtx->tileid=0xc2;
        mvtx->xform=0;
        mvtx++;
      }
    }
    if (handx<3) {
      mvtx->x=bx+margin+spacing*handx+(tilesize>>1);
      mvtx->y=by+margin+spacing*handy+(tilesize>>1);
    } else {
      mvtx->x=-100;
      mvtx->y=-100;
    }
    mvtx->tileid=0xc3;
    mvtx->xform=0;
    fmn_draw_mintile(mvtxv,10,2);
  }
  
  { // Clear-all button.
    uint8_t tileid=0xb5;
    if (handx==3) {
      if (bitpic) tileid=0xb6;
      else tileid=0xb7;
    }
    struct fmn_draw_mintile vtxv[4]={
      {clearx+(tilesize>>1),cleary+(tilesize>>1),tileid,0},
      {0,tilesize,tileid,FMN_XFORM_YREV},
      {tilesize,0,tileid,FMN_XFORM_XREV},
      {tilesize,tilesize,tileid,FMN_XFORM_XREV|FMN_XFORM_YREV},
    };
    vtxv[1].x+=vtxv[0].x;
    vtxv[1].y+=vtxv[0].y;
    vtxv[2].x+=vtxv[0].x;
    vtxv[2].y+=vtxv[0].y;
    vtxv[3].x+=vtxv[0].x;
    vtxv[3].y+=vtxv[0].y;
    fmn_draw_mintile(vtxv,sizeof(vtxv)/sizeof(vtxv[0]),2);
  }
}

/* Init.
 */
 
void fmn_menu_init_CHALK(struct fmn_menu *menu) {
  menu->update=_chalk_update;
  menu->render=_chalk_render;
  handx=handy=1;
  anchorx=anchory=-1;
  line_color=fmn_video_pixel_from_rgba(0xffffffff);
  highlight_color=fmn_video_pixel_from_rgba(0xffff00ff);
  bgcolor=fmn_video_pixel_from_rgba(0x000000ff);
}

/* Some shared chalk-drawing stuff.
 */
 
 
uint32_t fmn_chalk_points_from_bit(uint32_t bit) {
  switch (bit) {
    case 0x00001: return 0x1222;
    case 0x00002: return 0x0212;
    case 0x00004: return 0x2122;
    case 0x00008: return 0x1221;
    case 0x00010: return 0x1122;
    case 0x00020: return 0x1112;
    case 0x00040: return 0x0211;
    case 0x00080: return 0x1121;
    case 0x00100: return 0x0112;
    case 0x00200: return 0x0102;
    case 0x00400: return 0x0111;
    case 0x00800: return 0x2021;
    case 0x01000: return 0x1120;
    case 0x02000: return 0x1021;
    case 0x04000: return 0x1011;
    case 0x08000: return 0x0110;
    case 0x10000: return 0x1020;
    case 0x20000: return 0x0011;
    case 0x40000: return 0x0001;
    case 0x80000: return 0x0010;
  }
  return 0;
}

uint32_t fmn_chalk_bit_from_points(uint32_t points) {
  uint8_t a=points>>8,b=points;
  if (a>b) points=(b<<8)|a;
  switch (points) {
    case 0x1222: return 0x00001;
    case 0x0212: return 0x00002;
    case 0x2122: return 0x00004;
    case 0x1221: return 0x00008;
    case 0x1122: return 0x00010;
    case 0x1112: return 0x00020;
    case 0x0211: return 0x00040;
    case 0x1121: return 0x00080;
    case 0x0112: return 0x00100;
    case 0x0102: return 0x00200;
    case 0x0111: return 0x00400;
    case 0x2021: return 0x00800;
    case 0x1120: return 0x01000;
    case 0x1021: return 0x02000;
    case 0x1011: return 0x04000;
    case 0x0110: return 0x08000;
    case 0x1020: return 0x10000;
    case 0x0011: return 0x20000;
    case 0x0001: return 0x40000;
    case 0x0010: return 0x80000;
  }
  return 0;
}
