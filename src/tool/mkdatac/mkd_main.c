#include "mkd_internal.h"

struct mkd mkd={0};

#define _(tag) int mkd_main_##tag();
MKD_FOR_EACH_CMD
#undef _

static const char *mkd_cmd_repr(int cmd) {
  switch (cmd) {
    #define _(tag) case MKD_CMD_##tag: return #tag;
    MKD_FOR_EACH_CMD
    #undef _
  }
  return "?";
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err=mkd_config_argv(&mkd.config,argc,argv);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to evaluate command line.\n",mkd.config.exename);
    return 1;
  }
  if ((err=mkd_config_ready(&mkd.config))<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to finalize configuration.\n",mkd.config.exename);
    return 1;
  }
  switch (mkd.config.cmd) {
    #define _(tag) case MKD_CMD_##tag: err=mkd_main_##tag(); break;
    MKD_FOR_EACH_CMD
    #undef _
    default: fprintf(stderr,"%s: Command %d unset.\n",mkd.config.exename,mkd.config.cmd); return 1;
  }
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Error processing command '%s'\n",mkd.config.exename,mkd_cmd_repr(mkd.config.cmd));
    return 1;
  }
  sr_encoder_cleanup(&mkd.dst);
  mkd_config_cleanup(&mkd.config);
  return 0;
}
