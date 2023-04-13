const fs = require("fs").promises || require("../common/fakeFsPromises.js");
const readInstruments = require("./readInstruments.js");

// => { type, id, qualifier, name }
function parsePath(path) {
  const components = path.split(/[\/\\]/);
  if (components.length < 2) {
    throw new Error(`Unable to determine resource identity from ${JSON.stringify(path)}`);
  }
  const type = components[components.length - 2];
  const base = components[components.length - 1];
  
  // "string" and "instrument" resources are a little different.
  // Multiple resources per input file, and the entire basename is "qualifier".
  if (type === "string") return { type, id: 0, qualifier: base, name: "" };
  if (type === "instrument") return { type, id: 0, qualifier: base, name: "" };
  if (type === "sound") return { type, id: 0, qualifier: base, name: "" };
  
  // All other resources are 1:1 with input files.
  // Names are: ID[-NAME][.QUALIFIER][.FORMAT]
  // ID is required and must be in 1..0xffff. Some types restrict further to 0xff, but we're not enforcing that here.
  let dashp = base.indexOf("-");
  let dot0p = base.indexOf(".", dashp);
  if (dot0p < 0) dot0p = base.length;
  const dot1p = base.indexOf(".", dot0p + 1);
  if (dashp < 0) dashp = dot0p;
  const id = +base.substring(0, dashp);
  if (isNaN(id) || (id < 1) || (id > 0xffff)) {
    throw new Error(`Unable to determine resource ID from basename: ${JSON.stringify(base)}`);
  }
  const name = base.substring(dashp + 1, dot0p);
  const qualifier = (dot1p >= 0) ? base.substring(dot0p + 1, dot1p) : "";
  
  return { type, id, qualifier, name };
}

function readStrings(src, path, qualifier, cb) {
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    let nlp = src.indexOf(0x0a, srcp);
    if (nlp < 0) nlp = src.length;
    const line = src.toString("utf-8", srcp, nlp).trim();
    srcp = nlp + 1;
    if (!line) continue;
    const spacep = line.indexOf(" ");
    if (spacep < 0) throw new Error(`${path}:${lineno}: Expected 'ID TEXT...'`);
    const id = +line.substring(0, spacep);
    if (isNaN(id) || (id < 1) || (id > 0xffff)) {
      throw new Error(`${path}:${lineno}: Invalid string ID`);
    }
    cb(id, line.substring(spacep + 1));
  }
}

function encodeType(type) {
  switch (type) {
    case "image": return 0x01;
    case "song": return 0x02;
    case "map": return 0x03;
    case "tileprops": return 0x04;
    case "sprite": return 0x05;
    case "string": return 0x06;
    case "instrument": return 0x07;
    case "sound": return 0x08;
  }
  throw new Error(`Unknown resource type ${JSON.stringify(type)}`);
}

function encodeQualifier(type, qualifier) {
  switch (type) {
    case "string": {
        if (qualifier.length !== 2) {
          throw new Error(`String qualifier length must be 2: ${JSON.stringify(qualifier)}`);
        }
        return (qualifier.codePointAt(0) << 8) | qualifier.codePointAt(1);
      }
    case "image": {
        //TODO image qualifiers
        return 0;
      }
    case "instrument":
    case "sound": {
        if (!qualifier) return 0;
        if (qualifier === "WebAudio") return 1;
        if (qualifier === "minsyn") return 2;
        if (qualifier === "stdsyn") return 3;
        const n = +qualifier;
        if (!isNaN(n) && (n >= 0) && (n <= 0xffff)) return n;
        throw new Error(`Unexpected instruments qualifier ${JSON.stringify(qualifier)}`);
      }
  }
  return 0;
}

