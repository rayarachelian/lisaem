/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

// This has been heavily modified for the Lisa Emulator...  Modifications of this file
// are (c) 1998-2002 Ray Arachelian, http://lisa.sunder.net  updated to match gen 0.35

#define IN_CPU68K_C 1
#include <vars.h>
#include <generator.h>
#include <cpu68k.h>
#include <ui.h>
#include "def68k-iibs.h"
#include <def68k-proto.h>
#include <def68k-funcs.h>

// #ifdef DEBUG
// #undef DEBUG
// #endif

/*** externally accessible variables ***/
int do_iib_check = 0;
t_iib *cpu68k_iibtable[65536];
void (*cpu68k_functable[65536 * 2])(t_ipc *ipc);
int cpu68k_totalinstr;
int cpu68k_totalfuncs;
unsigned long cpu68k_frames;
unsigned long cpu68k_frozen; /* cpu frozen, do not interrupt, make pending */
int abort_opcode = 0;        /* used by MMU to signal access to illegal/invalid memory -- RA*/

t_regs regs;
uint8 movem_bit[256];

#ifdef DEBUG
void dumpallmmu(void);
#endif

// used by cpu68k_makeipclist as scratch space during opcode corrections.
// We allocate these but don't free them as they're reused, and repeated malloc/free's can
// fragment memory and are slow.  So we just regrow as needed.  But we need a nice big chunk
// of them so we can avoid realloc as much as possible.  16K worth of ipc's should be good.
t_ipc **ipcs = NULL;
// 2005.04.14 - set IPCS much much higher - sorry, LisaTest needs'em.
#define ipcs_to_get 8192 // 1024

/*** forward references ***/

void cpu68k_reset(void);

// #define TEST_FREE_IPCTS_ARE_EMPTY 1
// #define DEBUG 1

#ifdef DEBUG

static int iib_checks_enabled = 0;
static void *iib_table_ptr[65536];
static void *iib_check_fns[65536];

// this wasn't triggered, so our LisaTest bug isn't caused by this at all
#ifdef XXXXTEST_FREE_IPCTS_ARE_EMPTY
void check_ipct_memory(void)
{
  t_ipc_table *ipct = ipct_free_head;
  int i = 0, j;

  // walk free ipc linked list to ensure they don't segfault and that they're empty - if these get a hit, we've got bugs or memory clobbering
  while (ipct)
  {
    if (mmu_trans_all[context][(pc24 & MMUEPAGEFL) >> 9].table == ipct)
    {
      DEBUG_LOG(200, "There's an unhappy little IPCT over here, and maybe over there lives a big buggy mmu_trans_all table...")
      DEBUG_LOG(200, "*BUG* Found the current IPCT for PC:%d/%08x in the free list!!! at ipct_free_head[%d] ipct=%p mmu.table=%p",
                context, pc24, i, ipct, mmu_trans_all[context][(pc24 & MMUEPAGEFL) >> 9].table);
      DEBUG_LOG(200, "*BUG* ipct->next:%p", ipct->next);
      DEBUG_LOG(200, "*BUG* ipct->context:%d", ipct->context);
      DEBUG_LOG(200, "*BUG* ipct->address:%08x", ipct->address);
      DEBUG_LOG(200, "*BUG* ipct->used:%d", ipct->used);
      DEBUG_LOG(200, "*BUG* pc24>>9=%08x, mmu_trans_all[context][(pc24 & ADDRESSFILT)>>9].address=%08x rfn:%d,wfn:%d", (pc24 & MMUEPAGEFL) >> 9,
                mmu_trans_all[context][(pc24 & MMUEPAGEFL) >> 9].address,
                mmu_trans_all[context][(pc24 & MMUEPAGEFL) >> 9].readfn, mmu_trans_all[context][(pc24 & MMUEPAGEFL) >> 9].writefn);
    }

    for (j = 0; j < 256; j++)
    {
      if (ipct->ipc[j].function != NULL)
      {
        DEBUG_LOG(200, "*BUG* pc24:%d/%08x addr:%d/%08x ipct_free_head[%d]->ipc[%d].function is not NULL @%p\n", context, pc24, ipct->context, ipct->address, i, j, ipct);
      }
      if (ipct->ipc[j].used != 0)
      {
        DEBUG_LOG(200, "*BUG* pc24:%d/%08x addr:%d/%08x ipct_free_head[%d]->ipc[%d].used is not 0 @%p\n", context, pc24, ipct->context, ipct->address, i, j, ipct);
      }
      if (ipct->ipc[j].set != 0)
      {
        DEBUG_LOG(200, "*BUG* pc24:%d/%08x addr:%d/%08x ipct_free_head[%d]->ipc[%d].set is not 0 @%p\n", context, pc24, ipct->context, ipct->address, i, j, ipct);
      }
      if (ipct->ipc[j].opcode != 0xf33f)
      {
        DEBUG_LOG(200, "*BUG* pc24:%d/%08x addr:%d/%08x ipct_free_head[%d]->ipc[%d].opcode is not 0xf33f @%p it is %04x\n", context, pc24, ipct->context, ipct->address, i, j, ipct, ipct->ipc[j].opcode);
      }
      if (ipct->ipc[j].src != 0)
      {
        DEBUG_LOG(200, "*BUG* pc24:%d/%08x addr:%d/%08x ipct_free_head[%d]->ipc[%d].src is not 0 @%p\n", context, pc24, ipct->context, ipct->address, i, j, ipct, (long)ipct->ipc[j].src);
      }
      if (ipct->ipc[j].dst != 0)
      {
        DEBUG_LOG(200, "*BUG* pc24:%d/%08x addr:%d/%08x ipct_free_head[%d]->ipc[%d].dst is not 0 @%p\n", context, pc24, ipct->context, ipct->address, i, j, ipct, (long)ipct->ipc[j].dst);
      }

      // ipct->ipc[j].function=NULL;
      // ipct->ipc[j].used=0;
      // ipct->ipc[j].set=0;
      // ipct->ipc[j].opcode=0xf33f;
      // ipct->ipc[j].reg=0;
      // ipct->ipc[j].src=0;
      // ipct->ipc[j].dst=0;
    }

    if (mmu_trans_all[context][(pc24 & MMUEPAGEFL) >> 9].table == ipct)
    {
      DEBUG_LOG(200, "This is the end, my only friend, the end...")
      i = 0;
      ipct = ipct_free_head;
      for (int c = 0; c < 5; c++)
        for (int s = 0; s < 32768; s++)
          while (ipct)
          {
            if (mmu_trans_all[c][s].table == ipct)
            {
              DEBUG_LOG(200, "*BUG* ipct from free_list[%d] found pointed to from mmu_trans_all[%d][%d<<9=(%08x)].table==%p", i, context, s, s << 9, ipct);
              DEBUG_LOG(200, "*BUG* ipct->next:%p", ipct->next);
              DEBUG_LOG(200, "*BUG* ipct->context:%d", ipct->address);
              DEBUG_LOG(200, "*BUG* ipct->address:%08x", ipct->address);
              DEBUG_LOG(200, "*BUG* ipct->used:%d", ipct->used);
              DEBUG_LOG(200, "---------------------------------------------")
            }
            ipct = ipct->next;
            i++;
          }
      DEBUG_LOG(200, "======================== I am the Alizarine King, I can't do anything ====================================================");
      EXIT(0, 0, "kthxbyee! Found current ipct in free list, so quitting.");
    }

    ipct = ipct->next;
    i++;
  }
}

