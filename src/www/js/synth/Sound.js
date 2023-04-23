/* Sound.js
 * For printing short PCM snippets (sound effects), in our private format.
 * Detailed docs at etc/doc/instrument-format.md
 */
 
export class Sound {
  constructor(src, id, rate) {
    this.id = id;
    this.pcm = null; // Float32Array
    this.buffer = null; // AudioBuffer
    
    if (!rate || (rate < 200) || (rate > 200000)) throw new Error(`Invalid rate ${rate} for Sound`);
    this.rate = rate;
    
    if (!src) this._setEmpty();
    else if (src instanceof Uint8Array) this._decode(src);
    else if (src instanceof ArrayBuffer) this._decode(new Uint8Array(src));
    else if (src instanceof Sound) this._copy(src);
    else throw new Error(`Unsuitable input for Sound`);
  }
  
  _setEmpty() {
    this.pcm = new Float32Array([0]);
    this._makeAudioBuffer();
  }
  
  _copy(src) {
    this.rate = src.rate;
    this.pcm = new Float32Array(src.pcm);
    this._makeAudioBuffer();
  }
  
  _decode(src) {
    // First two bytes are length (u2.6) and buffer count.
    // If short, or if either is zero, we are empty.
    if ((src.length < 2) || !src[0] || !src[1]) {
      this._setEmpty();
      return;
    }
    const samplec = Math.max(1, ~~((src[0] * this.rate) / 64));
    this.buffers = [];
    for (let i=src[1]; i-->0; ) this.buffers.push(new Float32Array(samplec));
    for (let srcp=2; srcp<src.length; ) {
      srcp += this._decodeCommand(src, srcp);
    }
    this.pcm = this.buffers[0];
    delete this.buffers;
    this._makeAudioBuffer();
  }
  
  _makeAudioBuffer() {
    this.buffer = new AudioBuffer({
      length: this.pcm.length,
      numberOfChannels: 1,
      sampleRate: this.rate,
      channelCount: 1,
    });
    this.buffer.copyToChannel(this.pcm, 0);
  }
  
  /* Called from within _decode only.
   * (this.buffers) must be set.
   * We return the length consumed, >0, or throw an exception.
   */
  _decodeCommand(src, srcp) {
    const srcp0 = srcp;
    const opcode = src[srcp++];
    const bufferId = src[srcp++];
    if (bufferId >= this.buffers.length) throw new Error(`sound:${this.id}:${srcp-2}/${src.length}: Buffer ${bufferId} not defined`);
    try {
      switch (opcode) {
        
        case 0x00: return src.length - srcp0; // EOF
        case 0x01: this._cmd_noise(bufferId); break;
        case 0x02: srcp += this._cmd_wave_fixed_name(bufferId, src, srcp); break;
        case 0x03: srcp += this._cmd_wave_fixed_harm(bufferId, src, srcp); break;
        case 0x04: srcp += this._cmd_wave_env_name(bufferId, src, srcp); break;
        case 0x05: srcp += this._cmd_wave_env_harm(bufferId, src, srcp); break;
        case 0x06: srcp += this._cmd_fm_fixed_name(bufferId, src, srcp); break;
        case 0x07: srcp += this._cmd_fm_fixed_harm(bufferId, src, srcp); break;
        case 0x08: srcp += this._cmd_fm_env_name(bufferId, src, srcp); break;
        case 0x09: srcp += this._cmd_fm_env_harm(bufferId, src, srcp); break;
        case 0x0a: srcp += this._cmd_gain(bufferId, src, srcp); break;
        case 0x0b: srcp += this._cmd_env(bufferId, src, srcp); break;
        case 0x0c: srcp += this._cmd_mix(bufferId, src, srcp); break;
        case 0x0d: srcp += this._cmd_norm(bufferId, src, srcp); break;
        case 0x0e: srcp += this._cmd_delay(bufferId, src, srcp); break;
        case 0x0f: srcp += this._cmd_bandpass(bufferId, src, srcp); break;
        
        default: throw new Error(`Unknown opcode ${opcode}`);
      }
    } catch (e) {
      e.message = `sound:${this.id}:${srcp0}/${src.length}: ${e.message}`;
      throw e;
    }
    return srcp - srcp0;
  }
  
  _cmd_noise(bufferId) {
    const v = this.buffers[bufferId];
    for (let i=v.length; i-->0; ) v[i] = Math.random() * 2 - 1;
  }
  
