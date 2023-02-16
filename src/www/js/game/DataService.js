/* DataService.js
 * Provides static assets and saved state.
 *
 * Saved game:
 * null is valid, meaning no save file select.
 * The empty object is valid, and should mean the initial state.
 * It's stored in localStorage as base64-encoded JSON.
 * {
 *   time: u32 // active play time in ms, overflows every 1200 hours or so
 *   timeOverflow: boolean // if true, (time) is still valid, but should represent saturated eg "999:59:59"
 *   items: u8[] // value is FMN_ITEM_*, which ones we have
 *   itemQualifiers: u8[] // indexed by FMN_ITEM_*
 *   plants: [mapid,x,y,flowerTime,state,fruit][]
 *   sketches: [mapid,x,y,bits,time][]
 *   pos: [mapid,x,y] // hero's position
 *   selectedItem: u8
 * }
 *
 * map: {
 *   cells: Uint8Array(COLC * ROWC)
 *   bgImageId
 *   songId
 *   neighborw
 *   neighbore
 *   neighborn
 *   neighbors
 *   doors: {x,y,mapId,dstx,dsty}[]
 *   sprites: {x,y,spriteId,arg0,arg1,arg2}[]
 *   cellphysics: Uint8Array(256) | null
 * }
 */
 
import { Constants } from "./Constants.js";
import { Song } from "../synth/Song.js";
import { Instrument } from "../synth/Instrument.js";
import { Sound } from "../synth/Sound.js";

const RESTYPE_IMAGE = 0x01;
const RESTYPE_SONG = 0x02;
const RESTYPE_MAP = 0x03;
const RESTYPE_TILEPROPS = 0x04;
const RESTYPE_SPRITE = 0x05;
const RESTYPE_STRING = 0x06;
const RESTYPE_INSTRUMENT = 0x07;
const RESTYPE_SOUND = 0x08;
 
export class DataService {
  static getDependencies() {
    return [Window, Constants];
  }
  constructor(window, constants) {
    this.window = window;
    this.constants = constants;
    
    this.textDecoder = new this.window.TextDecoder("utf-8");
    this.toc = null; // null, Array, Promise, or Error. Array: { type, id, q, ser, obj }
    this.savedGame = null;
    this.savedGameId = null;
  }
  
  /* Access to resources.
   * These are all synchronous.
   * Once the service is loaded, all resources are available immediately or not at all.
   *****************************************************************/
   
  getMap(id) { return this.getResource(RESTYPE_MAP, id); }
  getSprite(id) { return this.getResource(RESTYPE_SPRITE, id); }
  getImage(id) { return this.getResource(RESTYPE_IMAGE, id); } // TODO qualifier
  getTileprops(id) { return this.getResource(RESTYPE_TILEPROPS, id); }
  getSong(id) { return this.getResource(RESTYPE_SONG, id); }
  getInstrument(id) { return this.getResource(RESTYPE_INSTRUMENT, id); }
  getString(id) { return this.getResource(RESTYPE_STRING, id); } // TODO qualifier
  getSound(id) { return this.getResource(RESTYPE_SOUND, id); }
  
  getResource(type, id, qualifier) {
    if (!(this.toc instanceof Array)) return null;
    const res = qualifier
      ? this.toc.find(t => ((t.type === type) && (t.id === id) && (t.q === qualifier)))
      : this.toc.find(t => ((t.type === type) && (t.id === id)));
    if (!res) return null;
    if (!res.obj) res.obj = this._decodeResource(res);
    return res.obj;
  }
  
  /* Load data.
   ******************************************************************/
   
  refresh() {
    this.toc = null;
    return this.load();
  }
   
  load() {
    if (!this.toc) this.toc = this._beginLoad();
    if (this.toc instanceof Array) return Promise.resolve(this.toc);
    if (this.toc instanceof Promise) return this.toc;
    return Promise.reject(this.toc);
  }
  
  _beginLoad() {
    return this.window.fetch("./fullmoon.data")
      .then(rsp => { if (!rsp.ok) return rsp.json().then(t => { throw t.log; }); return rsp.arrayBuffer(); })
      .then(serial => this._receiveArchive(serial))
      .then(() => this.toc)
      .catch(e => {
        if (!(e instanceof Error)) e = new Error(e);
        this.toc = e;
        throw e;
      });
  }
  
