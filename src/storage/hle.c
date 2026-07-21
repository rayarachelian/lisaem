/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2021.03.26                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2022 Ray A. Arachelian                          *
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
*           High Level Emulation and hacks for Profile Hard Disk Routines              *
*     Interfacing code can be found reg68k.c, rom.c, romless.c, and profile.c          *
*                                                                                      *
\**************************************************************************************/

#include <generator.h>
#include <registers.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <reg68k.h>
#include <cpu68k.h>
#include <ui.h>
#include <vars.h>

extern int get_vianum_from_addr(long addr);
extern void lisa_console_output(uint8 c);

#define D0 reg68k_regs[0]
#define D1 reg68k_regs[1]
#define D2 reg68k_regs[2]
#define D3 reg68k_regs[3]
#define D4 reg68k_regs[4]
#define D5 reg68k_regs[5]
#define D6 reg68k_regs[6]
#define D7 reg68k_regs[7]
#define A0 reg68k_regs[0 + 8]
#define A1 reg68k_regs[1 + 8]
#define A2 reg68k_regs[2 + 8]
#define A3 reg68k_regs[3 + 8]
#define A4 reg68k_regs[4 + 8]
#define A5 reg68k_regs[5 + 8]
#define A6 reg68k_regs[6 + 8]
#define A7 reg68k_regs[7 + 8]
#define SP reg68k_regs[7 + 8]
#define PC reg68k_pc
#define RTS                                                     \
  {                                                             \
    regs.pc = pc24 = reg68k_pc = fetchlong(reg68k_regs[8 + 7]); \
    reg68k_regs[8 + 7] += 4;                                    \
  }

void uniplus_set_partition_table_size(uint32 disk, uint32 sswap, uint32 rroot, int kernel)
{
  uint32 astart, bstart, cstart, dstart, estart, fstart, gstart, hstart,
      a_size, b_size, c_size, d_size, e_size, f_size, g_size, h_size;

  uint32 swap = 2400;  // default swap for 5MB/10MB ProFile. Corvus=3959, Priam=4000 (these are physical disk geometry related)
  uint32 root = 16955; // default root for 10MB+ disks
                       // uint32 disk   = 19456;
  uint32 save = 0;
  uint32 max = disk;
  uint32 addr = 0;

  if (disk == 9728 || disk == 19456)
    return; // default 5mb or 10mb profile, don't change table

  // are we patching kernel v1.1 ()/sunix), or 1.4 ()/unix ?
  if (kernel == 11)
    addr = 0x00024ddc;
  if (kernel == 14)
    addr = 0x00026b54;
  if (!addr)
    return;

  // only apply patches if the kernel hasn't already been patched.
  // if the kernel has already been patched, assume it came to LisaEm
  // via BLU and that the image came from a drive larger than 10MB

  int i = 0, flag = 0;

  if (fetchlong(addr + 0 + i) != 0x000009c5)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x0000423b)
  {
    flag = 1;
  }
  i += 8;
  if (fetchlong(addr + 0 + i) != 0x00000065)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x00000960)
  {
    flag = 1;
  }
  i += 8;
  if (fetchlong(addr + 0 + i) != 0x000009c5)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x00001c3b)
  {
    flag = 1;
  }
  i += 8;
  if (fetchlong(addr + 0 + i) != 0x00002600)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x00002600)
  {
    flag = 1;
  }
  i += 8;
  if (fetchlong(addr + 0 + i) != 0x00000000)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x00000000)
  {
    flag = 1;
  }
  i += 8;
  if (fetchlong(addr + 0 + i) != 0x00000000)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x00001c00)
  {
    flag = 1;
  }
  i += 8;
  if (fetchlong(addr + 0 + i) != 0x00001c00)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x000009c0)
  {
    flag = 1;
  }
  i += 8;
  if (fetchlong(addr + 0 + i) != 0x00000065)
  {
    flag = 1;
  }
  if (fetchlong(addr + 4 + i) != 0x00004b9b)
  {
    flag = 1;
  }
  i += 8;

  if (flag)
    return; // kernel was patched already.

  // use defaults or passed params?
  if (rroot == 0xffffffff || rroot == 0)
    root = disk - swap;
  else
    root = rroot;
  if (sswap)
    swap = sswap;

  astart = 101 + swap;
  a_size = root; // root on 10MB disk
  bstart = 101;
  b_size = swap; // swap (2400 blks normal
  cstart = 0;
  c_size = 0; // root on 5MB disk

  dstart = astart + a_size;
  d_size = max - dstart; //  2nd fs on 10MB disk
  if (!d_size || (d_size & 0x80000000))
  {
    dstart = 0;
    d_size = 0;
  }

  estart = 0;
  e_size = 0; //  unused
  fstart = 0;
  f_size = 0; //  old root - a
  gstart = 0;
  g_size = 0; //  old swap -b
  hstart = 101;
  h_size = max - 101; //  whole disk (blocks 0-100 reserved for boot loader)

  storelong(addr + 0, astart);
  storelong(addr + 4, a_size);
  addr += 8;
  storelong(addr + 0, bstart);
  storelong(addr + 4, b_size);
  addr += 8;
  storelong(addr + 0, cstart);
  storelong(addr + 4, c_size);
  addr += 8;
  storelong(addr + 0, dstart);
  storelong(addr + 4, d_size);
  addr += 8;
  storelong(addr + 0, estart);
  storelong(addr + 4, e_size);
  addr += 8;
  storelong(addr + 0, fstart);
  storelong(addr + 4, f_size);
  addr += 8;
  storelong(addr + 0, gstart);
  storelong(addr + 4, g_size);
  addr += 8;
  storelong(addr + 0, hstart);
  storelong(addr + 4, h_size);
  addr += 8;
}

