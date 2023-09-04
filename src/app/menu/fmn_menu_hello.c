#include "fmn_menu_internal.h"

#define titlecolor menu->argv[1]
#define framec menu->argv[2]
#define bgcolor menu->argv[3]
#define selp menu->argv[4]
#define opt_available menu->argv[5]
#define flash_text menu->argv[6]
#define clock menu->fv[0]
#define idleclock menu->fv[1]

#define FMN_IMAGEID_LABEL_CONTINUE 307
#define FMN_IMAGEID_LABEL_NEW 308
#define FMN_IMAGEID_LABEL_SETTINGS 309
#define FMN_IMAGEID_LABEL_QUIT 310
#define FMN_IMAGEID_LABEL_END_GAME 311 /* we don't use this one, but we prep for pause menu */

static uint32_t color_selected=0,color_enabled=0,color_disabled=0;

/* Dismiss.
 */
 
static uint8_t hello_submit(struct fmn_menu *menu) {
  switch (selp) {
    case 0: { // Continue
        if (fmn_game_load_saved_game()<0) {
          fmn_sound_effect(FMN_SFX_UI_NO);
        } else {
          fmn_sound_effect(FMN_SFX_UI_YES);
          fmn_dismiss_menu(menu);
          return 1;
        }
      } return 0;
    case 1: { // New
        if (menu->cb) {
          menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
        } else {
          fmn_sound_effect(FMN_SFX_UI_YES);
          fmn_dismiss_menu(menu);
          if (fmn_game_load_map(1,-1.0f,-1.0f)<0) fmn_abort();
          fmn_map_callbacks(FMN_MAP_EVID_LOADED,fmn_game_map_callback,0);
        }
      } return 1;
    case 2: { // Settings
        struct fmn_menu *settings=fmn_begin_menu(FMN_MENU_SETTINGS,0);
        if (settings) {
          fmn_sound_effect(FMN_SFX_UI_YES);
          menu->pvinput=0xff;
        } else {
          fmn_sound_effect(FMN_SFX_UI_NO);
        }
      } return 0;
    case 3: { // Quit
        if (fmn_quit()<0) {
          fmn_abort();
        }
      } return 1;
  }
  return 0;
}

/* Change selection.
 */
 
static void hello_change_selection(struct fmn_menu *menu,int8_t d) {
  if (!(opt_available&0x0f)) return; // oh no
  fmn_sound_effect(FMN_SFX_UI_SHIFT);
  while (1) {
    selp+=d;
    if (selp>=4) selp=0;
    else if (selp<0) selp=3;
    if (opt_available&(1<<selp)) return;
  }
}

/* Update.
 */
 
