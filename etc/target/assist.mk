# assist.mk
# Builds the native tooling.
# You don't need this if working exclusively with web, but do if you have any other targets.
# (and that will probably change soon; I want Assist to replace the Javascript build-time tools).

assist_MIDDIR:=mid/assist
assist_OUTDIR:=out/assist

assist_OPT_ENABLE:=alsa soft stdsyn minsyn datafile png midi pcmprint assist

assist_SRCFILES_FILTER_OUT:=src/app/%
assist_SRCFILES_EXTRA:=src/app/fmn_pixfmt.c

assist_EXE:=$(assist_OUTDIR)/assist
assist_DATA:=$(assist_OUTDIR)/data
assist-all:$(assist_EXE) $(assist_DATA)

assist_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  $(assist_CC_EXTRA) \
  $(foreach U,$(assist_OPT_ENABLE),-DFMN_USE_$U=1)
assist_LD:=gcc
assist_LDPOST:=-lm -lz -lpthread

$(eval $(call SINGLE_DATA_ARCHIVE,assist,$(assist_DATA)))

assist-run:$(assist_EXE) $(assist_DATA);$(assist_EXE) --data=$(assist_DATA) $(assist_RUN_ARGS)
