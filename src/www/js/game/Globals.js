/* Globals.js
 * Access to the global memory shared with C app.
 * Platform services should avoid accessing memory direct from WasmLoader.
 * Use me instead, to keep all the addresses and such straight.
 */
 
import { WasmLoader } from "../util/WasmLoader.js";
import { Constants } from "./Constants.js";
import { DataService } from "./DataService.js";
 
export class Globals {
  static getDependencies() {
    return [WasmLoader, Constants, DataService];
  }
  constructor(wasmLoader, constants, dataService) {
    this.wasmLoader = wasmLoader;
    this.constants = constants;
    this.dataService = dataService;
    
    this.p_fmn_global = 0;
    this.memU8 = null;
    this.memU16 = null;
    this.memS16 = null;
    this.memU32 = null;
    this.memF32 = null;
    this.recentSpellId = 8; // tracks spell declared by 'hero' and 'saveto' commands, as maps get loaded.
  }
  
  /* Call whenever wasmLoader reloads.
   */
  refresh() {
    this.recentSpellId = 8;
    
    // Yoink a few things straight off WasmLoader.
    this.p_fmn_global = this.wasmLoader.instance.exports.fmn_global.value;
    this.memU8 = this.wasmLoader.memU8;
    this.memU16 = this.wasmLoader.memU16;
    this.memS16 = this.wasmLoader.memS16;
    this.memU32 = this.wasmLoader.memU32;
    this.memF32 = this.wasmLoader.memF32;
    
    // "p_" Record some pointers into fmn_global.
    this.p_map_end = this.p_fmn_global + 8 + this.constants.COLC * this.constants.ROWC;
    this.p_cellphysics = this.p_map_end + 16;
    this.p_cellphysics_end = this.p_cellphysics + 256;
    this.p_sprite_storage = this.p_cellphysics_end;
    this.p_sprite_storage_end = this.p_sprite_storage + this.constants.SPRITE_STORAGE_SIZE;
    this.p_door_end = this.p_sprite_storage_end + this.constants.DOOR_LIMIT * this.constants.DOOR_SIZE;
    this.p_plantv_end = this.p_door_end + 4 + this.constants.PLANT_LIMIT * this.constants.PLANT_SIZE;
    this.p_sketchv_end = this.p_plantv_end + 4 + this.constants.SKETCH_LIMIT * this.constants.SKETCH_SIZE;
    this.p_hero = this.p_sketchv_end + 40;
    this.p_gs = this.p_hero + 36;
    this.p_violin_song = this.p_gs + this.constants.GS_SIZE;
    this.p_weather = this.p_violin_song + this.constants.VIOLIN_SONG_LENGTH + 7;
    
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
    this.g_mapdark = new Uint8Array(this.memU8.buffer, this.p_map_end + 10, 1);
    this.g_indoors = new Uint8Array(this.memU8.buffer, this.p_map_end + 11, 1);
    this.g_saveto = new Uint8Array(this.memU8.buffer, this.p_map_end + 14, 1);
    this.g_herostartp = new Uint8Array(this.memU8.buffer, this.p_map_end + 15, 1);
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
    this.g_match_illumination_time = new Float32Array(this.memU8.buffer, this.p_hero + 12, 1);
    this.g_cheesing = new Uint8Array(this.memU8.buffer, this.p_hero + 16, 1);
    this.g_spell_repudiation = new Uint8Array(this.memU8.buffer, this.p_hero + 17, 1);
    this.g_transmogrification = new Uint8Array(this.memU8.buffer, this.p_hero + 18, 1);
    this.g_hero_dead = new Uint8Array(this.memU8.buffer, this.p_hero + 19, 1);
    this.g_invisibility_time = new Float32Array(this.memU8.buffer, this.p_hero + 20, 1);
    this.g_curse_time = new Float32Array(this.memU8.buffer, this.p_hero + 24, 1);
    this.g_compass = new Int16Array(this.memU8.buffer, this.p_hero + 28, 2); // [x,y]
    this.g_shovel = new Int8Array(this.memU8.buffer, this.p_hero + 32, 2); // [x,y]
    this.g_werewolf_dead = new Uint8Array(this.memU8.buffer, this.p_hero + 34, 1);
    this.g_blowback = new Uint8Array(this.memU8.buffer, this.p_hero + 35, 1);
    this.g_gs = new Uint8Array(this.memU8.buffer, this.p_gs, this.constants.GS_SIZE);
    this.g_violin_song = new Uint8Array(this.memU8.buffer, this.p_violin_song, this.constants.VIOLIN_SONG_LENGTH);
    this.g_violin_clock = new Float32Array(this.memU8.buffer, this.p_violin_song + this.constants.VIOLIN_SONG_LENGTH, 1);
    this.g_violin_songp = new Uint8Array(this.memU8.buffer, this.p_violin_song + this.constants.VIOLIN_SONG_LENGTH + 4, 1);
    this.g_wind_dir = new Uint8Array(this.memU8.buffer, this.p_weather, 1);
    this.g_wind_time = new Float32Array(this.memU8.buffer, this.p_weather + 1, 1);
    this.g_rain_time = new Float32Array(this.memU8.buffer, this.p_weather + 5, 1);
    this.g_slowmo_time = new Float32Array(this.memU8.buffer, this.p_weather + 9, 1);
    this.g_terminate_time = new Float32Array(this.memU8.buffer, this.p_weather + 13, 1);
    this.g_blowbacknorm = new Float32Array(this.memU8.buffer, this.p_weather + 17, 2);
    this.g_damage_count = new Uint16Array(this.memU8.buffer, this.p_weather + 25, 1);
    this.g_earthquake_time = new Float32Array(this.memU8.buffer, this.p_weather + 29, 1);
    this.g_facedir_gsbit = new Uint16Array(this.memU8.buffer, this.p_weather+33, 2);
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
      address: p,
      x: this.memF32[p >> 2],
      y: this.memF32[(p >> 2) + 1],
      style: this.memU8[p + 8],
      imageid: this.memU8[p + 9],
      tileid: this.memU8[p + 10],
      xform: this.memU8[p + 11],
      b0: this.memU8[p + 16], // this was probably a mistake; use getSpriteBv() if you need them
      b1: this.memU8[p + 17],
      b2: this.memU8[p + 18],
    };
    return sprite;
  }
  
  getSpriteBv(p) {
    return new Uint8Array(this.memU8.buffer, p + 16, this.constants.SPRITE_BV_SIZE);
  }
  getSpriteSv(p) {
    return new Int16Array(this.memU8.buffer,
      p + 16 + this.constants.SPRITE_BV_SIZE,
      this.constants.SPRITE_SV_SIZE
    );
  }
  getSpriteFv(p) {
    return new Float32Array(this.memU8.buffer,
      p + 16 + this.constants.SPRITE_BV_SIZE + this.constants.SPRITE_SV_SIZE * 2,
      this.constants.SPRITE_FV_SIZE
    );
  }
  getSpritePv(p) {
    return new Uint32Array(this.memU8.buffer,
      p + 16 + this.constants.SPRITE_BV_SIZE + this.constants.SPRITE_SV_SIZE * 2 + this.constants.SPRITE_FV_SIZE * 4,
      this.constants.SPRITE_PV_SIZE
    );
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
  
  setMap(map, now) {
  
    if (map.spellid) this.recentSpellId = map.spellid;
    else if (map.saveto) this.recentSpellId = map.saveto;
  
    this.g_map.set(map.cells);
    this.g_maptsid[0] = map.bgImageId;
    this.g_songid[0] = map.songId;
    this.g_neighborw[0] = map.neighborw;
    this.g_neighbore[0] = map.neighbore;
    this.g_neighborn[0] = map.neighborn;
    this.g_neighbors[0] = map.neighbors;
    this.g_mapdark[0] = map.dark;
    this.g_indoors[0] = map.indoors;
    this.g_blowback[0] = map.blowback;
    this.g_herostartp[0] = map.herostartp;
    this.g_facedir_gsbit[0] = map.facedir_gsbit[0];
    this.g_facedir_gsbit[1] = map.facedir_gsbit[1];
    this.g_saveto[0] = map.saveto;
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
        this.g_doorv[dstp++] = door.extra;
        this.g_doorv[dstp++] = door.extra >> 8;
        if (door.mapId && door.extra) { // buried door. we must change the tile to 0x3f if warranted
          if (this.getGsBit(door.extra)) {
            this.g_map[door.y * this.constants.COLC + door.x] = 0x3f;
          }
        }
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
    for (const [mapid,x,y,bits,time] of this.dataService.sketches) {
      if (mapid !== map.id) continue;
      this.setSketch({ x, y, bits, time });
    }
    if (!this.g_sketchc[0]) { // No existing sketches, so look for initials from the map.
      for (const {x, y, bits} of map.sketches) {
        this.setSketch({ x, y, bits, now });
        this.dataService.updateSketch({ mapId: map.id, x, y, bits, time: now });
      }
    }
    
    this.g_plantc[0] = 0;
    for (let [mapid,x,y,flowerTime,state,fruit] of this.dataService.plants) {
      if (mapid !== map.id) continue;
      if (!map.indoors) { // auto-bloom... kind of a lot of conditions...
        if (now && flowerTime && (state === this.constants.PLANT_STATE_GROW) && (now >= flowerTime)) {
          state = this.constants.PLANT_STATE_FLOWER;
        }
      }
      this.addPlant(x, y, true, state, fruit, flowerTime);
    }
    
    this.g_rain_time[0] = 0;
    this.g_wind_time[0] = 0;
    if (map.wind) {
      this.g_wind_dir[0] = map.wind;
      this.g_wind_time[0] = 86400.0; // just wait one day and it will stop, easy.
    }
  }
  
  getGsBit(gsbit) {
    const p = gsbit >> 3;
    if (p >= this.g_gs.length) return 0;
    const mask = 0x80 >> (gsbit & 7);
    if (this.g_gs[p] & mask) return 1;
    return 0;
  }
  
  /* Plants and sketches.
   * We are only concerned with the ones shared with wasm, ie the ones onscreen.
   ***************************************************************************/
  
  getSketch(x, y, create, time) {
    if ((x < 0) || (y < 0) || (x >= this.constants.COLC) || (y >= this.constants.ROWC)) return null;
    const count = this.g_sketchc[0];
    for (let i=0, p=0; i<count; i++, p+=this.constants.SKETCH_SIZE) {
      if ((this.g_sketchv[p] === x) && (this.g_sketchv[p+1] === y)) return this.getSketchByIndex(i);
    }
    if (create && (count < this.constants.SKETCH_LIMIT)) {
      if (!time) time = 0;
      this.g_sketchc[0] = count + 1;
      for (let i=this.constants.SKETCH_SIZE, p=count*this.constants.SKETCH_SIZE; i-->0; p++) {
        this.g_sketchv[p] = 0;
      }
      let p = count * this.constants.SKETCH_SIZE;
      this.g_sketchv[p++] = x;
      this.g_sketchv[p++] = y;
      p += 6; // 2 bytes unused + 4 bytes bits
      this.g_sketchv[p++] = time;
      this.g_sketchv[p++] = time >> 8;
      this.g_sketchv[p++] = time >> 16;
      this.g_sketchv[p++] = time >> 24;
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
    for (let i=0; i<count; i++) {
      const result = cb(this.getSketchByIndex(i));
      if (result) return result;
    }
  }
  
  getPlantByIndex(p) {
    if ((p < 0) || (p >= this.g_plantc[0])) return null;
    p *= this.constants.PLANT_SIZE;
    const plant = {
      x: this.g_plantv[p],
      y: this.g_plantv[p + 1],
      state: this.g_plantv[p + 2],
      fruit: this.g_plantv[p + 3],
      flower_time: this.g_plantv[p + 4] | (this.g_plantv[p + 5] << 8) | (this.g_plantv[p + 6] << 16) | (this.g_plantv[p + 7] << 24),
    };
    return plant;
  }
  
  forEachPlant(cb) {
    const count = this.g_plantc[0];
    for (let i=0; i<count; i++) {
      const result = cb(this.getPlantByIndex(i));
      if (result) return result;
    }
  }
  
  addPlant(x, y, reuse, state, fruit, flower_time) {
    if (!state) state = this.constants.PLANT_STATE_SEED;
    if (!fruit) fruit = 0;
    if (!flower_time) flower_time = 0;
    if (this.g_plantc[0] >= this.constants.PLANT_LIMIT) {
      if (reuse) {
        for (let i=this.g_plantc[0], p=0; i-->0; p+=this.constants.PLANT_SIZE) {
          if (
            (this.g_plantv[p + 2] === this.constants.PLANT_STATE_DEAD) ||
            (this.g_plantv[p + 2] === this.constants.PLANT_STATE_NONE)
          ) {
            this.g_plantv[p++] = x;
            this.g_plantv[p++] = y;
            this.g_plantv[p++] = state;
            this.g_plantv[p++] = fruit;
            this.g_plantv[p++] = flower_time;
            this.g_plantv[p++] = flower_time >> 8;
            this.g_plantv[p++] = flower_time >> 16;
            this.g_plantv[p++] = flower_time >> 24;
            return { x, y, state, fruit, flower_time };
          }
        }
      }
      return null;
    }
    // If there's already a plant here, drop it and replace.
    // This is important, for PLANT_STATE_NONE.
    let index = null;
    for (let i=0, p=0; i<this.g_plantc[0]; i++, p+=this.constants.PLANT_SIZE) {
      if (this.g_plantv[0] !== x) continue;
      if (this.g_plantv[1] !== y) continue;
      index = i;
      break;
    }
    if (index === null) {
      index = this.g_plantc[0]++;
    }
    let p = this.constants.PLANT_SIZE * index;
    this.g_plantv[p++] = x;
    this.g_plantv[p++] = y;
    this.g_plantv[p++] = state;
    this.g_plantv[p++] = fruit;
    this.g_plantv[p++] = flower_time;
    this.g_plantv[p++] = flower_time >> 8;
    this.g_plantv[p++] = flower_time >> 16;
    this.g_plantv[p++] = flower_time >> 24;
    return { x, y, state, fruit, flower_time };
  }
  
  updatePlant(plant) {
    for (let i=this.g_plantc[0], p=0; i-->0; p+=this.constants.PLANT_SIZE) {
      if (this.g_plantv[p] !== plant.x) continue;
      if (this.g_plantv[p + 1] !== plant.y) continue;
      this.g_plantv[p + 2] = plant.state;
      this.g_plantv[p + 3] = plant.fruit;
      this.g_plantv[p + 4] = plant.flower_time;
      this.g_plantv[p + 5] = plant.flower_time >> 8;
      this.g_plantv[p + 6] = plant.flower_time >> 16;
      this.g_plantv[p + 7] = plant.flower_time >> 24;
      return;
    }
  }
}

Globals.singleton = true;
