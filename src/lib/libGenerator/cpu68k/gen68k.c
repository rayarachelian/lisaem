/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */
// modifications made to RESET and Illegal opcodes as well as SuperVisor mode
// opcodes -- Ray Arachelian 2002.08.02/2002.08.15 for LisaEm

/*-----------------4/4/2006 8:43AM------------------
 *  In my own tests on a 68040 (NeXTStep 3) I've found that the NV flags do not change for BCD
 *  instructions!  I'm not sure whether they are different on a 68000 or other CPU's in the family.
 *  however for the LisaEmulator project, I've not seen these opcodes used at all, so I'm unable
 *  to confirm that this matters.  For completeness I've added this next define which disables
 * N and V flag calculations for ABCD,SBCD,NBCD.
 *
 *
 * ***BROKEN*** opcode: 8300   deltad1 deltaflags
 * before d0=00000000    d1=00000001    CCR=XN.VC (27)
 * M68K   d0=00000000    d1=00000000    CCR=.N.V. (10) NV_same
 * GEN    d0=00000000    d1=00000001 *  CCR=..... * (0)
 *
 * ...
 *
 * ***BROKEN*** opcode: 8300   deltad1 deltaflags
 * before d0=00000000    d1=00000000    CCR=X...C (17)
 * M68K   d0=00000000    d1=00000099    CCR=X...C (17) NV_same
 * GEN    d0=00000000    d1=00000000 *  CCR=XN..C * (25)
 *
 * ...
 *
 * [ray@sunstreak:/NeXT/generator:148]$ bzip2 -cd gen-opcode-abcd.txt.bz2  | grep -ni M68K | grep -v NV_same
 * [ray@sunstreak:/NeXT/generator:149]$ bzip2 -cd gen-opcode-sbcd.txt.bz2  | grep -ni M68K | grep -v NV_same
 * [ray@sunstreak:/NeXT/generator:150]$ bzip2 -cd gen-opcode-nbcd.txt.bz2  | grep -ni M68K | grep -v NV_same
 * [ray@sunstreak:/NeXT/generator:151]$ bzip2 -cd gen-opcode-nbcd.txt.bz2  | grep -ni M68K | grep -v NV_same
 *
 * All of the test results show that the NV_same output exists, which means that the "before" input N,V flags
 * match the M68K output N,V flags.
 *
 * -- Ray Arachelian
 *
 * --------------------------------------------------*/
#define BCD_NV_SAME 1

#define PRESERVE_HIGH_BYTE 1

#include <stdio.h>
#include <stdlib.h>

#include <generator.h>

#include "def68k-iibs.h"

/* forward references */

#ifdef DEBUG
static int gendbg = 0;
#define GENDBG(x)                                                                   \
    {                                                                               \
        if (gendbg)                                                                 \
            fprintf(o, "//DBG:%s:%s:%d %s\n", __FILE__, __FUNCTION__, __LINE__, x); \
    }
#else
#define GENDBG(x)
#endif

void generate(FILE *o, int topnibble);
void generate_ea(FILE *o, t_iib *iib, t_type type, int update);
void generate_eaval(FILE *o, t_iib *iib, t_type type);
void generate_eastore(FILE *o, t_iib *iib, t_type type);
void generate_outdata(FILE *o, t_iib *iib, const char *init);
void generate_cc(FILE *o, t_iib *iib);
void generate_stdflag_n(FILE *o, t_iib *iib);
void generate_stdflag_z(FILE *o, t_iib *iib);
void generate_clrflag_v(FILE *o, t_iib *iib);
void generate_clrflag_c(FILE *o, t_iib *iib);
void generate_clrflag_n(FILE *o, t_iib *iib);
void generate_setflag_z(FILE *o, t_iib *iib);
void generate_subflag_c(FILE *o, t_iib *iib);
void generate_subflag_cx(FILE *o, t_iib *iib);
void generate_subxflag_cx(FILE *o, t_iib *iib);
void generate_subflag_v(FILE *o, t_iib *iib);
void generate_cmpaflag_c(FILE *o, t_iib *iib);
void generate_cmpaflag_v(FILE *o, t_iib *iib);
void generate_addflag_cx(FILE *o, t_iib *iib);
void generate_addflag_v(FILE *o, t_iib *iib);
void generate_addxflag_cx(FILE *o, t_iib *iib);
void generate_stdxflag_z(FILE *o, t_iib *iib);
void generate_negflag_cx(FILE *o, t_iib *iib);
void generate_negflag_v(FILE *o, t_iib *iib);
void generate_negxflag_cx(FILE *o, t_iib *iib);
void generate_negxflag_v(FILE *o, t_iib *iib);
void generate_bits(FILE *o, t_iib *iib);

// Added by Ray Arachelian to allow irq's to abort opcode execution.
#define ABORT_CHECK(OUTF) fputs("\nABORT_OPCODE_CHK();\n", OUTF)
#define FLAG_ACHK(OUTF) fputs("\n#ifndef CHK_ABRT_OPC\n  #define CHK_ABRT_OPC  \n#endif\n", OUTF)
#define C_ABRT_CHK(OUTF) fputs("\n#ifdef  CHK_ABRT_OPC\n  ABORT_OPCODE_CHK();   \n#endif\n", OUTF)
#define UNFLAG_ACHK(OUTF) fputs("\n#ifdef  CHK_ABRT_OPC\n  #undef CHK_ABRT_OPC   \n#endif\n", OUTF)

/*
 *
 * Story is this.  MMU will signal opcode aborts on failed memory accesses.  However, C has some limitations.
 * For example it's possible to define and assign a variable in a single shot.  i.e. uint8 blah=fetchword();
 * but it's not possible to abort assigning a value when it will do harm because you can't put an if statement
 * inbetween several such assignments.  Hence the above macros will optionally add a conditional abort check later.
 * FLAG and UNFLAG set a define that enables ABORT_CHECK if needed (so as to remove unnecessary checks that would
 * slow the emulator down.  checks must be done after [fetch|store][byte|word|long] calls.
 *
 * In some instances where these macros are unsuitable, I used temporary variables to do the fetches (b1,w1,l1, etc.)
 * so as to allow abort_opcode the opportunity to be set (and ABORT_CHECK to check it) before the opcode executes.
 *
 * -- Ray Arachelian.  2004.09.18
 */

/* defines */

#define HEADER "/*****************************************************************************/\n/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */\n/*****************************************************************************/\n/*                                                                           */\n/* cpu68k-%x.c                                                                */\n/*                                                                           */\n/*****************************************************************************/\n\n"

#define OUT(x) fputs(x, o);
#define FNAME_GEN68K_CPU_OUT "cpu68k-%x.c"

/* program entry routine */

int main(int argc, char *argv[])
{
    FILE *o;
    int i;
    char tmp[256];

    printf("Writing C files... ");
    fflush(stdout);

    for (i = 0; i < 16; i++)
    {

        printf("%1x. ", i);
        fflush(stdout);

        /* make filename */
        sprintf(tmp, FNAME_GEN68K_CPU_OUT, i);

        /* open output file */
        if ((o = fopen(tmp, "w")) == NULL)
        {
            perror("fopen output");
            exit(1);
        }

        /* output header */
        fprintf(o, HEADER, i);
        fprintf(o, "#include <cpu68k-inline.h>\n\n");

        // temporary vars - to for mmu error catching (would be nice in C++ due to try/catch)
        fprintf(o, "\n\nstatic uint8 b1,b2,b3,b4; static uint16 w1,w2,w3,w4; static uint32 l1, l2, l3, l4;\n\n");

        generate(o, i);

        /* close output */
        if (fclose(o))
        {
            perror("fclose output");
            exit(1);
        }
    }

    printf("done.\n");
    fflush(stdout);

    /* normal program termination */
    return (0);
}

