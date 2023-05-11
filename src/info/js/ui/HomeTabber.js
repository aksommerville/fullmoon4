/* HomeTabber.js
 * Top level of UI hierarchy.
 * Static header and footer are not included.
 */
 
import { Dom } from "../util/Dom.js";
import { NewsUi } from "./NewsUi.js";
import { PlayUi } from "./PlayUi.js";
import { LoreUi } from "./LoreUi.js";
import { HintsUi } from "./HintsUi.js";
import { LinksUi } from "./LinksUi.js";

export class HomeTabber {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const tabBar = this.dom.spawn(this.element, "DIV", ["tabBar"], { "on-click": e => this.onTabClick(e) });
    const panelContainer = this.dom.spawn(this.element, "DIV", ["panelContainer"]);
    
    this.addTab(tabBar, panelContainer, "News", NewsUi);
    this.addTab(tabBar, panelContainer, "Play", PlayUi);
    this.addTab(tabBar, panelContainer, "Lore", LoreUi);
    this.addTab(tabBar, panelContainer, "Hints", HintsUi);
    this.addTab(tabBar, panelContainer, "Links", LinksUi);
    
    this.selectTab("Play");
  }
  
  addTab(tabBar, panelContainer, name, clazz) {
    const tab = this.dom.spawn(tabBar, "DIV", ["tab"], name, { "data-name": name });
    const panel = this.dom.spawnController(panelContainer, clazz);
    panel.element.classList.add("panel");
    panel.element.setAttribute("data-name", name);
  }
  
  selectTab(name) {
    for (const element of this.element.querySelectorAll(".tab.selected")) element.classList.remove("selected");
    for (const element of this.element.querySelectorAll(".panel.selected")) element.classList.remove("selected");
    const tab = this.element.querySelector(`.tabBar > .tab[data-name='${name}']`);
    const panel = this.element.querySelector(`.panelContainer > .panel[data-name='${name}']`);
    if (tab) {
      tab.classList.add("selected");
    }
    if (panel) {
      panel.classList.add("selected");
      if (panel._fmn_controller && panel._fmn_controller.onShowTab) panel._fmn_controller.onShowTab();
    }
  }
  
  onTabClick(event) {
    const name = event.target.getAttribute("data-name");
    if (!name) return;
    this.selectTab(name);
  }
}
