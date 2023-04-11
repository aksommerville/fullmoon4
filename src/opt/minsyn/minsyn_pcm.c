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

/* Printer cleanup.
 */

void minsyn_printer_del(struct minsyn_printer *printer) {
  if (!printer) return;
  minsyn_pcm_del(printer->pcm);
  free(printer);
}

/* Update printer.
 */
 
int minsyn_printer_update(struct minsyn_printer *printer,int c) {
  int updc=printer->pcm->c-printer->p;
  if (updc>c) updc=c;
  
  //TODO generate pcm
  memset(printer->pcm->v+printer->p,0,updc<<1);
  
  printer->p+=updc;
  if (printer->p>=printer->pcm->c) return 0;
  return 1;
}
