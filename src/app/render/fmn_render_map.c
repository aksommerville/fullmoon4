#include "fmn_render_internal.h"
#include "app/fmn_game.h"

/* Freshen map.
 */
 
void fmn_render_freshen_map() {
  if (fmn_draw_set_output(FMN_IMAGEID_MAPBITS)<0) return;
  const int16_t ts=fmn_render_global.tilesize;
  const int16_t halfts=ts>>1;
  
  // One or two mintiles per cell.
  //TODO It's stupid, treating these as transparent. Just draw each cell with its full content.
  {
    struct fmn_draw_mintile vtxv[FMN_COLC*FMN_ROWC*2];
    int vtxc=0;
    const uint8_t *src=fmn_global.map;
    int yi=FMN_ROWC;
    int16_t dsty=halfts;
    for (;yi-->0;dsty+=ts) {
      int xi=FMN_COLC;
      int dstx=halfts;
      for (;xi-->0;src++,dstx+=ts) {
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
  
  // Plants.
  {
    struct fmn_draw_mintile vtxv[FMN_PLANT_LIMIT*2];
    int vtxc=0;
    const struct fmn_plant *plant=fmn_global.plantv;
    int i=fmn_global.plantc;
    for (;i-->0;plant++) {
      if (plant->state==FMN_PLANT_STATE_NONE) continue;
      struct fmn_draw_mintile *vtx=vtxv+vtxc++;
      vtx->x=plant->x*ts+halfts;
      vtx->y=plant->y*ts+halfts;
      vtx->tileid=0x3a+0x10*plant->state;
      vtx->xform=0;
      if ((plant->state==FMN_PLANT_STATE_FLOWER)&&plant->fruit) {
        vtxc++;
        vtx[1].x=vtx[0].x;
        vtx[1].y=vtx[0].y;
        vtx[1].tileid=0x2b+0x10*plant->fruit;
        vtx[1].xform=0;
      }
    }
    if (vtxc) fmn_draw_mintile(vtxv,vtxc,2);
  }
  
  // Sketches.
  {
    const int16_t margin=3;
    const int16_t spacing=4;
    struct fmn_draw_line vtxv[100];
    int vtxc=0;
    const struct fmn_sketch *sketch=fmn_global.sketchv;
    int i=fmn_global.sketchc;
    for (;i-->0;sketch++) {
      if (!sketch->bits) continue;
      if ((sketch->x<0)||(sketch->x>=FMN_COLC)) continue;
      if ((sketch->y<0)||(sketch->y>=FMN_ROWC)) continue;
      uint32_t mask=0x00080000;
      for (;mask;mask>>=1) {
        if (!(sketch->bits&mask)) continue;
        uint32_t points=fmn_chalk_points_from_bit(mask);
        if (!points) continue;
        if (vtxc>=100) {
          fmn_draw_line(vtxv,vtxc);
          vtxc=0;
        }
        struct fmn_draw_line *vtx=vtxv+vtxc;
        vtx->ax=sketch->x*ts+margin+spacing*((points>>12)&15);
        vtx->ay=sketch->y*ts+margin+spacing*((points>> 8)&15);
        vtx->bx=sketch->x*ts+margin+spacing*((points>> 4)&15);
        vtx->by=sketch->y*ts+margin+spacing*((points    )&15);
        vtx->pixel=0xffffffff;
        vtxc++;
      }
    }
    if (vtxc) {
      fmn_draw_line(vtxv,vtxc);
    }
  }
  
  fmn_draw_set_output(0);
}
