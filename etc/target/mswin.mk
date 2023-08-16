# mswin.mk
# Builds for Microsoft Windows.
# I plan to do the Windows build from Linux, so there is (mswin_TOOLCHAIN) prefix on all binaries.
# Uses bigpc like Linux, so in theory one has a choice of drivers.
# But I expect to only write one of each driver for Windows.
# To that end: All the Windows-specific drivers will live in the 'mswin' unit. No a-la-carte selection like Linux.

mswin_MIDDIR:=mid/mswin
mswin_OUTDIR:=out/mswin

# Can add stdsyn here, if I ever write it.
# Would like to have gl2 but will need mingw support.
mswin_OPT_ENABLE:=soft minsyn bigpc genioc mswin datafile png fmstore inmgr midi pcmprint

mswin_EXE:=$(mswin_OUTDIR)/fullmoon.exe
mswin_DATA_FULL:=$(mswin_OUTDIR)/data-full
mswin_DATA_DEMO:=$(mswin_OUTDIR)/data-demo
mswin-all:$(mswin_EXE) $(mswin_DATA_FULL) $(mswin_DATA_DEMO)

mswin_CC:=$(mswin_TOOLCHAIN)gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses -m32 -mwindows -D_POSIX_THREAD_SAFE_FUNCTIONS \
  -D_PTW32_STATIC_LIB=1 -D_WIN32_WINNT=0x0501 \
  $(mswin_CC_EXTRA) \
  $(foreach U,$(mswin_OPT_ENABLE),-DFMN_USE_$U=1)
mswin_LD:=$(mswin_TOOLCHAIN)gcc -m32 -mwindows -Wl,-static
mswin_LDPOST:=-lm -lz -lopengl32 -lglut -lwinmm -lhid

# From Plunder Squad:
#WINDEF:=-m32 -mwindows -D_WIN32_WINNT=0x0501 -D_POSIX_THREAD_SAFE_FUNCTIONS=1 -D_PTW32_STATIC_LIB=1

# Always filter instrument and sound (0 means no resources produced, if no synthesizers enabled; these resources always have a qualifier).
mswin_QFILTER:=instrument:0 sound:0
ifneq (,$(strip $(filter minsyn,$(mswin_OPT_ENABLE))))
  mswin_QFILTER+=instrument:minsyn sound:minsyn
endif
ifneq (,$(strip $(filter stdsyn,$(mswin_OPT_ENABLE))))
  mswin_QFILTER+=instrument:stdsyn sound:stdsyn
endif

$(eval $(call SINGLE_DATA_ARCHIVE,mswin,$(mswin_DATA_FULL),$(mswin_DATA_DEMO),$(mswin_QFILTER)))

mswin-run-full:$(mswin_EXE) $(mswin_DATA_FULL);wine $(mswin_EXE) --data=$(mswin_DATA_FULL) $(mswin_RUN_ARGS)
mswin-run-demo:$(mswin_EXE) $(mswin_DATA_DEMO);wine $(mswin_EXE) --data=$(mswin_DATA_DEMO) $(mswin_RUN_ARGS)
mswin-run:mswin-run-full

mswin-verify:$(tools_EXE_datan) $(mswin_DATA_FULL) $(mswin_DATA_DEMO);$(tools_EXE_datan) --src=src/data --archive=$(mswin_DATA_FULL) --archive=$(mswin_DATA_DEMO)

mswin-adulterate:;src/tool/adulterate/adulterate.js $(mswin_OUTDIR)/save
