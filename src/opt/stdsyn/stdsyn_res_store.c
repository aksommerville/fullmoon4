#include "fmn_stdsyn_internal.h"

/* Cleanup.
 */
 
void stdsyn_res_store_cleanup(struct stdsyn_res_store *store) {
  if (store->resv) {
    if (store->del) {
      struct stdsyn_res *res=store->resv;
      int i=store->resc;
      for (;i-->0;res++) {
        if (res->obj) store->del(res->obj);
      }
    }
    free(store->resv);
  }
}

/* Search.
 */
 
static int stdsyn_res_store_search(const struct stdsyn_res_store *store,int id) {
  int lo=0,hi=store->resc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct stdsyn_res *res=store->resv+ck;
         if (id<res->id) hi=ck;
    else if (id>res->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert.
 */
 
static struct stdsyn_res *stdsyn_res_store_insert(struct stdsyn_res_store *store,int p,int id) {
  if (id<0) return 0;
  if ((p<0)||(p>store->resc)) return 0;
  if (p&&(id<=store->resv[p-1].id)) return 0;
  if ((p<store->resc)&&(id>=store->resv[p].id)) return 0;
  if (store->resc>=store->resa) {
    int na=store->resa+16;
    if (na>INT_MAX/sizeof(struct stdsyn_res)) return 0;
    void *nv=realloc(store->resv,sizeof(struct stdsyn_res)*na);
    if (!nv) return 0;
    store->resv=nv;
    store->resa=na;
  }
  struct stdsyn_res *res=store->resv+p;
  memmove(res+1,res,sizeof(struct stdsyn_res)*(store->resc-p));
  store->resc++;
  memset(res,0,sizeof(struct stdsyn_res));
  res->id=id;
  return res;
}

/* Add serial.
 */
 
int stdsyn_res_store_add(struct stdsyn_res_store *store,int id,const void *v,int c) {
  if ((c<0)||(c&&!v)) return -1;
  if (id<0) return -1;
  int p=stdsyn_res_store_search(store,id);
  struct stdsyn_res *res;
  if (p<0) {
    p=-p-1;
    if (!(res=stdsyn_res_store_insert(store,p,id))) return -1;
  } else {
    res=store->resv+p;
  }
  void *nv=0;
  if (c) {
    if (!(nv=malloc(c))) return -1;
    memcpy(nv,v,c);
  }
  if (res->v) free(res->v);
  res->v=nv;
  res->c=c;
  return 0;
}

/* Get object, possibly decoding.
 */

void *stdsyn_res_store_get(struct stdsyn_res_store *store,int id) {
  int p=stdsyn_res_store_search(store,id);
  if (p<0) return 0;
  struct stdsyn_res *res=store->resv+p;
  if (!res->obj) {
    res->obj=store->decode(store->driver,id,res->v,res->c);
  }
  return res->obj;
}
