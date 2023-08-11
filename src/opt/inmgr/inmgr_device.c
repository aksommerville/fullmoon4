#include "inmgr_internal.h"

/* Delete.
 */
 
void inmgr_device_del(struct inmgr_device *device) {
  if (!device) return;
  if (device->name) free(device->name);
  if (device->capv) free(device->capv);
  free(device);
}

/* New.
 */
 
struct inmgr_device *inmgr_device_new(int devid,const char *name,int namec,uint16_t vid,uint16_t pid) {
  struct inmgr_device *device=calloc(1,sizeof(struct inmgr_device));
  if (!device) return 0;
  device->devid=devid;
  device->vid=vid;
  device->pid=pid;
  if (inmgr_device_set_name(device,name,namec)<0) {
    inmgr_device_del(device);
    return 0;
  }
  return device;
}

/* Set name.
 */

int inmgr_device_set_name(struct inmgr_device *device,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
  while (srcc&&((unsigned char)src[0]<=0x20)) { srcc--; src++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (device->name) free(device->name);
  device->name=nv;
  device->namec=srcc;
  return 0;
}

/* Capability list primitives.
 */
 
static int inmgr_device_capv_search(const struct inmgr_device *device,int btnid) {
  int lo=0,hi=device->capc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct inmgr_cap *cap=device->capv+ck;
         if (btnid<cap->btnid) hi=ck;
    else if (btnid>cap->btnid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct inmgr_cap *inmgr_device_capv_insert(struct inmgr_device *device,int p) {
  if ((p<0)||(p>device->capc)) return 0;
  if (device->capc>=device->capa) {
    int na=device->capc+32;
    if (na>INT_MAX/sizeof(struct inmgr_cap)) return 0;
    void *nv=realloc(device->capv,sizeof(struct inmgr_cap)*na);
    if (!nv) return 0;
    device->capv=nv;
    device->capa=na;
  }
  struct inmgr_cap *cap=device->capv+p;
  memmove(cap+1,cap,sizeof(struct inmgr_cap)*(device->capc-p));
  device->capc++;
  memset(cap,0,sizeof(struct inmgr_cap));
  return cap;
}

/* Capability list, public interface.
 */
 
struct inmgr_cap *inmgr_device_add_capability(struct inmgr_device *device,int btnid,int usage,int lo,int hi,int rest) {
  int p=inmgr_device_capv_search(device,btnid);
  if (p>=0) return 0;
  struct inmgr_cap *cap=inmgr_device_capv_insert(device,-p-1);
  if (!cap) return 0;
  cap->btnid=btnid;
  cap->usage=usage;
  cap->lo=lo;
  cap->hi=hi;
  cap->rest=rest;
  return cap;
}

struct inmgr_cap *inmgr_device_get_capability(const struct inmgr_device *device,int btnid) {
  int p=inmgr_device_capv_search(device,btnid);
  if (p<0) return 0;
  return device->capv+p;
}
