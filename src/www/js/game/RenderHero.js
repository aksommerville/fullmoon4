/* RenderHero.js
 * Subservient to RenderSprites, manages just the hero sprite.
 * This one sprite is complicated enough to deserve its own class.
 */
 
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { RenderBasics } from "./RenderBasics.js";
import { DataService } from "./DataService.js";
 
export class RenderHero {
  static getDependencies() {
    return [Constants, Globals, RenderBasics, DataService];
  }
  constructor(constants, globals, renderBasics, dataService) {
    this.constants = constants;
    this.globals = globals;
    this.renderBasics = renderBasics;
    this.dataService = dataService;
    
    /* Indexed by item ID 0..15.
     * cbActive if present returns a replacement for the layout, called when item is actively being used.
     * null or [tileId, dx, dy, xform, cbActive, cbAlways]
     * (xform) null for default (flip horizontally if facing E or N).
     */
    this.itemCarryLayout = [
      /* NONE     */ null,
      /* CORN     */ [0x34, -5, -3, null, null, () => (this.globals.g_itemqv[this.constants.ITEM_CORN] ? this.itemCarryLayout[this.constants.ITEM_CORN] : null)],
      /* PITCHER  */ [0x55, -5, -3, null, () => this._animatePitcher(), null],
      /* SEED     */ [0x34, -5, -3, null, null, () => (this.globals.g_itemqv[this.constants.ITEM_SEED] ? this.itemCarryLayout[this.constants.ITEM_SEED] : null)],
      /* COIN     */ [0x44, -5, -3, null, null, () => (this.globals.g_itemqv[this.constants.ITEM_COIN] ? this.itemCarryLayout[this.constants.ITEM_COIN] : null)],
      /* MATCH    */ [0x06, -5, -3, null, null, () => this._animateMatch()],
      /* BROOM    */ null,
      /* WAND     */ null,
      /* UMBRELLA */ [0x46, -5, -8, null, () => this._animateUmbrella(), null],
      /* FEATHER  */ [0x05, -5, -3, null, () => this._animateFeather(), null],
      /* SHOVEL   */ null,
      /* COMPASS  */ [0x64, -5, -3, null, null, null],
      /* VIOLIN   */ null,
      /* CHALK    */ null,
      /* BELL     */ [0x04, -5, -3, null, () => [(this.carryActiveTime & 16) ? 0x14 : 0x24, -5, -3, null], null],
      /* CHEESE   */ [0x54, -5, -3, null, null, () => (this.globals.g_itemqv[this.constants.ITEM_CHEESE] ? this.itemCarryLayout[this.constants.ITEM_CHEESE] : null)],
    ];
    
    this.frameCount = 0;
    this.carryActiveTime = 0;
    this.compassAngle = 0;
    this.compassRateMax = 0.200;
    this.compassRateMin = 0.010;
    this.compassDistanceMax = 40;
    this.compassDistanceMin = 1;
  }
  
  render(ctx, srcImage, sprite) {
    this.frameCount++;
    
    const midx = sprite.x;
    const midy = sprite.y;
    const tilesize = this.constants.TILESIZE;
    const halftile = tilesize >> 1;
    const left = ~~(midx * tilesize - halftile);
    const facedir = this.globals.g_facedir[0];
    
    // When injured, we do something entirely different, and all other state can be ignored.
    //TODO Does the carrying item matter for this?
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
    
    // Render order depends on facedir, mostly for the carrying item, should be occluded when facing N.
    if (facedir === this.constants.DIR_N) {
      this._renderItem(ctx, midx, midy, facedir, srcImage);
      this._renderBody(ctx, midx, midy, facedir, srcImage, col, xform);
      this._renderHead(ctx, midx, midy, facedir, srcImage, col, xform);
      this._renderHat(ctx, midx, midy, facedir, srcImage, col, xform);
    } else if (facedir === this.constants.DIR_S) {
      this._renderBody(ctx, midx, midy, facedir, srcImage, col, xform);
      this._renderHead(ctx, midx, midy, facedir, srcImage, col, xform);
      this._renderHat(ctx, midx, midy, facedir, srcImage, col, xform);
      this._renderItem(ctx, midx, midy, facedir, srcImage);
    } else {
      this._renderBody(ctx, midx, midy, facedir, srcImage, col, xform);
      this._renderItem(ctx, midx, midy, facedir, srcImage);
      this._renderHead(ctx, midx, midy, facedir, srcImage, col, xform);
      this._renderHat(ctx, midx, midy, facedir, srcImage, col, xform);
    }
  }
  
