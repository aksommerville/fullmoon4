all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $1 $(@F)" ; mkdir -p $(@D) ;

ifeq ($(MAKECMDGOALS),clean)
  clean:;rm -rf mid out
else

# XXX playing around to get automatic release tags...
BUILDTAG:=$(strip $(shell git status -s))
ifeq (,$(BUILDTAG)) # Working directory is clean, look for a tag.
  $(info wd clean)
  GITHEAD:=$(shell git rev-parse HEAD)
endif

include etc/config.mk
etc/config.mk:|etc/config.mk.example;$(PRECMD) cp etc/config.mk.example $@
  
SRCFILES:=$(shell find src -type f)

include etc/make/generate.mk
include etc/make/tools.mk

include etc/make/per-target.mk
$(foreach T,$(TARGETS), \
  $(eval include etc/target/$T.mk) \
  $(eval $(call TARGET_RULES,$T)) \
)

endif
