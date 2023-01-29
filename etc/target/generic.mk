# generic.mk
# Builds the C app with no provisions for I/O.
# This will not produce a playable game.
# Eventually, I want to use this for automation and such.

generic_MIDDIR:=mid/generic
generic_OUTDIR:=out/generic

generic_OPT_ENABLE:=generic_glue

generic_EXE:=$(generic_OUTDIR)/fullmoon

generic_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit
generic_LD:=gcc
generic_LDPOST:=-lm
