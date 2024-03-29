# Full Moon Maps Format

There's a text format for files under `src/data/map/`, intended to be both man- and machine-intelligible.
One file per map.

## Text Format

File's basename must be a decimal integer in 1..65535; that becomes its Map ID.
May also contain a name. "ID-NAME"

Starts with 12 lines of 40 characters each; every 2 characters are one map cell, hexadecimal.

After the map picture, each non-empty line is a command.
Order of commands is preserved.

Adding new commands, you must add a clause at src/tool/mkdata/processMap.js.
And probably want to add support in the editor too.

```
song SONGID
tilesheet IMAGEID
neighborw MAPID
neighbore MAPID
neighborn MAPID
neighbors MAPID
door X Y MAPID DSTX DSTY
sprite X Y SPRITEID ARG0 ARG1 ARG2
hero X Y SPELLID
saveto SPELLID # refers to some other map with a 'hero' command
transmogrify X Y MODE STATE # STATE=[1:pumpkin], MODE=[to,from,toggle]
wind N|E|S|W # direction it blows *to*, opposite the meterological convention
sketch X Y BITS # Creates sketch if there are none initially.
buried_treasure X Y GSBIT ITEMID
buried_door X Y GSBIT MAPID DSTX DSTY
callback EVID CBID PARAM
event_trigger X Y EVENTID # for fmn_game.h:fmn_game_event_listen. Not the same events as "callback".
facedir GSBIT_HORZ GSBIT_VERT # Set a gsbit when the hero turns.
flags [dark] [indoors] [blowback] [ancillary] [multihome] [nodoors]
ref 0..255 # arbitrary identifier, for crow guidance
```

Resource IDs may be name or number.

Required order of commands. (asserted only at `make verify`). All unlisted commands may appear anywhere.

- tilesheet,flags,hero,saveto
- neighborw,neighbore,neighborn,neighbors
- sketch
- door,sprite,buried_door

Flags

- dark: Start with lights out, matches required.
- indoors: Weather spells won't work.
- blowback: Edges without a neighbor, we will produce wind as you approach to signal "end of world".
- ancillary: Doesn't particpate in crow paths.
- multihome: (for `make verify` only) Indicate that the map is allowed to have multiple songs or save points.

## Binary Format

Each map starts with 20*12 cells, one byte each. (or FMN_MAP_COLC,ROWC in src/app/fmn_platform.h, if it changes).

After the cells is a packed list of commands, identifiable by their leading byte.
Future decoders are allowed to skip unknown commands if the length is known.

```
0x00..0x1f No payload
0x20..0x3f 1-byte payload
0x40..0x5f 2-byte payload
0x60..0x7f 4-byte payload
0x80..0x9f 6-byte payload
0xa0..0xbf 8-byte payload
0xc0..0xdf First byte of payload is subsequent length.
0xe0..0xff Known length. Decoders must fail if command unknown.
```

```
0x00 () EOF: Stop processing commands, the remainder is garbage. Optional, maps are sized externally.

0x20 (u8 songid) SONG
0x21 (u8 imageid) TILESHEET
0x22 (u8 spellid) SAVETO
0x23 (u8 dir) WIND
0x24 (u8 flags) FLAGS: 0x01=dark 0x02=indoors 0x04=blowback 0x08=ancillary 0x10=multihome 0x20=nodoors 0xe0=reserved
0x25 (u8 ref) REF

0x40 (u16 mapid) NEIGHBORW
0x41 (u16 mapid) NEIGHBORE
0x42 (u16 mapid) NEIGHBORN
0x43 (u16 mapid) NEIGHBORS
0x44 (u8 cellp,u8 0x80:to 0x40:from 0x3f:state) TRANSMOGRIFY
0x45 (u8 cellp,u8 spellid) HERO

0x60 (u8 cellp,u16 mapid,u8 dstcellp) DOOR
0x61 (u8 cellp,u24 bits) SKETCH
0x62 (u8 cellp,u16 gsbit,u8 itemid) BURIED_TREASURE
0x63 (u8 evid,u16 cbid,u8 param) CALLBACK
0x64 (u8 cellp,u16 eventid,u8 unused) EVENT_TRIGGER
0x65 (u16 gsbit_horz,u16 gsbit_vert) FACEDIR

0x80 (u8 cellp,u16 spriteid,u8 arg0,u8 arg1,u8 arg2) SPRITE
0x81 (u8 cellp,u16 gsbit,u16 mapid,u8 dstp) BURIED_DOOR
```
