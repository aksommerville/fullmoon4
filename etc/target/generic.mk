# generic.mk
# Builds the C app with no provisions for I/O.
# This will not produce a playable game.
# Eventually, I want to use this for automation and such.

generic_MIDDIR:=mid/generic
generic_OUTDIR:=out/generic

generic_OPT_ENABLE:=generic_glue

generic_EXE:=$(generic_OUTDIR)/fullmoon
generic_DATA_FULL:=$(generic_OUTDIR)/data-full
generic_DATA_DEMO:=$(generic_OUTDIR)/data-demo
generic-all:$(generic_EXE) $(generic_DATA_FULL) $(generic_DATA_DEMO)

generic_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses
generic_LD:=gcc
generic_LDPOST:=-lm

$(eval $(call SINGLE_DATA_ARCHIVE,generic,$(generic_DATA_FULL),$(generic_DATA_DEMO)))
