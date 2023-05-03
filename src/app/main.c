#include "fmn_platform.h"
#include "fmn_game.h"

/* Idle warning time, automatically issues a warning, then restarts, when idle.
 * I need this for demo kiosks. Disable for production!
 * You can disable via the build: -DFMN_IDLE_WARNING_TIME=0
 */
#ifndef FMN_IDLE_WARNING_TIME
  #define FMN_IDLE_WARNING_TIME 30.0f
  #define FMN_IDLE_RESTART_TIME 45.0f
#endif

#define ELAPSED_TIME_LIMIT 1000

struct fmn_global fmn_global={0};
static uint32_t last_update_time=0;
static uint8_t pvinput=0;
static uint32_t platform_game_time=0;
static float idle_time=0.0f;

int fmn_init() {
  if (FMN_IDLE_WARNING_TIME>0.0f) {
    fmn_log(
      "*** Idle warning at %.0f and restart at %.0f seconds. Don't release to prod with this enabled! ***",
      FMN_IDLE_WARNING_TIME,FMN_IDLE_RESTART_TIME
    );
  }
  idle_time=0.0f;
  last_update_time=0;
  const int16_t tilesize=16;
  if (fmn_video_init(FMN_COLC*tilesize,FMN_COLC*tilesize,FMN_ROWC*tilesize,FMN_ROWC*tilesize,FMN_VIDEO_PIXFMT_RGBA)<0) return -1;
  if (fmn_game_init()<0) return -1;
  return 0;
}

static uint8_t fmn_is_idle(struct fmn_menu *menu,uint8_t input) {
  // We're only idle if input is completely untouched.
  if (input) return 0;
  if (menu) {
    // Don't declare idle during HELLO, because that's what we would reset to.
    // Similarly, not during VICTORY, since HELLO is just a click away.
    if (menu->menuid==FMN_MENU_HELLO) return 0;
    if (menu->menuid==FMN_MENU_VICTORY) return 0;
  }
  return 1;
}

int fmn_get_idle_warning_time_s() {
  if (idle_time<FMN_IDLE_WARNING_TIME) return -1;
  return (int)(FMN_IDLE_RESTART_TIME-idle_time);
}

void fmn_update(uint32_t timems,uint8_t input) {

  platform_game_time=timems;
  uint32_t elapsedms=timems-last_update_time;
  if ((elapsedms>ELAPSED_TIME_LIMIT)||(elapsedms<1)) {
    fmn_log("Clock fault! last_time=%d time=%d elapsed=%d",last_update_time,timems,elapsedms);
    elapsedms=1;
  }
  last_update_time=timems;
  float elapsed=elapsedms/1000.0f;
  
  struct fmn_menu *menu=fmn_get_top_menu();
  
  if (FMN_IDLE_WARNING_TIME>0.0f) {
    if (!fmn_is_idle(menu,input)) {
      idle_time=0.0f;
    } else {
      idle_time+=elapsed;
      if (idle_time>=FMN_IDLE_RESTART_TIME) {
        fmn_log_event("idle-restart","");
        fmn_reset();
      } else if (idle_time>=FMN_IDLE_WARNING_TIME) {
      } else {
      }
    }
  }
  
  if (menu) {
    if (menu->update) menu->update(menu,elapsed,input);
    pvinput=0xff; // ensures buttons must be released after a menu's dismissal
  } else if (fmn_render_transition_in_progress()) {
    // Suspend game action during transitions (mind that the clock does keep running)
  } else {
    if (input!=pvinput) {
      #define BIT(tag) if ((input&FMN_INPUT_##tag)&&!(pvinput&FMN_INPUT_##tag)) fmn_game_input(FMN_INPUT_##tag,1,input); \
      else if (!(input&FMN_INPUT_##tag)&&(pvinput&FMN_INPUT_##tag)) fmn_game_input(FMN_INPUT_##tag,0,input);
      BIT(LEFT)
      BIT(RIGHT)
      BIT(UP)
      BIT(DOWN)
      BIT(USE)
      BIT(MENU)
      #undef BIT
      pvinput=input;
    }
    fmn_game_update(elapsed);
  }
}

uint32_t fmn_game_get_platform_time_ms() {
  return platform_game_time;
}

/* The other public entry point, fmn_render(), is in src/app/render/fmn_render_main.c
 */
