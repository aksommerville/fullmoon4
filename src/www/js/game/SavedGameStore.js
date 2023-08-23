/* SavedGameStore.js
 * Interacts with localStorage for saved games, and encode/decode against global state.
 */
 
import { DataService } from "./DataService.js";
import { Globals } from "./Globals.js";
import * as FMN from "./Constants.js";
import { Clock } from "./Clock.js";
import { Encoder } from "../util/Encoder.js";
import { Decoder } from "../util/Decoder.js";
import { WasmLoader } from "../util/WasmLoader.js";

export class SavedGameStore {
  static getDependencies() {
    return [Window, DataService, Globals, Clock, WasmLoader];
  }
  constructor(window, dataService, globals, clock, wasmLoader) {
    this.window = window;
    this.dataService = dataService;
    this.globals = globals;
    this.clock = clock;
    this.wasmLoader = wasmLoader;
    
    this.dirtyDebounceTimeMs = 2000;
    this.localStorageKey = "savedGame";
    this.dirty = false;
    this.updateTime = 0;
  }
  
  /* "Public" API. (But our only consumer should be Runtime).
   * Mostly these correspond to platform hooks.
   * We have an update method, which Runtime should spam, and we'll save when the time feels right.
   ************************************************************************/
  
  hasSavedGame() {
    return this.window.localStorage.getItem(this.localStorageKey) ? 1 : 0;
  }
  
  loadSavedGame() {
    const serial = this.window.localStorage.getItem(this.localStorageKey);
    if (!serial) return 0;
    try {
      const savedGame = this._decode(serial);
      const mapId = this._apply(savedGame);
      return mapId;
    } catch (e) {
      this.window.console.error(`Error loading saved game.`, e);
      return 0;
    }
  }
  
  deleteSavedGame() {
    this.window.localStorage.removeItem(this.localStorageKey);
  }
  
  setDirty() {
    if (this.dirty) return;
    this.dirty = true;
    this.updateTime = Date.now() + this.dirtyDebounceTimeMs;
  }
  
  update(gameTimeMs, cbPrep) {
    if (!this.dirty) return;
    if (Date.now() < this.updateTime) return;
    this.dirty = false; // even if it fails, don't retry until the next dirty (they'll be frequent)
    try {
      if (cbPrep) cbPrep();
      const savedGame = this._extract(gameTimeMs);
      const serial = this._encode(savedGame);
      this.window.localStorage.setItem(this.localStorageKey, serial);
    } catch (e) {
      this.window.console.error(`Error saving game.`, e);
    }
  }
  
  /* Private.
   * We deal in two types: savedGame and serial (see etc/doc/saved-game.md, but base64-encoded).
   * savedGame: {
   *   spellId: u8
   *   gameTimeMs: u32
   *   damageCount: u16
   *   transmogrification: u8
   *   selectedItem: u8
   *   itemBits: u16 (little-endian bits)
   *   itemQualifiers: Uint8Array[16]
   *   gs: Uint8Array
   *   plants: {
   *     mapId: u16
   *     x: u8
   *     y: u8
   *     state: u8
   *     fruit: u8
   *     flowerTime: u32
   *   }[]
   *   sketches: {
   *     mapId: u16
   *     x: u8
   *     y: u8
   *     bits: u24
   *   }[]
   * }
   **************************************************************************/
   
  _encode(savedGame) {
    const encoder = new Encoder();
    this._encodeHeader(encoder, savedGame);
    this._encodeGs(encoder, savedGame);
    this._encodeSketches(encoder, savedGame);
    this._encodePlants(encoder, savedGame);
    return encoder.finishBase64();
  }
   
  _decode(serial) {
    const savedGame = this._blankSavedGame();
    const decoder = Decoder.fromBase64(serial);
    while (decoder.remaining()) {
      if (!this._decodeChunk(savedGame, decoder)) break;
    }
    return savedGame;
  }
  
