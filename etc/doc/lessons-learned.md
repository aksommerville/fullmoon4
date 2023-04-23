# Full Moon Lessons Learned

Things I did wrong here, that I aim to do right in future projects:

- Platform should be generic. Game logic in the game, esp rendering, platform shouldn't know what a Full Moon is.
- Songs should contain their instruments. Possible exception for drums.
- Don't use Node for data tooling, it's too inconsistent across versions. And too much overhead, when you're building thousands of resources.
- Need to auto-generate code for enums etc.
- Any chance of using generic instruments and sound effects definitions? Maintaining these per synthesizer is painful.
- Web as the "principal" platform. It's cool and all, but Native is my style.

Things I did right here, and try not to forget:

- Treat render and synth like drivers, keep them pluggable.
- Host config in etc/config.mk, generate only if missing.
- Text for all non-standard data formats.
- Monthly TODO list. Really keeps me on track.
