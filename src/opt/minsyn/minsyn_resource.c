#include "fmn_minsyn_internal.h"

/* Cleanup.
 */
 
void minsyn_resource_cleanup(struct minsyn_resource *resource) {
  if (resource->v) free(resource->v);
}

static void minsyn_resource_reuse(struct minsyn_resource *resource) {
  if (resource->v) {
    free(resource->v);
    resource->v=0;
  }
  resource->c=0;
  resource->waveid=0;
  resource->pcmid=0;
  memset(&resource->envlo,0,sizeof(struct minsyn_env_config));
  memset(&resource->envhi,0,sizeof(struct minsyn_env_config));
}

/* Search.
 */
 
static int minsyn_resource_search(const struct bigpc_synth_driver *driver,int type,int id) {
  int lo=0,hi=DRIVER->resourcec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct minsyn_resource *q=DRIVER->resourcev+ck;
         if (type<q->type) hi=ck;
    else if (type>q->type) lo=ck+1;
    else if (id<q->id) hi=ck;
    else if (id>q->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert.
 */
 
static struct minsyn_resource *minsyn_resource_insert(struct bigpc_synth_driver *driver,int p,int type,int id) {
  if ((p<0)||(p>DRIVER->resourcec)) return 0;
  // order must be valid but i don't feel like validating
  if (DRIVER->resourcec>=DRIVER->resourcea) {
    int na=DRIVER->resourcea+16;
    if (na>INT_MAX/sizeof(struct minsyn_resource)) return 0;
    void *nv=realloc(DRIVER->resourcev,sizeof(struct minsyn_resource)*na);
    if (!nv) return 0;
    DRIVER->resourcev=nv;
    DRIVER->resourcea=na;
  }
  struct minsyn_resource *resource=DRIVER->resourcev+p;
  memmove(resource+1,resource,sizeof(struct minsyn_resource)*(DRIVER->resourcec-p));
  DRIVER->resourcec++;
  memset(resource,0,sizeof(struct minsyn_resource));
  resource->type=type;
  resource->id=id;
  return resource;
}

/* Public accessors.
 */
 
struct minsyn_resource *minsyn_resource_init_instrument(struct bigpc_synth_driver *driver,int id) {
  struct minsyn_resource *resource;
  int p=minsyn_resource_search(driver,FMN_RESTYPE_INSTRUMENT,id);
  if (p<0) {
    p=-p-1;
    if (!(resource=minsyn_resource_insert(driver,p,FMN_RESTYPE_INSTRUMENT,id))) return 0;
  } else {
    resource=DRIVER->resourcev+p;
    minsyn_resource_reuse(resource);
  }
  return resource;
}

struct minsyn_resource *minsyn_resource_init_sound(struct bigpc_synth_driver *driver,int id) {
  struct minsyn_resource *resource;
  int p=minsyn_resource_search(driver,FMN_RESTYPE_SOUND,id);
  if (p<0) {
    p=-p-1;
    if (!(resource=minsyn_resource_insert(driver,p,FMN_RESTYPE_SOUND,id))) return 0;
  } else {
    resource=DRIVER->resourcev+p;
    minsyn_resource_reuse(resource);
  }
  return resource;
}

struct minsyn_resource *minsyn_resource_get_instrument(const struct bigpc_synth_driver *driver,int id) {
  int p=minsyn_resource_search(driver,FMN_RESTYPE_INSTRUMENT,id);
  if (p<0) return 0;
  return DRIVER->resourcev+p;
}

struct minsyn_resource *minsyn_resource_get_sound(const struct bigpc_synth_driver *driver,int id) {
  int p=minsyn_resource_search(driver,FMN_RESTYPE_SOUND,id);
  if (p<0) return 0;
  return DRIVER->resourcev+p;
}

/* Decode wave from harmonics.
 */
 
static int minsyn_instrument_decode_wave(struct bigpc_synth_driver *driver,struct minsyn_resource *resource,const uint8_t *src,int srcc) {
  if (srcc<1) return -1;
  int coefc=src[0];
  int srcp=1;
  if (srcp>srcc-coefc) return -1;
  
  int waveid=minsyn_new_wave_harmonics(driver,src,coefc);
  if (waveid<0) return -1;
  srcp+=coefc;
  resource->waveid=waveid;
  
  return srcp;
}

/* Decode envelope.
 */
 
static void minsyn_instrument_decode_env(struct minsyn_env_config *env,struct bigpc_synth_driver *driver,const uint8_t *src) {
  env->atkc=(src[0]*driver->rate)/1000;
  env->atkv=(src[1]<<16)|(src[1]<<8)|src[1];
  env->decc=(src[2]*driver->rate)/1000;
  env->susv=(src[3]<<16)|(src[3]<<8)|src[3];
  env->rlsc=(src[4]*driver->rate*8)/1000;
}

/* Default envelope.
 */
 
static void minsyn_env_config_default(struct minsyn_env_config *env,int rate) {
  env->atkc=(20*rate)/1000;
  env->atkv=0x400000;
  env->decc=(25*rate)/1000;
  env->susv=0x200000;
  env->rlsc=(100*rate)/1000;
}
 
static void minsyn_instrument_set_default_env(struct bigpc_synth_driver *driver,struct minsyn_resource *resource) {
  minsyn_env_config_default(&resource->envlo,driver->rate);
  memcpy(&resource->envhi,&resource->envlo,sizeof(struct minsyn_env_config));
}

/* Decode instrument, install objects, update entry.
 */
 
static int minsyn_resource_decode_instrument(struct bigpc_synth_driver *driver,struct minsyn_resource *resource) {
  const uint8_t *src=resource->v;
  if (resource->c<1) return 0;
  uint8_t features=*src;
  int srcp=1,err;
  
  if (features&0x01) { // wave from harmonics
    if ((err=minsyn_instrument_decode_wave(driver,resource,src+srcp,resource->c-srcp))<0) return err;
    srcp+=err;
  }
  
  switch (features&0x06) { // low+high envelope
    case 0x00: {
        minsyn_instrument_set_default_env(driver,resource);
      } break;
    case 0x02:
    case 0x04: { // low or high only. same thing, we use the same envelope for both.
        if (srcp>resource->c-5) return -1;
        minsyn_instrument_decode_env(&resource->envlo,driver,src+srcp);
        srcp+=5;
        memcpy(&resource->envhi,&resource->envlo,sizeof(struct minsyn_env_config));
      } break;
    case 0x06: {
        if (srcp>resource->c-10) return -1;
        minsyn_instrument_decode_env(&resource->envlo,driver,src+srcp);
        srcp+=5;
        minsyn_instrument_decode_env(&resource->envhi,driver,src+srcp);
        srcp+=5;
      } break;
  }
        
  return 0;
}

/* Decode sound, install pcm, update entry.
 */
 
static int minsyn_resource_decode_sound(struct bigpc_synth_driver *driver,struct minsyn_resource *resource) {
  
  struct pcmprint *pcmprint=pcmprint_new(driver->rate,resource->v,resource->c);
  if (!pcmprint) {
    fprintf(stderr,"Failed to decode %d-byte program for sound:%d.\n",resource->c,resource->id);
    return -1;
  }
  int c=pcmprint_get_length(pcmprint);
  if (c<1) c=1;
  
  struct minsyn_pcm *pcm=minsyn_pcm_new(driver,c);
  if (!pcm) {
    pcmprint_del(pcmprint);
    return -1;
  }
  resource->pcmid=pcm->id;
  
  struct minsyn_printer *printer=minsyn_printer_new(driver,pcmprint,pcm);
  if (!printer) {
    pcmprint_del(pcmprint);
    // ok to leave (pcm) installed
    return -1;
  }
  
  if (DRIVER->update_in_progress_framec) {
    minsyn_printer_update(printer,DRIVER->update_in_progress_framec);
  }
  
  return 0;
}

/* Ready.
 */
 
int minsyn_resource_ready(struct bigpc_synth_driver *driver,struct minsyn_resource *resource) {
  if (resource->ready) return 0;
  resource->ready=1;
  switch (resource->type) {
    case FMN_RESTYPE_INSTRUMENT: {
        if (minsyn_resource_decode_instrument(driver,resource)<0) return -1;
      } break;
    case FMN_RESTYPE_SOUND: {
        if (minsyn_resource_decode_sound(driver,resource)<0) return -1;
      } break;
  }
  return 0;
}
