#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_noise {
  struct pcmprint_op hdr;
};

#define OP ((struct pcmprint_op_noise*)op)

/* Update.
 */
 
static void _noise_update(float *v,int c,struct pcmprint_op *op) {
  for (;c-->0;v++) *v=((rand()&0xffff)-32768.0f)/32768.0f;
}

/* Decode.
 */
 
static int _noise_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  op->update=_noise_update;
  return 0;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_noise={
  .command=0x03,
  .name="noise",
  .objlen=sizeof(struct pcmprint_op_noise),
  .decode=_noise_decode,
};
