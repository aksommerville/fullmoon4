/* InputManager.js
 * Keyboard, gamepad, touch.
 */
 
export class InputManager {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.state = 0;
    this.gamepads = []; // sparse, use provided index. {id, state, axes: [""|"x"|"y"...], buttons: [mask...], axesState: [number...], buttonsState: [boolean...] }
    this.touches = []; // {id, usage, x, y, anchorX, anchorY} usage: null, "dpad", "use", "menu"
    this.touchDeadRadius = 0;
    
    this.window.addEventListener("keydown", e => this.onKey(e, 1));
    this.window.addEventListener("keyup", e => this.onKey(e, 0));
    this.window.addEventListener("gamepadconnected", e => this.onGamepadConnected(e));
    this.window.addEventListener("gamepaddisconnected", e => this.onGamepadDisconnected(e));
  }
  
  registerTouchListeners(element) {
    element.addEventListener("touchstart", e => this.onTouchStart(e));
    element.addEventListener("touchend", e => this.onTouchEnd(e));
    element.addEventListener("touchcancel", e => this.onTouchEnd(e));
    element.addEventListener("touchmove", e => this.onTouchMove(e));
  }
  
  clearState() {
    this.state = 0;
  }
  
  update() {
    this._updateGamepad();
  }
  
  /* Keyboard.
   *******************************************/
   
  onKey(event, value) {
    if (event.repeat) return;
    const btnid = this.mapKey(event.code);
    if (!btnid) return;
    event.preventDefault();
    event.stopPropagation();
    if (value) {
      if (this.state & btnid) return;
      this.state |= btnid;
    } else {
      if (!(this.state & btnid)) return;
      this.state &= ~btnid;
    }
  }
  
  mapKey(code) {
    switch (code) {
      //TODO configurable
      case "ArrowLeft": return InputManager.INPUT_LEFT;
      case "ArrowRight": return InputManager.INPUT_RIGHT;
      case "ArrowUp": return InputManager.INPUT_UP;
      case "ArrowDown": return InputManager.INPUT_DOWN;
      case "KeyZ": return InputManager.INPUT_USE;
      case "KeyX": return InputManager.INPUT_MENU;
    }
    return null;
  }
  
  /* Gamepad.
   *****************************************************/
   
  _updateGamepad() {
    const axisThreshold = 0.1; // TODO configurable per device (per axis?)
    for (const src of this.window.navigator.getGamepads()) {
      if (!src) continue;
      const gamepad = this.gamepads[src.index];
      if (!gamepad) continue;
      for (let i=gamepad.axes.length; i-->0; ) {
        if (!gamepad.axes[i]) continue;
        const vraw = src.axes[i];
        const v = (vraw >= axisThreshold) ? 1 : (vraw <= -axisThreshold) ? -1 : 0;
        if (v === gamepad.axesState[i]) continue;
        let btnidLo, btnidHi;
        switch (gamepad.axes[i]) {
          case "x": btnidLo = InputManager.INPUT_LEFT; btnidHi = InputManager.INPUT_RIGHT; break;
          case "y": btnidLo = InputManager.INPUT_UP; btnidHi = InputManager.INPUT_DOWN; break;
          default: continue;
        }
        switch (gamepad.axesState[i]) {
          case -1: gamepad.state &= ~btnidLo; this.state &= ~btnidLo; break;
          case 1: gamepad.state &= ~btnidHi; this.state &= ~btnidHi; break;
        }
        switch (v) {
          case -1: gamepad.state |= btnidLo; this.state |= btnidLo; break;
          case 1: gamepad.state |= btnidHi; this.state |= btnidHi; break;
        }
        gamepad.axesState[i] = v;
      }
      for (let i=gamepad.buttons.length; i-->0; ) {
        if (!gamepad.buttons[i]) continue;
        if (src.buttons[i].value) {
          if (gamepad.buttonsState[i]) continue;
          gamepad.buttonsState[i] = true;
          gamepad.state |= gamepad.buttons[i];
          this.state |= gamepad.buttons[i];
        } else {
          if (!gamepad.buttonsState[i]) continue;
          gamepad.buttonsState[i] = false;
          gamepad.state &= ~gamepad.buttons[i];
          this.state &= ~gamepad.buttons[i];
        }
      }
    }
  }
  
  onGamepadConnected(e) {
    let gamepad = this.gamepads[e.gamepad.index];
    if (!gamepad) {
      gamepad = {
        id: e.gamepad.id,
        index: e.gamepad.index,
        state: 0,
      };
      this.gamepads[e.gamepad.index] = gamepad;
    }
    gamepad.axes = e.gamepad.axes.map(() => "");
    gamepad.buttons = e.gamepad.buttons.map(() => "");
    gamepad.axesState = e.gamepad.axes.map(() => 0);
    gamepad.buttonsState = e.gamepad.buttons.map(() => false);
    this._mapGamepad(gamepad);
  }
  
  onGamepadDisconnected(e) {
    const gamepad = this.gamepads[e.gamepad.index];
    if (!gamepad) return;
    this.gamepads[e.gamepad.index] = null;
    if (gamepad.state) {
      this.state &= ~gamepad.state;
    }
  }
  
  _mapGamepad(gamepad) {
    console.log(`TODO InputManager._mapGamepad ac=${gamepad.axes.length} bc=${gamepad.buttons.length} id=${JSON.stringify(gamepad.id)}`);
    
    // PS2 knockoffs, Linux, Chrome.
    if (gamepad.id === "MY-POWER CO.,LTD. 2In1 USB Joystick (STANDARD GAMEPAD Vendor: 0e8f Product: 0003)") {
      //if (gamepad.axes.length > 0) gamepad.axes[0] = "x"; // lx
      //if (gamepad.axes.length > 1) gamepad.axes[1] = "y"; // ly
      if (gamepad.buttons.length > 0) gamepad.buttons[0] = InputManager.INPUT_USE; // south
      if (gamepad.buttons.length > 2) gamepad.buttons[2] = InputManager.INPUT_MENU; // west
      if (gamepad.buttons.length > 0) gamepad.buttons[12] = InputManager.INPUT_UP; // dpad...
      if (gamepad.buttons.length > 0) gamepad.buttons[13] = InputManager.INPUT_DOWN;
      if (gamepad.buttons.length > 0) gamepad.buttons[14] = InputManager.INPUT_LEFT;
      if (gamepad.buttons.length > 0) gamepad.buttons[15] = InputManager.INPUT_RIGHT;
      return;
    }
  }
  
  /* Touch.
   ************************************************************/
   
  onTouchStart(e) {
    e.preventDefault();
    let highestElement = e.target;
    while (highestElement.parentNode && (highestElement.tagName !== "HTML")) highestElement = highestElement.parentNode;
    if (!this.touchDeadRadius) {
      this.touchDeadRadius = Math.min(highestElement.clientWidth, highestElement.clientHeight) / 8; // 8 arbitrarily
    }
    //console.log(`  client size ${highestElement.clientWidth},${highestElement.clientHeight}`, highestElement);
    for (const touch of e.changedTouches) {
      const t = {
        x: touch.clientX,
        y: touch.clientY,
        anchorX: touch.clientX,
        anchorY: touch.clientY,
        id: touch.identifier,
      };
      this.touches.push(t);
      if (touch.clientX < highestElement.clientWidth / 2) {
        t.usage = "dpad";
      } else if (touch.clientY < highestElement.clientHeight / 2) {
        t.usage = "menu";
        this.state |= InputManager.INPUT_MENU;
      } else {
        t.usage = "use";
        this.state |= InputManager.INPUT_USE;
      }
    }
  }
  
  onTouchEnd(e) {
    e.preventDefault();
    for (const touch of e.changedTouches) {
      const p = this.touches.findIndex(t => t.id === touch.identifier);
      if (p >= 0) {
        const t = this.touches[p];
        this.touches.splice(p, 1);
        switch (t.usage) {
          case "dpad": this.state &= ~(InputManager.INPUT_LEFT | InputManager.INPUT_RIGHT | InputManager.INPUT_UP | InputManager.INPUT_DOWN); break;
          case "use": this.state &= ~InputManager.INPUT_USE; break;
          case "menu": this.state &= ~InputManager.INPUT_MENU; break;
        }
      }
    }
  }
  
  onTouchMove(e) {
    e.preventDefault();
    for (const touch of e.changedTouches) {
      const t = this.touches.find(tt => tt.id === touch.identifier);
      if (!t) continue;
      if (t.usage !== "dpad") continue;
      t.x = touch.clientX;
      t.y = touch.clientY;
      const dx = Math.trunc((t.x - t.anchorX) / this.touchDeadRadius);
      const dy = Math.trunc((t.y - t.anchorY) / this.touchDeadRadius);
      if (dx < 0) {
        this.state &= ~InputManager.INPUT_RIGHT;
        this.state |= InputManager.INPUT_LEFT;
      } else if (dx > 0) {
        this.state &= ~InputManager.INPUT_LEFT;
        this.state |= InputManager.INPUT_RIGHT;
      } else {
        this.state &= ~(InputManager.INPUT_LEFT | InputManager.INPUT_RIGHT);
      }
      if (dy < 0) {
        this.state &= ~InputManager.INPUT_DOWN;
        this.state |= InputManager.INPUT_UP;
      } else if (dy > 0) {
        this.state &= ~InputManager.INPUT_UP;
        this.state |= InputManager.INPUT_DOWN;
      } else {
        this.state &= ~(InputManager.INPUT_UP | InputManager.INPUT_DOWN);
      }
    }
  }
}

InputManager.singleton = true;

InputManager.INPUT_LEFT  = 0x01;
InputManager.INPUT_RIGHT = 0x02;
InputManager.INPUT_UP    = 0x04;
InputManager.INPUT_DOWN  = 0x08;
InputManager.INPUT_USE   = 0x10;
InputManager.INPUT_MENU  = 0x20;