void generate(FILE *o, int topnibble)
{
    t_iib *iib;
    int i, flags, pcinc;
    int DEBUG_BRANCH = 0;
    int DEBUG_SR = 0;
    int DEBUG_RTE = 0;

    for (i = 0; i < iibs_num; i++)
    {
        iib = &iibs[i];

        if ((iib->mask & 0xF000) != 0xF000)
        {
            fprintf(stderr, "error: Strange mask %x\n", iib->mask);
            exit(1);
        }
        if (((iib->bits & 0xF000) >> 12) != topnibble)
        {
            continue;
        }

        for (flags = 0; flags < 2; flags++)
        {

            if (flags == 1 && iib->flags.set == 0)
            {
                /* there is no non-flags version, functable will already go
                   straight to the normal version anyway, so lets just skip it */
                continue;
            }

            fprintf(o, "void cpu_op_%i%s(t_ipc *ipc) /* %s */ {\n",
                    i, flags ? "b" : "a", mnemonic_table[iib->mnemonic].name);
            fprintf(o, "  /* mask %04x, bits %04x, mnemonic %d, priv %d, ",
                    iib->mask, iib->bits, iib->mnemonic, iib->flags.priv);
            fprintf(o, "endblk %d, imm_notzero %d, used %d",
                    iib->flags.endblk, iib->flags.imm_notzero, iib->flags.used);
            fprintf(o, "     set %d, size %d, stype %d, dtype %d, sbitpos %d, ",
                    iib->flags.set, iib->size, iib->stype, iib->dtype, iib->sbitpos);
            fprintf(o, "dbitpos %d, immvalue %d */\n", iib->dbitpos,
                    iib->immvalue);

            pcinc = 1;

            switch (iib->mnemonic)
            {

            case i_DIVS:
                GENDBG("");
                /* DIVx is the only instruction that has different sizes for the
                   source and destination! */
                /*
                DIVU.W  <ea>,Dn     32/16 -> 16r:16q
                DIVU.L  <ea>,Dq     32/32 -> 32q      (68020+)
                DIVU.L  <ea>,Dr:Dq  64/32 -> 32r:32q  (68020+)
                DIVUL.L <ea>,Dr:Dq  32/32 -> 32r:32q  (68020+)
                Size = (Word, Long)
                DIVS.W  <ea>,Dn     32/16 -> 16r:16q
                DIVS.L  <ea>,Dq     32/32 -> 32q      (68020+)
                DIVS.L  <ea>,Dr:Dq  64/32 -> 32r:32q  (68020+)
                DIVSL.L <ea>,Dr:Dq  32/32 -> 32r:32q  (68020+)
                Size = (Word, Long)
                */

                if (iib->dtype != dt_Dreg)
                    OUT("ERROR dtype\n");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);               /* 16bit EA */
                generate_ea(o, iib, tp_dst, 1);               /* 32bit Dn */
                OUT("  sint32 dstdata = DATAREG(dstreg);\n"); // <<-- why is this 32 bit?
                OUT("  sint32 quotient;\n");
                OUT("  sint16 remainder;\n");

                // OUT("  ALERT_LOG(0,\"i_DIVS @ %08x src:%08x dest:%08x (dstreg:%d) abort_opcode:%d\",PC,srcdata,dstdata,dstreg,abort_opcode);\n" );

                ABORT_CHECK(o);

                OUT("\n");
                OUT("  if (srcdata == 0) {\n");
                OUT("    ALERT_LOG(0,\"DIVIDE_BY_ZERO @ %08lx src:%08lx dest:%08lx\",(long)PC,(long)srcdata,(long)dstdata);\n");
                OUT("    ZFLAG=0; NFLAG=0;");
                fprintf(o, "    reg68k_internal_vector(V_ZERO, PC+%d,0);\n",
                        (iib->wordlen) * 2);
                OUT("    return;\n");
                OUT("  }\n");
                OUT("  quotient = dstdata / (sint16)srcdata;\n");
                OUT("  remainder = dstdata % (sint16)srcdata;\n");
                OUT("  if (((quotient & 0xffff8000) == 0) ||\n"); // if q==0 or q<0 { if dst<0 != r<0 ??? }
                OUT("      ((quotient & 0xffff8000) == 0xffff8000)) {\n");
                OUT("    if ((dstdata < 0) != (remainder < 0))\n");
                OUT("      remainder = -remainder;\n");
                OUT("    DATAREG(dstreg) = ((uint16)quotient) | ");
                OUT("(((uint16)(remainder))<<16);\n");
                // if (flags && iib->flags.set & IIB_FLAG_V)
                OUT("    VFLAG = 0;\n");
                // if (flags && iib->flags.set & IIB_FLAG_N)
                OUT("    NFLAG = ((sint16)quotient) < 0;\n");
                // if (flags && iib->flags.set & IIB_FLAG_Z)
                OUT("    ZFLAG = !((uint16)quotient);\n");
                // if (flags && (iib->flags.set & IIB_FLAG_V ||
                //               iib->flags.set & IIB_FLAG_N)) {
                OUT("  } else {\n");
                // if (flags && iib->flags.set & IIB_FLAG_V)
                OUT("    VFLAG = 1;\n");
                // if (flags && iib->flags.set & IIB_FLAG_N)
                OUT("    NFLAG = 1;\n");
                //}
                OUT("  }\n");
                //        if (flags && iib->flags.set & IIB_FLAG_C)
                OUT("  CFLAG = 0;\n");
                break;

            case i_ASR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_bits(o, iib);
                OUT("  uint8 count = srcdata & 63;\n");

                ABORT_CHECK(o);

                // ASR 31 loops into 31, bit 0 goes to c/x -- this needs to sign extend!

                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "((sint8)dstdata) >> "
                                             "(count > 7 ? 7 : count)");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "((sint16)dstdata) >> "
                                             "(count > 15 ? 15 : count)");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "((sint32)dstdata) >> "
                                             "(count > 31 ? 31 : count)");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                generate_eastore(o, iib, tp_dst);

                if (flags)
                {
                    OUT("\n");
                    OUT("  if (!count)\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = 0;\n");
                    // OUT("  else if (srcdata >= bits) {\n");  // removed by RA 2004.03.09
                    OUT("  else if (count   >= bits) {\n"); // RA 2004.03.09
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = (dstdata >> (bits - 1)) & 1;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = (dstdata >> (bits - 1)) & 1;\n");
                    OUT("  } else {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = (dstdata >> (count - 1)) & 1;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = (dstdata >> (count - 1)) & 1;\n");
                    OUT("  }\n");
                    if (iib->flags.set & IIB_FLAG_V)
                        generate_clrflag_v(o, iib);
                    if (iib->flags.set & IIB_FLAG_N)
                        generate_stdflag_n(o, iib);
                    if (iib->flags.set & IIB_FLAG_Z)
                        generate_stdflag_z(o, iib);
                }
                break;

            case i_LSR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_bits(o, iib);

                /* cbiere code*/
                OUT("  uint8 count = srcdata & 63;\n");

                generate_outdata(o, iib,
                                 "((count>(bits-1)) ? 0: (dstdata >> count))"); // RA2006.04.03 based on 68K tests

                // generate_outdata(o, iib,
                //          "dstdata >> (count > (bits-1) ? (bits-1) : count)");

                OUT("\n");
                generate_eastore(o, iib, tp_dst);
                if (flags)
                {
                    OUT("\n");
                    OUT("  if (!count)\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = 0;\n");
                    OUT("  else if (count >= bits) {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;\n");
                    OUT("  } else {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = dstdata>>(count-1) & 1;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = dstdata>>(count-1) & 1;\n");
                    OUT("  }\n");
                    if (iib->flags.set & IIB_FLAG_V)
                        generate_clrflag_v(o, iib);
                    if (iib->flags.set & IIB_FLAG_N)
                        generate_stdflag_n(o, iib);
                    if (iib->flags.set & IIB_FLAG_Z)
                        generate_stdflag_z(o, iib);
                }

                /* // original generator code
                OUT("  uint8 count = srcdata & 63;\n");
                generate_outdata(o, iib, "dstdata >> count");
                OUT("\n");
                generate_eastore(o, iib, tp_dst);

                // LSR 0->bit31, 0->c/x
                if (flags) {
                  OUT("\n");
                  OUT("  if (!count)\n");
                  if (iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = 0;\n");
                  OUT("  else if (count >= bits) {\n");
                  OUT("    outdata = 0;\n");
                  if (iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = (count == bits) ? (dstdata >> (bits - 1)) : 0;\n");
                  if (iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = (count == bits) ? (dstdata >> (bits - 1)) : 0;\n");
                  OUT("  } else {\n");
                  if (iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = (dstdata >> (count-1)) & 1;\n");
                  if (iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = (dstdata >> (count-1)) & 1;\n");
                  OUT("  }\n");
                  if (iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                  if (iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                  if (iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                }
                 */
                break;

            case i_ASL:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_bits(o, iib);
                OUT("  uint8 count = srcdata & 63;\n");
                generate_outdata(o, iib,
                                 "count >= bits ? 0 : (dstdata << count)");
                OUT("\n");
                generate_eastore(o, iib, tp_dst);

                // ASL 0 into bit 0 bit 31->c/x

                if (flags)
                {
                    OUT("\n");
                    OUT("  if (!count) {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = 0;\n");
                    if (iib->flags.set & IIB_FLAG_V)
                        OUT("    VFLAG = 0;\n");
                    OUT("  } else if (count >= bits) {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = (count == bits) ? (dstdata & 1) : 0;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = (count == bits) ? (dstdata & 1) : 0;\n");
                    if (iib->flags.set & IIB_FLAG_V)
                        OUT("    VFLAG = dstdata ? 1 : 0;\n");
                    OUT("  } else {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = (dstdata >> (bits-count)) & 1;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = (dstdata >> (bits-count)) & 1;\n");
                    if (iib->flags.set & IIB_FLAG_V)
                    {
                        OUT("    {\n");
                        switch (iib->size)
                        {
                        case sz_byte:
                            OUT("      uint8 mask = 0xff << (7-count);\n")
                            break;
                        case sz_word:
                            OUT("      uint16 mask = 0xffff << (15-count);\n");
                            break;
                        case sz_long:
                            OUT("      uint32 mask = 0xffffffff <<(31-count);\n");
                            break;
                        default:
                            OUT("ERROR size\n");
                            break;
                        }
                        OUT("      VFLAG = ((dstdata & mask) != mask) && ");
                        OUT("((dstdata & mask) != 0);\n");
                        OUT("    }\n");
                        OUT("  }\n");
                    }
                    if (iib->flags.set & IIB_FLAG_N)
                        generate_stdflag_n(o, iib);
                    if (iib->flags.set & IIB_FLAG_Z)
                        generate_stdflag_z(o, iib);
                }
                break;

            case i_LSL:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_bits(o, iib);
                OUT("  uint8 count = srcdata & 63;\n");
                generate_outdata(o, iib,
                                 "(count >= bits) ? 0 : (dstdata << count)");
                OUT("\n");

                // 2006.03.18// LSL bug bw Intel+SPARC vs M68K+PPC
                OUT(" if ((count&63)>=bits && outdata!=0) {outdata=0; ALERT_LOG(0,\"ERROR IN LSL count>=bits\");}\n");

                // LSL 0>bit 0, 31->c/x

                C_ABRT_CHK(o);
                generate_eastore(o, iib, tp_dst);
                if (flags)
                {
                    OUT("\n");
                    OUT("  if (!count)\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = 0;\n");
                    OUT("  else if (count >= bits) {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = (count == bits) ? (dstdata & 1) : 0;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = (count == bits) ? (dstdata & 1) : 0;\n");
                    OUT("  } else {\n");
                    if (iib->flags.set & IIB_FLAG_C)
                        OUT("    CFLAG = (dstdata >> (bits-count)) & 1;\n");
                    if (iib->flags.set & IIB_FLAG_X)
                        OUT("    XFLAG = (dstdata >> (bits-count)) & 1;\n");
                    OUT("  }\n");
                    if (iib->flags.set & IIB_FLAG_V)
                        generate_clrflag_v(o, iib);
                    if (iib->flags.set & IIB_FLAG_N)
                        generate_stdflag_n(o, iib);
                    if (iib->flags.set & IIB_FLAG_Z)
                        generate_stdflag_z(o, iib);
                }
                break;

            case i_OR:
            case i_AND:
            case i_EOR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_outdata(o, iib, "dstdata");
                OUT("\n");

                C_ABRT_CHK(o);

                switch (iib->mnemonic)
                {
                case i_OR:
                    OUT("  outdata|= srcdata;\n");
                    break;
                case i_AND:
                    OUT("  outdata&= srcdata;\n");
                    break;
                case i_EOR:
                    OUT("  outdata^= srcdata;\n");
                    break;
                default:
                    OUT("ERROR\n");
                    break;
                }
                generate_eastore(o, iib, tp_dst);
                C_ABRT_CHK(o);

                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                break;

            case i_ORSR:
            case i_ANDSR:
            case i_EORSR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                OUT("  unsigned int sr = SFLAG;\n");
                OUT("  int oim=IMASK;\n");

                C_ABRT_CHK(o);

                OUT("\n");
                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);
                if (iib->size == sz_word)
                {
                    OUT("  if (!SFLAG)\n");
                    fprintf(o, "    {reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);return;}\n",
                            (iib->wordlen) * 2);
                    OUT("\n");
                }
                switch (iib->mnemonic)
                {
                case i_ORSR:
                    OUT("  SR|= srcdata;\n");
                    break;
                case i_ANDSR:
                    if (iib->size == sz_byte)
                    {
                        OUT("  SR = (SR & 0xFF00) | (SR & srcdata);\n");
                    }
                    else
                    {
                        OUT("  SR&= srcdata;\n");
                    }
                    break;
                case i_EORSR:
                    OUT("  SR^= srcdata;\n");
                    break;
                default:
                    OUT("ERROR\n");
                    break;
                }
                ABORT_CHECK(o);                                // added since this opcode doesn't call eastore
                OUT("  if (sr != SFLAG) {SR_CHANGE()};\n");    // RA20050407
                OUT("  if (oim>IMASK)   {IRQMASKLOWER();}\n"); // RA20050411

                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);
                break;

            case i_SUB:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                C_ABRT_CHK(o);

                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "(sint8)dstdata - (sint8)srcdata");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "(sint16)dstdata - (sint16)srcdata");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "(sint32)dstdata - (sint32)srcdata");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                generate_eastore(o, iib, tp_dst);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_subflag_v(o, iib);
                if (flags && ((iib->flags.set & IIB_FLAG_C) ||
                              (iib->flags.set & IIB_FLAG_X)))
                    generate_subflag_cx(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_SUBA:
                GENDBG("");
                if (iib->dtype != dt_Areg)
                    OUT("Error\n");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                OUT("  uint32 dstdata = ADDRREG(dstreg);\n");
                switch (iib->size)
                {
                case sz_byte:
                    OUT("  uint32 outdata = (sint32)dstdata - (sint8)srcdata;\n");
                    break;
                case sz_word:
                    OUT("  uint32 outdata = (sint32)dstdata - (sint16)srcdata;\n");
                    break;
                case sz_long:
                    OUT("  uint32 outdata = (sint32)dstdata - (sint32)srcdata;\n");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                C_ABRT_CHK(o);
                //                    ABORT_CHECK(o);      // added since this opcode doesn't call eastore
                OUT("  ADDRREG(dstreg) = outdata;\n");
                break;

            case i_SUBX:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);

                C_ABRT_CHK(o);

                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "(sint8)dstdata - (sint8)srcdata "
                                             "- XFLAG");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "(sint16)dstdata - (sint16)srcdata"
                                             "- XFLAG");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "(sint32)dstdata - (sint32)srcdata"
                                             "- XFLAG");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                generate_eastore(o, iib, tp_dst);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_subflag_v(o, iib);
                if (flags && ((iib->flags.set & IIB_FLAG_C) ||
                              (iib->flags.set & IIB_FLAG_X)))
                    generate_subxflag_cx(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdxflag_z(o, iib);
                break;

            case i_ADD:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                C_ABRT_CHK(o);
                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "(sint8)dstdata + (sint8)srcdata");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "(sint16)dstdata + (sint16)srcdata");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "(sint32)dstdata + (sint32)srcdata");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                generate_eastore(o, iib, tp_dst);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_addflag_v(o, iib);
                if (flags && ((iib->flags.set & IIB_FLAG_C) ||
                              (iib->flags.set & IIB_FLAG_X)))
                    generate_addflag_cx(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_ADDA:
                GENDBG("");
                if (iib->dtype != dt_Areg)
                    OUT("Error\n");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);

                OUT("  uint32 dstdata = ADDRREG(dstreg);\n");

                switch (iib->size)
                {
                case sz_byte:
                    OUT("  uint32 outdata = (sint32)dstdata + (sint8)srcdata;\n");
                    break;
                case sz_word:
                    OUT("  uint32 outdata = (sint32)dstdata + (sint16)srcdata;\n");
                    break;
                case sz_long:
                    OUT("  uint32 outdata = (sint32)dstdata + (sint32)srcdata;\n");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                C_ABRT_CHK(o);
                OUT("  ADDRREG(dstreg) = outdata;\n");
                break;

            case i_ADDX:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                ABORT_CHECK(o);
                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "(sint8)dstdata + (sint8)srcdata "
                                             "+ XFLAG");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "(sint16)dstdata + (sint16)srcdata"
                                             "+ XFLAG");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "(sint32)dstdata + (sint32)srcdata"
                                             "+ XFLAG");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");

                generate_eastore(o, iib, tp_dst);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_addflag_v(o, iib);
                if (flags && ((iib->flags.set & IIB_FLAG_C) ||
                              (iib->flags.set & IIB_FLAG_X)))
                    generate_addxflag_cx(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdxflag_z(o, iib);
                break;

            case i_MULU:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                OUT("\n");
                OUT("  uint32 outdata = (uint32)srcdata * (uint32)dstdata;\n");
                if (iib->dtype != dt_Dreg)
                    OUT("ERROR dtype\n");
                C_ABRT_CHK(o);
                OUT("  DATAREG(dstreg) = outdata;\n");
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_N)
                    OUT("  NFLAG = ((sint32)outdata) < 0;\n");
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                break;

            case i_MULS:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);

                OUT("\n");
                OUT("  uint32 outdata = (sint32)(sint16)srcdata * "
                    "(sint32)(sint16)dstdata;\n");
                if (iib->dtype != dt_Dreg)
                    OUT("ERROR dtype\n");

                C_ABRT_CHK(o);

                OUT("  DATAREG(dstreg) = outdata;\n");
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_N)
                    OUT("  NFLAG = ((sint32)outdata) < 0;\n");
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                break;

            case i_CMP:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "(sint8)dstdata - (sint8)srcdata");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "(sint16)dstdata - (sint16)srcdata");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "(sint32)dstdata - (sint32)srcdata");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                C_ABRT_CHK(o);

                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_subflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_subflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_CMPA:
                GENDBG("");
                if (iib->dtype != dt_Areg || iib->size != sz_word)
                    OUT("Error\n");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                iib->size = sz_long;
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                OUT("  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;\n");

                C_ABRT_CHK(o);

                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_cmpaflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_cmpaflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                iib->size = sz_word;
                break;

            case i_BTST:
            case i_BCHG:
            case i_BCLR:
            case i_BSET:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);

                switch (iib->size)
                {
                case sz_byte:
                    OUT("  uint32 bitpos = 1<<(srcdata & 7);");
                    break;
                case sz_long:
                    OUT("  uint32 bitpos = 1<<(srcdata & 31);");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                C_ABRT_CHK(o);
                OUT("\n");
                switch (iib->mnemonic)
                {
                case i_BTST:

                    break;
                case i_BCHG:
                    generate_outdata(o, iib, "dstdata ^ bitpos");
                    generate_eastore(o, iib, tp_dst);
                    break;
                case i_BCLR:
                    generate_outdata(o, iib, "dstdata & ~bitpos");
                    generate_eastore(o, iib, tp_dst);
                    break;
                case i_BSET:
                    generate_outdata(o, iib, "dstdata | bitpos");
                    generate_eastore(o, iib, tp_dst);
                    break;
                default:
                    OUT("ERROR\n");
                    break;
                }
                OUT("\n");
                OUT("  ZFLAG = !(dstdata & bitpos);\n");
                break;

            case i_MOVE:
#ifdef XXXDEBUG
                if (iib->bits == 0x23b0 || iib->bits == 0x33b0 || iib->bits == 0x31b0)
                    gendbg = 1;
#endif
                // OUT("// generate_ea(o,iib,tp_src):\n"); // likely this guy
                generate_ea(o, iib, tp_src, 1);
                // OUT("// generate_eaval(o, iib, tp_src):\n"); // or this guy.
                generate_eaval(o, iib, tp_src);
                OUT("\n");

                // OUT("// generate_ea(o, iib, tp_dst, 1);:\n");
                generate_ea(o, iib, tp_dst, 1);
                OUT("\n");

                // OUT("// generate_outdata(o, iib, srcdata);:\n");
                generate_outdata(o, iib, "srcdata"); // maybe
                OUT("\n");
                // OUT("// generate_eastore(o, iib, tp_dst);:\n")
                generate_eastore(o, iib, tp_dst);
// OUT("//\n");
// GENDBG("");
#ifdef XXXDEBUG
                // if ( iib->bits == 0x23b0 || iib->bits==0x33b0 || iib->bits==0x31b0) {
                if ((iib->bits & iib->mask) == 0x31b0)
                { //(ipc->opcode==0x23b0 || ipc->opcode==0x33b0 || ipc->opcode==0x31b0)
                    OUT(" if ((ipc->opcode & 0xf1f8)==0x31b0)  {\n");
                    OUT("    fprintf(stderr,\"\\n\\n\\n%s:%s:%d: buggy bug is here.\\n\\n\\n\",__FILE__,__FUNCTION__,__LINE__);\n");
                    OUT("    fprintf(stderr,\"srcreg:%d dstreg:%d \\n\",srcreg,dstreg);\n");
                    OUT("    fprintf(stderr,\"idxval_dst(ipc):%08x   idxval_src(ipc):%08x :%s \\n\", idxval_dst(ipc), idxval_src(ipc),(idxval_dst(ipc)==idxval_src(ipc))?\"BROKEN\":\"\" );\n");
                    OUT("    fprintf(stderr,\"ipc->dst:%08x ipc->src:%08x ipc->reg s/d:%04x/%04x ipc->wordlen:%d\\n\", ipc->dst, ipc->src,ipc->sreg,ipc->dreg,ipc->wordlen);\n");
                    OUT("    fprintf(stderr,\"dstaddr:%08x srcaddr:%08x :%s\\n\",dstaddr,srcaddr,dstaddr==srcaddr ? \"BROKEN\":\"\");");
                    OUT("    fprintf(stderr,\" A0:%08x D0:%08x A1:%08x D1:%08x \\n\\n\",ADDRREG(0),DATAREG(0),ADDRREG(1),DATAREG(1) );\n");
                    OUT(" }\n");
                }
#endif

                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);

