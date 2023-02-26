# Full Moon Tileprops Format

Tileprops are resources parallel to image.
If a tileprops is present, the image is a grid of 16x16 tiles.

Each tile has 4 properties: physics, family, neighbors, weight.

#### physics

8-bit enum describing how the tile should behave when used in a map.
`FMN_CELLPHYSICS_`

 - 0: vacant
 - 1: solid
 - 2: hole
 - 3: vacant, unshovellable
 - 4: solid, unchalkable
 
#### family

If zero, the tile has no relation to other tiles in this image.

Otherwise, all tiles within an image of the same family are candidates to replace each other.

#### neighbors

8 bits indicating that a neighbor of the same family is expected in adjacent positions.
Order reads LRTB big-endianly, skipping the center.

 - 0x80 NW
 - 0x40 N
 - 0x20 NE
 - 0x10 W
 - 0x08 E
 - 0x04 SW
 - 0x02 S
 - 0x01 SE
 
eg an island tile is 0x00, the middle of a mass is 0xff, a crossroads is 0x5a.

#### weight

If the editor has multiple valid candidates for a tile, it will select randomly based on the candidates' weights.

0 means "by appointment only"; the tile will never be selected randomly.
Helpful for doors, important decorations, and such, where a tile has to join a mass of like tiles,
but should only be there if you say so.

1..255 are relative odds, higher is likelier.

A new weights table should initialize to straight 255, not zero.

### Binary Format

256 bytes per table. For now at least, only the Physics table is preserved in binary format.
 
### Text Format

Same idea as binary, but the tables are named and content is written in rows of 32 hexadecimal digits.

All whitespace except newline is ignored.
Lines beginning '#' after whitespace removal are comments. Full lines only.

Table name must come first, on its own line, then 16 rows of content.

eg
```
physics
0011abcd0011abcd0011abcd0011abcd
0011abcd0011abcd0011abcd0011abcd
...14 more content rows...
```
