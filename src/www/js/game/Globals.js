/* Globals.js
 * Access to the global memory shared with C app.
 * Platform services should avoid accessing memory direct from WasmLoader.
 * Use me instead, to keep all the addresses and such straight.
 */
 
import { WasmLoader } from "../util/WasmLoader.js";
import { Constants } from "./Constants.js";
 
export class Globals {
  static getDependencies() {
    return [WasmLoader, Constants];
  }
  constructor(wasmLoader, constants) {
    this.wasmLoader = wasmLoader;
    this.constants = constants;
    
    this.p_fmn_global = 0;
    this.memU8 = null;
    this.memU16 = null;
    this.memU32 = null;
    this.memF32 = null;
  }
  
  /* Call whenever wasmLoader reloads.
   */
  refresh() {
    
    // Yoink a few things straight off WasmLoader.
    this.p_fmn_global = this.wasmLoader.instance.exports.fmn_global.value;
    this.memU8 = this.wasmLoader.memU8;
    this.memU16 = this.wasmLoader.memU16;
    this.memU32 = this.wasmLoader.memU32;
    this.memF32 = this.wasmLoader.memF32;
    
    // "p_" Record some pointers into fmn_global.
    this.p_map_end = this.p_fmn_global + 8 + this.constants.COLC * this.constants.ROWC;
    this.p_cellphysics = this.p_map_end + 12;
    this.p_cellphysics_end = this.p_cellphysics + 256;
    this.p_sprite_storage = this.p_cellphysics_end;
    this.p_sprite_storage_end = this.p_sprite_storage + this.constants.SPRITE_STORAGE_SIZE;
    this.p_door_end = this.p_sprite_storage_end + this.constants.DOOR_LIMIT * this.constants.DOOR_SIZE;
    this.p_plantv_end = this.p_door_end + 4 + this.constants.PLANT_LIMIT * this.constants.PLANT_SIZE;
    this.p_sketchv_end = this.p_plantv_end + 4 + this.constants.SKETCH_LIMIT * this.constants.SKETCH_SIZE;
    this.p_hero = this.p_sketchv_end + 40;
    
    // "g_" Make a bunch of TypedArrays pointing to individual variables.
    this.g_spritev = new Uint32Array(this.memU8.buffer, this.p_fmn_global, 1);
    this.g_spritec = new Uint32Array(this.memU8.buffer, this.p_fmn_global + 4, 1);
    this.g_map = new Uint8Array(this.memU8.buffer, this.p_fmn_global + 8, this.constants.COLC * this.constants.ROWC);
    this.g_maptsid = new Uint8Array(this.memU8.buffer, this.p_map_end, 1);
    this.g_songid = new Uint8Array(this.memU8.buffer, this.p_map_end + 1, 1);
    this.g_neighborw = new Uint16Array(this.memU8.buffer, this.p_map_end + 2, 1);
    this.g_neighbore = new Uint16Array(this.memU8.buffer, this.p_map_end + 4, 1);
    this.g_neighborn = new Uint16Array(this.memU8.buffer, this.p_map_end + 6, 1);
    this.g_neighbors = new Uint16Array(this.memU8.buffer, this.p_map_end + 8, 1);
    this.g_cellphysics = new Uint8Array(this.memU8.buffer, this.p_cellphysics, 256);
    this.g_sprite_storage = new Uint8Array(this.memU8.buffer, this.p_sprite_storage, this.constants.SPRITE_STORAGE_SIZE);
    this.g_doorv = new Uint8Array(this.memU8.buffer, this.p_sprite_storage_end, this.constants.DOOR_SIZE * this.constants.DOOR_LIMIT);
    this.g_doorc = new Uint32Array(this.memU8.buffer, this.p_door_end, 1);
    this.g_plantv = new Uint8Array(this.memU8.buffer, this.p_door_end + 4, this.constants.PLANT_SIZE * this.constants.PLANT_LIMIT);
    this.g_plantc = new Uint32Array(this.memU8.buffer, this.p_plantv_end, 1);
    this.g_sketchv = new Uint8Array(this.memU8.buffer, this.p_plantv_end + 4, this.constants.SKETCH_SIZE * this.constants.SKETCH_LIMIT);
    this.g_sketchc = new Uint32Array(this.memU8.buffer, this.p_sketchv_end, 1);
    this.g_selected_item = new Uint8Array(this.memU8.buffer, this.p_sketchv_end + 4, 1);
    this.g_active_item = new Uint8Array(this.memU8.buffer, this.p_sketchv_end + 5, 1);
    this.g_show_off_item = new Uint8Array(this.memU8.buffer, this.p_sketchv_end + 6, 1);
    this.g_show_off_item_time = new Uint8Array(this.memU8.buffer, this.p_sketchv_end + 7, 1);
    this.g_itemv = new Uint8Array(this.memU8.buffer, this.p_sketchv_end + 8, 16);
    this.g_itemqv = new Uint8Array(this.memU8.buffer, this.p_sketchv_end + 24, 16);
    this.g_facedir = new Uint8Array(this.memU8.buffer, this.p_hero, 1);
    this.g_walking = new Uint8Array(this.memU8.buffer, this.p_hero + 1, 1);
    this.g_last_horz_dir = new Uint8Array(this.memU8.buffer, this.p_hero + 2, 1);
    this.g_wand_dir = new Uint8Array(this.memU8.buffer, this.p_hero + 3, 1);
    this.g_injury_time = new Float32Array(this.memU8.buffer, this.p_hero + 4, 1);
    this.g_illumination_time = new Float32Array(this.memU8.buffer, this.p_hero + 8, 1);
    this.g_compass = new Int16Array(this.memU8.buffer, this.p_hero + 12, 2); // [x,y]
    this.g_shovel = new Int8Array(this.memU8.buffer, this.p_hero + 16, 2); // [x,y]
  }
  
