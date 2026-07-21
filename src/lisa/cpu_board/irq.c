/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2021 Ray A. Arachelian                          *
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
*                       Interrupt and Timing Handlers                                  *
*                                                                                      *
\**************************************************************************************/

// Rewrite me to use FIFOs, or queue's or something!  I'm lame and slow!
#define IN_IRQ_C 1
#include <vars.h>

static FLIFLO_QUEUE_t IRQq;

static int myirq = 0;

static int vcount = 0; // video irq phase

void init_IRQ(void)
{
    int s;
    DEBUG_LOG(0, "initializing IRQ queue.  IRQq:%p  MAXIRQQUEUE:%d\n", &IRQq, MAXIRQQUEUE);
    s = fliflo_buff_create(&IRQq, MAXIRQQUEUE); // valgrind reports leak here 65 bytes, but not critical
    DEBUG_LOG(0, "IRQq:%p after init status:%d\n\n", &IRQq, s);
}

uint8 IRQRingGet(void)
{
#define DRIVE (2) // drive      5               // 00=lower, 80=upper

    uint16 irql = 0;
    // unused//    uint16 nextirql;

    // new code to fix bugs
    // if there's no queued IRQ, check COPS, maybe it has data.
    if (myirq)
        irql = myirq;
    // else
    // if ((via[1].via[IER] & VIA_IRQ_BIT_CA1) &&  (copsqueuelen>0 ))
    //{
    //    via[1].via[IFR] |= VIA_IRQ_BIT_CA1; irql=2;
    //    DEBUG_LOG(0,"queuing COPS irq: copsqlen:%d  items, %d mouse items. mouse is:%d\n",copsqueuelen,mousequeuelen,cops_mouse);
    // }

    myirq = 0;

    if (!irql)
        return 0;

    DEBUG_LOG(0, "irq.c:: about to signal IRQ:%d fliflo has %d items queued pc24=%08x\n\n", irql, fliflo_buff_size(&IRQq), pc24);
    if (irql < 7) // Is this a normal autovctor.
    {
        DEBUG_LOG(0, "IRQ Get: autovector %d: fliflo has %d items queued\n", irql, fliflo_buff_size(&IRQq));

        // bandaid
        if (irql == 2 && !copsqueuelen && !mouse_pending_x && !mouse_pending_y)
            return 0;

        if (irql)
        {
            DEBUG_LOG(0, "irc.c:: firing IRQ:%d", irql);
            reg68k_external_autovector(irql);
        }
    }
    else // no, then we need special handling (FDIR)
    {
        if ((floppy_irq_top && floppy_ram[DRIVE] == 0x80) || (floppy_irq_bottom && floppy_ram[DRIVE] == 00))
        {
            // FDIR may have been suppressed by 68k code clearing the IRQ, so don't fire if FDIR=0!
            if (floppy_FDIR && (floppy_ram[0x2c] & floppy_ram[DRIVE]))
            {
                DEBUG_LOG(0, "firing floppy_FDIR:%d ram[2c]=%2x, [drive]=%02x\n", floppy_FDIR, floppy_ram[0x2c],
                          floppy_ram[DRIVE]);
                reg68k_external_autovector(IRQ_FLOPPY);
                DEBUG_LOG(0, "irq.c:: fired IRQ floppy\n");
            }
        }
    }
    return irql;
}

int8 IRQRingBufferAdd(uint8 irql, uint32 address)
{
    UNUSED(address);
    DEBUG_LOG(0, "About to set next IRQ level to:%d address was:%08x\n", irql);
    if (myirq < irql)
        myirq = irql;
    return 0;

    if (fliflo_buff_is_full(&IRQq))
    {
        DEBUG_LOG(0, "Too many IRQ's have occured! Either Lisa is ignoring IRQ's, or is crashed!!!\n\n");
        return -1;
    }

    DEBUG_LOG(0, "irq.c:: about to add IRQ:%d fliflo has %d items queued\n", irql, fliflo_buff_size(&IRQq));

    fliflo_dump(buglog, &IRQq, "irq pre-add");
    fliflo_buff_add(&IRQq, irql);
    DEBUG_LOG(0, "irq.c:: added IRQ:%d to fliflo which now has %d items queued\n", irql, fliflo_buff_size(&IRQq));
    fliflo_dump(buglog, &IRQq, "irq post-add");
    return 0;
}

// note these are identical to the ones in via6522 - they're static inline
// for speed, but remember to copy them here.  Could turn them into macros, but...
// pass it the via[?].t?_e, get back the timer value (used to read t1/t2)
inline static XTIMER get_via_timer_left_from_te(XTIMER t_e)
{
    XTIMER diff = CPUCLK_TO_VIACLK(t_e - cpu68k_clocks);

    DEBUG_LOG(0, "via_clock_diff:%016llx", via_clock_diff);
    if (diff > 0)
        return diff;
    else
        return 0;
}

// pass it the value that was set into T1 or T2, get back the CPU clock at
// which the VIA will expire to set into via[?].t?_e
//
// 5Mhz / 10 = 500Khz    every 10 cpu cycles
// 5Mhz /4   = 1.25Mhz   every  4 cpu cycles

inline static XTIMER get_via_te_from_timer(uint16 timer)
{

    if (timer < 1)
        return -1;

#ifndef USE64BITTIMER
    if ((cpu68k_clocks + VIACLK_TO_CPUCLK(timer)) > HALF_CLK)
        prevent_clk_overflow_now(); // irq.c:142: warning: integer constant is too large for 'long' type
#endif

    return cpu68k_clocks + VIACLK_TO_CPUCLK(timer);
}

/*------------------------------------------------------------------------------------
 * note that this fn is a bit lossy, we lose the LSbit, but it shouldn't be too bad.
 *
 * -----------------------------------------------------------------------------------*/

#ifndef USING64BITTIMER

