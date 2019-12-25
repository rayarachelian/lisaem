#!/bin/bash -x

export VER=3.1.2

if [[ ! -d wxWidgets-${VER} ]]; then
   curl -L https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 \
        -o wxWidgets-${VER}.tar.bz2|| \
   wget https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 || exit 2
   tar xjvf wxWidgets-${VER}.tar.bz2 || exit 3
fi

if [[ -n "$(uname -s | grep -i cygwin)" ]]; then  
   export SUDO=""
else
   export SUDO="sudo"
fi

export TYPE=gtk
cd wxWidgets-${VER} || exit 1

rm -rf build-${TYPE}
mkdir  build-${TYPE}
cd     build-${TYPE}

export CFLAGS="-fPIC" CXXFLAGS="-fPIC"
../configure --with-gtk --enable-unicode --disable-debug --disable-shared --without-expat  --disable-richtext  --disable-dynlib \
             --with-libpng=builtin --with-libjpeg=builtin --with-libtiff=builtin --with-libxpm=builtin \
             --prefix=/usr/local/wx${VER}-${TYPE} \
             --with-libxpm=builtin  --prefix=/usr/local/wx${VER}-${TYPE} \
	     && make && $SUDO make install || exit 2
export PATH=/usr/local/wx${VER}-${TYPE}/bin/:$PATH
wx-config --list

