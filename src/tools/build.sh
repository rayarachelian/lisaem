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
if  [[ -x "${TLD}/bashbuild/src.build" ]]; then
    is_bashbuild_loaded 2>/dev/null || source ${TLD}/bashbuild/src.build
else
    echo "$PWD/$0 Cannot find $PWD/bashbuild/src.build" 1>&2
    echo "This is required as it contains the shared code for the unified build system scripts." 1>&2
    exit 1
fi

###########################################################################################
# if you're making your own package using the src.build system, you'll want to set
# these variables.  These will help pupulate the appropriate fields in packages.
###########################################################################################
   SOFTWARE="dc42 tools"                   # name of the software (can contain upper case)
     LCNAME="dc42 tools"                   # lower case name used for the directory
DESCRIPTION="dc42 tools for Apple Lisa"    # description of the package
        VER="0.9.6"                        # just the version number
  STABILITY="RELEASE"                      # DEVELOP,ALPHA, BETA, RC1, RC2, RC3... RELEASE
RELEASEDATE="2019.11.24"                   # release date.  must be YYYY.MM.DD
     AUTHOR="Ray Arachelian"               # name of the author
  AUTHEMAIL="ray@arachelian.com"           # email address for this software
    COMPANY="sunder.net"                   # company (vendor for sun pkg)
      CONAM="SUNDERNET"                    # company short name for Solaris pkgs
        URL="https://lisaem.sunder.net"    # url to website of package
# ----------------------------------------------------------------------------------------
# vars auto built from the above.
VERSION="${VER}-${STABILITY}_${RELEASEDATE}"
BUILDDIR="${LCNAME}-${VER}"             # this should match the base directory name

# copy the arguements given to us as they'll be used again when we make packages
BUILDARGS="$0 $@"
export VER STABILITY RELEASEDATE AUTHOR SOFTWARE LCNAME DESCRIPTION COMPANY CONAM URL VERSION BUILDDIR BUILDARGS 

#------------------------------------------------------------------------------------------#
# end of standard section for all build scripts.
#------------------------------------------------------------------------------------------#


WITHDEBUG=""             # -g for debugging, -p for profiling. -pg for both

#STATIC=--static
WITHOPTIMIZE="-O2 -ffast-math -fomit-frame-pointer"
WITHUNICODE=""


# turn this on by default.
for i in $@; do  [[ "$i" == "--no-banner" ]] && NOBANNER=1; done
if [[ -z "$NOBANNER" ]]; then
   image ${XTLD}/resources/libdc42-banner.png || (

      echo ' _____________ --------------------------------------------------------------'
      echo "| |DC42    |.|   dc42 tools ${VERSION}  -   Unified Build Script"
      echo '| |        | |                                                        '
      echo '| |________| |              http://lisaem.sunder.net'
      echo '|   ______   | Copyright (C) 2008 Ray Arachelian, All Rights Reserved'
      echo '|  |    | |  |Released under the terms of the GNU General Public License 2.0'
      echo '\__|____|_|__| --------------------------------------------------------------'
   )
fi

CHECKFILES libdc42-gpl-license.txt src/lisadiskinfo.c src/lisafsh-tool.c src/patchxenix.c src/dumper.c src/dc42-to-raw.c src/raw-to-dc42.c
create_machine_h

# Parse command line options if any, overriding defaults.

for i in $@; do

 case "$i" in
  clean)
            echo "* Removing fsh tools objs and bins"
            CLEANARTIFACTS  "*.a" "*.o" "*.dylib" "*.so" .last-opts last-opts machine.h "*.exe" get-uintX-types*
            cd ..

            #if we said clean install or clean build, then do not quit
            Z="`echo $@ | grep -i install``echo $@ | grep -i build`"
            [[ -z "$Z" ]] && exit 0

  ;;
 build*)    echo ;;    #default - nothing to do here, this is the default.
 install)
            [[ -z "$CYGWIN" ]] && [[ "`whoami`" != "root" ]] && echo "Need to be root to install. try sudo ./build.sh $@" && exit 1
            INSTALL=1;
            ;;

 uninstall)
           if [[ -n "$DARWIN" ]]; then
             echo Uninstall commands not yet implemented.
             exit 1
           fi

           if [[ -n "$CYGWIN" ]]; then
              [[ -n "$PREFIX" ]]    && echo Deleting $PREFIX    && rm -rf $PREFIX
              [[ -n "$PREFIXLIB" ]] && echo Deleting $PREFIXLIB && rm -rf $PREFIXLIB
              exit 0
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
#DEBUG DEBUGMEMCALLS IPC_COMMENTS

  -64|--64)
                               export SIXTYFOURBITS="--64"; 
                               export THIRTYTWOBITS="";
                               export ARCH="-m64"                        ;;

  -32|--32)
                               export SIXTYFOURBITS=""; 
                               export THIRTYTWOBITS="--32"; 
                               export ARCH="-m32"                        ;;

 --without-debug)              WITHDEBUG=""
                               WARNINGS=""                               ;;

 --with-debug)                 WITHDEBUG="$WITHDEBUG -g"
                               WARNINGS="-Wall"                          ;;


 --with-profile)               WITHDEBUG="$WITHDEBUG -p"                 ;;

 --with-tracelog)              WITHTRACE="-DDEBUG -DTRACE"
                               WARNINGS="-Wall"                          ;;
 --no-banner)                  NOBANNER="1";                             ;;
 *)                            UNKNOWNOPT="$UNKNOWNOPT $i"               ;;
 esac

