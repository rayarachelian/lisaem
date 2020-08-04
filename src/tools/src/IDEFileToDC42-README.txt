IDEFile to Disk Copy 4.2 image converter, version 1.0.
(C) 2011 Natalia Portillo, All Rights Reserved.
libdc32 (C) 2011 Ray Arachelian, All Rights Reserved.
Under BSD and GPL licenses.

Usage: IDEFileToDC42 IDEFile_Image.bin

This utility will take an IDEFile image and convert it to a Disk Copy 4.2
image suitable to be use on emulators.
Filename will be "idefile.dc42".

To compile simply do:
gcc -c libdc42.c
gcc -c IDEFileToDC42.c
gcc -o IDEFileToDC42 libdc42.o IDEFileToDC42.o

Enjoy!
