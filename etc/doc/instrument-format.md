# Full Moon Instrument Format

We expose the full MIDI Bank and Program space, about 2 million available program IDs.
*BUT BEWARE* when we pack archives, we insert dummies for any missing ID.
So if you add an instrument in bank 50, you're making 50 * 128 empty instruments before it.

Note that zero is a valid PID to MIDI, but we do not permit resources with ID zero.
Synthesizers are encouraged to substitute PID one for zero.
Or use a hard-coded default for zero. I'm not sure. Whatever.

I'm starting off with a WebAudio-based synthesizer.
Future platforms will almost certainly use something completely different.
So instruments are qualified by their synthesizer ID:

| Qualifier | Synthesizer |
|-----------|-------------|
|         1 | WebAudio    |

Each input file contains all of the instruments for one synthesizer.

Songs are not qualified by synthesizer, and I want to try to keep it that way.
So we need to be careful to keep each instrument sounding similar across the various synthesizers.

## Input Format For All Synthesizers

Plain line-oriented text.
`#` anywhere begins a line comment.

Each instrument must have a start and end line:
```
begin PID
  ...
end
```

`PID` can be the fully-qualified program id (0..0x1fffff), or `[BankMSB] BankLSB PID`.

Inside the instrument block, lines are "KEY [VALUE...]". Each KEY can only appear once.

## WebAudio Input Format

| Key             | Description |
|-----------------|-------------|
| wave            | "sine", "square", "sawtooth", "triangle", or a list of 16-bit coefficients, hexadecimal |
| env             | Velocity-Sensitive Envelope |
| modAbsoluteRate | Hz, modulator rate for a detune effect. Overrides `modRate` |
| modRate         | Multiplier against note frequency. Modulator rate. Usually 1, or a small multiple or fraction of 1 |
| modRange        | Modulator range limit. Usually in like 0..5 |
| modEnv          | Velocity-Sensitive Envelope, for modulation range. |
| modRangeLfoRate | Hz, extra LFO multiplied against modulator range. |
| wheelRange      | Cents |

## WebAudio Output Format

Packed stream of bytes.
Leading byte describes a command.

0x00 EOF ()
0x01 wave (u8 coefc,u16[] coefv) or (u8 0x80|shape) 0x80=sine 0x81=square 0x82=sawtooth 0x83=triangle
0x02 env (see below)
0x03 modAbsoluteRate (u16.8 hz)
0x04 modRate (u8.8 rate)
0x05 modRange (u8.8 range)
0x06 modEnv (see below)
0x07 modRangeLfoRate (u16.8 hz)
0x08 weelRange (u16 cents)

Envelopes:
  u8 flags
      80 Velocity
      40 HighResTimes: T is u16 if set, otherwise u8
      20 HighResLevels: V is u16 if set, otherwise u8
      10 EdgeLevels
      0f reserved, zero
  [V] initlo if EdgeLevels
  T atktlo
  V atkvlo
  T dectlo
  V susvlo
  T rlstlo
  [V] endvlo if EdgeLevels
  if Velocity:
    [V] inithi if EdgeLevels
    T atkthi
    V atkvhi
    T decthi
    V susvhi
    T rlsthi
    [V] endvhi if EdgeLevels
    
Times are always in ms. (So "HighResTimes" increases the range, not the resolution).
Total size 6..29 bytes, knowable from the first byte.

## Velocity-Sensitive Envelope

```
[v] t v t v t [v]

OR

[vlo] tlo vlo tlo vlo tlo [vlo] .. [vhi] thi vhi thi vhi thi [vhi]

# [INITIAL] ATTACK ATTACK DECAY SUSTAIN RELEASE [FINAL]
```

All `v` must be two or four hexadecimal digits.
All `t` are in decimal milliseconds, no fraction.
If any initial or final value is given, they must all be given.
