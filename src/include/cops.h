/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.6      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2007 Ray A. Arachelian                          *
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
*                             COPS Microcontroller includes                            *
*                                                                                      *
\**************************************************************************************/

#ifndef COPSHINCLUDED
#define COPSHINCLUDED 1
/* COPS VIA 6522 Ports */

#define CORB 0xFCdd81 /* ORB/IRB on COPS 6522           */
#define CIRB 0xFCdd81 /* ORB/IRB on COPS 6522           */
#define CORA 0xFCdd83 /* ORa/IRA on COPS 6522           */
#define CIRA 0xFCdd83 /* ORa/IRA on COPS 6522           */
#define CDDRB 0xFCDD85
#define CDDRA 0xFCDD87
#define CT1CL 0xFCdd89
#define CT1CH 0xFCdd8b
#define CT1CL2 0xFCdd8d
#define CT1CH2 0xFCdd8f
#define CT2CL 0xFCdd91
#define CT2CH 0xFCdd93
#define CVIASR 0xFCdd95
#define CVIAACR 0xFCdd97
#define CVIAPCR 0xFCDD99
#define CVIAIFR 0xFCdd9B
#define CVIAIER 0xFCdd9d
#define CORA1NH 0xFCDD9F
#define CIRA1NH 0xFCDD9F

/* COPS Commands: */

/* Written to COPS 6522 PA Port

7654  3210
0000  0000  Turn I/O port on
0000  0001  Turn I/O port off
0000  0010  Read Clock Data
0001  nnnn  Write nnnn to clock data
0010  spmm  Set Clock Modes: s=enable(1) disable(0) clock set mode.
            p=power on (1) or (0) off.
            mm=00 - Clock/Timer disable
               01 - Timer Disable
               10 - Timer Underflow interrupt
               11 - Timer Underflow Power On

0101 nnnn   - Set NMI Char high nibble to nnnn
0110 nnnn   - set NMI Char low nibble to nnnn
1xxxxxxxx   - NOP

in addition to the keyboard and mouse several other peripherals are interfaced via the keyboard 6522:
2 parallel port lines
3 volume control lines
speaker tone line
floppy disk interrupt.

Cops receives power from the backup supply.  This voltage is available at all times,
wheter the lisa is powered down or even unplugged...  cops always runs.

Keyboard:
d-direction 1=down, 0=up.  drrrnnnn:



The mouse button is returned as keycode of
d000 0110 (binary)
d=1 pressed, d=0 released.

If the mouse is plugged in, 1000 0111 is returned, if unplugged,
0000 0111 is returned.


Software takes care of shift+autorepeat.  any key can gen an nmi.
Special Reset Codes:

*/

#define COPS_RES_KBFAILURE 0xff /* Keyboard COPS failure detected  */
#define COPS_RES_IOFAILURE 0xfe /* I/O Board COPS failure detected */
#define COPS_RES_KBUNPLUGD 0xfd /* Keyboard unplugged              */
#define COPS_RES_CLOCKTIRQ 0xfc /* clock timer interrupt           */
#define COPS_RES_POWERKEY 0xfb  /* Soft Power Switch hit           */

#define COPS_CLOCK_0 0xe0 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_1 0xe1 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_2 0xe2 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_3 0xe3 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_4 0xe4 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_5 0xe5 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_6 0xe6 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_7 0xe7 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_8 0xe8 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_9 0xe9 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_a 0xea /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_b 0xeb /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_c 0xec /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_d 0xed /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_e 0xee /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_f 0xef /* COPS year  followed by 5 bytes */

/* dd dh hm ms st - ddd day, hh-hour, mm min ss=second t=tenths of second   */

/* Keyboard ID # produced whenever the keyboard cops is reset.  valid codes
   are  keytronix id,   apd  */

#define COPS_KEYID_FRENCH 0xAD2d
#define COPS_KEYID_GERMAN 0xAE2e
#define COPS_KEYID_UK 0xAF2f
#define COPS_KEYID_US 0xBF2f

