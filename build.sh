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
     SOFTWARE="LisaEm"                    # name of the software (can contain upper case)
       LCNAME="lisaem"                    # lower case name used for the directory
  DESCRIPTION="The first fully functional Lisa Emulator™"   # description of the package
          VER="1.2.7"                     # just the version number
    STABILITY="BETA"                      # DEVELOP,ALPHA, BETA, RC1, RC2, RC3... RELEASE
  RELEASEDATE="2020.03.31"                # release date.  must be YYYY.MM.DD
       AUTHOR="Ray Arachelian"            # name of the author
    AUTHEMAIL="ray@arachelian.com"        # email address for this software
      COMPANY="Sunder.NET"                # company (vendor for sun pkg)
        CONAM="SUNDERNET"                 # company short name for Solaris pkgs
          URL="http://lisaem.sunder.net"  # url to website of package
COPYRIGHTYEAR="2020"
COPYRIGHTLINE="Copyright © ${COPYRIGHTYEAR} $AUTHOR, All Rights Reserved"
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

# you'll have to rewrite (or at least customize) the rest of this script in a similar manner for your own package

# ensure the sub-builds are executables
chmod 755 src/lib/libdc42/build.sh src/lib/libGenerator/build.sh src/tools/build.sh  2>/dev/null

#--------------------------------------------------------------------------------------------------------
# this was old way of version tracking, left here for historical reference as to release dates
#--------------------------------------------------------------------------------------------------------
#VERSION="1.2.7-BETA_2019.03.24"
#VERSION="1.2.7-BETA_2019.03.14"
#VERSION="1.2.7-BETA_2019.03.09"
#VERSION="1.2.7-BETA_2019.03.03"
#VERSION="1.2.7-BETA_2019.02.29"
#VERSION="1.2.7-BETA_2019.02.25"
#VERSION="1.2.7-BETA_2020.02.22"
#VERSION="1.2.7-ALPHA_2019.11.11"
#VERSION="1.2.7-ALPHA_2019.10.20"
#VERSION="1.2.7-ALPHA_2019.10.15"
#VERSION="1.2.7-ALPHA_2019.10.13"
#VERSION="1.2.7-ALPHA_2019.09.29"
#VERSION="1.2.6.1-RELEASE_2012.12.12"
##VERSION="1.3.0-DEVELOP_2010.05.12"##dead end
#VERSION="1.2.6-RELEASE_2007.12.12"
#VERSION="1.2.5-RELEASE_2007.11.25"
#VERSION="1.2.2-RELEASE_2007.11.11"
#VERSION="1.2.0-RELEASE_2007.09.23"
#VERSION="1.0.1-DEV_2007.08.13"
#VERSION="1.0.0-RELEASE_2007.07.07"
#VERSION="1.0.0-RC2_2007.06.27"
#--------------------------------------------------------------------------------------------------------

WITHDEBUG=""             # -g for debugging, -p for profiling. -pg for both
LIBGENOPTS=""            # passthrough to libGenerator
#STATIC=--static
WITHOPTIMIZE="-O2 -ffast-math -fomit-frame-pointer"
WITHUNICODE="--unicode=yes"

#if compiling for win32, edit WXDEV path to specify the
#location of wxDev-CPP 6.10 (as a cygwin, not windows path.)
#i.e. WXDEV=/cygdrive/c/wxDEV-Cpp
#if left empty, code below will try to locate it, so only set this
#if you've installed it in a different path than the default.

#WXDEV=""


#export DEBUGCOMPILE="yes"

