/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, MMXX Ray A. Arachelian                          *
*                            All Rights Reserved                                       *
*                                                                                      *
*                                                                                      *
*                        wxWidgets User Interface Edition                              *
*                                                                                      *
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
*           Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
\**************************************************************************************/

#include <vars.h>

FILE *alertlogfh = NULL;

extern void save_configs(void);

static int lognum = 0;

extern void tracelog_screenshot(char *filename);

#ifdef DEBUG

extern void dumpallmmu(void);

char *debug_screenshot(void)
{
  static char filename[1024];
  snprintf(filename, 1024, "lisaem-output.%03d-%08x.%016llx.png", lognum, pc24, cpu68k_clocks);
  tracelog_screenshot(filename);
  DEBUG_LOG(0, "%s tracelog screenshot created", filename);
  snprintf(filename, 1024, "lisaem-output.%03d-%08x.%016llx.ram", lognum, pc24, cpu68k_clocks);
  return filename;
}

void debug_on(char *reason)
{
  char filename[1024];
  FILE *loglist = NULL;

  // if (!lisaram) {ALERT_LOG(0,"Cannot open log, no LisaRAM yet"); sleep(10); return; }

  if (buglog == NULL)
    buglog = stderr;
  if (buglog != stderr)
  {
    ALERT_LOG(0, "Not enabling tracelog - already enabled");
    return;
  }

#ifdef __WXMSW__
  return;
#endif

  debug_log_enabled = 1;
  lognum++;

  ALERT_LOG(0, "turning on debug log because %s", reason);

  // snprintf(filename,1024,"bzip2 -1 > ./lisaem-output.%03d-%08x.%016llx.txt.bz2",lognum,pc24,cpu68k_clocks);
  // snprintf(filename,1024,"gzip -1 > ./lisaem-output.%03d-%08x.%016llx.txt.gz",lognum,pc24,cpu68k_clocks);
  snprintf(filename, 1024, "lisaem-output.%03d-%08x.%016llx.txt", lognum, pc24, cpu68k_clocks);

  ALERT_LOG(0, filename);

  // buglog=popen(filename,"w");
  buglog = fopen(filename, "w");

  if (!buglog)
  {
    ALERT_LOG(0, ".");
    fprintf(stderr, "ERROR: Could not create buglog:%s:", filename);
    perror("");
    fprintf(stderr, "\n");
    ALERT_LOG(0, "could not create buglog");
    buglog = stderr;
  }
  else
  {
    // ALERT_LOG(0," ./lisaem-output.%03d-%08x-%016llx.txt.bz2 on %s",lognum,pc24,cpu68k_clocks,reason);
    ALERT_LOG(0, " lisaem-output.%03d-%08x-%016llx.txt on %s", lognum, pc24, cpu68k_clocks, reason);
    ALERT_LOG(0, ".");

    loglist = fopen("lisaem-output.logfiles.txt", "at");

    if (loglist)
      fprintf(loglist, "%s\n lisaem-output.%03d-%08x.%016llx.txt created\n",
              reason, lognum, pc24, cpu68k_clocks);
    ALERT_LOG(0, ".");

    if (loglist)
      fflush(loglist);
    if (loglist)
      fclose(loglist);
  }

  ALERT_LOG(0, "Turned log on.");
  save_configs();
  dumpallmmu();
}

void debug_off(void)
{
#ifdef __WXMSW__
  return;
#endif
  ALERT_LOG(0, "dumping mmu because shutting down log");
  if (buglog)
  {
    dumpallmmu();
    ALERT_LOG(0, "Shutting debug log pipe off");
    fflush(buglog);
    fclose(buglog);
  }
  buglog = NULL;
  debug_log_enabled = 0;
}
#endif

void mc68k_reset(void)
{

  if (buglog)
    fprintf(buglog, "CPU RESET OPCODE Called");
  floppy_6504_wait = 0;

  ALERT_LOG(0, "Maybe - Add more I/O RESET code here");
}

// for RC parsing
char *rstrip(char *s)
{
  char space = ' ';
  int32 i;
  i = strlen(s) - 1;
  if (i <= 0)
    return s;
  while (s[i] <= space && i > 0)
  {
    s[i] = 0;
    i--;
  }
  return s;
}

/* strip white space at the left (start) of a string.
   First we check that there is some, if there isn't,
   we bail out fast, if there is, we start copying the
   string over itself. For safety, we also check the
   size of the string so we prevent buffer overflows,
   incase we accidentally don't have a string terminator. */

// for RC parsing
char *lstrip(char *s, uint32 n) // n=size_t in C
{
  char space = ' ';
  uint32 i = 0, j = 0;

  if (*s > space)
    return s; // nothing to do, no ws leftmost

  while (s[i] <= space && i < n)
    i++; // find first nonspace.
  while (s[i] && i < n)
    s[j++] = s[i++]; // move the string over

  s[j++] = 0; // make sure that we terminate string properly.
  return s;
}

