# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Target platform is web browsers only.
There will be a "platform" layer in Javascript, and an "app" layer in C compiled to WebAssembly.

^ meaning "*initial* target platform". I am definitely making a Linux-native version, and yknow, as many others as we can.

## TODO

I want this thing ready to show off at GDEX 2023. Anything not necessary for demoing there can wait.

And tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### March

- [ ] Maps for demo.
- - [ ] Think on interiors, what should these walls look like?
- - [ ] pitcher house: 4 pictures on the wall: river, tree, bee, cow
- - [ ] violin house: challenge
- - [ ] violin house: teach a song. Lullabye? ha ha yes, there should be people lingering around and they fall asleep
- - [x] moonsong's basement: teach Spell of Opening
- - - It would be a terrible mistake to go into the basement if you are a pumpkin. Can we make that impossible?
- - - (I believe it currently is possible to make both witches pumpkins) -- CONFIRMED, easy, just cast the pumpkin spell but hold it until she gets you
- - - Or just open the gate automatically if you're a pumpkin and on the right of it?
- - [x] Put some matches down here too, because you have to learn the spell to get back out!
- - [ ] building by graveyard: church
- - [ ] church: teach the Wind Spell, and the chalkguard reference
- - [ ] Must somehow make some exception to the interior weather rule, or else send her outside to learn it.
- - - Climb to the belfry?
- - [ ] Some kind of excuse to dilly dally outside the church, so the ghosts can catch you
- - [ ] forest: challenges
- - [ ] forest: Song of Blooming
- - [ ] forest: Beehive. By the rabbit?
- - [ ] The Exciting Conclusion. Can we do the actual werewolf fight?
- [ ] Graphics.
- - [ ] Village interiors
- - [ ] Church
- - [ ] Castle exterior
- - [ ] Mountains exterior
- - [ ] Desert
- - [ ] Beach
- [ ] Out-to-sea blowback maps.
- [ ] farmer: shovel sound
- [ ] farmer: tweak position
- [ ] farmer: Sow and water on separate trips. Let the hero intervene.
- [ ] Simplify the Home spell. Maybe "SSS", like clicking your heels three times?
- [ ] Can we make like "song only if nothing yet" for map 1? It's jarring when you're just passing thru.

### April

- [ ] Compass and crow: How to decide on target location? see src/app/fmn_secrets.c
- [ ] Map gamepads necessary for demo (mind that they will be different per browser and os).
- [ ] UI for saved game management.
- [ ] Chrome Linux, I get 1-pixel artifacts when scaled up. Visible when the violin chart is up.
- - Would it help to scale up only by integers? That's probably a good idea in any case.
- - We're using straight `object-fit:contain` on a canvas that fills the entire available space. Will be tricky.
- [ ] Finalize sound effects.
- [ ] Hero's shadow when flying goes under the conveyor belts, but over would look more natural. Can anything be done?

### May

- [ ] Order merch.
- - [ ] Coloring book. (printingcenterusa.com says about $1/ea for 250)
- - [ ] Stickers and pins: Stickermule (they do t-shirts too but too expensive)
- - [ ] T-shirts. (Bolt is cheapest I've found, and they did good with Plunder Squad. UberPrints looks ok but more expensive)
- - [ ] Notebooks
- - [ ] Pencils
- - [ ] Baseball cards: Get in touch with Lucas, maybe we can do a full-GDEX set of cards? Emailed 2023-03-11. Gotprint.com: $35/250. 875x1225px
- - Probly no need for thumb drives if we've only got the demo.
- [ ] Beta test.
- [ ] Pretty up the public web page.
- [ ] Lively intro splash.
- [ ] Bonus secret 2-player mode: Second player is a ghost that can make wind.
- [ ] The Cheat Book. A nice looking "WARNING SPOILERS" notebook we can leave out for demo players.

### After GDEX

- [ ] Data editor improvements.
- - [ ] Home page
- - [ ] MapAllUi: Point out neighbor mismatches.
- - [ ] MapAllUi: Populate tattle.
- - [ ] Delete maps (and resources in general)
- - [ ] ImageAllUi: Show names
- [ ] Build-time support for enums and such? Thinking FMN_SPRITE_STYLE_ especially, it's a pain to add one.
- [ ] verify: analyze map songs, ensure no map could have a different song depending on entry point
- [ ] verify: Check neighbor edges cell by cell (be mindful of firewall)
- [ ] verify: Tile 0x0f must be UNSHOVELLABLE in all tile sheets.
- [ ] verify: Song channel 14 reserved for violin, songs must not use.
- [ ] verify: Teleport targets must set song.
- [ ] verify: Firewall must be on edge, with at least two vacant cells, and a solid on both ends.
- [ ] verify: Chalk duplicates
- [ ] verify: Resources named by sprite config, eg chalkguard strings
- [ ] verify: 'indoors' should be the same for all edge neighbors, should change only when passing thru a door
- [ ] Remove hard-coded teleport targets, store in the archive (fmn_spell_cast).
- [ ] Make up a new item to take Corn's place.
- [ ] Maps for full game.
- [ ] Ensure maximum update interval is short enough to avoid physics errors, eg walking thru walls. Currently 1..50 ms per Clock.js
- [ ] Detect heavy drops in requestAnimationFrame rate and stop music if too low.
- [ ] Rekajigger Constants.js, use actual inlinable constants.
- [ ] Filter resources by qualifier, see src/tool/mkdata/packArchive.js
- [ ] Translation.
- [ ] Web input: Key events are triggering the pause button when focussed. Why isn't it just space bar?
- [ ] Touch input.
- [ ] Input configuration.
- - [ ] Generalize mapping, see src/www/js/game/InputManager.js
- - [ ] Persist gamepad and keyboard mapping.
- - [ ] UI for mapping.
- [ ] Extra mappable input actions, eg hard pause and fullscreen toggle. I specifically don't want these for the demo, so punt.
- [ ] Finish everything by Halloween. (to this point in the list at least)
- [ ] Consider WebGL for rendering. CanvasRenderingContext2D is not performing to my hopes.
- [x] Move Map decoding out of DataService into a class of its own.
- [ ] Unit tests.
- - [ ] General-purpose test runner.
- - [ ] C tests
- - [ ] JS platform tests
- [ ] Automation against headless native build.
- [ ] Other platforms:
- - [ ] Web wrapper eg Electron
- - [ ] Linux
- - [ ] Tiny. I don't think it will work, but prove it.
- - [ ] MacOS
- - [ ] Windows
- - [ ] iOS
- - [ ] Android
- - [ ] Pippin
- - [ ] PicoSystem
- - [ ] Thumby
- - [ ] PocketStar
- - [ ] Playdate
- - [ ] 68k Mac
- - [ ] Wii
- - [ ] Modern consoles?
- - [ ] node or deno? Shame to write all this Javascript and use it for just one platform...

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
