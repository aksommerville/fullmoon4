/* RootUi.js
 */
 
import { Dom } from "/js/util/Dom.js";
import { ResService } from "/js/util/ResService.js";
import { ToolbarUi } from "/js/ui/ToolbarUi.js";
import { MapUi } from "/js/map/MapUi.js";
import { MapAllUi } from "/js/map/MapAllUi.js";
import { ImageUi } from "/js/image/ImageUi.js";
import { ImageAllUi } from "/js/image/ImageAllUi.js";
import { SpriteUi } from "/js/sprite/SpriteUi.js";
import { SpriteAllUi } from "/js/sprite/SpriteAllUi.js";
import { ChalkUi } from "/js/chalk/ChalkUi.js";
import { LogsUi } from "/js/logs/LogsUi.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom, Window, ResService];
  }
  constructor(element, dom, window, resService) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.resService = resService;
    
    this.toolbar = null;
    this.contentController = null;
    
    this.buildUi();
    
    this.hashListener = e => this.navigateToHash(e.newURL.split('#')[1]);
    if (this.window.location.hash) {
      this.navigateToHash(this.window.location.hash.substring(1));
    }
    this.window.addEventListener("hashchange", this.hashListener);
  }
  
  onRemoveFromDom() {
    this.window.removeEventListener("hashchange", this.hashListener);
    this.hashListener = null;
  }
  
  buildUi() {
    this.element.innerText = "";
    this.toolbar = this.dom.spawnController(this.element, ToolbarUi);
    this.dom.spawn(this.element, "DIV", ["content"]);
  }
  
  clearContent() {
    const content = this.element.querySelector(".content");
    content.innerHTML = "";
    this.contentController = null;
    this.toolbar.setTattleText("");
    this.toolbar.setMapToolsVisible(false);
    return content;
  }
  
  /* Navigation by hash.
   **********************************************************/
   
  navigateToHash(hash) {
    const words = hash.split("/").filter(v => v);
    switch (words[0] || "") {
      case "": this.navigateHome(); break;
      case "map": this.navigateMap(words.slice(1)); break;
      case "image": this.navigateImage(words.slice(1)); break;
      case "sprite": this.navigateSprite(words.slice(1)); break;
      case "chalk": this.navigateChalk(words.slice(1)); break;
      case "logs": this.navigateLogs(words.slice(1)); break;
      default: {
          console.error(`Unexpected hash ${JSON.stringify(hash)}. Routing to Home instead.`);
          this.navigateHome();
        }
    }
  }
  
  navigationError(message) {
    const content = this.clearContent();
    content.innerText = message || "Unspecified error";
  }
  
  navigateHome() {
    const content = this.clearContent();
    //TODO?
  }
  
  navigateMap(args) {
    let id = args[0];
    args = args.slice(1);
    switch (id) {
      case "all": return this.navigateMapAll(args);
    }
    id = +id;
    if (isNaN(id)) return this.navigationError(`Invalid map id`);
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, MapUi);
    this.contentController.setTattleText = (text) => this.toolbar.setTattleText(text);
    this.contentController.setup(id, args);
    this.toolbar.setMapToolsVisible(true);
  }
  
  navigateMapAll(args) {
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, MapAllUi);
    this.contentController.setTattleText = (text) => this.toolbar.setTattleText(text);
  }
  
  navigateImage(args) {
    if (args[0] === "all") return this.navigateImageAll();
    const id = +args[0];
    if (isNaN(id)) return this.navigationError(`Invalid image id`);
    args = args.slice(1);
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, ImageUi);
    this.contentController.setTattleText = (text) => this.toolbar.setTattleText(text);
    this.contentController.setup(id, args);
  }
  
  navigateImageAll() {
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, ImageAllUi);
  }
  
  navigateSprite(args) {
    let id = args[0];
    args = args.slice(1);
    switch (id) {
      case "all": return this.navigateSpriteAll(args);
    }
    id = +id;
    if (isNaN(id)) return this.navigationError(`Invalid sprite id`);
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, SpriteUi);
    this.contentController.setup(id, args);
  }
  
  navigateSpriteAll(args) {
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, SpriteAllUi);
  }
  
  navigateChalk(args) {
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, ChalkUi);
  }
  
  navigateLogs(args) {
    const content = this.clearContent();
    this.contentController = this.dom.spawnController(content, LogsUi);
  }
}
