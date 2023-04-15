#include "fmn_minsyn_internal.h"

/* PCM lifecycle.
 */
 
void minsyn_pcm_del(struct minsyn_pcm *pcm) {
  if (!pcm) return;
  if (pcm->refc-->1) return;
  if (pcm->v) free(pcm->v);
  free(pcm);
}

int minsyn_pcm_ref(struct minsyn_pcm *pcm) {
  if (!pcm) return -1;
  if (pcm->refc<1) return -1;
  if (pcm->refc==INT_MAX) return -1;
  pcm->refc++;
  return 0;
}

/* Get PCM by ID.
 */
 
struct minsyn_pcm *minsyn_pcm_by_id(const struct bigpc_synth_driver *driver,int id) {
  if (id<1) return 0;
  int lo=0,hi=DRIVER->pcmc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    struct minsyn_pcm *pcm=DRIVER->pcmv[ck];
         if (id<pcm->id) hi=ck;
    else if (id>pcm->id) lo=ck+1;
    else return pcm;
  }
  return 0;
}

/* New PCM, installed.
 */
 
struct minsyn_pcm *minsyn_pcm_new(struct bigpc_synth_driver *driver,int c) {
  if (c<1) c=1;
  if (DRIVER->pcmc&&(DRIVER->pcmv[DRIVER->pcmc-1]->id==INT_MAX)) return 0;
  int id=DRIVER->pcmc?(DRIVER->pcmv[DRIVER->pcmc-1]->id+1):1;
  if (DRIVER->pcmc>=DRIVER->pcma) {
    int na=DRIVER->pcma+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(DRIVER->pcmv,sizeof(void*)*na);
    if (!nv) return 0;
    DRIVER->pcmv=nv;
    DRIVER->pcma=na;
  }
  struct minsyn_pcm *pcm=calloc(1,sizeof(struct minsyn_pcm));
  if (!pcm) return 0;
  pcm->refc=1;
  pcm->id=id;
  // Not using (loopa,loopz).
  if (!(pcm->v=calloc(c,sizeof(int16_t)))) {
    minsyn_pcm_del(pcm);
    return 0;
  }
  pcm->c=c;
  DRIVER->pcmv[DRIVER->pcmc++]=pcm;
  return pcm;
}

/* Printer cleanup.
 */

void minsyn_printer_del(struct minsyn_printer *printer) {
  if (!printer) return;
  minsyn_pcm_del(printer->pcm);
  pcmprint_del(printer->pcmprint);
  free(printer);
}

/* Update printer.
 */
 
int minsyn_printer_update(struct minsyn_printer *printer,int c) {
  int updc=printer->pcm->c-printer->p;
  if (updc>c) updc=c;
  pcmprint_updatei(printer->pcm->v+printer->p,updc,printer->pcmprint);
  printer->p+=updc;
  if (printer->p>=printer->pcm->c) return 0;
  return 1;
}

/* New printer.
 */
 
struct minsyn_printer *minsyn_printer_new(struct bigpc_synth_driver *driver,struct pcmprint *pcmprint,struct minsyn_pcm *pcm) {
  if (DRIVER->printerc>=DRIVER->printera) {
    int na=DRIVER->printera+8;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(DRIVER->printerv,sizeof(void*)*na);
    if (!nv) return 0;
    DRIVER->printerv=nv;
    DRIVER->printera=na;
  }
  struct minsyn_printer *printer=calloc(1,sizeof(struct minsyn_printer));
  if (!printer) return 0;
  printer->pcmprint=pcmprint;
  if (minsyn_pcm_ref(pcm)<0) { free(printer); return 0; }
  printer->pcm=pcm;
  DRIVER->printerv[DRIVER->printerc++]=printer;
  return printer;
}
