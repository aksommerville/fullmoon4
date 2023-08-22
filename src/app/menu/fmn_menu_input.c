#include "fmn_menu_internal.h"

#define INPUT_GLYPH_W 8
#define INPUT_GLYPH_H 8

#define bgcolor menu->argv[0]
#define selcolor menu->argv[1]
#define cfgstate menu->argv[2]
#define cfgp menu->argv[3]
#define cfgbtn menu->argv[4]
#define selp menu->argv[5]
#define devc menu->argv[6]
#define cfgstatepv menu->argv[7]
#define cfgtime menu->fv[0] /* counts up during interactive config */

#define INPUT_CFGTIME_WARN 5.0f
#define INPUT_CFGTIME_ABORT 15.0f

/* Dismiss.
 */
 
static void input_dismiss(struct fmn_menu *menu) {
  fmn_sound_effect(FMN_SFX_UI_NO);
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Activate selection.
 */
 
static void input_activate(struct fmn_menu *menu) {
  if (cfgstate) return;
  if ((selp>=0)&&(selp<devc)) {
    fmn_sound_effect(FMN_SFX_UI_YES);
    fmn_platform_begin_input_configuration(selp);
  }
}

/* Cursor.
 */
 
static void input_move(struct fmn_menu *menu,int d) {
  if (cfgstate) return;
  if (devc<1) return;
  fmn_sound_effect(FMN_SFX_UI_SHIFT);
  selp+=d;
  if (selp<0) selp=devc-1;
  else if (selp>=devc) selp=0;
}

static void input_adjust(struct fmn_menu *menu,int d) {
  if (cfgstate) return;
}

/* Update.
 */
 
static void _input_update(struct fmn_menu *menu,float elapsed,uint8_t input) {

  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) {
      uint8_t cfgp8=0,cfgbtn8=0;
      if (fmn_platform_get_input_configuration_state(&cfgp8,&cfgbtn8)) {
        fmn_platform_cancel_input_configuration();
      } else {
        input_dismiss(menu);
        return;
      }
    }
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) input_activate(menu);
    const uint8_t verts=FMN_INPUT_UP|FMN_INPUT_DOWN;
    const uint8_t horzs=FMN_INPUT_LEFT|FMN_INPUT_RIGHT;
    switch ((input&verts)&~menu->pvinput) {
      case FMN_INPUT_UP: input_move(menu,-1); break;
      case FMN_INPUT_DOWN: input_move(menu,1); break;
    }
    switch ((input&horzs)&~menu->pvinput) {
      case FMN_INPUT_LEFT: input_adjust(menu,-1); break;
      case FMN_INPUT_RIGHT: input_adjust(menu,1); break;
    }
    menu->pvinput=input;
  }
  
  uint8_t cfgp8=0xff,cfgbtn8=0;
  cfgstate=fmn_platform_get_input_configuration_state(&cfgp8,&cfgbtn8);
  cfgp=cfgp8;
  cfgbtn=cfgbtn8;
  if (cfgstate&&(cfgstate!=cfgstatepv)) {
    cfgstatepv=cfgstate;
    cfgtime=0.0f;
  }
  if (cfgstate) {
    cfgtime+=elapsed;
    if (cfgtime>=INPUT_CFGTIME_ABORT) {
      cfgtime=0.0f;
      cfgstate=0;
      cfgstatepv=0;
      cfgp=0;
      cfgbtn=0;
      fmn_platform_cancel_input_configuration();
    }
  }
}

/* Render interactive configuration.
 */
 
static uint16_t input_stringid_for_btnid(uint8_t btnid) {
  switch (btnid) {
    case FMN_INPUT_LEFT: return 69;
    case FMN_INPUT_RIGHT: return 70;
    case FMN_INPUT_UP: return 71;
    case FMN_INPUT_DOWN: return 72;
    case FMN_INPUT_USE: return 73;
    case FMN_INPUT_MENU: return 74;
  }
  return 0;
}

// Returns full (vtxc). Flushes vertices if necessary.
static int input_add_text_line(struct fmn_draw_mintile *vtxv,int vtxc,int vtxa,struct fmn_menu *menu,const char *text,int textc,int16_t y) {
  if (textc<1) return vtxc;
  int16_t x=(menu->fbw>>1)-((textc*INPUT_GLYPH_W)>>1)+(INPUT_GLYPH_W>>1);
  const char *src=text;
  int i=textc;
  for (;i-->0;src++,x+=INPUT_GLYPH_W) {
    if ((unsigned char)(*src)<=0x20) continue;
    if (vtxc>=vtxa) {
      fmn_draw_mintile(vtxv,vtxc,20);
      vtxc=0;
    }
    struct fmn_draw_mintile *vtx=vtxv+vtxc++;
    vtx->x=x;
    vtx->y=y;
    vtx->tileid=*src;
    vtx->xform=0;
  }
  return vtxc;
}

