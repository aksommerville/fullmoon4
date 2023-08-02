/* Runtime.js
 * Top level coordinator for game's attachment to browser stuff, singleton.
 */
 
import { WasmLoader } from "../util/WasmLoader.js";
import { DataService } from "./DataService.js";
import { InputManager } from "./InputManager.js";
import { Renderer2d } from "./Renderer2d.js";
//import { RendererGl } from "./RendererGl.js";
import { Clock } from "./Clock.js";
import * as FMN from "./Constants.js";
import { Globals } from "./Globals.js";
import { FullmoonMap } from "./FullmoonMap.js";
import { Synthesizer } from "../synth/Synthesizer.js";
import { SoundEffects } from "../synth/SoundEffects.js";
import { SavedGameStore } from "./SavedGameStore.js";
 
export class Runtime {
  static getDependencies() {
    return [
      WasmLoader, DataService, InputManager, Window, 
      Renderer2d, /*RendererGl,*/ Clock, Globals,
      Synthesizer, SoundEffects, Document, SavedGameStore
    ];
  }
  constructor(
    wasmLoader, dataService, inputManager, window,
    renderer2d, /*rendererGl,*/ clock, globals,
    synthesizer, soundEffects, document, savedGameStore
  ) {
    this.wasmLoader = wasmLoader;
    this.dataService = dataService;
    this.inputManager = inputManager;
    this.window = window;
    this.renderer = renderer2d; // rendererGl or renderer2d, same interface. TODO let user choose
    this.clock = clock;
    this.globals = globals;
    this.synthesizer = synthesizer;
    this.soundEffects = soundEffects;
    this.document = document;
    this.savedGameStore = savedGameStore;
    
    this.onError = e => console.error(e); // RootUi should replace.
    this.onForcedPause = () => {}; // ''
    
    this.debugging = false; // when true, updates only happen explicitly via debugStep()
    this.running = false;
    this.animationFramePending = false;
    this.map = null;
    this.mapId = 0;
    this.clock.reset();
    
    this.wasmLoader.env.fmn_web_log = p => this.window.console.log(`[wasm] ${this.wasmLoader.zstringFromMemory(p)}`);
    this.wasmLoader.env.fmn_abort = () => this.dropAllState();
    this.wasmLoader.env.fmn_quit = () => -1;
    this.wasmLoader.env.fmn_can_quit = () => 0;
    this.wasmLoader.env.fmn_reset = () => this.reset();
    this.wasmLoader.env.fmn_load_map = (mapId, cbSpawn) => this.loadMap(mapId, cbSpawn);
    this.wasmLoader.env.fmn_get_map = (dstp, dsta, mapId) => this.getMap(dstp, dsta, mapId);
    this.wasmLoader.env.fmn_add_plant = (x, y) => this.addPlant(x, y);
    this.wasmLoader.env.fmn_begin_sketch = (x, y) => this.beginSketch(x, y);
    this.wasmLoader.env.fmn_sound_effect = (sfxid) => this.soundEffects.play(sfxid);
    this.wasmLoader.env.fmn_synth_event = (chid, opcode, a, b) => this.synthesizer.event(chid, opcode, a, b);
    this.wasmLoader.env.fmn_play_song = (songid, loop) => this.playSong(songid, loop);
    this.wasmLoader.env.fmn_get_string = (dst, dsta, id) => this.getString(dst, dsta, id);
    this.wasmLoader.env.fmn_find_map_command = (dstp, mask, vp) => this.findMapCommand(dstp, mask, vp);
    this.wasmLoader.env.fmn_find_teleport_target = (spellid) => this.findTeleportTarget(spellid);
    this.wasmLoader.env.fmn_find_direction_to_item = (itemid) => this.findDirectionToItem(itemid);
    this.wasmLoader.env.fmn_find_direction_to_map = (mapid) => this.findDirectionToMap(mapid);
    this.wasmLoader.env.fmn_find_direction_to_teleport = (spellid) => this.findDirectionToTeleport(mapid);
    this.wasmLoader.env.fmn_find_direction_to_map_reference = (ref) => this.findDirectionToMapReference(ref);
    this.wasmLoader.env.fmn_map_callbacks = (evid, cb, userdata) => this.mapCallbacks(evid, cb, userdata);
    this.wasmLoader.env.fmn_web_log_event = p => this.logBusinessEvent(this.wasmLoader.zstringFromMemory(p));
    this.wasmLoader.env.fmn_has_saved_game = () => this.savedGameStore.hasSavedGame();
    this.wasmLoader.env.fmn_load_saved_game = () => this.savedGameStore.loadSavedGame();
    this.wasmLoader.env.fmn_delete_saved_game = () => this.savedGameStore.deleteSavedGame();
    this.wasmLoader.env.fmn_saved_game_dirty = () => this.savedGameStore.setDirty();
    this.wasmLoader.env.fmn_is_demo = () => this.dataService.mapCount < 100;
    
    this.wasmLoader.env.fmn_video_init = (wmin, wmax, hmin, hmax, pixfmt) => this.renderer.fmn_video_init(wmin, wmax, hmin, hmax, pixfmt);
    this.wasmLoader.env.fmn_video_get_framebuffer_size = (wv, hv) => this.renderer.fmn_video_get_framebuffer_size(wv, hv);
    this.wasmLoader.env.fmn_video_get_pixfmt = () => 0x71; // RGBA
    this.wasmLoader.env.fmn_video_rgba_from_pixel = (pixel) => pixel;
    this.wasmLoader.env.fmn_video_pixel_from_rgba = (rgba) => rgba;
    this.wasmLoader.env.fmn_video_get_image_size = (wv, hv, imageid) => this.renderer.fmn_video_get_image_size(wv, hv, imageid);
    this.wasmLoader.env.fmn_video_init_image = (imageid, w, h) => this.renderer.fmn_video_init_image(imageid, w, h);
    this.wasmLoader.env.fmn_draw_set_output = (imageid) => this.renderer.fmn_draw_set_output(imageid);
    this.wasmLoader.env.fmn_draw_clear = () => this.renderer.fmn_draw_clear();
    this.wasmLoader.env.fmn_draw_line = (v, c) => this.renderer.fmn_draw_line(v, c);
    this.wasmLoader.env.fmn_draw_rect = (v, c) => this.renderer.fmn_draw_rect(v, c);
    this.wasmLoader.env.fmn_draw_mintile = (v, c, imageid) => this.renderer.fmn_draw_mintile(v, c, imageid);
    this.wasmLoader.env.fmn_draw_maxtile = (v, c, imageid) => this.renderer.fmn_draw_maxtile(v, c, imageid);
    this.wasmLoader.env.fmn_draw_decal = (v, c, imageid) => this.renderer.fmn_draw_decal(v, c, imageid);
    this.wasmLoader.env.fmn_draw_decal_swap = (v, c, imageid) => this.renderer.fmn_draw_decal_swap(v, c, imageid);
    this.wasmLoader.env.fmn_draw_recal = (v, c, imageid) => this.renderer.fmn_draw_recal(v, c, imageid);
    this.wasmLoader.env.fmn_draw_recal_swap = (v, c, imageid) => this.renderer.fmn_draw_recal_swap(v, c, imageid);
    
    // Fetch the data archive and wasm asap. This doesn't start the game or anything.
    this.preloadAtConstruction = Promise.all([
      this.dataService.load(),
      this.wasmLoader.load("./fullmoon.wasm"),
    ]);
  }
  