void prevent_clk_overflow_now(void)
{
    int i, flag = 0;
    // if any of these timers are over the HALF_CLK mark, adjust them all to prevent roll over

    for (i = 0; i < 9 && !flag; i++)
    {
        if (via[i].t1_e > HALF_CLK || via[i].t2_e > HALF_CLK)
            flag = 1;
        if (via[i].ProFile)
        {
            if (via[i].ProFile->clock_e > HALF_CLK)
                flag = 1;
        }
    }

    if ((virq_start > HALF_CLK) || (cpu68k_clocks_stop > HALF_CLK) ||
        (cpu68k_clocks > HALF_CLK) || (cops_event > HALF_CLK) ||
        (tenth_sec_cycles > HALF_CLK) || (fdir_timer > HALF_CLK) ||
        (z8530_event > HALF_CLK) || flag)
    {
#ifdef PARANOID

        if (virq_start != -1)
            CLKDIV2(virq_start); //>>=1;
        if (cpu68k_clocks_stop != -1)
            CLKDIV2(cpu68k_clocks_stop); //>>=1;
        if (cpu68k_clocks != -1)
            CLKDIV2(cpu68k_clocks); //>>=1;
        if (cops_event != -1)
            CLKDIV2(cops_event); //>>=1;
        if (tenth_sec_cycles != -1)
            CLKDIV2(tenth_sec_cycles); //>>=1;
        if (fdir_timer != -1)
            CLKDIV2(fdir_timer); //>>=1;
        if (z8530_event != -1)
            CLKDIV2(z8530_event); //>>=1;
        if (clktest != -1)
            CLKDIV2(clktest);
        if (lastrefresh != -1)
            CLKDIV2(lastrefresh);

#else

        CLKDIV2(virq_start);         //>>=1;
        CLKDIV2(cpu68k_clocks_stop); //>>=1;
        CLKDIV2(cpu68k_clocks);      //>>=1;
        CLKDIV2(cops_event);         //>>=1;
        CLKDIV2(tenth_sec_cycles);   //>>=1;
        CLKDIV2(fdir_timer);         //>>=1;
        CLKDIV2(z8530_event);        //>>=1;
        CLKDIV2(clktest);
        CLKDIV2(lastrefresh);

#endif

        for (i = 0; i < 9; i++)
        {

            CLKDIV2(via[i].t1_e); //      >>=1;
            CLKDIV2(via[i].t2_e); //      >>=1;
            CLKDIV2(via[i].sr_e); //      >>=1;

            // #ifdef DEBUG
            CLKDIV2(via[i].t1_set_cpuclk);   //   >>=1;
            CLKDIV2(via[i].t2_set_cpuclk);   //   >>=1;
            CLKDIV2(via[i].t1_fired_cpuclk); //   >>=1;
            CLKDIV2(via[i].t2_fired_cpuclk); //   >>=1;

            if (via[i].ProFile)
                CLKDIV2(via[i].ProFile->clock_e); // >>=1;

// #endif
#ifdef DEBUG
            if (via[i].t1_e < -2)
            {
                EXIT(623, 0, "*** in prevent_clk_overflow: via[%d].t1_e=%ld is negative cpu68k_clks=%016llx\n", i, via[i].t1_e, cpu68k_clocks);
            }
            if (via[i].t2_e < -2)
            {
                EXIT(623, 0, "*** in prevent_clk_overflow: via[%d].t2_e=%ld is negative cpu68k_clks=%016llx\n", i, via[i].t2_e, cpu68k_clocks);
            }
#endif
        }
    }
    if (cpu68k_clocks < 0)
        DEBUG_LOG(0, "*** in prevent_clk_overflow: cpu68k_clocks<0! %016llx\n", cpu68k_clocks);
}

void prevent_clk_overflow(void)
{
    if (cpu68k_clocks > HALF_CLK)
        prevent_clk_overflow_now();
}
#endif

// statics are faster, not that it matters that much in this case, but why not.
static uint8 next_expired_timer = 0;

// But! is only needed in a single place outside of this .c file by reg68k, so provide
// a way for it to get the proper value.  Need to eliminate this dependancy.
uint8 next_timer_id(void) { return next_expired_timer; }
void set_next_timer_id(uint8 x) { next_expired_timer = x; }
uint8 get_next_timer_id(void) { return next_expired_timer; }
// Hmmm, can probably get rid of this because the cops 1/10th second timer can serve
// well enough to exit the loop.

char *gettimername(uint8 t) // can turn this into an array of strings, but it's debug code, so why bother to optimize?
{                           // remember - premature optimization is the root of all evil. :-) DEBUG shouldn't be optimized
    switch (t)
    {
    case CYCLE_TIMER_VIA1_T1_TIMER:
        return "VIA 1 T1 t1clk";
    case CYCLE_TIMER_VIA2_T1_TIMER:
        return "VIA 2 T1 t2clk";
    case CYCLE_TIMER_VIA3_T1_TIMER:
        return "VIA 3 (slot 0) T1";
    case CYCLE_TIMER_VIA4_T1_TIMER:
        return "VIA 4 (slot 0) T1";
    case CYCLE_TIMER_VIA5_T1_TIMER:
        return "VIA 5 (slot 1) T1";
    case CYCLE_TIMER_VIA6_T1_TIMER:
        return "VIA 6 (slot 1) T1";
    case CYCLE_TIMER_VIA7_T1_TIMER:
        return "VIA 7 (slot 2) T1";
    case CYCLE_TIMER_VIA8_T1_TIMER:
        return "VIA 8 (slot 2) T1";

    case CYCLE_TIMER_VERTICAL_RETRACE:
        return "vertical retrace";
    case CYCLE_TIMER_COPS_MOUSE_IRQ:
        return "COPS mouse IRQ";
    case CYCLE_TIMER_COPS_CLOCK_DSEC:
        return "COPS clock 1/10sec beat";
    case CYCLE_TIMER_FDIR:
        return "Floppy FDIR";

    case CYCLE_TIMER_VIA1_T2_TIMER:
        return "VIA 1 T2";
    case CYCLE_TIMER_VIA2_T2_TIMER:
        return "VIA 2 T2";
    case CYCLE_TIMER_VIA3_T2_TIMER:
        return "VIA 3 (slot 0) T2";
    case CYCLE_TIMER_VIA4_T2_TIMER:
        return "VIA 4 (slot 0) T2";
    case CYCLE_TIMER_VIA5_T2_TIMER:
        return "VIA 5 (slot 1) T2";
    case CYCLE_TIMER_VIA6_T2_TIMER:
        return "VIA 6 (slot 1) T2";
    case CYCLE_TIMER_VIA7_T2_TIMER:
        return "VIA 7 (slot 2) T2";
    case CYCLE_TIMER_VIA8_T2_TIMER:
        return "VIA 8 (slot 2) T2";

    case CYCLE_TIMER_VIA1_SHIFTREG:
        return "VIA 1 SHIFTREG";
    case CYCLE_TIMER_VIA2_SHIFTREG:
        return "VIA 2 SHIFTREG";
    case CYCLE_TIMER_VIA3_SHIFTREG:
        return "VIA 3 (slot 0) SHIFTREG";
    case CYCLE_TIMER_VIA4_SHIFTREG:
        return "VIA 4 (slot 0) SHIFTREG";
    case CYCLE_TIMER_VIA5_SHIFTREG:
        return "VIA 5 (slot 1) SHIFTREG";
    case CYCLE_TIMER_VIA6_SHIFTREG:
        return "VIA 6 (slot 1) SHIFTREG";
    case CYCLE_TIMER_VIA7_SHIFTREG:
        return "VIA 7 (slot 2) SHIFTREG";
    case CYCLE_TIMER_VIA8_SHIFTREG:
        return "VIA 8 (slot 2) SHIFTREG";

    case CYCLE_TIMER_SCC_B_XMT_BUF_EMPTY:
        return "Z8530 Channel B Transmit Buffer Empty";
    case CYCLE_TIMER_SCC_B_EXT_STAT_CHG:
        return "Z8530 Channel B External/Status Change";
    case CYCLE_TIMER_SCC_B_RCVD_CHAR:
        return "Z8530 Channel B Receive Character Avail";
    case CYCLE_TIMER_SCC_B_SPECIAL:
        return "Z8530 Channel B Special Receive Condition";
    case CYCLE_TIMER_SCC_A_XMT_BUF_EMPTY:
        return "Z8530 Channel A Transmit Buffer Empty";
    case CYCLE_TIMER_SCC_A_EXT_STAT_CHG:
        return "Z8530 Channel A External/Status Change";
    case CYCLE_TIMER_SCC_A_RCVD_CHAR:
        return "Z8530 Channel A Receive Character Avail";
    case CYCLE_TIMER_SCC_A_SPECIAL:
        return "Z8530 Channel A Special Receive Condition";

    case 0:
        return "null timer";
    default:
        return "***UNKNOWN***";
    }
}

