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
    
    this.TRANSITION_TIME_MS = 600;
    
    this.TRANSITION_CUT = 0;
    this.TRANSITION_PAN_LEFT = 1;
    this.TRANSITION_PAN_RIGHT = 2;
    this.TRANSITION_PAN_UP = 3;
    this.TRANSITION_PAN_DOWN = 4;
    this.TRANSITION_FADE_BLACK = 5;
    this.TRANSITION_DOOR = 6;
    this.TRANSITION_WARP = 7;
    
    this.SFX_BELL = 1;
    this.SFX_REJECT_ITEM = 2;
    this.SFX_CHEESE = 3;
    this.SFX_HURT = 4;
    this.SFX_PITCHER_NO_PICKUP = 5;
    this.SFX_PITCHER_PICKUP = 6;
    this.SFX_PITCHER_POUR = 7;
    this.SFX_MATCH = 8;
    this.SFX_DIG = 9;
    this.SFX_REJECT_DIG = 10;
    this.SFX_INJURY_DEFLECTED = 11;
    this.SFX_KICK_1 = 35;
    this.SFX_KICK_2 = 36;
    this.SFX_SIDE_STICK = 37;
    this.SFX_SNARE_1 = 38;
    this.SFX_HAND_CLAP = 39;
    this.SFX_SNARE_2 = 40;
    this.SFX_TOM_1 = 41;
    this.SFX_CLOSED_HAT = 42;
    this.SFX_TOM_2 = 43;
    this.SFX_PEDAL_HAT = 44;
    this.SFX_TOM_3 = 45;
    this.SFX_OPEN_HAT = 46;
    this.SFX_TOM_4 = 47;
    this.SFX_TOM_5 = 48;
    this.SFX_CRASH_1 = 49;
    this.SFX_TOM_6 = 50;
    this.SFX_RIDE_1 = 51;
    this.SFX_CRASH_2 = 52;
    this.SFX_RIDE_2 = 53;
    this.SFX_TAMBOURINE = 54;
    this.SFX_SPLASH_1 = 55;
    this.SFX_COWBELL = 56;
    
    this.SPRITE_STYLE_HIDDEN = 0;
    this.SPRITE_STYLE_TILE = 1;
    this.SPRITE_STYLE_HERO = 2;
    this.SPRITE_STYLE_FOURFRAME = 3;
    
    this.AUDIO_FRAME_RATE = 44100;
    this.AUDIO_CHANNEL_COUNT = 16; // logical bus channels, not mono/stereo output
    
    this.MENU_PAUSE = -1;
    this.MENU_CHALK = -2;
    this.MENU_TREASURE = -3;
  }
}

Constants.singleton = true;
