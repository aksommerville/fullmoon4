/* MenuOptionsUi.js
 * Child of MenuUi, just the real controls.
 */
 
import { Dom } from "../util/Dom.js";
import { InputConfigurationModal } from "./InputConfigurationModal.js";

export class MenuOptionsUi {
  static getDependencies() {
    return [HTMLElement, Dom, "discriminator"];
  }
  constructor(element, dom, discriminator) {
    this.element = element;
    this.dom = dom;
    this.discriminator = discriminator;
    
    this.onchange = (prefs) => {}; // Parent should replace.
    this.onfullscreen = () => {};
    
    this.buildUi();
    this.element.addEventListener("change", (e) => this.onChange(e));
  }
  
  /* UI.
   ***************************************************/
  
  buildUi() {
    this.element.innerHTML = "";
    const table = this.dom.spawn(this.element, "TABLE");
    this.addRowRadio(table, "Music", ["On", "Off"]);
    this.addRowRadio(table, "Scaler", ["Pixelly", "Blurry"]);
    this.addRowButtons(table, "Video", [
      { label: "Fullscreen", cb: () => this.onfullscreen() },
    ]);
    this.addRowButtons(table, "Input", [
      { label: "Configure", cb: () => this.onConfigureInput() },
    ]);
  }
  
  addRowRadio(table, label, options) {
    const tr = this.dom.spawn(table, "TR");
    const tdk = this.dom.spawn(tr, "TD", ["key"], label);
    const tdv = this.dom.spawn(tr, "TD", ["value"]);
    for (const option of options) {
      const id = `MenuOptionsUi-${this.discriminator}-${label}-${option}`;
      this.dom.spawn(tdv, "INPUT", { id, type: "radio", name: label, value: option });
      this.dom.spawn(tdv, "LABEL", { for: id }, option);
    }
  }
  
  addRowButtons(table, label, buttons) {
    const tr = this.dom.spawn(table, "TR");
    const tdk = this.dom.spawn(tr, "TD", ["key"], label);
    const tdv = this.dom.spawn(tr, "TD", ["value"]);
    for (const button of buttons) {
      this.dom.spawn(tdv, "INPUT", { type: "button", value: button.label, "on-click": button.cb });
    }
  }
  
  /* Preferences model.
   * Schema: game/Preferences.js
   ***********************************************/
  
  populateUi(model) {
    this.checkRadio("Music", "On", model.musicEnable);
    this.checkRadio("Music", "Off", !model.musicEnable);
    this.checkRadio("Scaler", "Pixelly", model.scaling === "nearest");
    this.checkRadio("Scaler", "Blurry", model.scaling === "linear");
  }
  
  checkRadio(name, value, checked) {
    const element = this.element.querySelector(`input[name='${name}'][value='${value}']`);
    if (!element) return;
    element.checked = checked;
  }
   
  readUi() {
    return {
      musicEnable: this._readMusicEnable(),
      scaling: this._readScaling(),
    };
  }
  
  _readMusicEnable() {
    return !!this.element.querySelector("input[name='Music'][value='On']")?.checked;
  }
  
  _readScaling() {
    const element = this.element.querySelector("input[name='Scaler']:checked");
    switch (element?.value) {
      case "Pixelly": return "nearest";
      case "Blurry": return "linear";
    }
    return "nearest";
  }
  
  /* Events.
   ******************************************/
   
  onConfigureInput() {
    this.dom.spawnModal(InputConfigurationModal);
  }
  
  onChange(e) {
    this.onchange(this.readUi());
  }
}
