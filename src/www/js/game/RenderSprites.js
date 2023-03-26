/* RenderSprites.js
 * Subservient to Renderer, renders all sprites.
 * The hero sprite has its own helper class. We own that relationship.
 */
 
import { RenderHero } from "./RenderHero.js";
import { RenderWerewolf } from "./RenderWerewolf.js";
import { RenderBasics } from "./RenderBasics.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";
 
export class RenderSprites {
  static getDependencies() {
    return [RenderHero, RenderBasics, Constants, Globals, DataService, RenderWerewolf];
  }
  constructor(renderHero, renderBasics, constants, globals, dataService, renderWerewolf) {
    this.renderHero = renderHero;
    this.renderBasics = renderBasics;
    this.constants = constants;
    this.globals = globals;
    this.dataService = dataService;
    this.renderWerewolf = renderWerewolf;
    
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
        case this.constants.SPRITE_STYLE_TWOFRAME: {
            const frame = (this.frameCount >> 3) & 1;
            this.renderBasics.tile(ctx, sprite.x * tilesize, sprite.y * tilesize, srcImage, sprite.tileid + frame, sprite.xform);
          } break;
        case this.constants.SPRITE_STYLE_EIGHTFRAME: {
            const frame = (this.frameCount >> 1) & 7;
            this.renderBasics.tile(ctx, sprite.x * tilesize, sprite.y * tilesize, srcImage, sprite.tileid + frame, sprite.xform);
          } break;

        case this.constants.SPRITE_STYLE_DOUBLEWIDE: if (sprite.xform & this.constants.XFORM_XREV) {
            this.renderBasics.tile(ctx, sprite.x * tilesize - (tilesize >> 1), sprite.y * tilesize, srcImage, sprite.tileid + 1, sprite.xform);
            this.renderBasics.tile(ctx, (sprite.x + 1) * tilesize - (tilesize >> 1), sprite.y * tilesize, srcImage, sprite.tileid, sprite.xform);
          } else {
            this.renderBasics.tile(ctx, sprite.x * tilesize - (tilesize >> 1), sprite.y * tilesize, srcImage, sprite.tileid, sprite.xform);
            this.renderBasics.tile(ctx, (sprite.x + 1) * tilesize - (tilesize >> 1), sprite.y * tilesize, srcImage, sprite.tileid + 1, sprite.xform);
          } break;
          
        case this.constants.SPRITE_STYLE_FIRENOZZLE: this._renderFirenozzle(ctx, sprite, srcImage); break;
        case this.constants.SPRITE_STYLE_FIREWALL: this._renderFirewall(ctx, sprite, srcImage); break;
        case this.constants.SPRITE_STYLE_PITCHFORK: this._renderPitchfork(ctx, sprite, srcImage); break;
        case this.constants.SPRITE_STYLE_SCARYDOOR: this._renderScarydoor(ctx, sprite, srcImage); break;
        case this.constants.SPRITE_STYLE_WEREWOLF: this.renderWerewolf.render(ctx, sprite, srcImage); break;
        case this.constants.SPRITE_STYLE_FLOORFIRE: this._renderFloorfire(ctx, sprite, srcImage); break;
        case this.constants.SPRITE_STYLE_DEADWITCH: this._renderDeadwitch(ctx, sprite, srcImage); break;
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
  
  _renderFirewall(ctx, sprite, srcImage) {
    const tilesize = this.constants.TILESIZE;
    const fbw = this.constants.COLC * tilesize;
    const fbh = this.constants.ROWC * tilesize;
    const bv = this.globals.getSpriteBv(sprite.address);
    const fv = this.globals.getSpriteFv(sprite.address);
    const extent = ~~(fv[0] * tilesize);
    const sp = bv[1] * tilesize;
    const sc = bv[2] * tilesize;
    switch (bv[0]) { // dir
      case this.constants.DIR_W: this._renderFirewall1(ctx, srcImage, sprite.tileid, tilesize, 0, sp, extent, sc, bv[0]); break;
      case this.constants.DIR_N: this._renderFirewall1(ctx, srcImage, sprite.tileid, tilesize, sp, 0, sc, extent, bv[0]); break;
      case this.constants.DIR_E: this._renderFirewall1(ctx, srcImage, sprite.tileid, tilesize, fbw - extent, sp, extent, sc, bv[0]); break;
      case this.constants.DIR_S: this._renderFirewall1(ctx, srcImage, sprite.tileid, tilesize, sp, fbh - extent, sc, extent, bv[0]); break;
    }
  }
  
  _renderFirewall1(ctx, srcImage, tileId, tilesize, x, y, w, h, dir) {
    if (this.frameCount & 16) tileId += 0x20;
    let dxminor=0, dyminor=0, dxmajor=0, dymajor=0, xform=0;
    switch (dir) {
      case this.constants.DIR_N: y+=h-tilesize; dxminor=1; dymajor=-1; break;
      case this.constants.DIR_S: dxminor=1; dymajor=1; xform=this.constants.XFORM_YREV; break;
      case this.constants.DIR_W: x+=w-tilesize; dyminor=1; dxmajor=-1; xform=this.constants.XFORM_SWAP; break;
      case this.constants.DIR_E: dyminor=1; dxmajor=1; xform=this.constants.XFORM_SWAP|this.constants.XFORM_YREV; break;
      default: return;
    }
    const minorc = Math.ceil(Math.abs(w * dxminor + h * dyminor) / tilesize);
    const majorc = Math.ceil(Math.abs(w * dxmajor + h * dymajor) / tilesize);
    dxmajor *= tilesize;
    dymajor *= tilesize;
    dxminor *= tilesize;
    dyminor *= tilesize;
    x += tilesize >> 1;
    y += tilesize >> 1;
    let tileRow = 0x10;
    for (let majori=majorc; majori-->0; x+=dxmajor, y+=dymajor) {
      let x1=x, y1=y;
      for (let minori=minorc; minori-->0; x1+=dxminor, y1+=dyminor) {
        const subTileId = tileId + tileRow + ((minori === minorc-1) ? 0 : minori ? 1 : 2);
        this.renderBasics.tile(ctx, x1, y1, srcImage, subTileId, xform);
      }
      tileRow = 0;
    }
  }
  
  _renderPitchfork(ctx, sprite, srcImage) {
    const tilesize = this.constants.TILESIZE;
    const fv = this.globals.getSpriteFv(sprite.address);
    const dsty = sprite.y * tilesize;
    const bodydstx = sprite.x * tilesize;
    const dx = tilesize * ((sprite.xform & this.constants.XFORM_XREV) ? 1 : -1);
    let dstx = ~~(bodydstx + dx * (1 + fv[0]));
    this.renderBasics.tile(ctx, dstx, dsty, srcImage, sprite.tileid - 4, sprite.xform); // head
    for (;;) {
      dstx -= dx;
      if ((dx > 0) && (dstx <= bodydstx)) break;
      if ((dx < 0) && (dstx >= bodydstx)) break;
      this.renderBasics.tile(ctx, dstx, dsty, srcImage, sprite.tileid - 2, sprite.xform);
    }
    this.renderBasics.tile(ctx, bodydstx, dsty, srcImage, sprite.tileid - (fv[0] ? 0 : 1), sprite.xform); // body
  }
  
  _renderScarydoor(ctx, sprite, srcImage) {
    const tilesize = this.constants.TILESIZE;
    const tilesize2 = tilesize << 1;
    const fv = this.globals.getSpriteFv(sprite.address); // [0]:closedness
    const halfh = Math.floor(fv[0] * tilesize2);
    if (halfh <= 0) return;
    const dstx = Math.floor(sprite.x) * tilesize;
    const dsty = Math.floor(sprite.y) * tilesize;
    const srcx = (sprite.tileid & 15) * tilesize;
    const srcy = (sprite.tileid >> 4) * tilesize;
    ctx.drawImage(srcImage, srcx, srcy + tilesize2 - halfh, tilesize2, halfh, dstx, dsty, tilesize2, halfh);
    ctx.drawImage(srcImage, srcx, srcy + tilesize2, tilesize2, halfh, dstx, dsty + tilesize + tilesize2 - halfh, tilesize2, halfh);
  }
  
  _renderFloorfire(ctx, sprite, srcImage) {
    const tilesize = this.constants.TILESIZE;
    const fv = this.globals.getSpriteFv(sprite.address);
    const bv = this.globals.getSpriteBv(sprite.address);
    const RING_SPACING = 0.750; // should match fmn_sprite_floorfire.c:FLOORFIRE_RING_SPACING
    const RADIAL_SPACING = 0.500;
    const xlimit = (this.constants.COLC + 1) * tilesize;
    const ylimit = (this.constants.ROWC + 1) * tilesize;
    for (let i=bv[0], radius=fv[2], tileid=0xb0; (i-->0) && (tileid<0xbd); radius-=RING_SPACING, tileid++) {
      const firec = Math.floor((radius * 2 * Math.PI) / RADIAL_SPACING);
      if (firec < 1) break;
      const dt = (Math.PI * 2) / firec;
      for (let t=0, firei=firec; firei-->0; t+=dt) {
        const x = ~~((sprite.x + Math.cos(t) * radius) * tilesize);
        if ((x < -tilesize) || (x > xlimit)) continue;
        const y = ~~((sprite.y + Math.sin(t) * radius) * tilesize);
        if ((y < -tilesize) || (y > ylimit)) continue;
        this.renderBasics.tile(ctx, x, y, srcImage, tileid, 0);
      }
    }
    if (false) {
      ctx.beginPath();
      ctx.arc(sprite.x * tilesize, sprite.y * tilesize, fv[1] * tilesize, 0, Math.PI * 2); // leading edge
      if (fv[1] >= 2) {
        ctx.arc(sprite.x * tilesize, sprite.y * tilesize, (fv[1] - 2) * tilesize, 0, Math.PI * 2); // outer danger limit
        if (fv[1] >= 6) {
          ctx.arc(sprite.x * tilesize, sprite.y * tilesize, (fv[1] - 6) * tilesize, 0, Math.PI * 2); // inner danger limit
        }
      }
      ctx.strokeStyle = "#fff";
      ctx.stroke();
    }
  }
  
  _renderDeadwitch(ctx, sprite, srcImage) {
    const tilesize = this.constants.TILESIZE;
    const pxx = (sprite.x * tilesize);
    const pxy = (sprite.y * tilesize) + 2;
    const clock = this.globals.getSpriteFv(sprite.address)[0];
    let frame = ~~(clock * 7);
    if (frame >= 7) frame = 6;
    const puddles = [ // [x,y,tileid]
      [pxx - 8, pxy + 1, sprite.tileid - 0x0f + frame],
      [pxx + 8, pxy + 2, sprite.tileid + 0x01 + frame],
      [pxx - 2, pxy + 4, sprite.tileid + 0x01 + frame],
    ];
    for (const [x, y, tileid] of puddles) {
      this.renderBasics.tile(ctx, x, y, srcImage, tileid, 0);
    }
    this.renderBasics.tile(ctx, pxx, pxy, srcImage, sprite.tileid, sprite.xform);
  }
}

RenderSprites.singleton = true;
