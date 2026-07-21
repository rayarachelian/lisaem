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
   SOFTWARE="termwx"                       # name of the software (can contain upper case)
     LCNAME="termwx"                       # lower case name used for the directory
DESCRIPTION="TermWX"                       # description of the package
        VER="1.0"                          # the version number
RELEASEDATE="2019.11.24"                   # release date.  must be YYYY.MM.DD
     AUTHOR="Jeremy Salwen"                # name of the author
  AUTHEMAIL="jeremysalwen"                 # email address for this software
    COMPANY="jeremysalwen"                 # company (vendor for sun pkg)
      CONAM="TERMINALWX"                   # company short name for Solaris pkgs
        URL="http://jeremysalwen.github.com/TerminalWx/"    # url to website of package

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
   image ${XTLD}/resources/libTerminalWx.png || (
      echo ' _____________ --------------------------------------------------------------'
      echo "||          ||       TerminalWx ${VERSION}  -   Unified Build Script"
      echo '||TerminalWx||                                                                 '
      echo '||__________||                                                                 '
      echo '|            | Written by Jeremy Salwen, Mark Erikson, Derry Bryson, Tim Miller'
      echo '|============|  Released under the terms of the LGPL 2+/wxWindows License 3.1'
      echo '|/##########\\|          http://jeremysalwen.github.com/TerminalWx/'
      echo '               ---------------------------------------------------------------'
)
fi

[[ -d src ]] && mkdir obj lib 2>/dev/null
CHECKDIRS src obj lib
CHECKFILES src/GTerm/actions.cpp src/GTerm/gterm.cpp src/GTerm/keytrans.cpp src/GTerm/states.cpp src/GTerm/utils.cpp src/GTerm/vt52_states.cpp src/taTelnet/wxterm.cpp src/include/wxterm.h

# Parse command line options if any, overriding defaults.
#echo parsing options

#echo "$@" 1>&2
#echo "WARNINGS: $WARNINGS" 1>&2
#echo "CFLAGS: $CFLAGS" 1>&2
#echo "CPPFLAGS: $CPPFLAGS" 1>&2
#echo "CXXFLAGS: $CXXFLAGS" 1>&2

for i in $@
do
 i=`echo "$i" | sed -e 's/without/no/g' -e 's/disable/no/g' -e 's/enable-//g' -e 's/with-//g'`

 case "$i" in
  estimate)   cd ${XTLD}/src  # estimate compile count, we only have one file, but have to build library so return 2 units
              COMPILED=""
              if needed taTelnet/wxterm.cpp ../obj/wxterm.o || needed terminalwx.cpp ../lib/terminalwx.a; then
                 echo 8
              else
                 echo 0
              fi
              return 2>/dev/null >/dev/null #incase we're called directly 
              exit 0
              ;;
  clean)
            echo "* Removing terminalwx objs"
            CLEANARTIFACTS  "*.a" "*.o" "*.dylib" "*.so" .last-opts last-opts machine.h "*.exe" get-uintX-types
            cd ..

            #if we said clean install or clean build, then do not quit
            Z="`echo $@ | grep -i install``echo $@ | grep -i build`"
            [[ -z "$Z" ]] && return 2>/dev/null >/dev/null
            [[ -z "$Z" ]] && exit 0
	    export WARNINGS="-w" # 2022.03.27 - disable warnings for this proj
            export CFLAGS="$CFLAGS -w"
            export CPPFLAGS="$CPPFLAGS -w -Wno-effc++"
            export CXXFLAGS="$CXXFLAGS -w -Wno-effc++"
  ;;
 build*)    
	    export WARNINGS="-w" # 2022.03.27 - disable warnings for this proj
            export CFLAGS="$CFLAGS -w"
            export CPPFLAGS="$CPPFLAGS -w -Wno-effc++"
            export CXXFLAGS="$CXXFLAGS -w -Wno-effc++"
	    ;;
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
                               export ARCH="-m64"; export SARCH="-m64" ;;

  -32|--32|-m32)
                               export SIXTYFOURBITS=""; 
                               export THIRTYTWOBITS="--32"; 
                               [[ "$MACHINE" == "x86_64" ]] && export MACHINE="i386"
                               export ARCH="-m32"; export SARCH="-m32" ;;

 -march=*)                     export ARCH="${i} $ARCH"                ;;
 -arch=*)                      export ARCH="$(echo ${i} | sed -e 's/=/ /g') $ARCH"
                               export SARCH="$i $SARCH"                ;;

 --no-debug)                   export WITHDEBUG=""
                               export WARNINGS=""                      ;;
 --profile)
                               export WITHDEBUG="$WITHDEBUG -p"
                               export LIBGENOPTS="$LIBGENOPTS --with-profile"
                               export WITHPROFILE="yes"                ;;

 --debug)                      WITHDEBUG="$WITHDEBUG -g"
                               export WARNINGS="-g -DDEBUG" ;;

 --no-banner)                  NOBANNER="1"
                               export WARNINGS="-w" # 2022.03.27 - disable warnings for this proj
                               export CFLAGS="$CFLAGS -w"
                               export CPPFLAGS="$CPPFLAGS -w -Wno-effc++"
                               export CXXFLAGS="$CXXFLAGS -w -Wno-effc++"
                                                                       ;;
 *)                            UNKNOWNOPT="$UNKNOWNOPT $i"             ;;
 esac

