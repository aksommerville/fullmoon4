#include "datan_internal.h"
#include "opt/midi/midi.h"

/* MIDI event.
 */
 
static int datan_song_event(struct midi_file *midi,uint16_t qualifier,uint16_t id,const struct midi_event *event,int trackp,int durationms) {
  #define CHECKCHID { \
    if (event->chid==14) { \
      fprintf(stderr,"%s:song:%d(%d): Channel 14 is reserved for violin. (found on track %d)\n",datan.arpath,id,qualifier,trackp); \
      return -2; \
    } \
    if ((event->chid!=trackp)&&(event->chid!=0x0f)) { \
      fprintf(stderr, \
        "%s:song:%d(%d):WARNING: Found event on track %d for channel %d. This is legal and will work, but I've been keeping track and channel IDs in sync for cleanliness.\n", \
        datan.arpath,id,qualifier,trackp,event->chid \
      ); \
    } \
  }
  #define DONTUSE(tag) { \
    fprintf(stderr,"%s:song:%d(%d):WARNING: Found %s event on track %d. We're not using this.\n",datan.arpath,id,qualifier,#tag,trackp); \
  }
  switch (event->opcode) {
    
    case MIDI_OPCODE_NOTE_ON: CHECKCHID break;
    case MIDI_OPCODE_NOTE_OFF: CHECKCHID break;
    case MIDI_OPCODE_NOTE_ADJUST: DONTUSE(NOTE_ADJUST) break;
    case MIDI_OPCODE_CONTROL: DONTUSE(CONTROL) break;
    case MIDI_OPCODE_PROGRAM: {
        CHECKCHID
        if (durationms) {
          fprintf(stderr,"%s:song:%d(%d):WARNING: Program Change after time zero (track %d).\n",datan.arpath,id,qualifier,trackp);
        }
      } break;
    case MIDI_OPCODE_PRESSURE: DONTUSE(PRESSURE) break;
    case MIDI_OPCODE_WHEEL: CHECKCHID break;
    
    case MIDI_OPCODE_META: switch (event->a) {
        case 0x2f: break; // End of Track
        case 0x51: break; // Set Tempo
        default: {
            fprintf(stderr,"%s:song:%d(%d): Unexpected meta event 0x%02x, %d bytes, on track %d.\n",datan.arpath,id,qualifier,event->a,event->c,trackp);
            return -2;
          }
      } break;
      
    default: {
         fprintf(stderr,"%s:song:%d(%d): Unexpected opcode 0x%02x on track %d.\n",datan.arpath,id,qualifier,event->a,trackp);
         return -2;
       } break;
  }

  return 0;
}

/* Validate live MIDI file.
 */
 
static int datan_song_validate_midi_file(struct midi_file *midi,uint16_t qualifier,uint16_t id) {

  // Report time in milliseconds. This is unusably slow for PCM of course, but we're not actually generating audio here.
  if (midi_file_set_output_rate(midi,1000)<0) return -1;
  int durationms=0,eventc=0;
  
  while (1) {
    struct midi_event event={0};
    int trackp=-1;
    int err=midi_file_next(&event,midi,&trackp);
    if (err<0) break; // EOF or error. TODO Can we distinguish the two?
    if (err) { // delay
      if (midi_file_advance(midi,err)<0) return -1;
      durationms+=err;
    } else { // event
      if ((err=datan_song_event(midi,qualifier,id,&event,trackp,durationms))<0) {
        return err;
      }
      eventc++;
    }
  }
  
  //fprintf(stderr,"%s:song:%d(%d): Duration %d.%03d s, %d events.\n",datan.arpath,id,qualifier,durationms/1000,durationms%1000,eventc);
  
  return 0;
}

/* Validate MIDI file, main entry point.
 */
 
int datan_song_validate_serial(uint16_t qualifier,uint32_t id,const void *v,int c) {
  struct midi_file *midi=midi_file_new_borrow(v,c);
  if (!midi) {
    fprintf(stderr,"%s:song:%d(%d): Failed to decode %d bytes as MIDI\n",datan.arpath,id,qualifier,c);
    return -2;
  }
  int err=datan_song_validate_midi_file(midi,qualifier,id);
  if (err<0) {
    midi_file_del(midi);
    return err;
  }
  if (datan_res_add(FMN_RESTYPE_SONG,qualifier,id,midi,midi_file_del)<0) {
    midi_file_del(midi);
    return -1;
  }
  return 0;
}
