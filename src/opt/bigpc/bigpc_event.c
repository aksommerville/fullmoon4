#include "bigpc_internal.h"

/* Extra window manager events.
 * Not very interesting.
 */

void bigpc_cb_close(struct bigpc_video_driver *driver) {
  bigpc.sigc++;
}

void bigpc_cb_focus(struct bigpc_video_driver *driver,int focus) {
}

void bigpc_cb_resize(struct bigpc_video_driver *driver,int w,int h) {
}

/* Raw keyboard events from window manager.
 * Try sending to inmgr.
 */

int bigpc_cb_key(struct bigpc_video_driver *driver,int keycode,int value) {
  // For now, inmgr is not reporting whether an event was mapped.
  // Would be easy enough to do, if we ever need plain text input.
  inmgr_event(bigpc.inmgr,bigpc.devid_keyboard,keycode,value);
  return 1;
}

/* Text and mouse events from window manager.
 * I don't expect to use these, but maybe in the future, for menus and such.
 */

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

/* Audio driver ready for output.
 * Fetch from synth.
 */

void bigpc_cb_pcm_out(void *v,int c,struct bigpc_audio_driver *driver) {
  bigpc.synth->update(v,c,bigpc.synth);
}

/* Raw input events, from input drivers.
 * Delegate to inmgr.
 */

static int bigpc_cb_report_button(int btnid,int usage,int lo,int hi,int value,void *userdata) {
  //fprintf(stderr,"  id=%08x usage=%08x range=%d..%d value=%d\n",btnid,usage,lo,hi,value);
  int devid=*(int*)userdata;
  inmgr_add_capability(bigpc.inmgr,btnid,usage,lo,hi,value);
  return 0;
}

void bigpc_cb_connect(struct bigpc_input_driver *driver,int devid) {
  uint16_t vid=0,pid=0;
  const char *name=bigpc_input_device_get_ids(&vid,&pid,driver,devid);
  //fprintf(stderr,"%s %d %04x:%04x '%s'\n",__func__,devid,vid,pid,name);
  inmgr_connect(bigpc.inmgr,devid,name,-1,vid,pid);
  bigpc_input_device_for_each_button(driver,devid,bigpc_cb_report_button,&devid);
  if (inmgr_ready(bigpc.inmgr)>0) {
    bigpc_save_input_config();
  }
}

void bigpc_cb_disconnect(struct bigpc_input_driver *driver,int devid) {
  //fprintf(stderr,"%s %d\n",__func__,devid);
  inmgr_disconnect(bigpc.inmgr,devid);
}

void bigpc_cb_event(struct bigpc_input_driver *driver,int devid,int btnid,int value) {
  inmgr_event(bigpc.inmgr,devid,btnid,value);
}

/* Digested input events from inmgr.
 */

void bigpc_cb_state_change(struct inmgr *inmgr,uint16_t btnid,uint8_t value,uint16_t state) {
  //fprintf(stderr,"%s %d.%04x=%d [%04x]\n",__func__,playerid,btnid,value,state);
  //if (playerid) return; // For now, we're strictly one-player. (TODO I'd like at least a secret two-player easter egg eventually)
  if (value) bigpc.input_state|=btnid;
  else bigpc.input_state&=~btnid;
}

void bigpc_cb_action(struct inmgr *inmgr,uint16_t actionid) {
  //fprintf(stderr,"%s %d %d\n",__func__,playerid,actionid);
  switch (actionid) {
    case BIGPC_ACTIONID_quit: bigpc.sigc++; break;
    case BIGPC_ACTIONID_fullscreen: bigpc_video_set_fullscreen(bigpc.video,bigpc.video->fullscreen?0:1); break;
    case BIGPC_ACTIONID_pause: break;//TODO hard pause/resume
    case BIGPC_ACTIONID_step: break;//TODO step frame, when paused
    case BIGPC_ACTIONID_screencap: bigpc_cap_screen(); break;
    case BIGPC_ACTIONID_save: break;//TODO
    case BIGPC_ACTIONID_restore: break;//TODO
    case BIGPC_ACTIONID_mainmenu: break;//TODO
  }
}

/* Cheat input state.
 */

void bigpc_ignore_next_button() {
  inmgr_force_state(bigpc.inmgr,bigpc.input_state|(FMN_INPUT_USE|FMN_INPUT_MENU));
  bigpc.input_state=0;
}

/* Gamemon callbacks.
 */
#if FMN_USE_gamemon

void bigpc_cb_gamemon_connected(void *dummy) {
  fprintf(stderr,"%s\n",__func__);
  if (!bigpc.gamemon) return;
}

void bigpc_cb_gamemon_disconnected(void *dummy) {
  fprintf(stderr,"%s\n",__func__);
  if (!bigpc.gamemon) return;
  bigpc.gamemon_ready=0;
}

void bigpc_cb_gamemon_fb_format(int w,int h,int pixfmt,void *dummy) {
  fprintf(stderr,"%s %dx%d 0x%02x\n",__func__,w,h,pixfmt);
  if (!bigpc.gamemon) return;
  bigpc.gamemon_ready=1;
  bigpc.gamemon_clock=0;
}

void bigpc_cb_gamemon_input(int state,int pv,void *dummy) {
  //fprintf(stderr,"%s 0x%02x\n",__func__,state);
  if (!bigpc.gamemon) return;
  #define BTN(gm,fm) \
    if ((state&GAMEMON_INPUT_##gm)&&!(pv&GAMEMON_INPUT_##gm)) bigpc.input_state|=FMN_INPUT_##fm; \
    else if (!(state&GAMEMON_INPUT_##gm)&&(pv&GAMEMON_INPUT_##gm)) bigpc.input_state&=~FMN_INPUT_##fm;
  BTN(LEFT,LEFT)
  BTN(RIGHT,RIGHT)
  BTN(UP,UP)
  BTN(DOWN,DOWN)
  BTN(A,USE)
  BTN(B,MENU)
  #undef BTN
}

#endif
