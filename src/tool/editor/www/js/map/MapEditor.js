/* MapEditor.js
 * Responsible for turning user interactions into map edits.
 * Owned by MapUi; an unusual case of a service with "prototype" scope.
 */
 
import { MapService } from "/js/map/MapService.js";
import { FullmoonMap } from "/js/map/FullmoonMap.js";
import { ResService } from "/js/util/ResService.js";
 
export class MapEditor {
  static getDependencies() {
    return [MapService, Window, ResService];
  }
  constructor(mapService, window, resService) {
    this.mapService = mapService;
    this.window = window;
    this.resService = resService;
    
    // Owner should set:
    this.onDirty = () => {};
    this.onVeryDirty = () => {}; // change to commands
    
    this.map = null;
    this.tileprops = null;
    this.mouseCoords = null; // MapRenderer.mapCoordsFromCanvas
    this.toolInProgress = ""; // Effective tool name while dragging.
    this.pencilAutoAdjust = false; // true while dragging if we're the rainbow pencil.
    this.bulkAnchor = [0, 0]; // col, row; while dragging bulk copy.
    this.metaDrag = null; // poi, while dragging
  }
  
  setup(map, tileprops) {
    this.map = map;
    this.tileprops = tileprops;
  }
  
  /* Events from UI.
   * Owner must digest mouse events and provide them in a format closer to the logical model.
   * See MapRenderer about that.
   ******************************************************************/
  
  onMouseMove(mouseCoords) {
    this.mouseCoords = mouseCoords;
    switch (this.toolInProgress) {
      case "pencil": this.pencilMove(); break;
      case "bulk": this.bulkMove(); break;
      case "meta": this.metaMove(); break;
    }
  }
  
  /* {button,altKey,shiftKey,ctrlKey}
   * Returns true if you should track it; we will expect onMouseUp at some point.
   */
  onMouseDown(event) {
    if (this.toolInProgress) this.onMouseUp();
    if (!this.map) return false;
    const tool = this.getEffectiveTool(event);
    switch (tool) {
      case "pencil": return this.pencilBegin(false);
      case "rainbow": return this.pencilBegin(true);
      case "bulk": return this.bulkBegin();
      case "pickup": return this.pickupBegin();
      case "meta": return this.metaBegin(event);
      case "finger": return this.fingerBegin();
    }
    return false;
  }
  
  onMouseUp() {
    const termTool = this.toolInProgress;
    this.toolInProgress = "";
    switch (termTool) {
      case "pencil": this.pencilEnd(); break;
      case "bulk": this.bulkEnd(); break;
      case "meta": this.metaEnd(); break;
    }
  }
  
  onMouseWheel(dx, dy) {
    //TODO Not using wheel yet, but it's easy to imagine.
  }
  
  /* Tool selection.
   * There's a last-minute opportunity to modify the effective tool based on modifiers.
   * We don't track modifiers held globally, so unfortunately the toolbar doesn't preview this behavior.
   ********************************************************/
   
  getEffectiveTool(event) {
    const base = this.mapService.selectedTool;
    const tool = MapEditor.TOOLS.find(t => t.name === base);
    if (!tool) return "";
    if (event.ctrlKey && event.shiftKey && tool.ctrlShiftKey) return tool.ctrlShiftKey;
    if (event.ctrlKey && tool.ctrlKey) return tool.ctrlKey;
    if (event.shiftKey && tool.shiftKey) return tool.shiftKey;
    if (event.altKey && tool.altKey) return tool.altKey;
    return base;
  }
  
  /* Pencil (including Rainbow Pencil).
   **********************************************************/
   
  pencilBegin(autoAdjust) {
    this.toolInProgress = "pencil";
    
    // If we don't have a tileprops resource with families defined, the Rainbow Pencil is just a Pencil.
    // But don't make a big deal out of it, you'll hurt its feelings.
    if (this.tileprops && this.tileprops.fields.family) this.pencilAutoAdjust = autoAdjust;
    else this.pencilAutoAdjust = false;
    
    this.pencilMove();
    return true;
  }
  
  pencilEnd() {
  }
  
  pencilMove() {
    if (!this.mouseCoords) return;
    if (this.mouseCoords[0] !== this.map) return;
    const x = this.mouseCoords[1];
    const y = this.mouseCoords[2];
    if ((x < 0) || (x >= FullmoonMap.COLC) || (y < 0) || (y >= FullmoonMap.ROWC)) return;
    
    this.map.cells[y * FullmoonMap.COLC + x] = this.mapService.selectedTile;
    if (this.pencilAutoAdjust) {
      for (let dy=-1; dy<=1; dy++) {
        for (let dx=-1; dx<=1; dx++) {
          this.shineRainbow(x + dx, y + dy);
        }
      }
    }
    this.onDirty();
  }
  
