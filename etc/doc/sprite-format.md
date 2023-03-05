# Full Moon Sprite Data

One input text file per sprite.

## Text Format

Line-oriented "KEY VALUE" text.
'#' begins a comment anywhere in the line.
Commands that take strings can also be given the scalar integer value.

You can also express a command as a bunch of integers 0..255.
We validate the payload length generically (see binary format below).

```
image ID
tile ID
xform [xrev] [yrev] [swap]
style hidden|tile|hero|fourframe|firenozzle|firewall|doublewide
physics [motion] [edge] [sprites] [solid] [hole]
decay (FLOAT 0..255)
radius (FLOAT 0..255)
invmass (0..255)
controller (0..65535|name)
layer (0..255)
bv[N] (0..255) # N in 0..7
```

## Binary Format

The sprite resource is a stream of key-value pairs, where the leading byte indicates field ID and data size.
You can know the payload length from the leading byte:

```
0x00..0x1f = 0 
0x20..0x3f = 1
0x40..0x5f = 2
0x60..0x7f = 3
0x80..0x9f = 4
0xa0..0xcf = Next byte is remaining length.
0xd0..0xff = Reserved, decoder must fail if unknown.
```

Defined fields:

```
0x00 EOF. Any remaining content should be ignored.

0x20 Image ID.
0x21 Tile ID.
0x22 Xform.
0x23 Style. Default 1 (FMN_SPRITE_STYLE_TILE)
0x24 Physics. Bitfields, see below.
0x25 Inverse mass. 0=infinite, 1=heaviest, 255=lightest
0x26 Layer.

0x40 Velocity decay. u8.8 linear decay in m/s**2
0x41 Radius. u8.8 m
0x42 Controller. See FMN_SPRCTL_* in src/app/sprite/fmn_sprite.h
0x43 bv, controller-specific parameters. (u8 k,u8 v)
```

Physics:

```
0x01 MOTION
0x02 EDGE
0x04 SPRITES
0x10 SOLID
0x20 HOLE
```
