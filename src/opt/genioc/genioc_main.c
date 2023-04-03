#include "genioc_internal.h"

struct genioc genioc={0};

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  genioc_set_frame_rate(60);
  if ((argc=genioc_client_preprocess_argv(argc,argv))<0) {
    if (argc!=-2) fprintf(stderr,"Error processing arguments.\n");
    return 1;
  }
  if ((err=bigpc_init(argc,argv))<0) {
    if (err!=-2) fprintf(stderr,"Error initializing platform.\n");
    return 1;
  }
  genioc_clock_init();
  while (!genioc.quit) {
    genioc_clock_update();
    if ((err=bigpc_update())<0) {
      if (err!=-2) fprintf(stderr,"Error updating platform.\n");
      bigpc_quit();
      return 1;
    }
    if (!err) break;
  }
  genioc_clock_report();
  bigpc_quit();
  fprintf(stderr,"Normal exit.\n");
  return 0;
}

/* Request termination.
 */
 
void genioc_quit_soon() {
  genioc.quit=1;
}
