/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.07.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2020 Ray A. Arachelian                          *
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
*               AF version of MMU code - Memory and I/O access functions               *
*                                                                                      *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#define INMEMORYDOTC
#include <vars.h>

#ifdef DEBUG

#ifdef CPU_CORE_TESTER
// #include <cpucoretester.c>
extern void record_access(uint32 address, int size, int write, uint32 value);
#endif

void printregs(FILE *buglog, char *tag);
void extprintregs(FILE *buglog, char *tag);
#endif

extern uint32 getreg(uint8 regnum); // 16=pc, 17=sp 0=7Dregs 8-15Aregs;

extern long getsectornum(DC42ImageType *F, uint8 side, uint8 track, uint8 sec);

int check_mmu0_chk(void);

static int slot1h = 0, slot1l = 0;
static int slot2h = 0, slot2l = 0;
static int slot3h = 0, slot3l = 0;

static int parity_error_hit = 0;

#ifdef CHECKODDPOINTERS
#define CHK_R_ODD_ADR(addr)                                        \
    {                                                              \
        if ((addr) & 1)                                            \
        {                                                          \
            DEBUG_LOG(100, "Read of odd address, throwing error"); \
            CPU_READ_MODE = 0;                                     \
            lisa_addrerror((addr));                                \
            return 0;                                              \
        }                                                          \
    }
#define CHK_W_ODD_ADR(addr)                                         \
    {                                                               \
        if ((addr) & 1)                                             \
        {                                                           \
            DEBUG_LOG(100, "Write to odd address, throwing error"); \
            CPU_READ_MODE = 1;                                      \
            lisa_addrerror((addr));                                 \
            return;                                                 \
        }                                                           \
    }
#else
#define CHK_R_ODD_ADR(addr) \
    {                       \
    }
#define CHK_W_ODD_ADR(addr) \
    {                       \
    }
#endif

#ifdef PHYS_UNDER_BUSERR
#define CHK_PHYS_UFLOW(addr)          \
    {                                 \
        if (physaddr == -2)           \
            lisa_mmu_exception(addr); \
    }
#else
#define CHK_PHYS_UFLOW(addr) \
    {                        \
    }
#endif

#ifdef PHYS_OVERFLOW_BUSERR
#define CHK_PHYS_OFLOW(addr)          \
    {                                 \
        if (physaddr == -1)           \
            lisa_mmu_exception(addr); \
    }
#else
#define CHK_PHYS_OFLOW(addr) \
    {                        \
    }
#endif

// forward fn header refs
void lisa_parity_error(uint32 addr);
void lisa_hardmem_error(uint32 addr);
void lisa_softmem_error(uint32 addr);

uint8 lisa_rb_ram_parity(uint32 addr);
uint16 lisa_rw_ram_parity(uint32 addr);
uint32 lisa_rl_ram_parity(uint32 addr);
void lisa_wb_ram_parity(uint32 addr, uint8 data);
void lisa_ww_ram_parity(uint32 addr, uint16 data);
void lisa_wl_ram_parity(uint32 addr, uint32 data);

uint8 lisa_rb_vidram_parity(uint32 addr);
uint16 lisa_rw_vidram_parity(uint32 addr);
uint32 lisa_rl_vidram_parity(uint32 addr);
void lisa_wb_vidram_parity(uint32 addr, uint8 data);
void lisa_ww_vidram_parity(uint32 addr, uint16 data);
void lisa_wl_vidram_parity(uint32 addr, uint32 data);

void lisa_wb_xlvidram_parity(uint32 addr, uint8 data);
void lisa_ww_xlvidram_parity(uint32 addr, uint16 data);
void lisa_wl_xlvidram_parity(uint32 addr, uint32 data);
void lisa_wb_xlvidram(uint32 addr, uint8 data);
void lisa_ww_xlvidram(uint32 addr, uint16 data);
void lisa_wl_xlvidram(uint32 addr, uint32 data);

uint8 lisa_rb_ext_2par_via(viatype *V, uint32 addr);
void lisa_wb_ext_2par_via(viatype *V, uint32 addr, uint8 xvalue);

int get_vianum_from_addr(long addr)
{

    switch (addr & 0x00fffe00)
    {
    case 0x00FCDD00:
        return 1; // printf("cops: ");
    case 0x00fcd800:
        return 2; // printf("motherboard: ");
    case 0x00fcd900:
        return 2; // printf("motherboard: ");
    case 0x00fc2800:
        return 3; // printf("slot 1 high: ");
    case 0x00fc2000:
        return 4; // printf("slot 1 low : ");
    case 0x00fc6800:
        return 5; // printf("slot 2 high: ");
    case 0x00fc6000:
        return 6; // printf("slot 2 low : ");
    case 0x00fca800:
        return 7; // printf("slot 3 high: ");
    case 0x00fca000:
        return 8; // printf("slot 3 low : ");
    default:
        return 0;
    }
}

char slrnm[1024];
char *slrname(uint16 slr)
{
    switch (slr & SLR_MASK)
    {
    case SLR_RO_STK:
        sprintf(slrnm, "ro_stk:%04x", slr);
        break;
    case SLR_RO_MEM:
        sprintf(slrnm, "ro_mem:%04x", slr);
        break;
    case SLR_RW_STK:
        sprintf(slrnm, "rw_stk:%04x", slr);
        break;
    case SLR_RW_MEM:
        sprintf(slrnm, "rw_mem:%04x", slr);
        break;
    case SLR_IO_SPACE:
        sprintf(slrnm, "io_spc:%04x", slr);
        break;
    case SLR_UNUSED_PAGE:
        sprintf(slrnm, "unused:%04x", slr);
        break;
    case SLR_SIO_SPACE:
        sprintf(slrnm, "siospc:%04x", slr);
        break;
    default:
        sprintf(slrnm, "illgal:%04x", slr);
        break;
        DEBUG_LOG(100, "Illegal mode SLR:%04x encountered", slr);
    }

    return slrnm;
}

char *mspace(lisa_mem_t fn)
{
    if (fn < MAX_LISA_MFN)
        return memspaces[fn];
    else
        return "*ERROR*";
}

// print byte normalized into ascii (control chars turned back into letters, chars with high bit set get it stripped //
// unprintables become . (only null or DEL)

static inline uint8 normasc(uint8 a)
{
    a &= 0x7f;
    if (!a)
        a = ' ';
    else if (a == 127)
        a = '.';
    else if (a < 31)
        a += 'A';
    return a;
}

char norma[5]; // storage for normalized ascii

char *ascbyte(uint8 a)
{
    norma[0] = normasc(a);
    norma[1] = 0;
    return norma;
}
char *ascword(uint16 a)
{
    norma[0] = normasc((uint8)(a >> 8));
    norma[1] = normasc((uint8)((a) & 0xff));
    norma[2] = 0;
    return norma;
}
char *asclong(uint32 a)
{
    norma[0] = normasc((uint8)((a >> 24) & 0xff));
    norma[1] = normasc((uint8)((a >> 16) & 0xff));
    norma[2] = normasc((uint8)((a >> 8) & 0xff));
    norma[3] = normasc((uint8)((a) & 0xff));
    norma[4] = 0;
    return norma;
}

lisa_mem_t rmmuslr2fn(uint16 slr, uint32 a9)
{
    lisa_mem_t r = bad_page;

    switch (slr & SLR_MASK)
    {
    case SLR_RO_STK:
        r = ram;
        break; // DEBUG_LOG(100,"0100 read only stack"); break;
    case SLR_RO_MEM:
        r = ram;
        break; // DEBUG_LOG(100,"0101 read only ram  "); break;
    case SLR_RW_STK:
        r = ram;
        break; // DEBUG_LOG(100,"0110 r/w stack      "); break;
    case SLR_RW_MEM:
        r = ram;
        break; // DEBUG_LOG(100,"0111 r/w ram        "); break;
    case SLR_IO_SPACE:
        r = io_map[a9 & 0x7f];
        break; // DEBUG_LOG(100,"1001 slr=%04x a9=%08x -> io",slr,a9); break;
    case SLR_SIO_SPACE:
        r = (a9 & 64) ? sio_mrg : sio_rom;
        break; // DEBUG_LOG(100,"1111 slr=%04x a9=%08x sio mrg or rom? (%d ? mrg:rom)",slr,a9,a9 & 64); break;
    }

    return r;
}

lisa_mem_t wmmuslr2fn(uint16 slr, uint32 a9)
{
    lisa_mem_t w = bad_page;

    switch (slr & SLR_MASK)
    {
    case SLR_RO_STK:
        w = bad_page;
        break; // DEBUG_LOG(100,"0100 read only stack"); break;
    case SLR_RO_MEM:
        w = bad_page;
        break; // DEBUG_LOG(100,"0101 read only ram  "); break;
    case SLR_RW_STK:
        w = ram;
        break; // DEBUG_LOG(100,"0110 r/w stack      "); break;
    case SLR_RW_MEM:
        w = ram;
        break; // DEBUG_LOG(100,"0111 r/w ram        "); break;
    case SLR_IO_SPACE:
        w = io_map[a9 & 0x7f];
        break; // DEBUG_LOG(100,"1001 slr=%04x a9=%08x -> io",slr,a9); break; // should be this one.
    case SLR_SIO_SPACE:
        w = (a9 & 64) ? sio_mrg : sio_rom;
        break; // DEBUG_LOG(100,"1111 slr=%04x a9=%08x sio mrg or rom? (%d ? mrg:rom)",slr,a9,a9 & 64); break;
    }

    return w;
}

// Memory fetch fn's that are safe from MMU/address error exceptions.
// allows our emulator to peek into the Lisa's brains without causing
// side effects. Macro below contains code common to all three functions,
// so in order to help simplify debugging, it's a macro.

void lisa_ram_safe_setbyte(uint8 context, uint32 address, uint8 data)
{
    CHK_RAM_A_LIMITS(context, address);
    if (physaddr < 0)
        return;
    lisaram[physaddr] = data;
}

void lisa_ram_safe_setword(uint8 context, uint32 address, uint16 data)
{
    CHK_RAM_A_LIMITS(context, address);
    if (physaddr < 0)
        return;
    lisaram[physaddr] = (data >> 8) & 0x00ff;
    lisaram[physaddr + 1] = (data) & 0x00ff;
}

void lisa_ram_safe_setlong(uint8 context, uint32 address, uint32 data)
{
    CHK_RAM_A_LIMITS(context, address);
    if (physaddr < 0)
        return;
    lisaram[physaddr] = (data >> 24) & 0x00ff;
    lisaram[physaddr + 1] = (data >> 16) & 0x00ff;
    lisaram[physaddr + 2] = (data >> 8) & 0x00ff;
    lisaram[physaddr + 1] = (data) & 0x00ff;
}

uint8 lisa_ram_safe_getbyte(uint8 context, uint32 address)
{
    CHK_RAM_A_LIMITS(context, address);
    if (physaddr < 0)
        return 0xaf;
    return lisaram[physaddr];
}

uint16 lisa_ram_safe_getword(uint8 context, uint32 address)
{

    CHK_RAM_A_LIMITS(context, address);
    if (physaddr > -1)
        return LOCENDIAN16(*(uint16 *)(&lisaram[physaddr]));
    return 0xaf;
}

