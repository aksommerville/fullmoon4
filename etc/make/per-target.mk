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
$1_SRCFILES:=$$(filter-out $$($1_SRCFILES_FILTER_OUT),$$(filter $$($1_SRCPAT),$(SRCFILES) $$(addprefix $$($1_MIDDIR)/generated/,$(GENERATED_FILES)))) \
  $$($1_SRCFILES_EXTRA)

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

# With these rules, any change to mkdatac (think like, adding a new synth command), will force rebuild of every resource.
# I think it's ok. Individual runs of mkdatac are so fast you can't even time them.

$2:$$($1_DATA_MID) $(tools_EXE_mkdatac);$$(call PRECMD,$1) $(tools_EXE_mkdatac) --archive -o$$@ $$($1_DATA_MID)
  
$($1_MIDDIR)/data/% \
  :src/data/% $(tools_EXE_mkdatac) \
  ;echo "  $1 $$*" ; mkdir -p $$(@D) ; $(tools_EXE_mkdatac) --single -o$$@ $$<

endef
