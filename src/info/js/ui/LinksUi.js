/* LinksUi.js
 * Panel content with links to external content.
 */
 
import { Dom } from "../util/Dom.js";
import { Comm } from "../util/Comm.js";

export class LinksUi {
  static getDependencies() {
    return [HTMLElement, Dom, Comm, Window];
  }
  constructor(element, dom, comm, window) {
    this.element = element;
    this.dom = dom;
    this.comm = comm;
    this.window = window;
    
    this.toc = null;
    
    // Guess platform from userAgent, in the terms used by our package labels.
    if (this.window.navigator.userAgent.includes("Windows")) this.userPlatform = "mswin";
    else if (this.window.navigator.userAgent.includes("Linux")) this.userPlatform = "linux";
    else if (this.window.navigator.userAgent.includes("MacOS")) this.userPlatform = "macos";
    else this.userPlatform = "unknown";
    
    this.buildUi();
    
    //TODO Don't refresh until we are actually visible.
    // I suspect most visitors will not touch this Links page, and we can spare AWS some traffic.
    //this.refreshPackages();
  }
  
  onShowTab() {
    if (this.toc) return;
    this.toc = [];
    this.refreshPackages();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    // Itch demo.
    const itchIframe = this.dom.spawn(this.element, "IFRAME", {
      frameborder: "0",
      src: "https://itch.io/embed/2055668?linkback=true&amp;bg_color=000000&amp;fg_color=dcd6d6&amp;link_color=fa5c5c&amp;border_color=333333",
      width: "552",
      height: "167",
    });
    this.dom.spawn(itchIframe, "A", { href: "https://aksommerville.itch.io/full-moon" }, "Full Moon by aksommerville");
    
    //TODO Itch full version, I assume we'll be doing two separate Itch apps.
    //TODO Steam widget, once I set it up.
    
    // GitHub
    this.dom.spawn(this.dom.spawn(this.element, "DIV"), "A",
      { href: "https://github.com/aksommerville/fullmoon4" },
      "Source code: https://github.com/aksommerville/fullmoon4"
    );
    
    // Packages
    this.dom.spawn(this.element, "H2", "Download");
    this.dom.spawn(this.element, "TABLE", ["packages"]);
  }
  
  refreshPackages() {
    this.element.querySelector(".packages").innerHTML = "";
    return this.comm.getText("https://fullmoon-release.s3.us-east-2.amazonaws.com/").then(rsp => {
      const doc = new this.window.DOMParser().parseFromString(rsp, "application/xml");
      this.rebuildPackagesUi(this.toc = this.tocFromPackagesDom(doc));
    }).catch(error => {
      console.error(`get packages toc failed`, error);
    });
  }
  
  tocFromPackagesDom(doc) {
    return Array.from(doc.querySelectorAll("Contents"))
      .map(parent => [parent.querySelector("Key").textContent, +parent.querySelector("Size").textContent])
      .map(([name, size]) => {
        if (!size) return null;
        const match = name.match(/^fullmoon-([a-z0-9_]+)-([a-z0-9_]+)-(v[0-9]+\.[0-9]+\.[0-9]+)\..*$/);
        if (!match) return null;
        // Opportunity here to filter on version. But I'll probably take care of that in S3.
        return {
          name,
          size,
          platform: match[1],
          dataSet: match[2],
          version: match[3],
        };
      })
      .filter(v => v)
      .sort((a, b) => this.toccmp(a, b));
  }
  
  toccmp(a, b) {
    let v;
    // Higher versions first, that's easy.
    if (v = this.versioncmp(a.version, b.version)) return -v;
    
    // If one matches the platform inferred from userAgent, it comes first.
    if (a.platform !== b.platform) {
      if (a.platform === this.userPlatform) return -1;
      if (b.platform === this.userPlatform) return 1;
    }
    
    // dataSet: Not sure if I'll be putting real "full" version in the S3, but if so those come first.
    if (a.dataSet !== b.dataSet) {
      if (a.dataSet === "full") return -1;
      if (b.dataset === "full") return 1;
      if (a.dataSet === "demo") return -1;
      if (b.dataSet === "demo") return 1;
      // hmm not a dataSet I planned for. Put it after "full" and "demo".
    }
    
    // Nothing else meaningful.
    return 0;
  }
  
  versioncmp(a, b) {
    const [aa, ab, ac] = a.substring(1).split('.').map(v => +v);
    const [ba, bb, bc] = b.substring(1).split('.').map(v => +v);
    return (aa - ba) || (ab - bb) || (ac - bc);
  }
  
  rebuildPackagesUi(toc) {
    const table = this.element.querySelector(".packages");
    table.innerHTML = "";
    if (toc.length) {
      for (const { name, size, platform, dataSet, version } of toc) {
        const tr = this.dom.spawn(table, "TR");
        this.dom.spawn(tr, "TD", platform);
        this.dom.spawn(tr, "TD", version);
        this.dom.spawn(tr, "TD", dataSet);
        const tdName = this.dom.spawn(tr, "TD");
        const url = "https://fullmoon-release.s3.us-east-2.amazonaws.com/" + name;
        this.dom.spawn(tdName, "A", { href: url }, name);
        this.dom.spawn(tr, "TD", this.reprSize(size));
      }
    } else {
      this.dom.spawn(this.dom.spawn(table, "TR"), "TD", "No packages available.");
    }
  }
  
  reprSize(size) {
    if (size > 0x7fffffff) return "???";
    if (size >= 1 << 30) return `${size >> 30} GB`;
    if (size >= 1 << 20) return `${size >> 20} MB`;
    if (size >= 1 << 10) return `${size >> 10} kB`;
    return `${size} B`;
  }
}
