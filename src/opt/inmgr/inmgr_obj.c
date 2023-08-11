#include "inmgr_internal.h"

/* Delete.
 */

void inmgr_del(struct inmgr *inmgr) {
  if (!inmgr) return;
  if (inmgr->mapv) free(inmgr->mapv);
  if (inmgr->rulesv) {
    while (inmgr->rulesc-->0) inmgr_rules_del(inmgr->rulesv[inmgr->rulesc]);
    free(inmgr->rulesv);
  }
  if (inmgr->devicev) {
    while (inmgr->devicec-->0) inmgr_device_del(inmgr->devicev[inmgr->devicec]);
    free(inmgr->devicev);
  }
  inmgr_device_del(inmgr->device_pending);
  if (inmgr->deathrow_mapv) free(inmgr->deathrow_mapv);
  free(inmgr);
}

/* New.
 */

struct inmgr *inmgr_new(const struct inmgr_delegate *delegate) {
  if (!delegate) return 0;
  if (!delegate->all_btnid) return 0;
  if (!delegate->state) return 0;
  if (!delegate->action) return 0;
  struct inmgr *inmgr=calloc(1,sizeof(struct inmgr));
  if (!inmgr) return 0;
  inmgr->delegate=*delegate;
  return inmgr;
}

/* Trivial accessors.
 */

void *inmgr_get_userdata(const struct inmgr *inmgr) {
  return inmgr->delegate.userdata;
}

uint16_t inmgr_get_state(const struct inmgr *inmgr) {
  return inmgr->state;
}

void inmgr_force_state(struct inmgr *inmgr,uint16_t state) {
  inmgr->state=state;
}

int inmgr_for_each_device(
  struct inmgr *inmgr,
  int (*cb)(int devid,const char *name,int namec,uint16_t vid,uint16_t pid,void *userdata),
  void *userdata
) {
  int err,i=inmgr->devicec;
  struct inmgr_device **p=inmgr->devicev;
  for (;i-->0;p++) {
    struct inmgr_device *device=*p;
    if (err=cb(device->devid,device->name,device->namec,device->vid,device->pid,userdata)) return err;
  }
  return 0;
}

/* Map list.
 */

int inmgr_mapv_search(const struct inmgr *inmgr,int devid,int srcbtnid) {
  int lo=0,hi=inmgr->mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct inmgr_map *map=inmgr->mapv+ck;
         if (devid<map->devid) hi=ck;
    else if (devid>map->devid) lo=ck+1;
    else if (srcbtnid<map->srcbtnid) hi=ck;
    else if (srcbtnid>map->srcbtnid) lo=ck+1;
    else {
      while ((ck>lo)&&(map[-1].devid==devid)&&(map[-1].srcbtnid==srcbtnid)) { ck--; map--; }
      return ck;
    }
  }
  return -lo-1;
}

struct inmgr_map *inmgr_mapv_insert(struct inmgr *inmgr,int p,int devid,int srcbtnid) {
  if ((p<0)||(p>inmgr->mapc)) return 0;
  // not validating proper order, too much trouble
  if (inmgr->mapc>=inmgr->mapa) {
    int na=inmgr->mapa+32;
    if (na>INT_MAX/sizeof(struct inmgr_map)) return 0;
    void *nv=realloc(inmgr->mapv,sizeof(struct inmgr_map)*na);
    if (!nv) return 0;
    inmgr->mapv=nv;
    inmgr->mapa=na;
  }
  struct inmgr_map *map=inmgr->mapv+p;
  memmove(map+1,map,sizeof(struct inmgr_map)*(inmgr->mapc-p));
  inmgr->mapc++;
  memset(map,0,sizeof(struct inmgr_map));
  map->devid=devid;
  map->srcbtnid=srcbtnid;
  return map;
}

void inmgr_mapv_remove_devid(struct inmgr *inmgr,int devid) {
  int pa=inmgr_mapv_search(inmgr,devid,0);
  if (pa<0) pa=-pa-1;
  if (pa>=inmgr->mapc) return;
  if (inmgr->mapv[pa].devid!=devid) return;
  int pn=inmgr_mapv_search(inmgr,devid+1,0);
  if (pn<0) pn=-pn-1;
  if (pn<=pa) return;
  int rmc=pn-pa;
  struct inmgr_map *map=inmgr->mapv+pa;
  
  // Gather the state held by all of these maps.
  uint16_t state=0;
  {
    const struct inmgr_map *v=map;
    int i=rmc;
    for (;i-->0;v++) if (v->dsttype==INMGR_DSTTYPE_BUTTON) state|=v->dstbtnid;
  }
  
  inmgr->mapc-=rmc;
  memmove(map,map+rmc,sizeof(struct inmgr_map)*(inmgr->mapc-pa));
  
  // And now that we're stable, commit whatever state change there was.
  inmgr_set_state_multiple_with_callbacks(inmgr,state,0);
}

