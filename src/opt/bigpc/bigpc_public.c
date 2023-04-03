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
#include <GL/gl.h>//XXX
 
int bigpc_update() {
  if (bigpc.sigc) return 0;
  
  if (bigpc_video_driver_update(bigpc.video)<0) return -1;
  if (bigpc_audio_update(bigpc.audio)<0) return -1;
  int i=bigpc.inputc;
  while (i-->0) {
    if (bigpc_input_driver_update(bigpc.inputv[i])<0) return -1;
  }
  
  //TODO call out to game
  //TODO render frame
 
  //XXX TEMP try rendering
  if (bigpc_video_driver_begin_gx(bigpc.video)<0) return -1;
  glViewport(0,0,bigpc.video->w,bigpc.video->h);
  glClearColor(0.5f,0.25f,0.0f,1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glBegin(GL_TRIANGLES);
    glColor4ub(0xff,0x00,0x00,0xff); glVertex2f( 0.0f, 0.5f);
    glColor4ub(0x00,0xff,0x00,0xff); glVertex2f(-0.5f,-0.5f);
    glColor4ub(0x00,0x00,0xff,0xff); glVertex2f( 0.5f,-0.5f);
  glEnd();
  bigpc_video_driver_end(bigpc.video);
  
  return bigpc.sigc?0:1;
}
