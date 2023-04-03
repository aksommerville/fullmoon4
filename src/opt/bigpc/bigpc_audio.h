/* bigpc_audio.h
 * Generic interface for audio drivers.
 */
 
#ifndef BIGPC_AUDIO_H
#define BIGPC_AUDIO_H

#include <stdint.h>

struct bigpc_audio_driver;
struct bigpc_audio_type;
struct bigpc_audio_delegate;
struct bigpc_audio_config;

// Sample format.
#define BIGPC_AUDIO_FORMAT_s16n        1
#define BIGPC_AUDIO_FORMAT_f32n        2

#define BIGPC_AUDIO_FORMAT_FOR_EACH \
  _(s16n) \
  _(f32n)

/* (cb_pcm_out) can be called from a separate thread.
 */
struct bigpc_audio_delegate {
  void *userdata;
  void (*cb_pcm_out)(void *v,int c,struct bigpc_audio_driver *driver);
};

struct bigpc_audio_config {
  int rate;
  int chanc;
  int format;
  const char *device;
};

/* Driver instance.
 ********************************************************/
 
struct bigpc_audio_driver {
  const struct bigpc_audio_type *type;
  struct bigpc_audio_delegate delegate;
  int refc;
  int rate;
  int chanc;
  int format;
  int playing;
};

void bigpc_audio_driver_del(struct bigpc_audio_driver *driver);

/* What's requested in (config) is a suggestion only.
 * Driver should adjust if not acceptable.
 */
struct bigpc_audio_driver *bigpc_audio_driver_new(
  const struct bigpc_audio_type *type,
  const struct bigpc_audio_delegate *delegate,
  const struct bigpc_audio_config *config
);

/* Drivers should be stopped initially, and you must manually 'play' after construction.
 */
void bigpc_audio_play(struct bigpc_audio_driver *driver,int play);

/* Most drivers should spin off an I/O thread and update from there.
 * In that case, implement (lock) and (unlock), and guarantee the callback is not running while locked.
 * They could alternatively implement (update) and do all the I/O in there.
 * But we're vague about the interval between updates, so that's probably a bad idea.
 */
int bigpc_audio_update(struct bigpc_audio_driver *driver);
int bigpc_audio_lock(struct bigpc_audio_driver *driver);
void bigpc_audio_unlock(struct bigpc_audio_driver *driver);

/* Driver type.
 **********************************************************/
 
struct bigpc_audio_type {
  const char *name;
  const char *desc;
  int objlen;
  int appointment_only;
  void (*del)(struct bigpc_audio_driver *driver);
  int (*init)(struct bigpc_audio_driver *driver,const struct bigpc_audio_config *config);
  void (*play)(struct bigpc_audio_driver *driver,int play);
  int (*update)(struct bigpc_audio_driver *driver);
  int (*lock)(struct bigpc_audio_driver *driver);
  void (*unlock)(struct bigpc_audio_driver *driver);
};

const struct bigpc_audio_type *bigpc_audio_type_by_index(int p);
const struct bigpc_audio_type *bigpc_audio_type_by_name(const char *name,int namec);

#endif
