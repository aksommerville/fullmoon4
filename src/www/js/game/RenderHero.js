/* RenderHero.js
 * Subservient to RenderSprites, manages just the hero sprite.
 * This one sprite is complicated enough to deserve its own class.
 */
 
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { RenderBasics } from "./RenderBasics.js";
 
export class RenderHero {
  static getDependencies() {
    return [Constants, Globals, RenderBasics];
  }
  constructor(constants, globals, renderBasics) {
    this.constants = constants;
    this.globals = globals;
    this.renderBasics = renderBasics;
    
    this.frameCount = 0;
  }
  
  render(ctx, srcImage, sprite) {
    this.frameCount++;
    
    const midx = sprite.x;
    const midy = sprite.y;
    const tilesize = this.constants.TILESIZE;
    const halftile = tilesize >> 1;
    const left = ~~(midx * tilesize - halftile);
    const facedir = this.globals.g_facedir[0];
    const walking = this.globals.g_walking[0];
    
    // When injured, we do something entirely different, and all other state can be ignored.
    if (this.globals.g_injury_time[0] > 0) {
      const tileId = (this.frameCount & 4) ? 0x03 : 0x33;
      let hatDisplacement = 0;
      if (this.globals.g_injury_time[0] >= 0.8) ;
      else if (this.globals.g_injury_time[0] >= 0.4) hatDisplacement = Math.floor((0.8 - this.globals.g_injury_time[0]) * 20);
      else hatDisplacement = Math.floor(this.globals.g_injury_time[0] * 20);
      this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize, srcImage, tileId + 0x20, 0);
      this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 7, srcImage, tileId + 0x10, 0);
      this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 12 - hatDisplacement, srcImage, tileId, 0);
      return;
    }
    
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
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize, srcImage, bodyFrame, xform);
      
    // Head.
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 7, srcImage, 0x10 + col, xform);
      
    // Hat.
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 12, srcImage, 0x00 + col, xform);
  }
}

RenderHero.singleton = true;
