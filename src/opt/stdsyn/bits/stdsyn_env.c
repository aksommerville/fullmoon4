#include "../fmn_stdsyn_internal.h"

/* Decode or copy other half.
 */

void stdsyn_env_decode_lo(struct stdsyn_env *env,const void *v) {
  if (v) {
    const uint8_t *V=v;
    env->atktlo=V[0];
    env->atkvlo=V[1]/255.0f;
    env->dectlo=V[2];
    env->susvlo=V[3]/255.0f;
    env->rlstlo=V[4]*8;
  } else {
    env->atktlo=env->atkthi;
    env->atkvlo=env->atkvhi;
    env->dectlo=env->decthi;
    env->susvlo=env->susvhi;
    env->rlstlo=env->rlsthi;
  }
}

void stdsyn_env_decode_hi(struct stdsyn_env *env,const void *v) {
  if (v) {
    const uint8_t *V=v;
    env->atkthi=V[0];
    env->atkvhi=V[1]/255.0f;
    env->decthi=V[2];
    env->susvhi=V[3]/255.0f;
    env->rlsthi=V[4]*8;
  } else {
    env->atkthi=env->atktlo;
    env->atkvhi=env->atkvlo;
    env->decthi=env->dectlo;
    env->susvhi=env->susvlo;
    env->rlsthi=env->rlstlo;
  }
}

void stdsyn_env_decode_novelocity(struct stdsyn_env *env,const void *v) {
  if (v) {
    const uint8_t *V=v;
    env->atktlo=env->atkthi=V[0];
    env->atkvlo=env->atkvhi=V[1]/255.0f;
    env->dectlo=env->decthi=V[2];
    env->susvlo=env->susvhi=V[3]/255.0f;
    env->rlstlo=env->rlsthi=V[4]*8;
  } else {
    stdsyn_env_default(env);
  }
}

void stdsyn_env_default(struct stdsyn_env *env) {
  stdsyn_env_decode_lo(env,"\x14\x80\x20\x20\x18");
  stdsyn_env_decode_hi(env,"\x10\xc0\x24\x40\x30");
}

/* Set master levels.
 */
 
void stdsyn_env_multiply(struct stdsyn_env *env,float v) {
  env->valo*=v;
  env->vahi*=v;
  env->atkvlo*=v;
  env->atkvhi*=v;
  env->susvlo*=v;
  env->susvhi*=v;
  env->vzlo*=v;
  env->vzhi*=v;
}

/* Reset.
 */
 
void stdsyn_env_reset(struct stdsyn_env *env,uint8_t velocity,int mainrate) {

  env->released=0;
  env->finished=0;
  if (velocity<=0) {
    env->va=env->valo;
    env->atkt=env->atktlo;
    env->dect=env->dectlo;
    env->rlst=env->rlstlo;
    env->atkv=env->atkvlo;
    env->susv=env->susvlo;
    env->vz=env->vzlo;
  } else if (velocity>=0x7f) {
    env->va=env->vahi;
    env->atkt=env->atkthi;
    env->dect=env->decthi;
    env->rlst=env->rlsthi;
    env->atkv=env->atkvhi;
    env->susv=env->susvhi;
    env->vz=env->vzhi;
  } else {
    uint8_t loweight=0x80-velocity;
    env->atkt=(env->atktlo*loweight+env->atkthi*velocity)>>7;
    env->dect=(env->dectlo*loweight+env->decthi*velocity)>>7;
    env->rlst=(env->rlstlo*loweight+env->rlsthi*velocity)>>7;
    float hi=velocity/127.0f;
    float lo=1.0f-hi;
    env->va=env->valo*lo+env->vahi*hi;
    env->atkv=env->atkvlo*lo+env->atkvhi*hi;
    env->susv=env->susvlo*lo+env->susvhi*hi;
    env->vz=env->vzlo*lo+env->vzhi*hi;
  }
  if ((env->atkt=(env->atkt*mainrate)/1000)<1) env->atkt=1;
  if ((env->dect=(env->dect*mainrate)/1000)<1) env->dect=1;
  if ((env->rlst=(env->rlst*mainrate)/1000)<1) env->rlst=1;
  
  // Enter stage 0.
  env->c=env->atkt;
  env->v=env->va;
  env->dv=(env->atkv-env->va)/env->c;
  env->stage=0;
}

/* Release.
 */
 
void stdsyn_env_release(struct stdsyn_env *env) {
  env->released=1;
  if (env->stage==2) stdsyn_env_advance(env);
}

/* Advance to next stage.
 */
 
void stdsyn_env_advance(struct stdsyn_env *env) {
  if (env->stage>=3) {
    env->c=INT_MAX;
    env->v=env->vz;
    env->dv=0.0f;
    env->finished=1;
    return;
  }
  env->stage++;
  switch (env->stage) {
    case 1: {
        env->v=env->atkv;
        env->c=env->dect;
        env->dv=(env->susv-env->atkv)/env->c;
      } break;
    case 2: if (!env->released) {
        env->v=env->susv;
        env->c=INT_MAX;
        env->dv=0.0f;
        break;
      } // else pass
    case 3: {
        env->c=env->rlst;
        env->dv=(env->vz-env->v)/env->c;
      } break;
  }
}
