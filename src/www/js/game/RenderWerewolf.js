import { RenderBasics } from "./RenderBasics.js";
import { Globals } from "./Globals.js";
import { Constants } from "./Constants.js";

// Authority: src/app/sprite/types/fmn_sprite_werewolf.c.
// sprite->bv[0]
const STAGE_IDLE = 0;
const STAGE_SLEEP = 1;
const STAGE_ERECT = 2;
const STAGE_WALK = 3;
const STAGE_GROWL = 4;
const STAGE_POUNCE = 5;
const STAGE_HADOUKEN_CHARGE = 6;
const STAGE_HADOUKEN_FOLLOWTHRU = 7;
const STAGE_FLOORFIRE_CHARGE = 8;
const STAGE_FLOORFIRE_FOLLOWTHRU = 9;
const STAGE_EAT = 10;
const STAGE_SHOCK = 11;
const STAGE_DEAD = 12;

export class RenderWerewolf {
  static getDependencies() {
    return [RenderBasics, Globals, Constants];
  }
  constructor(renderBasics, globals, constants) {
    this.renderBasics = renderBasics;
    this.globals = globals;
    this.constants = constants;
    
    this.frameCount = 0;
    this.tilesize = this.constants.TILESIZE;
    this.floorfireChargeFrame = 0;
  }
  
  render(ctx, sprite, srcImage) {
    this.frameCount++;
    
    // Record a few transient things on this, so we don't have to pass them around.
    this.ctx = ctx;
    this.sprite = sprite;
    this.srcImage = srcImage;
    this.bv = this.globals.getSpriteBv(sprite.address);
    this.fv = this.globals.getSpriteFv(sprite.address);
    this.pxx = ~~(sprite.x * this.tilesize); // near horizontal center, about the shoulders
    this.pxy = ~~(sprite.y * this.tilesize); // near feet
    
    // It's helpful if (pxx) is the axis for horizontal reflection.
    // Cheat a little here and make that so.
    // It stays put on all fours, and moves toward the hind legs when erect.
    const originalPxx = this.pxx;
    switch (this.bv[0]) {
      case STAGE_ERECT: 
      case STAGE_HADOUKEN_CHARGE:
      case STAGE_HADOUKEN_FOLLOWTHRU:
      case STAGE_FLOORFIRE_CHARGE:
      case STAGE_FLOORFIRE_FOLLOWTHRU:
        if (this.sprite.xform & this.constants.XFORM_XREV) {
          this.pxx -= this.tilesize >> 1;
        } else {
          this.pxx += this.tilesize >> 1;
        } break;
    }
    
    switch (this.bv[0]) {
      case STAGE_IDLE: this._renderAllFours(false); break;
      case STAGE_SLEEP: this._renderSleep(); break;
      case STAGE_ERECT: this._renderErect(false, 0, 0); break;
      case STAGE_WALK: this._renderAllFours(true); break;
      case STAGE_GROWL: this._renderGrowl(); break;
      case STAGE_POUNCE: this._renderPounce(); break;
      case STAGE_HADOUKEN_CHARGE: this._renderErect(false, 0, 0); this._renderHadouken(); break;
      case STAGE_HADOUKEN_FOLLOWTHRU: this._renderErect(false, 2, 0); break;
      case STAGE_FLOORFIRE_CHARGE: this._renderFloorfireCharge(); this._renderErect(false, 1, 4); break;
      case STAGE_FLOORFIRE_FOLLOWTHRU: this._renderErect(false, 3, 0); this.floorfireChargeFrame = 0; break;
      case STAGE_EAT: this._renderEat(); break;
      case STAGE_SHOCK: this._renderShock(); break;
      case STAGE_DEAD: this._renderDead(); break;
    }
    
    // Highlight sprite's center.
    if (false) {
      ctx.globalAlpha = 0.5;
      ctx.fillStyle = "#f00";
      ctx.fillRect(this.pxx - 1, this.pxy - 1, 2, 2);
      if (this.pxx !== this.originalPxx) {
        ctx.fillStyle = "#f80";
        ctx.fillRect(originalPxx - 1, this.pxy - 1, 2, 2);
      }
      ctx.globalAlpha = 1;
    }
  }
  
  /* Private: Gross poses.
   ****************************************************************************/
   
