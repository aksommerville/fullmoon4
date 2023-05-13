/* ResService.js
 * Coordinates loading of resource TOCs and content.
 */
 
import { MapService } from "/js/map/MapService.js";
import { ImageService } from "/js/image/ImageService.js";
import { SpriteService } from "/js/sprite/SpriteService.js";
 
export class ResService {
  static getDependencies() {
    return [Window, MapService, ImageService, SpriteService];
  }
  constructor(window, mapService, imageService, spriteService) {
    this.window = window;
    this.mapService = mapService;
    this.imageService = imageService;
    this.spriteService = spriteService;
    
    this.DIRTY_DEBOUNCE_TIME = 5000;
    
    this.types = ["image", "tileprops", "map-demo", "map-full", "song", "string", "sprite", "chalk"];
    this.toc = []; // {type, id, name, q, lang, path, serial, object}
    this.dirties = []; // {type, id} The named TOC entries should have a fresh (object) and no (serial).
    this.dirtyDebounce = null;
    this.mapSet = "-demo"; // "-demo" or "-full" //TODO Add UI for toggling this.
    
    this.reloadAll();
    
    this.listeners = []; // {id,cb} for general events
    this.nextListenerId = 1;
  }
  
  whenLoaded() {
    if (this.toc.length) return Promise.resolve(this.toc);
    return new Promise((resolve, reject) => {
      const listenerId = this.listen(event => {
        if (event.type === "loaded") {
          this.unlisten(listenerId);
          resolve(this.toc);
        } else if (event.type === "loadError") {
          this.unlisten(listenerId);
          reject(event.error);
        }
      });
    });
  }
  