  /* Higher-level logical access with structured models.
   ****************************************************************/
   
  getSpriteByIndex(index) {
    if (index < 0) return null;
    if (index >= this.g_spritec[0]) return null;
    return this.getSpriteByAddress(this.memU32[(this.g_spritev[0] >> 2) + index]);
  }
  
  getSpriteByAddress(p) {
    const sprite = {
      x: this.memF32[p >> 2],
      y: this.memF32[(p >> 2) + 1],
      style: this.memU8[p + 8],
      imageid: this.memU8[p + 9],
      tileid: this.memU8[p + 10],
      xform: this.memU8[p + 11],
    };
    return sprite;
  }
  
  getHeroSprite() {
    let spritei = this.g_spritec[0];
    let spritepp = this.g_spritev[0] >> 2;
    for (; spritei-->0; spritepp++) {
      const spritep = this.memU32[spritepp];
      const style = this.memU8[spritep + 8];
      if (style === this.constants.SPRITE_STYLE_HERO) return this.getSpriteByAddress(spritep);
    }
    return null;
  }
  
  setMap(map) {
    this.g_map.set(map.cells);
    this.g_maptsid[0] = map.bgImageId;
    this.g_songid[0] = map.songId;
    this.g_neighborw[0] = map.neighborw;
    this.g_neighbore[0] = map.neighbore;
    this.g_neighborn[0] = map.neighborn;
    this.g_neighbors[0] = map.neighbors;
    if (map.doors && map.doors.length) {
      const doorc = Math.min(map.doors.length, this.constants.DOOR_LIMIT);
      this.g_doorc[0] = doorc;
      for (let i=0, dstp=0; i<doorc; i++ ) {
        const door = map.doors[i];
        this.g_doorv[dstp++] = door.x;
        this.g_doorv[dstp++] = door.y;
        this.g_doorv[dstp++] = door.mapId;
        this.g_doorv[dstp++] = door.mapId >> 8;
        this.g_doorv[dstp++] = door.dstx;
        this.g_doorv[dstp++] = door.dsty;
        dstp += 2; // pad to 8 bytes
      }
    } else {
      this.g_doorc[0] = 0;
    }
    if (map.cellphysics) {
      this.g_cellphysics.set(map.cellphysics);
    } else {
      for (let i=0; i<256; i++) this.g_cellphysics[i] = 0;
    }
    this.g_sketchc[0] = 0;
    this.g_plantc[0] = 0;
    //TODO fetch sketches and plants?
  }
  
