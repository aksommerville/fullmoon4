#include "fmn_stdsyn_internal.h"

/* Cleanup.
 */
 
static void _stdsyn_del(struct bigpc_synth_driver *driver) {
  if (DRIVER->qbuf) free(DRIVER->qbuf);
  
  if (DRIVER->voicev) {
    while (DRIVER->voicec-->0) {
      stdsyn_voice_cleanup(DRIVER->voicev+DRIVER->voicec);
    }
    free(DRIVER->voicev);
  }
  
  midi_file_del(DRIVER->song);
}

/* Drop voices that have run out of funk.
 */
 
static void stdsyn_drop_defunct_voices(struct bigpc_synth_driver *driver) {
  int i=DRIVER->voicec;
  struct stdsyn_voice *voice=DRIVER->voicev+i-1;
  for (;i-->0;voice--) {
    if (voice->halfperiod<1) {
      stdsyn_voice_cleanup(voice);
      DRIVER->voicec--;
      memmove(voice,voice+1,sizeof(struct stdsyn_voice)*(DRIVER->voicec-i));
    }
  }
}

/* Update, main.
 * We only operate in mono with float samples.
 * But we also provide conversion after the fact, to stereo and s16.
 */
 
static void _stdsyn_update_mono_f32n(float *v,int c,struct bigpc_synth_driver *driver) {
  memset(v,0,sizeof(float)*c);
  while (c>0) {
  
    int updc=c;
    if (DRIVER->song) {
      struct midi_event event={0};
      updc=midi_file_next(&event,DRIVER->song,0);
      if (updc<0) {
        fprintf(stderr,"stdsyn: song end or error\n");
        midi_file_del(DRIVER->song);
        DRIVER->song=0;
        continue;
      }
      if (updc) { // no song event; proceed with signal
        if (updc>c) updc=c;
      } else {
        driver->type->event(driver,event.chid,event.opcode,event.a,event.b);
        continue;
      }
    }
    
    stdsyn_generate_signal(v,updc,driver);
    v+=updc;
    c-=updc;
    if (DRIVER->song) midi_file_advance(DRIVER->song,updc);
  }
  stdsyn_drop_defunct_voices(driver);
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
  if (srcc<0) return -1;
  if (!srcc) {
    if (!DRIVER->song) return 0;
  }
  if (DRIVER->song) {
    if (!force) {
      if (DRIVER->song->v==src) return 0;
    }
    midi_file_del(DRIVER->song);
    DRIVER->song=0;
    stdsyn_release_all(driver);
  }
  if (!srcc) return 0;
  
  if (!(DRIVER->song=midi_file_new_borrow(src,srcc))) return -1;
  midi_file_set_output_rate(DRIVER->song,driver->rate);
  midi_file_set_loop_point(DRIVER->song);
  
  return 0;
}

/* Allocate voice.
 */
 
static struct stdsyn_voice *stdsyn_voice_alloc(struct bigpc_synth_driver *driver) {
  if (DRIVER->voicec<DRIVER->voicea) return DRIVER->voicev+DRIVER->voicec++;
  int na=DRIVER->voicea+8;
  if (na>STDSYN_VOICE_LIMIT) return 0;
  void *nv=realloc(DRIVER->voicev,sizeof(struct stdsyn_voice)*na);
  if (!nv) return 0;
  DRIVER->voicev=nv;
  DRIVER->voicea=na;
  return DRIVER->voicev+DRIVER->voicec++;
}

/* Release note.
 */
 
static void stdsyn_release_chid_noteid(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t noteid) {
  struct stdsyn_voice *voice=DRIVER->voicev;
  int i=DRIVER->voicec;
  for (;i-->0;voice++) {
    if (voice->chid!=chid) continue;
    if (voice->noteid!=noteid) continue;
    stdsyn_voice_release(voice);
  }
}

/* Begin note.
 */
 
static void stdsyn_note_on(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t noteid,uint8_t velocity) {
  if (noteid>=128) return;
  struct stdsyn_voice *voice=stdsyn_voice_alloc(driver);
  if (!voice) return;
  voice->chid=chid;
  voice->noteid=noteid;
  voice->level=0.100f;
  voice->phase=0;
  voice->halfperiod=(int)(driver->rate/(midi_note_frequency[noteid]*2.0f));
  voice->t=0.0f;
  voice->dt=(midi_note_frequency[noteid]*M_PI*2.0f)/driver->rate;
  
  voice->env.atktlo=  20; voice->env.atkthi=   5;
  voice->env.dectlo=  20; voice->env.decthi=  15;
  voice->env.rlstlo= 100; voice->env.rlsthi= 500;
  voice->env.atkvlo=0.090f; voice->env.atkvhi=0.170f;
  voice->env.susvlo=0.030f; voice->env.susvhi=0.050f;
  stdsyn_env_reset(&voice->env,velocity,driver->rate);
}

/* Event.
 */
 
static void _stdsyn_event(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %02x %02x %02x %02x\n",__func__,chid,opcode,a,b);
  if (chid==0x0f) return;//XXX throw away channel 15. we use it for sound effects and drums
  switch (opcode) {
    case MIDI_OPCODE_NOTE_OFF: stdsyn_release_chid_noteid(driver,chid,a); break;
    case MIDI_OPCODE_NOTE_ON: stdsyn_note_on(driver,chid,a,b); break;
  }
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
