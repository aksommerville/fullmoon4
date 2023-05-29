#include "fiddle_internal.h"

extern const struct bigpc_audio_type bigpc_audio_type_alsa;
extern const struct bigpc_synth_type bigpc_synth_type_minsyn;

/* Drop all drivers.
 */
 
void fiddle_drivers_quit() {
  fiddle_drivers_set(0);
}

/* PCM callback.
 */
 
static void fiddle_pcm_cb(void *v,int c,struct bigpc_audio_driver *audio) {
  if (fiddle.synth&&fiddle.synth->update) {
    fiddle.synth->update(v,c,fiddle.synth);
    
    const int16_t *src=v;
    int i=c;
    for (;i-->0;src++) {
      if (*src<fiddle.vumeter_lo) fiddle.vumeter_lo=*src;
      else if (*src>fiddle.vumeter_hi) fiddle.vumeter_hi=*src;
      float f=(*src)/32768.0f;
      fiddle.vumeter_sqsum+=f*f;
    }
    
  } else {
    memset(v,0,c<<1);
  }
  fiddle.vumeter_samplec+=c;
}

/* Initialize PCM-out driver.
 */
 
static int fiddle_pcm_init() {
  struct bigpc_audio_config config={
    .rate=44100,
    .chanc=1,
    .format=BIGPC_AUDIO_FORMAT_s16n,
    .device=0,
  };
  
  const struct bigpc_audio_type *type=0;
  #if FMN_USE_alsa
    type=&bigpc_audio_type_alsa;
  #endif
  if (!type) {
    fprintf(stderr,"%s:%d:%s: No audio driver.\n",__FILE__,__LINE__,__func__);
    return -1;
  }
  
  if (!(fiddle.audio=calloc(1,type->objlen))) return -1;
  fiddle.audio->type=type;
  fiddle.audio->refc=1;
  fiddle.audio->delegate.cb_pcm_out=fiddle_pcm_cb;
  fiddle.audio->rate=config.rate;
  fiddle.audio->chanc=config.chanc;
  fiddle.audio->format=config.format;
  if (fiddle.audio->type->init&&(fiddle.audio->type->init(fiddle.audio,&config)<0)) {
    if (fiddle.audio->type->del) fiddle.audio->type->del(fiddle.audio);
    free(fiddle.audio);
    fiddle.audio=0;
    return -1;
  }
  if (fiddle.audio->format!=BIGPC_AUDIO_FORMAT_s16n) {
    fprintf(stderr,"%s: Audio driver '%s' selected a format (%d) other than s16n. I refuse.\n",fiddle.exename,fiddle.audio->type->name,fiddle.audio->format);
    if (fiddle.audio->type->del) fiddle.audio->type->del(fiddle.audio);
    free(fiddle.audio);
    fiddle.audio=0;
    return -1;
  }
  return 0;
}

/* Initialize minsyn.
 */
 
static int fiddle_minsyn_init() {
  if (!fiddle.audio) return -1;
  if (!(fiddle.synth=calloc(1,bigpc_synth_type_minsyn.objlen))) return -1;
  fiddle.synth->type=&bigpc_synth_type_minsyn;
  fiddle.synth->refc=1;
  fiddle.synth->rate=fiddle.audio->rate;
  fiddle.synth->chanc=fiddle.audio->chanc;
  fiddle.synth->format=fiddle.audio->format;
  if (fiddle.synth->type->init&&(fiddle.synth->type->init(fiddle.synth)<0)) {
    if (fiddle.synth->type->del) fiddle.synth->type->del(fiddle.synth);
    free(fiddle.synth);
    fiddle.synth=0;
    return -1;
  }
  return 0;
}

/* Initialize stdsyn (not implemented)
 */
 
static int fiddle_stdsyn_init() {
  fprintf(stderr,"TODO %s %s:%d\n",__func__,__FILE__,__LINE__);
  return -1;
}

/* Load content to synth.
 */

static int fiddle_load_instrument(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  fiddle.synth->type->set_instrument(fiddle.synth,id,v,c);
  return 0;
}

static int fiddle_load_sound(uint16_t type,uint16_t qualifier,uint32_t id,const void *v,int c,void *userdata) {
  fiddle.synth->type->set_sound(fiddle.synth,id,v,c);
  return 0;
}

/* Set drivers to a qualifier.
 */
 
int fiddle_drivers_set(int qualifier) {

  // Drop whatever we have.
  // Dropping the synth driver implicitly ends the song.
  if (fiddle.audio) {
    if (fiddle.audio->type->play) fiddle.audio->type->play(fiddle.audio,0);
    if (fiddle.audio->type->del) fiddle.audio->type->del(fiddle.audio);
    free(fiddle.audio);
    fiddle.audio=0;
  }
  if (fiddle.synth) {
    if (fiddle.synth->type->del) fiddle.synth->type->del(fiddle.synth);
    free(fiddle.synth);
    fiddle.synth=0;
  }
  fiddle.songid=0;
  
  switch (qualifier) {
    case 1: { // WebAudio
        // What should we do here? I'm picturing something like, we tell the client to initialize a context?
        // Or should this be managed entirely on the client without our involvement?
      } break;
    case 2: { // minsyn
        if (fiddle_pcm_init()<0) return -1;
        if (fiddle_minsyn_init()<0) return -1;
      } break;
    case 3: { // stdsyn
        if (fiddle_pcm_init()<0) return -1;
        if (fiddle_stdsyn_init()<0) return -1;
      } break;
    // Anything else, eg zero, just leave the drivers unset.
  }
  
  if (fiddle.synth&&fiddle.datafile) {
    if (fiddle.synth->type->set_instrument) {
      fmn_datafile_for_each_of_qualified_type(
        fiddle.datafile,FMN_RESTYPE_INSTRUMENT,qualifier,
        fiddle_load_instrument,0
      );
    }
    if (fiddle.synth->type->set_sound) {
      fmn_datafile_for_each_of_qualified_type(
        fiddle.datafile,FMN_RESTYPE_SOUND,qualifier,
        fiddle_load_sound,0
      );
    }
  }
  
  return fiddle_drivers_play(1);
}

/* Access to drivers.
 */
 
int fiddle_drivers_play(int play) {
  if (!fiddle.audio) return 0;
  if (!fiddle.audio->type->play) return 0;
  fiddle.audio->type->play(fiddle.audio,play);
  return 0;
}

int fiddle_drivers_update() {
  if (!fiddle.audio) return 0;
  if (!fiddle.audio->type->update) return 0;
  return fiddle.audio->type->update(fiddle.audio);
}

int fiddle_drivers_lock() {
  if (!fiddle.audio) return 0;
  if (!fiddle.audio->type->lock) return 0;
  return fiddle.audio->type->lock(fiddle.audio);
}

void fiddle_drivers_unlock() {
  if (!fiddle.audio) return;
  if (!fiddle.audio->type->unlock) return;
  fiddle.audio->type->unlock(fiddle.audio);
}

void fiddle_synth_event(uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  if (fiddle_drivers_lock()<0) return;
  if (fiddle.synth&&fiddle.synth->type->event) {
    fiddle.synth->type->event(fiddle.synth,chid,opcode,a,b);
  }
  fiddle_drivers_unlock();
}
