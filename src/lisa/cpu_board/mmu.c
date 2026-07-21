/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.08.02                   *
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
*         DANGER!  This code may be harmful to your sanity! Avoid staring at it        *
*           for extended periods of time without first taking a full dose of           *
*           sarcasm! Irony supplements and vitamin humor are highly advised!           *
*                                                                                      *
*          sodium chloride with traces of iodine should be applied to all comments.    *
*                                                                                      *
\**************************************************************************************/

#define IN_MMU_C
#include <generator.h>
#include <cpu68k.h>
#include <vars.h>

void init_start_mode(void);
int check_mmu0_chk(void);
static uint32 mmu0_checksum = 0;

#ifdef DEBUG
void validate_mmu_segments(char *from);

#define CONTEXTSELECTOR()                                                                                                           \
    {                                                                                                                               \
        context = CXSASEL;                                                                                                          \
        mmudirty = mmudirty_all[CXASEL];                                                                                            \
        mmu_trans = mmu_trans_all[context];                                                                                         \
        mmu = mmu_all[context];                                                                                                     \
        if (debug_log_enabled)                                                                                                      \
            DEBUG_LOG(10, "--->switched to context:%d s1/s2/start:%d/%d/%d<----\n", context, segment1, segment2, start);            \
        if ((segment1 > 1) || (segment2 == 1 || segment2 > 2))                                                                      \
        {                                                                                                                           \
            EXIT(366, 0, "%s:%s%d:Error! seg1=%d, seg2=%d, start=%d", __FILE__, __FUNCTION__, __LINE__, segment1, segment2, start); \
        }                                                                                                                           \
    }
#else

#define CONTEXTSELECTOR()                   \
    {                                       \
        context = CXSASEL;                  \
        mmudirty = mmudirty_all[context];   \
        mmu_trans = mmu_trans_all[context]; \
        mmu = mmu_all[context];             \
    }

#endif

void mmuflush(uint16 opts);
void init_mmu_start_mode(uint16 i);

// Fn to use the above lookup table. :)  The param is the segment origin (address>>9)
lisa_mem_t get_lisa_sio_fn(uint32 sor) { return sio_map[sor & 0xff]; }

// Fn to use the above lookup table. :)  Parameter is the effective address trimmed.
lisa_mem_t get_io_fn(uint32 eaddress) { return io_map[(eaddress >> 9) & 0x7f]; }

// temp vars to play with, don't include these in vars.c
// mmu_t m;         // temp variable to play with
// mmu_trans_t mt;  // mmu translation - temporary var

int is_valid_slr(uint16 q)
{
    q = q & SLR_MASK;

    return ((q == SLR_RO_STK) || (q == SLR_RO_MEM) || (q == SLR_RW_STK) || (q == SLR_RW_MEM) ||
            (q == SLR_IO_SPACE) || (q == SLR_UNUSED_PAGE) || (q == SLR_SIO_SPACE));
}

// This is a very slow time consumer, but it makes sure that our MMU translation table has sane values.
// disabled for now.
#ifdef DEBUG
char *printslr(char *x, long size, uint16 slr);
void check_mmu_segment(uint8 segment, int32 ea, lisa_mem_t rfn, lisa_mem_t wfn, int pagestart, int pageend, char *from);
void checkcontext(uint8 c, char *text)
{
    UNUSED(c);
    UNUSED(text);
} // disabled

void xxxcheckcontext(uint8 c, char *text)
{
    int i, j, k, l;

    //------------------------------------------

    DEBUG_LOG(0, "checking for %s", text);
    k = 1; // supress initial good messages.
    for (i = 0; i < 32767; i++)
    {
        j = (mmu_trans_all[c][i].writefn > 0) && (mmu_trans_all[c][i].readfn > 0);
        l = (mmu_trans_all[c][i].writefn < MAX_LISA_MFN) && (mmu_trans_all[c][i].readfn < MAX_LISA_MFN);
        j = j && l;

        if (j != k)
        {
            k = j;
            DEBUG_LOG(10, "%s mmu fn trans_all %s context %d: i=%i addr:%08x rfn=%d wfn=%d addr=%08x\n",
                      j ? "Good" : "Bad!",
                      text, context, i, (i << 9), mmu_trans_all[c][i].readfn, mmu_trans_all[c][i].writefn,
                      mmu_trans_all[c][i].address);
            if (!j)
                EXIT(337, 0, "mmu context failure");
        }
    }
}
#else
void checkcontext(uint8 c, char *text)
{
    UNUSED(c);
    UNUSED(text);
}
#endif

void disable_vidram(void)
{
    if (!lisaram)
        return;
    mem68k_memptr[vidram] = lisa_mptr_ram;
    mem68k_fetch_byte[vidram] = lisa_rb_ram;
    mem68k_fetch_word[vidram] = lisa_rw_ram;
    mem68k_fetch_long[vidram] = lisa_rl_ram;
    mem68k_store_byte[vidram] = lisa_wb_ram;
    mem68k_store_word[vidram] = lisa_ww_ram;
    mem68k_store_long[vidram] = lisa_wl_ram;

    // videoramdirty=0;                    // no need to refresh the screen, save some cycles
}

void enable_vidram(void)
{
    if (!lisaram)
        return;
    if (has_lisa_xl_screenmod)
    {
        mem68k_memptr[vidram] = lisa_mptr_vidram;
        mem68k_fetch_byte[vidram] = lisa_rb_vidram;
        mem68k_fetch_word[vidram] = lisa_rw_vidram;
        mem68k_fetch_long[vidram] = lisa_rl_vidram;
        mem68k_store_byte[vidram] = lisa_wb_xlvidram;
        mem68k_store_word[vidram] = lisa_ww_xlvidram;
        mem68k_store_long[vidram] = lisa_wl_xlvidram;
    }
    else
    {
        mem68k_memptr[vidram] = lisa_mptr_vidram;
        mem68k_fetch_byte[vidram] = lisa_rb_vidram;
        mem68k_fetch_word[vidram] = lisa_rw_vidram;
        mem68k_fetch_long[vidram] = lisa_rl_vidram;
        mem68k_store_byte[vidram] = lisa_wb_vidram;
        mem68k_store_word[vidram] = lisa_ww_vidram;
        mem68k_store_long[vidram] = lisa_wl_vidram;
    }
    videoramdirty |= 9; // ok now it's time to do so.
}

void lisa_diag2_off_mem(void)
{
    DEBUG_LOG(0, "mmmmmmm ** DISABLING PARITY MEMORY DIAGNOSTIC FUNCTIONS ** mmmmmmmmmm");
    // Turn off parity memory diagnostics
    mem68k_memptr[ram] = lisa_mptr_ram;

    mem68k_fetch_byte[ram] = lisa_rb_ram;
    mem68k_fetch_word[ram] = lisa_rw_ram;
    mem68k_fetch_long[ram] = lisa_rl_ram;
    mem68k_memptr[vidram] = lisa_mptr_vidram;
    mem68k_fetch_byte[vidram] = lisa_rb_vidram;
    mem68k_fetch_word[vidram] = lisa_rw_vidram;
    mem68k_fetch_long[vidram] = lisa_rl_vidram;
    mem68k_store_byte[ram] = lisa_wb_ram;
    mem68k_store_word[ram] = lisa_ww_ram;
    mem68k_store_long[ram] = lisa_wl_ram;

    if (has_lisa_xl_screenmod)
    {
        mem68k_store_byte[vidram] = lisa_wb_xlvidram;
        mem68k_store_word[vidram] = lisa_ww_xlvidram;
        mem68k_store_long[vidram] = lisa_wl_xlvidram;
    }
    else
    {
        mem68k_store_byte[vidram] = lisa_wb_vidram;
        mem68k_store_word[vidram] = lisa_ww_vidram;
        mem68k_store_long[vidram] = lisa_wl_vidram;
    }
}

