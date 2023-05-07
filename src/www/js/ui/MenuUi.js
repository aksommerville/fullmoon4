/* MenuUi.js
 * Hideable menu, appears when you click anywhere.
 */
 
import { Dom } from "../util/Dom.js";
import { MenuOptionsUi } from "./MenuOptionsUi.js";
import { Preferences } from "../game/Preferences.js";

export class MenuUi {
  static getDependencies() {
    return [HTMLElement, Dom, Preferences];
  }
  constructor(element, dom, preferences) {
    this.element = element;
    this.dom = dom;
    this.preferences = preferences;
    
    this.visible = false;
    this.menuOptionsUi = null;
    this.onfullscreen = null;
    this.ondismiss = null;
    
    this.buildUi();
    
    this.preferencesListener = this.preferences.fetchAndListen(p => this.onPrefsChangedUpstream(p));
  }
  
  onRemoveFromDom() {
    this.preferences.unlisten(this.preferencesListener);
  }
  
  show(show) {
    if (show) {
      if (this.visible) return;
      this.visible = true;
      this.element.classList.add("visible");
    } else {
      if (!this.visible) return;
      this.visible = false;
      this.element.classList.remove("visible");
    }
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const pop = this.dom.spawn(this.element, "DIV", ["popup"], { "on-click": e => e.stopPropagation() });
    const header = this.dom.spawn(pop, "H2", "Full Moon: Settings");
    this.dom.spawn(header, "INPUT", { type: "button", value: "OK", "on-click": () => this.ondismiss?.() });
    this.menuOptionsUi = this.dom.spawnController(pop, MenuOptionsUi);
    this.menuOptionsUi.onchange = p => this.onPrefsChangedDownstream(p);
    this.menuOptionsUi.onfullscreen = () => this.onfullscreen?.();
    const footer = this.dom.spawn(pop, "DIV", ["footer"]);
    this.dom.spawn(footer, "DIV", "\u00a9 2023 AK Sommerville");
    this.dom.spawn(footer, "A", { href: "https://aksommerville.itch.io/full-moon", target: "_top" }, "Itch.io");
    this.dom.spawn(footer, "A", { href: "https://aksommerville.com/fullmoon", target: "_top" }, "Home");
    this.dom.spawn(footer, "A", { href: "https://github.com/aksommerville/fullmoon4", target: "_top" }, "Github");
    this.dom.spawn(footer, "DIV", ["spacer"]);
  }
  
  /* Events.
   ***********************************************/
   
  onPrefsChangedUpstream(p) {
    this.menuOptionsUi.populateUi(p);
  }
  
  onPrefsChangedDownstream(p) {
    this.preferences.update(p);
  }
}