  _renderBody(ctx, midx, midy, facedir, srcImage, col, xform) {
    const tilesize = this.constants.TILESIZE;
    let bodyFrame = 0x20 + col;
    if (this.globals.g_walking[0]) {
      const bodyClock = Math.floor((this.frameCount % 36) / 2);
      bodyFrame += 0x10 * [0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 3, 3, 4, 4, 4, 3, 3][bodyClock];
    }
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize, srcImage, bodyFrame, xform);
  }
  
  _renderHead(ctx, midx, midy, facedir, srcImage, col, xform) {
    const tilesize = this.constants.TILESIZE;
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 7, srcImage, 0x10 + col, xform);
  }
  
  _renderHat(ctx, midx, midy, facedir, srcImage, col, xform) {
    const tilesize = this.constants.TILESIZE;
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 12, srcImage, 0x00 + col, xform);
  }
  
  _renderItem(ctx, midx, midy, facedir, srcImage) {
    const tilesize = this.constants.TILESIZE;
    if (!this.globals.g_itemv[this.globals.g_selected_item[0]]) return;
    let itemCarryLayout = this.itemCarryLayout[this.globals.g_selected_item[0]];
    if (!itemCarryLayout) return;
    
    // cbAlways
    if (itemCarryLayout[5]) {
      if (!(itemCarryLayout = itemCarryLayout[5]())) return;
    }
    
    // cbActive
    if (itemCarryLayout[4] && (this.globals.g_active_item[0] === this.globals.g_selected_item[0])) {
      this.carryActiveTime++;
      if (!(itemCarryLayout = itemCarryLayout[4]())) return;
    } else {
      this.carryActiveTime = 0;
    }
    
    // ok draw it!
    let xform = 0;
    const dsty = midy * tilesize + itemCarryLayout[2];
    let dstx = midx * tilesize;
    if ((itemCarryLayout[3] === null) && ((facedir === this.constants.DIR_E) || (facedir === this.constants.DIR_N))) {
      dstx -= itemCarryLayout[1];
      xform = this.constants.XFORM_XREV;
    } else {
      dstx += itemCarryLayout[1];
      xform = itemCarryLayout[3] || 0;
    }
    this.renderBasics.tile(ctx, dstx, dsty, srcImage, itemCarryLayout[0], xform);
  }
  
  _animateFeather() {
    let tileId = 0x05, dx = -5, dy = -3, xform = null;
    
    // The defaults are good for W and E. But if facing N or S, we do something pretty different.
    // Active feather must point in the direction the hero is facing.
    switch (this.globals.g_facedir[0]) {
      case this.constants.DIR_N: dx = 2; dy = -13; xform = this.constants.XFORM_SWAP; break;
      case this.constants.DIR_S: dx = -2; dy = 3; xform = this.constants.XFORM_SWAP | this.constants.XFORM_XREV; break;
    }
    
    switch (this.carryActiveTime & 0x18) {
      case 0x00: tileId += 0x10; break;
      case 0x08: tileId += 0x20; break;
      case 0x10: tileId += 0x30; break;
      case 0x18: tileId += 0x40; break;
    }
    return [tileId, dx, dy, xform];
  }
  
  _animatePitcher() {
    switch (this.globals.g_facedir[0]) {
      case this.constants.DIR_N: return [0x65, 0, -5, this.constants.XFORM_XREV | this.constants.XFORM_SWAP];
      case this.constants.DIR_S: return [0x65, -1, -1, this.constants.XFORM_XREV];
      // E or W, defaults are good.
      default: return [0x65, -5, -3, null];
    }
  }
  
  _animateMatch() {
    let tileId = 0x06;
    // Our animation is not based on active_item, but rather on illumination_time.
    // And we do show her holding a lit match when quantity is zero, her arm only disappears when the illumination ends
    if (this.globals.g_illumination_time[0] <= 0) {
      if (this.globals.g_itemqv[this.constants.ITEM_MATCH]) return [tileId, -5, -3, null];
      return null;
    }
    switch (this.frameCount & 0x18) {
      case 0x00: tileId += 0x10; break;
      case 0x08: tileId += 0x20; break;
      case 0x10: tileId += 0x30; break;
      case 0x18: tileId += 0x20; break;
    }
    return [tileId, -5, -3, null];
  }
  
  _animateUmbrella() {
    switch (this.globals.g_facedir[0]) {
      case this.constants.DIR_N: return [0x56, 0, -10, this.constants.XFORM_SWAP];
      case this.constants.DIR_S: return [0x56, 0, 5, this.constants.XFORM_SWAP | this.constants.XFORM_XREV];
      default: return [0x56, -9, -4, null];
    }
  }
  
  /* Overlay: Rendered after all sprites.
   **********************************************************/
   
  renderOverlay(dst, ctx) {
    switch (this.globals.g_selected_item[0]) {
      case this.constants.ITEM_COMPASS: this._renderCompassOverlay(dst, ctx); return;
    }
  }
  
  // Compass is always visible while selected.
  _renderCompassOverlay(dst, ctx) {
    const sprite = this.globals.getHeroSprite();
    if (!sprite) return;
    const srcImage = this.dataService.getImage(sprite.imageid);
    if (!srcImage) return;
    
    let [targetx, targety] = this.globals.g_compass;
    if (!targetx && !targety) { // (0,0) is special, means there's nothing to point to. spin fast.
      this.compassAngle += this.compassRateMax;
    } else {
      targetx = (targetx + 0.5);
      targety = (targety + 0.5);
      const targetDistance = Math.sqrt((targety - sprite.y) ** 2 + (targetx - sprite.x) ** 2);
      if (targetDistance >= this.compassDistanceMax) {
        this.compassAngle += this.compassRateMax;
      } else {
        const targetAngle = Math.atan2(targetx - sprite.x, sprite.y - targety);
        if (targetDistance <= this.compassDistanceMin) {
          this.compassAngle = targetAngle;
        } else {
          const normDist = (targetDistance - this.compassDistanceMin) / (this.compassDistanceMax - this.compassDistanceMin);
          const minRate = this.compassRateMin + (this.compassRateMax - this.compassRateMin) * normDist;
          let diff = this.compassAngle - targetAngle;
          if (diff > Math.PI) diff -= Math.PI * 2;
          else if (diff < -Math.PI) diff += Math.PI * 2;
          diff = Math.abs(diff) / Math.PI;
          this.compassAngle += minRate + (diff * (this.compassRateMax - minRate));
        }
      }
    }
    if (this.compassAngle >= Math.PI) this.compassAngle -= Math.PI * 2;
    
    const t = this.compassAngle;
    const radius = this.constants.TILESIZE;
    const midx = sprite.x * this.constants.TILESIZE;
    const midy = sprite.y * this.constants.TILESIZE;
    const dstx = midx + radius * Math.sin(t);
    const dsty = midy - radius * Math.cos(t);
    ctx.save();
    ctx.translate(dstx, dsty);
    ctx.rotate(t);
    ctx.drawImage(srcImage,
      3 * this.constants.TILESIZE, 6 * this.constants.TILESIZE, this.constants.TILESIZE, this.constants.TILESIZE,
      -(this.constants.TILESIZE >> 1), -(this.constants.TILESIZE >> 1), this.constants.TILESIZE, this.constants.TILESIZE
    );
    ctx.restore();
  }
}

RenderHero.singleton = true;