void lisa_diag2_on_mem(void)
{
    DEBUG_LOG(0, "mmmmmmm ** ENABLING PARITY MEMORY DIAGNOSTIC FUNCTIONS ** mmmmmmmmmm");
    // Turn on parity memory diagnostics
    mem68k_fetch_byte[ram] = lisa_rb_ram_parity;
    mem68k_fetch_word[ram] = lisa_rw_ram_parity;
    mem68k_fetch_long[ram] = lisa_rl_ram_parity;

    mem68k_store_byte[ram] = lisa_wb_ram_parity;
    mem68k_store_word[ram] = lisa_ww_ram_parity;
    mem68k_store_long[ram] = lisa_wl_ram_parity;

    mem68k_fetch_byte[vidram] = lisa_rb_vidram_parity;
    mem68k_fetch_word[vidram] = lisa_rw_vidram_parity;
    mem68k_fetch_long[vidram] = lisa_rl_vidram_parity;

    mem68k_store_byte[vidram] = lisa_wb_vidram_parity;
    mem68k_store_word[vidram] = lisa_ww_vidram_parity;
    mem68k_store_long[vidram] = lisa_wl_vidram_parity;

    mem68k_memptr[ram] = lisa_mptr_ram; // these don't need to change.
    mem68k_memptr[vidram] = lisa_mptr_vidram;
}

uint32 get_mmu0_chk(void)
{
    uint32 mmu0_checksum = 0;
    uint32 i = 0;

    for (i = 0; i < 32768; i++)
    {
        mmu0_checksum += mmu_trans_all[0][i].address;
        mmu0_checksum ^= ((mmu_trans_all[0][i].readfn << 16) ^ (mmu_trans_all[0][i].writefn << 3) ^ (i << 1));
    }
    return mmu0_checksum;
}

int check_mmu0_chk(void)
{
    return 0; // not necessary
    if (mmu0_checksum != get_mmu0_chk())
    {
        EXITR(10, 0, "*BOINK!* mmu_trans_t[0] altered!\n");
    }
    return 0;
}

void init_start_mode_segment(long i)
{
    lisa_mem_t rfn;
    lisa_mem_t wfn;
    int32 adr = 0;

    rfn = get_lisa_sio_fn(i);
    wfn = get_lisa_sio_fn(i);

    if ((i & (32)) == 32) // 001 bit 14 is 1, means MMU is enabled, when in start mode.
    {
#ifdef DEBUG
        if (pc24) // don't warn when initializing mmu before executing POST.
        {
            if (rfn != sio_mmu)
                DEBUG_LOG(10, "*** sio_mmu seg #%d for i&32, rfn is needed:%d\n", i, rfn);
            if (wfn != sio_mmu)
                DEBUG_LOG(10, "*** sio_mmu seg #%d for i&32, wfn is needed:%d\n", i, rfn);
        }
#endif
        rfn = sio_mmu;
        wfn = sio_mmu;
    }
    else if ((i & (64 | 32)) == 64) // 010 bit 15 is 1, means MMU REG being accessed
    {
#ifdef DEBUG
        if (pc24) // don't warn when initializing mmu before executing POST.
        {
            if (rfn != sio_mrg)
                DEBUG_LOG(10, "*** sio_mmu seg #%d for i&64, rfn is needed:%d\n", i, rfn);
            if (wfn != sio_mrg)
                DEBUG_LOG(10, "*** sio_mmu seg #%d for i&64, wfn is needed:%d\n", i, wfn);
        }
#endif
        rfn = sio_mrg;
        wfn = sio_mrg;
    }

#ifdef DEBUG
    if (pc24)
    {
        if (mmu_trans_all[0][i].address != adr)
            DEBUG_LOG(10, "*** mmu_t[0][%d].adr!=adr invalid/changed, good to reset it\n", i);
        if (mmu_trans_all[0][i].readfn != rfn)
            DEBUG_LOG(10, "*** mmu_t[0][%d].rfn!=rfn invalid/changed, good to reset it\n", i);
        if (mmu_trans_all[0][i].writefn != wfn)
            DEBUG_LOG(10, "*** mmu_t[0][%d].wfn!=wfn invalid/changed, good to reset it\n", i);
    }
#endif

    mmu_trans_all[0][i].address = adr;
    mmu_trans_all[0][i].readfn = rfn;
    mmu_trans_all[0][i].writefn = wfn;
    mmu_trans_all[0][i].table = NULL; // 20190601 fixed a huge bug!
}

void init_start_mode(void)
{
    long i;
    for (i = 0; i < 32768; i++)
        init_start_mode_segment(i);
}

