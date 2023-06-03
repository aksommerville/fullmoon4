# Full Moon Fiddle

A wee program to run the synthesizer and permit access to it via a web app or the MIDI bus.

## HTTP Service Overview

```
GET /api/status => {synth,song,instrument,sound}
GET /api/synths => [qualifier,...]
GET /api/sounds?qualifier => [{id,name},...]
GET /api/instruments?qualifier => [{id,name},...]
GET /api/songs => [{id,name},...]
GET /api/insusage => see below
GET /api/datarate => see below
POST /api/synth/use?qualifier
POST /api/sound/play?id
POST /api/song/play?id # Stops playback if id invalid (eg zero)
POST /api/midi?chid&opcode&a&b # HTTP is overkill for sending MIDI events, but maybe ok for one-off things?
GET /* # Static files from src/tool/fiddlec/www
```

There are no endpoints for reading or writing the actual sound effects or songs.
User is expected to do that in a text editor.
Fiddle watches the synth data files and updates immediately when they change. (TODO)
Currently, you need to refresh the web app after changing anything. Requesting `/` triggers a `make` on the server side.

### POST /api/synth/use

Must call this first, otherwise nothing else can work.
Send any unknown qualifier to turn sound off.

### GET /api/status

Returns:
```
{
  synth: string
  song: int
  instrument: int
  sound: int
}
```

(synth) is the driver's name eg "minsyn".

(instrument) is the instrument currently active on channel 14, ie the last Program Change on 14 that anybody sent.

(sound) is the ID of the last sound effect requested via `POST /api/sound/play`.

### GET /api/insusage

Report instrument usage by song.

Returns:
```
[{
  id: int,
  instruments: [int...],
  sounds: [int...],
},...]
```

Every song is included in the response, even if it has no instruments or sounds. (which would be a weird song).
Sounds, as used by a song, are note IDs on channel 15.
Everything we return is sorted by id.

### GET /api/datarate

This is not very useful, just something I'm curious about.
Because I like to brag about how small our encoded data is.

Returns:
```
{
  soundCount: int
  soundSize: int // bytes, total
  soundTime: int // duration, total, ms
  songCount: int
  songSize: int // bytes, total
  songTime: int // duration, total, ms
}
```

So from this, `(soundSize + songSize) / (soundTime + songTime)` is our average data rate in bytes per millisecond.
