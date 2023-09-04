/* stdsyn_node_filter.c
 * argv:
 *   u8 mode:
 *     1 lopass
 *     2 hipass
 *     3 bandpass
 *     4 notch
 *     5 relative bandpass (noteid required; target freq is u8.8 multiplier)
 *   u16 target frequency, hz
 *   (u16) bandwidth, hz, for bandpass and notch
 */
 
#include "../fmn_stdsyn_internal.h"
#include <math.h>

struct stdsyn_node_filter {
  struct stdsyn_node hdr;
  float coefv[5];
  float statev[5];
};

#define NODE ((struct stdsyn_node_filter*)node)

/* Update.
 */
 
static void _filter_update_iir(float *v,int c,struct stdsyn_node *node) {
  for (;c-->0;v++) {
    NODE->statev[2]=NODE->statev[1];
    NODE->statev[1]=NODE->statev[0];
    NODE->statev[0]=*v;
    *v=
      NODE->statev[0]*NODE->coefv[0]+
      NODE->statev[1]*NODE->coefv[1]+
      NODE->statev[2]*NODE->coefv[2]+
      NODE->statev[3]*NODE->coefv[3]+
      NODE->statev[4]*NODE->coefv[4];
    NODE->statev[4]=NODE->statev[3];
    NODE->statev[3]=*v;
  }
}

static void _filter_update_fir(float *v,int c,struct stdsyn_node *node) {
  for (;c-->0;v++) {
    NODE->statev[2]=NODE->statev[1];
    NODE->statev[1]=NODE->statev[0];
    NODE->statev[0]=*v;
    *v=
      NODE->statev[0]*NODE->coefv[0]+
      NODE->statev[1]*NODE->coefv[1]+
      NODE->statev[2]*NODE->coefv[2];
  }
}

/* Init low pass.
 */
 
static int filter_init_lopass(struct stdsyn_node *node,const uint8_t *argv,int argc) {
  if (argc<3) return -1;
  int hz=(argv[1]<<8)|argv[2];
  float norm=(float)hz/(float)node->driver->rate;
  if (norm<0.0f) norm=0.0f; else if (norm>0.5f) norm=0.5f;
  
  /* Chebyshev low-pass filter, copied from the blue book without really understanding.
   * _The Scientist and Engineer's Guide to Digital Signal Processing_ by Steven W Smith, p 341.
   */
  float rp=-cosf(M_PI/2.0f);
  float ip=sinf(M_PI/2.0f);
  float t=2.0f*tanf(0.5f);
  float w=2.0f*M_PI*norm;
  float m=rp*rp+ip*ip;
  float d=4.0f-4.0f*rp*t+m*t*t;
  float x0=(t*t)/d;
  float x1=(2.0f*t*t)/d;
  float x2=(t*t)/d;
  float y1=(8.0f-2.0f*m*t*t)/d;
  float y2=(-4.0f-4.0f*rp*t-m*t*t)/d;
  float k=sinf(0.5f-w/2.0f)/sinf(0.5f+w/2.0f);
   
  NODE->coefv[0]=(x0-x1*k+x2*k*k)/d;
  NODE->coefv[1]=(-2.0f*x0*k+x1+x1*k*k-2.0f*x2*k)/d;
  NODE->coefv[2]=(x0*k*k-x1*k+x2)/d;
  NODE->coefv[3]=(2.0f*k+y1+y1*k*k-2.0f*y2*k)/d;
  NODE->coefv[4]=(-k*k-y1*k+y2)/d;
  
  node->update=_filter_update_iir;
  return 0;
}

/* Init high pass.
 */
 
static int filter_init_hipass(struct stdsyn_node *node,const uint8_t *argv,int argc) {
  if (argc<3) return -1;
  int hz=(argv[1]<<8)|argv[2];
  float norm=(float)hz/(float)node->driver->rate;
  if (norm<0.0f) norm=0.0f; else if (norm>0.5f) norm=0.5f;
  
  /* Chebyshev high-pass filter, copied from the blue book without really understanding.
   * Basically the same as low-pass, just (k) is different and the two leading coefficients are negative.
   * _The Scientist and Engineer's Guide to Digital Signal Processing_ by Steven W Smith, p 341.
   */
  float rp=-cosf(M_PI/2.0f);
  float ip=sinf(M_PI/2.0f);
  float t=2.0f*tanf(0.5f);
  float w=2.0f*M_PI*norm;
  float m=rp*rp+ip*ip;
  float d=4.0f-4.0f*rp*t+m*t*t;
  float x0=(t*t)/d;
  float x1=(2.0f*t*t)/d;
  float x2=(t*t)/d;
  float y1=(8.0f-2.0f*m*t*t)/d;
  float y2=(-4.0f-4.0f*rp*t-m*t*t)/d;
  float k=-cosf(w/2.0f+0.5f)/cosf(w/2.0f-0.5f);
   
  NODE->coefv[0]=-(x0-x1*k+x2*k*k)/d;
  NODE->coefv[1]=(-2.0f*x0*k+x1+x1*k*k-2.0f*x2*k)/d;
  NODE->coefv[2]=(x0*k*k-x1*k+x2)/d;
  NODE->coefv[3]=-(2.0f*k+y1+y1*k*k-2.0f*y2*k)/d;
  NODE->coefv[4]=(-k*k-y1*k+y2)/d;
  
  node->update=_filter_update_iir;
  return 0;
}

