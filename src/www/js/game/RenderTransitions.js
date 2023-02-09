/* RenderTransitions.js
 * Subservient to Render, responsible only for transitions between two scenes.
 */
 
import { Dom } from "../util/Dom.js";
import { Constants } from "./Constants.js";
import { Globals } from "./Globals.js";
 
export class RenderTransitions {
  static getDependencies() {
    return [Dom, Window, Constants, Globals];
  }
  constructor(dom, window, constants, globals) {
    this.dom = dom;
    this.window = window;
    this.constants = constants;
    this.globals = globals;
    
    /* Owner (Renderer) must set this at construction, to generate the incoming and outgoing scene images.
     * The outgoing image is captured once, when a transition starts.
     * Incoming will be rendered as usual, each frame.
     */
    this.renderScene = (dst) => {};
    
    this.incomingCanvas = null;
    this.outgoingCanvas = null;
    this.mode = 0; // FMN_TRANSITION_*. 0 is "CUT" which means "no transition"
    this.startTime = 0;
    this.endTime = 0;
    this.focusX = 0; // captured at prepare, for DOOR mode only.
    this.focusY = 0;
    this.subphase = 0; // zeroed at prepare, implementations can use for whatever
  }
  
  resize(w, h) {
    this.incomingCanvas = this.dom.createElement("CANVAS", { width: w, height: h });
    this.outgoingCanvas = this.dom.createElement("CANVAS", { width: w, height: h });
  }
  
  prepare(mode) {
    if (!this.outgoingCanvas) return;
    this.mode = mode;
    this.subphase = 0;
    this.renderScene(this.outgoingCanvas);
    switch (this.mode) {
      case this.constants.TRANSITION_DOOR: {
          const sprite = this.globals.getHeroSprite();
          if (sprite) {
            this.focusX = sprite.x * this.constants.TILESIZE;
            this.focusY = sprite.y * this.constants.TILESIZE;
          } else {
            this.focusX = this.outgoingCanvas.width * 0.5;
            this.focusY = this.outgoingCanvas.height * 0.5;
          }
        } break;
    }
  }
  
  commit() {
    if (!this.mode) return;
    this.startTime = this.window.Date.now();
    this.endTime = this.startTime + this.constants.TRANSITION_TIME_MS;
  }
  
  cancel() {
    this.mode = 0;
  }
  
  /* If a transition is in progress, we render it and return true.
   * No transition, we do nothing and return false.
   */
  render(dst) {
    if (!this.mode) return false;
    const now = this.window.Date.now();
    if ((now < this.startTime) || (now >= this.endTime)) {
      this.mode = 0;
      return false;
    }
    this.renderScene(this.incomingCanvas);
    const t = (now - this.startTime) / (this.endTime - this.startTime);
    switch (this.mode) {
      case this.constants.TRANSITION_PAN_LEFT: this._renderPan(dst, t, -1, 0); return true;
      case this.constants.TRANSITION_PAN_RIGHT: this._renderPan(dst, t, 1, 0); return true;
      case this.constants.TRANSITION_PAN_UP: this._renderPan(dst, t, 0, -1); return true;
      case this.constants.TRANSITION_PAN_DOWN: this._renderPan(dst, t, 0, 1); return true;
      case this.constants.TRANSITION_FADE_BLACK: this._renderFadeBlack(dst, t); return true;
      case this.constants.TRANSITION_DOOR: this._renderDoor(dst, t); return true;
      case this.constants.TRANSITION_WARP: this._renderWarp(dst, t); return true;
    }
    this.mode = 0;
    return false;
  }
  
  /* Pan.
   ***************************************************/
   
  _renderPan(dst, t, dx, dy) {
    const screenw = dst.width;
    const screenh = dst.height;
    const outdstx = Math.floor(-dx * t * screenw);
    const outdsty = Math.floor(-dy * t * screenh);
    const indstx = outdstx + dx * screenw;
    const indsty = outdsty + dy * screenh;
    const ctx = dst.getContext("2d");
    ctx.drawImage(this.incomingCanvas,
      0, 0, screenw, screenh,
      indstx, indsty, screenw, screenh
    );
    ctx.drawImage(this.outgoingCanvas,
      0, 0, screenw, screenh,
      outdstx, outdsty, screenw, screenh
    );
  }
  
