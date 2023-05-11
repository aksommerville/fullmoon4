/* InputConfigurationModal.js
 */
 
import { Dom } from "../util/Dom.js";
import { InputManager } from "../game/InputManager.js";
import { PressAKeyModal } from "./PressAKeyModal.js";

export class InputConfigurationModal {
  static getDependencies() {
    return [HTMLElement, InputManager, Dom, Window, "discriminator"];
  }
  constructor(element, inputManager, dom, window, discriminator) {
    this.element = element;
    this.inputManager = inputManager;
    this.dom = dom;
    this.window = window;
    this.discriminator = discriminator;
    
    this.DEVICE_NAME_LIMIT = 30;
    
    this.buildUi();
    
    // It's not gamepads we care about, so much as gamepad *maps*.
    // But we do need this listener.
    this.inputListenerId = this.inputManager.listen(e => this.onInputManagerEvent(e));
    this.inputManager.simulateConnectionOfUnmappedDevices();
    
    // We will only be presented when the runtime is paused.
    // So we need to pump InputManager for updates manually.
    this.running = true;
    this.scheduleUpdate();
    
    this.keyDownListener = e => this.setKeyboardTattle(e.code);
    this.keyUpListener = e => this.setKeyboardTattle("");
    this.window.addEventListener("keydown", this.keyDownListener);
    this.window.addEventListener("keyup", this.keyUpListener);
  }
  
  scheduleUpdate() {
    this.window.requestAnimationFrame(() => {
      if (!this.running) return;
      this.inputManager.update();
      this.scheduleUpdate();
    });
  }
  
