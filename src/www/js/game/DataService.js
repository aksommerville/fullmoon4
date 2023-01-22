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
    this.tileprops = []; // sparse, keyed by image id 0..255. Uint8Array(256) or absent.
    this.tilepropsLoadState = null; // null, Promise, Error, or this.tileprops
    this.sprites = []; // sparse, keyed by sprite id 0..65535. Uint8Array or absent.
    this.spritesLoadState = null; // null, Promise, Error, or this.sprites
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
      image.src = `./img/${id}.png`;
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
   ***********************************************************/
   
  fetchAllMaps() {
    if (!this.mapLoadState) {
      this.mapLoadState = this.fetchAllTileprops()
        .then(() => this.window.fetch("./maps.data"))
        .then(rsp => {
          if (!rsp.ok) return rsp.json().then(t => { throw t; });
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
      this.decodeMapCommand(map, opcode, src, p, paylen);
      p += paylen;
      c -= paylen;
    }
    map.cellphysics = this.loadTileprops(map.bgImageId);
    return map;
  }
  
  decodeMapCommand(map, opcode, src, p, c) {
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
  
  /* Tileprops.
   * Works the same way as Maps, and Maps implicitly load Tileprops first.
   *********************************************************/
   
  fetchAllTileprops() {
    if (!this.tilepropsLoadState) {
      this.tilepropsLoadState = this.window.fetch("./tileprops.data")
        .then(rsp => {
          if (!rsp.ok) return rsp.json().then(t => { throw t; });
          return rsp.arrayBuffer();
        }).then(input => {
          this.tileprops = this.decodeTileprops(input);
          this.tilepropsLoadState = this.tileprops;
        }).catch(error => {
          this.tilepropsLoadState = error || new Error("Failed to load tileprops.");
          throw this.tilepropsLoadState;
        });
    }
    if (this.tilepropsLoadState instanceof Promise) return this.tilepropsLoadState;
    if (this.tilepropsLoadState === this.tileprops) return Promise.resolve();
    return Promise.reject(this.tilepropsLoadState);
  }
   
  loadTileprops(imageId) {
    return this.tileprops[imageId];
  }
  
  decodeTileprops(src) {
    src = new Uint8Array(src);
    if (src[0]) throw new Error(`Invalid leading byte ${src[0]} in tileprops archive.`);
    const tileprops = [];
    for (let imageId=1; imageId<256; imageId++) {
      const tableIndex = src[imageId];
      if (!tableIndex) continue;
      const physics = new Uint8Array(256);
      const srcView = new Uint8Array(src.buffer, tableIndex * 256, 256);
      physics.set(srcView);
      tileprops[imageId] = physics;
    }
    return tileprops;
  }
  
  /* Sprite.
   * Works the same way as Maps and Tilesheets.
   *********************************************************/
   
  fetchAllSprites() {
    if (!this.spritesLoadState) {
      this.spritesLoadState = this.window.fetch("./sprites.data")
        .then(rsp => {
          if (!rsp.ok) return rsp.json().then(t => { throw t; });
          return rsp.arrayBuffer();
        }).then(input => {
          this.sprites = this.decodeSprites(input);
          this.spritesLoadState = this.sprites;
        }).catch(error => {
          this.spritesLoadState = error || new Error("Failed to load sprites.");
          throw this.spritesLoadState;
        });
    }
    if (this.spritesLoadState instanceof Promise) return this.spritesLoadState;
    if (this.spritesLoadState === this.sprites) return Promise.resolve();
    return Promise.reject(this.spritesLoadState);
  }
   
  loadSprite(spriteId) {
    return this.sprites[spriteId];
  }
  
  decodeSprites(src) {
    src = new Uint8Array(src);
    if (src.length < 6) throw new Error(`Invalid length ${src.length} for sprites archive.`);
    if ((src[0] !== 0xff) || (src[1] !== 0x4d) || (src[2] !== 0x53) || (src[3] !== 0x70)) {
      throw new Error(`Signature mismatch in sprites archive.`);
    }
    const sprites = [];
    const lastSpriteId = (src[4] << 8) | src[5];
    if (lastSpriteId > 0) {
      const dataStart = 6 + lastSpriteId * 4;
      let spriteId=1, tocp=6, lastId=0, lastOffset=dataStart;
      for (; tocp<dataStart; tocp+=4, lastId=spriteId, spriteId++) {
        const offset = (src[tocp] << 24) | (src[tocp + 1] << 16) | (src[tocp + 2] << 8) | src[tocp + 3];
        const len = offset - lastOffset;
        if (len < 0) throw new Error(`Invalid or missorted sprite offset around id ${spriteId} (${offset} < ${lastOffset})`);
        if (lastId && len) {
          const srcView = new Uint8Array(src.buffer, lastOffset, len);
          sprites[lastId] = new Uint8Array(len);
          sprites[lastId].set(srcView);
        }
        lastOffset = offset;
      }
      if (lastId && (lastOffset < src.length)) {
        const srcView = new Uint8Array(src.buffer, lastOffset, src.length - lastOffset);
        sprites[lastId] = new Uint8Array(src.length - lastOffset);
        sprites[lastId].set(srcView);
      }
    }
    return sprites;
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
