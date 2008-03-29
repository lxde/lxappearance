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
TMPDIR=`mktemp -d /tmp/icon-theme-XXXXXXX`

# decompress
if [ `basename -- $1 .tar.gz` != $1 ]
then
        tar -C $TMPDIR -xzf $1
elif [ `basename -- $1 .tar.bz2` != $1 ]
then
        tar -C $TMPDIR -xjf $1
fi

# create destdir
if [ -z "$XDG_DATA_HOME" ]
then
    export XDG_DATA_HOME="$HOME/.local/share";
fi
THEME_DIR="$XDG_DATA_HOME/icons"
mkdir -p "$THEME_DIR"

# install
cd $TMPDIR
DLIST=`find -name 'index.theme' -exec dirname {} \;`
for DIR in $DLIST
do
    basename "$DIR"
    mv -f "$DIR" "$THEME_DIR"
done

# clear temp dir
rm -rf $TMPDIR
