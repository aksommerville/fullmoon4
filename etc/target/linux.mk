# linux.mk
# Builds for Linux, whatever the host architecture is.
# We'll use dynamic driver selection at runtime, so the same build will support eg GLX and DRM.

linux_MIDDIR:=mid/linux
linux_OUTDIR:=out/linux

# 'xinerama' is not a code unit; it's a flag to 'glx', allowing it to use libXinerama.
linux_OPT_ENABLE:=bigpc genioc linux evdev alsa glx xinerama drm

linux_EXE:=$(linux_OUTDIR)/fullmoon

linux_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  -I/usr/include/libdrm \
  $(foreach U,$(linux_OPT_ENABLE),-DFMN_USE_$U=1)
linux_LD:=gcc
linux_LDPOST:=-lm -lX11 -lGL -lGLX -lXinerama -lEGL -ldrm -lgbm

linux-run:$(linux_EXE);$(linux_EXE)