  /* IDLE and WALK take the "AllFours" position: Bodies from row 1, with the head high.
   */
  _renderAllFours(animateLegs) {
    let bodyCol = 0;
    if (animateLegs) switch ((this.frameCount >> 3) & 3) {
      case 1: bodyCol = 1; break;
      case 3: bodyCol = 2; break;
    }
    const blink = (this.frameCount % 80 < 15);
    this._renderBody(0, 0, 1, bodyCol);
    this._renderHead(-(this.tilesize << 1) + 3, 0, blink ? 1 : 0);
  }
  
  /* "Erect" for all two-legged positions.
   * I think this is only Hadouken and Floorfire, but could surely imagine others.
   * I've drawn walking poses but not using yet. Row 2.
   * (armFrame) is 0..3, and we position the arms accordingly.
   */
  _renderErect(animateLegs, armFrame, headFrame) {
    let bodyCol = 0;
    if (animateLegs) switch ((this.frameCount >> 3) & 3) {
      case 1: bodyCol = 1; break;
      case 3: bodyCol = 2; break;
    }
    this._renderBackArm(armFrame);
    this._renderBody(0, 0, 2, bodyCol);
    this._renderHead(-this.tilesize, -7, headFrame);
    this._renderFrontArm(armFrame);
  }
  
  /* "Sleep" is basically "AllFours" with no animation, and modified head and body positions.
   */
  _renderSleep() {
    this._renderBody(0, 8, 1, 3);
    this._renderHead(-(this.tilesize << 1) + 3, this.tilesize, 3);
  }
  
  /* "Growl" is similar to "Sleep", a special variation of "AllFours".
   * Might want to animate this.
   */
  _renderGrowl() {
    this._renderBody(0, 0, 1, 4);
    this._renderHead(-(this.tilesize << 1) + 3, 5, 2);
  }
  
  /* "Pounce" kind of like "AllFours" but just one frame and the body is 3x2.
   */
  _renderPounce() {
    this._renderBody3x2(0, 0);
    this._renderHead(-(this.tilesize << 1) - 4, 0,5);
  }
  
  _renderEat() {
    this._renderBody(0, 0, 1, 5);
    let frame;
    switch (Math.floor(this.frameCount / 10) % 3) {
      case 0: frame = 3; break;
      case 1: frame = 4; break;
      case 2: frame = 5; break;
    }
    this._renderHead1x2(-this.tilesize - 6, 6, frame);
  }
  
  /* "Shock" the images are upside down, and the body twitches.
   */
  _renderShock() {
    let bodyFrame = 6;
    if (this.frameCount % 50 >= 42) bodyFrame = 7;
    this._renderBody(0, 0, 1, bodyFrame);
    this._renderHead(-(this.tilesize << 1) + 1, this.tilesize + 2, 6);
  }
  
  /* "Dead" almost exactly like "Sleep".
   */
  _renderDead() {
    this._renderBody(0, 8, 1, 3);
    this._renderHead(-(this.tilesize << 1) + 3, this.tilesize, 7);
  }
  
  /* Private: Bits n pieces.
   *******************************************************************************/
   
  // Single tile from row 6.
  _renderBackArm(column) {
    let x=0, y=0;
    switch (column) {
      case 0: x = 0; y = -this.tilesize - 1; break;//ok
      case 1: x = 1; y = -((this.tilesize * 3) >> 1); break; //TODO
      case 2: x = -(this.tilesize >> 1) + 2; y = -this.tilesize; break;//ok
      case 3: x = 1; y = -this.tilesize + 4; break; //TODO
    }
    if (this.sprite.xform & this.constants.XFORM_XREV) x = -x;
    x += this.pxx;
    y += this.pxy;
    this.renderBasics.tile(
      this.ctx, x, y,
      this.srcImage,
      0x60 + column,
      this.sprite.xform
    );
  }
   
  // Single tile from row 5.
  _renderFrontArm(column) {
    let x=0, y=0;
    switch (column) {
      case 0: x = (this.tilesize >> 1) + 2; y = -this.tilesize; break;//ok
      case 1: x = this.tilesize - 4; y = -((this.tilesize * 3) >> 1); break; //ok
      case 2: x = (this.tilesize >> 1) - 3; y = -this.tilesize; break;//ok
      case 3: x = this.tilesize - 4; y = -(this.tilesize >> 1) - 1; break; //ok
    }
    if (this.sprite.xform & this.constants.XFORM_XREV) x = -x;
    x += this.pxx;
    y += this.pxy;
    this.renderBasics.tile(
      this.ctx, x, y,
      this.srcImage, 
      0x50 + column,
      this.sprite.xform
    );
  }
  
