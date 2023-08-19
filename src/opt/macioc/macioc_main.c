#include "macioc.h"
#include "opt/bigpc/bigpc.h"
#include <stdio.h>

/* Globals.
 */

static char **gargv=0;
static int gargc=0;

/* Quit.
 */

static void cb_quit(void *userdata) {
  bigpc_quit();
}

/* Init.
 */

static int cb_init(void *userdata) {
  if (bigpc_init(gargc,gargv)<0) return -1;
  return 0;
}

/* Focus.
 */

static void cb_focus(void *userdata,int focus) {
}

/* Update.
 */

static void cb_update(void *userdata) {
  int err=bigpc_update();
  if (err<0) {
    macioc_terminate(1);
  } else if (!err) {
    macioc_terminate(0);
  }
}

/* Main.
 */

int main(int argc,char **argv) {
  gargc=argc;
  gargv=argv;
  struct macioc_delegate delegate={
    .rate=60,
    .focus=cb_focus,
    .init=cb_init,
    .quit=cb_quit,
    .update=cb_update,
  };
  return macioc_main(argc,argv,&delegate);
}
