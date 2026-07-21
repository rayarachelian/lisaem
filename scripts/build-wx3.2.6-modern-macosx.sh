#!/usr/bin/env bash

# usage: build-wx3.2.6-modern-macosx.sh {options} 
#
# options -m|--without-minimum-macos|--no-minimum-macos - disable passing minimum macos to wxWidgets build.
#

export OSVER="macOS-$(A=$(sw_vers -productVersion); MAJOR=$(echo ${A} | cut -f1 -d '.'); MID=$(echo ${A} | cut -f2 -d'.'); printf "%02d.%02d" ${MAJOR} ${MID} )"

#(jamesdenton) workaround for issue #11 - macOS > 15.0 deprecates CGDisplayCreateImage used by wxWidgets
export MIN_MACOSX_VERSION="14.5"
#export MIN_MACOSX_VERSION="$(  xcodebuild -showsdks 2>/dev/null | grep macosx | cut -d'-' -f2 | sed -e 's/sdk macosx//g' | sort -n | head -1 )"

[[ -z "$MIN_MACOSX_VERSION" ]] && export MIN_MACOSX_VERSION="$( basename $(ls -1d $(xcode-select -p )/SDKs/* | grep -i macosx10 | sort -n | head -1 ) | sed -e 's/.sdk$//g' -e 's/[Mm]ac[Oo][Ss][Xx]//g' )"
[[ -z "$MIN_MACOSX_VERSION" ]] && export MIN_MACOSX_VERSION="$OSVER"

OSVER="macOS-$( sw_vers -productVersion | cut -f1,2 -d'.' )"

echo "MIN_MACOSX_VERSION: $MIN_MACOSX_VERSION"

# note that 10.8 will only build 3.0.4 and 3.0.2

for VER in 3.2.6; do

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
TYPE=cocoa-${OSVER}-clang-sdl

rm -rf build-${TYPE}
mkdir  build-${TYPE}
cd     build-${TYPE}

# default to build for both 64 bit and 32 bit x86 for older OS's
XLIBS='LIBS="-lstdc++.6 -L /usr/lib"
CPUS="x86_64,i386"
CCXXFLAGS='CXXFLAGS="-std=c++0x"

[[ "$OSVER" < "macOS-10.09" ]] && export XLIBS="" CCXXFLAGS=""

# macos 10.14-10.15 build only x86_64, 11.0+ build for both x86_64 and arm64.
[[ "$OSVER" > "macOS-10.14"  ]] && export CPUS="x86_64"       XLIBS=""
[[ "$OSVER" > "macOS-10.99"  ]] && export CPUS="x86_64,arm64" XLIBS="" # CCXXFLAGS="-std=c++0x"

MINIMUMMACOS="--with-macosx-version-min=$MIN_MACOSX_VERSION"

[[ "$1" == "--without-minimum-macos" ]]  && MINIMUMMACOS=""
[[ "$1" == "--no-minimum-macos"      ]]  && MINIMUMMACOS=""
[[ "$1" == "-m"                      ]]  && MINIMUMMACOS=""

TYPE=cocoa-${OSVER}-${CPUS}
../configure --enable-monolithic --enable-unicode --with-cocoa ${CCXXFLAGS} ${XLIBS} \
             --enable-universal-binary=${CPUS} \
             "${MINIMUMMACOS}" \
             --disable-richtext  --disable-debug --disable-shared --without-expat  \
             --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --with-libxpm=builtin --with-zlib=builtin \
             --with-sdl \
	     --with-libiconv-prefix=/usr \
             --prefix=/usr/local/wx${VER}-${TYPE} && make -j $( sysctl -n hw.ncpu ) && sudo make -j $( sysctl -n hw.ncpu ) install 
popd

done
