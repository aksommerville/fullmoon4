#include "bigpc_internal.h"
 
struct bigpc bigpc={
  .exename="fullmoon",
};

/* Quit.
 */
 
void bigpc_quit() {
  bigpc_audio_play(bigpc.audio,0);
  
  int64_t elapsed_real_us=bigpc.clock.last_real_time_us-bigpc.clock.first_real_time_us;
  double elapsed_real_f=elapsed_real_us/1000000.0;
  fprintf(stderr,
    "%s, clock stats: Final game time %u ms (%u ms real time). overflow=%d underflow=%d fault=%d wrap=%d cpu=%.06f. Avg frame rate %.03f Hz.\n",
    __func__,bigpc.clock.last_game_time_ms,(int)(elapsed_real_us/1000),
    bigpc.clock.overflowc,bigpc.clock.underflowc,bigpc.clock.faultc,bigpc.clock.wrapc,
    bigpc_clock_estimate_cpu_load(&bigpc.clock),
    bigpc.clock.framec/elapsed_real_f
  );
  
  #if FMN_USE_gamemon
    gamemon_del(bigpc.gamemon);
  #endif
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
  if (bigpc.savedgame_path) free(bigpc.savedgame_path);
  if (bigpc.savedgame_serial) free(bigpc.savedgame_serial);
  if (bigpc.langv) free(bigpc.langv);
  bigpc_config_cleanup(&bigpc.config);
  
  memset(&bigpc,0,sizeof(struct bigpc));
  bigpc.exename="fullmoon";
}

/* Init.
 */
 
int bigpc_init(int argc,char **argv) {
  int err;
  bigpc_config_init();
  if ((err=bigpc_configure_argv(argc,argv))<0) return err;
  if ((err=bigpc_config_ready())<0) return err;
  
  if (!(bigpc.datafile=fmn_datafile_open(bigpc.config.data_path))) {
    fprintf(stderr,"%s: Failed to read data file.\n",bigpc.config.data_path);
    return -2;
  }
  if (!(bigpc.fmstore=fmstore_new())) return -1;
  if ((err=bigpc_log_init())<0) return err;
  bigpc_rebuild_language_list();
  
  bigpc_signal_init();
  if ((err=bigpc_video_init())<0) return err;
  if ((err=bigpc_audio_init())<0) return err;
  if ((err=bigpc_input_init())<0) return err;
  bigpc_savedgame_init();
  bigpc_settings_init();
  
  // With video and input both online, check whether we need to map the System Keyboard.
  if (bigpc.video->type->has_wm) {
    bigpc.devid_keyboard=bigpc_input_devid_next();
    inmgr_connect(bigpc.inmgr,bigpc.devid_keyboard,"System keyboard",-1,0,0);
    if (inmgr_ready(bigpc.inmgr)>0) {
      bigpc_save_input_config();
    }
  }
  
  if (fmn_init()<0) {
    fprintf(stderr,"Error initializing game.\n");
    return -2;
  }
  
  bigpc_audio_play(bigpc.audio,1);
  bigpc_clock_reset(&bigpc.clock);
  bigpc_clock_capture_start_time(&bigpc.clock);
  return 0;
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

/* Update.
 */
 
int bigpc_update() {
  if (bigpc.sigc) return 0;
  
  // Acquire live input config state. Commit if just completed.
  bigpc.incfg_status=inmgr_live_config_status(&bigpc.incfg_btnid,bigpc.inmgr);
  if (bigpc.incfg_status==INMGR_LIVE_CONFIG_SUCCESS) {
    inmgr_rewrite_rules_for_live_config(bigpc.inmgr);
    inmgr_live_config_end(bigpc.inmgr);
    bigpc_save_input_config();
  }
  
  // Update drivers.
  #if FMN_USE_gamemon
    if (bigpc.gamemon) {
      gamemon_update(bigpc.gamemon);
      if (bigpc.gamemon_ready) {
        if (bigpc.gamemon_clock--<=0) {
          bigpc.gamemon_clock=30; // Aim for about 2 Hz. The Tiny can handle considerably faster, but PicoSystem chokes.
          bigpc_gamemon_send_framebuffer();
        }
      }
    }
  #endif
  if (bigpc_video_driver_update(bigpc.video)<0) return -1;
  if (bigpc_audio_update(bigpc.audio)<0) return -1;
  int i=bigpc.inputc;
  while (i-->0) {
    if (bigpc_input_driver_update(bigpc.inputv[i])<0) return -1;
  }
  bigpc_savedgame_update();
  
  // Miscellaneous high-level business logic that had to live here.
  bigpc_check_violin();
  
  // Update game or top menu.
  fmn_update(bigpc_clock_update(&bigpc.clock),bigpc.input_state);
  if (bigpc.aborted) return -1;
  if (bigpc.sigc) return 0;
  bigpc_settings_save_if_dirty();
  
  // Render one frame.
  bigpc.render->w=bigpc.video->w;
  bigpc.render->h=bigpc.video->h;
  struct bigpc_image *fb=0;
  if (bigpc.video->renderer==BIGPC_RENDERER_rgb32) {
    if (!(fb=bigpc_video_driver_begin_soft(bigpc.video))) return -1;
  } else {
    if (bigpc_video_driver_begin_gx(bigpc.video)<0) return -1;
  }
  bigpc.render->type->begin(bigpc.render,fb);
  uint8_t render_result=fmn_render();
  bigpc.render->type->end(bigpc.render,render_result);
  bigpc_video_driver_end(bigpc.video);

  return bigpc.sigc?0:1;
}
