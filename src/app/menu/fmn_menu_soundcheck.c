#include "fmn_menu_internal.h"

#define SCK_GLYPH_W 8
#define SCK_GLYPH_H 8

#define bgcolor menu->argv[0]
#define selcolor menu->argv[1]
#define selp menu->argv[2]
#define songchoice menu->argv[3]
#define soundchoice menu->argv[4]
#define adjhold menu->argv[5]
#define clock menu->fv[0]
#define adjtime menu->fv[1]

/* Dismiss.
 */
 
static void soundcheck_dismiss(struct fmn_menu *menu) {
  fmn_sound_effect(FMN_SFX_UI_NO);
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
  }
}

/* Activate selection.
 */
 
static void soundcheck_activate(struct fmn_menu *menu) {
  switch (selp) {
    case 0: fmn_play_song(songchoice,1); break;
    case 1: fmn_sound_effect(soundchoice); break;
  }
}

/* Cursor.
 */
 
static void soundcheck_move(struct fmn_menu *menu,int d) {
  selp^=1;
}

static void soundcheck_adjust(struct fmn_menu *menu,int d) {
  switch (selp) {
    case 0: {
        songchoice+=d;
        if (songchoice<0) songchoice=999;
        else if (songchoice>999) songchoice=0;
      } break;
    case 1: {
        soundchoice+=d;
        if (soundchoice<0) soundchoice=999;
        else if (soundchoice>999) soundchoice=0;
      } break;
  }
}

/* Update.
 */
 
static void _soundcheck_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  clock+=elapsed;
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) {
      soundcheck_dismiss(menu);
      return;
    }
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) soundcheck_activate(menu);
    const uint8_t verts=FMN_INPUT_UP|FMN_INPUT_DOWN;
    const uint8_t horzs=FMN_INPUT_LEFT|FMN_INPUT_RIGHT;
    switch ((input&verts)&~menu->pvinput) {
      case FMN_INPUT_UP: soundcheck_move(menu,-1); break;
      case FMN_INPUT_DOWN: soundcheck_move(menu,1); break;
    }
    switch ((input&horzs)&~menu->pvinput) {
      case FMN_INPUT_LEFT: soundcheck_adjust(menu,-1); adjtime=0.25f; adjhold=FMN_INPUT_LEFT; break;
      case FMN_INPUT_RIGHT: soundcheck_adjust(menu,1); adjtime=0.25f; adjhold=FMN_INPUT_RIGHT; break;
      default: adjhold=0;
    }
    menu->pvinput=input;
  }
  if (adjhold) {
    if ((adjtime-=elapsed)<=0.0f) {
      adjtime=0.0675f;
      switch (adjhold) {
        case FMN_INPUT_LEFT: soundcheck_adjust(menu,-1); break;
        case FMN_INPUT_RIGHT: soundcheck_adjust(menu,1); break;
      }
    }
  }
}

/* Add text vertices.
 */
 
static int sck_add_string(struct fmn_draw_mintile *vtxv,int vtxc,int vtxa,int x,int y,const char *src) {
  for (;*src;src++,x+=SCK_GLYPH_W) {
    if (vtxc>=vtxa) {
      fmn_draw_mintile(vtxv,vtxc,20);
      vtxc=0;
    }
    vtxv[vtxc++]=(struct fmn_draw_mintile){
      .x=x,
      .y=y,
      .tileid=*src,
      .xform=0,
    };
  }
  return vtxc;
}

static int sck_add_decint(struct fmn_draw_mintile *vtxv,int vtxc,int vtxa,int x,int y,int v) {
  if (v<0) v=0;
  char s[4]={
    '0'+(v/100)%10,
    '0'+(v/10)%10,
    '0'+v%10,
    0,
  };
  return sck_add_string(vtxv,vtxc,vtxa,x,y,s);
}

/* Render.
 */
 
static void _soundcheck_render(struct fmn_menu *menu) {

  // Background and row highlight.
  {
    int y=(selp?120:100)-6;
    struct fmn_draw_rect vtxv[]={
      {0,0,menu->fbw,menu->fbh,bgcolor},
      {20,y,menu->fbw-40,12,selcolor},
    };
    fmn_draw_rect(vtxv,sizeof(vtxv)/sizeof(struct fmn_draw_rect));
  }
  
  // Options. Text labels on the left, and each has a 3-digit ID on the right.
  {
    struct fmn_draw_mintile vtxv[32];
    int vtxa=sizeof(vtxv)/sizeof(vtxv[0]);
    int vtxc=0;
    vtxc=sck_add_string(vtxv,vtxc,vtxa,100,100,"SONG");
    vtxc=sck_add_string(vtxv,vtxc,vtxa,92,120,"SOUND");
    vtxc=sck_add_decint(vtxv,vtxc,vtxa,150,100,songchoice);
    vtxc=sck_add_decint(vtxv,vtxc,vtxa,150,120,soundchoice);
    fmn_draw_mintile(vtxv,vtxc,20);
  }
}

/* Init.
 */
 
void fmn_menu_init_SOUNDCHECK(struct fmn_menu *menu) {
  menu->update=_soundcheck_update;
  menu->render=_soundcheck_render;
  menu->opaque=1;
  bgcolor=fmn_video_pixel_from_rgba(0x404060ff);
  selcolor=fmn_video_pixel_from_rgba(0x804000ff);
}
