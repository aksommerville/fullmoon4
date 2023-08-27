#include "fmn_menu_internal.h"
#include "app/eatwq/fmn_eatwq.h"

static struct arcade_global {
  struct fmn_eatwq_context *ctx;
} arcade_global={0};

#define skycolor menu->argv[0]
#define groundcolor menu->argv[1]
#define hellocolor menu->argv[2]
#define pvsongid menu->argv[4]
#define clock menu->fv[0]
#define nexttime menu->fv[1]

#define ARCADE_FRAME_TIME 0.016666f

/* Dismiss.
 */
 
static void arcade_dismiss(struct fmn_menu *menu) {
  fmn_sound_effect(FMN_SFX_UI_NO);
  fmn_play_song(pvsongid,1);
  arcade_global.ctx=0;
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Update.
 */
 
static void _arcade_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  clock+=elapsed;
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) {
      arcade_dismiss(menu);
      return;
    }
    // Input changed, so update whether we need it or not.
    fmn_eatwq_update(input,menu->pvinput);
    nexttime+=ARCADE_FRAME_TIME;
    menu->pvinput=input;
  }
  nexttime-=elapsed;
  if (nexttime<-0.5f) nexttime=0.0f; // clock panic
  while (nexttime<0.0f) {
    fmn_eatwq_update(input,input);
    nexttime+=ARCADE_FRAME_TIME;
  }
}

/* Render: Game in progress.
 */
 
static void arcade_render_running(struct fmn_menu *menu) {
  struct fmn_draw_rect rectv[]={
    {0,0,menu->fbw,menu->fbh,skycolor},
    {0,menu->fbh-16,menu->fbw,16,groundcolor},
  };
  fmn_draw_rect(rectv,sizeof(rectv)/sizeof(rectv[0]));
  struct fmn_draw_mintile vtxv[64];
  int vtxc=fmn_eatwq_render(vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  if (vtxc<=sizeof(vtxv)/sizeof(vtxv[0])) {
    // Game operates at (160,96) and our output is (320,192). Not a coincidence that it's exactly double.
    struct fmn_draw_mintile *vtx=vtxv;
    int i=vtxc; for (;i-->0;vtx++) {
      vtx->x<<=1;
      vtx->y<<=1;
    }
    fmn_draw_mintile(vtxv,vtxc,24);
  }
}

/* Render: Hello splash.
 */
 
static void arcade_render_hello(struct fmn_menu *menu) {
  struct fmn_draw_rect rectv[]={
    {0,0,menu->fbw,menu->fbh,hellocolor},
  };
  fmn_draw_rect(rectv,1);
  
  // We draw the 3 scale-up pieces. Mintile vertices still get generated at Hello, for the "high score" and "credits" display.
  struct fmn_draw_decal decalv[]={
    {(menu->fbw>>1)-64,25,128,64,80,224,64,32}, // "Enchanting Adventures"
    {(menu->fbw>>1)-64,90,128,32,144,224,64,16}, // "The Witch's Quest"
    {(menu->fbw>>1)-48,150,96,32,144,240,48,16}, // "Insert Coin"
  };
  int decalc=2;
  if (arcade_global.ctx&&!arcade_global.ctx->creditc) {
    float dummy;
    float phase=modff(clock*0.5f,&dummy);
    if (phase<0.75f) {
      decalc=3; // "Insert Coin"
    }
  }
  fmn_draw_decal(decalv,decalc,24);
  
  struct fmn_draw_mintile vtxv[64];
  int vtxc=fmn_eatwq_render(vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  if (vtxc<=sizeof(vtxv)/sizeof(vtxv[0])) {
    // Game operates at (160,96) and our output is (320,192). Not a coincidence that it's exactly double.
    struct fmn_draw_mintile *vtx=vtxv;
    int i=vtxc; for (;i-->0;vtx++) {
      vtx->x<<=1;
      vtx->y<<=1;
    }
    fmn_draw_mintile(vtxv,vtxc,24);
  }
}

/* Render, dispatch on state.
 */
 
static void _arcade_render(struct fmn_menu *menu) {
  if (fmn_eatwq_is_running()) arcade_render_running(menu);
  else arcade_render_hello(menu);
}

/* Runtime callbacks.
 */
 
static void arcade_cb_creditc(uint8_t creditc,void *userdata) {
  struct fmn_menu *menu=userdata;
  if (!arcade_global.ctx) return;
  arcade_global.ctx->creditc=creditc;
}
 
static void arcade_cb_hiscore(uint8_t hiscore,void *userdata) {
  struct fmn_menu *menu=userdata;
  if (!arcade_global.ctx) return;
  arcade_global.ctx->hiscore=hiscore;
}

/* Init.
 */
 
void fmn_menu_init_ARCADE(struct fmn_menu *menu) {
  if (!arcade_global.ctx) fmn_abort();
  menu->update=_arcade_update;
  menu->render=_arcade_render;
  menu->opaque=1;
  skycolor=fmn_video_pixel_from_rgba(0xa0c0f0ff);
  groundcolor=fmn_video_pixel_from_rgba(0x008020ff);
  hellocolor=fmn_video_pixel_from_rgba(0x391873ff);
  pvsongid=fmn_global.songid;
  fmn_eatwq_init(arcade_global.ctx->creditc,arcade_global.ctx->hiscore,arcade_cb_creditc,arcade_cb_hiscore,menu);
  fmn_play_song(14,1); // sky-gardening
}

/* Extra semi-private hooks.
 */
 
void fmn_arcade_set_context(struct fmn_eatwq_context *ctx) {
  arcade_global.ctx=ctx;
}
