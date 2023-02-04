/* Song.js
 * Immutable-ish representation of a MIDI file.
 * At decode, we parse the whole file and produce a single stream of events.
 */
 
export class Song {
  constructor(src) {
  
    this.division = 0; // ticks/qnote, 1..32767
    this.events = []; // (chid<<24)|(opcode<<16)|(a<<8)|b or 0x80000000|ticks
    this.usPerQnote = 500000;
    this.ticksPerSecond = 0;
    
    if (!src) ;
    else if (src instanceof Uint8Array) this._decode(src);
    else if (src instanceof ArrayBuffer) this._decode(new Uint8Array(src));
    else if (src instanceof Song) this._copy(src);
    else throw new Error(`Unexpected input for Song`);
  }
  
  _copy(src) {
    this.division = src.division;
    this.usPerQnote = src.usPerQnote;
    this.ticksPerSecond = src.ticksPerSecond;
    this.events = [...src.events];
  }
  
  _decode(src) {
    
    this.division = 0;
    this.usPerQnote = 500000;
    const mtrk = [];
    
    for (let srcp=0; ; ) {
      if (srcp > src.length - 8) break;
      const chunkId = (src[srcp] << 24) | (src[srcp + 1] << 16) | (src[srcp + 2] << 8) | src[srcp + 3];
      const chunkLength = (src[srcp + 4] << 24) | (src[srcp + 5] << 16) | (src[srcp + 6] << 8) | src[srcp + 7];
      srcp += 8;
      if ((chunkLength < 0) || (srcp > src.length - chunkLength)) {
        throw new Error(`Invalid chunk length ${chunkLength} in Song around ${srcp}/${src.length}`);
      }
      switch (chunkId) {
      
        case Song.CHUNKID_MThd: {
            if (this.division) throw new Error(`Multiple MThd`);
            if (chunkLength < 6) throw new Error(`Invalid MThd length ${chunkLength}`);
            this.division = (src[srcp + 4] << 8) | src[srcp + 5];
            if (!this.division || (this.division & 0x8000)) throw new Error(`Invalid MThd division ${this.division}`);
          } break;
          
        case Song.CHUNKID_MTrk: {
            const view = new Uint8Array(src.buffer, src.byteOffset + srcp, chunkLength);
            mtrk.push(view);
          }
          
        // Unknown chunks are fine, ignore them.
      }
      srcp += chunkLength;
    }
    this._decodeEvents(mtrk);
    this._requireTempo();
  }
  
  _decodeEvents(mtrk) {
    if (!this.division) throw new Error(`Missing MThd`);
    const tracks = mtrk.map(v => ({
      p: 0,
      v,
      delay: -1,
      done: false,
      status: 0,
    }));
    this.events = [];
    for (;;) {
      const shortestDelay = this._requireDelays(tracks);
      if (shortestDelay < 0) break;
      if (shortestDelay) {
        // combine delay events if possible
        if (this.events[this.events.length - 1] & 0x80000000) this.events[this.events.length - 1] += shortestDelay;
        else this.events.push(0x80000000 | shortestDelay);
        this._dropDelays(tracks, shortestDelay);
      }
      this._readEvents(this.events, tracks);
    }
  }
  
  /* Ensure that each track has a zero or positive delay, reading from (v) if necessary.
   * Returns the shortest delay, or <0 if complete.
   */
  _requireDelays(tracks) {
    let result = -1;
    for (const track of tracks) {
      if (track.done) continue;
      if (track.delay < 0) {
        if (track.p >= track.v.length) {
          track.done = true;
          continue;
        }
        this._readDelay(track);
      }
      if (result < 0) result = track.delay;
      else if (track.delay < result) result = track.delay;
    }
    return result;
  }
  
  _readDelay(track) {
    if (track.p >= track.v.length) {
      track.done = true;
    } else {
      const [delay, len] = this._readVlq(track.v, track.p);
      track.p += len;
      track.delay = delay;
    }
  }
  
  _dropDelays(tracks, ticks) {
    for (const track of tracks) {
      if (track.done) continue;
      track.delay -= ticks;
      if (track.delay < 0) throw new Error(`Track delay fell below zero during clock advance.`); // shouldn't be possible
    }
  }
  
  _readEvents(dst, tracks) {
    for (const track of tracks) {
      while (!track.done && !track.delay) {
        if (track.p >= track.v.length) {
          // Error but whatever.
          track.done = true;
          continue;
        }
      
        let lead = track.v[track.p];
        if (lead & 0x80) track.p++;
        else if (track.status & 0x80) lead = track.status;
        else throw new Error(`Expected status byte around ${track.p}/${track.v.length} in MTrk`);
        track.status = lead;
      
        let opcode = lead & 0xf0;
        let chid = lead & 0x0f;
        let a = 0, b = 0;
        const paramAb = () => {
          if (track.p > track.v.length - 2) throw new Error(`Overrun around ${track.p}/${track.v.length} in MTrk`);
          a = track.v[track.p++];
          b = track.v[track.p++];
        };
        const paramA = () => {
          if (track.p >= track.v.length) throw new Error(`Overrun around ${track.p}/${track.v.length} in MTrk`);
          a = track.v[track.p++];
        };
        switch (opcode) {
          case 0x80: paramAb(); break;
          case 0x90: paramAb(); if (!b) { opcode = 0x80; b = 0x40; } break;
          case 0xa0: paramAb(); break;
          case 0xb0: paramAb(); break;
          case 0xc0: paramA(); break;
          case 0xd0: paramA(); break;
          case 0xe0: paramAb(); break;
          case 0xf0: track.status = 0; opcode = lead; chid = 0x7f; switch (lead) {
              case 0xff: {
                  paramA();
                } // pass
              case 0xf0: case 0xf7: {
                  const [len, lenlen] = this._readVlq(track.v, track.p);
                  track.p += lenlen;
                  if (track.p > track.v.length - len) throw new Error(`Illegal Meta or Sysex length ${len} at ${track.p}/${track.v.length}`);
                  switch (a) {
                    // All Meta and Sysex events are global in scope (across channels but also across time).
                    // We do not have a means of encoding them in the event stream, intentionally.
                  
                    case 0x51: { // Set Tempo
                        if (len >= 3) {
                          this.usPerQnote = (track.v[track.p] << 16) | (track.v[track.p + 1] << 8) | track.v[track.p + 2];
                          if (!this.usPerQnote) this.usPerQnote = 500000;
                        }
                      } break;
                      
                  }
                  track.p += len;
                  opcode = 0;
                } break;
              default: throw new Error(`Unexpected opcode ${opcode}`);
            } break;
          default: throw new Error(`Unexpected opcode ${opcode}`);
        }
      
        if (opcode) dst.push((chid << 24) | (opcode << 16) | (a << 8) | b);
        this._readDelay(track);
      }
    }
  }
  
  // => [v,len]
  _readVlq(src, p) {
    let v = 0, len = 0;
    while (len < 4) {
      len++;
      if (p >= src.length) throw new Error(`Unexpected EOF reading VLQ`);
      v <<= 7;
      v |= src[p] & 0x7f;
      if (!(src[p] & 0x80)) return [v, len];
      p++;
    }
    throw new Error(`VLQ length greater than 4`);
  }
  
  _requireTempo() {
    if (!this.division || !this.usPerQnote) throw new Error(`Invalid tempo inputs division=${this.division} usPerQnote=${this.usPerQnote}`);
    this.ticksPerSecond = (this.division * 1000000) / this.usPerQnote;
  }
}

Song.CHUNKID_MThd = 0x4d546864;
Song.CHUNKID_MTrk = 0x4d54726b;
