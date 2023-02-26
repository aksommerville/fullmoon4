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
    this.sustainTime = 0; // Absolute context time when we enter sustain -- minimum time to begin release.
    this.sustainLevel = 0;
    this.modulatorReleaseTime = 0;
    this.modulatorReleaseLevel = 0;
    this.modulatorSustainLevel = 0;
    this.oscillators = [];
    this.frequencyTargets = []; // for bending
    this.unbentFrequency = 0;
    this.unbentModFrequency = 0;
  }
  
  setup(instrument, noteId, velocity, delayMs) {
    if (this.node) throw new Error(`Multiple calls to AudioVoice.setup`);
    this.instrument = instrument;
    this.noteId = noteId;
    const frequency = 440 * 2 ** ((this.noteId - 0x45) / 12);
    const ctx = this.synthesizer.context;
    const startTime = ctx.currentTime + ((delayMs > 0) ? (delayMs / 1000) : 0);
    
    // The three modes (Bandpass, Oscillator, FM):
    if (instrument.bpq) {
      this._initBandpass(ctx, instrument, frequency);
    } else {
      this._initOscillator(ctx, instrument, frequency);
      if (instrument.modRange) {
        this._initFm(ctx, instrument, frequency, velocity, startTime);
      }
    }
    
    this._initEnvelope(ctx, instrument, velocity, startTime);
    if (this.channel.bend !== 1) this.bend(this.channel.bend);
  }
  
  _initBandpass(ctx, ins, frequency) {
    this.unbentFrequency = frequency;
    const buffer = AudioVoice.getSharedNoiseBuffer(ctx.sampleRate);
    const bufferLengthSeconds = buffer.length / ctx.sampleRate;
    const noiseNode = new AudioBufferSourceNode(ctx, {
      buffer,
      channelCount: 1,
      loop: true,
      loopEnd: bufferLengthSeconds,
      loopStart: 0,
    });
    noiseNode.start(0, Math.random() * bufferLengthSeconds);
    this.oscillators.push(noiseNode); // It's not an "Oscillator" node, but it does have a stop(), and that must get called.
    let filter = new BiquadFilterNode(ctx, {
      type: "bandpass",
      frequency,
      Q: ins.bpq,
    });
    this.frequencyTargets.push(filter);
    noiseNode.connect(filter);
    if (ins.bpq2) {
      const secondBreakfast = new BiquadFilterNode(ctx, {
        type: "bandpass",
        frequency,
        Q: ins.bpq2,
      });
      this.frequencyTargets.push(secondBreakfast);
      filter.connect(secondBreakfast);
      filter = secondBreakfast;
    }
    if (ins.bpBoost !== 1) {
      const bigGain = new GainNode(ctx, {
        gain: ins.bpBoost,
      });
      filter.connect(bigGain);
      this.oscillator = bigGain;
    } else {
      this.oscillator = filter;
    }
  }
  
  static getSharedNoiseBuffer(sampleRate) {
    if (!AudioVoice.sharedNoiseBuffer) {
      const pcm = new Float32Array(sampleRate); // length not really important, long enough to not repeat itself much.
      for (let i=pcm.length; i-->0; ) pcm[i] = Math.random() * 2 - 1;
      AudioVoice.sharedNoiseBuffer = new AudioBuffer({
        length: pcm.length,
        numberOfChannels: 1,
        sampleRate,
        channelCount: 1,
      });
      AudioVoice.sharedNoiseBuffer.copyToChannel(pcm, 0);
    }
    return AudioVoice.sharedNoiseBuffer;
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
    this.frequencyTargets.push(this.oscillator);
  }
  
  _initFm(ctx, ins, frequency, velocity, startTime) {
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
      this.modulatorSustainLevel = r * sustainLevel;
      modGain.gain.value = r * startLevel;
      modGain.gain.setValueAtTime(r * startLevel, startTime);
      modGain.gain.linearRampToValueAtTime(r * attackLevel, startTime + attackTime);
      modGain.gain.linearRampToValueAtTime(r * sustainLevel, startTime + attackTime + decayTime);
    } else {
      modGain.gain.setValueAtTime(r, startTime);
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
      modLfoOscillator.start(); // Should delay until startTime... Does it matter?
      this.oscillators.push(modLfoOscillator);
    }
    modOscillator.connect(modGain);
    modOscillator.start();
    this.oscillators.push(modOscillator);
    this.modOscillator = modOscillator;
    modGain.connect(this.oscillator.frequency);
    this.modulator = modGain;
  }
  
  _initEnvelope(ctx, ins, velocity, startTime) {
    let { attackLevel, sustainLevel, attackTime, decayTime, releaseTime } = ins.env.apply(velocity / 0x7f);
    attackLevel *= this.channel.volume;
    sustainLevel *= this.channel.volume;
    this.sustainTime = startTime + attackTime + decayTime;
    this.sustainLevel = sustainLevel;
    this.releaseTime = releaseTime;
    this.node = new GainNode(ctx);
    this.node.gain.value = 0;
    this.node.gain.setValueAtTime(0, startTime);
    this.node.gain.linearRampToValueAtTime(attackLevel, startTime + attackTime);
    this.node.gain.linearRampToValueAtTime(sustainLevel, startTime + attackTime + decayTime);
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
  
  release(velocity, delayMs) {
    if (this.finishTime) return;
    let releaseTime = this.synthesizer.context.currentTime + ((delayMs > 0) ? (delayMs / 1000) : 0);
    if (releaseTime < this.sustainTime) {
      releaseTime = this.sustainTime;
    }
    this.finishTime = releaseTime + this.releaseTime + 0.010;
    if (this.node) {
      // Important to stake the current time, otherwise the ramp goes from the start of sustain, way in the past.
      this.node.gain.setValueAtTime(this.sustainLevel, releaseTime);
      this.node.gain.linearRampToValueAtTime(0, releaseTime + this.releaseTime);
    }
    if (this.modulator && this.modulatorReleaseTime) {
      this.modulator.gain.setValueAtTime(this.modulatorSustainLevel, releaseTime);
      this.modulator.gain.linearRampToValueAtTime(this.modulatorReleaseLevel, releaseTime + this.modulatorReleaseTime);
    }
    this.chid = -1;
    this.noteId = -1;
    this.channel = null;
  }
  
  bend(multiplier, delayMs) {
    const when = this.synthesizer.context.currentTime + ((delayMs > 0) ? (delayMs / 1000) : 0);
    for (const node of this.frequencyTargets) {
      node.frequency.setValueAtTime(this.unbentFrequency * multiplier, when);
    }
    if (this.modOscillator) {
      this.modOscillator.setValueAtTime(this.unbentModFrequency * multiplier, when);
    }
    // modLfoOscillator is not impacted; that's pegged to real time.
  }
}
