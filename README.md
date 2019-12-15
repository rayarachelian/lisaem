![LisaEm Logo](resources/lisaem-banner.png)

###Copyright © 2019 by Ray Arachelian, All Rights Reserved. 
###Released under the terms of the GNU Public License v3.

------------------------------------------------------------------------------
Lisa Emulator Source Build README                    http://lisaem.sunder.net/
------------------------------------------------------------------------------

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

##2019.10.13 Finally got it working on macos X 10.11+ but had to recompile
both wxWidgets and LisaEm with -stdlib=libc++, and LisaEm with -lstdc++.6,
there are still lots of bugs, there are stubs for HQX but this feature is
incomplete so not yet included in the code. I've added a scripts directory
to help the end user be able to build wxWidgets properly. Much more testing
is needed on macos X, and certainly I need to rewrite all the Windows building
code.


##2019.09.29 This is a developer grade preview, you can expect tons of bugs and
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

##Compiling for Linux:

You will need netpbm as well as wxWdigets 3.0.4-3.1.2 installed. Do not use
system provided wxWidgets, but rather build your own using the scripts in the
scripts directory.

You will want to install/compile wxWidgets without the
shared library option.      

After installing/compiling wxWidgets, ensure that wx-config
is in your path, cd to the source code directly and run

	./build.sh clean build
	./build.sh install  


This will install the lisaem and lisafsh-tool binaries to
/usr/local/bin, and will install sound files to 
/usr/local/share/LisaEm/

If your system has the upx command available, it will
also compress the resulting binary with upx in order to save
space.

```------------------------------------------------------------------------------
On windows and macos x we'll also want a static build so we don't have to ship
a copy of wxWidgets.

It appears that the IDE I depended on, wxDSGN.sourceforge.net hasn't been
updated since 2011, so I'll need to do something else with cygwin and ming64,
so that may complicate things. I could depend on the Ubuntu subsystem, but then
that's likely limited to just the pro versions of windows and not the home ones
so likely I'll continue to depend on cygwin, will have to see.

------------------------------------------------------------------------------```

## Compiling for Raspbian:

**NOTE:** the wx2.8 packages do not work with LisaEm, and custom ones are needed.
Build your own from the scripts dir.

(If your raspbian matches Raspbian GNU/Linux 7
Linux raspberrypi 3.18.7-v7+ #755 SMP PREEMPT Thu Feb 12 17:20:48 GMT 2015 armv7l GNU/Linux )

or:

Build wxWidgets and LisaEm yourself like so (assuming your user is pi):
(Likely you should use wx3.1.2 and not 2.9.5 - this text is old)

```
  sudo -s
  apt-get install libgtk-3-dev netpbm upx  #upx is optional but will shrink the binary by ~30%
  cd ~
  VER=2.9.5
  TYPE=gtk
  cd wxWidgets-${VER}
  mkdir build-${TYPE}
  cd    build-${TYPE}
  ../configure --enable-unicode --enable-debug --disable-shared --without-expat --without-regexp --disable-richtext          \
             --with-libtiff=builtin --with-libpng=builtin --with-libjpeg=builtin --with-libxpm=builtin --with-zlib=builtin \
             --prefix=/usr/local/wx${VER}-${TYPE} && make && make install
  cd ~pi
  export PATH=/usr/local/wx2.9.5-gtk/bin:$PATH
  export LD_LIBRARY_PATH=/usr/local/wx2.9.5-gtk/lib:/lib:/usr/lib:/usr/local/lib
  cd lisaem-1.2.6.2
  ./build.sh clean --without-static --without-rawbitmap  build install
  cd ~
  chown -R pi ~pi
  ln -s /usr/local/share/LisaEm /usr/local/share/lisaem
```

There's a bug where it warns on startup about wxScroll, you can ignore it.

Once LisaEm is built with --with-static, wxWidgets should no longer be needed.

TODO: convert wxWidgets and LisaEm to proper Rpi .debs
------------------------------------------------------------------------------

## Compiling for Mac OS X 10.3 and higher:

*NOTE* these are outdated instructions ::TODO:: fix these.

You will need to download the source code for wxWidgets 2.8.x
from www.wxwidgets.org.  After extracting it, you'll need
to modify it as follows:


IMPORTANT!

In your wxMac-2.8.0 dir, edit the file

include/wx/mac/carbon/chkconf.h

```
change the line with '#define wxMAC_USE_CORE_GRAPHICS 1' 
to                   '#define wxMAC_USE_CORE_GRAPHICS 0'
```

(Many thanks to Brian Foley for finding this!)

If you do not do this, the display will look very ugly.


You will want to install/compile wxWidgets without the
shared library option.      


	./build.sh clean build

The application will be inside of the source code
directory under ./lisa as LisaEm.app



------------------------------------------------------------------------------

## Compiling for win32:

Use the cygwin installer batch files and then extract or clone lisaem in
cygwin and use the download script in the scripts directory before building.

------------------------------------------------------------------------------
