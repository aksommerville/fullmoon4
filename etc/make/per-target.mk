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

$1_SRCPAT:=src/app/% $(foreach U,$($1_OPT_ENABLE),src/opt/$U/%)
$1_SRCFILES:=$$(filter $$($1_SRCPAT),$(SRCFILES))

$1_CFILES:=$$(filter %.c %.m %.cpp %.S,$$($1_SRCFILES))
$1_OFILES:=$$(patsubst src/%,$($1_MIDDIR)/%.o,$$(basename $$($1_CFILES)))
-include $$($1_OFILES:.o=.d)

$$($1_MIDDIR)/%.o:src/%.c;$$(call PRECMD,$1) $$($1_CC) -o$$@ $$<
ifneq (,$(strip $($1_CXX)))
  $$($1_MIDDIR)/%.o:src/%.cpp;$$(call PRECMD,$1) $$($1_CXX) -o$$@ $$<
endif
ifneq (,$(strip $($1_AS)))
  $$($1_MIDDIR)/%.o:src/%.S;$$(call PRECMD,$1) $$($1_AS) -o$$@ $$<
endif
ifneq (,$(strip $($1_OBJC)))
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
$1_DATA_IN:=$(filter src/data/%,$(SRCFILES))
$1_DATA_MID:=$$(patsubst src/data/%,$($1_MIDDIR)/data/%,$$($1_DATA_IN))

$2:$$($1_DATA_MID) $$(MKDATA_SOURCES);$$(call PRECMD,$1) $(NODE) src/tool/mkdata/main.js --archive -o$$@ $$($1_DATA_MID)
$($1_MIDDIR)/data/%:src/data/% $$(MKDATA_SOURCES);$$(call PRECMD,$1) $(NODE) src/tool/mkdata/main.js --single -o$$@ $$<

endef
