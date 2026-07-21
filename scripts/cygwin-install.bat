@ECHO OFF
:: Automates the Cygwin installation
:: Based on: https://github.com/rtwolf/cygwin-auto-install
 
SETLOCAL

SET ROOTDIR=C:\cygwin
SET PACKAGESDIR=C:\cygwin-packages

ECHO This Windows batch script installs (or updates if already present) Cygwin into folder %ROOTDIR%, along with the necessary packages to build LisaEm. 
ECHO In addition, folder "%PACKAGESDIR%" will be created; Cygwin stages its packages here. Do not delete it.
ECHO Before running this script, please download the Cygwin installation file setup-x86_64.exe from https://cygwin.com/install.html and then run this script, specifying the file location, e.g.
ECHO    %~nx0 Downloads\setup-x86_64.exe
ECHO Note: this script is just for your convenience; you can do the same manually, by launching the Cygwin installer setup-x86_64.exe manually.
ECHO Note: this script, and the Cygwin installer, DO NOT need to be run "As Administrator".
ECHO Note: this script installs the 64-bit version of Cygwin, which requires a 64-bit CPU (anything after Pentium 4), and a 64-bit version of Windows. 
ECHO Note: to uninstall Cygwin, just manually delete the two folders %ROOTDIR% and "%PACKAGESDIR%".

:: Change to the directory of the executing batch file. Some installation log files will be created here.
CD %~dp0

:: Check if a command-line argument was provided
if "%~1"=="" (
    ECHO No path to file setup-x86_64.exe was provided on the command line.
    ECHO Please download the Cygwin installation file setup-x86_64.exe from https://cygwin.com/install.html and re-run this script, specifying the file location, e.g.
    ECHO %~nx0 Downloads\setup-x86_64.exe
    EXIT /b 1
)

:: Store the path to file "setup-x86_64.exe" into INSTALLER_FILE_PATH
set INSTALLER_FILE_PATH=%~1

:: Check if the file exists
if not exist "%INSTALLER_FILE_PATH%" (
    ECHO File does not exist: %INSTALLER_FILE_PATH%. Please specify a valid path to the Cygwin installer file setup-x86_64.exe.
    EXIT /b 1
)

if "%INSTALLER_FILE_PATH:~-16%" neq "setup-x86_64.exe" (
  ECHO The specified file path should end with 'setup-x86_64.exe'. You are doing something wrong.
  EXIT /b 1
)


:: Print the provided path
ECHO The path %INSTALLER_FILE_PATH% to the Cygwin installation file looks legit.
ECHO Press Enter to start the Cygwin installer, and then choose "Yes" when prompted to run it ...
SET /p input=

:: Cygwin packages will be downloaded from here
SET SITE=https://mirror.clarkson.edu/cygwin/

:: We need these custom (non-default) packages:
SET PACKAGES=wget,git,upx,netpbm,xz,zip,unzip
SET PACKAGES=%PACKAGES%,mingw64-x86_64-gcc-core,mingw64-x86_64-gcc-debug-info,mingw64-x86_64-gcc-g++,mingw64-x86_64-gcc-objc,gcc-core,gcc-g++,make

ECHO Installing Cygwin and custom packages into %ROOTDIR%
START "Cygwin Installer" "%INSTALLER_FILE_PATH%" --quiet-mode --no-desktop --no-startmenu --download --local-install --no-verify -s %SITE% -l "%PACKAGESDIR%" -R "%ROOTDIR%" -P %PACKAGES%

ECHO Please wait for the Cygwin installer (which runs in a separate window) to finish.
ECHO To start Cygwin, open a Windows command prompt and type: "%ROOTDIR%\cygwin"
ECHO Note: to uninstall Cygwin, just manually delete the two folders %ROOTDIR% and "%PACKAGESDIR%".

ENDLOCAL

PAUSE
EXIT /B 0
