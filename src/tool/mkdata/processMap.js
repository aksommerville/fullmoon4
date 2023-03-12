const fs = require("fs").promises;
const getResourceIdByName = require("../common/getResourceIdByName.js");
const resolveGsbitName = require("../common/resolveGsbitName.js");

const COLC = 20;
const ROWC = 12;

/* Return an array of integer arguments after validating args.
 * (schema) are [name, lo, hi, resType], one per expected argument.
 */
 
function assertIntArgs(args, ...schema) {
  if (args.length !== schema.length) {
    throw new Error(`Expected ${schema.length} arguments, found ${args.length}`);
  }
  const dst = [];
  for (let i=0; i<args.length; i++) {
    let v;
    if (args[i].toString().startsWith("gs:")) args[i] = resolveGsbitName(args[i].substring(3));
    try {
      const resType = schema[i][3];
      v = getResourceIdByName(resType, args[i]);
      if ((v < schema[i][1]) || (v > schema[i][2])) {
        throw new Error(`Value '${args[i]}' must be integer in ${schema[i][1]}..${schema[i][2]}`);
      }
    } catch (e) {
      e.message = `Failed to evaluate '${schema[i][0]}': ${e.message}`;
      throw e;
    }
    dst.push(v);
  }
  return dst;
}

/* Generic single-argument commands.
 */
 
function decodeCommand_noarg(opcode, args) {
  if (args.length) throw new Error(`Expected no arguments`);
  const dst = Buffer.alloc(1);
  dst[0] = opcode;
  return dst;
}
 
function decodeCommand_singleU8(opcode, args, resType) {
  if (args.length !== 1) throw new Error(`Expected single argument in 0..255`);
  const v = getResourceIdByName(resType, args[0]);
  if ((v < 0) || (v > 0xff)) throw new Error(`Expected single argument in 0..255, found ${v}`);
  const dst = Buffer.alloc(2);
  dst[0] = opcode;
  dst[1] = v;
  return dst;
}

function decodeCommand_singleU16(opcode, args, resType) {
  if (args.length !== 1) throw new Error(`Expected single argument in 0..65535`);
  const v = getResourceIdByName(resType, args[0]);
  if ((v < 0) || (v > 0xffff)) throw new Error(`Expected single argument in 0..65535, found ${v}`);
  const dst = Buffer.alloc(3);
  dst[0] = opcode;
  dst[1] = v >> 8;
  dst[2] = v;
  return dst;
}

function decodeCommand_xy(opcode, args) {
  if (args.length !== 2) throw new Error(`Expected X Y`);
  const x = +args[0];
  const y = +args[1];
  if (isNaN(x) || isNaN(y) || (x < 0) || (x >= COLC) || (y < 0) || (y >= ROWC)) {
    throw new Error(`Map coordinates invalid or out of range: ${JSON.stringify(args)}`);
  }
  const dst = Buffer.alloc(2);
  dst[0] = opcode;
  dst[1] = y * COLC + x;
  return dst;
}

/* door X Y MAPID DSTX DSTY
 */
 
function decodeCommand_door(args) {
  const [x, y, mapId, dstx, dsty] = assertIntArgs(args,
    ["x", 0, COLC - 1],
    ["y", 0, ROWC - 1],
    ["mapId", 1, 0xffff, "map"],
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
    ["spriteId", 1, 0xffff, "sprite"],
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

/* transmogrify X Y MODE STATE
 */
 
function decodeCommand_transmogrify(args) {
  const originalMode = args[2];
  switch (args[2]) {
    case "to": args[2] = 0x80; break;
    case "from": args[2] = 0x40; break;
    case "toggle": args[2] = 0xc0; break;
  }
  const [x, y, mode, state] = assertIntArgs(args,
    ["x", 0, COLC - 1],
    ["y", 0, ROWC - 1],
    ["mode", 0x40, 0xc0],
    ["state", 0, 0x3f],
  );
  if (mode & 0x3f) throw new Error(`Invalid transmogify mode ${JSON.stringify(originalMode)}, must be "to", "from", or "toggle"`);
  const dst = Buffer.alloc(3);
  dst[0] = 0x44;
  dst[1] = y * COLC + x;
  dst[2] = mode | state;
  return dst;
}

/* wind DIR
 */
 
function decodeCommand_wind(args) {
  switch (args[0]) {
    case "N": args[0] = 0x40; break;
    case "W": args[0] = 0x10; break;
    case "E": args[0] = 0x08; break;
    case "S": args[0] = 0x02; break;
  }
  const [dir] = assertIntArgs(args,
    ["dir", 0, 255],
  );
  if ((dir !== 0x40) && (dir !== 0x10) && (dir !== 0x08) && (dir !== 0x02)) {
    throw new Error(`Invalid wind direction ${JSON.stringify(args[0])}, must be one of: N W E S`);
  }
  const dst = Buffer.alloc(2);
  dst[0] = 0x23;
  dst[1] = dir;
  return dst;
}

/* sketch X Y BITS
 */
 
function decodeCommand_sketch(args) {
  const [x, y, bits] = assertIntArgs(args,
    ["x", 0, COLC - 1],
    ["y", 0, ROWC - 1],
    ["bits", 1, 0x000fffff],
  );
  const dst = Buffer.alloc(5);
  dst[0] = 0x61;
  dst[1] = y * COLC + x;
  dst[2] = bits >> 16;
  dst[3] = bits >> 8;
  dst[4] = bits;
  return dst;
}

/* Main entry point, command TOC.
 */

function decodeCommand(words) {
  if (words.length < 1) return [];
  switch (words[0]) {
    case "song": return decodeCommand_singleU8(0x20, words.slice(1), "song");
    case "tilesheet": return decodeCommand_singleU8(0x21, words.slice(1), "image");
    case "neighborw": return decodeCommand_singleU16(0x40, words.slice(1), "map");
    case "neighbore": return decodeCommand_singleU16(0x41, words.slice(1), "map");
    case "neighborn": return decodeCommand_singleU16(0x42, words.slice(1), "map");
    case "neighbors": return decodeCommand_singleU16(0x43, words.slice(1), "map");
    case "door": return decodeCommand_door(words.slice(1));
    case "sprite": return decodeCommand_sprite(words.slice(1));
    case "dark": return decodeCommand_noarg(0x01, words.slice(1));
    case "hero": return decodeCommand_xy(0x22, words.slice(1));
    case "transmogrify": return decodeCommand_transmogrify(words.slice(1));
    case "indoors": return decodeCommand_noarg(0x02, words.slice(1));
    case "wind": return decodeCommand_wind(words.slice(1));
    case "sketch": return decodeCommand_sketch(words.slice(1));
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
      //throw new Error(`${path}:${lineno}: ${e.message}`);
      e.mesage = `${path}:${lineno}: ${e.message}`;
      throw e;
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
