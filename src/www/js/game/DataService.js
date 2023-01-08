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
 */
 
import { Constants } from "./Constants.js";
 
export class DataService {
  static getDependencies() {
    return [Window, Constants];
  }
  constructor(window, constants) {
    this.window = window;
    this.constants = constants;
    
    this.images = []; // sparse, keyed by image id 0..255. Image, or string for error.
    this.maps = []; // sparse, keyed by map id 0..65535.
    this.mapLoadState = null; // null, Promise, Error, or this.maps
    this.savedGame = null;
    this.savedGameId = null;
  }
  
  /* Images.
   ***************************************************************/
  
  // Resolves to Image once loaded.
  loadImage(id) {
    let result = this.images[id];
    if (result) {
      if (result instanceof Promise) return result;
      if (typeof(result) === "string") return Promise.reject(result);
      return Promise.resolve(result);
    }
    result = new Promise((resolve, reject) => {
      const image = new this.window.Image();
      image.onload = () => {
        this.images[id] = image;
        resolve(image);
      };
      image.onerror = (error) => {
        const message = `failed to load image ${id}`;
        console.error(message, error);
        this.images[id] = message;
        reject(error);
      };
      image.src = `/img/${id}.png`;
    });
    this.images[id] = result;
    return result;
  }
  
  // Null if not loaded, and we kick it off in the background if needed.
  loadImageSync(id) {
    if (this.images[id] instanceof Image) return this.images[id];
    if (!this.images[id]) this.loadImage(id);
    return null;
  }
  
  /* Maps.
   * Map loading must be synchronous because it's driven by a C hook that expects a result.
   * The asynchronous part happens up front at the initial load.
   ***********************************************************/
   
  fetchAllMaps() {
    if (!this.mapLoadState) {
      this.mapLoadState = this.window.fetch("/maps.data").then(rsp => {
        if (!rsp.ok) throw rsp;
        return rsp.arrayBuffer();
      }).then(input => {
        this.maps = this.decodeMaps(input);
        this.mapLoadState = this.maps;
      }).catch(error => {
        this.mapLoadState = error || new Error("Failed to load maps.");
        throw this.mapLoadState;
      });
    }
    if (this.mapLoadState instanceof Promise) return this.mapLoadState;
    if (this.mapLoadState === this.maps) return Promise.resolve();
    return Promise.reject(this.mapLoadState);
  }
   
  loadMap(mapId) {
    return this.maps[mapId];
  }
  
  decodeMaps(src) {
    src = new Uint8Array(src);
    if ((src.length < 6) || (src[0] !== 0xff) || (src[1] !== 0x00) || (src[2] !== 0x4d) || (src[3] !== 0x50)) {
      throw new Error(`Invalid signature or length in maps archive.`);
    }
    const lastMapId = (src[4] << 8) | src[5];
    const tocLength = lastMapId * 4;
    const dataStart = 6 + tocLength;
    console.log(`lastMapId=${lastMapId} tocLength=${tocLength} dataStart=${dataStart} src.length=${src.length}`);
    if (dataStart > src.length) {
      throw new Error(`Maps archive TOC overruns file`);
    }
    const toc = []; // [p,c]
    // First read the TOC verbatim, offsets only.
    for (let mapId=1, tocp=6; mapId<=lastMapId; mapId++, tocp+=4) {
      toc.push([(src[tocp] << 24) | (src[tocp+1] << 16) | (src[tocp+2] << 8) | src[tocp+3], 0]);
    }
    // Now run thru it again: Validate offsets and calculate lengths.
    for (let i=0; i<toc.length; i++) {
      const entry = toc[i];
      if (entry[0] < dataStart) throw new Error(`Illegal maps archive TOC entry [${i+1}]=${entry[0]}`);
      if (i < toc.length - 1) {
        entry[1] = toc[i+1][0] - entry[0];
      } else {
        entry[1] = src.length - entry[0];
      }
      if (entry[1] < 0) throw new Error(`Illegal length ${entry[1]} for maps archive TOC entry ${i+1}`);
    }
    const maps = [null]; // map zero is reserved; the TOC starts at one.
    for (const [p, c] of toc) {
      if (c) maps.push(this.decodeMap(src, p, c, maps.length));
      else maps.push(null);
    }
    return maps;
  }
  
  decodeMap(src, p, c, mapId) {
    const cellsLength = this.constants.COLC * this.constants.ROWC;
    if (c < cellsLength) {
      throw new Error(`Illegal length ${c} for map ${mapId}`);
    }
    const map = {
      cells: new Uint8Array(cellsLength),
      bgImageId: 1,//TODO
    };
    map.cells.set(new Uint8Array(src.buffer, src.byteOffset + p, cellsLength));
    //TODO additional map content
    return map;
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
