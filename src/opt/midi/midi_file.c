#include "midi.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

/* Delete.
 */
 
void midi_file_del(struct midi_file *file) {
  if (!file) return;
  if (!file->refc) return;
  if (file->refc-->1) return;
  
  if (file->v&&file->ownv) free(file->v);
  if (file->trackv) free(file->trackv);
  
  free(file);
}

/* Retain.
 */
 
int midi_file_ref(struct midi_file *file) {
  if (!file) return -1;
  if (file->refc<1) return -1;
  if (file->refc==INT_MAX) return -1;
  file->refc++;
  return 0;
}

/* Recalculate frames_per_tick.
 */
 
static void midi_file_tempo_changed(struct midi_file *file) {
  if (file->division) {
    file->frames_per_tick=((int64_t)file->usperqnote*(int64_t)file->output_rate)/(int64_t)(file->division*1000000ll);
    if (file->frames_per_tick<1) file->frames_per_tick=1;
  } else {
    file->frames_per_tick=1000;
  }
}

/* Receive MThd.
 */
 
static int midi_file_decode_MThd(struct midi_file *file,const uint8_t *v,int c) {
  if (file->division) return -1;
  if (c<6) return -1;
  
  file->format=(v[0]<<8)|v[1];
  file->track_count=(v[2]<<8)|v[3];
  file->division=(v[4]<<8)|v[5];
  
  if (file->division&0x8000) {
    // SMPTE timing. I don't know how to handle this, and don't have any example files. So fail.
    return -1;
  } else if (!file->division) {
    // Invalid either way, but especially for us, as (division) is our "MThd present" flag.
    return -1;
  }
  
  return 0;
}

/* Receive MTrk.
 */
 
static int midi_file_decode_MTrk(struct midi_file *file,const uint8_t *v,int c) {
  if (file->trackc>=file->tracka) {
    int na=file->tracka+8;
    if (na>INT_MAX/sizeof(struct midi_track)) return -1;
    void *nv=realloc(file->trackv,sizeof(struct midi_track)*na);
    if (!nv) return -1;
    file->trackv=nv;
    file->tracka=na;
  }
  struct midi_track *track=file->trackv+file->trackc++;
  memset(track,0,sizeof(struct midi_track));
  track->v=v;
  track->c=c;
  track->delay=-1;
  track->delaycapa=-1;
  return 0;
}

/* Validate after decoding.
 */
 
static int midi_file_decode_finish(struct midi_file *file) {
  if (!file->division) return -1; // No MThd
  if (file->trackc<1) return -1; // No MTrk
  return 0;
}

/* Decode new file.
 */
 
static int midi_file_decode(struct midi_file *file) {
  file->division=0;
  file->trackc=0;
  int srcp=0;
  while (1) {
    if (srcp>file->c-8) break;
    uint32_t chunkid=(file->v[srcp]<<24)|(file->v[srcp+1]<<16)|(file->v[srcp+2]<<8)|file->v[srcp+3];
    int len=(file->v[srcp+4]<<24)|(file->v[srcp+5]<<16)|(file->v[srcp+6]<<8)|file->v[srcp+7];
    srcp+=8;
    if ((len<0)||(srcp>file->c-len)) return -1;
    const void *chunk=file->v+srcp;
    srcp+=len;
    switch (chunkid) {
      case MIDI_CHUNK_MThd: if (midi_file_decode_MThd(file,chunk,len)<0) return -1; break;
      case MIDI_CHUNK_MTrk: if (midi_file_decode_MTrk(file,chunk,len)<0) return -1; break;
      default: /* Unknown chunk, let's say that's OK. */ break;
    }
  }
  return midi_file_decode_finish(file);
}

/* New, internal.
 */
 
static struct midi_file *midi_file_alloc() {
  struct midi_file *file=calloc(1,sizeof(struct midi_file));
  if (!file) return 0;
  file->refc=1;
  file->output_rate=44100;
  file->usperqnote=500000;
  file->usperqnotecapa=500000;
  midi_file_tempo_changed(file);
  return file;
}

/* New, public entry points.
 */

struct midi_file *midi_file_new_copy(const void *src,int srcc) {
  if ((srcc<1)||!src) return 0;
  struct midi_file *file=midi_file_alloc();
  if (!file) return 0;
  if (!(file->v=malloc(srcc))) {
    midi_file_del(file);
    return 0;
  }
  memcpy(file->v,src,srcc);
  file->c=srcc;
  file->ownv=1;
  if (midi_file_decode(file)<0) {
    midi_file_del(file);
    return 0;
  }
  return file;
}