  /* Fade to black and back.
   ****************************************************/
   
  _renderFadeBlack(dst, t) {
    const ctx = dst.getContext("2d");
    if (t < 0.5) {
      ctx.drawImage(this.outgoingCanvas, 0, 0);
      ctx.globalAlpha = t * 2;
    } else {
      ctx.drawImage(this.incomingCanvas, 0, 0);
      ctx.globalAlpha = (1 - t) * 2;
    }
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, dst.width, dst.height);
    ctx.globalAlpha = 1;
  }
  
  /* Crossfade.
   ******************************************************/
   
  _renderCrossfade(dst, t) {
    const ctx = dst.getContext("2d");
    ctx.drawImage(this.outgoingCanvas, 0, 0);
    ctx.globalAlpha = t;
    ctx.drawImage(this.incomingCanvas, 0, 0);
    ctx.globalAlpha = 1;
  }
  
  /* Door: Spotlight in to egress point, and then out from ingress point.
   **************************************************/
  
  _renderDoor(dst, t) {//ctx, prev, next, p, hero) {
    const ctx = dst.getContext("2d");
    if (t < 0.5) {
      this._renderDoor1(dst, ctx, this.outgoingCanvas, (0.5 - t) * 2);
    } else {
      if (!this.subphase) {
        this.subphase = 1;
        const sprite = this.globals.getHeroSprite();
        if (sprite) {
          this.focusX = sprite.x * this.constants.TILESIZE;
          this.focusY = sprite.y * this.constants.TILESIZE;
        } else {
          this.focusX = this.outgoingCanvas.width * 0.5;
          this.focusY = this.outgoingCanvas.height * 0.5;
        }
      }
      this._renderDoor1(dst, ctx, this.incomingCanvas, (t - 0.5) * 2);
    }
  }
  _renderDoor1(dst, ctx, src, t) {
    ctx.drawImage(src, 0, 0);
    
    let radiusLimit;
    if (this.focusX < dst.width / 2) {
      if (this.focusY < dst.height / 2) {
        radiusLimit = Math.sqrt((dst.width - this.focusX) ** 2 + (dst.height - this.focusY) ** 2);
      } else {
        radiusLimit = Math.sqrt((dst.width - this.focusX) ** 2 + this.focusY ** 2);
      }
    } else {
      if (this.focusY < dst.height / 2) {
        radiusLimit = Math.sqrt(this.focusX ** 2 + (dst.height - this.focusY) ** 2);
      } else {
        radiusLimit = Math.sqrt(this.focusY ** 2 + this.focusY ** 2);
      }
    }
    const radius = t * radiusLimit;
    
    ctx.beginPath();
    ctx.moveTo(this.focusX, 0);
    ctx.lineTo(this.focusX, this.focusY - radius);
    ctx.arcTo(this.focusX - radius, this.focusY - radius, this.focusX - radius, this.focusY, radius);
    ctx.arcTo(this.focusX - radius, this.focusY + radius, this.focusX, this.focusY + radius, radius);
    ctx.arcTo(this.focusX + radius, this.focusY + radius, this.focusX + radius, this.focusY, radius);
    ctx.arcTo(this.focusX + radius, this.focusY - radius, this.focusX, this.focusY - radius, radius);
    ctx.lineTo(this.focusX, 0);
    ctx.lineTo(dst.width, 0);
    ctx.lineTo(dst.width, dst.height);
    ctx.lineTo(0, dst.height);
    ctx.lineTo(0, 0);
    ctx.fillStyle = "#000";
    ctx.fill();
  }
  
  /* Warp: Crossfade.
   *************************************************/
   
  _renderWarp(dst, t) {
    this._renderCrossfade(dst, t);
  }
}

RenderTransitions.singleton = true;
