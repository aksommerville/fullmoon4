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
      /* BROOM    */ [0x66, -5, -3, null, null, null], // active broom is a whole different deal
      /* WAND     */ [0x08, -5, -3, null, null, null], // '' wand
      /* UMBRELLA */ [0x46, -5, -8, null, () => this._animateUmbrella(), null],
      /* FEATHER  */ [0x05, -5, -3, null, () => this._animateFeather(), null],
      /* SHOVEL   */ [0x07, -5, -3, null, () => this._animateShovel(), null],
      /* COMPASS  */ [0x64, -5, -3, null, null, null],
      /* VIOLIN   */ [0x18, -5, 0, null, null, null], // active violin is a whole different deal
      /* CHALK    */ [0x58, -5, -3, null, null, null],
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
    this.cheesePhase = 0;
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
    
    // Repudiating a spell is its own thing.
    if (this.globals.g_spell_repudiation[0]) {
      this._renderSpellRepudiation(ctx, midx, midy, srcImage);
      return;
    }
    
    // Riding the broom is different enough to get its own top-level handler here.
    if (this.globals.g_active_item[0] === this.constants.ITEM_BROOM) {
      this._renderBroom(ctx, midx, midy, facedir, srcImage);
      return;
    }
    
    // Ditto encoding the wand.
    if (this.globals.g_active_item[0] === this.constants.ITEM_WAND) {
      this._renderWand(ctx, midx, midy, facedir, srcImage);
      return;
    }
    
    // Ditto fiddling on the violin.
    if (this.globals.g_active_item[0] === this.constants.ITEM_VIOLIN) {
      this._renderViolin(ctx, midx, midy, facedir, srcImage);
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
    
    // Cheesing is an overlay, but doesn't need to worry about other sprites.
    if (this.globals.g_cheesing[0]) {
      this._renderCheeseWhiz(ctx, midx, midy, srcImage);
    } else {
      this.cheesePhase = 0;
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
  
  _renderSpellRepudiation(ctx, midx, midy, srcImage) {
    const tilesize = this.constants.TILESIZE;
    if (this.globals.g_spell_repudiation[0] > 111) { // total run time in frames. should be 15 mod 16.
      this.globals.g_spell_repudiation[0] = 111;
    }
    this.globals.g_spell_repudiation[0]--;
    const frame = (this.globals.g_spell_repudiation[0] & 16) ? 1 : 0;
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize, srcImage, 0x20, 0);
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 7, srcImage, 0x2b + frame, 0);
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 12, srcImage, 0x1b + frame, 0);
  }
  
  _renderBroom(ctx, midx, midy, facedir, srcImage) {
    const tilesize = this.constants.TILESIZE;
    let xform = 0;
    if (this.globals.g_last_horz_dir[0] === this.constants.DIR_E) xform = this.constants.XFORM_XREV;
    const dy = (this.frameCount & 32) ? -1 : 0;
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 3 + dy, srcImage, 0x57, xform);
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 9 + dy, srcImage, 0x12, xform);
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 14 + dy, srcImage, 0x02, xform);
  }
  
  _renderWand(ctx, midx, midy, facedir, srcImage) {
    const tilesize = this.constants.TILESIZE;
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 3, srcImage, 0x09, 0);
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 12, srcImage, 0x00, 0);
    let armsTileId = 0x19, armsDx = 0, armsDy = -3;
    switch (this.globals.g_wand_dir[0]) {
      case this.constants.DIR_W: armsTileId = 0x29; armsDx = -3; armsDy = -3; break;
      case this.constants.DIR_E: armsTileId = 0x39; armsDx = 3; armsDy = -3; break;
      case this.constants.DIR_S: armsTileId = 0x49; armsDx = 0; armsDy = -2; break;
      case this.constants.DIR_N: armsTileId = 0x59; armsDx = 0; armsDy = -8; break;
    }
    this.renderBasics.tile(ctx, midx * tilesize + armsDx, midy * tilesize + armsDy, srcImage, armsTileId, 0);
  }
  
  _renderViolin(ctx, midx, midy, facedir, srcImage) {
    const tilesize = this.constants.TILESIZE;
    this.renderBasics.tile(ctx, midx * tilesize + 2, midy * tilesize - 3, srcImage, 0x28, 0);
    this.renderBasics.tile(ctx, midx * tilesize, midy * tilesize - 12, srcImage, 0x00, 0);
    if (this.globals.g_wand_dir[0]) { // "wand_dir" is also the violin
      const xrange = 5;
      const yrange = -2;
      const phaseFrames = 60;
      const phase = this.frameCount % phaseFrames;
      const displacement = ((phase >= phaseFrames >> 1) ? (phaseFrames - phase) : phase) / (phaseFrames >> 1);
      const dx = Math.round(displacement * xrange);
      const dy = Math.round(displacement * yrange);
      this.renderBasics.tile(ctx, midx * tilesize + dx, midy * tilesize - 4 + dy, srcImage, 0x48, 0);
    } else {
      this.renderBasics.tile(ctx, midx * tilesize + 2, midy * tilesize - 3, srcImage, 0x38, 0);
    }
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
  
  _renderCheeseWhiz(ctx, midx, midy, srcImage) {
    const period = 40;
    const phasef = this.cheesePhase / period;
    this.cheesePhase++;
    if (this.cheesePhase >= period) {
      this.cheesePhase = 0;
    }
    
    midy -= 0.5;
    this._renderCheeseSpot(ctx, midx, midy, srcImage, 1.5, -1.5, "#ff0", phasef);
    this._renderCheeseSpot(ctx, midx, midy, srcImage, 0, -2.0, "#f80", phasef);
    this._renderCheeseSpot(ctx, midx, midy, srcImage, -1.5, -1.5, "#ff0", phasef);
  }
  
  _renderCheeseSpot(ctx, midx, midy, srcImage, dx, dy, color, phase) {
    const dstx = (midx + dx * phase) * this.constants.TILESIZE;
    const dsty = (midy + dy * phase) * this.constants.TILESIZE;
    const r = this.constants.TILESIZE / 8;
    ctx.beginPath();
    ctx.ellipse(dstx, dsty, r, r, 0, 0, Math.PI * 2);
    ctx.fillStyle = color;
    ctx.globalAlpha = 1 - phase;
    ctx.fill();
    ctx.globalAlpha = 1;
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
  
  _animateShovel() {
    let xform = 0, dx = 0, dy = -3;
    switch (this.globals.g_facedir[0]) {
      case this.constants.DIR_N: dy = -10; break;
      case this.constants.DIR_S: break;
      case this.constants.DIR_W: dx = -2; xform = this.constants.XFORM_XREV; break;
      case this.constants.DIR_E: dx = 2; break;
    }
    if (this.carryActiveTime < 15) return [0x37, dx, dy, xform];
    return [0x47, dx, dy, xform];
  }
  
  /* Overlay: Rendered after all sprites.
   **********************************************************/
   
  renderOverlay(dst, ctx) {
    if (this.globals.g_itemv[this.globals.g_selected_item[0]]) {
      switch (this.globals.g_selected_item[0]) {
        case this.constants.ITEM_COMPASS: this._renderCompassOverlay(dst, ctx); break;
      }
    }
    if (this.globals.g_show_off_item_time[0]) {
      if (this.frameCount & 3) { // skip 1/4 frames
        const sprite = this.globals.getHeroSprite();
        if (!sprite) return;
        const srcImage = this.dataService.getImage(sprite.imageid);
        if (!srcImage) return;
        this.renderBasics.tile(ctx,
          sprite.x * this.constants.TILESIZE,
          sprite.y * this.constants.TILESIZE - this.constants.TILESIZE * 1.5,
          srcImage, 0xf0 + this.globals.g_show_off_item[0], 0
        );
      }
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
  
  /* Underlay: Rendered before all sprites.
   **********************************************************/
   
  renderUnderlay(dst, ctx) {
    if (this.globals.g_itemv[this.globals.g_selected_item[0]]) {
      switch (this.globals.g_selected_item[0]) {
        case this.constants.ITEM_SHOVEL: this._renderShovelUnderlay(dst, ctx); return;
      }
    }
    switch (this.globals.g_active_item[0]) {
      case this.constants.ITEM_BROOM: this._renderBroomUnderlay(dst, ctx); return;
    }
  }
  
  _renderShovelUnderlay(dst, ctx) {
    const [x, y] = this.globals.g_shovel;
    if ((x < 0) || (y < 0) || (x >= this.constants.COLC) || (y >= this.constants.ROWC)) return;
    const srcImage = this.dataService.getImage(2);
    if (!srcImage) return;
    this.renderBasics.tile(ctx, (x + 0.5) * this.constants.TILESIZE, (y + 0.5) * this.constants.TILESIZE, srcImage, (this.frameCount & 0x10) ? 0x17 : 0x27, 0);
  }
  
  _renderBroomUnderlay(dst, ctx) {
    if (this.frameCount & 1) return;
    const sprite = this.globals.getHeroSprite();
    if (!sprite) return;
    const srcImage = this.dataService.getImage(sprite.imageid);
    if (!srcImage) return;
    this.renderBasics.tile(ctx,
      (sprite.x) * this.constants.TILESIZE,
      (sprite.y + 0.3) * this.constants.TILESIZE,
      srcImage, (this.frameCount & 32) ? 0x67 : 0x77, 0
    );
  }
}

RenderHero.singleton = true;
