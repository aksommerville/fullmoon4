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
web_HTSA_DATA:=$(web_HTSA_DIR)/fullmoon.data
web_HTSA_EXE:=$(web_HTSA_DIR)/fullmoon.wasm
web_HTSA_FILES:=$(web_HTSA_HTML) $(web_HTSA_WRAPPER) $(web_HTSA_JS) $(web_HTSA_DATA) $(web_HTSA_EXE)
web-all:$(web_HTSA_FILES)

web_EXE:=$(web_HTSA_EXE)

# We want a single archive file containing all the data resources.
# It's a ton of work, all implemented in etc/make/per-target.mk
$(eval $(call SINGLE_DATA_ARCHIVE,web,$(web_HTSA_DATA)))

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

# And pack the whole thing in a tarball for delivery.
web_HTSA_ARCHIVE:=$(web_OUTDIR)/fullmoon.tar.gz
web-all:$(web_HTSA_ARCHIVE)
$(web_HTSA_ARCHIVE):$(web_HTSA_FILES);$(call PRECMD,web) tar -C$(web_OUTDIR) -czf $(web_HTSA_ARCHIVE) $(notdir $(web_HTSA_DIR))

# ...and also pack the whole thing in a ZIP for delivery. Itch.io only does ZIP.
web_HTSA_ZIP:=$(web_OUTDIR)/fullmoon.zip
web-all:$(web_HTSA_ZIP)
$(web_HTSA_ZIP):$(web_HTSA_FILES);$(call PRECMD,web) \
  cd $(web_HTSA_DIR) && \
  zip -r tmp.zip * && \
  cd ../../.. && \
  mv $(web_HTSA_DIR)/tmp.zip $@

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
  web-deploy:$(web_HTSA_ARCHIVE); \
    ssh $(web_DEPLOY_USER)@$(web_DEPLOY_HOST) "cd $(web_DEPLOY_PATH) && cat - >fullmoon.tar.gz && tar -xzf fullmoon.tar.gz && rm -rf fullmoon && mv fmweb fullmoon" <$(web_HTSA_ARCHIVE)
else
  web-deploy:;echo "Must set: web_DEPLOY_USER web_DEPLOY_HOST web_DEPLOY_PATH" ; exit 1
endif

# Special app for audio work.
# Runs a Node server that serves a web app to play audio just like the game, and receive MIDI input via WebSocket.
# Designed to attach to my MIDI sequencer Midevil: https://github.com/aksommerville/midevil
web-fiddle:;$(NODE) src/tool/server/main.js \
  --htdocs=src/tool/fiddle/www --midi-broadcast --port=43215 --htalias=/js/:src/www/js/ --makeable=$(web_HTSA_DATA)
  
web-verify:$(web_HTSA_DATA);$(NODE) src/tool/verify/main.js --archive=$(web_HTSA_DATA) --dir=src/data
