#!/usr/bin/env bash

#------------------------------------------------------------------------------------------#
# Standard block for each bashbuild script - these are used for copyright notices, packages
# this standard block ends around line 45
#------------------------------------------------------------------------------------------#

# ensure we're running from our directory and that we haven't been called from somewhere else
cd "$(dirname $0)"
[[ "$(basename $0)" != "build.sh" ]] && echo "$0 must be named build.sh" 1>&2 && exit 9

# As this is the top level script so we force remove any saved envs here before saving new
# ones, this next line can be optionally added/removed as needed.
find . -type f -name '.env-*' -exec rm -f {} \;
# Include and execute unified build library code - this part is critical

[[ -z "$TLD" ]] && export TLD="${PWD}"
# set the local top level directory for this build, as we go down into subdirs and run
# build, TLD will be the top, but XTLD will be the "local" build's TLD.
export XTLD="$( /bin/pwd )"

if  [[ -x "${TLD}/bashbuild/src.build" ]]; then
    is_bashbuild_loaded 2>/dev/null || source ${TLD}/bashbuild/src.build
else
    echo "$PWD/$0 Cannot find $PWD/bashbuild/src.build" 1>&2
    echo "This is required as it contains the shared code for the unified build system scripts." 1>&2
    exit 1
fi

###########################################################################################
# if you're making your own package using the src.build system, you'll want to set
# these variables.  These will help populate the appropriate fields in packages.
###########################################################################################
   SOFTWARE="libdc42"                      # name of the software (can contain upper case)
     LCNAME="libdc42"                      # lower case name used for the directory
DESCRIPTION="libdc42 disk image library"   # description of the package
        VER="0.9.6"                        # the version number
RELEASEDATE="2019.11.24"                   # release date.  must be YYYY.MM.DD
     AUTHOR="Ray Arachelian"               # name of the author
  AUTHEMAIL="ray@arachelian.com"           # email address for this software
    COMPANY="sunder.net"                   # company (vendor for sun pkg)
      CONAM="SUNDERNET"                    # company short name for Solaris pkgs
        URL="https://lisaem.sunder.net"    # url to website of package

# ----------------------------------------------------------------------------------------
# vars auto built from the above.
VERSION="${VER}_${RELEASEDATE}"
# copy the arguements given to us as they'll be used again when we make packages
BUILDARGS="$0 $@"
export VER RELEASEDATE AUTHOR SOFTWARE LCNAME DESCRIPTION COMPANY CONAM URL VERSION BUILDARGS 

#------------------------------------------------------------------------------------------#
# end of standard section for all build scripts.
#------------------------------------------------------------------------------------------#


# turn this on by default.

#### Edit these options for your system

WITHOPTIMIZE="-O2 -ffast-math -fomit-frame-pointer"

COMPILED=""



for i in $@; do [[ "$i" == "--no-banner" ]] && NOBANNER=1; done
if [[ -z "$NOBANNER" ]]
then
   image ${XTLD}/resources/libdc42-banner.png || (
      echo ' _____________ --------------------------------------------------------------'
      echo "| |libDC42 |.|   libdc42 ${VERSION}  -   Unified Build Script"
      echo '| |        | |                                                        '
      echo '| |________| |              http://lisaem.sunder.net'
      echo '|   ______   | Copyright (C) 2008 Ray Arachelian, All Rights Reserved'
      echo '|  |    | |  |Released under the terms of the GNU General Public License 2.0'
      echo '\__|____|_|__|      or the terms of the LGPL version 2.0 - your choice.    '
      echo '               ---------------------------------------------------------------'
)
fi

CHECKDIRS include lib obj resources src
CHECKFILES libdc42-lgpl-license.txt libdc42-gpl-license.txt include/libdc42.h src/libdc42.c src/lib_raw_profile_image.c resources/libdc42-banner.png

