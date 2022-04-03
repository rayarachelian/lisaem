/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#define IN_REG68K_C

#include <generator.h>
#include <registers.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <reg68k.h>
#include <cpu68k.h>
#include <ui.h>
#include <vars.h>

// __CYGWIN__ wrapper added by Ray Arachelian for LisaEm to prevent crashes in reg68k_ext exec
// is this still needed? looks like both do the same thing.
#ifdef __CYGWIN__
  uint32 reg68k_pc;
  uint32 *reg68k_regs;
  t_sr reg68k_sr;
#else
#if (!(defined(PROCESSOR_ARM) || defined(PROCESSOR_SPARC) || defined(PROCESSOR_INTEL) ))
uint32 reg68k_pc;
uint32 *reg68k_regs;
t_sr reg68k_sr;
#endif
#endif


#define CHECK_HIGH_BYTE_PRESERVE 1



#define LISA_REBOOTED(x)   { ALERT_LOG(0,"rebooting? reg68k_pc:%08lx,pc24:%08lx %16lx",(long)reg68k_pc,(long)pc24,(long)cpu68k_clocks); return x;}
#define LISA_POWEREDOFF(x) { save_pram(); profile_unmount(); lisa_powered_off();  return x;}

//static const int insetjmpland=1; //RA20190601
#define insetjmpland 1 //RA20190601 -- so unused code will get optimzied out

static uint32 last_cpu68k_clocks=0;

static uint32 last_bus_error_pc=0;

static int    nmi_error_trap=0;
static uint32 last_nmi_error_pc=0;

static uint32 nmi_pc=0, nmi_addr_err=0, nmi_clk=0, nmi_stop=0;


uint16 InstructionRegister;
uint8  CPU_function_code;     // cpu function code state output bits (FC2/Fc1/FC0).  // fc2= supervisor mode, fc1/0=program/data access
                              //
                              // should set to CPU_function_code=(reg68k_sr.sr_struct.s<<2) | 1; for normal access
                              //               CPU_function_code=(reg68k_sr.sr_struct.s<<2) | 2; for execution/program access.
                              //

#define SET_CPU_FNC_DATA() {CPU_function_code=(reg68k_sr.sr_struct.s? 4:0) | 1;}
#define SET_CPU_FNC_CODE() {CPU_function_code=(reg68k_sr.sr_struct.s? 4:0) | 2;}


uint8  CPU_READ_MODE;         // 1=read, 0=write (for addr/bus error exceptions)

uint32 pc24=0, lastpc24=0, page;
static int loopy_vector=0;                  // prevent fetch from calling this again.




/*** forward references ***/
extern void lisa_console_output(uint8 c);

#ifdef DEBUG

void dumpmmu(uint8 c, FILE *out);
void validate_mmu_segments(char *from);
void printregs(FILE *buglog,char *tag);
void extprintregs(FILE *buglog,char *tag);

#include <diss68k.h>

#endif

extern char *slrname(uint16 slr);       // from memory.c

#include <diss68k.h>
// Make sure that the bitfield unions work correctly - if they're in the wrong order, they won't.  This double checks
// that ./configure got the right order.

void reg68k_sanity_check_bitorder(void)
{
  int bad=0; uint16 x;
  int64 i64;
  int32 i32;
  union
  {
    uint32 l;
    uint8  c[4];
  } e;


  union {
  struct {
  uint8 h:4;
  uint8 i0:1;
  uint8 i1:1;
  uint8 i2:1;
  uint8 i3:1;
  } hi;
  uint8 i;
  }bf;

  x=reg68k_sr.sr_int;

  reg68k_sr.sr_int=0x2700;
  #define IMSK (reg68k_sr.sr_struct.i0 | (reg68k_sr.sr_struct.i1<<1) | (reg68k_sr.sr_struct.i2<<2) )

   if (sizeof(reg68k_sr)!=2) {bad=1;
                              DEBUG_LOG(0,"expected reg68k_sr to be two bytes! it is %ld instead!",(long)(sizeof(reg68k_sr)) );
                              messagebox("Sanity Check Failed! reg68k_sr is not 2 bytes!", "Compilation Failure!");
                             }

  if (sizeof(uint8)  !=1 ) {bad=1; DEBUG_LOG(0,"Size of uint8  is not 1"); messagebox("Sanity check failed sizeof(uint8)!=1","Sanity check failed");}
  if (sizeof(uint16) !=2 ) {bad=1; DEBUG_LOG(0,"Size of uint16 is not 2"); messagebox("Sanity check failed sizeof(uint16)!=2","Sanity check failed");}
  if (sizeof(uint32) !=4 ) {bad=1; DEBUG_LOG(0,"Size of uint32 is not 4"); messagebox("Sanity check failed sizeof(uint32)!=4","Sanity check failed");}
#ifdef uint64
  if (sizeof(uint64) !=8 ) {bad=1; DEBUG_LOG(0,"Size of uint64 is not 8"); messagebox("Sanity check failed sizeof(uint64)!=8","Sanity check failed");}
#endif

  if (sizeof(int8)  !=1 ) {bad=1; DEBUG_LOG(0,"Size of int8  is not 1"); messagebox("Sanity check failed sizeof(int8)!=1","Sanity check failed");}
  if (sizeof(int16) !=2 ) {bad=1; DEBUG_LOG(0,"Size of int16 is not 2"); messagebox("Sanity check failed sizeof(int16)!=2","Sanity check failed");}
  if (sizeof(int32) !=4 ) {bad=1; DEBUG_LOG(0,"Size of int32 is not 4"); messagebox("Sanity check failed sizeof(int32)!=4","Sanity check failed");}
#ifdef int64
  if (sizeof(int64) !=8 ) {bad=1; DEBUG_LOG(0,"Size of int64 is not 8"); messagebox("Sanity check failed sizeof(int64)!=8","Sanity check failed");}
#endif

  e.l=0xdeadbeef;
  #ifdef  WORDS_BIGENDIAN
    if (e.c[0]!=0xde && e.c[1]!=0xad && e.c[2]!=0xbe && e.c[3]!=0xef )
       {bad=1; DEBUG_LOG(0,"Endian Mismatch - BIGENDIAN defined, but fails test"); messagebox("Sanity check failed: BIGENDIAN defined, but fails test","Sanity check failed");}
  #else
    if (e.c[0]!=0xef && e.c[1]!=0xbe && e.c[2]!=0xad && e.c[3]!=0xde )
       {bad=1; DEBUG_LOG(0,"Endian Mismatch - LITTLE ENDIAN defined, but fails test"); messagebox("Sanity check failed: LITTLE ENDIAN defined, but fails test","Sanity check failed");}

  #endif

  i64=-1;
  i32=-1;

  i64=(i64>>1);
  i32=(i32>>1);

  if (i64!=-1) {bad=1; DEBUG_LOG(0,"64 bit signed right shift of -1 !=-1"); messagebox("Sanity check failed: 64 bit signed right shift of -1 !=-1","Sanity check failed");}
  if (i32!=-1) {bad=1; DEBUG_LOG(0,"32 bit signed right shift of -1 !=-1"); messagebox("Sanity check failed: 32 bit signed right shift of -1 !=-1","Sanity check failed");}

  bf.i=0xf3;

  if (sizeof(bf)!=1) {bad=1; DEBUG_LOG(0,"sizeof(uint8) bitfield!=1!"); messagebox("Sanity check failed: sizeof(uint8) bitfield!=1!","Sanity Check Failure!");}

  #ifdef BYTES_HIGHFIRST
  //  bf.i=f3 bf.hi.h=f bf.hi.i0,1,2,3=0 0 1 1
   if (bf.hi.h  !=15) {bad=1; DEBUG_LOG(0,"bitfield/BYTES_HIGHFIRST hi.h!=f");  messagebox("Sanity check failed: bitfield/BYTES_HIGHFIRST hi.h!=f ","Sanity Check Failure");}
   if (bf.hi.i0 !=0 ) {bad=1; DEBUG_LOG(0,"bitfield/BYTES_HIGHFIRST hi.i0!=0"); messagebox("Sanity check failed: bitfield/BYTES_HIGHFIRST hi.i0!=0","Sanity Check Failure");}
   if (bf.hi.i1 !=0 ) {bad=1; DEBUG_LOG(0,"bitfield/BYTES_HIGHFIRST hi.i1!=0"); messagebox("Sanity check failed: bitfield/BYTES_HIGHFIRST hi.i1!=0","Sanity Check Failure");}
   if (bf.hi.i2 !=1 ) {bad=1; DEBUG_LOG(0,"bitfield/BYTES_HIGHFIRST hi.i2!=1"); messagebox("Sanity check failed: bitfield/BYTES_HIGHFIRST hi.i2!=1","Sanity Check Failure");}
   if (bf.hi.i3 !=1 ) {bad=1; DEBUG_LOG(0,"bitfield/BYTES_HIGHFIRST hi.i3!=1"); messagebox("Sanity check failed: bitfield/BYTES_HIGHFIRST hi.i3!=1","Sanity Check Failure");}


  #else
   // bf.i=f3 bf.hi.h=3 bf.hi.i0,1,2,3=1 1 1 1
   if (bf.hi.h  !=3 ) {bad=1; DEBUG_LOG(0,"bitfield/!BYTES_HIGHFIRST hi.h!=3");  messagebox("Sanity check failed: bitfield/!BYTES_HIGHFIRST hi.h!=3","Sanity Check Failure");}
   if (bf.hi.i0 !=1 ) {bad=1; DEBUG_LOG(0,"bitfield/!BYTES_HIGHFIRST hi.i0!=1"); messagebox("Sanity check failed: bitfield/!BYTES_HIGHFIRST hi.i0!=1","Sanity Check Failure");}
   if (bf.hi.i1 !=1 ) {bad=1; DEBUG_LOG(0,"bitfield/!BYTES_HIGHFIRST hi.i1!=1"); messagebox("Sanity check failed: bitfield/!BYTES_HIGHFIRST hi.i1!=1","Sanity Check Failure");}
   if (bf.hi.i2 !=1 ) {bad=1; DEBUG_LOG(0,"bitfield/!BYTES_HIGHFIRST hi.i2!=1"); messagebox("Sanity check failed: bitfield/!BYTES_HIGHFIRST hi.i2!=1","Sanity Check Failure");}
   if (bf.hi.i3 !=1 ) {bad=1; DEBUG_LOG(0,"bitfield/!BYTES_HIGHFIRST hi.i3!=1"); messagebox("Sanity check failed: bitfield/!BYTES_HIGHFIRST hi.i3!=1","Sanity Check Failure");}
  #endif


  reg68k_sr.sr_int=0x2700; if (IMSK!=7)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 7 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2600; if (IMSK!=6)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 6 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2500; if (IMSK!=5)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 5 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2400; if (IMSK!=4)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 4 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2300; if (IMSK!=3)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 3 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2200; if (IMSK!=2)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 2 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2100; if (IMSK!=1)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 1 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2000; if (IMSK!=0)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expect 0 got %d %c%c%c%c%c%c%c imsk:%d ",IMSK,(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}

  reg68k_sr.sr_int=0x2000; if (!reg68k_sr.sr_struct.s)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expected Supervisor Bit on  %c%c%c%c%c%c%c imsk:%d ",(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 )));  bad=1;}
  reg68k_sr.sr_int=0x0000; if ( reg68k_sr.sr_struct.s)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expected Supervisor Bit off %c%c%c%c%c%c%c imsk:%d ",(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}
  reg68k_sr.sr_int=0x2700; if (!reg68k_sr.sr_struct.s)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expected Supervisor Bit on  %c%c%c%c%c%c%c imsk:%d ",(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 )));  bad=1;}
  reg68k_sr.sr_int=0x0700; if ( reg68k_sr.sr_struct.s)  {DEBUG_LOG(0,"BIT ORDER IS WRONG! Expected Supervisor Bit off %c%c%c%c%c%c%c imsk:%d ",(reg68k_sr.sr_struct.t ? 't':'.'),(reg68k_sr.sr_struct.s ? 'S':'.'),(reg68k_sr.sr_struct.z ? 'z':'.'),(reg68k_sr.sr_struct.x ? 'x':'.'),(reg68k_sr.sr_struct.n ? 'n':'.'),(reg68k_sr.sr_struct.v ? 'v':'.'),        (reg68k_sr.sr_struct.c ? 'c':'.'),((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ))); bad=1;}

  reg68k_sr.sr_int=x;


  if (bad) {
             EXIT(1938,0,"Compilation failure! Sanity Check Failed! reg68k_sr int does not match bitfields!");
           }
}


char niceascii(char c)
{ c &=127;
 if (c<31) c|=32;
 if (c==127) c='.';
 return c;
}


void lisa_addrerror(uint32 addr);

char *getvector(int v);
static int atexitset=0;

// MMU Test Patterns.  1 assumes X flag is preset before. 2 assumes X=0
//                       0         1     2     3        4      5     6      7      8     9      10     11     12     13     14     15    16
uint16 mmupattern1[17]={0xa55a,0x4ab5,0x956b,0x2ad6,0x55ad,0xab5a,0x56b4,0xad69,0x5ad2,0xb5a5,0x6b4a,0xd695,0xad2a,0x5a55,0xb4ab,0x6956,0xd2ad};
uint16 mmupattern2[17]={0xa55a,0x4ab4,0x9569,0x2ad2,0x55a5,0xab4a,0x5694,0xad29,0x5a52,0xb4a5,0x694a,0xd295,0xa52a,0x4a55,0x94ab,0x2956,0x52ad};

static uint8 pending_vector_bitmap=0;

#ifdef DEBUG
extern void append_floppy_log(char *s);

//mmu.c
extern char *printslr(char *x, long size, uint16 slr);
// symbols.c
extern char *getvector(int v);
extern void lisaos_trap5(void);
// glue.c
extern void debug_on(char *reason);
extern void debug_off(void);



#endif

// only two are available: empty or dual parallel
int get_nmi_pending_irq(void) {return 0;}

int get_exs2_pending_irq_empty(void) {return 0;}
int get_exs1_pending_irq_empty(void) {return 0;}
int get_exs0_pending_irq_empty(void) {return 0;}


int get_exs0_pending_irq_2xpar(void) { if (via[3].via[IFR]&via[3].via[IER]) via[3].via[IFR] |=0x80;
                                       if (via[4].via[IFR]&via[4].via[IER]) via[4].via[IFR] |=0x80;
                                       return (via[3].via[IFR]&via[3].via[IER])||((via[4].via[IFR]&via[4].via[IER]));}

int get_exs1_pending_irq_2xpar(void) { if (via[5].via[IFR]&via[5].via[IER]) via[5].via[IFR] |=0x80;
                                       if (via[6].via[IFR]&via[6].via[IER]) via[6].via[IFR] |=0x80;
                                       return (via[5].via[IFR]&via[5].via[IER])||((via[6].via[IFR]&via[6].via[IER]));}

int get_exs2_pending_irq_2xpar(void) { if (via[7].via[IFR]&via[7].via[IER]) via[7].via[IFR] |=0x80;
                                       if (via[8].via[IFR]&via[8].via[IER]) via[8].via[IFR] |=0x80;
                                       return (via[7].via[IFR]&via[7].via[IER])||((via[8].via[IFR]&via[8].via[IER]));}


// These is here in this file because they are shared IRQs, it belongs more in the via6522.c file but there is a
// videoirq and floppy component too, so it should be in a shared place.  Might as well be static inline for speed.

int get_irq1_pending_irq(void )
{
            // this is just a fixup for the COPS VIA, doesn't affect the rest of this.
            if (!!(via[1].via[IER] & via[1].via[IFR] & 0x7f)) via[1].via[IFR] |=0x80; else  via[1].via[IFR] &=0x7f;

            // IRQ1 is the only shared IRQ - Vertical Retrace, Floppy FDIR, and VIA2 (Parallel Port) all use it.
            // fix via IFR bit 0x80's so bit 7 is properly reflecting enabled IRQ's.  Correct these bits before checking.
            if (!!(via[2].via[IER] & via[2].via[IFR] & 0x7f)) via[2].via[IFR] |=0x80; else  via[2].via[IFR] &=0x7f;

            DEBUG_LOG(0,"IRQ1: vertical:%d fdir:%d VIA2-IFR bits:%s%s%s%s%s%s%s%s returning:%d",
               (verticallatch && (videoirq & 1)), floppy_FDIR ,
               (via[2].via[IFR] &  VIA_IRQ_BIT_CA2         ? "0:CA2 ": " "   ),
               (via[2].via[IFR] &  VIA_IRQ_BIT_CA1         ? "1:CA1 ": " "   ),
               (via[2].via[IFR] &  VIA_IRQ_BIT_SR          ? "2:SR  ": " "   ),
               (via[2].via[IFR] &  VIA_IRQ_BIT_CB2         ? "3:CB2 ": " "   ),
               (via[2].via[IFR] &  VIA_IRQ_BIT_CB1         ? "4:CB1 ": " "   ),
               (via[2].via[IFR] &  VIA_IRQ_BIT_T2          ? "5:T2  ": " "   ),
               (via[2].via[IFR] &  VIA_IRQ_BIT_T1          ? "6:T1  ": " "   ),
               (via[2].via[IFR] &  VIA_IRQ_BIT_SET_CLR_ANY ? "7:ANY ": "None"),

               ((verticallatch && (videoirq & 1)) || floppy_FDIR || (via[2].via[IFR] & 0x80)));

            return ((verticallatch && (videoirq & 1)) || floppy_FDIR || (via[2].via[IFR] & 0x80)); //was vertical, not verticallatch
}


