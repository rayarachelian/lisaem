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
//


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


int get_exs0_pending_irq_2xpar(void) { return (via[3].via[IFR]&via[3].via[IER])||((via[4].via[IFR]&via[4].via[IER]));}
int get_exs1_pending_irq_2xpar(void) { return (via[5].via[IFR]&via[5].via[IER])||((via[6].via[IFR]&via[6].via[IER]));}
int get_exs2_pending_irq_2xpar(void) { return (via[7].via[IFR]&via[7].via[IER])||((via[8].via[IFR]&via[8].via[IER]));}



// These is here in this file because they are shared IRQs, it belongs more in the via6522.c file but there is a
// videoirq and floppy component too, so it should be in a shared place.  Might as well be static inline for speed.

int get_irq1_pending_irq(void )
{
            // this is just a fixup for the COPS VIA, doesn't affect the rest of this.
            if (via[1].via[IER] & via[1].via[IFR] & 0x7f) via[1].via[IFR] |=0x80; else  via[1].via[IFR] &=0x7f;

            // IRQ1 is the only shared IRQ - Vertical Retrace, Floppy FDIR, and VIA2 (Parallel Port) all use it.
            // fix via IFR bit 0x80's so bit 7 is properly reflecting enabled IRQ's.  Correct these bits before checking.
            if (via[2].via[IER] & via[2].via[IFR] & 0x7f) via[2].via[IFR] |=0x80; else  via[2].via[IFR] &=0x7f;

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





static inline uint8 get_pending_vector(void)
{
  int highest=0;


  //---- recalculated pending vector bitmap, and find the highest IRQ to be serviced  ------------


  #ifdef DEBUG
  pending_vector_bitmap=                                         // no such thing as IRQ0 on 68000
   (get_irq1_pending_irq() ? BIT1:0) |                           // VTIR/FDIR/VIA2   IRQ1
   (get_cops_pending_irq() ? BIT2:0) |                           // COPS/VIA1        IRQ2
   (get_exs2_pending_irq() ? BIT3:0) |                           // Expansion slot 2 IRQ3
   (get_exs1_pending_irq() ? BIT4:0) |                           // Expansion slot 1 IRQ4
   (get_exs0_pending_irq() ? BIT5:0) |                           // Expansion slot 0 IRQ5
   (get_scc_pending_irq()  ? BIT6:0) |                           // SCC              IRQ6
   (get_nmi_pending_irq()  ? BIT7:0) ;                           // NMI              IRQ7

    highest=highest_bit_num(pending_vector_bitmap);
    if (highest==0xff) highest=0;

   DEBUG_LOG(0,"IRQ1:: vertical retrace:%d floppy_FDIR:%d via2_IFR:%02x  IRQ2:via1_IFR:%02x",
           (vertical && (videoirq & 1)),
           floppy_FDIR,
           via[2].via[IFR],
           via[1].via[IFR]);
   if (highest==1 && floppy_FDIR && (!((regs.sr.sr_int>>8)&7))) append_floppy_log("get_pending_vector:Floppy FDIR IRQ1 is about to be taken if called\n");

   DEBUG_LOG(0,"Next pending vector is:%d, map is:%s%s%s%s%s%s%s",highest,
        ((pending_vector_bitmap & BIT1)  ? "v1":"."),
        ((pending_vector_bitmap & BIT2)  ? "v2":"."),
        ((pending_vector_bitmap & BIT3)  ? "v3":"."),
        ((pending_vector_bitmap & BIT4)  ? "v4":"."),
        ((pending_vector_bitmap & BIT5)  ? "v5":"."),
        ((pending_vector_bitmap & BIT6)  ? "v6":"."),
        ((pending_vector_bitmap & BIT7)  ? "v7":".") );


  #else

  if       (get_nmi_pending_irq()  ) {highest=7;pending_vector_bitmap=BIT7;}
  else if  (get_scc_pending_irq()  ) {highest=6;pending_vector_bitmap=BIT6;}
  else if  (get_exs0_pending_irq() ) {highest=5;pending_vector_bitmap=BIT5;}
  else if  (get_exs1_pending_irq() ) {highest=4;pending_vector_bitmap=BIT4;}
  else if  (get_exs2_pending_irq() ) {highest=3;pending_vector_bitmap=BIT3;}
  else if  (get_cops_pending_irq() ) {highest=2;pending_vector_bitmap=BIT2;}
  else if  (get_irq1_pending_irq() ) {highest=1;pending_vector_bitmap=BIT1;}

  #endif



  return highest;
}


#ifdef DEBUG
static char templine[1024];
#endif

#define IS_VECTOR_AVAILABLE_INT(avno) (  ((reg68k_sr.sr_int >> 8) & 7)<(avno)  || (avno)==7)
#define IS_VECTOR_AVAILABLE_EXT(avno) (  ((regs.sr.sr_int   >> 8) & 7)<(avno)  || (avno)==7)

int is_vector_available(int avno)
{
   if (insetjmpland) return (IS_VECTOR_AVAILABLE_INT(avno));
   return IS_VECTOR_AVAILABLE_EXT(avno);
}




static inline void fire_pending_external_autovector(void)
{
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
}

static inline void fire_pending_internal_autovector(void)
{
 uint8 i=get_pending_vector();
 if (!i) { DEBUG_LOG(0,"No pending IRQs"); return; }

 DEBUG_LOG(0,"Firing pending IRQ:%d if it meets the mask",i);
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

    //fprintf(out,"SRC::init_7E70SLR is set to: r/w %04x/%04x\n",mmu_trans_all[0][0x7e70].readfn,mmu_trans_all[0][0x7e70].writefn);

    //dump_scc();
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
    /* move PC and register block into global processor register variables */
//20190601        reg68k_pc = regs.pc;
//20190601        reg68k_regs = regs.regs;
//20190601        reg68k_sr.sr_int = regs.sr.sr_int;

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
  uint16 slr, sor; //, mfn;
  //uint32 a9 =((pc24) & 0x00ffffff)>>9;
  //uint32 a17=((pc24) & 0x00ffffff)>>17;
  uint32 mad; //filter; mtd;
  //lisa_mem_t fn;

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
  addr=addr & 0x00ffffff;
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

          //20180401//pc24 = reg68k_pc & 0x00ffffff;


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


#ifdef XXXDEBUG
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
          if  (reg68k_pc==0x00fe0090 || pc24==0x00fe0090)
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
                
                }


//        if (reg68k_pc==0x00fe1fee)
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

          if  (reg68k_pc==0x00020000)
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
              }            
}
#endif  /// end of ifdef XXXDEBUG disabled code

