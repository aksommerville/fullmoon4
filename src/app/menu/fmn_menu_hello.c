#include "fmn_menu_internal.h"

#define titlecolor menu->argv[1]
#define framec menu->argv[2]
#define bgcolor menu->argv[3]
#define clock menu->fv[0]

/* Dismiss.
 */
 
static void hello_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
    if (fmn_game_load_map(1)<0) fmn_abort();
  }
}

/* Update.
 */
 
static void _hello_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  clock+=elapsed;
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) { hello_dismiss(menu); return; }
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { hello_dismiss(menu); return; }
    menu->pvinput=input;
  }
}

/* Stars.
 */
 
#define HELLO_STARS_VTXC 200

static struct fmn_draw_line hello_stars_vtxv[HELLO_STARS_VTXC];
 
static void hello_stars_render(struct fmn_menu *menu) {

  // A very simple PRNG with a fixed initial state. Easier than storing all the positions and colors.
  const uint32_t a=71;
  const uint32_t c=11;
  const uint32_t m=999331;
  uint32_t X=0x55555555;
  
  int xp=0,yp=0;
  struct fmn_draw_line *vtx=hello_stars_vtxv;
  int i=HELLO_STARS_VTXC;
  for (;i-->0;vtx++) {
  
    vtx->ax=X%menu->fbw;
    X=(a*X+c)%m;
    vtx->ay=X%menu->fbh;
    X=(a*X+c)%m;
    vtx->bx=vtx->ax+1;
    vtx->by=vtx->ay;
    
    uint8_t luma=0x40;
    uint8_t r=luma+X%31-16; X=(a*X+c)%m;
    uint8_t g=luma+X%31-16; X=(a*X+c)%m;
    uint8_t b=luma+X%31-16; X=(a*X+c)%m;
    uint8_t a=X+framec; if (a>=0x80) a=0xff-a;
    a+=0x40;
    vtx->pixel=fmn_video_pixel_from_rgba((r<<24)|(g<<16)|(b<<8)|a);
    
  }
  fmn_draw_line(hello_stars_vtxv,HELLO_STARS_VTXC);
}

/* Title tint color.
 */
 
static struct hello_title_color {
  float time;
  uint8_t r,g,b,a;
} hello_title_colorv[]={
  // Times are absolute, in 0..1. Last entry must be 1.0.
  {   0.000f,0x00,0x00,0x00,0xff},
  {   0.050f,0x00,0x00,0x00,0xff},
  {   0.100f,0x00,0x00,0x00,0x00}, // fade in
  {   0.350f,0x00,0x00,0x00,0x00},
  {   0.400f,0x00,0xff,0x00,0xff}, // to green
  {   0.450f,0x00,0xff,0x00,0x00}, // to white
  {   0.650f,0x00,0x00,0x00,0x00},
  {   0.700f,0xff,0x00,0x00,0xff}, // to red
  {   0.850f,0xff,0x00,0x00,0x00}, // to white
  {   0.900f,0x00,0x00,0x00,0x00},
  {   0.950f,0x00,0x00,0x00,0xff}, // fade out
  {   1.000f,0x00,0x00,0x00,0xff},
};
static const float hello_title_color_duration=20.0f;
 
static uint32_t hello_title_get_color(float time) {
  float phase,dummy;
  phase=modff(time/hello_title_color_duration,&dummy);
  int i=sizeof(hello_title_colorv)/sizeof(struct hello_title_color);
  const struct hello_title_color *a=hello_title_colorv,*v=hello_title_colorv;
  for (;i-->0;v++) {
    if (v->time>=phase) break;
    a=v;
  }
  const struct hello_title_color *b=a;
  if (b->time<1.0f) b++;
  if (b->time<=a->time) {
    return (a->r<<24)|(a->g<<16)|(a->b<<8)|a->a;
  }
  int bweight=((phase-a->time)*255.0f)/(b->time-a->time);
  if (bweight<0) bweight=0; else if (bweight>0xff) bweight=0xff;
  uint8_t aweight=0xff-bweight;
  uint8_t r=(a->r*aweight+b->r*bweight)>>8;
  uint8_t g=(a->g*aweight+b->g*bweight)>>8;
  uint8_t u=(a->b*aweight+b->b*bweight)>>8;
  uint8_t l=(a->a*aweight+b->a*bweight)>>8;
  return fmn_video_pixel_from_rgba((r<<24)|(g<<16)|(u<<8)|l);
}

