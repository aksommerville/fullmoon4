/* Envelope.js
 * Model for velocity-sensitive envelopes.
 * These are not general in time, there are specific "attack", "decay", "sustain", "release" stages.
 */
 
export class Envelope {
  constructor(src) {
    // (lo,hi) are [startLevel, attackTime, attackLevel, decayTime, sustainLevel, releaseTime, endLevel].
    // Levels in 0..1, and times in seconds.
    this.lo = null; // float[7]
    this.hi = null; // float[7] | null
    if (!src) this._init();
    else if (src instanceof Uint8Array) this._decode(src);
    else if (src instanceof ArrayBuffer) this._decode(new Uint8Array(src));
    else if (src instanceof Envelope) this._copy(src);
    else throw new Error(`Unsuitable input for Envelope`);
  }
  
  apply(v) {
    if ((v <= 0) || !this.hi) return {
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
  
  // Length of encoded Envelope in bytes, including this leadingByte.
  static measure(leadingByte) {
    const velocity = leadingByte & 0x80;
    const timeSize = (leadingByte & 0x40) ? 2 : 1;
    const levelSize = (leadingByte & 0x20) ? 2 : 1;
    const edgeLevels = leadingByte & 0x10;
    const blockSize = 3 * timeSize + 2 * levelSize + (edgeLevels ? (2 * levelSize) : 0);
    return 1 + blockSize * (velocity ? 2 : 1);
  }
  
  _init() {
    this.lo = [0, 0.040, 0.100, 0.030, 0.050, 0.100, 0];
    this.hi = null;
    //this.hi = [0, 0.010, 0.200, 0.020, 0.080, 0.200, 0];
  }
  
  _copy(src) {
    this.lo = [...src.lo];
    this.hi = src.hi ? [...src.hi] : null;
  }
  
  _decode(src) {
    const velocity = src[0] & 0x80;
    const hiResTime = src[0] & 0x40;
    const hiResLevel = src[0] & 0x20;
    const edgeLevels = src[0] & 0x10;
    let srcp = 1;
    const readLevel = hiResLevel ? () => {
      srcp += 2;
      return ((src[srcp-2] << 8) | src[srcp-1]) / 0xffff;
    } : () => {
      return src[srcp++] / 0xff;
    };
    const readTime = hiResTime ? () => {
      srcp += 2;
      return ((src[srcp-2] << 8) | src[srcp-1]) / 1000;
    } : () => {
      return src[srcp++] / 1000;
    };
    this.lo = [0, 0, 0, 0, 0, 0, 0];
    if (edgeLevels) this.lo[0] = readLevel();
    this.lo[1] = readTime();
    this.lo[2] = readLevel();
    this.lo[3] = readTime();
    this.lo[4] = readLevel();
    this.lo[5] = readTime();
    if (edgeLevels) this.lo[6] = readLevel();
    if (velocity) {
      this.hi = [0, 0, 0, 0, 0, 0, 0];
      if (edgeLevels) this.hi[0] = readLevel();
      this.hi[1] = readTime();
      this.hi[2] = readLevel();
      this.hi[3] = readTime();
      this.hi[4] = readLevel();
      this.hi[5] = readTime();
      if (edgeLevels) this.hi[6] = readLevel();
    } else {
      this.hi = null;
    }
  }
}
