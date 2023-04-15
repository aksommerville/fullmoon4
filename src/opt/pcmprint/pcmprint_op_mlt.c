#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_mlt {
  struct pcmprint_op hdr;
  float v;
};

#define OP ((struct pcmprint_op_mlt*)op)

/* Update.
 */
 
static void _mlt_update(float *v,int c,struct pcmprint_op *op) {
  for (;c-->0;v++) (*v)*=OP->v;
}

/* Decode.
 */
 
static int _mlt_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  if (srcc<2) return -1;
  OP->v=((src[0]<<8)|src[1])/256.0f;
  op->update=_mlt_update;
  return 2;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_mlt={
  .command=0x05,
  .name="mlt",
  .objlen=sizeof(struct pcmprint_op_mlt),
  .decode=_mlt_decode,
};
