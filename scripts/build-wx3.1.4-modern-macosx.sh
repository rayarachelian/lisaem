#!/usr/bin/env bash

OSVER="$( sw_vers -productVersion | cut -f1,2 -d'.' )"

export MIN_MACOSX_VERSION="$(  xcodebuild -showsdks 2>/dev/null | grep macosx10 | cut -d'-' -f2 | sed -e 's/sdk macosx//g' | sort -n | head -1 )"
[[ -z "$MIN_MACOSX_VERSION" ]] && export MIN_MACOSX_VERSION="$( basename $(ls -1d $(xcode-select -p )/SDKs/* | grep -i macosx10 | sort -n | head -1 ) | sed -e 's/.sdk$//g' -e 's/[Mm]ac[Oo][Ss][Xx]//g' )"
[[ -z "$MIN_MACOSX_VERSION" ]] && export MIN_MACOSX_VERSION="$OSVER"

OSVER="macOS-$( sw_vers -productVersion | cut -f1,2 -d'.' )"

echo "MIN_MACOSX_VERSION: $MIN_MACOSX_VERSION"

# note that 10.8 will only build 3.0.4 and 3.0.2

for VER in 3.1.4; do

if [[ ! -d "wxWidgets-${VER}" ]]; then

   [[ ! -f wxWidgets-${VER}.tar.bz2 ]] &&  \
      (  curl -L https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 \
          -o wxWidgets-${VER}.tar.bz2|| \
         wget https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 || exit 2 )

   tar xjvf wxWidgets-${VER}.tar.bz2 || exit 3

fi


if [[ "$VER" < "3.1.3" ]]; then
   [[ -n "$( grep xcode-4-5-tr1-type-traits-file-not-found wxWidgets-${VER}/include/wx/strvararg.h )" ]] || \
         patch ./wxWidgets-${VER}/include/wx/strvararg.h wx-strvararg.patch || exit $?
fi

pushd wxWidgets-${VER}
TYPE=cocoa-x86-${OSVER}-clang-sdl

rm -rf build-${TYPE}
mkdir  build-${TYPE}
cd     build-${TYPE}

XLIBS='LIBS="-lstdc++.6 -L /usr/lib"'
CPUS="x86_64,i386"
CCXXFLAGS='CXXFLAGS="-std=c++0x -stdlib=libc++"'
if [[ "$OSVER" > "macOS-10.14" ]]; then 
   CPUS="x86_64"
   XLIBS=""
fi

# cc1plus: error: unrecognized command line option "-std=c++0x"
# cc1plus: error: unrecognized command line option "-stdlib=libc++"

if [[ "$OSVER" < "macOS-10.9" ]]; then 
   XLIBS=""
   CCXXFLAGS=""
fi

# catalina: ld: library not found for -lstdc++.6
# ray@catalina scripts % find /usr -name "libstdc++.*" -ls 2>/dev/null 
# lrwxr-xr-x    1 root             wheel                  21 Nov  3 12:45 /usr/lib/libstdc++.6.dylib -> libstdc++.6.0.9.dylib
# -rwxr-xr-x    1 root             wheel              725280 Sep 29  2019 /usr/lib/libstdc++.6.0.9.dylib
# lrwxr-xr-x    1 root             wheel                  17 Nov  3 12:45 /usr/lib/libstdc++.dylib -> libstdc++.6.dylib

#../configure --enable-monolithic --enable-unicode --with-cocoa  LIBS=-lc++ CXXFLAGS="-std=c++0x -stdlib=libc++" CPPFLAGS="-stdlib=libc++" LIBS=-lc++ \
# vs LIBS="-lstdc++.6 -L /usr/lib"
../configure --enable-monolithic --enable-unicode --with-cocoa ${CCXXFLAGS} ${XLIBS} \
             --enable-universal-binary=${CPUS} \
             --with-macosx-version-min=$MIN_MACOSX_VERSION \
             --disable-richtext  --disable-debug --disable-shared --without-expat  \
             --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --with-libxpm=builtin --with-zlib=builtin \
             --with-sdl \
	     --with-libiconv-prefix=/usr \
             --prefix=/usr/local/wx${VER}-${TYPE} && make -j $( sysctl -n hw.ncpu ) && sudo make -j $( sysctl -n hw.ncpu ) install 
popd

done
