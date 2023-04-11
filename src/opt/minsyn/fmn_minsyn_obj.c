#include "fmn_minsyn_internal.h"

/* Cleanup.
 */
 
static void _minsyn_del(struct bigpc_synth_driver *driver) {
  if (DRIVER->voicev) {
    while (DRIVER->voicec-->0) minsyn_voice_cleanup(DRIVER->voicev+DRIVER->voicec);
    free(DRIVER->voicev);
  }
  if (DRIVER->playbackv) {
    while (DRIVER->playbackc-->0) minsyn_playback_cleanup(DRIVER->playbackv+DRIVER->playbackc);
    free(DRIVER->playbackv);
  }
  if (DRIVER->pcmv) {
    while (DRIVER->pcmc-->0) minsyn_pcm_del(DRIVER->pcmv[DRIVER->pcmc]);
    free(DRIVER->pcmv);
  }
  if (DRIVER->wavev) {
    while (DRIVER->wavec-->0) free(DRIVER->wavev[DRIVER->wavec]);
    free(DRIVER->wavev);
  }
  if (DRIVER->printerv) {
    while (DRIVER->printerc-->0) minsyn_printer_del(DRIVER->printerv[DRIVER->printerc]);
    free(DRIVER->printerv);
  }
  midi_file_del(DRIVER->song);
}

/* Drop voices that have run out of funk.
 */
 
static void minsyn_drop_defunct_voices(struct bigpc_synth_driver *driver) {
  int i=DRIVER->voicec;
  struct minsyn_voice *voice=DRIVER->voicev+i-1;
  for (;i-->0;voice--) {
    if (!voice->v) {
      minsyn_voice_cleanup(voice);
      DRIVER->voicec--;
      memmove(voice,voice+1,sizeof(struct minsyn_voice)*(DRIVER->voicec-i));
    }
  }
  struct minsyn_playback *playback=DRIVER->playbackv+DRIVER->playbackc-1;
  for (i=DRIVER->playbackc;i-->0;playback--) {
    if (!playback->pcm) {
      minsyn_playback_cleanup(playback);
      DRIVER->playbackc--;
      memmove(playback,playback+1,sizeof(struct minsyn_playback)*(DRIVER->playbackc-i));
    }
  }
}

/* Update, main.
 * Our natural mode is s16 mono.
 */
 