  // RootUI should do this once, with the main canvas. OK to replace whenever.
  setRenderTarget(canvas) {
    this.renderer.setRenderTarget(canvas);
  }
  
  reset() {
    this.dropAllState();
    return this.wasmLoader.load("./fullmoon.wasm")
      .then(() => this.dataService.load())
      .then(() => {
        console.log(`Runtime: loaded wasm instance`, this.wasmLoader.instance);
        this.globals.refresh();
        this.running = true;
        //TODO load saved game or initial state
        this.clock.reset(0);
        if (this.wasmLoader.instance.exports.fmn_init()) {
          this.dropAllState();
          throw new Error("fmn_init failed");
        }
        this.scheduleUpdate();
      });
  }
  
  dropAllState() {
    this.wasmLoader.abort();
    this.running = false;
    this.inputManager.clearState();
    this.clock.reset(0);
    this.synthesizer.reset();
    this.dataService.dropGameState();
    this.map = null;
    this.mapId = 0;
  }
  
  pause() {
    if (!this.running) return;
    this.running = false;
    this.clock.pause();
    this.synthesizer.pause();
  }
  
  resume() {
    if (this.running) return;
    this.referenceTime += Date.now() - this.pauseTime;
    this.running = true;
    this.inputManager.clearState();
    this.clock.resume();
    if (!this.debugging) this.synthesizer.resume();
    this.scheduleUpdate();
  }
  
