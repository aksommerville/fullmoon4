# raspi.mk
# Basically the same thing as Linux, but there's a special video driver "bcm", and "stdsyn" is not an option.

raspi_MIDDIR:=mid/raspi
raspi_OUTDIR:=out/raspi

raspi_OPT_ENABLE:=bcm evdev alsa gl2 minsyn bigpc genioc linux datafile png fmstore inmgr midi pcmprint

raspi_EXE:=$(raspi_OUTDIR)/fullmoon
raspi_DATA_FULL:=$(raspi_OUTDIR)/data-full
raspi_DATA_DEMO:=$(raspi_OUTDIR)/data-demo
raspi-all:$(raspi_EXE) $(raspi_DATA_FULL) $(raspi_DATA_DEMO)

raspi_CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-comment -Wno-parentheses \
  -I/opt/vc/include \
  $(raspi_CC_EXTRA) \
  $(foreach U,$(raspi_OPT_ENABLE),-DFMN_USE_$U=1)
raspi_LD:=gcc -L/opt/vc/lib
raspi_LDPOST:=-lm -lGLESv2 -lEGL -lz -lpthread -lbcm_host

raspi_QFILTER:=instrument:minsyn sound:minsyn
$(eval $(call SINGLE_DATA_ARCHIVE,raspi,$(raspi_DATA_FULL),$(raspi_DATA_DEMO),$(raspi_QFILTER)))

raspi-run-full:$(raspi_EXE) $(raspi_DATA_FULL);$(raspi_EXE) --data=$(raspi_DATA_FULL)
raspi-run-demo:$(raspi_EXE) $(raspi_DATA_DEMO);$(raspi_EXE) --data=$(raspi_DATA_DEMO)
raspi-run:raspi-run-demo
