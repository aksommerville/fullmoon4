const Encoder = require("../common/Encoder.js");
const linewise = require("../common/linewise.js");
const getSoundEffectIdByName = require("../common/getSoundEffectIdByName.js");
const { MinsynInstrument, MinsynSound } = require("./minsyn.js");
const { StdsynInstrument, StdsynSound } = require("./stdsyn.js");

/* WebAudio instrument builder.
 **************************************************************/
 
class WebAudioInstrument {
  constructor(id, path, lineno) {
    this.id = id;
    this.path = path;
    this.lineno = lineno;
    this.type = "instrument";
    this.encoder = new Encoder();
    this.keys = [];
  }
  
  encode() {
    return this.encoder.finish();
  }
  
  _wave(words) {
    this.encoder.u8(0x01);
    if (words.length === 1) switch (words[0]) {
      case "sine": this.encoder.u8(0x80); return;
      case "square": this.encoder.u8(0x81); return;
      case "sawtooth": this.encoder.u8(0x82); return;
      case "triangle": this.encoder.u8(0x83); return;
    }
    let coefc = 0;
    const coefp = this.encoder.c;
    this.encoder.u8(0); // placeholder coefficient count
    for (const word of words) {
      const v = parseInt(word, 16);
      if (isNaN(v) || (word.length !== 4)) {
        throw new Error(`Expected 4-digit hexadecimal number, found ${JSON.stringify(word)}`);
      }
      this.encoder.u16be(v);
      coefc++;
    }
    if (!coefc) throw new Error(`Must supply at least one coefficient, or a name: sine square sawtooth triangle`);
    if (coefc > 0x7f) throw new Error(`Too many coefficients. ${coefc}, limit 127`);
    this.encoder.v[coefp] = coefc;
  }
  
  _env(words, opcode) {
    // Word count can only be (5,7,11,15), and that count tells us 0x80=Velocity and 0x10=EdgeLevels.
    let flags = 0;
    switch (words.length) {
      case 5: break;
      case 7: flags |= 0x10; break;
      case 11: flags |= 0x80; break;
      case 15: flags |= 0x90; break;
      default: throw new Error(`Bad envelope token count ${words.length}, expected 5, 7, 11, or 15`);
    }
  
    // Parse input tokens into a neat pair of integer arrays.
    const lo = [0, 0, 0, 0, 0, 0, 0];
    const hi = [0, 0, 0, 0, 0, 0, 0];
    let wordp = 0;
    const readLevel = () => {
      if (flags & 0x20) {
        if (words[wordp].length !== 4) throw new Error(`Level tokens must all be the same size (found ${words[wordp].length}, expected 4)`);
      } else {
        if (words[wordp].length !== 2) throw new Error(`Level tokens must all be the same size (found ${words[wordp].length}, expected 2)`);
      }
      const v = parseInt(words[wordp++], 16);
      if (isNaN(v)) throw new Error(`Failed to parse level ${JSON.stringify(words[wordp-1])}`);
      return v;
    };
    const readTime = () => {
      const v = +words[wordp++];
      if (isNaN(v) || (v < 0) || (v > 0xffff)) throw new Error(`Failed to parse time ${JSON.stringify(words[wordp-1])}`);
      if (v > 0xff) flags |= 0x40;
      return v;
    }
    if (flags & 0x10) lo[0] = readLevel();
    lo[1] = readTime();
    lo[2] = readLevel();
    lo[3] = readTime();
    lo[4] = readLevel();
    lo[5] = readTime();
    if (flags & 0x10) lo[6] = readLevel();
    if (flags & 0x80) {
      if (words[wordp] !== "..") throw new Error(`Expected separator '..', found ${JSON.stringify(words[wordp])}`);
      wordp++;
      if (flags & 0x10) hi[0] = readLevel();
      hi[1] = readTime();
      hi[2] = readLevel();
      hi[3] = readTime();
      hi[4] = readLevel();
      hi[5] = readTime();
      if (flags & 0x10) hi[6] = readLevel();
    }
  
    // Pack output.
    // Isn't this pretty? Sometimes I really love Javascript.
    const writeLevel = (flags & 0x20) ? (v => this.encoder.u16be(v)) : (v => this.encoder.u8(v));
    const writeTime = (flags & 0x40) ? (v => this.encoder.u16be(v)) : (v => this.encoder.u8(v));
    this.encoder.u8(opcode);
    this.encoder.u8(flags);
    if (flags & 0x10) writeLevel(lo[0]);
    writeTime(lo[1]);
    writeLevel(lo[2]);
    writeTime(lo[3]);
    writeLevel(lo[4]);
    writeTime(lo[5]);
    if (flags & 0x10) writeLevel(lo[6]);
    if (flags & 0x80) {
      if (flags & 0x10) writeLevel(hi[0]);
      writeTime(hi[1]);
      writeLevel(hi[2]);
      writeTime(hi[3]);
      writeLevel(hi[4]);
      writeTime(hi[5]);
      if (flags & 0x10) writeLevel(hi[6]);
    }
  }
  
