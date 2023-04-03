# Full Moon: Adding a Platform

2023-04-02: Taking notes as I implement Linux, the first native platform.

1. Copy one of the configs from `etc/target`. `generic` is probably a good starting point.
2. Add targets to the new config, always putting the config name first. I'm adding `linux-run`.
3. Add to `etc/config.mk`. (update the `run` target for convenience)
4. `make` at this point, it should work up to linking, then fail with a bunch of "no such symbol: fmn_yadda_yadda".
5. I'm making a brand new platform from scratch, so copy all the function declarations from fmn_platform.h and implement stubs.
6. ~We also need a `main`, I'll put that in its own file in bigpc.~
7. Actually, scratch that. Require target platforms to supply `main`, bigpc should only make doing so easy.

## bigpc

Most of what needs written for the Linux platform is not going to be Linux-specific.
I'll add an optional unit "bigpc" for all platforms that work more or less like a PC,
and have the horsepower to manage runtime driver selection and all that.

MacOS, Windows, Wii, most of these reasonably capable native platforms, should all use bigpc.


