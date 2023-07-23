/* TilepropsModal.js
 * Triggered when you click on one tile in ImageUi.
 * We can edit the whole sheet, we have a navigation widget and preset buttons.
 * But we never add or remove fields -- that's the parent's concern.
 */
 
import { Dom } from "/js/util/Dom.js";
import { Tileprops } from "/js/image/Tileprops.js";
import { ResService } from "/js/util/ResService.js";
import { ImageService } from "/js/image/ImageService.js";

export class TilepropsModal {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, Window];
  }
  constructor(element, dom, resService, window) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.window = window;
    
    // Owner should set. We will modify the provided object directly, and notify you right after.
    this.onDirty = () => {};
    
    this.tileprops = null;
    this.tileid = 0;
    this.image = null;
    
    this.buildUi();
  }
  
  setup(tileprops, tileid) {
    this.tileprops = tileprops;
    this.tileid = tileid;
    if (this.tileprops) this.image = this.resService.getResourceObject("image", this.tileprops.id);
    else this.image = null;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
     
    const header = this.dom.spawn(this.element, "DIV", ["header"]);
    this.dom.spawn(header, "DIV", ["tileid"]);
    this.dom.spawn(header, "CANVAS", ["thumbnail"]);
    {
      const navigationTable = this.dom.spawn(header, "TABLE", ["navigation"]);
      let tr = this.dom.spawn(navigationTable, "TR");
      this.dom.spawn(tr, "TD");
      this.dom.spawn(this.dom.spawn(tr, "TD"), "INPUT", { type: "button", value: "^", "on-click": () => this.onNavigation(0, -1) });
      tr = this.dom.spawn(navigationTable, "TR");
      this.dom.spawn(this.dom.spawn(tr, "TD"), "INPUT", { type: "button", value: "<", "on-click": () => this.onNavigation(-1, 0) });
      this.dom.spawn(tr, "TD");
      this.dom.spawn(this.dom.spawn(tr, "TD"), "INPUT", { type: "button", value: ">", "on-click": () => this.onNavigation(1, 0) });
      tr = this.dom.spawn(navigationTable, "TR");
      this.dom.spawn(tr, "TD");
      this.dom.spawn(this.dom.spawn(tr, "TD"), "INPUT", { type: "button", value: "v", "on-click": () => this.onNavigation(0, 1) });
    }
    
    const body = this.dom.spawn(this.element, "DIV", ["body"]);
    const presets = this.dom.spawn(body, "DIV", ["presets"]);
    const fields = this.dom.spawn(body, "TABLE", ["fields"], { "on-input": (e) => this.onInput(e) });
    
    const tilesize = 16;
    for (const preset of ImageService.PRESETS) {
      const button = this.dom.spawn(presets, "BUTTON", { name: preset.name, "on-click": () => this.onApplyPreset(preset) });
      this.dom.spawn(button, "CANVAS", ["preview"], { width: preset.w * tilesize, height: preset.h * tilesize });
      this.dom.spawn(button, "LABEL", preset.name);
    }
  }
  
  populateUi() {
    if (!this.tileprops) return; // not realistic, just leave it
    
    this.element.querySelector(".tileid").innerText = this.tileid.toString(16).padStart(2, '0');
    this.renderThumbnail();
    
    const x = this.tileid & 15;
    const y = this.tileid >> 4;
    for (const button of this.element.querySelectorAll(".presets button")) {
      const schema = ImageService.PRESETS.find(p => p.name === button.name);
      if (!schema) {
        button.disabled = true;
        this.renderUnavailablePreset(button);
        continue;
      }
      if ((x + schema.w > 16) || (y + schema.h > 16)) {
        button.disabled = true;
        this.renderUnavailablePreset(button);
      } else {
        button.disabled = false;
        this.renderPreset(button, schema);
      }
    }
    
    const fieldsTable = this.element.querySelector(".fields");
    fieldsTable.innerHTML = "";
    for (const key of Object.keys(this.tileprops.fields)) {
      const tr = this.dom.spawn(fieldsTable, "TR");
      this.dom.spawn(tr, "TD", ["key"], key);
      const tdInput = this.dom.spawn(tr, "TD", ["value"]);
      const value = this.tileprops.fields[key][this.tileid];
      switch (key) {
      
        case "physics": {
            const select = this.dom.spawn(tdInput, "SELECT", { name: key });
            // FMN_CELLPHYSICS
            this.dom.spawn(select, "OPTION", { value: "0" }, "Vacant");
            this.dom.spawn(select, "OPTION", { value: "1" }, "Solid");
            this.dom.spawn(select, "OPTION", { value: "2" }, "Hole");
            this.dom.spawn(select, "OPTION", { value: "3" }, "Unshovellable (vacant)");
            this.dom.spawn(select, "OPTION", { value: "4" }, "Unchalkable (solid)");
            this.dom.spawn(select, "OPTION", { value: "5" }, "Sap");
            this.dom.spawn(select, "OPTION", { value: "6" }, "Sap, unchalkable");
            this.dom.spawn(select, "OPTION", { value: "7" }, "Water");
            this.dom.spawn(select, "OPTION", { value: "8" }, "Revelable");
            this.dom.spawn(select, "OPTION", { value: "9" }, "Foot hazard");
            select.value = value.toString();
            // 10..255 are unreachable unless we add them. I expect to start using values beyond 2 eventually, but probly not many.
          } break;
        
        case "neighbors": {
            const subtable = this.dom.spawn(tdInput, "TABLE", ["neighbors"]);
            for (let subrow=0, mask=0x80; subrow<3; subrow++) {
              const subtr = this.dom.spawn(subtable, "TR");
              for (let subcol=0; subcol<3; subcol++) {
                const subtd = this.dom.spawn(subtr, "TD");
                if ((subrow === 1) && (subcol === 1)) continue; // middle cell vacant
                const checkbox = this.dom.spawn(subtd, "INPUT", {
                  type: "checkbox",
                  name: `neighbors:${mask}`,
                });
                if (value & mask) checkbox.checked = true;
                mask >>= 1;
              }
            }
          } break;
          
        case "family": { // like the default, but add a "pick unused" button
            this.dom.spawn(tdInput, "INPUT", {
              type: "number",
              min: 0,
              max: 255,
              value,
              name: key,
            });
            this.dom.spawn(tdInput, "INPUT", {
              type: "button",
              value: "New",
              "on-click": () => this.onNewFamily(),
            });
          } break;
        
        default: { // normal case, number entry
            this.dom.spawn(tdInput, "INPUT", {
              type: "number",
              min: 0,
              max: 255,
              value,
              name: key,
            });
          }
      }
    }
  }
  
  renderUnavailablePreset(button) {
    const canvas = button.querySelector("canvas");
    const context = canvas.getContext("2d");
    context.clearRect(0, 0, canvas.width, canvas.height);
  }
  
  renderPreset(button, schema) {
    const canvas = button.querySelector("canvas");
    const context = canvas.getContext("2d");
    context.clearRect(0, 0, canvas.width, canvas.height);
    if (!this.image) return;
    const tilesize = Math.floor(this.image.naturalWidth / 16);
    const srcx = (this.tileid & 15) * tilesize;
    const srcy = (this.tileid >> 4) * tilesize;
    context.drawImage(this.image, srcx, srcy, canvas.width, canvas.height, 0, 0, canvas.width, canvas.height);
  }
  
  renderThumbnail() {
    const canvas = this.element.querySelector(".thumbnail");
    canvas.width = 16; // natural size of a tile
    canvas.height = 16;
    const context = canvas.getContext("2d");
    context.clearRect(0, 0, canvas.width, canvas.height);
    if (this.image) {
      const srcx = (this.tileid & 15) * canvas.width;
      const srcy = (this.tileid >> 4) * canvas.height;
      context.drawImage(this.image, srcx, srcy, canvas.width, canvas.height, 0, 0, canvas.width, canvas.height);
    }
  }
  
  onNavigation(dx, dy) {
    let col = this.tileid & 15;
    let row = this.tileid >> 4;
    col += dx;
    row += dy;
    if (col < 0) col = 0; else if (col > 15) col = 15;
    if (row < 0) row = 0; else if (row > 15) row = 15;
    const tileid = (row << 4) | col;
    if (tileid === this.tileid) return;
    this.tileid = tileid;
    this.populateUi();
  }
  
  onInput(event) {
    let key = event.target.name;
    if (!key) return;
    let value;
    // Only "neighbors" is weird, value-wise.
    if (key.startsWith("neighbors:")) {
      const mask = +key.split(":")[1];
      if (event.target.checked) value = this.tileprops.fields.neighbors[this.tileid] | mask;
      else value = this.tileprops.fields.neighbors[this.tileid] & ~mask;
      key = "neighbors";
    } else {
      value = +event.target.value;
    }
    if (isNaN(value) || (value < 0) || (value > 255)) return;
    if (value === this.tileprops.fields[key][this.tileid]) return;
    this.tileprops.fields[key][this.tileid] = value;
    this.onDirty();
  }
  
  onNewFamily() {
    if (!this.tileprops) return;
    if (!this.tileprops.fields.family) return;
    const countByFamily = [];
    for (const v of this.tileprops.fields.family) {
      if (!countByFamily[v]) countByFamily[v] = 1;
      else countByFamily[v]++;
    }
    let family = 1;
    for (; family<256; family++) {
      if (!countByFamily[family]) break;
    }
    if (family > 255) {
      this.window.alert(`No unused family IDs! (how did you do this...)`);
      return;
    }
    this.tileprops.fields.family[this.tileid] = family;
    this.element.querySelector("input[name='family']").value = family;
    this.onDirty();
  }
  
  onApplyPreset(schema) {
    if (!this.tileprops) return;
    if (!schema) return;
    const x = this.tileid & 15;
    const y = this.tileid >> 4;
    if (x + schema.w > 16) return;
    if (y + schema.h > 16) return;
    for (let yi=0, sp=0; yi<schema.h; yi++) {
      for (let xi=0; xi<schema.w; xi++, sp++) {
        const dsttileid = ((y + yi) << 4) | (x + xi);
        
        // Family copies from the focus.
        if (this.tileprops.fields.family) {
          this.tileprops.fields.family[dsttileid] = this.tileprops.fields.family[this.tileid];
        }
        
        // Physics copies from the focus.
        if (this.tileprops.fields.physics) {
          this.tileprops.fields.physics[dsttileid] = this.tileprops.fields.physics[this.tileid];
        }
        
        // Neighbors comes from the schema if present. If not in schema, copy from focus.
        if (this.tileprops.fields.neighbors) {
          this.tileprops.fields.neighbors[dsttileid] = schema.neighbors ? schema.neighbors[sp] : this.tileprops.fields.neighbors[this.tileid];
        }
        
        // Weight comes from the schema if present. If not in schema, copy from focus.
        if (this.tileprops.fields.weight) {
          this.tileprops.fields.weight[dsttileid] = schema.weight ? schema.weight[sp] : this.tileprops.fields.weight[this.tileid];
        }
      }
    }
    // Should be only the neighbor mask can change in the focus tile, but let's be safe with a full repopulate:
    this.populateUi();
    this.onDirty();
  }
}
