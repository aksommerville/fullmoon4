/* bigpc_synth_dummy.c
 * Fake synth driver that discards events and generates silence.
 * If you want to disable audio, it would be wiser to dummy the audio driver.
 */
 
#include "bigpc_synth.h"
#include "bigpc_audio.h"
#include <string.h>

struct bigpc_synth_driver_dummy {
  struct bigpc_synth_driver hdr;
  int samplesize;
};

#define DRIVER ((struct bigpc_synth_driver_dummy*)driver)

static void _dummy_update(void *v,int c,struct bigpc_synth_driver *driver) {
  memset(v,0,c*DRIVER->samplesize);
}

static int _dummy_init(struct bigpc_synth_driver *driver) {
  switch (driver->format) {
    case BIGPC_AUDIO_FORMAT_s16n: DRIVER->samplesize=2; break;
    case BIGPC_AUDIO_FORMAT_f32n: DRIVER->samplesize=4; break;
    default: return -1;
  }
  driver->update=_dummy_update;
  return 0;
}

const struct bigpc_synth_type bigpc_synth_type_dummy={
  .name="dummy",
  .desc="Fake synthesizer that does nothing.",
  .objlen=sizeof(struct bigpc_synth_driver_dummy),
  .appointment_only=1,
  .init=_dummy_init,
};