#ifdef CPU_CORE_TESTER
void reg68k_ext_core_tester(void)
{
  char texttext[1024];

  abort_opcode=2;  diss68k_gettext(ipc, text);
  snprintf(texttext,1024,"opcode:%s (%04x) @%ld/%08lx icache:%02lx%02lx %02lx%02lx  %02lx%02lx %02lx%02lx  %02lx%02lx %02lx%02lx  %02lx%02lx %02lx%02lx\n",
            text,(long)ipc->opcode,(long)context,(long)reg68k_pc,
              (long)lisa_ram_safe_getbyte(context,pc24+0),    (long)lisa_ram_safe_getbyte(context,pc24+1),    (long)lisa_ram_safe_getbyte(context,pc24+2),
              (long)lisa_ram_safe_getbyte(context,pc24+3),    (long)lisa_ram_safe_getbyte(context,pc24+4),    (long)lisa_ram_safe_getbyte(context,pc24+5),
              (long)lisa_ram_safe_getbyte(context,pc24+6),    (long)lisa_ram_safe_getbyte(context,pc24+7),    (long)lisa_ram_safe_getbyte(context,pc24+8),
              (long)lisa_ram_safe_getbyte(context,pc24+9),    (long)lisa_ram_safe_getbyte(context,pc24+10),   (long)lisa_ram_safe_getbyte(context,pc24+11),
              (long)lisa_ram_safe_getbyte(context,pc24+12),   (long)lisa_ram_safe_getbyte(context,pc24+13),   (long)lisa_ram_safe_getbyte(context,pc24+14),
              (long)lisa_ram_safe_getbyte(context,pc24+15) );
              corecpu_start_opcode(texttext, context);

}
#endif



#endif // end of if DEBUG for reg68k_external_execute debug inlines //////////////////////////////////////////////////////
//------------------------------------------------------------------------------------------------------------------------

#ifdef DEBUG
extern void   check_ipct_memory(void);
#endif