#ifdef XXXDEBUG
                gendbg = 0;
#endif

                break;

            case i_MOVEA:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                ABORT_CHECK(o);
                if (iib->dtype != dt_Areg || iib->size != sz_word)
                    OUT("Error\n");
                OUT("\n");
                OUT("  ADDRREG(dstreg) = (sint32)(sint16)srcdata;\n");
                break;

            case i_MOVEPMR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                switch (iib->size)
                {
                case sz_word:
                    generate_outdata(o, iib, "0");
                    OUT("b1=fetchbyte(srcaddr);  \n");
                    OUT("b2=fetchbyte(srcaddr+2);\n");

                    ABORT_CHECK(o);

                    OUT("outdata=(b1<<8)+(b2);\n");
                    break;

                case sz_long:
                    generate_outdata(o, iib, "0");

                    OUT("b1= fetchbyte(srcaddr)  ;\n");
                    OUT("b2= fetchbyte(srcaddr+2);\n");
                    OUT("b3= fetchbyte(srcaddr+4);\n");
                    OUT("b4= fetchbyte(srcaddr+6);\n");
                    ABORT_CHECK(o);

                    OUT("outdata=((b1<<24)+(b2<<16)+(b3<<8)+b4);\n");

                    break;

                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                generate_eastore(o, iib, tp_dst);
                break;

            case i_MOVEPRM:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);

                OUT("\n");
                C_ABRT_CHK(o);
                // no need to for ABORT_CHECK after the following storebytes because the fn returns right after.
                // so there's nothing to abort
                switch (iib->size)
                {
                case sz_word:
                    OUT("  storebyte(dstaddr, (srcdata >> 8) & 0xFF);\n");
                    OUT("  storebyte(dstaddr+2, srcdata & 0xFF);\n");
                    break;
                case sz_long:
                    OUT("  storebyte(dstaddr, (srcdata >> 24) & 0xFF);\n");
                    OUT("  storebyte(dstaddr+2, (srcdata >> 16) & 0xFF);\n");
                    OUT("  storebyte(dstaddr+4, (srcdata >> 8) & 0xFF);\n");
                    OUT("  storebyte(dstaddr+6, srcdata & 0xFF);\n");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                break;

            case i_MOVEFSR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_outdata(o, iib, NULL);
                OUT("\n");
                OUT("  outdata = SR;\n");
                generate_eastore(o, iib, tp_src);
                break;

            case i_MOVETSR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);

                C_ABRT_CHK(o);

                OUT("  unsigned int sr = SFLAG;\n");
                OUT("  int oim=IMASK;\n"); // RA20050411
                OUT("\n");
                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);
                switch (iib->size)
                {
                case sz_byte:
                    OUT("  SR = (SR & ~0xFF) | srcdata;\n");
                    break;
                case sz_word:

                    OUT("  if (!SFLAG)\n");
                    fprintf(o, "    {reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);return;}\n",
                            (iib->wordlen) * 2);
                    OUT("\n");

                    OUT("\n");
                    ABORT_CHECK(o); // added since this opcode doesn't call eastore
                    OUT("  SR = srcdata;\n");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("  if (sr != SFLAG) {SR_CHANGE();}\n");
                OUT("  if (oim>IMASK)   {IRQMASKLOWER();}\n"); // RA20050411

                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);
                break;

            case i_MOVEMRM:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 0);

                if (iib->dtype == dt_Adec)
                {
                    OUT("  uint8 datamask = (srcdata & 0xFF00) >> 8;\n");
                    OUT("  uint8 addrmask = srcdata & 0xFF;");
                    OUT("\n");
                    ABORT_CHECK(o);
                    switch (iib->size)
                    {
                    case sz_word:
                        OUT("  while (addrmask) {\n");
                        OUT("    dstaddr-= 2;\n");
                        OUT("    storeword(dstaddr, ADDRREG((7-movem_bit[addrmask])));\n");
                        ABORT_CHECK(o);
                        OUT("    DEBUG_LOG(5,\"reg A%d.W written to %08x<-%04x  SRC:\",7-movem_bit[addrmask],dstaddr,ADDRREG((7-movem_bit[addrmask])));\n")
                        OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
                        OUT("  }\n");
                        OUT("  while (datamask) {\n");
                        OUT("    dstaddr-= 2;\n");
                        OUT("    storeword(dstaddr, DATAREG((7-movem_bit[datamask])));\n");
                        OUT("    DEBUG_LOG(5,\"reg D%d.W written to %08x <-%04x  SRC:\",7-movem_bit[datamask],dstaddr,DATAREG((7-movem_bit[datamask])));\n")
                        ABORT_CHECK(o);
                        OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
                        OUT("  }\n");
                        break;
                    case sz_long:
                        OUT("  while (addrmask) {\n");
                        OUT("    dstaddr-= 4;\n");
                        OUT("    storelong(dstaddr, ADDRREG((7-movem_bit[addrmask])));\n");
                        ABORT_CHECK(o);
                        OUT("    DEBUG_LOG(5,\"reg A%d.L written to %08x<-%08x  SRC:\",7-movem_bit[addrmask],dstaddr,ADDRREG((7-movem_bit[addrmask])));\n")
                        OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
                        OUT("  }\n");
                        OUT("  while (datamask) {\n");
                        OUT("    dstaddr-= 4;\n");
                        OUT("    storelong(dstaddr, DATAREG((7-movem_bit[datamask])));\n");
                        ABORT_CHECK(o);
                        OUT("    DEBUG_LOG(5,\"reg D%d.L written to %08x<-%08x  SRC:\",7-movem_bit[datamask],dstaddr,DATAREG((7-movem_bit[datamask])));\n")
                        OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
                        OUT("  }\n");
                        break;
                    default:
                        OUT("ERROR\n");
                        break;
                    }
                    OUT("  ADDRREG(dstreg) = dstaddr;");
                }
                else
                {
                    OUT("  uint8 addrmask = (srcdata & 0xFF00) >> 8;\n");
                    OUT("  uint8 datamask = srcdata & 0xFF;");
                    OUT("\n");
                    ABORT_CHECK(o);
                    switch (iib->size)
                    {
                    case sz_word:
                        OUT("  while (datamask) {\n");
                        OUT("    storeword(dstaddr, DATAREG(movem_bit[datamask]));\n");
                        ABORT_CHECK(o);
                        OUT("    DEBUG_LOG(5,\"reg D%d.W written to (%08x)<-%04x  SRC:\",movem_bit[datamask],dstaddr,DATAREG(movem_bit[datamask]));\n")
                        OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
                        OUT("    dstaddr+= 2;\n");
                        OUT("  }\n");
                        OUT("  while (addrmask) {\n");
                        OUT("    storeword(dstaddr, ADDRREG(movem_bit[addrmask]));\n");
                        OUT("    DEBUG_LOG(5,\"reg A%d.W written to (%08x)<-%04x  SRC:\",movem_bit[addrmask],dstaddr,ADDRREG(movem_bit[addrmask]));\n")
                        ABORT_CHECK(o);
                        OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
                        OUT("    dstaddr+= 2;\n");
                        OUT("  }\n");
                        break;
                    case sz_long:
                        OUT("  while (datamask) {\n");
                        OUT("    storelong(dstaddr, DATAREG(movem_bit[datamask]));\n");
                        ABORT_CHECK(o);
                        OUT("    DEBUG_LOG(5,\"reg D%d.L written to (%08x)<-%08x  SRC:\",movem_bit[datamask],dstaddr,DATAREG(movem_bit[datamask]));\n")
                        OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
                        OUT("    dstaddr+= 4;\n");
                        OUT("  }\n");
                        OUT("  while (addrmask) {\n");
                        OUT("    storelong(dstaddr, ADDRREG(movem_bit[addrmask]));\n");
                        ABORT_CHECK(o);
                        OUT("    DEBUG_LOG(5,\"reg A%d.L written to (%08x)<-%08x  SRC:\",movem_bit[addrmask],dstaddr,ADDRREG(movem_bit[addrmask]));\n")
                        OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
                        OUT("    dstaddr+= 4;\n");
                        OUT("  }\n");
                        break;
                    default:
                        OUT("ERROR\n");
                        break;
                    }
                    if (iib->dtype == dt_Ainc)
                    {
                        /* not supported */
                        OUT("ERROR\n");
                    }
                }
                break;

            case i_MOVEMMR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 0);
                OUT("  uint8 addrmask = (srcdata & 0xFF00) >> 8;\n");
                OUT("  uint8 datamask = srcdata & 0xFF;");
                OUT("\n");
                ABORT_CHECK(o);
                switch (iib->size)
                {
                case sz_word:
                    OUT("  while (datamask) {\n");

                    OUT("    w1=fetchword(dstaddr);");
                    ABORT_CHECK(o);
                    OUT("    DATAREG(movem_bit[datamask]) = ");
                    OUT("(sint32)(sint16)w1;\n");
                    OUT("    DEBUG_LOG(5,\"reg D%d.W read from (%08x)->%04x  SRC:\",movem_bit[datamask],dstaddr,DATAREG(movem_bit[datamask]));\n")
                    OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
                    OUT("    dstaddr+= 2;\n");
                    OUT("  }\n");
                    OUT("  while (addrmask) {\n");
                    OUT("    ADDRREG(movem_bit[addrmask]) = ");
                    OUT("(sint32)(sint16)fetchword(dstaddr);\n");
                    ABORT_CHECK(o);
                    OUT("    DEBUG_LOG(5,\"reg A%d.W read from (%08x)->%04x  SRC:\",movem_bit[addrmask],dstaddr,ADDRREG(movem_bit[addrmask]));\n")
                    OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
                    OUT("    dstaddr+= 2;\n");
                    OUT("  }\n");
                    break;
                case sz_long:
                    OUT("  while (datamask) {\n");
                    OUT("    l1=fetchlong(dstaddr);");
                    ABORT_CHECK(o);
                    OUT("    DATAREG(movem_bit[datamask]) = l1;\n");
                    OUT("    DEBUG_LOG(5,\"reg D%d.L read from (%08x)->%08x  SRC:\",movem_bit[datamask],dstaddr,DATAREG(movem_bit[datamask]));\n")
                    OUT("    datamask&= ~(1<<movem_bit[datamask]);\n");
                    OUT("    dstaddr+= 4;\n");
                    OUT("  }\n");
                    OUT("  while (addrmask) {\n");
                    OUT("    l1=fetchlong(dstaddr);");
                    ABORT_CHECK(o);
                    OUT("    ADDRREG(movem_bit[addrmask]) = l1;\n");
                    OUT("    DEBUG_LOG(5,\"reg A%d.L read from (%08x)->%08x  SRC:\",movem_bit[addrmask],dstaddr,ADDRREG(movem_bit[addrmask]));\n")
                    OUT("    addrmask&= ~(1<<movem_bit[addrmask]);\n");
                    OUT("    dstaddr+= 4;\n");
                    OUT("  }\n");
                    break;
                default:
                    OUT("ERROR\n");
                    break;
                }
                if (iib->dtype == dt_Ainc)
                {
                    OUT("  ADDRREG(dstreg) = dstaddr;\n");
                }
                else if (iib->dtype == dt_Adec)
                {
                    /* not supported */
                    OUT("ERROR\n");
                }
                break;

            case i_MOVETUSP:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                C_ABRT_CHK(o);

                OUT("\n");
                OUT("  if (!SFLAG)\n");
                fprintf(o, "    {reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);return;}\n",
                        (iib->wordlen) * 2);
                OUT("\n");

                OUT("  SP = srcdata;\n");
                break;

            case i_MOVEFUSP:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                OUT("  uint32 outdata;\n");
                OUT("\n");
                OUT("  if (!SFLAG)\n");
                fprintf(o, "    {reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);return;}\n",
                        (iib->wordlen) * 2);
                OUT("\n");

                OUT("  outdata = SP;\n");
                generate_eastore(o, iib, tp_src);
                break;

            case i_NEG:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                ABORT_CHECK(o);
                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "0 - (sint8)srcdata");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "0 - (sint16)srcdata");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "0 - (sint32)srcdata");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");
                generate_eastore(o, iib, tp_src);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_negflag_v(o, iib);
                if (flags && ((iib->flags.set & IIB_FLAG_C) ||
                              (iib->flags.set & IIB_FLAG_X)))
                    generate_negflag_cx(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_NEGX:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                switch (iib->size)
                {
                case sz_byte:
                    generate_outdata(o, iib, "0 - (sint8)srcdata - XFLAG");
                    break;
                case sz_word:
                    generate_outdata(o, iib, "0 - (sint16)srcdata - XFLAG");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "0 - (sint32)srcdata - XFLAG");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                OUT("\n");

                generate_eastore(o, iib, tp_src);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_negxflag_v(o, iib);
                if (flags && ((iib->flags.set & IIB_FLAG_C) ||
                              (iib->flags.set & IIB_FLAG_X)))
                    generate_negxflag_cx(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdxflag_z(o, iib);
                break;

            case i_CLR:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src); /* read before write */
                generate_outdata(o, iib, "0");
                OUT("\n");
                C_ABRT_CHK(o);

                generate_eastore(o, iib, tp_src);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_clrflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_setflag_z(o, iib);
                break;

            case i_NOT:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src); /* read before write */
                generate_outdata(o, iib, "~srcdata");
                C_ABRT_CHK(o);
                OUT("\n");

                generate_eastore(o, iib, tp_src);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_ABCD:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                C_ABRT_CHK(o);
                generate_outdata(o, iib, NULL);

                // #ifdef BCD_NV_SAME                     //save NV
                //   OUT("int vflagcpy=VFLAG;\n");
                //   OUT("int nflagcpy=NFLAG;\n");
                // #endif

                OUT("\n");
                OUT("  uint8 outdata_low = (dstdata & 0xF) + (srcdata & 0xF) ");
                OUT("+ XFLAG;\n");
                OUT("  uint16 precalc = dstdata + srcdata + XFLAG;\n");
                OUT("  uint16 outdata_tmp = precalc;\n");
                OUT("\n");
                OUT("  if (outdata_tmp > 0x99) {\n");
                OUT("    outdata_tmp+= 0x60;\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = 1;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = 1;\n");
                OUT("  } else {\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = 0;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = 0;\n");
                OUT("  }\n");
                OUT("  if (outdata_low > 0x09)\n");
                OUT("    outdata_tmp+= 0x06;\n");
                OUT("  outdata = outdata_tmp;\n");
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    OUT("  if (outdata) ZFLAG = 0;\n");

