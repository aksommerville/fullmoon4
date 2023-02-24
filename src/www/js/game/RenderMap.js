/* RenderMap.js
 * Subservient to Renderer, manages the background images that update infrequently.
 */
 
import { Dom } from "../util/Dom.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";
import { RenderBasics } from "./RenderBasics.js";
import { ChalkMenu } from "./Menu.js";
 
export class RenderMap {
  static getDependencies() {
    return [Dom, Constants, Globals, DataService, RenderBasics];
  }
  constructor(dom, constants, globals, dataService, renderBasics) {
    this.dom = dom;
    this.constants = constants;
    this.globals = globals;
    this.dataService = dataService;
    this.renderBasics = renderBasics;
    
    this.ILLUMINATION_PERIOD = 70; // frames. figure about 60 Hz, but it needn't be precise
    
    this.dirty = false;
    this.awaitingTilesheet = false;
    this.canvas = this.dom.createElement("CANVAS", {
      width: this.constants.COLC * this.constants.TILESIZE,
      height: this.constants.COLC * this.constants.TILESIZE,
    });
    this.illuminationPhase = ~~(this.ILLUMINATION_PERIOD * 0.25);
  }
  
  setDirty() {
    this.dirty = true;
  }
  
  // Redraws if needed, and returns the map image.
  update() {
    if (this.dirty) {
      this._render(this.canvas);
      this.dirty = false;
    }
    return this.canvas;
  }
  
  /* If the map is darkened, check illumination status and darken the entire framebuffer accordingly.
   * No darkening in play, we quickly no-op.
   */
  renderDarkness(canvas, ctx) {
    if (!this.globals.g_mapdark[0]) return;
    const countdown = this.globals.g_illumination_time[0];
    if (!countdown) {
      ctx.fillStyle = "#000";
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      this.illuminationPhase = ~~(this.ILLUMINATION_PERIOD * 0.25);
      return;
    }
    this.illuminationPhase++;
    if (this.illuminationPhase >= this.ILLUMINATION_PERIOD) this.illuminationPhase = 0;
    ctx.globalAlpha = Math.sin((this.illuminationPhase * Math.PI * 2) / this.ILLUMINATION_PERIOD) * 0.25 + 0.25;
    if (countdown < 1) {
      ctx.globalAlpha += (1 - ctx.globalAlpha) * (1 - countdown);
    }
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.globalAlpha = 1;
  }
  
  /* Private.
   *********************************************************************/
   
  _render(dst) {
    const ctx = dst.getContext("2d");
    const cellv = this.globals.g_map;
    let cellp = 0;
    const imageId = this.globals.g_maptsid[0];
    const tilesheet = this.dataService.getImage(imageId);
    
    // Black out and wait for the tilesheet if it's null or incomplete.
    // This is normal for the first few frames after startup, shouldn't happen after that.
    if (!tilesheet || !tilesheet.complete) {
      ctx.fillStyle = "#000";
      ctx.fillRect(0, 0, dst.width, dst.height);
      if (tilesheet && !this.awaitingTilesheet) {
        this.awaitingTilesheet = true;
        tilesheet.addEventListener("load", () => {
          this.setDirty();
          this.awaitingTilesheet = false;
        });
      }
      return;
    }
    this.awaitingTilesheet = false;
    
    const tilesize = this.constants.TILESIZE;
    for (let dsty=tilesize>>1, yi=this.constants.ROWC; yi-->0; dsty+=tilesize) {
      for (let dstx=tilesize>>1, xi=this.constants.COLC; xi-->0; dstx+=tilesize, cellp++) {
        this.renderBasics.tile(ctx, dstx, dsty, tilesheet, 0);
        if (cellv[cellp]) {
          this.renderBasics.tile(ctx, dstx, dsty, tilesheet, cellv[cellp], 0);
        }
      }
    }
    
    this.globals.forEachSketch(sketch => {
      if (sketch.bits) {
        let outx = sketch.x * tilesize;
        let outy = sketch.y * tilesize;
        const colw = Math.floor(tilesize / 3);
        const margin = ((tilesize - colw * 2) >> 1) + 0.5;
        outx += margin;
        outy += margin;
        ctx.beginPath();
        for (let mask=0x80000; mask; mask>>=1) {
          if (sketch.bits & mask) {
            const [ax, ay, bx, by] = ChalkMenu.pointsFromBit(mask);
            ctx.moveTo(outx + ax * colw, outy + ay * colw);
            ctx.lineTo(outx + bx * colw, outy + by * colw);
          }
        }
        ctx.strokeStyle = "#fff";
        ctx.stroke();
      }
    });
  }
}

RenderMap.singleton = true;