void init_lisa_mmu(void)
{
    long i, j;

    start = 1;
    segment1 = 0;
    segment2 = 0;
    context = 0;
    mmudirty = 0;
    mmudirty_all[0] = 0;
    mmudirty_all[1] = 0;
    mmudirty_all[2] = 0;
    mmudirty_all[3] = 0;
    mmudirty_all[4] = 0;

    DEBUG_LOG(0, "Initializing... mmu_trans_all: %p mmu_all: %p", mmu_trans_all, mmu_all);

    for (i = 0; i < 32768; i++) // Initialize START mode (setup in hwg81) translation table
    {
        // First, zap all MMU contexts to bad pages.  This is to make sure that
        // we don't accidentally hit unused pages.  Alternatively, for debugging
        // purposes, we could uncomment this block and see if anything breaks.
        mmu_trans_all[0][i].table = NULL;

        mmu_trans_all[1][i].address = 0;
        mmu_trans_all[1][i].readfn = bad_page;
        mmu_trans_all[1][i].writefn = bad_page;
        mmu_trans_all[1][i].table = NULL;

        mmu_trans_all[2][i].address = 0;
        mmu_trans_all[2][i].readfn = bad_page;
        mmu_trans_all[2][i].writefn = bad_page;
        mmu_trans_all[2][i].table = NULL;

        mmu_trans_all[3][i].address = 0;
        mmu_trans_all[3][i].readfn = bad_page;
        mmu_trans_all[3][i].writefn = bad_page;
        mmu_trans_all[3][i].table = NULL;

        mmu_trans_all[4][i].address = 0;
        mmu_trans_all[4][i].readfn = bad_page;
        mmu_trans_all[4][i].writefn = bad_page;
        mmu_trans_all[4][i].table = NULL;

        init_start_mode_segment(i);

        mmudirty = 0;
        mmudirty_all[0] = 0;
        mmudirty_all[1] = 0xdec0de;
        mmudirty_all[2] = 0xdec0de;
        mmudirty_all[3] = 0xdec0de;
        mmudirty_all[4] = 0xdec0de;
    }

    for (i = 0; i < 128; i++) // Initialize START mode fake mmu table
    {
        mmu_all[0][i].sor = 0;
        mmu_all[0][i].slr = 0xf00;
        mmu_all[0][i].changed = 0;

        mmu_all[1][i].sor = 0;
        mmu_all[3][i].sor = 0;
        mmu_all[1][i].slr = 0xc00;
        mmu_all[3][i].slr = 0xc00;
        mmu_all[1][i].changed = 1;
        mmu_all[3][i].changed = 1;

        mmu_all[2][i].sor = 0;
        mmu_all[4][i].sor = 0;
        mmu_all[2][i].slr = 0xc00;
        mmu_all[4][i].slr = 0xc00;
        mmu_all[2][i].changed = 1;
        mmu_all[4][i].changed = 1;
    }

    // fill up memory
    for (j = 0, i = (minlisaram >> 9); i < (maxlisaram >> 9); i += ((128 * 1024) >> 9), j++)
    {
        mmu_all[1][j].sor = (i >> 9);
        mmu_all[1][j].slr = 0x700;
    }

    // Setup I/O space in page 126/127 of our context 1 (lisa context 0)
    mmu_all[1][126].slr = 0x900;
    mmu_all[1][126].sor = 0;
    mmu_all[1][127].slr = 0xf00;
    mmu_all[1][127].sor = 0;

    // Initialize Lisa MMU Function Types
    // fn assignments
    for (i = 0; i < MAX_LISA_MFN; i++)
    {
        mem68k_memptr[i] = lisa_mptr_OxERROR;
        mem68k_fetch_byte[i] = lisa_rb_OxERROR;
        mem68k_fetch_word[i] = lisa_rw_OxERROR;
        mem68k_fetch_long[i] = lisa_rl_OxERROR;
        mem68k_store_byte[i] = lisa_wb_OxERROR;
        mem68k_store_word[i] = lisa_ww_OxERROR;
        mem68k_store_long[i] = lisa_wl_OxERROR;
    }

    mem68k_memptr[OxUnused] = lisa_mptr_OxUnused;
    mem68k_fetch_byte[OxUnused] = lisa_rb_OxUnused;
    mem68k_fetch_word[OxUnused] = lisa_rw_OxUnused;
    mem68k_fetch_long[OxUnused] = lisa_rl_OxUnused;
    mem68k_store_byte[OxUnused] = lisa_wb_OxUnused;
    mem68k_store_word[OxUnused] = lisa_ww_OxUnused;
    mem68k_store_long[OxUnused] = lisa_wl_OxUnused;

    mem68k_memptr[Ox0000_slot1] = lisa_mptr_Ox0000_slot1;
    mem68k_fetch_byte[Ox0000_slot1] = lisa_rb_Ox0000_slot1;
    mem68k_fetch_word[Ox0000_slot1] = lisa_rw_Ox0000_slot1;
    mem68k_fetch_long[Ox0000_slot1] = lisa_rl_Ox0000_slot1;
    mem68k_store_byte[Ox0000_slot1] = lisa_wb_Ox0000_slot1;
    mem68k_store_word[Ox0000_slot1] = lisa_ww_Ox0000_slot1;
    mem68k_store_long[Ox0000_slot1] = lisa_wl_Ox0000_slot1;

    mem68k_memptr[Ox2000_slot1] = lisa_mptr_Ox2000_slot1;
    mem68k_fetch_byte[Ox2000_slot1] = lisa_rb_Ox2000_slot1;
    mem68k_fetch_word[Ox2000_slot1] = lisa_rw_Ox2000_slot1;
    mem68k_fetch_long[Ox2000_slot1] = lisa_rl_Ox2000_slot1;
    mem68k_store_byte[Ox2000_slot1] = lisa_wb_Ox2000_slot1;
    mem68k_store_word[Ox2000_slot1] = lisa_ww_Ox2000_slot1;
    mem68k_store_long[Ox2000_slot1] = lisa_wl_Ox2000_slot1;

    mem68k_memptr[Ox4000_slot2] = lisa_mptr_Ox4000_slot2;
    mem68k_fetch_byte[Ox4000_slot2] = lisa_rb_Ox4000_slot2;
    mem68k_fetch_word[Ox4000_slot2] = lisa_rw_Ox4000_slot2;
    mem68k_fetch_long[Ox4000_slot2] = lisa_rl_Ox4000_slot2;
    mem68k_store_byte[Ox4000_slot2] = lisa_wb_Ox4000_slot2;
    mem68k_store_word[Ox4000_slot2] = lisa_ww_Ox4000_slot2;
    mem68k_store_long[Ox4000_slot2] = lisa_wl_Ox4000_slot2;

    mem68k_memptr[Ox6000_slot2] = lisa_mptr_Ox6000_slot2;
    mem68k_fetch_byte[Ox6000_slot2] = lisa_rb_Ox6000_slot2;
    mem68k_fetch_word[Ox6000_slot2] = lisa_rw_Ox6000_slot2;
    mem68k_fetch_long[Ox6000_slot2] = lisa_rl_Ox6000_slot2;
    mem68k_store_byte[Ox6000_slot2] = lisa_wb_Ox6000_slot2;
    mem68k_store_word[Ox6000_slot2] = lisa_ww_Ox6000_slot2;
    mem68k_store_long[Ox6000_slot2] = lisa_wl_Ox6000_slot2;

    mem68k_memptr[Ox8000_slot3] = lisa_mptr_Ox8000_slot3;
    mem68k_fetch_byte[Ox8000_slot3] = lisa_rb_Ox8000_slot3;
    mem68k_fetch_word[Ox8000_slot3] = lisa_rw_Ox8000_slot3;
    mem68k_fetch_long[Ox8000_slot3] = lisa_rl_Ox8000_slot3;
    mem68k_store_byte[Ox8000_slot3] = lisa_wb_Ox8000_slot3;
    mem68k_store_word[Ox8000_slot3] = lisa_ww_Ox8000_slot3;
    mem68k_store_long[Ox8000_slot3] = lisa_wl_Ox8000_slot3;

    mem68k_memptr[Oxa000_slot3] = lisa_mptr_Oxa000_slot3;
    mem68k_fetch_byte[Oxa000_slot3] = lisa_rb_Oxa000_slot3;
    mem68k_fetch_word[Oxa000_slot3] = lisa_rw_Oxa000_slot3;
    mem68k_fetch_long[Oxa000_slot3] = lisa_rl_Oxa000_slot3;
    mem68k_store_byte[Oxa000_slot3] = lisa_wb_Oxa000_slot3;
    mem68k_store_word[Oxa000_slot3] = lisa_ww_Oxa000_slot3;
    mem68k_store_long[Oxa000_slot3] = lisa_wl_Oxa000_slot3;

    mem68k_memptr[Oxc000_flopmem] = lisa_mptr_Oxc000_flopmem;
    mem68k_fetch_byte[Oxc000_flopmem] = lisa_rb_Oxc000_flopmem;
    mem68k_fetch_word[Oxc000_flopmem] = lisa_rw_Oxc000_flopmem;
    mem68k_fetch_long[Oxc000_flopmem] = lisa_rl_Oxc000_flopmem;
    mem68k_store_byte[Oxc000_flopmem] = lisa_wb_Oxc000_flopmem;
    mem68k_store_word[Oxc000_flopmem] = lisa_ww_Oxc000_flopmem;
    mem68k_store_long[Oxc000_flopmem] = lisa_wl_Oxc000_flopmem;

    mem68k_memptr[Oxd200_sccz8530] = lisa_mptr_Oxd200_sccz8530;
    mem68k_fetch_byte[Oxd200_sccz8530] = lisa_rb_Oxd200_sccz8530;
    mem68k_fetch_word[Oxd200_sccz8530] = lisa_rw_Oxd200_sccz8530;
    mem68k_fetch_long[Oxd200_sccz8530] = lisa_rl_Oxd200_sccz8530;
    mem68k_store_byte[Oxd200_sccz8530] = lisa_wb_Oxd200_sccz8530;
    mem68k_store_word[Oxd200_sccz8530] = lisa_ww_Oxd200_sccz8530;
    mem68k_store_long[Oxd200_sccz8530] = lisa_wl_Oxd200_sccz8530;

    mem68k_memptr[Oxd800_par_via2] = lisa_mptr_Oxd800_par_via2;
    mem68k_fetch_byte[Oxd800_par_via2] = lisa_rb_Oxd800_par_via2;
    mem68k_fetch_word[Oxd800_par_via2] = lisa_rw_Oxd800_par_via2;
    mem68k_fetch_long[Oxd800_par_via2] = lisa_rl_Oxd800_par_via2;
    mem68k_store_byte[Oxd800_par_via2] = lisa_wb_Oxd800_par_via2;
    mem68k_store_word[Oxd800_par_via2] = lisa_ww_Oxd800_par_via2;
    mem68k_store_long[Oxd800_par_via2] = lisa_wl_Oxd800_par_via2;

    mem68k_memptr[Oxdc00_cops_via1] = lisa_mptr_Oxdc00_cops_via1;
    mem68k_fetch_byte[Oxdc00_cops_via1] = lisa_rb_Oxdc00_cops_via1;
    mem68k_fetch_word[Oxdc00_cops_via1] = lisa_rw_Oxdc00_cops_via1;
    mem68k_fetch_long[Oxdc00_cops_via1] = lisa_rl_Oxdc00_cops_via1;
    mem68k_store_byte[Oxdc00_cops_via1] = lisa_wb_Oxdc00_cops_via1;
    mem68k_store_word[Oxdc00_cops_via1] = lisa_ww_Oxdc00_cops_via1;
    mem68k_store_long[Oxdc00_cops_via1] = lisa_wl_Oxdc00_cops_via1;

    mem68k_memptr[Oxe000_latches] = lisa_mptr_Oxe000_latches;
    mem68k_fetch_byte[Oxe000_latches] = lisa_rb_Oxe000_latches;
    mem68k_fetch_word[Oxe000_latches] = lisa_rw_Oxe000_latches;
    mem68k_fetch_long[Oxe000_latches] = lisa_rl_Oxe000_latches;
    mem68k_store_byte[Oxe000_latches] = lisa_wb_Oxe000_latches;
    mem68k_store_word[Oxe000_latches] = lisa_ww_Oxe000_latches;
    mem68k_store_long[Oxe000_latches] = lisa_wl_Oxe000_latches;

    mem68k_memptr[Oxe800_videlatch] = lisa_mptr_Oxe800_videlatch;
    mem68k_fetch_byte[Oxe800_videlatch] = lisa_rb_Oxe800_videlatch;
    mem68k_fetch_word[Oxe800_videlatch] = lisa_rw_Oxe800_videlatch;
    mem68k_fetch_long[Oxe800_videlatch] = lisa_rl_Oxe800_videlatch;
    mem68k_store_byte[Oxe800_videlatch] = lisa_wb_Oxe800_videlatch;
    mem68k_store_word[Oxe800_videlatch] = lisa_ww_Oxe800_videlatch;
    mem68k_store_long[Oxe800_videlatch] = lisa_wl_Oxe800_videlatch;

    mem68k_memptr[Oxf000_memerror] = lisa_mptr_Oxf000_memerror;
    mem68k_fetch_byte[Oxf000_memerror] = lisa_rb_Oxf000_memerror;
    mem68k_fetch_word[Oxf000_memerror] = lisa_rw_Oxf000_memerror;
    mem68k_fetch_long[Oxf000_memerror] = lisa_rl_Oxf000_memerror;
    mem68k_store_byte[Oxf000_memerror] = lisa_wb_Oxf000_memerror;
    mem68k_store_word[Oxf000_memerror] = lisa_ww_Oxf000_memerror;
    mem68k_store_long[Oxf000_memerror] = lisa_wl_Oxf000_memerror;

    mem68k_memptr[Oxf800_statreg] = lisa_mptr_Oxf800_statreg;
    mem68k_fetch_byte[Oxf800_statreg] = lisa_rb_Oxf800_statreg;
    mem68k_fetch_word[Oxf800_statreg] = lisa_rw_Oxf800_statreg;
    mem68k_fetch_long[Oxf800_statreg] = lisa_rl_Oxf800_statreg;
    mem68k_store_byte[Oxf800_statreg] = lisa_wb_Oxf800_statreg;
    mem68k_store_word[Oxf800_statreg] = lisa_ww_Oxf800_statreg;
    mem68k_store_long[Oxf800_statreg] = lisa_wl_Oxf800_statreg;

    mem68k_memptr[ram] = lisa_mptr_ram;
    mem68k_fetch_byte[ram] = lisa_rb_ram;
    mem68k_fetch_word[ram] = lisa_rw_ram;
    mem68k_fetch_long[ram] = lisa_rl_ram;
    mem68k_store_byte[ram] = lisa_wb_ram;
    mem68k_store_word[ram] = lisa_ww_ram;
    mem68k_store_long[ram] = lisa_wl_ram;

    mem68k_memptr[ro_violn] = lisa_mptr_ro_violn;
    mem68k_fetch_byte[ro_violn] = lisa_rb_ro_violn;
    mem68k_fetch_word[ro_violn] = lisa_rw_ro_violn;
    mem68k_fetch_long[ro_violn] = lisa_rl_ro_violn;
    mem68k_store_byte[ro_violn] = lisa_wb_ro_violn;
    mem68k_store_word[ro_violn] = lisa_ww_ro_violn;
    mem68k_store_long[ro_violn] = lisa_wl_ro_violn;

    mem68k_memptr[bad_page] = lisa_mptr_bad_page;
    mem68k_fetch_byte[bad_page] = lisa_rb_bad_page;
    mem68k_fetch_word[bad_page] = lisa_rw_bad_page;
    mem68k_fetch_long[bad_page] = lisa_rl_bad_page;
    mem68k_store_byte[bad_page] = lisa_wb_bad_page;
    mem68k_store_word[bad_page] = lisa_ww_bad_page;
    mem68k_store_long[bad_page] = lisa_wl_bad_page;

    mem68k_memptr[sio_rom] = lisa_mptr_sio_rom;
    mem68k_fetch_byte[sio_rom] = lisa_rb_sio_rom;
    mem68k_fetch_word[sio_rom] = lisa_rw_sio_rom;
    mem68k_fetch_long[sio_rom] = lisa_rl_sio_rom;
    mem68k_store_byte[sio_rom] = lisa_wb_sio_rom;
    mem68k_store_word[sio_rom] = lisa_ww_sio_rom;
    mem68k_store_long[sio_rom] = lisa_wl_sio_rom;

    mem68k_memptr[sio_mrg] = lisa_mptr_sio_mrg;
    mem68k_fetch_byte[sio_mrg] = lisa_rb_sio_mrg;
    mem68k_fetch_word[sio_mrg] = lisa_rw_sio_mrg;
    mem68k_fetch_long[sio_mrg] = lisa_rl_sio_mrg;
    mem68k_store_byte[sio_mrg] = lisa_wb_sio_mrg;
    mem68k_store_word[sio_mrg] = lisa_ww_sio_mrg;
    mem68k_store_long[sio_mrg] = lisa_wl_sio_mrg;

    mem68k_memptr[sio_mmu] = lisa_mptr_sio_mmu;
    mem68k_fetch_byte[sio_mmu] = lisa_rb_sio_mmu;
    mem68k_fetch_word[sio_mmu] = lisa_rw_sio_mmu;
    mem68k_fetch_long[sio_mmu] = lisa_rl_sio_mmu;
    mem68k_store_byte[sio_mmu] = lisa_wb_sio_mmu;
    mem68k_store_word[sio_mmu] = lisa_ww_sio_mmu;
    mem68k_store_long[sio_mmu] = lisa_wl_sio_mmu;

    mem68k_memptr[vidram] = lisa_mptr_vidram;
    mem68k_fetch_byte[vidram] = lisa_rb_vidram;
    mem68k_fetch_word[vidram] = lisa_rw_vidram;
    mem68k_fetch_long[vidram] = lisa_rl_vidram;
    mem68k_store_byte[vidram] = lisa_wb_vidram;
    mem68k_store_word[vidram] = lisa_ww_vidram;
    mem68k_store_long[vidram] = lisa_wl_vidram;

    mem68k_memptr[io] = lisa_mptr_io;
    mem68k_fetch_byte[io] = lisa_rb_io;
    mem68k_fetch_word[io] = lisa_rw_io;
    mem68k_fetch_long[io] = lisa_rl_io;
    mem68k_store_byte[io] = lisa_wb_io;
    mem68k_store_word[io] = lisa_ww_io;
    mem68k_store_long[io] = lisa_wl_io;

    mem68k_memptr[Oxd400_amd9512] = lisa_mptr_Oxd400_amd9512;
    mem68k_fetch_byte[Oxd400_amd9512] = lisa_rb_Oxd400_amd9512;
    mem68k_fetch_word[Oxd400_amd9512] = lisa_rw_Oxd400_amd9512;
    mem68k_fetch_long[Oxd400_amd9512] = lisa_rl_Oxd400_amd9512;
    mem68k_store_byte[Oxd400_amd9512] = lisa_wb_Oxd400_amd9512;
    mem68k_store_word[Oxd400_amd9512] = lisa_ww_Oxd400_amd9512;
    mem68k_store_long[Oxd400_amd9512] = lisa_wl_Oxd400_amd9512;

    mem68k_memptr[Oxd000_ff_space] = lisa_mptr_Oxd000_ff_space;
    mem68k_fetch_byte[Oxd000_ff_space] = lisa_rb_Oxd000_ff_space;
    mem68k_fetch_word[Oxd000_ff_space] = lisa_rw_Oxd000_ff_space;
    mem68k_fetch_long[Oxd000_ff_space] = lisa_rl_Oxd000_ff_space;
    mem68k_store_byte[Oxd000_ff_space] = lisa_wb_Oxd000_ff_space;
    mem68k_store_word[Oxd000_ff_space] = lisa_ww_Oxd000_ff_space;
    mem68k_store_long[Oxd000_ff_space] = lisa_wl_Oxd000_ff_space;

    mem68k_memptr[OxVoid] = lisa_mptr_OxVoid;
    mem68k_fetch_byte[OxVoid] = lisa_rb_OxVoid;
    mem68k_fetch_word[OxVoid] = lisa_rw_OxVoid;
    mem68k_fetch_long[OxVoid] = lisa_rl_OxVoid;
    mem68k_store_byte[OxVoid] = lisa_wb_OxVoid;
    mem68k_store_word[OxVoid] = lisa_ww_OxVoid;
    mem68k_store_long[OxVoid] = lisa_wl_OxVoid;

    DEBUG_LOG(0, "mmu_trans_all: %p mmu_all: %p", mmu_trans_all, mmu_all);
    segment1 = 0;
    segment2 = 0;
    start = 1;
    mmuflush(0);

    mmu0_checksum = get_mmu0_chk();

    DEBUG_LOG(0, "mmu_trans_all: %p mmu_all: %p", mmu_trans_all, mmu_all);
}

