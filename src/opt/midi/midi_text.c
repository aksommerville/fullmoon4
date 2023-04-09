#include "midi.h"
#include <string.h>

/* GM instruments.
 */
 
const char *midi_gm_instrument_names[128]={
  [0]="Acoustic Grand Piano",
  [1]="Bright Acoustic Piano",
  [2]="Electric Grand Piano",
  [3]="Honky",
  [4]="Electric Piano 1 (Rhodes Piano)",
  [5]="Electric Piano 2 (Chorused Piano)",
  [6]="Harpsichord",
  [7]="Clavinet",
  [8]="Celesta",
  [19]="Glockenspiel",
  [10]="Music Box",
  [11]="Vibraphone",
  [12]="Marimba",
  [13]="Xylophone",
  [14]="Tubular Bells",
  [15]="Dulcimer (Santur)",
  [16]="Drawbar Organ (Hammond)",
  [17]="Percussive Organ",
  [18]="Rock Organ",
  [19]="Church Organ",
  [20]="Reed Organ",
  [21]="Accordion (French)",
  [22]="Harmonica",
  [23]="Tango Accordion (Band neon)",
  [24]="Acoustic Guitar (nylon)",
  [25]="Acoustic Guitar (steel)",
  [26]="Electric Guitar (jazz)",
  [27]="Electric Guitar (clean)",
  [28]="Electric Guitar (muted)",
  [29]="Overdriven Guitar",
  [30]="Distortion Guitar",
  [31]="Guitar harmonics",
  [32]="Acoustic Bass",
  [33]="Electric Bass (fingered)",
  [34]="Electric Bass (picked)",
  [35]="Fretless Bass",
  [36]="Slap Bass 1",
  [37]="Slap Bass 2",
  [38]="Synth Bass 1",
  [39]="Synth Bass 2",
  [40]="Violin",
  [41]="Viola",
  [42]="Cello",
  [43]="Contrabass",
  [44]="Tremolo Strings",
  [45]="Pizzicato Strings",
  [46]="Orchestral Harp",
  [47]="Timpani",
  [48]="String Ensemble 1 (strings)",
  [49]="String Ensemble 2 (slow strings)",
  [50]="SynthStrings 1",
  [51]="SynthStrings 2",
  [52]="Choir Aahs",
  [53]="Voice Oohs",
  [54]="Synth Voice",
  [55]="Orchestra Hit",
  [56]="Trumpet",
  [57]="Trombone",
  [58]="Tuba",
  [59]="Muted Trumpet",
  [60]="French Horn",
  [61]="Brass Section",
  [62]="SynthBrass 1",
  [63]="SynthBrass 2",
  [64]="Soprano Sax",
  [65]="Alto Sax",
  [66]="Tenor Sax",
  [67]="Baritone Sax",
  [68]="Oboe",
  [69]="English Horn",
  [70]="Bassoon",
  [71]="Clarinet",
  [72]="Piccolo",
  [73]="Flute",
  [74]="Recorder",
  [75]="Pan Flute",
  [76]="Blown Bottle",
  [77]="Shakuhachi",
  [78]="Whistle",
  [79]="Ocarina",
  [80]="Lead 1 (square wave)",
  [81]="Lead 2 (sawtooth wave)",
  [82]="Lead 3 (calliope)",
  [83]="Lead 4 (chiffer)",
  [84]="Lead 5 (charang)",
  [85]="Lead 6 (voice solo)",
  [86]="Lead 7 (fifths)",
  [87]="Lead 8 (bass + lead)",
  [88]="Pad 1 (new age Fantasia)",
  [89]="Pad 2 (warm)",
  [90]="Pad 3 (polysynth)",
  [91]="Pad 4 (choir space voice)",
  [92]="Pad 5 (bowed glass)",
  [93]="Pad 6 (metallic pro)",
  [94]="Pad 7 (halo)",
  [95]="Pad 8 (sweep)",
  [96]="FX 1 (rain)",
  [97]="FX 2 (soundtrack)",
  [98]="FX 3 (crystal)",
  [99]="FX 4 (atmosphere)",
  [100]="FX 5 (brightness)",
  [101]="FX 6 (goblins)",
  [102]="FX 7 (echoes, drops)",
  [103]="FX 8 (sci",
  [104]="Sitar",
  [105]="Banjo",
  [106]="Shamisen",
  [107]="Koto",
  [108]="Kalimba",
  [109]="Bag pipe",
  [110]="Fiddle",
  [111]="Shanai",
  [112]="Tinkle Bell",
  [113]="Agogo",
  [114]="Steel Drums",
  [115]="Woodblock",
  [116]="Taiko Drum",
  [117]="Melodic Tom",
  [118]="Synth Drum",
  [119]="Reverse Cymbal",
  [120]="Guitar Fret Noise",
  [121]="Breath Noise",
  [122]="Seashore",
  [123]="Bird Tweet",
  [124]="Telephone Ring",
  [125]="Helicopter",
  [126]="Applause",
  [127]="Gunshot",
};

