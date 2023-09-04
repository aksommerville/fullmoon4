#include "fmn_stdsyn_internal.h"

/* Cleanup.
 */
 
static void _stdsyn_del(struct bigpc_synth_driver *driver) {
  if (DRIVER->qbuf) free(DRIVER->qbuf);
  stdsyn_node_del(DRIVER->main);
  midi_file_del(DRIVER->song);
  if (DRIVER->printerv) {
    while (DRIVER->printerc-->0) stdsyn_printer_del(DRIVER->printerv[DRIVER->printerc]);
    free(DRIVER->printerv);
  }
  stdsyn_res_store_cleanup(&DRIVER->sounds);
  stdsyn_res_store_cleanup(&DRIVER->instruments);
}

/* Update, main.
 * We may operate in mono or stereo -- the decision is mostly stdsyn_node_type_main's problem.
 */
 
static void _stdsyn_update_f32n(float *v,int c,struct bigpc_synth_driver *driver) {

  /* Advance PCM printers.
   */
  if (DRIVER->update_pending_framec) return;
  DRIVER->update_pending_framec=c;
  if (driver->chanc==2) DRIVER->update_pending_framec>>=1;
  int i=DRIVER->printerc;
  struct stdsyn_printer **p=DRIVER->printerv+i-1;
  for (;i-->0;p--) {
    if (stdsyn_printer_update(*p,c)<=0) {
      stdsyn_printer_del(*p);
      DRIVER->printerc--;
      memmove(p,p+1,sizeof(void*)*(DRIVER->printerc-i));
    }
  }
  
  while (c>0) {
  
    int updc=c;
    if (updc>STDSYN_BUFFER_SIZE) updc=STDSYN_BUFFER_SIZE;
    int updframec=updc;
    if (driver->chanc==2) updframec>>=1;
    
    if (DRIVER->song&&!DRIVER->songpause&&driver->music_enable) {
      struct midi_event event={0};
      int songframec=midi_file_next(&event,DRIVER->song,0);
      if (songframec<0) {
        midi_file_del(DRIVER->song);
        DRIVER->song=0;
        driver->song_finished=1;
        continue;
      }
      if (songframec) { // no song event; proceed with signal
        if (updframec>songframec) {
          updframec=songframec;
          updc=updframec;
          if (driver->chanc==2) c<<=1;
        }
      } else {
        if (event.opcode<0xf0) { // midi_file emits Meta and Sysex as events, opcode>=0xf0. Ignore them. (0xff would be interpretted as System Reset)
          driver->type->event(driver,event.chid,event.opcode,event.a,event.b);
        }
        continue;
      }
    }
    
    DRIVER->main->update(v,updc,DRIVER->main);
    
    v+=updc;
    c-=updc;
    if (DRIVER->song&&!DRIVER->songpause&&driver->music_enable) {
      midi_file_advance(DRIVER->song,updframec);
    }
  }
  DRIVER->update_pending_framec=0;
  if (DRIVER->main->lfupdate) DRIVER->main->lfupdate(DRIVER->main);
}

/* Update then quantize to integers.
 */
 
static void _stdsyn_update_s16n(int16_t *v,int c,struct bigpc_synth_driver *driver) {
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
  _stdsyn_update_f32n(DRIVER->qbuf,c,driver);
  const float *src=DRIVER->qbuf;
  for (;c-->0;v++,src++) {
    *v=(int16_t)((*src)*DRIVER->qlevel);
  }
}

/* Init.
 */
 
static int _stdsyn_init(struct bigpc_synth_driver *driver) {

  if (!(DRIVER->main=stdsyn_node_new(driver,&stdsyn_node_type_mixer,driver->chanc,1,0xff,0xff,0,0))) return -1;
  if (
    !DRIVER->main->update||
    !DRIVER->main->event
  ) return -1;
  
  switch (driver->format) {
    case BIGPC_AUDIO_FORMAT_s16n: driver->update=(void*)_stdsyn_update_s16n; break;
    case BIGPC_AUDIO_FORMAT_f32n: driver->update=(void*)_stdsyn_update_f32n; break;
    default: return -1;
  }
  
  DRIVER->qlevel=32000.0f;
  DRIVER->tempo=driver->rate>>1; // 120 bpm by default
  
  DRIVER->instruments.driver=driver;
  DRIVER->instruments.del=(void*)stdsyn_instrument_del;
  DRIVER->instruments.decode=(void*)stdsyn_instrument_decode;
  DRIVER->sounds.driver=driver;
  DRIVER->sounds.del=(void*)stdsyn_pcm_del;
  DRIVER->sounds.decode=(void*)stdsyn_sound_decode;
  
  return 0;
}

