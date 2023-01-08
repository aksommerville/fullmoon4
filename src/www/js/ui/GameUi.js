/* GameUi.js
 * Variable-size container for the main canvas.
 */
 
import { Dom } from "/js/util/Dom.js";

export class GameUi {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const canvas = this.dom.spawn(this.element, "CANVAS", ["gameView"]);
    /*XXX*/
    canvas.width = 320;
    canvas.height = 192;
    const context = canvas.getContext("2d");
    context.beginPath();
    context.moveTo(0, 0);
    context.lineTo(canvas.width, canvas.height);
    context.moveTo(canvas.width, 0);
    context.lineTo(0, canvas.height);
    context.moveTo(1, 1);
    context.lineTo(1, canvas.height - 1);
    context.lineTo(canvas.width - 1, canvas.height - 1);
    context.lineTo(canvas.width - 1, 1);
    context.lineTo(1, 1);
    context.lineWidth = 2;
    context.strokeStyle = "#fff";
    context.stroke();
    /**/
  }
  
  // There is no corresponding "exitFullscreen"; browser takes care of it.
  enterFullscreen() {
    this.element.querySelector(".gameView").requestFullscreen();
  }
  
  getCanvas() {
    return this.element.querySelector(".gameView");
  }
}