#ifdef DEBUG
char *printslr(char *x, long size, uint16 slr)
{
    *x = 0;

    memset(x, 0, size - 1);

    if (slr & 2048)
        strncat(x, "!mem ", size);
    else
        strncat(x, " mem ", size);

    if (slr & 1024)
        strncat(x, "!io  ", size);
    else
        strncat(x, " io  ", size);

    if (slr & 512)
        strncat(x, " rw  ", size);
    else
        strncat(x, " ro ", size);

    if (slr & 256)
        strncat(x, "!stk ", size);
    else
        strncat(x, " stk ", size);

    if ((slr & FILTR) == SLR_RO_STK)
        strncat(x, "=r/o stack", size);
    else if ((slr & FILTR) == SLR_RO_MEM)
        strncat(x, "=r/o mem", size);
    else if ((slr & FILTR) == SLR_RW_STK)
        strncat(x, "=r/w stack", size);
    else if ((slr & FILTR) == SLR_RW_MEM)
        strncat(x, "=r/w mem", size);
    else if ((slr & FILTR) == SLR_IO_SPACE)
        strncat(x, "=I/O space", size);
    else if ((slr & FILTR) == SLR_UNUSED_PAGE)
        strncat(x, "=bad_page", size);
    else if ((slr & FILTR) == SLR_SIO_SPACE)
        strncat(x, "=SI/O space", size);
    else
        strncat(x, "** INVALID CODE **", size);

    return x;
}

