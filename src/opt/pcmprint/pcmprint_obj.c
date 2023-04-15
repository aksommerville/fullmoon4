#include "pcmprint_internal.h"

/* Cleanup.
 */

void pcmprint_del(struct pcmprint *pcmprint) {
  if (!pcmprint) return;
  
  if (pcmprint->qbuf) free(pcmprint->qbuf);
  if (pcmprint->opv) {
    while (pcmprint->opc-->0) pcmprint_op_del(pcmprint->opv[pcmprint->opc]);
    free(pcmprint->opv);
  }
  if (pcmprint->wavev) {
    while (pcmprint->wavec-->0) free(pcmprint->wavev[pcmprint->wavec]);
    free(pcmprint->wavev);
  }
  
  free(pcmprint);
}

/* New.
 */

struct pcmprint *pcmprint_new(int rate,const void *src,int srcc) {
  if ((rate<1)||(rate>200000)||!src||(srcc<1)) return 0;
  struct pcmprint *pcmprint=calloc(1,sizeof(struct pcmprint));
  if (!pcmprint) return 0;
  
  pcmprint->rate=rate;
  pcmprint->qlevel=32000;
  
  if (pcmprint_decode_program(pcmprint,src,srcc)<0) {
    pcmprint_del(pcmprint);
    return 0;
  }
  return pcmprint;
}

/* Trivial accessors.
 */

int pcmprint_get_length(const struct pcmprint *pcmprint) {
  if (!pcmprint) return 0;
  return pcmprint->duration;
}

int16_t pcmprint_get_quantization_level(const struct pcmprint *pcmprint) {
  if (!pcmprint) return 0;
  return pcmprint->qlevel;
}

void pcmprint_set_quantization_level(struct pcmprint *pcmprint,int16_t q) {
  if (!pcmprint) return;
  pcmprint->qlevel=q;
}

/* Update.
 */

static void pcmprint_generate(float *v,int c,struct pcmprint *pcmprint) {
  memset(v,0,sizeof(float)*c);
  struct pcmprint_op **p=pcmprint->opv;
  int i=pcmprint->opc;
  for (;i-->0;p++) {
    (*p)->update(v,c,*p);
  }
}

int pcmprint_updatef(float *v,int c,struct pcmprint *pcmprint) {
  if (!v||(c<1)||!pcmprint) return -1;
  while (c>0) {
    int updc=pcmprint->duration-pcmprint->finishc;
    if (updc<=0) break;
    if (updc>c) updc=c;
    pcmprint_generate(v,updc,pcmprint);
    v+=updc;
    c-=updc;
    pcmprint->finishc+=updc;
  }
  if (c>0) memset(v,0,sizeof(float)*c);
  return (pcmprint->finishc>=pcmprint->duration)?0:1;
}

int pcmprint_updatei(int16_t *v,int c,struct pcmprint *pcmprint) {
  if (!v||(c<1)||!pcmprint) return -1;
  if (c>pcmprint->qbufa) {
    int na=c;
    if (na<INT_MAX-256) na=(na+256)&~255;
    if (na>INT_MAX/sizeof(float)) return -1;
    void *nv=realloc(pcmprint->qbuf,sizeof(float)*na);
    if (!nv) return -1;
    pcmprint->qbuf=nv;
    pcmprint->qbufa=na;
  }
  int err=pcmprint_updatef(pcmprint->qbuf,c,pcmprint);
  const float *src=pcmprint->qbuf;
  for (;c-->0;v++,src++) *v=(*src)*pcmprint->qlevel;
  return err;
}
