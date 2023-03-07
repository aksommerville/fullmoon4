const fs = require("fs");
const linewise = require("./linewise.js");

let toc = null;

function requireToc() {
  if (toc) return;
  toc = [];
  try {
    const src = fs.readFileSync("src/app/fmn_platform.h");
    linewise(src, (line, lineno) => {
      if (line.startsWith("#define FMN_SFX_")) {
        const words = line.substring(16).trim().split(/\s+/);
        if (words.length >= 2) { // sic '>=' not '==='; there can be a comment
          const id = +words[1];
          if (!isNaN(id) && (id > 0) && (id < 128)) {
            toc.push([words[0].toUpperCase(), id]);
          }
        }
      }
    }, true);
  } catch (e) {
    console.error(e);
  }
}

function getSoundEffectIdByName(name) {

  if (name === "$$toc$$") {
    requireToc();
    return toc;
  }

  // If it's already a number, cool. Our sound effect names must not be numbers.
  let id = +name;
  if (isNaN(id)) {
  
    if (name.startsWith("FMN_SFX_")) name = name.substring(8);
    else if (name.startsWith("SFX_")) name = name.substring(4);
    name = name.toUpperCase();
  
    requireToc();
    for (const [srcName, srcId] of toc) {
      if (srcName === name) {
        id = srcId;
        break;
      }
    }
  }

  if (isNaN(id) || (id < 1) || (id > 127)) {
    throw new Error(`Invalid sound effect name ${JSON.stringify(name)}`);
  }
  return id;
}

module.exports = getSoundEffectIdByName;
