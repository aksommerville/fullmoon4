#!/usr/bin/env node

const fs = require("fs");
const linewise = require("../../src/tool/common/linewise.js");

const path = "src/data/instrument/minsyn";

let dst = "";
const src = fs.readFileSync(path);
linewise(src, (line, lineno) => {
  const match = line.match(/^(\s*fm\s.*\(.*\).*\()(.*)(\).*)$/);
  if (!match) {
    dst += line + "\n";
    return;
  }
  const rangeTokens = match[2].split(/\s+/).filter(s => s);
  // The even indices are values, odds are times. We don't care about times, keep them as is.
  for (let i=0; i<rangeTokens.length; i+=2) {
    const range = +rangeTokens[i];
    const norm = ~~Math.max(0, Math.min(0xffff, range * 1000));
    rangeTokens[i] = norm.toString();
  }
  dst += `${match[1]} ${rangeTokens.join(' ')} ${match[3]}\n`;
}, true);

fs.writeFileSync(path, dst);
