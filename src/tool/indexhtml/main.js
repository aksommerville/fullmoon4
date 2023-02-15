const fs = require("fs");

let dstpath = "";
let srcpath = "";
let favicon = "";
let css = "";
for (let argi=2; argi<process.argv.length; ) {
  const arg = process.argv[argi++];
  if (arg === "--help") {
    console.log(`Usage: ${process.argv[1]} -oOUTPUT [--favicon=PATH] [--css=PATH] INPUT`);
    process.exit(0);
    
  } else if (arg.startsWith("-o")) {
    if (dstpath) throw new Error(`Multiple output paths`);
    dstpath = arg.substring(2);
    
  } else if (arg.startsWith("--favicon=")) {
    if (favicon) throw new Error(`Multiple favicon`);
    favicon = arg.substring(10);
    
  } else if (arg.startsWith("--css=")) {
    if (css) throw new Error(`Multiple css`);
    css = arg.substring(6);
  
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

// It's not rocket science. Just remove comments and excess whitespace.
function minifyCss(input) {
  let output = "";
  let inp = 0;
  let space = true;
  while (inp < input.length) {
    if ((input[inp] === "/") && (input[inp + 1] === "*")) {
      const closep = input.indexOf("*/", inp + 2);
      if (closep < 0) throw new Error("Unclosed comment in CSS");
      inp = closep + 2;
    } else if (input.charCodeAt(inp) <= 0x20) {
      if (space) {
        inp++;
      } else {
        // Don't bother figuring out whether the space is necessary. Only eliminate adjacent spaces.
        output += input[inp];
        space = true;
        inp++;
      }
    } else {
      space = false;
      output += input[inp];
      inp++;
    }
  }
  return output;
}

function insertIcon(html, base64) {
  const titlep = html.indexOf("<title");
  if (titlep < 0) throw new Error("Failed to locate '<title' in index.html");
  let linep = html.indexOf("\n", titlep);
  if (linep < 0) throw new Error("No newline after '<title' in index.html");
  linep++;
  return html.substring(0, linep) + "  <link rel=\"icon\" type=\"image/png\" href=\"data:;base64," + base64 + "\" />\n" + html.substring(linep);
}

function insertCss(html, css) {
  css = minifyCss(css);
  const startp = html.indexOf("  <link rel=\"stylesheet\"");
  let endp = html.indexOf("\n", startp);
  if ((startp < 0) || (endp < 0)) throw new Error("Failed to local '  <link rel=\"stylesheet\"' in index.html");
  endp++;
  return html.substring(0, startp) + "<style>\n" + css + "</style>\n" + html.substring(endp);
}

fs.promises.readFile(srcpath).then(src => {

  // Make it a string, and replace "bootstrap.js" with "fullmoon.js".
  let dst = src.toString("utf-8").replace(/bootstrap\.js/g, "fullmoon.js");
  
  // Insert favicon if requested.
  if (favicon) {
    const iconBase64 = fs.readFileSync(favicon).toString("base64");
    dst = insertIcon(dst, iconBase64);
  }
  
  // Insert CSS if requested.
  if (css) {
    const cssText = fs.readFileSync(css).toString("utf-8");
    dst = insertCss(dst, cssText);
  }

  return fs.promises.writeFile(dstpath, dst);
});
