/* RootUi.js
 * Top level view controller.
 */
 
import { Dom } from "/js/util/Dom.js";
import { WasmLoader } from "/js/util/WasmLoader.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom, WasmLoader, Window];
  }
  constructor(element, dom, wasmLoader, window) {
    this.element = element;
    this.dom = dom;
    this.wasmLoader = wasmLoader;
    this.window = window;
    
    this.inputState = 0;//XXX should not be my concern
    this.terminate = false;
    
    this.buildUi();
    
    this.wasmLoader.load("/fullmoon.wasm")
      .then((instance) => this.onLoaded(instance))
      .catch((e) => this.onLoadError(e));
      
    this.element.setAttribute("tabindex", "-1");
    this.element.addEventListener("keydown", e => this.onKey(e, 1));
    this.element.addEventListener("keyup", e => this.onKey(e, 0));
    this.element.focus();
    this.element.addEventListener("click", () => { this.terminate = true; });
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "DIV", ["loading"], "LOADING");
    this.dom.spawn(this.element, "DIV", ["errorMessage", "hidden"]);
    const gameView = this.dom.spawn(this.element, "CANVAS", ["gameView", "hidden"]);
  }
  
  displayError(message) {
    this.element.querySelector(".loading").classList.add("hidden");
    this.element.querySelector(".gameView").classList.add("hidden");
    const container = this.element.querySelector(".errorMessage");
    container.classList.remove("hidden");
    container.innerHTML = "";
    container.innerText = message;
  }
  
  displayGame() {
    this.element.querySelector(".loading").classList.add("hidden");
    this.element.querySelector(".errorMessage").classList.add("hidden");
    const gameView = this.element.querySelector(".gameView");
    gameView.classList.remove("hidden");
  }
  
  onLoaded(instance) {
    const status = this.wasmLoader.instance.exports.fmn_init()
    if (status<0) {
      this.displayError(`fmn_init() failed (${status})`);
      this.wasmLoader.abort();
    } else {
      this.displayGame();
      this.renderNow();
      this.window.requestAnimationFrame(() => this.update());
    }
  }
  
  onLoadError(error) {
    console.error(`RootUi.onLoadError`, error);
    if (error && error.log) {
      this.displayError(error.log);
    } else {
      this.displayError(JSON.stringify(error));
    }
  }
  
  update() {
    if (this.terminate) return;
    this.wasmLoader.instance.exports.fmn_update(Date.now(), this.input);
    this.renderNow();
    this.window.requestAnimationFrame(() => this.update());
  }
  
  renderNow() {
    const gameView = this.element.querySelector(".gameView");
    gameView.width = 640/2; // TODO framebuffer size
    gameView.height = 384/2;
    const context = gameView.getContext("2d");
    if (this.wasmLoader.sceneAddress) {
      //TODO transitions
      this.renderScene(gameView, context, this.wasmLoader.sceneAddress);
    } else {
      context.fillStyle = "#00f";
      context.fillRect(0, 0, gameView.width, gameView.height);
    }
    for (const menu of this.wasmLoader.menus) {
      this.renderMenu(gameView, context, menu);
    }
  }
  
  renderScene(gameView, context, sceneAddress) {
    const colw = gameView.width / this.wasmLoader.COLC;
    const rowh = gameView.height / this.wasmLoader.ROWC;
    
    if (this.wasmLoader.mapTilesheetImage) {
      //context.drawImage(this.wasmLoader.mapTilesheetImage, 0, 0);
      /* This is very expensive to render. Draw it to a temporary buffer and copy that in bulk each frame. */
      for (let row=0,y=0,p=sceneAddress; row<this.wasmLoader.ROWC; row++,y+=rowh) {
        for (let col=0,x=0; col<this.wasmLoader.COLC; col++,x+=colw,p++) {
          // Draw tile zero behind each cell.
          context.drawImage(
            this.wasmLoader.mapTilesheetImage,
            0, 0, colw, rowh,
            x, y, colw, rowh
          );
          const tileid = this.wasmLoader.memU8[p];
          if (tileid) {
            context.drawImage(
              this.wasmLoader.mapTilesheetImage,
              (tileid & 15) * colw, (tileid >> 4) * rowh, colw, rowh,
              x, y, colw, rowh
            );
          }
        }
      }
      /**/
    } else {
      context.fillStyle = "#000";
      context.fillRect(0, 0, gameView.width, gameView.height);
    }
    
    if (this.wasmLoader.mapTilesheetImage) { // XXX sprites are not all from the same tilesheet, and typically nothing to do with the map tilesheet
      let spritec = this.wasmLoader.readU32(sceneAddress + this.wasmLoader.COLC * this.wasmLoader.ROWC + 16);
      if (spritec > 0) {
        context.fillStyle = "#888";
        let spritevp = this.wasmLoader.readU32(sceneAddress + this.wasmLoader.COLC * this.wasmLoader.ROWC + 12);
        for (; spritec-->0; spritevp+=4) {
          const spritep = this.wasmLoader.readU32(spritevp);
          const x = this.wasmLoader.readF32(spritep);
          const y = this.wasmLoader.readF32(spritep + 4);
          //8:imageid
          const tileid = this.wasmLoader.readU8(spritep + 9);
          //10:xform
          context.drawImage(
            this.wasmLoader.mapTilesheetImage,
            (tileid & 15) * colw, (tileid >> 4) * rowh, colw, rowh,
            Math.floor(x * colw - (colw/2)), Math.floor(y * rowh - (rowh/2)), colw, rowh
          );
        }
      }
    }
  }
  
  renderMenu(gameView, context, menu) {
    context.fillStyle = "#fff";
    context.fillText(menu.prompt, 0, 10);
    context.fillStyle = "#ff0";
    let y = 20, p = 0;
    for (const option of menu.options) {
      let s = option.string;
      if (menu.selectedOptionIndex === p) s += " <--";
      context.fillText(s, 0, y);
      y += 10;
      p++;
    }
  }
  
  //TODO input manager. this is highly temporary
  onKey(event, value) {
    let btnid = this.btnidByKeyCode(event.code);
    if (!btnid) return;
    event.stopPropagation();
    event.preventDefault();
    const menu = this.wasmLoader.menus[this.wasmLoader.menus.length - 1];
    if (value) {
      if (this.inputState & btnid) return;
      this.inputState |= btnid;
    } else {
      if (!(this.inputState & btnid)) return;
      this.inputState &= ~btnid;
    }
    if (menu) {
      if (menu.input(btnid, value)) {
        this.renderNow();
      }
    }
  }
  
  btnidByKeyCode(code) {
    /* fmn_platform.h:
#define FMN_INPUT_LEFT     0x01
#define FMN_INPUT_RIGHT    0x02
#define FMN_INPUT_UP       0x04
#define FMN_INPUT_DOWN     0x08
#define FMN_INPUT_USE      0x10
#define FMN_INPUT_MENU     0x20
    */
    switch (code) {
      case "ArrowLeft": return 0x01;
      case "ArrowRight": return 0x02;
      case "ArrowUp": return 0x04;
      case "ArrowDown": return 0x08;
      case "KeyZ": return 0x10;
      case "KeyX": return 0x20;
    }
    return 0;
  }
}