  scheduleUpdate() {
    if (this.animationFramePending) return;
    if (!this.running) return;
    this.animationFramePending = true;
    this.window.requestAnimationFrame(() => {
      this.animationFramePending = false;
      this.update();
    });
  }
  
  update(viaExplicitDebugger) {
    if (!this.running) return;
    if (!this.wasmLoader.instance) return;
    if (this.debugging && !viaExplicitDebugger) return;
    
    if (this.clock.checkUpdateFrequencyPanic()) {
      console.log(`Pausing due to low update rate.`);
      this.pause();
      this.onForcedPause();
      return;
    }
    
    try {
    
      this.inputManager.update();
      if (!this.debugging) this.synthesizer.update();
    
      let updated = true;
      if (this.running) {
        const time = this.clock.update();
        if (time) {
          this.wasmLoader.instance.exports.fmn_update(time, this.inputManager.state);
        } else {
          updated = false;
        }
        if (!this.running) return;
        this.savedGameStore.update(time, () => this._persistPlants());
      } else {
        this.clock.skip();
        //TODO Should we still render in this case?
      }
    
      if (updated) {
        this.renderer.begin();
        if (this.wasmLoader.instance.exports.fmn_render()) {
          this.renderer.commit();
        }
      }
    
      if (!this.debugging) this.scheduleUpdate();
    } catch (e) {
      this.pause();
      this.onError(e);
    }
  }
  
  loadMap(mapId, cbSpawn) {
    this._persistPlants();
    const map = this.dataService.getMap(mapId, this.clock.lastGameTime);
    if (!map) return 0;
    cbSpawn = this.wasmLoader.instance.exports.__indirect_function_table.get(cbSpawn);
    this.map = map;
    this.mapId = mapId;
    this.globals.setMap(this.map, this.clock.lastGameTime);
    this.triggerMapSetup(cbSpawn);
    if (map.songId) {
      const song = this.dataService.getSong(map.songId);
      this.synthesizer.playSong(song, false, true);
    }
    this.savedGameStore.setDirty();
    return 1;
  }
  
  getMap(dstp, dsta, mapId) {
    const res = this.dataService.toc.find(r => r.type === 3 && r.id === mapId);
    if (!res) return 0;
    if (res.ser.length <= dsta) {
      const dstview = new Uint8Array(this.wasmLoader.memU8.buffer, this.wasmLoader.memU8.byteOffset + dstp, res.ser.length);
      dstview.set(res.ser);
    }
    return res.ser.length;
  }
  
  playSong(songid, loop) {
    const song = this.dataService.getSong(songid);
    this.synthesizer.playSong(song, false, loop);
  }
  
  triggerMapSetup(cbSpawn) {
    if (!this.map.sprites) return;
    for (const { x, y, spriteId, arg0, arg1, arg2 } of this.map.sprites) {
      const sprdef = this.dataService.getSprite(spriteId);
      let defc = 0;
      if (sprdef) {
        defc = sprdef.length;
        this.globals.g_sprite_storage.set(sprdef);
      }
      // (cb) has an "arg3" which we don't have, that's the '0'
      cbSpawn(x, y, spriteId, arg0, arg1, arg2, 0, this.globals.p_sprite_storage, defc);
    }
  }
  
