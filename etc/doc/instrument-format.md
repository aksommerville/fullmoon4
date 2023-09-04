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
|         2 | minsyn      |
|         3 | stdsyn      |

Each input file contains all of the instruments for one synthesizer.

Songs are not qualified by synthesizer, and I want to try to keep it that way.
So we need to be careful to keep each instrument sounding similar across the various synthesizers.

Note that Instrument and Sound are sourced from the same files.

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

Sound effects are defined similarly:
```
sound ID
  ...
end
```

ID for a Sound must currently be in 1..127. Maybe extend that later.

## WebAudio Instrument Input Format

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
| bpq             | Bandpass width ("Q" to a WebAudio BiquadFilterNode in "bandpass" mode, i think it's 0..100) |
| bpq2            | Second-pass bandpass filter, makes it much cleaner. |
| bpBoost         | Extra gain when `bpq` in play |

There are 3 distinct modes. (Actually 2; Oscillator is an edge case of FM).

| Mode | Fields | Description |
|------|--------|-------------|
| Oscillator | wave | Your basic osc+env synthesizer. |
| FM | wave, _modAbsoluteRate or modRate_, modRange, modEnv, modRangeLfoRate | FM. Wide range of expression, tends to sound synthy. |
| Bandpass | _bpq_, bpq2, bpBoost | White noise through a bandpass filter for breathy ghost sounds. Think pan flute or shakuhachi. |

All modes use `env` and `wheelRange`.

## WebAudio Instrument Output Format

Packed stream of bytes.
Leading byte describes a command.

```
0x00 EOF ()
0x01 wave (u8 coefc,u16[] coefv) or (u8 0x80|shape) 0x80=sine 0x81=square 0x82=sawtooth 0x83=triangle
0x02 env (see below)
0x03 modAbsoluteRate (u16.8 hz)
0x04 modRate (u8.8 rate)
0x05 modRange (u8.8 range)
0x06 modEnv (see below)
0x07 modRangeLfoRate (u16.8 hz)
0x08 weelRange (u16 cents)
0x09 bpq (u8.8 q)
0x0a bpq2 (u8.8 q)
0x0b bpBoost (u16 mlt)

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
```
    
Times are always in ms. (So "HighResTimes" increases the range, not the resolution).
Total size 6..29 bytes, knowable from the first byte.

## WebAudio Sound Input Format

Commands are processed in order.
Final output goes in Buffer 0.

`len SECONDS`
Must be the first command.
Nothing after 4 seconds will be preserved.

`noise BUFFER`
Fill buffer with random noise.

`wave BUFFER RATE SHAPE`
Overwrite a buffer with a normalized wave.
RATE in Hz.
SHAPE is "sine", "square", "sawtooth", "triangle", or coefficients (same as for Instruments).

`wave BUFFER ( ENV ) SHAPE`
ENV is "RATE TIME RATE TIME ... RATE", RATE in Hz, TIME in ms.
Playback rate interpolates *linearly* between given rates.
(exponential interpolation would be better acoustically but it's complicated).
(TODO?)
The last TIME in any envelope may be "*" meaning "until the end of the sound".

`fm BUFFER RATE MODRATE ( RANGEENV ) SHAPE`

`fm BUFFER ( ENV ) MODRATE ( RANGEENV ) SHAPE`
Like `wave` but an FM synthesizer.
The modulator wave is always a sine, not sure if other shapes are useful.

`gain BUFFER MLT [CLIP [GATE]]`

`env BUFFER ( LEVEL TIME LEVEL TIME ... LEVEL )`

`mix DSTBUFFER SRCBUFFER`

`norm BUFFER [PEAK]`

`delay BUFFER DURATION DRY WET STORE FEEDBACK`

`bandpass BUFFER MIDFREQ RANGE`
MIDFREQ and RANGE in Hz.

WebAudio instruments and sounds use completely separate implementations.
(The sounds are actually processed in pure software; WebAudio is not involved).
So there will be discrepancies between features that you'd expect to work the same.

## WebAudio Sound Output Format

Starts with a fixed-size header:
```
0000   1 Duration u2.6 s
0001   1 Buffer count
0002
```

Followed by commands knowable by the first byte:
```
0x00 EOF ()
0x01 NOISE (u8 buf)
0x02 WAVE_FIXED_NAME (u8 buf,u16 rate,u8 name)
0x03 WAVE_FIXED_HARM (u8 buf,u16 rate,u8 coefc,u16... coefv)
0x04 WAVE_ENV_NAME (u8 buf,ENV,u8 name)
0x05 WAVE_ENV_HARM (u8 buf,ENV,u8 coefc,u16... coefv)
0x06 FM_FIXED_NAME (u8 buf,u16 rate,u8.8 modrate,ENV range,u8 name)
0x07 FM_FIXED_HARM (u8 buf,u16 rate,u8.8 modrate,ENV range,u8 coefc,u16... coefv)
0x08 FM_ENV_NAME (u8 buf,ENV rate,u8.8 modrate,ENV range,u8 name)
0x09 FM_ENV_HARM (u8 buf,ENV rate,u8.8 modrate,ENV range,u8 coefc,u16... coefv)
0x0a GAIN (u8 buf,u8.8 mlt,u0.8 clip, u0.8 gate)
0x0b ENV (u8 buf,ENV)
0x0c MIX (u8 dst,u8 src)
0x0d norm (u8 buf,u0.8 peak)
0x0e delay (u8 buf,u16 durationMs,u0.8 dry,u0.8 wet,u0.8 store,u0.8 feedback)
0x0f bandpass (u8 buf,u16 midfreq,u16 range)
```

ENV: u16 level, u8 count, then count * (u16 timems,u16 level)

Wave names: 0=sine, 1=square, 2=sawtooth, 3=triangle

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

## Minsyn Input Format

Instruments are defined by a wave and envelope.

Waves are a list of harmonic coefficients, integers in 0..255. Keyword `wave`.

Envelope is: `atktlo atkvlo dectlo susvlo rlstlo .. atkthi atkvhi decthi susvhi rlsthi`. Keyword `env`.

You can omit `..` and everything after. Beyond that, everything is required.
Times are in milliseconds and levels in 0..255.
Release time will encode in 8-millisecond units, so the limit is 4088, not 255.

New: "mixwave" and "mixenv", same payloads as "wave" and "env", but defining an alternate wave and continuous mixing between them over time.
Because the existing "env" has to start and end at zero, "mixenv" does too. Might need to change that.

Example instrument:

```
begin 0x01
  wave 0xa0 0 0x30 0 0x08
  env 15 0x80 20 0x20 250 .. 10 0xff 15 0x40 400
end
```

Sound format is similar to WebAudio but simpler.
Mixing is now available, via a bounce mechanism "new_channel".
And since we operate in a streaming fashion, post-normalization is not possible.

```
len SECONDS
fm ( RATE_ENV ) MODRATE ( RANGE_ENV ) SHAPE
wave ( RATE_ENV ) SHAPE
noise
env ( ENV )
mlt SCALAR
delay PERIOD_SECONDS DRY WET STORE FEEDBACK
bandpass MID_HZ RANGE_HZ
new_channel : All subsequent commands belong to a new voice, same length as the first.
```

`len` must be first, and must be followed by one of `fm`, `wave`, `noise`, which overwrite the signal.

ENV are integers: `LEVEL [MS LEVEL...]`, one time may be replaced by `*` to fill in the remainder.

SHAPE is one of ("sine", "square", "sawtooth", "triangle"), or a list of integer in 0..255.

## Minsyn Binary Format

Instrument.
The first byte has one bit, in order, for all the features requested.
Decoder must stop at the first unknown set bit, but is allowed to proceed with whatever it has then.
Everything is optional, you'll get a sensible default for anything missing.

```
u8 Features present:
     0x01 Harmonics
     0x02 Low envelope
     0x04 High envelope
     0x08 Mix harmonics
     0x10 Low mix envelope
     0x20 High mix envelope
     0xc0 reserved, must be zero
If (Harmonics):
  u8 Count
  u8... Coefficients
If (Low envelope):
  u8 Attack time, ms
  u8 Attack level
  u8 Decay time, ms
  u8 Sustain level
  u8 Release time, 8ms
If (High envelope):
  5 Same as Low envelope
If (Mix harmonics):
  u8 Count
  u8... Coefficients
If (Low mix envelope):
  5 Envelope
If (High mix envelope):
  5 Envelope
```

Sound.

```
u1.7 Length
... Commands:
  u8 Opcode
  ... Params

0x01 FM (...rate_env,u4.4 modrate,...range_env,...shape) # range_env values in thousandths
0x02 WAVE (...rate_env,...shape)
0x03 NOISE ()
0x04 ENV (...env)
0x05 MLT (u8.8)
0x06 DELAY (u0.8 period,u0.8 dry,u0.8 wet,u0.8 store,u0.8 feedback)
0x07 BANDPASS (u16 mid,u16 range)
0x08 NEW_CHANNEL ()

env: u16 level,u8 count,count*(u16 ms,u16 level)
shape: u8 name: (200,201,202,203,204)=(sine,square,sawtooth,triangle,noise)
   OR: u8 coefc(<200),u8*coefc coefv
```

## Stdsyn

Adding this synthesizer pretty late in development, so I'm cutting some corners.
Sound effects are exactly the same as minsyn.
All minsyn instrument formats are valid too.

Stdsyn instruments have the two high bits of their leading byte set. Otherwise it's minsyn.

Text:

| keyword  | args
|----------|------
| fm       | `RATE RANGE [ENV]`: RATE is absolute Hz, or `*` and multiplier. RANGE is a scalar or parenthesized envelope (level/1000 and time in ms)
| stdenv   | Envelope. The "std" is to distinguish from minsyn's "env" command.
| master   | `MULTIPLIER`
| prefix   | Begin prefix section, for 3-legged program. Runs before voices. Buffer 0 is cleared, others are undefined.
| voice    | '' voice section. Runs per voice. Should add to buffer 0.
| suffix   | '' suffix section. Post-process finished content in buffer 0.
| *in pipes only* |
| silence  | `BUFID`
| noise    | `BUFID`
| copy     | `DST SRC`
| penv     | `BUFID ENV`
| osc      | `BUFID RATE SHAPE`. RATE may be `*N` to multiply against the note's rate, in a voice. Or `qN` to multiply by tempo qnote.
| pfm      | `BUFID RATE MODRATE RANGE [ENV|BUFFER]`. Final BUFFER is `bN`, a buffer to read as range multiplier.
| bandpass | `BUFID RATE WIDTH`. RATE and WIDTH in Hz, or `*N` for RATE.
| lopass   | `BUFID RATE`. RATE in Hz only.
| hipass   | `BUFID RATE`. RATE in Hz only.
| gain     | `BUFID GAIN CLIP GATE`
| delay    | `BUFID INTERVAL DRY WET STORE FEEDBACK`. INTERVAL in ms or `*N` to multiply against one qnote's length.
| add      | `DST SRC`
| mlt      | `BUFID MULTIPLIER`

If one of (prefix,voice,suffix) present, most other commands are an error. "master" is still allowed anywhere.

Binary:

| opcode | name        | args
|--------|-------------|------
| 0x01   | FM_A_S      | (u12.4 rate_hz,u8.8 range)
| 0x02   | FM_R_S      | (u8.8 rate_mlt,u8.8 range)
| 0x03   | FM_A_E      | (u12.4 rate_hz,u8.8 range,...range_env)
| 0x04   | FM_R_E      | (u8.8 rate_mlt,u8.8 range,...range_env)
| 0x05   | STDENV      | (...env)
| 0x06   | MASTER      | (u4.12 gain)
| 0x07   | PREFIX      | (u8 len,...pipe)
| 0x08   | VOICE       | (u8 len,...pipe)
| 0x09   | SUFFIX      | (u8 len,...pipe)
| 0xc0   | HELLO       | () Must be first op, to distinguish from minsyn.

Binary Pipe:

```
u8 opcode
  0xc0 buffer id 0..3
  0x3f opcode
... payload, depends on (opcode)

0x00 SILENCE ()
0x01 NOISE ()
0x02 COPY (u8 src)
0x03 PENV (...env)
0x04 OSC_A (u16 rate,u8 coefc,...coefv)
0x05 OSC_R (u8.8 rate,u8 coefc,...coefv)
0x06 PFM_A_A (u16 rate,u12.4 modrate,u8.8 range,...env) ; (env) may also be (0xf0|bufid) to source from a buffer
0x07 PFM_R_A (u8.8 rate_mlt,u12.4 modrate,u8.8 range,...env)
0x08 PFM_A_R (u16 rate,u8.8 modrate_mlt,u8.8 range,...env)
0x09 PFM_R_R (u8.8 rate_mlt,u8.8 modrate_mlt,u8.8 range,...env)
0x0a BANDPASS_A (u16 rate,u8.8 width)
0x0b BANDPASS_R (u8.8 rate,u8.8 width)
0x0c LOPASS (u16 rate)
0x0d HIPASS (u16 rate)
0x0e GAIN (u8.8 gain,u0.8 clip,u0.8 gate)
0x0f DELAY_A (u16 ms,u0.8 dry,u0.8 wet,u0.8 store,u0.8 feedback)
0x10 DELAY_R (u8.8 qnotes,u0.8 dry,u0.8 web,u0.8 store,u0.8 feedback)
0x11 ADD (u8 src)
0x12 MLT (u8.8 multiplier)
0x13 OSC_T (u8.8 rate(qnotes),u8 coefc,...coefv)
```

Text Envelope:

```
( [VINITIAL] TATTACK VATTACK TDECAY VSUSTAIN[*] TRELEASE [VFINAL] [.. [V] T V T V[*] T [V]] )

T are integers in ms.
V are floats in 0..1 or -1..1.

Initial and final level must be provided in both high and low edges, if present in one.
It's OK to supply only initial or only final -- we know what's what based on float-vs-int.

Star immediately after sustain level to indicate sustain is supported. (optional to repeat in high half)

We decide automatically whether to use high-res times and signed levels.
You may request hi-res levels by phrasing at least one "V" with more than 3 fractional digits.
```

Binary Envelope:

```
u8 features
  0x01 velocity
  0x02 sustain
  0x04 initial
  0x08 hires time: T=u16 ms if set, else T=u8 ms*4
  0x10 hires level: V=i16 if set, else V=i8
  0x20 signed level: Signed integers, and normalize to -1..1 instead of 0..1.
  0x40 final
  0x80 reserved
If (initial): V initial lo
T attack time lo
V attack level lo
T decay time lo
V sustain level lo
T release time lo
If (final): V final lo
If (velocity):
  If (initial): V initial hi
  T attack time hi
  V attack level hi
  T decay time hi
  V sustain level hi
  T release time hi
  If (final): V final hi
```

Envelope length is determinable from the first byte. Range 6..29.
