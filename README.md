![LisaEm Logo](resources/lisaem-banner.png)

### Copyright © 2020 by Ray Arachelian, All Rights Reserved. 
### Released under the terms of the GNU Public License v3.

------------------------------------------------------------------------------
Lisa Emulator Source Build README                    http://lisaem.sunder.net/
------------------------------------------------------------------------------

## What is this thing?

This is the source code for LisaEm 1.2.7. LisaEm is a emulator that runs on a wide variety of modern and somewhat old systems by means of the wxWidgets framework, implementing an emulation of the (in)famous Apple Lisa Computer. The Apple Lisa computer is the predecesor of the Mac and the "inspiration" of many GUI environments from Windows 1.x to GEOS. Many of the original ideas of the modern GUI were first envisioned in the Apple Lisa, so it's a historically very important machine.

This document contains a few brief updates that are also found in the ChangeLog file, as well as some information on how to compile wxWidgets and LisaEm.
(The updates will go away once the bugs are removed as ChangeLog is better suited for them.)

## 2020.02.22

Fixed a bunch of windowing stuff for Windows. Still have some macos x issues
when scale is other than 1.0 regarding refreshes, which leaves a white area
near the Lisa's display. The disappearing window issue from 2020.02.22 is
fixed (window resizing issue).

2Y, 2X,3Y modes still have mouse issues and there are still other bugs.

## 2020.02.22

I've made many fixes, but there are still about a dozen bugs left. Most
notably when the scale is changed the window size is wrong. On macos x
it sometimes disappears entirely because it goes to size zero. A temporary
work around is to install the Spectacle App from https://www.spectacleapp.com/
and use the Command+Option+F to maximize the window. Normal macos x without
this leaves you only with the Window >> Maximize menu.

There's still some refresh issues with macos X as well.

Zoom larger than 2.0 on macos x causes LisaEm to lock up, so I've disabled it
for this platform only.

2Y and 2X,3Y modes still have mouse location issues


## 2019.12.25

Cleaned up a bunch of stuff and fixed builds on both Windows and macos X.
Both now install properly. There are still some bugs. Windows crashes on the
hq3x renderer, and quits (I think on sound), but can now find its resources.
Also cleaned up the old upper case function names in bashbuild that were a
holdover from the 1.3.0 days.
Added initial FreeBSD.sys support.

## 2019.12.12 
I rewrote a lot of the 1.3.0 build scripts and separated the fn's out to their
own include files, standardized the build scripts to the new "buildbash"
way of doing things, added progress bar and image display for KiTTY
(will do for iTerm2 as well), sub-build scripts are now source'd instead
of executed separately for better performance.

The old git repo at https://github.com/rayarachelian/lisaem-1.2.x/ is now
considered deprecated and only left there for historical purposes, and incase
someone needs older stable versions such as 1.2.6, etc.

While these scripts do produce a lisaem.exe on Windows with a Cygwin/mingw
build environment, the resulting executable does not (yet) function, I'm
unsure why, if you're familiar with Windows and understand MingW and wx
Widgets, I'd welcome any insights you may have.

There's a lot of stuff to clean up and debug, especially these README files.

## 2019.10.13 Finally got it working on macos X 10.11+ but had to recompile
both wxWidgets and LisaEm with -stdlib=libc++, and LisaEm with -lstdc++.6,
there are still lots of bugs, there are stubs for HQX but this feature is
incomplete so not yet included in the code. I've added a scripts directory
to help the end user be able to build wxWidgets properly. Much more testing
is needed on macos X, and certainly I need to rewrite all the Windows building
code.


## 2019.09.29 This is a developer grade preview, you can expect tons of bugs and
incomplete features, likely it will turn into a release in a month or two
depending on free time, etc.

This is a bit of a rant here as I've not worked out all the details of all
the moving pieces yet as well as to explain some of the decisions made,
etc.

Note that the Widget code is incomplete, don't attempt to use this with a
2/10 88 ROM as it will fail to work. See the Changelog for descriptions of
what has been worked on.

Some planned features may not make it into 1.2.7 but may be moved to 1.2.8.

The main driver for this release is the impending release of macosX Catalina
which removes 32 bit support, thus forcing a 64 bit binary release of LisaEm

There are also weird issues based on which version of wxWidgets you use.
If you use wxWidgets earlier than 3.12, such as 3.11 or so, you may see
lots of menu issues now that I've switched some of the menu items to Radio
Button rather than checkbox.

