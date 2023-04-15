#include "pcmprint_internal.h"

static void _update_noop(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate) {}
   
/* Sine.
 */
 
static void _update_sine(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate) {
  int32_t drate=(int32_t)zrate-(int32_t)arate;
  for (;c-->0;v++,arate+=drate) {
    *v=sinf((osc->p*M_PI)/2147483648.0f);
    osc->p+=arate;
  }
}
 
static void pcmprint_oscillator_init_sine(struct pcmprint_oscillator *osc,struct pcmprint *pcmprint) {
  osc->p=0;
  osc->update=_update_sine;
}
   
/* Square.
 */
 
static void _update_square(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate) {
  int32_t drate=(int32_t)zrate-(int32_t)arate;
  for (;c-->0;v++,arate+=drate) {
    *v=(osc->p&0x80000000)?-1.0f:1.0f;
    osc->p+=arate;
  }
}
 
static void pcmprint_oscillator_init_square(struct pcmprint_oscillator *osc,struct pcmprint *pcmprint) {
  osc->p=0;
  osc->update=_update_square;
}
   
/* Saw.
 */
 
static void _update_sawtooth(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate) {
  int32_t drate=(int32_t)zrate-(int32_t)arate;
  for (;c-->0;v++,arate+=drate) {
    *v=(osc->p/2147483648.0f)-1.0f;
    osc->p+=arate;
  }
}
 
static void pcmprint_oscillator_init_sawtooth(struct pcmprint_oscillator *osc,struct pcmprint *pcmprint) {
  osc->p=0;
  osc->update=_update_sawtooth;
}
   
/* Triangle.
 */
 
static void _update_triangle(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate) {
  int32_t drate=(int32_t)zrate-(int32_t)arate;
  for (;c-->0;v++,arate+=drate) {
    if (osc->p<0x80000000) *v=(osc->p/1073741824.0f)-1.0f;
    else *v=(osc->p&0x7fffffff)/-1073741824.0f+1.0f;
    osc->p+=arate;
  }
}
 
static void pcmprint_oscillator_init_triangle(struct pcmprint_oscillator *osc,struct pcmprint *pcmprint) {
  osc->p=0;
  osc->update=_update_triangle;
}
   
/* Noise.
 */
 
static void _update_noise(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate) {
  for (;c-->0;v++) *v=((rand()&0xffff)-32768)/32768.0f;
}
 
static void pcmprint_oscillator_init_noise(struct pcmprint_oscillator *osc,struct pcmprint *pcmprint) {
  osc->update=_update_noise;
}
   
/* Harmonics.
 */
 
static void _update_harmonics(float *v,int c,struct pcmprint_oscillator *osc,uint32_t arate,uint32_t zrate) {
  if (arate==zrate) {
    for (;c-->0;v++) {
      *v=osc->wave->v[osc->p>>PCMPRINT_WAVE_SHIFT];
      osc->p+=arate;
    }
  } else {
    int32_t drate=(int32_t)zrate-(int32_t)arate;
    for (;c-->0;v++,arate+=drate) {
      *v=osc->wave->v[osc->p>>PCMPRINT_WAVE_SHIFT];
      osc->p+=arate;
    }
  }
}
 
static void pcmprint_oscillator_init_harmonics(struct pcmprint_oscillator *osc,const uint8_t *src,int srcc,struct pcmprint *pcmprint) {
  if (!(osc->wave=pcmprint_wave_decode(pcmprint,src,srcc))) {
    osc->update=_update_noop;
    return;
  }
  osc->p=0;
  osc->update=_update_harmonics;
}

/* Decode.
 */
 
int pcmprint_oscillator_decode(struct pcmprint_oscillator *osc,const uint8_t *src,int srcc,struct pcmprint *pcmprint) {
  if (!osc||!pcmprint||!src||(srcc<1)) return -1;
  osc->p=0.0f;
  if (src[0]>=200) switch (src[0]) {
    case 200: pcmprint_oscillator_init_sine(osc,pcmprint); return 1;
    case 201: pcmprint_oscillator_init_square(osc,pcmprint); return 1;
    case 202: pcmprint_oscillator_init_sawtooth(osc,pcmprint); return 1;
    case 203: pcmprint_oscillator_init_triangle(osc,pcmprint); return 1;
    case 204: pcmprint_oscillator_init_noise(osc,pcmprint); return 1;
    default: return -1;
  }
  if (1+src[0]>srcc) return -1;
  pcmprint_oscillator_init_harmonics(osc,src,1+src[0],pcmprint);
  return 1+src[0];
}
