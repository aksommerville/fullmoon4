/* SpriteAllUi.js
 * List of sprite resources.
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { SpriteService } from "./SpriteService.js";

export class SpriteAllUi {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, SpriteService, Window];
  }
  constructor(element, dom, resService, spriteService, window) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.spriteService = spriteService;
    this.window = window;
    
    this.buildUi();
    
    this.resService.whenLoaded().then(() => this.populateUi());
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    this.dom.spawn(this.element, "INPUT", {
      type: "button",
      value: "New",
      "on-click": () => this.onNewSprite(),
    });
    
    const table = this.dom.spawn(this.element, "TABLE", ["mainTable"], { "on-click": (e) => this.onClickTable(e) });
    const headerRow = this.dom.spawn(table, "TR");
    this.dom.spawn(headerRow, "TH", "ID");
    this.dom.spawn(headerRow, "TH", "Name");
    this.dom.spawn(headerRow, "TH", "Controller");
    this.dom.spawn(headerRow, "TH", "Preview");
  }
  
  populateUi() {
    const mainTable = this.element.querySelector(".mainTable");
    for (const tr of mainTable.querySelectorAll("tr.sprite")) tr.remove();
    for (const res of this.resService.toc) {
      if (res.type !== "sprite") continue;
      const tr = this.dom.spawn(mainTable, "TR", ["sprite"], { "data-spriteId": res.id });
      this.dom.spawn(tr, "TD", ["id"], res.id);
      this.dom.spawn(tr, "TD", ["name"], res.name || "");//TODO it would be nice if sprites had names
      this.dom.spawn(tr, "TD", ["controller"], res.object.getCommand("controller"));
      const tdPreview = this.dom.spawn(tr, "TD", ["preview"]);
      const canvasPreview = this.dom.spawn(tdPreview, "CANVAS");
      this.spriteService.generatePreview(canvasPreview, res.object, this.resService);
    }
  }
  
  onNewSprite() {
    const response = this.window.prompt("ID or ID-NAME:", this.resService.unusedId("sprite"));
    if (!response) return;
    let [id, name] = response.split("-");
    id = +id;
    name = name || "";
    if (!id || (id < 1) || (id > 0xffff)) return;
    const sprite = this.spriteService.generateNewSprite();
    sprite.id = id;
    this.resService.dirty("sprite", id, sprite, name);
    this.window.location = `#sprite/${id}`;
  }
  
  onClickTable(e) {
    if (!e || !e.target) return;
    let tr = e.target;
    while (tr && (tr.tagName !== "TR")) tr = tr.parentNode;
    if (!tr) return;
    if (!tr.classList.contains("sprite")) return;
    const spriteId = +tr.getAttribute("data-spriteId");
    if (!spriteId) return;
    this.window.location = `#sprite/${spriteId}`;
  }
}
