/* FiddleService.js
 * Cache and dispatcher for our backend API.
 * We're a pretty small app, so our business layer is just this one class.
 *
 * Events:
 *  {type:"status",status:{synth,song}} # can fire redundantly
 *  {type:"synths",synths:[qualifier,...]}
 *  {type:"sounds",sounds:[{id,name},...]}
 *  {type:"instruments",instruments:[{id,name},...]}
 *  {type:"songs",songs:[{id,name},...]}
 *  {type:"loading"}
 *  {type:"loaded"} # success
 *  {type:"loaded",error}
 *  {type:"websocket",data}
 *  {type:"websocketClose"}
 */
 
import { Comm } from "./util/Comm.js";

export class FiddleService {
  static getDependencies() {
    return [Comm];
  }
  constructor(comm) {
    this.comm = comm;
    
    this.listeners = [];
    this.nextListenerId = 1;
    this.pendingRefresh = null; // null|Promise
    this.status = { synth: null, song: 0 };
    this.pendingStatus = null; // null|Promise
    this.socket = null;
    
    this.synths = []; // qualifier(string)
    this.sounds = []; // {id,name}
    this.instruments = []; // {id,name}
    this.songs = []; // {id,name}
    
    this.socket = this.comm.connectWebsocket("/websocket");
    this.socket.addEventListener("message", e => this.broadcast({ type: "websocket", data: e.data }));
    this.socket.addEventListener("close", e => {
      this.socket = null;
      this.broadcast({ type: "websocketClose" });
    });
  }
  
  /* General signalling.
   **************************************************************/
  
  // Optional (type) to receive only those events. Unset to receive all.
  listen(cb, type) {
    const id = this.nextListenerId++;
    this.listeners.push({cb, id});
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) this.listeners.splice(p, 1);
  }
  
  broadcast(event) {
    if (!event || (typeof(event) !== "object")) return;
    for (const { cb, type } of this.listeners) {
      if (type && (type !== event.type)) continue;
      cb(event);
    }
  }
  
  /* Resource lists.
   ***************************************************************/
   
  dropToc() {
    if (this.synths.length) {
      this.synths = [];
      this.broadcast({ type: "synths", synths: this.synths });
    }
    if (this.sounds.length) {
      this.sounds = [];
      this.broadcast({ type: "sounds", sounds: this.sounds });
    }
    if (this.songs.length) {
      this.songs = [];
      this.broadcast({ type: "songs", songs: this.songs });
    }
    if (this.instruments.length) {
      this.instruments = [];
      this.broadcast({ type: "instruments", instruments: this.instruments });
    }
  }
  
  refreshToc() {
    if (this.pendingRefresh) return this.pendingRefresh;
    this.dropToc();
    
    this.pendingRefresh = this.comm.get("/api/synths")
      .then(rsp => rsp.json())
      .then(synths => {
        this.synths = synths;
        this.broadcast({ type: "synths", synths: this.synths });
      })
      .then(() => Promise.all([
        ...this.synths.map(q => this.comm.get("/api/sounds", { qualifier: q }).then(rsp => rsp.json()).then(sounds => this._addSounds(q, sounds))),
        ...this.synths.map(q => this.comm.get("/api/instruments", { qualifier: q }).then(rsp => rsp.json()).then(ins => this._addInstruments(q, ins))),
        this.comm.get("/api/songs").then(rsp => rsp.json()).then(songs => this.songs = songs),
        this.comm.get("/api/status").then(rsp => rsp.json()).then(status => this.status = status),
      ]))
      .then(() => {
        this.pendingRefresh = null;
        if (this.sounds.length) this.broadcast({ type: "sounds", sounds: this.sounds });
        if (this.instruments.length) this.broadcast({ type: "instruments", instruments: this.instruments });
        if (this.songs.length) this.broadcast({ type: "songs", songs: this.songs });
        this.broadcast({ type: "status", status: this.status });
        this.broadcast({ type: "loaded" });
      })
      .catch(error => {
        this.pendingRefresh = null;
        this.broadcast({ type: "loaded", error: error || "unspecified" });
      });
      
    this.broadcast({ type: "loading" });
    return this.pendingRefresh;
  }
  
  _addSounds(qualifier, sounds) {
    for (const { id, name } of sounds) {
      const p = this.sounds.findIndex(s => s.id === id);
      if (p < 0) this.sounds.push({ id, name });
      else if (!this.sounds[p].name) this.sounds[p].name = name;
    }
  }
  
  _addInstruments(qualifier, instruments) {
    for (const { id, name } of instruments) {
      const p = this.instruments.findIndex(s => s.id === id);
      if (p < 0) this.instruments.push({ id, name });
      else if (!this.instruments[p].name) this.instruments[p].name = name;
    }
  }
  
  /* Backend status.
   ***************************************************/
   
  refreshStatus() {
    if (this.pendingStatus) return this.pendingStatus;
    this.pendingStatus = this.comm.get("/api/status")
      .then(rsp => rsp.json())
      .then(status => {
        this.status = status;
        this.pendingStatus = null;
        this.broadcast({ type: "status", status: this.status });
        return this.status;
      })
      .catch(error => {
        this.pendingStatus = null;
        throw error;
      });
    return this.pendingStatus;
  }
  
  /* Websocket support.
   ********************************************************/
   
  sendMidiViaWebsocket(event) {
    if (!this.socket) return false;
    this.socket.send(event);
    return true;
  }
}

FiddleService.singleton = true;
