# Full Moon Fiddle

A wee program to run the synthesizer and permit access to it via a web app or the MIDI bus.

## HTTP Service

```
GET /api/status => {synth,song}
GET /api/synths => [qualifier,...]
GET /api/sounds?qualifier => [{id,name},...]
GET /api/instruments?qualifier => [{id,name},...]
GET /api/songs => [{id,name},...]
POST /api/synth/use?qualifier # Must select a synth before playing sounds or songs. Unknown name, sound is off.
POST /api/sound/play?id
POST /api/song/play?id # Stops playback if id invalid (eg zero)
POST /api/midi?chid&opcode&a&b # HTTP is overkill for sending MIDI events, but maybe ok for one-off things?
GET /* # Static files from src/tool/fiddlec/www
```

There are no endpoints for reading or writing the actual sound effects or songs.
User is expected to do that in a text editor.
Fiddle watches the synth data files and updates immediately when they change. (TODO)
Currently, you need to refresh the web app after changing anything. Requesting `/` triggers a `make` on the server side.