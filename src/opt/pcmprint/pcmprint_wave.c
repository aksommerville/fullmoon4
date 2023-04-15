#include "pcmprint_internal.h"

/* Print wave from harmonics.
 */
 
static void pcmprint_wave_print_harmonics(float *dst,const uint8_t *icoefv,int coefc) {
  while (coefc&&!icoefv[coefc-1]) coefc--;
  memset(dst,0,sizeof(float)*PCMPRINT_WAVE_SIZE_SAMPLES);
  int step=1;
  for (;step<=coefc;step++,icoefv++) {
    if (!*icoefv) continue;
    float coef=(*icoefv)/256.0f;
    float p=0.0f,dp=(M_PI*2.0f)/step;
    int i=PCMPRINT_WAVE_SIZE_SAMPLES;
    float *dstp=dst;
    for (;i-->0;dstp++,p+=dp) (*dstp)+=sinf(p)*coef;
  }
}

/* Fetch or decode wave.
 */
 
struct pcmprint_wave *pcmprint_wave_decode(struct pcmprint *pcmprint,const uint8_t *serial,int serialc) {
  if (!serial||(serialc<1)||(serialc>PCMPRINT_WAVE_SERIAL_LIMIT)) return 0;
  if (serial[0]>=200) return 0; // harmonics format only
  if (1+serial[0]>serialc) return 0;
  struct pcmprint_wave **p=pcmprint->wavev;
  int i=pcmprint->wavec;
  for (;i-->0;p++) {
    if ((*p)->serialc!=serialc) continue;
    if (memcmp((*p)->serial,serial,serialc)) continue;
    return *p;
  }
  if (pcmprint->wavec>=pcmprint->wavea) {
    int na=pcmprint->wavea+4;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(pcmprint->wavev,sizeof(void*)*na);
    if (!nv) return 0;
    pcmprint->wavev=nv;
    pcmprint->wavea=na;
  }
  struct pcmprint_wave *wave=malloc(sizeof(struct pcmprint_wave));
  if (!wave) return 0;
  pcmprint->wavev[pcmprint->wavec++]=wave;
  memcpy(wave->serial,serial,serialc);
  wave->serialc=serialc;
  pcmprint_wave_print_harmonics(wave->v,serial+1,serial[0]);
  return wave;
}
