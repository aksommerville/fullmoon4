#include "pcmprint_internal.h"

extern const struct pcmprint_op_type pcmprint_op_type_fm;
extern const struct pcmprint_op_type pcmprint_op_type_wave;
extern const struct pcmprint_op_type pcmprint_op_type_noise;
extern const struct pcmprint_op_type pcmprint_op_type_env;
extern const struct pcmprint_op_type pcmprint_op_type_mlt;
extern const struct pcmprint_op_type pcmprint_op_type_delay;
extern const struct pcmprint_op_type pcmprint_op_type_bandpass;
extern const struct pcmprint_op_type pcmprint_op_type_new_channel;

static const struct pcmprint_op_type *pcmprint_op_typev[]={
  &pcmprint_op_type_fm,
  &pcmprint_op_type_wave,
  &pcmprint_op_type_noise,
  &pcmprint_op_type_env,
  &pcmprint_op_type_mlt,
  &pcmprint_op_type_delay,
  &pcmprint_op_type_bandpass,
  &pcmprint_op_type_new_channel,
};

/* Decode one command, return consumed length.
 * Return zero for a legal EOF (except that doesn't exist), or <0 for errors.
 * Input must not be empty.
 */

static int pcmprint_decode_1(struct pcmprint *pcmprint,const uint8_t *src,int srcc) {
  int i=sizeof(pcmprint_op_typev)/sizeof(void*);
  const struct pcmprint_op_type **p=pcmprint_op_typev;
  for (;i-->0;p++) {
    if ((*p)->command!=src[0]) continue;
    
    if (pcmprint->opc>=pcmprint->opa) {
      int na=pcmprint->opa+8;
      if (na>INT_MAX/sizeof(void*)) return -1;
      void *nv=realloc(pcmprint->opv,sizeof(void*)*na);
      if (!nv) return -1;
      pcmprint->opv=nv;
      pcmprint->opa=na;
    }
    
    struct pcmprint_op *op=pcmprint_op_new(pcmprint,*p);
    if (!op) return -1;
    int err=pcmprint_op_decode_all(op,src+1,srcc-1);
    if (err<0) {
      pcmprint_op_del(op);
      return err;
    }
    pcmprint->opv[pcmprint->opc++]=op;
    return 1+err;
  }
  fprintf(stderr,"pcmprint: Unexpected leading byte 0x%02x\n",src[0]);
  return -1;
}

/* Decode program, main entry point.
 */
 
int pcmprint_decode_program(struct pcmprint *pcmprint,const uint8_t *src,int srcc) {

  // Read duration. Zero is legal, but we will add one implicit frame of silence in that case.
  if (!src||(srcc<1)) return -1;
  int dur_s128=src[0];
  pcmprint->duration=(pcmprint->rate*dur_s128)/128;
  if (pcmprint->duration<1) pcmprint->duration=1;
  int srcp=1;
  
  // Read commands.
  while (srcp<srcc) {
    int err=pcmprint_decode_1(pcmprint,src+srcp,srcc-srcp);
    if (err<0) return err;
    if (!err) break;
    srcp+=err;
  }
  
  // If there's a bounce command, pre-allocate (bouncev). Its existence tells us to use it.
  struct pcmprint_op **op=pcmprint->opv;
  int i=pcmprint->opc;
  for (;i-->0;op++) {
    if ((*op)->type==&pcmprint_op_type_new_channel) {
      pcmprint->bouncea=1024;
      if (!(pcmprint->bouncev=malloc(sizeof(float)*pcmprint->bouncea))) {
        pcmprint->bouncea=0;
        return -1;
      }
      break;
    }
  }
  
  return 0;
}