  _uscalar(words, opcode, wholeSize, fractSize) {
    if (words.length !== 1) throw new Error(`Expected scalar, found ${JSON.stringify(words.join(' '))}`);
    const outputSize = wholeSize + fractSize;
    const vf = parseFloat(words[0]);
    if (isNaN(vf) || (vf < 0) || (vf > 2 ** wholeSize)) {
      throw new Error(`Expected float in 0..${2 ** wholeSize}, found ${JSON.stringify(words[0])}`);
    }
    const limit = (outputSize === 32) ? 0xffffffff : ((1 << outputSize) - 1);
    const vi = Math.min(limit, Math.round(vf * (1 << fractSize)));
    this.encoder.u8(opcode);
    switch (outputSize) {
      case 8: this.encoder.u8(vi); break;
      case 16: this.encoder.u16be(vi); break;
      case 24: this.encoder.u24be(vi); break;
      case 32: this.encoder.u32be(vi); break;
      default: throw new Error(`Inappropriate fixed-point size ${wholeSize}.${fractSize}`);
    }
  }
  
  receiveLine(words, path, lineno) {
    try {
      if (this.keys.includes(words[0])) throw new Error(`Redefinition of field ${JSON.stringify(words[0])}`);
      switch (words[0]) {
        
        case "wave": this._wave(words.slice(1)); break;
        case "env": this._env(words.slice(1), 0x02); break;
        case "modAbsoluteRate": {
            if (this.keys.includes("modRate")) throw new Error(`modAbsoluteRate and modRate are mutually exclusive`);
            this._uscalar(words.slice(1), 0x03, 16, 8);
          } break;
        case "modRate": {
            if (this.keys.includes("modAbsoluteRate")) throw new Error(`modAbsoluteRate and modRate are mutually exclusive`);
            this._uscalar(words.slice(1), 0x04, 8, 8);
          } break;
        case "modRange": this._uscalar(words.slice(1), 0x05, 8, 8); break;
        case "modEnv": this._env(words.slice(1), 0x06); break;
        case "modRangeLfoRate": this._uscalar(words.slice(1), 0x07, 16, 8); break;
        case "wheelRange": this._uscalar(words.slice(1), 0x08, 16, 0); break;
        case "bpq": this._uscalar(words.slice(1), 0x09, 8, 8); break;
        case "bpq2": this._uscalar(words.slice(1), 0x0a, 8, 8); break;
        case "bpBoost": this._uscalar(words.slice(1), 0x0b, 16, 0); break;
        
        default: throw new Error(`Unknown key ${JSON.stringify(words[0])} for WebAudio instrument`);
      }
      this.keys.push(words[0]);
    } catch (e) {
      e.message = `${path}:${lineno}: ${e.message}`;
      throw e;
    }
  }
}

/* WebAudio sound builder.
 **************************************************************/
 
class WebAudioSound {
  constructor(id, path, lineno) {
    this.id = id;
    this.path = path;
    this.lineno = lineno;
    this.type = "sound";
    this.encoder = new Encoder();
    this.duration = null;
    this.buffers = new Set(); // every buffer ID that's been written to, 0..254
  }
  
  encode() {
    if (!this.duration) throw new Error(`${this.path}:${this.lineno}: Empty sound. Must contain at least "len"`);
    if (!this.buffers.has(0)) throw new Error(`${this.path}:${this.lineno}: Sound must write to buffer zero`);
    this.encoder.v[1] = this._countBuffers();
    return this.encoder.finish();
  }
  
  _countBuffers() {
    if (this.buffers.size < 1) return 0;
    return Math.max(...this.buffers) + 1;
  }
  
  _evalShape(src) {
    switch (src) {
      case "sine": return 0;
      case "square": return 1;
      case "sawtooth": return 2;
      case "triangle": return 3;
    }
    return -1;
  }
  