  shineRainbow(x, y) {
  
    // Focus cell must be in bounds and must have a nonzero family.
    if ((x < 0) || (y < 0) || (x >= FullmoonMap.COLC) || (y >= FullmoonMap.ROWC)) return;
    const tileid = this.map.cells[y * FullmoonMap.COLC + x];
    const family = this.tileprops.fields.family[tileid];
    if (!family) return;
    
    // If there's a weight table, and the focus tile has zero weight, don't touch it.
    if (this.tileprops.fields.weight && !this.tileprops.fields.weight[tileid]) return;
    
    // Initially the set of candidate tiles is everything in the focus's family.
    const candidates = [];
    for (let i=0; i<256; i++) {
      if (this.tileprops.fields.family[i] === family) {
        candidates.push(i);
      }
    }
    
    // If we have a neighbor mask table, determine the focus's neighbor mask and eliminate incompatible candidates.
    // Any candidate with a neighbor bit set, which is not among the neighbors we currently observe, is incompatible.
    if (this.tileprops.fields.neighbors) {
      const mask = this.gatherNeighborMask(x, y, family);
      const invalid = ~mask;
      for (let i=candidates.length; i-->0; ) {
        const ctileid = candidates[i];
        const cmask = this.tileprops.fields.neighbors[ctileid];
        if (cmask & invalid) candidates.splice(i, 1);
      }
      // It's possible to eliminate all possibilities here, eg a family that only works in fat masses.
      // If we did that, it's fine, just keep whatever is there.
      if (candidates.length < 1) return;
      // Next, count the bits in each candidate's neighbor mask. Only keep those with the most bits. Multiple only if there's a tie.
      const popCounts = candidates.map(ctileid => this.popcnt8(this.tileprops.fields.neighbors[ctileid]));
      const maxPop = Math.max(...popCounts);
      for (let i=candidates.length; i-->0; ) {
        if (popCounts[i] < maxPop) candidates.splice(i, 1);
      }
    }
  
    // If we have a weight table, eliminate zeroes, then select randomly, acknowledging weights.
    // They might all be zero, in which case we should keep whatever is present.
    if (this.tileprops.fields.weight) {
      let range = 0;
      for (let i=candidates.length; i-->0; ) {
        const ctileid = candidates[i];
        const cweight = this.tileprops.fields.weight[ctileid];
        if (!cweight) candidates.splice(i, 1);
        else range += cweight;
      }
      if (!range) return;
      let choice = Math.random() * range;
      for (const ctileid of candidates) {
        choice -= this.tileprops.fields.weight[ctileid];
        if (choice <= 0) {
          this.map.cells[y * FullmoonMap.COLC + x] = ctileid;
          return;
        }
      }
      // oops
      return;
    }
    
    // No weight table, so select uniformly from the candidates.
    const choice = Math.floor(Math.random() * candidates.length);
    this.map.cells[y * FullmoonMap.COLC + x] = candidates[choice];
  }
  
  popcnt8(v) {
    let c = 0;
    for (let i=0; i<8; i++, v>>=1) if (v & 1) c++;
    return c;
  }
  
  gatherNeighborMask(x, y, family) {
    let neighbors = 0;
    for (let dy=-1, qmask=0x80; dy<=1; dy++) {
      let ny = y + dy;
      if (ny < 0) ny = 0; // Clamp to nearest in-bounds cell. ie pretend the neighbor map matches our edge, which it should.
      else if (ny >= FullmoonMap.ROWC) ny = FullmoonMap.ROWC - 1;
      for (let dx=-1; dx<=1; dx++) {
        if (!dx && !dy) continue; // don't look in the middle (and don't advance qmask)
        let nx = x + dx;
        if (nx < 0) nx = 0;
        else if (nx >= FullmoonMap.COLC) nx = FullmoonMap.COLC - 1;
        const qtileid = this.map.cells[ny * FullmoonMap.COLC + nx];
        const qfamily = this.tileprops.fields.family[qtileid];
        if (qfamily === family) neighbors |= qmask;
        qmask >>= 1;
      }
    }
    return neighbors;
  }
  
  /* Bulk copy: The Mona Lisa Pencil.
   **********************************************************/
   
