#!/usr/bin/env bash
unset ARCH # FreeBSD ports presets this to amd64, which causes conflicts for us
#------------------------------------------------------------------------------------------#
# Standard block for each bashbuild script - these are used for copyright notices, packages
# this standard block ends around line 47
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
# Some of these variables can be overwritten from env variables.
# Below, VER="${VER:-undefined}" means: use the VER env. variable if it is defined, otherwise use the default value of "undefined" (undefined version).
# Similarly, the RELEASEDATE env. variavble will be used, or (if not set), it will use "today's date".
###########################################################################################
     SOFTWARE="LisaEm"                    # name of the software (can contain upper case)
       LCNAME="lisaem"                    # lower case name used for the directory
  DESCRIPTION="The first fully functional Lisa Emulator™"   # description of the package
          VER="${VER:-undefined}"         # the version number
                                          # snapcraft limits the length of the version
  RELEASEDATE="${RELEASEDATE:-$(date +%Y.%m.%d)}" # release date.  must be YYYY.MM.DD
       AUTHOR="Friends of Ray Arachelian" # name of the author
    AUTHEMAIL="lisaem@arcanebyte.com"     # email address for this software
      COMPANY="Sunder.NET"                # company (vendor for sun pkg)
        CONAM="SUNDERNET"                 # company short name for Solaris pkgs
          URL="https://lisaem.sunder.net" # url to website of package
COPYRIGHTYEAR="2026"
COPYRIGHTLINE="Copyright © ${COPYRIGHTYEAR} $AUTHOR,"
LICENSERELEASE="Released under the terms of the GNU GPL v3.0"
LICENSESHORT="GPL"                        # for use in RPM packages
# ----------------------------------------------------------------------------------------
# vars auto built from the above.
VERSION="${VER}_${RELEASEDATE}"
# copy the arguements given to us as they'll be used again when we make packages
BUILDARGS="$0 $@"
export VER RELEASEDATE AUTHOR SOFTWARE LCNAME DESCRIPTION COMPANY CONAM URL VERSION BUILDARGS 

#------------------------------------------------------------------------------------------#
# end of standard section for all build scripts.
#------------------------------------------------------------------------------------------#
# you'll have to rewrite (or at least customize) the rest of this script in a similar manner for your own package


# Packaging parameters for various operating system targets
export DEBDEPENDS="libpangocairo, libgtk-3, libatk1, libcario, Xtst, libpangoft2, libpango-1"
export RPMDEPENDS="gtk3"

# SNAP PLUGS/DEPENDS must be comma separate single strings for proper snapcraft.yaml syntax
export SNAPPLUGS="x11, pulseaudio, process-control, network, network-bind"
export SNAPDEPENDS="libgtk2.0-bin, libgtk2.0-0, libasound2, libasyncns0, libatk-bridge2.0-0, libatk1.0-0, libatspi2.0-0, libcairo-gobject2, libcairo2, libdatrie1, libepoxy0, libflac8, libfontconfig1, libfreetype6, libfribidi0, libgdk-pixbuf2.0-0, libgraphite2-3, libgtk-3-0, libharfbuzz0b, libice6, libogg0, libpango-1.0-0, libpangocairo-1.0-0, libpangoft2-1.0-0, libpixman-1-0, libpng16-16, libpulse0, libsdl2-2.0-0, libsm6, libsndfile1, libthai0, libvorbis0a, libvorbisenc2, libwayland-client0, libwayland-cursor0, libwayland-egl1, libx11-6, libxau6, libxcb-render0, libxcb-shm0, libxcb1, libxcomposite1, libxcursor1, libxdamage1, libxdmcp6, libxext6, libxfixes3, libxi6, libxinerama1, libxkbcommon0, libxrandr2, libxrender1, libxss1, libxxf86vm1"
export SNAPCONFINEMENT="devmode" # strict # classic
export SNAPCOMMAND="usr/local/bin/lisaem" # should not be absolute path due to snap install

export FREEBSDORIGIN="emulators/lisaem"
export FREEBSDDEPENDS="gtk3, gdk-pixbuf2, libtiff"
export DEBDEPENDS="gtk3, gdk-pixbuf2, libpangocairo, libgtk, libcario, libpangoft2, libpango1, libtiff, libglib"

export OPENBSDORIGIN="emulators/lisaem"
export OPENBSDDEPENDS="devel/desktop-file-utils:desktop-file-utils-*:desktop-file-utils-0.26 devel/sdl2:sdl2-*:sdl2-2.0.16 misc/shared-mime-info:shared-mime-info-*:shared-mime-info-2.1 x11/gtk+3,-guic:gtk-update-icon-cache-*:gtk-update-icon-cache-3.24.30 x11/gtk+3,-main:gtk+3-*:gtk+3-3.24.30"
export OPENBSDWANTLIB="GL.17.1 SDL2.0.10 c.96.1 gtk-3.2201.0 m.10.1"

export SUNOSCATEGORY="emulator"
export SUNOSIPSREPO="${LCNAME}-repository"
export SUNOSIPSPUBLISHER="sundernet"

# ensure the sub-builds are executable to avoid git caused issues
chmod 755 src/lib/libdc42/build.sh src/lib/libGenerator/build.sh src/tools/build.sh  2>/dev/null
#--------------------------------------------------------------------------------------------------------
# this was old way of version tracking, left here for historical reference as to release dates
#--------------------------------------------------------------------------------------------------------
#VERSION="1.2.7-RC3a_2020.08.24"
#VERSION="1.2.7-RC3_2020.08.19"
#VERSION="1.2.7-RC2_2020.08.03"
#VERSION="1.2.7-RC1_2020.05.27"
#VERSION="1.2.7-BETA_2019.03.31"
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

