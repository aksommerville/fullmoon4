# macos.mk
# Builds for Linux, whatever the host architecture is.
# We'll use dynamic driver selection at runtime, so the same build will support eg GLX and DRM.

macos_MIDDIR:=mid/macos
macos_OUTDIR:=out/macos

macos_OPT_ENABLE:=gl2 soft minsyn bigpc macos datafile png fmstore inmgr midi pcmprint macaudio machid macioc macwm

macos_ICONS_DIR:=src/opt/macos/appicon.iconset
macos_XIB:=src/opt/macos/Main.xib
macos_PLIST_SRC:=src/opt/macos/Info.plist
macos_ICONS:=$(wildcard $(macos_ICONS_DIR)/*)
macos_ICONS:=$(wildcard $(macos_ICONS_DIR)/*)
macos_BUNDLE:=$(macos_OUTDIR)/FullMoon.app
macos_PLIST:=$(macos_BUNDLE)/Contents/Info.plist
macos_NIB:=$(macos_BUNDLE)/Contents/Resources/Main.nib
macos_EXE:=$(macos_BUNDLE)/Contents/MacOS/fullmoon
macos_ICON:=$(macos_BUNDLE)/Contents/Resources/appicon.icns
macos_DATA:=$(macos_BUNDLE)/Contents/Resources/data

$(macos_NIB):$(macos_XIB);$(PRECMD) ibtool --compile $@ $<
$(macos_PLIST):$(macos_PLIST_SRC);$(PRECMD) cp $< $@
$(macos_ICON):$(macos_ICONS);$(PRECMD) iconutil -c icns -o $@ $(macos_ICONS_DIR)

macos-all:$(macos_EXE) $(macos_DATA) $(macos_ICON) $(macos_NIB) $(macos_PLIST)

macos_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  $(macos_CC_EXTRA) \
  $(foreach U,$(macos_OPT_ENABLE),-DFMN_USE_$U=1)
macos_OBJC:=$(macos_CC) -xobjective-c
macos_LD:=gcc
macos_LDPOST:=-lz -framework OpenGL -framework CoreGraphics -framework IOKit -framework AudioUnit -framework Cocoa -framework Quartz

$(eval $(call SINGLE_DATA_ARCHIVE,macos,$(macos_DATA)))

#macos-run:$(macos_EXE) $(macos_DATA);$(macos_EXE) --data=$(macos_DATA) $(macos_RUN_ARGS)
macos-run:macos-all;open -W $(macos_BUNDLE) --args --reopen-tty=$$(tty) --chdir=$$(pwd)
