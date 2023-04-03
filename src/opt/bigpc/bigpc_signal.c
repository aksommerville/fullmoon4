#include "bigpc_internal.h"
#include <signal.h>
#include <stdlib.h>

/* Signal handler.
 */
 
static void bigpc_cb_signal(int sigid) {
  switch (sigid) {
    case SIGINT: if (++(bigpc.sigc)>=3) {
        fprintf(stderr,"%s: Too many unprocessed signals.\n",bigpc.exename);
        exit(1);
      } break;
  }
}

/* Init signal handlers.
 */
 
void bigpc_signal_init() {
  signal(SIGINT,bigpc_cb_signal);
}
