#include "fmn_stdsyn_internal.h"

/* Delete.
 */

void stdsyn_node_del(struct stdsyn_node *node) {
  if (!node) return;
  if (node->refc-->1) return;
  if (node->type->del) node->type->del(node);
  if (node->srcv) {
    while (node->srcc-->0) stdsyn_node_del(node->srcv[node->srcc]);
    free(node->srcv);
  }
  free(node);
}

/* Retain.
 */
 
int stdsyn_node_ref(struct stdsyn_node *node) {
  if (!node) return -1;
  if (node->refc<1) return -1;
  if (node->refc==INT_MAX) return -1;
  node->refc++;
  return 0;
}

/* New.
 */

struct stdsyn_node *stdsyn_node_new(
  struct bigpc_synth_driver *driver,
  const struct stdsyn_node_type *type,
  int chanc,int overwrite,
  uint8_t noteid,uint8_t velocity
) {
  if (!driver||!type) return 0;
  struct stdsyn_node *node=calloc(1,type->objlen);
  if (!node) return 0;
  node->driver=driver;
  node->type=type;
  node->refc=1;
  node->chanc=chanc;
  node->overwrite=overwrite;
  node->chid=0xff;
  node->noteid=noteid;
  if (type->init&&(type->init(node,velocity)<0)) {
    stdsyn_node_del(node);
    return 0;
  }
  return node;
}

/* Spawn.
 */

struct stdsyn_node *stdsyn_node_spawn_source(
  struct stdsyn_node *parent,
  const struct stdsyn_node_type *type,
  int chanc,int overwrite,
  uint8_t noteid,uint8_t velocity
) {
  if (!parent) return 0;
  struct stdsyn_node *child=stdsyn_node_new(parent->driver,type,chanc,overwrite,noteid,velocity);
  if (!child) return 0;
  child->chid=parent->chid;
  int err=stdsyn_node_srcv_insert(parent,-1,child);
  stdsyn_node_del(child);
  if (err<0) return 0;
  return child;
}

/* srcv
 */

int stdsyn_node_srcv_insert(struct stdsyn_node *parent,int p,struct stdsyn_node *child) {
  if (!parent||!child) return -1;
  if (p>parent->srcc) return -1;
  if (parent->srcc>=parent->srca) {
    int na=parent->srca+4;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(parent->srcv,sizeof(void*)*na);
    if (!nv) return -1;
    parent->srcv=nv;
    parent->srca=na;
  }
  if (p<0) p=parent->srcc;
  if (stdsyn_node_ref(child)<0) return -1;
  memmove(parent->srcv+p+1,parent->srcv+p,sizeof(void*)*(parent->srcc-p));
  parent->srcc++;
  parent->srcv[p]=child;
  return 0;
}

int stdsyn_node_srcv_remove_at(struct stdsyn_node *parent,int p,int c) {
  if (!c) return 0;
  if ((p<0)||(p>=parent->srcc)||(c<0)||(p>parent->srcc-c)) return -1;
  int i=c;
  while (i-->0) stdsyn_node_del(parent->srcv[p+i]);
  parent->srcc-=c;
  memmove(parent->srcv+p,parent->srcv+p+c,sizeof(void*)*(parent->srcc-p));
  return 0;
}
  
int stdsyn_node_srcv_remove(struct stdsyn_node *parent,struct stdsyn_node *child) {
  if (!parent||!child) return -1;
  int i=parent->srcc;
  while (i-->0) if (parent->srcv[i]==child) return stdsyn_node_srcv_remove_at(parent,i,1);
  return -1;
}

/* New node from encoded controller.
 */

struct stdsyn_node *stdsyn_node_new_controller(
  struct bigpc_synth_driver *driver,
  int chanc,int overwrite,
  const struct stdsyn_instrument *ins
) {
  if (!ins||!ins->type||!ins->type->apply_instrument) return 0;
  struct stdsyn_node *node=stdsyn_node_new(driver,ins->type,chanc,overwrite,0x40,0x40);
  if (!node) return 0;
  if (node->type->apply_instrument(node,ins)<0) {
    fprintf(stderr,"stdsyn: Failed to apply instrument to '%s' node.\n",node->type->name);
    stdsyn_node_del(node);
    return 0;
  }
  
  // If we're calling it "controller", it must have the "event" hook. Others are optional.
  if (!node->event) {
    stdsyn_node_del(node);
    return 0;
  }
  return node;
}
