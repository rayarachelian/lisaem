/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2019.09.29                   *
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
*                     Profile/Widget Hard Disk  Routines                               *
*                                                                                   e   *
\**************************************************************************************/


#define IN_PROFILE_C
#include <vars.h>



// what's the size of a Profile image file header (so we can skip the start when doing data...)
#define PROFILE_IMG_HEADER_SIZE 2048

#define PROFILE_WAIT_EXEC_CYCLE 3 // #of ProFile Loops to wait after got 55 before we return results to Lisa

#define PRO_STATUS_CLEAR           {P->DataBlock[0]=0;P->DataBlock[1]=0;P->DataBlock[2]=0;P->DataBlock[3]=0;}
#define PRO_STATUS_GOT55           {P->DataBlock[0]|=128;}   // signal that I got 0x55 response
#define PRO_STATUS_NO55            {P->DataBlock[0]&=127;}   // signal that I didn't
#define PRO_STATUS_BUFFER_OVERFLOW {P->DataBlock[0]|=64; }   // Lisa sent too much data
#define PRO_STATUS_FAILED          {P->DataBlock[0]|=1;  }   // operation failed
#define PRO_STATUS_WAS_RESET       {P->DataBlock[3]|=128; P->last_reset_cpuclk=cpu68k_clocks;}   // profile was reset
#define PRO_INVALID_BLOCK_NO       {P->DataBlock[3]|=64; }   // asked to access invalid block #




char ProFile_Structure_Identity_table[536]=
{

 /*

    * The first 13 bytes show the device name, e.g. "PROFILE     ". Other drives are called "PROFILE 10M  " or "Widget-10   ".
    * The next three bytes hold the device number, which is $000000 for a 5MB ProFile, $000010 for a 10MB ProFile, and $001000 for a Widget 10 drive.
    * The next two bytes indicate  the firmware revision, e.g. $0398 for  3.98.
    * The next three bytes hold the total number of blocks available on the device. This is $002600 for 5MB and $004C00 for 10MB. You see, up to 8GB are possible. IDE reached its first limit at 520MB!
    * The next two bytes indicate the number of bytes per block: $0214 means 532. 532 means 512 byted user data and 20 bytes tag. This is the same format Macintosh MFS volumes and 400k/800k disks use. Send a MFS HD20SC with ST225N mechanism a read capacity command - the response will be 532 bytes per sector!
    * The next byte contains the total number of spare blocks available on the device, which is $20.
    * This is followed by the number of spare blocks currently allocated. A good drive uses less than three spares. When all these 32 blocks are allocated, the host will ask the user to call a qualified Apple Service technician for reformatting.
    * The next byte contains the number of bad blocks currently allocated. A bad block turns into a spared block during the next power-on self test.


 */





 //  0   1   2   3   4   5   6   7   8   9   a   b  c <- 13 bytes
    'P','R','O','F','I','L','E',' ',' ',' ',' ',' ',0 ,            // device name
//  d  e  f
    0, 0, 0,   // 0,0,0x10-10MB ProFile                            // device #
//  0x10,0x11
    0x03, 0x98,                                                    // ProFile firmware revsion
//  0x12, 0x13, 0x14
    0x00, 0x26, 0x00,                                              // # of available blocks to user *play with this*
    /*0x15, 0x16 */
    0x02, 0x14,                                                    // bytes/sector  =532
    32,                                                            // number of spares (blocks)  byte 22
    0,                                  // byte 23
    0,                                  // byte 24
    0xFF,0xFF,0xFF,                     // byte 25,26,27
    0,0,0,0,                            // byte 28,29,30,31  (now we need 500 more bytes full of zeros)

    // 50 0 bytes * 10 lines = 500 bytes... for both spare and error tables. (virtual profiles are perfect!)
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 1
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 2
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 3
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 4
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 5
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 6
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 7
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 8
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 9
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0   // 10
};


// shamelessly stolen from http://john.ccac.rwth-aachen.de:8000/patrick/UsbWidExamples.htm
// Thank  you for all your amazing work Dr. Patrick Schäfer!
/*

However, this spare table has two bad blocks and LOS refuses to install on it.  Need to clear them!!!
*** Spare Table:
  Spare block 25 is a spare table at LBA 001955
  Spare block 24 is spare for block 001988
  Spare block 26 is spare for block 001A32
  Spare block 50 is a spare table at LBA 0032AA 
*/

char Widget_Spare_Table[]={ // shamelessly stolen from http://john.ccac.rwth-aachen.de:8000/patrick/data/sparetable.txt - thank you Dr. Patrick!

/*0000:*/ 0xF0,0x78,0x3C,0x1E,
/*0004:*/ 0x00,0x00,0x00,0x06,//          ; table has been updated six times 
/*0008:*/ 0x00,//          ; offset 0 sectors
/*0009:*/ 0x01,//          ; interleave set 1 used
/*000A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x19,0x80,0x80,0x80,0x80,0x80,0x32,0x80,0x80,0x80,   //06: chain for LBA 001800..001BFF starts at spare Nr 19
              //0C: chain for LBA 003000..0033FF starts at spare Nr 32
/*001A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
/*002A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
/*003A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
/*004A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
/*005A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
/*006A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
/*007A:*/ 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
/*008A:*/ 0x00,//        ; 0 spares used
/*008B:*/ 0x00,//        ; 0 bad blocks pending
/*008C:*/ 0x00,0x00,0x00,0x40,0x00,0x00,0x20,0x00,0x00,0x00,//     ; spare bit map (bit set for each spare in use)
/*0096:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00A6:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00B6:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00C6:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00D6:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00E6:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*00F6:*/ 0x00,0x00,0x00,0x00,0xF8,0x01,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//  *  spare Nr 19: 1st spare table LBA 001955  *
/*0106:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0116:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0126:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0136:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0146:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0156:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x02,0xAA,0x00,0x00,0x00,0x00,0x00,//  * spare Nr 32: 2nd spare table LBA 0032AA  *
/*0166:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0176:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0186:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*0196:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*01A6:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*01B6:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

/*01C6:*/ 0x00,0x0C,0x05,0x11,0x0A,0x03,0x0F,0x08,0x01,0x0D,0x06,0x12,0x0B,0x04,0x10,0x09,//   ; remapping table ** INTERLEAVE TABLE ** this smells of profile though!!!!
/*01D6:*/ 0x02,0x0E,0x07,
/*01D9:*/ 0x45,0x11,
/*01DB:*/ 0xF0,0x78,0x3C,0x1E,
/*01DF:*/ 0x00,0x00,0x00,0x00,0x00,0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//   ; ADC offset value for each group of 16 cylinders
/*01EF:*/ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*01FF:*/ 0x00,
/*0200:*/ 0xF0,0x78,0x3C,0x1E
};


char Widget_Structure_Identity_Block[]={ // This is the Structure Identity Block according to Widget_ERS, block ffffff

 /*

    * The first 13 bytes show the device name, e.g. "PROFILE     ". Other drives are called "PROFILE 10M  " or "Widget-10   ".
    * The next three bytes hold the device number, which is $000000 for a 5MB ProFile, $000010 for a 10MB ProFile, and $001000 for a Widget 10 drive.
    * The next two bytes indicate  the firmware revision, e.g. $0398 for  3.98.
    * The next three bytes hold the total number of blocks available on the device. This is $002600 for 5MB and $004C00 for 10MB. You see, up to 8GB are possible. IDE reached its first limit at 520MB!
    * The next two bytes indicate the number of bytes per block: $0214 means 532. 532 means 512 byted user data and 20 bytes tag. This is the same format Macintosh MFS volumes and 400k/800k disks use. Send a MFS HD20SC with ST225N mechanism a read capacity command - the response will be 532 bytes per sector!
    * The next byte contains the total number of spare blocks available on the device, which is $20.
    * This is followed by the number of spare blocks currently allocated. A good drive uses less than three spares. When all these 32 blocks are allocated, the host will ask the user to call a qualified Apple Service technician for reformatting.
    * The next byte contains the number of bad blocks currently allocated. A bad block turns into a spared block during the next power-on self test.

From Widget-ERS.pdf (2018) pdfpg88
    -1 block:
    13 bytes name: $00:$0c -- 'Widget-10      ' (10,20,40)
    device widget 2 bytes: 0001 (0d,0e):: 00 00
    widget size 2 bytes one nibble, byte @0x0f: 10mb: 0x00, 20mb: 0x10, 40mb:0x20
    firmware revision 0x10, 0x11 (0x1a45)
    capacity 3 bytes: 0x12,0x13,0x14: 10MB:4c00,20MB:9800,40MB:13000
    bytes/block: 0214 bytes 0x15,0x16
    cyl10: 0202, cyl20: 0202, cyl40: 0404
    heads:  $02 - 1 byte at 0x19
    number of sectors: 1 byte: 10mb: 13, 20mb: 26, 40mb: 26
    # of possible spare location: at 0x1b-0x1d: 00004c
    # of spared blocks: 3 bytes at 1e-20: range 0-0x4b
    # of bad blocks: 3 bytes at 0x21-0x23:  range 0-0x4b
 */

 //  0   1   2   3   4   5   6   7   8   9   a   b  c <- 13 bytes
    'W','i','d','g','e','t','-','1','0',' ',' ',' ',0 ,            // device name
//   d    e     f
    0x00, 0x01, 0x10,   // 10MB Widget                             // device #
//  0x10,0x11
    0x1a, 0x45,                                                    // ProFile firmware revsion
    ///*0010*/ 0x1a,0x45,0x00,0x4c,0x00,0x02,0x14,0x20,  0x00,0x00,0xff,0xff,0xff,000,0x00,0x00,  //|.E.L.........L..|

//  0x12, 0x13, 0x14
    0x00, 0x4c, 0x00,                                              // # of available blocks to user *play with this*
    /*0x15, 0x16 */
    0x02, 0x14,                                                    // bytes/sector  =532
    // 0x17 0x18 cylinders
    0x02,0x02,                                           
    // 0x19
    0x02,  // heads
    // 0x1a - number of sectors 10mb: 13, 20mb: 26, 40mb: 26
    0x13,
    //0x1b-1d - # of possible spare locations
    0x00,0x00,0x4c,
    // 0x1e-0x20 # of actually spared blocks
    0x00,0x00,0x00,
    // 0x21-0x23, # of actual bad blocks
    0x00,0x00,0x00,

    0,                                  // byte 23
    0,                                  // byte 24
    0xFF,0xFF,0xFF,                     // byte 25,26,27
    0,0,0,0,                            // byte 28,29,30,31  (now we need 500 more bytes full of zeros)

    // 50 0 bytes * 10 lines = 500 bytes... for both spare and error tables. (virtual profiles are perfect!)
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 1
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 2
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 3
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 4
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 5
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 6
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 7
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 8
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,  // 9
    0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0   // 10
  

};
// 

