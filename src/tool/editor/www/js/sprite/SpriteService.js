/* SpriteService.js
 */
 
import { Sprite } from "./Sprite.js";
 
export class SpriteService {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.controllerNames = null;
  }
  
  decode(src, id) {
    const sprite = new Sprite(src);
    sprite.id = id;
    return sprite;
  }
  
  generateNewSprite() {
    const sprite = new Sprite();
    sprite.id = 0;
    return sprite;
  }
  
  getFieldCommentForDisplay(command) {
    // Originally these were going to like analyze the command's payload.
    // But I think it's actually more useful to use this static help text.
    switch (command[0]) {
      case "controller": return "FMN_SPRCTL_* from src/app/sprite/fmn_sprite.h";
      case "image": return "Image ID";
      case "tile": return "0..255";
      case "xform": return "Multi: xrev, yrev, swap";
      case "style": return "One: hidden, tile, hero, fourframe, firenozzle";
      case "physics": return "Multi: motion, edge, sprites, solid, hole";
      case "decay": return "0.0 <= n < 256.0";
      case "radius": return "0.0 <= n < 256.0";
      case "invmass": return "0=infinite, 1=heavy, 255=light";
      case "layer": return "hero=128";
    }
    return "";
  }
  
  keyIsValid(key) {
    if ([
      "controller",
      "image",
      "tile",
      "xform",
      "style",
      "physics",
      "decay",
      "radius",
      "invmass",
      "layer",
    ].indexOf(key) >= 0) return true;
    // Any integer 1..255 is also valid.
    const v = +key;
    if (isNaN(v)) return false;
    if (v < 1) return false;
    if (v > 0xff) return false;
    if (v % 1) return false;
    return true;
  }
  
  generatePreview(canvas, sprite, resService) {
    if (!canvas) return;
    canvas.width = 16;
    canvas.height = 16;
    const context = canvas.getContext("2d");
    context.clearRect(0, 0, canvas.width, canvas.height);
    if (!sprite || !resService) return;
    const imageId = +sprite.getCommand("image");
    if (!imageId) return;
    const image = resService.getResourceObject("image", imageId);
    if (!image) return;
    const tileId = +sprite.getCommand("tile");
    if (isNaN(tileId)) return;
    const xform = +sprite.getCommand("xform") || 0;
    const tilesize = image.naturalWidth >> 4;
    canvas.width = tilesize;
    canvas.height = tilesize;
    const srcx = (tileId & 15) * tilesize;
    const srcy = (tileId >> 4) * tilesize;
    context.drawImage(image, srcx, srcy, tilesize, tilesize, 0, 0, tilesize, tilesize);
  }
  
  // Calls (cb) with each name, not necessarily synchronously.
  // Returns a Promise that resolves after all callbacks are complete.
  listControllers(cb) {
    if (this.controllerNames) {
      for (const name of this.controllerNames) cb(name);
      return Promise.resolve();
    }
    return this.window.fetch("/api/spriteControllers")
      .then(rsp => { if (!rsp.ok) throw rsp; return rsp.json(); })
      .then(controllers => {
        this.controllerNames = controllers.map(c => c.name).filter(v => v);
        for (const name of this.controllerNames) cb(name);
      });
  }
}

SpriteService.singleton = true;
