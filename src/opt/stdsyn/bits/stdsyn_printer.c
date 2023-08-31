#include "../fmn_stdsyn_internal.h"

/* Decode. (store hook)
 */
 
struct stdsyn_pcm *stdsyn_sound_decode(struct bigpc_synth_driver *driver,int id,const void *v,int c) {
  struct stdsyn_printer *printer=stdsyn_begin_print(driver,id,v,c);
  if (!printer) return 0;
  if (stdsyn_pcm_ref(printer->pcm)<0) return 0;
  return printer->pcm;
}

/* Dumb PCM object.
 */

void stdsyn_pcm_del(struct stdsyn_pcm *pcm) {
  if (!pcm) return;
  if (pcm->refc-->1) return;
  free(pcm);
}

int stdsyn_pcm_ref(struct stdsyn_pcm *pcm) {
  if (!pcm) return -1;
  if (pcm->refc<1) return -1;
  if (pcm->refc==INT_MAX) return -1;
  pcm->refc++;
  return 0;
}
 
struct stdsyn_pcm *stdsyn_pcm_new(int framec) {
  if (framec<1) return 0;
  if (framec>STDSYN_PCM_SANITY_LIMIT) return 0;
  struct stdsyn_pcm *pcm=calloc(1,sizeof(struct stdsyn_pcm)+sizeof(float)*framec);
  if (!pcm) return 0;
  pcm->refc=1;
  pcm->c=framec;
  return pcm;
}

/* Delete printer.
 */

void stdsyn_printer_del(struct stdsyn_printer *printer) {
  if (!printer) return;
  pcmprint_del(printer->pcmprint);
  stdsyn_pcm_del(printer->pcm);
  free(printer);
}

/* New printer.
 */
 
struct stdsyn_printer *stdsyn_printer_new(
  int rate,const void *src,int srcc
) {
  struct stdsyn_printer *printer=calloc(1,sizeof(struct stdsyn_printer));
  if (!printer) return 0;
  if (!(printer->pcmprint=pcmprint_new(rate,src,srcc))) {
    stdsyn_printer_del(printer);
    return 0;
  }
  int framec=pcmprint_get_length(printer->pcmprint);
  if (framec<1) framec=1;
  if (!(printer->pcm=stdsyn_pcm_new(framec))) {
    stdsyn_printer_del(printer);
    return 0;
  }
  return printer;
}

/* Update.
 */
 
int stdsyn_printer_update(struct stdsyn_printer *printer,int framec) {
  int remaining=printer->pcm->c-printer->p;
  if (framec>remaining) framec=remaining;
  if (framec>0) pcmprint_updatef(printer->pcm->v+printer->p,framec,printer->pcmprint);
  else framec=0;
  printer->p+=framec;
  if (printer->p>=printer->pcm->c) return 0;
  return 1;
}
