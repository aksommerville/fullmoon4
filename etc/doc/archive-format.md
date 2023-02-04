# Full Moon Archive Format

Data gets packed into a single binary file.

Integers are big-endian unless specified.

File begins with a header:
```
0000   4 Signature: "\xffAR\x00"
0004   4 TOC Entry Count (both State Change and Resource)
0008   4 Additional Header Length
000c ... Additional Header
```

Followed immediately by the TOC.
Each TOC entry is a 32-bit integer.

```
0x80000000 State Change
0x7f000000 Reserved
0x00ff0000 Type
0x0000ffff Qualifier
```

or

```
0xff000000 Zero
0x00ffffff Offset
```

All Offset must be sequential.
Each Offset can be used to determine the length of the preceding entry.
(The final entry runs to EOF).

| Type | Name       | Qualifier |
|------|------------|-----------|
| 0x00 | Illegal    | |
| 0x01 | Image      | Size and colorspace (TODO) |
| 0x02 | Song       | |
| 0x03 | Map        | |
| 0x04 | Tileprops  | |
| 0x05 | Sprite     | |
| 0x06 | String     | Language |
| 0x07 | Instrument | Synthesizer, see instrument-format.md |

Within a (type, qualifier) pair, resources are in order starting from ID 1.
TOC may contain zero-length resources if the set is discontiguous.

## Resource Formats

Images are PNG.

Songs are MIDI.

Everything else is peculiar to Full Moon, see docs in this repo.