  // (args) is [value,(time,value...)]
  // Output: u16 value, u8 count, then count * (u16 timems,u16 value)
  _env(args) {
    this.encoder.u16be(args[0]);
    const cp = this.encoder.c;
    this.encoder.u8(0); // placeholder for count
    let c = 0;
    let elapsed = 0;
    for (let i=1; i<args.length; i+=2, c++) {
      let time = +args[i];
      let value = +args[i+1];
      if (isNaN(time)) {
        if (args[i] === '*') {
          // '*' as a time means "until the end of the sound".
          // It's only sensible as the last leg, but we don't actually require that.
          time = this.duration * 1000 - elapsed;
        } else {
          throw new Error(`Expected time in ms, found ${JSON.stringify(args[i])}`);
        }
      }
      if (isNaN(value)) {
        throw new Error(`Expected envelope value in 0..65535, found ${JSON.stringify(args[i+1])}`);
      }
      this.encoder.u16be(Math.max(0, Math.min(0xffff, time)));
      this.encoder.u16be(Math.max(0, Math.min(0xffff, value)));
      elapsed += time;
    }
    if (c > 0xff) throw new Error(`Too many points in envelope (${c}, limit 255)`);
    this.encoder.v[cp] = c;
  }
  
  // (args) are 16-bit integer, hexadecimal with no prefix.
  // Output: u8 coefc, u16 coefv...
  _harmonics(args) {
    const coefv = args.map(a => {
      const v = parseInt(a, 16);
      if (isNaN(v) || (v < 0) || (v > 0xffff)) throw new Error(`Expected hexadecimal integer in 0000..ffff, found ${JSON.stringify(a)}`);
      return v;
    });
    if (coefv.length > 0xff) {
      throw new Error(`Too many harmonics (${coefv.length}, limit 255)`);
    }
    this.encoder.u8(coefv.length);
    for (const coef of coefv) this.encoder.u16be(coef);
  }

  _cmd_noise(bufferId, args) {
    if (args.length) throw new Error(`"noise" takes no arguments`);
    this.encoder.u8(0x01);
    this.encoder.u8(bufferId);
  }
  
  _cmd_wave(bufferId, args) {
    // In:
    //   RATE SHAPE
    //   ( ENV ) SHAPE
    // Out:
    //   0x02 WAVE_FIXED_NAME (u8 buf,u16 rate,u8 name)
    //   0x03 WAVE_FIXED_HARM (u8 buf,u16 rate,u8 coefc,u16... coefv)
    //   0x04 WAVE_ENV_NAME (u8 buf,ENV,u8 name)
    //   0x05 WAVE_ENV_HARM (u8 buf,ENV,u8 coefc,u16... coefv)
    if (args.length < 2) throw new Error(`Too few arguments to "wave"`);
    let rate=0, envp=0, envc=0, shapep=0;
    if (args[0] === "(") {
      const closep = args.indexOf(")");
      if (closep < 0) throw new Error(`")" not found`);
      envp = 1;
      envc = closep - 1;
      shapep = closep + 1;
    } else {
      rate = +args[0];
      if (isNaN(rate) || (rate <= 0) || (rate > 0xffff)) {
        throw new Error(`Expected rate in 1..65535 or "( ENV )", found ${JSON.stringify(args[0])}`);
      }
      shapep = 1;
    }
    if (shapep >= args.length) {
      throw new Error(`Expected shape`);
    }
    let shapeId = -1;
    if (shapep === args.length - 1) shapeId = this._evalShape(args[shapep]);
    if (rate) {
      if (shapeId >= 0) { // fixed rate, named wave
        this.encoder.u8(0x02);
        this.encoder.u8(bufferId);
        this.encoder.u16be(rate);
        this.encoder.u8(shapeId);
      } else { // fixed rate, harmonics
        this.encoder.u8(0x03);
        this.encoder.u8(bufferId);
        this.encoder.u16be(rate);
        this._harmonics(args.slice(shapep));
      }
    } else if (shapeId >= 0) { // env rate, named wave
      this.encoder.u8(0x04);
      this.encoder.u8(bufferId);
      this._env(args.slice(1, 1 + envc));
      this.encoder.u8(shapeId);
    } else { // env rate, harmonics
      this.encoder.u8(0x05);
      this.encoder.u8(bufferId);
      this._env(args.slice(1, 1 + envc));
      this._harmonics(args.slice(shapep));
    }
  }
  
