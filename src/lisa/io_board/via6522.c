/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2021.03.26                   *
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
*                               VIA 6522 Emulators                                     *
*                                                                                      *
*           these control the parallel interfaces for external profile, and            *
*           internal widget hard drives, the ADMP/Canon printers, the COPS             *
*           keyboard/mouse/clock controller, and expansion slot parallel cards.        *
*                                                                                      *
\**************************************************************************************/

#define PROLOOP_EV_IRB 0 // event=0 <- Read from IRB
#define PROLOOP_EV_IRA 1 // event=1 <- Read from IRA
#define PROLOOP_EV_ORA 2 // event=2 <- write to ORA
#define PROLOOP_EV_ORB 3 // event=3 <- write to ORB
#define PROLOOP_EV_NUL 4 // event=4 <- null event - called occasionally by event handling to allow timeouts

#define FIX_VIA_IFR(vianum)              \
    {                                    \
        if (via[vianum].via[IFR] & 127)  \
            via[vianum].via[IFR] |= 128; \
        else                             \
            via[vianum].via[IFR] = 0;    \
    }

#define FIX_VIAP_IFR()          \
    {                           \
        if (V->via[IFR] & 127)  \
            V->via[IFR] |= 128; \
        else                    \
            V->via[IFR] = 0;    \
    }

// clear CA1/CA2 on ORA/IRA access
// these 3 macros must be copied to cops.c for via1_ira/ora.
#define VIA_CLEAR_IRQ_PORT_A(x)                                            \
    {                                                                      \
        if (((via[x].via[PCR] >> 1) & 7) != 1 &&                           \
            ((via[x].via[PCR] >> 1) & 7) != 3)                             \
            via[x].via[IFR] &= (0xff - VIA_IRQ_BIT_CA2 - VIA_IRQ_BIT_CA1); \
        via[x].ca1 = 0;                                                    \
        via[x].ca2 = 0;                                                    \
        if (via[x].via[IFR] == 128)                                        \
            via[x].via[IFR] = 0;                                           \
    }
// clear Cb1/Cb2 on ORb/IRb access
#define VIA_CLEAR_IRQ_PORT_B(x)                                            \
    {                                                                      \
        if (((via[x].via[PCR] >> 5) & 7) != 1 &&                           \
            ((via[x].via[PCR] >> 5) & 7) != 3)                             \
            via[x].via[IFR] &= (0xff - VIA_IRQ_BIT_CB2 - VIA_IRQ_BIT_CB1); \
        via[x].cb1 = 0;                                                    \
        via[x].cb2 = 0;                                                    \
        if (via[x].via[IFR] == 128)                                        \
            via[x].via[IFR] = 0;                                           \
    }
// clear SR irq on SR access
#define VIA_CLEAR_IRQ_SR(x)                          \
    {                                                \
        via[x].via[IFR] &= ~VIA_IRQ_BIT_SR;          \
        if (via[x].via[IFR] == 128)                  \
            via[x].via[IFR] = 0;                     \
        DEBUG_LOG(0, "SR IRQ on via %d cleared", x); \
    }

// clear T1 irq on T1 read low or write high
#define VIA_CLEAR_IRQ_T1(x)                              \
    {                                                    \
        via[x].via[IFR] &= ~VIA_IRQ_BIT_T1;              \
        if (via[x].via[IFR] == 128)                      \
            via[x].via[IFR] = 0;                         \
        DEBUG_LOG(0, "Timer1 IRQ on via %d cleared", x); \
    }

// clear T2 irq on T2 read low or write high
#define VIA_CLEAR_IRQ_T2(x)                              \
    {                                                    \
        via[x].via[IFR] &= ~VIA_IRQ_BIT_T2;              \
        if (via[x].via[IFR] == 128)                      \
            via[x].via[IFR] = 0;                         \
        DEBUG_LOG(0, "Timer2 IRQ on via %d cleared", x); \
    }

#define IN_VIA6522_C
#include <vars.h>

extern void set_next_timer_id(uint8 x);

#define VIA_SET_CB2(x)                      \
    {                                       \
        via[x].via[IFR] |= VIA_IRQ_BIT_CB2; \
    }

void via_set_cb2_ifr(int x)
{
    if ((via[x].via[PCR] & (128 | 64 | 32)) == 128)
        VIA_SET_CB2(x);
}

#define FIX_CLKSTOP_VIA_T1(x)                                     \
    {                                                             \
        if (via[x].t1_e > -1 && via[x].t1_e < cpu68k_clocks_stop) \
        {                                                         \
            cpu68k_clocks_stop = via[x].t1_e;                     \
            set_next_timer_id(CYCLE_TIMER_VIAn_T1_TIMER(x));      \
        }                                                         \
    }

#define FIX_CLKSTOP_VIA_T2(x)                                                                                                     \
    {                                                                                                                             \
        if (via[x].t2_e > -1 && via[x].t2_e < cpu68k_clocks_stop)                                                                 \
        {                                                                                                                         \
            cpu68k_clocks_stop = via[x].t2_e;                                                                                     \
            set_next_timer_id(CYCLE_TIMER_VIAn_T2_TIMER(x));                                                                      \
        }                                                                                                                         \
        DEBUG_LOG(0, "fix_clk_stop_via_t2(%d) now:%llx stop:%llx timer:%llx", x, cpu68k_clocks, cpu68k_clocks_stop, via[x].t2_e); \
    }

// for the benefit of cops.c
void set_crdy_line(void)
{
    via[1].via[IRB] |= 64;
    DEBUG_LOG(0, "IRB=%02x", via[1].via[IRB]);
}
void clr_crdy_line(void)
{
    via[1].via[IRB] &= (255 - 64);
    DEBUG_LOG(0, "IRB=%02x", via[1].via[IRB]);
}
int is_crdy_line(void) { return via[1].via[IRB] & 64; }

void set_kb_data_ready(void)
{
    if (via[1].via[IER] & 2)
        via[1].via[IFR] |= VIA_IRQ_BIT_CA1;
    if (cops_event < 0)
        SET_COPS_NEXT_EVENT(0);
}

static int crdy_toggle = 0;

/////////////////////////////////////////////////////////////////////////////////////////////
// note these are identical to the ones in via6522 - they're static inline
// for speed, but remember to copy them here.  Could turn them into macros, but...

// pass it the via[?].t?_e, get back the timer value (used to read t1/t2)
inline static XTIMER get_via_timer_left_from_te(XTIMER t_e)
{
    // int32 diff=(t_e-cpu68k_clocks)/via_clock_diff;
    XTIMER diff = CPUCLK_TO_VIACLK((t_e - cpu68k_clocks));
    DEBUG_LOG(0, "t_e-clocks:%llx-%llx = %llx adjusted_diff:%llx diff/10:%llx", t_e, cpu68k_clocks, t_e - cpu68k_clocks, diff, (t_e - cpu68k_clocks) / 10);
    return (diff > 0 && t_e > 0) ? diff : 0;
}

inline static XTIMER get_via_timer_left_or_passed_from_te(XTIMER t_e, XTIMER timersetval)
{
    // int32 diff=(t_e-cpu68k_clocks)/via_clock_diff;
    XTIMER diff = CPUCLK_TO_VIACLK((t_e - cpu68k_clocks));
    XTIMER passed = CPUCLK_TO_VIACLK(timersetval - cpu68k_clocks);

    DEBUG_LOG(0, "t_e-clocks:%llx-%llx = %llx adjusted_diff:%llx diff/10:%llx", t_e, cpu68k_clocks, t_e - cpu68k_clocks, diff, (t_e - cpu68k_clocks) / 10);
    if (diff > 0)
        return diff;
    if (passed < 0)
        return passed;
    return 0;
}

inline static XTIMER get_via_te_from_timer(uint16 timer)
{
    XTIMER x = cpu68k_clocks + VIACLK_TO_CPUCLK(timer);

    if (!timer)
        return -1;

#ifndef USE64BITTIMER
    if (cpu68k_clocks + x > (XTIMER)(HALF_CLK))
    {
        prevent_clk_overflow_now();
    }
#endif

    // return cpu68k_clocks+(timer*via_clock_diff);}
    return x;
}
/////////////////////////////////////////////////////////////////////////////////////////////

// weird for some reason GCC 4.0.x complains when this is used later without a proto

// in the Lisa's case we have two VIA controllers, so we'll need two sets of
// registers.

#define via1 (via[0])
#define via2 (via[1])
#define via0l (via[2])
#define via0h (via[3])
#define via1l (via[4])
#define via1h (via[5])
#define via2l (via[6])
#define via2h (via[7])

// viatype via1, via2,                     // built in VIA's
//
//         via0l, via0h,                   // slot 0 VIA's.
//         via1l, via1h,                   // slot 1 VIA's.
//         via2l, via2h;                   // slot 2 VIA's.

uint8 via1_irb(uint8 reg);
void via1_orb(uint8 data);

uint8 via2_ira(uint8 regnum);
uint8 via2_irb(void);
void via2_ora(uint8 data, uint8 regnum);
void via2_orb(uint8 data);

/**
 * TorZidan@: this code is currenty unused, in favor of VIAProfileLoop() further down.
 * This code was used in releases RC5-2025.03.15, RC5-2024.07.28 and perhaps in few older ones.
 * It has few known bugs:
 * - When Installing Lisa Office System 1.0 from floppies, Lisa cannot find a ProFile drive.
 * - When installing Lisa Office System 2.0 from floppies, Lisa cannot write to the ProFile drive.
 * - Same when installing Pascal Worksop from floppies, see https://github.com/arcanebyte/lisaem/issues/4 
 * It turned out that the older verson of the code (found in release 1.2.6) works well,
 * and that's what we are using further down.
 * I decided to keep this here, for posterity. 
void VIAProfileLoop_unused(int vianum, ProFileType *P, int event)
{
    if (event < 0)
        return;

    int BSY = (!(P->BSYLine != 0)); // before
    if (event != 100)
        ProfileLoop(P, event);       // ignore null events - 2021.06.06
    int NBSY = (!(P->BSYLine != 0)); // after

    // IF BSY has changed, and CA1 IRQ's are enabled, then, see if we need to send an IRQ based on polarity in PCR CA1 bit
    if (NBSY != BSY) // ignore when there's no transition.
    {
        // PCR bit 0 :=0 IRQ on high to low transition BSY=(1 0)NBSY
        // PCR bit 0 :=1 IRQ on low to high transition BSY=(0 1)=NBSY
        // can use XOR below with BSY
        if (BSY ^ (via[vianum].via[PCR] & 1))
        {
            if ((via[vianum].via[IFR] & VIA_IRQ_BIT_CA1) == 0)
            {
                DEBUG_LOG(0, "CA1 IER is not enabled, not checking for IRQ");
            }
            else
            {
                via[vianum].via[IFR] |= VIA_IRQ_BIT_CA1;
                DEBUG_LOG(0, "profile.c:Enabling BSY IRQ on CA1 for VIA #%d BSY transitioned to:%d%d PCR flag:%d", vianum, BSY, NBSY, (via[vianum].via[PCR] & 1));
            }
            return;
        }
    }
}
*/

// VIA Wrapper around ProFile loop to handle BSY (CA1) IRQ's -- if they're enabled that is.
void VIAProfileLoop(int vianum, ProFileType *P, int event)
{
    int BSY=P->BSYLine;
    //    if (via[2].ProFile->DENLine!=0) return;
    ProfileLoop(P,event);

    // IF BSY has changed, and CA1 IRQ's are enabled, then, see if we need to send an IRQ based on polarity in PCR CA1 bit
    if (P->BSYLine !=BSY)
    {
        //- not sure which is right. 1=positive edge, 0=negative edge, I think the logical XOR
        //if (P->BSYLine!=0) && ((via[vianum].via[PCR] & 1)!=0) )   via[vianum].via[IFR] |=VIA_IRQ_BIT_CA1;

        DEBUG_LOG(0,"oldbsy:%d newbsy:%d PCR_bit0:%d   tag:profile.c",BSY,P->BSYLine,via[vianum].via[PCR]);

        if ((P->BSYLine!=0) ^  ((via[vianum].via[PCR] & 1)!=0) )
            {
                via[vianum].via[IFR] |=VIA_IRQ_BIT_CA1;

                DEBUG_LOG(0,"state:%d Enabled CA1(BSY) on %d->%d BSY transition pcr:%d pc24:%08x tag:profile.c",
                    P->StateMachineStep,BSY,P->BSYLine,
                    (via[vianum].via[PCR] & 1), pc24);
            }
        else
            {
                DEBUG_LOG(0,"state:%d Didn't enable CA1(BSY) on %d->%d BSY transition pcr:%d pc24:%08x tag:profile.c",
                    P->StateMachineStep,BSY,P->BSYLine,
                    (via[vianum].via[PCR] & 1), pc24);
            }
        // too bad we don't have a ^^ operator like we do && and || - logical XOR would be nice here.
    }
}

/***********************************************************************************\
*  Functions to handle VIA2 I/O for profile drive                                   *
*                                                                                   *
\***********************************************************************************/

char *via_regname(int i)
{
    char *viaregname[] =
        {
            "I/ORB    ",
            "I/ORA    ",
            "DDRB     ",
            "DDRA     ",
            "T1CL     ",
            "T1CH     ",
            "T1LL     ",
            "T1LH     ",
            "T2CL     ",
            "T2CH     ",
            "SHIFTR   ",
            "ACR      ",
            "PCR      ",
            "IFR      ",
            "IER      ",
            "I/ORANH  ",
            "T2LL shadow",
            "T2LH shadow"};

    if (i > -1 && i < 18)
        return viaregname[i];

    return "Unknown via regname!";
}

