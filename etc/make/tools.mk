# tools.mk
# Rules for building programs that might be used during the rest of the build.
# Initially, all the build-time tools were written for Node and do not require building, but I'm getting away from that.

tools_CCOPT:=-Werror -Wimplicit -Wno-parentheses -Wno-format-overflow
tools_CC:=gcc -c -MMD -O3 -Isrc $(tools_CCOPT) $(tools_CC_EXTRA)
tools_LD:=gcc
tools_LDPOST:=-lz -lm

tools_MIDDIR:=mid/tools
tools_OUTDIR:=out/tools

tools_OPT_PATTERN:=$(foreach U, \
  datafile png assist pcmprint http, \
  src/opt/$U/%.c \
)

tools_CFILES:=$(filter-out src/opt/assist/assist_main.c, \
  $(filter src/tool/%.c $(tools_OPT_PATTERN),$(SRCFILES)) \
)
tools_OFILES:=$(patsubst src/%,$(tools_MIDDIR)/%.o,$(basename $(tools_CFILES)))
-include $(tools_OFILES:.o=.d)

tools_NAMES:=$(filter-out common,$(sort $(foreach F,$(filter src/tool/%,$(tools_CFILES)),$(word 3,$(subst /, ,$F)))))

$(tools_MIDDIR)/%.o:src/%.c;$(PRECMD) $(tools_CC) -o$@ $<

define tools_EXERULE
  tools_EXE_$1:=$(tools_OUTDIR)/$1
  tools-all:$$(tools_EXE_$1)
  tools_OFILES_$1:=$$(filter $(tools_MIDDIR)/tool/$1/%.o $(tools_MIDDIR)/tool/common/%.o $(tools_MIDDIR)/opt/%,$(tools_OFILES))
  $$(tools_EXE_$1):$$(tools_OFILES_$1);$$(PRECMD) $(tools_LD) -o$$@ $$(tools_OFILES_$1) $(tools_LDPOST)
endef
$(foreach T,$(tools_NAMES),$(eval $(call tools_EXERULE,$T)))

tools-all:
all:tools-all
