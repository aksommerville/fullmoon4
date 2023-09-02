/* fmn_stdsyn_internal.h
 * The standard synthesizer, for reasonably capable platforms.
 * Requires opt unit "midi".
 */

#ifndef FMN_STDSYN_INTERNAL_H
#define FMN_STDSYN_INTERNAL_H

/* Memory/speed balance.
 * No point going above the PCM driver's buffer size (1024 is typical).
 * Nodes may assume that they'll never be asked to produce more than so many samples at a time.
 */
#define STDSYN_BUFFER_SIZE 512

/* 1 M samples, about 24 seconds at 44.1 kHz. (but it's the limit regardless of rate).
 * We forbid anything longer than this, as a general safety net.
 */
#define STDSYN_PCM_SANITY_LIMIT (1<<20)

/* Waves are single-period PCM dumps of a fixed size.
 */
#define STDSYN_WAVE_SIZE_BITS 10
#define STDSYN_WAVE_SIZE_SAMPLES (1<<STDSYN_WAVE_SIZE_BITS)
#define STDSYN_WAVE_SHIFT (32-STDSYN_WAVE_SIZE_BITS)

#include "opt/bigpc/bigpc_synth.h"
#include "opt/bigpc/bigpc_audio.h"
#include "opt/midi/midi.h"
#include "opt/pcmprint/pcmprint.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "opt/stdsyn/bits/stdsyn_env.h"

struct stdsyn_node;
struct stdsyn_node_type;
struct stdsyn_pcm;
struct stdsyn_printer;
struct stdsyn_wave;
struct stdsyn_wave_runner;
struct stdsyn_instrument;
struct stdsyn_sound; // store unit

/* Generic node.
 ***********************************************************************/
 
struct stdsyn_node_type {
  const char *name;
  int objlen;
  
  void (*del)(struct stdsyn_node *node);
  
  /* Must fail if (chanc) or (overwrite) disagreeable.
   */
  int (*init)(struct stdsyn_node *node,uint8_t velocity);
  
  /* Channel controllers must implement.
   */
  int (*apply_instrument)(struct stdsyn_node *node,const struct stdsyn_instrument *ins);
  
  // Most of what you'd expect to be type hooks live on instances instead.
  // (So you can have different implementations per hook, depending on config).
};
 
struct stdsyn_node {

  /* Managed by driver. Implementations must not modify.
   */
  const struct stdsyn_node_type *type;
  struct bigpc_synth_driver *driver; // WEAK
  int refc;
  int chanc; // 1 or 2. Not necessarily the same as our output.
  int overwrite; // Nonzero if (update) is required to overwrite content. Otherwise it is in+out.
  uint8_t chid,noteid; // 0xff if unaddressable.
  
  /* Implementation sets nonzero once it becomes unable to generate any more signal.
   * This would normally be a little after (release), when your envelope completes.
   */
  int defunct;
  
  /* All nodes that generate a signal must populate this at init.
   * (c) is in samples -- mono or stereo was established before init.
   * You must respect the (overwrite) flag set before init.
   */
  void (*update)(float *v,int c,struct stdsyn_node *node);
  
  /* Opportunity for routine maintenance at the end of each driver cycle.
   * eg check for defunct sources.
   * Better to use this hook than piggyback on regular (update), since that might happen at fairly high frequency.
   */
  void (*lfupdate)(struct stdsyn_node *node);
  
  /* Optional specific MIDI events.
   * You should implement (release) if you're a voice controller.
   */
  void (*release)(struct stdsyn_node *node,uint8_t velocity);
  void (*adjust)(struct stdsyn_node *node,uint8_t velocity);
  void (*bend)(struct stdsyn_node *node,int16_t raw,float mlt); // (raw) -8192..8192, (mlt) scaled per outer config
  
  /* General MIDI events.
   * Only channel controllers need to implement this.
   * Return nonzero to acknowledge an event, or zero for the caller to continue searching recipients.
   */
  int (*event)(struct stdsyn_node *node,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b);
  void (*tempo)(struct stdsyn_node *node,int frames_per_qnote);
  
  /* Sources are managed by the implementation.
   * Only their storage is at the generic level, as a convenience.
   */
  struct stdsyn_node **srcv;
  int srcc,srca;
};

void stdsyn_node_del(struct stdsyn_node *node);
int stdsyn_node_ref(struct stdsyn_node *node);

