/* ToolbarUi.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { MapService } from "/js/map/MapService.js";
import { MapEditor } from "/js/map/MapEditor.js";

export class ToolbarUi {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, MapService];
  }
  constructor(element, dom, resService, mapService) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.mapService = mapService;
    
    this.mapToolsVisible = false;
    this.mapServiceListener = this.mapService.listen(e => this.onMapServiceEvent(e));
    this.resServiceListener = this.resService.listen(e => this.onResServiceEvent(e));
    
    this.buildUi();
  }
  
  /* Public API.
   ********************************************************************/
  
  setTattleText(text) {
    this.element.querySelector(".tattle").innerText = text || "\u00a0";
  }
  
  setMapToolsVisible(visible) {
    visible = !!visible;
    if (visible === this.mapToolsVisible) return;
    const mapTools = this.element.querySelector(".mapTools");
    const mapPalette = this.element.querySelector(".mapPalette");
    const mapCommandsButton = this.element.querySelector(".mapCommandsButton");
    if (this.mapToolsVisible = visible) {
      mapTools.classList.remove("hidden");
      mapPalette.classList.remove("hidden");
      mapCommandsButton.classList.remove("hidden");
    } else {
      mapTools.classList.add("hidden");
      mapPalette.classList.add("hidden");
      mapCommandsButton.classList.add("hidden");
    }
  }
  
  /* DOM.
   ********************************************************/
  
  onRemoveFromDom() {
    this.mapService.unlisten(this.mapServiceListener);
    this.resService.unlisten(this.resServiceListener);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const mapTools = this.dom.spawn(this.element, "DIV", ["mapTools"], { "on-click": e => this.onMapToolsClick(e) });
    if (!this.mapToolsVisible) mapTools.classList.add("hidden");
    for (const tool of MapEditor.TOOLS) {
      const toolButton = this.dom.spawn(mapTools, "IMG", { src: `/img/tool-${tool.name}.png`, "data-tool": tool.name });
      if (this.mapService.selectedTool === tool.name) {
        toolButton.classList.add("selected");
      }
    }
    
    const mapPalette = this.dom.spawn(this.element, "CANVAS", ["mapPalette"], {
      width: 16,
      height: 16,
      "on-click": () => this.mapService.requestPaletteModal(),
    });
    if (!this.mapToolsVisible) mapPalette.classList.add("hidden");
    
    const mapCommandsButton = this.dom.spawn(this.element, "INPUT", ["mapCommandsButton"], { 
      type: "button",
      value: "Commands",
      "on-click": () => this.mapService.requestCommandsModal(),
    });
    if (!this.mapToolsVisible) mapCommandsButton.classList.add("hidden");
    
    this.dom.spawn(this.element, "DIV", ["spacer"]);
    
    this.dom.spawn(this.element, "DIV", ["saveIndicator", "clean"]);
    this.dom.spawn(this.element, "DIV", ["tattle"], "\u00a0");
  }
  
  setSaveIndicator(state) {
    const saveIndicator = this.element.querySelector(".saveIndicator");
    // If the current state is "error", don't replace it with "dirty". Only "clean" will do.
    if (saveIndicator.classList.contains("error")) {
      if (state !== "clean") return;
    }
    saveIndicator.classList.remove("error");
    saveIndicator.classList.remove("clean");
    saveIndicator.classList.remove("dirty");
    saveIndicator.classList.add(state);
  }
  
  /* Events.
   *****************************************************/
   
  onMapToolsClick(event) {
    const name = event.target.getAttribute("data-tool");
    if (!name) return;
    this.mapService.setSelectedTool(name); // don't react; wait for MapService to call back.
  }
   
  onMapServiceEvent(event) {
    switch (event.type) {
      case "selectedTool": {
          for (const element of this.element.querySelectorAll(".mapTools .selected")) {
            element.classList.remove("selected");
          }
          this.element.querySelector(`.mapTools *[data-tool='${this.mapService.selectedTool}']`).classList.add("selected");
        } break;
      case "paletteImageId": this.drawPalette(); break;
      case "selectedTile": this.drawPalette(); break;
    }
  }
  
  onResServiceEvent(event) {
    switch (event.type) {
      case "dirty": this.setSaveIndicator("dirty"); break;
      case "saved": this.setSaveIndicator("clean"); break;
      case "saveError": this.setSaveIndicator("error"); break;
    }
  }
  
  /* Palette.
   *****************************************************/
   
  drawPalette() {
    const canvas = this.element.querySelector(".mapPalette");
    const context = canvas.getContext("2d");
    context.imageSmoothingEnabled = false;
    context.clearRect(0, 0, canvas.width, canvas.height);
    const src = this.resService.getResourceObject("image", this.mapService.paletteImageId);
    if (src) {
      const tilesize = src.naturalWidth >> 4;
      const srcx = (this.mapService.selectedTile & 0x0f) * tilesize;
      const srcy = (this.mapService.selectedTile >> 4) * tilesize;
      context.drawImage(src, srcx, srcy, tilesize, tilesize, 0, 0, canvas.width, canvas.height);
    }
  }
}
