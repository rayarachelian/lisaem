![BASHBUILD Logo](bashbuild-banner.png)

###Copyright © 2019 by Ray Arachelian, All Rights Reserved. 
###Released under the terms of the GNU Public License v3.

The logo for BashBuild comes from https://github.com/odb/official-bash-logo/ Copyright © 2016 ol' dirty bashnerds and is subjet to the MIT License.


***
# BASHBUILD Scripts

## What is this thing?
This is a side-side-project tangential to LisaEm. When I first started writing LisaEm I needed a simple, universal build system that would work on everything from Solaris and other Sys V unix machines, *BSD, Linux, and Windows machines.

It provides parallel compiles, library making, progress bar and banner image displaying (for terminals that support such functions as long as you've installed the appropriate tools, i.e. the tools for Iterm2 or use KiTTY.)

## History

At the time (~2007), the only game in town was automake, which IMO is a terrible domain specific language, and not wanting to learn yet another DSL, especially one as convoluted as automake, I was considering rolling my own, but then realized all of these systems do have BASH available to them, even if it's a third party package. (In the case of Windows, Cygwin can provide BASH for all flavors of Windows). 

Even then autoconf generates a `./configure` script after which you're supposed to run make. Why though? Why not put them all together in the same script and do away with automake as well?

CMake didn't exist yet, and now at this point I'm not likely to switch to it, though I might be convinced if someone else bothers to move LisaEm to it. But even then you need to have CMake installed and then at the end, it generates a make file.

I'm not against make files, and if it wasn't for the Value Added Reseller UNIX wars of the 1990s, all software would be portable to all systems with just a simple Makefile, and you wouldn't need to worry about where a specific function was listed in which include file, or where a library could be found on the file system, but if you're going to rely on shell scripts as part of  your build process, i.e. `./configure` why have so many other steps and packages involved? Why not directly use pure bash to handle all these steps and do it in a single invocation?

While it appears that macos X might deprecate bash in favor of zsh in the future, but bash will still be available via homebrew, and I likely intend to use homebrew cash packages for future macos releases. I'm unsure if all the bashisms here would work under zsh and as zsh isn't yet as universal as bash (yes, at one time bash was a 3rd party package too - i.e. Solaris 2-10, AIX, etc.), I prefer to invoke bash.

Back to 2007, I needed one single ability that make had, which is to compare a source file's time stamp to it's corresponding object file, and if either the object file wasn't present or was older than the source, to build that C source file into an object or executable. Back then the test command didn't always have the -nt option, everywhere so I used `ls -ltr | tail -1` to see if an object was older than its corresponding source file, but now even ancient macos 10.4 supports `[[ file1 -nt file2 ]]`

The build script provided with LisaEm upto 2010 was the result of the first pass of that effort, however circa 2010 I created LisaEm 1.3.0 and revamped the build script into a set of library function, the end result is now called BASHBUILD.

It provided parallel builds on systems with multicore processors via job control, and the ability to build packages and libraries. Unfortunately LisaEm 1.3.0 suffered from "Second System Syndrome" and has been scrapped (for now?) because that CPU core, while modular and better suited for an MMU than the libGenerator core in LisaEm 1.2.x and earlier does not function properly. I may still resurrect it at some point, time permitting.

Fast foward to November 2019, and I've decided to start reusing the LisaEm 1.3.0 version of the build script with LisaEm 1.2.7, and so I set about to clean it up even further and turn it into its own project, and this is the result of that effort.

## How BASHBUILD works

This documentation is a work in progress and I'll have to come back and clean it up later, but for now the example builds of LisaEm as well as these notes should help you get going with your own builds if you wish to use it.

This directory keeps the bash script library functions used by the build system. You should source bashbuild/src.build from your build script as shown in the block below.

Files in this directories are broken up into sub modules.

Sub modules work as follows, files ending in .sys are bash scripts that set variables for a specific OS, the OS name has to be returned by `uname -s` and must match files with that prefix, including case. These are used to set the compiler flags for your OS. If your OS can possibly use more than one compiler, i.e. both clag and gcc, you should either pass `CC=clag CXX=clang CPP=clang` when building, or modify the ${OS}*.sys file to detect the compiler, and decide which to use, and then set appropriate flags as required.

If your OS returns weird names with versions, *cough cough* i.e. `CYGWIN_NT-10.0`, then src.build will check for this only for CYGWIN, but you may need to change that code in `src.build` for others.

As I use Cygwin as a prefrontal-cortex to Windows, yes I know there is now an Ubuntu subsystem in the pro and enterprise versions of Windows, but I need a way to signal that you intend to build for Windows, so I'm making the assumption that if you're building via Cygwin, you intend to build a Windows application. If this is not the case, and you intend to build a Cygwin app, please make the required and obvious changes in `src.build` to allow the call to source the `CYGWIN.sys` OS include rather than `Windows.sys`

The OS specific scripts should have logic that detects the architecture, provide a MAKELIB function in order to build libraries for that OS, and so on, and package building functions should be stored there.

If you need to run scripts before the OS is detected and the main functions load, name them `pre-{something}.fn`

If you want to run scripts after the main functions, name them `post-{something}.fn` I use one of these to collect
output from `wx-config --libs` and `wx-config --cflags` for example.

To define your own functions to be sourced in and called by the main build script, suffix them with `.fn`

The scripts will be sourced in alpha-numeric sort order in honor of UNIX System V init scripts, but they are staged into pre, system, fn, and post rather than rc0.d-rc6.d.

The build system is holographic in nature (or more acurately, recursive), that is you can have a top level build script which in turn calls upon other builds inside subdirectories. For example for LisaEm, we depend on two libraries, libDC42 and libGenerator. Each sub build directory has its own bashbuild source directory, though in reality to prevent duplication, softlinks are made to the TLD of the project. (Hopefully git won't break them.)

Further, there's a set of DC42 Disk Image tools that also depend on the libDC42 library. Each of those things, the two libraries and the tools, as well as LisaEm itself are all build from BashBuild scripts, and the top level script for LisaEm calls the ones inside subdirectories.

This allows for a very powerful build system where each subdirectory is potentially its own project. You can run build in either just a subdirectory that contains a library to build/debug that library, or build the whole "world" from the top level.

And even though the `libdc42` library's sourced code is further up the tree from tools, by using softlinks we can have it as a subbuild of tools when needed. (See the tree map below.)

Inside interim directories, `build.sh` scripts that go up a level and run the same build command are provided so you should be able to run `build.sh` from any directory and have it build everything underneath your CWD. They look like this:

```
#!/usr/bin/env bash
cd ..
source ./build.sh $@
```

These scripts also provide for package creation, though as of this writing, no all package types are supported though some work was done for Solaris and RPMs.


## Standard BASHBUILD start codeblock
There is a block of code at the start of every `./build.sh` that needs to be there and cannot be included as it itself does the include of the library functions.

```
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
# these variables.  These will help populate the appropriate fields in packages.
###########################################################################################
   SOFTWARE="LisaEm"                    # name of the software (can contain upper case)
     LCNAME="lisaem"                    # lower case name used for the directory
DESCRIPTION="The Apple Lisa Emulator"   # description of the package
        VER="1.2.7"                     # just the version number
  STABILITY="ALPHA"                     # DEVELOP,ALPHA, BETA, RC1, RC2, RC3... RELEASE
RELEASEDATE="2019.11.24"                # release date.  must be YYYY.MM.DD
     AUTHOR="Ray Arachelian"            # name of the author
  AUTHEMAIL="ray@arachelian.com"        # email address for this software
    COMPANY="Sunder.NET"                # company (vendor for sun pkg)
      CONAM="SUNDERNET"                 # company short name for Solaris pkgs
        URL="http://lisaem.sunder.net"  # url to website of package
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

```
(Obviously you should change the parameters in the block above and fill in your software's details.)

There are some critical variables, TLD is the Top Level Directory path for the top-most build script. XTLD is added when `src.build` is sourced. Sub builds can have different TLD's with XTLDs (XTLD is the local one relative to the subbuild) However if you manually descend into a subbuild folder and run build yourself `[[ "$TLD" == "$XTLD" ]]` will be true.

Sub builds are sourced rather than executed. This is faster because we don't need to reload all the functions all over again, and detect the OS and so on, and the functions that implement subbuild will save the local environment and then run the subbuild for you. This means that any build.sh script that's intended as a subbuild (i.e. for creating libraries or tools) must not execute exit (except when errors are encountered) or compilation will stop prematurely.

(Pro tip: `source script.sh a b c` will set $1 to 'a' $2 to 'b', and $3 to 'c', so you can pass parameters to a sourced script and it will behave like a normal script!)
 
You should estimate the percentage of how many compiled files each subbuild will take and create a basepercent at each compile phase, this will allow the progress bar to display human friendly results, rather than anger the user by having it jump to 90% in the first few seconds and then take a long time for the last 10% like Windows ME used to do. :)  And likely you'll want to call the progressbar function fairly often to indicate status.

The `QJOB` and `COMPILELIST` functions can be used to compile code in parallel. The `MAKELIB` function will create libraries. It's your job to maintain and properly invoke the compiler on your system and pass the proper library names and include paths. There are comments on how to use them in the function headers of each function. These interfaces may change over time, but I'll eventually document them and stabilize their interfaces.

The build script will walk the directory tree and collect anything with the name of "include" as a directory, and anything with the name of "lib" and pass them all to the `LIB` and `INCLUDE` variables ahead of time.

There is a `wx-config.fn` script in the bashbuild directory, which should be ommitted if your project does not use wxWidgets, but it also seeds the include files, C/C++ flags, and libraries.

The `CHECKDIRS` and `CHECKFILES` functions can be used to ensure that your package contains all the files that it needs. You should call it from the top of your script after displaying a banner, but before parsing any command line options.

Your `build.sh` script is responsible for parsing any command line options, deciding what phases to compile your code in and how to display the progress bar, and when to call subbuilds or make packages and anything else, including cleaning your code. Follow the examples in LisaEm when in doubt.


## Directory Tree Map

Each bashbuild package should contain a `./build.sh` script, the `bashbuild` directory (or a link to the TLD containing it), a src directory for source code, an obj directory for objects and preliminary binaries such as `get-uintX-types` used to detect sizes of the machine's integers and generate machine.h inside of `src/include`, a `bin` directory for the resulting binaries, a `pkg` directory for creating/staging OS specific packages, such as `.rpm`s inside of. 

The rest is package dependent. For example LisaEm 1.2.7+ is organized like the hardware of an actual Lisa and broken up into different boards and devices, some of the dirs are just stubs. Some issues are related to the name of the app on different operating systems and the storage of those on case sensitive vs case insensitive file systems (HFS+, APFS, NTFS vs ext4, zfs, etc.).

The "scripts" directory is for LisaEm specific scripts such as how to build wxWidgets properly, and not part of the `bashbuild` system, as are the resource dirs specific to LisaEm and the storage of skins/sounds.

Some of the subbuilds such as libGenerator and libdc42 produce libraries and so they don't have a bin directory, but rather output to a lib directory.

As compile jobs are queued to run in parallel, it's difficult to display error messages and compiler warnings, instead these are saved under ${TLD}/obj/build-warnings.txt for inspection either when a job causes the build to abort, or for when warnings can be examined at the end of the build cycle.

```
./build.sh
├── bashbuild
├── bin
├── obj
├── pkg
│   ├── build
│   └── templates
├── resources
│   ├── resources -> ../resources
│   └── skins
│       └── default
├── scripts
├── share
│   ├── lisaem -> ../resources
│   └── LisaEm -> ../resources
└── src
    ├── host
    │   └── wxui
    │       └── include
    ├── include
    │   └── bits
    ├── lib
    │   ├── libdc42
    │   │   ├── bashbuild -> ../../../bashbuild
    │   │   ├── include
    │   │   ├── lib
    │   │   ├── obj
    │   │   ├── resources
    │   │   └── src
    │   │       └── include
    │   └── libGenerator
    │       ├── bashbuild -> ../../../bashbuild
    │       ├── cpu68k
    │       ├── generator
    │       ├── include
    │       ├── lib
    │       ├── obj
    │       ├── resources
    │       ├── src
    │       │   └── include
    │       └── tester
    │           ├── bashbuild -> ../../../../bashbuild/
    │           ├── libGenerator -> ../../libGenerator
    │           └── sample
    ├── lisa
    │   ├── cpu_board
    │   ├── crt
    │   │   └── hqx
    │   │       └── include
    │   ├── expansion_slots
    │   ├── floppy
    │   ├── io_board
    │   ├── keyboard
    │   │   └── include
    │   ├── memory
    │   ├── motherboard
    │   ├── power_supply
    │   └── video_board
    ├── lisaem -> ../resources
    ├── LisaEm -> ../resources
    ├── printer
    │   └── imagewriter
    │       └── include
    ├── storage
    └── tools
        ├── bashbuild -> ../../bashbuild
        ├── bin
        ├── include
        ├── obj
        ├── resources -> ../lib/libdc42/resources/
        └── src
            └── lib
                └── libdc42 -> ../../../lib/libdc42
```

















