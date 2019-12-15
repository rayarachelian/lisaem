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
*                         Floppy 6504 Controller Emulator                              *
*                                   Defines                                            *
\**************************************************************************************/

#ifndef FLOPPYHINCLUDED
#define FLOPPYHINCLUDED 1
/* Floppy Controller Commands */

#define FLOP_CONTROLLER 0xFCC001
#define FLOP_CTRLR_RWTS 0x81 /* execute the RWTS routine     */
#define FLOP_CTRLR_SEEK 0x83 /* seek to side/track           */
#define FLOP_CTRLR_JSR  0x84 /* JSR to routine in C003-5     */
#define FLOP_CTRLR_CLIS 0x85 /* Clear interrupt status       */
#define FLOP_CTRLR_STIM 0x86 /* Set interrupt mask           */
#define FLOP_CTRLR_CLIM 0x87 /* Clear interrupt mask         */
#define FLOP_CTRLR_WAIT 0x88 /* Wait in ROM until cold start */
#define FLOP_CTRLR_LOOP 0x89 /* Loop in ROM                  */

#define FLOP_COMMAND 0xFCC003
#define FLOP_CMD_READ   0x00 /* Read                         */
#define FLOP_CMD_WRITE  0x01 /* Write                        */
#define FLOP_CMD_UCLAMP 0x02 /* Unclamp                      */
#define FLOP_CMD_FORMAT 0x03 /* Format                       */
#define FLOP_CMD_VERIFY 0x04 /* Verify                       */
#define FLOP_CMD_FMTTRK 0x05 /* Format Track                 */
#define FLOP_CMD_VFYTRK 0x06 /* Verify Track                 */
#define FLOP_CMD_READX  0x07 /* Read without checksum vfy    */
#define FLOP_CMD_WRITX  0x08 /* Write without checksum       */
#define FLOP_CMD_CLAMP  0x09 /* Clamp                        */

#define FLOP_SELECT 0xFCC005
#define FLOP_SEL_LOWER 0x00
#define FLOP_SEL_UPPER 0x80
#define FLOP_SEL_DRV2  0x00
#define FLOP_SEL_DRV1  0x80

#define FLOP_SIDE_SELECT 0xFCC007
#define FLOP_SIDE_1      0x00
#define FLOP_SIDE_2      0x01
#define FLOP_SIDE_UPPER  0x00
#define FLOP_SIDE_LOWER  0x01

#define FLOP_SECTOR_NUMBER 0xFCC009  /* 0-22 */
#define FLOP_TRACK_NUMBER  0xFCC00B  /* 0-44 */
#define FLOP_SPEED_BYTE    0xFCC00D
#define FLOP_FORMAT_CONFIRM 0xFCC00F /* Format confirm byte */
#define FLOP_ERROR_STATUS   0xFCC011
#define FLOP_DISK_ID_VALUE  0xFCC013

// Floppies generalte interrupts to the CPU whenever a disk is inserted,
// eject button is pressed, whenever an 81 command is completed.

