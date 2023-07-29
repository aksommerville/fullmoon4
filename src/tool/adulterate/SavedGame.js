module.exports = class SavedGame {
  constructor(src, path) {
    this.src = src;
    this.path = path || "<unknown>";
    this.clearContent();
    this.dirty = false; // SavedGame never touches its own dirty flag, except right here.
  }
  
  clearContent() {
    this.spellid = 0;
    this.game_time_ms = 0;
    this.damage_count = 0;
    this.transmogrification = 0;
    this.selected_item = 0;
    this.itemv = 0; // 16 bits
    this.itemqv = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]; // Array or Uint8Array
    this.gs = new Uint8Array([]); // Bits indexed big-endianly.
    this.plants = []; // {mapid,cellp,state,fruit,flower_time}
    this.sketches = []; // {mapid,cellp,bits}
    this.tail = new Uint8Array([]); // Should be empty. May begin with an EOF chunk, and unknown content after.
    this.unknown = []; // Uint8Array for each unknown chunk, including header. Order with respect to known chunks is not preserved.
  }
  
  validateSignature() {
    if (this.src.length < 29) throw new Error(`${this.path}: Minimum length 29, found ${this.src.length}`);
    if ((this.src[0] !== 0x01) || (this.src[1] !== 0x1b)) {
      throw new Error(`${SAVEFILE}: Not a Full Moon saved game. Expected leading bytes (1,27), found (${src[0]},${src[1]})`);
    }
  }
  
  setField(k, v) {
    k = k.trim().toLowerCase();
    v = v.trim().toLowerCase();
    
    // Simple scalars, as defined in our fields.
    switch (k) {
      case "spellid": this.spellid = this.evalSpell(v); return;
      case "game_time_ms": this.game_time_ms = this.evalTime(v); return;
      case "damage_count": this.damage_count = this.evalInt(v, 0, 0xffff); return;
      case "transmogrification": this.transmogrification = this.evalTransmogrification(v); return;
      case "selected_item": this.selected_item = this.evalItem(v); return;
      case "itemv": this.itemv = this.evalItemBits(v); return;
      // "itemv+" or "itemv-" to set or clear item bits.
      case "itemv+": this.itemv |= this.evalItemBits(v); return;
      case "itemv-": this.itemv &= ~this.evalItemBits(v); return;
      // itemqv, gs, plants, sketches, tail, unknown: We allow clearing the whole thing, but not (yet?) inserting any value.
      case "itemqv": if (!v) this.itemqv = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]; else throw new Error(`not settable except empty (TODO?)`); return;
      case "gs": if (!v) this.gs = new Uint8Array([]); else throw new Error(`not settable except empty (TODO?)`); return;
      case "plants": if (!v) this.plants = []; else throw new Error(`not settable except empty (TODO?)`); return;
      case "sketches": if (!v) this.sketches = []; else throw new Error(`not settable except empty (TODO?)`); return;
      case "tail": if (!v) this.tail = new Uint8Array([]); else throw new Error(`not settable except empty (TODO?)`); return;
      case "unknown": if (!v) this.unknown = []; else throw new Error(`not settable except empty (TODO?)`); return;
    }
    
    // Item qualifiers.
    switch (k) {
      case "pitcher": this.itemqv[2] = this.evalPitcher(v); return;
      case "seeds": this.itemqv[3] = this.evalInt(v, 0, 0xff); return;
      case "coins": this.itemqv[4] = this.evalInt(v, 0, 0xff); return;
      case "cheese": this.itemqv[15] = this.evalInt(v, 0, 0xff); return;
    }
    
    // gs individual bits.
    let match = k.match(/^gs\[(\d+)]$/);
    if (match) return this.setGsBitByIndex(+match[1], v);
    match = k.match(/^gs\.([a-zA-Z_]+)$/);
    if (match) return this.setGsBitByName(match[1], v);
    
    throw new Error(`unexpected key`);
  }
  
  evalSpell(v) {
    // 8-bit integers are always ok.
    const n = +v;
    if (!isNaN(n) && (n >= 0) && (n <= 0xff)) return n;
    // Spell names or aliases, or the combined form as reported. Only spells we expect to be used.
    switch (v) {
      case "": return 0;
      case "home": case "forest": case "home:forest": return 8;
      case "tele1": case "beach": case "tele1:beach": return 9;
      case "tele2": case "swamp": case "tele2:swamp": return 10;
      case "tele3": case "mountains": case "tele3:mountains": return 11;
      case "tele4": case "castle": case "tele4:castle": return 12;
      case "tele5": case "desert": case "tele5:desert": return 22;
      case "tele6": case "steppe": case "tele6:steppe": return 23;
    }
    throw new Error(`invalid spell`);
  }
  
  evalTime(v) {
    let match;
    if (!v) return 0;
    if (match = v.match(/^\d+$/)) return +v;
    if (match = v.match(/^(\d+):(\d+)(\.(\d+))?$/)) return +v[1] * 60000 + +v[2] * 1000 + (+v[3] || 0);
    if (match = v.match(/^(\d+):(\d+):(\d+)(\.(\d+))?$/)) return +v[1] * 3600000 + +v[2] * 60000 + +v[3] * 1000 + (+v[4] || 0);
    throw new Error(`invalid time`);
  }
  
  evalInt(v, lo, hi) {
    v = +v;
    if (isNaN(v)) throw new Error(`expected integer in ${lo}..${hi}`);
    if (v < lo) throw new Error(`min ${lo}`);
    if (v > hi) throw new Error(`max ${hi}`);
    return v;
  }
  
  evalTransmogrification(v) {
    if (!v) return 0;
    switch (v) {
      case "normal": return 0;
      case "pumpkin": return 1;
    }
    return evalInt(v, 0, 0xff);
  }
  
  evalItem(v) {
    switch (v) {
      case "snowglobe": return 0;
      case "hat": return 1;
      case "pitcher": return 2; 
      case "seed": return 3;
      case "coin": return 4;
      case "match": return 5;
      case "broom": return 6;
      case "wand": return 7;
      case "umbrella": return 8;
      case "feather": return 9;
      case "shovel": return 10;
      case "compass": return 11;
      case "violin": return 12;
      case "chalk": return 13;
      case "bell": return 14;
      case "cheese": return 15;
    }
    return evalInt(v, 0, 15);
  }
  
  evalItemBits(v) {
    let bits = 0;
    for (const sub of v.split(/[\s,]+/g)) bits |= 1 << this.evalItem(sub);
    return bits;
  }
  
  evalPitcher(v) {
    if (!v) return 0;
    switch (v) {
      case "empty": return 0;
      case "water": return 1;
      case "milk": return 2;
      case "honey": return 3;
      case "sap": return 4;
    }
    return evalInt(v, 0, 0xff);
  }
  
  setGsBitByIndex(p, v) {
    v = this.evalInt(v, 0, 1);
    const bytep = p >> 3;
    if ((bytep >= 0) && (bytep < this.gs.length)) {
      const mask = 0x80 >> (p & 7);
      if (v) this.gs[bytep] |= mask;
      else this.gs[bytep] &= ~mask;
      return;
    }
    throw new Error(`set gsbit outside existing range not yet implemented (limit ${this.gs.length << 3})`);
  }
  
  setGsBitByName(name, v) {
    throw new Error(`gsbit name lookup not yet implemented`);
  }
  
  /* Replace (src) by reading from content fields.
   */
  encode() {
    this.dstc = 0;
    this.dsta = 1024;
    this.dst = new Uint8Array(this.dsta);
    
    this._appendDst([ // FIXED01
      0x01, 27,
      this.spellid,
      this.game_time_ms >> 24, (this.game_time_ms >> 16) & 0xff, (this.game_time_ms >> 8) & 0xff, this.game_time_ms & 0xff,
      (this.damage_count >> 8) & 0xff, this.damage_count & 0xff,
      this.transmogrification,
      this.selected_item,
      (this.itemv >> 8) & 0xff, this.itemv & 0xff,
      this.itemqv[0], this.itemqv[1], this.itemqv[2], this.itemqv[3],
      this.itemqv[4], this.itemqv[5], this.itemqv[6], this.itemqv[7],
      this.itemqv[8], this.itemqv[9], this.itemqv[10], this.itemqv[11],
      this.itemqv[12], this.itemqv[13], this.itemqv[14], this.itemqv[15],
    ]);
    
    for (let gsp=0; gsp<this.gs.length; ) {
      if (!this.gs[gsp]) {
        gsp++;
        continue;
      }
      let cpc = 1;
      // ideally, we'd stop at runs of 5 or more zeroes. this stops at every zero.
      while ((cpc < 253) && (gsp + cpc < this.gs.length) && this.gs[cpc]) cpc++;
      this._appendDst([ // GSBIT
        0x02, 2 + cpc,
        (gsp >> 8) & 0xff, gsp & 0xff,
        ...this.gs.slice(gsp, gsp + cpc),
      ]);
      gsp += cpc;
    }
    
    for (const plant of this.plants) {
      this._appendDst([ // PLANT
        0x03, 9,
        plant.mapid >> 8, plant.mapid & 0xff,
        plant.cellp,
        plant.state,
        plant.fruit,
        plant.flower_time >> 24, (plant.flower_time >> 16) & 0xff, (plant.flower_time >> 8) & 0xff, plant.flower_time & 0xff,
      ]);
    }
    
    for (const sketch of this.sketches) {
      this._appendDst([ // SKETCH
        0x04, 6,
        sketch.mapid >> 8, sketch.mapid & 0xff,
        sketch.cellp,
        sketch.bits >> 16, (sketch.bits >> 8) & 0xff, sketch.bits & 0xff,
      ]);
    }
    
    for (const unknown of this.unknown) this._appendDst(unknown);
    this._appendDst(this.tail);
    
    this.src = new Uint8Array(this.dstc);
    this.src.set(new Uint8Array(this.dst.buffer, this.dst.byteOffset, this.dstc), 0);
    delete this.dstc;
    delete this.dsta;
    delete this.dst;
  }
  
  _appendDst(src) {
    if (!src.length) return;
    let na = this.dstc + src.length;
    if (na > this.dsta) {
      this.dsta = (na + 1024) & ~1023;
      const nv = new Uint8Array(this.dsta);
      nv.set(this.dst);
      this.dst = nv;
    }
    this.dst.set(src, this.dstc, src.length);
    this.dstc += src.length;
  }
  
  /* Read from (src).
   * Replaces: spellid, game_time_ms, damage_count, transmogrification, selected_item, itemv, itemqv, gs, plants, sketches
   */
  decode() {
    this.clearContent();
    for (let srcp=0; srcp<this.src.length; ) {
      const srcp0 = srcp;
      const command = this.src[srcp++];
      if (command === 0x00) {
        this.tail = new Uint8Array(this.src.buffer, this.src.byteOffset + srcp0, this.src.length - srcp0);
        return;
      }
      const chunklen = this.src[srcp++];
      if (srcp > this.src.length - chunklen) {
        throw new Error(`${this.path}:${srcp0}/${this.src.length}: Illegal chunk length ${chunklen}`);
      }
      const chunk = new Uint8Array(this.src.buffer, this.src.byteOffset + srcp, chunklen);
      srcp += chunklen;
      switch (command) {
        case 1: this._decode_FIXED01(chunk, srcp0); break;
        case 2: this._decode_GSBIT(chunk, srcp0); break;
        case 3: this._decode_PLANT(chunk, srcp0); break;
        case 4: this._decode_SKETCH(chunk, srcp0); break;
        default: this._decode_unknown(command, chunk, srcp0); break;
      }
    }
  }
  
  _decode_FIXED01(chunk, srcp) {
    if (chunk.length !== 27) throw new Error(`${this.path}:${srcp}/${this.src.length}: FIXED01 length must be 27, found ${chunk.length}`);
    this.spellid = chunk[0];
    this.game_time_ms = (chunk[1] << 24) | (chunk[2] << 16) | (chunk[3] << 8) | chunk[4];
    this.damage_count = (chunk[5] << 8) | chunk[6];
    this.transmogrification = chunk[7];
    this.selected_item = chunk[8];
    this.itemv = (chunk[9] << 8) | chunk[10];
    this.itemqv = chunk.slice(11, 27);
  }
  
  _decode_GSBIT(chunk, srcp) {
    if (chunk.length < 2) throw new Error(`${this.path}:${srcp}/${this.src.length}: GSBIT minimum length 2, found ${chunk.length}`);
    const p = (chunk[0] << 8) | chunk[1];
    const addc = chunk.length - 2;
    if (addc < 1) return;
    const nc = p + addc;
    if (nc > this.gs.length) {
      const nv = new Uint8Array(nc);
      nv.set(this.gs);
      this.gs = nv;
    }
    this.gs.set(chunk.slice(2), p);
  }
  
  _decode_PLANT(chunk, srcp) {
    if (chunk.length !== 9) throw new Error(`${this.path}:${srcp}/${this.src.length}: PLANT length must be 9, found ${chunk.length}`);
    const mapid = (chunk[0] << 8) | chunk[1];
    const cellp = chunk[2];
    const state = chunk[3];
    const fruit = chunk[4];
    const flower_time = (chunk[5] << 24) | (chunk[6] << 16) | (chunk[7] << 8) | chunk[8];
    this.plants.push({ mapid, cellp, state, fruit, flower_time });
  }
  
  _decode_SKETCH(chunk, srcp) {
    if (chunk.length !== 6) throw new Error(`${this.path}:${srcp}/${this.src.length}: SKETCH length must be 6, found ${chunk.length}`);
    const mapid = (chunk[0] << 8) | chunk[1];
    const cellp = chunk[2];
    const bits = (chunk[3] << 16) | (chunk[4] << 8) | chunk[5];
    this.sketches.push({ mapid, cellp, bits });
  }
  
  _decode_unknown(command, chunk, srcp) {
    const bin = new Uint8Array(2 + chunk.length);
    bin[0] = command;
    bin[1] = chunk.length;
    bin.set(chunk, 2);
    this.unknown.push(bin);
  }
}
