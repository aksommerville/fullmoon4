/* validateAggregateMaps.js
 * Tests that run against the full set of maps.
 * Maps must be validated before calling.
 */
 
/* Every map must save to exactly one save point, unambiguously.
 * Maps with explicit 'saveto' or 'hero' with spellid, we can ignore, those are explicit.
 * Every other map must only be able to reach 'hero' and 'saveto' commands bearing the same spellid.
 **************************************************************************/

function validateDeterministicSavePoints(maps, resources) {
  let warningCount = 0;
  
  /* Make a visit list parallel to (maps), ie indexed by mapId.
   * Any missing maps are automatically ok, ignore them.
   */
  const ok = [];
  for (let i=0; i<maps.length; i++) {
    const map = maps[i];
    if (!map) ok.push(true);
    else ok.push(false);
  }
  
  /* Search every map.
   * We're going to walk from these start points, and potentially mark lots of others ok along the way.
   * Maps with an explicit spell do not get marked ok.
   */
  for (let i=0; i<maps.length; i++) {
    if (ok[i]) continue;
    const map = maps[i];
    if (map.spellId) continue;
    if (map.saveTo) continue;
    const spellIds = findReachableSpellIds(map, maps, ok, i, resources);
    if (spellIds.length > 1) throw new Error(`map:${i}: Ambiguous save point ${JSON.stringify(spellIds)}`);
  }
  
  return warningCount;
}

// Call only for maps not ok yet, which don't have an explicit spell.
function findReachableSpellIds(map, maps, ok, mapId, resources) {
  ok[mapId] = true;
  const spellIds = new Set();
  const neighborMapIds = getReachableNeighborMapIds(map, resources);
  for (const neighborMapId of neighborMapIds) {
    if (ok[neighborMapId]) continue;
    const neighbor = maps[neighborMapId];
    if (neighbor.spellId) spellIds.add(neighbor.spellId);
    else if (neighbor.saveTo) spellIds.add(neighbor.saveTo);
    else {
      for (const spellId of findReachableSpellIds(neighbor, maps, ok, neighborMapId, resources)) {
        spellIds.add(spellId);
      }
    }
  }
  return Array.from(spellIds);
}

// Get all neighbor IDs, but exclude cardinal neighbors with a straight solid edge.
function getReachableNeighborMapIds(map, resources) {
  let cellphysics = [];
  const tileProps = resources.find(r => ((r.type === 4) && (r.id === map.imageId)));
  if (tileProps) cellphysics = tileProps.serial;
  const ids = [...map.doorMapIds];
  if (map.neighborw && !mapRegionIsSolid(map, 0, 0, 1, 12, cellphysics)) ids.push(map.neighborw);
  if (map.neighbore && !mapRegionIsSolid(map, 19, 0, 1, 12, cellphysics)) ids.push(map.neighbore);
  if (map.neighborn && !mapRegionIsSolid(map, 0, 0, 20, 1, cellphysics)) ids.push(map.neighborn);
  if (map.neighbors && !mapRegionIsSolid(map, 0, 11, 20, 1, cellphysics)) ids.push(map.neighbors);
  return ids;
}

function mapRegionIsSolid(map, x, y, w, h, cellphysics) {
  for (let yi=0; yi<h; yi++) {
    for (let xi=0, p=(y+yi)*20+x; xi<w; xi++, p++) {
      const tileid = map.serial[p];
      switch (cellphysics[tileid]) {
        case 1: // SOLID
        case 4: // UNCHALKABLE
        case 5: // SAP
        case 6: // SAP_NOCHALK
        case 8: // REVELABLE
          break;
        default: return false;
      }
    }
  }
  return true;
}
 
/* From every map, no matter which way we travel, the first SONG command we encounter, they'll all be the same song.
 * Following on that, if a map is a teleport target, it *must* declare its song explicitly.
 ***************************************************************************/
 
