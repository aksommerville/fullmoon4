/* midi.h
 * Helpers to read MIDI files and streams.
 * Tables of defines, from the spec.
 */

#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>

/* Event and stream reader.
 * Neither requires cleanup.
 * Both require input to remain constant while generated events are in play.
 ****************************************************************************/

struct midi_event {
  uint8_t chid; // 0..15
  uint8_t opcode; // with (chid) zeroed
  uint8_t a,b; // data bytes
  const void *v; // borrowed data for sysex and meta
  int c;
};

struct midi_stream_reader {
  uint8_t status;
  const uint8_t *v;
  int c;
  uint8_t buf[4];
  uint8_t bufc;
};

/* Call "more" with an arbitrary chunk of input off the wire.
 * Then "next" until it returns zero.
 * These are for I/O streams, not files.
 * Initialize the reader to zeroes.
 */
int midi_stream_reader_more(struct midi_stream_reader *reader,const void *src,int srcc);
int midi_stream_reader_next(struct midi_event *event,struct midi_stream_reader *reader);

/* File reader.
 * This is the live player of a MIDI file, and can also serve as the data container.
 * Decoding is pretty cheap; it's reasonable to decode all your songs to midi_file at startup.
 *****************************************************************************/
 
struct midi_file {
  int refc;
  uint8_t *v;
  int c;
  int ownv;
  
  int output_rate;
  int usperqnote;
  int usperqnotecapa;
  int loop_set;
  int extra_delay; // frames
  double frames_per_tick;

  uint16_t format;
  uint16_t track_count;
  uint16_t division;

  struct midi_track {
    const uint8_t *v;
    int c;
    int term,termcapa;
    int p,pcapa;
    int delay,delaycapa; // <0 if not read yet
    uint8_t status,statuscapa;
  } *trackv;
  int trackc,tracka;
};

void midi_file_del(struct midi_file *file);
int midi_file_ref(struct midi_file *file);

struct midi_file *midi_file_new_copy(const void *src,int srcc);
struct midi_file *midi_file_new_borrow(const void *src,int srcc);
struct midi_file *midi_file_new_handoff(void *src,int srcc);

/* Tell us your driver's output rate and we'll take care of timing.
 * Defaults to 44100.
 */
int midi_file_set_output_rate(struct midi_file *file,int hz);

/* During playback, if you get some private Sysex or Meta event for the loop point, call this.
 * We will arrange to return to the current position (just *after* the most recently reported event) at EOF.
 * Default is to fail at EOF.
 * You can call this immediately after decode to loop the whole song, and set it elsewhere later.
 * When looping enabled, we always report a 1-frame delay just before the loop.
 */
int midi_file_set_loop_point(struct midi_file *file);

/* Discard all transient state, begin from the very beginning again.
 */
void midi_file_restart(struct midi_file *file);

/* If an event is ready to fire, populates (event) and returns zero.
 * No event ready, returns frame count to next event.
 * <0 at EOF (if no loop point) or error.
 * We can optionally report the track index the event was read from, though you shouldn't need it.
 */
int midi_file_next(struct midi_event *event,struct midi_file *file,int *trackp);

/* Advance our clock by so many frames.
 * This must not be more than the last thing returned by midi_file_next().
 */
int midi_file_advance(struct midi_file *file,int framec);

/* Symbols.
 ****************************************************************************/

/* Channel Voice Opcodes.
 * Only the 4 high bits matter; low 4 are chid.
 */
#define MIDI_OPCODE_NOTE_OFF       0x80 /* (noteid,velocity) */
#define MIDI_OPCODE_NOTE_ON        0x90 /* (noteid,velocity) */
#define MIDI_OPCODE_NOTE_ADJUST    0xa0 /* (noteid,pressure) */
#define MIDI_OPCODE_CONTROL        0xb0 /* (k,v) */
#define MIDI_OPCODE_PROGRAM        0xc0 /* (pid) */
#define MIDI_OPCODE_PRESSURE       0xd0 /* (pressure) */
#define MIDI_OPCODE_WHEEL          0xe0 /* (lo7,hi7) */

