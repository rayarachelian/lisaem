#!/bin/bash

OSVER="macOS-$( sw_vers -productVersion | cut -f1,2 -d'.' )"
VER=3.1.2
MIN="$OSVER"

if [[ ! -d wxWidgets-${VER} ]]; then
   curl -L https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 \
        -o wxWidgets-${VER}.tar.bz2|| \
   wget https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 || exit 2
   tar xjvf wxWidgets-${VER}.tar.bz2 || exit 3
fi

# use clang here
export CC=gcc CXX=gcc

CC --version
sleep 2

pushd wxWidgets-${VER}
TYPE=cocoa-x64-${OSVER}-cpp
set arch_flags="-m64 -arch x86_64"
rm -rf build-${TYPE}
mkdir  build-${TYPE}
cd     build-${TYPE}

# thanks to https://forums.wxwidgets.org/viewtopic.php?t=43760
# --with-macosx-version-min=${MIN}
../configure --enable-unicode --with-cocoa CXXFLAGS="-std=c++0x -stdlib=libc++" CPPFLAGS="-stdlib=libc++" LIBS=-lc++ \
             --disable-richtext  --disable-debug --disable-shared --without-expat  \
             --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --with-libxpm=builtin --with-zlib=builtin \
             --prefix=/usr/local/wx${VER}-${TYPE} && make && sudo make install || exit $?
popd

    
pushd wxWidgets-${VER}
TYPE=cocoa-i386-${OSVER}-cpp
set arch_flags="-m32 -arch i386"
rm -rf build-${TYPE}
mkdir  build-${TYPE}
cd     build-${TYPE}

../configure --enable-monolithic --enable-unicode --with-cocoa CXXFLAGS="-std=c++0x -stdlib=libc++" CPPFLAGS="-stdlib=libc++" LIBS=-lc++ \
             --disable-richtext  --disable-debug --disable-shared --without-expat  \
             --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --with-libxpm=builtin --with-zlib=builtin \
             --prefix=/usr/local/wx${VER}-${TYPE} && make && sudo make install || exit $?
popd