int32 reg68k_external_execute(int32 clocks)
{
  XTIMER entry=cpu68k_clocks;
  XTIMER entry_stop=cpu68k_clocks+clocks;
  XTIMER clks_stop=cpu68k_clocks+clocks;

 // remove these.
 // XTIMER entrystop=cpu68k_clocks_stop;

  int i,j,k=0;


#ifdef DEBUG
    static char text[16384];
    #ifdef SUPPRESS_LOOP_DISASM
      int32 suppress_printregs=0;
      int32 last_regs_idx=0;
      t_regs last_regs[MAX_LOOP_REGS];               // last opcode register values
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

/* move PC and register block into global variables */
//20190601        reg68k_pc   = regs.pc;//20060129// & 0x00ffffff;
//20190601        reg68k_regs = regs.regs;
//20190601        reg68k_sr.sr_int   = regs.sr.sr_int;
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

            pc24 = reg68k_pc; //20180401// & 0x00ffffff;
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
            page=(pc24 & 0x00ffffff)>>9;     mt=&mmu_trans[page];

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
                      {   uint16 myword=fetchword(pc24 & 0x00ffffff);
                          if (ipc->opcode!=myword) flag=1;
                      }

                  if (flag) //==13256== Conditional jump or move depends on uninitialised value(s)
                #endif
                {
                    if (abort_opcode==1) break;
                    if (!mt->table) {abort_opcode=2; mt->table=get_ipct(pc24);}  //we can skip free_ipct since there's one already here.
                    abort_opcode=2; cpu68k_makeipclist(pc24 & 0x00ffffff); if (abort_opcode==1) break; //==24726== Conditional jump or move depends on uninitialised value(s)
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

                #if defined(DEBUG) && defined(CPU_CORE_TESTER)
                  reg68k_ext_core_tester();
                #endif
                
                if   (ipc->function)                               // if the IPC is valid, and loaded            // valgrind:==24726== Conditional jump or move depends on uninitialised value(s)
                     {SET_CPU_FNC_DATA(); ipc->function(ipc);}     // execute the function, else rebuild the IPC //==24726==    by 0x300669: init_ipct_allocator (cpu68k.c:813)
                else  {                                             //                                            // Uninitialised value was created by a heap allocation
                        static t_iib *piib;

                        PAUSE_DEBUG_MESSAGES();
                        if (!(piib = cpu68k_iibtable[fetchword(reg68k_pc)]))
                            ALERT_LOG(1,"Invalid instruction @ %08lX\n", (long)reg68k_pc); // RA
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
                              if (ipc->function) ipc->function(ipc);
                              else {   EXITR(277,0,"No ipc function at %ld/%08lx, even after attempting to get one!\n",(long)context,(long)pc24);}
                            }
                      }
                  
                    #if defined(DEBUG) && defined(CPU_CORE_TESTER)
                      corecpu_complete_opcode(context);
                    #endif

                    #if defined(DEBUG) && defined(XXXDEBUG)
                      reg68k_ext_exec_various_dug();
                    #endif

                    pc24 = reg68k_pc; //20060321// & 0x00ffffff;
                    abort_opcode=0;
                    cpu68k_clocks+=ipc->clks;
              } // if execute from ram/rom else statement

    #ifdef DEBUG
      if (0==reg68k_pc) { DEBUG_LOG(0,"pc=0 LastPC24=%08lx pc24=%08lx abort_opcode:%ld",(long)lastpc24,(long)pc24,(long)abort_opcode);}
    #endif

    clks_stop=(MIN(clks_stop,cpu68k_clocks_stop));

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
    DEBUG_LOG(0,"VECTOR: %ld, OLDPC:%08lx memerr@:%08lx",(long)vector,(long)oldpc,(long)addr_error);
    if  (insetjmpland)
        {
          reg68k_internal_vector(vector,oldpc,addr_error);
          return;
        }

        DEBUG_LOG(0,"In setjmp land vector:%ld oldpc:%08lx regs.pc:%08lx regs.sr:%lx",(long)vector,(long)oldpc,(long)regs.pc,(long)regs.sr.sr_int);

//20190601        reg68k_pc = regs.pc;
//20190601        reg68k_regs = regs.regs;
//20190601        reg68k_sr.sr_int = regs.sr.sr_int;
////20190601      insetjmpland=1;

        DEBUG_LOG(0,"Calling internal_vector from external reg68k_pc:%08lx reg68k_sr:%lx",(long)reg68k_pc,(long)reg68k_sr.sr_int);
        reg68k_internal_vector(vector,oldpc,addr_error);
        DEBUG_LOG(0,"Done with internal_vector vector:%ld reg68k_pc:%08lx reg68k_sr:%08lx regs.pc:%08lx regs.sr:%08lx, exiting setjmp land",
                  (long)vector,
                  (long)reg68k_pc,
                  (long)reg68k_sr.sr_int,(long)regs.pc,(long)regs.sr.sr_int);

    /* restore global registers back to permanent storage */
        regs.pc = reg68k_pc;
        regs.sr.sr_int = reg68k_sr.sr_int;

        //insetjmpland=1; //20190601
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

