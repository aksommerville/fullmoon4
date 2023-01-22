/* SpriteUi.js
 * Controller for editing one sprite resource.
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { SpriteService } from "./SpriteService.js";

export class SpriteUi {
  static getDependencies() {
    return [HTMLElement, Dom, ResService, SpriteService, Window, "discriminator"];
  }
  constructor(element, dom, resService, spriteService, window, discriminator) {
    this.element = element;
    this.dom = dom;
    this.resService = resService;
    this.spriteService = spriteService;
    this.window = window;
    this.discriminator = discriminator;
    
    /* We'll show a few fields in a predictable order, whether they're defined or not.
     * The format is new as I'm writing this, and it's small enough we might as well include all of them.
     */
    this.fieldsAlways = ["controller", "image", "tile", "xform", "style", "physics", "decay", "radius"];
    
    this.spriteId = 0;
    this.sprite = null;
    
    this.buildUi();
  }
  
  setup(id, args) {
    this.spriteId = id;
    this.sprite = this.resService.getResourceObject("sprite", this.spriteId);
    this.populateUi();
  }
  
  /* UI.
   ************************************************************/
  
  buildUi() {
    this.element.innerHTML = "";
    const table = this.dom.spawn(this.element, "TABLE", ["mainTable"], { "on-change": () => this.onChange() });
    const trFooter = this.dom.spawn(table, "TR", ["footer"]);
    const tdFooter = this.dom.spawn(trFooter, "TD", { colspan: 4 });
    this.dom.spawn(tdFooter, "INPUT", ["addField"], { type: "button", value: "+", "on-click": () => this.onAddField(), disabled: "disabled" });
    
    const datalistControllers = this.dom.spawn(this.element, "DATALIST", { id: `SpriteUi-${this.discriminator}-controllers` });
    this.spriteService.listControllers(ctl => this.dom.spawn(datalistControllers, "OPTION", { value: ctl }));
  }
  
  populateUi() {
    const mainTable = this.element.querySelector(".mainTable");
    const trFooter = mainTable.querySelector("tr.footer");
    for (const tr of mainTable.querySelectorAll("tr.field")) tr.remove();
    if (this.sprite) {
    
      for (const key of this.fieldsAlways) {
        const tr = this.dom.createElement("TR", ["field"], { "data-key": key });
        mainTable.insertBefore(tr, trFooter);
        const command = this.sprite.commands.find(cmd => cmd[0] === key) || [key];
        this.populateFieldRow(tr, command);
      }
    
      for (const command of this.sprite.commands) {
      
        // This will also drop duplicates of our "always" fields.
        // "always" fields are supposed to be single-use so I think it's OK.
        if (this.fieldsAlways.indexOf(command[0]) >= 0) continue;
        
        const tr = this.dom.createElement("TR", ["field"], { "data-key": command[0] });
        mainTable.insertBefore(tr, trFooter);
        this.populateFieldRow(tr, command);
      }
      
      this.element.querySelector(".addField").disabled = false;
    } else {
      this.element.querySelector(".addField").disabled = false;
    }
  }
  
  populateFieldRow(tr, command) {
  
    const tdControls = this.dom.spawn(tr, "TD", ["controls"]);
    if (this.sprite.commands.indexOf(command) >= 0) {
      this.dom.spawn(tdControls, "INPUT", { type: "button", value: "X", tabindex: -1, "on-click": () => this.onDeleteCommand(command) });
      this.dom.spawn(tdControls, "INPUT", { type: "button", value: "^", tabindex: -1, "on-click": () => this.onMoveCommand(command, -1) });
      this.dom.spawn(tdControls, "INPUT", { type: "button", value: "v", tabindex: -1, "on-click": () => this.onMoveCommand(command, 1) });
    }
  
    const tdKey = this.dom.spawn(tr, "TD", ["key"], command[0]);
    
    const tdValue = this.dom.spawn(tr, "TD", ["value"]);
    this.populateValueCell(tdValue, command);
    
    const tdCommand = this.dom.spawn(tr, "TD", ["comment"], this.spriteService.getFieldCommentForDisplay(command));
  }
  
  populateValueCell(td, command) {
    let datalistId = "";
    switch (command[0]) {
      // If a particular command has a datalist, or needs entirely special handling, call that out here.
      // Most fields will be plain old text inputs.
      case "controller": datalistId = `SpriteUi-${this.discriminator}-controllers`; break;
    }
    const input = this.dom.spawn(td, "INPUT", ["field"], { type: "text", name: command[0], value: command.slice(1).join(" ") });
    if (datalistId) input.setAttribute("list", datalistId);
  }
  
  rebuildModelFromUi() {
    if (!this.sprite) return false;
    // Moves and deletes are committed immediately. We only need to rewrite existing commands.
    for (const element of this.element.querySelectorAll("input.field")) {
      const key = element.getAttribute("name");
      if (!key) continue;
      const value = element.value;
      if (!value) continue;
      this.sprite.setCommand(key, value);
    }
    return true;
  }
  
  /* Events.
   *************************************************************/
   
  onAddField() {
    if (!this.sprite) return;
    const key = this.window.prompt("Key:");
    if (!key) return;
    if (!this.spriteService.keyIsValid(key)) {
      this.window.alert(`Invalid key: ${JSON.stringify(key)}`);
      return;
    }
    this.sprite.commands.push([key]);
    this.populateUi();
    this.resService.dirty("sprite", this.spriteId, this.sprite);
  }
  
  onDeleteCommand(command) {
    if (!this.sprite) return;
    const p = this.sprite.commands.indexOf(command);
    if (p < 0) return;
    this.sprite.commands.splice(p, 1);
    this.populateUi();
    this.resService.dirty("sprite", this.spriteId, this.sprite);
  }
  
  onMoveCommand(command, d) {
    if (!this.sprite) return;
    const p = this.sprite.commands.indexOf(command);
    if (p < 0) return;
    let np;
    if (d < 0) {
      if (p < 1) return;
      np = p - 1;
    } else {
      if (p >= this.sprite.commands.length - 1) return;
      np = p + 1;
    }
    this.sprite.commands[p] = this.sprite.commands[np];
    this.sprite.commands[np] = command;
    this.populateUi();
    this.resService.dirty("sprite", this.spriteId, this.sprite);
  }
  
  onChange() {
    if (!this.rebuildModelFromUi()) return;
    this.resService.dirty("sprite", this.spriteId, this.sprite);
  }
}
