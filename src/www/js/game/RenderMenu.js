/* RenderMenu.js
 * Subservient to Renderer, manages the menus that overlay scene and transition.
 */
 
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { DataService } from "./DataService.js";
import { Menu, PauseMenu, ChalkMenu } from "./Menu.js";
import { RenderBasics } from "./RenderBasics.js";
 
export class RenderMenu {
  static getDependencies() {
    return [Constants, Globals, DataService, RenderBasics];
  }
  constructor(constants, globals, dataService, renderBasics) {
    this.constants = constants;
    this.globals = globals;
    this.dataService = dataService;
    this.renderBasics = renderBasics;
    
    this.itemsWithQuantity = [
      this.constants.ITEM_CORN,
      this.constants.ITEM_SEED,
      this.constants.ITEM_COIN,
      this.constants.ITEM_MATCH,
      this.constants.ITEM_CHEESE,
    ];
  }
  
  /* Caller should render the scene and transition first.
   * We render menus and blotter on top of that, according to (menus).
   */
  render(dst, menus) {
    if (menus.length < 1) return;
    const ctx = dst.getContext("2d");
    let blotterDelay = menus.length;
    for (const menu of menus) {
      if (!--blotterDelay) this._renderBlotter(dst, ctx);
      this._renderMenu(dst, ctx, menu);
    }
  }
  
  /* Private.
   **************************************************************************/
   
