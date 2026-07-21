/* These registers must be registers that are preserved over function calls
   in C.  What I mean by this is that if we're using these registers and we
   call a C function, then when that C function returns these registers are
   still what they were before we made the call. */

#ifdef __CYGWIN__
extern uint32 reg68k_pc;    // added by Ray Arachelian for LisaEm
extern uint32 *reg68k_regs; // cygwin crashes on access to reg68k_sr if it's a register via %esi
                            // and on these when returning from reg68k_external_execute.  Cygwin must
                            // use them as stack frame pointers or something.
                            //       register uint32 reg68k_pc asm ("%ebx");
                            //       register uint32 *reg68k_regs asm ("%edi");
extern t_sr reg68k_sr;
#else
#ifdef PROCESSOR_ARM
register uint32 reg68k_pc asm("r7");
register uint32 *reg68k_regs asm("r8");
register t_sr reg68k_sr asm("r9");
#else
#ifdef PROCESSOR_SPARC
register uint32 reg68k_pc asm("5");
register uint32 *reg68k_regs asm("6");
register t_sr reg68k_sr asm("7");
#else
// #    ifdef PROCESSOR_INTEL                               // commented out 20051125 - GCC 4.0 throws "warning: register used for two global register variables"
//          register uint32 reg68k_pc    asm ("%ebx");
//          register uint32 *reg68k_regs asm ("%edi");
//          register t_sr reg68k_sr      asm ("%esi");
// #    else
extern uint32 reg68k_pc;
extern uint32 *reg68k_regs;
extern t_sr reg68k_sr;
// #    endif
#endif
#endif
#endif // extra endif added by RA
