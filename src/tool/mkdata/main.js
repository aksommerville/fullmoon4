const fs = require("fs").promises;

function printUsage() {
  const exename = process.argv[1];
  console.log(
    `Usage: ${exename} --single -oOUTPUT INPUT\n` +
    `   Or: ${exename} --archive -oOUTPUT [INPUTS...]`
  );
}

let mode = "";
let dstpath = "";
const srcpaths = [];
for (let argi=2; argi<process.argv.length; ) {
  const arg = process.argv[argi++];
  
  if (arg === "--help") {
    printUsage();
    process.exit(0);
    
  } else if (arg.startsWith("-o")) {
    if (dstpath) throw new Error(`Multiple output paths.`);
    dstpath = arg.substring(2);
    
  } else if ((arg === "--single") || (arg === "--archive")) {
    if (mode) throw new Error(`Multiple modes. Must provide exactly one of '--single', '--archive'`);
    mode = arg;
    
  } else if (arg.startsWith("-")) {
    throw new Error(`Unexpected argument ${JSON.stringify(arg)}`);
    
  } else {
    srcpaths.push(arg);
  }
}
if (!dstpath || !mode) {
  printUsage();
  process.exit(1);
}
if ((mode === "--single") && (srcpaths.length !== 1)) {
  throw new Error(`Expected 1 input with '--single', found ${srcpaths.length}`);
}

function guessOperationName(dst, src) {
  const dataTypeMatch = src.match(/data\/([^\/]*)\//);
  if (dataTypeMatch) switch (dataTypeMatch[1]) {
    case "image": return "./copyVerbatim.js"; // PNG in, PNG out, for now at least
    case "song": return "./copyVerbatim.js"; // MIDI in, MIDI out, for now at least
    case "tileprops": return "./processTileprops.js";
    case "map": return "./processMap.js";
    case "sprite": return "./processSprite.js";
    case "string": return "./copyVerbatim.js"; // will always be verbatim; processed at packing
  }
  return "./copyVerbatim.js";
}

let op;
if (mode === "--archive") op = require("./packArchive.js")(srcpaths);
else {
  const opName = guessOperationName(dstpath, srcpaths[0]);
  if (!opName) {
    throw new Error(`Unable to determine operation for output=${dstpath} and input=${srcpaths[0]}`);
  }
  op = require(opName)(srcpaths[0]);
}

op.then(dst => fs.writeFile(dstpath, dst));
