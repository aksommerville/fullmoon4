/* FullmoonMap.js
 * "Map" to its friends, but that is already a JS class name.
 */
 
export class FullmoonMap {
  constructor(src, constants) {
    // Caller should assign:
    this.id = 0;
    this.cellphysics = null; // Uint8Array(256), ie Tileprops resource
    
    if (!src) this._init(constants);
    else if (src instanceof FullmoonMap) this._copy(src);
    else if (src instanceof Uint8Array) this._decode(src, constants);
    else if (src instanceof ArrayBuffer) this._decode(new Uint8Array(src), constants);
    else throw new Error(`Unsuitable input for FullmoonMap`);
  }
  
  // cb(opcode, v, argp, argc), return nonzero to stop
  forEachCommand(cb) {
    return this._readCommands(this.commands, cb);
  }
  
  getCommandLocation(opcode, argv, argp, argc) {
    //TODO hard-coded map dimensions 20x12. Put those constants somewhere we don't need an Injector to reach.
    switch (opcode) {
      // It's weird to ask this about NEIGHBOR commands, but let's be coherent about it:
      case 0x40: return [10 - 20, 6];
      case 0x41: return [10 + 20, 6];
      case 0x42: return [10, 6 - 12];
      case 0x43: return [10, 6 + 12];
      // Every positioned command so far stores its position packed in the first byte:
      case 0x22: // HERO
      case 0x44: // TRANSMOGRIFY
      case 0x60: // DOOR
      case 0x61: // SKETCH
      case 0x80: // SPRITE
      case 0x64: // EVENT_TRIGGER
          return [argv[argp] % 20, Math.floor(argv[argp] / 20)];
    }
    return [10, 6]; // Middle of screen, for commands without a location.
  }
  
  _init(constants) {
    this.cells = new Uint8Array(constants.COLC * constants.ROWC);
    this.commands = []; // Normally a Uint8Array pointing into the original source.
    this.doors = []; // {x,y,mapId,dstx,dsty,extra}
    this.sprites = []; // {x,y,spriteId,arg0,arg1,arg2}
    this.sketches = []; // {x,y,bits}. Should only be examined if no user sketches exist at load.
    this.callbacks = []; // {evid,cbid,param}
    this.cellphysics = null; // Uint8Array(256), but supplied by our owner
    this.dark = 0;
    this.indoors = 0;
    this.blowback = 0;
    this.ancillary = 0;
    this.wind = 0;
    this.herostartp = (constants.ROWC >> 1) * constants.COLC + (constants.COLC >> 1);
    this.songId = 0;
    this.bgImageId = 0;
    this.neighborw = 0;
    this.neighbore = 0;
    this.neighborn = 0;
    this.neighbors = 0;
  }
  
  _copy(src) {
    this.cells = new Uint8Array(src.cells);
    this.commands = new Uint8Array(src.commands);
    this.doors = src.doors.map(d => ({ ...d }));
    this.sprites = src.sprites.map(s => ({ ...s }));
    this.sketches = src.sketches.map(s => ({ ...s }));
    this.callbacks = src.callbacks.map(cb => ({ ...cb }));
    this.cellphysics = src.cellphysics; // null or a globally shared Tileprops, no need to copy.
    this.dark = src.dark;
    this.indoors = src.indoors;
    this.blowback = src.blowback;
    this.ancillary = src.ancillary;
    this.wind = src.wind;
    this.herostartp = src.herostartp;
    this.songId = src.songId;
    this.bgImageId = src.bgImageId;
    this.neighborw = src.neighborw;
    this.neighbore = src.neighbore;
    this.neighborn = src.neighborn;
    this.neighbors = src.neighbors;
  }
  
