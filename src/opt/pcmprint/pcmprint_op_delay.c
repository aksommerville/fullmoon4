#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_delay {
  struct pcmprint_op hdr;
  float *v;
  int c;
  int p;
  float dry,wet,store,feedback;
};

#define OP ((struct pcmprint_op_delay*)op)

/* Cleanup.
 */
 
static void _delay_del(struct pcmprint_op *op) {
  if (OP->v) free(OP->v);
}

/* Update.
 */
 
static void _delay_update(float *v,int c,struct pcmprint_op *op) {
  for (;c-->0;v++) {
    float pv=OP->v[OP->p];
    float in=*v;
    OP->v[OP->p]=in*OP->store+pv*OP->feedback;
    *v=in*OP->dry+pv*OP->wet;
    OP->p++;
    if (OP->p>=OP->c) OP->p=0;
  }
}

/* Decode.
 */
 
static int _delay_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  if (srcc<5) return -1;
  
  OP->c=(src[0]*op->pcmprint->rate)>>8;
  if (OP->c<1) OP->c=1;
  if (!(OP->v=malloc(sizeof(float)*OP->c))) return -1;
  
  OP->dry=src[1]/256.0f;
  OP->wet=src[2]/256.0f;
  OP->store=src[3]/256.0f;
  OP->feedback=src[4]/256.0f;
  
  op->update=_delay_update;
  return 5;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_delay={
  .command=0x06,
  .name="delay",
  .objlen=sizeof(struct pcmprint_op_delay),
  .del=_delay_del,
  .decode=_delay_decode,
};
