#ifndef FIDDLE_INTERNAL_H
#define FIDDLE_INTERNAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "opt/http/http.h"
#include "opt/http/http_socket.h"
#include "opt/datafile/fmn_datafile.h"
#include "opt/alsa/alsapcm.h"
#include "opt/bigpc/bigpc_synth.h" /* bigpc isn't linked; headers only */
#include "opt/bigpc/bigpc_audio.h"
#include "tool/common/serial/serial.h"

extern struct fiddle {

  const char *exename;
  int port;
  char *htdocs;
  int htdocsc;
  char *data;
  int datac;
  
  struct http_context *http;
  volatile int sigc;
  struct fmn_datafile *datafile;
  struct bigpc_audio_driver *audio;
  struct bigpc_synth_driver *synth;
  int songid;
  int latest_soundid;
  
  struct http_socket **websocketv;
  int websocketc,websocketa;
  
  // Our audio callback populates this (usually in a separate thread).
  // When (samplec>0), main loop should deliver to websockets and clear it.
  int vumeter_samplec;
  int16_t vumeter_lo,vumeter_hi;
  float vumeter_sqsum; // sqrt(sqsum/samplec)=rms, we don't precalculate because we'll keep adding to it. range 0..1

} fiddle;

int fiddle_configure(int argc,char **argv);

int fiddle_httpcb_get_status(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_get_synths(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_get_sounds(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_get_instruments(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_get_songs(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_post_synth_use(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_post_sound_play(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_post_song_play(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_post_midi(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_get_root(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_static(struct http_xfer *req,struct http_xfer *rsp,void *userdata);
int fiddle_httpcb_nonesuch(struct http_xfer *req,struct http_xfer *rsp,void *userdata);

int fiddle_wscb_connect(struct http_socket *socket,void *userdata);
int fiddle_wscb_disconnect(struct http_socket *socket,void *userdata);
int fiddle_wscb_message(struct http_socket *socket,int type,const void *v,int c,void *userdata);

const char *fiddle_guess_mime_type(const char *path,const void *v,int c);

/* Spawn a process and trigger (cb) in the future when it completes, before returning.
 * (cmd) is space-delimited, no quoting or variables.
 * I'm using a callback instead of a return vector in order to be drop-in compatible with the correct async version of this.
 */
int fiddle_spawn_process_sync(const char *cmd,int (*cb)(int status,const char *log,int logc,void *userdata),void *userdata);

void fiddle_drivers_quit();
int fiddle_drivers_set(int qualifier);
int fiddle_drivers_play(int play);
int fiddle_drivers_update();
int fiddle_drivers_lock();
void fiddle_drivers_unlock();
void fiddle_synth_event(uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b);

#endif
