/* ErrorModal.js
 * Generic error display.
 */
 
import { Dom } from "../util/Dom.js";

export class ErrorModal {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.error = null;
    
    this.buildUi();
  }
  
  setup(error) {
    this.error = error;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "DIV", ["error"]);
  }
  
  populateUi() {
    const output = this.element.querySelector(".error");
    if (!this.error) {
      output.innerText = "Unknown error";
    } else if (typeof(this.error) === "string") {
      output.innerText = this.error;
    } else if (typeof(this.error.log) === "string") {
      output.innerText = this.error.log;
    } else if (this.error instanceof Error) {
      output.innerText = this.error.stack?.toString() || this.error.toString();
    } else if (this.error instanceof Response) {
      output.innerText = `${this.error.url}\n${this.error.status} ${this.error.statusText}`;
    } else {
      try {
        output.innerText = JSON.stringify(this.error);
      } catch (e) {
        output.innerText = `Something failed, and then we failed to serialize the exception. ${e}`;
      }
    }
  }
}
