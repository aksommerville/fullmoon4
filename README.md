# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### TODO

- [ ] Windows: Must sign executable.
- [ ] MacOS: Must sign executable.
- [x] Touch input. ...already works. (i don't feel it's usable but it's not a technical problem)
- [ ] web incfg with hats: Coded but not tested. Validate on a Mac.
- [x] Sound effect for coin toss
- [ ] _Enchanting Adventures: The Witch's Quest_ minigame.
- [ ] Tree-shake resources when packing.
- [ ] stdsyn. Or drop it, maybe minsyn is adequate?
- [ ] Native fiddle tool.
- - [ ] MIDI-In and PCM/synth config at command line? Would be cool to have some functionality without the web app.
- - [ ] Broad support for level analysis -- I'd like to replace `assist` with this.
- - [ ] Mysterious segfault. Refreshed browser while song running and fiddle C code had changed.
- [ ] Playing first_frost via fiddle, with screwing around on ch 14, the bells timing is off.
- - Also noticing in blood_for_silver, as i build up inotify. (no midi in, just data changes)
- - These, and the segfault above, might be due to borrowed songs in minsyn:   if (!(DRIVER->song=midi_file_new_borrow(src,srcc))) return -1;
- - Problem is the live game depends on that serial address to detect redundant requests.
- - [ ] Can we move the redundant-song check up to the platform level, and make it undefined at the synth level?
- - I want songs to continue playing across archive reloads, in fiddle. (in fact this is very important)
- [ ] Measure full memory usage after all songs and sound effects have played -- I'm concerned it might be too high for the Pi.

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
