/* RootUi.js
 * Top of the view hierarchy.
 */
 
import { Dom } from "../util/Dom.js";
import { FiddleService } from "../FiddleService.js";
import { Comm } from "../util/Comm.js";
import { MidiInService } from "../util/MidiInService.js";
import { VumeterUi } from "./VumeterUi.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom, FiddleService, Comm, MidiInService];
  }
  constructor(element, dom, fiddleService, comm, midiInService) {
    this.element = element;
    this.dom = dom;
    this.fiddleService = fiddleService;
    this.comm = comm;
    this.midiInService = midiInService;
    
    this.fiddleServiceListener = this.fiddleService.listen(e => this.onFiddleEvent(e));
    this.midiInServiceListener = this.midiInService.listen(e => this.onMidiEvent(e));
    this.vumeter = null;
    
    this.buildUi();
    
    this.fiddleService.refreshToc();
  }
  
  onRemoveFromDom() {
    this.fiddleService.unlisten(this.fiddleServiceListener);
    this.midiInService.unlisten(this.midiInServiceListener);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const synthsRow = this.dom.spawn(this.element, "DIV");
    this.dom.spawn(synthsRow, "SELECT", ["synth"], { "on-change": () => this.chooseSynth() });
    const soundsRow = this.dom.spawn(this.element, "DIV");
    this.dom.spawn(soundsRow, "SELECT", ["sound"], { "on-change": () => this.playSound() });
    this.dom.spawn(soundsRow, "INPUT", { type: "button", value: "Play", "on-click": () => this.playSound() });
    const instrumentsRow = this.dom.spawn(this.element, "DIV");
    this.dom.spawn(instrumentsRow, "SELECT", ["instrument"], { "on-change": () => this.instrumentChanged() });
    const songsRow = this.dom.spawn(this.element, "DIV");
    this.dom.spawn(songsRow, "SELECT", ["song"], { "on-change": () => this.playSong() });
    const controlsRow = this.dom.spawn(this.element, "DIV");
    this.dom.spawn(controlsRow, "INPUT", { type: "button", value: "PANIC", "on-click": () => this.panic() });
    this.dom.spawn(this.element, "PRE", ["error"]);
    this.vumeter = this.dom.spawnController(this.element, VumeterUi);
  }
  
  populateSynths(synths) {
    const select = this.element.querySelector(".synth");
    select.innerHTML = "";
    this.dom.spawn(select, "OPTION", { value: "" }, "No synthesizer");
    for (const name of synths) {
      this.dom.spawn(select, "OPTION", { value: name }, name);
    }
  }
  
  populateSounds(sounds) {
    const select = this.element.querySelector(".sound");
    select.innerHTML = "";
    this.dom.spawn(select, "OPTION", { value: 0 }, "No sound");
    for (const { id, name } of sounds) {
      this.dom.spawn(select, "OPTION", { value: id }, `${id}: ${name}`);
    }
  }
  
  populateInstruments(instruments) {
    const select = this.element.querySelector(".instrument");
    select.innerHTML = "";
    this.dom.spawn(select, "OPTION", { value: 0 }, "No instrument");
    for (const { id, name } of instruments) {
      this.dom.spawn(select, "OPTION", { value: id }, `${id}: ${name}`);
    }
  }
  
  populateSongs(songs) {
    const select = this.element.querySelector(".song");
    select.innerHTML = "";
    this.dom.spawn(select, "OPTION", { value: 0 }, "No song");
    for (const { id, name } of songs) {
      this.dom.spawn(select, "OPTION", { value: id }, `${id}: ${name}`);
    }
  }
  
  onMidiEvent(event) {
    //console.log(`RootUi.onMidiEvent`, event);
    if (event instanceof Array) {
      if (!this.fiddleService.sendMidiViaWebsocket(new Uint8Array([0x0e, event[1], event[2], event[3]]))) {
        this.comm.post("/api/midi", { chid: 0x0e, opcode: event[1], a: event[2], b: event[3] })
          .catch(e => this.reportError(e));
      }
    }
  }
  
  onFiddleEvent(event) {
    //console.log(`RootUi.onFiddleEvent`, event);
    switch (event.type) {
      case "status": this.selectSynthAndSongPerStatus(event.status); break;
      case "synths": this.populateSynths(event.synths); break;
      case "sounds": this.populateSounds(event.sounds); break;
      case "instruments": this.populateInstruments(event.instruments); break;
      case "songs": this.populateSongs(event.songs); break;
      case "loaded": if (event.error) this.reportError(event.error); break;
      case "websocket": if (typeof(event.data) === "string") {
          try {
            const srcevent = JSON.parse(event.data);
            this.onBackendEvent(srcevent);
          } catch (e) {
            this.reportError(e);
          }
        } break;
      case "websocketClosed": if (this.vumeter) this.vumeter.end(); break;
    }
  }
  
  onBackendEvent(event) {
    switch (event.type) {
      case "vu": if (this.vumeter) this.vumeter.update(event); break;
    }
  }
  
  reportError(error) {
    console.error(error);
    this.element.querySelector(".error").innerText = error.message || JSON.stringify(error, 0, 2);
  }
  
  selectSynthAndSongPerStatus(status) {
    if (!status) return;
    this.element.querySelector(".synth").value = status.synth || "";
    this.element.querySelector(".song").value = status.song || 0;
    this.element.querySelector(".instrument").value = status.instrument || 0;
    this.element.querySelector(".sound").value = status.sound || 0;
  }
  
  chooseSynth() {
    const id = this.element.querySelector(".synth").value;
    this.comm.post("/api/synth/use", { qualifier: id }).catch(e => this.reportError(e));
    if (this.vumeter) this.vumeter.end();
  }
  
  playSound() {
    const id = this.element.querySelector(".sound").value;
    this.comm.post("/api/sound/play", { id }).catch(e => this.reportError(e));
  }
  
  playSong() {
    const id = this.element.querySelector(".song").value;
    this.comm.post("/api/song/play", { id }).catch(e => this.reportError(e));
  }
  
  instrumentChanged() {
    const id = this.element.querySelector(".instrument").value;
    this.comm.post("/api/midi", { chid: 0x0e, opcode: 0xc0, a: id }).catch(e => this.reportError(e));
  }
  
  panic() {
    this.comm.post("/api/midi", { opcode: 0xff }).catch(e => this.reportError(e));
  }
}
