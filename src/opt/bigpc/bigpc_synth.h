/* bigpc_synth.h
 * Synthesizers use a pluggable "driver" interface like hardware providers.
 * But of course, implementations tend to be pure software and will be the same choice for every build.
 * I'm thinking keep it loose, in case some platforms can get the good one, and some only the cheap one.
 */
 
#ifndef BIGPC_SYNTH_H
#define BIGPC_SYNTH_H

#include <stdint.h>

struct bigpc_synth_driver;
struct bigpc_synth_type;
struct bigpc_synth_config;

/* The (rate,chanc,format) you provide are dictates: Synth must accept them exactly, or fail.
 */
struct bigpc_synth_config {
  int rate;
  int chanc;
  int format; // BIGPC_AUDIO_FORMAT_*
  int music_enable;
};

/* Instance.
 *******************************************************/
 
struct bigpc_synth_driver {
  const struct bigpc_synth_type *type;
  int refc;
  int rate;
  int chanc;
  int format;
  int music_enable;
  int song_finished;
  
  /* Generate PCM.
   * (v) is an array of the type named by (config.format) at construction.
   * (c) is a count of samples -- not bytes, not frames.
   *
   * Drivers are required to set this, and it is required to overwrite the entire buffer, even in error cases.
   *
   * This lives on the instance instead of the type, in case a synthesizer
   * has different modes it can switch between at construction. (eg int vs float)
   */
  void (*update)(void *v,int c,struct bigpc_synth_driver *driver);
};

void bigpc_synth_del(struct bigpc_synth_driver *driver);
int bigpc_synth_ref(struct bigpc_synth_driver *driver);

struct bigpc_synth_driver *bigpc_synth_new(
  const struct bigpc_synth_type *type,
  const struct bigpc_synth_config *config
);

/* Upload data files.
 * Ideally, you'll do a bunch of these right after construction, before the first update or event.
 */
int bigpc_synth_set_instrument(struct bigpc_synth_driver *driver,int id,const void *src,int srcc);
int bigpc_synth_set_sound(struct bigpc_synth_driver *driver,int id,const void *src,int srcc);

void bigpc_synth_event(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b);
void bigpc_synth_release_all(struct bigpc_synth_driver *driver);
void bigpc_synth_silence_all(struct bigpc_synth_driver *driver);

/* Begin playing a song.
 * Caller must hold (src) constant for as long as it runs, which you don't know.
 * (force) to restart a song already in progress, otherwise synth should ignore duplicates.
 * (0,0) to stop whatever's playing.
 */
int bigpc_synth_play_song(struct bigpc_synth_driver *driver,const void *src,int srcc,int force,int loop);

/* When the song is paused, all notes get released, and it stops playing until you resume.
 * There will be a little bit of inconsistency on the resume, when notes expected on are in fact off.
 * Manual event processing continues as usual.
 * Intended for the Violin.
 */
void bigpc_synth_pause_song(struct bigpc_synth_driver *driver,int pause);

/* Type.
 *************************************************/
 
struct bigpc_synth_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  uint16_t data_qualifier;
  void (*del)(struct bigpc_synth_driver *driver);
  int (*init)(struct bigpc_synth_driver *driver);
  int (*set_instrument)(struct bigpc_synth_driver *driver,int id,const void *src,int srcc);
  int (*set_sound)(struct bigpc_synth_driver *driver,int id,const void *src,int srcc);
  void (*event)(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b);
  int (*play_song)(struct bigpc_synth_driver *driver,const void *src,int srcc,int force,int loop);
  void (*pause_song)(struct bigpc_synth_driver *driver,int pause);
  void (*enable_music)(struct bigpc_synth_driver *driver,int enable);
  int (*get_instrument_by_channel)(struct bigpc_synth_driver *driver,uint8_t chid);
  int (*get_song_position)(const struct bigpc_synth_driver *driver); // => roughly, how many frames into the song are we? must reset when song loops.
};

const struct bigpc_synth_type *bigpc_synth_type_by_index(int p);
const struct bigpc_synth_type *bigpc_synth_type_by_name(const char *name,int namec);

#endif