function validateDeterministicSongSelection(maps) {

  /* Take a list parallel to (maps), ie indexed by mapid, of effect song IDs.
   * Populate with explicit songs.
   * Assert that teleport targets have an explicit song; they are reachable from everywhere.
   */
  const effectiveSongIds = [];
  for (let i=0; i<maps.length; i++) {
    if (!maps[i]) effectiveSongIds.push(0);
    else if (maps[i].songId) effectiveSongIds.push(maps[i].songId);
    else if (maps[i].teleportable) throw new Error(`map:${i} is teleportable but does not declare a song`);
    else effectiveSongIds.push(0);
  }
  
  /* Now, everywhere we have an explicit song, flood-fill to its neighbors.
   * If we encounter a different implicit song during the flood-fill, that's an error.
   */
  for (let i=0; i<maps.length; i++) {
    floodFillSongIds(maps, effectiveSongIds, i, true, 0);
  }
  
  /* If any zeroes remain in valid slots of effectiveSongIds, it means those maps are unreachable.
   * We could assert that, but it's already being done in main.js.
   */
  
  return 0;
}

function floodFillSongIds(maps, effectiveSongIds, i, initial, incoming) {
  const map = maps[i];
  if (!map) return;
  
  if (initial) {
    if (!map.songId) return; // start from maps with an explicit song
    if (effectiveSongIds[i] !== map.songId) {
      throw new Error(`Conflict for song of map:${i}, have ${map.songId} explicitly and ${effectiveSongIds[i]} via neighbor.`);
    }
    incoming = map.songId;
    
  } else {
   if (map.songId) return; // explicit song, stop walking.
   if (effectiveSongIds[i]) {
      if (effectiveSongIds[i] !== incoming) {
        throw new Error(`Conflict for song of map:${i}, have ${effectiveSongIds[i]} and ${incoming} via different neighbors.`);
      }
      return;
    }
    effectiveSongIds[i] = incoming;
  }
  
  /* Not all neighbor relationships are traversable.
   * But for our purposes, we pretend they are.
   * One could examine the walls at each edge neighbor and skip it if it's fully solid, but I'd rather stay consistent across even those boundaries.
   * (especially since that's easier to do).
   */
  floodFillSongIds(maps, effectiveSongIds, map.neighborw, false, incoming);
  floodFillSongIds(maps, effectiveSongIds, map.neighbore, false, incoming);
  floodFillSongIds(maps, effectiveSongIds, map.neighborn, false, incoming);
  floodFillSongIds(maps, effectiveSongIds, map.neighbors, false, incoming);
  for (const neighborMapId of map.doorMapIds) {
    floodFillSongIds(maps, effectiveSongIds, neighborMapId, false, incoming);
  }
}

/* If a map is missing a neighbor, and that edge is not fully solid, it must also have blowback enabled.
 * This test doesn't actually require the full set, it's individual.
 * But it does require tileprops.
 ************************************************************************/
 
function loadCellPhysics(imageId, resources) {
  const res = resources.find(r => ((r.type === 4) && (r.id === imageId)));
  if (res) return res.serial;
  return [];
}

//TODO!!! This needs to account for firewall -- When firewall is in play, there is required to be an open edge, and don't worry, it's not reachable.

function requireBlowbackForOpenEdges(maps, resources) {
  let imageId = 0;
  let cellphysics = [];
  for (let id=0; id<maps.length; id++) {
    const map = maps[id];
    if (!map) continue;
    if (map.flags & 0x04) continue; // blowback enabled
    if (map.imageId !== imageId) {
      cellphysics = loadCellPhysics(map.imageId, resources);
      imageId = map.imageId;
    }
    if (!map.neighborw) requireBlowbackForOpenEdges1(map, id, cellphysics, 0, 0, 1, 12);
    if (!map.neighbore) requireBlowbackForOpenEdges1(map, id, cellphysics, 19, 0, 1, 12);
    if (!map.neighborn) requireBlowbackForOpenEdges1(map, id, cellphysics, 0, 0, 20, 1);
    if (!map.neighbors) requireBlowbackForOpenEdges1(map, id, cellphysics, 0, 11, 20, 1);
  }
  return 0;
}

