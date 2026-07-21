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

/**
 * TorZidan@: How to do regression testing if modifying this code?
 * The code is very fragile and therefore it must be tested thoroughly at each code change.
 * 0. In Preferences: use "H" ROMs; also, choose "Boot Rom Speedup Hacks" and "Hard Drive Acceleration" checkboxes.
 * 1. Switch I/O ROM to "40", restart the emulator, then install LOS1.0 from floppies (make sure to erase the disk), 
 *    then restart the emulator and boot from ProFile. 
 * 2. Switch I/O ROM to "A8" (or to any version other than "40") and restart the emulator. All tests below are done with this I/O ROM.
 * 3. Install LOS2.0 from floppies (make sure to erase the disk), then restart the emulator and boot from ProFile.
 * 4. Install LOS3.1 from floppies (make sure to erase the disk), then restart the emulator and boot from ProFile.
 * 5. Once booted in LOS3.1, insert and eject at-least 20 times all different kinds of Sony floppy images. 
 *    Spend more time on images that LOS3.1 does not recognize (and prompts you with a dialog). 
 *    When prompted to initialize such images, click "Cancel".
 *    Also, try File->Insert blank diskette, and then click "Initialize". Then eject it.
 *    If you get a popup error that "Lisa is experiencing technical difficulties and need to restart", rever your code.
 * 6. Install MacWorksXL3.0 from floppies (make sure to erase the disk), then restart the emulator, "insert" the "boot" disk and boot from it. 
 * 7. If any of the installations above randomly error out with Lisa error "10291" or any other error (and Lisa returns to the boot screen), 
 *    revert your code.
 *    Once booted, you should see the ProFile disk on the desktop.
 * 8. Optional: Install Pascal Workshop 3.0 (make sure to erase the disk), then restart the emulator and boot from ProFile.
 */
#define IN_FLOPPY_C
#include <vars.h>

#ifdef DEBUG
static int16 turn_logging_on_sector = -1, turn_logging_on_write = -1;
#endif

// slowdown floppy access - was 7f
#define SLOWDOWN 0x03

// Floppy controller macro commands
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
#define FLOP_CMD_READ   0x00  // Read
#define FLOP_CMD_WRITE  0x01  // Write
#define FLOP_CMD_UCLAMP 0x02  // Unclamp - aka eject
#define FLOP_CMD_FORMAT 0x03  // Format
#define FLOP_CMD_VERIFY 0x04  // Verify
#define FLOP_CMD_FMTTRK 0x05  // Format Track
#define FLOP_CMD_VFYTRK 0x06  // Verify Track
#define FLOP_CMD_READX  0x07  // Read without checksum vfy
#define FLOP_CMD_WRITX  0x08  // Write without checksum
#define FLOP_CMD_CLAMP  0x09  // Clamp

#define FLOPPY_MOTOR 0x21
#define MOTOR_SPIN(x)                    \
    {                                    \
        floppy_ram[FLOPPY_MOTOR] = 0xff; \
    }
#define MOTOR_STOP(x)                    \
    {                                    \
        floppy_ram[FLOPPY_MOTOR] = 0x00; \
    }

// These are Unused.
// #define FLOP_SELECT 0xFCC005
// #define FLOP_SEL_LOWER 0x00
// #define FLOP_SEL_UPPER 0x80
// #define FLOP_SEL_DRV2 0x00
// #define FLOP_SEL_DRV1 0x80
// #define FLOP_SIDE_SELECT 0xFCC007
// #define FLOP_SIDE_1 0x00
// #define FLOP_SIDE_2 0x01
// #define FLOP_SIDE_UPPER 0x00
// #define FLOP_SIDE_LOWER 0x01
// #define FLOP_SECTOR_NUMBER 0xFCC009 // 0-22
// #define FLOP_TRACK_NUMBER 0xFCC00B  // 0-44
// #define FLOP_SPEED_BYTE 0xFCC00D
// #define FLOP_FORMAT_CONFIRM 0xFCC00F // Format confirm byte
// #define FLOP_ERROR_STATUS 0xFCC011
// #define FLOP_DISK_ID_VALUE 0xFCC013

#define CURRENT_TRACK 0x22
#define CURRENT_TRACK_GRP 0x23
#define DRIVE_EXISTS 0x23

// Note(TorZidan@): There was confusion in the previous code about at which address is the "interrupt mask" byte.
// Both 0x2c and 0x2d were used in the code. I switched all code to use FLOP_INT_MASK=0x2d. Time will tell if this works well.
#define FLOP_INT_MASK 0x2d    // interrupt mask
// Note: address 0x2e is also used, e.g: floppy_ram[0x2e] = floppy_ram[FLOP_INT_MASK] & floppy_ram[FLOP_INT_STAT]
#define FLOP_INT_STAT 0x2f    // interrupt status address, mapped to 0xFCC05F

#define FLOP_STAT_INVCMD 0x01 // invalid command
#define FLOP_STAT_INVDRV 0x02 // invalid drive
#define FLOP_STAT_INVSEC 0x03 // invalid sector
#define FLOP_STAT_INVSID 0x04 // invalid side
#define FLOP_STAT_INVTRK 0x05 // invalid track
#define FLOP_STAT_INVCLM 0x06 // invalid clear mask
#define FLOP_STAT_NODISK 0x07 // no disk
#define FLOP_STAT_DRVNOT 0x08 // drive not enabled

#define FLOP_STAT_IRQPND 0x09 // Interrupts pending

#define FLOP_STAT_INVFMT 0x0a  // Invalid format configuration
#define FLOP_STAT_BADROM 0x0b  // ROM Selftest failure
#define FLOP_STAT_BADIRQ 0x0c  // Unexpected IRQ or NMI
#define FLOP_STAT_WRPROT 0x14  // Write protect error
#define FLOP_STAT_BADVFY 0x15  // Unable to verify
#define FLOP_STAT_NOCLMP 0x16  // Unable to clamp disk
#define FLOP_STAT_NOREAD 0x17  // Unable to read
#define FLOP_STAT_NOWRIT 0x18  // Unable to write
#define FLOP_STAT_NOWRITE 0x18 // Unable to write
#define FLOP_STAT_NOUCLP 0x19  // Unable to unclamp/eject
#define FLOP_STAT_NOCALB 0x1A  // Unable to find calibration
#define FLOP_STAT_NOSPED 0x1B  // Unable to adjust speed
#define FLOP_STAT_NWCALB 0x1c  // Unable to write calibration

#define FLOP_CPU_RAM 0xFCC181
#define FLOP_CPU_RAM_END 0xFCC1FF

#define FLOP_XFER_RAM 0xFCC501
#define FLOP_XFER_RAM_END 0xFCC7FF

// These are the "Floppy-disk Interrupt Sources" stored at address 0xFCC05F:
// See them on Figure 2-9 at https://lisa.sunder.net/LisaHardwareManual1983.pdf#page=50
#define FLOP_IRQ_SRC_DRV2 128  // set if 4, 5, or 6 set.
#define FLOP_IRQ_SRC_RWTS2 64  // set if drive 2 RWTS complete for drive 2
#define FLOP_IRQ_SRC_BTN2 32   // set if button on disk 2 pressed
#define FLOP_IRQ_SRC_DSKIN2 16 // set if disk in place in drive 2
#define FLOP_IRQ_SRC_DRV1 8    // set if bits 0,1 or 2 are set
#define FLOP_IRQ_SRC_RWTS1 4   // set if drive 1 RWTS complete for drive 1
#define FLOP_IRQ_SRC_BTN1 2    // set if button on disk 1 pressed
#define FLOP_IRQ_SRC_DSKIN1 1  // set if disk inserted in drive 1

#define NOFLOPPY -1
#define GENERICIMAGE 3

// Possible values at floppy_ram[TYPE] = $FCC015  = drive type:
#define SONY800KFLOPPY 2
#define SONY400KFLOPPY 1
#define TWIGGYFLOPPY 0

#define FLOPPY_ROM_VERSION 0xa8 // want a8 here normally

// I/O Control Block, see https://lisa.sunder.net/LisaHardwareManual1983.pdf#page=201
#define GOBYTE (0)   // gobyte     1
#define COMMAND (0)  // gobyte     1              // synonym
#define FUNCTION (1) // function   3
#define DRIVE (2)    // drive      5              // 00=lower, 80=upper
#define SIDE (3)     // side       7
#define SECTOR (4)   // sector     9
#define TRACK (5)    // track      b
#define SPEED (6)    // rotation speed 0=normal, DA is fast
#define CONFIRM (7)  // format confirm
#define STATUS (8)
#define INTERLEAVE (9) // sector interleave
#define TYPE (0xA)     // drive type id 0-twig, 1-sony, 2-double sony

#define STST (0xB)    // rom controller self test status
#define ROMVER (0x18) // ROM Version
// above are correct, not sure about below

// fcc05d=pending IRQ's?

// These are unused.
//#define LISATYPE 0x0018
//#define LISATYPE_LISA1 0
//#define LISATYPE_LISA2 (32 | 128)  // Lisa 2 with slow timers
//#define LISATYPE_LISA2F (64 | 128) // Lisa 2 with fast timers (or pepsi)
//#define LISATYPE_LISA2PEPSI (128)

#define DISKDATAHDR (0x1f4)              // disk buffer header
#define DISKDATASEC ((DISKDATAHDR) + 12) // disk buffer data  // check this!!!

// other status and error vars
#define FLOPPY_dat_bitslip1 0x005b
#define FLOPPY_dat_bitslip2 0x005c
#define FLOPPY_dat_chksum 0x005d
#define FLOPPY_adr_bitslip1 0x005e
#define FLOPPY_adr_bitslip2 0x005f
#define FLOPPY_wrong_sec 0x0060
#define FLOPPY_wrong_trk 0x0061
#define FLOPPY_adr_chksum 0x0062
#define FLOPPY_usr_cksum1 0x007e
#define FLOPPY_usr_cksum2 0x007f
#define FLOPPY_usr_cksum3 0x0080
#define FLOPPY_bad_sec_total 0x01d0
#define FLOPPY_err_track_num 0x01d1
#define FLOPPY_err_side_num 0x01d2
#define FLOPPY_bad_sect_map 0x01d3

#define FLOP_PENDING_IRQ_FLAG 0x2f

// RWTS subcommands
#define FLOP_CMD_READ   0x00 // Read
#define FLOP_CMD_WRITE  0x01 // Write
#define FLOP_CMD_UCLAMP 0x02 // Unclamp/eject
#define FLOP_CMD_FORMAT 0x03 // Format
#define FLOP_CMD_VERIFY 0x04 // Verify
#define FLOP_CMD_FMTTRK 0x05 // Format Track
#define FLOP_CMD_VFYTRK 0x06 // Verify Track
#define FLOP_CMD_READX  0x07 // Read without checksum vfy
#define FLOP_CMD_WRITX  0x08 // Write without checksum
#define FLOP_CMD_CLAMP  0x09 // Clamp
#define FLOP_CMD_OKAY   0xFF // Okay byte for format

