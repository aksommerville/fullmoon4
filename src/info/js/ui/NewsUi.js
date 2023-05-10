/* NewsUi.js
 * Panel content with updates from the author.
 */
 
import { Dom } from "../util/Dom.js";

export class NewsUi {
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
    //TODO Probly not the best presentation. Can we pretty this up a little?
    this.addStory("2023-06-22 .. 2023-06-25", `Come see us at <a href="https://thegdex.com/">GDEX</a>!`);
    this.addStory("2023-05-09", "Redesigning this home page for Full Moon. Isn't it pretty?");
  }
  
  addStory(date, html) {
    this.dom.spawn(this.element, "DIV", ["headline"], date);
    this.dom.spawn(this.element, "DIV", ["story"]).innerHTML = html;
  }
}