  _cmd_wave_fixed_name(bufferId, src, srcp) {
    const v = this.buffers[bufferId];
    const name = src[srcp + 2];
    switch (name) {
      case 0: { // sine
          const rate = this._readRateRadiansFromHz(src, srcp);
          for (let i=0, t=0; i<v.length; i++, t+=rate) {
            v[i] = Math.sin(t);
          }
        } break;
      case 1: { // square
          const rate = this._readRateNormFromHz(src, srcp);
          for (let i=0, t=0; i<v.length; i++, t+=rate) {
            if (t >= 1) t -= 1;
            v[i] = (t < 0.5) ? 1 : -1;
          }
        } break;
      case 2: { // sawtooth
          const rate = this._readRateNormFromHz(src, srcp) * 2;
          for (let i=0, t=-1; i<v.length; i++, t+=rate) {
            if (t >= 1) t -= 2;
            v[i] = t;
          }
        } break;
      case 3: { // triangle
          const rate = this._readRateNormFromHz(src, srcp) * 4;
          for (let i=0, t=-1; i<v.length; i++, t+=rate) {
            if (t >= 3) t -= 4;
            v[i] = (t < 1) ? t : (2 - t);
          }
        } break;
      default: throw new Error(`Unknown wave name ${name}`);
    }
    return 3;
  }

  _cmd_wave_fixed_harm(bufferId, src, srcp) {
    const srcp0 = srcp;
    const v = this.buffers[bufferId];
    const rate = this._readRateRadiansFromHz(src, srcp);
    srcp += 2;
    const coefc = src[srcp++];
    const voices = []; // [p,d,coef]
    for (let i=1; i<=coefc; i++, srcp+=2) {
      const coef = ((src[srcp] << 8) | src[srcp + 1]) / 0xffff;
      if (coef <= 0) continue;
      voices.push([0, rate * i, coef]);
    }
    for (let i=0; i<v.length; i++) {
      v[i] = 0;
      for (const voice of voices) {
        voice[0] += voice[1];
        if (voice[0] > Math.PI) voice[0] -= Math.PI * 2;
        v[i] += Math.sin(voice[0]) * voice[2];
      }
    }
    return srcp - srcp0;
  }
  
  _cmd_wave_env_name(bufferId, src, srcp) {
    const srcp0 = srcp;
    const v = this.buffers[bufferId];
    const points = [];
    srcp += this._readEnv(points, src, srcp, false);
    const name = src[srcp++];
    
    let synthesize;
    switch (name) {
      case 0: synthesize = p => Math.sin(p * Math.PI * 2); break;
      case 1: synthesize = p => ((p < 0.5) ? 1 : -1); break;
      case 2: synthesize = p => p * 2 - 1; break;
      case 3: synthesize = p => ((p < 0.5) ? (p * 4 - 1) : (3 - p * 4)); break;
      default: throw new Error(`Unknown wave name ${name}`);
    }
    
    // (points) values are currently in Hz. Convert to normalized steps.
    for (const point of points) {
      point[1] /= this.rate;
    }
    
    for (let i=0, p=0, pointsp=0, dp=0, ddp=0; i<v.length; i++, p+=dp, dp+=ddp) {
      if (pointsp < points.length) {
        if (i >= points[pointsp][0]) {
          dp = points[pointsp][1];
          pointsp++;
          if (pointsp >= points.length) {
            ddp = 0;
          } else {
            ddp = (points[pointsp][1] - points[pointsp-1][1]) / Math.max(1, (points[pointsp][0] - points[pointsp-1][0]));
          }
        }
      }
      if (p >= 1) p -= 1;
      v[i] = synthesize(p);
    }
    
    return srcp - srcp0;
  }
  
  _cmd_wave_env_harm(bufferId, src, srcp) {
    const srcp0 = srcp;
    const v = this.buffers[bufferId];

    const points = [];
    srcp += this._readEnv(points, src, srcp, false);
    for (const point of points) {
      point[1] = (point[1] * Math.PI * 2) / this.rate;
    }
    
    const coefc = src[srcp++];
    const voices = []; // [p,d,coef]
    for (let i=1; i<=coefc; i++, srcp+=2) {
      const coef = ((src[srcp] << 8) | src[srcp + 1]) / 0xffff;
      if (coef <= 0) continue;
      voices.push([0, i, coef]);
    }

    for (let i=0, pointsp=0, dp=0, ddp=0; i<v.length; i++, dp+=ddp) {
      if (pointsp < points.length) {
        if (i >= points[pointsp][0]) {
          dp = points[pointsp][1];
          pointsp++;
          if (pointsp >= points.length) {
            ddp = 0;
          } else {
            ddp = (points[pointsp][1] - points[pointsp-1][1]) / Math.max(1, (points[pointsp][0] - points[pointsp-1][0]));
          }
        }
      }
      v[i] = 0;
      for (const voice of voices) {
        voice[0] += dp * voice[1];
        if (voice[0] > Math.PI) voice[0] -= Math.PI * 2;
        v[i] += Math.sin(voice[0]) * voice[2];
      }
    }
    
    return srcp - srcp0;
  }
  