void check_ipct_counts(const char *function, const long linenum)
{
  long int sum = 0;
  long i = 0, usedsum = 0;
  t_ipc_table *ipctz = NULL, *ipctl = NULL;
  DEBUG_LOG(200, "------------------------------------------------------------------------------------ipc");
  DEBUG_LOG(200, "Called from: %s:%s:%d", __FILE__, function, linenum);
  DEBUG_LOG(200, "------------------------------------------------------------------------------------ipc");
  DEBUG_LOG(200, "iipct_mallocs:   %ld", (long)iipct_mallocs);
  DEBUG_LOG(200, "ipcts_allocated: %ld, %ld bytes", (long)ipcts_allocated, (long)(ipcts_allocated * sizeof(t_ipc_table)));
  DEBUG_LOG(200, "ipcts_used:      %ld", (long)ipcts_used);
  DEBUG_LOG(200, "ipcts_free:      %ld", (long)ipcts_free);
  // 2020.02.07 disabling this, while useful, not the issue. leaving it in, but commented out incase future bugs sneak in
  //  for (i=0; i<iipct_mallocs; i++)
  //  {
  //    DEBUG_LOG(200,"ipct_mallocs[%ld]=%p size:%ld ipcts size:%ld bytes",i,ipct_mallocs[i],sipct_mallocs[i],sipct_mallocs[i]*sizeof(t_ipc_table));
  //    sum+=sipct_mallocs[i];
  //  }
  //  DEBUG_LOG(200,"We have %ld mallocs, ipc_allocated are:%ld total ipcts:%ld total bytes: %ld",(long) iipct_mallocs, (long)ipcts_allocated,
  //            (long)sum, (long)(sum*sizeof(t_ipc_table)) );
  //  ipctz=ipct_free_head; i=0;
  //  while (ipctz) {
  //    ipctl=ipctz;
  //    usedsum+=ipctz->used;
  //    ipctz=ipctz->next;
  //    i++;
  //  }
  //  DEBUG_LOG(200,"Counted %ld ipcts, %ld are in use",i,usedsum);
  //  if (ipctl!=ipct_free_tail) {DEBUG_LOG(200,"actual ipct_tail:%p tail:%p",ipctl,ipct_free_tail)};
  //  DEBUG_LOG(200,"Counted %ld free ipcts",(long)i);
  check_ipct_memory();
  DEBUG_LOG(200, "------------------------------------------------------------------------------------ipc");
}
#endif

void seed_iib_check(void)
{
  long i;

  return; // disabled for speed
  /*
  for ( i=0; i<65536; i++)
  {
    iib_table_ptr[i]=cpu68k_iibtable[i];
    if ( cpu68k_iibtable[i])  iib_check_fns[i]=(void *)(cpu68k_iibtable[i]->funcnum);
    else iib_check_fns[i]=NULL;
  }

  iib_checks_enabled=1;
  */
}

void my_check_iib(char *filename, char *function, long line)
{
  t_iib *myiib;

  return; // disabled
  /*
    long i;
    myiib=cpu68k_iibtable[0x6602];
    if (!myiib->funcnum)
    {  EXIT(171,0,"IIB for 6602 got clobbered above this! %s:%s:%ld",filename,function,line);  }

    myiib=cpu68k_iibtable[0x4841];
    if (!myiib || !myiib->funcnum)
    {   EXIT(409,0,"IIB for 4841 got clobbered above this! %s:%s:%ld",filename,function,line); }
        return;                             // disable the full check to get some speed

        if ( !iib_checks_enabled) {seed_iib_check();iib_checks_enabled=1; return;  }

        for ( i=0; i<65536; i++)
        {
            if (iib_table_ptr[i]!=cpu68k_iibtable[i])
            {
                fprintf(buglog,"CRASH! IIB table # %04lx got clobbered here or above this! %s:%s:%ld",(long)i,filename,function,(long)line);
                fflush(buglog);
                EXIT(410,0,"CRASH! IIB table # %04lx got clobbered here or above this! %s:%s:%ld",(long)i,filename,function,(long)line);
            }
            if (iib_check_fns[i])
            {
                if ((void *)iib_check_fns[i]!=(void *)(cpu68k_iibtable[i]->funcnum))
                {
                    fprintf(buglog,"BOINK! IIB for opcode # %04lx got clobbered here or above this! %s:%s:%ld",(long)i,filename,function,(long)line);
                    fflush(buglog);
                    EXIT(411,0,"BOINK! IIB for opcode # %04lx got clobbered here or above this! %s:%s:%ld",(long)i,filename,function,(long)line);
                }
            }
        }//////////////////////////////////////////
  */
}

void my_check_valid_ipct(t_ipc_table *ipct, char *file, char *function, int line, char *text)
{
  uint16 i;
  int valid = 0;
  t_ipc_table *start = NULL, *end = NULL;
  void *vstart, *vend, *vipct;

  vipct = (void *)ipct;

  for (i = 0; i <= iipct_mallocs && !valid; i++)
  {
    start = ipct_mallocs[i];
    vstart = (void *)start;
    end = &start[sipct_mallocs[i]];
    vend = (void *)end;
    if (vipct >= vstart && vipct <= vend)
      valid = 1;
  }

  fflush(buglog);
  if (valid)
  {
    fprintf(buglog, "%s:%s:%ld valid ipct %p %s", file, function, (long)line, ipct, text);
    fflush(buglog);
  }
  else
  {
    EXIT(412, 0, "%s:%s:%ld ***HALT*** invalid ipct %p %s", file, function, (long)line, ipct, text);
  }
}

#define DEBUG_IPCT(ipct, text) \
  if (debug_log_enabled)       \
  my_check_valid_ipct(ipct, __FILE__, __FUNCTION__, __LINE__, text)
#else
#define DEBUG_IPCT(ipct, text) \
  {                            \
  }
#endif