  _cmd_fm(bufferId, args) {
    // In:
    //   fm BUFFER RATE MODRATE ( RANGEENV ) SHAPE
    //   fm BUFFER ( ENV ) MODRATE ( RANGEENV ) SHAPE
    // Out:
    //   0x06 FM_FIXED_NAME (u8 buf,u16 rate,u8.8 modrate,ENV range,u8 name)
    //   0x07 FM_FIXED_HARM (u8 buf,u16 rate,u8.8 modrate,ENV range,u8 coefc,u16... coefv)
    //   0x08 FM_ENV_NAME (u8 buf,ENV rate,u8.8 modrate,ENV range,u8 name)
    //   0x09 FM_ENV_HARM (u8 buf,ENV rate,u8.8 modrate,ENV range,u8 coefc,u16... coefv)
    const opcodep = this.encoder.c;
    this.encoder.u8(0); // placeholder for opcode
    this.encoder.u8(bufferId);
    let argp = 0;
    
    // RATE or ( ENV )
    let useRateEnv = false;
    if (args[argp] === '(') {
      const closep = args.indexOf(')', argp);
      if (closep < 0) throw new Error(`Unclosed envelope`);
      this._env(args.slice(argp + 1, closep));
      argp = closep + 1;
      useRateEnv = true;
    } else {
      const rate = +args[argp];
      if (isNaN(rate) || (rate < 0) || (rate > 0xffff)) {
        throw new Error(`Expected RATE in 0..65535, found ${JSON.stringify(args[argp])}`);
      }
      this.encoder.u16be(rate);
      argp++;
    }
    
    // MODRATE
    const modrate = +args[argp];
    if (isNaN(modrate) || (modrate <= 0) || (modrate >= 256)) {
      throw new Error(`Expected MODRATE in 0..256 exclusive, found ${JSON.stringify(args[argp])}`);
    }
    this.encoder.u16be(modrate * 256); // u8.8
    argp++;
    
    // RANGEENV
    if (args[argp] !== "(") throw new Error(`Expected ( RANGEENV ), found ${JSON.stringify(args[argp])}`);
    argp++;
    const closep = args.indexOf(")", argp);
    if (closep < 0) throw new Error(`Unclosed envelope`);
    this._env(args.slice(argp, closep));
    argp = closep + 1;
    
    // SHAPE
    let useHarmonics = false;
    const shapeId = this._evalShape(args[argp]);
    if (shapeId >= 0) {
      this.encoder.u8(shapeId);
    } else {
      useHarmonics = true;
      this._harmonics(args.slice(argp));
    }
    
    // Select opcode.
    if (useRateEnv) {
      if (useHarmonics) this.encoder.v[opcodep] = 0x09;
      else this.encoder.v[opcodep] = 0x08;
    } else if (useHarmonics) this.encoder.v[opcodep] = 0x07;
    else this.encoder.v[opcodep] = 0x06;
  }
  
  _cmd_gain(bufferId, args) {
    // gain BUFFER MLT [CLIP [GATE]]
    // 0x0a GAIN (u8 buf,u8.8 mlt,u0.8 clip, u0.8 gate)
    if (args.length < 1) throw new Error(`Too few arguments to "gain"`);
    if (args.length > 3) throw new Error(`Too many arguments to "gain"`);
    let mlt = +args[0];
    let clip = +args[1];
    if (isNaN(clip)) clip = 1;
    let gate = +args[2] || 0;
    this.encoder.u8(0x0a);
    this.encoder.u8(bufferId);
    this.encoder.u16be(Math.max(0, Math.min(0xffff, ~~(mlt * 256)))); // u8.8
    this.encoder.u8(Math.max(0, Math.min(0xff, ~~(clip * 256))));
    this.encoder.u8(Math.max(0, Math.min(0xff, ~~(gate * 256))));
  }
  
  _cmd_env(bufferId, args) {
    // env BUFFER ( LEVEL TIME LEVEL TIME ... LEVEL )
    // 0x0b ENV (u8 buf,ENV)
    if ((args.length < 2) || (args[0] !== '(') || (args[args.length - 1] !== ')')) {
      throw new Error(`Expected ( ENV )`);
    }
    this.encoder.u8(0x0b);
    this.encoder.u8(bufferId);
    this._env(args.slice(1, args.length - 1));
  }
  
