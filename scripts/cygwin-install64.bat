@ECHO OFF
REM -- Automates cygwin installation
REM -- Source: https://github.com/rtwolf/cygwin-auto-install
REM -- Based on: https://gist.github.com/wjrogers/1016065
 
SETLOCAL
 
REM -- Change to the directory of the executing batch file
CD %~dp0
 
REM -- Configure our paths
REM SET SITE=http://cygwin.mirrors.pair.com/
SET SITE=http://mirror.clarkson.edu/cygwin/
SET LOCALDIR=%CD%
SET ROOTDIR=C:/cygwin
e:
cd e:\
 
REM -- These are the packages we will install (in addition to the default packages)
SET PACKAGES=mintty,wget,ctags,diffutils,git,git-completion,git-svn,stgit,mercurial
REM -- These are necessary for apt-cyg install, do not change. Any duplicates will be ignored.
REM http://mirror.clarkson.edu/cygwin/x86_64/release/
SET PACKAGES=%PACKAGES%,wget,tar,gawk,bzip2,subversion,ImageMagick
SET PACKAGES=%PACKAGES%,mingw64-x86_64-binutils,mingw64-x86_64-gcc-core,mingw64-x86_64-gcc-debug-info,mingw64-x86_64-gcc-g++,mingw64-x86_64-gcc-objc,gcc-core,gcc-g++,make
SET PACKAGES=%PACKAGES%,upx,netpbm,p7zip,xz,zip,unzip,lynx,curl,lz4,libpng,libpng-devel,giflib,openal,vim,openjpeg2

 
REM -- Do it!
ECHO *** INSTALLING DEFAULT PACKAGES
e:\setup-x86_64.exe --quiet-mode --download --local-install --no-verify -s %SITE% -l "%LOCALDIR%" -R "%ROOTDIR%"
ECHO.
ECHO.
ECHO *** INSTALLING CUSTOM PACKAGES
e:\setup-x86_64.exe -q -d -D -L -X -s %SITE% -l "%LOCALDIR%" -R "%ROOTDIR%" -P %PACKAGES%
 
REM -- Show what we did
ECHO.
ECHO.
ECHO cygwin installation updated
ECHO  - %PACKAGES%
ECHO.

ECHO apt-cyg installing.
set PATH=%ROOTDIR%/bin;%PATH%
%ROOTDIR%/bin/wget https://raw.githubusercontent.com/transcode-open/apt-cyg/master/apt-cyg
%ROOTDIR%/bin/bash.exe -c 'mv apt-cyg /bin; chmod +x /bin/apt-cyg'
ECHO apt-cyg installed if it says somin like "A    /bin" and "A   /bin/apt-cyg" and "Exported revision 18" or some other number.

ENDLOCAL
 
PAUSE
EXIT /B 0
