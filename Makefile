all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $1 $(@F)" ; mkdir -p $(@D) ;

ifeq ($(MAKECMDGOALS),clean)
  clean:;rm -rf mid out
else

include etc/config.mk
etc/config.mk:|etc/config.mk.example;$(PRECMD) cp etc/config.mk.example $@
  
SRCFILES:=$(shell find src -type f)

include etc/make/per-target.mk
$(foreach T,$(TARGETS), \
  $(eval include etc/target/$T.mk) \
  $(eval $(call TARGET_RULES,$T)) \
)

endif
