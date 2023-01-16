/* MapUi.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { MapService } from "/js/map/MapService.js";
import { ResService } from "/js/util/ResService.js";
import { MapRenderer } from "/js/map/MapRenderer.js";
import { MapEditor } from "/js/map/MapEditor.js";
import { PaletteModal } from "/js/map/PaletteModal.js";
import { MapCommandsModal } from "/js/map/MapCommandsModal.js";
import { PoiModal } from "/js/map/PoiModal.js";

export class MapUi {
  static getDependencies() {
    return [HTMLElement, Dom, MapService, ResService, MapRenderer, MapEditor, Window];
  }
  constructor(element, dom, mapService, resService, mapRenderer, mapEditor, window) {
    this.element = element;
    this.dom = dom;
    this.mapService = mapService;
    this.resService = resService;
    this.mapRenderer = mapRenderer;
    this.mapEditor = mapEditor;
    this.window = window;
    
    // Owner should set:
    this.setTattleText = (text) => {};
    
    this.map = null;
    this.tileprops = null;
    this.mouseUpListener = null;
    this.mouseMapCoords = null; // null or [map,col,row] or [null,dx,dy]
    
    this.buildUi();
    
    this.mapEditor.onDirty = () => this.onDirty();
    this.mapEditor.onVeryDirty = () => this.onVeryDirty();
    
    this.mapServiceListener = this.mapService.listen(e => this.onMapServiceEvent(e));
    
    const resizeObserver = new this.window.ResizeObserver(() => this.mapRenderer.setLayoutDirty());
    resizeObserver.observe(this.element);
    
    this.element.addEventListener("mousedown", e => this.onMouseDown(e));
    this.element.addEventListener("mousemove", e => this.onMouseMove(e));
    this.element.addEventListener("mouseleave", e => this.onMouseMove(e));
    this.element.addEventListener("mousewheel", e => this.onMouseWheel(e));
    this.element.addEventListener("contextmenu", e => e.preventDefault());
    this.window.addEventListener("keydown", this.keyListener = e => this.onKeyDown(e));
  }
  
  onRemoveFromDom() {
    this.dropMouseListeners();
    this.window.removeEventListener("keydown", this.keyListener);
    this.mapService.unlisten(this.mapServiceListener);
  }
  
  setup(mapId, args) {
    this.map = this.resService.getResourceObject("map", mapId);
    const imageId = this.map ? this.map.getIntCommand("tilesheet") : 0;
    this.tileprops = this.resService.getResourceObject("tileprops", imageId);
    this.mapRenderer.setup(this.map, this.tileprops, this.element.querySelector(".mainView"));
    this.mapEditor.setup(this.map, this.tileprops);
    this.mapService.setPaletteImageId(imageId);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "CANVAS", ["mainView"]);
  }
  
  /* Input events.
   *********************************************************/
   
  dropMouseListeners() {
    if (this.mouseUpListener) {
      this.window.removeEventListener("mouseup", this.mouseUpListener);
      this.mouseUpListener = null;
    }
  }
  
  checkMouseMotion(content) {
  
    // Confirm logical position actually changed.
    if (content === this.mouseMapCoords) return;
    if (content && this.mouseMapCoords &&
      (content[0] === this.mouseMapCoords[0]) &&
      (content[1] === this.mouseMapCoords[1]) &&
      (content[2] === this.mouseMapCoords[2]) &&
      (content[3] === this.mouseMapCoords[3])
    ) return;
    this.mouseMapCoords = content;
    
    // Update tattle.
    if (!content) this.setTattleText("");
    else if (content[0] === this.map) {
      if (!content[3]) this.setTattleText(`${content[1]},${content[2]}`);
      else if (content[3].type === "sprite") this.setTattleText(`Sprite ${content[3].spriteId}`);
      else if (content[3].type === "exit") this.setTattleText(`To map ${content[3].mapId}`);
      else if (content[3].type === "entrance") this.setTattleText(`From map ${content[3].mapId}`);
      else this.setTattleText(content[3].type);
    } else if (content[0]) this.setTattleText(`Map ${content[0].id}`);
    else if (!content[1] && !content[2]) this.setTattleText("");
    else this.setTattleText("NEW");
    
    this.mapEditor.onMouseMove(content);
  }
  
  onMouseMove(e) {
    if (e.type === "mouseleave") {
      this.checkMouseMotion(null);
    } else {
      this.checkMouseMotion(this.mapRenderer.mapCoordsFromCanvas(e.offsetX, e.offsetY));
    }
  }
  
  onMouseDown(e) {
    this.dropMouseListeners();
    if (!this.mouseMapCoords) return;
    if (this.map && (this.mouseMapCoords[0] === this.map)) {
      if (!this.mapEditor.onMouseDown({
        button: e.button,
        altKey: e.altKey,
        ctrlKey: e.ctrlKey,
        shiftKey: e.shiftKey,
      })) return;
      this.mouseUpListener = e => {
        this.dropMouseListeners();
        this.mapEditor.onMouseUp();
      };
      this.window.addEventListener("mouseup", this.mouseUpListener);
    } else if (this.mouseMapCoords[0]) {
      this.window.location = `#map/${this.mouseMapCoords[0].id}`;
    } else {
      const newMapId = this.mapService.createNewNeighbor(this.map, this.mouseMapCoords[1], this.mouseMapCoords[2], this.resService);
      if (newMapId) {
        this.window.location = `#map/${newMapId}`;
      } else {
        console.error(`Failed to create neighbor map.`);
      }
    }
  }
  
  onMouseWheel(e) {
    // My workstation, Chrome on Linux, never reports (dx). (I thought it would if I hold Shift but no).
    const dx = Math.sign(e.deltaX);
    const dy = Math.sign(e.deltaY);
    if (!dx && !dy) return;
    this.mapEditor.onMouseWheel(dx, dy);
  }
  
  onKeyDown(e) {
    const tool = MapEditor.TOOLS.find(t => t.hotkey === e.code);
    if (!tool) return;
    this.mapService.setSelectedTool(tool.name);
    e.preventDefault();
    e.stopPropagation();
  }
  
  onDirty() {
    if (!this.map) return;
    this.mapRenderer.renderSoon();
    this.resService.dirty("map", this.map.id, this.map);
  }
  
  onVeryDirty() {
    if (!this.map) return;
    this.mapRenderer.setLayoutDirty();
    this.resService.dirty("map", this.map.id, this.map);
  }
  
  onMapServiceEvent(e) {
    switch (e.type) {
      case "requestPaletteModal": this.openPaletteModal(); break;
      case "requestCommandsModal": this.openCommandsModal(); break;
      case "requestPoiModal": this.openPoiModal(e.poi); break;
      case "requestNewPoiModal": this.openNewPoiModal(e.x, e.y); break;
    }
  }
  
  openPaletteModal() {
    if (!this.map) return;
    this.dom.spawnModal(PaletteModal);
    // No setup or anything; PaletteModal communicates via MapService
  }
  
  openCommandsModal() {
    if (!this.map) return;
    const modal = this.dom.spawnModal(MapCommandsModal);
    modal.setup(this.map);
    modal.onDirty = () => {
      // We use the more expensive setLayoutDirty, not renderSoon: Changing commands could change the tilesheet or neighbors.
      this.mapRenderer.setLayoutDirty();
      this.resService.dirty("map", this.map.id, this.map);
    };
  }
  
  openPoiModal(poi) {
    if (!this.map || !poi) return;
    
    // If it's an "entrance" POI, we will pretend to be the remote map.
    let map = this.map;
    if (poi.type === "entrance") {
      if (!(map = this.resService.getResourceObject("map", poi.mapId))) return;
      poi = {
        type: "exit",
        x: poi.dstx,
        y: poi.dsty,
        mapId: this.map.id,
        dstx: poi.x,
        dsty: poi.y,
      };
    }
    
    const modal = this.dom.spawnModal(PoiModal);
    modal.setupEdit(map, poi);
    modal.onDirty = (command) => {
      const remoteMapHandle = [];
      const liveCommand = this.mapService.findPoiCommand(map, poi, this.resService, remoteMapHandle);
      if (!liveCommand) {
        console.error(`poi command not found`, { poi, command });
        return;
      }
      liveCommand.splice(0, liveCommand.length, ...command);
      if (remoteMapHandle[0]) {
        this.resService.dirty("map", remoteMapHandle[0].id, remoteMapHandle[0]);
      } else {
        this.resService.dirty("map", map.id, map);
      }
      this.mapRenderer.setLayoutDirty();
    };
  }
  
  openNewPoiModal(x, y) {
    if (!this.map) return;
    const modal = this.dom.spawnModal(PoiModal);
    modal.setupNew(this.map, x, y);
    modal.onDirty = (command) => {
      this.map.commands.push(command);
      this.mapRenderer.setLayoutDirty();
      this.resService.dirty("map", this.map.id, this.map);
    };
    modal.onCreateRemoteCommand = (mapId, command) => {
      const map = this.resService.getResourceObject("map", mapId);
      if (!map) {
        console.error(`Can't create remote command, map ${mapId} not found.`, { command });
        return false;
      }
      map.commands.push(command);
      this.resService.dirty("map", mapId, map);
      return true;
    };
  }
}
