/* MidiInService.js
 * Provide input from Web MIDI API, and also the WebSocket strategy that the original Full Moon Fiddle used.
 * WebSocket is not yet implemented by our backend, so it's stubbed out here and will probably change somewhat as I enable it.
 */
 
//import { DataService } from "./js/game/DataService.js";
//import { Synthesizer } from "./js/synth/Synthesizer.js";
 
export class MidiInService {
  static getDependencies() {
    return [Window, /*DataService, Synthesizer*/];
  }
  constructor(window, /*dataService, synthesizer*/) {
    this.window = window;
    //this.dataService = dataService;
    //this.synthesizer = synthesizer;
    
    this.listeners = [];
    this.nextListenerId = 1;
    this.status = 0;
    
    this.socket = null;
    /*
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
    /**/
    
    this.midiAccess = null;
    this.pollPending = false;
    this.inputListeners = [];
    if (this.window.navigator.requestMIDIAccess) {
      this.window.navigator.requestMIDIAccess().then(access => {
        this.midiAccess = access;
        this.midiAccess.addEventListener("statechange", (event) => this.onStateChange(event));
        this.onStateChange(null);
      }).catch(error => {
        console.error(`MIDI access denied`, error);
      });
    } else {
      console.error(`MIDI bus not available`);
    }
  }
  
  /* Event dispatch.
   * Each MIDI event is an array: [chid, opcode, a, b]
   * Other events are objects with a 'type' string:
   *   {type:"devicesChanged"}
   ***************************************************************/
  
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
  
  /* Receive raw MIDI stream.
   * TODO This uses a shared Running Status across all input devices, which is definitely wrong.
   *********************************************************************/
  
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
        //this.dataService.refresh().then(() => this.synthesizer.dropChannels());
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
  
  /* Events from MIDI Bus.
   ************************************************************/
   
  onMidiMessage(event) {
    /*
    if ((this.inputChid >= 0) && (this.inputChid < 0x10)) {
      if ((event.data.length >= 1) && (event.data[0] >= 0x80) && (event.data[0] < 0xf0)) {
        const newData = new Uint8Array(event.data);
        newData[0] = (newData[0] & 0xf0) | this.inputChid;
        event = {
          target: event.target,
          data: newData,
        };
      }
    }
    this.dispatchEvent(new MidiEvent(event.target, event.data));
    if (this.playthroughDevice) {
      this.playthroughDevice.send(event.data);
    } else if (this.playthroughSocket) {
      const serial = new Uint8Array(event.data);
      this.playthroughSocket.send(event.data);
    }
    /**/
    this.broadcastEventsFromArrayBuffer(event.data.buffer);
  }
  
  /* MIDI Bus state changes.
   *******************************************************/
  
  // When I connect a device, I get half a dozen state change events (Linux, Chrome, MPK225). Debounce.
  onStateChange(event) {
    this.pollDevicesSoon();
  }
  
  pollDevicesSoon() {
    if (this.pollPending) return;
    this.pollPending = true;
    this.window.setTimeout(() => {
      this.pollPending = false;
      this.pollDevicesNow();
    }, 50);
  }
  
  pollDevicesNow() {
    for (const [device, cb] of this.inputListeners) {
      device.removeEventListener("midimessage", cb);
    }
    this.inputListeners = [];
    if (this.midiAccess) {
      for (const [id, device] of this.midiAccess.inputs) {
        const cb = (event) => this.onMidiMessage(event);
        device.addEventListener("midimessage", cb);
        this.inputListeners.push([device, cb]);
      }
    }
    this.broadcast({ type: "devicesChanged" });
  }
}

MidiInService.singleton = true;
