/**************************************************************************************\
*                                                                                      *
*       OS and ROM symbols - used for debugging.  This list was produced from various  *
*       sources.                                                                       *
*                                                                                      *
*         This is a derrived work, no copyright is claimed on this file.               *
*                                                                                      *
\**************************************************************************************/

#define IN_SYMBOLS_C

#include <vars.h>
#ifdef DEBUG

/**************************************************************************************\
*                                                                                      *
*       Standard 680000 Vector table                                                   *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

char vectxt[128];
char *getvector(int v)
{
    switch (v)
    {

    case 0:
        sprintf(vectxt, "0x%x (%d) reset ssp", v, v);
        return vectxt; // 0
    case 1:
        sprintf(vectxt, "0x%x (%d) reset initial pc", v, v);
        return vectxt; // 4
    case 2:
        sprintf(vectxt, "0x%x (%d) bus error", v, v);
        return vectxt; // 8
    case 3:
        sprintf(vectxt, "0x%x (%d) address error", v, v);
        return vectxt; // 12
    case 4:
        sprintf(vectxt, "0x%x (%d) illegal instruction", v, v);
        return vectxt; // 16
    case 5:
        sprintf(vectxt, "0x%x (%d) zero divide", v, v);
        return vectxt;
    case 6:
        sprintf(vectxt, "0x%x (%d) chk instruction", v, v);
        return vectxt;
    case 7:
        sprintf(vectxt, "0x%x (%d) trapv", v, v);
        return vectxt;
    case 8:
        sprintf(vectxt, "0x%x (%d) priviledge violation", v, v);
        return vectxt;
    case 9:
        sprintf(vectxt, "0x%x (%d) trace", v, v);
        return vectxt;
    case 10:
        sprintf(vectxt, "0x%x (%d) A-line trap", v, v);
        return vectxt;

    case 11:
        sprintf(vectxt, "0x%x (%d) F-line trap", v, v);
        return vectxt;

    case 14:
        sprintf(vectxt, "0x%x (%d) format error (RTE/TRAP?)", v, v);
        return vectxt;

    case 15:
        sprintf(vectxt, "0x%x (%d) uninitialized interrupt vector", v, v);
        return vectxt;

    case 24:
        sprintf(vectxt, "0x%x (%d) spurious irq - buserr during irq processing causes this", v, v);
        return vectxt;

    case 25:
        sprintf(vectxt, "0x%x (%d) other (disk,VTIR,via2,profile) autovector:1 ", v, v);
        return vectxt;
    case 26:
        sprintf(vectxt, "0x%x (%d) keybd autovect #2 ", v, v);
        return vectxt;
    case 27:
        sprintf(vectxt, "0x%x (%d) slot2 autovect #3 ", v, v);
        return vectxt;
    case 28:
        sprintf(vectxt, "0x%x (%d) slot1 autovect #4 ", v, v);
        return vectxt;
    case 29:
        sprintf(vectxt, "0x%x (%d) slot0 autovect #5 ", v, v);
        return vectxt;
    case 30:
        sprintf(vectxt, "0x%x (%d) rs232 autovect #6 ", v, v);
        return vectxt;
    case 31:
        sprintf(vectxt, "0x%x (%d) NMI autovector #7 ", v, v);
        return vectxt;

    case 32:
        sprintf(vectxt, "0x%x (%d) trap #0  ", v, v);
        return vectxt;
    case 33:
        sprintf(vectxt, "0x%x (%d) trap #1  ", v, v);
        return vectxt;
    case 34:
        sprintf(vectxt, "0x%x (%d) trap #2  ", v, v);
        return vectxt;
    case 35:
        sprintf(vectxt, "0x%x (%d) trap #3  ", v, v);
        return vectxt;
    case 36:
        sprintf(vectxt, "0x%x (%d) trap #4  ", v, v);
        return vectxt;
    case 37:
        sprintf(vectxt, "0x%x (%d) trap #5 (hwlib) ", v, v);
        return vectxt;
    case 38:
        sprintf(vectxt, "0x%x (%d) trap #6 (setup mmuseg) ", v, v);
        return vectxt;
    case 39:
        sprintf(vectxt, "0x%x (%d) trap #7 (D0=%04x->SR, JMP (A0=%08x) ", v, v, (uint16)(reg68k_regs[0] & 0xffff), reg68k_regs[8]);
        return vectxt;
    case 40:
        sprintf(vectxt, "0x%x (%d) trap #8  ", v, v);
        return vectxt;
    case 41:
        sprintf(vectxt, "0x%x (%d) trap #9  ", v, v);
        return vectxt;
    case 42:
        sprintf(vectxt, "0x%x (%d) trap #a  ", v, v);
        return vectxt;
    case 43:
        sprintf(vectxt, "0x%x (%d) trap #b  ", v, v);
        return vectxt;
    case 44:
        sprintf(vectxt, "0x%x (%d) trap #c  ", v, v);
        return vectxt;
    case 45:
        sprintf(vectxt, "0x%x (%d) trap #d  ", v, v);
        return vectxt;
    case 46:
        sprintf(vectxt, "0x%x (%d) trap #e  ", v, v);
        return vectxt;
    case 47:
        sprintf(vectxt, "0x%x (%d) trap #f  ", v, v);
        return vectxt;
    }
    if (v > 63)
    {
        sprintf(vectxt, "0x%x (%d) user irq vector #%d", v, v, v - 64);
        return vectxt;
    }

    sprintf(vectxt, "0x%x (%d) unassigned/reserved", v, v);
    return vectxt;
}

/**************************************************************************************\
*                                                                                      *
*       Lisa OS library traps - From Lisa programming documentation                    *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

void lisaos_trap5(void)
{
    char *label = "unknown";

    if (!debug_log_enabled)
        return;

    switch (reg68k_regs[7] & 0x0000ffff)
    {
    case 0x00: /* 0028  */
        label = "driverin";
        break;
    case 0x68: /* 0032  */
        label = "diskdriv";
        break;
    case 0x6a: /* 0046  */
        label = "twiggydr";
        break;
    case 0x6c: /* 005a  */
        label = "disksync";
        break;
    case 0x8c: /* 006a  */
        label = "nmisync ";
        break;
    case 0xaa: /* 0076  */
        label = "copsync ";
        break;
    case 0x9e: /* 0082  */
        label = "poll    ";
        break;
    case 0x02: /* 008e  */
        label = "mouseloc";
        break;
    case 0x04: /* a4    */
        label = "mouseupd";
        break;
    case 0x06: /* b4    */
        label = "mousesca";
        break;
    case 0x08: /* c4    */
        label = "mousethr";
        break;
    case 0xa8: /* d4    */
        label = "nouseodo";
        break;
    case 0x0a: /* e4    */
        label = "cursorlo";
        break;
    case 0x0c: /* f8    */
        label = "cursortr";
        break;
    case 0x0e: /* 108   */
        label = "cursorim";
        break;
    case 0x10: /* 158   */
        label = "cursorhi";
        break;
    case 0x12: /* 164   */
        label = "cursorsh";
        break;
    case 0x14: /* 188   */
        label = "cursordi";
        break;
    case 0x6e: /* 194   */
        label = "cursorob";
        break;
    case 0x70: /* 1a0   */
        label = "cursorin";
        break;
    case 0x9c: /* 1ac   */
        label = "cursorre";
        break;
    case 0x86: /* 1b8   */
        label = "busyimag";
        break;
    case 0x88: /* 208   */
        label = "busydela";
        break;
    case 0x16: /* 218   */
        label = "framecou";
        break;
    case 0x8a: /* 228   */
        label = "screensi";
        break;
    case 0x18: /* 23e   */
        label = "screenad";
        break;
    case 0x1a: /* 24e   */
        label = "altscree";
        break;
    case 0x8e: /* 25e   */
        label = "screenke";
        break;
    case 0x90: /* 26e   */
        label = "setscree";
        break;
    case 0x1c: /* 27e   */
        label = "contrast";
        break;
    case 0x1e: /* 28e   */
        label = "setcontr";
        break;
    case 0x20: /* 29e   */
        label = "rampcont";
        break;
    case 0x82: /* 2ae   */
        label = "dimcontr";
        break;
    case 0x84: /* 2be   */
        label = "setdimco";
        break;
    case 0x22: /* 2ce   */
        label = "fadedela";
        break;
    case 0x24: /* 2de   */
        label = "setfaded";
        break;
    case 0x26: /* 2ee   */
        label = "powerdow";
        break;
    case 0x28: /* 2fa   */
        label = "powercyc";
        break;
    case 0x2a: /* 30a   */
        label = "volume  ";
        break;
    case 0x2c: /* 31a   */
        label = "setvolum";
        break;
    case 0x2e: /* 32a   */
        label = "noise   ";
        break;
    case 0x30: /* 33a   */
        label = "silece  ";
        break;
    case 0x32: /* 346   */
        label = "beep    ";
        break;
    case 0x38: /* 358   */
        label = "keyboard";
        break;
    case 0x96: /* 368   */
        label = "legends ";
        break;
    case 0x98: /* 378   */
        label = "setlegen";
        break;
    case 0x34: /* 388   */
        label = "keyisdow";
        break;
    case 0x36: /* 39c   */
        label = "keymap  ";
        break;
    case 0x3c: /* 3b8   */
        label = "keybdpee";
        break;
    case 0x92: /* 3d0   */
        label = "altkeype";
        break;
    case 0x3a: /* 3fa   */
        label = "keybdeve";
        break;
    case 0x94: /* 412   */
        label = "altkeyev";
        break;
    case 0x3e: /* 43c   */
        label = "repeatra";
        break;
    case 0x40: /* 452   */
        label = "setrepea";
        break;
    case 0x42: /* 464   */
        label = "keypushe";
        break;
    case 0x72: /* 476   */
        label = "nmikey  ";
        break;
    case 0x74: /* 486   */
        label = "setnmike";
        break;
    case 0xa0: /* 498   */
        label = "toggleke";
        break;
    case 0xa2: /* 4ab   */
        label = "settoggl";
        break;
    case 0x9a: /* 4ba   */
        label = "microtim";
        break;
    case 0x44: /* 4ca   */
        label = "timer   ";
        break;
    case 0x46: /* 4da   */
        label = "alarmass";
        break;
    case 0x48: /* 4f2   */
        label = "alarmret";
        break;
    case 0x4a: /* 502   */
        label = "alarmabs";
        break;
    case 0x4c: /* 514   */
        label = "alarmrel";
        break;
    case 0x4e: /* 526   */
        label = "alarmoff";
        break;
    case 0x50: /* 536   */
        label = "datetime";
        break;
    case 0x52: /* 558   */
        label = "setdatet";
        break;
    case 0x54: /* 57a   */
        label = "datetoti";
        break;
    case 0x58: /* 5a4   */
        label = "timestam";
        break;
    case 0x5a: /* 5b4   */
        label = "settimes";
        break;
    case 0x5c: /* 5c4   */
        label = "timetoda";
        break;
    }
    if (buglog)
        fprintf(buglog, "TRAP #5 Hardware Library call taken D7=%08x, (%s)\n", reg68k_regs[7], label);
}

/**************************************************************************************\
*                                                                                      *
*       BOOT ROM Symbols - used for debugging, from the ROM sources                    *
*                                                                                      *
\**************************************************************************************/

