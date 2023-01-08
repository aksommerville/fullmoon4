/* Renderer.js
 */
 
import { Dom } from "../util/Dom.js";
import { WasmLoader } from "../util/WasmLoader.js";
import { Constants } from "./Constants.js";
import { DataService } from "./DataService.js";
 
export class Renderer {
  static getDependencies() {
    return [Dom, Constants, WasmLoader, DataService];
  }
  constructor(dom, constants, wasmLoader, dataService) {
    this.dom = dom;
    this.constants = constants;
    this.wasmLoader = wasmLoader;
    this.dataService = dataService;
  
    // FMN_TRANSITION_*. 0 is CUT, which is effectively no-op.
    this.transitionMode = 0;
    
    this.redrawMapAtNextRender = false;
    this.canvas = null;
    
    this.mapCanvas = this.dom.createElement("CANVAS");
    this.mapCanvas.width = this.constants.COLC * this.constants.TILESIZE;
    this.mapCanvas.height = this.constants.ROWC * this.constants.TILESIZE;
  }
  
  mapDirty() {
    this.redrawMapAtNextRender = true;
  }
  
  setRenderTarget(canvas) {
    this.canvas = canvas;
    this.canvas.width = this.constants.COLC * this.constants.TILESIZE;
    this.canvas.height = this.constants.ROWC * this.constants.TILESIZE;
  }
  
  /* Transition.
   ***********************************************************/
  
  prepareTransition(tmode) {
    console.log(`Renderer.prepareTransition ${tmode}`);
    this.transitionMode = tmode;
    //TODO Capture framebuffer for "from" state.
  }
  
  commitTransition() {
    console.log(`Renderer.commitTransition`);
  }
  
  cancelTransition() {
    console.log(`Renderer.cancelTransition`);
    this.transitionMode = 0;
  }
  
  /* Main rendering.
   *********************************************************/
   
  render() {
    if (!this.canvas) return;
    const globalp = this.wasmLoader.instance.exports.fmn_global.value;
    if (this.redrawMapAtNextRender) {
      this.renderMap(globalp);
      this.redrawMapAtNextRender = false;
    }
    const ctx = this.canvas.getContext("2d");
    ctx.drawImage(this.mapCanvas, 0, 0);
    
    let srcImage = null;
    let srcImageId = 0;
    let spritei = this.wasmLoader.memU32[(globalp + 4) >> 2];
    if (spritei > 0) {
      const tilesize = this.constants.TILESIZE;
      const halftile = tilesize >> 1;
      let spritepp = this.wasmLoader.memU32[globalp >> 2] >> 2;
      for (; spritei-->0; spritepp++) {
        let spritep = this.wasmLoader.memU32[spritepp];
        const midx = this.wasmLoader.memF32[spritep >> 2]; spritep += 4;
        const midy = this.wasmLoader.memF32[spritep >> 2]; spritep += 4;
        const imageId = this.wasmLoader.memU8[spritep++];
        const tileId = this.wasmLoader.memU8[spritep++];
        const xform = this.wasmLoader.memU8[spritep++];
        //TODO Special sprites that aren't just a tile. (eg hero)
        if (imageId !== srcImageId) {
          srcImage = this.dataService.loadImageSync(imageId);
          srcImageId = imageId;
        }
        if (!srcImage) continue;
        const srcx = (tileId & 0x0f) * tilesize;
        const srcy = (tileId >> 4) * tilesize;
        const dstx = ~~(midx * tilesize - halftile);
        const dsty = ~~(midy * tilesize - halftile);
        ctx.drawImage(srcImage, srcx, srcy, tilesize, tilesize, dstx, dsty, tilesize, tilesize);
      }
    }
  }
  
  renderMap(globalp) {
    let cellp = globalp + 8;
    const imageId = this.wasmLoader.memU8[cellp + this.constants.COLC * this.constants.ROWC];
    const tilesheet = this.dataService.loadImageSync(imageId);
    const ctx = this.mapCanvas.getContext("2d");
    if (!tilesheet) {
      ctx.fillStyle = "#000";
      ctx.fillRect(0, 0, this.mapCanvas.width, this.mapCanvas.height);
      this.dataService.loadImage(imageId).then(() => {
        this.mapDirty();
      });
      return;
    }
    const tilesize = this.constants.TILESIZE;
    for (let dsty=0, yi=this.constants.ROWC; yi-->0; dsty+=tilesize) {
      for (let dstx=0, xi=this.constants.COLC; xi-->0; dstx+=tilesize, cellp++) {
        const tileId = this.wasmLoader.memU8[cellp];
        const srcx = (tileId & 0x0f) * tilesize;
        const srcy = (tileId >> 4) * tilesize;
        // Tile zero gets drawn behind every cell.
        ctx.drawImage(tilesheet, 0, 0, tilesize, tilesize, dstx, dsty, tilesize, tilesize);
        if (tileId) {
          ctx.drawImage(tilesheet, srcx, srcy, tilesize, tilesize, dstx, dsty, tilesize, tilesize);
        }
      }
    }
  }
}

Renderer.singleton = true;

// FMN_TRANSITION_*, must match C header.
Renderer.TRANSITION_CUT = 0;
Renderer.TRANSITION_PAN_LEFT = 1;
Renderer.TRANSITION_PAN_RIGHT = 2;
Renderer.TRANSITION_PAN_UP = 3;
Renderer.TRANSITION_PAN_DOWN = 4;
Renderer.TRANSITION_FADE_BLACK = 5;
Renderer.TRANSITION_DOOR = 6;
Renderer.TRANSITION_WARP = 7;