static int input_add_string_line(struct fmn_draw_mintile *vtxv,int vtxc,int vtxa,struct fmn_menu *menu,uint16_t stringid,int16_t y) {
  char text[64];
  int textc=fmn_get_string(text,sizeof(text),stringid);
  if (textc<1) return vtxc;
  if (textc>sizeof(text)) textc=sizeof(text);
  return input_add_text_line(vtxv,vtxc,vtxa,menu,text,textc,y);
}
 
static void input_render_interactive(struct fmn_menu *menu,uint16_t promptstringid,uint16_t addlstringid) {
  struct fmn_draw_mintile vtxv[128];
  int vtxc=0,vtxa=sizeof(vtxv)/sizeof(vtxv[0]);
  vtxc=input_add_string_line(vtxv,vtxc,vtxa,menu,addlstringid,INPUT_GLYPH_H*9);
  vtxc=input_add_string_line(vtxv,vtxc,vtxa,menu,promptstringid,INPUT_GLYPH_H*11);
  vtxc=input_add_string_line(vtxv,vtxc,vtxa,menu,input_stringid_for_btnid(cfgbtn),INPUT_GLYPH_H*12);
  if (cfgtime>=INPUT_CFGTIME_WARN) {
    char msg[32]="Abort in ";
    int msgc=9;
    int s=INPUT_CFGTIME_ABORT-cfgtime;
    if (s<0) s=0;
    if (s>=10) {
      msg[msgc++]='0'+s/10;
    }
    msg[msgc++]='0'+s%10;
    vtxc=input_add_text_line(vtxv,vtxc,vtxa,menu,msg,msgc,INPUT_GLYPH_H*15);
  }
  fmn_draw_mintile(vtxv,vtxc,20);
}

/* Render device list, default view.
 */
 
static void input_render_list(struct fmn_menu *menu) {
  // It's hacky, but I'm going to use this render pass as also the canonical "gather list of devices" poll.
  // We're polling for devices every video frame. Hopefully it's a trivial matter on the platform end.
  devc=0;
  for (;devc<0x100;devc++) {
    char name[32];
    uint8_t namec=fmn_platform_get_input_device_name(name,sizeof(name),devc);
    if (!namec) break;
    if (namec>sizeof(name)) namec=sizeof(name);
    if (devc==selp) {
      struct fmn_draw_rect rect={0,devc*INPUT_GLYPH_H-1+(INPUT_GLYPH_H>>1),menu->fbw,INPUT_GLYPH_H+1,selcolor};
      fmn_draw_rect(&rect,1);
    }
    struct fmn_draw_mintile vtxv[32];
    int vtxc=0;
    int16_t x=INPUT_GLYPH_W; // no '>>1'; half a glyph of left margin
    int16_t y=(devc+1)*INPUT_GLYPH_H; // half a glyph of top margin
    int i=0; for (;i<namec;i++,x+=INPUT_GLYPH_W) {
      if (name[i]>0x20) {
        struct fmn_draw_mintile *vtx=vtxv+vtxc++;
        vtx->x=x;
        vtx->y=y;
        vtx->tileid=name[i];
        vtx->xform=0;
      }
    }
    fmn_draw_mintile(vtxv,vtxc,20);
  }
  if (selp>=devc) selp=0;
}

/* Render.
 */
 
static void _input_render(struct fmn_menu *menu) {
  {
    struct fmn_draw_rect vtxv[]={
      {0,0,menu->fbw,menu->fbh,bgcolor},
    };
    fmn_draw_rect(vtxv,sizeof(vtxv)/sizeof(struct fmn_draw_rect));
  }
  
  switch (cfgstate) {
    case FMN_INCFG_STATE_NONE: input_render_list(menu); break;
    case FMN_INCFG_STATE_READY: input_render_interactive(menu,66,0); break;
    case FMN_INCFG_STATE_CONFIRM: input_render_interactive(menu,67,0); break;
    case FMN_INCFG_STATE_FAULT: input_render_interactive(menu,66,68); break;
  }
}

/* Language changed.
 */
 
static void _input_language_changed(struct fmn_menu *menu) {
}

/* Init.
 */
 
void fmn_menu_init_INPUT(struct fmn_menu *menu) {
  menu->update=_input_update;
  menu->render=_input_render;
  menu->language_changed=_input_language_changed;
  menu->opaque=1;
  bgcolor=fmn_video_pixel_from_rgba(0x002030ff);
  selcolor=fmn_video_pixel_from_rgba(0x804000ff);
}