#ifndef BCD_NV_SAME
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                    OUT("  VFLAG = ((precalc & 1<<7) == 0) && (outdata & 1<<7);\n");
#endif

                OUT("  ALERT_LOG(0,\"BCD  @ %lx\", (long)PC);\n"); // 2018.03.10 RA

                generate_eastore(o, iib, tp_dst);

                break;

            case i_SBCD:
                GENDBG("");
                /* on real 68000: 0x10 - 0x0b - 0 = 0xff */
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_outdata(o, iib, NULL);

#ifdef BCD_NV_SAME // save NV
                OUT("int vflagcpy=VFLAG;\n");
                OUT("int nflagcpy=NFLAG;\n");
#endif

                OUT("\n");
                OUT("  sint8 outdata_low = (dstdata & 0xF) - (srcdata & 0xF) ");
                OUT("- XFLAG;\n");
                OUT("  sint16 precalc = dstdata - srcdata - XFLAG;\n");
                OUT("  sint16 outdata_tmp = precalc;\n");
                OUT("\n");
                C_ABRT_CHK(o);
                OUT("  if (outdata_tmp < 0) {\n");
                OUT("    outdata_tmp-= 0x60;\n");
                if (flags && iib->flags.set & IIB_FLAG_N)
                    OUT("    NFLAG = 1;\n");
                OUT("  } else {\n");

                if (flags && iib->flags.set & IIB_FLAG_N)
                    OUT("    NFLAG = 0;\n");

                OUT("  }\n");

                OUT("  if (outdata_low < 0)\n");
                OUT("    outdata_tmp-= 0x06;\n");
                OUT("  if (outdata_tmp < 0) {\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = 1;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = 1;\n");
                OUT("  } else {\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = 0;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = 0;\n");
                OUT("  }\n");
                OUT("  outdata = outdata_tmp;\n");
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    OUT("  if (outdata) ZFLAG = 0;\n");

#ifndef BCD_NV_SAME
                if (flags && iib->flags.set & IIB_FLAG_V)
                    OUT("  VFLAG = (precalc & 1<<7) && ((outdata & 1<<7) == 0);\n");
#endif
                generate_eastore(o, iib, tp_dst);

#ifdef BCD_NV_SAME // save NV
                OUT("VFLAG=vflagcpy;\n");
                OUT("NFLAG=nflagcpy;\n");
