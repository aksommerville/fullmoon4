/* Renderer2d.js
 * Identical interface as RendererGl, owner can swap them.
 */
 
import * as FMN from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";
import { Dom } from "../util/Dom.js";

export class Renderer2d {
  static getDependencies() {
    return [Globals, DataService, Dom];
  }
  constructor(globals, dataService, dom) {
    this.globals = globals;
    this.dataService = dataService;
    this.dom = dom;
  
    this.canvas = null;
    this.ctx = null;
    this.fbw = 0;
    this.fbh = 0;
    this.pixfmt = 0;
    this.images = []; // [id]:{id,image,ctx}, sparse, image can be <img> or <canvas>, not that it matters
    this.dstimage = null; // null for main (ie this.canvas), or something from this.images
    this.recalScratch = null;
  }
  
  setRenderTarget(canvas) {
    if (this.canvas = canvas) {
      this.canvas.width = this.fbw;
      this.canvas.height = this.fbh;
      this.recalScratch = this.dom.createElement("CANVAS", { width: this.fbw, height: this.fbh });
    }
    this.dstimage = null;
    this.ctx = null;
    this.images[0] = {
      id: 0,
      image: this.canvas,
      ctx: null,
    };
  }
  
  begin() {
    if (!this.images[0]) return;
    if (!this.images[0].image) return;
    if (!this.images[0].ctx) {
      this.images[0].ctx = this.images[0].image.getContext("2d", { alpha: false });
      this.images[0].ctx.imageSmoothingEnabled = false;
    }
    this.ctx = this.images[0].ctx;
    this.dstimage = this.images[0].image;
  }
  
  commit() {
    this.ctx = null;
  }
  
  /* Public API, overhead.
   ********************************************************************/
  
  fmn_video_init(wmin, wmax, hmin, hmax, pixfmt) {
    const wpref = FMN.COLC * FMN.TILESIZE;
    const hpref = FMN.ROWC * FMN.TILESIZE;
    
    if ((wmin > wpref) || (wmax < wpref)) return -1;
    if ((hmin > hpref) || (hmax < hpref)) return -1;
    this.fbw = wpref;
    this.fbh = hpref;
    if (this.canvas) {
      this.canvas.width = this.fbw;
      this.canvas.height = this.fbh;
      this.recalScratch = this.dom.createElement("CANVAS", { width: this.fbw, height: this.fbh });
    }
    
    switch (pixfmt) {
      case FMN.VIDEO_PIXFMT_ANY:
      case FMN.VIDEO_PIXFMT_ANY_32:
      case FMN.VIDEO_PIXFMT_RGBA:
        break;
      default: return -1;
    }
    this.pixfmt = FMN.VIDEO_PIXFMT_RGBA;
    
    return 0;
  }

  fmn_video_get_framebuffer_size(wv, hv) {
    this.globals.memU16[wv >> 1] = this.fbw;
    this.globals.memU16[hv >> 1] = this.fbh;
  }

  fmn_video_get_image_size(wv, hv, imageid) {
    const image = this.getImage(imageid);
    if (image) {
      this.globals.memS16[wv >> 1] = image.naturalWidth || image.width;
      this.globals.memS16[hv >> 1] = image.naturalHeight || image.height;
    } else {
      this.globals.memS16[wv >> 1] = 0;
      this.globals.memS16[hv >> 1] = 0;
    }
  }

  fmn_video_init_image(imageid, w, h) {
    if (imageid < 1) return;
    this.images[imageid] = {
      id: imageid,
      image: this.dom.createElement("CANVAS", { width: w, height: h }),
      ctx: null,
    };
  }

  fmn_draw_set_output(imageid) {
    if (imageid < 0) return -1;
    if (!this.images[imageid]) {
      this.images[imageid] = {
        id: imageid,
        image: this.dom.createElement("CANVAS", { width: this.canvas.width, height: this.canvas.height }),
        ctx: null,
      };
    }
    const r = this.images[imageid];
    if (!r.ctx) {
      r.ctx = r.image.getContext("2d", { alpha: imageid !== 0 }); // Image zero is opaque. Others, we can't assume.
      r.ctx.imageSmoothingEnabled = false;
    }
    this.dstimage = r.image;
    this.ctx = r.ctx;
    return 0;
  }
  
