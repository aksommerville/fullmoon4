class Encoder {
  constructor() {
    this.v = Buffer.alloc(256);
    this.c = 0;
  }
  
  finish() {
    const dst = Buffer.alloc(this.c);
    this.v.copy(dst);
    return dst;
  }
  
  require(addc) {
    if (addc < 1) return;
    if (this.c + addc <= this.v.length) return;
    const na = (this.c + addc + 256) & ~255;
    const nv = Buffer.alloc(na);
    this.v.set(nv, 0, this.c);
    this.v = nv;
  }
  
  u8(v) {
    this.require(1);
    this.v[this.c++] = v;
  }
  
  u16be(v) {
    this.require(2);
    this.v[this.c++] = v >> 8;
    this.v[this.c++] = v;
  }
  
  u24be(v) {
    this.require(3);
    this.v[this.c++] = v >> 16;
    this.v[this.c++] = v >> 8;
    this.v[this.c++] = v;
  }
  
  u32be(v) {
    this.require(4);
    this.v[this.c++] = v >> 24;
    this.v[this.c++] = v >> 16;
    this.v[this.c++] = v >> 8;
    this.v[this.c++] = v;
  }
  
  vlq(v) {
    if (v < 0) {
      throw new Error(`Not encodable as VLQ: ${v}`);
    } else if (v < 0x80) {
      this.u8(v);
    } else if (v < 0x4000) {
      this.u16be(0x8000 | ((v & 0x3f80) << 1) | (v & 0x7f));
    } else if (v < 0x200000) {
      this.u24be(0x808000 | ((v & 0x1fc000) << 2) | ((v & 0x3f80) << 1) | (v & 0x7f));
    } else if (v < 0x10000000) {
      this.u32be(0x80808000 | ((v & 0xfe00000) << 3) | ((v & 0x1fc000) << 2) | ((v & 0x3f80) << 1) | (v & 0x7f));
    } else {
      throw new Error(`Not encodable as VLQ: ${v}`);
    }
  }
}

module.exports = Encoder;
