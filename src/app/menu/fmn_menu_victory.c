#include "fmn_menu_internal.h"
#include "app/fmn_game.h"

#define VICTORY_STATS_W 240
#define VICTORY_STATS_H 80
#define FMN_IMAGEID_VICTORY_BITS 304 /* oops, these symbols are owned privately by fmn_render */

#define bits_ready menu->argv[1]

/* Dismiss.
 */
 
static void victory_dismiss(struct fmn_menu *menu) {
  if (menu->cb) {
    menu->cb(menu,FMN_MENU_MESSAGE_SUBMIT);
  } else {
    fmn_dismiss_menu(menu);
    fmn_init();
  }
}

/* Update.
 */
 
static void _victory_update(struct fmn_menu *menu,float elapsed,uint8_t input) {
  if (input!=menu->pvinput) {
    if ((input&FMN_INPUT_USE)&&!(menu->pvinput&FMN_INPUT_USE)) { victory_dismiss(menu); return; }
    if ((input&FMN_INPUT_MENU)&&!(menu->pvinput&FMN_INPUT_MENU)) { victory_dismiss(menu); return; }
    menu->pvinput=input;
  }
}

/* Render the stats and farewell message. Once per menu.
 */
 
static void victory_bits_prepare(struct fmn_menu *menu) {
  fmn_video_init_image(FMN_IMAGEID_VICTORY_BITS,VICTORY_STATS_W,VICTORY_STATS_H);
  if (fmn_draw_set_output(FMN_IMAGEID_VICTORY_BITS)<0) return;
  
  {
    struct fmn_draw_rect vtx={0,0,VICTORY_STATS_W,VICTORY_STATS_H,0x000000ff};
    fmn_draw_rect(&vtx,1);
  }
  
  uint32_t ms=fmn_game_get_play_time_ms();
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  int hour=min/60; min%=60;
  int itemc=0; int i=0; for (;i<FMN_ITEM_COUNT;i++) if (fmn_global.itemv[i]) itemc++;
  int damagec=fmn_global.damage_count;
  
  {
    const int16_t tilesize=8;
    int16_t px=tilesize>>1,py=tilesize>>1;
    const char *message;
    struct fmn_draw_mintile vtxv[200];
    int vtxc=0;
    #define CHR(codepoint) { \
      uint8_t ch=(codepoint); \
      if ((ch!=0x20)&&(vtxc<200)) vtxv[vtxc++]=(struct fmn_draw_mintile){px,py,ch,0}; \
      px+=tilesize; \
    }
    #define EOL { px=tilesize>>1; py+=tilesize; }
    
    for (message="         Time: ";*message;message++) CHR(*message)
    CHR('0'+hour) CHR(':') CHR('0'+min/10) CHR('0'+min%10) CHR(':') CHR('0'+sec/10) CHR('0'+sec%10) CHR('.') CHR('0'+ms/100) CHR('0'+(ms/10)%10) CHR('0'+ms%10)
    EOL
    
    for (message="        Items: ";*message;message++) CHR(*message)
    if (itemc>=10) CHR('0'+itemc/10) CHR('0'+itemc%10) CHR('/') CHR('1') CHR('6')
    EOL
    
    for (message="       Damage: ";*message;message++) CHR(*message)
    if (damagec>=10000) {
      for (message="what the...";*message;message++) CHR(*message)
    } else {
      if (damagec>=1000) CHR('0'+(damagec/1000)%10)
      if (damagec>= 100) CHR('0'+(damagec/ 100)%10)
      if (damagec>=  10) CHR('0'+(damagec/  10)%10)
                         CHR('0'+(damagec     )%10)
    }
    EOL
    
    EOL
    EOL
    for (message="Thanks for playing Full Moon! ";*message;message++) CHR(*message) EOL
    for (message="This was just a taste...      ";*message;message++) CHR(*message) EOL
    for (message="Real thing will be available  ";*message;message++) CHR(*message) EOL
    for (message="29 September 2023 on Itch.io. ";*message;message++) CHR(*message) EOL
    for (message="               -AK Sommerville";*message;message++) CHR(*message) EOL
    
    #undef CHR
    #undef EOL
    fmn_draw_mintile(vtxv,vtxc,20);
  }
  
  fmn_draw_set_output(0);
}

/* Render.
 */
 
static void _victory_render(struct fmn_menu *menu) {

  if (!bits_ready) {
    bits_ready=1;
    victory_bits_prepare(menu);
  }

  // Blackout.
  {
    struct fmn_draw_rect vtx={0,0,menu->fbw,menu->fbh,0x000000ff};
    fmn_draw_rect(&vtx,1);
  }
  
  // "Victory!"
  {
    int16_t w=128,h=48;
    int16_t dstx=(menu->fbw>>1)-(w>>1);
    int16_t dsty=16;
    struct fmn_draw_decal vtx={dstx,dsty,w,h,0,0,w,h};
    fmn_draw_decal(&vtx,1,14);
  }
  
  // Stats and message.
  {
    int16_t dstx=(menu->fbw>>1)-(VICTORY_STATS_W>>1);
    int16_t dsty=64+((menu->fbh-64)>>1)-(VICTORY_STATS_H>>1);
    struct fmn_draw_decal vtx={dstx,dsty,VICTORY_STATS_W,VICTORY_STATS_H,0,0,VICTORY_STATS_W,VICTORY_STATS_H};
    fmn_draw_decal(&vtx,1,FMN_IMAGEID_VICTORY_BITS);
  }
}

/* Init.
 */
 
void fmn_menu_init_VICTORY(struct fmn_menu *menu) {
  menu->update=_victory_update;
  menu->render=_victory_render;
  menu->opaque=1;
  //TODO change music
}
