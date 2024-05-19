#include "../fmn_stdsyn_internal.h"
#include <math.h>

/* Reference sine wave.
 */
 
static struct stdsyn_wave *stdsyn_wave_sine=0;

static int stdsyn_sine_require() {
  if (!stdsyn_wave_sine) {
    if (!(stdsyn_wave_sine=stdsyn_wave_new())) return -1;
    float *v=stdsyn_wave_sine->v;
    int i=STDSYN_WAVE_SIZE_SAMPLES;
    float t=0.0f;
    float d=(M_PI*2.0f)/STDSYN_WAVE_SIZE_SAMPLES;
    for (;i-->0;v++,t+=d) *v=sinf(t);
  }
  return 0;
}

struct stdsyn_wave *stdsyn_wave_get_sine() {
  if (stdsyn_sine_require()<0) return 0;
  if (stdsyn_wave_ref(stdsyn_wave_sine)<0) return 0;
  return stdsyn_wave_sine;
}

/* Dumb wave object.
 */

void stdsyn_wave_del(struct stdsyn_wave *wave) {
  if (!wave) return;
  if (wave->refc-->1) return;
  free(wave);
}

int stdsyn_wave_ref(struct stdsyn_wave *wave) {
  if (!wave) return -1;
  if (wave->refc<1) return -1;
  if (wave->refc==INT_MAX) return -1;
  wave->refc++;
  return 0;
}

struct stdsyn_wave *stdsyn_wave_new() {
  struct stdsyn_wave *wave=calloc(1,sizeof(struct stdsyn_wave));
  if (!wave) return 0;
  wave->refc=1;
  return wave;
}

/* New wave from 8-bit harmonics.
 */
 
static void stdsyn_add_harmonic(float *dst,float level,int step,const float *src) {
  int srcp=0;
  int i=STDSYN_WAVE_SIZE_SAMPLES;
  for (;i-->0;dst++,srcp+=step) {
    if (srcp>=STDSYN_WAVE_SIZE_SAMPLES) srcp-=STDSYN_WAVE_SIZE_SAMPLES;
    (*dst)+=src[srcp]*level;
  }
}

struct stdsyn_wave *stdsyn_wave_from_harmonics(const uint8_t *v,int c) {
  struct stdsyn_wave *wave=stdsyn_wave_new();
  if (!wave) return 0;
  if (v) {
    if (stdsyn_sine_require()<0) {
      stdsyn_wave_del(wave);
      return 0;
    }
    int limit=STDSYN_WAVE_SIZE_SAMPLES>>1;
    if (c>limit) c=limit;
    int i=0; for (;i<c;i++) { 
      if (!v[i]) continue;
      float level=v[i]/255.0f;
      stdsyn_add_harmonic(wave->v,level,i+1,stdsyn_wave_sine->v);
    }
  }
  
  /* Level check. *
  float lo=wave->v[0];
  float hi=wave->v[0];
  const float *p=wave->v;
  int i=STDSYN_WAVE_SIZE_SAMPLES;
  for (;i-->0;p++) {
    if (*p<lo) lo=*p;
    else if (*p>hi) hi=*p;
  }
  fprintf(stderr,"generated wave from harmonics. limits=%f..%f\n",lo,hi);
  /**/
  
  return wave;
}

/* Runner cleanup.
 */

void stdsyn_wave_runner_cleanup(struct stdsyn_wave_runner *runner) {
  stdsyn_wave_del(runner->wave);
  runner->wave=0;
}

/* Runner setup.
 */

int stdsyn_wave_runner_set_wave(struct stdsyn_wave_runner *runner,struct stdsyn_wave *wave) {
  if (runner->wave==wave) return 0;
  if (wave&&(stdsyn_wave_ref(wave)<0)) return -1;
  stdsyn_wave_del(runner->wave);
  runner->wave=wave;
  return 0;
}

void stdsyn_wave_runner_set_rate_norm(struct stdsyn_wave_runner *runner,float cycles_per_frame) {
  runner->dp=cycles_per_frame*4294967296.0f;
}

void stdsyn_wave_runner_set_rate_hz(struct stdsyn_wave_runner *runner,float hz,float mainrate) {
  stdsyn_wave_runner_set_rate_norm(runner,hz/mainrate);
}

/* Update at a flat rate.
 */
 
void stdsyn_wave_runner_update(float *v,int c,struct stdsyn_wave_runner *runner) {
  for (;c-->0;v++) {
    *v=runner->wave->v[runner->p>>STDSYN_WAVE_SHIFT];
    runner->p+=runner->dp;
  }
}

/* Update with modulator.
 */
 
void stdsyn_wave_runner_update_mod(float *v,int c,struct stdsyn_wave_runner *runner,const float *mod) {
  for (;c-->0;v++,mod++) {
    *v=runner->wave->v[runner->p>>STDSYN_WAVE_SHIFT];
    runner->p+=(int32_t)runner->dp+(int32_t)runner->dp*(*mod);
  }
}