done


#echo "$@" 1>&2
#echo "WARNINGS: $WARNINGS" 1>&2
#echo "CFLAGS: $CFLAGS" 1>&2
#echo "CPPFLAGS: $CPPFLAGS" 1>&2
#echo "CXXFLAGS: $CXXFLAGS" 1>&2



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
   cd ${XTLD}/obj       && /bin/rm -f *.a *.o
   cd ${XTLD}

fi

echo "LASTTRACE=\"$WITHTRACE\""  > .last-opts
echo "LASTDEBUG=\"$WITHDEBUG\""  >>.last-opts
echo "LASTBLITS=\"$WITHBLITS\""  >>.last-opts
echo "LASTMACHINE=\"$THISMACHINE\""  >>.last-opts


[[ -z "$PERCENTPROGRESS" ]] && export PERCENTPROGRESS=0 PERCENTCEILING=100 PERCENTJOB=0 NUMJOBSINPHASE=1 COMPILEPHASE="terminalwx"

###########################################################################

PHASETLIST="src/taTelnet/wxterm src/GTerm/actions src/GTerm/gterm src/GTerm/keytrans src/GTerm/states src/GTerm/utils src/GTerm/vt52_states src/terminalinputevent"

printf ' \r'
echo "* TerminalWX C++ Code          (./wxui)"

# save WARNINGS settings, add C++ extra warnings
OWARNINGS="$WARNINGS"
export WARNINGS="$WARNINGS -Weffc++ $NOIGNOREDQUALIFIERS $NODEPRECATED"

# Compile C++
cd "${XTLD}"
export COMPILEPHASE="C++ code"
export PERCENTPROGRESS=${PERCENTCEILING}

#echo "$PHASETLIST" 1>&2

export PERCENTCEILING=$(( ${PERCENTPROGRESS:-100} + ${ESTPHASETCOUNT:-0} ))
export PERCENTJOB=0 NUMJOBSINPHASE=6
CXXFLAGS="$CXXFLAGS -I src/include"
export COMPILECOMMAND="$CXX -W -Wno-write-strings $WARNINGS $WITHDEBUG $WITHTRACE $WITHBLITS $INC $CXXFLAGS $ARCH -c :INFILE:.cpp -o :OUTFILE:.o "

#echo "COMPILECOMMAND: $COMPILECOMMAND :::" 1>&2
#echo "  WAIT='' INEXT=cpp OUTEXT=o OBJDIR=obj VERB=Compiled COMPILELIST ${PHASETLIST} " 1>&2
#echo "==========================================================================================================================================" 1>&2
LIST=$( WAIT=yes INEXT=cpp OUTEXT=o OBJDIR=obj VERB=Compiled COMPILELIST ${PHASETLIST} )
#echo "==========================================================================================================================================" 1>&2
waitqall

COMPILED=""

cd ${XTLD}/obj

#(
#   echo "LIST: $LIST"
#   pwd 
#   ls -Flah
#   echo makelibs  ../lib terminalwx "${VERSION}" static "actions.o gterm.o keytrans.o states.o utils.o vt52_states.o wxterm.o" ) 1>&2

makelibs  ../lib terminalwx "${VERSION}" static "actions.o gterm.o keytrans.o states.o utils.o vt52_states.o wxterm.o terminalinputevent.o"

cd ..

###########################################################################

if [[ -n "$INSTALL" ]]; then
      # Needs to be run as "root" to succeed:
      if [[ $EUID -eq 0 ]]; then
          echo Installing TerminalWX lib $VERSION to $PREFIX/lib
          # echo "Current folder is $(pwd)"
          cd lib/
          mkdir -pm755 "$PREFIX/lib"
          cp terminalwx-$VERSION.a "$PREFIX/lib/"
          [ -n "$DARWIN" ] && cp terminalwx.${VERSION}.dylib "$PREFIX/lib/"
          cd "$PREFIX/lib"
          ln -s terminalwx-$VERSION.a terminalwx.a
      else
          echo "Skipping installation of TerminalWX lib: not running as root."
      fi
fi
echo
[[ -z "$NOBANNER" ]] && echo "terminalwx build done"
true
