#ifndef BIGPC_INTERNAL_H
#define BIGPC_INTERNAL_H

#include "app/fmn_platform.h"
#include "bigpc.h"
#include "bigpc_video.h"
#include "bigpc_audio.h"
#include "bigpc_input.h"
#include "bigpc_synth.h"
#include "bigpc_render.h"
#include "bigpc_clock.h"
#include "opt/datafile/fmn_datafile.h"
#include "opt/fmstore/fmstore.h"
#include "opt/inmgr/inmgr.h"
#include <stdio.h>
#include <stdlib.h>

#if FMN_USE_gamemon
  #include "gamemon.h"
#endif

#define BIGPC_ACTIONID_quit             1
#define BIGPC_ACTIONID_fullscreen       2
#define BIGPC_ACTIONID_pause            3
#define BIGPC_ACTIONID_step             4
#define BIGPC_ACTIONID_screencap        5
#define BIGPC_ACTIONID_save             6
#define BIGPC_ACTIONID_restore          7
#define BIGPC_ACTIONID_mainmenu         8

#define BIGPC_FOR_EACH_ACTIONID \
  _(quit) \
  _(fullscreen) \
  _(pause) \
  _(step) \
  _(screencap) \
  _(save) \
  _(restore) \
  _(mainmenu)
  
#define BIGPC_FOR_EACH_BTNID \
  _(LEFT) \
  _(RIGHT) \
  _(UP) \
  _(DOWN) \
  _(USE) \
  _(MENU)
  
struct bigpc_config {
  char *video_drivers;
  int video_fullscreen;
  int video_renderer;
  int tilesize;
  char *video_device;
  
  char *input_drivers;
  
  char *audio_drivers;
  int audio_rate;
  int audio_chanc;
  int audio_format;
  char *audio_device;
  
  char *synth_drivers;
  
  char *render_drivers;
  
  char *data_path; // guaranteed present after bigpc_config_ready
  char *log_path;
  char *savedgame_path; // per user. The one we actually use is (bigpc.savedgame_path).
  char *input_path;
  char *settings_path;
  int lang;
  char *gamemon_path;
  int gamemon_baud_rate;
};

extern struct bigpc {

  const char *exename;
  struct bigpc_config config;
  
  volatile int sigc;
  int aborted;
  
  struct bigpc_video_driver *video;
  struct bigpc_audio_driver *audio;
  struct bigpc_input_driver **inputv;
  int inputc,inputa;
  struct bigpc_synth_driver *synth;
  struct bigpc_render_driver *render;
  struct fmn_datafile *datafile;
  struct fmstore *fmstore;
  struct inmgr *inmgr;
  struct bigpc_clock clock;
  
  struct fmn_platform_settings settings;
  int settings_dirty;
  uint16_t *langv; // Built according to (datafile).
  int langc,langa;
  
  // Live input config state. Refreshed just before each input cycle.
  int incfg_status;
  uint16_t incfg_btnid;
  uint8_t incfg_p;
  int incfg_devid;
  
  uint8_t input_state;
  int devid_keyboard;
  
  uint16_t mapid;
  uint8_t saveto_recent; // last observed 'saveto', never zero
  struct bigpc_map_callback {
    uint8_t evid,param;
    uint16_t cbid;
  } *map_callbackv;
  int map_callbackc,map_callbacka;
  
  int pause_for_violin;
  
  struct bigpc_sound_blackout {
    uint16_t sfxid;
    int64_t realtime;
  } *sound_blackoutv;
  int sound_blackoutc,sound_blackouta;
  
  FILE *logfile;
  
  char *savedgame_path;
  void *savedgame_serial; // matches file
  int savedgame_serialc,savedgame_seriala;
  int savedgame_dirty; // nonzero to reencode and save
  int64_t savedgame_update_time; // timeout after setting dirty before committing. compare to bigpc.clock.last_real_time_us
  int savedgame_suppress; // nonzero after a reset, eg manually return to menu. to prevent unintentionally saving a wiped state.
  
  #if FMN_USE_gamemon
    struct gamemon *gamemon;
    int gamemon_clock;
    int gamemon_ready;
    uint8_t *gamemon_fb; // Always bgr332.
    int gamemon_fbw,gamemon_fbh;
  #endif
  
} bigpc;

void bigpc_config_cleanup(struct bigpc_config *config);
void bigpc_config_init();
int bigpc_configure_argv(int argc,char **argv);
int bigpc_configure_kv(const char *k,int kc,const char *v,int vc);
int bigpc_config_ready();
void bigpc_settings_init(); // Owned by config, but separate. Call after drivers and datafile are created.
void bigpc_settings_dirty();
void bigpc_settings_save_if_dirty();

void bigpc_signal_init();

int bigpc_video_init();

int bigpc_audio_init();

int bigpc_input_init();
int bigpc_save_input_config();

void bigpc_rebuild_language_list();

// Force a player's state to all ones, so each key needs to release before we notice the change.
void bigpc_ignore_next_button();

void bigpc_play_song(uint8_t songid,uint8_t loop);

// Nonzero if it's ok to play this sound. Updates all blackout state based on bigpc.clock.last_real_time_us.
int bigpc_check_sound_blackout(uint16_t sfxid);

int bigpc_log_init();

void bigpc_cb_close(struct bigpc_video_driver *driver);
void bigpc_cb_focus(struct bigpc_video_driver *driver,int focus);
void bigpc_cb_resize(struct bigpc_video_driver *driver,int w,int h);
int bigpc_cb_key(struct bigpc_video_driver *driver,int keycode,int value);
void bigpc_cb_text(struct bigpc_video_driver *driver,int codepoint);
void bigpc_cb_mmotion(struct bigpc_video_driver *driver,int x,int y);
void bigpc_cb_mbutton(struct bigpc_video_driver *driver,int btnid,int value);
void bigpc_cb_mwheel(struct bigpc_video_driver *driver,int dx,int dy);
void bigpc_cb_pcm_out(void *v,int c,struct bigpc_audio_driver *driver);
void bigpc_cb_connect(struct bigpc_input_driver *driver,int devid);
void bigpc_cb_disconnect(struct bigpc_input_driver *driver,int devid);
void bigpc_cb_event(struct bigpc_input_driver *driver,int devid,int btnid,int value);
void bigpc_cb_state_change(struct inmgr *inmgr,uint16_t btnid,uint8_t value,uint16_t state);
void bigpc_cb_action(struct inmgr *inmgr,uint16_t actionid);
void bigpc_cb_gamemon_connected(void *dummy);
void bigpc_cb_gamemon_disconnected(void *dummy);
void bigpc_cb_gamemon_fb_format(int w,int h,int pixfmt,void *dummy);
void bigpc_cb_gamemon_input(int state,int pv,void *dummy);

void bigpc_cap_screen();
void bigpc_gamemon_send_framebuffer(); // Also lives in bigpc_screencap.c, it's kind of similar.

void bigpc_savedgame_init();
void bigpc_savedgame_delete();
int bigpc_savedgame_validate(const void *src,int srcc); // idempotent
uint16_t bigpc_savedgame_load(const void *src,int srcc); // Validate first, otherwise we might fail midway.
void bigpc_savedgame_dirty(); // call liberally to set dirty flag and timeout if warranted
void bigpc_savedgame_update(); // call liberally to check dirty timeout and save when it expires

#endif
