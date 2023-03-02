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
    this.rain = null;
    this.wind = null;
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
  
  /* Rain, wind, and slowmo effects, as dictated by globals.
   */
  renderWeather(canvas, ctx) {
    if (this.globals.g_rain_time[0] > 0) {
      if (!this.rain) this.rain = this._initRain(canvas);
      this._renderRain(canvas, ctx, this.rain);
    } else {
      this.rain = null;
    }
    if (this.globals.g_wind_time[0] > 0) {
      if (!this.wind) this.wind = this._initWind(canvas, this.globals.g_wind_dir[0]);
      this._renderWind(canvas, ctx, this.wind);
    } else {
      this.wind = null;
    }
    // nothing special for slowmo, but if we want to, here is the place
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
    
    if (this.globals.g_plantc[0]) {
      const plantImage = this.dataService.getImage(2);
      if (plantImage && plantImage.complete) {
        this.globals.forEachPlant(plant => {
          const dstx = plant.x * this.constants.TILESIZE + (this.constants.TILESIZE >> 1);
          const dsty = plant.y * this.constants.TILESIZE + (this.constants.TILESIZE >> 1);
          let tileId = 0x3a + 0x10 * plant.state;
          this.renderBasics.tile(ctx, dstx, dsty, plantImage, tileId, 0);
          if ((plant.state === this.constants.PLANT_STATE_FLOWER) && plant.fruit) {
            this.renderBasics.tile(ctx, dstx, dsty, plantImage, 0x2b + 0x10 * plant.fruit, 0);
          }
        });
      }
    }
  }
  
  _initRain(canvas) {
    const rain = {
      particles: [],
    };
    for (let i=400; i-->0; ) {
      rain.particles.push({
        x: Math.floor(Math.random() * canvas.width) + 0.5,
        y: Math.floor(Math.random() * canvas.height) + 0.5
      });
    }
    return rain;
  }
  
  _renderRain(canvas, ctx, rain) {
    ctx.beginPath();
    for (const particle of rain.particles) {
      particle.y += 2;
      if (particle.y >= canvas.height) {
        particle.y = -2;
        particle.x = Math.floor(Math.random() * canvas.width) + 0.5;
      }
      ctx.moveTo(particle.x, particle.y);
      ctx.lineTo(particle.x, particle.y + 2);
    }
    ctx.strokeStyle = "#008";
    ctx.globalAlpha = 0.5;
    ctx.stroke();
    ctx.globalAlpha = 1;
  }
  
  _initWind(canvas, dir) {
    const wind = {
      dir,
      dx: 0,
      dy: 0,
      particles: [],
    }
    const speed = 4;
    switch (dir) {
      case 0x40: wind.dy = -speed; break;
      case 0x10: wind.dx = -speed; break;
      case 0x08: wind.dx = speed; break;
      case 0x02: wind.dy = speed; break;
    }
    for (let i=200; i-->0; ) {
      wind.particles.push({
        x: Math.random() * canvas.width,
        y: Math.random() * canvas.height,
      });
    }
    return wind;
  }
  
  _renderWind(canvas, ctx, wind) {
    ctx.beginPath();
    const linex = wind.dx ? 6 : 0;
    const liney = wind.dy ? 6 : 0;
    for (const particle of wind.particles) {
      particle.x += wind.dx;
      particle.y += wind.dy;
      if (particle.x < 0) {
        particle.x = canvas.width;
        particle.y = Math.random() * canvas.height;
      } else if (particle.y < 0) {
        particle.x = Math.random() * canvas.width;
        particle.y = canvas.height;
      } else if (particle.x >= canvas.width) {
        particle.x = 0;
        particle.y = Math.random() * canvas.height;
      } else if (particle.y >= canvas.height) {
        particle.x = Math.random() * canvas.width;
        particle.y = 0;
      }
      ctx.moveTo(particle.x, particle.y);
      ctx.lineTo(particle.x + linex, particle.y + liney);
    }
    ctx.strokeStyle = "#ccc";
    ctx.globalAlpha = 0.7;
    ctx.stroke();
    ctx.globalAlpha = 1;
  }
}

RenderMap.singleton = true;
