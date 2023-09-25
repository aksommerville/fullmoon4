/* DownloadsUi.js
 * Inside the Links page, access to the downloadable packages in S3.
 */
 
import { Dom } from "../util/Dom.js";
import { Comm } from "../util/Comm.js";

export class DownloadsUi {
  static getDependencies() {
    return [HTMLElement, Dom, Comm, "discriminator", Window];
  }
  constructor(element, dom, comm, discriminator, window) {
    this.element = element;
    this.dom = dom;
    this.comm = comm;
    this.discriminator = discriminator;
    this.window = window;
    
    this.toc = null; // null or [{name,size,platform,dataSet,version}]
    this.latestVersion = "";
    
    // Guess platform from userAgent, in the terms used by our package labels.
    if (this.window.navigator.userAgent.includes("Windows")) this.userPlatform = "mswin";
    else if (this.window.navigator.userAgent.includes("Linux")) this.userPlatform = "linux";
    else if (this.window.navigator.userAgent.includes("MacOS")) this.userPlatform = "macos";
    else this.userPlatform = "unknown";
    
    this.buildUi();
  }
  
  refreshIfEmpty() {
    if (this.toc) return;
    this.toc = [];
    this.refreshPackages();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    this.dom.spawn(this.element, "H2", "Direct Downloads");
    
    this.dom.spawn(this.element, "DIV",
      ["tech-warning", "hidden", "raspiWarning"],
      "On a v1 Raspberry Pi, you must set VRAM to at least 128 MB. Use raspi-config, under Advanced Settings."
    );
    
    const filterRow = this.dom.spawn(this.element, "DIV", ["filterRow"]);
    const explanation = this.dom.spawn(filterRow, "DIV", ["explanation"], "Showing 0 of 0 packages.");
    this.spawnFilterMenu(filterRow, "platform", "Platform");
    this.spawnFilterMenu(filterRow, "version", "Version", ["Latest"]);
    this.spawnFilterMenu(filterRow, "dataSet", "Data");
    
    const mainTable = this.dom.spawn(this.element, "TABLE", ["mainTable"]);
  }
  
  spawnFilterMenu(filterRow, key, label, extraOptionLabels) {
    const id = `DownloadsUi-${this.discriminator}-${key}`;
    const coupler = this.dom.spawn(filterRow, "DIV");
    this.dom.spawn(coupler, "LABEL", { for: id }, label);
    const select = this.dom.spawn(coupler, "SELECT", ["filter"], { id, name: key, "on-change": () => this.onFilterChange(event) });
    if (extraOptionLabels) for (const extraLabel of extraOptionLabels) {
      this.dom.spawn(select, "OPTION", { value: extraLabel, "data-extra": true }, extraLabel);
    }
    this.dom.spawn(select, "OPTION", { value: "all" }, "All");
    if (extraOptionLabels) select.value = extraOptionLabels[0];
    else select.value = "all";
  }
  
  onFilterChange(event) {
    this.rebuildPackagesUi(this.toc);
  }
  
  refreshPackages() {
    this.element.querySelector(".mainTable").innerHTML = "";
    return this.comm.getText("https://fullmoon-release.s3.us-east-2.amazonaws.com/").then(rsp => {
      const doc = new this.window.DOMParser().parseFromString(rsp, "application/xml");
      this.toc = this.tocFromPackagesDom(doc);
      this.rebuildFilters(this.toc);
      this.rebuildPackagesUi(this.toc);
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
  
  rebuildFilters(toc) {
    let platforms = new Set(), versions = new Set(), dataSets = new Set();
    for (const { platform, version, dataSet } of toc) {
      platforms.add(platform);
      versions.add(version);
      dataSets.add(dataSet);
    }
    platforms = Array.from(platforms).sort();
    versions = Array.from(versions).sort((a, b) => -this.versioncmp);
    dataSets = Array.from(dataSets).sort();
    
    this.latestVersion = versions[0] || "";
    
    // Add anything to the UI that's missing.
    // We don't do the expected remove, since really that's not going to happen -- there's just one repopulate of this list.
    let select = this.element.querySelector("select.filter[name='platform']");
    for (const value of platforms) {
      if (this.element.querySelector(`option[value='${value}']`)) continue;
      this.dom.spawn(select, "OPTION", { value }, value);
    }
    select = this.element.querySelector("select.filter[name='version']");
    for (const value of versions) {
      if (this.element.querySelector(`option[value='${value}']`)) continue;
      this.dom.spawn(select, "OPTION", { value }, value);
    }
    select = this.element.querySelector("select.filter[name='dataSet']");
    for (const value of dataSets) {
      if (this.element.querySelector(`option[value='${value}']`)) continue;
      this.dom.spawn(select, "OPTION", { value }, value);
    }
    
    // Select the platform we inferred, if it's real.
    // Like the removal concern, this would be different if we expected continuing refreshes from the backend. Would have to track pristine.
    if (platforms.indexOf(this.userPlatform) >= 0) {
      this.element.querySelector("select.filter[name='platform']").value = this.userPlatform;
    }
  }
  
  rebuildPackagesUi(toc) {
    const table = this.element.querySelector(".mainTable");
    table.innerHTML = "";
    let visibleCount = 0, availableCount = 0;
    let showRaspiWarning = false;
    if (toc.length) {
      const selectedPlatform = this.element.querySelector("select.filter[name='platform']").value;
      const selectedVersion = this.element.querySelector("select.filter[name='version']").value;
      const selectedDataSet = this.element.querySelector("select.filter[name='dataSet']").value;
      
      for (const { name, size, platform, dataSet, version } of toc) {
        availableCount++;
      
        if (selectedPlatform === "all") ;
        else if (selectedPlatform !== platform) continue;
        
        if (selectedVersion === "all") ;
        else if (selectedVersion === "Latest") {
          // "Latest" version should probably mean "after other filters are applied", but I expect we'll always have the full spread at each version.
          if (version !== this.latestVersion) continue;
        } else if (selectedVersion !== version) continue;
        
        if (selectedDataSet === "all") ;
        else if (selectedDataSet !== dataSet) continue;
        
        if (platform === "raspi") showRaspiWarning = true;
      
        const tr = this.dom.spawn(table, "TR");
        this.dom.spawn(tr, "TD", platform);
        this.dom.spawn(tr, "TD", version);
        this.dom.spawn(tr, "TD", dataSet);
        const tdName = this.dom.spawn(tr, "TD");
        const url = "https://fullmoon-release.s3.us-east-2.amazonaws.com/" + name;
        this.dom.spawn(tdName, "A", { href: url }, name);
        this.dom.spawn(tr, "TD", this.reprSize(size));
        
        visibleCount++;
      }
    } else {
      this.dom.spawn(this.dom.spawn(table, "TR"), "TD", "No packages available.");
    }
    this.element.querySelector(".explanation").innerText = `Showing ${visibleCount} of ${availableCount} packages.`;
    if (showRaspiWarning) this.element.querySelector(".raspiWarning").classList.remove("hidden");
    else this.element.querySelector(".raspiWarning").classList.add("hidden");
  }
  
  reprSize(size) {
    if (size > 0x7fffffff) return "???";
    if (size >= 1 << 30) return `${size >> 30} GB`;
    if (size >= 1 << 20) return `${size >> 20} MB`;
    if (size >= 1 << 10) return `${size >> 10} kB`;
    return `${size} B`;
  }
}
