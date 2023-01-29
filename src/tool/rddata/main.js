const fs = require("fs").promises;

function printUsage() {
  console.log(`Usage: ${process.argv[1]} ARCHIVE [--type=TYPE] [--id=ID] [--qualifier=QUALIFIER] [-oPATH]`);
  console.log(`No arguments, dump the TOC.`);
  console.log(`If type and ID are provided, dump that resource.`);
}

let srcpath = "";
let type = "";
let id = 0;
let qualifier = "";
let dstpath = "";
for (let argi=2; argi<process.argv.length; ) {
  const arg = process.argv[argi++];
  
  if (arg === "--help") {
    printUsage();
    process.exit(0);
    
  } else if (arg.startsWith("--type=")) {
    if (type) throw new Error(`Multiple --type`);
    type = arg.substring(7);
    
  } else if (arg.startsWith("--id=")) {
    if (id) throw new Error(`Multiple --id`);
    if (!(id = +arg.substring(5))) throw new Error(`Invalid id: ${JSON.stringify(arg.substring(5))}`);
    
  } else if (arg.startsWith("--qualifier=")) {
    if (qualifier) throw new Error(`Multiple --qualifier`);
    qualifier = arg.substring(12);
    
  } else if (arg.startsWith("-o")) {
    if (dstpath) throw new Error(`Multiple output paths`);
    dstpath = arg.substring(2);
    
  } else if (arg.startsWith("-")) {
    throw new Error(`Unexpected argument ${JSON.stringify(arg)}`);
  
  } else if (srcpath) {
    throw new Error(`Multiple input paths`);
    
  } else {
    srcpath = arg;
  }
}
if (!srcpath) {
  printUsage();
  process.exit(1);
}
if ((type && !id) || (!type && id)) {
  throw new Error(`--type and --id must both be provided, or neither`);
}

function reprType(type) {
  switch (type) {
    case 0x01: return "image";
    case 0x02: return "song";
    case 0x03: return "map";
    case 0x04: return "tileprops";
    case 0x05: return "sprite";
    case 0x06: return "string";
    case 0x07: return "instrument";
  }
  return type.toString();
}

function reprQualifier(type, q) {
  if (!q) return "";
  switch (type) {
    case "string": {
        const hi = q >> 8;
        const lo = q & 0xff;
        if ((hi >= 0x20) && (hi <= 0x7e) && (lo >= 0x20) && (lo <= 0x7e)) {
          const buf = Buffer.alloc(2);
          buf[0] = hi;
          buf[1] = lo;
          return buf.toString("utf-8");
        }
      } break;
  }
  return q.toString();
}

function generateOutput(src) {
  if (src.length < 12) throw new Error(`${srcpath}: Too small to be a Full Moon data archive (${src.length}<12)`);
  if ((src[0] !== 0xff) || (src[1] !== 0x41) || (src[2] !== 0x52) || (src[3] !== 0x00)) {
    throw new Error(`${srcpath}: Signature mismatch`);
  }
  const entryCount = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
  const addlLen = (src[8] << 24) | (src[9] << 16) | (src[10] << 8) | src[11];
  const tocStart = 12 + addlLen;
  const dataStart = tocStart + entryCount * 4;
  
  // Read the entire TOC into memory.
  const toc = [];
  let prev = null;
  for (let type="", qualifier="", id=1, tocp=tocStart, i=entryCount, dpmin=dataStart; i-->0; tocp+=4) {
    if (src[tocp] & 0x80) {
      type = reprType(src[tocp + 1]);
      qualifier = reprQualifier(type, (src[tocp + 2] << 8) | src[tocp + 3]);
      id = 1;
    } else {
      const offset = (src[tocp + 1] << 16) | (src[tocp + 2] << 8) | src[tocp + 3];
      if (offset < dpmin) throw new Error(`${srcpath}: Invalid TOC entry for ${type}:${qualifier}:${id}. offset=${offset} prev=${dpmin}`);
      if (prev) prev.len = offset - prev.offset;
      prev = { type, qualifier, id, offset, len: 0 };
      toc.push(prev);
      dpmin = offset;
      id++;
    }
  }
  if (prev) prev.len = src.length - prev.offset;
  
  // If they're looking to extract a resource, return its body as a Buffer.
  if (type && id) {
    let res = null;
    if (qualifier) res = toc.find(t => ((t.type === type) && (t.id === id) && (t.qualifier === qualifier)));
    else res = toc.find(t => ((t.type === type) && (t.id === id)));
    if (!res) throw new Error(`${srcpath}: resource ${type}:${qualifier}:${id} not found`);
    const dst = Buffer.alloc(res.len);
    src.copy(dst, 0, res.offset, res.offset + res.len);
    return dst;
  }
  
  // Generate a legible TOC dump and return it as a string.
  let dst = "";
  for (const { type, qualifier, id, offset, len } of toc) {
    dst += type.padStart(15);
    dst += qualifier.padStart(8);
    dst += id.toString().padStart(6);
    dst += offset.toString().padStart(12);
    dst += len.toString().padStart(12);
    dst += "\n";
  }
  return dst;
}

function hexdump(src) {
  let dst = "";
  for (let p=0; p<src.length; p+=16) {
    dst += p.toString(16).padStart(8, '0');
    dst += " |";
    for (let i=0; i<16; i++) {
      if (p + i >= src.length) {
        dst += "   ";
      } else {
        dst += " ";
        dst += "0123456789abcdef"[src[p + i] >> 4];
        dst += "0123456789abcdef"[src[p + i] & 15];
      }
    }
    dst += " | ";
    for (let i=0; i<16; i++) {
      if (p + i >= src.length) break;
      let ch = src[p + i];
      if ((ch >= 0x20) && (ch <= 0x7e)) dst += src.toString("utf8", p + i, p + i + 1);
      else dst += ".";
    }
    dst += "\n";
  }
  dst += src.length.toString(16).padStart(8, '0');
  dst += "\n";
  return dst;
}

fs.readFile(srcpath)
  .then(src => generateOutput(src))
  .then(bin => {
    if (dstpath) {
      return fs.writeFile(dstpath, bin);
    } else {
      if (typeof(bin) !== "string") bin = hexdump(bin);
      console.log(bin);
    }
  });
