/* Menu.js
 */
 
import { StringService } from "./StringService.js";
import { Globals } from "./Globals.js";
import { Constants } from "./Constants.js";
 
export class MenuFactory {
  static getDependencies() {
    return [StringService, Globals, Constants];
  }
  constructor(stringService, globals, constants) {
    this.stringService = stringService;
    this.globals = globals;
    this.constants = constants;
  }
  
  /* (options) is [[stringId, callback], ...]
   */
  newMenu(promptId, options, cbDismiss) {
    if (promptId < 0) switch (promptId) {
      case Menu.PAUSE: return new PauseMenu(cbDismiss, this.globals, this.constants);
    }
    const resolvedOptions = [];
    for (let i=0; i<options.length; i+=2) {
      resolvedOptions.push({
        string: this.stringService.getStringById(options[i]),
        callback: options[i + 1],
      });
    }
    return new Menu(
      this.stringService.getStringById(promptId),
      options.map(o => ({
        string: this.stringService.getStringById(o[0]),
        callback: o[1],
      })),
      cbDismiss
    );
  }
}

MenuFactory.singleton = true;

export class Menu {
  constructor(prompt, options, cbDismiss) {
    this.prompt = prompt;
    this.options = options;
    this.cbDismiss = cbDismiss || (() => {});
    this.selectedOptionIndex = 0;
    this.previousInput = 0xff;
  }
  
  update(state) {
    if (state === this.previousInput) return;
    if ((state & 0x04) && !(this.previousInput & 0x04)) {
      this.selectedOptionIndex--;
      if (this.selectedOptionIndex < 0) {
        this.selectedOptionIndex = this.options.length - 1;
      }
    }
    if ((state & 0x08) && !(this.previousInput & 0x08)) {
      this.selectedOptionIndex++;
      if (this.selectedOptionIndex >= this.options.length) {
        this.selectedOptionIndex = 0;
      }
    }
    if ((state & 0x10) && !(this.previousInput & 0x10)) {
      this.options[this.selectedOptionIndex]?.callback?.();
      this.cbDismiss(this);
    }
    this.previousInput = state;
  }
}

export class PauseMenu {
  constructor(cbDismiss, globals, constants) {
    this.cbDismiss = cbDismiss || (() => {});
    this.globals = globals;
    this.constants = constants;
    
    const selection = this.globals.g_selected_item[0];
    this.selx = selection & 3;
    this.sely = (selection >> 2) & 3;
    this.previousInput = 0xff;
  }
  
  update(state) {
    if (state === this.previousInput) return;
    const pressed = state & ~this.previousInput;
    if (pressed & 0x01) this.moveSelection(-1, 0);
    if (pressed & 0x02) this.moveSelection( 1, 0);
    if (pressed & 0x04) this.moveSelection( 0,-1);
    if (pressed & 0x08) this.moveSelection( 0, 1);
    if (pressed & 0x20) this.submit();//TODO I'd also like A to submit, but we must prevent it being delivered to the game after dismissal
    this.previousInput = state;
  }
  
  moveSelection(dx, dy) {
    this.selx += dx;
    this.selx &= 3;
    this.sely += dy;
    this.sely &= 3;
  }
  
  submit() {
    this.globals.g_selected_item[0] = (this.sely << 2) | this.selx;
    this.cbDismiss(this);
  }
}

// Special prompt IDs.
Menu.PAUSE = -1;
