/* FullmoonMap.js
 * "Map" to its friends, but that is already a JS class name.
 */
 
import * as FMN from "./Constants.js";
 
export class FullmoonMap {
  constructor(src) {
    // Caller should assign:
    this.id = 0;
    this.cellphysics = null; // Uint8Array(256), ie Tileprops resource
    
    if (!src) this._init();
    else if (src instanceof FullmoonMap) this._copy(src);
    else if (src instanceof Uint8Array) this._decode(src);
    else if (src instanceof ArrayBuffer) this._decode(new Uint8Array(src));
    else throw new Error(`Unsuitable input for FullmoonMap`);
  }
  
  // cb(opcode, v, argp, argc), return nonzero to stop
  forEachCommand(cb) {
    return this._readCommands(this.commands, cb);
  }
  
  getCommandLocation(opcode, argv, argp, argc) {
    const mapw = FMN.COLC;
    const maph = FMN.ROWC;
    const hmapw = mapw >> 1;
    const hmaph = maph >> 1;
    switch (opcode) {
      // It's weird to ask this about NEIGHBOR commands, but let's be coherent about it:
      case 0x40: return [hmapw - mapw, hmaph];
      case 0x41: return [hmapw + mapw, hmaph];
      case 0x42: return [hmapw, hmaph - maph];
      case 0x43: return [hmapw, hmaph + maph];
      // Every positioned command so far stores its position packed in the first byte:
      case 0x44: // TRANSMOGRIFY
      case 0x45: // HERO
      case 0x60: // DOOR
      case 0x61: // SKETCH
      case 0x80: // SPRITE
      case 0x64: // EVENT_TRIGGER
          return [argv[argp] % mapw, Math.floor(argv[argp] / mapw)];
    }
    return [hmapw, hmaph]; // Middle of screen, for commands without a location.
  }
  
  _init() {
    this.cells = new Uint8Array(FMN.COLC * FMN.ROWC);
    this.commands = []; // Normally a Uint8Array pointing into the original source.
    this.doors = []; // {x,y,mapId,dstx,dsty,extra}
    this.sprites = []; // {x,y,spriteId,arg0,arg1,arg2}
    this.sketches = []; // {x,y,bits}. Should only be examined if no user sketches exist at load.
    this.callbacks = []; // {evid,cbid,param}
    this.cellphysics = null; // Uint8Array(256), but supplied by our owner
    this.flag = 0; // bits, FMN.MAPFLAG_*
    this.wind = 0;
    this.herostartp = (FMN.ROWC >> 1) * FMN.COLC + (FMN.COLC >> 1);
    this.spellid = 0;
    this.songId = 0;
    this.bgImageId = 0;
    this.neighborw = 0;
    this.neighbore = 0;
    this.neighborn = 0;
    this.neighbors = 0;
    this.facedir_gsbit = [0, 0]; // [horz, vert]
    this.saveto = 0;
  }
  
  _copy(src) {
    this.cells = new Uint8Array(src.cells);
    this.commands = new Uint8Array(src.commands);
    this.doors = src.doors.map(d => ({ ...d }));
    this.sprites = src.sprites.map(s => ({ ...s }));
    this.sketches = src.sketches.map(s => ({ ...s }));
    this.callbacks = src.callbacks.map(cb => ({ ...cb }));
    this.cellphysics = src.cellphysics; // null or a globally shared Tileprops, no need to copy.
    this.flag = src.flag;
    this.wind = src.wind;
    this.herostartp = src.herostartp;
    this.spellid = src.spellid;
    this.songId = src.songId;
    this.bgImageId = src.bgImageId;
    this.neighborw = src.neighborw;
    this.neighbore = src.neighbore;
    this.neighborn = src.neighborn;
    this.neighbors = src.neighbors;
    this.facedir_gsbit = [...src.facedir_gsbit];
    this.saveto = src.saveto;
  }
  
