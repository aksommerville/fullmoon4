/* alsapcm_glue.c
 * Connects the pre-written alsapcm unit to Full Moon's bigpc unit.
 */
 
#include "alsapcm.h"
#include "opt/bigpc/bigpc_audio.h"

struct bigpc_audio_driver_alsa {
  struct bigpc_audio_driver hdr;
  struct alsapcm *alsapcm;
};

#define DRIVER ((struct bigpc_audio_driver_alsa*)driver)

static void _alsa_del(struct bigpc_audio_driver *driver) {
  alsapcm_del(DRIVER->alsapcm);
}

static int _alsa_init(struct bigpc_audio_driver *driver,const struct bigpc_audio_config *config) {
  
  struct alsapcm_delegate delegate={
    .userdata=driver,
    .pcm_out=(void*)driver->delegate.cb_pcm_out,
  };
  struct alsapcm_setup setup={
    .rate=config->rate,
    .chanc=config->chanc,
    .device=config->device,
    .buffersize=0, // TODO configurable? Zero lets alsapcm decide.
  };
  
  if (!(DRIVER->alsapcm=alsapcm_new(&delegate,&setup))) return -1;
  
  driver->rate=alsapcm_get_rate(DRIVER->alsapcm);
  driver->chanc=alsapcm_get_chanc(DRIVER->alsapcm);
  driver->format=BIGPC_AUDIO_FORMAT_s16n;
  
  return 0;
}

static void _alsa_play(struct bigpc_audio_driver *driver,int play) {
  alsapcm_set_running(DRIVER->alsapcm,play);
}

static int _alsa_update(struct bigpc_audio_driver *driver) {
  // 'update' is for error reporting only
  return alsapcm_update(DRIVER->alsapcm);
}

static int _alsa_lock(struct bigpc_audio_driver *driver) {
  return alsapcm_lock(DRIVER->alsapcm);
}

static void _alsa_unlock(struct bigpc_audio_driver *driver) {
  return alsapcm_unlock(DRIVER->alsapcm);
}

const struct bigpc_audio_type bigpc_audio_type_alsa={
  .name="alsa",
  .desc="Audio for Linux via ALSA.",
  .objlen=sizeof(struct bigpc_audio_driver_alsa),
  .appointment_only=0,
  .del=_alsa_del,
  .init=_alsa_init,
  .play=_alsa_play,
  .update=_alsa_update,
  .lock=_alsa_lock,
  .unlock=_alsa_unlock,
};