function CLEAN() {
            # note we expect to be inside src at this point, and we'll make our way out as needed.
            echo "* Cleaning..." 1>&2
            cd "${TLD}"
            CLEANARTIFACTS "*.o" "*.a" "*.so" "*.dylib" "*.exe" get-uintX-types "cpu68k-?.c" def68k gen68k
            subbuild src/lib/libGenerator --no-banner clean
            subbuild src/lib/libdc42      --no-banner clean
            subbuild src/tools            --no-banner clean
            rm -rf bin/LisaEm.app bin/lisaem bin/*.dSYM # for macos x - this is a dir so CLEANARTIFACTS will not handle it properly
            rm -f /tmp/slot.*.sh*
            rm -f ./pkg/build/*; echo "Built packages go here"    >pkg/build/README
            cd "${TLD}/bin"; ln -sf ../bashbuild/interim-build.sh build.sh
            if [[ -n "`which ccache 2>/dev/null`" ]]; then echo -n "* "; ccache -c; fi
}

function create_unvars_c {

echo Creating unvars.c
cat <<END1 >motherboard/unvars.c
/**************************************************************************************\\
*                             Apple Lisa 2 Emulator                                    *
*                                                                                      *
*              The Lisa Emulator Project  $VERSION                    *
*                  Copyright (C) ${COPYRIGHTYEAR} $AUTHOR                                   *
*                            All Rights Reserved                                       *
*                                                                                      *
*                        Reset Global Variables .c file                                *
*                                                                                      *
*            This is a stub file - actual variables are in vars.h.                     *
*        (this is autogenerated by the build script. do not hand edit.)                *
\**************************************************************************************/

#define IN_UNVARS_C 1
// include all the includes we'll (might) need (and want)
#include <vars.h>

#define REASSIGN(  a , b  , c  )  {(b) = (c);}

void unvars(void)
{

#undef GLOBAL
#undef AGLOBAL
#undef ACGLOBAL

END1
egrep 'GLOBAL\(' ../include/vars.h | grep -v '*' | grep -v AGLOBAL | grep -v '#define' | grep -v FILE | grep -v get_exs | grep -v '\[' | sed 's/GLOBAL/REASSIGN/g'  >>motherboard/unvars.c

echo "}" >> motherboard/unvars.c


cd ..
}



export COMPILEPHASE="preparing"

if [ -z "`echo $BUILDARGS | egrep -- '--quiet|--stfu'`" ]
then
  image resources/lisaem-banner.png || (
  echo 
  echo " +------------------+  Apple Lisa 2 Emulator -Unified Build Script"
  echo " |+---------+ _____ |                                             "
  echo " ||         | _____ |       LisaEm $VERSION"
  echo " ||         |  ---= |        http://lisaem.sunder.net             "
  echo " |+---------+     . |        The Lisa Emulator Project            "
  echo " +------------------+ Copyright (C) ${RELEASEDATE:0:4} ${AUTHOR}"
  echo " /=______________#_=\\           All Rights Reserved               "
  echo "                     Released under the terms of the GNU GPL v2.0"
  )
fi

CHECKDIRS bashbuild bin obj resources share src src/host src/include src/lib src/lisa src/printer \
            src/storage src/tools src/lib/libGenerator src/lib/libdc42


# Parse command line options if any, overriding defaults.

for j in $@; do
  opt=`echo "$j" | sed -e 's/without/no/g' -e 's/disable/no/g' -e 's/enable-//g' -e 's/with-//g'`

  case "$opt" in
    clean)
            CLEAN
            #if we said clean install or clean build, then do not quit, otherwise we clean and quit
            Z="`echo $@ | grep -i install``echo $@ | grep -i build`"
            if  [[ -z "$Z" ]]; then
                echo
                echo -n "Done. "
                elapsed=$(get_elapsed_time)
                [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
                rm -f .env-*
                exit 0
            fi

  ;;
  build*)    echo ;;    #default - nothing to do here, this is the default.


  --prefix=*)     export    PREFIX="${i:9}" ;;
  --pkg-prefix=*) export PKGPREFIX="${i:9}" ;;

  install)
            if [[ -z "$CYGWIN" ]]; then
               [[ "`whoami`" != "root" ]] && echo "Need to be root to install. try: sudo ./build.sh $@" && exit 1
            else
                CygwinSudoRebuild $@
            fi
            export INSTALL=1
            ;;

  uninstall)
            if  [[ -n "$DARWIN" ]]; then
                echo Uninstall commands not yet implemented.
                exit 1
            fi

            if  [[ -n "$CYGWIN"    ]]; then
                [[ -n "$PREFIX"    ]] && echo Deleting $PREFIX    && rm -rf $PREFIX
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

  -64|--64)                     export SIXTYFOURBITS="--64"; 
                                export THIRTYTWOBITS="";
                                export ARCH="-m64"                        ;;

  -32|--32)                     export SIXTYFOURBITS=""; 
                                export THIRTYTWOBITS="--32"; 
                                [[ "$MACHINE" == "x86_64" ]] && export MACHINE="i386"
                                export ARCH="-m32"                        ;;


 --no-color-warn) export GCCCOLORIZED="" ;;
 --no-tools) export NODC42TOOLS="yes" ;;
 --no-debug)
            WITHDEBUG=""
            LIBGENOPTS=""                                     ;;

 --valgrind)
            export GDB="$(which valgrind)"
            if [[ -z "$GDB" ]]; then
               echo "Could not find valgrind" 1>&2
               exit 5
            fi
            MEMTESTEROPTS="-v --show-error-list=yes --time-stamp=yes --leak-check=full --show-leak-kinds=all --track-origins=yes"
            WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG"
            WITHOUTSTRIP="nostrip"
            WITHOUTUPX="noupx"
            # disabled 1.3.0 core options
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            LIBGENOPTS="$LIBGENOPTS --with-debug"             ;;