// The data structures of the two floppy images that are currently in use.
// If e.g. the upper drive has no floppy inserted, its current_upper_floppy_image.RAM will be NULL. 
// Lisa 1 has two drives that map to the structs below; Lisa 2 has just one drive,
// which is the current_lower_floppy_image; the current_upper_floppy_image is unused by Lisa 2
// (Lisa 2 software never tries to use the upper drive).
DC42ImageType current_upper_floppy_image;
DC42ImageType current_lower_floppy_image;

// Keep stats here. They do not get reset until the emuiltor is restarted. 
// Why? There may be two floppy images inserted at any time, so resetting them upon eject of each flopy does not make sense.
uint32 total_num_sectors_read = 0;
uint32 total_num_sectors_written = 0;

static uint8 queuedfn = 0xff;
static uint8 queuedfn_drive = 0xff; // On which drive was the function requested (0x00 = upper, 0x80 = lower, 0xff = an invalid drive)

long getsectornum(DC42ImageType *F, uint8 side, uint8 track, uint8 sec);

#ifdef DEBUG
static char lastfloppyrwts[1024];
static long queuedsectornumber = -1;
#endif

// don't move this to vars file, must be local to this!
static uint8 floppy_last_macro;
// include the sector translation tables.

static void do_floppy_read(DC42ImageType *F);
static void do_floppy_write(DC42ImageType *F);

void fix_intstat_if_needed(void);
void RWTS_IRQ_SIGNAL(uint8 status);
void floppy_return(uint8 status);

uint8 is_upper_floppy_currently_inserted(void);
uint8 is_lower_floppy_currently_inserted(void);
uint8 is_floppy_image_currently_inserted(DC42ImageType *F);

char templine[1024];

/*
 * Print the command we are about to execute (stored at floppy_ram[GOBYTE]) and floppy_ram[FUNCTION])
 */
void flop_cmd_text(FILE *buglog)
{
    return; // shut it off for now -- too noisy

    if (!floppy_ram[GOBYTE])
        return; // gobyte=0 means Floppy Controller accepted command.  nothing to see here, move along citizen
    fprintf(buglog, "SRC: ");

    floppy_ram[0x30] = floppy_ram[0x00];
    floppy_ram[0x31] = floppy_ram[0x01];
    floppy_ram[0x32] = floppy_ram[0x02];
    floppy_ram[0x33] = floppy_ram[0x03];
    floppy_ram[0x34] = floppy_ram[0x04];
    floppy_ram[0x35] = floppy_ram[0x05];
    floppy_ram[0x36] = floppy_ram[0x06];
    floppy_ram[0x37] = floppy_ram[0x07];

    switch (floppy_ram[GOBYTE])
    {
    case FLOP_CTRLR_SHAKE:
        fprintf(buglog, "floppy cmd handshake\n");
        break;

    case FLOP_CTRLR_SEEK:
        fprintf(buglog, "floppy cmd seek\n");
        break;

    case FLOP_CTRLR_JSR:
        fprintf(buglog, "floppy cmd jsr\n");
        break;

    case FLOP_CTRLR_CLIS:
        fprintf(buglog, "floppy cmd clear IRQ status\n");
        break;

    case FLOP_CTRLR_STIM:
        fprintf(buglog, "floppy cmd set IRQ mask\n");
        break;

    case FLOP_CTRLR_CLIM:
        fprintf(buglog, "floppy cmd clear IRQ mask\n");
        break;

    case FLOP_CTRLR_WAIT:
        fprintf(buglog, "floppy cmd wait\n");
        break;

    case FLOP_CTRLR_LOOP:
        fprintf(buglog, "floppy cmd die\n");
        break;

    case FLOP_CTRLR_RWTS:
        switch (floppy_ram[FUNCTION])
        {
        case FLOP_CMD_READ:
            fprintf(buglog, "floppy cmd RWTS: read sector\n");
            break;
        case FLOP_CMD_WRITE:
            fprintf(buglog, "floppy cmd RWTS: write sector\n");
            break;
        case FLOP_CMD_UCLAMP:
            fprintf(buglog, "floppy cmd RWTS: unclamp/eject\n");
            break;
        case FLOP_CMD_FORMAT:
            fprintf(buglog, "floppy cmd RWTS: format floppy\n");
            break;
        case FLOP_CMD_VERIFY:
            fprintf(buglog, "floppy cmd RWTS: verify\n");
            break;
        case FLOP_CMD_FMTTRK:
            fprintf(buglog, "floppy cmd RWTS: format track\n");
            break;
        case FLOP_CMD_VFYTRK:
            fprintf(buglog, "floppy cmd RWTS: verify track\n");
            break;
        case FLOP_CMD_READX:
            fprintf(buglog, "floppy cmd RWTS: read sector ignoring errors\n");
            break;
        case FLOP_CMD_WRITX:
            fprintf(buglog, "floppy cmd RWTS: write sector with errors\n");
            break;
        case FLOP_CMD_CLAMP:
            fprintf(buglog, "floppy cmd RWTS: clamp\n");
            break;
        default:
            fprintf(buglog, "floppy cmd RWTS: unknown command:%02x\n", floppy_ram[FUNCTION]);
            break;
        }
    case 0:
        break;
    default:
        fprintf(buglog, "floppy unrecognized go-byte:%02x\n", floppy_ram[GOBYTE]);
    }
}

#ifdef DEBUGXXX
void append_floppy_log(char *s)
{
    FILE *f;
    // fprintf(buglog,s);
    f = fopen("./lisaem-output.sec", "a");
    if (f)
    {
        printlisatime(f);
        flop_cmd_text(f);
        fprintf(f, "pc24:%08x %s #%3d tags:%02x%02x, %02x%02x, %02x%02x, %02x%02x, %02x%02x, %02x%02x\n", pc24, s,
                getsectornum(&current_lower_floppy_image, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]),
                floppy_ram[0x1F4 + 0], floppy_ram[0x1F4 + 1], floppy_ram[0x1F4 + 2], floppy_ram[0x1F4 + 3], floppy_ram[0x1F4 + 4], floppy_ram[0x1F4 + 5],
                floppy_ram[0x1F4 + 6], floppy_ram[0x1F4 + 7], floppy_ram[0x1F4 + 8], floppy_ram[0x1F4 + 9], floppy_ram[0x1F4 + 10], floppy_ram[0x1F4 + 11]);

        fflush(f);
        fclose(f);
    }
}
#else
void append_floppy_log(char *s) { s = NULL; }
#endif

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
    if (s >= (12 - z))
        return -1;
    return b[z] + t * (12 - z) + s;
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
    if (s >= (12 - z))
        return -1;
    return b[z] + (t * 2 + d) * (12 - z) + s;
}

/**
 * 5.25" Twiggy floppy disk image files are generated by the Basic Lisa Utility 
 * (aka BLU, see http://sigmasevensystems.com/BLU.html).
 * BLU stores the sector (and tags) data of every valid combination of diskSide/Track/Sector
 * into a binary floppy disk image file in DC42 format.
 * This code tries to reverse-engineer that:
 * Given a track, disc side, and sector number that the Lisa wants to operate on,
 * find that sector (and its tags) in the DC42 image.
 * 
 * So this code generates a sector offset from the start of the disc image. 
 * Once we have that, we can seek to that sector's data in the DC42 image's "data section",
 * as well as to that sector's "tags" in the DC42 image's "tags section".
 * 
 * The Lisa Twiggy floppy disk has 46 consecutive sectors starting from 0 on the
 * outer side to 45 on the inside. The number of sectors are different on different tracks, 
 * which allows the Lisa to stuff in more data on the longer outer tracks.abort_opcode
 * 
 * The tracks for the two sides of the disc are stored in a consecutive (non-interleaved)
 * fashion in the DC42 image: side 1, track 0, sector 0 comes directly afer 
 * side 0, track 45, sector 14.
 * 
 * On each disk side we have (starting from track 0 on the outer side of the disk):
 * 4 tracks x 22 sectors =  88 sectors
 * 7 tracks x 21 sectors = 147 sectors
 * 6 tracks x 20 sectors = 120 sectors
 * 6 tracks x 19 sectors = 114 sectors
 * 6 tracks x 18 sectors = 108 sectors
 * 6 tracks x 17 sectors = 102 sectors
 * 7 tracks x 16 sectors = 112 sectors
 * 4 tracks x 15 sectors =  60 sectors
 * -------------------------
 * Total sectors per side= 851 sectors
 * Total sectors per disk= 851*2 = 1702  (0 to 1701)
 * Total tracks: 46 (0 to 45).
 * Vilid "side"-s: 0, 1
 * 
 * See more at https://lisa.sunder.net/LisaHardwareManual1983.pdf#page=209
 * 
 * Typical Twiggy DC42 file size in bytes:
 * 84(header bytes) + (512bytes + 12tags)*1702 sectors = 891,932 bytes, 
 * but most files are actually rounded up to the next whole 1024 bytes 
 * (which is not really needed), to arrive at the typical file size of 892,928 bytes.
 */
int floppy_twiggy(int side, int t, int s)
{
    if (t < 0 || side < 0 || s < 0)
        return -1;

    int o = 851 * side;
    int result = o;
    
    if (t>4) {
        result += 4*22;
        t = t-4;
    } else {
        return (s < 22) ? result + (t*22) + s : -1;
    }
    
    if (t>7) {
        result += 7*21;
        t = t-7;
    } else {
        return (s < 21) ? result + (t*21) + s : -1;
    }

    if (t>6) {
        result += 6*20;
        t = t-6;
    } else {
        return (s < 20) ? result + (t*20) + s : -1;
    }

    if (t>6) {
        result += 6*19;
        t = t-6;
    } else {
        return (s < 19) ? result + (t*19) + s : -1;
    }
    
    if (t>6) {
        result += 6*18;
        t = t-6;
    } else {
        return (s < 18) ? result + (t*18) + s : -1;
    }
    
    if (t>6) {
        result += 6*17;
        t = t-6;
    } else {
        return (s < 17) ? result + (t*17) + s : -1;
    }
    
    if (t>7) {
        result += 7*16;
        t = t-7;
    } else {
        return (s < 16) ? result + (t*16) + s : -1;
    }
    
    if (t>=4) {
        return -1;
    } else {
        return (s < 15) ? result + (t*15) + s : -1;
    }
}

/**********************************************************************************\
* Get a disk image virtual sector number given a Lisa floppy side, track, sec      *
* number and disk type parameter.                                                  *
\**********************************************************************************/

