all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $1 $(@F)" ; mkdir -p $(@D) ;
force:

ifeq ($(MAKECMDGOALS),clean)
  clean:;rm -rf mid out
else

include etc/config.mk
etc/config.mk:|etc/config.mk.example;$(PRECMD) cp etc/config.mk.example $@
export TARGETS
  
SRCFILES:=$(shell find src -type f)

include etc/make/generate.mk
include etc/make/tools.mk

include etc/make/per-target.mk
$(foreach T,$(TARGETS), \
  $(shell mkdir -p mid/$T out/$T) \
  $(eval include etc/target/$T.mk) \
  $(eval $(call TARGET_RULES,$T)) \
)

pkg:all;etc/tool/mkpkg.sh

endif