#endif

// context 0-3 are normal lisa context, except supervisor mode suggest use of context 0, but doesn't force it
// if SETUP bit is set, I go to context 5.        fill_mmu_segment(segment,ea,rfn,wfn,start,end);

void fill_mmu_segment(uint8 segment, int32 ea, lisa_mem_t rfn, lisa_mem_t wfn, int pagestart, int pageend) ///////////////////////////////////////
{
    mmu_trans_t *mt = NULL;
    int i = 0;
    int32 segment8 = segment << 8;
    int cx = CXSEL;

#ifdef DEBUG
    uint32 in, out;
#endif

    if (segment > 127)
    {
        DEBUG_LOG(-1, "Error: segment passed is out of range %d", segment);
        return;
    }
    if (start)
    {
        return;
    }

    DEBUG_LOG(0, "starting on segment %d", segment);

    if (!is_valid_slr(mmu_all[cx][segment].slr))
    {
        pagestart = -1;
        pageend = -1;
        DEBUG_LOG(0, "Illegal SLR:%04x @ seg %d in context:%d, forcing bad_page", mmu_all[cx][segment].slr, segment, context);
    }

    if (pagestart < 0 || pageend < 0 || pagestart > pageend || pagestart > 255 || pageend > 255) // invalidate the whole block, no need for checking pagestart/pageend
    {
        for (i = 0; i < 256; i++) // wipe all pages in a single segment -- the simpler version!
        {
            mt = &mmu_trans_all[cx][segment8 + i];
            mt->address = 0;
            mt->readfn = bad_page;
            mt->writefn = bad_page; // wipe it, wipe it good.
            if (mt->table)
            {
                DEBUG_LOG(0, "calling free_ipct for mmu_trans_all[%d][%ld]", cx, (long)(segment8 + i));
                free_ipct(mt->table);
                mt->table = NULL;
            } // invalidate ipct's
              // and wipe pointer to be sure it won't be followed
        }
    }
    else if (wfn == ram) // wfn to ram is special because writes could be to video ram which I need to trap
    {
        for (i = 0; i < 256; i++) // there are 256 words for every mmu page.
        {
            // uint16 sor=mmu_all[cx][segment].sor & 0xfff;
            uint32 epage = segment8 + i;
            uint32 epage9 = epage << 9;
            mt = &mmu_trans_all[cx][epage];

            if (i >= pagestart && i <= pageend)
            {
                if ((videolatchaddress + 32768) > (epage9) && videolatchaddress <= (epage9))
                {
                    mt->readfn = ram;
                    mt->writefn = vidram;
                    mt->address = ea;
                    lastvideo_mt = mt;
                }
                else
                {
                    mt->readfn = ram;
                    mt->writefn = ram;
                    mt->address = ea;
                }
            }
            else
            {
                mt->readfn = bad_page;
                mt->writefn = bad_page;
                mt->address = 0;
            }

#ifdef DEBUG
            in = (segment << 17) + (i << 9);
            out = (mmu_all[cx][segment].sor << 9) + (i << 9);
            if (in + ea != out)
            {
                DEBUG_LOG(10, "*** 1 MMU Additon is incorrect in+ea!=out %08x+%08x!=%08x in+ea=%08x  ea should be:%08x",
                          in, ea, out, in + ea, out - in);
            }
#endif
            if (mt->table)
            {
                DEBUG_LOG(0, "calling free_ipct for mmu_trans_all[%d][%ld]", cx, (long)(epage));
                free_ipct(mt->table);
                mt->table = NULL;
            } // invalidate ipct's
              //   and wipe pointer to be sure it won't be followed
        }
    }
    else
    {
        for (i = 0; i < 256; i++) // All others handled here
        {
            // uint16 sor=mmu_all[cx][segment].sor & 0xfff;
            mt = &mmu_trans_all[cx][segment8 + i];
            if (!mt)
            {
                EXIT(339, 0, "mt is null %s:%s:%d cx:%d segment:%d page:%d\n\n", __FILE__, __FUNCTION__, __LINE__, cx, segment, i);
            }

            if (i >= pagestart && i <= pageend)
            {
                mt->readfn = rfn;
                mt->writefn = wfn;
                mt->address = ea;
            }
            else
            {
                mt->readfn = bad_page;
                mt->writefn = bad_page;
                mt->address = 0;
            }

            if (mt->table)
            {
                DEBUG_LOG(0, "calling free_ipct for mmu_trans_all[%d][%ld]", cx, (long)(segment8 + i));
                free_ipct(mt->table);
                mt->table = NULL;
            } // invalidate ipct's
              //   and wipe pointer to be sure it won't be followed

#ifdef DEBUG
            in = (segment << 17) + (i << 9);
            out = (mmu[segment].sor << 9) + (i << 9);
            if (in + ea != out)
            {
                EXIT(340, 0, "*** 2 MMU Additon is incorrect in+ea!=out %08x+%08x!=%08x in+ea=%08x  ea should be:%08x",
                     in, ea, out, in + ea, out - in);
            }
#endif
        }
    }

} //////// end of fill_segment /////////////////////////////////////////////////////////////////////////////////////////////////

