/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      RC2 2020.06.21                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2021 Ray A. Arachelian                          *
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
*                           Floppy Disk Driver Routines                                *
*                                                                                      *
\**************************************************************************************/

#define IN_FLOPPY_C
#include <vars.h>


void floppy_return(DC42ImageType *F, uint8 boot, uint8 status);

#ifdef DEBUG
 static int16 turn_logging_on_sector=-1, turn_logging_on_write=-1;
#endif

//slowdown floppy access - was 7f
#define SLOWDOWN 0x03

// Floppy Controller Commands

#define FLOP_CONTROLLER 0xFCC001
#define FLOP_CTRLR_SHAKE 0x80 // Handshake
#define FLOP_CTRLR_RWTS  0x81 // execute the RWTS routine
#define FLOP_CTRLR_SEEK  0x83 // seek to side/track
#define FLOP_CTRLR_JSR   0x84 // JSR to routine in C003-5
#define FLOP_CTRLR_CLIS  0x85 // Clear interrupt status
#define FLOP_CTRLR_STIM  0x86 // Set interrupt mask
#define FLOP_CTRLR_CLIM  0x87 // Clear interrupt mask
#define FLOP_CTRLR_WAIT  0x88 // Wait for 69 96
#define FLOP_CTRLR_LOOP  0x89 // Loop until reset in ROM

#define FLOP_COMMAND 0xFCC003 // command code
#define FLOP_CMD_READ    0x00  // Read
#define FLOP_CMD_WRITE   0x01  // Write
#define FLOP_CMD_UCLAMP  0x02  // Unclamp - aka eject
#define FLOP_CMD_FORMAT  0x03  // Format
#define FLOP_CMD_VERIFY  0x04  // Verify
#define FLOP_CMD_FMTTRK  0x05  // Format Track
#define FLOP_CMD_VFYTRK  0x06  // Verify Track
#define FLOP_CMD_READX   0x07  // Read without checksum vfy
#define FLOP_CMD_WRITX   0x08  // Write without checksum
#define FLOP_CMD_CLAMP   0x09  // Clamp

#define FLOPPY_MOTOR   0x21
#define MOTOR_SPIN(x) {floppy_ram[FLOPPY_MOTOR]=0xff;}
#define MOTOR_STOP(x) {floppy_ram[FLOPPY_MOTOR]=0x00;}
#define CURRENT_TRACK 0x22
#define CURRENT_TRACK_GRP 0x23
#define DRIVE_EXISTS 0x23


#define FLOP_SELECT 0xFCC005
#define FLOP_SEL_LOWER   0x00
#define FLOP_SEL_UPPER   0x80
#define FLOP_SEL_DRV2    0x00
#define FLOP_SEL_DRV1    0x80

#define FLOP_SIDE_SELECT 0xFCC007
#define FLOP_SIDE_1       0x00
#define FLOP_SIDE_2       0x01
#define FLOP_SIDE_UPPER   0x00
#define FLOP_SIDE_LOWER   0x01

#define FLOP_SECTOR_NUMBER  0xFCC009  // 0-22
#define FLOP_TRACK_NUMBER   0xFCC00B  // 0-44
#define FLOP_SPEED_BYTE     0xFCC00D
#define FLOP_FORMAT_CONFIRM 0xFCC00F // Format confirm byte
#define FLOP_ERROR_STATUS   0xFCC011
#define FLOP_DISK_ID_VALUE  0xFCC013

// Floppies generalte interrupts to the CPU whenever a disk is inserted,
// eject button is pressed, whenever an 81 command is completed.

// these are wrong 5F is interrupt status!!
// INT_MASK was off!  0x2d is wrong, but 2c is off!
#define FLOP_INT_MASK 0x2d
#define FLOP_INT_STAT 0x2f
#define FLOP_INT_STATUS 0xFCC05F

#define FLOP_STAT_INVCMD   0x01  // invalid command
#define FLOP_STAT_INVDRV   0x02  // invalid drive
#define FLOP_STAT_INVSEC   0x03  // invalid sector
#define FLOP_STAT_INVSID   0x04  // invalid side
#define FLOP_STAT_INVTRK   0x05  // invalid track
#define FLOP_STAT_INVCLM   0x06  // invalid clear mask
#define FLOP_STAT_NODISK   0x07  // no disk
#define FLOP_STAT_DRVNOT   0x08  // drive not enabled


#define FLOP_STAT_IRQPND   0x09  // Interrupts pending

#define FLOP_STAT_INVFMT   0x0a  // Invalid format configuration
#define FLOP_STAT_BADROM   0x0b  // ROM Selftest failure
#define FLOP_STAT_BADIRQ   0x0c  // Unexpected IRQ or NMI
#define FLOP_STAT_WRPROT   0x14  // Write protect error
#define FLOP_STAT_BADVFY   0x15  // Unable to verify
#define FLOP_STAT_NOCLMP   0x16  // Unable to clamp disk
#define FLOP_STAT_NOREAD   0x17  // Unable to read
#define FLOP_STAT_NOWRIT   0x18  // Unable to write
#define FLOP_STAT_NOWRITE  0x18  // Unable to write
#define FLOP_STAT_NOUCLP   0x19  // Unable to unclamp/eject
#define FLOP_STAT_NOCALB   0x1A  // Unable to find calibration
#define FLOP_STAT_NOSPED   0x1B  // Unable to adjust speed
#define FLOP_STAT_NWCALB   0x1c  // Unable to write calibration

#define FLOP_CPU_RAM           0xFCC181
#define FLOP_CPU_RAM_END       0xFCC1FF

#define FLOP_XFER_RAM          0xFCC501
#define FLOP_XFER_RAM_END      0xFCC7FF

#define FLOP_IRQ_SRC_DRV2   128      // set if 4, 5, or 6 set.
#define FLOP_IRQ_SRC_RWTS2   64      // set if drive 2 RWTS complete for drive 2
#define FLOP_IRQ_SRC_BTN2    32      // set if button on disk 2 pressed
#define FLOP_IRQ_SRC_DSKIN2  16      // set if disk in place in drive 2
#define FLOP_IRQ_SRC_DRV1     8      // set if bits 0,1 or 2 are set
#define FLOP_IRQ_SRC_RWTS1    4      // set if drive 1 RWTS complete for drive 1
#define FLOP_IRQ_SRC_BTN1     2      // set if button on disk 1 pressed
#define FLOP_IRQ_SRC_DSKIN1   1      // set if disk inserted in drive 1



#define NOFLOPPY -1
#define GENERICIMAGE   3
#define SONY800KFLOPPY 2
#define SONY400KFLOPPY 1
#define TWIGGYFLOPPY   0
#define FLOPPY_ROM_VERSION 0xa8 //want a8 here normally


#define GOBYTE        (0) //gobyte     1
#define COMMAND       (0) //gobyte     1              // synonym
#define FUNCTION      (1) //function   3
#define DRIVE         (2) //drive      5              // 00=lower, 80=upper
#define SIDE          (3) //side       7
#define SECTOR        (4) //sector     9
#define TRACK         (5) //track      b
#define SPEED         (6)                             // rotation speed 0=normal, DA is fast
#define CONFIRM       (7)                             // format confirm
#define STATUS        (8)
#define INTERLEAVE    (9)                             // sector interleave
#define TYPE          (0xA)                           // drive type id 0-twig, 1-sony, 2-double sony
     // $FCC015 - DRVTYPE...
     #define  TWIGGY_TYPE 0
     #define SONY400_TYPE 1
     #define SONY800_TYPE 2

#define STST          (0xB)                           // rom controller self test status
#define ROMVER        (0x18)                          // ROM Version
// above are correct, not sure about below


#define DISKID        9                               // synonym
#define INTSTAT       (0x5f>>1)                       // interrupt status == 2f
#define INTSTATUS     (0x5f>>1)                       // interrupt status ==2f
// fcc05d=pending IRQ's?


//*** THIS IS WRONG! 2c is retry count, not drive enabled - changing it to 0x2d which is also wrong, but prevents conflicts
#define DRIVE_ENABLED 0x2d


#define LISATYPE   0x0018
#define LISATYPE_LISA1  0
#define LISATYPE_LISA2  (32|128)                      // Lisa 2 with slow timers
#define LISATYPE_LISA2F (64|128)                      // Lisa 2 with fast timers (or pepsi)
#define LISATYPE_LISA2PEPSI (128)




#define DISKDATAHDR   (0x1f4)                         // disk buffer header
#define DISKDATASEC   ((DISKDATAHDR)+12)              // disk buffer data  // check this!!!

// other status and error vars
#define  FLOPPY_dat_bitslip1                  0x005b
#define  FLOPPY_dat_bitslip2                  0x005c
#define  FLOPPY_dat_chksum                    0x005d
#define  FLOPPY_adr_bitslip1                  0x005e
#define  FLOPPY_adr_bitslip2                  0x005f
#define  FLOPPY_wrong_sec                     0x0060
#define  FLOPPY_wrong_trk                     0x0061
#define  FLOPPY_adr_chksum                    0x0062
#define  FLOPPY_usr_cksum1                    0x007e
#define  FLOPPY_usr_cksum2                    0x007f
#define  FLOPPY_usr_cksum3                    0x0080
#define  FLOPPY_bad_sec_total                 0x01d0
#define  FLOPPY_err_track_num                 0x01d1
#define  FLOPPY_err_side_num                  0x01d2
#define  FLOPPY_bad_sect_map                  0x01d3

#define  FLOP_PENDING_IRQ_FLAG  0x2f

// Controller macro commands

#define FLOP_CTRLR_RWTS  0x81 // execute the RWTS routine
#define FLOP_CTRLR_SEEK  0x83 // seek to side/track
#define FLOP_CTRLR_JSR   0x84 // JSR to routine in C003-5
#define FLOP_CTRLR_CLIS  0x85 // Clear interrupt status
#define FLOP_CTRLR_STIM  0x86 // Set interrupt mask
#define FLOP_CTRLR_CLIM  0x87 // Clear interrupt mask
#define FLOP_CTRLR_WAIT  0x88 // Wait in ROM until cold start
#define FLOP_CTRLR_LOOP  0x89 // Loop in ROM

// RWTS subcommands
#define FLOP_CMD_READ   0x00  // Read
#define FLOP_CMD_WRITE  0x01  // Write
#define FLOP_CMD_UCLAMP 0x02  // Unclamp/eject
#define FLOP_CMD_FORMAT 0x03  // Format
#define FLOP_CMD_VERIFY 0x04  // Verify
#define FLOP_CMD_FMTTRK 0x05  // Format Track
#define FLOP_CMD_VFYTRK 0x06  // Verify Track
#define FLOP_CMD_READX  0x07  // Read without checksum vfy
#define FLOP_CMD_WRITX  0x08  // Write without checksum
#define FLOP_CMD_CLAMP  0x09  // Clamp
#define FLOP_CMD_OKAY   0xFF  // Okay byte for format

//char DiskCopy42Sig[]={0x16,'-','n','o','t',' ','a',' ','M','a','c','i','n','t','o','s','h',' ',
//        'd','i','s','k','-'};


/**********************************************************************************\
*  Do a 6504 command.  This function reads the command and function                *
*  codes in the 6504 memory range and then goes and processes the command.         *
*                                                                                  *
\**********************************************************************************/

// Set IRQ on RWTS completion, won't fire IRQ if the mask isn't right, that's handled in fix_intstat.
#define RWTS_IRQ_SIGNAL(x) {\
                            floppy_ram[FLOP_INT_STAT]|=((floppy_ram[DRIVE] & 0x88)>>1); \
                            fix_intstat(1); floppy_return(F,0,(x)); }

