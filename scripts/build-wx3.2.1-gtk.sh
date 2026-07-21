#!/usr/bin/env bash

# I use this on my machine to test against multiple versions of wxWidgets, for normal people
# 3.1.5 is enough, I tend to build them all.
#
# for Fedora/CentOS/RHEL, need to first do:
#  dnf install gcc-g++ gtk3-devel gstreamer1-devel clutter-devel webkit2gtk3-devel libgda-devel gobject-introspection-devel
#


LIBTIFF="--with-libtiff=builtin"

function FreeBSDgetcpus()
{
  if [[ -z "$NUMCPUS" ]]; then
     export NUMCPUS="$(  sysctl hw.model hw.machine hw.ncpu | grep hw.ncpu: | cut -d: -f2 | sed -e's/ //g' )"
     [[ -z "$NUMCPUS" ]] && export NUMCPUS=1
     echo $NUMCPUS
  fi
}

if [[ "$(uname -s)" == "FreeBSD" ]]; then
   NUMCPUS=$( FreeBSDgetcpus )
   FREEBSD="yes"
   export DISABLEEPOLL="--disable-epollloop"
else
   if [[ -n "$(which nproc 2>/dev/null)" ]]; then NUMCPUS=$( nproc ); else NUMCPUS=1; fi
fi

[[ "$( uname -s )" == "SunOs" ]] && DISABLEMEDIACTL="--disable-mediactrl"


SDL2="$( find /usr -name libSDL2.a | head -1 )"
[[ -n "$SDL2" ]] && SDL2="--with-sdl=$SDL2"

if [[ "$( uname -s )"=="OpenBSD" ]]; then
   echo OpenBSD detected
   LIBTIFF="--with-libtiff=sys"
else
   LIBTIFF="$( find /usr -name 'libtiff.a' | head -1 )"
   if [[ -z "$LIBTIFF" ]]; then
      LIBTIFF="--with-libtiff=builtin"
   else
      # builtin doesn't compile on FreeBSD (tested only on fbsd13)
      [[ -z "$FREEBSD" ]] && LIBTIFF="--with-libtiff=$LIBTIFF" || LIBTIFF=""
   fi
fi


for VER in 3.2.1; do
#for VER in 3.0.2 3.0.4 3.1.0 3.1.1 3.1.2 3.1.3 3.1.4 3.1.5 3.2.1; do
#^ swap these two to compile multiple versions for testing LisaEm against them vs just 3.2.1
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

#	       --with-sdl \
  export CFLAGS="-fPIC" CXXFLAGS="-fPIC"
  ../configure --with-gtk --enable-unicode --disable-debug --disable-shared --without-expat  --disable-richtext  \
               --with-libpng=builtin --with-libjpeg=builtin $LIBTIFF --with-libxpm=builtin \
               --prefix=/usr/local/wx${VER}-${TYPE} $SDL2 $DISABLEEPOLL $DISABLEMEDIACTL \
               --with-libxpm=builtin  --prefix=/usr/local/wx${VER}-${TYPE} \
	       && make -j $NUMCPUS && $SUDO make -j $NUMCPUS install || exit 2
 # 2020.02.12: 
 # ^ added --with-sdl because wxWidgets wants oss which is extinct as modern distros have moved to pulseaudio
 # and so wxSound now does not work, even with osspd-pulseaudio, however it will be able to use SDL for sound.
 # this requires that you install SDL. 
 # see: https://trac.wxwidgets.org/ticket/18000 and https://trac.wxwidgets.org/ticket/14899

 echo export PATH=/usr/local/wx${VER}-${TYPE}/bin/:$PATH
 /usr/local/wx${VER}-${TYPE}/bin/wx-config --list
done