  _extract(gameTimeMs) {
    const savedGame = {
      spellId: this.globals.recentSpellId,
      gameTimeMs,
      damageCount: this.globals.g_damage_count[0],
      transmogrification: this.globals.g_transmogrification[0],
      selectedItem: this.globals.g_selected_item[0],
      itemBits: this.globals.g_itemv.reduce((a, v, i) => (a | (v ? (1 << i) : 0)), 0),
      itemQualifiers: this.globals.g_itemqv,
      gs: this.globals.g_gs,
      plants: this.dataService.plants.map(([mapId, x, y, flowerTime, state, fruit]) => ({
        mapId, x, y, flowerTime, state, fruit,
      })),
      sketches: this.dataService.sketches.map(([mapId, x, y, bits]) => ({
        mapId, x, y, bits,
      })),
    };
    /* gameTimeMs is the platform's idea of time, which will generally be future of the reported play time.
     * Reported time does not include menus or transitions, and is known only to the game, until we ask:
     */
    const playTimeMs = this.wasmLoader.instance.exports.fmn_game_get_play_time_ms();
    if ((playTimeMs > 0) && (playTimeMs < gameTimeMs)) {
      this._adjustTimestamps(savedGame, playTimeMs - gameTimeMs);
    }
    return savedGame;
  }
  
  // in place
  _adjustTimestamps(savedGame, dms) {
    savedGame.gameTimeMs += dms;
    for (const plant of savedGame.plants) {
      plant.flowerTime += dms;
    }
  }
  
  // => mapId
  _apply(savedGame) {
    this.globals.recentSpellId = savedGame.spellId;
    this.clock.reset(savedGame.gameTimeMs);
    this.wasmLoader.instance.exports.fmn_reset_clock(savedGame.gameTimeMs);
    this.globals.g_damage_count[0] = savedGame.damageCount;
    this.globals.g_transmogrification[0] = savedGame.transmogrification;
    this.globals.g_selected_item[0] = savedGame.selectedItem;
    for (let p=0, mask=1; p<16; p++, mask<<=1) {
      this.globals.g_itemv[p] = (savedGame.itemBits & mask) ? 1 : 0;
      this.globals.g_itemqv[p] = savedGame.itemQualifiers[p];
    }
    this.globals.g_gs.fill(0);
    const gsview = new Uint8Array(this.globals.g_gs.buffer, this.globals.g_gs.byteOffset, Math.min(this.globals.g_gs.length, savedGame.gs.length));
    gsview.set(savedGame.gs);
    // Unwisely touching DataService's formats for plants and sketches, to keep things simple:
    this.dataService.plants = savedGame.plants.map(p => [
      p.mapId, p.x, p.y, p.flowerTime, p.state, p.fruit,
    ]);
    this.dataService.sketches = savedGame.sketches.map(s => [
      s.mapId, s.x, s.y, s.bits, 0,
    ]);
    const map = this.dataService.forEachOfType("map", 0, map => map.spellid === savedGame.spellId);
    return map ? map.id : 1;
  }
  
  _encodeHeader(encoder, savedGame) {
    encoder.u8(0x01);
    encoder.u8(27);
    encoder.u8(savedGame.spellId);
    encoder.u32be(savedGame.gameTimeMs);
    encoder.u16be(savedGame.damageCount);
    encoder.u8(savedGame.transmogrification);
    encoder.u8(savedGame.selectedItem);
    encoder.u16be(savedGame.itemBits);
    encoder.raw(savedGame.itemQualifiers);
  }
  
  _encodeGs(encoder, savedGame) {
    // gs is currently fixed at 64 bytes, but could change whenever.
    // GSBIT chunks can hold up to 253 bytes each.
    // Writing this the right way, so if gs grows beyond 253 we'll automatically use the appropriate chunk count.
    // (if it does get that big, maybe consider writing sparsely, we don't actually need to include the zeroes).
    let srcp = 0;
    while (srcp < savedGame.gs.length) {
      const cpc = Math.min(253, savedGame.gs.length - srcp);
      encoder.u8(0x02);
      encoder.u8(2 + cpc);
      encoder.u16be(srcp);
      encoder.raw(new Uint8Array(savedGame.gs.buffer, savedGame.gs.byteOffset + srcp, cpc));
      srcp += cpc;
    }
  }
  
