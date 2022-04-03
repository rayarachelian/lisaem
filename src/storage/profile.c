/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2022.04.01                   *
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
*                         Profile Hard Disk  Routines                                  *
*                                                                                      *
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


char ProFile_Spare_table[536]=
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
    'P','R','O','F','I','L','E',' ',' ',' ',' ',' ',0 ,                 // device name
//  d  e  f
    0, 0, 0,   // 0,0,0x10-10MB ProFile                            // device #
//  0x10,0x11
    0x03, 0x98,                                                    // ProFile firmware revsion
//  0x12, 0x13, 0x14
    0x00, 0x26, 0x00,                                              // # of available blocks to user *play with this*
    /*20, 21 */
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


/*
 * do the actual reading and writing of the blocks off the profile hard drive image file
 *
 */

void ProfileReset(ProFileType *P);
void ProfileResetOff(ProFileType *P);


#ifdef DEBUG

// This is an appender log, flush+close so that incase there's a crash, no pending data gets lost.

void dump_profile_block(FILE *f, uint8 *RAM, long sectornumber, char *text)
{
   int i,j;
   char c;

   if (!debug_log_enabled) return;

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


void get_profile_spare_table(ProFileType *P)
{
   // copy spare table template
    DEBUG_LOG(0,"Profile spare table");
    memset(&P->DataBlock[4],0,536);
    memcpy(&P->DataBlock[4],ProFile_Spare_table,512);


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

     case 262144: a='1'; b='2'; c='8';    break;//128M
     case 524288: a='2'; b='5'; c='6';    break;//256M // 0x080000
     case 1048576:a='5'; b='1'; c='2';    break;//512M // 0x100000
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

    if (block<0x00f00000)   block=deinterleave5(block);

	if ( block==0x00fffffe)                 // return ProfileRAM buffer contents
	{
		P->indexread=0;                     // reset index pointers to status
		P->indexwrite=4;

        dump_profile_block(buglog, &(P->DataBlock[4]), block, "read");
        dump_profile_block_to_log( &(P->DataBlock[4]), block, "read");
		return;
	}


    if ( block==0x00ffffff)  {get_profile_spare_table(P); return;} // return Profile spare table


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
       {ALERT_LOG(0,"Read sector from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}

    if (blk!=NULL) memcpy( &(P->DataBlock[4+P->DC42.tagsize]), blk, P->DC42.datasize); //4

    if (block==0) {
        bootblockchecksum=0;
        for (uint32 i=0; i<P->DC42.datasize; i++) bootblockchecksum=( (bootblockchecksum<<1) | ((bootblockchecksum & 0x80000000) ? 1:0) ) ^ blk[i] ^ i;
    }

    blk=dc42_read_sector_tags(&(P->DC42),block);

    if (block==0) {
        for (uint32 i=0; i<P->DC42.tagsize; i++) bootblockchecksum=( (bootblockchecksum<<1) | ((bootblockchecksum & 0x80000000) ? 1:0) ) ^ blk[i] ^ i;
        ALERT_LOG(0,"Bootblock checksum:%08x for %s",bootblockchecksum,P->DC42.fname);
    }


    if (P->DC42.retval || blk==NULL)
       {ALERT_LOG(0,"Read tags from blk#%d failed with error:%d %s",block,P->DC42.retval,P->DC42.errormsg);}

    if (blk!=NULL) memcpy( &(P->DataBlock[4       ]), blk, P->DC42.tagsize);   //4+512

    #ifdef DEBUG
    dump_profile_block(buglog, &(P->DataBlock[4]), block, "read");
    #endif

	if ( errno )
	{
        ALERT_LOG(0,"Error reading ProFile Image! errno: %d, State:%d,IndexWrite:%d,ProfileCommand:%d,BSY:%d,Data:%d,CMDL:%d,buffer:%2x:%2x:%2x:%2x:%2x:%2x\n",
			errno,
			P->indexwrite,
			P->StateMachineStep,P->Command,P->BSYLine,P->VIA_PA,P->CMDLine,
			P->DataBlock[4],P->DataBlock[5],P->DataBlock[6],P->DataBlock[7],
			P->DataBlock[8],P->DataBlock[9]);

	}

	P->indexread=0;                     // reset index pointers to status
	P->indexwrite=4;
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

    DEBUG_LOG(0,"ProFile write request block #%ld 0x%08x deinterleaved:%ld 0x%08x\n",block,block,oblock,oblock);

	if ( block==0x00fffffe)                 // write ProfileRAM buffer contents
	{
		P->indexread=0;                     // reset index pointers to status
		P->indexwrite=4;
		return;
	}

	if ( block==0x00ffffff)                 // pretend to write ProfileRAM buffer contents
	{
       // copy spare table template

		for ( i=0; i<532; i++) {P->DataBlock[4+i]=ProFile_Spare_table[i];}

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



extern void update_profile_preferences_path(char *newfilename);


void profile_unmount(void)
{
    int i;

    for ( i=2; i<9; i++)
    {
        if (via[i].ProFile)
        {
            ALERT_LOG(0,"Shutting down profile at via #%d for shutdown/reboot. dc42:%p",i,via[i].ProFile->DC42); 
            dc42_close_image(&via[i].ProFile->DC42);
        }
    }
}

int profile_mount(char *filename, ProFileType *P)
{
 int i;

 // Open the profile - if it doesn't exist, create it.
 //9728+1 - 5mb profile + space for spare table at -1.


ALERT_LOG(0,"Attempting to open profile:%s",filename);
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

            ALERT_LOG(0,"Did not find %s, asking user what size drive to create",filename);
            sz=pickprofilesize(filename,1);  if (sz<-1 || sz>6) return -1;

            if (sz==-1) {
                ALERT_LOG(0,"User chose to select an existing profile instead");                
                update_profile_preferences_path(filename);
                i=dc42_open(&P->DC42,filename,"wm");
                return ( i ? -1 : 0 );
            }

            i=dc42_create(filename,"-lisaem.sunder.net hd-",blocks[sz]*512,blocks[sz]*20);
            i=dc42_open(&P->DC42,filename,"wm");
            if (i) {ALERT_LOG(0,"Failed to open newly created ProFile drive!");}
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
        "read IRB",
        "read IRA",
        "write ORA",
        "write ORB",
        "write NUL"
     };


// implement a timeout that resets the state machine back to state 0, and various delays.

                                        // alarm_len_e gets set to x 1st to avoid using (x) twice - avoids macro side effects
#define SET_PROFILE_LOOP_TIMEOUT(x)     {P->alarm_len_e=(x); P->clock_e=cpu68k_clocks + P->alarm_len_e;}

// same as above, but disable pre-entry delay
#define SET_PROFILE_LOOP_NO_PREDELAY(x) {P->alarm_len_e=0;   P->clock_e=cpu68k_clocks + (x);           }

#define CHECK_PROFILE_LOOP_TIMEOUT      {if (P->clock_e<=cpu68k_clocks)                                      \
                                            {                                                                \
                                             if (P->StateMachineStep==GET_CMDBLK_STATE && !P->BSYLine)       \
                                                {P->BSYLine=2;                                               \
                                                DEBUG_LOG(0,"force State:4b at timeout");     }              \
                                             else if (P->StateMachineStep!=IDLE_STATE) {                     \
                                                 DEBUG_LOG(0,"ZZZZZZZ Timeout, back to state 0 ZZZZZZZZZZ"); \
 	                                             P->StateMachineStep=IDLE_STATE;                             \
                                                 SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);                \
                                                 P->last_cmd=1;                                              \
                                                 return;    }                                                \
                                            }                                                                \
                                        }


// have we gone past x clk cycles since the last time the timeout was set?  useful for faking delays.
#define TIMEPASSED_PROFILE_LOOP(x) ((P->clock_e - P->alarm_len_e+(x)) < cpu68k_clocks)

#define TIMEPASSED_LEFT(x) ((P->clock_e - P->alarm_len_e+(x)) )

// this disables the above, allowing us to skip the initial delay before a state becomes active.
#define DISABLE_TIMEPASSED_DELAY   {P->alarm_len_e=0;}



//    -- actual clock_expiration          this is the bug, the issue is that it's the size of the window
//    +                 .                 not the mini expiration I'm looking for.
//    |\                .
//    | \__alarm_e_len  .
//    | /        <cpu68k_clocks
//    |/                .
//    |    +            .
//    -- clock set      .                                    -- actually, no, this should work just fine

// steps of ProFile state machine

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

char *profile_state_names[]={
   /*  0 */ "Idle",
   /*  1 */ "N/A",
   /*  2 */ "WAIT_1st_0x55_STATE",
   /*  3 */ "N/A-Delay-Before-CMD",
   /*  4 */ "GET_CMDBLK_STATE",
   /*  5 */ "WAIT_2nd_0x55_STATE",
   /*  6 */ "PARSE_CMD_STATE",
   /*  7 */ "ACCEPT_DATA_FOR_WRITE_STATE",
   /*  8 */ "WAIT_3rd_0x55_STATE",
   /*  9 */ "WRITE_BLOCK_STATE",
   /* 10 */ "SEND_DATA_AND_TAGS_STATE",
   /* 11 */ "FINAL_FLIP_TO_IDLE_STATE",
   /* 12 */ "SEND_STATUS_BYTES_STATE"
};

extern void  apply_los31_hacks(void);
extern void apply_mw30_hacks(void);

void ProfileLoop(ProFileType *P, int event)
{
    uint32 blocknumber=0;

    if (  !(profile_power & (1<<(P->vianum-2)) )  ) return;

    if ( !P->DENLine && P->vianum==2) {DEBUG_LOG(0,"DEN is disabled on via#%d- ignoring ProFile commands",P->vianum); return;}                   // Drive Enabled is off (active low 0=enabled, 1=disable profile)
    if ( !P->DENLine && P->vianum!=2) {DEBUG_LOG(0,"DEN is disabled on via#%d- ignoring ProFile commands",P->vianum); return;}                   // Drive Enabled is off (active low 0=enabled, 1=disable profile)
        
#ifdef DEBUG
if (!EVENT_WRITE_NUL)
    DEBUG_LOG(0,"ProFile access at PC:%d/%08x event:%d %s state:%d %s idxr,idxw: %d,%d bsy:%d cmd:%d rrw:%d",context,reg68k_pc,event,profile_event_names[event],
          P->StateMachineStep,profile_state_names[P->StateMachineStep],P->indexread,P->indexwrite,
          P->BSYLine,P->CMDLine,P->RRWLine);
#endif

    // Patch UniPlus loader handshaking so it doesn't fail on profile handshaking as UniPlus does super timing specific checks.
    if (uniplus_loader_patch) {
       if (running_lisa_os==0 && ((reg68k_pc & 0xffffff00)==0x060000) && context==1)
       {
          if (lisa_rw_ram(0x0006281c)==0x66ea) {lisa_ww_ram(0x0006281c,0x4e71); ALERT_LOG(0,"*** UniPlus Loader handshake BNE  patched  ***");}
          if (lisa_rw_ram(0x00062a52)==0x0000) {lisa_ww_ram(0x00062a52,0x0001); ALERT_LOG(0,"*** UniPlus Loader BSY delay wait patched ****");}

          // disable serial number check on loader - incase it's from a BLU image
          if (lisa_rl_ram(0x00060090)==0x2f3c0006 && lisa_rl_ram(0x00060094)==0x56c04eb9 && lisa_rl_ram(0x00060098)==0x00060438) {
              lisa_wl_ram(0x00060090,0x42804287);    lisa_wl_ram(0x00060094,0x4E714E71);    lisa_wl_ram(0x00060098,0x4E714E71);
             ALERT_LOG(0,"*** UniPlus Loader serial patched ****");
          }
          // uniplus serno check: sec 1 floppy or 13 profile/widget: 2f 3c 00 06 56 c0 4e b9 00 06 04 38 - JSR to SN check + push/pop
          //                                                         42 80 42 87 4E 71 4E 71 4E 71 4E 71 (CLRL D0; CLRL D7; NOP 4x)
          uniplus_loader_patch=0; // disable check now that we've applied the patch
       }
    }
    else if (running_lisa_os!=0) uniplus_loader_patch=0; // if another OS is running disable this check

    #ifdef XXDEBUG
    if (running_lisa_os==LISA_XENIX_RUNNING)
    {
            DEBUG_LOG(0,"----------------------------------------------------------------------");
            DEBUG_LOG(0,"Xenix ProFile State:         %04x",lisa_ram_safe_getword(1,0x0001d766) );
            DEBUG_LOG(0,"Xenix ProFile VIA address:   %08x",lisa_ram_safe_getlong(1,0x0001d768) );
            DEBUG_LOG(0,"Xenix ProFile status bytes:  %08x",lisa_ram_safe_getlong(1,0x0001d770) ); // 1d776+c=1D772 so c isn't the size of this.
            DEBUG_LOG(0,"Xenix ProFile unknown 1d774: %08x",lisa_ram_safe_getlong(1,0x0001d774) );
            DEBUG_LOG(0,"Xenix ProFile unknown 1d778: %08x",lisa_ram_safe_getlong(1,0x0001d778) );
            //if (debug_log_enabled) fdumpvia2(buglog);
            DEBUG_LOG(0,"----------------------------------------------------------------------");
    }
    #endif


 switch (P->StateMachineStep)
 {
     
    case IDLE_STATE:                           // 0 StateMachineStep is idle - wait for CMD to go low, then BUSY=0
         CHECK_PROFILE_LOOP_TIMEOUT;

         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"State:0 now - idle - CMDLine is:%d && P->last_cmd:%d if non zero, should see state transition.",P->CMDLine,P->last_cmd);
         #endif

         P->BSYLine=0;
         if (EVENT_WRITE_NUL) return;

        // not active.  If Lisa lowers CMD, in response we lower BSYLine and goto state 2
         if (P->CMDLine && P->last_cmd)          {
                                                P->StateMachineStep=WAIT_1st_0x55_STATE;
                                                SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);

                                                P->BSYLine=1;           // flip BSY
                                                P->VIA_PA=0x01;         // ACK cmd on bus
                                                P->last_a_accs=0;

                                                apply_los31_hacks();
                                                apply_mw30_hacks();

                                                DEBUG_LOG(0,"ACK CMD - sending 01 - State transition to State:2");
                                                return;
                                              }

         if (!P->CMDLine)  P->last_cmd=1;          // prevent a too early entry into state 3
         else             {
                           P->last_cmd=0;
                           SET_PROFILE_LOOP_TIMEOUT(HUN_THOUSANDTH_OF_A_SEC);
                           DEBUG_LOG(0,"cmd is still 0, lengthening timeout");

                          }


         return;


  //  case 1:                                    // waiting for CMD==1, if it does, raise BSY and ack with 01.
  //      CHECK_PROFILE_LOOP_TIMEOUT;
  //
  //      P->last_cmd=0;                            // clear return to state 0 to wait for cmd toggle
  //
  //       #ifdef DEBUG
  //       if (!(EVENT_WRITE_NUL))
  //           DEBUG_LOG(0,"State:1 - waiting for CMD==0, if I get it will raise BSY. BSY:%d CMD:%d",P->BSYLine,P->CMDLine);
  //       #endif
  //
  //
  //       P->BSYLine=1;
  //
  //
  //       P->BSYLine=0;
  //       P->VIA_PA=0x01;
  //       P->last_a_accs=0;
  //
  //       P->StateMachineStep=3;     //skip 2 from now on
  //       SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);
  //       DEBUG_LOG(0,"ACK CMD - sending 01 - State transition to Step:3");
  //
  //
  //       return;



    case WAIT_1st_0x55_STATE:                    // 2        // Is CMDLine supposed to be down now?

         CHECK_PROFILE_LOOP_TIMEOUT;
		
//20201029//         // basically, waith 1/100t of a second before flipping BSY, or if Lisa wrote a byte, then skip the wait
//20201029//        if ( !TIMEPASSED_PROFILE_LOOP(HUN_THOUSANDTH_OF_A_SEC)  && !EVENT_WRITE_ORA) //20060429// was 1/1000th sec
//20201029//           {
//20201029//            #ifdef DEBUG                          // don't fill up the log with useless shit
//20201029//            if (!(EVENT_WRITE_NUL))
//20201029//
//20201029//            DEBUG_LOG(0,"State:2 - wasting while waiting for 55, got %02x, last_a_accs:%d before turning BSY to 0: left:%016llx",
//20201029//               P->VIA_PA,
//20201029//               P->last_a_accs,
//20201029//               (TIMEPASSED_LEFT(HUN_THOUSANDTH_OF_A_SEC)-cpu68k_clocks)
//20201029//               );
//20201029//            #endif
//20201029//
//20201029//
//20201029//            P->BSYLine=0;
//20201029//            return;
//20201029//            }
         #ifdef DEBUG                         
         if (!(EVENT_WRITE_NUL)) DEBUG_LOG(0,"State:2 - waiting for 55, last PA=%02x, last_a_accs:%d  - BSY is now %d",P->VIA_PA, P->last_a_accs,P->BSYLine);
         #endif

         P->BSYLine=1;  if (EVENT_WRITE_NUL) return;

     //    if ( EVENT_WRITE_ORA)//P->last_a_accs)// now wait for 0x55 ACK from Lisa, else, go back to idle
                //{
                   if (P->VIA_PA==0x00 && EVENT_WRITE_ORA)  // 2021.09.14 for los1
                                               {
                                                 DEBUG_LOG(0,"VIA:%d Got %00x, will go back to idle now. last_a_accs=%d",
                                                              P->vianum, P->last_a_accs);
                                                 P->StateMachineStep=IDLE_STATE;
                                                 return;
                                               }


                    if (P->VIA_PA!=0x55 && P->VIA_PA!=0x01&& !EVENT_WRITE_ORA) {
                                               DEBUG_LOG(0,"VIA_PA=%02x - sending 01.",P->VIA_PA);    
                                               P->VIA_PA=0x01;
                                               return;
                                               }

                    if (P->VIA_PA==0x55 && !P->CMDLine)
                                               {
                                                PRO_STATUS_GOT55;
                                                P->StateMachineStep=GET_CMDBLK_STATE; //DELAY_BEFORE_CMD_STATE; //GET_CMDBLK_STATE;
                                                SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);

                                                DEBUG_LOG(0,"State:transition to Step:4 - got 0x55");
                                                P->indexread=4;         // start at offset 4
                                                P->indexwrite=4;        // (4 byte preable reserved for status for lisa to read later
                                                return;
                                               }

                   // else
                   if (P->VIA_PA!=0x55 && P->VIA_PA!=0x01 && EVENT_WRITE_ORA)
                                               {
                                                 DEBUG_LOG(0,"VIA:%d did not get 55, got %02x, will go back to idle now. last_a_accs=%d",
                                                              P->vianum,P->VIA_PA,P->last_a_accs);

                                                 P->StateMachineStep=IDLE_STATE;
                                                 return;
                                               }
                //}
//         if (P->VIA_PA!=0x55) P->VIA_PA=0x01;                // resend ACK if we got here since we didn't get 0x55.

         return;


case GET_CMDBLK_STATE:           // 4          // now copy command bytes into command buffer
         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"State:4 - waiting for command, event:%d",event);
         #endif
		
         // uniplus loader hax - weirdly this expects BSY to be down before it sends CMD!
         if ( running_lisa_os==0 && reg68k_pc==0x00062a38 && context==1 && P->indexwrite==10 && P->indexread==4) {
             DEBUG_LOG(0,"UniPlus Loader hax, faking BSYLine=0");
             SET_PROFILE_LOOP_NO_PREDELAY(TENTH_OF_A_SECOND); 
             P->BSYLine=0; return;
         }

         CHECK_PROFILE_LOOP_TIMEOUT;
         
         if ( (running_lisa_os==LISA_UNIPLUS_RUNNING || running_lisa_os == LISA_UNIPLUS_SUNIX_RUNNING || running_lisa_os == LISA_XENIX_RUNNING) && 
              (TIMEPASSED_PROFILE_LOOP( (HUN_THOUSANDTH_OF_A_SEC/1000)) ) ) {
              DEBUG_LOG(0,"UniPlus OS, faking BSYLine=1");
              SET_PROFILE_LOOP_NO_PREDELAY(TENTH_OF_A_SECOND); 
              via[P->vianum].via[IFR] |=VIA_IRQ_BIT_CA1; // force IFR BSY/CA1 bit on //2021.06.13
              P->BSYLine=1; return;
         }
         else 
            if (P->BSYLine!=2) {         // wait a bit before flopping busy, but if Lisa sends a byte, accept it
               if ( TIMEPASSED_PROFILE_LOOP( (HUN_THOUSANDTH_OF_A_SEC/100) )  && !(EVENT_WRITE_ORA) ) //was 1/100th HUN_THOUSANDTH_OF_A_SEC 49152, 2021.03.23 add /100
                  {
                      DEBUG_LOG(0,"State:4 - wasting 1/10,000,000th of a sec, BSYLine=0");
                      P->BSYLine=0;
                      return;
                  }
      
                  P->BSYLine=1; DEBUG_LOG(0,"State:4 BSYLine=1 signaled - IRQ should fire now.");
               }
         if (EVENT_WRITE_NUL) return;

         // step 4a
         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))   DEBUG_LOG(0,"State4a: Waiting for event=2:%02x RRWline=1:%02x and !P->CMDLine:%02x",
                                   event,P->RRWLine,P->CMDLine);
         #endif


         if (EVENT_WRITE_ORA && !P->CMDLine)
                         {

                          SET_PROFILE_LOOP_NO_PREDELAY(TENTH_OF_A_SECOND);   // reset timeout when we get a byte

                           // avoid a 2nd write as 0x55
                          if (P->VIA_PA==0x55 && P->indexwrite==4)
                           {
                             DEBUG_LOG(0,"Ignoring 2nd 0x55 write to avoid sync issues");
                             return;
                           }

                           P->DataBlock[P->indexwrite++]=P->VIA_PA;

                           switch(P->indexwrite-1)
                           {
                            case 4 : DEBUG_LOG(0,"Wrote CMD  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                            case 5 : DEBUG_LOG(0,"Wrote MSB  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                            case 6 : DEBUG_LOG(0,"Wrote mid  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                            case 7 : DEBUG_LOG(0,"Wrote LSB  %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                            case 8 : DEBUG_LOG(0,"Wrote RTRY %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;
                            case 9 : DEBUG_LOG(0,"Wrote SPAR %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1); break;

                            default: DEBUG_LOG(0,"Wrote ???? %02x into ProFile Data block index:%d",P->VIA_PA,P->indexwrite-1);
                           }

                           P->blocktowrite=(P->DataBlock[5] << 16) | (P->DataBlock[6] << 8) | (P->DataBlock[7]);

                           if (P->indexwrite>542) {DEBUG_LOG(0,"ProFile buffer overrun!"); P->indexwrite=4;} // prevent overrun ?
                           return;
                         }

         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"State4b: Waiting for CMDLine");
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
                              case 0 : P->VIA_PA=0x02; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"4b: ACK READ");          // read block
                                       P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                       SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                       return;


                              case 1 : P->VIA_PA=0x03; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"4b: ACK WRITE");         // write block
                                       P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                       SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                       return;

                              case 2 : P->VIA_PA=0x04; P->last_a_accs=0; P->BSYLine=1; DEBUG_LOG(0,"4b: ACK WRITE/VERIFY");  // write/verify block
                                       P->StateMachineStep=WAIT_2nd_0x55_STATE;
                                       SET_PROFILE_LOOP_NO_PREDELAY(HALF_OF_A_SECOND);
                                       return;

                              default: P->VIA_PA=0x00; P->BSYLine=0;
                                       P->StateMachineStep=IDLE_STATE;

                                       DEBUG_LOG(0,"S4B. Returning %02x as response to Lisa's command %02x and going to step %d",
                                                    P->VIA_PA,
                                                    P->DataBlock[4],
                                                    P->StateMachineStep);
                          }
                         }

         return;

    case WAIT_2nd_0x55_STATE:       // 5

         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))               DEBUG_LOG(0,"State:5");
         #endif

         CHECK_PROFILE_LOOP_TIMEOUT;

         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"State5: Waiting for EVENT_WRITE_ORA:%02x && P->last_a_accs:%02x && P->CMDLine>0:%02x && RRWLine>0:%02x && PortA=0x55:%02x",
                event,
                P->last_a_accs,
                P->CMDLine,
                P->RRWLine,
                P->VIA_PA);
         #endif

         P->BSYLine=1; //2006.05.09 was 0
         if (EVENT_WRITE_NUL) return;
         if (EVENT_WRITE_ORA) //  && P->last_a_accs && P->CMDLine && P->RRWLine)
            {      DEBUG_LOG(0,"State:5: checking what we got:%02x==0x55",P->VIA_PA);
                  if (P->VIA_PA==0x55)   {
                                          P->StateMachineStep=PARSE_CMD_STATE;
                                          SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);
                               
                                          DEBUG_LOG(0,"State5: got 0x55 w00t!");
                                         }
                   else
                                         {
                                          P->StateMachineStep=0; // possibly our old code
                                          PRO_STATUS_NO55;
                                          DEBUG_LOG(0,"State5: going into state 0 now. oh well.");
                                         }
            }

         return;

    case PARSE_CMD_STATE:   //6 // now we execute the command after simulating a busy profile
         // insert sound play some profile seeking sounds now?
         //CHECK_PROFILE_LOOP_TIMEOUT;

         if ( !TIMEPASSED_PROFILE_LOOP(HUN_THOUSANDTH_OF_A_SEC) ) //2021.08.24 - disabling this: && P->DataBlock[4]==0)  // this block was disabled 2021.06.15 added && P->DataBlock[4]
              {
                DEBUG_LOG(0,"State:6 - wasting cycles for a bit to simulate a busy profile (%d cycles)",PROFILE_WAIT_EXEC_CYCLE);
                return;
              }

         PRO_STATUS_CLEAR;

         DEBUG_LOG(0,"State:6 Done wasting cycles in step 6");

         P->indexread=0;

         blocknumber=(P->DataBlock[5]<<16) | (P->DataBlock[6]<< 8) |  (P->DataBlock[7]    ) ;

         P->BSYLine=1; //20060425-moved from step 5, and re-enabled above delay to slow down
         if (EVENT_WRITE_NUL) { DEBUG_LOG(0,"EVENT_WRITE_NULL - skipping"); return;}

         switch (P->DataBlock[4])                                                       // Now execute the command
         {

             case 0 :
                      #ifdef DEBUG
                                           //   0     1    2    3    4     5   6    7     8     9   10   11   12   13   14  15
                      DEBUG_LOG(0,"step6: Reading block#%d,0x%06x - buffer: %02x.%02x.%02x.%02x(%02x )[%02x %02x %02x]:%02x:%02x %02x %02x %02x %02x %02x %02x",
                        blocknumber, blocknumber,
                        P->DataBlock[ 0],
                        P->DataBlock[ 1],
                        P->DataBlock[ 2],
                        P->DataBlock[ 3],
                        P->DataBlock[ 4],
                        P->DataBlock[ 5],
                        P->DataBlock[ 6],
                        P->DataBlock[ 7],
                        P->DataBlock[ 8],
                        P->DataBlock[ 9],
                        P->DataBlock[10],
                        P->DataBlock[11],
                        P->DataBlock[12],
                        P->DataBlock[13],
                        P->DataBlock[14],
                        P->DataBlock[15]);
                     #endif

                      do_profile_read(P,blocknumber);
                      P->StateMachineStep=SEND_DATA_AND_TAGS_STATE;
                      SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);
                      break;
                      //no, this is bad//PRO_STATUS_GOT55;  // force status[0] to return the got 0x55 status.

             case 1 :
             case 2 :

                      P->StateMachineStep=ACCEPT_DATA_FOR_WRITE_STATE;
                      P->indexwrite=10;  // bytes 0-3 are status, 4-6 are cmd block, 10-522 are byte, 523-542 are tags.
                      SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND*3);
                      #ifdef DEBUG

                      DEBUG_LOG(0,"Step6: (cmd:%02x) Fixin to write to block#%d,0x%06x - buffer: %02x.%02x.%02x.%02x(%02x )[%02x %02x %02x]:%02x:%02x %02x %02x %02x %02x %02x %02x",
                        P->DataBlock[4],
                        blocknumber, blocknumber,
                        P->DataBlock[ 0],
                        P->DataBlock[ 1],
                        P->DataBlock[ 2],
                        P->DataBlock[ 3],
                        P->DataBlock[ 4],
                        P->DataBlock[ 5],
                        P->DataBlock[ 6],
                        P->DataBlock[ 7],
                        P->DataBlock[ 8],
                        P->DataBlock[ 9],
                        P->DataBlock[10],
                        P->DataBlock[11],
                        P->DataBlock[12],
                        P->DataBlock[13],
                        P->DataBlock[14],
                        P->DataBlock[15]);
                     #endif

                     break;  // write/verify block

             default:  
                       DEBUG_LOG(0,"Leaving state 6 for 0, because unknown command:%d",P->DataBlock[4])
                       P->StateMachineStep=IDLE_STATE;
         }
         return;


    case ACCEPT_DATA_FOR_WRITE_STATE:    // 7    // handle write/write+verify - read bytes from lisa into buffer
         CHECK_PROFILE_LOOP_TIMEOUT;

         P->BSYLine=0;

         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"State:7 - reading bytes from ProFile for write sector event:%02x bsy:%02x cmd:%02x rrw:%02x PA:%02x @ index%d",
           event, P->BSYLine,   P->CMDLine,  P->RRWLine, P->VIA_PA, P->indexwrite );
         #endif

         if (EVENT_WRITE_NUL) return; //2006.05.19

         if (EVENT_WRITE_ORA && P->RRWLine && !P->CMDLine)
         {
            
            P->DataBlock[P->indexwrite++]=P->VIA_PA;
            if (P->indexwrite>552) {PRO_STATUS_BUFFER_OVERFLOW; P->StateMachineStep=0; DEBUG_LOG(0,"State:7 write buffer Overflow");}
            else SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);

         }
         else
            if (P->CMDLine)
            {
                P->BSYLine=1;
                P->VIA_PA=0x06;
                DEBUG_LOG(0,"State7 ACK WRITE command with 06");

                P->StateMachineStep=WAIT_3rd_0x55_STATE;
                SET_PROFILE_LOOP_TIMEOUT(FIFTH_OF_A_SECOND);
            }
            else
                {
                    DEBUG_LOG(0,"State:7 did not recognize byte event:%02x %s bsy:%02x cmd:%02x rrw:%02x",
                      event, profile_event_names[event],
                      P->BSYLine,
                      P->CMDLine,
                      P->RRWLine);
                }
         return;


