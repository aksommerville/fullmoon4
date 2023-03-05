/* ChalkUi.js
 */
 
import { Dom } from "../util/Dom.js";
import { ResService } from "../util/ResService.js";
import { ChalkModal } from "./ChalkModal.js";
 
export class ChalkUi {
  static getDependencies() {
    return [HTMLElement, Dom, ResService];
  }
  constructor(element, dom, resService) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    
    this.glyphs = this.decode(
      this.resService.getResourceObject("chalk", 1) || ""
    );
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "INPUT", { value: "+", type: "button", "on-click": () => this.createGlyph() });
    for (const [codepoint, bits] of this.glyphs) {
      this.addGlyph(codepoint, bits);
    }
  }
  
  addGlyph(codepoint, bits) {
    const card = this.dom.spawn(this.element, "DIV", ["card"], {
      "data-codepoint": codepoint,
      "data-bits": bits,
      "on-click": e => this.editGlyph(e),
    });
    const topRow = this.dom.spawn(card, "DIV", ["topRow"]);
    this.dom.spawn(topRow, "span", ["bigGlyph"]);
    this.dom.spawn(topRow, "INPUT", { value: "X", type: "button", "on-click": (event) => {
      event.stopPropagation();
      this.deleteGlyph(event.target);
    }});
    this.dom.spawn(card, "CANVAS", ["bitsPicture"]);
    this.updateCard(card, codepoint, bits);
    return card;
  }
  
  updateCard(card, codepoint, bits) {
    card.setAttribute("data-codepoint", codepoint);
    card.setAttribute("data-bits", bits);
    card.querySelector(".bigGlyph").innerText = `${codepoint}: ${String.fromCharCode(codepoint)}`;
    const canvas = card.querySelector(".bitsPicture");
    ChalkModal.render(canvas, bits);
  }
  
  deleteGlyph(card) {
    while (card && !card.getAttribute("data-codepoint")) card = card.parentNode;
    if (!card) return;
    const originalCodepoint = +card.getAttribute("data-codepoint");
    const originalBits = +card.getAttribute("data-bits");
    const p = this.glyphs.findIndex(g => g[0] === originalCodepoint && g[1] === originalBits);
    if (p < 0) return;
    this.glyphs.splice(p, 1);
    this.resService.dirty("chalk", 1, this.encode());
    card.remove();
  }
  
  editGlyph(event) {
    let card = event.target;
    while (card && !card.getAttribute("data-codepoint")) card = card.parentNode;
    if (!card) return;
    const originalCodepoint = +card.getAttribute("data-codepoint");
    const originalBits = +card.getAttribute("data-bits");
    const p = this.glyphs.findIndex(g => g[0] === originalCodepoint && g[1] === originalBits);
    if (p < 0) return;
    const modal = this.dom.spawnModal(ChalkModal);
    modal.setup(originalCodepoint, originalBits);
    modal.onDirty = (codepoint, bits) => {
      this.glyphs[p] = [codepoint, bits];
      this.updateCard(card, codepoint, bits);
      this.resService.dirty("chalk", 1, this.encode());
    };
  }
  
  createGlyph() {
    let p = -1;
    let card = null;
    const modal = this.dom.spawnModal(ChalkModal);
    modal.setup(0x41, 0);
    modal.onDirty = (codepoint, bits) => {
      if (p < 0) {
        p = this.glyphs.length;
        this.glyphs.push([codepoint, bits]);
        card = this.addGlyph(codepoint, bits);
      } else {
        this.glyphs[p] = [codepoint, bits];
        this.updateCard(card, codepoint, bits);
      }
      this.resService.dirty("chalk", 1, this.encode());
    };
  }
  
  // Array of [codepoint, bits]
  decode(src) {
    const glyphs = [];
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line) continue;
      const [codepoint, bits] = line.split(/\s+/).map(v => +v);
      if (isNaN(codepoint) || (codepoint <= 0) || (codepoint > 0xff) || isNaN(bits) || (bits < 0) || (bits > 0x000fffff)) {
        console.error(`chalk:${lineno}: Failed to parse line ${JSON.stringify(line)}, skipping.`);
      } else {
        glyphs.push([codepoint, bits]);
      }
    }
    return glyphs;
  }
  
  encode() {
    return this.glyphs.map(([ch, v]) => `${ch} ${v}\n`).join("");
  }
}
