/* Tileprops.js
 * Resource type parallel to image, with extra per-tile data.
 */
 
export class Tileprops {
  constructor(src) {
    this.fields = {}; // name:Uint8Array(256)
    if (!src) ;
    else if (src instanceof Tileprops) this.copy(src);
    else if (typeof(src) === "string") this.decodeText(src);
    else if (src instanceof ArrayBuffer) this.decodeBinary(new Uint8Array(src));
    else if (src instanceof Uint8Array) this.decodeBinary(src);
    else throw new Error(`Inappropriate input for new Tileprops`);
  }
  
  copy(src) {
    for (const key of Object.keys(src.fields)) {
      this.fields[key] = new Uint8Array(256);
      this.fields[key].copy(src.fields[key]);
    }
  }
  
  decodeText(src) {
    let key = "";
    const intake = [];
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      // Replace \s, not just trim():
      const line = src.substring(srcp, nlp).replace(/\s/g, "");
      srcp = nlp + 1;
      if (!line || line[0] === '#') continue;
      
      if (!key) {
        if (line.match(/^[0-9a-fA-F]{32}$/)) {
          // No key will ever be composed of exactly 32 hex digits.
          throw new Error(`Expected key on line ${lineno}, found what looks like a data row.`);
        }
        key = line;
      } else {
        for (let linep=0; linep<32; linep+=2) {
          const token = line.substring(linep, linep+2);
          const v = parseInt(token, 16);
          if (isNaN(v)) {
            throw new Error(`Line ${lineno} in tileprops, unexpected value '${token}' for tile ${intake.length}.`);
          }
          intake.push(v);
        }
        if (intake.length === 256) {
          this.fields[key] = new Uint8Array(intake);
          intake.splice(0, 256);
          key = "";
        }
      }
    }
    if (key) {
      throw new Error(`Incomplete table '${key}' in tileprops.`);
    }
  }
  
  // Uint8Array
  decodeBinary(src) {
    for (let srcp=0, tablep=0; srcp<src.length; srcp+=256, tablep++) {
      const key = Tileprops.IMPLICIT_FIELD_ORDER[tablep];
      if (!key) break; // Ran out of implicit keys, discard the remainder.
      const srcView = new Uint8Array(
        src.buffer,
        src.byteOffset + srcp,
        256
      ); // should throw if input is short
      const dst = new Uint8Array(256);
      dst.set(srcView);
      this.fields[key] = dst;
    }
  }
  
  // To text. I'm not sure we'll be using the binary format.
  encode() {
    let dst = "";
    for (const key of Object.keys(this.fields)) {
      dst += key + "\n";
      const src = this.fields[key];
      for (let row=0, srcp=0; row<16; row++) {
        for (let col=0; col<16; col++, srcp++) {
          dst += "0123456789abcdef"[src[srcp] >> 4];
          dst += "0123456789abcdef"[src[srcp] & 0xf];
        }
        dst += "\n";
      }
      dst += "\n"; // extra blank line between tables for legibility
    }
    return dst;
  }
}

// Field order for binary files.
// If input to decode is longer than this, we drop the excess.
// When encoding, if we have a field not named here, we drop it.
Tileprops.IMPLICIT_FIELD_ORDER = [
  "physics",
  "family",
  "neighbors",
  "weight",
];