#endif

                OUT("  ALERT_LOG(0,\"BCD  @ %lx\", (long)PC);\n"); // 2018.03.10 RA

                break;

            case i_NBCD:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_outdata(o, iib, NULL);
                OUT("\n");
                OUT("  sint8 outdata_low = - (srcdata & 0xF) - XFLAG;\n");
                OUT("  sint16 precalc = - srcdata - XFLAG;\n");
                OUT("  sint16 outdata_tmp = precalc;\n");
                OUT("\n");
                C_ABRT_CHK(o);

                /*  cbiere code */
                OUT("  if (outdata_low < 0)\n");
                OUT("    outdata_tmp-= 0x06;\n");
                OUT("  if (outdata_tmp < 0) {\n");
                OUT("    outdata_tmp-= 0x60;\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = 1;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = 1;\n");
                OUT("  } else {\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("    CFLAG = 0;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                    OUT("    XFLAG = 0;\n");
                OUT("  }\n");
                OUT("  outdata = outdata_tmp;\n");
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    OUT("  if (outdata) ZFLAG = 0;\n");

#ifndef BCD_NV_SAME
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                    OUT("  VFLAG = (precalc & 1<<7) && ((outdata & 1<<7) == 0);\n");
#endif
                generate_eastore(o, iib, tp_src);

                OUT("  ALERT_LOG(0,\"BCD  @ %lx\", (long)PC);\n"); // 2018.03.10 RA

                // nbcd

                /* // original generator 3.5 code
                OUT("  if (outdata_low < 0)\n");
                OUT("    outdata_tmp-= 0x06;\n");
                OUT("  if (outdata_tmp < 0) {\n");
                OUT("    outdata_tmp-= 0x60;\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                  OUT("    CFLAG = 1;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                  OUT("    XFLAG = 1;\n");
                OUT("  } else {\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                  OUT("    CFLAG = 0;\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                  OUT("    XFLAG = 0;\n");
                OUT("  }\n");
                OUT("  outdata = outdata_tmp;\n");
                if (flags && iib->flags.set & IIB_FLAG_Z)
                  OUT("  if (outdata) ZFLAG = 0;\n");
                if (flags && iib->flags.set & IIB_FLAG_N)
                  generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                  OUT("  VFLAG = (precalc & 1<<7) && ((outdata & 1<<7) == 0);\n");
                generate_eastore(o, iib, tp_src);

                */
                break;

            case i_SWAP:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_outdata(o, iib, "(srcdata>>16) | (srcdata<<16)");
                OUT("\n");
                C_ABRT_CHK(o);
                generate_eastore(o, iib, tp_src);

                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_PEA:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                ABORT_CHECK(o);

                OUT("\n");
                OUT("  ADDRREG(7)-= 4;\n");
                OUT("  storelong(ADDRREG(7), srcaddr);\n");
                ABORT_CHECK(o); // added since this opcode doesn't call eastore
                break;

            case i_LEA:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_ea(o, iib, tp_dst, 1);
                generate_outdata(o, iib, "srcaddr");
                OUT("\n");
                C_ABRT_CHK(o);

                generate_eastore(o, iib, tp_dst);
                break;

            case i_EXT:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                OUT("\n");
                C_ABRT_CHK(o);
                switch (iib->size)
                {
                case sz_word:
                    generate_outdata(o, iib, "(sint16)(sint8)(srcdata)");
                    break;
                case sz_long:
                    generate_outdata(o, iib, "(sint32)(sint16)(srcdata)");
                    break;
                default:
                    fprintf(o, "ERROR size\n");
                    break;
                }
                generate_eastore(o, iib, tp_src);

                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_EXG:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                C_ABRT_CHK(o);
                OUT("\n");
                switch (iib->dtype)
                {
                case dt_Dreg:
                    OUT("  DATAREG(dstreg) = srcdata;\n");
                    break;
                case dt_Areg:
                    OUT("  ADDRREG(dstreg) = srcdata;\n");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                switch (iib->stype)
                {
                case dt_Dreg:
                    OUT("  DATAREG(srcreg) = dstdata;\n");
                    break;
                case dt_Areg:
                    OUT("  ADDRREG(srcreg) = dstdata;\n");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                break;

            case i_TST:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_outdata(o, iib, "srcdata");
                C_ABRT_CHK(o);
                if (flags)
                {
                    OUT("\n");
                    ABORT_CHECK(o); // added since this opcode doesn't call eastore
                }
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                break;

            case i_TAS:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_outdata(o, iib, "srcdata");
                C_ABRT_CHK(o);
                if (flags)
                {
                    OUT("\n");
                }
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_C)
                    generate_clrflag_c(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                switch (iib->size)
                {
                case sz_byte:
                    OUT("  outdata|= 1<<7;\n");
                    break;
                case sz_word:
                    OUT("  outdata|= 1<<15;\n");
                    break;
                case sz_long:
                    OUT("  outdata|= 1<<31;\n");
                    break;
                default:
                    OUT("ERROR size\n");
                    break;
                }
                generate_eastore(o, iib, tp_src);
                break;

            case i_CHK:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                ABORT_CHECK(o);
                fprintf(o, "\n");
                if (iib->size != sz_word)
                    OUT("ERROR size\n");

                fprintf(o, "  ZFLAG=(!dstdata); //RA2020.07.12\n"); // != srcdata); //RA20200712\n");  // 2020.07.12 fix

                fprintf(o, "  if ((sint16)srcdata < 0) {\n");
                if (flags)
                    OUT("    NFLAG = 1;\n");
                fprintf(o, "    reg68k_internal_vector(V_CHK, PC+%d,0);\n",
                        (iib->wordlen) * 2);
                OUT("  } else if (dstdata > srcdata) {\n");
                if (flags)
                    OUT("    NFLAG = 0;\n");
                fprintf(o, "    reg68k_internal_vector(V_CHK, PC+%d,0);\n",
                        (iib->wordlen) * 2);
                OUT("  }\n");
                break;

            case i_TRAPV:
                GENDBG("");
                OUT("  if (VFLAG) {\n");
                fprintf(o, "    reg68k_internal_vector(V_TRAPV, PC+%d,0);\n",
                        (iib->wordlen) * 2);
                OUT("  }\n");
                break;

            case i_TRAP:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                C_ABRT_CHK(o);
                OUT("\n");

                fprintf(o, "   #ifdef DEBUG\n");
                fprintf(o, "   trap_opcode(srcdata);\n");
                fprintf(o, "   #endif\n");

                fprintf(o, "  reg68k_internal_vector(V_TRAP+srcdata, PC+%d,0);\n",
                        (iib->wordlen) * 2);
                pcinc = 0;
                break;

            case i_RESET:
                OUT("  /* printf(\"RESET opcode executed @ %x\\n\", PC);*/ \n");
                OUT("  if (!SFLAG)\n");
                OUT("  {ALERT_LOG(0,\"RESET opcode executed @ %lx WITHOUT SUPERVISOR ON! Entering V_PRIVILEGE exception\\n\", (long)PC);\n");
                fprintf(o, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);return;}\n",
                        (iib->wordlen) * 2);
                OUT("\n");
                OUT("  mc68k_reset();\n"); // this just flips the extern RESET wire to reset I/O
                // devices.  It shouldn't stop the emulation. RA 2002.08.02
                break;

            case i_NOP:
                GENDBG("");
                break;

            case i_STOP:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                C_ABRT_CHK(o);
                OUT("\n");
                OUT("  if (regs.stop)\n");
                OUT("    return;\n");
                OUT("  if (!(SFLAG && (srcdata & 1<<13))) {\n");
                fprintf(o, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);\n",
                        (iib->wordlen) * 2);
                fprintf(o, "    PC+= %d;\n", (iib->wordlen) * 2);
                OUT("  } else {\n");
                OUT("    SR = srcdata;\n");
                OUT("    STOP = 1;\n");
                OUT("  }\n");
                pcinc = 0;
                break;

            case i_LINK:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                C_ABRT_CHK(o);
                if (iib->stype != dt_ImmW)
                    OUT("ERROR stype\n");
                OUT("\n");
                OUT("  ADDRREG(7)-= 4;\n");
                OUT("  storelong(ADDRREG(7), dstdata);\n");
                ABORT_CHECK(o); // added since this opcode doesn't call eastore
                OUT("  ADDRREG(dstreg) = ADDRREG(7);\n");
                OUT("  ADDRREG(7)+= (sint16)srcdata;\n");
                break;

            case i_UNLK: //       An -->; SP; (SP) -->; An; SP + 4 -->; SP
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                OUT("\n");
                // OUT("  ADDRREG(srcreg) = fetchlong(srcdata);\n");
                OUT("  l1=fetchlong(srcdata);");
                ABORT_CHECK(o);
                OUT("  ADDRREG(srcreg) =l1;\n");
                OUT("  ADDRREG(7) = srcdata+4;\n");
                break;

            case i_RTE:
                GENDBG("");
                ///// OUT("  uint16 FV=0; // addedd by RA for format/vector word\n");
                if (DEBUG_RTE)
                    fputs("  printf(\"RTE: 0x%X\\n\", PC);\n", o);
                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);
                //    OUT("  if (!SFLAG)\n");
                //  fprintf(o, "    reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);\n",
                //  (iib->wordlen)*2);
                //  OUT("\n");
                OUT("  if (!SFLAG)\n");
                fprintf(o, "    {reg68k_internal_vector(V_PRIVILEGE, PC+%d,0);return;}\n",
                        (iib->wordlen) * 2);
                OUT("\n");

                // RTE needs to POP off: SR (16 bit),PC (32 bit),Format (16 bit)

                OUT("w1=fetchword(ADDRREG(7));"); // SR
                OUT("w2=SFLAG;");                 /// was SR.sr_struct.s
                OUT("w3=IMASK;\n");               // RA20050411

                OUT("l1=fetchlong(ADDRREG(7)+2);"); // new PC
                ABORT_CHECK(o);

                OUT("  ADDRREG(7)+= 6;\n"); // fix stack

                OUT("  SR = w1;\n");

                // OUT("  FV = fetchword(ADDRREG(7)+6);   // added by RA for format/vector word\n");  // not applicable to 68000
                // OUT("  ADDRREG(7)+= 8;                 // \n\n");

                // OUT("  if ((FV & 0xf000)>0x1000)       // \n");
                // fprintf(o, "    {reg68k_internal_vector(V_FORMAT, PC+%d,0);return;}\n\n",(iib->wordlen)*2);

                OUT("  if (w2!=SFLAG) {SR_CHANGE();}\n");    // was SFLAG -- ra
                OUT("  if (w3>IMASK)  {IRQMASKLOWER();}\n"); // RA20050411

                OUT("fetchword(l1);"); // not sure about this 20060116
                ABORT_CHECK(o);        // not sure about this 20060116

                OUT("  PC = l1;\n");

                if (DEBUG_RTE)
                    fputs("  printf(\"RTE: ->0x%X\\n\", PC);\n", o);
                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);
                pcinc = 0;
                break;

            case i_RTS:
                GENDBG("");
                if (DEBUG_BRANCH)
                    fputs("  printf(\"RTS: 0x%X\\n\", PC);", o);
                OUT("l1=fetchlong(ADDRREG(7));");

                OUT("  ADDRREG(7)+= 4;\n"); // 20060116
                ABORT_CHECK(o);

                OUT("  PC = l1;\n");

                if (DEBUG_BRANCH)
                    fputs("  printf(\"RTS: ->0x%X\\n\", PC);", o);
#ifdef DEBUG
                fputs("  if (!(PC)) {extern void dumpallscreenshot(void); dumpallscreenshot();}", o);
#endif
                pcinc = 0;
                break;

            case i_RTR:
                GENDBG("");
                if (DEBUG_BRANCH)
                    fputs("  printf(\"RTR: 0x%X\\n\", PC);\n", o);
                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);

                OUT("w1=fetchword(ADDRREG(7));");
                OUT("l1=fetchlong(ADDRREG(7)+2);");
                ABORT_CHECK(o);

                OUT("  SR = (SR & ~0xFF) | (w1 & 0xFF);\n");
                OUT("  PC = l1;\n");
                OUT("  ADDRREG(7)+= 6;\n");

                if (DEBUG_BRANCH)
                    fputs("  printf(\"RTR: ->0x%X\\n\", PC);\n", o);
                if (DEBUG_SR)
                    fputs("  printf(\"SR: %08X %04X\\n\", PC, SR);\n",
                          o);
                pcinc = 0;
                break;

            case i_JSR: // todo check for JSR (A0)  vs PDIS off PC
                generate_ea(o, iib, tp_src, 1);
                OUT("\n");
                if (DEBUG_BRANCH)
                    fputs("  printf(\"JSR: 0x%X\\n\", PC);\n", o);
                OUT("  ADDRREG(7)-= 4;\n");
                fprintf(o, "  storelong(ADDRREG(7), PC+%d);\n", (iib->wordlen) * 2);
                ABORT_CHECK(o);
#ifdef PRESERVE_HIGH_BYTE
                // OUT("  fetchword(srcaddr);"); //20060113
                // ABORT_CHECK(o);
                if (iib->stype == dt_Pidx || iib->stype == dt_Pdis)
                {
                    OUT("  PC = (PC & 0xff000000) | (srcaddr & 0x00ffffff) ;\n");
                }
                else
                    OUT("  PC = srcaddr;\n");
#else
                OUT("  PC = srcaddr;\n");
#endif

                if (DEBUG_BRANCH)
                    fputs("  printf(\"JSR: ->0x%X\\n\", PC);", o);

                pcinc = 0;
                break;

            case i_JMP: // todo check for JMP (A0) vs PDIS off PC
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                OUT("\n");
                //                    if (DEBUG_BRANCH)
                //                        fputs("  printf(\"JMP: 0x%X\\n\", PC);", o);
                // OUT("  fetchword(srcaddr);\n"); //20060113
                // ABORT_CHECK(o);  // suspect there's an issue here **** 20180316 ****
                //         fputs("  fprintf(stderr,\"JMP: %08x->0x%08x\\n\", PC, srcaddr);\n", o);  //2020.07.24
#ifdef PRESERVE_HIGH_BYTE
                if (iib->stype == dt_Pidx || iib->stype == dt_Pdis)
                {
                    OUT("  PC = (PC & 0xff000000) | (srcaddr & 0x00ffffff) ;\n");
                }
                else
                    OUT("  PC = srcaddr;\n");
#else
                {
                    OUT("  PC = srcaddr;\n");
                }
#endif

                if (DEBUG_BRANCH)
                    fputs("  printf(\"JMP: ->0x%X\\n\", PC);\n", o);

                pcinc = 0;
                break;

            case i_Scc:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_cc(o, iib);
                generate_outdata(o, iib, "cc ? (uint8)(-1) : 0");
                OUT("\n");
                generate_eastore(o, iib, tp_src);
                break;

            case i_SF:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_outdata(o, iib, "0");
                OUT("\n");
                generate_eastore(o, iib, tp_src);
                break;

            case i_DBcc:
                GENDBG("");
                /* special case where ipc holds the already PC-relative value */
                fprintf(o, "  uint32 srcdata = ipc->src;\n");
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_cc(o, iib);
                C_ABRT_CHK(o);

                OUT("\n");
                if (iib->size != sz_word)
                {
                    OUT("ERROR size\n");
                }
                OUT("  if (!cc) {\n");
                OUT("    dstdata-= 1;\n");
                OUT("    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)\n");
                OUT("| (dstdata & 0xFFFF);\n");
                OUT("    if ((sint16)dstdata != -1)\n");
                OUT("      PC = srcdata;\n");
                OUT("    else\n");
                fprintf(o, "     {PC+= %d; cpu68k_clocks+=2;}\n", (iib->wordlen) * 2);
                OUT("  } else\n");
                fprintf(o, "    PC+= %d;\n", (iib->wordlen) * 2);
                pcinc = 0;
                break;

            case i_DBRA:
                GENDBG("");
                /* special case where ipc holds the already PC-relative value */
                fprintf(o, "  uint32 srcdata = ipc->src;\n");
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);

                C_ABRT_CHK(o);

                OUT("\n");
                if (iib->size != sz_word)
                {
                    OUT("ERROR size\n");
                }
                OUT("  dstdata-= 1;\n");
                OUT("  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF) | ");
                OUT("(dstdata & 0xFFFF);\n");
                OUT("  if ((sint16)dstdata != -1)\n");
                OUT("    PC = srcdata;\n");
                OUT("  else\n");
                fprintf(o, "   {PC+= %d; cpu68k_clocks+=4;}\n", (iib->wordlen) * 2);
                pcinc = 0;
                break;

            case i_Bcc: // the -8's are reminders that the base case takes 8 cycles -  // RA2005.05.09
                GENDBG("");
                /* special case where ipc holds the already PC-relative value */
                OUT("  uint32 srcdata = ipc->src;\n");
                generate_cc(o, iib);
                OUT("\n");
                OUT("  uint32 oldpc=PC;\n");
