/* DataService.js
 * Provides static assets and saved state.
 * Similar to saved state, we are also the live global repository for plants and sketches.
 * (the game proper is only aware of plants and sketches on screen).
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
 *   TODO gs
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
import { FullmoonMap } from "./FullmoonMap.js";

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
    this.plants = [];
    this.sketches = [];
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
    if (!id) return null;
    if (!(this.toc instanceof Array)) return null;
    const res = qualifier
      ? this.toc.find(t => ((t.type === type) && (t.id === id) && (t.q === qualifier)))
      : this.toc.find(t => ((t.type === type) && (t.id === id)));
    if (!res) return null;
    if (!res.obj) res.obj = this._decodeResource(res);
    return res.obj;
  }
  
  // Return nonzero to stop iteration, and return that resource again.
  forEachOfType(type, qualifier, cb) {
    type = this.resolveType(type);
    if (!(this.toc instanceof Array)) return null;
    if (!cb) throw new Error(`DataService.forEachOfType requires a callback. Did you forget qualifier?`);
    for (const res of this.toc) {
      if (res.type !== type) continue;
      if (qualifier && (res.qualifier !== qualifier)) continue;
      if (!res.obj) res.obj = this._decodeResource(res);
      if (cb(res.obj)) return res.obj;
    }
  }
  
  resolveType(type) {
    if (typeof(type) === "string") switch (type) {
      case "image": return RESTYPE_IMAGE;
      case "song": return RESTYPE_SONG;
      case "map": return RESTYPE_MAP;
      case "tileprops": return RESTYPE_TILEPROPS;
      case "sprite": return RESTYPE_SPRITE;
      case "string": return RESTYPE_STRING;
      case "instrument": return RESTYPE_INSTRUMENT;
      case "sound": return RESTYPE_SOUND;
    }
    return type;
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
      case RESTYPE_SOUND:
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
  
  _decodeMap(src, id) {
    const map = new FullmoonMap(src, this.constants);
    map.id = id;
    map.cellphysics = this.getTileprops(map.bgImageId);
    return map;
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
  
  dropSavedGame() {
    this.savedGame = null;
    this.savedGameId = null;
  }
  
  /* Plants and sketches.
   **************************************************************/
   
  dropGameState() {
    this.plants = [];
    this.sketches = [];
  }
   
  updateSketch(sketch) {
    for (let i=this.sketches.length; i-->0; ) {
      const existing = this.sketches[i];
      if (sketch.mapId !== existing[0]) continue;
      if (sketch.x !== existing[1]) continue;
      if (sketch.y !== existing[2]) continue;
      if (sketch.bits) {
        existing[3] = sketch.bits;
      } else {
        this.sketches.splice(i, 1);
      }
      return;
    }
    if (!sketch.bits) return;
    const time = 0;//TODO
    const record = [sketch.mapId, sketch.x, sketch.y, sketch.bits, time];
    this.sketches.push(record);
  }
  
  updatePlant(plant) {
    for (const existing of this.plants) {
      if (plant.mapId !== existing[0]) continue;
      if (plant.x !== existing[1]) continue;
      if (plant.y !== existing[2]) continue;
      existing[3] = plant.flower_time;
      existing[4] = plant.state;
      existing[5] = plant.fruit;
      return;
    }
    const record = [plant.mapId, plant.x, plant.y, plant.flower_time, plant.state, plant.fruit];
    this.plants.push(record);
  }
  
  removePlant(plant) {
    for (let i=this.plants.length; i-->0; ) {
      const existing = this.plants[i];
      if (plant.mapId !== existing[0]) continue;
      if (plant.x !== existing[1]) continue;
      if (plant.y !== existing[2]) continue;
      this.plants.splice(i, 1);
      return;
    }
  }
}

DataService.singleton = true;
