/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */
// modified lightly to support MMU functions of Lisa Emulator + MOVEM by Ray Arachelian. :)
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <generator.h>
#include <cpu68k.h>

/* forward references */
void diss68k_getoperand(char *text, t_ipc *ipc, t_iib *iib, t_type type);

void printiib(t_iib *iib)
{
    // t_iib *myiib;

    fprintf(buglog, "\n----iib address: %p\n", iib);
    fprintf(buglog, "mask/bits: %02x/%02x\n", iib->mask, iib->bits);
    fprintf(buglog, "mnemonic: %s ", mnemonic_table[iib->mnemonic].name);

    if (iib->flags.priv)
        fprintf(buglog, "privilidged ");
    if (iib->flags.endblk)
        fprintf(buglog, "end_block ");
    if (iib->flags.imm_notzero)
        fprintf(buglog, "imm_notzero ");
    if (iib->flags.used)
        fprintf(buglog, "privileged ");
    fprintf(buglog, "\nused/set: %02x/%02x\n", iib->flags.used, iib->flags.set);
    switch (iib->size)
    {
    case sz_none:
        fprintf(buglog, "sizeless\n");
        break;
    case sz_byte:
        fprintf(buglog, "byte\n");
        break;
    case sz_word:
        fprintf(buglog, "word\n");
        break;
    case sz_long:
        fprintf(buglog, "long\n");
        break;
    default:
        fprintf(buglog, "error size!\n");
        break;
    }
    fprintf(buglog, "source/dest type: %d/%d\n", (int)iib->stype, (int)iib->dtype);
    fprintf(buglog, "sbitpos/dbitpost: %d/%d\n", (int)iib->sbitpos, (int)iib->dbitpos);
    fprintf(buglog, "immvalue: %d cc:%d function:%d wordlen:%d clocks:%d\n", iib->immvalue, iib->cc, iib->funcnum, iib->wordlen, iib->clocks);
    fprintf(buglog, "-----\n\n");
    fflush(buglog);
}

/* functions */

int diss68k_gettext(t_ipc *ipc, char *text)
{
    t_iib *iib;
    char *p, *c;
    char src[128], dst[128];
    char mnemonic[64];

    *text = '\0';

    if (ipc->opcode == 0x4e7a)
    {
        sprintf(text, "MOVEC VBR,D0 [UNI+ 68010 TESt - ILLEGAL on 68000]");
        return 0;
    }
    //    DEBUG_LOG(1,"getting iib for opcode:%04x",ipc->opcode);
    iib = cpu68k_iibtable[ipc->opcode];
    //    printiib(iib);

    if (iib == NULL)
    {
        DEBUG_LOG(1, "Got a null IIB.");
        return 0;
    }

    // Added by RA to correct MOVEM direction order   - this worked.
    if (iib->mnemonic != i_MOVEMMR)
    {
        diss68k_getoperand(dst, ipc, iib, tp_dst);
        diss68k_getoperand(src, ipc, iib, tp_src);
    }
    else
    {
        diss68k_getoperand(src, ipc, iib, tp_dst);
        diss68k_getoperand(dst, ipc, iib, tp_src); //
    }

    if ((iib->mnemonic == i_Bcc) || (iib->mnemonic == i_BSR) ||
        (iib->mnemonic == i_DBcc))
    {
        sprintf(src, "$%08x", ipc->src);
    }

    strcpy(mnemonic, mnemonic_table[iib->mnemonic].name);

    if ((p = strstr(mnemonic, "cc")) != NULL)
    {
        if (iib->mnemonic == i_Bcc && iib->cc == 0)
        {
            p[0] = 'R';
            p[1] = 'A';
        }
        else
        {
            c = condition_table[iib->cc];
            strcpy(p, c);
        }
    }

    switch (iib->size)
    {
    case sz_byte:
        strcat(mnemonic, ".B");
        break;
    case sz_word:
        strcat(mnemonic, ".W");
        break;
    case sz_long:
        strcat(mnemonic, ".L");
        break;
    default:
        break;
    }

    sprintf(text, "%-10s %s%s%s", mnemonic, src, dst[0] ? "," : "", dst);

    return 1;
}

