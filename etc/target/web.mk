# web.mk
# Rules for building the WebAssembly app, and its Javascript wrapper.

web_MIDDIR:=mid/web
web_OUTDIR:=out/web

web_OPT_ENABLE:=web_glue

web_EXPORTED_SYMBOLS:=fmn_global fmn_init fmn_update fmn_render fmn_language_changed

# --allow-undefined replaced by --import-undefined around WASI 10, but my MacBook can't go above 8.
web_LDOPT:=-nostdlib -Xlinker --no-entry -Xlinker --export-table \
   $(foreach S,$(web_EXPORTED_SYMBOLS),-Xlinker --export=$S) \
  $(if $(findstring -8.0,$(web_WASI_SDK)), \
    -Xlinker --allow-undefined, \
    -Xlinker --import-undefined \
  )
   
web_CCOPT:=-c -MMD -O3 -nostdlib
web_CCINC:=-Isrc -I$(web_MIDDIR) $(web_CCINC_EXTRA)
web_CCWARN:=-Werror -Wimplicit -Wno-parentheses -Wno-comment
web_CC:=$(web_WASI_SDK)/bin/clang $(web_CCOPT) $(web_CCDEF) $(web_CCINC) $(web_CCWARN)
web_LD:=$(web_WASI_SDK)/bin/clang $(web_LDOPT)
web_LDPOST:=

#---------------------------------------------------------
# Static web app, final assembly.

web_HTSA_DIR:=$(web_OUTDIR)/fmweb
web_HTSA_HTML:=$(web_HTSA_DIR)/index.html
web_HTSA_WRAPPER:=$(web_HTSA_DIR)/wrapper.html
web_HTSA_JS:=$(web_HTSA_DIR)/fullmoon.js
web_HTSA_DATA_FULL:=$(web_HTSA_DIR)/fullmoon-full.data
web_HTSA_DATA_DEMO:=$(web_HTSA_DIR)/fullmoon-demo.data
web_HTSA_EXE:=$(web_HTSA_DIR)/fullmoon.wasm
web_HTSA_FMCHROME:=$(web_HTSA_DIR)/fmchrome.png
web_HTSA_FILES:=$(web_HTSA_HTML) $(web_HTSA_WRAPPER) $(web_HTSA_JS) $(web_HTSA_DATA_FULL) $(web_HTSA_DATA_DEMO) $(web_HTSA_EXE) $(web_HTSA_FMCHROME)
web-all:$(web_HTSA_FILES)

web_EXE:=$(web_HTSA_EXE)

# We want a single archive file containing all the data resources.
# It's a ton of work, all implemented in etc/make/per-target.mk
web_QFILTER:=instrument:WebAudio sound:WebAudio
$(eval $(call SINGLE_DATA_ARCHIVE,web,$(web_HTSA_DATA_FULL),$(web_HTSA_DATA_DEMO),$(web_QFILTER)))

# HTML, CSS, and ICON pack into one HTML file for output.
# This will be done slightly different standalone vs embedded.
$(web_HTSA_HTML):src/www/index.html src/www/favicon.ico src/www/fullmoon.css $(filter src/tool/indexhtml/%,$(SRCFILES)); \
  $(call PRECMD,web) $(NODE) src/tool/indexhtml/main.js -o$@ $< \
  --target=standalone \
  --favicon=src/www/favicon.ico \
  --css=src/www/fullmoon.css
  
# Wrapper and chrome get copied verbatim.
$(web_HTSA_WRAPPER):src/www/wrapper.html;$(PRECMD) cp $< $@
$(web_HTSA_FMCHROME):src/www/fmchrome.png;$(PRECMD) cp $< $@

# We emit one Javascript file for all the Javascript inputs.
# TODO Wiser to use some established tool for this. Is Webpack doable?
web_SRC_JS:=$(filter src/www/%.js,$(SRCFILES))
$(web_HTSA_JS):$(web_SRC_JS);$(call PRECMD,web) $(NODE) src/tool/packjs/main.js -o$@ $(web_SRC_JS)

# Final packaging.
web_HTSA_ARCHIVE_FULL:=$(web_OUTDIR)/fullmoon-full.tar.gz
web_HTSA_ARCHIVE_DEMO:=$(web_OUTDIR)/fullmoon-demo.tar.gz
web_HTSA_ZIP_FULL:=$(web_OUTDIR)/fullmoon-full.zip
web_HTSA_ZIP_DEMO:=$(web_OUTDIR)/fullmoon-demo.zip
web-all:$(web_HTSA_ARCHIVE_FULL) $(web_HTSA_ARCHIVE_DEMO) $(web_HTSA_ZIP_FULL) $(web_HTSA_ZIP_DEMO)

