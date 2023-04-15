# Full Moon Lessons Learned

Things I did wrong here, that I aim to do right in future projects:

- Platform should be generic. Game logic in the game, esp rendering, platform shouldn't know what a Full Moon is.
- Songs should contain their instruments. Possible exception for drums.
- Don't use Node for data tooling, it's too inconsistent across versions.
- Need to auto-generate code for enums etc.

Things I did right here, and try not to forget:

- Treat render and synth like drivers, keep them pluggable.
- Host config in etc/config.mk, generate only if missing.
- Text for all non-standard data formats.