#ifdef DEBUG
                OUT("static uint32 sent;\n");
#endif
                if (DEBUG_BRANCH)
                    fputs("  printf(\"Bcc: 0x%X\\n\", PC);\n", o);
                OUT("  if (cc) {\n");

#ifdef PRESERVE_HIGH_BYTE
                OUT("    {  PC = (PC & 0xff000000) | (srcdata & 0x00ffffff); cpu68k_clocks+=(10-8);}\n");
#else
                OUT("    {  PC = srcdata; cpu68k_clocks+=(10-8);}\n");
#endif

                OUT("        if (oldpc==PC) {\n");
#ifdef DEBUG
                fprintf(o, "            if (sent!=PC) {sent=PC; fprintf(stderr,\"infinite loop detected at %%08x opcode length: %d bytes\\n\",PC);}\n", (iib->wordlen) * 2);
#endif
                OUT("            cpu68k_clocks+=10000; }\n"); // move clocks forward to speed up next timer IRQ
                OUT("  } else\n");
                if (iib->size == sz_word)
                    fprintf(o, "    {PC+= %d;                       }\n", (iib->wordlen) * 2);
                else
                    fprintf(o, "    {PC+= %d; cpu68k_clocks+=(12-8);}\n", (iib->wordlen) * 2);
                if (DEBUG_BRANCH)
                    fputs("  printf(\"Bcc: ->0x%X\\n\", PC);\n", o);
                pcinc = 0;
                break;

            case i_BSR:
                GENDBG("");
                /* special case where ipc holds the already PC-relative value */
                OUT("  uint32 srcdata = ipc->src;\n");
                OUT("\n");
                if (DEBUG_BRANCH)
                    fputs("  printf(\"BSR: 0x%X\\n\", PC);\n", o);
                OUT("  ADDRREG(7)-= 4;\n");
                fprintf(o, "  storelong(ADDRREG(7), PC+%d);\n", (iib->wordlen) * 2);
                ABORT_CHECK(o);
#ifdef PRESERVE_HIGH_BYTE
                OUT("  PC = (PC & 0xff000000) | (srcdata & 0x00ffffff);\n");
#else
                OUT("  PC = srcdata;\n");
#endif
                if (DEBUG_BRANCH)
                    fputs("  printf(\"BSR: ->0x%X\\n\", PC);\n", o);
                pcinc = 0;
                break;

            case i_DIVU:
                GENDBG("");
                /* DIVx is the only instruction that has different sizes for the
                   source and destination! */
                if (iib->dtype != dt_Dreg)
                    OUT("ERROR dtype\n");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src); /* 16bit EA */
                generate_ea(o, iib, tp_dst, 1); /* 32bit Dn */
                OUT("  uint32 dstdata = DATAREG(dstreg);\n");
                OUT("  uint32 quotient;\n");

                // OUT("  ALERT_LOG(0,\"i_DIVU @ %08x src:%08x dest:%08x (dstreg:%d) abort_opcode:%d\",PC,srcdata,dstdata,dstreg,abort_opcode);\n" );

                C_ABRT_CHK(o);
                OUT("\n");
                OUT("  if (srcdata == 0) {\n");
                // OUT(            "    ALERT_LOG(0,\"DIVIDE_BY_ZERO @ %08lx src:%08lx dest:%08lx\",(long)PC,(long)srcdata,(long)dstdata);\n" );
                OUT("    ZFLAG=0; NFLAG=0;");
                fprintf(o, "    reg68k_internal_vector(V_ZERO, PC+%d,0);\n",
                        (iib->wordlen) * 2);
                OUT("    return;\n");
                OUT("  }\n");

                OUT("  quotient = dstdata / srcdata;\n");
                OUT("  if ((quotient & 0xffff0000) == 0) {\n");    // what's this if for and why? checking for overflow?
                OUT("    DATAREG(dstreg) = quotient | ");          // Notes:  1. If divide by zero occurs, an exception occurs.
                OUT("    (((uint16)(dstdata % srcdata))<<16);\n"); //         2. If overflow occurs, neither operand is affected.

                if (flags && iib->flags.set & IIB_FLAG_V)
                    OUT("    VFLAG = 0;\n");
                // 68020             if (flags && iib->flags.set & IIB_FLAG_N)           // 2018.03.10 RA
                // 68020                 OUT("    NFLAG = ((sint16)quotient) < 0;\n");

                if (flags && iib->flags.set & IIB_FLAG_Z)
                    OUT("  ZFLAG = !((uint16)quotient);\n");
                if (flags && (iib->flags.set & IIB_FLAG_V ||
                              iib->flags.set & IIB_FLAG_N))
                {
                    OUT("  } else {\n");
                    if (flags && iib->flags.set & IIB_FLAG_V)
                        OUT("    VFLAG = 1;\n");
                    //                        if (flags && iib->flags.set & IIB_FLAG_N)  //2018.03.10
                    //                          OUT("    NFLAG = 1;\n");

                    OUT("  NFLAG = !!((quotient & 0x80000));\n"); // 2020.07.28
                }
                OUT("  }\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("  CFLAG = 0;\n");
                break;

                /* 64 */

            case i_ROXR:
            case i_ROXL:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_bits(o, iib);
                OUT("  uint8 loop = srcdata & 63;\n");
                OUT("  uint8 cflag = CFLAG;\n");
                OUT("  uint8 xflag = XFLAG;\n");
                generate_outdata(o, iib, "dstdata");
                C_ABRT_CHK(o);
                OUT("\n");

                // ROXL bit31->c, x->bit0, bit31->x
                // ROXR x->bit31,bit0->c, bit0->x

                if (iib->mnemonic == i_ROXR)
                {
                    OUT("  while(loop) {\n");
                    OUT("    cflag = outdata & 1;\n");
                    OUT("    outdata>>= 1;\n");
                    OUT("    if (xflag)\n");
                    OUT("      outdata |= 1<<(bits-1);\n");
                    OUT("    xflag = cflag;\n");
                    OUT("    loop--;\n");
                    OUT("  }\n");
                }
                else
                {
                    OUT("  while(loop) {\n");
                    OUT("    cflag = outdata & 1<<(bits-1) ? 1 : 0;\n");
                    OUT("    outdata<<= 1;\n");
                    OUT("    outdata |= xflag;\n");
                    OUT("    xflag = cflag;\n");
                    OUT("    loop--;\n");
                    OUT("  }\n");
                }
                generate_eastore(o, iib, tp_dst);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_X)
                    OUT("  XFLAG = xflag;\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("  CFLAG = xflag;\n");
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                break;

            case i_ROR:
            case i_ROL:
                GENDBG("");
                generate_ea(o, iib, tp_src, 1);
                generate_eaval(o, iib, tp_src);
                generate_ea(o, iib, tp_dst, 1);
                generate_eaval(o, iib, tp_dst);
                generate_bits(o, iib);
                OUT("  uint8 loop = srcdata & 63;\n");
                OUT("  uint8 cflag = 0;\n");
                generate_outdata(o, iib, "dstdata");
                C_ABRT_CHK(o);
                OUT("\n");

                // ROL bit31->c only->bit 0
                // ROR bit 0->c and  bit31

                if (iib->mnemonic == i_ROR)
                {
                    OUT("  while(loop) {\n");
                    OUT("    cflag = outdata & 1;\n");
                    OUT("    outdata>>= 1;\n");
                    OUT("    if (cflag)\n");
                    OUT("      outdata |= 1<<(bits-1);\n");
                    OUT("    loop--;\n");
                    OUT("  }\n");
                }
                else
                {
                    OUT("  while(loop) {\n");
                    OUT("    cflag = outdata & 1<<(bits-1) ? 1 : 0;\n");
                    OUT("    outdata<<= 1;\n");
                    OUT("    if (cflag)\n");
                    OUT("      outdata |= 1;\n");
                    OUT("    loop--;\n");
                    OUT("  }\n");
                }
                generate_eastore(o, iib, tp_dst);
                if (flags)
                    OUT("\n");
                if (flags && iib->flags.set & IIB_FLAG_C)
                    OUT("  CFLAG = cflag;\n");
                if (flags && iib->flags.set & IIB_FLAG_N)
                    generate_stdflag_n(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_Z)
                    generate_stdflag_z(o, iib);
                if (flags && iib->flags.set & IIB_FLAG_V)
                    generate_clrflag_v(o, iib);
                break;

            case i_LINE10:
                GENDBG("");
                OUT("\n");
                // fprintf(o, "  PC+=2;\n  reg68k_internal_vector(V_LINE10, PC,0);\n");        //20060116-RA

                fprintf(o, "#ifdef DEBUG\n");
                fprintf(o, "a_line();\n");
                fprintf(o, "#endif\n");
                fprintf(o, "reg68k_internal_vector(V_LINE10, PC,0);\n");

                pcinc = 0;
                break;

            case i_LINE15:
                GENDBG("");
                OUT("\n");
                fprintf(o, "  reg68k_internal_vector(V_LINE15, PC,0);\n");
                pcinc = 0;
                break;

            case i_ILLG:
                GENDBG("");
                OUT("  if (ipc->opcode==0x4e7a) PC+=4; else PC+=2;\n"); // http://bitsavers.org/pdf/apple/lisa/unisoft/Unisoft_Lisa_Kernel_Aug1983.pdf p3 movec vbr,d0
                OUT("  printf(\"Illegal instruction @ %x\\n\", PC);\n");
                fprintf(o, "  reg68k_internal_vector(4, PC,0);\n"); // RA2002.08.02
                pcinc = 0;
                break;

            } /* switch */

            if (pcinc)
            {
                // added to prevent bus error issues. -- RA 2003.12.04.
                // 20180318// ABORT_CHECK(o);
                //
                fprintf(o, "  PC+= %d;\n", (iib->wordlen) * 2);
            }

            UNFLAG_ACHK(o); // RA
            OUT("}\n\n");
        }
    }
}

