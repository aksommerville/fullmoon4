# per-target.mk
# Runs for each enabled target platform, after its target-specific Makefile.
# The target's Makefile must define:
#   $(TARGET)_CC
#   $(TARGET)_LD
#   $(TARGET)_LDPOST
#   $(TARGET)_OPT_ENABLE
#   $(TARGET)_EXE
# And optionally:
#   $(TARGET)_CXX
#   $(TARGET)_AS
#   $(TARGET)_OBJC

define TARGET_RULES

all:$1-all

#--------------------------------------------------------
# Discover input files, compile code, link the executable...

$1_SRCPAT:=src/app/% $$($1_MIDDIR)/generated/% $(foreach U,$($1_OPT_ENABLE),src/opt/$U/%)
$1_SRCFILES:=$$(filter-out $$($1_SRCFILES_FILTER_OUT),$$(filter $$($1_SRCPAT),$(SRCFILES) $$(addprefix $$($1_MIDDIR)/generated/,$(GENERATED_FILES))))

$1_CFILES:=$$(filter %.c %.m %.cpp %.S,$$($1_SRCFILES))
$1_OFILES:=$$(addsuffix .o,$$(patsubst src/%,$$($1_MIDDIR)/%,$$(basename $$($1_CFILES))))
-include $$($1_OFILES:.o=.d)

$$($1_MIDDIR)/generated/%.o:$$($1_MIDDIR)/generated/%.c;$$(call PRECMD,$1) $$($1_CC) -o$$@ $$<
$$($1_MIDDIR)/%.o:src/%.c;$$(call PRECMD,$1) $$($1_CC) -o$$@ $$<
ifneq (,$(strip $($1_CXX)))
  $$($1_MIDDIR)/%.o:$($1_MIDDIR)/%.cpp;$$(call PRECMD,$1) $$($1_CXX) -o$$@ $$<
  $$($1_MIDDIR)/%.o:src/%.cpp;$$(call PRECMD,$1) $$($1_CXX) -o$$@ $$<
endif
ifneq (,$(strip $($1_AS)))
  $$($1_MIDDIR)/%.o:$$($1_MIDDIR)/%.S;$$(call PRECMD,$1) $$($1_AS) -o$$@ $$<
  $$($1_MIDDIR)/%.o:src/%.S;$$(call PRECMD,$1) $$($1_AS) -o$$@ $$<
endif
ifneq (,$(strip $($1_OBJC)))
  $$($1_MIDDIR)/%.o:$$($1_MIDDIR)/%.m;$$(call PRECMD,$1) $$($1_OBJC) -o$$@ $$<
  $$($1_MIDDIR)/%.o:src/%.m;$$(call PRECMD,$1) $$($1_OBJC) -o$$@ $$<
endif

ifneq (,$(strip $($1_EXE)))
  $1-all:$($1_EXE)
  $($1_EXE):$$($1_OFILES);$$(call PRECMD,$1) $$($1_LD) -o$$@ $$^ $$($1_LDPOST)
endif

endef

#----------------------------------------------------------
# Data rules, for packing every resource into a single archive.
#  $1 target
#  $2 archive

define SINGLE_DATA_ARCHIVE

MKDATA_SOURCES:=$(filter src/tool/mkdata/% src/tool/common/%,$(SRCFILES))
$1_DATA_IN:=$(filter-out src/data/chalk/%,$(filter src/data/%,$(SRCFILES)))
$1_DATA_MID:=$$(patsubst src/data/%,$($1_MIDDIR)/data/%,$$($1_DATA_IN))

$2:$$($1_DATA_MID) $$(MKDATA_SOURCES);$$(call PRECMD,$1) $(NODE) src/tool/mkdata/main.js --archive -o$$@ $$($1_DATA_MID)

# This approach is messy and decidedly imperfect.
# But it does at least seem to achieve my immediate goal, editing the synth tools without having to rebuild all the maps and sprites and such.
TOOL_COMMON:=$(filter src/tool/common/%,$(SRCFILES)) src/tool/mkdata/main.js src/tool/mkdata/copyVerbatim.js

$($1_MIDDIR)/data/instrument/WebAudio \
  :src/data/instrument/WebAudio $$(TOOL_COMMON) src/tool/mkdata/readInstruments.js \
  ;echo "  $1 WebAudio" ; mkdir -p $$(@D) ; $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<

$($1_MIDDIR)/data/instrument/minsyn \
  :src/data/instrument/minsyn $$(TOOL_COMMON) src/tool/mkdata/minsyn.js \
  ;echo "  $1 minsyn" ; mkdir -p $$(@D) ; $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<

$($1_MIDDIR)/data/instrument/stdsyn \
  :src/data/instrument/stdsyn $$(TOOL_COMMON) src/tool/mkdata/stdsyn.js \
  ;echo "  $1 stdsyn" ; mkdir -p $$(@D) ; $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<
  
$($1_MIDDIR)/data/map/% \
  :src/data/map/% $$(TOOL_COMMON) src/tool/mkdata/processMap.js \
  ;echo "  $1 map/$$*" ; mkdir -p $$(@D) ; $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<
  
$($1_MIDDIR)/data/sprite/% \
  :src/data/sprite/% $$(TOOL_COMMON) src/tool/mkdata/processSprite.js \
  ;echo "  $1 sprite/$$*" ; mkdir -p $$(@D) ; $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<
  
$($1_MIDDIR)/data/tileprops/% \
  :src/data/tileprops/% $$(TOOL_COMMON) src/tool/mkdata/processTileprops.js \
  ;echo "  $1 tileprops/$$*" ; mkdir -p $$(@D) ; $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<
  
$($1_MIDDIR)/data/% \
  :src/data/% $$(TOOL_COMMON) \
  ;echo "  $1 $$*" ; mkdir -p $$(@D) ; $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<

endef