function requireBlowbackForOpenEdges1(map, id, cellphysics, x, y, w, h) {
  for (let yi=0; yi<h; yi++) {
    for (let xi=0, p=(y+yi)*20+x; xi<w; xi++, p++) {
      const tileid = map.serial[p];
      switch (cellphysics[tileid]) {
        case 1: // SOLID
        case 4: // UNCHALKABLE
        case 5: // SAP
        case 6: // SAP_NOCHALK
        case 8: // REVELABLE
          break;
        default: throw new Error(`map:${id} at ${x+xi},${y+yi} must be solid or have blowback enabled.`);
      }
    }
  }
}

/* Validation for specific sprites.
 * These could have been done at the initial decode, they're not really aggregates.
 * Assertions:
 *  - Firewall sprites must be on the edge, must have a vacant neighbor, and must be bounded by solids.
 ************************************************************************/
 
function getSpriteByResourceId(resources, id) {
  const res = resources.find(r => ((r.type === 5) && (r.id === id)));
  if (!res) return null;
  const sprite = { id, bv: [] };
  let srcp = 0;
  while (srcp < res.serial.length) {
    const lead = res.serial[srcp++];
    if (!lead) break;
    switch (lead) {
    
      case 0x20: sprite.imageId = res.serial[srcp++]; break;
      case 0x21: sprite.tileid = res.serial[srcp++]; break;
      case 0x22: sprite.xform = res.serial[srcp++]; break;
      case 0x23: sprite.style = res.serial[srcp++]; break;
      case 0x24: sprite.physics = res.serial[srcp++]; break;
      case 0x25: sprite.invmass = res.serial[srcp++]; break;
      case 0x26: sprite.layer = res.serial[srcp++]; break;
      
      case 0x40: srcp += 2; break; // veldecay
      case 0x41: srcp += 2; break; // radius
      case 0x42: sprite.controller = (res.serial[srcp] << 8) | res.serial[srcp + 1]; srcp += 2; break;
      case 0x43: sprite.bv.push([res.serial[srcp], res.serial[srcp + 1]]); srcp += 2; break;
      
      default: throw new Error(`sprite:${id}: Unexpected command ${lead}`);
    }
  }
  return sprite;
}
 
function validateSprites(maps, resources) {
  let warningCount = 0;
  let spriteId = 0;
  let sprite = null;
  for (let i=0; i<maps.length; i++) {
    const map = maps[i];
    if (!map) continue;
    const cellphysics = loadCellPhysics(map.imageId, resources);
    for (const spawn of map.sprites) {
      if (spawn.id !== spriteId) {
        sprite = getSpriteByResourceId(resources, spawn.id);
        spriteId = spawn.id;
      }
      if (!sprite) {
        console.log(`map:${i}:WARNING: sprite:${spawn.id} not found`);
        warningCount++;
        continue;
      }
      switch (sprite.controller) {
      
        case 18: warningCount += validateSprite_firewall(map, i, spawn, cellphysics); break;
        
      }
    }
  }
  return warningCount;
}

function validateSprite_firewall(map, mapId, sprite, cellphysics) {
  if (!cellphysics) {
    console.log(`map:${mapId}: Can't validate firewall due to missing tileprops (${map.imageId})`);
    return 1;
  }
  let dx = 0, dy = 0;
  if (
    ((sprite.x < 1) || (sprite.x >= 19)) &&
    ((sprite.y < 1) || (sprite.y >= 11))
  ) throw new Error(`map:${mapId}: Firewall sprite must not be in corner.`);
  if ((sprite.x === 0) || (sprite.x === 19)) {
    dy = 1;
  } else if ((sprite.y === 0) || (sprite.y === 11)) {
    dx = 1;
  } else throw new Error(`map:${mapId}: Firewall sprite must be on edge.`);
  const solid = (physics) => {
    switch (physics) {
      case 0: // VACANT
      case 2: // HOLE
      case 3: // UNSHOVELLABLE
      case 7: // WATER
        return false;
      default: return true;
    }
  };
  let px = sprite.x, py = sprite.y, vacantc = 0, aok = false, bok = false;
  while ((px < 20) && (py < 12)) {
    if (solid(cellphysics[map.serial[py * 20 + px]])) { aok = true; break; }
    vacantc++;
    px += dx;
    py += dy;
  }
  px = sprite.x - dx;
  py = sprite.y - dy;
  while ((px >= 0) && (py >= 0)) {
    if (solid(cellphysics[map.serial[py * 20 + px]])) { bok = true; break; }
    vacantc++;
    px -= dx;
    py -= dy;
  }
  if (vacantc < 2) throw new Error(`map:${mapId}: Insufficient free space for firewall at ${sprite.x},${sprite.y}. Is it spawned on a solid?`);
  if (!aok || !bok) throw new Error(`map:${mapId}: Firewall must be bounded by solids.`);
  return 0;
}

