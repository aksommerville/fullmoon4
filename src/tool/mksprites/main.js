const fs = require("fs");
const decodeCommand = require("./decodeCommand.js");

const srcpaths = [];
let dstpath = "";
for (let argp=2; argp<process.argv.length; argp++) {
  const arg = process.argv[argp];
  
  if (arg === "--help") {
    console.log(`Usage: ${process.argv[1]} -oOUTPUT [INPUTS...]`);
    process.exit(0);
    
  } else if (arg.startsWith("-o")) {
    if (dstpath) throw new Error("Multiple output paths");
    dstpath = arg.slice(2);
    
  } else if (arg.startsWith("-")) {
    throw new Error(`Unexpected option '${arg}'`);
  
  } else {
    srcpaths.push(arg);
  }
}
if (!dstpath) {
  throw new Error("Please indicate output path with '-oPATH'");
}

function spriteIdFromPath(path) {
  const spriteId = +path.replace(/^.*[\/\\]/, "").split("-")[0];
  if (isNaN(spriteId) || (spriteId < 1) || (spriteId > 0xffff)) return 0;
  return ~~spriteId;
}

function compileSprite(path) {
  const src = fs.readFileSync(path);
  let serial = Buffer.alloc(256);
  let serialc = 0;
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    try {
      let nlp = src.indexOf(0x0a, srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.toString("utf-8", srcp, nlp).replace(/#.*$/, "").trim();
      srcp = nlp + 1;
      if (!line) continue;
      const words = line.split(/\s+/).filter(v => v);
      const intake = decodeCommand(words);
      if (!intake || !intake.length) continue;
      
      if (serialc + intake.length > serial.length) {
        const na = (serialc + intake.length + 1024) & ~1023;
        const nserial = Buffer.alloc(na);
        serial.copy(nserial, 0);
        serial = nserial;
      }
      intake.copy(serial, serialc);
      serialc += intake.length;
    } catch (e) {
      e.message = `${path}:${lineno}: ${e.message}`;
      throw e;
    }
  }
  if (serialc < serial.length) {
    const nserial = Buffer.alloc(serialc);
    serial.copy(nserial);
    serial = nserial;
  }
  return serial;
}

const spritesById = [];
for (const srcpath of srcpaths) {
  const spriteId = spriteIdFromPath(srcpath);
  if (!spriteId) throw new Error(`Failed to guess sprite ID from path '${srcpath}'`);
  if (spritesById[spriteId]) throw new Error(`Multiple inputs for sprite ID ${spriteId} ('${srcpath}' and something else)`);
  spritesById[spriteId] = compileSprite(srcpath);
}

// If there were no inputs, make up a blank "sprite 1", so I don't need to think about the edge empty case.
if (spritesById.length < 2) sprites[1] = Buffer.alloc(0);

// Take a few measurements, and allocate the final output.
const lastSpriteId = spritesById.length - 1;
const tocCount = lastSpriteId;
const tocLength = tocCount * 4;
const headerLength = 6;
const totalLength = headerLength + tocLength + spritesById.reduce((a, v) => a + (v ? v.length : 0), 0);
const dst = Buffer.alloc(totalLength);

// Header.
dst[0] = 0xff;
dst[1] = 0x4d; // M
dst[2] = 0x53; // S
dst[3] = 0x70; // p
dst[4] = lastSpriteId >> 8;
dst[5] = lastSpriteId;

// TOC.
let dstp = headerLength, datap = headerLength + tocLength;
for (let spriteId=1; spriteId<=lastSpriteId; spriteId++) {
  const sprite = spritesById[spriteId];
  dst[dstp++] = datap >> 24;
  dst[dstp++] = datap >> 16;
  dst[dstp++] = datap >> 8;
  dst[dstp++] = datap;
  if (sprite) datap += sprite.length;
}
if (dstp !== headerLength + tocLength) {
  throw new Error(`Position ${dstp} after TOC, expected ${headerLength + tocLength}, tocCount=${tocCount}`);
}

// Data.
for (const sprite of spritesById) {
  if (!sprite) continue;
  sprite.copy(dst, dstp);
  dstp += sprite.length;
}
if (dstp !== totalLength) {
  throw new Error(`Position ${dstp} after data, expected ${totalLength}, tocCount=${tocCount}`);
}

fs.writeFileSync(dstpath, dst);