/*-----------------12/14/2006 1:39PM----------------
 * no longer needed as wxWidgets has the INI processing.

// for RC parsing
int isalphanumeric(char *s)
{
    while (*s)
    { if (isalnum((int)(*s))) s++; else return 0; }
    return 1;
}

// for RC parsing
char *stringtoupper(char *s)
{
    while ( (*s=toupper(*s)) ) s++; return s;
}
**********************************************************/

extern int mouse_keys_enabled, mouse_keys_x, mouse_keys_y;

// Fn to get a hash of the screen.  Unlike most hashes, we want a hash that compares similarity rather than difference.
// most hashes want to change about half the bits if there's one bit of change.  This hash wants to change as few bits as
// possible if there's a small difference, so we can compare closeness.
//
// 1st 12 bytes indicate closeness

uint8 *get_screen_hash(void)
{
  uint8 xhash[90];
  static uint8 hashtable[16];
  int i, x, y; // lisa_vid_size_y, lisa_vid_size_xbytes;
  uint8 q;

  memset(xhash, 0, 90);
  memset(hashtable, 0, 16);

  // if (has_lisa_xl_screenmod) {lisa_vid_size_y=431; lisa_vid_size_xbytes=76;} else {lisa_vid_size_y=364; lisa_vid_size_xbytes=90;}

  for (y = 0; y < lisa_vid_size_y; y++)
    for (x = 0; x < lisa_vid_size_xbytes; x++)
    {
      q = lisaram[videolatchaddress + x + (y * lisa_vid_size_xbytes)];
      if (q != 0xaa && q != 0x55) // ignore fuzzy 50% gray background pattern
      {
        xhash[x] = brol((uint8)xhash[x], (uint8)1) ^ q;
        if ((q & 1) ^ (x & 1))
        {
          xhash[x] = xhash[x] ^ 0xff;
        }
      }
    }

  for (i = 0, x = 0; x < lisa_vid_size_xbytes; x += 8, i++)
    hashtable[i] = (eparity[xhash[x]] << 7) | (eparity[xhash[x + 1]] << 6) | (eparity[xhash[x + 2]] << 5) | (eparity[xhash[x + 3]] << 4) | (eparity[xhash[x + 4]] << 3) | (eparity[xhash[x + 5]] << 2) | (eparity[xhash[x + 6]] << 1) | (eparity[xhash[x + 7]]);

  return hashtable;
}

uint8 cmp_screen_hash(uint8 *hashtable1, uint8 *hashtable2)
{
  int count = 0, i;
  /*
  count= (hashtable1[0] ==hashtable2[0])      +
         (hashtable1[1] ==hashtable2[1])      +
         (hashtable1[2] ==hashtable2[2])      +
         (hashtable1[3] ==hashtable2[3])      +
         (hashtable1[4] ==hashtable2[4])      +
         (hashtable1[5] ==hashtable2[5])      +
         (hashtable1[6] ==hashtable2[6])      +
         (hashtable1[7] ==hashtable2[7])      +
         (hashtable1[8] ==hashtable2[8])      +
         (hashtable1[9] ==hashtable2[9])      +
         (hashtable1[10]==hashtable2[10])    +
         (hashtable1[11]==hashtable2[11]);
  */

  for (i = 0; i < 12; i++)
  {
    if (hashtable1[i] == hashtable2[i])
      count += 2;
    else
    {
      if ((hashtable1[i] & 0x0f) == (hashtable2[i] & 0x0f))
        count++;
      if ((hashtable1[i] & 0xf0) == (hashtable2[i] & 0xff))
        count++;
    }
  }

  return count;
}

// (jamesdenton) Defined in vars.h but re-defined here
#define LISA_ROM_RUNNING 0
#define LISA_OFFICE_RUNNING 1
#define LISA_TEST_RUNNING 2
#define LISA_MACWORKS_RUNNING 3
#define LISA_MONITOR_RUNNING 4
#define LISA_XENIX_RUNNING 5
#define LISA_UNIPLUS_RUNNING 6
#define LISA_UNIPLUS_SUNIX_RUNNING 7
#define UNKNOWN_OS_RUNNING 100

// remove me
// static int crapcycles=0;
// remove me

extern void apply_xenix_hle_patches(void);
extern void apply_monitor_hle_patches(void);