  onRemoveFromDom() {
    this.running = false;
    if (this.inputListenerId) {
      this.inputManager.unlisten(this.inputListenerId);
      this.inputListenerId = null;
    }
    if (this.keyDownListener) {
      this.window.removeEventListener("keydown", this.keyDownListener);
      this.keyDownListener = null;
    }
    if (this.keyUpListener) {
      this.window.removeEventListener("keyup", this.keyUpListener);
      this.keyUpListener = null;
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    this.keyCodeListId = `InputConfigurationModal-${this.discriminator}-keyCodeList`;
    const keyCodeList = this.dom.spawn(this.element, "DATALIST", { id: this.keyCodeListId });
    for (const code of InputConfigurationModal.KNOWN_KEY_CODE_NAMES) {
      this.dom.spawn(keyCodeList, "OPTION", { value: code });
    }
    
    const deviceList = this.dom.spawn(this.element, "DIV", ["deviceList"]);
    const detailsPanel = this.dom.spawn(this.element, "DIV", ["detailsPanel"], { "on-change": () => this.onChange() });
    this.addDevice({ id: "Keyboard" });
    this.addDevice({ id: "Touch" });
    for (const map of this.inputManager.gamepadMaps) {
      this.addDevice(map);
    }
  }
  
  populateDetailsForDevice(device) {
    const panel = this.element.querySelector(".detailsPanel");
    panel.innerHTML = "";
    switch (device.id) {
      case "Keyboard": this.populateDetailsForKeyboard(panel); break;
      case "Touch": this.populateDetailsForTouch(panel); break;
      default: this.populateDetailsForGamepad(panel, device); break;
    }
  }
  
  populateDetailsForKeyboard(panel) {
    const getKeyName = (btnid) => {
      for (const entry of this.inputManager.keyMaps) {
        if (entry.btnid === btnid) return entry.code;
      }
      return "";
    };
    const spawnKeyButton = (parent, btnid) => {
      this.dom.spawn(parent, "INPUT", {
        type: "button",
        value: getKeyName(btnid),
        "data-btnid": btnid,
        "on-click": () => this.reassignKey(btnid),
      });
    };
    const container = this.dom.spawn(panel, "DIV", ["keyboard"]);
    const dpad = this.dom.spawn(container, "DIV", ["dpad"]);
    spawnKeyButton(dpad, InputManager.INPUT_UP);
    const dpadMid = this.dom.spawn(dpad, "DIV", ["dpadMid"]);
    spawnKeyButton(dpadMid, InputManager.INPUT_LEFT);
    this.dom.spawn(dpadMid, "DIV", ["input-pic-dpad"]);
    spawnKeyButton(dpadMid, InputManager.INPUT_RIGHT);
    spawnKeyButton(dpad, InputManager.INPUT_DOWN);
    const useRow = this.dom.spawn(container, "DIV", ["kRow"]);
    this.dom.spawn(useRow, "DIV", ["input-pic-use"]);
    spawnKeyButton(useRow, InputManager.INPUT_USE);
    const chooseRow = this.dom.spawn(container, "DIV", ["kRow"]);
    this.dom.spawn(chooseRow, "DIV", ["input-pic-choose"]);
    spawnKeyButton(chooseRow, InputManager.INPUT_MENU);
  }
  
  addKeyMapEntry(mapContainer, code, btnid) {
    const entryContainer = this.dom.spawn(mapContainer, "DIV", ["mapEntry"]);
    this.dom.spawn(entryContainer, "INPUT", {
      type: "button",
      value: "X",
      "on-click": () => {
        this.onDeleteKeyMap(code, btnid);
        entryContainer.remove();
      },
    });
    this.dom.spawn(entryContainer, "INPUT", { type: "text", value: code, name: "code", list: this.keyCodeListId });
    const select = this.dom.spawn(entryContainer, "SELECT", { name: "btnid" });
    this.dom.spawn(select, "OPTION", { value: 0 }, "(none)");
    this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_LEFT }, "Left");
    this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_RIGHT }, "Right");
    this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_UP }, "Up");
    this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_DOWN }, "Down");
    this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_USE }, "Use");
    this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_MENU }, "Choose");
    select.value = btnid;
  }
  
  updateKeyboardMapFromUi() {
    const newMaps = [];
    for (const entryContainer of this.element.querySelectorAll(".mapContainer[data-deviceId='Keyboard'] .mapEntry")) {
      const code = entryContainer.querySelector("*[name='code']").value;
      const btnid = +entryContainer.querySelector("*[name='btnid']").value;
      if (!code || !btnid) continue;
      newMaps.push({ code, btnid });
    }
    this.inputManager.updateKeyMaps(newMaps);
  }
  
  populateDetailsForTouch(panel) {
    panel.innerText = "TODO touch";
  }
  
  updateTouchMapFromUi() {
    //TODO
  }
  
  populateDetailsForGamepad(panel, map) {
    const device = this.inputManager.gamepads.find(g => g && g.id === map.id);
    const axisCount = Math.max(map.axes.length, device?.axes.length || 0);
    const buttonCount = Math.max(map.buttons.length, device?.buttons.length || 0);
    this.dom.spawn(panel, "H2", "Axes");
    const axesContainer = this.dom.spawn(panel, "DIV", ["mapContainer", "axes"], { "data-deviceId": map.id });
    for (let i=0; i<axisCount; i++) {
      const axisContainer = this.dom.spawn(axesContainer, "DIV", ["mapEntry"], { "data-index": i });
      this.dom.spawn(axisContainer, "DIV", ["index"], i);
      const select = this.dom.spawn(axisContainer, "SELECT");
      this.dom.spawn(select, "OPTION", { value: 0 }, "(none)");
      this.dom.spawn(select, "OPTION", { value: "x" }, "Horz");
      this.dom.spawn(select, "OPTION", { value: "y" }, "Vert");
      this.dom.spawn(select, "OPTION", { value: "hat" }, "Hat");
      select.value = map.axes[i];
    }
    this.dom.spawn(panel, "H2", "Buttons");
    const buttonsContainer = this.dom.spawn(panel, "DIV", ["mapContainer", "buttons"], { "data-deviceId": map.id });
    for (let i=0; i<buttonCount; i++) {
      const buttonContainer = this.dom.spawn(buttonsContainer, "DIV", ["mapEntry"], { "data-index": i });
      this.dom.spawn(buttonContainer, "DIV", ["index"], i);
      const select = this.dom.spawn(buttonContainer, "SELECT");
      this.dom.spawn(select, "OPTION", { value: 0 }, "(none)");
      this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_LEFT }, "Left");
      this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_RIGHT }, "Right");
      this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_UP }, "Up");
      this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_DOWN }, "Down");
      this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_USE }, "Use");
      this.dom.spawn(select, "OPTION", { value: InputManager.INPUT_MENU }, "Choose");
      select.value = map.buttons[i];
    }
  }
  
  updateGamepadMapFromUi(deviceId) {
    const map = { id: deviceId, axes: [], buttons: [] };
    for (const input of this.element.querySelectorAll(".mapContainer.axes .mapEntry")) {
      const index = +input.getAttribute("data-index");
      map.axes[index] = input.querySelector("select").value;
    }
    for (const input of this.element.querySelectorAll(".mapContainer.buttons .mapEntry")) {
      const index = +input.getAttribute("data-index");
      map.buttons[index] = +input.querySelector("select").value;
    }
    this.inputManager.updateGamepadMap(map);
  }
  
  onInputManagerEvent(e) {
    switch (e.type) {
      case "gamepadconnected": {
          // If no map exists for it, add to the device list. (otherwise we already have it).
          if (!this.inputManager.gamepadMaps.find(m => m.id === e.gamepad.id)) {
            this.addDevice({
              id: e.gamepad.id,
              axes: e.gamepad.axes.map(() => ""),
              buttons: e.gamepad.buttons.map(() => 0),
            });
          }
        } break;
      case "axis": {
          const element = this.element.querySelector(`.mapContainer.axes[data-deviceId='${e.device.id}'] .mapEntry[data-index='${e.axisIndex}']`);
          if (element) {
            if (e.v) element.classList.add("active");
            else element.classList.remove("active");
          }
        } break;
      case "button": {
          const element = this.element.querySelector(`.mapContainer.buttons[data-deviceId='${e.device.id}'] .mapEntry[data-index='${e.buttonIndex}']`);
          if (element) {
            if (e.v) element.classList.add("active");
            else element.classList.remove("active");
          }
        } break;
    }
  }
  
  addDevice(device) {
    const deviceList = this.element.querySelector(".deviceList");
    const listEntry = this.dom.spawn(deviceList, "DIV", ["entry"], this.reprDevice(device), {
      "data-deviceId": device.id,
      "on-click": () => this.selectDevice(device),
    });
  }
  
  // (device) could be a fake thing we make up for Keyboard and Touch, or a Gamepad map. (not necessary belonging to InputManager).
  reprDevice(device) {
    if (device.id) {
      if (device.id.length > this.DEVICE_NAME_LIMIT) {
        return device.id.substring(0, this.DEVICE_NAME_LIMIT) + "...";
      }
      return device.id;
    }
    return "Device";
  }
  
  selectDevice(device) {
    for (const element of this.element.querySelectorAll(".deviceList .entry.selected")) element.classList.remove("selected");
    const entry = this.element.querySelector(`.deviceList .entry[data-deviceId='${device.id}']`);
    entry.classList.add("selected");
    this.populateDetailsForDevice(device);
  }
  
  onDeleteKeyMap(code, btnid) {
    const p = this.inputManager.keyMaps.findIndex(m => m.code === code && m.btnid === btnid);
    if (p >= 0) {
      const newMaps = [...this.inputManager.keyMaps.slice(0, p), ...this.inputManager.keyMaps.slice(p + 1)];
      this.inputManager.updateKeyMaps(newMaps);
    }
  }
  
  onAddKeyMap() {
    const mapContainer = this.element.querySelector(".mapContainer[data-deviceId='Keyboard']");
    if (!mapContainer) return;
    this.addKeyMapEntry(mapContainer, "", 0);
  }
  
  setKeyboardTattle(src) {
    const tattle = this.element.querySelector(".tattle");
    if (!tattle) return;
    tattle.innerText = src;
  }
  
  onChange() {
    const selectedElement = this.element.querySelector(".deviceList .entry.selected");
    if (!selectedElement) return;
    const deviceId = selectedElement.getAttribute("data-deviceId");
    if (!deviceId) return;
    switch (deviceId) {
      case "Keyboard": this.updateKeyboardMapFromUi(); break;
      case "Touch": this.updateTouchMapFromUi(); break;
      default: this.updateGamepadMapFromUi(deviceId); break;
    }
  }
  
  reassignKey(btnid) {
    const modal = this.dom.spawnModal(PressAKeyModal);
    modal.setBtnid(btnid);
    modal.onchoose = (code) => {
      const newMaps = [...this.inputManager.keyMaps];
      const p = newMaps.findIndex(m => m.btnid === btnid);
      if (p >= 0) newMaps[p] = { code, btnid };
      else newMaps.push({ code, btnid });
      this.inputManager.updateKeyMaps(newMaps);
      
      const input = this.element.querySelector(`input[data-btnid='${btnid}']`);
      if (input) input.value = code;
    };
  }
}