void fdumpvia1(FILE *out)
{
    int i, pcr, acr;

    fflush(out);
    fprintf(out, "Current CPU clock:%llx\n", cpu68k_clocks);
    fprintf(out, "via1 (COPS Controller) active:%d\n", via[1].active);
    fprintf(out, "via1 vianum:%d\n", via[1].vianum);

    fprintf(out, "via1 irqnum:%d\n", via[1].irqnum);
    fprintf(out, "via1 srcount:%d\n", via[1].srcount);
    fprintf(out, "via1 CA1,2/CB1,2:%02x,%02x/%02x,%02x\n", via[1].ca1, via[1].ca2, via[1].cb1, via[1].cb2);
    for (i = 0; i < 18; i++)
    {
        fprintf(out, "via1 reg[%d]=%02x %s\n", i, via[1].via[i], via_regname(i));

        if (i == 2)
            fprintf(out, "PB0:%s !PB1:%s PB2:%s PB3:%s !PB4:%s PB5:%s PB6:%s PB7:%s \n",
                    via[1].via[i] & 1 ? "Out" : "In",
                    via[1].via[i] & 2 ? "Out" : "In",
                    via[1].via[i] & 4 ? "Out" : "In",
                    via[1].via[i] & 8 ? "Out" : "In",
                    via[1].via[i] & 16 ? "Out" : "In",
                    via[1].via[i] & 32 ? "Out" : "In",
                    via[1].via[i] & 64 ? "Out" : "In",
                    via[1].via[i] & 128 ? "Out" : "In");

        if (i == 0)
            fprintf(out, "PB0:kreset:%s PB1-3:volume:%d PB4:floppy:%s PB5:parity_reset:%s PB6:cops_handshake:%s PB7:ctrl_reset:%s \n",
                    via[1].via[i] & 1 ? "On" : "Off",

                    (via[1].via[i] >> 1) & 7,

                    via[1].via[i] & 16 ? "On" : "Off",
                    via[1].via[i] & 32 ? "On" : "Off",
                    via[1].via[i] & 64 ? "On" : "Off",
                    via[1].via[i] & 128 ? "On" : "Off");

        fflush(out);
    }

    fprintf(out, "VIA1 IFR: (fired IRQ's): ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_CA2)
        fprintf(out, "CA2 ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_CA1)
        fprintf(out, "CA1 ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_SR)
        fprintf(out, "SHIFTREG ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_CB2)
        fprintf(out, "CB2 ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_CB1)
        fprintf(out, "CB1 ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_T2)
        fprintf(out, "TIMER2 ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_T1)
        fprintf(out, "TIMER1 ");
    if (via[1].via[IFR] & VIA_IRQ_BIT_SET_CLR_ANY)
        fprintf(out, "IRQON ");
    fprintf(out, "\n");
    fprintf(out, "VIA1 IER: (enabled IRQs): ");
    if (via[1].via[IER] & VIA_IRQ_BIT_CA2)
        fprintf(out, "CA2 ");
    if (via[1].via[IER] & VIA_IRQ_BIT_CA1)
        fprintf(out, "CA1 ");
    if (via[1].via[IER] & VIA_IRQ_BIT_SR)
        fprintf(out, "SHIFTREG ");
    if (via[1].via[IER] & VIA_IRQ_BIT_CB2)
        fprintf(out, "CB2 ");
    if (via[1].via[IER] & VIA_IRQ_BIT_CB1)
        fprintf(out, "CB1 ");
    if (via[1].via[IER] & VIA_IRQ_BIT_T2)
        fprintf(out, "TIMER2 ");
    if (via[1].via[IER] & VIA_IRQ_BIT_T1)
        fprintf(out, "TIMER1 ");
    if (via[1].via[IER] & VIA_IRQ_BIT_SET_CLR_ANY)
        fprintf(out, "SET/!CLR");
    fprintf(out, "\n");
    fprintf(out, "VIA1 Timer 1 fired %16llx times\n", via[1].t1_fired);
    fprintf(out, "VIA1 Timer 2 fired %16llx times\n", via[1].t2_fired);
    fflush(out);

    pcr = via[1].via[12];
    fprintf(out, "via1 PCR: CA1:%s\n", (pcr & 1) ? "Postive Active Edge" : "Negative Active Edge");
    switch ((pcr >> 1) & 7)
    {
    case 0:
        fprintf(out, "via1 PCR: CA2: 000 Input Negative Active Edge\n");
        break;
    case 1:
        fprintf(out, "via1 PCR: CA2: 001 Independent IRQ Input Negative Edge\n");
        break;
    case 2:
        fprintf(out, "via1 PCR: CA2: 010 Input Positive Active Edge\n");
        break;
    case 3:
        fprintf(out, "via1 PCR: CA2: 011 Independent IRQ Input Positive Edge\n");
        break;
    case 4:
        fprintf(out, "via1 PCR: CA2: 100 Handshake Output\n");
        break;
    case 5:
        fprintf(out, "via1 PCR: CA2: 101 Pulse Output\n");
        break;
    case 6:
        fprintf(out, "via1 PCR: CA2: 110 Low Output\n");
        break;
    case 7:
        fprintf(out, "via1 PCR: CA2: 111 High Output\n");
        break;
    }
    fprintf(out, "via1 PCR: CB1:%s\n", (pcr & 16) ? "Postive Active Edge" : "Negative Active Edge");
    switch ((pcr >> 5) & 7)
    {
    case 0:
        fprintf(out, "via1 PCR: CB2: 000 Input Negative Active Edge\n");
        break;
    case 1:
        fprintf(out, "via1 PCR: CB2: 001 Independent IRQ Input Negative Edge\n");
        break;
    case 2:
        fprintf(out, "via1 PCR: CB2: 010 Input Positive Active Edge\n");
        break;
    case 3:
        fprintf(out, "via1 PCR: CB2: 011 Independent IRQ Input Positive Edge\n");
        break;
    case 4:
        fprintf(out, "via1 PCR: CB2: 100 Handshake Output\n");
        break;
    case 5:
        fprintf(out, "via1 PCR: CB2: 101 Pulse Output\n");
        break;
    case 6:
        fprintf(out, "via1 PCR: CB2: 110 Low Output\n");
        break;
    case 7:
        fprintf(out, "via1 PCR: CB2: 111 High Output\n");
        break;
    }

    acr = via[1].via[11];
    fprintf(out, "via1 ACR: PA %s\n", (acr & 1) ? "latching enabled" : "latching disabled");
    fprintf(out, "via1 ACR: PB %s\n", (acr & 2) ? "latching enabled" : "latching disabled");
    fprintf(out, "via1 SHIFT Register Control: ");
    switch ((acr >> 2) & 7)
    {
    case 0:
        fprintf(out, "000 Disabled\n");
        break;
    case 1:
        fprintf(out, "001 under control of T2\n");
        break;
    case 2:
        fprintf(out, "010 under control of phi2\n");
        break;
    case 3:
        fprintf(out, "011 under control of ext. clock\n");
        break;
    case 4:
        fprintf(out, "100 free running at t2 rate\n");
        break;
    case 5:
        fprintf(out, "101 under control of t2\n");
        break;
    case 6:
        fprintf(out, "110 under control of phi2\n");
        break;
    case 7:
        fprintf(out, "111 under control of ext. clock\n");
        break;
    }

    fprintf(out, "via1 ACR: T2 Timer Control:%s\n", (acr & 32) ? "Countdown with pulses on PB6" : "Timed IRQ");

    fprintf(out, "via1 T2 cpu_68k clk expiration:%016llx cpu68k_clocks:%16llx fired %lld times\n", via[1].t2_e, cpu68k_clocks, via[1].t2_fired);
    fprintf(out, "via1 T1 cpu_68k clk expiration:%016llx cpu68k_clocks:%16llx fired %lld times\n", via[2].t1_e, cpu68k_clocks, via[1].t1_fired);

    fprintf(out, "via1 T1 Control: ");
    switch (acr >> 6)
    {
    case 0:
        fprintf(out, "00 Timed IRQ each time T1 Loaded, PB7 Disabled\n");
        break;
    case 1:
        fprintf(out, "01 Continous IRQ's  PB7 Disabled\n");
        break;
    case 2:
        fprintf(out, "10 Timed IRQ each time T1 Loaded, PB7 One Shot Output\n");
        break;
    case 3:
        fprintf(out, "11 Continous IRQ's, PB7 Square Wave Output\n");
        break;
    }
    fflush(out);
}

void fdumpvia2(FILE *out)
{
    int i, pcr, acr;

    fprintf(out, "\n\n============0=via2 (Motherboard Parallel Port) active:%d============\n", via[2].active);
    fprintf(out, "via2 vianum:%d\n", via[2].vianum);

    fprintf(out, "via2 irqnum:%d\n", via[2].irqnum);
    fprintf(out, "via2 srcount:%d\n", via[2].srcount);
    fprintf(out, "via2 CA1,2/CB1,2:%02x,%02x/%02x,%02x\n", via[2].ca1, via[2].ca2, via[2].cb1, via[2].cb2);

    for (i = 0; i < 18; i++)
        fprintf(out, "via2 reg[%d]=%02x %s\n", i, via[2].via[i], via_regname(i));

    if (i == 2)
        fprintf(out, "PB0:%s PB1:%s PB2:%s PB3:%s PB4:%s PB5:%s PB6:%s PB7:%s \n",
                via[2].via[i] & 1 ? "OCD:Out" : "OCD:In",
                via[2].via[i] & 2 ? "BSY:Out" : "BSY:In",
                via[2].via[i] & 4 ? "DEN:Out" : "DEN:In",
                via[2].via[i] & 8 ? "RRW:Out" : "RRW:In",
                via[2].via[i] & 16 ? "CMD:Out" : "CMD:In",
                via[2].via[i] & 32 ? "PAR:Out" : "PAR:In",
                via[2].via[i] & 64 ? "DSK:Out" : "DSK:In",
                via[2].via[i] & 128 ? "CRS:Out" : "CRS:In");

    if (i == 0)
        fprintf(out, "PB0:OCD:%s PB1<!BSY:%s PB2:!DEN:%s PB3:drw:%s PB4>!CMD:%s PB5:!parity:%s PB6:FDIR:%s PB7:Contrast:%s \n",
                via[2].via[i] & 1 ? "no" : "connected",
                via[2].via[i] & 2 ? "busy" : "ready",
                via[2].via[i] & 4 ? "contrast" : "parallel",
                via[2].via[i] & 8 ? "write" : "read",
                via[2].via[i] & 16 ? "no" : "yes",
                via[2].via[i] & 32 ? "ok" : "error",
                via[2].via[i] & 64 ? "1" : "0",
                via[2].via[i] & 128 ? "1" : "0");

    fprintf(out, "VIA2 IFR: (fired IRQ's): ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_CA2)
        fprintf(out, "CA2 ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_CA1)
        fprintf(out, "CA1 ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_SR)
        fprintf(out, "SHIFTREG ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_CB2)
        fprintf(out, "CB2 ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_CB1)
        fprintf(out, "CB1 ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_T2)
        fprintf(out, "TIMER2 ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_T1)
        fprintf(out, "TIMER1 ");
    if (via[2].via[IFR] & VIA_IRQ_BIT_SET_CLR_ANY)
        fprintf(out, "IRQON ");
    fprintf(out, "\n");
    fprintf(out, "VIA2 IER: (enabled IRQs): ");
    if (via[2].via[IER] & VIA_IRQ_BIT_CA2)
        fprintf(out, "CA2 ");
    if (via[2].via[IER] & VIA_IRQ_BIT_CA1)
        fprintf(out, "CA1 ");
    if (via[2].via[IER] & VIA_IRQ_BIT_SR)
        fprintf(out, "SHIFTREG ");
    if (via[2].via[IER] & VIA_IRQ_BIT_CB2)
        fprintf(out, "CB2 ");
    if (via[2].via[IER] & VIA_IRQ_BIT_CB1)
        fprintf(out, "CB1 ");
    if (via[2].via[IER] & VIA_IRQ_BIT_T2)
        fprintf(out, "TIMER2 ");
    if (via[2].via[IER] & VIA_IRQ_BIT_T1)
        fprintf(out, "TIMER1 ");
    if (via[2].via[IER] & VIA_IRQ_BIT_SET_CLR_ANY)
        fprintf(out, "SET/!CLR");
    fprintf(out, "\n");
    fprintf(out, "VIA2 Timer 1 fired %lld times\n", via[2].t1_fired);
    fprintf(out, "VIA2 Timer 2 fired %lld times\n", via[2].t2_fired);

    pcr = via[2].via[12];
    fprintf(out, "via2 PCR: CA1:%s\n", (pcr & 1) ? "Postive Active Edge" : "Negative Active Edge");
    switch ((pcr >> 1) & 7)
    {
    case 0:
        fprintf(out, "via2 PCR: CA2: 000 Input Negative Active Edge\n");
        break;
    case 1:
        fprintf(out, "via2 PCR: CA2: 001 Independent IRQ Input Negative Edge\n");
        break;
    case 2:
        fprintf(out, "via2 PCR: CA2: 010 Input Positive Active Edge\n");
        break;
    case 3:
        fprintf(out, "via2 PCR: CA2: 011 Independent IRQ Input Positive Edge\n");
        break;
    case 4:
        fprintf(out, "via2 PCR: CA2: 100 Handshake Output\n");
        break;
    case 5:
        fprintf(out, "via2 PCR: CA2: 101 Pulse Output\n");
        break;
    case 6:
        fprintf(out, "via2 PCR: CA2: 110 Low Output\n");
        break;
    case 7:
        fprintf(out, "via2 PCR: CA2: 111 High Output\n");
        break;
    }
    fprintf(out, "via2 PCR: CB1:%s\n", (pcr & 16) ? "Postive Active Edge" : "Negative Active Edge");
    switch ((pcr >> 5) & 7)
    {
    case 0:
        fprintf(out, "via2 PCR: CB2: 000 Input Negative Active Edge\n");
        break;
    case 1:
        fprintf(out, "via2 PCR: CB2: 001 Independent IRQ Input Negative Edge\n");
        break;
    case 2:
        fprintf(out, "via2 PCR: CB2: 010 Input Positive Active Edge\n");
        break;
    case 3:
        fprintf(out, "via2 PCR: CB2: 011 Independent IRQ Input Positive Edge\n");
        break;
    case 4:
        fprintf(out, "via2 PCR: CB2: 100 Handshake Output\n");
        break;
    case 5:
        fprintf(out, "via2 PCR: CB2: 101 Pulse Output\n");
        break;
    case 6:
        fprintf(out, "via2 PCR: CB2: 110 Low Output\n");
        break;
    case 7:
        fprintf(out, "via2 PCR: CB2: 111 High Output\n");
        break;
    }

    acr = via[2].via[11];
    fprintf(out, "via2 ACR: PA %s\n", (acr & 1) ? "latching enabled" : "latching disabled");
    fprintf(out, "via2 ACR: PB %s\n", (acr & 2) ? "latching enabled" : "latching disabled");
    fprintf(out, "via2 SHIFT Register Control: ");
    switch (acr >> 2)
    {
    case 0:
        fprintf(out, "000 Disabled\n");
        break;
    case 1:
        fprintf(out, "001 under control of T2\n");
        break;
    case 2:
        fprintf(out, "010 under control of phi2\n");
        break;
    case 3:
        fprintf(out, "011 under control of ext. clock\n");
        break;
    case 4:
        fprintf(out, "100 free running at t2 rate\n");
        break;
    case 5:
        fprintf(out, "101 under control of t2\n");
        break;
    case 6:
        fprintf(out, "110 under control of phi2\n");
        break;
    case 7:
        fprintf(out, "111 under control of ext. clock\n");
        break;
    }

    fprintf(out, "via2 ACR: T2 Timer Control:%s\n", (acr & 32) ? "Countdown with pulses on PB6" : "Timed IRQ");
    fprintf(out, "via2 T2 cpu_68k clk expiration:%16llx cpu68k_clocks:%016llx fired %lld times\n", via[2].t2_e, cpu68k_clocks, via[2].t2_fired);
    fprintf(out, "via2 T1 cpu_68k clk expiration:%16llx cpu68k_clocks:%016llx t1_fired:%lld times\n", via[2].t1_e, cpu68k_clocks, via[2].t1_fired);

    fprintf(out, "via2 T1 Control: ");
    switch (acr >> 6)
    {
    case 0:
        fprintf(out, "00 Timed IRQ each time T1 Loaded, PB7 Disabled\n");
        break;
    case 1:
        fprintf(out, "01 Continous IRQ's  PB7 Disabled\n");
        break;
    case 2:
        fprintf(out, "10 Timed IRQ each time T1 Loaded, PB7 One Shot Output\n");
        break;
    case 3:
        fprintf(out, "11 Continous IRQ's, PB7 Square Wave Output\n");
        break;
    }

    fprintf(out, "================================================================================\n");
}

void dumpvia(void)
{
    char filename[1024];
    FILE *out;
    return; // disabled

    // debug_log_enabled=1;
    sprintf(filename, "./lisaem-output-%08x.%016llx.vias.txt", pc24, cpu68k_clocks);
    out = fopen(filename, "wt");

    fdumpvia1(out);
    fdumpvia2(out);

    fclose(out);
}

void print_via_profile_state(char *s, uint8 data, viatype *V)
{
#ifdef DEBUG
    if (!debug_log_enabled || !buglog)
        return;
    fprintf(buglog, "%s(%02x) via#:%d ProFileState#:%d widx:%d, ridx:%d,block#:%ld pb1<!BSY:%d pb2!DEN:%d pb3!RRW:%d pb4>!CMD:%d :: VIA:: ", s, data,
            V->vianum,
            V->ProFile->StateMachineStep, V->ProFile->indexwrite, V->ProFile->indexread, (long)V->ProFile->blocktowrite,
            V->ProFile->BSYLine,
            V->ProFile->DENLine,
            V->ProFile->RRWLine,
            V->ProFile->CMDLine);
    if (OCDLine_BIT & V->via[DDRB])
        fprintf(buglog, " !OCDLine:%02x ", data & OCDLine_BIT);
    if (BSYLine_BIT & V->via[DDRB])
        fprintf(buglog, "<!BSYLine:%02x ", data & BSYLine_BIT);
    if (DENLine_BIT & V->via[DDRB])
        fprintf(buglog, " !DENLine:%02x ", data & DENLine_BIT);
    if (RRWLine_BIT & V->via[DDRB])
        fprintf(buglog, " !R/RWLine:%02x ", data & RRWLine_BIT);
    if (CMDLine_BIT & V->via[DDRB])
        fprintf(buglog, ">!CMDLine :%02x ", data & CMDLine_BIT);
    if (PARITY_BIT & V->via[DDRB])
        fprintf(buglog, " !ParityReset:%02x ", data & PARITY_BIT);
    if (DSK_DIAG_BIT & V->via[DDRB])
        fprintf(buglog, " Floppy_wait:%02x ", data & DSK_DIAG_BIT);
    if (CTRL_RES_BIT & V->via[DDRB])
        fprintf(buglog, " !ProFileReset:%02x", data & CTRL_RES_BIT);
    fprintf(buglog, "\n");
    fflush(buglog);
    // if (   (9 & via[2].via[DDRB])==9) {if ((data & OCDLine_BIT)==0) {debug_log_enabled=1; debug_on("");} }
#else
    UNUSED(s);
    UNUSED(data);
    UNUSED(V);
#endif
}

int check_contrast_set(void)
{
    if ((via[2].via[DDRB] & 0x84) == 0x84 && (via[2].via[ORBB] & 0x4) == 4 && via[2].via[DDRA] != 0) // this is OK (DDRA used with port B!)
    {
        contrast = via[2].via[DDRA] & via[2].via[ORAA];
        videoramdirty |= 9;
        //      ALERT_LOG(0,"Setting Contrast:%02x",contrast);
        if (contrast == 0xff)
            disable_vidram();
        else
            enable_vidram();

        contrastchange(); // force UI to adjust display
        return 1;
    }

    return 0;
}

uint8 via2_ira(uint8 regnum)
{
    UNUSED(regnum);
    VIA_CLEAR_IRQ_PORT_A(2); // clear CA1/CA2 on ORA/IRA access

    if (via[2].via[ORBB] & via[2].via[DDRB] & 4)
    {
        DEBUG_LOG(0, "Returning Contrast");
        return contrast;
    }

    if (via[2].ProFile) // Is this a profile?
    {
        VIAProfileLoop(2, via[2].ProFile, PROLOOP_EV_IRA);
        via[2].via[IRA] = via[2].ProFile->VIA_PA;
        DEBUG_LOG(0, "profile.c:READ %02x from ProFile, pc24:%08x  tag:profile state:%d", via[2].via[IRA], pc24, via[2].ProFile->StateMachineStep);
        return via[2].via[IRA];
    }
    else if (via[2].ADMP)
        return 0;

    return 0; // nothing attached, ignore.
}

// lisa_rb_Oxd800_par_via2(uint32 addr)
void via2_ora(uint8 data, uint8 regnum)
{
    UNUSED(regnum);
    VIA_CLEAR_IRQ_PORT_A(2); // clear CA1/CA2 on ORA/IRA access
    via[2].last_pa_write = cpu68k_clocks;
    DEBUG_LOG(0, "ORA:%02x DDRA:%02x   ORB:%02x DDRB:%0x", via[2].via[ORA], via[2].via[DDRA], via[2].via[ORB], via[2].via[DDRB]);
    if (via[2].via[DDRA] == 0)
        return; // can't write just yet, ignore.
    if (check_contrast_set())
        return;

    if (via[2].via[ORBB] & via[2].via[DDRB] & 4)
        return; // driver enable is off, don't process ADMP/Profile data output.
    via[2].orapending = 0;

    // if (debug_log_enabled) fdumpvia2(buglog);
    if (via[2].ProFile) // Is this a profile?
    {
        DEBUG_LOG(0, "profile.c:widget.c:Sending byte >%02x from via2_ora to ProFile code pc24:%08x tag:profile step:%d @%d",
                  data,
                  pc24,
                  via[2].ProFile->StateMachineStep, via[2].ProFile->indexwrite);
        via[2].ProFile->VIA_PA = data;
        via[2].via[ORA] = data;
        via[2].ProFile->last_a_accs = 1; // 20060323//
        VIAProfileLoop(2, via[2].ProFile, PROLOOP_EV_ORA);
        return;
    }

    if (via[2].ADMP)
    {
        via[2].via[ORA] = data;
        ImageWriterLoop(via[2].ADMP, data);
        return;
    }
}

void via2_orb(uint8 data)
{
#ifdef DEBUG
    uint8 flipped = ((data ^ via[2].via[ORBB]) & (via[2].via[DDRB]));
#endif

    VIA_CLEAR_IRQ_PORT_B(2); // clear Cb1/Cb2 on ORb/IRb access

    // Setting Contrast - DEN is off, PB7 is used as Strobe.
    if (check_contrast_set())
        return;

    // Is there an attached ProFile device on this parallel port?
    if (via[2].ProFile)
    {

        // Push the ORB bits to the ProFile flags. (OCD/BSY/DSKDiag/PARITY_BIT are inputs, so we don't do them, hence they're
        // commented out, but left in for the sake of completeness / being pendantic, fasict, totalitarian, etc.
        //------------------------------------------------------------------------------------------------------------------------------------
        //  if ( OCDLine_BIT & via[2].via[DDRB]) {via[2].ProFile->DENLine=(data & OCDLine_BIT) ? 0:1;}   //0 Ignored - hardwired input only line
        //  if ( BSYLine_BIT & via[2].via[DDRB]) {via[2].ProFile->BSYLine=(data & BSYLine_BIT) ? 0:1;}   //1 Ignored - hardwired input only line
        if (DENLine_BIT & via[2].via[DDRB])
        {
            via[2].ProFile->DENLine = (data & DENLine_BIT) ? 0 : 1;
        } // 2 DEN - drive enable....
        if (RRWLine_BIT & via[2].via[DDRB])
        {
            via[2].ProFile->RRWLine = (data & RRWLine_BIT) ? 0 : 1;
        } // 3 RRW is 1=write, 0=read from port
        if (CMDLine_BIT & via[2].via[DDRB])
        {
            via[2].ProFile->CMDLine = (data & CMDLine_BIT) ? 0 : 1;
        } // 4 CMD line
        //  if ( PARITY_BIT   & via[2].via[DDRB]) {via[2].ProFile->Parity=(data &  PARITY_BIT) ? 0:1;}   //5 Parity error bit is input
        if (DSK_DIAG_BIT & via[2].via[DDRB])
        {
            DEBUG_LOG(0, "Write to DSK_DIAG on via2_orb!:%02x", data & DSK_DIAG_BIT);
        } // 6 Floppy Disk Diag is input

        // this is wrong, reset is on VIa1 - this is an input from the IWM WRQ pin 9, to PB7 - input only? there's a buffer at U1C LS367 pin 14->13
        // if ( CTRL_RES_BIT & via[2].via[DDRB]) {if ((data & CTRL_RES_BIT)==0)                         //7 Controller reset
        //                                           { DEBUG_LOG(0,"Sent Controller Reset to ProFile:%02x",data & CTRL_RES_BIT);
        //                                             ProfileReset(via[2].ProFile);
        //                                           }
        //                                       }

        DEBUG_LOG(0, "profile.c:ORA:%02x DDRA:%02x   ORB:%02x DDRB:%0x", via[2].via[ORA], via[2].via[DDRA], via[2].via[ORB], via[2].via[DDRB]);
        DEBUG_LOG(0, "profile.c:pb0:!OCD:%d pb1<!BSY:%d pb2:!DEN:%d pb3:!DR/W/:%d pb4>!CMD/:%d pb5:PRES:%d pb6:DIAGWait:%d pb7:CTRL_RES:%d = Data=%02x",
                  (data & 1), (data & 2), (data & 4), (data & 8), (data & 16), (data & 32), (data & 64), (data & 128), data);

        DEBUG_LOG(0, "Flipped bits: %s%s%s%s%s%s%s%s", ((flipped & 1) ? "OCD:pb0 " : "    "), ((flipped & 2) ? "BSY:pb1 " : "    "), ((flipped & 4) ? "DEN:pb2 " : "    "), ((flipped & 8) ? "RRW:pb3 " : "    "), ((flipped & 16) ? "CMD:pb4 " : "    "), ((flipped & 32) ? "PAR:pb5 " : "    "), ((flipped & 64) ? "DSK:pb6 " : "    "), ((flipped & 128) ? "CRS:pb7 " : "    "));

        via[2].ca1 = via[2].ProFile->BSYLine; // refresh

        DEBUG_LOG(0, "before Lines DEN:%d RRW:%d CMD:%d",
                  via[2].ProFile->DENLine, via[2].ProFile->RRWLine, via[2].ProFile->CMDLine);

        //       if (pc24<0x00fe0000) print_via_profile_state("\nvia2_orb pre-loop ", data,&via[2]);
        //       if (debug_log_enabled) fdumpvia2(buglog);
        VIAProfileLoop(2, via[2].ProFile, PROLOOP_EV_ORB);

        if (pc24 < 0x00fe0000)
            print_via_profile_state("\nvia2_orb post-loop ", data, &via[2]);
    }

    // Is there an attached dot matrix printer?
    if (via[2].ADMP)
        return; // Output to Apple Dot Matrix Printer -- not yet implemented

    return; // nothing attached, ignore.
}

uint8 via2_irb(void) // this is good
{
    // remove these two.
    // static uint32 lastpc;
    // static uint32 lastpccount;

    uint8 retval = 0;

    // floppy_6504_wait=!floppy_6504_wait;

    VIA_CLEAR_IRQ_PORT_B(2); // clear Cb1/Cb2 on ORb/IRb access

    // return the values of the previous output bits
    /// retval = via[2].via[IRB] & via[2].via[DDRB];  //20060110 - no longer needed

    if (floppy_ram[0])
    {
        floppy_go6504();
    } // if this is being read, it's likely Lisa is waiting on floppy, so speed it up

    // set up the return of the common bits before we deal with attached devices
    retval |= (floppy_6504_wait ? 0 : DSK_DIAG_BIT);                                                // return floppy diag bit
    retval |= (((via[2].ProFile || via[2].ADMP) && IS_PARALLEL_PORT_ENABLED(2)) ? 0 : OCDLine_BIT); // ADMP/Profile attached?

    if (via[2].ProFile)
    {

        if (pc24 < 0x00fe0000)
            print_via_profile_state("\nvia2_irb pre-loop ", via[2].via[IRB], &via[2]);

        // BSY is controlled by the profile, while DEN, RRW, and CMD are controlled by Lisa.  Lisa bits must be set before
        // entering the ProFile loop - and they are from via2_orb, while BSY must be set after the call to ProFile loop,
        // however, we set the rest of the bits in retval to return them to the Lisa as expected and to verify correctness.

        VIAProfileLoop(2, via[2].ProFile, PROLOOP_EV_IRB);

        // update return values from ProFile handshake/comm bits
        via[2].ca1 = via[2].ProFile->BSYLine;

        // Next four lines do the same thing as their uncommented versions below.  Leaving commented version in for clarity.
        // if (!(via[2].via[DDRB] & BSYLine_BIT)) retval |= (via[2].ProFile->BSYLine ? 0 : BSYLine_BIT); else retval |=(via[2].via[IRB] & BSYLine_BIT);
        // if (!(via[2].via[DDRB] & DENLine_BIT)) retval |= (via[2].ProFile->DENLine ? 0 : DENLine_BIT); else retval |=(via[2].via[IRB] & DENLine_BIT);
        // if (!(via[2].via[DDRB] & RRWLine_BIT)) retval |= (via[2].ProFile->RRWLine ? 0 : RRWLine_BIT); else retval |=(via[2].via[IRB] & RRWLine_BIT);
        // if (!(via[2].via[DDRB] & CMDLine_BIT)) retval |= (via[2].ProFile->CMDLine ? 0 : CMDLine_BIT); else retval |=(via[2].via[IRB] & CMDLine_BIT);

        // If the DDRB bit is an input return the value sent by the profile, otherwise return the previous value of that bit from the old IRB
        retval |= (!(via[2].via[DDRB] & BSYLine_BIT)) ? (via[2].ProFile->BSYLine ? 0 : BSYLine_BIT) : (via[2].via[IRB] & BSYLine_BIT);
        retval |= (!(via[2].via[DDRB] & DENLine_BIT)) ? (via[2].ProFile->DENLine ? 0 : DENLine_BIT) : (via[2].via[IRB] & DENLine_BIT);
        retval |= (!(via[2].via[DDRB] & RRWLine_BIT)) ? (via[2].ProFile->RRWLine ? 0 : RRWLine_BIT) : (via[2].via[IRB] & RRWLine_BIT);
        retval |= (!(via[2].via[DDRB] & CMDLine_BIT)) ? (via[2].ProFile->CMDLine ? 0 : CMDLine_BIT) : (via[2].via[IRB] & CMDLine_BIT);

        if ((via[2].via[DDRB] & PARITY_BIT) == 0)
            retval |= (eparity[via[2].via[ORA]] ? PARITY_BIT : 0); // re-enabled 2006.05.24
                                                                   //  if ((via[2].via[DDRB] & PARITY_BIT)==0) retval|=(eparity[via[2].via[ORA]] ? 0:PARITY_BIT);  // re-enabled 2006.05.25

        if (pc24 < 0x00fe0000)
            print_via_profile_state("\nvia2_irb post-loop ", retval, &via[2]);

        ///////////////////////////////////////////////////////////
        DEBUG_LOG(0, "Returning: %02x from via2_irb ProFile code", retval);
        DEBUG_LOG(0, "profile.c:ORA:%02x DDRA:%02x   ORB:%02x DDRB:%0x", via[2].via[ORA], via[2].via[DDRA], via[2].via[ORB], via[2].via[DDRB]);
        via[2].via[IRB] = retval;
        return retval;
    }
    else if (via[2].ADMP != 0)
    {
        retval |= BSYLine_BIT;
        return retval;
    }

    else if (via[2].irb)
        return via[2].irb(2, IRB);

    ALERT_LOG(0, "Unhandled device via #%d", 2);

    // retval |=0x02;   // Set ProFile busy line, since there isn't once attached.
    // retval |=4;      // Set Disk Enable Line to no drive there.

    DEBUG_LOG(0, "profile.c:pb0:!OCD:%d pb1<!BSY:%d pb2:!DEN:%d pb3:DR/W/:%d pb4>!CMD/:%d pb5:!PRES:%d pb6:DIAGWait:%d pb7:CTRL_RES:%d = Data=%02x",
              (retval & 1), (retval & 2), (retval & 4), (retval & 8), (retval & 16), (retval & 32), (retval & 64), (retval & 128), retval);

    // if (debug_log_enabled) fdumpvia2(buglog);
    via[2].via[IRB] = retval;
    return retval;
}

uint8 via1_irb(uint8 reg)
{
    uint8 data = 0;
    UNUSED(reg);
    data = via[1].via[ORB] & via[1].via[DDRB]; // allow Lisa to read back the outputs?
    DEBUG_LOG(0, "SRC: VIA1/COPS: reading reg %d port B", reg);
    VIA_CLEAR_IRQ_PORT_B(1);        // clear Cb1/Cb2 on ORb/IRb access
    data |= ((volume & 0x07) << 1); // PB1-3 is volume
    // data=via[1].via[IRB] & (255-16-64); // get whatever's in the register, except for stuff we set:FDIR,CRDY
    //  If lisa is reading this, it might be waiting on FDIR - so speed it up
    if (floppy_ram[0])
    {
        floppy_go6504();
    }

    // data |=(floppy_FDIR ? 16: 0);

    if (floppy_FDIR)
    {
        data |= 16;
        DEBUG_LOG(0, "FDIR=1 returned via1_irb\n"); /*floppy_FDIR=0;*/
    }                                               // was just 4, 16 didn't work, now I'm trying 16|4
    else
        DEBUG_LOG(0, "FDIR=0 returned via1_irb\n");

    // Is there a ProFile on VIA2? - yes, some lines are crossed between VIA1 and VIA2 - likely a legacy design bug before ProFiles
    if (via[2].ProFile)
    {
        data &= ~32;
        data |= (eparity[via[2].ProFile->VIA_PA] ? 32 : 0); // was via[2].via[ORA]
        if (data & 128)
            data |= 32; // when PB7=1 means ProFile is being reset, LisaTest expects PB5=1 too
        DEBUG_LOG(0, "Last ORA:%02x - Parity:%02x, DDRB:%d returning:%d",
                  via[2].via[ORA],
                  !eparity[via[2].via[ORA]],
                  (via[1].via[DDRB] & 32),
                  data & 32);
        // if ((via[2].via[DDRB] & PARITY_BIT)==0) data|=(eparity[via[2].via[ORA]] ? PARITY_BIT:0);
    }
    else
        DEBUG_LOG(0, "No Profile attached to Parallel Port!");

    // if ((via[2].via[DDRB] & PARITY_BIT)==0) data|=(eparity[via[2].via[ORA]] ? PARITY_BIT:0);

    // toggle ready on and off all the time to stimulate handshaking process - when data is ready on the COPS?

    crdy_toggle = (crdy_toggle + 1) & 3;
    // crdy_toggle = !crdy_toggle; //pb6 goes to D3 on COPS CA1,CA2 goes to S0,S1 on COPS. L0-L7=PA0-PA7.
    if (crdy_toggle)
    {
        data = data | 0x40;
        DEBUG_LOG(0, "via1_irb: COPS: CRDY is on");
    } // PB6 //
    else
    {
        data = data & (0xff - 0x40);
        DEBUG_LOG(0, "via1_irb: COPS: CRDY is off");
    }

    // 20060609-1750//if (copsqueuelen>0 || mousequeuelen>0)   data |= 0x40;
    // 20060609-1750//else                                     data &=~0x40;

    DEBUG_LOG(0, "SRC: COPS: copsqueuelen=%d", copsqueuelen);
    DEBUG_LOG(0, "SRC: COPS: returning data: %02x & %02x = %02x", data, via[1].via[DDRB] ^ 0xff, data & (~via[1].via[DDRB]));

    DEBUG_LOG(0, "via1_irb: pb1-3volume:%d  pb4:%s pb6:%s pb0:%s pb5:%s pb7:%s\n",
              ((data >> 1) & 7),
              ((data & 16) ? "FDIR=1" : "FDIR=0"),
              ((data & 64) ? "COPS_CRDY=1" : "COPS_CRDY=0"),
              ((data & 1) ? "COPS_KB_RESET=1" : "COPS_KB_reset=off"),
              ((data & 32) ? "Parity=On" : "Parity=Off"),
              ((data & 128) ? "Parallel_Controller_Reset=On" : "Parallel_Controller_Reset=Off"));

    return data & (~via[1].via[DDRB]);
}

void via1_orb(uint8 data)
{
    // unused???//uint8 xdata=data & via[1].via[DDRB];
#ifdef DEBUG
    uint8 flipped = ((via[1].via[ORB] ^ data) & via[1].via[DDRB]);

    DEBUG_LOG(0, "Writing VIA1 ORB. DDRB mask=%02x:(%s%s%s%s%s%s%s%s)", via[1].via[DDRB],
              ((via[1].via[DDRB] & 1) ? "PB0:out " : "pb0:in  "),
              ((via[1].via[DDRB] & 2) ? "PB1:out " : "pb1:in  "),
              ((via[1].via[DDRB] & 4) ? "PB2:out " : "pb2:in  "),
              ((via[1].via[DDRB] & 8) ? "PB3:out " : "pb3:in  "),
              ((via[1].via[DDRB] & 16) ? "PB4:out " : "pb4:in  "),
              ((via[1].via[DDRB] & 32) ? "PB5:out " : "pb5:in  "),
              ((via[1].via[DDRB] & 64) ? "PB6:out " : "pb6:in  "),
              ((via[1].via[DDRB] & 128) ? "PB7:out " : "pb7:in  "));

    DEBUG_LOG(0, "VIA1 ORB bits Flipped  :(%s%s%s%s%s%s%s%s)",
              ((flipped & 1) ? "PB0_kb reset." : " "),
              ((flipped & 2) ? "PB1 vol." : " "),
              ((flipped & 4) ? "PB2 vol." : " "),
              ((flipped & 8) ? "PB3 vol." : " "),
              ((flipped & 16) ? "PB4 FDIR." : " "),
              ((flipped & 32) ? "PB5 !PRES." : " "),
              ((flipped & 64) ? "PB6 cops_handshake." : " "),
              ((flipped & 128) ? "PB7 profile_reset." : " "));

    DEBUG_LOG(0, "VIA1 ORB write=%02x:(COPSRESET:%s Volume:%d FDIR:%s PRES:%s COPSHANDSHAKE:%s ProRESET:%s)", data,
              ((data & 1) ? "PB0:1no_reset " : "PB0:0-RESET "),
              ((data >> 1) & 7),
              ((data & 16) ? "PB4:1 " : "PB4:0 "),
              ((data & 32) ? "PB5:1 " : "PB5:0 "),
              ((data & 64) ? "PB6:1 " : "PB6:0 "),
              ((data & 128) ? "PB7:1 " : "PB7:0 "));

#endif

    VIA_CLEAR_IRQ_PORT_B(1); // clear Cb1/Cb2 on ORb/IRb access

    DEBUG_LOG(0, "SRC: COPS:handshake VOLUME: FDIR: Profile:reset outputting %02x to VIA1 ORB", data);

    via[1].via[ORB] = data;

    if (0x01 & via[1].via[DDRB])
    {
        if (!(data & 1))
            cops_reset();
    } // Reset Cops when PB0=0

    // Set sound volume PB1-3
    if ((via[1].via[DDRB] & 0x0e))
    {
        volume = ((volume << 1) & (0x0e ^ (via[1].via[DDRB] & 0x0e))) | (data & via[1].via[DDRB] & 0x0e) >> 1;
        return;
    }

    // Do I need this here???
    // if (via[1].via[DDRB]) floppy_FDIR=(data & 0x10 ? 1:0); // Is Lisa trying to clear or set FDIR?
    if (via[1].via[DDRB] & 16)
    {
        DEBUG_LOG(0, "Lisa attempting to clear/set FDIR  %02x -> %02x\n", data, data & 16);
    }
    //                              floppy_FDIR=(data&16 ? floppy_FDIR:0); }      // Is Lisa trying to clear FDIR?

    if (data & 32 & via[1].via[DDRB])
    {
        DEBUG_LOG(0, "lisa writing to Parity RESET: %02x ->%02x\n", data, data & 0x40);
    } // PRES/ Parity Reset

    if (data & 64 & via[1].via[DDRB])
    {
        DEBUG_LOG(0, "lisa writing to cops handshake: %02x ->%02x\n", data, data & 0x40);
    } // cops handshake

    // PB7=CRES/
    if (0x80 & via[1].via[DDRB])
    {
        DEBUG_LOG(0, "via1_orb ProFile !RESET :%02x %02x (0 means no reset, 80 means reset)\n", data, data & 0x80); // 20051214 flipped
        if (((data & 0x80)) && via[2].ProFile)
            ProfileReset(via[2].ProFile);
    } // PB7 = reset Profile

    return;
}

void chk_sound_play(void)
{
    if (via[1].via[SHIFTREG] != 0 && via[1].via[T2LL] != 0 && ((via[1].via[ACR] & 0x10) == 0x10))
    {
        sound_play(via[1].via[T2LH] << 8 | (via[1].via[T2LL])); // enable sound, smaller the higher the pitch.
        return;
    }

    if ((via[1].via[ACR] & 0x10) == 0)
        sound_off(); // otherwise, STFU
}

void lisa_wb_Oxdc00_cops_via1(uint32 addr, uint8 xvalue)
{

    via[1].active = 1; // these are always active as they're on the
    via[2].active = 1; // motherboard of the machine...

#ifdef DEBUG
    if (!(addr & 1))
    {
        DEBUG_LOG(0, "will ignore illegal address access to %08x write value=%02x", addr, xvalue);
    }
    else
    {
        DEBUG_LOG(0, "writing %02x to register %d (%s)", xvalue, (addr - VIA1BASE) / 2, via_regname((addr - VIA1BASE) / 2));
    }
#endif
    uint8 port = addr & 0x1f;
    switch (port)
    {
    case ORB1:
        /* It is possible to write multiple bytes by writing different
         * sized values out the VIA - this is used by the parallel via
         * for "speed", so we need to check the sizes.  Now since you
         * can't write to the port unless the Data Direction Reg bits
         * are on, we do an AND with them before writing out.         This is handled by the MMU! */

        VIA_CLEAR_IRQ_PORT_B(1); // clear Cb1/Cb2 on ORb/IRb access

        DEBUG_LOG(0, "ORB1");
        via[1].via[ORB] = xvalue;
        via[1].last_port = port;
        DEBUG_LOG(0, "VIA1 ORB(NH)=%02x", via[1].via[ORB]); // set the register to the unmasked data for later use.
        if (!via[1].via[DDRB])
            return;                                   // don't write anything if the DDRA is all inputs
        via1_orb(via[1].via[ORB] & via[1].via[DDRB]); // output the masked data.
        return;

    case ORANH1: // IRA/ORA
        via[1].last_a_accs = 1;
        DEBUG_LOG(0, "ORANH1");
        via[1].via[ORAA] = xvalue;
        via[1].last_port = port;
        via[1].via[ORA] = (via[1].via[IRAA] & (~via[1].via[DDRA])) | (via[1].via[ORAA] & via[1].via[DDRA]);
        DEBUG_LOG(0, "VIA1 ORA(NH)=%02x DDRA=%02x", via[1].via[ORA], via[1].via[DDRA]);
        if (!via[1].via[DDRA])
        {
            via[1].orapending = 1;
            return;
        } // don't write anything if the DDRA is all inputs
        via1_ora((via[1].via[ORA] & via[1].via[DDRA]), 15);
        return;

    case ORA1:
        via[1].last_a_accs = 1;

        if (via[1].via[ORA] == xvalue && via[1].last_port == ORANH2)
        {
            DEBUG_LOG(0, "in ORA1: Already wrote %02x to ORANH1, not pushing it again.\n", xvalue);
            via[1].last_port = port;
            return; // already what's on the port, ignore the write
        }

        VIA_CLEAR_IRQ_PORT_A(1); // clear CA1/CA2 on ORA/IRA access

        DEBUG_LOG(0, "ORA1");
        via[1].via[ORA] = (via[1].via[IRAA] & (~via[1].via[DDRA])) | (via[1].via[ORAA] & via[1].via[DDRA]);
        via[1].via[ORA] = xvalue;
        via[1].last_port = port;
        if (!via[1].via[DDRA])
        {
            via[1].orapending = 1;
            ;
            return;
        }                                                   // don't write anything if the DDRA is all inputs
        via1_ora(via[1].via[ORAA] & via[1].via[DDRA], ORA); // output the masked data
        return;

    case DDRB1: // DDRB 2
        DEBUG_LOG(0, "DDRB1");

        if (xvalue && xvalue != via[1].via[DDRB]) // only try the write if the mask has changed
        {
            via[1].via[DDRB] = xvalue;                     // do not optimize this out!
            via1_orb(via[1].via[ORBB] & via[1].via[DDRB]); // output the data that was masked off
            return;
        }
        else
            via[1].via[DDRB] = xvalue;
        via[1].last_port = port;

        return;

    case DDRA1: // and DDRA 3 registers
        DEBUG_LOG(0, "DDRA1");

        via[1].via[DDRA] = xvalue;
        via[1].last_port = port;
        if (via[1].orapending && xvalue != 0)           // only try to write if this mask changed and last access to A was a write
            via1_ora((via[1].via[ORAA] & xvalue), ORA); // output the masked data
        return;

        /*

        read from register
        4 T1LC read - T1 IRQ flag cleared (bit 6 in IRQ flag register)
        5 T1CH read - no comment on reset of T1 IRQ flag
        6 T1LL read - does not reset T1 IRQ flag!
        7 T1HL read - no comment on reset of T1 IRQ flag ?

        write to register
        4 T1LC actually writes to T1LL only
        5 T1HC actually writes to T1HL + copies T1LL->T1CL, T1LH->T1CH
        6 T1LL same as write to reg4
        7 T1HL value copied into T1HL, no transfer to T1CH
        */

    case T1LL1:                    // Timer 1 Low Order Latch
        DEBUG_LOG(0, "T1LL1");     // 6 T1LL same as write to reg4
        via[1].via[T1CL] = xvalue; // 4 T1LC actually writes to T1LL only
        via[1].via[T1LL] = xvalue;
        via[1].t1_e = get_via_te_from_timer((via[1].via[T1LH] << 8) | via[1].via[T1LL]);
        via[1].last_port = port;

        FIX_CLKSTOP_VIA_T1(1); // update cpu68k_clocks_stop if needed

        // #ifdef DEBUG
        via[1].t1_set_cpuclk = cpu68k_clocks;
        // #endif
        via_running = 1;
        DEBUG_LOG(0, "ll-t1clk:%d T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((via[1].via[T1LH] << 8) | via[1].via[T1LL]), via[1].t1_e, via[1].t1_e - cpu68k_clocks, cpu68k_clocks);

        return;

    case T1LH1:
        DEBUG_LOG(0, "T1LH1");       // 7 T1HL value copied into T1HL, no transfer to T1CH
        via[1].via[T1LH] = (xvalue); // Set timer1 latch and counter
        via[1].last_port = port;
        via_running = 1;

        VIA_CLEAR_IRQ_T1(1); // clear T1 irq on T1 read low or write high - does this clear the timer? // 2020.11.06 re-enabling this and fixing via #

        // 2005.05.31 - this next line was disabled - should it have been?
        // via[1].t1_e=get_via_te_from_timer((via[1].via[T1LH]<<8)|via[1].via[T1LL]);
        //  VIA_CLEAR_IRQ_T1(1);     // clear T1 irq on T1 read low or write high - does this clear the timer?

        DEBUG_LOG(0, "lh-t1clk:%d (%04x) t1lh1=%02x T1 will now expire at:%ld - %llx cycles from now - clock now:%llx",
                  ((via[1].via[T1LH] << 8) | via[1].via[T1LL]),
                  ((via[1].via[T1LH] << 8) | via[1].via[T1LL]), xvalue,
                  via[1].t1_e,
                  via[1].t1_e - cpu68k_clocks, cpu68k_clocks);

        return;

    case T1CL1: // 4 T1LC actually writes to T1LL only
        DEBUG_LOG(0, "T1CL1");
        via[1].via[T1LL] = xvalue;
        via[1].last_port = port;

        via_running = 1;

        DEBUG_LOG(0, "cl-t1clk:%d (%04x) CL1=%02x T1 will now expire at:%llx - %lld cycles from now - clock now:%llx",
                  ((via[1].via[T1LH] << 8) | via[1].via[T1LL]),
                  ((via[1].via[T1LH] << 8) | via[1].via[T1LL]), xvalue,
                  via[1].t1_e, via[1].t1_e - cpu68k_clocks, cpu68k_clocks);

        return;

    case T1CH1:
        DEBUG_LOG(0, "T1CH1");
        if (via[1].via[ACR] & 128)
            via[1].via[IRB] &= 0x7f; // with ACR7=1, a write to T1C brings PB7 low
        via[1].via[T1LH] = xvalue;   // load into register
        via[1].via[T1CH] = via[1].via[T1LH];
        via[1].via[T1CL] = via[1].via[T1LL]; // 5 T1HC actually writes to T1HL + copies T1LL->T1CL, T1LH->T1CH
        via[1].last_port = port;

        via[1].t1_e = get_via_te_from_timer((via[1].via[T1CH] << 8) | via[1].via[T1LL]); // this one does tell the counter to count down!
        FIX_CLKSTOP_VIA_T1(1);                                                           // update cpu68k_clocks_stop if needed
        via[1].t1_set_cpuclk = cpu68k_clocks;                                            // timer was set right now (at this clock)

        VIA_CLEAR_IRQ_T1(1); // clear T1 irq on T1 read low or write high

        DEBUG_LOG(0, "ch-t1clk:%d (%04x) CH1=%02x T1 will now expire at:%ld - %llx cycles from now - clock now:%llx",
                  ((via[1].via[T1LH] << 8) | via[1].via[T1LL]),
                  ((via[1].via[T1LH] << 8) | via[1].via[T1LL]), xvalue,
                  via[1].t1_e, via[1].t1_e - cpu68k_clocks, cpu68k_clocks);

        via_running = 1;

        return;

    case T2CL1: // Set Timer 2 Low Latch
        DEBUG_LOG(0, "T2CL1");
        via[1].via[T2LL] = xvalue; // Set t2 latch low
        via[1].last_port = port;

        // #ifdef DEBUG
        via[1].t2_set_cpuclk = cpu68k_clocks;
        DEBUG_LOG(0, "t2clk:%d (write takes effect on write to T2H) T2 set to expire at CPU clock:%llx time now is %llx which is %llx cycles from now",
                  ((via[1].via[T2LH] << 8) | via[1].via[T2CL]), via[1].t2_e, cpu68k_clocks, via[1].t2_e - cpu68k_clocks);
        // #endif
        via_running = 1;
        chk_sound_play();
        return;

    case T2CH1: // timer2 high byte
                // a write to T2H will load and the counter as well.
        DEBUG_LOG(0, "T2CH1");

        via[1].via[T2LH] = xvalue; // Set t2 latch high
        via[1].via[T2CH] = xvalue;
        via[1].via[T2CL] = via[1].via[T2LL]; // load the counter with latches
        via[1].last_port = port;

        VIA_CLEAR_IRQ_T2(1);                                                             // clear T2 irq on T1 read low or write high
        via[1].t2_e = get_via_te_from_timer((via[1].via[T2LH] << 8) | via[1].via[T2CL]); // set timer expiration
        FIX_CLKSTOP_VIA_T2(1);                                                           // update cpu68k_clocks_stop if needed
        via[1].t2_set_cpuclk = cpu68k_clocks;                                            // timer was set right now (at this clock)

        DEBUG_LOG(0, "t2clk:%d T2 set to expire at CPU clock:%llx time now is %llx which is %llx cycles from now",
                  ((via[1].via[T2LH] << 8) | via[1].via[T2CL]), via[1].t2_e, cpu68k_clocks, via[1].t2_e - cpu68k_clocks);

        via_running = 1;
        chk_sound_play();
        return;

    case SR1:
    { /*
       * If the VIA1 SR has been written to by any value other than zero,
       * the Lisa is trying to beep.  T2Low is the Frequency.
       * The lisa will continue to play tones until SR=0 or ACR1
       * disables free run on T2.
       */
        uint8 shift = (via[1].via[ACR] & 28) >> 2;

        DEBUG_LOG(0, "SR1");
        via[1].via[SHIFTREG] = xvalue; // write shift register
        via[1].last_port = port;
        via_running = 1;
        via[1].srcount = 0;
        VIA_CLEAR_IRQ_SR(1); // clear SR irq on SR access

        // now if this has really be done, then we need to know if actual
        // data is in here or zero's or ones.

        // make sure we're not writing all 1's or all 0's, AND the timer is set, AND set to free run.
        // if (shift==4) {} - free running mode from T2 - timing tied to T2
        // if (shift==5) {} - one shot T2 mode - timing tied to T2
        if (shift == 6)
        {
            via[1].sr_e = cpu68k_clocks + VIACLK_TO_CPUCLK(8);
            get_next_timer_event();
        }
        // if (shift==6)    {via[1].sr_e=cpu68k_clocks+8*via_clock_diff; get_next_timer_event();}
        // if (shift==7) - not implemented since CB1 is not connected on Lisa I/O board

        chk_sound_play();
        return;
    }
    case ACR1:

        DEBUG_LOG(0, "ACR1 Port A Latching:%s  Port B Latching:%s", ((xvalue & BIT0) ? "Enabled" : "Disabled"), ((xvalue & BIT1) ? "Enabled" : "Disabled"));
#ifdef DEBUG

        switch (xvalue & (BIT4 | BIT3 | BIT2) >> 2)
        {
        case 0:
            DEBUG_LOG(0, "ACR1 SHIFT REG DISABLED");
            break;
        case 1:
            DEBUG_LOG(0, "ACR1 SR SHIFT IN UNDER COMTROL OF T2");
            break;
        case 2:
            DEBUG_LOG(0, "ACR1 SR SHIFT IN UNDER CONTROL OF 02");
            break;
        case 3:
            DEBUG_LOG(0, "ACR1 SR SHIFT IN UNDER CONTROL OF EXT.CLK");
            break;
        case 4:
            DEBUG_LOG(0, "ACR1 SR SHIFT OUT FREE-RUNNING AT T2 RATE");
            break;
        case 5:
            DEBUG_LOG(0, "ACR1 SR SHIFT OUT UNDER CONTROL OF T2");
            break;
        case 6:
            DEBUG_LOG(0, "ACR1 SR SHIFT OUT UNDER CONTROL OF 02");
            break;
        case 7:
            DEBUG_LOG(0, "ACR1 SR SHIFT OUT UNDER CONTROL OF EXT.CLK");
            break;
        }

        DEBUG_LOG(0, "ACR1 Timer2 Control: %s", ((xvalue & BIT6) ? "Countdown with PB6 Pulses" : "Timed IRQ"));

        switch (xvalue & (BIT7 | BIT6) >> 6)
        {
        case 0:
            DEBUG_LOG(0, "ACR1 Timer1 Control: TIMED IRQ Each Time T1 Loaded, PB7 Output Disabled");
            break;
        case 1:
            DEBUG_LOG(0, "ACR1 Timer1 Control: CONTINUOUS IRQ's, PB7 Output Disabled");
            break;
        case 2:
            DEBUG_LOG(0, "ACR1 Timer1 Control: TIMED IRQ Each Time T1 is loaded, PB7 - One Shot Output");
            break;
        case 3:
            DEBUG_LOG(0, "ACR1 Timer1 Control: Continous IRQ's, PB7: SQUARE WAVE OUTPUT");
            break;
        }
#endif
        via[1].via[ACR] = xvalue;
        via[1].last_port = port;
        chk_sound_play();
        return;

    case PCR1:
        DEBUG_LOG(0, "PCR1: ca1 irq ctl:%s active edge", ((xvalue & BIT0) ? "1:positive" : "0:negative"));
#ifdef DEBUG
        switch ((xvalue & 15) >> 1)
        {
        case 0:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
            break;
        case 1:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT Input NEG. Active Edge");
            break;
        case 2:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
            break;
        case 3:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT INPUT POSITIVE Active Edge");
            break;
        case 4:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: HANDSHAKE OUTPUT");
            break;
        case 5:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: PULSE OUTPUT");
            break;
        case 6:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: LOW OUTPUT");
            break;
        case 7:
            DEBUG_LOG(0, "PCR1: CA2 INTERRUPT CONTROL: HIGH OUTPUT");
            break;
        }
        DEBUG_LOG(0, "PCR1: cb1 irq ctl:%s active edge", ((xvalue & BIT4) ? "1:positive" : "0:negative"));
        switch ((xvalue & (BIT7 | BIT6 | BIT5)) >> 5)
        {
        case 0:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
            break;
        case 1:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT Input NEG. Active Edge");
            break;
        case 2:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
            break;
        case 3:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT INPUT POSITIVE Active Edge");
            break;
        case 4:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: HANDSHAKE OUTPUT");
            break;
        case 5:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: PULSE OUTPUT");
            break;
        case 6:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: LOW OUTPUT");
            break;
        case 7:
            DEBUG_LOG(0, "PCR1: CB2 INTERRUPT CONTROL: HIGH OUTPUT");
            break;
        }

#endif
        if ((via[1].via[PCR] ^ xvalue) & 1)
            via[1].via[IFR] |= VIA_IRQ_BIT_CA1;
        via[1].via[PCR] = xvalue;
        via[1].last_port = port;
        return;

    case IFR1:                                       /* IFR  */
        via[1].via[IFR] &= (0x7f ^ (xvalue & 0x7f)); // 1 writes to IFR are used to clear bits!
        via[1].last_port = port;
        FIX_VIA_IFR(1);

        DEBUG_LOG(0, "VIA1 IFR Write:%02x::%s %s %s %s %s %s %s %s\n", xvalue,
                  (xvalue & VIA_IRQ_BIT_CA2) ? "CA2" : "",
                  (xvalue & VIA_IRQ_BIT_CA1) ? "CA1" : "",
                  (xvalue & VIA_IRQ_BIT_SR) ? "SR " : "",
                  (xvalue & VIA_IRQ_BIT_CB2) ? "CB2" : "",
                  (xvalue & VIA_IRQ_BIT_CB1) ? "CB1" : "",
                  (xvalue & VIA_IRQ_BIT_T2) ? "T2 " : "",
                  (xvalue & VIA_IRQ_BIT_T1) ? "T1 " : "",
                  (xvalue & VIA_IRQ_BIT_SET_CLR_ANY) ? "ANY" : "");

        return;

    case IER1:
        /* IER  */
        DEBUG_LOG(0, "IER1");

        // if bit 7=0, then all 1 bits are reversed. 1=no irq, 0=irq enabled.
        if (xvalue & 0x80)
            via[1].via[IER] |= xvalue;
        else
            via[1].via[IER] &= (0x7f ^ (xvalue & 0x7f));
        via[1].last_port = port;

        DEBUG_LOG(0, "VIA1 IER Write:%02x::%s %s %s %s %s %s %s %s\n", xvalue,
                  (xvalue & VIA_IRQ_BIT_CA2) ? "CA2" : "",
                  (xvalue & VIA_IRQ_BIT_CA1) ? "CA1" : "",
                  (xvalue & VIA_IRQ_BIT_SR) ? "SR " : "",
                  (xvalue & VIA_IRQ_BIT_CB2) ? "CB2" : "",
                  (xvalue & VIA_IRQ_BIT_CB1) ? "CB1" : "",
                  (xvalue & VIA_IRQ_BIT_T2) ? "T2 " : "",
                  (xvalue & VIA_IRQ_BIT_T1) ? "T1 " : "",
                  (xvalue & VIA_IRQ_BIT_SET_CLR_ANY) ? "SET" : "CLEAR");

        DEBUG_LOG(0, "VIA1 IER Value:%02x::%s %s %s %s %s %s %s %s\n", via[1].via[IER],
                  (via[1].via[IER] & VIA_IRQ_BIT_CA2) ? "CA2" : "",
                  (via[1].via[IER] & VIA_IRQ_BIT_CA1) ? "CA1" : "",
                  (via[1].via[IER] & VIA_IRQ_BIT_SR) ? "SR " : "",
                  (via[1].via[IER] & VIA_IRQ_BIT_CB2) ? "CB2" : "",
                  (via[1].via[IER] & VIA_IRQ_BIT_CB1) ? "CB1" : "",
                  (via[1].via[IER] & VIA_IRQ_BIT_T2) ? "T2 " : "",
                  (via[1].via[IER] & VIA_IRQ_BIT_T1) ? "T1 " : "",
                  (via[1].via[IER] & VIA_IRQ_BIT_SET_CLR_ANY) ? "SET" : "CLEAR");

        return;
    default:
        DEBUG_LOG(0, "Illegal I/O addr Error %08x ignored (%02x not writen!)", addr, xvalue);
        return;
    }
    return;
}

// lisa_rb_Oxdc00_cops_via1
uint8 lisa_rb_Oxdc00_cops_via1(uint32 addr)
{
    via[1].active = 1; // these are always active as they're on the
    via[2].active = 1; // motherboard of the machine...

    DEBUG_LOG(0, "reading from register %d (%s)", ((addr & 0x1f) >> 1), via_regname(((addr & 0x1f) >> 1)));

    uint8 port = addr & 0x1f;
    switch (port)
    {
    case IRB1:
        VIA_CLEAR_IRQ_PORT_B(1);
        DEBUG_LOG(0, "IRB1"); // 20060105 | for previously used outputs

        via[1].via[IRBB] = via1_irb(0); // read full byte into shadow

        // now =        input shadow       input enabled bits       |  output shadow       output enabled bits
        via[1].via[IRB] = (via[1].via[IRBB] & (0xff ^ via[1].via[DDRB])) | (via[1].via[ORBB] & via[1].via[DDRB]);
        via[1].last_port = port;

        return via[1].via[IRB];

    case IRANH1:
        VIA_CLEAR_IRQ_PORT_A(1); // was ORANH1
        via[1].last_a_accs = 0;
        DEBUG_LOG(0, "IRANH1");

        via[1].via[IRAA] = via1_ira(15); // read full byte into shadow

        // now =        input shadow        input enabled bits       |  output shadow      output enabled bits
        via[1].via[IRA] = (via[1].via[IRAA] & (~via[1].via[DDRA])) | (via[1].via[ORAA] & via[1].via[DDRA]); // read data from port base it on DDRA mask
        via[1].last_port = port;
        // if needed insert code to not do handshaking here
        return via[1].via[IRA];

    case IRA1:
        via[1].last_a_accs = 0;
        VIA_CLEAR_IRQ_PORT_A(1);
        DEBUG_LOG(0, "IRA1"); // 20060105 | previously used outputs

        via[1].via[IRAA] = via1_ira(15);                                                                    // read full byte into shadow
        via[1].via[IRA] = (via[1].via[IRAA] & (~via[1].via[DDRA])) | (via[1].via[ORAA] & via[1].via[DDRA]); // read data from port base it on DDRA mask
        via[1].last_port = port;
        return via[1].via[IRA];

    case DDRB1:
        DEBUG_LOG(0, "DDRB1");
        via[1].last_port = port;
        return via[1].via[DDRB]; // DDRB
    case DDRA1:
        DEBUG_LOG(0, "DDRA1");
        via[1].last_port = port;
        return via[1].via[DDRA]; // DDRA

    case T1LL1: // Timer 1 Low Order Latch
        DEBUG_LOG(0, "T1LL1");
        VIA_CLEAR_IRQ_T1(1); // clear T1 irq on T1 read low or write high - does this clear the timer? // 2020.11.06 re-enabling this and fixing via #
        via[1].last_port = port;

        return (via[1].via[T1LL]);

    case T1LH1: // Time 1 High Order Latch
        DEBUG_LOG(0, "T1LH1");
        via[1].last_port = port;
        return via[1].via[T1LH];

    case T1CL1: // Timer 1 Low Order Counter
        DEBUG_LOG(0, "T1CL1");
        VIA_CLEAR_IRQ_T1(1); // clear T1 irq on T1 read low or write high
        // via[1].via[T1CL]=get_via_timer_left_from_te(via[1].t1_e) & 0xff;

        via[1].via[T1CL] = get_via_timer_left_or_passed_from_te(via[1].t1_e, via[1].t1_set_cpuclk) & 0xff;
        via[1].last_port = port;
        return via[1].via[T1CL];

    case T1CH1: // Timer 1 High Order Counter
        DEBUG_LOG(0, "T1CH1");
        VIA_CLEAR_IRQ_T1(1); // clear timer 1 interrupt on access
        via[1].via[T1CH] = get_via_timer_left_or_passed_from_te(via[1].t1_e, via[1].t1_set_cpuclk) >> 8;
        via[1].last_port = port;
        return via[1].via[T1CH];

    case T2CL1: // Timer 2 Low Byte
        DEBUG_LOG(0, "read T2CL1 t2_e:%ld set at:%ld", via[1].t2_e, via[1].t2_set_cpuclk);

        via[1].via[T2CL] = get_via_timer_left_or_passed_from_te(via[1].t2_e, via[1].t2_set_cpuclk) & 0xff;
        via[1].last_port = port;

        // via[1].via[IFR] &=0xDF;                                    // clear T2 Interrupt from IFR  (bit 5) */
        VIA_CLEAR_IRQ_T2(1); // clear T2 irq on T1 read low or write high

        return (via[1].via[T2CL]);

    case T2CH1:
        DEBUG_LOG(0, "T2CH1");
        via[1].via[T2CH] = get_via_timer_left_or_passed_from_te(via[1].t2_e, via[1].t2_set_cpuclk) >> 8;
        via[1].last_port = port;
        return via[1].via[T2CH];

    case SR1:
        VIA_CLEAR_IRQ_SR(1); // clear SR irq on SR access
        DEBUG_LOG(0, "SR1");
        via[1].last_port = port;
        return via[1].via[SHIFTREG];

    case ACR1:
        DEBUG_LOG(0, "ACR1");
        via[1].last_port = port;
        return via[1].via[ACR];
    case PCR1:
        DEBUG_LOG(0, "PCR1");
        via[1].last_port = port;
        return via[1].via[PCR];

    case IFR1:
        DEBUG_LOG(0, "IFR COPS: SRC: copsqueuelen=%d", copsqueuelen);
        // via1_ifr bit1 (2) is data in cops queue indicator

        if (copsqueuelen > 0 || (mousequeuelen > 0 && cops_mouse > 0))
            via[1].via[IFR] |= VIA_IRQ_BIT_CA1;
        else
            via[1].via[IFR] &= ~VIA_IRQ_BIT_CA1;
        via[1].last_port = port;

#ifdef DEBUG
        DEBUG_LOG(0, "COPS-IFR SRC: returning %02x, copsqueuelen:%d, mousequeuelen:%d", via[1].via[IFR], copsqueuelen, mousequeuelen);
        if (pc24 < 0xfe0000)
            DEBUG_LOG(0, "VIA1 IFR read:%02x at PC:%d/%08x::ifr0:%s ifr1:%s ifr2:%s ifr3:%s ifr4:%s ifr5:%s ifr6:%s ifr7:%s\n", via[1].via[IFR], context, pc24,
                      (via[1].via[IFR] & VIA_IRQ_BIT_CA2) ? "CA2:on" : "CA2:off",
                      (via[1].via[IFR] & VIA_IRQ_BIT_CA1) ? "CA1:on" : "CA1:off",
                      (via[1].via[IFR] & VIA_IRQ_BIT_SR) ? "SR :on" : "SR :off",
                      (via[1].via[IFR] & VIA_IRQ_BIT_CB2) ? "CB2:on" : "CB2:off",
                      (via[1].via[IFR] & VIA_IRQ_BIT_CB1) ? "CB1:on" : "CB1:off",
                      (via[1].via[IFR] & VIA_IRQ_BIT_T2) ? "T2 :on" : "T2 :off",
                      (via[1].via[IFR] & VIA_IRQ_BIT_T1) ? "T1 :on" : "T1 :off",
                      (via[1].via[IFR] & VIA_IRQ_BIT_SET_CLR_ANY) ? "ANY:on" : "ANY:off");
#endif

        FIX_VIA_IFR(1);
        return via[1].via[IFR];

    case IER1:
        DEBUG_LOG(0, "IER1");

        if (pc24 < 0xfe0000)
            DEBUG_LOG(0, "VIA1 IER read:%02x::ier0:%s ier1:%s ier2:%s ier3:%s ier4:%s ier5:%s ier6:%s ier7:%s\n", via[1].via[IFR],
                      (via[1].via[IER] & VIA_IRQ_BIT_CA2) ? "CA2:on" : "CA2:off",
                      (via[1].via[IER] & VIA_IRQ_BIT_CA1) ? "CA1:on" : "CA1:off",
                      (via[1].via[IER] & VIA_IRQ_BIT_SR) ? "SR :on" : "SR :off",
                      (via[1].via[IER] & VIA_IRQ_BIT_CB2) ? "CB2:on" : "CB2:off",
                      (via[1].via[IER] & VIA_IRQ_BIT_CB1) ? "CB1:on" : "CB1:off",
                      (via[1].via[IER] & VIA_IRQ_BIT_T2) ? "T2 :on" : "T2 :off",
                      (via[1].via[IER] & VIA_IRQ_BIT_T1) ? "T1 :on" : "T1 :off",
                      (via[1].via[IER] & VIA_IRQ_BIT_SET_CLR_ANY) ? "ANY:on" : "ANY:off");

        via[1].via[IER] |= 0x80; // docs says bit 7 is always logical 1
        via[1].last_port = port;
        return via[1].via[IER];

    default:
        DEBUG_LOG(0, "Illegal addr error:%08x", addr);
        return 0xff;
    }
    return 0xff;
}

void lisa_wb_Oxd800_par_via2(uint32 addr, uint8 xvalue)
{

    via[1].active = 1; // these are always active as they're on the
    via[2].active = 1; // motherboard of the machine...

#ifdef DEBUG
    if (via[2].ProFile)
        DEBUG_LOG(0, "profile.c:State:%d VIA:2 writing to register %d (%s)", via[2].ProFile->StateMachineStep, ((addr & 0x7f) >> 3), via_regname(((addr & 0x7f) >> 3)));
#else
    DEBUG_LOG(0, "writing %02x to register %d (%s) @%08x", xvalue, (addr & 0x79) / 8, via_regname((addr & 0x79) / 8), addr);
#endif

    uint8 port = addr & 0x79; // fcd901      // was 0x7f it's now 0x79  // 2004.06.24 because saw MOVEP.W to T2CH, but suspect it also writes to T2CL!
    switch (port)
    {
    case ORB2: // IRB/ORB
    {
#ifdef DEBUG
        uint8 flipped = (via[2].via[ORBB] ^ xvalue) & via[2].via[DDRB];

        DEBUG_LOG(0, "Writing VIA2 ORB. DDRB mask=%02x:(%s%s%s%s%s%s%s%s)", via[2].via[DDRB],
                  ((via[2].via[DDRB] & 1) ? "OCD:PB0:out " : "OCD:pb0:in  "),
                  ((via[2].via[DDRB] & 2) ? "BSY:PB1:out " : "BSY:pb1:in  "),
                  ((via[2].via[DDRB] & 4) ? "DEN:PB2:out " : "DEN:pb2:in  "),
                  ((via[2].via[DDRB] & 8) ? "RRW:PB3:out " : "RRW:pb3:in  "),
                  ((via[2].via[DDRB] & 16) ? "CMD:PB4:out " : "CMD:pb4:in  "),
                  ((via[2].via[DDRB] & 32) ? "PAR:PB5:out " : "PAR:pb5:in  "),
                  ((via[2].via[DDRB] & 64) ? "DSK:PB6:out " : "DSK:pb6:in  "),
                  ((via[2].via[DDRB] & 128) ? "CRS:PB7:out " : "CRS:pb7:in  "));

        DEBUG_LOG(0, "ORB bits Flipped  :(%s%s%s%s%s%s%s%s)",
                  ((flipped & 1) ? "OCD:PB0 " : " "),
                  ((flipped & 2) ? "BSY:PB1 " : " "),
                  ((flipped & 4) ? "DEN:PB2 " : " "),
                  ((flipped & 8) ? "RRW:PB3 " : " "),
                  ((flipped & 16) ? "CMD:PB4 " : " "),
                  ((flipped & 32) ? "PAR:PB5 " : " "),
                  ((flipped & 64) ? "DSK:PB6 " : " "),
                  ((flipped & 128) ? "CRS:PB7 " : " "));

        if (flipped & 16)
            DEBUG_LOG(0, "profile.c:widget.c: >%sCMD Flipped to %d at PC24:%08x  tag:profile step:%d   pc24:%08x",
                      (xvalue & 16 ? "+" : "!"), xvalue & 16, pc24, via[2].ProFile->StateMachineStep, pc24);

        DEBUG_LOG(0, "VIA2 ORB write=%02x:(OCD:%s BSY:%s DEN:%s RRW: %sCMD:%s PAR:%s DSKDIAG:%s CRES:%s)", xvalue,
                  ((xvalue & 1) ? "OCD:PB0:1 " : "OCD:PB0:0 "),
                  ((xvalue & 2) ? "BSY:PB1:1 " : "BSY:PB1:0 "),
                  ((xvalue & 4) ? "DEN:PB2:1 " : "DEN:PB2:0 "),
                  ((xvalue & 8) ? "RRW:PB3:1 " : "RRW:PB3:0 "),
                  ((xvalue & 16) ? "CMD:PB4:1 " : "CMD:PB4:0 "),
                  ((xvalue & 32) ? "PAR:PB5:1 " : "PAR:PB5:0 "),
                  ((xvalue & 64) ? "DSK:PB6:1 " : "DSK:PB6:0 "),
                  ((xvalue & 128) ? "CRS:PB7:1 " : "CRS:PB7:0 "));

#endif

        via[2].via[ORBB] = xvalue; // set the shadow register to the unmasked data for later use.
        via[2].via[ORB] = (via[2].via[IRBB] & (~via[2].via[DDRB])) | (via[2].via[ORBB] & via[2].via[DDRB]);
        via[2].last_port = port;

        VIA_CLEAR_IRQ_PORT_B(2); // clear Cb1/Cb2 on ORb/IRb access

        if (!via[2].via[DDRB])
            return;                                    // don't write anything (no handshake) if the DDRB is all inputs
        via2_orb(via[2].via[ORBB] & via[2].via[DDRB]); // output the masked data.

        return;
    }
    case ORANH2: // IRA/ORA
        DEBUG_LOG(0, "ORANH2");

        via[2].last_a_accs = 1;
        if (via[2].ProFile)
            via[2].ProFile->last_a_accs = 1;

        // Note (TorZidan@): I commented out this code as I think it's wrong (it belongs in the "case ORA2" below).
        // Once we confirm that things are running smoothly, we can remove this. See more in the docs for variable "last_port".
        // if (via[2].via[ORA] == xvalue)
        // {
        //     DEBUG_LOG(0, "ORANH2: Already wrote %02 to port, not pushing it", xvalue);
        //     return; // already what's on the port, ignore the write //20060418
        // }

        via[2].via[ORAA] = xvalue; // set the shadow register to the unmasked data for later use.
        via[2].via[ORA] = (via[2].via[IRAA] & (~via[2].via[DDRA])) | (via[2].via[ORAA] & via[2].via[DDRA]);
        via[2].last_port = port;

        if (!via[2].via[DDRA])
        { // don't write anything if the DDRA is all inputs
            via[2].orapending = 1;
            DEBUG_LOG(0, "ORANH: Not writing %02x now, but setting pending since DDRA=0", xvalue);
            return;
        }

        via2_ora(via[2].via[ORAA] & via[2].via[DDRA], 15);
        // via[2].orapending=1;              //20060417//

        return;

    case ORA2:
        DEBUG_LOG(0, "ORA2");
        via[2].last_a_accs = 1;
        if (via[2].ProFile)
            via[2].ProFile->last_a_accs = 1;

        if (via[2].via[ORA] == xvalue && via[2].last_port == ORANH2)
        {
            DEBUG_LOG(0, "in ORA2: Already wrote %02x to ORANH2, not pushing it again.\n", xvalue);
            via[2].last_port = port;
            return; // already what's on the port, ignore the write
        }

        VIA_CLEAR_IRQ_PORT_A(2); // clear CA1/CA2 on ORA/IRA access

        via[2].via[ORA] = (via[2].via[IRAA] & (~via[2].via[DDRA])) | (via[2].via[ORAA] & via[2].via[DDRA]);

        via[2].via[ORAA] = xvalue;
        via[2].last_port = port;

        if (!via[2].via[DDRA])
        { // don't write anything if the DDRA is all inputs
            via[2].orapending = 1;
            return;
        }

        via2_ora(via[2].via[ORAA] & via[2].via[DDRA], ORA); // output the masked data
        via[2].orapending = 0;
        return;

    case DDRB2: // DDRB 2
        DEBUG_LOG(0, "DDRB2");
        via[2].via[DDRB] = xvalue;

        DEBUG_LOG(0, "Writing VIA2 DDRB mask=%02x:(%s%s%s%s%s%s%s%s)", via[2].via[DDRB],
                  ((via[2].via[DDRB] & 1) ? "OCD:PB0:out " : "OCD:pb0:in  "),
                  ((via[2].via[DDRB] & 2) ? "BSY:PB1:out " : "BSY:pb1:in  "),
                  ((via[2].via[DDRB] & 4) ? "DEN:PB2:out " : "DEN:pb2:in  "),
                  ((via[2].via[DDRB] & 8) ? "RRW:PB3:out " : "RRW:pb3:in  "),
                  ((via[2].via[DDRB] & 16) ? "CMD:PB4:out " : "CMD:pb4:in  "),
                  ((via[2].via[DDRB] & 32) ? "PAR:PB5:out " : "PAR:pb5:in  "),
                  ((via[2].via[DDRB] & 64) ? "DSK:PB6:out " : "DSK:pb6:in  "),
                  ((via[2].via[DDRB] & 128) ? "CRS:PB7:out " : "CRS:pb7:in  "));

        if (xvalue)                                        // only try the write if the mask has changed
            via2_orb(via[2].via[ORBB] & via[2].via[DDRB]); // output the data that was masked off
        via[2].last_port = port;
        return;

    case DDRA2: // and DDRA 3 registers

        DEBUG_LOG(0, "Overwriting DDRA (previous value was=%02x, new value is:%02x)",
                  via[2].via[DDRA], xvalue);

        via[2].via[DDRA] = xvalue;
        via[2].last_port = port;

        DEBUG_LOG(0, "Wrote DDRA mask=%02x:(%s%s%s%s%s%s%s%s)", via[2].via[DDRA],
                  ((via[2].via[DDRA] & 1) ? "PA0:out " : "pb0:in  "),
                  ((via[2].via[DDRA] & 2) ? "PA1:out " : "pb1:in  "),
                  ((via[2].via[DDRA] & 4) ? "PA2:out " : "pb2:in  "),
                  ((via[2].via[DDRA] & 8) ? "PA3:out " : "pb3:in  "),
                  ((via[2].via[DDRA] & 16) ? "PA4:out " : "pb4:in  "),
                  ((via[2].via[DDRA] & 32) ? "PA5:out " : "pb5:in  "),
                  ((via[2].via[DDRA] & 64) ? "PA6:out " : "pb6:in  "),
                  ((via[2].via[DDRA] & 128) ? "PA7:out " : "pb7:in  "));

        if (via[2].orapending && xvalue != 0) // only try to write if this mask changed and last access to A was a write
        {
            DEBUG_LOG(0, "via[2].orapending=%02x, DDRA=%02x, last write was:%02x",
                      via[2].orapending,
                      xvalue,
                      via[2].via[ORAA]);

            via2_ora((via[2].via[ORAA] & xvalue), ORA); // output the masked data
        }
        return;

    case T1LL2: // Timer 1 Low Order Latch
        DEBUG_LOG(0, "T1LL2");

        via[2].via[T1CL] = xvalue; // 4 T1LC actually writes to T1LL only
        via[2].via[T1LL] = xvalue;
        via[2].t1_e = get_via_te_from_timer((via[2].via[T1LH] << 8) | via[2].via[T1LL]);
        via[2].last_port = port;

        FIX_CLKSTOP_VIA_T1(2); // update cpu68k_clocks_stop if needed

        // #ifdef DEBUG
        via[2].t1_set_cpuclk = cpu68k_clocks;
        // #endif
        via_running = 1;
        DEBUG_LOG(0, "ll-t1clk:%d T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((via[2].via[T1LH] << 8) | via[2].via[T1LL]), via[2].t1_e, via[2].t1_e - cpu68k_clocks, cpu68k_clocks);

        return; // Set timer2 low byte

    case T1LH2:
        DEBUG_LOG(0, "T1LH2");
        via[2].via[T1LH] = (xvalue); // Set timer1 latch and counter
        via[2].last_port = port;
        via_running = 1;
        // 2005.05.31 - this next line was disabled - should it have been?
        // via[1].t1_e=get_via_te_from_timer((via[1].via[T1LH]<<8)|via[1].via[T1LL]);
        VIA_CLEAR_IRQ_T1(2); // clear T1 irq on T1 read low or write high - does this clear the timer? // 2020.11.06 re-enabling this and fixing via #

        DEBUG_LOG(0, "lh-t1clk:%d (%04x) t1lh1=%02x T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((via[2].via[T1LH] << 8) | via[2].via[T1LL]),
                  ((via[2].via[T1LH] << 8) | via[2].via[T1LL]), xvalue,
                  via[2].t1_e,
                  via[2].t1_e - cpu68k_clocks, cpu68k_clocks);

        return;

    case T1CL2:
        DEBUG_LOG(0, "T1CL2");

        via[2].via[T1LL] = xvalue;
        via[2].last_port = port;

        via_running = 1;

        DEBUG_LOG(0, "cl-t1clk:%d (%04x) CL1=%02x T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((via[2].via[T1LH] << 8) | via[2].via[T1LL]),
                  ((via[2].via[T1LH] << 8) | via[2].via[T1LL]), xvalue,
                  via[2].t1_e, via[2].t1_e - cpu68k_clocks, cpu68k_clocks);
        return;

    case T1CH2:
        DEBUG_LOG(0, "T1CH2");
        if (via[2].via[ACR] & 128)
            via[2].via[IRB] &= 0x7f; // with ACR7=1, a write to T1C brings PB7 low
        via[2].via[T1LH] = xvalue;   // load into register
        via[2].via[T1CH] = via[2].via[T1LH];
        via[2].via[T1CL] = via[2].via[T1LL]; // 5 T1HC actually writes to T1HL + copies T1LL->T1CL, T1LH->T1CH

        via[2].t1_e = get_via_te_from_timer((via[2].via[T1CH] << 8) | via[2].via[T1LL]); // this one does tell the counter to count down!
        FIX_CLKSTOP_VIA_T1(2);                                                           // update cpu68k_clocks_stop if needed
        via[2].t1_set_cpuclk = cpu68k_clocks;                                            // timer was set right now (at this clock)
        via[2].last_port = port;

        VIA_CLEAR_IRQ_T1(2); // clear T1 irq on T1 read low or write high

        DEBUG_LOG(0, "ch-t1clk:%d (%04x) CH1=%02x T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((via[2].via[T1LH] << 8) | via[2].via[T1LL]),
                  ((via[2].via[T1LH] << 8) | via[2].via[T1LL]), xvalue,
                  via[2].t1_e, via[2].t1_e - cpu68k_clocks, cpu68k_clocks);

        via_running = 1;
        return;

    case T2CL2: // Set Timer 2 Low Latch
        DEBUG_LOG(0, "T2CL2");
        via[2].via[T2LL] = xvalue; // Set t2 latch low

        // #ifdef DEBUG
        via[2].t2_set_cpuclk = cpu68k_clocks;
        via[2].last_port = port;
        DEBUG_LOG(0, "t2clk:%d (write takes effect on write to T2H) T2 set to expire at clock:%llx time now is %llx which is %llx cycles from now",
                  ((via[2].via[T2LH] << 8) | via[2].via[T2CL]), via[2].t2_e, cpu68k_clocks, via[2].t2_e - cpu68k_clocks);
        // #endif
        via_running = 1;
        return;

    case T2CH2: // timer2 high byte
                // a write to T2H will load and the counter as well.
        DEBUG_LOG(0, "T2CH2");

        via[2].via[T2LH] = xvalue; // Set t2 latch high
        via[2].via[T2CH] = xvalue;
        via[2].via[T2CL] = via[2].via[T2LL]; // load the counter with latches
        via[2].last_port = port;

        VIA_CLEAR_IRQ_T2(2); // clear T2 irq on T2 read low or write high

        via[2].t2_e = get_via_te_from_timer((via[2].via[T2CH] << 8) | via[2].via[T2CL]); // set timer expiration
        DEBUG_LOG(0, "via[2].t2_e=%ld", via[2].t2_e);
        FIX_CLKSTOP_VIA_T2(2);                // update cpu68k_clocks_stop if needed
        via[2].t2_set_cpuclk = cpu68k_clocks; // timer was set right now (at this clock)

        DEBUG_LOG(0, "t2clk:%d T2 set to expire at CPU clock:%llx time now is %llx which is %llx cycles from now.  stop is:%llx",
                  ((via[2].via[T2LH] << 8) | via[2].via[T2CL]), via[2].t2_e, cpu68k_clocks, via[2].t2_e - cpu68k_clocks, cpu68k_clocks_stop);

        via_running = 1;

        return;

    case SR2:
    {
        // char srwrite[128];             //unused
        uint8 shift = (via[2].via[ACR] & 28) >> 2;

        DEBUG_LOG(0, "Write %02x to shift register on VIA2!!!!", xvalue);
        DEBUG_LOG(0, "SR2");
        via[2].via[SHIFTREG] = xvalue; // write shift register
        via_running = 1;
        via[2].srcount = 0;
        via[2].last_port = port;

        // if (shift==4) {} - free running mode from T2 - timing tied to T2
        // if (shift==5) {} - one shot T2 mode - timing tied to T2
        if (shift == 6)
        {
            via[2].sr_e = cpu68k_clocks + VIACLK_TO_CPUCLK(8);
            get_next_timer_event();
        }
        /// if (shift==6)    {via[2].sr_e=cpu68k_clocks+8*via_clock_diff; get_next_timer_event();}
        // if (shift==7) - not implemented since CB1 is not connected on Lisa I/O board

        VIA_CLEAR_IRQ_SR(2); // clear SR irq on SR access
    }
        return;

    case ACR2:
        DEBUG_LOG(0, "ACR2 Port A Latching:%s  Port B Latching:%s",
                  ((xvalue & BIT0) ? "Enabled" : "Disabled"),
                  ((xvalue & BIT1) ? "Enabled" : "Disabled"));
#ifdef DEBUG
        switch (xvalue & (BIT4 | BIT3 | BIT2) >> 2)
        {
        case 0:
            DEBUG_LOG(0, "ACR2 SHIFT REG DISABLED");
            break;
        case 1:
            DEBUG_LOG(0, "ACR2 SR SHIFT IN UNDER COMTROL OF T2");
            break;
        case 2:
            DEBUG_LOG(0, "ACR2 SR SHIFT IN UNDER CONTROL OF 02");
            break;
        case 3:
            DEBUG_LOG(0, "ACR2 SR SHIFT IN UNDER CONTROL OF EXT.CLK");
            break;
        case 4:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT FREE-RUNNING AT T2 RATE");
            break;
        case 5:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT UNDER CONTROL OF T2");
            break;
        case 6:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT UNDER CONTROL OF 02");
            break;
        case 7:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT UNDER CONTROL OF EXT.CLK");
            break;
        }

        DEBUG_LOG(0, "ACR2 Timer2 Control: %s", ((xvalue & BIT6) ? "Countdown with PB6 Pulses" : "Timed IRQ"));

        switch (xvalue & (BIT7 | BIT6) >> 6)
        {
        case 0:
            DEBUG_LOG(0, "ACR2 Timer1 Control: TIMED IRQ Each Time T1 Loaded, PB7 Output Disabled");
            break;
        case 1:
            DEBUG_LOG(0, "ACR2 Timer1 Control: CONTINUOUS IRQ's, PB7 Output Disabled");
            break;
        case 2:
            DEBUG_LOG(0, "ACR2 Timer1 Control: TIMED IRQ Each Time T1 is loaded, PB7 - One Shot Output");
            break;
        case 3:
            DEBUG_LOG(0, "ACR2 Timer1 Control: Continous IRQ's, PB7: SQUARE WAVE OUTPUT");
            break;
        }
#endif
        via[2].via[ACR] = xvalue;
        via[2].last_port = port;
        return;

    case PCR2:
        DEBUG_LOG(0, "PCR2");
#ifdef DEBUG

        if (xvalue & 0)
        {
            DEBUG_LOG(0, "PCR2: CA1 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
        }
        else
        {
            DEBUG_LOG(0, "PCR2: CA1 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
        }

        switch ((xvalue & 15) >> 1)
        {
        case 0:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
            break;
        case 1:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT Input NEG. Active Edge");
            break;
        case 2:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
            break;
        case 3:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT INPUT POSITIVE Active Edge");
            break;
        case 4:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: HANDSHAKE OUTPUT");
            break;
        case 5:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: PULSE OUTPUT");
            break;
        case 6:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: LOW OUTPUT");
            break;
        case 7:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: HIGH OUTPUT");
            break;
        }
        DEBUG_LOG(0, "PCR2: cb1 irq ctl:%s active edge", ((xvalue & BIT4) ? "1:positive" : "0:negative"));
        switch ((xvalue & (BIT7 | BIT6 | BIT5)) >> 5)
        {
        case 0:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
            break;
        case 1:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT Input NEG. Active Edge");
            break;
        case 2:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
            break;
        case 3:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT INPUT POSITIVE Active Edge");
            break;
        case 4:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: HANDSHAKE OUTPUT");
            break;
        case 5:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: PULSE OUTPUT");
            break;
        case 6:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: LOW OUTPUT");
            break;
        case 7:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: HIGH OUTPUT");
            break;
        }
#endif
        if ((via[2].via[PCR] ^ xvalue) & 1)
            via[2].via[IFR] |= VIA_IRQ_BIT_CA1;
        via[2].via[PCR] = xvalue;
        via[2].last_port = port;
        return;

    case IFR2: /* IFR  */

        DEBUG_LOG(0, "IFR2=%02x before write bits= %s %s %s %s %s %s %s %s", via[2].via[IFR],
                  (via[2].via[IFR] & 1) ? "ifr0CA2:on" : "ifr0CA2:off",
                  (via[2].via[IFR] & 2) ? "bsy_ifr1CA1:on" : "bsy_ifr1CA1:off",
                  (via[2].via[IFR] & 4) ? "ifr2SR :on" : "ifr2SR :off",
                  (via[2].via[IFR] & 8) ? "parity_ifr3CB2:on" : "parity_ifr3CB2:off",
                  (via[2].via[IFR] & 16) ? "not_connected_ifr4CB1:on" : "not_connected_ifr4CB1:off",
                  (via[2].via[IFR] & 32) ? "ifr5T2 :on" : "ifr5T2 :off",
                  (via[2].via[IFR] & 64) ? "ifr6T1 :on" : "ifr6T1 :off",
                  (via[2].via[IFR] & 128) ? "ifr7ANY:on" : "ifr7ANY:off");

        via[2].via[IFR] &= ~(xvalue); // 1 writes to IFR are used to clear bits!
        via[2].last_port = port;
        FIX_VIA_IFR(2);

        DEBUG_LOG(0, "IFR2 write=%02x bits: %s %s %s %s %s %s %s %s", xvalue,
                  (via[2].via[IFR] & 1) ? "ifr0CA2:on" : "ifr0CA2:off",
                  (via[2].via[IFR] & 2) ? "bsy_ifr1CA1:on" : "bsy_ifr1CA1:off",
                  (via[2].via[IFR] & 4) ? "ifr2SR :on" : "ifr2SR :off",
                  (via[2].via[IFR] & 8) ? "parity_ifr3CB2:on" : "parity_ifr3CB2:off",
                  (via[2].via[IFR] & 16) ? "not_connected_ifr4CB1:on" : "not_connected_ifr4CB1:off",
                  (via[2].via[IFR] & 32) ? "ifr5T2 :on" : "ifr5T2 :off",
                  (via[2].via[IFR] & 64) ? "ifr6T1 :on" : "ifr6T1 :off",
                  (via[2].via[IFR] & 128) ? "ifr7ANY:on" : "ifr7ANY:off");
        return;

    case IER2:
        DEBUG_LOG(0, "IER2");
        // if bit 7=0, then all 1 bits are reversed. 1=no irq, 0=irq enabled.

        if (xvalue & 0x80)
            via[2].via[IER] |= xvalue;
        else
            via[2].via[IER] &= (0x7f ^ (xvalue & 0x7f));

        // clear out anything that IER has disabled on the write.  2020.11.06
        via[2].via[IFR] &= via[2].via[IER];
        via[2].last_port = port;
        FIX_VIA_IFR(2);

// from via 1// if bit 7=0, then all 1 bits are reversed. 1=no irq, 0=irq enabled.
/// if   (xvalue & 128) {via[1].via[IER] |= xvalue;}
/// else via[1].via[IER] &=(0x7f-( xvalue & 0x7f));
#ifdef DEBUG
        if (xvalue & VIA_IRQ_BIT_SET_CLR_ANY)
        {
            DEBUG_LOG(0, "VIA2 IER Write Set:%02x::%s %s %s %s %s %s %s %s\n", xvalue,
                      ((xvalue & VIA_IRQ_BIT_CA2) ? "CA2" : ""),
                      ((xvalue & VIA_IRQ_BIT_CA1) ? "CA1" : ""),
                      ((xvalue & VIA_IRQ_BIT_SR) ? "SR " : ""),
                      ((xvalue & VIA_IRQ_BIT_CB2) ? "CB2" : ""),
                      ((xvalue & VIA_IRQ_BIT_CB1) ? "CB1" : ""),
                      ((xvalue & VIA_IRQ_BIT_T2) ? "T2 " : ""),
                      ((xvalue & VIA_IRQ_BIT_T1) ? "T1 " : ""),
                      ((xvalue & VIA_IRQ_BIT_SET_CLR_ANY) ? "SET" : "CLEAR"));
        }
        else
        {
            DEBUG_LOG(0, "VIA2 IER clear Value:%02x::%s %s %s %s %s %s %s %s\n", via[2].via[IER],
                      ((via[2].via[IER] & VIA_IRQ_BIT_CA2) ? "CA2" : ""),
                      ((via[2].via[IER] & VIA_IRQ_BIT_CA1) ? "CA1" : ""),
                      ((via[2].via[IER] & VIA_IRQ_BIT_SR) ? "SR " : ""),
                      ((via[2].via[IER] & VIA_IRQ_BIT_CB2) ? "CB2" : ""),
                      ((via[2].via[IER] & VIA_IRQ_BIT_CB1) ? "CB1" : ""),
                      ((via[2].via[IER] & VIA_IRQ_BIT_T2) ? "T2 " : ""),
                      ((via[2].via[IER] & VIA_IRQ_BIT_T1) ? "T1 " : ""),
                      ((via[2].via[IER] & VIA_IRQ_BIT_SET_CLR_ANY) ? "SET" : "CLEAR"));
        }
#endif
        return;
    default:
        DEBUG_LOG(0, "Illegal address error Write to illegal address in via2_wb_ @ %08x", addr);
    }
    return;
}

uint8 lisa_rb_Oxd800_par_via2(uint32 addr)
{
    via[1].active = 1; // these are always active as they're on the
    via[2].active = 1; // motherboard of the machine...

#ifdef DEBUG
    if (via[2].ProFile)
        DEBUG_LOG(0, "profile.c:State:%d VIA:2 reading from register %d (%s)", via[2].ProFile->StateMachineStep, ((addr & 0x7f) >> 3), via_regname(((addr & 0x7f) >> 3)));
#else
    DEBUG_LOG(0, "reading from register %d (%s)", ((addr & 0x7f) >> 3), via_regname(((addr & 0x7f) >> 3)));
#endif

    // if (via[2].ProFile) VIAProfileLoop(2,via[2].ProFile,PROLOOP_EV_NUL); // 2021.05.24

    uint8 port = addr & 0x79; // was 7f, changing to 79
    switch (port) 
    {
    case IRB2:
    {
        // if (via[2].ProFile) VIAProfileLoop(2,via[2].ProFile,PROLOOP_EV_IRB); // 2021.05.24

#ifdef DEBUG
        uint8 flipped = via[2].via[IRBB];
#endif

        DEBUG_LOG(0, "Reading VIA2 IRB. DDRB mask=%02x:(%s%s%s%s%s%s%s%s)", via[2].via[DDRB],
                  ((via[2].via[DDRB] & 1) ? "OCD:PB0:out " : "OCD:pb0:in  "),
                  ((via[2].via[DDRB] & 2) ? "BSY:PB1:out " : "BSY:pb1:in  "),
                  ((via[2].via[DDRB] & 4) ? "DEN:PB2:out " : "DEN:pb2:in  "),
                  ((via[2].via[DDRB] & 8) ? "RRW:PB3:out " : "RRW:pb3:in  "),
                  ((via[2].via[DDRB] & 16) ? "CMD:PB4:out " : "CMD:pb4:in  "),
                  ((via[2].via[DDRB] & 32) ? "PAR:PB5:out " : "PAR:pb5:in  "),
                  ((via[2].via[DDRB] & 64) ? "DSK:PB6:out " : "DSK:pb6:in  "),
                  ((via[2].via[DDRB] & 128) ? "CRS:PB7:out " : "CRS:pb7:in  "));

        via[2].via[IRBB] = via2_irb();

        DEBUG_LOG(0, "via2_irb() returned %02x", via[2].via[IRBB]);

        via[2].via[IRB] = (via[2].via[IRBB] & (~via[2].via[DDRB])) | (via[2].via[ORBB] & via[2].via[DDRB]);
        via[2].last_port = port;

        DEBUG_LOG(0, "returning irb & !DDRB is %02x", via[2].via[IRB]);

        VIA_CLEAR_IRQ_PORT_B(2); // clear Cb1/Cb2 on ORb/IRb access

#ifdef DEBUG
        flipped ^= via[2].via[IRB];
        DEBUG_LOG(0, "IRB Flipped:(%s%s%s%s%s%s%s%s)",
                  ((flipped & 1) ? "OCD:PB0 " : " "),
                  ((flipped & 2) ? "BSY:PB1 " : " "),
                  ((flipped & 4) ? "DEN:PB2 " : " "),
                  ((flipped & 8) ? "RRW:PB3 " : " "),
                  ((flipped & 16) ? "CMD:PB4 " : " "),
                  ((flipped & 32) ? "PAR:PB5 " : " "),
                  ((flipped & 64) ? "DSK:PB6 " : " "),
                  ((flipped & 128) ? "CRS:PB7 " : " "));

        if (flipped & 2)
            DEBUG_LOG(0, "profile.c:widget.c:<%sBSY flipped to %d   pc24:%08x", ((via[2].via[IRB] & 2) ? "+" : "!"), via[2].via[IRB] & 2, pc24);

        DEBUG_LOG(0, "VIA2 IRB read=%02x:(OCD:%s BSY:%s DEN:%s RRW: %sCMD:%s PAR:%s DSKDIAG:%s CRES:%s)", via[2].via[IRB],
                  ((via[2].via[IRB] & 1) ? "PB0:1 " : "pb0:0 "),
                  ((via[2].via[IRB] & 2) ? "PB1:1 " : "pb1:0 "),
                  ((via[2].via[IRB] & 4) ? "PB2:1 " : "pb2:0 "),
                  ((via[2].via[IRB] & 8) ? "PB3:1 " : "pb3:0 "),
                  ((via[2].via[IRB] & 16) ? "PB4:1 " : "pb4:0 "),
                  ((via[2].via[IRB] & 32) ? "PB5:1 " : "pb5:0 "),
                  ((via[2].via[IRB] & 64) ? "PB6:1 " : "pb6:0 "),
                  ((via[2].via[IRB] & 128) ? "PB7:1 " : "pb7:0 "));

#endif
        return via[2].via[IRB];
    }

    case IRANH2:
        via[2].last_a_accs = 0;
        // if (via[2].ProFile) VIAProfileLoop(2,via[2].ProFile,PROLOOP_EV_IRA); // 2021.05.24

        if (via[2].ProFile)
            via[2].ProFile->last_a_accs = 0;

        via[2].via[IRAA] = (via2_ira(15));
        via[2].via[IRA] = (via[2].via[IRAA] & (~via[2].via[DDRA])) | (via[2].via[ORAA] & via[2].via[DDRA]); // read data from port base it on DDRA mask
        via[2].last_port = port;

        DEBUG_LOG(0, "profile.c:widget.c: IRANH2  Profile to Lisa <%02x  pc24:%08x", via[2].via[IRA], pc24);
        return via[2].via[IRA];

    case IRA2:
        via[2].last_a_accs = 0;
        // if (via[2].ProFile) VIAProfileLoop(2,via[2].ProFile,PROLOOP_EV_IRA); // 2021.05.24

        if (via[2].ProFile)
            via[2].ProFile->last_a_accs = 0;

        VIA_CLEAR_IRQ_PORT_A(2); // clear CA1/CA2 on ORA/IRA access

        via[2].via[IRAA] = (via2_ira(15));
        via[2].via[IRA] = (via[2].via[IRAA] & (~via[2].via[DDRA])) | (via[2].via[ORAA] & via[2].via[DDRA]); // read data from port base it on DDRA mask
        via[2].last_port = port;

        DEBUG_LOG(0, "profile.c:widget.c: IRA2  Profile to Lisa <%02x   pc24:%08x", via[2].via[IRA], pc24);
        return via[2].via[IRA];

    case DDRB2:
        DEBUG_LOG(0, "DDRB2");
        return via[2].via[DDRB]; // DDRB
    case DDRA2:
        DEBUG_LOG(0, "DDRA2");
        return via[2].via[DDRA]; // DDRA

    case T1LL2: // Timer 1 Low Order Latch
        DEBUG_LOG(0, "T1LL2=%02x", (via[2].via[T1LL]));
        VIA_CLEAR_IRQ_T1(2); // clear T1 irq on T1 read low or write high - does this clear the timer? // 2020.11.06 re-enabling this and fixing via #
        via[2].last_port = port;

        return (via[2].via[T1LL]);

    case T1LH2: // Time 1 High Order Latch
        DEBUG_LOG(0, "T1LH2 %02x", via[2].via[T1LH]);
        via[2].last_port = port;
        return via[2].via[T1LH];

    case T1CL2: // Timer 1 Low Order Counter
        DEBUG_LOG(0, "T1CL2");
        VIA_CLEAR_IRQ_T1(2); // clear T1 irq on T1 read low or write high
        // via[2].via[T1CL]=get_via_timer_left_from_te(via[2].t1_e) & 0xff;

        via[2].via[T1CL] = get_via_timer_left_or_passed_from_te(via[2].t1_e, via[2].t1_set_cpuclk) & 0xff;
        via[2].last_port = port;
        return via[2].via[T1CL];

    case T1CH2: // Timer 1 High Order Counter
        DEBUG_LOG(0, "T1CH2");
        VIA_CLEAR_IRQ_T1(2); // clear timer 1 interrupt on access
        via[2].via[T1CH] = get_via_timer_left_or_passed_from_te(via[2].t1_e, via[2].t1_set_cpuclk) >> 8;
        via[2].last_port = port;
        return via[2].via[T1CH];

    case T2CL2: // Timer 2 Low Byte
        DEBUG_LOG(0, "read T2CL2 t2_e:%ld set at:%ld", via[2].t2_e, via[2].t2_set_cpuclk);

        via[2].via[T2CL] = get_via_timer_left_or_passed_from_te(via[2].t2_e, via[2].t2_set_cpuclk) & 0xff;
        via[2].last_port = port;

        // via[2].via[IFR] &=0xDF;                                    // clear T2 Interrupt from IFR  (bit 5) */
        VIA_CLEAR_IRQ_T2(2); // clear T2 irq on T1 read low or write high

        return (via[2].via[T2CL]);

    case T2CH2:
        DEBUG_LOG(0, "T2CH2"); // return via[2].via[T2CH];
        via[2].via[T2CH] = get_via_timer_left_or_passed_from_te(via[2].t2_e, via[2].t2_set_cpuclk) >> 8;
        via[2].last_port = port;
        return via[2].via[T2CH];

    case SR2:
    {
        DEBUG_LOG(0, "SR2");
        VIA_CLEAR_IRQ_SR(2); // clear SR irq on SR access
        via[2].last_port = port;
        return via[2].via[SHIFTREG];
    }

    case ACR2:
        DEBUG_LOG(0, "ACR2");
        via[2].last_port = port;
        return via[2].via[ACR];
    case PCR2:
        DEBUG_LOG(0, "PCR2");
        via[2].last_port = port;
        return via[2].via[PCR];

    case IFR2:
        if (via[2].ProFile)
        {

            if (via[2].ProFile->BSYLine)
                via[2].via[IFR] |= (VIA_IRQ_BIT_CA1);
            // 20060526-bug here// else                          via[2].via[IFR] &=(255-VIA_IRQ_BIT_CA1);

            // 20060525// via[2].via[IFR]|=8; -- do not enable this.       // Parity Error=true
            via[2].via[IFR] &= ~VIA_IRQ_BIT_CB2; // 20060525 - want this one  // Parity Error=false
        }

        via[2].via[IFR] &= ~VIA_IRQ_BIT_SR; // SR is not used on a Lisa // 20071103
        FIX_VIA_IFR(2);

        DEBUG_LOG(0, "IFR2=%02x bits: %s %s %s %s %s %s %s %s", via[2].via[IFR],
                  (via[2].via[IFR] & 1) ? "ifr0CA2:on" : "ifr0CA2:off",
                  (via[2].via[IFR] & 2) ? "bsy_ifr1CA1:on" : "bsy_ifr1CA1:off",
                  (via[2].via[IFR] & 4) ? "ifr2SR :on" : "ifr2SR :off",
                  (via[2].via[IFR] & 8) ? "parity_ifr3CB2:on" : "parity_ifr3CB2:off",
                  (via[2].via[IFR] & 16) ? "not_connected_ifr4CB1:on" : "not_connected_ifr4CB1:off",
                  (via[2].via[IFR] & 32) ? "ifr5T2 :on" : "ifr5T2 :off",
                  (via[2].via[IFR] & 64) ? "ifr6T1 :on" : "ifr6T1 :off",
                  (via[2].via[IFR] & 128) ? "ifr7ANY:on" : "ifr7ANY:off");

        via[2].last_port = port;
        return via[2].via[IFR];

    case IER2:
        DEBUG_LOG(0, "IER2");
        via[2].via[IER] |= 0x80; // docs says this returns logical 1 for bit 7 when read
        via[2].last_port = port;
        return via[2].via[IER];

    default:
        DEBUG_LOG(0, "Illegal Address Access VIA2:%08x", addr);
    }
    return 0;
}

uint8 viaX_ira(viatype *V, uint8 regnum)
{
    VIA_CLEAR_IRQ_PORT_A(V->vianum); // clear CA1/CA2 on ORA/IRA access
    // if (debug_log_enabled) fdumpvia2(buglog);
    //  driver enable is off, don't process ADMP/Profile data output - not sure if this is correct.

    if (V->vianum == 2 && V->via[ORBB] & V->via[DDRB] & 4)
    {
        DEBUG_LOG(0, "VIA:%d Returning Contrast", V->vianum);
        return contrast;
    }

    if (V->ProFile) // Is this a profile?
    {
        VIAProfileLoop(V->vianum, V->ProFile, PROLOOP_EV_IRA);
        V->via[IRA] = V->ProFile->VIA_PA;
        DEBUG_LOG(0, "VIA:%d profile.c:READ %02x from ProFile, pc24:%08x  tag:profile state:%d", V->vianum, V->via[IRA], pc24, V->ProFile->StateMachineStep);
        return V->via[IRA];
    }
    else if (V->ADMP)
    {
        DEBUG_LOG(0, "ADMP read from IRA");
        return 0;
    } // ADMP doesn't return anything far as I know
    // else if (V->ira!=NULL) return viaX_ira(2,regnum); // Some other thing attached.
    UNUSED(regnum);
    return 0; // nothing attached, ignore.
}

// this is the one that's actually used!
void viaX_ora(viatype *V, uint8 data, uint8 regnum)
{
    UNUSED(regnum);
    VIA_CLEAR_IRQ_PORT_A(V->vianum); // clear CA1/CA2 on ORA/IRA access
    V->last_pa_write = cpu68k_clocks;
    DEBUG_LOG(0, "VIA:%D ORA:%02x DDRA:%02x   ORB:%02x DDRB:%0x", V->vianum, V->via[ORA], V->via[DDRA], V->via[ORB], V->via[DDRB]);
    if (V->via[DDRA] == 0)
        return; // can't write just yet, ignore.
    if (V->vianum == 2)
    {
        if (check_contrast_set())
            return;
    }

    if (V->via[ORBB] & V->via[DDRB] & 4)
        return; // driver enable is off, don't process ADMP/Profile data output.
    V->orapending = 0;

    // if (debug_log_enabled) fdumpvia2(buglog);
    if (V->ProFile) // Is this a profile?
    {
        DEBUG_LOG(0, "VIA:%d profile.c:Sending byte %02x from viaX_ora to ProFile code pc24:%08x tag:profile step:%d @%d",
                  V->vianum,
                  data,
                  pc24,
                  V->ProFile->StateMachineStep, V->ProFile->indexwrite);
        V->ProFile->VIA_PA = data;
        V->via[ORA] = data;
        V->ProFile->last_a_accs = 1; // 20060323//
        VIAProfileLoop(V->vianum, V->ProFile, PROLOOP_EV_ORA);
        return;
    }

    if (V->ADMP)
    {
        V->via[ORA] = data;
        DEBUG_LOG(0, "ADMP ORA: Sending %02x to ADMP on VIA #%d", data, V->vianum);
        ImageWriterLoop(V->ADMP, data);
        V->via[IRB] = BSYLine_BIT;
        V->via[IFR] |= VIA_IRQ_BIT_CA1;
        return;
    }
    ALERT_LOG(0, "Unhandled ORA: Sending %02x to nowhere on VIA #%d", data, V->vianum);
}

void viaX_orb(ViaType *V, uint8 data)
{
#ifdef DEBUG
    uint8 flipped = ((data ^ V->via[ORBB]) & (V->via[DDRB]));
#endif

    VIA_CLEAR_IRQ_PORT_B(V->vianum); // clear Cb1/Cb2 on ORb/IRb access

    // Setting Contrast - DEN is off, PB7 is used as Strobe.
    if (V->vianum == 2)
    {
        if (check_contrast_set())
            return;
    }

    // Is there an attached ProFile device on this parallel port?
    if (V->ProFile)
    {

        // Push the ORB bits to the ProFile flags. (OCD/BSY/DSKDiag/PARITY_BIT are inputs, so we don't do them, hence they're
        // commented out, but left in for the sake of completeness / being pendantic, fasict, totalitarian, etc.
        //------------------------------------------------------------------------------------------------------------------------------------
        //  if ( OCDLine_BIT & V->via[DDRB]) {V->ProFile->DENLine=(data & OCDLine_BIT) ? 0:1;}   //0 Ignored - hardwired input only line
        //  if ( BSYLine_BIT & V->via[DDRB]) {V->ProFile->BSYLine=(data & BSYLine_BIT) ? 0:1;}   //1 Ignored - hardwired input only line
        if (DENLine_BIT & V->via[DDRB])
        {
            V->ProFile->DENLine = (data & DENLine_BIT) ? 0 : 1;
        } // 2 DEN - drive enable....
        if (RRWLine_BIT & V->via[DDRB])
        {
            V->ProFile->RRWLine = (data & RRWLine_BIT) ? 0 : 1;
        } // 3 RRW is 1=write, 0=read from port
        if (CMDLine_BIT & V->via[DDRB])
        {
            V->ProFile->CMDLine = (data & CMDLine_BIT) ? 0 : 1;
        } // 4 CMD line
        //  if ( PARITY_BIT   & V->via[DDRB]) {V->ProFile->Parity=(data &  PARITY_BIT) ? 0:1;}   //5 Parity error bit is input
        if (DSK_DIAG_BIT & V->via[DDRB])
        {
            DEBUG_LOG(0, "VIA:%d Write to DSK_DIAG on viaX_orb!:%02x", V->vianum, data & DSK_DIAG_BIT);
        } // 6 Floppy Disk Diag is input

        // this is wrong, reset is on VIa1!
        // if ( CTRL_RES_BIT & V->via[DDRB]) {if ((data & CTRL_RES_BIT)==0)                         //7 Controller reset
        //                                           { DEBUG_LOG(0,"Sent Controller Reset to ProFile:%02x",data & CTRL_RES_BIT);
        //                                             ProfileReset(V->ProFile);
        //                                           }
        //                                       }

        DEBUG_LOG(0, "VIA:%d profile.c:ORA:%02x DDRA:%02x   ORB:%02x DDRB:%0x", V->vianum, V->via[ORA], V->via[DDRA], V->via[ORB], V->via[DDRB]);
        DEBUG_LOG(0, "VIA:%d profile.c:pb0:!OCD:%d pb1<!BSY:%d pb2:!DEN:%d pb3:!DR/W/:%d pb4>!CMD/:%d pb5:PRES:%d pb6:DIAGWait:%d pb7:CTRL_RES:%d = Data=%02x",
                  V->vianum,
                  (data & 1), (data & 2), (data & 4), (data & 8), (data & 16), (data & 32), (data & 64), (data & 128), data);

        DEBUG_LOG(0, "Flipped bits: %s%s%s%s%s%s%s%s", ((flipped & 1) ? "OCD:pb0 " : "    "), ((flipped & 2) ? "BSY:pb1 " : "    "), ((flipped & 4) ? "DEN:pb2 " : "    "), ((flipped & 8) ? "RRW:pb3 " : "    "), ((flipped & 16) ? "CMD:pb4 " : "    "), ((flipped & 32) ? "PAR:pb5 " : "    "), ((flipped & 64) ? "DSK:pb6 " : "    "), ((flipped & 128) ? "CRS:pb7 " : "    "));

        V->ca1 = V->ProFile->BSYLine; // refresh

        DEBUG_LOG(0, "before Lines DEN:%d RRW:%d CMD:%d",
                  V->ProFile->DENLine, V->ProFile->RRWLine, V->ProFile->CMDLine);

        VIAProfileLoop(V->vianum, V->ProFile, PROLOOP_EV_ORB);

        //        if (pc24<0x00fe0000) print_via_profile_state("\nviaX_orb post-loop ", data,&via[2]);
        //
        //    if (0x80 & V->via[DDRB])
        //        {
        // hack//               if (  ( (data & 0x80)==0) && V->ProFile) ProfileReset(V->ProFile);  ///
        //        }  // PB7 = reset Profile
    }

    // Is there an attached dot matrix printer?
    if (V->ADMP)
    {
        DEBUG_LOG(0, "ADMP VIA:%d profile.c:pb0:!OCD:%d pb1<!BSY:%d pb2:!DEN:%d pb3:!DR/W/:%d pb4>!CMD/:%d pb5:PRES:%d pb6:DIAGWait:%d pb7:CTRL_RES:%d = Data=%02x",
                  V->vianum,
                  (data & 1), (data & 2), (data & 4), (data & 8), (data & 16), (data & 32), (data & 64), (data & 128), data);

        return; // Output to Apple Dot Matrix Printer -- no control needed here
    }
    return; // nothing attached, ignore.
}

uint8 viaX_irb(viatype *V) // this is good
{

    uint8 retval = 0;

    VIA_CLEAR_IRQ_PORT_B(V->vianum); // clear Cb1/Cb2 on ORb/IRb access

    if (V->vianum < 3 && floppy_ram[0])
    {
        floppy_go6504();
    } // if this is being read, it's likely Lisa is waiting on floppy, so speed it up

    // set up the return of the common bits before we deal with attached devices

    if (V->vianum == 2)
        retval |= (floppy_6504_wait ? 0 : DSK_DIAG_BIT);                                            // return floppy diag bit
    retval |= (((V->ProFile || V->ADMP) && IS_PARALLEL_PORT_ENABLED(V->vianum)) ? 0 : OCDLine_BIT); // ADMP/Profile attached?

    if (V->ADMP)
    {
        DEBUG_LOG(0, "ADMP VIA#%d polled for IRB", V->vianum);

        retval |= (V->ProFile || V->ADMP) && IS_PARALLEL_PORT_ENABLED(V->vianum) ? 0 : OCDLine_BIT; // ADMP/Profile attached?
        if (IS_PARALLEL_PORT_ENABLED(V->vianum))
            retval = BSYLine_BIT;

        // retval=V->via[IRB];
        V->via[IRB] = retval;
        return retval;
    }

    if (V->ProFile)
    {

        retval &= ~PARITY_BIT;
        retval |= (eparity[V->ProFile->VIA_PA] ? PARITY_BIT : 0); // was V->via[ORA]

        if (retval & CTRL_RES_BIT)
            retval |= PARITY_BIT; // when PB7=1 means ProFile is being reset, LisaTest expects PB5=1 too

        // if ((V->via[DDRB] & PARITY_BIT)==0) retval|=(eparity[V->via[ORA]] ? PARITY_BIT:0);
    }

    if (V->ProFile)
    {

        if (pc24 < 0x00fe0000)
            print_via_profile_state("\nviaX_irb pre-loop ", V->via[IRB], V);

        // BSY is controlled by the profile, while DEN, RRW, and CMD are controlled by Lisa.  Lisa bits must be set before
        // entering the ProFile loop - and they are from viaX_orb, while BSY must be set after the call to ProFile loop,
        // however, we set the rest of the bits in retval to return them to the Lisa as expected and to verify correctness.

        VIAProfileLoop(V->vianum, V->ProFile, PROLOOP_EV_IRB); // 2 means type of access, not VIA#

        // update return values from ProFile handshake/comm bits
        V->ca1 = V->ProFile->BSYLine;

        // Next four lines do the same thing as their uncommented versions below.  Leaving commented version in for clarity.
        // if (!(V->via[DDRB] & BSYLine_BIT)) retval |= (V->ProFile->BSYLine ? 0 : BSYLine_BIT); else retval |=(V->via[IRB] & BSYLine_BIT);
        // if (!(V->via[DDRB] & DENLine_BIT)) retval |= (V->ProFile->DENLine ? 0 : DENLine_BIT); else retval |=(V->via[IRB] & DENLine_BIT);
        // if (!(V->via[DDRB] & RRWLine_BIT)) retval |= (V->ProFile->RRWLine ? 0 : RRWLine_BIT); else retval |=(V->via[IRB] & RRWLine_BIT);
        // if (!(V->via[DDRB] & CMDLine_BIT)) retval |= (V->ProFile->CMDLine ? 0 : CMDLine_BIT); else retval |=(V->via[IRB] & CMDLine_BIT);

        // If the DDRB bit is an input return the value sent by the profile, otherwise return the previous value of that bit from the old IRB
        retval |= (!(V->via[DDRB] & BSYLine_BIT)) ? (V->ProFile->BSYLine ? 0 : BSYLine_BIT) : (V->via[IRB] & BSYLine_BIT);
        retval |= (!(V->via[DDRB] & DENLine_BIT)) ? (V->ProFile->DENLine ? 0 : DENLine_BIT) : (V->via[IRB] & DENLine_BIT);
        retval |= (!(V->via[DDRB] & RRWLine_BIT)) ? (V->ProFile->RRWLine ? 0 : RRWLine_BIT) : (V->via[IRB] & RRWLine_BIT);
        retval |= (!(V->via[DDRB] & CMDLine_BIT)) ? (V->ProFile->CMDLine ? 0 : CMDLine_BIT) : (V->via[IRB] & CMDLine_BIT);

        if ((V->via[DDRB] & PARITY_BIT) == 0)
            retval |= (eparity[V->via[ORA]] ? PARITY_BIT : 0); // re-enabled 2006.05.24
                                                               //  if ((V->via[DDRB] & PARITY_BIT)==0) retval|=(eparity[V->via[ORA]] ? 0:PARITY_BIT);  // re-enabled 2006.05.25
#ifdef DEBUG
        if (pc24 < 0x00fe0000)
            print_via_profile_state("\nviaX_irb post-loop ", retval, V);
        ///////////////////////////////////////////////////////////
        DEBUG_LOG(0, "VIA:%d Returning: %02x from viaX_irb ProFile code", V->vianum, retval);
        DEBUG_LOG(0, "VIA:%d profile.c:ORA:%02x DDRA:%02x   ORB:%02x DDRB:%0x", V->vianum, V->via[ORA], V->via[DDRA], V->via[ORB], V->via[DDRB]);
#endif

        V->via[IRB] = retval;
        return retval;
    }
    // else
    //        return V->via[IRB];

    // retval |=0x02;   // Set ProFile busy line, since there isn't once attached.
    // retval |=4;      // Set Disk Enable Line to no drive there.

    DEBUG_LOG(0, "VIA:%d profile.c:pb0:!OCD:%d pb1<!BSY:%d pb2:!DEN:%d pb3:DR/W/:%d pb4>!CMD/:%d pb5:!PRES:%d pb6:DIAGWait:%d pb7:CTRL_RES:%d = Data=%02x",
              V->vianum,
              (retval & 1), (retval & 2), (retval & 4), (retval & 8), (retval & 16), (retval & 32), (retval & 64), (retval & 128), retval);

    // if (debug_log_enabled) fdumpvia2(buglog);
    V->via[IRB] = retval;
    return retval;
}

uint8 lisa_rb_ext_2par_via(ViaType *V, uint32 addr)
{
    via[1].active = 1; // these are always active as they're on the
    via[2].active = 1; // these are always active as they're on the
    V->active = 1;     // motherboard of the machine...

#ifdef DEBUG
    if (V->ProFile)
    {
        DEBUG_LOG(0, "profile.c:State:%d VIA:%d reading from register %d (%s)", V->ProFile->StateMachineStep, V->vianum, ((addr & 0x7f) >> 3), via_regname(((addr & 0x7f) >> 3)));
    }
    else
    {
        DEBUG_LOG(0, "VIA:%d reading from register %d (%s)", V->vianum, ((addr & 0x7f) >> 3), via_regname(((addr & 0x7f) >> 3)));
    }
#endif

    if (V->ProFile)
        VIAProfileLoop(V->vianum, V->ProFile, PROLOOP_EV_NUL); // 2021.05.24

    uint8 port = addr & 0x79; // was 7f, changing to 79
    switch (port) 
    {
    case IRB2:
    {
#ifdef DEBUG
        uint8 flipped = V->via[IRBB];

        DEBUG_LOG(0, "Reading VIA2 IRB. DDRB mask=%02x:(%s%s%s%s%s%s%s%s)", V->via[DDRB],
                  ((V->via[DDRB] & 1) ? "OCD:PB0:out " : "OCD:pb0:in  "),
                  ((V->via[DDRB] & 2) ? "BSY:PB1:out " : "BSY:pb1:in  "),
                  ((V->via[DDRB] & 4) ? "DEN:PB2:out " : "DEN:pb2:in  "),
                  ((V->via[DDRB] & 8) ? "RRW:PB3:out " : "RRW:pb3:in  "),
                  ((V->via[DDRB] & 16) ? "CMD:PB4:out " : "CMD:pb4:in  "),
                  ((V->via[DDRB] & 32) ? "PAR:PB5:out " : "PAR:pb5:in  "),
                  ((V->via[DDRB] & 64) ? "DSK:PB6:out " : "DSK:pb6:in  "),
                  ((V->via[DDRB] & 128) ? "CRS:PB7:out " : "CRS:pb7:in  "));
#endif

        V->via[IRBB] = viaX_irb(V);

        DEBUG_LOG(0, "viaX_irb() returned %02x", V->via[IRBB]);

        V->via[IRB] = (V->via[IRBB] & (~V->via[DDRB])) | (V->via[ORBB] & V->via[DDRB]);
        V->last_port = port;

        DEBUG_LOG(0, "returning irb & !DDRB is %02x", V->via[IRB]);

        VIA_CLEAR_IRQ_PORT_B(V->vianum); // clear Cb1/Cb2 on ORb/IRb access

#ifdef DEBUG
        flipped ^= V->via[IRB];
        DEBUG_LOG(0, "IRB Flipped:(%s%s%s%s%s%s%s%s)",
                  ((flipped & 1) ? "OCD:PB0 " : " "),
                  ((flipped & 2) ? "BSY:PB1 " : " "),
                  ((flipped & 4) ? "DEN:PB2 " : " "),
                  ((flipped & 8) ? "RRW:PB3 " : " "),
                  ((flipped & 16) ? "CMD:PB4 " : " "),
                  ((flipped & 32) ? "PAR:PB5 " : " "),
                  ((flipped & 64) ? "DSK:PB6 " : " "),
                  ((flipped & 128) ? "CRS:PB7 " : " "));

        DEBUG_LOG(0, "VIA2 IRB read=%02x:(OCD:%s BSY:%s DEN:%s RRW: %sCMD:%s PAR:%s DSKDIAG:%s CRES:%s)", V->via[IRB],
                  ((V->via[IRB] & 1) ? "PB0:1 " : "pb0:0 "),
                  ((V->via[IRB] & 2) ? "PB1:1 " : "pb1:0 "),
                  ((V->via[IRB] & 4) ? "PB2:1 " : "pb2:0 "),
                  ((V->via[IRB] & 8) ? "PB3:1 " : "pb3:0 "),
                  ((V->via[IRB] & 16) ? "PB4:1 " : "pb4:0 "),
                  ((V->via[IRB] & 32) ? "PB5:1 " : "pb5:0 "),
                  ((V->via[IRB] & 64) ? "PB6:1 " : "pb6:0 "),
                  ((V->via[IRB] & 128) ? "PB7:1 " : "pb7:0 "));

#endif
        return V->via[IRB];
    }

    case IRANH2:
        V->last_a_accs = 0;
        if (V->ProFile)
            V->ProFile->last_a_accs = 0;

        V->via[IRAA] = (viaX_ira(V, 15));
        V->via[IRA] = (V->via[IRAA] & (~V->via[DDRA])) | (V->via[ORAA] & V->via[DDRA]); // read data from port base it on DDRA mask
        V->last_port = port;

        DEBUG_LOG(0, "profile.c:widget.c: Profile->Lisa IRANH %02x  pc24:%08x", V->via[IRA], pc24); // 20060105 or in old outputs
        return V->via[IRA];

    case IRA2:
        V->last_a_accs = 0;
        if (V->ProFile)
            V->ProFile->last_a_accs = 0;
        VIA_CLEAR_IRQ_PORT_A(V->vianum); // clear CA1/CA2 on ORA/IRA access

        V->via[IRAA] = (viaX_ira(V, 15));
        V->via[IRA] = (V->via[IRAA] & (~V->via[DDRA])) | (V->via[ORAA] & V->via[DDRA]);             // read data from port base it on DDRA mask
        V->last_port = port;
        DEBUG_LOG(0, "profile.c:widget.c: Profile->Lisa IRA2 %02x   pc24:%08x", V->via[IRA], pc24); // 20060105 or in old outputs

        return V->via[IRA];

    case DDRB2:
        DEBUG_LOG(0, "DDRB");
        V->last_port = port;
        return V->via[DDRB]; // DDRB
    case DDRA2:
        DEBUG_LOG(0, "DDRA");
        V->last_port = port;
        return V->via[DDRA]; // DDRA

    case T1LL2: // Timer 1 Low Order Latch
        DEBUG_LOG(0, "T1LL2=%02x", (V->via[T1LL]));
        VIA_CLEAR_IRQ_T1(V->vianum);
        V->last_port = port;
        return (V->via[T1LL]);

    case T1LH2: // Time 1 High Order Latch
        DEBUG_LOG(0, "T1LH2 %02x", V->via[T1LH]);
        V->last_port = port;
        return V->via[T1LH];

    case T1CL2: // Timer 1 Low Order Counter
        DEBUG_LOG(0, "T1CL2");
        VIA_CLEAR_IRQ_T1(V->vianum); // clear T1 irq on T1 read low or write high
        // V->via[T1CL]=get_via_timer_left_from_te(V->t1_e) & 0xff;

        V->via[T1CL] = get_via_timer_left_or_passed_from_te(V->t1_e, V->t1_set_cpuclk) & 0xff;
        V->last_port = port;
        return V->via[T1CL];

    case T1CH2: // Timer 1 High Order Counter
        DEBUG_LOG(0, "T1CH2");
        VIA_CLEAR_IRQ_T1(V->vianum); // clear timer 1 interrupt on access
        V->via[T1CH] = get_via_timer_left_or_passed_from_te(V->t1_e, V->t1_set_cpuclk) >> 8;
        V->last_port = port;
        return V->via[T1CH];

    case T2CL2: // Timer 2 Low Byte
        DEBUG_LOG(0, "read T2CL2 t2_e:%ld set at:%ld", V->t2_e, V->t2_set_cpuclk);

        V->via[T2CL] = get_via_timer_left_or_passed_from_te(V->t2_e, V->t2_set_cpuclk) & 0xff;
        V->last_port = port;

        // V->via[IFR] &=0xDF;                                    // clear T2 Interrupt from IFR  (bit 5) */
        VIA_CLEAR_IRQ_T2(V->vianum); // clear T2 irq on T1 read low or write high

        return (V->via[T2CL]);

    case T2CH2:
        DEBUG_LOG(0, "T2CH2"); // return V->via[T2CH];
        V->via[T2CH] = get_via_timer_left_or_passed_from_te(V->t2_e, V->t2_set_cpuclk) >> 8;
        V->last_port = port;
        return V->via[T2CH];

    case SR2:
    { // unused//uint8 shift=(V->via[ACR] & 28)>>2;
        // set up an event to expire 8 VIA clock cycles after current time.
        /// 2005.06.28///                      if (shift==2) {V->sr_e=cpu68k_clocks+VIACLK_TO_CPUCLK(8); get_next_timer_event();}
        /// if (shift==2) {V->sr_e=cpu68k_clocks+8*via_clock_diff; get_next_timer_event();}
        DEBUG_LOG(0, "SR");
        VIA_CLEAR_IRQ_SR(V->vianum); // clear SR irq on SR access
        V->last_port = port;
        return V->via[SHIFTREG];
    }

    case ACR2:
        DEBUG_LOG(0, "ACR");
        V->last_port = port;
        return V->via[ACR];
    case PCR2:
        DEBUG_LOG(0, "PCR");
        V->last_port = port;
        return V->via[PCR];

    case IFR2:
        if (V->ProFile)
        {

            if (V->ProFile->BSYLine)
                V->via[IFR] |= (VIA_IRQ_BIT_CA1);
            // 20060526-bug here// else                          V->via[IFR] &=(255-VIA_IRQ_BIT_CA1);

            // 20060525// V->via[IFR]|=8; -- do not enable this.       // Parity Error=true
            V->via[IFR] &= ~VIA_IRQ_BIT_CB2; // 20060525 - want this one  // Parity Error=false
        }

        V->via[IFR] &= ~VIA_IRQ_BIT_SR; // explicitly shut off SR bit.
        V->last_port = port;
        FIX_VIAP_IFR();
        DEBUG_LOG(0, "IFR2=%02x", V->via[IFR]);

        DEBUG_LOG(0, "IFR2 bits: %s %s %s %s %s %s %s %s",
                  (V->via[IFR] & 1) ? "ifr0CA2:on" : "ifr0CA2:off",
                  (V->via[IFR] & 2) ? "bsy_ifr1CA1:on" : "bsy_ifr1CA1:off",
                  (V->via[IFR] & 4) ? "ifr2SR :on" : "ifr2SR :off",
                  (V->via[IFR] & 8) ? "parity_ifr3CB2:on" : "parity_ifr3CB2:off",
                  (V->via[IFR] & 16) ? "not_connected_ifr4CB1:on" : "not_connected_ifr4CB1:off",
                  (V->via[IFR] & 32) ? "ifr5T2 :on" : "ifr5T2 :off",
                  (V->via[IFR] & 64) ? "ifr6T1 :on" : "ifr6T1 :off",
                  (V->via[IFR] & 128) ? "ifr7ANY:on" : "ifr7ANY:off");

        return V->via[IFR];

    case IER2:
        DEBUG_LOG(0, "IER2");
        V->via[IER] |= 0x80; // docs says this returns logical 1 for bit 7 when read
        V->last_port = port;
        return V->via[IER];

    default:
        DEBUG_LOG(0, "Illegal Address Access VIA2:%08x", addr);
    }
    return 0;
}

void lisa_wb_ext_2par_via(ViaType *V, uint32 addr, uint8 xvalue)
{

    via[1].active = 1; // these are always active as they're on the
    V->active = 1;     // motherboard of the machine...

#ifdef DEBUG
    if (via[2].ProFile)
        DEBUG_LOG(0, "profile.c:State:%d VIA:2 writing to register %d (%s)", via[2].ProFile->StateMachineStep, ((addr & 0x7f) >> 3), via_regname(((addr & 0x7f) >> 3)));
#else
    DEBUG_LOG(0, "VIA:%d writing %02x to register %d (%s) @%08x", V->vianum, xvalue, (addr & 0x79) / 8, via_regname((addr & 0x79) / 8), addr);
#endif

    uint8 port = addr & 0x79; // fcd901      // was 0x7f it's now 0x79  // 2004.06.24 because saw MOVEP.W to T2CH, but suspect it also writes to T2CL!
    switch (port) 
    {
    case ORB2: // IRB/ORB
    {
#ifdef DEBUG
        uint8 flipped = (V->via[ORBB] ^ xvalue) & V->via[DDRB];

        DEBUG_LOG(0, "Writing VIA2 ORB. DDRB mask=%02x:(%s%s%s%s%s%s%s%s)", V->via[DDRB],
                  ((V->via[DDRB] & 1) ? "OCD:PB0:out " : "OCD:pb0:in  "),
                  ((V->via[DDRB] & 2) ? "BSY:PB1:out " : "BSY:pb1:in  "),
                  ((V->via[DDRB] & 4) ? "DEN:PB2:out " : "DEN:pb2:in  "),
                  ((V->via[DDRB] & 8) ? "RRW:PB3:out " : "RRW:pb3:in  "),
                  ((V->via[DDRB] & 16) ? "CMD:PB4:out " : "CMD:pb4:in  "),
                  ((V->via[DDRB] & 32) ? "PAR:PB5:out " : "PAR:pb5:in  "),
                  ((V->via[DDRB] & 64) ? "DSK:PB6:out " : "DSK:pb6:in  "),
                  ((V->via[DDRB] & 128) ? "CRS:PB7:out " : "CRS:pb7:in  "));

        DEBUG_LOG(0, "ORB bits Flipped  :(%s%s%s%s%s%s%s%s)",
                  ((flipped & 1) ? "OCD:PB0 " : " "),
                  ((flipped & 2) ? "BSY:PB1 " : " "),
                  ((flipped & 4) ? "DEN:PB2 " : " "),
                  ((flipped & 8) ? "RRW:PB3 " : " "),
                  ((flipped & 16) ? "CMD:PB4 " : " "),
                  ((flipped & 32) ? "PAR:PB5 " : " "),
                  ((flipped & 64) ? "DSK:PB6 " : " "),
                  ((flipped & 128) ? "CRS:PB7 " : " "));

        if (flipped & 16)
            DEBUG_LOG(0, "profile.c: CMD Flipped to %d at PC24:%08x  tag:profile step:%d", xvalue & 16, pc24,
                      (V->ProFile != NULL) ? (V->ProFile->StateMachineStep) : -99);

        DEBUG_LOG(0, "VIA2 ORB write=%02x:(OCD:%s BSY:%s DEN:%s RRW: %sCMD:%s PAR:%s DSKDIAG:%s CRES:%s)", xvalue,
                  ((xvalue & 1) ? "OCD:PB0:1 " : "OCD:PB0:0 "),
                  ((xvalue & 2) ? "BSY:PB1:1 " : "BSY:PB1:0 "),
                  ((xvalue & 4) ? "DEN:PB2:1 " : "DEN:PB2:0 "),
                  ((xvalue & 8) ? "RRW:PB3:1 " : "RRW:PB3:0 "),
                  ((xvalue & 16) ? "CMD:PB4:1 " : "CMD:PB4:0 "),
                  ((xvalue & 32) ? "PAR:PB5:1 " : "PAR:PB5:0 "),
                  ((xvalue & 64) ? "DSK:PB6:1 " : "DSK:PB6:0 "),
                  ((xvalue & 128) ? "CRS:PB7:1 " : "CRS:PB7:0 "));

#endif

        V->via[ORBB] = xvalue; // set the shadow register to the unmasked data for later use.
        V->via[ORB] = (V->via[IRBB] & (~V->via[DDRB])) | (V->via[ORBB] & V->via[DDRB]);
        V->last_port = port;

        VIA_CLEAR_IRQ_PORT_B(V->vianum); // clear Cb1/Cb2 on ORb/IRb access

        if (!V->via[DDRB])
            return;                               // don't write anything (no handshake) if the DDRB is all inputs
        viaX_orb(V, V->via[ORBB] & V->via[DDRB]); // output the masked data.

        return;
    }
    case ORANH2: // IRA/ORA
        DEBUG_LOG(0, "ORANH");

        V->last_a_accs = 1;
        if (V->ProFile)
            V->ProFile->last_a_accs = 1;

	// Note (TorZidan@): I commented out this code as I think it's wrong (it belongs in the "case ORA2" below).
        // Once we confirm that things are running smoothly, we can remove this. See more in the docs for variable "last_port".
        // if (V->via[ORA] == xvalue)
        // {
        //     DEBUG_LOG(0, "ORANH2: Already wrote %02 to port, not pushing it", xvalue);
        //     return; // already what's on the port, ignore the write //20060418
        // }

        V->via[ORAA] = xvalue; // set the shadow register to the unmasked data for later use.
        V->via[ORA] = (V->via[IRAA] & (~V->via[DDRA])) | (V->via[ORAA] & V->via[DDRA]);
        V->last_port = port;

        if (!V->via[DDRA])
        { // don't write anything if the DDRA is all inputs
            V->orapending = 1;
            DEBUG_LOG(0, "ORANH: Not writing %02x now, but setting pending since DDRA=0", xvalue);
            return;
        }
        DEBUG_LOG(0, "profile.c:widget.c: Lisa->Profile ORA2NH %02x   pc24:%08x", V->via[ORA], pc24);
        viaX_ora(V, V->via[ORAA] & V->via[DDRA], 15);
        // V->orapending=1;              //20060417//
        return;

    case ORA2:
        DEBUG_LOG(0, "ORA");
        V->last_a_accs = 1;
        if (V->ProFile)
            V->ProFile->last_a_accs = 1;

        if (V->via[ORA] == xvalue && V->last_port == ORANH2)
        {
            DEBUG_LOG(0, "in ORA2: Already wrote %02x to ORANH2, not pushing it again.\n", xvalue);
            via[2].last_port = port;
            return; // already what's on the port, ignore the write
        }

        VIA_CLEAR_IRQ_PORT_A(V->vianum); // clear CA1/CA2 on ORA/IRA access

        V->via[ORA] = (V->via[IRAA] & (~V->via[DDRA])) | (V->via[ORAA] & V->via[DDRA]);

        V->via[ORAA] = xvalue;
        V->last_port = port;

        if (!V->via[DDRA])
        { // don't write anything if the DDRA is all inputs
            V->orapending = 1;
            return;
        }
        DEBUG_LOG(0, "profile.c:widget.c: Lisa->Profile ORA2 %02x  pc24:%08x", V->via[ORA], pc24);
        viaX_ora(V, V->via[ORAA] & V->via[DDRA], ORA); // output the masked data
        V->orapending = 0;
        return;

    case DDRB2: // DDRB 2
        DEBUG_LOG(0, "DDRB2");
        V->via[DDRB] = xvalue;
        V->last_port = port;

        DEBUG_LOG(0, "Writing VIA2 DDRB mask=%02x:(%s%s%s%s%s%s%s%s)", V->via[DDRB],
                  ((V->via[DDRB] & 1) ? "OCD:PB0:out " : "OCD:pb0:in  "),
                  ((V->via[DDRB] & 2) ? "BSY:PB1:out " : "BSY:pb1:in  "),
                  ((V->via[DDRB] & 4) ? "DEN:PB2:out " : "DEN:pb2:in  "),
                  ((V->via[DDRB] & 8) ? "RRW:PB3:out " : "RRW:pb3:in  "),
                  ((V->via[DDRB] & 16) ? "CMD:PB4:out " : "CMD:pb4:in  "),
                  ((V->via[DDRB] & 32) ? "PAR:PB5:out " : "PAR:pb5:in  "),
                  ((V->via[DDRB] & 64) ? "DSK:PB6:out " : "DSK:pb6:in  "),
                  ((V->via[DDRB] & 128) ? "CRS:PB7:out " : "CRS:pb7:in  "));

        if (xvalue)                                   // only try the write if the mask has changed
            viaX_orb(V, V->via[ORBB] & V->via[DDRB]); // output the data that was masked off
        return;

    case DDRA2: // and DDRA 3 registers

        DEBUG_LOG(0, "Overwriting DDRA (previous value was=%02x, new value is:%02x)",
                  V->via[DDRA], xvalue);

        V->via[DDRA] = xvalue;
        V->last_port = port;

        DEBUG_LOG(0, "Wrote DDRA mask=%02x:(%s%s%s%s%s%s%s%s)", V->via[DDRA],
                  ((V->via[DDRA] & 1) ? "PA0:out " : "pb0:in  "),
                  ((V->via[DDRA] & 2) ? "PA1:out " : "pb1:in  "),
                  ((V->via[DDRA] & 4) ? "PA2:out " : "pb2:in  "),
                  ((V->via[DDRA] & 8) ? "PA3:out " : "pb3:in  "),
                  ((V->via[DDRA] & 16) ? "PA4:out " : "pb4:in  "),
                  ((V->via[DDRA] & 32) ? "PA5:out " : "pb5:in  "),
                  ((V->via[DDRA] & 64) ? "PA6:out " : "pb6:in  "),
                  ((V->via[DDRA] & 128) ? "PA7:out " : "pb7:in  "));

        if (V->orapending && xvalue != 0) // only try to write if this mask changed and last access to A was a write
        {
            DEBUG_LOG(0, "V->orapending=%02x, DDRA=%02x, last write was:%02x",
                      V->orapending,
                      xvalue,
                      V->via[ORAA]);

            viaX_ora(V, (V->via[ORAA] & xvalue), ORA); // output the masked data
        }
        return;

    case T1LL2: // Timer 1 Low Order Latch
        DEBUG_LOG(0, "T1LL2");

        V->via[T1CL] = xvalue; // 4 T1LC actually writes to T1LL only
        V->via[T1LL] = xvalue;
        V->t1_e = get_via_te_from_timer((V->via[T1LH] << 8) | V->via[T1LL]);
        V->last_port = port;

        FIX_CLKSTOP_VIA_T1(V->vianum); // update cpu68k_clocks_stop if needed

        // #ifdef DEBUG
        V->t1_set_cpuclk = cpu68k_clocks;
        // #endif
        via_running = 1;
        DEBUG_LOG(0, "ll-t1clk:%d T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((V->via[T1LH] << 8) | V->via[T1LL]), V->t1_e, V->t1_e - cpu68k_clocks, cpu68k_clocks);

        return; // Set timer2 low byte

    case T1LH2:
        DEBUG_LOG(0, "T1LH2");
        V->via[T1LH] = (xvalue); // Set timer1 latch and counter
        V->last_port = port;
        via_running = 1;
        // 2005.05.31 - this next line was disabled - should it have been?
        // via[1].t1_e=get_via_te_from_timer((via[1].via[T1LH]<<8)|via[1].via[T1LL]);
        //  VIA_CLEAR_IRQ_T1(1);     // clear T1 irq on T1 read low or write high - does this clear the timer?

        DEBUG_LOG(0, "lh-t1clk:%d (%04x) t1lh1=%02x T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((V->via[T1LH] << 8) | V->via[T1LL]),
                  ((V->via[T1LH] << 8) | V->via[T1LL]), xvalue,
                  V->t1_e,
                  V->t1_e - cpu68k_clocks, cpu68k_clocks);

        return;

    case T1CL2:
        DEBUG_LOG(0, "T1CL2");

        V->via[T1LL] = xvalue;
        V->last_port = port;

        // V->t1_e=get_via_te_from_timer((V->via[T1LH]<<8)|via[1].via[T1LL]);   // so, does this still count down? likely not!
        // FIX_CLKSTOP_VIA_T1(1);      // update cpu68k_clocks_stop if needed           // disable this code here.
        //  V->t1_set_cpuclk=cpu68k_clocks;

        via_running = 1;

        DEBUG_LOG(0, "cl-t1clk:%d (%04x) CL1=%02x T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((V->via[T1LH] << 8) | V->via[T1LL]),
                  ((V->via[T1LH] << 8) | V->via[T1LL]), xvalue,
                  V->t1_e, V->t1_e - cpu68k_clocks, cpu68k_clocks);
        return;

    case T1CH2:
        DEBUG_LOG(0, "T1CH");
        if (V->via[ACR] & 128)
            V->via[IRB] &= 0x7f; // with ACR7=1, a write to T1C brings PB7 low
        V->via[T1LH] = xvalue;   // load into register
        V->via[T1CH] = V->via[T1LH];
        V->via[T1CL] = V->via[T1LL]; // 5 T1HC actually writes to T1HL + copies T1LL->T1CL, T1LH->T1CH
        V->last_port = port;

        V->t1_e = get_via_te_from_timer((V->via[T1CH] << 8) | V->via[T1LL]); // this one does tell the counter to count down!
        FIX_CLKSTOP_VIA_T1(2);                                               // update cpu68k_clocks_stop if needed
        V->t1_set_cpuclk = cpu68k_clocks;                                    // timer was set right now (at this clock)

        VIA_CLEAR_IRQ_T1(V->vianum); // clear T1 irq on T1 read low or write high

        DEBUG_LOG(0, "ch-t1clk:%d (%04x) CH1=%02x T1 will now expire at:%llx - %llx cycles from now - clock now:%llx",
                  ((V->via[T1LH] << 8) | V->via[T1LL]),
                  ((V->via[T1LH] << 8) | V->via[T1LL]), xvalue,
                  V->t1_e, V->t1_e - cpu68k_clocks, cpu68k_clocks);

        via_running = 1;
        return;

    case T2CL2: // Set Timer 2 Low Latch
        DEBUG_LOG(0, "T2CL");
        V->via[T2LL] = xvalue; // Set t2 latch low
        V->last_port = port;
        // V->via[T2CL]=xvalue;                                                       // NOT SURE ABout this one!  2004.12.02
        //  no change to the counter! only the latch! //2005.06.05
        //  V->t2_e=get_via_te_from_timer((V->via[T2LH]<<8)|V->via[T2CL]);     // set timer expiration
        // FIX_CLKSTOP_VIA_T2(1);                                                         // update cpu68k_clocks_stop if needed
        // V->t2_set_cpuclk=cpu68k_clocks;                                            // timer was set right now (at this clock)

        // #ifdef DEBUG
        V->t2_set_cpuclk = cpu68k_clocks;
        DEBUG_LOG(0, "t2clk:%d (write takes effect on write to T2H) T2 set to expire at clock:%llx time now is %llx which is %llx cycles from now",
                  ((V->via[T2LH] << 8) | V->via[T2CL]), V->t2_e, cpu68k_clocks, V->t2_e - cpu68k_clocks);
        // #endif
        via_running = 1;
        return;

    case T2CH2: // timer2 high byte
                // a write to T2H will load and the counter as well.
        DEBUG_LOG(0, "T2CH");

        V->via[T2LH] = xvalue; // Set t2 latch high
        V->via[T2CH] = xvalue;
        V->via[T2CL] = V->via[T2LL]; // load the counter with latches
        V->last_port = port;

        VIA_CLEAR_IRQ_T2(V->vianum); // clear T2 irq on T2 read low or write high

        V->t2_e = get_via_te_from_timer((V->via[T2CH] << 8) | V->via[T2CL]); // set timer expiration
        DEBUG_LOG(0, "V->t2_e=%ld", V->t2_e);
        FIX_CLKSTOP_VIA_T2(2);            // update cpu68k_clocks_stop if needed
        V->t2_set_cpuclk = cpu68k_clocks; // timer was set right now (at this clock)

        DEBUG_LOG(0, "t2clk:%d T2 set to expire at CPU clock:%llx time now is %llx which is %llx cycles from now.  stop is:%llx",
                  ((V->via[T2LH] << 8) | V->via[T2CL]), V->t2_e, cpu68k_clocks, V->t2_e - cpu68k_clocks, cpu68k_clocks_stop);

        via_running = 1;

        return;

    case SR2:
    {
        // char srwrite[128];             //unused
        uint8 shift = (V->via[ACR] & 28) >> 2;

        DEBUG_LOG(0, "Write %02x to shift register on VIA2!!!!", xvalue);
        // fprintf(buglog,"Write %02x to shift register on VIA2!!!!\n",xvalue);

        // #ifdef DEBUG
        // if (xvalue==0)  debug_off();
        // #endif

        // EXIT(53);

        DEBUG_LOG(0, "SR");
        V->via[SHIFTREG] = xvalue; // write shift register
        V->last_port = port;
        via_running = 1;
        V->srcount = 0;

        // if (shift==4) {} - free running mode from T2 - timing tied to T2
        // if (shift==5) {} - one shot T2 mode - timing tied to T2
        if (shift == 6)
        {
            V->sr_e = cpu68k_clocks + VIACLK_TO_CPUCLK(8);
            get_next_timer_event();
        }
        /// if (shift==6)    {V->sr_e=cpu68k_clocks+8*via_clock_diff; get_next_timer_event();}
        // if (shift==7) - not implemented since CB1 is not connected on Lisa I/O board

        VIA_CLEAR_IRQ_SR(V->vianum); // clear SR irq on SR access
    }
        return;

    case ACR2:
        DEBUG_LOG(0, "ACR2 Port A Latching:%s  Port B Latching:%s",
                  ((xvalue & BIT0) ? "Enabled" : "Disabled"),
                  ((xvalue & BIT1) ? "Enabled" : "Disabled"));
#ifdef DEBUG
        switch (xvalue & (BIT4 | BIT3 | BIT2) >> 2)
        {
        case 0:
            DEBUG_LOG(0, "ACR2 SHIFT REG DISABLED");
            break;
        case 1:
            DEBUG_LOG(0, "ACR2 SR SHIFT IN UNDER COMTROL OF T2");
            break;
        case 2:
            DEBUG_LOG(0, "ACR2 SR SHIFT IN UNDER CONTROL OF 02");
            break;
        case 3:
            DEBUG_LOG(0, "ACR2 SR SHIFT IN UNDER CONTROL OF EXT.CLK");
            break;
        case 4:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT FREE-RUNNING AT T2 RATE");
            break;
        case 5:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT UNDER CONTROL OF T2");
            break;
        case 6:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT UNDER CONTROL OF 02");
            break;
        case 7:
            DEBUG_LOG(0, "ACR2 SR SHIFT OUT UNDER CONTROL OF EXT.CLK");
            break;
        }

        DEBUG_LOG(0, "ACR Timer2 Control: %s", ((xvalue & BIT6) ? "Countdown with PB6 Pulses" : "Timed IRQ"));

        switch (xvalue & (BIT7 | BIT6) >> 6)
        {
        case 0:
            DEBUG_LOG(0, "ACR2 Timer1 Control: TIMED IRQ Each Time T1 Loaded, PB7 Output Disabled");
            break;
        case 1:
            DEBUG_LOG(0, "ACR2 Timer1 Control: CONTINUOUS IRQ's, PB7 Output Disabled");
            break;
        case 2:
            DEBUG_LOG(0, "ACR2 Timer1 Control: TIMED IRQ Each Time T1 is loaded, PB7 - One Shot Output");
            break;
        case 3:
            DEBUG_LOG(0, "ACR2 Timer1 Control: Continous IRQ's, PB7: SQUARE WAVE OUTPUT");
            break;
        }
#endif
        V->via[ACR] = xvalue;
        V->last_port = port;
        return;

    case PCR2:
        DEBUG_LOG(0, "PCR via#%d", V->vianum);
#ifdef DEBUG
        DEBUG_LOG(0, "PCR via#%d", V->vianum);
        if (xvalue & 0)
        {
            DEBUG_LOG(0, "PCR2: CA1 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
        }
        else
        {
            DEBUG_LOG(0, "PCR2: CA1 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
        }

        switch ((xvalue & 15) >> 1)
        {
        case 0:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
            break;
        case 1:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT Input NEG. Active Edge");
            break;
        case 2:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
            break;
        case 3:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT INPUT POSITIVE Active Edge");
            break;
        case 4:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: HANDSHAKE OUTPUT");
            break;
        case 5:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: PULSE OUTPUT");
            break;
        case 6:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: LOW OUTPUT");
            break;
        case 7:
            DEBUG_LOG(0, "PCR2: CA2 INTERRUPT CONTROL: HIGH OUTPUT");
            break;
        }
        DEBUG_LOG(0, "PCR2: cb1 irq ctl:%s active edge", ((xvalue & BIT4) ? "1:positive" : "0:negative"));
        switch ((xvalue & (BIT7 | BIT6 | BIT5)) >> 5)
        {
        case 0:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INPUT NEG. ACTIVE EDGE");
            break;
        case 1:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT Input NEG. Active Edge");
            break;
        case 2:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INPUT POS. ACTIVE EDGE");
            break;
        case 3:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: INDEPENDENT INTERRUPT INPUT POSITIVE Active Edge");
            break;
        case 4:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: HANDSHAKE OUTPUT");
            break;
        case 5:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: PULSE OUTPUT");
            break;
        case 6:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: LOW OUTPUT");
            break;
        case 7:
            DEBUG_LOG(0, "PCR2: CB2 INTERRUPT CONTROL: HIGH OUTPUT");
            break;
        }
#endif
        if ((V->via[PCR] ^ xvalue) & 1)
            V->via[IFR] |= 2;
        V->via[PCR] = xvalue;
        V->last_port = port;
        return;

    case IFR2: /* IFR  */

        // V->via[IFR]&=(0x7f^(xvalue & 0x7f));  // 1 writes to IFR are used to clear bits!
        V->via[IFR] &= ~(xvalue); // 1 writes to IFR are used to clear bits!
        V->last_port = port;

        if (V->via[IFR] & 127)
            V->via[IFR] |= 128; // if all are cleared, clear bit 7 else set it
        else
            V->via[IFR] = 0;

        DEBUG_LOG(0, "IFR write bits: %s %s %s %s %s %s %s %s",
                  (V->via[IFR] & 1) ? "ifr0CA2:on" : "ifr0CA2:off",
                  (V->via[IFR] & 2) ? "bsy_ifr1CA1:on" : "bsy_ifr1CA1:off",
                  (V->via[IFR] & 4) ? "ifr2SR :on" : "ifr2SR :off",
                  (V->via[IFR] & 8) ? "parity_ifr3CB2:on" : "parity_ifr3CB2:off",
                  (V->via[IFR] & 16) ? "not_connected_ifr4CB1:on" : "not_connected_ifr4CB1:off",
                  (V->via[IFR] & 32) ? "ifr5T2 :on" : "ifr5T2 :off",
                  (V->via[IFR] & 64) ? "ifr6T1 :on" : "ifr6T1 :off",
                  (V->via[IFR] & 128) ? "ifr7ANY:on" : "ifr7ANY:off");
        return;

    case IER2:
        DEBUG_LOG(0, "IER");
        // if bit 7=0, then all 1 bits are reversed. 1=no irq, 0=irq enabled.

        if (xvalue & 0x80)
            V->via[IER] |= xvalue;
        else
            V->via[IER] &= (0x7f ^ (xvalue & 0x7f));

        // clear out anything that IER has disabled on the write.  2020.11.06
        V->via[IFR] &= V->via[IER];
        if (V->via[IFR] & 127)
            V->via[IFR] |= 128; // if all are cleared, clear bit 7 else set it
        else
            V->via[IFR] = 0;

        V->last_port = port;
// from via 1// if bit 7=0, then all 1 bits are reversed. 1=no irq, 0=irq enabled.
/// if   (xvalue & 128) {via[1].via[IER] |= xvalue;}
/// else via[1].via[IER] &=(0x7f-( xvalue & 0x7f));
#ifdef DEBUG
        if (xvalue & 0x80)
        {
            DEBUG_LOG(0, "VIA IER Write Set:%02x::%s %s %s %s %s %s %s %s\n", xvalue,
                      ((xvalue & VIA_IRQ_BIT_CA2) ? "CA2" : ""),
                      ((xvalue & VIA_IRQ_BIT_CA1) ? "CA1" : ""),
                      ((xvalue & VIA_IRQ_BIT_SR) ? "SR " : ""),
                      ((xvalue & VIA_IRQ_BIT_CB2) ? "CB2" : ""),
                      ((xvalue & VIA_IRQ_BIT_CB1) ? "CB1" : ""),
                      ((xvalue & VIA_IRQ_BIT_T2) ? "T2 " : ""),
                      ((xvalue & VIA_IRQ_BIT_T1) ? "T1 " : ""),
                      ((xvalue & VIA_IRQ_BIT_SET_CLR_ANY) ? "SET" : "CLEAR"));
        }
        else
        {
            DEBUG_LOG(0, "VIA IER clear Value:%02x::%s %s %s %s %s %s %s %s\n", V->via[IER],
                      ((V->via[IER] & VIA_IRQ_BIT_CA2) ? "CA2" : ""),
                      ((V->via[IER] & VIA_IRQ_BIT_CA1) ? "CA1" : ""),
                      ((V->via[IER] & VIA_IRQ_BIT_SR) ? "SR " : ""),
                      ((V->via[IER] & VIA_IRQ_BIT_CB2) ? "CB2" : ""),
                      ((V->via[IER] & VIA_IRQ_BIT_CB1) ? "CB1" : ""),
                      ((V->via[IER] & VIA_IRQ_BIT_T2) ? "T2 " : ""),
                      ((V->via[IER] & VIA_IRQ_BIT_T1) ? "T1 " : ""),
                      ((V->via[IER] & VIA_IRQ_BIT_SET_CLR_ANY) ? "SET" : "CLEAR"));
        }
#endif
        return;
    default:
        DEBUG_LOG(0, "Illegal address error Write to illegal address in viaX_wb_ @ %08x", addr);
    }
    return;
}

void init_vias(void)
{
    int i, j;

    memset(&via[0], 0, 8 * sizeof(ViaType));
    via[0].active = 0; // sacrificial lamb. :)  (aka waste of memory)
    via[1].active = 1;
    via[2].active = 1;
    via[3].active = 0;
    via[4].active = 0;
    via[5].active = 0;
    via[6].active = 0;
    via[7].active = 0;
    via[8].active = 0;

    via[1].irqnum = 2; // cops via
    via[2].irqnum = 1; // parallel port via

    via[3].irqnum = 5; // slot 1
    via[4].irqnum = 5; // slot 1
    via[5].irqnum = 4; // slot 2
    via[6].irqnum = 4; // slot 2
    via[7].irqnum = 3; // slot 3
    via[8].irqnum = 3; // slot 3

    via[1].active = via[2].active = 1;

    for (i = 1; i < 9; i++)
    {
        via[i].vianum = i;
        for (j = 0; j < 16; j++)
            via[i].via[j] = 0;

        via[i].last_a_accs = 0;
        via[i].irb = NULL;
        via[i].ira = NULL;
        via[i].orb = NULL;
        via[i].ora = NULL;
        via[i].ca1 = 0;
        via[i].ca2 = 0;
        via[i].cb1 = 0;
        via[i].cb2 = 0;
        via[i].srcount = 0;
        via[i].ProFile = NULL;
        via[i].ADMP = 0;
        via[i].t1_e = -1;
        via[i].t2_e = -1;
        via[i].t1_fired = 0;
        via[i].t2_fired = 0;
        // #ifdef DEBUG
        via[i].t1_set_cpuclk = 0;
        via[i].t2_set_cpuclk = 0;
        via[i].t1_fired_cpuclk = 0;
        via[i].t2_fired_cpuclk = 0;
        via[i].last_port = 0xff; // an invalid port number
        // #endif
    }

    via_running = 0;

    // #ifdef PROFILE_VIA2
    // #else
    //
    //    via[2].ProFile=NULL;
    // #endif
}

/* This is called by reg68k_external_execute in order to figure out whether or not
   one of the VIA timers will occur in the current video frame as called by emulate()
   in lisaem.c

   min_via_num is used as a return value to reg68k to signal which VIA timer is set.
   to find the via #, do (min_via_num & 0x7f)  To find out if it's timer 2 in that
   via do (min_via_num & 0x80)
*/

void reset_via(int i)
{

    via[1].irqnum = 2; // cops via
    via[2].irqnum = 1; // parallel port via

    via[3].irqnum = 5; // slot 1
    via[4].irqnum = 5; // slot 1

    via[5].irqnum = 4; // slot 2
    via[6].irqnum = 4; // slot 2

    via[7].irqnum = 3; // slot 3
    via[8].irqnum = 3; // slot 3

    via[1].vianum = 1;
    via[2].vianum = 2;
    via[3].vianum = 3;
    via[4].vianum = 4;
    via[5].vianum = 5;
    via[6].vianum = 6;
    via[7].vianum = 7;
    via[8].vianum = 8;

    via[1].active = 1;
    via[2].active = 1;
    via[i].srcount = 0;
    via[i].vianum = i;
    via[i].via[0] = 0;
    via[i].via[1] = 0;
    via[i].via[2] = 0;
    via[i].via[3] = 0;

    via[i].via[11] = 0;
    via[i].via[12] = 0;
    via[i].via[13] = 0;
    via[i].via[14] = 0;
    via[i].via[15] = 0;
    via[i].irb = NULL;
    via[i].ira = NULL;
    via[i].orb = NULL;
    via[i].ora = NULL;

    via[i].ca1 = 0;
    via[i].ca2 = 0;
    via[i].cb1 = 0;
    via[i].cb2 = 0;

    via[i].last_port = 0xff; // an invalid port number

    via_running = 0;
}

// sitting in the lab, and I'm codin' all night
// project won't compile, it'll be alright
// computer science for life, and that's my direction
// instead of b-balls, my homies throw exceptions    --mc++
