#!/usr/bin/env bash

VER=3.1.2
GCCVER=gcc730

if [[ -z "$(which wget)" ]]; then
   echo "Couldn't find wget, please install or check PATH" 1>&2
   exit 1
fi

if [[ -z "$(which 7za)" ]]; then
   echo "Couldn't find 7za, please install or check PATH" 1>&2
   exit 1
fi

if [[ ! -f wxMSW-${VER}_${GCCVER}_ReleaseDLL.7z ]] || [[ ! -f wxMSW-${VER}_${GCCVER}_x64_ReleaseDLL.7z  ]] \
   || [[ ! -f wxWidgets-${VER}-headers.7z ]]; then 
   wget https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxMSW-${VER}_${GCCVER}_ReleaseDLL.7z \
        https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxMSW-${VER}_${GCCVER}_x64_ReleaseDLL.7z \
        https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}-headers.7z
fi

rm -rf wxmsw
mkdir -pm755 wxmsw
cd wxmsw
7za x -y  ../wxMSW-${VER}_${GCCVER}_ReleaseDLL.7z    || exit 1
7za x -y ../wxMSW-${VER}_${GCCVER}_x64_ReleaseDLL.7z || exit 1
7za x -y  ../wxWidgets-${VER}-headers.7z || exit 1
cd include/wx; ln -s msw/setup.h
