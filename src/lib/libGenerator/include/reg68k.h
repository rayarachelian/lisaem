/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* reg68k.h                                                                  */
/*                                                                           */
/*****************************************************************************/

unsigned int reg68k_external_step(void);
// int32 reg68k_external_execute(void); //changed by RA for LisaEm - no longer using clocks param, now using irq.c timers.
int32 reg68k_external_execute(int32 clocks); // changed by RA for LisaEm
void reg68k_external_autovector(int avno);

void reg68k_internal_autovector(int avno);
void reg68k_internal_vector(int vno, uint32 oldpc, uint32 addr_error); // change by RA for LisaEm to add bus/addr error location
