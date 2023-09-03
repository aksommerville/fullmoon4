/* stdsyn_node_mixbuf.c
 * Mix input with a global buffer.
 */
 
#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_mixbuf {
  struct stdsyn_node hdr;
  uint8_t srcbufid;
  float mixa,mixb;
};

#define NODE ((struct stdsyn_node_mixbuf*)node)

/* Update.
 */
 
static void _mixbuf_update_00(float *v,int c,struct stdsyn_node *node) {
}

static void _mixbuf_update_ff(float *v,int c,struct stdsyn_node *node) {
  memcpy(v,stdsyn_node_get_buffer(node,NODE->srcbufid),sizeof(float)*c);
}

static void _mixbuf_update_mix(float *v,int c,struct stdsyn_node *node) {
  const float *src=stdsyn_node_get_buffer(node,NODE->srcbufid);
  for (;c-->0;v++,src++) *v=(*v)*NODE->mixa+(*src)*NODE->mixb;
}

static void _mixbuf_update_add(float *v,int c,struct stdsyn_node *node) {
  const float *src=stdsyn_node_get_buffer(node,NODE->srcbufid);
  for (;c-->0;v++,src++) (*v)+=*src;
}

/* Init.
 */
 
static int _mixbuf_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  const uint8_t *ARGV=argv;
  if (node->chanc!=1) return -1;
  if (argc<1) return -1;
  if (ARGV[0]>=STDSYN_BUFFER_COUNT) return -1;
  NODE->srcbufid=ARGV[0];
  if (argc<2) node->update=_mixbuf_update_add;
  else if (ARGV[1]==0xff) node->update=_mixbuf_update_ff;
  else if (!ARGV[1]) node->update=_mixbuf_update_00;
  else {
    node->update=_mixbuf_update_mix;
    NODE->mixa=ARGV[1]/255.0f;
    NODE->mixb=1.0f-NODE->mixa;
  }
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_mixbuf={
  .name="mixbuf",
  .objlen=sizeof(struct stdsyn_node_mixbuf),
  .init=_mixbuf_init,
};