void diss68k_getoperand(char *text, t_ipc *ipc, t_iib *iib, t_type type)
{
    int bitpos;
    uint32 val;

    if (type == tp_src)
    {
        bitpos = iib->sbitpos;
        val = ipc->src;
    }
    else
    {
        bitpos = iib->dbitpos;
        val = ipc->dst;
    }

    switch (type == tp_src ? iib->stype : iib->dtype)
    {
    case dt_Dreg:
        sprintf(text, "D%d", (ipc->opcode >> bitpos) & 7);
        break;
    case dt_Areg:
        sprintf(text, "A%d", (ipc->opcode >> bitpos) & 7);
        break;
    case dt_Aind:
        sprintf(text, "(A%d)", (ipc->opcode >> bitpos) & 7);
        break;
    case dt_Ainc:
        sprintf(text, "(A%d)+", (ipc->opcode >> bitpos) & 7);
        break;
    case dt_Adec:
        sprintf(text, "-(A%d)", (ipc->opcode >> bitpos) & 7);
        break;
    case dt_Adis:
        sprintf(text, "$%04x(A%d)", (uint16)val, (ipc->opcode >> bitpos) & 7);
        break;
    case dt_Aidx:
        // sprintf(text, "$%02x(A%d,Rx.X)", (uint8)val, (ipc->opcode >> bitpos) & 7);
        // sprintf(text, "AIDX:%d(A%d,%c%d.%c)",
        //  this is broken here: s/b: MOVEA.L                     -4(A0,D0.W),A0
        //  shows up as:     2070 00fc  MOVE.L     {AIDX:fffffffc}=(-4+A0+A7.L),A0
        sprintf(text, "%d(A%d+%c%d.%c)",
                (sint8)(val & 0xff), // RA - signed decimal since it's signed
                (ipc->opcode >> bitpos) & 7,
                ((val & 0x80000000) ? 'A' : 'D'), // this was wrong // RA 2004.04.27 Address vs Data register
                ((val >> (12 + 16)) & 7),         // this should have been D0. //               Reg #
                ((val & 0x8000000) ? 'L' : 'W')   //               word VS Long
        );
        break;
    case dt_AbsW:
        sprintf(text, "$%08x", val);
        break;
    case dt_AbsL:
        sprintf(text, "$%08x", val);
        break;
    case dt_Pdis:
        // sprintf(text, "%d(pc)",
        sprintf(text, "{PDIS:%08x}=(PC%c#$%04x) src:%08x dst:%08x", val,
                ((val - pc24) & 0x80000000 ? '-' : '+'),
                ((val - pc24) & 0x80000000 ? (pc24 - val) : (val - pc24)) & 0xffff,
                ipc->src, ipc->dst); // RA 2004.11.13 +2 as per Macintosh ResEdit
        break;

    case dt_Pidx:
        // sprintf(text, "PIDX:%d(pc, %c%d.%c)",
        sprintf(text, "{PIDX:%08x}=(%d+PC+%c%d.%c)", val,
                (sint8)(val - pc24),              // RA 2004.04.27 - decimal because it's signed
                ((val & 0x80000000) ? 'A' : 'D'), // RA 2004.04.27 Address vs Data register
                ((val >> (12 + 16)) & 7),         //               Reg #
                ((val & 0x8000000) ? 'L' : 'W')   //               word VS Long
        );                                        // might want to change %02x to signed decimal
        break;

    case dt_ImmB:
        sprintf(text, "#$%02x", val);
        break;

    case dt_ImmW:
        // Added by RAY for output of LINK #$ (needs to print negative integers instead of #$FFxx
        if (iib->mnemonic == i_LINK)
        {
            int32 ival = (int16)(val);
            sprintf(text, "#%d", ival);
            break;
        }

        // Added by RA for output of MOVEM opcode disassembly
        if (iib->mnemonic != i_MOVEMRM && iib->mnemonic != i_MOVEMMR)
        {
            sprintf(text, "#$%04x", val);
            break;
        }
        {
            //    15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
            //     0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
            char *REGMASK = "D0/D1/D2/D3/D4/D5/D6/D7:A0/A1/A2/A3/A4/A5/A6/A7/";
            int16 i, k = 0;
            uint8 mode = (ipc->opcode >> 3) & 7;

            // remove next line after you debug this:
            sprintf(text, "{0x%04x}: ", val);
            k = strlen(text);

            if (mode == 4)
            {
                // remove next line after debug
                strcat(text, "REV:");
                // select pre-decrement register order instead
                for (i = 15; i > -1; i--) // there are 16 registers to deal with. A0-A7,D0-D7
                {
                    if (val & (1 << i))
                    {
                        text[k++] = REGMASK[(15 - i) * 3];
                        text[k++] = REGMASK[(15 - i) * 3 + 1]; // bit is set, print register associated with it

                        if (val & (1 << (i - 1)) && i != 7) // is the next bit selected too?
                        {
                            text[k++] = '-'; // yes, and this is going to be a range.

                            while (val & (1 << i) && i > -1) // skip over bits that are not in the range.
                            {
                                i--;
                                if (i == 7)
                                    break;
                            } // but stop on border b/w A/D regs

                            i++; // step back one since we passed the last 1 bit (or on border)

                            // print the end register's name
                            text[k++] = REGMASK[(15 - i) * 3];
                            text[k++] = REGMASK[(15 - i) * 3 + 1];
                        }
                        text[k++] = '/'; // print a slash and leave it for the next one.
                    }
                }
            }
            else
            {
                // remove next line after debug
                strcat(text, "FWD:");

                for (i = 0; i < 16; i++) // there are 16 registers to deal with. A0-A7,D0-D7
                {
                    if (val & (1 << i))
                    {
                        text[k++] = REGMASK[i * 3];
                        text[k++] = REGMASK[i * 3 + 1]; // bit is set, print register associated with it

                        if (val & (1 << (i + 1)) && i != 8) // is the next bit selected too?
                        {
                            text[k++] = '-'; // yes, and this is going to be a range.

                            while (val & (1 << i)) // skip over bits that are not in the range.
                            {
                                i++;
                                if (i == 8)
                                    break;
                            } // but stop on border b/w A/D regs

                            i--; // step back one since we passed the last 1 bit (or on border)

                            // print the end register's name
                            text[k++] = REGMASK[i * 3];
                            text[k++] = REGMASK[i * 3 + 1];
                        }
                        text[k++] = '/'; // print a slash and leave it for the next one.
                    }
                }
            }
            text[k - 1] = 0; // terminate
        }
        break;

    case dt_ImmL:
        sprintf(text, "#$%08x", val);
        break;
    case dt_ImmS:
        sprintf(text, "#%d", iib->immvalue);
        break;
    case dt_Imm3:
        sprintf(text, "#%d", (ipc->opcode >> bitpos) & 7);
        break;
    case dt_Imm4:
        sprintf(text, "#%d", (ipc->opcode >> bitpos) & 15);
        break;
    case dt_Imm8:
        sprintf(text, "#%d", (ipc->opcode >> bitpos) & 255);
        break;
    case dt_Imm8s:
        sprintf(text, "#%d", (sint32)val);
        break;

    default:
        *text = 0;
        break;
    }
}