  _receiveArchive(serial) {
    const src = new Uint8Array(serial);
    this.toc = [];
    if (src.length < 12) throw new Error(`Short archive ${src.length}<12`);
    if ((src[0] !== 0xff) || (src[1] !== 0x41) || (src[2] !== 0x52) || (src[3] !== 0x00)) {
      throw new Error(`Archive signature mismatch`);
    }
    const entryCount = (src[4] << 24) | (src[5] << 16) | (src[6] << 8) | src[7];
    const addlLen = (src[8] << 24) | (src[9] << 16) | (src[10] << 8) | src[11];
    const tocStart = 12 + addlLen;
    const dataStart = tocStart + entryCount * 4;
    
    let qualified = true;
    let nextType = 0, nextQualifier = 0, nextId = 1, prev = null;
    for (let tocp=tocStart, i=entryCount; i-->0; tocp+=4) {
      if (src[tocp] & 0x80) {
        nextType = src[tocp + 1];
        nextQualifier = (src[tocp + 2] << 8) | src[tocp + 3];
        nextId = 1;
        qualified = this.shouldRetainResources(nextType, nextQualifier);
      } else {
        if (!nextType) throw new Error(`Expected nonzero State Change in TOC around ${tocp}/${src.length}`);
        const offset = (src[tocp] << 24) | (src[tocp + 1] << 16) | (src[tocp + 2] << 8) | src[tocp + 3];
        if (prev) {
          if (offset < prev.p) throw new Error(`Archive offsets out of order, ${offset}<${prev.p}`);
          const len = offset - prev.p;
          if (len > 0) {
            const srcView = new Uint8Array(serial, prev.p, len);
            prev.ser = new Uint8Array(len);
            prev.ser.set(srcView);
          }
        }
        if (qualified) {
          prev = {
            type: nextType,
            q: nextQualifier,
            id: nextId,
            p: offset,
          };
          this.toc.push(prev);
        } else {
          prev = null;
        }
        nextId++;
      }
    }
    if (prev) {
      const len = src.length - prev.p;
      if (len > 0) {
        const srcView = new Uint8Array(serial, prev.p, len);
        prev.ser = new Uint8Array(len);
        prev.ser.set(srcView);
      }
    }
  }
  
  /* Opportunity to filter early based on type and qualifier.
   * We do retain qualifier in the live TOC, so we can filter on it on the fly too.
   * This pass is only for resources we definitely will never need. (TODO: Well, don't put them in the archive then, brainiac)
   */
  shouldRetainResources(type, qualifier) {
    switch (type) {
      case RESTYPE_INSTRUMENT: switch (qualifier) {
          case 1: return true; // WebAudio
          default: return false;
        }
    }
    return true;
  }
  
  /* Decode one resource.
   * This happens lazy, the first time somebody requests the specific resource.
   **************************************************************/
   
  _decodeResource(res) {
    switch (res.type) {
      case RESTYPE_IMAGE: return this._decodeImage(res.ser, res.id);
      case RESTYPE_MAP: return this._decodeMap(res.ser, res.id);
      case RESTYPE_SPRITE: return this._decodeSprite(res.ser, res.id);
      case RESTYPE_SONG: return this._decodeSong(res.ser, res.id);
      case RESTYPE_STRING: return this._decodeString(res.ser, res.id);
      case RESTYPE_INSTRUMENT: return this._decodeInstrument(res.ser, res.id);
      case RESTYPE_TILEPROPS: return this._decodeTileprops(res.ser, res.id);
      case RESTYPE_SOUND: return this._decodeSound(res.ser, res.id);
    }
    throw new Error(`Unexpected resource type ${res.type}`);
  }
  
  _decodeImage(src, id) {
    const image = new Image();
    const blob = new Blob([src], { type: "image/png" });
    const url = URL.createObjectURL(blob);
    // NB we never call revokeObjectURL because we have a finite set of images, which should all remain loaded.
    image.src = url;
    return image;
  }
  