  _cmd_fm_fixed_name(bufferId, src, srcp) {
    const srcp0 = srcp;
    const rate = this._readRateNormFromHz(src, srcp);
    const modrate = ((src[srcp + 2] << 8) | src[srcp + 3]) / 256; // u8.8
    srcp += 4;
    let rangePoints = [];
    srcp += this._readEnv(rangePoints, src, srcp, false);
    rangePoints = rangePoints.map(v => [v[0], v[1]/1000]);
    const name = src[srcp++];
    this._fm(bufferId, [[0, rate]], modrate, rangePoints, name);
    return srcp - srcp0;
  }
  
  _cmd_fm_fixed_harm(bufferId, src, srcp) {
    const srcp0 = srcp;
    const rate = this._readRateNormFromHz(src, srcp);
    const modrate = ((src[srcp + 2] << 8) | src[srcp + 3]) / 256; // u8.8
    srcp += 4;
    let rangePoints = [];
    srcp += this._readEnv(rangePoints, src, srcp, false);
    rangePoints = rangePoints.map(v => [v[0], v[1]/1000]);
    const coefc = src[srcp++];
    const voices = []; // [p,d,coef]
    for (let i=1; i<=coefc; i++, srcp+=2) {
      const coef = ((src[srcp] << 8) | src[srcp + 1]) / 0xffff;
      if (coef <= 0) continue;
      voices.push([0, i, coef]);
    }
    this._fm(bufferId, [[0, rate]], modrate, rangePoints, voices);
    return srcp - srcp0;
  }
  
  _cmd_fm_env_name(bufferId, src, srcp) {
    const srcp0 = srcp;
    const ratePoints = [];
    srcp += this._readEnv(ratePoints, src, srcp, false);
    for (const point of ratePoints) point[1] = point[1] / this.rate;
    const modrate = ((src[srcp] << 8) | src[srcp + 1]) / 256; // u8.8
    srcp += 2;
    let rangePoints = [];
    srcp += this._readEnv(rangePoints, src, srcp, false);
    rangePoints = rangePoints.map(v => [v[0], v[1]/1000]);
    const name = src[srcp++];
    this._fm(bufferId, ratePoints, modrate, rangePoints, name);
    return srcp - srcp0;
  }
  
  _cmd_fm_env_harm(bufferId, src, srcp) {
    const srcp0 = srcp;
    const ratePoints = [];
    srcp += this._readEnv(ratePoints, src, srcp, false);
    for (const point of ratePoints) point[1] = point[1] / this.rate;
    const modrate = ((src[srcp] << 8) | src[srcp + 1]) / 256; // u8.8
    srcp += 2;
    let rangePoints = [];
    srcp += this._readEnv(rangePoints, src, srcp, false);
    rangePoints = rangePoints.map(v => [v[0], v[1]/1000]);
    const coefc = src[srcp++];
    const voices = []; // [p,d,coef]
    for (let i=1; i<=coefc; i++, srcp+=2) {
      const coef = ((src[srcp] << 8) | src[srcp + 1]) / 0xffff;
      if (coef <= 0) continue;
      voices.push([0, i, coef]);
    }
    this._fm(bufferId, ratePoints, modrate, rangePoints, voices);
    return srcp - srcp0;
  }
  
