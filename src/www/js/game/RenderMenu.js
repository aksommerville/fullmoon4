/* RenderMenu.js
 * Subservient to Renderer, manages the menus that overlay scene and transition.
 */
 
import { Constants } from "./Constants.js";
import { DataService } from "./DataService.js";
import { Menu, PauseMenu } from "./Menu.js";
 
export class RenderMenu {
  static getDependencies() {
    return [Constants, DataService];
  }
  constructor(constants, dataService) {
    this.constants = constants;
    this.dataService = dataService;
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
    else throw new Error(`Unexpected object provided to RenderMenu._renderMenu`);
  }
  
  _renderOptionsMenu(dst, ctx, menu) {
    //TODO
  }
  
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
      for (let row=0, dsty=gridy+cellPaddingX, itemId=0; row<4; row++, dsty+=rowh) {
        for (let col=0, dstx=gridx+cellPaddingY; col<4; col++, dstx+=colw, itemId++) {
          //TODO don't draw items we don't have
          ctx.drawImage(srcImage,
            itemId * this.constants.TILESIZE, 15 * this.constants.TILESIZE, this.constants.TILESIZE, this.constants.TILESIZE,
            dstx, dsty, this.constants.TILESIZE, this.constants.TILESIZE
          );
          //TODO quantity or qualifier
        }
      }
      
      const srcx = 0, srcy = this.constants.TILESIZE * 12;
      const dstx = gridx + colw * menu.selx + (colw >> 1) - this.constants.TILESIZE;
      const dsty = gridy + rowh * menu.sely + (rowh >> 1) - this.constants.TILESIZE;
      const w = this.constants.TILESIZE << 1;
      const h = this.constants.TILESIZE << 1;
      ctx.drawImage(srcImage, srcx, srcy, w, h, dstx, dsty, w, h);
    }
  }
}

RenderMenu.singleton = true;
