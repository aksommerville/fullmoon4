/* validateNonArchiveFiles.js
 * We were supplied a path to the loose input files.
 * Validate that against the unpacked archive.
 */
 
const fs = require("fs");
const linewise = require("../common/linewise.js");

/* gsbit
 * There must be a file "{dirPath}/gsbit" where each line is "ID NAME".
 * These are used during resource compilation so eg a map can say "gs:footswitch_in_castle_dungeon" instead of "1234".
 * Each ID and NAME must be unique. This may have already been asserted during compile, but we can do it here as a safety net.
 * If we wanted to get fancy with it, we could warn about missing or unused bits. I think not worth it.
 * FYI: Demo and Full maps use the same set of gsbit.
 ***************************************************************************/
 
function validateGsBit(dirPath) {
  let warningCount = 0;
  const defs = []; // [lineno, id, name]
  const src = fs.readFileSync(dirPath + "/gsbit");
  linewise(src, (line, lineno) => {
    let [id, name, ...unexpected] = line.split(' ');
    id = +id;
    if (unexpected.length || isNaN(id) || (id < 0) || !name.match(/^[a-zA-Z_][a-zA-Z_0-9]*$/)) {
      throw new Error(`${dirPath}/gsbit:${lineno}: Malformed line`);
    }
    if (!id) {
      console.log(`${dirPath}/gsbit:${lineno}:WARNING: gsbit ID zero not recommended for use.`);
      warningCount++;
    }
    const already = defs.find(([ln, aid, aname]) => ((aid === id) || (aname === name)));
    if (already) {
      if (already[1] === id) {
        if (already[2] === name) {
          console.log(`${dirPath}/gsbit:${lineno}:WARNING: "${id}=${name}" was already defined on line ${already[0]}`);
          warningCount++;
        } else {
          throw new Error(`${dirPath}/gsbit:${lineno}: gsbit ${id} defined as both "${already[2]}" on line ${already[0]} and "${name}" on line ${lineno}`);
        }
      } else if (already[2] === name) {
        throw new Error(`${dirPath}/gsbit:${lineno}: gsbit "${name}" defined as ID ${already[1]} on line ${already[0]} and ${id} on line ${lineno}`);
      }
    }
    defs.push([lineno, id, name]);
  });
  return warningCount;
}

/* chalk
 * There must be a directory "{dirPath}/chalk" with one file named "1".
 * That path enables our editor to load it like other resources.
 * Our archiver knows to ignore it.
 * Each line is "CODEPOINT BITS" where CODEPOINT in 0x21..0x7e and BITS in 1..0x000fffff.
 * CODEPOINT will not be unique but BITS must be.
 * Each of the uppercase letters (0x41..0x5a) must be defined at least once.
 *******************************************************************************/
 
function validateChalk(dirPath, resources) {
  let warningCount = 0;
  const defs = []; // [lineno, codepoint, bits]
  const bases = fs.readdirSync(dirPath + "/chalk");
  if ((bases.length !== 1) || (bases[0] !== "1")) {
    throw new Error(`${dirPath}/chalk: Expected ["1"] in chalk directory, found ${JSON.stringify(bases)}`);
  }
  const src = fs.readFileSync(dirPath + "/chalk/1");
  linewise(src, (line, lineno) => {
    const [codepoint, bits, ...unexpected] = line.split(' ').map(v => +v);
    if (isNaN(codepoint) || isNaN(bits) || unexpected.length) {
      throw new Error(`${dirPath}/chalk/1:${lineno}: Malformed line`);
    }
    if ((codepoint < 0x21) || (codepoint > 0x7e)) {
      throw new Error(`${dirPath}/chalk/1:${lineno}: Invalid codepoint ${codepoint}, must be in G0`);
    }
    if ((codepoint >= 0x61) && (codepoint <= 0x7a)) {
      throw new Error(`${dirPath}/chalk/1:${lineno}: Codepoint ${codepoint} is lowercase. Please use uppercase only.`);
    }
    if ((bits < 1) || (bits > 0x000fffff)) {
      throw new Error(`${dirPath}/chalk/1:${lineno}: Bits must be in 1..${0xfffff} (ie 20 bits), found ${bits}`);
    }
    const already = defs.find(([ln, cp, b]) => b === bits);
    if (already) {
      // Here's the main thing we're interested in, realistically.
      throw new Error(`${dirPath}/chalk/1:${lineno}: Bits ${bits} for codepoint ${codepoint} was already used on line ${already[0]} for codepoint ${already[1]}`);
    }
    defs.push([lineno, codepoint, bits]);
  });
  return warningCount;
}
 
/* Export and TOC.
 ************************************************************************/
 
module.exports = (dirPath, archivePath, resources) => {
  let warningCount = 0;
  
  warningCount += validateGsBit(dirPath);
  warningCount += validateChalk(dirPath, resources);
  
  return warningCount;
};
