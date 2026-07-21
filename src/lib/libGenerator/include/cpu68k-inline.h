/* the ordering of these includes is important - stdio can have inline
   functions - so can mem68k.h - and registers.h must appear before them */

#include <generator.h>
#include <registers.h>

#include <stdio.h>

#include <cpu68k.h>
#include <reg68k.h>

// only needed for DEBUG()
#include <vars.h>

extern void a_line(void);
extern void trap_opcode(uint32 n);

#ifdef XXXDEBUG
#define GENDBG(x)                                                                                       \
  {                                                                                                     \
    if ((ipc->opcode & 0xf1f8) == 0x31b0)                                                               \
    {                                                                                                   \
      fprintf(stdout, "//DBG:%s:%s:%d IR:%04x %s\n", __FILE__, __FUNCTION__, __LINE__, ipc->opcode, x); \
    }                                                                                                   \
  }
#else
#define GENDBG(x)
#endif

#define DATAREG(a) (reg68k_regs[a])
#define ADDRREG(a) (reg68k_regs[8 + (a)])
#define PC (reg68k_pc)
#define SR (reg68k_sr.sr_int)
#define SP (regs.sp)
#define STOP (regs.stop)
#define TFLAG (reg68k_sr.sr_struct.t)
#define SFLAG (reg68k_sr.sr_struct.s)
#define XFLAG (reg68k_sr.sr_struct.x)
#define NFLAG (reg68k_sr.sr_struct.n)
#define ZFLAG (reg68k_sr.sr_struct.z)
#define VFLAG (reg68k_sr.sr_struct.v)
#define CFLAG (reg68k_sr.sr_struct.c)
#define IMASK ((SR >> 8) & 7)

static __inline__ sint32 idxval_dst(t_ipc *ipc)
{
  sint32 r;

  // switch( ((ipc->dst>>27) & 1) | ((ipc->dst>>30) & 2) ) {
  //   23,24,25,26,27,28,29,30,31
  //   -1, 0, 1, 2, 3, 4, 5, 6, 7
  switch (((ipc->dreg >> 3) & 1) | ((ipc->dreg >> 6) & 2))
  {
  case 0:
    GENDBG("0-data,word"); // data, word
    r = ((sint16)DATAREG((ipc->dreg >> 4) & 7)) + ((sint32)(ipc->dst));
    DEBUG_LOG(10, "r=%08lx %ld =datareg(%ld)=%08lx + %08lx", (long)r, (long)r,
              (long)(ipc->dreg & 7),
              (long)((sint16)DATAREG(ipc->dreg & 7)),
              (long)((sint32)(ipc->dst)));
    return r;

  case 1:
    GENDBG("1: data,long"); // data, long
    r = ((sint32)DATAREG((ipc->dreg >> 4) & 7)) + ((sint32)(ipc->dst));
    DEBUG_LOG(10, "r=%08lx %ld = = datareg(%ld) %08lx + %08lx", (long)r, (long)r,
              (long)(ipc->dreg & 7),
              (long)(sint32)DATAREG(ipc->dreg & 7),
              (long)((sint32)(ipc->dst)));
    return r;

  case 2:
    GENDBG("2-addr,word"); // addr, word
    r = ((sint16)ADDRREG((ipc->dreg >> 4) & 7)) + ((sint32)(ipc->dst));
    DEBUG_LOG(0, "SUSPECT r=%08lx %ld = addrreg(%ld).w %04lx + %08lx  DANGER HERE DANGER HERE DANGER HERE!!!", (long)r, (long)r,
              (long)(ipc->dreg & 7),
              (long)(sint16)ADDRREG(ipc->dreg & 7),
              (long)((sint32)(ipc->dst)));
    return r;

  case 3:
    GENDBG("3: addr,long"); // addr, long
    r = ((sint32)ADDRREG((ipc->dreg >> 4) & 7) + ((sint32)(ipc->dst)));
    DEBUG_LOG(10, "r=%08lx %ld = addrreg(%ld) %08lx + %08lx", (long)r, (long)r,
              (long)(ipc->dreg & 7),
              (long)(sint32)ADDRREG(ipc->dreg & 7),
              (long)((sint32)(ipc->dst)));
    return r;
  }
  return 0;
}

