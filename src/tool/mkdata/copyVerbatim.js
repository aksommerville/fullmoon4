const fs = require("fs").promises || require("../common/fakeFsPromises.js");

module.exports = path => fs.readFile(path);