uint32 lisa_ram_safe_getlong(uint8 context, uint32 address)
{
    CHK_RAM_A_LIMITS(context, address);
    if (physaddr > -1)
        return LOCENDIAN32(*(uint32 *)(&lisaram[physaddr]));
    return 0xaf;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Debug versions of function dispatchers.  These are replaced with macros in production code.  These funcs are
// just wrappers so they can spit out what memory access have occured, and also test MMU translation table validity
// as compared to Lisa's MMU registers.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUGMEMCALLS
char schkmt[1024];

int collapse_mem_fn(int i) // collapse to general fn type to make mmu_trans to mmu type checking possible
{

    switch (i)
    {
    case OxERROR:
        return OxERROR; // 0      /* This should never be used - it indicates a bug in our code */
    case OxUnused:
        return OxUnused; // 1     /* unused I/O space address (only used in I/O space map)      */

    case Ox0000_slot1:     // 2
    case Ox2000_slot1:     // 3
    case Ox4000_slot2:     // 4
    case Ox6000_slot2:     // 5
    case Ox8000_slot3:     // 6
    case Oxa000_slot3:     // 7
    case Oxc000_flopmem:   // 8
    case Oxd200_sccz8530:  // 10
    case Oxd800_par_via2:  // 11
    case Oxdc00_cops_via1: // 12
    case Oxe000_latches:   // 13
    case Oxe800_videlatch: // 14
    case Oxf000_memerror:  // 15
    case Oxf800_statreg:
        return io; // 16

    // Real Lisa memory
    case ram:
        return ram; // 17           /* Plain old RAM, or stack access.                                   */
    case vidram:
        return ram; //  18          /* same as ram, but flag on write that screen needs refreshing       */
    case ro_violn:
        return ro_violn; //  19     /* Read only violation - what trap should I call? See schematic      */
    case bad_page:
        return bad_page; //  20     /* Bad page or unallocated segment - what trap here?                 */

    // Special I/O space
    case sio_rom:
        return sio_mmu; //  21      /* access to ROM via sio mode                                        */
    case sio_mrg:
        return sio_mmu; //  22      /* mmu register being accessed.  Which depends on bit 3 of addr      */

    case sio_mmu:
        return sio_mmu; //  23      /* access ram or other spaces via the mmu (bit14=1 in address)       */

    // Disparcher to I/O space (dispatcher to the Ox????_ fn's list above)
    case io:
        return io; //  24           /* This is a dispatcher for I/O space when we don't know the address */
    }

    return 0;
}

char *chk_mtmmu(uint32 a, uint8 write)
{
    uint16 fn, slr, sor, mfn;
    uint32 a9, a17, mad, mtd, filter;

    if (!context)
    {
        *schkmt = 0;
        return schkmt;
    }

    a9 = GETEPAGE(a);
    a17 = GETSEG(a);

    slr = mmu_all[context][a17].slr;
    sor = mmu_all[context][a17].sor;

    mfn = write ? wmmuslr2fn(slr, (a & 0x00ffffff) >> 9) : rmmuslr2fn(slr, a9);
    fn = write ? mmu_trans_all[context][a9].writefn : mmu_trans_all[context][a9].readfn;

    mad = (((sor & 0x0fff) << 9) + (a & MMUXXFILT)) & TWOMEGMLIM;

    mtd = CHK_MMU_TRANS(a);

    filter = 0;

    // ea offset comparison filter
    if (collapse_mem_fn(mfn) == sio_rom)
        filter = 0x3fff;
    if (collapse_mem_fn(mfn) == ram)
        filter = TWOMEGMLIM;
    if ((collapse_mem_fn(mfn) == sio_mmu && collapse_mem_fn(fn) == ram) || (collapse_mem_fn(fn) == sio_mmu && collapse_mem_fn(mfn) == ram))
        filter = 0;

    snprintf(schkmt, 1024, "\nchkmtmmu: %s:%d/%d {mmu/mt:%08x/%08x %s},   mmu_t[%d][%04x].fn=%d %s   mmu[%ld].fn=%d %s, mmudirty:%x",
             (mfn == fn ? "(ok)" : "(*MMU/t fn diff!*)"), mfn, fn,
             mad, mtd, ((filter & (collapse_mem_fn(mad) != collapse_mem_fn(mtd))) ? "*MISMATCH*" : "ok"),
             context, a9, fn, mspace(fn),
             a17, mfn, mspace(mfn),
             mmudirty);

    if (mmu_trans_all[context][a9].writefn < 2 || mmu_trans_all[context][a9].readfn < 2)
    {
        EXITR(304, NULL, "*BUG*: mmu_trans_all[%d][%04x].writefn=%d,readfn=%d",
              context, a9, mmu_trans_all[context][a9].writefn, mmu_trans_all[context][a9].readfn);
    }

    if (filter & (mad ^ mtd))
        validate_mmu_segments(schkmt);

    return schkmt;
}

uint32 get_mmu_roff(uint8 con, uint32 a)
{
    uint16 slr, sor; //, mfn;
    uint32 a17 = GETSEG(a);

    slr = mmu_all[con][a17].slr;
    sor = mmu_all[con][a17].sor;
    // mfn=(write ? wmmuslr2fn(slr, ((a) & 0x00ffffff)>>9) : rmmuslr2fn(slr, ((a) & 0x00ffffff)>>9) );
    return (((sor & 0x0fff) << 9) + (a & 0x00ffffff)) & TWOMEGMLIM;

    // if (mfn!=ram) {DEBUG_LOG(100, "get_mmu_ram called with non-ram!"); EXIT(1);}
    // return mad & TWOMEGMLIM;
}

void *dmem68k_memptr(char *file, char *function, int line, uint32 addr)
{
    void *ret;
    HIGH_BYTE_FILTER();
#ifdef DEBUGMEMCHKMMU
    DEBUG_LOG(1, "\n%s:%s:%d: READ POINTER TO @ %d/%08x:: %s", file, function, line, context, addr, chk_mtmmu(addr, 0));
#endif
    if (mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn < 2)
    {
        EXITR(305, NULL, "MMU_T[%04x].rfn is %d!", ((addr)&MMUEPAGEFL) >> 9, mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn);
    }

    ret = (void *)mem68k_memptr[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn](addr);
    MEMDEBUG_LOG(1, "fn returned pointer to %p\n", (void *)(ret));
    return ret;
}

uint32 get_physaddr(uint32 addr)
{
    CHK_RAM_LIMITS(addr);
    return physaddr;
}

uint8 dmem68k_fetch_byte(char *file, char *function, int line, uint32 addr)
{
    uint8 ret;
    HIGH_BYTE_FILTER();
    uint32 a = addr;

#ifdef DEBUGMEMCHKMMU
    MEMDEBUG_LOG(100, "%s:%s:%d: READ BYTE @ %d/%08x:: %s", file, function, line, context, addr, chk_mtmmu(a, 0));
#endif
    int rfn = mmu_trans[((a)&MMUEPAGEFL) >> 9].readfn;
    if (rfn < 2)
    {
        EXITR(306, 0, "MMU_T[%04x].rfn is %d!", ((addr)&MMUEPAGEFL) >> 9, rfn);
    }

    ret = mem68k_fetch_byte[rfn](a);
    MEMDEBUG_LOG(100, ":::::READ BYTE %02x '%s' from @%d/%08x (phys @%08x) using %s", ret, ascbyte(ret), context, a, get_physaddr(addr), memspaces[rfn]);

#ifdef DEBUG
    if (abort_opcode == 1)
        MEMDEBUG_LOG(100, ":::::Got abort_opcode=1 for READ BYTE %02x '%s' from @%d/%08x (@%08x)", ret, ascbyte(ret), context, a, addr);

#endif

#ifdef CPU_CORE_TESTER
    record_access(addr, 1, 0, (uint32)ret);
#endif

    return ret;
}

uint16 dmem68k_fetch_word(char *file, char *function, int line, uint32 addr)
{
    HIGH_BYTE_FILTER();
    uint32 a = addr;
    uint16 ret;
#ifdef DEBUGMEMCHKMMU
    MEMDEBUG_LOG(100, "\n%s:%s:%d: READ WORD @ %d/%08x:: %s", file, function, line, context, a, chk_mtmmu(a, 0));
#endif
    int rfn = mmu_trans[((a)&MMUEPAGEFL) >> 9].readfn;
    if (rfn < 2)
    {
        EXITR(307, 0, "MMU_T[%04x].rfn is %d!", ((a)&MMUEPAGEFL) >> 9, rfn);
    }
    ret = mem68k_fetch_word[rfn](a);
    MEMDEBUG_LOG(100, ":::::READ WORD %04x '%s' from @%d/%08x (phys @%08x) using %s", ret, ascword(ret), context, a, get_physaddr(addr), memspaces[rfn]);

#ifdef DEBUG
    if (abort_opcode == 1)
        DEBUG_LOG(100, ":::::GOT ABORT_OPCODE! READ WORD %04x '%s' from @%d/%08x (@%08x)", ret, ascword(ret), context, a, addr);
#endif

#ifdef CPU_CORE_TESTER
    //    void record_access(uint32 address, int size, int write, uint32 prewriteval, uint32 value)
    record_access(addr, 2, 0, (uint32)ret);
#endif

    return ret;
}

uint32 dmem68k_fetch_long(char *file, char *function, int line, uint32 addr)
{
    HIGH_BYTE_FILTER();
    uint32 a = addr;
    uint32 ret;

#ifdef DEBUGMEMCHKMMU
    MEMDEBUG_LOG(100, "\n%s:%s:%d: REAd LONG @ %d/%08x:: %s", file, function, line, context, a, chk_mtmmu(a, 0));
#endif
    int rfn = mmu_trans[((a)&MMUEPAGEFL) >> 9].readfn;
    if (rfn < 2)
    {
        EXITR(308, 0, "MMU_T[%04x].rfn is %d!", ((a)&MMUEPAGEFL) >> 9, rfn);
    }

    ret = mem68k_fetch_long[rfn](a);
    MEMDEBUG_LOG(100, ":::::READ LONG %08x '%s' from @%d/%08x (phys @%08x) using %s", ret, asclong(ret), context, a, get_physaddr(addr), memspaces[rfn]);

#ifdef DEBUG
    if (abort_opcode == 1)
        DEBUG_LOG(100, ":::::GOT ABORT_OPCODE! READ LONG %08x '%s' from @%d/%08x (@%08x)", ret, asclong(ret), context, a, addr);
#endif

#ifdef CPU_CORE_TESTER
    //    void record_access(uint32 address, int size, int write, uint32 prewriteval, uint32 value)
    record_access(addr, 3, 0, ret);
#endif

    return ret;
}

void dmem68k_store_byte(char *file, char *function, int line, uint32 addr, uint8 d)
{
    HIGH_BYTE_FILTER();
    uint32 a = addr;
#ifdef DEBUGMEMCHKMMU
    MEMDEBUG_LOG(100, "\n%s:%s:%d: WRITE BYTE %02x to @ %d/%08x:: %s", file, function, line, d, context, a, chk_mtmmu(a, 1));
#endif
    int wfn = mmu_trans[((a)&MMUEPAGEFL) >> 9].writefn;
    if (wfn < 2)
    {
        EXIT(309, 0, "MMU_T[%04x].wfn is %d!", ((a)&MMUEPAGEFL) >> 9, wfn);
    }

    MEMDEBUG_LOG(0, ":::::WRITE BYTE %02x '%s' to @%d/%08x using %s", d, ascbyte(d), context, a, memspaces[wfn]);
    mem68k_store_byte[wfn](a, d);

#ifdef DEBUG
    if (abort_opcode == 1)
        DEBUG_LOG(100, ":::::GOT ABORT_OPCODE! :::::WRITE BYTE %02x '%s' to @%d/%08x (phys @%08x) using %s", d, ascbyte(d), context, a, get_physaddr(addr), memspaces[wfn]);
#endif
#ifdef CPU_CORE_TESTER
    //    void record_access(uint32 address, int size, int write, uint32 prewriteval, uint32 value)
    record_access(addr, 1, 1, (uint32)d);
#endif
}

void dmem68k_store_word(char *file, char *function, int line, uint32 addr, uint16 d)
{
    HIGH_BYTE_FILTER();
    uint32 a = addr;
#ifdef DEBUGMEMCHKMMU
    MEMDEBUG_LOG(100, "\n%s:%s:%d: WRITE WORD %04x to @ %d/%08x:: %s", file, function, line, d, context, a, chk_mtmmu(a, 1));
#endif
    int wfn = mmu_trans[((a)&MMUEPAGEFL) >> 9].writefn;
    if (wfn < 2)
    {
        EXIT(309, 0, "MMU_T[%04x].wfn is %d!", ((a)&MMUEPAGEFL) >> 9, wfn);
    }

    MEMDEBUG_LOG(100, ":::::WRITE WORD %04x '%s' to @%d/%08x using %s", d, ascword(d), context, a, memspaces[wfn]);
    mem68k_store_word[wfn](a, d);

#ifdef DEBUG
    if (abort_opcode == 1)
        DEBUG_LOG(100, ":::::GOT ABORT_OPCODE! WRITE WORD %04x '%s' to @%d/%08x (phys @%08x)using %s", d, ascword(d), context, a, get_physaddr(addr), memspaces[wfn]);
#endif

#ifdef CPU_CORE_TESTER
    //    void record_access(uint32 address, int size, int write, uint32 prewriteval, uint32 value)
    record_access(addr, 2, 1, (uint32)d);
#endif
}

void dmem68k_store_long(char *file, char *function, int line, uint32 addr, uint32 d)
{
    HIGH_BYTE_FILTER();
    uint32 a = addr;

    int wfn = mmu_trans[((a)&MMUEPAGEFL) >> 9].writefn;
    if (wfn < 2)
    {
        EXIT(309, 0, "MMU_T[%04x].wfn is %d!", ((a)&MMUEPAGEFL) >> 9, wfn);
    }

    MEMDEBUG_LOG(100, ":::::WRITE LONG %08x '%s' to @%d/%08x %s (phys @%08x) using %s", d, asclong(d), context, a, ((a < 0x80) ? getvector(a / 4) : ""), get_physaddr(addr),
                 memspaces[wfn]);

    mem68k_store_long[wfn](a, d);

#ifdef DEBUG
    if (abort_opcode == 1)
        DEBUG_LOG(100, ":::::GOT ABORT_OPCODE! WRITE LONG %08x '%s' to @%d/%08x %s using %s", d, asclong(d), context, a, ((a < 0x80) ? getvector(a / 4) : ""), memspaces[wfn]);
#endif

#ifdef CPU_CORE_TESTER
    //    void record_access(uint32 address, int size, int write, uint32 prewriteval, uint32 value)
    record_access(addr, 3, 1, d);
#endif
}
#endif
///////////////

/*
void fixmmu(uint32 addr)  ///////////////// junk?
{
    int i;

    //uint32 a9 =GETEPAGE(addr);
    uint32 a17=GETSEG(addr);

    DEBUG_LOG(1,"INVALID MMU IS: con=%d, mmu[%d].sor=%04x slr=%04x changed=%d",context,a17,
        mmu[a17].sor,mmu[a17].slr,mmu[a17].changed);

    DEBUG_LOG(1,"INVALID TRANSLATION IS: con=%d, mmu_trans[%d].address=%08x rfn=%d/wfn=%d",context,a17,
        mmu_trans[a17].address,mmu_trans[a17].readfn,mmu_trans[a17].writefn);

    for (i=0; i<128; i++)
    {
        DEBUG_LOG(1,"mmu[%d].sor=%04x slr=%04x changed=%d",i,
            mmu[i].sor,mmu[i].slr,mmu[i].changed);
    }
    EXIT(312);
}
*/

void activate_parity_check(void)
{

    lisa_diag2_on_mem(); // hook memory function
    if (mem_parity_bits2)
        return; // already allocated, don't do it again

    DEBUG_LOG(100, "allocating %d bits", (2 + (maxlisaram >> 3))); // 1 parity bit for every 8 bits
    mem_parity_bits2 = calloc(1, 2 + (maxlisaram >> 3));
    if (!mem_parity_bits2)
    {
        EXIT(313, 0, "Couldn't allocate %d bytes of memory for parity check.", (2 + (maxlisaram >> 3)));
    }

    memset(mem_parity_bits2, 0, maxlisaram >> 3);
}

int release_parity_check(void)
{
    if (!mem_parity_bits2)
        free(mem_parity_bits2); // can't free NULL. :)
    mem_parity_bits2 = NULL;

    lisa_diag2_off_mem(); // restore memory functions
    return 0;             // always return 0!!!
}

int parity_check(uint32 address)
{
    uint32 retval, i, size, sum, *parity32;

    // -1 means that it was a hardmem test (write to hardmem 2x)  if abort_opcode - then this is from the NMI or opcode decoder
    // so ignore parity issues.

    if (parity_error_hit == -1 || abort_opcode == 2)
    {
        DEBUG_LOG(100, "skipping test:: parity_error_hit=%d or abort_opcode=2 (is %d) not looking at parity memory - harderr2x",
                  parity_error_hit, abort_opcode);
        return 0;
    }

    if (mem_parity_bits2 == NULL)
    {
        DEBUG_LOG(100, "No parity bits allocated, returning 0\n");
        return 0;
    }
    if (address > maxlisaram)
    {
        DEBUG_LOG(100, "Parity check address out of bounds! %08x > %08x\n", address, maxlisaram);
        return 0;
    }

    DEBUG_LOG(100, "will be checking parity for %08x mem_parity_bits2[%08x]=%08x & %02x results: %02x\n", address, (address >> 3), mem_parity_bits2[address >> 3], (1 << (address & 7)), (mem_parity_bits2[address >> 3] & (1 << (address & 7))));

    if (!hardmem)
    {
        DEBUG_LOG(100, "hardmem isn't set, so I'm not checking it\n");
        return 0;
    }

    retval = (mem_parity_bits2[address >> 3] & (1 << (address & 7)));

    if (!diag2) // If diag2 mode is off, we might be able to free the parity array and save 256K
    {
        mem_parity_bits2[address >> 3] &= (0xff ^ (1 << (address & 7))); // clear the bad bit, it was already read

        //-----can we free the parity array yet?--------------------------------------------------------------------------------------
        parity32 = (uint32 *)(mem_parity_bits2); // do these 32 bits wide at a time, es muy rapido!
        size = (maxlisaram >> 5);                // div by 8 ,for 8 bits/byte, div by 4 for 4 bytes/uint32
        for (sum = 0, i = 0; i < size && !sum; sum |= parity32[i++])
            ; // check the bits - if any are set, we keep the mem_parity array

        ALERT_LOG(0, "binary or sum of parity set bits is:%d, if zero, releasing parity memory", sum);

        if (!sum)
            release_parity_check(); // if no mem_parity bits, we can free up some memory.
                                    //--------------------------------------------------------------------------------------------------------------------
    }
    else
    {
        ALERT_LOG(0, "DIAG2 is still set, I didn't try to release the parity ram");
    }

#ifdef DEBUG
    if (mem_parity_bits2)
        DEBUG_LOG(100, "checking mem_parity_bits2[%08x]=%08x & %02x results: %02x",
                  (address >> 3),
                  mem_parity_bits2[address >> 3],
                  (1 << (address & 7)),
                  (mem_parity_bits2[address >> 3] & (1 << (address & 7))));
#endif

    parity_error_hit |= (retval != 0);

    DEBUG_LOG(100, "parity_error_hit is now:%d", retval);

    return retval; // return the parity check back
}

void set_parity_check(uint32 address)
{
    // uint32 size,i;

    if (address > maxlisaram)
    {
        DEBUG_LOG(100, "Parity check address out of bounds! %08x>%08x", address, maxlisaram);
        return;
    }

    if (mem_parity_bits2 == NULL)
    {
        DEBUG_LOG(100, "No parity bits allocated, activating parity check...");
        activate_parity_check();
#ifdef DEBUG
        if (mem_parity_bits2 == NULL)
        {
            EXIT(314, 0, "Parity bits activation failure even after retry.");
        }
#endif
    }

    DEBUG_LOG(100, "Setting mem_parity_bits2[%08x]=%08x | %02x results: %02x", (address >> 3), mem_parity_bits2[address >> 3], (1 << (address & 7)), (mem_parity_bits2[address >> 3] | (1 << (address & 7))));
    mem_parity_bits2[address >> 3] |= 1 << (address & 7);

    return;
}

int getsnbit(void)
{
    int bit = 0;
    static uint32 lastaddr;

    bit = (serialnum240[serialnumshiftcount >> 3]) & (1 << ((serialnumshiftcount & 7) ^ 7));

    // "DRM is not going to stop file sharing. They're trying to catch smoke with nets." -- stuart@realhappy.net

    serialnumshiftcount = (serialnumshiftcount + 1) & 127;

    if (pc24 != lastaddr)
        ALERT_LOG(0, "Possible read of serial number from %d/%08x", context, pc24);
    lastaddr = pc24;
    return (bit ? 32768 : 0);
}

int peeksnbit(void) { return (((serialnum240[serialnumshiftcount >> 3]) & (1 << ((serialnumshiftcount & 7) ^ 7))) ? 32768 : 0); }

uint8 *lisa_mptr_OxERROR(uint32 addr)
{
    EXITR(321, ((uint8 *)(NULL)), "Something's wrong with the MMU - bad mapping at %8x****\n", addr);
}

uint8 lisa_rb_OxERROR(uint32 addr)
{
    EXITR(321, 0, "Something's wrong with the MMU - bad mapping at %8x****\n", addr);
}

uint16 lisa_rw_OxERROR(uint32 addr)
{
    EXITR(321, 0, "Something's wrong with the MMU - bad mapping at %8x****\n", addr);
}

uint32 lisa_rl_OxERROR(uint32 addr)
{
    EXITR(321, 0, "Something's wrong with the MMU - bad mapping at %8x****\n", addr);
}

void lisa_wb_OxERROR(uint32 addr, uint8 data)
{
    UNUSED(data);
    EXIT(321, 0, "Something's wrong with the MMU - bad mapping at %8x****\n", addr);
}

void lisa_ww_OxERROR(uint32 addr, uint16 data)
{
    UNUSED(data);
    EXIT(321, 0, "Something's wrong with the MMU - bad mapping at %8x****\n", addr);
}

void lisa_wl_OxERROR(uint32 addr, uint32 data)
{
    UNUSED(data);
    EXIT(321, 0, "Something's wrong with the MMU - bad mapping at %8x****\n", addr);
}

uint8 *lisa_mptr_OxUnused(uint32 addr)
{
    EXITR(322, NULL, "Something's wrong with the MMU - bad mapping at %d/%08x**** %s\n", context, addr, memspaces[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn]);
}

uint8 lisa_rb_OxUnused(uint32 addr)
{
    EXITR(322, 0, "Something's wrong with the MMU - bad mapping at %d/%08x**** %s\n", context, addr, memspaces[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn]);
}

uint16 lisa_rw_OxUnused(uint32 addr)
{
    EXITR(322, 0, "Something's wrong with the MMU - bad mapping at %d/%08x**** %s\n", context, addr, memspaces[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn]);
}

uint32 lisa_rl_OxUnused(uint32 addr)
{
    EXITR(322, 0, "Something's wrong with the MMU - bad mapping at %d/%08x**** %s\n", context, addr, memspaces[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn]);
}

void lisa_wb_OxUnused(uint32 addr, uint8 data)
{
    UNUSED(data);
    EXIT(322, 0, "Something's wrong with the MMU - bad mapping at %d/%08x**** %s\n", context, addr, memspaces[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn]);
}

void lisa_ww_OxUnused(uint32 addr, uint16 data)
{
    UNUSED(data);
    EXIT(322, 0, "Something's wrong with the MMU - bad mapping at %d/%08x**** %s\n", context, addr, memspaces[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn]);
}

void lisa_wl_OxUnused(uint32 addr, uint32 data)
{
    UNUSED(data);
    EXIT(322, 0, "Something's wrong with the MMU - bad mapping at %d/%08x**** %s\n", context, addr, memspaces[mmu_trans[((addr)&MMUEPAGEFL) >> 9].readfn]);
}

/*  For now we're not using the expansion port slots.  When we do, these stub
    functions will be replaced by use of pointers with those that emulate the
    hardware cards such as parallel cards, etc. */

uint8 *lisa_mptr_Ox0000_slot1(uint32 addr)
{
    // ui_log_verbose("***** lisa_mptr_Ox0000_slot1l got called! ******");
    CHECK_DIRTY_MMU(addr);

    if (!slot1l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }

    return NULL;
}

uint8 lisa_rb_Ox0000_slot1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "access to %08x from %08x returning 0", addr, reg68k_pc);
    if (!slot1l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0;
}

uint16 lisa_rw_Ox0000_slot1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "access to %08x from %08x", addr, reg68k_pc);
    if (!slot1l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);

    return 0;
}

uint32 lisa_rl_Ox0000_slot1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "access to %08x from %08x", addr, reg68k_pc);
    if (!slot1l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);

    return 0;
}