// Scraped from MDN: https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
// console.log([...new Set(Array.from(document.querySelectorAll("code")).map(e=>e.innerText).filter(s=>s.match(/^"[0-9A-Za-z_]+"$/)))].join(",\n  "))
// There doesn't appear to be an authoritative source, not that I looked very hard.
InputConfigurationModal.KNOWN_KEY_CODE_NAMES = [
  "Escape",
  "Digit1",
  "Digit2",
  "Digit3",
  "Digit4",
  "Digit5",
  "Digit6",
  "Digit7",
  "Digit8",
  "Digit9",
  "Digit0",
  "Minus",
  "Equal",
  "Backspace",
  "Tab",
  "KeyQ",
  "KeyW",
  "KeyE",
  "KeyR",
  "KeyT",
  "KeyY",
  "KeyU",
  "KeyI",
  "KeyO",
  "KeyP",
  "BracketLeft",
  "BracketRight",
  "Enter",
  "ControlLeft",
  "KeyA",
  "KeyS",
  "KeyD",
  "KeyF",
  "KeyG",
  "KeyH",
  "KeyJ",
  "KeyK",
  "KeyL",
  "Semicolon",
  "Quote",
  "Backquote",
  "ShiftLeft",
  "Backslash",
  "KeyZ",
  "KeyX",
  "KeyC",
  "KeyV",
  "KeyB",
  "KeyN",
  "KeyM",
  "Comma",
  "Period",
  "Slash",
  "ShiftRight",
  "NumpadMultiply",
  "AltLeft",
  "Space",
  "CapsLock",
  "F1",
  "F2",
  "F3",
  "F4",
  "F5",
  "F6",
  "F7",
  "F8",
  "F9",
  "F10",
  "Pause",
  "ScrollLock",
  "Numpad7",
  "Numpad8",
  "Numpad9",
  "NumpadSubtract",
  "Numpad4",
  "Numpad5",
  "Numpad6",
  "NumpadAdd",
  "Numpad1",
  "Numpad2",
  "Numpad3",
  "Numpad0",
  "NumpadDecimal",
  "PrintScreen",
  "IntlBackslash",
  "F11",
  "F12",
  "NumpadEqual",
  "F13",
  "F14",
  "F15",
  "F16",
  "F17",
  "F18",
  "F19",
  "F20",
  "F21",
  "F22",
  "F23",
  "F24",
  "KanaMode",
  "Lang2",
  "Lang1",
  "IntlRo",
  "Lang4",
  "Lang3",
  "Convert",
  "NonConvert",
  "IntlYen",
  "NumpadComma",
  "Undo",
  "Paste",
  "MediaTrackPrevious",
  "Cut",
  "Copy",
  "MediaTrackNext",
  "NumpadEnter",
  "ControlRight",
  "LaunchMail",
  "AudioVolumeMute",
  "LaunchApp2",
  "MediaPlayPause",
  "MediaStop",
  "Eject",
  "VolumeDown",
  "AudioVolumeDown",
  "VolumeUp",
  "AudioVolumeUp",
  "BrowserHome",
  "NumpadDivide",
  "AltRight",
  "Help",
  "NumLock",
  "Home",
  "ArrowUp",
  "PageUp",
  "ArrowLeft",
  "ArrowRight",
  "End",
  "ArrowDown",
  "PageDown",
  "Insert",
  "Delete",
  "OSLeft",
  "MetaLeft",
  "OSRight",
  "MetaRight",
  "ContextMenu",
  "Power",
  "Sleep",
  "WakeUp",
  "BrowserSearch",
  "BrowserFavorites",
  "BrowserRefresh",
  "BrowserStop",
  "BrowserForward",
  "BrowserBack",
  "LaunchApp1",
  "MediaSelect",
  "Fn",
  "VolumeMute",
  "Lang5",
  "Abort",
  "Again",
  "Props",
  "Select",
  "Open",
  "Find",
  "NumpadParenLeft",
  "NumpadParenRight",
  "BrightnessUp",
];
