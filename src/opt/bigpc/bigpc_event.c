#include "bigpc_internal.h"

//TODO

void bigpc_cb_close(struct bigpc_video_driver *driver) {
  //fprintf(stderr,"%s\n",__func__);
  bigpc.sigc++;
}

void bigpc_cb_focus(struct bigpc_video_driver *driver,int focus) {
  //fprintf(stderr,"%s %d\n",__func__,focus);
}

void bigpc_cb_resize(struct bigpc_video_driver *driver,int w,int h) {
  //fprintf(stderr,"%s %d,%d\n",__func__,w,h);
}

int bigpc_cb_key(struct bigpc_video_driver *driver,int keycode,int value) {
  //fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);
  return 0;
}

void bigpc_cb_text(struct bigpc_video_driver *driver,int codepoint) {
  //fprintf(stderr,"%s U+%x\n",__func__,codepoint);
}

void bigpc_cb_mmotion(struct bigpc_video_driver *driver,int x,int y) {
  //fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

void bigpc_cb_mbutton(struct bigpc_video_driver *driver,int btnid,int value) {
  //fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

void bigpc_cb_mwheel(struct bigpc_video_driver *driver,int dx,int dy) {
  //fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
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

static int bigpc_cb_report_button(int btnid,int usage,int lo,int hi,int value,void *userdata) {
  fprintf(stderr,"  id=%08x usage=%08x range=%d..%d value=%d\n",btnid,usage,lo,hi,value);
  return 0;
}

void bigpc_cb_connect(struct bigpc_input_driver *driver,int devid) {
  uint16_t vid=0,pid=0;
  const char *name=bigpc_input_device_get_ids(&vid,&pid,driver,devid);
  fprintf(stderr,"%s %d %04x:%04x '%s'\n",__func__,devid,vid,pid,name);
  //bigpc_input_device_for_each_button(driver,devid,bigpc_cb_report_button,0);
}

void bigpc_cb_disconnect(struct bigpc_input_driver *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

void bigpc_cb_event(struct bigpc_input_driver *driver,int devid,int btnid,int value) {
  //fprintf(stderr,"%s %d.%d=%d\n",__func__,devid,btnid,value);
}