case WAIT_3rd_0x55_STATE:              // 8    // wait for 0x55 again

         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))

         DEBUG_LOG(0,"State:8 - wait for 3rd 0x55 - write:bsy:%d, PA=%02x CMD:%d",P->BSYLine,P->VIA_PA,P->CMDLine);
         #endif

         CHECK_PROFILE_LOOP_TIMEOUT;
         P->BSYLine=1; //no this should always be 1 - do not change it!
         if (EVENT_WRITE_NUL) return;

         if (EVENT_WRITE_ORA)
            {if (P->VIA_PA==0x55)
                {
                 P->StateMachineStep=WRITE_BLOCK_STATE; // accept command
                 SET_PROFILE_LOOP_TIMEOUT(HUN_THOUSANDTH_OF_A_SEC*5);
                 P->indexread=0;
                 PRO_STATUS_GOT55;
                 DEBUG_LOG(0,"Command accepted, transition to Step:9");
                }
             else
                 {P->StateMachineStep=0;
                  PRO_STATUS_NO55;}
            }
         return;


    case WRITE_BLOCK_STATE:                    // 8  // do the write and waste some time
         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"State:9 - write and waste more time (%d)",PROFILE_WAIT_EXEC_CYCLE);
         #endif
         if ( !TIMEPASSED_PROFILE_LOOP(HUN_THOUSANDTH_OF_A_SEC*5) ) return;

         DEBUG_LOG(0,"State:9b - time's done");

         blocknumber=(P->DataBlock[5]<<16) |
                     (P->DataBlock[6]<< 8) |
                     (P->DataBlock[7]    ) ;

         #ifdef DEBUG
                                                      //   0    1   2     3    4      5    6   7      8    9   10   11   12   13   14  15
          DEBUG_LOG(0,"Writing block#%d,0x%06x - buffer: %02x.%02x.%02x.%02x(%02x )[%02x %02x %02x]:%02x:%02x|%02x %02x %02x %02x %02x %02x",
            blocknumber, blocknumber,
            P->DataBlock[ 0],
            P->DataBlock[ 1],
            P->DataBlock[ 2],
            P->DataBlock[ 3],
            P->DataBlock[ 4],
            P->DataBlock[ 5],
            P->DataBlock[ 6],
            P->DataBlock[ 7],
            P->DataBlock[ 8],
            P->DataBlock[ 9],
            P->DataBlock[10],
            P->DataBlock[11],
            P->DataBlock[12],
            P->DataBlock[13],
            P->DataBlock[14],
            P->DataBlock[15]);

          //                   0    1    2    3   4   5    6    7    8    9     10   11   12   13  14   15   16   17   18   19
          DEBUG_LOG(0,"Tags: %02x %02x %02x %02x[%02x %02x]%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
            P->DataBlock[522+ 0],
            P->DataBlock[522+ 1],
            P->DataBlock[522+ 2],
            P->DataBlock[522+ 3],
            P->DataBlock[522+ 4],
            P->DataBlock[522+ 5],
            P->DataBlock[522+ 6],
            P->DataBlock[522+ 7],
            P->DataBlock[522+ 0],
            P->DataBlock[522+ 9],
            P->DataBlock[522+ 10],
            P->DataBlock[522+ 11],
            P->DataBlock[522+ 12],
            P->DataBlock[522+ 13],
            P->DataBlock[522+ 14],
            P->DataBlock[522+ 15],
            P->DataBlock[522+ 16],
            P->DataBlock[522+ 17],
            P->DataBlock[522+ 18],
            P->DataBlock[522+ 19]
            );

         #endif

         do_profile_write(P,blocknumber);

         P->indexwrite=4;
         P->indexread=0;
         P->DataBlock[0]=0;
         P->DataBlock[1]=0;
         P->DataBlock[2]=0;
         P->DataBlock[3]=0;

         P->BSYLine=0;  //2006.05.17 was 1
         if (running_lisa_os == LISA_UNIPLUS_SUNIX_RUNNING || running_lisa_os == LISA_XENIX_RUNNING) 
             via[P->vianum].via[IFR] |=VIA_IRQ_BIT_CA1; // 2021.06.06 - force IFR BSY/CA1 bit on

         P->StateMachineStep=SEND_STATUS_BYTES_STATE;
         SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);

         return;


    case SEND_DATA_AND_TAGS_STATE:     //10                       // Let Lisa read the status/data
         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
         DEBUG_LOG(0,"State:10, allow Lisa to read the status and data  - pointer:%d",P->indexread);
         #endif

         P->BSYLine=0;

         CHECK_PROFILE_LOOP_TIMEOUT;
         if (EVENT_WRITE_NUL) return;

         if (!P->CMDLine)
         {

            P->BSYLine=0;            //2006.05.15
            if (EVENT_READ_IRA)
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
             DEBUG_LOG(0,"Returning %02x from index:%d",P->VIA_PA,P->indexread-1);
            }
         }
         else
         {
             #ifdef DEBUG
             if (P->indexread != 536)
             {
                 blocknumber=(P->DataBlock[5]<<16) | (P->DataBlock[6]<< 8) |    (P->DataBlock[7]    );
                 DEBUG_LOG(0,"Warning: read %d bytes instead of 536 for block read of sector #%08x (%x)",P->indexread,blocknumber,blocknumber);
             }
             #endif

             if (EVENT_READ_IRA)             // oops we fell out of sync - recover please!
             {
               P->VIA_PA=1;   P->last_a_accs=0;
               P->StateMachineStep=WAIT_1st_0x55_STATE;
               SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);
               P->BSYLine=1;           // flip BSY
               return;
             }

             P->BSYLine=1;
             P->StateMachineStep=FINAL_FLIP_TO_IDLE_STATE; SET_PROFILE_LOOP_TIMEOUT(TEN_THOUSANDTH_OF_A_SEC);
         }
         if (P->indexread==536) {DEBUG_LOG(0,"Returning to idle state"); P->StateMachineStep=IDLE_STATE; P->BSYLine=1; P->indexread=4; P->indexwrite=0;} //2021.09.14


         return;



    case FINAL_FLIP_TO_IDLE_STATE:        // 11

         // let ProFile see BSY strobe for a short period, this speeds up LisaTest immensely.  returns to state 0 via timeout
         CHECK_PROFILE_LOOP_TIMEOUT;

         if (EVENT_READ_IRA)                 // oops we fell out of sync - recover please!
         {
           P->VIA_PA=1;   P->last_a_accs=0;
           P->StateMachineStep=WAIT_1st_0x55_STATE;
           SET_PROFILE_LOOP_TIMEOUT(TENTH_OF_A_SECOND);
           P->BSYLine=1;           // flip BSY
           return;
         }

         P->BSYLine=1;
         return;


    case SEND_STATUS_BYTES_STATE:              //12              // Let Lisa read the status/data

         #ifdef DEBUG                          // don't fill up the log with useless shit
         if (!(EVENT_WRITE_NUL))
          DEBUG_LOG(0,"State:12, post write - allow Lisa to read the status and data  - pointer:%d",P->indexread);
         #endif

         CHECK_PROFILE_LOOP_TIMEOUT;

         if (running_lisa_os==LISA_UNIPLUS_RUNNING || running_lisa_os == LISA_UNIPLUS_SUNIX_RUNNING || running_lisa_os == LISA_XENIX_RUNNING) 
             via[P->vianum].via[IFR] |=VIA_IRQ_BIT_CA1; // 2021.06.06 - force IFR BSY/CA1 bit on
         P->BSYLine=0;

         if (EVENT_WRITE_NUL || EVENT_READ_IRB) return;

         if (!P->CMDLine)
         {
            if (EVENT_READ_IRA)
            {
             P->VIA_PA=P->DataBlock[P->indexread++]; if (P->indexread>3) P->indexread=0;
             P->last_a_accs=0;
             SET_PROFILE_LOOP_TIMEOUT(HALF_OF_A_SECOND);   // reset timeout  // was FIFTH_OF_A_SECOND
             DEBUG_LOG(0,"Returning %02x from index:%d",P->VIA_PA,P->indexread-1);
            }
         }
         else
         {
             DEBUG_LOG(0,"Going back to idle since Lisa set CMDLine");
             P->StateMachineStep=IDLE_STATE;
         }


         return;

    default:
           DEBUG_LOG(0,"Unknown ProFile state %d, returning to idle.",P->StateMachineStep);
           P->StateMachineStep=IDLE_STATE;
           return;
    }

}


// I gotta get home, dirty,
// I haveta code.
// we gotta code all day all night
// You know it sounds right         - mc++
