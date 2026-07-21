/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.6      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2007 Ray A. Arachelian                          *
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
*            "A30N" version of MMU code - Memory and I/O access functions              *
*                                                                                      *
\**************************************************************************************/

// uint8 *(*mem68k_memptr[MAX_LISA_MFN]) (uint32 addr);
// uint8 (*mem68k_fetch_byte[MAX_LISA_MFN]) (uint32 addr);
// uint16 (*mem68k_fetch_word[MAX_LISA_MFN]) (uint32 addr);
// uint32 (*mem68k_fetch_long[MAX_LISA_MFN]) (uint32 addr);
// void (*mem68k_store_byte[MAX_LISA_MFN]) (uint32 addr, uint8 data);
// void (*mem68k_store_word[MAX_LISA_MFN]) (uint32 addr, uint16 data);
// void (*mem68k_store_long[MAX_LISA_MFN]) (uint32 addr, uint32 data);

// fn protos

uint8 *lisa_mptr_OxERROR(uint32 addr);
uint8 lisa_rb_OxERROR(uint32 addr);
uint16 lisa_rw_OxERROR(uint32 addr);
uint32 lisa_rl_OxERROR(uint32 addr);
void lisa_wb_OxERROR(uint32 addr, uint8 data);
void lisa_ww_OxERROR(uint32 addr, uint16 data);
void lisa_wl_OxERROR(uint32 addr, uint32 data);

uint8 *lisa_mptr_OxUnused(uint32 addr);
uint8 lisa_rb_OxUnused(uint32 addr);
uint16 lisa_rw_OxUnused(uint32 addr);
uint32 lisa_rl_OxUnused(uint32 addr);
void lisa_wb_OxUnused(uint32 addr, uint8 data);
void lisa_ww_OxUnused(uint32 addr, uint16 data);
void lisa_wl_OxUnused(uint32 addr, uint32 data);

uint8 *lisa_mptr_Ox0000_slot1(uint32 addr);
uint8 lisa_rb_Ox0000_slot1(uint32 addr);
uint16 lisa_rw_Ox0000_slot1(uint32 addr);
uint32 lisa_rl_Ox0000_slot1(uint32 addr);
void lisa_wb_Ox0000_slot1(uint32 addr, uint8 data);
void lisa_ww_Ox0000_slot1(uint32 addr, uint16 data);
void lisa_wl_Ox0000_slot1(uint32 addr, uint32 data);

uint8 *lisa_mptr_Ox2000_slot1(uint32 addr);
uint8 lisa_rb_Ox2000_slot1(uint32 addr);
uint16 lisa_rw_Ox2000_slot1(uint32 addr);
uint32 lisa_rl_Ox2000_slot1(uint32 addr);
void lisa_wb_Ox2000_slot1(uint32 addr, uint8 data);
void lisa_ww_Ox2000_slot1(uint32 addr, uint16 data);
void lisa_wl_Ox2000_slot1(uint32 addr, uint32 data);

uint8 *lisa_mptr_Ox4000_slot2(uint32 addr);
uint8 lisa_rb_Ox4000_slot2(uint32 addr);
uint16 lisa_rw_Ox4000_slot2(uint32 addr);
uint32 lisa_rl_Ox4000_slot2(uint32 addr);
void lisa_wb_Ox4000_slot2(uint32 addr, uint8 data);
void lisa_ww_Ox4000_slot2(uint32 addr, uint16 data);
void lisa_wl_Ox4000_slot2(uint32 addr, uint32 data);

uint8 *lisa_mptr_Ox6000_slot2(uint32 addr);
uint8 lisa_rb_Ox6000_slot2(uint32 addr);
uint16 lisa_rw_Ox6000_slot2(uint32 addr);
uint32 lisa_rl_Ox6000_slot2(uint32 addr);
void lisa_wb_Ox6000_slot2(uint32 addr, uint8 data);
void lisa_ww_Ox6000_slot2(uint32 addr, uint16 data);
void lisa_wl_Ox6000_slot2(uint32 addr, uint32 data);

