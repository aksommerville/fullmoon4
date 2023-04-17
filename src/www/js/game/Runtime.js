/* Runtime.js
 * Top level coordinator for game's attachment to browser stuff, singleton.
 */
 
import { WasmLoader } from "../util/WasmLoader.js";
import { DataService } from "./DataService.js";
import { InputManager } from "./InputManager.js";
import { Renderer } from "./Renderer.js";
import { MenuFactory, ChalkMenu, VictoryMenu, GameOverMenu } from "./Menu.js";
import { Clock } from "./Clock.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { FullmoonMap } from "./FullmoonMap.js";
import { Synthesizer } from "../synth/Synthesizer.js";
import { SoundEffects } from "../synth/SoundEffects.js";
 
export class Runtime {
  static getDependencies() {
    return [
      WasmLoader, DataService, InputManager, Window, 
      Renderer, MenuFactory, Clock, Constants, Globals,
      Synthesizer, SoundEffects,
    ];
  }
  constructor(
    wasmLoader, dataService, inputManager, window,
    renderer, menuFactory, clock, constants, globals,
    synthesizer, soundEffects
  ) {
    this.wasmLoader = wasmLoader;
    this.dataService = dataService;
    this.inputManager = inputManager;
    this.window = window;
    this.renderer = renderer;
    this.menuFactory = menuFactory;
    this.clock = clock;
    this.constants = constants;
    this.globals = globals;
    this.synthesizer = synthesizer;
    this.soundEffects = soundEffects;
    
    this.onError = e => console.error(e); // RootUi should replace.
    this.onForcedPause = () => {}; // ''
    
    this.debugging = false; // when true, updates only happen explicitly via debugStep()
    this.running = false;
    this.animationFramePending = false;
    this.menus = []; // last one is active
    this.map = null;
    this.mapId = 0;
    this.clock.reset();
    
    this.wasmLoader.env.fmn_web_log = p => this.window.console.log(`[wasm] ${this.wasmLoader.zstringFromMemory(p)}`);
    this.wasmLoader.env.fmn_abort = () => this.dropAllState();
    this.wasmLoader.env._fmn_begin_menu = (prompt, varargs) => this.beginMenu(prompt, varargs);
    this.wasmLoader.env.fmn_prepare_transition = (transition) => this.renderer.prepareTransition(transition);
    this.wasmLoader.env.fmn_commit_transition = () => this.renderer.commitTransition();
    this.wasmLoader.env.fmn_cancel_transition = () => this.renderer.cancelTransition();
    this.wasmLoader.env.fmn_load_map = (mapId, cbSpawn) => this.loadMap(mapId, cbSpawn);
    this.wasmLoader.env.fmn_map_dirty = () => this.mapDirty();
    this.wasmLoader.env.fmn_add_plant = (x, y) => this.addPlant(x, y);
    this.wasmLoader.env.fmn_begin_sketch = (x, y) => this.beginSketch(x, y);
    this.wasmLoader.env.fmn_sound_effect = (sfxid) => this.soundEffects.play(sfxid);
    this.wasmLoader.env.fmn_synth_event = (chid, opcode, a, b) => this.synthesizer.event(chid, opcode, a, b);
    this.wasmLoader.env.fmn_get_string = (dst, dsta, id) => this.getString(dst, dsta, id);
    this.wasmLoader.env.fmn_find_map_command = (dstp, mask, vp) => this.findMapCommand(dstp, mask, vp);
    this.wasmLoader.env.fmn_find_direction_to_item = (itemid) => this.findDirectionToItem(itemid);
    this.wasmLoader.env.fmn_find_direction_to_map = (mapid) => this.findDirectionToMap(mapid);
    this.wasmLoader.env.fmn_map_callbacks = (evid, cb, userdata) => this.mapCallbacks(evid, cb, userdata);
    this.wasmLoader.env.fmn_log_event = (key, fmt, varags) => {};
    
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
    this.dataService.dropSavedGame();
    this.dataService.dropGameState();
    this.menus = [];
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
    
      if (this.gameShouldUpdate()) {
        const time = this.clock.update();
        this.wasmLoader.instance.exports.fmn_update(time, this.inputManager.state);
      } else {
        this.clock.skip();
        if (this.menus.length > 0) {
          const menu = this.menus[this.menus.length - 1];
          menu.update(this.inputManager.state);
        }
      }
    
      this.renderer.render(this.menus);
    
      if (!this.debugging) this.scheduleUpdate();
    } catch (e) {
      this.pause();
      this.onError(e);
    }
  }
  
  gameShouldUpdate() {
    if (!this.running) return false;
    if (this.menus.length) return false;
    if (this.renderer.getTransitionMode()) return false;
    return true;
  }
  
  beginMenu(prompt, varargs) {
    const options = [];
    if (varargs) for (;;) {
      const stringId = this.wasmLoader.memU32[varargs >> 2];
      if (!stringId) break;
      varargs += 4;
      const cbid = this.wasmLoader.memU32[varargs >> 2];
      const cb = this.wasmLoader.instance.exports.__indirect_function_table.get(cbid);
      if (!cb) break;
      options.push([stringId, cb]);
    }
    for (const menu of this.menus) menu.update(0xff);
    const menu = this.menuFactory.newMenu(prompt, options, menu => this.dismissMenu(menu));
    this.menus.push(menu);
    
    if (menu instanceof GameOverMenu) this.synthesizer.playSong(this.dataService.getSong(6));
    else if (menu instanceof VictoryMenu) this.synthesizer.playSong(this.dataService.getSong(7));
    
    return menu;
  }
  
  dismissMenu(menu) {
    const p = this.menus.indexOf(menu);
    if (p < 0) return;
    this.menus.splice(p, 1);
    
    if (menu instanceof ChalkMenu) this.renderer.mapDirty();
    else if (menu instanceof VictoryMenu) this.reset();
    
    this.inputManager.clearState();
  }
  
  loadMap(mapId, cbSpawn) {
    this.dropWitheredPlants();
    const map = this.dataService.getMap(mapId);
void fmn_log_set_time(uint32_t abstime_ms);
void fmn_log_event(const char *key,const char *fmt,...);
    if (!map) return 0;
    cbSpawn = this.wasmLoader.instance.exports.__indirect_function_table.get(cbSpawn);
    this.map = map;
    this.mapId = mapId;
    this.globals.setMap(this.map, this.clock.lastGameTime);
    this.triggerMapSetup(cbSpawn);
    this.renderer.mapDirty();
    if (map.songId) {
      const song = this.dataService.getSong(map.songId);
      this.synthesizer.playSong(song);
    }
    return 1;
  }
  
  dropWitheredPlants() {
    this.globals.forEachPlant(p => {
      if (p.state === this.constants.PLANT_STATE_DEAD) {
        this.dataService.removePlant({
          ...p,
          mapId: this.mapId,
        });
      }
    });
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
  
  mapDirty() {
    this.renderer.mapDirty();
    this.globals.forEachPlant(p => {
      p.mapId = this.mapId;
      if (p.state === this.constants.PLANT_STATE_NONE) { // signal from app to delete plant
        this.dataService.removePlant(p);
        return;
      }
      if ((p.state === this.constants.PLANT_STATE_GROW) && !p.flower_time) {
        p.flower_time = this.clock.lastGameTime + this.constants.PLANT_FLOWER_TIME * 1000;
        this.globals.updatePlant(p);
      }
      this.dataService.updatePlant(p);
    });
  }
  
  beginSketch(x, y) {
    const sketch = this.globals.getSketch(x, y, true, this.clock.lastGameTime);
    if (!sketch) return -1;
    const menu = this.beginMenu(-2, null);
    if (menu instanceof ChalkMenu) {
      menu.setup(sketch, s => this.dataService.updateSketch({
        ...s,
        mapId: this.mapId,
      }));
    }
  }
  
  addPlant(x, y) {
    if ((x < 0) || (y < 0) || (x >= this.globals.COLC) || (y >= this.globals.ROWC)) return -1;
    if (this.globals.forEachPlant(plant => {
      if (plant.state === this.constants.PLANT_STATE_NONE) return 0;
      return ((plant.x === x) && (plant.y === y));
    })) return -1;
    const plant = this.globals.addPlant(
      x, y,
      true,
      this.constants.PLANT_STATE_SEED,
      this.constants.PLANT_FRUIT_SEED,
      0 // flowerTime (0=never, gets set when you water it)
    );
    if (!plant) return -1;
    this.dataService.updatePlant({
      ...plant,
      mapId: this.mapId,
    });
    this.globals.g_map[y * this.constants.COLC + x] = 0x00;
    this.renderer.mapDirty();
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
            dstv[0] += this.constants.COLC * dx;
            dstv[1] += this.constants.ROWC * dy;
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
  
  findDirectionToItem(itemid) {
    return this.findDirectionToMap(
      this.dataService.forEachOfType("map", null, (map) => {
        if (map.ancillary) return false;
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
    const includeHoles = this.globals.g_itemv[this.constants.ITEM_BROOM];
    const checkNeighbor = (mapId, ndir) => {
      if (!mapId) return;
      // Don't provide directions thru a wall:
      switch (ndir) {
        case 0x10: if (!from.regionContainsPassableCell(0, 0, 1, this.constants.ROWC, includeHoles)) return; break;
        case 0x08: if (!from.regionContainsPassableCell(this.constants.COLC - 1, 0, 1, this.constants.ROWC, includeHoles)) return; break;
        case 0x40: if (!from.regionContainsPassableCell(0, 0, this.constants.COLC, 1, includeHoles)) return; break;
        case 0x02: if (!from.regionContainsPassableCell(0, this.constants.ROWC - 1, this.constants.COLC, 1, includeHoles)) return; break;
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
    const includeHoles = this.globals.g_itemv[this.constants.ITEM_BROOM];
    const neighborw = (from.neighborw && from.regionContainsPassableCell(0, 0, 1, this.constants.ROWC, includeHoles)) ? from.neighborw : 0;
    const neighbore = (from.neighbore && from.regionContainsPassableCell(this.constants.COLC - 1, 0, 1, this.constants.ROWC, includeHoles)) ? from.neighbore : 0;
    const neighborn = (from.neighborn && from.regionContainsPassableCell(0, 0, this.constants.COLC, 1, includeHoles)) ? from.neighborn : 0;
    const neighbors = (from.neighbors && from.regionContainsPassableCell(0, this.constants.ROWC - 1, this.constants.COLC, 1, includeHoles)) ? from.neighbors : 0;
    
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
}

Runtime.singleton = true;
