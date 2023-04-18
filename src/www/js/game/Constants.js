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
    this.PLANT_SIZE = 8;
    this.SKETCH_LIMIT = 16;
    this.SKETCH_SIZE = 12;
    this.DOOR_LIMIT = 16;
    this.DOOR_SIZE = 8;
    this.SPRITE_STORAGE_SIZE = 64;
    
    this.ITEM_NONE = 0;
    this.ITEM_CORN = 1;
    this.ITEM_PITCHER = 2;
    this.ITEM_SEED = 3;
    this.ITEM_COIN = 4;
    this.ITEM_MATCH = 5;
    this.ITEM_QUALIFIER_COUNT = 6;
    this.ITEM_BROOM = 6;
    this.ITEM_WAND = 7;
    this.ITEM_UMBRELLA = 8;
    this.ITEM_FEATHER = 9;
    this.ITEM_SHOVEL = 10;
    this.ITEM_COMPASS = 11;
    this.ITEM_VIOLIN = 12;
    this.ITEM_CHALK = 13;
    this.ITEM_BELL = 14;
    this.ITEM_CHEESE = 15;
    this.ITEM_COUNT = 16;
    
    this.PLANT_STATE_NONE = 0;
    this.PLANT_STATE_SEED = 1;
    this.PLANT_STATE_GROW = 2;
    this.PLANT_STATE_FLOWER = 3;
    this.PLANT_STATE_DEAD = 4;
    
    this.PLANT_FRUIT_NONE = 0;
    this.PLANT_FRUIT_SEED = 1;
    this.PLANT_FRUIT_CHEESE = 2;
    this.PLANT_FRUIT_COIN = 3;
    this.PLANT_FRUIT_MATCH = 4;
    
    this.PLANT_FLOWER_TIME = 20;
    
    this.TRANSITION_TIME_MS = 600;
    
    this.TRANSITION_CUT = 0;
    this.TRANSITION_PAN_LEFT = 1;
    this.TRANSITION_PAN_RIGHT = 2;
    this.TRANSITION_PAN_UP = 3;
    this.TRANSITION_PAN_DOWN = 4;
    this.TRANSITION_FADE_BLACK = 5;
    this.TRANSITION_DOOR = 6;
    this.TRANSITION_WARP = 7;
    
    this.SPRITE_STYLE_HIDDEN = 1;
    this.SPRITE_STYLE_TILE = 2;
    this.SPRITE_STYLE_HERO = 3;
    this.SPRITE_STYLE_FOURFRAME = 4;
    this.SPRITE_STYLE_FIRENOZZLE = 5;
    this.SPRITE_STYLE_FIREWALL = 6;
    this.SPRITE_STYLE_DOUBLEWIDE = 7;
    this.SPRITE_STYLE_PITCHFORK = 8;
    this.SPRITE_STYLE_TWOFRAME = 9;
    this.SPRITE_STYLE_EIGHTFRAME = 10;
    this.SPRITE_STYLE_SCARYDOOR = 11;
    this.SPRITE_STYLE_WEREWOLF = 12;
    this.SPRITE_STYLE_FLOORFIRE = 13;
    this.SPRITE_STYLE_DEADWITCH = 14;
    
    this.SPRITE_BV_SIZE = 8;
    this.SPRITE_SV_SIZE = 4;
    this.SPRITE_FV_SIZE = 8;
    this.SPRITE_PV_SIZE = 1;
    
    this.GS_SIZE = 64;
    
    this.VIOLIN_SONG_LENGTH = 20;
    
    this.AUDIO_FRAME_RATE = 44100;
    this.AUDIO_CHANNEL_COUNT = 16; // logical bus channels, not mono/stereo output
    
    this.MENU_PAUSE = -1;
    this.MENU_CHALK = -2;
    this.MENU_TREASURE = -3;
    this.MENU_VICTORY = -4;
    this.MENU_GAMEOVER = -5;
    this.MENU_HELLO = -6;
  }
}

Constants.singleton = true;