/* mouse commands:
  format 0111ennn (binary)

  e=1 mouse interrupts enabled, e=0 disabled. nnn=time interval
  The time interval is the value of nnn*4miliseconds.  Whenever
  you move the mouse, it interrupts the processor with this period
  Mouse data are transfered to the CPU in 3 bytes.  The significance
  of the bytess is as follows:

   00 mouse data follows:
   dx change in x direction -127 to +127
   dy change in y direction -128 to +127

  Each time a value is accepted by the CPU, the bytes are reset.
  Should the CPU not respond immediatly, the data are updated to show
  a cumulative value.

  The mouse button is returned as keycode of
  d000 0110 (binary)
  d=1 pressed, d=0 released.

  If the mouse is plugged in, 1000 0111 is returned, if unplugged,
  0000 0111 is returned.



  The real time clock (RTC) is capable of resoultions to 1/10th of
  a second and needs to be reset only every 16 years.  An alarm can
  be programmed via the RTC to generate a interrupt and/or turn the
  Lisa on after a timeout of upto 0xFFFFF seconds or about 12 days.

  Clock commands are coded:

  p=power on or off, 0=shut down 1= trn on.  s=enables and disables
  clock set mode.  In clock set mode, only as many nibbles as are
  required are sent. Once the "s" bit is cleared, the peripheral
  assumes that the data was complete.  The clock and timer must be
  stopped while the clock is being set.  The clock can be left running
  while setting up the tiimer.

  aa aa ay dd dh hm ms st - aaaaa=timer delay value in seconds from
  0-fffff.  y=0 to 15. ddd=1-365, hh=0-23, mm=00-59, ss=0-59 t=1/10th
  of a second...  All nibbles are bcd except for timer and year which
  are binary. When reading the clock, the data are returned as a series
  of reset codes in the formL 80 Ey dd dh hm ms st.

Software On-Off

Software controls the on and off states in the Lisa.  When the power
switch on the lower-front cabinet is pressed durring operation a reset
code is presented to the CPU.  This allows software to finish operations
in progress and store work files before turning off the lisa.  Note that
pressing doesn't remove power, only unplugging the lisa does.

Processor Board Control:

The processor board has a number of control bits that are set and reset
by access to a particular address in I/O space.:
*/

/* Storage for the I/O address controls */
extern uint32 diag1, diag2, seg1, seg2, setup, softmem, vertical, hardmem, videolatch, memerror, statusregister;

#define DIAG1RESET 0xFCE000
#define DIAG1SET 0xFCe002
#define DIAG2RESET 0xFCe004
#define DIAG2SET 0xFCe006
#define SEG1RESET 0xFCe008
#define SEG1SET 0xFCe00a
#define SEG2RESET 0xFCe00c
#define SEG2SET 0xFCe00e
#define SETUPRESET 0xFCe010
#define SETUPSET 0xFCe012
#define SOFTMEMRESET 0xFCe014
#define SOFTMEMSET 0xFCe016
#define VERTICALRESET 0xFCe018
#define VERTICALSET 0xFCe01a
#define HARDMEMRESET 0xFCe01c
#define HARDMEMSET 0xFCe01e

#define VIDEOLATCH 0xFCE800
#define MEMERRLATCH 0xFCF000
#define STATUSREGISTER 0xFCF800

/*

Interrupt Handling:

Upon detection of an interupt, the CPU enters supervisor mode automatically, it then uses a table
in memory know as the exception vector table, to point to the location at which the routine to
handle the interrupt can be found.  The priority level of the source of the interrupt can be used
as an index into the table, or the interrupt level itself can be used directly as the exception
vector.

*/

#define IRQRESETINITIALSSP 0x000000, 0
#define IRQRESETINITIALPC 0x000004, 0
#define IRQBUSERROR 0x000008, 0
#define IRQADDRESSERROR 0x00000C, 0
#define IRQLLLEGALINSTRUCTION 0x000010, 0
#define IRQZERODIVIDE 0x000014, 0
#define IRQCHKINSTRUCTION 0x000018, 0
#define IRQTRAPV 0x00001C, 0
#define IRQPRIVILIDGEVIOLATION 0x000020, 0
#define IRQTRACE 0x000024, 0
#define IRQALINETRAP 0x000028, 0
#define IRQFLINETRAP 0x00002C, 0
#define IRQUNKNOWN 0x000030, 0x00005F
#define IRQSPURIOUS 0x000060, 0
#define IRQOTHERINTERNALIRQ 0x000064, 0
#define IRQKEYBOARDIRQ 0x000068, 0
#define IRQSLOT2AUTOVECTOR 0x00006C, 0
#define IRQSLOT1AUTOVECTOR 0x000070, 0
#define IRQSLOT0AUTOVECTOR 0x000074, 0
#define IRQRS232 0x000078, 0
#define IRQNMI 0x00007C, 0
#define IRQTRAPVECTORS 0x000080, 0x000BF
#define IRQUNASSIGNED 0x0000C0, 0x000Cf
#define IRQUSERINTERRUPT 0x000100, 0x003FF

NMI Interrupts can come from : Power Failure, Hard / Soft memory error, keyboard reset.

                                                                            Level 1 irq : hard disk interface /
                                                                            parallel port,
                               floppy,
                               video.

                                   Pwer Reset Vector isnt shown because it is in ROM which is not accessed except in special i /
                                   o space durring powerup processing.

                                   Error Processing :

    (page 2 - 28 hw manual 83)

                                   * /

#endif