/* Cardinal neighbors must share a value for 'indoors'. It can only change when you pass thru a door.
 ***********************************************************************/
 
function indoorOutdoorRequiresDoor(maps) {
  for (let i=0; i<maps.length; i++) {
    const map = maps[i];
    if (!map) continue;
    const check1 = (id) => {
      if (!id) return;
      const other = maps[id];
      if ((map.flags & 2) !== (other.flags & 2)) {
        throw new Error(`map:${i} and map:${id} are cardinal neighbors but one is indoors and the other not`);
      }
    };
    check1(map.neighborw);
    check1(map.neighbore);
    check1(map.neighborn);
    check1(map.neighbors);
  }
  return 0;
}
 
/* Map decode and dispatch.
 ************************************************************************/
 
function decodeMap(src) {
  let teleportable = false;
  let songId = 0;
  let neighborw = 0;
  let neighbore = 0;
  let neighborn = 0;
  let neighbors = 0;
  let imageId = 0;
  let flags = 0;
  let spellId = 0;
  let saveTo = 0;
  const doorMapIds = [];
  const sprites = []; // {x,y,id,argv[3]}
  for (let srcp=20*12; srcp<src.length; ) {
    const lead = src[srcp++];
    if (!lead) break;
    let paylen = 0;
         if (lead < 0x20) paylen = 0;
    else if (lead < 0x40) paylen = 1;
    else if (lead < 0x60) paylen = 2;
    else if (lead < 0x80) paylen = 4;
    else if (lead < 0xa0) paylen = 6;
    else if (lead < 0xc0) paylen = 8;
    else if (lead < 0xe0) paylen = src[srcp++];
    switch (lead) {
      case 0x20: songId = src[srcp]; break;
      case 0x21: imageId = src[srcp]; break;
      case 0x22: saveTo = src[srcp]; break;
      case 0x24: flags = src[srcp]; break;
      case 0x40: neighborw = (src[srcp]<<8) | src[srcp+1]; break;
      case 0x41: neighbore = (src[srcp]<<8) | src[srcp+1]; break;
      case 0x42: neighborn = (src[srcp]<<8) | src[srcp+1]; break;
      case 0x43: neighbors = (src[srcp]<<8) | src[srcp+1]; break;
      case 0x45: if (src[srcp+1]) { teleportable = true; spellId = src[srcp+1]; } break;
      case 0x60: doorMapIds.push((src[srcp+1]<<8) | src[srcp+2]); break;
      case 0x80: sprites.push({
          x: src[srcp] % 20,
          y: Math.floor(src[srcp] / 20),
          id: (src[srcp + 1] << 8) | src[srcp + 2],
          argv: [src[srcp + 3], src[srcp + 4], src[srcp + 5]],
        }); break;
      case 0x81: doorMapIds.push((src[srcp+3]<<8) | src[srcp+4]); break;
    }
    srcp += paylen;
  }
  return {
    serial: src,
    songId,
    imageId,
    flags,
    teleportable,
    spellId,
    saveTo,
    neighborw, neighbore, neighborn, neighbors,
    doorMapIds,
    sprites,
  };
}
 
module.exports = function(resources) {
  let warningCount = 0;
  const mapsById = [];
  for (const { type, id, serial } of resources) {
    if (type !== 3) continue;
    const map = decodeMap(serial, id);
    mapsById[id] = map;
  }
  
  warningCount += validateDeterministicSavePoints(mapsById, resources);
  warningCount += validateDeterministicSongSelection(mapsById);
  warningCount += requireBlowbackForOpenEdges(mapsById, resources);
  warningCount += validateSprites(mapsById, resources);
  warningCount += indoorOutdoorRequiresDoor(mapsById);
  // Lots of other things we'll want to check. This is the spot.
  
  return warningCount;
};