  _cmd_mix(bufferId, args) {
    // mix DSTBUFFER SRCBUFFER
    // 0x0c MIX (u8 dst,u8 src)
    if (args.length !== 1) throw new Error(`"mix" takes one argument (bufferId)`);
    const srcBufferId = ~~+args[0];
    if ((srcBufferId < 0) || (srcBufferId > 0xff)) {
      throw new Error(`Expected bufferId, found ${JSON.stringify(args[0])}`);
    }
    if (!this.buffers.has(srcBufferId)) {
      throw new Error(`Buffer ${srcBufferId} has not been written yet`);
    }
    if (bufferId === srcBufferId) {
      throw new Error(`Mixing buffer ${bufferId} on to itself is weird. Use "gain ${bufferId} 2" if you really meant to`);
    }
    this.encoder.u8(0x0c);
    this.encoder.u8(bufferId);
    this.encoder.u8(srcBufferId);
  }
  
  _cmd_norm(bufferId, args) {
    // norm BUFFER [PEAK]
    // 0x0d norm (u8 buf,u0.8 peak)
    if (args.length > 1) throw new Error(`Too many arguments to "norm"`);
    let peak = 1;
    if (args.length >= 1) {
      peak = +args[0];
      if (isNaN(peak) || (peak < 0) || (peak > 1)) {
        throw new Error(`Expected PEAK in 0..1, found ${JSON.stringify(args[0])}`);
      }
    }
    this.encoder.u8(0x0d);
    this.encoder.u8(bufferId);
    this.encoder.u8(Math.max(0, Math.min(0xff, ~~(peak * 256))));
  }
  
  _cmd_delay(bufferId, args) {
    // delay BUFFER DURATION DRY WET STORE FEEDBACK
    // 0x0e delay (u8 buf,u16 durationMs,u0.8 dry,u0.8 wet,u0.8,u0.8 feedback)
    if (args.length !== 5) throw new Error(`"delay" takes five arguments: INTERVAL DRY WET STORE FEEDBACK`);
    const interval = +args[0];
    const dry = +args[1];
    const wet = +args[2];
    const store = +args[3];
    const feedback = +args[4];
    if (isNaN(interval) || isNaN(dry) || isNaN(wet) || isNaN(store) || isNaN(feedback)) {
      throw new Error(`Invalid argument, expected seconds, then four of 0..1`);
    }
    this.encoder.u8(0x0e);
    this.encoder.u8(bufferId);
    this.encoder.u16be(Math.max(0, Math.min(0xffff, ~~(interval * 1000))));
    this.encoder.u8(Math.max(0, Math.min(0xff, ~~(dry * 256))));
    this.encoder.u8(Math.max(0, Math.min(0xff, ~~(wet * 256))));
    this.encoder.u8(Math.max(0, Math.min(0xff, ~~(store * 256))));
    this.encoder.u8(Math.max(0, Math.min(0xff, ~~(feedback * 256))));
  }
  
  _cmd_bandpass(bufferId, args) {
    // bandpass BUFFER MIDFREQ RANGE
    // 0x0f bandpass (u8 buf,u16 midfreq,u16 range)
    if (args.length !== 2) throw new Error(`"bandpass" takes two arguments: MIDFREQ RANGE, both in Hz`);
    const midfreq = +args[0];
    const range = +args[1];
    this.encoder.u8(0x0f);
    this.encoder.u8(bufferId);
    this.encoder.u16be(Math.max(0, Math.min(0xffff, ~~midfreq)));
    this.encoder.u16be(Math.max(0, Math.min(0xffff, ~~range)));
  }
  
  // True if (kw) is read/write and is not sensible to operate on a new buffer.
  // There are commands eg "mix" that might be used just to copy a buffer, when you know the output is fresh.
  // So only ones where we know it's an error to use on a fresh buffer.
  isReadRequiredCommand(kw) {
    switch (kw) {
      case "gain":
      case "env":
      case "norm":
      case "delay":
        return true;
    }
    return false;
  }
  
