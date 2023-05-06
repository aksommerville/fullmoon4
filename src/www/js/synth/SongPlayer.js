/* SongPlayer.js
 * Coordinates playback of one song.
 * When the song changes, we kill its player and make a new one.
 */
 
export class SongPlayer {
  constructor(synthesizer, song) {
    this.synthesizer = synthesizer;
    this.song = song;
    this.delay = 0; // ms
    this.lastUpdateTime = 0; // ms
    this.eventp = 0;
    this.msPerTick = 1000 / this.song.ticksPerSecond;
    this.notes = []; // as a safety net, we track all held notes
    this.foresightTime = 100; // Read ahead in time, up to so many ms. Zero is valid.
    this.enabled = true; // We track time regardless, but if disabled will not emit any events.
  }
  
  update() {
    const now = Date.now();
    if (this.lastUpdateTime) {
      const elapsedMs = now - this.lastUpdateTime;
      // Delay can go negative. It's cool, it helps mitigate rounding error. i think.
      this.delay -= elapsedMs;
    }
    this.lastUpdateTime = now;
    while (this.delay <= this.foresightTime) {
      this._nextEvent();
    }
  }
  
  reset() {
    this.releaseAll();
    this.delay = 0;
    this.lastUpdateTime = 0;
    this.eventp = 0;
  }
  
  enable(enable) {
    if (enable) {
      if (this.enabled) return;
      this.enabled = true;
    } else {
      if (!this.enabled) return;
      this.enabled = false;
      this.releaseAll();
    }
  }
  
  _nextEvent() {
    if (this.song.events.length < 1) return;
  
    // End of song. Drop all notes, reset the event pointer, and hold all else steady.
    if (this.eventp >= this.song.events.length) {
      this.eventp = 0;
      this.releaseAll();
    }
    
    const event = this.song.events[this.eventp++];
    if (event & 0x80000000) {
      this.delay += this.msPerTick * (event & 0x7fffffff);
    } else {
    
      const chid = (event >> 24) & 0x7f;
      const opcode = (event >> 16) & 0xff;
      const a = (event >> 8) & 0x7f;
      const b = event & 0x7f;
      // Note On are only emitted when enabled. All other events flow through regardless.
      // (eg it's important that the synthesizer capture program and control changes, in case music turns back on later).
      if (this.enabled || (opcode !== 0x90)) {
        this.synthesizer.event(chid, opcode, a, b, this.delay);
      }
      
      if (opcode === 0x90) this.notes.push([chid, a]);
      else if (opcode === 0x80) {
        const p = this.notes.findIndex(n => ((n[0] === chid) && (n[1] === a)));
        if (p >= 0) this.notes.splice(p, 1);
      }
    }
  }
  
  releaseAll() {
    // Releasing voices happens regardless of (enabled).
    for (const [chid, a] of this.notes) {
      this.synthesizer.event(chid, 0x80, a, 0x40);
    }
    this.notes = [];
  }
  
  resetClock() {
    this.lastUpdateTime = Date.now();
  }
}
