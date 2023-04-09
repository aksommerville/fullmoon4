#include "fmn_stdsyn_internal.h"

/* Reset.
 */
 
void stdsyn_env_reset(struct stdsyn_env *env,uint8_t velocity,int mainrate) {

  env->released=0;
  env->finished=0;
  if (velocity<=0) {
    env->atkt=env->atktlo;
    env->dect=env->dectlo;
    env->rlst=env->rlstlo;
    env->atkv=env->atkvlo;
    env->susv=env->susvlo;
  } else if (velocity>=0x7f) {
    env->atkt=env->atkthi;
    env->dect=env->decthi;
    env->rlst=env->rlsthi;
    env->atkv=env->atkvhi;
    env->susv=env->susvhi;
  } else {
    uint8_t loweight=0x80-velocity;
    env->atkt=(env->atktlo*loweight+env->atkthi*velocity)>>7;
    env->dect=(env->dectlo*loweight+env->decthi*velocity)>>7;
    env->rlst=(env->rlstlo*loweight+env->rlsthi*velocity)>>7;
    float hi=velocity/127.0f;
    float lo=1.0f-hi;
    env->atkv=env->atkvlo*lo+env->atkvhi*hi;
    env->susv=env->susvlo*lo+env->susvhi*hi;
  }
  if ((env->atkt=(env->atkt*mainrate)/1000)<1) env->atkt=1;
  if ((env->dect=(env->dect*mainrate)/1000)<1) env->dect=1;
  if ((env->rlst=(env->rlst*mainrate)/1000)<1) env->rlst=1;
  
  // Enter stage 0.
  env->c=env->atkt;
  env->v=0.0f;
  env->dv=env->atkv/env->c;
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
    env->v=0.0f;
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
        env->v=env->susv;
        env->c=env->rlst;
        env->dv=-env->v/env->c;
      } break;
  }
}