  _decode(src) {
    this._init();
    
    const cellsLength = FMN.COLC * FMN.ROWC;
    if (src.length < cellsLength) {
      throw new Error(`Illegal length ${c} for map`);
    }
    this.cells.set(new Uint8Array(src.buffer, src.byteOffset, cellsLength));
    
    this.commands = new Uint8Array(src.buffer, src.byteOffset + cellsLength, src.length - cellsLength);
    this._readCommands(this.commands, (opcode, v, argp, argc) => {
      switch (opcode) {
      
        case 0x20: this.songId = v[argp]; break;
        case 0x21: this.bgImageId = v[argp]; break;
        case 0x22: this.saveto = v[argp]; break;
        case 0x23: this.wind = v[argp]; break;
        case 0x24: this.flag = v[argp]; break;
        case 0x40: this.neighborw = (v[argp] << 8) | v[argp + 1]; break;
        case 0x41: this.neighbore = (v[argp] << 8) | v[argp + 1]; break;
        case 0x42: this.neighborn = (v[argp] << 8) | v[argp + 1]; break;
        case 0x43: this.neighbors = (v[argp] << 8) | v[argp + 1]; break;
        case 0x45: this.herostartp = v[argp]; this.spellid = v[argp + 1]; break;
        
        case 0x44: this.doors.push({ // transmogrify
            x: v[argp] % FMN.COLC,
            y: Math.floor(v[argp] / FMN.COLC),
            mapId: 0,
            dstx: v[argp + 1] & 0xc0,
            dsty: v[argp + 1] & 0x3f,
            extra: 0,
          });
          break;
          
        case 0x60: { // regular door
            const door = {
              x: v[argp] % FMN.COLC,
              y: Math.floor(v[argp] / FMN.COLC),
              mapId: (v[argp + 1] << 8) | v[argp + 2],
              dstx: v[argp + 3] % FMN.COLC,
              dsty: Math.floor(v[argp + 3] / FMN.COLC),
              extra: 0,
            };
            if (v[argp + 3] === 0xff) door.dstx = door.dsty = -1;
            this.doors.push(door);
          } break;
          
        case 0x61: this.sketches.push({
            x: v[argp] % FMN.COLC,
            y: Math.floor(v[argp] / FMN.COLC),
            bits: (v[argp + 1] << 16) | (v[argp + 2] << 8) | v[argp + 3],
          });
          break;
          
        case 0x62: this.doors.push({ // buried_treasure
            x: v[argp] % FMN.COLC,
            y: Math.floor(v[argp] / FMN.COLC),
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
            x: v[argp] % FMN.COLC,
            y: Math.floor(v[argp] / FMN.COLC),
            mapId: 0,
            dstx: 0x20,
            dsty: 0,
            extra: (v[argp + 1] << 8) | v[argp + 2],
          });
          break;
          
        case 0x65: {
            this.facedir_gsbit[0] = (v[argp] << 8) | v[argp + 1];
            this.facedir_gsbit[1] = (v[argp + 2] << 8) | v[argp + 3];
          } break;
          
        case 0x80: this.sprites.push({
            x: v[argp] % FMN.COLC,
            y: Math.floor(v[argp] / FMN.COLC),
            spriteId: (v[argp + 1] << 8) | v[argp + 2],
            arg0: v[argp + 3],
            arg1: v[argp + 4],
            arg2: v[argp + 5],
          });
          break;
          
        case 0x81: { // buried_door
            const door = {
              x: v[argp] % FMN.COLC,
              y: Math.floor(v[argp] / FMN.COLC),
              mapId: (v[argp + 3] << 8) | v[argp + 4],
              dstx: v[argp + 5] % FMN.COLC,
              dsty: Math.floor(v[argp + 5] / FMN.COLC),
              extra: (v[argp + 1] << 8) | v[argp + 2],
            };
            if (v[argp + 5] === 0xff) door.dstx = door.dsty = -1;
            this.doors.push(door);
          } break;
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
    if (x + w > FMN.COLC) w = FMN.COLC - x;
    if (y + h > FMN.ROWC) h = FMN.ROWC - y;
    if ((w < 1) || (h < 1)) return false;
    if (!this.cellphysics) return true; // physics unset, everything is passable
    for (let rowp=y*FMN.COLC+x; h-->0; rowp+=FMN.COLC) {
      for (let p=rowp, xi=w; xi-->0; p++) {
        switch (this.cellphysics[this.cells[p]]) { // XXX hard-coded FMN.CELLPHYSICS
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
