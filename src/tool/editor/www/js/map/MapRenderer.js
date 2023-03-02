/* MapRenderer.js
 * Responsible for drawing the map and converting between DOM coordinates and map cells.
 * Owned by MapUi; an unusual case of a service with "prototype" scope.
 */
 
import { FullmoonMap } from "/js/map/FullmoonMap.js";
import { ResService } from "/js/util/ResService.js";
import { MapService } from "/js/map/MapService.js";
 
export class MapRenderer {
  static getDependencies() {
    return [Window, ResService, MapService];
  }
  constructor(window, resService, mapService) {
    this.window = window;
    this.resService = resService;
    this.mapService = mapService;
    
    this.map = null;
    this.tileprops = null;
    this.canvas = null;
    this.renderTimeout = null;
    this.layoutDirty = true; // true to look up neighbors and calculate bounds at next render
    this.layout = []; // {map,bounds,dir} (dir) is absent or "nw","n","ne","w","e","sw","s","se"
    
    this.poiImages = {};
    const poiImagePromises = [];
    for (const name of ["sprite", "exit", "entrance", "hero"]) {
      const promise = new Promise((resolve, reject) => {
        const image = new Image();
        image.onload = resolve;
        image.onerror = reject;
        image.src = `/img/poi-${name}.png`;
        this.poiImages[name] = image;
      });
      poiImagePromises.push(promise);
    }
    Promise.all(poiImagePromises).then(() => this.renderSoon());
  }
  
  /* Regular public API.
   *****************************************************/
  
  setup(map, tileprops, canvas) {
    this.map = map;
    this.tileprops = tileprops;
    this.canvas = canvas;
    this.pois = this.mapService.getPois(this.map, this.resService);
    this.renderSoon();
  }
  
  // Call when bounds, neighbors, or commands change for a full refresh.
  setLayoutDirty() {
    this.pois = this.mapService.getPois(this.map, this.resService);
    this.layoutDirty = true;
    this.renderSoon();
  }
  