static void _minsyn_update_mono(int16_t *v,int c,struct bigpc_synth_driver *driver) {
  memset(v,0,c<<1);

  if (DRIVER->update_in_progress_framec) return;
  DRIVER->update_in_progress_framec=c;
  int i=DRIVER->printerc;
  struct minsyn_printer **p=DRIVER->printerv+i-1;
  for (;i-->0;p--) {
    if (minsyn_printer_update(*p,c)<=0) {
      minsyn_printer_del(*p);
      DRIVER->printerc--;
      memmove(p,p+1,sizeof(void*)*(DRIVER->printerc-i));
    }
  }

  while (c>0) {
  
    int updc=c;
    if (DRIVER->song) {
      struct midi_event event={0};
      updc=midi_file_next(&event,DRIVER->song,0);
      if (updc<0) {
        fprintf(stderr,"minsyn: song end or error\n");
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
    
    minsyn_generate_signal(v,updc,driver);
    v+=updc;
    c-=updc;
    if (DRIVER->song) midi_file_advance(DRIVER->song,updc);
  }
  minsyn_drop_defunct_voices(driver);
  DRIVER->update_in_progress_framec=0;
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

/* Release all voices.
 */
 
void minsyn_release_all(struct bigpc_synth_driver *driver) {
  struct minsyn_voice *voice=DRIVER->voicev;
  int i=DRIVER->voicec;
  for (;i-->0;voice++) minsyn_voice_release(voice);
  struct minsyn_playback *playback=DRIVER->playbackv;
  for (i=DRIVER->playbackc;i-->0;playback++) minsyn_playback_release(playback);
}
 
void minsyn_silence_all(struct bigpc_synth_driver *driver) {
  struct minsyn_voice *voice=DRIVER->voicev;
  int i=DRIVER->voicec;
  for (;i-->0;voice++) minsyn_voice_cleanup(voice);
  DRIVER->voicec=0;
  struct minsyn_playback *playback=DRIVER->playbackv;
  for (i=DRIVER->playbackc;i-->0;playback++) minsyn_playback_cleanup(playback);
  DRIVER->playbackc=0;
}

/* Play song.
 */
 
static int _minsyn_play_song(struct bigpc_synth_driver *driver,const void *src,int srcc,int force) {
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
    minsyn_release_all(driver);
  }
  if (!srcc) return 0;
  if (!(DRIVER->song=midi_file_new_borrow(src,srcc))) return -1;
  midi_file_set_output_rate(DRIVER->song,driver->rate);
  midi_file_set_loop_point(DRIVER->song);
  return 0;
}

/* Allocate voice.
 */
 
static struct minsyn_voice *minsyn_voice_alloc(struct bigpc_synth_driver *driver) {
  if (DRIVER->voicec<DRIVER->voicea) return DRIVER->voicev+DRIVER->voicec++;
  int na=DRIVER->voicea+16;
  if (na>MINSYN_VOICE_LIMIT) return 0;
  void *nv=realloc(DRIVER->voicev,sizeof(struct minsyn_voice)*na);
  if (!nv) return 0;
  DRIVER->voicev=nv;
  DRIVER->voicea=na;
  return DRIVER->voicev+DRIVER->voicec++;
}

static struct minsyn_playback *minsyn_playback_alloc(struct bigpc_synth_driver *driver) {
  if (DRIVER->playbackc<DRIVER->playbacka) return DRIVER->playbackv+DRIVER->playbackc++;
  int na=DRIVER->playbacka+16;
  if (na>MINSYN_PLAYBACK_LIMIT) return 0;
  void *nv=realloc(DRIVER->playbackv,sizeof(struct minsyn_playback)*na);
  if (!nv) return 0;
  DRIVER->playbackv=nv;
  DRIVER->playbacka=na;
  return DRIVER->playbackv+DRIVER->playbackc++;
}

/* Release note.
 */
 
static void minsyn_release_chid_noteid(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t noteid) {
  struct minsyn_voice *voice=DRIVER->voicev;
  int i=DRIVER->voicec;
  for (;i-->0;voice++) {
    if (voice->chid!=chid) continue;
    if (voice->noteid!=noteid) continue;
    minsyn_voice_release(voice);
  }
  struct minsyn_playback *playback=DRIVER->playbackv;
  for (i=DRIVER->playbackc;i-->0;playback++) {
    if (playback->chid!=chid) continue;
    if (playback->noteid!=noteid) continue;
    minsyn_playback_release(playback);
  }
}

/* Begin note.
 */
 
static void minsyn_note_on(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t noteid,uint8_t velocity) {
  if (noteid>=128) return;

  if (chid<0x0f) { // channel 0..14 are tuned voices
    struct minsyn_voice *voice=minsyn_voice_alloc(driver);
    if (!voice) return;
    memset(voice,0,sizeof(struct minsyn_voice));
    voice->chid=chid;
    voice->noteid=noteid;
    minsyn_voice_init(driver,voice,velocity);
    
  } else if (chid==0x0f) { // channel 15 is raw pcm
    struct minsyn_pcm *pcm=0;//TODO acquire pcm per noteid
    if (!pcm) return;
    struct minsyn_playback *playback=minsyn_playback_alloc(driver);
    if (!playback) return;
    memset(playback,0,sizeof(struct minsyn_playback));
    playback->chid=chid;
    playback->noteid=noteid;
    minsyn_playback_init(driver,playback,pcm);
  }
}

/* Event.
 */
 
static void _minsyn_event(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %02x %02x %02x %02x\n",__func__,chid,opcode,a,b);
  switch (opcode) {
    case MIDI_OPCODE_NOTE_OFF: minsyn_release_chid_noteid(driver,chid,a); break;
    case MIDI_OPCODE_NOTE_ON: minsyn_note_on(driver,chid,a,b); break;
    case MIDI_OPCODE_RESET: minsyn_silence_all(driver); break;
  }
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
