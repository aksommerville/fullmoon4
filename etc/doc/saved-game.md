# Full Moon Saved Games

Saved games should be compatible across installations regardless of platform.
Most platforms, it's a private-format binary file.
Web, it's the same format but base64-encoded for safekeeping in localStorage or whatever.

One file is one campaign of play.
I don't plan to have any "pick your file" feature, but if we do, those should be different files on disk.

Most of the content needed for a saved game is in fmn_global, and fixed size.

Exceptions:
- Plants (fmstore for native)
- Sketches (fmstore for native)
- fmn_game_global.c:game_time_ms (why isn't this in fmn_global?)
- fmn_game_global.c:fmn_free_bird_count. I think it's OK to drop this when saving.

We don't record mapid, instead record spellid -- This must be one of the teleport spells.
Any map with a "hero" command with nonzero spellid is eligible as a save point.
When loading, if we can't find the named map, substitute HOME.

Be careful to ensure that if you save as a pumpkin, you can only restart in a pumpkin-safe zone.
And use the pumpkin-proof toll troll to enforce that zone; he's smart enough to let pumpkins back in when it fails.

Do not save if fmn_global.hero_dead or fmn_global.werewolf_dead.
...on second thought, do save in these cases. But don't record those facts.

Platforms: Do not save between fmn_reset() and the next fmn_load_map(). eg choosing End Game from the Pause menu should not erase the save file.

All content to persist:
- Plants: u16 mapid,u8 cellp,u8 state,u8 fruit,u32 flower_time
- Sketches: u16 mapid,u8 cellp,u24 bits ; no need to record time
- u8[FMN_GS_SIZE] gs. Size is currently 64, but encode as if it's variable.
- u32 game_time_ms
- u8 selected_item. Don't record active_item, it clears to zero.
- u16 itemv. Little-endian bits.
- u8[16] itemqv. Only a few items use this, but store all 16 to keep it simple.
- u8 transmogrification
- u16 damage_count
- u8 position, spell id.

Encoded format is chunks with a 2-byte header:
```
u8 chunkid
u8 length
```

The first chunk must be (0x01,0x1b) ie (FIXED01,27), which can also be used as a signature.

```
Chunk 0x00 EOF
  Ignore payload and anything after.
  
Chunk 0x01 FIXED01
  Must be the first chunk in every file.
  Straight zeroes is kosher, equivalent to a new game.
  If we add more fixed-length content in the future, it should go in a new chunk.
  Fixed length 27 bytes:
    u8 spellid. Must match a "hero" command in some map. (otherwise start at map 1 like a new game)
    u32 game_time_ms
    u16 damage_count
    u8 transmogrification
    u8 selected_item
    u16 itemv
    u8[16] itemqv
    
Chunk 0x02 GSBIT
  Initialize fmn_global.gs to zero as in a new game.
  GSBIT chunks do not necessarily cover the whole thing.
  If two GSBIT chunks cover the same range, the later one takes precedence.
  Decoder must check range and ignore excess.
  (Current implementation has a 64-byte gs, and I plan to encode the whole thing verbatim in one chunk).
  Any length.
    u16 startp, byte offset in fmn_global.gs
    ... content
  
Chunk 0x03 PLANT
  Fixed length 9 bytes:
    u16 mapid
    u8 cellp
    u8 state
    u8 fruit
    u32 flower_time
    
Chunk 0x04 SKETCH
  Fixed length 6 bytes:
    u16 mapid
    u8 cellp
    u24 bits
```
