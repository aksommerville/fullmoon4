/* Constants.js
 * You might expect it to be static, but I'm making it an injectable singleton in case some "constants" prove to be dynamic.
 */
 
export class Constants {
  static getDependencies() {
    return [];
  }
  constructor() {
  
    this.COLC = 20;
    this.ROWC = 12;
    
    // Arguably could live in Renderer instead, but i dunno...
    this.TILESIZE = 16;
    
    this.XFORM_XREV = 1;
    this.XFORM_YREV = 2;
    this.XFORM_SWAP = 4;
    
    this.DIR_NW   = 0x80;
    this.DIR_N    = 0x40;
    this.DIR_NE   = 0x20;
    this.DIR_W    = 0x10;
    this.DIR_MID  = 0x00;
    this.DIR_E    = 0x08;
    this.DIR_SW   = 0x04;
    this.DIR_S    = 0x02;
    this.DIR_SE   = 0x01;
    
    this.PLANT_LIMIT = 16;
    this.PLANT_SIZE = 12;
    this.SKETCH_LIMIT = 16;
    this.SKETCH_SIZE = 12;
  }
}

Constants.singleton = true;