#ifdef PARANOID
#define HIGHEST(x) (highest=((x)>highest ? (x):highest))
#else
#define HIGHEST(x) (highest=(x))
#endif
// re/calculates the pending_vector_bitmap and returns the highest IRQ to be serviced.

// this is mirrored from cops.c
static inline int get_cops_pending_irq(void )
{
     // bit 7 of IFR indicates whether any VIA1 IRQ's have been fired, so check to see if any of them have, then set bit 7
     if (via[1].via[IER] & via[1].via[IFR] & 0x7f)
     {
       via[1].via[IFR] |=0x80;
       return 0x80;
     }

     via[1].via[IFR] &=0x7f;
     return 0;
}

int get_irq_mask(void) {return ((regs.sr.sr_int>>8) & 7);}

static inline uint8 get_pending_vector(void)
{
  int highest=0;

  int mask=get_irq_mask();

  //---- recalculated pending vector bitmap, and find the highest IRQ to be serviced  ------------
  // 2021.03.29 skip lower tests when mask is high, all it does is slow emulation down and then
  // it fills the log with tons of noise.
  if (get_nmi_pending_irq()  ) {highest=7; pending_vector_bitmap=BIT7; }   if (mask==7) return highest;
  if (get_scc_pending_irq()  ) {highest=6; pending_vector_bitmap=BIT6; }   if (mask==6) return highest;
  if (get_exs0_pending_irq() ) {highest=5; pending_vector_bitmap=BIT5; }   if (mask==5) return highest;
  if (get_exs1_pending_irq() ) {highest=4; pending_vector_bitmap=BIT4; }   if (mask==4) return highest;
  if (get_exs2_pending_irq() ) {highest=3; pending_vector_bitmap=BIT3; }   if (mask==3) return highest;
  if (get_cops_pending_irq() ) {highest=2; pending_vector_bitmap=BIT2; }   if (mask==2) return highest;
  if (get_irq1_pending_irq() ) {highest=1; pending_vector_bitmap=BIT1; }   

  return highest;
}

#ifdef DEBUG
static char templine[1024];
#endif

#define IS_VECTOR_AVAILABLE_INT(avno) (  ((reg68k_sr.sr_int >> 8) & 7)<(avno)  || (avno)==7)
#define IS_VECTOR_AVAILABLE_EXT(avno) (  ((regs.sr.sr_int   >> 8) & 7)<(avno)  || (avno)==7)

int is_vector_available(int avno)
{
   //if (insetjmpland) 
     return (IS_VECTOR_AVAILABLE_INT(avno));
   //return IS_VECTOR_AVAILABLE_EXT(avno);
}





static inline void fire_pending_internal_autovector(void)
{
 uint8 i=get_pending_vector();
 if (!i) return;

// DEBUG_LOG(0,"Firing pending IRQ:%d if it meets the mask",i);
 if (IS_VECTOR_AVAILABLE_INT(i))
     {
       #ifdef DEBUG
       snprintf(templine,1024,"fire_pending_int_av:Firing IRQ1 while floppy_FDIR is set. mask:%d",((reg68k_sr.sr_int>>8) & 7) );
       if (i==1 && floppy_FDIR) append_floppy_log("fire_pending_int_av:Firing IRQ1 while floppy_FDIR is set");
       #endif
       reg68k_internal_autovector(i);
     }
 #ifdef DEBUG
 else
    {
     DEBUG_LOG(0,"Could not fire IRQ:%d because it did not meet the mask.",i);
    }
 #endif

}


static inline void fire_pending_external_autovector(void)
{
  fire_pending_internal_autovector();
  return;
/*
 uint8 i=get_pending_vector();
 DEBUG_LOG(0,"Firing pending IRQ:%d if it meets the mask",i);


 if (IS_VECTOR_AVAILABLE_EXT(i))
    {
    #ifdef DEBUG
    snprintf(templine,1024,"fire_pending_ext_av:Firing IRQ1 while floppy_FDIR is set. mask:%d",((regs.sr.sr_int>>8) & 7) );
    if (i==1 && floppy_FDIR) append_floppy_log(templine);
    #endif

    reg68k_external_autovector(i);

    }
*/
}


#ifdef DEBUG


void check_mmu_pattern(uint32 x)
{
 int i,found=0;

 for (i=0; i<17; i++)
   {
    if (x==mmupattern1[i])
            {found=1; if (buglog) fprintf(buglog,"MMUPATTERN: %4x found at %2d in 1 con:%d @pc=%08x a0=%08x reg#:%d\n",mmupattern1[i],i,1+(segment1|segment2),pc24,reg68k_regs[8],reg68k_regs[8]>>17); break;}
    if (x==mmupattern2[i]) {found=2; if (buglog) fprintf(buglog,"MMUPATTERN: %4x found at %2d in 2 con:%d @pc=%08x a0=%08x reg#:%d\n",mmupattern2[i],i,1+(segment1|segment2),pc24,reg68k_regs[8],reg68k_regs[8]>>17); break;}
   }
   if ( !found && buglog) fprintf(buglog,"MMUPATTERN: %4x not found        con:%d @pc=%08x a0=%08x reg#:%d\n",x,1+(segment1|segment2),pc24,reg68k_regs[8],reg68k_regs[8]>>17);
}

