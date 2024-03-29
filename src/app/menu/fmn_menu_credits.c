#include "fmn_menu_internal.h"
#include "fmn_scene.h"
#include "app/fmn_game.h"

#define CREDITS_SUMMARY_W 320
#define CREDITS_SUMMARY_H 16
#define FMN_IMAGEID_CREDITS_SUMMARY 312 /* fmn_render_internal.h */
#define CREDITS_SCENE_W 320
#define CREDITS_SCENE_H 128
#define CREDITS_MESSAGE_COLC 40
#define CREDITS_MESSAGE_ROWC 5
#define CREDITS_GLYPH_W 8
#define CREDITS_GLYPH_H 8
#define CREDITS_MIN_UPTIME 10.0f /* very long, so you don't accidentally miss it, but don't make them wait the whole time */
#define CREDITS_TYPEWRITER_DELAY 0.100f
#define CREDITS_MESSAGE_POST_DELAY 18.0f

#define summary_ready menu->argv[1]
#define clock menu->fv[0]

/* Content schedule.
 */

static const struct credits_text {
  uint16_t stringidv[CREDITS_MESSAGE_ROWC];
  int align;
} credits_textv[]={
  {{0,0,0,0,0},0}, // wait a tasteful interval
  {{34,35,36,37,38},0}, // cast
  {{39,0,40,0,0},0}, // AK
  {{41,0,42,43,44},0}, // open source
  {{45,0,46,47,48},0}, // beta
  {{0,49},0}, // gdex
  {{0,0,0,0,50},1}, // Dot Vine will return
};

static const struct fmn_scene_setup credits_setupv[]={
  {0,0,5.0f}, // take a deep breath
  {146,FMN_SCENE_ACTION_DRAG_RL,20.0f}, // castle 3f
  {147,FMN_SCENE_ACTION_DRAG_LR,20.0f}, // castle 2f
  {148,FMN_SCENE_ACTION_DRAG_RL,20.0f}, // castle 1f
  {149,FMN_SCENE_ACTION_DRAG_HOME,10.0f}, // outside dot's
  {0,FMN_SCENE_ACTION_BUTCHER,10.0f},
  {0,FMN_SCENE_ACTION_SEW,10.0f},
  {150,FMN_SCENE_ACTION_CLOTHE,30.0f}, // village
  {101,FMN_SCENE_ACTION_GOODNIGHT,20.0f},
};

/* Globals. We need more context than the menu object will comfortably hold.
 */
 
static float credits_typewriter_clock=0.0f; // counts down for each character
static int credits_textp=0;
static float credits_text_clock=0.0f; // counts down until the next message, once the message is done typing.
static struct fmn_draw_mintile credits_messagev[CREDITS_MESSAGE_COLC*CREDITS_MESSAGE_ROWC];
static int credits_messagec=0;
static int credits_messagep=0; // typewriter position

static struct fmn_scene credits_scene={0};
static int credits_scenep=0; // next position in credits_setupv

/* Prepare the next text message.
 */
 
static void credits_append_line(uint16_t stringid,int align,int row) {
  if (!stringid) return;
  char text[64];
  int textc=fmn_get_string(text,sizeof(text),stringid);
  if ((textc<1)||(textc>sizeof(text))) return;
  int textw=CREDITS_GLYPH_W*textc;
  int16_t x;
  if (align<0) x=0;
  else if (align>0) x=CREDITS_MESSAGE_COLC*CREDITS_GLYPH_W-textw;
  else x=((CREDITS_MESSAGE_COLC*CREDITS_GLYPH_W)>>1)-(textw>>1);
  x+=CREDITS_GLYPH_W>>1;
  int16_t y=CREDITS_SUMMARY_H+CREDITS_SCENE_H+row*CREDITS_GLYPH_H+CREDITS_GLYPH_H; // we actually have room for 6 lines, but use 5 to allow some spacing
  
  // In order to align the "Cast" section, I pad edges with tildes. Now that we're measured, strip them.
  const char *src=text;
  while (textc&&(text[textc-1]=='~')) textc--;
  while (textc&&(src[0]=='~')) { src++; textc--; x+=CREDITS_GLYPH_W; }
  
  int i=0; for (;i<textc;i++,src++,x+=CREDITS_GLYPH_W) {
    if (!*src) continue;
    struct fmn_draw_mintile *vtx=credits_messagev+credits_messagec++;
    vtx->x=x;
    vtx->y=y;
    vtx->tileid=*src;
    vtx->xform=0;
  }
}

static void credits_text_next(struct fmn_menu *menu) {
  credits_messagep=0;
  credits_messagec=0;
  int c=sizeof(credits_textv)/sizeof(struct credits_text);
  if (credits_textp>=c) {
    credits_messagep=0;
    credits_messagec=0;
    credits_typewriter_clock=0.0f;
    credits_text_clock=999.9f;
    return;
  }
  const struct credits_text *text=credits_textv+credits_textp++;
  int row=0; for (;row<CREDITS_MESSAGE_ROWC;row++) {
    credits_append_line(text->stringidv[row],text->align,row);
  }
  if (credits_messagec) {
    credits_typewriter_clock=CREDITS_TYPEWRITER_DELAY;
    credits_text_clock=CREDITS_MESSAGE_POST_DELAY;
  } else {
    credits_typewriter_clock=0.0f;
    credits_text_clock=CREDITS_MESSAGE_POST_DELAY;
  }
}

/* Begin next scene.
 */

