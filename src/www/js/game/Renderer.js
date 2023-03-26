/* Renderer.js
 * No actual rendering here! That's a big enough job that we've split it into a few sections.
 * We are only the point of contact for Runtime, to all the rendering logic.
 */
 
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
import { RenderTransitions } from "./RenderTransitions.js";
import { RenderMap } from "./RenderMap.js";
import { RenderSprites } from "./RenderSprites.js";
import { RenderHero } from "./RenderHero.js";
import { RenderMenu } from "./RenderMenu.js";
import { RenderViolin } from "./RenderViolin.js";
 
export class Renderer {
  static getDependencies() {
    return [
      Constants, Globals,
      RenderTransitions, RenderMap, RenderSprites, RenderHero, RenderMenu, RenderViolin,
    ];
  }
  constructor(
    constants, globals,
    renderTransitions, renderMap, renderSprites, renderHero, renderMenu, renderViolin
  ) {
    this.constants = constants;
    this.globals = globals;
    this.renderTransitions = renderTransitions;
    this.renderMap = renderMap;
    this.renderSprites = renderSprites;
    this.renderHero = renderHero;
    this.renderMenu = renderMenu;
    this.renderViolin = renderViolin;
    
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
    this.renderMap.resetWeather();
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
    if (!this.globals.p_fmn_global) return;
    this.frameCount++;
    
    if (!this.renderTransitions.render(this.canvas)) {
      this._renderScene(this.canvas, true);
    }
    
    if (this.globals.g_active_item[0] === this.constants.ITEM_VIOLIN) {
      this.renderViolin.render(this.canvas);
    } else if (this.renderViolin.running) {
      this.renderViolin.reset();
    }
    
    if (this.globals.g_werewolf_dead[0] || this.globals.g_hero_dead[0]) {
      const FADE_OUT_TIME = 2.0;
      const lightness = this.globals.g_terminate_time[0] / FADE_OUT_TIME;
      if (lightness < 1) {
        const ctx = this.canvas.getContext("2d");
        ctx.globalAlpha = 1 - lightness;
        ctx.fillStyle = "#000";
        ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);
        ctx.globalAlpha = 1;
      }
    }
    
    this.renderMenu.render(this.canvas, menus);
  }
  
  /* Private.
   **********************************************************/
   
  _renderScene(canvas, withOverlay) {
    const ctx = canvas.getContext("2d");
    if (this.globals.p_fmn_global) {
      const bg = this.renderMap.update();
      ctx.drawImage(bg, 0, 0);
      this.renderMap.renderDarkness(canvas, ctx);
      if (withOverlay) this.renderHero.renderUnderlay(canvas, ctx);
      this.renderSprites.render(canvas, ctx);
      if (withOverlay) this.renderHero.renderOverlay(canvas, ctx);
      this.renderMap.renderWeather(canvas, ctx);
    } else {
      ctx.fillStyle = "#000";
      ctx.fillRect(0, 0, canvas.width, canvas.height);
    }
  }
}

Renderer.singleton = true;