/* GM families.
 */
 
const char *midi_gm_family_names[16]={
  "Piano",
  "Chromatic",
  "Organ",
  "Guitar",
  "Bass",
  "Solo string",
  "String ensemble",
  "Brass",
  "Solo reed",
  "Solo flute",
  "Synth lead",
  "Synth pad",
  "Synth effects",
  "World",
  "Percussion",
  "Sound effects",
};

/* GM drums.
 */
 
const char *midi_gm_drum_names[128]={
  [35]="Acoustic Bass Drum",
  [36]="Bass Drum 1",
  [37]="Side Stick",
  [38]="Acoustic Snare",
  [39]="Hand Clap",
  [40]="Electric Snare",
  [41]="Low Floor Tom",
  [42]="Closed Hi Hat",
  [43]="High Floor Tom",
  [44]="Pedal Hi-Hat",
  [45]="Low Tom",
  [46]="Open Hi-Hat",
  [47]="Low-Mid Tom",
  [48]="Hi Mid Tom",
  [49]="Crash Cymbal 1",
  [50]="High Tom",
  [51]="Ride Cymbal 1",
  [52]="Chinese Cymbal",
  [53]="Ride Bell",
  [54]="Tambourine",
  [55]="Splash Cymbal",
  [56]="Cowbell",
  [57]="Crash Cymbal 2",
  [58]="Vibraslap",
  [59]="Ride Cymbal 2",
  [60]="Hi Bongo",
  [61]="Low Bongo",
  [62]="Mute Hi Conga",
  [63]="Open Hi Conga",
  [64]="Low Conga",
  [65]="High Timbale",
  [66]="Low Timbale",
  [67]="High Agogo",
  [68]="Low Agogo",
  [69]="Cabasa",
  [70]="Maracas",
  [71]="Short Whistle",
  [72]="Long Whistle",
  [73]="Short Guiro",
  [74]="Long Guiro",
  [75]="Claves",
  [76]="Hi Wood Block",
  [77]="Low Wood Block",
  [78]="Mute Cuica",
  [79]="Open Cuica",
  [80]="Mute Triangle",
  [81]="Open Triangle",

};

/* Noteid.
 */
 
