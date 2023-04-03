#include "bigpc_audio.h"
#include <sys/time.h>
#include <stdlib.h>

/* In case of clock faults or other weirdness, we enforce a hard limit in real time.
 * If you go longer than this between updates, the audio's time will fall behind.
 * I think 10 Hz sounds fair?
 */
#define BIGPC_AUDIO_DUMMY_UPDATE_LIMIT_US 100000

/* Keep this large enough that we don't have to segment calls out to the synthesizer very much.
 * That segmentation kills performance.
 * 1k should be plenty, and hey why not 4.
 */
#define BIGPC_AUDIO_DUMMY_BUFFER_LIMIT_BYTES 4096

/* Instance definition.
 */
 
struct bigpc_audio_driver_dummy {
  struct bigpc_audio_driver hdr;
  int samplesize;
  int64_t clock; // Last observed real time, us. Zero initially; start tracking at the first update.
  double frames_per_us;
  void *buf;
  int bufa_frames;
};

#define DRIVER ((struct bigpc_audio_driver_dummy*)driver)

/* Cleanup.
 */
 
static void _dummy_del(struct bigpc_audio_driver *driver) {
  if (DRIVER->buf) free(DRIVER->buf);
}

/* Current time.
 */
 
static int64_t dummy_now() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (int64_t)tv.tv_sec*1000000ll+tv.tv_usec;
}

/* Init.
 */
 
static int _dummy_init(struct bigpc_audio_driver *driver,const struct bigpc_audio_config *config) {
  if (!config) return -1;
  
  if (config->rate<200) driver->rate=200;
  else if (config->rate>200000) driver->rate=200000;
  else driver->rate=config->rate;
  
  if (config->chanc<1) driver->chanc=1;
  else if (config->chanc>2) driver->chanc=2;
  else driver->chanc=config->chanc;
  
  driver->format=config->format;
  switch (config->format) {
    case BIGPC_AUDIO_FORMAT_s16n: DRIVER->samplesize=2; break;
    case BIGPC_AUDIO_FORMAT_f32n: DRIVER->samplesize=4; break;
    default: driver->format=BIGPC_AUDIO_FORMAT_s16n; DRIVER->samplesize=2; break;
  }
  
  DRIVER->frames_per_us=driver->rate/1000000.0;
  
  int bytes_per_frame=DRIVER->samplesize*driver->chanc;
  DRIVER->bufa_frames=BIGPC_AUDIO_DUMMY_BUFFER_LIMIT_BYTES/bytes_per_frame;
  if (!(DRIVER->buf=malloc(DRIVER->bufa_frames*bytes_per_frame))) return -1;
  
  return 0;
}

/* Generate and discard audio, arbitrarily large amount.
 */
 
static void dummy_generate_frames_1(struct bigpc_audio_driver *driver,int framec) {
  int samplec=framec*driver->chanc;
  driver->delegate.cb_pcm_out(DRIVER->buf,samplec,driver);
  // Opportunity here to check levels, dump to a WAV file, that kind of thing...
}
 
static void dummy_generate_frames(struct bigpc_audio_driver *driver,int framec) {
  while (framec>=DRIVER->bufa_frames) {
    dummy_generate_frames_1(driver,DRIVER->bufa_frames);
    framec-=DRIVER->bufa_frames;
  }
  if (framec>0) {
    dummy_generate_frames_1(driver,framec);
  }
}

/* Play/pause.
 */
 
static void _dummy_play(struct bigpc_audio_driver *driver,int play) {
  if (driver->playing=play) {
    DRIVER->clock=dummy_now();
  }
}

/* Update.
 */
 
static int _dummy_update(struct bigpc_audio_driver *driver) {
  if (!driver->playing) return 0;
  int64_t now=dummy_now();
  if (DRIVER->clock) {
    int64_t elapsedus=now-DRIVER->clock;
    if (elapsedus>BIGPC_AUDIO_DUMMY_UPDATE_LIMIT_US) {
      elapsedus=BIGPC_AUDIO_DUMMY_UPDATE_LIMIT_US;
    } else if (elapsedus<0) {
      elapsedus=0;
    }
    // There's sure to be some round-off error here, which if we were a real driver we'd probably want to account for.
    int framec=(int)(elapsedus*DRIVER->frames_per_us);
    if (framec>0) {
      dummy_generate_frames(driver,framec);
    }
  }
  DRIVER->clock=now;
  return 0;
}

/* Type definition.
 */
 
const struct bigpc_audio_type bigpc_audio_type_dummy={
  .name="dummy",
  .desc="Stub audio driver that keeps time but discards output.",
  .objlen=sizeof(struct bigpc_audio_driver_dummy),
  .appointment_only=0, // Let it be used, as last resort. Running with no sound might be acceptable.
  .del=_dummy_del,
  .init=_dummy_init,
  .play=_dummy_play,
  .update=_dummy_update,
};
