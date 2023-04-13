// Instrument features.
const FEATURE_HARMONICS = 0x01;
const FEATURE_LOW_ENV = 0x02;
const FEATURE_HIGH_ENV = 0x04;

/* Instrument.
 ****************************************************************/

class MinsynInstrument {
  constructor(id, path, lineno) {
    this.id = id;
    this.path = path;
    this.lineno = lineno;
    this.type = "instrument";
    this.commands = { // null or array of strings
      wave: null,
      env: null,
    };
  }
  
  encode() {
    const len = this._determineFeatures();
    const dst = Buffer.alloc(len);
    dst[0] = this.features;
    let dstp = 1;
    if (this.features & FEATURE_HARMONICS) dstp = this._encodeHarmonics(dst, dstp, this.commands.wave);
    if (this.features & FEATURE_LOW_ENV) dstp = this._encodeEnvelope(dst, dstp, this.commands.env.slice(0, 5));
    if (this.features & FEATURE_HIGH_ENV) {
      if ((this.commands.env.length !== 11) || (this.commands.env[5] !== "..")) {
        throw new Error(`${this.path}:${this.lineno}: Malformed 'env' command. Must be 5 or 11 tokens`);
      }
      dstp = this._encodeEnvelope(dst, dstp, this.commands.env.slice(6));
    }
    if (dstp !== dst.length) throw new Error(`${this.path}:${this.lineno}: Internal encoder error! Expected ${dst.length} bytes but produced ${dstp}`);
    return dst;
  }
  
  receiveLine(words, path, lineno) {
    if (this.commands[words[0]]) throw new Error(`${path}:${lineno}: Duplicate command ${JSON.stringify(words[0])}`);
    if (!this.commands.hasOwnProperty(words[0])) throw new Error(`${path}:${lineno}: Unknown command ${JSON.stringify(words[0])}`);
    this.commands[words[0]] = words.slice(1);
  }
  
  // Reset (this.features) and return the full encoded length.
  _determineFeatures() {
    this.features = 0;
    let dstc = 1;
    if (this.commands.wave) {
      this.features |= FEATURE_HARMONICS;
      dstc += 1 + this.commands.wave.length;
    }
    if (this.commands.env) {
      this.features |= FEATURE_LOW_ENV;
      dstc += 5;
      if (this.commands.env.length > 5) {
        this.features |= FEATURE_HIGH_ENV;
        dstc += 5;
      }
    }
    return dstc;
  }
  
  _encodeHarmonics(dst, dstp, src) {
    if (src.length > 255) src = src.slice(0, 255);
    dst[dstp++] = src.length;
    for (const token of src) {
      const v = +token;
      if (isNaN(v) || (v < 0) || (v > 0xff)) {
        throw new Error(`${this.path}:${this.lineno}: Invalid token ${JSON.stringify(token)} in 'wave' command, expected integer in 0..255`);
      }
      dst[dstp++] = v;
    }
    return dstp;
  }
  
  _encodeEnvelope(dst, dstp, src) {
    if (src.length !== 5) throw new Error(`${this.path}:${this.lineno}: Expected 5 integers for 'env'`);
    src = src.map(v => +v);
    src[4] = Math.floor(src[4] / 8);
    for (const v of src) {
      if (isNaN(v) || (v < 0) || (v > 0xff)) {
        throw new Error(`${this.path}:${this.lineno}: Invalid token in 'env' command.`);
      }
      dst[dstp++] = v;
    }
    return dstp;
  }
}

/* Sound.
 *************************************************************/

class MinsynSound {
  constructor(id, path, lineno) {
    this.id = id;
    this.path = path;
    this.lineno = lineno;
    this.type = "sound";
  }
  
  encode() {
    return Buffer.alloc(0);
  }
  
  receiveLine(words, path, lineno) {
  }
}

module.exports = { MinsynInstrument, MinsynSound };