/* above is quicker version of this: floppy_ram[INTSTAT]|=((floppy_ram[DRIVE] & 0x80) ? 0x40:0);  \
                                  // floppy_ram[INTSTAT]|=((floppy_ram[DRIVE] & 0x08) ? 0x04:0);
*/


DC42ImageType Floppy_u,                 // Lisa1 had two twiggy floppies, Lisa2 has only 1
              Floppy_l;

        static uint8 queuedfn=0xff;
        DC42ImageType *FQ=NULL;


long getsectornum(DC42ImageType *F, uint8 side, uint8 track, uint8 sec);

#ifdef DEBUG
static char lastfloppyrwts[1024];
static long queuedsectornumber=-1;
#endif



// don't move this to vars file, must be local to this!
static uint8 floppy_last_macro;
// include the sector translation tables.

static void do_floppy_read(DC42ImageType *F);
static void do_floppy_write(DC42ImageType *F);


void fix_intstat(int RWTS);


char templine[1024];

void flop_cmd_text(FILE *buglog)
{

  return;                               // shut it off for now -- too noisy

  if ( !floppy_ram[GOBYTE]) return;     // gobyte=0 means Floppy Controller accepted command.  nothing to see here, move along citizen
  fprintf(buglog,"SRC: ");

  floppy_ram[0x30]=floppy_ram[0x00];
  floppy_ram[0x31]=floppy_ram[0x01];
  floppy_ram[0x32]=floppy_ram[0x02];
  floppy_ram[0x33]=floppy_ram[0x03];
  floppy_ram[0x34]=floppy_ram[0x04];
  floppy_ram[0x35]=floppy_ram[0x05];
  floppy_ram[0x36]=floppy_ram[0x06];
  floppy_ram[0x37]=floppy_ram[0x07];

  switch ( floppy_ram[GOBYTE])
  {
    case FLOP_CTRLR_SHAKE:  fprintf(buglog,"floppy cmd handshake\n"); break;

    case FLOP_CTRLR_SEEK:   fprintf(buglog,"floppy cmd seek\n"); break;

    case FLOP_CTRLR_JSR:    fprintf(buglog,"floppy cmd jsr\n"); break;

    case FLOP_CTRLR_CLIS:   fprintf(buglog,"floppy cmd clear IRQ status\n");   break;

    case FLOP_CTRLR_STIM:   fprintf(buglog,"floppy cmd set IRQ mask\n");            break;

    case FLOP_CTRLR_CLIM:   fprintf(buglog,"floppy cmd clear IRQ mask\n");  break;

    case FLOP_CTRLR_WAIT:   fprintf(buglog,"floppy cmd wait\n"); break;

    case FLOP_CTRLR_LOOP:   fprintf(buglog,"floppy cmd die\n"); break;

    case FLOP_CTRLR_RWTS:  switch (floppy_ram[FUNCTION])
    {     case FLOP_CMD_READ:    fprintf(buglog,"floppy cmd RWTS: read sector\n"); break;
          case FLOP_CMD_WRITE:   fprintf(buglog,"floppy cmd RWTS: write sector\n"); break;
          case FLOP_CMD_UCLAMP:  fprintf(buglog,"floppy cmd RWTS: unclamp/eject\n"); break;
          case FLOP_CMD_FORMAT:  fprintf(buglog,"floppy cmd RWTS: format floppy\n"); break;
          case FLOP_CMD_VERIFY:  fprintf(buglog,"floppy cmd RWTS: verify\n"); break;
          case FLOP_CMD_FMTTRK:  fprintf(buglog,"floppy cmd RWTS: format track\n"); break;
          case FLOP_CMD_VFYTRK:  fprintf(buglog,"floppy cmd RWTS: verify track\n"); break;
          case FLOP_CMD_READX:   fprintf(buglog,"floppy cmd RWTS: read sector ignoring errors\n"); break;
          case FLOP_CMD_WRITX:   fprintf(buglog,"floppy cmd RWTS: write sector with errors\n"); break;
          case FLOP_CMD_CLAMP:   fprintf(buglog,"floppy cmd RWTS: clamp\n"); break;
          default:               fprintf(buglog,"floppy cmd RWTS: unknown command:%02x\n",floppy_ram[FUNCTION]); break;
    }
    case 0 : break;
    default:  fprintf(buglog,"floppy unrecognized go-byte:%02x\n",floppy_ram[GOBYTE]);
  }
}

#ifdef DEBUGXXX
void append_floppy_log(char *s)
{
  FILE *f;
  //fprintf(buglog,s);
  f=fopen("./lisaem-output.sec","a");
  if (f) {
          printlisatime(f);
          flop_cmd_text(f);
          fprintf(f,"pc24:%08x %s #%3d tags:%02x%02x, %02x%02x, %02x%02x, %02x%02x, %02x%02x, %02x%02x\n",pc24,s,
            getsectornum(NULL, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]),
            floppy_ram[0x1F4+0],floppy_ram[0x1F4+1],floppy_ram[0x1F4+2],floppy_ram[0x1F4+3],floppy_ram[0x1F4+4], floppy_ram[0x1F4+5],
            floppy_ram[0x1F4+6],floppy_ram[0x1F4+7],floppy_ram[0x1F4+8],floppy_ram[0x1F4+9],floppy_ram[0x1F4+10],floppy_ram[0x1F4+11]);

          fflush(f); fclose(f);
         }
}
#else
    void append_floppy_log(char *s) {s=NULL;}
#endif



int getmaxsector(int type, int track)
{

    if (type==SONY400KFLOPPY || type==SONY800KFLOPPY)
    {
        if (track<0) return -1;
        if (track>-1 && track<16) return 12;  // tracks  0-15
        if (track>15 && track<32) return 11;  // tracks 16-31
        if (track>31 && track<48) return 10;  // tracks 32-47
        if (track>47 && track<64) return 9;   // tracks 48-63
        if (track>63 && track<80) return 8;   // tracks 64-79
        return -1;
    }

    if (type==TWIGGYFLOPPY)
    {
        if (track<0) return -1;
        if (track>-1 && track< 4) return 23;
        if (track> 3 && track<10) return 22;
        if (track> 9 && track<17) return 21;
        if (track>16 && track<23) return 20;
        if (track>22 && track<29) return 19;
        if (track>28 && track<33) return 18;
        if (track>32 && track<39) return 17;
        if (track>38 && track<46) return 16;
        return -2;
    }

    return -2;
}

/*
 * Given a track and sector number on a 400 kB 3.5" floppy, return the
 * equivalent sector offset from the start of a disc image, or -1 if it's
 * an invalid (track, sector) pair.
 * The Lisa's 400 kB format had 80 tracks, track 0 on the outside of the disc
 * and track 79 on the inside. The tracks were divided up into 4 zones
 * of 16 tracks each.
 * Tracks  0-15 had 12 sectors.
 * Tracks 16-31 had 11 sectors.
 * Tracks 32-47 had 10 sectors.
 * Tracks 48-63 had  9 sectors.
 * Tracks 64-79 had  8 sectors.
 */
int floppy_sony400(int t, int s)
{
    int z = t >> 4;
    int b[] = {0, 16, 48, 96, 160};
    if (s>= (12 - z)) return -1;
    return b[z] + t * (12-z) + s;
}

/*
 * Given a track, disc side, and sector number on an 800 kB 3.5" floppy,
 * return the equivalent sector offset from the start of the disc image.
 * The Lisa's double sided 800 kB format is identical to the single sided
 * 400 kB one, except for the fact that there are two sides. The tracks
 * are stored in the disc image in an interleaved format. So track n, side 0
 * is followed by track n, side 1, then track n+1 side 0,  track n+1, side 1
 * and so on.
 */
int floppy_sony800(int t, int d, int s)
{
    int z = t >> 4;
    int b[] = {0, 32, 96, 192, 320};
    if (s>= (12 - z)) return -1;
    return b[z] + (t * 2 + d) * (12-z) + s;
}

/*
 * Given a track, disc side, and sector number on an 868 kb 5.25" 'Twiggy'
 * floppy, return the equivalent sector offset from the start of the disc
 * image. The Lisa format has 46 consecutive sectors starting from 0 on the
 * outside to 45 on the inside. The tracks are divided up into 8 zones
 * containing a different number of sectors. The tracks for the two sides of
 * the disc are stored in a non-interleaved fashion in an image: side 1, track
 * 0 comes directly afer track 45, side 0.
 * Note this code has not been tested with real disc images and may be
 * incorrect, it is also at odds with the description in the 1981 Lisa
 * Hardware Manual.
 *
 *         This code                    Lisa Hardware Manual 1981
 * Tracks  0- 2 had 23 sectors         Tracks  0- 3 has 23 sectors
 * Tracks  3- 8 had 22 sectors         Tracks  4- 9 had 22 sectors
 * Tracks  9-15 had 21 sectors         Tracks 10-16 had 21 sectors
 * Tracks 16-21 had 20 sectors         Tracks 17-22 had 20 sectors
 * Tracks 22-27 had 19 sectors         Tracks 23-28 had 19 sectors
 * Tracks 28-31 had 18 sectors         Tracks 29-32 had 18 sectors
 * Tracks 32-37 had 17 sectors         Tracks 33-38 had 17 sectors
 * Tracks 38-43 had 16 sectors         Tracks 39-45 had 16 sectors
 */
int floppy_twiggy(int d, int t, int s)
{
    int o = 852 * d;
    if (t < 0 || d < 0 || s < 0) return -1;
    if (t<3 ) return (s<23)? o +       t * 23 + s:-1;
    if (t<9 ) return (s<22)? o +   3 + t * 22 + s:-1;
    if (t<16) return (s<21)? o +  12 + t * 21 + s:-1;
    if (t<22) return (s<20)? o +  28 + t * 20 + s:-1;
    if (t<28) return (s<19)? o +  50 + t * 19 + s:-1;
    if (t<32) return (s<18)? o +  78 + t * 18 + s:-1;
    if (t<38) return (s<17)? o + 110 + t * 17 + s:-1;
    if (t<44) return (s<16)? o + 148 + t * 16 + s:-1;
    return -1;
}

/**********************************************************************************\
* Get a disk image virtual sector number given a Lisa floppy side, track, sec      *
* number and disk type parameter.                                                  *
\**********************************************************************************/

long getsectornum(DC42ImageType *F, uint8 side, uint8 track, uint8 sec)
{

    if (!F) F=&Floppy_u;                // default


    switch(F->ftype)
    {
        case TWIGGYFLOPPY:    if ( track>46 || sec>23 || side>2 ) return -1;
                              return floppy_twiggy(side, track, sec);

        case SONY400KFLOPPY:  if (side>0 ) return -1;
                              return  floppy_sony400(track, sec);

        case SONY800KFLOPPY:  if ( track>79 || sec>11 || side>1 ) return -1;
                              return floppy_sony800(track, side, sec);

        // Fake SuperFloppy - if possible -- for future use.
        case 3 :              if (track>F->maxtrk || sec>F->maxsec || side>F->maxside ) return -1;
                              return side*(F->maxtrk*F->maxsec)+(track*F->maxsec)+sec;
    }
    return -1;
}

/*************************************************************************************\
*  Interrupt the 68000 if needed - and if the interrupts are enabled.                 *
*                                                                                     *
*  The FDIR line is set regardless.  If RWTS, then the INTSTATUS is set for the drive *
\*************************************************************************************/

//void FloppyDelayGOBYTE0(void)     {IRQRingBufferAdd(IRQ_GOBYTE_0, 0L);}
static int my_rwts=0;


