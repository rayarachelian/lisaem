#!/bin/bash -x

VER=3.1.2
export TYPE=gtk
cd wxWidgets-${VER} || exit 1

mkdir build-${TYPE}
cd    build-${TYPE}

export CFLAGS="-fPIC" CXXFLAGS="-fPIC"
../configure --enable-unicode --disable-debug --disable-shared --without-expat  --disable-richtext \
             --with-libxpm=builtin  --prefix=/usr/local/wx${VER}-${TYPE} \
	     && make && sudo make install || exit 2
export PATH=/usr/local/wx${VER}-${TYPE}/bin/:$PATH
wx-config --list

