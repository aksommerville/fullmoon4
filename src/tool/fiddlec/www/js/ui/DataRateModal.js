/* DataRateModal.js
 */
 
import { Dom } from "../util/Dom.js";

export class DataRateModal {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.report = null;
  }
  
  setup(report) {
    this.report = report;
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    if (!this.report) return;
    const table = this.dom.spawn(this.element, "TABLE");
    let tr;
    
    tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TH", "Type");
    this.dom.spawn(tr, "TH", "Count");
    this.dom.spawn(tr, "TH", "Size");
    this.dom.spawn(tr, "TH", "Duration");
    this.dom.spawn(tr, "TH", "b/s");
    
    tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", "Sound");
    this.dom.spawn(tr, "TD", this.report.soundCount);
    this.dom.spawn(tr, "TD", this.report.soundSize);
    this.dom.spawn(tr, "TD", this.reprTime(this.report.soundTime));
    this.dom.spawn(tr, "TD", Math.round((this.report.soundSize * 1000) / this.report.soundTime));
    
    tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", "Song");
    this.dom.spawn(tr, "TD", this.report.songCount);
    this.dom.spawn(tr, "TD", this.report.songSize);
    this.dom.spawn(tr, "TD", this.reprTime(this.report.songTime));
    this.dom.spawn(tr, "TD", Math.round((this.report.songSize * 1000) / this.report.songTime));
    
    tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", "Total");
    this.dom.spawn(tr, "TD", this.report.soundCount + this.report.songCount);
    this.dom.spawn(tr, "TD", this.report.soundSize + this.report.songSize);
    this.dom.spawn(tr, "TD", this.reprTime(this.report.soundTime + this.report.songTime));
    this.dom.spawn(tr, "TD", Math.round(((this.report.soundSize + this.report.songSize) * 1000) / (this.report.soundTime + this.report.songTime)));
  }
  
  reprTime(ms) {
    let sec = Math.floor(ms / 1000); ms %= 1000;
    let min = Math.floor(sec / 60); sec %= 60;
    let hour = Math.floor(min / 60); min %= 60;
    if (min < 10) min = "0" + min;
    if (sec < 10) sec = "0" + sec;
    if (ms < 10) ms = "00" + ms;
    else if (ms < 100) ms = "0" + ms;
    return `${hour}:${min}:${sec}.${ms}`;
  }
}
