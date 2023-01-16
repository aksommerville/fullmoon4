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
  if (words.length < 1) return null;
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

module.exports = decodeCommand;