void hle_los31_read(uint32 count)
{
  ProFileType *P = NULL;
  int vianum = get_vianum_from_addr(A2);
  if (vianum > 1 && vianum < 9)
    P = via[vianum].ProFile;
  else
  {
    ALERT_LOG(0, "Got insane via #%d A2:%08x", vianum, A2);
    return;
  }

  int blocknumber = (P->DataBlock[5] << 16) | (P->DataBlock[6] << 8) | (P->DataBlock[7]);
  ALERT_LOG(0, "HLE LOS31 via:%d cmd:%d block:%d idxread:%d count:%d  PC:%08x A0:%08x", vianum, P->DataBlock[4], blocknumber, P->indexread, count, PC, A0);

  uint8 r = 0;
  uint32 e = D1;

  for (uint32 i = 0; i <= count; i++)
  {
    r = P->DataBlock[P->indexread++];
    e ^= r;
    lisa_wb_ram(A0, r);
    A0++;
  }

  D0 = r;
  D1 = e;

  cpu68k_clocks += (count * (8 + 4 + 8 + 10)); // this will be off for the 8x loop.
  D2 |= 0x0000ffff;                            // after DBRA.W D2 is -1

  RTS;

  ALERT_LOG(0, "Returning HLE LOS31 via:%d cmd:%d block:%d idxread:%d count:%d pc:%08x A0:%08x D1:%08x", vianum, P->DataBlock[4], blocknumber, P->indexread, count,
            PC, A0, D1);
}

void hle_los31_write(void)
{
  ProFileType *P = NULL;
  int vianum = get_vianum_from_addr(A2);
  if (vianum > 1 && vianum < 9)
    P = via[vianum].ProFile;
  else
    return;

  for (int i = 0; i < 128; i++)
  {
    P->DataBlock[P->indexwrite++] = fetchbyte(A0);
    A0++; // MOVE.B     (A0)+,(A2)
    P->DataBlock[P->indexwrite++] = fetchbyte(A0);
    A0++; // MOVE.B     (A0)+,(A2)
    P->DataBlock[P->indexwrite++] = fetchbyte(A0);
    A0++; // MOVE.B     (A0)+,(A2)
    P->DataBlock[P->indexwrite++] = fetchbyte(A0);
    A0++; // MOVE.B     (A0)+,(A2)
  }

  cpu68k_clocks += ((12 + 12 + 4 + 12 + 4 + 12 + 4 + 12 + 10) * 128 + 20);
  D0 |= 0x0000ffff; // d0.w=-1 after dbra
  RTS;
}

void hle_los31_write_00c08d0a(void)
{
  ProFileType *P = NULL;
  int vianum = get_vianum_from_addr(A2);
  if (vianum > 1 && vianum < 9)
    P = via[vianum].ProFile;
  else
    return;

  A1++; // a1 ++
  P->DataBlock[P->indexwrite++] = fetchbyte(A1);
  A1++; // fetchbyte(A1++) should but does not work right because we want to pass A1 before incrementing it
  P->DataBlock[P->indexwrite++] = fetchbyte(A1);
  A1++;
  P->DataBlock[P->indexwrite++] = fetchbyte(A1);
  A1++;

  RTS;
  cpu68k_clocks += (8 + 12 + 4 + 12 + 4 + 20);
}

// We no-longer do MacWorksXL3.0 hacks. See more at https://github.com/arcanebyte/lisaem/issues/40
// void hle_macws_read_unused(uint32 count)
// {
//   ProFileType *P = NULL;
//   int vianum = get_vianum_from_addr(A0);
//   if (vianum > 1 && vianum < 9)
//     P = via[vianum].ProFile;
//   else
//   {
//     ALERT_LOG(0, "Got insane via #%d A0:%08x", vianum, A0);
//     return;
//   }

