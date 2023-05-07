/* WorldMap.js
 * All maps laid out 2-dimensionally.
 * This is two classes: WorldMap, a single composition, and WorldMapGenerator for generating them.
 * Plenty of opportunity for optimization in generating these maps, it's pretty wasteful.
 */
 
export class WorldMapGenerator {
  static getDependencies() {
    return [];
  }
  constructor() {
  }
  
  /* Given a TOC of all map resources, generate a WorldMap containing all of them.
   * Maps with no neighbors will be islands; an edge or empty cell on all sides.
   * The returned map is guaranteed to contain exactly all of the maps in TOC.
   * If neighbor links are inconsistent we'll do something, but exactly what is undefined.
   * See ResService for TOC shape.
   */
  generateWorldMap(toc) {
    if (!toc.length) return new WorldMap(0, 0);
    if (toc.find(res => (!res.type.startsWith?.("map") || !res.object))) {
      throw new Error(`Invalid TOC for world map generation, expected maps`);
    }
    const pending = [...toc];
    let world = this.generateContiguousMap(pending.pop(), pending);
    while (pending.length) {
      const next = this.generateContiguousMap(pending.pop(), pending);
      world = this.joinMaps(world, next);
    }
    return world;
  }
  
  /* Make a new WorldMap containing (res) and anything from (pending) which is reachable from there.
   * Anything from (pending) that we add to the WorldMap, we remove from (pending).
   */
  generateContiguousMap(res, pending) {
    let world = new WorldMap(20, 20);
    const p = [world.w >> 1, world.h >> 1];
    world.maps[p[1] * world.w + p[0]] = res.object;
    world = this.extendMap(world, res.object, p, "n", pending);
    world = this.extendMap(world, res.object, p, "s", pending);
    world = this.extendMap(world, res.object, p, "w", pending);
    world = this.extendMap(world, res.object, p, "e", pending);
    return this.trim(world);
  }
  
  /* Grow the world recursively, starting from a populated point and direction.
   * (p) is [x,y]. We may change it, if we reallocate the WorldMap.
   * (d) is one of "nswe".
   */
  extendMap(world, map, p, d, pending) {
  
    // It's entirely possible that a prior pass already reached this cell.
    // Or that the map we're looking at has no neighbor in that direction.
    // All good, get out.
    let dx=0, dy=0;
    switch (d) {
      case "n": dy = -1; break;
      case "s": dy = 1; break;
      case "w": dx = -1; break;
      case "e": dx = 1; break;
      default: return world;
    }
    let nx = p[0] + dx;
    let ny = p[1] + dy;
    if ((nx >= 0) && (nx < world.w) && (ny >= 0) && (ny < world.h) && world.maps[ny * world.w + nx]) return world;
    const nextId = map.getNeighborId(d);
    if (!nextId) return world;
    const resix = pending.findIndex(r => r.id === nextId);
    if (resix < 0) return world;
    const nextMap = pending[resix].object;
    pending.splice(resix, 1);
    
    // If it crossed a boundary, grow in that direction.
    if (nx < 0) {
      world = this.growMap(world, -10, 0);
      p[0] += 10;
      nx += 10;
    }
    if (nx >= world.w) {
      world = this.growMap(world, 10, 0);
    }
    if (ny < 0) {
      world = this.growMap(world, 0, -10);
      p[1] += 10;
      ny += 10;
    }
    if (ny >= world.h) {
      world = this.growMap(world, 0, 10);
    }
    
    // Assign to world, then enter neighbors.
    const nextp = [nx, ny];
    world.maps[ny * world.w + nx] = nextMap;
    world = this.extendMap(world, nextMap, nextp, "n", pending);
    world = this.extendMap(world, nextMap, nextp, "s", pending);
    world = this.extendMap(world, nextMap, nextp, "w", pending);
    world = this.extendMap(world, nextMap, nextp, "e", pending);
    
    return world;
  }
  
  trim(world) {
    let n = 0, s = 0, w = 0, e = 0;
    while ((n < world.h) && !this.anyMapInRect(world, 0, n, world.w, 1)) n++;
    while ((n + s < world.h) && !this.anyMapInRect(world, 0, world.h - s - 1, world.w, 1)) s++;
    while ((w < world.w) && !this.anyMapInRect(world, w, 0, 1, world.h)) w++;
    while ((w + e < world.w) && !this.anyMapInRect(world, world.w - e - 1, 0, 1, world.h)) e++;
    if (!(n + s + w + e)) return world;
    const dst = new WorldMap(world.w - w - e, world.h - n - s);
    for (let y=0, dstp=0; y<dst.h; y++) {
      for (let x=0; x<dst.w; x++, dstp++) {
        dst.maps[dstp] = world.maps[(y + n) * world.w + x + w];
      }
    }
    return dst;
  }
  