int check_running_lisa_os(void)
{
  uint32 v1, v2;
  if (!lisaram)
  {
    running_lisa_os = UNKNOWN_OS_RUNNING;
    return running_lisa_os;
  }

  mouse_x_tolerance = 1;
  mouse_y_tolerance = 1;
  mouse_x_halfing_tolerance = 1;
  mouse_y_halfing_tolerance = 1;
  v1 = lisa_ram_safe_getlong((uint8)1, (uint32)0x0064);
  v2 = lisa_ram_safe_getlong((uint8)1, (uint32)0x0068);

  // crapcycles++;
  // if (crapcycles>20) {fprintf(stderr,"v1:%08x v2:%08x @%08x\n",v1,v2,pc24); crapcycles=0;}

  if ((v1 & 0x00ff0000) == 0x00fe0000 && (v2 & 0x00ff0000) == 0x00fe0000) // Lisa ROM
  {
    // if (lisa_os_mouse_x_ptr!=0x00000486) ALERT_LOG(0,"Mouse vector changed from %08x,%08x to 486",lisa_os_mouse_x_ptr,lisa_os_mouse_y_ptr);
    lisa_os_mouse_x_ptr = 0x00000486;
    lisa_os_mouse_y_ptr = 0x00000488;
    running_lisa_os = LISA_ROM_RUNNING;
    DEBUG_LOG(0, "Lisa ROM v1:%08x v2:%08x", v1, v2);
    return running_lisa_os;
  }
  else if ((v1 & 0x00ff0000) == 0x00500000 && (v2 & 0x00ff0000) == 0x00500000)
  {
    // LOS 1.0 or 1.2: v1=0050080e v2=0050098c; test1,2,3: 426effe8, e5402070, 00363d68
    // if (lisa_os_mouse_x_ptr!=0x00cc00f0) ALERT_LOG(0,"Mouse vector changed from %08x,%08x to cc00f0",lisa_os_mouse_x_ptr,lisa_os_mouse_y_ptr);
    lisa_os_mouse_x_ptr = 0x00cc00ea;
    lisa_os_mouse_y_ptr = 0x00cc00ec;
    mouse_x_tolerance = 4;
    mouse_y_tolerance = 4;
    running_lisa_os = LISA_OFFICE_RUNNING;   
    DEBUG_LOG(0, "Lisa Office System version 1.0 or 1.2 : v1:%08x v2:%08x", v1, v2);
    return running_lisa_os;
  }
  else if ((v1 & 0x00ff0000) == 0x00520000 && (v2 & 0x00ff0000) == 0x00520000)
  {
    // LOS 2.0 or 3.0 or 3.1 or Workshop 3: v1=005208a6 v2=00520a50; test1,2,3: 67202054, 0000226e, 001630ae
    // if (lisa_os_mouse_x_ptr!=0x00cc00f0) ALERT_LOG(0,"Mouse vector changed from %08x,%08x to cc00f0",lisa_os_mouse_x_ptr,lisa_os_mouse_y_ptr);
    lisa_os_mouse_x_ptr = 0x00cc00f0; 
    lisa_os_mouse_y_ptr = 0x00cc00f2; 
    mouse_x_tolerance = 4;
    mouse_y_tolerance = 4;
    running_lisa_os = LISA_OFFICE_RUNNING;
    DEBUG_LOG(0, "Lisa Office System versions 2.x or 3.x : v1:%08x v2:%08x", v1, v2);
    return running_lisa_os;
  }
  else if (
    ((v1 & 0x00ff0000) == 0x00ec0000 && (v2 & 0x00fff000) == 0x00ec0000) || // LisaTest - this one might be wrong!
    ((v1 & 0x000ff000) == 0x000ec000 && (v2 & 0x000ff000) == 0x000ec000))   // LisaTest for Peripherals v1=000ec92a v2=000eca7a
  {                                                                                //                 v1:000ec92a                       v2:000eca7a
                                                                                   // if (lisa_os_mouse_x_ptr!=0x00000fec) ALERT_LOG(0,"Mouse vector changed from %08x,%08x to fec",lisa_os_mouse_x_ptr,lisa_os_mouse_y_ptr);
    lisa_os_mouse_x_ptr = 0x00000fec;
    lisa_os_mouse_y_ptr = 0x000000fee;
    running_lisa_os = LISA_TEST_RUNNING;
    DEBUG_LOG(0, "LisaTest v1:%08x v2:%08x", v1, v2);
    return running_lisa_os;
  }
  else if ((v1 & 0x000ff000) == 0x000e3000 && (v2 & 0x000ff000) == 0x000e3000) // Lisa Monitor v11.0 - v1=000e32e6 v2=000e3330, Lisa Monitor v11.1 - v1=000e32e6 v2=000e3330
  {
    // Note: This setting fixes the mouse pointer in Smalltalk-80 when running it in Lisa Monitor v11.0 or v11.1
  
    // Note: There is also Lisa Monitor v11.2, but we could not find floppy disk images
    // of it on the webs, hence it is unclear if it will work with the settings below.
    lisa_os_mouse_x_ptr = 0x000010e6;
    lisa_os_mouse_y_ptr = 0x000010e8;
    running_lisa_os = LISA_MONITOR_RUNNING;
    DEBUG_LOG(0, "Lisa Monitor v11.0 or v11.1 Running: v1=%08x v2=%08x", v1, v2);
    if (monitor_patch)
      apply_monitor_hle_patches();
    return running_lisa_os;
  }
  else if ((v1 & 0x00ffffff) == 0x000e2980 && (v2 & 0x00ffffff) == 0x000e2ad2) // Lisa Monitor v11.8 - v1=000e2980 v2=000e2ad2  
  {
    // Note: This setting fixes the mouse pointer in Smalltalk-80 when running it in Lisa Monitor v11.8
    lisa_os_mouse_x_ptr = 0x000010ea;
    lisa_os_mouse_y_ptr = 0x000010ec;
    running_lisa_os = LISA_MONITOR_RUNNING;
    DEBUG_LOG(0, "Lisa Monitor v11.8 Running: v1=%08x v2=%08x", v1, v2);
    if (monitor_patch)
      apply_monitor_hle_patches();
    return running_lisa_os;
  }
  else if (
    ((v1 & 0x000ff000) == 0x000e2000 && (v2 & 0x000ff000) == 0x000e2000) || // Lisa Monitor v12.3 - v1=000e244e, v2=000e259e 
    ((v1 & 0x000ff000) == 0x000d5000 && (v2 & 0x000fff00) == 0x000e2500) || // Lisa Monitor v12.x
    ((v1 & 0x00fff000) == 0x001c2000 && (v2 & 0x00ffff00) == 0x001c2500) || // this is a clue - address has changed!!!! maybe that's why LOS crashes!
    ((v1 & 0x00fff000) == 0x00142000 && (v2 & 0x00fff000) == 0x00142000) ||
    ((v1 & 0x00fff000) == 0x00162000 && (v2 & 0x00fff000) == 0x00162000))
  {
    // if (lisa_os_mouse_x_ptr!=0x00000fec) ALERT_LOG(0,"Mouse vector changed from %08x,%08x to fec",lisa_os_mouse_x_ptr,lisa_os_mouse_y_ptr);
    lisa_os_mouse_x_ptr = 0x00000fec;
    lisa_os_mouse_y_ptr = 0x000000fee;
    running_lisa_os = LISA_MONITOR_RUNNING;
    DEBUG_LOG(0, "Lisa Monitor v12 Running: v1=%08x v2=%08x", v1, v2);
    if (monitor_patch)
      apply_monitor_hle_patches();
    return running_lisa_os;
  } 
  else if (
    ((v1 & 0x00fff000) == 0x000e4000 && (v2 & 0x00fff000) == 0x000e4000) || // Macworks XL 3.0
    ((v1 & 0x00fff000) == 0x00144000 && (v2 & 0x00fff000) == 0x00144000) || // 2020.08.03 did this move due to extra RAM?
    ((v1 & 0x00fff000) == 0x001b6000 && (v2 & 0x00fff000) == 0x001b6000) || // 2021.03.23 and yet a new address!
    ((v1 & 0x00fff000) == 0x00156000 && (v2 & 0x00fff000) == 0x00156000) || // MacWorks XL 4.5 v1=00156206 v2=00156384; test1,2,3: 50555445, 400022d8, 91fc0000| 23:27:31.1 1242627682
    ((v1 & 0x00fff000) == 0x001c4000 && (v2 & 0x00fff000) == 0x001c4000) ||
    ((v1 & 0x00fff000) == 0x00164000 && (v2 & 0x00fff000) == 0x00164000) ||
    (bootblockchecksum == 0xce0ca734 || bootblockchecksum == 0xce0cbba3) || // MWXL3.0, MWXL4.5 floppy
    (bootblockchecksum == 0xb66c2a5d) ||                                    // MW+II v2.3.0 floppy
    (bootblockchecksum == 0x703fe7ba) ||                                    // MWXL 3.0 profile boot block
    (bootblockchecksum == 0xce0cb94e)                                       // MW1.018 for system 6
      )
  {
    // if (lisa_os_mouse_x_ptr!=0x0000082e) ALERT_LOG(0,"Mouse vector changed from %08x,%08x to 82e",lisa_os_mouse_x_ptr,lisa_os_mouse_y_ptr);
    lisa_os_mouse_x_ptr = 0x0000082e;
    lisa_os_mouse_y_ptr = 0x00000082c;
    mouse_x_tolerance = 4;
    mouse_y_tolerance = 4;
    DEBUG_LOG(0, "MacWorks v1:%08x v2:%08x", v1, v2);
    running_lisa_os = LISA_MACWORKS_RUNNING;
    return running_lisa_os;
  }
  else if ((v1 & 0x00ffffff) == 0x000001c0 && (v2 & 0x00ffffff) == 0x000001e0) // Xenix
  {
    // if (lisa_os_mouse_x_ptr!=0x0000082e) ALERT_LOG(0,"Mouse vector changed from %08x,%08x to 82e",lisa_os_mouse_x_ptr,lisa_os_mouse_y_ptr);
    lisa_os_mouse_x_ptr = 0x0000082e;
    lisa_os_mouse_y_ptr = 0x00000082c;
    running_lisa_os = LISA_XENIX_RUNNING;
    DEBUG_LOG(0, "MicroSoft Xenix Running: v1=%08x v2=%08x", v1, v2);
    if (xenix_patch)
      apply_xenix_hle_patches();
    return running_lisa_os;
  }
  else if ((v1 & 0x00ffffff) == 0x0001c26c && (v2 & 0x00ffffff) == 0x0001c270) // UniPlus -  v1=0001c26c v2=0001c270|
  {
    lisa_os_mouse_x_ptr = 0x0000082e;
    lisa_os_mouse_y_ptr = 0x00000082c; // Unknown mouse handler locations for now, will revisit after getting it working
    running_lisa_os = LISA_UNIPLUS_RUNNING;
    DEBUG_LOG(0, "UniPlus Running: v1=%08x v2=%08x", v1, v2);
    return running_lisa_os;
  }
  else if ((v1 & 0x00ffffff) == 0x0001c4ac && (v2 & 0x00ffffff) == 0x0001c4b0) // UniPlus SUNIX-   v1=0001c4ac v2=0001c4b0
  {
    running_lisa_os = LISA_UNIPLUS_SUNIX_RUNNING;
    DEBUG_LOG(0, "UniPlus sunix v1.1 kernel Running: v1=%08x v2=%08x", v1, v2);
    return running_lisa_os;
  }
  // src/lisa/motherboard/glue.c:check_running_lisa_os:382:Unknown OS Running: v1=0001c4ac v2=0001c4b0| 20:10:38.8 441279005

  abort_opcode = 2;
  uint32 test1 = fetchlong(0x400040);
  abort_opcode = 0;
  abort_opcode = 2;
  uint32 test2 = fetchlong(0x400080);
  abort_opcode = 0;
  abort_opcode = 2;
  uint32 test3 = fetchlong(0x400088);
  abort_opcode = 0;

  running_lisa_os = UNKNOWN_OS_RUNNING;
  ALERT_LOG(0, "Unknown OS Running: v1=%08x v2=%08x; test1,2,3: %08x, %08x, %08x", v1, v2, test1, test2, test3);
  return running_lisa_os;
}