int cpu68k_init(void)
{
  t_iib *iib;
  uint16 bitmap;
  int i, j, sbit, dbit, sbits, dbits;

  memset(cpu68k_iibtable, 0, sizeof(cpu68k_iibtable)); // 0x55 is a sentinel value - odd so it traps long/word writes
  memset(cpu68k_functable, 0, sizeof(cpu68k_functable));
  memset(&regs, 0, sizeof(regs));

  cpu68k_frozen = 0;
  cpu68k_totalinstr = 0;

  for (i = 0; i < iibs_num; i++)
  {
    iib = &iibs[i];

    bitmap = iib->mask;
    sbits = 0;
    dbits = 0;

    for (j = 0; j < 2; j++)
    {
      switch (j ? iib->stype : iib->dtype)
      {
      case dt_Dreg:
      case dt_Areg:
      case dt_Aind:
      case dt_Ainc:
      case dt_Adec:
      case dt_Adis:
      case dt_Aidx:
        if (j)
        {
          bitmap ^= 7 << iib->sbitpos;
          sbits = 3;
        }
        else
        {
          bitmap ^= 7 << iib->dbitpos;
          dbits = 3;
        }
        break;
      case dt_AbsW:
      case dt_AbsL:
      case dt_Pdis:
      case dt_Pidx:
        break;
      case dt_ImmB:
      case dt_ImmW:
      case dt_ImmL:
      case dt_ImmS:
        break;
      case dt_Imm3:
        if (j)
        {
          bitmap ^= 7 << iib->sbitpos;
          sbits = 3;
        }
        else
        {
          bitmap ^= 7 << iib->dbitpos;
          dbits = 3;
        }
        break;
      case dt_Imm4:
        if (j)
        {
          bitmap ^= 15 << iib->sbitpos;
          sbits = 4;
        }
        else
        {
          bitmap ^= 15 << iib->dbitpos;
          dbits = 4;
        }
        break;
      case dt_Imm8:
      case dt_Imm8s:
        if (j)
        {
          bitmap ^= 255 << iib->sbitpos;
          sbits = 8;
        }
        else
        {
          bitmap ^= 255 << iib->dbitpos;
          dbits = 8;
        }
        break;
      case dt_ImmV:
        sbits = 12;
        bitmap ^= 0x0FFF;
        break;
      case dt_Ill:
        /* no src/dst parameter */
        break;
      default:
        LOG_CRITICAL("CPU definition #%d incorrect", i);
        return 1;
      }
    }
    if (bitmap != 0xFFFF)
    {
      LOG_CRITICAL("CPU definition #%d incorrect (0x%x)", i, bitmap);
      return 1;
    }
    for (sbit = 0; sbit < (1 << sbits); sbit++)
    {
      for (dbit = 0; dbit < (1 << dbits); dbit++)
      {
        bitmap = iib->bits | (sbit << iib->sbitpos) | (dbit << iib->dbitpos);
        if (iib->stype == dt_Imm3 || iib->stype == dt_Imm4 || iib->stype == dt_Imm8)
        {
          if (sbit == 0 && iib->flags.imm_notzero)
          {
            continue;
          }
        }
        if (cpu68k_iibtable[bitmap] != NULL)
        {
          EXITR(283, 0, "CPU definition #%d conflicts (0x%x)", i, bitmap);
        }

        cpu68k_iibtable[bitmap] = iib;
        /* set both flag and non-flag versions */

#ifdef NORMAL_GENERATOR_FLAGS
        cpu68k_functable[bitmap * 2] = cpu68k_funcindex[i * 2];
        cpu68k_functable[bitmap * 2 + 1] = cpu68k_funcindex[i * 2 + 1];
#else
        // This forces Generator to calculate flags for every instruction that does it.
        // it's slower.
        cpu68k_functable[bitmap * 2] = cpu68k_funcindex[i * 2 + 1];
        cpu68k_functable[bitmap * 2 + 1] = cpu68k_funcindex[i * 2 + 1];
#endif
        cpu68k_totalinstr++;
      }
    }
  }

  j = 0;

  for (i = 0; i < 65536; i++)
  {
    if (cpu68k_iibtable[i])
      j++;
    else
    {
      cpu68k_iibtable[i] = cpu68k_iibtable[0x4AFC * 2]; // set all unused to illegal 2020.11.03
      cpu68k_functable[i * 2] = cpu68k_functable[0x4AFC * 2];
      cpu68k_functable[i * 2 + 1] = cpu68k_functable[0x4AFC * 2];
    }
  }

  if (j != cpu68k_totalinstr)
  {
    EXITR(84, 0, "Instruction count not verified (%d/%d)\n",
          cpu68k_totalinstr, i);
  }

  cpu68k_totalfuncs = iibs_num;

  for (i = 0; i < 256; i++)
  {
    for (j = 0; j < 8; j++)
    {
      if (i & (1 << j))
        break;
    }
    movem_bit[i] = j;
  }

  return 0;
}

#ifdef DEBUG
void cpu68k_printipc(t_ipc *ipc)
{

  if (DEBUGLEVEL > 4 || debug_log_enabled == 0)
    return;

  fprintf(buglog, "IPC @ 0x%p\n", ipc);
  fprintf(buglog, "  opcode: %04X, uses %X set %X\n", ipc->opcode, ipc->used,
          ipc->set);
  fprintf(buglog, "  src = %08X\n", ipc->src);
  fprintf(buglog, "  dst = %08X\n", ipc->dst);
  fprintf(buglog, "  fn  = %p\n", ipc->function);
  // if ( !ipc->function)
  //{
  //     fprintf(buglog,"**DANGER*** No function pointer!\n");
  // }
  fprintf(buglog, "next  = %p\n", ipc->next);
  fprintf(buglog, "length= %d\n", ipc->wordlen);
  fflush(buglog);

  // check_iib();
  // printf("Next  = %0X\n",ipc->next);
}
#else
void cpu68k_printipc(t_ipc *ipc) {}
#endif

