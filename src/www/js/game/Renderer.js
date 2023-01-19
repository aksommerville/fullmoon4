/* Renderer.js
 */
 
import { Dom } from "../util/Dom.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";

// Must match fmn_platform.h
const FMN_SPRITE_STYLE_HIDDEN = 0;
const FMN_SPRITE_STYLE_TILE = 1;
const FMN_SPRITE_STYLE_HERO = 2;
 
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
    this.transitionStartTime = 0;
    this.transitionFocusX = 0; // used by DOOR only
    this.transitionFocusY = 0; // ''
    
    this.redrawMapAtNextRender = false;
    this.canvas = null;
    this.frameCount = 0; // increments at each render
    
    // mapCanvas holds the current background image
    this.mapCanvas = this.dom.createElement("CANVAS");
    this.mapCanvas.width = this.constants.COLC * this.constants.TILESIZE;
    this.mapCanvas.height = this.constants.ROWC * this.constants.TILESIZE;
    
    // transitionCanvas holds the new-state image during a transition.
    this.transitionCanvas = this.dom.createElement("CANVAS");
    this.transitionCanvas.width = this.constants.COLC * this.constants.TILESIZE;
    this.transitionCanvas.height = this.constants.ROWC * this.constants.TILESIZE;
    
    // previousCanvas holds a static capture of the outgoing screen during a transition.
    this.previousCanvas = this.dom.createElement("CANVAS");
    this.previousCanvas.width = this.constants.COLC * this.constants.TILESIZE;
    this.previousCanvas.height = this.constants.ROWC * this.constants.TILESIZE;
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
    const hero = this.render(this.previousCanvas, true);
    this.transitionMode = tmode;
    this.transitionFocusX = hero.x;
    this.transitionFocusY = hero.y;
  }
  
  commitTransition() {
    this.transitionStartTime = Date.now();
  }
  
  cancelTransition() {
    this.transitionMode = 0;
  }
  
  /* Main rendering.
   * Call render() with no argument for the default, rendering with transition to the previously-attached canvas.
   *********************************************************/
   
  render(target, hideHero) {
  
    if (!target) {
      if (!this.canvas) return;
      if (this.transitionMode) {
        const now = Date.now();
        const tp = (now - this.transitionStartTime) / this.constants.TRANSITION_TIME_MS;
        if (tp < 0) { // Panic! Drop the transition.
          this.transitionMode = 0;
        } else if (tp >= 1) { // Transition complete.
          this.transitionMode = 0;
        } else { // Transition in progress.
          const hero = this.render(this.transitionCanvas, true);
          const ctx = this.canvas.getContext("2d");
          this.renderTransition(ctx, this.previousCanvas, this.transitionCanvas, tp, this.transitionMode, hero);
          return;
        }
      }
      target = this.canvas;
    }
    
    this.frameCount++;
    if (this.redrawMapAtNextRender) {
      this.renderMap();
      this.redrawMapAtNextRender = false;
    }
    const ctx = target.getContext("2d");
    ctx.drawImage(this.mapCanvas, 0, 0);
    
    let hero = null;
    let srcImage = null;
    let srcImageId = 0;
    let spritei = this.globals.g_spritec[0];
    if (spritei > 0) {
      const tilesize = this.constants.TILESIZE;
      const halftile = tilesize >> 1;
      let spritepp = this.globals.g_spritev[0] >> 2;
      for (; spritei-->0; spritepp++) {
     
        const sprite = this.globals.getSpriteByAddress(this.globals.memU32[spritepp]);
        if (sprite.style === FMN_SPRITE_STYLE_HIDDEN) continue;
        
        // Ensure image loaded. NB This is for all sprite render types, not just the generic.
        if (sprite.imageid !== srcImageId) {
          srcImage = this.dataService.loadImageSync(sprite.imageid);
          srcImageId = sprite.imageid;
        }
        if (!srcImage) continue;
        
        switch (sprite.style) {
          case FMN_SPRITE_STYLE_TILE: this.renderTile(ctx, sprite.x * tilesize, sprite.y * tilesize, srcImage, sprite.tileid, sprite.xform); break;
          case FMN_SPRITE_STYLE_HERO: {
              if (hideHero) {
                hero = sprite;
                continue;
              }
              this.renderHero(ctx, srcImage, sprite.x, sprite.y);
            } break;
        }
      }
    }
    return hero;
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
    
    // Physics for debugging.
    if (false) {
      ctx.beginPath();
      ctx.arc(midx * tilesize, midy * tilesize, 0.200 * tilesize, 0, Math.PI * 2);
      ctx.fillStyle = "#f00";
      ctx.globalAlpha = 0.5;
      ctx.fill();
      ctx.globalAlpha = 1.0;
    }
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
  
  /* Transitions.
   **************************************************************/
   
  renderTransition(ctx, prev, next, p, mode, hero) {
    switch (mode) {
      case this.constants.TRANSITION_PAN_LEFT: return this.renderPan(ctx, prev, next, p, 1, 0, hero);
      case this.constants.TRANSITION_PAN_RIGHT: return this.renderPan(ctx, prev, next, p, -1, 0, hero);
      case this.constants.TRANSITION_PAN_UP: return this.renderPan(ctx, prev, next, p, 0, 1, hero);
      case this.constants.TRANSITION_PAN_DOWN: return this.renderPan(ctx, prev, next, p, 0, -1, hero);
      
      case this.constants.TRANSITION_DOOR: return this.renderTransitionDoor(ctx, prev, next, p, hero);
      
      case this.constants.TRANSITION_FADE_BLACK:
      case this.constants.TRANSITION_WARP: {
          //TODO For now, the "other" transitions are all a simple cross-fade:
          ctx.drawImage(prev, 0, 0);
          ctx.globalAlpha = p;
          ctx.drawImage(next, 0, 0);
          ctx.globalAlpha = 1.0;
          this.renderHero(ctx, this.dataService.loadImageSync(hero.imageid), hero.x, hero.y);
        } return;
    }
    
    // All else fails, just use the incoming image.
    ctx.drawImage(next, 0, 0);
    this.renderHero(ctx, this.dataService.loadImageSync(hero.imageid), hero.x, hero.y);
  }
  
  renderPan(ctx, prev, next, p, dx, dy, hero) {
    let dstx = dx * p * prev.width;
    let dsty = dy * p * prev.height;
    ctx.drawImage(prev, dstx, dsty);
    dstx -= dx * prev.width;
    dsty -= dy * prev.height;
    ctx.drawImage(next, dstx, dsty);
    dstx /= this.constants.TILESIZE;
    dsty /= this.constants.TILESIZE;
    this.renderHero(ctx, this.dataService.loadImageSync(hero.imageid), hero.x + dstx, hero.y + dsty);
  }
  
  renderTransitionDoor(ctx, prev, next, p, hero) {
    if (p < 0.5) this.renderTransitionDoor1(ctx, prev, (0.5 - p) * 2, null, this.transitionFocusX, this.transitionFocusY);
    else this.renderTransitionDoor1(ctx, next, (p - 0.5) * 2, hero, hero.x, hero.y);
  }
  renderTransitionDoor1(ctx, src, p, hero, focusx, focusy) {
    ctx.drawImage(src, 0, 0);
    if (hero) this.renderHero(ctx, this.dataService.loadImageSync(hero.imageid), hero.x, hero.y);
    
    let radiusLimit;
    if (focusx < this.constants.COLC / 2) {
      if (focusy < this.constants.ROWC / 2) {
        radiusLimit = Math.sqrt((this.constants.COLC - focusx) ** 2 + (this.constants.ROWC - focusy) ** 2);
      } else {
        radiusLimit = Math.sqrt((this.constants.COLC - focusx) ** 2 + focusy ** 2);
      }
    } else {
      if (focusy < this.constants.ROWC / 2) {
        radiusLimit = Math.sqrt(focusx ** 2 + (this.constants.ROWC - focusy) ** 2);
      } else {
        radiusLimit = Math.sqrt(focusx ** 2 + focusy ** 2);
      }
    }
    const radius = p * radiusLimit * this.constants.TILESIZE;
    focusx *= this.constants.TILESIZE;
    focusy *= this.constants.TILESIZE;
    
    ctx.beginPath();
    ctx.moveTo(focusx, 0);
    ctx.lineTo(focusx, focusy - radius);
    ctx.arcTo(focusx - radius, focusy - radius, focusx - radius, focusy, radius);
    ctx.arcTo(focusx - radius, focusy + radius, focusx, focusy + radius, radius);
    ctx.arcTo(focusx + radius, focusy + radius, focusx + radius, focusy, radius);
    ctx.arcTo(focusx + radius, focusy - radius, focusx, focusy - radius, radius);
    ctx.lineTo(focusx, 0);
    ctx.lineTo(this.canvas.width, 0);
    ctx.lineTo(this.canvas.width, this.canvas.height);
    ctx.lineTo(0, this.canvas.height);
    ctx.lineTo(0, 0);
    ctx.fillStyle = "#000";
    ctx.fill();
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