struct stdsyn_node *stdsyn_node_new(
  struct bigpc_synth_driver *driver,
  const struct stdsyn_node_type *type,
  int chanc,int overwrite,
  uint8_t noteid,uint8_t velocity
);

struct stdsyn_node *stdsyn_node_spawn_source(
  struct stdsyn_node *parent,
  const struct stdsyn_node_type *type,
  int chanc,int overwrite,
  uint8_t noteid,uint8_t velocity
);

struct stdsyn_node *stdsyn_node_new_controller(
  struct bigpc_synth_driver *driver,
  int chanc,int overwrite,
  const struct stdsyn_instrument *ins
);

int stdsyn_node_srcv_insert(struct stdsyn_node *parent,int p,struct stdsyn_node *child); // (p<0) to append
int stdsyn_node_srcv_remove_at(struct stdsyn_node *parent,int p,int c);
int stdsyn_node_srcv_remove(struct stdsyn_node *parent,struct stdsyn_node *child);

extern const struct stdsyn_node_type stdsyn_node_type_mixer; // Full bus. src are signal nodes.
extern const struct stdsyn_node_type stdsyn_node_type_ctl3; // Multi-voice program with shared intro and outro legs.
extern const struct stdsyn_node_type stdsyn_node_type_ctlv; // Voice controller. Produces 'basic' voices.
extern const struct stdsyn_node_type stdsyn_node_type_ctlp; // PCM controller, eg drum kit. Each voice is plain PCM.
extern const struct stdsyn_node_type stdsyn_node_type_ctlm; // Controller for minsyn-format instruments.
extern const struct stdsyn_node_type stdsyn_node_type_minsyn; // Voice for ctlm.
extern const struct stdsyn_node_type stdsyn_node_type_basic; // Single voice with just oscillator and envelope.
extern const struct stdsyn_node_type stdsyn_node_type_pcm; // Single voice dumping verbatim PCM.
extern const struct stdsyn_node_type stdsyn_node_type_oscillator; // Primitive oscillators.
extern const struct stdsyn_node_type stdsyn_node_type_subtract; // Pseudo-oscillator, can accept input (white noise).
extern const struct stdsyn_node_type stdsyn_node_type_fm; // FM oscillator.
extern const struct stdsyn_node_type stdsyn_node_type_env; // Envelope, can accept input.
extern const struct stdsyn_node_type stdsyn_node_type_gain; // Filter.
extern const struct stdsyn_node_type stdsyn_node_type_delay; // Filter.
//TODO FIR? IIR? Detune? Reverb?

int stdsyn_node_mixer_add_voice(struct stdsyn_node *node,struct stdsyn_node *voice);
int stdsyn_node_pcm_set_pcm(struct stdsyn_node *node,struct stdsyn_pcm *pcm);
int stdsyn_node_minsyn_setup(
  struct stdsyn_node *node,
  struct stdsyn_wave *wave,
  struct stdsyn_wave *mixwave,
  const struct stdsyn_env *env,
  const struct stdsyn_env *mixenv,
  float trim,float pan
);
int stdsyn_node_basic_setup_fm(
  struct stdsyn_node *node,
  struct stdsyn_wave *carrier,
  int abs_mod_rate,
  float rate,
  const struct stdsyn_env *rangeenv,
  const struct stdsyn_env *env,
  float trim,float pan
);

/* Single-period wave helper.
 **************************************************************/
 
struct stdsyn_wave {
  int refc;
  float v[STDSYN_WAVE_SIZE_SAMPLES];
};

void stdsyn_wave_del(struct stdsyn_wave *wave);
int stdsyn_wave_ref(struct stdsyn_wave *wave);
struct stdsyn_wave *stdsyn_wave_new();

struct stdsyn_wave *stdsyn_wave_from_harmonics(const uint8_t *v,int c);

/* Allocate statically, all zeroes.
 */
struct stdsyn_wave_runner {
  struct stdsyn_wave *wave;
  uint32_t p;
  uint32_t dp;
};

void stdsyn_wave_runner_cleanup(struct stdsyn_wave_runner *runner);

int stdsyn_wave_runner_set_wave(struct stdsyn_wave_runner *runner,struct stdsyn_wave *wave);
void stdsyn_wave_runner_set_rate_norm(struct stdsyn_wave_runner *runner,float cycles_per_frame); // should be in 0..0.5
void stdsyn_wave_runner_set_rate_hz(struct stdsyn_wave_runner *runner,float hz,float mainrate); // hz should be <= mainrate/2