/** DANGER ** When we call this the *addr param must come from fetchaddr(addr) else no MMU translation! **/
// actually *addr appears to no longer be used, so there.
// int diss68k_getdumpline(uint32 addr68k, uint8 *addr, char *dumpline)

int diss68k_getdumpline(uint32 addr68k, char *dumpline)
{
    t_ipc ipc;
    abort_opcode = 2;
    PAUSE_DEBUG_MESSAGES();
    t_iib *iibp = cpu68k_iibtable[fetchword(addr68k)];

    if (abort_opcode == 1)
    {
        RESUME_DEBUG_MESSAGES();
        DEBUG_LOG(0, "got_abort_opcode!");
        return 0;
    }
    else
        abort_opcode = 0;

    int words, i;
    char dissline[64], *p;

    if (addr68k < 256)
    {
        abort_opcode = 2;
        sprintf(dissline, "dc.l $%08x", fetchlong(addr68k));

        if (abort_opcode == 1)
        {
            RESUME_DEBUG_MESSAGES();
            DEBUG_LOG(0, "got_abort_opcode!");
            return 0;
        }
        else
            abort_opcode = 0;
        abort_opcode = 0;
        words = 2;
    }
    else
    {

#if DEBUG
        if (!iibp)
            ALERT_LOG(0, "about to pass NULL IIB");
#endif

        if (iibp != NULL)
        {
            cpu68k_ipc(addr68k, iibp, &ipc);
            if (abort_opcode == 1)
            {
                RESUME_DEBUG_MESSAGES();
                DEBUG_LOG(0, "got_abort_opcode!");
                return 0;
            }
            else
                abort_opcode = 0;
            if (!diss68k_gettext(&ipc, dissline))
                strcpy(dissline, "Illegal Instruction");
        }
        else
            strcpy(dissline, "Illegal Instruction");
        words = ipc.wordlen;
    }

    p = dumpline;
    abort_opcode = 2;

    if ((ipc.opcode & 0xf000) == 0xa000)
    { // A-Line Lisa traps are 4 bytes wide
        p += sprintf(p, ": %08x ", (fetchbyte(addr68k) << 24) + (fetchbyte(addr68k + 1) << 16) + (fetchbyte(addr68k + 2) << 8) + fetchbyte(addr68k + 3));
    }
    else
        p += sprintf(p, ": %04x ", (fetchbyte(addr68k) << 8) + fetchbyte(addr68k + 1));
    if (abort_opcode == 1)
    {
        RESUME_DEBUG_MESSAGES();
        DEBUG_LOG(0, "got_abort_opcode!");
        return 0;
    }
    else
        abort_opcode = 0;

    for (i = 1; i < words; i++)
    {
        uint16 w;
        abort_opcode = 2;
        p += sprintf(p, "%04x ", fetchword(addr68k + (i * 2)));
        if (abort_opcode == 1)
        {
            RESUME_DEBUG_MESSAGES();
            DEBUG_LOG(0, "got_abort_opcode!");
            return 0;
        }
        else
            abort_opcode = 0;
    }
    for (i = 29 - strlen(dumpline); i > 0; i--)
    {
        *p++ = ' ';
    }
    p += sprintf(p, ": ");

    for (i = 0; i < words; i++)
    {
        abort_opcode = 2;
        if (isalnum(fetchbyte(addr68k + (i * 2))))
        {
            if (abort_opcode == 1)
            {
                RESUME_DEBUG_MESSAGES();
                DEBUG_LOG(0, "got_abort_opcode!");
                return 0;
            }
            else
                abort_opcode = 0;
            abort_opcode = 2;
            *p++ = fetchbyte(addr68k + i * 2);
            if (abort_opcode == 1)
            {
                RESUME_DEBUG_MESSAGES();
                DEBUG_LOG(0, "got_abort_opcode!");
                return 0;
            }
            else
                abort_opcode = 0;
        }
        else
            *p++ = '.';
        abort_opcode = 2;
        if (isalnum(fetchbyte(addr68k + (i * 2 + 1))))
        {
            if (abort_opcode == 1)
            {
                RESUME_DEBUG_MESSAGES();
                DEBUG_LOG(0, "got_abort_opcode!");
                return 0;
            }
            else
                abort_opcode = 0;
            abort_opcode = 2;
            *p++ = fetchbyte(addr68k + (i * 2 + 1));
            if (abort_opcode == 1)
            {
                RESUME_DEBUG_MESSAGES();
                DEBUG_LOG(0, "got_abort_opcode!");
                return 0;
            }
            else
                abort_opcode = 0;
        }
        else
            *p++ = '.';
    }
    *p = '\0';
    for (i = 39 - strlen(dumpline); i > 0; i--)
    {
        *p++ = ' ';
    }
    if (iibp)
    {
        sprintf(p, " : %4d : %s", iibp->funcnum, dissline);
    }
    else
    {
        sprintf(p, " :      : %s", dissline);
    }

    RESUME_DEBUG_MESSAGES();
    return words;
}
