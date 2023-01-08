all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $(@F)" ; mkdir -p $(@D) ;

ifeq ($(MAKECMDGOALS),clean)
  clean:;rm -rf mid out
else

include etc/config.mk
etc/config.mk:etc/config.mk.example;$(PRECMD) cp $< $@
  
SRCFILES:=$(shell find src -type f)
MIDDIR:=mid
OUTDIR:=out

#TODO Not --export-all. Be more selective about it.
WASM_LDOPT:=-nostdlib -Xlinker --no-entry -Xlinker --import-undefined -Xlinker --export-all
WASM_CCOPT:=-c -MMD -O3 -nostdlib
WASM_CCINC:=-Isrc -I$(MIDDIR)
WASM_CCWARN:=-Werror -Wimplicit -Wno-parentheses
WASM_CC:=$(WASI_SDK)/bin/clang $(WASM_CCOPT) $(WASM_CCDEF) $(WASM_CCINC) $(WASM_CCWARN)
WASM_LD:=$(WASI_SDK)/bin/clang $(WASM_LDOPT)
WASM_LDPOST:=

#TODO Generated C files for embedded data.
CFILES:=$(filter %.c %.cpp %.m %.S,$(SRCFILES))
OFILES:=$(patsubst src/%,$(MIDDIR)/%.o,$(basename $(CFILES)))
-include $(OFILES:.o=.d)

$(MIDDIR)/%.o:src/%.c;$(PRECMD) $(WASM_CC) -o $@ $<

WASM_EXE:=$(OUTDIR)/fullmoon.wasm
all:$(WASM_EXE)
$(WASM_EXE):$(OFILES);$(PRECMD) $(WASM_LD) -o $@ $^ $(WASM_LDPOST)

#TODO Validate and minify web app.
#TODO Preprocess included data, eg pack or reformat spritesheets.
HTDOCS_SRC:=$(filter src/www/%,$(SRCFILES))
HTDOCS_DST:=$(patsubst src/www/%,$(OUTDIR)/%,$(HTDOCS_SRC))
all:$(HTDOCS_DST)
$(OUTDIR)/%:src/www/%;$(PRECMD) cp $< $@

MAPS_DATA:=$(OUTDIR)/maps.data
all:$(MAPS_DATA)
MAPS_INPUT:=$(filter src/data/map/%,$(SRCFILES))
$(MAPS_DATA):$(MAPS_INPUT) src/tool/mkmaps/main.js;$(PRECMD) $(NODE) src/tool/mkmaps/main.js -o$@ $(MAPS_INPUT)

# "make run-final" to pack the web app and serve it statically.
ifeq (,$(strip $(HTTP_SERVER_CMD)))
  run-final:;echo "Please set HTTP_SERVER_CMD in etc/config.mk" ; exit 1
else
  run-final:$(WASM_EXE) $(HTDOCS_DST);$(HTTP_SERVER_CMD) $(OUTDIR)
endif

# "make run" for a dynamic server preferred for dev work.
# Static files serve straight off the source. If you change any Javascript, just refresh.
# The Wasm file, we rerun make before serving, and if it fails, we send the make output instead with a 555 status.
run:$(WASM_EXE);$(NODE) src/tool/server/main.js --htdocs=src/www --makeable=$(WASM_EXE) --makeable=$(MAPS_DATA)

endif
