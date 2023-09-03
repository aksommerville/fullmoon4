/* stdsyn_node_oscillator.c
 * Overwrites buffer with a tuned wave, noise, or silence.
 *
 * (argv) is a list of 8-bit coefficients.
 * Use a single zero for silence, or empty for noise.
 */
 
#include "../fmn_stdsyn_internal.h"

struct stdsyn_node_oscillator {
  struct stdsyn_node hdr;
  struct stdsyn_wave_runner runner;
};

#define NODE ((struct stdsyn_node_oscillator*)node)

/* Cleanup.
 */
 
static void _oscillator_del(struct stdsyn_node *node) {
  stdsyn_wave_runner_cleanup(&NODE->runner);
}

/* Update.
 */
 
static void _oscillator_update_silence(float *v,int c,struct stdsyn_node *node) {
  memset(v,0,sizeof(float)*c);
}

static void _oscillator_update_noise(float *v,int c,struct stdsyn_node *node) {
  for (;c-->0;v++) {
    *v=((rand()&0xffff)-32768)/32768.0f;
  }
}

static void _oscillator_update_wave(float *v,int c,struct stdsyn_node *node) {
  stdsyn_wave_runner_update(v,c,&NODE->runner);
}

/* Init with harmonics.
 */
 
static int oscillator_init_harmonics(struct stdsyn_node *node,const void *argv,int argc) {
  //TODO!!! This makes a new wave for each voice. We need to cache them somewhere else.
  struct stdsyn_wave *wave=stdsyn_wave_from_harmonics(argv,argc);
  if (!wave) return -1;
  int err=stdsyn_wave_runner_set_wave(&NODE->runner,wave);
  stdsyn_wave_del(wave);
  if (err<0) return err;
  stdsyn_wave_runner_set_rate_hz(&NODE->runner,midi_note_frequency[node->noteid&0x7f],node->driver->rate);
  return 0;
}

/* Init.
 */
 
static int _oscillator_init(struct stdsyn_node *node,uint8_t velocity,const void *argv,int argc) {
  if (!node->overwrite) return -1;
  if (node->chanc!=1) return -1;
  const uint8_t *ARGV=argv;
  if (!argc) {
    node->update=_oscillator_update_noise;
  } else if ((argc==1)&&!ARGV[0]) {
    node->update=_oscillator_update_silence;
  } else {
    if (oscillator_init_harmonics(node,argv,argc)<0) return -1;
  }
  return 0;
}

/* Type definition.
 */
 
const struct stdsyn_node_type stdsyn_node_type_oscillator={
  .name="oscillator",
  .objlen=sizeof(struct stdsyn_node_oscillator),
  .del=_oscillator_del,
  .init=_oscillator_init,
};
