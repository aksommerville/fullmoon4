#include "fmn_menu_internal.h"
#include "fmn_settings_model.h"

#define bgcolor menu->argv[0]
#define selcolor menu->argv[1]

/* Globals.
 */
 
static struct fmn_settings_model settings_model={0};

/* Dismiss.
 */
 
static void settings_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Activate selection.
 */
 
static void settings_activate(struct fmn_menu *menu) {
  switch (fmn_settings_model_get_field_id(&settings_model)) {
    // Boolean fields, activate is equivalent to adjust:
    case FMN_SETTINGS_FIELD_FULLSCREEN: fmn_settings_model_adjust(&settings_model,1); break;
    case FMN_SETTINGS_FIELD_MUSIC_ENABLE: fmn_settings_model_adjust(&settings_model,1); break;
    // But mostly we care about the stateless actions:
    case FMN_SETTINGS_FIELD_INPUT: {
        fmn_begin_menu(FMN_MENU_INPUT,0); 
        menu->pvinput=0xff;
      } break;
  }
}

/* Update.
 */
 
static void _settings_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  if (input!=menu->pvinput) {
    uint8_t pv=menu->pvinput; // cache this; settings_activate() may adulterate it
    menu->pvinput=input;
    if ((input&FMN_INPUT_MENU)&&!(pv&FMN_INPUT_MENU)) { settings_dismiss(menu); return; }
    if ((input&FMN_INPUT_USE)&&!(pv&FMN_INPUT_USE)) settings_activate(menu);
    const uint8_t verts=FMN_INPUT_UP|FMN_INPUT_DOWN;
    const uint8_t horzs=FMN_INPUT_LEFT|FMN_INPUT_RIGHT;
    switch ((input&verts)&~pv) {
      case FMN_INPUT_UP: fmn_settings_model_move(&settings_model,-1); break;
      case FMN_INPUT_DOWN: fmn_settings_model_move(&settings_model,1); break;
    }
    switch ((input&horzs)&~pv) {
      case FMN_INPUT_LEFT: fmn_settings_model_adjust(&settings_model,-1); break;
      case FMN_INPUT_RIGHT: fmn_settings_model_adjust(&settings_model,1); break;
    }
  }
}

/* Render.
 */
 
static void _settings_render(struct fmn_menu *menu) {
  const int16_t tilesize=menu->fbw/FMN_COLC;
  const int16_t glyphw=8,glyphh=8;
  
  // Blackout.
  {
    struct fmn_draw_rect vtxv[]={
      {0,0,menu->fbw,menu->fbh,bgcolor},
      {0,settings_model.selrow*glyphh-1,menu->fbw,glyphh+1,selcolor},
    };
    fmn_draw_rect(vtxv,sizeof(vtxv)/sizeof(struct fmn_draw_rect));
  }
  
  // Model's text grid.
  {
    struct fmn_draw_mintile vtxv[128];
    int vtxc=0;
    const char *src=settings_model.text;
    int16_t y=glyphh>>1,yi=FMN_SETTINGS_ROWC;
    for (;yi-->0;y+=glyphh) {
      int16_t x=glyphw>>1,xi=FMN_SETTINGS_COLC;
      for (;xi-->0;x+=glyphw,src++) {
        if (!*src) continue;
        struct fmn_draw_mintile *vtx=vtxv+vtxc++;
        vtx->x=x;
        vtx->y=y;
        vtx->tileid=*src;
        vtx->xform=0;
        if (vtxc>=sizeof(vtxv)/sizeof(struct fmn_draw_mintile)) {
          fmn_draw_mintile(vtxv,vtxc,20);
          vtxc=0;
        }
      }
    }
    if (vtxc>0) {
      fmn_draw_mintile(vtxv,vtxc,20);
    }
  }
}

/* Language changed.
 */
 
static void _settings_language_changed(struct fmn_menu *menu) {
  fmn_settings_model_language_changed(&settings_model);
}

/* Init.
 */
 
void fmn_menu_init_SETTINGS(struct fmn_menu *menu) {
  menu->update=_settings_update;
  menu->render=_settings_render;
  menu->language_changed=_settings_language_changed;
  menu->opaque=1;
  bgcolor=fmn_video_pixel_from_rgba(0x100020ff);
  selcolor=fmn_video_pixel_from_rgba(0x402000ff);
  fmn_settings_model_init(&settings_model);
}