done


if [[ -n "$UNKNOWNOPT" ]]; then
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
--no-banner               Suppress version/copyright banner

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

MACHINE="`uname -mrsv`"
[[ "$MACHINE"   != "$LASTMACHINE" ]] && needclean=1
#debug and tracelog changes affect the whole project, so need to clean it all
[[ "$WITHTRACE" != "$LASTTRACE" ]] && needclean=1
[[ "$WITHDEBUG" != "$LASTDEBUG" ]] && needclean=1
# display mode changes affect only the main executable.

if [[ "$needclean" -gt 0 ]]; then
   rm -f .last-opts last-opts
   cd bin         && /bin/rm -f *
   cd ../obj      && /bin/rm -f *.a *.o
   cd ..
fi

echo "LASTDEBUG=\"$WITHDEBUG\""  >>.last-opts
echo "LASTMACHINE=\"$MACHINE\""  >>.last-opts

###########################################################################

# :TODO: un-silence warnings for demos and fix code

CFLAGS="$CFLAGS   -Wno-empty-body  -Wno-duplicate-decl-specifier  -Wno-incompatible-pointer-types  -Wno-implicit-function-declaration \
                  -Wno-parentheses  -Wno-format -Wno-implicit-function-declaration \
                  -Wno-unused-parameter  -Wno-unused "

echo Building lisa disk utilities...
echo

# Check to see if libdc42 exists locally, or globally
WHICHLIBDC42="`ls ../lib/libdc42/lib/libdc42.*.a 2>/dev/null`"
if [[ -z "$WHICHLIBDC42" ]]; then
   [[ -f "/usr/local/lib/libdc42.*.a" ]] && WHICHLIBDC42="/usr/local/lib/libdc42.*.a" && DC42INCLUDE="/usr/local/include/libdc42.h"
   [[ -f "/usr/lib/libdc42.*.a" ]]       && WHICHLIBDC42="/usr/lib/libdc42.*.a" && DC42INCLUDE="/usr/include/libdc42.h"
else
   DC42INCLUDE="../../lib/libdc42/include"
fi

# if not found, build it by calling subbuild
if [[ -n "$WHICHLIBDC42" ]]; then
   WHICHLIBDC42="../$WHICHLIBDC42"
else
   subbuild src/lib/libdc42      --no-banner             $SIXTYFOURBITS
   WHICHLIBDC42="`ls src/lib/libdc42/lib/libdc42.*.a 2>/dev/null`"
   if [[ ! -f "$WHICHLIBDC42" ]]; then exit 1; fi
   DC42INCLUDE="../../lib/libdc42/include"
   WHICHLIBDC42="../$WHICHLIBDC42"
fi

create_machine_h

cd src

export COMPILECOMMAND="$CC $CLICMD -o :OUTFILE: -W $WARNINGS -Wstrict-prototypes $WITHDEBUG $WITHTRACE $ARCH $CFLAGS -I $DC42INCLUDE $INC -Wno-format -Wno-unused :INFILE:.c $WHICHLIBDC42"
LIST1=$(WAIT="yes" OBJDIR="../bin/" INEXT=c OUTEXT="${EXTTYPE}" VERB=Compiling COMPILELIST lisadiskinfo  lisafsh-tool   dumper raw-to-dc42 dc42-to-raw patchxenix )

###########################################################################

if [[ -n "$INSTALL" ]]; then
      cd ../bin/
      [[ -n "$DARWIN" ]] && PREFIX=/usr/local/bin  # these shouldn't go into /Applications
      echo Installing tools to $PREFIX/bin
      mkdir -pm755 $PREFIX/bin 2>/dev/null
      # * is for .exe on windows
      for i in lisadiskinfo*  lisafsh-tool*  patchxenix*  raw-to-dc42* dc42-to-raw*; do
         cp ${i}* $PREFIX/bin/ || exit 1
      done
      cd ..
fi
echo
[ -z "$NOBANNER" ] && echo "Disk Tools build done."
true