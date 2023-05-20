#include "fiddle_internal.h"
#include <signal.h>
#include <unistd.h>
#include <sys/poll.h>

struct http_xfer *fiddle_held_xfer=0;//XXX

struct fiddle fiddle={0};

/* Cleanup.
 */
 
static void fiddle_cleanup() {

  http_context_del(fiddle.http);

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

/* Main.
 */
 
int main(int argc,char **argv) {
  fiddle.exename=((argc>=1)&&argv&&argv[0]&&argv[0][0])?argv[0]:"fiddle";
  fiddle.port=6666;
  
  signal(SIGINT,fiddle_cb_signal);
  
  if (!(fiddle.http=http_context_new())) {
    fprintf(stderr,"%s: http_context_new() failed\n",fiddle.exename);
    return 1;
  }
  
  if (
    !http_serve(fiddle.http,fiddle.port)||
    !http_listen(fiddle.http,0,"/**",fiddle_httpcb_default,0)
  ) {
    fprintf(stderr,"%s: Failed to initialize HTTP server on port %d.\n",fiddle.exename,fiddle.port);
    fiddle_cleanup();
    return 1;
  }
  fprintf(stderr,"%s: Serving on port %d.\n",fiddle.exename,fiddle.port);
  
  while (!fiddle.sigc) {
    if (http_update(fiddle.http,1000)<0) {
      fprintf(stderr,"%s: Error updating HTTP context.\n",fiddle.exename);
      fiddle_cleanup();
      return 1;
    }
    if (!http_context_get_server_by_index(fiddle.http,0)) {
      fprintf(stderr,"%s: Abort due to server removal.\n",fiddle.exename);
      fiddle_cleanup();
      return 1;
    }
    
    //XXX
    if (fiddle_held_xfer) {
      struct pollfd pollfd={.fd=STDIN_FILENO,.events=POLLIN};
      if (poll(&pollfd,1,0)>0) {
        char msg[256];
        int msgc=read(STDIN_FILENO,msg,sizeof(msg));
        http_xfer_set_body(fiddle_held_xfer,msg,msgc);
        http_xfer_set_header(fiddle_held_xfer,"Content-Type",12,"text/plain",-1);
        http_xfer_set_status(fiddle_held_xfer,200,0);
        http_xfer_ready(fiddle_held_xfer);
        http_xfer_del(fiddle_held_xfer);
        fiddle_held_xfer=0;
      }
    }
  }
  
  fiddle_cleanup();
  fprintf(stderr,"%s: Normal exit.\n",fiddle.exename);
  return 0;
}
