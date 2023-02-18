/* RootUi.js
 * This will hopefully also be "OnlyUi.js", we're not a big UI app.
 */
 
import { Dom } from "./js/util/Dom.js";
import { Synthesizer } from "./js/synth/Synthesizer.js";
import { DataService } from "./js/game/DataService.js";
import { MidiInService } from "./MidiInService.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Synthesizer, DataService, Dom, Window, MidiInService];
  }
  constructor(element, synthesizer, dataService, dom, window, midiInService) {
    this.element = element;
    this.synthesizer = synthesizer;
    this.dataService = dataService;
    this.dom = dom;
    this.window = window;
    this.midiInService = midiInService;
    
    this.buildUi();
    this.registerForUpdate();
    this.dataService.load()
      .then(() => console.log(`data loaded`))
      .catch((e) => console.error(`failed to load data`, e));
    this.midiInListener = this.midiInService.listen(e => this.synthesizer.event(...e));
    
    this.window.fetch("/api/soundNames").then((rsp) => {
      if (!rsp.ok) throw rsp;
      return rsp.json();
    }).then((toc) => this.setSoundEffectButtonNames(toc))
    .catch((e) => console.error(`Failed to fetch sound effect names`, e));
  }
  
  onRemoveFromDom() {
    this.midiInService.unlisten(this.midiInListener);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const buttonsRow = this.dom.spawn(this.element, "DIV", ["buttonsRow"]);
    this.dom.spawn(buttonsRow, "INPUT", {
      type: "button",
      value: "Reset Synthesizer",
      "on-click": () => this.onReset(),
    });
    this.dom.spawn(buttonsRow, "INPUT", {
      type: "button",
      value: "Silence",
      "on-click": () => this.onSilence(),
    });
    
    const settingsTable = this.dom.spawn(this.element, "TABLE", ["settings"]);
    this.spawnSettingsRow(settingsTable, "Instrument", () => this.onInstrumentChange());
    
    this.dom.spawn(this.element, "H2", "Sound effects");
    const soundsTable = this.dom.spawn(this.element, "TABLE", ["sounds"]);
    const soundsPerRow = 8;
    for (let rowStart=0; rowStart<128; rowStart+=soundsPerRow) {
      const tr = this.dom.spawn(soundsTable, "TR");
      for (let i=0; i<soundsPerRow; i++) {
        const sndid = rowStart + i;
        const td = this.dom.spawn(soundsTable, "TD");
        const button = this.dom.spawn(td, "INPUT", {
          type: "button",
          value: sndid,
          "on-click": () => this.playSound(sndid),
          "data-sndid": sndid,
        });
      }
    }
  }
  
  spawnSettingsRow(table, label, cbChanged) {
    const tr = this.dom.spawn(table, "TR");
    const tdKey = this.dom.spawn(table, "TD", label);
    const tdValue = this.dom.spawn(table, "TD");
    const input = this.dom.spawn(tdValue, "INPUT", {
      type: "number",
      name: label,
      "on-change": cbChanged,
    });
    return input;
  }
  
  registerForUpdate() {
    this.window.requestAnimationFrame(() => {
      this.update();
      this.registerForUpdate();
    });
  }
  
  setSoundEffectButtonNames(toc) {
    for (const [name, id] of toc) {
      const button = this.element.querySelector(`input[data-sndid="${id}"]`);
      if (button) button.value = name;
    }
  }
  
  update() {
    this.synthesizer.update();
  }
  
  onReset() {
    this.synthesizer.reset();
  }
  
  onSilence() {
    this.synthesizer.silenceAll();
  }
  
  playSound(sndid) {
    this.synthesizer.event(0x0f, 0x90, sndid, 0x40);
  }
  
  onInstrumentChange() {
    const pid = +this.element.querySelector("input[name='Instrument']").value;
    this.synthesizer.overrideAllInstruments(pid);
  }
}
