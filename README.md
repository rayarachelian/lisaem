# LisaEm - An Apple Lisa Emulator

[![Build (Ubuntu 24.04)](https://github.com/arcanebyte/lisaem/actions/workflows/workflow.yaml/badge.svg?branch=master)](https://github.com/arcanebyte/lisaem/actions/workflows/workflow.yaml)

Copyright © 1998-2022 by Ray Arachelian, All Rights Reserved.  
Copyright © 2023-2026 by Friends of Ray Arachelian.  
Released under the terms of the [GNU Public License v3](https://www.gnu.org/licenses/gpl-3.0.txt).

------------------------------------------------------------------------------
# Helpful Links
 
* [https://lisaem.sunder.net/](https://lisaem.sunder.net/)
* [https://lisafaq.sunder.net/](https://lisafaq.sunder.net/)
* [https://lisalist2.com](https://lisalist2.com)
* [https://github.com/arcanebyte/lisaem](https://github.com/arcanebyte/lisaem)

------------------------------------------------------------------------------

## What is this thing?

You're looking at the repository for the source code for the Lisa Emulator Project (LisaEm). 

LisaEm is a emulator that runs on a wide variety of modern and somewhat old systems by means of the wxWidgets framework, implementing an emulation of the (in)famous Apple Lisa Computer. The Apple Lisa computer is the predecessor of the Mac and the "inspiration" of many GUI environments from Windows 1.x to GEOS, GEM, VisiOn, etc. 

Many of the original ideas of the modern GUI, including copy and paste, were first envisioned in the Apple Lisa, so it's a historically very important machine. The seeds of these ideas were first found in the Xerox Alto, but the Lisa took them to their next logical step.

This document contains a few brief updates that are also found in the ChangeLog file, as well as some information on how to compile wxWidgets and LisaEm.

## Command Line Options

LisaEm accepts the following command line options which can be used to customize it in various situations such as running in a Kiosk mode on a Raspberry Pi inside a 3D printed Apple Lisa case, or for an automation pipeline:

```
Usage: lisaem [-h] [-p] [-q] [-f <str>] [-d] [-F[-]] [-z <double>] [-s[-]] [-c <str>] [-k] [-o[-]]
  -h, --help            show this help message
  -p, --power           power on as soon as LisaEm is launched
  -q, --quit            quit after Lisa shuts down
  -f, --floppy=<str>    boot from which floppy image ROMless only
  -d, --drive           boot from motherboard ProFile/Widget ROMless only
  -F, --fullscreen      fullscreen mode (-F- to turn off)
  -z, --zoom=<double>   set zoom level (0.50, 0.75, 1.0, 1.25,... 3.0)
  -s, --skin            turn on skin (-s- or --skin-- to turn off)
  -c, --config=<str>    Open which lisaem config file
  -k, --kiosk           kiosk mode (suitable for RPi Lisa case)
  -o, --originctr       skinless mode: center video(-o) vs topleft(-o-)
```

## Tested Host Operating Systems

The following table outlines successful builds against the respective operating systems, architectures, and wxWidget versions:

| OS | Arch | wxWidgets | Status |
|---|---|---|---|
| macOS 13.4.1 | arm64 | 3.2.3 | Tested - OK |
| macOS 13.6 | x86_64 | 3.2.3 | Tested - OK |
| macOS 14.0 | arm64 | 3.2.4 | Tested - OK |
| macOS 14.5 | arm64 | 3.2.5 | Tested - OK |
| macOS 15.1.1 | arm64 | 3.2.6 | Tested - OK |
| macOS 15.6.1 | arm64 | 3.2.9 | Tested - OK |
| Windows 10 | x86_64 | N/A | Untested |
| Ubuntu 22.04 | x86_64 | N/A | Untested |

To sustain development efforts, compatibility with legacy architectures (ie. PowerPC) and operating systems (ie. MacOS X < 10.12)
is not guaranteed and support may be removed.

## Tested Emulated Operating Systems

Below is a table of recently tested emulated operating systems. If you've successfully installed something listed (or not listed) here, please email or leave a note.

| OS | Status | Notes |
|---|---|---|
| MacWorks XL 3.0 | Tested - OK | Hard disk support functional. Requires MW boot disk. |
| MacWorks Plus 1.0.14 | Tested - Issues | Issues as of RC4 |
| MacWorks Plus 10.0.18 | Tested - Issues | Issues as of RC4 |
| MacWorks Plus II 2.5.0 | Tested - Unsupported | Does not boot. Requires PFG. |
| Lisa Office System 1.x | Untested | |
| Lisa Office System 2.x | Untested | |
| Lisa Office System 3.x | Tested - Issues | Installer boots but fails to install Disk 1. Bails with 'Odd Address Exception' error in debug logs. |
| Microsoft Xenix | Untested | |
| UniSoft UniPlus Unix V | Untested | |
| Lisa Workshop | Untested | |

## 2025.03.15

Due to wxWidgets limitations, the minimum macOS version for the latest LisaEm release has been hardcoded to macOS 14.5. If an older macOS release is
required, please file an issue.

## 2021.01.15

Added TerminalWx vt100 terminal widget, xmodem for interfacing with BLU
switched Preferences for serial port to pull down menu instead of radio button since there's too many endpoints now.
Disabled Serial port A as it causes crashing in Lisa Terminal

## 2020.08.21

Fixed "the bug" - Desktop menu, scrollbars, linker errors.
Rebuilt the display refresh system so that the refresh rate is relative to host timing, not guest CPU.
Added a checkbox to disable mouse scaling under the display menu (if the mouse is incorrect for your system, toggle this).

If you see weird behavior on startup, try deleting both the main preferences as well as the specific Lisa preferences.

------------------------------------------------------------------------------

## Compiling LisaEm from sources

### Compiling wxWidgets for your system
LisaEm uses the wxWidgets library extensively. wxWidgets must be built and installed before building LisaEm from sources.

The scripts directory contains several scripts that you could use to build wxWidgets for your system. We will generally link LisaEm statically, especially for macOS and Windows.

NOTE: Some of these scripts are referenced in OS-specific installation processes later in this document.

```
scripts/build-wx3.2.9-modern-macosx.sh
scripts/build-wx3.1.5-cygwin-windows.sh
scripts/build-wxwidgets-gtk.sh
```
After wxWidgets is installed to `/usr/local/wxsomething`, add `/usr/local/wxsomething/bin` to your path before running the LisaEm build script.

Example:

```
$ export PATH=$PATH:/usr/local/wx3.2.9-cocoa-macOS-15.1-x86_64,arm64

# verify that wxWidgets is available:
wx-config --list
```

![building-wx-widgets](resources/1-clone-and-build-wx-widgets.gif)

### Compiling LisaEm (Windows)
We have detailed instructions [here](HowToBuildOnWindows.md).

### Compiling LisaEm (MacOS X)

Compiling on MacOS X (or macOS) requires upstream wxWidgets (not system provided). Use the included build scripts with the following directions:

```
$ git clone https://github.com/arcanebyte/lisaem
$ cd lisaem
$ scripts/build-wx3.2.9-modern-macosx.sh --enable-universal_binary=arm64,x86_64

$ export PATH="/usr/local/wx3.2.9-cocoa-macOS-$(sw_vers -productVersion | cut -d. -f1-2)-x86_64,arm64/bin:$PATH"
$ ./build.sh clean build
$ sudo ./build.sh install 
```

The `LisaEm.app` application will be installed in the Applications folder.

### Compiling LisaEm (for all other platforms):

You will need wxWidgets 3.0.4-3.2.9 installed. Do not use system provided wxWidgets, but rather build your own using the scripts in the scripts directory as mentioned above.

You will want to install/compile wxWidgets **without** the shared library option, except perhaps on GTK systems, but if you do this, it will not be portable except to systems of the same kind and version.

Unlike most apps that use autotools, or cmake, LisaEm uses the [bashbuild](https://github.com/rayarachelian/bashbuild) system which was created as a side effect of developing LisaEm. There's a fake `./configure` and `Makefile` that are just wrappers around `bashbuild`.

After installing/compiling wxWidgets, ensure that wx-config is in your path, cd to the LisaEm source code directly and run:

	```
	./build.sh clean build
	sudo ./build.sh install 
	```

This will install the lisaem and lisafsh-tool binaries to /usr/local/bin, and will install skins and sound files to /usr/local/share/LisaEm/; on Windows it will be installed to C:\Program Files\Sunder.Net\LisaEm and /Applications for macOS.

![compiling lisaem](resources/2-build-lisaem.gif)

### Cross compiling

Cross compiling is possible by passing the -arch=xxx option to the top level build.sh script. This is how PPC binaries are built on an i386 10.5 VM.

### Compressing binaries with UPX

If your system has the upx command available, it will also compress the resulting binary with upx in order to save space, it's a good idea to install this command. (The upx step is disabled for debug-enabled builds as it interferes with gdb, and on certain older versions of macOS X such as 10.8, 10.9 where it's broken and causes segfaults in the resulting binaries.)

### Statically linked wxWidgets

On Windows and macOS x we'll also want a static build so we don't have to ship a copy of wxWidgets along with the app. Of course building for your own system doesn't require that, however the included scripts are setup to produce static libraries, except for Linux/GTK.

If you don't plan on using wxWidgets for anything else on Windows or macOS x and have compiled static versions of wxWidgets and LisaEm, you may then discard the wxWidgets directory. You could also uninstall Cygwin. 

But you may find yourself needing to redo all this support infrastructure when a new version of LisaEm is released. Also, don't forget to clean up the resulting wxWidgets3.x.x directories under the script directory as they'll take up quite a lot of space.

Native OS provided copies of wxWidgets, such as those packaged with various Linux distributions are likely not going to work as well with LisaEm, so your mileage may vary. If you experience issues with those, please use the appropriate script in the scripts directory for your system to compile wxWidgets (although as I write this, the wxWidgets 3.0 for GTK provided with Ubuntu 18.04 seems to function correctly.)

### About macOS binary distributions

Binary distributions of LisaEm for macOS X provided on `dmg` images, will include multiple binaries for multiple CPU architectures such as PPC, PPC64, i386, and x86-64. While the macOS x `lipo` command can glue multiple architecture binaries together, it cannot let you package up multiple copies of a single architecture. To get around this, I've added an internal selector script `lisaem.sh` in the resources directory which is copied to `LisaEm.app/Contents/MacOS/lisaem.sh` - this scripts looks at your system and figures out what macOS version and CPU architecture you're running and then attempts to find the most likely binary suitable for your machine and runs that one.

Because some macOS/Xcode versions are only compatible with certain versions of wxWidgets, for example, Mac OS X 10.8 (Mountain Lion) will use a copy of LisaEm compiled against wxWidgets 3.0.4, while macOS 10.15 (Catalina) will run a copy of LisaEm compiled against wxWidgets 3.1.3. You can use the "About LisaEm" menu item under the credits to see the version.

Additionally a `--delete-other-binaries` option is provided in the script, so if you run `/Applications/LisaEm.app/Contents/MacOS/lisaem.sh --delete-other-binaries` from iTerm or Terminal, it will delete the shipped binaries that are not appropriate to your system, saving you a few MB.

![done compile](resources/3-done.gif)
