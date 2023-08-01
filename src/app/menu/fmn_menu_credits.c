#include "fmn_menu_internal.h"
#include "app/fmn_game.h"

#define CREDITS_SUMMARY_W 320
#define CREDITS_SUMMARY_H 16
#define FMN_IMAGEID_CREDITS_SUMMARY 312 /* fmn_render_internal.h */

#define summary_ready menu->argv[1]
#define clock menu->fv[0]

#define CREDITS_MIN_UPTIME 1.0f

/* Globals. We need more context than the menu object will comfortably hold.
 */
 
static char credits_message[256];
static int credits_messagec=0;
static int credits_messagep=0;
static float credits_message_clock=0.0f; // counts down for each character

/* Break lines and prepare a new message.
 */
 
static void credits_set_message(struct fmn_menu *menu,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  const int linew=39;
  const int linelimit=5;
  credits_messagec=0;
  credits_messagep=0;
  int linec=1;
  int linelen=0;
  int srcp=0;
  for (;srcp<srcc;srcp++) {
  
    // Explicit LF, break the current line.
    if (src[srcp]==0x0a) {
      while (linelen<linew) {
        linelen++;
        credits_message[credits_messagec++]=' ';
        if (credits_messagec>=sizeof(credits_message)) goto _done_;
      }
      linelen=0;
      linec++;
      if (linec>linelimit) break;
      continue;
    }
    
    // If we crossed the line width, jump to the next, at the first non-space character.
    // Lines broken implicitly will never begin with a space.
    if (linelen>=linew) {
      if (src[srcp]==0x20) continue;
      linelen=0;
      linec++;
      if (linec>linelimit) break;
    }
  
    credits_message[credits_messagec++]=src[srcp];
    if (credits_messagec>=sizeof(credits_message)) break;
    linelen++;
    
    // If we just emitted a space, read ahead and break the line if the next word is too long.
    int wordlen=0;
    while ((srcp+1+wordlen<srcc)&&((unsigned char)src[srcp+1+wordlen]>0x20)) wordlen++;
    if ((linelen+wordlen>linew)&&(wordlen<=linew)) {
      while (linelen<linew) {
        linelen++;
        credits_message[credits_messagec++]=' ';
        if (credits_messagec>=sizeof(credits_message)) goto _done_;
      }
      linelen=0;
      linec++;
      if (linec>linelimit) break;
    }
  }
 _done_:;
  credits_message_clock=0.25f;
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
  
  if (credits_message_clock>0.0f) {
    if ((credits_message_clock-=elapsed)<=0.0f) {
      while ((credits_messagep<credits_messagec)&&(credits_message[credits_messagep++]<=0x20)) ;
      if (credits_messagep<credits_messagec) {
        credits_message_clock=0.125f;
      } else {
        credits_message_clock=0.0f;
      }
    }
  }
}

/* Generate summary: One line across the top showing statistics.
 */
 
static void credits_summary_generate(struct fmn_menu *menu) {
  fmn_video_init_image(FMN_IMAGEID_CREDITS_SUMMARY,CREDITS_SUMMARY_W,CREDITS_SUMMARY_H);
  if (fmn_draw_set_output(FMN_IMAGEID_CREDITS_SUMMARY)<0) return;
  fmn_draw_clear();
  
  // Framebuffer is 320 pixels wide, exactly 40 characters. Plus one for snprintf's retarded NUL.
  #define DIGIT_LIMIT 41
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
  char message[DIGIT_LIMIT];
  int messagec=snprintf(message,sizeof(message),
    " %2d:%02d:%02d.%03d       %2d/%02d         %5d ",
    hour,min,sec,ms,itemc,FMN_ITEM_COUNT,damagec
  );
  if ((messagec<1)||(messagec>=sizeof(message))) return;
  
  struct fmn_draw_mintile vtxv[DIGIT_LIMIT];
  int vtxc=0;
  const char *src=message;
  int x=4;
  for (i=messagec;i-->0;src++,x+=8) {
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
    struct fmn_draw_rect vtx={0,0,menu->fbw,menu->fbh,0x000000ff};
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
  
  // Picture.
  {
    struct fmn_draw_decal decal={
      0,CREDITS_SUMMARY_H,320,128,
      0,384,320,128,
    };
    fmn_draw_decal(&decal,1,26);
  }
  
  // Message. Break when we exceed the right margin, which will happen every 39 characters.
  {
    struct fmn_draw_mintile vtxv[160];
    int vtxc=0;
    const char *src=credits_message;
    int i=credits_messagep;
    int16_t x=8;
    int16_t y=CREDITS_SUMMARY_H+128+8;
    for (;i-->0;src++,x+=8) {
      if (x>312) {
        x=8;
        y+=8;
      }
      if (*src<=0x20) continue;
      if (vtxc>=sizeof(vtxv)/sizeof(vtxv[0])) break;
      struct fmn_draw_mintile *vtx=vtxv+vtxc++;
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=*src;
      vtx->xform=0;
    }
    fmn_draw_mintile(vtxv,vtxc,20);
  }
}

/* Init.
 */
 
void fmn_menu_init_CREDITS(struct fmn_menu *menu) {
  menu->update=_credits_update;
  menu->render=_credits_render;
  menu->opaque=1;
  fmn_play_song(7);
  credits_set_message(menu,"TODO: Decide what goes here.",-1);
}