void dumpmmupage(uint8 c, uint8 i, FILE *out)
{
    char s[1024];

    int16 pagestart, pageend;
    lisa_mem_t rfn, wfn;
    uint32 mymaxlisaram=maxlisaram;

    if (maxlisaram==1024*1024) mymaxlisaram+=0x80000;

    get_slr_page_range(c,i,&pagestart,&pageend,&rfn,&wfn);
    fprintf(out,"mmu[%d][%d].slr:%04x in->(%08x-%08x)  base:%08x->sor:%04x %s ch:%d pgrange:%d-%d arange: out-> %08x-%08x  r/wfn:%d:%d\n",
        c,i,
             mmu_all[c][i].slr,
             i<<17,
             (i<<17) | 0x1FFFF,

        mmu_all[c][i].sor<<9,mmu_all[c][i].sor,
        printslr(s,1024,mmu_all[c][i].slr),
        mmu_all[c][i].changed,pagestart,pageend,

        ((((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)),
        (((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511)),rfn,wfn    );

    if ((unsigned)(((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)>mymaxlisaram)
            {fprintf(out,"**** START RANGE OVER MAXLISARAM (%08x)!!!!!! chopped to 2mb it's:%08x, unchopped:%08x****\n\n",mymaxlisaram,
            (((((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)) & TWOMEGMLIM),
            (((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)    );}

    if ((unsigned)((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511)>mymaxlisaram)
            {fprintf(out,"**** END RANGE OVER MAXLISARAM (%08x)!!!!!! chopped to 2mb it's:%08x, unchopped:%08x****\n\n",mymaxlisaram,
            (((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511) & TWOMEGMLIM),
            (((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511))  );}


}

void dumpmmu(uint8 c, FILE *out)
{
    //char s[1024];
    int i,f;

    //int16 pagestart, pageend;
    //lisa_mem_t rfn, wfn;

    fprintf(out,"\nmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm dump_mmu mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm\n\n");

    for (i=0; i<128; i++)
    {
        dumpmmupage(c,i,out);
        // keep,might want to re-enable this later
        /*
        get_slr_page_range(c,i,&pagestart,&pageend,&rfn,&wfn);
        fprintf(out,"mmu[%d][%d].slr:%04x in->(%08x-%08x)  base:%08x->sor:%04x %s ch:%d pgrange:%d-%d arange: out-> %08x-%08x  r/wfn:%d:%d\n",
            c,i,
                 mmu_all[c][i].slr,
                 i<<17,
                 (i<<17) | 0x1FFFF,

            mmu_all[c][i].sor<<9,mmu_all[c][i].sor,
            printslr(s,1024,mmu_all[c][i].slr),
            mmu_all[c][i].changed,pagestart,pageend,

            ((((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)),
            (((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511)),rfn,wfn    );

        // is this +i before  + pagestart i.e i+pagestart) and i+pageend) needed/allowed here?
        if ((unsigned)(((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)>maxlisaram)
                {fprintf(out,"**** START RANGE OVER MAXLISARAM (%08x)!!!!!! chopped to 2mb it's:%08x, unchopped:%08x****\n\n",maxlisaram,
                (((((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)) & TWOMEGMLIM),
                (((mmu_all[c][i].sor & 0xfff)+pagestart)<<9)    );}

        if ((unsigned)((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511)>maxlisaram)
                {fprintf(out,"**** END RANGE OVER MAXLISARAM (%08x)!!!!!! chopped to 2mb it's:%08x, unchopped:%08x****\n\n",maxlisaram,
                (((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511) & TWOMEGMLIM),
                (((((mmu_all[c][i].sor & 0xfff)+pageend)<<9)+511))  );}
        */
    }

    fflush(out);
    fprintf(out,"\n\n");
    for ( i=0; i<32768; i++)
    {
       fprintf(out,"%08x-%08x mmu[%d] mmu_t[%d][%04x].addr=%08x rfn=%s wfn=%s ipct:%s out:%08x-%08x",
            (i*512),(i*512)+511,(i*512)>>17,
            c,i,
            mmu_trans_all[c][i].address,
            mspace(mmu_trans_all[c][i].readfn),
            mspace(mmu_trans_all[c][i].writefn),
            (mmu_trans_all[c][i].table ? "yes":"no"),
            ((i*512)+mmu_trans_all[c][i].address),((i*512)+511+mmu_trans_all[c][i].address)
            );

            // 1000000000000000
            // 5432109876543210
            if (mmu_trans_all[c][i].readfn==sio_mmu)
            {
                f=rmmuslr2fn(mmu_all[c][i>>8].slr,i);
                fprintf(out,"mmu_sio: (%d) %s",f,mspace(f));
            }
       fprintf(out,"\n");
    }
    fflush(out);
}


void dumpallmmu(void)
{
    //long i;
    FILE *out;
    static char filename[128];
    static int instance;

    if (!lisaram) return;
    if (!debug_log_enabled) return;

    instance++;
    snprintf(filename,128,"lisaem-output-mmu-%08x-%d.txt",pc24,instance);
    DEBUG_LOG(0,"debug log file: %s",filename);
    out=fopen(filename,"wt");

    #ifdef DEBUG
    printregs(out,"");
    #endif
    dumpmmu(0,out); dumpmmu(1,out); dumpmmu(2,out); dumpmmu(3,out); dumpmmu(4,out);

    fclose(out);
}


#endif



unsigned int reg68k_external_step(void)
{
    static t_ipc ipc;
    static t_iib *piib;
    static unsigned long clks;

    {
        regs.pending = get_pending_vector();
        if (regs.pending && ((reg68k_sr.sr_int >> 8) & 7) < regs.pending)
            reg68k_internal_autovector(regs.pending);

        if (!(piib = cpu68k_iibtable[fetchword(reg68k_pc)]))
            DEBUG_LOG(1,"Invalid instruction @ %08X\n", reg68k_pc); // RA

       #if DEBUG
       if (!piib) DEBUG_LOG(0,"about to pass NULL IIB");
       #endif

        cpu68k_ipc(reg68k_pc, piib,&ipc);
        if (!abort_opcode)
           cpu68k_functable[fetchword(reg68k_pc) * 2 + 1] (&ipc);
    /* restore global registers back to permanent storage */
        regs.pc = reg68k_pc; regs.sr = reg68k_sr;
    }
    cpu68k_clocks += ipc.clks;
    DEBUG_LOG(0,"this opcode:%d clks",ipc.clks);
    return clks;                  /* number of clocks done */
}

uint32 getreg(uint8 regnum)  //16=pc, 17=sp 0=7Dregs 8-15Aregs
{
    if (!insetjmpland) //*************
            {
                if (regnum==16) return regs.pc;
                if (regnum==17) return regs.sp;
                if (regnum<16 ) return regs.regs[regnum];

            }

    else   {
                if (regnum==16) return reg68k_pc;
                if (regnum==17) return regs.sp;
                if (regnum<16 ) return reg68k_regs[regnum];
           }
    return 0xdeadbeef;

}


#ifdef DEBUG
void printregs(FILE *buglog,char *tag)
{


    if (!debug_log_enabled || buglog==NULL) return;
    if (!insetjmpland) {extprintregs(buglog,tag); return;}

    fprintf(buglog,"%sD 0:%08x 1:%08x 2:%08x 3:%08x 4:%08x 5:%08x 6:%08x 7:%08x %c%c%c%c%c%c%c imsk:%d pnd:%s%s%s%s%s%s%s (%d/%d/%s cx:%d)SRC:\n",tag,
        reg68k_regs[0], reg68k_regs[1], reg68k_regs[2], reg68k_regs[3], reg68k_regs[4],
        reg68k_regs[5], reg68k_regs[6], reg68k_regs[7],
        (reg68k_sr.sr_struct.t ? 't':'.'),
        (reg68k_sr.sr_struct.s ? 'S':'.'),
        (reg68k_sr.sr_struct.z ? 'z':'.'),
        (reg68k_sr.sr_struct.x ? 'x':'.'),
        (reg68k_sr.sr_struct.n ? 'n':'.'),
        (reg68k_sr.sr_struct.v ? 'v':'.'),
        (reg68k_sr.sr_struct.c ? 'c':'.'),
        ((reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 )),

        (pending_vector_bitmap & 1) ? "1":"",
        (pending_vector_bitmap & 2) ? "2":"",
        (pending_vector_bitmap & 4) ? "3":"",
        (pending_vector_bitmap & 8) ? "4":"",
        (pending_vector_bitmap & 16) ? "5":"",
        (pending_vector_bitmap & 32) ? "6":"",
        (pending_vector_bitmap & 64) ? "7":"",

        segment1,segment2,start ? "START":"normal",context );

    fprintf(buglog,"%sA 0:%08x 1:%08x 2:%08x 3:%08x 4:%08x 5:%08x 6:%08x 7:%08x SP:%08x PC:%08x SRC:\n\n",tag,
        reg68k_regs[8], reg68k_regs[9], reg68k_regs[10],reg68k_regs[11],reg68k_regs[12],
        reg68k_regs[13],reg68k_regs[14],reg68k_regs[15],regs.sp,reg68k_pc);
    fflush(buglog);
}

/*
        reg68k_pc = regs.pc;
        reg68k_regs = regs.regs;
        reg68k_sr = regs.sr;
 */
void extprintregs(FILE *buglog,char *tag)
{
    if (!debug_log_enabled || !buglog) return;
    if (insetjmpland) {printregs(buglog,tag); return;}
    // the SRC: at the end is so I can grep the output and see both registers and source code. :)
    fprintf(buglog,"%sD 0:%08x 1:%08x 2:%08x 3:%08x 4:%08x 5:%08x 6:%08x 7:%08x %c%c%c%c%c%c%c irqmsk:%d  %d/%d/%d context:%d SRC:\n",tag,
        regs.regs[0], regs.regs[1], regs.regs[2], regs.regs[3], regs.regs[4],
        regs.regs[5], regs.regs[6], regs.regs[7],
        (regs.sr.sr_struct.t ? 't':'.'),
        (regs.sr.sr_struct.s ? 'S':'.'),
        (regs.sr.sr_struct.z ? 'z':'.'),
        (regs.sr.sr_struct.x ? 'x':'.'),
        (regs.sr.sr_struct.n ? 'n':'.'),
        (regs.sr.sr_struct.v ? 'v':'.'),
        (regs.sr.sr_struct.c ? 'c':'.'),
        ((regs.sr.sr_struct.i2 ? 4:0 )+(regs.sr.sr_struct.i1 ? 2:0 )+(regs.sr.sr_struct.i0 ? 1:0 )),
        segment1,segment2,start,context );

    fprintf(buglog,"%sA 0:%08x 1:%08x 2:%08x 3:%08x 4:%08x 5:%08x 6:%08x 7:%08x SP:%08x PC:%08x SRC:\n\n",tag,
        regs.regs[8], regs.regs[9], regs.regs[10],regs.regs[11],regs.regs[12],
        regs.regs[13],regs.regs[14],regs.regs[15],regs.sp,regs.pc);
    fflush(buglog);
}
#endif


void dumpram(char *reason)
{
  FILE *ramdump, *ramdumpM0, *ramdumpM1, *ramdumpM2, *ramdumpM3;
  char filename[256];
  uint32 i,j; //,k;
  uint16 slr, sor; 
  uint32 mad;
  snprintf(filename,256,"lisaem-output-ramdump-%s-%08lx.%016llx",reason,(long)pc24,(long long)cpu68k_clocks);
  ramdump=fopen(filename,"wt");
  if (!ramdump)
  {
    ALERT_LOG(0,"Could not dump ram because could not create file %s",filename);
    return;
  }

  fprintf(ramdump,"ramsize:%08lx, lastcx:%ld, cx:%ld seg1:%ld, seg2:%ld, start:%ld, mmudirty:%08lx,%08lx,%08lx,%08lx,%08lx\n",
          (long)maxlisaram,
          (long)lastcontext, (long)context,
          (long)segment1, (long)segment2, (long)start,
          (long)mmudirty_all[0],(long)mmudirty_all[1],(long)mmudirty_all[2],(long)mmudirty_all[3],(long)mmudirty_all[4]);

  fprintf(ramdump,"diag1:%ld, diag2:%ld, soft:%ld, hard:%ld, vert:%ld, vidlatch:%02lx, lastvidlatch:%02lx, vidlatchaddr:%08lx, lastvidlatchadr:%08lx",
          (long) diag1, (long)diag2,
          (long) softmem,
          (long) hardmem,
          (long) vertical,
          (long) videolatch, (long)lastvideolatch,  (long)videolatchaddress, (long)lastvideolatchaddress);

        //videoramdirty,
        //videoximgdirty,

  fprintf(ramdump,"regs D 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx SR:%02lx %c%c%c%c%c%c%c irqmsk:%ld  %ld/%ld/%ld context:%ld\n",
          (long)reg68k_regs[0], (long)reg68k_regs[1], (long)reg68k_regs[2], (long)reg68k_regs[3], (long)reg68k_regs[4],
          (long)reg68k_regs[5], (long)reg68k_regs[6], (long)reg68k_regs[7], (long)reg68k_sr.sr_int,
          (reg68k_sr.sr_struct.t ? 't':'.'),
          (reg68k_sr.sr_struct.s ? 'S':'.'),
          (reg68k_sr.sr_struct.z ? 'z':'.'),
          (reg68k_sr.sr_struct.x ? 'x':'.'),
          (reg68k_sr.sr_struct.n ? 'n':'.'),
          (reg68k_sr.sr_struct.v ? 'v':'.'),
          (reg68k_sr.sr_struct.c ? 'c':'.'),
          (long)((reg68k_sr.sr_struct.i2 ? 4:0 )+(long)(reg68k_sr.sr_struct.i1 ? 2:0 )+(long)(reg68k_sr.sr_struct.i0 ? 1:0 )),
          (long)segment1,(long)segment2,(long)start,(long)context );

  fprintf(ramdump,"regs A 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx SP:%08lx PC:%08lx\n\n\n",
          (long)reg68k_regs[8], (long)reg68k_regs[9], (long)reg68k_regs[10],(long)reg68k_regs[11],(long)reg68k_regs[12],
          (long)reg68k_regs[13],(long)reg68k_regs[14],(long)reg68k_regs[15],(long)regs.sp,(long)reg68k_pc);


  for (i=0; i<5; i++)
    for ( j=0; j<128; j++)
    {
      slr=mmu_all[i][j].slr;
      sor=mmu_all[i][j].sor;

      mad=((sor & 0x0fff)<<9) & TWOMEGMLIM;

    fprintf(ramdump,"mmu[%ld][%3ld].slr:%04lx,sor:%04lx  %08lx-%08lx::-->(%08lx)  type::%s\n",
          (long)i,(long)j,(long)slr,(long)sor,(long)((uint32)j<<17),(long)((uint32)j<<17)+((1<<17)-1),(long)mad,
          slrname(slr)
        );
    }
  fflush(ramdump);

  for ( i=0; i<maxlisaram; i+=16)
  {
    if (lisaram[i+0]|lisaram[i+1]|lisaram[i+2]| lisaram[i+3]| lisaram[i+4]| lisaram[i+5]| lisaram[i+6]| lisaram[i+7]|
        lisaram[i+8]|lisaram[i+9]|lisaram[i+10]|lisaram[i+11]|lisaram[i+12]|lisaram[i+13]|lisaram[i+14]|lisaram[i+15])
    fprintf(ramdump,"RAM %06lx: %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx:%02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx |%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
        (long)i,
        (long)lisaram[i+0],(long)lisaram[i+1],(long)lisaram[i+2], (long)lisaram[i+3], (long)lisaram[i+4], (long)lisaram[i+5], (long)lisaram[i+6], (long)lisaram[i+7],
        (long)lisaram[i+8],(long)lisaram[i+9],(long)lisaram[i+10],(long)lisaram[i+11],(long)lisaram[i+12],(long)lisaram[i+13],(long)lisaram[i+14],(long)lisaram[i+15],
        ((lisaram[i+0 ]>' ' && lisaram[i+0 ]<127) ? lisaram[i+0 ]:'.'),
        ((lisaram[i+1 ]>' ' && lisaram[i+1 ]<127) ? lisaram[i+1 ]:'.'),
        ((lisaram[i+2 ]>' ' && lisaram[i+2 ]<127) ? lisaram[i+2 ]:'.'),
        ((lisaram[i+3 ]>' ' && lisaram[i+3 ]<127) ? lisaram[i+3 ]:'.'),
        ((lisaram[i+4 ]>' ' && lisaram[i+4 ]<127) ? lisaram[i+4 ]:'.'),
        ((lisaram[i+5 ]>' ' && lisaram[i+5 ]<127) ? lisaram[i+5 ]:'.'),
        ((lisaram[i+6 ]>' ' && lisaram[i+6 ]<127) ? lisaram[i+6 ]:'.'),
        ((lisaram[i+7 ]>' ' && lisaram[i+7 ]<127) ? lisaram[i+7 ]:'.'),
        ((lisaram[i+8 ]>' ' && lisaram[i+8 ]<127) ? lisaram[i+8 ]:'.'),
        ((lisaram[i+9 ]>' ' && lisaram[i+9 ]<127) ? lisaram[i+9 ]:'.'),
        ((lisaram[i+10]>' ' && lisaram[i+10]<127) ? lisaram[i+10]:'.'),
        ((lisaram[i+11]>' ' && lisaram[i+11]<127) ? lisaram[i+11]:'.'),
        ((lisaram[i+12]>' ' && lisaram[i+12]<127) ? lisaram[i+12]:'.'),
        ((lisaram[i+13]>' ' && lisaram[i+13]<127) ? lisaram[i+13]:'.'),
        ((lisaram[i+14]>' ' && lisaram[i+14]<127) ? lisaram[i+14]:'.'),
        ((lisaram[i+15]>' ' && lisaram[i+15]<127) ? lisaram[i+15]:'.')  );
  }

  fflush(ramdump);
  fclose(ramdump);

  snprintf(filename,256,"lisaem-output-physical-ramdump-%s-%08lx.%016llx.%fMB.bin",reason,(long)pc24,(long long)cpu68k_clocks,(float)(maxlisaram/1024.0));
  ramdump=fopen(filename,"wb");
  if (!ramdump) return;
  fwrite(lisaram,maxlisaram,1,ramdump);
  fflush(ramdump);
  fclose(ramdump);


  snprintf(filename,256,"lisaem-output-mmu0-ramdump-%s-%08lx.%016llx.%fMB.bin",reason,(long)pc24,(long long)cpu68k_clocks,(float)(maxlisaram/1024.0));
  ramdumpM0=fopen(filename,"wb"); if (!ramdumpM0) {fclose(ramdump); return;}

  snprintf(filename,256,"lisaem-output-mmu1-ramdump-%s-%08lx.%016llx.%fMB.bin",reason,(long)pc24,(long long)cpu68k_clocks,(float)(maxlisaram/1024.0));

  ramdumpM1=fopen(filename,"wb"); if (!ramdumpM1) {fclose(ramdump); fclose(ramdumpM0); return;}
  snprintf(filename,256,"lisaem-output-mmu2-ramdump-%s-%08lx.%016llx.%fMB.bin",reason,(long)pc24,(long long)cpu68k_clocks,(float)(maxlisaram/1024.0));

  ramdumpM2=fopen(filename,"wb"); if (!ramdumpM2) {fclose(ramdump); fclose(ramdumpM0); fclose(ramdumpM1); return;}

  snprintf(filename,256,"lisaem-output-mmu3-ramdump-%s-%08lx.%016llx.%fMB.bin",reason,(long)pc24,(long long)cpu68k_clocks,(float)(maxlisaram/1024.0));
  ramdumpM3=fopen(filename,"wb");if (!ramdumpM3) {fclose(ramdump); fclose(ramdumpM0); fclose(ramdumpM3); return;}

  fprintf(ramdump,"context:%ld segment1:%ld, segment2:%ld, start:%ld pc24:%08lx\n\n",(long)context,(long)segment1,(long)segment2,(long)start,(long)pc24);

  // each context=16M, so this will be slow - could make this go faster by using custom MMU translation code and doing one segment at a time, but, meh,
  // this is only used for debugging, so, why bother.
  for (i=0; i<0x01000000; i+=16*4)
  {
      int z; 
      uint32 buf[16];
      buf[ 0]=lisa_ram_safe_getlong(0,(i+ 0)); buf[ 1]=lisa_ram_safe_getlong(0,(i+ 1)); buf[ 2]=lisa_ram_safe_getlong(0,(i+ 2)); buf[ 3]=lisa_ram_safe_getlong(0,(i+ 3));
      buf[ 4]=lisa_ram_safe_getlong(0,(i+ 4)); buf[ 5]=lisa_ram_safe_getlong(0,(i+ 5)); buf[ 6]=lisa_ram_safe_getlong(0,(i+ 6)); buf[ 7]=lisa_ram_safe_getlong(0,(i+ 7));
      buf[ 8]=lisa_ram_safe_getlong(0,(i+ 8)); buf[ 9]=lisa_ram_safe_getlong(0,(i+ 9)); buf[10]=lisa_ram_safe_getlong(0,(i+10)); buf[11]=lisa_ram_safe_getlong(0,(i+11));
      buf[12]=lisa_ram_safe_getlong(0,(i+12)); buf[13]=lisa_ram_safe_getlong(0,(i+13)); buf[14]=lisa_ram_safe_getlong(0,(i+14)); buf[15]=lisa_ram_safe_getlong(0,(i+15));
      z=fwrite(buf,16*4,1,ramdumpM0);

      buf[ 0]=lisa_ram_safe_getlong(1,(i+ 0)); buf[ 1]=lisa_ram_safe_getlong(1,(i+ 1)); buf[ 2]=lisa_ram_safe_getlong(1,(i+ 2)); buf[ 3]=lisa_ram_safe_getlong(1,(i+ 3));
      buf[ 4]=lisa_ram_safe_getlong(1,(i+ 4)); buf[ 5]=lisa_ram_safe_getlong(1,(i+ 5)); buf[ 6]=lisa_ram_safe_getlong(1,(i+ 6)); buf[ 7]=lisa_ram_safe_getlong(1,(i+ 7));
      buf[ 8]=lisa_ram_safe_getlong(1,(i+ 8)); buf[ 9]=lisa_ram_safe_getlong(1,(i+ 9)); buf[10]=lisa_ram_safe_getlong(1,(i+10)); buf[11]=lisa_ram_safe_getlong(1,(i+11));
      buf[12]=lisa_ram_safe_getlong(1,(i+12)); buf[13]=lisa_ram_safe_getlong(1,(i+13)); buf[14]=lisa_ram_safe_getlong(1,(i+14)); buf[15]=lisa_ram_safe_getlong(1,(i+15));
      z=fwrite(buf,16*4,1,ramdumpM1);

      buf[ 0]=lisa_ram_safe_getlong(2,(i+ 0)); buf[ 1]=lisa_ram_safe_getlong(2,(i+ 1)); buf[ 2]=lisa_ram_safe_getlong(2,(i+ 2)); buf[ 3]=lisa_ram_safe_getlong(2,(i+ 3));
      buf[ 4]=lisa_ram_safe_getlong(2,(i+ 4)); buf[ 5]=lisa_ram_safe_getlong(2,(i+ 5)); buf[ 6]=lisa_ram_safe_getlong(2,(i+ 6)); buf[ 7]=lisa_ram_safe_getlong(2,(i+ 7));
      buf[ 8]=lisa_ram_safe_getlong(2,(i+ 8)); buf[ 9]=lisa_ram_safe_getlong(2,(i+ 9)); buf[10]=lisa_ram_safe_getlong(2,(i+10)); buf[11]=lisa_ram_safe_getlong(2,(i+11));
      buf[12]=lisa_ram_safe_getlong(2,(i+12)); buf[13]=lisa_ram_safe_getlong(2,(i+13)); buf[14]=lisa_ram_safe_getlong(2,(i+14)); buf[15]=lisa_ram_safe_getlong(2,(i+15));
      z=fwrite(buf,16*4,1,ramdumpM2);

      buf[ 0]=lisa_ram_safe_getlong(3,(i+ 0)); buf[ 1]=lisa_ram_safe_getlong(3,(i+ 1)); buf[ 2]=lisa_ram_safe_getlong(3,(i+ 2)); buf[ 3]=lisa_ram_safe_getlong(3,(i+ 3));
      buf[ 4]=lisa_ram_safe_getlong(3,(i+ 4)); buf[ 5]=lisa_ram_safe_getlong(3,(i+ 5)); buf[ 6]=lisa_ram_safe_getlong(3,(i+ 6)); buf[ 7]=lisa_ram_safe_getlong(3,(i+ 7));
      buf[ 8]=lisa_ram_safe_getlong(3,(i+ 8)); buf[ 9]=lisa_ram_safe_getlong(3,(i+ 9)); buf[10]=lisa_ram_safe_getlong(3,(i+10)); buf[11]=lisa_ram_safe_getlong(3,(i+11));
      buf[12]=lisa_ram_safe_getlong(3,(i+12)); buf[13]=lisa_ram_safe_getlong(3,(i+13)); buf[14]=lisa_ram_safe_getlong(3,(i+14)); buf[15]=lisa_ram_safe_getlong(3,(i+15));
      z=fwrite(buf,16*4,1,ramdumpM3);
  }

  fflush(ramdumpM0); fclose(ramdumpM0);
  fflush(ramdumpM1); fclose(ramdumpM1);
  fflush(ramdumpM2); fclose(ramdumpM2);
  fflush(ramdumpM3); fclose(ramdumpM3);

}



extern void xxxcheckcontext(uint8 c, char *text);

void xdumpram(void) {dumpram("atexit");}


#ifdef DEBUG

extern char *gettimername(uint8 t);
#ifdef LOOKAHEAD
        static int been_here_before=0;
#endif

#ifdef PROCNAME_DEBUG
int is_valid_procname(uint8 c)
{
    c &=0x7f;
    if (c>='A' && c<='Z') return 1;
    if (c=='_' || c==' ') return 1;
    if (c>='0' && c<='9') return 1;
    return 0;
}

int is_valid_procname_w(uint16 w){ return (is_valid_procname(w>>8) && is_valid_procname(w & 0x7f) ); }
#endif

#endif


int get_address_mmu_rfn_type(uint32 addr)
{
  addr=addr & ADDRESSFILT;
  return mmu_trans[(addr>>9) & 32767].readfn;
}

extern long get_wx_millis(void);
extern char *debug_screenshot(void);

// debug inline functions for reg68k_external_execute - pulled out because this function was getting too long
#ifdef DEBUG

#ifdef DISASM_SKIPPED_OPCODES
inline void debug_reg68k_exec_disasm_skipped(void)
{
        if (debug_log_enabled && buglog && pc24>lastpc24 && lastpc24>0)
          if ( (pc24-lastpc24)<128)
          {
            uint32 cursor=lastpc24;
            char text[1024];
            static t_ipc myipc;
            static t_iib *mypiib;

            DEBUG_LOG(0,"disassembling skipped opcodes between %08lx-%08lx",(long)cursor,(long)pc24);

            while (cursor<pc24)
            {
              abort_opcode=2;
              if (!(mypiib = cpu68k_iibtable[fetchword(cursor)]))
                  {DEBUG_LOG(1,"Invalid instruction @ %08lX\n", (long)cursor); cursor=pc24;
                    break;
                  }
              if (abort_opcode==1) cursor=pc24;
              abort_opcode=2;
              if (!mypiib) DEBUG_LOG(0,"About to send null IPC!");
              cpu68k_ipc(cursor, mypiib,&myipc); if (abort_opcode==1) cursor=pc24;

              abort_opcode=2;  diss68k_gettext(&myipc, text); if (abort_opcode==1) cursor=pc24;

              fprintf(buglog,"\nx:%08x (skipped) opcode=%04x %s             +%d clks   SRC:\n",cursor,myipc.opcode,text,myipc.clks);

               cursor +=myipc.wordlen*2;        if (!myipc.wordlen) {EXIT(14,0,"*** Doh! ipc.wordlen=0 **");}
            }
            if (abort_opcode==1) DEBUG_LOG(0,"**DANGER*** GOT abort_opcode!******\n");
            debug_log_enabled=1;
          }
}         
#endif

#ifdef LOOKAHEAD
inline void debug_reg68k_exec_lookahead(void) {
    if (pc24==LOOKSTARTADDR && been_here_before==0)
      {
        uint32 cursor=pc24;
        char text[1024];
        static t_ipc myipc;
        static t_iib *mypiib;

        been_here_before=1;
        debug_off();
        debug_on("lookahead");

        DEBUG_LOG(0,"lookahead disassembling skipped opcodes between %08x-%08x",cursor,pc24);

        pc24 = reg68k_pc; 

        while (cursor<LOOKENDADDR)
          {
            abort_opcode=2;
            if (!(mypiib = cpu68k_iibtable[fetchword(cursor)]))
                { DEBUG_LOG(1,"Invalid instruction @ %08lX\n", (long)cursor); cursor=pc24;
                  break;
                }
            if (abort_opcode==1) cursor=LOOKENDADDR;
            abort_opcode=2;
            if (!mypiib) DEBUG_LOG(0,"About to send null IPC!");
            cpu68k_ipc(cursor, mypiib,&myipc); if (abort_opcode==1) cursor=LOOKENDADDR;

            abort_opcode=2;  diss68k_gettext(&myipc, text); if (abort_opcode==1) cursor=LOOKENDADDR;

            if (mypiib->clocks!=myipc.clks) DEBUG_LOG(0,"ERROR:iib clocks:%ld != ipc clocks:%ld !", mypiib->clocks , myipc.clks);
            fprintf(buglog,"\nL:%08lx  opcode=%04lx   %-70s  +%ld clks",(long)cursor,(long)myipc.opcode,text,(long)myipc.clks);

             cursor +=myipc.wordlen*2;        if (!myipc.wordlen) {EXIT(14,0,"*** Doh! ipc.wordlen=0 **");}
          }

        if (abort_opcode==1) DEBUG_LOG(0,"**DANGER*** GOT abort_opcode!******\n");
        debug_log_enabled=0; debug_off();
      }
}
#endif


#ifdef SUPPRESS_LOOP_DISASM
inline void reg68k_exec_suppress_loop_disasm(void) 
{

      if  (!suppress_printregs)
          { if (debug_log_enabled)  {printregs(buglog,"");} }
      else
          {
            int32 i;  
            int32 j=(suppress_printregs & 32767); 
            int flag=0;
            char line[1024], buf[1024];

            snprintf(line,1024,"loop:: %08x: ",pc24);

             // might want to change j to be the previous instruction - but doesn't matter very much I suppose
            for (i=0; i<8;  i++)  
                if (last_regs[j].regs[i]!=reg68k_regs[i]) {snprintf(buf,1024,"D%ld:%08lx ",(long)i,(long)reg68k_regs[i]);   strncat(line,buf,1024);flag=1;}
            for (i=8; i<16; i++)  
                if (last_regs[j].regs[i]!=reg68k_regs[i]) {snprintf(buf,1024,"A%ld:%08lx ",(long)i-8,(long)reg68k_regs[i]); strncat(line,buf,1024);flag=1;}

            if (last_regs[j].sr.sr_int!=reg68k_sr.sr_int)
                {   flag=1;
                    snprintf(buf,1024,"%c%c%c%c%c%c%c imsk:%ld ",
                    (reg68k_sr.sr_struct.t ? 't':'.'),
                    (reg68k_sr.sr_struct.s ? 'S':'.'),
                    (reg68k_sr.sr_struct.z ? 'z':'.'),
                    (reg68k_sr.sr_struct.x ? 'x':'.'),
                    (reg68k_sr.sr_struct.n ? 'n':'.'),
                    (reg68k_sr.sr_struct.v ? 'v':'.'),
                    (reg68k_sr.sr_struct.c ? 'c':'.'),
                    (long)(  (reg68k_sr.sr_struct.i2 ? 4:0 )+(reg68k_sr.sr_struct.i1 ? 2:0 )+(reg68k_sr.sr_struct.i0 ? 1:0 ) )   );
                    strncat(line,buf,1024);
                }
              if (flag && buglog) fprintf(buglog,"%s\n",line);
            }
}
#endif

#ifdef ICACHE_DEBUG
inline void reg68k_exec_icache_debug(void)
{    
    if (dbx && pc24>16)
    {
        int i;  uint8 c,nice[1024], hex[1024], tmp[1024];
        nice[0]=0; hex[0]=0;
        for (i=0; i<32; i++) {
            abort_opcode=2;
            c=lisa_ram_safe_getbyte(context,(pc24-16+i));

            if (i & 1) snprintf(tmp,1024,"%02lx ",(long)c);
            else       snprintf(tmp,1024,"%02lx", (long)c);

            strncat(hex,tmp,1024);
            nice[i]=niceascii(c);
        }

        debug_log_enabled|=dbx;     // re-enable debug log only after icache fetches to prevent verbosity

        if (buglog) fprintf(buglog,"%08lx:%s|%s clk:%016llx\n",(long)pc24-16,hex,nice,(long long)cpu68k_clocks);
        //DEBUG_LOG(0,"icache:%04x:%04x:%04x:%04x:%04x  clk:%08x",ipc->opcode,icache2,icache4, icache6, icache8,cpu68k_clocks);
        abort_opcode=0;
    }
    else debug_log_enabled=dbx;
} ///**** moved this insdie meh ****
#endif


#ifdef SKIP_BIGWAIT_NMI_CODE
inline void reg68k_exec_skip_bigwait_nmi_code(void) 
{
  if  (lisa_os_boot_mouse_x_ptr==0xfec)
      {/////////////////////////////////////////////////////////////////////////////////////////

        if ( ipc->opcode==0x5381)
          {
           // hack to speed up lisatest?
            if (pc24==0xd36e && reg68k_regs[1]==0x3d090) {reg68k_regs[1]=1; DEBUG_LOG(0,"SKIP_BIGWAIT_NMI_CODE D1=1 @d36e");}
            if (pc24==0xdd52 && reg68k_regs[1]>1 )       {reg68k_regs[1]=1; DEBUG_LOG(0,"SKIP_BIGWAIT_NMI_CODE D1=1 @dd52");}
          }
        else                                    // 2005.04.05 06:22am - Turn Debugging on LisaTest CPU ErrorLevel
          {
              if  (pc24==0x0000d2c8 && ipc->opcode==0xb3cb && reg68k_regs[6]==0x00000018 && reg68k_regs[8]<0x001ff000)
                  {
                    debug_on("LisaTest-cpu-errorlogic"); debug_log_enabled=1; debug_log_enabled=1; last_dbe=1;
                    reg68k_regs[8]=0x001fff00; reg68k_regs[9]=0x001fff00;
                    memerror=0xFFF8;
                    DEBUG_LOG(0,"LisaTest CPU Error Logic - enabling debug 2005.04.05");
                  }
          }

      }///////////////////////////////////////////////////////////////////////////////////////////
}
#endif

//inline 
void reg68k_exec_debug_block(int32 clocks, mmu_trans_t *mt, int k, t_ipc *ipc,char *text) {

  // keep this can use for debuggery later
  #ifdef xxxHALT_AT
  if ( (pc24==0xfe06f2 || pc24==0xfe14F2 || pc24==0xfe144a pc==0xfe) && mt->readfn==sio_rom) {
        EXIT(398,0,"compiled in halt.");
  }
  #endif

  // if the tracelog is not enabled, skip this
  if  (debug_log_enabled)
      {
         // are we using the H ROM source, if so output it
          if (mt->readfn==sio_mmu && rom_source_file && dtc_rom_fseeks && ((pc24 & 0xffff)<0x3fff))
          {   char *s; char buff[1024];
              fseek(rom_source_file,dtc_rom_fseeks[pc24 & 0x3fff],SEEK_SET);
              s=fgets(buff,1024,rom_source_file); 
              if (strlen(buff)>5 && buglog && s!=NULL) fprintf(buglog,"%08lx: SRC: %s\n",(long)pc24,buff);
          }

          // this suppresses loop disassembly instead just showing what changed between opcodes     
          #ifdef SUPPRESS_LOOP_DISASM
          {
                suppress_printregs=0;

                j=last_regs_idx;
                do   
                  {     j--; if (j<0) j=MAX_LOOP_REGS-1;
                        if (pc24==last_regs[j].pc) k=1;
                  } 
                  while (j!=last_regs_idx && !k);

                  // copy to current register
                  j=last_regs_idx+1;  if (j>=MAX_LOOP_REGS) j=0;

                  last_regs[j].pc=pc24;
                  last_regs[j].sp=regs.sp;
                  last_regs[j].sr.sr_int=reg68k_sr.sr_int;
                  for (i=0; i<16; i++)  last_regs[j].regs[i]=reg68k_regs[i];
                  last_regs_idx=j;


                  if ((ipc->opcode & 0xf000)==0xa000)
                      {
                        char temp[256];
                        snprintf(temp,255,"::%08lx ",(long)fetchlong(pc24+2));
                        strncat(text,temp,16383);
                      }
          }
          #endif  //--- SUPRESS_LOOP_DISASM



          #ifdef PROCNAME_DEBUG
          {  uint16 w,w1,w2,w3;

              w=lisa_ram_safe_getword(context,(pc24-2));
              //DEBUG_LOG(0,"entering procedure: got 5th word:%04x",w);
              if (w<0x00f0)
              {
                w3=lisa_ram_safe_getword(context,(pc24-10));
                if  ((w3 & 0x8000) && is_valid_procname_w(w3 & 0x7f7f) )
                    {
                      w2=lisa_ram_safe_getword(context,(pc24-8));
                      w1=lisa_ram_safe_getword(context,(pc24-6));
                      w =lisa_ram_safe_getword(context,(pc24-4));
                      if  (is_valid_procname_w(w) && is_valid_procname_w(w1) && is_valid_procname_w(w2)  )
                          {
                          if (buglog) 
                              fprintf(buglog,"\n\n====== procedure: %c%c%c%c%c%c%c%c =====\n\n",
                                      ((w3>>8) & 0x7f),(w3 & 0x7f),
                                      ((w2>>8) & 0x7f),(w2 & 0x7f),
                                      ((w1>>8) & 0x7f),(w1 & 0x7f),
                                      ((w >>8) & 0x7f),(w  & 0x7f)     );
                        /*
                        // 20180326
                            if  (         ((w3>>8) & 0x7f)=='E' && (w3 & 0x7f)=='A' &&
                                          ((w2>>8) & 0x7f)=='C' && (w2 & 0x7f)=='H' &&
                                          ((w1>>8) & 0x7f)=='M' && (w1 & 0x7f)=='E' &&
                                          ((w >>8) & 0x7f)=='M' && (w  & 0x7f)=='B'    ) debug_on("EACHMEMB");

                            if  (         ((w3>>8) & 0x7f)=='P' && (w3 & 0x7f)=='R' &&
                                          ((w2>>8) & 0x7f)=='T' && (w2 & 0x7f)=='E' &&
                                          ((w1>>8) & 0x7f)=='N' && (w1 & 0x7f)=='T' &&
                                          ((w >>8) & 0x7f)=='N' && (w  & 0x7f)=='A'    ) debug_off(); // PRTENTNA
                         */
                          }
                    }
                }
           } // PROCNAME DEBUG b
          #endif       // end of PROCNAME_DEBUG


          if (!k)
            {   char dumpline[1024];
                if (ipc->opcode!=0xe350) abort_opcode=2;  
                diss68k_getdumpline(pc24,dumpline);
                diss68k_gettext(ipc, text);
                 //fprintf(buglog,"%d/%08x (cx %d %d/%d/%d) opcode=%04x %s    SRC:clk:%016lx +%ld clks\n%s",context,pc24,
                 //       (segment1|segment2),segment1,segment2,start,ipc->opcode,text,cpu68k_clocks, ipc->clks,dumpline);
                if (buglog)
                    fprintf(buglog,"%ld/%08lx (%ld %ld/%ld/%ld) %s  SRC:clk:%016llx +%ld clks\n",(long)context,(long)pc24,
                        (long)(segment1|segment2),(long)segment1,(long)segment2,(long)start,dumpline,(long long)cpu68k_clocks, (long)ipc->clks);

            }
            #ifdef SUPPRESS_LOOP_DISASM
            else suppress_printregs=32768|j;
            #endif

      } // if debug_log_enabled at top of reg68k_exec_debug_block

}
//} // end of reg68k_exec_debug_block fn


#ifdef DEBUG
void reg68k_ext_exec_various_dbug(void) 
{
/*
          if ((pc24 & 0x00ff0000)!=0x00fe0000 && (reg68k_pc & 0x00ff0000)==0x00fe0000)
              ALERT_LOG(0,"Entering ROM from operating system at %08x from %08x %s\n"
                          "D 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx\n"
                          "A 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx\n",
                          (long)reg68k_pc,(long)pc24,(long)get_rom_label(reg68k_pc),
                          (long)reg68k_regs[0], (long)reg68k_regs[1], (long)reg68k_regs[2], (long)reg68k_regs[3], (long)reg68k_regs[4], (long)reg68k_regs[5], (long)reg68k_regs[6], (long)reg68k_regs[7],
                          (long)reg68k_regs[8], (long)reg68k_regs[9], (long)reg68k_regs[10],(long)reg68k_regs[11],(long)reg68k_regs[12],(long)reg68k_regs[13],(long)reg68k_regs[14],(long)reg68k_regs[15]
                        );
*/

 switch (reg68k_pc) {
   case 0x00fe0090:
              {
                  ALERT_LOG(0,"ENT Entering ROM profile read.  ROMLESS")    
                  int i;
                  uint8 b;
                
                  b=fetchbyte(0x1b3);
                
                  ALERT_LOG(0,"ENT Booting up reg68k_pc:%08lx from pc24:%08lx %s. reg68k_sr.sr_int=%04lx regs.sp=%08lx 1b3:%02lx   ROMLESS\n"
                              "ENT D 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx ROMLESS\n"
                              "ENT A 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx ROMLESS\n",
                              (long) reg68k_pc,(long)pc24,get_rom_label(reg68k_pc),(long)reg68k_sr.sr_int,(long)regs.sp,(long)b,
                              (long) reg68k_regs[0], (long)reg68k_regs[1], (long)reg68k_regs[2], (long)reg68k_regs[3], (long)reg68k_regs[4], (long)reg68k_regs[5], (long)reg68k_regs[6], (long)reg68k_regs[7],
                              (long) reg68k_regs[8], (long)reg68k_regs[9], (long)reg68k_regs[10],(long)reg68k_regs[11],(long)reg68k_regs[12],(long)reg68k_regs[13],(long)reg68k_regs[14],(long)reg68k_regs[15]
                          );
                
                  return;
                }


// case  0x00fe1fee:
//           {
//
//               ALERT_LOG(0,"RET Return from ROM profile read.  ROMLESS")    
//               int i;
//               uint8 b;
//                    
//               b=fetchbyte(0x1b3);
//                   
//               ALERT_LOG(0,"RET Booting up reg68k_pc:%08lx from pc24:%08lx %s. reg68k_sr.sr_int=%04lx regs.sp=%08lx 1b3:%02lx ROMLESS\n"
//                           "RET D 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx ROMLESS\n"
//                           "RET A 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx ROMLESS\n",
//                           (long)reg68k_pc,pc24,get_rom_label(reg68k_pc),(long)reg68k_sr.sr_int,regs.sp,(long)b,
//                           (long)reg68k_regs[0], (long)reg68k_regs[1], (long)reg68k_regs[2], (long)reg68k_regs[3], (long)reg68k_regs[4], (long)reg68k_regs[5], (long)reg68k_regs[6], (long)reg68k_regs[7],
//                           (long)reg68k_regs[8], (long)reg68k_regs[9], (long)reg68k_regs[10],(long)reg68k_regs[11],(long)reg68k_regs[12],(long)reg68k_regs[13],(long)reg68k_regs[14],(long)reg68k_regs[15]
//                        );
//                   
//                  }
   

   case 0x00fe1d3e: {ALERT_LOG(0,"Return to ROM - Error 75"); debug_off();  return; }
   case 0x00fe0084: {ALERT_LOG(0,"Return to ROM");            debug_off();  return; }
   case 0x00fe1fde: {ALERT_LOG(0,"Return to ROM - PROERR");   debug_off();  return; }
   case 0x00fe1f3a: {ALERT_LOG(0,"Return to ROM - BOOTERR");  debug_off();  return; }
   case 0x00020000:
              {
                  int i;
                  uint8 b;
                  b=fetchbyte(0x1b3);
                  ALERT_LOG(0,"INT Booting up reg68k_pc:%08lx from pc24:%08lx %s. reg68k_sr.sr_int=%04lx regs.sp=%08lx 1b3:%02lx  ROMLESS\n"
                              "INT D 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx  ROMLESS\n"
                              "INT A 0:%08lx 1:%08lx 2:%08lx 3:%08lx 4:%08lx 5:%08lx 6:%08lx 7:%08lx  ROMLESS\n",
                              (long)reg68k_pc,(long)pc24,get_rom_label(reg68k_pc),(long)reg68k_sr.sr_int,(long)regs.sp,(long)b,
                              (long)reg68k_regs[0], (long)reg68k_regs[1], (long)reg68k_regs[2], (long)reg68k_regs[3], (long)reg68k_regs[4], (long)reg68k_regs[5], (long)reg68k_regs[6], (long)reg68k_regs[7],
                              (long)reg68k_regs[8], (long)reg68k_regs[9], (long)reg68k_regs[10],(long)reg68k_regs[11],(long)reg68k_regs[12],(long)reg68k_regs[13],(long)reg68k_regs[14],(long)reg68k_regs[15]
                            );
                  return;
              }            
   }
}
#endif  /// end of ifdef XXXDEBUG disabled code

// move these to cpu-record-master.c and use #include ../../../experimental/cpu-record-master.c
#ifdef CPU_CORE_TESTER
#include "../../../../experimental/cpu-record-master.c"

void print_ipc(uint32 pc)
{
    t_ipc *ipc;
    t_iib *piib;
    mmu_trans_t *mt;

    uint32 page;
    page=(pc & ADDRESSFILT)>>9;     
    mt=&mmu_trans[page];
    ipc=&(mt->table->ipc[(pc & 0x1ff)>>1]);
    piib = cpu68k_iibtable[ipc->opcode];

    fprintf(stdout,"ipc-opcode:%04x used/set:%02x/%02x wordlen:%d src:dst: %08x %08x s/dreg:%04x/%04x\n",
                    ipc->opcode,ipc->set,ipc->used,ipc->wordlen, ipc->src,ipc->dst, ipc->sreg, ipc->dreg);

    fprintf(stdout,"iib: mask/bits:%04x/%04x len:%d size:%d s/dtype:%d/%d s/dbitpos:%d immvalue:%08x/%d cc:%x\niib: priv:%d endblk:%d notzero:%d used:%d set:%d, fn:%d",
            piib->mask,piib->bits,piib->wordlen,piib->size,piib->stype,piib->dtype,
            piib->sbitpos, piib->dbitpos, piib->immvalue,
            piib->cc,
            piib->flags.priv,piib->flags.endblk,piib->flags.imm_notzero,piib->flags.used,piib->flags.set,
            piib->funcnum);

}

#endif


#endif // end of if DEBUG for reg68k_external_execute debug inlines //////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------------------------------

#ifdef DEBUG
extern void   check_ipct_memory(void);

#ifdef CPU_CORE_TESTER_PATTERN_TEST
void drive_pattern_test(void);
#endif

#endif

static uint32 last_display_str=0xc0def3f3;

void display_stack_vals(void)
{
   uint32 straddrp = reg68k_regs[8+7];

   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+ 0, lisa_ram_safe_getlong(context,straddrp+ 0));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+ 4, lisa_ram_safe_getlong(context,straddrp+ 4));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+ 8, lisa_ram_safe_getlong(context,straddrp+ 8));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+12, lisa_ram_safe_getlong(context,straddrp+12));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+16, lisa_ram_safe_getlong(context,straddrp+16));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+20, lisa_ram_safe_getlong(context,straddrp+20));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+24, lisa_ram_safe_getlong(context,straddrp+24));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+28, lisa_ram_safe_getlong(context,straddrp+28));
   fprintf(buglog,"stackval[@%08x]=%08x\n",straddrp+32, lisa_ram_safe_getlong(context,straddrp+32));
}


void trap_opcode(uint32 n) {
#ifdef DEBUG
    if (!debug_log_enabled) return;

    switch(n)
     {
       case 7: return; // trampoline
       case 5:
       {
        char *label=NULL;
        switch(reg68k_regs[7] & 0x0000ffff)
        {
        case 0x00:     /* 0028  */ label="driverin"; break;
        case 0x68:     /* 0032  */ label="diskdriv"; break;
        case 0x6a:     /* 0046  */ label="twiggydr"; break;
        case 0x6c:     /* 005a  */ label="disksync"; break;
        case 0x8c:     /* 006a  */ label="nmisync "; break;
        case 0xaa:     /* 0076  */ label="copsync "; break;
        case 0x9e:     /* 0082  */ label="poll    "; break;
        case 0x02:     /* 008e  */ label="mouseloc"; break;
        case 0x04:     /* a4    */ label="mouseupd"; break;
        case 0x06:     /* b4    */ label="mousesca"; break;
        case 0x08:     /* c4    */ label="mousethr"; break;
        case 0xa8:     /* d4    */ label="nouseodo"; break;
        case 0x0a:     /* e4    */ label="cursorlo"; break;
        case 0x0c:     /* f8    */ label="cursortr"; break;
        case 0x0e:     /* 108   */ label="cursorim"; break;
        case 0x10:     /* 158   */ label="cursorhi"; break;
        case 0x12:     /* 164   */ label="cursorsh"; break;
        case 0x14:     /* 188   */ label="cursordi"; break;
        case 0x6e:     /* 194   */ label="cursorob"; break;
        case 0x70:     /* 1a0   */ label="cursorin"; break;
        case 0x9c:     /* 1ac   */ label="cursorre"; break;
        case 0x86:     /* 1b8   */ label="busyimag"; break;
        case 0x88:     /* 208   */ label="busydela"; break;
        case 0x16:     /* 218   */ label="framecou"; break;
        case 0x8a:     /* 228   */ label="screensi"; break;
        case 0x18:     /* 23e   */ label="screenad"; break;
        case 0x1a:     /* 24e   */ label="altscree"; break;
        case 0x8e:     /* 25e   */ label="screenke"; break;
        case 0x90:     /* 26e   */ label="setscree"; break;
        case 0x1c:     /* 27e   */ label="contrast"; break;
        case 0x1e:     /* 28e   */ label="setcontr"; break;
        case 0x20:     /* 29e   */ label="rampcont"; break;
        case 0x82:     /* 2ae   */ label="dimcontr"; break;
        case 0x84:     /* 2be   */ label="setdimco"; break;
        case 0x22:     /* 2ce   */ label="fadedela"; break;
        case 0x24:     /* 2de   */ label="setfaded"; break;
        case 0x26:     /* 2ee   */ label="powerdow"; break;
        case 0x28:     /* 2fa   */ label="powercyc"; break;
        case 0x2a:     /* 30a   */ label="volume  "; break;
        case 0x2c:     /* 31a   */ label="setvolum"; break;
        case 0x2e:     /* 32a   */ label="noise   "; break;
        case 0x30:     /* 33a   */ label="silece  "; break;
        case 0x32:     /* 346   */ label="beep    "; break;
        case 0x38:     /* 358   */ label="keyboard"; break;
        case 0x96:     /* 368   */ label="legends "; break;
        case 0x98:     /* 378   */ label="setlegen"; break;
        case 0x34:     /* 388   */ label="keyisdow"; break;
        case 0x36:     /* 39c   */ label="keymap  "; break;
        case 0x3c:     /* 3b8   */ label="keybdpee"; break;
        case 0x92:     /* 3d0   */ label="altkeype"; break;
        case 0x3a:     /* 3fa   */ label="keybdeve"; break;
        case 0x94:     /* 412   */ label="altkeyev"; break;
        case 0x3e:     /* 43c   */ label="repeatra"; break;
        case 0x40:     /* 452   */ label="setrepea"; break;
        case 0x42:     /* 464   */ label="keypushe"; break;
        case 0x72:     /* 476   */ label="nmikey  "; break;
        case 0x74:     /* 486   */ label="setnmike"; break;
        case 0xa0:     /* 498   */ label="toggleke"; break;
        case 0xa2:     /* 4ab   */ label="settoggl"; break;
        case 0x9a:     /* 4ba   */ label="microtim"; break;
        case 0x44:     /* 4ca   */ label="timer   "; break;
        case 0x46:     /* 4da   */ label="alarmass"; break;
        case 0x48:     /* 4f2   */ label="alarmret"; break;
        case 0x4a:     /* 502   */ label="alarmabs"; break;
        case 0x4c:     /* 514   */ label="alarmrel"; break;
        case 0x4e:     /* 526   */ label="alarmoff"; break;
        case 0x50:     /* 536   */ label="datetime"; break;
        case 0x52:     /* 558   */ label="setdatet"; break;
        case 0x54:     /* 57a   */ label="datetoti"; break;
        case 0x58:     /* 5a4   */ label="timestam"; break;
        case 0x5a:     /* 5b4   */ label="settimes"; break;
        case 0x5c:     /* 5c4   */ label="timetoda"; break;
        default: fprintf(buglog,"Unknown trap #5 D7=%08x\n",reg68k_regs[7]);
        }
        return;
     }
     default:
       fprintf(buglog,"Unknown TRAP #%d\n",n);
       display_stack_vals();
     }
#endif
}


extern void  lpw_console_output(char *text);

extern void init_terminal_serial_port(int port); // in src/host/wxui/z8530-terminal.cpp

void a_line(void) {  // this is invoked from cpu68k-a.c before the vector is taken
                     // reg68k_pc is the address of the the trap opcode where it was invoked, 
                     // not the vector address as this is before vector processing.

    if (running_lisa_os!=LISA_OFFICE_RUNNING) return;

    abort_opcode=0;
    uint32 alineopcode=(InstructionRegister<<16) | fetchword(reg68k_pc + 2);
    if (abort_opcode) {abort_opcode=0;return;}

    switch (alineopcode)
    {
      // maybe 3/a0c60100 or possibly: 3/a02201c8 - this is really a memcpy equivalent, but it is used for displaying in console in quickdraw I think
      // idea check stack frame for call from 3/a0c02584 as a filter for this that's cleaner and won't need patching, except that context switch will
      // also change A7.

      case 0xa022024c: //called from CKOUTRED trap a022024c goes to PC=a022024c. trap lives at 3/a0c02584 within CKOUTRED
      /*26842886 +6  push address 00f7bd6b - this is the text to print!
        26842887 +2  push address 00f7bc3d - buffer to copy into
        26842888 +0  word 2a=42   size of buffer */
            {
              uint32 straddrp = reg68k_regs[8+7]+6; // this is usually odd so have to use fetchbyte, most likely this was a pString which had a length byte ahead of it
              uint32 straddr  = lisa_ram_safe_getlong(context,straddrp); // this is usually odd so have to use fetchbyte, most likely this was a pString which had a length byte ahead of it
              if (straddr==0xaf) {return;}

              if (reg68k_pc!=0xa0c02584) return; // filter out memcpy equivalent calls to only ones from CKOUTRED

              uint32 sizeaddr = reg68k_regs[8+7];  // that the wrapper library stripped out on the A-Line call
              uint16 size     = lisa_ram_safe_getword(context,sizeaddr); 
              if (size==0xaf) {return;}

              static char text[132*50];  // 80x25=20000 don't expect things being printed more than 1 screen at a time
              text[0]=0;

              #ifdef DEBUG
              fflush(buglog);
              // this is a first pass at this, so this is not efficient
              fprintf(buglog,"LISACONSOLE: PC:%d/%08x at (%d,%d) clk:%016llx: str@%08x size:%04x text:", context, reg68k_pc, lisa_ram_safe_getbyte(1,0x1f9),lisa_ram_safe_getbyte(1,0x1f8),
              (long long)cpu68k_clocks,straddr,size);
              #endif

              last_display_str=lisa_ram_safe_getlong(context,reg68k_regs[8+7]+2);

              //:                   Workshop
              // 0123456789012345678901234567
              //           1         2      size:  2e=46
              // size:004f text:{V3.9} WORKSHOP: FILE-MGR, SYSTEM-MGR, Edit, Run, Debug, Pascal, Basic, Quit, ? |
              //                01234567890123456789012345678901234567890123456789012345678901234567890123456789|
              //                          1         2         3         4         5         6         7         8
              // size 4f=79
              for (int i=0, j=0; i<size; j++, i++) {
                uint8 c=lisa_ram_safe_getbyte(context,straddr+i); //if (abort_opcode) {fprintf(buglog,"[abort-opcode on read]\n"); abort_opcode=0; return;}

                #ifdef DEBUG
                switch (c) {
                  case 13: fprintf(buglog,"[CR]");                         break;
                  case 10: fprintf(buglog,"[LF]");                         break;
                  case 27: fprintf(buglog,"[ESC]");                        break;
                  case ('L'-'@'): fprintf(buglog,"[FF]");                  break;

                  default:  if (c>31 && c<128) { fprintf(buglog,"%c",c);        }
                            else if (c<32 )    { fprintf(buglog,"[^%c]",c+'@'); }
                            else if (c>126)    { fprintf(buglog,"[0x%02x]",c);  }
                }
                #endif

                if (j<64) {  text[j]=c; text[j+1]=0; }
              }

              // don't open terminalwx window until we see the Workshop header since we can't tell LOS apart from LPW
              // don't mess with the number of spaces below, they match LPW output exactly
              if  (consoletermwindow) {
                                  //:                   Workshop
                                  // 0123456789012345678901234567
                  if  (strncmp(text,"                   Workshop",27)==0) {
                      #define CONSOLETERM 2
                      init_terminal_serial_port(CONSOLETERM); // open terminal window for console
                      lpw_console_output("\e=-0");
                  }
                  lpw_console_output(text);
              }

              #ifdef DEBUG
              fprintf(buglog,"\n");
              fflush(buglog);
              #endif
            }
            break;

      // might not need this, not sure.
      case 0xa0c00050: 
                       #ifdef DEBUG
                       fprintf(buglog,"LISACONSOLE: CLS\n"); 
                       #endif
                       //if (consoletermwindow) lisa_console_output('L'-'@');
                       break; //CLEARTHE screen - 0x0c = ^L (FormFeed char)

      default:
            {
              #ifdef DEBUG
              #ifndef __MSVCRT__
              uint32 straddrp = reg68k_regs[8+7]+0; // this is usually odd so have to use fetchbyte, most likely this was a pString which had a length byte ahead of it
              uint32 straddr  = lisa_ram_safe_getlong(context,straddrp); // this is usually odd so have to use fetchbyte, most likely this was a pString which had a length byte ahead of it
              uint8 size=lisa_ram_safe_getbyte(context,straddr); if (size==0xaf) return;
              fprintf(buglog,"ALINE:%08x:%016llx: A7=%08x\n",alineopcode,(long long)cpu68k_clocks,straddrp);

              for (int i=0; i<32; i+=2)
                  if (lisa_ram_safe_getlong(context,straddrp+ i)==last_display_str ) {
                      fprintf(buglog,"\n\nLISADISPLAY maybe found trap 0x%08xdisplay param is at offset %i off A7\n",alineopcode,i);
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+ 0, lisa_ram_safe_getlong(context,straddrp+ 0));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+ 4, lisa_ram_safe_getlong(context,straddrp+ 4));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+ 8, lisa_ram_safe_getlong(context,straddrp+ 8));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+12, lisa_ram_safe_getlong(context,straddrp+12));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+16, lisa_ram_safe_getlong(context,straddrp+16));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+20, lisa_ram_safe_getlong(context,straddrp+20));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+24, lisa_ram_safe_getlong(context,straddrp+24));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+28, lisa_ram_safe_getlong(context,straddrp+28));
                      fprintf(buglog,"val[@%08x]=%08x\n",straddrp+32, lisa_ram_safe_getlong(context,straddrp+32));
                      fprintf(buglog,"\n\n");
                  }
              return;
              #endif
              #endif
            }
  }
}

