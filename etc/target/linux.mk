# linux.mk
# Builds for Linux, whatever the host architecture is.
# We'll use dynamic driver selection at runtime, so the same build will support eg GLX and DRM.

linux_MIDDIR:=mid/linux
linux_OUTDIR:=out/linux

# Truly optional units, ie you can change these (remember to update LDPOST et al).
# You must enable at least one of (glx,drm), otherwise you're not going to see anything.
linux_OPT_ENABLE:=evdev gl2 soft minsyn

# The rest are mandatory, no alternatives:
linux_OPT_ENABLE+=bigpc genioc linux datafile png fmstore inmgr midi pcmprint

linux_EXE:=$(linux_OUTDIR)/fullmoon
linux_DATA_FULL:=$(linux_OUTDIR)/data-full
linux_DATA_DEMO:=$(linux_OUTDIR)/data-demo
linux-all:$(linux_EXE) $(linux_DATA_FULL) $(linux_DATA_DEMO)

linux_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  $(linux_CC_EXTRA) \
  -I/usr/include/libdrm \
  $(foreach U,$(linux_OPT_ENABLE),-DFMN_USE_$U=1)
linux_LD:=gcc
linux_LDPOST:=-lm -lGL -lz -lpthread

ifneq (,$(linux_USE_XINERAMA))
  linux_OPT_ENABLE+=xinerama
  linux_CC+=-DFMN_USE_xinerama
  linux_LDPOST+=-lXinerama
endif
ifneq (,$(linux_USE_GLX))
  linux_OPT_ENABLE+=glx
  linux_CC+=-DFMN_USE_glx=1
  linux_LDPOST+=-lX11
endif
ifneq (,$(linux_USE_DRM))
  linux_OPT_ENABLE+=drm
  linux_CC+=-DFMN_USE_drm=1
  linux_LDPOST+=-ldrm -lEGL -lgbm
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
linux-run:linux-run-demo

linux-verify:$(linux_DATA_FULL) $(linux_DATA_DEMO); \
  $(NODE) src/tool/verify/main.js --archive=$(linux_DATA_FULL) --dir=src/data && \
  $(NODE) src/tool/verify/main.js --archive=$(linux_DATA_DEMO) --dir=src/data
