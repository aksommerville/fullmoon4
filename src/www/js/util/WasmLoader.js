/* WasmLoader.js
 * Manages the relationship with our WebAssembly module.
 */
 
import { MenuFactory } from "/js/game/Menu.js";
import { DataService } from "/js/game/DataService.js";
 
export class WasmLoader {
  static getDependencies() {
    return [Window, MenuFactory, DataService];
  }
  constructor(window, menuFactory, dataService) {
    this.window = window;
    this.menuFactory = menuFactory;
    this.dataService = dataService;
    
    this.SIZE_OF_APP_MODEL = 32;
    this.COLC = 20;
    this.ROWC = 12;
    
    this.instance = null;
    this.memU8 = null;
    this.memF32 = null;
    this.appModelAddress = 0;
    this.appModel = null;
    this.sceneAddress = 0;
    this.menus = []; // stack; last item is active
    this.mapTilesheetImage = null;
  }
  
  load(url) {
    this.abort();
    return this.window.fetch(url).then((rsp) => {
      if (rsp.ok) return rsp.arrayBuffer();
      console.log(`http response`, rsp);
      if (rsp.status === 555) return rsp.json().then(e => { throw e; });
      throw rsp;
    }).then((serial) => this.window.WebAssembly.instantiate(serial, this.generateOptions()))
    .then((result) => {
      this.instance = result.instance;
      console.log(`WASM instance`, this.instance);
      this.memU8 = new Uint8Array(this.instance.exports.memory.buffer);
      this.memF32 = new Float32Array(this.instance.exports.memory.buffer);
      this.appModelAddress = this.instance.exports.fmn_app_model.value;
      this.appModel = this.getMemoryView(this.appModelAddress, this.SIZE_OF_APP_MODEL);
      this.sceneAddress = 0;
      return this.instance;
    });
  }
  
  abort() {
    this.instance = null;
    this.memU8 = null;
    this.memF32 = null;
    this.appModelAddress = 0;
    this.appModel = null;
    this.sceneAddress = 0;
    this.menus = [];
  }
  
  generateOptions() {
    return {
      env: {
        rand: () => Math.floor(Math.random() * 0x10000),
        fmn_web_log: p => this.window.console.log(`[wasm] ${this.zstringFromMemory(p)}`),
        fmn_set_scene: (p, transition) => this.setScene(p, transition),
        _fmn_begin_menu: (prompt, varargs) => this.beginMenu(prompt, varargs),
      }
    };
  }
  
  setScene(p, transition) {
    const mapsize = this.COLC * this.ROWC;
    this.sceneAddress = p;
    //TODO transition between scenes
    const tsid = this.memU8[this.sceneAddress + mapsize];
    console.log(`TODO load map tilesheet ${tsid}`);
    console.log(`TODO play song ${this.memU8[this.sceneAddress + mapsize + 1]}`);
    this.dataService.loadImage(tsid).then((img) => {
      this.mapTilesheetImage = img;
    });
  }
  
  beginMenu(prompt, varargs) {
    const options = [];
    const limit = this.memU8.length;
    for (let p=varargs; p<limit; p+=8) {
      const stringId = this.memU8[p] | (this.memU8[p+1] << 8) | (this.memU8[p+2] << 16) | (this.memU8[p+3] << 24);
      if (!stringId) break;
      const fnId = this.memU8[p+4] | (this.memU8[p+5] << 8) | (this.memU8[p+6] << 16) | (this.memU8[p+7] << 24);
      if (!fnId) break;
      if (fnId >= this.instance.exports.__indirect_function_table.length) break;
      options.push([stringId, this.instance.exports.__indirect_function_table.get(fnId)]);
    }
    this.menus.push(this.menuFactory.newMenu(prompt, options));
  }
  
  readU8(p) {
    return this.memU8[p];
  }
  
  readU16(p) {
    return this.memU8[p] | (this.memU8[p + 1] << 8);
  }
  
  readU32(p) {
    return this.memU8[p] | (this.memU8[p + 1] << 8) | (this.memU8[p + 2] << 16) | (this.memU8[p + 3] << 24);
  }
  
  readF32(p) {
    return this.memF32[p >> 2];
  }
  
  zstringFromMemory(p) {
    if (!this.instance) throw new Error(`WASM instance not loaded`);
    if (typeof p !== "number") throw new Error(`expected integer address, got ${p}`);
    if ((p < 0) || (p >= this.instance.exports.memory.buffer.byteLength)) throw new Error(`invalid address ${p}`);
    const view = new Uint8Array(this.instance.exports.memory.buffer, p);
    const sanityLimit = 1024;
    let c = 0;
    while ((c < view.length) && view[c] && (c < sanityLimit)) c++;
    return new TextDecoder("utf-8").decode(view.slice(0, c));
  }
  
  getMemoryView(v, c) {
    if (!this.instance) throw new Error(`WASM instance not loaded`);
    const memory = this.instance.exports.memory.buffer;
    if ((typeof(v) !== "number") || (typeof(c) !== "number") || (v < 0) || (c < 0) || (v > memory.byteLength - c)) {
      throw new Error(`Invalid address ${c} @ ${v}`);
    }
    return new Uint8Array(memory, v, c);
  }
  
  copyMemoryView(v, c) {
    const src = this.getMemoryView(v, c);
    const dst = new Uint8Array(src.length);
    dst.set(src);
    return dst;
  }
  
  extractAppModel() {
    if (!this.instance) throw new Error(`WASM instance not loaded`);
    console.log(`extractAppModel...`);
    const p = this.instance.exports.fmn_app_model.value;
    console.log(`p=${p}`, p);
    return this.getMemoryView(p, 256);
    return {};
  }
}

WasmLoader.singleton = true;