int inmgr_mapv_add(struct inmgr *inmgr,int devid,int srcbtnid,int srcvalue,int srclo,int srchi,uint8_t dsttype,uint8_t dstvalue,uint16_t dstbtnid) {
  int p=inmgr_mapv_search(inmgr,devid,srcbtnid);
  if (p<0) p=-p-1;
  struct inmgr_map *map=inmgr_mapv_insert(inmgr,p,devid,srcbtnid);
  if (!map) return -1;
  map->srcvalue=srcvalue;
  map->srclo=srclo;
  map->srchi=srchi;
  map->dsttype=dsttype;
  map->dstvalue=dstvalue;
  map->dstbtnid=dstbtnid;
  return 0;
}

/* Maps death row: Maps for the device under live configuration, which we can restore if live config cancels.
 */
 
int inmgr_deathrow_mapv_rebuild(struct inmgr *inmgr,int devid) {
  inmgr->deathrow_mapc=0;
  int p=inmgr_mapv_search(inmgr,devid,0);
  if (p<0) p=-p-1;
  const struct inmgr_map *map=inmgr->mapv+p;
  int c=0;
  while ((p+c<inmgr->mapc)&&(map[c].devid==devid)) c++;
  if (c>inmgr->deathrow_mapa) {
    int na=c;
    if (na>INT_MAX/sizeof(struct inmgr_map)) return -1;
    void *nv=realloc(inmgr->deathrow_mapv,sizeof(struct inmgr_map)*na);
    if (!nv) return -1;
    inmgr->deathrow_mapv=nv;
    inmgr->deathrow_mapa=na;
  }
  memcpy(inmgr->deathrow_mapv,map,sizeof(struct inmgr_map)*c);
  inmgr->deathrow_mapc=c;
  return 0;
}

int inmgr_deathrow_mapv_restore(struct inmgr *inmgr) {
  if (!inmgr->live_config_device) return 0;
  int devid=inmgr->live_config_device->devid;
  int p=inmgr_mapv_search(inmgr,devid,0);
  if (p<0) p=-p-1;
  int rmc=0;
  while ((p+rmc<inmgr->mapc)&&(inmgr->mapv[p+rmc].devid==devid)) rmc++;
  if (rmc<inmgr->deathrow_mapc) {
    int na=inmgr->mapc+inmgr->deathrow_mapc-rmc;
    if (na<INT_MAX-64) na=(na+64)&~63;
    if (na>INT_MAX/sizeof(struct inmgr_map)) return -1;
    void *nv=realloc(inmgr->mapv,sizeof(struct inmgr_map)*na);
    if (!nv) return -1;
    inmgr->mapv=nv;
    inmgr->mapa=na;
  }
  if (rmc!=inmgr->deathrow_mapc) {
    memmove(inmgr->mapv+p+inmgr->deathrow_mapc,inmgr->mapv+p+rmc,sizeof(struct inmgr_map)*(inmgr->mapc-rmc-p));
    inmgr->mapc+=inmgr->deathrow_mapc-rmc;
  }
  memcpy(inmgr->mapv+p,inmgr->deathrow_mapv,sizeof(struct inmgr_map)*inmgr->deathrow_mapc);
  return 0;
}

/* Update state.
 */
 
void inmgr_set_state_multiple_with_callbacks(struct inmgr *inmgr,uint16_t btnids,uint8_t value) {
  uint16_t btnid=1;
  for (;btnids;btnids>>=1,btnid<<=1) {
    if (!(btnids&1)) continue;
    if (value) {
      if (inmgr->state&btnid) continue;
      inmgr->state|=btnid;
      inmgr->delegate.state(inmgr,btnid,1,inmgr->state);
    } else {
      if (!(inmgr->state&btnid)) continue;
      inmgr->state&=~btnid;
      inmgr->delegate.state(inmgr,btnid,0,inmgr->state);
    }
  }
}
 
void inmgr_set_state_single_with_callbacks(struct inmgr *inmgr,uint16_t btnid,uint8_t value) {
  if (value) {
    if (inmgr->state&btnid) return;
    inmgr->state|=btnid;
    inmgr->delegate.state(inmgr,btnid,1,inmgr->state);
  } else {
    if (!(inmgr->state&btnid)) return;
    inmgr->state&=~btnid;
    inmgr->delegate.state(inmgr,btnid,0,inmgr->state);
  }
}

