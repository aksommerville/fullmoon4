/* Runtime.js
 * Top level coordinator for game's attachment to browser stuff, singleton.
 */
 
import { WasmLoader } from "../util/WasmLoader.js";
import { DataService } from "./DataService.js";
import { InputManager } from "./InputManager.js";
import { Renderer } from "./Renderer.js";
import { MenuFactory, ChalkMenu } from "./Menu.js";
import { Clock } from "./Clock.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
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
    this.wasmLoader.env.fmn_map_dirty = () => this.renderer.mapDirty();
    this.wasmLoader.env.fmn_add_plant = (x, y) => {};//TODO
    this.wasmLoader.env.fmn_begin_sketch = (x, y) => this.beginSketch(x, y);
    this.wasmLoader.env.fmn_sound_effect = (sfxid) => this.soundEffects.play(sfxid);
    this.wasmLoader.env.fmn_synth_event = (chid, opcode, a, b) => this.synthesizer.event(chid, opcode, a, b);
    
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
    return menu;
  }
  
  dismissMenu(menu) {
    const p = this.menus.indexOf(menu);
    if (p < 0) return;
    this.menus.splice(p, 1);
    if (menu instanceof ChalkMenu) this.renderer.mapDirty();
    this.inputManager.clearState();
  }
  
  loadMap(mapId, cbSpawn) {
    const map = this.dataService.getMap(mapId);
    if (!map) return 0;
    cbSpawn = this.wasmLoader.instance.exports.__indirect_function_table.get(cbSpawn);
    this.map = map;
    this.mapId = mapId;
    this.globals.setMap(this.map);
    this.triggerMapSetup(cbSpawn);
    this.renderer.mapDirty();
    if (map.songId) {
      const song = this.dataService.getSong(map.songId);
      this.synthesizer.playSong(song);
      //if (song) console.log(`playing song:${map.songId}`);
      //else console.log(`song:${map.songId} not found, playing silence`);
    }
    return 1;
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
  
  beginSketch(x, y) {
    const sketch = this.globals.getSketch(x, y, true);
    if (!sketch) return -1;
    const menu = this.beginMenu(-2, null);
    if (menu instanceof ChalkMenu) {
      menu.setup(sketch);
    }
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
}

Runtime.singleton = true;