-D*)        EXTRADEFINES="${EXTRADEFINES} ${opt}";;

 --drmemory)
            export GDB="$(which drmemory)"
            if [[ -z "$(which drmemory)" ]]; then
               echo "Could not find drmemory" 1>&2
               exit 5
            fi
            #MEMTESTEROPTS="-results_to_stderr -show_reachable -summary -fuzz -- "
            MEMTESTEROPTS="-results_to_stderr -- "
            WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG"
            WITHOUTSTRIP="nostrip"
            WITHOUTUPX="noupx"
            # disabled 1.3.0 core options
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            LIBGENOPTS="$LIBGENOPTS --with-debug"             ;;

 --debug|debuggery|bugger*)
            export WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG"
            export WITHDEBUG="$WITHDEBUG -g -DDEBUG"
            WITHOUTSTRIP="nostrip"
            WITHOUTUPX="noupx"
            # disabled 1.3.0 core options
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            LIBGENOPTS="$LIBGENOPTS --with-debug"             ;;

 --profile)
            WITHDEBUG="$WITHDEBUG -p"
            LIBGENOPTS="$LIBGENOPTS --with-profile"           ;;

 --static)
            STATIC="-static"                                  ;;

 --no-static)
            STATIC=""                                         ;;

 --debug*-on-start)
            LIBGENOPTS="$LIBGENOPTS --with-debug-on-start"
            export WARNINGS="-Wall -Wextra -Wno-write-strings -g"
            export WITHDEBUG="$WITHDEBUG -DDEBUGLOG_ON_START -DDEBUG"
            WITHOUTSTRIP="nostrip"
            WITHOUTUPX="noupx"
            WARNINGS="-Wall -Wno-write-strings"
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            LIBGENOPTS="$LIBGENOPTS --with-debug"             ;;

 --no-optimize)
            LIBGENOPTS="$LIBGENOPTS --without-optimize"
            WITHOPTIMIZE=""                                   ;;

 --trace*)
           #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            WARNINGS="-Wall -Wextra -Wno-write-strings -g"
            WITHOUTSTRIP="nostrip"
            WITHOUTUPX="noupx"
            #WITHTRACE="-DDEBUG -DTRACE -DIPC_COMMENTS"
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            LIBGENOPTS="$LIBGENOPTS --with-debug"                         
            LIBGENOPTS="$LIBGENOPTS --with-tracelog"
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            WITHDEBUG="$WITHDEBUG -g -DDEBUG"
            WARNINGS="-Wall -Wno-write-strings"               ;;

 --showcmd) DEBUGCOMPILE="yes"                                ;;
 --debug-mem*|--mem*)
            export WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            export WITHTRACE="-DDEBUG -DTRACE -DDEBUGMEMCALLS"
            export LIBGENOPTS="$LIBGENOPTS --with-tracelog"
            export LIBGENOPTS="$LIBGENOPTS --with-debug-mem"
           #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
           #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            WARNINGS="-Wall -Wno-write-strings"               ;;

 --no-68kflag-opt*)
            LIBGENOPTS="$LIBGENOPTS --no-68kflag-optimize"    ;;

 --no-upx)
            WITHOUTUPX="noupx"                                ;;

 --no-strip)
            WITHOUTSTRIP="nostrip"                            ;;

 --rawbit*)
            WITHBLITS="-DUSE_RAW_BITMAP_ACCESS"               ;;

 --no-rawbit*)
            WITHBLITS="-DNO_RAW_BITMAP_ACCESS"                ;;
 --quiet*|--stfu)
            QUIET="YES"                                       ;;

 *)                        UNKNOWNOPT="$UNKNOWNOPT $j"                       ;;
 esac

done

LIBGENOPTS="$LIBGENOPTS"

if [[ -n "$UNKNOWNOPT" ]]; then
 echo
 echo "Unknown options $UNKNOWNOPT"
 cat <<ENDHELP

Commands:
  clean                 Removes all compiled objects, libs, executables
  build                 Compiles lisaemm, libraries, and tools (default)
  clean build           Remove existing objects, compile everything cleanly
  install               Not yet implemented on all platforms
  uninstall             Not yet implemented on all platforms
  package               Build a package (Not yet implemented everywhere)

Options:
--without-debug         Disables debug and profiling
--with-debug            Enables symbol compilation
--with-tracelog         Enable tracelog (needs debug on, not on win32)
--with-debug-mem        Enables debug and tracelog and memory fn debugging
--with-debug-on-start   Enable debug output as soon as libGenerator is invoked
--valgrind              Same as debug but runs valgrind instead of gdb/lldb
--drmemory              Same as debug but runs drmemory instead of gdb/lldb
--no-color-warn         don't record color ESC codes in compiler warnings
--no-tools              don't compile dc42 tool commands

