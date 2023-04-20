#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_new_channel {
  struct pcmprint_op hdr;
};

#define OP ((struct pcmprint_op_new_channel*)op)

/* Cleanup.
 */
 
static void _new_channel_del(struct pcmprint_op *op) {
}

/* Update.
 */
 
static void _new_channel_update(float *v,int c,struct pcmprint_op *op) {
  float *src=v;
  float *dst=op->pcmprint->bouncev;
  int i=c;
  for (;i-->0;src++,dst++) (*dst)+=(*src);
  memset(v,0,sizeof(float)*c);
}

/* Decode.
 */
 
static int _new_channel_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  op->update=_new_channel_update;
  return 0;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_new_channel={
  .command=0x08,
  .name="new_channel",
  .objlen=sizeof(struct pcmprint_op_new_channel),
  .del=_new_channel_del,
  .decode=_new_channel_decode,
};
