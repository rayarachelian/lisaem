

### Copyright © MMXX by Ray Arachelian, All Rights Reserved. 
### Released under the terms of the GNU Public License v3.

The logo for BashBuild comes from https://github.com/odb/official-bash-logo/ Copyright © 2016 ol' dirty bashnerds and is subjet to the MIT License.

***
# BashBuild Scripts

## What is this thing?
This is a side-side-project tangential to LisaEm. When I first started writing LisaEm I needed a simple, universal build system that would work on everything from Solaris and other Sys V unix machines, *BSD, Linux, and Windows machines.

It provides parallel compiles, library making, progress bar and banner image displaying (for terminals that support such functions as long as you've installed the appropriate tools, i.e. the tools for Iterm2, terminology, or KiTTY.)

## Theory

To build modern, portable C/C++ (but not limited to those) software, you need a build system that can:

 * run on multiple operating systems
 * compiles your code into binaries and libraries in parallel, but also with stages so inter-dependencies can be handled.
 * strip off the debugging symbols when you're not debugging. possibly invoke the debugger if you are.
 * optionally create various kinds packages, depending on the OS and available tools.

Additionally, bashbuild also gives you the following:

 * detects the operating system and lets you use code to detect the compiler and enable options
 * collects include and library directories and passes them to the compiler/linker
 * automatically compiles in parallel by default (without requiring `make -j $(nproc)` ) while still aborting on errors, collecting all warning messages to a single file without cluttering the output, so you can examine and correct them later.
 * optionally compress your binaries (using upx if your system has it)
 * detects the sizes of integer types and defines them to bit size, i.e. uint8, uint16, uint32 vs char, short, int
 * do the build, make, make install steps in one single command line invocation.
 * Not having to learn separate Domain Specific Languages (automake, autoconf, CMake, make)
 * Allows you to build sub builds to compile libraries and tools tangent or dependent by your application
 * While you're traversing various directories to edit your code or examine contents, you can run `./build.sh` and if it needs to, it will go up a level or many, to recompile that code


## What's needed to do away with autoconf/make/cmake/make

To achieve this, we need a build system that can 

 * find the include files and libraries, and pass them to your code, and also pass your own libraries and include paths, and then invoke the compiler.
 * detect when the dependencies of a target binary have changed, and whether they're newer than that dependency so it can decide what to compile.


## What it doesn't do

bashbuild isn't going to find what functions are defined in which include file or find out if some function is supported or not, because at the end of the day that just translates into a `#define` or a `-D FEATURE_FLAG` anwyay, and is very time consuming, and usually these things are easily detected by virtue of knowing what OS you're running on.

Literally every person who runs the same OS and same OS version as you has to waste minutes runinng ./configure while it goes off and tries to compile a bunch of test programs to detect what features are available. A better way is simply to know what operating system you're running on and use that information to decide. If your app has optionals, you can detect them yourself by querrying the OS or file system from within your build script.

## History

When I started on LisaEm, the the only game in town was automake, which IMO is a terrible Domain Specific Language, and not wanting to learn yet another DSL, especially one as convoluted as automake, I was considering rolling my own. But the realization was that it was unncessary as almosst every system has a shell interpreter built in. While Unix System V systems such as AIX, Solaris, and the like came with sh, csh, and ksh only, bash was available, and almost all default to bash these days, and those that don't have it as an option. (zsh did not yet exist.)

Even then, thinking about it, if I was to use autoconf, autoconf generates a `./configure` script from `configure.in` after which you're supposed to run `make` and then again `sudo make install`. So it's a multi step process, and each step is somewhat manual. As Morty would say, "isn't that just make with extra steps?" and certainly it is!

It makes sense that you want to catch error messages early on, for example, unmet dependencies and such, but that doesn't really imply that you should have to manually run 4 steps one at a time and wait for each step (well 3 as usually configure is provided by the developer as the output of autoconf.)

Why though? Why not put them all together in the same script and do away with automake as well and the separate extra manual steps. Generally almost everyone types in `./configure && make && sudo make install` anyway, so all this has done is caused people to type in a whole bunch of stuff they didn't need to in the first place.

Now with bashbuild, you could just do `sudo ./build.sh install` and it will do the whole thing for you end to end. Or if you don't need to install, you can just do `./build.sh` and you're done. And this follows the original unix theory. (this is why we us `ls` and not `catalog` to see what files we have in the current directory, for example.)

Now that it's 2020, the only systems that I've come across that done come with bash out of the box as the default shell are FreeBSD, but even there, it is available, even if you have to build it yourself from the ports tree, or install it as a package.

While it appears that macos might deprecate bash in favor of zsh in the future, bash will still be available via homebrew, and I likely intend to use homebrew cask packages for future macos releases. I'm unsure if all the bashisms here would work under zsh and as zsh isn't yet as universal as bash (yes, at one time bash was a 3rd party package too - i.e. Solaris 2-10, AIX, etc.), I prefer to invoke bash for now. In the future, I may either support zsh directly or allow these to work with zsh. We'll see.

I'm not against make files (thought the choice of tab is controversial), and if it wasn't for the Value Added Reseller UNIX wars of the 1990s, all software would be portable to all systems with just a simple Makefile, and you wouldn't need to worry about where a specific function was listed in which include file, or where a library could be found on the file system. 

Appreciably POSIX attempted to fix this, but in the end it just complicated things and now you have the Linux way, the BSD way, the macos way and the POSIX way. Meh!

The build script provided with LisaEm upto 2010 was the result of the first pass of that effort, however circa 2010 I created LisaEm 1.3.0 and revamped the build script into a set of library function, the end result is now called BASHBUILD.

It provided parallel builds on systems with multicore processors via job control, and the ability to build packages and libraries. Unfortunately LisaEm 1.3.0 suffered from "Second System Syndrome" and has been scrapped (for now?) because that CPU core, while modular and better suited for an MMU than the libGenerator core in LisaEm 1.2.x and earlier does not function properly. I may still resurrect it at some point, time permitting.

Fast foward to November 2019, and I've decided to start reusing the LisaEm 1.3.0 version of the build script with LisaEm 1.2.7, and so I set about to clean it up even further and turn it into its own project, and this is the result of that effort.

## Using BashBuild for your own project

This documentation is a work in progress and I'll have to come back and clean it up later, but for now the example builds of LisaEm as well as these notes should help you get going with your own builds if you wish to use it.

This directory keeps the bash script library functions used by the build system. You should `source bashbuild/src.build` from your build script as shown in the block below.

Files in this directories are broken up into sub modules.

Sub modules work as follows, files ending in .sys are bash scripts that set variables for a specific OS, the OS name has to be returned by `uname -s` and must match files with that prefix, including case. These are used to set the compiler flags for your OS. If your OS can possibly use more than one compiler, i.e. both clag and gcc, you should either pass `CC=clag CXX=clang CPP=clang` when building, or modify the ${OS}*.sys file to detect the compiler, and decide which to use, and then set appropriate flags as required.

If your OS returns weird names with versions, *cough cough* i.e. `CYGWIN_NT-10.0`, i.e. not just OS but also version, then src.build will check for this only for CYGWIN, but you may need to change that code in `src.build` for others.

As I use Cygwin as a prefrontal-cortex to Windows, yes I know there is now an Ubuntu subsystem in the pro and enterprise versions of Windows, but I need a way to signal that you intend to build for Windows, so I'm making the assumption that if you're building via Cygwin, you intend to build a Windows application. If this is not the case, and you intend to build a Cygwin app, please make the required and obvious changes in `src.build` to allow the call to source the `CYGWIN.sys` OS include rather than `Windows.sys`

The OS specific scripts should have logic that detects the architecture, provide a `makelib` function in order to build libraries for that OS, and so on, `getcpus` function, package building functions as well as any other OS specific thing needed. Further, they should detect the compiler being used and set parameters for it. They should also distinguish between versions of the OS and deal with that. (ie: modern macos vs early mac OS X 10.4).

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
        VER="0.9.6"                     # the version number
RELEASEDATE="2019.12.23"                # release date.  must be YYYY.MM.DD
     AUTHOR="Ray Arachelian"            # name of the author
  AUTHEMAIL="ray@arachelian.com"        # email address for this software
    COMPANY="Sunder.NET"                # company (vendor for sun pkg)
      CONAM="SUNDERNET"                 # company short name for Solaris pkgs
        URL="http://lisaem.sunder.net"  # url to website of package
COPYRIGHTYEAR="2019"
COPYRIGHTLINE="Copyright (C) ${COPYRIGHTYEAR} Ray Arachelian, All Rights Reserved"
# ----------------------------------------------------------------------------------------
# vars auto built from the above.
VERSION="${VER}_${RELEASEDATE}"
# copy the arguements given to us as they'll be used again when we make packages
BUILDARGS="$0 $@"
export VER RELEASEDATE AUTHOR SOFTWARE LCNAME DESCRIPTION COMPANY CONAM URL VERSION BUILDARGS 

#------------------------------------------------------------------------------------------#
# end of standard section for all build scripts.
#------------------------------------------------------------------------------------------#

```
(Obviously you should change the parameters in the block above and fill in your software's details.)

There are some critical variables, TLD is the Top Level Directory path for the top-most build script. XTLD is added when `src.build` is sourced. Sub builds can have different TLD's with XTLDs (XTLD is the local one relative to the subbuild) However if you manually descend into a subbuild folder and run build yourself `[[ "$TLD" == "$XTLD" ]]` will be true.

Sub builds are sourced rather than executed. This is faster because we don't need to reload all the functions all over again, and detect the OS and so on, and the functions that implement subbuild will save the local environment and then run the subbuild for you. This means that any build.sh script that's intended as a subbuild (i.e. for creating libraries or tools) must not execute exit (except when errors are encountered) or compilation will stop prematurely.

(Pro tip: `source script.sh a b c` will set $1 to 'a' $2 to 'b', and $3 to 'c', so you can pass parameters to a sourced script and it will behave like a normal script!)
 
You should estimate the percentage of how many compiled files each subbuild will take and create a basepercent at each compile phase, this will allow the progress bar to display human friendly results, rather than anger the user by having it jump to 90% in the first few seconds and then take a long time for the last 10% like Windows ME used to do. :)  Yes, I'm bad at this too.

And likely you'll want to call the progressbar function fairly often to indicate status.

The `qjob` and `compilelist` functions can be used to compile code in parallel. The `makelib` function will create libraries. It's your job to maintain and properly invoke the compiler on your system and pass the proper library names and include paths. There are comments on how to use them in the function headers of each function. These interfaces may change over time, but I'll eventually document them and stabilize their interfaces.

The build script will walk the directory tree and collect anything with the name of "include" as a directory, and anything with the name of "lib" and pass them all to the `LIB` and `INCLUDE` variables ahead of time.

There is a `wx-config.fn` script in the bashbuild directory, which should be ommitted if your project does not use wxWidgets, but it also seeds the include files, C/C++ flags, and libraries.

The `checkdirs` and `checkfiles` functions can be used to ensure that your package contains all the files that it needs. You should call them from the top of your script after displaying a banner, but before parsing any command line options.

Your `build.sh` script is responsible for parsing any command line options, deciding what phases to compile your code in and how to display the progress bar, and when to call subbuilds or make packages and anything else, including cleaning your code. Follow the examples in LisaEm when in doubt.


## Directory Tree Map

Each bashbuild package should contain a `./build.sh` script, the `bashbuild` directory (or a link to the TLD containing it), a src directory for source code, an obj directory for objects and preliminary binaries such as `get-uintX-types` used to detect sizes of the machine's integers and generate machine.h inside of `src/include`, a `bin` directory for the resulting binaries, a `pkg` directory for creating/staging OS specific packages, such as `.rpm`s inside of. 

The rest is package dependent. For example LisaEm 1.2.7+ is organized like the hardware of an actual Lisa and broken up into different boards and devices, some of the dirs are just stubs. Some issues are related to the name of the app on different operating systems and the storage of those on case sensitive vs case insensitive file systems (HFS+, APFS, NTFS vs ext4, zfs, etc.).

The "scripts" directory is for LisaEm specific scripts such as how to build wxWidgets properly, and not part of the `bashbuild` system, as are the resource dirs specific to LisaEm and the storage of skins/sounds.

Some of the subbuilds such as libGenerator and libdc42 produce libraries and so they don't have a bin directory.

As compile jobs are queued to run in parallel, it's difficult to display error messages and compiler warnings, instead these are collected and saved under `${TLD}/obj/build-warnings.txt` for inspection either when a job causes the build to abort, or for when warnings can be examined at the end of the build cycle.

The tools sub build has a bin that's softlinked to LisaEm's bin. Similar things can be done with obj and lib, if you so wish.
The bashbuild in sub builds are softlinks to the top level to remove duplication.

```
.
├── bashbuild
├── bin
├── lisaem -> resources
├── obj
├── pkg
│   ├── build
│   └── templates
├── resources
│   └── skins
│       └── default
├── scripts
├── share
│   └── lisaem -> ../resources
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
    │       └── src
    │           └── include
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
        ├── bin -> ../../bin
        ├── include
        ├── obj
        ├── resources -> ../lib/libdc42/resources/
        └── src
            └── lib
                └── libdc42 -> ../../../lib/libdc42
```

















