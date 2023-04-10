# linux.mk
# Builds for Linux, whatever the host architecture is.
# We'll use dynamic driver selection at runtime, so the same build will support eg GLX and DRM.

linux_MIDDIR:=mid/linux
linux_OUTDIR:=out/linux

# Truly optional units, ie you can change these (remember to update LDPOST et al).
# You must enable at least one of (glx,drm), otherwise you're not going to see anything.
linux_OPT_ENABLE:=evdev alsa glx drm gl2 soft stdsyn minsyn

# The rest are mandatory, no alternatives:
linux_OPT_ENABLE+=bigpc genioc linux datafile png fmstore inmgr midi

linux_EXE:=$(linux_OUTDIR)/fullmoon
linux_DATA:=$(linux_OUTDIR)/data
linux-all:$(linux_EXE) $(linux_DATA)

linux_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  $(linux_CC_EXTRA) \
  -I/usr/include/libdrm \
  $(foreach U,$(linux_OPT_ENABLE),-DFMN_USE_$U=1)
linux_LD:=gcc
linux_LDPOST:=-lm -lX11 -lGL -lGLX -lEGL -ldrm -lgbm -lz -lpthread

ifneq (,$(linux_USE_XINERAMA))
  linux_OPT_ENABLE+=xinerama
  linux_CC+=-DFMN_USE_xinerama
  linux_LDPOST+=-lXinerama
endif

$(eval $(call SINGLE_DATA_ARCHIVE,linux,$(linux_DATA)))

linux-run:$(linux_EXE) $(linux_DATA);$(linux_EXE) --data=$(linux_DATA)
