#include "pcmprint_internal.h"

/* Instance definition.
 */
 
struct pcmprint_op_bandpass {
  struct pcmprint_op hdr;
  float coefv[5];
  float vv[5];
};

#define OP ((struct pcmprint_op_bandpass*)op)

/* Update.
 */
 
static void _bandpass_update(float *v,int c,struct pcmprint_op *op) {
  for (;c-->0;v++) {
    OP->vv[2]=OP->vv[1];
    OP->vv[1]=OP->vv[0];
    OP->vv[0]=*v;
    float ws=
      OP->vv[0]*OP->coefv[0]+
      OP->vv[1]*OP->coefv[1]+
      OP->vv[2]*OP->coefv[2]+
      OP->vv[3]*OP->coefv[3]+
      OP->vv[4]*OP->coefv[4];
    OP->vv[4]=OP->vv[3];
    OP->vv[3]=ws;
    *v=ws;
  }
}

/* Decode.
 */
 
static int _bandpass_decode(struct pcmprint_op *op,const uint8_t *src,int srcc) {
  if (srcc<4) return -1;
  
  int midhz=(src[0]<<8)|src[1];
  int whz=(src[2]<<8)|src[3];
  float midnorm=(float)midhz/op->pcmprint->rate;
  float wnorm=(float)whz/op->pcmprint->rate;
  
  /* 3-point IIR bandpass.
   * I have only a vague idea of how this works, and the formula is taken entirely on faith.
   * Reference:
   *   Steven W Smith: The Scientist and Engineer's Guide to Digital Signal Processing
   *   Ch 19, p 326, Equation 19-7
   */
  float r=1.0f-3.0f*wnorm;
  float cosfreq=cosf(M_PI*2.0f*midnorm);
  float k=(1.0f-2.0f*r*cosfreq+r*r)/(2.0f-2.0f*cosfreq);
  
  OP->coefv[0]=1.0f-k;
  OP->coefv[1]=2.0f*(k-4)*cosfreq;
  OP->coefv[2]=r*r-k;
  OP->coefv[3]=2.0f*4*cosfreq;
  OP->coefv[4]=-r*r;
  
  op->update=_bandpass_update;
  return 4;
}

/* Type definition.
 */
 
const struct pcmprint_op_type pcmprint_op_type_bandpass={
  .command=0x07,
  .name="bandpass",
  .objlen=sizeof(struct pcmprint_op_bandpass),
  .decode=_bandpass_decode,
};
