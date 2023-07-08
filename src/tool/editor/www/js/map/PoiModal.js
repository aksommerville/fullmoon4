/* PoiModal.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { FullmoonMap } from "/js/map/FullmoonMap.js";
import { MapService } from "./MapService.js";
import { ChalkModal } from "../chalk/ChalkModal.js";

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
    this.dom.spawn(typeSelect, "OPTION", { value: "transmogrify" }, "transmogrify");
    this.dom.spawn(typeSelect, "OPTION", { value: "sketch" }, "sketch");
    this.dom.spawn(typeSelect, "OPTION", { value: "buried_treasure" }, "buried_treasure");
    this.dom.spawn(typeSelect, "OPTION", { value: "buried_door" }, "buried_door");
    this.dom.spawn(typeSelect, "OPTION", { value: "event_trigger" }, "event_trigger");
    
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
        poi[key] = this.evalField(key, element.value);
      }
    }
    return poi;
  }
  
  evalField(key, value) {
    const nvalue = +value;
    if (isNaN(nvalue)) return value;
    return nvalue;
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
      case "transmogrify": this.buildTransmogrifyForm(table); break;
      case "sketch": this.buildSketchForm(table); break;
      case "buried_treasure": this.buildBuriedTreasureForm(table); break;
      case "buried_door": this.buildBuriedDoorForm(table); break;
      case "event_trigger": this.buildEventTriggerForm(table); break;
    }
  }
  
  buildExitForm(table) {
    this.addNumberRow(table, "mapId", 0, 65535, this.poi ? this.poi.mapId : 0, "0 to create");
    this.addNumberRow(table, "dstx", 0, FullmoonMap.COLC - 1, this.poi ? this.poi.dstx : 0);
    this.addNumberRow(table, "dsty", 0, FullmoonMap.ROWC - 1, this.poi ? this.poi.dsty : 0);
    if (!this.poi) this.addCheckboxRow(table, "createRemote", true);
  }
  
  buildSpriteForm(table) {
    const sprite = this.poi ? this.resService.getResourceObject("sprite", this.poi.spriteId) : null;
    const idField = this.addTextRow(table, "spriteId", this.poi ? this.reprSpriteId(this.poi.spriteId) : "", `PoiModal-${this.discriminator}-spriteList`);
    idField.addEventListener("blur", () => this.reconsiderSpriteArgLabels());
    this.addTextRow(table, sprite ? sprite.getArgLabel(0) : "arg0", this.poi ? this.poi.argv[0] : 0, null, "arg0");
    this.addTextRow(table, sprite ? sprite.getArgLabel(1) : "arg1", this.poi ? this.poi.argv[1] : 0, null, "arg1");
    this.addTextRow(table, sprite ? sprite.getArgLabel(2) : "arg2", this.poi ? this.poi.argv[2] : 0, null, "arg2");
  }
  
  reconsiderSpriteArgLabels() {
    const spriteId = this.element.querySelector(`input[name='spriteId']`).value;
    const sprite = this.resService.getResourceObject("sprite", spriteId);
    if (!sprite) return;
    this.element.querySelector(`td.key[data-inputName='arg0']`).innerText = sprite.getArgLabel(0);
    this.element.querySelector(`td.key[data-inputName='arg1']`).innerText = sprite.getArgLabel(1);
    this.element.querySelector(`td.key[data-inputName='arg2']`).innerText = sprite.getArgLabel(2);
  }
  
  buildTransmogrifyForm(table) {
    this.addEnumRow(table, "mode", this.poi ? this.poi.mode : "toggle", ["to", "from", "toggle"]);
    this.addNumberRow(table, "state", 0, 63, this.poi ? this.poi.state : 1, "1=pumpkin");
  }
  
  buildSketchForm(table) {
    this.addNumberRow(table, "bits", 1, 0x000fffff, this.poi ? this.poi.bits : 0);
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", "Edit in ChalkModal");
    const td = this.dom.spawn(tr, "TD");
    this.dom.spawn(td, "INPUT", { type: "button", value: "Open", "on-click": () => this.onEditChalk() });
  }
  
  buildBuriedTreasureForm(table) {
    this.addTextRow(table, "gsbit", this.poi ? this.poi.gsbit : "");
    this.addTextRow(table, "itemId", this.poi ? this.poi.itemId : "");
  }
  
  buildBuriedDoorForm(table) {
    this.addTextRow(table, "gsbit", this.poi ? this.poi.gsbit : "");
    this.addNumberRow(table, "mapId", 0, 65535, this.poi ? this.poi.mapId : 0, "0 to create");
    this.addNumberRow(table, "dstx", 0, FullmoonMap.COLC - 1, this.poi ? this.poi.dstx : 0);
    this.addNumberRow(table, "dsty", 0, FullmoonMap.ROWC - 1, this.poi ? this.poi.dsty : 0);
    if (!this.poi) this.addCheckboxRow(table, "createRemote", true);
  }
  
  buildEventTriggerForm(table) {
    this.addNumberRow(table, "eventid", 0, 65535, this.poi ? this.poi.eventid : 0);
  }
  
  reprSpriteId(id) {
    const res = this.resService.toc.find(r => r.type === "sprite" && r.id === id);
    if (res && res.name) return res.name;
    return id.toString();
  }
  
  addTextRow(table, key, value, dataListId, inputName) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], key, { "data-inputName": inputName || key });
    const td = this.dom.spawn(tr, "TD", ["value"]);
    const input = this.dom.spawn(td, "INPUT", {
      type: "text",
      name: inputName || key,
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
  
  addCheckboxRow(table, key, checked) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], key);
    const td = this.dom.spawn(tr, "TD", ["value"]);
    const input = this.dom.spawn(td, "INPUT", {
      type: "checkbox",
      name: key,
      checked,
    });
    return input;
  }
  
  addEnumRow(table, key, value, options) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], key);
    const td = this.dom.spawn(tr, "TD", ["value"]);
    const select = this.dom.spawn(td, "SELECT", {
      name: key,
    });
    for (const option of options) this.dom.spawn(select, "OPTION", { value: option }, option);
    select.value = value;
    return select;
  }
  
  onEditChalk() {
    const modal = this.dom.spawnModal(ChalkModal);
    modal.setup(0, this.poi ? this.poi.bits : 0);
    modal.onDirty = (codepoint, bits) => {
      this.element.querySelector("input[name='bits']").value = bits;
    };
  }
  
  onSubmit() {
    const poi = this.readPoiFromDom();
    const command = this.commandFromPoiModel(poi);
    if (!command) return;
    
    if ((command[0] === "door") && (+command[3] === 0)) {
      const mapId = this.mapService.createMap(this.resService);
      command[3] = mapId.toString();
      poi.mapId = mapId;
    } else if ((command[0] === "buried_door") && (+command[4] === 0)) {
      const mapId = this.mapService.createMap(this.resService);
      command[4] = mapId.toString();
      poi.mapId = mapId;
    }
    
    if (poi.createRemote) {
      let remoteCommand;
      switch (command[0]) {
        case "door": remoteCommand = ["door", command[4], command[5], this.map.id.toString(), command[1], command[2]]; break;
        case "buried_door": remoteCommand = ["door", command[5], command[6], this.map.id.toString(), command[1], command[2]]; break;
      }
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
          let spriteId = +poi.spriteId;
          if (isNaN(spriteId) || (spriteId < 1) || (spriteId > 0xffff)) spriteId = poi.spriteId;
          //if (isNaN(poi.arg0) || (poi.arg0 < 0) || (poi.arg0 > 0xff)) return null;
          //if (isNaN(poi.arg1) || (poi.arg1 < 0) || (poi.arg1 > 0xff)) return null;
          //if (isNaN(poi.arg2) || (poi.arg2 < 0) || (poi.arg2 > 0xff)) return null;
          return ["sprite", poi.x.toString(), poi.y.toString(), spriteId, poi.arg0.toString(), poi.arg1.toString(), poi.arg2.toString()];
        }
        
      case "hero": {
          return ["hero", poi.x.toString(), poi.y.toString()];
        }
        
      case "transmogrify": {
          return ["transmogrify", poi.x.toString(), poi.y.toString(), poi.mode, poi.state.toString()];
        }
        
      case "sketch": {
          return ["sketch", poi.x.toString(), poi.y.toString(), poi.bits.toString()];
        }
        
      case "buried_treasure": {
          return ["buried_treasure", poi.x.toString(), poi.y.toString(), poi.gsbit.toString(), poi.itemId.toString()];
        }
        
      case "buried_door": {
          return ["buried_door", poi.x.toString(), poi.y.toString(), poi.gsbit.toString(), poi.mapId.toString(), poi.dstx.toString(), poi.dsty.toString()];
        }
        
      case "event_trigger": {
          return ["event_trigger", poi.x.toString(), poi.y.toString(), poi.eventid.toString()];
        }
    }
    return null;
  }
}
