#include "fiddle_internal.h"
#include <signal.h>
#include <unistd.h>

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
    !http_listen(fiddle.http,0,"/*",fiddle_httpcb_default,0)
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
  }
  
  fiddle_cleanup();
  fprintf(stderr,"%s: Normal exit.\n",fiddle.exename);
  return 0;
}
