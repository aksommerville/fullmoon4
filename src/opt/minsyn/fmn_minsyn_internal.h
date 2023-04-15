/* fmn_minsyn_internal.h
 * Minimal synthesizer.
 * My Raspberry Pi 1 runs the game like butter, but can't handle the floating-point stdsyn.
 * Minsyn is an all-integer alternative, designed for performance at the expense of quality.
 */

#ifndef FMN_MINSYN_INTERNAL_H
#define FMN_MINSYN_INTERNAL_H

#include "opt/bigpc/bigpc_synth.h"
#include "opt/bigpc/bigpc_audio.h"
#include "opt/midi/midi.h"
#include "opt/datafile/fmn_datafile.h"
#include "opt/pcmprint/pcmprint.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MINSYN_VOICE_LIMIT 32
#define MINSYN_PLAYBACK_LIMIT 32

/* Wave size must be a power of 2.
 * Then use the full range of a uint32_t as phase, and shift by MINSYN_WAVE_SHIFT for the offset.
 */
#define MINSYN_WAVE_SIZE_BITS 10
#define MINSYN_WAVE_SIZE_SAMPLES (1<<MINSYN_WAVE_SIZE_BITS)
#define MINSYN_WAVE_SHIFT (32-MINSYN_WAVE_SIZE_BITS)

struct minsyn_env {
  int32_t v; // current level in 24 bits
  int32_t dv;
  int32_t c; // frames to next stage
  uint8_t stage; // 0,1,2,3=attack,decay,sustain,release
  uint8_t released;
  uint8_t term; // nonzero if terminated
  
  int32_t atkv,susv; // velocity-adjusted attack and sustain levels in 24 bits
  int32_t atkc,decc,rlsc; // velocity-adjusted attack, decay, release times in frames
  
  // Constantish:
  int32_t atkvlo,atkvhi; // attack level, 24 bits
  int32_t susvlo,susvhi; // sustain level, 24 bits
  int32_t atkclo,atkchi; // attack time, frames
  int32_t decclo,decchi; // decay time, frames
  int32_t rlsclo,rlschi; // release time, frames
};

struct minsyn_env_config {
  int32_t atkv,susv,atkc,decc,rlsc;
};

/* minsyn_voice for tuned notes with a flexible envelope.
 */
struct minsyn_voice {
  uint8_t chid,noteid; // 0xff,0xff if unaddressable (must self-terminate)
  const int16_t *v; // WEAK, points into one of (DRIVER->wavev). null if terminated
  uint32_t p;
  uint32_t dp;
  struct minsyn_env env;
};

/* minsyn_playback for PCM at the natural rate with an optional loop.
 */
struct minsyn_playback {
  uint8_t chid,noteid; // 0xff,0xff if unaddressable
  struct minsyn_pcm *pcm; // STRONG, we can't guarantee that (DRIVER->pcmv) retain objects forever. null if terminated
  int p;
  int loop;
};

/* Read-only-ish PCM dump. We write each once.
 */
struct minsyn_pcm {
  int16_t *v;
  int c;
  int loopa,loopz;
  int id;
  int refc;
};

/* Read-only single-period wave.
 */
struct minsyn_wave {
  int id;
  int16_t v[MINSYN_WAVE_SIZE_SAMPLES];
};

struct minsyn_printer {
  struct minsyn_pcm *pcm;
  struct pcmprint *pcmprint;
  int p;
};

/* Loaded instrument or sound. At initial load, we only copy the serial data.
 */
struct minsyn_resource {
  int type; // FMN_RESTYPE_INSTRUMENT|FMN_RESTYPE_SOUND
  int id;
  void *v;
  int c;
  int ready;
  int waveid; // >0 if there is one
  int pcmid; // >0 if there is one
  struct minsyn_env_config envlo,envhi;
};

struct bigpc_synth_driver_minsyn {
  struct bigpc_synth_driver hdr;
  struct minsyn_voice *voicev;
  int voicec,voicea;
  struct minsyn_playback *playbackv;
  int playbackc,playbacka;
  struct midi_file *song;
  struct minsyn_pcm **pcmv;
  int pcmc,pcma;
  struct minsyn_wave **wavev;
  int wavec,wavea;
  struct minsyn_printer **printerv;
  int printerc,printera;
  int update_in_progress_framec; // pre-run any new printers to this length (we're in the middle of an update)
  struct minsyn_resource *resourcev;
  int resourcec,resourcea;
  int16_t sine[MINSYN_WAVE_SIZE_SAMPLES]; // reference wave for generating new ones.
  int instrument_by_chid[16];
};