# Parse command line options if any, overriding defaults.
#echo parsing options
for i in $@
do
 i=`echo "$i" | sed -e 's/without/no/g' -e 's/disable/no/g' -e 's/enable-//g' -e 's/with-//g'`

 case "$i" in
  estimate)   cd ${XTLD}/src  # estimate compile count, we only have one file, but have to build library so return 2 units
              COMPILED=""
              if needed libdc42.c ../obj/libdc42.o || needed libdc42.c ../lib/libdc42.a; then
                 echo 2
              else
                 echo 0
              fi
              return 2>/dev/null >/dev/null #incase we're called directly 
              exit 0
              ;;
  clean)
            echo "* Removing libdc42 objs"
            CLEANARTIFACTS  "*.a" "*.o" "*.dylib" "*.so" .last-opts last-opts machine.h "*.exe" get-uintX-types
            cd ..

            #if we said clean install or clean build, then do not quit
            Z="`echo $@ | grep -i install``echo $@ | grep -i build`"
            [[ -z "$Z" ]] && return 2>/dev/null >/dev/null
            [[ -z "$Z" ]] && exit 0

  ;;
 build*)    echo ;;    #default - nothing to do here, this is the default.
 install)
            if [[ -z "$CYGWIN" ]]; then
               [[ "`whoami`" != "root" ]] && echo "Need to be root to install. try: sudo ./build.sh $@" && exit 1
            else
                CygwinSudoRebuild $@
            fi
            INSTALL=1
            ;;

 skipinstall)  INSTALL="" ;;    # skip install (used to disable $INSTALL passed from TLD builds)

-D*)        EXTRADEFINES="${EXTRADEFINES} ${i}";;

 uninstall)
           if [[ -n "$DARWIN" ]]; then
             echo Uninstall commands not implemented.
             exit 1
           fi

           if [[ -n "$CYGWIN" ]]; then
              echo Uninstall commands not implemented.
              exit 1
           fi

           #Linux, etc.

           #PREFIX="/usr/local/bin"
           #PREFIXLIB="/usr/local/share/"

           echo Uninstalling from $PREFIX and $PREFIXLIB
           rm -rf $PREFIXLIB/lisaem/
           rm -f  $PREFIX/lisaem
           rm -f  $PREFIX/lisafsh-tool
           rm -f  $PREFIX/lisadiskinfo
           rm -f  $PREFIX/patchxenix

           exit 0

    ;;

  -64|--64|-m64)
                               export SIXTYFOURBITS="--64"; 
                               export THIRTYTWOBITS="";
                               export ARCH="-m64"; export SARCH="-m64"  ;;

  -32|--32|-m32)
                               export SIXTYFOURBITS=""; 
                               export THIRTYTWOBITS="--32"; 
                               [[ "$MACHINE" == "x86_64" ]] && export MACHINE="i386"
                               export ARCH="-m32"; export SARCH="-m32" ;;

 -march=*)                     export ARCH="${i} $ARCH"                ;;
 -arch=*)                      export ARCH="$(echo ${i} | sed -e 's/=/ /g') $ARCH"
                               export SARCH="$i $SARCH"                ;;

 --no-debug)                   WITHDEBUG=""
                               WARNINGS=""                               ;;
 --profile)
                               export WITHDEBUG="$WITHDEBUG -p"
                               export LIBGENOPTS="$LIBGENOPTS --with-profile"
                               export WITHPROFILE="yes"                                 ;;

 --debug)                      WITHDEBUG="$WITHDEBUG -g"
                               WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG" ;;

 --no-banner)                  NOBANNER="1";                             ;;
 *)                            UNKNOWNOPT="$UNKNOWNOPT $i"               ;;
 esac

done


create_machine_h

if [ -n "$UNKNOWNOPT" ]
then
 echo
 echo "Unknown options $UNKNOWNOPT"
 cat <<ENDHELP

Commands:
 clean                    Removes all compiled objects, libs, executables
 build                    Compiles lisaem (default)
 clean build              Remove existing objects, compile everything cleanly
 install                  Not yet implemented on all platforms
 uninstall                Not yet implemented on all platforms

Debugging Options:
--without-debug           Disables debug and profiling
--with-debug              Enables symbol compilation

Other Options:
--without-optimize        Disables optimizations
--no-68kflag-optimize     Force flag calculation on every opcode
--no-banner               Suppress version/copyright banner

--64                      64 bit compile
--32                      32 bit compile

Environment Variables you can pass:

CC                        Path to C Compiler
CPP                       Path to C++ Compiler
WXDEV                     Cygwin Path to wxDev-CPP 6.10 (win32 only)
PREFIX                    Installation directory