void FloppyIRQ_time_up(void)
{

       if (queuedfn==0xff) return;

       if (floppy_6504_wait<254) floppy_6504_wait=0;


       if (queuedfn==FLOP_CMD_UCLAMP)
       {
          DEBUG_LOG(0,"Signaling Eject completed now.");
          append_floppy_log("completed pending Eject IRQ in RWTS::: ");
          floppy_ram[0x2f]=0x80|0x40|0x20;
          floppy_ram[0x2e]=floppy_ram[0x2f] & floppy_ram[0x2c];
          floppy_ram[0x20]=0x0;
          dc42_close_image(FQ);

          queuedfn=0xff;

          return;
       }


       #ifdef DEBUG
       DEBUG_LOG(0,"%s: cpuclock:%016llx, current sector:%ld,(side/track/sect:%d:%d:%d)",
            lastfloppyrwts, cpu68k_clocks,
            getsectornum(&Floppy_u, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]),
            floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR] );
       #endif




        switch ( queuedfn)
        {
         case FLOP_CMD_READX :
         case FLOP_CMD_READ  : append_floppy_log("completing pending READ IRQ in RWTS");
                               {
                               DC42ImageType *F=FQ;
                               if (FQ)
                                    {if (FQ->RAM) do_floppy_read(FQ);
                                     else {
                                            DEBUG_LOG(0,"No floppy in drive");
                                            RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                                            floppy_FDIR=(floppy_ram[0x2e] & 0x80) ? 1:0;
                                          }
                                    }
                               else {
                                     DEBUG_LOG(0,"No floppy in drive");
                                     RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                                     floppy_FDIR=(floppy_ram[0x2e] & 0x80) ? 1:0;
                                    }
                               }
                               #ifdef DEBUG
                               append_floppy_log(lastfloppyrwts);
                               #endif
                               break;

         case FLOP_CMD_WRITX :
         case FLOP_CMD_WRITE : append_floppy_log("completing pending WRITE IRQ in RWTS");
                               {
                               DC42ImageType *F=FQ;
                               if (FQ)
                                    {
                                     if (FQ->RAM) do_floppy_write(FQ);
                                     else {
                                            DEBUG_LOG(0,"No floppy in drive");
                                            RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                                            floppy_FDIR=(floppy_ram[0x2e] & 0x80) ? 1:0;
                                          }
                                    }
                               else {
                                      DEBUG_LOG(0,"No floppy in drive");
                                      RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                                      floppy_FDIR=(floppy_ram[0x2e] & 0x80) ? 1:0;
                                    }
                               }
                               #ifdef DEBUG
                               append_floppy_log(lastfloppyrwts);
                               #endif
                               break;
         default:
         append_floppy_log("unknown queued command");
         DEBUG_LOG(0,"Unknown queued command:%02x",queuedfn);

        }


       if ( my_rwts) //floppy_ram[FLOP_INT_STAT] |=(floppy_ram[DRIVE]?0xc0:0x0c);
       {
           if (floppy_ram[0x2f] & 0x70) floppy_ram[0x2f]|=0x80;
       }

       floppy_ram[0x2e]=floppy_ram[0x2f] & floppy_ram[0x2c];
              DEBUG_LOG(0,"Setting floppyram[intstat]=%02x intresult=%02x intmask=%02x",
                            floppy_ram[0x2f], floppy_ram[0x2e], floppy_ram[0x2c]);

       floppy_FDIR=(floppy_ram[0x2e] & 0x80) ? 1:0;
       if (!floppy_FDIR)
       {
             DEBUG_LOG(0,"DANGER - FDIR not set floppyram[intstat]=%02x intresult=%02x intmask=%02x",
                            floppy_ram[0x2f], floppy_ram[0x2e], floppy_ram[0x2c]);
            floppy_FDIR=1;
       }
       queuedfn=0xff;

}


void FloppyIRQ(uint8 RWTS)
{

      fdir_timer=cpu68k_clocks+(RWTS==2 ? (HALF_OF_A_SECOND):(HUN_THOUSANDTH_OF_A_SEC));  //THOUSANDTH_OF_A_SECOND;
      cpu68k_clocks_stop=MIN(fdir_timer+1,cpu68k_clocks_stop); //2021.06.11
      my_rwts=RWTS;

      #ifndef USE64BITTIMER
      prevent_clk_overflow_now();
      #endif

      DEBUG_LOG(0,"Scheduling FDIR to fire at clock:%016llx  Time now is %016llx",fdir_timer,cpu68k_clocks);
      //2021.06.10 causing IRQ before PC+=iib->opcodesize update! can we live without this?  //get_next_timer_event();
      DEBUG_LOG(0,"returning from FloppyIRQ - event should not have fired now uness CPU stopped, check it please.");
}



#ifdef JUNK
void floppy_sec_dump(int lognum, DC42ImageType *F,int32 sectornumber, char *text)
{
 FILE *f;
 uint32 i,j;
 char c;
 uint32 xfersize=((F->tagsize)+(F->sectorsize));
 uint32 secoff=xfersize*sectornumber+F->sectoroffset;


 errno=0;



 if (lognum)    f=fopen("./lisaem-output.sec","a");
 else           f=fopen("./lisaem-output.profile","a");

 //fprintf(buglog,"sector dump of %ld: tags:",sectornumber);
 fprintf(f      ,"%s floppy sector dump of %ld: tags:",text,(long)sectornumber);
 for (i=0; i<F->tagsize; i++) {//fprintf(buglog,"%02x ",F->RAM[secoff+i]);
                                 fprintf(f     ,"%02x ",F->RAM[secoff+i]);}

 for (i=0; i<F->sectorsize; i+=16)
 {
      //fprintf(buglog,"\nsector dump %ld %04x:: ",sectornumber,i);
      fprintf(f     ,"\n%s sector dump %ld %04x:: ",text,(long)sectornumber,i);

      for (j=0; j<16; j++)  {//fprintf(buglog,"%02x ",F->RAM[secoff+i+j]);
                               fprintf(     f,"%02x ",F->RAM[secoff+i+j]);
                            }
      //fputc('|',buglog);
      fputc('|',f);
      for (j=0; j<16; j++)
      {     c=F->RAM[secoff+i+j];
            c &=0x7f;
            if (c< 32) c|=32;
            if (c>125) c='.';
            //fputc(c,buglog);
            fputc(c,f);
      }
 }
 //fputc('\n',buglog);
 fputc('\n',f);
 fclose(f);
}
#endif

extern void enable_4MB_macworks(void);


static void do_floppy_read(DC42ImageType *F)
{
        long sectornumber=0;
        uint8 *ptr;
        DEBUG_LOG(0,"SRC:Read\n");
        if ( F==NULL)                     { DEBUG_LOG(0,"SRC:null ft\n");       RWTS_IRQ_SIGNAL(FLOP_STAT_DRVNOT);return;  }
        if ( F->fd<0 && !F->fh)           { DEBUG_LOG(0,"SRC:null fhandle\n");  RWTS_IRQ_SIGNAL(FLOP_STAT_DRVNOT);return;  }
        if (floppy_ram[TRACK]>F->maxtrk)  { DEBUG_LOG(0,"SRC:track>max %d>%d\n",floppy_ram[TRACK],F->maxtrk); RWTS_IRQ_SIGNAL(FLOP_STAT_INVTRK); return;}
        if (floppy_ram[SECTOR]>F->maxsec) { DEBUG_LOG(0,"SRC:sect:%d>max %d\n", floppy_ram[SECTOR],F->maxsec); RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}
        if (floppy_ram[SIDE]>F->maxside)  { DEBUG_LOG(0,"SRC:side>max %d>%d\n", floppy_ram[SIDE],F->maxside);  RWTS_IRQ_SIGNAL(FLOP_STAT_INVSID); return;}

        sectornumber=getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);

        #ifdef DEBUG
        if (sectornumber!=queuedsectornumber && queuedsectornumber>0)
        {
            EXIT(1,0,"Danger! Sector number is %ld but was %ld when first requested in do_floppy_read.",sectornumber,queuedsectornumber);
        }
        #endif


        if ( sectornumber==-1)            { DEBUG_LOG(0,"Invalid floppy sector %ld",sectornumber);  RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}


        // convert unbootable fake dual parallel card to have the proper ID.
           if (!sectornumber && (pc24 & 0x00ff0000)==0x00fe0000 && !romless && dualparallelrom[0x30]==0xff && dualparallelrom[0x31]==0xff)
        {
          if (lisa_ram_safe_getbyte(1,0x299)==0x02) {lisa_ram_safe_setbyte(1,0x298,0xe0); }
          if (lisa_ram_safe_getbyte(1,0x29b)==0x02) {lisa_ram_safe_setbyte(1,0x29a,0xe0); }
          if (lisa_ram_safe_getbyte(1,0x29d)==0x02) {lisa_ram_safe_setbyte(1,0x29c,0xe0); }
        }


        floppy_6504_wait=1;

        errno=0;
        DEBUG_LOG(0,"reading from ram.");

        DEBUG_LOG(0,"read floppy sector #%ld tags at offset:%ld size %d bytes",sectornumber,
                 (sectornumber*F->tagsize)    +F->tagstart,F->tagsize          );

        DEBUG_LOG(0,"tagsize is:%ld sectorsize is:%d tagstart is:%ld",F->tagsize,F->sectorsize,F->tagstart);

        if (F->RAM==NULL) {DEBUG_LOG(0,"F->RAM is null!!!"); }

        // wipe the tags
        floppy_ram[0x1F4+0]=0;  floppy_ram[0x1F4+1]=0;  floppy_ram[0x1F4+2]=0;  floppy_ram[0x1F4+3]=0;
        floppy_ram[0x1F4+4]=0;  floppy_ram[0x1F4+5]=0;  floppy_ram[0x1F4+6]=0;  floppy_ram[0x1F4+7]=0;
        floppy_ram[0x1F4+8]=0;  floppy_ram[0x1F4+9]=0;  floppy_ram[0x1F4+10]=0; floppy_ram[0x1F4+11]=0;

        floppy_motor_sounds(floppy_ram[TRACK]);

        DEBUG_LOG(0,"reading data for sector:%d", sectornumber);
        ptr=dc42_read_sector_data(F,sectornumber);  if (!ptr) {DEBUG_LOG(0,"Could not read sector #%ld",sectornumber); return;}
        memcpy(&floppy_ram[DISKDATASEC],ptr,F->datasize);

        if (sectornumber==0) {
            bootblockchecksum=0;
            for (uint32 i=0; i<F->datasize; i++) bootblockchecksum=( (uint32)(bootblockchecksum<<1) | ((uint32)(bootblockchecksum & 0x80000000) ? 1:0) ) ^ (uint32)ptr[i] ^ i;
        }

        DEBUG_LOG(0,"reading tags for sector %d",sectornumber);
        ptr=dc42_read_sector_tags(F,sectornumber);
        if (ptr!=NULL) memcpy(&floppy_ram[DISKDATAHDR],ptr,F->tagsize);

        if (sectornumber==0) {
            for (uint32 i=0; i<F->tagsize; i++) bootblockchecksum=( (bootblockchecksum<<1) | ((bootblockchecksum & 0x80000000) ? 1:0) ) ^ ptr[i] ^ i;

            ALERT_LOG(0,"Bootblock checksum:%08x",bootblockchecksum);

            if (bootblockchecksum==0xce0cbba3 && macworks4mb) enable_4MB_macworks(); 
        }