/* Render.
 */
 
static void _hello_render(struct fmn_menu *menu) {
  const int16_t tilesize=menu->fbw/FMN_COLC;
  framec++;
  
  // Blackout.
  {
    // We, unlike other menus, can get created before the first render. Which can mean the renderer doesn't know its pixel format yet.
    // TODO That's highly stupid. Make it know its own format.
    if (!bgcolor) bgcolor=fmn_video_pixel_from_rgba(0x000000ff);
    struct fmn_draw_rect vtx={0,0,menu->fbw,menu->fbh,bgcolor};
    fmn_draw_rect(&vtx,1);
  }
  
  // Stars.
  hello_stars_render(menu);
  
  // Moon rises and Dot flies by it, coinciding at exactly the center of the screen.
  {
    const float period=23.0f;
    float dummy;
    float phase=modff(clock/period,&dummy);
    struct fmn_draw_decal vtxv[2];
    
    const int16_t moonw=tilesize*7;
    const int16_t moonh=tilesize*7;
    const int16_t moonya=menu->fbh+moonh; // start one moonheight below the bottom, allow some lead time before we appear
    const int16_t moonyz=-moonh;
    vtxv[0].dstx=(menu->fbw>>1)-(moonw>>1);
    vtxv[0].dsty=moonya+(moonyz-moonya)*phase;
    vtxv[0].srcx=0;
    vtxv[0].srcy=tilesize*5;
    vtxv[0].dstw=vtxv[0].srcw=moonw;
    vtxv[0].dsth=vtxv[0].srch=moonh;
    
    int16_t moonmidy=vtxv[0].dsty+(moonh>>1);
    int16_t flighta=(menu->fbh*2)/3;
    int16_t flightz=(menu->fbh*1)/3;
    float flightphase=(float)(moonmidy-flighta)/(float)(flightz-flighta);
    if (flightphase<0.0f) flightphase=0.0f;
    else if (flightphase>1.0f) flightphase=1.0f;
    const int16_t dotw=tilesize*5;
    const int16_t doth=tilesize*5;
    const int16_t dotxa=menu->fbw;
    const int16_t dotxz=-dotw;
    const int16_t dotya=menu->fbh;
    const int16_t dotyz=-doth;
    vtxv[1].dstx=dotxa+(dotxz-dotxa)*flightphase;
    vtxv[1].dsty=dotya+(dotyz-dotya)*flightphase;
    vtxv[1].srcx=tilesize*8;
    vtxv[1].srcy=tilesize*5;
    vtxv[1].dstw=vtxv[1].srcw=dotw;
    vtxv[1].dsth=vtxv[1].srch=doth;
    
    fmn_draw_decal(vtxv,2,14);
  }
  
  // "Full Moon"
  {
    int16_t srcw=0,srch=0;
    fmn_video_get_image_size(&srcw,&srch,18);
    if (srcw>0) {
      struct fmn_draw_recal vtx={
        (menu->fbw>>1)-(srcw>>1),(menu->fbh>>1)-(srch>>1),srcw,srch,
        0,0,srcw,srch,
        hello_title_get_color(clock),
      };
      fmn_draw_decal((struct fmn_draw_decal*)&vtx,1,18);
      fmn_draw_recal(&vtx,1,19);
    }
  }
}

/* Init.
 */
 
void fmn_menu_init_HELLO(struct fmn_menu *menu) {
  menu->update=_hello_update;
  menu->render=_hello_render;
  menu->opaque=1;
  fmn_play_song(1);
}
