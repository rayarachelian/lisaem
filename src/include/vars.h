/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.08.02                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, MMXX Ray A. Arachelian                          *
*                                All Rights Reserved                                   *
*                                                                                      *
*           This program is free software; you can redistribute it and/or              *
*           modify it under the terms of the GNU General Public License                *
*           as published by the Free Software Foundation; either version 2             *
*           of the License, or (at your option) any later version.                     *
*                                                                                      *
*           This program is distributed in the hope that it will be useful,            *
*           but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*           MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*           GNU General Public License for more details.                               *
*                                                                                      *
*           You should have received a copy of the GNU General Public License          *
*           along with this program;  if not, write to the Free Software               *
*           Foundation, Inc., 59 Temple Place #330, Boston, MA 02111-1307, USA.        *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
*                                                                                      *
*                                                                                      *
*      Global externed variables, macros, macro constants, and function prototypes.    *
*                                                                                      *
*   ---------------------------------------------------------------------------------  *
*                                                                                      *
*   Yes, this is ugly, yes, it's not quite standard practice, but there is madness to  *
*   method.  ;-)    (For the irony impaired, yes, that IS backwards)                   *
*                                                                                      *
*   1. Include all the needed include files.                                           *
*   2. Define macro fn's and constants used elsewhere                                  *
*   3. define type definitions                                                         *
*   4. define externed global variables                                                *
*   5. define fn extern or local prototypes (self checking)                            *
*                                                                                      *
\**************************************************************************************/

// This should always be on, but we can cheat for a slight speedup to turn it off?
#define CHECKODDPOINTERS 1
#define DEBUGLEVEL 100

#define UNUSED(x) (void)(x)

// If tracelog is enabled, also enable DEBUG code so that tracing can be done.
#ifdef TRACE
#ifndef DEBUG
#define DEBUG
#endif
#endif

// Enable to build a Lisa1 emulator instead of a Lisa2 emulator - not yet implemented - place holder.
// #define LISA1

// enables MMU cache vs mmu registers testing - this is VERY VERY SLOW, and no longer valid
// #define MMUVALIDATE 1

// If this is disabled the Generator CORE will calculate the status flags on every opcode
// hack for LisaEm to see if there are flag calculation issues.
#define NORMAL_GENERATOR_FLAGS 1

// This forces each executed opcode to have it's IPC re-created - used to see if IPC cache
// isn't working properly, or if we're hitting self-modifying code  -- very slow, do not use
// enabling this exposes cache sanitation issues
// - LPW: run one program, then when you run another, get bus error.
// #define EVALUATE_EACH_IPC 1

// Verify that free ipcts are accessible and don't cause segfaults
#define TEST_FREE_IPCTS_ARE_EMPTY 1

// enable a read after write to ensure we get back the same data we wrote - no longer supported
// #define VERIFY_WRITES 1
//  If this is enabled, it's a little slower because extra checks are required.

// do not use this ever//#define ALLOW_1536K_RAM 1 -- no longer

// Enable this if you want all of the Lisa Status register bits to be set. - slower, but more acurate.
#define FULL_STATREG 1

// If this is turned on, the VIA timers will no longer be tied to the CPU speed, rather,
// they'll be adjusted to match the throttle. So if the throttle is 10Mhz, these will take
// 2x CPU cycles they normally do.  This is in the hopes of slowing down the blinking
// text cursor to a reasonable value so it doesn't flicker.  It might not work with all
// OS's - will certainly cause LisaTest to complain if its run at throttle >5.0Mhz
// #define TIE_VIA_TIMER_TO_HOST 1
// DO NOT ENABLE THIS
// above does not work and instead throws error 93 on the dual parallel slot's power on self test.

// How close do we need to be in order to declare a matching the screen hash?
#define SCREENHASH_LIMIT 16

// Do read-only memory violations cause bus errors?
#define ROMEMCAUSESBUSERROR 1
#define NO_BUS_ERR_ON_NULL_IO_WRITE 1
//^2018 turns out that on the lisa, writes to undefined I/O space should not cause bus errors
// Thanks to Gilles Fetis for this. (seen in Gemdos)
// define this to re-enable the bus errors.

// Do physical memory over/underflows cause MMU exceptions?
// #define PHYS_UNDER_BUSERR 1
// #ifdef PHYS_OVERFLOW_BUSERR 1

// Enable this to avoid C compiler assumptions (noteably in the get_pending_vector() fn)
// it's safer to enable this if not using GCC, or you can't guarantee expression evaluation order is left to right
#define PARANOID 1

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following options are only for DEBUG enabled compiles
#ifdef DEBUG

// If this is enabled, and debug log is enabled, it will attempt to suppress loop disassembly
// by keeping track of the last MAX_LOOP_REGS and will only output the changed registers.
// #define SUPPRESS_LOOP_DISASM 1
// #define MAX_LOOP_REGS 10

// turn tracelog on/off using right mouse click
#define RIGHT_CLICK_TRACELOG 1

// Enable this to disassemble skipped opcodes to the log that weren't executed - might be buggy!
// #define DISASM_SKIPPED_OPCODES 1

// Same as above, but specific to a single block of code instead of every chunk of code.
// //0xd27a-0xddff for LisaTest  *REMOVE*
// #define LOOKAHEAD 1
// #define LOOKSTARTADDR    0x00b848d0
// #define LOOKENDADDR      0x00b849cc

// If enabled, what screenhash value to turn debugging on
// #define DEBUG_ON_SCREENHASH  {0,0,0,0,0,0,0,0,       0x7f,0x19,0xcc,0x88,0x87,0x5f,0xca,0x64,0xff,0xfc,0xff,0x5c,0x00,0x00}
// #define DEBUG_OFF_SCREENHASH {0,0,0,0,0,0,0,0,       0xff,0xff,0x9b,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0xdf}
//                             {0x7f,0xcb,0xe7,0xbd,0xe0,0xf3,0x2b,0xcd,0x7f,0xf3,0xff,0x5d,0x00,0x00}

// switches to fn's that log memory calls, if undefined, these are macros, so they're much faster.
// only turn this on if you need it.
#define DEBUGMEMCALLS 1

// this 2nd one also enables mmu table output
// #define DEBUGMEMCHKMMU

// Useful for recording what's plotted to the display.
#define DEBUGVIDMEMWRITES 1

// This leaves mouse droppings on the screen
// #define DEBUG_HOTBUTTONS

// This logs the mouse coordinates in stderr
// #define DEBUG_MOUSE_LOG 1

// this enables the cpucoretester.c which is runs multiple cores in parallel and tests them
// #define CPU_CORE_TESTER 1

// If debugging is turned on, this option can be turned on to provide a dump of the
// instruction cache and also the current cpu clock value when debugging is enabled.
// useful for extra long opcodes, and the debuggng of the 68K core.
// #define ICACHE_DEBUG 1

// Detect procedure entry into LisaOS
#define PROCNAME_DEBUG 1

#endif
// The above options are only for DEBUG enabled compiles
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// If this is enabled, it will force the Instruction Parameter Cache to be invalidated on
// for that address on each write to memory.  This is to correct problems with self-modifying code
// turns out this is needed - at least for LPW to work, because it will load a new program in the
// same space another one used to occupy, causing issues. *Want this on*
#define FORCE_MEMWRITE_TO_INVALIDATE_IPC 1

//-----------------------------------------------------------------------------------------------------------------------------

#ifndef IN_VARS_H
#define IN_VARS_H 1

#define BIT0 1
#define BIT1 2
#define BIT2 4
#define BIT3 8
#define BIT4 16
#define BIT5 32
#define BIT6 64
#define BIT7 128
#define BIT8 256
#define BIT9 512
#define BIT10 1024
#define BIT11 2048
#define BIT12 4096
#define BIT13 8192
#define BIT14 16384
#define BIT15 32768
#define BIT16 65536
#define BIT17 131072
#define BIT18 262144
#define BIT19 524288
#define BIT20 1048576
#define BIT21 2097152
#define BIT22 4194304
#define BIT23 8388608
#define BIT24 16777216
#define BIT25 33554432
#define BIT26 67108864
#define BIT27 134217728
#define BIT28 268435456
#define BIT29 536870912
#define BIT30 1073741824
#define BIT31 2147483648
#define BIT32 4294967296

// Allow C to call C++

#ifdef __cplusplus
#define CPP2C "C"
#else
#define CPP2C
#endif

// Magic Macros - allows me to get around one of C's more annoying limits
// global variables are generally a big no-no, but in emulators, you want
// to get as much performance out of things as you can, so you bend the rules.
//
// Globals accessed by other .c files need extern declarations, which is
// fine and great.  But externs stupidly don't ignore assignments, so you
// can't just use a global variable file, so you wind up with a .c and a .h
// if these get out of sync, all sorts of evil things can happen.  i.e. you
// delcare a variable as a uint8 in one file and a uint16 in another.  When
// you compile, the compiler has no knowledge that one .c file expencts a
// byte and the other expects a word - because the two .c files have a different
// view of things - one has the actual variable, the other has an extern
// delcaration - hence, very bad things happen.
//
// Solution: we fix this by putting all the global variables in a single file
// this one, but use C MACRO's to allow it to turn on extern in the case of
// the include, and assign in the value in the case of the globals vars.c file.
//
// Neat, no?
//
// The one caveat is that C Macro's can't handle missing parameters, so we need
// two versions - one for when we don't assign a value (DECLARE) and one for when
// we do (GLOBAL).  Also, we can't do this: int a,b,c - need one GLOBAL per var now.
//- - - - - - - - - -

///////////////////////////////////////////////////////////////////////////////////
// Are we assigning a value to the variable? (and alternate default value version)

// #define    ASSIGN(TYPE, VAR, val...)        TYPE VAR= ## val  // ## causes gcc 3.x preprocessor to warn
#define ASSIGN(TYPE, VAR, val...) TYPE VAR = val
#define CASSIGN(MYTYPE, MYVAR, val...) const MYTYPE MYVAR = val
#define NOASSIGN(TYPE, VAR) TYPE VAR

// Are we defining an extern reference to the variable?
#define REFERENCE(TYPE, VAR, val...) extern TYPE VAR
#define CREFERENCE(TYPE, VAR, val...) extern const TYPE VAR

// Doesn't matter, we can use a single macro for both, but decide which one here.
#ifdef IN_VARS_C

#define GLOBAL(TYPE, VAR, val...) TYPE VAR = val
#define DECLARE(TYPE, VAR) TYPE VAR
#define AGLOBAL(MYTYPE, MYVAR, val...) const MYTYPE MYVAR = val
#define ACGLOBAL(TYPE, VAR, val...) const TYPE VAR = val

#else
#define AGLOBAL REFERENCE
#define ACGLOBAL REFERENCE
#define GLOBAL REFERENCE
#define DECLARE REFERENCE
#endif

/////////////////////////////////////////////////////////////////////////////////

// include all the includes we'll (might) need (and want)
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#ifndef __MSVCRT__
#include <sys/mman.h>
#endif

#include "built_by.h"
#include "generator.h"
#include "registers.h"

// disable this - dangerous to little-endian transforms??
#ifdef ALIGNLONGS
#undef ALIGNLONGS
#endif

#include "z8530_structs.h"
#include "libdc42.h"

// If the Host OS has this, use it.
#ifndef FILENAME_MAX
#define FILENAME_MAX 256
#endif

/*-------------------- 2003.07.08 17:23 ------------------------
 * hack to fix endianness under CygWin.  Might not be an
 * actuall issue with cygwin, but rather with the configure
 * scripts that I use...  *NOTE* Modifications to these must
 * also be reflected in registers.h and generator.h!
 * ----------------------------------------------------------*/

#ifdef PROCESSOR_INTEL
#undef WORDS_BIGENDIAN
#undef BIG_ENDIAN
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#undef BYTESHIGHFIRST
#endif

#ifdef PROCESSOR_SPARC
#define WORDS_BIGENDIAN 1
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 1
#endif
#undef LITTLE_ENDIAN
#define BYTESHIGHFIRST 1
#endif

#ifdef __POWERPC__
#define BYTES_HIGHFIRST 1
#define WORDS_BIGENDIAN 1
#endif
#ifdef __sparc__
#define BYTES_HIGHFIRST 1
#define WORDS_BIGENDIAN
#endif
#ifdef sparc
#define BYTES_HIGHFIRST 1
#define WORDS_BIGENDIAN 1
#endif

#ifdef __CYGWIN__
#undef WORDS_BIGENDIAN
#undef BIG_ENDIAN
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#undef BYTESHIGHFIRST
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifdef CPU_CORE_TESTER
extern void corecpu_get_start_masterregs(void);
extern void corecpu_complete_opcode(int endmmucx);
#endif

// Utility tables
//
// These are here to make certain transformation operations faster, and easier.
//
// Doesn't look like I'm using most of these, but it may be worth having them around
//
//
//////////////////////////////////////////////////////////////////////////////////

// given a byte, find the bit# with the highest value.  return ff for 0 since
// no bit is set (bit0=1, bit1=2, bit2=4, bit3=8, etc.)
static inline int highest_bit_num(uint8 v)
{
  unsigned int s, r = 0;
  if (v == 0)
    return 0xff;
  s = ((v & 0xf0) != 0) << 2;
  v >>= s;
  r |= s;
  s = ((v & 0x0c) != 0) << 1;
  v >>= s;
  r |= s;
  s = ((v & 0x02) != 0) << 0;
  v >>= s;
  r |= s;
  return r;
}

// same as above but instead, return the value of the highest bit, not it's #
ACGLOBAL(uint8, highest_bit_val[],
         {0x00, 0x01, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
          0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
          0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
          0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
          0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
          0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
          0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
          0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
          0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80});

// same as above but invert the result (to save an ^0xff operation on masks)
ACGLOBAL(uint8, highest_bit_val_inv[],
         {0xff, 0xfe, 0xfd, 0xfd, 0xfb, 0xfb, 0xfb, 0xfb, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7, 0xf7,
          0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef, 0xef,
          0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf,
          0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf,
          0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
          0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
          0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
          0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf, 0xbf,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
          0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f});

/////////////////////////////////////  Macro Constants ////////////////////////////////////////////////////////////////////////

//////
// Used to decode MMU SLR
//                                8    4    2   1
#define FILTR 0xf00           //(2048+1024+512+256)
#define SLR_MASK 0xf00        //(2048+1024+512+256)  // f
#define SLR_RO_STK 0x400      //(     1024        )  // 4
#define SLR_RO_MEM 0x500      //(     1024+    256)  // 5
#define SLR_RW_STK 0x600      //(     1024+512    )  // 6
#define SLR_RW_MEM 0x700      //(     1024+512+256)  // 7
#define SLR_IO_SPACE 0x900    //(2048+         256)  // 8
#define SLR_UNUSED_PAGE 0xc00 //(2048+1024)          // c   1100= IO+RO (Read Only + IO)
#define SLR_SIO_SPACE 0xf00   //(2048+1024+512+256)  // f

#define SERIAL_PORT_A_DATA 0xFCD247
#define SERIAL_PORT_A_CONTROL 0xFCD243
#define SERIAL_PORT_B_DATA 0xFCD245
#define SERIAL_PORT_B_CONTROL 0xFCD241
#define IRQ_FLOPPY 1   // run of the mill floppy IRQ's.
#define IRQ_VIDEO 1    // not sure if this is correct
#define IRQ_FDIR_ON 40 // special cases.  These are for floppy fdir=1, fdir=0
#define IRQ_FDIR_OFF 41
#define IRQ_GOBYTE_0 42 // zero gobyte after some time

#define IRQ_SCC 6   // Serial port
#define IRQ_VIA1 2  // Keyboard/Mouse/Clock (COPS controller)
#define IRQ_COPS 2  // Keyboard/Mouse/Clock (COPS controller)
#define IRQ_VIA2 1  // Parallel Port VIA
#define IRQ_SLOT0 5 // Expasion Slot IRQ's
#define IRQ_SLOT1 4
#define IRQ_SLOT2 3

/*
*   7   NMI - Highest Priority                                                         *
*   6   RS-232 Ports                                                                   *
*   5   Expansion Slot 0                                                               *
*   4   Expansion Slot 1                                                               *
*   3   Expansion Slot 2                                                               *
*   2   Keyboard                                                                       *
*   1   All other internal interrupts -- lowest priority                               *
*                                                                                      *
*   Reset: Initial SSP               $000000           0                               *
*   Reset: Initial PC                $000004           1                               *
*   Bus Error:                       $000008           2                               *
*   Address Error:                   $00000C           3                               *
*   Illegal Instruction:             $000010           4                               *
*   Zero Divide:                     $000014           5                               *
*   CHK Instruction:                 $000018           6                               *
*   TRAPV Instruction:               $00001C                                           *
*   Privilidge Violation:            $000020                                           *
*   Trace:                           $000024                                           *
*   Unimplemented Instruction 1010   $000028                                           *
*   Unimplemented Instruction 1111   $00002C                                           *
*   Reserved/Unassigned:             $000030 - $00005F                                 *
*   Spurious Interrupt:              $000060           24                              *
*   Other Internal Interrupt:        $000064                                           *
*   Keyboard Interrupt:              $000068           1a/26                           *
*   Slot 2 Autovector:               $00006C           1b/27                           *
*   Slot 1 Autovector:               $000070           1c/28                           *
*   Slot 0 Autovector:               $000074           1d/29                           *
*   RS-232 Interrupt                 $000078           1e/30                           *
*   Non-Maskable Interrupt:          $00007C           1f/31                           *
*   Trap Instruction Vectors:        $000080 - $000BF                                  *
*   Reserved, Unassigned:            $0000C0 - $000Cf                                  *
*   User Interrupt Vectors:          $000100 - $003FF                                  *

*/