/* Rules list.
 */
 
struct inmgr_rules *inmgr_rulesv_match_device(const struct inmgr *inmgr,const struct inmgr_device *device) {
  struct inmgr_rules **p=inmgr->rulesv;
  int i=inmgr->rulesc;
  for (;i-->0;p++) {
    struct inmgr_rules *rules=*p;
    
    if (rules->vid&&(rules->vid!=device->vid)) continue;
    if (rules->pid&&(rules->pid!=device->pid)) continue;
    if (!inmgr_pattern_match(rules->name,rules->namec,device->name,device->namec)) continue;
    
    return rules;
  }
  return 0;
}

struct inmgr_rules *inmgr_rulesv_new(struct inmgr *inmgr,int p) {
  if (p<0) p=inmgr->rulesc;
  if (p>inmgr->rulesc) return 0;
  if (inmgr->rulesc>=inmgr->rulesa) {
    int na=inmgr->rulesa+8;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(inmgr->rulesv,sizeof(void*)*na);
    if (!nv) return 0;
    inmgr->rulesv=nv;
    inmgr->rulesa=na;
  }
  struct inmgr_rules *rules=inmgr_rules_new();
  if (!rules) return 0;
  memmove(inmgr->rulesv+p+1,inmgr->rulesv+p,sizeof(void*)*(inmgr->rulesc-p));
  inmgr->rulesc++;
  inmgr->rulesv[p]=rules;
  return rules;
}

/* Device list.
 */
 
static int inmgr_devicev_search(const struct inmgr *inmgr,int devid) {
  int lo=0,hi=inmgr->devicec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct inmgr_device *device=inmgr->devicev[ck];
         if (devid<device->devid) hi=ck;
    else if (devid>device->devid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
 
struct inmgr_device *inmgr_devicev_search_devid(const struct inmgr *inmgr,int devid) {
  int p=inmgr_devicev_search(inmgr,devid);
  if (p<0) return 0;
  return inmgr->devicev[p];
}

int inmgr_devicev_add(struct inmgr *inmgr,struct inmgr_device *device) {
  if (!device||(device->devid<1)) return -1;
  int p=inmgr_devicev_search(inmgr,device->devid);
  if (p>=0) return -1;
  p=-p-1;
  if (inmgr->devicec>=inmgr->devicea) {
    int na=inmgr->devicea+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(inmgr->devicev,sizeof(void*)*na);
    if (!nv) return -1;
    inmgr->devicev=nv;
    inmgr->devicea=na;
  }
  memmove(inmgr->devicev+p+1,inmgr->devicev+p,sizeof(void*)*(inmgr->devicec-p));
  inmgr->devicec++;
  inmgr->devicev[p]=device;
  return 0;
}

void inmgr_devicev_remove_devid(struct inmgr *inmgr,int devid) {
  int p=inmgr_devicev_search(inmgr,devid);
  if (p<0) return;
  
  // Abort live config if in progress.
  // Nothing similar for device_pending, because a device can't be both pending and listed.
  if (inmgr->live_config_device==inmgr->devicev[p]) {
    inmgr->live_config_device=0;
    inmgr->live_config_status=INMGR_LIVE_CONFIG_NONE;
    inmgr->live_config_btnid=0;
  }
  
  inmgr_device_del(inmgr->devicev[p]);
  inmgr->devicec--;
  memmove(inmgr->devicev+p,inmgr->devicev+p+1,sizeof(void*)*(inmgr->devicec-p));
}

int inmgr_device_index_by_devid(const struct inmgr *inmgr,int devid) {
  return inmgr_devicev_search(inmgr,devid);
}

int inmgr_device_devid_by_index(const struct inmgr *inmgr,int index) {
  if (index<0) return 0;
  if (index>=inmgr->devicec) return 0;
  return inmgr->devicev[index]->devid;
}

uint8_t inmgr_device_name_by_index(char *dst,int dsta,const struct inmgr *inmgr,int p) {
  if (p<0) return 0;
  if (p>=inmgr->devicec) return 0;
  const struct inmgr_device *device=inmgr->devicev[p];
  uint8_t dstc;
  if (device->namec>0xff) dstc=0xff;
  else dstc=device->namec;
  if (dstc<=dsta) memcpy(dst,device->name,dstc);
  return dstc;
}
