#include "pulse_internal.h"
#include "opt/bigpc/bigpc_audio.h"

/* Instance definition.
 */
 
struct bigpc_audio_driver_pulse {
  struct bigpc_audio_driver hdr;
  struct pulse *pulse;
};

#define DRIVER ((struct bigpc_audio_driver_pulse*)driver)

/* Hooks.
 */
 
static void _pulse_del(struct bigpc_audio_driver *driver) {
  pulse_del(DRIVER->pulse);
}

static void _pulse_cb(int16_t *v,int c,struct pulse *pulse) {
  struct bigpc_audio_driver *driver=pulse_get_userdata(pulse);
  driver->delegate.cb_pcm_out(v,c,driver->delegate.userdata);
}

static int _pulse_init(
  struct bigpc_audio_driver *driver,
  const struct bigpc_audio_config *config
) {
  //TODO Can we get appname from the caller?
  if (config) {
    driver->rate=config->rate;
    driver->chanc=config->chanc;
  }
  if (!driver->rate) driver->rate=44100;
  if (!driver->chanc) driver->chanc=2;
  if (!(DRIVER->pulse=pulse_new(driver->rate,driver->chanc,_pulse_cb,driver,""))) return -1;
  driver->rate=pulse_get_rate(DRIVER->pulse);
  driver->chanc=pulse_get_chanc(DRIVER->pulse);
  driver->format=BIGPC_AUDIO_FORMAT_s16n;
  return 0;
}

static void _pulse_play(struct bigpc_audio_driver *driver,int play) {
  pulse_play(DRIVER->pulse,play);
  driver->playing=play?1:0;
}

static int _pulse_lock(struct bigpc_audio_driver *driver) {
  return pulse_lock(DRIVER->pulse);
}

static void _pulse_unlock(struct bigpc_audio_driver *driver) {
  pulse_unlock(DRIVER->pulse);
}

/* Type definition.
 */
 
const struct bigpc_audio_type bigpc_audio_type_pulse={
  .name="pulse",
  .desc="Audio for Linux via PulseAudio, preferred for desktop systems.",
  .objlen=sizeof(struct bigpc_audio_driver_pulse),
  .del=_pulse_del,
  .init=_pulse_init,
  .play=_pulse_play,
  .lock=_pulse_lock,
  .unlock=_pulse_unlock,
};