long getsectornum(DC42ImageType *F, uint8 side, uint8 track, uint8 sec)
{
    switch (F->ftype)
    {
    case TWIGGYFLOPPY:
        if (track > 45 || sec > 21 || side > 1)
            return -1;
        return floppy_twiggy(side, track, sec);

    case SONY400KFLOPPY:
        if (side > 0)
            return -1;
        return floppy_sony400(track, sec);

    case SONY800KFLOPPY:
        if (track > 79 || sec > 11 || side > 1)
            return -1;
        return floppy_sony800(track, side, sec);

    // Fake SuperFloppy - if possible -- for future use.
    case 3:
        if (track > F->maxtrk || sec > F->maxsec || side > F->maxside)
            return -1;
        return side * (F->maxtrk * F->maxsec) + (track * F->maxsec) + sec;
    }
    return -1;
}

/*************************************************************************************\
*  Interrupt the 68000 if needed - and if the interrupts are enabled.                 *
*                                                                                     *
*  The FDIR line is set regardless.  If RWTS, then the INTSTATUS is set for the drive *
\*************************************************************************************/
void FloppyIRQ_time_up(void)
{
    // ALERT_LOG(0, "In FloppyIRQ_time_up() : queuedfn=%02x, queuedfn_drive=%02x \n", queuedfn, queuedfn_drive);

    if (queuedfn == 0xff)
        return;

    if (floppy_6504_wait < 254)
        floppy_6504_wait = 0;

#ifdef DEBUG
    DEBUG_LOG(0, "%s: cpuclock:%016llx, current sector:%ld,(side/track/sect:%d:%d:%d)",
              lastfloppyrwts, cpu68k_clocks,
              getsectornum(&current_lower_floppy_image, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]),
              floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
#endif

    switch (queuedfn)
    {
    case FLOP_CMD_READX:
    case FLOP_CMD_READ:
        append_floppy_log("completing pending READ IRQ in RWTS");
        {
            DC42ImageType *F = (queuedfn_drive==0)? &current_upper_floppy_image:&current_lower_floppy_image;
            if (F)
            {
                if (F->RAM)
                    do_floppy_read(F);
                else
                {
                    DEBUG_LOG(0, "No floppy in drive");
                    RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                    floppy_FDIR = 1;
                }
            }
            else
            {
                DEBUG_LOG(0, "No floppy in drive");
                RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                floppy_FDIR = 1;
            }
        }
#ifdef DEBUG
        append_floppy_log(lastfloppyrwts);
#endif
        break;

    case FLOP_CMD_WRITX:
    case FLOP_CMD_WRITE:
        append_floppy_log("completing pending WRITE IRQ in RWTS");
        {
            DC42ImageType *F = (queuedfn_drive==0)? &current_upper_floppy_image:&current_lower_floppy_image;
            if (F)
            {
                if (F->RAM)
                    do_floppy_write(F);
                else
                {
                    DEBUG_LOG(0, "No floppy in drive");
                    RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                    floppy_FDIR = 1;
                }
            }
            else
            {
                DEBUG_LOG(0, "No floppy in drive");
                RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                floppy_FDIR = 1;
            }
        }
#ifdef DEBUG
        append_floppy_log(lastfloppyrwts);
#endif
        break;
    default:
        append_floppy_log("unknown queued command");
        DEBUG_LOG(0, "Unknown queued command:%02x", queuedfn);
    }

    floppy_ram[FLOP_INT_STAT] |=(queuedfn_drive?0xc0:0x0c);
    fix_intstat_if_needed();

    floppy_ram[0x2e] = floppy_ram[FLOP_INT_MASK] & floppy_ram[FLOP_INT_STAT];
    DEBUG_LOG(0, "Setting floppyram[intstat]=%02x intresult=%02x intmask=%02x",
              floppy_ram[0x2f], floppy_ram[0x2e], floppy_ram[0x2c]);

    floppy_FDIR = (floppy_ram[0x2e] > 0)? 1 : 0;
    if (!floppy_FDIR)
    {
        ALERT_LOG(0, "DANGER - FDIR not set floppyram[intstat]=%02x intresult=%02x intmask=%02x",
                  floppy_ram[0x2f], floppy_ram[0x2e], floppy_ram[0x2c]);
        floppy_FDIR = 1;
    }
    queuedfn = 0xff;
    queuedfn_drive = 0xff;
}

/**
 * The disk drives generate an interrupt to the CPU whenever a disk is inserted or ejected, 
 * and when an 0x81 command (RWTS routine) completes.
 * 
 * The FloppyIRQ(uint32 delay) function generates a Floppy Disk Interrupt request (aka FDIR) ,
 * e.g. "read sector RWTS routine has completed", so that the 68000 can come and get the data. 
 * 
 * The CPU can examine the command success/failure at memory offset 0xFCC011. 
 * The possible values are in Figure 2-8 at https://lisa.sunder.net/LisaHardwareManual1983.pdf#page=49
 * 
 * The CPU can find the interrupt source (e.g. "RWTS routine completed for Drive 1") at memory address 0xFCC05F. 
 * The possible values are in Figure 2-9 at https://lisa.sunder.net/LisaHardwareManual1983.pdf#page=50
 * 
 * Note: this code just schedules the interrupt to be generated after a delay, which gives
 * a chance to the 68000 to get to the "wait for interrupt" cycle.
 */
void FloppyIRQ(uint32 delay)
{

    fdir_timer = cpu68k_clocks + delay;
    cpu68k_clocks_stop = MIN(fdir_timer + 1, cpu68k_clocks_stop);                              // 2021.06.11

#ifndef USE64BITTIMER
    prevent_clk_overflow_now();
#endif
    DEBUG_LOG(0, "Scheduling FDIR to fire at clock:%016llx  Time now is %016llx", fdir_timer, cpu68k_clocks);
    // 2021.06.10 causing IRQ before PC+=iib->opcodesize update! can we live without this?  //get_next_timer_event();
    DEBUG_LOG(0, "returning from FloppyIRQ - event should not have fired now uness CPU stopped, check it please.");
}

#ifdef JUNK
void floppy_sec_dump(int lognum, DC42ImageType *F, int32 sectornumber, char *text)
{
    FILE *f;
    uint32 i, j;
    char c;
    uint32 xfersize = ((F->tagsize) + (F->sectorsize));
    uint32 secoff = xfersize * sectornumber + F->sectoroffset;

    errno = 0;

    if (lognum)
        f = fopen("./lisaem-output.sec", "a");
    else
        f = fopen("./lisaem-output.profile", "a");

    // fprintf(buglog,"sector dump of %ld: tags:",sectornumber);
    fprintf(f, "%s floppy sector dump of %ld: tags:", text, (long)sectornumber);
    for (i = 0; i < F->tagsize; i++)
    { // fprintf(buglog,"%02x ",F->RAM[secoff+i]);
        fprintf(f, "%02x ", F->RAM[secoff + i]);
    }

    for (i = 0; i < F->sectorsize; i += 16)
    {
        // fprintf(buglog,"\nsector dump %ld %04x:: ",sectornumber,i);
        fprintf(f, "\n%s sector dump %ld %04x:: ", text, (long)sectornumber, i);

        for (j = 0; j < 16; j++)
        { // fprintf(buglog,"%02x ",F->RAM[secoff+i+j]);
            fprintf(f, "%02x ", F->RAM[secoff + i + j]);
        }
        // fputc('|',buglog);
        fputc('|', f);
        for (j = 0; j < 16; j++)
        {
            c = F->RAM[secoff + i + j];
            c &= 0x7f;
            if (c < 32)
                c |= 32;
            if (c > 125)
                c = '.';
            // fputc(c,buglog);
            fputc(c, f);
        }
    }
    // fputc('\n',buglog);
    fputc('\n', f);
    fclose(f);
}
#endif

extern void enable_4MB_macworks(void);

// Note: the validation of side,track,sector, etc has been done by the caller. No need to do it again.
static void do_floppy_read(DC42ImageType *F)
{
    uint8 *ptr;
    long sectornumber = getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
#ifdef DEBUG
    if (sectornumber != queuedsectornumber && queuedsectornumber > 0)
    {
        EXIT(1, 0, "Danger! Sector number is %ld but was %ld when first requested in do_floppy_read.", sectornumber, queuedsectornumber);
    }
#endif
    // convert unbootable fake dual parallel card to have the proper ID.
    if (!sectornumber && (pc24 & 0x00ff0000) == 0x00fe0000 && !romless && dualparallelrom[0x30] == 0xff && dualparallelrom[0x31] == 0xff)
    {
        if (lisa_ram_safe_getbyte(1, 0x299) == 0x02)
        {
            lisa_ram_safe_setbyte(1, 0x298, 0xe0);
        }
        if (lisa_ram_safe_getbyte(1, 0x29b) == 0x02)
        {
            lisa_ram_safe_setbyte(1, 0x29a, 0xe0);
        }
        if (lisa_ram_safe_getbyte(1, 0x29d) == 0x02)
        {
            lisa_ram_safe_setbyte(1, 0x29c, 0xe0);
        }
    }

    floppy_6504_wait = 1;

    errno = 0;
    DEBUG_LOG(0, "reading from ram.");
    DEBUG_LOG(0, "read floppy sector #%ld tags at offset:%ld size %d bytes", sectornumber,
              (sectornumber * F->tagsize) + F->tagstart, F->tagsize);
    DEBUG_LOG(0, "tagsize is:%ld sectorsize is:%d tagstart is:%ld", F->tagsize, F->sectorsize, F->tagstart);

    // wipe the tags
    floppy_ram[0x1F4 + 0] = 0;
    floppy_ram[0x1F4 + 1] = 0;
    floppy_ram[0x1F4 + 2] = 0;
    floppy_ram[0x1F4 + 3] = 0;
    floppy_ram[0x1F4 + 4] = 0;
    floppy_ram[0x1F4 + 5] = 0;
    floppy_ram[0x1F4 + 6] = 0;
    floppy_ram[0x1F4 + 7] = 0;
    floppy_ram[0x1F4 + 8] = 0;
    floppy_ram[0x1F4 + 9] = 0;
    floppy_ram[0x1F4 + 10] = 0;
    floppy_ram[0x1F4 + 11] = 0;

    //floppy_motor_sounds(floppy_ram[TRACK]); //JD commented out

    DEBUG_LOG(0, "reading data for sector:%d", sectornumber);
    ptr = F->read_sector_data(F, sectornumber);
    if (!ptr)
    {
        ALERT_LOG(0, "Could not read sector #%ld ! \n", sectornumber);
        return;
    }
    // Copy datasize=512 bytes from the dc42 image into floppy_ram[], starting at location DISKDATASEC=512
    memcpy(&floppy_ram[DISKDATASEC], ptr, F->datasize);

    if (sectornumber == 0)
    {
        bootblockchecksum = 0;
        for (uint32 i = 0; i < F->datasize; i++)
            bootblockchecksum = ((uint32)(bootblockchecksum << 1) | ((uint32)(bootblockchecksum & 0x80000000) ? 1 : 0)) ^ (uint32)ptr[i] ^ i;
    }

    DEBUG_LOG(0, "reading tags for sector %d", sectornumber);
    ptr = F->read_sector_tags(F, sectornumber);
    if (ptr != NULL) {
        // Copy tagsize=12 bytes from the dc42 image into floppy_ram[], starting at location DISKDATAHDR=0x1f4=500
        memcpy(&floppy_ram[DISKDATAHDR], ptr, F->tagsize);
    }

    if (sectornumber == 0)
    {
        for (uint32 i = 0; i < F->tagsize; i++)
            bootblockchecksum = ((bootblockchecksum << 1) | ((bootblockchecksum & 0x80000000) ? 1 : 0)) ^ ptr[i] ^ i;

        ALERT_LOG(0, "Bootblock checksum:%08x", bootblockchecksum);

        if (bootblockchecksum == 0xce0cbba3 && macworks4mb)
            enable_4MB_macworks();
    }

    //        #ifdef DEBUG
    //        if (debug_log_enabled)  floppy_sec_dump(0,F,sectornumber,"read");
    //        if (debug_log_enabled)  floppy_sec_dump(1,F,sectornumber,"read");
    //        #endif

    DEBUG_LOG(0, "read floppy sector #%ld data at offset %ld size %d bytes", sectornumber,
              (sectornumber * F->sectorsize) + F->sectoroffset, F->sectorsize);

    DEBUG_LOG(0, "SRC:read sector #%ld from RAM (side:%d:trk:%d:sec:%d) tag:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
              sectornumber, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR],
              floppy_ram[0x1F4 + 0], floppy_ram[0x1F4 + 1], floppy_ram[0x1F4 + 2], floppy_ram[0x1F4 + 3], floppy_ram[0x1F4 + 4], floppy_ram[0x1F4 + 5],
              floppy_ram[0x1F4 + 6], floppy_ram[0x1F4 + 7], floppy_ram[0x1F4 + 8], floppy_ram[0x1F4 + 9], floppy_ram[0x1F4 + 10], floppy_ram[0x1F4 + 11]);

    total_num_sectors_read++;
}