//   int blocknumber = (P->DataBlock[5] << 16) | (P->DataBlock[6] << 8) | (P->DataBlock[7]);
//   ALERT_LOG(0, "HLE MWXL30 via:%d cmd:%d block:%d idxread:%d count:%d  PC:%08x A0:%08x", vianum, P->DataBlock[4], blocknumber, P->indexread, count, PC, A0);

//   uint8 r = 0;
//   uint32 e = D1;

//   for (uint32 i = 0; i < count; i++)
//   {
//     r = P->DataBlock[P->indexread++];
//     e ^= r;
//     lisa_wb_ram(A2, r);
//     A2++;
//   }

//   D0 = r;
//   D1 = e;

//   cpu68k_clocks += (count * (8 + 4 + 8 + 10)); // this will be off for the 8x loop.
//   D2 |= 0x0000ffff;                            // after DBRA.W D2 is -1

//   A0 = fetchlong(A7);
//   A7 += 4;

//   RTS;

//   ALERT_LOG(0, "Returning HLE MWXL30 via:%d cmd:%d block:%d idxread:%d count:%d pc:%08x A0:%08x D1:%08x", vianum, P->DataBlock[4], blocknumber, P->indexread, count,
//             PC, A0, D1);
// }

// We no-longer do MacWorksXL3.0 hacks. See more at https://github.com/arcanebyte/lisaem/issues/40
// void hle_macws_write_unused(void)
// {
//   ProFileType *P = NULL;
//   int vianum = get_vianum_from_addr(A0);
//   if (vianum > 1 && vianum < 9)
//     P = via[vianum].ProFile;
//   else
//   {
//     ALERT_LOG(0, "Got insane via #%d A0:%08x", vianum, A0);
//     return;
//   }

//   int blocknumber = (P->DataBlock[5] << 16) | (P->DataBlock[6] << 8) | (P->DataBlock[7]);
//   ALERT_LOG(0, "HLE LOS31 via:%d cmd:%d block:%d idxread:%d   PC:%08x A0:%08x A2:%08x", vianum, P->DataBlock[4], blocknumber, P->indexread, PC, A0, A2);

//   for (int i = 0; i <= 128; i++)
//   {
//     P->DataBlock[P->indexwrite++] = fetchbyte(A2);
//     A2++; // MOVE.B     (A0)+,(A2)
//     P->DataBlock[P->indexwrite++] = fetchbyte(A2);
//     A2++; // MOVE.B     (A0)+,(A2)
//     P->DataBlock[P->indexwrite++] = fetchbyte(A2);
//     A2++; // MOVE.B     (A0)+,(A2)
//     P->DataBlock[P->indexwrite++] = fetchbyte(A2);
//     A2++; // MOVE.B     (A0)+,(A2)
//   }

//   cpu68k_clocks += ((12 + 12 + 4 + 12 + 4 + 12 + 4 + 12 + 10) * 128 + 20);
//   D0 |= 0x0000ffff; // d0.w=-1 after dbra
//   A0 = fetchlong(A7);
//   A7 += 4;
//   RTS;

//   ALERT_LOG(0, "HLE LOS31 via:%d cmd:%d block:%d idxread:%d   PC:%08x A0:%08x A2:%08x", vianum, P->DataBlock[4], blocknumber, P->indexread, PC, A0, A2);
// }

// We no-longer do MacWorksXL3.0 hacks. See more at https://github.com/arcanebyte/lisaem/issues/40
// void hle_mw30_intercept_unused(void)
// {
//   if (PC == 0x00163946)
//   {
//     hle_macws_read_unused((D2 + 1) * 8);
//     return;
//   } // same exact code for MW XL 3.0 but A2/A0 swapped and A0 pushed to stack before RTS
//   if (PC == 0x00163ade)
//   {
//     hle_macws_write_unused();
//     return;
//   } // same exact code for MW XL 3.0 but A2/A0 swapped and A0 pushed to stack before RTS
//   ALERT_LOG(0, "Unhandled MW intercept at %08x", PC);
// }

// We no-longer do MacWorksXL3.0 hacks. This code was being called from profile.c
// See more at https://github.com/arcanebyte/lisaem/issues/40
// void apply_mw30_hacks_unused(void)
// {
//   if (!macworks_hle)
//     return;

//   if (check_running_lisa_os() != LISA_MACWORKS_RUNNING || context != 1)
//     return;

//   if (lisa_rl_ram(0x00163946) == 0x1010b101 && lisa_rl_ram(0x0016394a) == 0x14c01010)
//   {
//     lisa_ww_ram(0x00163946, 0xf33d);
//     lisa_ww_ram(0x00163ade, 0xf33d);

//     ALERT_LOG(0, "#  # #    #### Patching for MacWorks XL 3.0 ProFile");
//     ALERT_LOG(0, "#  # #    #    Patching for MacWorks XL 3.0 ProFile");
//     ALERT_LOG(0, "#### #    #### Patching for MacWorks XL 3.0 ProFile");
//     ALERT_LOG(0, "#  # #    #    Patching for MacWorks XL 3.0 ProFile");
//     ALERT_LOG(0, "#  # #### #### Patching for MacWorks XL 3.0 ProFile");
//   }