//        #ifdef DEBUG
//        if (debug_log_enabled)  floppy_sec_dump(0,F,sectornumber,"read");
//        if (debug_log_enabled)  floppy_sec_dump(1,F,sectornumber,"read");
//        #endif


        DEBUG_LOG(0,"read floppy sector #%ld data at offset %ld size %d bytes",sectornumber,
                (sectornumber*F->sectorsize) +F->sectoroffset,F->sectorsize);

        DEBUG_LOG(0,"SRC:read sector #%ld from RAM (side:%d:trk:%d:sec:%d) tag:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                sectornumber,floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR],
                floppy_ram[0x1F4+0],floppy_ram[0x1F4+1],floppy_ram[0x1F4+2],floppy_ram[0x1F4+3],floppy_ram[0x1F4+4], floppy_ram[0x1F4+5],
                floppy_ram[0x1F4+6],floppy_ram[0x1F4+7],floppy_ram[0x1F4+8],floppy_ram[0x1F4+9],floppy_ram[0x1F4+10],floppy_ram[0x1F4+11]
        );


        RWTS_IRQ_SIGNAL(0);
        return;
  }

  static void do_floppy_write(DC42ImageType *F)
  {
        long sectornumber=0;

        if ( F==NULL)                     { DEBUG_LOG(0,"SRC:null ft\n"); RWTS_IRQ_SIGNAL(FLOP_STAT_DRVNOT);return;  }
        if ( F->fd<0 && F->fh==NULL)      { DEBUG_LOG(0,"SRC:null fhandle\n"); RWTS_IRQ_SIGNAL(FLOP_STAT_DRVNOT);return;  }
        if (floppy_ram[TRACK]>F->maxtrk)  { DEBUG_LOG(0,"SRC:track>max %d>%d\n",floppy_ram[TRACK],F->maxtrk); RWTS_IRQ_SIGNAL(FLOP_STAT_INVTRK); return;}
        if (floppy_ram[SECTOR]>F->maxsec) { DEBUG_LOG(0,"SRC:sect:%d>max %d\n",floppy_ram[SECTOR],F->maxsec); RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}
        if (floppy_ram[SIDE]>F->maxside)  { DEBUG_LOG(0,"SRC:side>max %d>%d\n",floppy_ram[SIDE],F->maxside);  RWTS_IRQ_SIGNAL(FLOP_STAT_INVSID); return;}
        sectornumber=getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);

        if ( sectornumber==-1)            { DEBUG_LOG(0,"Invalid floppy write sector %ld",sectornumber);                      RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}


        #ifdef DEBUG
        if (sectornumber!=queuedsectornumber  && queuedsectornumber>0)
        {
            EXIT(1,0,"Danger! Sector number is %ld but was %ld when first requested in do_floppy_write.",sectornumber,queuedsectornumber);
        }
        #endif


        if ( F->fd<0 && F->fh==NULL)
        {   //floppy_ram[FLOP_INT_STAT] |=(floppy_ram[DRIVE]?0x0c:0xc0);
            RWTS_IRQ_SIGNAL(FLOP_STAT_DRVNOT);
            return;
        }

        errno=0;

        DEBUG_LOG(0,"write floppy sector #%ld",sectornumber);

        floppy_motor_sounds(floppy_ram[TRACK]);

        dc42_write_sector_tags(F,sectornumber,&floppy_ram[DISKDATAHDR]);
        dc42_write_sector_data(F,sectornumber,&floppy_ram[DISKDATASEC]);

        RWTS_IRQ_SIGNAL(0);


//        #ifdef DEBUG
//         if (debug_log_enabled)  floppy_sec_dump(0,F,sectornumber,"write");
//         if (debug_log_enabled)  floppy_sec_dump(1,F,sectornumber,"write");
//        #endif
//        return;


  } /////////////////////////////////////////////////////////////////////////////////////////////////////////





void floppy_go6504(void)
{
    static uint8 slowdown;
    long sectornumber=0;
//    char *tempbuf;
    DC42ImageType *F;

    //char *DTCImage1Sig="SIGNATURE: DISK IMAGE FILE      VERSION: 1  ";
 // //                012345678901234567890123456789012345678901234
 //                           1         2         3         4
    int i,j,k;

    flop_cmd_text(buglog);
    if ( floppy_6504_wait==255) {//fprintf(buglog,"floppy controller in jail until reset\n");
                                 return;}          // floppy controller in jail until hard machine reset (power off)

    MOTOR_STOP(0);

    if (!(floppy_ram[0x40]--)) floppy_ram[0x41]--;  // command counter


    i=floppy_ram[CONFIRM];
    j=floppy_ram[FUNCTION];
    k=floppy_ram[GOBYTE];

    if (k>0x83) floppy_ram[0x2a]=floppy_ram[GOBYTE]-0x83;

    // If we're formatting and confirmation isn't set, ignore it and don't say Ok by zeroing the go byte.
    if (k==FLOP_CTRLR_RWTS)  {
                               if ((j==FLOP_CMD_FMTTRK||j==FLOP_CMD_FORMAT)&&i!=0xff) 
                                   {
                                    ALERT_LOG(0,"Format Track reply 0xff not received yet, got %02x",i);
                                    return;
                                   }
                             }

    if (floppy_6504_wait<250)
    {
      slowdown=((slowdown & SLOWDOWN) ? (slowdown & SLOWDOWN)-1 : 0);
      if (k==FLOP_CTRLR_CLIS) {floppy_6504_wait=1;  slowdown=0;} // go on through skip the wait cycle.
      if (k==FLOP_CTRLR_CLIM) {floppy_6504_wait=1;  slowdown=0;} // go on through skip the wait cycle.
      if (k==FLOP_CTRLR_STIM) {floppy_6504_wait=1;  slowdown=0;} // go on through skip the wait cycle.
      DEBUG_LOG(0,"floppy: wait state:%d\n",slowdown);
      if (slowdown) {floppy_6504_wait=1;return;}
    }

    if ( floppy_6504_wait==254)                  // floppy controller is sleeping -
    {
    //fprintf(buglog,"floppy: in wait loop jail\n");
 // since the docs weren't too clear where 69 96 gets stored, I'm putting in several ways to bail
 // out of ROMWAIT...  whichever it turns out is correct will remain in the final code......
            if (floppy_last_macro==  0x96 && k==  0x69)
            {
                DEBUG_LOG(0,"6504 wakeup 1:Got decimal 96 69 seq in gobyte");
                floppy_6504_wait=0; floppy_last_macro=k;return;
            }
            if (floppy_last_macro==  0x69 && k==  0x96)
            {
                DEBUG_LOG(0,"6504 wakeup 1:Got decimal 69 96 seq in gobyte");
                floppy_6504_wait=0; floppy_last_macro=k;return;
            }


 // floppy_return(DC42ImageType *F, uint8 boot, uint8 status)
 // floppy_return(F,0,0);

        floppy_last_macro=k;                    // copy the new gobyte over the last one;
        return;
    }


    floppy_ram[GOBYTE]=0;                   // signal the command as taken
    // cleanup
    floppy_ram[0x26]=floppy_ram[0x19];
    floppy_ram[0x27]=floppy_ram[0x1a];
    floppy_ram[0x42]=floppy_ram[0x17];
    if (floppy_ram[0x33]) floppy_ram[0x33]=32;
    floppy_ram[0x40]=0;
    floppy_ram[0x41]=0;
    floppy_ram[0x48+0]=0;
    floppy_ram[0x48+1]=0;
    floppy_ram[0x48+2]=0;
    floppy_ram[0x48+3]=0;
    floppy_ram[0x48+4]=0;
    floppy_ram[0x48+5]=0;
    floppy_ram[0x48+6]=0;
    floppy_ram[0x48+7]=0;
    floppy_ram[0x7b]=0;


    floppy_last_macro=k;                       // copy the new gobyte over the last one;

    #ifdef DEBUG
    if (floppy_ram[FLOP_PENDING_IRQ_FLAG]!=0 || fdir_timer!=-1)
    {

         if (fdir_timer > cpu68k_clocks )  // if the fdir timer has not yet expired, we don't expect any commands
         {                                 // it's OK to get CLIS when fdir is already on however as code can poll for this.

           if (k==FLOP_CTRLR_CLIS) snprintf(templine,1023,"floppy: FLOP_CTRLR_CLIS %02x while IRQ pending to fire @%16llx\n",k,fdir_timer);
           else                    snprintf(templine,1023,"floppy: DANGER unexpected floppy command %02x (%02x) while IRQ pending @%16lx\n",k,j,fdir_timer);

           DEBUG_LOG(0,templine);
           append_floppy_log(templine);
         }
    }
    #endif

#ifdef IGNORE_FDIR_ON_COMMANDS
    // RWTS can't execute if there's a pending IRQ - return an error.
     if (k==FLOP_CTRLR_RWTS && (floppy_ram[FLOP_PENDING_IRQ_FLAG]!=0 || fdir_timer!=-1) )
             {
                 #ifdef DEBUG
                 DEBUG_LOG(0,"floppy: DANGER cannot execute cmd - IRQ pending.\n");
                 append_floppy_log("floppy: DANGER cannot execute cmd - IRQ pending\n");
                 #endif
 
                 floppy_ram[STATUS]=FLOP_STAT_IRQPND;
                 return;
            }
#endif

    DEBUG_LOG(0," SRC:gobyte:%02x, function:%02x drive:%02x gobyte@%d fn@%d side/trk/sec:%d/%d/%d\n",
            floppy_last_macro,floppy_ram[FUNCTION],floppy_ram[DRIVE],GOBYTE,FUNCTION,
            floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]
            );

    if ( floppy_ram[DRIVE]==0x80)      F=&Floppy_u;
    else if ( floppy_ram[DRIVE]==0x80) F=&Floppy_l;
    else { //fprintf(buglog,"floppy: invalid drive:%02x!\n",floppy_ram[DRIVE]);
           F=&Floppy_u;}



    floppy_ram[STATUS]=0;               // pre-emptive status good
    //floppy_ram[RETRY]=64;

    floppy_ram[0x1d0]=0;
    floppy_ram[0x1d1]=0;
    floppy_ram[0x1d2]=0;
    floppy_ram[0x1d3]=0;

//    floppy_FDIR=0;                    // removing 2004.08.12 3:40am
    //floppy_ram[DISKID]=1;                  // 0=duo 1=lisa disk, 2=mac


    switch ( floppy_last_macro )
    {
        case  0 :                return;               // no command

        case  FLOP_CTRLR_SHAKE:  floppy_ram[STATUS]=0; /* floppy_FDIR=0; removing 2004.08.12 3:40am */ return;

        case  FLOP_CTRLR_RWTS: // FALLTHROUGH 
             floppy_ram[0x68]=0;                                // disable brute force flag

             #ifdef DEBUG
             memset(lastfloppyrwts,0,1024);
             #endif

// two drive code, twiggy code - remove this and modify it to allow it to work
             if ( floppy_ram[DRIVE]!=0x80)
                {floppy_ram[COMMAND]=0; RWTS_IRQ_SIGNAL(FLOP_STAT_INVDRV);
                    //fprintf(buglog,"RWTS access to invalid drive:%02x side:%d, track:%d,sector:%d\n",
                     //       floppy_ram[DRIVE],floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
                    return;}
            /* FALLTHROUGH */ 
            switch(floppy_ram[FUNCTION]) 
            {
            
                case FLOP_CMD_READX : DEBUG_LOG(0,"brute force: "); floppy_ram[0x68]=0xff;   /* FALLTHRU */ // enable brute force flag
                                      
                case FLOP_CMD_READ  : /* FALLTHRU */

                       if (F->fd<0 && F->fh==NULL)       { DEBUG_LOG(0,"SRC:null fhandle\n"); RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK); floppy_FDIR=1;return;}
                       if (F->RAM==NULL)                 { DEBUG_LOG(0,"SRC:no RAM\n");       RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK); floppy_FDIR=1;return;}

                       if (floppy_ram[TRACK]>F->maxtrk)  { DEBUG_LOG(0,"SRC:track>max %d>%d\n",floppy_ram[TRACK],F->maxtrk); RWTS_IRQ_SIGNAL(FLOP_STAT_INVTRK); return;}
                       if (floppy_ram[SECTOR]>F->maxsec) { DEBUG_LOG(0,"SRC:sect:%d>max %d\n",floppy_ram[SECTOR],F->maxsec); RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}
                       if (floppy_ram[SIDE]>F->maxside)  { DEBUG_LOG(0,"SRC:side>max %d>%d\n",floppy_ram[SIDE],F->maxside);  RWTS_IRQ_SIGNAL(FLOP_STAT_INVSID); return;}

                       sectornumber=getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
                       if ( sectornumber==-1)            { DEBUG_LOG(0,"Invalid sector %ld",sectornumber);                   RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}

                       queuedfn=floppy_ram[FUNCTION];
                       if ( floppy_ram[DRIVE]==0x80)      FQ=&Floppy_u;
                       else if ( floppy_ram[DRIVE]==0x80) FQ=&Floppy_l;
                       else { //fprintf(buglog,"floppy: invalid drive:%02x!\n",floppy_ram[DRIVE]);
                              F=&Floppy_u;}

                       #ifdef DEBUG
                        queuedsectornumber=sectornumber;
                       #endif

                       //do_floppy_read(F);  here here here here here here

