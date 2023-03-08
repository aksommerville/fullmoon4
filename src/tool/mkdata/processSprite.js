const fs = require("fs").promises;
const linewise = require("../common/linewise.js");
const getControllerIdsByName = require("../common/getControllerIdsByName.js");
const getResourceIdByName = require("../common/getResourceIdByName.js");

const BV_SIZE = 8;

/* Generic handlers.
 */
 
function decodeCommand_u8(src, opcode, resType) {
  if (src.length !== 2) throw new Error(`Expected one argument for command ${JSON.stringify(src[0])}`);
  const v = getResourceIdByName(resType, src[1]);
  if (isNaN(v) || (v < 0) || (v > 0xff)) throw new Error(`Expected integer in 0..255, found ${JSON.stringify(src[1])}`);
  return Buffer.from([opcode, v]);
}

function decodeCommand_u88(src, opcode) {
  if (src.length !== 2) throw new Error(`Expected one argument for command ${JSON.stringify(src[0])}`);
  const v = +src[1];
  if (isNaN(v) || (v < 0) || (v >= 256.0)) throw new Error(`Expected number 0 <= n < 256, found ${JSON.stringify(src[1])}`);
  const dst = Buffer.alloc(3);
  dst[0] = opcode;
  dst[1] = Math.floor(v);
  dst[2] = Math.floor((v % 1) * 256);
  return dst;
}

function decodeCommand_u16(src, opcode, resType) {
  if (src.length !== 2) throw new Error(`Expected one argument for command ${JSON.stringify(src[0])}`);
  const v = getResourceIdByName(resType, src[1]);
  if (isNaN(v) || (v < 0) || (v >= 0xffff)) throw new Error(`Expected number in 0..65535, found ${JSON.stringify(src[1])}`);
  const dst = Buffer.alloc(3);
  dst[0] = opcode;
  dst[1] = v >> 8;
  dst[2] = v;
  return dst;
}

function decodeCommand_u8k(src, opcode, firstByte, resType) {
  if (src.length !== 2) throw new Error(`Expected one argument for command ${JSON.stringify(src[0])}`);
  const v = getResourceIdByName(resType, src[1]);
  if (isNaN(v) || (v < 0) || (v > 0xff)) throw new Error(`Expected integer in 0..255, found ${JSON.stringify(src[1])}`);
  return Buffer.from([opcode, firstByte, v]);
}

/* Commands with special tokens.
 * In order to share the final output functions, these all emit a fake input array.
 */
 
function rewriteInput_xform(src) {
  // xform [xrev] [yrev] [swap]
  let xform = 0;
  for (let i=1; i<src.length; i++) {
    const token = src[i];
         if (token === "xrev") xform |= 1;
    else if (token === "yrev") xform |= 2;
    else if (token === "swap") xform |= 4;
    else {
      const v = +token;
      if (isNaN(v) || (v < 0) || (v > 0xff)) {
        throw new Error(`Expected 'xrev', 'yrev', 'swap', or integer in 0..255, found ${JSON.stringify(token)}`);
      }
      xform |= v;
    }
  }
  return ["xform", xform];
}

function rewriteInput_style(src) {
  if (src.length !== 2) throw new Error(`Expected 1 argument after 'style', found ${src.length - 1}`);
  let v = src[1];
  switch (v) {
    case "hidden": v = 1; break;
    case "tile": v = 2; break;
    case "hero": v = 3; break;
    case "fourframe": v = 4; break;
    case "firenozzle": v = 5; break;
    case "firewall": v = 6; break;
    case "doublewide": v = 7; break;
    case "pitchfork": v = 8; break;
    case "twoframe": v = 9; break;
  }
  return ["style", v];
}

function rewriteInput_physics(src) {
  // physics [motion] [edge] [sprites] [solid] [hole]
  let physics = 0;
  for (let i=1; i<src.length; i++) {
    const token = src[i];
         if (token === "motion") physics |= 0x01;
    else if (token === "edge") physics |= 0x02;
    else if (token === "sprites") physics |= 0x04;
    else if (token === "solid") physics |= 0x10;
    else if (token === "hole") physics |= 0x20;
    else if (token === "blowable") physics |= 0x40;
    else {
      const v = +token;
      if (isNaN(v) || (v < 0) || (v > 0xff)) {
        throw new Error(`Expected 'motion', 'edge', 'sprites', 'solid', 'hole', 'blowable', or integer in 0..255, found ${JSON.stringify(token)}`);
      }
      physics |= v;
    }
  }
  return ["physics", physics];
}