void lisa_wb_Ox0000_slot1(uint32 addr, uint8 data)
{
    UNUSED(data);
    UNUSED(addr);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot1l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_ww_Ox0000_slot1(uint32 addr, uint16 data)
{
    UNUSED(data);
    UNUSED(addr);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot1l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_wl_Ox0000_slot1(uint32 addr, uint32 data)
{
    UNUSED(data);
    UNUSED(addr);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot1l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

uint8 *lisa_mptr_Ox2000_slot1(uint32 addr)
{
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    // ui_log_verbose("****lisa_mptr_Ox2000_slot1h got called!****");
    if (!slot1h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);

    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return NULL;
}

uint8 lisa_rb_Ox2000_slot1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    if (!slot1h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xff;
}

uint16 lisa_rw_Ox2000_slot1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    if (!slot1h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xffff;
}

uint32 lisa_rl_Ox2000_slot1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    if (!slot1h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xffffffff;
}

void lisa_wb_Ox2000_slot1(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot1h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_ww_Ox2000_slot1(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot1h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_wl_Ox2000_slot1(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot1h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

uint8 *lisa_mptr_Ox4000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
    if (!slot2l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return NULL;
}

uint8 lisa_rb_Ox4000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x returning 0xff", reg68k_pc, addr);
    if (!slot2l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xff;
}

uint16 lisa_rw_Ox4000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
    if (!slot2l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xffff;
}

uint32 lisa_rl_Ox4000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
    if (!slot2l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xffffffff;
}

void lisa_wb_Ox4000_slot2(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "@%08x", addr);
    // ui_log_verbose("lisa_mptr_Ox2000_slot2l got called!");
    if (!slot2l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_ww_Ox4000_slot2(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot2l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_wl_Ox4000_slot2(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot2l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

uint8 *lisa_mptr_Ox6000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
    if (!slot2h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    // ui_log_verbose("****lisa_mptr_Ox6000_slot2h got called!****");
    return NULL;
}

uint8 lisa_rb_Ox6000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
    if (!slot2h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xff;
}

uint16 lisa_rw_Ox6000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
    if (!slot2h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xffff;
}

uint32 lisa_rl_Ox6000_slot2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
    if (!slot2h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
    return 0xffffffff;
}

void lisa_wb_Ox6000_slot2(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot2h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_ww_Ox6000_slot2(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot2h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

void lisa_wl_Ox6000_slot2(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "access from %08x to address %08x", reg68k_pc, addr);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot2h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    else
        ALERT_LOG(0, "Unused access @%08x", addr);
#endif
    return;
}

// Dual Parallel Card

uint8 *lisa_mptr_2x_parallel_l(uint32 addr)
{
    // 0xFC0001 slot 1
    // 0xFC4001 slot 2
    // 0xFC8001 slot 3  // every VIA register is 8 bytes from the other - exactly the same as on the motherboard parallel port

    addr &= 0x00003fff;
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    if (addr < 0x1fff)
    {
        addr = addr >> 1;
        return &(dualparallelrom[addr & 0x0fff]);
    }
    return NULL;
}

uint8 lisa_rb_2x_parallel_l(uint32 addr)
{
#ifdef DEBUG
    uint32 oa = addr;
#endif

    int vianum = get_vianum_from_addr(addr); // int vianum=(  (((addr>>0xe)&3)<<1) | (((addr>>0xb)&1)^1)  )+ 3;
    //    ALERT_LOG(0,"access from %08x to address",reg68k_pc,addr);

    addr &= 0x00000fff;
    addr = addr >> 1;

    if (addr < 0x1000)
    {
        // if we're not boot-ROMless, but the dual parallel card is romless, and the Lisa POST is probing, lie so
        // that no garbage icons show  up.  Later will need to zap the memory address for the slots before booting.
        if (!romless && dualparallelrom[0x30] == 0xff && dualparallelrom[0x31] == 0xff)
        {
            if ((pc24 & 0x00ff0000) == 0x00fe0000 && addr == 0)
                return 0; // lie to the ROM and hide the dual parallel card
        }
        return (dualparallelrom[addr & 0x0fff]);
    }
    return 0xff;
}

uint16 lisa_rw_2x_parallel_l(uint32 addr)
{
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    return (uint16)(lisa_rb_2x_parallel_l(addr) << 8) + (uint16)(lisa_rb_2x_parallel_l(addr + 1));
}

uint32 lisa_rl_2x_parallel_l(uint32 addr)
{
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    return (uint32)((uint32)(lisa_rb_2x_parallel_l(addr + 0) << 24) + (uint32)(lisa_rb_2x_parallel_l(addr + 1) << 16) +
                    (uint32)(lisa_rb_2x_parallel_l(addr + 2) << 8) + (uint32)(lisa_rb_2x_parallel_l(addr + 3)));
}

void lisa_wb_2x_parallel_l(uint32 addr, uint8 data)
{
    UNUSED(data);
    uint32 oa = addr;

    int vianum = get_vianum_from_addr(addr);
    // int vianum=(  (((addr>>0xe)&3)<<1) | (((addr>>0xb)&1)^1)  )+ 3;
    addr &= 0x00003fff;
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);

    if (addr < 0x1fff)
    {
        ALERT_LOG(100, "Attempt to write to Parallel Port ROM offset %08x", addr);
        return;
    }

    ALERT_LOG(0, "Weird extport write value %08x to address %08x for VIA #%d slot %d", data, oa, vianum, vianum >> 1);
}

void lisa_ww_2x_parallel_l(uint32 addr, uint16 data)
{
    lisa_wb_2x_parallel_l(addr, (uint8)(data >> 8));
    lisa_wb_2x_parallel_l(addr + 1, (uint8)(data & 0xff));
}

void lisa_wl_2x_parallel_l(uint32 addr, uint32 data)
{
    lisa_wb_2x_parallel_l(addr, (uint8)(data >> 24));
    lisa_wb_2x_parallel_l(addr + 1, (uint8)(data >> 16));
    lisa_wb_2x_parallel_l(addr + 2, (uint8)(data >> 8));
    lisa_wb_2x_parallel_l(addr + 3, (uint8)(data & 0xff));
}

uint8 *lisa_mptr_2x_parallel_h(uint32 addr)
{
    // int vianum=(  (((addr>>0xe)&3)<<1) | (((addr>>0xb)&1)^1)  )+ 3;
    int vianum = get_vianum_from_addr(addr);

    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);
    addr &= 0x00003fff;
    if (addr < 0x1fff)
    {
        addr = addr >> 1;
        return &(dualparallelrom[addr & 0x0fff]);
    }
    return NULL;
}

uint8 lisa_rb_2x_parallel_h(uint32 addr)
{
    // 0xFC0001 slot 1
    // 0xFC4001 slot 2
    // 0xFC8001 slot 3  // every VIA register is 8 bytes from the other - exactly the same as on the motherboard parallel port

    // int vianum=(  (((addr>>0xe)&3)<<1) | (((addr>>0xb)&1)^1)  )+ 3;
    int vianum = get_vianum_from_addr(addr);

    uint32 oa = addr;
    addr &= 0x00003fff;
    ALERT_LOG(0, "access from %08x to address", reg68k_pc, addr);

    if (addr & 1)
    {
        ALERT_LOG(100, "Access to lower VIA #%d at %08x slot", vianum, oa, (vianum - 1) >> 1);
        return lisa_rb_ext_2par_via(&via[vianum], addr);
    }

    ALERT_LOG(0, "Weird extport access to %08x via %d slot %d", oa, vianum, (vianum >> 1));
    return 0xff;
}

uint16 lisa_rw_2x_parallel_h(uint32 addr)
{
    // int vianum=(  (((addr>>0xe)&3)<<1) | (((addr>>0xb)&1)^1)  )+ 3;
    int vianum = get_vianum_from_addr(addr);

    ALERT_LOG(100, "Access to lower VIA #%d at %08x slot %d pc:%08x", vianum, addr, (vianum - 1) >> 1, reg68k_pc);
    return (uint16)(lisa_rb_2x_parallel_h(addr) << 8) + (uint16)(lisa_rb_2x_parallel_h(addr + 1));
}

uint32 lisa_rl_2x_parallel_h(uint32 addr)
{
    // int vianum=(  (((addr>>0xe)&3)<<1) | (((addr>>0xb)&1)^1)  )+ 3;
    int vianum = get_vianum_from_addr(addr);

    ALERT_LOG(100, "Access to lower VIA #%d at %08x slot %d pc:%08x", vianum, addr, (vianum - 1) >> 1, reg68k_pc);
    return (uint32)((uint32)(lisa_rb_2x_parallel_h(addr + 0) << 24) + (uint32)(lisa_rb_2x_parallel_h(addr + 1) << 16) +
                    (uint32)(lisa_rb_2x_parallel_h(addr + 2) << 8) + (uint32)(lisa_rb_2x_parallel_h(addr + 3)));
}

void lisa_wb_2x_parallel_h(uint32 addr, uint8 data)
{
    uint32 oa = addr;
    // int vianum=(  (((addr>>0xe)&3)<<1) | (((addr>>0xb)&1)^1)  )+ 3;
    int vianum = get_vianum_from_addr(addr);

    addr &= 0x00003fff;

    ALERT_LOG(100, "Access to lower VIA #%d at %08x slot %d pc:%08x", vianum, addr, (vianum - 1) >> 1, reg68k_pc);

    if (addr < 0x1fff)
    {
        ALERT_LOG(0, "Attempt to write to Parallel Port ROM offset %08x", addr);
        return;
    }

    if (addr & 1)
    {
        ALERT_LOG(100, "Access to VIA #%d at %08x slot %d", vianum, oa, (vianum - 1) >> 1);
        lisa_wb_ext_2par_via(&via[vianum], addr, data);
        return;
    }

    ALERT_LOG(0, "Unhandled extport access to %08x for VIA #%d slot %d", oa, vianum, vianum >> 1);
}

void lisa_ww_2x_parallel_h(uint32 addr, uint16 data)
{
    lisa_wb_2x_parallel_h(addr, (uint8)(data >> 8));
    lisa_wb_2x_parallel_h(addr + 1, (uint8)(data & 0xff));
}

void lisa_wl_2x_parallel_h(uint32 addr, uint32 data)
{
    lisa_wb_2x_parallel_h(addr, (uint8)(data >> 24));
    lisa_wb_2x_parallel_h(addr + 1, (uint8)(data >> 16));
    lisa_wb_2x_parallel_h(addr + 2, (uint8)(data >> 8));
    lisa_wb_2x_parallel_h(addr + 3, (uint8)(data & 0xff));
}

uint8 *lisa_mptr_Oxd000_ff_space(uint32 addr)
{
    UNUSED(addr);
    return NULL;
}
uint8 lisa_rb_Oxd000_ff_space(uint32 addr)
{
    UNUSED(addr);
    return 0xff;
}
uint16 lisa_rw_Oxd000_ff_space(uint32 addr)
{
    UNUSED(addr);
    return 0xffff;
}
uint32 lisa_rl_Oxd000_ff_space(uint32 addr)
{
    UNUSED(addr);
    return 0xffffffff;
}
void lisa_wb_Oxd000_ff_space(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}
void lisa_ww_Oxd000_ff_space(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}
void lisa_wl_Oxd000_ff_space(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}

// not implemented - for future AMD9512 FPU's
uint8 *lisa_mptr_Oxd400_amd9512(uint32 addr)
{
    UNUSED(addr);
    return NULL;
}
uint8 lisa_rb_Oxd400_amd9512(uint32 addr)
{
    UNUSED(addr);
    return 0xff;
}
uint16 lisa_rw_Oxd400_amd9512(uint32 addr)
{
    UNUSED(addr);
    return 0xffff;
}
uint32 lisa_rl_Oxd400_amd9512(uint32 addr)
{
    UNUSED(addr);
    return 0xffffffff;
}
void lisa_wb_Oxd400_amd9512(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}
void lisa_ww_Oxd400_amd9512(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}
void lisa_wl_Oxd400_amd9512(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}

// reserved memory access for mmu pages out of scope that should not cause mmu_exception
uint8 *lisa_mptr_OxVoid(uint32 addr)
{
    UNUSED(addr);
    return NULL;
}
uint8 lisa_rb_OxVoid(uint32 addr)
{
    UNUSED(addr);
    return 0x13;
}
uint16 lisa_rw_OxVoid(uint32 addr)
{
    UNUSED(addr);
    return 0x1313;
}
uint32 lisa_rl_OxVoid(uint32 addr)
{
    UNUSED(addr);
    return 0x13131313;
}
void lisa_wb_OxVoid(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}
void lisa_ww_OxVoid(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}
void lisa_wl_OxVoid(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    return;
}

uint8 *lisa_mptr_Ox8000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
    if (!slot3l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    // ui_log_verbose("****lisa_mptr_Ox8000_slot3l got called!****");
    return NULL;
}

uint8 lisa_rb_Ox8000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x returning 0xff", addr, reg68k_pc);
    if (!slot3l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    return 0xff;
}

uint16 lisa_rw_Ox8000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
    if (!slot3l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    return 0xffff;
}

uint32 lisa_rl_Ox8000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
    if (!slot3l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    return 0xffffffff;
}

void lisa_wb_Ox8000_slot3(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot3l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
#endif
    return;
}

void lisa_ww_Ox8000_slot3(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot3l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
    return;
#endif
}

void lisa_wl_Ox8000_slot3(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot3l)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
#endif
    return;
}

uint8 *lisa_mptr_Oxa000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
    if (!slot3h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    // ui_log_verbose("*****lisa_mptr_Oxa000_slot3h got called!*****");
    return NULL;
}

uint8 lisa_rb_Oxa000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
    if (!slot3h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    return 0xff;
}

uint16 lisa_rw_Oxa000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
    if (!slot3h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    return 0xffff;
}

uint32 lisa_rl_Oxa000_slot3(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
    if (!slot3h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 1;
        lisa_buserror(addr);
    }
    return 0xffffffff;
}

void lisa_wb_Oxa000_slot3(uint32 addr, uint8 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot3h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
#endif
    return;
}

void lisa_ww_Oxa000_slot3(uint32 addr, uint16 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot3h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
#endif
    return;
}

void lisa_wl_Oxa000_slot3(uint32 addr, uint32 data)
{
    UNUSED(addr);
    UNUSED(data);
    ALERT_LOG(0, "addr: %08x from pc:%08x", addr, reg68k_pc);
#ifndef NO_BUS_ERR_ON_NULL_IO_WRITE
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (!slot3h)
    {
        bustimeout = 1;
        CPU_READ_MODE = 0;
        lisa_buserror(addr);
    }
#endif
    return;
}

//  char unkno1[4]; //00-03
//  char fileid[2]; //04-05
//  char absrel[2]; //06-07
//  char fwdrel[2]; //09-0b
//  char bkdrel[2]; //0a-0b

char *get_tag_name(uint8 n)
{
    switch (n)
    {
    case 0:
        return "volume id high";
    case 1:
        return "volume id low";
    case 2:
        return "unknown";
    case 3:
        return "unknown";
    case 4:
        return "file-id high";
    case 5:
        return "file-id low";
    case 6:
        return "absrel high";
    case 7:
        return "absrel low";
    case 8:
        return "forward relative pointer high";
    case 9:
        return "forward relative pointer low";
    case 10:
        return "back relative pointer high";
    case 11:
        return "back relative pointer low";
    default:
        return "unknown";
    }
}

// static uint32 a0401, a0601, a07ff, a2401, a2601, a27ff, pc401, pc601, pc7ff,
//              sec401=0xdeadbeef, sec601=0xcafebabe, sec7ff=0xf00f7871;  // make sure it doesn't match for sector 0.

void getflopramaddr(uint32 a)
{

#ifdef DEBUGXXXX
    // extra logging to floppy sector log wanted
    if (a == 0xfcc401 || a == 0xfcc601 || a == 0xfcc7ff) // log 1st, middle and last floppy buffer reads so we'll know where data
    {                                                    // is copied to in RAM based on the registers.
        FILE *out;
        int debug_log_cpy = debug_log_enabled;
        debug_log_enabled = 1; // force debug log on and save it's value
        char banner[128];
#define DRIVE (2)  // drive      5               // 00=lower, 80=upper
#define SIDE (3)   // side       7
#define SECTOR (4) // sector     9
#define TRACK (5)  // track      b

#define A0 (8 + 0)
#define A2 (8 + 2)
#define PC (16)
        // uint32 getreg(uint8 regnum)  //16=pc, 17=sp 0=7Dregs 8-15Aregs

        long sectornumber = getsectornum(NULL, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR]);

        if (a > 0xfcc180 && a < 0xfcc200)
            DEBUG_LOG(100, "%08x battery backed up PRAM", a);

        snprintf(banner, 128, "fram@%08x:%3ld ", a, sectornumber);
        out = fopen("./lisaem-output.sec", "a");
        if (out)
        {

            if (a > 0xfcc180 && a < 0xfcc200)
                fprintf(out, "%08x battery backed up PRAM\n", a);
            uint32 a0401 = 0, a0601 = 0, a07ff = 0;
            uint32 a2401 = 0, a2601 = 0, a27ff = 0;
            uint32 pc401 = 0, pc601 = 0, pc7ff = 0;
            uint32 sec401 = 0, sec601 = 0, sec7ff = 0;
            switch (a & 0xfff)
            {
            case 0x401:
                a0401 = getreg(A0);
                a2401 = getreg(A2);
                pc401 = getreg(PC);
                sec401 = sectornumber;
                break;
            case 0x601:
                a0601 = getreg(A0);
                a2601 = getreg(A2);
                pc601 = getreg(PC);
                sec601 = sectornumber;
                break;
            case 0x7ff:
                a07ff = getreg(A0);
                a27ff = getreg(A2);
                pc7ff = getreg(PC);
                sec7ff = sectornumber;
                break;
            }

            fprintf(out, "%s:read #%03ld (hts:%d/%02d/%02d) t:%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    banner, sectornumber, floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR],
                    floppy_ram[0x1F4 + 0], floppy_ram[0x1F4 + 1], floppy_ram[0x1F4 + 2], floppy_ram[0x1F4 + 3], floppy_ram[0x1F4 + 4], floppy_ram[0x1F4 + 5],
                    floppy_ram[0x1F4 + 6], floppy_ram[0x1F4 + 7], floppy_ram[0x1F4 + 8], floppy_ram[0x1F4 + 9], floppy_ram[0x1F4 + 10], floppy_ram[0x1F4 + 11]);
#ifdef DEBUG
            printregs(out, banner); // tag is used so that when I parse the logs, I can grep for
#endif

            fprintf(out, "\n");

            if (sec401 == sec601 && sec601 == sec7ff)
            {
                uint32 start = 0xdeadbeef;
                uint32 end = 0xdeadbeef;

                if (((a07ff - a0401) > 0x180 || (a0401 - a07ff) > 0x180) && (a0601 - a0401) == 0x100)
                {
                    start = a0401;
                    end = a07ff;
                }
                if (((a27ff - a2401) > 0x180 || (a2401 - a27ff) > 0x180) && (a2601 - a2401) == 0x100)
                {
                    start = a2401;
                    end = a27ff;
                }

                fprintf(out, "map::%08x-%08x pc:%08x #%03d(hts:%1d/%02d/%02d) t:%02x %02x.%02x %02x:%02x_%02x:%02x %02x.%02x %02x.%02x %02x\n",
                        start, end, pc401,
                        (uint16)(sectornumber),
                        floppy_ram[SIDE], floppy_ram[TRACK], floppy_ram[SECTOR],
                        floppy_ram[0x1F4 + 0], floppy_ram[0x1F4 + 1], floppy_ram[0x1F4 + 2], floppy_ram[0x1F4 + 3], floppy_ram[0x1F4 + 4], floppy_ram[0x1F4 + 5],
                        floppy_ram[0x1F4 + 6], floppy_ram[0x1F4 + 7], floppy_ram[0x1F4 + 8], floppy_ram[0x1F4 + 9], floppy_ram[0x1F4 + 10], floppy_ram[0x1F4 + 11]);
                fprintf(out, "------------------------------------------------------\n");
            }

            fflush(out);
            fclose(out);
        }                                  // this tag when looking for sector access and see these too.
        debug_log_enabled = debug_log_cpy; // restore debug log value
    }
#endif

    if (!debug_log_enabled)
        return;

    DEBUG_LOG(100, " floppy_ram (fdir=%d [via1 bit 4] wait=%d timer:%ld) %08x::(%02x) ", floppy_FDIR, floppy_6504_wait, fdir_timer,
              a, floppy_ram[(a & 0x7ff) >> 1]);
    switch (a | 1)
    {
    case 0xfcc001:
        DEBUG_LOG(100, "gobyte");
        break;
    case 0xfcc003:
        DEBUG_LOG(100, "function");
        break;
    case 0xfcc005:
        DEBUG_LOG(100, "drive");
        break;
    case 0xfcc007:
        DEBUG_LOG(100, "side");
        break;
    case 0xfcc009:
        DEBUG_LOG(100, "sector");
        break;
    case 0xfcc00b:
        DEBUG_LOG(100, "track");
        break;
    case 0xfcc00d:
        DEBUG_LOG(100, "speed");
        break;
    case 0xfcc00f:
        DEBUG_LOG(100, "format confirm");
        break;
    case 0xfcc011:
        DEBUG_LOG(100, "status");
        break;
    case 0xfcc013:
        DEBUG_LOG(100, "diskID (0=apple2,1=lisa,2=mac)");
        break;
    case 0xfcc015:
        DEBUG_LOG(100, "drvtype (0=twiggy, 1=sony400, 2=sony800)");
        break;
    case 0xfcc017:
        DEBUG_LOG(100, "self diag result");
        break;

    case 0xfcc01b:
        DEBUG_LOG(100, "step error result");
        break;

    case 0xfcc021:
        DEBUG_LOG(100, "motor speed trk group 1");
        break;
    case 0xfcc023:
        DEBUG_LOG(100, "motor speed trk group 2");
        break;
    case 0xfcc025:
        DEBUG_LOG(100, "motor speed trk group 3");
        break;
    case 0xfcc027:
        DEBUG_LOG(100, "motor speed trk group 4");
        break;
    case 0xfcc029:
        DEBUG_LOG(100, "motor speed trk group 5");
        break;

    case 0xfcc02b:
        DEBUG_LOG(100, "min delay post motor speed change");
        break;
    case 0xfcc02d:
        DEBUG_LOG(100, "min delay post seek");
        break;
    case 0xfcc02f:
        DEBUG_LOG(100, "min delay to stop motor");
        break;

    case 0xfcc031:
        DEBUG_LOG(100, "rom version");
        break;

    case 0xfcc033:
        DEBUG_LOG(100, "#retries before re-seek");
        break;
    case 0xfcc035:
        DEBUG_LOG(100, "max retries");
        break;
    case 0xfcc037:
        DEBUG_LOG(100, "post step delay");
        break;
    case 0xfcc039:
        DEBUG_LOG(100, "delay post motor on");
        break;

    case 0xfcc041:
        DEBUG_LOG(100, "disk inserted status (FF=inserted)");
        break;
    case 0xfcc043:
        DEBUG_LOG(100, "motor spinning status (FF=spinning)");
        break;

    case 0xfcc045:
        DEBUG_LOG(100, "current track");
        break;
    case 0xfcc047:
        DEBUG_LOG(100, "current track/16 (trk group)");
        break;
    case 0xfcc049:
        DEBUG_LOG(100, "drive present (FF=yes)");
        break;
    case 0xfcc04b:
        DEBUG_LOG(100, "drive sides #$02=single sided, #$22 double sided");
        break;

    case 0xfcc04d:
        DEBUG_LOG(100, "command error count");
        break;
    case 0xfcc04f:
        DEBUG_LOG(100, "seek retry count");
        break;
    case 0xfcc051:
        DEBUG_LOG(100, "command count");
        break;
    case 0xfcc053:
        DEBUG_LOG(100, "motor retry count");
        break;
    case 0xfcc055:
        DEBUG_LOG(100, "command-0x83 if > 0x83");
        break;

    case 0xfcc057:
        DEBUG_LOG(100, "#self-sync bytes pre-sector when formatting");
        break;
    case 0xfcc059:
        DEBUG_LOG(100, "0x80=drive enable mask");
        break;
        // case 0xfcc059:  DEBUG_LOG(100, "retry"); break;

    case 0xfcc05d:
        DEBUG_LOG(100, "current irq req (drive enable mask & intstatus)");
        break;
    case 0xfcc05f:
        DEBUG_LOG(100, "intstatus - current pending interrupt");
        break;

    case 0xfcc061:
        DEBUG_LOG(100, "copy of cmd block 0xfcc001");
        break;
    case 0xfcc063:
        DEBUG_LOG(100, "copy of cmd block 0xfcc003");
        break;
    case 0xfcc065:
        DEBUG_LOG(100, "copy of cmd block 0xfcc005");
        break;
    case 0xfcc067:
        DEBUG_LOG(100, "copy of cmd block 0xfcc007");
        break;
    case 0xfcc069:
        DEBUG_LOG(100, "copy of cmd block 0xfcc009");
        break;
    case 0xfcc06b:
        DEBUG_LOG(100, "copy of cmd block 0xfcc00b");
        break;
    case 0xfcc06d:
        DEBUG_LOG(100, "copy of cmd block 0xfcc00d");
        break;
    case 0xfcc06f:
        DEBUG_LOG(100, "copy of cmd block 0xfcc00f");
        break;

    case 0xfcc071:
        DEBUG_LOG(100, "sector addr header parts");
        break;
    case 0xfcc073:
        DEBUG_LOG(100, "sector addr header parts");
        break;
    case 0xfcc075:
        DEBUG_LOG(100, "sector addr header parts");
        break;
    case 0xfcc077:
        DEBUG_LOG(100, "sector addr header parts");
        break;
    case 0xfcc079:
        DEBUG_LOG(100, "sector addr header parts");
        break;

    case 0xfcc07b:
        DEBUG_LOG(100, "write scratch data 1");
        break;
    case 0xfcc07d:
        DEBUG_LOG(100, "write scratch data 2");
        break;

    case 0xfcc081:
        DEBUG_LOG(100, "cmd delay low");
        break;
    case 0xfcc083:
        DEBUG_LOG(100, "cmd delay mid");
        break;
    case 0xfcc085:
        DEBUG_LOG(100, "cmd delay high");
        break;

    case 0xfcc087:
        DEBUG_LOG(100, "address pointer low");
        break;
    case 0xfcc089:
        DEBUG_LOG(100, "address pointer high");
        break;

    case 0xfcc08B:
        DEBUG_LOG(100, "always 00");
        break;
    case 0xfcc08d:
        DEBUG_LOG(100, "always ff");
        break;

    case 0xfcc091:
        DEBUG_LOG(100, "#retries of sector header not found");
        break;
    case 0xfcc093:
        DEBUG_LOG(100, "#retries of sector trailer not found");
        break;
    case 0xfcc095:
        DEBUG_LOG(100, "#retries bad checksum");
        break;

    case 0xfcc097:
        DEBUG_LOG(100, "#retries of address header not found");
        break;
    case 0xfcc099:
        DEBUG_LOG(100, "#retries of address trailer not found");
        break;

    case 0xfcc09b:
        DEBUG_LOG(100, "#retries of sector not found on track");
        break;

    case 0xfcc09d:
        DEBUG_LOG(100, "#retries of bad track # in header");
        break;
    case 0xfcc09f:
        DEBUG_LOG(100, "#retries of addr header checksum wrong");
        break;

    case 0xfcc0a1:
        DEBUG_LOG(100, "address header storage 1");
        break;
    case 0xfcc0a3:
        DEBUG_LOG(100, "address header storage 1");
        break;
    case 0xfcc0a5:
        DEBUG_LOG(100, "address header storage 1");
        break;
    case 0xfcc0a7:
        DEBUG_LOG(100, "address header storage 1");
        break;
    case 0xfcc0a9:
        DEBUG_LOG(100, "address header storage 1");
        break;

    case 0xfcc0ab:
        DEBUG_LOG(100, "address header chksum");
        break;

    case 0xfcc0ad:
        DEBUG_LOG(100, "did last seek move disk head");
        break;

    case 0xfcc0af:
        DEBUG_LOG(100, "did last access turn motor on");
        break;

    case 0xfcc0b1:
        DEBUG_LOG(100, "step count on last seek (trackdiff*8)");
        break;
    case 0xfcc0b3:
        DEBUG_LOG(100, "step direction on last seek");
        break;

    // case 0xfcc0b7:  DEBUG_LOG(100, "dat_bitslip1"); break;
    case 0xfcc0b7:
        DEBUG_LOG(100, "(retry count in addr header | ROM cksum | motor rotation time) LSB");
        break;
    case 0xfcc0b9:
        DEBUG_LOG(100, "(retry count in addr header | ROM cksum | motor rotation time) MSB");
        break;
        // case 0xfcc0b9:  DEBUG_LOG(100, "dat_bitslip2"); break;
        // case 0xfcc0bb:  DEBUG_LOG(100, "dat_chksum"); break;
        // case 0xfcc0bd:  DEBUG_LOG(100, "adr_bitslip1"); break;
        // case 0xfcc0bf:  DEBUG_LOG(100, "adr_bitslip2"); break;
        // case 0xfcc0c1:  DEBUG_LOG(100, "wrong_sec"); break;
        // case 0xfcc0c3:  DEBUG_LOG(100, "wrong_trk"); break;
        // case 0xfcc0c5:  DEBUG_LOG(100, "adr_chksum"); break;

    case 0xfcc0bb:
        DEBUG_LOG(100, "temp write area 1");
        break;

    case 0xfcc0bd:
        DEBUG_LOG(100, "temp write area 2");
        break;
    case 0xfcc0bf:
        DEBUG_LOG(100, "temp write area 3");
        break;

    case 0xfcc0c1:
        DEBUG_LOG(100, "read/write sector expected checksum value LowSB");
        break;
    case 0xfcc0c3:
        DEBUG_LOG(100, "read/write sector expected checksum value MidSB");
        break;
    case 0xfcc0c5:
        DEBUG_LOG(100, "read/write sector expected checksum value HISB");
        break;

    case 0xfcc0c7:
        DEBUG_LOG(100, "read sector actual checksum value LowSB");
        break;
    case 0xfcc0c9:
        DEBUG_LOG(100, "read sector actual checksum value MidSB");
        break;
    case 0xfcc0cb:
        DEBUG_LOG(100, "read sector actual checksum value HISB");
        break;

    case 0xfcc0cf:
        DEBUG_LOG(100, "#of sector for curr track in format and verify commands");
        break;
    case 0xfcc0d1:
        DEBUG_LOG(100, "ignore chksum flag for read/write brute");
        break;

    case 0xfcc0d3:
        DEBUG_LOG(100, "computed seek delay post seek");
        break;

    case 0xfcc0d5:
        DEBUG_LOG(100, "future use 1");
        break;
    case 0xfcc0d7:
        DEBUG_LOG(100, "future use 2");
        break;
    case 0xfcc0d9:
        DEBUG_LOG(100, "future use 3");
        break;
    case 0xfcc0db:
        DEBUG_LOG(100, "future use 4");
        break;
    case 0xfcc0dd:
        DEBUG_LOG(100, "tmp save scratch 1");
        break;
    case 0xfcc0df:
        DEBUG_LOG(100, "tmp save scratch 2");
        break;

    case 0xfcc0e1:
        DEBUG_LOG(100, "sector data constants 1");
        break;
    case 0xfcc0e3:
        DEBUG_LOG(100, "sector data constants 2");
        break;
    case 0xfcc0e5:
        DEBUG_LOG(100, "sector data constants 3");
        break;
    case 0xfcc0e7:
        DEBUG_LOG(100, "sector data constants 4");
        break;
    case 0xfcc0e9:
        DEBUG_LOG(100, "sector data constants 5");
        break;

    case 0xfcc0eb:
        DEBUG_LOG(100, "format routine count of even sectors");
        break;
    case 0xfcc0ed:
        DEBUG_LOG(100, "format routine count of odd sectors");
        break;
    case 0xfcc0ef:
        DEBUG_LOG(100, "current sector even/odd : 0/1");
        break;

    case 0xfcc0f1:
        DEBUG_LOG(100, "format loop count");
        break;
    case 0xfcc0f3:
        DEBUG_LOG(100, "seek loop count");
        break;

    case 0xfcc0f7:
        DEBUG_LOG(100, "total error count in r/w routine");
        break;

    case 0xfcc0fb:
        DEBUG_LOG(100, "current index in command history in format (8*n)-1");
        break;
    case 0xfcc0fd:
        DEBUG_LOG(100, "pointer to cmd history lsb");
        break;
    case 0xfcc0ff:
        DEBUG_LOG(100, "pointer to cmd history msb");
        break;

    // case 0xfcc0fd:  DEBUG_LOG(100, "usr_cksum1"); break;
    // case 0xfcc0ff:  DEBUG_LOG(100, "usr_cksum2"); break;
    // case 0xfcc101:  DEBUG_LOG(100, "usr_cksum3"); break;

    // these are wrong?
    case 0xfcc161:
        DEBUG_LOG(100, "pram Error code");
        break;
    case 0xfcc163:
        DEBUG_LOG(100, "pram Contents of memory error address latch if parity error");
        break;
    case 0xfcc165:
        DEBUG_LOG(100, "pram Contents of memory error address latch if parity error");
        break;
    case 0xfcc167:
        DEBUG_LOG(100, "pram Memory board slot # if memory error");
        break;
    case 0xfcc169:
        DEBUG_LOG(100, "pram Last value read from clock 1");
        break;
    case 0xfcc16b:
        DEBUG_LOG(100, "pram Last value read from clock 2");
        break;
    case 0xfcc16d:
        DEBUG_LOG(100, "pram Last value read from clock 3");
        break;
    case 0xfcc16f:
        DEBUG_LOG(100, "pram Last value read from clock 4");
        break;
    case 0xfcc171:
        DEBUG_LOG(100, "pram Last value read from clock 5");
        break;
    case 0xfcc173:
        DEBUG_LOG(100, "pram Last value read from clock 6");
        break;
    case 0xfcc17D:
        DEBUG_LOG(100, "pram Checksum");
        break;
    case 0xfcc17f:
        DEBUG_LOG(100, "pram Checksum");
        break;

    case 0xfcc3a1:
        DEBUG_LOG(100, "bad_sec_total found by verify");
        break;
    case 0xfcc3a3:
        DEBUG_LOG(100, "track # where error was found by verify command");
        break;
    case 0xfcc3a5:
        DEBUG_LOG(100, "side where error was found by verify command");
        break;
    case 0xfcc3a7:
        DEBUG_LOG(100, "bad_sect_map");
        break;

    case 0xfcc3e9:
        DEBUG_LOG(100, "buff_data_tag_begin");
        break; //  (524 bytes!) - tag bytes first  it's not 24 bytes, it is 12 bytes because flopmem is distributed"); break;

    // case 0xfcc401:  DEBUG_LOG(100, "disk sector data"); break;
    // case 0xfcc7ff:  DEBUG_LOG(100, "buff_data_end"); break;
    // case 0xfcc801:  DEBUG_LOG(100, "parameter memory for lisa"); break;  //*  nvram at c181-c1ff (might still be true)
    default:

        if (a > 0xfcc180 && a < 0xfcc200)
            DEBUG_LOG(100, "battery backed up PRAM")
        else if (a > 0xfcc200 && a < 0xfcc39f)
            DEBUG_LOG(100, "6504 stack")
        else if (a > 0xfcc3a6 && a < 0xfcc3be)
            DEBUG_LOG(100, "bad sector map")
        else if (a > 0xfcc400 && a < 0xfcc800)
            DEBUG_LOG(100, "disk data buffer")
        else if (a > 0xfcc3e8 && a < 0xfcc400)
            DEBUG_LOG(100, "disk tag #%d %s)", (a - 0xfcc3e9) / 2, get_tag_name((a - 0xfcc3e9) / 2))
        else if (a > 0xfcc100 && a < 0xfcc17f)
            DEBUG_LOG(100, "command history buffer")
        else
            DEBUG_LOG(100, "unknown")

#ifdef DEBUG
        if (a == 0xfcc401 || a == 0xfcc601 || a == 0xfcc7ff) // log 1st and last sector buffer reads so we'll know where data is copied to in RAM
        {
            int debug_log_cpy = debug_log_enabled;
            debug_log_enabled = 1; // force debug log on and save it's value

            printregs(buglog, "flopram:"); // tag is used so that when I parse the logs, I can grep for

            // this tag when looking for sector access and see these too.
            debug_log_enabled = debug_log_cpy; // restore debug log value
        }
#endif
    }
}

uint8 *lisa_mptr_Oxc000_flopmem(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxc000_flopmem got called!****");
    getflopramaddr(addr);
    addr = (addr & 0x07ff) >> 1;
    return (uint8 *)(&floppy_ram[addr]);
}

uint8 lisa_rb_Oxc000_flopmem(uint32 xaddr)
{
    uint32 addr;
    getflopramaddr(xaddr);
    addr = (xaddr & 0x07ff) >> 1;
    DEBUG_LOG(100, "floppy read byte value: %02x @ %08x pc:%08x", (floppy_ram[addr]), xaddr, pc24);
    // If the Lisa is checking this the GOBYTE, then it's waiting for it to be accepted,
    // so speed things up and execute whatever needs to be done.
    if (!addr && floppy_ram[addr])
    {
        floppy_go6504();
    }

    return (floppy_ram[addr]);
}

uint16 lisa_rw_Oxc000_flopmem(uint32 xaddr)
{
    uint32 addr;
    CHECK_DIRTY_MMU(xaddr);
    getflopramaddr(xaddr);
    addr = (xaddr & 0x07ff) >> 1;

    DEBUG_LOG(100, "floppy read word value: %04x @ %08x pc:%08x",
              ((floppy_ram[addr]) << 8) | (floppy_ram[addr]), xaddr, pc24);

    // If the Lisa is checking this the GOBYTE, then it's waiting for it to be accepted,
    // so speed things up and execute whatever needs to be done.
    if (!addr && floppy_ram[addr])
    {
        floppy_go6504();
    }

    // fill don't cares with duplicate value - might need to fill with vidram data instead?
    return ((floppy_ram[addr]) << 8) | (floppy_ram[addr]);
}

uint32 lisa_rl_Oxc000_flopmem(uint32 xaddr)
{
    uint32 addr;
    CHECK_DIRTY_MMU(xaddr);
    getflopramaddr(xaddr);
    addr = (xaddr & 0x07ff) >> 1;

    DEBUG_LOG(100, "floppy read long value: %08x @ %08x pc:%08x",
              ((floppy_ram[addr]) << 24) | (floppy_ram[addr] << 16) | ((floppy_ram[addr + 1]) << 8) | (floppy_ram[addr + 1]),
              xaddr, pc24);

    // 24,16,8,0
    //  x  o x o  -- x=don't care., o = odd, write.
    //  e  o e o  -- even byte, odd byte, etc.
    //
    // fill don't cares with duplicate value - might need to fill with vidram data instead?

    // If the Lisa is checking this the GOBYTE, then it's waiting for it to be accepted,
    // so speed things up and execute whatever needs to be done.
    if (!addr && floppy_ram[addr])
    {
        floppy_go6504();
    }

    return ((floppy_ram[addr]) << 24) | (floppy_ram[addr] << 16) | ((floppy_ram[addr + 1]) << 8) | (floppy_ram[addr + 1]);
}

void lisa_wb_Oxc000_flopmem(uint32 addr, uint8 data)
{
    // skip ram speedup hack - only for the right kinds of ROM
    if (cheat_ram_test && addr == 0x00fcc18d && lisarom[0x3ffc] == 0x02 && lisarom[0x3ffd] >= 'F')
        data &= (0xff - 64); // trick ROM into doing one pass of memory test so as to speed it up.

    //   if (addr>0xfcc160 && addr<0xfcc200)
    //      {
    //        if (floppy_ram[(addr & 0x07ff)>>1]!=data)
    //           ALERT_LOG(0,"PRAM old:%02x changed to %02x at %08x",floppy_ram[(addr & 0x07ff)>>1],data,addr);
    //      }

    CHECK_DIRTY_MMU(addr);
#ifdef DEBUG
    if (debug_log_enabled)
    {
        DEBUG_LOG(100, "floppymem write %02x->%08x", data, addr);
        getflopramaddr(addr);
    }
#endif

    //    if ((addr & 1)==0) return;            // ignore even addresses

    addr = (addr & 0x07ff) >> 1;

    // get out of jail free card
    if (!addr && floppy_6504_wait == 254 && data == 0x96 && floppy_ram[0] == 0x69)
    {
        floppy_6504_wait = 0;
        floppy_ram[0] = 0;
        DEBUG_LOG(100, "floppy out of jail");
    }
    if (!addr && floppy_6504_wait == 254 && data == 0x69 && floppy_ram[0] == 0x96)
    {
        floppy_6504_wait = 0;
        floppy_ram[0] = 0;
        DEBUG_LOG(100, "floppy out of jail");
    }

    floppy_ram[addr] = data;

    if (!addr && floppy_ram[addr])
    {
        floppy_go6504();
    }
}

void lisa_ww_Oxc000_flopmem(uint32 addr, uint16 data)
{

    if (addr > 0xfcc180 && addr < 0xfcc200)
        ALERT_LOG(0, "PRAM write %04x to %08x", data, addr);

    if (debug_log_enabled)
    {
        DEBUG_LOG(100, "floppymem write %04x->%08x", data, addr);
        getflopramaddr(addr);
    }

    CHECK_DIRTY_MMU(addr);
    addr = (addr & 0x07ff) >> 1; // ignore even addrs //floppy_ram[addr]=data>>8;

    // get out of jail free card
    if (!addr && floppy_6504_wait == 254 && (data & 0xff) == 0x96 && floppy_ram[0] == 0x69)
    {
        floppy_6504_wait = 0;
        floppy_ram[0] = 0;
        DEBUG_LOG(100, "floppy out of jail");
    }
    if (!addr && floppy_6504_wait == 254 && (data & 0xff) == 0x69 && floppy_ram[0] == 0x96)
    {
        floppy_6504_wait = 0;
        floppy_ram[0] = 0;
        DEBUG_LOG(100, "floppy out of jail");
    }

    floppy_ram[addr] = data & 0xff;
    if (!addr && floppy_ram[addr])
    {
        floppy_go6504();
    }
}

void lisa_wl_Oxc000_flopmem(uint32 addr, uint32 data)
{

    if (addr > 0xfcc180 && addr < 0xfcc200)
        ALERT_LOG(0, "PRAM write %08x to %08x", data, addr);

    if (debug_log_enabled)
    {
        DEBUG_LOG(100, "floppymem write %08x->%08x ");
        getflopramaddr(addr);
    }
    CHECK_DIRTY_MMU(addr);
    addr = (addr & 0x07ff) >> 1;

    // get out of jail free card
    if (!addr && floppy_6504_wait == 254 && (data & 0xff) == 0x96 && floppy_ram[0] == 0x69)
    {
        floppy_6504_wait = 0;
        floppy_ram[0] = 0;
        DEBUG_LOG(100, "floppy out of jail");
    }
    if (!addr && floppy_6504_wait == 254 && (data & 0xff) == 0x69 && floppy_ram[0] == 0x96)
    {
        floppy_6504_wait = 0;
        floppy_ram[0] = 0;
        DEBUG_LOG(100, "floppy out of jail");
    }

    // ignore even addrs //floppy_ram[addr  ]=(data>>24); // no need to and here, since source is 32 bit

    // 24,16,8,0
    //  x  o x o  -- x=don't care., o = odd, write.
    //  e  o e o  -- even byte, odd byte, etc.
    floppy_ram[addr] = (data >> 16) & 0xff;
    floppy_ram[addr + 1] = (data) & 0xff; // +1, not +2 because we divided addr by two with addr=(addr & 0x7ff)>>1 above

    if (!addr && floppy_ram[addr])
    {
        floppy_go6504();
    }
}

uint8 *lisa_mptr_Oxd200_sccz8530(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxd200_sccz8530 got called!****");
    return NULL;
}

// This lives in the zilog8530.c file so I just have a prototype in here...

uint16 lisa_rw_Oxd200_sccz8530(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return (lisa_rb_Oxd200_sccz8530(addr) << 8) | lisa_rb_Oxd200_sccz8530(1 + addr);
}

uint32 lisa_rl_Oxd200_sccz8530(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return (lisa_rb_Oxd200_sccz8530(addr) << 24) |
           (lisa_rb_Oxd200_sccz8530(1 + addr) << 16) |
           (lisa_rb_Oxd200_sccz8530(2 + addr) << 8) |
           (lisa_rb_Oxd200_sccz8530(3 + addr));
}

void lisa_ww_Oxd200_sccz8530(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    lisa_wb_Oxd200_sccz8530(0 + addr, data >> 8);
    lisa_wb_Oxd200_sccz8530(1 + addr, data & 0xff);
}

void lisa_wl_Oxd200_sccz8530(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    lisa_wb_Oxd200_sccz8530(0 + addr, (uint8)((data >> 24) & 0xff));
    lisa_wb_Oxd200_sccz8530(1 + addr, (uint8)((data >> 16) & 0xff));
    lisa_wb_Oxd200_sccz8530(2 + addr, (uint8)((data >> 8) & 0xff));
    lisa_wb_Oxd200_sccz8530(3 + addr, (uint8)(data & 0xff));
}

uint8 *lisa_mptr_Oxd800_par_via2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxd800_par_via2 got called!****");
    return NULL;
}

uint16 lisa_rw_Oxd800_par_via2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return (lisa_rb_Oxd800_par_via2(addr) << 8) | lisa_rb_Oxd800_par_via2(addr + 1);
}

uint32 lisa_rl_Oxd800_par_via2(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return (lisa_rb_Oxd800_par_via2(addr) << 24) | lisa_rb_Oxd800_par_via2(addr) << 16 |
           (lisa_rb_Oxd800_par_via2(addr) << 8) | lisa_rb_Oxd800_par_via2(addr);
}

void lisa_ww_Oxd800_par_via2(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    lisa_wb_Oxd800_par_via2(addr, (uint8)((data >> 8) & 0xff));
    lisa_wb_Oxd800_par_via2(addr + 1, (uint8)((data) & 0xff));
}

void lisa_wl_Oxd800_par_via2(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // HWG says that it's possible to write words and longs to these ports and have it work properly
    // without worrying about the address.
    lisa_wb_Oxd800_par_via2(addr + 0, (uint8)((data >> 24) & 0xff));
    lisa_wb_Oxd800_par_via2(addr + 1, (uint8)((data >> 16) & 0xff));
    lisa_wb_Oxd800_par_via2(addr + 2, (uint8)((data >> 8) & 0xff));
    lisa_wb_Oxd800_par_via2(addr + 3, (uint8)((data) & 0xff));
}

uint8 *lisa_mptr_Oxdc00_cops_via1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxdc00_cops_via1 got called!****");
    return NULL;
}

uint16 lisa_rw_Oxdc00_cops_via1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return (lisa_rb_Oxdc00_cops_via1(addr + 0) << 8) |
           (lisa_rb_Oxdc00_cops_via1(addr + 1));
}

uint32 lisa_rl_Oxdc00_cops_via1(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return (lisa_rb_Oxdc00_cops_via1(addr + 0) << 24) |
           (lisa_rb_Oxdc00_cops_via1(addr + 1) << 16) |
           (lisa_rb_Oxdc00_cops_via1(addr + 2) << 8) |
           (lisa_rb_Oxdc00_cops_via1(addr + 3));
}

void lisa_ww_Oxdc00_cops_via1(uint32 addr, uint16 data)
{
    // HWG says that it's possible to write words and longs to these ports and have it work properly
    // without worrying about the address.
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);

    lisa_wb_Oxdc00_cops_via1(addr + 0, (uint8)(data >> 8));
    lisa_wb_Oxdc00_cops_via1(addr + 1, (uint8)(data & 0xff));
}

void lisa_wl_Oxdc00_cops_via1(uint32 addr, uint32 data)
{
    // HWG says that it's possible to write words and longs to these ports and have it work properly
    // without worrying about the address.
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    lisa_wb_Oxdc00_cops_via1(addr + 0, (uint8)((data >> 24) & 0xff));
    lisa_wb_Oxdc00_cops_via1(addr + 1, (uint8)((data >> 16) & 0xff));
    lisa_wb_Oxdc00_cops_via1(addr + 2, (uint8)((data >> 8) & 0xff));
    lisa_wb_Oxdc00_cops_via1(addr + 3, (uint8)((data) & 0xff));
}

uint8 *lisa_mptr_Oxe000_latches(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxe000_latches got called!****");
    return NULL;
}

void lisa_wb_Oxe000_latches(uint32 addr, uint8 data)
{
    UNUSED(data);
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);

    switch (addr & 0x0000001f)
    { // 0xfce???
        //---------------------------------------------------------------------------------------------------------------------
        // only change latches if they're not the value we're trying to set.  Note that there's a hardware bug in the mc68000
        // for CLR that does a read before a write and we'd wind up refreshing the mmu tables twice!  checking for a setting
        // being set prevents doing 2x. the work, since mmu flushes are expensive.  i.e. the if (!segment1) before setting
        // the latch.  Note that the return is outside the if statement, rather than using breaks.  This is to keep the code
        // visually uncluttered, as well as to exit the switch without further tests.  No need to do a break, which will just
        // jmp to code that that will just exit the function anyway (the return.)  Scroll right to see the return.
        //
        // Yes, this is wide, but it's 2003 already, if your editor/terminal can't do more than 80 columns, get yourself an
        // upgrade!

        // if (!segment1)
        // if ( segment1)
        // if (!segment2)
        // if ( segment2)
        // if (!start)
        // if ( start)

        //
        //---------------------------------------------------------------------------------------------------------------------

    case 0x002:
        DEBUG_LOG(100, "diag1=1 was %d", diag1);
        if (!diag1)
        {
            diag1 = 1;
        }
        return;
    case 0x000:
        DEBUG_LOG(100, "diag1=0 was %d", diag1);
        if (diag1)
        {
            diag1 = 0;
        }
        return;
    case 0x006:
        DEBUG_LOG(100, "diag2=1 was %d", diag2);
        if (!diag2)
        {
            diag2 = 1;
            activate_parity_check();
        }
        return;
    case 0x004:
        DEBUG_LOG(100, "diag2=0 was %d", diag2);
        if (diag2)
        {
            diag2 = 0;
        }
        return;

    case 0x00A:
        DEBUG_LOG(100, "seg1 =1 was %d SRC:context_switch!", segment1);
        {
            segment1 = 1;
            if (CXSASEL != context)
                mmuflush(0);
        }
        return;
    case 0x008:
        DEBUG_LOG(100, "seg1 =0 was %d SRC:context_switch!", segment1);
        {
            segment1 = 0;
            if (CXSASEL != context)
                mmuflush(0);
        }
        return;
    case 0x00E:
        DEBUG_LOG(100, "seg2 =2 was %d SRC:context_switch!", segment2);
        {
            segment2 = 2;
            if (CXSASEL != context)
                mmuflush(0);
        }
        return;
    case 0x00C:
        DEBUG_LOG(100, "seg2 =0 was %d SRC:context_switch!", segment2);
        {
            segment2 = 0;
            if (CXSASEL != context)
                mmuflush(0);
        }
        return;
    case 0x010:
        DEBUG_LOG(100, "start=1 was %d SRC:context_switch!", start);
        {
            start = 1;
            if (CXSASEL != context)
                mmuflush(0);
        }
        return;
    case 0x012:
        DEBUG_LOG(100, "start=0 was %d SRC:context_switch!", start);
        {
            start = 0;
            if (CXSASEL != context)
                mmuflush(0);
        }
        return;

    // fce01a - turn on video irq's
    case 0x018:
        DEBUG_LOG(100, "About to reset VideoIRQ. videoirq=%d vertical=%d", videoirq, vertical);

        if (videoirq)
        {
            vertical = 0;
            serialnumshiftcount = 0;
            serialnumshift = 0;
            DEBUG_LOG(100, "virq:VTIRMASK: disabling vertical retrace IRQ's");
        }
        else
        {
            DEBUG_LOG(100, "virq clear is already cleared...");
        }
        videoirq = 0; // 20060610 - was 0
        verticallatch = 0;
        // DEBUG_LOG(100,"video IRQ turned OFF");
        return; // allow fall through

    case 0x01A:
        DEBUG_LOG(100, "About to ENABLE VideoIRQ. videoirq=%d vertical=%d", videoirq, vertical);
        if (!videoirq)
        {
            serialnumshiftcount = 0;
            serialnumshift = 0;
        }
        videoirq = 1;
        DEBUG_LOG(100, "virq:VTIRMASK: enabling vertical retrace IRQ's");
        verticallatch = 0;
        reset_video_timing();
        return;
        // fce018                        // was vertical=0 for fce018, disabling to satisfy lisa os 3.1

    case 0x016:
        DEBUG_LOG(100, "softmem on");
        if (!softmem)
        {
            softmem = 1;
            DEBUG_LOG(100, "softmem=1");
        }
        else
        {
            DEBUG_LOG(100, "Softmem error already on, issuing NMI now.");
            lisa_softmem_error(pc24);
        }
        return;
    case 0x014:
        DEBUG_LOG(100, "softmem off");
        if (softmem)
        {
            softmem = 0;
            DEBUG_LOG(100, "softmem=0");
        }
        return;

    case 0x01E:
        DEBUG_LOG(100, "hardmem on");
        if (!hardmem)
        {
            hardmem = 1;
            DEBUG_LOG(100, "hardmem=1");
        }
        else
        {
#ifndef LISA1
            DEBUG_LOG(100, "Hardmem error already on, issuing NMI now.");

            lisa_hardmem_error(pc24);
#endif
        }
        return;

    case 0x01C:
        DEBUG_LOG(100, "hardmem off");
        if (hardmem)
        {
            hardmem = 0;
            DEBUG_LOG(100, "hardmem=0");
        }
        return;

    // trap access to hidden latches if any or run away code...
    default:
        DEBUG_LOG(100, "Strange access to latches @ %08x filtered is: %08x", addr, addr & 0xfff);
    }
}
// These are, of course cheats, but it's ok.  Yes, both reads and writes to latches have the same effects, because the latches
// are JK(?) flip-flops with their input lines triggered by the >ADDRESS< placed on the bus, not the contents of a write.  So
// reads can set/clear the latches too.  The lisa never accesses two latches at a time, so no need to worry about long accesses.

