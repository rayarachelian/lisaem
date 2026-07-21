# Scripts Directory

Some sample scripts as to how to build wxWidgets 3.x for LisaEm as well as:

1. A script to add the interim build.sh links (see README.md inside bashbuild).
2. A sample Cygwin install script cygwin-install.bat to help install Cygwin
   properly on Windows, with the right set of packages. See more details
   [here](../HowToBuildOnWindows.md).
3. A script to remove the lisaem preferences hidden inside ~/Library on macosx
   (this might fail under Mojave/Catalina due to gatecreeper).

When building on macos x, it's possible to build wxWidgets and LisaEm using
just the command line tools, but it doesn't always work properly and you can
get all sorts of weird issues. It's best to make sure you install the right
version of XCode for your OS as well as the matching command line tools package.
Generally I go with the lowest major version that an OS will handle, and then
the highest middle/minor version. This allows that OS to have the older SDK
for the previous os. See: https://en.wikipedia.org/wiki/Xcode#Version_comparison_table
for versions of XCode that map to what OS.

For example for macos 10.9 I'd use XCode 5.1.1 which supports 10.9 and 10.8.

NOTE: there are issues building LisaEm on older OS X using clang/llvm/Xcode.
To fix this, you should install the GNU C Compiler Suite using homebrew from
brew.sh using the command brew install gcc@7.

On older systems, you'll need to go through some pretty unfriendly gymanstics,
such as first installing the XCode command line tools (brew.sh will do this for you)
and then using the clang compiler to build gcc@4.9, then use gcc-4.9 to build gcc@5,
brew install --cc=gcc-4.9 gcc@5 
and so on all the way to gcc-7 (though it should be possible to build both wxWidgets
and LisaEm with gcc-5).

Then invoke the build script like so; ommit --with-debug --with-tracelog if you don't
need them.

                 v- change this to match your system
    export PATH=/usr/local/wx3.1.2-cocoa-x64-macOS-10.11/bin:$PATH
    CC=gcc-7 CXX=gcc-7 ./build.sh clean build --64 --with-debug --with-tracelog

(On newer systems it should build with clang so you can ommit the CC and CXX env vars)

On OS X, you may get this error with wxWidgets 3.1.x:
/usr/local/wx3.1.2-cocoa-x64-macOS-10.11/include/wx-3.1/wx/strvararg.h:27:18: fatal error: 'tr1/type_traits' file not found

To fix this, add the following lines at around line 20:

see: https://stackoverflow.com/a/37632111
insert that block of code to fix issue with tr1/type_traits

Slightly modified the code above, to avoid compiler complaints:

Paste the following into strvararg.h just before #ifdefined (HAVE_TYPE_TRAITS)

    #include <ciso646>  // detect std::lib
    #ifdef _LIBCPP_VERSION
    // using libc++
    #ifndef HAVE_TYPE_TRAITS
    #define HAVE_TYPE_TRAITS 1
    #endif
    #else
    // using libstdc++
    #ifndef HAVE_TR1_TYPE_TRAITS
    #define HAVE_TR1_TYPE_TRAITS 1
    #endif
    #endif


Like so:

    10 #ifndef _WX_STRVARARG_H_
    11 #define _WX_STRVARARG_H_
    12 
    13 #include "wx/platform.h"
    14 
    15 #include "wx/cpp.h"
    16 #include "wx/chartype.h"
    17 #include "wx/strconv.h"
    18 #include "wx/buffer.h"
    19 #include "wx/unichar.h"
    20 
    21 // >8 -------- insert here -------- 8<
    22 #include <ciso646>  // detect std::lib
    23 #ifdef _LIBCPP_VERSION
    24 // using libc++
    25 #ifndef HAVE_TYPE_TRAITS
    26 #define HAVE_TYPE_TRAITS 1
    27 #endif
    28 #else
    29 // using libstdc++
    30 #ifndef HAVE_TR1_TYPE_TRAITS
    31 #define HAVE_TR1_TYPE_TRAITS 1
    32 #endif
    33 #endif
    34 // >8 -------- insert here -------- 8<
    35 
    36 
    37 
    38 #if defined(HAVE_TYPE_TRAITS)
    39     #include <type_traits>
    40 #elif defined(HAVE_TR1_TYPE_TRAITS)
    41     #ifdef __VISUALC__
    42         #include <type_traits>
    43     #else
    44         #include <tr1/type_traits>
    45     #endif
    46 #endif
    47 