--with-static           Enables a static compile
--without-static        Enables shared library compile (not recommended)
--without-optimize      Disables optimizations
--without-upx           Disables UPX compression (no upx on OS X)

--without-strip         Disable strip when compiling without debug

-DFOO                   Pass extra defines to C/C++ compilers
-DFOO=BAR

Environment Variables you can pass:

CC                      Path to C Compiler
CPP                     Path to C++ Compiler
WXDEV                   Cygwin Path to wxDev-CPP 6.10 (win32 only)
PREFIX                  Installation directory
NUMCPUS                 override the number of CPUs - Set to 1 to only use 1

i.e. CC=/usr/local/bin/gcc ./build.sh ...

ENDHELP
# --64                 experimental for OS X 10.6
exit 1

fi


# allow versions of LisaEm compiled under Linux, *BSD, etc. to run as compiled
# without being installed.  (Some versions of wxWidgets look for the lowercase name, others, upper)
cd ${XTLD}/share || ( echo "Could not find ../share from $(/bin/pwd)" 1>&2; exit 1 )
ln -sf ../resources lisaem
cd ..

# source code lists for compilation as well as counting needed
export  PHASE1INEXT=c PHASE1OUTEXT=o PHASE2OBJDIR=obj
export  PHASE1LIST="\
        src/lisa/io_board/floppy          \
        src/storage/profile               \
        src/lisa/motherboard/unvars       \
        src/lisa/motherboard/vars         \
        src/lisa/motherboard/glue         \
        src/lisa/motherboard/fliflo_queue \
        src/lisa/io_board/cops            \
        src/lisa/io_board/zilog8530       \
        src/lisa/io_board/via6522         \
        src/lisa/cpu_board/irq            \
        src/lisa/cpu_board/mmu            \
        src/lisa/cpu_board/rom            \
        src/lisa/cpu_board/romless        \
        src/lisa/cpu_board/memory         \
        src/lisa/motherboard/symbols"

export  PHASE2INEXT=cpp PHASE2OUTEXT=o PHASE2OBJDIR=obj
export  PHASE2LIST="\
        src/host/wxui/lisaem_wx:src/include/vars.h:src/host/wxui/include/LisaConfig.h:src/host/wxui/include/LisaConfigFrame.h:src/host/wxui/include/LisaSkin.h:./src/printer/imagewriter/include/imagewriter-wx.h                                                  \
        src/host/wxui/LisaConfig:src/host/wxui/include/LisaConfig.h \
        src/host/wxui/LisaConfigFrame:src/host/wxui/include/LisaConfigFrame.h \
        src/host/wxui/LisaSkin:src/host/wxui/include/LisaSkin.h \
        src/lisa/crt/hqx/hq3x-3x:src/lisa/crt/hqx/include/common.h:src/lisa/crt/hqx/include/hqx.h \
        src/printer/imagewriter/imagewriter-wx:./src/printer/imagewriter/include/imagewriter-wx.h"
# change ^- hq3x vs hq3x-3x here as needed as well as the #define


[[ -n "${WITHDEBUG}${WITHTRACE}" ]] && if [[ -n "$INSTALL" ]];
then
    echo "Warning, will not install since debug/profile/trace options are enabled"
    echo "as Install command is only for production builds."
    INSTALL=""
fi

if [[ -n "${WITHDEBUG}${WITHTRACE}" ]]
then
  WITHOPTIMIZE="-O2 -ffast-math"
fi
cd ./src || exit 1
if needed include/vars.h lisa/motherboard/unvars.c
then
cd ./lisa || exit 1


create_unvars_c
fi


# Has the configuration changed since last time? if so we may need to do a clean build. -----------------------------------
[[ -f "${TLD}/.last-opts" ]] && source "${TLD}/.last-opts"

needclean=0
#debug and tracelog changes affect the whole project, so need to clean it all
[[ "$WITHTRACE" != "$LASTTRACE" ]]             && needclean=1 # && echo "Clean Needed: LASTRACE Changed" 1>&2
[[ "$WITHDEBUG" != "$LASTDEBUG" ]]             && needclean=1 # && echo "Clean Needed: WITHDEBUG Changed" 1>&2
[[ "$SIXTYFOURBITS" != "$LASTSIXTYFOURBITS" ]] && needclean=1 # && echo "Clean Needed: SIXTYFOURBITS Changed" 1>&2
[[ "$THIRTYTWOITS" != "$LASTTHIRTYTWOBITS" ]]  && needclean=1 # && echo "Clean Needed: THIRTYTWOBITS Changed" 1>&2
[[ "$LASTWHICHWXCONFIG" != "$WHICHWXCONFIG" ]] && needclean=1 # && echo "Clean Needed: WHICHWXCONFIG Changed" 1>&2