function encode(files) {
  
  // Read TOC metadata from basenames.
  for (const file of files) {
    const { type, id, qualifier, name } = parsePath(file.path);
    file.type = type;
    file.id = id;
    file.qualifier = qualifier;
  }
  
  //TODO Allow an option to filter resources out based on qualifier.
  // No sense including hi-res graphics if we're building for a low-res-only platform, eg.
  
  // "string" and "instrument" arrive as packed sub-archives. Decode and split them up.
  for (let i=files.length; i-->0; ) {
    const file = files[i];
    let unpack = null;
    switch (file.type) {
      case "string": unpack = readStrings; break;
      case "instrument": unpack = readInstruments; break;
      case "sound": unpack = readInstruments; break; // sic; sound and instrument are the same thing
    }
    if (unpack) {
      files.splice(i, 1);
      unpack(file.serial, file.path, file.qualifier, (id, serial, type) => {
        files.push({
          type: type || file.type,
          path: file.path,
          qualifier: file.qualifier,
          id, serial,
        });
      });
    }
  }
  
  // Sort by (type,qualifier,id) and abort on duplicates.
  // I don't think the particular order of types and qualifiers matters, as long as each (type,qualifier) pair is contiguous.
  files.sort((a, b) => {
    if (a.type < b.type) return -1;
    if (a.type > b.type) return 1;
    if (a.qualifier < b.qualifier) return -1;
    if (a.qualifier > b.qualifier) return -1;
    if (a.id < b.id) return -1;
    if (a.id > b.id) return 1;
    throw new Error(`Duplicate resource ${a.type}:${a.qualifier}:${a.id}`);
  });
  
  // Insert dummy entries where (type,qualifier) changes, and for missing resources.
  for (let i=0, type="", qualifier="", nextId=1; i<files.length; ) {
    const file = files[i];
    if ((file.type !== type) || (file.qualifier !== qualifier)) {
      const stateChange = {
        path: "",
        type: file.type,
        qualifier: file.qualifier,
        id: 0,
        serial: "",
      };
      files.splice(i, 0, stateChange);
      nextId = 1;
      i++;
      type = file.type;
      qualifier = file.qualifier;
    }
    while (file.id > nextId) {
      const dummy = {
        path: "",
        type,
        qualifier,
        id: nextId,
        serial: "",
      };
      files.splice(i, 0, dummy);
      nextId++;
      i++;
    }
    nextId++;
    i++;
  }
  
  // Take some measurements.
  const headerLength = 12; // 4:signature, 4:count, 4:addl
  const addlLength = 0;
  const tocLength = files.length * 4;
  let datap = headerLength + addlLength + tocLength;
  const totalLength = datap + files.reduce((a, v) => a + v.serial.length, 0);
  
  // Generate the output buffer and emit header.
  const dst = Buffer.alloc(totalLength);
  let dstp = 0;
  dst[dstp++] = 0xff;
  dst[dstp++] = 0x41; // 'A'
  dst[dstp++] = 0x52; // 'R'
  dst[dstp++] = 0x00;
  dst[dstp++] = files.length >> 24;
  dst[dstp++] = files.length >> 16;
  dst[dstp++] = files.length >> 8;
  dst[dstp++] = files.length;
  dst[dstp++] = addlLength >> 24;
  dst[dstp++] = addlLength >> 16;
  dst[dstp++] = addlLength >> 8;
  dst[dstp++] = addlLength;
  // 'addl' goes here if we ever do that.
  
  // For each file, add to TOC and data.
  //console.log(`encoding archive...`);
  for (const file of files) {
  
    //if (file.id) console.log(`  ${file.type}:${file.qualifier}:${file.id}: ${file.serial.length} bytes`);
    //else console.log(`  BEGIN ${file.type}:${file.qualifier}`);
  
    if (file.id) { // Resource
      dst[dstp++] = datap >> 24;
      dst[dstp++] = datap >> 16;
      dst[dstp++] = datap >> 8;
      dst[dstp++] = datap;
      if (file.serial.length) {
        if (typeof(file.serial) === "string") {
          file.serial = Buffer.from(file.serial);
        }
        file.serial.copy(dst, datap);
        datap += file.serial.length;
      }
    } else { // State Change
      dst[dstp++] = 0x80;
      dst[dstp++] = encodeType(file.type);
      const q = encodeQualifier(file.type, file.qualifier);
      dst[dstp++] = q >> 8;
      dst[dstp++] = q;
    }
  }
  
  const expect = headerLength + addlLength + tocLength;
  if (dstp !== expect) {
    throw new Error(`Expected TOC to end at ${expect} (${headerLength}+${addlLength}+${tocLength}), but ended at ${dstp}`);
  }
  if (datap !== dst.length) {
    throw new Error(`Expected data dump to end at ${dst.length}, but ended at ${datap}`);
  }
  
  return dst;
}

function packArchive(paths) {
  paths = paths.filter(p => !p.endsWith("/gsbit"));
  return Promise.all(paths.map(path => fs.readFile(path).then(serial => ({ path, serial }))))
    .then(files => encode(files));
}

module.exports = packArchive;