//                       #ifdef DEBUG
//                         snprintf(templine,1024,"queing read from sector %ld (hd/trk/sec: %d/%d/%d)",
//                                                 sectornumber,floppy_ram[SIDE],floppy_ram[TRACK],floppy_ram[SECTOR]);
//                         append_floppy_log(templine);
//
//                         if ( sectornumber==turn_logging_on_sector)
//                               {
//                                   debug_log_enabled=1;  // turn on debug log on sector read
//                                   debug_on("sector read");
//                                   fprintf(buglog,"***DEBUGGING TURNED ON READ FROM SECTOR %d *****\n",turn_logging_on_sector);
//                                   DEBUG_LOG(0,"LisaEm Debugging turned on - sector read");
//                               }
//                         if (turn_logging_on_sector==-666) {debug_log_enabled=1; debug_on("auto-all-sector");}
//                       #endif
//
//                       #ifdef DEBUG
//                       snprintf(lastfloppyrwts,1023,"read sec #%3ld NOW(side:%d:trk:%d:sec:%d)",
//                               sectornumber,floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
//
//                       //DEBUG_LOG(0,"read from sector #%d",sectornumber);
//
//                       #endif

                       snprintf(templine,1023,"read sec #%ld PENDING (side:%d:trk:%d:sec:%d)\n",
                               sectornumber,floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
                       append_floppy_log("------------------------------------------------------------------");
                       append_floppy_log(templine);


                       FloppyIRQ(1);

                       return;

                case FLOP_CMD_WRITX : fprintf(buglog,"brute force: "); floppy_ram[0x68]=0xff; /* FALLTHRU */
                case FLOP_CMD_WRITE : /* FALLTHRU */
                       DEBUG_LOG(0,"SRC:Write\n");
                       if (F->fd<0)                      { DEBUG_LOG(0,"SRC:null fhandle\n"); RWTS_IRQ_SIGNAL(FLOP_STAT_DRVNOT);return;  }
                       if (floppy_ram[TRACK]>F->maxtrk)  { DEBUG_LOG(0,"SRC:track>max %d>%d\n",floppy_ram[TRACK],F->maxtrk); RWTS_IRQ_SIGNAL(FLOP_STAT_INVTRK); return;}
                       if (floppy_ram[SECTOR]>F->maxsec) { DEBUG_LOG(0,"SRC:sect:%d>max %d\n",floppy_ram[SECTOR],F->maxsec); RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}
                       if (floppy_ram[SIDE]>F->maxside)  { DEBUG_LOG(0,"SRC:side>max %d>%d\n",floppy_ram[SIDE],F->maxside);  RWTS_IRQ_SIGNAL(FLOP_STAT_INVSID); return;}

                       sectornumber=getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
                       if ( sectornumber==-1)            { DEBUG_LOG(0,"Invalid sector %ld",sectornumber);                       RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC); return;}

                       queuedfn=floppy_ram[FUNCTION];
                       if ( floppy_ram[DRIVE]==0x80)      FQ=&Floppy_u;
                       else if ( floppy_ram[DRIVE]==0x80) FQ=&Floppy_l;
                       else { //fprintf(buglog,"floppy: invalid drive:%02x!\n",floppy_ram[DRIVE]);
                              F=&Floppy_u;}

                       #ifdef DEBUG
                        queuedsectornumber=sectornumber;
                        snprintf(templine,1024,"queing write to sector %ld (hd/trk/sec: %d/%d/%d)",
                                                 sectornumber,floppy_ram[SIDE],floppy_ram[TRACK],floppy_ram[SECTOR]);
                        append_floppy_log(templine);
                       #endif


                       snprintf(templine,1024,"write sec #%ld trk/sec/head:%d/%d/%d SRC:\n",sectornumber,floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
                       append_floppy_log(templine);
                       #ifdef DEBUG
                       snprintf(lastfloppyrwts,1023,"write sec #%ld NOW(side:%d:trk:%d:sec:%d) tag:%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x ",
                               sectornumber,floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR],
                       floppy_ram[0x1F4+0],floppy_ram[0x1F4+1],floppy_ram[0x1F4+2],floppy_ram[0x1F4+3],floppy_ram[0x1F4+4], floppy_ram[0x1F4+5],
                       floppy_ram[0x1F4+6],floppy_ram[0x1F4+7],floppy_ram[0x1F4+8],floppy_ram[0x1F4+9],floppy_ram[0x1F4+10],floppy_ram[0x1F4+11]);
                       #endif


//                       #ifdef DEBUG
//                         if ( sectornumber==turn_logging_on_write)
//                               {
//                                   debug_log_enabled=1;  // turn on debug log on sector read
//                                   debug_on("sector read");
//                                   fprintf(buglog,"***DEBUGGING TURNED ON READ FROM SECTOR %d *****\n",turn_logging_on_sector);
//                                   DEBUG_LOG(0,"LisaEm Debugging turned on - sector read");
//                               }
//                         if (turn_logging_on_sector==-666) {debug_log_enabled=1; debug_on("auto-all-sector");}
//                       #endif


                    // do_floppy_write(F);
                    FloppyIRQ(1);
                    return;


                case FLOP_CMD_CLAMP :          // mount/open disk image
                        //fprintf(buglog,"SRC:Floppy Clamp\n");
                        RWTS_IRQ_SIGNAL(0);
                        return;

                case FLOP_CMD_UCLAMP:  // eject/close/unclamp disk image

                    // dumpram("floppy-eject");



