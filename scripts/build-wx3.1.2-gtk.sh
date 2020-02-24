#!/usr/bin/env bash

# I use this on my machine to test against multiple versions of wxWidgets, for normal people
# 3.1.2 or eventually 3.1.4 should suffice.

for VER in 3.1.2; do
#for VER in 3.0.2 3.0.4 3.1.0 3.1.1 3.1.2 3.1.3; do
#^ swap these two to compile multiple versions for testing LisaEm against them vs just 3.1.2
  export VER

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
  ../configure --with-gtk --enable-unicode --disable-debug --disable-shared --without-expat  --disable-richtext  \
               --with-libpng=builtin --with-libjpeg=builtin --with-libtiff=builtin --with-libxpm=builtin \
               --prefix=/usr/local/wx${VER}-${TYPE} \
               --with-libxpm=builtin  --prefix=/usr/local/wx${VER}-${TYPE} \
	       --with-sdl \
	       && make -j $( nproc ) && $SUDO make -j $( nproc ) install || exit 2
 # 2020.02.12: 
 # ^ added --with-sdl because wxWidgets wants oss which is extinct as modern distros have moved to pulseaudio
 # and so wxSound now does not work, even with osspd-pulseaudio, however it will be able to use SDL for sound.
 # this requires that you install SDL. 
 # see: https://trac.wxwidgets.org/ticket/18000 and https://trac.wxwidgets.org/ticket/14899

 echo export PATH=/usr/local/wx${VER}-${TYPE}/bin/:$PATH
 /usr/local/wx${VER}-${TYPE}/bin/wx-config --list
done