  getSketch(x, y, create) {
    if ((x < 0) || (y < 0) || (x >= this.constants.COLC) || (y >= this.constants.ROWC)) return null;
    const count = this.g_sketchc[0];
    for (let i=0, p=0; i<count; i++, p+=this.constants.SKETCH_SIZE) {
      if ((this.g_sketchv[p] === x) && (this.g_sketchv[p+1] === y)) return this.getSketchByIndex(i);
    }
    if (create && (count < this.constants.SKETCH_LIMIT)) {
      this.g_sketchc[0] = count + 1;
      for (let i=this.constants.SKETCH_SIZE, p=count*this.constants.SKETCH_SIZE; i-->0; p++) {
        this.g_sketchv[p] = 0;
      }
      let p = count * this.constants.SKETCH_SIZE;
      this.g_sketchv[p++] = x;
      this.g_sketchv[p++] = y;
      return this.getSketchByIndex(count);
    }
    return null;
  }
  
  getSketchByIndex(p) {
    if ((p < 0) || (p >= this.g_sketchc[0])) return null;
    p *= this.constants.SKETCH_SIZE;
    return {
      x: this.g_sketchv[p],
      y: this.g_sketchv[p + 1],
      bits: this.g_sketchv[p+4] | (this.g_sketchv[p+5] << 8) | (this.g_sketchv[p+6] << 16) | (this.g_sketchv[p+7] << 24),
      time: this.g_sketchv[p+8] | (this.g_sketchv[p+9] << 8) | (this.g_sketchv[p+10] << 16) | (this.g_sketchv[p+11] << 24),
    };
  }
  
  setSketch(sketch) {
    if ((sketch.x < 0) || (sketch.y < 0) || (sketch.x >= this.constants.COLC) || (sketch.y >= this.constants.ROWC)) return;
    const count = this.g_sketchc[0];
    for (let i=0, p=0; i<count; i++, p+=this.constants.SKETCH_SIZE) {
      if ((this.g_sketchv[p] === sketch.x) && (this.g_sketchv[p+1] === sketch.y)) {
        this.g_sketchv[p+4] = sketch.bits;
        this.g_sketchv[p+5] = sketch.bits >> 8;
        this.g_sketchv[p+6] = sketch.bits >> 16;
        this.g_sketchv[p+7] = sketch.bits >> 24;
        this.g_sketchv[p+8] = sketch.time;
        this.g_sketchv[p+9] = sketch.time >> 8;
        this.g_sketchv[p+10] = sketch.time >> 16;
        this.g_sketchv[p+11] = sketch.time >> 24;
        return;
      }
    }
    if (count >= this.constants.SKETCH_LIMIT) return;
    this.g_sketchc[0] = count + 1;
    for (let i=this.constants.SKETCH_SIZE, p=count*this.constants.SKETCH_SIZE; i-->0; ) {
      this.g_sketchv[p] = 0;
    }
    let p = count * this.constants.SKETCH_SIZE;
    this.g_sketchv[p++] = sketch.x;
    this.g_sketchv[p++] = sketch.y;
    p += 2;
    this.g_sketchv[p++] = sketch.bits;
    this.g_sketchv[p++] = sketch.bits >> 8;
    this.g_sketchv[p++] = sketch.bits >> 16;
    this.g_sketchv[p++] = sketch.bits >> 24;
    this.g_sketchv[p++] = sketch.time;
    this.g_sketchv[p++] = sketch.time >> 8;
    this.g_sketchv[p++] = sketch.time >> 16;
    this.g_sketchv[p++] = sketch.time >> 24;
  }
  
  forEachSketch(cb) {
    const count = this.g_sketchc[0];
    for (let i=0; i<count; i++) cb(this.getSketchByIndex(i));
  }
}

Globals.singleton = true;