  _encodeSketches(encoder, savedGame) {
    for (const sketch of savedGame.sketches) {
      encoder.u8(0x04);
      encoder.u8(6);
      encoder.u16be(sketch.mapId);
      encoder.u8(sketch.y * FMN.COLC + sketch.x);
      encoder.u24be(sketch.bits);
    }
  }
  
  _encodePlants(encoder, savedGame) {
    for (const plant of savedGame.plants) {
      encoder.u8(0x03);
      encoder.u8(9);
      encoder.u16be(plant.mapId);
      encoder.u8(plant.y * FMN.COLC + plant.x);
      encoder.u8(plant.state);
      encoder.u8(plant.fruit);
      encoder.u32be(plant.flowerTime);
    }
  }
  
  _blankSavedGame() {
    return {
      spellId: 0,
      gameTimeMs: 0,
      damageCount: 0,
      transmogrification: 0,
      selectedItem: 0,
      itemBits: 0,
      itemQualifiers: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
      gs: [], // empty array is fine, but must be Uint8Array if not empty.
      plants: [],
      sketches: [],
    };
  }
  
  // false to stop reading (explicit EOF).
  _decodeChunk(savedGame, decoder) {
    const chunkId = decoder.u8();
    if (!chunkId) return false;
    const chunkLength = decoder.u8();
    const sub = new Decoder(decoder.uint8ArrayView(chunkLength));
    switch (chunkId) {
      case 0x01: this._decodeChunk_FIXED01(savedGame, sub); break;
      case 0x02: this._decodeChunk_GSBIT(savedGame, sub); break;
      case 0x03: this._decodeChunk_PLANT(savedGame, sub); break;
      case 0x04: this._decodeChunk_SKETCH(savedGame, sub); break;
    }
    return true;
  }
  
  _decodeChunk_FIXED01(savedGame, decoder) {
    savedGame.spellId = decoder.u8();
    savedGame.gameTimeMs = decoder.u32be();
    savedGame.damageCount = decoder.u16be();
    savedGame.transmogrification = decoder.u8();
    savedGame.selectedItem = decoder.u8();
    savedGame.itemBits = decoder.u16be();
    savedGame.itemQualifiers = decoder.uint8ArrayView(16);
  }
  
  _decodeChunk_GSBIT(savedGame, decoder) {
    const offset = decoder.u16be();
    const srcc = decoder.remaining();
    if (!srcc) return;
    const minlen = offset + srcc;
    if (minlen > savedGame.gs.length) {
      const nv = new Uint8Array(minlen);
      if (savedGame.gs.length) {
        const dstview = new Uint8Array(nv.buffer, 0, savedGame.gs.length);
        dstview.set(savedGame.gs, 0);
      }
      savedGame.gs = nv;
    }
    const dstview = new Uint8Array(savedGame.gs.buffer, savedGame.gs.byteOffset + offset, srcc);
    dstview.set(decoder.uint8ArrayView(srcc));
  }
  
  _decodeChunk_PLANT(savedGame, decoder) {
    const plant = {};
    plant.mapId = decoder.u16be();
    const cellp = decoder.u8();
    plant.x = cellp % FMN.COLC;
    plant.y = Math.floor(cellp / FMN.COLC);
    plant.state = decoder.u8();
    plant.fruit = decoder.u8();
    plant.flowerTime = decoder.u32be();
    savedGame.plants.push(plant);
  }
  
  _decodeChunk_SKETCH(savedGame, decoder) {
    const sketch = {};
    sketch.mapId = decoder.u16be();
    const cellp = decoder.u8();
    sketch.x = cellp % FMN.COLC;
    sketch.y = Math.floor(cellp / FMN.COLC);
    sketch.bits = decoder.u24be();
    savedGame.sketches.push(sketch);
  }
}

SavedGameStore.singleton = true;