static void _hello_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  clock+=elapsed;
  if (input!=menu->pvinput) {
    
    // Pressing USE may invoke a new menu, in which case pvinput will be forced on.
    // May also dismiss this menu.
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) {
      menu->pvinput=input;
      if (hello_submit(menu)) return;
      
    // MENU means "cancel" in this context, but there's nothing to cancel out to.
    } else if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) {
      if (0) { // ...or is there? Optionally use B to launch the secret sound-check menu.
        fmn_begin_menu(FMN_MENU_SOUNDCHECK,0);
        fmn_sound_effect(FMN_SFX_UI_YES);
        menu->pvinput=0xff;
      } else {
        menu->pvinput=input;
        fmn_sound_effect(FMN_SFX_UI_NO);
      }
      
    } else {
      const uint8_t verts=FMN_INPUT_UP|FMN_INPUT_DOWN;
      switch ((input&verts)&~menu->pvinput) {
        case FMN_INPUT_UP: hello_change_selection(menu,-1); break;
        case FMN_INPUT_DOWN: hello_change_selection(menu,1); break;
      }
      menu->pvinput=input;
    }
  }
  if (!input) {
    idleclock+=elapsed;
  } else {
    idleclock=0.0f;
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

/* Choose a tile (image 16) for the cursor.
 */
 
static uint8_t hello_select_cursor_tile(struct fmn_menu *menu) {
  // After 90 seconds, give up and take a nap.
  if (idleclock>=89.0f) {
    float p=fmodf(idleclock,2.0f);
    if (p>=1.8f) return 0x08;
    if (p>=1.6f) return 0x07;
    if (p>=1.4f) return 0x06;
    return 0x05;
  }
  // Blinking attentive for 20 seconds, then bored for 10...
  float grossp=fmodf(idleclock,30.0f);
  if (grossp>=20.5f) { // tapping foot
    float finep=fmodf(idleclock,0.3f);
    if (finep>=0.15f) return 0x03;
    return 0x02;
  } else if (grossp>=19.0f) { // look at watch
    return 0x04;
  } else { // blinking
    float finep=fmodf(idleclock,5.0f);
    if (finep>=4.7f) return 0x01;
    return 0x00;
  }
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
    if (!bgcolor)        bgcolor=fmn_video_pixel_from_rgba(       0x000000ff);
    if (!color_selected) color_selected=fmn_video_pixel_from_rgba(0xffff00ff);
    if (!color_enabled)  color_enabled=fmn_video_pixel_from_rgba( 0xffffffff);
    if (!color_disabled) color_disabled=fmn_video_pixel_from_rgba(0xc0c0c060);
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
  
  // Generate labels if we don't have them yet.
  // (selp==0xff) is our signal that we're not fully initialized yet. (have to wait until now to ensure the renderer is fully up).
  // The images persist for the program's life but we redraw every time this menu starts up. Not a big deal.
  if ((selp==0xff)||flash_text) {
    fmn_generate_string_image(FMN_IMAGEID_LABEL_CONTINUE,3,0,0);
    fmn_generate_string_image(FMN_IMAGEID_LABEL_NEW,4,0,0);
    fmn_generate_string_image(FMN_IMAGEID_LABEL_SETTINGS,5,0,0);
    fmn_generate_string_image(FMN_IMAGEID_LABEL_QUIT,6,0,0);
    fmn_generate_string_image(FMN_IMAGEID_LABEL_END_GAME,7,0,0);
    if (selp==0xff) selp=(opt_available&1)?0:1; // "Continue" if available, otherwise "New".
    flash_text=0;
  }
  
  int16_t cursorx=0,cursory=-100;
  { // Draw the text labels.
    int16_t w,h;
    struct fmn_draw_recal vtx;
    int16_t dstx=menu->fbw-100;
    int16_t dsty=menu->fbh-47;
    const int16_t vspacing=2;
    cursorx=dstx-10;
    const uint16_t imageidv[]={
      FMN_IMAGEID_LABEL_CONTINUE,
      FMN_IMAGEID_LABEL_NEW,
      FMN_IMAGEID_LABEL_SETTINGS,
      FMN_IMAGEID_LABEL_QUIT,
    };
    uint8_t optmask=1;
    const uint16_t *imageid=imageidv;
    int i=sizeof(imageidv)/sizeof(imageidv[0]),p=0;
    for (;i-->0;imageid++,optmask<<=1,dsty+=h+vspacing,p++) {
      fmn_video_get_image_size(&w,&h,*imageid);
      vtx=(struct fmn_draw_recal){dstx,dsty,w,h,0,0,w,h,(selp==p)?color_selected:(opt_available&optmask)?color_enabled:color_disabled};
      fmn_draw_recal(&vtx,1,*imageid);
      if (selp==p) cursory=dsty+(h>>1)-2;
    }
  }
  
  { // Draw an indicator left of the selected text label.
    struct fmn_draw_mintile vtx={cursorx,cursory,hello_select_cursor_tile(menu),0};
    fmn_draw_mintile(&vtx,1,16);
  }
}

/* Language changed.
 */
 
static void _hello_language_changed(struct fmn_menu *menu) {
  flash_text=1;
}

/* Init.
 */
 
void fmn_menu_init_HELLO(struct fmn_menu *menu) {
  menu->update=_hello_update;
  menu->render=_hello_render;
  menu->language_changed=_hello_language_changed;
  menu->opaque=1;
  selp=0xff;
  opt_available=0x06; // 0x01=continue 0x02=new 0x04=settings 0x08=quit
  if (fmn_game_has_saved_game()) {
    opt_available|=0x01;
  }
  if (fmn_can_quit()) {
    opt_available|=0x08;
  }
  fmn_play_song(1,1);
}
