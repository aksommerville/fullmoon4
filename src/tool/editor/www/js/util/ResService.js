/* ResService.js
 * Coordinates loading of resource TOCs and content.
 */
 
import { MapService } from "/js/map/MapService.js";
 
export class ResService {
  static getDependencies() {
    return [Window, MapService];
  }
  constructor(window, mapService) {
    this.window = window;
    this.mapService = mapService;
    
    this.types = ["image", "map", "song", "string"];
    this.toc = []; // {type, id, name, q, lang, path, serial, object}
    
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
  
  getResourceObject(type, id) {
    const res = this.toc.find(r => r.type === type && r.id === id);
    if (res) return res.object;
    return null;
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
      this.broadcast({ type: "loaded" });
      return this.toc;
    }).catch((e) => {
      console.error(`failed to fetch resource toc`, e);
      this.broadcast({ type: "loadError", error: e });
    });
  }
  
  fetchDecodeAndStore(type, base) {
    let load;
    switch (type) {
      case "image": return this.loadImage(base);
      case "string": return this.loadStrings(base);
      case "map": return this.loadText(type, base, (src, id) => this.mapService.decode(src, id));
      case "song": return this.loadTocOnly(type, base);
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
    const id = +base;
    if (isNaN(id) || (id < 1) || (id > 65535)) {
      throw new Error(`Invalid ${type} ID '${base}'`);
    }
    const path = `/data/${type}/${base}`;
    return this.window.fetch(path).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.text();
    }).then(serial => {
      const object = decode(serial, id);
      return { object, serial, id, path, type };
    }).then(res => {
      this.toc.push(res);
    });
  }
  
  loadTocOnly(type, base) {
    const path = `/data/${type}/${base}`;
    const id = +base.split('.')[0];
    if (isNaN(id) || (id < 1) || (id > 255)) {
      throw new Error(`Invalid ${type} ID '${base}'`);
    }
    this.toc.push({ type, id, path });
    return Promise.resolve();
  }
  
  loadUnknown(type, base) {
    const path = `/data/${type}/${base}`;
    const id = +base.split('.')[0];
    if (isNaN(id) || (id < 1) || (id > 255)) {
      throw new Error(`Invalid ${type} ID '${base}'`);
    }
    return this.window.fetch(path).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.arrayBuffer();
    }).then(serial => {
      const res = { type, id, path, serial };
      this.toc.push(res);
    });
  }
  
  /* Save.
   *********************************************************/
  
  saveAll() {
    return Promise.resolve();//TODO dirty debounce
  }
}

ResService.singleton = true;