  // (ratePoints) takes normalized rates.
  // (nameOrVoices) is an integer wave name, or [[p,d,coef]...]
  _fm(bufferId, ratePoints, modrate, rangePoints, nameOrVoices) {
    const v = this.buffers[bufferId];
    
    let synthesize;
    if (typeof(nameOrVoices) === "number") switch (nameOrVoices) {
      case 0: synthesize = p => Math.sin(p * Math.PI * 2); break;
      case 1: synthesize = p => ((p < 0.5) ? 1 : -1); break;
      case 2: synthesize = p => p * 2 - 1; break;
      case 3: synthesize = p => ((p < 0.5) ? (p * 4 - 1) : (3 - p * 4)); break;
      default: throw new Error(`Unknown wave name ${nameOrVoices}`);
    } else {
      synthesize = p => {
        let sample = 0;
        for (const voice of nameOrVoices) {
          let p0 = (p * voice[1]) % 1
          sample += Math.sin(p0 * Math.PI * 2) * voice[2];
        }
        return sample;
      };
    }
    
    for (
      let i=0, modp=0, carp=0, ratePointsp=0, dp=0, ddp=0, rangePointsp=0, range=0, drange=0;
      i<v.length;
      i++, dp+=ddp, range+=drange
    ) {
      if (ratePointsp < ratePoints.length) {
        if (i >= ratePoints[ratePointsp][0]) {
          dp = ratePoints[ratePointsp][1];
          ratePointsp++;
          if (ratePointsp >= ratePoints.length) {
            ddp = 0;
          } else {
            ddp = (ratePoints[ratePointsp][1] - ratePoints[ratePointsp-1][1]) / Math.max(1, (ratePoints[ratePointsp][0] - ratePoints[ratePointsp-1][0]));
          }
        }
      }
      if (rangePointsp < rangePoints.length) {
        if (i >= rangePoints[rangePointsp][0]) {
          range = rangePoints[rangePointsp][1];
          rangePointsp++;
          if (rangePointsp >= rangePoints.length) {
            drange = 0;
          } else {
            drange = (rangePoints[rangePointsp][1] - rangePoints[rangePointsp-1][1]) / Math.max(1, (rangePoints[rangePointsp][0] - rangePoints[rangePointsp-1][0]));
          }
        }
      }
      modp += dp * modrate;
      modp %= 1;
      const mod = Math.sin(modp * Math.PI * 2);
      carp += dp + mod * range * dp;
      carp %= 1;
      if (carp < 0) carp += 1;
      v[i] = synthesize(carp);
    }
  }
  
  _cmd_gain(bufferId, src, srcp) {
    const v = this.buffers[bufferId];
    const mlt = src[srcp] + src[srcp + 1] / 256.0;
    const clip = src[srcp + 2] / 256.0;
    const gate = src[srcp + 3] / 256.0;
    const nclip = -clip;
    const ngate = -gate;
    for (let i=v.length; i-->0; ) {
      v[i] *= mlt;
           if (v[i] > clip) v[i] = clip;
      else if (v[i] < nclip) v[i] = nclip;
      else if (v[i] >= gate) ;
      else if (v[i] <= ngate) ;
      else v[i] = 0;
    }
    return 4;
  }
  
  _cmd_env(bufferId, src, srcp) {
    const srcp0 = srcp;
    const points = [];
    srcp += this._readEnv(points, src, srcp, true);
    const v = this.buffers[bufferId];
    
    // Combine points into ramps, it's easier to visualize that way.
    // Each ramp's startTime and startLevel are the same as its predecessor's endTime and endLevel.
    // The set of ramps will cover the signal completely (possibly further).
    const ramps = []; // [startTime, endTime, startLevel, endLevel]
    for (let i=0; i<points.length; i++) {
      const next = ((i+1)<points.length) ? points[i+1] : [v.length, points[i][1]];
      ramps.push([points[i][0], next[0], points[i][1], next[1]]);
    }
    
    for (const ramp of ramps) {
      if (ramp[0] >= ramp[1]) continue;
      const d = (ramp[3] - ramp[2]) / (ramp[1] - ramp[0]);
      let end = ramp[1];
      if (end > v.length) end = v.length;
      let level=ramp[2];
      for (let vp=ramp[0]; vp<end; vp++, level+=d) {
        v[vp] *= level;
      }
    }
      
    return srcp - srcp0;
  }
  
  _cmd_mix(bufferId, src, srcp) {
    const srcBufferId = src[srcp];
    if (srcBufferId >= this.buffers.length) throw new Buffer(`Buffer ${srcBufferId} not defined`);
    const to = this.buffers[bufferId];
    const from = this.buffers[srcBufferId];
    for (let i=to.length; i-->0; ) to[i] += from[i];
    return 1;
  }
  