void flag_via_sr_irq(int i)
{
    viatype *V = (i < 9 && i > 0) ? &via[i] : NULL;
    uint8 shift;

    if (!V)
        return;
    if (!V->active)
        return;

    shift = (V->via[ACR] & 28) >> 2; // 4,5 for T2 expiration
                                     // high bit of shift is input or output mode.
                                     // In Lisa shift under ext clock is disabled since CB1 is NC.
                                     // so, shift=0, shift=3, shift=7 - no rotations done at all
    switch (shift)
    {
    case 0:
        break; // disabled SR
    case 1:
    case 2:
    {
        V->via[SHIFTREG] = 0; // it is on parallel boards for the LCHK line, but this isn't used by
                              // motherboard parallel ports

        // shift=1 in under T2 control, in shift=2 under system clock, 3=CB1 pulses
        V->cb2 = 0; // all inputs are zero,   mode=4-shift out under t2, mode=5 under sys clock

        V->via[IFR] |= VIA_IRQ_BIT_SR | ((V->via[IER] & VIA_IRQ_BIT_SR) ? VIA_IRQ_BIT_SET_CLR_ANY : 0);
        V->srcount++;

        break;
    }
    // outputs
    case 6: // shift out at system clock rate
    case 7:
    {
        V->cb2 = (V->via[SHIFTREG] & 0x80) ? 1 : 0;
        via[2].sr_e = -1; // capture last bit shifted out
        V->via[IFR] |= VIA_IRQ_BIT_SR | ((V->via[IER] & VIA_IRQ_BIT_SR) ? VIA_IRQ_BIT_SET_CLR_ANY : 0);
        V->srcount = 0;
        V->via[SHIFTREG] = 0;
    }
    break;
    }
    return;

} /// end of shift register irq code //////////////////////////////////////////////////////////////////////////////////////////

void flag_via_t2_irq(int i)
{
    viatype *V = (i < 9 && i > 0) ? &via[i] : NULL;
    uint8 shift;
#ifdef DEBUG
    XTIMER rate = 0;
    uint16 latch = 0;
#endif

    if (!V)
        return;
    if (!V->active)
        return;

    shift = (V->via[ACR] & 28) >> 2; // 4,5 for T2 expiration
    V->t2_e = -1;
    V->via[IFR] |= VIA_IRQ_BIT_T2; //  Set the IRQ flag for the VIA
    V->t2_fired++;
#ifdef DEBUG
    latch = (V->via[T2CH] << 8) | (V->via[T2CL]);
    rate = (latch ? ((cpu68k_clocks - V->t1_set_cpuclk) / latch) : 0);
    DEBUG_LOG(0, "via#%d T2 Timer expired at clock:%016llx was initialized at clock %016llx, ran for:%016llx clock cycles latch was:%04x rate:%ld\n",
              V->vianum,
              cpu68k_clocks,
              V->t2_set_cpuclk,
              cpu68k_clocks - V->t2_set_cpuclk,
              latch,
              rate);
#endif
    V->t2_fired_cpuclk = cpu68k_clocks; // V->t1_set_cpuclk=cpu68k_clocks;

    // if (shift==7) shift==5;            // mode 7 is not supported on Lisa as CB1 is not connected.
    //  if you need to implement it copy mode 5 add if (shift==7) shift==5 above
    //  and make proper timing changes to via6522.c

    /// SR input mode 1 (under timer 2 control) /////////////////////////////////////////////////////////////////
    if (shift == 1) // don't think SR is used as input anywhere... maybe, not sure.
    {               // it is on parallel boards for the LCHK line, but this isn't used by
                    // motherboard parallel ports, only Parallel Port cards

        V->via[SHIFTREG] = (((V->via[SHIFTREG] << 1) & 0xff) | 0); // input is all 0's.

        // shift=1 in under T2 control, in shift=2 under system clock, 3=CB1 pulses
        V->cb2 = 0; // all inputs are zero,   mode=4-shift out under t2, mode=5 under sys clock

        if (V->srcount < 8)
            V->srcount++;
        if (V->srcount == 8)
        {
            V->via[IFR] |= VIA_IRQ_BIT_SR; // | (  (V->via[IER] &  VIA_IRQ_BIT_SR) ? VIA_IRQ_BIT_SET_CLR_ANY:0  );
            V->srcount = 0;                //++   // set it to 9 so we don't repeat until SR is re-accessed.
        }
    }
    /// SR output modes 4,5 (under timer2 control) //////////////////////////////////////////////////////////////
    if (shift == 5 || shift == 4) // mode 4 is free running, so reload the loop bit back into the register
    {
        uint8 loopbit = 0;
        loopbit = (shift == 4) ? ((V->via[SHIFTREG] & 0x80) ? 1 : 0) : 0;
        V->cb2 = (V->via[SHIFTREG] & 0x80) ? 1 : 0;
        V->via[SHIFTREG] = (((V->via[SHIFTREG] << 1) & 0xff) | loopbit);
        V->via[IFR] |= VIA_IRQ_BIT_SR | VIA_IRQ_BIT_SET_CLR_ANY;

        // mode5 (and 6, and 7) doesn't repeat
        if (V->srcount > 8) // are we done with a full byte
        {
            if (V->srcount < 9)                                          // signal the IRQ if set that SR is done shifting (mode=5 hits this once)
                V->via[IFR] |= VIA_IRQ_BIT_SR | VIA_IRQ_BIT_SET_CLR_ANY; // | (  (V->via[IER] &  VIA_IRQ_BIT_SR) ? VIA_IRQ_BIT_SET_CLR_ANY:0  );
            if (shift == 4)
                V->srcount = 0; // free running repeats
            if (shift == 5)
                V->srcount = 9; // lock mode 5 from repeating and prevent future IRQ until SR reload
        }
        else
            V->srcount++;
    }

    // handle timer 2 code //////////////////////////////////////////////////////////////////////////////////////
    if (V->via[IER] & VIA_IRQ_BIT_T2)
    {
        V->via[IFR] |= (VIA_IRQ_BIT_T2 | VIA_IRQ_BIT_SET_CLR_ANY); //  Set the IRQ flag for the VIA

        DEBUG_LOG(0, "T2 Timer VIA#%d IRQ:%d queued", V->vianum, V->irqnum);
        // reg68k_external_autovector(V->irqnum);   // fire interrupt  //2021.03.21 moved to irq loop

    } /////////////////////////////////// end of timer 2 ///////////////////////////////////////////////////////
}