/*
 * do the actual reading and writing of the blocks off the profile hard drive image file
 *
 */

void ProfileReset(ProFileType *P);
void ProfileResetOff(ProFileType *P);

#ifdef DEBUG
extern void fdumpvia2(FILE *out);

// This is an appender log, flush+close so that incase there's a crash, no pending data gets lost.

void dump_profile_block(FILE *f, uint8 *RAM, long sectornumber, char *text)
{
   int i,j;
   char c;

   if (!debug_log_enabled || !f) return;

   fprintf(f      ,"%s %ld tags::",text,sectornumber);

   for (i=0; i<20; i++)   fprintf(f     ,"%02x ",RAM[i]);

   for (i=0; i<512; i+=16)
   {
        fprintf(f     ,"\n%s %ld %04x:: ",text,sectornumber,i);

        for (j=0; j<16; j++)  fprintf(     f,"%02x ",RAM[20+i+j]);

        fputc('|',f);

        for (j=0; j<16; j++)
        {     c=RAM[20+i+j];
              c &=0x7f;
              if (c< 32) c|=32;
              if (c>125) c='.';
              fputc(c,f);
        }
   }
   fputc('\n',f);
}

void dump_profile_block_to_log(uint8 *RAM, long sectornumber, char *text)
{
   FILE *f;
   int i,j;
   char c;

   return;

   f=fopen("./lisaem-output.profile","a");
   if (!f) return;

   fprintf(f      ,"%s %ld tags::",text,sectornumber);

   for (i=0; i<20; i++)   fprintf(f     ,"%02x ",RAM[i]);

   for (i=0; i<512; i+=16)
   {
        fprintf(f     ,"\n%s %ld %04x:: ",text,sectornumber,i);

        for (j=0; j<16; j++)  fprintf(     f,"%02x ",RAM[20+i+j]);

        fputc('|',f);

        for (j=0; j<16; j++)
        {     c=RAM[20+i+j];
              c &=0x7f;
              if (c< 32) c|=32;
              if (c>125) c='.';
              fputc(c,f);
        }
   }
   fputc('\n',f);
   fclose(f);


}

void append_profile_log(int level, char *s,...)
{
  FILE *f;
  //fprintf(buglog,s);



  if (level>DEBUGLEVEL) return;

  return;

  f=fopen("./lisaem-output.profile","a");

  if (f) {va_list params;

          printlisatime(f);
          fprintf(f,"pc24:%08x ",pc24);

          va_start(params, s);
          vfprintf(f,s, params);
          va_end(params);

          if (debug_log_enabled)
          {
             printlisatime(buglog);
             fprintf(buglog,"pc24:%08x ",pc24);
             va_start(params, s);
             vfprintf(buglog,s, params);
             va_end(params);
             fprintf(buglog,"\n");
          }
          fprintf(f,"\n");
          fflush(f);
          fclose(f);

         }
}

#else

 #define dump_profile_block(s...) {;}
 #define dump_profile_block_to_log(s...) {;}
 #define append_profile_log(s...)                    {;}

#endif

long interleave5(long sector)
{
  static const int offset[]={0,5,10,15,4,9,14,3,8,13,2,7,12,1,6,11,16,21,26,31,20,25,30,19,24,29,18,23,28,17,22,27};
  return offset[sector&31] + sector-(sector&31);
}

long deinterleave5(long sector)
{
  static const int offset[]={0,13,10,7,4,1,14,11,8,5,2,15,12,9,6,3,16,29,26,23,20,17,30,27,24,21,18,31,28,25,22,19};
  return offset[sector&31] + sector-(sector&31);
}


void get_structure_identity_table(ProFileType *P)
{
   // copy spare table template
    DEBUG_LOG(0,"Profile spare table");
    memset(&P->DataBlock[4],0,536);

    // icanhaz widgets?
    if (floppy_iorom ==0x88 && P->DC42.numblocks==19456 && P==via[2].ProFile)
    {
      memcpy(&P->DataBlock[4],Widget_Structure_Identity_Block,512);

      switch (P->DC42.numblocks)
      {
      
         case 19456: /* 10M */ P->DataBlock[4+0x07]='1';
                               P->DataBlock[4+0x0d]=0x00; P->DataBlock[4+0x0e]=0x01;                                // on real dump +0x0e=1!
                             //P->DataBlock[4+0x12]=0x00; P->DataBlock[4+0x13]=0x4c; P->DataBlock[4+0x14]=0x00;     // number of blocks
                               P->DataBlock[4+0x17]=0x02; P->DataBlock[4+0x18]=0x02;                                // cylinders
                               P->DataBlock[4+0x19]=0x13;                                                           //  number of sectors (per track maybe?)
                               break;

         // these are all hypothetical - don't know if LOS or anything will support them.
         case 40960: /* 20M */ P->DataBlock[4+0x07]='2';
                               P->DataBlock[4+0x0d]=0x00; P->DataBlock[4+0x0e]=0x10;
                               P->DataBlock[4+0x12]=0x00; P->DataBlock[4+0x13]=0x98; P->DataBlock[4+0x14]=0x00;
                               P->DataBlock[4+0x17]=0x02; P->DataBlock[4+0x18]=0x02;
                               P->DataBlock[4+0x1a]=0x26;
                               break;

         case 81920: /* 40M */ P->DataBlock[4+0x07]='4';
                               P->DataBlock[4+0x0d]=0x00; P->DataBlock[4+0x0e]=0x20;
                             //P->DataBlock[4+0x12]=0x01; P->DataBlock[4+0x13]=0x03; P->DataBlock[4+0x14]=0x00;
                               P->DataBlock[4+0x17]=0x04; P->DataBlock[4+0x18]=0x04;
                               P->DataBlock[4+0x1a]=0x26;
                               break;

         case 163840:/* 80M */ P->DataBlock[4+0x07]='8';
                               P->DataBlock[4+0x0d]=0x00; P->DataBlock[4+0x0e]=0x20;
                             //P->DataBlock[4+0x12]=0x01; P->DataBlock[4+0x13]=0x03; P->DataBlock[4+0x14]=0x00;
                               P->DataBlock[4+0x17]=0x04; P->DataBlock[4+0x18]=0x04;
                               P->DataBlock[4+0x1a]=0x26;
                               break;

       }

      P->DataBlock[4+0x12]=(( (P->DC42.numblocks ) >> 16) & 0x0000ff);  // msb
      P->DataBlock[4+0x13]=(( (P->DC42.numblocks ) >>  8) & 0x0000ff);  // middle
      P->DataBlock[4+0x14]=(( (P->DC42.numblocks )      ) & 0x0000ff);  // lsb
 
      P->indexread=0;                     // reset index pointers to status
      P->indexwrite=4;
      ALERT_LOG(0,"profile.c:widget.c:!!!!!! Using Widget_Structure_Identity_Block !!!!!!!!! pc24:%08x",pc24);
      return;
    }

    memcpy(&P->DataBlock[4],ProFile_Structure_Identity_table,512);


    P->DataBlock[0]=0;
    P->DataBlock[1]=0;
    P->DataBlock[2]=0;
    P->DataBlock[3]=0;


   // fill in profile size of this profile

// 0x12, 0x13, 0x14
    if (P->DC42.numblocks==0) P->DC42.numblocks=  (P->DC42.datasizetotal/P->DC42.sectorsize);

    char a,b,c;
    switch (P->DC42.numblocks)
    {
     case 19456:  a='1'; b='0'; c='M';    break;//10M
     case 32768:  a='1'; b='6'; c='M';    break;//16M
     case 40960:  a='2'; b='0'; c='M';    break;//20M
     case 65536:  a='3'; b='2'; c='M';    break;//32M
     case 81920:  a='4'; b='0'; c='M';    break;//40M
     case 131072: a='6'; b='4'; c='M';    break;//64M
   //case 9728:
     default:     a=' '; b=' '; c=' ';          // 5M
    }


    P->DataBlock[4+0x08]=a;
    P->DataBlock[4+0x09]=b;
    P->DataBlock[4+0x0a]=c;
    P->DataBlock[4+0x0b]=' ';

    if (P->DC42.numblocks>9728) P->DataBlock[4+0x0f]=0x10;

    //  PROFILE 10M " $000010 or "Widget-10 " $001000

    P->DataBlock[4+18]=(( (P->DC42.numblocks ) >> 16) & 0x0000ff);  // msb
    P->DataBlock[4+19]=(( (P->DC42.numblocks ) >>  8) & 0x0000ff);  // middle
    P->DataBlock[4+20]=(( (P->DC42.numblocks )      ) & 0x0000ff);  // lsb

    P->indexread=0;                     // reset index pointers to status
    P->indexwrite=4;

    return;
}


