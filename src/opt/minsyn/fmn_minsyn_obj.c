#include "fmn_minsyn_internal.h"

/* Cleanup.
 */
 
static void _minsyn_del(struct bigpc_synth_driver *driver) {
}

/* Update, main.
 * Our natural mode is s16 mono.
 */
 
static void _minsyn_update_mono(int16_t *v,int c,struct bigpc_synth_driver *driver) {
  memset(v,0,c<<1);//TODO
}

/* Update, convert.
 */
 
static void _minsyn_update_stereo(int16_t *v,int c,struct bigpc_synth_driver *driver) {
  int framec=c>>1;
  _minsyn_update_mono(v,framec,driver);
  const int16_t *src=v+framec;
  int16_t *dst=v+c;
  while (framec-->0) {
    src--;
    *(--dst)=*src;
    *(--dst)=*src;
  }
}

/* Init.
 */
 
static int _minsyn_init(struct bigpc_synth_driver *driver) {

  if (driver->format!=BIGPC_AUDIO_FORMAT_s16n) return -1;
  if (driver->chanc==1) driver->update=(void*)_minsyn_update_mono;
  else if (driver->chanc==2) driver->update=(void*)_minsyn_update_stereo;
  else return -1;
  
  return 0;
}

/* Set data.
 */
 
static int _minsyn_set_instrument(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  return -1;
}

static int _minsyn_set_sound(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  return -1;
}

/* Play song.
 */
 
static int _minsyn_play_song(struct bigpc_synth_driver *driver,const void *src,int srcc,int force) {
  return -1;
}

/* Event.
 */
 
static void _minsyn_event(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
}

/* Type.
 */
 
const struct bigpc_synth_type bigpc_synth_type_minsyn={
  .name="minsyn",
  .desc="Simple synthesizer for resource-constrained systems.",
  .objlen=sizeof(struct bigpc_synth_driver_minsyn),
  .del=_minsyn_del,
  .init=_minsyn_init,
  .set_instrument=_minsyn_set_instrument,
  .set_sound=_minsyn_set_sound,
  .play_song=_minsyn_play_song,
  .event=_minsyn_event,
};
