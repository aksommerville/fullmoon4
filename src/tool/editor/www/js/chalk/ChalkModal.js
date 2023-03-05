/* ChalkModal.js
 */
 
import { Dom } from "../util/Dom.js";

export class ChalkModal {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.codepoint = 0;
    this.bits = 0;
    this.onDirty = (codepoint, bits) => {};
    this.anchor = null; // null or [col,row]
    
    this.buildUi();
  }
  
  setup(codepoint, bits) {
    this.codepoint = codepoint || 0;
    this.bits = bits || 0;
    this.element.querySelector("input[name='codepoint']").value = this.codepoint;
    this.populateCodepointPreview(this.codepoint);
    this.drawBits(this.bits);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "INPUT", { name: "codepoint", type: "number", min: 0, max: 255, "on-input": () => this.onCodepointChange() });
    this.dom.spawn(this.element, "DIV", ["codepointPreview"]);
    const bitsContainer = this.dom.spawn(this.element, "DIV", ["bitsContainer"], { "on-click": e => this.onClick(e) });
    this.dom.spawn(bitsContainer, "CANVAS", ["lines"]);
    const table = this.dom.spawn(bitsContainer, "TABLE", ["buttonsTable"]);
    for (let row=0; row<3; row++) {
      const tr = this.dom.spawn(table, "TR");
      for (let col=0; col<3; col++) {
        const td = this.dom.spawn(tr, "TD");
        const input = this.dom.spawn(td, "INPUT", ["vertex"], { type: "button", "data-row": row, "data-col": col });
      }
    }
  }
  
  onCodepointChange() {
    const codepoint = +this.element.querySelector("input[name='codepoint']").value;
    if (!this.populateCodepointPreview(codepoint)) return;
    this.codepoint = codepoint;
    this.onDirty(this.codepoint, this.bits);
  }
  
  populateCodepointPreview(codepoint) {
    if (isNaN(codepoint) || (codepoint < 1) || (codepoint > 0xff)) {
      this.element.querySelector(".codepointPreview").innerText = "INVALID";
      return false;
    }
    if ((codepoint > 0x20) && (codepoint < 0x7f)) {
      this.element.querySelector(".codepointPreview").innerText = String.fromCharCode(codepoint);
    } else {
      this.element.querySelector(".codepointPreview").innerText = "---";
    }
    return true;
  }
  
  onClick(event) {
    if (event.target.tagName !== "INPUT") return;
    const col = +event.target.getAttribute("data-col");
    const row = +event.target.getAttribute("data-row");
    if (isNaN(col) || isNaN(row)) return;
    if (this.anchor) {
      this.toggleLine(col, row, this.anchor[0], this.anchor[1]);
      this.anchor = null;
      for (const button of this.element.querySelectorAll(".vertex")) button.disabled = false;
    } else {
      this.anchor = [col, row];
      for (const button of this.element.querySelectorAll(".vertex")) button.disabled = true;
      for (let dx=-1; dx<=1; dx++) for (let dy=-1; dy<=1; dy++) {
        if (!dx && !dy) continue;
        const button = this.element.querySelector(`.vertex[data-col='${col + dx}'][data-row='${row + dy}']`);
        if (button) button.disabled = false;
      }
    }
  }
  
  toggleLine(ax, ay, bx, by) {
    const bit = ChalkModal.bitFromPoints(ax, ay, bx, by);
    if (!bit) return;
    this.bits ^= bit;
    this.drawBits(this.bits);
    this.onDirty(this.codepoint, this.bits);
  }
  
  drawBits(bits) {
    const canvas = this.element.querySelector(".lines");
    ChalkModal.render(canvas, bits);
  }
  
  static render(canvas, bits) {
    canvas.width = canvas.clientWidth;
    canvas.height = canvas.clientHeight;
    const ctx = canvas.getContext("2d");
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    if (bits) {
      ctx.beginPath();
      const colw = Math.floor(canvas.width / 3);
      const rowh = Math.floor(canvas.height / 3);
      const offx = Math.floor(colw / 2) + 0.5;
      const offy = Math.floor(rowh / 2) + 0.5;
      for (let mask=1; mask<0x00100000; mask<<=1) {
        if (bits & mask) {
          const [ax, ay, bx, by] = ChalkModal.pointsFromBit(mask);
          ctx.moveTo(offx + ax * colw, offy + ay * rowh);
          ctx.lineTo(offx + bx * colw, offy + by * rowh);
        }
      }
      ctx.strokeStyle = "#fff";
      ctx.lineWidth = 3;
      ctx.stroke();
    }
  }
  
  static bitFromPoints(ax, ay, bx, by) {
    const xd = bx - ax;
    const yd = by - ay;
    if ((xd < -1) || (xd > 1) || (yd < -1) || (yd > 1) || (!xd &&!yd)) return 0;
    let ap = ay * 3 + ax;
    let bp = by * 3 + bx;
    if (ap > bp) {
      const tmp = ap;
      ap = bp;
      bp = tmp;
    }
    if ((ap === 0) && (bp === 1)) return 0x80000;
    if ((ap === 0) && (bp === 3)) return 0x40000;
    if ((ap === 0) && (bp === 4)) return 0x20000;
    if ((ap === 1) && (bp === 2)) return 0x10000;
    if ((ap === 1) && (bp === 3)) return 0x08000;
    if ((ap === 1) && (bp === 4)) return 0x04000;
    if ((ap === 1) && (bp === 5)) return 0x02000;
    if ((ap === 2) && (bp === 4)) return 0x01000;
    if ((ap === 2) && (bp === 5)) return 0x00800;
    if ((ap === 3) && (bp === 4)) return 0x00400;
    if ((ap === 3) && (bp === 6)) return 0x00200;
    if ((ap === 3) && (bp === 7)) return 0x00100;
    if ((ap === 4) && (bp === 5)) return 0x00080;
    if ((ap === 4) && (bp === 6)) return 0x00040;
    if ((ap === 4) && (bp === 7)) return 0x00020;
    if ((ap === 4) && (bp === 8)) return 0x00010;
    if ((ap === 5) && (bp === 7)) return 0x00008;
    if ((ap === 5) && (bp === 8)) return 0x00004;
    if ((ap === 6) && (bp === 7)) return 0x00002;
    if ((ap === 7) && (bp === 8)) return 0x00001;
    return 0;
  }
  
  static pointsFromBit(bit) {
    switch (bit) {
      case 0x00001: return [1, 2, 2, 2];
      case 0x00002: return [0, 2, 1, 2];
      case 0x00004: return [2, 1, 2, 2];
      case 0x00008: return [2, 1, 1, 2];
      case 0x00010: return [1, 1, 2, 2];
      case 0x00020: return [1, 1, 1, 2];
      case 0x00040: return [1, 1, 0, 2];
      case 0x00080: return [1, 1, 2, 1];
      case 0x00100: return [0, 1, 1, 2];
      case 0x00200: return [0, 1, 0, 2];
      case 0x00400: return [0, 1, 1, 1];
      case 0x00800: return [2, 0, 2, 1];
      case 0x01000: return [2, 0, 1, 1];
      case 0x02000: return [1, 0, 2, 1];
      case 0x04000: return [1, 0, 1, 1];
      case 0x08000: return [1, 0, 0, 1];
      case 0x10000: return [1, 0, 2, 0];
      case 0x20000: return [0, 0, 1, 1];
      case 0x40000: return [0, 0, 0, 1];
      case 0x80000: return [0, 0, 1, 0];
    }
    return [0, 0, 0, 0];
  }
}
