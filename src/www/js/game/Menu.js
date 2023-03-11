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
      case this.constants.MENU_PAUSE: return new PauseMenu(cbDismiss, this.globals, this.constants);
      case this.constants.MENU_CHALK: return new ChalkMenu(cbDismiss, this.globals, this.constants);
      case this.constants.MENU_TREASURE: return new TreasureMenu(cbDismiss, this.globals, this.constants, options);
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

/* Menu: the generic prompt-and-options one
 ************************************************************************/

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

/* PauseMenu
 ***********************************************************/

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
    if (pressed & 0x30) this.submit();
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

/* ChalkMenu
 **********************************************************/

export class ChalkMenu {
  constructor(cbDismiss, globals, constants) {
    this.cbDismiss = cbDismiss || (() => {});
    this.globals = globals;
    this.constants = constants;
    
    this.cbDirty = () => {};
    this.selx = 1;
    this.sely = 1;
    this.previousInput = 0xff;
    this.anchor = null; // null or [x,y]
    this.sketch = 0; // bits only
    this.pendingLine = 0; // sketch bit
    this.x = -1; // position in map
    this.y = -1;
  }
  
  setup(sketch, cbDirty) {
    this.sketch = sketch.bits;
    this.x = sketch.x;
    this.y = sketch.y;
    this.cbDirty = cbDirty;
  }
  
  update(state) {
    if (state === this.previousInput) return;
    const pressed = state & ~this.previousInput;
    const released = ~state & this.previousInput;
    if (pressed & 0x01) this.moveSelection(-1, 0);
    if (pressed & 0x02) this.moveSelection( 1, 0);
    if (pressed & 0x04) this.moveSelection( 0,-1);
    if (pressed & 0x08) this.moveSelection( 0, 1);
    if (pressed & 0x10) this.anchor = [this.selx, this.sely];
    if (released & 0x10) this.release();
    if (pressed & 0x20) this.submit();
    this.previousInput = state;
  }
  
  moveSelection(dx, dy) {
    let nx = this.selx + dx;
    let ny = this.sely + dy;
         if (nx < 0) nx = 2;
    else if (nx >= 3) nx = 0;
         if (ny < 0) ny = 2;
    else if (ny >= 3) ny = 0;
    if (this.anchor) {
      if (Math.abs(this.anchor[0] - nx) > 1) return;
      if (Math.abs(this.anchor[1] - ny) > 1) return;
      this.pendingLine = ChalkMenu.bitFromPoints(this.anchor[0], this.anchor[1], nx, ny);
    }
    this.selx = nx;
    this.sely = ny;
  }
  
  release() {
    if (this.pendingLine) {
      this.sketch ^= this.pendingLine;
      this.pendingLine = 0;
      this.cbDirty({
        x: this.x,
        y: this.y,
        bits: this.sketch,
      });
    }
    this.anchor = null;
  }
  
  submit() {
    this.globals.setSketch({
      x: this.x,
      y: this.y,
      bits: this.sketch,
    });
    this.cbDismiss(this);
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

/* TreasureMenu
 * Not interactive. Just shows off a new treasure and waits for acknowledgement.
 **********************************************************/

export class TreasureMenu {
  constructor(cbDismiss, globals, constants, options) {
    this.cbDismiss = cbDismiss || (() => {});
    this.globals = globals;
    this.constants = constants;
    
    if (options.length !== 1) throw new Error(`TreasureMenu requires 1 option, got ${options.length}`);
    this.itemId = options[0][0];
    this.nativeCallback = options[0][1];
    
    this.SUSPENSE_TIME = 500;
    this.OPEN_TIME = 1000;
    
    this.previousState = 0xff;
    this.startTime = Date.now();
    this.curtainOpenness = 0; // 0..1
  }
  
  update(state) {
  
    if (this.curtainOpenness < 1) {
      const now = Date.now();
      const elapsed = now - this.startTime;
      if (elapsed < this.SUSPENSE_TIME) {
        this.curtainOpenness = 0;
      } else {
        this.curtainOpenness = Math.max(0, Math.min(1, (elapsed - this.SUSPENSE_TIME) / this.OPEN_TIME));
      }
    }
  
    if (state === this.previousState) return;
    const pressed = state & ~this.previousState;
    if (pressed & 0x30) {
      this.cbDismiss(this);
      this.nativeCallback();
    }
    this.previousState = state;
  }
}
