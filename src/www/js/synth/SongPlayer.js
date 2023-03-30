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
      this.synthesizer.event(chid, opcode, a, b, this.delay);
      
      if (opcode === 0x90) this.notes.push([chid, a]);
      else if (opcode === 0x80) {
        const p = this.notes.findIndex(n => ((n[0] === chid) && (n[1] === a)));
        if (p >= 0) this.notes.splice(p, 1);
      }
    }
  }
  
  releaseAll() {
    for (const [chid, a] of this.notes) {
      this.synthesizer.event(chid, 0x80, a, 0x40);
    }
    this.notes = [];
  }
  
  resetClock() {
    this.lastUpdateTime = Date.now();
  }
}
