/* fmn_stdsyn_internal.h
 * The standard synthesizer, for reasonably capable platforms.
 * Requires opt unit "midi".
 */

#ifndef FMN_STDSYN_INTERNAL_H
#define FMN_STDSYN_INTERNAL_H

#include "opt/bigpc/bigpc_synth.h"
#include "opt/bigpc/bigpc_audio.h"
#include "opt/midi/midi.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include "stdsyn_env.h"

#define STDSYN_VOICE_LIMIT 128

struct stdsyn_voice {
//XXX ridiculously simple voice implementation, just to get something happening
  uint8_t chid,noteid;
  float level;
  int halfperiod;
  int phase;
  float t,dt;
  struct stdsyn_env env;
};

struct bigpc_synth_driver_stdsyn {
  struct bigpc_synth_driver hdr;
  float *qbuf;
  int qbufa;
  float qlevel; // 0..32767
  struct midi_file *song;
  struct stdsyn_voice *voicev;
  int voicec,voicea;
};

#define DRIVER ((struct bigpc_synth_driver_stdsyn*)driver)

/* Pump the signal graph for so many frames.
 * It must be zero initially.
 */
void stdsyn_generate_signal(float *v,int c,struct bigpc_synth_driver *driver);

void stdsyn_release_all(struct bigpc_synth_driver *driver);
void stdsyn_silence_all(struct bigpc_synth_driver *driver);

void stdsyn_voice_cleanup(struct stdsyn_voice *voice);
void stdsyn_voice_update(float *v,int c,struct stdsyn_voice *voice);
void stdsyn_voice_release(struct stdsyn_voice *voice);

#endif