void invalidate_mmu_segment(uint8 segment)
{
    DEBUG_LOG(0, "invalidate %d", segment);
    fill_mmu_segment(segment, 0L, bad_page, bad_page, -1, -1);
}

void get_slr_page_range(int cx, int seg, int16 *pagestart, int16 *pageend, lisa_mem_t *rfn, lisa_mem_t *wfn)
{
    uint8 page;
    uint16 slr = mmu_all[cx][seg].slr;

    page = ((slr & 0xff) ^ 0xff);

    switch (slr & FILTR)
    {
    case SLR_RO_STK:
        *pagestart = page;
        *pageend = 255;
        *rfn = ram;
        *wfn = ro_violn;
        return; // start=page|stkpage, end=255 works anything else crashes LOS
    case SLR_RW_STK:
        *pagestart = page;
        *pageend = 255;
        *rfn = ram;
        *wfn = ram;
        return; // 2020.11.08 was pagestart=page;
    case SLR_RO_MEM:
        *pagestart = 0;
        *pageend = page;
        *rfn = ram;
        *wfn = ro_violn;
        return;
    case SLR_RW_MEM:
        *pagestart = 0;
        *pageend = page;
        *rfn = ram;
        *wfn = ram;
        return;
    case SLR_IO_SPACE:
        *pagestart = 0;
        *pageend = page;
        *rfn = io;
        *wfn = io;
        return;
    case SLR_SIO_SPACE:
        *pagestart = 0;
        *pageend = page;
        *rfn = sio_mmu;
        *wfn = sio_mmu;
        return;
    case SLR_UNUSED_PAGE:
    default:
        *pagestart = 0;
        *pageend = 255;
        *rfn = bad_page;
        *wfn = bad_page;
        return;
    }
}

// assumes that mmu[segment] has been set for both sor and slr.
void create_mmu_segment(uint8 segment)
{
    int32 ea;        // epage,         // page and effective page (sor+/-page), and diff address ea
    uint16 sor, slr; // mmu regs
    lisa_mem_t rfn = bad_page, wfn = bad_page;
    int16 pagestart, pageend;

    int32 sor9, segment17;
#ifdef DEBUG
    char s1[160]; // s2[160];
    if (segment > 127)
    {
        EXIT(42, 0, "Create_mmu_segment passed segment>127!");
    }
#endif

    if (start)
    {
        DEBUG_LOG(0, "context=0");
        init_start_mode_segment(segment);
        return;
    }

    sor = mmu[segment].sor & 0x0fff;
    slr = mmu[segment].slr;

    DEBUG_LOG(1, "MMU page type to set: %s (%04x) segment #%d in context %d", printslr(s1, 160, slr), slr, segment, context);

    segment17 = (int32)(segment << 17);
    sor9 = (int32)(sor << 9);
    ea = GET_MMU_EFFECTIVE_ADDRESS(segment17, sor9);

    get_slr_page_range(context, segment, &pagestart, &pageend, &rfn, &wfn);
    DEBUG_LOG(0, "MMU: Got values from page_range Segment:%d ea:%08x, rfn:%d, wfn:%d, pagestart:%08x,pageend:%08x\n", segment, ea, rfn, wfn, pagestart, pageend);

    fill_mmu_segment(segment, ea, rfn, wfn, pagestart, pageend);

    //    #ifdef DEBUG
    //    if ( (slr & FILTR)==SLR_RO_STK || (slr & FILTR)==SLR_RW_STK )  {
    //       ALERT_LOG(0,"stack segment:mmu[%d][%d].slr=%03x, sor=%03x, pagestart:%d pageend:%d",context,segment,slr,sor,pagestart,pageend);
    //       ALERT_LOG(0,"stack segment MMU: Got values from page_range Segment:%d ea:%08x, rfn:%d, wfn:%d, pagestart:%08x,pageend:%08x\n",segment,ea,rfn,wfn,pagestart,pageend);
    //       dumpmmupage(context, segment, stdout);
    //       if (buglog) dumpmmupage(context, segment, buglog);
    //
    //    }
    //    #endif

    DEBUG_LOG(0, "MMU: Segment:%d ea:%08x, rfn:%d, wfn:%d, pagestart:%08x,pageend:%08x\n", segment, ea, rfn, wfn, pagestart, pageend);

#ifdef DEBUG
    check_mmu_segment(segment, ea, rfn, wfn, pagestart, pageend, "create_mmu_segment1");
// DEBUG_LOG(10,"post check_mmu_segment:: context=%d s1/s2=%d/%d start=%d\n",context,segment1,segment2,start);
#endif
    return;

} //---------------- end of function --------------------------------------------------------------------------

/*
Flush dirty MMU pages to our mmu_trans structures.

If the MMU is already dirty and it's a different register set, MMUDIRTY will be >128 since
we shift the last mmudirty to the left 8 bits.  This situation then indicates that we need
to flush all segments with mmu[seg].changed=1 in the current context (discounting START mode)

We do a+1 since segment 0 would be dirty, but need to set mmudirty to nonzero.

And we don't flush the mmu changes until we either change contexts, or we need to use
the MMU map while in special I/O mode.  This way we can be a bit lazy and avoid needless
MMU map recalculations.
*/

/////////////////////////////////////////////////////////////////////////////////
// reg68k externs - allow us to make sure that lastsflag is correct before     //
// changing contexts, else we face disasterous unpredictable results!          //
/////////////////////////////////////////////////////////////////////////////////

extern void reg68k_update_supervisor_internal(void);
extern void reg68k_update_supervisor_external(void);

extern void refresh_vidram_fns(void);

// opts |=0x1000 turn on supervisor
// opts |=0x2000 full mmu flush whether needed or not

