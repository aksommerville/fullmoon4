# macos.mk
# MacOS 10+

macos_MIDDIR:=mid/macos
macos_OUTDIR:=out/macos

#TODO reenable gl2, it should be present but optional at runtime
macos_OPT_ENABLE:=soft minsyn bigpc macos datafile png fmstore inmgr midi pcmprint macaudio machid macioc macwm gl2

# TODO I guess the easier thing would be to make separate app bundles for Full and Demo.
macos_ICONS_DIR:=src/opt/macos/appicon.iconset
macos_XIB:=src/opt/macos/Main.xib
macos_PLIST_SRC:=src/opt/macos/Info.plist
macos_ICONS:=$(wildcard $(macos_ICONS_DIR)/*)
macos_BUNDLE:=$(macos_OUTDIR)/FullMoon.app
macos_PLIST:=$(macos_BUNDLE)/Contents/Info.plist
macos_NIB:=$(macos_BUNDLE)/Contents/Resources/Main.nib
macos_EXE:=$(macos_BUNDLE)/Contents/MacOS/fullmoon
macos_ICON:=$(macos_BUNDLE)/Contents/Resources/appicon.icns
macos_DATA_FULL:=$(macos_BUNDLE)/Contents/Resources/data-full
macos_DATA_DEMO:=$(macos_BUNDLE)/Contents/Resources/data-demo

$(macos_NIB):$(macos_XIB);$(PRECMD) ibtool --compile $@ $<
$(macos_PLIST):$(macos_PLIST_SRC);$(PRECMD) cp $< $@
$(macos_ICON):$(macos_ICONS);$(PRECMD) iconutil -c icns -o $@ $(macos_ICONS_DIR)

macos-all:$(macos_EXE) $(macos_DATA_FULL) $(macos_DATA_DEMO) $(macos_ICON) $(macos_NIB) $(macos_PLIST)

macos_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  $(macos_CC_EXTRA) \
  $(foreach U,$(macos_OPT_ENABLE),-DFMN_USE_$U=1)
macos_OBJC:=$(macos_CC) -xobjective-c
macos_LD:=gcc
macos_LDPOST:=-lz -framework OpenGL -framework CoreGraphics -framework IOKit -framework AudioUnit -framework Cocoa -framework Quartz 

# Always filter instrument and sound (0 means no resources produced, if no synthesizers enabled; these resources always have a qualifier).
macos_QFILTER:=instrument:0 sound:0
ifneq (,$(strip $(filter minsyn,$(macos_OPT_ENABLE))))
  macos_QFILTER+=instrument:minsyn sound:minsyn
endif
ifneq (,$(strip $(filter stdsyn,$(macos_OPT_ENABLE))))
  macos_QFILTER+=instrument:stdsyn sound:stdsyn
endif

$(eval $(call SINGLE_DATA_ARCHIVE,macos,$(macos_DATA_FULL),$(macos_DATA_DEMO),$(macos_QFILTER)))

# TODO Smarten up the automatic data file detection; do not require a command-line arg!
macos-run-full:$(macos_EXE) $(macos_DATA_FULL);open -W $(macos_BUNDLE) --args --reopen-tty=$$(tty) --chdir=$$(pwd) --data=$(macos_DATA_FULL) $(macos_RUN_ARGS)
macos-run-demo:$(macos_EXE) $(macos_DATA_DEMO);open -W $(macos_BUNDLE) --args --reopen-tty=$$(tty) --chdir=$$(pwd) --data=$(macos_DATA_DEMO) $(macos_RUN_ARGS)
macos-run:macos-run-demo