static __inline__ sint32 idxval_src(t_ipc *ipc)
{
  sint32 r;
  //  24,25,26,27,28,29,30,31
  //   0  1  2  3  4  5  6  7
  switch (((ipc->sreg >> 3) & 1) | ((ipc->sreg >> 6) & 2))
  {
  case 0:
    GENDBG("0: data,word");                                             // data, word
    r = ((sint16)DATAREG((ipc->sreg >> 4) & 7) + ((sint32)(ipc->src))); // lisaem1.2.x
    DEBUG_LOG(10, "idxvsrc:0:data,word D%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx", (long)((ipc->sreg >> 4) & 7), (long)DATAREG((ipc->sreg >> 4) & 7), (long)r, (long)r, (long)ipc->src, (long)ipc->dst);
    DEBUG_LOG(10, "idxval_src=%08lx (%ld) = %04lx+%08lx", (unsigned long int)r, (long)r,
              (unsigned long int)(((sint32)(sint16)DATAREG(ipc->sreg & 7))), ((unsigned long int)((sint32)(ipc->src))));
    return r;

  case 1:
    GENDBG("1: data,long"); // data, long
    // 20060130-RA// r=((sint32)DATAREG((ipc->src>>28)&7))  +  ((((sint32)(ipc->src<<8)))>>8);
    // 20060203-RA// r=((sint32)DATAREG((ipc->src>>28)&7))  +  ((sint32)(ipc->src) );

    r = ((sint32)DATAREG((ipc->sreg >> 4) & 7) + ((sint32)(ipc->src)));

    DEBUG_LOG(10, "idxvsrc:1:data,long D%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx", (long)((ipc->sreg >> 4) & 7), (long)DATAREG((ipc->sreg >> 4) & 7), (long)r, (long)r, (long)ipc->src, (long)ipc->dst);
    DEBUG_LOG(10, "idxval_src=%08lx (%ld) = %08lx+%08lx", (long)r, (long)r,
              (unsigned long)((sint32)DATAREG(ipc->sreg & 7)), (unsigned long)((sint32)(ipc->src)));
    return r;

  case 2:
    GENDBG("2: addr,word"); // addr, word
    r = ((sint16)ADDRREG((ipc->sreg >> 4) & 7)) + ((sint32)(ipc->src));
    DEBUG_LOG(10, "idxvsrc:2:addr,word A%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx", (long)((ipc->sreg >> 4) & 7), (unsigned long)ADDRREG((ipc->sreg) & 7), (long)r, (unsigned long)r, (unsigned long)ipc->src, (unsigned long)ipc->dst);
    DEBUG_LOG(10, "idxval_src=%08lx (%ld) = %04lx+%08lx", (unsigned long)r, (long)r,
              (long)((sint16)ADDRREG(ipc->sreg & 7)), (unsigned long)((sint32)(ipc->src)));
    return r;
  case 3:
    GENDBG("3: addr,long"); // addr, long
    r = ((sint32)ADDRREG((ipc->sreg >> 4) & 7) + ((sint32)(ipc->src)));
    DEBUG_LOG(10, "idxvsrc:3:addr,long A%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx", (long)((ipc->sreg >> 4) & 7), (long)ADDRREG((ipc->sreg) & 7), (long)r, (long)r, (unsigned long)ipc->src, (unsigned long)ipc->dst);

    DEBUG_LOG(10, "idxval_src=%08lx (%ld) = %08lx+%08lx", (long)r, (long)r,
              (long)((sint32)ADDRREG(ipc->sreg & 7)), (unsigned long)((sint32)(ipc->src)));
    return r;
  }
  return 0;
}

// Added by RA
#define SWAP_USP_SSP()                                                                                      \
  {                                                                                                         \
    ADDRREG(7) ^= SP;                                                                                       \
    SP ^= ADDRREG(7);                                                                                       \
    ADDRREG(7) ^= SP;                                                                                       \
    DEBUG_LOG(5, "S mode change SP:%08lx/A7:%08lx swapped.", (unsigned long)ADDRREG(7), (unsigned long)SP); \
  }
#define SYNC_PC_SR()     \
  {                      \
    regs.pc = reg68k_pc; \
    regs.sr = reg68k_sr; \
  }

// Added by Ray Arachelian for Lisa Emulator to handle address errors and such without completion of the operation.
extern int abort_opcode;
#define ABORT_OPCODE_CHK()                                               \
  {                                                                      \
    if (abort_opcode == 1)                                               \
    {                                                                    \
      DEBUG_LOG(0, "ABORTING OPCODE AT %08lx abort_opcode=1", (long)PC); \
      return;                                                            \
    }                                                                    \
  }

// Added by Ray Arachelian for Lisa Emulator: on S flag change, mmu may change context, etc.
// this does the following: 1. swap USP with SSP, 2. synchronize PC and SR to setjmp land, 3. does an MMU flush

#define SR_CHANGE()                          \
  {                                          \
    SWAP_USP_SSP();                          \
    SYNC_PC_SR();                            \
    mmuflush(0x2000 | (SFLAG ? 0x1000 : 0)); \
  }

////// #define IRQMASKLOWER() { DEBUG_LOG(0,"IRQMASK lowered, setting clocks_stop from:%016llx to %016llx ",cpu68k_clocks_stop,cpu68k_clocks-1); cpu68k_clocks_stop=cpu68k_clocks-1;}
#define IRQMASKLOWER()               \
  {                                  \
    DEBUG_LOG(0, "IRQMASK lowered"); \
  }
