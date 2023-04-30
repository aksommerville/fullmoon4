#include "fmn_render_internal.h"

/* Freshen map.
 */
 
void fmn_render_freshen_map() {
  if (fmn_draw_set_output(FMN_IMAGEID_MAPBITS)<0) return;
  
  // One or two mintiles per cell.
  //TODO It's stupid, treating these as transparent. Just draw each cell with its full content.
  {
    struct fmn_draw_mintile vtxv[FMN_COLC*FMN_ROWC*2];
    int vtxc=0;
    const uint8_t *src=fmn_global.map;
    int yi=FMN_ROWC;
    int16_t dsty=fmn_render_global.tilesize>>1;
    for (;yi-->0;dsty+=fmn_render_global.tilesize) {
      int xi=FMN_COLC;
      int dstx=fmn_render_global.tilesize>>1;
      for (;xi-->0;src++,dstx+=fmn_render_global.tilesize) {
        vtxv[vtxc].x=dstx;
        vtxv[vtxc].y=dsty;
        vtxv[vtxc].xform=0;
        vtxv[vtxc].tileid=0;
        vtxc++;
        if (*src) {
          vtxv[vtxc].x=dstx;
          vtxv[vtxc].y=dsty;
          vtxv[vtxc].xform=0;
          vtxv[vtxc].tileid=*src;
          vtxc++;
        }
      }
    }
    fmn_draw_mintile(vtxv,vtxc,fmn_global.maptsid);
  }
  
  //TODO plants
  //TODO sketches
}
