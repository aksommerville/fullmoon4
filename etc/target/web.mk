# web.mk
# Rules for building the WebAssembly app, and its Javascript wrapper.

web_MIDDIR:=mid/web
web_OUTDIR:=out/web

web_OPT_ENABLE:=web_glue

web_EXPORTED_SYMBOLS:=fmn_global fmn_init fmn_update __indirect_function_table

web_LDOPT:=-nostdlib -Xlinker --no-entry -Xlinker --import-undefined \
   $(foreach S,$(web_EXPORTED_SYMBOLS),-Xlinker --export=$S)
web_CCOPT:=-c -MMD -O3 -nostdlib
web_CCINC:=-Isrc -I$(web_MIDDIR)
web_CCWARN:=-Werror -Wimplicit -Wno-parentheses -Wno-comment
web_CC:=$(web_WASI_SDK)/bin/clang $(web_CCOPT) $(web_CCDEF) $(web_CCINC) $(web_CCWARN)
web_LD:=$(web_WASI_SDK)/bin/clang $(web_LDOPT)
web_LDPOST:=

#---------------------------------------------------------
# Static web app, final assembly.

web_HTPROD_DIR:=$(web_OUTDIR)/deploy
web_HTPROD_HTML:=$(web_HTPROD_DIR)/index.html
web_HTPROD_CSS:=$(web_HTPROD_DIR)/fullmoon.css
web_HTPROD_ICON:=$(web_HTPROD_DIR)/favicon.ico
web_HTPROD_JS:=$(web_HTPROD_DIR)/fullmoon.js
web_HTPROD_DATA:=$(web_HTPROD_DIR)/fullmoon.data
web_HTPROD_EXE:=$(web_HTPROD_DIR)/fullmoon.wasm
web_HTPROD_FILES:=$(web_HTPROD_HTML) $(web_HTPROD_CSS) $(web_HTPROD_ICON) $(web_HTPROD_JS) $(web_HTPROD_DATA) $(web_HTPROD_EXE)
web-all:$(web_HTPROD_FILES)

web_EXE:=$(web_HTPROD_EXE)

# We want a single archive file containing all the data resources.
# It's a ton of work, all implemented in etc/make/per-target.mk
$(eval $(call SINGLE_DATA_ARCHIVE,web,$(web_HTPROD_DATA)))

# HTML, CSS, and ICON are 1:1 from source files.
# TODO May eventually need build-time tweaking, minification, etc.
$(web_HTPROD_HTML):src/www/index.html;$(call PRECMD,web) $(NODE) src/tool/indexhtml/main.js -o$@ $<
$(web_HTPROD_CSS):src/www/fullmoon.css;$(call PRECMD,web) cp $< $@
$(web_HTPROD_ICON):src/www/favicon.ico;$(call PRECMD,web) cp $< $@

# We emit one Javascript file for all the Javascript inputs.
# TODO Wiser to use some established tool for this. Is Webpack doable?
web_SRC_JS:=$(filter src/www/%.js,$(SRCFILES))
$(web_HTPROD_JS):$(web_SRC_JS);$(call PRECMD,web) $(NODE) src/tool/packjs/main.js -o$@ $(web_SRC_JS)

# And pack the whole thing in a tarball for delivery.
web_HTPROD_ARCHIVE:=$(web_OUTDIR)/fullmoon-prod.tar.gz
web-all:$(web_HTPROD_ARCHIVE)
$(web_HTPROD_ARCHIVE):$(web_HTPROD_FILES);$(call PRECMD,web) tar -C$(web_OUTDIR) -czf $(web_HTPROD_ARCHIVE) $(notdir $(web_HTPROD_DIR))

#-----------------------------------------------------------
# Commands.

# This is what you want during development. You can edit JS and data files, and just refresh the browser to pick up the changes.
web-run:$(web_EXE) $(web_HTPROD_DATA);$(NODE) src/tool/server/main.js \
  --htdocs=src/www --makeable=$(web_HTPROD_DATA) --makeable=$(web_EXE)
  
# Serve locally as if it's prod, ie pack everything. Not much need for this, after you've seen it work once.
web-run-prod:$(web_HTPROD_FILES);$(NODE) src/tool/server/main.js --htdocs=$(web_HTPROD_DIR)

# Optionally deploy straight to a remote server.
ifeq (,$(findstring --,-$(web_DEPLOY_USER)-$(web_DEPLOY_HOST)-$(web_DEPLOY_PATH)-))
  web-deploy:$(web_HTPROD_ARCHIVE); \
     ssh $(web_DEPLOY_USER)@$(web_DEPLOY_HOST) "cd $(web_DEPLOY_PATH) && cat - >fullmoon.tar.gz && tar -xzf fullmoon.tar.gz && rm -rf fullmoon && mv deploy fullmoon" <$(web_HTPROD_ARCHIVE)
else
  web-deploy:;echo "Must set: web_DEPLOY_USER web_DEPLOY_HOST web_DEPLOY_PATH" ; exit 1
endif

