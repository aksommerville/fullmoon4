#include "fmn_minsyn_internal.h"

/* Reset.
 */
 
void minsyn_env_reset(struct minsyn_env *env,uint8_t velocity,int mainrate) {

  /* XXX TEMP We don't currently have a venue for storing configured envelopes.
   * So make it up on the fly each time, the same content.
   */
  env->atkvlo=0x100000; env->atkvhi=0x300000;
  env->susvlo=0x050000; env->susvhi=0x100000;
  env->atkclo=  30; env->atkchi=  10;
  env->decclo=  50; env->decchi=  30;
  env->rlsclo= 100; env->rlschi= 600;
  
  /* Apply velocity. (times in ms at this point).
   */
  if (!velocity) {
    env->atkv=env->atkvlo;
    env->susv=env->susvlo;
    env->atkc=env->atkclo;
    env->decc=env->decclo;
    env->rlsc=env->rlsclo;
  } else if (velocity>=0x7f) {
    env->atkv=env->atkvhi;
    env->susv=env->susvhi;
    env->atkc=env->atkchi;
    env->decc=env->decchi;
    env->rlsc=env->rlschi;
  } else {
    uint8_t rev=0x7f-velocity;
    env->atkv=(env->atkvlo*rev+env->atkvhi*velocity)>>7;
    env->susv=(env->susvlo*rev+env->susvhi*velocity)>>7;
    env->atkc=(env->atkclo*rev+env->atkchi*velocity)>>7;
    env->decc=(env->decclo*rev+env->decchi*velocity)>>7;
    env->rlsc=(env->rlsclo*rev+env->rlschi*velocity)>>7;
  }
  if (!(env->atkc=(env->atkc*mainrate)/1000)) env->atkc=1;
  if (!(env->decc=(env->decc*mainrate)/1000)) env->decc=1;
  if (!(env->rlsc=(env->rlsc*mainrate)/1000)) env->rlsc=1;
  
  env->stage=0;
  env->released=0;
  env->term=0;
  env->v=0;
  env->c=env->atkc;
  env->dv=env->atkv/env->c;
}

/* Release.
 */
 
void minsyn_env_release(struct minsyn_env *env) {
  env->released=1;
  if (env->stage==2) minsyn_env_advance(env);
}

/* Advance.
 */
 
void minsyn_env_advance(struct minsyn_env *env) {
  switch (env->stage) {
    case 0: {
        env->stage=1;
        env->v=env->atkv;
        env->c=env->decc;
        env->dv=(env->susv-env->atkv)/env->c;
      } break;
    case 1: if (!env->released) {
        env->stage=2;
        env->v=env->susv;
        env->c=0xffffffff;
        env->dv=0;
        break;
      } // pass
    case 2: {
        env->stage=3;
        env->v=env->susv;
        env->c=env->rlsc;
        env->dv=-env->v/env->c;
      } break;
    default: env->term=1; env->v=0; env->dv=0;
  }
}