  /* Public rendering API.
   ******************************************************************************/
   
  fmn_draw_clear() {
    if (!this.ctx) return;
    this.ctx.clearRect(0, 0, this.dstimage.width, this.dstimage.height);
  }

  fmn_draw_line(v, c) {
    if (!this.ctx) return;
    while (c-->0) {
      const ax = this.globals.memS16[v >> 1] + 0.5; v += 2;
      const ay = this.globals.memS16[v >> 1] + 0.5; v += 2;
      const bx = this.globals.memS16[v >> 1] + 0.5; v += 2;
      const by = this.globals.memS16[v >> 1] + 0.5; v += 2;
      const a = this.globals.memU8[v++];
      const b = this.globals.memU8[v++];
      const g = this.globals.memU8[v++];
      const r = this.globals.memU8[v++];
      this.ctx.beginPath();
      this.ctx.moveTo(ax, ay);
      this.ctx.lineTo(bx, by);
      this.ctx.strokeStyle = `rgb(${r} ${g} ${b} / ${a/255})`;
      this.ctx.stroke();
    }
  }

 fmn_draw_rect(v, c) {
   if (!this.ctx) return;
   while (c-->0) {
     const x = this.globals.memS16[v >> 1]; v += 2;
     const y = this.globals.memS16[v >> 1]; v += 2;
     const w = this.globals.memS16[v >> 1]; v += 2;
     const h = this.globals.memS16[v >> 1]; v += 2;
     const a = this.globals.memU8[v++];
     const b = this.globals.memU8[v++];
     const g = this.globals.memU8[v++];
     const r = this.globals.memU8[v++];
     this.ctx.fillStyle = `rgb(${r} ${g} ${b} / ${a/255})`;
     this.ctx.fillRect(x, y, w, h);
   }
 }

  fmn_draw_mintile(v, c, srcimageid) {
    if (!this.ctx) return;
    const srcimage = this.getImage(srcimageid);
    if (!srcimage) return;
    const w = srcimage.naturalWidth >> 4;
    const h = srcimage.naturalHeight >> 4;
    const halfw = w >> 1;
    const halfh = h >> 1;
    while (c-->0) {
      const x = this.globals.memS16[v >> 1]; v += 2;
      const y = this.globals.memS16[v >> 1]; v += 2;
      const tileid = this.globals.memU8[v++];
      const xform = this.globals.memU8[v++];
      const srcx = (tileid & 0x0f) * w;
      const srcy = (tileid >> 4) * h;
      this.renderDecal(x - halfw, y - halfh, srcimage, srcx, srcy, w, h, xform);
    }
  }

  fmn_draw_maxtile(v, c, srcimageid) {
    if (!this.ctx) return;
    const srcimage = this.getImage(srcimageid);
    if (!srcimage) return;
    const tilew = srcimage.naturalWidth >> 4;
    const tileh = srcimage.naturalHeight >> 4;
    while (c-->0) {
      const x = this.globals.memS16[v >> 1]; v += 2;
      const y = this.globals.memS16[v >> 1]; v += 2;
      const tileid = this.globals.memU8[v++];
      const rotate = this.globals.memU8[v++]; 
      const size = this.globals.memU8[v++];
      v += 9;/* maxtile is currently only used by the treasure-menu charms and the compass. They don't use xform, tint, or alpha.
      const xform = this.globals.memU8[v++];
      const tr = this.globals.memU8[v++];
      const tg = this.globals.memU8[v++];
      const tb = this.globals.memU8[v++];
      const ta = this.globals.memU8[v++];
      const pr = this.globals.memU8[v++];
      const pg = this.globals.memU8[v++];
      const pb = this.globals.memU8[v++];
      const a = this.globals.memU8[v++];
      /**/
      const srcx = (tileid & 0x0f) * tilew;
      const srcy = (tileid >> 4) * tileh;
      const halfsize = size >> 1;
      const rot = (rotate * Math.PI) / 128.0;
      const cost = Math.cos(rot);
      const sint = Math.sin(rot);
      const scale = size / tilew;
      
      this.ctx.save();
      this.ctx.setTransform(
        cost * scale, sint * scale,
        -sint * scale, cost * scale,
        x, y
      );
      this.ctx.drawImage(srcimage, srcx, srcy, tilew, tileh, -halfsize, -halfsize, size, size);
      this.ctx.restore();
    }
  }