XTIMER entry;
XTIMER entry_stop;
XTIMER clks_stop;


int32 reg68k_external_execute(int32 clocks)
{
  entry_stop=cpu68k_clocks+clocks;
  clks_stop=cpu68k_clocks+clocks;
  clks_stop=cpu68k_clocks+clocks;

  int i,j,k=0;


#ifdef DEBUG
    static char text[16384];
    #ifdef SUPPRESS_LOOP_DISASM
      int32 suppress_printregs=0;
      int32 last_regs_idx=0;
      t_regs last_regs[MAX_LOOP_REGS];               // last opcode register values
    #endif

    #ifdef CPU_CORE_TESTER_PATTERN_TEST
    if  (debug_log_cpu_core_tester==1) {                     
         debug_log_cpu_core_tester=100;
         drive_pattern_test();
    }
    #endif

#endif


#define MAX_INSTR_PER_CALL 1000

static t_ipc *ipc;
static mmu_trans_t *mt;
static uint32 last_pc;

#ifdef DEBUG
  if  ( !atexitset)
      {
      //atexit(dump_scc);
      //atexit(dumpallmmu);
      //atexit(xdumpram);
      //atexit(dumpvia);
        atexitset=1;
      }
#endif

{
    last_bus_error_pc=0;

    if ( (reg68k_pc) & 1  || (regs.pc &1)  )  LISA_REBOOTED(0);

    get_next_timer_event();

    #if defined(DEBUG) && defined(DISASM_SKIPPED_OPCODES)
    debug_reg68k_exec_disasm_skipped();
    #endif

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    lastpc24=pc24;
    clks_stop=MIN(clks_stop,cpu68k_clocks_stop);

    DEBUG_LOG(0,"\n\ncpu68k_clocks is:%016llx before entering do-while loop\nwill expire at %016llx",(long long)cpu68k_clocks,
                (long long)clks_stop);
    do {
            abort_opcode=0;
            SET_CPU_FNC_CODE();

            pc24 = reg68k_pc;
            if (reg68k_pc & 1)  LISA_REBOOTED(0);

            /* C ROM */ 
            //#ifdef DEBUG
            //if (lisarom[0x3ffc]==0x02 && lisarom[0x3ffd]==0x11 && pc24==0xfe0270) 
            //   {ALERT_LOG(0,"C ROM 0x275:%02x",lisarom[0x0275]);}
            //#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// disassemble skipped code (within reason)
// 1/0000d42e (lisacx 0 0/0/0) opcode=303c MOVE.W
// 2005.04.15 hack to get at the LisaTest Video test routines.

// Screenshot
//if (pc24==0xd5ec) ascii_screendump();
            #if defined(DEBUG) && defined(LOOKAHEAD)
            debug_reg68k_exec_lookahead();
            #endif

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////
            mt=&mmu_trans[(pc24>>9) & 32767];
            if  (mt->readfn==bad_page)
                {
                    //ALERT_LOG(0,"****************\nCurrent opcode lives inside a bad_page, throwing lisa_mmu_exception @ %ld/%08lx. Previous IR=%04lx",
                    //     (long)context,(long)pc24,(long)InstructionRegister);
                    //ALERT_LOG(0,"mmu_trans[%04lx] segment #%ld\n\n",(long)(pc24>>9) & 32767,(long)(pc24>>17));
                    //fflush(buglog);dumpmmupage(context, (pc24>>17), buglog); fflush(buglog);
                    lisa_mmu_exception(pc24);
                    break;           // this break here causes the "fucked timing" danger warning - it can be ignored
                }

            lastsflag=reg68k_sr.sr_struct.s;

            #ifdef DEBUG
              //if (debug_log_enabled) check_ipct_memory();
              #ifdef SUPPRESS_LOOP_DISASM
              reg68k_exec_suppress_loop_disasm();
              #else
                if (debug_log_enabled)  {printregs(buglog,"");} // printlisatime(buglog);}
              #endif
            #endif
            // get the page and the mmu_translation table entry for this pc24
            page=(pc24 & ADDRESSFILT)>>9;     mt=&mmu_trans[page];

            // Is this page table allocated?  If not allocate it as needed.
            if (mt!=NULL && mt->table!=NULL)
            {
                ipc=&(mt->table->ipc[(pc24 & 0x1ff)>>1]);

                // we have an IPC, now check it to see that it matches what's in there
                // this is a sanity check against moved pages, but not against self
                // modifying code which only changes operands - that would be too slow
                // to check.
                abort_opcode=2;
                // this all evaluates to a single if statement, which if the define is set is skipped
                #ifndef EVALUATE_EACH_IPC
                 // these are a bit weird because I don't want the compiler through optimization
                 // to reorder the tests and cause a segfault.
                  volatile static int flag;
                  flag=(ipc==NULL);
                  if  (!flag) flag=(ipc->function==NULL);

                  if  (!flag) 
                      {   uint16 myword=fetchword(pc24 & ADDRESSFILT);
                          if (ipc->opcode!=myword) flag=1;
                      }

                  if (flag) //==13256== Conditional jump or move depends on uninitialised value(s)
                #endif
                {
                    if (abort_opcode==1) break;
                    if (!mt->table) {abort_opcode=2; mt->table=get_ipct(pc24);}  //we can skip free_ipct since there's one already here.
                    abort_opcode=2; cpu68k_makeipclist(pc24 & ADDRESSFILT);
                    if (abort_opcode==1) break; //==24726== Conditional jump or move depends on uninitialised value(s)
                    ipc=&(mt->table->ipc[(pc24 & 0x1ff)>>1]);
                }
                abort_opcode=0;


                #if defined(DEBUG) && defined(ICACHE_DEBUG)
                  reg68k_exec_icache_debug();
                #else
                  debug_log_enabled|=dbx;
                #endif
            } // end of if (mt!=NULL && mt->table!=NULL)
            else // need to make this IPC table
            {
                //if ( !mt)  { fprintf(buglog,"Doh! mt is null! bye!"); EXIT(4);  }
                abort_opcode=2; mt->table=get_ipct(pc24);  if (abort_opcode==1) break;
                //abort_opcode=0;

                abort_opcode=2; cpu68k_makeipclist(pc24);
                if (abort_opcode==1) break;

                #ifdef DEBUG
                    if ( !mt->table)  {DEBUG_LOG(-1,"reg68k_extern_exec: got a null mt->table from makeipclist!");}
                #endif

                ipc=&(mt->table->ipc[(pc24 & 0x1ff)>>1]);
            }

            // If the page isn't RAM or ROM, then we can't execute it.
            // I can get rid of this check to speed things up... but...

            #ifdef DEBUG
              if (mt->readfn!=ram && mt->readfn!=sio_rom && mt->readfn!=sio_mmu)
              {
                  EXITR(397,0,"Woot! Trying to execute from non-ram/rom! Living dangerously!"
                            "Bye Bye! PC24= %08lx  ipc# %ld mt->readfn=%ld %s\n",(long)pc24,(long)(pc24 & 0x1ff)>>1,(long)mt->readfn,mspace(mt->readfn));
              }
              else
            #endif
              {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                #ifdef DEBUG
                  reg68k_exec_debug_block(clocks,mt,k,ipc,text);
                #endif
               //this guy 20190630}
                last_cpu68k_clocks=cpu68k_clocks;

               // Cheat to skip over big wait loop in NMI code
                #ifdef SKIP_BIGWAIT_NMI_CODE
                  reg68k_exec_skip_bigwait_nmi_code();
                #endif

                if (abort_opcode==1) {EXITR(476,0,"MMU/BUS/Address exception occured on opcode fetch!\n");}
                abort_opcode=0;          // clear any addr/bus errors/traps/etc that may have occured.

                InstructionRegister=ipc->opcode;

                
                if   (ipc->function)                               // if the IPC is valid, and loaded            // valgrind:==24726== Conditional jump or move depends on uninitialised value(s)
                     {
                       #if defined(DEBUG) && defined(CPU_CORE_TESTER)
                       reg68k_ext_core_tester_pre();
                       #endif
                       SET_CPU_FNC_DATA();
                       #ifdef CHECK_HIGH_BYTE_PRESERVE
                       static int tested;
                       uint32 opc=pc24;
                       #endif
                       ipc->function(ipc);
                       #ifdef CHECK_HIGH_BYTE_PRESERVE
                       if ((opc & 0xff000000)!=0 && (reg68k_pc & 0xff000000)==0) {
                         if ( (reg68k_pc -(opc & 0x00ffffff))<16 ) ALERT_LOG(0,"Lost high byte in PC oldpc:%08x after:%08x",opc,reg68k_pc);
                       }

                       if (  (opc & 0xff000000)==(reg68k_pc & 0xff000000) && (reg68k_pc & 0xff000000)!=0 && ! tested)
                          {ALERT_LOG(0,"high byte test PASSED."); tested=1;}
                       #endif
                     }     // execute the function, else rebuild the IPC //==24726==    by 0x300669: init_ipct_allocator (cpu68k.c:813)
                else  {                                             //                                            // Uninitialised value was created by a heap allocation
                        static t_iib *piib;

                        PAUSE_DEBUG_MESSAGES();
                        if (!(piib = cpu68k_iibtable[fetchword(reg68k_pc)]))
                            {
                               ALERT_LOG(1,"Invalid instruction @ %08lX\n", (long)reg68k_pc); // RA
                               piib = cpu68k_iibtable[0x4AFC*2]; // point to illegal opcode
                               ipc->function=cpu68k_functable[0x4afc];
                            }
                        RESUME_DEBUG_MESSAGES();

                        SET_CPU_FNC_CODE();
                        #ifdef DEBUG
                          if (!ipc)  DEBUG_LOG(0,"About to send null IPC!");
                          if (!piib) DEBUG_LOG(0,"About to send null PIIB!");
                        #endif

                        cpu68k_ipc(reg68k_pc,piib,ipc);

                        #ifdef DEBUG
                          if (piib->clocks!=ipc->clks)
                              DEBUG_LOG(0,"ERROR:iib clocks:%ld != ipc clocks:%ld !", (long)piib->clocks , (long)ipc->clks);
                          DEBUG_LOG(0,"Got an IPC without a function at %08lx, opcode is:%04lx doing it manually - like it's just 0000 OR ",(long)reg68k_pc,(long)ipc->opcode);
                        #endif

                        if (abort_opcode==1) break;

                        PAUSE_DEBUG_MESSAGES();
                        ipc->function=cpu68k_functable[fetchword(reg68k_pc) * 2 + 1];
                        if (!ipc->function) ipc->function=cpu68k_functable[0x4afc*2];
                        if (!ipc->function) ALERT_LOG(0,"Failed to get function for illegal opcode %04x", 0x4afc); //(unsigned int)cpu68k_functable[0x4afc]);

                        PAUSE_DEBUG_MESSAGES();

                        SET_CPU_FNC_DATA();
                        if  (abort_opcode==1) 
                            {
                              DEBUG_LOG(0,"MMU/BUS/Address exception occured on opcode fetch!\n"); 
                              fflush(buglog); 
                              break;
                            }
                        else {
                              InstructionRegister=ipc->opcode;
                              abort_opcode=0;
                              #if defined(DEBUG) && defined(CPU_CORE_TESTER)
                              reg68k_ext_core_tester_pre();
                              #endif

                              if  (ipc->function) {
                                  #ifdef CHECK_HIGH_BYTE_PRESERVE
                                  uint32 opc=pc24;
                                  #endif
                                  ipc->function(ipc);
                                  #ifdef CHECK_HIGH_BYTE_PRESERVE
                                  if ((opc & 0xff000000)!=0 && (reg68k_pc & 0xff000000)==0)
                                    if ( (reg68k_pc -(opc & 0x00ffffff))<16 ) ALERT_LOG(0,"Lost high byte in PC oldpc:%08x after:%08x",opc,reg68k_pc);
                                  #endif
                              }
                              else {   EXITR(277,0,"No ipc function at %ld/%08lx, even after attempting to get one!\n",(long)context,(long)pc24);}
                            }
                      }
                  
                    #if defined(DEBUG) && defined(CPU_CORE_TESTER)
                      reg68k_ext_core_tester_post();
                    #endif

                    #if defined(DEBUG)
                      reg68k_ext_exec_various_dbug();
                    #endif

                    pc24 = reg68k_pc;
                    abort_opcode=0;
                    cpu68k_clocks+=ipc->clks;
              } // if execute from ram/rom else statement

    #ifdef DEBUG
      if (0==reg68k_pc) { DEBUG_LOG(0,"pc=0 LastPC24=%08lx pc24=%08lx abort_opcode:%ld",(long)lastpc24,(long)pc24,(long)abort_opcode);}
    #endif

    clks_stop=(MIN(clks_stop,cpu68k_clocks_stop));

   // one opcode at a time if we're doing a pattern test
   #ifdef CPU_CORE_TESTER
   if (debug_log_cpu_core_tester==100) clks_stop=cpu68k_clocks;
   #endif

  } while (clks_stop>cpu68k_clocks && !regs.stop);


#ifdef DEBUG
    DEBUG_LOG(0,"pc=%08lx cpu68k_clocks is:%016llx stop is :%016llx diff:%016llx  regs.stop is %ld event:%d %s after exiting do-while loop",
            (long)reg68k_pc, (long long)cpu68k_clocks, (long long)cpu68k_clocks_stop,
            (long long)cpu68k_clocks_stop-cpu68k_clocks, (long)regs.stop,
            next_timer_id(),gettimername(next_timer_id() ) );

    printregs(buglog,""); 
#endif

    last_pc=reg68k_pc;
    regs.pc = reg68k_pc;  regs.sr.sr_int = reg68k_sr.sr_int;


   // handle NMI - NMI unlike Bus Error occurs AFTER the current opcode completes. ////////////////////////////////////////////
    if (nmi_clk && nmi_pc)
    {  // regs.stop=nmi_stop;                                    // NMI can only occur on memory access resulting in soft parity error, not while cpu is stopped
        regs.stop=0;
        reg68k_internal_vector(0x1f, reg68k_pc,nmi_addr_err);  // 7 is wrong for vector, right for autovector.
        nmi_addr_err=0;  nmi_clk=0; nmi_pc=0; nmi_stop=0;      // clear flags
        regs.stop=0;
    }
   else  // NMI did not occur, instead, see if there's a pending IRQ, or we're very close to one.
    {
                              // check for the next pending IRQ's.
          if (cpu68k_clocks+10>=cpu68k_clocks_stop)
          {
            //long x=get_wx_millis();
            fire_pending_internal_autovector();
            //long y=get_wx_millis();
            check_current_timer_irq();
            //long z=get_wx_millis();

            //ALERT_LOG(0,"irq timings: fire_pending_int_auto:%ldms, check_current_timer_irq:%ldms, total:%ldms",y-x,z-y,z-x);
          }
    }

    if (regs.stop)  cpu68k_clocks=cpu68k_clocks_stop;
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //insetjmpland=0; //2019601
  }

 return entry_stop-cpu68k_clocks;           // how many cycles left over if positive, negative if did too many.
}



