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
  }
  
  onRemoveFromDom() {
    this.midiInService.unlisten(this.midiInListener);
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "INPUT", {
      type: "button",
      value: "Reset Synthesizer",
      "on-click": () => this.onReset(),
    });
    this.dom.spawn(this.element, "INPUT", {
      type: "button",
      value: "Silence",
      "on-click": () => this.onSilence(),
    });
  }
  
  registerForUpdate() {
    this.window.requestAnimationFrame(() => {
      this.update();
      this.registerForUpdate();
    });
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
}