void reg68k_internal_vector(int vno, uint32 oldpc, uint32 addr_error)
{
    static uint32 lastoldpc;
    static int32  lastclk;
    static int lastvno;
    uint16 saved_sr=reg68k_sr.sr_int;

    int avno=(vno-V_AUTO+1);
    int curlevel = (reg68k_sr.sr_int >> 8) & 7;
    //unused//static uint32 tmpaddr;
    uint16 busfunctioncode;
    uint8 old_supervisor=reg68k_sr.sr_struct.s;

    if (vno==V_LINE15)
    { 
      uint16 opcode,fn;

      if (romless && (reg68k_pc & 0x00ff0000)==0x00fe0000)
      {
        if (romless_entry()) return;
      }

      abort_opcode=2; opcode=fetchword(reg68k_pc);          //if (abort_opcode==1) fn=0xffff;
      abort_opcode=2; fn=fetchword(reg68k_pc+2);            //if (abort_opcode==1) fn=0xffff;
      abort_opcode=0;

      if (opcode == 0xfeed)
      {
        ALERT_LOG(0,"F-Line-Wormhole %04lx",(long)fn);
        switch(fn)
        {
          #ifdef DEBUG

            case 0:   if (debug_log_enabled) { ALERT_LOG(0,"->debug_off");                              debug_off(); reg68k_pc+=4; return; }
                      else                   { ALERT_LOG(0,"->debug_off, was already off!!!");          debug_off(); reg68k_pc+=4; return; }

            case 1:   if (debug_log_enabled) { ALERT_LOG(0,"->tracelog, was already on! turning off!"); debug_off(); reg68k_pc+=4; return; }
                      else                   { ALERT_LOG(0,"->tracelog on");            debug_on("F-line-wormhole"); reg68k_pc+=4; return; } 
          #else

            case 0:
            case 1:   reg68k_pc+=4; 
                      ALERT_LOG(0,"tracelog -> but emulator wasn't compiled in with debuggin enabled!"); 
                      return;
          #endif
          default: ALERT_LOG(0,"Unknown F-Line-wormhole:%04x or actual F-Line trap, falling through a LINE15 exception will occur.",fn);
          // anything unimplemented/undefined should fall through and generate an exception.
        }
      }
      if (opcode==0xfeef) {
                              ALERT_LOG(0,"Executing blank IPC @ %08lx",(long)reg68k_pc);
                              EXIT(99,0,"EEEK! Trying to execute blank IPC at %08lx - something is horribly wrong!",(long)reg68k_pc);
                          }
      ALERT_LOG(0,"Unhandled F-Line %04lx at %08lx",(long)opcode,(long)reg68k_pc);
    }

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

   // #ifdef DEBUG
   //     validate_mmu_segments("reg68k internal_vector");
   //  #endif

   // avoid bus/addr error repeats on multi-operand opcodes...
    if  (lastclk==cpu68k_clocks && lastvno==vno)
        {
          DEBUG_LOG(0,"Suppressing internal_vector - VECTOR:%ld, oldpc:%08lx clk:%016llx",(long)vno,(long)oldpc,(long long)cpu68k_clocks);
          return;
        }

    lastclk =cpu68k_clocks;   lastvno =vno;   lastoldpc =oldpc;

    #ifdef DEBUG

      DEBUG_LOG(0,"Entering: internal_vector - VECTOR:%ld (%s), addr_err:%08lx oldpc:%08lx, pc24:%08lx, reg68k_pc:%08lx",
                  (long)vno,getvector(vno), (long)addr_error,(long)oldpc,(long)pc24,(long)reg68k_pc);

      if (oldpc!=reg68k_pc && vno<32)
      ALERT_LOG(0,"DANGER DANGER DANGER oldpc:%08lx is not reg68k_pc:%08lx, pc24:%08lx oldpc24:%08lx vector:%ld",(long)oldpc, (long)reg68k_pc,(long)pc24,(long)lastpc24,(long)vno);

      if (vno==37) lisaos_trap5();        // log LisaOS trap #5 calls by name
      printregs(buglog,"irqdone");

    #endif

   if (avno>0 && avno<8)                // If it's an autovector, check the IRQ mask before allowing it to occur.
      {
        DEBUG_LOG(0,"PC:%08lx AutoVector is:%ld vector:%ld curlevel is %ld cpu stop status is:%ld",(long)reg68k_pc,(long)avno,(long)vno,(long)curlevel,(long)regs.stop);
        if (! (IS_VECTOR_AVAILABLE_INT(avno))) {DEBUG_LOG(0,"Vector requested is not available (INTMASK too low)");return;}
      }
   else avno=0;                         // clear avno since it wasn't an autovector - this saves time later on the avno check.


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
        DEBUG_LOG(0,"exiting from STOP opcode @%08lx - will continue at %08lx after ISR handles IRQ:%ld.",(long)reg68k_pc,(long)reg68k_pc+4,(long)vno);
        oldpc+=4;
        regs.stop = 0;
        reg68k_pc=oldpc;
        pc24=oldpc;
        regs.pc = oldpc; regs.sr = reg68k_sr;
    }

    if  (vno==2 || vno==3)
        {
            if        ((InstructionRegister & 0xff00)==0x4a00) {oldpc+=2;}
            else if   ((InstructionRegister & 0xff00)==0x4e00)
            {
              if      ((InstructionRegister & 0x00f0)==0x00d0) {oldpc+=2;}
              else if ((InstructionRegister & 0x00ff)==0x0075) {oldpc+=2; regs.sp+=4;}
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

        DEBUG_LOG(0,"PUSH PC %08lx context:%ld",(long)oldpc,(long)context);  
        A7PUSH_LONG(oldpc); //   ? (oldpc+(cpu68k_iibtable[InstructionRegister]->wordlen<<1)) : oldpc);
        if (abort_opcode==1) {EXIT(783,0,"Doh! got abort_opcode=1 on push pc %s!\n",__FUNCTION__); }
        DEBUG_LOG(0,"PUSH SR %04x context:%d",saved_sr,context);  
        A7PUSH_WORD(saved_sr); 
        if (abort_opcode==1) {EXIT(784,0,"Doh! got abort_opcode=1 on push sr in %s!\n",__FUNCTION__); }
    // "Short format 0 only four words are to be removed from the top of the stack. SR and PC are loaded from the stack frame."
    abort_opcode=0;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (vno==2 || vno==3)               // PUSH IS IN THIS ORDER: ---> PC,SR,IR,ADDR,BUSFN
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

void lisa_buserror(uint32 addr_error)
{
    reg68k_internal_vector(2 ,reg68k_pc, addr_error);
    abort_opcode=1;
}

#define LINE(x) DEBUG_LOG(0,"%s:%s:%d %s LINE LOG"       ,__FILE__,__FUNCTION__,__LINE__,x); fflush(buglog);

void lisa_mmu_exception(uint32 addr_error)
{
    char temp[1024];

    //20060105 memerror=(uint16)(( CHK_MMU_TRANS(addr_error)+(maxlisaram!=RAM1024K ? 0:RAM512K) )>>5);
    memerror=(uint16)(( CHK_MMU_TRANS(addr_error) )>>5);
/*
    ALERT_LOG(0,"MMU BUSERROR: IR:%04x @ %d/@%08x :: buserr@addr:%08x -> phys-addr:%08x SOR:%04x SLR:%04x %s",
          InstructionRegister,context,reg68k_pc,
          addr_error,
          CHK_MMU_REGST(addr_error),
          mmu[(addr_error & MMUSEGFILT)>>17].sor,
          mmu[(addr_error & MMUSEGFILT)>>17].slr,
          printslr(temp,1024, mmu[(addr_error & MMUSEGFILT)>>17].slr)
          );
*/
    reg68k_internal_vector(2,reg68k_pc,addr_error);
    abort_opcode=1;
}


void lisa_addrerror(uint32 addr_error)
{
    ALERT_LOG(0,"Odd Address Exception @%08lx PC=%08lx clk:%016llx ",(long)addr_error,(long)reg68k_pc,(long long)cpu68k_clocks);
    DEBUG_LOG(0,"ADDRESS EXCEPTION @%08lx PC=%08lx",(long)addr_error,(long)reg68k_pc);
    reg68k_internal_vector(3,reg68k_pc,addr_error);
    abort_opcode=1;
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
