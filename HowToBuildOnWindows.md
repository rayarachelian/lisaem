# How to build LisaEm on Windows

## Overview
The build instructions below have been tested on Windows 11, with the latest LisaEm code in the "master" branch.

Note: they may not work well with the "RC5-2024.07.28" release, as we made fixes since then.

The build process involves installing the latest Cygwin version with a set of required packages, checking out the LisaEm code from this repository, building the wxWidgets library from sources, building the LisaEm binary and tools, and installing them in in Windows.

Note: The generated binaries will work on 64-bit x86 Windows systems. Building for 32-bit x86 processors and 32-bit Windows OS is no-longer supported. If you wish to use LisaEm on such systems, please download an older 32-bit build from https://lisa.sunder.net/downloads.html.

## 1. Install Cygwin
Cygwin is a Linux-like command line environment for Windows.

Please download and install the latest version of Cygwin from  https://cygwin.com/install.html , along with the following packages: `wget,git,upx,netpbm,xz,zip,unzip,mingw64-x86_64-gcc-core,mingw64-x86_64-gcc-debug-info,mingw64-x86_64-gcc-g++,mingw64-x86_64-gcc-objc,gcc-core,gcc-g++,make`.

Alternatively, for Cygwin beginners, we have an automated [unattended Windows batch script](scripts/cygwin-install.bat) which does that for you. Follow these steps:
  * Download file `setup-x86_64.exe` from https://cygwin.com/install.html into your "Downloads" folder.
  * Open a command prompt and run the following commands:
      ```
      cd Downloads
      # this will download our unattended cygwin inastallation batch script:
      powershell Invoke-WebRequest -Uri https://raw.githubusercontent.com/arcanebyte/lisaem/refs/heads/master/scripts/cygwin-install.bat -OutFile cygwin-install.bat
      # Run the unattended cygwin installation batch script:
      cygwin-install.bat setup-x86_64.exe
      ```

      The Cygwin installer window should open and it should start downloading the required packages. Please wait for its completion; it should exit by itself. At the end, Cygwin will be installed into folder `C:\cygwin`, along with all packages necessary for building LisaEm. To customize your installation, see file [cygwin-install.bat](scripts/cygwin-install.bat).
 * Note: The steps above do not need to be "Run as Administrator". This is because your user is allowed to create and write to folder `C:\cygwin`.

## 2. Open a Cygwin command window
  * Start the Windows program "Cygwin64 Terminal", or
  * Open a new Command Prompt and type: `c:\cygwin\cygwin`.
  * You are in Cygwin now.
    
  Note: Your "home folder" (accessible via `cd ~`) is also accessible in Windows Explorer: you can find it in `C:\cygwin\home\<your-windows-username>`.

## 3. Download the LisaEm sources from GitHub
Run the following commands in Cygwin:

```
cd ~
git clone https://github.com/arcanebyte/lisaem.git
```

This will download the latest LisaEm sources into new subfolder `lisaem`.

## 4. Download and build the wxWidgets binaries
Run the following commands in Cygwin:

```
cd ~/lisaem/scripts
./build-wx3.2.7-cygwin-windows.sh
```

This will download file `wxWidgets-3.2.7.tar.bz2` from https://github.com/wxwidgets/wxwidgets/releases , expand it into subfolder `wxWidgets-3.2.7`, build it in subfolder `wxWidgets-3.2.7/build-msw`, install wxWidgets into folder `/usr/local/wx3.2.7-msw` and delete the temporary build folder above. Upon success, it should print e.g. `Default config is msw-unicode-static-3.1`.

## 5. Build the LisaEm binaries
Run the following commands in Cygwin:

```
cd ~/lisaem
./build.sh clean build
```

This will compile and generate the LisaEm executable file `bin/lisaem.exe`, along with other tools in the same `bin` subfolder.

If you want to run the produced executable file (above), you will first need to copy the "skins" folder:

```
cp -r resources/skins/ bin
bin/lisaem.exe
```

## 6. Installing the LisaEm binaries on Windows

  ### Option 1: Use the "build install" command:
  The LisaEm build system comes with the "build install" command, which creates folder `C:\Program Files\Sunder.NET\LisaEm` and copies the necessary LisaEm files into it. It does not do anything else than that (does not create any other files, does not create desktop or startup menu shortcuts, does not modify the Windows registry). Run the following commands in Cygwin:
  
  ```
  cd ~/lisaem
  ./build.sh install
  ```

  Follow the prompts. It will launch a separate "mintty.exe" command window "As Administrator", so that it would be able to write into folder C:\Program Files\Sunder.NET\LisaEm.

  ### Option 2: Install it manually, by running a set of commands in Cygwin

  The commands below will "install" LisaEm into folder "LisaEm" on your desktop. You can change the TARGET_FOLDER name if desired, note that it is  in "cygwin file format". Run them in Cygwin:
  ```
  TARGET_FOLDER=/cygdrive/c/Users/$USER/Desktop/LisaEm
  mkdir $TARGET_FOLDER
  mkdir $TARGET_FOLDER/bin
  cd ~/lisaem
  cp -r resources/skins $TARGET_FOLDER
  cp -r bin/*.exe $TARGET_FOLDER/bin
  mv $TARGET_FOLDER/bin/lisaem.exe $TARGET_FOLDER
  ```

## Running the LisaEm emulator
Once you install LisaEm using the steps above, you don't need Cygwin in order to run it.
Open Windows File Explorer, navigate to your installation folder (e.g. `C:\Program Files\Sunder.NET\LisaEm`) and double-click on LisaEm.exe to launch it.
Optionally, drag a shortcut to your desktop, to find it easier next time.

At first start, it will create the emulator "preferences" file `C:\Users\<Your Windows User>\AppData\Roaming\lisaem.conf`.
This way, regardless of how / from where you've started the emulator, it will always find this config file.

Does the Lisa feel sluggish? No problem! Go to the "Throttle" menu, and choose how fast do you want it to be, e.g. change it from the default 5 Mhz to 256 Mhz. Restart.

## Uninstalling LisaEm
To reduce the risk of deleting "too much" by mistake, we have not implemented a "build uninstall" command.
So, to uninstall LisaEm, just manually delete the folder where it is installed.
You may also want to delete the emulator "preferences" file `C:\Users\<Your Windows User>\AppData\Roaming\lisaem.conf`.