struct midi_file *midi_file_new_borrow(const void *src,int srcc) {
  if ((srcc<1)||!src) return 0;
  struct midi_file *file=midi_file_alloc();
  if (!file) return 0;
  file->v=(void*)src;
  file->c=srcc;
  file->ownv=0;
  if (midi_file_decode(file)<0) {
    midi_file_del(file);
    return 0;
  }
  return file;
}

struct midi_file *midi_file_new_handoff(void *src,int srcc) {
  if ((srcc<1)||!src) return 0;
  struct midi_file *file=midi_file_alloc();
  if (!file) return 0;
  file->v=src;
  file->c=srcc;
  file->ownv=1;
  if (midi_file_decode(file)<0) {
    midi_file_del(file);
    return 0;
  }
  return file;
}

/* Set output rate.
 */

int midi_file_set_output_rate(struct midi_file *file,int hz) {
  if (!file) return -1;
  if (hz<1) return -1;
  file->output_rate=hz;
  midi_file_tempo_changed(file);
  return 0;
}

/* Set loop point.
 */

int midi_file_set_loop_point(struct midi_file *file) {
  if (!file) return -1;
  struct midi_track *track=file->trackv;
  int i=file->trackc;
  for (;i-->0;track++) {
    track->termcapa=track->term;
    track->pcapa=track->p;
    track->delaycapa=track->delay;
    track->statuscapa=track->status;
  }
  file->usperqnotecapa=file->usperqnote;
  file->loop_set=1;
  return 0;
}

/* Start again from the top.
 */

void midi_file_restart(struct midi_file *file) {
  if (!file) return;
  struct midi_track *track=file->trackv;
  int i=file->trackc;
  for (;i-->0;track++) {
    track->term=0;
    track->p=0;
    track->delay=-1;
    track->status=0;
  }
  if (file->usperqnote!=500000) {
    file->usperqnote=500000;
    midi_file_tempo_changed(file);
  }
}

/* Return to the loop point.
 */
 
static void midi_file_loop(struct midi_file *file) {
  struct midi_track *track=file->trackv;
  int i=file->trackc;
  for (;i-->0;track++) {
    track->term=track->termcapa;
    track->p=track->pcapa;
    track->delay=track->delaycapa;
    track->status=track->statuscapa;
  }
  if (file->usperqnote!=file->usperqnotecapa) {
    file->usperqnote=file->usperqnotecapa;
    midi_file_tempo_changed(file);
  }
}

/* Check for any local action required from one event (eg tempo).
 */
 
static int midi_file_local_event(
  struct midi_file *file,
  struct midi_track *track,
  struct midi_event *event
) {
  switch (event->opcode) {
    case MIDI_OPCODE_META: switch (event->a) {
        case MIDI_META_SET_TEMPO: if (event->c==3) {
            const uint8_t *V=event->v;
            int usperqnote=(V[0]<<16)|(V[1]<<8)|V[2];
            if (usperqnote&&(usperqnote!=file->usperqnote)) {
              file->usperqnote=usperqnote;
              midi_file_tempo_changed(file);
            }
          } break;
      } break;
  }
  return 0;
}

/* VLQ.
 */
 
static int midi_vlq_decode(int *dst,const uint8_t *v,int c) {
  if (c<1) return -1;
  if (!(v[0]&0x80)) {
    *dst=v[0];
    return 1;
  }
  if ((c>=2)&&!(v[1]&0x80)) {
    *dst=((v[0]&0x7f)<<7)|v[1];
    return 2;
  }
  if ((c>=3)&&!(v[2]&0x80)) {
    *dst=((v[0]&0x7f)<<14)|((v[1]&0x7f)<<7)|v[2];
    return 3;
  }
  if ((c>=4)&&!(v[3]&0x80)) {
    *dst=((v[0]&0x7f)<<21)|((v[1]&0x7f)<<14)|((v[2]&0x7f)<<7)|v[3];
    return 4;
  }
  return -1;
}

/* Read delay for one track.
 */
 
static int midi_track_read_delay(struct midi_track *track) {
  if (track->p>=track->c) {
    track->term=1;
    return 0;
  }
  const uint8_t *v=track->v+track->p;
  int c=track->c-track->p;
  int err=midi_vlq_decode(&track->delay,v,c);
  if (err<1) return -1;
  track->p+=err;
  return 0;
}

/* Read one event from track.
 */
 
