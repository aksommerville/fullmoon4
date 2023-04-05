#include "bigpc_internal.h"
 
struct bigpc bigpc={
  .exename="fullmoon",
};

/* Quit.
 */
 
void bigpc_quit() {
  bigpc_audio_play(bigpc.audio,0);
  
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
}

/* Init.
 */
 
int bigpc_init(int argc,char **argv) {
  int err;
  if ((err=bigpc_configure_argv(argc,argv))<0) return err;
  if ((err=bigpc_config_ready())<0) return err;
  
  if (!bigpc.config.data_path) {
    //TODO Locate data file on our own.
    fprintf(stderr,"%s: Please indicate data file as '--data=PATH'\n",bigpc.exename);
    return -2;
  }
  if (!(bigpc.datafile=fmn_datafile_open(bigpc.config.data_path))) {
    fprintf(stderr,"%s: Failed to read data file.\n",bigpc.config.data_path);
    return -2;
  }
  
  bigpc_signal_init();
  if ((err=bigpc_video_init())<0) return err;
  if ((err=bigpc_audio_init())<0) return err;
  if ((err=bigpc_input_init())<0) return err;
  
  if (fmn_init()<0) {
    fprintf(stderr,"Error initializing game.\n");
    return -2;
  }
  
  bigpc_audio_play(bigpc.audio,1);
  return 0;
}

/* Current time in milliseconds. XXX need a smarter clock with a concept of "game time".
 */
 
#include <sys/time.h>

static uint32_t bigpc_now() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return tv.tv_sec*1000+tv.tv_usec/1000;
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
  
  // Update game.
  uint32_t now=bigpc_now();//TODO this is "active game time", but we're stubbing with absolute real time for now.
  uint8_t input_state=0;//TODO from input manager
  fmn_update(now,input_state);
  
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
