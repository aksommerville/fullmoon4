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
    this.instrument = null;
    this.finishTime = null;
    this.oscillator = null;
    this.node = null;
    this.modulator = null;
    this.modOscillator = null;
    this.releaseTime = 0;
    this.modulatorReleaseTime = 0;
    this.modulatorReleaseLevel = 0;
    this.oscillators = [];
    this.unbentFrequency = 0;
    this.unbentModFrequency = 0;
  }
  
  setup(instrument, noteId, velocity) {
    if (this.node) throw new Error(`Multiple calls to AudioVoice.setup`);
    this.instrument = instrument;
    this.noteId = noteId;
    const frequency = 440 * 2 ** ((this.noteId - 0x45) / 12);
    const ctx = this.synthesizer.context;
    this._initOscillator(ctx, instrument, frequency);
    if (instrument.modRange) {
      this._initFm(ctx, instrument, frequency, velocity);
    }
    this._initEnvelope(ctx, instrument, velocity);
    if (this.channel.bend !== 1) this.bend(this.channel.bend);
  }
  
  _initOscillator(ctx, ins, frequency) {
    this.unbentFrequency = frequency;
    const oscillatorOptions = { frequency };
    if (typeof(ins.wave) === "string") {
      oscillatorOptions.type = ins.wave;
    } else if (ins.wave instanceof Array) {
      oscillatorOptions.type = "custom";
      oscillatorOptions.periodicWave = new PeriodicWave(ctx, {
        real: new Float32Array(ins.wave),
      });
    } else {
      oscillatorOptions.type = "sine";
    }
    this.oscillator = new OscillatorNode(ctx, oscillatorOptions);
    this.oscillator.start();
    this.oscillators.push(this.oscillator);
  }
  
  _initFm(ctx, ins, frequency, velocity) {
    this.unbentModFrequency = ins.modAbsoluteRate || (frequency * ins.modRate);
    const modOscillator = new OscillatorNode(ctx, {
      frequency: this.unbentModFrequency,
      type: "sine", // TODO Do we want non-sine modulation oscillators?
    });
    const modGain = new GainNode(ctx);
    const r = frequency * ins.modRange;
    if (ins.modEnv) {
      const { startLevel, attackLevel, sustainLevel, attackTime, decayTime, releaseTime, endLevel } = ins.modEnv.apply(velocity / 0x7f);
      this.modulatorReleaseTime = releaseTime;
      this.modulatorReleaseLevel = r * endLevel;
      modGain.gain.setValueAtTime(r * startLevel, ctx.currentTime);
      modGain.gain.linearRampToValueAtTime(r * attackLevel, ctx.currentTime + attackTime);
      modGain.gain.linearRampToValueAtTime(r * sustainLevel, ctx.currentTime + attackTime + decayTime);
    } else {
      modGain.gain.setValueAtTime(r, ctx.currentTime);
    }
    if (ins.modRangeLfoRate) {
      const modLfoOscillator = new OscillatorNode(ctx, {
        frequency: ins.modRangeLfoRate,
        type: "sine",
      });
      const modLfoScaleUp = new GainNode(ctx);
      modLfoScaleUp.gain.value = r;
      modLfoOscillator.connect(modLfoScaleUp);
      modLfoScaleUp.connect(modGain.gain);
      modLfoOscillator.start();
      this.oscillators.push(modLfoOscillator);
    }
    modOscillator.connect(modGain);
    modOscillator.start();
    this.oscillators.push(modOscillator);
    this.modOscillator = modOscillator;
    modGain.connect(this.oscillator.frequency);
    this.modulator = modGain;
  }
  
  _initEnvelope(ctx, ins, velocity) {
    let { attackLevel, sustainLevel, attackTime, decayTime, releaseTime } = ins.env.apply(velocity / 0x7f);
    attackLevel *= this.channel.volume;
    sustainLevel *= this.channel.volume;
    this.releaseTime = releaseTime;
    this.node = new GainNode(ctx);
    this.node.gain.setValueAtTime(0, ctx.currentTime);
    this.node.gain.linearRampToValueAtTime(attackLevel, ctx.currentTime + attackTime);
    this.node.gain.linearRampToValueAtTime(sustainLevel, ctx.currentTime + attackTime + decayTime);
    this.oscillator.connect(this.node);
    this.node.connect(ctx.destination);
  }

  abort() {
    if (this.node) {
      this.node.disconnect();
    }
    this.oscillator = null;
    this.node = null;
    this.modulator = null;
    for (const o of this.oscillators) o.stop();
    this.oscillators = [];
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
  
  bend(multiplier) {
    if (this.oscillator) {
      this.oscillator.frequency.value = this.unbentFrequency * multiplier;
    }
    if (this.modOscillator) {
      this.modOscillator.frequency.value = this.unbentModFrequency * multiplier;
    }
    // modLfoOscillator is not impacted; that's pegged to real time.
  }
}
