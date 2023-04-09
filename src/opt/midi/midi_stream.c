#include "midi.h"
#include <string.h>

/* Supply input to a stream reader.
 */
 
int midi_stream_reader_more(struct midi_stream_reader *reader,const void *src,int srcc) {
  if (!reader) return -1;
  if (reader->c) return -1;
  if (!src||(srcc<1)) return 0;
  reader->v=src;
  reader->c=srcc;
  return 0;
}

/* Something wrong in the input stream.
 * Reset the decoder's state and report a RESET event.
 * Advances read head by one byte, maybe we'll have better luck there.
 */
 
static int midi_stream_reader_panic(
  struct midi_event *event,
  struct midi_stream_reader *reader
) {
  event->opcode=MIDI_OPCODE_RESET;
  event->chid=MIDI_CHID_ALL;
  reader->status=0;
  if (reader->bufc) {
    reader->bufc--;
    memmove(reader->buf,reader->buf+1,reader->bufc);
  } else if (reader->c) {
    reader->v++;
    reader->c--;
  }
  return 1;
}

/* Decode event from the provided serial source.
 * Ignore (v,buf).
 * Return <0 if misencoded or state inconsistent, 0 if incomplete, or length consumed.
 */
 
static int midi_stream_reader_decode(
  struct midi_event *event,
  struct midi_stream_reader *reader,
  const uint8_t *v,int c
) {
  if (c<1) return -1;
  
  if (reader->status==MIDI_OPCODE_SYSEX) {
    int sec=0;
    while ((sec<c)&&(v[sec]!=MIDI_OPCODE_EOX)) sec++;
    if (sec) {
      event->chid=MIDI_CHID_ALL;
      event->opcode=MIDI_OPCODE_SYSEX;
      event->v=v;
      event->c=sec;
      return sec;
    }
  }
  
  int p=0;
  uint8_t lead=v[0];
  if (lead&0x80) { p++; reader->status=lead; }
  else if (!reader->status) return -1;
  else lead=reader->status;
  
  // Optimistically assume Channel Voice Event.
  event->chid=lead&0x0f;
  event->opcode=lead&0xf0;
  event->a=0;
  event->b=0;
  event->v=0;
  event->c=0;
  
  #define PARAM_A if (p>=c) return 0; event->a=v[p++];
  #define PARAM_AB if (p>c-2) return 0; event->a=v[p++]; event->b=v[p++];
  switch (event->opcode) {
    case MIDI_OPCODE_NOTE_OFF: PARAM_AB break;
    case MIDI_OPCODE_NOTE_ON: PARAM_AB if (!event->b) { event->opcode=MIDI_OPCODE_NOTE_OFF; event->b=0x40; } break;
    case MIDI_OPCODE_NOTE_ADJUST: PARAM_AB break;
    case MIDI_OPCODE_CONTROL: PARAM_AB break;
    case MIDI_OPCODE_PROGRAM: PARAM_A break;
    case MIDI_OPCODE_PRESSURE: PARAM_A break;
    case MIDI_OPCODE_WHEEL: PARAM_AB break;
    default: {
        event->chid=MIDI_CHID_ALL;
        event->opcode=lead;
        reader->status=0;
        switch (event->opcode) {
          case MIDI_OPCODE_SYSEX: reader->status=MIDI_OPCODE_SYSEX; break;
          case MIDI_OPCODE_SONG_POSITION: PARAM_AB break;
          case MIDI_OPCODE_SONG_SELECT: PARAM_A break;
          case MIDI_OPCODE_TUNE_REQUEST: break;
          case MIDI_OPCODE_EOX: break;
          case MIDI_OPCODE_TIMING_CLOCK: break;
          case MIDI_OPCODE_START: break;
          case MIDI_OPCODE_CONTINUE: break;
          case MIDI_OPCODE_STOP: break;
          case MIDI_OPCODE_ACTIVE_SENSING: break;
          case MIDI_OPCODE_RESET: break;
          default: return -1;
        }
      }
  }
  #undef PARAM_A
  #undef PARAM_AB
  
  return p;
}

/* Pop events off a stream reader.
 */
 
int midi_stream_reader_next(struct midi_event *event,struct midi_stream_reader *reader) {
  
  /* Events straddling blocks of input are a real mess.
   * If anything is in (buf), fill (buf) as much as possible, and work straight off that.
   */
  if (reader->bufc) {
    int addc=sizeof(reader->buf)-reader->bufc;
    if (addc<1) return midi_stream_reader_panic(event,reader);
    if (addc>reader->c) addc=reader->c;
    memcpy(reader->buf+reader->bufc,reader->v,addc);
    reader->bufc+=addc;
    reader->v+=addc;
    reader->c-=addc;
    int err=midi_stream_reader_decode(event,reader,reader->buf,reader->bufc);
    if (err<0) return midi_stream_reader_panic(event,reader);
    if (!err) {
      if (reader->bufc==sizeof(reader->buf)) return midi_stream_reader_panic(event,reader);
      return 0;
    }
    if (err>=reader->bufc) {
      reader->bufc=0;
    } else {
      reader->bufc-=err;
      memmove(reader->buf,reader->buf+err,reader->bufc);
    }
    return 1;
  }
  
  // Nothing buffered, it's a bit more straightforward.
  if (reader->c<1) return 0;
  int err=midi_stream_reader_decode(event,reader,reader->v,reader->c);
  if (err<0) return midi_stream_reader_panic(event,reader);
  if (!err) { // incomplete: Must buffer the rest.
    if (reader->c>sizeof(reader->buf)) return midi_stream_reader_panic(event,reader);
    memcpy(reader->buf,reader->v,reader->c);
    reader->bufc=reader->c;
    reader->c=0;
    return 0;
  }
  reader->v+=err;
  reader->c-=err;
  return 1;
}