///////////// REPLACE THESE!!!!!!!!!!!!!!! /////////////////////////

extern void extprintregs(FILE *buglog, char *tag);

///////////// REPLACE THESE!!!!!!!!!!!!!!! /////////////////////////

#ifdef DEBUGGER
// untested code!!!!!  DELETE THIS!

void get_dbgr_prams(char *cmd, uint32 *dcx, uint32 *cursor, uint32 *size)
{
  char line[1024];
  char *slash;
  char *l = line;
  uint32 curs;

  strncpy(l, cmd, 1023);
  slash = strchr(l, '/');

  while (*l && *l == ' ')
    l++; // skip spaces
  if (!*l)
    return;

  // context is optional.
  if (slash)
  {
    slash--; // character just before slash
    if (*slash >= '0' && *slash < '5')
      *dcx = (*slash) - '0';
    slash++;
    slash++;
    l = slash; // move pointer past the slash;
  }

  if (!*l)
    return;
  curs = 0;

  while ((*l >= '0' && *l <= '9') || (*l >= 'a' && *l <= 'f'))
  {
    curs <<= 4;
    if (*l >= '0' && *l <= '9')
      curs |= (*l - '0');
    if (*l >= 'a' && *l <= 'f')
      curs |= (*l - 'a' + 10);
    l++;
    *cursor = curs;
    if (!*l)
      return;
  }

  while (*l && *l == ' ')
    l++; // skip spaces
  if (!*l)
    return;

  curs = 0;
  while ((*l >= '0' && *l <= '9') || (*l >= 'a' && *l <= 'f'))
  {
    curs <<= 4;
    if (*l >= '0' && *l <= '9')
      curs |= (*l - '0');
    if (*l >= 'a' && *l <= 'f')
      curs |= (*l - 'a' + 10);
    l++;
    *size = curs;
    if (!*l)
      return;
  }
}

