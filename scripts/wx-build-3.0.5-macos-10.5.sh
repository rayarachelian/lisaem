#!/bin/bash

VER=3.0.5

if [[ ! -d "wxWidgets-${VER}" ]]; then

   [[ ! -f wxWidgets-${VER}.tar.bz2 ]] &&  \
      (  curl -L https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 \
          -o wxWidgets-${VER}.tar.bz2|| \
         wget https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 || exit 2 )

   tar xjvf wxWidgets-${VER}.tar.bz2 || exit 3

fi
for arch in i386 ppc ppc64 x86_64; do

#arch=i386,ppc,ppc64,x86_64
VER=3.0.5
TYPE=cocoa-10.5-$arch
pushd wxWidgets-$VER
rm    -rf    build-$TYPE
mkdir -pm755 build-$TYPE
cd           build-$TYPE

make clean

export LDFLAGS='-flat_namespace'

#             --with-macosx-sdk=/Developer3.1/SDKs/MacOSX10.4u.sdk \

../configure --with-osx-cocoa --disable-debug --disable-shared --disable-richtext \
             --enable-macosx_arch=${arch} \
             --with-sdl  \
             --with-macosx-sdk=/Developer/SDKs/MacOSX10.5.sdk \
             --with-macosx-version-min=10.4 \
             --with-libxpm=builtin  \
             --with-libjpeg=builtin \
             --with-libpng=builtin  \
             --with-regex=builtin   \
             --with-libtiff=builtin \
             --with-zlib=builtin    \
             --with-expat=builtin   \
             --prefix=/usr/local/wx-${VER}-${TYPE} && make && sudo make install &&
             cd / && sudo tar cjpvf $HOME/Downloads/usr.local.wx-${VER}-${TYPE}.tar.bz2 /usr/local/wx-${VER}-${TYPE} 
popd
done
exit $?

             #--enable-macosx_arch=ppc,i386,x86_64,ppc64 --enable-compat28 --enable-cmdline --enable-sound --enable-dnd --enable-accel \
../configure --with-osx-cocoa --disable-debug --disable-shared \
             --enable-macosx_arch=ppc,i386,x86_64,ppc64 \
             --without-expat --disable-richtext --with-macosx-version-min=10.4 \
             --enable-compat28 --enable-cmdline --enable-sound --enable-dnd --enable-accel --with-libpng=builtin \
             --disable-fswatcher --disable-stdpaths --with-libtiff=builtin  --with-libjpeg=builtin --with-libxpm=builtin    \
             --prefix=/usr/local/wx-${VER}-${TYPE} && make && sudo make install &&
             cd / && sudo tar cjpvf $HOME/Downloads/usr.local.wx-${VER}-${TYPE}.tar.bz2 /usr/local/wx-${VER}-${TYPE}             
