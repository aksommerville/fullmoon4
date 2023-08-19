#include "opt/genioc/genioc.h"
#include <stdio.h>

/* Preprocess argv.
 */
 
int genioc_client_preprocess_argv(int argc,char **argv) {
  setbuf(stderr,NULL); // Please print stderr immediately. Why on earth would you not?
  return argc;
}
