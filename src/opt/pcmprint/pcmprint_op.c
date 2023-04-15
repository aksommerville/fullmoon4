#include "pcmprint_internal.h"

/* Cleanup.
 */
 
void pcmprint_op_del(struct pcmprint_op *op) {
  if (!op) return;
  if (op->type->del) op->type->del(op);
  free(op);
}

/* New.
 */
 
struct pcmprint_op *pcmprint_op_new(struct pcmprint *pcmprint,const struct pcmprint_op_type *type) {
  if (!type) return 0;
  struct pcmprint_op *op=calloc(1,type->objlen);
  if (!op) return 0;
  op->type=type;
  op->pcmprint=pcmprint;
  if (type->init&&(type->init(op)<0)) {
    pcmprint_op_del(op);
    return 0;
  }
  return op;
}

/* Decode and ready.
 */
 
int pcmprint_op_decode_all(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  if (!op) return -1;
  int srcp=0;
  if (op->type->decode) {
    int err=op->type->decode(op,src+srcp,srcc-srcp);
    if (err<0) return err;
    srcp+=err;
  }
  if (!op->update) return -1;
  return srcp;
}