However, wx3.1.2 introduces a snag, at least in GTK on HiDPI ("retina" in
Apple-speak) 4K+ displays. Instead of reporting the actual display resolution
it returns 1920x1080p - not sure if this a GTK thing or a wxWidgets thing,
however, wx3.1.1 does not behave this way. Ironically, I went into building
1.2.7 with the goal of addressing HiDPI issues as in 2017 my 2011 17" Macbook
Pro died due to GPU failure, and due to concerns about repairability of the
newer machines which feature glued in batteries, soldered in RAM, soldered
in SSD, non-pro high priced laptops branded as pro, but limited in RAM and
CPU, and other user-unfriendly anti-features, I decided to give up on
Apple Inc. and go to Linux. In doing so I went to a 17" Acer Predator with
a GTX1070 GPU and a wonderful 17" 4K display and 64G of RAM, on which LisaEm
1.2.6 and earlier look very tiny and unusable. So now with wx3.1.2 doing 
weird things with HiDPI mode, I've come full circle. :)


I highly recommend NOT using any wxWidgets that ships with your OS and
instead compiling wx3.1.2 yourself, as they're going to be older and not
as featureful.

I've not YET tested this code on macOS X and Windows, but will do so over
the next few days and release a few binary releases, hopefully before
Apple releases Catalina, though it may wind up being after. Once vetted
through a few beta and release-candidate versions on win10 and macosx, the
final 1.2.7 will be released in binary form as well.

------------------------------------------------------------------------------
## Special steps for Windows:

If you wish to build for Windows, Cygwin, along with regular gcc and mingw is required.

Please examine the two cygwin installation batch files which will setup the appropriate packages. As you'll need to right click on the appropriate one (32 bit vs 64 bit) and run as Administrator, it's highly recommended that you carefully examine what they do.

(The Cygwin installer requires that it be run as Administrator when scripted, otherwise it goes interactive and does not properly select the packages passed to it on the command line.)

These scripts expect to be run from the E:\ drive, and that you've downloaded the Setup.exe for Cygwin and have named it properly as the script expects. Once cygwin is properly installed, copy the LisaEm source tarball (or do a git clone) to your home directory inside of Cygwin.

Next, use the scripts in the scripts directory that build wxWidgets.

## Compiling wxWidgets for your system

The scripts directory contains several scripts that you could use to build wxWidgets for your system. We will generally link LisaEm statically, especially for macos x and 

```
build-wx3.1.2-modern-macosx.sh
build-wxwidgets-cygwin-windows.sh
build-wxwidgets-gtk.sh
```
After wxWidgets is installed to `/usr/local/wxsomething`, add `/usr/local/wxsomething/bin` to your path before running the LisaEm build script.

## Compiling LisaEm for all platforms:

You will need netpbm as well as wxWdigets 3.0.4-3.1.2 installed. Do not use system provided wxWidgets, but rather build your own using the scripts in the scripts directory as mentioned above.

You will want to install/compile wxWidgets **without** the shared library option, except perhaps on GTK systems, but if you do this, it will not be portable except to systems of the same kind and version.

After installing/compiling wxWidgets, ensure that wx-config is in your path, cd to the source code directly and run

	./build.sh clean build
	sudo ./build.sh install 

(Don't use sudo on Cygwin, instead you'll be prompted whether you wish to launch the command in an Administration MinTTY session.)

This will install the lisaem and lisafsh-tool binaries to /usr/local/bin, and will install skins and sound files to /usr/local/share/LisaEm/; on Windows it will be installed to C:\Program Files\Sunder.Net\LisaEm and /Applications for macos.

If your system has the upx command available, it will also compress the resulting binary with upx in order to save space, it's a good idea to install this command. (The upx step is disabled for debug-enabled builds as it interferes with gdb.)

On Windows and macos x we'll also want a static build so we don't have to ship
a copy of wxWidgets along with the app. Of course building for your own system doesn't require that, however the included scripts are setupt to produce static libraries, except for Linux/GTK.

OS provided copies of wxWidgets are likely not going to work with LisaEm. Please use the appropriate script in the scripts directory for your system to compile wxWidgets (although as I write this, the wxWidgets 3.0 provided with Ubuntu 18.04 seems to function correctly.)
