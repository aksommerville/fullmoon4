const fs = require("fs").promises;

let dstpath = "";
let srcpath = "";
for (let argi=2; argi<process.argv.length; ) {
  const arg = process.argv[argi++];
  if (arg === "--help") {
    console.log(`Usage: ${process.argv[1]} -oOUTPUT INPUT`);
    process.exit(0);
  } else if (arg.startsWith("-o")) {
    if (dstpath) throw new Error(`Multiple output paths`);
    dstpath = arg.substring(2);
  } else if (arg.startsWith("-")) {
    throw new Error(`Unexpected argument ${JSON.stringify(arg)}`);
  } else if (srcpath) {
    throw new Error(`Multiple input paths`);
  } else {
    srcpath = arg;
  }
}
if (!dstpath || !srcpath) {
  console.log(`Usage: ${process.argv[1]} -oOUTPUT INPUT`);
  process.exit(1);
}

fs.readFile(srcpath).then(src => {
  // There's really not much to do. Just swap in "fullmoon.js" for "bootstrap.js", and we're done.
  const dst = src.toString("utf-8").replace(/bootstrap\.js/g, "fullmoon.js");
  return fs.writeFile(dstpath, dst);
});
