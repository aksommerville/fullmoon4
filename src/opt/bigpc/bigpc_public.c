#include "bigpc_internal.h"
 
struct bigpc bigpc={
  .exename="fullmoon",
};

/* Quit.
 */
 
void bigpc_quit() {
  bigpc_audio_play(bigpc.audio,0);
  
  int64_t elapsed_real_us=bigpc.clock.last_real_time_us-bigpc.clock.first_real_time_us;
  fprintf(stderr,
    "%s, clock stats: Final game time %u ms (%u ms real time). overflow=%d underflow=%d fault=%d wrap=%d\n",
    __func__,bigpc.clock.last_game_time_ms,(int)(elapsed_real_us/1000),
    bigpc.clock.overflowc,bigpc.clock.underflowc,bigpc.clock.faultc,bigpc.clock.wrapc
  );
  
  bigpc_video_driver_del(bigpc.video);
  bigpc_audio_driver_del(bigpc.audio);
  if (bigpc.inputv) {
    while (bigpc.inputc-->0) {
      bigpc_input_driver_del(bigpc.inputv[bigpc.inputc]);
    }
    free(bigpc.inputv);
  }
  
  bigpc_render_del(bigpc.render);
  bigpc_synth_del(bigpc.synth);
  fmn_datafile_del(bigpc.datafile);
  fmstore_del(bigpc.fmstore);
  inmgr_del(bigpc.inmgr);
  if (bigpc.map_callbackv) free(bigpc.map_callbackv);
  if (bigpc.sound_blackoutv) free(bigpc.sound_blackoutv);
  if (bigpc.logfile) fclose(bigpc.logfile);
  
  memset(&bigpc,0,sizeof(struct bigpc));
  bigpc.exename="fullmoon";
}

/* Init.
 */
 
int bigpc_init(int argc,char **argv) {
  int err;
  if ((err=bigpc_configure_argv(argc,argv))<0) return err;
  if ((err=bigpc_config_ready())<0) return err;
  
  if (!bigpc.config.data_path) {
    bigpc_config_guess_data_path();
    if (!bigpc.config.data_path) {
      fprintf(stderr,"%s: Please indicate data file as '--data=PATH'\n",bigpc.exename);
      return -2;
    }
  }
  if (!(bigpc.datafile=fmn_datafile_open(bigpc.config.data_path))) {
    fprintf(stderr,"%s: Failed to read data file.\n",bigpc.config.data_path);
    return -2;
  }
  if (!(bigpc.fmstore=fmstore_new())) return -1;
  if ((err=bigpc_log_init())<0) return err;
  
  if (BIGPC_IDLE_RESTART_TIME) {
    fprintf(stderr,"%s: Idle restart time %d. Don't release into prod with this set!\n",bigpc.exename,(int)(BIGPC_IDLE_RESTART_TIME/1000000ll));
  }
  
  bigpc_signal_init();
  if ((err=bigpc_video_init())<0) return err;
  if ((err=bigpc_audio_init())<0) return err;
  if ((err=bigpc_input_init())<0) return err;
  
  // With video and input both online, check whether we need to map the System Keyboard.
  if (bigpc.video->type->has_wm) {
    bigpc.devid_keyboard=bigpc_input_devid_next();
    inmgr_connect_system_keyboard(bigpc.inmgr,bigpc.devid_keyboard);
  }
  
  if (fmn_init()<0) {
    fprintf(stderr,"Error initializing game.\n");
    return -2;
  }
  
  bigpc_audio_play(bigpc.audio,1);
  bigpc_clock_reset(&bigpc.clock);
  bigpc.last_input_time=bigpc.clock.last_real_time_us;
  bigpc.idle_warning_time=0;
  return 0;
}

/* Reset, eg after the victory menu.
 */
 
static void bigpc_reset() {

  memset(&fmn_global,0,sizeof(fmn_global));
  fmstore_del(bigpc.fmstore);
  bigpc.fmstore=fmstore_new();
  
  if (fmn_init()<0) {
    fprintf(stderr,"Error reinitializing game.\n");
    bigpc.sigc++;
    return;
  }
  bigpc_clock_reset(&bigpc.clock);
  bigpc.last_input_time=bigpc.clock.last_real_time_us;
  bigpc.idle_warning_time=0;
}

/* Pop the top menu off the stack.
 */
 
static void bigpc_pop_menu() {
  if (bigpc.menuc<1) return;
  struct bigpc_menu *menu=bigpc.menuv[--(bigpc.menuc)];
  int prompt=menu->prompt;
  bigpc_menu_del(menu);
  bigpc_ignore_next_button();
  
  switch (prompt) {
    case FMN_MENU_VICTORY: bigpc_reset(); break;
    case FMN_MENU_HELLO: bigpc_play_song(3); break;
  }
}

