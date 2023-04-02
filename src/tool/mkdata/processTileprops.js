const fs = require("fs").promises || require("../common/fakeFsPromises.js");

function decodeCell(src) {
  const v = parseInt(src, 16);
  if (isNaN(v)) throw new Error(`Illegal cell value '${src}'`);
  return v;
}

function compileTileprops(src, path) {
  const physics = Buffer.alloc(256);
  let physicsc = 0;
  let reading = false;
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

module.exports = path => fs.readFile(path)
  .then(src => compileTileprops(src, path));