//   macworks_hle = 0;
// }

void hle_los_intercept(void)
{

  if (PC == 0x00c08a86)
  {
    ALERT_LOG(0, "511");
    hle_los31_read(511);
    return;
  }
  if (PC == 0x00c08a2e)
  {
    ALERT_LOG(0, "D2");
    hle_los31_read(D2);
    return;
  }
  if (PC == 0x00c08d0a)
  {
    ALERT_LOG(0, "c08d0a");
    hle_los31_write_00c08d0a();
    return;
  }
  if (PC == 0x00c08c86)
  {
    ALERT_LOG(0, "write");
    hle_los31_write();
    return;
  }

  ALERT_LOG(0, "UNHANDLED LOS31 HLE at %d/%08x", context, PC);
}

void apply_los31_hacks(void)
{
  if (!los31_hle)
    return;

  if (check_running_lisa_os() != LISA_OFFICE_RUNNING || context != 1)
    return;

  if (lisa_rw_ram(0x00c08a86) == 0x743f && lisa_rl_ram(0x00c08a88) == 0x1012b101 && lisa_rl_ram(0x00c08a88 + 4) == 0x10c01012)
  {

    lisa_ww_ram(0x00c08a86, 0xf33d); // read 512
    lisa_ww_ram(0x00c08a2e, 0xf33d); // read 1x
    lisa_ww_ram(0x00c08d0a, 0xf33d); // write 3x
    lisa_ww_ram(0x00c08c86, 0xf33d); // write 512

    ALERT_LOG(0, "#  # #    #### Patching for LOS3.1 ProFile");
    ALERT_LOG(0, "#  # #    #    Patching for LOS3.1 ProFile");
    ALERT_LOG(0, "#### #    #### Patching for LOS3.1 ProFile");
    ALERT_LOG(0, "#  # #    #    Patching for LOS3.1 ProFile");
    ALERT_LOG(0, "#  # #### #### Patching for LOS3.1 ProFile");
  }

  los31_hle = 0;
}

// also in z8530-terminal.cpp and reg68k.c
#define CONSOLETERM 2
extern void init_terminal_serial_port(int port); // create console terminalwx window in src/host/wxui/z8530-terminal.cpp
extern void lisa_console_output(uint8 c);        // single char output directly as VT100 in src/host/wxui/z8530-terminal.cpp
extern void lpw_console_output(char *text);      // output a whole string and translate SOROC->vt100 of whole string in src/host/wxui/z8530-terminal.cpp
extern void lpw_console_output_c(char c);        // output with SOROC->vt100 of a single char rather than string in src/host/wxui/z8530-terminal.cpp

void hle_monitor_intercept(void)
{
  switch (PC)
  {
  case 0x0f161854:
  {
    uint8 c = lisa_rb_ram(A3); // ADD.B      #1,(A3)
    lisa_wb_ram(A3, c + 1);
    regs.pc = 0x0F161856;
    pc24 = 0x0F161856;
    reg68k_pc = 0x0F161856;
    cpu68k_clocks += 8;
    lisa_console_output(13);
    lisa_console_output(13);
    ALERT_LOG(0, "Newline");
    return;
  }
  case 0x0f1618a4:
  {
    lisa_ww_ram((A3), 0x0101); // 740486-1/0f1618a4 (0 0/0/0) : 36bc 0101                  : 6...     :  516 : MOVE.W     #$0101,(A3)  SRC:clk:000000000cd54bf2 +12 clks
    regs.pc = 0x0f1618a6;
    pc24 = 0x0f1618a6;
    reg68k_pc = 0x0f1618a6;
    cpu68k_clocks += 12;
    reg68k_sr.sr_struct.z = 0;
    reg68k_sr.sr_struct.n = 0;
    reg68k_sr.sr_struct.c = 0;
    reg68k_sr.sr_struct.x = 0;
    lisa_console_output(27);  // ESC
    lisa_console_output('['); // [
    lisa_console_output('H'); // H
    ALERT_LOG(0, "Home");
    return;
  }
  case 0x0f16172c:
  {
    uint8 c = lisa_rb_ram(A2); // 1/0f16172c (0 0/0/0) : 101a                       : ..       :  247 : MOVE.B     (A2)+,D0  SRC:clk:0000000004af1a07 +8 clks
    D0 = (D0 & 0xffffff00) | c;
    A2 = (A2) + 1;
    reg68k_sr.sr_struct.z = (c == 0);
    reg68k_sr.sr_struct.n = (c > 127);
    reg68k_sr.sr_struct.c = 0;
    reg68k_sr.sr_struct.x = 0;
    regs.pc = 0x0f16172e;
    pc24 = 0x0f16172e;
    reg68k_pc = 0x0f16172e;
    cpu68k_clocks += 8;
    lpw_console_output_c(c); // character out
    ALERT_LOG(0, "Sending to TerminalWx Console: %c (%02x)", c, c)
    return;
  }
  default:
    ALERT_LOG(0, "Unimplemented HLE patch Monitor OS 12.x at %d/%08x", context, PC);
  }
}