#ifdef CPU_CORE_TESTER_PATTERN_TEST

void test_an_opcode_a(uint32 a0, uint32 a1, uint32 d0, uint32 d1, uint8 s) {

  for (int i=0; i<16; i++) reg68k_regs[i]=0xdeadbeef;

  regs.stop=0;
  reg68k_pc=pc24=0x20000;
  reg68k_regs[0]=d0;
  reg68k_regs[1]=d1;
  reg68k_regs[8]=a0;
  reg68k_regs[9]=a1;
  regs.sp=reg68k_regs[15]=0x40000;

  regs.sr.sr_struct.t=0;
  regs.sr.sr_struct.s=0;

  regs.sr.sr_struct.z=!!(s &  1);
  regs.sr.sr_struct.x=!!(s &  2);
  regs.sr.sr_struct.n=!!(s &  4);
  regs.sr.sr_struct.v=!!(s &  8);
  regs.sr.sr_struct.c=!!(s & 16);
  
  regs.sr.sr_struct.i2=1;
  regs.sr.sr_struct.i1=1;
  regs.sr.sr_struct.i0=1;

  reg68k_external_execute(1);
  if (reg68k_pc<0x20000 || reg68k_pc>0x20010)
     {fprintf(stdout,"Error in testing, PC is outside range:%08x\n",reg68k_pc);}
}


