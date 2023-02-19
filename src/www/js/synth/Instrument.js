/* Instrument.js
 * Constant configuration for one MIDI program.
 */
 
import { Envelope } from "./Envelope.js";
 
export class Instrument {
  constructor(src, pid) {

    // Our fields are named just like keys in the config file, not a coincidence.
    // Values will vary slightly.
    this.pid = pid;
    this.wave = "sine"; // string | float[]; immutable either way
    this.env = new Envelope();
    this.modAbsoluteRate = 0;
    this.modRate = 0;
    this.modRange = 0; // nonzero to select FM mode.
    this.modEnv = null; // Envelope | null
    this.modRangeLfoRate = 0;
    this.wheelRange = 200;
    this.bpq = 0; // nonzero to select Bandpass mode.
    this.bpq2 = 0;
    this.bpBoost = 1;

    if (!src) ;
    else if (src instanceof Uint8Array) this._decode(src);
    else if (src instanceof ArrayBuffer) this._decode(new Uint8Array(src));
    else if (src instanceof Instrument) this._copy(src);
    else throw new Error(`Unsuitable input for Instrument`);
  }
  
  _copy(src) {
    // Don't copy (pid); we got a unique one given separately to the ctor.
    this.wave = src.wave;
    this.env = new Envelope(src.env);
    this.modAbsoluteRate = src.modAbsoluteRate;
    this.modRate = src.modRate;
    this.modRange = src.modRange;
    if (src.modEnv) this.modEnv = new Envelope(src.modEnv);
    this.modRangeLfoRate = src.modRangeLfoRate;
    this.wheelRange = src.wheelRange;
  }
  
  _decode(src) {
    for (let srcp=0; srcp<src.length; ) {
      const opcode = src[srcp++];
      switch (opcode) {
        case 0x00: return; // EOF
        case 0x01: srcp += this._decodeWave(src, srcp); break;
        case 0x02: srcp += this._decodeEnv("env", src, srcp); break;
        case 0x03: this.modAbsoluteRate = this._decodeScalar(src, srcp, 16, 8); srcp += 3; break;
        case 0x04: this.modRate = this._decodeScalar(src, srcp, 8, 8); srcp += 2; break;
        case 0x05: this.modRange = this._decodeScalar(src, srcp, 8, 8); srcp += 2; break;
        case 0x06: srcp += this._decodeEnv("modEnv", src, srcp); break;
        case 0x07: this.modRangeLfoRate = this._decodeScalar(src, srcp, 16, 8); srcp += 3; break;
        case 0x08: this.wheelRange = this._decodeScalar(src, srcp, 16, 0); srcp += 2; break;
        case 0x09: this.bpq = this._decodeScalar(src, srcp, 8, 8); srcp += 2; break;
        case 0x0a: this.bpq2 = this._decodeScalar(src, srcp, 8, 8); srcp += 2; break;
        case 0x0b: this.bpBoost = this._decodeScalar(src, srcp, 16, 0); srcp += 2; break;
        default: throw new Error(`Unexpected instrument opcode ${opcode} at ${srcp-1}/${src.length} in pid ${this.pid}`);
      }
    }
  }
  
  _decodeWave(src, srcp) {
    if (src[srcp] & 0x80) {
      switch (src[srcp]) {
        case 0x80: this.wave = "sine"; break;
        case 0x81: this.wave = "square"; break;
        case 0x82: this.wave = "sawtooth"; break;
        case 0x83: this.wave = "triangle"; break;
        default: {
            console.log(`Unknown enumerated wave shape ${src[srcp]}. Will use "sine"`);
            this.wave = "sine";
          }
      }
      return 1;
    }
    const srcp0 = srcp;
    const coefc = src[srcp++];
    const coefv = [0]; // WebAudio expects a DC coefficient, we won't supply one.
    for (let i=coefc; i-->0; srcp+=2) {
      const vi = (src[srcp] << 8) | src[srcp + 1];
      coefv.push(vi / 0xffff);
    }
    this.wave = coefv;
    return srcp - srcp0;
  }
  
  _decodeEnv(k, src, srcp) {
    const srcc = Envelope.measure(src[srcp]);
    const srcView = new Uint8Array(src.buffer, src.byteOffset + srcp, srcc);
    this[k] = new Envelope(srcView);
    return srcc;
  }
  
  _decodeScalar(src, srcp, wholeSize, fractSize) {
    const totalSizeBits = wholeSize + fractSize;
    const totalSizeBytes = totalSizeBits >> 3;
    let vi; switch (totalSizeBytes) {
      case 1: vi = src[srcp]; break;
      case 2: vi = (src[srcp] << 8) | src[srcp+1]; break;
      case 3: vi = (src[srcp] << 16) | (src[srcp+1] << 8) | src[srcp+2]; break;
      case 4: vi = (src[srcp] << 24) | (src[srcp+1] << 16) | (src[srcp+2] << 8) | src[srcp+3]; break;
      default: throw new Error(`Illegal size ${wholeSize}.${fractSize} for Instrument._decodeScalar`);
    }
    if (!fractSize) return vi;
    return vi / ((1 << fractSize) - 1);
  }
}
