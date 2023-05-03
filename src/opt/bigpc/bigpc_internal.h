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
  
  char *input_drivers;
  
  char *audio_drivers;
  int audio_rate;
  int audio_chanc;
  int audio_format;
  
  char *synth_drivers;
  
  char *render_drivers;
  
  char *data_path;
  char *log_path;
};

extern struct bigpc {

  const char *exename;
  struct bigpc_config config;
  
  volatile int sigc;
  
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
  
  uint8_t input_state;
  int devid_keyboard;
  
  uint16_t mapid;
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
} bigpc;

void bigpc_config_cleanup(struct bigpc_config *config);
int bigpc_configure_argv(int argc,char **argv);
int bigpc_configure_kv(const char *k,int kc,const char *v,int vc);
int bigpc_config_ready();
int bigpc_config_guess_data_path();

void bigpc_signal_init();

int bigpc_video_init();

int bigpc_audio_init();

int bigpc_input_init();

// Force a player's state to all ones, so each key needs to release before we notice the change.
void bigpc_ignore_next_button();

void bigpc_play_song(uint8_t songid);

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
void bigpc_cb_state_change(void *userdata,uint8_t playerid,uint16_t btnid,uint8_t value,uint16_t state);
void bigpc_cb_action(void *userdata,uint8_t playerid,uint16_t actionid);

#endif