uint8 lisa_rb_Oxe000_latches(uint32 addr)
{
    lisa_wb_Oxe000_latches(addr & 0x00fffffe, 0);
    return 0;
}
uint16 lisa_rw_Oxe000_latches(uint32 addr)
{
    lisa_wb_Oxe000_latches(addr & 0x00fffffe, 0);
    return 0;
}
uint32 lisa_rl_Oxe000_latches(uint32 addr)
{
    lisa_wb_Oxe000_latches(addr & 0x00fffffe, 0);
    return 0;
}
void lisa_ww_Oxe000_latches(uint32 addr, uint16 data)
{
    lisa_wb_Oxe000_latches(addr & 0x00fffffe, 0);
    UNUSED(data);
}
void lisa_wl_Oxe000_latches(uint32 addr, uint32 data)
{
    lisa_wb_Oxe000_latches(addr & 0x00fffffe, 0);
    UNUSED(data);
}

uint8 *lisa_mptr_Oxe800_videlatch(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxe800_videlatch got called!****");
    return NULL;
}

uint8 lisa_rb_Oxe800_videlatch(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if ((addr & 0x00000fff) == 0x00000800)
        return (videolatch & 0x000000ff);
    else
        return 0;
}

uint16 lisa_rw_Oxe800_videlatch(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if ((addr & 0x00000fff) == 0x00000800)
        return (videolatch & 0x000000ff);
    else
        return 0;
}