char *get_rom_label(uint32 pc24)
{

    // if (!debug_log_enabled) return "";

    switch (pc24 & 0x00ffff)
    {
    case 0x0000:
        return "BASE     ";
    case 0x0008:
        return "2BUSVCT   ";
    case 0x000C:
        return "3ADRVCT   ";
    case 0x0010:
        return "ILLVCT   ";
    case 0x0014:
        return "DIV0VCT  ";
    case 0x0018:
        return "CHKVCT   ";
    case 0x001C:
        return "TRAPVCT  ";
    case 0x0020:
        return "PRIVCT   ";
    case 0x0024:
        return "TRCVCT   ";
    case 0x0028:
        return "L10VCT   ";
    case 0x002C:
        return "L11VCT   ";
    case 0x0030:
        return "EXCPERR  ";
    case 0x003A:
        return "SAVEREGS ";
    case 0x003E:
        return "SAVEREG2 ";
    case 0x0052:
        return "SVCMSG   ";
    case 0x0060:
        return "SPURVCT  ";
    case 0x0064:
        return "LVL1VCT  ";
    case 0x0068:
        return "LVL2VCT  ";
    case 0x006C:
        return "LVL3VCT  ";
    case 0x0070:
        return "LVL4VCT  ";
    case 0x0074:
        return "LVL5VCT  ";
    case 0x0078:
        return "LVL6VCT  ";
    case 0x007C:
        return "LVL7V-NMI";
    case 0x0080:
        return "DORESET ";
    case 0x0084:
        return "INITMON ";
    case 0x0088:
        return "CONVRTD5";
    case 0x008C:
        return "WRTMMU  ";
    case 0x0090:
        return "PROREAD ";
    case 0x0094:
        return "TWGREAD ";
    case 0x0098:
        return "RAMTEST ";
    case 0x00A4:
        return "READMMU ";
    case 0x00A8:
        return "COPSCMD ";
    case 0x00AC:
        return "READCLK ";
    case 0x00B0:
        return "DSPDEC  ";
    case 0x00B4:
        return "CONSET2 ";
    case 0x00B8:
        return "TONE    ";
    case 0x00BC:
        return "VFYCHKSM";
    case 0x00C0:
        return "WRTSUM  ";
    case 0x00C4:
        return "RDSERN  ";
    case 0x00C8:
        return "SPIN     ";
    case 0x00CA:
        return "NMIEXCP??";
    case 0x00F6:
        return "BEGIN    ";
    case 0x0152:
        return "BEGIN2   ";
    case 0x018A:
        return "BEGIN3   ";
    case 0x0194:
        return "ROMTST   ";
    case 0x019E:
        return "DOSUM    ";
    case 0x01B0:
        return "MMUTST   ";
    case 0x01D8:
        return "MMUERR   ";
    case 0x01E0:
        return "MMULP    ";
    case 0x0200:
        return "TSTLOOP  ";
    case 0x0202:
        return "REGTST   ";
    case 0x0216:
        return "MMUINIT  ";
    case 0x022A:
        return "MMURW    ";
    case 0x023C:
        return "RWCHK1   ";
    case 0x024C:
        return "RWCHK2   ";
    case 0x0254:
        return "RWCHK3   ";
    case 0x025E:
        return "CHKBASE  ";
    case 0x0270:
        return "MMUACHK  ";
    case 0x0286:
        return "ACHK1    ";
    case 0x0290:
        return "MMUSET   ";
    case 0x029C:
        return "ACHK2    ";
    case 0x02B0:
        return "MADRERR  ";
    case 0x02B4:
        return "CHKRW    ";
    case 0x02C2:
        return "RWERR    ";
    case 0x02C6:
        return "SETMMU   ";
    case 0x02E4:
        return "LOADORG  ";
    case 0x0316:
        return "LOADLMT  ";
    case 0x0348:
        return "MMUTST2  ";
    case 0x03F4:
        return "MMUERR2  ";
    case 0x03FA:
        return "MMUERR3  ";
    case 0x0408:
        return "MMULPCHK ";
    case 0x0410:
        return "CONCHK   ";
    case 0x043E:
        return "CONOK    ";
    case 0x0440:
        return "START    ";
    case 0x0446:
        return "MEMSIZ   ";
    case 0x045E:
        return "CHKLO    ";
    case 0x04BC:
        return "SAVELO   ";
    case 0x04CE:
        return "TSTHI    ";
    case 0x04E2:
        return "CHKHI    ";
    case 0x04FA:
        return "SAVEHI   ";
    case 0x0500:
        return "WRAPXIT  ";
    case 0x0502:
        return "SIZXIT   ";
    case 0x0508:
        return "CHKMEM   ";
    case 0x0554:
        return "LOTONE   ";
    case 0x0566:
        return "RSTMMU   ";
    case 0x0590:
        return "REMAP    ";
    case 0x05A4:
        return "MAPINV   ";
    case 0x05BA:
        return "WRTMMU   ";
    case 0x05D0:
        return "READMMU  ";
    case 0x0620:
        return "MEMTST1  ";
    case 0x0654:
        return "TONEDLY  ";
    case 0x066C:
        return "INITMEM  ";
    case 0x06A6:
        return "INITVCT  ";
    case 0x06AC:
        return "SETVCTRS ";
    case 0x06E2:
        return "SETBUSVC ";
    case 0x06EC:
        return "MISC     ";
    case 0x06F8:
        return "IERR     ";
    case 0x0704:
        return "NMI      ";
    case 0x0730:
        return "NOTPE    ";
    case 0x0736:
        return "TRPERR   ";
    case 0x0742:
        return "BERR     ";
    case 0x074E:
        return "AERR     ";
    case 0x0758:
        return "EXCP0    ";
    case 0x0764:
        return "EXCP1    ";
    case 0x0774:
        return "SCCSET   ";
    // case 0x0780:   return "VIA2CHK  ";
    case 0x0780:
        return "VIA2TST  ";
    case 0x07AE:
        return "VIA2VCT  ";
    case 0x07B8:
        return "VIATST   ";
    case 0x07D0:
        return "VIARW    ";
    case 0x07F4:
        return "VIAFAIL  ";
    case 0x07F6:
        return "VIARWEND ";
    case 0x07FA:
        return "CONSET   ";
    case 0x0800:
        return "CONOFF   ";
    case 0x0802:
        return "CONSET2  ";
    case 0x0822:
        return "SCRNTST  ";
    case 0x086A:
        return "SCRNERR  ";
    case 0x086C:
        return "SCRNOK   ";
    case 0x0874:
        return "SCRNSAV  ";
    case 0x0886:
        return "SETVLTCH ";
    case 0x08A2:
        return "VIA1TST  ";
    // case 0x08A2:   return "INVTST   ";
    case 0x08B0:
        return "VIA1CHK  ";
    case 0x08DC:
        return "DSPCPURM ";
    case 0x08EA:
        return "COPSENBL ";
    case 0x090A:
        return "COPSBAD  ";
    case 0x0916:
        return "COPSVCT  ";
    case 0x0918:
        return "IOVCT    ";
    case 0x0920:
        return "CPSINIT  ";
    case 0x093E:
        return "TURNON   ";
    case 0x0956:
        return "COPSCMD  ";
    case 0x09C2:
        return "RSTSCAN  ";
    case 0x09DC:
        return "RST0     ";
    case 0x09F0:
        return "GET0     ";
    case 0x09F6:
        return "RST2     ";
    case 0x0A06:
        return "RST1     ";
    case 0x0A38:
        return "GETJMP   ";
    case 0x0A3C:
        return "RSTXIT   ";
    case 0x0A52:
        return "MSCHK    ";
    case 0x0A60:
        return "SCANERR  ";
    case 0x0A66:
        return "IOCERR   ";
    case 0x0A6A:
        return "SCANXIT  ";
    case 0x0A7E:
        return "GETDATA  ";
    case 0x0AA2:
        return "GETIT    ";
    case 0x0AAA:
        return "RSTKBD   ";
    case 0x0AC4:
        return "CLRRST   ";
    case 0x0ACC:
        return "DELAY_1  ";
    case 0x0AD4:
        return "DELAY5   ";
    case 0x0ADC:
        return "KBDDELAY ";
    case 0x0AE2:
        return "DELAY    ";
    case 0x0AE8:
        return "BEEP     ";
    case 0x0AEE:
        return "CLICK    ";
    case 0x0AF6:
        return "TONE     ";
    case 0x0B06:
        return "TONE2    ";
    case 0x0B52:
        return "SILENCE  ";
    case 0x0B5A:
        return "NOIO     ";
    case 0x0B76:
        return "NOIO2    ";
    case 0x0B8E:
        return "NOIO3    ";
    case 0x0B96:
        return "VIDTST   ";
    case 0x0BA2:
        return "VIDCHK   ";
    case 0x0BD4:
        return "VIDERR   ";
    case 0x0BE0:
        return "VIDXIT   ";
    case 0x0BF6:
        return "RDSERN   ";
    case 0x0C1C:
        return "GETBITS1 ";
    case 0x0C50:
        return "GETBITS2 ";
    case 0x0C82:
        return "GETBYTES ";
    case 0x0CB2:
        return "CHECKSUM ";
    case 0x0CF8:
        return "EXIT     ";
    case 0x0D0A:
        return "FINDSYNC ";
    case 0x0D34:
        return "GETNIBBL ";
    case 0x0D58:
        return "TAG      ";
    case 0x0D5C:
        return "PARTST   ";
    case 0x0DD2:
        return "PARERR   ";
    case 0x0DE8:
        return "PARXIT   ";
    case 0x0DF4:
        return "WWPERR   ";
    case 0x0DF8:
        return "VIA1VCT  ";
    case 0x0E02:
        return "MEMTST2  ";
    case 0x0E0E:
        return "MEMLOOP  ";
    case 0x0E4E:
        return "TSTDONE  ";
    case 0x0E6A:
        return "BASICTST ";
    // case 0x0E6A:   return "RUNTESTS ";
    case 0x0E6C:
        return "CALL3    ";
    case 0x0E84:
        return "TSTINIT  ";
    case 0x0E98:
        return "SAVRSLT  ";
    case 0x0EB0:
        return "RAMTEST  ";
    case 0x0EC0:
        return "RAMRW    ";
    case 0x0ECC:
        return "RAMCHK2  ";
    case 0x0EDE:
        return "RAMNXT   ";
    case 0x0EE6:
        return "ADRTST   ";
    case 0x0EF6:
        return "ADRCHK   ";
    case 0x0F00:
        return "ADRCLR   ";
    case 0x0F0C:
        return "RDERR    ";
    case 0x0F14:
        return "PRTYINT1 ";
    case 0x0F68:
        return "TSTSTAT  ";
    case 0x0F72:
        return "PRTYINT2 ";
    case 0x0FC6:
        return "PRIXIT   ";
    case 0x0FD0:
        return "PCERR    ";
    case 0x0FF0:
        return "GETPADDR ";
    case 0x1000:
        return "IOTST    ";
    case 0x1008:
        return "SCCTEST  ";
    case 0x101A:
        return "VECTLOOP ";
    case 0x1038:
        return "B96DATA  ";
    case 0x1048:
        return "SETSCC   ";
    case 0x1054:
        return "LPTEST   ";
    case 0x1058:
        return "SCCLOOP  ";
    case 0x1066:
        return "SCCOUT   ";
    case 0x106C:
        return "SCCLOOP2 ";
    case 0x107A:
        return "SCCIN    ";
    case 0x108A:
        return "SCCLXIT  ";
    case 0x108C:
        return "SCCLERR  ";
    case 0x108E:
        return "SCCEXIT  ";
    case 0x10BE:
        return "WRITESCC ";
    case 0x10CA:
        return "INITBDAT ";
    case 0x10CE:
        return "INITB2   ";
    case 0x10D0:
        return "RSTSCC   ";
    case 0x10EE:
        return "SCCVCT   ";
    case 0x110C:
        return "DSKTST   ";
    case 0x1174:
        return "INTERR   ";
    case 0x1180:
        return "DSKXIT   ";
    case 0x1186:
        return "DSKVCT   ";
    case 0x1198:
        return "SETTYPE  ";
    case 0x11C0:
        return "COPSCHK  ";
    case 0x11E2:
        return "SCANCPS  ";
    case 0x11EA:
        return "KEYSCAN  ";
    case 0x125E:
        return "XLATE    ";
    case 0x127A:
        return "KEYTBL   ";
    case 0x128B:
        return "TBLEND   ";
    case 0x128C:
        return "CLKTST   ";
    case 0x12A0:
        return "READCLK  ";
    case 0x12B2:
        return "RDCLK0   ";
    case 0x12D4:
        return "RDCLK1   ";
    case 0x12E2:
        return "CLKERR   ";
    case 0x12EE:
        return "CONFIG   ";
    case 0x12F6:
        return "CONFIG2  ";
    case 0x1306:
        return "RDSLOTS  ";
    case 0x1332:
        return "NOCRD1   ";
    case 0x1334:
        return "SLOT2    ";
    case 0x1350:
        return "NOCRD2   ";
    case 0x1352:
        return "SLOT3    ";
    case 0x136E:
        return "NOCRD3   ";
    case 0x1370:
        return "CFGEXIT  ";
    case 0x1380:
        return "CHKID    ";
    case 0x139A:
        return "TSTCHK   ";
    case 0x13AA:
        return "TST2     ";
    case 0x140E:
        return "EXCHK    ";
    case 0x145E:
        return "IOCHK    ";
    case 0x14EA:
        return "ERRDISP  ";
    case 0x14F8:
        return "KBDCHK   ";
    case 0x1514:
        return "MEMCHK   ";
    case 0x153C:
        return "CHKMADR  ";
    case 0x154C:
        return "SCNRSLTS ";
    case 0x1572:
        return "MERRCHK  ";
    case 0x1582:
        return "MEMERR   ";
    case 0x1590:
        return "IOSCHK   ";
    case 0x15C0:
        return "TSTXIT   ";
    case 0x15C4:
        return "TSTXIT2  ";
    case 0x15CC:
        return "GOTOMON  ";
    case 0x15DC:
        return "PMVCT    ";
    case 0x15E6:
        return "OTHER    ";
    case 0x1622:
        return "DSPCODE  ";
    case 0x1630:
        return "DSPDEC   ";
    case 0x1634:
        return "GETDIG   ";
    case 0x1668:
        return "DSPCXIT  ";
    case 0x166E:
        return "OUTCHR   ";
    case 0x1678:
        return "OUTCH    ";
    case 0x1696:
        return "OUTNIB   ";
    case 0x16A8:
        return "ALPHA    ";
    case 0x16B0:
        return "DSPCH    ";
    case 0x16B8:
        return "NOTIFY   ";
    case 0x16C8:
        return "HIPTCH   ";
    case 0x16CC:
        return "LOPTCH   ";
    case 0x16CE:
        return "SETDUR   ";
    case 0x16DE:
        return "SYSOK    ";
    case 0x16E2:
        return "DOBOOT   ";
    case 0x16E6:
        return "BOOTCHK  ";
    case 0x1754:
        return "DVCECHK  ";
    case 0x1818:
        return "PMEXIT   ";
    case 0x1820:
        return "PMERR    ";
    case 0x182E:
        return "LSTCHK   ";
    case 0x1836:
        return "CHKPM    ";
    case 0x184E:
        return "SAV2PM   ";
    case 0x187E:
        return "WRTSUM   ";
    case 0x188C:
        return "VFYCHKSM ";
    case 0x1890:
        return "CKLOOP   ";
    case 0x18AE:
        return "CKXIT    ";
    case 0x18B0:
        return "EXPAND   ";
    case 0x18D6:
        return "SEARCH   ";
    case 0x18F8:
        return "BOOTMENU ";
    case 0x1986:
        return "ICONCHK  ";
    case 0x19D6:
        return "SCNSLTS  ";
    case 0x19EE:
        return "CHKS2    ";
    case 0x1A00:
        return "CHKS3    ";
    case 0x1A14:
        return "WT4BOOT  ";
    case 0x1A36:
        return "CHKPROFI ";
    case 0x1A94:
        return "DSPMNTRY ";
    case 0x1AAA:
        return "ICONMENU ";
    case 0x1B44:
        return "CHKSLOT  ";
    case 0x1B68:
        return "CHKICONS ";
    case 0x1B94:
        return "CHKSXIT  ";
    case 0x1B9A:
        return "RDSLT    ";
    case 0x1BCC:
        return "TWGBOOT  ";
    case 0x1C24:
        return "CLRINT   ";
    case 0x1C2C:
        return "DOREAD   ";
    case 0x1C48:
        return "RDRETRY  ";
    case 0x1C6C:
        return "RDSCTR1  ";
    case 0x1C82:
        return "STRTBOOT ";
    case 0x1C98:
        return "DSKTIMER ";
    case 0x1C9A:
        return "DSKCHK   ";
    case 0x1CAA:
        return "DSKBAD   ";
    case 0x1CBC:
        return "DSKOUT   ";
    case 0x1CC0:
        return "DSKDIS   ";
    case 0x1CC4:
        return "DSKERR   ";
    case 0x1D0A:
        return "TBOOTERR ";
    case 0x1D0E:
        return "DSKERR2  ";
    case 0x1D14:
        return "DSKERR3  ";
    case 0x1D1C:
        return "SAVEXCP  ";
    case 0x1D2A:
        return "BTERR    ";
    case 0x1D46:
        return "DSABLDSK ";
    case 0x1D5C:
        return "CHKDRIVE ";
    case 0x1D70:
        return "TWGRD    ";
    case 0x1D76:
        return "TWGREAD  ";
    case 0x1DB4:
        return "XFRHDR   ";
    case 0x1DCE:
        return "XFRDATA  ";
    case 0x1DF0:
        return "TWGOUT   ";
    case 0x1DF2:
        return "TWGERR   ";
    case 0x1DFA:
        return "TWGOK    ";
    case 0x1DFE:
        return "TWGRXIT  ";
    case 0x1E04:
        return "CMDCHK   ";
    case 0x1E3E:
        return "CHKFIN   ";
    case 0x1E56:
        return "EJCTDSK  ";
    case 0x1E72:
        return "CLRFDIR  ";
    case 0x1E96:
        return "CHKFDIR  ";
    case 0x1EA6:
        return "DSPWTICO ";
    // case 0x1EA6:   return "WAITALRT ";
    case 0x1EB0:
        return "VCTRINIT ";
    case 0x1ECE:
        return "PROBOOT  ";
    case 0x1EFC:
        return "PBOOT    ";
    case 0x1F10:
        return "HDSKERR  ";
    case 0x1F3A:
        return "BOOTFAIL ";
    case 0x1F3E:
        return "BFAIL2   ";
    case 0x1F5E:
        return "HDERR2   ";
    case 0x1F66:
        return "HDERR3   ";
    case 0x1F70:
        return "PROREAD  ";
    case 0x1F82:
        return "CHKBSY   ";
    case 0x1F90:
        return "TRYRD    ";
    case 0x1FD2:
        return "RDDATA   ";
    case 0x1FDE:
        return "PROERR   ";
    case 0x1FE6:
        return "PROXIT   ";
    case 0x1FEA:
        return "PROXIT2  ";
    case 0x1FF0:
        return "PROINIT  ";
    case 0x2032:
        return "READIT   ";
    case 0x2048:
        return "STRTRD   ";
    case 0x2060:
        return "GETSTAT  ";
    case 0x2074:
        return "STRTXIT  ";
    case 0x2076:
        return "STAT01   ";
    case 0x2092:
        return "COPY6    ";
    case 0x20A0:
        return "COPY6LP  ";
    case 0x20B2:
        return "STATERR  ";
    case 0x20B6:
        return "STATXIT  ";
    case 0x20BC:
        return "FINDD2   ";
    case 0x20CE:
        return "GETRSP   ";
    case 0x20DC:
        return "RSPOK    ";
    case 0x20DE:
        return "SNDR1    ";
    case 0x20E6:
        return "FINDERR  ";
    case 0x20F6:
        return "FNDXIT   ";
    case 0x20FC:
        return "WFBSY    ";
    case 0x2100:
        return "WFBSY1   ";
    case 0x210E:
        return "SENDRSP  ";
    case 0x2122:
        return "WFNBSY   ";
    case 0x212A:
        return "WFNBSY2  ";
    case 0x2132:
        return "WFNBSY3  ";
    case 0x2138:
        return "WFNBSY1  ";
    case 0x2146:
        return "DOCRES   ";
    case 0x215E:
        return "IOSBOOT  ";
    case 0x2186:
        return "STATOK   ";
    case 0x2194:
        return "NOCRD    ";
    case 0x219E:
        return "INVID    ";
    case 0x21A2:
        return "BADBRD   ";
    case 0x21A6:
        return "SENDMSG  ";
    case 0x21CC:
        return "RDIOSLT  ";
    case 0x21EA:
        return "LOADPGM  ";
    case 0x222A:
        return "INVSUM   ";
    case 0x222C:
        return "SAVERR   ";
    case 0x2234:
        return "RDIOXIT  ";
    case 0x223A:
        return "CHKPASS  ";
    case 0x2244:
        return "CHKPAS2  ";
    case 0x2250:
        return "CLRPM    ";
    case 0x229A:
        return "CHKTIM   ";
    case 0x22EA:
        return "TWGCHK   ";
    case 0x2344:
        return "TSTERR   ";
    case 0x2364:
        return "DISINT   ";
    case 0x2376:
        return "WRTMSG   ";
    case 0x238A:
        return "CNTINC   ";
    case 0x2398:
        return "DSPTIM   ";
    case 0x23BE:
        return "NOCHG    ";
    case 0x23E8:
        return "SHUTDOWN ";
    case 0x23FC:
        return "DSCONT   ";
    case 0x2444:
        return "SELF     ";
    case 0x2448:
        return "SETERR1  ";
    case 0x244C:
        return "SETERR2  ";
    case 0x244E:
        return "DSPERR   ";
    case 0x245E:
        return "CMDERR   ";
    case 0x2466:
        return "TODSET   ";
    case 0x247C:
        return "SETXIT   ";
    case 0x247E:
        return "TWGTST   ";
    case 0x249A:
        return "TWGLOOP  ";
    case 0x24C2:
        return "TOOLONG  ";
    case 0x24C4:
        return "TERR     ";
    case 0x24CA:
        return "DSPCLK   ";
    case 0x2510:
        return "TWGDSP   ";
    case 0x2534:

        if (debug_log_enabled)
        {
            ALERT_LOG(0, "Returning to Lisa POST Monitoring, so disabling debug now.");
            debug_off();
        }

        return "INITMON  ";
    case 0x2544:
        return "INIT1    ";
    case 0x255C:
        return "INIT2    ";
    case 0x2570:
        return "INIT3    ";
    case 0x259C:
        return "MONITOR  ";
    case 0x25A4:
        return "LEVEL1   ";
    // case 0x25EC:   return "OTHRBTNS ";
    case 0x25EC:
        return "DOMENU   ";
    case 0x262A:
        return "GETL1    ";
    case 0x2652:
        return "DORESET  ";
    case 0x265E:
        return "CONTCHK  ";
    case 0x26DA:
        return "GETL1XIT ";
    case 0x26DE:
        return "LEV1LOOP ";
    case 0x26E6:
        return "GETERR   ";
    case 0x26F0:
        return "CLRSCRN  ";
    case 0x26F2:
        return "WRTSCRN  ";
    case 0x2702:
        return "READKEY  ";
    case 0x270C:
        return "SQUAWK   ";
    case 0x271A:
        return "KEYTOASC ";
    case 0x273C:
        return "LEVEL2   ";
    case 0x2744:
        return "DSPMENU  ";
    case 0x274C:
        return "GETLEV2  ";
    case 0x27A0:
        return "WRTMENU  ";
    case 0x27D0:
        return "DSPMENUB ";
    case 0x2814:
        return "MAKESVCW ";
    case 0x2836:
        return "DSPMEM   ";
    case 0x285E:
        return "RDCNT    ";
    case 0x28A2:
        return "SETMEM   ";
    case 0x28C6:
        return "RDDTA    ";
    case 0x2908:
        return "CALLRTN  ";
    case 0x2934:
        return "LOOPTST  ";
    case 0x29C0:
        return "MMUTSTE1 ";
    case 0x29CA:
        return "MEMTST3  ";
    case 0x29DC:
        return "LOOPTBL  ";
    case 0x29F4:
        return "VIDAJST  ";
    case 0x2A26:
        return "DRWHORZ  ";
    case 0x2A40:
        return "DRWVERT  ";
    case 0x2A6C:
        return "POWERCYC ";
    case 0x2A74:
        return "INVALID  ";
    case 0x2A82:
        return "LEV2LOOP ";
    case 0x2A86:
        return "INVXIT   ";
    case 0x2A92:
        return "PROMPT   ";
    case 0x2ABA:
        return "RDINPUT  ";
    case 0x2AC4:
        return "READIN   ";
    case 0x2B08:
        return "SCROLL   ";
    case 0x2B50:
        return "PUTLF    ";
    case 0x2B6C:
        return "PUTBS    ";
    case 0x2B7A:
        return "CLRIT    ";
    case 0x2B86:
        return "ENQKBD   ";
    case 0x2B96:
        return "GETCH    ";
    case 0x2BA4:
        return "GETA     ";
    case 0x2BB2:
        return "GETPARM  ";
    case 0x2BB6:
        return "READQ    ";
    case 0x2BDC:
        return "OKCH     ";
    case 0x2BE8:
        return "GETEXIT  ";
    case 0x2BEE:
        return "INVPARM  ";
    case 0x2BF2:
        return "GETXIT2  ";
    case 0x2BF4:
        return "CONVERT  ";
    case 0x2C0A:
        return "DBOXDSPL ";
    case 0x2C28:
        return "CLRDBOX  ";
    case 0x2C46:
        return "GETINPUT ";
    case 0x2C4E:
        return "GET1     ";
    case 0x2C52:
        return "CHKIT    ";
    // case 0x002CBA:   return "GET2     ";
    case 0x2CBA:
        return "WAIT2    ";
    case 0x2CBE:
        return "CHKIT2   ";
    case 0x2D0C:
        return "GET3     ";
    case 0x2D0E:
        return "WAIT3    ";
    case 0x2D38:
        return "WT4INPUT ";
    // case 0x2D38:   return "COPS0    ";
    case 0x2D48:
        return "COPS1    ";
    case 0x2D5C:
        return "COPS2    ";
    case 0x2D9E:
        return "COPS4    ";
    case 0x2DBE:
        return "READCOPS ";
    case 0x2DD8:
        return "POWEROFF ";
    case 0x2E2A:
        return "ENBLDRVS ";
    case 0x2E46:
        return "CHKPOSN  ";
    case 0x2E5A:
        return "GETNTRY  ";
    case 0x2E9C:
        return "CHKPXIT  ";
    case 0x2EA2:
        return "CHKINPUT ";
    case 0x2EAC:
        return "RDENTRY  ";
    case 0x2EC2:
        return "INVERT   ";
    case 0x2F2A:
        return "MOUSEMOV ";
    case 0x2F42:
        return "SCALE    ";
    case 0x2F56:
        return "FINE     ";
    case 0x2F6C:
        return "COARSE   ";
    case 0x2F7C:
        return "BOUNDS   ";
    case 0x2FB2:
        return "MOUSINIT ";
    case 0x2FCC:
        return "CURSORIN ";
    case 0x2FEA:
        return "CURSORHI ";
    case 0x300E:
        return "CURSORDI ";
    case 0x30D6:
        return "DRAWDESK ";
    case 0x30DA:
        return "CLRDESK  ";
    case 0x30EE:
        return "GRAY     ";
    case 0x30F4:
        return "GRAY1    ";
    case 0x3106:
        return "BLACKEN  ";
    case 0x3108:
        return "WHITEN   ";
    case 0x3114:
        return "CLRMENU  ";
    case 0x3128:
        return "PAINT_BO ";
    // case 0x3128:   return "PAINTB1  ";
    case 0x312E:
        return "INVERSE  ";
    case 0x3132:
        return "CONT     ";
    case 0x3136:
        return "PAINTB2  ";
    case 0x3148:
        return "STARTOP  ";
    case 0x314A:
        return "MOVINST  ";
    case 0x314E:
        return "EXCLUSIV ";
    case 0x3150:
        return "COMPARE  ";
    case 0x3156:
        return "NEXTLINE ";
    case 0x3164:
        return "MAKEPCAL ";
    case 0x3168:
        return "MAKEALER ";
    case 0x3180:
        return "MAKETEST ";
    case 0x31B2:
        return "MAKEDBOX ";
    case 0x31C6:
        return "MAKEWIND ";
    case 0x31E6:
        return "MAKEBOX  ";
    case 0x325A:
        return "PAINTV1  ";
    // case 0x325A:   return "PAINT_V  ";
    case 0x326A:
        return "PAINTBIT ";
    case 0x327A:
        return "MAKEBUTN ";
    case 0x32F8:
        return "DRAWBUTN ";
    case 0x3342:
        return "DRAWSIDE ";
    case 0x336A:
        return "MAKEMENU ";
    case 0x33B8:
        return "WRITETIT ";
    case 0x3406:
        return "GETROWCO ";
    case 0x3418:
        return "GETLENGT ";
    case 0x3424:
        return "DSPRGICO ";
    case 0x342C:
        return "OUTPUT   ";
    case 0x3432:
        return "LOOP0    ";
    case 0x3438:
        return "LOOP1    ";
    case 0x343A:
        return "LOOP2    ";
    case 0x3450:
        return "DSPNUMIC ";
    case 0x3464:
        return "DSPNUM   ";
    case 0x34DA:
        return "DSPERRIC ";
    case 0x34EC:
        return "DSPBAD   ";
    case 0x34FA:
        return "MRGICON  ";
    case 0x352C:
        return "DSPALRTI ";
    case 0x3556:
        return "DSPQICON ";
    case 0x3574:
        return "INVICON  ";
    case 0x3588:
        return "DSPCPU   ";
    case 0x3592:
        return "DSPMBRD  ";
    case 0x359C:
        return "DSPIOB   ";
    case 0x35A6:
        return "DSPXCRD  ";
    case 0x35AE:
        return "DODSPLY  ";
    case 0x35B6:
        return "CHKCPU   ";
    case 0x35BE:
        return "CHKMBRD  ";
    case 0x35C6:
        return "CHKIOBRD ";
    case 0x35CE:
        return "CHKXCRD  ";
    case 0x35D4:
        return "DSPCHECK ";
    case 0x35E2:
        return "DSPICON  ";
    case 0x35EC:
        return "DLOOP    ";
    case 0x35F2:
        return "MLOOP    ";
    case 0x35FC:
        return "BLACK    ";
    case 0x35FE:
        return "CHECK    ";
    case 0x360A:
        return "DONE     ";
    case 0x361A:
        return "XLOOP    ";
    case 0x3634:
        return "DSPSTRIN ";
    case 0x3686:
        return "DSPALL   ";
    case 0x36A6:
        return "DSPGERMN ";
    case 0x36AA:
        return "DSPFRNCH ";
    case 0x36B0:
        return "DSPOUT   ";
    case 0x36B8:
        return "DSPIT    ";
    case 0x36D2:
        return "DSPMSLSH ";
    case 0x36DA:
        return "DSPALRTM ";
    case 0x36EE:
        return "CONVRTD5 ";
    case 0x36F6:
        return "DSPMSGR  ";
    case 0x3700:
        return "DSPMSG   ";
    case 0x3716:
        return "DSPDONE  ";
    case 0x371A:
        return "SETCRSR  ";
    case 0x371E:
        return "SETCRSR2 ";
    case 0x373A:
        return "DSPVAL   ";
    case 0x37A0:
        return "OUT      ";
    case 0x37B4:
        return "DSPVXIT  ";
    case 0x37BA:
        return "SPACE    ";
    case 0x37C0:
        return "FONTTBL  ";
    case 0x38AA:
        return "QUESTCH  ";
    case 0x38B0:
        return "INVCHAR  ";
    case 0x38B6:
        return "APPLICON ";
    case 0x38BC:
        return "ASCIITAB ";
    case 0x391C:
        return "CRSRMASK ";
    // case 0x391C:   return "CRSRDATA ";
    case 0x393C:
        return "ONE      ";
    case 0x3941:
        return "TWO      ";
    case 0x3946:
        return "THREE    ";
    case 0x394B:
        return "IOBRD    ";
    case 0x398B:
        return "CPUBRD   ";
    case 0x39CE:
        return "MEMBRD   ";
    case 0x3A14:
        return "XCARD    ";
    case 0x3A3D:
        return "WAITICON ";
    case 0x3A9F:
        return "PROICON  ";
    case 0x3ADA:
        return "UPPER    ";
    case 0x3B1E:
        return "DRIVEN   ";
    case 0x3B56:
        return "INSERTD  ";
    case 0x3BA5:
        return "KEYBDOUT ";
    case 0x3C2B:
        return "MOUSEOUT ";
    case 0x3C65:
        return "QUESTION ";
    case 0x3C88:
        return "CHECKMRK ";
    case 0x3CD6:
        return "BADMRK   ";
    case 0x3D1E:
        return "DISKETTE ";
    case 0x3D54:
        return "LISA     ";
    case 0x3DCE:
        return "BRNMSG   ";
    case 0x3DE0:
        return "TIMMSG   ";
    case 0x3DE9:
        return "TWGMSG   ";
    case 0x3DF4:
        return "LOOPMSG  ";
    case 0x3E03:
        return "PMMSG    ";
    case 0x3E10:
        return "TWGFAIL  ";
    case 0x3E23:
        return "TWGRSLT  ";
    case 0x3E3A:
        return "CHKMSG   ";
    case 0x3E58:
        return "RTRYMSG  ";
    case 0x3E78:
        return "CONTMSG  ";
    case 0x3E98:
        return "STRTMSG  ";
    case 0x3EBD:
        return "PERIODS  ";
    case 0x3EC1:
        return "MENUHDG  ";
    case 0x3EC9:
        return "DISPMSG  ";
    case 0x3ED9:
        return "SETMSG   ";
    case 0x3EE9:
        return "CALLMSG  ";
    case 0x3EF9:
        return "LPMSG    ";
    case 0x3F09:
        return "VIDMSG   ";
    case 0x3F19:
        return "CYCLMSG  ";
    case 0x3F29:
        return "QUITMSG  ";
    case 0x3F39:
        return "MENUID   ";
    case 0x3F40:
        return "TSTMENU  ";
    case 0x3FBB:
        return "ADDRMSG  ";
    case 0x3FC5:
        return "DATAMSG  ";
    case 0x3FCC:
        return "CNTMSG   ";
    case 0x3FD4:
        return "TSTMSG   ";
    case 0x3FDB:
        return "WHATMSG  ";
    case 0x3FF4:
        return "HDGMSG   ";
    case 0x3FFC:
        return "VRSN     ";
    case 0x3FFD:
        return "REV      ";
    case 0x3FFE:
        return "LAST     ";
    default:
        return NULL;
    }
}

