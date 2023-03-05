#!/bin/sh

DSTPATH="$1"
SRCPATH="$2"

cat - >"$DSTPATH" <<EOF
const int fmn_chalk_glyphs[]={
EOF

sed -E 's/^([0-9]+) ([0-9]+)$/  \1,\2,/' <"$SRCPATH" >>"$DSTPATH"

cat - >>"$DSTPATH" <<EOF
  0,0,
};
EOF