  _renderBlotter(dst, ctx) {
    ctx.globalAlpha = 0.6;
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, dst.width, dst.height);
    ctx.globalAlpha = 1;
  }
  
  _renderMenu(dst, ctx, menu) {
    if (menu instanceof PauseMenu) this._renderPauseMenu(dst, ctx, menu);
    else if (menu instanceof Menu) this._renderOptionsMenu(dst, ctx, menu);
    else if (menu instanceof ChalkMenu) this._renderChalkMenu(dst, ctx, menu);
    else throw new Error(`Unexpected object provided to RenderMenu._renderMenu`);
  }
  
  /* General prompt-and-options menu.
   ***************************************************************/
  
  _renderOptionsMenu(dst, ctx, menu) {
    //TODO
  }
  
  /* The pause menu, where you select an item.
   ***********************************************************/
  
  _renderPauseMenu(dst, ctx, menu) {
    const cellPaddingX = 4;
    const cellPaddingY = 4;
    const colw = this.constants.TILESIZE + (cellPaddingX << 1);
    const rowh = this.constants.TILESIZE + (cellPaddingY << 1);
    const gridx = (dst.width >> 1) - (colw << 1);
    const gridy = (dst.height >> 1) - (rowh << 1);
    const srcImage = this.dataService.getImage(2);
    
    ctx.fillStyle = "#000";
    ctx.globalAlpha = 1;
    ctx.fillRect(gridx, gridy, colw << 2, rowh << 2);
    ctx.globalAlpha = 1;
    
    if (srcImage) {
      // Base image for available items.
      for (let row=0, dsty=gridy+cellPaddingX, itemId=0; row<4; row++, dsty+=rowh) {
        for (let col=0, dstx=gridx+cellPaddingY; col<4; col++, dstx+=colw, itemId++) {
          if (!this.globals.g_itemv[itemId]) continue;
          ctx.drawImage(srcImage,
            itemId * this.constants.TILESIZE, 15 * this.constants.TILESIZE, this.constants.TILESIZE, this.constants.TILESIZE,
            dstx, dsty, this.constants.TILESIZE, this.constants.TILESIZE
          );
        }
      }
      
      // Adjustable content (other than quantities).
      if (this.globals.g_itemv[this.constants.ITEM_PITCHER]) {
        const contentId = this.globals.g_itemqv[this.constants.ITEM_PITCHER];
        if (contentId) { // zero is "empty", and that's the default image too
          ctx.drawImage(srcImage,
            (contentId + 1) * this.constants.TILESIZE, 208, this.constants.TILESIZE, this.constants.TILESIZE,
            gridx + cellPaddingX + (this.constants.ITEM_PITCHER & 3) * colw,
            gridy + cellPaddingY + (this.constants.ITEM_PITCHER >> 2) * rowh,
            this.constants.TILESIZE, this.constants.TILESIZE
          );
        }
      }
      
      // Focus indicator.
      const srcx = 0, srcy = this.constants.TILESIZE * 12;
      const dstx = gridx + colw * menu.selx + (colw >> 1) - this.constants.TILESIZE;
      const dsty = gridy + rowh * menu.sely + (rowh >> 1) - this.constants.TILESIZE;
      const w = this.constants.TILESIZE << 1;
      const h = this.constants.TILESIZE << 1;
      ctx.drawImage(srcImage, srcx, srcy, w, h, dstx, dsty, w, h);
      
      // Quantity labels for selected items.
      for (const itemId of this.itemsWithQuantity) {
        if (!this.globals.g_itemv[itemId]) continue;
        this._renderQuantity(
          ctx,
          gridx + ((itemId & 3) + 1) * colw - 3,
          gridy + ((itemId >> 2) + 1) * rowh - 1,
          this.globals.g_itemqv[itemId],
          srcImage
        );
      }
    }
  }
  
  // (dstx,dsty) is the lower-right corner of the focussed cell.
  _renderQuantity(ctx, dstx, dsty, q, srcImage) {
    dsty -= 7;
    let digitc;
    if (q >= 100) digitc = 3;
    else if (q >= 10) digitc = 2;
    else digitc = 1;
    dstx -= 3 + digitc * 3;
    const bgbase = q ? 30 : 37;
    ctx.drawImage(srcImage, bgbase, 224, 2, 7, dstx, dsty, 2, 7);
    ctx.drawImage(srcImage, bgbase + 6, 224, 1, 7, dstx + 2 + digitc * 4, dsty, 1, 7);
    for (let x=dstx+2+(digitc-1)*4, i=digitc; i-->0; x-=4, q=Math.floor(q/10)) {
      const digit = q % 10;
      ctx.drawImage(srcImage, bgbase + 2, 224, 4, 7, x, dsty, 4, 7);
      ctx.drawImage(srcImage, digit * 3, 224, 3, 5, x, dsty + 1, 3, 5);
    }
  }
  
  /* Chalk menus, for drawing with chalk.
   ****************************************************/
   
  _renderChalkMenu(dst, ctx, menu) {
    const dotSpacing = ~~(Math.min(dst.width, dst.height) / 3);
    const fieldw = dotSpacing * 2;
    const fieldx = (dst.width >> 1) - (fieldw >> 1);
    const fieldy = (dst.height >> 1) - (fieldw >> 1);
    const tilesize = this.constants.TILESIZE;
    const srcImage = this.dataService.getImage(2);
    if (!srcImage) return;
    
    for (let col=0; col<3; col++) {
      for (let row=0; row<3; row++) {
        this.renderBasics.tile(ctx, fieldx + col * dotSpacing, fieldy + row * dotSpacing, srcImage, 0xc2, 0);
      }
    }
    
    const connect = (ax, ay, bx, by) => {
      ctx.moveTo(fieldx + ax * dotSpacing, fieldy + ay * dotSpacing);
      ctx.lineTo(fieldx + bx * dotSpacing, fieldy + by * dotSpacing);
    };
    ctx.lineWidth = 2;
    if (menu.sketch) {
      ctx.beginPath();
      for (let mask=0x80000; mask; mask>>=1) {
        if (!(menu.sketch & mask)) continue;
        const [ax, ay, bx, by] = ChalkMenu.pointsFromBit(mask);
        connect(ax, ay, bx, by);
      }
      ctx.strokeStyle = "#fff";
      ctx.stroke();
    }
    if (menu.pendingLine) {
      ctx.beginPath();
      const [ax, ay, bx, by] = ChalkMenu.pointsFromBit(menu.pendingLine);
      connect(ax, ay, bx, by);
      ctx.strokeStyle = "#ff0";
      ctx.stroke();
    }
    ctx.lineWidth = 1;
    
    this.renderBasics.tile(ctx, fieldx + menu.selx * dotSpacing + (tilesize >> 1), fieldy + menu.sely * dotSpacing + (tilesize >> 1), srcImage, 0xc3, 0);
  }
}

RenderMenu.singleton = true;