  fmn_draw_decal(v, c, srcimageid) {
    if (!this.ctx) return;
    const srcimage = this.getImage(srcimageid);
    if (!srcimage) return;
    while (c-->0) {
      const dstx = this.globals.memS16[v >> 1]; v += 2;
      const dsty = this.globals.memS16[v >> 1]; v += 2;
      const dstw = this.globals.memS16[v >> 1]; v += 2;
      const dsth = this.globals.memS16[v >> 1]; v += 2;
      const srcx = this.globals.memS16[v >> 1]; v += 2;
      const srcy = this.globals.memS16[v >> 1]; v += 2;
      const srcw = this.globals.memS16[v >> 1]; v += 2;
      const srch = this.globals.memS16[v >> 1]; v += 2;
      this.renderDecalScale(dstx, dsty, dstw, dsth, srcimage, srcx, srcy, srcw, srch);
    }
  }
  
  fmn_draw_decal_swap(v, c, srcimageid) {
    if (!this.ctx) return;
    const srcimage = this.getImage(srcimageid);
    if (!srcimage) return;
    while (c-->0) {
      const dstx = this.globals.memS16[v >> 1]; v += 2;
      const dsty = this.globals.memS16[v >> 1]; v += 2;
      const dstw = this.globals.memS16[v >> 1]; v += 2;
      const dsth = this.globals.memS16[v >> 1]; v += 2;
      const srcx = this.globals.memS16[v >> 1]; v += 2;
      const srcy = this.globals.memS16[v >> 1]; v += 2;
      const srcw = this.globals.memS16[v >> 1]; v += 2;
      const srch = this.globals.memS16[v >> 1]; v += 2;
      //TODO decal_swap is not used. implementing as plain decal as a placeholder.
      // I think it wouldn't be a problem to add a "swap" argument here. But will need a test case.
      this.renderDecalScale(dstx, dsty, dstw, dsth, srcimage, srcx, srcy, srcw, srch);
    }
  }

  fmn_draw_recal(v, c, srcimageid) {
    if (!this.ctx) return;
    const srcimage = this.getImage(srcimageid);
    if (!srcimage) return;
    const recalCtx = this.recalScratch.getContext("2d");
    while (c-->0) {
      const dstx = this.globals.memS16[v >> 1]; v += 2;
      const dsty = this.globals.memS16[v >> 1]; v += 2;
      const dstw = this.globals.memS16[v >> 1]; v += 2;
      const dsth = this.globals.memS16[v >> 1]; v += 2;
      const srcx = this.globals.memS16[v >> 1]; v += 2;
      const srcy = this.globals.memS16[v >> 1]; v += 2;
      const srcw = this.globals.memS16[v >> 1]; v += 2;
      const srch = this.globals.memS16[v >> 1]; v += 2;
      const a = this.globals.memU8[v++];
      const b = this.globals.memU8[v++];
      const g = this.globals.memU8[v++];
      const r = this.globals.memU8[v++];
      recalCtx.globalCompositeOperation = "copy";
      recalCtx.fillStyle = `rgb(${r} ${g} ${b})`;
      recalCtx.fillRect(0, 0, this.recalScratch.width, this.recalScratch.height);
      recalCtx.globalCompositeOperation = "destination-in";
      //TODO Should transform like this.renderDecalScale, but I know we're not using that.
      recalCtx.drawImage(srcimage, srcx, srcy, srcw, srch, dstx, dsty, dstw, dsth);
      this.ctx.globalAlpha = a / 255.0;
      this.ctx.drawImage(this.recalScratch, dstx, dsty, dstw, dsth, dstx, dsty, dstw, dsth);
    }
    this.ctx.globalAlpha = 1;
  }
  
