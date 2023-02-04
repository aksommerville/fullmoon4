/* AudioChannel.js
 * One logical channel of output.
 * This has nothing to do with mixing.
 * It's a matter of holding configuration, mostly.
 */
 
import { AudioVoice } from "./AudioVoice.js";
import { Envelope } from "./Envelope.js";
import { Instrument } from "./Instrument.js";

/* Instrument envelopes and MIDI channel volume control should both be normalized 0..1.
 * However, we surely don't want individual channels producing voices at unit level!
 * I'm thinking 1/3 is reasonable as an absolute ceiling...
 */
const UNIVERSAL_CHANNEL_VOLUME_LIMIT = 0.333;
 
export class AudioChannel {
  constructor(synthesizer, chid, pid, instrument) {
    this.synthesizer = synthesizer;
    this.chid = chid;
    this.pid = pid;
    this.instrument = instrument || new Instrument(null, pid);
    this.volume = UNIVERSAL_CHANNEL_VOLUME_LIMIT * 0.5;
    
    //TODO pitch wheel
  }
  
  /* Usually not meaningful.
   * But maybe channels will produce output sometimes? In which case, use this hook to drop any nodes from the context.
   * "abort" is permanent, but "silence" we should remain configured and ready.
   */
  abort() {
  }
  silence() {
  }
  
  event(opcode, a, b) {
    switch (opcode) {
      // 0x80 Note Off are not delivered to channels; Synthesizer delivers straight to the appropriate AudioVoice.
      case 0x90: {
          const voice = new AudioVoice(this.synthesizer, this, this.chid);
          voice.setup(this.instrument, a, b);
          this.synthesizer.voices.push(voice);
        } break;
      case 0xb0: switch (a) {
          case 0x01: break; // Mod
          case 0x04: break; // Foot
          case 0x07: this.volume = UNIVERSAL_CHANNEL_VOLUME_LIMIT * (b / 0x7f); break; // NB does not apply to existing notes
          case 0x0a: break; // Pan
          case 0x0b: break; // Expression
          case 0x40: break; // Sustain
        } break;
      case 0xe0: this.wheelEvent(a | (b << 7)); break;
    }
  }
  
  wheelEvent(vi) {
    //TODO important that we apply this to both new and existing voices.
    console.log(`TODO AudioChannel.wheelEvent ${vi}`);
  }
}