// Note: the validation of side,track,sector, etc has been done by the caller. No need to do it again.
static void do_floppy_write(DC42ImageType *F)
{
    long sectornumber = getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
#ifdef DEBUG
    if (sectornumber != queuedsectornumber && queuedsectornumber > 0)
    {
        EXIT(1, 0, "Danger! Sector number is %ld but was %ld when first requested in do_floppy_write.", sectornumber, queuedsectornumber);
    }
#endif
    DEBUG_LOG(0, "write floppy sector #%ld", sectornumber);
    //floppy_motor_sounds(floppy_ram[TRACK]); //JD commented out
    F->write_sector_tags(F, sectornumber, &floppy_ram[DISKDATAHDR]);
    F->write_sector_data(F, sectornumber, &floppy_ram[DISKDATASEC]);
    total_num_sectors_written++;
}

/**
 * Check for a new command from the 68000 in it's shared memory command block, and then execute it.
 * When the command is completed, an IRQ is generated (for most, but not all commands).
 */
void floppy_go6504(void)
{
    // ALERT_LOG(0, "Starting in floppy_go6504() (drive/side/track/sector:%d / %d / %d / %d): floppy_6504_wait=%d\n", 
    //     floppy_ram[DRIVE], floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR], floppy_6504_wait);

    static uint8 slowdown;
    long sectornumber = 0;
    //    char *tempbuf;
    DC42ImageType *F = (floppy_ram[DRIVE]==0)? &current_upper_floppy_image:&current_lower_floppy_image;

    // char *DTCImage1Sig="SIGNATURE: DISK IMAGE FILE      VERSION: 1  ";
    // //                012345678901234567890123456789012345678901234
    //                           1         2         3         4
    int i, j, k;

    flop_cmd_text(buglog);
    if (floppy_6504_wait == 255)
    { // fprintf(buglog,"floppy controller in jail until reset\n");
        return;
    } // floppy controller in jail until hard machine reset (power off)

    MOTOR_STOP(0);

    if (!(floppy_ram[0x40]--))
        floppy_ram[0x41]--; // command counter

    i = floppy_ram[CONFIRM];
    j = floppy_ram[FUNCTION];
    k = floppy_ram[GOBYTE];

    if (k > 0x83)
        floppy_ram[0x2a] = floppy_ram[GOBYTE] - 0x83;

    // If we're formatting and confirmation isn't set, ignore it and don't say Ok by zeroing the go byte.
    if (k == FLOP_CTRLR_RWTS)
    {
        if ((j == FLOP_CMD_FMTTRK || j == FLOP_CMD_FORMAT) && i != 0xff)
        {
            ALERT_LOG(0, "Format Track reply 0xff not received yet, got %02x", i);
            return;
        }
    }

    if (floppy_6504_wait < 250)
    {
        slowdown = ((slowdown & SLOWDOWN) ? (slowdown & SLOWDOWN) - 1 : 0);
        if (k == FLOP_CTRLR_CLIS)
        {
            floppy_6504_wait = 1;
            slowdown = 0;
        } // go on through skip the wait cycle.
        if (k == FLOP_CTRLR_CLIM)
        {
            floppy_6504_wait = 1;
            slowdown = 0;
        } // go on through skip the wait cycle.
        if (k == FLOP_CTRLR_STIM)
        {
            floppy_6504_wait = 1;
            slowdown = 0;
        } // go on through skip the wait cycle.
        DEBUG_LOG(0, "floppy: wait state:%d\n", slowdown);
        if (slowdown)
        {
            floppy_6504_wait = 1;
            return;
        }
    }

    if (floppy_6504_wait == 254) // floppy controller is sleeping -
    {
        // fprintf(buglog,"floppy: in wait loop jail\n");
        // since the docs weren't too clear where 69 96 gets stored, I'm putting in several ways to bail
        // out of ROMWAIT...  whichever it turns out is correct will remain in the final code......
        if (floppy_last_macro == 0x96 && k == 0x69)
        {
            DEBUG_LOG(0, "6504 wakeup 1:Got decimal 96 69 seq in gobyte");
            floppy_6504_wait = 0;
            floppy_last_macro = k;
            return;
        }
        if (floppy_last_macro == 0x69 && k == 0x96)
        {
            DEBUG_LOG(0, "6504 wakeup 1:Got decimal 69 96 seq in gobyte");
            floppy_6504_wait = 0;
            floppy_last_macro = k;
            return;
        }

        // floppy_return(0);

        floppy_last_macro = k; // copy the new gobyte over the last one;
        return;
    }

    floppy_ram[GOBYTE] = 0; // signal the command as taken
    // cleanup
    floppy_ram[0x26] = floppy_ram[0x19];
    floppy_ram[0x27] = floppy_ram[0x1a];
    floppy_ram[0x42] = floppy_ram[0x17];
    if (floppy_ram[0x33])
        floppy_ram[0x33] = 32;
    floppy_ram[0x40] = 0;
    floppy_ram[0x41] = 0;
    floppy_ram[0x48 + 0] = 0;
    floppy_ram[0x48 + 1] = 0;
    floppy_ram[0x48 + 2] = 0;
    floppy_ram[0x48 + 3] = 0;
    floppy_ram[0x48 + 4] = 0;
    floppy_ram[0x48 + 5] = 0;
    floppy_ram[0x48 + 6] = 0;
    floppy_ram[0x48 + 7] = 0;
    floppy_ram[0x7b] = 0;

    floppy_last_macro = k; // copy the new gobyte over the last one;

#ifdef DEBUG
    if (floppy_ram[FLOP_PENDING_IRQ_FLAG] != 0 || fdir_timer != -1)
    {

        if (fdir_timer > cpu68k_clocks) // if the fdir timer has not yet expired, we don't expect any commands
        {                               // it's OK to get CLIS when fdir is already on however as code can poll for this.

            if (k == FLOP_CTRLR_CLIS) {
                // ALERT_LOG(0, "floppy: FLOP_CTRLR_CLIS %02x while IRQ pending to fire @%16llx\n", k, fdir_timer);
                snprintf(templine, 1023, "floppy: FLOP_CTRLR_CLIS %02x while IRQ pending to fire @%16llx\n", k, fdir_timer);
            } else {
                // ALERT_LOG(0, "floppy: DANGER unexpected floppy command %02x (%02x) while IRQ pending @%16lx\n", k, j, fdir_timer);
                snprintf(templine, 1023, "floppy: DANGER unexpected floppy command %02x (%02x) while IRQ pending @%16lx\n", k, j, fdir_timer);
            }
            DEBUG_LOG(0, templine);
            append_floppy_log(templine);
        }
    }
#endif

#ifdef IGNORE_FDIR_ON_COMMANDS
    // RWTS can't execute if there's a pending IRQ - return an error.
    if (k == FLOP_CTRLR_RWTS && (floppy_ram[FLOP_PENDING_IRQ_FLAG] != 0 || fdir_timer != -1))
    {
#ifdef DEBUG
        ALERT_LOG(0, "floppy: DANGER cannot execute cmd - IRQ pending.\n");
        append_floppy_log("floppy: DANGER cannot execute cmd - IRQ pending\n");
#endif

        floppy_ram[STATUS] = FLOP_STAT_IRQPND;
        return;
    }
#endif

    DEBUG_LOG(0, " SRC:gobyte:%02x, function:%02x drive:%02x gobyte@%d fn@%d side/trk/sec:%d/%d/%d\n",
              floppy_last_macro, floppy_ram[FUNCTION], floppy_ram[DRIVE], GOBYTE, FUNCTION,
              floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);

    floppy_ram[STATUS] = 0; // pre-emptive status good
    // floppy_ram[RETRY]=64;

    floppy_ram[0x1d0] = 0;
    floppy_ram[0x1d1] = 0;
    floppy_ram[0x1d2] = 0;
    floppy_ram[0x1d3] = 0;

    //    floppy_FDIR=0;                    // removing 2004.08.12 3:40am

    switch (floppy_last_macro)
    {
    case 0:
        return; // no command

    case FLOP_CTRLR_SHAKE:
        floppy_ram[STATUS] = 0; /* floppy_FDIR=0; removing 2004.08.12 3:40am */
        return;

    case FLOP_CTRLR_RWTS:  // 0x81  // execute the RWTS routine   // FALLTHROUGH
        floppy_ram[0x68] = 0; // disable brute force flag

#ifdef DEBUG
        memset(lastfloppyrwts, 0, 1024);
#endif
        /* FALLTHROUGH */
        switch (floppy_ram[FUNCTION])
        {

        case FLOP_CMD_READX:
            DEBUG_LOG(0, "brute force: ");
            floppy_ram[0x68] = 0xff; /* FALLTHRU */ // enable brute force flag

        case FLOP_CMD_READ: /* FALLTHRU */
            if (F == NULL || F->RAM == NULL)
            {
                DEBUG_LOG(0, "SRC:null fhandle\n");
                RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                floppy_FDIR = 1;
                return;
            }
            if (F->fd < 0 && F->fh == NULL)
            {
                DEBUG_LOG(0, "SRC:null fhandle\n");
                RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                floppy_FDIR = 1;
                return;
            }
            if (floppy_ram[TRACK] > F->maxtrk)
            {
                DEBUG_LOG(0, "SRC:track>max %d>%d\n", floppy_ram[TRACK], F->maxtrk);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVTRK);
                floppy_FDIR = 1;
                return;
            }
            if (floppy_ram[SECTOR] > F->maxsec)
            {
                DEBUG_LOG(0, "SRC:sect:%d>max %d\n", floppy_ram[SECTOR], F->maxsec);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC);
                floppy_FDIR = 1;
                return;
            }
            if (floppy_ram[SIDE] > F->maxside)
            {
                DEBUG_LOG(0, "SRC:side>max %d>%d\n", floppy_ram[SIDE], F->maxside);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVSID);
                floppy_FDIR = 1;
                return;
            }

            sectornumber = getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
            if (sectornumber == -1)
            {
                DEBUG_LOG(0, "Invalid sector %ld", sectornumber);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC);
                floppy_FDIR = 1;
                return;
            }

            queuedfn = floppy_ram[FUNCTION];
            queuedfn_drive = floppy_ram[DRIVE];