void flag_via_t1_irq(int i)
{ // Are IRQ's enabled for T1?
    viatype *V = (i < 9 && i > 0) ? &via[i] : NULL;
    XTIMER rate = 0;
    uint16 latch = 0;

    if (!V)
        return;
    if (!V->active)
        return;

    latch = (V->via[T1LH] << 8) | (V->via[T1LL]);
    rate = (latch ? ((cpu68k_clocks - V->t1_set_cpuclk) / latch) : 0);

    V->t1_e = -1;
    V->via[IFR] |= VIA_IRQ_BIT_T1;
    if (V->via[IER] & VIA_IRQ_BIT_T1)
    {
        V->via[IFR] |= (VIA_IRQ_BIT_T1 | VIA_IRQ_BIT_SET_CLR_ANY); // any bit only set when IRQ is fired.
        DEBUG_LOG(0, "T1 Timer Via#%d IRQ:%d queued.", V->vianum, V->irqnum);
        // reg68k_external_autovector(V->irqnum);   // fire interrupt  //2021.03.21 moved to irq loop
    }
    V->t1_fired++;
    DEBUG_LOG(0, "VIA %d T1 Timer expired at clock:%016llx was initialized at clock %016llx, ran for:%016llx clock cycles latch was:%04x rate:%ld",
              V->vianum,
              cpu68k_clocks,
              V->t1_set_cpuclk,
              cpu68k_clocks - V->t1_set_cpuclk,
              latch,
              rate);

    V->t1_fired_cpuclk = cpu68k_clocks;
    // Will the timer reload?  Does PB7 need to be set?
    switch (V->via[ACR] >> 6)
    {
    case 0:
        break; // 00 Timed IRQ each time T1 loaded, one shot PB7 disabled
    case 2:
        V->via[IRB] |= 128;
        break; // 10 Timed IRQ each time T1 loaded, one shot PB7 high on expire
    case 3:
        V->via[IRB] ^= 128; // continous irq PB7 square wave output (fall through)
                            /* FALLTHROUGH */
    case 1:
        V->via[T1CL] = V->via[T1LL]; // continous irq PB7 disabled -- just reload timer
        V->via[T1CH] = V->via[T1LH];
        V->t1_e = get_via_te_from_timer((V->via[T1CH] << 8) | V->via[T1CL]);

        DEBUG_LOG(0, "VIA:%d T1 Timer went to 0, reloaded with:%04x will go off after clock=%016llx, clock is now:%016llx\n",
                  V->vianum,
                  ((V->via[T1CH] << 8) | V->via[T1CL]),
                  V->t1_e, cpu68k_clocks);

        V->t1_set_cpuclk = cpu68k_clocks;
        break;
    }

} ///////////////////////// end of timer 1

