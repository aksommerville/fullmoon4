/* MidiInService.js
 * It's confusing: Yes, we are taking a MIDI stream as input, but no it is not over the MIDI bus.
 * For this arrangement, we have Midevil talking to the bus, and it forwards events through our server, to here.
 */
 
import { DataService } from "./js/game/DataService.js";
import { Synthesizer } from "./js/synth/Synthesizer.js";
 
export class MidiInService {
  static getDependencies() {
    return [Window, DataService, Synthesizer];
  }
  constructor(window, dataService, synthesizer) {
    this.window = window;
    this.dataService = dataService;
    this.synthesizer = synthesizer;
    
    this.listeners = [];
    this.nextListenerId = 1;
    this.status = 0;
    
    this.socket = null;
    const socket = new this.window.WebSocket(`ws://${this.window.location.host}/midi`);
    socket.addEventListener("open", e => {
      this.socket = socket;
    });
    socket.addEventListener("close", e => {
      console.log(`lost WebSocket connection`, e);
      this.socket = null;
    });
    socket.addEventListener("message", e => {
      e.data.arrayBuffer().then(v => this.broadcastEventsFromArrayBuffer(v));
    });
  }
  
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({ cb, id });
    return id;
  }
  
  unlisten(lid) {
    const p = this.listeners.findIndex(({ id }) => id === lid);
    if (p >= 0) this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    for (const { cb } of this.listeners) cb(event);
  }
  
  broadcastEventsFromArrayBuffer(buffer) {
    const src = new Uint8Array(buffer);
    for (let srcp=0; srcp<src.length; ) {
      
      let lead = src[srcp];
      if (lead & 0x80) srcp++;
      else if (this.status & 0x80) lead = this.status;
      else {
        console.log(`Error decoding MIDI. Will try to pick it back up.`);
        srcp++;
        continue;
      }
      this.status = lead;
      
      // Look for the special "config changed" event.
      if ((lead === 0xf0) && (src[srcp] === 0x12) && (src[srcp + 1] === 0x34) && (src[srcp + 2] === 0xf7)) {
        srcp += 3;
        this.dataService.refresh().then(() => this.synthesizer.dropChannels());
        continue;
      }
      
      let chid = lead & 0x0f;
      let opcode = lead & 0xf0;
      let a = 0, b = 0;
      switch (opcode) {
        case 0x80: 
        case 0x90:
        case 0xa0:
        case 0xb0:
        case 0xe0: a = src[srcp++]; b = src[srcp++]; break;
        case 0xc0:
        case 0xd0: a = src[srcp++]; break;
        case 0xf0: opcode = lead; chid = -1; this.status = 0; break;
      }
      this.broadcast([chid, opcode, a, b]);
    }
  }
}

MidiInService.singleton = true;
