#include "bigpc_internal.h"

//TODO

void bigpc_cb_close(struct bigpc_video_driver *driver) {
}

void bigpc_cb_focus(struct bigpc_video_driver *driver,int focus) {
}

void bigpc_cb_resize(struct bigpc_video_driver *driver,int w,int h) {
}

int bigpc_cb_key(struct bigpc_video_driver *driver,int keycode,int value) {
  return 0;
}

void bigpc_cb_text(struct bigpc_video_driver *driver,int codepoint) {
}

void bigpc_cb_mmotion(struct bigpc_video_driver *driver,int x,int y) {
}

void bigpc_cb_mbutton(struct bigpc_video_driver *driver,int btnid,int value) {
}

void bigpc_cb_mwheel(struct bigpc_video_driver *driver,int dx,int dy) {
}

void bigpc_cb_pcm_out(void *v,int c,struct bigpc_audio_driver *driver) {
  int samplesize;
  switch (driver->format) {
    case BIGPC_AUDIO_FORMAT_s16n: samplesize=2; break;
    case BIGPC_AUDIO_FORMAT_f32n: samplesize=4; break;
    default: return;
  }
  memset(v,0,c*samplesize);
}

void bigpc_cb_connect(struct bigpc_input_driver *driver,int devid) {
}

void bigpc_cb_disconnect(struct bigpc_input_driver *driver,int devid) {
}

void bigpc_cb_event(struct bigpc_input_driver *driver,int devid,int btnid,int value) {
}
