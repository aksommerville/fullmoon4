#include "../fmn_gl2_internal.h"

/* Plants.
 */
 
static void fmn_gl2_map_draw_plants(struct bigpc_render_driver *driver) {
  struct fmn_gl2_vertex_mintile vtxv[FMN_PLANT_LIMIT*2];
  int vtxc=0;
  const struct fmn_plant *plant=fmn_global.plantv;
  int i=fmn_global.plantc;
  for (;i-->0;plant++) {
    if (plant->state==FMN_PLANT_STATE_NONE) continue;
    struct fmn_gl2_vertex_mintile *vtx=vtxv+vtxc++;
    vtx->x=plant->x*DRIVER->game.tilesize+(DRIVER->game.tilesize>>1);
    vtx->y=plant->y*DRIVER->game.tilesize+(DRIVER->game.tilesize>>1);
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
  if (vtxc&&(fmn_gl2_texture_use(driver,2)>=0)) {
    fmn_gl2_program_use(driver,&DRIVER->program_mintile);
    fmn_gl2_draw_mintile(vtxv,vtxc);
  }
}

/* Sketches.
 */
 
uint32_t fmn_gl2_chalk_points_from_bit(uint32_t bit) {
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

uint32_t fmn_gl2_chalk_bit_from_points(uint32_t points) {
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
 
static void fmn_gl2_map_draw_sketches(struct bigpc_render_driver *driver) {
  int margin=3;
  int spacing=4;
  struct fmn_gl2_vertex_raw vtxv[100];
  int vtxc=0;
  const struct fmn_sketch *sketch=fmn_global.sketchv;
  int i=fmn_global.sketchc;
  for (;i-->0;sketch++) {
    if (!sketch->bits) continue;
    if ((sketch->x<0)||(sketch->x>=FMN_COLC)) continue;
    if ((sketch->y<0)||(sketch->y>=FMN_ROWC)) continue;
    uint32_t mask=0x00080000;
    for (;mask;mask>>=1/*,tileid--*/) {
      if (!(sketch->bits&mask)) continue;
      uint32_t points=fmn_gl2_chalk_points_from_bit(mask);
      if (!points) continue;
      if (vtxc>=100) {
        fmn_gl2_program_use(driver,&DRIVER->program_raw);
        fmn_gl2_draw_raw(GL_LINES,vtxv,vtxc);
        vtxc=0;
      }
      struct fmn_gl2_vertex_raw *vtx=vtxv+vtxc;
      vtx[0].x=sketch->x*DRIVER->game.tilesize+margin+spacing*((points>>12)&15);
      vtx[0].y=sketch->y*DRIVER->game.tilesize+margin+spacing*((points>> 8)&15);
      vtx[0].r=vtx[0].g=vtx[0].b=vtx[0].a=0xff;
      vtx[1].x=sketch->x*DRIVER->game.tilesize+margin+spacing*((points>> 4)&15);
      vtx[1].y=sketch->y*DRIVER->game.tilesize+margin+spacing*((points    )&15);
      vtx[1].r=vtx[1].g=vtx[1].b=vtx[1].a=0xff;
      vtxc+=2;
    }
  }
  if (vtxc) {
    fmn_gl2_program_use(driver,&DRIVER->program_raw);
    fmn_gl2_draw_raw(GL_LINES,vtxv,vtxc);
  }
}

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
  fmn_gl2_map_draw_plants(driver);
  fmn_gl2_map_draw_sketches(driver);
}
