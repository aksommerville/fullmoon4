# web.mk
# Rules for building the WebAssembly app, and its Javascript wrapper.

web_MIDDIR:=mid/web
web_OUTDIR:=out/web

web_OPT_ENABLE:=web_glue

web_EXPORTED_SYMBOLS:=fmn_global fmn_init fmn_update fmn_render

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
web_HTSA_FILES:=$(web_HTSA_HTML) $(web_HTSA_WRAPPER) $(web_HTSA_JS) $(web_HTSA_DATA_FULL) $(web_HTSA_DATA_DEMO) $(web_HTSA_EXE)
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
  
# Wrapper gets copied verbatim.
$(web_HTSA_WRAPPER):src/www/wrapper.html;$(PRECMD) cp $< $@

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
$(web_HTSA_DATA):$(web_HTSA_DIR)/fullmoon-demo.data;$(PRECMD) cp $< $@

#-----------------------------------------------------------
# Commands.

# This is what you want during development. You can edit C, JS and data files, and just refresh the browser to pick up the changes.
web-run:$(web_EXE) $(web_HTSA_DATA);$(NODE) src/tool/server/main.js \
  --htdocs=src/www --makeable=$(web_HTSA_DATA) --makeable=$(web_EXE)

# Same as web-run, but on INADDR_ANY, so other hosts on your network can reach it (eg mobile).
web-run-routable:$(web_EXE) $(web_HTSA_DATA);$(NODE) src/tool/server/main.js \
  --htdocs=src/www --makeable=$(web_HTSA_DATA) --makeable=$(web_EXE) --host=0.0.0.0

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
  web-deploy:$(web_HTSA_ARCHIVE_DEMO); \
    ssh $(web_DEPLOY_USER)@$(web_DEPLOY_HOST) "cd $(web_DEPLOY_PATH) && cat - >fullmoon.tar.gz && tar -xzf fullmoon.tar.gz && rm -rf fullmoon && mv fmweb fullmoon" <$(web_HTSA_ARCHIVE_DEMO)
else
  web-deploy:;echo "Must set: web_DEPLOY_USER web_DEPLOY_HOST web_DEPLOY_PATH" ; exit 1
endif

# Special app for audio work.
# Runs a Node server that serves a web app to play audio just like the game, and receive MIDI input via WebSocket.
# Designed to attach to my MIDI sequencer Midevil: https://github.com/aksommerville/midevil
web-fiddle:;$(NODE) src/tool/server/main.js \
  --htdocs=src/tool/fiddle/www --midi-broadcast --port=43215 --htalias=/js/:src/www/js/ --makeable=$(web_HTSA_DATA)
  
web-verify:$(web_HTSA_DATA);$(NODE) src/tool/verify/main.js --archive=$(web_HTSA_DATA) --dir=src/data