#ifdef DEBUG
            queuedsectornumber = sectornumber;
#endif
            snprintf(templine, 1023, "read sec #%ld PENDING (side:%d:trk:%d:sec:%d)\n",
                     sectornumber, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
            append_floppy_log("------------------------------------------------------------------");
            append_floppy_log(templine);

            FloppyIRQ(HUN_THOUSANDTH_OF_A_SEC);
            return;

        case FLOP_CMD_WRITX:
            DEBUG_LOG(0, "brute force write: ");
            floppy_ram[0x68] = 0xff; /* FALLTHRU */
        case FLOP_CMD_WRITE:         /* FALLTHRU */
            DEBUG_LOG(0, "SRC:Write\n");
            if (F == NULL || F->RAM == NULL)
            {
                DEBUG_LOG(0, "SRC:null fhandle\n");
                RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                floppy_FDIR = 1;
                return;
            }
            if (F->fd < 0 && F->fh == NULL)
            {
                DEBUG_LOG(0, "SRC:null fhandle\n");
                RWTS_IRQ_SIGNAL(FLOP_STAT_NODISK);
                floppy_FDIR = 1;
                return;
            }
            if (floppy_ram[TRACK] > F->maxtrk)
            {
                DEBUG_LOG(0, "SRC:track>max %d>%d\n", floppy_ram[TRACK], F->maxtrk);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVTRK);
                floppy_FDIR = 1;
                return;
            }
            if (floppy_ram[SECTOR] > F->maxsec)
            {
                DEBUG_LOG(0, "SRC:sect:%d>max %d\n", floppy_ram[SECTOR], F->maxsec);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC);
                floppy_FDIR = 1;
                return;
            }
            if (floppy_ram[SIDE] > F->maxside)
            {
                DEBUG_LOG(0, "SRC:side>max %d>%d\n", floppy_ram[SIDE], F->maxside);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVSID);
                floppy_FDIR = 1;
                return;
            }

            sectornumber = getsectornum(F, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
            if (sectornumber == -1)
            {
                DEBUG_LOG(0, "Invalid sector %ld", sectornumber);
                RWTS_IRQ_SIGNAL(FLOP_STAT_INVSEC);
                floppy_FDIR = 1;
                return;
            }

            queuedfn = floppy_ram[FUNCTION];
            queuedfn_drive = floppy_ram[DRIVE];

#ifdef DEBUG
            queuedsectornumber = sectornumber;
            snprintf(templine, 1024, "queing write to sector %ld (hd/trk/sec: %d/%d/%d)",
                     sectornumber, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
            append_floppy_log(templine);
#endif

            snprintf(templine, 1024, "write sec #%ld trk/sec/head:%d/%d/%d SRC:\n", sectornumber, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);
            append_floppy_log(templine);
#ifdef DEBUG
            snprintf(lastfloppyrwts, 1023, "write sec #%ld NOW(side:%d:trk:%d:sec:%d) tag:%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x ",
                     sectornumber, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR],
                     floppy_ram[0x1F4 + 0], floppy_ram[0x1F4 + 1], floppy_ram[0x1F4 + 2], floppy_ram[0x1F4 + 3], floppy_ram[0x1F4 + 4], floppy_ram[0x1F4 + 5],
                     floppy_ram[0x1F4 + 6], floppy_ram[0x1F4 + 7], floppy_ram[0x1F4 + 8], floppy_ram[0x1F4 + 9], floppy_ram[0x1F4 + 10], floppy_ram[0x1F4 + 11]);
#endif
            FloppyIRQ(HUN_THOUSANDTH_OF_A_SEC);
            return;

        case FLOP_CMD_CLAMP: // Lisa 1 generates this command to clamp the twiggy heads onto the disk. 
            DEBUG_LOG(0, "Floppy clamp queued on drive %02x \n", floppy_ram[DRIVE]);
            // Can (and does) the floppy disk controller recognize that there is no disk in the drive? We assume it does:
            uint8 clamp_return_status = (F->RAM == NULL || (F->fd < 0 && F->fh == NULL))? FLOP_STAT_NODISK:0; // or perhaps return FLOP_STAT_NOCLMP?
            DEBUG_LOG(0, "the FLOP_CMD_CLAMP for drive %02x return status is: %02x \n", floppy_ram[DRIVE], clamp_return_status);
            RWTS_IRQ_SIGNAL(clamp_return_status);
            floppy_FDIR = 1;
            return;

        case FLOP_CMD_UCLAMP: // eject/close/unclamp disk image
            DEBUG_LOG(0, "Floppy eject queued on drive %02x \n", floppy_ram[DRIVE]);
            if (F->close_image) // ensure function pointer is valid before calling it
                F->close_image(F);
            floppy_ram[0x20] = 0;
            RWTS_IRQ_SIGNAL(0);
            floppy_FDIR = 1;
#ifdef DEBUG
            snprintf(lastfloppyrwts, 1023, "Eject NOW");
#endif
            // Do not animate ejecting the bottom drive 2 in Lisa 1 mode, because there is no "skin" for it:
            if((floppy_iorom == 0x40 && floppy_ram[DRIVE] == 0x00) || (floppy_iorom != 0x40)) 
                eject_floppy_animation();

            return;

        case FLOP_CMD_FMTTRK:
            DEBUG_LOG(0, "Format track.");
            /* FALLTHRU */ // ignore the rest o'this
        case FLOP_CMD_FORMAT:
            DEBUG_LOG(0, "Format whole disk.");
            /* FALLTHRU */ // format a track (erase all it's sectors!)

        case FLOP_CMD_VERIFY:
            /* FALLTHRU */
        case FLOP_CMD_VFYTRK:
            floppy_FDIR = 1; // was missing from pre RC2!
            RWTS_IRQ_SIGNAL(0);
            DEBUG_LOG(0, "...")
            return;
        }
        /* FALLTHRU */
    case FLOP_CTRLR_SEEK:
        // fprintf(buglog,"SRC:seek\n");
        /// RWTS_IRQ_SIGNAL(F, 0);  //2005.02.04 - maybe we don't need to wait on fdir?
        floppy_ram[SPEED] = 0;
        floppy_ram[STATUS] = 0;

        return; // NO FDIR here charlie!
        break;  // this is fucking bullshit!

    case FLOP_CTRLR_JSR: // trap these and complain loudly of 6504 usage. //
        ALERT_LOG(0, "SRC:The Lisa is trying to tell the 6504 floppy controller to execute code. (%08x) JSR %02x%02x  This is not always supported by the emulator!\n",
                  floppy_ram[0], floppy_ram[2], floppy_ram[1]);

        if (floppy_ram[0x200] == 0xa0 && // 0200   A0 00      LDY #$00
            floppy_ram[0x201] == 0x00 &&
            floppy_ram[0x202] == 0x88 && // 0202   88         DEY  FF FE
            floppy_ram[0x203] == 0xc8 && // 0203   C8         INY  00 00
            floppy_ram[0x204] == 0x88 && // 0204   88         DEY  FF FE
            floppy_ram[0x205] == 0xd0 && // 0205   D0 FB      BNE $0202   // loop around 256 times, Y=0
            floppy_ram[0x206] == 0xfb &&
            floppy_ram[0x207] == 0x88 && // 0207   88         DEY         // this should now be FF?
            floppy_ram[0x208] == 0x84 && // 0208   84 00      STY $00     // store FF in zero?
            floppy_ram[0x209] == 0x00 &&
            floppy_ram[0x20a] == 0xa5 && // 020A   A5 00      LDA $00     // loop forever? wtf? why?
            floppy_ram[0x20b] == 0x00 &&
            floppy_ram[0x20c] == 0xd0 && // 020C   D0 FC      BNE $020A   // wonder if it will later modify this code after this piece executes.
            floppy_ram[0x20d] == 0xfc &&
            floppy_ram[0x20e] == 0x60) // 020E   60         RTS
        {
#ifdef DEBUG
// debug_on("MacWorks+II forever loop");
#endif
            floppy_ram[0] = 0xff;
            reg68k_regs[1] = 0x1ff;
            ALERT_LOG(0, "MW+II 6504 timing/go-away-loop part 1 completed.");
            return;
        }
        // hack to allow MacWorks XL 3.0 code to run - think that this reads the
        // write protect tab of the current floppy, not 100% sure.
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (floppy_ram[0x200] == 0x20 && // JSR 1320 for I/O 88 ROM, JSR 1302 for I/O A8 ROM.
            (floppy_ram[0x201] == 0x20 || floppy_ram[0x201] == 0x02) &&
            floppy_ram[0x202] == 0x13 && //
            floppy_ram[0x203] == 0xa9 && // lda  #$00
            floppy_ram[0x204] == 0x00 && //
            floppy_ram[0x205] == 0xaa && // tax
            floppy_ram[0x206] == 0xb0 && // bcs   +1  // skip 1 opcode past dex, (execute STX)
            floppy_ram[0x207] == 0x01 && //
            floppy_ram[0x208] == 0xca && // dex
            floppy_ram[0x209] == 0x86 && // stx $08   // fcd0011 - status (0 or ff)
            floppy_ram[0x20a] == 0x08 && //
            floppy_ram[0x20b] == 0x85 && // sta $06   // fcd000d - speed - signal end
            floppy_ram[0x20c] == 0x06 && //
            floppy_ram[0x20d] == 0x60)   // rts
        {
            // if we want to signal a write protected disk, return 0xff in status instead.
            floppy_ram[SPEED] = 0;
            floppy_ram[STATUS] = 0;
            ALERT_LOG(0, "6504 write-protect-jsr executed");
            return;
        }

        // lisatest bus error test
        if (floppy_ram[0x01f4] == 0x8d && //  01f4 8d 19 04: sta $0419  //lisatest  // set DIS
            floppy_ram[0x01f5] == 0x19 && //  01f7 a2 ff:    ldx #$ff               // wait
            floppy_ram[0x01f6] == 0x04 && //  01f9 ca:       dex                    //
            floppy_ram[0x01f7] == 0xa2 && //  01fa d0 fd:    bne $1f9               //
            floppy_ram[0x01f8] == 0xff && //  01fc 8d 18 04: sta $0418              // clear DIS
            floppy_ram[0x01f9] == 0xca && //  01ff 60:       rts                    // return
            floppy_ram[0x01fa] == 0xd0 &&
            floppy_ram[0x01fb] == 0xfd &&
            floppy_ram[0x01fc] == 0x8d &&
            floppy_ram[0x01fd] == 0x18 &&
            floppy_ram[0x01fe] == 0x04 &&
            floppy_ram[0x01ff] == 0x60)
        {
            ALERT_LOG(0, "6504 LisaTest Disk Test #11 executed - it might be wanting to set a bus error");

            /*
              4402869 1/0000d11a (lisacx 0 0/0/0) opcode=4a29 TST.B      $0001(A1)    SRC:clk:845889594:(diff:8)
              4402870 memory.c:lisa_rb_io:2963:@00fcc001

            */
            lisa_buserror(0xfcc001);
            return;
        }

        ALERT_LOG(0, "Attempt to execute unrecognized 6504 code");
