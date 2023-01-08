const fs = require("fs");

const COLC = 20;
const ROWC = 12;

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

function mapIdFromPath(path) {
  const mapId = +path.replace(/^.*[\/\\]/, "");
  if (isNaN(mapId) || (mapId < 1) || (mapId > 0xffff)) return 0;
  return ~~mapId;
}

function decodeCell(src) {
  switch (src) {
    case ". ": return 0x00;
    case "Xx": return 0x3d;
    //TODO more aliases, user-defined aliases, neighbor joining
  }
  const v = parseInt(src, 16);
  if (isNaN(v)) throw new Error(`Illegal cell value '${src}'`);
  return v;
}

function compileMap(path) {
  const src = fs.readFileSync(path);
  const lineLength = COLC * 2 + 1;
  const minimumLength = lineLength * ROWC;
  if (src.length < minimumLength) {
    throw new Error(`${path}: Length ${src.length} below minimum ((${COLC}*2+1)*${ROWC}==${minimumLength})`);
  }
  const cells = Buffer.alloc(COLC * ROWC);
  let cellp = 0, linep = 0;
  for (let row=0; row<ROWC; row++, linep+=lineLength) {
    try {
      for (let col=0, srcp=linep; col<COLC; col++, cellp++, srcp+=2) {
        cells[cellp] = decodeCell(src.toString("utf-8", srcp, srcp+2));
      }
    } catch (e) {
      throw new Error(`${path}:${row+1}: ${e.message}`);
    }
  }
  
  const serial = cells;
  //TODO further content. spawn points etc...
  
  return {
    path,
    cells,
    serial,
  };
}

const maps = [null]; // indexed by mapId and can be sparse (will always be sparse; zero is not used)
for (const srcpath of srcpaths) {
  const mapId = mapIdFromPath(srcpath);
  if (!mapId) throw new Error(`Failed to guess map ID from path '${srcpath}'`);
  if (maps[mapId]) throw new Error(`Multiple inputs for map ID ${mapId}: ${maps[mapId].srcpath} and ${srcpath}`);
  maps[mapId] = compileMap(srcpath);
}

let totalLength = 6; // Signature and Last Map ID.
let dataStart = 0;
totalLength += 4 * (maps.length - 1);
dataStart = totalLength;
for (const map of maps) {
  if (!map) continue;
  totalLength += map.serial.length;
}

const dst = Buffer.alloc(totalLength);

// Signature.
dst[0] = 0xff;
dst[1] = 0x00;
dst[2] = 0x4d; // M
dst[3] = 0x50; // P

// Last Map ID.
const lastMapId = maps.length - 1;
dst[4] = lastMapId >> 8;
dst[5] = lastMapId;

// TOC and data.
let tocp = 6, datap = dataStart;
for (let mapp=1; mapp<maps.length; mapp++) {
  const map = maps[mapp];

  dst[tocp++] = datap >> 24;
  dst[tocp++] = datap >> 16;
  dst[tocp++] = datap >> 8;
  dst[tocp++] = datap;
  
  if (map) {
    map.serial.copy(dst, datap);
    datap += map.serial.length;
  }
}
if (tocp !== dataStart) throw new Error(`Expected TOC to run to ${dataStart} but it ended at ${tocp}.`);
if (datap !== dst.length) throw new Error(`Expected data to run to ${dst.length} but it ended at ${datap}.`);

fs.writeFileSync(dstpath, dst);
