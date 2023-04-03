#include "bigpc_internal.h"
 
struct bigpc bigpc={
  .exename="fullmoon",
};

/* Quit.
 */
 
void bigpc_quit() {
  fmn_log("%s",__func__);
  bigpc_audio_play(bigpc.audio,0);
  
  bigpc_video_driver_del(bigpc.video);
  bigpc_audio_driver_del(bigpc.audio);
  if (bigpc.inputv) {
    while (bigpc.inputc-->0) {
      bigpc_input_driver_del(bigpc.inputv[bigpc.inputc]);
    }
    free(bigpc.inputv);
  }
}

/* Init.
 */
 
int bigpc_init(int argc,char **argv) {
  fmn_log("%s",__func__);
  int err;
  if ((err=bigpc_configure_argv(argc,argv))<0) return err;
  if ((err=bigpc_config_ready())<0) return err;
  
  bigpc_signal_init();
  if ((err=bigpc_video_init())<0) return err;
  if ((err=bigpc_audio_init())<0) return err;
  if ((err=bigpc_input_init())<0) return err;
  
  bigpc_audio_play(bigpc.audio,1);
  return 0;
}

/* Update.
 */
 
int bigpc_update() {
  if (bigpc.sigc) return 0;
  //TODO
  return 1;
}