  receiveLine(words, path, lineno) {
    try {
    
      // First line must be "len".
      if (!this.duration) {
        if (words[0] !== "len") throw new Error(`First instruction in a WebAudio sound must be "len SECONDS"`);
        if (words.length !== 2) throw new Error(`"len" takes one argument`);
        this.duration = +words[1];
        if (isNaN(this.duration) || (this.duration <= 0) || (this.duration >= 4)) {
          throw new Error(`Length must be in 0..4 (exclusive), found ${JSON.stringify(words[1])}`);
        }
        this.encoder.u8(~~(this.duration * 64)); // u2.6
        this.encoder.u8(0); // placeholder for buffer count
        
      // All other lines can be in any order, and are "COMMAND BUFFER [ARGS...]"
      } else {
        if (words.length < 2) throw new Error(`Expected buffer ID`);
        const kw = words[0];
        const bufferId = +words[1];
        if (isNaN(bufferId) || (bufferId < 0) || (bufferId >= 0xff)) {
          throw new Error(`Invalid buffer id ${JSON.stringify(words[1])}, expected integer in 0..254`);
        }
        if (this.isReadRequiredCommand(kw)) {
          if (!this.buffers.has(bufferId)) {
            throw new Error(`Command ${JSON.stringify(kw)} is read/write but buffer ${bufferId} has not been written yet`);
          }
        }
        this.buffers.add(bufferId);
        const args = words.slice(2);
        switch (kw) {

          case "noise": this._cmd_noise(bufferId, args); break;
          case "wave": this._cmd_wave(bufferId, args); break;
          case "fm": this._cmd_fm(bufferId, args); break;
          case "gain": this._cmd_gain(bufferId, args); break;
          case "env": this._cmd_env(bufferId, args); break;
          case "mix": this._cmd_mix(bufferId, args); break;
          case "norm": this._cmd_norm(bufferId, args); break;
          case "delay": this._cmd_delay(bufferId, args); break;
          case "bandpass": this._cmd_bandpass(bufferId, args); break;

          default: throw new Error(`Unknown WebAudio sound command ${JSON.stringify(kw)}`);
        }
      }
    } catch (e) {
      e.message = `${path}:${lineno}: ${e.message}`;
      throw e;
    }
  }
}

/* Object factory.
 * Creates a builder for instrument or sound, for the given synthesizer (qualifier).
 * All returned objects must have these members:
 *   number id,
 *   string type,
 *   Buffer encode(),
 *   number lineno,
 *   void receiveLine(words, path, lineno).
 *****************************************************/
 
function newInstrument(qualifier, args, path, lineno) {
  let id = 0;
  for (let i=args.length, shift=0; i-->0; shift+=7) {
    const v = +args[i];
    if (isNaN(v)) throw new Error(`${path}:${lineno}: Expected integer, found ${JSON.stringify(args[i])}`);
    id |= v << shift;
  }
  if ((id < 1) || (id > 0x1fffff)) throw new Error(`${path}:${lineno}: Invalid instrument ID`);
  switch (qualifier) {
  
    case "WebAudio": return new WebAudioInstrument(id, path, lineno);
    case "minsyn": return new MinsynInstrument(id, path, lineno);
    case "stdsyn": return new StdsynInstrument(id, path, lineno);
    
  }
  throw new Error(`Unknown instrument qualifier ${JSON.stringify(qualifier)}`);
}

function newSound(qualifier, args, path, lineno) {
  if (args.length !== 1) throw new Error(`${path}:${lineno}: Expected integer, found ${JSON.stringify(args.join(' '))}`);
  let id;
  try {
    id = getSoundEffectIdByName(args[0]);
  } catch (e) {
    e.message = `${path}:${lineno}: ${e.message}`;
    throw e;
  }
  switch (qualifier) {
  
    case "WebAudio": return new WebAudioSound(id, path, lineno);
    case "minsyn": return new MinsynSound(id, path, lineno);
    case "stdsyn": return new StdsynSound(id, path, lineno);
    
  }
  throw new Error(`Unknown sound qualifier ${JSON.stringify(qualifier)}`);
}

/* Main entry point.
 *************************************************/

function readInstruments(src, path, qualifier, cb) {
  let obj = null;
  linewise(src, (line, lineno) => {
    const words = line.split(/\s+/);
    if (words[0] === "end") {
      if (!obj) throw new Error(`${path}:${lineno}: "end" outside any block`);
      cb(obj.id, obj.encode(), obj.type);
      obj = null;
    } else if (obj) {
      obj.receiveLine(words, path, lineno);
    } else if (words[0] === "begin") {
      obj = newInstrument(qualifier, words.slice(1), path, lineno);
    } else if (words[0] === "sound") {
      obj = newSound(qualifier, words.slice(1), path, lineno);
    } else {
      throw new Error(`${path}:${lineno}: Expected "begin ID" or "sound ID"`);
    }
  });
  if (obj) {
    throw new Error(`${path}:${obj.lineno}: Unclosed object, expected "end"`);
  }
}

module.exports = readInstruments;
