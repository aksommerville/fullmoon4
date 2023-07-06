/* spriteMetadata.js
 * Reads "argtype" commands from the sprites' source files.
 */
 
const fs = require("fs");
const linewise = require("../common/linewise.js");
 
function spriteMetadataNew() {
  return {
    byId: [],
  };
}

function spriteMetadataPopulateAll(spriteMetadata, dirPath) {
  for (const base of fs.readdirSync(dirPath + "/sprite")) {
    const match = base.match(/^\d+/);
    if (!match) continue;
    const id = +match;
    if (!id) continue;
    const metadata = {
      argv: [], // {type,name}
    };
    const path = `${dirPath}/sprite/${base}`;
    const src = fs.readFileSync(path);
    linewise(src, (line, lineno) => {
      const words = line.split(/\s+/);
      if (words.length !== 4) return;
      if (words[0] !== "argtype") return;
      metadata.argv[+words[1]] = { type: words[2], name: words[3] };
    });
    spriteMetadata.byId[id] = metadata;
  }
}
 
module.exports = (dirPath) => {
  const spriteMetadata = spriteMetadataNew();
  if (dirPath) spriteMetadataPopulateAll(spriteMetadata, dirPath);
  return spriteMetadata;
};