# display mode changes affect only the main executable, mark it for recomoilation
if [[ "$WITHBLITS" != "$LASTBLITS" ]]; then
  # rm -rf ./lisa/lisaem_wx.o ./lisa/lisaem ./lisa/lisaem.exe ./lisa/LisaEm.app;
  touch host/wxui/lisaem_wx.cpp
fi

cat  > "${TLD}/.last-opts" <<ENDLAST
LASTTRACE="$WITHTRACE"
LASTDEBUG="$WITHDEBUG"
LASTBLITS="$WITHBLITS"
LASTTHIRTYTWOBITS="$LASTTHIRTYTWOBITS"
LASTSIXTYFOURBITS="$LASTSIXTYFOURBITS" 
LASTWHICHWXCONFIG="$WHICHWXCONFIG"
ENDLAST

[[ "$needclean" -gt 0 ]] && CLEAN

export CFLAGS="$ARCH $CFLAGS $NOWARNFORMATTRUNC $NOUNKNOWNWARNING $EXTRADEFINES"
export CPPFLAGS="$ARCH $CPPFLAGS $NODEPRECATEDCPY $NOWARNFORMATTRUNC $NOUNKNOWNWARNING $EXTRADEFINES"
export CXXFLAGS="$ARCH $CXXFLAGS $NODEPRECATEDCPY $NOWARNFORMATTRUNC $NOUNKNOWNWARNING $EXTRADEFINES" 
#2020.01.14 - ^ GCC 9.2.1 throws these on wxWidgets includes, which I'm not going to fix.


#--------------------------------------------------------------------------------------------------------------------------
# estimate how many compile passes we need. if upx is enabled, which takes a very long time, multiply it by some factor
# since upx uses a single core

ESTLIBGENCOUNT=$(  subestimate src/lib/libGenerator --no-banner $LIBGENOPTS    $SIXTYFOURBITS $THIRTYTWOBITS  )
ESTLIBDC42COUNT=$( subestimate src/lib/libdc42      --no-banner                $SIXTYFOURBITS $THIRTYTWOBITS  ) 
[[ -z "$NODC42TOOLS" ]] && ESTTOOLSCOUNT=$( subestimate src/tools  --no-banner $SIXTYFOURBITS $THIRTYTWOBITS  ) || ESTTOOLSCOUNT=0
ESTPHASE1COUNT=$( INEXT=${PHASE1INEXT} OUTEXT=${PHASE1OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiling COUNTNEEDED ${PHASE1LIST} )
ESTPHASE2COUNT=$( INEXT=${PHASE2INEXT} OUTEXT=${PHASE2OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiling COUNTNEEDED ${PHASE2LIST} )

# multiply estimates when using UPX as UPX is very slow
if [[ -z "$WITHOUTUPX" ]] && [[ -n "$UPXCMD" ]]; then
   ESTTOOLSCOUNT=$((  $ESTTOOLSCOUNT  * 10 ));
   ESTPHASE2COUNT=$(( $ESTPHASE2COUNT *  5 ));
fi

ESTIMATETOTALS=$(( $ESTLIBGENCOUNT + $ESTLIBDC42COUNT + $ESTTOOLSCOUNT + $ESTPHASE1COUNT + $ESTPHASE2COUNT + 5 ))

 ESTLIBGENCOUNT=$(( $ESTLIBGENCOUNT  * 100 / $ESTIMATETOTALS ))
ESTLIBDC42COUNT=$(( $ESTLIBDC42COUNT * 100 / $ESTIMATETOTALS ))
  ESTTOOLSCOUNT=$(( $ESTTOOLSCOUNT   * 100 / $ESTIMATETOTALS ))
 ESTPHASE1COUNT=$(( $ESTPHASE1COUNT  * 100 / $ESTIMATETOTALS ))
 ESTPHASE2COUNT=$(( $ESTPHASE2COUNT  * 100 / $ESTIMATETOTALS ))

#(echo "Estimates:"
#echo "libgen:   $ESTLIBGENCOUNT"
#echo "libdc42:  $ESTLIBDC42COUNT"
#echo "tools:    $ESTTOOLSCOUNT"
#echo "Phase1:   $ESTPHASE1COUNT"
#echo "Phase2:   $ESTPHASE2COUNT"
#) 1>&2
#read x
#---------------------------------------------------------------------------------------------------------------------------
echo "* Building prerequisites..."
echo

create_builtby

create_machine_h

cd "${TLD}/src"
if [[ -f   include/machine.h ]]; then
    ln -sf include/machine.h tools/include/machine.h
    ln -sf include/machine.h lib/libGenerator/include/machine.h    
    ln -sf include/machine.h lib/libdc42/include/machine.h       
else
    echo "machine.h failed to create $(pwd)" 1>&2
    exit 1
fi

cd ${TLD}
# Build libraries and tools using subbuild

export PERCENTPROGRESS=0 PERCENTCEILING=${ESTLIBGENCOUNT}
if [[ $ESTLIBGENCOUNT -gt 0 ]]; then
    export COMPILEPHASE="libGenerator"
    export PERCENTJOB=0 REUSESAVE=""
    subbuild src/lib/libGenerator --no-banner $LIBGENOPTS $SIXTYFOURBITS $THIRTYTWOBITS $EXTRADEFINES skipinstall
    unset LIST
fi

export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTLIBGENCOUNT )) 
if [[ $ESTLIBDC42COUNT -gt 0 ]]; then
  export COMPILEPHASE="libdc42"
  export PERCENTJOB=0 REUSESAVE="yes"
  subbuild src/lib/libdc42      --no-banner             $SIXTYFOURBITS $THIRTYTWOBITS $EXTRADEFINES skipinstall
  unset LIST
