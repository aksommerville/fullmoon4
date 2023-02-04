const Encoder = require("../common/Encoder.js");

/* "[[BankMSB] BankLSB] PID"
 * <0 on errors.
 */
function parsePid(arg) {
  const words = arg.split(/\s+/);
  if ((words.length < 1) || (words.length > 3)) return -1;
  const vv = words.map(s => +s);
  if (vv.find(n => (isNaN(n) || (n < 0)))) return -1;
  if (vv.length > 1) {
    if (vv.find(n => n > 0x7f)) return -1;
  } else {
    if (vv[0] > 0x1fffff) return -1;
  }
  let fqpid = 0;
  for (const v of vv) {
    fqpid <<= 7;
    fqpid |= v;
  }
  return fqpid;
}

/* wave SHAPE | COEFFICIENTS
 */
 
function encodeInstrument_WebAudio_wave(dst, src) {
  dst.u8(0x01);
  switch (src) {
    case "sine": dst.u8(0x80); return;
    case "square": dst.u8(0x81); return;
    case "sawtooth": dst.u8(0x82); return;
    case "triangle": dst.u8(0x83); return;
  }
  let coefc = 0;
  const coefp = dst.c;
  dst.u8(0); // placeholder coefficient count
  for (const word of src.split(/\s+/)) {
    const v = parseInt(word, 16);
    if (isNaN(v) || (word.length !== 4)) {
      throw new Error(`Expected 4-digit hexadecimal number, found ${JSON.stringify(word)}`);
    }
    dst.u16be(v);
    coefc++;
  }
  if (!coefc) throw new Error(`Must supply at least one coefficient, or a name: sine square sawtooth triangle`);
  if (coefc > 0x7f) throw new Error(`Too many coefficients. ${coefc}, limit 127`);
  dst.v[coefp] = coefc;
}

/* env ...complicated...
 */
 
function encodeInstrument_WebAudio_env(dst, src, opcode) {
  const words = src.split(/\s+/);
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
  const writeLevel = (flags & 0x20) ? (v => dst.u16be(v)) : (v => dst.u8(v));
  const writeTime = (flags & 0x40) ? (v => dst.u16be(v)) : (v => dst.u8(v));
  dst.u8(opcode);
  dst.u8(flags);
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

/* WebAudio trivial fields.
 */
 
function encodeInstrument_WebAudio_uscalar(dst, src, opcode, wholeSize, fractSize) {
  const outputSize = wholeSize + fractSize;
  const vf = parseFloat(src);
  if (isNaN(vf) || (vf < 0) || (vf > 2 ** wholeSize)) {
    throw new Error(`Expected float in 0..${2 ** wholeSize}, found ${JSON.stringify(src)}`);
  }
  const limit = (outputSize === 32) ? 0xffffffff : ((1 << outputSize) - 1);
  const vi = Math.min(limit, Math.round(vf * (1 << fractSize)));
  dst.u8(opcode);
  switch (outputSize) {
    case 8: dst.u8(vi); break;
    case 16: dst.u16be(vi); break;
    case 24: dst.u24be(vi); break;
    case 32: dst.u32be(vi); break;
    default: throw new Error(`Inappropriate fixed-point size ${wholeSize}.${fractSize}`);
  }
}

/* WebAudio synth keys.
 */
 
function encodeInstrument_WebAudio(ins) {
  // First check for conflicting fields eg (modAbsoluteRate,modRate)
  const keys = Object.keys(ins);
  if (keys.includes("modAbsoluteRate") && keys.includes("modRate")) {
    // We could let it pass; modAbsoluteRate will win. But better to be clear about it.
    throw new Error(`modAbsoluteRate and modRate are mutually exclusive`);
  }
  if (keys.find(k => k.startsWith("mod"))) {
    const range = +ins.modRange;
    if (!range) throw new Error(`All "mod" fields are meaningless without a nonzero "modRange"`);
  }
  
  const dst = new Encoder();
  for (const key of keys) {
    const src = ins[key];
    switch (key) {
      case "wave": encodeInstrument_WebAudio_wave(dst, src); break;
      case "env": encodeInstrument_WebAudio_env(dst, src, 0x02); break;
      case "modAbsoluteRate": encodeInstrument_WebAudio_uscalar(dst, src, 0x03, 16, 8); break;
      case "modRate": encodeInstrument_WebAudio_uscalar(dst, src, 0x04, 8, 8); break;
      case "modRange": encodeInstrument_WebAudio_uscalar(dst, src, 0x05, 8, 8); break;
      case "modEnv": encodeInstrument_WebAudio_env(dst, src, 0x06); break;
      case "modRangeLfoRate": encodeInstrument_WebAudio_uscalar(dst, src, 0x07, 16, 8); break;
      case "wheelRange": encodeInstrument_WebAudio_uscalar(dst, src, 0x08, 16, 0); break;
    }
  }
  return dst.finish();
}

/* (ins) is an object containing its block. First word of each line is a key, remaining words are the value.
 */
function encodeInstrument(ins, qualifier) {
  switch (qualifier) {
    case "WebAudio": return encodeInstrument_WebAudio(ins);
  }
  throw new Error(`Unknown qualifier ${JSON.stringify(qualifier)}`);
}

/* cb(id, serial)
 */
function readInstruments(src, path, qualifier, cb) {
  let pid = null;
  let openLine = 0;
  let ins = null;
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    try {
      let nlp = src.indexOf(0x0a, srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.toString("utf-8", srcp, nlp).split('#')[0].trim();
      srcp = nlp + 1;
      if (!line) continue;
    
      let spacep = line.indexOf(" ");
      if (spacep < 0) spacep = line.length;
      const kw = line.substring(0, spacep);
      const arg = line.substring(spacep).trim();
    
      if (pid === null) {
        if (kw !== "begin") throw new Error(`Expected 'begin PID'`);
        if ((pid = parsePid(arg)) < 0) throw new Error(`Failed to parse PID ${JSON.stringify(arg)}`);
        openLine = lineno;
        ins = {};
      } else {
        if (kw === "end") {
          if (arg) throw new Error(`Unexpected tokens after 'end'`);
          cb(pid, encodeInstrument(ins, qualifier));
          pid = null;
          ins = null;
        } else {
          if (ins.hasOwnProperty(kw)) throw new Error(`Duplicate key ${JSON.stringify(kw)}`);
          ins[kw] = arg;
        }
      }
    } catch (e) {
      // gaaa after all this trouble, lineno is not so useful because we do the real processing at the end of the block.
      // Could record line numbers per field as we go, but that's just more hassle. Whatever.
      e.message = `${path}:${lineno}: ${e.message}`;
      throw e;
    }
  }
  if (pid !== null) {
    throw new Error(`${path}:${openline}: Unclosed instrument block`);
  }
}

module.exports = readInstruments;
