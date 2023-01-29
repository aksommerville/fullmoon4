const fs = require("fs");

let idsByName = null;

function getControllerIdsByName() {
  if (idsByName) return idsByName;
  idsByName = {};
  
  // The source of truth for controller IDs is src/app/sprite/fmn_sprite.h.
  const src = fs.readFileSync("src/app/sprite/fmn_sprite.h");
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    let nlp = src.indexOf(0x0a, srcp);
    if (nlp < 0) nlp = src.length;
    const line = src.toString("utf-8", srcp, nlp).trim();
    srcp = nlp + 1;
    
    const match = line.match(/^#define FMN_SPRCTL_([^\s]*)\s+([^\s]*)$/);
    if (!match) continue;
    const name = match[1];
    const id = +match[2];
    if (!name || isNaN(id) || (id < 0) || (id > 0xffff)) continue;
    idsByName[name] = id;
  }
  return idsByName;
}

module.exports = getControllerIdsByName;
