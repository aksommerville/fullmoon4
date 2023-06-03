/* UsageModal.js
 * Display a report from GET /api/insusage, instruments by song or vice-versa.
 */
 
import { Dom } from "../util/Dom.js";
import { FiddleService } from "../FiddleService.js";

export class UsageModal {
  static getDependencies() {
    return [HTMLElement, Dom, FiddleService];
  }
  constructor(element, dom, fiddleService) {
    this.element = element;
    this.dom = dom;
    this.fiddleService = fiddleService;
    
    this.report = null;
  }
  
  setup(report) {
    this.report = report;
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const insTable = this.dom.spawn(this.element, "TABLE", ["insTable"]);
    const insHeader = this.dom.spawn(insTable, "TR");
    this.dom.spawn(insHeader, "TH", "Instr");
    // Instruments don't have names. We could use the GM names, but I haven't been adhering to them when composing.
    this.dom.spawn(insHeader, "TH", "Songs");
    
    const sndTable = this.dom.spawn(this.element, "TABLE", ["sndTable"]);
    const sndHeader = this.dom.spawn(sndTable, "TR");
    this.dom.spawn(sndHeader, "TH", "Sound");
    this.dom.spawn(sndHeader, "TH", "Name");
    this.dom.spawn(sndHeader, "TH", "Songs");
    
    const songTable = this.dom.spawn(this.element, "TABLE", ["songTable"]);
    const songHeader = this.dom.spawn(songTable, "TR");
    this.dom.spawn(songHeader, "TH", "ID");
    this.dom.spawn(songHeader, "TH", "Name");
    this.dom.spawn(songHeader, "TH", "Instruments");
    this.dom.spawn(songHeader, "TH", "Sounds");
    
    if (this.report) {
      for (const { id, name } of this.fiddleService.sounds) this.addSound(sndTable, id, name);
      for (const { id, name } of this.fiddleService.instruments) this.addInstrument(insTable, id, name);
      for (const { id, name } of this.fiddleService.songs) this.addSong(songTable, id, name);
    }
  }
  
  addSound(table, id, name) {
    // There are lots of sounds for artificial use only, I don't want to confuse matters by including those.
    // A sound is reported if it's used by any song, or if its ID is in the GM drum range (35..81).
    // well. GM drums officially run thru 81, but we only go thru 56 COWBELL.
    const songids = this.report.filter(r => r.sounds.indexOf(id) >= 0).map(r => r.id);
    if ((songids.length > 0) || ((id >= 35) && (id <= 56))) {
      const tr = this.dom.spawn(table, "TR");
      this.dom.spawn(tr, "TD", id);
      this.dom.spawn(tr, "TD", name);
      this.dom.spawn(tr, "TD", JSON.stringify(songids));
      if (songids.length < 1) {
        tr.classList.add("notice");
      }
    }
  }
  
  addInstrument(table, id, name) {
    const songids = this.report.filter(r => r.instruments.indexOf(id) >= 0).map(r => r.id);
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", id);
    this.dom.spawn(tr, "TD", JSON.stringify(songids));
    if (id === 1) { // Program 1 is special, it's Dot's violin. Must not be used by any songs.
      if (songids.length) tr.classList.add("notice");
    } else if (songids.length !== 1) { // Each instrument should be used exactly once.
      tr.classList.add("notice");
    }
  }
  
  addSong(table, id, name) {
    const r = this.report.find(rr => rr.id === id) || { id, instruments: [], sounds: [] };
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", id);
    this.dom.spawn(tr, "TD", name);
    this.dom.spawn(tr, "TD", JSON.stringify(r.instruments));
    this.dom.spawn(tr, "TD", JSON.stringify(r.sounds));
    if (r.instruments.length < 1) {
      tr.classList.add("notice");
    }
  }
}