/* fill in ipc */
void cpu68k_ipc(uint32 addr68k, t_iib *iib, t_ipc *ipc)
{
  t_type type;
  uint32 *p = NULL, a9, adr;
  // uint16 rfn;
  // uint8 xaddr[32];
  uint8 *addr;
  addr = NULL;

#define MMU_FAIL_CHECK                                         \
  if (abort_opcode == 1)                                       \
  {                                                            \
    DEBUG_LOG(200, "DANGER! GOT abort_opcode=1 unexpectedly"); \
    ipc->function = NULL;                                      \
    ipc->opcode = 0xf33f;                                      \
    return;                                                    \
  }
#define RET_FAIL          \
  {                       \
    ipc->function = NULL; \
    ipc->opcode = 0xf33f; \
    return;               \
  }
#define MMU_TRY(A)         \
  {                        \
    abort_opcode = 2;      \
    A;                     \
    if (abort_opcode == 1) \
    {                      \
      mmuflush(0x2000);    \
      abort_opcode = 2;    \
      A;                   \
      MMU_FAIL_CHECK;      \
    }                      \
    abort_opcode = 0;      \
  }

#ifdef DEBUG
  checkcontext(context, "pre-entry to cpu68k_ipc");
#endif

  if (ipc == NULL)
  {
    DEBUG_LOG(200, "NULL ipc.\nLet the bodies hit the floor... 1, Nothin' wrong with me. 2, Nothing wrong with me, 3. Nothing wrong with me. 4. Nothing wrong with me.");
    EXIT(123, 0, "Received NULL IPC! This usually indicates a corrupt operating system.");
  }

  if (iib == NULL)
  {
    DEBUG_LOG(200, "NULL iib.\nLet the bodies hit the floor... 1, Nothin' wrong with me. 2, Nothing wrong with me, 3. Nothing wrong with me. 4. Nothing wrong with me.");
    EXIT(123, 0, "Received NULL IIB! This usually indicates a corrupt operating system.");
  }

  addr68k &= ADDRESSFILT;

  a9 = ((addr68k)) >> 9;
  adr = ((addr68k + 11)) >> 9; // test version 11 bytes later of the same.

  MMU_TRY(ipc->opcode = fetchword(addr68k));
  ipc->clks = iib->clocks;
  ipc->wordlen = 1;
  ipc->used = iib->flags.used;
  ipc->set = iib->flags.set;

  if ((iib->mnemonic == i_Bcc) || (iib->mnemonic == i_BSR))
  {
    /* special case - we can calculate the offset now */
    /* low 8 bits of current instruction are addr+1 */
    // ipc->src = (sint32)(*(sint8 *)(addr + 1));
    MMU_TRY(ipc->src = (sint32)((sint8)(fetchbyte(addr68k + 1))))
    // DEBUG_LOG(100,"i_Bcc @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)ipc->src,(long)ipc->opcode);

    if (ipc->src == 0)
    {
      MMU_TRY(ipc->src = (sint32)(sint16)(fetchword(addr68k + 2)));
      // DEBUG_LOG(100,"i_Bcc2 @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)ipc->src,(long)ipc->opcode);
      ipc->wordlen++;
    }

    ipc->src += addr68k + 2; /* add PC of next instruction */
    // DEBUG_LOG(100,"i_Bcc2 @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)ipc->src,(long)ipc->opcode);
    return;
  }

  if (iib->mnemonic == i_DBcc || iib->mnemonic == i_DBRA)
  {
    /* special case - we can calculate the offset now */
    PAUSE_DEBUG_MESSAGES();
    MMU_TRY(ipc->src = (sint32)(sint16)fetchword(addr68k + 2));
    RESUME_DEBUG_MESSAGES();
    ipc->src += addr68k + 2; /* add PC of next instruction */
    ipc->wordlen++;
    // DEBUG_LOG(100,"i_DBcc/DBRA @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)ipc->src,(long)ipc->opcode);
    return;
  }

  addr += 2;
  addr68k += 2;

  // check_iib();

  for (type = 0; type <= 1; type++)
  {
    p = (type == tp_src) ? &(ipc->src) : &(ipc->dst);

    switch (type == tp_src ? iib->stype : iib->dtype)
    {
    case dt_Adis:
      MMU_TRY(*p = (sint32)(sint16)(fetchword(addr68k)));
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;

    case dt_Aidx:
      // 68K Ref says: Address Register Indirect with Index requires one word of extension formatted as:
      // D/A bit 15, reg# bits 14,13,12, Word/Long bit 11.  Bits 10-8 are 0's.  low 8 bits are displacement

      /*    case dt_Aidx:  // original generator code
            *p = (sint32)(sint8)addr[1];
            *p = (*p & 0xFFFFFF) | (*addr) << 24;
            ipc->wordlen++;
            addr += 2;
            addr68k += 2;
            break;
      */

      MMU_TRY(*p = (sint32)(sint8)fetchbyte(addr68k + 1)); // displacement (low byte of extension)
      if (type == tp_src)
      {
        MMU_TRY(ipc->sreg = fetchbyte(addr68k));
      }
      else
      {
        MMU_TRY(ipc->dreg = fetchbyte(addr68k));
      }

      // DEBUG_LOG(100,"dt_Adix @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);

      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;

    case dt_AbsW:
      PAUSE_DEBUG_MESSAGES();
      MMU_TRY(*p = (sint32)(sint16)fetchword(addr68k));
      RESUME_DEBUG_MESSAGES();
      ipc->wordlen++;
      // DEBUG_LOG(100,"dt_AbsW @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      addr += 2;
      addr68k += 2;
      break;

    case dt_AbsL:
      //*p = (uint32)((LOCENDIAN16(*(uint16 *)addr) << 16) +
      //              LOCENDIAN16(*(uint16 *)(addr + 2)));
      MMU_TRY(*p = fetchlong(addr68k));
      // DEBUG_LOG(100,"dt_AbsL @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      ipc->wordlen += 2;
      addr += 4;
      addr68k += 4;
      break;

    case dt_Pdis:
      PAUSE_DEBUG_MESSAGES();
      MMU_TRY(*p = (sint32)(sint16)fetchword(addr68k));
      RESUME_DEBUG_MESSAGES();
      *p += addr68k; /* add PC of extension word (this word) */
      // DEBUG_LOG(100,"dt_Pdis @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;

    case dt_Pidx:
      PAUSE_DEBUG_MESSAGES();
      /*
          original code was:
                *p = ((sint32)(sint8)addr[1]) + addr68k;
                *p = (*p & 0x00FFFFFF) | (*addr) << 24;
                ipc->wordlen++;
                addr += 2;
                addr68k += 2;
      */
      MMU_TRY(*p = ((sint32)(sint8)(fetchbyte(addr68k + 1)) + addr68k));
      // MMU_TRY( *p = (*p & 0x00FFFFFF) | (fetchbyte(addr68k)) << 24      );  // 20180321 adding this guy back in to test a theory - yup this causes PC bits 24-31 to be set, ytho?  **
      // MMU_TRY( *p = (*p & 0x00FFFFFF) | (fetchbyte(addr68k)) << 24      );  // 20200724 - no good, puts trash in MSB // 2020.07.22 re-enabling to test if PC=a0xxxxxx comes back
      if (type == tp_src)
      {
        MMU_TRY(ipc->sreg = fetchbyte(addr68k));
      }
      else
      {
        MMU_TRY(ipc->dreg = fetchbyte(addr68k));
      }
      RESUME_DEBUG_MESSAGES();
      // DEBUG_LOG(100,"dt_Pidx @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;

    case dt_ImmB:
      /* low 8 bits of next 16 bit word is addr+1 */
      PAUSE_DEBUG_MESSAGES();
      MMU_TRY(*p = (uint32)fetchbyte(addr68k + 1));
      RESUME_DEBUG_MESSAGES();
      // DEBUG_LOG(100,"dt_ImmB @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;

    case dt_ImmW:
      PAUSE_DEBUG_MESSAGES();
      MMU_TRY(*p = (uint32)fetchword(addr68k));
      RESUME_DEBUG_MESSAGES();
      // DEBUG_LOG(100,"dt_ImmW @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      ipc->wordlen++;
      addr += 2;
      addr68k += 2;
      break;

    case dt_ImmL:
      PAUSE_DEBUG_MESSAGES();
      MMU_TRY(*p = (uint32)fetchlong(addr68k)); // ((LOCENDIAN16(*(uint16 *)addr) << 16) +  LOCENDIAN16(*(uint16 *)(addr + 2)));
      RESUME_DEBUG_MESSAGES();
      // DEBUG_LOG(100,"dt_ImmL @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      ipc->wordlen += 2;
      addr += 4;
      addr68k += 4;
      break;

    case dt_Imm3:
      if (type == tp_src)
      {
        *p = (ipc->opcode >> iib->sbitpos) & 7;
        // DEBUG_LOG(100,"dt_Imm3 src @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      else
      {
        *p = (ipc->opcode >> iib->dbitpos) & 7;
        // DEBUG_LOG(100,"dt_Imm3 dst @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      break;

    case dt_Imm4:
      if (type == tp_src)
      {
        *p = (ipc->opcode >> iib->sbitpos) & 15;
        // DEBUG_LOG(100,"dt_Imm4 src @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      else
      {
        *p = (ipc->opcode >> iib->dbitpos) & 15;
        // DEBUG_LOG(100,"dt_Imm4 dst @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      break;

    case dt_Imm8:
      if (type == tp_src)
      {
        *p = (ipc->opcode >> iib->sbitpos) & 255;
        // DEBUG_LOG(100,"dt_Imm8 src @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      else
      {
        *p = (ipc->opcode >> iib->dbitpos) & 255;
        // DEBUG_LOG(100,"dt_Imm8 dst @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      break;

    case dt_Imm8s:
      if (type == tp_src)
      {
        *p = (sint32)(sint8)((ipc->opcode >> iib->sbitpos) & 255);
        // DEBUG_LOG(200,"dt_Imm8s src @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      else
      {
        *p = (sint32)(sint8)((ipc->opcode >> iib->dbitpos) & 255);
        // DEBUG_LOG(200,"dt_Imm8s dst @ %08lx target:%08lx opcode:%04lx",(long)addr68k,(long)*p,(long)ipc->opcode);
      }
      break;

    default:
      break;
    }
  }
  /******************* FUN ENDS HERE ***********************************/
#ifdef DEBUG
  check_iib();
  checkcontext(context, "exit from cpu68k_ipc");
#endif
  // DEBUG_LOG(200,"returned.");
}

// keep me - I've been checked.
void init_ipct_allocator(void)
{
  uint32 i, j;
  t_ipc_table *ipct;

#ifdef DEBUG
  if (iipct_mallocs != 0 || ipcts_allocated != 0)
    EXIT(409, 0, "=== suspect init_ipct_allocator is being called more than once without freeing ===");
#endif

  iipct_mallocs = 0;
  ipcts_allocated = 0;
  ipcts_used = 0;
  ipcts_free = 0;

  DEBUG_LOG(100, "init ipct_allocator.");
  // clear our table of pointers
  for (i = 0; i < MAX_IPCT_MALLOCS; i++)
    ipct_mallocs[i] = NULL;

  if ((ipct_mallocs[0] = (t_ipc_table *)calloc(initial_ipcts, (sizeof(t_ipc_table)))) == NULL) // Uninitialised value was created by a heap allocation
  {
    EXIT(85, 0, "Out of memory while allocating initial ipc list ");
  }
  sipct_mallocs[0] = initial_ipcts; // save the size of the first one we got.

  /* ----- Add all of them to the free ipc_linked list  -----*/
  ipct = ipct_free_head = ((t_ipc_table *)ipct_mallocs[0]);

  for (i = 0; i < initial_ipcts; i++)
  {
    ipct[i].next = &(ipct[i + 1]);
    ipct[i].address = 0xf33f1234;
    ipct[i].used = 0;
    ipct[i].context = -1;
    for (j = 0; j < 256; j++)
    {
      ipct[i].ipc[j].function = NULL;
      ipct[i].ipc[j].used = 0;
      ipct[i].ipc[j].set = 0;
      ipct[i].ipc[j].opcode = 0xf33f;
      ipct[i].ipc[j].src = 0;
      ipct[i].ipc[j].dst = 0;
    }
  }

  i--; // because i is now 1 over initial_ipcts
  ipct[i].next = NULL;
  ipct_free_tail = &ipct[i];
  ipcts_used = 0;
  ipcts_allocated = initial_ipcts;
  ipcts_free = i;

  DEBUG_LOG(100, "zzzzzzz ipct land allocated:: %p -to- %p", ipct_mallocs[0], (void *)(ipct_mallocs[0] + initial_ipcts * sizeof(t_ipc_table)));
}

// Keep me, I've been checked.

// *** DANGER YOU MUST DO mt->table=NULL immediately after calling this function to avoid lots of grief!  It can't do it for you!

void free_ipct(t_ipc_table *ipct)
{
  int i;
  t_ipc *ipc = NULL;
  if (!ipct)
    return;

  if (ipct == (void *)0xffffffffffffffff)
  {
    DEBUG_LOG(200, "Was passed 0xffffffffffffffff to free!");
    return;
  }

  if (!ipct->used)
  {
    DEBUG_LOG(200, "Attempt to free already free ipct at %p, current pc:%d/%08x, current ipct:%p", ipct, context, pc24, mmu_trans_all[context][(pc24 & ADDRESSFILT) >> 9].table);
    return;
  }

  if (mmu_trans_all[context][(pc24 & ADDRESSFILT) >> 9].table == ipct)
  {
    DEBUG_LOG(200, "*BUG* attempt to free the current IPCT, allowing it at %p, current pc:%d/%08x, current ipct:%p", ipct, context, pc24, mmu_trans_all[context][(pc24 & ADDRESSFILT) >> 9].table);
    // debug_on("free_ipct=current pc");
    // return;
  }

  DEBUG_LOG(200, "Freeing IPCT at %p:", ipct);

  // check_ipct_counts(__FUNCTION__,__LINE__);

  // clear each of the IPC's in this blocks so incase we accidentally re-used them
  for (i = 0; i < 256; i++)
  {
    ipct->ipc[i].function = NULL;
    ipct->ipc[i].opcode = 0xf33f; // these two signal an unused/free/invalid IPC
    ipct->ipc[i].used = 0;
    ipct->ipc[i].set = 0;
    ipct->ipc[i].sreg = 0;
    ipct->ipc[i].dreg = 0;
    ipct->ipc[i].src = 0;
    ipct->ipc[i].dst = 0;
  }

  ipct->used = 0;    // mark it as free
  ipct->next = NULL; // the next in the free chain of ipc blocks isn't yet here.
  // add this ipct to the next pointer of the last ipct, which is pointed to by free-tail.
  ipct_free_tail->next = ipct;
  ipcts_free++;
  ipcts_used--;          // update free/used counts
  ipct_free_tail = ipct; // this ipct is now the tail of the free chain

  // check_ipct_counts(__FUNCTION__,__LINE__);
}

void free_all_ipcts(void)
{
  int i;
  unsigned long sum = 0;

  for (i = 0; i < MAX_IPCT_MALLOCS; i++)
    if (ipct_mallocs[i] != NULL)
    {
      DEBUG_LOG(200, "ipct[%d]=%p count:%uld", i, ipct_mallocs[i], sipct_mallocs[i]);
      free(ipct_mallocs[i]);
      ipct_mallocs[i] = NULL;
      sum += sipct_mallocs[i];
      sipct_mallocs[i] = 0;
    }
  DEBUG_LOG(200, "--------------------------------------------------------------------------------------------")
  DEBUG_LOG(200, "Freed all IPCTs: %ld mallocs were done, %ld count, %ld bytes", (long)iipct_mallocs, (long)sum, (long)(sum * sizeof(t_ipc_table)));

  iipct_mallocs = 0;
  ipcts_allocated = 0;
  ipcts_used = 0;
  ipcts_free = 0;
  ipct_free_head = NULL;
  ipct_free_tail = NULL;

  for (int c = 0; c < 4; c++)
    for (int a9 = 0; a9 < 32768; a9++)
      mmu_trans_all[c][a9].table = NULL;

  if (ipcs)
  {
    free(ipcs);
    ipcs = NULL;
  }
}

//---- Take an IPC off the top of the free chain and return it to the caller
t_ipc_table *get_ipct(uint32 address)
{
  int64 size_to_get, i, j;
  t_ipc_table *ipct = NULL;
  // check_iib();
  address &= ADDRESSFILT;
  // check_ipct_counts(__FUNCTION__,__LINE__);

  // if we already have one for this address, just return it, ipcs inside it will be overwritten as needed.
  if (mmu_trans_all[context][(address) >> 9].table)
  {
    if (mmu_trans_all[context][(address) >> 9].address == ((address & 0x00fffe00)))
    {
      DEBUG_LOG(200, "Recycling IPCT at mmu_trans_all[%d][%08x] for address %08x", context, (address & 0x00fffe00) >> 9, address);
      return mmu_trans_all[context][(address) >> 9].table;
    }
  }

#ifdef DEBUG
  for (long i = 0; i < iipct_mallocs; i++)
    for (long s = 0; s < sipct_mallocs[i]; s++)
    {
      if (ipct_mallocs[i][s].context == context && ipct_mallocs[i][s].address == (address & 0x00fffe00))
      {
        DEBUG_LOG(200, "Found lost IPCT at ipct_mallocs[%ld][%ld] for context:%d address %08x used flag:%d", i, s, context, address, ipct_mallocs[i][s].used);
        if (mmu_trans_all[context][(address) >> 9].table == &ipct_mallocs[i][i])
          DEBUG_LOG(200, "^ this is the actual one, the others are lost");
      }
    }

  if (ipcts_free < 0)
    EXITR(0, NULL, "LisaEm bug ipcts_free is negative!!");
  if (ipcts_used < 0)
    EXITR(0, NULL, "LisaEm bug ipcts_used is negative!!");
  if (ipcts_free + ipcts_used + 1 != ipcts_allocated)
    DEBUG_LOG(200, "LisaEm bug ipcts_free+ipcts_used !=ipcts_allocated %lld+%lld!=%lld!!",
              (long long)ipcts_free, (long long)ipcts_used, (long long)ipcts_allocated);

#ifdef TEST_FREE_IPCTS_ARE_EMPTY
// check_ipct_counts(__FUNCTION__,__LINE__);
#endif
#endif

  /*--- Do we have any free ipcs? if so take one from the head of the list and return it. ---*/
  if (ipcts_free > 0 && ipct_free_head != NULL)
  {
    ipcts_free--;
    ipcts_used++; // update free/used count

    ipct = ipct_free_head;       // pop an ipct off the chain
    ipct_free_head = ipct->next; // the next one on the chain is the new head

    ipct->next = NULL;
    ipct->used = 1;          // mark it as used
    ipct->context = context; // log context and address for debugging purposes
    ipct->address = (address & 0x00fffe00);
    return ipct;
  }
  else /*---- Nope! We're out of IPCt's, allocate some more.  ----*/
  {
    /*--- Did we call Malloc too many times? ---*/
    if ((iipct_mallocs++) > MAX_IPCT_MALLOCS)
    {
      EXITR(2, NULL, "Excessive mallocs of ipct's recompile with more!");
    }

    // possible off by 1?
    if (ipcts_free)
      DEBUG_LOG(200, "There are no free ipcts, but ipcts_free is non zero! %lld", (long long)ipcts_free);
    ipcts_free = 0;
    size_to_get = (100 * ipcts_allocated / IPCT_ALLOC_PERCENT) + 1; // add a percentange of what we have, least 1
    if (!(ipct_mallocs[iipct_mallocs] = (t_ipc_table *)calloc(size_to_get, sizeof(t_ipc_table))))
    {
      DEBUG_LOG(200, "Out of RAM %ld ipcts allocated so far, %ld are free, %ld used, %ld mallocs done", (long)ipcts_allocated, (long)ipcts_free, (long)ipcts_used, (long)iipct_mallocs);
      // check_ipct_counts(__FUNCTION__,__LINE__);
      EXITR(86, NULL, "Out of memory while allocating more ipct's");
    }
    DEBUG_LOG(200, "ipct[%d]=%p count:%lld", iipct_mallocs, ipct_mallocs[iipct_mallocs], size_to_get);

    // Zap the new IPC's and link them to the free linked list ---  since we're fresh out
    // of ipc table entries what we've just allocated is the new head of the free list
    sipct_mallocs[iipct_mallocs] = size_to_get;
    ipcts_allocated += size_to_get;
    ipct_free_head = &ipct_mallocs[iipct_mallocs][1]; // free head is the 2nd ipct in the malloc as we return the first to the caller
    ipct = ipct_mallocs[iipct_mallocs];

    {
      long int sum = 0;
      for (i = 0; i < iipct_mallocs; i++)
        sum += sipct_mallocs[i];
      DEBUG_LOG(200, "Allocated more ipct's.  we now have %ld mallocs, ipc_allocated are:%ld total ipcts:%ld total bytes: %ld", (long)iipct_mallocs, (long)ipcts_allocated,
                (long)sum, (long)(sum * sizeof(t_ipc_table)));
    }
    // check_ipct_counts(__FUNCTION__,__LINE__);

    ipcts_free = size_to_get - 1;    // no need to add to the tail since we know we have none left
                                     // no need to add to ipcts_free since it should be zero at this point.
    for (i = 1; i < ipcts_free; i++) // start at 1 since we will return ipct0 to caller  //20180323
    {
      ipct[i].next = &ipct[i + 1]; // link it to the next free one in the chain
      ipct[i].used = 0;            // mark it as free
      ipct[i].address = 0xf33f1234;
      ipct[i].context = -1;
// mark opcode with illegal one incase we execute from here.
#ifdef DEBUG
      for (j = 0; j < 256; j++)
      {
        ipct[i].ipc[j].function = NULL;
        ipct[i].ipc[j].used = 0;
        ipct[i].ipc[j].set = 0;
        ipct[i].ipc[j].opcode = 0xf33f;
        ipct[i].ipc[j].src = 0;
        ipct[i].ipc[j].dst = 0;
      }
#endif
    }
    ipct_free_tail = &ipct[i - 1]; // fix the tail. - shyte, did I have an off by 1 here?
    ipct_free_tail->next = NULL;   // last free one in chain has no next link.
  }

  ipct[0].next = NULL;
  ipct[0].used = 1; // mark it as used
  ipct[0].context = context;
  ipct[0].address = (address & 0x00fffe00);
#ifdef DEBUG
  for (j = 0; j < 256; j++)
  {
    ipct[0].ipc[j].function = NULL;
    ipct[0].ipc[j].used = 0;
    ipct[0].ipc[j].set = 0;
    ipct[0].ipc[j].opcode = 0xf33f;
    ipct[0].ipc[j].src = 0;
    ipct[i].ipc[j].dst = 0;
  }
#endif
  // check_ipct_counts(__FUNCTION__,__LINE__);
  return &ipct[0];
}

/*
    Need to check all of these.  Remember:  Pages are 512 bytes long, it's ok to leave one at the end.
    should any block be greater than 512 bytes long, chopt it there and force it to set the flags if it
    does set any flags.  Also, we should have each function set it's own clocks.  Yes, it's slower, but
    it's far more acurate.

*/

void checkNullIPCFN(void)
{
  long int i;
  int q = 0;

  for (i = 0; i < 65536 * 2; i++)
    if (!cpu68k_functable[i])
    {
      DEBUG_LOG(100, "Null function: for opcode %04lx:%ld", (long)(i >> 1), (long)(i & 1));
      q = 1;
    }
  if (q)
  {
    EXIT(13, 9, "failed checkNullIPCFN");
  }
  // check_iib();
}

// 20061223 need to optimize this - it's the heaviest fn according to gprof.
t_ipc_table *cpu68k_makeipclist(uint32 pc)
{
  // Generator was meant to work from ROMs, not from volatile RAM.
  // In fact, the original code checks to see if code is executing in RAM,
  // and if it is, it disables the IPC's.  But this would be slow.  In the
  // Lisa emulator, the problem is that we have an MMU and we also have
  // virtual memory which pages things in and out of memory.  So I had to
  // change the way Generator's IPC's work.  For one thing it has to release
  // any IPC's that are no longer needed when the MMU releases or changes the
  // segment that they're built for.  Repeated calls to malloc and free would
  // fragment RAM and cause problems.  So instead I opted to replace the IPC
  // arrays with ones attached to the MMU pages and got rid of the linked list
  // and array based hash.
  //
  // Since this is the only function that ever does ipc--, we can save
  // some RAM by keeping an array of pointers to ipc's so we can walk
  // backwards when we need to. This potentially saves us RAM and access
  // speed in the IPC linked list space, though here we give up 32k or 64k,
  // but only for this function, and if it goes over it will alloc more.
  //
  // So that's an acceptable price to pay.
  //
  // (access speed is of course the overhead needed to also maintain a
  // previous IPC pointer. Memory depends on how many IPC's we wind up
  // with, and keeping the IPC structures small saves the L1/L2 caches
  // from reading in previous pointers that are unnecessary for the rest
  // of the code except for this function.)  -- Ray Arachelian.

  // int size = 0;
  uint32 original_pc = pc;
  pc &= ADDRESSFILT;

  t_iib *iib, *illegaliib; //*myiib,
  uint32 instrs = 0;
  uint16 required;
  uint32 xpc = pc | 511;
// int i;
#ifdef DEBUG
  int dbx = debug_log_enabled;
#endif

  mmu_trans_t *mmu_trn;
  t_ipc *ipc = NULL; // *opc; myipc,
  // t_ipc **ipcs; // since this is the only function that walks backwards in the IPC's we
  //  collect them here temporarily in an array of pointers.  Since the malloc should be
  //  immediately followed by a free (reallocs granted, yes), it shouldn't fragment ram too much.

  // int ipcptr=0;

  t_ipc_table *rettable, *table;

  int ix = 0; // index to the ipcs.
  abort_opcode = 0;

#ifdef DEBUG
  DEBUG_LOG(1000, "PC I filtered to 24 bit: %08x", pc);
  // check_iib();
  checkcontext(context, "pre-entry to cpu68k_makeipc");
#endif

  if (ipcs == NULL)
  {
    ipcs = (t_ipc **)calloc(sizeof(t_ipc *), ipcs_to_get);
    if (!ipcs)
      EXITN(0, 0, "Out of memory in makeipclist trying to get ipc pointers!");
  }

  mmu_trn = &mmu_trans[((pc) >> 9)];
  if (mmu_trn->readfn == bad_page)
  {
    free(ipcs);
    return NULL;
  }

  if (mmu_trn && mmu_trn->table)
    ipc = &(mmu_trn->table->ipc[(((pc) >> 1) & 0xff)]); // Get the pointer to the IPC.

  if (!ipc)
  {
    DEBUG_LOG(1000, "Nope - calling get_ipct()");
    mmu_trn->table = get_ipct(pc);
    if (!mmu_trn->table)
    {
      EXITR(21, NULL, "Couldn't get IPC Table! Doh!");
    }
#ifdef DEBUG
    if (mmu_trn->table == (void *)0xffffffffffffffff)
    {
      EXITR(21, NULL, "Couldn't get IPC Table! Got -1! Doh!");
    }
    if (pc & 1)
      EXITR(14, NULL, "odd pc!");
#endif

    ipc = &(table->ipc[((pc >> 1) & 0xff)]);
    DEBUG_LOG(200, "ipc is now %p at pc %08lx max %08lx", ipc, (long)pc, (long)xpc);
    if (!ipc)
    {
      EXITR(501, NULL, "cpu68k_makeipclist: But! ipc is null!");
    }
  }

  // check_ipct_counts(__FUNCTION__,__LINE__);

  table = (mmu_trn->table);             // we might not yet have table until we get here.
  rettable = table;                     // save this to return, we now have a pointer to the table we want to return.
  illegaliib = cpu68k_iibtable[0x4afc]; // cache for later.

  xpc = pc | 0x1ff; // set the end to the end of the current page. (was 1fe)

  instrs = 0;
  do
  {
    uint16 opcode;

    instrs++;
    // check_ipct_counts(__FUNCTION__,__LINE__);
    // check_iib();
    // DEBUG_LOG(100,"ipc is now %p at pc %06x max %06x",ipc,pc,xpc);

    // Get the IIB, if it's NULL, then get the IIB for an illegal instruction.
    abort_opcode = 2;
    opcode = fetchword((pc));
    if (abort_opcode == 1) // flush mmu and retry once
    {
      mmuflush(0x2000);
      abort_opcode = 2;
      opcode = fetchword((pc));
      if (abort_opcode == 1)
      {
        DEBUG_LOG(200, "DANGER! GOT abort_opcode=1 the 2nd time!");
        free(ipcs);
        ipcs = NULL;
        return NULL;
      }
    }
    abort_opcode = 0;
    iib = cpu68k_iibtable[opcode]; // iib =  myiib ? myiib : illegaliib;  *** REMOVED DUE TO ILLEGAL OPCODES *****
    if (!iib)
    {
      DEBUG_LOG(200, "There's no proper IIB for the possibly illegal instruction opcode %04x @ pc=%08x\n", opcode, pc);
      free(ipcs);
      ipcs = NULL;
      return NULL;
    }

    if (!iib)
    {
      EXITR(53, NULL, "There's no proper IIB for the possibly illegal instruction opcode %04x @ pc=%08lx\n", opcode, (long)pc);
    }
    if (!ipc)
    {
      EXITR(54, NULL, "Have a cow man! ipc=NULL\n");
    }

    cpu68k_ipc((pc), iib, ipc);
    if (abort_opcode == 1)
    {
      free(ipcs);
      ipcs = NULL;
      return rettable;
    } // got MMU Exception, but return what we have.

    DEBUG_LOG(200, "******* for next ip at instrs %ld, pc=%08lx, opcode=%04lx", (long)instrs, (long)pc, (long)opcode);
    // cpu68k_printipc(ipc);
    pc += (iib->wordlen) << 1;

#ifdef DEBUG
    {
      char text[1024];
      diss68k_gettext(ipc, text);
      DEBUG_LOG(200, "ipc ipc ipc opcode I got %08lx :%s instr#:%ld", (long)pc, text, (long)instrs);
    }
#endif

    // grow the list of ipcs if we need to.
    if (instrs >= ipcs_to_get)
    {
      uint32 opc = original_pc & ADDRESSFILT;
      int count = 0;
      ALERT_LOG(0, "=============== FATAL ERROR: Welcome to the Realms of Chaos ===============================================");
      ALERT_LOG(0, "Entered at: %08x", original_pc);
      while (opc < pc)
      {
        abort_opcode = 2;
        opcode = fetchword(pc);
        printf("%04x ", opcode);
        opc++;
        count++;
        if (count > 15)
        {
          count = 0;
          printf("\n%08x: ", opc);
        }
      }
#ifdef DEBUG
      ALERT_LOG(0, "dumpallmmu");
      debug_on("chaos");
      dumpram("chaos");
      dumpallmmu();
      debug_off();
      ALERT_LOG(0, "MMU dump completed");
#else
      ALERT_LOG(0, "\n\n\nDebug has not been compiled in, cannot dump mmu\n\n\n");
#endif
      EXITR(24, NULL, "Welcome to the realms of chaos! I'm dealing with over %ld instructions in a basic block, this should not happen, %ld ipcs! %ld/%ld/%ld pc=%ld/%08lx",
            (long)instrs, (long)ipcs_to_get, (long)segment1, (long)segment2, (long)start, (long)context, (long)pc);
      exit(255);
    }

    DEBUG_LOG(200, "Copying ipc to ipcs buffer");
    ipcs[instrs - 1] = ipc; // copy pointer to ipcs buffer
    // check_iib();
    DEBUG_LOG(200, "XPC I set as limit: %08lx pc is %08lx", (long)xpc, (long)pc);
    if (pc > xpc) // did we step over the page? If so, setup for the next page.
    {
      xpc = pc | 0x1ff;
      DEBUG_LOG(200, "XPC I set as limit: %08x pc is %08x", xpc, pc);
      mmu_trn = &mmu_trans[(pc >> 9) & 32767];
      table = mmu_trn->table; // we might not yet have table until we get here.
      if (!table)
      {
        DEBUG_LOG(1000, "Nope didn't get the table after crossing page boundary- calling get_ipct()");
        mmu_trn->table = get_ipct((pc)); // allocate an ipc table for this mmu_t
        table = mmu_trn->table;
        if (!table)
          EXITR(99, NULL, "Couldnt get IPC Table! Doh!\n");
      }
      ipc = &(table->ipc[((pc >> 1) & 0xff)]); // setup next ipc
      DEBUG_LOG(200, "ipc is now %p at pc %08lx max %08lx", ipc, (long)pc, (long)xpc);
    }
    else // No we didn't go over the MMU page limited yet, it's cool
    {
      if (!mmu_trn->table)
      {
        mmu_trn->table = get_ipct((pc));
        table = mmu_trn->table;
        if (!table)
          EXITR(27, NULL, "Couldnt get IPC Table! Doh!");
      }
      ipc = &(table->ipc[((pc >> 1) & 0xff)]); // ipc points to the ipc in mmu_trans
      DEBUG_LOG(200, "ipc is now %p at pc %08lx max %08lx", ipc, (long)pc, (long)xpc);
    }
    DEBUG_LOG(200, "ipc is now %p at pc %08lx max %08lx", ipc, (long)pc, (long)xpc);

  } while (!iib->flags.endblk);

  // check_iib();
  DEBUG_LOG(200, "ipc is now %p at pc %06lx max %06lx  WE DID %d instructions! pc=%08lx xpc=%08lx**** \n\n", ipc, (long)pc, (long)xpc, instrs, (long)pc, (long)xpc);

  // Do we need this?  correct final ipc out of the loop.
  // iib=cpu68k_iibtable[(fetchword(pc))]; // iib =  myiib ? myiib : illegaliib;  *** REMOVED DUE TO ILLEGAL OPCODES *****
  // if ( !iib ) {DEBUG_LOG(100,"Worse yet, there's no proper IIB for the illegal instruction opcode 0x4afc!"); EXIT(555);}
  // if ( !ipc )  { DEBUG_LOG(100,"I'm about to do something really stupid.(ipc=NULL)\n");  EXIT(1);  }
  //
  // cpu68k_ipc(pc, iib, ipc);
  // table->t.clocks += iib->clocks;
  DEBUG_LOG(200, "hangover: %08lx,%ld", (long)pc, (long)instrs);
  cpu68k_printipc(ipc);

  // table->ipc[((pc>>1) & 0xff)]=NULL;      // make sure next one is not set up.
  ////check_iib();
  //  *(int *)ipc = 0; // wtf?  why set it to null? indicate end of list. yup. yup.
  if (instrs == 2)
  {
    DEBUG_LOG(200, "*~*~*~*~*~*~ in 2instrs ipc is now %p at pc %06lx max %06lx", ipc, (long)pc, (long)xpc);
    ipc = ipcs[instrs - 1 - 1]; // ipc--
    DEBUG_LOG(200, "ipc is now %p at pc %08lx max %06lx", ipc, (long)pc, (long)xpc);

    if (iib->mnemonic == i_Bcc && ipc->src == xpc) // valgrind: ==24726== Conditional jump or move depends on uninitialised value(s)
    {                                              // RA list->pc <- xpc
                                                   /* we have a 2-instruction block ending in a branch to start */
      ipc = ipcs[instrs - 1 + 1];                  // ipc++
      DEBUG_LOG(200, "ipc is now %p at pc %08lx max %08lx instruction # %ld", ipc, (long)pc, (long)xpc, (long)instrs - 1);
      // Get the IIB, if it's NULL, then get the IIB for an illegal instruction.

      iib = cpu68k_iibtable[ipc->opcode];
      if (!iib)
      {
        DEBUG_LOG(100, "Boink! Got null IIB for this opcode=%04lx", (long)ipc->opcode);
      }
    }
  }
  // check_iib();

  // This walks the ipc's backwards... this is why we need the ipcs array.
  // otherwise we'd need to set up a set of previous and next link pointers
  // and eat more valuable ram.  this too eats ram, but only once, and about
  // half of what would needed (with a two way linked list, those pointers would
  // have been wasted as they're only used here!)  We could walk the page array
  // of ipcs, but that's slower and it sucks, etc.

  //    ipc = ((t_ipc *) (list + 1)) + instrs - 1;

  ipc->next = NULL; // next pointer of last IPC is always null as there is no next one yet.

  ix = instrs; /*****************/
  //    ipc = ipcs[ix];
  required = 0x1F; /* all 5 flags need to be correct at end */

  // check_ipct_counts(__FUNCTION__,__LINE__);

  DEBUG_LOG(200, "ipc is now %p at pc %06lx max %06lx **** About to correct ipc's: %ld instructions **** \n\n",
            ipc, (long)pc, (long)xpc, (long)instrs);

  while (ix--)
  {
    ipc = ipcs[ix];

    DEBUG_LOG(200, "ipc is now %p at pc %06lx max %06lx ix=%ld", ipc, (long)pc, (long)xpc, (long)ix);
    if (!ipc)
    {
      EXITR(29, NULL, "Null ipc, bye");
    }

    ipc->set &= required;
    required &= ~ipc->set;
    required |= ipc->used;

    ipc->function = cpu68k_functable[(ipc->opcode << 1) + (ipc->set ? 1 : 0)];

    if (!ipc->function)
    {
#ifdef DEBUG
      debug_off();
#endif
      EXITR(3, NULL, "Null IPC fn returned for opcode:%04lx ix=%ld of %ld instrs", (long)ipc->opcode, (long)ix, (long)instrs);
    }

    DEBUG_LOG(200, "ipc is now %p at pc %06lx max %06lx ix=%ld", ipc, (long)pc, (long)xpc, (long)ix);
    if (ix)
      ipcs[ix - 1]->next = ipc; // previous ipc's next link points to current ipc.
  }
  DEBUG_LOG(200, "out of ix-- loop, ix=%ld ipc is now %p at pc %06lx max %06lx **** corrected ipc's: %ld instructions **** \n\n", (long)ix,
            ipc, (long)pc, (long)xpc, (long)instrs);

  if (ipcs)
  {
    free(ipcs);
    ipcs = NULL;
  }
  return rettable;
}

void cpu68k_endfield(void)
{ /* cpu68k_clocks = 0; */
}

// Before we call this, Lisa memory must be allocated, ROM mem must be allocated and loaded!
// the MALLOC pointers must be set to NULL the first time around, otherwise we'd attempt to free garbage.
// these must be properly done in main!

void cpu68k_reset(void)
{
  // int i;

  reg68k_regs = regs.regs; // thar be a poynter, yaaar
  reg68k_pc = regs.pc = fetchlong(4);
  reg68k_regs[15] = regs.regs[15] = fetchlong(0);

  DEBUG_LOG(100, "CPU68K Reset: Context: %ld, PC set to %08lx, SSP set to %08lx\n", (long)context, (long)regs.pc, (long)regs.regs[15]);

  regs.sr.sr_int = 0;
  regs.sr.sr_struct.s = 1; /* Supervisor mode */
  regs.stop = 0;
  reg68k_sr.sr_int = regs.sr.sr_int;

  cpu68k_frames = 0; /* Number of frames */
  virq_start = FULL_FRAME_CYCLES;
  fdir_timer = -1;
  cpu68k_clocks_stop = ONE_SECOND;
  cpu68k_clocks = 0;
  cops_event = -1;
  tenth_sec_cycles = TENTH_OF_A_SECOND;

  // for (i=0; i<MAX_IPCT_MALLOCS; i++)
  //    if (ipct_mallocs[i]!=NULL) {free(ipct_mallocs[i]); ipct_mallocs[i]=NULL;}
  // iipct_mallocs=0;
  //
  // init_ipct_allocator();
  floppy_6504_wait = 0;
  segment1 = 0;
  segment2 = 0;

  // check_ipct_counts(__FUNCTION__,__LINE__);
}
