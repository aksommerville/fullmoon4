/* RenderSprites.js
 * Subservient to Renderer, renders all sprites.
 * The hero sprite has its own helper class. We own that relationship.
 */
 
import { RenderHero } from "./RenderHero.js";
import { RenderBasics } from "./RenderBasics.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";
 
export class RenderSprites {
  static getDependencies() {
    return [RenderHero, RenderBasics, Constants, Globals, DataService];
  }
  constructor(renderHero, renderBasics, constants, globals, dataService) {
    this.renderHero = renderHero;
    this.renderBasics = renderBasics;
    this.constants = constants;
    this.globals = globals;
    this.dataService = dataService;
    
    this.frameCount = 0;
  }
  
  render(dst, ctx) {
    this.frameCount++;
    let srcImage = null;
    let srcImageId = 0;
    let spritei = this.globals.g_spritec[0];
    if (spritei < 1) return;
    const tilesize = this.constants.TILESIZE;
    const halftile = tilesize >> 1;
    let spritepp = this.globals.g_spritev[0] >> 2;
    for (; spritei-->0; spritepp++) {
     
      const sprite = this.globals.getSpriteByAddress(this.globals.memU32[spritepp]);
      if (sprite.style === this.constants.SPRITE_STYLE_HIDDEN) continue;
        
      // Ensure image loaded. NB This is for all sprite render types, not just the generic.
      if (sprite.imageid !== srcImageId) {
        srcImage = this.dataService.getImage(sprite.imageid);
        srcImageId = sprite.imageid;
      }
      if (!srcImage) continue;
        
      switch (sprite.style) {

        case this.constants.SPRITE_STYLE_TILE: {
            this.renderBasics.tile(ctx, sprite.x * tilesize, sprite.y * tilesize, srcImage, sprite.tileid, sprite.xform);
          } break;

        case this.constants.SPRITE_STYLE_HERO: {
            this.renderHero.render(ctx, srcImage, sprite);
          } break;

        case this.constants.SPRITE_STYLE_FOURFRAME: {
            const frame = (this.frameCount >> 3) & 3;
            this.renderBasics.tile(ctx, sprite.x * tilesize, sprite.y * tilesize, srcImage, sprite.tileid + frame, sprite.xform);
          } break;
          
        case this.constants.SPRITE_STYLE_FIRENOZZLE: {
            this._renderFirenozzle(ctx, sprite, srcImage);
          } break;
      }
    }
  }
  
  _renderFirenozzle(ctx, sprite, srcImage) {
    const tilesize = this.constants.TILESIZE;
    switch (sprite.b1) {
      case 1: { // huff
          this.renderBasics.tile(ctx, sprite.x * tilesize, sprite.y * tilesize, srcImage, sprite.tileid + 1, sprite.xform);
        } break;
      case 2: { // puff
          let dstx = ~~(sprite.x * tilesize);
          let dsty = ~~(sprite.y * tilesize);
          let dx=0, dy=0;
          switch (sprite.xform) {
            case 0: dx = tilesize; break;
            case this.constants.XFORM_XREV: dx = -tilesize; break;
            case this.constants.XFORM_SWAP: dy = tilesize; break;
            case this.constants.XFORM_SWAP | this.constants.XFORM_XREV: dy = -tilesize; break;
          }
          this.renderBasics.tile(ctx, dstx, dsty, srcImage, sprite.tileid + 2, sprite.xform);
          const flameBase = sprite.tileid + ((this.frameCount & 8) ? 3 : 6);
          dstx += dx;
          dsty += dy;
          this.renderBasics.tile(ctx, dstx, dsty, srcImage, flameBase, sprite.xform);
          dstx += dx;
          dsty += dy;
          for (let i=sprite.b2; i-->2; dstx+=dx, dsty+=dy) {
            this.renderBasics.tile(ctx, dstx, dsty, srcImage, flameBase + 1, sprite.xform);
          }
          this.renderBasics.tile(ctx, dstx, dsty, srcImage, flameBase + 2, sprite.xform);
        } break;
      default: { // idle
          this.renderBasics.tile(ctx, sprite.x * tilesize, sprite.y * tilesize, srcImage, sprite.tileid, sprite.xform);
        } break;
    }
  }
}

RenderSprites.singleton = true;