void generate_ea(FILE *o, t_iib *iib, t_type type, int update)
{
    t_datatype datatype = type ? iib->dtype : iib->stype;

    /* generate information about EA to be used in calculations */

    GENDBG(":");
    fprintf(o, "//type:%d datatype: %d, iib->dtype:%d, iib->stype:%d\n", type, datatype, iib->dtype, iib->stype);

#ifdef DEBUG
    switch (datatype)
    {
    case dt_Dreg:
        GENDBG("dt_Dreg");
        break;
    case dt_Areg:
        GENDBG("dt_Areg");
        break;
    case dt_Aind:
        GENDBG("dt_Aind");
        break;
    case dt_Ainc:
        GENDBG("dt_Ainc");
        break;
    case dt_Adec:
        GENDBG("dt_Adec");
        break;
    case dt_Adis:
        GENDBG("dt_Adis");
        break;
    case dt_Aidx:
        GENDBG("dt_Aidx");
        break;
    case dt_ImmB:
        GENDBG("dt_ImmB");
        break;
    case dt_ImmW:
        GENDBG("dt_ImmW");
        break;
    case dt_ImmL:
        GENDBG("dt_ImmL");
        break;
    case dt_ImmS:
        GENDBG("dt_ImmS");
        break;
    case dt_Imm3:
        GENDBG("dt_Imm3");
        break;
    case dt_Imm4:
        GENDBG("dt_Imm4");
        break;
    case dt_Imm8:
        GENDBG("dt_Imm8");
        break;
    case dt_Imm8s:
        GENDBG("dt_Imm8s");
        break;

    default:
        GENDBG("other");
        break;
    }
#endif

    switch (datatype)
    {
    case dt_Dreg:
    case dt_Areg:
    case dt_Aind:
    case dt_Ainc:
    case dt_Adec:
    case dt_Adis:
    case dt_Aidx:
        if (type == tp_src)
            fprintf(o, "  int srcreg = (ipc->opcode >> %d) & 7;\n", iib->sbitpos);
        else
            fprintf(o, "  int dstreg = (ipc->opcode >> %d) & 7;\n", iib->dbitpos);
        break;
    default:
        break;
    }

    if (datatype == dt_Ainc && update)
    {

        /* Ainc and update */

        switch (iib->size)
        {
        case sz_byte:
            if (type == tp_src)
            {
                fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg);\n");
                fprintf(o, "  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= "
                           "(srcreg == 7 ? 2 : 1), 0);\n");
            }
            else
            {
                fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg);\n");
                fprintf(o, "  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= "
                           "(dstreg == 7 ? 2 : 1), 0);\n");
            }
            break;
        case sz_word:
            if (type == tp_src)
                fprintf(o, "  uint32 srcaddr = (ADDRREG(srcreg)+=2, "
                           "ADDRREG(srcreg)-2);\n");
            else
                fprintf(o, "  uint32 dstaddr = (ADDRREG(dstreg)+=2, "
                           "ADDRREG(dstreg)-2);\n");
            break;
        case sz_long:
            if (type == tp_src)
                fprintf(o, "  uint32 srcaddr = (ADDRREG(srcreg)+=4, "
                           "ADDRREG(srcreg)-4);\n");
            else
                fprintf(o, "  uint32 dstaddr = (ADDRREG(dstreg)+=4, "
                           "ADDRREG(dstreg)-4);\n");
            break;
        default:
            fprintf(o, "ERROR size\n");
            break;
        }
    }
    else if (datatype == dt_Adec && update)
    {

        /* Adec and update */

        switch (iib->size)
        {
        case sz_byte:
            if (type == tp_src)
            {
                fprintf(o, "  uint32 srcaddr = (ADDRREG(srcreg)-= "
                           "(srcreg == 7 ? 2 : 1));\n");
            }
            else
            {
                fprintf(o, "  uint32 dstaddr = (ADDRREG(dstreg)-= "
                           "(dstreg == 7 ? 2 : 1));\n");
            }
            break;
        case sz_word:
            if (type == tp_src)
                fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg)-=2;\n");
            else
                fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg)-=2;\n");
            break;
        case sz_long:
            if (type == tp_src)
                fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg)-=4;\n");
            else
                fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg)-=4;\n");
            break;
        default:
            fprintf(o, "ERROR size\n");
            break;
        }
    }
    else
    {

        /* no update required */

        switch (datatype)
        {
        case dt_Dreg:
        case dt_Areg:
            break;
        case dt_Aind:
        case dt_Adec:
        case dt_Ainc:
            if (type == tp_src)
                fprintf(o, "  uint32 srcaddr = ADDRREG(srcreg);\n");
            else
                fprintf(o, "  uint32 dstaddr = ADDRREG(dstreg);\n");
            break;
        case dt_Adis:
            if (type == tp_src)
                fprintf(o, "  uint32 srcaddr = (sint32)ADDRREG(srcreg) + "
                           "(sint32)(sint16)ipc->src;\n");
            else
                fprintf(o, "  uint32 dstaddr = (sint32)ADDRREG(dstreg) + "
                           "(sint32)(sint16)ipc->dst;\n");
            break;
        case dt_Aidx:
            if (type == tp_src)
            {
                fprintf(o, "  uint32 srcaddr = (sint32)ADDRREG(srcreg) + "
                           "idxval_src(ipc);\n");
            }
            else
            {
                fprintf(o, "  uint32 dstaddr = (sint32)ADDRREG(dstreg) + "
                           "idxval_dst(ipc);\n");
            }
            break;
        case dt_AbsW:
        case dt_AbsL:
        case dt_Pdis:
            if (type == tp_src)
                fprintf(o, "  uint32 srcaddr = ipc->src;\n");
            else
                fprintf(o, "  uint32 dstaddr = ipc->dst;\n");
            break;
        case dt_Pidx:
            if (type == tp_src)
            {
                fprintf(o, "  uint32 srcaddr = idxval_src(ipc);\n");
            }
            else
            {
                fprintf(o, "  uint32 dstaddr = idxval_dst(ipc);\n");
            }
            break;
        case dt_ImmB:
        case dt_ImmW:
        case dt_ImmL:
        case dt_ImmS:
        case dt_Imm3:
        case dt_Imm4:
        case dt_Imm8:
        case dt_Imm8s:
            /* no address - it is immediate */
            break;
        default:
            fprintf(o, "ERROR\n");
            break;
        }
    }
}

void generate_eaval(FILE *o, t_iib *iib, t_type type)
{
    t_datatype datatype = type ? iib->dtype : iib->stype;

    /* get value in EA */
    GENDBG(":");
    fprintf(o, "//type:%d datatype: %d, iib->dtype:%d, iib->stype:%d\n", type, datatype, iib->dtype, iib->stype);

    switch (datatype)
    {
    case dt_Dreg:
        switch (iib->size)
        {
        case sz_byte:
            GENDBG("byte");
            if (type == tp_src)
                fprintf(o, "  uint8 srcdata = DATAREG(srcreg);\n");
            else
                fprintf(o, "  uint8 dstdata = DATAREG(dstreg);\n");
            break;
        case sz_word:
            GENDBG("word");
            if (type == tp_src)
                fprintf(o, "  uint16 srcdata = DATAREG(srcreg);\n");
            else
                fprintf(o, "  uint16 dstdata = DATAREG(dstreg);\n");
            break;
        case sz_long:
            GENDBG("long");
            if (type == tp_src)
                fprintf(o, "  uint32 srcdata = DATAREG(srcreg);\n");
            else
                fprintf(o, "  uint32 dstdata = DATAREG(dstreg);\n");
            break;
        default:
            GENDBG("");
            fprintf(o, "ERROR size\n");
            break;
        }
        break;
    case dt_Areg:
        switch (iib->size)
        {
        case sz_byte:
            GENDBG("");
            if (type == tp_src)
                fprintf(o, "  uint8 srcdata = ADDRREG(srcreg);\n");
            else
                fprintf(o, "  uint8 dstdata = ADDRREG(dstreg);\n");
            break;
        case sz_word:
            GENDBG("");
            if (type == tp_src)
                fprintf(o, "  uint16 srcdata = ADDRREG(srcreg);\n");
            else
                fprintf(o, "  uint16 dstdata = ADDRREG(dstreg);\n");
            break;
        case sz_long:
            GENDBG("");
            if (type == tp_src)
                fprintf(o, "  uint32 srcdata = ADDRREG(srcreg);\n");
            else
                fprintf(o, "  uint32 dstdata = ADDRREG(dstreg);\n");
            break;
        default:
            GENDBG("");
            fprintf(o, "ERROR size\n");
            break;
        }
        break;
    case dt_Aind:
    case dt_Adec:
    case dt_Ainc:
    case dt_Adis:
    case dt_Aidx:
    case dt_AbsW:
    case dt_AbsL:
    case dt_Pdis:
    case dt_Pidx:
        switch (iib->size)
        {
        case sz_byte:
            GENDBG("byte");
            if (type == tp_src)

                fprintf(o, "  uint8 srcdata = fetchbyte(srcaddr);\n");
            else
                fprintf(o, "  uint8 dstdata = fetchbyte(dstaddr);\n");
            FLAG_ACHK(o);
            break;
        case sz_word:
            GENDBG("word");
            if (type == tp_src)
                fprintf(o, "  uint16 srcdata = fetchword(srcaddr);\n");
            else
                fprintf(o, "  uint16 dstdata = fetchword(dstaddr);\n");
            FLAG_ACHK(o);
            break;
        case sz_long:
            GENDBG("long");
            if (type == tp_src)
                fprintf(o, "  uint32 srcdata = fetchlong(srcaddr);\n");
            else
                fprintf(o, "  uint32 dstdata = fetchlong(dstaddr);\n");
            FLAG_ACHK(o);
            break;
        default:
            GENDBG("");
            fprintf(o, "ERROR size\n");
            break;
        }
        break;
    case dt_ImmB:
        GENDBG("immB");
        if (type == tp_src)
            fprintf(o, "  uint8 srcdata = ipc->src;\n");
        else
            fprintf(o, "  uint8 dstdata = ipc->dst;\n");
        break;
    case dt_ImmW:
        GENDBG("immW");
        if (type == tp_src)
            fprintf(o, "  uint16 srcdata = ipc->src;\n");
        else
            fprintf(o, "  uint16 dstdata = ipc->dst;\n");
        break;
    case dt_ImmL:
        GENDBG("immL");
        if (type == tp_src)
            fprintf(o, "  uint32 srcdata = ipc->src;\n");
        else
            fprintf(o, "  uint32 dstdata = ipc->dst;\n");
        break;
    case dt_ImmS:
        GENDBG("immS");
        if (type == tp_src)
            fprintf(o, "  unsigned int srcdata = %d;\n", iib->immvalue);
        else
            fprintf(o, "  unsigned int dstdata = %d;\n", iib->immvalue);
        break;
    case dt_Imm3:
    case dt_Imm4:
    case dt_Imm8:
        GENDBG("imm3,4,8");
        if (type == tp_src)
            fprintf(o, "  unsigned int srcdata = ipc->src;\n");
        else
            fprintf(o, "  unsigned int dstdata = ipc->dst;\n");
        break;
    case dt_Imm8s:
        GENDBG("imm8S");
        if (type == tp_src)
            fprintf(o, "  signed int srcdata = ipc->src;\n");
        else
            fprintf(o, "  signed int dstdata = ipc->dst;\n");
        break;
    default:
        fprintf(o, "ERROR\n");
    }
}

void generate_eastore(FILE *o, t_iib *iib, t_type type)
{

    fputs("ABORT_OPCODE_CHK();\n", o);

    /* get value in EA */
    GENDBG(":");
    fprintf(o, "//type:%d  iib->dtype:%d, iib->stype:%d\n", type, iib->dtype, iib->stype);

    switch (type == tp_dst ? iib->dtype : iib->stype)
    {
    case dt_Dreg:
        switch (iib->size)
        {
        case sz_byte:
            GENDBG("D-reg byte");
            if (type == tp_src)
                fprintf(o, "  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | "
                           "outdata;\n");
            else
                fprintf(o, "  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | "
                           "outdata;\n");
            break;
        case sz_word:
            GENDBG("D-reg word");
            if (type == tp_src)
                fprintf(o, "  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xffff) | "
                           "outdata;\n");
            else
                fprintf(o, "  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | "
                           "outdata;\n");
            break;
        case sz_long:
            GENDBG("D-reg long");
            if (type == tp_src)
                fprintf(o, "  DATAREG(srcreg) = outdata;\n");
            else
                fprintf(o, "  DATAREG(dstreg) = outdata;\n");
            break;
        default:
            fprintf(o, "ERROR size\n");
            break;
        }
        break;
    case dt_Areg:
        switch (iib->size)
        {
        case sz_byte:
            GENDBG("A-reg-byte");
            if (type == tp_src)
                fprintf(o, "  ADDRREG(srcreg) = (ADDRREG(srcreg) & ~0xff) | "
                           "outdata;\n");
            else
                fprintf(o, "  ADDRREG(dstreg) = (ADDRREG(dstreg) & ~0xff) | "
                           "outdata;\n");
            break;
        case sz_word:
            GENDBG("A-reg-word");
            if (type == tp_src)
                fprintf(o, "  ADDRREG(srcreg) = (ADDRREG(srcreg) & ~0xffff) | "
                           "outdata;\n");
            else
                fprintf(o, "  ADDRREG(dstreg) = (ADDRREG(dstreg) & ~0xffff) | "
                           "outdata;\n");
            break;
        case sz_long:
            GENDBG("A-reg-long");
            if (type == tp_src)
                fprintf(o, "  ADDRREG(srcreg) = outdata;\n");
            else
                fprintf(o, "  ADDRREG(dstreg) = outdata;\n");
            break;
        default:
            fprintf(o, "ERROR size\n");
            break;
        }
        break;
    case dt_Aind:
    case dt_Ainc:
    case dt_Adec:
    case dt_Adis:
    case dt_Aidx:
    case dt_AbsW:
    case dt_AbsL:
    case dt_Pdis:
    case dt_Pidx:
        switch (iib->size)
        {
        case sz_byte:
            GENDBG("byte");
            if (type == tp_src)
                fprintf(o, "  storebyte(srcaddr, outdata);\n");
            else
                fprintf(o, "  storebyte(dstaddr, outdata);\n");

            break;
        case sz_word:
            GENDBG("word");
            if (type == tp_src)
                fprintf(o, "  storeword(srcaddr, outdata);\n");
            else
                fprintf(o, "  storeword(dstaddr, outdata);\n");
            break;
        case sz_long:
            GENDBG("long");
            if (type == tp_src)
                fprintf(o, "  storelong(srcaddr, outdata);\n");
            else
                fprintf(o, "  storelong(dstaddr, outdata);\n");
            break;
        default:
            GENDBG("");
            fprintf(o, "ERROR size\n");
        }
        ABORT_CHECK(o); // all of the above call store, so check for MMU error
        break;
    default:
        fprintf(o, "ERROR type\n");
    }
}