  //TODO Maps deserve a proper class.
  _decodeMap(src, mapId) {
    let p = 0, c = src.length;
    const cellsLength = this.constants.COLC * this.constants.ROWC;
    if (c < cellsLength) {
      throw new Error(`Illegal length ${c} for map ${mapId}`);
    }
    const map = {
      cells: new Uint8Array(cellsLength),
      doors: [],
      sprites: [],
      cellphysics: null,
    };
    map.cells.set(new Uint8Array(src.buffer, src.byteOffset + p, cellsLength));
    p += cellsLength;
    c -= cellsLength;
    while (c > 0) {
      const opcode = src[p++]; c--;
      if (!opcode) break; // Explicit commands terminator.
      let paylen = 0;
      switch (opcode & 0xe0) {
        case 0x00: paylen = 0; break;
        case 0x20: paylen = 1; break;
        case 0x40: paylen = 2; break;
        case 0x60: paylen = 4; break;
        case 0x80: paylen = 6; break;
        case 0xa0: paylen = 8; break;
        case 0xc0: if (!c) throw new Error(`Unexpected EOF in map ${mapId}`); paylen = src[p++]; c--; break;
        case 0xe0: throw new Error(`Unknown opcode 0x${opcode.toString(16)} in map ${mapId}`);
      }
      if (paylen > c) throw new Error(`Unexpected EOF in map ${mapId}`);
      this._decodeMapCommand(map, opcode, src, p, paylen);
      p += paylen;
      c -= paylen;
    }
    map.cellphysics = this.getTileprops(map.bgImageId);
    return map;
  }
  
  _decodeMapCommand(map, opcode, src, p, c) {
    switch (opcode) {
      case 0x20: map.songId = src[p]; break;
      case 0x21: map.bgImageId = src[p]; break;
      case 0x40: map.neighborw = (src[p] << 8) | src[p + 1]; break;
      case 0x41: map.neighbore = (src[p] << 8) | src[p + 1]; break;
      case 0x42: map.neighborn = (src[p] << 8) | src[p + 1]; break;
      case 0x43: map.neighbors = (src[p] << 8) | src[p + 1]; break;
      case 0x60: map.doors.push({
          x: src[p] % this.constants.COLC,
          y: Math.floor(src[p] / this.constants.COLC),
          mapId: (src[p + 1] << 8) | src[p + 2],
          dstx: src[p + 3] % this.constants.COLC,
          dsty: Math.floor(src[p + 3] / this.constants.COLC),
        }); break;
      case 0x80: map.sprites.push({
          x: src[p] % this.constants.COLC,
          y: Math.floor(src[p] / this.constants.COLC),
          spriteId: (src[p + 1] << 8) | src[p + 2],
          arg0: src[p + 3],
          arg1: src[p + 4],
          arg2: src[p + 5],
        }); break;
    }
  }
  
  _decodeSprite(src, id) {
    return src;
  }
  
  _decodeSong(src, id) {
    return new Song(src);
  }
  
  _decodeString(src, id) {
    return this.textDecoder.decode(src);
  }
  
  _decodeInstrument(src, id) {
    return new Instrument(src, id);
  }
  
  _decodeTileprops(src, id) {
    return src;
  }
  
  _decodeSound(src, id) {
    return new Sound(src, id, this.constants.AUDIO_FRAME_RATE);
  }
  
  /* Saved game.
   *********************************************************/
   
  // Resolves when loaded. For now it's synchronous but I don't expect that to remain true forever.
  loadSavedGame(id) {
    this.savedGameId = id;
    if (!id) {
      this.savedGame = {};
      return Promise.resolve();
    }
    this.savedGame = null;
    const key = this.localStorageKeyFromSavedGameId(id);
    const serial = this.window.localStorage.getItem(key);
    if (serial) {
      try {
        this.savedGame = JSON.parse(atob(serial));
      } catch (error) {
        console.error(`Failed to decode saved game ${id}`, { serial, error, key });
        this.savedGame = {};
      }
    } else {
      this.savedGame = {};
    }
    return Promise.resolve();
  }
  
  storeSavedGame(id) {
    if (!id) {
      id = this.savedGameId;
      if (!id) return Promise.reject("Saved game ID required");
    }
    if (!this.savedGame) this.savedGame = {};
    this.savedGameId = id;
    return new Promise((resolve, reject) => {
      try {
        const key = this.localStorageKeyFromSavedGameId(id);
        const serial = btoa(JSON.stringify(this.savedGame));
        this.window.localStorage.setItem(key, serial);
      } catch (e) {
        reject(e);
        return;
      }
      resolve();
    });
  }
  
  localStorageKeyFromSavedGameId(id) {
    if (!id) throw new Error("Invalid saved game ID");
    return `savedGame[${id}]`;
  }
  
  listSavedGames() {
    const ids = new Set();
    for (let i=this.window.localStorage.length; i-->0; ) {
      const key = this.window.localStorage.key(i);
      const match = key.match(/^savedGame[(.*)]$/);
      if (!match) continue;
      if (!match[1]) continue;
      ids.add(match[1]);
    }
    return Promise.resolve(Array.from(ids));
  }
}

DataService.singleton = true;