  _cmd_norm(bufferId, src, srcp) {
    const peak = src[srcp] / 0xff;
    const v = this.buffers[bufferId];
    let lo=0, hi=0;
    for (let i=v.length; i-->0; ) {
      if (v[i] < lo) lo = v[i];
      else if (v[i] > hi) hi = v[i];
    }
    lo = -lo;
    if (lo > hi) hi = lo;
    if (!hi) return 1; // Silent input is obviously noop, but would also be a divide-by-zero if we try.
    const scale = peak / hi;
    for (let i=v.length; i-->0; ) {
      v[i] *= scale;
    }
    return 1;
  }
  
  _cmd_delay(bufferId, src, srcp) {
    const v = this.buffers[bufferId];
    const interval = Math.max(1, (((src[srcp] << 8) | src[srcp + 1]) * this.rate) / 1000);
    const dry = src[srcp + 2] / 0xff;
    const wet = src[srcp + 3] / 0xff;
    const store = src[srcp + 4] / 0xff;
    const feedback = src[srcp + 5] / 0xff;
    const ring = new Float32Array(interval);
    
    for (let i=0, ringp=0; i<v.length; i++, ringp++) {
      if (ringp >= ring.length) ringp = 0;
      
      const inSample = v[i];
      const pvSample = ring[ringp];
      
      v[i] = inSample * dry + pvSample * wet;
      ring[ringp] = inSample * store + pvSample * feedback;
    }
    
    return 6;
  }
  
  _cmd_bandpass(bufferId, src, srcp) {
    const v = this.buffers[bufferId];
    const midfreqHz = (src[srcp] << 8) | src[srcp + 1];
    const rangeHz = (src[srcp + 2] << 8) | src[srcp + 3];
    
    /* 3-point IIR bandpass.
     * I have only a vague idea of how this works, and the formula is taken entirely on faith.
     * Reference:
     *   Steven W Smith: The Scientist and Engineer's Guide to Digital Signal Processing
     *   Ch 19, p 326, Equation 19-7
     */
    const midfreqNorm = (midfreqHz / this.rate);
    const rangeNorm = (rangeHz / this.rate);
    const r = 1 - 3 * rangeNorm;
    const cosfreq = Math.cos(Math.PI * 2 * midfreqNorm);
    const k = (1 - 2 * r * cosfreq + r * r) / (2 - 2 * cosfreq);
    const dryCoefs = [1 - k, 2 * (k - r) * cosfreq, r * r - k];
    const wetCoefs = [2 * r * cosfreq, -r * r];
    const dry = [0, 0, 0];
    const wet = [0, 0];
    
    let lo=0, hi=0;
    for (let i=0; i<v.length; i++) {
      dry[2] = dry[1];
      dry[1] = dry[0];
      dry[0] = v[i];
      const ws = 
        dry[0] * dryCoefs[0] +
        dry[1] * dryCoefs[1] +
        dry[2] * dryCoefs[2] +
        wet[0] * wetCoefs[0] +
        wet[1] * wetCoefs[1];
      wet[1] = wet[0];
      wet[0] = ws;
      v[i] = ws;
      
      if (ws < lo) lo=ws; else if (ws>hi) hi=ws;
    }
    
    return 4;
  }
  
  // Populates (points) with [[time(frames), v(0..1 or 0..0xffff)]...], and returns src length consumed.
  _readEnv(points, src, srcp, normalizeValues) {
    const srcp0 = srcp;
    const timescale = this.rate / 1000;
    points.push([0, ((src[srcp] << 8) | src[srcp + 1]) / (normalizeValues ? 0xffff : 1)]);
    const count = src[srcp + 2];
    srcp += 3;
    const rdv = normalizeValues
      ? (() => (((src[srcp + 2] << 8) | src[srcp + 3]) / 0xffff))
      : (() => ((src[srcp + 2] << 8) | src[srcp + 3]));
    for (let i=count, time=0; i-->0; srcp+=4) {
      const reltime = ~~(((src[srcp] << 8) | src[srcp + 1]) * timescale);
      const level = rdv();
      time += reltime;
      points.push([time, level]);
    }
    return srcp - srcp0;
  }
  
  _readRateHz(src, srcp) {
    return (src[srcp] << 8) | src[srcp + 1];
  }
  
  _readRateNormFromHz(src, srcp) {
    const hz = (src[srcp] << 8) | src[srcp + 1];
    return hz / this.rate;
  }
  
  _readRateRadiansFromHz(src, srcp) {
    const hz = (src[srcp] << 8) | src[srcp + 1];
    const norm = hz / this.rate;
    return norm * Math.PI * 2;
  }
}