/* Update at a flat rate.
 */
void stdsyn_wave_runner_update(float *v,int c,struct stdsyn_wave_runner *runner);

/* Multiply rate by (mod) at each step -- must be the same length as (v).
 */
void stdsyn_wave_runner_update_mod(float *v,int c,struct stdsyn_wave_runner *runner,const float *mod);

/* Flat rate update, pulling one sample at a time.
 */
static inline float stdsyn_wave_runner_step(struct stdsyn_wave_runner *runner) {
  float v=runner->wave->v[runner->p>>STDSYN_WAVE_SHIFT];
  runner->p+=runner->dp;
  return v;
}

/* PCM printer.
 ****************************************************************/
 
struct stdsyn_pcm {
  int refc;
  int c;
  float v[];
};

void stdsyn_pcm_del(struct stdsyn_pcm *pcm);
int stdsyn_pcm_ref(struct stdsyn_pcm *pcm);

struct stdsyn_pcm *stdsyn_pcm_new(int framec);
 
struct stdsyn_printer {
  struct pcmprint *pcmprint;
  struct stdsyn_pcm *pcm;
  uint8_t soundid;
  int p;
};

void stdsyn_printer_del(struct stdsyn_printer *printer);

/* Allocates its (pcm) during construction. Never null after a success.
 * PCM content is initially zeroed.
 */
struct stdsyn_printer *stdsyn_printer_new(
  int rate,const void *src,int srcc
);

/* <0 on errors, 0 if complete, either of those you should stop calling.
 */
int stdsyn_printer_update(struct stdsyn_printer *printer,int framec);

/* Resource store.
 *********************************************************************/
 
struct stdsyn_res_store {
  struct bigpc_synth_driver *driver;
  void (*del)(void *obj);
  void *(*decode)(struct bigpc_synth_driver *driver,int id,const void *v,int c);
  struct stdsyn_res {
    int id;
    void *v;
    int c;
    void *obj;
  } *resv;
  int resc,resa;
};

/* The resource stores decode objects only on the first get, and hold on to the first decode.
 */
void stdsyn_res_store_cleanup(struct stdsyn_res_store *store);
int stdsyn_res_store_add(struct stdsyn_res_store *store,int id,const void *v,int c);
void *stdsyn_res_store_get(struct stdsyn_res_store *store,int id);

struct stdsyn_instrument {
  const struct stdsyn_node_type *type;
  
  // ctlm (minsyn):
  struct stdsyn_wave *wave;
  struct stdsyn_wave *mixwave;
  struct stdsyn_env env; // both minsyn and stdsyn, but they'd be sourced by different data
  struct stdsyn_env mixenv;
  
  // stdsyn features:
  int fmabs;
  float fmrate;
  struct stdsyn_env fmenv;
};

void stdsyn_instrument_del(struct stdsyn_instrument *ins);
struct stdsyn_instrument *stdsyn_instrument_decode(struct bigpc_synth_driver *driver,int id,const void *v,int c);

struct stdsyn_pcm *stdsyn_sound_decode(struct bigpc_synth_driver *driver,int id,const void *v,int c);

/* Driver globals.
 ***********************************************************************/

struct bigpc_synth_driver_stdsyn {
  struct bigpc_synth_driver hdr;
  float *qbuf;
  int qbufa;
  float qlevel; // 0..32767
  struct midi_file *song;
  int songpause;
  struct stdsyn_node *main;
  struct stdsyn_res_store instruments;
  struct stdsyn_res_store sounds;
  struct stdsyn_printer **printerv;
  int printerc,printera;
  int update_pending_framec;
};

#define DRIVER ((struct bigpc_synth_driver_stdsyn*)driver)
#define NDRIVER ((struct bigpc_synth_driver_stdsyn*)(node->driver))

void stdsyn_release_all(struct bigpc_synth_driver *driver);
void stdsyn_silence_all(struct bigpc_synth_driver *driver);

/* Instantiates and installs printer, and pre-runs if an update is in progress.
 */
struct stdsyn_printer *stdsyn_begin_print(struct bigpc_synth_driver *driver,int id,const void *src,int srcc);

#endif