/* Init band pass.
 */
 
static int filter_init_bandpass(struct stdsyn_node *node,const uint8_t *argv,int argc) {
  if (argc<5) return -1;
  int hz=(argv[1]<<8)|argv[2];
  float midnorm=(float)hz/(float)node->driver->rate;
  if (midnorm<0.0f) midnorm=0.0f; else if (midnorm>0.5f) midnorm=0.5f;
  hz=(argv[3]<<8)|argv[4];
  float wnorm=(float)hz/(float)node->driver->rate;
  if (wnorm<0.0f) wnorm=0.0f; else if (wnorm>0.5f) wnorm=0.5f;
  
  /* 3-point IIR bandpass.
   * I have only a vague idea of how this works, and the formula is taken entirely on faith.
   * Reference:
   *   Steven W Smith: The Scientist and Engineer's Guide to Digital Signal Processing
   *   Ch 19, p 326, Equation 19-7
   */
  float r=1.0f-3.0f*wnorm;
  float cosfreq=cosf(M_PI*2.0f*midnorm);
  float k=(1.0f-2.0f*r*cosfreq+r*r)/(2.0f-2.0f*cosfreq);
  
  NODE->coefv[0]=1.0f-k;
  NODE->coefv[1]=2.0f*(k-r)*cosfreq;
  NODE->coefv[2]=r*r-k;
  NODE->coefv[3]=2.0f*r*cosfreq;
  NODE->coefv[4]=-r*r;
  
  node->update=_filter_update_iir;
  return 0;
}

/* Init notch.
 */
 
static int filter_init_notch(struct stdsyn_node *node,const uint8_t *argv,int argc) {
  if (argc<5) return -1;
  int hz=(argv[1]<<8)|argv[2];
  float midnorm=(float)hz/(float)node->driver->rate;
  if (midnorm<0.0f) midnorm=0.0f; else if (midnorm>0.5f) midnorm=0.5f;
  hz=(argv[3]<<8)|argv[4];
  float wnorm=(float)hz/(float)node->driver->rate;
  if (wnorm<0.0f) wnorm=0.0f; else if (wnorm>0.5f) wnorm=0.5f;
  
  /* 3-point IIR notch.
   * From the same source as bandpass, and same level of mystery to me.
   */
  float r=1.0f-3.0f*wnorm;
  float cosfreq=cosf(M_PI*2.0f*midnorm);
  float k=(1.0f-2.0f*r*cosfreq+r*r)/(2.0f-2.0f*cosfreq);
  
  NODE->coefv[0]=k;
  NODE->coefv[1]=-2.0f*k*cosfreq;
  NODE->coefv[2]=k;
  NODE->coefv[3]=2.0f*r*cosfreq;
  NODE->coefv[4]=-r*r;
  
  node->update=_filter_update_iir;
  return 0;
}

/* Init.
 */
 
static int _filter_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  const uint8_t *ARGV=argv;
  if (node->overwrite) return -1;
  if (node->chanc!=1) return -1;
  if (argc<1) return -1;
  switch (ARGV[0]) {
    case 1: if (filter_init_lopass(node,argv,argc)<0) return -1; break;
    case 2: if (filter_init_hipass(node,argv,argc)<0) return -1; break;
    case 3: if (filter_init_bandpass(node,argv,argc)<0) return -1; break;
    case 4: if (filter_init_notch(node,argv,argc)<0) return -1; break;
    
    case 5: { // bandpass, relative to note
        if (argc<5) return -1;
        uint8_t rewrite[5];
        memcpy(rewrite,argv,5);
        float freq=midi_note_frequency[node->noteid&0x7f];
        freq*=((float)((ARGV[1]<<8)|ARGV[2])/256.0f);
        int ifreq=(int)freq;
        if (ifreq<0) ifreq=0; else if (ifreq>0xffff) ifreq=0xffff;
        rewrite[1]=ifreq>>8;
        rewrite[2]=ifreq;
        if (filter_init_bandpass(node,rewrite,5)<0) return -1;
      } break;
      
    default: return -1;
  }
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_filter={
  .name="filter",
  .objlen=sizeof(struct stdsyn_node_filter),
  .init=_filter_init,
};
