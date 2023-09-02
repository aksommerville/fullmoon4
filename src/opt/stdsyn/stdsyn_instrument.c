#include "fmn_stdsyn_internal.h"

/* Delete.
 */
 
void stdsyn_instrument_del(struct stdsyn_instrument *ins) {
  if (!ins) return;
  free(ins);
}

/* Decode.
 */
 
struct stdsyn_instrument *stdsyn_instrument_decode(struct bigpc_synth_driver *driver,int id,const void *v,int c) {
  fprintf(stderr,"%s id=%d c=%d\n",__func__,id,c);
  struct stdsyn_instrument *instrument=calloc(1,sizeof(struct stdsyn_instrument)+c);
  if (!instrument) return 0;
  instrument->c=c;
  memcpy(instrument->v,v,c);
  return instrument;
}