void apply_monitor_hle_patches(void)
{
  monitor_patch = 0; // flag done so we don't keep doing it repeatedly

  ALERT_LOG(0, "#  # #    #### Patching for Monitor OS 12.x");
  ALERT_LOG(0, "#  # #    #    Patching for Monitor OS 12.x");
  ALERT_LOG(0, "#### #    #### Patching for Monitor OS 12.x");
  ALERT_LOG(0, "#  # #    #    Patching for Monitor OS 12.x");
  ALERT_LOG(0, "#  # #### #### Patching for Monitor OS 12.x");

  ALERT_LOG(0, "consoletermwindow=%d", consoletermwindow);

  if (consoletermwindow)
  {
    if (lisa_rw_ram(0x0f161854) != 0x5213)
    { // 33620868-1/0f161854 (0 0/0/0) : 5213                       : R.       :  785 : ADD.B      #1,(A3)  SRC:clk:000000000845b32d +8 clks
      ALERT_LOG(0, "Cannot patch, address 0x0f161854 does not contain 0x5213");
      ALERT_LOG(0, "Cannot patch, address 0x0f161854 does not contain 0x5213");
      ALERT_LOG(0, "Cannot patch, address 0x0f161854 does not contain 0x5213");
      return;
    }
    if (lisa_rl_ram(0x0f1618a4) != 0x36bc0101)
    {
      ALERT_LOG(0, "Cannot patch, address 0x0f1618a4 does not contain 0x36bc0101");
      ALERT_LOG(0, "Cannot patch, address 0x0f1618a4 does not contain 0x36bc0101");
      ALERT_LOG(0, "Cannot patch, address 0x0f1618a4 does not contain 0x36bc0101");
      return;
    }
    if (lisa_rw_ram(0x0f16172c) != 0x101a)
    {
      ALERT_LOG(0, "Cannot patch, address 0x0f16172c does not contain 0x101a");
      ALERT_LOG(0, "Cannot patch, address 0x0f16172c does not contain 0x101a");
      ALERT_LOG(0, "Cannot patch, address 0x0f16172c does not contain 0x101a");
      return;
    }

    abort_opcode = 0;
    ALERT_LOG(0, "Enabling TerminalWx console window");
    ALERT_LOG(0, "Enabling TerminalWx console window");
    ALERT_LOG(0, "Enabling TerminalWx console window");

    init_terminal_serial_port(CONSOLETERM); // open terminal window for console

    // since we don't detect MonitorOS until after it started, we lose the first menu prompt
    static char *mon = "\fMonitor: E(dit, C(ompile, F(ile, L(ink, X(ecute, A(ssemble, S(ysmgr, ? [0.12.3]";
    lpw_console_output(mon);

    // patch with F-Line HLE traps
    lisa_ww_ram(0x0f161854, 0xf33d); //  854277-1/0f161854 (0 0/0/0) : 5213                       : R.       :  785 : ADD.B      #1,(A3)  SRC:clk:000000000cd9ddb2 +8 clks
    lisa_ww_ram(0x0f1618a4, 0xf33d); //  740486-1/0f1618a4 (0 0/0/0) : 36bc 0101                  : 6...     :  516 : MOVE.W     #$0101,(A3)  SRC:clk:000000000cd54bf2 +12 clks
    lisa_ww_ram(0x0f16172c, 0xf33d); // 7404839-1/0f16172c (0 0/0/0) : 101a                       : ..       :  247 : MOVE.B     (A2)+,D0  SRC:clk:0000000004af1a07 +8 clks
  }
}

