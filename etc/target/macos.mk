# macos.mk
# MacOS 10+

macos_MIDDIR:=mid/macos
macos_OUTDIR:=out/macos

macos_OPT_ENABLE:=soft minsyn stdsyn bigpc macos datafile png fmstore inmgr midi pcmprint macaudio machid macioc macwm gl2

# TODO I guess the easier thing would be to make separate app bundles for Full and Demo.
macos_ICONS_DIR:=src/opt/macos/appicon.iconset
macos_XIB:=src/opt/macos/Main.xib
macos_PLIST_SRC:=src/opt/macos/Info.plist
macos_ICONS:=$(wildcard $(macos_ICONS_DIR)/*)

macos_BUNDLE_FULL:=$(macos_OUTDIR)/FullMoon.app
macos_PLIST_FULL:=$(macos_BUNDLE_FULL)/Contents/Info.plist
macos_NIB_FULL:=$(macos_BUNDLE_FULL)/Contents/Resources/Main.nib
macos_EXE_FULL:=$(macos_BUNDLE_FULL)/Contents/MacOS/fullmoon
macos_ICON_FULL:=$(macos_BUNDLE_FULL)/Contents/Resources/appicon.icns
macos_DATA_FULL:=$(macos_BUNDLE_FULL)/Contents/Resources/data-full

macos_BUNDLE_DEMO:=$(macos_OUTDIR)/FullMoonDemo.app
macos_PLIST_DEMO:=$(macos_BUNDLE_DEMO)/Contents/Info.plist
macos_NIB_DEMO:=$(macos_BUNDLE_DEMO)/Contents/Resources/Main.nib
macos_EXE_DEMO:=$(macos_BUNDLE_DEMO)/Contents/MacOS/fullmoon
macos_ICON_DEMO:=$(macos_BUNDLE_DEMO)/Contents/Resources/appicon.icns
macos_DATA_DEMO:=$(macos_BUNDLE_DEMO)/Contents/Resources/data-demo

# NIB, PLIST, ICON: Same for Full and Demo. So Demo will copy out of Full.
$(macos_NIB_FULL):$(macos_XIB);$(call PRECMD,macos) ibtool --compile $@ $<
$(macos_PLIST_FULL):$(macos_PLIST_SRC);$(call PRECMD,macos) cp $< $@
$(macos_ICON_FULL):$(macos_ICONS);$(call PRECMD,macos) iconutil -c icns -o $@ $(macos_ICONS_DIR)
$(macos_NIB_DEMO):$(macos_NIB_FULL);$(call PRECMD,macos) cp $< $@
$(macos_PLIST_DEMO):$(macos_PLIST_FULL);$(call PRECMD,macos) cp $< $@
$(macos_ICON_DEMO):$(macos_ICON_FULL);$(call PRECMD,macos) cp $< $@
$(macos_EXE_DEMO):$(macos_EXE_FULL);$(call PRECMD,macos) cp $< $@

macos-full-outputs:$(macos_EXE_FULL) $(macos_DATA_FULL) $(macos_ICON_FULL) $(macos_NIB_FULL) $(macos_PLIST_FULL)
macos-demo-outputs:$(macos_EXE_DEMO) $(macos_DATA_DEMO) $(macos_ICON_DEMO) $(macos_NIB_DEMO) $(macos_PLIST_DEMO)
macos-all:macos-full-outputs macos-demo-outputs
macos_EXE:=$(macos_EXE_FULL)

ifneq (,$(strip $(macos_CODESIGN_NAME)))
  macos-all:macos-codesign
  macos-codesign:macos-full-outputs macos-demo-outputs;$(call PRECMD,macos) \
    codesign -s '$(macos_CODESIGN_NAME)' -f $(macos_BUNDLE_FULL) && \
    codesign -s '$(macos_CODESIGN_NAME)' -f $(macos_BUNDLE_DEMO)
endif

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
macos-run-full:macos-all;open -W $(macos_BUNDLE_FULL) --args --reopen-tty=$$(tty) --chdir=$$(pwd) $(macos_RUN_ARGS)
macos-run-demo:macos-all;open -W $(macos_BUNDLE_DEMO) --args --reopen-tty=$$(tty) --chdir=$$(pwd) $(macos_RUN_ARGS)
macos-run:macos-run-demo
