/* On my ancient Asus laptop, can't seem to run Node after version 9.
 * There's no `fs.promises` in v9, so faking it.
 */
 
const fs = require("fs");
 
function readFile(path) {
  return new Promise((resolve, reject) => {
    fs.readFile(path, (error, content) => {
      if (error) reject(error);
      else resolve(content);
    });
  });
}

function writeFile(path, content) {
  return new Promise((resolve, reject) => {
    fs.writeFile(path, content, (error, content) => {
      if (error) reject(error);
      else resolve(content);
    });
  });
}
 
module.exports = {
  readFile,
  writeFile,
};
