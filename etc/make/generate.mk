# generate.mk
# Rules for generated source files.

GENERATED_FILES:=chalk.c appicon.c

mid/%/generated/chalk.c:src/data/chalk/1 etc/tool/mkchalk.sh;$(PRECMD) etc/tool/mkchalk.sh $@ $<
mid/%/generated/appicon.c:src/opt/bigpc/appicon.png out/tools/mkdatac;$(PRECMD) $(tools_EXE_mkdatac) --appicon -o$@ $<