// this must be a power of 2 under 65536 and huge!  This is the size of the Interrupt Queue Ring buffer.
// since the lisa can shut off interrupts,  we need this to be huge.
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAXIRQQUEUE 64

/* Defines for the Via 6522 code -- MMU needs these to decypher the register accessed. */
// Expansion Port Slots.  PARPORTVER is the ROM version number.
#define PARPORTVER   \
  {                  \
    0x00, 01, 00, 07 \
  }
#define V0HVIA1BASE 0xFC2000
#define V0LVIA1BASE 0xFC2800
#define V1HVIA1BASE 0xFC4000
#define V1LVIA1BASE 0xFC4800
#define V2HVIA1BASE 0xFC8000
#define V2LVIA1BASE 0xFC8800

/* Definitions for VIA 1   -- COPS                                      */
#define VIA1BASE 0xFCDD80 /* Base address of VIA 1          */

// VIA1 registers are on the odd addresses in increments of 2.
#define ORB1 ((0 * 2) + 1) /* Port B output Register         */
#define IRB1 ((0 * 2) + 1)
#define ORA1 ((1 * 2) + 1) /* Port A output Register         */
#define IRA1 ((1 * 2) + 1)
#define DDRB1 ((2 * 2) + 1) /* Port B Data Direction Register */
#define DDRA1 ((3 * 2) + 1) /* Port A Data Direction Register */
#define T1CL1 ((4 * 2) + 1)
#define T1CH1 ((5 * 2) + 1)
#define T1LL1 ((6 * 2) + 1) /* Low Order T1 Latch             */
#define T1LH1 ((7 * 2) + 1) /* High Order T1 Latch            */
#define T2CL1 ((8 * 2) + 1) /* Low Order T2 Counter           */
#define T2CH1 ((9 * 2) + 1) /* Low Order T2 Counter           */
#define SR1 ((10 * 2) + 1)
#define ACR1 ((11 * 2) + 1)
#define PCR1 ((12 * 2) + 1)
#define IFR1 ((13 * 2) + 1)
#define IER1 ((14 * 2) + 1)
#define ORANH1 ((15 * 2) + 1)
#define IRANH1 ((15 * 2) + 1)

#define VVIA2BASE 0xFCD900 /* Base address of VIA 2          */

/* Definitions for VIA 2   -- Parallel Port  */
// VIA2 registers are on the odd addresses in increments of 8.
#define ORB2 ((0 * 8) + 1)
#define IRB2 ((0 * 8) + 1)
#define ORA2 ((1 * 8) + 1)
#define IRA2 ((1 * 8) + 1)
#define DDRB2 ((2 * 8) + 1)
#define DDRA2 ((3 * 8) + 1)
#define T1CL2 ((4 * 8) + 1)
#define T1CH2 ((5 * 8) + 1)
#define T1LL2 ((6 * 8) + 1)
#define T1LH2 ((7 * 8) + 1)
#define T2CL2 ((8 * 8) + 1)
#define T2CH2 ((9 * 8) + 1)
#define SR2 ((10 * 8) + 1)
#define ACR2 ((11 * 8) + 1)
#define PCR2 ((12 * 8) + 1)
#define IFR2 ((13 * 8) + 1)
#define IER2 ((14 * 8) + 1)
#define ORANH2 ((15 * 8) + 1)
#define IRANH2 ((15 * 8) + 1)

#define ORB 0
#define IRB 0
#define ORA 1
#define IRA 1
#define DDRB 2
#define DDRA 3
#define T1CL 4
#define T1CH 5
#define T1LL 6
#define T1LH 7
#define T2CL 8
#define T2CH 9
#define SHIFTREG 10
#define ACR 11
#define PCR 12
#define IFR 13
#define IER 14
#define ORANH 15
#define IRANH 15

// Shadow of T2
#define T2LL 16
#define T2LH 17

// Shadows of I/O regs
#define IRAA 18
#define ORAA 19
#define IRBB 20
#define ORBB 21

// Paralell port VIA PORT B bit definitions
#define OCDLine_BIT 1
#define BSYLine_BIT 2
#define DENLine_BIT 4
#define RRWLine_BIT 8
#define CMDLine_BIT 16
#define PARITY_BIT 32
#define DSK_DIAG_BIT 64
#define CTRL_RES_BIT 128

// if all are cleared, clear bit 7 else set it
// if (via[2].via[IER] & via[2].via[IFR] & 0x7f) via[2].via[IFR] |=0x80; // if any actively on, bit 7 is on.

#define FIX_VIA_IFR(vianum)         \
  {                                 \
    if (via[vianum].via[IFR] & 127) \
      via[vianum].via[IFR] |= 128;  \
    else                            \
      via[vianum].via[IFR] = 0;     \
  }

#define FIX_VIAP_IFR()     \
  {                        \
    if (V->via[IFR] & 127) \
      V->via[IFR] |= 128;  \
    else                   \
      V->via[IFR] = 0;     \
  }

#define IS_PARALLEL_PORT_ENABLED(vianum) (profile_power & (1 << (vianum - 2)))

#define VIA_IRQ_BIT_CA2 1           // cleared by read/write of reg1 (ora) unless CA2/CB2 in PCR set as independent irq - only writing to ifr will then clear it
#define VIA_IRQ_BIT_CA1 2           // cleared by read or write reg 1
#define VIA_IRQ_BIT_SR 4            // cleared by read/write of shift register
#define VIA_IRQ_BIT_CB2 8           // cleared by read or write orb - except if PCR sets it as independent IRQ like ca2
#define VIA_IRQ_BIT_CB1 16          // cleared by read/write orb
#define VIA_IRQ_BIT_T2 32           // read t2 low or write t2 high clears it
#define VIA_IRQ_BIT_T1 64           // read t1 low or wite t1 high clears it
#define VIA_IRQ_BIT_SET_CLR_ANY 128 // only cleared when all other IRQ's are cleared.

/* Timer ID's  */
#define CYCLE_TIMER_VIAn_T1_TIMER(x) ((x))
#define CYCLE_TIMER_VIAn_T2_TIMER(x) ((x) + 128)
#define CYCLE_TIMER_VIAn_SHIFTREG(x) ((x) + 64)

#define CYCLE_TIMER_VIA1_T1_TIMER (1)
#define CYCLE_TIMER_VIA1_T2_TIMER (1 + 128)
#define CYCLE_TIMER_VIA1_SHIFTREG (1 + 64)
#define CYCLE_TIMER_VIA1_CA1 (1 + 32) // for completion, not really used

#define CYCLE_TIMER_VIA2_T1_TIMER (2)
#define CYCLE_TIMER_VIA2_T2_TIMER (2 + 128)
#define CYCLE_TIMER_VIA2_SHIFTREG (2 + 64)
#define CYCLE_TIMER_VIA2_CA1 (2 + 32) // BSY

#define CYCLE_TIMER_VIA3_T1_TIMER (3)
#define CYCLE_TIMER_VIA3_T2_TIMER (3 + 128)
#define CYCLE_TIMER_VIA3_SHIFTREG (3 + 64)
#define CYCLE_TIMER_VIA3_CA1 (3 + 32) // BSY

#define CYCLE_TIMER_VIA4_T1_TIMER (4)
#define CYCLE_TIMER_VIA4_T2_TIMER (4 + 128)
#define CYCLE_TIMER_VIA4_SHIFTREG (4 + 64)
#define CYCLE_TIMER_VIA4_CA1 (4 + 32) // BSY

#define CYCLE_TIMER_VIA5_T1_TIMER (5)
#define CYCLE_TIMER_VIA5_T2_TIMER (5 + 128)
#define CYCLE_TIMER_VIA5_SHIFTREG (5 + 64)
#define CYCLE_TIMER_VIA5_CA1 (5 + 32) // BSY

#define CYCLE_TIMER_VIA6_T1_TIMER (6)
#define CYCLE_TIMER_VIA6_T2_TIMER (6 + 128)
#define CYCLE_TIMER_VIA6_SHIFTREG (6 + 64)
#define CYCLE_TIMER_VIA6_CA1 (6 + 32) // BSY

#define CYCLE_TIMER_VIA7_T1_TIMER (7)
#define CYCLE_TIMER_VIA7_T2_TIMER (7 + 128)
#define CYCLE_TIMER_VIA7_SHIFTREG (7 + 64)
#define CYCLE_TIMER_VIA7_CA1 (7 + 32) // BSY

#define CYCLE_TIMER_VIA8_T1_TIMER (8)
#define CYCLE_TIMER_VIA8_T2_TIMER (8 + 128)
#define CYCLE_TIMER_VIA8_SHIFTREG (8 + 64)
#define CYCLE_TIMER_VIA8_CA1 (8 + 32) // BSY

#define CYCLE_TIMER_VERTICAL_RETRACE (11)
#define CYCLE_TIMER_COPS_MOUSE_IRQ (12)
#define CYCLE_TIMER_COPS_CLOCK_DSEC (13)
#define CYCLE_TIMER_FDIR (14)

#define CYCLE_TIMER_Z8530 (15)

#define CYCLE_TIMER_SCC_B_XMT_BUF_EMPTY (16 + 0) // Z8530 Channel B Transmit Buffer Empty
#define CYCLE_TIMER_SCC_B_EXT_STAT_CHG (16 + 1)  // Z8530 Channel B External/Status Change
#define CYCLE_TIMER_SCC_B_RCVD_CHAR (16 + 2)     // Z8530 Channel B Receive Character Avail
#define CYCLE_TIMER_SCC_B_SPECIAL (16 + 3)       // Z8530 Channel B Special Receive Condition

#define CYCLE_TIMER_SCC_A_XMT_BUF_EMPTY (20 + 0) // Z8530 Channel A Transmit Buffer Empty
#define CYCLE_TIMER_SCC_A_EXT_STAT_CHG (20 + 1)  // Z8530 Channel A External/Status Change
#define CYCLE_TIMER_SCC_A_RCVD_CHAR (20 + 2)     // Z8530 Channel A Receive Character Avail
#define CYCLE_TIMER_SCC_A_SPECIAL (20 + 3)       // Z8530 Channel A Special Receive Condition

///////////////////////////////////////////////// Type definitions /////////////////////////////////////////////////
//
// 64 bit values might not be available on all host systems.  They seem to be defined on modern ones, although I've
// noticed buggy/limited support for them when using shift operations.  However, as long as addition/substraction
// and multiplication works, and they're built in to your CPU, they can be used to gain an advantage with the timer
// code.  int32's can also be used by the timer code, however they will cause a slowdown every 5 minutes of guest
// runtime, and at every event as the timers are checked for overflow.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define USE64BITTIMER 1

// HALF_CLK needs to be the highest bit we can go before the value becomes negative. i.e. for 32 bit values, bit 30.
// for 64 bit values, bit 62 (since the msb is the sign bit)
//
//------------------------------------------------------------------------------------------------------------------------
//
// I'd love to change these to int64's and HALF_CLK to (1<<62), however, it seems 64 bit shifts are a bit buggy at the moment
// on Linux IA32 with debian gcc 4.0.3-1

// #ifdef XXBROKENXX
#ifdef USE64BITTIMER
//  0123456789012345 - careful, want to use 1<<62 however, found bugs in shifting with gcc 4!
#define HALF_CLK (0x4000000000000000)
#define XTIMER int64
// #define prevent_clk_overflow_now(x) {1}
// #define prevent_clk_overflow(x) {1}
#define USING64BITTIMER 1

#else

#define HALF_CLK (1 << 30)
#define XTIMER int32
#define CLKDIV2(x) ((x) >>= 1)
// #define CLKDIV2(x) ((x)/=2)
extern void prevent_clk_overflow_now(void);

#endif

extern void init_ipct_allocator(void);

GLOBAL(uint8, *lisaram, NULL); // pointer to Lisa RAM

// this enables a hack that tricks the lisa into skipping the full ram test, thus speeding up
// the boot process - this sets a PRAM variable saying RAM test is done.
GLOBAL(int, cheat_ram_test, 1);       // careful if we change the type of this: `extern "C" float hidpi_scale;` in LisaConfigFrame.cpp also
DECLARE(int, hle);                    // flag to enable HLE hacks
GLOBAL(uint32, bootblockchecksum, 0); // checksum of bootsector (sector 0) whether from profile or floppy.
DECLARE(int, macworks4mb);
DECLARE(int, consoletermwindow);           // preference: enable TerminalWx window for console terminal (UniPlux, LPW, Xenix, etc.)
GLOBAL(int, romless, 0);                   // are we romless?
GLOBAL(int, xenix_patch, 1);               // 2022.03.06 flag to signal Xenix HLE patches
GLOBAL(int, macworks_hle, 1);              // 2025.12.27 this flag is no-longer used. See https://github.com/arcanebyte/lisaem/issues/40
GLOBAL(int, los31_hle, 1);                 // 2021.04.14 flag to signal LOS 3.1 has been patched for HLE
GLOBAL(int, monitor_patch, 1);             // 2022.03.06 flag to signal Monitor 12.x has been patched for HLE
GLOBAL(int, uniplus_hacks, 1);             // 2021.03.05 flag to signal that UniPlus has been patched for profile handshaking
GLOBAL(int, uniplus_loader_patch, 1);      // 2021.03.17 flag to signal that UniPlus boot loader has been patched for profile handshaking
GLOBAL(int, uniplus_sunix_patch, 1);       // 2021.03.18 flag to signal that UniPlus sunix v1.1 kernel (used for installing) has been patched for profile handshaking
GLOBAL(uint32, rom_profile_read_entry, 0); // ROM entry to profile read block
GLOBAL(int, double_sided_floppy, 0);       // 2021.03.18 flag to signal that UniPlus sunix v1.1 kernel (used for installing) has been patched for profile handshaking

// other globally saved defaults
GLOBAL(int, sound_effects_on, 1);
GLOBAL(int, profile_power, 127);

DECLARE(float, hidpi_scale); // time to support HiDPI displays and "Retina" displays, yeay!  Party like it's 2012!
DECLARE(float, mouse_scale);
DECLARE(int, hide_host_mouse);
DECLARE(int, skins_on);
DECLARE(int, skins_on_next_run);
DECLARE(int, skinless_center);
DECLARE(int, lisa_ui_video_mode);
DECLARE(uint32, refresh_rate_used);

typedef uint8 lisa_mem_t; /* type indicator for the above #defines for use with fn's           */

typedef struct
{
  uint32 size;
  uint8 *buffer;
  uint32 start;
  uint32 end;
} FLIFLO_QUEUE_t;

typedef struct
{
  int16 x;
  int16 y;
  int8 button; // 0=no click change, 1=down, -1=up.
} mousequeue_t;

typedef struct
{
  int8 Command;           // what command is the profile doing:
                          // -1=disabled, -2=idle, 1=read, 2=write, 3=write verify
                          //
  uint8 StateMachineStep; // what step of Command is the state machine of this profile in?

  uint8 DataBlock[4 + 8 + 532 + 8 + 2]; // 4 status bytes, command block, data block, 2 for good luck ;)

  uint16 indexread; // indeces into the buffer for the Lisa to Read or Write
  uint16 indexwrite;
  uint32 blocktowrite; // used by the write command to store the block number to write

  // uint32  numblocks;                   // (24 bit value!) size of this profile in blocks.
  //    (9728=5mb, 532 bytes/sector)
  //  We shouldn't make this more than 10Mb until we've tried it.

  // Control Lines from 6522 (except OCDLine which is Command=-1)
  uint8 CMDLine; // set by Lisa
  uint8 BSYLine; // set by ProFile
  uint8 DENLine; // set by Lisa (drive enabled)
  uint8 RRWLine; // set by Lisa (read or write)

  uint8 VIA_PA; // data to from VIA PortA (copied to/from V->via[0])
  uint8 last_a_accs;
  int32 last_reset_cpuclk; // last reset occured at what cpu clock?

  DC42ImageType DC42; // actual storage container

  // DC42 contains the ProFilename and file handler
  // char  ProFileFileName[FILENAME_MAX]; // the file name for this Profile disk image to open;

  XTIMER clock_e;     // used for timeouts - this is in relation to cpu68k_clocks
  XTIMER alarm_len_e; // used for timeouts - how long was the delay set for in clock_e event expiration

  int vianum;
  uint16 last_cmd;
} ProFileType;

