#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_wave {
  struct pcmprint_op hdr;
  struct pcmprint_env rateenv;
  struct pcmprint_oscillator osc;
};

#define OP ((struct pcmprint_op_wave*)op)

/* Cleanup.
 */
 
static void _wave_del(struct pcmprint_op *op) {
  pcmprint_env_cleanup(&OP->rateenv);
}

/* Update.
 */
 
static void _wave_update(float *v,int c,struct pcmprint_op *op) {
  for (;c-->0;v++) {
    uint32_t rate=(uint32_t)pcmprint_env_next(&OP->rateenv);
    OP->osc.update(v,1,&OP->osc,rate,rate);
  }
}

/* Decode.
 */
 
static int _wave_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  int srcp=0,err;
  if ((err=pcmprint_env_decode(&OP->rateenv,src,srcc,op->pcmprint->rate))<0) return err; srcp+=err;
  pcmprint_env_scale(&OP->rateenv,op->pcmprint->rate*65536.0f); // funny how this works out. scale the normalized rate up to 32 bits
  if ((err=pcmprint_oscillator_decode(&OP->osc,src+srcp,srcc-srcp,op->pcmprint))<0) return err; srcp+=err;
  op->update=_wave_update;
  return srcp;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_wave={
  .command=0x02,
  .name="wave",
  .objlen=sizeof(struct pcmprint_op_wave),
  .del=_wave_del,
  .decode=_wave_decode,
};
