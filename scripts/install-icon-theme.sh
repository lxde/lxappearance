#!/bin/sh
# Script used to install new icon theme.
# Copyright (C) 2008 paulliu, pcman
# License: GNU GPL

# test argc
if (test $# -ne 1)
then
  echo Usage: $0 compressed_file
  exit
fi

# make temp dir
THEME_DIR=$HOME/.icons
mkdir -p $THEME_DIR
TMPDIR=`mktemp -d $THEME_DIR/XXXXXXX`

BASE=`basename $1`

# decompress
if [ `basename -- $BASE .tar.gz` != "$BASE" ]
then
        tar -C $TMPDIR -xzf $1
elif [ `basename -- $BASE .tar.bz2` != "$BASE" ]
then
        tar -C $TMPDIR -xjf $1
fi

# install
cd $TMPDIR
DLIST=`find -name 'index.theme' -exec dirname {} \;`
for DIR in $DLIST
do
    basename "$DIR"
    mv -f "$DIR" "$THEME_DIR"
done

cd ..
# clear temp dir
rm -rf $TMPDIR
