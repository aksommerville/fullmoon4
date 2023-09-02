#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_pcm {
  struct stdsyn_node hdr;
  struct stdsyn_pcm *pcm;
  int p;
};

#define NODE ((struct stdsyn_node_pcm*)node)

/* Cleanup.
 */
 
static void _pcm_del(struct stdsyn_node *node) {
  stdsyn_pcm_del(NODE->pcm);
}

/* Update.
 * Only the "add" versions will currently be used.
 * Overwrite is the simpler case, so I figured why not.
 * TODO Do we want gain? Would be trivial to implement right here.
 */
 
static void _pcm_update_mono_overwrite(float *v,int c,struct stdsyn_node *node) {
  if (!NODE->pcm||(NODE->p>=NODE->pcm->c)) {
    node->defunct=1;
    memset(v,0,sizeof(float)*c);
    return;
  }
  int cpc=NODE->pcm->c-NODE->p;
  if (cpc>c) cpc=c;
  memcpy(v,NODE->pcm->v+NODE->p,sizeof(float)*cpc);
  NODE->p+=cpc;
  if (cpc<c) memset(v+cpc,0,sizeof(float)*(c-cpc));
}
 
static void _pcm_update_mono_add(float *v,int c,struct stdsyn_node *node) {
  if (!NODE->pcm||(NODE->p>=NODE->pcm->c)) {
    node->defunct=1;
    return;
  }
  int cpc=NODE->pcm->c-NODE->p;
  if (cpc>c) cpc=c;
  float *dst=v;
  const float *src=NODE->pcm->v+NODE->p;
  int i=cpc;
  for (;i-->0;dst++,src++) (*dst)+=(*src);
  NODE->p+=cpc;
}
 
static void _pcm_update_stereo_overwrite(float *v,int c,struct stdsyn_node *node) {
  if (!NODE->pcm||(NODE->p>=NODE->pcm->c)) {
    node->defunct=1;
    memset(v,0,sizeof(float)*c);
    return;
  }
  int totalframec=c>>1;
  int cpframec=NODE->pcm->c-NODE->p;
  if (cpframec>totalframec) cpframec=totalframec;
  float *dst=v;
  const float *src=NODE->pcm->v+NODE->p;
  int i=cpframec;
  for (;i-->0;dst+=2,src++) {
    //TODO Do we want pan control? Maybe let that be mixer's problem. In which case we should not claim to support stereo.
    dst[0]=*src;
    dst[1]=*src;
  }
  NODE->p+=cpframec;
  if (cpframec<totalframec) memset(v+(cpframec<<1),0,sizeof(float)*((totalframec-cpframec)<<1));
}
 
static void _pcm_update_stereo_add(float *v,int c,struct stdsyn_node *node) {
  if (!NODE->pcm||(NODE->p>=NODE->pcm->c)) {
    node->defunct=1;
    return;
  }
  int totalframec=c>>1;
  int cpframec=NODE->pcm->c-NODE->p;
  if (cpframec>totalframec) cpframec=totalframec;
  float *dst=v;
  const float *src=NODE->pcm->v+NODE->p;
  int i=cpframec;
  for (;i-->0;dst+=2,src++) {
    //TODO Do we want pan control? Maybe let that be mixer's problem. In which case we should not claim to support stereo.
    dst[0]+=*src;
    dst[1]+=*src;
  }
  NODE->p+=cpframec;
}

/* Init.
 */
 
static int _pcm_init(struct stdsyn_node *node,uint8_t velocity) {
  if (node->chanc==1) {
    if (node->overwrite) node->update=_pcm_update_mono_overwrite;
    else node->update=_pcm_update_mono_add;
  } else if (node->chanc==2) {
    if (node->overwrite) node->update=_pcm_update_stereo_overwrite;
    else node->update=_pcm_update_stereo_add;
  } else return -1;
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_pcm={
  .name="pcm",
  .objlen=sizeof(struct stdsyn_node_pcm),
  .del=_pcm_del,
  .init=_pcm_init,
};

/* Set PCM.
 */
 
int stdsyn_node_pcm_set_pcm(struct stdsyn_node *node,struct stdsyn_pcm *pcm) {
  if (!node||(node->type!=&stdsyn_node_type_pcm)) return -1;
  if (NODE->pcm==pcm) return 0;
  if (pcm&&(stdsyn_pcm_ref(pcm)<0)) return -1;
  stdsyn_pcm_del(NODE->pcm);
  NODE->pcm=pcm;
  NODE->p=0;
  return 0;
}
