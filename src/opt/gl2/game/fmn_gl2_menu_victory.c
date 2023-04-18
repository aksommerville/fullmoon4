#include "../fmn_gl2_internal.h"
#include "opt/bigpc/bigpc_menu.h"
#include "opt/fontosaur/fontosaur.h"

/* Irresponsible globals.
 */
 
static struct fmn_gl2_texture *victory_stats=0;

/* Clear victory menu.
 * We've arranged with bigpc to signal the last frame that the menu will be visible.
 */
 
void fmn_gl2_clear_menu_victory(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  if (victory_stats) {
    fmn_gl2_texture_cleanup(victory_stats);
    free(victory_stats);
    victory_stats=0;
  }
}

/* Generate the stats texture if it's null.
 */
 
static int fmn_gl2_require_menu_victory(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {
  if (victory_stats) return 0;
  
  uint32_t ms=bigpc_get_game_time_ms();
  int sec=ms/1000; ms%=1000;
  int min=sec/60; sec%=60;
  int hour=min/60; min%=60;
  int itemc=0; int i=1; for (;i<FMN_ITEM_COUNT;i++) if (fmn_global.itemv[i]) itemc++;
  int damagec=fmn_global.damage_count;
  
  char text[1024];
  int textc=snprintf(text,sizeof(text),
    "Time: %d:%02d:%02d.%03d\n"
    "Items: %d/15\n"
    "Damage: %d\n"
    "\n"
    "Thanks for playing this\n"
    "tiny Full Moon demo!\n"
    "Full version will be available\n"
    "on Steam and Itch.io\n"
    "29 September 2023\n"
    "- AK Sommerville",
    hour,min,sec,ms,itemc,damagec
  );
  if ((textc<0)||(textc>=sizeof(text))) return -1;

  struct fontosaur_image image={0};
  if (fontosaur_render_text(&image,driver->datafile,16,32,0x00000000,0xffffffff,0,text,textc)<0) return -1;
  
  if (!(victory_stats=calloc(1,sizeof(struct fmn_gl2_texture)))) {
    fontosaur_image_cleanup(&image);
    return -1;
  }
  if (fmn_gl2_texture_init_rgba(victory_stats,image.w,image.h,image.v)<0) {
    fontosaur_image_cleanup(&image);
    free(victory_stats);
    victory_stats=0;
    return -1;
  }
  fontosaur_image_cleanup(&image);
  
  return 0;
}

/* Render victory menu.
 */
 
void fmn_gl2_render_menu_victory(struct bigpc_render_driver *driver,struct bigpc_menu *menu) {

  // Blackout.
  fmn_gl2_program_use(driver,&DRIVER->program_raw);
  fmn_gl2_draw_raw_rect(0,0,DRIVER->mainfb.texture.w,DRIVER->mainfb.texture.h,0x000000ff);
  
  // "Victory!"
  if (fmn_gl2_texture_use(driver,14)>=0) {
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    int16_t w=128,h=48;
    int16_t dstx=(DRIVER->mainfb.texture.w>>1)-(w>>1);
    int16_t dsty=16;
    fmn_gl2_draw_decal(dstx,dsty,w,h,0,0,w,h);
  }
  
  // Stats. Render them if we don't have yet.
  if (fmn_gl2_require_menu_victory(driver,menu)>=0) {
    fmn_gl2_texture_use_object(driver,victory_stats);
    fmn_gl2_program_use(driver,&DRIVER->program_decal);
    int16_t dstx=(DRIVER->mainfb.texture.w>>1)-(victory_stats->w>>1);
    int16_t dsty=64+((DRIVER->mainfb.texture.h-64)>>1)-(victory_stats->h>>1);
    fmn_gl2_draw_decal(dstx,dsty,victory_stats->w,victory_stats->h,0,0,victory_stats->w,victory_stats->h);
  }
  
  if (menu->extra[0]) fmn_gl2_clear_menu_victory(driver,menu);
}