void drive_pattern_test(void)
{

uint32 i,j,k;

uint32 pattern[]={
0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006, 0x00000007, 0x00000008, 0x00000009,
0x0000000a, 0x0000000b, 0x0000000c, 0x0000000d, 0x0000000e, 0x0000000f, 0x00000010, 0x00000011, 0x00000012, 0x00000013,
0x00000014, 0x00000015, 0x00000016, 0x00000017, 0x00000018, 0x00000019, 0x0000001a, 0x0000001b, 0x0000001c, 0x0000001d,
0x0000001e, 0x0000001f, 0x00000020, 0x00000021, 0x00000022, 0x00000023, 0x00000024, 0x00000025, 0x00000026, 0x00000027,
0x00000028, 0x00000029, 0x0000002a, 0x0000002b, 0x0000002c, 0x0000002d, 0x0000002e, 0x0000002f, 0x00000030, 0x00000031,
0x00000032, 0x00000033, 0x00000034, 0x00000035, 0x00000036, 0x00000037, 0x00000038, 0x00000039, 0x0000003a, 0x0000003b,
0x0000003c, 0x0000003d, 0x0000003e, 0x0000003f, 0x00000040, 0x00000041, 0x00000042, 0x00000043, 0x00000044, 0x00000045,
0x00000046, 0x00000047, 0x00000048, 0x00000049, 0x0000004a, 0x0000004b, 0x0000004c, 0x0000004d, 0x0000004e, 0x0000004f,
0x00000050, 0x00000051, 0x00000052, 0x00000053, 0x00000054, 0x00000055, 0x00000056, 0x00000057, 0x00000058, 0x00000059,
0x0000005a, 0x0000005b, 0x0000005c, 0x0000005d, 0x0000005e, 0x0000005f, 0x00000060, 0x00000061, 0x00000062, 0x00000063,
0x00000064, 0x00000065, 0x00000066, 0x00000067, 0x00000068, 0x00000069, 0x0000006a, 0x0000006b, 0x0000006c, 0x0000006d,
0x0000006e, 0x0000006f, 0x00000070, 0x00000071, 0x00000072, 0x00000073, 0x00000074, 0x00000075, 0x00000076, 0x00000077,
0x00000078, 0x00000079, 0x0000007a, 0x0000007b, 0x0000007c, 0x0000007d, 0x0000007e, 0x0000007f, 0x00000080, 0x00000081,
0x00000082, 0x00000083, 0x00000084, 0x00000085, 0x00000086, 0x00000087, 0x00000088, 0x00000089, 0x0000008a, 0x0000008b,
0x0000008c, 0x0000008d, 0x0000008e, 0x0000008f, 0x00000090, 0x00000091, 0x00000092, 0x00000093, 0x00000094, 0x00000095,
0x00000096, 0x00000097, 0x00000098, 0x00000099, 0x0000009a, 0x0000009b, 0x0000009c, 0x0000009d, 0x0000009e, 0x0000009f,
0x000000a0, 0x000000a1, 0x000000a2, 0x000000a3, 0x000000a4, 0x000000a5, 0x000000a6, 0x000000a7, 0x000000a8, 0x000000a9,
0x000000aa, 0x000000ab, 0x000000ac, 0x000000ad, 0x000000ae, 0x000000af, 0x000000b0, 0x000000b1, 0x000000b2, 0x000000b3,
0x000000b4, 0x000000b5, 0x000000b6, 0x000000b7, 0x000000b8, 0x000000b9, 0x000000ba, 0x000000bb, 0x000000bc, 0x000000bd,
0x000000be, 0x000000bf, 0x000000c0, 0x000000c1, 0x000000c2, 0x000000c3, 0x000000c4, 0x000000c5, 0x000000c6, 0x000000c7,
0x000000c8, 0x000000c9, 0x000000ca, 0x000000cb, 0x000000cc, 0x000000cd, 0x000000ce, 0x000000cf, 0x000000d0, 0x000000d1,
0x000000d2, 0x000000d3, 0x000000d4, 0x000000d5, 0x000000d6, 0x000000d7, 0x000000d8, 0x000000d9, 0x000000da, 0x000000db,
0x000000dc, 0x000000dd, 0x000000de, 0x000000df, 0x000000e0, 0x000000e1, 0x000000e2, 0x000000e3, 0x000000e4, 0x000000e5,
0x000000e6, 0x000000e7, 0x000000e8, 0x000000e9, 0x000000ea, 0x000000eb, 0x000000ec, 0x000000ed, 0x000000ee, 0x000000ef,
0x000000f0, 0x000000f1, 0x000000f2, 0x000000f3, 0x000000f4, 0x000000f5, 0x000000f6, 0x000000f7, 0x000000f8, 0x000000f9,
0x000000fa, 0x000000fb, 0x000000fc, 0x000000fd, 0x000000fe, 0x000000ff,                 0x7fff, 0x7fff7fff, 0x7fffffff,
0x10101010, 0x01010101, 0x20202020, 0x02020202, 0x40404040, 0x04040404, 0x08080808, 0x80808080, 0xaa00aa00, 0x00aa00aa,
0x0a0a0a0a, 0xa0a0a0a0, 0x55005500, 0x00550055, 0x50505050, 0x05050505, 0xff00ff00, 0x00ff00ff, 0xaa55aa55, 0x55aa55aa,
0xff55ff55, 0x55ff55ff, 0xffaa55aa, 0xaaffaaff, 0xf0f0f0f0, 0x0f0f0f0f, 0x77777777, 0x88888888, 0xaaaaaaaa, 0xbbbbbbbb,
0xcccccccc, 0xdddddddd, 0xeeeeeeee, 0xffffffff,
0xdeadbeef}; // (misfolded) Prion enabled cow says "woof!" :)


  for (i=0x20010; i<0x30000; i++) lisa_ww_ram(i,0x4EFA); // fill ram with JMP to end this basic block - long stretches of instructions kill the ipc creation process

  for (i=0; i<294; i++) {
        fprintf(stdout,"\n\n================================= test set %d of 293 ====================================================\n\n\n",i);
        k=8192;
        //for (k=0; k<32; k++) // x,c,v,z,n
        {
            uint32 d0=pattern[i], d1=pattern[i]; // 1st batch only d0 is used, d1 is filler
            d0=d0 & 0xfffffffe; // avoid odd address errors
            d1=d1 & 0xfffffffe; // avoid odd address errors

//            lisa_ww_ram(0x20000,0x48c0);  test_an_opcode_a(d0,d1,d0,d1,k);                                         // c048 EXT.L D0
//            lisa_ww_ram(0x20000,0x56c0);  test_an_opcode_a(d0,d1,d0,d1,k);                                         // c056 SNE.B.D0
//            lisa_ww_ram(0x20000,0x57c0);  test_an_opcode_a(d0,d1,d0,d1,k);                                         // c056 SEQ.B.D0
//            lisa_ww_ram(0x20000,0xe448);  test_an_opcode_a(d0,d1,d0,d1,k);                                         // 48e4 LSR #2, D0

            for (j=0; j<294; j++) {
               d1=pattern[j];
               d1=d1 & 0xfffffffe; // avoid odd address errors

               for (int z=0x20000; z<0x20010; z++) lisa_ww_ram(z,0x4E71); // fill ram with NOP

//                lisa_ww_ram(0x20000,0xbc43); lisa_wb_ram(0x2002,(d0 & 0xffff));         test_an_opcode_a(d0,d1,d0,d1,k); // CHK.W #$0000,D1
//                lisa_ww_ram(0x20000,0x47fb); lisa_wb_ram(0x2002,(d1 & 0xff) | 0x0800 ); test_an_opcode_a(d0,d1,d0,d1,k); // LEA (65,PC,D0.L),A3 // 65-2 because PC is involved.
//                lisa_ww_ram(0x20000,0x47f0); lisa_wb_ram(0x2002,(d1 & 0xff) | 0x0800 ); test_an_opcode_a(d0,d1,d0,d1,k); // LEA (65,A0,D0.L),A3 // 0x41=65
              //lisa_ww_ram(0x20000,0x47E8); lisa_wb_ram(0x2002,(d0 & 0xffff));         test_an_opcode_a(d0,d1,d0,d1,k); // LEA (65,A0),A3
//                lisa_ww_ram(0x20000,0xd181);                                            test_an_opcode_a(d0,d1,d0,d1,k); // d181 ADDX.L D1,D0

                //00020000  33B0 0002 1004            10     move.w 2(A0,D0.W),4(A1,D1.W):
                lisa_ww_ram(0x20000,0x33b0); lisa_ww_ram(0x20002,0x0002); lisa_ww_ram(0x20004,0x1004); test_an_opcode_a(0x30000, 0x400000,d0,d1,k);
                lisa_ww_ram(0x20000,0x47F1); lisa_ww_ram(0x20002,0x7004); test_an_opcode_a(0x30000, 0x400000,d0,d1,k); // 47F1 1004   lea  4(A1,D1.W),A3
                lisa_ww_ram(0x20000,0x47F0); lisa_ww_ram(0x20002,0x6002); test_an_opcode_a(0x30000, 0x400000,d0,d1,k); // 47F0 1002   lea  2(A0,D1.W),A3
                lisa_ww_ram(0x20000,0x47F1); lisa_ww_ram(0x20002,0x0008); test_an_opcode_a(0x30000, 0x400000,d0,d1,k); // 47F1 0008   lea  8(A1,D0.W),A3
                lisa_ww_ram(0x20000,0x47F0); lisa_ww_ram(0x20002,0x000A); test_an_opcode_a(0x30000, 0x400000,d0,d1,k); // 47F0 000A   lea  10(A0,D0.W),A3


        }
    }
   }

   fprintf(stdout,"\n\n\n==========================================================================================\n");
   fprintf(stdout,      "==========================================================================================\n");
   fprintf(stdout,      "==========================================================================================\n");
   fprintf(stdout,      "======================                That's all Folks!              =====================\n");
   fprintf(stdout,      "==========================================================================================\n");
   fprintf(stdout,      "==========================================================================================\n");
   fprintf(stdout,      "==========================================================================================\n");

   exit(1);
}
#endif