//                    #ifdef DEBUG
//                     if (turn_logging_on_sector==-666) {debug_log_enabled=1; debug_on("eject-always-on");}
//                     else {debug_off(); debug_log_enabled=0;}
//                    #endif

                    //fprintf(buglog,"SRC:floppy Unclamp/Eject\n");
                    //DEBUG_LOG(0,"Lisa Ejected floppy");
                    append_floppy_log("Lisa Ejected Floppy.");

                    dc42_close_image(F);
                    RWTS_IRQ_SIGNAL(0);

                    floppy_ram[0x20]=0;
                    floppy_ram[0x2f]=0x80|0x40|0x20;  // not sure about the 0x20 at the end.
                    #ifdef DEBUG
                    snprintf(lastfloppyrwts,1023,"Eject NOW");
                    #endif
                    RWTS_IRQ_SIGNAL(0);
                    FloppyIRQ(2); // 2006.05.21  // still doesn't work too well.  need to add a 1 second delay for eject msg's.

                    DEBUG_LOG(0,"Floppy eject queued.");
                    floppy_ram[0x2f]=0x80|0x40; //|0x20;  // not sure about the 0x20 at the end.

                    //enabled-for-macworks-20060607
                    //if (!debug_log_enabled) { debug_on("eject-always-on"); debug_log_enabled=1;  }

                    floppy_FDIR=1;

                    eject_floppy_animation();

                    return;

                case FLOP_CMD_FMTTRK: ALERT_LOG(0,"Format track."); 
                     /* FALLTHRU */      // ignore the rest o'this
                case FLOP_CMD_FORMAT: ALERT_LOG(0,"Format whole disk."); 
                    /* FALLTHRU */ // format a track (erase all it's sectors!)

                case FLOP_CMD_VERIFY:                                    
                    /* FALLTHRU */
                case FLOP_CMD_VFYTRK: floppy_FDIR=1;  // was missing from pre RC2!
                                      RWTS_IRQ_SIGNAL(0);
                                      ALERT_LOG(0,"...")
                                      return;
            }
            /* FALLTHRU */
        case  FLOP_CTRLR_SEEK: 
            //fprintf(buglog,"SRC:seek\n");
            ///RWTS_IRQ_SIGNAL(0);  //2005.02.04 - maybe we don't need to wait on fdir?
                    floppy_ram[SPEED]=0;
                    floppy_ram[STATUS]=0;

            return;  // NO FDIR here charlie!
            break; //this is fucking bullshit!

        case  FLOP_CTRLR_JSR :  // trap these and complain loudly of 6504 usage. //
              ALERT_LOG(0,"SRC:The Lisa is trying to tell the 6504 floppy controller to execute code. (%08x) JSR %02x%02x  This is not always supported by the emulator!\n",
              floppy_ram[0], floppy_ram[2],floppy_ram[1]);
            
              if (floppy_ram[0x200]==0xa0 &&  // 0200   A0 00      LDY #$00  
                  floppy_ram[0x201]==0x00 &&   
                  floppy_ram[0x202]==0x88 &&  // 0202   88         DEY  FF FE
                  floppy_ram[0x203]==0xc8 &&  // 0203   C8         INY  00 00   
                  floppy_ram[0x204]==0x88 &&  // 0204   88         DEY  FF FE
                  floppy_ram[0x205]==0xd0 &&  // 0205   D0 FB      BNE $0202   // loop around 256 times, Y=0
                  floppy_ram[0x206]==0xfb &&    
                  floppy_ram[0x207]==0x88 &&  // 0207   88         DEY         // this should now be FF?
                  floppy_ram[0x208]==0x84 &&  // 0208   84 00      STY $00     // store FF in zero?
                  floppy_ram[0x209]==0x00 && 
                  floppy_ram[0x20a]==0xa5 &&  // 020A   A5 00      LDA $00     // loop forever? wtf? why?
                  floppy_ram[0x20b]==0x00 && 
                  floppy_ram[0x20c]==0xd0 &&  // 020C   D0 FC      BNE $020A   // wonder if it will later modify this code after this piece executes.
                  floppy_ram[0x20d]==0xfc && 
                  floppy_ram[0x20e]==0x60   ) // 020E   60         RTS
              {
                #ifdef DEBUG
                //debug_on("MacWorks+II forever loop");
                #endif
                floppy_ram[0]=0xff;
                reg68k_regs[1]=0x1ff;
                ALERT_LOG(0,"MW+II 6504 timing/go-away-loop part 1 completed.");
                return;
              }
                                                     // hack to allow MacWorks XL 3.0 code to run - think that this reads the
                                                     // write protect tab of the current floppy, not 100% sure.
            ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if (floppy_ram[0x200]==0x20 &&           // JSR 1320 for I/O 88 ROM, JSR 1302 for I/O A8 ROM.
               (floppy_ram[0x201]==0x20 || floppy_ram[0x201]==0x02) &&
                floppy_ram[0x202]==0x13 &&           //
                floppy_ram[0x203]==0xa9 &&           // lda  #$00
                floppy_ram[0x204]==0x00 &&           //
                floppy_ram[0x205]==0xaa &&           // tax
                floppy_ram[0x206]==0xb0 &&           // bcs   +1  // skip 1 opcode past dex, (execute STX)
                floppy_ram[0x207]==0x01 &&           //
                floppy_ram[0x208]==0xca &&           // dex
                floppy_ram[0x209]==0x86 &&           // stx $08   // fcd0011 - status (0 or ff)
                floppy_ram[0x20a]==0x08 &&           //
                floppy_ram[0x20b]==0x85 &&           // sta $06   // fcd000d - speed - signal end
                floppy_ram[0x20c]==0x06 &&           //
                floppy_ram[0x20d]==0x60    )         // rts
                {
                    // if we want to signal a write protected disk, return 0xff in status instead.
                    floppy_ram[SPEED]=0;
                    floppy_ram[STATUS]=0;
                    ALERT_LOG(0,"6504 write-protect-jsr executed");
                    return;
                }



            // lisatest bus error test
            if (floppy_ram[0x01f4]==0x8d &&               //  01f4 8d 19 04: sta $0419  //lisatest  // set DIS
                floppy_ram[0x01f5]==0x19 &&               //  01f7 a2 ff:    ldx #$ff               // wait
                floppy_ram[0x01f6]==0x04 &&               //  01f9 ca:       dex                    //
                floppy_ram[0x01f7]==0xa2 &&               //  01fa d0 fd:    bne $1f9               //
                floppy_ram[0x01f8]==0xff &&               //  01fc 8d 18 04: sta $0418              // clear DIS
                floppy_ram[0x01f9]==0xca &&               //  01ff 60:       rts                    // return
                floppy_ram[0x01fa]==0xd0 &&
                floppy_ram[0x01fb]==0xfd &&
                floppy_ram[0x01fc]==0x8d &&
                floppy_ram[0x01fd]==0x18 &&
                floppy_ram[0x01fe]==0x04 &&
                floppy_ram[0x01ff]==0x60    )
                {
                    ALERT_LOG(0,"6504 LisaTest Disk Test #11 executed - it might be wanting to set a bus error");

                    /*
                      4402869 1/0000d11a (lisacx 0 0/0/0) opcode=4a29 TST.B      $0001(A1)    SRC:clk:845889594:(diff:8)
                      4402870 memory.c:lisa_rb_io:2963:@00fcc001

                    */
                    lisa_buserror(0xfcc001);
                    return;
                }

            ALERT_LOG(0,"Attempt to execute unrecognized 6504 code");
            #ifdef DEBUG
            debug_on("Unrecognized 6504 code");
            #endif 
           {
             int i;
             for ( i=0; i<2048; i+=16)
             {
                ALERT_LOG(0,"%04x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", i,
                    floppy_ram[i+ 0],floppy_ram[i+ 1],floppy_ram[i+ 2],floppy_ram[i+ 3],
                    floppy_ram[i+ 4],floppy_ram[i+ 5],floppy_ram[i+ 6],floppy_ram[i+ 7],
                    floppy_ram[i+ 8],floppy_ram[i+ 9],floppy_ram[i+10],floppy_ram[i+11],
                    floppy_ram[i+12],floppy_ram[i+13],floppy_ram[i+14],floppy_ram[i+15]  );

             }
            }

            RWTS_IRQ_SIGNAL(FLOP_STAT_INVCMD);
            EXIT(84,1,"Attempt to execute unrecognized 6504 code.  Emulation failed.");
            return;

        case  FLOP_CTRLR_CLIM:

            floppy_ram[FLOP_INT_MASK] &= (floppy_ram[FUNCTION]^0xff);      // clear mask bits
            floppy_ram[FLOP_INT_STAT] &= (floppy_ram[FUNCTION]^0xff);      // clear current state as well
            floppy_ram[FLOP_PENDING_IRQ_FLAG]=0;
            floppy_return(F,0,0);
            floppy_6504_wait=0;

            if (!floppy_ram[FLOP_INT_STAT]) {floppy_FDIR=0;fdir_timer=-1;}  /* keeping this one 2004.08.12 3:40am */
            return;

        case  FLOP_CTRLR_STIM: //86                                        // drive enable interrupt
            floppy_ram[FLOP_INT_MASK] |= (floppy_ram[FUNCTION]);
            floppy_ram[FLOP_PENDING_IRQ_FLAG]=0;
            floppy_irq_top=((floppy_ram[FUNCTION] & 0x80) ? 1:0);
            floppy_irq_bottom=((floppy_ram[FUNCTION] & 0x08) ? 1:0);
            DEBUG_LOG(0,"SRC:floppy F IRQ enabled on FDIR fn(%02x) irqtop:%d irqbot:%d:: MASK IS NOW:%02x\n",
                    floppy_ram[FUNCTION],floppy_irq_top,floppy_irq_bottom,floppy_ram[FLOP_INT_MASK]);

            floppy_return(F,0,0);       // no call to RWTS_IRQ_SIGNAL() since NO IRQ is issued here.
            floppy_6504_wait=0;
            //if (!floppy_ram[FLOP_INT_STAT]) floppy_FDIR=0;
            floppy_FDIR=0; fdir_timer=-1;             /* keeping 2004.08.12 3:40am */
            return;

        case  FLOP_CTRLR_CLIS:

            floppy_ram[FLOP_INT_STAT] &= (floppy_ram[0x01]^0xff);

            // clear IRQ if the low bits are cleared
            if ((floppy_ram[FLOP_INT_STAT] & 0x0f)==0x08) floppy_ram[FLOP_INT_STAT] &=0xf0;
            if ((floppy_ram[FLOP_INT_STAT] & 0xf0)==0x80) floppy_ram[FLOP_INT_STAT] &=0x0f;

            floppy_ram[0x2e]=0;
            DEBUG_LOG(0,"SRC:floppy Clear IRQ Status(%02x) result written to post fixintstat IRQSTAT:%02x\n",
                    floppy_ram[FUNCTION],floppy_ram[FLOP_INT_STAT]);

            floppy_ram[STATUS]=0; //20070723// buggy bug?
            floppy_return(F,0,0);
            floppy_6504_wait=0;
            //if (!floppy_ram[FLOP_INT_STAT]) floppy_FDIR=0;  //commented out 20060607 21:28
            floppy_FDIR=0; fdir_timer=-1;
            return;

        case  FLOP_CTRLR_WAIT: // 88  Wait in ROM until cold start
            DEBUG_LOG(0,"Floppy controller in 6996 wait jail");
            floppy_6504_wait=254;
            floppy_return(F,0,0);
            return;

        case  FLOP_CTRLR_LOOP:  //89    Loop in ROM forever
            fflush(buglog);
            floppy_6504_wait=255;
            DEBUG_LOG(0,"Floppy controller in RESET wait jail");
            floppy_return(F,0,0);

            DEBUG_LOG(0,"Lisa is about to power off - floppy controller told to go away");


            return;
        default:
            ALERT_LOG(0,"SRC:unrecognized command GOBYTE:%08x, FUNCTION:%08x\n",floppy_last_macro,floppy_ram[FUNCTION]);
            //RWTS_IRQ_SIGNAL(FLOP_STAT_INVCMD);
    }
}

void hexprint(char *x, int size, int ascii_print)
{
int i,half;
unsigned char c;
char ascii[130];
half=(size/2) -1;
   if (size>128) {fprintf(buglog,"SRC:hexprintf given illegal size %d\n",size); return;};
   memset(ascii,0,130);
   for (i=0; i<size; i++)
    {
     c=x[i];

     if (i==half) printf("%02x . ",c);
     else printf("%02x ",c);

     if (ascii_print)
          {
        if (c>126) c &=127;
        if (c<31)     c |= 32;
        ascii[i]=c;
      }
    }
   if (size<16) while(16-size) {printf("   "); size++;}
   if (ascii_print) printf("  |  %s\n",ascii);
}





/**********************************************************************************\
*  Routine to reset the return values of the ram block for the lisa.               *
*                                                                                  *
*                                                                                  *
\**********************************************************************************/

void floppy_return(DC42ImageType *F, uint8 boot, uint8 status)
{

    if (!!(floppy_ram[ROMVER] & 0x80))
       floppy_ram[TYPE]=(double_sided_floppy) ? SONY800KFLOPPY : SONY400KFLOPPY;
    else 
       floppy_ram[TYPE]=TWIGGYFLOPPY;
    //ALERT_LOG(0,"Set drive type to:%d",floppy_ram[TYPE]);

    if (!F) return;
       // override drive type if needed.
       if (F->numblocks==1702 && floppy_ram[TYPE]!=0)   floppy_ram[TYPE]=TWIGGYFLOPPY;
       if (F->numblocks==800  && floppy_ram[TYPE]==0)   floppy_ram[TYPE]=SONY400KFLOPPY; // ignore 400k disk in 800k drive, but not 400k in twiggy
       if (F->numblocks==1600 && floppy_ram[TYPE]!=2)   floppy_ram[TYPE]=SONY800KFLOPPY;

//    switch(F->ftype)
//        {// floppy type 0=twig, 1=sony400k, 2=sony800k, 3=freeform, 254/255=disabled
//            case  TWIGGYFLOPPY:   // FALLTHROUGH
//            case  SONY400KFLOPPY: // FALLTHROUGH
//            case  SONY800KFLOPPY: floppy_ram[TYPE]=F->ftype; break;
//            default: floppy_ram[TYPE]=SONY400KFLOPPY;  // lie.
//            ALERT_LOG(0,"Set drive type to:%d",floppy_ram[TYPE]);
//        }

    if ( boot )
    {
        floppy_ram[GOBYTE    ]=0x00;
        floppy_ram[FUNCTION  ]=0x88;
        floppy_ram[DRIVE     ]=0x80;
        floppy_ram[SIDE      ]=0x00;
        floppy_ram[SECTOR    ]=0x00;
        floppy_ram[TRACK     ]=0x01;
        floppy_ram[SPEED     ]=0x00;
        floppy_ram[CONFIRM   ]=0x00;

        // which one of these is correct?????
        //floppy_ram[FLOP_STAT ]=status;

        floppy_ram[STATUS    ]=status;

        floppy_ram[DISKID    ]=0x00;
        floppy_ram[INTERLEAVE]=0x01;
  //    floppy_ram[TYPE      ]=SONY400_TYPE;
        floppy_ram[STST      ]=0x00;
        floppy_ram[INTSTATUS ]=0;

        floppy_ram[DRIVE_ENABLED]=0x80;


  // since this is an emulator and everything is virtual, there won't be any errors at all
  // if only real life were so perfect, "But then again, as you said, it's an imperfect universe, Mr. Bester"

        floppy_ram[FLOPPY_dat_bitslip1 ]=0;
        floppy_ram[FLOPPY_dat_bitslip2 ]=0;
        floppy_ram[FLOPPY_dat_chksum   ]=0;
        floppy_ram[FLOPPY_adr_bitslip1 ]=0;
        floppy_ram[FLOPPY_adr_bitslip2 ]=0;
        floppy_ram[FLOPPY_wrong_sec    ]=0;
        floppy_ram[FLOPPY_wrong_trk    ]=0;
        floppy_ram[FLOPPY_adr_chksum   ]=0;
        floppy_ram[FLOPPY_usr_cksum1   ]=0;
        floppy_ram[FLOPPY_usr_cksum2   ]=0;
        floppy_ram[FLOPPY_usr_cksum3   ]=0;
        floppy_ram[FLOPPY_bad_sec_total]=0;
        floppy_ram[FLOPPY_err_track_num]=0;
        floppy_ram[FLOPPY_err_side_num ]=0;
        floppy_ram[FLOPPY_bad_sect_map ]=0;

        floppy_ram[0x15]=0x1e;
        floppy_ram[0x16]=4;
        floppy_ram[0x17]=9;
        floppy_ram[0x19]=0x64;
        floppy_ram[0x1a]=2;
        floppy_ram[0x1b]=0x82;
        floppy_ram[0x1c]=0x4f;
        floppy_ram[0x1d]=0x0c;

        floppy_ram[0x48]=0;
        floppy_ram[0x49]=0;
        floppy_ram[0x4a]=0;
        floppy_ram[0x4b]=0;
        floppy_ram[0x4c]=0;
        floppy_ram[0x4d]=0;
        floppy_ram[0x4e]=0;
        floppy_ram[0x4f]=0;
    }


    floppy_ram[GOBYTE]=0;
    floppy_ram[STATUS]=status;          // it passed the self tests.
    //floppy_ram[ROMVER]=FLOPPY_ROM_VERSION;  // handled in the C++ code now!

    if ((floppy_ram[ROMVER] & 96)==32) via_clock_diff=2; //via_clock_diff=10;  // slow VIA clock
    else                               via_clock_diff=5; //via_clock_diff=4;

    MOTOR_SPIN(0);

    floppy_ram[CURRENT_TRACK]=floppy_ram[TRACK];
    floppy_ram[CURRENT_TRACK_GRP]=floppy_ram[TRACK]>>4;

    floppy_ram[SPEED]=floppy_ram[0x10 +(floppy_ram[CURRENT_TRACK_GRP])];


    floppy_ram[DRIVE_EXISTS]=0xff;

    floppy_ram[0x26]=0;
    floppy_ram[0x27]=0;

    return;
}


