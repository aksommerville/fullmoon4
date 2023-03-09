const fs = require("fs");
const linewise = require("./linewise.js");

let gsbitToc = null;

function resolveGsbitName(src) {
  if (!gsbitToc) {
    const src = fs.readFileSync("src/data/gsbit");
    gsbitToc = {};
    linewise(src, (line, lineno) => {
      let [gsbit, name] = line.trim().split(/\s+/);
      gsbit = +gsbit;
      if (!gsbit) throw new Error(`src/data/gsbit:${lineno}: Expected integer >0`);
      if (gsbitToc[name]) throw new Error(`src/data/gsbit:${lineno}: gsbit name ${JSON.stringify(name)} previously defined as ${gsbitToc[name]}`);
      gsbitToc[name] = gsbit;
    });
  }
  return gsbitToc[src] || src;
}

module.exports = resolveGsbitName;