define web_TARBALL
  $1:$2;$$(call PRECMD,web) \
    rm -rf $(web_MIDDIR)/fullmoon ; \
    mkdir -p $(web_MIDDIR)/fullmoon || exit 1 ; \
    cp $2 $(web_MIDDIR)/fullmoon || exit 1 ; \
    cd $(web_MIDDIR) ; \
    mv fullmoon/fullmoon-full.data fullmoon/fullmoon.data 2>/dev/null || mv fullmoon/fullmoon-demo.data fullmoon/fullmoon.data ; \
    tar -czf fullmoon.tar.gz fullmoon || exit 1 ; \
    cd ../.. ; \
    rm -rf $(web_MIDDIR)/fullmoon ; \
    mv $(web_MIDDIR)/fullmoon.tar.gz $1
endef

# To comply with Itch.io, our Zip files have no top-level directory.
# Makes me feel dirty.
define web_ZIP
  $1:$2;$$(call PRECMD,web) \
    rm -rf $(web_MIDDIR)/zipasm ; \
    mkdir -p $(web_MIDDIR)/zipasm || exit 1 ; \
    cp $2 $(web_MIDDIR)/zipasm || exit 1 ; \
    cd $(web_MIDDIR)/zipasm ; \
    mv fullmoon-full.data fullmoon.data 2>/dev/null || mv fullmoon-demo.data fullmoon.data ; \
    zip -qr fullmoon.zip * || exit 1 ; \
    cd ../../.. ; \
    mv $(web_MIDDIR)/zipasm/fullmoon.zip $1 ; \
    rm -rf $(web_MIDDIR)/zipasm
endef

$(eval $(call web_TARBALL,$(web_HTSA_ARCHIVE_FULL),$(filter-out %-demo.data,$(web_HTSA_FILES))))
$(eval $(call web_TARBALL,$(web_HTSA_ARCHIVE_DEMO),$(filter-out %-full.data,$(web_HTSA_FILES))))
$(eval $(call web_ZIP,$(web_HTSA_ZIP_FULL),$(filter-out %-demo.data,$(web_HTSA_FILES))))
$(eval $(call web_ZIP,$(web_HTSA_ZIP_DEMO),$(filter-out %-full.data,$(web_HTSA_FILES))))

# Getting a little weird... We need to serve "fullmoon.data", but that file won't exist anymore before the final packaging.
web_HTSA_DATA:=$(web_HTSA_DIR)/fullmoon.data
$(web_HTSA_DATA):$(web_HTSA_DIR)/fullmoon-full.data;$(PRECMD) cp $< $@

#-----------------------------------------------------------
# Info site for aksommerville.com

web_INFO_DIR:=$(web_OUTDIR)/info
web_INFO_HTML:=$(web_INFO_DIR)/index.html
web_INFO_IFRAME:=$(web_INFO_DIR)/iframe-placeholder.html
web_INFO_CSS:=$(web_INFO_DIR)/fminfo.css
web_INFO_JS:=$(web_INFO_DIR)/bootstrap.js
web_INFO_FAVICON:=$(web_INFO_DIR)/favicon.ico
web_INFO_GAME_HTML:=$(web_INFO_DIR)/game.html
web_INFO_GAME_EXE:=$(web_INFO_DIR)/fullmoon.wasm
web_INFO_GAME_JS:=$(web_INFO_DIR)/fullmoon.js
web_INFO_GAME_DATA:=$(web_INFO_DIR)/fullmoon.data
web_INFO_GAME_FMCHROME:=$(web_INFO_DIR)/fmchrome.png
web_INFO_IMAGES:=$(patsubst src/info/img/%,$(web_INFO_DIR)/img/%,$(filter src/info/img/%,$(SRCFILES)))
web_INFO_FILES:=$(web_INFO_HTML) $(web_INFO_IFRAME) $(web_INFO_CSS) $(web_INFO_JS) $(web_INFO_FAVICON) $(web_INFO_IMAGES) \
  $(web_INFO_GAME_HTML) $(web_INFO_GAME_EXE) $(web_INFO_GAME_JS) $(web_INFO_GAME_DATA) $(web_INFO_GAME_FMCHROME)
 
web-all:$(web_INFO_FILES)