void reg68k_external_autovector(int avno)
{
    /* move PC and register block into global processor register variables */
//20190601        reg68k_pc = regs.pc;
//20190601        reg68k_regs = regs.regs;
//20190601        reg68k_sr.sr_int = regs.sr.sr_int;
        DEBUG_LOG(0,"Inside setjmp land, about to call internal_autovector:%ld reg68k_pc:%08lx reg68k_sr:%08lx",(long)avno,(long)reg68k_pc,(long)reg68k_sr.sr_int);
        //insetjmpland=1;

        reg68k_internal_autovector(avno);

        DEBUG_LOG(0,"Autovector:%ld reg68k_pc:%08lx reg68k_sr:%08lx regs.pc:%08lx regs.sr:%08lx, exiting setjmp land",(long)avno,(long)reg68k_pc,(long)reg68k_sr.sr_int,
                  (long)regs.pc,(long)regs.sr.sr_int);
        /* restore global registers back to permanent storage */
        regs.pc = reg68k_pc;
        regs.sr.sr_int = reg68k_sr.sr_int;
        //insetjmpland=1; //20190601
}


void reg68k_external_vector(int vector, uint32 oldpc,uint32 addr_error)
{
    reg68k_internal_vector(vector,oldpc,addr_error);
    regs.pc = reg68k_pc;
    regs.sr.sr_int = reg68k_sr.sr_int;
}


// called by mmuflush to make sure that the correct context is set!
void reg68k_update_supervisor_internal(void)  {lastsflag=reg68k_sr.sr_struct.s;}
void reg68k_update_supervisor_external(void)  {lastsflag=regs.sr.sr_struct.s;}

// Macros for 68000 Vector operations - these are turned into macro's to make the source code more readable and still faster than fn's

#define A7PUSH_BYTE(x)    {abort_opcode=2; reg68k_regs[15] -= 1;    storebyte(reg68k_regs[15], (x));}
#define A7PUSH_WORD(x)    {abort_opcode=2; reg68k_regs[15] -= 2;    storeword(reg68k_regs[15], (x));}
#define A7PUSH_LONG(x)    {abort_opcode=2; reg68k_regs[15] -= 4;    storelong(reg68k_regs[15], (x));}
#define GETVECTOR(x)      (fetchlong((x)*4))
#define SWAPA7withSSP()   {reg68k_regs[15] ^= regs.sp; regs.sp ^= reg68k_regs[15]; reg68k_regs[15] ^= regs.sp;}
#define SETSUPERVISOR(x)  {reg68k_sr.sr_struct.s = (x);}
#define GETSUPERVISOR()   (reg68k_sr.sr_struct.s)

void reg68k_internal_autovector(int avno)   { reg68k_internal_vector(V_AUTO + avno - 1,reg68k_pc,0); }

static uint32 lastoldpc;
static int32  lastclk;
static int lastvno;

#ifdef DISABLEDXXXDEBUG
void find_uniplus_partition_table(void) {
/* find partition table in kernel - should be something like this:
                        a-start      a-size     b-start
RAM 0a6b50: 04 03 02 01]00 00 09 c5[00 00 42 3b]00 00 00 65]|..........B;...e 09c5=2501 (astart) 423b=16955 (asize);  0x65=101=b-start
RAM 0a6b60: 00 00 09 60]00 00 09 c5[00 00 1c 3b]00 00 26 00 |...`.......;..&. 0960=2400 (b-sizeswapsize), 09c5: c-start, 0x13cb=7227=c-size, 0x2600=9728 (dstart)
RAM 0a6b70: 00 00 26 00 00 00 00 00:00 00 00 00 00 00 00 00 |..&............. 0x2600=9728 (dsize), estart=0,esize=0, fstart=0, 
RAM 0a6b80: 00 00 1c 00 00 00 1c 00:00 00 09 c0 00 00 00 65 |...............e 0x1c00=7168 (fsize), gstart=7168=0x1c00, 0065=g_size
RAM 0a6b90: 00 00 4b 9b]00 00 00 00:00 00 00 00 00 00 00 00 |..K............. 4b9b=h_size (whole disk -100)
RAM 0a6ba0: 00 02 d0 ea 00 02 d0 ea:00 00 00 00 00 00 00 00 |................ 02d0ea=184554 not sure what drive that is. 92MB? so priam?

{
   astart = 101+swap;  a_size =   16955; //  root on 10MB disk
   bstart =  101;      b_size =    swap; //  swap (2400 blks normal
   cstart = 101+swap;  c_size =    7227; //  root on 5MB disk
   dstart = 9728;      d_size =    9728; //  2nd fs on 10MB disk
   estart =    0;      e_size =       0; //  unused
   fstart =    0;      f_size =    7168; //  old root - a
   gstart = 7168;      g_size =    2496; //  old swap -b
   hstart =  101;      h_size =   19355; //  whole disk (blocks 0-100 reserved for boot loader)
}

0x000009c5 0x0000423b 
0x00000065 0x00000960
0x000009c5 0x00001c3b
0x00002600 0x00002600 
0x00000000 0x00000000 
0x00000000 0x00001c00
0x00001c00 0x000009c0 
0x00000065 0x00004b9b 
*/

    for (uint32 addr=0x100; addr<0x00fbffff; addr+=2) {

        abort_opcode=2; 
        uint32 val=fetchlong(addr);                            //a-start

        fprintf(stderr,"Searching: %08x\r",addr);
        if (val==0x000009c5) {
                abort_opcode=2; uint32 val1=fetchlong(addr +      ( 1*4));     //a-size
            fprintf(stderr,"\n found 0x000009c5 %08x at %08x (want 0x0000423b)\n",val1,addr);
            if (val1==0x0000423b) {
                abort_opcode=2; uint32 val2=fetchlong(addr +  ( 2*4))==0x00000065; // b-start 2501
                abort_opcode=2; uint32 val3=fetchlong(addr +  ( 3*4))==0x00000960; // b-size
                abort_opcode=2; uint32 val4=fetchlong(addr +  ( 4*4))==0x000009c5; // c-start
                abort_opcode=2; uint32 val5=fetchlong(addr +  ( 5*4))==0x00001c3b; // c-size
                abort_opcode=2; uint32 val6=fetchlong(addr +  ( 6*4))==0x00002600; // d-start
                abort_opcode=2; uint32 val7=fetchlong(addr +  ( 7*4))==0x00002600; // d-size
                abort_opcode=2; uint32 val8=fetchlong(addr +  ( 8*4))==0x00000000; // e-start
                abort_opcode=2; uint32 val9=fetchlong(addr +  ( 9*4))==0x00000000; // e-size
                abort_opcode=2; uint32 vala=fetchlong(addr +  (10*4))==0x00000000; // f-start
                abort_opcode=2; uint32 valb=fetchlong(addr +  (11*4))==0x00001c00; // f-size
                abort_opcode=2; uint32 valc=fetchlong(addr +  (12*4))==0x00001c00; // g-start
                abort_opcode=2; uint32 vald=fetchlong(addr +  (13*4))==0x000009c0; // g-size
                abort_opcode=2; uint32 vale=fetchlong(addr +  (14*4))==0x00000065; // h-start
                abort_opcode=2; uint32 valf=fetchlong(addr +  (15*4))==0x00004b9b; // h-size
                int count= val2+val3+val4+val5+val6+val7+val8+val9+vala+valb+valc+vald+vale+valf;
                if (count > 6)
                   fprintf(stderr, "**** Possible partition table found at address 1/%08x count:%d****\n",addr,count);
                else
                   fprintf(stderr, "**** Low likelyhood partition table found at address 1/%08x count:%d****\n",addr,count);

                for (int i=0; i<16; i++) {
                     val=fetchlong(addr  + (i*4) );
                     if ((i & 1)==0) fprintf(stderr,"1/%08x: %08x ", addr + (i*4),val);
                     else            fprintf(stderr," %08x\n", addr + (i*4),val);
                }
                fprintf(stderr,"\n\n");
            }
        }
     }
     EXIT(0,0,"Partition table location");
}
#endif

extern void apply_uniplus_hacks(void);

#define SEND_STATUS_BYTES_STATE    12    // Let Lisa read the status bytes after the write is done, then return to idle.

extern void  hle_intercept(void);
extern int line15_hle_intercept(void);
//extern void enable_4MB_macworks(void);

void disable_4MB_macworks(void) {
  ALERT_LOG(0,"Need to implement me. or maybe just reset");
}

extern void refresh_vidram_fns(void);

void enable_4MB_macworks(void) {
  return; // broken, do not use yet
  ALERT_LOG(0,"\n\n\n\n\n\n");
  ALERT_LOG(0," **** Enabling 4MB RAM for MacWorks ****");
  ALERT_LOG(0," **** Enabling 4MB RAM for MacWorks ****");
  ALERT_LOG(0," **** Enabling 4MB RAM for MacWorks ****");

  regs.pc = pc24 = reg68k_pc = 0x0002001c+6;
  maxlisaram=0x3E0000;        //0x004000040-(128*1024);
  TWOMEGMLIM=0x003fffff;

  ALERT_LOG(0,"Move vidram to %08x",maxlisaram-32768);
  memmove(&lisaram[maxlisaram-32768],&lisaram[videolatchaddress],32768); // move video data over

  // 4MB RAM sleight of hand hack - was this your card? if minlisaram is set, reallocate RAM to zero
  if (minlisaram) {
     memmove(&lisaram[0],&lisaram[minlisaram],minlisaram);
     
     for (uint32 i=0; i<maxlisaram/0x00200; i++) {mmu_trans_all[1][i].address=0; mmu_trans_all[1][i].readfn=17; mmu_trans_all[1][i].writefn=17;} 
     for (uint32 i=0; i<maxlisaram/0x20000; i++) {mmu[i].sor=0x000; mmu[i].slr=0x0700;}

     minlisaram=0;
  }

  // might be able to get as high as 8MB as videoram latch=255 -> 0x7F8000
  videolatchaddress  = maxlisaram-32768;
  videolatch         = videolatchaddress / 32768;

  ALERT_LOG(0,"Set vidlatch to %02x address: %08x",videolatch,videolatchaddress);
  ALERT_LOG(0,"Min/max/total RAM: %08x - %08x :%08x",minlisaram,maxlisaram,maxlisaram-minlisaram);

  storelong(0x2A4,minlisaram);
  storelong(0x294,maxlisaram);
  storelong(0x2A8,maxlisaram-minlisaram);
  storelong(0x110,videolatchaddress); // ROM SCRNBASE variable
  refresh_vidram_fns();
  sleep(10);
}


