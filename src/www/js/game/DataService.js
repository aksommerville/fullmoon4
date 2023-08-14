/* DataService.js
 * Provides static assets and certain global state.
 * We are the live global repository for plants and sketches.
 * (the game proper is only aware of plants and sketches on screen).
 */
 
import * as FMN from "./Constants.js";
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
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.textDecoder = new this.window.TextDecoder("utf-8");
    this.toc = null; // null, Array, Promise, or Error. Array: { type, id, q, ser, obj }
    this.languages = []; // string resource qualifiers as 16-bit big-endian integers, extracted at load as an optimization
    this.language = this.languageFromIsoString(this.window.navigator.language) || 0x656e;
    this.plants = [];
    this.sketches = [];
    this.mapCount = 0;
  }
  
  languageFromIsoString(src) {
    if (!src || (src.length < 2)) return null;
    src = src.toLowerCase();
    const a = src.charCodeAt(0);
    const b = src.charCodeAt(1);
    if ((a < 0x61) || (a > 0x7a) || (b < 0x61) || (b > 0x7a)) return null;
    return (a << 8) | b;
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
  getString(id) { return this.getResource(RESTYPE_STRING, id, this.language); }
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
    this.languages = [];
    this.mapCount = 0;
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
    this.languages = [];
    this.mapCount = 0;
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
        if (nextType === RESTYPE_MAP) this.mapCount++;
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
          if ((nextType === RESTYPE_STRING) && (this.languages[this.languages.length - 1] !== nextQualifier)) {
            this.languages.push(nextQualifier);
          }
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
    if (this.languages.length && (this.languages.indexOf(this.language) < 0)) {
      // Preferred language is not in our resource set. If we have English, use that. Otherwise whatever's first.
      if (this.languages.indexOf(0x656e) >= 0) this.language = 0x656e;
      else this.language = this.languages[0];
    }
    this._preloadAsNeeded();
  }
  
  /* Immediately after populating toc, an opportunity to decode resources in advance of usage.
   * This matters for images! We need to load them all before the game starts.
   */
  _preloadAsNeeded() {
    for (const res of this.toc) {
      if (res.type === RESTYPE_IMAGE) {
        res.obj = this._decodeResource(res);
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
    return new Sound(src, id, FMN.AUDIO_FRAME_RATE);
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
