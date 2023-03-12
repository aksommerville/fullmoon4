/* WasmLoader.js
 * Manages the relationship with our WebAssembly module.
 * In the interest of performance, we expose a lot of instance members for clients to work with directly.
 */
 
export class WasmLoader {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.instance = null;
    this.memU8 = null;
    this.memU16 = null;
    this.memU32 = null;
    this.memF32 = null;
    this.serialWasm = null; // For reset, so we don't try to download twice.
    this.currentUrl = null;
    
    // Owner should fill this in before loading.
    this.env = {
      rand: () => Math.floor(Math.random() * 0x10000),
      sinf: x => Math.sin(x),
      cosf: x => Math.cos(x),
      roundf: x => Math.round(x),
      atan2f: (x, y) => Math.atan2(x, y),
    };
  }
  
  load(url) {
    this.abort();
    return this.acquireSerial(url).then((serial) => {
      this.serialWasm = serial;
      this.currentUrl = url;
      return this.window.WebAssembly.instantiate(serial, this.generateOptions());
    }).then((result) => {
      this.instance = result.instance;
      this.memU8 = new Uint8Array(this.instance.exports.memory.buffer);
      this.memU16 = new Uint16Array(this.instance.exports.memory.buffer);
      this.memU32 = new Uint32Array(this.instance.exports.memory.buffer);
      this.memF32 = new Float32Array(this.instance.exports.memory.buffer);
      return this.instance;
    });
  }
  
  acquireSerial(url) {
    if (this.serialWasm && (url === this.currentUrl)) return Promise.resolve(this.serialWasm);
    return this.window.fetch(url).then((rsp) => {
      if (rsp.ok) return rsp.arrayBuffer();
      if (rsp.status === 555) return rsp.json().then(e => { throw e; });
      throw rsp;
    });
  }
  
  abort() {
    this.instance = null;
    this.memU8 = null;
    this.memU16 = null;
    this.memU32 = null;
    this.memF32 = null;
  }
  
  generateOptions() {
    return {
      env: this.env,
    };
  }
  
  zstringFromMemory(p) {
    if (!this.instance) throw new Error(`WASM instance not loaded`);
    if (typeof p !== "number") throw new Error(`expected integer address, got ${p}`);
    if ((p < 0) || (p >= this.memU8.length)) throw new Error(`invalid address ${p}`);
    const sanityLimit = 1024;
    let c = 0;
    while ((p + c < this.memU8.length) && this.memU8[p + c] && (c < sanityLimit)) c++;
    return new TextDecoder("utf-8").decode(this.memU8.slice(p, p + c));
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
}

WasmLoader.singleton = true;
