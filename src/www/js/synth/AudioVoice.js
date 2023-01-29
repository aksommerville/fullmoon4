/* AudioVoice.js
 * Single note of output.
 */
 
export class AudioVoice {
  constructor(synthesizer, channel, noteId, velocity) {
    this.synthesizer = synthesizer;
    this.channel = channel;
    this.noteId = noteId;
    this.chid = -1; // owner should set
    
    //XXX playing around...
    
    const frequency = 440 * 2 ** ((this.noteId - 0x45) / 12);
    const type = "sine";//TODO
    this.oscillator = new OscillatorNode(this.synthesizer.context, { frequency, type });
    
    this.node = new GainNode(this.synthesizer.context);
    this.node.gain.setValueAtTime(0, this.synthesizer.context.currentTime);
    //TODO envelope
    this.node.gain.linearRampToValueAtTime(0.400, this.synthesizer.context.currentTime + 0.010);
    this.node.gain.linearRampToValueAtTime(0.100, this.synthesizer.context.currentTime + 0.010 + 0.015);
    this.oscillator.connect(this.node);
    
    this.node.connect(this.synthesizer.context.destination);
    this.oscillator.start();
    
    this.finishTime = null;
    
    console.log(`voice started`);
  }

  abort() {
    this.node.disconnect();
  }
  
  isFinished() {
    if (!this.finishTime) return false;
    if (Date.now() < this.finishTime) return false;
    return true;
  }
  
  release(velocity) {
    console.log(`AudioVoice.release`);
    this.chid = -1;
    this.noteId = -1;
    this.channel = null;
    if (this.finishTime) return;
    this.finishTime = Date.now() + 510;
    if (this.node) {
      this.node.gain.linearRampToValueAtTime(0, this.synthesizer.context.currentTime + 0.500);
    }
  }
}
