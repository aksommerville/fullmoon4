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
    this.frameCount = 0; // increments at each render
    
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
    this.frameCount++;
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
      
        // Read sprite header.
        let spritep = this.wasmLoader.memU32[spritepp];
        const midx = this.wasmLoader.memF32[spritep >> 2]; spritep += 4;
        const midy = this.wasmLoader.memF32[spritep >> 2]; spritep += 4;
        const imageId = this.wasmLoader.memU8[spritep++];
        const tileId = this.wasmLoader.memU8[spritep++];
        const xform = this.wasmLoader.memU8[spritep++];
        
        // Ensure image loaded. NB This is for all sprite render types, not just the generic.
        if (imageId !== srcImageId) {
          srcImage = this.dataService.loadImageSync(imageId);
          srcImageId = imageId;
        }
        if (!srcImage) continue;
        
        if (true) { // TODO if hero...
          this.renderHero(ctx, srcImage, midx, midy, globalp);
        
        } else { // generic single-tile sprites
          this.renderTile(ctx, srcImage, midx, midy, tileId, xform);
        }
      }
    }
  }
  
  renderHero(ctx, srcImage, midx, midy, globalp) {
    const tilesize = this.constants.TILESIZE;
    const halftile = tilesize >> 1;
    const left = ~~(midx * tilesize - halftile);
    let memp = globalp // at (selected_item). TODO maybe don't depend so hard on struct layout...
      + 8 
      + this.constants.COLC * this.constants.ROWC + 12
      + this.constants.PLANT_LIMIT * this.constants.PLANT_SIZE
      + this.constants.SKETCH_LIMIT * this.constants.SKETCH_SIZE
      + 8;
    const selectedItem = this.wasmLoader.memU8[memp];
    const activeItem = this.wasmLoader.memU8[memp + 1];
    memp += 36; // skip to Hero section
    const facedir = this.wasmLoader.memU8[memp++];
    const walking = this.wasmLoader.memU8[memp];
    
    // The most common tiles are arranged in the first three columns: DOWN, UP, LEFT.
    let col = 0, xform = 0;
    switch (facedir) {
      case this.constants.DIR_N: col = 1; break;
      case this.constants.DIR_W: col = 2; break;
      case this.constants.DIR_E: col = 2; xform = this.constants.XFORM_XREV; break;
    }
    
    // Body, centered on (mid).
    let bodyFrame = 0x20 + col;
    if (walking) {
      const bodyClock = Math.floor((this.frameCount % 36) / 2);
      bodyFrame += 0x10 * [0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 3, 3, 4, 4, 4, 3, 3][bodyClock];
    }
    this.renderTile(ctx, midx * tilesize, midy * tilesize, srcImage, bodyFrame, xform);
      
    // Head.
    this.renderTile(ctx, midx * tilesize, midy * tilesize - 7, srcImage, 0x10 + col, xform);
      
    // Hat.
    this.renderTile(ctx, midx * tilesize, midy * tilesize - 12, srcImage, 0x00 + col, xform);
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
    for (let dsty=tilesize>>1, yi=this.constants.ROWC; yi-->0; dsty+=tilesize) {
      for (let dstx=tilesize>>1, xi=this.constants.COLC; xi-->0; dstx+=tilesize, cellp++) {
        this.renderTile(ctx, dstx, dsty, tilesheet, 0);
        if (this.wasmLoader.memU8[cellp]) {
          this.renderTile(ctx, dstx, dsty, tilesheet, this.wasmLoader.memU8[cellp], 0);
        }
      }
    }
  }
  
  renderTile(ctx, midx, midy, srcImage, tileid, xform) {
    const tilesize = this.constants.TILESIZE;
    if (!xform) {
      ctx.drawImage(srcImage,
        (tileid & 0x0f) * tilesize, (tileid >> 4) * tilesize, tilesize, tilesize,
        ~~(midx - (tilesize >> 1)), ~~(midy - (tilesize >> 1)), tilesize, tilesize
      );
    } else {
      ctx.save();
      ctx.translate(~~midx, ~~midy);
      switch (xform & (this.constants.XFORM_XREV | this.constants.XFORM_YREV)) {
        case this.constants.XFORM_XREV: ctx.scale(-1, 1); break;
        case this.constants.XFORM_YREV: ctx.scale(1, -1); break;
        case this.constants.XFORM_XREV | this.constants.XFORM_YREV: ctx.scale(-1, -1); break;
      }
      //TODO XFORM_SWAP
      ctx.drawImage(srcImage,
        (tileid & 0x0f) * tilesize, (tileid >> 4) * tilesize, tilesize, tilesize,
        -(tilesize >> 1), -(tilesize >> 1), tilesize, tilesize
      );
      ctx.restore();
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
