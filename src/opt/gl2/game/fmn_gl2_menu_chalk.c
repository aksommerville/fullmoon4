int fmn_gl2_menu_chlk_dummy=0;
#if 0 /*XXX move to client*/
#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"

/* Render chalk menu.
 */
 
void fmn_gl2_render_menu_chalk(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  int margin=DRIVER->game.tilesize;
  int spacing=DRIVER->game.tilesize*3;
  int bw=spacing*2+margin*2;
  int bh=bw;
  int bx=(DRIVER->mainfb.w>>1)-(bw>>1);
  int by=(DRIVER->mainfb.h>>1)-(bh>>1);
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(bx,by,bw,bh,0x000000ff);
  
  // Lines.
  struct fmn_gl2_vertex_raw vtxv[42]; // 20 possible lines, 2 vertices apiece, plus 2 for the focus line.
  int vtxc=0;
  uint32_t bits=menu->extra[0];
  if (bits) {
    uint32_t mask=0x80000;
    for (;mask;mask>>=1) {
      if (!(bits&mask)) continue;
      uint32_t points=fmn_gl2_chalk_points_from_bit(mask);
      if (!points) continue;
      struct fmn_gl2_vertex_raw *vtx=vtxv+vtxc;
      vtx[0].x=bx+margin+spacing*((points>>12)&15);
      vtx[0].y=by+margin+spacing*((points>> 8)&15);
      vtx[0].r=vtx[0].g=vtx[0].b=vtx[0].a=0xff;
      vtx[1].x=bx+margin+spacing*((points>> 4)&15);
      vtx[1].y=by+margin+spacing*((points    )&15);
      vtx[1].r=vtx[1].g=vtx[1].b=vtx[1].a=0xff;
      vtxc+=2;
    }
  }
  if (menu->extra[1]) {
    uint32_t points=fmn_gl2_chalk_points_from_bit(menu->extra[1]);
    if (points) {
      struct fmn_gl2_vertex_raw *vtx=vtxv+vtxc;
      vtx[0].x=bx+margin+spacing*((points>>12)&15);
      vtx[0].y=by+margin+spacing*((points>> 8)&15);
      vtx[0].r=vtx[0].g=vtx[0].a=0xff; vtx[0].b=0x00;
      vtx[1].x=bx+margin+spacing*((points>> 4)&15);
      vtx[1].y=by+margin+spacing*((points    )&15);
      vtx[1].r=vtx[1].g=vtx[1].a=0xff; vtx[0].b=0x00;
      vtxc+=2;
    }
  }
  if (vtxc) fmn_gl2_draw_raw(GL_LINES,vtxv,vtxc);
  
  // Anchor points and hand.
  if (fmn_gl2_texture_use_imageid(driver,2)>=0) {
    struct fmn_gl2_vertex_mintile mvtxv[10];
    struct fmn_gl2_vertex_mintile *mvtx=mvtxv;
    int col=0; for (;col<3;col++) {
      int row=0; for (;row<3;row++) {
        mvtx->x=bx+margin+spacing*col;
        mvtx->y=by+margin+spacing*row;
        mvtx->tileid=0xc2;
        mvtx->xform=0;
        mvtx++;
      }
    }
    mvtx->x=bx+margin+spacing*menu->selx+(DRIVER->game.tilesize>>1);
    mvtx->y=by+margin+spacing*menu->sely+(DRIVER->game.tilesize>>1);
    mvtx->tileid=0xc3;
    mvtx->xform=0;
    fmn_gl2_program_use(driver,&DRIVER->program_mintile);
    fmn_gl2_draw_mintile(mvtxv,10);
  }
}
#endif