#define DRIVER ((struct bigpc_synth_driver_minsyn*)driver)

/* "init" discards any existing resource for this id and returns a fresh one.
 * "get" only returns existing resources. Getting does not finish decoding.
 */
void minsyn_resource_cleanup(struct minsyn_resource *resource);
struct minsyn_resource *minsyn_resource_init_instrument(struct bigpc_synth_driver *driver,int id);
struct minsyn_resource *minsyn_resource_init_sound(struct bigpc_synth_driver *driver,int id);
struct minsyn_resource *minsyn_resource_get_instrument(const struct bigpc_synth_driver *driver,int id);
struct minsyn_resource *minsyn_resource_get_sound(const struct bigpc_synth_driver *driver,int id);

/* Call each time you want to use a resource.
 * This sets its "ready" flag, subsequent readies will quickly noop.
 * Populates (waveid,pcmid,env) as warranted.
 * Adds waves, pcms, and printers to the driver as warranted.
 */
int minsyn_resource_ready(struct bigpc_synth_driver *driver,struct minsyn_resource *resource);

void minsyn_voice_cleanup(struct minsyn_voice *voice);
void minsyn_voice_release(struct minsyn_voice *voice);

/* (voice->chid,noteid) must be set, and the rest zeroed.
 * No errors. If anything goes wrong, we'll quietly mark the voice defunct.
 */
void minsyn_voice_init(
  struct bigpc_synth_driver *driver,
  struct minsyn_voice *voice,
  uint8_t velocity,
  struct minsyn_resource *instrument
);

void minsyn_playback_cleanup(struct minsyn_playback *playback);
void minsyn_playback_release(struct minsyn_playback *playback);

void minsyn_playback_init(struct bigpc_synth_driver *driver,struct minsyn_playback *playback,struct minsyn_pcm *pcm);

/* Buffer must be zeroed initially, we will add to it.
 */
void minsyn_generate_signal(int16_t *v,int c,struct bigpc_synth_driver *driver);

void minsyn_release_all(struct bigpc_synth_driver *driver);
void minsyn_silence_all(struct bigpc_synth_driver *driver);

struct minsyn_wave *minsyn_get_wave(struct bigpc_synth_driver *driver,int id);
int minsyn_new_wave_harmonics(struct bigpc_synth_driver *driver,const uint8_t *coefv,int coefc);
void minsyn_generate_sine(int16_t *v,int c); // should only be done once; after init use DRIVER->sine

void minsyn_pcm_del(struct minsyn_pcm *pcm);
int minsyn_pcm_ref(struct minsyn_pcm *pcm);
struct minsyn_pcm *minsyn_pcm_by_id(const struct bigpc_synth_driver *driver,int id);
struct minsyn_pcm *minsyn_pcm_new(struct bigpc_synth_driver *driver,int c); // => WEAK

void minsyn_printer_del(struct minsyn_printer *printer);
int minsyn_printer_update(struct minsyn_printer *printer,int c); // <=0 if terminated

// (pcmprint) HANDOFF, (pcm) STRONG, returns WEAK.
struct minsyn_printer *minsyn_printer_new(struct bigpc_synth_driver *driver,struct pcmprint *pcmprint,struct minsyn_pcm *pcm);

void minsyn_env_reset(struct minsyn_env *env,uint8_t velocity,int mainrate);
void minsyn_env_release(struct minsyn_env *env);
void minsyn_env_advance(struct minsyn_env *env); // only minsyn_env_next, below, should call this

static inline uint8_t minsyn_env_next(struct minsyn_env *env) {
  if (env->term) return 0;
  if (!env->c) minsyn_env_advance(env);
  env->c--;
  env->v+=env->dv;
  return env->v>>16;
}

static inline void minsyn_signal_add_v(int16_t *dst,const int16_t *src,int c) {
  for (;c-->0;dst++,src++) (*dst)+=(*src);
}

#endif
