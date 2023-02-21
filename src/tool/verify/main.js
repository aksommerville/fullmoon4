/* verify/main.js
 * Examines our data files, looks for inconsistencies.
 */
 
const fs = require("fs");
const verifyImage = require("./verifyImage.js");
const verifySong = require("./verifySong.js");
const verifyMap = require("./verifyMap.js");
const verifyTileprops = require("./verifyTileprops.js");
const verifySprite = require("./verifySprite.js");
const verifyString = require("./verifyString.js");
const verifyWebAudioInstrument = require("./verifyWebAudioInstrument.js");
const verifyWebAudioSound = require("./verifyWebAudioSound.js");

const RESTYPE_IMAGE = 1;
const RESTYPE_SONG = 2;
const RESTYPE_MAP = 3;
const RESTYPE_TILEPROPS = 4;
const RESTYPE_SPRITE = 5;
const RESTYPE_STRING = 6;
const RESTYPE_INSTRUMENT = 7;
const RESTYPE_SOUND = 8;
 
let archivePath = "";
let dirPath = "";
for (let i=2; i<process.argv.length; i++) {
  const arg = process.argv[i];
  
  if (arg.startsWith("--archive=")) {
    if (archivePath) throw new Error(`Multiple --archive`);
    archivePath = arg.substring(10);
    
  } else if (arg.startsWith("--dir=")) {
    if (dirPath) throw new Error(`Multiple --dir`);
    dirPath = arg.substring(6);
    
  } else {
    throw new Error(`Usage: ${process.argv[1]} [--archive=PATH] [--dir=PATH]`);
  }
}
if (!archivePath && !dirPath) {
  throw new Error(`Usage: ${process.argv[1]} [--archive=PATH] [--dir=PATH]`);
}

/* Each member is {
 *   type: 0..0xff
 *   qualifier: 0..0xffff
 *   id: 1..
 *   offset?: number // absolute position in archive (records appear in the original order; next entry's offset tells you this one's length)
 *   serial?: Buffer // final format
 *   srcPath?: string // if source file present
 *   srcSerial?: Buffer // ''
 * }
 */
const resources = [];

let warningCount = 0;

/* Populate (resources) from archive and validate archive structure.
 ************************************************************************************/
 
if (archivePath) {
  const src = fs.readFileSync(archivePath);
  
  /* Validate archive's 12-byte header.
   */
  if ((src[0] !== 0xff) || (src[1] !== 0x41) || (src[2] !== 0x52) || (src[3] !== 0x00)) {
    throw new Error(`${archivePath}: Signature mismatch ${JSON.stringify(Buffer.slice(0, 4))}, expected [255, 65, 82, 0]`);
  }
  const tocc = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
  if ((tocc < 0) || (tocc > 0x00ffffff)) {
    throw new Error(`${archivePath}: Improbable TOC Count ${tocc}`);
  }
  const addllen = (src[8] << 24) | (src[9] << 16) | (src[10] << 8) | src[11];
  if ((addllen < 0) || (addllen > src.length - 12)) {
    throw new Error(`${archivePath}: Improbable Additional Length ${addllen}, total length ${src.length}`);
  } else if (addllen) {
    // No "additional header" content is defined yet.
    console.log(`${archivePath}:WARNING: ${addllen} bytes additional header, but I don't know what it's for.`);
    warningCount++;
  }
  const expectLen = 12 + addllen + tocc * 4;
  if (expectLen > src.length) {
    throw new Error(`${archivePath}: TOC exceeds file size (expected ${expectLen}, have ${src.length}`);
  }
  
  /* Read archive and record all possible resources.
   */
  for (let i=0, srcp=12+addllen, type=0, qualifier=0, id=1; i<tocc; i++, srcp+=4) {
    const entry = (src[srcp] << 24) | (src[srcp+1] << 16) | (src[srcp+2] << 8) | src[srcp+3];
    if (entry & 0x80000000) {
      if (entry & 0x7f000000) {
        console.log(`${archivePath}:WARNING: Entry ${i}/${tocc}, 0x${entry.toString(16)}, reserved bits set`);
        warningCount++;
      }
      type = (entry >> 16) & 0xff;
      qualifier = entry & 0xffff;
      id = 1;
    } else {
      if (entry & 0xff000000) {
        // We do have a hard 16 MB limit, by design.
        throw new Error(`${archivePath}: Entry ${i}/${tocc}, 0x${entry.toString(16)}, high bits of offset set. Top 8 bits must be zero.`);
      }
      if (entry < expectLen) {
        throw new Error(`${archivePath}: Entry ${i}/${tocc} (${type}:${qualifier}:${id}), offset ${entry} overlaps header or TOC.`);
      }
      resources.push({
        type,
        qualifier,
        id,
        offset: entry,
      });
      id++;
    }
  }
  
  /* If there's at least one resource, the last one must have a nonzero length.
   * (if it's empty, why did they bother encoding it in the TOC?)
   * Last resource is kind of special too, since it doesn't have a next resource to read length from.
   */
  if (resources.length > 0) {
    const res = resources[resources.length - 1];
    const len = src.length - res.offset;
    if (len <= 0) throw new Error(`${archivePath}: Final resource ${res.type}:${res.qualifier}:${res.id} should have a nonzero length (total size ${src.length})`);
    res.serial = src.slice(res.offset);
  }
  
  /* Determine length of each resource.
   * If nonzero, copy its serial data.
   * If zero, remove the entry and issue a warning.
   * (this should be a gentle warning. Sparse resources are perfectly legal, and harmless if it's just a few here and there).
   * Come to think of it, don't log each sparse slot. Record them all, and we'll decide what to do in a sec.
   */
  const missing = [];
  for (let i=resources.length-1; i-->0; ) {
    const res = resources[i];
    const next = resources[i+1];
    const len = next.offset - res.offset;
    if (len < 0) {
      throw new Error(`${archivePath}: ${res.type}:${res.qualifier}:${res.id} at ${res.offset} followed by ${next.type}:${next.qualifier}:${next.id} at ${next.offset}. Offsets out of order.`);
    }
    if (!len) {
      missing.push(res);
      resources.splice(i, 1);
      continue;
    }
    res.serial = src.slice(res.offset, next.offset);
  }
  if (missing.length > 100) { // completely arbitrary; but do impose a limit in case it's like a million
    throw new Error(`${archivePath}: Missing ${missing.length} resources. This seems high. Please force IDs contiguous.`);
  }
  if (missing.length) {
    warningCount++;
    console.log(`${archivePath}:WARNING: Missing ${missing.length} resources:`);
    for (let i=missing.length; i-->0; ) { // we captured them backward, and with good cause. just reverse it again.
      const res = missing[i];
      console.log(`  ${res.type}:${res.qualifier}:${res.id}`);
    }
  }
}

