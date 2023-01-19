const fs = require("fs");
//const decodeCommand = require("./decodeCommand.js");

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

function imageIdFromPath(path) {
  const imageId = +path.replace(/^.*[\/\\]/, "");
  if (isNaN(imageId) || (imageId < 1) || (imageId > 0xff)) return 0;
  return ~~imageId;
}

function decodeCell(src) {
  const v = parseInt(src, 16);
  if (isNaN(v)) throw new Error(`Illegal cell value '${src}'`);
  return v;
}

function compileTileprops(path) {
  const physics = Buffer.alloc(256);
  let physicsc = 0;
  let reading = false;
  const src = fs.readFileSync(path);
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    try {
      let nlp = src.indexOf(0x0a, srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.toString("utf-8", srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line || line.startsWith("#")) continue;
      
      if (line === "physics") {
        reading = true;
        continue;
      }
      if (line.length !== 32) {
        reading = false;
        continue;
      }
      if (!reading) continue;
    
      for (let linep=0; linep<line.length; linep+=2) {
        physics[physicsc++] = decodeCell(line.substring(linep, linep + 2));
      }
    } catch (e) {
      e.message = `${path}:${lineno}: ${e.message}`;
      throw e;
    }
  }
  if (physicsc !== 256) throw new Error(`${path}: Found ${physicsc} cells, expected 256`);
  return physics;
}

const tablesToAppend = [];
const toc = Buffer.alloc(256);
let dstp = 1;
for (const srcpath of srcpaths) {
  const imageId = imageIdFromPath(srcpath);
  if (!imageId) throw new Error(`Failed to guess image ID from path '${srcpath}'`);
  if (toc[imageId]) throw new Error(`Multiple inputs for image ID ${imageId}`);
  tablesToAppend.push(compileTileprops(srcpath));
  toc[imageId] = dstp++;
}

const dst = Buffer.alloc(256 + 256 * tablesToAppend.length);
toc.copy(dst, 0);
for (let dstp=256, i=0; i<tablesToAppend.length; i++, dstp+=256) {
  tablesToAppend[i].copy(dst, dstp);
}

fs.writeFileSync(dstpath, dst);
