#!/bin/bash

#----------------------------------------------------------
# add-interim-build-links-to-all-dirs.sh
# A Part of the bashbuild system and LisaEm
#
# Copyright (C) 2019 Ray Arachelian, All Rights Reserved
# Released under the terms of the GNU GPL 3.0
#
# This script populates all empty dirs with a link to the
# interim build script which just goes up a dir and runs
# the build command there.
#
# It also serves another purpose which is to allow empty
# directories to be commited to git without needing a
# README or .gitignore. (Assuming git will behave with
# links that is)
#
# Only works upto 10 levels deep.
#
#----------------------------------------------------------
cd ..
if [[ ! -x bashbuild/interim-build.sh ]]; then
   echo "bashbuild/interim-build.sh not found" 1>&2
   echo "Run this with CWD inside the scripts directory" 1>&2
   exit 1
fi

find . -type d | while read i; do 
  if [[ -f "$i/build.sh" ]]; then
     echo "$i/build.sh found"
  else
     UP=""
     for l in 1 2 3 4 5 6 7 8 9 10;do
     export UP="${UP}../"
     pushd "$i" >/dev/null
     if [[ -f $UP/bashbuild/interim-build.sh ]]; then
	     ln -s ${UP}bashbuild/interim-build.sh build.sh
	     echo ${UP}bashbuild/interim-build.sh build.sh $i
     fi
     popd >/dev/null
     done
  fi
done