/***********************************************************************************\
*  VIA/Timer House Keeping.  This function is called for each of the via's to       *
*  process timers, shift registers, etc.                                            *
*                                                                                   *
*  It is also used to keep timing of vertical retraces, cops clock, mouse irq timers*
*  since it is the same mechanism.                                                  *
*                                                                                   *
*  This must be called whenever any event timer changes!                            *
\***********************************************************************************/
void get_next_timer_event(void)
{
    int i;
    XTIMER clx_timer;     // turn clx into a positive value so the following code
    XTIMER min_clx_timer; // is easier to read/debug.

    if (next_expired_timer == 0 || cpu68k_clocks_stop < cpu68k_clocks)
        cpu68k_clocks_stop = cpu68k_clocks + ONE_SECOND;

    clx_timer = cpu68k_clocks_stop - cpu68k_clocks; // cycles to be done this time around
    min_clx_timer = cpu68k_clocks_stop;             // ditto

// correct cpu68k_clocks as it's gone over 1/2 of a 32 bit value to prevent cpu68k_clocks overflows, but only for 32 bit timers
#ifndef USE64BITTIMER
    prevent_clk_overflow();
#endif

    for (i = 1; i < 9; i++) // search timers in all VIA's
        if (via[i].active)  // which are marked as active.
        {
            // Shift Register Timing
            if (via[i].sr_e > cpu68k_clocks && is_vector_available(via[i].irqnum))
            {
                DEBUG_LOG(0, "via#%d sr_e:       :%ld \t(diff:%016llx)", i, via[i].sr_e, via[i].sr_e - cpu68k_clocks);
                if (cpu68k_clocks_stop > via[i].sr_e) // about to fire in this frame?
                {
                    cpu68k_clocks_stop = via[i].sr_e;
                    next_expired_timer = i | 0x40;
                } // yes, mark it as such.
            }
            else
            {
                if (via[i].sr_e > -1)
                {
                    flag_via_sr_irq(i);
                }
            } // oops! it expired, but we missed it!

            if (via[i].ProFile)
                VIAProfileLoop(i, via[i].ProFile, PROLOOP_EV_NUL);

#ifdef DEBUG
            if (i < 3 || via[i].active)
            {

                char *ca1txt = ((via[i].via[PCR] & 1) ? "CA1:Positive Edge " : "CA1:Negative Edge");
                char *cb1txt = ((via[i].via[PCR] & 16) ? "CB1:Positive Edge " : "CB1:Negative Edge");

                DEBUG_LOG(0, "via#%d timer2 latch:%04x, timer1 latch:%04x IER:(%s%s%s%s%s%s%s) IFR:(%s%s%s%s%s%s%s) %s %s",
                          i,
                          (via[i].via[T2CH] << 8) | (via[i].via[T2CL]),
                          (via[i].via[T1LH] << 8) | (via[i].via[T1LL]),

                          ((via[i].via[IER] & VIA_IRQ_BIT_CA2) ? "0:CA2 " : ""),
                          ((via[i].via[IER] & VIA_IRQ_BIT_CA1) ? "1:CA1 " : ""),
                          ((via[i].via[IER] & VIA_IRQ_BIT_SR) ? "2:SR  " : ""),
                          ((via[i].via[IER] & VIA_IRQ_BIT_CB2) ? "3:CB2 " : ""),
                          ((via[i].via[IER] & VIA_IRQ_BIT_CB1) ? "4:CB1 " : ""),
                          ((via[i].via[IER] & VIA_IRQ_BIT_T2) ? "5:T2  " : ""),
                          ((via[i].via[IER] & VIA_IRQ_BIT_T1) ? "6:T1  " : ""),

                          ((via[i].via[IFR] & VIA_IRQ_BIT_CA2) ? "0:CA2 " : ""),
                          ((via[i].via[IFR] & VIA_IRQ_BIT_CA1) ? "1:CA1 " : ""),
                          ((via[i].via[IFR] & VIA_IRQ_BIT_SR) ? "2:SR  " : ""),
                          ((via[i].via[IFR] & VIA_IRQ_BIT_CB2) ? "3:CB2 " : ""),
                          ((via[i].via[IFR] & VIA_IRQ_BIT_CB1) ? "4:CB1 " : ""),
                          ((via[i].via[IFR] & VIA_IRQ_BIT_T2) ? "5:T2  " : ""),
                          ((via[i].via[IFR] & VIA_IRQ_BIT_T1) ? "6:T1  " : ""),
                          ca1txt, cb1txt);
            }
#endif

            ////////////////////// timer 2/////////////////////////////////////////////////////////////
            if (via[i].t2_e != (XTIMER)0xffffffffffffffff)
            {
                if (via[i].t2_e > cpu68k_clocks && is_vector_available(via[i].irqnum))
                {
                    DEBUG_LOG(0, "via#%d t2_e:       :%016llx \t(diff:%016llx)", i, via[i].t2_e, via[i].t2_e - cpu68k_clocks);
                    if (cpu68k_clocks_stop > via[i].t2_e) // same as above, only for timer 2
                    {
                        cpu68k_clocks_stop = via[i].t2_e;
                        next_expired_timer = i | 0x80;
                        // flag_via_t2_irq(i);  //2005.06.08 9pm - hasn't happened yet, don't flag it.
                    }
                }
                // was missing 2004.12.07
                else
                {
                    if (via[i].t2_e > -1)
                    {
                        flag_via_t2_irq(i);
                    }
                } // oops! it expired, but we missed it!
            }

            ////////////////////// timer 1 //////////////////////////////////////////////////////////////
            if ((via[i].via[T1LH] | via[i].via[T1LL]) && (via[i].t1_e != (XTIMER)0xffffffffffffffff)) // 2020.11.06
            {
                if (via[i].t1_e > cpu68k_clocks && is_vector_available(via[i].irqnum))
                {
                    DEBUG_LOG(0, "via#%d t1_e:       :%16llx \t(diff:%016llx)", i, via[i].t1_e, via[i].t1_e - cpu68k_clocks);
                    if (cpu68k_clocks_stop > via[i].t1_e) // about to fire in this frame?
                    {
                        cpu68k_clocks_stop = via[i].t1_e;
                        next_expired_timer = i;
                    } // yes, mark it as such.
                }
                else
                    flag_via_t1_irq(i); // cpu68k_clocks_stop=cpu68k_clocks+175; next_expired_timer=i;} }      // oops! it expired, but we missed it!
            }

            if (!!(via[i].via[IER] & via[i].via[IFR]))
                reg68k_external_autovector(via[i].irqnum); // 2021.03.21 fire interrupt if IFR set to enabled bits
        }

    // non-VIA timers - if they're due in this cycle, then see if they're smaller than the current min
    if (cpu68k_clocks_stop > virq_start && virq_start > -1)
    {
        cpu68k_clocks_stop = virq_start;
        next_expired_timer = CYCLE_TIMER_VERTICAL_RETRACE;
    }
    if (cpu68k_clocks_stop > cops_event && cops_event > -1)
    {
        cpu68k_clocks_stop = cops_event;
        next_expired_timer = CYCLE_TIMER_COPS_MOUSE_IRQ;
    }
    if (cpu68k_clocks_stop > tenth_sec_cycles && tenth_sec_cycles > -1)
    {
        cpu68k_clocks_stop = tenth_sec_cycles;
        next_expired_timer = CYCLE_TIMER_COPS_CLOCK_DSEC;
    }
    if (cpu68k_clocks_stop > fdir_timer && fdir_timer > -1)
    {
        cpu68k_clocks_stop = fdir_timer;
        next_expired_timer = CYCLE_TIMER_FDIR;
    }

    // count zero interrupt
    if (cpu68k_clocks_stop > z8530_event && z8530_event > -1)
    {
        cpu68k_clocks_stop = z8530_event;
        next_expired_timer = CYCLE_TIMER_Z8530;
    }

    // OOps! A timer expired, but we missed it! Do it very soon if it's enabled, if there's no timer, then recalculate this again
    // by setting clocks_stop very far in the future, which should cause the tenth_sec_cycles to kick in - if it's not, we just
    // get a null timer which kicks in again needlessly after 164 cycles.  We're not trying for recusrsion here, this shouldn't
    // be called more than two deep.
    ////    if (cpu68k_clocks_stop<cpu68k_clocks)
    ////      {
    ////        if (next_expired_timer || tenth_sec_cycles==-1)       cpu68k_clocks_stop=cpu68k_clocks+164;
    ////        else {cpu68k_clocks_stop=cpu68k_clocks+655360;get_next_timer_event(); return;}
    ////      }

#ifdef DEBUG
    if (cpu68k_clocks < 0 || cpu68k_clocks_stop < 0)
    {
        EXIT(989, 0, "cpu68k_clocks_stop<clocks or negative! (%016llx<%016llx)",
             cpu68k_clocks_stop, cpu68k_clocks);
    }

    DEBUG_LOG(0, "cpu68k_clocks     :%016llx", cpu68k_clocks);
    DEBUG_LOG(0, "cpu68k_clocks_stop:%016llx \t(diff:%016llx)", cpu68k_clocks_stop, cpu68k_clocks_stop - cpu68k_clocks);
    if (virq_start != -1)
        DEBUG_LOG(0, "virq_start        :%016llx \t(diff:%016llx)", virq_start, virq_start - cpu68k_clocks);
    if (cops_event != -1)
        DEBUG_LOG(0, "cops_event        :%016llx \t(diff:%016llx)", cops_event, cops_event - cpu68k_clocks);
    if (tenth_sec_cycles != -1)
        DEBUG_LOG(0, "tenth_sec_cycles  :%016llx \t(diff:%016llx)", tenth_sec_cycles, tenth_sec_cycles - cpu68k_clocks);
    if (fdir_timer != -1)
        DEBUG_LOG(0, "FDIR TIMER        :%016llx \t(diff:%016llx)", fdir_timer, fdir_timer - cpu68k_clocks);
    if (z8530_event != -1)
        DEBUG_LOG(0, "Z8530 Count Zero  :%016llx \t(diff:%016llx)", z8530_event, z8530_event - cpu68k_clocks);
    DEBUG_LOG(0, "Next timer        :%d %s", next_expired_timer, gettimername(next_expired_timer));
    DEBUG_LOG(0, "-------------------------------------------");
#endif
}