void get_spare_table(ProFileType *P)
{
   // copy spare table template
    DEBUG_LOG(0,"Profile spare table");
    memset(&P->DataBlock[4],0,536);
    memcpy(&P->DataBlock[4],Widget_Spare_Table,512+4);
    P->indexread=0;                     // reset index pointers to status
    P->indexwrite=4;
    P->DataBlock[0]=0;
    P->DataBlock[1]=0;
    P->DataBlock[2]=0;
    P->DataBlock[3]=0;
    return;
}



void do_profile_read(ProFileType *P, uint32 block)
{
    //uint16 i,j;
    uint8 *blk;
    //#ifdef DEBUG
    uint32 oblock=block;
    //#endif

    if (!P                 ) {ALERT_LOG(0,"ProfileType P is null!"); return;}    // no image is opened!
    if (!P->DC42.sectorsize) {ALERT_LOG(0,"Profile sector size is 0!"); return;}


//    if (block==0x00ffffff || block<30)
//      ALERT_LOG(0,"Slot 1 ID:%04x, Slot 2 ID:%04x, Slot 3 ID:%04x, via:%d blk:%d",
//             lisa_ram_safe_getword(1,0x298), lisa_ram_safe_getword(1,0x29a), lisa_ram_safe_getword(1,0x29c),P->vianum,block ) 


    // convert unbootable fake dual parallel card to have the proper ID.
    if (block==0x00ffffff && (pc24 & 0x00ff0000)==0x00fe0000 && !romless && dualparallelrom[0x30]==0xff && dualparallelrom[0x31]==0xff)
    {
        if (lisa_ram_safe_getbyte(1,0x299)==0x02) lisa_ram_safe_setbyte(1,0x298,0xe0); 
        if (lisa_ram_safe_getbyte(1,0x29b)==0x02) lisa_ram_safe_setbyte(1,0x29a,0xe0); 
        if (lisa_ram_safe_getbyte(1,0x29d)==0x02) lisa_ram_safe_setbyte(1,0x29c,0xe0); 
    }

// disable interleave for now.
//    if (block<0x00f00000)   block=deinterleave5(block); /// here here here may need to change for widget

    ALERT_LOG(0,"profile.c:widget.c: block:%x (%d) [not used: deinterleave5: 0x%x %d pc24:%08x",block,block,deinterleave5(block),deinterleave5(block),pc24);

    if ( block==0x00fffffe)                 // return ProfileRAM buffer contents // this should be full spare table for Widget instead.
    {
        P->indexread=0;                     // reset index pointers to status
        P->indexwrite=4;

        dump_profile_block(buglog, &(P->DataBlock[4]), block, "read");
        dump_profile_block_to_log( &(P->DataBlock[4]), block, "read");
        return;
    }

    if ( block==0x00ffffff)  {get_structure_identity_table(P); return;}

    if (P->DC42.numblocks==0 && P->DC42.sectorsize) P->DC42.numblocks=  (P->DC42.datasizetotal/P->DC42.sectorsize);
    if ( block>=P->DC42.numblocks)            // wrong block #
    {
        P->DataBlock[3] |=64;                // set status byte to block number is invalid
        P->indexread=0;                      // reset index pointers to status
        P->indexwrite=4;
        return;
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    errno=0;
    blk=dc42_read_sector_data(&(P->DC42),block);

    if (P->DC42.retval || blk==NULL)
       {ALERT_LOG(0,"profile.c:widget.c:Read sector from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}

    if (blk!=NULL) memcpy( &(P->DataBlock[4+P->DC42.tagsize]), blk, P->DC42.datasize); //4

    errno=0;
    blk=dc42_read_sector_tags(&(P->DC42),block);
    if (P->DC42.retval || blk==NULL)
       {ALERT_LOG(0,"profile.c:widget.c:Read tags from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}

    if (blk!=NULL) memcpy( &(P->DataBlock[4       ]), blk, P->DC42.tagsize);   //4+512
    if ( errno )
    {
        ALERT_LOG(0,"profile.c:widget.c:Error reading ProFile Image! errno: %d, State:%d,IndexWrite:%d,ProfileCommand:%d,BSY:%d,Data:%d,CMDL:%d,buffer:%2x:%2x:%2x:%2x:%2x:%2x\n",
            errno,
            P->indexwrite,
            P->StateMachineStep,P->Command,P->BSYLine,P->VIA_PA,P->CMDLine,
            P->DataBlock[4],P->DataBlock[5],P->DataBlock[6],P->DataBlock[7],
            P->DataBlock[8],P->DataBlock[9]);

    }


    #ifdef DEBUG
    dump_profile_block(buglog, &(P->DataBlock[4]), block, "read");
    #endif
    errno=0;


    P->indexread=0;                     // reset index pointers to status
    P->indexwrite=4;
}


void do_widget_read(ProFileType *P, uint32 block)
{
    //uint16 i,j;
    uint8 *blk;
    //#ifdef DEBUG
    uint32 oblock=block;
    //#endif

    if (!P                 ) {ALERT_LOG(0,"ProfileType P is null!"); return;}    // no image is opened!
    if (!P->DC42.sectorsize) {ALERT_LOG(0,"Profile sector size is 0!"); return;}


//    if (block==0x00ffffff || block<30)
//      ALERT_LOG(0,"Slot 1 ID:%04x, Slot 2 ID:%04x, Slot 3 ID:%04x, via:%d blk:%d",
//             lisa_ram_safe_getword(1,0x298), lisa_ram_safe_getword(1,0x29a), lisa_ram_safe_getword(1,0x29c),P->vianum,block ) 


    // convert unbootable fake dual parallel card to have the proper ID.
    if (block==0x00ffffff && (pc24 & 0x00ff0000)==0x00fe0000 && !romless && dualparallelrom[0x30]==0xff && dualparallelrom[0x31]==0xff)
    {
        if (lisa_ram_safe_getbyte(1,0x299)==0x02) lisa_ram_safe_setbyte(1,0x298,0xe0); 
        if (lisa_ram_safe_getbyte(1,0x29b)==0x02) lisa_ram_safe_setbyte(1,0x29a,0xe0); 
        if (lisa_ram_safe_getbyte(1,0x29d)==0x02) lisa_ram_safe_setbyte(1,0x29c,0xe0); 
    }

// disable interleave for now.
//    if (block<0x00f00000)   block=deinterleave5(block); /// here here here may need to change for widget

    ALERT_LOG(0,"profile.c:widget.c: block:%x (%d) [not used: deinterleave5: 0x%x %d pc24:%08x",block,block,deinterleave5(block),deinterleave5(block),pc24);

    if ( block==0x00fffffe)                 // return ProfileRAM buffer contents // this should be full spare table for Widget instead.
    {
        P->indexread=0;                     // reset index pointers to status
        P->indexwrite=4;

        dump_profile_block(buglog, &(P->DataBlock[4]), block, "read");
        dump_profile_block_to_log( &(P->DataBlock[4]), block, "read");
        return;
    }


    if ( block==0x00ffffff)  {get_structure_identity_table(P); return;} // return Profile spare table


    if (P->DC42.numblocks==0 && P->DC42.sectorsize) P->DC42.numblocks=  (P->DC42.datasizetotal/P->DC42.sectorsize);
    if ( block>=P->DC42.numblocks)            // wrong block #
    {
        P->DataBlock[3] |=64;                // set status byte to block number is invalid
        P->indexread=0;                      // reset index pointers to status
        P->indexwrite=4;
        return;
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    errno=0;
    blk=dc42_read_sector_data(&(P->DC42),block);

    if (P->DC42.retval || blk==NULL)
       {ALERT_LOG(0,"profile.c:widget.c:Read sector from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}

    if (blk!=NULL) memcpy( &(P->DataBlock[4+P->DC42.tagsize]), blk, P->DC42.datasize); //4

    errno=0;
    blk=dc42_read_sector_tags(&(P->DC42),block);
    if (P->DC42.retval || blk==NULL)
       {ALERT_LOG(0,"profile.c:widget.c:Read tags from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}

    if (blk!=NULL) memcpy( &(P->DataBlock[4       ]), blk, P->DC42.tagsize);   //4+512
    if ( errno )
    {
        ALERT_LOG(0,"profile.c:widget.c:Error reading ProFile Image! errno: %d, State:%d,IndexWrite:%d,ProfileCommand:%d,BSY:%d,Data:%d,CMDL:%d,buffer:%2x:%2x:%2x:%2x:%2x:%2x\n",
            errno,
            P->indexwrite,
            P->StateMachineStep,P->Command,P->BSYLine,P->VIA_PA,P->CMDLine,
            P->DataBlock[4],P->DataBlock[5],P->DataBlock[6],P->DataBlock[7],
            P->DataBlock[8],P->DataBlock[9]);

    }


    #ifdef DEBUG
    dump_profile_block(buglog, &(P->DataBlock[4]), block, "read");
    #endif
    errno=0;


    P->indexread=0;                     // reset index pointers to status
    P->indexwrite=4;
    P->DataBlock[0]=0x22;
}


void do_profile_write(ProFileType *P,uint32 block)
{
    uint16 i;
    #ifdef DEBUG
    uint8 *blk;
    uint32 oblock=block;
    #endif

    if (!P                 ) {ALERT_LOG(0,"Request for null profile blk=%d!",block); return;}    // no image is opened!
    if (!P->DC42.sectorsize) {ALERT_LOG(0,"Profile sector size is 0!");              return;}

    if (block<0x00f00000) block=deinterleave5(block);

    #ifdef DEBUG
    ALERT_LOG(0,"profile.c:widget.c:ProFile write request block #%ld 0x%08x deinterleaved:%ld 0x%08x   pc24:%08x\n",block,block,oblock,oblock,pc24);
    #endif

    if ( block==0x00fffffe)                 // write ProfileRAM buffer contents
    {
        P->indexread=0;                     // reset index pointers to status
        P->indexwrite=4;
        return;
    }

    if ( block==0x00ffffff)                 // pretend to write ProfileRAM buffer contents
    {
       // copy spare table template

        for ( i=0; i<532; i++) {P->DataBlock[4+i]=ProFile_Structure_Identity_table[i];}

       // fill in profile size of this profile

        P->DataBlock[18]=((P->DC42.numblocks >> 16) & 0x0000ff);  // msb
        P->DataBlock[19]=((P->DC42.numblocks >>  8) & 0x0000ff);  // middle
        P->DataBlock[20]=((P->DC42.numblocks      ) & 0x0000ff);  // lsb
        P->Command=-2;                      // is now idle
        P->indexread=0;                     // reset index pointers to status
        P->indexwrite=4;

        return;
    }

    if ( P->DC42.fd<2 && !P->DC42.fh) { P->Command=-1; return;  }  // if the handle isn't open, disable profile.



    if (P->DC42.numblocks==0) P->DC42.numblocks=  (P->DC42.datasizetotal/P->DC42.sectorsize);
    if ( block>=P->DC42.numblocks)            // wrong block #
    {
        P->DataBlock[3] |=64;                // set status byte to block number is invalid
        P->indexread=0;                      // reset index pointers to status
        P->indexwrite=4;
        return;
    }


    //2007.01.25
    //P->DC42.synconwrite=1;              // Ensure data is immediately written to the disk.

    errno=0;

    //fprintf(stderr,"ProFile write to %ld %d bytes\n",block,P->DC42.datasize);
    dc42_write_sector_data(&P->DC42,block,&(P->DataBlock[4+6+P->DC42.tagsize]));
    if (P->DC42.retval) {DEBUG_LOG(0,"Write sector from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}

    dc42_write_sector_tags(&P->DC42,block,&(P->DataBlock[4+6]));
    if (P->DC42.retval) {DEBUG_LOG(0,"Write tags from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}


    // 20061223 - gprof catches the calculate checksums call that this calls to be very expensive
    //dc42_sync_to_disk(&P->DC42);


    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if ( errno )
    {

        EXIT(0,0,"Error reading ProFile Image! errno: %d, State:%d,IndexWrite:%d,ProfileCommand:%d,BSY:%d,Data:%d,CMDL:%d,buffer:%2x:%2x:%2x:%2x:%2x:%2x\n",
            errno,
            P->indexwrite,
            P->StateMachineStep,P->Command,P->BSYLine,P->VIA_PA,P->CMDLine,
            P->DataBlock[4],P->DataBlock[5],P->DataBlock[6],P->DataBlock[7],
            P->DataBlock[8],P->DataBlock[9]);
    }

    P->Command=-2;                      // is now idle
    P->indexread=0;                     // reset index pointers to status
    P->indexwrite=4;
}



/******************************************************************************\
*  Reset the Profile PB7 on VIA  because we got a signal to do so or because   *
*  we just booted up.                                                          *
*                                                                              *
\******************************************************************************/

void init_Profiles(void)
{
    //ProfileReset(via[0].ProFile);         // this can be ignored since no profile lives here.
//
    ProfileReset(via[1].ProFile);

    ProfileReset(via[2].ProFile);
    ProfileReset(via[3].ProFile);
    ProfileReset(via[4].ProFile);
    ProfileReset(via[5].ProFile);
    ProfileReset(via[6].ProFile);


// ProFileType profiles[7]; -- no need these are attached to the VIA's, so it's via[x]->Profile. *Burp*
}





void profile_unmount(void)
{
int i;
 for ( i=2; i<9; i++)
 {
  if (via[i].ProFile)
     { dc42_close_image(&via[i].ProFile->DC42);}

 }

}

int profile_mount(char *filename, ProFileType *P)
{
 int i;

 // Open the profile - if it doesn't exist, create it.
 //9728+1 - 5mb profile + space for spare table at -1.


#ifndef __MSVCRT__
 i=dc42_open(&P->DC42,filename,"wb");
#else
 i=dc42_open(&P->DC42,filename,"wn");            //win32
#endif
 if (i)
         {  int sz;
            int blocks[]={9728,19456,32768,40960,65536,81920,131072};
            //               0     1     2     3     4     5     6
            //              5M   10M   16M   20M   32M   40M    64M

            if (floppy_iorom ==0x88 && P==via[2].ProFile)
            {
              sz=1; // 10MB Widget
            }
            else
            {
              DEBUG_LOG(0,"Did not find %s, asking user what size drive to create",filename);
              sz=pickprofilesize(filename);  if (sz<0 || sz>6) return -1;
            }
            i=dc42_create(filename,"-lisaem.sunder.net hd-",blocks[sz]*512,blocks[sz]*20);
            i=dc42_open(&P->DC42,filename,"wm");
            //if (i) {DEBUG_LOG(0,"Failed to open newly created ProFile drive!");  exit(1);}
         }

 if (i) return -1;
// 9690blks ~~5mb?               9728-9690= 38
//19448blks ~~10mb?            19448+38 = 19486      likely is 19486 blocks.

return 0;
}

void ProfileResetOff(ProFileType *P)
{
  P->last_reset_cpuclk=-1;
}

void ProfileReset(ProFileType *P)
{
    if (!P) return;

    P->DataBlock[0]=0;
    P->DataBlock[1]=0;
    PRO_STATUS_WAS_RESET;
    P->DataBlock[3]=0;

 // reset state machine

    P->Command=-2;                         // is now idle
    P->StateMachineStep=0;                 // no step, just idle
    P->CMDLine=0;
    P->BSYLine=0;                          // not busy
    P->DENLine=1;
    P->RRWLine=0;
    P->VIA_PA= 1;                        // must always be 1 when ProFile is ready.
    P->clock_e=0;
    P->last_cmd=1;
    //DEBUG_LOG(0,"PROFILE RESET - ACK  01   tag:via2_ora");
    //append_profile_log(0,"PROFILE RESET - ACK  01   tag:via2_ora");

    //if (cpu68k_clocks>1000 && !debug_log_enabled)  { debug_on("profile-reset"); debug_log_enabled=1; }


 // reset buffer pointers

    P->indexread = 0;                        // reset index pointers to data
    P->indexwrite = 4;

    P->blocktowrite = 0;

    P->last_a_accs = 0;
    P->last_reset_cpuclk = 0;

    memset(P->DataBlock, 0, 542);
}


/*
 *
 * this gets called by the VIA code.  Via code passes pointer to profile structure AND
 * sets the event.  if the control lines are set, event code is zero, if data (pa read)
 * then event=1, if pawrite event=2.
//      uint8   CMDLine;                // set by Lisa
//      uint8   BSYLine;                // set by ProFile
//      uint8   DENLine;                // set by Lisa (drive enabled)
//      uint8   RRWLine;                // set by Lisa (read or write)
//
//      uint8   VIA_PA;                 // data to from VIA PortA (copied to/from V->via[0])
 *
 *
 */




// Macros to make the code a lot more readable/maintainable

// these are why the profile loop was called
#define PROLOOP_EV_IRB 0                   // event=0 <- Read from IRB
#define PROLOOP_EV_IRA 1                   // event=1 <- Read from IRA
#define PROLOOP_EV_ORA 2                   // event=2 <- write to ORA
#define PROLOOP_EV_ORB 3                   // event=3 <- write to ORB
#define PROLOOP_EV_NUL 4                   // event=4 <- null event - called occasionally by event handling to allow timeouts

#define EVENT_READ_IRB  (event==0)
#define EVENT_READ_IRA  (event==1)
#define EVENT_WRITE_ORA (event==2)
#define EVENT_WRITE_ORB (event==3)
#define EVENT_WRITE_NUL (event==4)


char *profile_event_names[5]=
     {
        "IRB",
        "IRA",
        "ORA",
        "ORB",
        "NUL"
     };


// implement a timeout that resets the state machine back to state 0, and various delays.

                                        // alarm_len_e gets set to x 1st to avoid using (x) twice - avoids macro side effects
#define SET_PROFILE_LOOP_TIMEOUT(x)     {P->alarm_len_e=(x); P->clock_e=cpu68k_clocks + P->alarm_len_e;}

// same as above, but disable pre-entry delay
#define SET_PROFILE_LOOP_NO_PREDELAY(x) {P->alarm_len_e=0;   P->clock_e=cpu68k_clocks + (x);           }

#define CHECK_PROFILE_LOOP_TIMEOUT      {if (P->clock_e<=cpu68k_clocks)                   \
                                            {                                             \
                                              P->StateMachineStep=IDLE_STATE;             \
                                             SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND); \
                                             P->last_cmd=1;                               \
                                             return;}                                     \
                                        }

//#define WAIT_FOR_PROFILE_LOOP_TIMEOUT   {if (P->clock_e >cpu68k_clocks) return;         }   // delete me not used


// have we gone past x clk cycles since the last time the timeout was set?  useful for faking delays.
#define TIMEPASSED_PROFILE_LOOP(x) ((P->clock_e - P->alarm_len_e+(x)) < cpu68k_clocks)

#define TIMEPASSED_LEFT(x) ((P->clock_e - P->alarm_len_e+(x)) )

// this disables the above, allowing us to skip the initial delay before a state becomes active.
#define DISABLE_TIMEPASSED_DELAY   {P->alarm_len_e=0;}


// initial handshaking steps - common to both read/write states

#define IDLE_STATE                  0    // wait for Lisa to flip CMD, then send 01 and flip BSY
#define WAIT_1st_0x55_STATE         2    // wait for Lisa to answer 01 with 0x55

//define DELAY_BEFORE_CMD_STATE      3    // delay entry into next state <- get rid of this
#define GET_CMDBLK_STATE            4    // read the 6 byte cmd block from the Lisa, send cmd response byte

#define WAIT_2nd_0x55_STATE         5    // wait for 2nd 0x55 response to cmd response byte
#define PARSE_CMD_STATE             6    // parse the command and switch to read or write block

// states for block writes
#define ACCEPT_DATA_FOR_WRITE_STATE 7    // for write block, accept the data+tag bytes
#define WAIT_3rd_0x55_STATE         8    // wait for 3rd 0x55 confirmation before writing block
#define WRITE_BLOCK_STATE           9    // waste some time simulating a busy profile, and actually write the block
#define SEND_STATUS_BYTES_STATE    12    // Let Lisa read the status bytes after the write is done, then return to idle.

// states for block reads
#define SEND_DATA_AND_TAGS_STATE   10    // send the status+data+tags back to the Lisa
#define FINAL_FLIP_TO_IDLE_STATE   11    // short delay and return back to idle.

#define WID_MULTI_READ_SEND_DATA_AND_TAGS_STATE 100 // Widget multi block read
#define WID_MULTI_READ_NEXT_BLK                 101 // Indicate ready for next block

#define WID_WRITE_BLOCK_STATE                   200
#define WID_MULTI_WRITE_NEXT_BLK                201
//#define WID_MULTI_WRITE_NEXT_BLK                202


void ProfileLoop(ProFileType *P, int event)
{
    uint32 blocknumber=0;

    if (  !(profile_power & (1<<(P->vianum-2)) )  ) return;
    if ( !P->DENLine && P->vianum!=2) {ALERT_LOG(0,"DEN is disabled on via#%d- ignoring ProFile commands",P->vianum); return;}                   // Drive Enabled is off (active low 0=enabled, 1=disable profile)
        
    #ifdef DEBUG
    if (debug_log_enabled && buglog!=NULL) fdumpvia2(buglog);
    #endif

 switch (P->StateMachineStep)
 {


    case IDLE_STATE:                           // 0 StateMachineStep is idle - wait for CMD to go low, then BUSY=0
         CHECK_PROFILE_LOOP_TIMEOUT;

         #ifdef DEBUG                          
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"In state 0 now - idle - CMDLine is:%d && P->last_cmd:%d if non zero, should see state transition.",P->CMDLine,P->last_cmd);
         #endif

         P->BSYLine=0;
         if (EVENT_WRITE_NUL) return;

                             // not active.  If Lisa lowers CMD, we lower BSYLine and goto state 2
         if (P->CMDLine && P->last_cmd)          {
                                                P->StateMachineStep=WAIT_1st_0x55_STATE;
                                                SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);

                                                P->BSYLine=1;           // flip BSY
                                                P->VIA_PA=0x01;         // ACK cmd on bus
                                                P->last_a_accs=0;

                                                DEBUG_LOG(0,"ACK CMD - sending 01 - State Transition to 3");
                                                return;
                                              }

         if (!P->CMDLine)  P->last_cmd=1;          // prevent a too early entry into state 3
         else             {
                           P->last_cmd=0;
                           SET_PROFILE_LOOP_TIMEOUT(HUN_THOUSANDTH_OF_A_SEC);
                           DEBUG_LOG(0,"cmd is still 0, lengthening timeout");

                          }


         return;


    case WAIT_1st_0x55_STATE:

         CHECK_PROFILE_LOOP_TIMEOUT;

         #define PROF_WAIT_TIME (MILLIONTH_OF_A_SECOND)

        if ( !TIMEPASSED_PROFILE_LOOP(PROF_WAIT_TIME)  && !EVENT_WRITE_ORA)
           {
            #ifdef DEBUG                         
            if (!(EVENT_WRITE_NUL))
            DEBUG_LOG(0,"In state 2 - <!BSY wasting while waiting for 55, got %02x, last_a_accs:%d before turning BSY to 0, EVENT_WRITE_ORA:%d CPU cycles left:%016llx",
               P->VIA_PA,
               P->last_a_accs,
               (EVENT_WRITE_ORA),
               (TIMEPASSED_LEFT(PROF_WAIT_TIME))
               );
            #endif

            P->BSYLine=0;
            return;
           }

         DEBUG_LOG(0,"In state 2 - <+BSY NOT waiting for 55, got %02x, last_a_accs:%d signalling BSY to 0: EVENT_WRITE_ORA:%d CPU cycles left:%016llx",
               P->VIA_PA,
               P->last_a_accs,
               (EVENT_WRITE_ORA),
               (TIMEPASSED_LEFT(PROF_WAIT_TIME))
               );

         P->BSYLine=1; DEBUG_LOG(0,"<+BSY")
         if (EVENT_WRITE_NUL) {DEBUG_LOG(0,"Time's up! Did not see write to ORA, screw you guys, going home!"); return;}

         #ifdef DEBUG
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"In state 2 - waiting for 55 got a write, Got PA=%02x from Lisa, last_a_accs:%d  - BSY is now %d",
                P->VIA_PA,
                P->last_a_accs,P->BSYLine);
         #endif

     //    if ( EVENT_WRITE_ORA)//P->last_a_accs)// now wait for 0x55 ACK from Lisa, else, go back to idle
                //{
                   //*** might need to flip BSY and check CA1 changing on BSY, I see IFR stuff around it? ***
                   if (P->VIA_PA!=0x55 && P->VIA_PA!=0x01)  // 0x69 is the official go back to idle command.  0xAA for profile maybe.
                                               {
                                                 DEBUG_LOG(0,"state 2: <!BSY VIA:%d did not get 55, got %02x, going back to idle now. last_a_accs=%d",
                                                              P->vianum,P->VIA_PA,P->last_a_accs);
                                                 P->StateMachineStep=IDLE_STATE;
                                                 P->BSYLine=0;  DEBUG_LOG(0,"<!BSY")//2018.04.19 - maybe all we need?
                                                 return;
                                               }

                    if (!(P->VIA_PA==0x55 || P->VIA_PA==0x69) && EVENT_WRITE_ORA) {DEBUG_LOG(0,"Did not get 0x55|0x69!!! Sending >01!!!"); 
                                                                                   P->VIA_PA=0x01;}
                    if ( (P->VIA_PA==0x55 || P->VIA_PA==0x69) && !P->CMDLine)
                                               {
                                                PRO_STATUS_GOT55;
                                                P->StateMachineStep=GET_CMDBLK_STATE; //DELAY_BEFORE_CMD_STATE; //GET_CMDBLK_STATE;
                                                SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);

                                                DEBUG_LOG(0,"State Transition to 4 - got 0x%02x",P->VIA_PA);
                                                P->indexread=4;         // start at offset 4
                                                P->indexwrite=4;        // (4 byte preable reserved for status for lisa to read later
                                                return;
                                               }
         return;


case GET_CMDBLK_STATE:           // 4          // now copy command bytes into command buffer
         #ifdef DEBUG                         
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"In state 4 - waiting for command");
         #endif
        

         CHECK_PROFILE_LOOP_TIMEOUT;

         // wait a bit before flopping busy, but if Lisa sends a byte, accept it
         if ( TIMEPASSED_PROFILE_LOOP(HUN_THOUSANDTH_OF_A_SEC)  && !(EVENT_WRITE_ORA)) //was 1/100th
                {
                    DEBUG_LOG(0,"State 4 - wasting 1/100,000th of a sec");
                    P->BSYLine=0; DEBUG_LOG(0,"<!BSY")
                    return;
                }

          P->BSYLine=1;
          if (EVENT_WRITE_NUL) return;

         // step 4a
          #ifdef DEBUG
            if (!(EVENT_WRITE_NUL))   DEBUG_LOG(0,"State4a: Waiting for event=2:%02x RRWline=1:%02x and !P->CMDLine:%02x",
                                      event,P->RRWLine,P->CMDLine);
          #endif

          if  (EVENT_WRITE_ORA && !P->CMDLine)
              {
                  SET_PROFILE_LOOP_NO_PREDELAY(TENTH_OF_A_SECOND);   // reset timeout when we get a byte
                  // avoid a 2nd write as 0x55
                  if  (P->VIA_PA==0x55 && P->indexwrite==4)
                      {
                          DEBUG_LOG(0,"Ignoring 2nd 0x55 write to avoid sync issues");
                          return;
                      }

                  P->DataBlock[P->indexwrite++]=P->VIA_PA;

                  switch (P->indexwrite-1)
                    {
                          case 4 : DEBUG_LOG(0,"Wrote CMD  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                          case 5 : DEBUG_LOG(0,"Wrote MSB  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                          case 6 : DEBUG_LOG(0,"Wrote mid  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                          case 7 : DEBUG_LOG(0,"Wrote LSB  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                          case 8 : DEBUG_LOG(0,"Wrote RTRY %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                          case 9 : DEBUG_LOG(0,"Wrote SPAR %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                          default: DEBUG_LOG(0,"Wrote ???? %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1);
                    }

                    if (P->indexwrite>542) P->indexwrite=4; // prevent overrun

                    return;
              }

              #ifdef DEBUG
                if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State4b: Waiting for CMDLine");
              #endif

         //step 4b
         if (P->CMDLine) //for lisaos// && !P->RRWLine)  // we are done.
            {
                  // interpret command
                  blocknumber=(P->DataBlock[5]<<16) |
                              (P->DataBlock[6]<< 8) |
                              (P->DataBlock[7]    ) ;
                  DEBUG_LOG(0,"In 4b. Lisa raised CMDLine, might go to step 5. blk#%d",blocknumber);

                  switch (P->DataBlock[4])
                         {                                     //20060515-P->BSYLINE=0 replaced with 1
                            case 0: P->VIA_PA=0x02; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"4b: ACK READ");          // read block
                                    P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                    SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                    return;
                            case 1: P->VIA_PA=0x03; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"4b: ACK WRITE");         // write block
                                    P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                    SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                    return;
                            case 2: P->VIA_PA=0x04; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"4b: ACK WRITE/VERIFY");  // write/verify block
                                    P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                    SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                    return;
                            case 0x12: // Status
                                  {
                                    P->VIA_PA=0x00; P->BSYLine=0; DEBUG_LOG(0,"<!BSY")
                                    P->StateMachineStep=IDLE_STATE; // changeme!
                                    #ifdef DEBUG
                                      { uint8 chk=P->DataBlock[4]+P->DataBlock[5]+P->DataBlock[6]+P->DataBlock[7]+P->DataBlock[8]+P->DataBlock[9]+P->DataBlock[10]+1;
                                        ALERT_LOG(0,"profile.c:widget.c:12: Status block#%d,0x%06x - calc-chk:%02x len:%d buffer:"
                                                    " %02x.%02x.%02x.%02x(%02x )(cmd:%02x) [count:%02x] [%02x:%02x:%02x] %02x %02x %02x %02x %02x %02x   pc24:%08x",
                                        blocknumber, blocknumber, chk, P->indexwrite,
                                        P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2],   P->DataBlock[ 3],
                                        P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6],   P->DataBlock[ 7],
                                        P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10],   P->DataBlock[11],
                                        P->DataBlock[12], P->DataBlock[13], P->DataBlock[14],   P->DataBlock[15],  pc24  );
                                      }
                                    #endif
                                    P->last_cmd=0x1200 | P->DataBlock[5]; // :TODO: insert subcommands here
                                       // 12 00 ED - Read ID
                                       // 12 07 E6 - Soft Reset
                                       // 12 08 E5 - Send Park
                                       // 12 09 E4 - Diag Read
                                       // 12 0B E2 - Diag Write
                                       // 12 0D E0 - Read spare Table
                                       // 12 0E DF - Write Spare table - crossed out?
                                       // 12 11 DC - Read Abort Stat
                                       // 12 12 DB - Reset Servo
                                       // 12 13 DA - Scan for bad blocks.
                                    if  (P->DataBlock[5]==0x0d && P->DataBlock[6]==0xe0)
                                        {
                                          get_spare_table(P);
                                          SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                          ALERT_LOG(0,"widget.c:got 120d0e, sending <0x22"); ALERT_LOG(0,"widget.c<+BSY=1, next state WAIT_2nd_0x55_STATE");
                                          P->VIA_PA=0x22; P->last_a_accs=0; P->BSYLine=1;
                                          P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                          return;
                                        }
                                       //switch (P->DataBlock[5])
                                       //{
                                       // case: 0x0d: do_widget_spare_fffffe(); // read spare table
                                       //}
                                  } // end of status case
                                  break;

                            case 0x26: // SYSREAD   26,00,<count>,<3-bytes-block-id>,checkbyte  Widget_ERS.pdf pdf-pg127
                                       // SYSWRITE  26,01,<count>,<3-bytes-block-id>,checkbyte
                                       // SYSWRITEV 26,02,<count>,<3-bytes-block-id>,checkbyte
                              /*
                              profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - 
                                                                        v  read  +1  blk blk  blk   chk  ??????????????
//                                                 buffer: 80.00.00.00(26 )[00   01  00]:00:  26    b2   31 31 20 20 20
//                                                 must respond with controller ready whatever that means in between block reads
//                                                 checksum= ~1+~2+~3+~4
*/
                                        #ifdef DEBUG
                                          {  uint8 chk=P->DataBlock[4]+P->DataBlock[5]+P->DataBlock[6]+P->DataBlock[7]+P->DataBlock[8]+P->DataBlock[9]+P->DataBlock[10]+1;
                                            ALERT_LOG(0,"profile.c:widget.c:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#%d,0x%06x - calc-chk:%02x len:%d buffer:"
                                                        " %02x.%02x.%02x.%02x(%02x )(cmd:%02x) [count:%02x] [%02x:%02x:%02x] %02x %02x %02x %02x %02x %02x   pc24:%08x",
                                            blocknumber, blocknumber, chk, P->indexwrite,
                                            P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2],   P->DataBlock[ 3],
                                            P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6],   P->DataBlock[ 7],
                                            P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10],   P->DataBlock[11],
                                            P->DataBlock[12], P->DataBlock[13], P->DataBlock[14],   P->DataBlock[15],  pc24  ); }
                                        #endif

                                        P->last_cmd=0x2600 | P->DataBlock[5];
                                        blocknumber=P->blocktowrite=(P->DataBlock[7]<<16) | (P->DataBlock[8]<<8) | P->DataBlock[9];
                                        P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                        SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                       // need to add a widget flag to the profile structure and then keep the block read and count and state
                                       // not sure what the widget ACK is either.
                                        switch (P->DataBlock[5]) // subcommand.
                                        {
                                           // this was invoked.
                                           case 0 : P->VIA_PA=0x22; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"100 4b: SYSREAD ACK");      // read blocks
                                                    P->StateMachineStep=WID_MULTI_READ_SEND_DATA_AND_TAGS_STATE;
                                                    do_widget_read(P,blocknumber);
                                                    SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                                    P->VIA_PA=0x22; P->last_a_accs=0; P->BSYLine=1; 
                                                    return;

                                           case 1 : P->VIA_PA=0x23; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"100 4b: ACK WRITE");         // write blocks
                                                    P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                                    SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                                    return;

                                           case 2 : P->VIA_PA=0x24; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"100 4b: ACK WRITE/VERIFY");  // write/verify block
                                                    P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                                    SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                                    return;
                                          default: ;
                                        }

                            default:  P->VIA_PA=0x00; P->BSYLine=0; DEBUG_LOG(0,"<!BSY")
                                      P->StateMachineStep=IDLE_STATE;
                                      ALERT_LOG(0,"profile.c:widget.c:S4B. Unknown command! Returning %02x as response to Lisa's command %02x and going to step %d  pc24:%08x",
                                                    P->VIA_PA,  P->DataBlock[4],  P->StateMachineStep);
                                      #ifdef DEBUG
                                        {uint8 chk=P->DataBlock[4]+P->DataBlock[5]+P->DataBlock[6]+P->DataBlock[7]+P->DataBlock[8]+P->DataBlock[9]+P->DataBlock[10]+1;
                                          ALERT_LOG(0,"profile.c:widget.c:Unknown CMD: block#%d,0x%06x - calc-chk:%02x len:%d buffer:"
                                                      " %02x.%02x.%02x.%02x(%02x )(cmd:%02x) [count:%02x] [%02x:%02x:%02x] %02x %02x %02x %02x %02x %02x",
                                          blocknumber, blocknumber, chk, P->indexwrite,
                                          P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2], P->DataBlock[ 3],
                                          P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6], P->DataBlock[ 7],
                                          P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10], P->DataBlock[11],
                                          P->DataBlock[12], P->DataBlock[13], P->DataBlock[14], P->DataBlock[15],   pc24); }
                                      #endif

                          } // end of switch (P->DataBlock[4])
             } // end of if  (P->CMDLine) //for lisaos// && !P->RRWLine)  // we are done.

        return;

    case WAIT_2nd_0x55_STATE:       // 5

        #ifdef DEBUG
        if (!(EVENT_WRITE_NUL))               DEBUG_LOG(0,"State 5");
        #endif

        CHECK_PROFILE_LOOP_TIMEOUT;

        #ifdef DEBUG
        if (!(EVENT_WRITE_NUL))
        DEBUG_LOG(0,"State5: Waiting for EVENT_WRITE_ORA:%02x && P->last_a_accs:%02x && P->CMDLine>0:%02x && RRWLine>0:%02x && PortA=0x55|0x69:%02x",
                event,
                P->last_a_accs,
                P->CMDLine,
                P->RRWLine,
                P->VIA_PA);
        #endif

        P->BSYLine=1; DEBUG_LOG(0,"<+BSY") //2006.05.09 was 0
        if  (EVENT_WRITE_NUL) return;
        if  (EVENT_WRITE_ORA) //  && P->last_a_accs && P->CMDLine && P->RRWLine)
            {   DEBUG_LOG(0,"State 5: checking what we got:%02x==0x55",P->VIA_PA);
                if  (P->VIA_PA==0x55)
                    {
                        P->StateMachineStep=PARSE_CMD_STATE;
                        SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);
                        PRO_STATUS_GOT55;
                        DEBUG_LOG(0,"State5: got 0x55 w00t!");
                    }
                else
                    {
                        P->StateMachineStep=0; // possibly our old code
                        PRO_STATUS_NO55;
                        DEBUG_LOG(0,"State5: going into state 0 now. oh well.");
                    }
            } // if EVENT_WRITE_ORA
        return;

    case  PARSE_CMD_STATE:   //6 // now we execute the command after simulating a busy profile
         // play some profile sounds now?
          CHECK_PROFILE_LOOP_TIMEOUT;

          if  ( !TIMEPASSED_PROFILE_LOOP(HUN_THOUSANDTH_OF_A_SEC) )  // this block was disabled -200604025
              {

                #ifdef DEBUG
                if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State 6 - wasting cycles for a bit to simulate a busy profile (%d cycles)",PROFILE_WAIT_EXEC_CYCLE);
                #endif

                return;
              }

          PRO_STATUS_CLEAR;

          #ifdef DEBUG
          if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"Done wasting cycles in step 6");
          #endif

          P->indexread=0;

          blocknumber=(P->DataBlock[5]<<16) | (P->DataBlock[6]<< 8) |  (P->DataBlock[7]    ) ;

          P->BSYLine=1; //20060425-moved from step 5, and re-enabled above delay to slow down
          if (EVENT_WRITE_NUL) return;

          switch (P->DataBlock[4])                                                       // Now execute the command
          {
              case 0 :                                                   //   0     1    2    3    4     5   6    7     8     9   10   11   12   13   14  15
                      DEBUG_LOG(0,"step6: Reading block#%d,0x%06x - buffer: %02x.%02x.%02x.%02x(%02x )[%02x %02x %02x]:%02x:%02x %02x %02x %02x %02x %02x %02x",
                        blocknumber, blocknumber,
                        P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2],  P->DataBlock[ 3],
                        P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6],  P->DataBlock[ 7],
                        P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10],  P->DataBlock[11],
                        P->DataBlock[12], P->DataBlock[13], P->DataBlock[14],  P->DataBlock[15]);

                      do_profile_read(P,blocknumber);
                      P->StateMachineStep=SEND_DATA_AND_TAGS_STATE;
                      SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);
                      break;
                      //no, this is bad//PRO_STATUS_GOT55;  // force status[0] to return the got 0x55 status.

              case 1 : // FALLTHROUGH
              case 2 :
                      P->StateMachineStep=ACCEPT_DATA_FOR_WRITE_STATE;
                      P->indexwrite=10;  // bytes 0-3 are status, 4-6 are cmd block, 10-522 are byte, 523-542 are tags.
                      SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);
                      DEBUG_LOG(0,"step6: (cmd:%02x) Fixin to write to block#%d,0x%06x - buffer: %02x.%02x.%02x.%02x(%02x )[%02x %02x %02x]:%02x:%02x %02x %02x %02x %02x %02x %02x",
                        P->DataBlock[4], blocknumber, blocknumber,
                        P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2],  P->DataBlock[ 3],
                        P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6],  P->DataBlock[ 7],
                        P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10],  P->DataBlock[11],
                        P->DataBlock[12], P->DataBlock[13], P->DataBlock[14],  P->DataBlock[15]);
                     break;  // write/verify block

              case 0x12: // Status
                      P->VIA_PA=0x00; P->BSYLine=0; DEBUG_LOG(0,"<!BSY")
                      P->StateMachineStep=IDLE_STATE; // changeme!

                      #ifdef DEBUG
                      {uint8 chk=P->DataBlock[4]+P->DataBlock[5]+P->DataBlock[6]+P->DataBlock[7]+P->DataBlock[8]+P->DataBlock[9]+P->DataBlock[10]+1;
                      ALERT_LOG(0,"profile.c:widget.c:12: Status block#%d,0x%06x - calc-chk:%02x len:%d buffer:"
                                  " %02x.%02x.%02x.%02x(%02x )(cmd:%02x) [count:%02x] [%02x:%02x:%02x] %02x %02x %02x %02x %02x %02x   pc24:%08x",
                                  blocknumber, blocknumber, chk, P->indexwrite,
                        P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2],  P->DataBlock[ 3],
                        P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6],  P->DataBlock[ 7],
                        P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10],  P->DataBlock[11],
                        P->DataBlock[12], P->DataBlock[13], P->DataBlock[14],  P->DataBlock[15], pc24);}
                      #endif

                      P->last_cmd=0x1200 | P->DataBlock[5];

                        //switch (P->DataBlock[5])
                        //       {
                        //           case: 0x0d: do_widget_spare_fffffe(); // read spare table
                        //       }
                        break;


               case 0x26: // SYSREAD   26,00,<count>,<3-bytes-block-id>,checkbyte  Widget_ERS.pdf pdf-pg127
                                         // SYSWRITE  26,01,<count>,<3-bytes-block-id>,checkbyte
                                         // SYSWRITEV 26,02,<count>,<3-bytes-block-id>,checkbyte
                              /*
                              profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - buffer: 80.00.00.00(26 )[00 01 00]:00:26 b2 31 31 20 20 20
profile.c:ProfileLoop:1182:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#256,0x000100 - 
//                                                                      v  read  +1  blk blk  blk   chk  ??????????????
//                                                 buffer: 80.00.00.00(26 )[00   01  00]:00:  26    b2   31 31 20 20 20
//                                                 must respond with controller ready whatever that means in between block reads
//                                                 checksum= ~1+~2+~3+~4
*/
                            #ifdef DEBUG
                            { uint8 chk=P->DataBlock[4]+P->DataBlock[5]+P->DataBlock[6]+P->DataBlock[7]+P->DataBlock[8]+P->DataBlock[9]+P->DataBlock[10]+1;
                              ALERT_LOG(0,"profile.c:widget.c:26: SYSCMD 26,[01R|02W|03WV],<count>,<3-bytes-block-id>,checkbyte block#%d,0x%06x - calc-chk:%02x len:%d buffer:"
                                          " %02x.%02x.%02x.%02x(%02x )(cmd:%02x) [count:%02x] [%02x:%02x:%02x] %02x %02x %02x %02x %02x %02x   pc24:%08x",
                                            blocknumber, blocknumber, chk, P->indexwrite,
                                            P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2],  P->DataBlock[ 3],
                                            P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6],  P->DataBlock[ 7],
                                            P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10],  P->DataBlock[11],
                                            P->DataBlock[12], P->DataBlock[13], P->DataBlock[14],  P->DataBlock[15], pc24);   }
                            #endif
                            P->last_cmd=0x2600 | P->DataBlock[5];
                            P->blocktowrite=(P->DataBlock[7]<<16) | (P->DataBlock[8]<<8) | P->DataBlock[9];
                            P->VIA_PA=0x22; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"<+BSY")
                            P->StateMachineStep=WAIT_2nd_0x55_STATE;
                            SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                            // need to add a widget flag to the profile structure and then keep the block read and count and state
                            // not sure what the widget ACK is either.
                            switch (P->DataBlock[5]) // subcommand.
                                  {
                                       case 0 : P->VIA_PA=0x22; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"6 PARSE_CMD_STATE-> SYSCMD READ ACK");      // read blocks
                                                P->StateMachineStep=WID_MULTI_READ_SEND_DATA_AND_TAGS_STATE;
                                                do_profile_read(P,blocknumber);
                                                SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                                return;

                                       case 1 : P->VIA_PA=0x23; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"6 PARSE_CMD_STATE-> SYSCMD ACK WRITE");         // write blocks
                                                P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                                SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                                return;

                                       case 2 : P->VIA_PA=0x24; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"6 PARSE_CMD_STATE-> SYSCMD ACK WRITE/VERIFY");  // write/verify block
                                                P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                                SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                                return;
                                  }

              default:    ALERT_LOG(0,"profile.c:widget.c:Unhandled Profile/Widget command: %02x",P->DataBlock[4]);
                          ALERT_LOG(0,"profile.c:widget.c:step6: (cmd:%02x) block#%d,0x%06x - buffer: %02x.%02x.%02x.%02x(%02x )[%02x %02x %02x]:%02x:%02x %02x %02x %02x %02x %02x %02x   pc24:%08x",
                          P->DataBlock[4],blocknumber, blocknumber,
                          P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2],  P->DataBlock[ 3],
                          P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6],  P->DataBlock[ 7],
                          P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10],  P->DataBlock[11],
                          P->DataBlock[12], P->DataBlock[13], P->DataBlock[14],  P->DataBlock[15], pc24);
                          P->StateMachineStep=IDLE_STATE;
        }
        return;


    case  ACCEPT_DATA_FOR_WRITE_STATE:    // 7    // handle write/write+verify - read bytes from lisa into buffer
          CHECK_PROFILE_LOOP_TIMEOUT;
          P->BSYLine=0; DEBUG_LOG(0,"<!BSY")
          #ifdef DEBUG
          if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State 7 - ready to read bytes from ProFile for writes event:%02x bsy:%02x cmd:%02x rrw:%02x PA:%02x @%d",
                                              event, P->BSYLine,   P->CMDLine,  P->RRWLine, P->VIA_PA, P->indexwrite );
          #endif
          if (EVENT_WRITE_NUL) return; //2006.05.19

          if (EVENT_WRITE_ORA && P->RRWLine && !P->CMDLine)
          {
            P->DataBlock[P->indexwrite++]=P->VIA_PA;
            if (P->indexwrite>552) {PRO_STATUS_BUFFER_OVERFLOW; P->StateMachineStep=0; DEBUG_LOG(0,"State 7 write buffer Overflow");}
            else SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);

          }
          else
          { if  (P->CMDLine)
                {
                  P->BSYLine=1; DEBUG_LOG(0,"<+BSY")
                  P->VIA_PA=0x06;
                  DEBUG_LOG(0,"State7 ACK WRITE command with 06");
                  P->StateMachineStep=WAIT_3rd_0x55_STATE;
                  SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);
                }
            else
                { DEBUG_LOG(0,"State 7 did not recognize byte event:%02x bsy:%02x cmd:%02x rrw:%02x", event, P->BSYLine, P->CMDLine, P->RRWLine); }
          }
          return;


    case  WAIT_3rd_0x55_STATE:              // 8    // wait for 0x55 again
          #ifdef DEBUG
            if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State 8 - wait for 3rd 0x55 - write:bsy:%d",P->BSYLine);
          #endif

          CHECK_PROFILE_LOOP_TIMEOUT;
          //
          //if (EVENT_WRITE_ORA && P->RRWLine && !P->CMDLine)
          //
          P->BSYLine=1; DEBUG_LOG(0,"<+BSY") //no this should always be 1 - do not change it!
          if (EVENT_WRITE_NUL) return;

          if  (EVENT_WRITE_ORA)
              {
                if  (P->VIA_PA==0x55)
                    {
                        P->StateMachineStep=WRITE_BLOCK_STATE; // accept command
                        SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);
                        P->indexread=0;
                        PRO_STATUS_GOT55;
                        DEBUG_LOG(0,"Command accepted, transition to 9");
                    }
                else
                    {P->StateMachineStep=0; PRO_STATUS_NO55;}
              }
          return;

    case  WRITE_BLOCK_STATE:     // 8  // do the write and waste some time
          #ifdef DEBUG
            if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State 9 - write and waste more time (%d)",PROFILE_WAIT_EXEC_CYCLE);
          #endif

          CHECK_PROFILE_LOOP_TIMEOUT;
          if ( !TIMEPASSED_PROFILE_LOOP(HUN_THOUSANDTH_OF_A_SEC) ) return;

          blocknumber=(P->DataBlock[5]<<16) |
                      (P->DataBlock[6]<< 8) |
                      (P->DataBlock[7]    ) ;

         #ifdef DEBUG                                                              //   0     1    2    3    4     5   6    7     8     9   10   11   12   13   14  15
          DEBUG_LOG(0,"Writing block#%d,0x%06x - buffer: %02x.%02x.%02x.%02x(%02x )[%02x %02x %02x]:%02x:%02x|%02x %02x %02x %02x %02x %02x", blocknumber, blocknumber,
            P->DataBlock[ 0], P->DataBlock[ 1], P->DataBlock[ 2], P->DataBlock[ 3],
            P->DataBlock[ 4], P->DataBlock[ 5], P->DataBlock[ 6], P->DataBlock[ 7],
            P->DataBlock[ 8], P->DataBlock[ 9], P->DataBlock[10], P->DataBlock[11],
            P->DataBlock[12], P->DataBlock[13], P->DataBlock[14], P->DataBlock[15]);
          //                   0    1    2    3   4   5    6    7    8    9     10   11   12   13  14   15   16   17   18   19
          DEBUG_LOG(0,"Tags: %02x %02x %02x %02x[%02x %02x]%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
            P->DataBlock[522+ 0],  P->DataBlock[522+ 1],  P->DataBlock[522+ 2],  P->DataBlock[522+ 3],
            P->DataBlock[522+ 4],  P->DataBlock[522+ 5],  P->DataBlock[522+ 6],  P->DataBlock[522+ 7],
            P->DataBlock[522+ 0],  P->DataBlock[522+ 9],  P->DataBlock[522+ 10], P->DataBlock[522+ 11],
            P->DataBlock[522+ 12], P->DataBlock[522+ 13], P->DataBlock[522+ 14], P->DataBlock[522+ 15],
            P->DataBlock[522+ 16], P->DataBlock[522+ 17], P->DataBlock[522+ 18], P->DataBlock[522+ 19]                            );
          #endif

          do_profile_write(P,blocknumber);

          P->indexwrite=4;
          P->indexread=0;  P->DataBlock[0]=0;  P->DataBlock[1]=0; P->DataBlock[2]=0;  P->DataBlock[3]=0;

          P->BSYLine=0;  DEBUG_LOG(0,"<!BSY") //2006.05.17 was 1
          P->StateMachineStep=SEND_STATUS_BYTES_STATE;
          SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);

          return;

    case  WID_MULTI_READ_SEND_DATA_AND_TAGS_STATE:     //10    // Let Lisa read the status/data
          #ifdef DEBUG
          if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State 100, Multiblock allow Lisa to read the status and data  - pointer:0x%x (%d)",P->indexread,P->indexread);
          #endif

          P->BSYLine=0; DEBUG_LOG(0,"<!BSY")

          CHECK_PROFILE_LOOP_TIMEOUT;
          if (EVENT_WRITE_NUL) return;

          if ( P->CMDLine) /// this might be different for Widget - was !
          {
            P->BSYLine=0; DEBUG_LOG(0,"<!BSY")           //2006.05.15

            if (EVENT_READ_IRA)
            {
             //P->BSYLine=0;        //2006.05.15
              P->VIA_PA=P->DataBlock[P->indexread++];
              if (P->indexread>542) {   DEBUG_LOG(0,"IndexRead went over 542, resetting to 0");  P->indexread=0; }

              P->last_a_accs=0;
              SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);   // reset timeout  // was FIFTH_OF_A_SECOND
              DEBUG_LOG(0,"Returning Profile Read buffer %02x from index:0x%x(%d)",P->VIA_PA,P->indexread-1,P->indexread-1);
            }

          }
          else
          {
              #ifdef DEBUG
                if  (P->indexread != 536)
                    {   blocknumber=(P->DataBlock[5]<<16) | (P->DataBlock[6]<< 8) |    (P->DataBlock[7]    );
                        DEBUG_LOG(0,"Warning: read %d bytes instead of 536 for block read of sector #%08x (%x)",P->indexread,blocknumber,blocknumber); }
              #endif

            if  (EVENT_READ_IRA)             // oops we fell out of sync - recover please!
                {
                    P->VIA_PA=1;   P->last_a_accs=0;
                    P->StateMachineStep=WAIT_1st_0x55_STATE;
                    SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);
                    P->BSYLine=1;           // flip BSY
                    return;
                }

            P->BSYLine=1; DEBUG_LOG(0,"<+BSY")
            P->StateMachineStep=FINAL_FLIP_TO_IDLE_STATE; SET_PROFILE_LOOP_TIMEOUT(TEN_THOUSANDTH_OF_A_SEC);
          }

          return;


