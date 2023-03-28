const resolveItemName = require("./resolveItemName.js");
const resolveGsbitName = require("./resolveGsbitName.js");
const resolveNameFromCpp = require("./resolveNameFromCpp.js");
const fs = require("fs");

/* Keyed by resource type string, values are { [name]: id }
 */
let toc = {};

function generateToc(resType) {
  const typeToc = {};
  
  for (const base of (fs.readdirSync(`src/data/${resType}`) || [])) {
    const match = base.match(/^(\d+)-([^.]*)/);
    if (match && match[2]) {
      const id = +match[1];
      if (!isNaN(id) && (id > 0)) {
        typeToc[match[2]] = id;
      }
    }
  }
  
  return typeToc;
}

function getResourceIdByName(resType, name) {

  // If it's a number already, roll with it.
  // We don't check range, that's the caller's problem.
  // (worth noting, resource names are not supposed to be numbers).
  let id = +name;
  if (!isNaN(id)) return id;
  
  // Allow a few string things that aren't resources but similar.
  // This is really getting stupid. Next time around, let's have some structure around dynamic enums.
  if (typeof(name) === "string") {
         if (name.startsWith("gs:")) id = resolveGsbitName(name.substring(3));
    else if (name.startsWith("item:")) id = resolveItemName(name.substring(5));
    else if (name.startsWith("ev:")) id = resolveNameFromCpp(name.substring(3), "src/app/fmn_platform.h", "FMN_MAP_EVID_");
    else if (name.startsWith("cb:")) id = resolveNameFromCpp(name.substring(3), "src/app/fmn_game.h", "FMN_MAP_CALLBACK_");
    else id = null;
    if (typeof(id) === "number") return id;
  }
  
  if (resType) {
    if (!toc[resType]) toc[resType] = generateToc(resType);
    const typeToc = toc[resType];
    const id = typeToc[name];
    if (!isNaN(id)) return id;
    throw new Error(`Unknown ${resType} resource ${JSON.stringify(name)}`);
  }
  throw new Error(`Failed to evaluate ${JSON.stringify(name)}`);
}

module.exports = getResourceIdByName;
