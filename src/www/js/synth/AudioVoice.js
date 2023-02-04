/* AudioVoice.js
 * Single note of output.
 */
 
export class AudioVoice {
  constructor(synthesizer, channel, chid) {
    this.synthesizer = synthesizer;
    this.channel = channel;
    this.chid = chid;
    this.noteId = -1;
    this.velocity = 0;
    this.finishTime = null;
    this.oscillator = null;
    this.node = null;
    this.modulator = null;
    this.releaseTime = 0;
    this.modulatorReleaseTime = 0;
    this.modulatorReleaseLevel = 0;
  }
  
  /* (noteId) is MIDI 0..0x7f.
   * (wave) is an OscillatorNode type string, or a PeriodicWave.
   * (modulation): null or {
   *   absoluteRate: hz, overrides (rate)
   *   rate: multiplier
   *   range: low positive number, 1 is sensible
   *   env: Envelope for range
   *   rangeLfoRate: hz
   * }
   */
  setupOscillator(noteId, velocity, wave, modulation) {
    this.noteId = noteId;
    const frequency = 440 * 2 ** ((this.noteId - 0x45) / 12);
    const ctx = this.synthesizer.context;

    const oscillatorOptions = { frequency };
    if (typeof(wave) === "string") {
      oscillatorOptions.type = wave;
    } else if (wave instanceof PeriodicWave) {
      oscillatorOptions.type = "custom";
      oscillatorOptions.periodicWave = wave;
    } else {
      oscillatorOptions.type = "sine";
    }
    this.oscillator = new OscillatorNode(ctx, oscillatorOptions);
    
    if (modulation) {
      const modOscillator = new OscillatorNode(ctx, {
        frequency: modulation.absoluteRate || (frequency * modulation.rate),
        type: "sine",
      });
      const modGain = new GainNode(ctx);
      const r = frequency * modulation.range;
      if (modulation.env) {
        const { startLevel, attackLevel, sustainLevel, attackTime, decayTime, releaseTime, endLevel } = modulation.env.apply(velocity / 0x7f);
        this.modulatorReleaseTime = releaseTime;
        this.modulatorReleaseLevel = r * endLevel;
        modGain.gain.setValueAtTime(r * startLevel, ctx.currentTime);
        modGain.gain.linearRampToValueAtTime(r * attackLevel, ctx.currentTime + attackTime);
        modGain.gain.linearRampToValueAtTime(r * sustainLevel, ctx.currentTime + attackTime + decayTime);
      } else {
        modGain.gain.setValueAtTime(r, ctx.currentTime);
      }
      if (modulation.rangeLfoRate) {
        const modLfoOscillator = new OscillatorNode(ctx, {
          frequency: modulation.rangeLfoRate,
          type: "sine",
        });
        const modLfoScaleUp = new GainNode(ctx);
        modLfoScaleUp.gain.value = r;
        modLfoOscillator.connect(modLfoScaleUp);
        modLfoScaleUp.connect(modGain.gain);
        modLfoOscillator.start();
      }
      modOscillator.connect(modGain);
      modOscillator.start();
      modGain.connect(this.oscillator.frequency);
      this.modulator = modGain;
    }

    this.oscillator.start();
  }
  
  setupEnvelope(velocity, env) {
    if (!this.oscillator) throw new Error(`Set oscillator before envelope`);
    const { attackLevel, sustainLevel, attackTime, decayTime, releaseTime } = env.apply(velocity / 0x7f);
    this.releaseTime = releaseTime;
    this.node = new GainNode(this.synthesizer.context);
    this.node.gain.setValueAtTime(0, this.synthesizer.context.currentTime);
    this.node.gain.linearRampToValueAtTime(attackLevel, this.synthesizer.context.currentTime + attackTime);
    this.node.gain.linearRampToValueAtTime(sustainLevel, this.synthesizer.context.currentTime + attackTime + decayTime);
    this.oscillator.connect(this.node);
    this.node.connect(this.synthesizer.context.destination);
  }

  abort() {
    if (this.node) this.node.disconnect();
  }
  
  isFinished() {
    if (!this.finishTime) return false;
    if (this.synthesizer.context.currentTime < this.finishTime) return false;
    return true;
  }
  
  release(velocity) {
    this.chid = -1;
    this.noteId = -1;
    this.channel = null;
    if (this.finishTime) return;
    this.finishTime = this.synthesizer.context.currentTime + this.releaseTime + 0.010;
    if (this.node) {
      // Important to stake the current time, otherwise the ramp goes from the start of sustain, way in the past.
      this.node.gain.setValueAtTime(this.node.gain.value, this.synthesizer.context.currentTime);
      this.node.gain.linearRampToValueAtTime(0, this.synthesizer.context.currentTime + this.releaseTime);
      //this.modulatorGain.gain.setValueAtTime(this.modulatorGain.gain.value, this.synthesizer.context.currentTime);
      //this.modulatorGain.gain.linearRampToValueAtTime(0, this.synthesizer.context.currentTime + this.releaseTime);
    }
    if (this.modulator && this.modulatorReleaseTime) {
      this.modulator.gain.setValueAtTime(this.modulator.gain.value, this.synthesizer.context.currentTime);
      this.modulator.gain.linearRampToValueAtTime(this.modulatorReleaseLevel, this.synthesizer.context.currentTime + this.modulatorReleaseTime);
    }
  }
}
