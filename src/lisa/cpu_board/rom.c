/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2007.12.04                   *
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
*                                                                                      *
*                 CPU Board ROM loader and associated functions                        *
*                                                                                      *
\**************************************************************************************/

#define IN_ROM_C 1

#include <vars.h>

uint8 ishex(char c)
{
    if (c >= '0' && c <= '9')
        return 1;
    if (c >= 'a' && c <= 'f')
        return 1;
    if (c >= 'A' && c <= 'F')
        return 1;
    return 0;
}

uint8 gethex(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

// shamelessly stolen from ROL in generator so they match  -- used to fake the Lisa ROM checksum.
uint32 lrol(uint32 outdata, uint8 loop)
{
    uint32 cflag = 0;

    while (loop)
    {
        cflag = outdata & 1 << 31 ? 1 : 0;
        outdata <<= 1;
        if (cflag)
            outdata |= 1;
        loop--;
    }

    return outdata;
}

uint16 wrol(uint16 outdata, uint8 loop)
{
    uint16 cflag = 0;

    while (loop)
    {
        cflag = outdata & 1 << 15 ? 1 : 0;
        outdata <<= 1;
        if (cflag)
            outdata |= 1;
        loop--;
    }

    return outdata;
}

uint8 brol(uint8 outdata, uint8 loop)
{
    uint8 cflag = 0;

    while (loop)
    {
        cflag = outdata & 1 << 7 ? 1 : 0;
        outdata <<= 1;
        if (cflag)
            outdata |= 1;
        loop--;
    }

    return outdata;
}

void vidfixromchk(uint8 *s)
{
    int i;
    uint16 chksum, sum;

    s[0] = 0xff;
    s[7] = 0xff;
    chksum = (s[24] * 100) + (s[25] * 10) + s[26];

    for (sum = 0, i = 0; i < 24; i++)
        sum += (s[i] >> 4) + (s[i] & 15);
    sum += s[27];
    sum -= 60;

    // DEBUG_LOG(0,"serno:checksum I calculated: %04x, checksum stored: %04x lisa calculated 0x82",sum,chksum);

    /*  The checksum the lisa does is the addition of vals of addresses 0x250-0x268  -60 + s[27]
     *
     *
     */

    if (chksum == sum)
        return;

    s[24] = sum / 100;
    sum = sum % 100;
    s[25] = sum / 10;
    sum = sum % 10;
    s[26] = sum;

    // DEBUG_LOG(0,"Corrected checksum bytes from (%d) to (%d) (%d %d %d)",chksum,sum,s[24],s[25],s[26]);
}

uint16 chksum_a_rom_range(uint8 *rom, uint32 a0, uint32 a1)
{
    uint16 d0 = 0;

    do // DOSUM
    {
        d0 += (rom[a0] << 8) + rom[a0 + 1]; // ADD(A0)+,D0
        a0 += 2;
        d0 = wrol(d0, 1); // ROL #1,D0
    }                     // CMPA.L A0,A1
    while (a0 != a1);     // BNE.S DOSUM

    return d0;
}

void fixromchk(void)
{

    /* C ROM */ if (lisarom[0x3ffc] == 0x02 && lisarom[0x3ffd] == 0x11)
        ALERT_LOG(0, "C ROM 0x275:%02x", lisarom[0x0275]);

    // don't touch any ROM except H
    if (lisarom[0x3ffc] != 0x02 || lisarom[0x3ffd] != 'H')
    {
        ALERT_LOG(0, "ROM is not H-ROM, will not patch");
        return;
    }

    if (cheat_ram_test)
    {

        /* Only apply these patches if we're fairly sure it's the 2.H ROM */

        /*
         * Patch the check in MEMTST2 to decide how many tests to run based on
         * warmstart/coldstart to instead jump straight to TSTDONE always.
         */

        ALERT_LOG(0, "H ROM patched with ram-test cheats")

        lisarom[0x0e1a] = 0x60;
        lisarom[0x0e1b] = 0x32;

        /* Instead of jumping to BEGIN go to SETMMU. */
        lisarom[0x0006] = 0x02;
        lisarom[0x0007] = 0xc6;

        // 0AD4| 203C 0013 12D0
        lisarom[0x0ad7] = 8; // 5 second delay -> 2s delay during boot process. this is used as a long beep, during booting, service mode display testing
    }

    ALERT_LOG(0, "HLE is: %d", hle)
    if (hle)
    {
        // patch the ROM entry at 090, and also the actual call inside the ROM, which can
        // move around depending on the ROM version, because when booting from the ROM
        // the ROM will call the actual address of PROREAD rather than go through the
        // jump table entry at fe0090

        lisarom[0x90] = 0xf3;
        lisarom[0x91] = 0x3d;

        rom_profile_read_entry = (lisarom[0x92] << 8) + (lisarom[0x93]) + 0x92;

        lisarom[rom_profile_read_entry + 0] = 0xf3;
        lisarom[rom_profile_read_entry + 1] = 0x3d;

        rom_profile_read_entry |= 0x00fe0000;

        ALERT_LOG(0, "#  # #    ####    Applied ROM HLE  to ProFile Read fe0090 and %08x", rom_profile_read_entry);
        ALERT_LOG(0, "#  # #    #       Applied ROM HLE  to ProFile Read fe0090 and %08x", rom_profile_read_entry);
        ALERT_LOG(0, "#### #    ####    Applied ROM HLE  to ProFile Read fe0090 and %08x", rom_profile_read_entry);
        ALERT_LOG(0, "#  # #    #       Applied ROM HLE  to ProFile Read fe0090 and %08x", rom_profile_read_entry);
        ALERT_LOG(0, "#  # #### ####    Applied ROM HLE  to ProFile Read fe0090 and %08x", rom_profile_read_entry);
    }

    /* Simulate all but the final add of the ROM checksum routine */
    uint16 d0;
    uint32 a0, a1;

    d0 = 0;      // CLR.L D0
    a0 = 0;      // LEA BASE,A0
    a1 = 0x3ffe; // LEA LAST,A1

    d0 = chksum_a_rom_range(lisarom, a0, a1);

    //    ALERT_LOG(1,"Lisa ROM Checksum is %04x,negated it's %04x, zero=%04x",d0,(~d0)+1,d0+(~d0)+1);

    /* negate checksum so when lisa rom adds it, it will be zero. */
    d0 = (~d0) + 1;

    lisarom[0x3ffe] = (d0 >> 8);
    lisarom[0x3fff] = (d0 & 0xff);
}

// not sure if this will work with non 2.x ROMs (i.e. Lisa 1 or 3A), worst case
// it'll just send an alert.
int checkromchksum(void)
{
    /* Simulate all including final add of the ROM checksum routine */
    uint16 d0;
    uint32 a0, a1;

    d0 = 0;      // CLR.L D0
    a0 = 0;      // LEA BASE,A0
    a1 = 0x4000; // LEA LAST,A1

    d0 = chksum_a_rom_range(lisarom, a0, a1);

    //    ALERT_LOG(1,"Original Lisa ROM CHKSUM is %02x%02x",lisarom[0x3ffe],lisarom[0x3fff]);
    //    ALERT_LOG(1,"Lisa ROM Checksum is %04x,negated it's %04x, zero=%04x",d0,(~d0)+1,d0+(~d0)+1);

    /* negate checksum so when lisa rom adds it, it will be zero. */
    return (~d0) + 1;
}

int has_xl_screenmod(void)
{

    //      |VERSION|CHKSUM
    // 3ffc::|03 41 |d9 05 | <-3A ROM
    // 3ffc: |02 48 |3f 7b |  <-H ROM
    // 3ffc  |02 46 |f6 f5 |  <-F ROM
    // 3ffc  |02 11 |b2 67 |  <-C ROM
    // 3ffc  |02 09 |5E 4B |  <-B ROM
    //------|------|------|-----------
    //      |0c 0d |0e 0f |
    return (lisarom[0x3ffc] == 0x03);
}

/**************************************************************************************\
* This function loads in a ROM image from the source code as provided by DTC.  This    *
* expects the ASSEMBLED source file as input.  It can also read a hex dump made by a   *
* debugger if in the same format, and the dump is less than 28 chars wide.             *
* i.e.: 0000| AA AB BC CD FF                                                           *
*       addr| hex byte data.                                                           *
*                                                                                      *
* The main reason for this is so that during debugging I can see what the ROM is       *
* attempting to execute and the results.  This way I can output the ROM source instead *
* of hard to read uncommented machine code without symbols.                            *
*                                                                                      *
* Valgrind reports memory leak here, don't really care.                                *
\**************************************************************************************/

int16 read_dtc_rom(char *filename, uint8 *ROM)
{

    char *line; // line buffer
    unsigned long address;
    unsigned int data;
    long fileposition;
    int i;
    int ok = 0;

    rom_source_file = fopen(filename, "rt");
    if (!rom_source_file)
        return -1;

    line = (char *)calloc(1, 1024);
    if (!line)
        return -2;

    if (rom_source_file)
    {
#ifdef DEBUG
        dtc_rom_fseeks = (long *)calloc(sizeof(long), 0x4000); // fseeks for source code.
        if (!dtc_rom_fseeks)
        {
            EXITR(5, 0, "Couldn't allocate enough memory for ROM source pointers!\n");
        }
        memset(dtc_rom_fseeks, 0, sizeof(long) * 0x4000);
#endif

        // Fill ROM with NOP's incase there are "holes" in the ROM    NOP=0x4e71

        // Repair ROM holes in DTC rom
        // memset(ROM,0,16383);
        for (i = 0; i < 0x4000; i += 2)
        {
            ROM[i] = 0x4e;
            ROM[i | 1] = 0x71;
        }
        memset(&ROM[0x39C3], 0xff, 0x3fff - 0x39c3);

        DEBUG_LOG(0, "Reading source assembled ROM\n");

        while (!feof(rom_source_file))
        {
            char *v = NULL;
            fileposition = ftell(rom_source_file);
            v = fgets(line, 1024, rom_source_file);
            if (!v)
                ALERT_LOG(0, "fgets returned NULL when reading from rom source file!");
            if (ishex(line[0]) && ishex(line[1]) && ishex(line[2]) && ishex(line[3]) && line[4] == '|' && line[5] == ' ')
            {
                address = (gethex(line[0]) << 12) + (gethex(line[1]) << 8) + (gethex(line[2]) << 4) + gethex(line[3]);
                // DEBUG_LOG(0,("%04x:",address);
                for (i = 6; i < 29; i++)
                    if (ishex(line[i]) && ishex(line[i + 1]))
                    {
                        data = (gethex(line[i]) << 4) + gethex(line[i + 1]);
                        // DEBUG_LOG(0,("%02x ",data);
                        if (address < 0x4000)
                        {
                            ROM[address] = data;
                            ok = 1;
#ifdef DEBUG
                            if (dtc_rom_fseeks)
                                dtc_rom_fseeks[address] = fileposition;
#endif
                        }
                        i++;       // since we read another in line(i+1)
                        address++; // since this could be a word, or long
                    }
            } // If hex data on line
            // printf("\n");
        } // file reading while loop

#ifdef DEBUG
        fseek(rom_source_file, 0L, SEEK_SET);
#else
        if (rom_source_file)
            fclose(rom_source_file);
#endif
    }
    // See if we have the object file.

    snprintf(line, 1024, "%s.obj", filename);
    rom_source_file = fopen(line, "rt");
    if (!rom_source_file && ok)
    {
        fprintf(buglog, "Warning! Lisa rom source is filled with 'holes' - the emulator will fail!\n");
        fprintf(buglog, "Add the object code file, or use a real ROM, but the source won't match it.\n\n");
        free(line);
        return ok ? 0 : 1;

        // "Imagine if every Thursday your shoes exploded if you tied them the
        //  usual way.  This happens to us all the time with computers, and nobody
        //  thinks of complaining." -- Jef Raskin, interviewed in Doctor Dobb's Journal
    }

    if (!ok) // If we didn't get the source rom, plug the holes.
    {
        for (i = 0; i < 0x4000; i += 2)
        {
            ROM[i] = 0x4e;
            ROM[i | 1] = 0x71;
        }
        memset(&ROM[0x39C3], 0xff, 0x3fff - 0x39c3);
    }

    if (rom_source_file)
    {

#define OBJOFF 0x0038
#define READ_HEX_QUAD(i)                                                                      \
    if (ishex(line[i + 0]) && ishex(line[i + 1]) && ishex(line[i + 2]) && ishex(line[i + 3])) \
    {                                                                                         \
        if (address > OBJOFF - 1 && address < 0x3ffe)                                         \
        {                                                                                     \
            uint8 a, b;                                                                       \
            a = ROM[address - OBJOFF];                                                        \
            b = ROM[address - OBJOFF + 1];                                                    \
            ROM[address - OBJOFF] = (gethex(line[i + 0]) << 4) | gethex(line[i + 1]);         \
            address++;                                                                        \
            ROM[address - OBJOFF] = (gethex(line[i + 2]) << 4) | gethex(line[i + 3]);         \
            address++;                                                                        \
        }                                                                                     \
        else                                                                                  \
            address += 2;                                                                     \
    }

        while (!feof(rom_source_file))
        {
            char *z = NULL;
            //   01234567890123456789012345678901234567890123456789
            //             1         2         3         4
            //   00aaaa: xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx [................]

            z = fgets(line, 1024, rom_source_file);
            if (!z)
            {
                ALERT_LOG(0, "Error reading line from %s", filename);
            }

            if (line[7] == ' ' && line[6] == ':' && line[0] == '0' && line[1] == '0' && ishex(line[2]) && ishex(line[3]) && ishex(line[4]) && ishex(line[5]))
            {
                address = (gethex(line[2]) << 12) + (gethex(line[3]) << 8) + (gethex(line[4]) << 4) + gethex(line[5]);
                if (address > 0x4039)
                {
                    if (rom_source_file)
                        fclose(rom_source_file);
                    free(line);
#ifdef DEBUG
                    rom_source_file = fopen(filename, "rt");
                    fseek(rom_source_file, 0L, SEEK_SET);
#endif
                    return 0;
                }
                READ_HEX_QUAD(8);
                READ_HEX_QUAD(13);
                READ_HEX_QUAD(18);
                READ_HEX_QUAD(23);
                READ_HEX_QUAD(28);
                READ_HEX_QUAD(33);
                READ_HEX_QUAD(38);
                READ_HEX_QUAD(43);
            }
        }
    }

    if (rom_source_file)
        fclose(rom_source_file);
    free(line);
#ifdef DEBUG
    rom_source_file = fopen(filename, "rt");
    fseek(rom_source_file, 0L, SEEK_SET);
#endif

    return (ok ? 0 : 1);
}

/**************************************************************************************\
* This function loads in a ROM image that is split up into even and odd bytes as       *
* as provided by the MESS project.                                                     *
\**************************************************************************************/
extern void rename_rompath(char *rompath);

int16 read_split_rom(char *filename, uint8 *ROMMX)
{
    int i, j;
    char myfilename[FILENAME_MAX];
    char infilename[FILENAME_MAX];
    char *s;
    uint8 *ROMLO, *ROMHI;
    FILE *low, *high, *out;
    size_t c;
    int lohiflag = 0;
    if (!filename || !ROMMX)
        return -3;
    strncpy(infilename, filename, FILENAME_MAX - 1); // copy the original file name

    ROMLO = (uint8 *)calloc(1, 8194);
    if (!ROMLO)
    {
        return -2;
    }
    ROMHI = (uint8 *)calloc(1, 8194);
    if (!ROMHI)
    {
        free(ROMLO);
        return -2;
    }

    // :TODO: is if strcasestr exists on all our supported platforms
    s = strstr(infilename, ".lo");
    if (!s)
        s = strstr(infilename, ".LO");
    if (!s)
        s = strstr(infilename, ".Lo");
    if (s)
        *s = 0; // strip off any possible .lo

    s = strstr(infilename, ".hi");
    if (!s)
        s = strstr(infilename, ".HI");
    if (!s)
        s = strstr(infilename, ".Hi");
    if (s)
        *s = 0; // strip off any possible .hi

    // You try to pick the lesser of, But evil doesn't come in twos, Whoooo!
    // note we do assignements inside the if here, because we're super cool like that
    //                         0123456
    if ((s = strstr(infilename, "341-175-")) || (s = strstr(infilename, "341-176-")))
    {

        s[6] = '5';
        low = fopen(infilename, "rb");
        if (!low)
        {
            free(ROMHI);
            free(ROMLO);
            return -1;
        }
        DEBUG_LOG(0, "Opened %s\n", infilename);

        s[6] = '6';
        high = fopen(infilename, "rb");
        if (!high)
        {
            fclose(low);
            free(ROMHI);
            free(ROMLO);
            return -1;
        }
        DEBUG_LOG(0, "Opened %s\n", infilename);

        // strip off .bin, we'll add .ROM later
        s = strstr(infilename, ".bin");
        if (!s)
            s = strstr(infilename, ".BIN");
        if (!s)
            s = strstr(infilename, ".Bin");
        if (s)
            *s = 0;
    }
    else
    {
        snprintf(myfilename, FILENAME_MAX - 1, "%s.lo", infilename);
        low = fopen(myfilename, "rb");
        if (!low)
        {
            free(ROMHI);
            free(ROMLO);
            return -1;
        }
        DEBUG_LOG(0, "Opened %s\n", myfilename);

        snprintf(myfilename, FILENAME_MAX - 1, "%s.hi", infilename);
        high = fopen(myfilename, "rb");
        if (!high)
        {
            fclose(low);
            free(ROMHI);
            free(ROMLO);
            return -1;
        }
        DEBUG_LOG(0, "Opened %s\n", myfilename);
    }

    // [control|absorb|destroy] yourself [you're better alone|see who gives a fuck|if there was a day I could live|if there was a single breath I could breathe]... LoG!
    c = fread(ROMLO, 8192, 1, low);
    if (c != 1)
    {
        ALERT_LOG(0, "Error reading  LOW bytes from %s", myfilename);
    }
    c = fread(ROMHI, 8192, 1, high);
    if (c != 1)
    {
        ALERT_LOG(0, "Error reading HIGH bytes from %s", myfilename);
    }
    // for the truth shall set you free
    fclose(low);
    fclose(high); // and bury you with honesty

    // stitch the lo/hi | even/odd ROMs
    for (i = 0, j = 0; i < 16384; i += 2, j++)
    {
        ROMMX[i + 1] = ROMLO[j];
        ROMMX[i] = ROMHI[j];
    }

    // lay this to rest
    free(ROMHI);
    free(ROMLO);

    // collosal hate arrises
    snprintf(myfilename, 1024, "%s.ROM", infilename);
    out = fopen(myfilename, "wb");
    if (out)
    {
        fwrite(ROMMX, 16384, 1, out);
        fflush(out);
        fclose(out);
        rename_rompath(myfilename);
    }

    // for (a=good; time<9999; see++) {youtu_be("KgaEI08JsiE"); ALERT_LOG(0,"Randy, you're scaring the kids.  Go scream in the closet! lel!");
    DEBUG_LOG(0, "Saved merged rom file as %s\n", myfilename);

    if (ROMMX[0] == 0x00 &&
        ROMMX[1] == 0x00 &&
        ROMMX[2] == 0x80 &&
        ROMMX[3] == 0x04)
        return 0;

    return -3;
}

/**************************************************************************************\
* This function loads in a straight/raw 16k rom dump.                                  *
\**************************************************************************************/

extern void enable_4MB_macworks(void);

int16 read_rom(char *filename, uint8 *ROMMX)
{
    FILE *rom;
    int c;
    if (!filename || !ROMMX)
        return -3;
    rom = fopen(filename, "rb");
    if (!rom)
    {
        return -1;
    }
    c = fread(ROMMX, 16384, 1, rom);
    fclose(rom);
    if (c != 1)
    {
        ALERT_LOG(0, "fread returned %d count instead of 1", c);
        return -1;
    }
#ifdef DEBUG
    DEBUG_LOG(0, "Checking for screenmod:%02x subver %02x", ROMMX[0x3ffc], ROMMX[0x3ffd]);
    if (has_xl_screenmod())
        DEBUG_LOG(0, "Is screenmod ROM");
#endif

    ALERT_LOG(0, "Lisa ROM CHKSUM is %02x%02x version:%02x.%c (%02x)", ROMMX[0x3ffe], ROMMX[0x3fff], ROMMX[0x3ffc], ROMMX[0x3ffd], ROMMX[0x3ffd]);
#ifdef DEBUG
//    if (lisarom[0x3ffc]==2 && lisarom[0x3ffd]<'F') debug_on("C-ROM");
#endif

    if (lisarom[0x3ffc] == 3 && lisarom[0x3ffd] >= 'A')
    { // 3A ROM
        if (macworks4mb)
            enable_4MB_macworks();
    }

    return 0;
}

extern int romless_dualparallel(void);

int read_parallel_card_rom(char *filename)
{
    int ret = 1;
    uint16 newchks;

    FILE *ROMFILE;
    ROMFILE = fopen(filename, "rb");
    if (!ROMFILE)
    {
        ALERT_LOG(0, "Could not open %s\n", filename);
        perror("");
        ret = 0;
    }
    else
    {
        ret = fread(dualparallelrom, 2048, 1, ROMFILE);
        fclose(ROMFILE);
    }

    // Skip over dual parallel ROM test if cheats are enabled by clearing D0 and returning in status routine
    // of card.  our checksum of the rom (excl chkword) says 632b, but the word there is 9cd5 which is
    // (~632b)+1. :)

    uint16 cardid = (dualparallelrom[0] << 8) | (dualparallelrom[1]);
    uint16 words = ((dualparallelrom[2] << 8) | (dualparallelrom[3])) * 2 + 4;

    //    ALERT_LOG(0,"Opened %s as card:%04x of size:%04x",filename,cardid,words);

    // if the ROM size is wrong, or the card ID doesn't indicate a parallel card,
    // or if the checksum is wrong, we failed to load the ROM we wanted.  Don't bother
    // passing it to the virtual Lisa.
    //
    // For future versions of the emulator we can edit the dualparallel filter
    // if the emulator handles that type of card, and it'safe to disable the status
    // routine as a time saving cheat.
    //
    if (words > 0x1fff || (cardid & 0xff00) != 0xe000)
        ret = 0;
    else if (chksum_a_rom_range(dualparallelrom, 0, words + 2))
        ret = 0;

    // Ensure that we have a good ROM, that cheats enabled, and that this ROM has a status routine.
    if (ret == 1 && cheat_ram_test && (cardid & 0x4000))
    {
        uint16 newchks;

        // blow away status routine, always returning a successful test.
        dualparallelrom[4 + 0x0a] = 0x42;
        dualparallelrom[4 + 0x0b] = 0x40; // CLR.W D0
        dualparallelrom[4 + 0x0c] = 0x4e;
        dualparallelrom[4 + 0x0d] = 0x75; // RTS

        // recalculate checksum so the BOOT ROM won't complain.
        dualparallelrom[words] = 0;
        dualparallelrom[words + 1] = 0;
        newchks = chksum_a_rom_range(dualparallelrom, 0, words);
        newchks = (~newchks) + 1;
        dualparallelrom[words] = (newchks >> 8);
        dualparallelrom[words + 1] = (newchks & 0xff);
    }

    if (!ret)
        return romless_dualparallel();

    return (ret != 1);
}

uint8 *decode_lisa_icon(uint8 *icon)
{
    int iindex = 0, oindex = 0;
    int octals, mapbyte, obit;
    static uint8 output[6 * 33 + 1];

    memset(output, 0, 6 * 33);

    for (octals = 0; octals < 24; octals++)
    {
        mapbyte = icon[iindex++] | 256;
        while (mapbyte)
        {
            obit = mapbyte & 1;
            mapbyte = mapbyte >> 1;
            if (!mapbyte)
                break;
            if (obit)
                output[oindex++] = 0;
            else
                output[oindex++] = icon[iindex++];

            if (iindex > 6 * 33 || oindex > 6 * 32)
                return output;
        }
    }

    return output;
}
