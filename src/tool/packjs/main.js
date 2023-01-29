const fs = require("fs").promises;

function printUsage() {
  console.log(`Usage: ${process.argv[1]} -oOUTPUT [INPUT...]`);
}

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
    
  } else if (arg.startsWith("-")) {
    throw new Error(`Unexpected argument ${JSON.stringify(arg)}`);
    
  } else {
    srcpaths.push(arg);
  }
}
if (!dstpath) {
  printUsage();
  process.exit(1);
}

function acquireInput(path) {
  return fs.readFile(path);
}

function addImport(imports, line) {
  // I'm not sure that we actually need these.
}

function addExport(exports, line) {
  // I'm not sure that we actually need these.
}

function processFile(f) {
  let dst = "";
  const imports = [];
  const exports = [];
  for (let srcp=0, lineno=1; srcp<f.text.length; lineno++) {
    let nlp = f.text.indexOf(0x0a, srcp);
    if (nlp < 0) nlp = f.text.length;
    let line = f.text.toString("utf-8", srcp, nlp).trim();
    srcp = nlp + 1;
    
    //TODO Comment removal would be nice too. Not important right now.
    
    if (line.startsWith("import ")) {
      addImport(imports, line);
      continue;
    }
    if (line.startsWith("export ")) {
      addExport(exports, line);
      line = line.substring(7).trim();
    }
    if (line) {
      dst += line + "\n";
    }
  }
  return {
    path: f.path,
    text: dst,
    imports,
    exports,
  };
}

Promise.all(
  srcpaths.map(path => acquireInput(path).then(text => ({ path, text })))
).then(srcfiles => srcfiles.map(f => processFile(f)))
.then(srcfiles => {
  const text = srcfiles.reduce((a, v) => a + v.text, "");
  return fs.writeFile(dstpath, text);
});
