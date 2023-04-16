#include "macaudio.h"
#include "opt/bigpc/bigpc_audio.h"

/* Instance definition.
 */

struct bigpc_audio_driver_macaudio {
  struct bigpc_audio_driver hdr;
  struct macaudio *macaudio;
};

#define DRIVER ((struct bigpc_audio_driver_macaudio*)driver)

/* Cleanup.
 */

static void _macaudio_del(struct bigpc_audio_driver *driver) {
  macaudio_del(DRIVER->macaudio);
}

/* Init.
 */

static int _macaudio_init(struct bigpc_audio_driver *driver,const struct bigpc_audio_config *config) {
  struct macaudio_delegate delegate={
    .userdata=driver,
    .pcm_out=(void*)driver->delegate.cb_pcm_out,
  };
  struct macaudio_setup setup={
    .rate=config->rate,
    .chanc=config->chanc,
  };
  if (!(DRIVER->macaudio=macaudio_new(&delegate,&setup))) return -1;
  driver->rate=macaudio_get_rate(DRIVER->macaudio);
  driver->chanc=macaudio_get_chanc(DRIVER->macaudio);
  driver->format=BIGPC_AUDIO_FORMAT_s16n;
  return 0;
}

/* Play, lock, unlock.
 */

static void _macaudio_play(struct bigpc_audio_driver *driver,int play) {
  macaudio_play(DRIVER->macaudio,play);
  driver->playing=play;
}

static int _macaudio_lock(struct bigpc_audio_driver *driver) {
  return macaudio_lock(DRIVER->macaudio);
}

static void _macaudio_unlock(struct bigpc_audio_driver *driver) {
  macaudio_unlock(DRIVER->macaudio);
}

/* Type definition.
 */

const struct bigpc_audio_type bigpc_audio_type_macaudio={
  .name="macaudio",
  .desc="Audio out via AudioUnit, for MacOS",
  .objlen=sizeof(struct bigpc_audio_driver_macaudio),
  .del=_macaudio_del,
  .init=_macaudio_init,
  .play=_macaudio_play,
  .lock=_macaudio_lock,
  .unlock=_macaudio_unlock,
};