void reg68k_internal_vector(int vno, uint32 oldpc, uint32 addr_error)
{
    uint16 saved_sr=reg68k_sr.sr_int;

    int avno=(vno-V_AUTO+1);
    int curlevel = (reg68k_sr.sr_int >> 8) & 7;
    //unused//static uint32 tmpaddr;
    uint16 busfunctioncode;
    uint8 old_supervisor=reg68k_sr.sr_struct.s;


    if (vno==V_LINE15) { if (line15_hle_intercept()) return;}
    if (vno==2 && reg68k_pc == 0x0002001c && addr_error==0x00400000 && macworks4mb) {enable_4MB_macworks(); return;}
    /*-----------------12/8/2003 9:56PM-----------------
     * needs to do this: RA
     * SSP-2 ->SSP                      // push FV -- 680010+ only
     * Format/Vector Offset -> (SSP)
     * SSP-4->4->SSP                    // push PC
     * PC->(SSP)
     * SSP-2 ->SSP                      // push SR
     * SR->(SSP)
     * Vector Address->PC               // PC=Vector
     * --------------------------------------------------*/


   // avoid bus/addr error repeats on multi-operand opcodes...
    if  (lastclk==cpu68k_clocks && lastvno==vno)
        {
          DEBUG_LOG(0,"Suppressing duplicate internal_vector - VECTOR:%ld, oldpc:%08lx clk:%016llx",(long)vno,(long)oldpc,(long long)cpu68k_clocks);
          return;
        }

    lastclk =cpu68k_clocks;   lastvno =vno;   lastoldpc =oldpc;


    if (avno>0 && avno<8)                // If it's an autovector, check the IRQ mask before allowing it to occur.
      {
        if (! (IS_VECTOR_AVAILABLE_INT(avno))) {
            clks_stop=cpu68k_clocks+8192;   // reschedule checking later instead of after every instruction. 5MHz/60Hz=83,333 cpu cycles/ video frame
            DEBUG_LOG(0,"Vector requested is not available (INTMASK too low)"); // otherwise this code slows down to a crawl checking after evern opcode
            return;
        }
        DEBUG_LOG(0,"PC:%08lx AutoVector is:%ld vector:%ld curlevel is %ld cpu stop status is:%ld",(long)reg68k_pc,(long)avno,(long)vno,(long)curlevel,(long)regs.stop);
      }
    else avno=0;                         // clear avno since it wasn't an autovector - this saves time later on the avno check.


    #ifdef DEBUG
      DEBUG_LOG(0,"Entering: internal_vector - VECTOR:%ld (%s), addr_err:%08lx oldpc:%08lx, pc24:%08lx, reg68k_pc:%08lx",
                  (long)vno,getvector(vno), (long)addr_error,(long)oldpc,(long)pc24,(long)reg68k_pc);

      if (oldpc!=reg68k_pc && vno<32)
      ALERT_LOG(0,"DANGER DANGER DANGER oldpc:%08lx is not reg68k_pc:%08lx, pc24:%08lx oldpc24:%08lx vector:%ld",(long)oldpc, (long)reg68k_pc,(long)pc24,(long)lastpc24,(long)vno);

      if (vno==37) lisaos_trap5();        // log LisaOS trap #5 calls by name
      printregs(buglog,"irqdone");

    #endif


    if (!GETSUPERVISOR()) {
        SWAPA7withSSP();
        SETSUPERVISOR(1);
        // 2005.02.01 - removed if statement to force flush here on switch from user to supervisor mode
        //if (segment1|segment2)                 // if we're not in supervisor space already, flush the mmu
        { DEBUG_LOG(0,"Turning Supervisor flag on while in internal_vector flushing mmu context=%ld s1/s2=%ld/%ld start=%ld\n",(long)context,(long)segment1,(long)segment2,(long)start);
          SET_MMU_DIRTY(0xdec0de);  mmuflush(0x3000);
          DEBUG_LOG(0,"post mmuflush context=%d s1/s2=%d/%d start=%d\n",context,segment1,segment2,start);
        }

        DEBUG_LOG(5,"S mode change SP:%08lx/A7:%08lx swapped.",(long)reg68k_regs[15],(long)regs.sp);
        lastsflag=reg68k_sr.sr_struct.s;  regs.pc = reg68k_pc; regs.sr.sr_int = reg68k_sr.sr_int;
    }

   // might be a retake of the same addr/bus error on same opcode.  doh!  ie: when btst is done (read, write) two bus errors can occur
   // this is here because the previous run would have set Supervisor, and if it was already set before that, that's fine too
   // the problem is the following fetchlong can't be done until mmu context is set, and mmu is flushed, else gremlins

    {
      abort_opcode=2;
      uint32 x=GETVECTOR(vno);
      uint32 y=lisa_ram_safe_getlong(1, vno*4);

      if (x & 0xff000000) {ALERT_LOG(0,"OOPS! Vector address has bits24-31 set via GETVECTOR(%ld)=%ld/%08lx - safeget_long is 1/%08lx",(long)vno,(long)CXSASEL,(long)x,(long)y);}

      if (x==0xaf) {
                      ALERT_LOG(0,"OOPS! Got bus error whilst trying to fetch vector:%ld - PC:%ld/%08lx",(long)vno,(long)context,(long)oldpc);
                      SET_MMU_DIRTY(0xdec0de);  mmuflush(0x3000);
                      x=GETVECTOR(vno);
                      if (x==0xaf) ALERT_LOG(0,"Failed again after trying to flush mmu! Something is very wrong!");
                      ALERT_LOG(0,"Attempting the old fetchlong method, if you don't see the result, we're recursive");
                      abort_opcode=2;
//                    x=fetchlong(vno*4);
                      x=lisa_ram_safe_getlong(1, vno*4);

                      ALERT_LOG(0,"Got back from fetchlong:%08lx",(long)x);
                    }

      if (x==0xaf || x==oldpc)  {loopy_vector--; abort_opcode=0;  DEBUG_LOG(0,"retake of same vector"); return;}
    }

    if (abort_opcode==1)        {EXIT(782,0,"Doh! got abort_opcode=1 on vector fetch in %s!\n",__FUNCTION__); }
    abort_opcode=0;

    if (regs.stop && nmi_clk && nmi_pc) {regs.stop=0; nmi_addr_err=0;  nmi_clk=0; nmi_pc=0; nmi_stop=0;}      // clear flags
    else if (regs.stop)
    {
        
        DEBUG_LOG(0,"exiting from STOP opcode @%08lx (oldpc=%08lx) - will continue at %08lx after ISR handles IRQ:%ld.",(long)reg68k_pc,(long)oldpc,(long)reg68k_pc+4,(long)vno);
        oldpc=reg68k_pc+4;//20200712RA
        regs.stop = 0;
        regs.pc=pc24=reg68k_pc=oldpc;
        //pc24=oldpc;
        //regs.pc = oldpc; 
        regs.sr = reg68k_sr;
    }

    if  (vno==2 || vno==3 ||vno==4)
        {
            if        ((InstructionRegister & 0xff00)==0x4a00) {oldpc+=2;}
            else if   ((InstructionRegister & 0xff00)==0x4e00)
            {
              if      ((InstructionRegister & 0x00f0)==0x00d0) {oldpc+=2;}
              else if ((InstructionRegister & 0x00ff)==0x0075) {oldpc+=2; regs.sp+=4;}
              else if ((InstructionRegister & 0x00ff)==0x007a) {oldpc+=0;} // movec vbr,d0 from http://bitsavers.org/pdf/apple/lisa/unisoft/Unisoft_Lisa_Kernel_Aug1983.pdf p3 determine cpu type
              else if ((InstructionRegister & 0x00ff)==0x0073) {oldpc+=0;}
              else if ((InstructionRegister & 0x00ff)==0x00f9) {oldpc+=2;}
              else if ((InstructionRegister & 0x00ff)==0x00b9) {oldpc+=6;}
              else if ((InstructionRegister & 0x00ff)==0x00a8) {oldpc+=4;}
              else if ((InstructionRegister & 0x00ff)==0x0090) {oldpc+=2;}
              else    {
                        ALERT_LOG(0,"\n\n\7*** DANGER *** Unhandled opcode in buserror:%04lx\n\n\7",(long)InstructionRegister);
                        oldpc+=(cpu68k_iibtable[InstructionRegister]->wordlen<<1);
                      }
            }
        }

    if (uniplus_hacks && vno==4 && InstructionRegister==0x4e7a) apply_uniplus_hacks();

    DEBUG_LOG(0,"PUSH PC %08lx context:%ld",(long)oldpc,(long)context);  

    // this fails, if oldpc points to a page that's lisa_wl_bad_page - it will recursively call bus error and crash.
    // add check before A7PUSH_LONG.
    A7PUSH_LONG(oldpc);  if (abort_opcode==1) {EXIT(783,0,"Doh! got abort_opcode=1 on push pc %s!\n",__FUNCTION__); }

    DEBUG_LOG(0,"PUSH SR %04x context:%d",saved_sr,context);  
    A7PUSH_WORD(saved_sr);
    if (abort_opcode==1) {EXIT(784,0,"Doh! got abort_opcode=1 on push sr in %s!\n",__FUNCTION__); }
    // "Short format 0 only four words are to be removed from the top of the stack. SR and PC are loaded from the stack frame."
    abort_opcode=0;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (vno==2 || vno==3 || vno==4)    // PUSH IS IN THIS ORDER: ---> PC,SR,IR,ADDR,BUSFN
    {  // memerror=(uint16)(( CHK_MMU_TRANS(addr_error) )>>5);

        memerror=(uint16)((               addr_error  )>>5);

        busfunctioncode=CPU_function_code | (CPU_READ_MODE<<4) |1;

        cpu68k_clocks+=(116*2); // Addr/Bus  // *2 2005.04.13.18:43

        DEBUG_LOG(0,"BUS_OR_ADDR: ADDR error:%ld at PC:%08lx pc24:%08lx addr:%08lx clk:%016llx - pushing extended exception frame for 68000",
              (long)vno,(long)oldpc,(long)pc24,(long)addr_error,(long long)cpu68k_clocks);

        DEBUG_LOG(0,"PUSH IR:%04lx context:%ld",(long)InstructionRegister,(long)context); A7PUSH_WORD(InstructionRegister);  if (abort_opcode==1) {EXIT(784,0,"Doh! got abort_opcode=1 on push IR    in %s!\n",__FUNCTION__); }
        DEBUG_LOG(0,"PUSH AE:%08lx context:%ld",(long)addr_error,(long)context);          A7PUSH_LONG(addr_error);           if (abort_opcode==1) {EXIT(784,0,"Doh! got abort_opcode=1 on push ADDRC in %s!\n",__FUNCTION__); }
        DEBUG_LOG(0,"PUSH BF:%04lx context:%ld",(long)busfunctioncode,(long)context);     A7PUSH_WORD(busfunctioncode);      if (abort_opcode==1) {EXIT(784,0,"Doh! got abort_opcode=1 on push BUSFN in %s!\n",__FUNCTION__); }

        // prevent autovectors from interrupting BUS/ADDR exception ISR's.
        //DEBUG_LOG(0,"Set Mask to 7 for BUS/ADDR Exception");

        // this might not be such a good idea
        // reg68k_sr.sr_int |=(7 << 8);      // no need for &= since we turn on all 3 bits.
        regs.pc = reg68k_pc;  regs.sr.sr_int = reg68k_sr.sr_int;
    }
    else // is this an autovector? if so handle that.
    {
        if (avno)                                   //2005.02.02 - code refactoring/consolidation
        {
          #ifdef DEBUG
           if (avno==1 && floppy_FDIR) append_floppy_log("reg68k_int_av_ ***Firing IRQ1 because FDIR is set***\n");
          #endif

        //reg68k_sr.sr_struct.t = 0;                // disable trace bit - no need for it - it's handled below
          reg68k_sr.sr_int &= ~0x0700;              // clear IRQ Mask
          reg68k_sr.sr_int |= (avno<<8);            // set IRQ mask to avno
        //tmpaddr = reg68k_pc;                      // not used

          DEBUG_LOG(0,"autovector#%ld V_AUTO:%08lx Vector address: %ld * 4 = %08lx",(long)vno-V_AUTO+1,(long)V_AUTO,(long)vno,(long)(vno*4));

        //abort_opcode=2;reg68k_pc = GETVECTOR(vno);  if (abort_opcode==1) {fprintf(buglog,"Doh! got abort_opcode=1 while fetching vector for IRQ!\n"); EXIT(781);}
        }
       cpu68k_clocks+=87;  // IRQ1-7
    }


   ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    abort_opcode=2;  reg68k_pc=GETVECTOR(vno);    // should turn this into vector 15 - spurious IRQ
    if (abort_opcode==1) {EXIT(58,0,"Doh got abort_opcode=1 on vector fetch in %s - BYE BYE\n",__FUNCTION__); }
    if (reg68k_pc &1)    {EXIT(58,0,"Doh odd PC value (%08x) on vector (%d) fetch in %s - BYE BYE\n",reg68k_pc,vno,__FUNCTION__); }

    abort_opcode=0;

    DEBUG_LOG(0,"SRC: VECTOR IRQ: %s oldpc:%08lx -> newpc:%08lx A7=%08lx SP=%08lx Mode pre-IRQ:%s\n",
                 getvector(vno), (long)oldpc, (long)reg68k_pc, (long)reg68k_regs[15], (long)regs.sp, 
                (old_supervisor ? "Supervisor":"User") );

    regs.pending = get_pending_vector();                                           // not sure that I need this any longer - RA


    if (0==reg68k_pc)  
    {
      #ifdef DEBUG
      debug_on("reg68k_pc=0"); 
      #endif
      EXIT(59,0,"reg68k_pc got nullified. bye");  
    }
    if (reg68k_pc& 1)   LISA_REBOOTED();

    reg68k_sr.sr_struct.t=0;                                                       // turn off trace on trap.
    regs.pc = reg68k_pc; regs.sr.sr_int = reg68k_sr.sr_int;                        // flush to external copy of regs
    DEBUG_LOG(0,"Done with vector fn. regs.pc=%08lx reg68k_pc:%08lx",(long)regs.pc,(long)reg68k_pc);

    abort_opcode=0;
    return;
}


#ifdef DEBUG
void print_pc_and_regs(char *text)
{
  if (buglog) 
    { fprintf(buglog,"\n\n%s PC%08lx\n",text, (long)reg68k_pc);  fflush(buglog);
      printregs(buglog,""); fflush(buglog);}
}
#endif

static uint32 last_be_ex_pc;
static XTIMER last_be_clocks;

void lisa_buserror(uint32 addr_error)
{
    if (reg68k_pc == last_be_ex_pc && cpu68k_clocks == last_be_clocks) return;
    last_be_ex_pc=reg68k_pc; last_be_clocks=cpu68k_clocks;

    reg68k_internal_vector(2 ,reg68k_pc, addr_error);
    abort_opcode=1;
}

#define LINE(x) DEBUG_LOG(0,"%s:%s:%d %s LINE LOG"       ,__FILE__,__FUNCTION__,__LINE__,x); fflush(buglog);

static uint32 last_mmu_ex_pc;
static XTIMER last_mmu_clocks;

void lisa_mmu_exception(uint32 addr_error)
{
    if (reg68k_pc == last_mmu_ex_pc && cpu68k_clocks == last_mmu_clocks) return;
    last_mmu_ex_pc=reg68k_pc; last_mmu_clocks=cpu68k_clocks;

    memerror=(uint16)(( CHK_MMU_TRANS(addr_error) )>>5);

    lisa_buserror(addr_error);
}

static uint32 lastaddrpc=0;
void lisa_addrerror(uint32 addr_error)
{
#ifndef CPU_CORE_TESTER
    ALERT_LOG(0,"Odd Address Exception @%08lx PC=%08lx clk:%016llx ",(long)addr_error,(long)reg68k_pc,(long long)cpu68k_clocks);
    DEBUG_LOG(0,"ADDRESS EXCEPTION @%08lx PC=%08lx",(long)addr_error,(long)reg68k_pc);
    if (reg68k_pc==lastaddrpc) return;
    reg68k_internal_vector(3,reg68k_pc,addr_error);
    lastaddrpc=reg68k_pc;
    abort_opcode=1;
#endif

}


// this may only be called from internal land!
void lisa_nmi_vector(uint32 addr_error)
{
   //unused//uint32 pc_before_nmi=reg68k_pc;
   //20060105memerror=(uint16)(( CHK_MMU_TRANS(addr_error)+(maxlisaram!=RAM1024K ? 0:RAM512K) )>>5);
    memerror=(uint16)(( CHK_MMU_TRANS(addr_error) )>>5);

//  ALERT_LOG(0,"NMI @%08x PC=%08x clk:%016lx memerr latch:%04x - resucitated memlatch:%08x",addr_error,reg68k_pc,cpu68k_clocks,memerror,(uint32)(memerror<<5));
    DEBUG_LOG(0,"NMI Exception @%08lx (phys:%08lx) PC=%08lx clk:%016llx ",
                (long)addr_error,
                (long)(CHK_MMU_TRANS(addr_error)),
                (long)reg68k_pc,
                (long long)cpu68k_clocks);

    DEBUG_LOG(0,"NMI ERROR: current state: context:%ld seg1:%ld seg2:%ld start:%ld",(long)context,(long)segment1,(long)segment2,(long)start);

    nmi_pc      = reg68k_pc;
    nmi_addr_err= addr_error;
    nmi_clk     = cpu68k_clocks;
    nmi_stop    = regs.stop;

    regs.stop=1;
    return;
}

void lisa_external_nmi_vector(uint32 addr_error)
{
    uint32 pc_before_nmi=reg68k_pc;

    DEBUG_LOG(0,"External NMI vector @%08lx PC=%08lx clk:%016llx ",(long)addr_error,(long)reg68k_pc,(long long)cpu68k_clocks);
    DEBUG_LOG(0,"\n** NMI @%08lx pc24==%08lx\n",(long)addr_error,(long)pc24);
    DEBUG_LOG(0,"NMI extern");

    if (abort_opcode || last_nmi_error_pc==reg68k_pc) { if (buglog) {fprintf(buglog,"suppressing NMI - abort_opcode=1\n");} return;}

    //#ifdef DEBUG
      if (nmi_error_trap>3 || last_nmi_error_pc==pc24) {EXIT(786,0,"NMI on top of NMI!\n"); };
      last_nmi_error_pc=reg68k_pc;
      nmi_error_trap++;
    //#endif

    DEBUG_LOG(0,"*** SRC: Lisa NMI - likely for parity test @ %08lx pc=%08lx ***",(long)addr_error,(long)reg68k_pc);
    DEBUG_LOG(0,"int VECTOR 1F (autovector 6 NMI.  mem addr:%08lx, reg68k_pc:%08lx pc24:%08lx lastpc24:%08lx",
              (long)addr_error, (long)reg68k_pc,(long)pc24,(long)lastpc24);
   // reg68k_external_autovector(7);         // using 7 now    ???
    memerror=(uint16)(( CHK_MMU_TRANS(addr_error) )>>5);
    reg68k_external_vector(0x1f, reg68k_pc,addr_error);  // 7 is wrong for vector, right for autovector.  // DANGER DANGER was pc24
    abort_opcode=1;                                      /// VERY STRANGE THAT THIS NEEDS TO BE SO!!! Something odd about this???

    DEBUG_LOG(0,"Done with external_vector call. reg68k_pc is now %08lx, it originally was %08lx",(long)reg68k_pc,(long)pc_before_nmi);
    if (reg68k_pc & 1) {if (buglog) fprintf(buglog,"%s:%s:%d reg68k_pc is now odd!!!!%08lx\n",__FILE__,__FUNCTION__,__LINE__,(long)pc24);}
    //#ifdef DEBUG
      nmi_error_trap--;
    //#endif
}
