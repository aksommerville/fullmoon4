/* linewise.js
 * Trigger your callback for each non-empty line after stripping comments and whitespace.
 * Input should be a Buffer.
 */

module.exports = (src, cb) => {
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    let nlp = src.indexOf(0x0a, srcp);
    if (nlp < 0) nlp = src.length;
    const line = src.toString("utf-8", srcp, nlp).split('#')[0].trim();
    srcp = nlp + 1;
    if (line) cb(line, lineno);
  }
};
