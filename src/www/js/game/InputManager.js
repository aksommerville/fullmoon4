/* InputManager.js
 * Keyboard, gamepad, touch.
 */
 
import * as FMN from "./Constants.js";
 
export class InputManager {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.state = 0;
    this.gamepads = []; // sparse, use provided index. {id, name, state, axes: [""|"x"|"y"|"hat"...], buttons: [mask...], axesState: [number...], buttonsState: [boolean...] }
    this.touches = []; // {id, usage, x, y, anchorX, anchorY} usage: null, "dpad", "use", "menu"
    this.touchDeadRadius = 0;
    this.dirtyTimeout = null;
    this.incfg = {
      state: FMN.INCFG_STATE_NONE,
      devp: 0,
      btnid: 0,
    };
    this.incfgPrivate = {
      gamepad: null,
      srcbtnid: 0, // Nonzero if awaiting a release.
      qualifier: undefined, // ''
      initialSrcbtnid: 0, // Relevant during CONFIRM stage.
      initialQualifier: undefined, // ''
      maps: [], // [[srcbtnid,qualifier,dstbtnid],...] ; (dstbtnid) is actually redundant but makes me feel safer
    };
    
    // If we don't get it from localStorage, set a sensible default configuration.
    if (!this.load()) {
      //console.log(`Failed to load inputConfig, using defaults.`);
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
  
  simulateConnectionOfUnmappedDevices() {
    for (const g of this.gamepads) {
      if (!g) continue;
      if (this.gamepadMaps.find(m => m.id === g.id)) continue;
      this.broadcast({ type: "gamepadconnected", gamepad: g });
    }
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
    //console.log(`Saved inputConfig`);
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
  
    if (this.incfg.state && !this.incfg.devp) {
      //console.log(`TODO Intercept keystroke for incfg`, { event, value });
      // While live config in progress, we eat every keyboard event...
      event.preventDefault();
      event.stopPropagation();
      if (value === 1) {
        this._advanceIncfg(event.code);
      } else if (value === 0) {
        this._releaseIncfg(event.code);
      }
      return;
    }
  
    const btnid = this.mapKey(event.code);
    if (!btnid) return;
    event.preventDefault();
    event.stopPropagation();
    if (event.repeat) return; // Repeats for mapped keys, we do still want to preventDefault
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
   
  /* (v) in -1..1, output in -1..7.
   */
  _normalizeHat(v) {
    if ((v < -1) || (v > 1)) return -1;
    return Math.min(7, Math.max(0, Math.round(((v + 1) * 7) / 2)));
  }

  _updateGamepadHat(src, gamepad, axisIndex) {
    const state = this._normalizeHat(src.axes[axisIndex])
    if (state === gamepad.axesState[axisIndex]) return;
    gamepad.axesState[axisIndex] = state;
    let x, y;
    switch (state) {
      case 0:         y = -1; break;
      case 1: x =  1; y = -1; break;
      case 2: x =  1;         break;
      case 3: x =  1; y =  1; break;
      case 4:         y =  1; break;
      case 5: x = -1; y =  1; break;
      case 6: x = -1;         break;
      case 7: x = -1; y = -1; break;
    }
    const DPAD = InputManager.INPUT_LEFT | InputManager.INPUT_RIGHT | InputManager.INPUT_UP | InputManager.INPUT_DOWN;
    gamepad.state &= ~DPAD;
    this.state &= ~DPAD;
         if (x < 0) { gamepad.state |= InputManager.INPUT_LEFT;  this.state |= InputManager.INPUT_LEFT; }
    else if (x > 0) { gamepad.state |= InputManager.INPUT_RIGHT; this.state |= InputManager.INPUT_RIGHT; }
         if (y < 0) { gamepad.state |= InputManager.INPUT_UP;    this.state |= InputManager.INPUT_UP; }
    else if (y > 0) { gamepad.state |= InputManager.INPUT_DOWN;  this.state |= InputManager.INPUT_DOWN; }
    if (this.listeners.length) this.broadcast({ type: "axis", device: src, axisIndex, v: state });
  }
   
  _updateGamepad() {
    const axisThreshold = 0.25; // TODO configurable per device (per axis?)
    if (!this.window.navigator.getGamepads) return; // old browser, or unsecure connection
    for (const src of this.window.navigator.getGamepads()) {
      if (!src) continue;
      const gamepad = this.gamepads[src.index];
      if (!gamepad) continue;
      
      if (this.incfg.state && (this.incfgPrivate.gamepad === gamepad)) {
        this._updateGamepadIncfg(gamepad, src, axisThreshold);
        continue;
      }
      
      for (let i=gamepad.axes.length; i-->0; ) {
        if (!gamepad.axes[i] && !this.listeners.length) continue;
        if (gamepad.axes[i] === "hat") {
          this._updateGamepadHat(src, gamepad, i);
          continue;
        }
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
  
  /* Call with the gamepad currently in progress of incfg.
   */
  _updateGamepadIncfg(gamepad, src, axisThreshold) {
    let virgin = true; // Advance incfg only once per cycle. I guess we don't care which button, when there's multiple.
    for (let i=gamepad.axes.length; i-->0; ) {
    
      if (gamepad.axes[i] === "hat") {
        const state = this._normalizeHat(v);
        if (state === gamepad.axesState) continue;
        gamepad.axesState[i] = state;
        let qualifier = "";
        switch (state) { // Ignore diagonals.
          case 0: qualifier = "hatn"; break;
          case 2: qualifier = "hate"; break; // <-- "hate" "hats", i sure do
          case 4: qualifier = "hats"; break;
          case 6: qualifier = "hatw"; break;
        }
        if (qualifier && virgin) {
          virgin = false;
          this._advanceIncfg(i, qualifier);
        } else if (state === -1) { // must fully release to drop the stroke, and we don't bother tracking which direction
          this._releaseIncfg(i, "hatn");
          this._releaseIncfg(i, "hate");
          this._releaseIncfg(i, "hats");
          this._releaseIncfg(i, "hatw");
        }
        continue;
      }
        
      const v = (src.axes[i] >= axisThreshold) ? 1 : (src.axes[i] <= -axisThreshold) ? -1 : 0;
      if (v === gamepad.axesState[i]) continue;
      const pv = gamepad.axesState[i];
      gamepad.axesState[i] = v;
      if (v && virgin) {
        virgin = false;
        this._advanceIncfg(i, (v < 0) ? "axislo" : "axishi");
      } else if (!v) {
        this._releaseIncfg(i, (pv < 0) ? "axislo" : "axishi");
      }
    }
    for (let i=gamepad.buttons.length; i-->0; ) {
      if (src.buttons[i].value) {
        if (gamepad.buttonsState[i]) continue;
        gamepad.buttonsState[i] = true;
        if (virgin) {
          virgin = false;
          this._advanceIncfg(i, "button");
        }
      } else {
        if (!gamepad.buttonsState[i]) continue;
        gamepad.buttonsState[i] = false;
        this._releaseIncfg(i, "button");
      }
    }
  }
  
  onGamepadConnected(e) {
    if (!e.gamepad) return;
    let gamepad = this.gamepads[e.gamepad.index];
    if (!gamepad) {
      gamepad = {
        id: e.gamepad.id,
        name: this._sanitizeGamepadName(e.gamepad.id),
        index: e.gamepad.index,
        state: 0,
      };
      this.gamepads[e.gamepad.index] = gamepad;
    }
    //console.log(`InputManager.onGamepadConnected`, e);
    gamepad.axes = e.gamepad.axes.map(v => {
      // MacOS unhelpfully reports dpads as hats regardless of the hardware's representation.
      // And Chrome makes it even worse by mapping that 0..7 or whatever, to -1..1.
      // These remapped hats should have a value outside -1..1 when at rest. Hopefully they are at rest at connection.
      if ((v < -1) || (v > 1)) return "hat";
      return "";
    });
    gamepad.buttons = e.gamepad.buttons.map(() => "");
    gamepad.axesState = e.gamepad.axes.map(() => 0);
    gamepad.buttonsState = e.gamepad.buttons.map(() => false);
    this._mapGamepad(gamepad);
    this.broadcast({ type: "gamepadconnected", gamepad });
  }
  
  onGamepadDisconnected(e) {
    if (!e.gamepad) return;
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
      //console.log(`gamepad ${JSON.stringify(gamepad.id)} mapped per config`);
    } else {
      //console.log(`gamepad ${JSON.stringify(gamepad.id)} no map. ${gamepad.axes.length} axes, ${gamepad.buttons.length} buttons`);
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
  
  _sanitizeGamepadName(input) {
    if (!input) return "Gamepad";
    const clean = input
      .replace(/\(.*\)/g, '')
      .replace(/[^a-zA-Z0-9 ]/g, ' ')
      .trim()
      .replace(/  /g, ' ');
    return clean;
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
  
  /* API to support interactive config.
   * These are Javascriptified but otherwise match the fmn_platform API.
   *******************************************************************/
   
  getInputDeviceName(devp) { // => string
    if ((devp < 0) || (typeof(devp) !== "number")) return "";
    
    // System Keyboard first.
    if (!devp--) return "System Keyboard";
    
    // Then all attached gamepads.
    // Beware that the array can be sparse, but (devp) must be contiguous! We have to walk it and check each member.
    for (let i=0; i<this.gamepads.length; i++) {
      if (!this.gamepads[i]) continue;
      if (devp--) continue;
      return this.gamepads[i].name;
    }
    
    // Touch input doesn't need to participate.
    return "";
  }
  
  beginInputConfiguration(devp) {
    // Important that this logic match getInputDeviceName() exactly.
    if ((devp < 0) || (typeof(devp) !== "number")) return "";
    const devp0 = devp;
    if (!devp--) return this._beginIncfgKeyboard();
    for (let i=0; i<this.gamepads.length; i++) {
      if (!this.gamepads[i]) continue;
      if (devp--) continue;
      return this._beginIncfgGamepad(i, devp0);
    }
  }
  
  cancelInputConfiguration() {
    this.incfg = {
      state: FMN.INCFG_STATE_NONE,
      devp: 0,
      btnid: 0,
    };
    this.incfgPrivate = {
      gamepad: null,
      srcbtnid: 0,
      qualifier: undefined,
      initialSrcbtnid: 0,
      initialQualifier: undefined,
      maps: [],
    };
  }
  
  getInputConfigurationState() { // => {state,devp,btnid}
    return this.incfg;
  }
  
  _beginIncfgKeyboard() {
    this.incfg = {
      state: FMN.INCFG_STATE_READY,
      devp: 0,
      btnid: InputManager.INPUT_LEFT,
    };
    this.incfgPrivate = {
      gamepad: null,
      srcbtnid: 0,
      qualifier: undefined,
      initialSrcbtnid: 0,
      initialQualifier: undefined,
      maps: [],
    };
  }
  
  _beginIncfgGamepad(gpix, devp) {
    this.incfg = {
      state: FMN.INCFG_STATE_READY,
      devp,
      btnid: InputManager.INPUT_LEFT,
    };
    this.incfgPrivate = {
      gamepad: this.gamepads[gpix],
      srcbtnid: 0,
      qualifier: undefined,
      initialSrcbtnid: 0,
      initialQualifier: undefined,
      maps: [],
    };
  }
  
  /* (qualifier) is one of ["button","axislo","axishi","hatn","hate","hats","hatw"] for gamepads only.
   */
  _advanceIncfg(btnid, qualifier) {
    if (this.incfgPrivate.srcbtnid || this.incfgPrivate.qualifier) return;
    switch (this.incfg.state) {
      case FMN.INCFG_STATE_READY:
      case FMN.INCFG_STATE_FAULT: {
          this.incfgPrivate.srcbtnid = btnid;
          this.incfgPrivate.qualifier = qualifier;
          this.incfgPrivate.initialSrcbtnid = btnid;
          this.incfgPrivate.initialQualifier = qualifier;
        } break;
      case FMN.INCFG_STATE_CONFIRM: {
          this.incfgPrivate.srcbtnid = btnid;
          this.incfgPrivate.qualifier = qualifier;
        } break;
    }
  }
  
  _releaseIncfg(btnid, qualifier) {
    if ((btnid !== this.incfgPrivate.srcbtnid) || (qualifier !== this.incfgPrivate.qualifier)) return;
    switch (this.incfg.state) {
      case FMN.INCFG_STATE_READY:
      case FMN.INCFG_STATE_FAULT: {
          this.incfg.state = FMN.INCFG_STATE_CONFIRM;
          this.incfgPrivate.initialSrcbtnid = btnid;
          this.incfgPrivate.initialQualifier = qualifier;
        } break;
      case FMN.INCFG_STATE_CONFIRM: {
          if (
            (this.incfgPrivate.srcbtnid === this.incfgPrivate.initialSrcbtnid) &&
            (this.incfgPrivate.qualifier === this.incfgPrivate.initialQualifier)
          ) {
            this.incfgPrivate.maps.push([btnid, qualifier, this.incfg.btnid]);
            this.incfg.state = FMN.INCFG_STATE_READY;
            this.incfg.btnid <<= 1;
            if (this.incfg.btnid > InputManager.INPUT_MENU) {
              this._commitIncfg();
              this.cancelInputConfiguration();
            }
          } else {
            this.incfg.state = FMN.INCFG_STATE_FAULT;
          }
          this.incfgPrivate.initialSrcbtnid = 0;
          this.incfgPrivate.initialQualifier = undefined;
        } break;
    }
    this.incfgPrivate.srcbtnid = 0;
    this.incfgPrivate.qualifier = undefined;
  }
  
  _commitIncfg() {
    
    if (this.incfgPrivate.gamepad) {
      // Gamepad.
      // !!! Axes can only map to a LEFT/RIGHT or UP/DOWN pair, and hats only to the dpad !!!
      // Unfortunately, we don't present interactive config that way. So we might disobey the user's intention a bit here.
      // This will be a problem eg for axes that report upside-down.
      const newMaps = this.gamepadMaps.filter(m => m.id !== this.incfgPrivate.gamepad.id);
      const newMap = {
        id: this.incfgPrivate.gamepad.id,
        axes: this.incfgPrivate.gamepad.axes.map(v => ""),
        buttons: this.incfgPrivate.gamepad.buttons.map(v => 0),
      };
      for (const [srcbtnid, qualifier, dstbtnid] of this.incfgPrivate.maps) {
        switch (qualifier) {
          case "button": {
              if ((srcbtnid >= 0) && (srcbtnid < newMap.buttons.length)) {
                newMap.buttons[srcbtnid] = dstbtnid;
              }
            } break;
          case "axislo":
          case "axishi": {
              if ((srcbtnid >= 0) && (srcbtnid < newMap.axes.length)) {
                switch (dstbtnid) {
                  case InputManager.INPUT_LEFT:
                  case InputManager.INPUT_RIGHT: newMap.axes[srcbtnid] = "x"; break;
                  case InputManager.INPUT_UP:
                  case InputManager.INPUT_DOWN: newMap.axes[srcbtnid] = "y"; break;
                }
              }
            } break;
          case "hatn":
          case "hate":
          case "hats":
          case "hatw": {
              if ((srcbtnid >= 0) && (srcbtnid < newMap.axes.length)) {
                switch (dstbtnid) {
                  case InputManager.INPUT_LEFT:
                  case InputManager.INPUT_RIGHT:
                  case InputManager.INPUT_UP:
                  case InputManager.INPUT_DOWN: newMap.axes[srcbtnid] = "hat"; break;
                }
              }
            } break;
        }
      }
      newMaps.push(newMap);
      this.gamepadMaps = newMaps;
      
      // Gamepads keep their own copy of the mapping, in addition to the state. Rewrite those.
      this.incfgPrivate.gamepad.axes = [...newMap.axes];
      this.incfgPrivate.gamepad.buttons = [...newMap.buttons];
      this.incfgPrivate.gamepad.axesState = newMap.axes.map(m => 0);
      this.incfgPrivate.gamepad.buttonsState = newMap.buttons.map(m => false);
    
    } else {
      // Keyboard.
      this.keyMaps = this.incfgPrivate.maps.map(src => ({
        code: src[0],
        btnid: src[2],
      }));
      
    }
    this.dirty();
  }
}

InputManager.singleton = true;

InputManager.INPUT_LEFT  = 0x01;
InputManager.INPUT_RIGHT = 0x02;
InputManager.INPUT_UP    = 0x04;
InputManager.INPUT_DOWN  = 0x08;
InputManager.INPUT_USE   = 0x10;
InputManager.INPUT_MENU  = 0x20;