  _persistPlants() {
    this.globals.forEachPlant(p => {
      p.mapId = this.mapId;
      if (
        (p.state === FMN.PLANT_STATE_NONE) ||
        (p.state === FMN.PLANT_STATE_DEAD)
      ) {
        this.dataService.removePlant({ ...p, mapId: this.mapId });
        return;
      }
      this.dataService.updatePlant({ ...p, mapId: this.mapId });
    });
    this.globals.forEachSketch(s => {
      this.dataService.updateSketch({ ...s, mapId: this.mapId });
    });
  }
  
  beginSketch(x, y) {
    const sketch = this.globals.getSketch(x, y, true, this.clock.lastGameTime);
    if (!sketch) return -1;
    return sketch.bits;
  }
  
  addPlant(x, y) {
    if ((x < 0) || (y < 0) || (x >= this.globals.COLC) || (y >= this.globals.ROWC)) return -1;
    if (this.globals.forEachPlant(plant => {
      if (plant.state === FMN.PLANT_STATE_NONE) return 0;
      return ((plant.x === x) && (plant.y === y));
    })) return -1;
    const plant = this.globals.addPlant(
      x, y,
      true,
      FMN.PLANT_STATE_SEED,
      FMN.PLANT_FRUIT_SEED,
      0 // flowerTime (0=never, gets set when you water it)
    );
    if (!plant) return -1;
    this.dataService.updatePlant({
      ...plant,
      mapId: this.mapId,
    });
    this.globals.g_map[y * FMN.COLC + x] = 0x00;
    this.savedGameStore.setDirty();
    return 0;
  }
  
  debugPauseToggle() {
    if (this.debugging) {
      this.debugging = false;
      this.clock.undebug();
      if (this.running) {
        this.synthesizer.resume();
        this.scheduleUpdate();
      }
    } else {
      this.clock.debug();
      this.debugging = true;
      this.synthesizer.pause();
    }
  }
  
  debugStep() {
    if (!this.debugging) return;
    this.update(true);
  }
  
  getString(dst, dsta, id) {
    const src = this.dataService.getString(id) || "";
    const cpc = Math.min(dsta, src.length);
    for (let i=cpc; i-->0; ) this.wasmLoader.memU8[dst + i] = src.charCodeAt(i);
    return cpc;
  }
  
  findMapCommand(dstp, mask, vp) {
    if (!this.map) return 0;
    let vc = 0;
         if (mask & 0x80) vc = 8;
    else if (mask & 0x40) vc = 7;
    else if (mask & 0x20) vc = 6;
    else if (mask & 0x10) vc = 5;
    else if (mask & 0x08) vc = 4;
    else if (mask & 0x04) vc = 3;
    else if (mask & 0x02) vc = 2;
    else if (mask & 0x01) vc = 1;
    else return 0; // logically, this should be "any command", but that's a stupid request.
    const v = new Uint8Array(this.wasmLoader.memU8.buffer, vp, vc);
    
    const checkMap = (map, dx, dy) => {
      if (!map) return 0;
      return map.forEachCommand((opcode, argv, argp, argc) => {
        if ((mask & 0x01) && (v[0] !== opcode)) return;
        if (argc < vc - 1) return;
        let match = true;
        for (let vi=1, argi=argp, bit=0x02; vi<vc; vi++, argi++, bit<<=1) {
          if (!(bit & mask)) continue;
          if (argv[argi] !== v[vi]) {
            match = false;
            break;
          }
        }
        if (match) {
          const [x, y] = this.map.getCommandLocation(opcode, argv, argp, argc);
          const dstv = new Int16Array(this.wasmLoader.memU8.buffer, dstp, 2);
          dstv[0] = x;
          dstv[1] = y;
          if (typeof(dx) === "number") {
            dstv[0] += FMN.COLC * dx;
            dstv[1] += FMN.ROWC * dy;
          } else { // (dx) is a door, and we point to it, rather than the actual focus
            dstv[0] = dx.x;
            dstv[1] = dx.y;
          }
          return 1;
        }
      });
    };
    
    if (checkMap(this.map, 0, 0)) return 1;
    if (this.map.neighborn && checkMap(this.dataService.getMap(this.map.neighborn), 0, -1)) return 1;
    if (this.map.neighbors && checkMap(this.dataService.getMap(this.map.neighbors), 0, 1)) return 1;
    if (this.map.neighborw && checkMap(this.dataService.getMap(this.map.neighborw), -1, 0)) return 1;
    if (this.map.neighbore && checkMap(this.dataService.getMap(this.map.neighbore), 1, 0)) return 1;
    for (const door of this.map.doors) {
      if (!door.mapId) continue;
      if (checkMap(this.dataService.getMap(door.mapId), door)) return 1;
    }
    
    return 0;
  }
  
