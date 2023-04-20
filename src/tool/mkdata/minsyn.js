// Instrument features.
const FEATURE_HARMONICS = 0x01;
const FEATURE_LOW_ENV = 0x02;
const FEATURE_HIGH_ENV = 0x04;
const FEATURE_MIXWAVE_HARMONICS = 0x08;
const FEATURE_LOW_MIXENV = 0x10;
const FEATURE_HIGH_MIXENV = 0x20;

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
      mixwave: null,
      mixenv: null,
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
    if (this.features & FEATURE_MIXWAVE_HARMONICS) dstp = this._encodeHarmonics(dst, dstp, this.commands.mixwave);
    if (this.features & FEATURE_LOW_MIXENV) dstp = this._encodeEnvelope(dst, dstp, this.commands.mixenv.slice(0, 5));
    if (this.features & FEATURE_HIGH_MIXENV) {
      if ((this.commands.mixenv.length !== 11) || (this.commands.mixenv[5] !== "..")) {
        throw new Error(`${this.path}:${this.lineno}: Malformed 'mixenv' command. Must be 5 or 11 tokens`);
      }
      dstp = this._encodeEnvelope(dst, dstp, this.commands.mixenv.slice(6));
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
    if (this.commands.mixwave) {
      this.features |= FEATURE_MIXWAVE_HARMONICS;
      dstc += 1 + this.commands.mixwave.length;
    }
    if (this.commands.mixenv) {
      this.features |= FEATURE_LOW_MIXENV;
      dstc += 5;
      if (this.commands.mixenv.length > 5) {
        this.features |= FEATURE_HIGH_MIXENV;
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
    this.lenms = 0;
    
    // We encode on the fly.
    this.dst = Buffer.alloc(1024);
    this.dstc = 0;
  }
  
  encode() {
    const dst = Buffer.alloc(this.dstc);
    this.dst.copy(dst, 0, 0, this.dstc);
    return dst;
  }
  
  _u0_8(params) {
    if (params.length < 1) throw new Error(`Expected u0.8 before end of line`);
    let v = +params[0];
    if (isNaN(v)) throw new Error(`Expected u0.8, found ${JSON.stringify(params[0])}`);
    v = ~~(v * 256);
    if (v <= 0) this.dst[this.dstc++] = 0x00;
    else if (v > 0xff) this.dst[this.dstc++] = 0xff;
    else this.dst[this.dstc++] = v;
    params.splice(0, 1);
  }
  _u4_4(params) {
    if (params.length < 1) throw new Error(`Expected u4.4 before end of line`);
    let v = +params[0];
    if (isNaN(v)) throw new Error(`Expected u4.4, found ${JSON.stringify(params[0])}`);
    v = ~~(v * 16);
    if (v <= 0) this.dst[this.dstc++] = 0x00;
    else if (v > 0xff) this.dst[this.dstc++] = 0xff;
    else this.dst[this.dstc++] = v;
    params.splice(0, 1);
  }
  _u8_8(params) {
    if (params.length < 1) throw new Error(`Expected u8.8 before end of line`);
    let v = +params[0];
    if (isNaN(v)) throw new Error(`Expected u8.8, found ${JSON.stringify(params[0])}`);
    v = ~~(v * 256);
    if (v <= 0) { this.dst[this.dstc++] = 0x00; this.dst[this.dstc++] = 0x00; }
    else if (v > 0xffff) { this.dst[this.dstc++] = 0xff; this.dst[this.dstc++] = 0xff; }
    else { this.dst[this.dstc++] = v >> 8; this.dst[this.dstc++] = v; }
    params.splice(0, 1);
  }
  _u16(params) {
    if (params.length < 1) throw new Error(`Expected u16 before end of line`);
    let v = +params[0];
    if (isNaN(v)) throw new Error(`Expected u16, found ${JSON.stringify(params[0])}`);
    v = ~~v;
    if (v <= 0) { this.dst[this.dstc++] = 0x00; this.dst[this.dstc++] = 0x00; }
    else if (v > 0xffff) { this.dst[this.dstc++] = 0xff; this.dst[this.dstc++] = 0xff; }
    else { this.dst[this.dstc++] = v >> 8; this.dst[this.dstc++] = v; }
    params.splice(0, 1);
  }
  
  _env(params) {
    const closep = params.indexOf(")");
    if ((params[0] !== "(") || (closep < 0)) throw new Error(`Expected parenthesized envelope.`);
    if (closep === 1) throw new Error(`Envelope must contain at least one value.`);
    if (closep & 1) throw new Error(`Envelope must have an odd member count ( LEVEL [TIME LEVEL...] )`);
    
    let v = +params[1];
    if (isNaN(v)) throw new Error(`Expected integer in 0..65535, found ${JSON.stringify(params[1])}`);
    if (v < 0) { this.dst[this.dstc++] = 0x00; this.dst[this.dstc++] = 0x00; }
    else if (v > 0xffff) { this.dst[this.dstc++] = 0xff; this.dst[this.dstc++] = 0xff; }
    else { this.dst[this.dstc++] = v >> 8; this.dst[this.dstc++] = v; }
    
    const countp = this.dstc++;
    let paramsp = 2;
    let wildcardp = -1; // in (this.dst)
    let totalms = 0;
    let count = 0;
    while (paramsp < closep) {
    
      if (params[paramsp] === "*") {
        if (wildcardp >= 0) throw new Error(`Envelope contains more than one wildcard.`);
        wildcardp = this.dstc;
        this.dstc += 2;
      } else {
        v = +params[paramsp];
        if (isNaN(v)) throw new Error(`Expected integer in 0..65535, found ${JSON.stringify(params[paramsp])}`);
        if (v < 0) { this.dst[this.dstc++] = 0x00; this.dst[this.dstc++] = 0x00; }
        else if (v > 0xffff) { this.dst[this.dstc++] = 0xff; this.dst[this.dstc++] = 0xff; }
        else { this.dst[this.dstc++] = v >> 8; this.dst[this.dstc++] = v; }
        totalms += v;
      }
      paramsp++;
    
      v = +params[paramsp];
      if (isNaN(v)) throw new Error(`Expected integer in 0..65535, found ${JSON.stringify(params[paramsp])}`);
      if (v < 0) { this.dst[this.dstc++] = 0x00; this.dst[this.dstc++] = 0x00; }
      else if (v > 0xffff) { this.dst[this.dstc++] = 0xff; this.dst[this.dstc++] = 0xff; }
      else { this.dst[this.dstc++] = v >> 8; this.dst[this.dstc++] = v; }
      paramsp++;
      
      count++;
    }
    
    if (count > 0xff) throw new Error(`Too many entries in envelope, limit 255. (how the heck...)`);
    this.dst[countp] = count;
    
    if (totalms > this.lenms) {
      throw new Error(`Envelope length ${totalms} in sound length ${this.lenms}`);
    }
    if (wildcardp >= 0) {
      const fill = this.lenms - totalms;
      this.dst[wildcardp] = fill >> 8;
      this.dst[wildcardp + 1] = fill;
    }
    
    params.splice(0, closep + 1);
  }
  
  _shape(params) {
    if (params.length < 1) throw new Error(`Expected wave shape before end of line`);
    switch (params[0]) {
      case "sine":     this.dst[this.dstc++] = 200; params.splice(0, 1); return;
      case "square":   this.dst[this.dstc++] = 201; params.splice(0, 1); return;
      case "sawtooth": this.dst[this.dstc++] = 202; params.splice(0, 1); return;
      case "triangle": this.dst[this.dstc++] = 203; params.splice(0, 1); return;
    }
    if (params.length > 0xff) throw new Error(`Too many coefficients, limit 255.`);
    this.dst[this.dstc++] = params.length;
    for (const param of params) {
      const v = +param;
      if (isNaN(v)) throw new Error(`Expected integer in 0..255, found ${JSON.stringify(param)}`);
      if (v < 0) this.dst[this.dstc++] = 0x00;
      else if (v > 0xff) this.dst[this.dstc++] = 0xff;
      else this.dst[this.dstc++] = v;
    }
    params.splice(0, params.length);
  }
  
  _cmd_fm(params) {
    //fm ( RATE_ENV ) MODRATE ( RANGE_ENV ) SHAPE
    //0x01 FM (...rate_env,u4.4 modrate,...range_env,...shape)
    this.dst[this.dstc++] = 0x01;
    this._env(params);
    this._u4_4(params);
    this._env(params);
    this._shape(params);
    if (params.length) throw new Error(`Unexpected extra tokens`);
  }
  
  _cmd_wave(params) {
    //wave ( RATE_ENV ) SHAPE
    //0x02 WAVE (...rate_env,...shape)
    this.dst[this.dstc++] = 0x02;
    this._env(params);
    this._shape(params);
    if (params.length) throw new Error(`Unexpected extra tokens`);
  }
  
  _cmd_noise(params) {
    //noise
    //0x03 NOISE ()
    this.dst[this.dstc++] = 0x03;
    if (params.length) throw new Error(`Unexpected extra tokens`);
  }
  
  _cmd_env(params) {
    //env ( ENV )
    //0x04 ENV (...env)
    this.dst[this.dstc++] = 0x04;
    this._env(params);
    if (params.length) throw new Error(`Unexpected extra tokens`);
  }
  
  _cmd_mlt(params) {
    //mlt SCALAR
    //0x05 MLT (u8.8)
    this.dst[this.dstc++] = 0x05;
    this._u8_8(params);
    if (params.length) throw new Error(`Unexpected extra tokens`);
  }
  
  _cmd_delay(params) {
    //delay PERIOD_SECONDS DRY WET STORE FEEDBACK
    //0x06 DELAY (u0.8 period,u0.8 dry,u0.8 wet,u0.8 store,u0.8 feedback)
    this.dst[this.dstc++] = 0x06;
    this._u0_8(params);
    this._u0_8(params);
    this._u0_8(params);
    this._u0_8(params);
    this._u0_8(params);
    if (params.length) throw new Error(`Unexpected extra tokens`);
  }
  
  _cmd_bandpass(params) {
    //bandpass MID_HZ RANGE_HZ
    //0x07 BANDPASS (u16 mid,u16 range)
    this.dst[this.dstc++] = 0x07;
    this._u16(params);
    this._u16(params);
  }
  
  _cmd_new_channel(params) {
    //new_channel
    //0x08 NEW_CHANNEL ()
    this.dst[this.dstc++] = 0x08;
  }
  
  receiveLine(words, path, lineno) {
    try {
    
      // First command must be "len SECONDS".
      if (!this.dstc) {
        if ((words[0] !== "len") || (words.length !== 2)) throw new Error(`First command must be "len SECONDS"`);
        let seconds = +words[1];
        if (isNaN(seconds)) throw new Error(`Expected seconds in 0..2, found ${JSON.stringify(words[1])}`);
        this.lenms = ~~(seconds * 1000);
        seconds = ~~(seconds * 128);
        if (seconds <= 0) this.dst[this.dstc++] = 0x00;
        else if (seconds > 0xff) this.dst[this.dstc++] = 0xff;
        else this.dst[this.dstc++] = seconds;
        return;
      }
      
      // We could also assert that the second command be "fm", "wave", or "noise", and that those not occur elsewhere.
      // But that's not a structural requirement so whatever.
      switch (words[0]) {
        case "fm": return this._cmd_fm(words.slice(1));
        case "wave": return this._cmd_wave(words.slice(1));
        case "noise": return this._cmd_noise(words.slice(1));
        case "env": return this._cmd_env(words.slice(1));
        case "mlt": return this._cmd_mlt(words.slice(1));
        case "delay": return this._cmd_delay(words.slice(1));
        case "bandpass": return this._cmd_bandpass(words.slice(1));
        case "new_channel": return this._cmd_new_channel(words.slice(1));
      }
      
      throw new Error(`Unexpected sound command ${JSON.stringify(words[0])}`);
    } catch (e) {
      e.message = `${path}:${lineno}: ${e.message}`;
      throw e;
    }
  }
}

module.exports = { MinsynInstrument, MinsynSound };
