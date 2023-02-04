/* Envelope.js
 * Model for velocity-sensitive envelopes.
 * These are not general in time, there are specific "attack", "decay", "sustain", "release" stages.
 */
 
export class Envelope {
  constructor() {
    // (lo,hi) are [startLevel, attackTime, attackLevel, decayTime, sustainLevel, releaseTime, endLevel].
    // Levels in 0..1, and times in seconds.
    this.lo = [0, 0.040, 0.100, 0.030, 0.050, 0.100, 0];
    this.hi = [0, 0.010, 0.200, 0.020, 0.080, 0.200, 0];
  }
  
  apply(v) {
    if (v <= 0) return {
      startLevel:   this.lo[0],
      attackTime:   this.lo[1],
      attackLevel:  this.lo[2],
      decayTime:    this.lo[3],
      sustainLevel: this.lo[4],
      releaseTime:  this.lo[5],
      endLevel:     this.lo[6],
    };
    if (v >= 1) return {
      startLevel:   this.hi[0],
      attackTime:   this.hi[1],
      attackLevel:  this.hi[2],
      decayTime:    this.hi[3],
      sustainLevel: this.hi[4],
      releaseTime:  this.hi[5],
      endLevel:     this.hi[6],
    };
    const loWeight = 1 - v;
    return {
      startLevel:   this.lo[0] * loWeight + this.hi[0] * v,
      attackTime:   this.lo[1] * loWeight + this.hi[1] * v,
      attackLevel:  this.lo[2] * loWeight + this.hi[2] * v,
      decayTime:    this.lo[3] * loWeight + this.hi[3] * v,
      sustainLevel: this.lo[4] * loWeight + this.hi[4] * v,
      releaseTime:  this.lo[5] * loWeight + this.hi[5] * v,
      endLevel:     this.lo[6] * loWeight + this.hi[6] * v,
    };
  }
}
