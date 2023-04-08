#include "../fmn_gl2_internal.h"

/* Draw map.
 */
 
void fmn_gl2_game_freshen_map(struct bigpc_render_driver *driver) {
  struct fmn_gl2_vertex_mintile vtxv[FMN_COLC*FMN_ROWC*2];
  struct fmn_gl2_vertex_mintile *vtx=vtxv;
  int vtxc=0;
  const uint8_t *src=fmn_global.map;
  int y=DRIVER->game.tilesize>>1;
  int yi=FMN_ROWC;
  for (;yi-->0;y+=DRIVER->game.tilesize) {
    int x=DRIVER->game.tilesize>>1;
    int xi=FMN_COLC;
    for (;xi-->0;x+=DRIVER->game.tilesize,src++) {
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=0x00;
      vtx->xform=0;
      vtx++;
      vtxc++;
      if (*src) {
        vtx->x=x;
        vtx->y=y;
        vtx->tileid=*src;
        vtx->xform=0;
        vtx++;
        vtxc++;
      }
    }
  }
  fmn_gl2_framebuffer_use(driver,&DRIVER->game.mapbits);
  fmn_gl2_texture_use(driver,fmn_global.maptsid);
  fmn_gl2_program_use(driver,&DRIVER->program_mintile);
  fmn_gl2_draw_mintile(vtxv,vtxc);
}