  // 2x2 tile. Row 1 all fours, row 2 erect. (column) 0..7.
  // (dx,dy) zero for the natural all-fours position.
  _renderBody(dx, dy, row, column) {
    if (this.sprite.xform & this.constants.XFORM_XREV) dx -= (this.tilesize * 3) >> 1;
    else dx -= this.tilesize >> 1;
    dy -= (this.tilesize * 3) >> 1;
    const doublesize = this.tilesize << 1;
    const srcx = column * doublesize;
    const srcy = this.tilesize + (row - 1) * doublesize;
    this.renderBasics.decal(
      this.ctx, this.pxx + dx, this.pxy + dy,
      this.srcImage, srcx, srcy,
      doublesize, doublesize,
      this.sprite.xform
    );
  }
  
  // 3x2 tile. Right now there's just one pose, so you don't specify.
  // (dx,dy) zero for the natural pounce position.
  _renderBody3x2(dx, dy) {
    if (this.sprite.xform & this.constants.XFORM_XREV) dx -= (this.tilesize * 3) >> 1;
    else dx -= (this.tilesize * 3) >> 1;
    dy -= this.tilesize << 1;
    this.renderBasics.decal(
      this.ctx, this.pxx + dx, this.pxy + dy,
      this.srcImage, 0, this.tilesize * 8,
      this.tilesize * 3, this.tilesize * 2,
      this.sprite.xform
    );
  }
  
  // 2x1 tile. (column) 0..7.
  // (dx,dy) will generally be nonzero, they are straight against (pxx,pxy) facing left (we take care of transform).
  // (dx) is the nose position relative to reference point, when facing left.
  _renderHead(dx, dy, column) {
    let dstx;
    if (this.sprite.xform & this.constants.XFORM_XREV) {
      dstx = this.pxx - dx - (this.tilesize << 1);
    } else {
      dstx = this.pxx + dx;
    }
    dy += -(this.tilesize << 1) + 3;
    this.renderBasics.decal(
      this.ctx, dstx, this.pxy + dy,
      this.srcImage, column * this.tilesize * 2, 0,
      this.tilesize << 1, this.tilesize,
      this.sprite.xform
    );
  }
  
  _renderHead1x2(dx, dy, column) {
    let dstx;
    if (this.sprite.xform & this.constants.XFORM_XREV) {
      dstx = this.pxx - dx - this.tilesize;
    } else {
      dstx = this.pxx + dx;
    }
    dy += -(this.tilesize << 1) + 3;
    this.renderBasics.decal(
      this.ctx, dstx, this.pxy + dy,
      this.srcImage, column * this.tilesize, 8 * this.tilesize,
      this.tilesize, this.tilesize << 1,
      this.sprite.xform
    );
  }
  
  // Just the decoration during HADOUKEN_CHARGE stage. The flying one is its own sprite.
  _renderHadouken() {
    const CHARGE_TIME = 0.5; // should match fmn_sprite_werewolf.c:WEREWOLF_HADOUKEN_CHARGE_TIME
    let tileId = 0x70 + (this.fv[1] * 4) / CHARGE_TIME;
    if (tileId > 0x73) tileId = 0x73;
    this.renderBasics.tile(this.ctx,
      ~~(this.sprite.x * this.tilesize),
      ~~((this.sprite.y - 1.0) * this.tilesize),
      this.srcImage, tileId, 0
    );
  }
  
  // A column of tiles extending to the ceiling, heavily animated.
  _renderFloorfireCharge() {
    const x = this.pxx + ((this.sprite.xform & this.constants.XFORM_XREV) ? -(this.tilesize >> 1) : (this.tilesize >> 1));
    let y = this.pxy - this.tilesize;
    const stop = -this.tilesize;
    for (; y>stop; y-=this.tilesize) {
      this.renderBasics.tile(this.ctx, x, y, this.srcImage, 0xa0 + this.floorfireChargeFrame, 0);
    }
    this.floorfireChargeFrame++;
    if (this.floorfireChargeFrame >= 16) this.floorfireChargeFrame -= 4;
  }
  
}