inline static uint8 busfreefetchbyte(uint32 addr)
{
  abort_opcode = 13;
  return lisa_ram_safe_getbyte(cx, addr);
} // supress bus error

void debugger(void)
{
  char cmd[1024];

  uint32 page, size;
  uint32 oldpc = pc24;
  uint32 mcursor = 0;
  uint32 cursor = pc24;
  uint32 oldcx = context;
  uint32 dcx = context, mdcx = context;
  uint i;

  //         .........1.........2.........3.........4.........5.........6.........7
  //         123456789012345678901234567890123456789012345678901234567890123456789012345678
  printf("\n\n");
  printf("-----------------------------------------------------------------------\n");
  printf("  The Lisa Emulator - Debugger V0.8.9-Dev  http://lisa.sunder.net/dbg  \n");
  printf("  -------------------------------------------------------------------  \n");
  printf("  Copyright  (C)   MMIV   by Ray A. Arachelian,   All Rights Reserved  \n");
  printf("  Released  under  the terms of  the  GNU Public License  Version 2.0  \n");
  printf("  -------------------------------------------------------------------  \n");
  printf("  For historical/archival/educational use only - no warranty provied.  \n");
  printf("-----------------------------------------------------------------------\n\n");
  putchar(7);

  fflush(stderr);

  debug_off();
  buglog = stdout;
  debug_log_enabled = 1;

  extprintregs(buglog, "");

  while (1)
  {
    fflush(stdout);
    fflush(stderr);
    fflush(buglog);
    putchar(']');
    putchar(' ');
    fgets(cmd, 1023, stdin);
    abort_opcode = 0;

    switch (tolower(cmd[0]))
    {
    case 'd': // display commands;
      if (cmd[1] == 'v')
      {
        if (cmd[2] == '1')
        {
          fdumpvia1(stdout);
          break;
        }
        if (cmd[2] == '2')
        {
          fdumpvia2(stdout);
          break;
        }
        fdumpvia1(stdout);
        fdumpvia2(stdout);
        break;
      }
      else if (cmd[1] == 'r')
      {
        extprintregs(buglog, "");
        break;
      }
      else if (cmd[1] == 'c')
      {
        my_dump_cops(stdout);
        break;
      }
      else if (cmd[1] == 'z')
      {
        dump_scc();
        break;
      }
      else if (cmd[1] == 'h')
      {
        print_via_profile_state("ProFile", 0, &via[2]);
      }
      else if (cmd[1] == 'i')
      {
        t_iib *piib;
        t_ipc *ipc = NULL; ////// what gets ipc??? this is a buggy fn!
        mmu_trans_t *mt;
        char line[1024];

        get_dbgr_prams(cmd, &dcx, &cursor, &size);
        if (dcx != context)
        {
          context = dcx;
          mmuflush(0x2000);
        }

        for (i = 0; i < size; i++)
        {
          cursor &= 0x00ffffff;
          page = cursor >> 9;
          mt = &mmu_trans[page];
          if (mt->readfn == bad_page)
          {
            fprintf(buglog, "\n\nCurrent opcode lives inside a bad_page @ %08x\n\n", cursor);
            break;
          }

          if (pc24 > 0xfe0000 && pc24 < 0xfe7fff && mt->readfn == sio_mmu && rom_source_file && dtc_rom_fseeks && debug_log_enabled)
          {
            char *rom_label;
            rom_label = get_rom_label(pc24 & 0x00ffff);
            if (rom_label)
              fprintf(buglog, "SRC: Exec ROM label:%s\n", rom_label);
          }

          if (!(piib = cpu68k_iibtable[fetchword(cursor)]))
          {
            printf("Invalid opcode %04x @ %08X\n", ipc->opcode, cursor);
            break;
          }

#if DEBUG
          if (!piib)
            ALERT_LOG(0, "about to pass NULL IIB");
          if (!ipc)
            ALERT_LOG(0, "about to pass NULL IIB");
#endif

          cpu68k_ipc(cursor, piib, ipc);
          if (!abort_opcode)
            diss68k_gettext(ipc, line);
          else
            strncpy(line, "abort_opcode(memory_error)", 1023);

          printf("%d/%08x  (opcode:%04x)  %s\n", context, cursor, ipc->opcode, line);
          cursor += piib->wordlen;
        }
        break;
      }

      else if (cmd[1] == 'u')
      {
        get_dbgr_prams(cmd, &mdcx, &mcursor, &size);
        if (dcx != context)
        {
          context = dcx;
          mmuflush(0x2000);
        }
        if (size > 127)
          size = 127;

        for (i = 0; i < size; i++)
        {
          printf("mmu[%d][%3d].slr:%04x,sor:%04x  %08x-%08x::-->(%08x)\n", mdcx, mcursor,
                 mmu_all[mdcx][mcursor].slr, mmu_all[dcx][mcursor].sor,
                 ((uint32)mcursor << 17), ((uint32)mcursor << 17) + ((1 << 17) - 1), (((mmu_all[dcx][mcursor].sor & 0x0fff) << 9) & TWOMEGMLIM));
          mcursor++;
        }
      }
      else if (cmd[1] == 'p')
      {
        uint32 i; // private i  tee hee, tee hee.

        get_dbgr_prams(cmd, &dcx, &cursor, &size);
        if (dcx != context)
        {
          context = dcx;
          mmuflush(0x2000);
        }
        size >>= 4;

        if (size > 1)
          puts("\ncx/addr    : +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +a +b +c +d +e +f | ascii");

        for (i = cursor; i < (cursor + size); i++)
        {
          if (cursor + 16 > maxlisaram)
            cursor = 0;
          printf("%d/%08x: %02x %02x %02x %02x %02x %02x %02x %02x:%02x %02x %02x %02x %02x %02x %02x %02x |%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                 i,
                 lisaram[i + 0], lisaram[i + 1], lisaram[i + 2], lisaram[i + 3], lisaram[i + 4], lisaram[i + 5], lisaram[i + 6], lisaram[i + 7],
                 lisaram[i + 8], lisaram[i + 9], lisaram[i + 10], lisaram[i + 11], lisaram[i + 12], lisaram[i + 13], lisaram[i + 14], lisaram[i + 15],
                 ((lisaram[i + 0] > ' ' && lisaram[i + 0] < 127) ? lisaram[i + 0] : '.'),
                 ((lisaram[i + 1] > ' ' && lisaram[i + 1] < 127) ? lisaram[i + 1] : '.'),
                 ((lisaram[i + 2] > ' ' && lisaram[i + 2] < 127) ? lisaram[i + 2] : '.'),
                 ((lisaram[i + 3] > ' ' && lisaram[i + 3] < 127) ? lisaram[i + 3] : '.'),
                 ((lisaram[i + 4] > ' ' && lisaram[i + 4] < 127) ? lisaram[i + 4] : '.'),
                 ((lisaram[i + 5] > ' ' && lisaram[i + 5] < 127) ? lisaram[i + 5] : '.'),
                 ((lisaram[i + 6] > ' ' && lisaram[i + 6] < 127) ? lisaram[i + 6] : '.'),
                 ((lisaram[i + 7] > ' ' && lisaram[i + 7] < 127) ? lisaram[i + 7] : '.'),
                 ((lisaram[i + 8] > ' ' && lisaram[i + 8] < 127) ? lisaram[i + 8] : '.'),
                 ((lisaram[i + 9] > ' ' && lisaram[i + 9] < 127) ? lisaram[i + 9] : '.'),
                 ((lisaram[i + 10] > ' ' && lisaram[i + 10] < 127) ? lisaram[i + 10] : '.'),
                 ((lisaram[i + 11] > ' ' && lisaram[i + 11] < 127) ? lisaram[i + 11] : '.'),
                 ((lisaram[i + 12] > ' ' && lisaram[i + 12] < 127) ? lisaram[i + 12] : '.'),
                 ((lisaram[i + 13] > ' ' && lisaram[i + 13] < 127) ? lisaram[i + 13] : '.'),
                 ((lisaram[i + 14] > ' ' && lisaram[i + 14] < 127) ? lisaram[i + 14] : '.'),
                 ((lisaram[i + 15] > ' ' && lisaram[i + 15] < 127) ? lisaram[i + 15] : '.'));
          cursor += 16;
        }
      }
      else if (cmd[1] == 'm')
      {
        uint32 i; // private i  tee hee, tee hee. - ok this joke is getting old

        get_dbgr_prams(cmd, &dcx, &cursor, &size);
        if (dcx != context)
        {
          context = dcx;
          mmuflush(0x2000);
        }
        size >>= 4;

        if (size > 1)
          puts("\ncx/addr    : +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +a +b +c +d +e +f | ascii");

        for (i = cursor; i < (cursor + size); i++)
        {
          cursor &= 0x00ffffff;
          abort_opcode = 13;
          printf("%d/%08x: %02x %02x %02x %02x %02x %02x %02x %02x:%02x %02x %02x %02x %02x %02x %02x %02x |%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                 i,
                 busfreefetchbyte(i + 0), busfreefetchbyte(i + 1), busfreefetchbyte(i + 2), busfreefetchbyte(i + 3), busfreefetchbyte(i + 4), busfreefetchbyte(i + 5), busfreefetchbyte(i + 6), busfreefetchbyte(i + 7),
                 busfreefetchbyte(i + 8), busfreefetchbyte(i + 9), busfreefetchbyte(i + 10), busfreefetchbyte(i + 11), busfreefetchbyte(i + 12), busfreefetchbyte(i + 13), busfreefetchbyte(i + 14), busfreefetchbyte(i + 15),
                 ((busfreefetchbyte(i + 0) > ' ' && busfreefetchbyte(i + 0) < 127) ? busfreefetchbyte(i + 0) : '.'),
                 ((busfreefetchbyte(i + 1) > ' ' && busfreefetchbyte(i + 1) < 127) ? busfreefetchbyte(i + 1) : '.'),
                 ((busfreefetchbyte(i + 2) > ' ' && busfreefetchbyte(i + 2) < 127) ? busfreefetchbyte(i + 2) : '.'),
                 ((busfreefetchbyte(i + 3) > ' ' && busfreefetchbyte(i + 3) < 127) ? busfreefetchbyte(i + 3) : '.'),
                 ((busfreefetchbyte(i + 4) > ' ' && busfreefetchbyte(i + 4) < 127) ? busfreefetchbyte(i + 4) : '.'),
                 ((busfreefetchbyte(i + 5) > ' ' && busfreefetchbyte(i + 5) < 127) ? busfreefetchbyte(i + 5) : '.'),
                 ((busfreefetchbyte(i + 6) > ' ' && busfreefetchbyte(i + 6) < 127) ? busfreefetchbyte(i + 6) : '.'),
                 ((busfreefetchbyte(i + 7) > ' ' && busfreefetchbyte(i + 7) < 127) ? busfreefetchbyte(i + 7) : '.'),
                 ((busfreefetchbyte(i + 8) > ' ' && busfreefetchbyte(i + 8) < 127) ? busfreefetchbyte(i + 8) : '.'),
                 ((busfreefetchbyte(i + 9) > ' ' && busfreefetchbyte(i + 9) < 127) ? busfreefetchbyte(i + 9) : '.'),
                 ((busfreefetchbyte(i + 10) > ' ' && busfreefetchbyte(i + 10) < 127) ? busfreefetchbyte(i + 10) : '.'),
                 ((busfreefetchbyte(i + 11) > ' ' && busfreefetchbyte(i + 11) < 127) ? busfreefetchbyte(i + 11) : '.'),
                 ((busfreefetchbyte(i + 12) > ' ' && busfreefetchbyte(i + 12) < 127) ? busfreefetchbyte(i + 12) : '.'),
                 ((busfreefetchbyte(i + 13) > ' ' && busfreefetchbyte(i + 13) < 127) ? busfreefetchbyte(i + 13) : '.'),
                 ((busfreefetchbyte(i + 14) > ' ' && busfreefetchbyte(i + 14) < 127) ? busfreefetchbyte(i + 14) : '.'),
                 ((busfreefetchbyte(i + 15) > ' ' && busfreefetchbyte(i + 15) < 127) ? busfreefetchbyte(i + 15) : '.'));
          cursor += 16;
          if (abort_opcode != 13 && abort_opcode)
          {
            puts("Bus error!");
            abort_opcode = 0;
            break;
          }
        }
        abort_opcode = 0;
        break;
      }

      puts("SYNTAX ERROR");
      break;

    case 'q':
      Quit();
      EXIT(1);

    case 'v':
      videoramdirty = 1;
      LisaScreenRefresh();
      break;

    case 'f':
      mmuflush(0x2000);
      break;

    case 's': // set commands;
    case 'c': // clear breakpoint
    case 't': // trace
    case 'r': // reboot emulator
      puts("sorry, not implemented yet");
      break;

    case 'g':
      pc24 = oldpc;
      return;

    case '?':
    case 'h':
    default:
      //.......1.........2.........3.........4.........5.........6.........7
      // 3456789012345678901234567890123456789012345678901234567890123456789012345678
      puts("\n\
    ?               - this help screen                           \n\
\n\
cpu regs:a0-7,d0-7,sr,sp. values are hex unless preceeded by +   \n\
addresses are in hex. i.e. 1/00fc0000 is fc0000 in context 1     \n\
\n\
control commands:                                                \n\
    f               - flush mmu cache                            \n\
    t               - trace one instruction                      \n\
    g               - go (continue execution)                    \n\
    q               - quit emulator                              \n\
    r               - reboot emulator                            \n\
    v               - refresh video window                       \n\
\n\
display commands:                                                \n\
    dv              - display via 1,2 registers                  \n\
    dv1             - display COPS via                           \n\
    dv2             - display parallel port via registers        \n\
    dc              - display COPS buffer                        \n\
    dz              - display z8530 buffer                       \n\
    dh              - display hard drive machine state           \n\
    di addr         - list assembly instructions at address      \n\
    dr              - display cpu registers                      \n\
    du {0-4/}{0-127}- display mmu registers                      \n\
    dm addr         - display memory at address                  \n\
    dp addr         - display physical memory                    \n\
\n\
None of the following are implemented \n\
    ds s/h/t        - display sector side/head/track             \n\
    db              - display breakpoints                        \n\
\n\
set commands:                                                    \n\
    sm addr val     - memory at address                          \n\
    sp addr val     - physical memory                            \n\
    su reg  val     - mmu register                               \n\
    sr reg  val     - registers                                  \n\
    sbo opcode      - break on opcode (ffff to disable)          \n\
    sbm address     - break on access to address                 \n\
    sbp address     - break on access to physical address        \n\
    cbo             - clear break on opcode                      \n\
    cbm             - clear break on memory                      \n\
    cbp             - clear break on physical memory             \n\
");
    }
  }
}

#endif

/*----------------------------------------------------------

                Another Glitch in the Call
                ------- ------ -- --- ----
        (Sung to the tune of a recent Pink Floyd song.)

We don't need no indirection
We don't need no flow control
No data typing or declarations
Did you leave the lists alone?

        Hey!  Hacker!  Leave those lists alone!

Chorus:
        All in all, it's just a pure-LISP function call.
        All in all, it's just a pure-LISP function call.


(From this morning's fortune.)
-------------------------------------------------------------*/