  fmn_draw_recal_swap(v, c, srcimageid) {
    if (!this.ctx) return;
    const srcimage = this.getImage(srcimageid);
    if (!srcimage) return;
    while (c-->0) {
      const dstx = this.globals.memS16[v >> 1]; v += 2;
      const dsty = this.globals.memS16[v >> 1]; v += 2;
      const dstw = this.globals.memS16[v >> 1]; v += 2;
      const dsth = this.globals.memS16[v >> 1]; v += 2;
      const srcx = this.globals.memS16[v >> 1]; v += 2;
      const srcy = this.globals.memS16[v >> 1]; v += 2;
      const srcw = this.globals.memS16[v >> 1]; v += 2;
      const srch = this.globals.memS16[v >> 1]; v += 2;
      const a = this.globals.memU8[v++];
      const b = this.globals.memU8[v++];
      const g = this.globals.memU8[v++];
      const r = this.globals.memU8[v++];
      //TODO recolor decal and swap axes. This hook is not currently used so no worries.
      this.renderDecalScale(dstx, dsty, dstw, dsth, srcimage, srcx, srcy, srcw, srch);
    }
  }
  
  /* Private primitive ops.
   ***********************************************************/
   
  getImage(id) {
    if (!id) return this.canvas;
    let r = this.images[id];
    if (r) return r.image;
    const image = this.dataService.getImage(id);
    this.images[id] = {
      id,
      image,
      ctx: null,
    };
    return image;
  }
   
  renderDecal(dstx, dsty, src, srcx, srcy, w, h, xform) {
    if (!xform) return this.ctx.drawImage(src, srcx, srcy, w, h, dstx, dsty, w, h);
    const halfw = w >> 1;
    const halfh = h >> 1;
    const dstmidx = dstx + halfw;
    const dstmidy = dsty + halfh;
    this.ctx.save();
    this.ctx.translate(dstmidx, dstmidy);
    switch (xform) {
      case FMN.XFORM_XREV: this.ctx.scale(-1, 1); break;
      case FMN.XFORM_YREV: this.ctx.scale(1, -1); break;
      case FMN.XFORM_XREV | FMN.XFORM_YREV: this.ctx.scale(-1, -1); break;
      case FMN.XFORM_SWAP: this.ctx.setTransform(0, 1, 1, 0, dstmidx, dstmidy); break;
      case FMN.XFORM_SWAP | FMN.XFORM_XREV: this.ctx.setTransform(0, -1, 1, 0, dstmidx, dstmidy); break;
      case FMN.XFORM_SWAP | FMN.XFORM_YREV: this.ctx.setTransform(0, 1, -1, 0, dstmidx, dstmidy); break;
      case FMN.XFORM_SWAP | FMN.XFORM_XREV | FMN.XFORM_YREV: this.ctx.setTransform(0, -1, -1, 0, dstmidx, dstmidy); break;
    }
    this.ctx.drawImage(src, srcx, srcy, w, h, -halfw, -halfh, w, h);
    this.ctx.restore();
  }
  
  renderDecalScale(dstx, dsty, dstw, dsth, src, srcx, srcy, srcw, srch) {
    // Fullmoon API allows either size negative to flip. CanvasRenderingContext2D only permits negative for src.
    // Correction: MDN says it permits negative for src, but I'm observing otherwise, in Chrome 112.
    // Well anyway, put all the negativity in (src), and then determine whether we need to transform.
    if (dstw < 0) {
      dstx += dstw;
      dstw = -dstw;
      srcx += srcw;
      srcw = -srcw;
    }
    if (dsth < 0) {
      dsty += dsth;
      dsth = -dsth;
      srcy += srch;
      srch = -srch;
    }
    if ((srcw < 0) || (srch < 0)) {
      this.ctx.save();
      const dstwhalf = dstw >> 1;
      const dsthhalf = dsth >> 1;
      const dstmidx = dstx + dstwhalf;
      const dstmidy = dsty + dsthhalf;
      this.ctx.translate(dstmidx, dstmidy);
      this.ctx.scale((srcw < 0) ? -1 : 1, (srch < 0) ? -1 : 1);
      if (srcw < 0) { srcx += srcw; srcw = -srcw; }
      if (srch < 0) { srcy += srch; srch = -srch; }
      this.ctx.drawImage(src, srcx, srcy, srcw, srch, -dstwhalf, -dsthhalf, dstw, dsth);
      this.ctx.restore();
    } else {
      this.ctx.drawImage(src, srcx, srcy, srcw, srch, dstx, dsty, dstw, dsth);
    }
  }
}

Renderer2d.singleton = true;

