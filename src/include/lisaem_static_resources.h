/**************************************************************************************\
*                                                                                      *
*                                                                                      *
*                            Apple Lisa 2 Emulator                                     *
*                                                                                      *
*                           http://lisaem.sunder.net                                   *
*                                                                                      *
*                                                                                      *
*                  Copyright (C) 1998, 2007 Ray A. Arachelian                          *
*                            All Rights Reserved                                       *
*                                                                                      *
*                  Released under the terms of the GNU GPL 2.0                         *
*                                                                                      *
*                            static resource headers                                   *
*                                                                                      *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

// references to static resources from lisaem_static_resources.cpp
// shoved in here to save time whilst compiling.

// icon - is static, must be loaded in the .h
#if !defined(__WXMSW__) && !defined(__WXOSX__)
#include "../resources/lisa2icon.xpm"
#endif

// sounds - these are linked into the binary on Win32/UNIX, and are .WAV files
// in the application bundle on Mac OS X.

/**** no longer used *****
#if !defined(__WXOSX__)
extern unsigned char floppy_eject[];
extern unsigned char floppy_insert_sound[];
extern unsigned char floppy_motor1[];
extern unsigned char floppy_motor2[];
extern unsigned char lisa_power_switch[];
extern unsigned char lisa_power_switch01[];
extern unsigned char lisa_power_switch02[];
extern unsigned char poweroffclk[];

extern int floppy_eject_size;
extern int floppy_insert_sound_size;
extern int floppy_motor1_size;
extern int floppy_motor2_size;
extern int lisa_power_switch01_size;
extern int lisa_power_switch02_size;
extern int poweroffclk_size;
#endif
***********************************************/

// image resources - these are BMPs on Win32 and linked in via resources.
// They're .PNG files in the application bundle on Mac OS X.
#if !defined(__WXMSW__) && !defined(__WXOSX__)

extern unsigned char *floppy0_xpm[];
extern unsigned char *floppy1_xpm[];
extern unsigned char *floppy2_xpm[];
extern unsigned char *floppy3_xpm[];
extern unsigned char *floppyN_xpm[];
extern unsigned char *lisaface0_xpm[];
extern unsigned char *lisaface1_xpm[];
extern unsigned char *lisaface2_xpm[];
extern unsigned char *lisaface3_xpm[];
extern unsigned char *power_off_xpm[];
extern unsigned char *power_on_xpm[];
#endif

extern int how_many_bits[];

// Remove this from actual project
// extern unsigned char screenshot[];

extern char *wx_keysyms[];
extern long wx_keyvals[];
