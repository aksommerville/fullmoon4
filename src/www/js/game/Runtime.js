/* Runtime.js
 * Top level coordinator for game's attachment to browser stuff, singleton.
 */
 
import { WasmLoader } from "../util/WasmLoader.js";
import { DataService } from "./DataService.js";
import { InputManager } from "./InputManager.js";
import { Renderer } from "./Renderer.js";
import { MenuFactory } from "./Menu.js";
import { Clock } from "./Clock.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { Synthesizer } from "../synth/Synthesizer.js";
 
export class Runtime {
  static getDependencies() {
    return [
      WasmLoader, DataService, InputManager, Window, 
      Renderer, MenuFactory, Clock, Constants, Globals,
      Synthesizer,
    ];
  }
  constructor(
    wasmLoader, dataService, inputManager, window,
    renderer, menuFactory, clock, constants, globals,
    synthesizer
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
    this.wasmLoader.env.fmn_begin_sketch = (x, y) => {};//TODO
    
    this.dataService.load();
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
        this.clock.runtimeLoaded();
        if (this.wasmLoader.instance.exports.fmn_init()) {
          this.dropAllState();
          throw new Error("fmn_init failed");
        }
        //TODO load saved game or initial state
        //this.clock.setPriorTime(savedGame.time)
        this.scheduleUpdate();
      });
  }
  
  dropAllState() {
    this.wasmLoader.abort();
    this.running = false;
    this.inputManager.clearState();
    this.clock.reset();
    this.synthesizer.reset();
  }
  
  pause() {
    if (!this.running) return;
    this.running = false;
    this.clock.hardPause();
    this.synthesizer.pause();
  }
  
  resume() {
    if (this.running) return;
    this.referenceTime += Date.now() - this.pauseTime;
    this.running = true;
    this.inputManager.clearState();
    this.clock.hardResume();
    this.synthesizer.resume();
    this.scheduleUpdate();
  }
  
  scheduleUpdate() {
    if (this.animationFramePending) return;
    if (!this.running) return;
    this.window.requestAnimationFrame(() => this.update());
  }
  
  update() {
    if (!this.running) return;
    if (!this.wasmLoader.instance) return;
    
    this.inputManager.update();
    this.synthesizer.update();
    
    if (this.gameShouldUpdate()) {
      this.clock.softResume();
      const time = this.clock.update();
      this.wasmLoader.instance.exports.fmn_update(time, this.inputManager.state);
    } else {
      this.clock.softPause();
    }
    
    this.renderer.render(this.menus);
    
    this.scheduleUpdate();
  }
  
  gameShouldUpdate() {
    if (!this.running) return false;
    if (this.menus.length) return false;
    if (this.renderer.getTransitionMode()) return false;
    return true;
  }
  
  beginMenu(prompt, varargs) {
    const options = [];
    for (;;) {
      const stringId = this.wasmLoader.memU32[varargs >> 2];
      if (!stringId) break;
      varargs += 4;
      const cbid = this.wasmLoader.memU32[varargs >> 2];
      const cb = this.wasmLoader.instance.exports.__indirect_function_table.get(cbid);
      if (!cb) break;
      options.push([stringId, cb]);
    }
    this.menus.push(this.menuFactory.newMenu(prompt, options));
    console.log(`Runtime.beginMenu`, { prompt, options, menus: this.menus });
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
    
    //XXX TEMP. 1=tangled-vine, 2=seven-circles-of-a-witchs-soul, 3=toil-and-trouble
    const song = this.dataService.getSong(3);
    this.synthesizer.playSong(song);
    
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
}

Runtime.singleton = true;
