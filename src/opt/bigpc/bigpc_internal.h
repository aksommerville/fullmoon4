#ifndef BIGPC_INTERNAL_H
#define BIGPC_INTERNAL_H

#include "app/fmn_platform.h"
#include "bigpc.h"
#include "bigpc_video.h"
#include "bigpc_audio.h"
#include "bigpc_input.h"
#include "bigpc_synth.h"
#include "bigpc_render.h"
#include "opt/datafile/fmn_datafile.h"
#include <stdio.h>
#include <stdlib.h>

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
  
  uint8_t input_state;
  
} bigpc;

void bigpc_config_cleanup(struct bigpc_config *config);
int bigpc_configure_argv(int argc,char **argv);
int bigpc_configure_kv(const char *k,int kc,const char *v,int vc);
int bigpc_config_ready();

void bigpc_signal_init();

int bigpc_video_init();

int bigpc_audio_init();

int bigpc_input_init();

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

#endif
