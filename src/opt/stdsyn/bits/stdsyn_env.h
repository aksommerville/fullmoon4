/* stdsyn_env.h
 * Simple linear envelope generator.
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
  float va,atkv,susv,vz; // initial, attack, sustain, final levels in normal units
  int finished;
  
  // Constant config. "lo,hi" refer to Note On velocity; we scale linearly between them.
  float valo,vahi; // initial level
  int atktlo,atkthi; // attack time, ms
  float atkvlo,atkvhi; // attack level in normal units
  int dectlo,decthi; // decay time, ms
  float susvlo,susvhi; // sustain level in normal units
  int rlstlo,rlsthi; // release time, ms
  float vzlo,vzhi; // final level
  int autorelease;
  
};

/* (v) must be 5 bytes:
 *   u8 Attack time, ms
 *   u8 Attack level
 *   u8 Decay time, ms
 *   u8 Sustain level
 *   u8 Release time, 8ms
 * Or null to copy from the other side.
 */
void stdsyn_env_decode_lo(struct stdsyn_env *env,const void *v);
void stdsyn_env_decode_hi(struct stdsyn_env *env,const void *v);
void stdsyn_env_decode_novelocity(struct stdsyn_env *env,const void *v);
void stdsyn_env_default(struct stdsyn_env *env);
void stdsyn_env_init_constant(struct stdsyn_env *env,float v);

/* Decode long-form envelopes (the kind we don't share with minsyn).
 * Returns length consumed, or <0 on errors. Always <=srcc.
 * OK to call with null (env) to measure only.
 */
int stdsyn_env_decode(struct stdsyn_env *env,const void *src,int srcc);

// Multiply all levels. eg Channel Volume.
void stdsyn_env_multiply(struct stdsyn_env *env,float v);

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