#define FLOP_STATUS 0xFCC05F
#define FLOP_STAT_INVCMD   0x01  /* invalid command                                  */
#define FLOP_STAT_INVDRV   0x02  /* invalid drive                                    */
#define FLOP_STAT_INVSEC   0x03  /* invalid sector                                   */
#define FLOP_STAT_INVSID   0x04  /* invalid side                                     */
#define FLOP_STAT_INVTRK   0x05  /* invalid track                                    */
#define FLOP_STAT_INVCLM   0x06  /* invalid clear mask                               */
#define FLOP_STAT_NODISK   0x07  /* no disk                                          */
#define FLOP_STAT_DRVNOT   0x08  /* drive not enabled                                */
#define FLOP_STAT_IRQPND   0x09  /* Interrupts pending                               */
#define FLOP_STAT_INVFMT   0x0a  /* Invalid format configuration                     */
#define FLOP_STAT_BADROM   0x0b  /* ROM Selftest failure                             */
#define FLOP_STAT_BADIRQ   0x0c  /* Unexpected IRQ or NMI                            */
#define FLOP_STAT_WRPROT   0x14  /* Write protect error                              */
#define FLOP_STAT_BADVFY   0x15  /* Unable to verify                                 */
#define FLOP_STAT_NOCLMP   0x16  /* Unable to clamp disk                             */
#define FLOP_STAT_NOREAD   0x17  /* Unable to read                                   */
#define FLOP_STAT_NOWRIT   0x18  /* Unable to write                                  */
#define FLOP_STAT_NOWRITE  0x18  /* Unable to write                                  */
#define FLOP_STAT_NOUCLP   0x19  /* Unable to unclamp                                */
#define FLOP_STAT_NOCALB   0x1A  /* Unable to find calibration                       */
#define FLOP_STAT_NOSPED   0x1B  /* Unable to adjust speed                           */
#define FLOP_STAT_NWCALB   0x1C  /* Unable to write calibration                      */

#define FLOP_CPU_RAM           0xFCC181
#define FLOP_CPU_RAM_END       0xFCC1FF

#define FLOP_XFER_RAM          0xFCC501
#define FLOP_XFER_RAM_END      0xFCC7FF

#define FLOP_IRQ_SRC_DRV2   128      /* set if 4, 5, or 6 set.                        */
#define FLOP_IRQ_SRC_RWTS2   64      /* set if drive 2 RWTS complete for drive 2      */
#define FLOP_IRQ_SRC_BTN2    32      /* set if button on disk 2 pressed               */
#define FLOP_IRQ_SRC_DSKIN2  16      /* set if disk in place in drive 2               */
#define FLOP_IRQ_SRC_DRV1     8      /* set if bits 0,1 or 2 are set                  */
#define FLOP_IRQ_SRC_RWTS1    4      /* set if drive 1 RWTS complete for drive 1      */
#define FLOP_IRQ_SRC_BTN1     2      /* set if button on disk 1 pressed               */
#define FLOP_IRQ_SRC_DSKIN1   1      /* set if disk inserted in drive 1               */

/*

P2-11 1983 HW Book

Floppy Disk Control

The floppy disk controller is locate don the I/O board and is controlled by addressing
the portion of physical I/O space in the range 00C001-00C7FF.  This area contains
command and status data as described.

The floppy disk controller consists of a 6504 based microcomputer which has 4K of ROM
for its own exclusive use and 1K of buffer RAM that is shared with the 68000.  The
RAM is provided with power backup by a battery.  Paramters stored in the floppy disk
controller RAM are therefore not lost durring power down.

The low 16 words of the 6504 address space are treated as a command block.  The 1st
byte is used for communication between the 68k and 6504.   The others are used to pass
parameters for use in defining command data and status.

Controller commands are written to 00C001 and have the siginficance of fig 2-6.
(FLOP_CTRLR_...)

The main part of the 6504's code consists of a RWTS (Read Write Track Sector) routine.
This utilizes a command block in the 8 bytes of memory that should be configured by the
CPU in accordance with Fig 2-7 (FLOP_CTRLR... fcc003-fcc013)

The disk drivers generate an irq to the CPU whenever a disk is inserted or the eject
button is pressed.  An interrupt is also generated upon completion of an "81" command.
The status of the controller can be found by examining location FCc05f.

Note: interrupt flag must be 1st enabled or a bus error will occur.  the enable bit
must be high in order to be able to access the floppy RAM that is shared by the
disk controller and the processor board.  The interrupt source is identified by this
status byte, the bit interpretation coded according to fig 2-9.

The disk controller has an area of ram that can be used for CPU storage of params.
It's located between 00C181 and C1FF.

The memory area used for info transfer to and from the CPU and disk controller
is shared by the 68k and 6504 is located between C501 and C7FF.

*/
#endif
