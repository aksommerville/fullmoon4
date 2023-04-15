#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_env {
  struct pcmprint_op hdr;
  struct pcmprint_env env;
};

#define OP ((struct pcmprint_op_env*)op)

/* Cleanup.
 */
 
static void _env_del(struct pcmprint_op *op) {
  pcmprint_env_cleanup(&OP->env);
}

/* Update.
 */
 
static void _env_update(float *v,int c,struct pcmprint_op *op) {
  for (;c-->0;v++) (*v)*=pcmprint_env_next(&OP->env);
}

/* Decode.
 */
 
static int _env_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  int srcp=0,err;
  if ((err=pcmprint_env_decode(&OP->env,src,srcc,op->pcmprint->rate))<0) return err; srcp+=err;
  op->update=_env_update;
  return srcp;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_env={
  .command=0x04,
  .name="env",
  .objlen=sizeof(struct pcmprint_op_env),
  .del=_env_del,
  .decode=_env_decode,
};