uint32 lisa_rl_Oxe800_videlatch(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if ((addr & 0x00000fff) == 0x00000800)
        return (videolatch & 0x000000ff);
    else
        return 0;
}

void refresh_vidram_fns(void)
{ // erase old vidram fn's and update the new one.
    int i, j;
    uint32 a, videomax = 0;

    // if (maxlisaram==1024*1024) videolatchaddress-=0x80000;
    videomax = videolatchaddress + 32768;

    // ALERT_LOG(100,"new videolatch address:%02x->@%08x:Refreshing mmu_t tables for vidram for %08x-%08x at pc=%d/%08x",data,addr,videolatchaddress,videomax-1,context,pc24);

    //    for (j=0; j<5; j++)
    for (i = 0; i < 32768; i++)
    { // clear existing vidram and turn it into just ram
        if (mmu_trans_all[context][i].writefn == vidram)
            mmu_trans_all[context][i].writefn = ram;
        // if it's RAM check to see if it can be vidram
        if (mmu_trans_all[context][i].writefn == ram)
        {
            a = ((i << 9) + mmu_trans_all[context][i].address) & TWOMEGMLIM;
            if (a >= videolatchaddress && a < videomax)
                mmu_trans_all[context][i].writefn = vidram;
        }
    }
    videoramdirty = 32768;
    videoximgdirty |= 9;
    LisaScreenRefresh();
    videoramdirty = 32768;
    videoximgdirty |= 9;
}

extern void disable_4MB_macworks(void);

void lisa_wb_Oxe800_videlatch(uint32 addr, uint8 data)
{
    int i, j;
    uint32 a, videomax = 0;
    //  #ifdef DEBUG
    //  char shout[1024];
    //  #endif

    ALERT_LOG(100, "@%08x:%02x videolatch/motherboard error led, set by pc:%d/%08x", addr, data, context, reg68k_pc);

    videoramdirty = 32768;

    if ((addr & 0x000000fff) == 0x00000800)
    {
        if (!!(data & 0x80))
        {
            ALERT_LOG(100, "video latch write:MOTHERBOARD ERROR LED FLASHING: @%08x latch=%02x set by pc:%d/%08x", addr, data, context, reg68k_pc);
            return;
        }
        data &= 0x7f;
        // if  ( (maxlisaram!=1024*1024 && ((uint32)(data<<15) < maxlisaram ))  || (maxlisaram==1024*1024 && ((uint32)(data<<15) < maxlisaram+0x80000 )) )

        if ((uint32)(data * 32768) < maxlisaram && (uint32)(data * 32768) >= minlisaram)
        {
            videolatch = data & 0x7f;
            videolatchaddress = (videolatch * 32768);
            ALERT_LOG(0, "Video Latch set to:%02x address:%08x", videolatch, videolatchaddress);
            if (lastvideolatch != videolatch)
                refresh_vidram_fns();
            lastvideolatch = videolatch;
            lastvideolatchaddress = videolatchaddress;

            if ((reg68k_pc & 0x00ff0000) == 0x00fe0000 && macworks4mb)
            {
                ALERT_LOG(0, "disabling 4mb macworks");
                disable_4MB_macworks();
            }
        }
        else
        {
            ALERT_LOG(100, "video latch write:OUT OF RANGE:%02x->@%08x %08x-%08x at pc=%d/%08x\n",
                      data,
                      addr,
                      videolatchaddress,
                      videomax - 1,
                      context,
                      pc24);
        }
        //   if (data!=((maxlisaram>>15)-1))
        //       {   debug_log_enabled=1; debug_on(""); DEBUG_LOG(100,"Debug ON - latch write");
        //           DEBUG_LOG(100,"Debug Enabled because of write %02x to %08x video latch at pc=%d/%08x",data,addr,context,pc24);
        //       }
    }
}

void lisa_ww_Oxe800_videlatch(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);

    DEBUG_LOG(100, "Warning word write to video latch:%04x->@%08x", data, addr);

    lisa_wb_Oxe800_videlatch(addr, (uint8)((data >> 8) & 0xff));
    lisa_wb_Oxe800_videlatch(addr + 1, (uint8)((data) & 0xff));
}

void lisa_wl_Oxe800_videlatch(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    DEBUG_LOG(100, "Warning long write to video latch:%08x->@%08x", data, addr);
    lisa_wb_Oxe800_videlatch(addr, (uint8)((data >> 24) & 0xff));
    lisa_wb_Oxe800_videlatch(addr + 1, (uint8)((data >> 16) & 0xff));
    lisa_wb_Oxe800_videlatch(addr + 2, (uint8)((data >> 8) & 0xff));
    lisa_wb_Oxe800_videlatch(addr + 3, (uint8)((data) & 0xff));
}

uint8 *lisa_mptr_Oxf000_memerror(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxf000!****"); return NULL;
    return NULL;
}

uint8 lisa_rb_Oxf000_memerror(uint32 addr)
{

    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    bustimeout = 0;
    return ((addr & 1) ? ((uint8)(memerror & 0xff00) >> 8) : ((uint8)(memerror & 0x00fe)));

    // switch (addr & 0x00000001)
    //{
    //     case 0x0000:  return (uint8)(memerror & 0xff000000)>>24;   //FCF000
    //     case 0x0001:  return (uint8)(memerror & 0x00ff0000)>>16;   //FCF001
    //     case 0x0002:  return (uint8)(memerror & 0x0000ff00)>>8;    //FCF002
    //     case 0x0003:  return (uint8)(memerror & 0x000000ff);       //FCF003
    // }
    // return 0; // so the compiler doesn't bitch at us, I know there's no other path, but the compiler is a dumb turd
}

uint16 lisa_rw_Oxf000_memerror(uint32 addr)
{
    UNUSED(addr);
    DEBUG_LOG(100, "Memerror:Returning:%08x as a uint16:%04x", memerror, (uint16)(memerror & 0xffff));
    bustimeout = 0;
    return (memerror & 0xfffe);
}

uint32 lisa_rl_Oxf000_memerror(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);

    if ((addr & 0x00000fff) == 0)
        return (uint32)(memerror & 0xfffe);
    bustimeout = 0;
    return 0L;
}

void lisa_wb_Oxf000_memerror(uint32 addr, uint8 data)
{
    // uint16 status=0;

    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    bustimeout = 0;
    memerror = (addr & 1) ? ((memerror & 0xff00) | data) : ((memerror & 0x00ff) | (data << 8));
    return;

    //    switch (addr & 0x000001)
    //    {
    //        case 0x0000:  memerror=(memerror & 0x00ffffff) | data<<24; return;  //FCF000
    //        case 0x0001:  memerror=(memerror & 0xff00ffff) | data<<16; return;  //FCF001
    //        case 0x0002:  memerror=(memerror & 0xffff00ff) | data<<8;  return;  //FCF002
    //        case 0x0003:  memerror=(memerror & 0xffffff00) | data;     return;  //FCF003
    //        // *** Might be able to ignore this, but might be needed to pass ROM MMU tests ***  // memory error address latch
    //        //  case 0x0800: return; //FCF800  // status register;
    //        default: return;
    //    }
}

void lisa_ww_Oxf000_memerror(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    bustimeout = 0;
    memerror = data;
}

void lisa_wl_Oxf000_memerror(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    bustimeout = 0;
    if ((addr & 0x000003) == 0)
        memerror = data;
    //    else
    //    {
    //        lisa_wb_Oxf000_memerror(addr,   (uint8)((data>>24)&0xff));
    //        lisa_wb_Oxf000_memerror(addr+1, (uint8)((data>>16)&0xff));
    //        lisa_wb_Oxf000_memerror(addr+2, (uint8)((data>> 8)&0xff));
    //        lisa_wb_Oxf000_memerror(addr+3, (uint8)(data)&0xff);
    //    }
}

uint8 *lisa_mptr_Oxf800_statreg(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_Oxf800_statreg got called!****");
    return NULL;
}

uint8 lisa_rb_Oxf800_statreg(uint32 addr); // fwd reference

#ifdef ANCIENT_DISCARD_ME
uint8 _ANCIENT_DISCARD_ME__lisa_rb_Oxf800_statreg(uint32 addr)
{

    /* From Schematic CPU page 3 Lower Right
     *

    StatReg
    U9E-LS244
     0 SF
     1 HD
     2 VTIR
     3 BUS
     4 VID
     5 CSYNC
     6 INVID              +5V
     7 +1 or Ground.      +5V
    */

#ifdef INVERSE_VIDEO
    uint8 status = STATREG_INVBIT;
#else
    uint8 status = 0;
#endif

#ifdef FULL_STATREG
    //  memory.c:lisa_rb_Oxf800_statreg:2008:
    //  cxdiff:9420 (x,y):(59,45) csync:32 minx,y:(0,-1) maxxy:(100,332)
    //  clk:931924332,vidscan:931914912
    //  maybe need to edit CXSTARTOFFSET for XY calc?  Maybe that would do the trick for CSYNC?  Maybe???
    //
    //  might also need to simulate this in another bit of code with cpu cycles going every 12 bytes
    //  this would probably be a better idea overall.  Likely what's happening is that 202 is a little too large
    //  but anything lower doesn't synchronize properly where csync is concerned because of the size of X.
    //  so if we switch to 101, might need to get rid of the X multiplier as well?
    //  but then we might go way over the number of cycles...
    //  so maybe steal a CPU cycle every so often?  this might help...  note maxy s/b 380, not 332.
    //  380/332 [1.14457831325]   so stealing one cpu cycle every so often might do the trick.
    //  380-332 [48] -

#define CYCLE_FACTOR 202  // 202=(100,392)[d3=379]   201=(100,393) [d3=381!!!so close!]   200=(99,395)[d3=383]further     203=maxxy:(101,389)[d3=377]  197=maxxy:(98,399)[d3=389]  //204=maxxy:(99,396)[D3=375]  200=maxxy:(99,395)  199=maxxy:(99,396) 202=maxxy(100,391) //CYCLES_PER_LINE //202=17b-d3 //was 101  //ef=190 ? s/b  //lower to raise d3 - higher to lower d3 //d3=14d. needs 17c     //was 202          //d3=17d need 17c, so need to eat the last one.
#define CXSTARTOFFSET 202 // 200 //202,200::maxxy(100,389):[d3=17b=379] //                   //CYCLES_PER_LINE
#define CSYNCCYCLEHIT 24
#define CSCYCLEEND 46870 // 59296

    int32 videox = 0,
          videoy = 0,
          cxdiff = (cpu68k_clocks - video_scan),
          cxdiffo = ((cpu68k_clocks - video_scan) - CXSTARTOFFSET);

    static int32 maxx, maxy, minx, miny;

    // 2005.05.02 hack     // correct timing.
    // cpu68k_clocks+=2; -- take this out! don't think it's right!

    // if (cxdiffo>(CYCLE_FACTOR*50))  cxdiffo+=10;
    // if (cxdiffo>(CYCLE_FACTOR*100)) cxdiffo+=10;
    if (cxdiffo > (CYCLE_FACTOR * 150))
        cxdiffo += 10;
    // if (cxdiffo>(CYCLE_FACTOR*200)) cxdiffo+=5;

    // with these settings we get to 382!
    q = ((cxdiffo & 2048)) ? CYCLE_FACTOR : CYCLE_FACTOR - 1; // 2048 is the one!!! 380!!! //1024=384! Even closer!  512=389   !512=389!! very close  !192=417-close but further   192=416-wrong direction 256=399 instead of 380
    videoy = ((cxdiffo / (q)));                               // should yield 0-379
    videox = ((cxdiffo % (q))) >> 1;                          // divided/2 yields 0-100

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (videox > maxx)
        maxx = videox;
    if (videoy > maxy)
        maxy = videoy;
    if (videox < minx && videox > -1)
        minx = videox;
    if (videoy < miny)
        miny = videoy;
    //////////////////////////////////////////////////////////////////////////////////////////////                        //89
    if (videox >= 0 && videox < (89 - (cxdiffo & 2)))
        status |= STATREG_CSYNC; // csync bit active=0, so when x>89, csync is on.

    DEBUG_LOG(100, "cxdiff:%ld (x,y):(%d,%d) csync:%d minx,y:(%d,%d) maxxy:(%d,%d) clk:%llx,vidscan:%ld a1:%08x",
              cxdiff, videox, videoy, status & STATREG_CSYNC, minx, miny, maxx, maxy, cpu68k_clocks, video_scan, getreg(9));

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // correct x,y coordinates so we can get the video bit value without going out of bounds.  also calc csync bit.

    if (has_xl_screen)
    {
        if (videox > 76)
            videox = 76;
        if (videox < 0)
            videox = 76;
        if (videoy < 0)
            videoy = 0;
        if (videoy > 431)
            videoy = 431;
    }
    else
    {
        if (videox > 89)
            videox = 89;
        if (videox < 0)
            videox = 89;
        if (videoy < 0)
            videoy = 0;
        if (videoy > 363)
            videoy = 363;
    }

    status |= (lisaram[videolatchaddress + ((videox + videoy * 90) & 32767)] & STATREG_VIDEOBIT);

#else
    status |= STATREG_VIDEOBIT;
#endif
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);

    status |= (softmemerror ? 0 : STATREG_SOFTMEM_ERR) | // reminder: statreg is backwards, so it's 0 when on, 1 when off.
              (parity_error_hit ? 0 : STATREG_HARDMEM_ERR) |
              (vertical ? 0 : STATREG_VERTICALRTC) |
              (bustimeout ? 0 : STATREG_BUSTIMEOUT);

    //    {                      status |= videobit|invvidbit|hsync;   }
    parity_error_hit = 0; // clear the parity error flag - do we do this now or when PAROFF happens?
    softmemerror = 0;
    DEBUG_LOG(100, "status:%02x :: (active low) bit0softerr:%d,1parity:%d,2vret:%d::%d,3bustmout:%d,4vidbit:%d,5hzsync:%d,6invbit:%d  maxx",
              status,
              status & STATREG_SOFTMEM_ERR,
              status & STATREG_HARDMEM_ERR,
              status & STATREG_VERTICALRTC, vertical,
              status & STATREG_BUSTIMEOUT,
              status & STATREG_VIDEOBIT,
              status & STATREG_HORIZONTAL,
              status & STATREG_INVBIT);

#ifdef DEBUG
    if (vertical && (status & STATREG_VERTICALRTC))
    {
        EXIT(1, 0, "DOH! vertical bug in memory.c!");
    }
#endif

    return status;
}

#endif

uint16 lisa_rw_Oxf800_statreg(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return LOCENDIAN16((uint16)((lisa_rb_Oxf800_statreg(addr) << 8) + lisa_rb_Oxf800_statreg(addr + 1)));
}

uint32 lisa_rl_Oxf800_statreg(uint32 addr)
{
    uint32 ret;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if ((addr & 0x00000003) == 0x0)
        return LOCENDIAN32((uint32)(lisa_rb_Oxf800_statreg(addr)));

    ret = (uint32)((lisa_rb_Oxf800_statreg(addr) << 24) +
                   (lisa_rb_Oxf800_statreg(addr + 1) << 16) +
                   (lisa_rb_Oxf800_statreg(addr + 2) << 8) +
                   (lisa_rb_Oxf800_statreg(addr + 3)));
    ret = LOCENDIAN32(ret);
    return ret;
}

void lisa_wb_Oxf800_statreg(uint32 addr, uint8 data)
{
    UNUSED(data);
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return; // this does nothing
}

void lisa_ww_Oxf800_statreg(uint32 addr, uint16 data)
{
    UNUSED(data);
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return; // this does nothing
}

void lisa_wl_Oxf800_statreg(uint32 addr, uint32 data)
{
    UNUSED(data);
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return; // this does nothing
}

/*********************************************************************\
* Lisa Memory Access functions.  These read/write the main memory in  *
* 8, 16, and 32 bit sizes.                                            *
*                                                                     *
* the lisa_??_ram fn's are proudly and shamelessly stolen from   *
* Generator like most of the CPU core, but I'm sure James won't mind. *
\*********************************************************************/

uint8 *lisa_mptr_ram(uint32 addr)
{
    HIGH_BYTE_FILTER();
    CHK_RAM_LIMITS(addr);
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    if (physaddr != -1)
        return (uint8 *)(&lisaram[physaddr]);
    else
        return NULL;
}

uint8 lisa_rb_ram(uint32 addr)
{

    IS_MMU_VALID_HERE();
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
        return (uint8)(lisaram[physaddr]);
    CPU_READ_MODE = 1;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);

    return 0x39;
}

uint16 lisa_rw_ram(uint32 addr)
{
    IS_MMU_VALID_HERE();
    HIGH_BYTE_FILTER();
    // 20061223 lisa_rw_ram is the most called, let's not check for this! //CHK_R_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
        return LOCENDIAN16(*(uint16 *)(&lisaram[physaddr]));

    // ALERT_LOG(0,"over/underflow at %08x",addr);
    CPU_READ_MODE = 1;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
    return 0x3939;
}

