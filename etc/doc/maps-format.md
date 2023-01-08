# Full Moon Maps Format

There's a text format for files under `src/data/map/`, intended to be both man- and machine-intelligible.

Then there's a packed binary format as used in `out/maps.data`, for production use.

Our script `src/tool/mkmaps/main.js` compiles the binary from multiple text files.

## Text Format

File's basename must be a decimal integer in 1..65535; that becomes its Map ID.

Starts with 12 lines of 40 characters each; every 2 characters are one map cell.
Either a hexadecimal integer, or one of these aliases:

- `. ` 0x00
- `Xx` 0x01

TODO Should have many more aliases. Maybe customizable without changing the compiler?

TODO Should do automatic neighbor-joining too, describe that.

TODO Further data eg neighbor maps and spawn points.

## Binary Format

Integers are big-endian.

```
0000   4 Signature: "\xff\x00MP"
0004   2 Last Map ID (ie count of entries; they start from 1).
0006 ... Offset Table.
.... ... Data.
```

Offset Table is a u32 for each map (from 1 through Last Map ID).
Offset is a position in this file where that maps begins.
Offsets include the header and Offset Table, and decoders should validate against that.
Must be sorted by ID. So you can examine the next offset to know a map's length.
If a map is absent, it must still have a valid entry here, and its length can be inferred as zero.
The last map runs to EOF.