// Normal ProFile protocol or cmd 0
    case  SEND_DATA_AND_TAGS_STATE:     //10    // Let Lisa read the status/data
          #ifdef DEBUG
            if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State 10, allow Lisa to read the status and data  - pointer:0x%x (%d)",P->indexread,P->indexread);
          #endif

          P->BSYLine=0; DEBUG_LOG(0,"<!BSY")

          CHECK_PROFILE_LOOP_TIMEOUT;
          if (EVENT_WRITE_NUL) return;

          if  (!P->CMDLine)
              {

                P->BSYLine=0; DEBUG_LOG(0,"<!BSY")           //2006.05.15
                if  (EVENT_READ_IRA)
                    {
                        //P->BSYLine=0;        //2006.05.15
                        P->VIA_PA=P->DataBlock[P->indexread++];
                        if (P->indexread>542)
                        {
                            DEBUG_LOG(0,"IndexRead went over 542, resetting to 0");
                            P->indexread=0;
                        }

                        P->last_a_accs=0;
                        SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);   // reset timeout  // was FIFTH_OF_A_SECOND
                        DEBUG_LOG(0,"Returning Profile Read buffer %02x from index:0x%x(%d)",P->VIA_PA,P->indexread-1,P->indexread-1);
                    }
              }
          else
              {
                  #ifdef DEBUG
                    if (P->indexread != 536) {blocknumber=(P->DataBlock[5]<<16) | (P->DataBlock[6]<< 8) |    (P->DataBlock[7]    );
                          DEBUG_LOG(0,"Warning: read %d bytes instead of 536 for block read of sector #%08x (%x)",P->indexread,blocknumber,blocknumber);  }
                  #endif

                  if  (EVENT_READ_IRA)             // oops we fell out of sync - recover please!
                      {
                        P->VIA_PA=1;   P->last_a_accs=0;
                        P->StateMachineStep=WAIT_1st_0x55_STATE;
                        SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);
                        P->BSYLine=1; DEBUG_LOG(0,"<!BSY")          // flip BSY
                        return;
                      }

                  P->BSYLine=1;DEBUG_LOG(0,"<+BSY")
                  P->StateMachineStep=FINAL_FLIP_TO_IDLE_STATE; SET_PROFILE_LOOP_TIMEOUT(TEN_THOUSANDTH_OF_A_SEC);
                  P->VIA_PA=0x22; 
              } // end of if  (!P->CMDLine) else

          return;



    case  FINAL_FLIP_TO_IDLE_STATE:        // 11
          // let ProFile see BSY strobe for a short period, this speeds up LisaTest immensely.  returns to state 0 via timeout
          CHECK_PROFILE_LOOP_TIMEOUT;
          if  (EVENT_READ_IRA)                 // oops we fell out of sync - recover please!
              {
                P->VIA_PA=1;   P->last_a_accs=0;
                P->StateMachineStep=WAIT_1st_0x55_STATE;
                SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);
                P->BSYLine=1;           // flip BSY
                return;
              }

          P->BSYLine=1; DEBUG_LOG(0,"<+BSY")
          return;

    case  SEND_STATUS_BYTES_STATE:              //12              // Let Lisa read the status/data

          #ifdef DEBUG
            if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State 12, post write - allow Lisa to read the status and data  - pointer:%d",P->indexread);
          #endif
 
          CHECK_PROFILE_LOOP_TIMEOUT;
          P->BSYLine=0; DEBUG_LOG(0,"<+BSY")
          if (EVENT_WRITE_NUL || EVENT_READ_IRB) return;

          if  (!P->CMDLine)
              {
                if  (EVENT_READ_IRA)
                    {
                      P->VIA_PA=P->DataBlock[P->indexread++]; if (P->indexread>3) P->indexread=0;
                      P->last_a_accs=0;
                      SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);   // reset timeout  // was FIFTH_OF_A_SECOND
                      ALERT_LOG(0,"profile.c:widget.c:Returning %02x from index:%d   pc24:%08x",P->VIA_PA,P->indexread-1,  pc24);
                    }
              }
              else  P->StateMachineStep=IDLE_STATE;


          return;

    default:  ALERT_LOG(0,"profile.c:widget.c:Unknown ProFile state %d, returning to idle.   pc24:%08x",P->StateMachineStep, pc24);
              P->StateMachineStep=IDLE_STATE;
              return;
    }

}


// I gotta get home, dirty,
// I haveta code.
// we gotta code all day all night
// You know it sounds right         - mc++