uint8 *lisa_mptr_Ox8000_slot3(uint32 addr);
uint8 lisa_rb_Ox8000_slot3(uint32 addr);
uint16 lisa_rw_Ox8000_slot3(uint32 addr);
uint32 lisa_rl_Ox8000_slot3(uint32 addr);
void lisa_wb_Ox8000_slot3(uint32 addr, uint8 data);
void lisa_ww_Ox8000_slot3(uint32 addr, uint16 data);
void lisa_wl_Ox8000_slot3(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxa000_slot3(uint32 addr);
uint8 lisa_rb_Oxa000_slot3(uint32 addr);
uint16 lisa_rw_Oxa000_slot3(uint32 addr);
uint32 lisa_rl_Oxa000_slot3(uint32 addr);
void lisa_wb_Oxa000_slot3(uint32 addr, uint8 data);
void lisa_ww_Oxa000_slot3(uint32 addr, uint16 data);
void lisa_wl_Oxa000_slot3(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxc000_flopmem(uint32 addr);
uint8 lisa_rb_Oxc000_flopmem(uint32 addr);
uint16 lisa_rw_Oxc000_flopmem(uint32 addr);
uint32 lisa_rl_Oxc000_flopmem(uint32 addr);
void lisa_wb_Oxc000_flopmem(uint32 addr, uint8 data);
void lisa_ww_Oxc000_flopmem(uint32 addr, uint16 data);
void lisa_wl_Oxc000_flopmem(uint32 addr, uint32 data);

/***
uint8  *lisa_mptr_Oxd000_contrast(uint32 addr);
uint8  lisa_rb_Oxd000_contrast(uint32 addr);
uint16 lisa_rw_Oxd000_contrast(uint32 addr);
uint32 lisa_rl_Oxd000_contrast(uint32 addr);
void   lisa_wb_Oxd000_contrast(uint32 addr, uint8 data);
void   lisa_ww_Oxd000_contrast(uint32 addr, uint16 data);
void   lisa_wl_Oxd000_contrast(uint32 addr, uint32 data);
****/
uint8 *lisa_mptr_Oxd200_sccz8530(uint32 addr);
uint8 lisa_rb_Oxd200_sccz8530(uint32 addr);
uint16 lisa_rw_Oxd200_sccz8530(uint32 addr);
uint32 lisa_rl_Oxd200_sccz8530(uint32 addr);
void lisa_wb_Oxd200_sccz8530(uint32 addr, uint8 data);
void lisa_ww_Oxd200_sccz8530(uint32 addr, uint16 data);
void lisa_wl_Oxd200_sccz8530(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxd800_par_via2(uint32 addr);
uint8 lisa_rb_Oxd800_par_via2(uint32 addr);
uint16 lisa_rw_Oxd800_par_via2(uint32 addr);
uint32 lisa_rl_Oxd800_par_via2(uint32 addr);
void lisa_wb_Oxd800_par_via2(uint32 addr, uint8 data);
void lisa_ww_Oxd800_par_via2(uint32 addr, uint16 data);
void lisa_wl_Oxd800_par_via2(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxdc00_cops_via1(uint32 addr);
uint8 lisa_rb_Oxdc00_cops_via1(uint32 addr);
uint16 lisa_rw_Oxdc00_cops_via1(uint32 addr);
uint32 lisa_rl_Oxdc00_cops_via1(uint32 addr);
void lisa_wb_Oxdc00_cops_via1(uint32 addr, uint8 data);
void lisa_ww_Oxdc00_cops_via1(uint32 addr, uint16 data);
void lisa_wl_Oxdc00_cops_via1(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxe000_latches(uint32 addr);
uint8 lisa_rb_Oxe000_latches(uint32 addr);
uint16 lisa_rw_Oxe000_latches(uint32 addr);
uint32 lisa_rl_Oxe000_latches(uint32 addr);
void lisa_wb_Oxe000_latches(uint32 addr, uint8 data);
void lisa_ww_Oxe000_latches(uint32 addr, uint16 data);
void lisa_wl_Oxe000_latches(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxe800_videlatch(uint32 addr);
uint8 lisa_rb_Oxe800_videlatch(uint32 addr);
uint16 lisa_rw_Oxe800_videlatch(uint32 addr);
uint32 lisa_rl_Oxe800_videlatch(uint32 addr);
void lisa_wb_Oxe800_videlatch(uint32 addr, uint8 data);
void lisa_ww_Oxe800_videlatch(uint32 addr, uint16 data);
void lisa_wl_Oxe800_videlatch(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxf000_memerror(uint32 addr);
uint8 lisa_rb_Oxf000_memerror(uint32 addr);
uint16 lisa_rw_Oxf000_memerror(uint32 addr);
uint32 lisa_rl_Oxf000_memerror(uint32 addr);
void lisa_wb_Oxf000_memerror(uint32 addr, uint8 data);
void lisa_ww_Oxf000_memerror(uint32 addr, uint16 data);
void lisa_wl_Oxf000_memerror(uint32 addr, uint32 data);

uint8 *lisa_mptr_Oxf800_statreg(uint32 addr);
uint8 lisa_rb_Oxf800_statreg(uint32 addr);
uint16 lisa_rw_Oxf800_statreg(uint32 addr);
uint32 lisa_rl_Oxf800_statreg(uint32 addr);
void lisa_wb_Oxf800_statreg(uint32 addr, uint8 data);
void lisa_ww_Oxf800_statreg(uint32 addr, uint16 data);
void lisa_wl_Oxf800_statreg(uint32 addr, uint32 data);

uint8 *lisa_mptr_ram(uint32 addr);
uint8 lisa_rb_ram(uint32 addr);
uint16 lisa_rw_ram(uint32 addr);
uint32 lisa_rl_ram(uint32 addr);
void lisa_wb_ram(uint32 addr, uint8 data);
void lisa_ww_ram(uint32 addr, uint16 data);
void lisa_wl_ram(uint32 addr, uint32 data);

uint8 *lisa_mptr_ro_violn(uint32 addr);
uint8 lisa_rb_ro_violn(uint32 addr);
uint16 lisa_rw_ro_violn(uint32 addr);
uint32 lisa_rl_ro_violn(uint32 addr);
void lisa_wb_ro_violn(uint32 addr, uint8 data);
void lisa_ww_ro_violn(uint32 addr, uint16 data);
void lisa_wl_ro_violn(uint32 addr, uint32 data);

uint8 *lisa_mptr_bad_page(uint32 addr);
uint8 lisa_rb_bad_page(uint32 addr);
uint16 lisa_rw_bad_page(uint32 addr);
uint32 lisa_rl_bad_page(uint32 addr);
void lisa_wb_bad_page(uint32 addr, uint8 data);
void lisa_ww_bad_page(uint32 addr, uint16 data);
void lisa_wl_bad_page(uint32 addr, uint32 data);

uint8 *lisa_mptr_sio_rom(uint32 addr);
uint8 lisa_rb_sio_rom(uint32 addr);
uint16 lisa_rw_sio_rom(uint32 addr);
uint32 lisa_rl_sio_rom(uint32 addr);
void lisa_wb_sio_rom(uint32 addr, uint8 data);
void lisa_ww_sio_rom(uint32 addr, uint16 data);
void lisa_wl_sio_rom(uint32 addr, uint32 data);

uint8 *lisa_mptr_sio_mrg(uint32 addr);
uint8 lisa_rb_sio_mrg(uint32 addr);
uint16 lisa_rw_sio_mrg(uint32 addr);
uint32 lisa_rl_sio_mrg(uint32 addr);
void lisa_wb_sio_mrg(uint32 addr, uint8 data);
void lisa_ww_sio_mrg(uint32 addr, uint16 data);
void lisa_wl_sio_mrg(uint32 addr, uint32 data);

uint8 *lisa_mptr_sio_mmu(uint32 addr);
uint8 lisa_rb_sio_mmu(uint32 addr);
uint16 lisa_rw_sio_mmu(uint32 addr);
uint32 lisa_rl_sio_mmu(uint32 addr);
void lisa_wb_sio_mmu(uint32 addr, uint8 data);
void lisa_ww_sio_mmu(uint32 addr, uint16 data);
void lisa_wl_sio_mmu(uint32 addr, uint32 data);

uint8 *lisa_mptr_vidram(uint32 addr);
uint8 lisa_rb_vidram(uint32 addr);
uint16 lisa_rw_vidram(uint32 addr);
uint32 lisa_rl_vidram(uint32 addr);
void lisa_wb_vidram(uint32 addr, uint8 data);
void lisa_ww_vidram(uint32 addr, uint16 data);
void lisa_wl_vidram(uint32 addr, uint32 data);

uint8 *lisa_mptr_io(uint32 addr);
uint8 lisa_rb_io(uint32 addr);
uint16 lisa_rw_io(uint32 addr);
uint32 lisa_rl_io(uint32 addr);
void lisa_wb_io(uint32 addr, uint8 data);
void lisa_ww_io(uint32 addr, uint16 data);
void lisa_wl_io(uint32 addr, uint32 data);

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

typedef enum
{
    mem_byte,
    mem_word,
    mem_long
} t_memtype;

int mem68k_init(void);

// extern uint8 *(*mem68k_memptr[MAX_LISA_MFN]) (uint32 addr);
// extern uint8 (*mem68k_fetch_byte[MAX_LISA_MFN]) (uint32 addr);
// extern uint16 (*mem68k_fetch_word[MAX_LISA_MFN]) (uint32 addr);
// extern uint32 (*mem68k_fetch_long[MAX_LISA_MFN]) (uint32 addr);
// extern void (*mem68k_store_byte[MAX_LISA_MFN]) (uint32 addr, uint8 data);
// extern void (*mem68k_store_word[MAX_LISA_MFN]) (uint32 addr, uint16 data);
// extern void (*mem68k_store_long[MAX_LISA_MFN]) (uint32 addr, uint32 data);