void apply_xenix_hle_patches(void)
{

  if (context != 1)
    return; // can only apply in context 1 (supervisor mode)
  // xenix_patch=0;            // flag done so we don't keep patching repeatedly

  ALERT_LOG(0, "#  # #    #### Patching for Xenix");
  ALERT_LOG(0, "#  # #    #    Patching for Xenix");
  ALERT_LOG(0, "#### #    #### Patching for Xenix");
  ALERT_LOG(0, "#  # #    #    Patching for Xenix");
  ALERT_LOG(0, "#  # #### #### Patching for Xenix");

  ALERT_LOG(0, "consoletermwindow=%d", consoletermwindow);
  // insert ProFile patches for Xenix here, not below.

  if (consoletermwindow)
  {
    abort_opcode = 0;
    ALERT_LOG(0, "Enabling TerminalWx console window");
    ALERT_LOG(0, "Enabling TerminalWx console window");
    ALERT_LOG(0, "Enabling TerminalWx console window");

    init_terminal_serial_port(CONSOLETERM); // open terminal window for console

    lisa_ww_ram(0x0000e180, 0xf33d); // 1e2e 000b                  : ....     :  263 : MOVE.B     $000b(A6),D7  PC:0000e184 SRC:

    ALERT_LOG(0, "abort_opcode:%d", abort_opcode);
    ALERT_LOG(0, "Enabling TerminalWx console window");
    ALERT_LOG(0, "Enabling TerminalWx console window");
    ALERT_LOG(0, "Enabling TerminalWx console window");
  }
  else
  {
    ALERT_LOG(0, "TerminalWx NOT enabled because consoletermwindow=%d not enabling terminal", consoletermwindow);
  }
}

// 2022.03.06 added console terminal wx to Xenix because I'm on a roll, and why not.
void hle_xenix_intercept(void)
{
  switch (PC)
  {
  case 0x0000e180:
  {
    D7 = (D7 & 0xffffff00) | lisa_rb_ram(0x000b + (A6));
    // putchar  1e2e 000b   : ....     :  263 : MOVE.B     $000b(A6),D7  PC:0000e184
    regs.pc = 0x0000e184;
    pc24 = 0x0000e184;
    reg68k_pc = 0x0000e184;
    reg68k_sr.sr_struct.z = (D7 == 0);
    lisa_console_output((uint8)(D7 & 0xff));
    ALERT_LOG(0, "%c", D7 & 0xff)
    return;
  }
  default:
    ALERT_LOG(0, "Unhandled HLE intercept for Xenix at %d/%08x", context, PC);
  }
}

void hle_intercept(void)
{
  DEBUG_LOG(0, ".");
  check_running_lisa_os();

  ProFileType *P = NULL;
  uint32 a4 = A4; // get a4 to use as the target for copying data to
  uint32 a5 = A5;
  long size = 0;

  DEBUG_LOG(0, "running os: %d, want:%d pc:%08x a4:%08x, a5:%08x, d7:%08x", running_lisa_os, LISA_UNIPLUS_RUNNING, PC, a4, a5, D7);

  if (PC == 0x00fe0090 || PC == rom_profile_read_entry)
  {
    DEBUG_LOG(0, "profile.c:state:hle - boot rom read pc:%08x", PC);
    romless_entry();
    return;
  } // Boot ROM ProFile Read HLE

  switch (running_lisa_os)
  {
  case LISA_XENIX_RUNNING:
    hle_xenix_intercept();
    return;
  case LISA_MONITOR_RUNNING:
    hle_monitor_intercept();
    return;
  case LISA_OFFICE_RUNNING:
    hle_los_intercept();
    return;
  case LISA_MACWORKS_RUNNING:
    // hle_mw30_intercept_unused();
    return;
  }

  // console kernel putchar intercept - first few calls go to this before running OS is detectable since the kernel
  // hasn't yet initialized the vector tables, but still calls putchar
  if (reg68k_pc == 0x000236c6 && (running_lisa_os == LISA_UNIPLUS_RUNNING || running_lisa_os == 0))
  {
    D7 = (D7 & 0xffffff00) | fetchbyte(0x0b + (A6));
    lisa_console_output((uint8)(D7 & 0x7f));
    ALERT_LOG(0, "uniplus putchar: %02x %c", (uint8)(D0 & 0x7f), (uint8)(D0 & 0x7f));
    regs.pc = 0x000236ca;
    pc24 = 0x000236ca;
    reg68k_pc = 0x000236ca;
  }

  if (running_lisa_os == LISA_UNIPLUS_RUNNING)
  {
    int vianum = get_vianum_from_addr(a5);
    if (vianum > 1 && vianum < 9)
      P = via[vianum].ProFile;

    DEBUG_LOG(0, "running os: %d pc:%08x a4:%08x, a5:%08x via:%d profile :%p", running_lisa_os, PC, a4, a5, vianum, P);

    // profile related intercepts
    if (P)
    {
      switch (reg68k_pc)
      {
      // uniplus lock speedup by pushing to next frame, and hopefully the next IRQ or timer event
      case 0x0000c188:
        if (!reg68k_sr.sr_struct.z)
        {
          reg68k_pc = 0x0000c182;
          cpu68k_clocks = cpu68k_clocks_stop + 24;
        }
        else
        {
          reg68k_pc = 0x0000c18c;
          cpu68k_clocks += 24;
        }
        break;

      case 0x00020c64:
        size = 4;
        reg68k_pc = 0x00020c74;
        ALERT_LOG(0, "profile.c:hle:read status");
        P->indexread = 0;
        P->indexwrite = 4;
        P->DataBlock[0] = 0;
        P->DataBlock[1] = 0;
        P->DataBlock[2] = 0;
        P->DataBlock[3] = 0;
        P->BSYLine = 0;
        P->StateMachineStep = 12;
        break; // read 4 status bytes into A4, increase a4+=4, PC=00020c74

      case 0x00020d1e:
        size = 20;
        regs.pc = pc24 = reg68k_pc = 0x00020d26;
        DEBUG_LOG(0, "profile.c:state:hle:read tags");
        break; // read 20 bytes of tags into *a4, increase a4+=20, D5= 0x0000ffff PC=00020d26
      case 0x00020d3e:
        size = 512;
        regs.pc = pc24 = reg68k_pc = 0x00020d72;
        DEBUG_LOG(0, "profile.c:state:hle:read sector");
        break; // read 512 bytes of data into *a4, a4+=512 D5=0x0000ffff PC=0x00020d72

      // write tag/sector specific
      case 0x00020ebc:
        regs.pc = pc24 = reg68k_pc = 0x00020ef0;
        DEBUG_LOG(0, "profile.c:state:hle:write sector+tags"); // write tags and data in one shot, then return to 0x00020ef0
        size = D0;                                             // d5
        a4 = A4;
        while (size--)
        {
          if (P->indexwrite > 542)
          {
            ALERT_LOG(0, "ProFile buffer overrun!");
            P->indexwrite = 4;
          }
          P->DataBlock[P->indexwrite++] = fetchbyte(a4++);
        }

        return; // we're done, so return.

      default:
        ALERT_LOG(0, "Unknown F-Line error: PC:%08x", PC);
        return;
      }

      // fall through common code for read status, tag, data

      A4 += size; // final A4 value to return to UniPlus.
      regs.pc = pc24 = reg68k_pc;
      D5 = 0x0000ffff; // D5 is done in dbra loop for all 3 cases, mark it with -1 as done.

      while (size--)
      {
        uint8 r = P->DataBlock[P->indexread++];
        DEBUG_LOG(0, "profile hle:%02x to %08x from index:%d state:%d", r, a4, P->indexread - 1, P->StateMachineStep);
        lisa_wb_ram(a4++, r);
      }

      ALERT_LOG(0, "returning to: %08x, a4:%08x,%08x", PC, A4, a4);
      return;
    }
  }
}

