# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### TODO

- [ ] Windows: Must sign executable.
- [ ] MacOS: Must sign executable.
- [ ] MacOS: Compare iMac to MacBook builds. Are they identical? Can each run on the other machine?
- [ ] web incfg with hats: Coded but not tested. Validate on a Mac.
- [ ] stdsyn.
- - [x] Ensure we implement music repeat (check end credits)
- - [x] Also music suspend (check violin) and disable (via settings)
- - [x] env: Can we do nonzero initial and final levels? Will want this for eg pitch bend, fm range
- - [x] Generic node.
- - [x] Tuned nodes.
- - [x] Tangled Vine and Toil And Trouble both seem to terminate early, last few notes don't play.
- - - Snowglobe, too, the last note kind of cuts off.
- - - Due to misinterpretted Meta events, oops.
- - [x] Are there stray notes playing in Sky Gardening?
- - - There's some kind of buffer problem. I sometimes hear a note re-trigger quickly. Most apparent when using violin.
- - [x] Clipping. Is there a general strategy we can use to mitigate?
- - - What I was hearing was mostly noise introduced by faulty memsets in pcm node, and reset-to-sustain level when env releases.
- - [x] Channel volume. I don't think any song actually uses it.
- - [ ] Stereo.
- - [x] Release notes when violin comes up.
- - [x] I suspect waves are being produced beyond the nominal -1..1 range. eg Snowglobe song is way too loud.
- - - Snowglobe, program 29 is loud (~0.94) but within range.
- - - Let's not worry about this; I'm going to rewrite all the stdsyn instruments anyway.
- - - (fwiw, Blood for Silver and Eye of Newt instruments do go OOB; I did that deliberately to clip at the wave level)
- - [ ] Tangled Vine, there's a sour low note in the ornamental octave bits in the second A section.
- - [x] Some kind of memory fault on song repeat. Got unexpected behavior twice in a row from Toil and Trouble.
- - - Reproducible by making a song play for the second time. eg leave area and return. Something about reusing the instrument?
- - - Assigning 'wave' instead of 'mixwave', oops.
- - [x] Master trim per instrument.
- - [x] Finish all pipe stages.
- - [x] Fancify all instruments.
- - - [x] 1-tangled_vine.mid
- - - [x] 2-seven_circles.mid
- - - [x] 3-toil_and_trouble.mid
- - - [x] 4-eye_of_newt.mid
- - - - [ ] Tone down the rhythm guitar a little.
- - - [x] 5-jaws_of_wrath.mid
- - - [x] 6-truffles_in_forbidden_sauce.mid
- - - [x] 7-seventh_roots_of_unity.mid
- - - [x] 8-snowglobe.mid
- - - - [ ] too loud
- - - [x] 9-choose_a_door.mid
- - - [x] 10-blood_for_silver.mid
- - - - [ ] I love this, but is it too much?
- - - [x] 11-first_frost.mid
- - - [x] 12-gloom_for_company.mid
- - - [x] 13-nearer_the_sky.mid
- - - [x] 14-sky-gardening.mid
- - [x] stdsyn is murder on the CPU. Since adding ctl3, I'm seeing CPU usage >20%. Can we optimize somewhere?
- - - [x] Measure performance closely.
- - - [x] It's not the new programs, it's some kind of leak -- in real play, CPU usage climbs steady
- - - [x] CONFIRMED: ctl3 programs are leaking, but not all of them. Travel between forest and village, the village's ctl3 program leaks.
- - - - ...because ctl3 is leaking voices. They never disappear. ...seems to be pfm at fault.
- [ ] Info site downloads: Emphasize most recent version and inferred platform. (sorting isn't enough).
- - Maybe hide old versions? Just a UI thing, you click and they appear.
- [ ] Art and music for info site.
- [x] Itch site for full version
- [ ] Steam
- - [x] Provision app, bakshish.
- - [ ] Must reach "Coming Soon" two weeks ahead of release -- 2023-09-15
- - [ ] Can only accept Zip uploads. Update pack script.
- [x] Promo graphics
- - Itch banner: 630x500
- - Steam banners?
- - - Header Capsule: 460x215. Full branding.
- - - Small Capsule: 231x87. Mostly logo.
- - - Main Capsule: 616x353. Top of front page.
- - - Vertical Capsule: 374x448. 
- - - Page Background: 1438x810. Optional.
- - - Screenshots x5.
- - - Library Capsule: 600x900. Title but no other text.
- - - Library Hero: 3840x1240. No text. 860x380 "safe zone" in the center.
- - - Library Logo: 1280x720. Transparent background, will overlay Library Hero.
- - - Client Image: 16x16. (In "Installation")
- - - Client Icon: 32x32 ''
- - [ ] PDF manual. Any point doing that?
- - [ ] Video trailer
- [ ] Freshen aksommerville.com landing page, link to both Plunder Squad and Full Moon.
- [ ] Slow motion vs conveyor belts

### Links

- (Itch, Full)[https://aksommerville.itch.io/full-moon]
- (Itch, Demo)[https://aksommerville.itch.io/full-moon-demo]
- Steam: TODO
- (My page)[https://aksommerville.com/fullmoon]

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.
