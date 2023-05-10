/* LinksUi.js
 * Panel content with links to external content.
 */
 
import { Dom } from "../util/Dom.js";

export class LinksUi {
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
    
    /* TODO Enable this once the Itch page is live.
     *
    const itchIframe = this.dom.spawn(this.element, "IFRAME", {
      frameborder: "0",
      src: "https://itch.io/embed/2055668?linkback=true&amp;bg_color=000000&amp;fg_color=dcd6d6&amp;link_color=fa5c5c&amp;border_color=333333",
      width: "552",
      height: "167",
    });
    this.dom.spawn(itchIframe, "A", { href: "https://aksommerville.itch.io/full-moon" }, "Full Moon by aksommerville");
    /**/
    
    //TODO Steam widget, once I set it up.
    
    this.dom.spawn(this.dom.spawn(this.element, "DIV"), "A",
      { href: "https://github.com/aksommerville/fullmoon4" },
      "Source code (currently invitation-only, pending some license decisions)"
    );
    
    // TODO Fetch downloadable packages off S3.
  }
}
