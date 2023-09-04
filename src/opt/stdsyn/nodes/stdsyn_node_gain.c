#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_gain {
  struct stdsyn_node hdr;
  float gain,clip,gate;
};

#define NODE ((struct stdsyn_node_gain*)node)

/* Update.
 */
 
static void _gain_update(float *v,int c,struct stdsyn_node *node) {
  float nclip=-NODE->clip;
  float ngate=-NODE->gate;
  for (;c-->0;v++) {
    (*v)*=NODE->gain;
    if (*v<nclip) *v=nclip;
    else if (*v>NODE->clip) *v=NODE->clip;
    else if (*v<ngate) ;
    else if (*v>NODE->gate) ;
    else *v=0.0f;
  }
}

/* Init.
 */
 
static int _gain_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  const uint8_t *ARGV=argv;
  if (node->overwrite) return -1;
  if (argc!=4) return -1;
  NODE->gain=((ARGV[0]<<8)|ARGV[1])/256.0f;
  NODE->clip=ARGV[2]/255.0f;
  NODE->gate=ARGV[3]/255.0f;
  node->update=_gain_update;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_gain={
  .name="gain",
  .objlen=sizeof(struct stdsyn_node_gain),
  .init=_gain_init,
};
