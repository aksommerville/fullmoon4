# linux.mk
# Builds for Linux, whatever the host architecture is.
# We'll use dynamic driver selection at runtime, so the same build will support eg GLX and DRM.
#
# Set these flags (empty=false, not-empty=true) in your config.mk, for optional units.
# These all involve extra libraries.
#   linux_USE_GLX
#   linux_USE_XINERAMA: Only relevant if linux_USE_GLX
#   linux_USE_DRM
#   linux_USE_ALSA
#   linux_USE_PULSE
# If either GLX or DRM is requested, you'll get 'gl2' as well.
# (and you must request at least one of those, otherwise there's no video!)

linux_MIDDIR:=mid/linux
linux_OUTDIR:=out/linux

# Optional units that I didn't bother exposing with "linux_USE_" flags.
linux_OPT_ENABLE:=evdev soft minsyn stdsyn

# The rest are mandatory, no alternatives:
linux_OPT_ENABLE+=bigpc genioc linux datafile png fmstore inmgr midi pcmprint

linux_EXE:=$(linux_OUTDIR)/fullmoon
linux_DATA_FULL:=$(linux_OUTDIR)/data-full
linux_DATA_DEMO:=$(linux_OUTDIR)/data-demo
linux-all:$(linux_EXE) $(linux_DATA_FULL) $(linux_DATA_DEMO)

linux_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  $(linux_CC_EXTRA) \
  $(foreach U,$(linux_OPT_ENABLE),-DFMN_USE_$U=1)
linux_LD:=gcc
linux_LDPOST:=-lm -lz

ifneq (,$(linux_USE_STDSYN))
  linux_OPT_ENABLE+=stdsyn
endif
ifneq (,$(linux_USE_GLX))
  linux_OPT_ENABLE+=glx
  linux_CC+=-DFMN_USE_glx=1
  linux_LDPOST+=-lX11
  ifneq (,$(linux_USE_XINERAMA))
    linux_OPT_ENABLE+=xinerama
    linux_CC+=-DFMN_USE_xinerama
    linux_LDPOST+=-lXinerama
  endif
endif
ifneq (,$(linux_USE_DRM))
  linux_OPT_ENABLE+=drm
  linux_CC+=-DFMN_USE_drm=1 -I/usr/include/libdrm
  linux_LDPOST+=-ldrm -lEGL -lgbm
endif
ifneq (,$(linux_USE_GLX)$(linux_USE_DRM))
  linux_OPT_ENABLE+=gl2
  linux_CC+=-DFMN_USE_gl2=1
  linux_LDPOST+=-lGL
endif
ifneq (,$(linux_USE_ALSA))
  linux_OPT_ENABLE+=alsa
  linux_CC+=-DFMN_USE_alsa=1
endif
ifneq (,$(linux_USE_PULSE))
  linux_OPT_ENABLE+=pulse
  linux_CC+=-DFMN_USE_pulse=1
  linux_LDPOST+=-lpulse-simple
endif
ifneq (,$(linux_USE_ALSA)$(linux_USE_PULSE))
  linux_LDPOST+=-lpthread
endif
ifneq (,$(strip $(GAMEMON)))
  linux_OPT_ENABLE+=gamemon
  linux_CC+=-DFMN_USE_gamemon=1 -I$(GAMEMON)/src/lib
  linux_LDPOST+=$(GAMEMON)/out/lib/libgamemon.a
endif

# Always filter instrument and sound (0 means no resources produced, if no synthesizers enabled; these resources always have a qualifier).
linux_QFILTER:=instrument:0 sound:0
ifneq (,$(strip $(filter minsyn,$(linux_OPT_ENABLE))))
  linux_QFILTER+=instrument:minsyn sound:minsyn
endif
ifneq (,$(strip $(filter stdsyn,$(linux_OPT_ENABLE))))
  linux_QFILTER+=instrument:stdsyn sound:stdsyn
endif

$(eval $(call SINGLE_DATA_ARCHIVE,linux,$(linux_DATA_FULL),$(linux_DATA_DEMO),$(linux_QFILTER)))

linux-run-full:$(linux_EXE) $(linux_DATA_FULL);$(linux_EXE) --data=$(linux_DATA_FULL) $(linux_RUN_ARGS)
linux-run-demo:$(linux_EXE) $(linux_DATA_DEMO);$(linux_EXE) --data=$(linux_DATA_DEMO) $(linux_RUN_ARGS)
linux-run:linux-run-full

linux-verify:$(tools_EXE_datan) $(linux_DATA_FULL) $(linux_DATA_DEMO);$(tools_EXE_datan) --src=src/data --archive=$(linux_DATA_FULL) --archive=$(linux_DATA_DEMO)

linux-adulterate:;src/tool/adulterate/adulterate.js $(linux_OUTDIR)/save