// UniPlus's kernel does a test call to a 68010 MOVEC opcode to cause an Illegal instruction
// exception on 680000 on purpose. This is used to test if it's running on a 68010. Using this
// as a UniPlus detector in order to patch it.
// Tried to patch the OS when it accesses the profile, however that happens too late and UniPlus
// ASSERT still gets triggered.  2021.03.06.
//
// I suspect there is 68010 support in the UniPlus kernel, but this is not evidence that the
// Lisa ever actually used an 010, but rather this is for cross system compatibility with other
// m68k systems that UniPlus ported to. I believe this is the case because I also randomly see
// some drivers use very exacting CPU timing delays which either use DBRA loops when interfacing
// with hardware or use NOPs. An 010 in a physical Lisa would throw off these time delay loops,
// and certainly when James Denton tried to use an 010 in a Lisa, one of the first things to fail
// was the very tight timing loop in the ROM that fetches the serial number of the Lisa. A similar
// one is used in uniplus boot loader, which also fails with "Can't get serial number." or such.

void apply_uniplus_hacks(void)
{
  ALERT_LOG(0, ".");
  check_running_lisa_os();
  if (running_lisa_os == 0) // this is zero when the 68010 opcode is executed, becuse the kernel isn't yet done initializing.
  {
    uint8 r = lisa_rb_ram(0x00020f9c); // check to see that we're on uniplus v1.4 or 1.1 or some other thing.

    ALERT_LOG(0, "**** UniPlus bootstrap - MOVEC detected, r=%02x running_lisa_os =%d ****", r, running_lisa_os);

#ifdef DEBUG
// debug_on("uniplus");
// find_uniplus_partition_table();
#endif

    DEBUG_LOG(0, "**** UniPlus bootstrap - MOVEC detected, r=%02x running_lisa_os =%d ****", r, running_lisa_os);

    // v1.1 /sunix patch
    if (r == 0x60)
    {
      ALERT_LOG(0, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
      ALERT_LOG(0, "Patching for UniPlus v1.1 sunix");
      ALERT_LOG(0, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

      lisa_wb_ram(0x0001fe24, 0x60);   // skip assert BSY 1
      lisa_wb_ram(0x0001ff38, 0x01);   // increase timeout to ludicrous length
      lisa_wl_ram(0x0000c188, 0xf33d); // time speedup patch   TST.W      $00022d46/BNE.W      $0000c182

      uniplus_hacks = 0; // turn off short-circuit logic-AND flag
    }

    // v1.4 /unix patch
    if (r == 0x67)
    { // v1.4 this checks to ensure that the pro.c driver is loaded before we modify it. Likely not necessary.
      ALERT_LOG(0, "====================================================================");
      ALERT_LOG(0, "Patching for UNIPLUS v1.4");
      ALERT_LOG(0, "====================================================================");

      // these two are needed to pass handshaking in Uni+ with our shitty profile emulation
      lisa_wb_ram(0x00020f9c, 0x60);   // skip assert BSY
      lisa_wb_ram(0x000210b0, 0x01);   // increase timeout to ludicrous length
      lisa_wl_ram(0x0000c188, 0xf33d); // time speedup patch   TST.W      $00022d46/BNE.W      $0000c182

      // these are optional for HLE acceleration of ProFile reads/writes
      if (hle)
      {
        ALERT_LOG(0, "#  # #    #### Patching for UNIPLUS v1.4 HLE");
        ALERT_LOG(0, "#  # #    #    Patching for UNIPLUS v1.4 HLE");
        ALERT_LOG(0, "#### #    #### Patching for UNIPLUS v1.4 HLE");
        ALERT_LOG(0, "#  # #    #    Patching for UNIPLUS v1.4 HLE");
        ALERT_LOG(0, "#  # #### #### Patching for UNIPLUS v1.4 HLE");

        // profile HLE patches
        lisa_ww_ram(0x00020c64, 0xf33d); // read 4 status bytes into A4, increase a4+=4, PC=00020c74
        lisa_ww_ram(0x00020d1e, 0xf33d); // read 20 bytes of tags into *a4, increase a4+=20, D5= 0x0000ffff PC=00020d26
        lisa_ww_ram(0x00020d3e, 0xf33d); // read 512 bytes of data into *a4, a4+=512 D5=0x0000ffff PC=0x00020d72
        lisa_ww_ram(0x00020ebc, 0xf33d); // wrtite 512+20 bytes of data+tags in one shot. into *a4, a4+=512 D5=0x0000ffff PC=0x00020ef0

        // putchar intercept for terminal
        if (consoletermwindow)
        {
          lisa_ww_ram(0x000236c6, 0xf33d);
          init_terminal_serial_port(CONSOLETERM); // open terminal window for console
        }
      }
      uniplus_hacks = 0; // turn off short-circuit logic-AND flag
    }
  }

  else

  {
    if (running_lisa_os > 1)
      uniplus_hacks = 0; // turn off short-circuit logic-AND flag if OS is not uniplus
  }
}

// return 1 will instruct the interrupt to code to skip processing, return 0 will wind up throwin an F-line exception
// back to the guest OS as usual.
int line15_hle_intercept(void)
{
  uint16 opcode, fn;

  if (romless && (PC & 0x00ff0000) == 0x00fe0000)
  {
    if (romless_entry())
      return 1;
  }

  abort_opcode = 2;
  opcode = fetchword(PC); // if (abort_opcode==1) fn=0xffff;
  abort_opcode = 2;
  fn = fetchword(PC + 2); // if (abort_opcode==1) fn=0xffff;
  abort_opcode = 0;

  ALERT_LOG(0, "Possible HLE call at %08x opcode:%04x", PC, opcode);

  if (opcode == 0xf33d)
  {
    hle_intercept();
    return 1;
  }

  if (opcode == 0xfeed)
  {
    ALERT_LOG(0, "F-Line-Wormhole %04lx", (long)fn);
    switch (fn)
    {
#ifdef DEBUG

    case 0:
      if (debug_log_enabled)
      {
        ALERT_LOG(0, "->debug_off");
      }
      else
      {
        ALERT_LOG(0, "->debug_off, was already off!!!");
      }
      debug_off();
      PC += 4;
      return 1;

    case 1:
      if (debug_log_enabled)
      {
        ALERT_LOG(0, "->tracelog, was already on! turning off!");
        debug_off();
      }
      else
      {
        ALERT_LOG(0, "->tracelog on");
        debug_on("F-line-wormhole");
      }
      PC += 4;
      return 1;
#else

    case 0:
    case 1:
      PC += 4;
      ALERT_LOG(0, "tracelog -> but emulator wasn't compiled in with debugging enabled!");
      return 1;
#endif
    default:
      ALERT_LOG(0, "Unknown F-Line-wormhole:%04x or actual F-Line trap, falling through a LINE15 exception will occur.", fn);
      // anything unimplemented/undefined should fall through and generate an exception.
    }
  }
  if (opcode == 0xfeef)
  {
    ALERT_LOG(0, "Executing blank IPC @ %08lx", (long)PC);
    EXITR(99, 0, "EEEK! Trying to execute blank IPC at %08lx - something is horribly wrong!", (long)PC);
  }
  ALERT_LOG(0, "Unhandled F-Line %04lx at %08lx", (long)opcode, (long)PC);
  return 0;
}