static void credits_scene_next(struct fmn_menu *menu) {
  int c=sizeof(credits_setupv)/sizeof(struct fmn_scene_setup);
  if (credits_scenep>=c) {
    fmn_scene_blank(&credits_scene);
    return;
  }
  if (fmn_scene_init(&credits_scene,credits_setupv+credits_scenep++)<0) {
    fmn_scene_blank(&credits_scene);
    return;
  }
}

/* Dismiss.
 */
 
static void credits_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
    fmn_reset();
  }
}

/* Update.
 */
 
static void _credits_update(struct fmn_menu *menu,float elapsed,uint8_t input) {

  clock+=elapsed;
  if (input!=menu->pvinput) {
    if (clock>=CREDITS_MIN_UPTIME) {
      if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) { credits_dismiss(menu); return; }
      if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { credits_dismiss(menu); return; }
    }
    menu->pvinput=input;
  }
  
  if (credits_messagep>=credits_messagec) {
    if ((credits_text_clock-=elapsed)<=0.0f) {
      credits_text_next(menu);
    }
  }
  
  if (credits_typewriter_clock>0.0f) {
    if ((credits_typewriter_clock-=elapsed)<=0.0f) {
      credits_messagep++;
      if (credits_messagep<credits_messagec) {
        credits_typewriter_clock=CREDITS_TYPEWRITER_DELAY;
      } else {
        credits_typewriter_clock=0.0f;
      }
    }
  }
  
  if (fmn_scene_update(&credits_scene,elapsed)) {
    credits_scene_next(menu);
  }
}

/* Generate summary: One line across the top showing statistics.
 */
 
static void credits_summary_generate(struct fmn_menu *menu) {
  fmn_video_init_image(FMN_IMAGEID_CREDITS_SUMMARY,CREDITS_SUMMARY_W,CREDITS_SUMMARY_H);
  if (fmn_draw_set_output(FMN_IMAGEID_CREDITS_SUMMARY)<0) return;
  fmn_draw_clear();
  
  uint32_t ms=fmn_game_get_play_time_ms();
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  int hour=min/60; min%=60;
  if (hour>99) { // we'll display 2-digit hours, which is crazy, but won't do 3. Show all nines to make clear it's saturated.
    hour=min=sec=99;
    ms=999;
  }
  int itemc=0; int i=0; for (;i<FMN_ITEM_COUNT;i++) if (fmn_global.itemv[i]) itemc++;
  int damagec=fmn_global.damage_count; // stored in 16 bits; can't exceed 5 digits.
  char message[CREDITS_MESSAGE_COLC]=" XX:XX:XX.XXX       XX/XX         XXXXX ";
  message[1]=(hour>=10)?('0'+hour/10):' ';
  message[2]='0'+hour%10;
  message[4]='0'+min/10;
  message[5]='0'+min%10;
  message[7]='0'+sec/10;
  message[8]='0'+sec%10;
  message[10]='0'+ms/100;
  message[11]='0'+(ms/10)%10;
  message[12]='0'+ms%10;
  message[20]=(itemc>=10)?('0'+itemc/10):' ';
  message[21]='0'+itemc%10;
  message[23]='0'+FMN_ITEM_COUNT/10;
  message[24]='0'+FMN_ITEM_COUNT%10;
  message[34]=(damagec>=10000)?('0'+damagec/10000):' ';
  message[35]=(damagec>=1000)?('0'+(damagec/1000)%10):' ';
  message[36]=(damagec>=100)?('0'+(damagec/100)%10):' ';
  message[37]=(damagec>=10)?('0'+(damagec/10)%10):' ';
  message[38]='0'+damagec%10;
  int messagec=40;
  
  struct fmn_draw_mintile vtxv[CREDITS_MESSAGE_COLC];
  int vtxc=0;
  const char *src=message;
  int x=CREDITS_GLYPH_W>>1;
  for (i=messagec;i-->0;src++,x+=CREDITS_GLYPH_W) {
    if (*src<=0x20) continue;
    struct fmn_draw_mintile *vtx=vtxv+vtxc++;
    vtx->x=x;
    vtx->y=CREDITS_SUMMARY_H>>1;
    vtx->tileid=*src;
    vtx->xform=0;
  }
  fmn_draw_mintile(vtxv,vtxc,20);
  
  fmn_draw_set_output(0);
  #undef DIGIT_LIMIT
}

/* Render.
 */
 
static void _credits_render(struct fmn_menu *menu) {

  if (!summary_ready) {
    credits_summary_generate(menu);
    summary_ready=1;
  }

  // Blackout.
  {
    struct fmn_draw_rect vtx={0,0,menu->fbw,menu->fbh,fmn_video_pixel_from_rgba(0x000000ff)};
    fmn_draw_rect(&vtx,1);
  }
  
  // Summary.
  {
    struct fmn_draw_decal decal={
      0,0,CREDITS_SUMMARY_W,CREDITS_SUMMARY_H,
      0,0,CREDITS_SUMMARY_W,CREDITS_SUMMARY_H,
    };
    fmn_draw_decal(&decal,1,FMN_IMAGEID_CREDITS_SUMMARY);
  }
  
  // Scene.
  fmn_scene_render(0,CREDITS_SUMMARY_H,320,128,&credits_scene);
  
  // Message.
  fmn_draw_mintile(credits_messagev,credits_messagep,20);
}

/* Init.
 */
 
void fmn_menu_init_CREDITS(struct fmn_menu *menu) {
  menu->update=_credits_update;
  menu->render=_credits_render;
  menu->opaque=1;
  fmn_play_song(7,0);
  credits_textp=0;
  credits_text_next(menu);
  credits_scenep=0;
  credits_scene_next(menu);
}