void generate_outdata(FILE *o, t_iib *iib, const char *init)
{
    GENDBG(":");
    fprintf(o, "//iib->dtype:%d, iib->stype:%d init:%s\n", iib->dtype, iib->stype, init);

    switch (iib->size)
    {
    case sz_byte:
        GENDBG("byte");
        fprintf(o, "  uint8 ");
        break;
    case sz_word:
        GENDBG("word");
        fprintf(o, "  uint16 ");
        break;
    case sz_long:
        GENDBG("long");
        fprintf(o, "  uint32 ");
        break;
    default:
        GENDBG("");
        fprintf(o, "ERROR size\n");
        break;
    }
    fprintf(o, "outdata%s%s;\n", (init && init[0]) ? " = " : "",
            init ? init : "");
}

void generate_cc(FILE *o, t_iib *iib)
{
    switch (iib->cc)
    {
    case 0: /*  T */
        fprintf(o, "  uint8 cc = 1;\n");
        break;
    case 1: /*  F */
        fprintf(o, "  uint8 cc = 0;\n");
        break;
    case 2: /* HI */
        fprintf(o, "  uint8 cc = !(CFLAG || ZFLAG);\n");
        break;
    case 3: /* LS */
        fprintf(o, "  uint8 cc = CFLAG || ZFLAG;\n");
        break;
    case 4: /* CC */
        fprintf(o, "  uint8 cc = !CFLAG;\n");
        break;
    case 5: /* CS */
        fprintf(o, "  uint8 cc = CFLAG;\n");
        break;
    case 6: /* NE */
        fprintf(o, "  uint8 cc = !ZFLAG;\n");
        break;
    case 7: /* EQ */
        fprintf(o, "  uint8 cc = ZFLAG;\n");
        break;
    case 8: /* VC */
        fprintf(o, "  uint8 cc = !VFLAG;\n");
        break;
    case 9: /* VS */
        fprintf(o, "  uint8 cc = VFLAG;\n");
        break;
    case 10: /* PL */
        fprintf(o, "  uint8 cc = !NFLAG;\n");
        break;
    case 11: /* MI */
        fprintf(o, "  uint8 cc = NFLAG;\n");
        break;
    case 12: /* GE */
        fprintf(o, "  uint8 cc = (NFLAG == VFLAG);\n");
        break;
    case 13: /* LT */
        fprintf(o, "  uint8 cc = (NFLAG != VFLAG);\n");
        break;
    case 14: /* GT */
        fprintf(o, "  uint8 cc = !ZFLAG && (NFLAG == VFLAG);\n");
        break;
    case 15: /* LE */
        fprintf(o, "  uint8 cc = ZFLAG || (NFLAG != VFLAG);\n");
        break;
    default:
        fprintf(o, "ERROR cc\n");
        break;
    }
}

void generate_stdflag_n(FILE *o, t_iib *iib)
{
    switch (iib->size)
    {
    case sz_byte:
        GENDBG("");
        fprintf(o, "  NFLAG = ((sint8)outdata) < 0;\n");
        break;
    case sz_word:
        GENDBG("");
        fprintf(o, "  NFLAG = ((sint16)outdata) < 0;\n");
        break;
    case sz_long:
        GENDBG("");
        fprintf(o, "  NFLAG = ((sint32)outdata) < 0;\n");
        break;
    default:
        GENDBG("");
        fprintf(o, "ERROR size\n");
        break;
    }
}

void generate_stdflag_z(FILE *o, t_iib *iib)
{
    fprintf(o, "  ZFLAG = !outdata;\n");
}

void generate_clrflag_v(FILE *o, t_iib *iib)
{
    fprintf(o, "  VFLAG = 0;\n");
}

void generate_clrflag_c(FILE *o, t_iib *iib)
{
    fprintf(o, "  CFLAG = 0;\n");
}

void generate_clrflag_n(FILE *o, t_iib *iib)
{
    fprintf(o, "  NFLAG = 0;\n");
}

void generate_setflag_z(FILE *o, t_iib *iib)
{
    fprintf(o, "  ZFLAG = 1;\n");
}

void generate_subflag_c(FILE *o, t_iib *iib)
{
    /* C = (Sm && !Dm) || (Rm && !Dm) || (Sm && Rm)
       carry is performed as if both source and destination are unsigned,
       so this is simply if the source is greater than the destination - I
       have proven this to be the case using a truth table and the above
       motorola definition */
    fprintf(o, "  CFLAG = srcdata > dstdata;\n");
}

void generate_subflag_cx(FILE *o, t_iib *iib)
{
    /* C = (Sm && !Dm) || (Rm && !Dm) || (Sm && Rm)
       carry is performed as if both source and destination are unsigned,
       so this is simply if the source is greater than the destination - I
       have proven this to be the case using a truth table and the above
       motorola definition */
    fprintf(o, "  XFLAG = CFLAG = srcdata > dstdata;\n");
}

void generate_subxflag_cx(FILE *o, t_iib *iib)
{
    /* V = (Sm && !Dm) || (Rm && (!Dm || Sm)) */
    fprintf(o, "  {\n");
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "    int Sm = (sint8)srcdata < 0;\n");
        fprintf(o, "    int Dm = (sint8)dstdata < 0;\n");
        fprintf(o, "    int Rm = (sint8)outdata < 0;\n");
        break;
    case sz_word:
        fprintf(o, "    int Sm = (sint16)srcdata < 0;\n");
        fprintf(o, "    int Dm = (sint16)dstdata < 0;\n");
        fprintf(o, "    int Rm = (sint16)outdata < 0;\n");
        break;
    case sz_long:
        fprintf(o, "    int Sm = (sint32)srcdata < 0;\n");
        fprintf(o, "    int Dm = (sint32)dstdata < 0;\n");
        fprintf(o, "    int Rm = (sint32)outdata < 0;\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
    fprintf(o, "    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));\n");
    fprintf(o, "  }\n");
}

void generate_cmpaflag_c(FILE *o, t_iib *iib)
{
    /* see generate_subflag_c - this is just the same but with a sign extend
       on the source */
    fprintf(o, "  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;\n");
}

void generate_subflag_v(FILE *o, t_iib *iib)
{
    /* V = (!Sm && Dm && !Rm) || (Sm && !Dm && Rm)
       overflow is performed as if both source and destination are signed,
       the only two condtions are if we've added too much to a +ve number
       or we've subtracted too much from a -ve number - I have proven the
       this to be the case using a truth table and the above motorola
       definition */
    /* the technique to implement the above formula is to make sure the sign
       of Sm != the sign of Dm, and the sign of Dm != the sign of Rm. */
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) ");
        fprintf(o, "&&\n    (((sint8)dstdata < 0) != ((sint8)outdata < 0));\n");
        break;
    case sz_word:
        fprintf(o, "  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) ");
        fprintf(o, "&&\n    (((sint16)dstdata < 0) != ((sint16)outdata < 0));\n");
        break;
    case sz_long:
        fprintf(o, "  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) ");
        fprintf(o, "&&\n    (((sint32)dstdata < 0) != ((sint32)outdata < 0));\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
}

void generate_cmpaflag_v(FILE *o, t_iib *iib)
{
    /* see generate_subflag_v - this is just the sz_long version with a sign
       extend on the source */
    fprintf(o, "  VFLAG = (((sint32)(sint16)srcdata < 0) != ");
    fprintf(o, "((sint32)dstdata < 0)) ");
    fprintf(o, "&&\n    (((sint32)dstdata < 0) != ((sint32)outdata < 0));\n");
}

void generate_stdxflag_z(FILE *o, t_iib *iib)
{
    fprintf(o, "  if (outdata) ZFLAG = 0;\n");
}

void generate_addflag_cx(FILE *o, t_iib *iib)
{
    /* C = (Sm && Dm) || (!Rm && Dm) || (Sm && !Rm)
       carry is performed as if both source and destination are unsigned,
       so this is simply if the source is bigger than how much can be added
       to the destination without wrapping - this is of course just the bit
       inverse of the destination. */
    /* 0xFFFF - dstdata is obviously just ~dstdata, but try and get the
       compiler to do that without producing a warning, go on... */
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);\n");
        break;
    case sz_word:
        fprintf(o, "  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);\n");
        break;
    case sz_long:
        fprintf(o, "  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
    /* gcc doesn't like a one's compliment of an unsigned - take out the
       casts in the above and it will warn you incorrectly */
}

void generate_addxflag_cx(FILE *o, t_iib *iib)
{
    /* V = (Sm && Dm) || (!Rm && (Dm || Sm)) */
    fprintf(o, "  {\n");
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "    int Sm = (sint8)srcdata < 0;\n");
        fprintf(o, "    int Dm = (sint8)dstdata < 0;\n");
        fprintf(o, "    int Rm = (sint8)outdata < 0;\n");
        break;
    case sz_word:
        fprintf(o, "    int Sm = (sint16)srcdata < 0;\n");
        fprintf(o, "    int Dm = (sint16)dstdata < 0;\n");
        fprintf(o, "    int Rm = (sint16)outdata < 0;\n");
        break;
    case sz_long:
        fprintf(o, "    int Sm = (sint32)srcdata < 0;\n");
        fprintf(o, "    int Dm = (sint32)dstdata < 0;\n");
        fprintf(o, "    int Rm = (sint32)outdata < 0;\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
    fprintf(o, "    XFLAG = CFLAG = (Sm && Dm) || (!Rm && (Dm || Sm));\n");
    fprintf(o, "  }\n");
}

void generate_addflag_v(FILE *o, t_iib *iib)
{
    /* V = (Sm && Dm && !Rm) || (!Sm && !Dm && Rm)
       overflow is performed as if both source and destination are signed,
       the only two condtions are if we've added too much to a +ve number
       or we've subtracted too much from a -ve number */
    /* the technique to implement the above formula is to make sure the sign
       of Sm == the sign of Dm, and the sign of Dm != the sign of Rm. */
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) ");
        fprintf(o, "&&\n    (((sint8)dstdata < 0) != ((sint8)outdata < 0));\n");
        break;
    case sz_word:
        fprintf(o, "  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) ");
        fprintf(o, "&&\n    (((sint16)dstdata < 0) != ((sint16)outdata < 0));\n");
        break;
    case sz_long:
        fprintf(o, "  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) ");
        fprintf(o, "&&\n    (((sint32)dstdata < 0) != ((sint32)outdata < 0));\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
}

void generate_negflag_cx(FILE *o, t_iib *iib)
{
    /* C = (Dm || Rm) */
    fprintf(o, "  XFLAG = CFLAG = srcdata ? 1 : 0;\n");
}

void generate_negflag_v(FILE *o, t_iib *iib)
{
    /* V = (Dm && Rm)
       which is the same as V = src == 1<<15 as the only case where both the
       data and the result are both negative is in the case of the extreme
       negative number (1<<15 in the case of 16 bit) since 0 - 1<<15 = 1<<15. */
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "  VFLAG = (srcdata == (1u<<7));\n");
        break;
    case sz_word:
        fprintf(o, "  VFLAG = (srcdata == (1u<<15));\n");
        break;
    case sz_long:
        fprintf(o, "  VFLAG = (srcdata == (1u<<31));\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
}

void generate_negxflag_cx(FILE *o, t_iib *iib)
{
    /* C = (Dm || Rm)
       unlike NEG, NEGX is done properly */
    fprintf(o, "  XFLAG = CFLAG ");
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "= ((sint8)srcdata < 0) || ((sint8)outdata < 0);\n");
        break;
    case sz_word:
        fprintf(o, "= ((sint16)srcdata < 0) || ((sint16)outdata < 0);\n");
        break;
    case sz_long:
        fprintf(o, "= ((sint32)srcdata < 0) || ((sint32)outdata < 0);\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
}

void generate_negxflag_v(FILE *o, t_iib *iib)
{
    /* V = (Dm && Rm)
       unlike NEG, NEGX is done properly */
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "  VFLAG = ((sint8)srcdata < 0) && ((sint8)outdata < 0);\n");
        break;
    case sz_word:
        fprintf(o, "  VFLAG = ((sint16)srcdata < 0) && ((sint16)outdata < 0);\n");
        break;
    case sz_long:
        fprintf(o, "  VFLAG = ((sint32)srcdata < 0) && ((sint32)outdata < 0);\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
}

void generate_bits(FILE *o, t_iib *iib)
{
    switch (iib->size)
    {
    case sz_byte:
        fprintf(o, "  uint8 bits = 8;\n");
        break;
    case sz_word:
        fprintf(o, "  uint8 bits = 16;\n");
        break;
    case sz_long:
        fprintf(o, "  uint8 bits = 32;\n");
        break;
    default:
        fprintf(o, "ERROR size\n");
        break;
    }
}