void mmuflush(uint16 opts)
{
    int i, changedcx[5]; //,j

    /* Save context and push to context without START mode.  Changes to the MMU map during Context 0 (START)
       need to propagate to the proper MMU context bank!  This ensures that it's done.  Further, our context 0
       is a fake context and should never be changed as this would destroy the proper translation table for it.
       Instead we rely on the lisa_??_sio_mmu to access to the right context when the MMU map addressing is
       in use while in START (context 0)  because of this it needs to be reset to start mode defaults.*/

#ifdef DEBUG
    DEBUG_LOG(10, "Entering MMUflush Context Change - current context:%d", context);
    // dumpmmu(context,buglog);
#endif

    // handle options.
    if (opts & 0x1000)
        lastsflag = 1;
    else
        reg68k_update_supervisor_external();
    if (opts & 0x2000)
        mmudirty = 0xdec0de;

    CONTEXTSELECTOR();

    DEBUG_LOG(10, "mmu flush - old context:%d supervisor_flag:%d current s flag:%d switching to context=%d s1/s2=%d/%d start=%d\n",
              lastcontext, lastsflag, regs.sr.sr_struct.s,
              context, segment1, segment2, start);

    // fflush(buglog);
    if (!context)
    { // we need to do special things for magical mystery context 0.
        int i;

        if (lastcontext && lastcontext < 5)
            for (i = 0; i < 128; i++)
            {
                mmu_all[0][i].sor = mmu_all[lastcontext][i].sor;
                mmu_all[0][i].slr = mmu_all[lastcontext][i].slr;
                mmu_all[0][i].changed = mmu_all[lastcontext][i].changed; // was zero...
            }

        init_start_mode();
        lastcontext = context;
        DEBUG_LOG(10, "Initialized start mode, done with mmuflush\n");
        return;
    }

    ////// there's some fucked up bug where the flush doesn't properly happen ////////////////////////////////
    changedcx[0] = 0;
    changedcx[1] = 0;
    changedcx[2] = 0;
    changedcx[3] = 0;
    changedcx[4] = 0;

    // for (j=1; j<5; j++)
    // for (i=0; i<128; i++) if (mmu_all[j][i].changed) cxchanged[cx]=1;
    for (i = 0; i < 128; i++)
        if (mmu[i].changed)
            changedcx[context]++;
    if (changedcx[context] > 1)
        mmudirty = 0xdec0de;
    if (changedcx[context] == 1 && !mmudirty)
        mmudirty = 0xdec0de;
    /////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (!mmudirty)
    {
        lastcontext = context;
        DEBUG_LOG(10, "mmu really isn't dirty, returning.\n");
        return;
    }

    // if (!mmudirty) mmudirty=0xdec0de;   // temporary hack, let's see what happens.

    DEBUG_LOG(0, "context=%d s1/s2=%d/%d start=%d", context, segment1, segment2, start);
    DEBUG_LOG(0, "mmudirty=%08x mmu context=%d", mmudirty, context);

    if (mmudirty <= 128)  // singe mmu change
    {                     ////////
        i = mmudirty - 1; // correct it, -1 since we did a+1 in the lisa_??_mrg fn's - check the changed flag just incase.
        if (i < 0)
        {
            EXIT(341, 10, "i<0!!!!!!!!");
        }

        if (mmu[i].changed && is_valid_slr(mmu[i].slr))
        {
            DEBUG_LOG(0, "flushing mmu segment %d (invalidate+create) in context %d slr=%04x,sor=%04x", i, context, mmu[i].slr, mmu[i].sor);
            create_mmu_segment(i);
            mmu[i].changed = 0;
        }
        else
        {
            mmudirty = 0xdec0de;
        } // fall through, something's odd}

    } /////////

    if (mmudirty > 128) // ---------------------------------------------------------------------------------------------------
    {
        DEBUG_LOG(10, "mmu is dirty(context switch or more than one mmu change- doing all changed registers with valid SLR's\n");
        for (i = 0; i < 128; i++)
            if (mmu[i].changed && is_valid_slr(mmu[i].slr))
            {
                DEBUG_LOG(0, "flushing mmu segment %d (invalidate+create) in context %d slr=%04x,sor=%04x", i, context, mmu[i].slr, mmu[i].sor);
                create_mmu_segment(i);
                mmu[i].changed = 0;
            }

        SET_MMU_DIRTY(0); // MMU is now clean.
    }                     // 0xdec0de --- ends
#ifdef DEBUG
    DEBUG_LOG(10, "Validating MMU segments.");
    DEBUG_LOG(10, "pre-validating context=%d s1/s2=%d/%d start=%d\n", context, segment1, segment2, start);
    validate_mmu_segments("mmu.c end of flush");
    DEBUG_LOG(10, "post-validating context=%d s1/s2=%d/%d start=%d\n", context, segment1, segment2, start);
#endif

    lastcontext = context;
    DEBUG_LOG(10, "Done with MMU Flushcontext=%d s1/s2=%d/%d start=%d\n", context, segment1, segment2, start);

    //    refresh_vidram_fns();
}

#ifdef DEBUG
static int mmu_seg_error = 0;

