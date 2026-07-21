#!/usr/bin/env bash

# This script downloads the wxWidgets-3.2.7.tar.bz2 source archive file from
# https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.7/wxWidgets-3.2.7.tar.bz2
# , extracts it into subfolder "wxWidgets-3.2.7",
# then builds wxWidgets into subfolder "wxWidgets-3.2.7/build-msw",
# and finally installs wxWidgets into folder "/usr/local/wx3.2.7-msw",
# 
# Note: on a normal Linux installation, you will need to "sudo" in order to write to folder
# /usr/local ; however, Cygwin uses the Windows folder permissons model; so if you
# installed Cygwin into C:\cygwin (as recommended), then you can write to any folder under "/",
# since it'd really the Windows folder C:\cygwin, which has no special Windows permissions.

# These are the expected paths for the mingw Cygwin package binaries.
[[ -z "$AR"      ]] && export AR=/bin/x86_64-w64-mingw32-ar.exe
[[ -z "$AS"      ]] && export AS=/bin/x86_64-w64-mingw32-as.exe
[[ -z "$CPP"     ]] && export CPP=/bin/x86_64-w64-mingw32-cpp.exe
[[ -z "$DLLTOOL" ]] && export DLLTOOL=/bin/x86_64-w64-mingw32-dlltool.exe
[[ -z "$DLLWRAP" ]] && export DLLWRAP=/bin/x86_64-w64-mingw32-dllwrap.exe
[[ -z "$CXX"     ]] && export CXX=/bin/x86_64-w64-mingw32-g++.exe
[[ -z "$CC"      ]] && export CC=/bin/x86_64-w64-mingw32-gcc.exe
[[ -z "$RANLIB"  ]] && export RANLIB=/bin/x86_64-w64-mingw32-ranlib.exe
[[ -z "$GCOV"    ]] && export GCOV=/bin/x86_64-w64-mingw32-gcov.exe
[[ -z "$GPROF"   ]] && export GPROF=/bin/x86_64-w64-mingw32-gprof.exe
[[ -z "$LD"      ]] && export LD=/bin/x86_64-w64-mingw32-ld.exe
[[ -z "$NM"      ]] && export NM=/bin/x86_64-w64-mingw32-nm.exe
[[ -z "$OBJCOPY" ]] && export OBJCOPY=/bin/x86_64-w64-mingw32-objcopy.exe
[[ -z "$OBJDUMP" ]] && export OBJDUMP=/bin/x86_64-w64-mingw32-objdump.exe
[[ -z "$STRIP"   ]] && export STRIP=/bin/x86_64-w64-mingw32-strip.exe
[[ -z "$WINDMC"  ]] && export WINDMC=/bin/x86_64-w64-mingw32-windmc.exe
[[ -z "$WINDRES" ]] && export WINDRES=/bin/x86_64-w64-mingw32-windres.exe

for VER in 3.2.7; do
#for VER in 3.0.2 3.0.4 3.1.0 3.1.1 3.1.2 3.1.3 3.1.4 3.1.5 3.2.7; do
  export TYPE=msw
  export VER

  if [[ ! -d wxWidgets-${VER} ]]; then
     curl -L https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 \
          -o wxWidgets-${VER}.tar.bz2|| \
     wget https://github.com/wxWidgets/wxWidgets/releases/download/v${VER}/wxWidgets-${VER}.tar.bz2 || exit 2
     tar xjvf wxWidgets-${VER}.tar.bz2 || exit 3
  fi

  cd wxWidgets-${VER} || exit 1

  mkdir build-${TYPE}
  cd    build-${TYPE}

  export CFLAGS="-fPIC" CXXFLAGS="-fPIC"
  ../configure --with-msw -enable-unicode --disable-debug --disable-shared --without-expat  --disable-richtext \
               --with-libpng=builtin --with-libjpeg=builtin --with-libtiff=builtin --with-libxpm=builtin \
               --prefix=/usr/local/wx${VER}-${TYPE} \
  	     && make -j $( nproc ) && make -j $( nproc ) install || exit 2

  rm /usr/local/bin/wx-config 2> /dev/null
  ln -s /usr/local/wx${VER}-${TYPE}/bin/wx-config /usr/local/bin/wx-config

  # Test the installation, it should print e.g. "Default config is msw-unicode-static-3.2"
  /usr/local/bin/wx-config --list
done