uint32 lisa_rl_ram(uint32 addr)
{
    IS_MMU_VALID_HERE();
    HIGH_BYTE_FILTER();
    CHK_R_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
        return LOCENDIAN32(*(uint32 *)(&lisaram[physaddr]));

    CPU_READ_MODE = 1;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
    // ALERT_LOG(0,"over/underflow at %08x",addr);
    return 0x39393939;
}

void lisa_wb_ram(uint32 addr, uint8 data)
{
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
    {
        *(uint8 *)(&lisaram[physaddr]) = data;
        return;
    }
    // ALERT_LOG(0,"over/underflow at %08x",addr);
    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_ww_ram(uint32 addr, uint16 data)
{
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    CHK_W_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
    {
        *(uint16 *)(&lisaram[physaddr]) = LOCENDIAN16(data);
        return;
    }
    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_wl_ram(uint32 addr, uint32 data)
{
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    CHK_W_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
    {
        *(uint32 *)(&lisaram[physaddr]) = LOCENDIAN32(data);
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

/*********************************************************************\
* Lisa Video Memory access fn's  These read/write the main memory in  *
* 8, 16, and 32 bit sizes and log writes indicating that the display  *
* has to be refreshed.  Identical to the _ram fn's except for writes  *
* which may call qplot if so compiled.                                *
\*********************************************************************/

// These are unnecessary as they're never called - only here for completion.
uint8 *lisa_mptr_vidram(uint32 addr) { return lisa_mptr_ram(addr); }
uint8 lisa_rb_vidram(uint32 addr) { return lisa_rb_ram(addr); }
uint16 lisa_rw_vidram(uint32 addr) { return lisa_rw_ram(addr); }
uint32 lisa_rl_vidram(uint32 addr) { return lisa_rl_ram(addr); }

void lisa_wb_vidram(uint32 addr, uint8 data)
{
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    videoramdirty++;

    {
        int x = ((addr & 32767) % 90) << 3, y = ((addr & 32767) / 90);
        dirty_x_min = MIN(dirty_x_min, x);
        dirty_x_max = MAX(dirty_x_max, x + 8);
        dirty_y_min = MIN(dirty_y_min, y);
        dirty_y_max = MAX(dirty_y_max, y);

#ifdef DEBUGVIDMEMWRITES
        DEBUG_LOG(0, "plotted byte at:%3d,%3d (pixel) @%08x %c%c%c%c%c%c%c%c %08x", x, y, addr,
                  (data & BIT7) ? '#' : ' ',
                  (data & BIT6) ? '#' : ' ',
                  (data & BIT5) ? '#' : ' ',
                  (data & BIT4) ? '#' : ' ',
                  (data & BIT3) ? '#' : ' ',
                  (data & BIT2) ? '#' : ' ',
                  (data & BIT1) ? '#' : ' ',
                  (data & BIT0) ? '#' : ' ', data);
#endif
    }

    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
    {
        *(uint8 *)(&lisaram[physaddr]) = data;
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_ww_vidram(uint32 addr, uint16 data)
{
    uint16 dat;
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    videoramdirty++;
    {
        int x = ((addr & 32767) % 90) << 3, y = ((addr & 32767) / 90);
        dirty_x_min = MIN(dirty_x_min, x);
        dirty_x_max = MAX(dirty_x_max, x + 16);
        dirty_y_min = MIN(dirty_y_min, y);
        dirty_y_max = MAX(dirty_y_max, y);
#ifdef DEBUGVIDMEMWRITES
        DEBUG_LOG(0, "plotted word at:%3d,%3d (pixel) %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c addr:%08x %04x", x, y,
                  (data & BIT15) ? '#' : ' ',
                  (data & BIT14) ? '#' : ' ',
                  (data & BIT13) ? '#' : ' ',
                  (data & BIT12) ? '#' : ' ',
                  (data & BIT11) ? '#' : ' ',
                  (data & BIT10) ? '#' : ' ',
                  (data & BIT9) ? '#' : ' ',
                  (data & BIT8) ? '#' : ' ',
                  (data & BIT7) ? '#' : ' ',
                  (data & BIT6) ? '#' : ' ',
                  (data & BIT5) ? '#' : ' ',
                  (data & BIT4) ? '#' : ' ',
                  (data & BIT3) ? '#' : ' ',
                  (data & BIT2) ? '#' : ' ',
                  (data & BIT1) ? '#' : ' ',
                  (data & BIT0) ? '#' : ' ', addr, data);
#endif
    }

    CHK_W_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    if (physaddr > -1)
    {
        *(uint16 *)(&lisaram[physaddr]) = LOCENDIAN16(data);
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_wl_vidram(uint32 addr, uint32 data)
{
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    videoramdirty++;

    {
        int x = ((addr & 32767) % 90) << 3, y = ((addr & 32767) / 90);
        dirty_x_min = MIN(dirty_x_min, x);
        dirty_x_max = MAX(dirty_x_max, x + 32);
        dirty_y_min = MIN(dirty_y_min, y);
        dirty_y_max = MAX(dirty_y_max, y);
#ifdef DEBUGVIDMEMWRITES
        DEBUG_LOG(0, "plotted long at:%3d,%3d @%08x : %08x (pixel) %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", x, y, addr, data,
                  (data & BIT31) ? '#' : ' ',
                  (data & BIT30) ? '#' : ' ',
                  (data & BIT29) ? '#' : ' ',
                  (data & BIT28) ? '#' : ' ',
                  (data & BIT27) ? '#' : ' ',
                  (data & BIT26) ? '#' : ' ',
                  (data & BIT25) ? '#' : ' ',
                  (data & BIT24) ? '#' : ' ',
                  (data & BIT23) ? '#' : ' ',
                  (data & BIT22) ? '#' : ' ',
                  (data & BIT21) ? '#' : ' ',
                  (data & BIT20) ? '#' : ' ',
                  (data & BIT19) ? '#' : ' ',
                  (data & BIT18) ? '#' : ' ',
                  (data & BIT17) ? '#' : ' ',
                  (data & BIT16) ? '#' : ' ',
                  (data & BIT15) ? '#' : ' ',
                  (data & BIT14) ? '#' : ' ',
                  (data & BIT13) ? '#' : ' ',
                  (data & BIT12) ? '#' : ' ',
                  (data & BIT11) ? '#' : ' ',
                  (data & BIT10) ? '#' : ' ',
                  (data & BIT9) ? '#' : ' ',
                  (data & BIT8) ? '#' : ' ',
                  (data & BIT7) ? '#' : ' ',
                  (data & BIT6) ? '#' : ' ',
                  (data & BIT5) ? '#' : ' ',
                  (data & BIT4) ? '#' : ' ',
                  (data & BIT3) ? '#' : ' ',
                  (data & BIT2) ? '#' : ' ',
                  (data & BIT1) ? '#' : ' ',
                  (data & BIT0) ? '#' : ' ');
#endif
    }

    CHK_W_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
    {
        *(uint32 *)(&lisaram[physaddr]) = LOCENDIAN32(data);
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

/*-----------------------------------------------------------------------------
  Lisa Mac XL with Screen mod kit - 608x431 is the screen resolution!
  608/8=76 bytes/line instead of 90.  431*76=32756 bytes.
  ----------------------------------------------------------------------------*/

void lisa_wb_xlvidram(uint32 addr, uint8 data)
{
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    videoramdirty++;

    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    {
        int x = ((addr & 32767) % 76) << 3, y = ((addr & 32767) / 76);
        dirty_x_min = MIN(dirty_x_min, x);
        dirty_x_max = MAX(dirty_x_max, x + 8);
        dirty_y_min = MIN(dirty_y_min, y);
        dirty_y_max = MAX(dirty_y_max, y);
#ifdef DEBUGVIDMEMWRITES
        DEBUG_LOG(0, "plotted byte at:%3d,%3d @%08x : %08x (pixel) %c%c%c%c%c%c%c%c", x, y, addr, data,
                  (data & BIT7) ? '#' : ' ',
                  (data & BIT6) ? '#' : ' ',
                  (data & BIT5) ? '#' : ' ',
                  (data & BIT4) ? '#' : ' ',
                  (data & BIT3) ? '#' : ' ',
                  (data & BIT2) ? '#' : ' ',
                  (data & BIT1) ? '#' : ' ',
                  (data & BIT0) ? '#' : ' ');
#endif
    }

    if (physaddr > -1)
    {
        *(uint8 *)(&lisaram[physaddr]) = data;
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_ww_xlvidram(uint32 addr, uint16 data)
{
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    videoramdirty++;

    CHK_W_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    {
        int x = ((addr & 32767) % 76) << 3, y = ((addr & 32767) / 76);
        dirty_x_min = MIN(dirty_x_min, x);
        dirty_x_max = MAX(dirty_x_max, x + 16);
        dirty_y_min = MIN(dirty_y_min, y);
        dirty_y_max = MAX(dirty_y_max, y);
#ifdef DEBUGVIDMEMWRITES
        DEBUG_LOG(0, "plotted word at:%3d,%3d (pixel) %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c addr:%08x %04x", x, y,
                  (data & BIT15) ? '#' : ' ',
                  (data & BIT14) ? '#' : ' ',
                  (data & BIT13) ? '#' : ' ',
                  (data & BIT12) ? '#' : ' ',
                  (data & BIT11) ? '#' : ' ',
                  (data & BIT10) ? '#' : ' ',
                  (data & BIT9) ? '#' : ' ',
                  (data & BIT8) ? '#' : ' ',
                  (data & BIT7) ? '#' : ' ',
                  (data & BIT6) ? '#' : ' ',
                  (data & BIT5) ? '#' : ' ',
                  (data & BIT4) ? '#' : ' ',
                  (data & BIT3) ? '#' : ' ',
                  (data & BIT2) ? '#' : ' ',
                  (data & BIT1) ? '#' : ' ',
                  (data & BIT0) ? '#' : ' ', addr, data);
#endif
    }
    if (physaddr > -1)
    {
        *(uint16 *)(&lisaram[physaddr]) = LOCENDIAN16(data);
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_wl_xlvidram(uint32 addr, uint32 data)
{
    HIGH_BYTE_FILTER();
    INVALIDATE_IPC();
    IS_MMU_VALID_HERE();
    videoramdirty++;

    CHK_W_ODD_ADR(addr);
    CHK_RAM_LIMITS(addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    {
        int x = ((addr & 32767) % 76) << 3, y = ((addr & 32767) / 76);
        dirty_x_min = MIN(dirty_x_min, x);
        dirty_x_max = MAX(dirty_x_max, x + 32);
        dirty_y_min = MIN(dirty_y_min, y);
        dirty_y_max = MAX(dirty_y_max, y);
#ifdef DEBUGVIDMEMWRITES
        DEBUG_LOG(0, "plotted long at:%3d,%3d @%08x : %08x (pixel) %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", x, y, addr, data,
                  (data & BIT31) ? '#' : ' ',
                  (data & BIT30) ? '#' : ' ',
                  (data & BIT29) ? '#' : ' ',
                  (data & BIT28) ? '#' : ' ',
                  (data & BIT27) ? '#' : ' ',
                  (data & BIT26) ? '#' : ' ',
                  (data & BIT25) ? '#' : ' ',
                  (data & BIT24) ? '#' : ' ',
                  (data & BIT23) ? '#' : ' ',
                  (data & BIT22) ? '#' : ' ',
                  (data & BIT21) ? '#' : ' ',
                  (data & BIT20) ? '#' : ' ',
                  (data & BIT19) ? '#' : ' ',
                  (data & BIT18) ? '#' : ' ',
                  (data & BIT17) ? '#' : ' ',
                  (data & BIT16) ? '#' : ' ',
                  (data & BIT15) ? '#' : ' ',
                  (data & BIT14) ? '#' : ' ',
                  (data & BIT13) ? '#' : ' ',
                  (data & BIT12) ? '#' : ' ',
                  (data & BIT11) ? '#' : ' ',
                  (data & BIT10) ? '#' : ' ',
                  (data & BIT9) ? '#' : ' ',
                  (data & BIT8) ? '#' : ' ',
                  (data & BIT7) ? '#' : ' ',
                  (data & BIT6) ? '#' : ' ',
                  (data & BIT5) ? '#' : ' ',
                  (data & BIT4) ? '#' : ' ',
                  (data & BIT3) ? '#' : ' ',
                  (data & BIT2) ? '#' : ' ',
                  (data & BIT1) ? '#' : ' ',
                  (data & BIT0) ? '#' : ' ');
#endif
    }
    if (physaddr > -1)
    {
        *(uint32 *)(&lisaram[physaddr]) = LOCENDIAN32(data);
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

// Yeah, yeah, there's no such thing as a read only violation on a read.  I know.
// These are here for "completeness" it would be far too ugly to separate things out
// and wouldn't buy us anything back.  So I put these three worthless fn's in, they'll
// never be called.

uint8 *lisa_mptr_ro_violn(uint32 addr) { return lisa_mptr_ram(addr); }
uint8 lisa_rb_ro_violn(uint32 addr) { return lisa_rb_vidram(addr); }
uint16 lisa_rw_ro_violn(uint32 addr) { return lisa_rw_vidram(addr); }
uint32 lisa_rl_ro_violn(uint32 addr) { return lisa_rl_vidram(addr); }

#ifdef ROMEMCAUSESBUSERROR
void lisa_wb_ro_violn(uint32 addr, uint8 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CPU_READ_MODE = 1;
    lisa_mmu_exception(addr);
    UNUSED(data);
}
void lisa_ww_ro_violn(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_W_ODD_ADR(addr);
    CPU_READ_MODE = 0;
    lisa_mmu_exception(addr);
    UNUSED(data);
}
void lisa_wl_ro_violn(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_W_ODD_ADR(addr);
    CPU_READ_MODE = 0;
    lisa_mmu_exception(addr);
    UNUSED(data);
}
#else
void lisa_wb_ro_violn(uint32 addr, uint8 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CPU_READ_MODE = 1;
    UNUSED(data);
}
void lisa_ww_ro_violn(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_W_ODD_ADR(addr);
    CPU_READ_MODE = 0;
    UNUSED(data);
}
void lisa_wl_ro_violn(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_W_ODD_ADR(addr);
    CPU_READ_MODE = 0;
    UNUSED(data);
}
#endif

uint8 *lisa_mptr_bad_page(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return NULL;
}
uint8 lisa_rb_bad_page(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CPU_READ_MODE = 1;
    if (abort_opcode != 2)
        lisa_mmu_exception(addr);
    return 0;
}
uint16 lisa_rw_bad_page(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_R_ODD_ADR(addr);
    CPU_READ_MODE = 1;
    if (abort_opcode != 2)
        lisa_mmu_exception(addr);
    return 0;
}
uint32 lisa_rl_bad_page(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_R_ODD_ADR(addr);
    CPU_READ_MODE = 1;
    if (abort_opcode != 2)
        lisa_mmu_exception(addr);
    return 0;
}
void lisa_wb_bad_page(uint32 addr, uint8 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CPU_READ_MODE = 0;
    if (abort_opcode != 2)
        lisa_mmu_exception(addr);
    UNUSED(data);
}
void lisa_ww_bad_page(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_W_ODD_ADR(addr);
    CPU_READ_MODE = 0;
    if (abort_opcode != 2)
        lisa_mmu_exception(addr);
    UNUSED(data);
}
void lisa_wl_bad_page(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    CHK_W_ODD_ADR(addr);
    CPU_READ_MODE = 0;
    if (abort_opcode != 2)
        lisa_mmu_exception(addr);
    UNUSED(data);
}

// Do reads of the ROM
uint8 *lisa_mptr_sio_rom(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return (uint8 *)(&lisarom[addr & 0x3fff]);
}
uint8 lisa_rb_sio_rom(uint32 addr)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return lisarom[addr & 0x3fff];
}

uint16 lisa_rw_sio_rom(uint32 addr)
{
    register uint16 val;
    register uint16 *ptr;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    addr = (addr & 0x3fff);
    CHK_R_ODD_ADR(addr);
    ptr = (uint16 *)(&lisarom[addr]);
    val = *ptr;
    return LOCENDIAN16(val);
} // CHK_R_ODD_ADR(addr);//(uint16)((lisarom[addr])<<8) | (lisarom[addr+1]);}

uint32 lisa_rl_sio_rom(uint32 addr)
{
    register uint32 val;
    register uint32 *ptr;

    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    addr = (addr & 0x3fff);
    CHK_R_ODD_ADR(addr);
    ptr = (uint32 *)(&lisarom[addr]);
    val = *ptr;
    return LOCENDIAN32(val);
} //((lisarom[addr])<<24) | (lisarom[ad)dr+1]<<16) | ((lisarom[addr+2])<<8)|(lisarom[addr+3]);}

// Ingore writes to ROM - log for debug only
void lisa_wb_sio_rom(uint32 addr, uint8 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "byte write to ROM @%08x", addr);
    UNUSED(data);
}
void lisa_ww_sio_rom(uint32 addr, uint16 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "word write to ROM @%08x", addr);
    CHK_W_ODD_ADR(addr);
    UNUSED(data);
}
void lisa_wl_sio_rom(uint32 addr, uint32 data)
{
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "long write to ROM @%08x", addr);
    CHK_W_ODD_ADR(addr);
    UNUSED(data);
}

uint8 *lisa_mptr_sio_mrg(uint32 addr)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // CHK_R_ODD_ADR(addr);
    // ui_log_verbose("*** lisa_mptr_sio_mrg got called! ****");
    return NULL;
}

uint8 lisa_rb_sio_mrg(uint32 addr)
{
    uint16 r;
    uint8 r1, r2;
    uint32 a = GETSEG(addr);
    uint16 con = CXASEL;
    addr &= ADDRESSFILT;

    CHECK_DIRTY_MMU(addr);

    if (addr & 8) // Segment Origin Register
    {
        DEBUG_LOG(100, "mmu_all[%d][%d].sor=%04x chg:%d", con, a, mmu_all[con][a].sor, mmu_all[con][a].changed);
        r = mmu_all[con][a].sor;
        if ((addr & 1) == 0)
            r = r | (getsnbit() >> 8);
        r1 = r >> 8;
        r2 = r & 0xff;
    }
    else // Segment Limit Register
    {
        DEBUG_LOG(100, "mmu_all[%d][%d].slr=%04x chg:%d", con, a, mmu_all[con][a].slr, mmu_all[con][a].changed);
        r = mmu_all[con][a].slr;
        if ((addr & 1) == 0)
            r = r | (getsnbit() >> 8);
        r1 = r >> 8;
        r2 = r & 0xff;
    }
    // weird read, have to pick out high or low byte
    return (addr & 1) ? r2 : r1;
}

uint16 lisa_rw_sio_mrg(uint32 addr)
{
    uint32 a = GETSEG(addr);
    uint16 con = CXASEL;

    addr &= ADDRESSFILT;

    CHK_R_ODD_ADR(addr);

    DEBUG_LOG(100, "@%08x", addr);

    if (addr & 8) // Segment Origin Register
    {
        DEBUG_LOG(100, "SRC: sor read: mmu_all[%d][%d].slr=%04x .sor:%04x changed=%d", con, a, mmu_all[con][a].slr, mmu_all[con][a].sor, mmu_all[con][a].changed);
        DEBUG_LOG(100, "will return: %04x | serno", (mmu_all[con][a].sor & 0xfff));

        return ((mmu_all[con][a].sor & 0xfff) | getsnbit());
    }
    else // Segment Limit Register
    {
        DEBUG_LOG(100, "SRC: slr read: mmu_all[%d][%d].slr=%04x .sor:%04x changed=%d", con, a, mmu_all[con][a].slr, mmu_all[con][a].sor, mmu_all[con][a].changed);
        DEBUG_LOG(100, "will return: %04x | serno", (mmu_all[con][a].slr & 0xfff));

        return ((mmu_all[con][a].slr & 0xfff) | getsnbit());
    }
}

uint32 lisa_rl_sio_mrg(uint32 addr) { return (lisa_rw_sio_mrg(addr) << 16) | lisa_rw_sio_mrg(addr + 2); }

void lisa_wb_sio_mrg(uint32 addr, uint8 data)
{ // these should never happen - but just in case.
    uint32 a;
    uint16 r;
    uint8 r2; // r1
    uint16 con = CXASEL;
    addr &= ADDRESSFILT;

    CHECK_DIRTY_MMU(addr);
    a = GETSEG(addr);
    if (addr & 8) // Segment Origin Register
    {
        r = mmu_all[con][a].sor; // r1=r>>8;
        r2 = r & 0xff;
        if (addr & 1)
            r = (r & 0xff00) | (data);
        else
            r = (data << 8) | r2;
        if (mmu_all[con][a].sor == r)
            return; // don't bother wasting time on writing back the same value

        DEBUG_LOG(100, "mmu_all[%d][%d].sor=%04x chg:%d", con, a, mmu_all[con][a].sor, mmu_all[con][a].changed);
        // Shadow the write to context 0 (context 0 is our START mode, context 1 is the real lisa context 0)
        if (con)
        {
            mmu_all[0][a].sor = r;
            mmu_all[0][a].changed |= 2;
        }
        mmu_all[con][a].sor = r;
        mmu_all[con][a].changed |= 2;

        // #ifdef DEBUG
        //   dumpmmupage(con,a,buglog);
        // #endif

        /* If the MMU is already dirty and it's a different register set, MMUDIRTY will be >128 since
           we shift the last mmudirty to the left 8 bits.  This situation then indicates that we need
           to flush all segments with mmu_all[con][seg].changed=1 in the current context (discounting START mode)

           We do a+1 since segment 0 would be dirty, but need to set mmudirty to nonzero.

           And we don't flush the mmu changes until we either change contexts, or we need to use
           the MMU map while in special I/O mode.  This way we can be a bit lazy and avoid needless
           MMU map recalculations.
        */

        GET_MMU_DIRTY(0);
        if ((a + 1) != mmudirty)
        {
            SET_MMU_DIRTY((mmudirty << 8) | (a + 1));
            if (context)
                mmuflush(0);
        }
        return;
    }
    else // Segment Limit Register
    {
        DEBUG_LOG(100, "mmu_all[%d][%d].slr=%04x chg:%d", con, a, mmu_all[con][a].slr, mmu_all[con][a].changed);
        r = mmu_all[con][a].slr; // r1=r>>8;
        r2 = r & 0xff;
        if (addr & 1)
            r = (r & 0xff00) | (data);
        else
            r = (data << 8) | r2;
        if (mmu_all[con][a].slr == r)
            return; // don't bother wasting time on writing back the same value

        if (con)
        {
            mmu_all[0][a].slr = r;
            mmu_all[0][a].changed |= 1;
        }
        mmu_all[con][a].slr = r;
        mmu_all[con][a].changed |= 1;

        // must correct mmu here!
        GET_MMU_DIRTY(0);
        if ((a + 1) != mmudirty)
        {
            SET_MMU_DIRTY((mmudirty << 8) | (a + 1));
            if (context)
                mmuflush(0);
        }
        return;
    }
}

void lisa_ww_sio_mrg(uint32 addr, uint16 data)
{
    uint32 a = GETSEG(addr);
    uint16 con = CXASEL;
    addr &= ADDRESSFILT;

    CHK_W_ODD_ADR(addr);

    if (addr & 8) // Segment Origin Register
    {
        data &= 0x0fff;
        if (data == mmu_all[con][a].sor)
        {
            DEBUG_LOG(100, "mmu[%d][%d].sor no change needed ", con, a);
            return;
        } // don't bother wasting time here - sync new just incase.

        // ALERT_LOG(0,"Wrote %03x to mmu[%d][%d].sor addr=%08x",data,context,a,addr);

        // Shadow the write to context 0 and 1 (context 0 is our START mode, context 1 is the real lisa context 0)
        if (con == 1)
        {
            mmu_all[0][a].sor = data;
            mmu_all[0][a].changed |= 2;
        }
        mmu_all[con][a].sor = data;
        mmu_all[con][a].changed |= 2;

        GET_MMU_DIRTY(0);
        if ((a + 1) != mmudirty)
        {
            SET_MMU_DIRTY((mmudirty << 8) | (a + 1));
            if (context)
                mmuflush(0);
        }
#ifdef DEBUG
        {
            int16 pagestart, pageend;
            lisa_mem_t rfn, wfn;

            get_slr_page_range(con, a, &pagestart, &pageend, &rfn, &wfn);
            DEBUG_LOG(100, "sor write mmu[%d][%d].slr=%04x (%s) .sor:%04x, data:%04x pagestart-end:%08x,%08x r/wfn:%d,%d",
                      con, a,
                      mmu_all[con][a].slr, slrname(mmu_all[con][a].slr),
                      mmu_all[con][a].sor, data, (uint32)(pagestart << 9), (uint32)(pageend << 9), rfn, wfn);
        }
#endif
        return;
    }
    else // Segment Limit Register
    {
        data &= 0x0fff;

        // ALERT_LOG(0,"Wrote %03x to mmu[%d][%d].slr addr=%08x",data,context,a,addr);

        if (data == mmu_all[con][a].slr)
        {
            DEBUG_LOG(100, "no change to slr: mmu_all[%d][%d].slr=%04x changed=%d data=%04x",
                      con, a,
                      mmu_all[con][a].slr,
                      mmu_all[con][a].changed,
                      data);
            return;
        } // don't bother wasting time here - sync new just incase.

        // context=1 shadows context 0
        if (con == 1)
        {
            mmu_all[0][a].slr = data;
            mmu_all[0][a].changed |= 1;
        }
        mmu_all[con][a].slr = data;
        mmu_all[con][a].changed |= 1;

        /// correct MMU here!

        GET_MMU_DIRTY(0);
        if ((a + 1) != mmudirty)
        {
            SET_MMU_DIRTY((mmudirty << 8) | (a + 1));
            if (context)
                mmuflush(0);
        }

#ifdef DEBUG
        {
            int16 pagestart, pageend;
            lisa_mem_t rfn, wfn;

            get_slr_page_range(con, a, &pagestart, &pageend, &rfn, &wfn);
            DEBUG_LOG(100, "slr write mmu[%d][%d].slr=%04x (%s) .sor:%04x, data:%04x pagestart-end:%08x,%08x r/wfn:%d,%d",
                      con, a,
                      mmu_all[con][a].slr, slrname(mmu_all[con][a].slr),
                      mmu_all[con][a].sor, data, (uint32)(pagestart << 9), (uint32)(pageend << 9), rfn, wfn);
        }
#endif

        return;
    }
}

void lisa_wl_sio_mrg(uint32 addr, uint32 data)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // cheat a little here, why not, it would get too gnarly otherwise...  Besides these should be very rare.
    CHK_W_ODD_ADR(addr);
    lisa_ww_sio_mrg(addr, data >> 16);
    lisa_ww_sio_mrg(addr + 2, data & 0x0000ffff);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Used in SIO/Start Mode to do accesses via the MMU map, but disables start bit filtering.

uint8 *lisa_mptr_sio_mmu(uint32 addr)
{
    uint32 a9 = GETEPAGE(addr);
    uint32 a17 = GETSEG(addr);
    uint16 con = CXSEL;
    lisa_mem_t f;
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);

    GET_MMUS_DIRTY(x);
    if (mmudirty && context)
        mmuflush(0);

    DEBUG_LOG(5, "postflush address=%08x, s1/s2=%d/%d context=%d, mycontext=%d rfn=%02d,wfn=%02d,addrmod=%08x", addr, segment1, segment2, context, con,
              mmu_trans_all[con][a9].readfn, mmu_trans_all[con][a9].writefn,
              mmu_trans_all[con][a9].address);

    ///// changed context here to con!!!!!
    f = rmmuslr2fn(mmu_all[context][a17].slr, a9); /// *** CONTEXT or CON????
    DEBUG_LOG(3, "fetching mmu function called: %d %s con:%d slr=%04x @%08x", f, mspace(f), con, mmu_all[con][a17].slr, addr);

    if (f != ram && f != vidram)
        return mem68k_memptr[f](addr);

    DEBUG_LOG(100, "ram sio from %d/%08x", (con), CHK_MMU_A_REGST((con), addr));
    return (uint8 *)(RAM_MMU_A_TRANS((con), addr));
}

uint8 lisa_rb_sio_mmu(uint32 addr)
{
    uint32 a9 = GETEPAGE(addr);
    uint32 a17 = GETSEG(addr);
    uint16 con = CXSEL;
    lisa_mem_t f;
    addr &= ADDRESSFILT;

    //    DEBUG_LOG(100,"@%08x",addr);

    //    DEBUG_LOG(100,"preflush addr=%08x, s1/s2=%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    GET_MMUS_DIRTY(x);
    if (mmudirty)
        mmuflush(0);

    //    DEBUG_LOG(100,"post addr=%08x, s1/s2=%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    ///// changed context here to con!!!!!
    f = rmmuslr2fn(mmu_all[context][a17].slr, a9); /// *** CONTEXT or CON????

    //    DEBUG_LOG(100,"fetching mmu function called: %d %s seg1/seg2/start:%d/%d/%d con:%d context%d slr=%04x",f,mspace(f),
    //            segment1,segment2,start,
    //            con,context,mmu_all[con][a17].slr);

    if (f != ram && f != vidram)
        return mem68k_fetch_byte[f](addr);
    //    DEBUG_LOG(100,"ram sio from %d/%08x",(con),CHK_MMU_A_REGST((con),addr));

    CHK_RAM_A_LIMITS(con, addr);
    if (physaddr > -1)
        return (uint8)(lisaram[physaddr]);

    CPU_READ_MODE = 1;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);

    return 0x93;
}

uint16 lisa_rw_sio_mmu(uint32 addr)
{
    uint32 a9 = GETEPAGE(addr);
    uint32 a17 = GETSEG(addr);
    uint16 con = CXSEL;
    lisa_mem_t f;
    addr &= ADDRESSFILT;

    // DEBUG_LOG(100,"@%08x",addr);
    //
    // CHK_R_ODD_ADR(addr);           //RISKY! 20061223
    //
    // DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //     addr,segment1,segment2,start,context,con,
    //     mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //     mmu_trans_all[con][a9].address,a17,
    //     mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //     mmu_all[con][a17].changed);

    GET_MMUS_DIRTY(x);
    if (mmudirty)
        mmuflush(0);

    // DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //     addr,segment1,segment2,start,context,con,
    //     mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //     mmu_trans_all[con][a9].address,a17,
    //     mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //     mmu_all[con][a17].changed);

    ///// changed context here to con!!!!!
    f = rmmuslr2fn(mmu_all[context][a17].slr, a9); /// *** CONTEXT or CON????

    //    DEBUG_LOG(3,"fetching mmu function called: %d %s con:%d slr=%04x",f,mspace(f),con,mmu_all[con][a17].slr);

    if (f != ram && f != vidram)
        return mem68k_fetch_word[f](addr);
    //    DEBUG_LOG(100,"ram sio from %d/%08x",(con),CHK_MMU_A_REGST((con),addr));

    CHK_RAM_A_LIMITS(con, addr);
    if (physaddr > -1)
        return LOCENDIAN16(*(uint16 *)(&lisaram[physaddr]));

    CPU_READ_MODE = 1;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);

    return 0x9393;
}

uint32 lisa_rl_sio_mmu(uint32 addr)
{
    uint32 a9 = GETEPAGE(addr);
    uint32 a17 = GETSEG(addr);
    uint16 con = CXSEL;

    lisa_mem_t f;
    addr &= ADDRESSFILT;

    DEBUG_LOG(100, "@%08x", addr);
    CHK_R_ODD_ADR(addr);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    GET_MMUS_DIRTY(x);
    if (mmudirty)
        mmuflush(0);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    ///// changed context here to con!!!!!
    f = rmmuslr2fn(mmu_all[context][a17].slr, a9); /// *** CONTEXT or CON????

    //    DEBUG_LOG(3,"fetching mmu function called: %d %s con:%d slr=%04x",f,mspace(f),con,mmu_all[con][a17].slr);

    if (f != ram && f != vidram)
        return mem68k_fetch_long[f](addr);
    //    DEBUG_LOG(100,"ram sio from %08x",CHK_MMU_A_REGST((con),addr));

    CHK_RAM_A_LIMITS(con, addr);

    if (physaddr > -1)
        return LOCENDIAN32(*(uint32 *)(&lisaram[physaddr]));

    CPU_READ_MODE = 1;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);

    return 0x93939393;
}

void lisa_wb_sio_mmu(uint32 addr, uint8 data)
{
    uint32 a9 = GETEPAGE(addr);
    uint32 a17 = GETSEG(addr);
    uint16 con = CXSEL;
    lisa_mem_t f;

    addr &= ADDRESSFILT;

    //    DEBUG_LOG(100,"@%08x",addr);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    GET_MMUS_DIRTY(x);
    if (mmudirty)
        mmuflush(0);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x slr=%04x chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    ///// changed context here to con!!!!!
    f = wmmuslr2fn(mmu_all[context][a17].slr, a9); /// *** CONTEXT or CON????
                                                   //    DEBUG_LOG(3,"storing mmu function called: %d %s con:%d slr=%04x",f,mspace(f),con,mmu_all[context][a17].slr);

    if (f != ram && f != vidram)
    {
        mem68k_store_byte[f](addr, data);
        return;
    }

    videoramdirty++;

    CHK_RAM_A_LIMITS(con, addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
    {
        lisaram[physaddr] = data;
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_ww_sio_mmu(uint32 addr, uint16 data)
{
    uint32 a9 = GETEPAGE(addr);
    uint32 a17 = GETSEG(addr);
    uint16 con = CXSEL;
    lisa_mem_t f;
    addr &= ADDRESSFILT;
    //  DEBUG_LOG(100,"@%08x",addr);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    // Flush MMU cache if needed.
    GET_MMUS_DIRTY(x);
    if (mmudirty)
        mmuflush(0);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    ///// changed context here to con!!!!!
    f = wmmuslr2fn(mmu_all[context][a17].slr, a9); /// *** CONTEXT or CON????

    //    DEBUG_LOG(3,"storing mmu function called: %d %s con:%d slr=%04x",f,mspace(f),con,mmu_all[con][a17].slr);

    if (f != ram && f != vidram)
    {
        mem68k_store_word[f](addr, data);
        return;
    }
    //    DEBUG_LOG(100,"ram sio from %d/%08x",(con),CHK_MMU_A_REGST((con),addr));

    CHK_W_ODD_ADR(addr);
    CHK_RAM_A_LIMITS(con, addr);
    if (physaddr > -1)
    {
        *(uint16 *)(&lisaram[physaddr]) = LOCENDIAN16(data);
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

void lisa_wl_sio_mmu(uint32 addr, uint32 data)
{
    uint32 a9 = GETEPAGE(addr);
    uint32 a17 = GETSEG(addr);
    uint16 con = CXSEL;
    lisa_mem_t f;
    addr &= ADDRESSFILT;

    //    DEBUG_LOG(100,"@%08x",addr);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    GET_MMUS_DIRTY(x);
    if (mmudirty)
        mmuflush(0);

    //    DEBUG_LOG(5,"post addr=%08x, s1/s2/start=%d/%d/%d context=%d, con=%d r=%s,w=%s,ea=%08x mmu[%d].sor=%04x,slr=%04x,chg:%d\n",
    //        addr,segment1,segment2,start,context,con,
    //        mspace(mmu_trans_all[con][a9].readfn),mspace(mmu_trans_all[con][a9].writefn),
    //        mmu_trans_all[con][a9].address,a17,
    //        mmu_all[con][a17].sor,mmu_all[con][a17].slr,
    //        mmu_all[con][a17].changed);

    ///// changed context here to con!!!!!
    f = wmmuslr2fn(mmu_all[context][a17].slr, a9); /// *** CONTEXT or CON????

    //    DEBUG_LOG(3,"storing mmu function called: %d %s con:%d slr=%04x",f,mspace(f),con,mmu_all[context][a17].slr);

    if (f != ram && f != vidram)
    {
        mem68k_store_long[f](addr, data);
        return;
    }
    //    DEBUG_LOG(100,"ram sio from %d/%08x",(con),CHK_MMU_A_REGST((con),addr));

    CHK_W_ODD_ADR(addr);
    CHK_RAM_A_LIMITS(con, addr);
    // DEBUG_LOG(100,"mmu translation of %d/%08x is: %08x",context,addr,physaddr);
    if (physaddr > -1)
    {
        *(uint32 *)(&lisaram[physaddr]) = LOCENDIAN32(data);
        return;
    }

    CPU_READ_MODE = 0;
    CHK_PHYS_OFLOW(addr);
    CHK_PHYS_UFLOW(addr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// These are probably not needed, but might be useful to debug.

uint8 *lisa_mptr_io(uint32 addr)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    // ui_log_verbose("****lisa_mptr_io got called!****"); //return NULL;
    return mem68k_memptr[io_map[(addr & 0xffff) >> 9]](addr);
}

uint8 lisa_rb_io(uint32 addr)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return mem68k_fetch_byte[io_map[(addr & 0xffff) >> 9]](addr);
}

uint16 lisa_rw_io(uint32 addr)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return mem68k_fetch_word[io_map[(addr & 0xffff) >> 9]](addr);
}

uint32 lisa_rl_io(uint32 addr)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    return mem68k_fetch_long[io_map[(addr & 0xffff) >> 9]](addr);
}

void lisa_wb_io(uint32 addr, uint8 data)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    mem68k_store_byte[io_map[(addr & 0xffff) >> 9]](addr, data);
}

void lisa_ww_io(uint32 addr, uint16 data)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    mem68k_store_word[io_map[(addr & 0xffff) >> 9]](addr, data);
}

void lisa_wl_io(uint32 addr, uint32 data)
{
    addr &= ADDRESSFILT;
    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);
    DEBUG_LOG(100, ": %08x<-%08x indx:%d iomap[]=%d", addr, data, (addr & 0xffff) >> 9, io_map[(addr & 0xffff) >> 9]);

    mem68k_store_long[io_map[(addr & 0xffff) >> 9]](addr, data);
}

void lisa_parity_error(uint32 addr)
{
    DEBUG_LOG(100, "Boink! Parity error detected! @ %08x", addr);
    parity_error_hit = 1;
    lisa_nmi_vector(addr);
}

void lisa_hardmem_error(uint32 addr)
{
    DEBUG_LOG(100, "Boink! Hardmem Parity error detected! @ %08x", addr);
    parity_error_hit = -1; // magic flag that says, don't bother checking the pairty array + informs statreg too
    lisa_nmi_vector(addr);
}

void lisa_softmem_error(uint32 addr)
{
    DEBUG_LOG(100, "Boink! SoftMem Parity error detected! @ %08x", addr);
    softmemerror = 1;
    lisa_nmi_vector(addr);
}

// Parity Memory Checking/Setting wrapper functions
// junk return
uint8 lisa_rb_ram_parity(uint32 addr)
{
    DEBUG_LOG(100, "read parity %08x", addr);
    if (parity_check(addr))
    {
        lisa_parity_error(addr);
        return 0xff;
    }
    else
        return lisa_rb_ram(addr);
}
uint16 lisa_rw_ram_parity(uint32 addr)
{
    DEBUG_LOG(100, "read parity %08x", addr);
    if (parity_check(addr))
    {
        lisa_parity_error(addr);
        return 0xffff;
    }
    else
        return lisa_rw_ram(addr);
}
uint32 lisa_rl_ram_parity(uint32 addr)
{
    DEBUG_LOG(100, "read parity %08x", addr);
    if (parity_check(addr))
    {
        lisa_parity_error(addr);
        return 0xffffffff;
    }
    else
        return lisa_rl_ram(addr);
}

uint8 lisa_rb_vidram_parity(uint32 addr)
{
    DEBUG_LOG(100, "read parity %08x", addr);
    if (parity_check(addr))
    {
        lisa_parity_error(addr);
        return 0xff;
    }
    else
        return lisa_rb_vidram(addr);
}
uint16 lisa_rw_vidram_parity(uint32 addr)
{
    DEBUG_LOG(100, "read parity %08x", addr);
    if (parity_check(addr))
    {
        lisa_parity_error(addr);
        return 0xffff;
    }
    else
        return lisa_rw_vidram(addr);
}
uint32 lisa_rl_vidram_parity(uint32 addr)
{
    DEBUG_LOG(100, "read parity %08x", addr);
    if (parity_check(addr))
    {
        lisa_parity_error(addr);
        return 0xffffffff;
    }
    else
        return lisa_rl_vidram(addr);
}

/* return actual data
uint8  lisa_rb_ram_parity(uint32 addr)     {  DEBUG_LOG(100,"read parity %08x",addr);if (parity_check(addr)) lisa_parity_error(addr);    return lisa_rb_ram(addr); }
uint16 lisa_rw_ram_parity(uint32 addr)     {  DEBUG_LOG(100,"read parity %08x",addr);if (parity_check(addr)) lisa_parity_error(addr);    return lisa_rw_ram(addr); }
uint32 lisa_rl_ram_parity(uint32 addr)     {  DEBUG_LOG(100,"read parity %08x",addr);if (parity_check(addr)) lisa_parity_error(addr);    return lisa_rl_ram(addr); }

uint8  lisa_rb_vidram_parity(uint32 addr)  {  DEBUG_LOG(100,"read parity %08x",addr);if (parity_check(addr)) lisa_parity_error(addr);    return lisa_rb_vidram(addr); }
uint16 lisa_rw_vidram_parity(uint32 addr)  {  DEBUG_LOG(100,"read parity %08x",addr);if (parity_check(addr)) lisa_parity_error(addr);    return lisa_rw_vidram(addr); }
uint32 lisa_rl_vidram_parity(uint32 addr)  {  DEBUG_LOG(100,"read parity %08x",addr);if (parity_check(addr)) lisa_parity_error(addr);    return lisa_rl_vidram(addr); }
*/

void lisa_wb_ram_parity(uint32 addr, uint8 data)
{
    DEBUG_LOG(100, "write parity %02x to %08x", data, addr);
    set_parity_check(addr);
    lisa_wb_ram(addr, data);
}
void lisa_ww_ram_parity(uint32 addr, uint16 data)
{
    DEBUG_LOG(100, "write parity %04x to %08x", data, addr);
    set_parity_check(addr);
    lisa_ww_ram(addr, data);
}
void lisa_wl_ram_parity(uint32 addr, uint32 data)
{
    DEBUG_LOG(100, "write parity %08x to %08x", data, addr);
    set_parity_check(addr);
    lisa_wl_ram(addr, data);
}

void lisa_wb_vidram_parity(uint32 addr, uint8 data)
{
    DEBUG_LOG(100, "write parity %02x to %08x", data, addr);
    set_parity_check(addr);
    lisa_wb_vidram(addr, data);
}
void lisa_ww_vidram_parity(uint32 addr, uint16 data)
{
    DEBUG_LOG(100, "write parity %04x to %08x", data, addr);
    set_parity_check(addr);
    lisa_ww_vidram(addr, data);
}
void lisa_wl_vidram_parity(uint32 addr, uint32 data)
{
    DEBUG_LOG(100, "write parity %08x to %08x", data, addr);
    set_parity_check(addr);
    lisa_wl_vidram(addr, data);
}

void lisa_wb_xlvidram_parity(uint32 addr, uint8 data)
{
    DEBUG_LOG(100, "write parity %02x to %08x", data, addr);
    set_parity_check(addr);
    lisa_wb_xlvidram(addr, data);
}
void lisa_ww_xlvidram_parity(uint32 addr, uint16 data)
{
    DEBUG_LOG(100, "write parity %04x to %08x", data, addr);
    set_parity_check(addr);
    lisa_ww_xlvidram(addr, data);
}
void lisa_wl_xlvidram_parity(uint32 addr, uint32 data)
{
    DEBUG_LOG(100, "write parity %08x to %08x", data, addr);
    set_parity_check(addr);
    lisa_wl_xlvidram(addr, data);
}

static uint32 statreg_map[335] = {             /*clk       0*/
                                  0xaaaeaaaa,  /*clk     192*/
                                  0xaaaaeaaa,  /*clk     384*/
                                  0xaaaaaeaa,  /*clk     576*/
                                  0xaaaaaaea,  /*clk     768*/
                                  0xeaaaaaae,  /*clk     960*/
                                  0xeffffeaa,  /*clk    1152*/
                                  0xfeffffff,  /*clk    1344*/
                                  0xffefffff,  /*clk    1536*/
                                  0xfffeffff,  /*clk    1728*/
                                  0xffffefff,  /*clk    1920*/
                                  0xfffffeff,  /*clk    2112*/
                                  0xffffffef,  /*clk    2304*/
                                  0xeffffffe,  /*clk    2496*/
                                  0xfeffffff,  /*clk    2688*/
                                  0xffefffff,  /*clk    2880*/
                                  0xfffeffff,  /*clk    3072*/
                                  0xffffefff,  /*clk    3264*/
                                  0xfffffeff,  /*clk    3456*/
                                  0xffffffef,  /*clk    3648*/
                                  0xeffffffe,  /*clk    3840*/
                                  0xfeffffff,  /*clk    4032*/
                                  0xffefffff,  /*clk    4224*/
                                  0xfffeffff,  /*clk    4416*/
                                  0xffffefff,  /*clk    4608*/
                                  0xfffffeff,  /*clk    4800*/
                                  0xffffffef,  /*clk    4992*/
                                  0xeffffffe,  /*clk    5184*/
                                  0xfeffffff,  /*clk    5376*/
                                  0xffefffff,  /*clk    5568*/
                                  0xfffeffff,  /*clk    5760*/
                                  0xffffefff,  /*clk    5952*/
                                  0xfffffeff,  /*clk    6144*/
                                  0xffffffef,  /*clk    6336*/
                                  0xeffffffe,  /*clk    6528*/
                                  0xfeffffff,  /*clk    6720*/
                                  0xffefffff,  /*clk    6912*/
                                  0xfffeffff,  /*clk    7104*/
                                  0xffffefff,  /*clk    7296*/
                                  0xfffffeff,  /*clk    7488*/
                                  0xffffffef,  /*clk    7680*/
                                  0xeffffffe,  /*clk    7872*/
                                  0xfeffffff,  /*clk    8064*/
                                  0xffefffff,  /*clk    8256*/
                                  0xfffeffff,  /*clk    8448*/
                                  0xffffefff,  /*clk    8640*/
                                  0xfffffeff,  /*clk    8832*/
                                  0xffffffef,  /*clk    9024*/
                                  0xeffffffe,  /*clk    9216*/
                                  0xfeffffff,  /*clk    9408*/
                                  0xffefffff,  /*clk    9600*/
                                  0xfffeffff,  /*clk    9792*/
                                  0xffffefff,  /*clk    9984*/
                                  0xfffffeff,  /*clk   10176*/
                                  0xffffffef,  /*clk   10368*/
                                  0xeffffffe,  /*clk   10560*/
                                  0xfeffffff,  /*clk   10752*/
                                  0xffefffff,  /*clk   10944*/
                                  0xfffeffff,  /*clk   11136*/
                                  0xffffefff,  /*clk   11328*/
                                  0xfffffeff,  /*clk   11520*/
                                  0xffffffef,  /*clk   11712*/
                                  0xeffffffe,  /*clk   11904*/
                                  0xfeffffff,  /*clk   12096*/
                                  0xffefffff,  /*clk   12288*/
                                  0xfffeffff,  /*clk   12480*/
                                  0xffffefff,  /*clk   12672*/
                                  0xfffffeff,  /*clk   12864*/
                                  0xffffffef,  /*clk   13056*/
                                  0xeffffffe,  /*clk   13248*/
                                  0xfeffffff,  /*clk   13440*/
                                  0xffefffff,  /*clk   13632*/
                                  0xfffeffff,  /*clk   13824*/
                                  0xffffefff,  /*clk   14016*/
                                  0xfffffeff,  /*clk   14208*/
                                  0xffffffef,  /*clk   14400*/
                                  0xeffffffe,  /*clk   14592*/
                                  0xfeffffff,  /*clk   14784*/
                                  0xffefffff,  /*clk   14976*/
                                  0xfffeffff,  /*clk   15168*/
                                  0xffffefff,  /*clk   15360*/
                                  0xfffffeff,  /*clk   15552*/
                                  0xffffffef,  /*clk   15744*/
                                  0xeffffffe,  /*clk   15936*/
                                  0xfeffffff,  /*clk   16128*/
                                  0xffefffff,  /*clk   16320*/
                                  0xfffeffff,  /*clk   16512*/
                                  0xffffefff,  /*clk   16704*/
                                  0xfffffeff,  /*clk   16896*/
                                  0xffffffef,  /*clk   17088*/
                                  0xeffffffe,  /*clk   17280*/
                                  0xfeffffff,  /*clk   17472*/
                                  0xffefffff,  /*clk   17664*/
                                  0xfffeffff,  /*clk   17856*/
                                  0xffffefff,  /*clk   18048*/
                                  0xfffffeff,  /*clk   18240*/
                                  0xffffffef,  /*clk   18432*/
                                  0xeffffffe,  /*clk   18624*/
                                  0xfeffffff,  /*clk   18816*/
                                  0xffefffff,  /*clk   19008*/
                                  0xfffeffff,  /*clk   19200*/
                                  0xffffefff,  /*clk   19392*/
                                  0xfffffeff,  /*clk   19584*/
                                  0xffffffef,  /*clk   19776*/
                                  0xeffffffe,  /*clk   19968*/
                                  0xfeffffff,  /*clk   20160*/
                                  0xffefffff,  /*clk   20352*/
                                  0xfffeffff,  /*clk   20544*/
                                  0xffffefff,  /*clk   20736*/
                                  0xfffffeff,  /*clk   20928*/
                                  0xffffffef,  /*clk   21120*/
                                  0xeffffffe,  /*clk   21312*/
                                  0xfeffffff,  /*clk   21504*/
                                  0xffefffff,  /*clk   21696*/
                                  0xfffeffff,  /*clk   21888*/
                                  0xffffefff,  /*clk   22080*/
                                  0xfffffeff,  /*clk   22272*/
                                  0xffffffef,  /*clk   22464*/
                                  0xeffffffe,  /*clk   22656*/
                                  0xfeffffff,  /*clk   22848*/
                                  0xffefffff,  /*clk   23040*/
                                  0xfffeffff,  /*clk   23232*/
                                  0xffffefff,  /*clk   23424*/
                                  0xfffffeff,  /*clk   23616*/
                                  0xffffffef,  /*clk   23808*/
                                  0xeffffffe,  /*clk   24000*/
                                  0xfeffffff,  /*clk   24192*/
                                  0xffefffff,  /*clk   24384*/
                                  0xfffeffff,  /*clk   24576*/
                                  0xffffefff,  /*clk   24768*/
                                  0xfffffeff,  /*clk   24960*/
                                  0xffffffef,  /*clk   25152*/
                                  0xeffffffe,  /*clk   25344*/
                                  0xfeffffff,  /*clk   25536*/
                                  0xffefffff,  /*clk   25728*/
                                  0xfffeffff,  /*clk   25920*/
                                  0xffffefff,  /*clk   26112*/
                                  0xfffffeff,  /*clk   26304*/
                                  0xffffffef,  /*clk   26496*/
                                  0xeffffffe,  /*clk   26688*/
                                  0xfeffffff,  /*clk   26880*/
                                  0xffefffff,  /*clk   27072*/
                                  0xfffeffff,  /*clk   27264*/
                                  0xffffefff,  /*clk   27456*/
                                  0xfffffeff,  /*clk   27648*/
                                  0xffffffef,  /*clk   27840*/
                                  0xeffffffe,  /*clk   28032*/
                                  0xfeffffff,  /*clk   28224*/
                                  0xffefffff,  /*clk   28416*/
                                  0xfffeffff,  /*clk   28608*/
                                  0xffffefff,  /*clk   28800*/
                                  0xfffffeff,  /*clk   28992*/
                                  0xffffffef,  /*clk   29184*/
                                  0xeffffffe,  /*clk   29376*/
                                  0xfeffffff,  /*clk   29568*/
                                  0xffefffff,  /*clk   29760*/
                                  0xfffeffff,  /*clk   29952*/
                                  0xffffefff,  /*clk   30144*/
                                  0xfffffeff,  /*clk   30336*/
                                  0xffffffef,  /*clk   30528*/
                                  0xeffffffe,  /*clk   30720*/
                                  0xfeffffff,  /*clk   30912*/
                                  0xffefffff,  /*clk   31104*/
                                  0xfffeffff,  /*clk   31296*/
                                  0xffffefff,  /*clk   31488*/
                                  0xfffffeff,  /*clk   31680*/
                                  0xffffffef,  /*clk   31872*/
                                  0xeffffffe,  /*clk   32064*/
                                  0xfeffffff,  /*clk   32256*/
                                  0xffefffff,  /*clk   32448*/
                                  0xfffeffff,  /*clk   32640*/
                                  0xffffefff,  /*clk   32832*/
                                  0xfffffeff,  /*clk   33024*/
                                  0xffffffef,  /*clk   33216*/
                                  0xeffffffe,  /*clk   33408*/
                                  0xfeffffff,  /*clk   33600*/
                                  0xffefffff,  /*clk   33792*/
                                  0xfffeffff,  /*clk   33984*/
                                  0xffffefff,  /*clk   34176*/
                                  0xfffffeff,  /*clk   34368*/
                                  0xffffffef,  /*clk   34560*/
                                  0xeffffffe,  /*clk   34752*/
                                  0xfeffffff,  /*clk   34944*/
                                  0xffefffff,  /*clk   35136*/
                                  0xfffeffff,  /*clk   35328*/
                                  0xffffefff,  /*clk   35520*/
                                  0xfffffeff,  /*clk   35712*/
                                  0xffffffef,  /*clk   35904*/
                                  0xeffffffe,  /*clk   36096*/
                                  0xfeffffff,  /*clk   36288*/
                                  0xffefffff,  /*clk   36480*/
                                  0xfffeffff,  /*clk   36672*/
                                  0xffffefff,  /*clk   36864*/
                                  0xfffffeff,  /*clk   37056*/
                                  0xffffffef,  /*clk   37248*/
                                  0xeffffffe,  /*clk   37440*/
                                  0xfeffffff,  /*clk   37632*/
                                  0xffefffff,  /*clk   37824*/
                                  0xfffeffff,  /*clk   38016*/
                                  0xffffefff,  /*clk   38208*/
                                  0xfffffeff,  /*clk   38400*/
                                  0xffffffef,  /*clk   38592*/
                                  0xeffffffe,  /*clk   38784*/
                                  0xfeffffff,  /*clk   38976*/
                                  0xffefffff,  /*clk   39168*/
                                  0xfffeffff,  /*clk   39360*/
                                  0xffffefff,  /*clk   39552*/
                                  0xfffffeff,  /*clk   39744*/
                                  0xffffffef,  /*clk   39936*/
                                  0xeffffffe,  /*clk   40128*/
                                  0xfeffffff,  /*clk   40320*/
                                  0xffefffff,  /*clk   40512*/
                                  0xfffeffff,  /*clk   40704*/
                                  0xffffefff,  /*clk   40896*/
                                  0xfffffeff,  /*clk   41088*/
                                  0xffffffef,  /*clk   41280*/
                                  0xeffffffe,  /*clk   41472*/
                                  0xfeffffff,  /*clk   41664*/
                                  0xffefffff,  /*clk   41856*/
                                  0xfffeffff,  /*clk   42048*/
                                  0xffffefff,  /*clk   42240*/
                                  0xfffffeff,  /*clk   42432*/
                                  0xffffffef,  /*clk   42624*/
                                  0xeffffffe,  /*clk   42816*/
                                  0xfeffffff,  /*clk   43008*/
                                  0xffefffff,  /*clk   43200*/
                                  0xfffeffff,  /*clk   43392*/
                                  0xffffefff,  /*clk   43584*/
                                  0xfffffeff,  /*clk   43776*/
                                  0xffffffef,  /*clk   43968*/
                                  0xeffffffe,  /*clk   44160*/
                                  0xfeffffff,  /*clk   44352*/
                                  0xffefffff,  /*clk   44544*/
                                  0xfffeffff,  /*clk   44736*/
                                  0xffffefff,  /*clk   44928*/
                                  0xfffffeff,  /*clk   45120*/
                                  0xffffffef,  /*clk   45312*/
                                  0xeffffffe,  /*clk   45504*/
                                  0xfeffffff,  /*clk   45696*/
                                  0xffefffff,  /*clk   45888*/
                                  0xfffeffff,  /*clk   46080*/
                                  0xffffefff,  /*clk   46272*/
                                  0xfffffeff,  /*clk   46464*/
                                  0xffffffef,  /*clk   46656*/
                                  0xeffffffe,  /*clk   46848*/
                                  0xfeffffff,  /*clk   47040*/
                                  0xffefffff,  /*clk   47232*/
                                  0xfffeffff,  /*clk   47424*/
                                  0xffffefff,  /*clk   47616*/
                                  0xfffffeff,  /*clk   47808*/
                                  0xffffffef,  /*clk   48000*/
                                  0xeffffffe,  /*clk   48192*/
                                  0xfeffffff,  /*clk   48384*/
                                  0xffefffff,  /*clk   48576*/
                                  0xfffeffff,  /*clk   48768*/
                                  0xffffefff,  /*clk   48960*/
                                  0xfffffeff,  /*clk   49152*/
                                  0xffffffef,  /*clk   49344*/
                                  0xeffffffe,  /*clk   49536*/
                                  0xfeffffff,  /*clk   49728*/
                                  0xffefffff,  /*clk   49920*/
                                  0xfffeffff,  /*clk   50112*/
                                  0xffffefff,  /*clk   50304*/
                                  0xfffffeff,  /*clk   50496*/
                                  0xffffffef,  /*clk   50688*/
                                  0xeffffffe,  /*clk   50880*/
                                  0xfeffffff,  /*clk   51072*/
                                  0xffefffff,  /*clk   51264*/
                                  0xfffeffff,  /*clk   51456*/
                                  0xffffefff,  /*clk   51648*/
                                  0xfffffeff,  /*clk   51840*/
                                  0xffffffef,  /*clk   52032*/
                                  0xeffffffe,  /*clk   52224*/
                                  0xfeffffff,  /*clk   52416*/
                                  0xffefffff,  /*clk   52608*/
                                  0xfffeffff,  /*clk   52800*/
                                  0xffffefff,  /*clk   52992*/
                                  0xfffffeff,  /*clk   53184*/
                                  0xffffffef,  /*clk   53376*/
                                  0xeffffffe,  /*clk   53568*/
                                  0xfeffffff,  /*clk   53760*/
                                  0xffefffff,  /*clk   53952*/
                                  0xfffeffff,  /*clk   54144*/
                                  0xffffefff,  /*clk   54336*/
                                  0xfffffeff,  /*clk   54528*/
                                  0xffffffef,  /*clk   54720*/
                                  0xeffffffe,  /*clk   54912*/
                                  0xfeffffff,  /*clk   55104*/
                                  0xffefffff,  /*clk   55296*/
                                  0xfffeffff,  /*clk   55488*/
                                  0xffffefff,  /*clk   55680*/
                                  0xfffffeff,  /*clk   55872*/
                                  0xffffffef,  /*clk   56064*/
                                  0xeffffffe,  /*clk   56256*/
                                  0xfeffffff,  /*clk   56448*/
                                  0xffefffff,  /*clk   56640*/
                                  0xfffeffff,  /*clk   56832*/
                                  0xffffefff,  /*clk   57024*/
                                  0xfffffeff,  /*clk   57216*/
                                  0xffffffef,  /*clk   57408*/
                                  0xeffffffe,  /*clk   57600*/
                                  0xfeffffff,  /*clk   57792*/
                                  0xffefffff,  /*clk   57984*/
                                  0xfffeffff,  /*clk   58176*/
                                  0xffffefff,  /*clk   58368*/
                                  0xfffffeff,  /*clk   58560*/
                                  0xffffffef,  /*clk   58752*/
                                  0xeffffffe,  /*clk   58944*/
                                  0xfeffffff,  /*clk   59136*/
                                  0xffefffff,  /*clk   59328*/
                                  0xfffeffff,  /*clk   59520*/
                                  0xffffefff,  /*clk   59712*/
                                  0xfffffeff,  /*clk   59904*/
                                  0xffffffef,  /*clk   60096*/
                                  0xeffffffe,  /*clk   60288*/
                                  0xfeffffff,  /*clk   60480*/
                                  0xffefffff,  /*clk   60672*/
                                  0xfffeffff,  /*clk   60864*/
                                  0xffffefff,  /*clk   61056*/
                                  0xfffffeff,  /*clk   61248*/
                                  0xffffffef,  /*clk   61440*/
                                  0xeffffffe,  /*clk   61632*/
                                  0xfeffffff,  /*clk   61824*/
                                  0xffefffff,  /*clk   62016*/
                                  0xfffeffff,  /*clk   62208*/
                                  0xffffefff,  /*clk   62400*/
                                  0xfffffeff,  /*clk   62592*/
                                  0xffffffef,  /*clk   62784*/
                                  0xeffffffe,  /*clk   62976*/
                                  0xfeffffff,  /*clk   63168*/
                                  0xffefffff,  /*clk   63360*/
                                  0xfffeffff,  /*clk   63552*/
                                  0x0003efff,  /*clk   63744*/
                                  0x00000040,  /*clk   63936*/
                                  0x40000004,  /*clk   64128*/
                                  0x04000000}; /*clk   64320*/

uint8 lisa_rb_Oxf800_statreg(uint32 addr)
{

    int32 cxdiff = (cpu68k_clocks - video_scan);

    uint8 retval = 0;
    uint8 index_bit = (cxdiff & 0xf) << 1;
    uint8 index = (cxdiff >> 4);
    int32 x, y;
    int32 cxx;
    uint32 vmidx;

    retval = (((((index_bit) ? (statreg_map[index] >> index_bit) : (statreg_map[index])) & 1) ? STATREG_CSYNC : 0) |
              ((((index_bit) ? (statreg_map[index] >> index_bit) : (statreg_map[index])) & 2) ? STATREG_VERTICALRTC : 0) |
              STATREG_UNUSEDBIT);

#ifndef INVERSE_VIDEO
    retval |= STATREG_INVBIT;
#endif

    if (cxdiff >= 64236)
    {
        video_scan = cpu68k_clocks;
        cxdiff = 0;
        retval &= ~STATREG_VERTICALRTC; // 2021.02.23 this was clears vertical bit=0
        cxdiff = cxdiff % 64236;
    }
    else
        retval |= STATREG_VERTICALRTC; // 2021.02.23 this did not exist - if these two are reversed Boot throws error 42, but UniPlus goes further, else gets stuck in infinite loop

    cxx = (int32)(cxdiff - 2328);

    if (cxx < 0 || cxdiff > 61823)
        vmidx = 32767;
    else
    {
        y = (cxx / 168);

        if (y > 363)
            vmidx = 32767;
        else
        {
            x = (cxx % 168) - 36;
            if (x < 0)
            {
                x = 120;
                y--;
            }
            if (x > 120)
                x = 120;

            if (y < 0)
                vmidx = 32767;
            else
                vmidx = (y * 90 + (x * 3 / 4));
        }
    }

    if (lisaram[videolatchaddress + vmidx] & 1)
        retval |= STATREG_VIDEOBIT;

    CHECK_DIRTY_MMU(addr);
    DEBUG_LOG(100, "@%08x", addr);

    retval |= (softmemerror ? 0 : STATREG_SOFTMEM_ERR) | // reminder: statreg is inverted logic, so it's 0 when on, 1 when off.
              (parity_error_hit ? 0 : STATREG_HARDMEM_ERR) |
              (bustimeout ? 0 : STATREG_BUSTIMEOUT);

    parity_error_hit = 0; // clear the parity error flag - do we do this now or when PAROFF happens?
    softmemerror = 0;

    DEBUG_LOG(100, "retval:%02x :: (active low) bit0softerr:%d, 1parity:%d, 2vret:%d::%d, 3bustmout:%d, 4vidbit:%d, 5hzsync:%d, 6invbit:%d  maxx",
              retval,
              retval & STATREG_SOFTMEM_ERR,
              retval & STATREG_HARDMEM_ERR,
              retval & STATREG_VERTICALRTC, vertical,
              retval & STATREG_BUSTIMEOUT,
              retval & STATREG_VIDEOBIT,
              retval & STATREG_HORIZONTAL,
              retval & STATREG_INVBIT);

    // if (vertical != ((retval &  STATREG_VERTICALRTC) ?1:0) );
    //    DEBUG_LOG(100,"Vertical=%d statreg bit is: %d at clk:%016llx",vertical,(retval &  STATREG_VERTICALRTC),cpu68k_clocks );

    if (verticallatch)
        retval &= ~STATREG_VERTICALRTC;

    return retval;
}

/*
"No program is perfect,"
They said with a shrug.
"The customer's happy--
What's one little bug?"

But he was determined,                  Then change two, then three more,
The others went home.                   As year followed year.
He dug out the flow chart               And strangers would comment,
Deserted, alone.                        "Is that guy still here?"

Night passed into morning.              He died at the console
The room was cluttered                  Of hunger and thirst
With core dumps, source listings.       Next day he was buried
"I'm close," he muttered.               Face down, nine edge first.

Chain smoking, cold coffee,             And his wife through her tears
Logic, deduction.                       Accepted his fate.
"I've got it!" he cried,                Said "He's not really gone,
"Just change one instruction."          He's just working late."
                -- The Perfect Programmer


(From the fortune cookie program)
*/
