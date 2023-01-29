/* AudioChannel.js
 * One logical channel of output.
 * This has nothing to do with mixing.
 * It's a matter of holding configuration, mostly.
 */
 
import { AudioVoice } from "./AudioVoice.js";
 
export class AudioChannel {
  constructor(synthesizer, chid, pid) {
    this.synthesizer = synthesizer;
    this.chid = chid;
    this.pid = pid;
    console.log(`AudioChannel.ctor ${pid}`);
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
    console.log(`AudioChannel.event ${opcode} ${a} ${b}`);
    if (opcode === 0x90) {
      const voice = new AudioVoice(this.synthesizer, this, a, b);
      voice.channel = this;
      voice.chid = this.chid;
      this.synthesizer.voices.push(voice);
    }
  }
}