  /* cb is called with {
   *   type: "loaded"
   * } or {
   *   type: "loadError"
   *   error: any
   * } or {
   *   type: "dirty"
   * } or {
   *   type: "saving"
   * } or {
   *   type: "saveError"
   *   error: any
   * } or {
   *   type: "saved"
   * }
   */
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({ id, cb });
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    for (const { id, cb } of this.listeners) {
      cb(event);
    }
  }
  
  getResourceObject(type, idOrName) {
    if (type === "map") type += this.mapSet;
    if (typeof(idOrName) === "string") {
      const res = this.toc.find(r => r.type === type && r.name === idOrName);
      if (res) return res.object;
      idOrName = +idOrName;
    }
    const res = this.toc.find(r => r.type === type && r.id === idOrName);
    if (res) return res.object;
    return null;
  }
  
  getResourceName(type, id) {
    if (type === "map") type += this.mapSet;
    const res = this.toc.find(r => r.type === type && r.id === id);
    if (!res) return "";
    return res.name || "";
  }
  
  /* Fetch.
   *************************************************************/
  
  reloadAll() {
    return this.saveAll().then(() => {
      this.toc = [];
      return Promise.all(this.types.map(t => this.window.fetch(`/data/${t}`).then(rsp => {
        if (!rsp.ok) {
          if (rsp.status === 404) return [];
          throw rsp;
        }
        return rsp.json();
      })));
    }).then((content) => {
      const promises = [];
      for (let i=0; i<this.types.length; i++) {
        for (const base of content[i]) {
          promises.push(this.fetchDecodeAndStore(this.types[i], base));
        }
      }
      return Promise.all(promises);
    }).then(() => {
      this.sortToc();
      this.broadcast({ type: "loaded" });
      return this.toc;
    }).catch((e) => {
      console.error(`failed to fetch resource toc`, e);
      this.broadcast({ type: "loadError", error: e });
    });
  }
  
  sortToc() {
    this.toc.sort((a, b) => {
      if (a.type < b.type) return -1;
      if (a.type > b.type) return 1;
      return a.id - b.id;
    });
  }
  
  fetchDecodeAndStore(type, base) {
    let load;
    switch (type) {
      case "image": return this.loadImage(base);
      case "tileprops": return this.loadText(type, base, (src, id) => this.imageService.decodeTileprops(src, id));
      case "string": return this.loadStrings(base);
      case "map": return this.loadText(type, base, (src, id) => this.mapService.decode(src, id));
      case "map-full": return this.loadText(type, base, (src, id) => this.mapService.decode(src, id));
      case "map-demo": return this.loadText(type, base, (src, id) => this.mapService.decode(src, id));
      case "song": return this.loadTocOnly(type, base);
      case "sprite": return this.loadText(type, base, (src, id) => this.spriteService.decode(src, id));
      case "chalk": return this.loadText(type, base, (src, id) => src);
      default: return this.loadUnknown(type, base);
    }
  }
  
  loadImage(base) {
    const path = `/data/image/${base}`;
    const { id, q, lang, name } = this.parseImageBasename(base);
    return new Promise((resolve, reject) => {
      const image = new Image();
      image.onload = () => resolve(image);
      image.onerror = (e) => reject(e);
      image.src = path;
    }).then(image => {
      this.toc.push({
        type: "image",
        id,
        name,
        q,
        lang,
        path,
        object: image,
      });
    });
  }
  
  // => {id, q, lang, name}
  parseImageBasename(base) {
    // For now all image names are just "ID.png", and in fact our build process won't allow anything else.
    // That will change eventually, and they will be "ID[-name][.qualifier][.lang].png"
    let id = 0, name = "", q = "", lang = "";
    const dotDivisions = base.split(".");
    if ((dotDivisions.length < 2) || (dotDivisions.length > 4)) {
      throw new Error(`Invalid image name '${base}'`);
    }
    const nameIntroducer = dotDivisions[0].indexOf("-");
    if (nameIntroducer >= 0) {
      id = +dotDivisions[0].substring(0, nameIntroducer);
      name = dotDivisions[0].substring(nameIntroducer + 1);
    } else {
      id = +dotDivisions[0];
    }
    if (isNaN(id) || (id < 1) || (id > 255)) {
      throw new Error(`Invalid image name '${base}'`);
    }
    if (dotDivisions.length === 4) {
      q = dotDivisions[1];
      lang = dotDivisions[2];
    } else if (dotDivisions.length === 3) {
      if (dotDivisions[1].length === 2) {
        lang = dotDivisions[1];
      } else {
        q = dotDivisions[1];
      }
    }
    return { id, q, lang, name };
  }
  
  loadStrings(base) {
    const lang = base;
    const path = `/data/string/${base}`;
    return this.window.fetch(path).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.text();
    }).then(text => this.loadStringsText(text, lang, path));
  }
  
  loadStringsText(text, lang, path) {
    let lineno = 0, textp = 0;
    while (textp < text.length) {
      lineno++;
      let nlp = text.indexOf("\n", textp);
      if (nlp < 0) nlp = text.length;
      const line = text.substring(textp, nlp);
      textp = nlp + 1;
      const words = line.split(/\s+/).filter(v => v);
      if (words.length < 1) continue;
      const id = +words[0];
      if (isNaN(id) || (id < 1) || (id > 65535)) {
        throw new Error(`${path}:${lineno}: Invalid string ID ${id}`);
      }
      const serial = words.slice(1).join(" ");
      this.toc.push({
        type: "string",
        id,
        lang,
        path,
        serial,
      });
    }
  }
  
  loadText(type, base, decode) {
    const id = +base.split("-")[0];
    if (isNaN(id) || (id < 1) || (id > 65535)) {
      throw new Error(`Invalid ${type} ID '${base}'`);
    }
    const name = (base.match(/^\d+-([^.]*)/) || [])[1] || "";
    const path = `/data/${type}/${base}`;
    return this.window.fetch(path).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.text();
    }).then(serial => {
      const object = decode(serial, id);
      return { object, serial, id, path, type, name };
    }).then(res => {
      this.toc.push(res);
    });
  }
  
  loadTocOnly(type, base) {
    const path = `/data/${type}/${base}`;
    const id = +base.split(/[-\.]/)[0];
    if (isNaN(id) || (id < 1) || (id > 255)) {
      throw new Error(`Invalid ${type} ID '${base}'`);
    }
    const name = (base.match(/^\d+-([^.]*)/) || [])[1] || "";
    this.toc.push({ type, id, path, name });
    return Promise.resolve();
  }
  
  loadUnknown(type, base) {
    const path = `/data/${type}/${base}`;
    const id = +base.split(/[-\.]/)[0];
    if (isNaN(id) || (id < 1) || (id > 255)) {
      throw new Error(`Invalid ${type} ID '${base}'`);
    }
    const name = (base.match(/^\d+-([^.]*)/) || [])[1] || "";
    return this.window.fetch(path).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.arrayBuffer();
    }).then(serial => {
      const res = { type, id, path, serial, name };
      this.toc.push(res);
    });
  }
  
  /* Save.
   *********************************************************/
   
  generatePathForNewResource(type, id, name) {
    if (name) return `/data/${type}/${id}-${name}`;
    return `/data/${type}/${id}`;
  }
   
  dirty(type, id, object, name) {
    // TODO Can we use this same function for deleting? Maybe with (object===null)?
    if (type === "map") type += this.mapSet;
    let res = this.toc.find(r => r.type === type && r.id === id);
    if (!res) {
      res = {
        type, id,
        path: this.generatePathForNewResource(type, id, name),
      };
      this.toc.push(res);
    }
    if (name) res.name = name;
    res.serial = null;
    res.object = object;
    if (!this.dirties.find(d => d.type === type && d.id === id)) {
      this.dirties.push({ type, id });
    }
    if (!this.dirtyDebounce) {
      this.broadcast({ type: "dirty" });
      this.dirtyDebounce = this.window.setTimeout(() => {
        this.dirtyDebounce = null;
        this.saveAll().then(() => {
        }).catch(() => {
          // We don't need to 'dirty' it again, but we do need to trigger this setTimeout again.
          this.dirty(type, id, object);
        });
      }, this.DIRTY_DEBOUNCE_TIME);
    }
  }
  
  saveAll() {
    if (!this.dirties.length) return Promise.resolve();
    const dirties = this.dirties;
    this.dirties = [];
    const promises = [];
    
    for (let i=dirties.length; i-->0; ) {
      const { type, id } = dirties[i];
      promises.push(new Promise((resolve, reject) => {
        const res = this.toc.find(r => r.type === type && r.id === id);
        if (!res) return reject(`Resource ${type}:${id} disappeared from TOC between dirty and save.`);
        if (!res.serial) {
          if (!res.object) return reject(`Resource ${type}:${id} has no object in TOC.`);
          if (typeof(res.object) === "string") {
            res.serial = res.object;
          } else {
            if (!res.object.encode) return reject(`Unable to encode resource ${type}:${id}.`);
            if (!(res.serial = res.object.encode())) return reject(`Failed to encode resource ${type}:${id}`);
          }
        }
        this.window.fetch(res.path, { method: "PUT", body: res.serial })
          .then(rsp => { if (!rsp.ok) throw rsp; return rsp.text(); })
          .then((rsp) => {
            const p = dirties.findIndex(d => d.type === type && d.id === id);
            if (p >= 0) dirties.splice(p, 1);
            resolve();
          })
          .catch(reject);
      }));
    }
    
    return Promise.all(promises)
      .then(() => {
        // We could, and often do, collect new dirties during the save.
        // That's fine. But don't report "saved", since that's not really our state now.
        if (!this.dirties.length) this.broadcast({ type: "saved" });
      })
      .catch((e) => {
        this.broadcast({ type: "saveError", error: e });
        // Anything we haven't saved yet, put it back in the dirty queue.
        for (const { type, id } of dirties) {
          const res = this.toc.find(r => r.type === type && r.id === id);
          const object = res ? res.object : null;
          this.dirty(type, id, object);
        }
        throw e;
      });
  }
  
  /* Odds, ends.
   ***************************************************/
   
  unusedId(type) {
    if (type === "map") type += this.mapSet;
    const used = [];
    let hi = 0;
    for (const res of this.toc) {
      if (res.type !== type) continue;
      used.push(res.id);
      if (res.id > hi) hi = res.id;
    }
    if (used.length < 1) return 1;
    if (hi === used.length) return hi + 1; // they are contiguous from 1; return the next.
    // find the gap...
    for (let id=1; ; id++) {
      if (used.indexOf(id) < 0) return id;
    }
  }
  
  resolveId(type, src) {
    if (type === "map") type += this.mapSet;
    let id = +src;
    if (!isNaN(id)) return id;
    const r = this.toc.find(e => e.type === type && e.name === src);
    if (r) return r.id;
    throw new Error(`Unable to resolve resource id ${type}:${src}`);
  }
}

ResService.singleton = true;