/**************************************************************************************\
*                                                                                      *
*       Macintosh OS Traps - used for debugging.  This list was produced from the      *
*       excellent compilation Mac Alamanac II which can be found here:                 *
*                  http://www.mac.linux-m68k.org/devel/macalmanac.php                  *
*                                                                                      *
*  This is a derrived work from the above URL, no copyright is claimed on this file.   *
*                                                                                      *
\**************************************************************************************/

char *mac_aline_traps(uint16 opcode)
{

    // Since this isn't used for anything other than debugging, we don't really care about
    // speed here.

    switch (opcode)
    {
    case 0xA000:
        return "_Open";
    case 0xA001:
        return "_Close";
    case 0xA002:
        return "_Read";
    case 0xA003:
        return "_Write";
    case 0xA004:
        return "_Control";
    case 0xA005:
        return "_Status";
    case 0xA006:
        return "_KillIO";
    case 0xA007:
        return "_GetVolInfo";
    case 0xA008:
        return "_Create";
    case 0xA009:
        return "_Delete";
    case 0xA00A:
        return "_OpenRF";
    case 0xA00B:
        return "_Rename";
    case 0xA00C:
        return "_GetFileInfo";
    case 0xA00D:
        return "_SetFileInfo";
    case 0xA00E:
        return "_UnmountVol";
    case 0xA00F:
        return "_MountVol";
    case 0xA010:
        return "_Allocate";
    case 0xA011:
        return "_GetEOF";
    case 0xA012:
        return "_SetEOF";
    case 0xA013:
        return "_FlushVol";
    case 0xA014:
        return "_GetVol";
    case 0xA015:
        return "_SetVol";
    case 0xA016:
        return "_InitQueue";
    case 0xA017:
        return "_Eject";
    case 0xA018:
        return "_GetFPos";
    case 0xA019:
        return "_InitZone";
    case 0xA01B:
        return "_SetZone";
    case 0xA01C:
        return "_FreeMem";
    case 0xA01F:
        return "_DisposPtr";
    case 0xA020:
        return "_SetPtrSize";
    case 0xA021:
        return "_GetPtrSize";
    case 0xA023:
        return "_DisposHandle";
    case 0xA024:
        return "_SetHandleSize";
    case 0xA025:
        return "_GetHandleSize";
    case 0xA027:
        return "_ReallocHandle";
    case 0xA029:
        return "_HLock";
    case 0xA02A:
        return "_HUnlock";
    case 0xA02B:
        return "_EmptyHandle";
    case 0xA02C:
        return "_InitApplZone";
    case 0xA02D:
        return "_SetApplLimit";
    case 0xA02E:
        return "_BlockMove";
    case 0xA02F:
        return "_PostEvent";
    case 0xA030:
        return "_OSEventAvail";
    case 0xA031:
        return "_GetOSEvent";
    case 0xA032:
        return "_FlushEvents";
    case 0xA033:
        return "_VInstall";
    case 0xA034:
        return "_VRemove";
    case 0xA035:
        return "_Offline";
    case 0xA036:
        return "_MoreMasters";
    case 0xA038:
        return "_WriteParam";
    case 0xA039:
        return "_ReadDateTime";
    case 0xA03A:
        return "_SetDateTime";
    case 0xA03B:
        return "_Delay";
    case 0xA03C:
        return "_CmpString";
    case 0xA03D:
        return "_DrvrInstall";
    case 0xA03E:
        return "_DrvrRemove";
    case 0xA03F:
        return "_InitUtil";
    case 0xA040:
        return "_ResrvMem";
    case 0xA041:
        return "_SetFilLock";
    case 0xA042:
        return "_RstFilLock";
    case 0xA043:
        return "_SetFilType";
    case 0xA044:
        return "_SetFPos";
    case 0xA045:
        return "_FlushFile";
    case 0xA047:
        return "_SetTrapAddress";
    case 0xA049:
        return "_HPurge";
    case 0xA04A:
        return "_HNoPurge";
    case 0xA04B:
        return "_SetGrowZone";
    case 0xA04C:
        return "_CompactMem";
    case 0xA04D:
        return "_PurgeMem";
    case 0xA04E:
        return "_AddDrive";
    case 0xA04F:
        return "_RDrvrInstall";
    case 0xA050:
        return "_RelString";
    case 0xA054:
        return "_UprString";
    case 0xA055:
        return "_StripAddress";
    case 0xA057:
        return "_SetAppBase";
    case 0xA05D:
        return "_SwapMMUMode";
    case 0xA061:
        return "_MaxBlock";
    case 0xA062:
        return "_PurgeSpace";
    case 0xA063:
        return "_MaxApplZone";
    case 0xA064:
        return "_MoveHHi";
    case 0xA065:
        return "_StackSpace";
    case 0xA066:
        return "_NewEmptyHandle";
    case 0xA067:
        return "_HSetRBit";
    case 0xA068:
        return "_HClrRBit";
    case 0xA069:
        return "_HGetState";
    case 0xA06A:
        return "_HSetState";
    case 0xA06E:
        return "_SlotManager";
    case 0xA06F:
        return "_SlotVInstall";
    case 0xA070:
        return "_SlotVRemove";
    case 0xA071:
        return "_AttachVBL";
    case 0xA072:
        return "_DoVBLTask";
    case 0xA075:
        return "_SIntInstall";
    case 0xA076:
        return "_SIntRemove";
    case 0xA077:
        return "_CountADBs";
    case 0xA078:
        return "_GetIndADB";
    case 0xA079:
        return "_GetADBInfo";
    case 0xA07A:
        return "_SetADBInfo";
    case 0xA07B:
        return "_ADBReInit";
    case 0xA07C:
        return "_ADBOp";
    case 0xA07D:
        return "_GetDefaultStartup";
    case 0xA07E:
        return "_SetDefaultStartup";
    case 0xA07F:
        return "_InternalWait";
    case 0xA080:
        return "_GetVideoDefault";
    case 0xA081:
        return "_SetVideoDefault";
    case 0xA082:
        return "_DTInstall";
    case 0xA083:
        return "_SetOSDefault";
    case 0xA084:
        return "_GetOSDefault";
    case 0xA090:
        return "_SysEnvirons";
    case 0xA11A:
        return "_GetZone";
    case 0xA11D:
        return "_MaxMem";
    case 0xA11E:
        return "_NewPtr";
    case 0xA122:
        return "_NewHandle";
    case 0xA126:
        return "_HandleZone";
    case 0xA128:
        return "_RecoverHandle";
    case 0xA12F:
        return "_PPostEvent";
    case 0xA146:
        return "_GetTrapAddress";
    case 0xA148:
        return "_PtrZone";
    case 0xA260:
        return "_HFSDispatch";
    case 0xA808:
        return "_InitProcMenu";
    case 0xA809:
        return "_GetCVariant";
    case 0xA80A:
        return "_GetWVariant";
    case 0xA80B:
        return "_PopUpMenuSelect";
    case 0xA80C:
        return "_RGetResource";
    case 0xA80D:
        return "_Count1Resources";
    case 0xA80E:
        return "_Get1IxResource";
    case 0xA80F:
        return "_Get1IxType";
    case 0xA810:
        return "_Unique1ID";
    case 0xA811:
        return "_TESelView";
    case 0xA812:
        return "_TEPinScroll";
    case 0xA813:
        return "_TEAutoView";
    case 0xA815:
        return "_SCSIDispatch";
    case 0xA816:
        return "_Pack8";
    case 0xA817:
        return "_CopyMask";
    case 0xA818:
        return "_FixAtan2";
    case 0xA81C:
        return "_Count1Types";
    case 0xA81F:
        return "_Get1Resource";
    case 0xA820:
        return "_Get1NamedResource";
    case 0xA821:
        return "_MaxSizeRsrc";
    case 0xA826:
        return "_InsMenuItem";
    case 0xA827:
        return "_HideDItem";
    case 0xA828:
        return "_ShowDItem";
    case 0xA82B:
        return "_Pack9";
    case 0xA82C:
        return "_Pack10";
    case 0xA82D:
        return "_Pack11";
    case 0xA82E:
        return "_Pack12";
    case 0xA82F:
        return "_Pack13";
    case 0xA830:
        return "_Pack14";
    case 0xA831:
        return "_Pack15";
    case 0xA834:
        return "_SetFScaleDisable";
    case 0xA835:
        return "_FontMetrics";
    case 0xA837:
        return "_MeasureText";
    case 0xA838:
        return "_CalcMask";
    case 0xA839:
        return "_SeedFill";
    case 0xA83A:
        return "_ZoomWindow";
    case 0xA83B:
        return "_TrackBox";
    case 0xA83C:
        return "_TEGetOffset";
    case 0xA83D:
        return "_TEDispatch";
    case 0xA83E:
        return "_TEStyleNew";
    case 0xA83F:
        return "_Long2Fix";
    case 0xA840:
        return "_Fix2Long";
    case 0xA841:
        return "_Fix2Frac";
    case 0xA842:
        return "_Frac2Fix";
    case 0xA843:
        return "_Fix2X";
    case 0xA844:
        return "_X2Fix";
    case 0xA845:
        return "_Frac2X";
    case 0xA846:
        return "_X2Frac";
    case 0xA847:
        return "_FracCos";
    case 0xA848:
        return "_FracSin";
    case 0xA849:
        return "_FracSqrt";
    case 0xA84A:
        return "_FracMul";
    case 0xA84B:
        return "_FracDiv";
    case 0xA84D:
        return "_FixDiv";
    case 0xA84E:
        return "_GetItemCmd";
    case 0xA84F:
        return "_SetItemCmd";
    case 0xA850:
        return "_InitCursor";
    case 0xA851:
        return "_SetCursor";
    case 0xA852:
        return "_HideCursor";
    case 0xA853:
        return "_ShowCursor";
    case 0xA855:
        return "_ShieldCursor";
    case 0xA856:
        return "_ObscureCursor";
    case 0xA858:
        return "_BitAnd";
    case 0xA859:
        return "_BitXor";
    case 0xA85A:
        return "_BitNot";
    case 0xA85B:
        return "_BitOr";
    case 0xA85C:
        return "_BitShift";
    case 0xA85D:
        return "_BitTst";
    case 0xA85E:
        return "_BitSet";
    case 0xA85F:
        return "_BitClr";
    case 0xA861:
        return "_Random";
    case 0xA862:
        return "_ForeColor";
    case 0xA863:
        return "_BackColor";
    case 0xA864:
        return "_ColorBit";
    case 0xA865:
        return "_GetPixel";
    case 0xA866:
        return "_StuffHex";
    case 0xA867:
        return "_LongMul";
    case 0xA868:
        return "_FixMul";
    case 0xA869:
        return "_FixRatio";
    case 0xA86A:
        return "_HiWord";
    case 0xA86B:
        return "_LoWord";
    case 0xA86C:
        return "_FixRound";
    case 0xA86D:
        return "_InitPort";
    case 0xA86E:
        return "_InitGraf";
    case 0xA86F:
        return "_OpenPort";
    case 0xA870:
        return "_LocalToGlobal";
    case 0xA871:
        return "_GlobalToLocal";
    case 0xA872:
        return "_GrafDevice";
    case 0xA873:
        return "_SetPort";
    case 0xA874:
        return "_GetPort";
    case 0xA875:
        return "_SetPBits";
    case 0xA876:
        return "_PortSize";
    case 0xA877:
        return "_MovePortTo";
    case 0xA878:
        return "_SetOrigin";
    case 0xA879:
        return "_SetClip";
    case 0xA87A:
        return "_GetClip";
    case 0xA87B:
        return "_ClipRect";
    case 0xA87C:
        return "_BackPat";
    case 0xA87D:
        return "_ClosePort";
    case 0xA87E:
        return "_AddPt";
    case 0xA87F:
        return "_SubPt";
    case 0xA880:
        return "_SetPt";
    case 0xA881:
        return "_EqualPt";
    case 0xA882:
        return "_StdText";
    case 0xA883:
        return "_DrawChar";
    case 0xA884:
        return "_DrawString";
    case 0xA885:
        return "_DrawText";
    case 0xA886:
        return "_TextWidth";
    case 0xA887:
        return "_TextFont";
    case 0xA888:
        return "_TextFace";
    case 0xA889:
        return "_TextMode";
    case 0xA88A:
        return "_TextSize";
    case 0xA88B:
        return "_GetFontInfo";
    case 0xA88C:
        return "_StringWidth";
    case 0xA88D:
        return "_CharWidth";
    case 0xA88E:
        return "_SpaceExtra";
    case 0xA890:
        return "_StdLine";
    case 0xA891:
        return "_LineTo";
    case 0xA892:
        return "_Line";
    case 0xA893:
        return "_MoveTo";
    case 0xA894:
        return "_Move";
    case 0xA895:
        return "_Shutdown";
    case 0xA896:
        return "_HidePen";
    case 0xA897:
        return "_ShowPen";
    case 0xA898:
        return "_GetPenState";
    case 0xA899:
        return "_SetPenState";
    case 0xA89A:
        return "_GetPen";
    case 0xA89B:
        return "_PenSize";
    case 0xA89C:
        return "_PenMode";
    case 0xA89D:
        return "_PenPat";
    case 0xA89E:
        return "_PenNormal";
    case 0xA8A0:
        return "_StdRect";
    case 0xA8A1:
        return "_FrameRect";
    case 0xA8A2:
        return "_PaintRect";
    case 0xA8A3:
        return "_EraseRect";
    case 0xA8A4:
        return "_InverRect";
    case 0xA8A5:
        return "_FillRect";
    case 0xA8A6:
        return "_EqualRect";
    case 0xA8A7:
        return "_SetRect";
    case 0xA8A8:
        return "_OffsetRect";
    case 0xA8A9:
        return "_InsetRect";
    case 0xA8AA:
        return "_SectRect";
    case 0xA8AB:
        return "_UnionRect";
    case 0xA8AC:
        return "_Pt2Rect";
    case 0xA8AD:
        return "_PtInRect";
    case 0xA8AE:
        return "_EmptyRect";
    case 0xA8AF:
        return "_StdRRect";
    case 0xA8B1:
        return "_PaintRoundRect";
    case 0xA8B2:
        return "_EraseRoundRect";
    case 0xA8B3:
        return "_InverRoundRect";
    case 0xA8B4:
        return "_FillRoundRect";
    case 0xA8B5:
        return "_ScriptUtil";
    case 0xA8B6:
        return "_StdOval";
    case 0xA8B7:
        return "_FrameOval";
    case 0xA8B8:
        return "_PaintOval";
    case 0xA8B9:
        return "_EraseOval";
    case 0xA8BA:
        return "_InvertOval";
    case 0xA8BB:
        return "_FillOval";
    case 0xA8BC:
        return "_SlopeFromAngle";
    case 0xA8BD:
        return "_StdArc";
    case 0xA8BE:
        return "_FrameArc";
    case 0xA8BF:
        return "_PaintArc";
    case 0xA8C0:
        return "_EraseArc";
    case 0xA8C1:
        return "_InvertArc";
    case 0xA8C2:
        return "_FillArc";
    case 0xA8C3:
        return "_PtToAngle";
    case 0xA8C4:
        return "_AngleFromSlope";
    case 0xA8C5:
        return "_StdPoly";
    case 0xA8C6:
        return "_FramePoly";
    case 0xA8C7:
        return "_PaintPoly";
    case 0xA8C8:
        return "_ErasePoly";
    case 0xA8C9:
        return "_InvertPoly";
    case 0xA8CA:
        return "_FillPoly";
    case 0xA8CB:
        return "_OpenPoly";
    case 0xA8CC:
        return "_ClosePgon";
    case 0xA8CD:
        return "_KillPoly";
    case 0xA8CE:
        return "_OffsetPoly";
    case 0xA8CF:
        return "_PackBits";
    case 0xA8D0:
        return "_UnpackBits";
    case 0xA8D1:
        return "_StdRgn";
    case 0xA8D2:
        return "_FrameRgn";
    case 0xA8D3:
        return "_PaintRgn";
    case 0xA8D4:
        return "_EraseRgn";
    case 0xA8D5:
        return "_InverRgn";
    case 0xA8D6:
        return "_FillRgn";
    case 0xA8D8:
        return "_NewRgn";
    case 0xA8D9:
        return "_DisposRgn";
    case 0xA8DA:
        return "_OpenRgn";
    case 0xA8DB:
        return "_CloseRgn";
    case 0xA8DC:
        return "_CopyRgn";
    case 0xA8DD:
        return "_SetEmptyRgn";
    case 0xA8DE:
        return "_SetRecRgn";
    case 0xA8DF:
        return "_RectRgn";
    case 0xA8E0:
        return "_OfsetRgn";
    case 0xA8E1:
        return "_InsetRgn";
    case 0xA8E2:
        return "_EmptyRgn";
    case 0xA8E3:
        return "_EqualRgn";
    case 0xA8E4:
        return "_SectRgn";
    case 0xA8E5:
        return "_UnionRgn";
    case 0xA8E6:
        return "_DiffRgn";
    case 0xA8E7:
        return "_XorRgn";
    case 0xA8E8:
        return "_PtInRgn";
    case 0xA8E9:
        return "_RectInRgn";
    case 0xA8EA:
        return "_SetStdProcs";
    case 0xA8EB:
        return "_StdBits";
    case 0xA8EC:
        return "_CopyBits";
    case 0xA8ED:
        return "_StdTxMeas";
    case 0xA8EE:
        return "_StdGetPic";
    case 0xA8EF:
        return "_ScrollRect";
    case 0xA8F0:
        return "_StdPutPic";
    case 0xA8F1:
        return "_StdComment";
    case 0xA8F2:
        return "_PicComment";
    case 0xA8F3:
        return "_OpenPicture";
    case 0xA8F4:
        return "_ClosePicture";
    case 0xA8F5:
        return "_KillPicture";
    case 0xA8F6:
        return "_DrawPicture";
    case 0xA8F8:
        return "_ScalePt";
    case 0xA8F9:
        return "_MapPt";
    case 0xA8FA:
        return "_MapRect";
    case 0xA8FB:
        return "_MapRgn";
    case 0xA8FC:
        return "_MapPoly";
    case 0xA8FE:
        return "_InitFonts";
    case 0xA8FF:
        return "_GetFName";
    case 0xA900:
        return "_GetFNum";
    case 0xA901:
        return "_FMSwapFont";
    case 0xA902:
        return "_RealFont";
    case 0xA903:
        return "_SetFontLock";
    case 0xA904:
        return "_DrawGrowIcon";
    case 0xA905:
        return "_DragGrayRgn";
    case 0xA906:
        return "_NewString";
    case 0xA907:
        return "_SetString";
    case 0xA908:
        return "_ShowHide";
    case 0xA909:
        return "_CalcVis";
    case 0xA90A:
        return "_CalcVBehind";
    case 0xA90B:
        return "_ClipAbove";
    case 0xA90C:
        return "_PaintOne";
    case 0xA90D:
        return "_PaintBehind";
    case 0xA90E:
        return "_SaveOld";
    case 0xA90F:
        return "_DrawNew";
    case 0xA910:
        return "_GetWMgrPort";
    case 0xA911:
        return "_CheckUpdate";
    case 0xA912:
        return "_InitWindows";
    case 0xA913:
        return "_NewWindow";
    case 0xA914:
        return "_DisposWindow";
    case 0xA915:
        return "_ShowWindow";
    case 0xA916:
        return "_HideWindow";
    case 0xA917:
        return "_GetWRefCon";
    case 0xA918:
        return "_SetWRefCon";
    case 0xA919:
        return "_GetWTitle";
    case 0xA91A:
        return "_SetWTitle";
    case 0xA91B:
        return "_MoveWindow";
    case 0xA91C:
        return "_HiliteWindow";
    case 0xA91D:
        return "_SizeWindow";
    case 0xA91E:
        return "_TrackGoAway";
    case 0xA91F:
        return "_SelectWindow";
    case 0xA920:
        return "_BringToFront";
    case 0xA921:
        return "_SendBehind";
    case 0xA922:
        return "_BeginUpdate";
    case 0xA923:
        return "_EndUpdate";
    case 0xA924:
        return "_FrontWindow";
    case 0xA925:
        return "_DragWindow";
    case 0xA926:
        return "_DragTheRgn";
    case 0xA927:
        return "_InvalRgn";
    case 0xA928:
        return "_InvalRect";
    case 0xA929:
        return "_ValidRgn";
    case 0xA92A:
        return "_ValidRect";
    case 0xA92B:
        return "_GrowWindow";
    case 0xA92C:
        return "_FindWindow";
    case 0xA92D:
        return "_CloseWindow";
    case 0xA92E:
        return "_SetWindowPic";
    case 0xA92F:
        return "_GetWindowPic";
    case 0xA930:
        return "_InitMenus";
    case 0xA931:
        return "_NewMenu";
    case 0xA932:
        return "_DisposMenu";
    case 0xA933:
        return "_AppendMenu";
    case 0xA934:
        return "_ClearMenuBar";
    case 0xA935:
        return "_InsertMenu";
    case 0xA936:
        return "_DeleteMenu";
    case 0xA937:
        return "_DrawMenuBar";
    case 0xA938:
        return "_HiliteMenu";
    case 0xA939:
        return "_EnableItem";
    case 0xA93A:
        return "_DisableItem";
    case 0xA93B:
        return "_GetMenuBar";
    case 0xA93C:
        return "_SetMenuBar";
    case 0xA93D:
        return "_MenuSelect";
    case 0xA93E:
        return "_MenuKey";
    case 0xA93F:
        return "_GetItmIcon";
    case 0xA940:
        return "_SetItmIcon";
    case 0xA941:
        return "_GetItmStyle";
    case 0xA942:
        return "_SetItmStyle";
    case 0xA943:
        return "_GetItmMark";
    case 0xA944:
        return "_SetItmMark";
    case 0xA945:
        return "_CheckItem";
    case 0xA946:
        return "_GetItem";
    case 0xA947:
        return "_SetItem";
    case 0xA948:
        return "_CalcMenuSize";
    case 0xA949:
        return "_GetMHandle";
    case 0xA94A:
        return "_SetMFlash";
    case 0xA94B:
        return "_PlotIcon";
    case 0xA94C:
        return "_FlashMenuBar";
    case 0xA94D:
        return "_AddResMenu";
    case 0xA94E:
        return "_PinRect";
    case 0xA94F:
        return "_DeltaPoint";
    case 0xA950:
        return "_CountMItems";
    case 0xA951:
        return "_InsertResMenu";
    case 0xA952:
        return "_DelMenuItem";
    case 0xA953:
        return "_UpdtControl";
    case 0xA954:
        return "_NewControl";
    case 0xA955:
        return "_DisposControl";
    case 0xA956:
        return "_KillControls";
    case 0xA957:
        return "_ShowControl";
    case 0xA958:
        return "_HideControl";
    case 0xA959:
        return "_MoveControl";
    case 0xA95A:
        return "_GetCRefCon";
    case 0xA95B:
        return "_SetCRefCon";
    case 0xA95C:
        return "_SizeControl";
    case 0xA95D:
        return "_HiliteControl";
    case 0xA95E:
        return "_GetCTitle";
    case 0xA95F:
        return "_SetCTitle";
    case 0xA960:
        return "_GetCtlValue";
    case 0xA961:
        return "_GetMinCtl";
    case 0xA962:
        return "_GetMaxCtl";
    case 0xA963:
        return "_SetCtlValue";
    case 0xA964:
        return "_SetMinCtl";
    case 0xA965:
        return "_SetMaxCtl";
    case 0xA966:
        return "_TestControl";
    case 0xA967:
        return "_DragControl";
    case 0xA968:
        return "_TrackControl";
    case 0xA969:
        return "_DrawControls";
    case 0xA96A:
        return "_GetCtlAction";
    case 0xA96B:
        return "_SetCtlAction";
    case 0xA96C:
        return "_FindControl";
    case 0xA96D:
        return "_Draw1Control";
    case 0xA96E:
        return "_Dequeue";
    case 0xA96F:
        return "_Enqueue";
    case 0xA970:
        return "_GetNextEvent";
    case 0xA971:
        return "_EventAvail";
    case 0xA972:
        return "_GetMouse";
    case 0xA973:
        return "_StillDown";
    case 0xA974:
        return "_Button";
    case 0xA975:
        return "_TickCount";
    case 0xA976:
        return "_GetKeys";
    case 0xA977:
        return "_WaitMouseUp";
    case 0xA978:
        return "_UpdtDialog";
    case 0xA979:
        return "_CouldDialog";
    case 0xA97A:
        return "_FreeDialog";
    case 0xA97B:
        return "_InitDialogs";
    case 0xA97C:
        return "_GetNewDialog";
    case 0xA97D:
        return "_NewDialog";
    case 0xA97E:
        return "_SelIText";
    case 0xA97F:
        return "_IsDialogEvent";
    case 0xA980:
        return "_DialogSelect";
    case 0xA981:
        return "_DrawDialog";
    case 0xA982:
        return "_CloseDialog";
    case 0xA983:
        return "_DisposDialog";
    case 0xA984:
        return "_FindDItem";
    case 0xA985:
        return "_Alert";
    case 0xA986:
        return "_StopAlert";
    case 0xA987:
        return "_NoteAlert";
    case 0xA988:
        return "_CautionAlert";
    case 0xA989:
        return "_CouldAlert";
    case 0xA98A:
        return "_FreeAlert";
    case 0xA98B:
        return "_ParamText";
    case 0xA98C:
        return "_ErrorSound";
    case 0xA98D:
        return "_GetDItem";
    case 0xA98E:
        return "_SetDItem";
    case 0xA98F:
        return "_SetIText";
    case 0xA990:
        return "_GetIText";
    case 0xA991:
        return "_ModalDialog";
    case 0xA992:
        return "_DetachResource";
    case 0xA993:
        return "_SetResPurge";
    case 0xA994:
        return "_CurResFile";
    case 0xA995:
        return "_InitResources";
    case 0xA996:
        return "_RsrcZoneInit";
    case 0xA997:
        return "_OpenResFile";
    case 0xA998:
        return "_UseResFile";
    case 0xA999:
        return "_UpdateResFile";
    case 0xA99A:
        return "_CloseResFile";
    case 0xA99B:
        return "_SetResLoad";
    case 0xA99C:
        return "_CountResources";
    case 0xA99D:
        return "_GetIndResource";
    case 0xA99E:
        return "_CountTypes";
    case 0xA99F:
        return "_GetIndType";
    case 0xA9A0:
        return "_GetResource";
    case 0xA9A1:
        return "_GetNamedResource";
    case 0xA9A2:
        return "_LoadResource";
    case 0xA9A3:
        return "_ReleaseResource";
    case 0xA9A4:
        return "_HomeResFile";
    case 0xA9A5:
        return "_SizeRsrc";
    case 0xA9A6:
        return "_GetResAttrs";
    case 0xA9A7:
        return "_SetResAttrs";
    case 0xA9A8:
        return "_GetResInfo";
    case 0xA9A9:
        return "_SetResInfo";
    case 0xA9AA:
        return "_ChangedResource";
    case 0xA9AB:
        return "_AddResource";
    case 0xA9AC:
        return "_AddReference";
    case 0xA9AD:
        return "_RmveResource";
    case 0xA9AE:
        return "_RmveReference";
    case 0xA9AF:
        return "_ResError";
    case 0xA9B0:
        return "_FrameRoundRect/WriteResource";
    case 0xA9B1:
        return "_CreateResFile";
    case 0xA9B2:
        return "_SystemEvent";
    case 0xA9B3:
        return "_SystemClick";
    case 0xA9B4:
        return "_SystemTask";
    case 0xA9B5:
        return "_SystemMenu";
    case 0xA9B6:
        return "_OpenDeskAcc";
    case 0xA9B7:
        return "_CloseDeskAcc";
    case 0xA9B8:
        return "_GetPattern";
    case 0xA9B9:
        return "_GetCursor";
    case 0xA9BA:
        return "_GetString";
    case 0xA9BB:
        return "GetIcon";
    case 0xA9BC:
        return "_GetPicture";
    case 0xA9BD:
        return "_GetNewWindow";
    case 0xA9BE:
        return "_GetNewControl";
    case 0xA9BF:
        return "_GetRMenu";
    case 0xA9C0:
        return "_GetNewMBar";
    case 0xA9C1:
        return "_UniqueID";
    case 0xA9C2:
        return "_SysEdit";
    case 0xA9C3:
        return "_KeyTrans";
    case 0xA9C4:
        return "_OpenRFPerm";
    case 0xA9C5:
        return "_RsrcMapEntry";
    case 0xA9C6:
        return "_Secs2Date";
    case 0xA9C7:
        return "_Date2Sec";
    case 0xA9C8:
        return "_SysBeep";
    case 0xA9C9:
        return "_SysError";
    case 0xA9CB:
        return "_TEGetText";
    case 0xA9CC:
        return "_TEInit";
    case 0xA9CD:
        return "_TEDispose";
    case 0xA9CE:
        return "_TextBox";
    case 0xA9CF:
        return "_TESetText";
    case 0xA9D0:
        return "_TECalText";
    case 0xA9D1:
        return "_TESetSelect";
    case 0xA9D2:
        return "_TENew";
    case 0xA9D3:
        return "_TEUpdate";
    case 0xA9D4:
        return "_TEClick";
    case 0xA9D5:
        return "_TECopy";
    case 0xA9D6:
        return "_TECut";
    case 0xA9D7:
        return "_TEDelete";
    case 0xA9D8:
        return "_TEActivate";
    case 0xA9D9:
        return "_TEDeactivate";
    case 0xA9DA:
        return "_TEIdle";
    case 0xA9DB:
        return "_TEPaste";
    case 0xA9DC:
        return "_TEKey";
    case 0xA9DD:
        return "_TEScroll";
    case 0xA9DE:
        return "_TEInsert";
    case 0xA9DF:
        return "_TESetJust";
    case 0xA9E0:
        return "_Munger";
    case 0xA9E1:
        return "_HandToHand";
    case 0xA9E2:
        return "_PtrToXHand";
    case 0xA9E3:
        return "_PtrToHand";
    case 0xA9E4:
        return "_HandAndHand";
    case 0xA9E5:
        return "_InitPack";
    case 0xA9E6:
        return "_InitAllPacks";
    case 0xA9E7:
        return "_Pack0";
    case 0xA9E8:
        return "_Pack1";
    case 0xA9E9:
        return "_Pack2";
    case 0xA9EA:
        return "_Pack3";
    case 0xA9EB:
        return "_FP68K/Pack4";
    case 0xA9EC:
        return "_Elems68K/Pack5";
    case 0xA9ED:
        return "_Pack6";
    case 0xA9EE:
        return "_Pack7";
    case 0xA9EF:
        return "_PtrAndHand";
    case 0xA9F0:
        return "_LoadSeg";
    case 0xA9F1:
        return "_UnloadSeg";
    case 0xA9F2:
        return "_Launch";
    case 0xA9F3:
        return "_Chain";
    case 0xA9F4:
        return "_ExitToShell";
    case 0xA9F5:
        return "_GetAppParms";
    case 0xA9F6:
        return "_GetResFileAttrs";
    case 0xA9F7:
        return "_SetResFileAttrs";
    case 0xA9F9:
        return "_InfoScrap";
    case 0xA9FA:
        return "_UnlodeScrap";
    case 0xA9FB:
        return "_LodeScrap";
    case 0xA9FC:
        return "_ZeroScrap";
    case 0xA9FD:
        return "_GetScrap";
    case 0xA9FE:
        return "_PutScrap";
    case 0xAA00:
        return "_OpenCport";
    case 0xAA01:
        return "_InitCport";
    case 0xAA03:
        return "_NewPixMap";
    case 0xAA04:
        return "_DisposPixMap";
    case 0xAA05:
        return "_CopyPixMap";
    case 0xAA06:
        return "_SetCPortPix";
    case 0xAA07:
        return "_NewPixPat";
    case 0xAA08:
        return "_DisposPixPat";
    case 0xAA09:
        return "_CopyPixPat";
    case 0xAA0A:
        return "_PenPixPat";
    case 0xAA0B:
        return "_BackPixPat";
    case 0xAA0C:
        return "_GetPixPat";
    case 0xAA0D:
        return "_MakeRGBPat";
    case 0xAA0E:
        return "_FillCRect";
    case 0xAA0F:
        return "_FillCOval";
    case 0xAA10:
        return "_FillCRoundRect";
    case 0xAA11:
        return "_FillCArc";
    case 0xAA12:
        return "_FillCRgn";
    case 0xAA13:
        return "_FillCPoly";
    case 0xAA14:
        return "_RGBForeColor";
    case 0xAA15:
        return "_RGBBackColor";
    case 0xAA16:
        return "_SetCPixel";
    case 0xAA17:
        return "_GetCPixel";
    case 0xAA18:
        return "_GetCTable";
    case 0xAA19:
        return "_GetForeColor";
    case 0xAA1A:
        return "_GetBackColor";
    case 0xAA1B:
        return "_GetCCursor";
    case 0xAA1C:
        return "_SetCCursor";
    case 0xAA1D:
        return "_AllocCursor";
    case 0xAA1E:
        return "_GetCIcon";
    case 0xAA1F:
        return "_PlotCIcon";
    case 0xAA21:
        return "_OpColor";
    case 0xAA22:
        return "_HiliteColor";
    case 0xAA23:
        return "_CharExtra";
    case 0xAA24:
        return "_DisposCTable";
    case 0xAA25:
        return "_DisposCIcon";
    case 0xAA26:
        return "_DisposCCursor";
    case 0xAA27:
        return "_GetMaxDevice";
    case 0xAA29:
        return "_GetDeviceList";
    case 0xAA2A:
        return "_GetMainDevice";
    case 0xAA2B:
        return "_GetNextDevice";
    case 0xAA2C:
        return "_TestDeviceAttribute";
    case 0xAA2D:
        return "_SetDeviceAttribute";
    case 0xAA2E:
        return "_InitGDevice";
    case 0xAA2F:
        return "_NewGDevice";
    case 0xAA30:
        return "_DisposGDevice";
    case 0xAA31:
        return "_SetGDevice";
    case 0xAA32:
        return "_GetGDevice";
    case 0xAA33:
        return "_Color2Index";
    case 0xAA34:
        return "_Index2Color";
    case 0xAA35:
        return "_InvertColor";
    case 0xAA36:
        return "_RealColor";
    case 0xAA37:
        return "_GetSubTable";
    case 0xAA39:
        return "_MakeITable";
    case 0xAA3A:
        return "_AddSearch";
    case 0xAA3B:
        return "_AddComp";
    case 0xAA3C:
        return "_SetClientID";
    case 0xAA3D:
        return "_ProtectEntry";
    case 0xAA3E:
        return "_ReserveEntry";
    case 0xAA3F:
        return "_SetEntries";
    case 0xAA40:
        return "_QDError";
    case 0xAA41:
        return "_SetWinColor";
    case 0xAA42:
        return "_GetAuxWin";
    case 0xAA43:
        return "_SetCtlColor";
    case 0xAA44:
        return "_GetAuxCtl";
    case 0xAA45:
        return "_NewCWindow";
    case 0xAA46:
        return "_GetNewCWindow";
    case 0xAA47:
        return "_SetDeskCPat";
    case 0xAA48:
        return "_GetCWMgrPort";
    case 0xAA49:
        return "_SaveEntries";
    case 0xAA4A:
        return "_RestoreEntries";
    case 0xAA4B:
        return "_NewCDialog";
    case 0xAA4C:
        return "_DelSearch";
    case 0xAA4D:
        return "_DelComp";
    case 0xAA4F:
        return "_CalcCMask";
    case 0xAA50:
        return "_SeedCFill";
    case 0xAA60:
        return "_DelMCEntries";
    case 0xAA61:
        return "_GetMCInfo";
    case 0xAA62:
        return "_SetMCInfo";
    case 0xAA63:
        return "_DispMCEntries";
    case 0xAA64:
        return "_GetMCEntry";
    case 0xAA65:
        return "_SetMCEntries";
    case 0xAA66:
        return "_MenuChoice";
    default:
        return "Unknown_A-Line-Trap";
    }
}

