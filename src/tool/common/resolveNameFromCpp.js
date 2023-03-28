const fs = require("fs");
const linewise = require("./linewise.js");

// { [path] : { [nameWithPrefix] : value } }
const resolvedFiles = {};

function getNamesForFile(path) {
  let file = resolvedFiles[path];
  if (file) return file;
  let src;
  try {
    src = fs.readFileSync(path);
  } catch (e) {
    src = Buffer.alloc(0);
  }
  file = {};
  linewise(src, (line, lineno) => {
    const match = line.match(/^\s*#define\s+([0-9a-zA-Z_]+)\s+([0-9a-zA-Z_]+)/);
    if (!match) return;
    const v = +match[2];
    if (isNaN(v)) return;
    file[match[1]] = v;
  }, true);
  resolvedFiles[path] = file;
  return file;
}

/* Look for simple "#define MACRO INTEGER" directives in (path).
 * Use the same (path) every time you're referring to the same file. "abc.d" and "./abc.d", we'll have to read it twice.
 * eg:
 *   the_file.h:
 *     ...
 *     #define MY_THANG_zoop_de_boop 123
 *     ...
 *   resolveNameFromCpp("zoop_de_boop", "the_file.h", "MY_THANG_") => 123
 */
function resolveNameFromCpp(name, path, prefix) {
  const file = getNamesForFile(path);
  if (!file) return null;
  return file[prefix + name];
}

module.exports = resolveNameFromCpp;