extern void LisaScreenRefresh(void);

#define STAMP(x) \
    {            \
    }
//{DEBUG_LOG(0,"%s:%s:%d::%s\n",__FILE__,__FUNCTION__,__LINE__,x);}

//  If this is called, it's from reg68k_INTERNAL_execute because the via's timer has fired

// static int screenrefreshcount=0;

// 2021.02.23 UniPlus gets stuck in a loop like so, checking and resetting the VTIR timing
// 1/0001bd18 (0 0/0/0) : 13fc 0001 00fc e01a        : ........ :  318 : MOVE.B     #$01,$00fce01a  SRC:clk:00000000105f56d5 +20 clks
// 1/0001bd20 (0 0/0/0) : 0839 0002 00fc f801        : .9...... :  106 : BTST.B     #$02,$00fcf801  SRC:clk:00000000105f56e9 +20 clks
// 1/0001bd28 (0 0/0/0) : 66ee                       : f.       : 1053 : BNE.B      $0001bd18  SRC:clk:00000000105f56fd +8 clks
// so added a limit on how often you can reset the VTIR
static XTIMER lastvideotimimgreset = 0;
void reset_video_timing(void)
{
    // if you keep mashing on enable VTIR in a loop, this will only reset the circuitry once
    if ((cpu68k_clocks - lastvideotimimgreset) < 60)
    {
        lastvideotimimgreset = cpu68k_clocks;
        return;
    }
    lastvideotimimgreset = cpu68k_clocks;

    get_next_timer_event();
    video_scan = cpu68k_clocks; // keep track of where we are
    virq_start = cpu68k_clocks + FULL_FRAME_CYCLES;
    get_next_timer_event();
}

void decisecond_clk_tick(void)
{
    int years, days, hours, minutes, seconds, tenths;

    // get current time into human readable variables
    years = lisa_clock.year & 0x0f;
    days = (((lisa_clock.days_h & 0xf0) >> 4) * 100 + (lisa_clock.days_h & 0x0f) * 10 + lisa_clock.days_l);
    hours = lisa_clock.hours_h * 10 + lisa_clock.hours_l;
    minutes = lisa_clock.mins_h * 10 + lisa_clock.mins_l;
    seconds = lisa_clock.secs_h * 10 + lisa_clock.secs_l;
    tenths = lisa_clock.tenths;

    // tick occured, update clock
    tenths++;

    // correct time as needed
    while (tenths > 9)
    {
        tenths -= 10;
        seconds++;
        if (lisa_alarm > 0) // alarm decrements every second if turned on.
            if (0 == (--lisa_alarm))
            {
                cops_timer_alarm();
                DEBUG_LOG(0, "copsclk: ALARM fired.");
            }
    }
    while (seconds > 59)
    {
        seconds -= 60;
        minutes++;
    }
    while (minutes > 59)
    {
        minutes -= 60;
        hours++;
    }
    while (hours > 23)
    {
        hours -= 24;
        days++;
    }
    while (days > 365)
    {
        days -= 365;
        years++;
    }

    // save human readable time back to Lisa format
    lisa_clock.year = (0x0f & years) | 0xe0;
    if ((lisa_clock.year & 0x0f) < 3)
        lisa_clock.year = 0xe0 | 0x07; // prevent warning about clock by going to 1983.
    lisa_clock.days_l = days % 10;
    days /= 10;
    lisa_clock.days_h = days % 10;
    days /= 10;
    lisa_clock.days_h = ((lisa_clock.days_h & 0x0f) | (days << 4));

    lisa_clock.hours_h = hours / 10;
    lisa_clock.hours_l = hours % 10;
    lisa_clock.mins_h = minutes / 10;
    lisa_clock.mins_l = minutes % 10;
    lisa_clock.secs_h = seconds / 10;
    lisa_clock.secs_l = seconds % 10;
    lisa_clock.tenths = tenths;
}