  findTeleportTarget(spellid) {
    const map = this.dataService.forEachOfType("map", 0, map => {
      if (map.spellid === spellid) return map.id;
    });
    if (map) return map.id;
    return 0;
  }
  
  findDirectionToItem(itemid) {
    return this.findDirectionToMap(
      this.dataService.forEachOfType("map", null, (map) => {
        if (map.flag & FMN.MAPFLAG_ANCILLARY) return false;
        for (const sprite of map.sprites) {
          if (sprite.spriteId === 3) { // treasure. TODO Can we avoid hard-coding resource IDs?
            if (sprite.arg0 === itemid) return map;
          }
        }
        for (const door of map.doors) {
          if (door.mapId) continue;
          if (door.dstx !== 0x30) continue;
          if (door.dsty !== itemid) continue;
          // Check whether we've already got it, which shouldn't be possible but hey.
          if (door.extra && this.globals.getGsBit(door.extra)) continue;
          return map;
        }
        return false;
      })
    );
  }
  
  findDirectionToTeleport(spellid) {
    return this.findDirectionToMap(
      this.dataService.forEachOfType("map", null, (map) => map.spellid === spellid)
    );
  }
  
  findDirectionToMapReference(ref) {
    return this.findDirectionToMap(
      this.dataService.forEachOfType("map", null, (map) => map.ref === ref)
    );
  }
  
  findDirectionToMap(mapIdOrMapOrFalse) {
    if (!this.map) return 0;
    let map;
    if (mapIdOrMapOrFalse instanceof FullmoonMap) map = mapIdOrMapOrFalse;
    else map = this.dataService.getMap(mapIdOrMapOrFalse);
    if (!map) return 0;
    return this.firstDirectionFromMapToMap(this.map, map);
  }
  
  firstDirectionFromMapToMap(from, to) {
    if (!from || !to) return 0;
    if (from.id === to.id) return 0xff;
    let dir = 0, distance = 99;
    const includeHoles = this.globals.g_itemv[FMN.ITEM_BROOM];
    const checkNeighbor = (mapId, ndir) => {
      if (!mapId) return;
      // Don't provide directions thru a wall:
      switch (ndir) {
        case 0x10: if (!from.regionContainsPassableCell(0, 0, 1, FMN.ROWC, includeHoles)) return; break;
        case 0x08: if (!from.regionContainsPassableCell(FMN.COLC - 1, 0, 1, FMN.ROWC, includeHoles)) return; break;
        case 0x40: if (!from.regionContainsPassableCell(0, 0, FMN.COLC, 1, includeHoles)) return; break;
        case 0x02: if (!from.regionContainsPassableCell(0, FMN.ROWC - 1, FMN.COLC, 1, includeHoles)) return; break;
      }
      if (mapId === to.id) {
        dir = ndir;
        distance = 0;
        return;
      }
      const qdist = this.distanceFromMapToMap(this.dataService.getMap(mapId), to, [from.id], 10);
      // Expected path: 1(home) => 2(bridge) => 5(farm) => 8(pond) => 29(pub)
      if (qdist < distance) {
        dir = ndir;
        distance = qdist;
      }
    };
    checkNeighbor(from.neighborw, 0x10); if (!distance) return dir;
    checkNeighbor(from.neighbore, 0x08); if (!distance) return dir;
    checkNeighbor(from.neighborn, 0x40); if (!distance) return dir;
    checkNeighbor(from.neighbors, 0x02); if (!distance) return dir;
    for (const door of from.doors) {
      if (!door.mapId) continue;
      if (door.mapId === to.id) return 0xff;
      let qdist = this.distanceFromMapToMap(this.dataService.getMap(door.mapId), to, [from.id], 10);
      if (qdist < distance) {
        distance = qdist;
        dir = 0xff;
      }
    }
    return dir;
  }
  