typedef struct
{
  int8 Command;           // what command is the profile doing:
                          // -1=disabled, -2=idle, 1=read, 2=write, 3=write verify
                          //
  uint8 StateMachineStep; // what step of Command is the state machine of this profile in?

  uint8 DataBlock[4 + 8 + 532 + 8 + 2]; // 4 status bytes, command block, data block, 2 for good luck ;)

  uint16 indexread; // indeces into the buffer for the Lisa to Read or Write
  uint16 indexwrite;
  uint32 blocktowrite; // used by the write command to store the block number to write

  // uint32  numblocks;                   // (24 bit value!) size of this profile in blocks.
  //    (9728=5mb, 532 bytes/sector)
  //  We shouldn't make this more than 10Mb until we've tried it.

  // Control Lines from 6522 (except OCDLine which is Command=-1)
  uint8 CMDLine; // set by Lisa
  uint8 BSYLine; // set by ProFile
  uint8 DENLine; // set by Lisa (drive enabled)
  uint8 RRWLine; // set by Lisa (read or write)

  uint8 VIA_PA; // data to from VIA PortA (copied to/from V->via[0])
  uint8 last_a_accs;
  int32 last_reset_cpuclk; // last reset occured at what cpu clock?

  DC42ImageType DC42; // actual storage container

  // DC42 contains the ProFilename and file handler
  // char  ProFileFileName[FILENAME_MAX]; // the file name for this Profile disk image to open;

  XTIMER clock_e;     // used for timeouts - this is in relation to cpu68k_clocks
  XTIMER alarm_len_e; // used for timeouts - how long was the delay set for in clock_e event expiration

  int vianum;
  uint16 last_cmd;
} WidgetType;

typedef struct
{
  uint8 active; // is this VIA active? (except via1 and via2 s/b always active)
                // make sure that the initialization code sets these anyway!!!

  uint8 vianum; // Which VIA is this?

  uint8 via[22]; // standard via registers + timer 2 shadows and actual IRA/ORA/IRB/IRA shadows, etc.

  int last_a_accs; // was the last access to port A a read or a write? (for DDRA flips from 00 to ff)
                   // I've seen some code that writes to port A, then sets DDRA to FF, only at that point
                   // is the data put on the port.  I've seen other code that does the opposite, so I need
                   // some way to distinguish what the value in PORT A is from in order to output properly

  int orapending; // there are many ways to output data.  some OS's set DDRA=0, then write to ORA, then
                  // set DDRA=FF to output.  Others set DDRA=FF 1st, then write to ORA.  When DDRA=0 and
                  // a write is made to ora, the data cannot be placed on PA, but as soon as DDRA=FF, the
                  // PA gets data out and CA2 is set.  So to avoid a bug where an extra ORA is made when
                  // DDRA=FF, ORA is written, DDRA=0, then DDRA=FF, this keeps track of the pending.

  uint8 last_port;// Some Lisa software (e.g. MacWorksXL3.0 boot routines) write the same byte to the VIA twice: 
                  // once to port ORANH2 (NH means "no handshake"), and then again to ORA2. 
                  // We need to ignore the 2nd write, otherwise Lisa does not boot (we get a Lisa error 75).
                  // Here we store the last port used, so that when writing to ORA2 we could check if the last
                  // port was ORANH2 and the last value was the same, and ignore the write. 

  XTIMER t1_e; // e_clock trigger - at what point will the via clock expire vs cpu68k_clocks
  XTIMER t1_fired;
  XTIMER t2_e;
  XTIMER t2_fired;
  XTIMER sr_e;
  XTIMER sr_fired;

  XTIMER last_pa_write; // used to find the end of print jobs

  // #ifdef DEBUG
  XTIMER t1_set_cpuclk;
  XTIMER t2_set_cpuclk;
  XTIMER t1_fired_cpuclk;
  XTIMER t2_fired_cpuclk;
  // #endif

  ProFileType *ProFile; // If there's a ProFile attached, this structure deals with it.
  int ADMP;             // If there's an Apple Dot Matrix Printer (same as IW, but parallel)

  uint8 irqnum; // Interrupt number for this VIA
  uint8 srcount;
  uint8 ca1, ca2, cb1, cb2; // port A,B control lines

  uint8 (*irb)(uint8 vianum, uint8 reg); // Function Pointers to the Handlers for what's connected to the VIA's.
  void (*orb)(uint8 vianum, uint8 data, uint8 reg);
  uint8 (*ira)(uint8 vianum, uint8 reg);
  void (*ora)(uint8 vianum, uint8 data, uint8 reg);
} viatype;

// What can I say, I'm a lazy fuck, I don't like to worry about the case of these.
#define ViaType viatype
#define VIAType viatype
#define VIAtype viatype

GLOBAL(int32, video_scan, 0);

GLOBAL(int, lisa_vid_size_x, 720);
GLOBAL(int, lisa_vid_size_y, 364);
GLOBAL(int, lisa_vid_size_xbytes, 90);
GLOBAL(int, has_lisa_xl_screenmod, 0);

// used by mouse routines to detect mouse acceleration undo strategy and other stuff
#define LISA_ROM_RUNNING 0
#define LISA_OFFICE_RUNNING 1
#define LISA_TEST_RUNNING 2
#define LISA_MACWORKS_RUNNING 3
#define LISA_MONITOR_RUNNING 4
#define LISA_XENIX_RUNNING 5
#define LISA_UNIPLUS_RUNNING 6
#define LISA_UNIPLUS_SUNIX_RUNNING 7
#define UNKNOWN_OS_RUNNING 100

GLOBAL(int, running_lisa_os, LISA_ROM_RUNNING);
int check_running_lisa_os(void);
GLOBAL(int, mouse_x_tolerance, 0);
GLOBAL(int, mouse_y_tolerance, 0);

GLOBAL(int, mouse_x_halfing_tolerance, 1);
GLOBAL(int, mouse_y_halfing_tolerance, 1);

// can turn these into a pointer and several arrays for various OS's.
GLOBAL(int, anti_jitter_sample_dec1[], {64, 32, 16, 8, 4, 2, 0, 0, 0});
GLOBAL(int, anti_jitter_sample_dec2[], {1, 2, 4, 16, 32, 64, 0, 0, 0});

GLOBAL(int, anti_jitter_decellerate_xt[], {8, 32, 64, 0});
GLOBAL(int, anti_jitter_decellerate_xn[], {1, 2, 4, 0});
GLOBAL(int, anti_jitter_decellerate_yt[], {8, 32, 64, 0});
GLOBAL(int, anti_jitter_decellerate_yn[], {1, 2, 3, 0});

// GLOBAL(int,via_port_idx_bits[],{0, 2,1, 4,3, 6,5});
// 2, 4,3, 6,5, 8,7
GLOBAL(uint32, lisa_os_mouse_x_ptr, 0x486);
GLOBAL(uint32, lisa_os_mouse_y_ptr, 0x488);
GLOBAL(uint32, lisa_os_boot_mouse_x_ptr, 0x486);
GLOBAL(uint32, lisa_os_boot_mouse_y_ptr, 0x488);
GLOBAL(int8, floppy_picked, 1); // 2006.06.11 - if 1 enable profile access immediately
GLOBAL(uint8, floppy_iorom, 0);
typedef struct _lisa_clock
{
  uint8 year;
  uint8 days_h;
  uint8 days_l;
  uint8 hours_h;
  uint8 hours_l;
  uint8 mins_h;
  uint8 mins_l;
  uint8 secs_h;
  uint8 secs_l;
  uint8 tenths;
} t_lisa_clock;

DECLARE(t_lisa_clock, lisa_clock);

GLOBAL(int32, lisa_alarm, 0);
GLOBAL(uint8, lisa_clock_set_idx, 0);
GLOBAL(uint8, lisa_alarm_power, 0);
GLOBAL(uint8, lisa_clock_on, 1);
DECLARE(uint8, eparity[256]);
DECLARE(uint8, lisa_clock_set[16]);

// Instruction Parameter Cache
typedef struct _t_ipc
{
  void (*function)(struct _t_ipc *ipc); // 8/4    // pointer to the function that executes this opcode
  uint8 used;                           // 1      // bitmap of XNZVC flags inspected
  uint8 set;                            // 1      // bitmap of XNZVC flags altered
  uint16 opcode;                        // 2      // absolutely necessary - the opcode itself i.e. 0x4e75=RTS

  uint16 wordlen; // 2      // might be able to delete this if we also add nextpc, but diss68k will need work as will
                  //  lots of mods of cpu68.k  To get it call iib->wordlen  but the illegal instruction in
                  //  there needs checking.

  unsigned int : 0; // 3      // what's this here for?  alignment for the next two
  uint32 src;       // 4
  uint32 dst;       // 4

  // //  // // // // // // // // // // // // // these are extensions to the normal generator code in order to refine it, etc.

  uint16 sreg, dreg; // 2      // for cpu68k-inline idx_val macros/inlines replacing the bug that causes the
                     //  high octet in PC (bits 31-24) to be filled, then causes negative PC on sign ext.

  uint8 clks; // 1      // might be able to remove this if I can get this from iib without too much of a slowdown - maybe

  struct _t_ipc *next; // 8/4    // next ipc in chain. we could get rid of this, but then things would run slower
                       //  and we'd need to do a whole lot of more work to get things to work.
                       //  Need to get rid of this, but have to redo stuff in cpu68k.c to do it.
  // //  // // // // // // // // // // // // // /////////////////////////////////////////////////////////////////////////////
} t_ipc;

typedef struct _t_ipc_table
{
  // Pointers to all the IPC's in this page.  Since the min 68k opcode is 2 bytes in size
  // the most you can have are 256 instructions per page.  We thus no longer need a hash table
  // nor any linked list of IPC's as this is a direct pointer to the IPC.  Ain't life grand?

  t_ipc ipc[256];
  struct _t_ipc_table *next;
  //    } t;
  int used;
  int context;
  uint32 address;

#ifdef PROCESSOR_ARM
  void (*compiled)(struct _t_ipc *ipc);
  // uint8 norepeat;    // what's this do? this only gets written to, but not read.  Maybe Arm needs it?
#endif
} t_ipc_table;

//////////////////////////////////////////////////////////
//
// MMU Translation Table for page.  This provides function pointers to read/write handlers as well as address translation
// via the address offset (signed) integer which gets added to the address.
//
// The table is an array of 256's ipc's -- if NULL it hasn't been allocated for this page.  The table is a pointer to
// an element of a linked list of IPC tables.
//
// For a 2MB (fully loaded non-XL 4MB hacked up Lisa) you have upto: 2097152 bytes of RAM + 16384ROM -> that's
// upto 2113536 bytes maximum of executable code (most of it won't be executable.)
//
// In terms of 512 byte MMU pages, this is: 4128 pages.  Each ipc structure takes about 32 bytes * 256/page = 8K * 4128 pages
// we have a maximum of 32MB (it's much smaller than this since not all of the Lisa's memory will be IPC's.)
// Yup, this is one memory hungry emulator....  So we need decent memory management on the MMU+ipct's.
//
// Why 256 ipc's/page? Simple: smallest 68k opcode is 2 bytes, there are 512/page, so at most there are 256 ipc's/page.
// But because management of these is not trivial, we allocate 256 of them per page if they're used.
//
// Note that the IPC's point to the virtual, not physical pages.

typedef struct _mmu_trans_t
{
  int32 address;      /* quick ea translation. Just add to lower bits  - needs to be signed */
  lisa_mem_t readfn;  /* index to read and write fn's for that segment, that way I            */
  lisa_mem_t writefn; /* can have read only segments without doing special checking.          */
  t_ipc_table *table; /* Pointer to a table of IPC's or NULL if one hasn't been assigned.     */
} mmu_trans_t;

// Lisa's MMU table.  This get's converted to mmu_trans table above on a per/page basis.
typedef struct
{
  uint16 sor, slr; // real sor, slr
  uint8 changed;   // this is a flag to let us know that an mmu segment has changed.
                   // come back later to correct it. bit 0=newslr set, bit 1=newsor set.
} mmu_t;

GLOBAL(uint8, lastsflag, 0);

GLOBAL(uint8, floppy_FDIR, 0);
GLOBAL(uint8, floppy_6504_wait, 1);
GLOBAL(uint8, floppy_irq_top, 1);
GLOBAL(uint8, floppy_irq_bottom, 1); // interrupt settings (are floppies allowd to interrupt)
DECLARE(uint8, floppy_ram[2048]);

GLOBAL(uint32, mmudirty, 0);
DECLARE(uint32, mmudirty_all[5]);

DECLARE(uint8, lisarom[0x4000]);       // space for the MC68000 ROM
DECLARE(uint8, dualparallelrom[2048]); // rom space for the dual parallel card

DECLARE(uint8, dirtyvidram[32768]);

GLOBAL(uint32, segment1, 0); // MMU related bits
GLOBAL(uint32, segment2, 0);
GLOBAL(uint32, context, 0);
GLOBAL(uint32, lastcontext, 0);

GLOBAL(uint32, address32, 0); // not sure that this is needed anymore
GLOBAL(uint32, address, 0);
GLOBAL(uint32, mmuseg, 0);
GLOBAL(uint32, mmucontext, 0);
GLOBAL(uint32, transaddress, 0);

// special memory latches toggled by i/o space addresses in the lisa.
GLOBAL(uint32, diag1, 0);
GLOBAL(uint32, diag2, 0);
GLOBAL(uint32, start, 1);
GLOBAL(uint32, softmem, 0);
GLOBAL(uint32, vertical, 0);
GLOBAL(uint32, verticallatch, 0);
GLOBAL(uint32, hardmem, 0);
GLOBAL(uint32, videolatch, 0x2f);
GLOBAL(uint32, lastvideolatch, 0x2f);
GLOBAL(uint32, videolatchaddress, (0x2f * 32768));
GLOBAL(uint32, lastvideolatchaddress, 0x2f * 32768);
GLOBAL(uint32, statusregister, 0);
GLOBAL(uint32, videoramdirty, 0);
GLOBAL(uint32, videoximgdirty, 0);
GLOBAL(uint16, memerror, 0);

// hack for intermediate RC2 version - stolen from LisaCanvas.cpp as an ugly quick and dirty hack
GLOBAL(int, dirty_x_min, 720);
GLOBAL(int, dirty_x_max, 0);
GLOBAL(int, dirty_y_min, 364);
GLOBAL(int, dirty_y_max, 0);

GLOBAL(int, e_dirty_x_min, 720);
GLOBAL(int, e_dirty_x_max, 0);
GLOBAL(int, e_dirty_y_min, 500);
GLOBAL(int, e_dirty_y_max, 0);

GLOBAL(uint8, contrast, 0xff); // 0xff=black 0x80=visible 0x00=all white
GLOBAL(uint8, volume, 4);      // 0x0e is the mask for this.
GLOBAL(long, *dtc_rom_fseeks, NULL);
GLOBAL(FILE, *rom_source_file, NULL);

GLOBAL(int, debug_log_enabled, 0);
GLOBAL(int, debug_log_onclick, 0);

// CPU_CORE_TESTER
GLOBAL(int, debug_log_cpu_core_tester, 0);
GLOBAL(int, on_click_debug_log_cpu_core_tester, 0);

GLOBAL(int, dbx, 0);
GLOBAL(FILE, *buglog, NULL);

#ifdef DEBUG

#define PAUSE_DEBUG_MESSAGES() \
  {                            \
    dbx |= debug_log_enabled;  \
    debug_log_enabled = 0;     \
  }
#define RESUME_DEBUG_MESSAGES() \
  {                             \
    debug_log_enabled |= dbx;   \
    dbx = 0;                    \
  }
#else
#define PAUSE_DEBUG_MESSAGES() \
  {                            \
    ;                          \
  }
#define RESUME_DEBUG_MESSAGES() \
  {                             \
    ;                           \
  }
#endif

#ifdef DEBUG
#ifdef DEBUG_ON_SCREENHASH
GLOBAL(uint8, debug_hash[], DEBUG_ON_SCREENHASH);
#endif
#ifdef DEBUG_OFF_SCREENHASH
GLOBAL(uint8, debug_hash_off[], DEBUG_OFF_SCREENHASH);
#endif
#endif

// hexadecimal conversion table table.
ACGLOBAL(char, *hex, "0123456789abcdef");

/****************** COPS.C definitions/vars visible to outside world... ************************************/

#define MAXCOPSQUEUE 8192
#define MAXMOUSEQUEUE 16

GLOBAL(int16, copsqueuelen, 0);
DECLARE(uint8, copsqueue[MAXCOPSQUEUE]);
GLOBAL(uint8, NMIKEY, 0);
GLOBAL(uint8, cops_powerset, 0);
GLOBAL(uint8, cops_clocksetmode, 0);
GLOBAL(uint8, cops_timermode, 0);
GLOBAL(int8, mouse_pending, 0);
GLOBAL(int8, mouse_pending_x, 0);
GLOBAL(int8, mouse_pending_y, 0);
GLOBAL(int16, last_mouse_x, 0);
GLOBAL(int16, last_mouse_y, 0);
GLOBAL(int16, last_mouse_button, 0);
GLOBAL(int16, mousequeuelen, 0);
DECLARE(mousequeue_t, mousequeue[MAXMOUSEQUEUE]);

