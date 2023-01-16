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
    this.DOOR_LIMIT = 16;
    this.DOOR_SIZE = 8;
    
    this.TRANSITION_TIME_MS = 800;
    
    this.TRANSITION_CUT = 0;
    this.TRANSITION_PAN_LEFT = 1;
    this.TRANSITION_PAN_RIGHT = 2;
    this.TRANSITION_PAN_UP = 3;
    this.TRANSITION_PAN_DOWN = 4;
    this.TRANSITION_FADE_BLACK = 5;
    this.TRANSITION_DOOR = 6;
    this.TRANSITION_WARP = 7;
  }
}

Constants.singleton = true;
