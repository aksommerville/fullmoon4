/* MapAllUi.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { MapService } from "/js/map/MapService.js";
import { ResService } from "/js/util/ResService.js";
import { WorldMapGenerator } from "/js/map/WorldMap.js";
import { FullmoonMap } from "/js/map/FullmoonMap.js";

export class MapAllUi {
  static getDependencies() {
    return [HTMLElement, Dom, MapService, ResService, WorldMapGenerator, Window];
  }
  constructor(element, dom, mapService, resService, worldMapGenerator, window) {
    this.element = element;
    this.dom = dom;
    this.mapService = mapService;
    this.resService = resService;
    this.worldMapGenerator = worldMapGenerator;
    this.window = window;
    
    // Owner:
    this.setTattleText = (text) => {};
    
    this.NATURAL_TILESIZE = 16; // for rendering scratch maps. should match the real game.
    this.TILESIZE = 4; // for our measurement purposes
    this.SPACING = 1;
    this.COLW = FullmoonMap.COLC * this.TILESIZE + this.SPACING;
    this.ROWH = FullmoonMap.ROWC * this.TILESIZE + this.SPACING;
    
    this.renderTimeout = null;
    this.scratchCanvas = this.dom.createElement("CANVAS");
    this.scratchCanvas.width = FullmoonMap.COLC * this.NATURAL_TILESIZE;
    this.scratchCanvas.height = FullmoonMap.ROWC * this.NATURAL_TILESIZE;
    
    const mapResType = "map" + this.resService.mapSet;
    this.worldMap = this.worldMapGenerator.generateWorldMap(
      this.resService.toc.filter(res => res.type === mapResType)
    );
    this.session = null;
    
    this.element.addEventListener("mousemove", e => this.onMouseMove(e));
    this.lastTattleMapId = 0;
    
    this.buildUi();
    this.renderSoon();
  }
  
  onRemoveFromDom() {
    if (this.renderTimeout) {
      this.window.clearTimeout(this.renderTimeout);
      this.renderTimeout = null;
    }
  }
  
  /* Not for the main view; this is used by LogsUi to show session details geographically.
   * Also an opportunity to re-check ResService.mapSet, though we do need a more universal solution for that.
   */
  rebuildWithSession(session) {
    const mapResType = "map" + this.resService.mapSet;
    this.worldMap = this.worldMapGenerator.generateWorldMap(
      this.resService.toc.filter(res => res.type === mapResType)
    );
    this.session = session;
    this.buildUi();
    this.renderSoon();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const canvas = this.dom.spawn(this.element, "CANVAS", ["mainView"]);
    const scroller = this.dom.spawn(this.element, "DIV", ["scroller"], { "on-scroll": () => this.renderSoon() });
    const sizer = this.dom.spawn(scroller, "DIV", ["sizer"], { "on-click": event => this.onClick(event) });
    
    const [worldw, worldh] = this.measureWorld();
    
    const pad = 20; // a little extra size to ensure the scroll bars don't occlude the map.
    // ...there are better ways to get a scroll bar's width but meh.
    sizer.style.width = `${worldw + pad}px`;
    sizer.style.height = `${worldh + pad}px`;
  }
  
  onClick(event) {
    const x = Math.floor(event.offsetX / this.COLW);
    if ((x < 0) || (x >= this.worldMap.w)) return;
    const y = Math.floor(event.offsetY / this.ROWH);
    if ((y < 0) || (y >= this.worldMap.h)) return;
    const map = this.worldMap.maps[y * this.worldMap.w + x];
    if (!map) return;
    this.window.location = `#map/${map.id}`;
  }
  
  // Display size of the full world map.
  measureWorld() {
    return [
      this.worldMap.w * this.COLW,
      this.worldMap.h * this.ROWH,
    ];
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, 0);
  }
  
  renderNow() {
    const scroller = this.element.querySelector(".scroller");
    const canvas = this.element.querySelector(".mainView");
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
    const context = canvas.getContext("2d");
    
    context.fillStyle = "#8ac";
    context.fillRect(0, 0, canvas.width, canvas.height);
    
    let y = Math.max(Math.floor(scroller.scrollTop / this.ROWH), 0);
    let x0 = Math.max(Math.floor(scroller.scrollLeft / this.COLW), 0);
    let dsty = y * this.ROWH - scroller.scrollTop;
    let dstx0 = x0 * this.COLW - scroller.scrollLeft;
    for (; (y<this.worldMap.h) && (dsty<canvas.height); y++, dsty+=this.ROWH) {
      for (let x=x0, dstx=dstx0; (x<this.worldMap.w) && (dstx<canvas.width); x++, dstx+=this.COLW) {
        this.renderMap(context, dstx, dsty, this.worldMap.maps[y * this.worldMap.w + x]);
      }
    }
  }
  
  renderMap(context, dstx, dsty, map) {
    if (!map) return;
    const imageId = this.resService.resolveId("image", map.getCommand("tilesheet"));
    const tilesheet = imageId && this.resService.getResourceObject("image", imageId);
    if (tilesheet) {
      this.renderScratchMap(map, tilesheet);
    } else {
      this.renderScratchMapImageless(map);
    }
    context.drawImage(this.scratchCanvas, 
      0, 0, this.scratchCanvas.width, this.scratchCanvas.height,
      dstx, dsty, FullmoonMap.COLC * this.TILESIZE, FullmoonMap.ROWC * this.TILESIZE
    );
    if (this.session) {
      this.decorateScaledDownMap(
        context,
        dstx, dsty, FullmoonMap.COLC * this.TILESIZE, FullmoonMap.ROWC * this.TILESIZE,
        map, this.session
      );
    }
  }
  
  decorateScaledDownMap(context, dstx, dsty, dstw, dsth, map, session) {
    // Overlay with a flat color based on visits to map.
    const visit = session.maps?.find(m => m.mapid === map.id) || { duration: 0, count: 0 };
    if (!visit.count) {
      context.globalAlpha = 0.7;
      context.fillStyle = "#888";
      context.fillRect(dstx, dsty, dstw, dsth);
      context.globalAlpha = 1;
      context.strokeStyle = "#000";
      context.strokeRect(dstx, dsty, dstw, dsth);
    } else {
      const maxDuration = Math.max(...session.maps.map(m => m.duration));
      const normDuration = visit.duration / maxDuration;
      context.globalAlpha = 0.2 + 0.6 * normDuration;
      context.fillStyle = "#0f0";
      context.fillRect(dstx, dsty, dstw, dsth);
      context.globalAlpha = 1;
    }
    // Red bar if any injuries here.
    const injuries = (session.injuries || []).filter(i => i.location[0] === map.id);
    if (injuries.length > 0) {
      const barx = dstx + 4;
      const bary = dsty + 4;
      const barh = 4;
      const wlimit = dstw - 8;
      let barw = 4 + injuries.length * 2;
      if (barw > wlimit) barw = wlimit;
      context.fillStyle = "#f00";
      context.fillRect(barx, bary, barw, barh);
    }
  }
  
  renderScratchMap(map, tilesheet) {
    const context = this.scratchCanvas.getContext("2d");
    for (let yi=0, y=0, vp=0; yi<FullmoonMap.ROWC; yi++, y+=this.NATURAL_TILESIZE) {
      for (let xi=0, x=0; xi<FullmoonMap.COLC; xi++, x+=this.NATURAL_TILESIZE, vp++) {
        const srcx = (map.cells[vp] & 0xf) * this.NATURAL_TILESIZE;
        const srcy = (map.cells[vp] >> 4) * this.NATURAL_TILESIZE;
        context.drawImage(tilesheet,
          0, 0, this.NATURAL_TILESIZE, this.NATURAL_TILESIZE,
          x, y, this.NATURAL_TILESIZE, this.NATURAL_TILESIZE
        );
        if (map.cells[vp]) {
          context.drawImage(tilesheet,
            srcx, srcy, this.NATURAL_TILESIZE, this.NATURAL_TILESIZE,
            x, y, this.NATURAL_TILESIZE, this.NATURAL_TILESIZE
          );
        }
      }
    }
  }
  
  renderScratchMapImageless(map) {
    const context = this.scratchCanvas.getContext("2d");
    for (let yi=0, y=0, vp=0; yi<FullmoonMap.ROWC; yi++, y+=this.NATURAL_TILESIZE) {
      for (let xi=0, x=0; xi<FullmoonMap.COLC; xi++, x+=this.NATURAL_TILESIZE, vp++) {
        const v = map.cells[vp];
        let r = v & 0xe0; r |= r >> 3; r |= r >> 6;
        let g = v & 0x1c; g |= g << 3; g |= g >> 6;
        let b = v & 0x03; b |= b << 2; b |= b << 4;
        context.fillStyle = `rgb(${r}, ${g}, ${b})`;
        context.fillRect(x, y, this.NATURAL_TILESIZE, this.NATURAL_TILESIZE);
      }
    }
  }
  
  onMouseMove(event) {
    const mapId = this.getMapIdForEvent(event);
    if (mapId === this.lastTattleMapId) return;
    this.lastTattleMapId = mapId;
    this.setTattleText(mapId || "");
  }
  
  getMapIdForEvent(event) {
    const x = Math.floor(event.offsetX / this.COLW);
    if ((x < 0) || (x >= this.worldMap.w)) return 0;
    const y = Math.floor(event.offsetY / this.ROWH);
    if ((y < 0) || (y >= this.worldMap.h)) return 0;
    const map = this.worldMap.maps[y * this.worldMap.w + x];
    if (!map) return 0;
    return map.id;
  }
}
