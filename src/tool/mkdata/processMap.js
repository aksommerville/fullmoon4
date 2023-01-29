const fs = require("fs").promises;

const COLC = 20;
const ROWC = 12;

/* Return an array of integer arguments after validating args.
 * (schema) are [name, lo, hi], one per expected argument.
 */
 
function assertIntArgs(args, ...schema) {
  if (args.length !== schema.length) {
    throw new Error(`Expected ${schema.length} arguments, found ${args.length}`);
  }
  const dst = [];
  for (let i=0; i<args.length; i++) {
    const v = +args[i];
    if (isNaN(v) || (v < schema[i][1]) || (v > schema[i][2])) {
      throw new Error(`Unexpected value '${args[i]}' for '${schema[i][0]}', must be integer in ${schema[i][1]}..${schema[i][2]}`);
    }
    dst.push(v);
  }
  return dst;
}

/* Generic single-argument commands.
 */
 
function decodeCommand_singleU8(opcode, args) {
  if (args.length !== 1) throw new Error(`Expected single argument in 0..255`);
  const v = +args[0];
  if (isNaN(v) || (v < 0) || (v > 255)) throw new Error(`Expected single argument in 0..255`);
  const dst = Buffer.alloc(2);
  dst[0] = opcode;
  dst[1] = v;
  return dst;
}

function decodeCommand_singleU16(opcode, args) {
  if (args.length !== 1) throw new Error(`Expected single argument in 0..65535`);
  const v = +args[0];
  if (isNaN(v) || (v < 0) || (v > 255)) throw new Error(`Expected single argument in 0..65535`);
  const dst = Buffer.alloc(3);
  dst[0] = opcode;
  dst[1] = v >> 8;
  dst[2] = v;
  return dst;
}

/* door X Y MAPID DSTX DSTY
 */
 
function decodeCommand_door(args) {
  const [x, y, mapId, dstx, dsty] = assertIntArgs(args,
    ["x", 0, COLC - 1],
    ["y", 0, ROWC - 1],
    ["mapId", 1, 0xffff],
    ["dstx", 0, COLC - 1],
    ["dsty", 0, ROWC - 1],
  );
  const dst = Buffer.alloc(5);
  dst[0] = 0x60;
  dst[1] = y * COLC + x;
  dst[2] = mapId >> 8;
  dst[3] = mapId;
  dst[4] = dsty * COLC + dstx;
  return dst;
}

/* sprite X Y SPRITEID ARG0 ARG1 ARG2
 */
 
function decodeCommand_sprite(args) {
  const [x, y, spriteId, arg0, arg1, arg2] = assertIntArgs(args,
    ["x", 0, COLC - 1],
    ["y", 0, ROWC - 1],
    ["spriteId", 1, 0xffff],
    ["arg0", 0, 0xff],
    ["arg1", 0, 0xff],
    ["arg2", 0, 0xff],
  );
  const dst = Buffer.alloc(7);
  dst[0] = 0x80;
  dst[1] = y * COLC + x;
  dst[2] = spriteId >> 8;
  dst[3] = spriteId;
  dst[4] = arg0;
  dst[5] = arg1;
  dst[6] = arg2;
  return dst;
}

/* Main entry point, command TOC.
 */

function decodeCommand(words) {
  if (words.length < 1) return [];
  switch (words[0]) {
    case "song": return decodeCommand_singleU8(0x20, words.slice(1));
    case "tilesheet": return decodeCommand_singleU8(0x21, words.slice(1));
    case "neighborw": return decodeCommand_singleU16(0x40, words.slice(1));
    case "neighbore": return decodeCommand_singleU16(0x41, words.slice(1));
    case "neighborn": return decodeCommand_singleU16(0x42, words.slice(1));
    case "neighbors": return decodeCommand_singleU16(0x43, words.slice(1));
    case "door": return decodeCommand_door(words.slice(1));
    case "sprite": return decodeCommand_sprite(words.slice(1));
  }
  return null;
}

function decodeCell(src) {
  const v = parseInt(src, 16);
  if (isNaN(v)) throw new Error(`Illegal cell value '${src}'`);
  return v;
}

function compileMap(src, path) {
  const lineLength = COLC * 2 + 1;
  const minimumLength = lineLength * ROWC;
  if (src.length < minimumLength) {
    throw new Error(`${path}: Length ${src.length} below minimum ((${COLC}*2+1)*${ROWC}==${minimumLength})`);
  }
  const cells = Buffer.alloc(COLC * ROWC);
  let cellp = 0, linep = 0;
  for (let row=0; row<ROWC; row++, linep+=lineLength) {
    try {
      if (src[linep + lineLength - 1] !== 0x0a) throw new Error("Incorrect line length.");
      for (let col=0, srcp=linep; col<COLC; col++, cellp++, srcp+=2) {
        cells[cellp] = decodeCell(src.toString("utf-8", srcp, srcp+2));
      }
    } catch (e) {
      throw new Error(`${path}:${row+1}: ${e.message}`);
    }
  }
  
  let serial = cells;
  let serialc = cells.length;
  for (let lineno=ROWC+1; linep<src.length; lineno++) {
    let nlp = src.indexOf("\n", linep);
    if (nlp < 0) nlp = src.length;
    const line = src.toString("utf8", linep, nlp).trim();
    linep = nlp + 1;
    if (!line || line.startsWith("#")) continue;
    const words = line.split(/\s+/).filter(v => v);
    try {
      const cmdSerial = decodeCommand(words);
      if (!cmdSerial) throw new Error(`${lineno}: Failed to decode command: ${line}`);
      if (!cmdSerial.length) continue;
      if (serialc + cmdSerial.length > serial.length) {
        const na = (serialc + cmdSerial.length + 1024) & ~1023;
        const nserial = Buffer.alloc(na);
        serial.copy(nserial, 0);
        serial = nserial;
      }
      cmdSerial.copy(serial, serialc);
      serialc += cmdSerial.length;
    } catch (e) {
      throw new Error(`${path}:${lineno}: ${e.message}`);
    }
  }
  if (serialc < serial.length) {
    const nserial = Buffer.alloc(serialc);
    serial.copy(nserial);
    serial = nserial;
  }
  
  return {
    path,
    cells,
    serial,
  };
}

module.exports = path => fs.readFile(path)
  .then(src => compileMap(src, path))
  .then(model => model.serial);
