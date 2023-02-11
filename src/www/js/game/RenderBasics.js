/* RenderBasics.js
 * Shared by all the "Render" classes.
 */
 
import { Constants } from "./Constants.js";
 
export class RenderBasics {
  static getDependencies() {
    return [Constants];
  }
  constructor(constants) {
    this.constants = constants;
  }
  
  tile(ctx, midx, midy, srcImage, tileid, xform) {
    const tilesize = this.constants.TILESIZE;
    if (!xform) {
      ctx.drawImage(srcImage,
        (tileid & 0x0f) * tilesize, (tileid >> 4) * tilesize, tilesize, tilesize,
        ~~(midx - (tilesize >> 1)), ~~(midy - (tilesize >> 1)), tilesize, tilesize
      );
    } else {
      ctx.save();
      ctx.translate(~~midx, ~~midy);
      if (xform & this.constants.XFORM_SWAP) {
        ctx.transform(0, 1, 1, 0, 0, 0);
      }
      switch (xform & (this.constants.XFORM_XREV | this.constants.XFORM_YREV)) {
        case this.constants.XFORM_XREV: ctx.scale(-1, 1); break;
        case this.constants.XFORM_YREV: ctx.scale(1, -1); break;
        case this.constants.XFORM_XREV | this.constants.XFORM_YREV: ctx.scale(-1, -1); break;
      }
      ctx.drawImage(srcImage,
        (tileid & 0x0f) * tilesize, (tileid >> 4) * tilesize, tilesize, tilesize,
        -(tilesize >> 1), -(tilesize >> 1), tilesize, tilesize
      );
      ctx.restore();
    }
  }
}

RenderBasics.singleton = true;