fi

export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTTOOLSCOUNT )) 
if  [[ $ESTTOOLSCOUNT -gt 0 ]]; then
    export COMPILEPHASE="tools"
    export REUSESAVE="yes"
    subbuild src/tools            --no-banner             $SIXTYFOURBITS $THIRTYTWOBITS $EXTRADEFINES
    unset LIST
fi

echo "* Building LisaEm..."
echo
echo "* LisaEm C Code                (./lisa)"


# Compile C
export COMPILEPHASE="C code"

export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTPHASE1COUNT ))
export PERCENTJOB=0 NUMJOBSINPHASE=15
export COMPILECOMMAND="$CC -W $WARNINGS -Wstrict-prototypes -Wno-format -Wno-unused $WITHDEBUG $WITHTRACE $CFLAGS $INC -c :INFILE:.c -o :OUTFILE:.o"
LIST1=$(WAIT="" INEXT=${PHASE1INEXT} OUTEXT=${PHASE1OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiled COMPILELIST ${PHASE1LIST} )


cd ${TLD}/src/host || (echo "Couldn't cd into host from $(/bin/pwd)" 1>&2; exit 1)

export WINDOWS_RES_ICONS=$( printf 'lisa2icon   ICON   "lisa2icon.ico"\r\n')
# used to have these too, but now switched to skins and PNGs, can use printf if we want to add such things
# to print multiple lines into that var.
#//floppy0   BITMAP "floppy0.bmp"
#//floppy1   BITMAP "floppy1.bmp"
#//floppy2   BITMAP "floppy2.bmp"
#//floppy3   BITMAP "floppy3.bmp"
#//floppyN   BITMAP "floppyN.bmp"
#//lisaface0 BITMAP "lisaface0.bmp"
#//lisaface1 BITMAP "lisaface1.bmp"
#//lisaface2 BITMAP "lisaface2.bmp"
#//lisaface3 BITMAP "lisaface3.bmp"
#//power_off BITMAP "power_off.bmp"
#//power_on  BITMAP "power_on.bmp"

printf ' \r'  # eat twirly cursor, since we're not waitqing

if needed lisaem_static_resources.cpp ${TLD}/obj/lisaem_static_resources.o; then
  qjob "!!  Compiled lisaem_static_resources.cpp " $CXX $ARCH $CXXFLAGS -c lisaem_static_resources.cpp -o ${TLD}/obj/lisaem_static_resources.o 1>&2
  waitqall
fi

printf ' \r'

#echo "LIST:$LIST" >/tmp/slot.list.0

LIST="$LIST ${TLD}/obj/lisaem_static_resources.o $(windres)"

#echo "LIST:$LIST" >/tmp/slot.list.1

# ^^ this also creates the windows resource via the windres function for Windows only. windres fn must be empty elsewhere
# note the $(windres) is executing that function call
rm -f lisaem lisaem.exe
#vars.c must be linked in before any C++ source code or else there will be linking conflicts!

echo
printf ' \r'
echo "* wxWidgets C++ Code           (./wxui)"

# save WARNINGS settings, add C++ extra warnings
if [[ -n "$WARNINGS" ]]; then
  OWARNINGS="$WARNINGS"
  WARNINGS="$WARNINGS -Weffc++"
fi

# Compile C++
cd "${TLD}"
export COMPILEPHASE="C++ code"
export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTPHASE1COUNT ))
export PERCENTJOB=0 NUMJOBSINPHASE=6
CXXFLAGS="$CXXFLAGS -I src/include -I resources"
export COMPILECOMMAND="$CXX -W -Wno-write-strings $WARNINGS $WITHDEBUG $WITHTRACE $WITHBLITS $INC $CXXFLAGS -c :INFILE:.cpp -o :OUTFILE:.o "
LIST=$( WAIT="yes" INEXT=${PHASE2INEXT} OUTEXT=${PHASE2OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiled COMPILELIST ${PHASE2LIST} )
waitqall

#echo "LIST:$LIST" >/tmp/slot.list.2

# restore warnings
[ -n "$OWARNINGS" ] && WARNINGS="$OWARNINGS"


for i in `echo $LIST`; do WXLIST="$WXLIST `echo $i|grep -v lisaem_wx`"; done

echo
echo '---------------------------------------------------------------' >> $BUILDWARNINGS

cd "${TLD}"

[[ -n "$DARWIN" ]] && LISANAME="LisaEm" || LISANAME="lisaem${EXT}"


#(
#  echo "LIST: $LIST"
#  echo "LIST1: $LIST1"
#  echo "WXLIST: $WXLIST"
#) >/tmp/slot.lists.txt

export COMPILEPHASE="linking"
export PERCENTPROCESS=98 PERCENTCEILING=99 PERCENTJOB=0 NUMJOBSINPHASE=1
update_progress_bar $PERCENTPROCESS $PERCENTJOB $NUMJOBSINPHASE $PERCENTCEILING
waitqall
qjob  "!!* Linked ./bin/${LISANAME}" $CXX $ARCH $GUIAPP $GCCSTATIC $WITHTRACE $WITHDEBUG -o bin/$LISANAME  $LIST1 $LIST src/lib/libGenerator/lib/libGenerator.a \
      src/lib/libdc42/lib/libdc42.a  $LINKOPTS $SYSLIBS $LIBS
waitqall

export COMPILEPHASE="pack/install"
export PERCENTPROCESS=98 PERCENTCEILING=100 PERCENTJOB=0 NUMJOBSINPHASE=1
update_progress_bar $PERCENTPROCESS $PERCENTJOB $NUMJOBSINPHASE $PERCENTCEILING

cd "${TLD}/bin"
if  [[ -f "$LISANAME" ]]; then

    strip_and_compress ${LISANAME}
#.
#└── Contents                                ${TLD}/bin/LisaEm.app/Contents
#    ├── Info.plist
#    ├── MacOS                               ${TLD}/bin/LisaEm.app/Contents/MacOS
#    │   └── LisaEm
#    ├── PkgInfo
#    └── Resources                           ${TLD}/bin/LisaEm.app/Contents/Resources
#        └── skins                           
#            └── default
#                ├── default.conf
#                ├── floppy0.png
#                ├── floppy1.png
    
    if [[ -n "$DARWIN" ]]; then
        echo "* Creating macos application" 1>&2

        CONTENTS="${TLD}/bin/LisaEm.app/Contents/"
        RESOURCES="${TLD}/bin/LisaEm.app/Contents/Resources"
        BIN="${TLD}/bin/LisaEm.app/Contents/MacOS"

        PLISTSRC="${TLD}/resources/Info.plist"
        ICONSRC="${TLD}/resources/LisaEm.icns"

        mkdir -pm775 "${BIN}" "${RESOURCES}"
        cp "${TLD}/resources/lisaem.sh"               "${BIN}/lisaem.sh"                               || exit $?
        chmod 755                                     "${BIN}/lisaem.sh"                               || exit $?
        mv "${TLD}/bin/${LISANAME}"                   "${BIN}/lisaem${BINARYEXTENSION}-wx${WXVERSION}" || exit $?
        sed "s/_VERSION_/$VERSION/g" < "${PLISTSRC}"> "${CONTENTS}/Info.plist"                         || exit $?
        echo -n 'APPL????'                          > "${CONTENTS}/PkgInfo"                            || exit $?
        cp "${ICONSRC}"                               "${RESOURCES}"                                   || exit $?
        (cd "${TLD}/resources";tar cpf - skins) | (cd "${RESOURCES}"; tar xpf - )
        x=$?
        if  [[ "$x" -ne 0 ]]; then
            echo "Failed to copy ${TLD}/resources/skins to ${RESOURCES}" 1>&2
            exit $x
        fi
        
        LISANAME="${BIN}/lisaem${BINARYEXTENSION}-wx${WXVERSION}"

        if [[ -n "$WITHDEBUG" ]]; then
            echo "run -p" >gdb-run
            echo "Launching debugger:"
            [[ -n "$( echo $GDB | grep gdb )"  ]] && $GDB -x gdb-run     "$LISANAME"
            [[ -n "$( echo $GDB | grep lldb )" ]] && $GDB -o "run -p" -f "$LISANAME"
            fi
        fi
        #if we turned on profiling, process the results
        if [[ `echo "$WITHDEBUG" | grep 'p' >/dev/null 2>/dev/null` ]];then
          $GPROF "${LISANAME}" >lisaem-gprof-out
          echo lisaem-gprof-out created.
        fi

        if [[ -n "$INSTALL" ]]; then
          echo "* Installing LisaEm.app" 1>&2
          (cd "${TLD}/bin"; tar cf - ./LisaEm.app ) | (cd "$PREFIX"; tar xf -)
          x=$?
          if  [[ "$x" -ne 0 ]]; then
              echo "Failed to copy ${TLD}/bin/LisaEm.app to ${PREFIX}" 1>&2
              exit $x
          fi
          echo "* Done Installing." 1>&2
          elapsed=$(get_elapsed_time)
          [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
          exit 0
        fi
        echo "Done." 1>&2; exit 0
    fi
    
    
    echo
    ####
    
    if [[ -z "$WITHDEBUG" ]]; then
    
      ## Install ###################################################
      if [[ -n "$INSTALL" ]]; then
    
        if [[ -n "$CYGWIN" ]]; then
              #PREFIX   ="/cygdrive/c/Program Files/Sunder.NET/LisaEm"
              #PREFIXLIB="/cygdrive/c/Program Files/Sunder.NET/LisaEm"
              echo "* Installing skins in $PREFIXLIB/LisaEm"
              mkdir -pm755 "$PREFIX"
              (cd "${TLD}/resources"; tar cpf - skins) | (cd "$PREFIX"; tar xpf - )
              echo "* Installing lisaem.exe binary in $PREFIX/lisaem"
              cp "${TLD}/bin/lisaem.exe" "$PREFIX"
              echo -n "  Done Installing."
              elapsed=$(get_elapsed_time)
              [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
              exit 0
        else
          #   PREFIX="/usr/local/bin"
          #   PREFIXLIB="/usr/local/share/"
    
          echo "* Installing resources in     $PREFIXLIB/lisaem" 1>&2
          mkdir -pm755 $PREFIXLIB/LisaEm/ $PREFIX
          cp ../resources/*.wav ../resources/*.png "$PREFIXLIB/lisaem/"
          echo "* Installing lisaem binary in $PREFIX/lisaem" 1>&2
          cp lisaem "$PREFIX"
          echo -n "  Done Installing." 1>&2
          elapsed=$(get_elapsed_time)
          [[ -n "$elapsed" ]] && echo "$elapsed seconds" 1>&2 || echo 1>&2
          exit 0
        fi
    
      fi     # end of  INSTALL
      ##########################################################
    
    else
      elapsed=$(get_elapsed_time)
      [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
    
      if [[ -z "$CYGWIN" ]]; then
        cd ../bin
        echo "run -p" >gdb-run
        if [[ -n "$(echo $GDB | grep valgrind)" ]]; then 
            echo "* Note: $GDB output will be appended to ${BUILDWARNINGS}" 1>&2
            echo "Running:             $GDB $MEMTESTEROPTS ./lisaem 2>&1 | tee -a ${BUILDWARNINGS} from `pwd`" 1>&2
            if [[ -n "$MEMTESTEROPTS" ]]; then $GDB $MEMTESTEROPTS ./lisaem -p 2>&1 | tee -a "${BUILDWARNINGS}"
            else                               $GDB -x gdb-run     ./lisaem    2>&1 | tee -a "${BUILDWARNINGS}"
            fi
        else
            $GDB ./lisaem      -x gdb-run
            exit $?
        fi
      else
	cp lisaem.exe "/cygdrive/c/Program Files/Sunder.NET/LisaEm" || exit 1
	cd ../resources
	tar cpf - skins | ( cd "/cygdrive/c/Program Files/Sunder.NET/LisaEm"; tar xpf - )
	cd "/cygdrive/c/Program Files/Sunder.NET/LisaEm" || exit 1
        echo "run -p" >gdb-run
        $GDB ./lisaem.exe -x gdb-run
        exit $?
      fi
    
      if  [[ -n "$(echo -- $WITHDEBUG | grep p)" ]]; then
          [[ -n "$CYGWIN" ]] && $GPROF lisaem.exe >obj/lisaem-gprof-out.txt
          [[ -z "$CYGWIN" ]] && $GPROF lisaem     >obj/lisaem-gprof-out.txt
          echo "Profiling output written to lisaem-gprof-out.txt" 1>&2
          exit 0
      fi   
    fi
fi

if  egrep -i 'warn:|error:' $BUILDWARNINGS >/dev/null 2>/dev/null; then
    echo "Compiler warnings/errors were saved to: $BUILDWARNINGS" 1>&2
fi

elapsed=$(get_elapsed_time)
[[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo

# As this is the top level script, it's allowed to exit. subbuilds may not!
exit 0
