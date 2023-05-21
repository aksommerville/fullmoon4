#include "fiddle_internal.h"
#include <signal.h>
#include <unistd.h>
#include <math.h>

struct fiddle fiddle={0};

/* Cleanup.
 */
 
static void fiddle_cleanup() {

  fiddle_drivers_quit();
  
  if (fiddle.websocketv) {
    while (fiddle.websocketc-->0) http_socket_del(fiddle.websocketv[fiddle.websocketc]);
    free(fiddle.websocketv);
  }

  http_context_del(fiddle.http);
  fmn_datafile_del(fiddle.datafile);
  if (fiddle.htdocs) free(fiddle.htdocs);
  if (fiddle.data) free(fiddle.data);

  const char *exename=fiddle.exename;
  memset(&fiddle,0,sizeof(struct fiddle));
  fiddle.exename=exename;
}

/* Signal.
 */
 
static void fiddle_cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++(fiddle.sigc)>=3) {
        fprintf(stderr,"%s: Too many unprocessed signals.\n",fiddle.exename);
        exit(1);
      } break;
  }
}

/* VU meter.
 */
 
static void fiddle_vumeter_update() {
  if (!fiddle.vumeter_samplec) return;
  if (!fiddle.websocketc) return; // let the meter run forever if nobody's connected, whatever.
  if (fiddle_drivers_lock()<0) return;
  
  struct sr_encoder msg={0};
  if (fiddle.audio) {
    sr_encode_json_object_start(&msg,0,0);
    sr_encode_json_string(&msg,"type",4,"vu",2);
    sr_encode_json_double(&msg,"dur",3,fiddle.vumeter_samplec/(double)(fiddle.audio->rate*fiddle.audio->chanc));
    sr_encode_json_int(&msg,"lo",2,fiddle.vumeter_lo);
    sr_encode_json_int(&msg,"hi",2,fiddle.vumeter_hi);
    sr_encode_json_double(&msg,"rms",3,sqrt(fiddle.vumeter_sqsum/fiddle.vumeter_samplec));
    sr_encode_json_object_end(&msg,0);
  }
    
  fiddle.vumeter_lo=fiddle.vumeter_hi=0;
  fiddle.vumeter_sqsum=0.0f;
  fiddle.vumeter_samplec=0;

  fiddle_drivers_unlock();

  if (msg.c) {
    int i=fiddle.websocketc;
    while (i-->0) {
      http_websocket_send(fiddle.websocketv[i],1,msg.v,msg.c);
    }
  }
  sr_encoder_cleanup(&msg);
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err=fiddle_configure(argc,argv);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"fiddle: Unspecified error reading configuration.\n");
    return 1;
  }
  
  signal(SIGINT,fiddle_cb_signal);
  
  if (fiddle.datac) {
    if (!(fiddle.datafile=fmn_datafile_open(fiddle.data))) {
      fprintf(stderr,"%s: Failed to open data file.\n",fiddle.data);
      return 1;
    }
  }
  
  if (!(fiddle.http=http_context_new())) {
    fprintf(stderr,"%s: http_context_new() failed\n",fiddle.exename);
    return 1;
  }
  
  if (
    !http_serve(fiddle.http,fiddle.port)||
    
    #define SERVE(method,path,cb) !http_listen(fiddle.http,HTTP_METHOD_##method,path,cb,0)||
    #define ALLMETH(path,cb) !http_listen(fiddle.http,0,path,cb,0)||
    !http_listen_websocket(fiddle.http,"/websocket",fiddle_wscb_connect,fiddle_wscb_disconnect,fiddle_wscb_message,0)||
    SERVE(GET, "/api/status",fiddle_httpcb_get_status)
    SERVE(GET, "/api/synths",fiddle_httpcb_get_synths)
    SERVE(GET, "/api/sounds",fiddle_httpcb_get_sounds)
    SERVE(GET, "/api/instruments",fiddle_httpcb_get_instruments)
    SERVE(GET, "/api/songs",fiddle_httpcb_get_songs)
    SERVE(POST,"/api/synth/use",fiddle_httpcb_post_synth_use)
    SERVE(POST,"/api/sound/play",fiddle_httpcb_post_sound_play)
    SERVE(POST,"/api/song/play",fiddle_httpcb_post_song_play)
    SERVE(POST,"/api/midi",fiddle_httpcb_post_midi)
    ALLMETH(   "/api/**",fiddle_httpcb_nonesuch)
    SERVE(GET, "/",fiddle_httpcb_get_root)
    SERVE(GET, "/**",fiddle_httpcb_static)
    ALLMETH(   "/**",fiddle_httpcb_nonesuch)
    #undef SERVE
    #undef ALLMETH
    
  0) {
    fprintf(stderr,"%s: Failed to initialize HTTP server on port %d.\n",fiddle.exename,fiddle.port);
    fiddle_cleanup();
    return 1;
  }
  fprintf(stderr,"%s: Serving on port %d.\n",fiddle.exename,fiddle.port);
  
  while (!fiddle.sigc) {
    if (http_update(fiddle.http,100)<0) {
      fprintf(stderr,"%s: Error updating HTTP context.\n",fiddle.exename);
      fiddle_cleanup();
      return 1;
    }
    if (!http_context_get_server_by_index(fiddle.http,0)) {
      fprintf(stderr,"%s: Abort due to server removal.\n",fiddle.exename);
      fiddle_cleanup();
      return 1;
    }
    if (fiddle_drivers_update()<0) {
      fprintf(stderr,"%s: Error updating drivers. Dropping PCM and synth.\n",fiddle.exename);
      fiddle_drivers_set(0);
    }
    fiddle_vumeter_update();
  }
  
  fiddle_cleanup();
  fprintf(stderr,"%s: Normal exit.\n",fiddle.exename);
  return 0;
}
