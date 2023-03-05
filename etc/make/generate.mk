# generate.mk
# Rules for generated source files.

GENERATED_FILES:=chalk.c

mid/%/generated/chalk.c:src/data/chalk/1 etc/tool/mkchalk.sh;$(PRECMD) etc/tool/mkchalk.sh $@ $<
