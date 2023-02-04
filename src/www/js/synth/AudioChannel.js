/* AudioChannel.js
 * One logical channel of output.
 * This has nothing to do with mixing.
 * It's a matter of holding configuration, mostly.
 */
 
import { AudioVoice } from "./AudioVoice.js";
import { Envelope } from "./Envelope.js";
 
export class AudioChannel {
  constructor(synthesizer, chid, pid) {
    this.synthesizer = synthesizer;
    this.chid = chid;
    this.pid = pid;
    this.wave = "sine"; // OscillatorNode.type or PeriodicWave
    this.env = new Envelope();
    this.modulation = null; // See AudioVoice.setupOscillator
    
    /* warble */
    this.modulation = {
      absoluteRate: 4,
      range: 0.030,
    };
    /**/
    
    /* classic fm *
    this.modulation = {
      rate: 0.5,
      range: 4,
    };
    /**/
    
    /* twang *
    this.modulation = {
      rate: 1,
      range: 1,
      env: new Envelope(),
    };
    this.modulation.env.lo[2] = 0.8;
    this.modulation.env.hi[2] = 1.0;
    this.modulation.env.lo[4] = 0.8;
    this.modulation.env.hi[4] = 1.0;
    this.modulation.env.lo[6] = 4;
    this.modulation.env.hi[6] = 5;
    /**/
    
    /* auto-wah, and can also multiply against an envelope
    this.modulation = {
      rate: 4,
      range: 1.25,
      rangeLfoRate: 3,
      env: new Envelope(),
    };
    this.modulation.env.lo[2] = 0.8;
    this.modulation.env.hi[2] = 1.0;
    this.modulation.env.lo[4] = 0.8;
    this.modulation.env.hi[4] = 1.0;
    /**/
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
    if (opcode === 0x90) {
      const voice = new AudioVoice(this.synthesizer, this, this.chid);
      voice.setupOscillator(a, b, this.wave, this.modulation);
      voice.setupEnvelope(b, this.env);
      this.synthesizer.voices.push(voice);
    }
  }
}
