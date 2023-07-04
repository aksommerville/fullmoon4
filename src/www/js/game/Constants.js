/* Constants.js
 * Mostly these mimic defines from fmn_platform.h.
 */

export const COLC = 20;
export const ROWC = 12;
    
export const TILESIZE = 16;
    
export const XFORM_XREV = 1;
export const XFORM_YREV = 2;
export const XFORM_SWAP = 4;
    
export const DIR_NW   = 0x80;
export const DIR_N    = 0x40;
export const DIR_NE   = 0x20;
export const DIR_W    = 0x10;
export const DIR_MID  = 0x00;
export const DIR_E    = 0x08;
export const DIR_SW   = 0x04;
export const DIR_S    = 0x02;
export const DIR_SE   = 0x01;
    
export const PLANT_LIMIT = 16;
export const PLANT_SIZE = 8;
export const SKETCH_LIMIT = 16;
export const SKETCH_SIZE = 12;
export const DOOR_LIMIT = 16;
export const DOOR_SIZE = 8;
export const SPRITE_STORAGE_SIZE = 64;
    
export const ITEM_SNOWGLOBE = 0;
export const ITEM_HAT = 1;
export const ITEM_PITCHER = 2;
export const ITEM_SEED = 3;
export const ITEM_COIN = 4;
export const ITEM_MATCH = 5;
export const ITEM_QUALIFIER_COUNT = 6;
export const ITEM_BROOM = 6;
export const ITEM_WAND = 7;
export const ITEM_UMBRELLA = 8;
export const ITEM_FEATHER = 9;
export const ITEM_SHOVEL = 10;
export const ITEM_COMPASS = 11;
export const ITEM_VIOLIN = 12;
export const ITEM_CHALK = 13;
export const ITEM_BELL = 14;
export const ITEM_CHEESE = 15;
export const ITEM_COUNT = 16;
    
export const PLANT_STATE_NONE = 0;
export const PLANT_STATE_SEED = 1;
export const PLANT_STATE_GROW = 2;
export const PLANT_STATE_FLOWER = 3;
export const PLANT_STATE_DEAD = 4;
    
export const PLANT_FRUIT_NONE = 0;
export const PLANT_FRUIT_SEED = 1;
export const PLANT_FRUIT_CHEESE = 2;
export const PLANT_FRUIT_COIN = 3;
export const PLANT_FRUIT_MATCH = 4;
    
export const PLANT_FLOWER_TIME = 20;
    
export const TRANSITION_TIME_MS = 600;
    
export const TRANSITION_CUT = 0;
export const TRANSITION_PAN_LEFT = 1;
export const TRANSITION_PAN_RIGHT = 2;
export const TRANSITION_PAN_UP = 3;
export const TRANSITION_PAN_DOWN = 4;
export const TRANSITION_FADE_BLACK = 5;
export const TRANSITION_DOOR = 6;
export const TRANSITION_WARP = 7;
    
export const SPRITE_STYLE_HIDDEN = 1;
export const SPRITE_STYLE_TILE = 2;
export const SPRITE_STYLE_HERO = 3;
export const SPRITE_STYLE_FOURFRAME = 4;
export const SPRITE_STYLE_FIRENOZZLE = 5;
export const SPRITE_STYLE_FIREWALL = 6;
export const SPRITE_STYLE_DOUBLEWIDE = 7;
export const SPRITE_STYLE_PITCHFORK = 8;
export const SPRITE_STYLE_TWOFRAME = 9;
export const SPRITE_STYLE_EIGHTFRAME = 10;
export const SPRITE_STYLE_SCARYDOOR = 11;
export const SPRITE_STYLE_WEREWOLF = 12;
export const SPRITE_STYLE_FLOORFIRE = 13;
export const SPRITE_STYLE_DEADWITCH = 14;
    
export const SPRITE_BV_SIZE = 8;
export const SPRITE_SV_SIZE = 4;
export const SPRITE_FV_SIZE = 8;
export const SPRITE_PV_SIZE = 1;
    
export const GS_SIZE = 64;
    
export const VIOLIN_SONG_LENGTH = 20;
    
export const AUDIO_FRAME_RATE = 44100;
export const AUDIO_CHANNEL_COUNT = 16; // logical bus channels, not mono/stereo output
    
export const VIDEO_PIXFMT_ANY        = 0x00;
export const VIDEO_PIXFMT_ANY_1      = 0x10;
export const VIDEO_PIXFMT_Y1BE       = 0x11;
export const VIDEO_PIXFMT_W1BE       = 0x12;
export const VIDEO_PIXFMT_ANY_2      = 0x20;
export const VIDEO_PIXFMT_ANY_4      = 0x30;
export const VIDEO_PIXFMT_ANY_8      = 0x40;
export const VIDEO_PIXFMT_I8         = 0x41;
export const VIDEO_PIXFMT_Y8         = 0x42;
export const VIDEO_PIXFMT_ANY_16     = 0x50;
export const VIDEO_PIXFMT_RGB565LE   = 0x51;
export const VIDEO_PIXFMT_RGBA4444BE = 0x52;
export const VIDEO_PIXFMT_ANY_24     = 0x60;
export const VIDEO_PIXFMT_ANY_32     = 0x70;
export const VIDEO_PIXFMT_RGBA       = 0x71;
export const VIDEO_PIXFMT_BGRA       = 0x72;
export const VIDEO_PIXFMT_ARGB       = 0x73;
export const VIDEO_PIXFMT_ABGR       = 0x74;
