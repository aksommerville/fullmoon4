/* LinksUi.js
 * Panel content with links to external content.
 */
 
import { Dom } from "../util/Dom.js";
import { Comm } from "../util/Comm.js";
import { DownloadsUi } from "./DownloadsUi.js";

export class LinksUi {
  static getDependencies() {
    return [HTMLElement, Dom, Comm, Window];
  }
  constructor(element, dom, comm, window) {
    this.element = element;
    this.dom = dom;
    this.comm = comm;
    this.window = window;
    
    this.downloadsUi = null;
    
    this.buildUi();
  }
  
  onShowTab() {
    this.downloadsUi.refreshIfEmpty();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    // Itch demo.
    const itchDemoIframe = this.dom.spawn(this.element, "IFRAME", {
      frameborder: "0",
      src: "https://itch.io/embed/2055668?linkback=true&amp;bg_color=000000&amp;fg_color=dcd6d6&amp;link_color=fa5c5c&amp;border_color=333333",
      width: "552",
      height: "167",
    });
    this.dom.spawn(itchDemoIframe, "A", { href: "https://aksommerville.itch.io/full-moon-demo" }, "Full Moon Demo by aksommerville");
    
    // Itch full version.
    const itchFullIframe = this.dom.spawn(this.element, "IFRAME", {
      frameborder: "0",
      src: "https://itch.io/embed/2237816",
      width: "552",
      height: "167",
    });
    this.dom.spawn(itchFullIframe, "A", { href: "https://aksommerville.itch.io/full-moon" }, "Full Moon by aksommerville");
    
    // Steam.
    this.dom.spawn(this.element, "IFRAME", {
      src: "https://store.steampowered.com/widget/2578750/",
      frameborder: "0",
      width: "646",
      height: "190",
    });
    
    // GitHub
    this.dom.spawn(this.dom.spawn(this.element, "DIV"), "A",
      { href: "https://github.com/aksommerville/fullmoon4" },
      "Source code: https://github.com/aksommerville/fullmoon4"
    );
    
    // Packages
    this.downloadsUi = this.dom.spawnController(this.element, DownloadsUi);
  }
}
