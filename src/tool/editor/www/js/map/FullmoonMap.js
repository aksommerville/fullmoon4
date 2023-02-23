/* FullmoonMap.js
 */
 
export class FullmoonMap {
  constructor(src) {
  
    this.cells = new Uint8Array(FullmoonMap.COLC * FullmoonMap.ROWC);
    this.commands = []; // string[], straight off the encoded text
    this.id = 0;
    
    if (!src) ;
    else if (typeof(src) === "string") this.decode(src);
    else if (src instanceof FullmoonMap) this.copy(src);
    else throw new Error(`Inappropriate input for FullmoonMap`);
  }
  
  /* Encode to text.
   *************************************************************/
   
  encode() {
    return this.encodeCells() + this.encodeCommands();
  }
  
  encodeCells() {
    const dst = new Uint8Array((FullmoonMap.COLC * 2 + 1) * FullmoonMap.ROWC);
    for (let dstp=0, srcp=0, row=0; row<FullmoonMap.ROWC; row++) {
      for (let col=0; col<FullmoonMap.COLC; col++, srcp++) {
        dst[dstp++] = "0123456789abcdef".codePointAt(this.cells[srcp] >> 4);
        dst[dstp++] = "0123456789abcdef".codePointAt(this.cells[srcp] & 0xf);
      }
      dst[dstp++] = 0x0a;
    }
    return new TextDecoder("utf8").decode(dst);
  }
  
  encodeCommands() {
    return this.commands.map(c => c.join(" ")).reduce((a, v) => a + v + "\n", "");
  }
  
  /* Access to commands.
   **********************************************************/
   
  forEachCommand(keyword, cb) {
    for (const command of this.commands) {
      if (command[0] !== keyword) continue;
      cb(command);
    }
  }
  
  getCommand(keyword, index=0) {
    for (const command of this.commands) {
      if (command[0] !== keyword) continue;
      if (index--) continue;
      return command[1];
    }
    return null;
  }
  
  getIntCommand(keyword, index=0) {
    const command = this.getCommand(keyword, index);
    if (command === null) return 0;
    return +command || 0;
  }
  
  // Replaces the first existing instance, or appends. Single value only.
  setCommand(keyword, value) {
    const command = [keyword, "" + value];
    for (let i=0; i<this.commands.length; i++) {
      if (this.commands[i][0] !== keyword) continue;
      this.commands[i] = command;
      return;
    }
    this.commands.push(command);
  }
  
  // Appends even if the keyword has been played already.
  addCommand(keyword, ...args) {
    this.commands.push([keyword, ...args]);
  }
  
  getNeighborId(dir) {
    return this.getIntCommand("neighbor" + dir);
  }
  
  /* The three ways to make a new FullmoonMap. Privateish.
   ***************************************************************/
   
  decodeCommands(src) {
    this.commands = [];
    for (let srcp=0; srcp<src.length; ) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const command = this.decodeCommand(src.substring(srcp, nlp));
      if (command) this.commands.push(command);
      srcp = nlp + 1;
    }
  }
  
  decode(src) {
    const piclen = (FullmoonMap.COLC * 2 + 1) * FullmoonMap.ROWC;
    if (src.length < piclen) throw new Error(`Invalid length ${src.length} for encoded map.`);
    this.decodeCells(src);
    for (let srcp=piclen; srcp<src.length; ) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const command = this.decodeCommand(src.substring(srcp, nlp));
      if (command) this.commands.push(command);
      srcp = nlp + 1;
    }
  }
  
  copy(src) {
    this.cells.set(src.cells);
    this.commands = src.commands.map(cmd => [...cmd]);
    // (id) deliberately not copied
  }
  
  decodeCells(src) {
    this.cells = new Uint8Array(FullmoonMap.COLC * FullmoonMap.ROWC);
    for (let dstp=0, srcp=0, row=0; row < FullmoonMap.ROWC; row++) {
      for (let col=0; col<FullmoonMap.COLC; col++, dstp++, srcp+=2) {
        this.cells[dstp] = this.decodeCell(src.substring(srcp, srcp+2));
      }
      if (src[srcp] !== "\n") throw new Error(`Invalid line length in map cells, row ${row}`);
      srcp++;
    }
  }
  
  decodeCell(src) {
    switch (src) {
      case ". ": return 0x00;
      case "Xx": return 0x01;
    }
    const v = parseInt(src, 16);
    if (isNaN(v)) throw new Error(`Invalid map cell '${src}'`);
    return v;
  }
  
  decodeCommand(src) {
    const command = src.split(/\s+/).filter(v => v);
    if (!command.length) return null;
    return command;
  }
}

// Must agree with src/app/fmn_platform.h
FullmoonMap.COLC = 20;
FullmoonMap.ROWC = 12;
