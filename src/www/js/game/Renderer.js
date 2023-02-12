/* Renderer.js
 * No actual rendering here! That's a big enough job that we've split it into a few sections.
 * We are only the point of contact for Runtime, to all the rendering logic.
 */
 
import { Constants } from "./Constants.js";
import { RenderTransitions } from "./RenderTransitions.js";
import { RenderMap } from "./RenderMap.js";
import { RenderSprites } from "./RenderSprites.js";
import { RenderHero } from "./RenderHero.js";
import { RenderMenu } from "./RenderMenu.js";
 
export class Renderer {
  static getDependencies() {
    return [Constants, RenderTransitions, RenderMap, RenderSprites, RenderHero, RenderMenu];
  }
  constructor(constants, renderTransitions, renderMap, renderSprites, renderHero, renderMenu) {
    this.constants = constants;
    this.renderTransitions = renderTransitions;
    this.renderMap = renderMap;
    this.renderSprites = renderSprites;
    this.renderHero = renderHero;
    this.renderMenu = renderMenu;
    
    this.canvas = null;
    this.frameCount = 0;
    
    this.renderTransitions.renderScene = dst => this._renderScene(dst);
  }
  
  /* Public API.
   ******************************************************************/
  
  mapDirty() {
    this.renderMap.setDirty();
  }
  
  setRenderTarget(canvas) {
    if (this.canvas = canvas) {
      this.canvas.width = this.constants.COLC * this.constants.TILESIZE;
      this.canvas.height = this.constants.ROWC * this.constants.TILESIZE;
      this.renderTransitions.resize(this.canvas.width, this.canvas.height);
    }
  }
  
  prepareTransition(tmode) {
    this.renderTransitions.prepare(tmode);
  }
  
  commitTransition() {
    this.renderTransitions.commit();
  }
  
  cancelTransition() {
    this.renderTransitions.cancel();
  }
  
  // Zero (CUT) means "no transition in progress", probably all you're interested in.
  getTransitionMode() {
    return this.renderTransitions.mode;
  }
  
  render(menus) {
    if (!this.canvas) return;
    this.frameCount++;
    if (!this.renderTransitions.render(this.canvas)) {
      this._renderScene(this.canvas, true);
    }
    this.renderMenu.render(this.canvas, menus);
  }
  
  /* Private.
   **********************************************************/
   
  _renderScene(canvas, withOverlay) {
    const ctx = canvas.getContext("2d");
    const bg = this.renderMap.update();
    ctx.drawImage(bg, 0, 0);
    if (true||withOverlay) this.renderHero.renderUnderlay(canvas, ctx);
    this.renderSprites.render(canvas, ctx);
    if (withOverlay) this.renderHero.renderOverlay(canvas, ctx);
  }
}

Renderer.singleton = true;