function rewriteInput_controller(src) {
  if (src.length !== 2) throw new Error(`Expected 1 argument after 'controller', found ${src.length - 1}`);
  const lu = getControllerIdsByName();
  let id = lu[src[1]];
  if (isNaN(id)) { // NB zero is a valid controller ID.
    id = +src[1];
    if (isNaN(id) || (id < 0) || (id > 0xffff)) throw new Error(`Not a known sprite controller: ${JSON.stringify(src[1])}`);
  }
  return ["controller", id];
}

/* (src) is an array of strings, one per token.
 * Output is null or a Buffer.
 */
function decodeCommand(src) {
  if (src.length < 1) return null;
  switch (src[0]) {
    case "image": return decodeCommand_u8(src, 0x20, "image");
    case "tile": return decodeCommand_u8(src, 0x21);
    case "xform": return decodeCommand_u8(rewriteInput_xform(src), 0x22);
    case "style": return decodeCommand_u8(rewriteInput_style(src), 0x23);
    case "physics": return decodeCommand_u8(rewriteInput_physics(src), 0x24);
    case "decay": return decodeCommand_u88(src, 0x40);
    case "radius": return decodeCommand_u88(src, 0x41);
    case "invmass": return decodeCommand_u8(src, 0x25);
    case "controller": return decodeCommand_u16(rewriteInput_controller(src), 0x42);
    case "layer": return decodeCommand_u8(src, 0x26);
    default: {
        let match;
        if (match = src[0].match(/^bv\[(\d+)\]$/)) {
          const n = +match[1];
          if (isNaN(n) || (n < 0) || (n >= BV_SIZE)) {
            throw new Error(`Invalid bv index ${JSON.stringify(match[1])}`);
          }
          return decodeCommand_u8k(src, 0x43, n);
        }
      }
  }
  // Not a known token. The input can be all integers, 1 output byte each, and if it adds up, we'll use it.
  const lead = +src[0];
  if (isNaN(lead) || (lead < 0) || (lead > 0xff)) throw new Error(`Unknown sprite command ${JSON.stringify(src[0])}`);
  let expectedLength;
       if (lead < 0x20) expectedLength = 0;
  else if (lead < 0x40) expectedLength = 1;
  else if (lead < 0x60) expectedLength = 2;
  else if (lead < 0x80) expectedLength = 3;
  else if (lead < 0xa0) expectedLength = 4;
  else if (lead < 0xd0) {
    if (src.length < 2) throw new Error(`Require length byte after opcode ${lead}`);
    const len = +src[1];
    if (isNaN(len) || (len < 0) || (len > 0xff)) throw new Error(`Invalid length byte ${JSON.stringify(src[1])}`);
    expectedLength = 1 + len;
  } else { // 0xd0..0xff anything goes
    expectedLength = src.length - 1;
  }
  if (src.length !== expectedLength + 1) {
    throw new Error(`Expected ${expectedLength} bytes payload for opcode ${lead}, but found ${src.length - 1}`);
  }
  const dst = Buffer.alloc(1 + expectedLength);
  for (let i=0; i<src.length; i++) {
    const n = +src[i];
    if (isNaN(n) || (n < 0) || (n > 0xff)) throw new Error(`Illegal byte ${JSON.stringify(src[i])}`);
    dst[i] = n;
  }
  return dst;
}

/* Main.
 */

function reencode(src, path) {
  let dst = Buffer.alloc(256);
  let dstc = 0;
  linewise(src, (line, lineno) => {
    const words = line.split(/\s+/).filter(v => v);
    const command = decodeCommand(words);
    if (!command || !command.length) return;
    if (dstc + command.length > dst.length) {
      const na = dstc + command.length + 256;
      const nv = Buffer.alloc(na);
      dst.copy(nv);
      dst = nv;
    }
    command.copy(dst, dstc);
    dstc += command.length;
  });
  if (dstc < dst.length) {
    const nv = Buffer.alloc(dstc);
    dst.copy(nv);
    dst = nv;
  }
  return dst;
}

module.exports = path => fs.readFile(path)
  .then(src => reencode(src, path));
