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
    this.dirtyTimeout = null;
    
    // If we don't get it from localStorage, set a sensible default configuration.
    if (!this.load()) {
      console.log(`Failed to load inputConfig, using defaults.`);
      this.keyMaps = [ // {code, btnid}
        { code: "ArrowLeft", btnid: InputManager.INPUT_LEFT },
        { code: "ArrowRight", btnid: InputManager.INPUT_RIGHT },
        { code: "ArrowUp", btnid: InputManager.INPUT_UP },
        { code: "ArrowDown", btnid: InputManager.INPUT_DOWN },
        { code: "KeyZ", btnid: InputManager.INPUT_USE },
        { code: "KeyX", btnid: InputManager.INPUT_MENU },
      ];
      this.gamepadMaps = []; // {id, axes, buttons}
    }
    
    this.listeners = [];
    this.nextListenerId = 1;
    
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
  
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({cb, id});
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p < 0) return;
    this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    for (const {cb} of this.listeners) cb(event);
  }
  
  forEachGamepad(cb) {
    for (const g of this.gamepads) {
      if (!g) continue;
      const result = cb(g);
      if (result) return result;
    }
    return null;
  }
  
  updateGamepadMap(map) {
    if (!map?.id) throw new Error(`Invalid gamepad map`);
    const p = this.gamepadMaps.findIndex(m => m.id === map.id);
    if (p >= 0) {
      this.gamepadMaps[p] = map;
    } else {
      this.gamepadMaps.push(map);
    }
    for (const gamepad of this.gamepads) {
      if (!gamepad) continue;
      if (gamepad.id !== map.id) continue;
      gamepad.axes = [...map.axes];
      gamepad.buttons = [...map.buttons];
      gamepad.axesState = gamepad.axes.map(() => 0);
      gamepad.buttonsState = gamepad.buttons.map(() => false);
      this.state &= ~gamepad.state;
      gamepad.state = 0;
    }
    this.dirty();
  }
  
  updateKeyMaps(map) {
    this.keyMaps = map.map(m => ({...m}));
    this.dirty();
  }
  
  /* Load and save localStorage.
   *******************************************************/
   
  dirty() {
    if (this.dirtyTimeout) {
      this.window.clearTimeout(this.dirtyTimeout);
    }
    this.dirtyTimeout = this.window.setTimeout(() => {
      this.dirtyTimeout = null;
      this.saveNow();
    }, 2000);
  }
  
  saveNow() {
    if (this.dirtyTimeout) {
      this.window.clearTimeout(this.dirtyTimeout);
      this.dirtyTimeout = null;
    }
    this.window.localStorage.setItem("inputConfig", this.encodeConfig());
    console.log(`Saved inputConfig`);
  }
  
  load() {
    try {
      const src = this.window.localStorage.getItem("inputConfig");
      if (!src) throw "inputConfig not found in localStorage.";
      const decoded = JSON.parse(src);
      this.keyMaps = decoded.keyMaps || [];
      this.gamepadMaps = decoded.gamepadMaps || [];
      return true;
    } catch (e) {
      return false;
    }
  }
  
  encodeConfig() {
    return JSON.stringify({
      keyMaps: this.keyMaps,
      gamepadMaps: this.gamepadMaps,
    });
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
    const map = this.keyMaps.find(m => m.code === code);
    if (map) return map.btnid;
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
        if (!gamepad.axes[i] && !this.listeners.length) continue;
        const vraw = src.axes[i];
        const v = (vraw >= axisThreshold) ? 1 : (vraw <= -axisThreshold) ? -1 : 0;
        if (v === gamepad.axesState[i]) continue;
        let btnidLo = 0, btnidHi = 0;
        switch (gamepad.axes[i]) {
          case "x": btnidLo = InputManager.INPUT_LEFT; btnidHi = InputManager.INPUT_RIGHT; break;
          case "y": btnidLo = InputManager.INPUT_UP; btnidHi = InputManager.INPUT_DOWN; break;
          default: if (!this.listeners.length) continue;
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
        if (this.listeners.length) this.broadcast({ type: "axis", device: src, axisIndex: i, v });
      }
      for (let i=gamepad.buttons.length; i-->0; ) {
        if (!gamepad.buttons[i] && !this.listeners.length) continue;
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
        if (this.listeners.length) this.broadcast({ type: "button", device: src, buttonIndex: i, v: gamepad.buttonsState[i] });
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
    this.broadcast({ type: "gamepadconnected", gamepad });
  }
  
  onGamepadDisconnected(e) {
    const gamepad = this.gamepads[e.gamepad.index];
    if (!gamepad) return;
    this.gamepads[e.gamepad.index] = null;
    if (gamepad.state) {
      this.state &= ~gamepad.state;
    }
    this.broadcast({ type: "gamepaddisconnected", gamepad });
  }
  
  _mapGamepad(gamepad) {
    const map = this.gamepadMaps.find(m => m.id === gamepad.id);
    if (map) {
      this._applyGamepadMap(gamepad, map);
      console.log(`gamepad ${JSON.stringify(gamepad.id)} mapped per config`);
    } else {
      console.log(`gamepad ${JSON.stringify(gamepad.id)} no map. ${gamepad.axes.length} axes, ${gamepad.buttons.length} buttons`);
    }
  }
  
  _applyGamepadMap(gamepad, map) {
    for (let i=Math.min(gamepad.axes.length, map.axes.length); i-->0; ) {
      gamepad.axes[i] = map.axes[i];
    }
    for (let i=Math.min(gamepad.buttons.length, map.buttons.length); i-->0; ) {
      gamepad.buttons[i] = map.buttons[i];
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
