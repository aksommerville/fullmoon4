#!/bin/sh

echo "mkpkg.sh..."
echo "TARGETS=$TARGETS"

#--------------------------------------------------------------
# Determine what to call this build.

if [ -z "$(git status -s)" ] ; then
  echo "wd clean"
else
  echo "wd dirty"
fi

single_target() {
  echo "package for $1..."
}

for T in $TARGETS ; do
  single_target $T
done

# Generate a build tag (whether we need it or not).
# git "vN.N.N" tag if present, or commit hash, or date if workspace dirty.
#BUILDTAG:=$(strip $(shell git status -s))
#ifeq (,$(BUILDTAG)) # Working directory is clean, look for a tag.
#  BUILDTAG:=$(strip $(shell git describe --tags | grep -E '^v[0-9]+\.[0-9]+\.[0-9]+$$'))
#  ifeq (,$(BUILDTAG)) # Current commit does not have a "vN.N.N" tag. Use commit hash instead.
#    BUILDTAG:=$(shell git rev-parse HEAD)
#  endif
#else # Working directory dirty. Don't use any git symbols, use the time instead.
#  BUILDTAG:=$(shell date +%Y%m%d-%H%M%S)
#endif
