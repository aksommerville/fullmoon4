#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_fm {
  struct pcmprint_op hdr;
  struct pcmprint_env rateenv,rangeenv;
  float modrate;
  struct pcmprint_oscillator osc;
  float modp;
};

#define OP ((struct pcmprint_op_fm*)op)

/* Cleanup.
 */
 
static void _fm_del(struct pcmprint_op *op) {
  pcmprint_env_cleanup(&OP->rateenv);
  pcmprint_env_cleanup(&OP->rangeenv);
}

/* Update.
 */
 
static void _fm_update(float *v,int c,struct pcmprint_op *op) {
  for (;c-->0;v++) {
    float rate=pcmprint_env_next(&OP->rateenv);
    float range=pcmprint_env_next(&OP->rangeenv);
    float mod=sinf(OP->modp);
    OP->modp+=rate*OP->modrate;
    float crate=rate+rate*mod*range;
    float irate=crate*4294967296.0f;
    OP->osc.update(v,1,&OP->osc,irate,irate);
  }
}

/* Decode.
 */
 
static int _fm_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  int srcp=0,err;

  if ((err=pcmprint_env_decode(&OP->rateenv,src,srcc,op->pcmprint->rate))<0) return err; srcp+=err;
  pcmprint_env_scale(&OP->rateenv,op->pcmprint->rate/65536.0f);

  if (srcp>=srcc) return -1;
  OP->modrate=src[srcp++]/16.0f;
  OP->modrate*=M_PI*2.0f;

  if ((err=pcmprint_env_decode(&OP->rangeenv,src+srcp,srcc-srcp,op->pcmprint->rate))<0) return err; srcp+=err;
  pcmprint_env_scale(&OP->rangeenv,65.536f);
  
  if ((err=pcmprint_oscillator_decode(&OP->osc,src+srcp,srcc-srcp,op->pcmprint))<0) return err; srcp+=err;
  
  op->update=_fm_update;
  
  return srcp;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_fm={
  .command=0x01,
  .name="fm",
  .objlen=sizeof(struct pcmprint_op_fm),
  .del=_fm_del,
  .decode=_fm_decode,
};
