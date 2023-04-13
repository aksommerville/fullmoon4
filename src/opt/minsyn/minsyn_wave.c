#include "fmn_minsyn_internal.h"
#include <math.h>

/* Generate sine wave.
 */

void minsyn_generate_sine(int16_t *v,int c) {
  if (c<1) return;
  int i=c;
  float t=0.0f;
  float dt=(M_PI*2.0f)/c;
  for (;i-->0;v++,t+=dt) *v=sinf(t)*32000.0f;
}

/* Print wave.
 */
 
static int minsyn_print_wave(struct minsyn_wave *wave,struct bigpc_synth_driver *driver) {
  //TODO fancy wave printing
  
  /* simple saw. works, sounds awful */
  int16_t *v=wave->v;
  int i=MINSYN_WAVE_SIZE_SAMPLES;
  for (;i-->0;v++) {
    *v=(i*0x10000)/MINSYN_WAVE_SIZE_SAMPLES-0x8000;
  }
  /**/
  
  /* sine. *
  int16_t *v=wave->v;
  int i=MINSYN_WAVE_SIZE_SAMPLES;
  float t=0.0f;
  float dt=(M_PI*2.0f)/MINSYN_WAVE_SIZE_SAMPLES;
  for (;i-->0;v++,t+=dt) *v=sinf(t)*32000.0f;
  /**/
  
  return 0;
}

/* Search wave list.
 */
 
static int minsyn_wave_search(const struct bigpc_synth_driver *driver,int id) {
  int lo=0,hi=DRIVER->wavec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct minsyn_wave *q=DRIVER->wavev[ck];
         if (id<q->id) hi=ck;
    else if (id>q->id) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Get wave by id.
 */

struct minsyn_wave *minsyn_get_wave(struct bigpc_synth_driver *driver,int id) {
  int p=minsyn_wave_search(driver,id);
  if (p>=0) return DRIVER->wavev[p];
  //TODO decide whether we can add it
  p=-p-1;
  if (DRIVER->wavec>=DRIVER->wavea) {
    int na=DRIVER->wavea+8;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(DRIVER->wavev,sizeof(void*)*na);
    if (!nv) return 0;
    DRIVER->wavev=nv;
    DRIVER->wavea=na;
  }
  struct minsyn_wave *wave=malloc(sizeof(struct minsyn_wave));
  if (!wave) return 0;
  if (minsyn_print_wave(wave,driver)<0) {
    free(wave);
    return 0;
  }
  wave->id=id;
  memmove(DRIVER->wavev+p+1,DRIVER->wavev+p,sizeof(void*)*(DRIVER->wavec-p));
  DRIVER->wavev[p]=wave;
  return wave;
}

/* New silent wave.
 */
 
static struct minsyn_wave *minsyn_new_wave(struct bigpc_synth_driver *driver) {
  int id=1;
  if (DRIVER->wavec) {
    if (DRIVER->wavev[DRIVER->wavec-1]->id==INT_MAX) return 0; // dammit who did this?
    id=DRIVER->wavev[DRIVER->wavec-1]->id+1;
  }
  if (DRIVER->wavec>=DRIVER->wavea) {
    int na=DRIVER->wavea+8;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(DRIVER->wavev,sizeof(void*)*na);
    if (!nv) return 0;
    DRIVER->wavev=nv;
    DRIVER->wavea=na;
  }
  struct minsyn_wave *wave=calloc(1,sizeof(struct minsyn_wave));
  if (!wave) return 0;
  DRIVER->wavev[DRIVER->wavec++]=wave;
  wave->id=id;
  return wave;
}

/* New wave, decoding harmonics.
 */
 
int minsyn_new_wave_harmonics(struct bigpc_synth_driver *driver,const uint8_t *coefv,int coefc) {
  if (!coefv||(coefc<0)) coefc=0; else if (coefc>32) coefc=32;
  struct minsyn_wave *wave=minsyn_new_wave(driver);
  if (!wave) return -1;
  
  const int16_t *src=DRIVER->sine;
  int step=1;
  for (;coefc-->0;coefv++,step++) {
    if (!*coefv) continue;
    int16_t *dst=wave->v;
    int srcp=0,i=MINSYN_WAVE_SIZE_SAMPLES;
    for (;i-->0;dst++,srcp+=step) {
      if (srcp>=MINSYN_WAVE_SIZE_SAMPLES) srcp-=MINSYN_WAVE_SIZE_SAMPLES;
      (*dst)+=(src[srcp]*(*coefv))>>8;
    }
  }
  
  return wave->id;
}