#ifdef DEBUG
        debug_on("Unrecognized 6504 code");
#endif
        {
            int i;
            for (i = 0; i < 2048; i += 16)
            {
                ALERT_LOG(0, "%04x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", i,
                          floppy_ram[i + 0], floppy_ram[i + 1], floppy_ram[i + 2], floppy_ram[i + 3],
                          floppy_ram[i + 4], floppy_ram[i + 5], floppy_ram[i + 6], floppy_ram[i + 7],
                          floppy_ram[i + 8], floppy_ram[i + 9], floppy_ram[i + 10], floppy_ram[i + 11],
                          floppy_ram[i + 12], floppy_ram[i + 13], floppy_ram[i + 14], floppy_ram[i + 15]);
            }
        }

        RWTS_IRQ_SIGNAL(FLOP_STAT_INVCMD);
        EXIT(84, 1, "Attempt to execute unrecognized 6504 code.  Emulation failed.");
        return;

    case FLOP_CTRLR_CLIM:

        floppy_ram[FLOP_INT_MASK] &= (floppy_ram[FUNCTION] ^ 0xff); // clear mask bits
        floppy_ram[FLOP_INT_STAT] &= (floppy_ram[FUNCTION] ^ 0xff); // clear current state as well
        // Note(TorZidan@): The line below writes to the same address as the line above, essentially overwriting the line above. TODO: clean this up.
        floppy_ram[FLOP_PENDING_IRQ_FLAG] = 0;
        floppy_return(0);
        floppy_6504_wait = 0;

        if (!floppy_ram[FLOP_INT_STAT])
        {
            floppy_FDIR = 0;
            fdir_timer = -1;
        } /* keeping this one 2004.08.12 3:40am */
        return;

    case FLOP_CTRLR_STIM: // 86 : Set interrupt mask // drive enable interrupt
        floppy_ram[FLOP_INT_MASK] |= (floppy_ram[FUNCTION]);
        floppy_ram[FLOP_PENDING_IRQ_FLAG] = 0;

        // TODO(TorZidan): it seems that variables floppy_irq_top and floppy_irq_bottom are named wrongly; they shoould be swapped.
        // Note that 0x00 is the top floppy drive, and 0x80 is the bottom floppy drive. 
        // For Twiggy floppy images we see in the log: "SRC:floppy F IRQ enabled on FDIR fn(08) irqtop:0 irqbot:1:: MASK IS NOW:08"
        // For Sony   floppy images we see in the log: "SRC:floppy F IRQ enabled on FDIR fn(80) irqtop:1 irqbot:0:: MASK IS NOW:80"
        // See their use in ir.c
        floppy_irq_top = ((floppy_ram[FUNCTION] & 0x80) ? 1 : 0);
        floppy_irq_bottom = ((floppy_ram[FUNCTION] & 0x08) ? 1 : 0);
        DEBUG_LOG(0, "SRC:floppy F IRQ enabled on FDIR fn(%02x) irqtop:%d irqbot:%d:: MASK IS NOW:%02x\n",
                  floppy_ram[FUNCTION], floppy_irq_top, floppy_irq_bottom, floppy_ram[FLOP_INT_MASK]);

        floppy_return(0); // no call to RWTS_IRQ_SIGNAL() since NO IRQ is issued here.
        floppy_6504_wait = 0;
        // if (!floppy_ram[FLOP_INT_STAT]) floppy_FDIR=0;
        floppy_FDIR = 0;
        fdir_timer = -1; /* keeping 2004.08.12 3:40am */
        return;

    case FLOP_CTRLR_CLIS:

        floppy_ram[FLOP_INT_STAT] &= (floppy_ram[0x01] ^ 0xff);

        // clear IRQ if the low bits are cleared
        if ((floppy_ram[FLOP_INT_STAT] & 0x0f) == 0x08)
            floppy_ram[FLOP_INT_STAT] &= 0xf0;
        if ((floppy_ram[FLOP_INT_STAT] & 0xf0) == 0x80)
            floppy_ram[FLOP_INT_STAT] &= 0x0f;

        floppy_ram[0x2e] = 0;
        DEBUG_LOG(0, "SRC:floppy Clear IRQ Status(%02x) result written to post fixintstat IRQSTAT:%02x\n",
                  floppy_ram[FUNCTION], floppy_ram[FLOP_INT_STAT]);

        floppy_ram[STATUS] = 0; // 20070723// buggy bug?
        floppy_return(0);
        floppy_6504_wait = 0;
        // if (!floppy_ram[FLOP_INT_STAT]) floppy_FDIR=0;  //commented out 20060607 21:28
        floppy_FDIR = 0;
        fdir_timer = -1;
        return;

    case FLOP_CTRLR_WAIT: // 88  Wait in ROM until cold start
        DEBUG_LOG(0, "Floppy controller in 6996 wait jail");
        floppy_6504_wait = 254;
        floppy_return(0);
        return;

    case FLOP_CTRLR_LOOP: // 89    Loop in ROM forever
        fflush(buglog);
        floppy_6504_wait = 255;
        DEBUG_LOG(0, "Floppy controller in RESET wait jail");
        floppy_return(0);

        DEBUG_LOG(0, "Lisa is about to power off - floppy controller told to go away");

        return;
    default:
        ALERT_LOG(0, "SRC:unrecognized command GOBYTE:%08x, FUNCTION:%08x\n", floppy_last_macro, floppy_ram[FUNCTION]);
        // RWTS_IRQ_SIGNAL(FLOP_STAT_INVCMD);
    }
}

void hexprint(char *x, int size, int ascii_print)
{
    int i, half;
    unsigned char c;
    char ascii[130];
    half = (size / 2) - 1;
    if (size > 128)
    {
        fprintf(buglog, "SRC:hexprintf given illegal size %d\n", size);
        return;
    };
    memset(ascii, 0, 130);
    for (i = 0; i < size; i++)
    {
        c = x[i];

        if (i == half)
            printf("%02x . ", c);
        else
            printf("%02x ", c);

        if (ascii_print)
        {
            if (c > 126)
                c &= 127;
            if (c < 31)
                c |= 32;
            ascii[i] = c;
        }
    }
    if (size < 16)
        while (16 - size)
        {
            printf("   ");
            size++;
        }
    if (ascii_print)
        printf("  |  %s\n", ascii);
}

/**********************************************************************************\
*  Routine to reset the return values of the ram block for the lisa.               *
*                                                                                  *
*                                                                                  *
\**********************************************************************************/

void floppy_return(uint8 status)
{
    floppy_ram[GOBYTE] = 0;
    floppy_ram[STATUS] = status; // it passed the self tests.
    // floppy_ram[ROMVER]=FLOPPY_ROM_VERSION;  // handled in the C++ code now!

    if ((floppy_ram[ROMVER] & 96) == 32)
        via_clock_diff = 2; // via_clock_diff=10;  // slow VIA clock
    else
        via_clock_diff = 5; // via_clock_diff=4;

    MOTOR_SPIN(0);

    floppy_ram[CURRENT_TRACK] = floppy_ram[TRACK];
    floppy_ram[CURRENT_TRACK_GRP] = floppy_ram[TRACK] >> 4;

    floppy_ram[SPEED] = floppy_ram[0x10 + (floppy_ram[CURRENT_TRACK_GRP])];

    floppy_ram[DRIVE_EXISTS] = 0xff;

    floppy_ram[0x26] = 0;
    floppy_ram[0x27] = 0;
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
    *plant = ((serialnum240[0x02] & 0x0f) << 4) | (serialnum240[0x03] & 0x0f);
    *year = ((serialnum240[0x04] & 0x0f) << 4) | (serialnum240[0x05] & 0x0f);
    *day = ((serialnum240[0x06] & 0x0f) << 8) | ((serialnum240[0x07] & 0x0f) << 4) | (serialnum240[0x08] & 0x0f);

    *sn = ((serialnum240[0x09] & 0x0f) << 12) | ((serialnum240[0x0a] & 0x0f) << 8) | ((serialnum240[0x0b] << 4) & 0x0f) |
          (serialnum240[0x0c] & 0x0f);
    // applenet
    *prefix = ((serialnum240[0x10] & 0x0f) << 8) | ((serialnum240[0x11] & 0x0f) << 4) | (serialnum240[0x12] & 0x0f);
    *net = ((serialnum240[0x13] & 0x0f) << 16) | ((serialnum240[0x14] & 0x0f) << 12) | ((serialnum240[0x15] & 0x0f) << 8) |
           ((serialnum240[0x16] & 0x0f) << 4) | (serialnum240[0x17] & 0x0f);

    ALERT_LOG(0, "serial240: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
              serialnum240[0], serialnum240[1], serialnum240[2], serialnum240[3],
              serialnum240[4], serialnum240[5], serialnum240[6], serialnum240[7],
              serialnum240[8], serialnum240[9], serialnum240[10], serialnum240[11],
              serialnum240[12], serialnum240[13], serialnum240[14], serialnum240[15]);
    ALERT_LOG(0, "serial250: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
              serialnum240[16], serialnum240[17], serialnum240[18], serialnum240[19],
              serialnum240[20], serialnum240[21], serialnum240[22], serialnum240[23],
              serialnum240[24], serialnum240[25], serialnum240[26], serialnum240[27],
              serialnum240[28], serialnum240[29], serialnum240[30], serialnum240[31]);

    ALERT_LOG(0, "Lisa SN: plant:%x born on year:0x%x (%d) day:0x%x (%d) sn:0x%x (%d) applenet %x-%x",
              *plant, *year, *year, *day, *day,
              *sn, *sn, *prefix, *net);
}

