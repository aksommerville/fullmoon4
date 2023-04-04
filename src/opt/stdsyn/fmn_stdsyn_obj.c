#include "fmn_stdsyn_internal.h"

/* Cleanup.
 */
 
static void _stdsyn_del(struct bigpc_synth_driver *driver) {
  if (DRIVER->qbuf) free(DRIVER->qbuf);
}

/* Update, main.
 * We only operate in mono with float samples.
 * But we also provide conversion after the fact, to stereo and s16.
 */
 
static void _stdsyn_update_mono_f32n(float *v,int c,struct bigpc_synth_driver *driver) {
  memset(v,0,sizeof(float)*c);//TODO
}

/* Update wrappers: quantize or expand stereo.
 */
 
static void _stdsyn_update_mono_s16n(int16_t *v,int c,struct bigpc_synth_driver *driver) {
  if (c>DRIVER->qbufa) {
    int na=(c+256)&~255;
    float *nv=realloc(DRIVER->qbuf,sizeof(float)*na);
    if (!nv) {
      memset(v,0,c<<1);
      return;
    }
    DRIVER->qbuf=nv;
    DRIVER->qbufa=na;
  }
  _stdsyn_update_mono_f32n(DRIVER->qbuf,c,driver);
  const float *src=DRIVER->qbuf;
  for (;c-->0;v++,src++) {
    *v=(int16_t)((*src)*DRIVER->qlevel);
  }
}

static void _stdsyn_update_stereo_s16n(int16_t *v,int c,struct bigpc_synth_driver *driver) {
  int framec=c>>1;
  _stdsyn_update_mono_s16n(v,framec,driver);
  const int16_t *src=v+framec;
  int16_t *dst=v+c;
  while (framec-->0) {
    src--;
    *(--dst)=*src;
    *(--dst)=*src;
  }
}

static void _stdsyn_update_stereo_f32n(float *v,int c,struct bigpc_synth_driver *driver) {
  int framec=c>>1;
  _stdsyn_update_mono_f32n(v,framec,driver);
  const float *src=v+framec;
  float *dst=v+c;
  while (framec-->0) {
    src--;
    *(--dst)=*src;
    *(--dst)=*src;
  }
}

/* Init.
 */
 
static int _stdsyn_init(struct bigpc_synth_driver *driver) {
  
  
  if ((driver->chanc==1)&&(driver->format==BIGPC_AUDIO_FORMAT_s16n)) {
    driver->update=(void*)_stdsyn_update_mono_s16n;
  } else if ((driver->chanc==2)&&(driver->format==BIGPC_AUDIO_FORMAT_s16n)) {
    driver->update=(void*)_stdsyn_update_stereo_s16n;
  } else if ((driver->chanc==1)&&(driver->format==BIGPC_AUDIO_FORMAT_f32n)) {
    driver->update=(void*)_stdsyn_update_mono_f32n;
  } else if ((driver->chanc==2)&&(driver->format==BIGPC_AUDIO_FORMAT_f32n)) {
    driver->update=(void*)_stdsyn_update_stereo_f32n;
  } else return -1;
  
  DRIVER->qlevel=32000.0f;
  
  return 0;
}

/* Set data.
 */
 
static int _stdsyn_set_instrument(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  return -1;
}

static int _stdsyn_set_sound(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  return -1;
}

/* Play song.
 */
 
static int _stdsyn_play_song(struct bigpc_synth_driver *driver,const void *src,int srcc,int force) {
  return -1;
}

/* Event.
 */
 
static void _stdsyn_event(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
}

/* Type.
 */
 
const struct bigpc_synth_type bigpc_synth_type_stdsyn={
  .name="stdsyn",
  .desc="The preferred synthesizer.",
  .objlen=sizeof(struct bigpc_synth_driver_stdsyn),
  .del=_stdsyn_del,
  .init=_stdsyn_init,
  .set_instrument=_stdsyn_set_instrument,
  .set_sound=_stdsyn_set_sound,
  .play_song=_stdsyn_play_song,
  .event=_stdsyn_event,
};
