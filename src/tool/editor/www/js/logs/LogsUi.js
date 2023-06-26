/* LogsUi.js
 * For displaying digests from app logs.
 * We have a wealth of player data from GDEX 2023 (in repo fullmoon-errata); this is the place to play with it.
 */
 
import { Dom } from "/js/util/Dom.js";
import { MapAllUi } from "/js/map/MapAllUi.js";

export class LogsUi {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.report = null;
    this.mapAllUi = null;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const fileRow = this.dom.spawn(this.element, "DIV", ["row", "file"]);
    const fileInput = this.dom.spawn(fileRow, "INPUT", {
      type: "file",
      "on-change": e => this.onOpen(fileInput.files[0]),
    });
    
    const sessionListRow = this.dom.spawn(this.element, "DIV", ["row", "sessionList"]);
    
    const mapRow = this.dom.spawn(this.element, "DIV", ["row"]);
    this.mapAllUi = this.dom.spawnController(mapRow, MapAllUi);
  }
  
  populateUi() {
    this.populateSessionList(this.element.querySelector(".sessionList"), this.report);
  }
  
  populateSessionList(sessionList, report) {
    sessionList.innerHTML = "";
    if (!report || !report.sessions) return;
    for (const session of report.sessions) {
      const row = this.dom.spawn(sessionList, "DIV", ["session"],
        this.reprSession(session),
        { "on-click": () => this.onChooseSession(session, row) }
      );
    }
  }
  
  reprSession(session) {
    let disposition = session.disposition;
    switch (disposition) {
      case "idle-restart": disposition = "idle"; break;
      case "kill-werewolf": disposition = "win!"; break;
    }
    return `${this.reprDuration(session.duration)} ${disposition} ${session.path} ${session.indexInFile}`;
  }
  
  reprDuration(ms) {
    let sec = Math.floor(ms / 1000); ms %= 1000;
    let min = Math.floor(sec / 60); sec %= 60;
    let hour = Math.floor(min / 60); min %= 60;
    if (sec < 10) sec = "0" + sec;
    if (min < 10) min = "0" + min;
    return `${hour}:${min}:${sec}`;
  }
  
  onOpen(file) {
    if (!file) return;
    file.text().then(content => {
      this.report = JSON.parse(content);
      this.populateUi();
    }).catch(e => console.error(e));
  }
  
  onChooseSession(session, row) {
    for (const element of this.element.querySelectorAll(".session.selected")) element.classList.remove("selected");
    if (row) row.classList.add("selected");
    this.mapAllUi.rebuildWithSession(session);
  }
}
