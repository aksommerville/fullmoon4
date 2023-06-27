#include "fmn_menu_internal.h"
#include <math.h>

#define itemid menu->argv[0]
#define framec menu->argv[1]
#define bgcolor menu->argv[2]
#define clock menu->fv[0]

#define TREASURE_MIN_UPTIME 0.5f

// Timing in frames, fair to assume 60 Hz.
#define TREASURE_SUSPENSE_TIME  30
#define TREASURE_SLIDE_TIME     60
#define TREASURE_CHARM_PERIOD  200
#define TREASURE_WOBBLE_PERIOD  50
#define TREASURE_PULSE_PERIOD  150
#define TREASURE_CHARM_COUNT    12

/* Dismiss.
 */
 
static void treasure_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Update.
 */
 
static void _treasure_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  clock+=elapsed;
  if (input!=menu->pvinput) {
    if (clock>=TREASURE_MIN_UPTIME) {
      if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) { treasure_dismiss(menu); return; }
      if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { treasure_dismiss(menu); return; }
    }
    menu->pvinput=input;
  }
}

/* Charms.
 */
 
static void treasure_render_charms(struct fmn_menu *menu) {
  int tilesize=menu->fbw/FMN_COLC;
  float midx=menu->fbw*0.5f;
  float midy=menu->fbh*0.5f+tilesize;
  float radius=48.0f+sinf(((framec%TREASURE_PULSE_PERIOD)*M_PI*2.0f)/TREASURE_PULSE_PERIOD)*12.0f;
  float wobble=sinf(((framec%TREASURE_WOBBLE_PERIOD)*M_PI*2.0f)/TREASURE_WOBBLE_PERIOD);
  int8_t wobbleq=wobble*40.0f; // 128/pi but anywhere in the ballpark is fine
  float t=((framec%TREASURE_CHARM_PERIOD)*M_PI*2.0f)/TREASURE_CHARM_PERIOD;
  float dt=(M_PI*2.0f)/TREASURE_CHARM_COUNT;
  const uint8_t tileidv[]={0x0d,0x0e,0x0f,0x1d,0x1e,0x1f};
  int tileidc=sizeof(tileidv);
  
  struct fmn_draw_maxtile vtxv[TREASURE_CHARM_COUNT];
  struct fmn_draw_maxtile *vtx=vtxv;
  int i=TREASURE_CHARM_COUNT;
  for (;i-->0;vtx++,t+=dt) {
    vtx->x=midx+sinf(t)*radius;
    vtx->y=midy-cosf(t)*radius;
    vtx->tileid=tileidv[i%tileidc];
    vtx->rotate=wobbleq;
    vtx->size=tilesize;
    vtx->xform=0;
    vtx->ta=0;
    vtx->pr=vtx->pg=vtx->pb=0x80;
    vtx->alpha=0xff;
  }
  
  fmn_draw_maxtile(vtxv,sizeof(vtxv)/sizeof(vtxv[0]),4);
}

/* Render.
 */
 
static void _treasure_render(struct fmn_menu *menu) {
  framec++;
  int tilesize=menu->fbw/FMN_COLC;
  
  // Blackout.
  {
    struct fmn_draw_rect vtx={0,0,menu->fbw,menu->fbh,bgcolor};
    fmn_draw_rect(&vtx,1);
  }
  
  treasure_render_charms(menu);
  
  // Everything else is a decal from image 4. Batch the calls.
  struct fmn_draw_decal vtxv[32];
  int vtxc=0;
  #define ADD_DECAL(dstx,dsty,srcx,srcy,w,h) { \
    if (vtxc<32) vtxv[vtxc++]=(struct fmn_draw_decal){dstx,dsty,w,h,srcx,srcy,w,h}; \
    else fmn_log("!!! too many decals in treasure menu"); \
  }
  #define REV_DECAL(dstx,dsty,srcx,srcy,w,h) { \
    if (vtxc<32) vtxv[vtxc++]=(struct fmn_draw_decal){dstx,dsty,w,h,(srcx)+(w),srcy,-(w),h}; \
    else fmn_log("!!! too many decals in treasure menu"); \
  }
  
  { // Big item picture.
    int16_t w=tilesize*3;
    int16_t srcx=tilesize*3+(itemid&3)*w;
    int16_t srcy=tilesize*2+(itemid>>2)*w;
    int16_t dstx=(menu->fbw>>1)-(w>>1);
    int16_t dsty=(menu->fbh>>1)-(w>>1)+tilesize; // +tilesize to account for upper bunting
    ADD_DECAL(dstx,dsty,srcx,srcy,w,w)
  }
  
  { // Curtains.
    int16_t overlap=tilesize>>1;
    int16_t openrange=(menu->fbw>>1)-tilesize;
    int16_t openness;
    if (framec<TREASURE_SUSPENSE_TIME) openness=0;
    else if (framec>=TREASURE_SUSPENSE_TIME+TREASURE_SLIDE_TIME) openness=openrange;
    else openness=((framec-TREASURE_SUSPENSE_TIME)*openrange)/TREASURE_SLIDE_TIME;
    int16_t dstx=(menu->fbw>>1)-tilesize-openness+overlap;
    ADD_DECAL(dstx,0,tilesize*2,0,tilesize,menu->fbh)
    while (dstx>0) {
      dstx-=tilesize<<1;
      ADD_DECAL(dstx,0,0,0,tilesize<<1,menu->fbh)
    }
    dstx=(menu->fbw>>1)+openness-overlap;
    REV_DECAL(dstx,0,tilesize*2,0,tilesize,menu->fbh)
    dstx+=tilesize;
    while (dstx<menu->fbw) {
      REV_DECAL(dstx,0,0,0,tilesize<<1,menu->fbh)
      dstx+=tilesize<<1;
    }
  }
  
  { // Bunting.
    ADD_DECAL(0,0,tilesize*3,0,menu->fbw>>1,tilesize<<1)
    REV_DECAL(menu->fbw>>1,0,tilesize*3,0,menu->fbw>>1,tilesize<<1)
  }
  
  #undef ADD_DECAL
  #undef REV_DECAL
  fmn_draw_decal(vtxv,vtxc,4);
}

/* Init.
 */
 
void fmn_menu_init_TREASURE(struct fmn_menu *menu) {
  menu->update=_treasure_update;
  menu->render=_treasure_render;
  menu->opaque=1;
  bgcolor=fmn_video_pixel_from_rgba(0x000000ff);
}
