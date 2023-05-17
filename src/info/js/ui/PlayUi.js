/* PlayUi.js
 * Panel content with the embedded game.
 */
 
import { Dom } from "../util/Dom.js";

export class PlayUi {
  static getDependencies() {
    return [HTMLElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const flexor = this.dom.spawn(this.element, "DIV", ["flexor"]);
    this.dom.spawn(flexor, "DIV", "Open for beta testing. This is not the finished product.");
    const linkDiv = this.dom.spawn(flexor, "DIV");
    this.dom.spawn(linkDiv, "SPAN", "Comments or questions, please email me: ");
    this.dom.spawn(linkDiv, "A", { href: "mailto:aksommerville@gmail.com", "on-click": e => this.onEmail(e) }, "aksommerville@gmail.com");
    this.dom.spawn(flexor, "DIV", "Please note! There's no automatic logging. I don't know about any problems unless you email.");
    const iframe = this.dom.spawn(flexor, "IFRAME", { src: "./iframe-placeholder.html", width: "640", height: "384" });
    this.dom.spawn(flexor, "DIV", "Click to pause (fullscreen, input config, etc)");
    this.dom.spawn(flexor, "DIV", "Default controls: Arrows, Z, X");
  }
  
  onEmail(e) {
    const iframe = this.element.querySelector("iframe");
    const log = iframe.contentDocument._fmn_business_log;
    const text = log ? log.text : "";
    e.preventDefault();
    e.stopPropagation();
    const url = "mailto:aksommerville@gmail.com?subject=fullmoon%20beta&body=%0aFull%20session%20log:%0a" + encodeURIComponent(text);
    this.window.open(url);
  }
}
