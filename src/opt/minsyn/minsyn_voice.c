#include "fmn_minsyn_internal.h"

/* Pitch.
 */
 
static uint32_t minsyn_dp_for_noteid(struct bigpc_synth_driver *driver,uint8_t noteid) {
  if (noteid>=0x80) return 0;
  //TODO is it worth precalculating all these?
  return (uint32_t)((midi_note_frequency[noteid]*UINT32_MAX)/driver->rate);
}

/* Cleanup.
 */

void minsyn_voice_cleanup(struct minsyn_voice *voice) {
}

void minsyn_playback_cleanup(struct minsyn_playback *playback) {
  minsyn_pcm_del(playback->pcm);
  playback->pcm=0;
}

/* Init voice.
 */

void minsyn_voice_init(struct bigpc_synth_driver *driver,struct minsyn_voice *voice,uint8_t velocity) {

  int waveid=1;//TODO waveid from channel config or something?
  struct minsyn_wave *wave=minsyn_get_wave(driver,waveid);
  if (!wave) return;
  voice->v=wave->v;
  
  //TODO initialize voice->env
  minsyn_env_reset(&voice->env,velocity,driver->rate);

  voice->p=0;
  if (!(voice->dp=minsyn_dp_for_noteid(driver,voice->noteid))) {
    voice->v=0;
  }
}

/* Init playback.
 */
 
void minsyn_playback_init(struct bigpc_synth_driver *driver,struct minsyn_playback *playback,struct minsyn_pcm *pcm) {
  if (minsyn_pcm_ref(pcm)<0) return;
  playback->pcm=pcm;
  playback->p=0;
  playback->loop=(pcm->loopa<pcm->loopz);
}

/* Release.
 */
 
void minsyn_voice_release(struct minsyn_voice *voice) {
  voice->chid=voice->noteid=0xff;
  minsyn_env_release(&voice->env);
}

void minsyn_playback_release(struct minsyn_playback *playback) {
  playback->chid=playback->noteid=0xff;
  playback->loop=0;
}

/* Update voice.
 */

void minsyn_voice_update(int16_t *v,int c,struct minsyn_voice *voice) {
  if (!voice->v) return;
  for (;c-->0;v++) {
    (*v)+=(voice->v[voice->p>>MINSYN_WAVE_SHIFT]*minsyn_env_next(&voice->env))>>8;
    voice->p+=voice->dp;
  }
  if (voice->env.term) {
    voice->v=0;
  }
}

/* Update playback.
 */
 
void minsyn_playback_update(int16_t *v,int c,struct minsyn_playback *playback) {
  if (!playback->pcm) return;
  if (playback->loop&&(playback->pcm->loopa<playback->pcm->loopz)) {
    while (c>0) {
      int updc=playback->pcm->loopz-playback->p;
      if (updc>c) updc=c;
      minsyn_signal_add_v(v,playback->pcm->v+playback->p,updc);
      playback->p+=updc;
      if (playback->p>=playback->pcm->loopz) playback->p=playback->pcm->loopa;
    }
  } else {
    int updc=playback->pcm->c-playback->p;
    if (updc>c) updc=c;
    minsyn_signal_add_v(v,playback->pcm->v+playback->p,updc);
    playback->p+=updc;
    if (playback->p>=playback->pcm->c) {
      minsyn_pcm_del(playback->pcm);
      playback->pcm=0;
    }
  }
}

/* Update full signal graph.
 */

void minsyn_generate_signal(int16_t *v,int c,struct bigpc_synth_driver *driver) {
  int i;
  struct minsyn_voice *voice=DRIVER->voicev;
  for (i=DRIVER->voicec;i-->0;voice++) minsyn_voice_update(v,c,voice);
  struct minsyn_playback *playback=DRIVER->playbackv;
  for (i=DRIVER->playbackc;i-->0;playback++) minsyn_playback_update(v,c,playback);
}