void check_current_timer_irq(void)
{
    uint8 i = next_expired_timer & 0x1f;   // get the timer #
    viatype *V = (i < 9) ? &via[i] : NULL; // get VIA # if it's a via

    seek_mouse_event();

    // ALERT_LOG(0,"Entering check_current_timer_irq. cpu68k_clocks:%016llx timer that expired:%d %s",
    //        cpu68k_clocks,next_expired_timer,gettimername(next_expired_timer));

    // printregs(buglog,"");

    // DEBUG_LOG(0,"Timer #%d expired.  Clocks at :%ld\n",next_expired_timer,cpu68k_clocks);
    // DEBUG_LOG(0,"vertical:%ld(%s)\n cops_event:%ld(%s)\n tenth:%ld(%s)\npc24=%08x\n",
    //           virq_start,         (virq_start      >cpu68k_clocks ? "Good":"*Lost*"),
    //           cops_event,         (cops_event      >cpu68k_clocks ? "Good":"*Lost*"),
    //           tenth_sec_cycles,   (tenth_sec_cycles>cpu68k_clocks ? "Good":"*Lost*"),pc24 );

    // null cycle timer - just allow host ui to get an event, etc.
    if (!next_expired_timer)
    { // STAMP("timer=0");
        DEBUG_LOG(0, "Waaa! Could not find a timer! cpu68k_clocks:%016llx before get_next_timer_event", cpu68k_clocks);
        get_next_timer_event();
        DEBUG_LOG(0, "Woot! cpu68k_clocks:%016llx after get_next_timer_event.  Got %d %s as the next timer.", cpu68k_clocks,
                  next_expired_timer, gettimername(next_expired_timer));

        if (cpu68k_clocks < cpu68k_clocks_stop)
            return;

        if (!next_expired_timer)
        {
            DEBUG_LOG(0, "Got 0, bye.");
            return;
        }
    }

    if (next_expired_timer >= CYCLE_TIMER_SCC_B_XMT_BUF_EMPTY && next_expired_timer <= CYCLE_TIMER_SCC_A_SPECIAL)
    {
        switch (next_expired_timer)
        {
        case CYCLE_TIMER_SCC_B_XMT_BUF_EMPTY:
            break;
        case CYCLE_TIMER_SCC_B_EXT_STAT_CHG:
            break;
        case CYCLE_TIMER_SCC_B_RCVD_CHAR:
            break;
        case CYCLE_TIMER_SCC_B_SPECIAL:
            break;
        case CYCLE_TIMER_SCC_A_XMT_BUF_EMPTY:
            break;
        case CYCLE_TIMER_SCC_A_EXT_STAT_CHG:
            break;
        case CYCLE_TIMER_SCC_A_RCVD_CHAR:
            break;
        case CYCLE_TIMER_SCC_A_SPECIAL:
            break;
        }
    }

    if ((next_expired_timer & 0x7f) == CYCLE_TIMER_VERTICAL_RETRACE) // non-via timer such as vertical irq
    {
        /* This was wrong.  We have 0=entering retrace, 1= vertical off, but still in retrace, 2=display
           it's 15 lines each for 0,1, and 364 for the rest.
            1 video bit = 5ns or 20Mhz.  1 line is 720 bits + h_sync of 11 words.
            1 cpu cycle=4 bits.  720 bits = 180 cpu cycles + 11 (16 bit words) + 44 cycles H_SYNC=224 cycles/line
            364 lines=81536 cpu cycles for the main display
            15 lines 1/2 vsync = 3360 cpu cycles.
        */

        vcount++;
        if (vcount > 2)
            vcount = 0; // 0=normal display, 1=vertical retrace with signal on, 2=vertical retrace, signal off

        // exiting vertical retrace, entering start of video frame.
        DEBUG_LOG(0, "Vertical Retrace Phase:%d, VTIR is %d:", vcount, videoirq);
        switch (vcount)
        {
        case 0:
        {
            DEBUG_LOG(0, "vertical retrace phase 0");

            //                if ( (screenrefreshcount=(screenrefreshcount+1) & 0x01)==0 ) LisaScreenRefresh();  // entering refresh cycle, so update screen with changes

            next_expired_timer = 0;
            get_next_timer_event();
            video_scan = cpu68k_clocks; // keep track of where we are
            virq_start = cpu68k_clocks + FULL_FRAME_CYCLES;

            return;
        }

        // entering vertical retrace, signal IRQ if enabled
        case 1:
        {
            DEBUG_LOG(0, "vertical retrace phase 1, VIDEOIRQ PHASE/ENABLED is:%d", videoirq);
            virq_start = cpu68k_clocks + VERT_RETRACE_ON;
            vertical = 1;
            verticallatch = 1;

            if (videoirq & 1) // Interrupt if turned on.
            {                 // STAMP("autovector: firing video IRQ\n");
                DEBUG_LOG(0, "Firing IRQ1 for vertical retrace");
                reg68k_external_autovector(IRQ_VIDEO); // this was wrong - should be EXTERNAL!
            }
            else
                DEBUG_LOG(0, "Not firing IRQ1 for vertical retrace because it's off (videoirq=%d)", videoirq);

            next_expired_timer = 0;
            get_next_timer_event(); // setup next cycle
            return;
        }
        // exiting vertical retrace, shut vertical retrace signal off
        case 2:
        {
            DEBUG_LOG(0, "Vertical retrace phase 2");
            virq_start = cpu68k_clocks + VERT_RETRACE_CYCLES;
            vertical = 0; // not sure if vertical=0 s/b here!

            next_expired_timer = 0;
            get_next_timer_event(); // setup next cycle
            return;
        }
        }
        // this should be unreachable code.
        next_expired_timer = 0;
        get_next_timer_event(); // setup next cycle
        return;
    }

    // COPS mouse timer
    if ((next_expired_timer & 0x7f) == CYCLE_TIMER_COPS_MOUSE_IRQ)
    { // need to be careful with the next line as it calls reg68k_internal vector while *OUTSIDE*!!!!
        DEBUG_LOG(0, " CYCLE_TIMER_COPS_MOUSE_IRQ");
        SET_COPS_NEXT_EVENT(0);
        // DEBUG_LOG(0,"COPS Timer Entered:: cpu68k_clocks:%ld, cops_event:%ld cops_mouse:%d\n",cpu68k_clocks,cops_event,cops_mouse);

        // Are COPS IRQ's enabled?  If so, either mouse timer IRQ's enabled? or was there a keystroke?
        // then, fire the IRQ.
        if ((cops_event > 0 || copsqueuelen)) // 20060609   was (via[1].via[IER] & VIA_IRQ_BIT_CA1)    &&
        {
            DEBUG_LOG(0, "Setting IFR because either of these happened:");
            DEBUG_LOG(0, "via[1].via[IER] & CA1 bit:%d", (via[1].via[IER] & VIA_IRQ_BIT_CA1));
            DEBUG_LOG(0, "(cops_event>0 is:%016llx   || copsqueuelen is %d)", cops_event, copsqueuelen);

            via[1].via[IFR] |= VIA_IRQ_BIT_CA1;

            //  DEBUG_LOG(0,"queuing COPS [mouse timed irq:] copsqlen:%d  items, %d mouse items. mouse is:%d\n",
            //              copsqueuelen,mousequeuelen,cops_mouse);

            // 20060118-disabled this as it's handled via get_pending_vector! //  reg68k_external_autovector(IRQ_COPS);
        }
        else
            DEBUG_LOG(0, "Did not queue IRQ COPS");

        // STAMP("timer=0 - setting up next cycle");get_next_timer_event();     // setup next cycle
        next_expired_timer = 0;
        get_next_timer_event();
        return;
    }

    if ((next_expired_timer & 0x7f) == CYCLE_TIMER_COPS_CLOCK_DSEC)
    { // update system clock

// might want to find a better place for this, but since 1/10th of a second occurs relatively
// rarely compared to the 1/60th sec vertical redraw, it's somewhat better.
#ifdef DEBUG
#ifdef DEBUG_ON_SCREENHASH

        if (cmp_screen_hash(debug_hash, get_screen_hash()) > SCREENHASH_LIMIT && 0 == debug_log_enabled)
        {
            debug_on("screenhash-met");
            debug_log_enabled = 1;
        }

#endif
#ifdef DEBUG_OFF_SCREENHASH
        if (cmp_screen_hash(debug_hash_off, get_screen_hash()) > SCREENHASH_LIMIT && debug_log_enabled)
        {
            debug_off();
            debug_log_enabled = 0;
            ALERT_LOG(0, "Debug OFf SCREENHASH_OFF Met");
        }
#endif
#endif

        DEBUG_LOG(0, "COPS Decisecond timer entered.\n");

        // take care of some other housekeeping too
        if (floppy_ram[0])
            floppy_go6504();
        if (scc_running > 2)
            scc_control_loop();
        seek_mouse_event();
        if (scc_running)
            scc_control_loop();

        next_expired_timer = 0;
        get_next_timer_event();

        // lisa can ask COPS to disable the clock.  When this is done, don't track time until re-enabled
        if (!lisa_clock_on)
        {
            STAMP("lisa clock is disabled, therefore frozen");
            return;
        }

#ifndef USE64BITTIMER
        if (lasttenth > HALF_CLK)
            lasttenth = CLKDIV2(lasttenth);
#endif

        if ((lasttenth + TENTH_OF_A_SECOND) > cpu68k_clocks)
            return; // ensure we're not updating too often.

        // decisecond_clk_tick();             // handled by lisaem_wx.cpp OnIdle loop //20070409//
        tenth_sec_cycles = cpu68k_clocks + TENTH_OF_A_SECOND; // schedule next 1/10th second IRQ to fire

        // ALERT_LOG(0,"1/10th tick. cpu clk:%lld\n",cpu68k_clocks);

        // printlisatime(buglog);
        return;
    }

    /// Floppy FDIR events /////////////////////////////////////////////////////////////////////////////////////////////////////////
    if ((next_expired_timer & 0x7f) == CYCLE_TIMER_FDIR)
    {
        DEBUG_LOG(0, "Entering CYCLE_TIMER_FDIR");

        next_expired_timer = 0;
        get_next_timer_event();

        if (fdir_timer == -1)
            return; // prevent duplicate IRQ's.

        fdir_timer = -1; // clear the timer
        FloppyIRQ_time_up();

        if (!floppy_FDIR) // 20060605 - then the thing is cleared here!
        {
            DEBUG_LOG(0, "floppy_FDIR is not set, so no IRQ to fire.");
            return;
        } // was it suppressed already? ignore it.

        if ((floppy_irq_top && (floppy_ram[DRIVE] & 0x80)) || (floppy_irq_bottom && (floppy_ram[DRIVE] & 0x08)))
        {
            DEBUG_LOG(0, "Adding AUTOVECTOR FDIR IRQ to queue because FDIR is on");
            reg68k_external_autovector(1);
        }
        return;
    }

    if (next_expired_timer == CYCLE_TIMER_Z8530)
    {
        DEBUG_LOG(0, "[zilog8530.c:]Count Zero Interrupt");
        z8530_event = -1;
        z8530_last_irq_status_bits = 128;
    }

    // Handle VIA related timers from this point on
    if (!V)
        return; // if we have an erroneous timer, return;

    /// Shift register events //////////////////////////////////////////////////////////////////////////////////////////////////////
    if (next_expired_timer & 0x40) // shift register
    {
        DEBUG_LOG(0, "Handling Shift Register");
        flag_via_sr_irq(i);

        next_expired_timer = 0;
        get_next_timer_event();
        return;
    }

    /// VIA Timer 1 and Timer 2 Events ////////////////////////////////////////////////////////////////////////////////////////////
    DEBUG_LOG(0, "Handling timer 1 or 2");
    if (next_expired_timer & 0x80)
        flag_via_t2_irq(i);
    else
        flag_via_t1_irq(i);

    next_expired_timer = 0;
    get_next_timer_event();
    return;
}

void printlisatime(FILE *out) // printclock
{
    //              year  days___  h h  m m  s s tenths
    if (out)
        fprintf(out, "%d.%02x%1x-%d%d:%d%d:%d%d.%d alarm:%ld clk:%016llx ",

                1980 + (lisa_clock.year & 0x0f),
                lisa_clock.days_h, lisa_clock.days_l,

                lisa_clock.hours_h, lisa_clock.hours_l,
                lisa_clock.mins_h, lisa_clock.mins_l,
                lisa_clock.secs_h, lisa_clock.secs_l,
                lisa_clock.tenths, (long)lisa_alarm, cpu68k_clocks);

    normalize_lisa_clock();
}