/* System and Realtime Opcodes.
 * SYSEX can appear in files, with some formatting quirks. Otherwise these are only for streams.
 */
#define MIDI_OPCODE_SYSEX            0xf0 /* Starts with 7-bit mfr id, then data until EOX. */
#define MIDI_OPCODE_SONG_POSITION    0xf2 /* (lo7,hi7) */
#define MIDI_OPCODE_SONG_SELECT      0xf3 /* (songid) */
#define MIDI_OPCODE_TUNE_REQUEST     0xf6
#define MIDI_OPCODE_EOX              0xf7
#define MIDI_OPCODE_TIMING_CLOCK     0xf8
#define MIDI_OPCODE_START            0xfa
#define MIDI_OPCODE_CONTINUE         0xfb
#define MIDI_OPCODE_STOP             0xfc
#define MIDI_OPCODE_ACTIVE_SENSING   0xfe /* Receiver should reset if no events within 300ms */
#define MIDI_OPCODE_RESET            0xff

/* Non-Channel-Voice Opcodes legal only in files.
 */
#define MIDI_OPCODE_SYSEX_OPEN      0xf0
#define MIDI_OPCODE_SYSEX_CLOSED    0xf7
#define MIDI_OPCODE_META            0xff

/* Other opcodes specific to this API.
 */
#define MIDI_OPCODE_ERROR     0x00

#define MIDI_CHID_ALL 0xff

/* Meta event IDs.
 * These can only appear in files, not streams.
 */
#define MIDI_META_TEXT            0x01
#define MIDI_META_COPYRIGHT       0x02
#define MIDI_META_TRACK_NAME      0x03
#define MIDI_META_INSTRUMENT_NAME 0x04
#define MIDI_META_LYRICS          0x05
// Other events through 0x1f are text but exact meaning unknown.
#define MIDI_META_CHANNEL_PREFIX  0x20
#define MIDI_META_EOT             0x2f
#define MIDI_META_SET_TEMPO       0x51 /* u24be:us/qnote */
#define MIDI_META_SMPTE_OFFSET    0x54
#define MIDI_META_TIME_SIGNATURE  0x58
#define MIDI_META_KEY_SIGNATURE   0x59

/* Control keys.
 */