static int midi_track_read_event(struct midi_event *event,struct midi_track *track,struct midi_file *file) {
  track->delay=-1;
  const uint8_t *v=track->v+track->p;
  int c=track->c-track->p;
  int p=0;
  if (c<1) return -1;
  
  uint8_t lead=v[0];
  if (lead&0x80) { track->status=lead; p=1; }
  else if (track->status&0x80) lead=track->status;
  else return -1;
  
  event->opcode=lead&0xf0;
  event->chid=lead&0x0f;
  event->a=0;
  event->b=0;
  event->v=0;
  event->c=0;
  
  #define PARAM_A if (p>=c) return -1; event->a=v[p++];
  #define PARAM_AB if (p>c-2) return -1; event->a=v[p++]; event->b=v[p++];
  switch (event->opcode) {
    case MIDI_OPCODE_NOTE_OFF: PARAM_AB break;
    case MIDI_OPCODE_NOTE_ON: PARAM_AB if (!event->b) { event->opcode=MIDI_OPCODE_NOTE_OFF; event->b=0x40; } break;
    case MIDI_OPCODE_NOTE_ADJUST: PARAM_AB break;
    case MIDI_OPCODE_CONTROL: PARAM_AB break;
    case MIDI_OPCODE_PROGRAM: PARAM_A break;
    case MIDI_OPCODE_PRESSURE: PARAM_A break;
    case MIDI_OPCODE_WHEEL: PARAM_AB break;
    default: {
        track->status=0;
        event->opcode=lead;
        event->chid=MIDI_CHID_ALL;
        switch (event->opcode) {
          case MIDI_OPCODE_META: PARAM_A // ...and then it's just like sysex...
          case MIDI_OPCODE_SYSEX_OPEN:
          case MIDI_OPCODE_SYSEX_CLOSED: {
              int err,len;
              if ((err=midi_vlq_decode(&len,v+p,c-p))<1) return -1;
              p+=err;
              if (p>c-len) return -1;
              event->v=v+p;
              event->c=len;
              p+=len;
            } break;
          default: return -1;
        }
      }
  }
  #undef PARAM_A
  #undef PARAM_AB
  
  track->p+=p;
  if (midi_file_local_event(file,track,event)<0) return -1;
  return 0;
}

/* Convert ticks to frames and vice-versa.
 */
 
static int midi_file_frames_from_ticks(struct midi_file *file,int tickc) {
  if (tickc<1) return 0;
  int framec=(int)(file->frames_per_tick*tickc);
  if (framec<1) return 1;
  return framec;
}

static int midi_file_ticks_from_frames(struct midi_file *file,int framec,int *remainder) {
  if (remainder) *remainder=0;
  if (framec<1) return 0;
  int tickc=(int)(framec/file->frames_per_tick);
  if (tickc<0) tickc=0;
  else if (remainder&&(framec>0)) {
    *remainder=framec%file->frames_per_tick;
  }
  return tickc;
}

/* One event if ready, or delay in frames to the next one.
 */

int midi_file_next(struct midi_event *event,struct midi_file *file,int *trackp) {
  if (!event||!file) return -1;
  
  int mindelay=INT_MAX;
  struct midi_track *track=file->trackv;
  int i=0;
  for (;i<file->trackc;i++,track++) {
    if (track->term) continue;
    if (track->delay<0) {
      if (midi_track_read_delay(track)<0) return -1;
      if (track->term) continue;
    }
    if (!track->delay) {
      if (midi_track_read_event(event,track,file)<0) return -1;
      if (trackp) *trackp=i;
      return 0;
    }
    if (track->delay<mindelay) mindelay=track->delay;
  }
  
  if (mindelay==INT_MAX) { // EOF
    if (file->loop_set) {
      midi_file_loop(file);
      file->extra_delay=1;
      return 1;
    }
    return -1;
  }
  
  int framec=midi_file_frames_from_ticks(file,mindelay);
  return framec;
}

/* Advance all track clocks.
 */
 
static void midi_file_advance_tracks(struct midi_file *file,int tickc) {
  struct midi_track *track=file->trackv;
  int i=file->trackc;
  for (;i-->0;track++) {
    if (track->term) continue;
    if (track->delay<0) {
      if (midi_track_read_delay(track)<0) continue;
    }
    if (tickc>=track->delay) track->delay=0;
    else track->delay-=tickc;
  }
}

/* Advance clock.
 */

int midi_file_advance(struct midi_file *file,int framec) {
  if (!file) return -1;
  if (framec<1) return 0;
  
  framec+=file->extra_delay;
  
  int tickc=midi_file_ticks_from_frames(file,framec,&file->extra_delay);
  if (tickc>0) {
    midi_file_advance_tracks(file,tickc);
  }
  return 0;
}
