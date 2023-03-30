/* RenderViolin.js
 * Renders the chart overlay, while a violin is being played.
 */
 
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { RenderBasics } from "./RenderBasics.js";
import { DataService } from "./DataService.js";

export class RenderViolin {
  static getDependencies() {
    return [Constants, Globals, RenderBasics, DataService];
  }
  constructor(constants, globals, renderBasics, dataService) {
    this.constants = constants;
    this.globals = globals;
    this.renderBasics = renderBasics;
    this.dataService = dataService;
    
    this.running = false;
    this.verticalPosition = 0; // 0=top, 1=bottom
    this.bounds = null; // Can be null even if running, if the canvas is too small or something.
    this.srcImage = null;
  }
  
  // Renderer should call at the first frame that we're not needed.
  reset() {
    this.running = false;
  }
  
  render(canvas) {
    if (!this.running) {
      this._startup(canvas);
    }
    const ctx = canvas.getContext("2d");
    if (!this.bounds) return;
    
    // 1-pixel top and bottom border, and 1-pixel shadow on bottom, all within the bounds.
    ctx.beginPath();
    ctx.moveTo(this.bounds.x, this.bounds.y + 0.5);
    ctx.lineTo(this.bounds.x + this.bounds.w, this.bounds.y + 0.5);
    ctx.moveTo(this.bounds.x, this.bounds.y + this.bounds.h - 1.5);
    ctx.lineTo(this.bounds.x + this.bounds.w, this.bounds.y + this.bounds.h - 1.5);
    ctx.strokeStyle = "#000";
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(this.bounds.x, this.bounds.y + this.bounds.h - 0.5);
    ctx.lineTo(this.bounds.x + this.bounds.w, this.bounds.y + this.bounds.h - 0.5);
    ctx.globalAlpha = 0.25;
    ctx.stroke();
    ctx.globalAlpha = 1;
    
    // White-ish background, within that border.
    if (!(this.globals.g_violin_songp[0] & 1) && (this.globals.g_violin_clock[0] < 0.5)) {
      const aweight = this.globals.g_violin_clock[0] * 2;
      const bweight = 1 - aweight; // 'b' color is the constant below
      const r = ~~(0xc0 * aweight + 0xf8 * bweight);
      const g = ~~(0xb0 * aweight + 0xf4 * bweight);
      const b = ~~(0xa0 * aweight + 0xe0 * bweight);
      ctx.fillStyle = `rgb(${r},${g},${b})`;
    } else {
      ctx.fillStyle = "#f8f4e0";
    }
    ctx.fillRect(this.bounds.x, this.bounds.y + 1, this.bounds.w, this.bounds.h - 3);
    
    // Staff lines. Basically static, but they'll change color as you play a note.
    const staffLineColors = ["#864", "#864", "#864", "#864"];
    switch (this.globals.g_wand_dir[0]) {
      case this.constants.DIR_N: staffLineColors[0] = "#c00"; break;
      case this.constants.DIR_E: staffLineColors[1] = "#c00"; break;
      case this.constants.DIR_W: staffLineColors[2] = "#c00"; break;
      case this.constants.DIR_S: staffLineColors[3] = "#c00"; break;
    }
    const staffLineSpacing = Math.floor(this.bounds.h / 5);
    for (let i=0, y=this.bounds.y+1.5+staffLineSpacing; i<4; i++, y+=staffLineSpacing) {
      ctx.beginPath();
      ctx.moveTo(this.bounds.x, y);
      ctx.lineTo(this.bounds.x + this.bounds.w, y);
      ctx.strokeStyle = staffLineColors[i];
      ctx.stroke();
    }
    
    // Measure bars, every fourth beat.
    const noteSpacing = this.bounds.w / this.constants.VIOLIN_SONG_LENGTH;
    const barSpacing = noteSpacing * 4;
    let barx = this.bounds.x - (this.globals.g_violin_songp[0] + this.globals.g_violin_clock[0] - 2.5) * noteSpacing;
    const bartop = this.bounds.y + 1 + staffLineSpacing;
    const barbtm = bartop + staffLineSpacing * 3;
    ctx.beginPath();
    for (; barx<this.bounds.x+this.bounds.w; barx+=barSpacing) {
      ctx.moveTo(~~barx, bartop);
      ctx.lineTo(~~barx, barbtm);
    }
    ctx.strokeStyle = "#864";
    ctx.stroke();
    
    // And finally the notes. The oldest note is usually off-screen.
    if (this.srcImage && this.srcImage.complete) {
      let x = this.bounds.x - this.globals.g_violin_clock[0] * noteSpacing;
      let srcp = this.globals.g_violin_songp[0];
      const srcp0=srcp;
      for (let i=this.constants.VIOLIN_SONG_LENGTH; i-->0; x+=noteSpacing, srcp++) {
        if (srcp >= this.constants.VIOLIN_SONG_LENGTH) srcp = 0;
        switch (this.globals.g_violin_song[srcp]) {
          case this.constants.DIR_N: this._drawNote(ctx, 0xc4, ~~x, ~~(this.bounds.y + 1.5 + staffLineSpacing * 1) - 3); break;
          case this.constants.DIR_E: this._drawNote(ctx, 0xc5, ~~x, ~~(this.bounds.y + 1.5 + staffLineSpacing * 2) + 4); break;
          case this.constants.DIR_W: this._drawNote(ctx, 0xc6, ~~x, ~~(this.bounds.y + 1.5 + staffLineSpacing * 3) - 3); break;
          case this.constants.DIR_S: this._drawNote(ctx, 0xc7, ~~x, ~~(this.bounds.y + 1.5 + staffLineSpacing * 4) + 4); break;
        }
      }
    }
  }
  
  _drawNote(ctx, tileId, x, y) {
    this.renderBasics.tile(ctx, x, y, this.srcImage, tileId, 0);
  }
  
  _startup(canvas) {
    this.running = true;
    const hero = this.globals.getHeroSprite();
    if (hero) {
      if (hero.y < this.constants.ROWC >> 1) this.verticalPosition = 1;
      else this.verticalPosition = 0;
    } else {
      this.verticalPosition = 1;
    }
    this.bounds = this._selectBounds(canvas.width, canvas.height);
    if (!this.srcImage) {
      this.srcImage = this.dataService.getImage(2);
    }
  }
  
  _selectBounds(fullw, fullh) {
    // No particular reason for a minimum of TILESIZE, just a sanity check.
    if (!fullw || (fullw < this.constants.TILESIZE)) return null;
    if (!fullh || (fullh < this.constants.TILESIZE)) return null;
    const w = fullw;
    const h = ~~(fullh / 3);
    const x = 0;
    let y = 10; // Arbitrary margin.
    if (this.verticalPosition) {
      y = fullh - h - y;
    }
    return { x, y, w, h };
  }
}

RenderViolin.singleton = true;