/**************************************************************************************\
*                                                                                      *
*       Lisa Office System / Workshop Error Codes.                                     *
*                                                                                      *
*       Source: lisafaq                                                                *
\**************************************************************************************/

#endif

char *los_error_code(signed long loserror)
{

    switch (loserror)
    {
    case -6081:
        return "End of exec file input";
    case -6004:
        return "Attempt to reset text file with typed-file type";
    case -6003:
        return "Attempt to reset nontext file with text type";
    case -1885:
        return "ProFile not present during driver initialization";
    case -1882:
        return "ProFile not present during driver initialization";
    case -1176:
        return "Data in the object have been altered by Scavenger";
    case -1175:
        return "File or volume was scavenged";
    case -1174:
        return "File was left open or volume was left mounted, and system crashed";
    case -1173:
        return "File was last closed by the OS";
    case -1146:
        return "Only a portion of the space requested was allocated";
    case -1063:
        return "Attempt to mount boot volume from another Lisa or not most recent boot volume";
    case -1060:
        return "Attempt to mount a foreign boot disk following a temporary unmount";
    case -1059:
        return "The bad block directory of the diskette is almost full or difficult to read";
    case -876:
        return "(from CLOSE_OBJECT) The file just closed. May be damaged or contain corrupted data because an I/O error occurred when writing a part of the file that was buffered by the file system";
    case -696:
        return "Printer out of paper during initialization";
    case -660:
        return "Cable disconnected during ProFile initialization";
    case -626:
        return "Scavenger indicated data are questionable, but may be OK";
    case -622:
        return "Parameter memory and the disk copy were both invalid";
    case -621:
        return "Parameter memory was invalid but the disk copy was valid";
    case -620:
        return "Parameter memory was valid but the disk copy was invalid";
    case -413:
        return "Event channel was scavenged";
    case -412:
        return "Event channel was left open and system crashed";
    case -321:
        return "Data segment open when the system crashed. Data possibly invalid.";
    case -320:
        return "Could not determine size of data segment";
    case -150:
        return "Process was created, but a library used by program has been scavenged and altered";
    case -149:
        return "Process was created, but the specified program file has been scavenged and altered";
    case -125:
        return "Specified process is already terminating";
    case -120:
        return "Specified process is already active";
    case -115:
        return "Specified process is already suspended";
    case 100:
        return "Specified process does not exist";
    case 101:
        return "Specified process is a system process";
    case 110:
        return "Invalid priority specified (must be l..225)";
    case 130:
        return "Could not open program file";
    case 131:
        return "File System error while trying to read program file";
    case 132:
        return "Invalid program file (incorrect format)";
    case 133:
        return "Could not get a stack segment for new process";
    case 134:
        return "Could not get a syslocal segment for new process";
    case 135:
        return "Could not get sysglobal space for new process";
    case 136:
        return "Could not set up communication channel for new process";
    case 138:
        return "Error accessing program file while loading";
    case 141:
        return "Error accessing a library file while loading program";
    case 142:
        return "Cannot run protected file on this machine";
    case 143:
        return "Program uses an intrinsic unit not found in the Intrinsic Library";
    case 144:
        return "Program uses an intrinsic unit whose name/type does not agree with";
    case 145:
        return "Program uses a shared segment not found in the Intrinsic Library";
    case 146:
        return "Program uses a shared segment whose name does not agree with the Intrinsic Library";
    case 147:
        return "No space in syslocal for program file descriptor during process creation";
    case 148:
        return "No space in the shared IU data segment for the program's shared IU globals";
    case 190:
        return "No space in syslocal for program tile description during List_LibFiles operation";
    case 191:
        return "Could not open program file";
    case 192:
        return "Error trying to read program file";
    case 193:
        return "Cannot read protected program file";
    case 194:
        return "Invalid program file (incorrect format)";
    case 195:
        return "Program uses a shared segment not found in the Intrinsic Library";
    case 196:
        return "Program uses a shared segment whose name does not agree with the Intrinsic Library";
    case 198:
        return "Disk I/O error trying to read the intrinsic unit directory";
    case 199:
        return "Specified library file number does not exist in the Intrinsic Library";
    case 201:
        return "No such exception name declared";
    case 202:
        return "No space left in the system data area for Declare_Excep_Hdl or Signal_Excep";
    case 203:
        return "Null name specified as exception name";
    case 302:
        return "Invalid LDSN";
    case 303:
        return "No data segment hound to the LDSN";
    case 304:
        return "Data segment already bound to the LDSN";
    case 306:
        return "Data segment too large";
    case 307:
        return "Input data segment path name is invalid";
    case 308:
        return "Data segment already exists";
    case 309:
        return "Insufficient disk space for data segment";
    case 310:
        return "An invalid size has been specified";
    case 311:
        return "Insufficient system resources";
    case 312:
        return "Unexpected File System error";
    case 313:
        return "Data segment not found";
    case 314:
        return "Invalid address passed to Info_Address";
    case 315:
        return "Insufficient memory for operation";
    case 317:
        return "Disk error while trying to swap in data segment";
    case 401:
        return "Invalid event channel name passed to Make_Event_Chn";
    case 402:
        return "No space left in system global data area for Open_Event_Chn";
    case 403:
        return "No space left in system local data area for Open_Event_Chn";
    case 404:
        return "Non-block-structured device specified in pathname";
    case 405:
        return "Catalog is full in Make_Event_Chn or Open Event_Chn";
    case 406:
        return "No such event channel exists in Kill_Event_Chn";
    case 410:
        return "Attempt to open a local event channel to send";
    case 411:
        return "Attempt to open event channel to receive when event channel has a receiver";
    case 413:
        return "Unexpected File System error in Open_Event_Chn";
    case 416:
        return "Cannot get enough disk space for event channel in Open_Event_Chn";
    case 417:
        return "Unexpected File System error in Close_Event_Chn";
    case 420:
        return "Attempt to wait on a channel that the calling process did not open";
    case 421:
        return "Wait_Event_Chn returns empty because sender process could not complete";
    case 422:
        return "Attempt to call Wait_Event_Chn on an empty event-call channel";
    case 423:
        return "Cannot find corresponding event channel after being blocked";
    case 424:
        return "Amount of data returned while reading from event channel not of expected size";
    case 425:
        return "Event channel empty after being unblocked, Wait_Event_Chn";
    case 426:
        return "Bad request pointer error returned in Wait_Event_Chn";
    case 427:
        return "Wait List has illegal length specified";
    case 428:
        return "Receiver unblocked because last sender closed";
    case 429:
        return "Unexpected File System error in Wait_Event_Chn";
    case 430:
        return "Attempt to send to a channel which the calling process does not have open";
    case 431:
        return "Amount of data transferred while writing to event channel not of expected size";
    case 432:
        return "Sender unblocked because receiver closed in Send_Event_Chn";
    case 433:
        return "Unexpected File System error in Send_Event_Chn";
    case 440:
        return "unexpected File System error in Make_Event_Chn";
    case 441:
        return "Event channel already exists in Make_Event_Chn";
    case 445:
        return "Unexpected File System error in Kill_Event_Chn";
    case 450:
        return "Unexpected File System error in Flush Event_Chn";
    case 530:
        return "Size of stack expansion request exceeds limit specified for program";
    case 531:
        return "Cannot perform explicit stack expansion due to lack of memory";
    case 532:
        return "Insufficient disk space for explicit stack expansion";
    case 600:
        return "Attempt to perform I/O operation on non I/O request";
    case 602:
        return "No more alarms available during driver initialization";
    case 605:
        return "Call to nonconfigured device driver";
    case 606:
        return "Cannot find sector on floppy diskette (disk unformatted)";
    case 608:
        return "Illegal length or disk address for transfer";
    case 609:
        return "Call to nonconfigured device driver";
    case 610:
        return "No more room in sysglobal for I/O request";
    case 613:
        return "Unpermitted direct access to spare track with sparing enabled on floppy drive";
    case 614:
        return "No disk present in drive";
    case 615:
        return "Wrong call version to floppy drive";
    case 616:
        return "Unpermitted floppy drive function";
    case 617:
        return "Checksum error on floppy diskette";
    case 618:
        return "Cannot format, or write protected, or error unclamping floppy diskette";
    case 619:
        return "No more room in sysglobal for I/O request";
    case 623:
        return "Illegal device control parameters to floppy drive";
    case 625:
        return "Scavenger indicated data are bad";
    case 630:
        return "The time passed to Delay_Time, Convert_Time, or Send_Event_Chn has invalid year";
    case 631:
        return "Illegal timeout request parameter";
    case 632:
        return "No memory available to initialize clock";
    case 634:
        return "Illegal timed event id of [DEL: 1 :DEL]";
    case 635:
        return "Process got unblocked prematurely due to process termination";
    case 636:
        return "Timer request did not complete successfully";
    case 638:
        return "Time passed to Delay_Time or Send_Event_Chn more than 23 days from current time";
    case 639:
        return "Illegal date passed to Set_Time, or illegal date from system clock in Get Time";
    case 640:
        return "RS 232 driver called with wrong version number";
    case 641:
        return "RS-232 read or write initiated with illegal parameter";
    case 642:
        return "Unimplemented or unsupported RS-232 driver function";
    case 646:
        return "No memory available to initialize RS-232";
    case 647:
        return "Unexpected RS-232 timer interrupt";
    case 648:
        return "Unpermitted RS-232 initialization, or disconnect detected";
    case 649:
        return "Illegal device control parameters to RS-232";
    case 652:
        return "N-port driver not initialized prior to ProFile";
    case 653:
        return "No room in sysglobal to initialize ProFile";
    case 654:
        return "Hard error status returned from drive";
    case 655:
        return "Wrong call version to ProFile";
    case 656:
        return "Unpermitted ProFile function";
    case 657:
        return "Illegal device control parameter to ProFile";
    case 658:
        return "Premature end of file when reading from driver";
    case 659:
        return "Corrupt File System header chain found in driver";
    case 660:
        return "Cable disconnected";
    case 662:
        return "Parity error while sending command or writing data to ProFile";
    case 663:
        return "Checksum error or CRC error or parity error in data read";
    case 666:
        return "Timeout";
    case 670:
        return "Bad command response from drive";
    case 671:
        return "Illegal length specified (must = 1 on input)";
    case 672:
        return "Unimplemented console driver function";
    case 673:
        return "No memory available to initialize console";
    case 674:
        return "Console driver called with wrong version number";
    case 675:
        return "Illegal device control";
    case 680:
        return "Wrong call version to serial driver";
    case 682:
        return "Unpermitted serial driver function";
    case 683:
        return "No room in sysglobal to initialize serial driver";
    case 685:
        return "Eject not allowed for this device";
    case 686:
        return "No room in sysglobal to initialize n-port card driver";
    case 687:
        return "Unpermitted n-port card driver function";
    case 688:
        return "Wrong call version to n-port card driver";
    case 690:
        return "Wrong call version to parallel printer";
    case 691:
        return "Illegal parallel printer parameters";
    case 692:
        return "N-port card not initialized prior to parallel printer";
    case 693:
        return "No room in sysgloball. to initialize parallel printer";
    case 694:
        return "Unimplemented parallel printer function";
    case 695:
        return "Illegal device control parameters (parallel printer)";
    case 696:
        return "Printer out of paper";
    case 698:
        return "Printer offline";
    case 699:
        return "No response from printer";
    case 700:
        return "Mismatch between loader version number and Operating System version number";
    case 701:
        return "OS exhausted its internal space during startup";
    case 702:
        return "Cannot make system process";
    case 703:
        return "Cannot kill pseudo-outer process";
    case 704:
        return "Cannot create driver";
    case 706:
        return "Cannot initialize floppy disk driver";
    case 707:
        return "Cannot initialize the File System volume";
    case 708:
        return "Hard disk mount table unreadable";
    case 709:
        return "Cannot map screen data";
    case 710:
        return "Too many slot-based devices";
    case 724:
        return "The boot tracks do not know the right File System version";
    case 725:
        return "Either damaged File System or damaged contents";
    case 726:
        return "Boot device read failed";
    case 727:
        return "The OS will not fit into the available memory";
    case 728:
        return "SYSTEM.OS is missing";
    case 729:
        return "SYSTEM.CONFIG is corrupt";
    case 730:
        return "SYSTEM.OS is corrupt";
    case 731:
        return "SYSTEM.DEBUG or SYSTEM.DEBUG2 is corrupt";
    case 732:
        return "SYSTEM.LLD is corrupt";
    case 733:
        return "Loader range error";
    case 734:
        return "Wrong driver is found. For instance, storing a diskette loader on a ProFile";
    case 735:
        return "SYSTEM.LLD is missing";
    case 736:
        return "SYSTEM.UNPACK is missing";
    case 737:
        return "Unpack of SYSTEM.OS with SYSTEM.UNPACK failed";
    case 791:
        return "Configurable Driver (CD) code file not executable";
    case 792:
        return "Could not get code space for the CD";
    case 793:
        return "I/O error reading CD code file";
    case 794:
        return "CD code file not found";
    case 795:
        return "CD has more than 1 segment";
    case 796:
        return "Could not get temporary space while loading CD";
    case 801:
        return "I/O Result <> 0 on I/O using the Monitor";
    case 802:
        return "Asynchronous I/O request not completed successfully";
    case 803:
        return "Bad combination of mode parameters";
    case 806:
        return "Page specified is out of range";
    case 809:
        return "Invalid arguments (page, address, offset, or count)";
    case 810:
        return "The requested page could not be read in";
    case 816:
        return "Not enough sysglobal space for File System buffers";
    case 819:
        return "Bad device number";
    case 820:
        return "No space in sysglobal for asynchronous request list";
    case 821:
        return "Already initialized I/O for this device";
    case 822:
        return "Bad device number";
    case 825:
        return "Error in parameter values (Allocate)";
    case 826:
        return "No more room to allocate pages on device";
    case 828:
        return "Error in parameter values (Deallocate)";
    case 829:
        return "Partial deallocation only (ran into unallocated region)";
    case 835:
        return "Invalid s-file number";
    case 837:
        return "Unallocated s-file or I/O error";
    case 838:
        return "Map overflow: s-file too large";
    case 839:
        return "Attempt to compact file past PEOF";
    case 840:
        return "The allocation map of this file is truncated";
    case 841:
        return "Unallocated s-file or I/O error";
    case 843:
        return "Requested exact fit, but one could not be provided";
    case 847:
        return "Requested transfer count is <= 0";
    case 848:
        return "End of file encountered";
    case 849:
        return "Invalid page or offset value in parameter list";
    case 852:
        return "Bad unit number";
    case 854:
        return "No free slots in s-list directory (too many s-files)";
    case 855:
        return "No available disk space for file hints";
    case 856:
        return "Device not mounted";
    case 857:
        return "Empty, locked, or invalid s-file";
    case 861:
        return "Relative page is beyond PEOF (bad parameter value)";
    case 864:
        return "No sysglobal space for volume bitmap";
    case 866:
        return "Wrong FS version or not a valid Lisa FS volume";
    case 867:
        return "Bad unit number";
    case 868:
        return "Bad unit number";
    case 869:
        return "Unit already mounted (mount)/no unit mounted";
    case 870:
        return "No sysglobal space for DCB or MDDF";
    case 871:
        return "Parameter not a valid s-file ID";
    case 872:
        return "No sysglobal space for s-file control block";
    case 873:
        return "Specified file is already open for private access";
    case 874:
        return "Device not mounted";
    case 875:
        return "Invalid s-file ID or s-file control block";
    case 879:
        return "Attempt to position past LEOF";
    case 881:
        return "Attempt to read empty file";
    case 882:
        return "No space on volume for new data page of file";
    case 883:
        return "Attempt to read past LEOF";
    case 884:
        return "Not first auto-allocation, but file was empty";
    case 885:
        return "Could not update filesize hints after a write";
    case 886:
        return "No syslocal space for I/O request list";
    case 887:
        return "Catalog pointer does not indicate a catalog (bad parameter)";
    case 888:
        return "Entry not found in catalog";
    case 890:
        return "Entry that name already exists";
    case 891:
        return "Catalog is full or is damaged";
    case 892:
        return "Illegal name for an entry";
    case 894:
        return "Entry not round, or catalog is damaged";
    case 895:
        return "Invalid entry name";
    case 896:
        return "Safety switch is on-[DEL: cannot kill entry :DEL]";
    case 897:
        return "Invalid bootdev value";
    case 899:
        return "Attempt to allocate a pipe";
    case 900:
        return "Invalid page count or FCB pointer argument";
    case 901:
        return "Could not satisfy allocation request";
    case 921:
        return "Pathname invalid or no such device";
    case 922:
        return "Invalid label size";
    case 926:
        return "Pathname invalid or no such device";
    case 927:
        return "Invalid label size";
    case 941:
        return "Pathname invalid or no such device";
    case 944:
        return "Object is not a file";
    case 945:
        return "File is not in the killed state";
    case 946:
        return "Pathname invalid or no such device";
    case 947:
        return "Not enough space in syslocal for File System retdb";
    case 948:
        return "Entry not found in specified catalog";
    case 949:
        return "Private access not allowed if file already open shared";
    case 950:
        return "Pipe already in use, requested access not possible or dwrite not allowed";
    case 951:
        return "File is already opened in private mode";
    case 952:
        return "Bad refnum";
    case 954:
        return "Bad refnum";
    case 955:
        return "Read access not allowed to specified object";
    case 956:
        return "Attempt to position FMARK past LEOF not allowed";
    case 957:
        return "Negative request count is illegal";
    case 958:
        return "Nonsequential access is not allowed";
    case 959:
        return "System resources exhausted";
    case 960:
        return "Error writing to pipe while an unsatisfied read was pending";
    case 961:
        return "Bad refnum";
    case 962:
        return "No WRITE or APPEND access allowed";
    case 963:
        return "Attempt to position FMARK too far past LEOF";
    case 964:
        return "Append access not allowed in absolute mode";
    case 965:
        return "Append access not allowed in relative mode";
    case 966:
        return "Internal inconsistency of FMARK and LEOF (warning)";
    case 967:
        return "Nonsequential access is not allowed";
    case 968:
        return "Bad refnum";
    case 971:
        return "Pathname invalid or no such device";
    case 972:
        return "Entry not found in specified catalog";
    case 974:
        return "Bad refnum";
    case 977:
        return "Bad refnum";
    case 978:
        return "Page count is nonpositive";
    case 979:
        return "Not a block structured device";
    case 981:
        return "Bad refnum";
    case 982:
        return "No space has been allocated for specified file";
    case 983:
        return "Not a block-structured device";
    case 985:
        return "Bad refnum";
    case 986:
        return "No space has been allocated for specified file";
    case 987:
        return "Not a block-structured device";
    case 988:
        return "Bad refnum";
    case 989:
        return "Caller is not a reader of the pipe";
    case 990:
        return "Not a block-structured device";
    case 994:
        return "Invalid refnum";
    case 995:
        return "Not a block-structured device";
    case 999:
        return "Asynchronous read was unblocked before it was satisfied";
    case 1021:
        return "Pathname invalid or no such entry";
    case 1022:
        return "No such entry found";
    case 1023:
        return "Invalid newname, check for [DEL: ' in string :DEL]";
    case 1024:
        return "New name already exists in catalog";
    case 1031:
        return "Pathname invalid or no such entry";
    case 1032:
        return "Invalid transfer count";
    case 1033:
        return "No such entry found";
    case 1041:
        return "Pathname invalid or no such entry";
    case 1042:
        return "Invalid transfer count";
    case 1043:
        return "No such entry found";
    case 1051:
        return "No device or volume by that name";
    case 1052:
        return "A volume is already mounted on device";
    case 1053:
        return "Attempt to mount temporarily unmounted hoot volume just unmounted from this Lisa";
    case 1054:
        return "The had block directory of the diskette is invalid";
    case 1061:
        return "No device or volume by that name";
    case 1062:
        return "No volume is mounted on device";
    case 1071:
        return "Not a valid or mounted volume for working directory";
    case 1091:
        return "Pathname invalid or no such entry";
    case 1092:
        return "No such entry found";
    case 1101:
        return "Invalid device name";
    case 1121:
        return "Invalid device, not mounted, or catalog is damaged";
    case 1122:
        return "(from RESET_CATALOG) There is no space available in the process' local data area to allocate the catalog scan buffer";
    case 1124:
        return "(from GET_NEXT ENTRY) There is no space available in the process' local data area to allocate the catalog scan buffer";
    case 1128:
        return "Invalid pathname, device, or volume not mounted";
    case 1130:
        return "File is protected; cannot open due to protection violation";
    case 1131:
        return "No device or volume by that name";
    case 1132:
        return "No volume is mounted on that device";
    case 1133:
        return "No more open files in the file list of that device";
    case 1134:
        return "Cannot find space in sysglobal for open file list";
    case 1135:
        return "Cannot find the open file entry to modify";
    case 1136:
        return "Boot volume not mounted";
    case 1137:
        return "Boot volume already unmounted";
    case 1138:
        return "Caller cannot have higher priority than system processes when calling ubd";
    case 1141:
        return "Boot volume was not unmounted when calling rbd";
    case 1142:
        return "Some other volume still mounted on the boot device when calling rbd";
    case 1143:
        return "No sysglobal space for MDDF to do rbd";
    case 1144:
        return "Attempt to remount volume which is not the temporarily unmounted hoot volume";
    case 1145:
        return "No sysglobal space for hit map to do rbd";
    case 1158:
        return "Track-by-track copy buffer is too small";
    case 1159:
        return "Shutdown requested while boot volume was unmounted";
    case 1160:
        return "Destination device too small for track-by-track copy";
    case 1161:
        return "Invalid final shutdown mode";
    case 1162:
        return "Power is already off";
    case 1163:
        return "Illegal command";
    case 1164:
        return "Device is not a diskette device";
    case 1165:
        return "No volume is mounted on the device";
    case 1166:
        return "A valid volume is already mounted on the device";
    case 1167:
        return "Not a block-structured device";
    case 1168:
        return "Device name is invalid";
    case 1169:
        return "Could not access device before initialization using default device parameters";
    case 1170:
        return "Could not mount volume after initialization";
    case 1171:
        return "'-' is not allowed in a volume name";
    case 1172:
        return "No space available to initialize a bitmap for the volume";
    case 1176:
        return "Cannot read from a pipe more than half of its allocated physical size";
    case 1177:
        return "Cannot cancel a read request for a pipe";
    case 1178:
        return "Process waiting for pipe data got unblocked because last pipe writer closed it";
    case 1180:
        return "Cannot write to a pipe more than halt of its allocated physical size";
    case 1181:
        return "No system space left for request block for pipe";
    case 1182:
        return "Writer process to a pipe got unblocked before the request was satisfied";
    case 1183:
        return "Cannot cancel a write request for a pipe";
    case 1184:
        return "Process waiting for pipe space got unblocked because the reader closed the pipe";
    case 1186:
        return "Cannot allocate space to a pipe while it has data wrapped around";
    case 1188:
        return "Cannot compact a pipe while it has data wrapped around";
    case 1190:
        return "Attempt to access a page that is not allocated to the pipe";
    case 1191:
        return "Bad parameter";
    case 1193:
        return "Premature end of file encountered";
    case 1196:
        return "Something is still open on device-[DEL: cannot unmount :DEL]";
    case 1197:
        return "Volume is not formatted or cannot be read";
    case 1198:
        return "Negative request count is illegal";
    case 1199:
        return "Function or procedure is not yet implemented";
    case 1200:
        return "Illegal volume parameter";
    case 1201:
        return "Blank file parameter";
    case 1202:
        return "Error writing destination file";
    case 1203:
        return "Invalid UCSD directory";
    case 1204:
        return "File not found";
    case 1210:
        return "Boot track program not executable";
    case 1211:
        return "Boot track program too big";
    case 1212:
        return "Error reading boot track program";
    case 1213:
        return "Error writing boot track program";
    case 1214:
        return "Boot track program file not found";
    case 1215:
        return "Cannot write boot tracks on that device";
    case 1216:
        return "Could not create/close internal buffer";
    case 1217:
        return "Boot track program has too many code segments";
    case 1218:
        return "Could not find configuration information entry";
    case 1219:
        return "Could not get enough working space";
    case 1220:
        return "Premature SOP in boot track program";
    case 1221:
        return "Position out of range";
    case 1222:
        return "No device at that position";
    case 1225:
        return "Scavenger has detected an internal inconsistency symptomatic of a software bug";
    case 1226:
        return "Invalid device name";
    case 1227:
        return "Device is not block structured";
    case 1228:
        return "Illegal attempt to scavenge the boot volume";
    case 1229:
        return "Cannot read consistently from the volume";
    case 1230:
        return "Cannot write consistently to the volume";
    case 1231:
        return "Cannot allocate space (Heap segment)";
    case 1232:
        return "Cannot allocate space (Map segment)";
    case 1233:
        return "Cannot allocate space (SFDB segment)";
    case 1237:
        return "Error rebuilding the volume root directory";
    case 1240:
        return "Illegal attempt to scavenge a non OS-formatted volume";
    case 1295:
        return "The allocation map of this file is damaged and cannot be read";
    case 1296:
        return "Bad string argument has been passed";
    case 1297:
        return "Entry name for the object is invalid (on the volume)";
    case 1298:
        return "S-list entry for the object is invalid (on the volume)";
    case 1807:
        return "No disk in floppy drive";
    case 1820:
        return "Write-protect error on floppy drive";
    case 1822:
        return "Unable to clamp floppy drive";
    case 1824:
        return "Floppy drive write error";
    case 1882:
        return "Bad response from ProFile";
    case 1885:
        return "ProFile timeout error";
    case 1998:
        return "Invalid parameter address";
    case 1999:
        return "Bad refnum";
    case 6001:
        return "Attempt to access unopened file";
    case 6002:
        return "Attempt to reopen a file which is not closed using an open FIB (file info block)";
    case 6003:
        return "Operation incompatible with access mode";
    case 6004:
        return "Printer offline";
    case 6005:
        return "File record type incompatible with character device (must be byte sized)";
    case 6006:
        return "Bad integer (read)";
    case 6010:
        return "Operation incompatible with file type or access mode";
    case 6081:
        return "Premature end of exec file";
    case 6082:
        return "Invalid exec (temporary) file name";
    case 6083:
        return "Attempt to set prefix with null name";
    case 6090:
        return "Attempt to move console with exec or output file open";
    case 6101:
        return "Bad real (read)";
    case 6151:
        return "Attempt to reinitialize heap already in use";
    case 6152:
        return "Bad argument to NEW (negative size)";
    case 5153:
        return "Insufficient memory for NEW request";
    case 6154:
        return "Attempt to RELEASE outside of heap";
    case 10050:
        return "Request block is not chained to a PCB (Unblk_Req)";
    case 10051:
        return "Bld_Req is called with interrupts off";
    case 10100:
        return "An error was returned from SetUp_Directory or a Data Segment Routine Setup_IUInfo)";
    case 10102:
        return "Error > 0 trying to create shell (Root)";
    case 10103:
        return "Sem_Count > 1 (mit_Sem)";
    case 10104:
        return "Could not open event channel for shell (Root)";
    case 10197:
        return "Automatic stack expansion fault occurred in system code (Check_Stack)";
    case 10198:
        return "Need_Mem set for current process while scheduling is disabled SimpleScheduler)";
    case 10199:
        return "Attempt to block for reason other than I/O while scheduling is disabled (SimpleScheduler)";
    case 10201:
        return "Hardware exception occurred while in system code";
    case 10202:
        return "No space left from Sigl Excep call in Hard_Excep";
    case 10203:
        return "No space left from Sigl Excep call in Nmi_Excep";
    case 10205:
        return "Error from Wait_Event_Chn called in Excep_Prolog";
    case 10207:
        return "No system data space in Excep_Setup";
    case 10208:
        return "No space left from Sigl Excep call in range error";
    case 10212:
        return "Error in Term_Def_Hdl from Enable_Excep";
    case 10213:
        return "Error in Force_Term_Excep, no space in Enq_Ex_Data";
    case 10401:
        return "Error from Close_Event Chn in Sc Cleanup";
    case 10582:
        return "Unable to get space in Freeze_Seq";
    case 10590:
        return "Fatal memory parity error";
    case 10593:
        return "Unable to move memory manager segment during startup";
    case 10594:
        return "Unable to swap in a segment during startup";
    case 10595:
        return "Unable to get space in Extend MMlist";
    case 10596:
        return "Trying to alter size of segment that is not data or stack (Alt_DS Size)";
    case 10597:
        return "Trying to allocate space to an allocated segment (Alloc_Mem)";
    case 10598:
        return "Attempting to allocate a nonfree memory region (Take_Free)";
    case 10599:
        return "Fatal Disk error trying to read system code into memory";
    case 10600:
        return "Error attempting to make timer pipe";
    case 10601:
        return "Error from Kill_Object of an existing timer pipe";
    case 10602:
        return "Error from second Make Pipe to make timer pipe";
    case 10603:
        return "Error from Open to open timer pipe";
    case 10604:
        return "No syslocal space for head of timer list";
    case 10605:
        return "Error during allocate space for timer pipe, or interrupt from nonconfigured device";
    case 10609:
        return "Interrupt from nonconfigured device";
    case 10610:
        return "Error from info about timer pipe";
    case 10611:
        return "Spurious interrupt from floppy drive #2";
    case 10612:
        return "Spurious interrupt from floppy drive #1, or no syslocal space for timer list element";
    case 10613:
        return "Error from Read_Data of timer pipe";
    case 10614:
        return "Actual returned from Read_Data is not the same as requested from timer pipe";
    case 10615:
        return "Error from open of the receiver's event channel";
    case 10616:
        return "Error from Write_Event to the receiver's event channel";
    case 10617:
        return "Error from Close_Event_Chn on the receiver's pipe";
    case 10619:
        return "No sysglobal space for timer request block";
    case 10624:
        return "Attempt to shut down floppy disk controller while drive is still busy";
    case 10637:
        return "Not enough memory to initialize system timeout drives";
    case 10675:
        return "Spurious timeout on console driver";
    case 10699:
        return "Spurious timeout on parallel printer driver";
    case 10700:
        return "Mismatch between loader version number and Operating System version number";
    case 10701:
        return "OS exhausted its internal space during startup";
    case 10702:
        return "Cannot make system process";
    case 10703:
        return "Cannot kill pseudo-outer process";
    case 10704:
        return "Cannot create driver";
    case 10706:
        return "Cannot initialize floppy disk driver";
    case 10707:
        return "Cannot initialize the File System volume";
    case 10708:
        return "Hard disk mount table unreadable";
    case 10709:
        return "Cannot map screen data";
    case 10710:
        return "Too many slot-based devices";
    case 10724:
        return "The boot tracks do not know the right File System version";
    case 10725:
        return "Either damaged File System or damaged contents";
    case 10726:
        return "Boot device read failed";
    case 10727:
        return "The OS will not fit into the available memory";
    case 10728:
        return "SYSTEM.OS is missing";
    case 10729:
        return "SYSTEM.CONFIG is corrupt";
    case 10730:
        return "SYSTEM.OS is corrupt";
    case 10731:
        return "SYSTEM.DEBUG or SYSTEM.DEBUG2 is corrupt";
    case 10732:
        return "SYSTEM.LLD is corrupt";
    case 10733:
        return "Loader range error";
    case 10734:
        return "Wrong driver is found. For instance, storing a diskette loader on a ProFile";
    case 10735:
        return "SYSTEM.LLD is missing";
    case 10736:
        return "SYSTEM.UNPACK is missing";
    case 10737:
        return "Unpack of SYSTEM.OS with SYSTEM.UNPACK failed";
    case 10738:
        return "Checksum failure or too many hardware config changes (not sure of the text for this error.)";
    case 11176:
        return "Found a pending write request for a pipe while in Close_Object when it is called by the last writer of the pipe";
    case 11177:
        return "Found a pending read request for a pipe while in Close Object when it is called by the (only possible) reader of the pipe";
    case 11178:
        return "Found a pending read request for a pipe while in Read_Data from the pipe";
    case 11180:
        return "Found a pending write request for a pipe while in Write_Data to the pipe";
    case 11807:
        return "No disk in floppy drive";
    case 11820:
        return "Write-protect error on floppy drive";
    case 11822:
        return "Unable to clamp floppy drive";
    case 11824:
        return "Floppy drive write error";
    case 11882:
        return "Bad response from ProFile";
    case 11885:
        return "ProFile timeout error";
    case 11901:
        return "Call to Getspace or Relspace with a bad parameter, or free pool is bad";
    default:
        return "(unknown error)";
    }
}