/* Pause/resume song in response to violin.
 */
 
static void bigpc_check_violin() {
  if (bigpc.pause_for_violin) {
    if (fmn_global.active_item!=FMN_ITEM_VIOLIN) {
      bigpc.pause_for_violin=0;
      if (bigpc_audio_lock(bigpc.audio)>=0) {
        bigpc_synth_pause_song(bigpc.synth,0);
        bigpc_audio_unlock(bigpc.audio);
      }
    }
  } else if (fmn_global.active_item==FMN_ITEM_VIOLIN) {
    bigpc.pause_for_violin=1;
    if (bigpc_audio_lock(bigpc.audio)>=0) {
      bigpc_synth_pause_song(bigpc.synth,1);
      bigpc_audio_unlock(bigpc.audio);
    }
  }
}

/* When the werewolf gets killed, stop background music.
 */
 
static void bigpc_check_dead_werewolf() {
  if (fmn_global.werewolf_dead) {
    if (!bigpc.werewolf_dead) {
      bigpc.werewolf_dead=1;
      bigpc_play_song(0xff);
    }
  } else {
    bigpc.werewolf_dead=0;
  }
}

/* Watch for input changes. If input goes dead for a certain interval, show a warning, then revert to hello.
 * This is for my GDEX kiosk. Probably not appropriate for production.
 */
 
static int bigpc_is_idle() {
  if (bigpc.pvinput!=bigpc.input_state) return 0;
  if (bigpc.menuc>=1) {
    int prompt=bigpc.menuv[bigpc.menuc-1]->prompt;
    if (prompt==FMN_MENU_HELLO) return 0;
    if (prompt==FMN_MENU_VICTORY) return 0;
  }
  return 1;
}
 
static void bigpc_check_idle() {
  if (!bigpc_is_idle()) {
    bigpc.pvinput=bigpc.input_state;
    bigpc.last_input_time=bigpc.clock.last_real_time_us;
    bigpc.idle_warning_time=0;
  } else {
    int64_t idle_time=bigpc.clock.last_real_time_us-bigpc.last_input_time;
    if (idle_time>=BIGPC_IDLE_RESTART_TIME) {
      fmn_log_event("idle-restart","");
      bigpc_reset();
    } else if (idle_time>=BIGPC_IDLE_RESTART_WARN_TIME) {
      bigpc.idle_warning_time=(BIGPC_IDLE_RESTART_TIME-idle_time)/1000000ll+1;
    } else {
      bigpc.idle_warning_time=0;
    }
  }
}

int bigpc_get_idle_warning_time_s() {
  return bigpc.idle_warning_time;
}

/* Update.
 */
 
int bigpc_update() {
  if (bigpc.sigc) return 0;
  
  // Update drivers.
  if (bigpc_video_driver_update(bigpc.video)<0) return -1;
  if (bigpc_audio_update(bigpc.audio)<0) return -1;
  int i=bigpc.inputc;
  while (i-->0) {
    if (bigpc_input_driver_update(bigpc.inputv[i])<0) return -1;
  }
  
  // Miscellaneous high-level business logic that had to live here.
  bigpc_check_violin();
  bigpc_check_dead_werewolf();
  bigpc_check_idle();
  
  // Update game or top menu.
  if (bigpc.menuc) {
    bigpc_clock_skip(&bigpc.clock);
    int err=bigpc_menu_update(bigpc.menuv[bigpc.menuc-1]);
    if (err<0) return -1;
    if (!err) bigpc_pop_menu();
  } else if (bigpc.render->transition_in_progress) {
    bigpc_clock_skip(&bigpc.clock);
  } else {
    fmn_update(bigpc_clock_update(&bigpc.clock),bigpc.input_state);
  }
  
  // Render one frame.
  bigpc.render->w=bigpc.video->w;
  bigpc.render->h=bigpc.video->h;
  struct bigpc_image *fb=0;
  if (bigpc.video->renderer==BIGPC_RENDERER_rgb32) {
    if (!(fb=bigpc_video_driver_begin_soft(bigpc.video))) return -1;
  } else {
    if (bigpc_video_driver_begin_gx(bigpc.video)<0) return -1;
  }
  if (bigpc_render_update(fb,bigpc.render)<0) {
    bigpc_video_driver_cancel(bigpc.video);
    return -1;
  }
  bigpc_video_driver_end(bigpc.video);
  
  return bigpc.sigc?0:1;
}

/* Trivial accessors.
 */
 
struct bigpc_menu *bigpc_get_menu() {
  if (bigpc.menuc<1) return 0;
  return bigpc.menuv[bigpc.menuc-1];
}

uint32_t bigpc_get_game_time_ms() {
  return bigpc.clock.last_game_time_ms;
}
