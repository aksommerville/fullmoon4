#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_env {
  struct stdsyn_node hdr;
  struct stdsyn_env env;
};

#define NODE ((struct stdsyn_node_env*)node)

/* Update.
 */
 
static void _env_update(float *v,int c,struct stdsyn_node *node) {
  for (;c-->0;v++) {
    float level=stdsyn_env_update(NODE->env);
    (*v)*=level;
  }
  if (NODE->env.finished&&!node->defunct) {
    node->defunct=1;
  }
}

/* Release.
 */
 
static void _env_release(struct stdsyn_node *node,uint8_t velocity) {
  stdsyn_env_release(&NODE->env);
}

/* Init.
 */
 
static int _env_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  if (node->chanc!=1) return -1;
  if (node->overwrite) return -1;
  if (stdsyn_env_decode(&NODE->env,argv,argc)<0) return -1;
  stdsyn_env_reset(&NODE->env,velocity,node->driver->rate);
  node->update=_env_update;
  node->release=_env_release;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_env={
  .name="env",
  .objlen=sizeof(struct stdsyn_node_env),
  .init=_env_init,
};