int midi_noteid_repr(char *dst,int dsta,uint8_t noteid) {
  if (!dst||(dsta<0)) dsta=0;
  noteid&=0x7f;
  int octave=noteid/12-1; // Lowest octave is -1, and it does align with the start of names (which is %$#@ "C", not "A").
  char name,modifier=0;
  switch (noteid%12) {
    case 0: name='c'; break;
    case 1: name='c'; modifier='s'; break;
    case 2: name='d'; break;
    case 3: name='d'; modifier='s'; break;
    case 4: name='e'; break;
    case 5: name='f'; break;
    case 6: name='f'; modifier='s'; break;
    case 7: name='g'; break;
    case 8: name='g'; modifier='s'; break;
    case 9: name='a'; break;
    case 10: name='a'; modifier='s'; break;
    case 11: name='b'; break;
  }
  int dstc=2; // always at least name+octave
  if (octave<0) dstc++;
  if (modifier) dstc++;
  if (dstc>dsta) return dstc;
  
  int dstp=0;
  dst[dstp++]=name;
  if (modifier) dst[dstp++]=modifier;
  if (octave<0) { dst[dstp++]='n'; octave=-octave; }
  dst[dstp++]='0'+octave;
  
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

int midi_noteid_eval(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  
  int srcp=0;
  if (srcp>=srcc) return -1;
  char name=src[srcp++];
  if ((name>='A')&&(name<='Z')) name+=0x20;
  if ((name<'a')||(name>'z')) return -1;
  if (srcp>=srcc) return -1;
  int modifier=0;
  if ((src[srcp]=='#')||(src[srcp]=='s')) {
    modifier=1;
    srcp++;
  } else if (src[srcp]=='b') {
    modifier=-1;
    srcp++;
  }
  if (srcp>=srcc) return -1;
  int positive=1;
  if ((src[srcp]=='-')||(src[srcp]=='n')) {
    positive=0;
    if (++srcp>=srcc) return -1;
  }
  if ((src[srcp]<'0')||(src[srcp]>'9')) return -1;
  int octave=src[srcp++];
  if (!positive) {
    // There is only one negative octave; and positive 0..9.
    if (octave!=1) return -1;
    octave=-1;
  }
  if (srcp<srcc) return -1;
  
  int noteid=(octave+1)*12;
  switch (name) {
    case 'c': break;
    case 'd': noteid+=2; break;
    case 'e': noteid+=4; break;
    case 'f': noteid+=5; break;
    case 'g': noteid+=7; break;
    case 'a': noteid+=9; break;
    case 'b': noteid+=11; break;
  }
  noteid+=modifier; // We allow eg "e#", equivalent to "f".
  
  if ((noteid<0)||(noteid>=0x80)) return -1;
  return noteid;
}

/* Frequency in Hz for a MIDI note.
 * (ok ok this is not "text").
 * Regenerate with a little Javascript:
 *   const refnote=0x45; const refrate=440; let dst=[]; 
 *   for (let n=0;n<128;n++) dst.push(refrate*Math.pow(2,(n-refnote)/12)); 
 *   JSON.stringify(dst);
 */
 
const float midi_note_frequency[128]={
  8.175798915643707,8.661957218027252,9.177023997418988,9.722718241315027,10.300861153527183,10.913382232281373,11.562325709738577,12.249857374429663,
  12.978271799373287,13.75,14.567617547440307,15.433853164253883,16.351597831287414,17.323914436054505,18.354047994837977,19.445436482630054,
  20.601722307054366,21.826764464562746,23.124651419477154,24.499714748859326,25.956543598746574,27.5,29.13523509488062,30.86770632850775,
  32.70319566257483,34.64782887210902,36.70809598967594,38.89087296526011,41.20344461410874,43.653528929125486,46.24930283895431,48.999429497718666,
  51.91308719749314,55,58.27047018976124,61.7354126570155,65.40639132514966,69.29565774421803,73.41619197935188,77.78174593052022,82.40688922821748,
  87.30705785825097,92.49860567790861,97.99885899543733,103.82617439498628,110,116.54094037952248,123.47082531403103,130.8127826502993,138.59131548843604,
  146.8323839587038,155.56349186104043,164.81377845643496,174.61411571650194,184.99721135581723,195.99771799087463,207.65234878997256,220,
  233.08188075904496,246.94165062806206,261.6255653005986,277.1826309768721,293.6647679174076,311.12698372208087,329.6275569128699,349.2282314330039,
  369.99442271163446,391.99543598174927,415.3046975799451,440,466.1637615180899,493.8833012561241,523.2511306011972,554.3652619537442,587.3295358348151,
  622.2539674441618,659.2551138257398,698.4564628660078,739.9888454232689,783.9908719634986,830.6093951598903,880,932.3275230361799,987.7666025122483,
  1046.5022612023945,1108.7305239074883,1174.6590716696303,1244.5079348883235,1318.5102276514797,1396.9129257320155,1479.9776908465378,1567.981743926997,
  1661.2187903197805,1760,1864.6550460723597,1975.533205024496,2093.004522404789,2217.461047814977,2349.31814333926,2489.015869776647,2637.0204553029594,
  2793.825851464031,2959.9553816930757,3135.9634878539946,3322.437580639561,3520,3729.3100921447194,3951.066410048992,4186.009044809578,4434.922095629954,
  4698.63628667852,4978.031739553294,5274.040910605919,5587.651702928062,5919.910763386151,6271.926975707989,6644.875161279122,7040,7458.620184289437,
  7902.132820097988,8372.018089619156,8869.844191259906,9397.272573357044,9956.063479106588,10548.081821211836,11175.303405856126,11839.821526772303,12543.853951415975
};