// How many percent of the current IPCT's should we allocate.
#define IPCT_ALLOC_PERCENT 20

// What's the maximum time's we'll call malloc?
#define MAX_IPCT_MALLOCS 32

// we should never need to even do more than the initial malloc if initial_ipcts=4128.
DECLARE(t_ipc_table, *ipct_mallocs[MAX_IPCT_MALLOCS]);

// size of each of the above mallocs - not really needed, but could be useful for debugging.
DECLARE(uint32, sipct_mallocs[MAX_IPCT_MALLOCS]);

DECLARE(t_ipc_table, *ipct_mallocs[MAX_IPCT_MALLOCS]);
DECLARE(uint32, sipct_mallocs[MAX_IPCT_MALLOCS]);
GLOBAL(uint32, iipct_mallocs, 0);
GLOBAL(uint32, ipcts_allocated, 0);
GLOBAL(int64, ipcts_used, 0);
GLOBAL(int64, ipcts_free, 0);
GLOBAL(t_ipc_table, *ipct_free_head, NULL);
GLOBAL(t_ipc_table, *ipct_free_tail, NULL);
#ifdef DEBUG
GLOBAL(t_ipc_table, *ipct_used_head, NULL);
GLOBAL(t_ipc_table, *ipct_used_tail, NULL);
//^- if we do this will also need to add a previous link
#endif

/* (2MB RAM max+ 16KROM)=2113536 bytes of potentially executable code divided by 512(bytes/mmu page) = 4128
 *  ipc's page = maximum= 8256 ipct's is maximum - should not have to go above this ever.
 *  this value is only for testing, until we test the LisaEM under heavy load to find the actual used number of ipct's
 *  and lessen it to free things up.  remember, each ipct holds 256 ipcs plus extra info.  */

GLOBAL(uint32, initial_ipcts, 4128);

// 212,179 ->missing 18 lines! 18 lines is the entire retrace cycle!

#define CYCLES_PER_LINE (212)                     // was212               //213       /* (720+176)/(20.375Mhz/5Mhz) .. =219.87 was 224*/
#define VISIBLE_LINE_CYCLES (179)                 // 180
#define FULL_FRAME_CYCLES (CYCLES_PER_LINE * 364) //-4330)   /* 364 lines visible  -17 added 2005.04.14 10:51am */
#define HALF_FRAME_CYCLES (FULL_FRAME_CYCLES / 2)
#define QUARTER_FRAME_CYCLES (FULL_FRAME_CYCLES / 4)
// #define VERT_RETRACE_ON           (450)                 //450   //-VERT_RETRACE_ON-250
#define VERT_RETRACE_CYCLES_X ((CYCLES_PER_LINE * (380 - 364))) //-VERT_RETRACE_ON)  //-VERT_RETRACE_ON)   //2005.05.04 - change it to 16  was 15  /* 15 retrace lines  */
#define VERT_RETRACE_ON (VERT_RETRACE_CYCLES_X / 2)
#define VERT_RETRACE_CYCLES (VERT_RETRACE_CYCLES_X / 2)

#define ONE_SECOND 5000000
// #define ONE_SECOND                5093750

#define COPS_IRQ_TIMER_FACTOR 20375 // was 20000
#define THIRTY_SECONDS (ONE_SECOND * 30)
#define TWENTY_SECONDS (ONE_SECOND * 20)
#define FIFTEEN_SECONDS (ONE_SECOND * 15)
#define TEN_SECONDS (ONE_SECOND * 10)
#define FIVE_SECONDS (ONE_SECOND * 5)
#define MILLIONTH_OF_A_SECOND (ONE_SECOND / 1000000)
#define HUN_THOUSANDTH_OF_A_SEC (ONE_SECOND / 100000)
#define TEN_THOUSANDTH_OF_A_SEC (ONE_SECOND / 10000)
#define THOUSANDTH_OF_A_SECOND (ONE_SECOND / 1000)
#define HUNDREDTH_OF_A_SECOND (ONE_SECOND / 100)
#define TENTH_OF_A_SECOND (ONE_SECOND / 10)

#define HALF_OF_A_SECOND (ONE_SECOND / 2)
#define THIRD_OF_A_SECOND (ONE_SECOND / 3)
#define QUARTER_OF_A_SECOND (ONE_SECOND / 4)
#define FIFTH_OF_A_SECOND (ONE_SECOND / 5)
#define EIGHTH_OF_A_SECOND (ONE_SECOND / 8)

#define REFRESHRATE (ONE_SECOND / 60)

// old versions of above - likely wrong
// #define FULL_FRAME_CYCLES    65520
// #define HALF_FRAME_CYCLES    32760
// #define QUARTER_FRAME_CYCLES 16380
// #define VERT_RETRACE_CYCLES   2700
// #define COPS_IRQ_TIMER_FACTOR     20000
// #define HUNDRETH_OF_A_SECOND      50000
// #define TENTH_OF_A_SECOND        500000
// #define ONE_SECOND              5000000

// number of CPU cycles between transmitting a character of the SCC and the count zero interrupt.
#define Z8530_XMIT_DELAY 1000

GLOBAL(XTIMER, lastrefresh, 0);
GLOBAL(XTIMER, virq_start, FULL_FRAME_CYCLES);
GLOBAL(XTIMER, fdir_timer, -1);
GLOBAL(XTIMER, cpu68k_clocks_stop, ONE_SECOND);
GLOBAL(XTIMER, cpu68k_clocks, 0);
GLOBAL(XTIMER, lasttenth, 0);
GLOBAL(XTIMER, clktest, 0);
GLOBAL(XTIMER, cops_event, -1);
GLOBAL(XTIMER, tenth_sec_cycles, TENTH_OF_A_SECOND); // 10th of a second cycles.  5,000,000 cycles/sec so 500000 10ths/sec
GLOBAL(XTIMER, z8530_event, -1);
GLOBAL(XTIMER, cops_mouse, (COPS_IRQ_TIMER_FACTOR * 4));

DECLARE(int, irqs[7]); // flagged IRQs to fire

#define KBCOPSCYCLES 6350
                       // if mouse is enabled, next clock event is cops_mouse.
                       // if copsqueuelen - that means we have data, and either cops event is off, or it's further
                       // away, then set it closer.

#define SET_COPS_NEXT_EVENT(x)                                                                 \
  {                                                                                            \
    cops_event = (cops_mouse ? (cpu68k_clocks + cops_mouse) : -1);                             \
    if (copsqueuelen > 0 && ((cops_event > (cpu68k_clocks + KBCOPSCYCLES)) || cops_event < 0)) \
      cops_event = cpu68k_clocks + KBCOPSCYCLES;                                               \
    DEBUG_LOG(0, "SET_COPS_NEXT:copsqueuelen:%d cops_mouse:%ld cops_event:%016llx    \n",      \
              copsqueuelen, cops_mouse, cops_event);                                           \
  }

//         ***** still need logic for when cops_event=-1 but there's now data to deal with
//                 i.e. need to make sure timer is turned on.  likely can change the send macro to check
//
// if (cops_event<0) SET_COPS_NEXT_EVENT(0);

GLOBAL(uint32, via_clock_diff, 2); // 2

// These should be external as they belong to generator
#ifndef IN_CPU68K_C
extern unsigned long cpu68k_frames;
extern unsigned long cpu68k_frozen;
extern int abort_opcode;
#ifdef DEBUG
extern int do_iib_check;

#endif
#endif

// recheck these for correctness!

// 0x3100=12544
// X  =  100000/12544 [7.97193877551]
// Hmmm, seems it's better to use 8 as a timing factor
GLOBAL(float, via_throttle_factor, 1.0);

// something is broken with the VIA timing, no matter what I do with it, the clock is still skewed.
#ifdef TIE_VIA_TIMER_TO_HOST

#define LISA2ECLKFACTOR (XTIMER)((float)10 * (via_throttle_factor)) // 10 //( 7.750950) //8.117647058825 //.11764705882=5,5  83=5,7 //.1176470588=5,5  .1176470589=5,7  .117647058[7|5]=5,5 585=5,5  57=5,5 9=5,7 //.117647055=5,5  .11764706[5|3|1]=5,7 .11764706=5,7  .11764707=5,7  .11764705=5,5 .1176471=5,7 72=5,7  .1176470=5,5 .117647[3|5]=5,7  //.117648=5,7  //.117647=5,5 //.117649=5,7  //117645=5,5 //1176[3/4]=5,5 //1165=5,7// 1176=5,5 //.1178/7=5,7 // 1175=5,5 //118=5,7 //117=5,5 //.119=5,7 // 8.113/114/115=5,5 // 8.11=5,5 // 8.12=5,7 // 8.16,14=5,7// 8.18=5,7 // 8.20=5,7 // 8.1=5,5 //8.0=5,5 // 8.3 5,7 // 8.25=5,7 //  8.2=5,7 // 8.1=7,5  // 9 5,7 //7.9=5,5  //8=5.5  //7.8=5.5  //8             // 10  // 8
#define LISA1ECLKFACTOR (XTIMER)((float)4 * (via_throttle_factor))  //( 3.263358)

#else

#define LISA2ECLKFACTOR 10 // 10 //( 7.750950) //8.117647058825 //.11764705882=5,5  83=5,7 //.1176470588=5,5  .1176470589=5,7  .117647058[7|5]=5,5 585=5,5  57=5,5 9=5,7 //.117647055=5,5  .11764706[5|3|1]=5,7 .11764706=5,7  .11764707=5,7  .11764705=5,5 .1176471=5,7 72=5,7  .1176470=5,5 .117647[3|5]=5,7  //.117648=5,7  //.117647=5,5 //.117649=5,7  //117645=5,5 //1176[3/4]=5,5 //1165=5,7// 1176=5,5 //.1178/7=5,7 // 1175=5,5 //118=5,7 //117=5,5 //.119=5,7 // 8.113/114/115=5,5 // 8.11=5,5 // 8.12=5,7 // 8.16,14=5,7// 8.18=5,7 // 8.20=5,7 // 8.1=5,5 //8.0=5,5 // 8.3 5,7 // 8.25=5,7 //  8.2=5,7 // 8.1=7,5  // 9 5,7 //7.9=5,5  //8=5.5  //7.8=5.5  //8             // 10  // 8
#define LISA1ECLKFACTOR 4  //( 3.263358)

#endif
// #define LISA1ECLKFACTOR (24.117647058825) //24.117647058825 //.11764705882=5,5  83=5,7 //.1176470588=5,5  .1176470589=5,7  .117647058[7|5]=5,5 585=5,5  57=5,5 9=5,7 //.117647055=5,5  .11764706[5|3|1]=5,7 .11764706=5,7  .11764707=5,7  .11764705=5,5 .1176471=5,7 72=5,7  .1176470=5,5 .117647[3|5]=5,7  //.117648=5,7  //.117647=5,5 //.117649=5,7  //117645=5,5 //1176[3/4]=5,5 1165=5,7// 1176=5,5 //.1178/6=5,7 // 1175=5,5 //118=5,7 //117=5,5 //         //24.113/114/115=5,5//24.11=,5   //24.12=5,7 //24.16,14=5,7 //24.18=5.7//24.20=5,7 //24.1=5,5 //24.0=5,5  24.3 5,7 //24.25=5,7 // 24.2=5,7 //24.1=7,5  //25 5,7 //23.9=5,5 //24=5.5 //23.8=5.5 //23.3=err 5 //23            // 25  // 20    needs to be 22.5 or 7.5
//  above might need to be fractional?

// 5Mhz / 10 = 500Khz    every 10 cpu cycles
// 5Mhz /4   = 1.25Mhz   every  4 cpu cycles
//
#define VIACLK_TO_CPUCLK(X) (((floppy_ram[0x18] & 96) == 32) ? ((X) * (XTIMER)(LISA2ECLKFACTOR)) : ((X) * (XTIMER)(LISA1ECLKFACTOR)))
#define CPUCLK_TO_VIACLK(X) (((floppy_ram[0x18] & 96) == 32) ? ((X) / (XTIMER)(LISA2ECLKFACTOR)) : ((X) / (XTIMER)(LISA1ECLKFACTOR)))

// How many microseconds should lisaem sleep between emulation cycles.  This allows us to slow down the emulator
// so that we can either get an acurate 5MHz clock, or be nice to the host OS.  This is implemented using poll with
// a null handle as per poll's man page.  The value of microsleep_tix is global so that it may be adjusted elsewhere
// in the code on the fly, as needed, perhaps it can be raised during I/O and idle mouse times.
#ifdef DEBUG
GLOBAL(int, microsleep_tix, 0);
#else
GLOBAL(int, microsleep_tix, 0);
#endif

// Scotty: Captain, we din' can reference it!
// Kirk:   Analysis, Mr. Spock?
// Spock:  Captain, it doesn't appear in the symbol table.
// Kirk:   Then it's of external origin?
// Spock:  Affirmative.
// Kirk:   Mr. Sulu, go to pass two.
// Sulu:   Aye aye, sir, going to pass two.

/*
 * But wait, there is mad to my method and reason!  Read on gentle hacker.  Here's the explanation.
 *
 * For sooth, thou shalt learneth...
 *
 * You see, C depends on variable and prototype declarations to know whether they be externed, to know the proper number and
 * types of parameters and return type of said functions.  If it does not know the proper types, it will warn, however, if the
 * prototype for a given function is incorrect, you can expect a maladjusted stack, and bad things will certainly happen.
 *
 * Having header files solves this - except it introduces another snag.  Should your header file delcare parameters differently
 * (aka incorrectly) than your .c file, you can expect warnings neither from gcc, nor from ld, which cannot know that this has
 * occured.
 *
 * One way to test for this is to include your header file into your C file, *BUT* this will not work from extern'ed functions.
 * gcc will complain about "conflicting types for...." and "previous declaration..." indicating a bug.  But you cannot use this
 * to self check your headers... so what to do? what to do?
 *
 * Another way is to use a program that automatically builds header files, but, meh - why would you want to have all your fn's
 * in a header file?
 *
 * Well, if you like Macro's, (and what good C programmer doesn't?) then the answer is clear.  Define a Macro to resolve as
 * extern when it should, and null when it shouldn't.  Easy as pie.
 *
 * I ran across this issue when I added several parameters to a function that I forgot was used externally in other code.
 * The program compiled just fine, and crashed beautifully.  gdb was able to solve the issue, but I thought I'd kill it for
 * good.  Always, always, always, program defensively.
 *
 *
 *
 */

#ifdef EXTERNX
#undef EXTERNX
#endif
#ifndef IN_FLIFLO_QUEUE_C
#define EXTERNX extern
#else
#define EXTERNX ;
#endif
EXTERNX int fliflo_buff_is_full(FLIFLO_QUEUE_t *b);
EXTERNX int fliflo_buff_has_data(FLIFLO_QUEUE_t *b);
EXTERNX int fliflo_buff_is_empty(FLIFLO_QUEUE_t *b);
EXTERNX uint32 fliflo_buff_size(FLIFLO_QUEUE_t *b);
EXTERNX uint32 fliflo_buff_percent_full(FLIFLO_QUEUE_t *b);
EXTERNX int fliflo_buff_add(FLIFLO_QUEUE_t *b, uint8 data);
EXTERNX uint8 fliflo_buff_pop(FLIFLO_QUEUE_t *b);
EXTERNX uint8 fliflo_buff_get(FLIFLO_QUEUE_t *b);
EXTERNX uint8 fliflo_buff_peek(FLIFLO_QUEUE_t *b);
EXTERNX uint8 fliflo_buff_peek_end(FLIFLO_QUEUE_t *b);
EXTERNX int fliflo_buff_create(FLIFLO_QUEUE_t *b, uint32 size);
EXTERNX void fliflo_buff_destroy(FLIFLO_QUEUE_t *b);

// extern void alertlog(char *alert);

// #endif