  _decode(src, constants) {
    this._init(constants);
    
    const cellsLength = constants.COLC * constants.ROWC;
    if (src.length < cellsLength) {
      throw new Error(`Illegal length ${c} for map`);
    }
    this.cells.set(new Uint8Array(src.buffer, src.byteOffset, cellsLength));
    
    this.commands = new Uint8Array(src.buffer, src.byteOffset + cellsLength, src.length - cellsLength);
    this._readCommands(this.commands, (opcode, v, argp, argc) => {
      switch (opcode) {
      
        case 0x01: this.dark = 1; break;
        case 0x02: this.indoors = 1; break;
        case 0x03: this.blowback = 1; break;
        case 0x04: this.ancillary = 1; break;
        case 0x20: this.songId = v[argp]; break;
        case 0x21: this.bgImageId = v[argp]; break;
        case 0x22: this.herostartp = v[argp]; break;
        case 0x23: this.wind = v[argp]; break;
        case 0x40: this.neighborw = (v[argp] << 8) | v[argp + 1]; break;
        case 0x41: this.neighbore = (v[argp] << 8) | v[argp + 1]; break;
        case 0x42: this.neighborn = (v[argp] << 8) | v[argp + 1]; break;
        case 0x43: this.neighbors = (v[argp] << 8) | v[argp + 1]; break;
        
        case 0x44: this.doors.push({ // transmogrify
            x: v[argp] % constants.COLC,
            y: Math.floor(v[argp] / constants.COLC),
            mapId: 0,
            dstx: v[argp + 1] & 0xc0,
            dsty: v[argp + 1] & 0x3f,
            extra: 0,
          });
          break;
          
        case 0x60: this.doors.push({ // regular door
            x: v[argp] % constants.COLC,
            y: Math.floor(v[argp] / constants.COLC),
            mapId: (v[argp + 1] << 8) | v[argp + 2],
            dstx: v[argp + 3] % constants.COLC,
            dsty: Math.floor(v[argp + 3] / constants.COLC),
            extra: 0,
          });
          break;
          
        case 0x61: this.sketches.push({
            x: v[argp] % constants.COLC,
            y: Math.floor(v[argp] / constants.COLC),
            bits: (v[argp + 1] << 16) | (v[argp + 2] << 8) | v[argp + 3],
          });
          break;
          
        case 0x62: this.doors.push({ // buried_treasure
            x: v[argp] % constants.COLC,
            y: Math.floor(v[argp] / constants.COLC),
            mapId: 0,
            dstx: 0x30,
            dsty: v[argp + 3],
            extra: (v[argp + 1] << 8) | v[argp + 2],
          });
          break;
          
        case 0x63: this.callbacks.push({
            evid: v[argp],
            cbid: (v[argp + 1] << 8) | v[argp + 2],
            param: v[argp + 3],
          });
          break;
          
        case 0x64: this.doors.push({ // event_trigger
            x: v[argp] % constants.COLC,
            y: Math.floor(v[argp] / constants.COLC),
            mapId: 0,
            dstx: 0x20,
            dsty: 0,
            extra: (v[argp + 1] << 8) | v[argp + 2],
          });
          break;
          
        case 0x80: this.sprites.push({
            x: v[argp] % constants.COLC,
            y: Math.floor(v[argp] / constants.COLC),
            spriteId: (v[argp + 1] << 8) | v[argp + 2],
            arg0: v[argp + 3],
            arg1: v[argp + 4],
            arg2: v[argp + 5],
          });
          break;
          
        case 0x81: this.doors.push({ // buried_door
            x: v[argp] % constants.COLC,
            y: Math.floor(v[argp] / constants.COLC),
            mapId: (v[argp + 3] << 8) | v[argp + 4],
            dstx: v[argp + 5] % constants.COLC,
            dsty: Math.floor(v[argp + 5] / constants.COLC),
            extra: (v[argp + 1] << 8) | v[argp + 2],
          });
          break;
      }
    });
  }
  
  // cb(opcode, v, argp, argc), return nonzero to stop
  _readCommands(src, cb) {
    for (let p=0; p<src.length; ) {
      const opcode = src[p++];
      if (!opcode) break; // Explicit terminator.
      let paylen = 0;
      switch (opcode & 0xe0) {
        case 0x00: paylen = 0; break;
        case 0x20: paylen = 1; break;
        case 0x40: paylen = 2; break;
        case 0x60: paylen = 4; break;
        case 0x80: paylen = 6; break;
        case 0xa0: paylen = 8; break;
        case 0xc0: {
            if (p >= src.length) throw new Error(`Unexpected EOF in map`);
            paylen = src[p++];
          } break;
        case 0xe0: throw new Error(`Unknown opcode 0x${opcode.toString(16)} in map`);
      }
      if (paylen > src.length - p) throw new Error(`Unexpected EOF in map`);
      const err = cb(opcode, src, p, paylen);
      if (err) return err;
      p += paylen;
    }
  }
  
  regionContainsPassableCell(x, y, w, h, includeHoles) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > 20) w = 20 - x; //XXX hard-coded map size
    if (y + h > 12) h = 12 - y;
    if ((w < 1) || (h < 1)) return false;
    if (!this.cellphysics) return true; // physics unset, everything is passable
    for (let rowp=y*20+x; h-->0; rowp+=20) {
      for (let p=rowp, xi=w; xi-->0; p++) {
        switch (this.cellphysics[this.cells[p]]) { // XXX hard-coded FMN_CELLPHYSICS
          // Truly vacant, no doubt:
          case 0x00:
          case 0x03:
            return true;
          // Water and hole, conditional:
          case 0x02:
          case 0x07:
            if (includeHoles) return true;
            break;
          // Others are all solid.
        }
      }
    }
    return false;
  }
}