void get_lisa_serialnumber(uint32 *plant, uint32 *year, uint32 *day, uint32 *sn, uint32 *prefix, uint32 *net)
{
  /*
https://www.applefritter.com/content/apple-lisa-serial-number-info
First remove every other nibble like this:
00000240: 0F0F 0002 0802 0002 0000 0400 0300 0F0F
240: F F 0 2 8 2 0 2 0 0 4 0 3 0 F F

Then group the numbers as follows:
Number of the Nibble in Hex 01 23 45 678 9ABC D EF

+         +0+1 +2+3 +4+5 +6+7 +8+9 +a+b +c+d +e+f
00000240: 0F0F 0002 0802 0002 0000 0400 0300 0F0F
          XXXX XPXP XYXY XDXD XDXS XSXS XSXX XXXX


Address 240: FF 02 82 020 0403 0 FF
             XX PP YY DDD SSSS X XX

Extract the serial number from this group of 16 nibbles as follows:

a. Ignore nibbles 0,1,D,E and F, marked as XX or X above.
b. Nibbles 2 and 3 are the two digit plant code (PP).
c. Nibbles 4 and 5 are the two digit year code (YY).
d. Nibbles 6, 7 and 8 are the day of the year code (DDD).
e. Nibbles 9 thru C are the 4 digit serial number (SSSS).

The Applenet Number is similarly embedded in the first 8 bytes of the next
line of the memory dump. So, using the same method as step 4 above, we get:

+        +0+1 +2+3 +4+5 +6+7 +8+9 +a+b +c+d +e+f
00000250 0000|0100|0004|0102|0002|0900|0000|0000
     250: 0 0| 1 0| 0 4| 1 2| 0 2| 9 0| 0 0|0 0
          P P  P N  N N  N N
Number of the Nibble in Hex 012 34567 89ABCDEF

Address 250: 001 00412 02900000
             PPP NNNNN XXXXXXXX
             123 12345

7. To extract the Applenet Number:

a. Ignore nibbles 8 through F, marked as XXXXXXXX above.
b. Nibbles 0, 1 and 2 are the AppleNet prefix (PPP).
c. Nibbles 3 thru 7 are the AppleNet number (NNNNN).

          ff02 8308 1040 50ff 0010 1635 0470 0000
          XXXX XPXP XYXY XDXD XDXS XSXS XSXX XXXX
                              vvvv
mine:     ff028308104050ff0010163504700000
theirs:   ff02080202004030ff00100412029000
                              ^^^^
    0x0f,0x0f,0x00,0x02,0x08,0x03,0x00,0x08,0x01,0x00,0x04,0x00,0x05,0x00,0x0f,0x0f,  // 250 
    0x00,0x00,0x01,0x00,0x01,0x06,0x03,0x05,0x00,0x04,0x07,0x00,0x00,0x00,0x00,0x00,  // 260 


plant 38 year 00 day 0f0 0654
ff028308104050ff0010163504700000 <<- mine
ff028308104050ff0010163504700000
XXXXXPXPXYXYXDXDXDXSXSXSXSXXXXXX
     3 8|0 0|0 f 0|0 6 5 4     sn=654 -> 1620 dec.
FF028202004030FF <<- example
plant 38, year 0 (1983), day 240, sn 0654

this is all wrong I think:
Your Lisa's serial number was built
in Apple Plant #02 on the 081st day of 1983
with serial #0405 (1029)
It has the applenet id: 001:01635

floppy.c:get_lisa_serialnumber:1532:serial240: ff 02 83 08 10 40 50 ff 00 10 16 35 04 70 00 00
                                               XX XX XP XP XY XY XD XD XD XS XS XS XS XX XX XX
                                                      3  8 |0  0 |0  f  0| 0  6  5  4 |

floppy.c:get_lisa_serialnumber:1538:serial250: 00 00 00 00 00 00 00 00 00 05 08 00 00 00 00 00
floppy.c:get_lisa_serialnumber:1541:Lisa SN: plant:38 born on year:0x0 (0) day:0xf0 (240) sn:0x604 (1540) applenet 0-0
floppy.c:deserialize:1581:Disk signed by Lisa SN: 10663



*/
        *plant  =                                       ((serialnum240[0x02] & 0x0f)<< 4) | ( serialnum240[0x03] & 0x0f);
        *year   =                                       ((serialnum240[0x04] & 0x0f)<< 4) | ( serialnum240[0x05] & 0x0f);
        *day    =   ((serialnum240[0x06] & 0x0f)<< 8) | ((serialnum240[0x07] & 0x0f)<< 4) | ( serialnum240[0x08] & 0x0f);

        *sn     =   ((serialnum240[0x09] & 0x0f)<<12) | ((serialnum240[0x0a] & 0x0f)<< 8) | ((serialnum240[0x0b]<<4) & 0x0f) |
                                                                                             (serialnum240[0x0c]     & 0x0f);
// applenet
        *prefix =   ((serialnum240[0x10] & 0x0f)<< 8) | ((serialnum240[0x11] & 0x0f)<< 4) |  (serialnum240[0x12] & 0x0f);
        *net    =   ((serialnum240[0x13] & 0x0f)<<16) | ((serialnum240[0x14] & 0x0f)<<12) | ((serialnum240[0x15] & 0x0f)<<8) |
                                                        ((serialnum240[0x16] & 0x0f)<< 4) |  (serialnum240[0x17] & 0x0f);


         ALERT_LOG(0,"serial240: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                     serialnum240[ 0], serialnum240[ 1], serialnum240[ 2], serialnum240[ 3],
                     serialnum240[ 4], serialnum240[ 5], serialnum240[ 6], serialnum240[ 7],
                     serialnum240[ 8], serialnum240[ 9], serialnum240[10], serialnum240[11],
                     serialnum240[12], serialnum240[13], serialnum240[14], serialnum240[15]
                  );
         ALERT_LOG(0,"serial250: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                     serialnum240[16], serialnum240[17], serialnum240[18], serialnum240[19],
                     serialnum240[20], serialnum240[21], serialnum240[22], serialnum240[23],
                     serialnum240[24], serialnum240[25], serialnum240[26], serialnum240[27],
                     serialnum240[28], serialnum240[29], serialnum240[30], serialnum240[31]
                  );

         ALERT_LOG(0,"Lisa SN: plant:%x born on year:0x%x (%d) day:0x%x (%d) sn:0x%x (%d) applenet %x-%x",
                   *plant, *year, *year, *day, *day, 
                   *sn, *sn, *prefix, *net);

}

void deserialize(DC42ImageType *F)
{
         uint32 plant, year, day, sn, prefix, net;
         get_lisa_serialnumber(&plant, &year, &day, &sn, &prefix, &net);

         uint8 *mddftag=dc42_read_sector_tags(F,28);
         uint8 *mddfsec=dc42_read_sector_data(F,28);



//         ALERT_LOG(0,"Disk Serial #%02x%02x%02x%02x Lisa Serial#%02x%02x%02x%02x",
//             mddfsec[0xcc], mddfsec[0xcd], mddfsec[0xce], mddfsec[0xcf],
//             serialnum240[8],serialnum240[9],serialnum240[10],serialnum240[11]);
        
         // Remove Lisa signature on previously used Serialized Master Disks //////////////////////////////////////////////
         if (mddftag[4]==0 && mddftag[5]==1 &&  // MDDF Signature
             mddfsec[0x0d]=='O' &&
             mddfsec[0x0e]=='f' &&
             mddfsec[0x0f]=='f' &&
             mddfsec[0x10]=='i' &&
             mddfsec[0x11]=='c' &&
             mddfsec[0x12]=='e' &&
             mddfsec[0x13]==' ' &&
             mddfsec[0x14]=='S' &&
             mddfsec[0x15]=='y' &&
             mddfsec[0x16]=='s' &&
             mddfsec[0x17]=='t' &&
             mddfsec[0x18]=='e' &&
             mddfsec[0x19]=='m'    )
          {

         uint32 disk_sn=(mddfsec[0xcc]<<24) | (mddfsec[0xcd]<<16) | (mddfsec[0xce]<<8) | (mddfsec[0xcf]);

         ALERT_LOG(0,"Disk signed by Lisa SN: %08x (%d)",disk_sn,disk_sn);

         if (disk_sn != sn && disk_sn!=0)
                {
                     {
                        uint8 buf[512];
                        memcpy(buf,mddfsec,512);
                        buf[0xcc]=buf[0xcd]=buf[0xce]=buf[0xcf]=0;
                        dc42_write_sector_data(F,28,buf);
                     }
     
                }
           }

         ////   Deserialize tools on diskettes //////////////////////////////////////////////////////////////////////////////////////
         uint8 *ftag; //=dc42_read_sector_tags(F,28);
         uint8 *fsec; //=dc42_read_sector_data(F,28);
         int sec;
         for (sec=32; sec<128; sec++)
             {
               char name[64];
               ftag=dc42_read_sector_tags(F,sec);
//               ALERT_LOG(0,"Checking sector:%d fileid4=%02x",sec,ftag[4]);

               if (ftag[4]==0xff)                  // tool entry tags have tag 4 as ff, others do as well, but it's a good indicator
               {
                  fsec=dc42_read_sector_data(F,sec);
                  int s=fsec[0];                   // size of string (pascal string)
                  // possible file name, very likely to be the right size.  
                  // Look for {T*}obj.  i.e. {T5}obj is LisaList, but could have {T9999}obj but very unlikely
                  // also check the friendly tool name size at offset 0x182.
    
                  if (s>6 && s<32 && fsec[1]=='{' && fsec[2]=='T' && fsec[3]>='0' && fsec[3]<='9'  &&
                         fsec[s-3]=='}' && tolower(fsec[s-2])=='o' && tolower(fsec[s-1])=='b' && tolower(fsec[s])=='j' && fsec[0x182]<64 )
                  {
                    uint32 toolsn=(fsec[0x42]<<24) | (fsec[0x43]<<16) | (fsec[0x44]<<8) | fsec[0x45];
                    if (  toolsn != sn )     
                       {
                        s=fsec[0x182];               // Size of tool name
                        memcpy(name,&fsec[0x183],s);  // copy it over.
                        name[s]=0;                   // string terminator.

                        char message[256];
                        snprintf(message,256,
                             "Found Office System tool %s serialized for Lisa #%02x%02x%02x%02x.  Your virtual Lisa is #%02x%02x%02x%02x, which may prevent installation.  De-Serialize this tool?",
                             name,
                             fsec[0x42], fsec[0x43], fsec[0x44], fsec[0x45],
                              serialnum240[8],
                              serialnum240[9],
                              serialnum240[10],
                              serialnum240[11]);
                             
                        if (yesnomessagebox(message,"De-Serialize this Tool?") )
                           {
                                uint8 buf[512];
                                memcpy(buf,fsec,512);
                                buf[0x42]=buf[0x43]=buf[0x44]=buf[0x45]=buf[0x48]=buf[0x49]=0;
                                dc42_write_sector_data(F,sec,buf);
                           }
                      }        
                  }
               }
               
    
    
             }
 }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