  distanceFromMapToMap(from, to, poisonMapIds, limit) {
  
    // is it invalid, or the same map?
    if (!from || !to) return 999;
    if (from.id === to.id) return 0;
    
    // Filter neighbors. We only want them if that edge is actually reachable.
    const includeHoles = this.globals.g_itemv[FMN.ITEM_BROOM];
    const neighborw = (from.neighborw && from.regionContainsPassableCell(0, 0, 1, FMN.ROWC, includeHoles)) ? from.neighborw : 0;
    const neighbore = (from.neighbore && from.regionContainsPassableCell(FMN.COLC - 1, 0, 1, FMN.ROWC, includeHoles)) ? from.neighbore : 0;
    const neighborn = (from.neighborn && from.regionContainsPassableCell(0, 0, FMN.COLC, 1, includeHoles)) ? from.neighborn : 0;
    const neighbors = (from.neighbors && from.regionContainsPassableCell(0, FMN.ROWC - 1, FMN.COLC, 1, includeHoles)) ? from.neighbors : 0;
    
    // is it an immediate neighbor?
    if (neighborw === to.id) return 1;
    if (neighbore === to.id) return 1;
    if (neighborn === to.id) return 1;
    if (neighbors === to.id) return 1;
    for (const door of from.doors) {
      if (door.mapId === to.id) return 1;
    }
    
    // recursion limits?
    if (limit-- <= 0) return 999;
    if (poisonMapIds.indexOf(from.id) >= 0) return 999;
    poisonMapIds = [...poisonMapIds, from.id];
    
    // Take the lowest result from all my neighbors and add 1.
    let best = 999;
    for (const nid of [neighborw, neighbore, neighborn, neighbors]) {
      if (!nid) continue;
      const q = this.distanceFromMapToMap(this.dataService.getMap(nid), to, poisonMapIds, limit);
      if (q < best) best = q;
    }
    for (const door of from.doors) {
      if (!door.mapId) continue;
      // Buried doors (door.gsbit nonzero) count whether exposed or not.
      const q = this.distanceFromMapToMap(this.dataService.getMap(door.mapId), to, poisonMapIds, limit);
      if (q < best) best = q;
    }
    return best + 1;
  }
  
  mapCallbacks(qevid, cbp, userdata) {
    if (!this.map) return;
    const cb = this.wasmLoader.instance.exports.__indirect_function_table.get(cbp);
    for (const { evid, cbid, param } of this.map.callbacks) {
      if (evid !== qevid) continue;
      cb(cbid, param, userdata);
    }
  }
  
  logBusinessEvent(text) {
    //TODO This shouldn't exist in prod, just stub it out.
    // For now, gathering all business events for inclusion in an email, if the user opts to.
    if (!this.document._fmn_business_log) {
      this.document._fmn_business_log = { text: "" };
    }
    /* Prefix as generated by bigpc. Followed by space, then message from client.
    "%u:%u+%u:%lld@%d,%d,%d;%s",
    bigpc.clock.last_game_time_ms,
    bigpc.clock.framec,
    bigpc.clock.skipc,
    (long long)(bigpc.clock.last_real_time_us-bigpc.clock.first_real_time_us),
    bigpc.mapid,(int)herox,(int)heroy,
    key
    */
    // Our logs are formatted a little different.
    this.document._fmn_business_log.text += `${this.clock.lastGameTime}@${this.mapId} ${text}\n`;
  }
}

Runtime.singleton = true;
