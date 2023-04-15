#include "assist_internal.h"

/* Main.
 */
 
int main(int argc,char **argv) {

  const char *datapath=0;

  int argi=1; while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    
    if (!memcmp(arg,"--data=",7)) {
      datapath=arg+7;
      continue;
    }
    
    fprintf(stderr,"%s: Unexpected argument '%s'\n",argv[0],arg);
    return 1;
  }
  
  // What are we being asked to assist with?
  if (datapath) return assist_analyze_data(datapath);
  
  fprintf(stderr,"%s: No command\n",argv[0]);
  return 1;
}
