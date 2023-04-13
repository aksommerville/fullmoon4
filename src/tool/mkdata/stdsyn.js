class StdsynInstrument {
  constructor(id, path, lineno) {
    this.id = id;
    this.path = path;
    this.lineno = lineno;
    this.type = "instrument";
  }
  
  encode() {
    return Buffer.alloc(0);
  }
  
  receiveLine(words, path, lineno) {
  }
}

class StdsynSound {
  constructor(id, path, lineno) {
    this.id = id;
    this.path = path;
    this.lineno = lineno;
    this.type = "sound";
  }
  
  encode() {
    return Buffer.alloc(0);
  }
  
  receiveLine(words, path, lineno) {
  }
}

module.exports = { StdsynInstrument, StdsynSound };