//insert_floppy
int floppy_insert(char *Image)     // emulator should call this when user decides to open disk image...
{
    DC42ImageType *F;
    int err=0;
    int floppynum=1;

    floppy_picked=1;

    DEBUG_LOG(0,"Inserting [%s] floppy",Image);
    DEBUG_LOG(0,"MAX RAM:%08x MINRAM:%08x TOTRAM:%08X BADRAMID:%02x SYSTEMTYPE:%02x 0=lisa1, 1=lisa2, 2=lisa2+profile 3=lisa2+widget ramchkbitmap:%04x",
            fetchlong(0x294),
            fetchlong(0x2a4),
            fetchlong(0x2a8),
            fetchbyte(0x2ad), fetchbyte(0x2af),
            fetchword(0x184));

    floppynum=1;                        // Lisa 2 can only boot off one floppy!


    append_floppy_log("Inserting floppy:");
    append_floppy_log(Image);

    if ( floppynum)
    {
      strncpy(Floppy_u.fname,Image,255);

      F=&Floppy_u;
      floppy_ram[INTSTAT]|=(FLOP_IRQ_SRC_DSKIN2|FLOP_IRQ_SRC_DRV2);  fix_intstat(0);
    }
    else
    {
      strncpy(Floppy_u.fname,Image,255);

      F=&Floppy_l;

      floppy_ram[INTSTAT]|=(FLOP_IRQ_SRC_DSKIN1|FLOP_IRQ_SRC_DRV1);  fix_intstat(0);
    }

    //fprintf(buglog,"SRC:Opening Floppy Image file... %s\n",F->fname);
    errno=0;

    dc42_close_image(F);                        // close any previously opened disk image
    err=dc42_auto_open(F,Image,"wb");           // for testing the emulator, open images as private "p"  w=writeable, b=best
    if (err)
       {
          floppy_return(F,0,FLOP_STAT_NOCLMP);  // return failed clamp status
          FloppyIRQ(1);
          ALERT_LOG(0,"could not open: %s because %s",Image,F->errormsg );
          messagebox(F->errormsg, "Could not open this Floppy! Sorry!");
          append_floppy_log("Could not open floppy");
          perror("error");
          return -1;

       }

    err=dc42_check_checksums(F);    // 0 if they match, 1 if tags, 2 if data, 3 if both don't match
    switch (err)
    {
      case 1 : messagebox("The Tag checksum failed for this disk.  It may be corrupted.","Danger!"); break;
      case 2 : messagebox("The Data checksum failed for this disk.  It may be corrupted.","Danger!"); break;
      case 3 : messagebox("Both the Data and Tag checksums failed for this disk!","Danger!"); break;
      default:
      case 0 : ;
    }

    if (F->numblocks==1440 || F->numblocks==2880 || F->numblocks==5760)
        {
          messagebox("Yuck! I can't swallow MFM (720K/1.44M/2.88M) floppies, those are for PC's", "Wrong kind of floppy");

          floppy_return(F,0,FLOP_STAT_NOCLMP);
          FloppyIRQ(1);
          dc42_close_image(F);
          return -1;
        }

    if (F->numblocks>1742)
        {
          messagebox("That's a very strangely sized disk!", "Wrong kind of floppy");

          floppy_return(F,0,FLOP_STAT_NOCLMP);
          FloppyIRQ(1);
          dc42_close_image(F);
          return -1;
        }



    deserialize(F);


    floppy_return(F,0,0);

    FloppyIRQ(1);

    floppy_ram[0x20]=0xff;
    floppy_ram[0x2f]=0x10|0x80;

    //floppy_ram[INTSTAT]|=(FLOP_IRQ_SRC_DSKIN2|FLOP_IRQ_SRC_DRV2);

    fix_intstat(0);

    floppy_FDIR=1;

    return 0;
}




void fix_intstat(int RWTS)
{
  uint8 intstat=floppy_ram[FLOP_INT_STAT];

  // correct IRQ status if needed.
  intstat &=0x77;
  intstat |=((intstat & 0x70) ? 0x80:0) | ((intstat & 0x07) ? 0x08:0);
  floppy_ram[FLOP_INT_STAT]=intstat;

  floppy_ram[0x2e]=floppy_ram[FLOP_INT_MASK] & floppy_ram[FLOP_INT_STAT];

  if (floppy_ram[0x2e])
        {
         DEBUG_LOG(0,"Will be Firing IRQ1 after delay since INTSTAT is %02x & INTMASK is %02x, 2e is :%02x\n",
         floppy_ram[FLOP_INT_STAT],floppy_ram[FLOP_INT_MASK],floppy_ram[0x2e]);


         FloppyIRQ(RWTS);
         //floppy_FDIR=1; fdir_timer=cpu68k_clocks+HUNDRETH_OF_A_SECOND;
         //IRQRingBufferAdd(IRQ_FDIR_ON, 0L);

        }
  if (floppy_6504_wait<250) floppy_6504_wait=0;  // shut down diag wait

/*
7 set if bits 4,5,6 set
6 if RWTS is complete for drive 2                        upper
5 when button on drive 2 is pushed
4 when disk in place on drive 2

3 set if bits 0,1,2 set
2 set if RWTS complete for drive1
1 set when button on drive 1 is pushed
0 set when disk in place on drive 1                      lower
*/

}


void floppy_button(uint8 floppynum)     // press floppy button
{ floppy_ram[FLOP_INT_STAT] |=(floppynum?0x02:0x20); fix_intstat(0);  }


void init_floppy(long iorom)
{
 Floppy_u.RAM=NULL;
 Floppy_l.RAM=NULL;
 Floppy_u.fd=-1;
 Floppy_l.fd=-1;
 Floppy_u.fh=NULL;
 Floppy_l.fh=NULL;

 floppy_FDIR=0; fdir_timer=-1;
 floppy_6504_wait=0;

 //memset(floppy_ram,0,1024);             // clear ram
 // load_pram();                          // obsolete - let Lisaconfig handle this

 // setup various stuff found in floppy mem, not sure what all of these are,
 // but set them incase the 68k expects them...

 floppy_ram[0x06]=0x00;
 floppy_ram[0x08]=0x00;
 floppy_ram[0x09]=0x01;
 floppy_ram[0x0a]=0x01;
 floppy_ram[0x0c]=0x00;

 floppy_ram[0x10]=0xD5;
 floppy_ram[0x11]=0xC0;
 floppy_ram[0x12]=0xA7;
 floppy_ram[0x13]=0x89;
 floppy_ram[0x14]=0x64;
 floppy_ram[0x15]=0x1E;
 floppy_ram[0x16]=0x04;
 floppy_ram[0x17]=0x09;
 floppy_ram[0x18]=iorom & 0xff; // FLOPPY_ROM_VERSION now obsolete, handled by C++ code now.

 floppy_ram[0x19]=0x64;
 floppy_ram[0x1a]=0x02;
 floppy_ram[0x1b]=0x82;
 floppy_ram[0x1c]=0x4F;
 floppy_ram[0x1d]=0x0c;

 floppy_ram[0x20]=0x00;
 floppy_ram[0x21]=0xff;
 floppy_ram[0x24]=0xff;
 floppy_ram[0x25]=0x02;

 floppy_ram[0x38]=0xD5;
 floppy_ram[0x39]=0xAA;
 floppy_ram[0x3a]=0x96;
 floppy_ram[0x3b]=0xDE;
 floppy_ram[0x3c]=0xAA;

 floppy_ram[0x46]=0xff;

 floppy_ram[0x70]=0xD5;
 floppy_ram[0x71]=0xAA;
 floppy_ram[0x72]=0xAD;
 floppy_ram[0x73]=0xDE;
 floppy_ram[0x74]=0xAA;

if (!!(floppy_ram[ROMVER] & 0x80))
    floppy_ram[TYPE]=(double_sided_floppy) ? SONY800KFLOPPY : SONY400KFLOPPY;
else 
    floppy_ram[TYPE]=TWIGGYFLOPPY;

 floppy_return(NULL,1,FLOP_STAT_NODISK);
}






void sector_checksum(void)
{
uint16 a,atmp,x,y,c,nc,m43=0;
#define CARRY(x) {c=(a<0xff ?1:0); a &=0xff;}
#define ASL {a=((a<<1) | c); c=(a<0xff ?1:0); a &=0xff;}
#define ADC(x) {a+=(x)+c; c=(a<0xff ?1:0); a &=0xff;}
#define INX    {x++; x&=0xff;}
#define INY    {y++; y&=0xff;}
#define TAX    {x=a;}
#define TXA    {a=x;}

        a=0; c=0; nc=0;                        //lsb,msb (m43,m44)
        floppy_ram[0x64]=0; floppy_ram[0x65]=0;
        floppy_ram[0x43]=0; floppy_ram[0x44]=0;
        m43=0x100;                                          //1937
        y=0xf4;                                             //1939

x193b:  ASL;                                                //193b
        atmp=a;                                             //193c
        ADC(0x00);                                          //193d
        floppy_ram[0x64]=a;                                 //193f
        a=atmp;                                             //1941
        a=floppy_ram[m43+y];                                //1942
        TAX;                                                //1944
        a ^=floppy_ram[0x63];                               //1945
        //floppy_ram[m43+y]=a;                              //1947
        TXA;                                                //1949
        ADC(floppy_ram[0x61]);                              //194a
        floppy_ram[0x61]=a;                                 //194c
        INY;                                                //194e
        if (y) goto x1953;       //bne                      //194f
        m43+=0x100;              //inc $44                  //1951
x1953:  a=floppy_ram[m43+y];                                //1953
        TAX;                                                //1955
        ADC(floppy_ram[0x62]);                              //1956
        floppy_ram[0x62]=a;                                 //1958
        TXA;                                                //195a
        a^=floppy_ram[0x61];                                //195b
        //floppy_ram[m43+y]=a;                              //195d
        INY;                                                //195f
        if (!y) goto xret;   //1973::rts                    //1960
        a=floppy_ram[m43+y];                                //1962
        TAX;                                                //1964
        a^=floppy_ram[0x62];                                //1965
        //floppy_ram[m43+y]=a                               //1967
        TXA;                                                //1969
        ADC(floppy_ram[0x63]);                              //196a
        INY;                                                //196c
        if (y) goto x193b;                                  //196d
        m43+=0x100;                                         //196f
        if (m43>>8) goto x193b;                             //1971
xret:
        floppy_ram[0x44]=m43>>8; floppy_ram[0x43]=m43 & 0xff;
}



// if we could factor large composites in real time
// we'd have enough money not to need to rhyme
// digesting messages with a hashing function
// using sha-1 or else it will cause disfuction -- MC++