// 0x00..0x1f are MSB corresponding to 0x20..0x3f LSB.
#define MIDI_CTL_BANK_MSB          0x00
#define MIDI_CTL_MOD_MSB           0x01
#define MIDI_CTL_BREATH_MSB        0x02
#define MIDI_CTL_FOOT_MSB          0x04
#define MIDI_CTL_PORTA_TIME_MSB    0x05
#define MIDI_CTL_DATA_ENTRY_MSB    0x06
#define MIDI_CTL_VOLUME_MSB        0x07
#define MIDI_CTL_BALANCE_MSB       0x08
#define MIDI_CTL_PAN_MSB           0x0a
#define MIDI_CTL_EXPRESSION_MSB    0x0b
#define MIDI_CTL_EFFECT1_MSB       0x0c
#define MIDI_CTL_EFFECT2_MSB       0x0d
#define MIDI_CTL_EFFECT3_MSB       0x0e
#define MIDI_CTL_EFFECT4_MSB       0x0f
#define MIDI_CTL_GP1_MSB           0x10
#define MIDI_CTL_GP2_MSB           0x11
#define MIDI_CTL_GP3_MSB           0x12
#define MIDI_CTL_GP4_MSB           0x13
#define MIDI_CTL_BANK_LSB          0x20
#define MIDI_CTL_MOD_LSB           0x21
#define MIDI_CTL_BREATH_LSB        0x22
#define MIDI_CTL_FOOT_LSB          0x24
#define MIDI_CTL_PORTA_TIME_LSB    0x25
#define MIDI_CTL_DATA_ENTRY_LSB    0x26
#define MIDI_CTL_VOLUME_LSB        0x27
#define MIDI_CTL_BALANCE_LSB       0x28
#define MIDI_CTL_PAN_LSB           0x2a
#define MIDI_CTL_EXPRESSION_LSB    0x2b
#define MIDI_CTL_EFFECT1_LSB       0x2c
#define MIDI_CTL_EFFECT2_LSB       0x2d
#define MIDI_CTL_EFFECT3_LSB       0x2e
#define MIDI_CTL_EFFECT4_LSB       0x2f
#define MIDI_CTL_GP1_LSB           0x30
#define MIDI_CTL_GP2_LSB           0x31
#define MIDI_CTL_GP3_LSB           0x32
#define MIDI_CTL_GP4_LSB           0x33
// 0x40..0x45 are switches. <0x40=off >=0x40=on
#define MIDI_CTL_SUSTAIN           0x40
#define MIDI_CTL_PORTAMENTO        0x41
#define MIDI_CTL_SUSTENUTO         0x42
#define MIDI_CTL_SOFT              0x43
#define MIDI_CTL_LEGATO            0x44
#define MIDI_CTL_HOLD2             0x45
// 0x46..0x5f are general continuous controls.
#define MIDI_CTL_VARIATION         0x46
#define MIDI_CTL_TIMBRE            0x47
#define MIDI_CTL_RELEASE_TIME      0x48
#define MIDI_CTL_ATTACK_TIME       0x49
#define MIDI_CTL_BRIGHTNESS        0x4a
#define MIDI_CTL_CTL6              0x4b
#define MIDI_CTL_CTL7              0x4c
#define MIDI_CTL_CTL8              0x4d
#define MIDI_CTL_CTL9              0x4e
#define MIDI_CTL_CTL10             0x4f
#define MIDI_CTL_GP5               0x50
#define MIDI_CTL_GP6               0x51
#define MIDI_CTL_GP7               0x52
#define MIDI_CTL_GP8               0x53
#define MIDI_CTL_PORTA_CONTROL     0x54 /* noteid */
#define MIDI_CTL_EFFECT1_DEPTH     0x5b
#define MIDI_CTL_EFFECT2_DEPTH     0x5c
#define MIDI_CTL_EFFECT3_DEPTH     0x5d
#define MIDI_CTL_EFFECT4_DEPTH     0x5e
#define MIDI_CTL_EFFECT5_DEPTH     0x5f
// 0x78..0x7f are system controls. Any event here implies All Notes Off.
#define MIDI_CTL_ALL_SOUND_OFF     0x78
#define MIDI_CTL_RESET_CONTROLLERS 0x79
#define MIDI_CTL_LOCAL_SWITCH      0x7a /* ? */
#define MIDI_CTL_ALL_NOTES_OFF     0x7b
#define MIDI_CTL_OMNI_OFF          0x7c
#define MIDI_CTL_OMNI_ON           0x7d
#define MIDI_CTL_POLY_SWITCH       0x7e
#define MIDI_CTL_POLY_ON           0x7f

/* Chunk ID in a file, read big-endianly.
 */
#define MIDI_CHUNK_MThd (('M'<<24)|('T'<<16)|('h'<<8)|'d')
#define MIDI_CHUNK_MTrk (('M'<<24)|('T'<<16)|('r'<<8)|'k')

/* Optional, defined in midi_text.c.
 * Note that octaves begin on C. It's [c,d,e,f,g,a,b], in the past I usually incorrectly started on A.
 * (Why would "A" not be the first thing? Fucking musicians).
 * Octaves are -1..9; the last one ends with G.
 * We allow "n" in place of "-" for the negative octave, and "s" in place of "#" for sharp, to eliminate punctation.
 */
extern const char *midi_gm_instrument_names[128]; // key=pid
extern const char *midi_gm_family_names[16]; // key=(pid>>3)
extern const char *midi_gm_drum_names[128]; // key=noteid
int midi_noteid_repr(char *dst,int dsta,uint8_t noteid);
int midi_noteid_eval(const char *src,int srcc);

extern const float midi_note_frequency[128]; // key=noteid

#endif
