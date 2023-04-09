/* stdsyn_env.h
 * Simple linear envelope generator.
 * These are only suitable for ADSR envelopes with initial and final level of zero.
 */
 
#ifndef STDSYN_ENV_H
#define STDSYN_ENV_H

struct stdsyn_env {

  // Volatile state.
  int c; // frames remaining in stage
  float v; // current level
  float dv; // level change per frame
  int released;
  int stage; // 0,1,2,3=attack,decay,sustain,release
  int atkt,dect,rlst; // attack, decay, release times in frames (NB not ms)
  float atkv,susv; // attack, sustain levels in normal units
  int finished;
  
  // Constant config. "lo,hi" refer to Note On velocity; we scale linearly between them.
  int atktlo,atkthi; // attack time, ms
  float atkvlo,atkvhi; // attack level in normal units
  int dectlo,decthi; // decay time, ms
  float susvlo,susvhi; // sustain level in normal units
  int rlstlo,rlsthi; // release time, ms
  
};

void stdsyn_env_reset(struct stdsyn_env *env,uint8_t velocity,int mainrate);
void stdsyn_env_release(struct stdsyn_env *env);

// Get the next sample.
#define stdsyn_env_update(env) ({ \
  if ((env).c>0) { \
    (env).c--; \
    (env).v+=(env).dv; \
  } else { \
    stdsyn_env_advance(&(env)); \
  } \
  (env).v; \
})

// Should only be called via stdsyn_env_update().
void stdsyn_env_advance(struct stdsyn_env *env);

#endif
