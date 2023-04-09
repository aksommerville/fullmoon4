#include "fmn_stdsyn_internal.h"

/* Cleanup.
 */
 
void stdsyn_voice_cleanup(struct stdsyn_voice *voice) {
}

/* Update.
 */
 
static void stdsyn_signal_add_s(float *v,int c,float s) {
  for (;c-->0;v++) (*v)+=s;
}
 
void stdsyn_voice_update(float *v,int c,struct stdsyn_voice *voice) {
  if (voice->halfperiod<1) return;
  for (;c-->0;v++,voice->t+=voice->dt) {
    (*v)+=sinf(voice->t)*stdsyn_env_update(voice->env);
  }
  if (voice->env.finished) {
    voice->halfperiod=0;
  }
}

/* Release.
 */
 
void stdsyn_voice_release(struct stdsyn_voice *voice) {
  voice->chid=0xff;
  voice->noteid=0xff;
  stdsyn_env_release(&voice->env);
}