/* Set data.
 */
 
static int _stdsyn_set_instrument(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  return stdsyn_res_store_add(&DRIVER->instruments,id,src,srcc);
}

static int _stdsyn_set_sound(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  return stdsyn_res_store_add(&DRIVER->sounds,id,src,srcc);
}

/* Play song.
 */
 
static int _stdsyn_play_song(struct bigpc_synth_driver *driver,const void *src,int srcc,int force,int loop) {
  if (srcc<0) return -1;
  driver->song_finished=1;
  if (!srcc) {
    if (!DRIVER->song) return 0;
  }
  if (DRIVER->song) {
    if (!force) {
      if (DRIVER->song->v==src) return 0;
    }
    midi_file_del(DRIVER->song);
    DRIVER->song=0;
    stdsyn_release_all(driver,0x40);
  }
  if (!srcc) return 0;
  
  if (!(DRIVER->song=midi_file_new_borrow(src,srcc))) return -1;
  driver->song_finished=0;
  midi_file_set_output_rate(DRIVER->song,driver->rate);
  if (loop) midi_file_set_loop_point(DRIVER->song);
  
  DRIVER->tempo=DRIVER->song->frames_per_tick*DRIVER->song->division;
  if (DRIVER->main->tempo) {
    DRIVER->main->tempo(DRIVER->main,DRIVER->tempo);
  }
  
  return 0;
}

/* Pause song.
 */
 
static void _stdsyn_pause_song(struct bigpc_synth_driver *driver,int pause) {
  if (pause) {
    if (DRIVER->songpause) return;
    DRIVER->songpause=1;
    stdsyn_release_all(driver,0xff);
  } else {
    if (!DRIVER->songpause) return;
    DRIVER->songpause=0;
  }
}

static void _stdsyn_enable_music(struct bigpc_synth_driver *driver,int enable) {
  if (enable) {
    if (driver->music_enable) return;
    driver->music_enable=1;
  } else {
    if (!driver->music_enable) return;
    driver->music_enable=0;
    stdsyn_release_all(driver,0x40);
  }
}

/* Events.
 */

void stdsyn_release_all(struct bigpc_synth_driver *driver,uint8_t velocity) {
  if (DRIVER->main->release) DRIVER->main->release(DRIVER->main,velocity);
  else DRIVER->main->event(DRIVER->main,0xff,0xff,0,0);
}

void stdsyn_silence_all(struct bigpc_synth_driver *driver) {
  DRIVER->main->event(DRIVER->main,0xff,0xff,0,0);
}
 
static void _stdsyn_event(struct bigpc_synth_driver *driver,uint8_t chid,uint8_t opcode,uint8_t a,uint8_t b) {
  //fprintf(stderr,"%s %02x %02x %02x %02x\n",__func__,chid,opcode,a,b);
  DRIVER->main->event(DRIVER->main,chid,opcode,a,b);
}

/* Begin PCM print.
 */
 
struct stdsyn_printer *stdsyn_begin_print(struct bigpc_synth_driver *driver,int id,const void *src,int srcc) {
  if (DRIVER->printerc>=DRIVER->printera) {
    int na=DRIVER->printera+8;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(DRIVER->printerv,sizeof(void*)*na);
    if (!nv) return 0;
    DRIVER->printerv=nv;
    DRIVER->printera=na;
  }
  struct stdsyn_printer *printer=stdsyn_printer_new(driver->rate,src,srcc);
  if (!printer) return 0;
  DRIVER->printerv[DRIVER->printerc++]=printer;
  if (DRIVER->update_pending_framec) stdsyn_printer_update(printer,DRIVER->update_pending_framec);
  printer->soundid=id;
  return printer;
}

/* Get song position.
 */
 
static int _stdsyn_get_song_position(const struct bigpc_synth_driver *driver) {
  if (!DRIVER->song) return 0;
  return DRIVER->song->position;
}

/* Type.
 */
 
const struct bigpc_synth_type bigpc_synth_type_stdsyn={
  .name="stdsyn",
  .desc="The preferred synthesizer.",
  .data_qualifier=3,
  .objlen=sizeof(struct bigpc_synth_driver_stdsyn),
  .del=_stdsyn_del,
  .init=_stdsyn_init,
  .set_instrument=_stdsyn_set_instrument,
  .set_sound=_stdsyn_set_sound,
  .play_song=_stdsyn_play_song,
  .pause_song=_stdsyn_pause_song,
  .enable_music=_stdsyn_enable_music,
  .event=_stdsyn_event,
  .get_song_position=_stdsyn_get_song_position,
};
