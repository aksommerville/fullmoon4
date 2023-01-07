/* Menu.js
 */
 
import { StringService } from "./StringService.js";
 
export class MenuFactory {
  static getDependencies() {
    return [StringService];
  }
  constructor(stringService) {
    this.stringService = stringService;
  }
  
  /* (options) is [[stringId, callback], ...]
   */
  newMenu(promptId, options) {
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
      }))
    );
  }
}

MenuFactory.singleton = true;

export class Menu {
  constructor(prompt, options) {
    this.prompt = prompt;
    this.options = options;
    this.selectedOptionIndex = 0;
  }
  
  input(btnid, value) {
    if (value) switch (btnid) {
      case 0x04: this.selectedOptionIndex--; return true;
      case 0x08: this.selectedOptionIndex++; return true;
      case 0x10: this.options[this.selectedOptionIndex]?.callback?.(); return true;
    }
    return false;
  }
}
