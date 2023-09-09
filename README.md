# Full Moon

Non-violent adventure game where the witch has to find and kill the werewolf.

I mean "non-violent" in a pretty loose sense.
Because there is definitely a werewolf-slaying involved.

Tentatively aiming for full release 29 September 2023, the first full moon of autumn.

### TODO

- [x] The new ldcmd that I assumed was broken on Mac; it's only failing on clean builds due to MIDDIR not created yet. etc/make/per-target.mk
- - Make MIDDIR and OUTDIR at the top level, from Makefile, before anything else. Easy!
- - ...god damn it, no it seems it actually is broken on the iMac. `$$(file > PATH,CONTENT)` doesn't create a file.
- [x] Re-test on iMac; I changed some macwm window resizing logic.
- - ...of course it's fucking broken again
- [ ] Re-test on MacBook; smash both Macs with baseball bat if it doesn't render right.
- [x] I believe gameplay, graphics, and audio are complete, at v0.3.0. Confirm with a thorough playthrough on all devices, Demo and Full.
- - [x] iMac 3:15.498 9:46.619
- - [x] MacBook 3:05.121 8:33.225
- - [x] Pi 3:06.276 8:54.414
- - [x] VCS 3:21.450 9:03.565
- - [x] Nuc 3:02.859 8:46.005
- - [x] Dell 3:02.793 8:12.546
- - [x] Asus 2:58.524 8:29.773
- - [x] Web 2:54.989 8:01.584
- [ ] Info site downloads: Emphasize most recent version and inferred platform. (sorting isn't enough).
- - Maybe hide old versions? Just a UI thing, you click and they appear.
- [ ] Art and music for info site.
- [ ] Steam
- - [ ] Must reach "Coming Soon" two weeks ahead of release -- 2023-09-15
- - [ ] Can only accept Zip uploads. Update pack script.
- [ ] Freshen aksommerville.com landing page, link to both Plunder Squad and Full Moon.

### Links

- Itch, Full: https://aksommerville.itch.io/full-moon
- Itch, Demo: https://aksommerville.itch.io/full-moon-demo
- Steam: TODO
- My page: https://aksommerville.com/fullmoon
- Github: https://github.com/aksommerville/fullmoon4
- ("fullmoon4"? It's not a sequel, it's the fourth attempt :P )

### Disambiguation

There are other things called Full Moon, which are not this game.

- Bart Bonte's 2009 Flash game: https://www.bartbonte.com/fullmoon/
- ^ this is really cool
- The tabletop gaming store in Terre Haute: https://www.fullmoongames.com/
- Claude Leroy's nice-looking card game: https://boardgamegeek.com/boardgame/136523/full-moon
- Many other google hits containing "Full Moon". But hey, it's a reasonably common phrase.
- I think we're OK step-on-toes-wise.

### World Records

- Demo Any%
- Demo 100%: 2:54.989 AK Web
- Full Any%
- Full 100%: 8:01.584 AK Web
