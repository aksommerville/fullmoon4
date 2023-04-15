#include "pcmprint_internal.h"

/* Cleanup.
 */
 
void pcmprint_env_cleanup(struct pcmprint_env *env) {
  if (env->pointv) free(env->pointv);
}

/* Decode.
 */
 
int pcmprint_env_decode(struct pcmprint_env *env,const uint8_t *src,int srcc,int rate) {
  if (srcc<3) return -1;
  if (3+src[2]*4>srcc) return -1;
  env->pointa=src[2];
  if (!(env->pointv=malloc(sizeof(struct pcmprint_env_point)*env->pointa))) return -1;
  env->pointc=0;
  env->v=((src[0]<<8)|src[1])/65536.0f;
  int srcp=3;
  struct pcmprint_env_point *point=env->pointv;
  while (env->pointc<env->pointa) {
    if ((point->t=(((src[srcp]<<8)|src[srcp+1])*rate)/1000)<1) point->t=1;
    point->v=((src[srcp+2]<<8)|src[srcp+3])/65536.0f;
    point++;
    env->pointc++;
    srcp+=4;
  }
  
  env->pointp=0;
  if (env->pointc>0) {
    env->c=env->pointv[0].t;
    env->dv=(env->pointv[0].v-env->v)/env->c;
  } else {
    env->c=INT_MAX;
    env->dv=0.0f;
  }
  
  return srcp;
}

/* Advance.
 */
 
void pcmprint_env_advance(struct pcmprint_env *env) {
  env->pointp++;
  if (env->pointp<env->pointc) {
    env->v=env->pointv[env->pointp-1].v;
    env->c=env->pointv[env->pointp].t;
    env->dv=(env->pointv[env->pointp].v-env->v)/env->c;
  } else {
    env->c=INT_MAX;
    env->dv=0.0f;
  }
}

/* Scale.
 */
 
void pcmprint_env_scale(struct pcmprint_env *env,float scale) {
  env->v*=scale;
  struct pcmprint_env_point *point=env->pointv;
  int i=env->pointc;
  for (;i-->0;point++) point->v*=scale;
  if (env->pointc>0) env->dv=(env->pointv[0].v-env->v)/env->c;
}