/* Examine serial data of all resources.
 ***********************************************************************/
 
/* {
 *   fromType: number; // referrer
 *   fromId: number;
 *   toType: number; // referand
 *   toId: number;
 * }
 */
const references = []

/* Check each resource individually.
 * Errors here are not as likely as you think; most of these resources are compiled before archiving and we verify things then.
 */
for (const res of resources) {
  const { type, qualifier, id, serial } = res;
  try {
    const ref = (toType, toId) => references.push({ fromType: type, fromId: id, toType, toId });
    switch (type) {
      case 0x01: warningCount += verifyImage(serial, qualifier); break;
      case 0x02: warningCount += verifySong(serial, resources, ref); break;
      case 0x03: warningCount += verifyMap(serial, id, resources, ref); break;
      case 0x04: warningCount += verifyTileprops(serial, resources, ref); break;
      case 0x05: warningCount += verifySprite(serial, resources, ref); break;
      case 0x06: warningCount += verifyString(serial, qualifier); break;
      case 0x07: switch (qualifier) {
          case 0x01: warningCount += verifyWebAudioInstrument(serial); break;
          default: throw new Error(`Unknown instrument qualifier`);
        } break;
      case 0x08: switch (qualifier) {
          case 0x01: warningCount += verifyWebAudioSound(serial); break;
          default: throw new Error(`Unknown sound qualifier`);
        } break;
      default: throw new Error(`Unknown type`);
    }
  } catch (e) {
    e.message = `${type}:${qualifier}:${id}: ${e.message}`;
    throw e;
  }
}

/* Look for missing resources per qualifier.
 * Anywhere qualifiers are in play, a given ID must appear with every qualifier.
 * (that's not a technical requirement, but as of now at least, there aren't any exceptions to it).
 * It would be easy to add a string in English and forget the Latvian.
 */
function listUniqueProperties(read) {
  const s = new Set();
  for (const res of resources) s.add(read(res));
  return Array.from(s);
}
for (const type of listUniqueProperties(r => r.type)) {
  const qualifiers = listUniqueProperties(r => (r.type === type) ? r.qualifier : 0).filter(v => v);
  if (!qualifiers.length) continue;
  const ids = listUniqueProperties(r => (r.type === type) ? r.id : 0).filter(v => v);
  for (const id of ids) {
    for (const qualifier of qualifiers) {
      if (!resources.find(r => r.type === type && r.qualifier === qualifier && r.id === id)) {
        console.log(`${archivePath}:WARNING: Missing ${type}:${qualifier}:${id}, but that type::id is used by other qualifiers.`);
        warningCount++;
      }
    }
  }
}

/* Verify that all resources are reachable.
 */
const unreachable = [...resources];
function reachable(type, id) {
  let found = false;
  for (let i=unreachable.length; i-->0; ) {
    if ((unreachable[i].type === type) && (unreachable[i].id === id)) {
      unreachable.splice(i, 1);
      found = true;
    }
  }
  if (!found) return;
  for (let i=references.length; i-->0; ) {
    const ref = references[i];
    if ((ref.fromType === type) && (ref.fromId === id)) {
      reachable(ref.toType, ref.toId);
    }
  }
}

// Resources known to be referred in code:
reachable(RESTYPE_MAP, 1); // initial map
reachable(RESTYPE_IMAGE, 2); // hero
reachable(RESTYPE_IMAGE, 4); // items splash
for (let id=1; id<=11; id++) { // #define FMN_SFX_INJURY_DEFLECTED 11
  reachable(RESTYPE_SOUND, id);
}

if (unreachable.length) {
  warningCount++;
  console.log(`${archivePath}:WARNING: ${unreachable.length} unreachable resources:`);
  // If few enough, show each unreachable resources.
  if (unreachable.length < 30) {
    for (let i=0; i<unreachable.length; i++) {
      const res = unreachable[i];
      console.log(`  ${res.type}:${res.qualifier}:${res.id}`);
    }
  // Too many, summarize by type.
  } else {
    for (const type of listUniqueProperties(r => r.type)) {
      const rs = unreachable.filter(r => r.type === type);
      if (!rs.length) continue;
      let ids = JSON.stringify(rs.slice(0, 5).map(r => r.id));
      if (rs.length > 5) ids += "...";
      console.log(`  ${type}: ${rs.length} ${ids}`);
    }
  }
}

/* Examine input files.
 *************************************************************************/
 
if (dirPath) {
  //TODO Not sure we need to do anything here.
  // It would be crazy to recompile them all just to compare against the archive (tho that is an option).
}

if (warningCount) {
  console.log(`${archivePath}: Valid but some issues, see above.`);
} else {
  console.log(`${archivePath}: Valid.`);
}
