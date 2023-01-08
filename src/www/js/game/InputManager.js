/* InputManager.js
 * Keyboard, gamepad, touch.
 */
 
export class InputManager {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.state = 0;
    
    this.window.addEventListener("keydown", e => this.onKey(e, 1));
    this.window.addEventListener("keyup", e => this.onKey(e, 0));
  }
  
  clearState() {
    this.state = 0;
  }
  
  update() {
  }
  
  /* Keyboard.
   *******************************************/
   
  onKey(event, value) {
    const btnid = this.mapKey(event.code);
    if (!btnid) return;
    event.preventDefault();
    event.stopPropagation();
    if (value) {
      if (this.state & btnid) return;
      this.state |= btnid;
    } else {
      if (!(this.state & btnid)) return;
      this.state &= ~btnid;
    }
  }
  
  mapKey(code) {
    switch (code) {
      //TODO configurable
      case "ArrowLeft": return InputManager.INPUT_LEFT;
      case "ArrowRight": return InputManager.INPUT_RIGHT;
      case "ArrowUp": return InputManager.INPUT_UP;
      case "ArrowDown": return InputManager.INPUT_DOWN;
      case "KeyZ": return InputManager.INPUT_USE;
      case "KeyX": return InputManager.INPUT_MENU;
    }
    return null;
  }
  
  //TODO gamepad
  //TODO touch
}

InputManager.singleton = true;

InputManager.INPUT_LEFT  = 0x01;
InputManager.INPUT_RIGHT = 0x02;
InputManager.INPUT_UP    = 0x04;
InputManager.INPUT_DOWN  = 0x08;
InputManager.INPUT_USE   = 0x10;
InputManager.INPUT_MENU  = 0x20;