$(web_INFO_HTML):src/info/index.html;$(PRECMD) cp $< $@
$(web_INFO_IFRAME):src/info/iframe-placeholder.html;$(PRECMD) cp $< $@
$(web_INFO_CSS):src/info/fminfo.css;$(PRECMD) cp $< $@
$(web_INFO_FAVICON):src/info/favicon.ico;$(PRECMD) cp $< $@
$(web_INFO_GAME_HTML):$(web_HTSA_HTML);$(PRECMD) cp $< $@
$(web_INFO_GAME_EXE):$(web_HTSA_EXE);$(PRECMD) cp $< $@
$(web_INFO_GAME_JS):$(web_HTSA_JS);$(PRECMD) cp $< $@
$(web_INFO_GAME_DATA):$(web_HTSA_DATA);$(PRECMD) cp $< $@
$(web_INFO_GAME_FMCHROME):$(web_HTSA_FMCHROME);$(PRECMD) cp $< $@
$(web_INFO_DIR)/img/%:src/info/img/%;$(PRECMD) cp $< $@

web_INFO_SRC_JS:=$(filter src/info/%.js,$(SRCFILES))
$(web_INFO_JS):$(web_INFO_SRC_JS);$(call PRECMD,web) $(NODE) src/tool/packjs/main.js -o$@ $(web_INFO_SRC_JS)

web_INFO_ARCHIVE:=$(web_OUTDIR)/fullmoon-info.tar.gz
web-all:$(web_INFO_ARCHIVE)
$(web_INFO_ARCHIVE):$(web_INFO_FILES);$(PRECMD) cd $(web_OUTDIR) ; tar -czf fullmoon-info.tar.gz info

#-----------------------------------------------------------
# Commands.

# This is what you want during development. You can edit C, JS and data files, and just refresh the browser to pick up the changes.
web-run:$(web_EXE) $(web_HTSA_DATA);$(NODE) src/tool/server/main.js \
  --htdocs=src/www --makeable=$(web_HTSA_DATA) --makeable=$(web_EXE)

# Same as web-run, but on INADDR_ANY, so other hosts on your network can reach it (eg mobile).
web-run-routable:$(web_EXE) $(web_HTSA_DATA);$(NODE) src/tool/server/main.js \
  --htdocs=src/www --makeable=$(web_HTSA_DATA) --makeable=$(web_EXE) --host=0.0.0.0
  
# Simulate aksommerville.com locally.
web-run-info:$(web_HTSA_HTML) $(web_HTSA_JS) $(web_HTSA_EXE) $(web_HTSA_DATA) $(web_HTSA_FMCHROME); \
  $(NODE) src/tool/server/main.js --htdocs=src/info \
  --makeable=$(web_HTSA_HTML):/game.html \
  --makeable=$(web_HTSA_JS) \
  --makeable=$(web_HTSA_EXE) \
  --makeable=$(web_HTSA_DATA) \
  --makeable=$(web_INFO_JS) \
  --makeable=$(web_HTSA_FMCHROME)

# Run our maps editor.
web-edit:;$(NODE) src/tool/editor/main.js --htdocs=src/tool/editor/www --data=src/data
  
# Serve locally as if it's prod, ie pack everything. Not much need for this, after you've seen it work once.
web-run-prod:$(web_HTSA_FILES);$(NODE) src/tool/server/main.js --htdocs=$(web_HTSA_DIR) \
  --makeable=$(web_HTSA_HTML) \
  --makeable=$(web_HTSA_JS) \
  --makeable=$(web_HTSA_EXE) \
  --makeable=$(web_HTSA_DATA) \
  --makeable=$(web_HTSA_WRAPPER)

# Optionally deploy straight to a remote server.
ifeq (,$(findstring --,-$(web_DEPLOY_USER)-$(web_DEPLOY_HOST)-$(web_DEPLOY_PATH)-))
  web-deploy:$(web_INFO_ARCHIVE); \
    ssh $(web_DEPLOY_USER)@$(web_DEPLOY_HOST) "cd $(web_DEPLOY_PATH) && cat - >fullmoon.tar.gz && tar -xzf fullmoon.tar.gz && rm -fr fullmoon && mv info fullmoon" <$(web_INFO_ARCHIVE)
else
  web-deploy:;echo "Must set: web_DEPLOY_USER web_DEPLOY_HOST web_DEPLOY_PATH" ; exit 1
endif

# Special app for audio work.
# Runs a Node server that serves a web app to play audio just like the game, and receive MIDI input via WebSocket.
# Designed to attach to my MIDI sequencer Midevil: https://github.com/aksommerville/midevil
web-fiddle:;$(NODE) src/tool/server/main.js \
  --htdocs=src/tool/fiddle/www --midi-broadcast --port=43215 --htalias=/js/:src/www/js/ --makeable=$(web_HTSA_DATA)
  
web-verify:$(web_HTSA_DATA_FULL) $(web_HTSA_DATA_DEMO) $(tools_EXE_datan);$(tools_EXE_datan) --src=src/data --archive=$(web_HTSA_DATA_FULL) --archive=$(web_HTSA_DATA_DEMO)
