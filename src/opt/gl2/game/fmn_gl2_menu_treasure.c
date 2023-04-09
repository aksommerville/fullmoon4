#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"

// Timing in frames, fair to assume 60 Hz.
#define FMN_GL2_TREASURE_SUSPENSE_TIME  30
#define FMN_GL2_TREASURE_SLIDE_TIME     60
#define FMN_GL2_TREASURE_CHARM_PERIOD  200
#define FMN_GL2_TREASURE_WOBBLE_PERIOD  50
#define FMN_GL2_TREASURE_PULSE_PERIOD  150
#define FMN_GL2_TREASURE_CHARM_COUNT    12

/* Draw a rotating circle of charms.
 */
 
static void fmn_gl2_treasure_charms(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  if (fmn_gl2_texture_use(driver,4)<0) return;
  float midx=DRIVER->mainfb.texture.w*0.5f;
  float midy=DRIVER->mainfb.texture.h*0.5f+DRIVER->game.tilesize;
  float radius=48.0f+sinf(((menu->framec%FMN_GL2_TREASURE_PULSE_PERIOD)*M_PI*2.0f)/FMN_GL2_TREASURE_PULSE_PERIOD)*12.0f;
  float wobble=sinf(((menu->framec%FMN_GL2_TREASURE_WOBBLE_PERIOD)*M_PI*2.0f)/FMN_GL2_TREASURE_WOBBLE_PERIOD);
  uint8_t wobbleq=wobble*40.0f; // 128/pi but anywhere in the ballpark is fine
  float t=((menu->framec%FMN_GL2_TREASURE_CHARM_PERIOD)*M_PI*2.0f)/FMN_GL2_TREASURE_CHARM_PERIOD;
  float dt=(M_PI*2.0f)/FMN_GL2_TREASURE_CHARM_COUNT;
  const uint8_t tileidv[]={0x0d,0x0e,0x0f,0x1d,0x1e,0x1f};
  int tileidc=sizeof(tileidv);
  
  struct fmn_gl2_vertex_maxtile vtxv[FMN_GL2_TREASURE_CHARM_COUNT];
  struct fmn_gl2_vertex_maxtile *vtx=vtxv;
  int i=FMN_GL2_TREASURE_CHARM_COUNT;
  for (;i-->0;vtx++,t+=dt) {
    vtx->x=midx+sinf(t)*radius;
    vtx->y=midy-cosf(t)*radius;
    vtx->tileid=tileidv[i%tileidc];
    vtx->rotate=wobbleq;
    vtx->size=DRIVER->game.tilesize;
    vtx->xform=0;
    vtx->ta=0;
    vtx->pr=vtx->pg=vtx->pb=0x80;
    vtx->alpha=0xff;
  }
  
  fmn_gl2_program_use(driver,&DRIVER->program_maxtile);
  fmn_gl2_draw_maxtile(vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
}

/* Render treasure menu.
 */
 
void fmn_gl2_render_menu_treasure(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  if (menu->optionc<1) return;
  uint8_t itemid=menu->optionv[0].stringid;
  
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,0x000000ff);
  
  fmn_gl2_treasure_charms(driver,menu);
  
  // All the rest are decals from image 4.
  if (fmn_gl2_texture_use(driver,4)<0) return;
  fmn_gl2_program_use(driver,&DRIVER->program_decal);
  
  { // Big item picture.
    int16_t w=DRIVER->game.tilesize*3;
    int16_t srcx=DRIVER->game.tilesize*3+(itemid&3)*w;
    int16_t srcy=DRIVER->game.tilesize*2+(itemid>>2)*w;
    int16_t dstx=(DRIVER->mainfb.texture.w>>1)-(w>>1);
    int16_t dsty=(DRIVER->mainfb.texture.h>>1)-(w>>1)+DRIVER->game.tilesize; // +tilesize to account for upper bunting
    fmn_gl2_draw_decal(dstx,dsty,w,w,srcx,srcy,w,w);
  }
  
  { // Curtains.
    int16_t overlap=DRIVER->game.tilesize>>1;
    int16_t openrange=(DRIVER->mainfb.texture.w>>1)-DRIVER->game.tilesize;
    int16_t openness;
    if (menu->framec<FMN_GL2_TREASURE_SUSPENSE_TIME) openness=0;
    else if (menu->framec>=FMN_GL2_TREASURE_SUSPENSE_TIME+FMN_GL2_TREASURE_SLIDE_TIME) openness=openrange;
    else openness=((menu->framec-FMN_GL2_TREASURE_SUSPENSE_TIME)*openrange)/FMN_GL2_TREASURE_SLIDE_TIME;
    int16_t dstx=(DRIVER->mainfb.texture.w>>1)-DRIVER->game.tilesize-openness+overlap;
    fmn_gl2_draw_decal(
      dstx,0,DRIVER->game.tilesize,DRIVER->mainfb.texture.h,
      DRIVER->game.tilesize*2,0,DRIVER->game.tilesize,DRIVER->mainfb.texture.h
    );
    while (dstx>0) {
      dstx-=DRIVER->game.tilesize<<1;
      fmn_gl2_draw_decal(
        dstx,0,DRIVER->game.tilesize<<1,DRIVER->mainfb.texture.h,
        0,0,DRIVER->game.tilesize<<1,DRIVER->mainfb.texture.h
      );
    }
    dstx=(DRIVER->mainfb.texture.w>>1)+openness-overlap;
    fmn_gl2_draw_decal(
      dstx,0,DRIVER->game.tilesize,DRIVER->mainfb.texture.h,
      DRIVER->game.tilesize*3,0,-DRIVER->game.tilesize,DRIVER->mainfb.texture.h
    );
    dstx+=DRIVER->game.tilesize;
    while (dstx<DRIVER->mainfb.texture.w) {
      fmn_gl2_draw_decal(
        dstx,0,DRIVER->game.tilesize<<1,DRIVER->mainfb.texture.h,
        DRIVER->game.tilesize<<1,0,-(DRIVER->game.tilesize<<1),DRIVER->mainfb.texture.h
      );
      dstx+=DRIVER->game.tilesize<<1;
    }
  }
  
  { // Bunting.
    fmn_gl2_draw_decal(
      0,0,DRIVER->mainfb.texture.w>>1,DRIVER->game.tilesize<<1,
      DRIVER->game.tilesize*3,0,DRIVER->mainfb.texture.w>>1,DRIVER->game.tilesize<<1
    );
    fmn_gl2_draw_decal(
      DRIVER->mainfb.texture.w>>1,0,DRIVER->mainfb.texture.w>>1,DRIVER->game.tilesize<<1,
      DRIVER->game.tilesize*3+(DRIVER->mainfb.texture.w>>1),0,-(DRIVER->mainfb.texture.w>>1),DRIVER->game.tilesize<<1
    );
  }
}