void check_mmu_segment(uint8 segment, int32 ea, lisa_mem_t rfn, lisa_mem_t wfn, int pagestart, int pageend, char *from) ///////////////////////////////////////
{
    uint32 mymaxlisaram = maxlisaram;
    mmu_trans_t *mt = NULL;
    int i = 0;
    int32 segment8 = segment << 8, page, epage, xea;
    lisa_mem_t xrfn, xwfn;
    int cx = CXSEL;

    // 20051111
    if (maxlisaram == 1024 * 1024)
        mymaxlisaram += 0x80000;
    ///////////////////////

    return; // blah

    if (!debug_log_enabled)
        return;

    if (segment > 127)
    {
        DEBUG_LOG(-1, "Error: segment passed is out of range %d %s", segment, from);
        return;
    }
    if (start)
    {
        return;
    }

    if (!is_valid_slr(mmu_all[cx][segment].slr))
    {
        pagestart = -1;
        pageend = -1;
    }

    if (pagestart < 0 || pageend < 0 || pageend < pagestart || pagestart > 255 || pageend > 255) // invalidate the whole block, no need for checking pagestart/pageend
    {
        for (i = 0; i < 256; i++) // wipe all pages in a single segment -- the simpler version!
        {
            mt = &mmu_trans_all[cx][segment8 + i];
            if (!mt)
            {
                EXIT(2, 0, "mt is null!");
            }

            xrfn = bad_page;
            xwfn = bad_page;
            xea = 0;
            page = i;
            epage = segment8 + i;

            if (mt->address != 0 || mt->readfn != bad_page || mt->writefn != bad_page)
            { // debug_log_enabled=1;
                DEBUG_LOG(0, "3 mmu validation failed %d::%d/%d/%d seg:%02x(%d) page:%i epage:0x%04x(%d), slr:%s:sor:%04x\n s/b %ld/%08x,%d/%d(%s/%s) is: %ld/%08x,%d/%d(%s/%s) pages:%d-%d '%s'",
                          context, segment1, segment2, start,
                          segment, segment, page, epage, epage, slrname(mmu_all[cx][segment].slr), mmu_all[cx][segment].sor,
                          xea, xea, xrfn, xwfn,
                          mspace(xrfn), mspace(xwfn),
                          mt->address, mt->address, mt->readfn, mt->writefn,
                          mspace(mt->readfn), mspace(mt->writefn),
                          pagestart, pageend, from);
                mmu_seg_error = 1;
            }
        }
    }
    else if (wfn == ram) // wfn to ram is special because writes could be to video ram which I need to trap
    {
        for (i = 0; i < 256; i++)
        {
            uint32 sor = (uint32)(mmu_all[cx][segment].sor & 0xfff);
            uint32 epage = segment8 + i;
            uint32 epage9 = epage << 9;
            mt = &mmu_trans_all[cx][epage];
            if (!mt)
            {
                EXIT(2, 0, "mt is null!");
            }
            page = i;
            epage = segment8 + i;

            if (i >= pagestart && i <= pageend)
            {
                if ((videolatchaddress + 32768) > (epage9) && videolatchaddress <= (epage9))
                {
                    xrfn = ram;
                    xwfn = vidram;
                    xea = ea;
                }
                else
                {
                    xrfn = ram;
                    xwfn = ram;
                    xea = ea;
                }
            }
            else
            {
                xea = 0;
                xrfn = bad_page;
                xwfn = bad_page;
            }

            if (mt->writefn == ram && xwfn == vidram)
                xwfn = ram; // bypass incase video page has changed.
            if (mt->writefn == vidram && xwfn == ram)
                xwfn = vidram; // bypass incase video page has changed.

            if (mt->address != xea || mt->readfn != xrfn || mt->writefn != xwfn)
            { // debug_log_enabled=1;
                DEBUG_LOG(0, "vidram:%08x-%08x", videolatchaddress, videolatchaddress + 32767);
                DEBUG_LOG(0, "2 mmu validation failed %d::%d/%d/%d seg:%02x(%d) page:%i epage:0x%04x(%d), slr:%s:sor:%04x\n s/b %ld/%08x,%d/%d(%s/%s) is: %ld/%08x,%d/%d(%s/%s) pages:%d-%d",
                          context, segment1, segment2, start,
                          segment, segment, page, epage, epage, slrname(mmu_all[cx][segment].slr), mmu_all[cx][segment].sor,
                          xea, xea, xrfn, xwfn,
                          mspace(xrfn), mspace(xwfn),
                          mt->address, mt->address, mt->readfn, mt->writefn,
                          mspace(mt->readfn), mspace(mt->writefn), pagestart, pageend);
                mmu_seg_error = 1;
            }

            if ((uint32)(((sor + pagestart) << 9) + 0) > mymaxlisaram)
            {
                DEBUG_LOG(0, "**** cx:%d pc24=%08x START RANGE OVER MAXLISARAM (%08x)!!!!!! chopped to 2mb it's:%08x, unchopped:%08x****\n",
                          cx, pc24, mymaxlisaram,
                          (((((mmu_all[cx][segment].sor & 0xfff) + pagestart) << 9)) & TWOMEGMLIM),
                          (((((mmu_all[cx][segment].sor & 0xfff) + pagestart) << 9))));
                dumpmmupage(cx, segment, buglog);
                fprintf(buglog, "--\n\n");
            }

            if ((uint32)(((sor + pageend) << 9) + 511) > mymaxlisaram)
            {
                DEBUG_LOG(0, "**** cx:%d pc24=%08x seg:%d END RANGE OVER MAXLISARAM (%08x) sor:%04x pageend:%d!!!!!! chopped to 2mb it's:%08x, unchopped:%08x****\n",
                          cx, pc24, segment, mymaxlisaram, mmu_all[cx][segment].sor, pageend,
                          (((((mmu_all[cx][segment].sor & 0xfff) + pageend) << 9) + 511) & TWOMEGMLIM),
                          (((((mmu_all[cx][segment].sor & 0xfff) + pageend) << 9) + 511)));
                dumpmmupage(cx, segment, buglog);
                DEBUG_LOG(0, "--\n\n");
            }
        }
    }
    else
    {
        for (i = 0; i < 256; i++) // All others handled here
        {
            lisa_mem_t mrfn, mwfn;
            int32 mxea;

            mt = &mmu_trans_all[cx][segment8 + i];
            if (!mt)
            {
                EXIT(2, 0, "mt is null!");
            }

            page = i;
            epage = segment8 + i;

            if (i >= pagestart && i <= pageend)
            {
                xrfn = rfn;
                xwfn = wfn;
                xea = ea;
            }
            else
            {
                xrfn = bad_page;
                xwfn = bad_page;
                xea = 0;
            }

            mrfn = mt->readfn;
            mwfn = mt->writefn;
            mxea = mt->address;

            if (rfn == sio_mmu && mrfn == sio_mmu)
            {
                mrfn = sio_mmu;
                mxea = xea;
            }
            if (wfn == sio_mmu && mwfn == sio_mmu)
            {
                mrfn = sio_mmu;
                mxea = xea;
            }
            if (rfn == sio_mmu && mrfn == sio_rom)
            {
                mrfn = sio_mmu;
                mxea = xea;
            }
            if (wfn == sio_mmu && mwfn == sio_rom)
            {
                mwfn = sio_mmu;
                mxea = xea;
            }
            if (rfn == sio_mmu && mrfn == sio_mrg)
            {
                mrfn = sio_mmu;
                mxea = xea;
            }
            if (wfn == sio_mmu && mwfn == sio_mrg)
            {
                mwfn = sio_mmu;
                mxea = xea;
            }

            if (mxea != xea || mrfn != xrfn || mwfn != xwfn)
            { // debug_log_enabled=1;
                DEBUG_LOG(0, "1 mmu validation failed %d::%d/%d/%d seg:%02x(%d) page:%i epage:0x%04x(%d), slr:%s:sor:%04x\n s/b %ld/%08x,%d/%d(%s/%s) is: %ld/%08x,%d/%d(%s/%s) pages:%d-%d '%s'",
                          context, segment1, segment2, start,
                          segment, segment, page, epage, epage, slrname(mmu_all[cx][segment].slr), mmu_all[cx][segment].sor,
                          xea, xea, xrfn, xwfn,
                          mspace(xrfn), mspace(xwfn),
                          mt->address, mt->address, mt->readfn, mt->writefn,
                          mspace(mt->readfn), mspace(mt->writefn),
                          pagestart, pageend, from);
                mmu_seg_error = 1;
                dumpmmupage(cx, segment, buglog);
            }
        } // for loop
    }     // if/else ram

} //////// end of fill_segment /////////////////////////////////////////////////////////////////////////////////////////////////

void validate_mmu_segments(char *from)
{
    // uint32 page=0;             // page #
    int32 ea;        //  epage,        // page and effective page (sor+/-page), and diff address ea
    uint16 sor, slr; // mmu regs
    uint8 segment;
    uint32 segment8; // segment shifted 8 bits over into place
    // mmu_trans_t *mt=NULL;
    lisa_mem_t rfn = bad_page, wfn = bad_page;
    int cx = CXSEL;
    int16 pagestart, pageend;

    //   #ifdef DEBUG
    //   char s1[160], s2[160];
    //   #endif

    GET_MMU_DIRTY(0);
    if (start || mmudirty)
    {
        DEBUG_LOG(250, "fskipping start=%d || mmudirty=%d from :%s", start, mmudirty, from);
        return;
    } // ignore changes to context zero.

    mmu_seg_error = 0;

    for (segment = 0; segment < 128; segment++)
    {
        sor = mmu_all[cx][segment].sor & 0xfff;
        slr = mmu_all[cx][segment].slr;
        segment8 = segment << 8;
        ea = (int64)(-((int32)(segment << 17)) + ((int32)(sor << 9))); // 2020.11.21

        get_slr_page_range(context, segment, &pagestart, &pageend, &rfn, &wfn);
        check_mmu_segment(segment, ea, rfn, wfn, pagestart, pageend, from);
    }

    /////if (mmu_seg_error) {DEBUG_LOG(10,"\n\nmmu validation failed from %s\n\n\n",from); EXIT(1);}

} //---------------- end of function --------------------------------------------------------------------------

#endif

// Welcome to the Realms of Chaos.  Your nightmare has just begun
// You wake up screaming, but there's no place left to run.  -- Bolt Thrower

//  "...for every complex problem, there is a solution that is simple, neat, and wrong."  -- H.L. Mencken