# hooks for my own selfish experimentation
[[ -f experimental/experimental-hooks.sh ]] && ./experimental/experimental-hooks.sh

WITHDEBUG=""             # -g for debugging, -p for profiling. -pg for both
LIBGENOPTS=""            # passthrough to libGenerator
WITHOPTIMIZE="-O2 -ffast-math -fomit-frame-pointer"
WITHUNICODE="--unicode=yes"

function CLEAN() {
            # note we expect to be inside src at this point, and we'll make our way out as needed.
            echo "* Cleaning..." 1>&2
            cd "${TLD}"
            CLEANARTIFACTS "*.o" "*.a" "*.so" "*.dylib" "*.exe" get-uintX-types "cpu68k-?.c" def68k gen68k hashes.txt "slot.*.sh" build-warnings.txt windres_private.res
            subbuild src/lib/libGenerator --no-banner clean

            # TerminalWx isn't something I intend to develop, but rather just use, so disable its warnings
            export OWARNINGS="$WARNINGS"
            export OCFLAGS="$CFLAGS"
            export OCPPFLAGS="$CPPFLAGS"
            export OCXXFLAGS="$CXXFLAGS"

            subbuild src/lib/TerminalWx   --no-banner clean

            export CFLAGS="$OCFLAGS"
            export CPPFLAGS="$OCPPFLAGS"
            export CXXFLAGS="$OCXXFLAGS"
            unset  OWARNINGS OCFLAGS OCPPFLAGS OCXXFLAGS

            subbuild src/lib/libdc42      --no-banner clean
            subbuild src/tools            --no-banner clean
            rm -rf bin/${SOFTWARE}.app bin/lisaem bin/${MACOSX_MAJOR_VER}/*.dSYM # for macos x - this is a dir so CLEANARTIFACTS will not handle it properly
            rm -f /tmp/slot.*.sh*
            rm -rf ./pkg/build; mkdir -pm755  pkg/build; echo "Built packages go here" >./pkg/build/README
            rm -rf scripts/wxWidgets-?\.*
            cd "${TLD}/bin"; ln -sf ../bashbuild/interim-build.sh build.sh
            if [[ -n "$(which ccache 2>/dev/null)" ]]; then echo -n "* "; ccache -c; fi
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
  echo #                       12345678901234567890123456789012345678901234
  echo " +------------------+  Apple Lisa 2 Emulator -Unified Build Script"
  echo " |+---------+ _____ |                                             "
  echo " ||         | _____ |  $(center 44 ${SOFTWARE}\ ${VERSION})"
  echo " ||         |  ---= |        ${URL}"
  echo " |+---------+     . |        The Lisa Emulator Project            "
  echo " +------------------+ ${COPYRIGHTLINE}"
  echo " /=______________#_=\\           All Rights Reserved               "
  echo "                      ${LICENSERELEASE}"
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
            #if we said clean with install, package, or build, then do not quit after clean
            Z="`echo $@ | egrep -i 'install|package|pkg|build'`"
            if  [[ -z "$Z" ]]; then
                echo
                echo -n "Done. "
                elapsed=$(get_elapsed_time)
                [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
                rm -f .env-* bin/hashes.txt bin/gdb-run obj/build-warnings.txt obj/get-uintX-types .last-opts
                exit 0
            fi

  ;;
  build*)    echo ;;    #default - nothing to do here, this is the default.

  -y)             export    YESTOALL="yes"  ;;

  --prefix=*)     export    PREFIX="${i:9}" ;;
  --pkg-prefix=*) export PKGPREFIX="${i:9}" ;;


  pkg|package)  
                # The "package" command sets INSTALL=1, WITHPKG="yes" for supported platforms, 
                # which invokes the "install" codepath, with the target folder set to "lisaem/pkg/build/tmp".
                # The WITHPKG="yes" triggers an extra step that packages the files in that folder.
                # The "install" code for Windows requires Admininstative privileges, to write into "C:\Program Files\...".
                # Even though "package" does not need Admininstative privileges, it is hard to enable/enforce that check
                # only for "install" and not for "package". 
                # TO DO: Rewrite the "package" command, to be completely separate from "install"; the code will be much cleaner.
                if ! is_pkg_available; then
                    echo "This is only implemented on macos, Windows, Linux, FreeBSD, and OpenIndiana currently" 1>&2
                    exit 1
                fi

                # Sets INSTALL=1 for platforms that support packaging:
                do_fake_install_for_pkg
                export WITHPKG="yes"
                package_check_sudo_pass
                ;;

  install)  install_check_sudo_pass
            ;;

  uninstall)
            if  [[ -n "$DARWIN" ]]; then
                echo Uninstall commands not yet implemented.
                exit 1
            fi

            if  [[ -n "$CYGWIN"    ]]; then
                echo "Uninstall command is not implemented on Windows. To uninstall LisaEm, please manually delete folder C:\Program Files\\${COMPANY}\\${SOFTWARE}."
                exit 0
            fi

            #Linux, etc.
            #PREFIX="/usr/local/bin"
            #PREFIXLIB="/usr/local/share/"

            echo Uninstalling from $PREFIX and $PREFIXLIB

            rm -rf $PREFIXLIB/lisaem/ 

            rm -f  $PREFIX/blu-to-dc42
            rm -f  $PREFIX/dc42-copy-boot-loader
            rm -f  $PREFIX/dc42-dumper
            rm -f  $PREFIX/dc42-resize-to-400k
            rm -f  $PREFIX/dc42-to-raw
            rm -f  $PREFIX/dc42-to-rraw
            rm -f  $PREFIX/dc42-to-split-raw
            rm -f  $PREFIX/dc42-to-tar
            rm -f  $PREFIX/decode-vsrom
            rm -f  $PREFIX/idefile-to-dc42
            rm -f  $PREFIX/lisa-serial-info
            rm -f  $PREFIX/lisadiskinfo
            rm -f  $PREFIX/lisaem
            rm -f  $PREFIX/lisafsh-tool
            rm -f  $PREFIX/los-bozo-on
            rm -f  $PREFIX/los-deserialize
            rm -f  $PREFIX/patchxenix
            rm -f  $PREFIX/raw-to-dc42
            rm -f  $PREFIX/rraw-to-dc42
            rm -f  $PREFIX/uniplus-bootloader-deserialize
            rm -f  $PREFIX/uniplus-set-profile-size
            rm -f  $PREFIX/dc42-add-tags
            rm -f  $PREFIX/dc42-diff
            rm -f  $PREFIX/dc42-copy-selected-sectors
            exit 0

    ;;

  -64|--64|-m64)                export SIXTYFOURBITS="--64"; 
                                export THIRTYTWOBITS="";
                                export ARCH="-m64"; export SARCH="-m64"     ;;

  -32|--32|-m32)                export SIXTYFOURBITS=""; 
                                export THIRTYTWOBITS="--32"; 
                                [[ "$MACHINE" == "x86_64" ]] && export MACHINE="i386"
                                export ARCH="-m32"; export SARCH="-m32"     ;;

  -march=*|--march=*)           export ARCH="${opt} $ARCH"                  ;;
  -arch=*|--arch=*)             export ARCH="$(echo ${opt} | sed -e 's/=/ /g') $ARCH"
                                export ARCHOVERRIDE="$(echo ${opt} | cut -d= -f2 )"
                                export SARCH="$opt $SARCH" ;;

 --no-color-warn) export GCCCOLORIZED="" ;;
 --no-tools) export NODC42TOOLS="yes" ;;
 --no-debug)
            export WITHDEBUG=""
            export LIBGENOPTS=""                                     ;;

 --allow2mbram)    export WITHDEBUG="$WITHDEBUG -DALLOW2MBRAM"              ;;
 --full2mbram)     export WITHDEBUG="$WITHDEBUG -DALLOW2MBRAM -DFULL2MBRAM" ;;
 --enableseriala)  export WITHDEBUG="$WITHDEBUG -DALLOWSERIALA"             ;;

 --valgrind)
            export GDB="$(which valgrind)"
            if [[ -z "$GDB" ]]; then
               echo "Could not find valgrind" 1>&2
               exit 5
            fi
            export MEMTESTEROPTS="-v --show-error-list=yes --time-stamp=yes --leak-check=full --show-leak-kinds=all --track-origins=yes"
            export WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG"
            export WITHOUTSTRIP="nostrip"
            export WITHOUTUPX="noupx"
            # disabled 1.3.0 core options
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            export LIBGENOPTS="$LIBGENOPTS --with-debug"             ;;

-D*)        export EXTRADEFINES="${EXTRADEFINES} ${opt}";;

 --drmemory)
            export GDB="$(which drmemory)"
            if [[ -z "$(which drmemory)" ]]; then
               echo "Could not find drmemory" 1>&2
               exit 5
            fi
            #MEMTESTEROPTS="-results_to_stderr -show_reachable -summary -fuzz -- "
            export MEMTESTEROPTS="-results_to_stderr -- "
            export WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG"
            export WITHOUTSTRIP="nostrip"
            export WITHOUTUPX="noupx"
            # disabled 1.3.0 core options
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            export LIBGENOPTS="$LIBGENOPTS --with-debug"             ;;

 --debug|debuggery|bugger*|bug*)
            export WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG"
            export WITHDEBUG="$WITHDEBUG -g -DDEBUG"
            export WITHOUTSTRIP="nostrip"
            export WITHOUTUPX="noupx"
            export LIBGENOPTS="$LIBGENOPTS --with-debug"             
            export WITHOPTIMIZE=""                                   ;;
            # disabled 1.3.0 core options
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"

 --debug-memcalls|debug-mem-calls)
            export WITHDEBUG="$WITHDEBUG -DDEBUGMEMCALLS"            ;;

 --profile)
            export WITHDEBUG="$WITHDEBUG -p"
            export LIBGENOPTS="$LIBGENOPTS --with-profile"
            export WITHPROFILE="yes"                                 ;;

 --static)
            STATIC="-static"                                         ;;

 --no-static)
            STATIC=""                                                ;;

 --trace*on-start)
            export LIBGENOPTS="$LIBGENOPTS --debug --trace-on-start -DDEBUGLOG_ON_START"
            export WARNINGS="-Wall -Wextra -Wno-write-strings -g"
            export WITHDEBUG="$WITHDEBUG -DDEBUGLOG_ON_START -DDEBUG"
            export WITHOUTSTRIP="nostrip"
            export WITHOUTUPX="noupx"
            export WARNINGS="-Wall -Wno-write-strings"
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #export LIBGENOPTS="$LIBGENOPTS --with-debug"             
            ;;

 --no-optimize)
            export LIBGENOPTS="$LIBGENOPTS --without-optimize"
            export WITHOPTIMIZE=""                                   ;;

 --trace*)
            export WARNINGS="-Wall -Wextra -Wno-write-strings -g -DDEBUG -DTRACE"
            export WITHOUTSTRIP="nostrip"
            export WITHOUTUPX="noupx"
            export LIBGENOPTS="$LIBGENOPTS --with-debug --with-tracelog"
            export WITHDEBUG="$WITHDEBUG -g -DDEBUG -DTRACE"
            export LIBGENOPTS="$LIBGENOPTS --with-debug"
            export WITHOPTIMIZE=""                                   ;;
            #WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            #WITHTRACE="-DDEBUG -DTRACE -DIPC_COMMENTS"
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"

 --showcmd) export DEBUGCOMPILE="yes"                                ;;
 --debug-mem*|--mem*)
            export WITHDEBUG="$WITHDEBUG -g -DIPC_COMMENTS"
            export WITHTRACE="-DDEBUG -DTRACE -DDEBUGMEMCALLS"
            export LIBGENOPTS="$LIBGENOPTS --with-tracelog"
            export LIBGENOPTS="$LIBGENOPTS --with-debug-mem"
            #LIBGENOPTS="$LIBGENOPTS --with-ipc-comments"
            #LIBGENOPTS="$LIBGENOPTS --with-reg-ipc-comments"
            export WARNINGS="-Wall -Wno-write-strings"               ;;

 --no-68kflag-opt*)
            export LIBGENOPTS="$LIBGENOPTS --no-68kflag-optimize"    ;;

 --no-upx)
            export WITHOUTUPX="noupx"                                ;;

 --no-strip)
            export WITHOUTSTRIP="nostrip"                            ;;

 --rawbit*|--raw-bit*)
            export WITHBLITS="-DUSE_RAW_BITMAP_ACCESS"               ;;

 --no-rawbit*|--no-raw-bit*)
            export WITHBLITS="-DNO_RAW_BITMAP_ACCESS"                ;;
 --quiet*|--stfu|-q)
            export QUIET="YES"                                       ;;

 *)         UNKNOWNOPT="$UNKNOWNOPT $j"                              ;;
 esac

done

LIBGENOPTS="$LIBGENOPTS"

if [[ -n "$UNKNOWNOPT" ]]; then
 echo
 echo "Unknown options $UNKNOWNOPT"
 cat <<ENDHELP

Commands:
  clean                 Removes all compiled objects, libs, executables
                        (does not build unless you also add build)
  build                 Compiles lisaemm, libraries, and tools (default)
  clean build           Remove existing objects, compile everything cleanly
  install               Not yet implemented on all platforms
  uninstall             Not yet implemented on all platforms
  package|pkg           Build a package (DEB, RPM: Linux, NSIS/ZIP: windows,
                        FreeBSD packages, OpenIndiana/Solaris: classic pkg)

Options:                (can skip '--with-', or use '--no-' instead of '--without-')
--without-debug         Disables debug and profiling
--with-debug            Enables symbol compilation
--with-tracelog         Enable tracelog (needs debug on, not on win32)
--with-debug-mem        Enables debug and tracelog and memory fn debugging
--with-trace-on-start   Tracelog on as soon as powered on
--valgrind              Same as debug but runs valgrind instead of gdb/lldb
--drmemory              Same as debug but runs drmemory instead of gdb/lldb
--no-color-warn         don't record color ESC codes in compiler warnings
--no-tools              don't compile dc42 tool commands
--with-static           Enables a static compile
--without-static        Enables shared library compile (not recommended)
--without-optimize      Disables optimizations
--without-upx           Disables UPX compression (no upx on some macos x)
--without-strip         Disable strip when compiling without debug
Crashy options:
--allow2mbram           Allow 2MB RAM for LOS (actually 2MB-128K)
--full2mbram            Allow real 2MB RAM, but won't work with H-ROM or LOS
--enableseriala         Enable Serial Port A - broken in LOS causes crashes

Compiling Options:
-DFOO                   Pass extra defines to C/C++ compilers
-DFOO=BAR

--32|--64               Select 32 or 64 bit binaries (or -32|-m32|-64|-m64)
--arch={}               Compile for specific architecture
                          i.e. x86_64, ppc, ppc64, arm64, etc...
                          but you should avoid -32|-64

Environment Variables you can pass:

CC,CPP,GDB              Paths to C/C++ Compiler tools
WXDEV                   Cygwin Path to wxDev-CPP 6.10 (win32 only)
PREFIX                  Installation directory
NUMCPUS                 override the number of CPUs - Set to 1 to only use 1

i.e. CC=/usr/local/bin/gcc ./build.sh ...

ENDHELP
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
        src/storage/hle                   \
        src/lisa/motherboard/unvars       \
        src/lisa/motherboard/vars         \
        src/lisa/motherboard/glue         \
        src/lisa/motherboard/fliflo_queue \
        src/lisa/io_board/cops            \
        src/lisa/io_board/z8530           \
        src/lisa/io_board/z8530-telnetd   \
        src/lisa/io_board/z8530-pty       \
        src/lisa/io_board/z8530-tty       \
        src/lisa/io_board/z8530-shell     \
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
        src/printer/imagewriter/imagewriter-wx:./src/printer/imagewriter/include/imagewriter-wx.h \
        src/host/wxui/z8530-terminal"
# change ^- hq3x vs hq3x-3x here as needed as well as the #define


[[ -n "${WITHDEBUG}${WITHTRACE}" ]] && if [[ -n "$INSTALL" ]];
then
    echo "Warning, will not install since debug/profile/trace options are enabled"
    echo "as Install command is only for production builds."
    INSTALL=""
fi

if [[ -n "${WITHDEBUG}${WITHTRACE}" ]]
then
  WITHOPTIMIZE="-ffast-math"
fi
cd ./src || exit 1
if needed include/vars.h lisa/motherboard/unvars.c
then
cd ./lisa || exit 1


create_unvars_c
fi

if  [[ -n "$DARWIN" ]]; then
    LISANAME="${SOFTWARE}"
    # may need to test which versions this works with, etc.
    WITHBLITS="-DUSE_RAW_BITMAP_ACCESS"
else
    LISANAME="lisaem${EXT}"
fi


# Has the configuration changed since last time? if so we may need to do a clean build. -----------------------------------
[[ -f "${TLD}/.last-opts" ]] && source "${TLD}/.last-opts"

needclean=0
#debug and tracelog changes affect the whole project, so need to clean it all
[[ "$WITHTRACE" != "$LASTTRACE"            ]]  && needclean=1 #&& echo "Clean Needed: LASTRACE Changed" 1>&2
[[ "$WITHDEBUG" != "$LASTDEBUG"             ]] && needclean=1 #&& echo "Clean Needed: WITHDEBUG Changed" 1>&2
[[ "$SIXTYFOURBITS" != "$LASTSIXTYFOURBITS" ]] && needclean=1 #&& echo "Clean Needed: SIXTYFOURBITS Changed :$SIXTYFOURBITS: :$LASTSITYFOURBITS:" 1>&2
[[ "$THIRTYTWOITS" != "$LASTTHIRTYTWOBITS"  ]] && needclean=1 #&& echo "Clean Needed: THIRTYTWOBITS Changed" 1>&2
[[ "$LASTWHICHWXCONFIG" != "$WHICHWXCONFIG" ]] && needclean=1 #&& echo "Clean Needed: WHICHWXCONFIG Changed" 1>&2
[[ "$LASTARCH" != "$ARCH"                   ]] && needclean=1 #&& echo "Clean Needed: ARCH Changed" 1>&2
# display mode changes affect only the main executable, mark it for recomoilation
if [[ "$WITHBLITS" != "$LASTBLITS" ]]; then
  # rm -rf ./lisa/lisaem_wx.o ./lisa/lisaem ./lisa/lisaem.exe ./lisa/${SOFTWARE}.app;
  touch host/wxui/lisaem_wx.cpp
fi

cat  > "${TLD}/.last-opts" <<ENDLAST
LASTTRACE="$WITHTRACE"
LASTDEBUG="$WITHDEBUG"
LASTBLITS="$WITHBLITS"
LASTTHIRTYTWOBITS="$THIRTYTWOBITS"
LASTSIXTYFOURBITS="$SIXTYFOURBITS" 
LASTWHICHWXCONFIG="$WHICHWXCONFIG"
LASTARCH="$ARCH"
ENDLAST

[[ "$needclean" -gt 0 ]] && CLEAN

export CFLAGS="$CFLAGS $NOWARNFORMATTRUNC $NOUNKNOWNWARNING $EXTRADEFINES"
export CPPFLAGS="$CPPFLAGS $NODEPRECATEDCPY $NOWARNFORMATTRUNC $NOUNKNOWNWARNING $EXTRADEFINES"
export CXXFLAGS="$CXXFLAGS $NODEPRECATEDCPY $NOWARNFORMATTRUNC $NOUNKNOWNWARNING $EXTRADEFINES" 
#2020.01.14 - ^ GCC 9.2.1 throws these on wxWidgets includes, which I'm not going to fix.


#--------------------------------------------------------------------------------------------------------------------------
# estimate how many compile passes we need. if upx is enabled, which takes a very long time, multiply it by some factor
# since upx uses a single core

ESTLIBGENCOUNT=$(  subestimate src/lib/libGenerator --no-banner $LIBGENOPTS    $SIXTYFOURBITS $THIRTYTWOBITS  )
ESTLIBDC42COUNT=$( subestimate src/lib/libdc42      --no-banner                $SIXTYFOURBITS $THIRTYTWOBITS  ) 
ESTLIBDC42COUNT=$( subestimate src/lib/libdc42      --no-banner                $SIXTYFOURBITS $THIRTYTWOBITS  ) 
ESTLIBTERMCOUNT=$( subestimate src/lib/TerminalWx   --no-banner                $SIXTYFOURBITS $THIRTYTWOBITS  ) 
[[ -z "$NODC42TOOLS" ]] && ESTTOOLSCOUNT=$( subestimate src/tools  --no-banner $SIXTYFOURBITS $THIRTYTWOBITS  ) || ESTTOOLSCOUNT=0
ESTPHASE1COUNT=$( INEXT=${PHASE1INEXT} OUTEXT=${PHASE1OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiling COUNTNEEDED ${PHASE1LIST} )
ESTPHASE2COUNT=$( INEXT=${PHASE2INEXT} OUTEXT=${PHASE2OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiling COUNTNEEDED ${PHASE2LIST} )

# multiply estimates when using UPX as UPX is very slow
if [[ -z "$WITHOUTUPX" ]] && [[ -n "$UPXCMD" ]]; then
   ESTTOOLSCOUNT=$((  $ESTTOOLSCOUNT  * 10 ))
   ESTPHASE2COUNT=$(( $ESTPHASE2COUNT *  5 ))
fi

# package making times are wildly different, worst case I've seen so far are snapcraft.
[[ -n "$WITHPKG" ]] && ESTPHASE2COUNT=$(( $ESTPHASE2COUNT *  2 ))

ESTIMATETOTALS=$(( $ESTLIBGENCOUNT + $ESTLIBDC42COUNT + $ESTTOOLSCOUNT + $ESTPHASE1COUNT + $ESTPHASE2COUNT + 5 ))

 ESTLIBGENCOUNT=$(( $ESTLIBGENCOUNT  * 100 / $ESTIMATETOTALS ))
ESTLIBDC42COUNT=$(( $ESTLIBDC42COUNT * 100 / $ESTIMATETOTALS ))
  ESTTOOLSCOUNT=$(( $ESTTOOLSCOUNT   * 100 / $ESTIMATETOTALS ))
 ESTPHASE1COUNT=$(( $ESTPHASE1COUNT  * 100 / $ESTIMATETOTALS ))
 ESTPHASE2COUNT=$(( $ESTPHASE2COUNT  * 100 / $ESTIMATETOTALS ))

#---------------------------------------------------------------------------------------------------------------------------
# (echo "Estimates:"
# echo "libgen:   $ESTLIBGENCOUNT"
# echo "libdc42:  $ESTLIBDC42COUNT"
# echo "tools:    $ESTTOOLSCOUNT"
# echo "TermWx:   $ESTLIBTERMCOUNT"
# echo "Phase1:   $ESTPHASE1COUNT"
# echo "Phase2:   $ESTPHASE2COUNT"
# ) 1>&2
# read x
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

export OWARNINGS="$WARNINGS"

export PERCENTPROGRESS=0 PERCENTCEILING=${ESTLIBGENCOUNT}
if [[ $ESTLIBGENCOUNT -gt 0 ]]; then
    export COMPILEPHASE="libGenerator"
    export PERCENTJOB=0 REUSESAVE=""
    subbuild src/lib/libGenerator --no-banner $LIBGENOPTS $SIXTYFOURBITS $THIRTYTWOBITS $EXTRADEFINES $SARCH skipinstall
    unset LIST
fi

export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTLIBGENCOUNT )) 
if [[ $ESTLIBDC42COUNT -gt 0 ]]; then
  export COMPILEPHASE="libdc42"
  export PERCENTJOB=0 REUSESAVE="yes"
  subbuild src/lib/libdc42      --no-banner             $SIXTYFOURBITS $THIRTYTWOBITS $EXTRADEFINES $SARCH skipinstall
  unset LIST
fi

export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTTOOLSCOUNT )) 
if  [[ $ESTTOOLSCOUNT -gt 0 ]]; then
    export COMPILEPHASE="tools"
    export REUSESAVE="yes"
    subbuild src/tools            --no-banner           $SIXTYFOURBITS $THIRTYTWOBITS $SARCH $EXTRADEFINES
    unset LIST
fi

export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTLIBTERMCOUNT )) 
if  [[ $ESTLIBTERMCOUNT -gt 0 ]]; then
    export COMPILEPHASE="TerminalWx"
    export REUSESAVE="yes"
    subbuild src/lib/TerminalWx  --no-banner             $SIXTYFOURBITS $THIRTYTWOBITS $SARCH $EXTRADEFINES
    unset LIST
fi

export WARNINGS="$OWARNINGS"
unset OWARNINGS

echo "* Building ${SOFTWARE}..."
echo
echo "* ${SOFTWARE} C Code                (./lisa)"


# Compile C
export COMPILEPHASE="C code"

export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTPHASE1COUNT ))
export PERCENTJOB=0 NUMJOBSINPHASE=15
export COMPILECOMMAND="$CC -W $WARNINGS -Wstrict-prototypes -Wno-format -Wno-unused $WITHDEBUG $WITHTRACE $CFLAGS $ARCH $INC -c :INFILE:.c -o :OUTFILE:.o"
LIST1=$(WAIT="" INEXT=${PHASE1INEXT} OUTEXT=${PHASE1OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiled COMPILELIST ${PHASE1LIST} )

EXPECTED_COUNT=$(echo "$PHASE1LIST" | wc -w)
ACTUAL_COUNT=$(echo "$LIST1" | wc -w)
if [[ "$EXPECTED_COUNT" -ne "$ACTUAL_COUNT" ]]; then
    echo "Stopping due to failure..." 1>&2
    exit 12
fi

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
  qjob "!!  Compiled lisaem_static_resources.cpp " $CXX $ARCH $CXXFLAGS $ARCH -c lisaem_static_resources.cpp -o ${TLD}/obj/lisaem_static_resources.o 1>&2
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
OWARNINGS="$WARNINGS"
export WARNINGS="$WARNINGS $NOIGNOREDQUALIFIERS $NODEPRECATED"

# Compile C++
cd "${TLD}"
export COMPILEPHASE="C++ code"
export PERCENTPROGRESS=${PERCENTCEILING}
export PERCENTCEILING=$(( $PERCENTPROGRESS + $ESTPHASE1COUNT ))
export PERCENTJOB=0 NUMJOBSINPHASE=6
CXXFLAGS="$CXXFLAGS -I src/include -I resources"
export COMPILECOMMAND="$CXX -W -Wno-write-strings $WARNINGS $WITHDEBUG $WITHTRACE $WITHBLITS $INC $CXXFLAGS $ARCH -c :INFILE:.cpp -o :OUTFILE:.o "
LIST=$( WAIT="yes" INEXT=${PHASE2INEXT} OUTEXT=${PHASE2OUTEXT} OBJDIR=${PHASE2OBJDIR} VERB=Compiled COMPILELIST ${PHASE2LIST} )
waitqall


# restore warnings
[ -n "$OWARNINGS" ] && export WARNINGS="$OWARNINGS"

for i in `echo $LIST`; do WXLIST="$WXLIST `echo $i|grep -v lisaem_wx`"; done

echo
echo '---------------------------------------------------------------' >> $BUILDWARNINGS
waitqall
cd "${TLD}"

#(
#    echo "PHASE1LIST: $PHASE1LIST"
#    echo "PHASE2LIST: $PHASE2LIST"
#    echo "LIST: $LIST"
#    echo "LIST1: $LIST1"
#    echo "WXLIST: $WXLIST"
#) >/tmp/slot.lists.txt

if [[ $(echo "$PHASE2LIST" | wc -w ) -ne  $(echo "$LIST" | wc -w )  ]]; then
    echo "Stopping due to failure..." 1>&2
    exit 12
fi

export COMPILEPHASE="linking"
export PERCENTPROCESS=97 PERCENTCEILING=98 PERCENTJOB=0 NUMJOBSINPHASE=1
update_progress_bar $PERCENTPROCESS $PERCENTJOB $NUMJOBSINPHASE $PERCENTCEILING
waitqall
qjob  "!!* Linked ./bin/${LISANAME}" $CXX $ARCH $GUIAPP $GCCSTATIC $WITHTRACE $WITHDEBUG -o bin/$LISANAME  $LIST1 $LIST src/lib/libGenerator/lib/libGenerator.a src/lib/TerminalWx/lib/terminalwx.a \
      src/lib/libdc42/lib/libdc42.a  $LINKOPTS $SYSLIBS $LIBS
waitqall

export COMPILEPHASE="pack/install"
export PERCENTPROCESS=98 PERCENTCEILING=100 PERCENTJOB=0 NUMJOBSINPHASE=1
update_progress_bar $PERCENTPROCESS $PERCENTJOB $NUMJOBSINPHASE $PERCENTCEILING

cd "${TLD}/bin"

if  [[ -f "$LISANAME" ]]; then

    strip_and_compress "${LISANAME}"

        #.
        #└── Contents                                ${TLD}/bin/${SOFTWARE}.app/Contents
        #    ├── Info.plist
        #    ├── MacOS                               ${TLD}/bin/${SOFTWARE}.app/Contents/MacOS
        #    │   └── ${SOFTWARE}
        #    ├── PkgInfo
        #    └── Resources                           ${TLD}/bin/${SOFTWARE}.app/Contents/Resources
        #        └── skins                           
        #            └── default
        #                ├── default.conf
        #                ├── floppy0.png
        #                ├── floppy1.png

    if [[ -n "$DARWIN" ]]; then
        echo "* Creating macos application" 1>&2
        # replace machine type if we overode it
        if [[ -n "$ARCHOVERRIDE" ]]; then
           export BINARYEXTENSION="-${ARCHOVERRIDE}-${OSMAJOR}.${OSMIDDLE}"
        fi
        CONTENTS="${TLD}/bin/${SOFTWARE}.app/Contents/"
        RESOURCES="${TLD}/bin/${SOFTWARE}.app/Contents/Resources"
        BIN="${TLD}/bin/${SOFTWARE}.app/Contents/MacOS"

        PLISTSRC="${TLD}/resources/Info.plist"
        ICONSRC="${TLD}/resources/${SOFTWARE}.icns"
        mkdir -pm775 "${BIN}" "${RESOURCES}"
        cp "${TLD}/resources/lisaem.sh"               "${BIN}/lisaem.sh"                               || exit $?
        chmod 755                                     "${BIN}/lisaem.sh"                               || exit $?

        mv "${TLD}/bin/${LISANAME}"                   "${BIN}/lisaem${BINARYEXTENSION}-wx${WXVERSION}" || exit $?
        sed -e "s/_VERSION_/$VERSION/g" \
            -e "s/_MINMACOS_/$MACOSX_MAJOR_VER/g"   < "${PLISTSRC}"> "${CONTENTS}/Info.plist"          || exit $?

        echo -n 'APPL????'                          > "${CONTENTS}/PkgInfo"                            || exit $?
        cp "${ICONSRC}"                               "${RESOURCES}"                                   || exit $?

        (cd "${TLD}/resources";tar cpf - skins) | (cd "${RESOURCES}"; tar xpf - )
        x=$?
        if  [[ "$x" -ne 0 ]]; then
            echo "Failed to copy ${TLD}/resources/skins to ${RESOURCES}" 1>&2
            exit $x
        fi
        (cd "${RESOURCES}"; rm -f resources; ln -sf . resources 2>/dev/null ) # fix cross-os path issue with a lame ass link.
        LISANAME="${BIN}/lisaem${BINARYEXTENSION}-wx${WXVERSION}"

        if [[ -n "$WITHDEBUG" ]]; then
            echo "run -p" >gdb-run
            echo "Launching debugger:"
            [[ -n "$( echo $GDB | grep gdb )"  ]] && $GDB -x gdb-run     "$LISANAME"
            [[ -n "$( echo $GDB | grep lldb )" ]] && $GDB -o "run -p" -f "$LISANAME"
        fi

        #if we turned on profiling, process the results
        if [[ -n "$WITHPROFILE" ]];then
          $GPROF "${LISANAME}" >lisaem-gprof-out
          echo lisaem-gprof-out created.
        fi

        if [[ -n "$INSTALL" ]]; then
          echo "* Installing ${SOFTWARE}.app" 1>&2
          (cd "${TLD}/bin"; tar cf - ./${SOFTWARE}.app ) | (cd "$PREFIX"; tar xf -)
          x=$?
          if  [[ "$x" -ne 0 ]]; then
              echo "Failed to copy ${TLD}/bin/${SOFTWARE}.app to ${PREFIX}" 1>&2
              exit $x
          fi
          echo "* Done Installing." 1>&2
          elapsed=$(get_elapsed_time)
          [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
          exit 0
        fi

        if [[ -n "$WITHPKG" ]]; then

           cd "${TLD}/bin/${MACOSX_MAJOR_VER}/" || exit $?

           mkdir -pm755 ./pkg/usr/local/bin     || exit $?
           chmod 755      pkg/usr/local pkg/usr || exit $? # fix inner dir perms

           [[ -n "$ARCHOVERRIDE" ]] && MACHINE="$ARCHOVERRIDE"

           TOOLLIST="patchxenix blu-to-dc42  dc42-resize-to-400k  dc42-dumper  lisadiskinfo  lisafsh-tool dc42-copy-boot-loader lisa-serial-info los-bozo-on los-deserialize idefile-to-dc42 rraw-to-dc42"
           mv ${TOOLLIST} "${TLD}/bin/${MACOSX_MAJOR_VER}/pkg/usr/local/bin/" || exit $?

           # create a tools only package
           macospkg "net.sunder.lisaem" "../../pkg/lisaem-cli-tools-macos-${MACOSX_MAJOR_VER}-${MACHINE}.pkg"
           mkdir -pm755     ./pkg/Applications || exit $?
           mv ../${SOFTWARE}.app ./pkg/Applications || exit $?

           # create a LisaEm package
           macospkg "net.sunder.lisaem" "../../pkg/lisaem-macos-${MACOSX_MAJOR_VER}-${MACHINE}.pkg"
           # put things back and clean up
           mv pkg/usr/local/bin/*  "${TLD}/bin/${MACOSX_MAJOR_VER}/"
           mv pkg/Applications/*   "${TLD}/bin"

           rmdir ${TLD}/bin/${MACOSX_MAJOR_VER}/pkg/usr/local/bin  ${TLD}/bin/${MACOSX_MAJOR_VER}/pkg/usr/local \
                 ${TLD}/bin/${MACOSX_MAJOR_VER}/pkg/usr  ${TLD}/bin/${MACOSX_MAJOR_VER}/pkg/Applications \
                 ${TLD}/bin/${MACOSX_MAJOR_VER}/pkg
        fi #end of macos package

        echo "Done." 1>&2
        exit 0
    fi #end of DARWIN/macos build/package/etc.
     
    echo
    ####
    
    if [[ -z "$WITHDEBUG" ]]; then
    
      ## Install ###################################################
      if [[ -n "$INSTALL" ]]; then
  
        if [[ -n "$CYGWIN" ]]; then
              # PREFIX will go to: "/cygdrive/c/Program Files/${COMPANY}/${SOFTWARE}"
              # it is set in the bashbuild/Windows.sys file
              ACTION="Installing"

              if  [[ -n "$WITHPKG" ]]; then
                  set_pkg_prefix
                  ACTION="Packaging"
              fi

              echo "Creating folder ${PREFIX}"
              mkdir -pm755 "${PREFIX}"
              echo "* ${ACTION} skins into ${PREFIX}"
              cp -r "${XTLD}/resources/skins" "${PREFIX}"			  
              echo "* ${ACTION} lisaem.exe and tools binaries into ${PREFIX}"
              mkdir -pm755 "${PREFIX}/bin"
              cp "${XTLD}/bin/"*.exe "${PREFIX}/bin"
              mv "${PREFIX}/bin/lisaem.exe" "${PREFIX}"
              echo "  Done ${ACTION} to ${PREFIX}"

              create_packages_for_os

              exit 0
        else   # Install for linux/freebsd/sunos, etc.

              ACTION="Installing"

              if  [[ -n "$WITHPKG" ]]; then
                  export PREFIX="${XTLD}/pkg/build/tmp/${COMPANY}/${SOFTWARE}/$PREFIX"
                  export PREFIXLIB="${XTLD}/pkg/build/tmp/${COMPANY}/${SOFTWARE}/$PREFIXLIB"
                  ACTION="Packaging"
              fi

              # PREFIX="/usr/local/bin" PREFIXLIB="/usr/local/share/" - these are set in bashbuild/${OS}.sys file
              echo "* ${ACTION} resources in     $PREFIXLIB/lisaem" 1>&2
              mkdir -pm755 $PREFIXLIB/lisaem $PREFIX
              cp -r ../resources/skins "$PREFIXLIB/lisaem/"

              # create dirs incase path doesn't exist - i.e. target is in pkg/build/tmp/${COMPANY}/${SOFTWARE}
              mkdir -pm755 "$PREFIXLIB/icons/hicolor/128x128/apps" 2>/dev/null
              mkdir -pm755 "$PREFIXLIB/applications"               2>/dev/null 

              # copy icon + GNOME desktop file
              cp ../resources/lisaem.png     "$PREFIXLIB/icons/hicolor/128x128/apps"
              cp ../resources/lisaem.desktop "$PREFIXLIB/applications/"

              # copy all our binaries, but skip hashes.txt, bin link, and the interim build.sh link
              (cd ../bin; for f in $(ls -1 | egrep -v 'build.sh|bin|hashes.txt'); do cp "$f" "${PREFIX}"; done)

              echo -n "  Done ${ACTION}." 1>&2
              elapsed=$(get_elapsed_time) && [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo

              create_packages_for_os

              elapsed=$(get_elapsed_time) && [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
              exit 0
        fi # cygwin vs linux/freebsd/solaris install
      fi #end of INSTALLa

      ##########################################################
    
    else # debug builds should invoke GDB/lldb
      elapsed=$(get_elapsed_time)
      [[ -n "$elapsed" ]] && echo "$elapsed seconds" || echo
    
      if [[ -z "$CYGWIN" ]]; then # debugger invocation not cygwin
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
      else # debug under cygwin
        cp lisaem.exe "/cygdrive/c/Program Files/${COMPANY}/${SOFTWARE}" || exit 1
        cd ../resources
        cp -r "skins" "${PREFIX}"
        cd "/cygdrive/c/Program Files/${COMPANY}/${SOFTWARE}" || exit 1
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
