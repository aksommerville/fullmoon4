/* ImageUi.js
 * Top-level controller for "image" and "tileprops" resources.
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { Tileprops } from "/js/image/Tileprops.js";
import { TilepropsModal } from "/js/image/TilepropsModal.js";

export class ImageUi {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, "discriminator", Window];
  }
  constructor(element, dom, resService, discriminator, window) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.discriminator = discriminator;
    this.window = window;
    
    // Owner should set:
    this.setTattleText = (text) => {};
    
    this.image = null;
    this.tileprops = null;
    this.renderTimeout = null;
    this.gridBounds = { x: 0, y: 0, w: 1, h: 1 };
    
    this.directionsImage = new Image();
    this.directionsImage.addEventListener("load", () => this.renderSoon());
    this.directionsImage.src = "/img/directions.png";
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    if (this.renderTimeout) this.window.clearTimeout(this.renderTimeout);
  }
  
  setup(id, args) {
    this.image = this.resService.getResourceObject("image", id);
    this.tileprops = this.resService.getResourceObject("tileprops", id);
    if (this.image && !this.tileprops) {
      this.tileprops = new Tileprops();
      this.tileprops.id = id;
      // Don't report this new object to ResService until the user actually changes something.
      // Otherwise we'd create a new resource each time they accidentally open a non-tilesheet image.
    }
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const controlsRow = this.dom.spawn(this.element, "DIV", ["controlsRow"]);
    this.dom.spawn(controlsRow, "INPUT", { type: "button", value: "+", "on-click": () => this.onAddField() });
    
    const visibilityRow = this.dom.spawn(controlsRow, "DIV", ["visibility"], { "on-change": () => this.renderSoon() });
    this.spawnVisibilityToggle(visibilityRow, "image");
    this.spawnVisibilityToggle(visibilityRow, "grid");
    this.spawnVisibilityToggle(visibilityRow, "tileid");
    
    this.dom.spawn(this.element, "CANVAS", ["grid"], { "on-click": (e) => this.onClickCanvas(e) });
  }
  
  spawnVisibilityToggle(parent, name, volatile) {
    const input = this.dom.spawn(parent, "INPUT", {
      type: "checkbox",
      id: `ImageUi-${this.discriminator}-visibility-${name}`,
      checked: "checked",
      name,
    });
    const label = this.dom.spawn(parent, "LABEL", { for: input.id }, name);
    if (volatile) {
      input.classList.add("volatile");
      label.classList.add("volatile");
    }
  }
  
  populateUi() {
    const visibilityRow = this.element.querySelector(".visibility");
    for (const element of visibilityRow.querySelectorAll(".volatile")) {
      element.remove();
    }
    if (this.tileprops) {
      for (const key of Object.keys(this.tileprops.fields)) {
        //TODO We repopulate on adding fields. Try to preserve selections across the rebuild.
        this.spawnVisibilityToggle(visibilityRow, key, true);
      }
    }
    this.renderSoon();
  }
  
  recalculateGridBounds(fullw, fullh) {
    // Square aspect ratios are easy...
    const w = (fullw <= fullh) ? fullw : fullh;
    const h = w;
    const x = (fullw >> 1) - (w >> 1);
    const y = (fullh >> 1) - (h >> 1);
    this.gridBounds = {x, y, w, h};
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, 0);
  }
  
  renderNow() {
    const canvas = this.element.querySelector(".grid");
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
    this.recalculateGridBounds(canvas.width, canvas.height);
    const context = canvas.getContext("2d");
    context.imageSmoothingEnabled = false;
    context.clearRect(0, 0, canvas.width, canvas.height);
    const visibility = this.readVisibilityFromDom();
    
    // "image" and "grid" visibilities are special.
    if (this.image && visibility.image) {
      context.drawImage(
        this.image, 0, 0, this.image.naturalWidth, this.image.naturalHeight,
        this.gridBounds.x, this.gridBounds.y, this.gridBounds.w, this.gridBounds.h
      );
    }
    if (visibility.grid) {
      this.renderGrid(context);
    }
    
    // All other visibilities are rendered as a row within each tile.
    let badgey = 0;
    for (const key of Object.keys(visibility)) {
      if (key === "image") continue;
      if (key === "grid") continue;
      badgey += this.renderField(context, key, badgey);
    }
  }
  
  renderGrid(context) {
    context.beginPath();
    for (let i=0; i<=16; i++) {
      const p = Math.floor((i * this.gridBounds.w) / 16) + 0.5;
      context.moveTo(this.gridBounds.x + p, this.gridBounds.y);
      context.lineTo(this.gridBounds.x + p, this.gridBounds.y + this.gridBounds.h);
      context.moveTo(this.gridBounds.x, this.gridBounds.y + p);
      context.lineTo(this.gridBounds.x + this.gridBounds.w, this.gridBounds.y + p);
    }
    context.strokeStyle = "#fff";
    context.stroke();
  }
  
  // Returns vertical advancement.
  renderField(context, key, suby) {
  
    let src = null;
    if (key === "tileid") src = ImageUi.TILEID_BY_TILEID;
    else if (this.tileprops) src = this.tileprops.fields[key];
    if (!src) return;
    
    const tilesize = this.gridBounds.w / 16;
    const halftilesize = tilesize / 2;
    let render = (x, y, v) => {};
    let advancement = 0;
    switch (key) {
    
      case "tileid": { // text, but hexadecimal instead of decimal.
          render = (x, y, v) => {
            context.fillStyle = "#fff";
            context.globalAlpha = 0.5;
            context.fillRect(x, y, halftilesize, 10);
            context.globalAlpha = 1.0;
            context.fillStyle = "#000";
            context.fillText(v.toString(16).padStart(2, '0'), x, y + 10);
          };
          advancement = 10;
        } break;
        
      case "physics": {
          render = (x, y, v) => {
            switch (v) { // FMN_CELLPHYSICS
              case 0: context.fillStyle = "#0f0"; break; // vacant, green
              case 1: context.fillStyle = "#000"; break; // solid, black
              case 2: context.fillStyle = "#008"; break; // hole, blue
              case 3: context.fillStyle = "#080"; break; // unshovellable, basically vacant
              case 4: context.fillStyle = "#446"; break; // unchalkable, basically solid
              case 5: context.fillStyle = "#ff0"; break; // sap
              case 6: context.fillStyle = "#f80"; break; // sap, no chalk
              case 7: context.fillStyle = "#00f"; break; // water
              default: context.fillStyle = "#f00"; break; // unknown, red
              // It's entirely possible we'll add more, but I expect simple colors will work forever.
            }
            context.globalAlpha = 0.7;
            context.fillRect(x + halftilesize, y - suby, halftilesize, halftilesize);
            context.globalAlpha = 1.0;
          };
          advancement = 0;
        } break;
        
      case "neighbors": if (this.directionsImage.complete) {
          const sw = this.directionsImage.naturalWidth / 3;
          const sh = this.directionsImage.naturalHeight / 3;
          const render1 = (x, y, sx, sy) => {
            x += tilesize - this.directionsImage.naturalWidth + sx * sw;
            y += tilesize - suby - this.directionsImage.naturalHeight + sy * sh;
            context.drawImage(this.directionsImage, sx * sw, sy * sh, sw, sh, x, y, sw, sh);
          };
          render = (x, y, v) => {
            if (!v) return;
            if (v & 0x80) render1(x, y, 0, 0);
            if (v & 0x40) render1(x, y, 1, 0);
            if (v & 0x20) render1(x, y, 2, 0);
            if (v & 0x10) render1(x, y, 0, 1);
            if (v & 0x08) render1(x, y, 2, 1);
            if (v & 0x04) render1(x, y, 0, 2);
            if (v & 0x02) render1(x, y, 1, 2);
            if (v & 0x01) render1(x, y, 2, 2);
          };
          advancement = 0;
          break;
        } // else pass:
        
      default: { // regular decimal text
          render = (x, y, v) => {
            context.fillStyle = "#fff";
            context.globalAlpha = 0.5;
            context.fillRect(x, y, halftilesize, 10);
            context.globalAlpha = 1.0;
            context.fillStyle = "#000";
            context.fillText(v.toString(), x, y + 10);
          };
          advancement = 10;
        }
    }
  
    for (let dsty=this.gridBounds.y + suby, row=0, p=0; row<16; row++, dsty+=tilesize) {
      for (let dstx=this.gridBounds.x, col=0; col<16; col++, dstx+=tilesize, p++) {
        render(Math.floor(dstx), Math.floor(dsty), src[p]);
      }
    }
    return advancement;
  }
  
  readVisibilityFromDom() {
    return Array.from(this.element.querySelectorAll(".visibility input:checked"))
      .map(e => e.name)
      .reduce((a, v) => { a[v] = true; return a; }, {});
  }
  
  onAddField() {
    if (!this.tileprops) return;
    const suggest = Tileprops.IMPLICIT_FIELD_ORDER
      .filter(s => !this.tileprops.fields[s]);
    const prompt = suggest.length
      ? `${suggest.join(', ')}, or make one up:`
      : "All known fields are already defined, but you can make one up:";
    const key = this.window.prompt(prompt);
    if (!key) return;
    if (!key.match(/^[0-9a-zA-Z_]{1,30}$/)) {
      this.window.alert(`Invalid field name ${JSON.stringify(response)}`);
      return;
    }
    this.tileprops.fields[key] = new Uint8Array(256);
    
    // Most tables can take zero as the default value, but "weight" should be straight 255s.
    if (key === "weight") {
      const vv = this.tileprops.fields[key];
      for (let i=0; i<256; i++) vv[i] = 255;
    }
    
    this.resService.dirty("tileprops", this.tileprops.id, this.tileprops);
    this.populateUi();
  }
  
  onClickCanvas(event) {
    if (!this.tileprops) return;
    const canvasBounds = this.element.querySelector(".grid").getBoundingClientRect();
    const col = Math.floor(((event.clientX - canvasBounds.x - this.gridBounds.x) * 16) / this.gridBounds.w);
    const row = Math.floor(((event.clientY - canvasBounds.y - this.gridBounds.y) * 16) / this.gridBounds.h);
    if ((col < 0) || (col > 15) || (row < 0) || (row > 15)) return;
    const tileid = (row << 4) | col;
    const modal = this.dom.spawnModal(TilepropsModal);
    modal.setup(this.tileprops, tileid);
    modal.onDirty = () => {
      this.resService.dirty("tileprops", this.tileprops.id, this.tileprops);
      this.renderSoon();
    };
  }
}

ImageUi.TILEID_BY_TILEID = [
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
  48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
  64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
  80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
  96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,
  112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
  128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
  144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
  160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
  176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
];
