/* Renderer.js
 */
 
import { Dom } from "../util/Dom.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";
 
export class Renderer {
  static getDependencies() {
    return [Dom, Constants, DataService, Globals];
  }
  constructor(dom, constants, dataService, globals) {
    this.dom = dom;
    this.constants = constants;
    this.dataService = dataService;
    this.globals = globals;
  
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
    if (this.redrawMapAtNextRender) {
      this.renderMap();
      this.redrawMapAtNextRender = false;
    }
    const ctx = this.canvas.getContext("2d");
    ctx.drawImage(this.mapCanvas, 0, 0);
    
    let srcImage = null;
    let srcImageId = 0;
    let spritei = this.globals.g_spritec[0];
    if (spritei > 0) {
      const tilesize = this.constants.TILESIZE;
      const halftile = tilesize >> 1;
      let spritepp = this.globals.g_spritev[0] >> 2;
      for (; spritei-->0; spritepp++) {
     
        const sprite = this.globals.getSpriteByAddress(this.globals.memU32[spritepp]);
        
        // Ensure image loaded. NB This is for all sprite render types, not just the generic.
        if (sprite.imageid !== srcImageId) {
          srcImage = this.dataService.loadImageSync(sprite.imageid);
          srcImageId = sprite.imageid;
        }
        if (!srcImage) continue;
        
        if (true) { // TODO if hero...
          this.renderHero(ctx, srcImage, sprite.x, sprite.y);
        
        } else { // generic single-tile sprites
          this.renderTile(ctx, srcImage, sprite.x, sprite.y, sprite.tileid, sprite.xform);
        }
      }
    }
  }
  
  renderHero(ctx, srcImage, midx, midy) {
    const tilesize = this.constants.TILESIZE;
    const halftile = tilesize >> 1;
    const left = ~~(midx * tilesize - halftile);
    const facedir = this.globals.g_facedir[0];
    const walking = this.globals.g_walking[0];
    
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
    const cellv = this.globals.g_map;
    let cellp = 0;
    const imageId = this.globals.g_maptsid[0];
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
        if (cellv[cellp]) {
          this.renderTile(ctx, dstx, dsty, tilesheet, cellv[cellp], 0);
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