i.e. CC=/usr/local/bin/gcc ./build.sh ...

ENDHELP
exit 1

fi

[[ -n "${WITHDEBUG}${WITHTRACE}" ]] && if [[ -n "$INSTALL" ]]; then
   echo "Warning, will not install since debug/profile/trace options are enabled"
   echo "as Install command is only for production builds."
   INSTALL=""
fi

###########################################################################

# Has the configuration changed since last time? if so we may need to do a clean build.
[[ -f .last-opts ]] && source .last-opts

needclean=0

THISMACHINE="`uname -mrsv`"
[[ "$THISMACHINE"   != "$LASTMACHINE" ]] && needclean=1
#debug and tracelog changes affect the whole project, so need to clean it all
[[ "$WITHTRACE" != "$LASTTRACE" ]] && needclean=1
[[ "$WITHDEBUG" != "$LASTDEBUG" ]] && needclean=1
# display mode changes affect only the main executable.

if [[ "$needclean" -gt 0 ]]; then
   rm -f .last-opts last-opts
   cd ${XTLD}/lib       && /bin/rm -f *.a *.o
   cd ${XTLD}/obj      && /bin/rm -f *.a *.o
   cd ${XTLD}

fi

echo "LASTTRACE=\"$WITHTRACE\""  > .last-opts
echo "LASTDEBUG=\"$WITHDEBUG\""  >>.last-opts
echo "LASTBLITS=\"$WITHBLITS\""  >>.last-opts
echo "LASTMACHINE=\"$THISMACHINE\""  >>.last-opts


[[ -z "$PERCENTPROGRESS" ]] && export PERCENTPROGRESS=0 PERCENTCEILING=100 PERCENTJOB=0 NUMJOBSINPHASE=1 COMPILEPHASE="libdc42"

###########################################################################
echo "* Building libdc42..."
echo

# :TODO: silence warnings via rewriting code that emits them
CFLAGS="$CFLAGS   $NOEMPTYBODY $NODUPEDECL $NOINCOMPATIBLEPTR \
                  -Wno-implicit-function-declaration -Wno-parentheses  -Wno-format -Wno-implicit-function-declaration  \
                  -Wno-unused-parameter  -Wno-unused $EXTRADEFINES"

# moved to use qjob so can properly capture warning output
cd src
COMPILED=""
if needed libdc42.c ../obj/libdc42.o || needed libdc42.c ../lib/libdc42.a; then
   qjob "!!  Compiled libdc42.c..." $CC -W $WARNINGS -Wstrict-prototypes $INC -Wno-format -Wno-unused  $WITHDEBUG $WITHTRACE $ARCH $CFLAGS -c libdc42.c -o ../obj/libdc42.o || exit 1
   waitqall
fi

if needed lib_raw_profile_image.c ../obj/lib_raw_profile_image.o || needed lib_raw_profile_image.c ../lib/libdc42.a; then
   qjob "!!  Compiled lib_raw_profile_image.c..." $CC -W $WARNINGS -Wstrict-prototypes $INC -Wno-format -Wno-unused  $WITHDEBUG $WITHTRACE $ARCH $CFLAGS -c lib_raw_profile_image.c -o ../obj/lib_raw_profile_image.o || exit 1
   waitqall
fi

if needed ../obj/libdc42.o ../lib/libdc42.a || needed ../obj/lib_raw_profile_image.o ../lib/libdc42.a; then
    echo "  Making libdc42.a library..." 1>&2
    makelibs  ../lib libdc42 "${VERSION}" static "../obj/libdc42.o ../obj/lib_raw_profile_image.o"
fi

cd ..

###########################################################################

if [[ -n "$INSTALL" ]]; then
      cd ${XTLD}/lib/ || (echo "from $(pwd)" 1>&2; exit 1;)
      echo Installing libGenerator $VERSION
      mkdir -pm755 "$PREFIX/lib"
      cp libdc42-$VERSION.a "$PREFIX/lib/"
      [ -n "$DARWIN" ] && cp libdc42.${VERSION}.dylib "$PREFIX/lib/"
      cd "$PREFIX/lib"
      ln -sf libdc42-$VERSION.a libdc42.a
fi
echo
[[ -z "$NOBANNER" ]] && echo "libdc42 build done"
true