  // Will redraw everything. Won't look up neighbors or recalculate map positions.
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, 0);
  }
  
  /* Coordinate transforms.
   ***************************************************************/
   
  /* Given a point in my canvas, return one of:
   *   [map, col, row]
   *   [null, dx, dy] (-1,0,1).
   *   [map, col, row, poi]
   */
  mapCoordsFromCanvas(x, y) {
    for (const { map, bounds, dir } of this.layout) {
      if (x < bounds.x) continue;
      if (y < bounds.y) continue;
      if (x >= bounds.x + bounds.w) continue;
      if (y >= bounds.y + bounds.h) continue;
      const col = Math.floor((x - bounds.x) / (bounds.tilesize + bounds.spacing));
      const row = Math.floor((y - bounds.y) / (bounds.tilesize + bounds.spacing));
      
      if (!dir) {
        for (const poi of this.pois) {
          if (poi.x !== col) continue;
          if (poi.y !== row) continue;
          const badgeBounds = [
            bounds.x + col * (bounds.tilesize + bounds.spacing) + (bounds.tilesize >> 1),
            bounds.y + row * (bounds.tilesize + bounds.spacing) + (bounds.tilesize >> 1),
            12, 12,
          ];
          if (!(poi.index & 1)) badgeBounds[0] -= badgeBounds[2];
          if (!(poi.index & 2)) badgeBounds[1] -= badgeBounds[3];
          // yeah. this is fine.
          if ((x >= badgeBounds[0]) && (y >= badgeBounds[1]) && (x <= badgeBounds[0] + badgeBounds[2]) && (y <= badgeBounds[1] + badgeBounds[3])) {
            return [map, col, row, poi];
          }
        }
      }
      return [map, col, row];
    }
    if (this.layout.length < 1) return [null, 0, 0];
    const mid = this.layout[0].bounds;
    const dx = (x < mid.x) ? -1 : (x < mid.x + mid.w) ? 0 : 1;
    const dy = (y < mid.y) ? -1 : (y < mid.y + mid.h) ? 0 : 1;
    return [null, dx, dy];
  }
  
  /* {x,y,w,h,tilesize,spacing} in canvas.
   * (w=COLC*tilesize+(COLC-1)*spacing) and (h=ROWC*tilesize+(ROWC-1)*spacing).
   * Values will always be integers.
   */
  getMapBounds() {
    // Leave some border around the edge of canvas empty, to fill in with neighbors.
    const limitw = Math.max(0, (this.canvas.width * 7) >> 3);
    const limith = Math.max(0, (this.canvas.height * 7) >> 3);
    // Fit our maximum (unrounded) bounds into (limit).
    let guessw, guessh;
    const wforh = (FullmoonMap.COLC * limith) / FullmoonMap.ROWC;
    if (wforh <= limitw) {
      guessw = wforh;
      guessh = limith;
    } else {
      guessw = limitw;
      guessh = (FullmoonMap.ROWC * limitw) / FullmoonMap.COLC;
    }
    // Calculate (tilesize,spacing) to fit in (guess).
    const spacing = 1;
    const tilesize = Math.max(1, ~~Math.min(
      (guessw - (spacing * (FullmoonMap.COLC - 1))) / FullmoonMap.COLC,
      (guessh - (spacing * (FullmoonMap.ROWC - 1))) / FullmoonMap.ROWC
    ));
    // And the rest writes itself...
    const w = tilesize * FullmoonMap.COLC + (spacing * (FullmoonMap.COLC - 1));
    const h = tilesize * FullmoonMap.ROWC + (spacing * (FullmoonMap.ROWC - 1));
    const x = (this.canvas.width >> 1) - (w >> 1);
    const y = (this.canvas.height >> 1) - (h >> 1);
    return { x, y, w, h, tilesize, spacing };
  }
  
  /* Rebuild (layout) if dirty.
   * If no map is present, it will be empty.
   * If we do have a map, that main one goes in slot [0].
   * There can be up to 8 additional slots for neighbors.
   */
  requireLayout() {
    if (!this.layoutDirty) return;
    this.layoutDirty = false;
    this.layout = [];
    if (!this.map) return;
    
    const mainBounds = this.getMapBounds();
    this.layout.push({
      map: this.map,
      bounds: mainBounds,
    });
    
    // (dira,dirb) are the directions to proceed from the first neighbor.
    // Must be sorted, ie ("n","s") or ("w","e").
    const addNeighbors = (dx, dy, dir0, dira, dirb) => {
      const mapId = this.map.getNeighborId(dir0);
      if (!mapId) return;
      const map = this.resService.getResourceObject("map", mapId);
      if (!map) return;
      this.layout.push({
        map,
        dir: dir0,
        bounds: {
          ...mainBounds,
          x: mainBounds.x + mainBounds.w * dx,
          y: mainBounds.y + mainBounds.h * dy,
        },
      });
      let nextDirs = [];
      if ((dir0 === "n") || (dir0 === "s")) {
        nextDirs.push([-1, dy, dira, dir0 + dira]);
        nextDirs.push([1, dy, dirb, dir0 + dirb]);
      } else {
        nextDirs.push([dx, -1, dira, dira + dir0]);
        nextDirs.push([dx, 1, dirb, dirb + dir0]);
      }
      for (const [nextDx, nextDy, nextDir, nextDirCombined] of nextDirs) {
        if (this.layout.find(l => l.dir === nextDirCombined)) continue;
        const nextMap = this.resService.getResourceObject("map", map.getNeighborId(nextDir));
        if (!nextMap) continue;
        this.layout.push({
          map: nextMap,
          dir: nextDirCombined,
          bounds: {
            ...mainBounds,
            x: mainBounds.x + mainBounds.w * nextDx,
            y: mainBounds.y + mainBounds.h * nextDy,
          },
        });
      }
    };
    addNeighbors(0, -1, "n", "w", "e");
    addNeighbors(-1, 0, "w", "n", "s");
    addNeighbors(1, 0, "e", "n", "s");
    addNeighbors(0, 1, "s", "w", "e");
  }
  
  /* Render.
   *************************************************************/
  
  renderNow() {
    if (!this.canvas) return;
    this.canvas.width = this.canvas.offsetWidth;
    this.canvas.height = this.canvas.offsetHeight;
    this.requireLayout();
    const context = this.canvas.getContext("2d");
    context.imageSmoothingEnabled = false;
    context.fillStyle = "#666";
    context.fillRect(0, 0, this.canvas.width, this.canvas.height);
    if (this.layout.length < 1) return; // eg no map provided
    for (const { map, bounds } of this.layout) {
      this.renderMap(context, map, bounds);
    }
    this.renderBadges(context, this.layout[0].bounds);
    this.renderShade(context, this.layout[0].bounds);
  }
  
  // (style) is "main" or "neighbor". But actually not using it.
  // Renders only the map cells. No badges or shade.
  renderMap(context, map, bounds, style) {
    
    // TODO Fallback when image not available. Does it matter?
    const tilesheetId = this.resService.resolveId("image", map.getCommand("tilesheet"));
    if (!tilesheetId) return;
    const tilesheet = this.resService.getResourceObject("image", tilesheetId);
    if (!tilesheet) return;
    
    // TODO Could clamp to canvas first and avoid iterating over offscreen cells. Does it matter?
    const fullTilesize = bounds.tilesize + bounds.spacing;
    const srcTilesize = tilesheet.naturalWidth >> 4;
    for (let dsty=bounds.y, row=0, cellp=0; row<FullmoonMap.ROWC; row++, dsty+=fullTilesize) {
      for (let dstx=bounds.x, col=0; col<FullmoonMap.COLC; col++, dstx+=fullTilesize, cellp++) {
        const tileId = map.cells[cellp];
        context.drawImage(tilesheet, 0, 0, srcTilesize, srcTilesize, dstx, dsty, bounds.tilesize, bounds.tilesize);
        if (tileId) {
          const srcx = (tileId & 0x0f) * srcTilesize;
          const srcy = (tileId >> 4) * srcTilesize;
          context.drawImage(tilesheet, srcx, srcy, srcTilesize, srcTilesize, dstx, dsty, bounds.tilesize, bounds.tilesize);
        }
      }
    }
  }
  
  renderBadges(context, bounds) {
    const tilesize = this.layout[0].bounds.tilesize + this.layout[0].bounds.spacing;
    for (const poi of this.pois) {
      let image = null;
      let srcx = 0, srcy = 0, srcw = 0, srch = 0;
    
      // For sprites, use a thumbnail if possible.
      if (poi.type === "sprite") {
        const sprite = this.resService.getResourceObject("sprite", poi.spriteId);
        if (sprite) {
          const imageName = sprite.getCommand("image");
          const tileId = +sprite.getCommand("tile");
          // There's xform if we need it, not sure if that will matter.
          if (imageName && !isNaN(tileId)) {
            if (image = this.resService.getResourceObject("image", imageName)) {
              srcw = image.naturalWidth >> 4;
              srch = image.naturalHeight >> 4;
              srcx = (tileId & 15) * srcw;
              srcy = (tileId >> 4) * srch;
            }
          }
        }
      }
    
      // Everything else, use our canned images.
      if (!image) {
        image = this.poiImages[poi.type];
        if (!image || !image.complete) continue;
        srcw = image.naturalWidth;
        srch = image.naturalHeight;
      }
      
      // Badges get one corner against the center of the cell.
      // The first four POI for each cell will all be visible, then they start occluding each other.
      let x = this.layout[0].bounds.x + tilesize * poi.x + (tilesize >> 1);
      if (!(poi.index & 1)) x -= srcw; else x += 1;
      let y = this.layout[0].bounds.y + tilesize * poi.y + (tilesize >> 1);
      if (!(poi.index & 2)) y -= srch; else y += 1;
      
      context.drawImage(image, srcx, srcy, srcw, srch, x, y, srcw, srch);
    }
  }
  
  // Partly black out the region outside (bounds), and draw some loud lines to highlight the edges.
  renderShade(context, bounds) {
    context.fillStyle = "#000";
    context.globalAlpha = 0.5;
    context.fillRect(0, 0, bounds.x, this.canvas.height);
    context.fillRect(bounds.x + bounds.w, 0, this.canvas.width, this.canvas.height);
    context.fillRect(bounds.x, 0, bounds.w, bounds.y);
    context.fillRect(bounds.x, bounds.y + bounds.h, bounds.w, this.canvas.height);
    context.globalAlpha = 1.0;
    
    context.beginPath();
    context.moveTo(bounds.x, 0);
    context.lineTo(bounds.x, this.canvas.height);
    context.moveTo(bounds.x + bounds.w, 0);
    context.lineTo(bounds.x + bounds.w, this.canvas.height);
    context.moveTo(0, bounds.y);
    context.lineTo(this.canvas.width, bounds.y);
    context.moveTo(0, bounds.y + bounds.h);
    context.lineTo(this.canvas.width, bounds.y + bounds.h);
    context.strokeStyle = "#fff";
    context.stroke();
  }
}
