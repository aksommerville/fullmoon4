/* MapService.js
 * Your one-stop shop for all* things FullmoonMap.
 * [*] Not all, and not most either.
 */
 
import { FullmoonMap } from "./FullmoonMap.js";
 
export class MapService {
  static getDependencies() {
    return [];
  }
  constructor() {
    this.selectedTool = "rainbow";
    this.selectedTile = 0;
    this.paletteImageId = 0;
    
    this.listeners = []; // {id,cb}
    this.nextListenerId = 1;
  }
  
  /* Observable state.
   ***********************************************************/
  
  /* (cb) is called with: {
   *   type: "selectedTool"
   *   selectedTool: string
   * } or {
   *   type: "requestPaletteModal"
   * } or {
   *   type: "paletteImageId"
   *   paletteImageId: number
   * } or {
   *   type: "selectedTile"
   *   selectedTile: number
   * } or {
   *   type: "requestCommandsModal"
   * } or {
   *   type: "requestPoiModal"
   *   map
   *   poi
   * } or {
   *   type: "requestNewPoiModal"
   *   map
   *   x,y: number
   * }
   */
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({ id, cb });
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p < 0) return;
    this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    for (const { id, cb } of this.listeners) cb(event);
  }
  
  setSelectedTool(tool) {
    if (tool === this.selectedTool) return;
    this.selectedTool = tool;
    this.broadcast({ type: "selectedTool", selectedTool: this.selectedTool });
  }
  
  requestPaletteModal() {
    this.broadcast({ type: "requestPaletteModal" });
  }
  
  setSelectedTile(tile) {
    if (tile === this.selectedTile) return;
    this.selectedTile = tile;
    this.broadcast({ type: "selectedTile", selectedTile: this.selectedTile });
  }
  
  setPaletteImageId(imageId) {
    if (imageId === this.paletteImageId) return;
    this.paletteImageId = imageId;
    this.broadcast({ type: "paletteImageId", paletteImageId: this.paletteImageId });
  }
  
  requestCommandsModal() {
    this.broadcast({ type: "requestCommandsModal" });
  }
  
  requestPoiModal(map, poi) {
    this.broadcast({ type: "requestPoiModal", map, poi });
  }
  
  requestNewPoiModal(map, x, y) {
    this.broadcast({ type: "requestNewPoiModal", map, x, y });
  }
  
  /* Decode.
   ***********************************************************/
   
  decode(serial, id) {
    const map = new FullmoonMap(serial);
    map.id = id || 0;
    return map;
  }
  
  /* Encode.
   *******************************************************/
   
  encode(map) {
    return map.encode();
  }
  
  /* Points Of Interest.
   * These are commands which will display on the map for interaction.
   * As such they are bound to a specific cell of the map.
   * Can include commands from other maps (eg doors that lead here).
   * You have to supply the ResService. We can't inject it because that would be a loop, it injects us.
   ********************************************************/
  
  getPois(map, resService) {
    const pois = [];
    if (map) {
      for (const command of map.commands) {
        switch (command[0]) {
          case "door": {
              const x = +command[1], y = +command[2];
              pois.push({
                type: "exit",
                x, y,
                index: pois.filter(p => p.x === x && p.y === y).length,
                mapId: +command[3],
                dstx: +command[4],
                dsty: +command[5],
              });
            } break;
          case "sprite": {
              const x = +command[1], y = +command[2];
              pois.push({
                type: "sprite",
                x, y,
                index: pois.filter(p => p.x === x && p.y === y).length,
                spriteId: +command[3] || command[3],
                argv: [command[4] || 0, command[5] || 0, command[6] || 0],
              });
            } break;
          case "hero": {
              const x = +command[1], y = +command[2];
              pois.push({
                type: "hero",
                x, y,
              });
            } break;
          case "transmogrify": {
              const x = +command[1], y = +command[2], mode = command[3], state = command[4];
              pois.push({
                type: "transmogrify",
                x, y, mode, state,
              });
            } break;
        }
      }
    }
    if (resService && map) {
      const myIdAsString = map.id.toString();
      for (const { type, object } of resService.toc) {
        if (type !== "map") continue;
        if (!object || (object === map)) continue;
        for (const command of object.commands) {
          if (command[0] === "door") {
            if (command[3] === myIdAsString) {
              const x = +command[4], y = +command[5];
              pois.push({
                type: "entrance",
                x, y,
                index: pois.filter(p => p.x === x && p.y === y).length,
                mapId: object.id,
                dstx: +command[1],
                dsty: +command[2],
              });
            }
          }
        }
      }
    }
    return pois;
  }
  
  movePoi(map, poi, x, y, resService) {
    if (!map || !poi) return false;
    if ((x < 0) || (y < 0) || (x >= FullmoonMap.COLC) || (y >= FullmoonMap.ROWC)) return false;
    if ((x === poi.x) && (y === poi.y)) return false;
    const remoteMapHandle = [];
    const command = this.findPoiCommand(map, poi, resService, remoteMapHandle);
    if (!command) return false;
    poi.x = x;
    poi.y = y;
    switch (poi.type) {
      case "entrance": command[4] = x.toString(); command[5] = y.toString(); break;
      case "exit": command[1] = x.toString(); command[2] = y.toString(); break;
      case "sprite": command[1] = x.toString(); command[2] = y.toString(); break;
      case "hero": command[1] = x.toString(); command[2] = y.toString(); break;
      case "transmogrify": command[1] = x.toString(); command[2] = y.toString(); break;
    }
    if (remoteMapHandle[0]) {
      // dirtying resources is really not our job, but we don't have any other way to tell our caller that it happened.
      resService.dirty("map", remoteMapHandle[0].id, remoteMapHandle[0]);
    }
    return true;
  }
  
  findPoiCommand(map, poi, resService, remoteMapHandle) {
    switch (poi.type) {
    
      case "entrance": {
          for (const { type, object } of resService.toc) {
            if (type !== "map") continue;
            if (object === map) continue;
            if (object.id !== poi.mapId) continue;
            for (const command of object.commands) {
              if (command[0] !== "door") continue;
              if (+command[1] !== poi.dstx) continue;
              if (+command[2] !== poi.dsty) continue;
              if (+command[3] !== map.id) continue;
              if (+command[4] !== poi.x) continue;
              if (+command[5] !== poi.y) continue;
              remoteMapHandle[0] = object;
              return command;
            }
          }
        } break;
        
      case "exit": {
          for (const command of map.commands) {
            if (command[0] !== "door") continue;
            if (+command[1] !== poi.x) continue;
            if (+command[2] !== poi.y) continue;
            if (+command[3] !== poi.mapId) continue;
            if (+command[4] !== poi.dstx) continue;
            if (+command[5] !== poi.dsty) continue;
            return command;
          }
        } break;
        
      case "sprite": {
          for (const command of map.commands) {
            if (command[0] !== "sprite") continue;
            if (+command[1] !== poi.x) continue;
            if (+command[2] !== poi.y) continue;
            if (command[4] !== poi.argv[0]) continue;
            if (command[5] !== poi.argv[1]) continue;
            if (command[6] !== poi.argv[2]) continue;
            return command;
          }
        } break;
        
      case "hero": {
          for (const command of map.commands) {
            if (command[0] !== "hero") continue;
            if (+command[1] !== poi.x) continue;
            if (+command[2] !== poi.y) continue;
            return command;
          }
        } break;
        
      case "transmogrify": {
          for (const command of map.commands) {
            if (command[0] !== "transmogrify") continue;
            if (+command[1] !== poi.x) continue;
            if (+command[2] !== poi.y) continue;
            return command;
          }
        } break;
    }
    return null;
  }
  
  /* Create a neighbor map.
   ************************************************************/
   
  createNewNeighbor(fromMap, dx, dy, resService) {
    if (!fromMap || !resService) return 0;
    
    let dir, reverseDir;
         if ((dx < 0) && !dy) { dir = "w"; reverseDir = "e"; }
    else if ((dx > 0) && !dy) { dir = "e"; reverseDir = "w"; }
    else if (!dx && (dy < 0)) { dir = "n"; reverseDir = "s"; }
    else if (!dx && (dy > 0)) { dir = "s"; reverseDir = "n"; }
    else return 0;
    
    // Ensure we don't already have a neighbor in this direction.
    if (fromMap.getNeighborId(dir)) return 0;
    
    const toId = resService.unusedId("map");
    if (isNaN(toId) || (toId < 1) || (toId > 0xffff)) return 0;
    fromMap.setCommand("neighbor" + dir, toId);
    
    const toMap = new FullmoonMap();
    toMap.id = toId;
    toMap.setCommand("tilesheet", fromMap.getCommand("tilesheet") || 1);
    toMap.setCommand("neighbor" + reverseDir, fromMap.id);
    
    /* Important! Walk from fromMap around the new one, and check if any other neighbors exist.
     * If so, we must add them right now. Things will go seriously cock-eyed if we skip this.
     * (path) are cardinal directions to stop from fromMap. Append ":DIR" when the new map should be visible from current position.
     */
    const lookForNeighbors = (...path) => {
      let currentMap = fromMap;
      for (const step of path) {
        const [stepDir, checkDir] = step.split(":");
        if (!(currentMap = resService.getResourceObject("map", currentMap.getNeighborId(stepDir)))) return;
        if (checkDir) {
          const reverseCheckDir = (checkDir === "w") ? "e" : (checkDir === "e") ? "w" : (checkDir === "n") ? "s" : "n";
          toMap.setCommand("neighbor" + reverseCheckDir, currentMap.id);
          const conflictId = currentMap.getNeighborId(checkDir);
          if (!conflictId) { // good. assign it
            currentMap.setCommand("neighbor" + checkDir, toId);
            resService.dirty("map", currentMap.id, currentMap);
          } else if (conflictId === toId) { // already been here, fine
          } else { // conflict!!!
            console.error(`Neighbor conflict placing new map! New ID ${toId}. Map ${currentMap.id} thinks ${conflictId} should go there (dir ${checkDir})`);
          }
        }
      }
    };
    switch (dir) {
      case "w": lookForNeighbors("n", "w:s", "w", "s:e"); lookForNeighbors("s", "w:n", "w", "n:e"); break;
      case "e": lookForNeighbors("n", "e:s", "e", "s:w"); lookForNeighbors("s", "e:n", "e", "n:w"); break;
      case "n": lookForNeighbors("w", "n:e", "n", "e:s"); lookForNeighbors("e", "n:w", "n", "w:s"); break;
      case "s": lookForNeighbors("w", "s:e", "s", "e:n"); lookForNeighbors("e", "s:w", "s", "w:n"); break;
    }

    resService.dirty("map", fromMap.id, fromMap);
    resService.dirty("map", toId, toMap);
    return toId;
  }
  
  /* Create a new map, make up an ID. Not a neighbor of an existing one.
   * PoiModal presents this option when you make a door.
   ***********************************************************/
   
  createMap(resService) {
    if (!resService) throw new Error(`MapService.createMap() caller must supply a ResService`);
    const id = resService.unusedId("map");
    if (isNaN(id) || (id < 1) || (id > 0xffff)) throw new Error(`Unable to allocate a new map id`);
    const map = new FullmoonMap();
    map.id = id;
    resService.dirty("map", id, map);
    return id;
  }
}

MapService.singleton = true;
