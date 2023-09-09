#!/bin/sh

#--------------------------------------------------------------
# Determine what to call this build.

if [ -z "$(git status -s)" ] ; then
  TAG="$(git describe --tags | grep -E '^v[0-9]+\.[0-9]+\.[0-9]+$')"
  if [ -n "$TAG" ] ; then # Current commit is a "vN.N.N" tag, perfect, use it.
    BUILDTAG=$TAG
  else # Use the commit hash.
    BUILDTAG=$(git rev-parse HEAD)
  fi
else # Dirty workspace, use a timestamp.
  BUILDTAG=$(date +%Y%m%d-%H%M%S)
fi

OUTDIR=pack/$BUILDTAG
MIDDIR=mid/pkg
HOMEDIR="$PWD"

mkdir -p $OUTDIR
mkdir -p $MIDDIR
rm -rf $MIDDIR/*

#-----------------------------------------------------------------

#TODO Eventually we'll want a README, maybe other errata?

linuxy() { # $1=TARGET $2=exe-suffix $3=archive-type("zip", or none for tar-gzip)
  mkdir $MIDDIR/fullmoon-$1-full
  cp out/$1/fullmoon$2 $MIDDIR/fullmoon-$1-full/fullmoon$2
  cp out/$1/data-full $MIDDIR/fullmoon-$1-full/data
  mkdir $MIDDIR/fullmoon-$1-demo
  cp out/$1/fullmoon$2 $MIDDIR/fullmoon-$1-demo/fullmoon$2
  cp out/$1/data-demo $MIDDIR/fullmoon-$1-demo/data

  for OPTFILE in icon.ico ; do # Add other optional errata here.
    if [ -f "out/$1/$OPTFILE" ] ; then
      cp out/$1/$OPTFILE $MIDDIR/fullmoon-$1-full/$OPTFILE
      cp out/$1/$OPTFILE $MIDDIR/fullmoon-$1-demo/$OPTFILE
    fi
  done
  
  cd $MIDDIR
  
  ARSFX=
  if [ "$3" = .zip ] ; then
    ARSFX=.zip
    if [ -n "$JDK" ] ; then
      "$JDK/bin/jar" -cMf fullmoon-$1-full-$BUILDTAG$ARSFX fullmoon-$1-full
      "$JDK/bin/jar" -cMf fullmoon-$1-demo-$BUILDTAG$ARSFX fullmoon-$1-demo
    else
      zip -r fullmoon-$1-full-$BUILDTAG$ARSFX fullmoon-$1-full
      zip -r fullmoon-$1-demo-$BUILDTAG$ARSFX fullmoon-$1-demo
    fi
  else
    ARSFX=.tar.gz
    tar -czf fullmoon-$1-full-$BUILDTAG$ARSFX fullmoon-$1-full
    tar -czf fullmoon-$1-demo-$BUILDTAG$ARSFX fullmoon-$1-demo
  fi
  
  cd "$HOMEDIR"
  mv $MIDDIR/fullmoon-$1-full-$BUILDTAG$ARSFX $OUTDIR
  mv $MIDDIR/fullmoon-$1-demo-$BUILDTAG$ARSFX $OUTDIR
}

macbundle() { # $1=TARGET(macos) $2=selection(full,demo)
  mkdir $MIDDIR/fullmoon-$1-$2
  if [ "$2" = "demo" ] ; then
    cp -r out/$1/FullMoonDemo.app $MIDDIR/fullmoon-$1-$2
  else
    cp -r out/$1/FullMoon.app $MIDDIR/fullmoon-$1-$2
  fi
  
  cd $MIDDIR
  zip -r fullmoon-$1-$2-$BUILDTAG.zip fullmoon-$1-$2
  cd "$HOMEDIR"
  mv $MIDDIR/fullmoon-$1-$2-$BUILDTAG.zip $OUTDIR
}

web() { # Everything is packaged correctly by make; just copy and rename.
  cp out/web/fullmoon-demo.tar.gz $OUTDIR/fullmoon-web-demo-$BUILDTAG.tar.gz
  cp out/web/fullmoon-full.tar.gz $OUTDIR/fullmoon-web-full-$BUILDTAG.tar.gz
  cp out/web/fullmoon-demo.zip $OUTDIR/fullmoon-web-demo-$BUILDTAG.zip
  cp out/web/fullmoon-full.zip $OUTDIR/fullmoon-web-full-$BUILDTAG.zip
  cp out/web/fullmoon-info.tar.gz $OUTDIR/fullmoon-web-info-$BUILDTAG.tar.gz
}

single_target() { # $1=TARGET
  case "$1" in
  
    assist) ;; # don't do anything for 'assist'
  
    generic) linuxy $1 ;;
    linux) linuxy $1 "" .zip ;;
    raspi) linuxy $1 ;;
    mswin) linuxy $1 .exe .zip ;;
  
    macos)
      macbundle $1 full
      macbundle $1 demo
    ;;
    
    web) web ;;
    
    *)
      echo "$0: Unknown target '$1'"
      exit 1
  esac
}

for T in $TARGETS ; do
  single_target $T
done

echo "Generated package '$BUILDTAG' for targets: $TARGETS"