#define UI_LOG(level, fmt, args...)                                  \
  {                                                                  \
    fprintf(buglog, "%s:%s:%d: ", __FILE__, __FUNCTION__, __LINE__); \
    fprintf(buglog, fmt, ##args);                                    \
    fprintf(buglog, "\n");                                           \
    fflush(buglog);                                                  \
  }

DECLARE(char, _msg_alert[1024]);
DECLARE(char, _msg_alert2[1024]);

#ifdef DEBUG

extern void dumpmmu(uint8 c, FILE *out);
extern void dumpmmupage(uint8 c, uint8 i, FILE *out);

/*define check_iib() {my_check_iib(__FILE__,__FUNCTION__,__LINE__);}
   ifndef IN_CPU68K_C
   extern void my_check_iib(char *filename, char *function, long line);
   endif
*/
#define check_iib() \
  {                 \
    ;               \
  }

// do full iib sanity check
// #define DEBUG_LOG( level, fmt, args... ) { if ( level <= DEBUGLEVEL ) {fprintf(buglog,"%s:%s:%d: ",__FILE__,__FUNCTION__,__LINE__); fprintf(buglog,  fmt , ## args); fprintf(buglog,"\n"); fflush(buglog);if (do_iib_check) my_check_iib(__FILE__,__FUNCTION__,__LINE__);} }
// don't do iib sanity check - speed things up

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEBUG_LOG(level, fmt, args...)                                \
  {                                                                   \
    if ((level <= (DEBUGLEVEL)) && (debug_log_enabled && !!buglog))   \
    {                                                                 \
      fprintf(buglog, "%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__); \
      fprintf(buglog, fmt, ##args);                                   \
      fprintf(buglog, "| %x%x:%x%x:%x%x.%x %ld\n",                    \
              lisa_clock.hours_h, lisa_clock.hours_l,                 \
              lisa_clock.mins_h, lisa_clock.mins_l,                   \
              lisa_clock.secs_h, lisa_clock.secs_l,                   \
              lisa_clock.tenths, (long)cpu68k_clocks);                \
      fflush(buglog);                                                 \
      fflush(stdout);                                                 \
    }                                                                 \
  }

#define MEMDEBUG_LOG(level, fmt, args...)                                            \
  {                                                                                  \
    if ((level <= DEBUGLEVEL) && debug_log_enabled && !!buglog && abort_opcode != 2) \
    {                                                                                \
      fprintf(buglog, "%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__);                \
      fprintf(buglog, fmt, ##args);                                                  \
      fprintf(buglog, "| %x%x:%x%x:%x%x.%x %ld\n",                                   \
              lisa_clock.hours_h, lisa_clock.hours_l,                                \
              lisa_clock.mins_h, lisa_clock.mins_l,                                  \
              lisa_clock.secs_h, lisa_clock.secs_l,                                  \
              lisa_clock.tenths, (long)cpu68k_clocks);                               \
      fflush(buglog);                                                                \
      fflush(stdout);                                                                \
    }                                                                                \
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
     if (mmu_trans!=mmu_trans_all[context] || mmu!=mmu_all[context] || (start && context))
        { fprintf(buglog,"\n\n\nBUGCHK: context:%d start:%d seg1/2:%d/%d\n",context,start,segment1,segment2);
          fprintf(buglog,"BUGCHK: mmu_trans=%p [0]=%p [1]=%p [2]=%p [3]=%p [4]=%p\n", mmu_trans,mmu_trans_all[0],mmu_trans_all[1],mmu_trans_all[2],mmu_trans_all[3],mmu_trans_all[4]);
          fprintf(buglog,"BUGCHK: mmu=%p mmu_all[0]=%p [1]=%p [2]=%p [3]=%p [4]=%p\n\n\n",mmu,mmu_all[0],mmu_all[1],mmu_all[2],mmu_all[3],mmu_all[4]);
          _EXIT(1); }
     }

fprintf(buglog,"context:%d videoram @ %08x\n",context,videolatchaddress); fflush(buglog);
*/
#else
#define DEBUG_LOG(level, fmt, args...) \
  {                                    \
  }
#define check_iib() \
  {                 \
  }
#define MEMDEBUG_LOG(level, fmt, args...) \
  {                                       \
  }                                       \
  }
#endif

#ifndef __IN_LISAEM_WX__
extern void on_lisa_exit(void);
#endif

// this is needed because gdb doesn't tell you where your program quit from, just gives you the octal version of the exit
// parameter which is chopped to 9 bits for some oddball reason.
#define EXIT(x, cmd, fmt, args...)                                                                                        \
  {                                                                                                                       \
    char msg[1024], msg2[1024 - 100];                                                                                     \
    snprintf(msg2, 1024 - 100, fmt, ##args);                                                                              \
    snprintf(msg, 1024, "We've encountered a problem!\n%s\nStopped at %s:%s:%d with code :%d", msg2,                      \
             __FILE__, __FUNCTION__, __LINE__, x);                                                                        \
    if (!cmd)                                                                                                             \
      strncat(msg, "\nLisaEM will now quit.", 1023);                                                                      \
    fprintf((buglog ? buglog : stderr), "%s:%s:%d: exit with code :%d\n%s\n", __FILE__, __FUNCTION__, __LINE__, x, msg2); \
    messagebox(msg, "Emulation aborted!");                                                                                \
    fflush(buglog);                                                                                                       \
    if (!cmd)                                                                                                             \
      on_lisa_exit();                                                                                                     \
    return;                                                                                                               \
  }

#define EXITR(x, cmd, fmt, args...)                                                                                       \
  {                                                                                                                       \
    char msg[2048], msg2[2048 - 100];                                                                                     \
    fprintf(stderr, "exitr called from:%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);                                      \
    snprintf(msg2, 2048 - 100, fmt, ##args);                                                                              \
    snprintf(msg, 1024, "Sorry, the emulation aborted due to a fatal error\n%s\nStopped at %s:%s:%d with code :%d", msg2, \
             __FILE__, __FUNCTION__, __LINE__, x);                                                                        \
    if (!cmd)                                                                                                             \
      strncat(msg, "\nLisaEM will now quit.", 1023);                                                                      \
    fprintf((buglog ? buglog : stderr), "%s:%s:%d: exit with code :%d\n%s\n", __FILE__, __FUNCTION__, __LINE__, x, msg2); \
    messagebox(msg, "Emulation aborted!");                                                                                \
    fflush(buglog);                                                                                                       \
    if (!cmd)                                                                                                             \
      on_lisa_exit();                                                                                                     \
    return cmd - 1;                                                                                                       \
  }

#define EXITN(x, cmd, fmt, args...)                                                                                               \
  {                                                                                                                               \
    char msg[1024], msg2[1024 - 100];                                                                                             \
    snprintf(msg2, 1024 - 100, fmt, ##args);                                                                                      \
    snprintf(msg, 1024, "I'm sorry, the emulation has aborted due to a fatal error\n%s\nStopped at %s:%s:%d with code :%d", msg2, \
             __FILE__, __FUNCTION__, __LINE__, x);                                                                                \
    if (!cmd)                                                                                                                     \
      strncat(msg, "\nLisaEM will now quit.", 1023);                                                                              \
    fprintf((buglog ? buglog : stderr), "%s:%s:%d: exit with code :%d\n%s\n", __FILE__, __FUNCTION__, __LINE__, x, msg2);         \
    messagebox(msg, "Emulation aborted!");                                                                                        \
    fflush(buglog);                                                                                                               \
    if (!cmd)                                                                                                                     \
      on_lisa_exit();                                                                                                             \
    return NULL;                                                                                                                  \
  }

// 20191008 disable alert-log for production builds to save on code size + time as *printf is expensive and it will just go
// to /dev/null anyway in most cases.
#ifdef DEBUG
//////////////////////////////////////////////////////////////////////////////////////////////////
#define ALERT_LOG(level, fmt, args...)                                                    \
  {                                                                                       \
    if ((level <= DEBUGLEVEL))                                                            \
      fprintf((buglog ? buglog : stderr), "%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__); \
    fprintf((buglog ? buglog : stderr), fmt, ##args);                                     \
    fprintf((buglog ? buglog : stderr), "| %x%x:%x%x:%x%x.%x %ld\n",                      \
            lisa_clock.hours_h, lisa_clock.hours_l,                                       \
            lisa_clock.mins_h, lisa_clock.mins_l,                                         \
            lisa_clock.secs_h, lisa_clock.secs_l,                                         \
            lisa_clock.tenths, (long)cpu68k_clocks);                                      \
  }
//////////////////////////////////////////////////////////////////////////////////////////////////
#else
#define ALERT_LOG(level, fmt, args...) \
  {                                    \
  }
#endif
///////// Memory access macros////////////////////////////////////////////////////////////////////

/******* Memory/MMU related defines, protos and vars *******/

// Context Selectors
#define CXASEL (1 + (segment1 | segment2))
#define CXSEL (1 + ((segment1 | segment2) & (lastsflag ? 0 : 3)))
#define CXSASEL ((1 + ((segment1 | segment2) & (lastsflag ? 0 : 3))) & (start ? 0 : 7))

// mmu cache coherency
#define SET_MMU_DIRTY(x)             \
  {                                  \
    mmudirty = (x);                  \
    mmudirty_all[CXASEL] = mmudirty; \
  }
#define GET_MMU_DIRTY(x)             \
  {                                  \
    mmudirty = mmudirty_all[CXASEL]; \
  }
#define GET_MMUS_DIRTY(x)             \
  {                                   \
    mmudirty = mmudirty_all[CXSASEL]; \
  }

// are we trying to access a changed MMU block?  If so rebuild it!
#define CHECK_DIRTY_MMU(addr)                                  \
  {                                                            \
    if (context && (mmu[((addr) & 0x00fe0000) >> 17].changed)) \
    {                                                          \
      SET_MMU_DIRTY(0xdec0de);                                 \
      mmuflush(0);                                             \
    }                                                          \
  }

// filter out segment # - i.e. keep page+offset
#define MMUXXFILT 0x0001ffff

// #define SEGCHOP 1  /// bad - causes POST to get stuck

#ifdef SEGCHOP
#define MMU_X_FIL 0x0001ffff
#else
#define MMU_X_FIL 0x00ffffff
#endif

#define MMUSEGFILT 0x00fe0000
#define MMUEPAGEFL 0x00fffe00
#define ADDRESSFILT 0x00ffffff

// #define TWOMEGMLIM 0x001fffff
GLOBAL(uint32, TWOMEGMLIM, 0x001fffff);

// memory function definitions.
#ifndef IN_REG68K_C
#define EXTERN extern
#else
#define EXTERN ;
#endif

// :TODO: is this still valid? 20191017?
#ifndef IN_REG68K_C /////////////////////////////////////////////////////////////////////////////////////////////////////////
// __CYGWIN__ wrapper added by Ray Arachelian for LisaEm to prevent crashes in reg68k_ext exec
#ifdef __CYGWIN__
EXTERN uint32 reg68k_pc;
EXTERN uint32 *reg68k_regs;
EXTERN t_sr reg68k_sr;
#else
// #if (!(defined(PROCESSOR_ARM) || defined(PROCESSOR_SPARC) || defined(PROCESSOR_INTEL) ))
#if (!(defined(PROCESSOR_ARM) || defined(PROCESSOR_SPARC))) // 20051125
EXTERN uint32 reg68k_pc;
EXTERN uint32 *reg68k_regs;
EXTERN t_sr reg68k_sr;
#endif
#endif

#else

#ifdef __CYGWIN__
extern uint32 reg68k_pc;
extern uint32 *reg68k_regs;
extern t_sr reg68k_sr;
#else
#if (!(defined(PROCESSOR_ARM) || defined(PROCESSOR_SPARC) || defined(PROCESSOR_INTEL)))
extern uint32 reg68k_pc;
extern uint32 *reg68k_regs;
extern t_sr reg68k_sr;
#endif
#endif

#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef EXTERN

DECLARE(viatype, via[10]);
GLOBAL(uint8, via_running, 0); // If any VIA has a runing timer/SHIFTREG, then this is set (using bitmap of vianumber)

extern uint32 pc24;
// Used for Exception processing in Address/BUS errors
extern uint16 InstructionRegister;
extern uint8 CPU_function_code;
extern uint8 CPU_READ_MODE;

extern void contrastchange(void);

extern uint8 get_next_timer_id(void);

extern void cops_reset(void);
extern void disable_vidram(void);
extern void enable_vidram(void);
extern uint8 via1_ira(uint8 regnum);
extern void via1_ora(uint8 data, uint8 regnum);
extern void print_pc_and_regs(char *text);

extern uint8 viaX_ira(viatype *V, uint8 regnum);
extern uint8 viaX_irb(viatype *V);
extern void viaX_ora(viatype *V, uint8 data, uint8 regnum);
extern void viaX_orb(viatype *V, uint8 data);

extern void reset_via(int i);

extern void set_crdy_line(void);
extern void clr_crdy_line(void);
extern int is_crdy_line(void);
extern void set_kb_data_ready(void);
extern int is_vector_available(int avno);
extern void lisa_external_nmi_vector(uint32 addr);
extern void reg68k_internal_autovector(int avno);
extern int get_address_mmu_rfn_type(uint32 addr);
extern void reg68k_sanity_check_bitorder(void);

extern char *mspace(lisa_mem_t fn);
extern int floppy_insert(char *Image, uint8 insert_in_upper_floppy_drive);
extern void floppy_eject_button_pressed(uint8 on_upper_floppy_drive) ;
extern void apple_1(void);
extern void apple_2(void);
extern void apple_3(void);
extern void apple_dot(void);
extern void apple_dot_down(void);
extern void apple_dot_up(void);
extern void send_nmi_key(void);
extern void presspowerswitch(void);

extern void shift_option_0(void);
extern void shift_option_4(void);
extern void shift_option_7(void);

extern void add_mouse_event(int16 x, int16 y, int8 button);

extern void mmuflush(uint16 opts);
extern lisa_mem_t rmmuslr2fn(uint16 slr, uint32 a9);
extern t_ipc_table *get_ipct(uint32 address);
extern void checkcontext(uint8 c, char *text);
extern void cpu68k_printipc(t_ipc *ipc);
#ifdef DEBUG
extern void dump_scc(void);
#endif
extern char *printslr(char *x, long size, uint16 slr);
extern lisa_mem_t rmmuslr2fn(uint16 slr, uint32 a9);
extern void get_slr_page_range(int cx, int seg, int16 *pagestart, int16 *pageend, lisa_mem_t *rfn, lisa_mem_t *wfn);
extern lisa_mem_t rmmuslr2fn(uint16 slr, uint32 a9);

extern void dumpram(char *reason);
extern void dumpvia(void);
extern void fdumpvia1(FILE *out);
extern void fdumpvia2(FILE *out);
extern void fliflo_dump(FILE *log, FLIFLO_QUEUE_t *b, char *s);

#ifdef DEBUG
extern char *slrname(uint16 slr);
#endif

#ifdef DEBUG
extern void validate_mmu_segments(char *from);
#endif

#ifdef EXTERNX
#undef EXTERNX
#endif
#ifndef IN_ROM_C
#define EXTERNX extern
#else
#define EXTERNX ;
#endif
EXTERNX int has_xl_screenmod(void);
EXTERNX int16 read_dtc_rom(char *filename, uint8 *ROM);
EXTERNX int16 read_split_rom(char *filename, uint8 *ROMMX);
EXTERNX int16 read_rom(char *filename, uint8 *ROMMX);
EXTERNX uint8 *decode_lisa_icon(uint8 *icon);
EXTERNX int read_parallel_card_rom(char *filename);

// not likely that these are needed, but could be useful elsewhere:
EXTERNX uint8 ishex(char c);
EXTERNX uint8 gethex(char c);
EXTERNX uint8 brol(uint8 outdata, uint8 loop);
EXTERNX uint16 wrol(uint16 outdata, uint8 loop);
EXTERNX uint32 lrol(uint32 outdata, uint8 loop);

#ifdef EXTERNX
#undef EXTERNX
#endif
#ifndef IN_MMU_C
#define EXTERNX extern
#else
#define EXTERNX ;
#endif
EXTERNX void init_lisa_mmu(void);
EXTERNX void checkcontext(uint8 c, char *text);

#ifdef EXTERNX
#undef EXTERNX
#endif
#ifndef IN_UI_LOG_C
#define EXTERNX extern
#else
#define EXTERNX ;
#endif
EXTERNX void ui_err(const char *text, ...);
EXTERNX void ui_error(const char *text, ...);
EXTERNX void ui_log(unsigned int loglevel, const char *text, ...);
EXTERNX void ui_log_verbose(const char *text, ...);
EXTERNX void ui_log_request(const char *text, ...);
EXTERNX void ui_log_critical(const char *text, ...);
EXTERNX void ui_log_debug3(const char *text, ...);
EXTERNX void ui_log_debug2(const char *text, ...);
EXTERNX void ui_log_debug1(const char *text, ...);
EXTERNX void ui_log_user(const char *text, ...);
EXTERNX void ui_log_normal(const char *text, ...);

#ifdef EXTERNX
#undef EXTERNX
#endif
#ifndef IN_VIA6522_C
#define EXTERNX extern
#else
#define EXTERNX ;
#endif
EXTERNX void all_vias_run(void);
EXTERNX void init_vias(void);
EXTERNX void get_next_timer_event(void);
EXTERNX void check_current_timer_irq(void);
EXTERNX uint8 next_timer_id(void);
EXTERNX void VIAProfileLoop(int vianum, ProFileType *P, int event);

#ifndef IN_IRQ_C
extern int8 IRQRingBufferAdd(uint8 irql, uint32 address);
extern uint8 IRQRingGet(void);
extern void init_IRQ(void);
#endif

#ifndef IN_COPS_C
extern void cops_keyboard_id(void);
extern void set_keyboard_id(int32);
extern void init_cops(void);
#endif

#ifndef IN_FLOPPY_C
extern void floppy_go6504(void);
#endif

#ifndef IN_PROFILE_C
extern void ProfileLoop(ProFileType *P, int event);
extern void ProfileReset(ProFileType *P);
extern void ProfileResetOff(ProFileType *P);

// commenting out widget code for now
extern void get_profile_spare_table(ProFileType *P);
// extern void get_structure_identity_table(ProFileType *P);

#endif

#define PROLOOP_EV_IRB 0 // event=0 <- Read from IRB
#define PROLOOP_EV_IRA 1 // event=1 <- Read from IRA
#define PROLOOP_EV_ORA 2 // event=2 <- write to ORA
#define PROLOOP_EV_ORB 3 // event=3 <- write to ORB
#define PROLOOP_EV_NUL 4 // event=4 <- null event - called occasionally by event handling to allow timeouts

/****** Video ***********/

GLOBAL(uint8, bitdepth, 0);

GLOBAL(uint8, softmemerror, 0);
GLOBAL(uint8, harderror, 0);
GLOBAL(uint8, videoirq, 0);
GLOBAL(uint8, bustimeout, 0);
GLOBAL(uint8, videobit, 0);
GLOBAL(uint8, serialnumshiftcount, 0);
GLOBAL(uint8, serialnumshift, 0);
DECLARE(uint8, serialnum[8]);
DECLARE(uint8, serialnum240[32]);

#define MAXSOUNDS 10

GLOBAL(int, SoundLastOne, 5);
DECLARE(char, *SoundBuffer[MAXSOUNDS]);
DECLARE(uint32, SoundBufSize[MAXSOUNDS]);

GLOBAL(int, z8530_last_irq_status_bits, 0);
DECLARE(char, *scc_a_port);
DECLARE(char, scc_a_OPTIONS[1024]);
DECLARE(char, *scc_b_port);
DECLARE(char, scc_b_OPTIONS[1024]);
DECLARE(int, scc_a_telnet_port);
DECLARE(int, scc_b_telnet_port);

GLOBAL(int, scc_a_IW, -1);
GLOBAL(int, scc_b_IW, -1);

GLOBAL(uint8, serial_a, SCC_NOTHING);
GLOBAL(uint8, serial_b, SCC_NOTHING);

GLOBAL(FILE, *scc_a_port_F, NULL);
GLOBAL(FILE, *scc_b_port_F, NULL);
DECLARE(int, xonenabled[2]);
DECLARE(int, baudoverride[2]);

// Used by memory diag tests
GLOBAL(uint8, *mem_parity_bits1, NULL);
GLOBAL(uint8, *mem_parity_bits2, NULL);
GLOBAL(uint32, last_bad_parity_adr, 0);

GLOBAL(int, scc_running, 0);

extern void sound_fork(void);
extern void sound_play(uint16 freq);
extern void sound_off(void);

// DECLARE(scc_func_t, scc_fn[2] );

//  5 sets of mmu registers, each of 128 segments.
//  (4 are real lisa mmu contexts, added an extra for easier START mode translation)
DECLARE(mmu_t, mmu_all[5][128]);

// *mmu is in current context. i.e. mmu = mmu_all[4] sets context[4] (START mode).  I do this so that I

// don't have to dereference the array every single memory access as that would add an unnecessary expense.
// i.e. mmu_all[SEGMENT1+SEGMENT2|SETUP][(address>>17)&0x7f] is very expensive, but
// mmu[(address>>17)&0x7f] is far less expensive.
GLOBAL(mmu_t, *mmu, NULL);

DECLARE(mmu_trans_t, mmu_trans_all[5][32768]);

// DECLARE(mmu_trans_t,mmu_trans_all[5][32768*256]);  // this is 640MB just for pointers, I don't think this is going to work
//  to fix the high byte pages, we'll have to go to 32768*256=4294967296 * 5. that's not feasable.
//  so ugly hacks it is. i.e. pc=0xa0xxxxxx.

/* sadly if I used enums for lisa_mem_t.readfn/.writefn gcc would allocate 4 bytes for each so the total
   or this would be 12 bytes x 32768 pages/context x 5 = 1.85MB of ram for this table.  Instead I used
   defines and uint8's for lisa_mem_t, so this should be 1.25M depending on how they're packed    */

GLOBAL(mmu_trans_t, *mmu_trans, mmu_trans_all[0]); // ptr to current context, so I don't have to do expensive double array deref.
// i.e. mmu_trans=mmu_trans_all[0] is context 0.

// Refs for Generator's mem68k.c
// These are initialized by init_lisa_mmu.

// temp vars to play with, don't include these in vars.c
DECLARE(mmu_t, m);        // temp variable to play with
DECLARE(mmu_trans_t, mt); // mmu translation - temporary var
DECLARE(mmu_trans_t, *lastvideo_mt);

// result of mmu logical->physical translation
GLOBAL(int32, physaddr, 0);

// Lisa I/O Space types and other types of accesses.

// These used to be enums, but enums in gcc are 32 bits long, and unless you want to pass
// the -fshort-enums which may break other things, I'd rather just set them up as #DEFINEs and
// be done with it.  Could have left them as enums, but that would bloat the mmu structures I use
// as mmu_trans_all would grow huge.  consts can't be used to set arrays, so #DEFINES they'll have
// to be.

// Note, these are the letters Ox not zero x, so they're perfectly legal symbol names
// It makes for easier reading of addresses when reading the source. :)

// Regular (non-special) I/O address space.  Subject to MMU mapping.

#define OxERROR 0  /* This should never be used - it indicates a bug in our code */
#define OxUnused 1 /* unused I/O space address (only used in I/O space map) */
#define Ox0000_slot1 2
#define Ox2000_slot1 3
#define Ox4000_slot2 4
#define Ox6000_slot2 5
#define Ox8000_slot3 6
#define Oxa000_slot3 7
#define Oxc000_flopmem 8
#define Oxd000_ff_space 9
#define Oxd200_sccz8530 10
#define Oxd800_par_via2 11
#define Oxdc00_cops_via1 12
#define Oxe000_latches 13
#define Oxe800_videlatch 14
#define Oxf000_memerror 15
#define Oxf800_statreg 16

// Real Lisa memory
#define ram 17      /* Plain old RAM, or stack access.                                   */
#define vidram 18   /* same as ram, but flag on write that screen needs refreshing       */
#define ro_violn 19 /* Read only violation - what trap should I call? See schematic      */
#define bad_page 20 /* Bad page or unallocated segment - what trap here?                 */

// Special I/O space
#define sio_rom 21 /* access to ROM via sio mode                                        */
#define sio_mrg 22 /* mmu register being accessed.  Which depends on bit 3 of addr      */

#define sio_mmu 23 /* access ram or other spaces via the mmu (bit14=1 in address)       */

// Disparcher to I/O space (dispatcher to the Ox????_ fn's list above)
#define io 24 /* This is a dispatcher for I/O space when we don't know the address */

#define Oxd400_amd9512 25

#define OxVoid 26 /* Reserved mem fn's that do nothing, and return junk                */

// 27-31 unused.

#define MAX_LISA_MFN 27 /* The last Lisa Memory function type we have                        */

#define DEBUG_MFN_TRACE 32 // trap/trace fn's.

// I/O Address Maps.  These are used to initialize the fn pointers on the OUTPUT side of the mmu.
GLOBAL(char, *memspaces[],
       {"00-OxERROR",
        "01-OxUnused",
        "02-Ox0000_slot1",
        "03-Ox2000_slot1",
        "04-Ox4000_slot2",
        "05-Ox6000_slot2",
        "06-Ox8000_slot3",
        "07-Oxa000_slot3",
        "08-Oxc000_flopmem",
        "09-0xd000_ff_space",
        "10-Oxd200_sccz8530",
        "11-Oxd800_par_via2",
        "12-Oxdc00_cops_via1",
        "13-Oxe000_latches",
        "14-Oxe800_videlatch",
        "15-Oxf000_memerror",
        "16-Oxf800_statreg",
        "17-ram",
        "18-vidram",
        "19-ro_violn",
        "20-bad_page",
        "21-sio_rom",
        "22-sio_mrg",
        "23-sio_mmu",
        "24-io",
        "25-Oxd400_amd9512",
        "26-OxVoid"});

ACGLOBAL(lisa_mem_t, io_map[],
         {
             // +0             // +1              // +2              //+3
             /*                 +000              +200                +400              +600 */
             /* 0xfc0000 : */ Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, //   0
             /* 0xfc0800 : */ Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, //   4
             /* 0xfc1000 : */ Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, //   8
             /* 0xfc1800 : */ Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, Ox0000_slot1, //  12
             /* 0xfc2000 : */ Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, //  16

             /* 0xfc2800 : */ Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, //  20
             /* 0xfc3000 : */ Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, //  24
             /* 0xfc3800 : */ Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, Ox2000_slot1, //  28
                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfc4000 : */ Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, //  32
             /* 0xfc4800 : */ Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, //  36

             /* 0xfc5000 : */ Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, //  40
             /* 0xfc5800 : */ Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, Ox4000_slot2, //  44
             /* 0xfc6000 : */ Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, //  48
             /* 0xfc6800 : */ Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, //  52
             /* 0xfc7000 : */ Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, //  56

             /* 0xfc7800 : */ Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, Ox6000_slot2, //  60
                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfc8000 : */ Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, //  74
             /* 0xfc8800 : */ Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, //  78
             /* 0xfc9000 : */ Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, //  82
             /* 0xfc9800 : */ Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, Ox8000_slot3, //  86

             /* 0xfca000 : */ Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, //  90
             /* 0xfca800 : */ Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, //  94
             /* 0xfcb000 : */ Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, //  98
             /* 0xfcb800 : */ Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, Oxa000_slot3, // 102
                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////

             /* 0xfcc000 : */ Oxc000_flopmem, Oxc000_flopmem, Oxc000_flopmem, Oxc000_flopmem, // 106

             /* 0xfcc800 : */ Oxc000_flopmem, Oxc000_flopmem, Oxc000_flopmem, Oxc000_flopmem, // 110

             ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfcd000 : */ Oxd000_ff_space, Oxd200_sccz8530, Oxd400_amd9512, Oxd400_amd9512,       // 114
                                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfcd800 : */ Oxd800_par_via2, Oxd800_par_via2, Oxdc00_cops_via1, Oxdc00_cops_via1,   // 118
                                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfce000 : */ Oxe000_latches, OxUnused, OxUnused, OxUnused,                           // 122
                                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfce800 : */ Oxe800_videlatch, Oxe800_videlatch, Oxe800_videlatch, Oxe800_videlatch, // 126
                                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfcf000 : */ Oxf000_memerror, Oxf000_memerror, Oxf000_memerror, Oxf000_memerror,     // 130
                                                                                                      ///////////////////////////////////////////////////////////////////////////////////////////////////////
             /* 0xfcf800 : */ Oxf800_statreg, Oxf800_statreg, Oxf800_statreg, Oxf800_statreg          // 134
             /*                 +000              +200                +400              +600 */
         });

ACGLOBAL(lisa_mem_t, sio_map[], // danger! this is for use on pre-MMU addresses - lop off the top 17 bits of the address
                                // as well as the low 9 bits ( address & 01fe00)
         {
             /*              000      200      400      600      800      a00      c00     e00 */
             /*000000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, // 8/line
             /*001000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
             /*002000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
             /*003000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
             /*004000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*005000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*006000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*007000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*008000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*009000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*00a000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*00b000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*00c000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*00d000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*00e000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*00f000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*010000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
             /*011000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
             /*012000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
             /*013000:*/ sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom, sio_rom,
             /*014000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*015000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*016000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*017000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*018000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*019000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*01a000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*01b000:*/ sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg, sio_mrg,
             /*01c000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*01d000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*01e000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu,
             /*01f000:*/ sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu, sio_mmu});

GLOBAL(int, dispmemready, 0);

// Status register in memory.c
//------------------------------------------------------------------------------
#define STATREG_SOFTMEM_ERR 1 /* lisa doesn't use this bit */
#define STATREG_HARDMEM_ERR 2 /* Parity error bit 1=not set, 0=set */
#define STATREG_VERTICALRTC 4
#define STATREG_BUSTIMEOUT 8
#define STATREG_VIDEOBIT 16
#define STATREG_CSYNC 32
#define STATREG_HORIZONTAL 32
#define STATREG_INVBIT 64     /* lisa doesn't use this bit normally */
#define STATREG_UNUSEDBIT 128 /* lisa doesn't use this bit */

#ifndef IN_CPU68K_C
extern void free_ipct(t_ipc_table *ipct);
#endif

extern int diss68k_gettext(t_ipc *ipc, char *text);

// memory function definitions.
#ifndef INMEMORYDOTC
#define EXTERN extern
#else
#define EXTERN ;
#endif

EXTERN uint8 *(*mem68k_memptr[MAX_LISA_MFN])(uint32 addr);
EXTERN uint8 (*mem68k_fetch_byte[MAX_LISA_MFN])(uint32 addr);
EXTERN uint16 (*mem68k_fetch_word[MAX_LISA_MFN])(uint32 addr);
EXTERN uint32 (*mem68k_fetch_long[MAX_LISA_MFN])(uint32 addr);
EXTERN void (*mem68k_store_byte[MAX_LISA_MFN])(uint32 addr, uint8 data);
EXTERN void (*mem68k_store_word[MAX_LISA_MFN])(uint32 addr, uint16 data);
EXTERN void (*mem68k_store_long[MAX_LISA_MFN])(uint32 addr, uint32 data);
#undef EXTERN

extern int get_exs2_pending_irq_empty(void);
extern int get_exs1_pending_irq_empty(void);
extern int get_exs0_pending_irq_empty(void);

extern int get_exs0_pending_irq_2xpar(void);
extern int get_exs1_pending_irq_2xpar(void);
extern int get_exs2_pending_irq_2xpar(void);

GLOBAL(int, (*get_exs0_pending_irq)(void), &get_exs0_pending_irq_empty);
GLOBAL(int, (*get_exs1_pending_irq)(void), &get_exs1_pending_irq_empty);
GLOBAL(int, (*get_exs2_pending_irq)(void), &get_exs2_pending_irq_empty);

extern void lisa_ram_safe_setbyte(uint8 context, uint32 address, uint8 data);
extern void lisa_ram_safe_setword(uint8 context, uint32 address, uint16 data);
extern void lisa_ram_safe_setlong(uint8 context, uint32 address, uint32 data);
extern uint8 lisa_ram_safe_getbyte(uint8 context, uint32 address);
extern uint16 lisa_ram_safe_getword(uint8 context, uint32 address);
extern uint32 lisa_ram_safe_getlong(uint8 context, uint32 address);

#ifndef INMEMORYDOTC
extern void *dmem68k_memptr(char *file, char *function, int line, uint32 a);
extern uint8 dmem68k_fetch_byte(char *file, char *function, int line, uint32 a);
extern uint16 dmem68k_fetch_word(char *file, char *function, int line, uint32 a);
extern uint32 dmem68k_fetch_long(char *file, char *function, int line, uint32 a);
extern void dmem68k_store_byte(char *file, char *function, int line, uint32 a, uint8 d);
extern void dmem68k_store_word(char *file, char *function, int line, uint32 a, uint16 d);
extern void dmem68k_store_long(char *file, char *function, int line, uint32 a, uint32 d);
#endif

#ifdef DEBUGMEMCALLS

#define fetchaddr(a) dmem68k_memptr((char *)__FILE__, (char *)__FUNCTION__, __LINE__, a)
#define fetchbyte(a) dmem68k_fetch_byte((char *)__FILE__, (char *)__FUNCTION__, __LINE__, (uint32)(a))
#define fetchword(a) dmem68k_fetch_word((char *)__FILE__, (char *)__FUNCTION__, __LINE__, (uint32)(a))
#define fetchlong(a) dmem68k_fetch_long((char *)__FILE__, (char *)__FUNCTION__, __LINE__, (uint32)(a))
#define storebyte(a, d) dmem68k_store_byte((char *)__FILE__, (char *)__FUNCTION__, __LINE__, (uint32)(a), (uint8)(d))
#define storeword(a, d) dmem68k_store_word((char *)__FILE__, (char *)__FUNCTION__, __LINE__, (uint32)(a), (uint16)(d))
#define storelong(a, d) dmem68k_store_long((char *)__FILE__, (char *)__FUNCTION__, __LINE__, (uint32)(a), (uint32)(d))
#else

#define fetchaddr(a) mem68k_memptr[(mmu_trans[((a) & MMUEPAGEFL) >> 9].readfn)](a)
#define fetchbyte(a) mem68k_fetch_byte[(mmu_trans[((a) & MMUEPAGEFL) >> 9].readfn)](a)
#define fetchword(a) mem68k_fetch_word[(mmu_trans[((a) & MMUEPAGEFL) >> 9].readfn)](a)
#define fetchlong(a) mem68k_fetch_long[(mmu_trans[((a) & MMUEPAGEFL) >> 9].readfn)](a)
#define storebyte(a, d) mem68k_store_byte[(mmu_trans[((a) & MMUEPAGEFL) >> 9].writefn)]((a), (uint8)(d))
#define storeword(a, d) mem68k_store_word[(mmu_trans[((a) & MMUEPAGEFL) >> 9].writefn)]((a), (uint16)(d))
#define storelong(a, d) mem68k_store_long[(mmu_trans[((a) & MMUEPAGEFL) >> 9].writefn)]((a), (uint32)(d))
#endif

// As these are defined in memory.c, we cannot define them here as externs.  like duh!
#ifndef INMEMORYDOTC
// fn protos
extern uint8 *lisa_mptr_OxERROR(uint32 addr);
extern uint8 lisa_rb_OxERROR(uint32 addr);
extern uint16 lisa_rw_OxERROR(uint32 addr);
extern uint32 lisa_rl_OxERROR(uint32 addr);
extern void lisa_wb_OxERROR(uint32 addr, uint8 data);
extern void lisa_ww_OxERROR(uint32 addr, uint16 data);
extern void lisa_wl_OxERROR(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_OxUnused(uint32 addr);
extern uint8 lisa_rb_OxUnused(uint32 addr);
extern uint16 lisa_rw_OxUnused(uint32 addr);
extern uint32 lisa_rl_OxUnused(uint32 addr);
extern void lisa_wb_OxUnused(uint32 addr, uint8 data);
extern void lisa_ww_OxUnused(uint32 addr, uint16 data);
extern void lisa_wl_OxUnused(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Ox0000_slot1(uint32 addr);
extern uint8 lisa_rb_Ox0000_slot1(uint32 addr);
extern uint16 lisa_rw_Ox0000_slot1(uint32 addr);
extern uint32 lisa_rl_Ox0000_slot1(uint32 addr);
extern void lisa_wb_Ox0000_slot1(uint32 addr, uint8 data);
extern void lisa_ww_Ox0000_slot1(uint32 addr, uint16 data);
extern void lisa_wl_Ox0000_slot1(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Ox2000_slot1(uint32 addr);
extern uint8 lisa_rb_Ox2000_slot1(uint32 addr);
extern uint16 lisa_rw_Ox2000_slot1(uint32 addr);
extern uint32 lisa_rl_Ox2000_slot1(uint32 addr);
extern void lisa_wb_Ox2000_slot1(uint32 addr, uint8 data);
extern void lisa_ww_Ox2000_slot1(uint32 addr, uint16 data);
extern void lisa_wl_Ox2000_slot1(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Ox4000_slot2(uint32 addr);
extern uint8 lisa_rb_Ox4000_slot2(uint32 addr);
extern uint16 lisa_rw_Ox4000_slot2(uint32 addr);
extern uint32 lisa_rl_Ox4000_slot2(uint32 addr);
extern void lisa_wb_Ox4000_slot2(uint32 addr, uint8 data);
extern void lisa_ww_Ox4000_slot2(uint32 addr, uint16 data);
extern void lisa_wl_Ox4000_slot2(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Ox6000_slot2(uint32 addr);
extern uint8 lisa_rb_Ox6000_slot2(uint32 addr);
extern uint16 lisa_rw_Ox6000_slot2(uint32 addr);
extern uint32 lisa_rl_Ox6000_slot2(uint32 addr);
extern void lisa_wb_Ox6000_slot2(uint32 addr, uint8 data);
extern void lisa_ww_Ox6000_slot2(uint32 addr, uint16 data);
extern void lisa_wl_Ox6000_slot2(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Ox8000_slot3(uint32 addr);
extern uint8 lisa_rb_Ox8000_slot3(uint32 addr);
extern uint16 lisa_rw_Ox8000_slot3(uint32 addr);
extern uint32 lisa_rl_Ox8000_slot3(uint32 addr);
extern void lisa_wb_Ox8000_slot3(uint32 addr, uint8 data);
extern void lisa_ww_Ox8000_slot3(uint32 addr, uint16 data);
extern void lisa_wl_Ox8000_slot3(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxa000_slot3(uint32 addr);
extern uint8 lisa_rb_Oxa000_slot3(uint32 addr);
extern uint16 lisa_rw_Oxa000_slot3(uint32 addr);
extern uint32 lisa_rl_Oxa000_slot3(uint32 addr);
extern void lisa_wb_Oxa000_slot3(uint32 addr, uint8 data);
extern void lisa_ww_Oxa000_slot3(uint32 addr, uint16 data);
extern void lisa_wl_Oxa000_slot3(uint32 addr, uint32 data);

extern uint8 lisa_rb_ext_2par_via(viatype *V, uint32 addr);
extern void lisa_wb_ext_2par_via(viatype *V, uint32 addr, uint8 xvalue);

extern uint8 *lisa_mptr_2x_parallel_l(uint32 addr);
extern uint8 lisa_rb_2x_parallel_l(uint32 addr);
extern uint16 lisa_rw_2x_parallel_l(uint32 addr);
extern uint32 lisa_rl_2x_parallel_l(uint32 addr);
extern void lisa_wb_2x_parallel_l(uint32 addr, uint8 data);
extern void lisa_ww_2x_parallel_l(uint32 addr, uint16 data);
extern void lisa_wl_2x_parallel_l(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_2x_parallel_h(uint32 addr);
extern uint8 lisa_rb_2x_parallel_h(uint32 addr);
extern uint16 lisa_rw_2x_parallel_h(uint32 addr);
extern uint32 lisa_rl_2x_parallel_h(uint32 addr);
extern void lisa_wb_2x_parallel_h(uint32 addr, uint8 data);
extern void lisa_ww_2x_parallel_h(uint32 addr, uint16 data);
extern void lisa_wl_2x_parallel_h(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxc000_flopmem(uint32 addr);
extern uint8 lisa_rb_Oxc000_flopmem(uint32 addr);
extern uint16 lisa_rw_Oxc000_flopmem(uint32 addr);
extern uint32 lisa_rl_Oxc000_flopmem(uint32 addr);
extern void lisa_wb_Oxc000_flopmem(uint32 addr, uint8 data);
extern void lisa_ww_Oxc000_flopmem(uint32 addr, uint16 data);
extern void lisa_wl_Oxc000_flopmem(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxd200_sccz8530(uint32 addr);
extern uint8 lisa_rb_Oxd200_sccz8530(uint32 addr);
extern uint16 lisa_rw_Oxd200_sccz8530(uint32 addr);
extern uint32 lisa_rl_Oxd200_sccz8530(uint32 addr);
extern void lisa_wb_Oxd200_sccz8530(uint32 addr, uint8 data);
extern void lisa_ww_Oxd200_sccz8530(uint32 addr, uint16 data);
extern void lisa_wl_Oxd200_sccz8530(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxd800_par_via2(uint32 addr);
extern uint8 lisa_rb_Oxd800_par_via2(uint32 addr);
extern uint16 lisa_rw_Oxd800_par_via2(uint32 addr);
extern uint32 lisa_rl_Oxd800_par_via2(uint32 addr);
extern void lisa_wb_Oxd800_par_via2(uint32 addr, uint8 data);
extern void lisa_ww_Oxd800_par_via2(uint32 addr, uint16 data);
extern void lisa_wl_Oxd800_par_via2(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxdc00_cops_via1(uint32 addr);
extern uint8 lisa_rb_Oxdc00_cops_via1(uint32 addr);
extern uint16 lisa_rw_Oxdc00_cops_via1(uint32 addr);
extern uint32 lisa_rl_Oxdc00_cops_via1(uint32 addr);
extern void lisa_wb_Oxdc00_cops_via1(uint32 addr, uint8 data);
extern void lisa_ww_Oxdc00_cops_via1(uint32 addr, uint16 data);
extern void lisa_wl_Oxdc00_cops_via1(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxe000_latches(uint32 addr);
extern uint8 lisa_rb_Oxe000_latches(uint32 addr);

extern uint16 lisa_rw_Oxe000_latches(uint32 addr);
extern uint32 lisa_rl_Oxe000_latches(uint32 addr);
extern void lisa_wb_Oxe000_latches(uint32 addr, uint8 data);
extern void lisa_ww_Oxe000_latches(uint32 addr, uint16 data);
extern void lisa_wl_Oxe000_latches(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxe800_videlatch(uint32 addr);
extern uint8 lisa_rb_Oxe800_videlatch(uint32 addr);
extern uint16 lisa_rw_Oxe800_videlatch(uint32 addr);
extern uint32 lisa_rl_Oxe800_videlatch(uint32 addr);
extern void lisa_wb_Oxe800_videlatch(uint32 addr, uint8 data);
extern void lisa_ww_Oxe800_videlatch(uint32 addr, uint16 data);
extern void lisa_wl_Oxe800_videlatch(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxf000_memerror(uint32 addr);
extern uint8 lisa_rb_Oxf000_memerror(uint32 addr);
extern uint16 lisa_rw_Oxf000_memerror(uint32 addr);
extern uint32 lisa_rl_Oxf000_memerror(uint32 addr);
extern void lisa_wb_Oxf000_memerror(uint32 addr, uint8 data);
extern void lisa_ww_Oxf000_memerror(uint32 addr, uint16 data);
extern void lisa_wl_Oxf000_memerror(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxf800_statreg(uint32 addr);
extern uint8 lisa_rb_Oxf800_statreg(uint32 addr);
extern uint16 lisa_rw_Oxf800_statreg(uint32 addr);
extern uint32 lisa_rl_Oxf800_statreg(uint32 addr);
extern void lisa_wb_Oxf800_statreg(uint32 addr, uint8 data);
extern void lisa_ww_Oxf800_statreg(uint32 addr, uint16 data);
extern void lisa_wl_Oxf800_statreg(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_ram(uint32 addr);
extern uint8 lisa_rb_ram(uint32 addr);
extern uint16 lisa_rw_ram(uint32 addr);
extern uint32 lisa_rl_ram(uint32 addr);
extern void lisa_wb_ram(uint32 addr, uint8 data);
extern void lisa_ww_ram(uint32 addr, uint16 data);
extern void lisa_wl_ram(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_ro_violn(uint32 addr);
extern uint8 lisa_rb_ro_violn(uint32 addr);
extern uint16 lisa_rw_ro_violn(uint32 addr);
extern uint32 lisa_rl_ro_violn(uint32 addr);
extern void lisa_wb_ro_violn(uint32 addr, uint8 data);
extern void lisa_ww_ro_violn(uint32 addr, uint16 data);
extern void lisa_wl_ro_violn(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_bad_page(uint32 addr);
extern uint8 lisa_rb_bad_page(uint32 addr);
extern uint16 lisa_rw_bad_page(uint32 addr);
extern uint32 lisa_rl_bad_page(uint32 addr);
extern void lisa_wb_bad_page(uint32 addr, uint8 data);
extern void lisa_ww_bad_page(uint32 addr, uint16 data);
extern void lisa_wl_bad_page(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_sio_rom(uint32 addr);
extern uint8 lisa_rb_sio_rom(uint32 addr);
extern uint16 lisa_rw_sio_rom(uint32 addr);
extern uint32 lisa_rl_sio_rom(uint32 addr);
extern void lisa_wb_sio_rom(uint32 addr, uint8 data);
extern void lisa_ww_sio_rom(uint32 addr, uint16 data);
extern void lisa_wl_sio_rom(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_sio_mrg(uint32 addr);
extern uint8 lisa_rb_sio_mrg(uint32 addr);
extern uint16 lisa_rw_sio_mrg(uint32 addr);
extern uint32 lisa_rl_sio_mrg(uint32 addr);
extern void lisa_wb_sio_mrg(uint32 addr, uint8 data);
extern void lisa_ww_sio_mrg(uint32 addr, uint16 data);
extern void lisa_wl_sio_mrg(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_sio_mmu(uint32 addr);
extern uint8 lisa_rb_sio_mmu(uint32 addr);
extern uint16 lisa_rw_sio_mmu(uint32 addr);
extern uint32 lisa_rl_sio_mmu(uint32 addr);
extern void lisa_wb_sio_mmu(uint32 addr, uint8 data);
extern void lisa_ww_sio_mmu(uint32 addr, uint16 data);
extern void lisa_wl_sio_mmu(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_vidram(uint32 addr);
extern uint8 lisa_rb_vidram(uint32 addr);
extern uint16 lisa_rw_vidram(uint32 addr);
extern uint32 lisa_rl_vidram(uint32 addr);
extern void lisa_wb_vidram(uint32 addr, uint8 data);
extern void lisa_ww_vidram(uint32 addr, uint16 data);
extern void lisa_wl_vidram(uint32 addr, uint32 data);

extern void lisa_wb_xlvidram_parity(uint32 addr, uint8 data);
extern void lisa_ww_xlvidram_parity(uint32 addr, uint16 data);
extern void lisa_wl_xlvidram_parity(uint32 addr, uint32 data);
extern void lisa_wb_xlvidram(uint32 addr, uint8 data);
extern void lisa_ww_xlvidram(uint32 addr, uint16 data);
extern void lisa_wl_xlvidram(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_io(uint32 addr);
extern uint8 lisa_rb_io(uint32 addr);
extern uint16 lisa_rw_io(uint32 addr);
extern uint32 lisa_rl_io(uint32 addr);
extern void lisa_wb_io(uint32 addr, uint8 data);
extern void lisa_ww_io(uint32 addr, uint16 data);
extern void lisa_wl_io(uint32 addr, uint32 data);

extern uint8 lisa_rb_ram_parity(uint32 addr);
extern uint16 lisa_rw_ram_parity(uint32 addr);
extern uint32 lisa_rl_ram_parity(uint32 addr);
extern void lisa_wb_ram_parity(uint32 addr, uint8 data);
extern void lisa_ww_ram_parity(uint32 addr, uint16 data);
extern void lisa_wl_ram_parity(uint32 addr, uint32 data);

extern uint8 lisa_rb_vidram_parity(uint32 addr);
extern uint16 lisa_rw_vidram_parity(uint32 addr);
extern uint32 lisa_rl_vidram_parity(uint32 addr);
extern void lisa_wb_vidram_parity(uint32 addr, uint8 data);
extern void lisa_ww_vidram_parity(uint32 addr, uint16 data);
extern void lisa_wl_vidram_parity(uint32 addr, uint32 data);

extern uint8 *lisa_mptr_Oxd000_ff_space(uint32 addr);
extern uint8 lisa_rb_Oxd000_ff_space(uint32 addr);
extern uint16 lisa_rw_Oxd000_ff_space(uint32 addr);
extern uint32 lisa_rl_Oxd000_ff_space(uint32 addr);
extern void lisa_wb_Oxd000_ff_space(uint32 addr, uint8 data);
extern void lisa_ww_Oxd000_ff_space(uint32 addr, uint16 data);
extern void lisa_wl_Oxd000_ff_space(uint32 addr, uint32 data);

// not implemented - for future AMD9512 FPU's
extern uint8 *lisa_mptr_Oxd400_amd9512(uint32 addr);
extern uint8 lisa_rb_Oxd400_amd9512(uint32 addr);
extern uint16 lisa_rw_Oxd400_amd9512(uint32 addr);
extern uint32 lisa_rl_Oxd400_amd9512(uint32 addr);
extern void lisa_wb_Oxd400_amd9512(uint32 addr, uint8 data);
extern void lisa_ww_Oxd400_amd9512(uint32 addr, uint16 data);
extern void lisa_wl_Oxd400_amd9512(uint32 addr, uint32 data);

// reserved memory access for mmu pages out of scope that should not cause mmu_exception
extern uint8 *lisa_mptr_OxVoid(uint32 addr);
extern uint8 lisa_rb_OxVoid(uint32 addr);
extern uint16 lisa_rw_OxVoid(uint32 addr);
extern uint32 lisa_rl_OxVoid(uint32 addr);
extern void lisa_wb_OxVoid(uint32 addr, uint8 data);
extern void lisa_ww_OxVoid(uint32 addr, uint16 data);
extern void lisa_wl_OxVoid(uint32 addr, uint32 data);

#endif

extern void keystroke_cops(unsigned char c);
extern void send_cops_keycode(int k);

extern void apple_1(void);
extern void apple_2(void);
extern void apple_3(void);
extern void apple_enternum(void);
extern void apple_S(void); // for lisatest
extern void apple_enter(void);
extern void apple_renter(void);

extern void vidfixromchk(uint8 *s);
extern void keystroke_cops(unsigned char c);
extern void init_IRQ(void);
extern void init_Profiles(void);

extern void profile_unmount(void);

extern void init_sounds(void);
extern void initialize_scc(int actual);
extern void init_pty_serial_port(int port, char *desired_pty_port_symlink_name);
extern void init_shell_serial_port(int port);
extern void init_telnet_serial_port(int port);

extern uint16 crc16(uint16 crc, uint8 data);
extern void seek_mouse_event(void);
extern void init_floppy(long iorom);

extern void lisa_addrerror(uint32 addr);
extern void lisa_busrerror(uint32 addr);
extern void lisa_mmu_exception(uint32 addr);
extern void lisa_nmi_vector(uint32 addr);

extern CPP2C void lisa_powered_off(void);
extern CPP2C void lisa_rebooted(void);
extern CPP2C void messagebox(char *s, char *t);
extern CPP2C int yesnomessagebox(char *s, char *t);
extern CPP2C void floppy_motor_sounds(int track);
extern CPP2C void eject_floppy_animation(void);
extern CPP2C void save_pram(void);
extern CPP2C int pickprofilesize(char *filename, int allowexisting);

// extern CPP2C char *getDocumentsDir(void);
// extern CPP2C char *getResourcesDir(void);
// extern CPP2C char *getExecutablePath(void);

extern CPP2C int ImageWriter_LisaEm_Init(int iwnum);
extern CPP2C void iw_formfeed(int iw);
extern CPP2C void ImageWriterLoop(int iw, uint8 c);
extern CPP2C void gen_clock_diff_sleep(long diff);

void set_loram_clk(void);

extern void lisa_diag2_on_mem(void);
extern void lisa_diag2_off_mem(void);

extern void lisa_buserror(uint32 addr);
extern void mc68k_reset(void);
extern uint8 lisa_rb_Oxd200_sccz8530(uint32 addr);
extern uint8 lisa_rb_Oxd800_par_via2(uint32 addr);
extern void lisa_wb_Oxd800_par_via2(uint32 addr, uint8 data);
extern uint8 lisa_rb_Oxdc00_cops_via1(uint32 addr);
extern void lisa_wb_Oxdc00_cops_via1(uint32 addr, uint8 data);
extern void fixromchk(void);
extern int checkromchksum(void);

#ifndef IN_ROMLESS_C
extern void romless_vfychksum(void);
extern void romless_proread(void);
extern void romless_twgread(void);
extern int romless_boot(int profileboot);
extern int romless_entry(void);
#endif
extern void my_dump_cops(FILE *buglog);
// extern char *dis_movem(char *out, uint16, opcode, uint16 msk);

extern void get_next_timer_event(void);
extern void FloppyIRQ_time_up(void);

extern void debug_on(char *reason);
extern void debug_off(void);
extern char *chk_mtmmu(uint32 a, uint8 write);
extern void print_via_profile_state(char *s, uint8 data, viatype *V);
extern int profile_mount(char *filename, ProFileType *P);
extern void reg68k_external_autovector(int avno);

extern CPP2C void LisaScreenRefresh(void);

extern uint8 is_lisa_mouse_on(void);
extern void cops_timer_alarm(void);
extern void flag_vert_retrace_irq(void);

extern int get_nmi_pending_irq(void);
extern int get_scc_pending_irq(void);
// extern int get_exs0_pending_irq(void);
// extern int get_exs1_pending_irq(void);
// extern int get_exs2_pending_irq(void);
// extern int get_cops_pending_irq(void);  made static inline and placed in reg68k.c for speedup
extern int get_irq1_pending_irq(void);
extern void reset_video_timing(void);

#ifdef DEBUG
extern char *get_rom_label(uint32 pc24);
extern char *getvector(int v);
extern void lisaos_trap5(void);
extern char *mac_aline_traps(uint16 opcode);
#endif

extern uint32 getreg(uint8 regnum);

extern void printlisatime(FILE *out);
extern void normalize_lisa_clock(void);
extern void normalize_lisa_set_clock(void);
extern void decisecond_clk_tick(void);
extern void ascii_screendump(void);
uint8 cmp_screen_hash(uint8 *hashtable1, uint8 *hashtable2);

#define GETSEG(a) (((a & MMUSEGFILT) >> 17) & 0x7f)
#define GETEPAGE(a) (((a & MMUEPAGEFL) >> 9) & 0x7fff)

/* these are a bit too conservative perhaps, but they will prevent address overflows.              0x00fe0000*/
//** DANGER ** REMOVE & TWOMEGLIM!!! ***
#define CHK_MMU_REGST(addr) ((((mmu[(addr & MMUSEGFILT) >> 17].sor << 9) + (addr & MMUXXFILT))))
// #define RAM_MMU_REGST(addr)       (lisaram+(((       mmu[(addr & MMUSEGFILT)>>17].sor<<9) +  (addr & MMUXXFILT))  & TWOMEGMLIM))
#define CHK_MMU_A_REGST(c, addr) ((((mmu_all[c][(addr & MMUSEGFILT) >> 17].sor << 9) + (addr & MMUXXFILT))))
#define RAM_MMU_A_REGST(c, addr) (lisaram + (((mmu_all[c][(addr & MMUSEGFILT) >> 17].sor << 9) + (addr & MMUXXFILT))))

#define VALIDATE_MMU(addr) ((((mmu[(addr & MMUSEGFILT) >> 17].sor << 9) + (addr & MMUXXFILT)) & TWOMEGMLIM))

// cheato
// #define XCHK_MMU_TRANS(addr)      (        (((       mmu[(addr & MMUSEGFILT)>>17].sor<<9) + (addr & MMUXXFILT))  & TWOMEGMLIM))
// #define XRAM_MMU_TRANS(addr)      (lisaram+(((       mmu[(addr & MMUSEGFILT)>>17].sor<<9) + (addr & MMUXXFILT))  & TWOMEGMLIM))
// #define XCHK_MMU_A_TRANS(c,addr)  (        (((mmu_all[c][(addr & MMUSEGFILT)>>17].sor<<9) + (addr & MMUXXFILT))  & TWOMEGMLIM))
// #define XRAM_MMU_A_TRANS(c,addr)  (lisaram+(((mmu_all[c][(addr & MMUSEGFILT)>>17].sor<<9) + (addr & MMUXXFILT))  & TWOMEGMLIM))

// half way cheato
// #define XXCHK_MMU_TRANS(addr)     ((        (((addr & 0xffffff)+mmu_trans[       (addr & MMUEPAGEFL)>>9].sor9) + (addr & MMUXXFILT)  & TWOMEGMLIM)))
// #define XXRAM_MMU_TRANS(addr)     ((lisaram+(((addr & 0xffffff)+mmu_trans[       (addr & MMUEPAGEFL)>>9].sor9) + (addr & MMUXXFILT)  & TWOMEGMLIM)))
// #define XXCHK_MMU_A_TRANS(c,addr) ((        (((addr & 0xffffff)+mmu_trans_all[c][(addr & MMUEPAGEFL)>>9].sor9) + (addr & MMUXXFILT)  & TWOMEGMLIM)))
// #define XXRAM_MMU_A_TRANS(c,addr) ((lisaram+(((addr & 0xffffff)+mmu_trans_all[c][(addr & MMUEPAGEFL)>>9].sor9) + (addr & MMUXXFILT)  & TWOMEGMLIM)))

// needs to be above fn def below
GLOBAL(uint32, maxlisaram, 2 * 1024 * 1024);
GLOBAL(uint32, minlisaram, 0);

#define RAM512K (512 * 1024)
#define RAM1024K (1024 * 1024)
#define RAM1536K (1536 * 1024)
#define RAM2048K (2048 * 1024)

// 2020.11.21
#define GET_MMU_EFFECTIVE_ADDRESS(seg17, sorg9) ((int32)(int64)(-((int32)(seg17)) + ((int32)(sorg9))))

#define RAW_MMU_TRANSLATE(addr) (addr & 0x0001ffff) + (mmu[(addr & 0x00fe0000) >> 17].sor << 9)
#define RAW_MMU_TRANSLATE_CX(cx, addr) (addr & 0x0001ffff) + (mmu_all[cx][(addr & 0x00fe0000) >> 17].sor << 9)

// 12345678
#define RAM_MMU_TRANS(addr) (lisaram + ((uint32)(((int32)(addr & ADDRESSFILT) + mmu_trans[(addr & MMUEPAGEFL) >> 9].address)) & 0x1fffff))
#define RAM_MMU_A_TRANS(c, addr) (lisaram + ((uint32)(((int32)(addr & ADDRESSFILT) + mmu_trans[(addr & MMUEPAGEFL) >> 9].address)) & 0x1fffff))
#define CHK_MMU_TRANS(addr) (((uint32)(((int32)(addr & ADDRESSFILT) + mmu_trans[(addr & MMUEPAGEFL) >> 9].address)) & 0x1fffff))
#define CHK_MMU_A_TRANS(c, addr) (((uint32)(((int32)(addr & ADDRESSFILT) + mmu_trans[(addr & MMUEPAGEFL) >> 9].address)) & 0x1fffff))

#define RFN_MMU_TRANS(addr) (mmu_trans[(addr & MMUEPAGEFL) >> 9].readfn)
#define WFN_MMU_TRANS(addr) (mmu_trans[(addr & MMUEPAGEFL) >> 9].writefn)
#define RFN_MMU_A_TRANS(c, addr) (mmu_trans_all[c][(addr & MMUEPAGEFL) >> 9].readfn)
#define WFN_MMU_A_TRANS(c, addr) (mmu_trans_all[c][(addr & MMUEPAGEFL) >> 9].writefn)

// Memory checking macros
// CHK- check only - no bus error
// RCHK-Read check - call bus error
// WCHK-Write check - call bus error
// QCHK_RAM_LIMITS - check and quit emulator

#ifdef EXTRA_DEBUG_MMU
#define ALERTOVERFLOW(s)                                                                                                           \
  {                                                                                                                                \
    ALERT_LOG(0, "physram %s: in addr:%08x max/min:%08x/%08x translated:%08x ea/-ea:%08x/%08x", (s), addr, maxlisaram, minlisaram, \
              physaddr, mmu_trans[(addr & MMUEPAGEFL) >> 9].address, -mmu_trans[(addr & MMUEPAGEFL) >> 9].address);                \
    ALERT_LOG(0, "sor:%04x slr:%04x", mmu[(addr & MMUEPAGEFL) >> 9].sor, mmu[(addr & MMUEPAGEFL) >> 9].slr);                       \
  }
#else
#define ALERTOVERFLOW(s) \
  {                      \
    ;                    \
  }
#endif

// can call CHK_RAM_LIMITS, then check for -1, else do lisaram[physaddr] for a pointer
// phys=   (uint32)( (int32)(addr    ) + ea );  //2020.11.22 9pm
// physaddr=( (uint32)((  (int32)(addr & ADDRESSFILT)+mmu_trans[   (addr & MMUEPAGEFL)>>9].address) )  & 0x1fffff);
#define CHK_RAM_LIMITS(addr)                \
  {                                         \
    physaddr = RAW_MMU_TRANSLATE(addr);     \
    if (physaddr < (signed)minlisaram)      \
    {                                       \
      ALERTOVERFLOW("underflow");           \
      physaddr = -2;                        \
    }                                       \
    else if (physaddr > (signed)maxlisaram) \
    {                                       \
      ALERTOVERFLOW("overflow");            \
      physaddr = -1;                        \
    }                                       \
  }

#define CHK_RAM_A_LIMITS(c, addr)                                                               \
  {                                                                                             \
    physaddr = ((((addr & ADDRESSFILT) + mmu_trans_all[c][(addr & MMUEPAGEFL) >> 9].address))); \
    if (physaddr < (signed)minlisaram)                                                          \
      physaddr = -2;                                                                            \
    else if (physaddr > (signed)maxlisaram)                                                     \
      physaddr = -1;                                                                            \
  }

// check, and quit if error
#define QCHK_RAM_LIMITS(addr)                                                        \
  {                                                                                  \
    physaddr = RAW_MMU_TRANSLATE(addr);                                              \
    if (physaddr < 0 || physaddr > (signed)maxlisaram)                               \
    {                                                                                \
      fprintf(buglog, "*** %s:%s:%d:: mem out of range! @ %d/%08x :: @mmu=%08x\n\n", \
              __FILE__, __FUNCTION__, __LINE__, context, addr, physaddr);            \
      EXIT(2);                                                                       \
    }                                                                                \
  }

// check and abort on read, or write
// if (TWOMEGMLIM==0x001fffff && (physaddr<minlisaram||((uint32)(physaddr))>maxlisaram))
//
// if ((physaddr<minlisaram||((uint32)(physaddr))>maxlisaram))

#define RCHK_RAM_LIMITS(addr)                                                        \
  {                                                                                  \
    physaddr = RAW_MMU_TRANSLATE(addr);                                              \
    if (physaddr < minlisaram)                                                       \
      return 0x75;                                                                   \
    else if (physaddr > (signed)maxlisaram)                                          \
      physaddr = -1;                                                                 \
    if ((((uint32)(physaddr)) >= maxlisaram))                                        \
    {                                                                                \
      fprintf(buglog, "*** %s:%s:%d:: mem out of range! @ %d/%08x :: @mmu=%08x\n\n", \
              __FILE__, __FUNCTION__, __LINE__, context, addr, physaddr);            \
      CPU_READ_MODE = 1;                                                             \
      lisa_mmu_exception(addr);                                                      \
      return 0x93;                                                                   \
    }                                                                                \
  } // lisa_mmu_exception(addr);

#define WCHK_RAM_LIMITS(addr)                                                        \
  {                                                                                  \
    physaddr = RAW_MMU_TRANSLATE(addr);                                              \
    if (physaddr < minlisaram)                                                       \
      return;                                                                        \
    else if (physaddr > (signed)maxlisaram)                                          \
      physaddr = -1;                                                                 \
    if ((((uint32)(physaddr)) >= maxlisaram))                                        \
    {                                                                                \
      fprintf(buglog, "*** %s:%s:%d:: mem out of range! @ %d/%08x :: @mmu=%08x\n\n", \
              __FILE__, __FUNCTION__, __LINE__, context, addr, physaddr);            \
      CPU_READ_MODE = 0;                                                             \
      lisa_mmu_exception(addr);                                                      \
      return;                                                                        \
    }                                                                                \
  } // lisa_mmu_exception(addr);

#define XRCHK_RAM_LIMITS(addr) \
  {                            \
  }
#define XWCHK_RAM_LIMITS(addr) \
  {                            \
  }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif

// memory.c RAM macros - good idea to enable this actually, perhaps should disable cross context free, not sure yet.
#ifdef FORCE_MEMWRITE_TO_INVALIDATE_IPC

#define INVALIDATE_IPC()                                                                                                   \
  {                                                                                                                        \
    uint32 iA9 = (addr & ADDRESSFILT) >> 9;                                                                                \
    int32 iAD = mmu_trans[iA9].address;                                                                                    \
    if (mmu_trans_all[context][iA9].address == iAD &&                                                                      \
        mmu_trans_all[context][iA9].table != NULL)                                                                         \
    {                                                                                                                      \
      DEBUG_LOG(200, "calling free for address:%d/%08x pc:%08x mmu_trans_all[%d][%d]", context, addr, pc24, context, iA9); \
      free_ipct(mmu_trans_all[context][iA9].table);                                                                        \
      mmu_trans_all[context][iA9].table = NULL;                                                                            \
    }                                                                                                                      \
  }
#else
#define INVALIDATE_IPC() \
  {                      \
  }
#endif

#define XXXINVALIDATE_IPC()                                                                                             \
  {                                                                                                                     \
    uint32 iA9 = (addr & ADDRESSFILT) >> 9;                                                                             \
    int32 iAD = mmu_trans[iA9].address;                                                                                 \
    int iCTX;                                                                                                           \
    for (iCTX = 0; iCTX < 5; iCTX++)                                                                                    \
    {                                                                                                                   \
      if (mmu_trans_all[iCTX][iA9].address == iAD &&                                                                    \
          mmu_trans_all[iCTX][iA9].table != NULL)                                                                       \
      {                                                                                                                 \
        ALERT_LOG(0, "calling free for address:%d/%08x pc:%08x mmu_trans_all[%d][%d]", context, addr, pc24, iCTX, iA9); \
        free_ipct(mmu_trans_all[iCTX][iA9].table);                                                                      \
      }                                                                                                                 \
    }                                                                                                                   \
  }

// {if (addr>0xffffff) {DEBUG_LOG(0, "Access above 24 bits: %08x", addr);}     addr &=0x00ffffff; }
#ifdef DEBUG
#define HIGH_BYTE_FILTER() \
  {                        \
    addr &= 0x00ffffff;    \
  }

#ifdef MMUVALIDATE
#define IS_MMU_VALID_HERE()                                                                                                                   \
  {                                                                                                                                           \
    if (VALIDATE_MMU(addr) != CHK_MMU_TRANS(addr))                                                                                            \
    {                                                                                                                                         \
      EXIT(1, 0, "MMU VALIDATION FAILURE addr:%08x validates to:%08x but translated to %08x", addr, VALIDATE_MMU(addr), CHK_MMU_TRANS(addr)); \
    }                                                                                                                                         \
  }
#else
#define IS_MMU_VALID_HERE() \
  {                         \
  }
#endif

#else
#define HIGH_BYTE_FILTER() \
  {                        \
    addr &= 0x00ffffff;    \
  }

#define IS_MMU_VALID_HERE() \
  {                         \
  }

#endif