  bulkBegin() {
    if (!this.mouseCoords) return false;
    if (this.mouseCoords[0] !== this.map) return false;
    const x = this.mouseCoords[1];
    const y = this.mouseCoords[2];
    if ((x < 0) || (x >= FullmoonMap.COLC) || (y < 0) || (y >= FullmoonMap.ROWC)) return false;
    this.bulkAnchor = [x, y];
    this.toolInProgress = "bulk";
    this.bulkMove();
    return true;
  }
  
  bulkEnd() {
  }
  
  bulkMove() {
    if (!this.mouseCoords) return;
    if (this.mouseCoords[0] !== this.map) return;
    const x = this.mouseCoords[1];
    const y = this.mouseCoords[2];
    if ((x < 0) || (x >= FullmoonMap.COLC) || (y < 0) || (y >= FullmoonMap.ROWC)) return;
    const dx = x - this.bulkAnchor[0];
    const dy = y - this.bulkAnchor[1];
    const tcol = (this.mapService.selectedTile & 0x0f) + dx;
    const trow = (this.mapService.selectedTile >> 4) + dy;
    if ((tcol < 0) || (tcol > 15) || (trow < 0) || (trow > 15)) return;
    this.map.cells[y * FullmoonMap.COLC + x] = (trow << 4) | tcol;
    this.onDirty();
  }
  
  /* Meta. Add, edit, or move.
   *********************************************************/
   
  metaBegin(event) {
    if (!this.map) return false;
    if (!this.mouseCoords) return false;
    if (this.mouseCoords[3]) { // clicked on a poi. either edit or move
      if (event.ctrlKey) { // control=edit
        this.mapService.requestPoiModal(this.map, this.mouseCoords[3]);
        return false;
      } else { // drag
        this.toolInProgress = "meta";
        this.metaDrag = this.mouseCoords[3];
        return true;
      }
    } else { // clicked in free space. add poi
      this.mapService.requestNewPoiModal(this.map, this.mouseCoords[1], this.mouseCoords[2]);
    }
    return false;
  }
  
  metaEnd() {
    this.metaDrag = null;
  }
  
  metaMove() {
    if (!this.metaDrag) return;
    this.mapService.movePoi(this.map, this.metaDrag, this.mouseCoords[1], this.mouseCoords[2], this.resService);
    this.onVeryDirty();
  }
  
  /* Pickup.
   ************************************************************/
   
  pickupBegin() {
    if (!this.map) return false;
    if (!this.mouseCoords) return false;
    if (this.mouseCoords[0] !== this.map) return false;
    const x = this.mouseCoords[1];
    const y = this.mouseCoords[2];
    if ((x < 0) || (x >= FullmoonMap.COLC) || (y < 0) || (y >= FullmoonMap.ROWC)) return false;
    this.mapService.setSelectedTile(this.map.cells[y * FullmoonMap.COLC + x]);
    return false;
  }
  
  /* Finger.
   ********************************************************/
   
  fingerBegin() {
    if (!this.mouseCoords) return false;
    const poi = this.mouseCoords[3];
    if (!poi) return false;
    // so.... i consider it outside our scope to notify ResService of dirty maps,
    // but navigating to another page direct against Window?
    // yeah that's ok
    // Consider using a callback or something for this.
    switch (poi.type) {
      case "exit": this.window.location = `#map/${poi.mapId}`; return false;
      case "entrance": this.window.location = `#map/${poi.mapId}`; return false;
      case "buried_door": this.window.location = `#map/${poi.mapId}`; return false;
    }
    return false;
  }
}

/* (name) is what we pass around. There must be a 16x16 image /img/tool-${name}.png
 * (altKey,ctrlKey,shiftKey,ctrlShiftKey) are which tool to use instead, when starting this one with the given modifiers.
 * I'm not actually using Alt because the window manager likes to own that one.
 * (hotkey) is a key you can press globally to select this tool. Avoid anything text-related, it might interfere with text entry.
 * Tools will display in the toolbar in the order listed here.
 */
MapEditor.TOOLS = [{
  name: "pencil",
  ctrlKey: "pickup",
  shiftKey: "rainbow",
  ctrlShiftKey: "meta",
  hotkey: "F1",
}, {
  name: "rainbow",
  ctrlKey: "pickup",
  shiftKey: "pencil",
  ctrlShiftKey: "meta",
  hotkey: "F2",
}, {
  name: "bulk",
  ctrlKey: "pickup",
  ctrlShiftKey: "meta",
  hotkey: "F3",
}, {
  name: "pickup",
  hotkey: "F4",
}, {
  name: "meta", // add, edit, move; delete via edit. Just one tool!
  // Don't override ctrlKey.
  shiftKey: "finger",
  hotkey: "F5",
}, {
  name: "finger",
  shiftKey: "meta",
  hotkey: "F6",
}];
