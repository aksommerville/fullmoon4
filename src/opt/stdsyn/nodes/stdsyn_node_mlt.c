#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_mlt {
  struct stdsyn_node hdr;
  float v;
};

#define NODE ((struct stdsyn_node_mlt*)node)

/* Update.
 */
 
static void _mlt_update(float *v,int c,struct stdsyn_node *node) {
  for (;c-->0;v++) (*v)*=NODE->v;
}

/* Init.
 */
 
static int _mlt_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  const uint8_t *ARGV=argv;
  if (node->overwrite) return -1;
  if (node->chanc!=1) return -1;
  if (argc!=2) return -1;
  NODE->v=((ARGV[0]<<8)|ARGV[1])/256.0f;
  node->update=_mlt_update;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_mlt={
  .name="mlt",
  .objlen=sizeof(struct stdsyn_node_mlt),
  .init=_mlt_init,
};