void deserialize(DC42ImageType *F)
{
    // The MDF sector number is 41 on Twiggy floppy images, and 28 on Sony:
    uint32 mdf_sector_num = (F->ftype==0)? 41:28;

    if (F->numblocks<=mdf_sector_num) {
        ALERT_LOG(0, "numblocks=%d in the disk image is below the desired %d; skipping deserialize()\n", 
            F->numblocks, mdf_sector_num-1);
        return;
    }
    uint32 plant, year, day, sn, prefix, net;
    get_lisa_serialnumber(&plant, &year, &day, &sn, &prefix, &net);

    uint8 *mddftag = F->read_sector_tags(F, mdf_sector_num);
    if (F->retval!=0) {
        ALERT_LOG(0,"There was a problem reading tags at sector %d: %s. Will not attempt to deseriaize this floppy disk image.", mdf_sector_num, F->errormsg);
        return;
    }

    uint8 *mddfsec = F->read_sector_data(F, mdf_sector_num);
    if (F->retval!=0) {
        ALERT_LOG(0,"There was a problem reading data at sector %d: %s. Will not attempt to deseriaize this floppy disk image.", mdf_sector_num, F->errormsg);
        return;
    }

    //         ALERT_LOG(0,"Disk Serial #%02x%02x%02x%02x Lisa Serial#%02x%02x%02x%02x",
    //             mddfsec[0xcc], mddfsec[0xcd], mddfsec[0xce], mddfsec[0xcf],
    //             serialnum240[8],serialnum240[9],serialnum240[10],serialnum240[11]);

    // Remove Lisa signature on previously used Serialized Master Disks //////////////////////////////////////////////
    if (mddftag[4] == 0 && mddftag[5] == 1 && // MDDF Signature
        mddfsec[0x0d] == 'O' &&
        mddfsec[0x0e] == 'f' &&
        mddfsec[0x0f] == 'f' &&
        mddfsec[0x10] == 'i' &&
        mddfsec[0x11] == 'c' &&
        mddfsec[0x12] == 'e' &&
        mddfsec[0x13] == ' ' &&
        mddfsec[0x14] == 'S' &&
        mddfsec[0x15] == 'y' &&
        mddfsec[0x16] == 's' &&
        mddfsec[0x17] == 't' &&
        mddfsec[0x18] == 'e' &&
        mddfsec[0x19] == 'm')
    {

        uint32 disk_sn = (mddfsec[0xcc] << 24) | (mddfsec[0xcd] << 16) | (mddfsec[0xce] << 8) | (mddfsec[0xcf]);

        ALERT_LOG(0, "This Lisa Office System installation floppy disk was signed by Lisa SN: %08x (%d)", disk_sn, disk_sn);

        if (disk_sn != sn && disk_sn != 0)
        {
            {
                uint8 buf[512];
                memcpy(buf, mddfsec, 512);
                buf[0xcc] = buf[0xcd] = buf[0xce] = buf[0xcf] = 0;
                F->write_sector_data(F, 28, buf);
            }
        }
    } else {
        ALERT_LOG(0, "Not a Lisa Office System installation floppy disk. Will not attempt to deserialize it.");
    }

    ////   Deserialize Lisa Office System Tools on diskettes //////////////////////////////////////////////////////////////////////////////////////
    uint8 *ftag; //=F->read_sector_tags(F,28);
    uint8 *fsec; //=F->read_sector_data(F,28);
    int sec;
    for (sec = 32; sec < 128; sec++)
    {
        char name[64];
        ftag = F->read_sector_tags(F, sec);
        //               ALERT_LOG(0,"Checking sector:%d fileid4=%02x",sec,ftag[4]);
        if (F->retval!=0) {
            ALERT_LOG(0,"There was a problem reading tags at sector %d: %s. Will not attempt to deseriaize this floppy disk image.", sec, F->errormsg);
            return;
        }

        if (ftag[4] == 0xff) // tool entry tags have tag 4 as ff, others do as well, but it's a good indicator
        {
            fsec = F->read_sector_data(F, sec);
            if (F->retval!=0) {
                ALERT_LOG(0,"There was a problem reading data at sector %d: %s. Will not attempt to deseriaize this floppy disk image.", sec, F->errormsg);
                return;
            }
            int s = fsec[0]; // size of string (pascal string)
            // possible file name, very likely to be the right size.
            // Look for {T*}obj.  i.e. {T5}obj is LisaList, but could have {T9999}obj but very unlikely
            // also check the friendly tool name size at offset 0x182.

            if (s > 6 && s < 32 && fsec[1] == '{' && fsec[2] == 'T' && fsec[3] >= '0' && fsec[3] <= '9' &&
                fsec[s - 3] == '}' && tolower(fsec[s - 2]) == 'o' && tolower(fsec[s - 1]) == 'b' && tolower(fsec[s]) == 'j' && fsec[0x182] < 64)
            {
                uint32 toolsn = (fsec[0x42] << 24) | (fsec[0x43] << 16) | (fsec[0x44] << 8) | fsec[0x45];
                if (toolsn != sn)
                {
                    s = fsec[0x182];               // Size of tool name
                    memcpy(name, &fsec[0x183], s); // copy it over.
                    name[s] = 0;                   // string terminator.

                    char message[256];
                    snprintf(message, 256,
                             "Found Office System tool %s serialized for Lisa #%02x%02x%02x%02x.  Your virtual Lisa is #%02x%02x%02x%02x, which may prevent installation.  De-Serialize this tool?",
                             name,
                             fsec[0x42], fsec[0x43], fsec[0x44], fsec[0x45],
                             serialnum240[8],
                             serialnum240[9],
                             serialnum240[10],
                             serialnum240[11]);

                    if (yesnomessagebox(message, "De-Serialize this Tool?"))
                    {
                        uint8 buf[512];
                        memcpy(buf, fsec, 512);
                        buf[0x42] = buf[0x43] = buf[0x44] = buf[0x45] = buf[0x48] = buf[0x49] = 0;
                        F->write_sector_data(F, sec, buf);
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// insert_floppy
int floppy_insert(char *Image, uint8 insert_in_upper_floppy_drive) // emulator should call this when user decides to open disk image...
{
    DC42ImageType *F = (insert_in_upper_floppy_drive) ? &current_upper_floppy_image:&current_lower_floppy_image;
    int err = 0;

    DEBUG_LOG(0, "Inserting [%s] floppy", Image);
    DEBUG_LOG(0, "MAX RAM:%08x MINRAM:%08x TOTRAM:%08X BADRAMID:%02x SYSTEMTYPE:%02x 0=lisa1, 1=lisa2, 2=lisa2+profile 3=lisa2+widget ramchkbitmap:%04x",
              fetchlong(0x294),
              fetchlong(0x2a4),
              fetchlong(0x2a8),
              fetchbyte(0x2ad), fetchbyte(0x2af),
              fetchword(0x184));


    append_floppy_log("Inserting floppy:");
    append_floppy_log(Image);

    strncpy(F->fname, Image, 255);
    // fprintf(buglog,"SRC:Opening Floppy Image file... %s\n",F->fname);
    errno = 0;

    if (F->close_image) // ensure function pointer is valid before calling it
        F->close_image(F); // close any previously opened disk image

    err = dc42_auto_open(F, Image, "wb"); // for testing the emulator, open images as private "p"  w=writeable, b=best
    if (err)
    {
        ALERT_LOG(0, "could not open: %s because %s", Image, F->errormsg);
        messagebox(F->errormsg, "Could not open this Floppy! Sorry!");
        append_floppy_log("Could not open floppy");
        perror("error");
        return -1;
    }

    err = dc42_check_checksums(F); // 0 if they match, 1 if tags, 2 if data, 3 if both don't match
    switch (err)
    {
    case 1:
        messagebox("The Tag checksum failed for this disk.  It may be corrupted.", "Danger!");
        break;
    case 2:
        messagebox("The Data checksum failed for this disk.  It may be corrupted.", "Danger!");
        break;
    case 3:
        messagebox("Both the Data and Tag checksums failed for this disk!", "Danger!");
        break;
    default:
    case 0:;
    }

    if (F->numblocks == 1440 || F->numblocks == 2880 || F->numblocks == 5760)
    {
        messagebox("Yuck! I can't swallow MFM (720K/1.44M/2.88M) floppies, those are for PC's", "Wrong kind of floppy");
        if (F->close_image) // ensure function pointer is valid before calling it
            F->close_image(F);
        return -1;
    }

    if (F->numblocks > 1742)
    {
        messagebox("That's a very strangely sized disk!", "Wrong kind of floppy");
        if (F->close_image) // ensure function pointer is valid before calling it
            F->close_image(F);
        return -1;
    }

    deserialize(F);

    if (floppy_ram[ROMVER]!=0x40 && F->ftype==0) 
    {
        char message[] = "You are trying to use a Twiggy floppy image file in Lisa 2 mode, but it is intended to be used in Lisa 1 mode, with Lisa 1 software. This may not end well.\n\n"
            "Tips for booting from a Twiggy floppy disk image:\n"
            "  - In the File->Preferences menu, switch to Lisa 1 mode by choosing I/O ROM version 40 and any Lisa ROM file. "
            "Apply and restart the emulator.\n"
            "  - Insert a Twiggy diskette in the upper drive 1 (File->Insert diskette).\n"
            "  - At the Lisa boot screen, click on the upper floppy drive 1 icon to boot from it."; 
        messagebox(message, "This may not end well :(");
    }
    else if (floppy_ram[ROMVER]==0x40 && F->ftype!=0) 
    {
        char message[] = "You are trying to use a Sony floppy image file in Lisa 1 mode, but it is intended to be used in Lisa 2 mode, with Lisa 2 software. This may not end well.\n\n"
            "Tips for booting from a Sony floppy disk image:\n"
            "  - In the File->Preferences menu, switch to Lisa 2 mode by choosing any I/O ROM version EXCEPT version 40, and a ROM file version H or 3A. "
            "Apply and restart the emulator.\n"
            "  - Insert a Sony diskette (File->Insert diskette).\n"
            "  - At the Lisa boot screen, click on the floppy drive icon to boot from it."; 
        messagebox(message, "This may not end well :(");
    }

    if (insert_in_upper_floppy_drive)
    {
        DEBUG_LOG(0, "Inserting a floppy disk image in the upper drive 1 (with drive number 0x00) \n");
        floppy_ram[FLOP_INT_STAT] |= (FLOP_IRQ_SRC_DSKIN1 | FLOP_IRQ_SRC_DRV1);
    }
    else
    {
        DEBUG_LOG(0, "Inserting a floppy disk image in the lower drive 2 (or the only drive in Lisa 2 mode) (with drive number 0x80) \n");
        floppy_ram[FLOP_INT_STAT] |= (FLOP_IRQ_SRC_DSKIN2 | FLOP_IRQ_SRC_DRV2);
    }
    fix_intstat_if_needed();
    floppy_ram[0x20] = 0xff;

    FloppyIRQ(HUN_THOUSANDTH_OF_A_SEC);
    floppy_FDIR = 1;
    // Note: do not call floppy_return(0) because we are not executing a command.
    return 0;
}

// Set IRQ upon RWTS completion. Won't fire any IRQ if the interrupt mask isn't right.
void RWTS_IRQ_SIGNAL(uint8 status) {
    if (floppy_ram[DRIVE] == 0x00)
    {
        floppy_ram[FLOP_INT_STAT] |= FLOP_IRQ_SRC_RWTS1; // Set this bit
    }
    else if (floppy_ram[DRIVE] == 0x80)
    {
        floppy_ram[FLOP_INT_STAT] |= FLOP_IRQ_SRC_RWTS2; // Set this bit
    }
    else
    {
        ALERT_LOG(0, "Invalid drive: %02x !\n");
        status = FLOP_STAT_INVDRV;
    }
    fix_intstat_if_needed();

    floppy_ram[0x2e] = floppy_ram[FLOP_INT_MASK] & floppy_ram[FLOP_INT_STAT];

    if (floppy_ram[0x2e])
    {
        DEBUG_LOG(0, "Will be Firing IRQ1 after delay since INTSTAT is %02x & INTMASK is %02x, 2e is :%02x\n",
                  floppy_ram[FLOP_INT_STAT], floppy_ram[FLOP_INT_MASK], floppy_ram[0x2e]);
        FloppyIRQ(HUN_THOUSANDTH_OF_A_SEC);
    }
    else
    {
       ALERT_LOG(0, "Will not be firing IRQ1 because: INTSTAT is %02x & INTMASK is %02x, 2e is zero :%02x\n",
                  floppy_ram[FLOP_INT_STAT], floppy_ram[FLOP_INT_MASK], floppy_ram[0x2e]);
    }

    if (floppy_6504_wait < 250)
        floppy_6504_wait = 0; // shut down diag wait
    
    floppy_return(status);
}

/**
 * Sets bit 3 of floppy_ram[FLOP_INT_STAT], if any of bits 0,1,2 are set.
 * Also, sets bit 7 if any of bits 4,5,6 are set.
 */
void fix_intstat_if_needed()
{
    uint8 intstat = floppy_ram[FLOP_INT_STAT];
    // correct IRQ status if needed.
    intstat &= 0x77;
    intstat |= ((intstat & 0x70) ? 0x80 : 0) | ((intstat & 0x07) ? 0x08 : 0);
    floppy_ram[FLOP_INT_STAT] = intstat;
}

/**
 * This is called from lisaem_wx.cpp when the user pressed the floppy eject button.
 * It fires a Floppy IRQ with the proper "floppy eject requested" bit set.
 * The Lisa software intercepts the IRQ, sends a "floppy eject" command FLOP_CMD_UCLAMP,
 * and we process it (elsewhere in floppy.c) and eject the floppy.
 * Note: Lisa may refuse to do anything with this IRQ, and hence it will not eject the floppy. This is evident at the BIOS boot menu screen.
 * Note: there is another way to eject a floppy: if we are booted into e.g. LOS3.1, we can eject a floppy using Lisa's menu.
 */
void floppy_eject_button_pressed(uint8 on_upper_floppy_drive)
{
    if (on_upper_floppy_drive) 
    {
        floppy_ram[FLOP_INT_STAT] |= FLOP_IRQ_SRC_BTN1;
    }
    else
    {
        floppy_ram[FLOP_INT_STAT] |= FLOP_IRQ_SRC_BTN2;
    }
    fix_intstat_if_needed();
    // Note: do not call floppy_return(0) because we are not executing any command.
    FloppyIRQ(HUN_THOUSANDTH_OF_A_SEC);
    floppy_FDIR = 1;
}

void init_floppy(long iorom)
{
    memset(&current_upper_floppy_image, 0, sizeof(DC42ImageType));
    memset(&current_lower_floppy_image, 0, sizeof(DC42ImageType));

    floppy_FDIR = 0;
    fdir_timer = -1;
    floppy_6504_wait = 0;

    total_num_sectors_read = 0;
    total_num_sectors_written = 0;

    // memset(floppy_ram,0,1024);             // clear ram
    //  load_pram();                          // obsolete - let Lisaconfig handle this

    // setup various stuff found in floppy mem, not sure what all of these are,
    // but set them incase the 68k expects them...

    floppy_ram[0x06] = 0x00;
    floppy_ram[0x08] = 0x00;
    floppy_ram[0x09] = 0x01;
    floppy_ram[0x0a] = 0x01;
    floppy_ram[0x0c] = 0x00;

    floppy_ram[0x10] = 0xD5;
    floppy_ram[0x11] = 0xC0;
    floppy_ram[0x12] = 0xA7;
    floppy_ram[0x13] = 0x89;
    floppy_ram[0x14] = 0x64;
    floppy_ram[0x15] = 0x1E;
    floppy_ram[0x16] = 0x04;
    floppy_ram[0x17] = 0x09;
    floppy_ram[0x18] = iorom & 0xff; // FLOPPY_ROM_VERSION now obsolete, handled by C++ code now.

    floppy_ram[0x19] = 0x64;
    floppy_ram[0x1a] = 0x02;
    floppy_ram[0x1b] = 0x82;
    floppy_ram[0x1c] = 0x4F;
    floppy_ram[0x1d] = 0x0c;

    floppy_ram[0x20] = 0x00;
    floppy_ram[0x21] = 0xff;
    floppy_ram[0x24] = 0xff;
    floppy_ram[0x25] = 0x02;

    floppy_ram[0x38] = 0xD5;
    floppy_ram[0x39] = 0xAA;
    floppy_ram[0x3a] = 0x96;
    floppy_ram[0x3b] = 0xDE;
    floppy_ram[0x3c] = 0xAA;

    floppy_ram[0x46] = 0xff;

    floppy_ram[0x70] = 0xD5;
    floppy_ram[0x71] = 0xAA;
    floppy_ram[0x72] = 0xAD;
    floppy_ram[0x73] = 0xDE;
    floppy_ram[0x74] = 0xAA;

    if (iorom != 0x40)
        floppy_ram[TYPE] = (double_sided_floppy) ? SONY800KFLOPPY : SONY400KFLOPPY;
    else
        floppy_ram[TYPE] = TWIGGYFLOPPY;
}

uint8 is_upper_floppy_currently_inserted() {
    return is_floppy_image_currently_inserted(&current_upper_floppy_image);
}

uint8 is_lower_floppy_currently_inserted() {
    return is_floppy_image_currently_inserted(&current_lower_floppy_image);
}

uint8 is_floppy_image_currently_inserted(DC42ImageType *F) {
    return (F->RAM == NULL)? 0:1;
}

void sector_checksum(void)
{
    uint16 a, atmp, x, y, c, nc, m43 = 0;
#define CARRY(x)                \
    {                           \
        c = (a < 0xff ? 1 : 0); \
        a &= 0xff;              \
    }
#define ASL                     \
    {                           \
        a = ((a << 1) | c);     \
        c = (a < 0xff ? 1 : 0); \
        a &= 0xff;              \
    }
#define ADC(x)                  \
    {                           \
        a += (x) + c;           \
        c = (a < 0xff ? 1 : 0); \
        a &= 0xff;              \
    }
#define INX        \
    {              \
        x++;       \
        x &= 0xff; \
    }
#define INY        \
    {              \
        y++;       \
        y &= 0xff; \
    }
#define TAX    \
    {          \
        x = a; \
    }
#define TXA    \
    {          \
        a = x; \
    }

    a = 0;
    c = 0;
    nc = 0; // lsb,msb (m43,m44)
    floppy_ram[0x64] = 0;
    floppy_ram[0x65] = 0;
    floppy_ram[0x43] = 0;
    floppy_ram[0x44] = 0;
    m43 = 0x100; // 1937
    y = 0xf4;    // 1939

x193b:
    ASL;                     // 193b
    atmp = a;                // 193c
    ADC(0x00);               // 193d
    floppy_ram[0x64] = a;    // 193f
    a = atmp;                // 1941
    a = floppy_ram[m43 + y]; // 1942
    TAX;                     // 1944
    a ^= floppy_ram[0x63];   // 1945
    // floppy_ram[m43+y]=a;                              //1947
    TXA;                   // 1949
    ADC(floppy_ram[0x61]); // 194a
    floppy_ram[0x61] = a;  // 194c
    INY;                   // 194e
    if (y)
        goto x1953; // bne                      //194f
    m43 += 0x100;   // inc $44                  //1951
x1953:
    a = floppy_ram[m43 + y]; // 1953
    TAX;                     // 1955
    ADC(floppy_ram[0x62]);   // 1956
    floppy_ram[0x62] = a;    // 1958
    TXA;                     // 195a
    a ^= floppy_ram[0x61];   // 195b
    // floppy_ram[m43+y]=a;                              //195d
    INY; // 195f
    if (!y)
        goto xret;           // 1973::rts                    //1960
    a = floppy_ram[m43 + y]; // 1962
    TAX;                     // 1964
    a ^= floppy_ram[0x62];   // 1965
    // floppy_ram[m43+y]=a                               //1967
    TXA;                   // 1969
    ADC(floppy_ram[0x63]); // 196a
    INY;                   // 196c
    if (y)
        goto x193b; // 196d
    m43 += 0x100;   // 196f
    if (m43 >> 8)
        goto x193b; // 1971
xret:
    floppy_ram[0x44] = m43 >> 8;
    floppy_ram[0x43] = m43 & 0xff;
}

// if we could factor large composites in real time
// we'd have enough money not to need to rhyme
// digesting messages with a hashing function
// using sha-1 or else it will cause disfuction -- MC++