  anyMapInRect(world, x, y, w, h) {
    for (; h-->0; y++) {
      if (y < 0) continue;
      if (y >= world.h) break;
      for (let i=w, p=y*world.w+x; i-->0; p++) {
        const xx = x + i;
        if (xx < 0) continue;
        if (xx >= world.w) break;
        if (world.maps[p]) return world.maps[p];
      }
    }
    return null;
  }
  
  /* Return a WorldMap containing both (a) and (b), with a margin between them.
   * We might create a new one, or might insert one into the other.
   */
  joinMaps(a, b) {
    // If either is empty, return the other. This shouldn't happen but whatever.
    if (!a.w && !a.h) return b;
    if (!b.w && !b.h) return a;
    // If one is at least 2 units smaller than the other on both axes, try to insert.
    if ((a.w <= b.w - 2) && (a.h <= b.h - 2) && this.insertMap(b, a)) return b;
    if ((b.w <= a.w - 2) && (b.h <= a.h - 2) && this.insertMap(a, b)) return a;
    // Concatenate side-by-side with inner margin, whichever direction is shorter.
    const sumw = a.w + 1 + b.w;
    const sumh = a.h + 1 + b.h;
    if (sumw <= sumh) {
      const dst = new WorldMap(sumw, Math.max(a.h, b.h));
      this.copyMap(dst, 0, 0, a);
      this.copyMap(dst, a.w + 1, 0, b);
      return dst;
    } else {
      const dst = new WorldMap(Math.max(a.w, b.w), sumh);
      this.copyMap(dst, 0, 0, a);
      this.copyMap(dst, 0, a.h + 1, b);
      return dst;
    }
  }
  
  /* Find a place in (big) where (lil) can be copied preserving its margin.
   * If possible, perform the copy and return true.
   * Otherwise, do nothing and return false.
   * We don't try to jigsaw the edges together; we treat (lil) as full no matter what.
   */
  insertMap(big, lil) {
    // Search the corners first. We don't need a margin on the edge sides.
    if (!this.anyMapInRect(big, 0, 0, lil.w + 1, lil.h + 1)) {
      this.copyMap(big, 0, 0, lil);
      return true;
    }
    if (!this.anyMapInRect(big, big.w - lil.w - 1, 0, lil.w + 1, lil.h + 1)) {
      this.copyMap(big, big.w - lil.w, 0, lil);
      return true;
    }
    if (!this.anyMapInRect(big, 0, big.h - lil.h - 1, lil.w + 1, lil.h + 1)) {
      this.copyMap(big, 0, big.h - lil.h, lil);
      return true;
    }
    if (!this.anyMapInRect(big, big.w - lil.w - 1, big.h - lil.h - 1, lil.w + 1, lil.h + 1)) {
      this.copyMap(big, big.w - lil.w, big.h - lil.h, lil);
      return true;
    }
    // Now the edges, inside of corners.
    const trimw = big.w - lil.w - 2;
    const trimh = big.h - lil.h - 2;
    for (let x=0; x<trimw; x++) {
      if (!this.anyMapInRect(big, x, 0, lil.w + 2, lil.h + 1)) {
        this.copyMap(big, x + 1, 0, lil);
        return true;
      }
      if (!this.anyMapInRect(big, x, big.h - lil.h - 1, lil.w + 2, lil.h + 1)) {
        this.copyMap(big, x + 1, big.h - lil.h, lil);
        return true;
      }
    }
    for (let y=0; y<trimh; y++) {
      if (!this.anyMapInRect(big, 0, y, lil.w + 1, lil.h + 2)) {
        this.copyMap(big, 0, y + 1, lil);
        return true;
      }
      if (!this.anyMapInRect(big, big.w - lil.w - 1, y, lil.w + 1, lil.h + 2)) {
        this.copyMap(big, big.w - lil.w, y + 1, lil);
      }
    }
    // Finally scan the interior. We can start at 2: If there wasn't space at 0, there can't be at 1 either.
    for (let x=0; x<trimw; x++) {
      for (let y=0; y<trimh; y++) {
        if (this.anyMapInRect(big, x, y, lil.w + 2, lil.h + 2)) continue;
        this.copyMap(big, x + 1, y + 1, lil);
        return true;
      }
    }
    return false;
  }
  
  copyMap(dst, dstx, dsty, src) {
    if (
      (dstx < 0) || (dstx + src.w > dst.w) ||
      (dsty < 0) || (dsty + src.h > dst.h)
    ) throw new Error(`Invalid map copy of (${src.w},${src.h}) input to (${dstx},${dsty}) in (${dst.w},${dst.h}) output`);
    for (let yi=src.h, srcp=0; yi-->0; dsty++) {
      let dstp = dsty * dst.w + dstx;
      for (let xi=src.w; xi-->0; dstp++, srcp++) {
        dst.maps[dstp] = src.maps[srcp];
      }
    }
  }
}

WorldMapGenerator.singleton = true;

export class WorldMap {
  constructor(w, h) {
    this.w = w;
    this.h = h;
    this.maps = []; // FullmoonMap|null, LRTB
    for (let i=w*h; i-->0; ) this.maps.push(null);
  }
}
