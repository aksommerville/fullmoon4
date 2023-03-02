/* PoiModal.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { FullmoonMap } from "/js/map/FullmoonMap.js";
import { MapService } from "./MapService.js";

export class PoiModal {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, "discriminator", MapService];
  }
  constructor(element, dom, resService, discriminator, mapService) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.discriminator = discriminator;
    this.mapService = mapService;
    
    // Owner should set:
    this.onDirty = (command) => {};
    this.onCreateRemoteCommand = (mapId, command) => false;
    
    this.map = null;
    this.x = 0;
    this.y = 0;
    this.poi = null;
    
    this.buildUi();
  }
  
  setupNew(map, x, y) {
    this.map = map;
    this.x = x;
    this.y = y;
    this.poi = null;
    this.populateUi();
  }
  
  setupEdit(map, poi) {
    this.map = map;
    this.x = poi.x;
    this.y = poi.y;
    this.poi = poi;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const permanentForm = this.dom.spawn(this.element, "FORM", ["permanentForm"]);
    
    const typeSelect = this.dom.spawn(permanentForm, "SELECT", { name: "type", "on-change": () => this.onTypeChanged() });
    this.dom.spawn(typeSelect, "OPTION", { value: "exit" }, "door");
    // Can't select "entrance" type this way. You have to create doors from their exit map, where they live ultimately.
    this.dom.spawn(typeSelect, "OPTION", { value: "sprite" }, "sprite");
    this.dom.spawn(typeSelect, "OPTION", { value: "hero" }, "hero");
    
    this.dom.spawn(permanentForm, "INPUT", {
      type: "number",
      name: "x",
      min: 0,
      max: FullmoonMap.COLC - 1,
    });
    this.dom.spawn(permanentForm, "INPUT", {
      type: "number",
      name: "y",
      min: 0,
      max: FullmoonMap.ROWC - 1,
    });
    
    const transientForm = this.dom.spawn(this.element, "TABLE", ["transientForm"]);
    
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "OK", "on-click": () => this.onSubmit() });
    
    // No provision for updating after: I don't think it will be possible to add, remove, or rename sprites while the POI modal is open.
    const spriteList = this.dom.spawn(this.element, "DATALIST", { id: `PoiModal-${this.discriminator}-spriteList` });
    for (const res of this.resService.toc) {
      if (res.type !== "sprite") continue;
      if (!res.name) continue;
      this.dom.spawn(spriteList, "OPTION", { value: res.name });
    }
  }
  
  populateUi() {
    this.element.querySelector("select[name='type']").value = this.poi ? this.poi.type : "";
    this.element.querySelector("input[name='x']").value = this.x;
    this.element.querySelector("input[name='y']").value = this.y;
    this.onTypeChanged(); // rebuild the transient form
  }
  
  readPoiFromDom() {
    const poi = {
      type: this.element.querySelector("select[name='type']").value,
      x: +this.element.querySelector("input[name='x']").value,
      y: +this.element.querySelector("input[name='y']").value,
    };
    for (const element of this.element.querySelectorAll(".transientForm *[name]")) {
      const key = element.name;
      if (element.type === "checkbox") {
        poi[key] = element.checked;
      } else {
        //poi[key] = +element.value;
        poi[key] = this.evalField(key, element.value);
      }
    }
    return poi;
  }
  
  evalField(key, value) {
    switch (key) {
      case "spriteId": {
          if (value) {
            const res = this.resService.toc.find(r => r.type === "sprite" && r.name === value);
            if (res) return res.id;
          }
        } break;
    }
    return +value;
  }
  
  onTypeChanged() {
    const type = this.element.querySelector("select[name='type']").value;
    const transientForm = this.element.querySelector(".transientForm");
    transientForm.innerHTML = "";
    const table = this.dom.spawn(transientForm, "TABLE");
    switch (type) {
      case "exit": this.buildExitForm(table); break;
      case "sprite": this.buildSpriteForm(table); break;
      case "hero": break;
    }
  }
  
  buildExitForm(table) {
    this.addNumberRow(table, "mapId", 0, 65535, this.poi ? this.poi.mapId : 1, "0 to create");
    this.addNumberRow(table, "dstx", 0, FullmoonMap.COLC - 1, this.poi ? this.poi.dstx : 0);
    this.addNumberRow(table, "dsty", 0, FullmoonMap.ROWC - 1, this.poi ? this.poi.dsty : 0);
    if (!this.poi) this.addCheckboxRow(table, "createRemote");
  }
  
  buildSpriteForm(table) {
    //TODO Sprite metadata, get meaningful labels instead of "arg0" etc.
    this.addTextRow(table, "spriteId", this.poi ? this.reprSpriteId(this.poi.spriteId) : "", `PoiModal-${this.discriminator}-spriteList`);
    this.addNumberRow(table, "arg0", 0, 255, this.poi ? this.poi.argv[0] : 0);
    this.addNumberRow(table, "arg1", 0, 255, this.poi ? this.poi.argv[1] : 0);
    this.addNumberRow(table, "arg2", 0, 255, this.poi ? this.poi.argv[2] : 0);
  }
  
  reprSpriteId(id) {
    const res = this.resService.toc.find(r => r.type === "sprite" && r.id === id);
    if (res && res.name) return res.name;
    return id.toString();
  }
  
  addTextRow(table, key, value, dataListId) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], key);
    const td = this.dom.spawn(tr, "TD", ["value"]);
    const input = this.dom.spawn(td, "INPUT", {
      type: "text",
      name: key,
      value,
    });
    if (dataListId) input.setAttribute("list", dataListId);
    return input;
  }
  
  addNumberRow(table, key, lo, hi, value, comment) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], key);
    const td = this.dom.spawn(tr, "TD", ["value"]);
    const input = this.dom.spawn(td, "INPUT", {
      type: "number",
      name: key,
      min: lo,
      max: hi,
      value,
    });
    if (comment) this.dom.spawn(tr, "TD", ["comment"], comment);
    return input;
  }
  
  addCheckboxRow(table, key) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], key);
    const td = this.dom.spawn(tr, "TD", ["value"]);
    const input = this.dom.spawn(td, "INPUT", {
      type: "checkbox",
      name: key,
    });
    return input;
  }
  
  onSubmit() {
    const poi = this.readPoiFromDom();
    const command = this.commandFromPoiModel(poi);
    if (!command) return;
    
    if ((command[0] === "door") && (command[3] === "0")) {
      const mapId = this.mapService.createMap(this.resService);
      command[3] = mapId.toString();
      poi.mapId = mapId;
    }
    
    if (poi.createRemote) {
      const remoteCommand = ["door", command[4], command[5], this.map.id.toString(), command[1], command[2]];
      if (!this.onCreateRemoteCommand(poi.mapId, remoteCommand)) return;
    }
    
    this.onDirty(command);
    this.dom.popModal(this);
  }
  
  /* (poi) is our format, and close to what MapRenderer uses for display. But not exactly.
   * We don't want those models to travel very far.
   * So as part of the validation process here, convert our POI into a Command ready to add to the map.
   */
  commandFromPoiModel(poi) {
    if (isNaN(poi.x) || (poi.x < 0) || (poi.x >= FullmoonMap.COLC)) return null;
    if (isNaN(poi.y) || (poi.y < 0) || (poi.y >= FullmoonMap.ROWC)) return null;
    switch (poi.type) {
    
      case "exit": {
          if (isNaN(poi.mapId) || (poi.mapId < 0) || (poi.mapId > 0xffff)) return null;
          if (isNaN(poi.dstx) || (poi.dstx < 0) || (poi.dstx >= FullmoonMap.COLC)) return null;
          if (isNaN(poi.dsty) || (poi.dsty < 0) || (poi.dsty >= FullmoonMap.COLC)) return null;
          return ["door", poi.x.toString(), poi.y.toString(), poi.mapId.toString(), poi.dstx.toString(), poi.dsty.toString()];
        }
      
      case "sprite": {
          if (isNaN(poi.spriteId) || (poi.spriteId < 1) || (poi.spriteId > 0xffff)) return null;
          if (isNaN(poi.arg0) || (poi.arg0 < 0) || (poi.arg0 > 0xff)) return null;
          if (isNaN(poi.arg1) || (poi.arg1 < 0) || (poi.arg1 > 0xff)) return null;
          if (isNaN(poi.arg2) || (poi.arg2 < 0) || (poi.arg2 > 0xff)) return null;
          return ["sprite", poi.x.toString(), poi.y.toString(), poi.spriteId.toString(), poi.arg0.toString(), poi.arg1.toString(), poi.arg2.toString()];
        }
        
      case "hero": {
          return ["hero", poi.x.toString(), poi.y.toString()];
        }
    }
    return null;
  }
}
