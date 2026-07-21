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
*                                                                                      *
*                              COPS.C Routines                                         *
*                                                                                      *
\**************************************************************************************/

/*  Yes, we are including a .c file in here, that's because this file is actually the
 * keyboard table array, so it's nicer to have it in a separate file and since it doesn't
 * actually contain any code, it doesn't make sense to have a .h file for it with just a
 * single extern refrence to the array which would be lame.
 *
 */

#define IN_COPS_C
#include <vars.h>
#include <keyscan.h>
#include <keytable.h>

// Older COPS behavior is to send keyboard unplugged signal before keyboard id,
// and mouse unplugged signal before mouse plugged signal on a reset.
// // #define OLD_COPS_BEHAVIOR 1

#define LISA_REBOOTED(x) \
  {                      \
    save_pram();         \
    profile_unmount();   \
    lisa_rebooted();     \
    return;              \
  }

// this one is a bit more convoluted because we need to force reg68k.c to quit without doing anything else, or crashing the emulator.
#define LISA_POWEREDOFF(x)              \
  {                                     \
    save_pram();                        \
    profile_unmount();                  \
    lisa_powered_off();                 \
    pc24 = 0xffffffff;                  \
    cpu68k_clocks_stop = cpu68k_clocks; \
    abort_opcode = 1;                   \
    memset(lisaram, 0xff, 65536);       \
    return;                             \
  }

static int32 cops_key_id = 0;
static int keyboard_keytronix = 1; // need to recode this as user editable

// used to debug LisaTest (mouse seeking doesn't work for this OS, likely because it's
// either freezing after displaying the menu, or it stores mouse X,Y somewhere else in memory.
//
// So I cheat by using control+shift+comma to enable it, then control-shift-arrow and . to work the mouse
// while debugging is enabled, so I can see where mouse x,y is stored.
//
int mouse_keys_enabled = 0, mouse_keys_x = 0, mouse_keys_y = 0;

// duplicated from VIA:
#define VIA_CLEAR_IRQ_PORT_A(x)                                      \
  {                                                                  \
    if (((via[x].via[PCR] >> 1) & 7) != 1 &&                         \
        ((via[x].via[PCR] >> 1) & 7) != 3)                           \
      via[x].via[IFR] &= (0xff - VIA_IRQ_BIT_CA2 - VIA_IRQ_BIT_CA1); \
    via[x].ca1 = 0;                                                  \
    via[x].ca2 = 0;                                                  \
    if (via[x].via[IFR] == 128)                                      \
      via[x].via[IFR] = 0;                                           \
  }
// clear Cb1/Cb2 on ORb/IRb access
#define VIA_CLEAR_IRQ_PORT_B(x)                                      \
  {                                                                  \
    if (((via[x].via[PCR] >> 5) & 7) != 1 &&                         \
        ((via[x].via[PCR] >> 5) & 7) != 3)                           \
      via[x].via[IFR] &= (0xff - VIA_IRQ_BIT_CB2 - VIA_IRQ_BIT_CB1); \
    via[x].cb1 = 0;                                                  \
    via[x].cb2 = 0;                                                  \
    if (via[x].via[IFR] == 128)                                      \
      via[x].via[IFR] = 0;                                           \
  }
// clear SR irq on SR access
#define VIA_CLEAR_IRQ_SR(x)                      \
  {                                              \
    via[x].via[IFR] &= !VIA_IRQ_BIT_SR;          \
    if (via[x].via[IFR] == 128)                  \
      via[x].via[IFR] = 0;                       \
    DEBUG_LOG(0, "SR IRQ on via %d cleared", x); \
  }

///////////////////////////////////

void init_clock(void);

void bigm_delta(int16 x, int16 y);
// void recalibratemouse_inside(int x,int y,int wx,int wy,int ww,int wh);

#define MAXQUEUEFULL 32
// static  int16 diff_mouse_x=0,    diff_mouse_y=0,    diff_mouse_button=0;
static uint16 copsqueuefull = 0;

#define COPS_RES_KBFAILURE 0xff /* Keyboard COPS failure detected  */
#define COPS_RES_IOFAILURE 0xfe /* I/O Board COPS failure detected */
#define COPS_RES_KBUNPLUGD 0xfd /* Keyboard unplugged              */
#define COPS_RES_CLOCKTIRQ 0xfc /* clock timer interrupt           */
#define COPS_RES_POWERKEY 0xfb  /* Soft Power Switch hit           */

#define COPS_CLOCK_0 0xe0 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_1 0xe1 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_2 0xe2 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_3 0xe3 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_4 0xe4 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_5 0xe5 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_6 0xe6 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_7 0xe7 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_8 0xe8 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_9 0xe9 /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_a 0xea /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_b 0xeb /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_c 0xec /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_d 0xed /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_e 0xee /* COPS year  followed by 5 bytes */
#define COPS_CLOCK_f 0xef /* COPS year  followed by 5 bytes */

/* dd dh hm ms st - ddd day, hh-hour, mm min ss=second t=tenths of second   */

/* Keyboard ID # produced whenever the keyboard cops is reset.  valid codes
   are  keytronix id,   apd  */

#define COPS_KEYID_FRENCH 0xAD2d
#define COPS_KEYID_GERMAN 0xAE2e
#define COPS_KEYID_UK 0xAF2f
#define COPS_KEYID_US 0xBF2f

#define GET_RAT_XY(z)                                                        \
  {                                                                          \
    DEBUG_LOG(0, "Reading Mouse XY (mouse_keys is:%d)", mouse_keys_enabled); \
    if (mouse_keys_enabled)                                                  \
    {                                                                        \
      ratx = mouse_keys_x;                                                   \
      raty = mouse_keys_y;                                                   \
    }                                                                        \
    else                                                                     \
    {                                                                        \
      ratx = lisa_ram_safe_getword(1, lisa_os_mouse_x_ptr);                  \
      raty = lisa_ram_safe_getword(1, lisa_os_mouse_y_ptr);                  \
    }                                                                        \
  }

static int mouse_seek_count = 0;
static int last_mouse_button_state = 0; // this is used to prevent repeated mouse up, mouse down events being sent.
                                        // sort of like a switch debounce

// keep track of the last 3 mouse positions when we've sent.
// initialize these with 3 different values
static uint16 ratx = 8192, raty = 8192,
              last_ratx = 16384, last_raty = 16384,
              llast_ratx = 32767, llast_raty = 32767;

// Is Lisa listening to the mouse?
uint8 is_lisa_mouse_on(void)
{
  uint8 r;
  r = lisa_ram_safe_getlong(1, 0x490) != 0xffffffff;
  DEBUG_LOG(0, "Checking is Mouse On? - %s", r ? "yes" : "no");
  return r;
  //     (! (fetchbyte(0x49a)|(!fetchbyte(0x49b))) );
}

// ifdef DEBUG
void my_dump_cops(FILE *buglog)
{
  int i;

  fflush(buglog);

  if (copsqueuelen <= 0)
  {
    switch (copsqueuelen)
    {
    case 0:
      DEBUG_LOG(0, "COPS queue is empty, and no mouse motion is pending."); /* FALLTHRU */
    case -1:
      DEBUG_LOG(0, "Mouse: pending the send of 00"); /* FALLTHRU */
    case -2:
      DEBUG_LOG(0, "Mouse: pending the send of mouse_x_delta %d", mouse_pending_x); /* FALLTHRU */
    case -3:
      DEBUG_LOG(0, "Mouse: pending the send of mouse_y_delta %d", mouse_pending_y); /* FALLTHRU */
    default:
      DEBUG_LOG(0, "ERROR! copsqueuelen is negative but out of range! %d", copsqueuelen);
    }
    return;
  }

  fprintf(buglog, "SRC: COPS: QUEUE contains: %d items:\nSRC: COPS:", copsqueuelen);
  for (i = 0; i < copsqueuelen; i++)
  {
    fprintf(buglog, "%02x ", copsqueue[i]);
  }
  fprintf(buglog, "\n");
  fflush(buglog);
}
// #else
///
void dump_cops(FILE *buglog) { my_dump_cops(buglog); }

// #endif

//     dump_cops(buglog); was at the end of these
#define SEND_COPS_CODE(x)                                                          \
  {                                                                                \
    set_kb_data_ready();                                                           \
    DEBUG_LOG(0, "COPS queue len: %d adding 0x%02x", copsqueuelen, (unsigned)(x)); \
    if (copsqueuelen >= 0 && (copsqueuelen + 1 < MAXCOPSQUEUE))                    \
      copsqueue[copsqueuelen] = (x);                                               \
    copsqueuelen++;                                                                \
  }

#define cops_reset_status(x)                                              \
  {                                                                       \
    set_kb_data_ready();                                                  \
    DEBUG_LOG(0, "COPS queue len: %d adding reset (0x80)", copsqueuelen); \
    if (copsqueuelen >= 0 && (copsqueuelen + 1 < MAXCOPSQUEUE))           \
      copsqueue[copsqueuelen] = 0x80;                                     \
    copsqueuelen++;                                                       \
  }
#define SEND_RESETCOPS_AND_CODE(x)                                                           \
  {                                                                                          \
    set_kb_data_ready();                                                                     \
    DEBUG_LOG(0, "COPS queue len: %d adding reset and code (0x80+0x%02x)", copsqueuelen, x); \
    if (copsqueuelen >= 0 && (copsqueuelen + 2 < MAXCOPSQUEUE))                              \
    {                                                                                        \
      copsqueue[copsqueuelen] = 0x80;                                                        \
      copsqueuelen++;                                                                        \
      copsqueue[copsqueuelen] = (x);                                                         \
      copsqueuelen++;                                                                        \
    }                                                                                        \
  }

#ifndef abs
#define abs(x) (x < 0 ? -x : x)
#endif

#ifndef sgn
#define sgn(x) (x < 0 ? -1 : (x > 0 ? 1 : 0))
#endif

void bigm_delta(int16 x, int16 y) // x,y are delta-x, delta-y, but there are limits (max=+/-127) this normalizes it.
{
  int16 dx = 0, dy = 0, max = 32; // max=127;

  DEBUG_LOG(0, "x,y inputs before dx,dy:%d,%d", x, y);

  if (lisa_os_mouse_x_ptr != 0x486)
  {
    max = 127; // hack to limit Lisa Test Mouse Jitter.
    while (x || y)
    {
      dx = 0;
      dy = 0;

      if (x > -(max + 1) && x < 1)
      {
        dx = x;
        x = 0;
      }
      else if (x < -max)
      {
        dx = -max;
        x += max;
      }
      else if (x > 0 && x < (max + 1))
      {
        dx = x;
        x = 0;
      }
      else if (x > max)
      {
        dx = +max;
        x -= max;
      }

      if (y > -(max + 1) && y < 1)
      {
        dy = y;
        y = 0;
      }
      else if (y < -max)
      {
        dy = -max;
        y += max;
      }
      else if (y > 0 && y < (max + 1))
      {
        dy = y;
        y = 0;
      }
      else if (y > max)
      {
        dy = +max;
        y -= max;
      }
    }

    // reduce jitter caused by acceleration a bit further when getting close to the mouse pointer

    if (abs(dx) > mouse_x_halfing_tolerance)
      dx >>= 1;
    if (abs(dy) > mouse_y_halfing_tolerance)
      dy >>= 1;

    mouse_pending = -1;
    mouse_pending_x = dx;
    mouse_pending_y = dy;

    // DEBUG_LOG(0,"Returing dx,dy:%d,%d",dx,dy);

    return;
  }

  while (abs(x) > mouse_x_tolerance || abs(y) > mouse_y_tolerance) // jitter reduction
  {
    dx = 0;
    dy = 0;

    if (x > -(max + 1) && x < 1)
    {
      dx = x;
      x = 0;
    }
    else if (x < -max)
    {
      dx = -max;
      x += max;
    }
    else if (x > 0 && x < (max + 1))
    {
      dx = x;
      x = 0;
    }
    else if (x > max)
    {
      dx = +max;
      x -= max;
    }

    if (y > -(max + 1) && y < 1)
    {
      dy = y;
      y = 0;
    }
    else if (y < -max)
    {
      dy = -max;
      y += max;
    }
    else if (y > 0 && y < (max + 1))
    {
      dy = y;
      y = 0;
    }
    else if (y > max)
    {
      dy = +max;
      y -= max;
    }

    // reduce jitter caused by acceleration a bit further when getting close to the mouse pointer
    mouse_pending = -1;
    mouse_pending_x = dx;
    mouse_pending_y = dy;
    //      copsqueuelen=-1;                  // -1=send 00, -2=sendx, -3=sendy, 0=copsqueuelen=0;
  }
}

// mirrored inside reg68k as inline for speed.

int get_cops_pending_irq(void)
{
  // bit 7 of IFR indicates whether any VIA1 IRQ's have been fired, so check to see if any of them have, then set bit 7
  if (via[1].via[IER] & via[1].via[IFR] & 0x7f)
  {
    via[1].via[IFR] |= 0x80;
    return 0x80;
  }

  via[1].via[IFR] &= 0x7f;
  return 0;
}

void keystroke_cops(unsigned char c)
{
  uint8 k;
  uint8 j, len;

  //    DEBUG_LOG(0,"SRC: COPS Keystroke %02x %c",c,c>31 && c<127 ? c:'*');

  for (len = 0, j = 0; j < 9; j++)
    if (keydecodetable[(unsigned)c][j])
      len = j;

  DEBUG_LOG(0, "key length is:%d", len);

  // should this be <=0????
  if (!len || copsqueuelen < 0 || copsqueuelen > MAXCOPSQUEUE - len - 1)
  {
    DEBUG_LOG(0, "Could not add keystroke %02x because:qlen=%d len=%d", c, copsqueuelen, len);
    return;
  }

  for (j = 0; j <= len; j++)
  {
    k = keydecodetable[c][j];
    if (k)
    {
      SEND_COPS_CODE(k);
    }
    else
    {
      if (k == NMIKEY && NMIKEY)
      {
        fprintf(buglog, "COPS NMI KEY:%04x hit, firing.\n", NMIKEY);
        lisa_nmi_vector(CHK_MMU_TRANS(pc24));
      }
    }
  }
  SET_COPS_NEXT_EVENT(0);
}

void send_nmi_key(void)
{
  if ((NMIKEY & 0x7f) == 0)
  {
    if (yesnomessagebox("The Lisa isn't ready to accept an NMI, are you sure?",
                        "Lisa Not ready for NMI") == 0)
      return;
  }

  //    DEBUG_LOG(0,"SRC: COPS User NMI key");
  if (copsqueuelen < 0 || copsqueuelen > MAXCOPSQUEUE - 8)
  {
    messagebox("Could not add NMI keystroke because cops queue is full", "COPS queue full");
    DEBUG_LOG(0, "Could not add NMI keystroke because:qlen=%d", NMIKEY, copsqueuelen);
    return;
  }

  if (NMIKEY & 0x80)
    SEND_COPS_CODE(KEYCODE_COMMAND | KEY_DOWN); // apple key down
                                                // if (NMIKEY & 0x40)  SEND_COPS_CODE(KEYCODE_LOPTION|KEY_DOWN); // option key down
  SEND_COPS_CODE((NMIKEY & 0x7f) | KEY_DOWN);   // NMI key down          //3f if option enabled
  SEND_COPS_CODE((NMIKEY & 0x7f));              // NMI key up            //3f if option enabled
                                                // if (NMIKEY & 0x40)  SEND_COPS_CODE(KEYCODE_LOPTION         ); // option key down
  if (NMIKEY & 0x80)
    SEND_COPS_CODE(KEYCODE_COMMAND); // apple key up

  lisa_nmi_vector(CHK_MMU_TRANS(pc24));

  SET_COPS_NEXT_EVENT(0);
}

void send_cops_keycode(int k)
{
  SEND_COPS_CODE(k);
  SET_COPS_NEXT_EVENT(0);
}

void cops_timer_alarm(void)
{
  DEBUG_LOG(0, "Sending Alarm code.");
  // SEND_COPS_CODE(COPS_RES_CLOCKTIRQ);}
  SEND_RESETCOPS_AND_CODE(COPS_RES_CLOCKTIRQ);
}

// static uint8 releasekey=0;
// static uint8 release_shift=0;
// static uint8 release_cmd=0;
// char *release_key_s;

void apple_combo_key(uint8 key, uint8 shift, uint8 cmdkey, uint8 option, char *s)
{
  int size = 1;

  ALERT_LOG(0, "Sending %s", s);

  if (shift)
    size += 2;
  if (cmdkey)
    size += 2;
  if (option)
    size += 2;

  if (copsqueuelen > MAXCOPSQUEUE - size || copsqueuelen < 0)
  {
    ALERT_LOG(0, "cannot send key - copsqlen");
    return;
  }

  if (option)
    SEND_COPS_CODE(KEYCODE_OPTION | KEY_DOWN);
  if (cmdkey)
    SEND_COPS_CODE(KEYCODE_COMMAND | KEY_DOWN);
  if (shift)
    SEND_COPS_CODE(KEYCODE_SHIFT | KEY_DOWN);

  /*      */ SEND_COPS_CODE(key | KEY_DOWN);

  /*      */ SEND_COPS_CODE(key | KEY_UP);

  if (shift)
    SEND_COPS_CODE(KEYCODE_SHIFT | KEY_UP);
  if (cmdkey)
    SEND_COPS_CODE(KEYCODE_COMMAND | KEY_UP);
  if (option)
    SEND_COPS_CODE(KEYCODE_OPTION | KEY_UP);

  if ((NMIKEY & 0x7f) == key)
  {
    ALERT_LOG(0, "Also sending NMI");
    lisa_external_nmi_vector(pc24);
  }

  ALERT_LOG(0, "done.")
  UNUSED(s);
}

//                                            KEYCODE_,   shift,apple,option, text for logging
void apple_1(void) { apple_combo_key(KEYCODE_1, 0, 1, 0, "Apple-1"); }
void apple_2(void) { apple_combo_key(KEYCODE_2, 0, 1, 0, "Apple-2"); }
void apple_3(void) { apple_combo_key(KEYCODE_3, 0, 1, 0, "Apple-3"); }
void apple_enternum(void) { apple_combo_key(KEYCODE_ENTERNUM, 0, 1, 0, "Apple-ENTERNUM"); }
void apple_enter(void) { apple_combo_key(KEYCODE_ENTERNUM, 0, 1, 0, "Apple-ENTERNUM"); }
void apple_renter(void) { apple_combo_key(KEYCODE_LENTER, 0, 1, 0, "Apple-LENTER"); }
void apple_S(void) { apple_combo_key(KEYCODE_S, 0, 1, 0, "Apple-S"); }
void apple_dot(void) { apple_combo_key(KEYCODE_DOT, 0, 1, 0, "Apple-DOT"); }
void shift_option_0(void) { apple_combo_key(KEYCODE_0, 1, 0, 1, "Shift-Option-0"); }
void shift_option_4(void) { apple_combo_key(KEYCODE_4, 1, 0, 1, "Shift-Option-4"); }
void shift_option_7(void) { apple_combo_key(KEYCODE_7, 1, 0, 1, "Shift-Option-7"); }

void apple_dot_down(void)
{
  if ((NMIKEY & 0x7f) == KEYCODE_DOT)
  {
    lisa_external_nmi_vector(pc24);
    return;
  }
  if (copsqueuelen > MAXCOPSQUEUE - 3 || copsqueuelen < 0)
    return;
  DEBUG_LOG(0, "apple-. down");
  SEND_COPS_CODE(KEYCODE_LEFTCMD | KEY_DOWN);
  SEND_COPS_CODE(KEYCODE_DOT | KEY_DOWN);
}

void apple_dot_up(void)
{
  if (copsqueuelen > MAXCOPSQUEUE - 3 || copsqueuelen < 0)
    return;
  DEBUG_LOG(0, "apple-. up");
  SEND_COPS_CODE(KEYCODE_DOT | KEY_UP);
  SEND_COPS_CODE(KEYCODE_LEFTCMD | KEY_UP);
}

void presspowerswitch(void)
{
  DEBUG_LOG(0, "SRC: COPS: Pressing Power Switch");
  SEND_RESETCOPS_AND_CODE(0xFB); // 0x80 0xFB - Soft Power Switch Sequence
}

void cops_fire_clock_timer_irq(void)
{
  DEBUG_LOG(0, "SRC: COPS: Timer IRQ");
  SEND_RESETCOPS_AND_CODE(0xFB); // 0x80 0xFC - Clock Timer IRQ
}

void set_keyboard_id(int32 id)
{

  DEBUG_LOG(0, "SRC: COPS: Setting keyboard type to %08x", id);

  // incase it's out of range default to the US keyboard - is that super-pro-american patriotic or what? (humming yankee doodle...) :)
  if (id > 0xffff || id < -5)
  {
    cops_key_id = COPS_KEYID_US;
    return;
  }
  switch (id)
  {
  case -1:
    cops_key_id = COPS_KEYID_US;
    ALERT_LOG(0, "US");
    break;
  case -2:
    cops_key_id = COPS_KEYID_UK;
    ALERT_LOG(0, "UK");
    break;
  case -3:
    cops_key_id = COPS_KEYID_GERMAN;
    ALERT_LOG(0, "DE");
    break;
  case -4:
    cops_key_id = COPS_KEYID_FRENCH;
    ALERT_LOG(0, "FR");
    break;
  default:
    cops_key_id = (id & 0xffff);
    ALERT_LOG(0, "Other");
  }
}

// a keyboard ID is sent when the keyboard is plugged in, or power is turned on
void cops_keyboard_id(void)
{
  uint8 x;
  DEBUG_LOG(0, "SRC: COPS: reporting keyboard ID:%04x len:%d", cops_key_id, copsqueuelen);
  if (keyboard_keytronix)
    x = (cops_key_id >> 8) & 0xff; // keyboard id (allow user to change keyboards later)
  else
    x = (uint8)(cops_key_id & 0xff); // keyboard id (allow user to change keyboards later)

  if ((reg68k_pc & 0x00ff0000) == 0x00fe0000)
  {
    SEND_RESETCOPS_AND_CODE(x);
    ALERT_LOG(0, "Sending keyboard id: %02x", x)
    DEBUG_LOG(0, "len:%d", copsqueuelen);
  }
}

void cops_unplug_keyboard(void)
{
  DEBUG_LOG(0, "SRC: COPS: Unplug");
  SEND_RESETCOPS_AND_CODE(0xFD); // send 0x80 0xFD
}

void unplugmouse(void) // Used by initialization sequence - checked by Lisa POST
{
  /*  If the mouse is plugged in, 1000 0111 is returned, if unplugged,
      0000 0111 is returned. */
  DEBUG_LOG(0, "SRC: COPS: Unplug");
  // cops_mouse=0;
  SEND_COPS_CODE(0x07);
}

void plugmouse(void) // Used by initialization sequence - checked by Lisa POST
{
  /*  If the mouse is plugged in, 1000 0111 is returned, if unplugged,
      0000 0111 is returned. */
  if (!cops_mouse)
    cops_mouse = 400;
  SEND_COPS_CODE(0x87);
  DEBUG_LOG(0, "SRC: COPS: plugmouse. copsqueue=%d after pushing 80,87", copsqueuelen);
}

void set_mouse_button(int i)
{
  DEBUG_LOG(0, "mouse button event:%d %s", i, (i ? "down" : "up"));
  SEND_COPS_CODE((i ? 0x86 : 0x06));
}

// #define OLD_COPS_BEHAVIOR

void cops_reset(void) // write 0 to port b bit 0
{
  // Older COPS behavior is to send keyboard unplugged signal before keyboard id,
  // and mouse unplugged signal before mouse plugged signal on a reset.
  copsqueuelen = 0;

  // DEBUG_LOG(0,"COPS Reset");
  // if ((pc24 & 0x00ff0000)!=0x00fe0000) {debug_on("cops_reset"); debug_log_enabled=1;}
  // else {DEBUG_LOG(0,"Did not turn on logging for COPS_RESET because pc24:%08x",pc24);}

#ifdef OLD_COPS_BEHAVIOR
  cops_unplug_keyboard();
  cops_keyboard_id();
  unplugmouse();
  plugmouse();
#else
  cops_keyboard_id();
  // plugmouse();
#endif

#ifdef DEBUG
  if (0 == copsqueuelen)
  {
    DEBUG_LOG(0, "ERROR COPSQUEUELEN IS ZERO AFTER COPSRESET");
  }
  else
  {
    DEBUG_LOG(0, "After COPS Reset copsqueuelen=%d", copsqueuelen);
  }
#endif

  if ((lisa_clock.year & 0x0f) < 3)
    init_clock(); // prevent warning about clock by going to 1983.
}

void normalize_lisa_clock(void)
{
  int years, days, hours, minutes, seconds, tenths;

  years = lisa_clock.year & 0x0f;
  days = (((lisa_clock.days_h & 0xf0) >> 4) * 100 + (lisa_clock.days_h & 0x0f) * 10 + lisa_clock.days_l);
  hours = lisa_clock.hours_h * 10 + lisa_clock.hours_l;
  minutes = lisa_clock.mins_h * 10 + lisa_clock.mins_l;
  seconds = lisa_clock.secs_h * 10 + lisa_clock.secs_l;
  tenths = lisa_clock.tenths;

  //   fflush(buglog);

  if (tenths > 9)
    tenths = 9;
  if (seconds > 59)
    seconds = 59;
  if (minutes > 59)
    minutes = 59;
  if (hours > 23)
    hours = 23;
  if (days > 365)
    days = 365;

  lisa_clock.year = (0x0f & years) | 0xe0;
  if ((lisa_clock.year & 0x0f) < 3)
    init_clock(); // prevent warning about clock by going to 1983.

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

void normalize_lisa_set_clock(void)
{
  int years, days, hours, minutes, seconds, tenths;

  years = lisa_clock.year & 0x0f;
  days = (((lisa_clock.days_h & 0xf0) >> 4) * 100 + (lisa_clock.days_h & 0x0f) * 10 + lisa_clock.days_l);
  hours = lisa_clock.hours_h * 10 + lisa_clock.hours_l;
  minutes = lisa_clock.mins_h * 10 + lisa_clock.mins_l;
  seconds = lisa_clock.secs_h * 10 + lisa_clock.secs_l;
  tenths = lisa_clock.tenths;

  // fprintf(buglog,"normalizing_set clock\n"); fflush(buglog);

  // might just have to do a wrap around here instead?
  if (years == 0 && days == 366 && hours == 23 && minutes == 59 && seconds == 59 && tenths == 9)
  {
    years = 1;
    days = 0;
    hours = 23;
    minutes = 59;
    seconds = 59;
    tenths = 9;
    // fprintf(buglog,"hak done normalizing_set clock:: %d,%d %d:%d:%d.%d :: ",years,days,hours,minutes,seconds,tenths);
  }
  else
  {
    if (tenths > 9)
      tenths = 9;
    if (seconds > 59)
      seconds = 59;
    if (minutes > 59)
      minutes = 59;
    if (hours > 23)
      hours = 23;
    if (days > 365)
      days = 365;
    // fprintf(buglog,"done normalizing_set clock:: %d,%d %d:%d:%d.%d :: ",years,days,hours,minutes,seconds,tenths);
  }

  // fprintf(buglog,"\n");

  lisa_clock.year = (0x0f & years) | 0xe0;
  if ((lisa_clock.year & 0x0f) < 3)
    init_clock(); // prevent warning about clock by going to 1983.

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

  // fprintf(buglog,"hex:: %02x %02x%x  %x%x:%x%x:%x%x.%x \n",lisa_clock.year, lisa_clock.days_h, lisa_clock.days_l,
  //                                                           lisa_clock.hours_h,lisa_clock.hours_l,
  //                                                           lisa_clock.mins_h,lisa_clock.mins_l,
  //                                                           lisa_clock.secs_h,lisa_clock.secs_l,
  //                                                           lisa_clock.tenths);
  //
  //   fflush(buglog);
}

/*-----------------4/29/98 11:14AM------------------
 * These routines interface with the Lisa's VIA and
 * are called by the Lisa emulated code.
 * --------------------------------------------------*/

void init_clock(void)
{
  struct tm *timev;
  time_t clktime;
  uint8 dd_hun, dd_ten, dd_one;
  uint16 yday;

  clktime = time(&clktime);
  timev = (struct tm *)localtime(&clktime);

  yday = timev->tm_yday;
  dd_hun = (uint8)(yday / 100);
  dd_ten = (uint8)((yday / 10) % 10);
  dd_one = (uint8)(yday % 10);

  // It hasn't yet been initialized, so do that.
  if ((lisa_clock.year & 0xf0) != 0xe0)
  {
    //       ALERT_LOG(0,"date not set, setting now from system time.\n");
    // tenth_sec_cycles =cpu68k_clocks+500000;
    lisa_clock.year = 0xe7; //;|(timev->tm_year & 0x0f);
    if (lisa_clock.year < 0xe3)
      lisa_clock.year = 0xe0 | 0x07;
    lisa_clock.days_h = (dd_hun << 4) | dd_ten;

    lisa_clock.days_l = (dd_one);
    lisa_clock.hours_h = (timev->tm_hour / 10);

    lisa_clock.hours_l = (timev->tm_hour % 10);
    lisa_clock.mins_h = (timev->tm_min / 10);

    lisa_clock.mins_l = (timev->tm_min % 10);

    if (timev->tm_sec > 59)
      timev->tm_sec = 0;

    lisa_clock.secs_h = (timev->tm_sec / 10);

    lisa_clock.secs_l = (timev->tm_sec % 10);
    lisa_clock.tenths = 0;

    // hacks to do a test:
    // lisa_clock.days_l=2;
    // lisa_clock.days_h=0;
    // lisa_clock.hours_l=11;
    // lisa_clock.hours_h=0;
  }
}

void init_cops(void)
{
  copsqueuelen = 0;
  memset(copsqueue, 0, MAXCOPSQUEUE);
  lisa_clock.year = 0; // flag to get real time from host OS on first Lisa timer request.
  init_clock();
}

/* send commands to the cops, or to other stuff on VIA1 - not simple shit */
void via1_ora(uint8 data, uint8 regnum)
{

  VIA_CLEAR_IRQ_PORT_A(1); // clear CA1/CA2 on ORA/IRA access

  if (via[1].via[DDRA] == 0)
    return; // output is disabled, can't write.

  DEBUG_LOG(0, "SRC: SRC: COPS: command is is %02x regnum: %d CRDY line is %d", data, regnum, is_crdy_line());
  set_crdy_line();
  if (data & 0x80)
    return; // handle NOP's

  // added hack to fix issue when cops is in middle of mouse but lisa just sent a command
  // 20060607//copsqueuelen=0;
  ///

  switch (data)
  {
  case 0x00:
    cops_reset();
    DEBUG_LOG(0, "cops reset 0 - PORT ON\n");
    return; // turn off port - just ignore it (reset signal received)

  case 0x01:
    cops_reset();
    DEBUG_LOG(0, "cops reset 1 - PORT OFF cops turned off???\n");
    return; // not sure if this is correct;

  case 0x02: // read clock data

    if (copsqueuelen + 8 > MAXCOPSQUEUE)
    {
      fprintf(buglog, "\n\n\n\n     COPS OVERFLOW! CAN'T SET DATE!\n\n\n");
    }
    if (copsqueuelen + 7 < MAXCOPSQUEUE) // if the queue isn't full that is...
    {
      ALERT_LOG(0, "sending clock.\n");

#ifdef DEBUG
      DEBUG_LOG(0, "cops.c - sent cops clock time-date to lisa:\n");
      DEBUG_LOG(0, "before normalize hex:: %02x %02x%x  %x%x:%x%x:%x%x.%x \n", lisa_clock.year, lisa_clock.days_h, lisa_clock.days_l,
                lisa_clock.hours_h, lisa_clock.hours_l,
                lisa_clock.mins_h, lisa_clock.mins_l,
                lisa_clock.secs_h, lisa_clock.secs_l,
                lisa_clock.tenths);

      if (debug_log_enabled)
        printlisatime(buglog); // this calls normalize_lisa_clock, if you comment this, add normalize_lisa_clock(); here

#else
      normalize_lisa_clock();
#endif

      SEND_COPS_CODE(0x80);                                                   // COPS Reset Code
      SEND_COPS_CODE(((lisa_clock.year & 0x0f) | 0xe0));                      // 0xeY  Y=0-15
      SEND_COPS_CODE((lisa_clock.days_h));                                    // 0xdd  days
      SEND_COPS_CODE((lisa_clock.days_l << 4) | (lisa_clock.hours_h));        // 0xdh  h=tens of hours
      SEND_COPS_CODE((uint8)((lisa_clock.hours_l << 4) | lisa_clock.mins_h)); // 0xhm
      SEND_COPS_CODE((uint8)((lisa_clock.mins_l << 4) | lisa_clock.secs_h));  // 0xms
      SEND_COPS_CODE((uint8)((lisa_clock.secs_l << 4) | lisa_clock.tenths));  // 0xst t=tenths of sec, leave
                                                                              // empty unless OS can get
                                                                              // this for ya...
      DEBUG_LOG(0, "cops.c - done sending clock time-date normalizing_set::");
      DEBUG_LOG(0, "hex:: %02x %02x%x  %x%x:%x%x:%x%x.%x \n", lisa_clock.year, lisa_clock.days_h, lisa_clock.days_l,
                lisa_clock.hours_h, lisa_clock.hours_l,
                lisa_clock.mins_h, lisa_clock.mins_l,
                lisa_clock.secs_h, lisa_clock.secs_l,
                lisa_clock.tenths);

      return;
    } /* FALLTHRU */

  case 0x10: /* FALLTHRU */
  case 0x11: /* FALLTHRU */
  case 0x12: /* FALLTHRU */
  case 0x13: /* FALLTHRU */
  case 0x14: /* FALLTHRU */
  case 0x15: /* FALLTHRU */
  case 0x16: /* FALLTHRU */
  case 0x17: /* FALLTHRU */
  case 0x18: /* FALLTHRU */
  case 0x19: /* FALLTHRU */
  case 0x1a: /* FALLTHRU */
  case 0x1b: /* FALLTHRU */
  case 0x1c: /* FALLTHRU */
  case 0x1d: /* FALLTHRU */
  case 0x1e: /* FALLTHRU */
  case 0x1f: // 0001 xxxx  0001 nnnn - 0x1? write nnnn to clock
  {          // 8421 8421
    ALERT_LOG(0, "Writing %02x at position %d of LisaClockSet", data & 0x0f, lisa_clock_set_idx);

    if (lisa_clock_set_idx < 16)
      lisa_clock_set[lisa_clock_set_idx++] = data & 0x0f;
#ifdef DEBUG
    else
      DEBUG_LOG(0, "attempt to write %02x to clock set failed because index is %d", data, lisa_clock_set_idx);
#endif
    return;
  }

  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x24:
  case 0x25:
  case 0x26:
  case 0x27:
  case 0x28:
  case 0x29:
  case 0x2a:
  case 0x2b:
  case 0x2c:
  case 0x2d:
  case 0x2e:
  case 0x2f: // 0010 spmm - 0x2? set clock modes
  {          //      8421                      // 8421 8421
             /* 0010 spmm - 2x set clock modes: 00 -disable clock/timer, 01 - timer disable,
              *              10=timer underflow generates irq, 11=timer underflow turns system on if off, else generates irq */
    if ((data & 3) == 0)
    {
      lisa_clock_on = 0;
      lisa_alarm = 0;
    } // disable clock/timer both stop alarm timer.
    if ((data & 3) == 1)
      lisa_alarm = 0; // disable timer only
    lisa_alarm_power = (data & 7);
    DEBUG_LOG(0, "copsclk:  %d ALARM:%d Alarm Power:%d", lisa_clock_on, lisa_alarm, lisa_alarm_power);
    if (data & 8)
    {
      lisa_clock_set_idx = 0;
    } // entering setmode
    //            spmm
    //
    // 0x21 - 0010 0001 - 21==power off now and stay off. (timer off, clock on, power off)
    // 0x23 - 0010 0011 - 23==power cycle from Lisa ROM - have to enable this properly.

    // Check for poweroff command
    if (((data & 8) == 0) && lisa_alarm_power == 1) // timer is disabled, power=0, poweroff now
      LISA_POWEREDOFF(0);

    // timer based power cycle - should turn back on based on alarm
    if (((data & 8) == 0) && lisa_alarm_power == 3)
      LISA_POWEREDOFF(0);

    if (((data & 8) == 0) && lisa_alarm_power == 3)
      LISA_REBOOTED(0);

    // ALERT_LOG(0,"Data:%02x alarm:%02x\n",data,lisa_alarm_power);

    if (((data & 8) == 0) && lisa_clock_set_idx) // exitting setmode ////////////////////////////////////////
    {
      char temp[1024]; //, temp2[1024];
      lisa_alarm = 0;
      lisa_clock_on = 1;

      // fprintf(buglog,"copsclk:");
      switch (lisa_clock_set_idx - 1) // don't want breaks here, we want it to fall through!!
      {
      case 0xf:
        lisa_clock.tenths = lisa_clock_set[0xf]; /* FALLTHRU */
      case 0xe:
        lisa_clock.secs_l = lisa_clock_set[0xe]; /* FALLTHRU */
      case 0xd:
        lisa_clock.secs_h = lisa_clock_set[0xd]; /* FALLTHRU */
      case 0xc:
        lisa_clock.mins_l = lisa_clock_set[0xc]; /* FALLTHRU */
      case 0xb:
        lisa_clock.mins_h = lisa_clock_set[0xb]; /* FALLTHRU */
      case 0xa:
        lisa_clock.hours_l = lisa_clock_set[0xa]; /* FALLTHRU */
      case 0x9:
        lisa_clock.hours_h = lisa_clock_set[0x9]; /* FALLTHRU */

      case 0x8:
        lisa_clock.days_l = lisa_clock_set[0x8]; /* FALLTHRU */
                                                 //                               fprintf(buglog,"days_l=%02x ",lisa_clock.days_l); /* FALLTHRU */

      case 0x7:
        lisa_clock.days_h = ((lisa_clock_set[0x7] & 0x0f) | (lisa_clock.days_h & 0xf0)); /* FALLTHRU */
                                                                                         //                               fprintf(buglog,"days_h=%02x ",lisa_clock.days_h);           /* FALLTHRU */

      case 0x6:
        lisa_clock.days_h = ((lisa_clock_set[0x6] & 0x0f) << 4) | (lisa_clock.days_h & 0x0f); /* FALLTHRU */
                                                                                              //                               fprintf(buglog,"days_h=%02x ",lisa_clock.days_h);         /* FALLTHRU */

      case 0x5:
        lisa_clock.year = (lisa_clock_set[0x5] & 0x0f) | 0xe0; /* FALLTHRU */
                                                               //                               fprintf(buglog,"year=%02x ",lisa_clock.year);            /* FALLTHRU */
      case 0x4:                                                /* FALLTHRU */
      case 0x3:                                                /* FALLTHRU */
      case 0x2:                                                /* FALLTHRU */
      case 0x1:                                                /* FALLTHRU */
      case 0x0:
        lisa_alarm = 0;
        lisa_alarm = ((lisa_clock_set[0x4] & 0x0f)) | ((lisa_clock_set[0x3] & 0x0f) << 4) | ((lisa_clock_set[0x2] & 0x0f) << 8) | ((lisa_clock_set[0x1] & 0x0f) << 12) | ((lisa_clock_set[0x0] & 0x0f) << 16);

        //                                  fprintf(buglog,"alarm=%02x \n",lisa_alarm);
      }

      //               fprintf(buglog,"copsclk: Clock set: buffer:%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x idx:%d",
      //                                                  lisa_clock_set[ 0],
      //                                                  lisa_clock_set[ 1],
      //                                                  lisa_clock_set[ 2],
      //                                                  lisa_clock_set[ 3],
      //                                                  lisa_clock_set[ 4],     ////////////////
      //                                                  lisa_clock_set[ 5],
      //                                                  lisa_clock_set[ 6],
      //                                                  lisa_clock_set[ 7],
      //                                                  lisa_clock_set[ 8],
      //                                                  lisa_clock_set[ 9],
      //                                                  lisa_clock_set[10],
      //                                                  lisa_clock_set[11],
      //                                                  lisa_clock_set[12],
      //                                                  lisa_clock_set[13],
      //                                                  lisa_clock_set[14],
      //                                                  lisa_clock_set[15],lisa_clock_set_idx);

      lisa_clock_set_idx = 0;

      snprintf(temp, 1024, "copsclk: Clock set:cpuclk:%llx year:%d days:%d, hours:%x%x,mins:%x%x, secs:%x%x, tenths:%x alarm:%1x%1x%1x%1x%1x",
               cpu68k_clocks,
               1980 + (lisa_clock.year & 0x0f),

               (lisa_clock.days_h & 0x0f) * 10 + (lisa_clock.days_h >> 4) * 100 + lisa_clock.days_l,

               lisa_clock.hours_h, lisa_clock.hours_l,
               lisa_clock.mins_h, lisa_clock.mins_l,
               lisa_clock.secs_h, lisa_clock.secs_l,
               lisa_clock.tenths,
               lisa_clock_set[0x0], lisa_clock_set[0x1], lisa_clock_set[0x2], lisa_clock_set[0x3], lisa_clock_set[0x4]);
      // fprintf(buglog,temp);
      DEBUG_LOG(0, temp);
      DEBUG_LOG(0, temp);
      // debug_on("clock set");

      // #ifdef DEBUG
      //   fprintf(buglog,"cops.c - sent cops clock time-date to lisa:\n");
      //   printlisatime(buglog);
      // #endif

      normalize_lisa_set_clock();
    } ////////////////////////////////////////////////////////////
    return;
  }

  case 0x30:
  case 0x31:
  case 0x32:
  case 0x33:
  case 0x34:
  case 0x35:
  case 0x36:
  case 0x37:
  case 0x38:
  case 0x39:
  case 0x3a:
  case 0x3b:
  case 0x3c:
  case 0x3d:
  case 0x3e:
  case 0x3f:
    DEBUG_LOG(0, "SRC: 0x%2x Weird COPS command: set high keyboard LED's:%d%d%d%d", data, data & 8, data & 4, data & 2, data & 1);
    return;

  case 0x40:
  case 0x41:
  case 0x42:
  case 0x43:
  case 0x44:
  case 0x45:
  case 0x46:
  case 0x47:
  case 0x48:
  case 0x49:
  case 0x4a:
  case 0x4b:
  case 0x4c:
  case 0x4d:
  case 0x4e:
  case 0x4f:
    DEBUG_LOG(0, "SRC: 0x%2x Weird COPS command: set low keyboard LED's:%d%d%d%d", data, data & 8, data & 4, data & 2, data & 1);
    return;

  case 0x50: // set high nibble of NMI key  // or disable NMI to nothing
  case 0x51:
  case 0x52:
  case 0x53:
  case 0x54:
  case 0x55:
  case 0x56:
  case 0x57:
  case 0x58:
  case 0x59:
  case 0x5a:
  case 0x5b:
  case 0x5c:
  case 0x5d:
  case 0x5e:
  case 0x5f:
    NMIKEY = (NMIKEY & 0x0f) | (data & 0x0f) << 4;
    ALERT_LOG(0, "");
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "cops command - set nmi key high:%02x NMI key is now:%02x", data, NMIKEY);
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "");
    return;

  case 0x60: // set low nibble of NMI key
  case 0x61:
  case 0x62:
  case 0x63:
  case 0x64:
  case 0x65:
  case 0x66:
  case 0x67:
  case 0x68:
  case 0x69:
  case 0x6a:
  case 0x6b:
  case 0x6c:
  case 0x6d:
  case 0x6e:
  case 0x6f:
    NMIKEY = (NMIKEY & 0xf0) | (data & 0x0f);
    ALERT_LOG(0, "");
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "cops command - set nmi key low:%02x NMI key is now:%02x\n", data, NMIKEY);
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "=====================================================================");
    ALERT_LOG(0, "");
    return;

    // cops irq timing in miliseconds * 4.  i.e. 7c=4*4
  case 0x70:
  case 0x71:
  case 0x72:
  case 0x73:
  case 0x74:
  case 0x75:
  case 0x76:
  case 0x77:
    DEBUG_LOG(0, "SRC: Mouse turned off %02x copsqueuelen:%d", data, copsqueuelen);
    // fprintf(buglog,"COPS Timer: mouse turned off :%02x\n",data);
    cops_mouse = 0;
    SET_COPS_NEXT_EVENT(0);
    return; // turn mouse off

  case 0x78:
  case 0x79:
  case 0x7a:
  case 0x7b:
  case 0x7c:
  case 0x7d:
  case 0x7e:
  case 0x7f:
    DEBUG_LOG(0, "SRC: Mouse turned on %02x", data);
    // fprintf(buglog,"COPS Timer: mouse set to:%02x\n",data);
    cops_mouse = (data & 7) * COPS_IRQ_TIMER_FACTOR;
    SET_COPS_NEXT_EVENT(0);
    return; // turn mouse on

  case 0x80: // COPS NOP - just ignore'em
  case 0x81:
  case 0x82:
  case 0x83:
  case 0x84:
  case 0x85:
  case 0x86:
  case 0x87:
  case 0x88:
  case 0x89:
  case 0x8a:
  case 0x8b:
  case 0x8c:
  case 0x8d:
  case 0x8e:
  case 0x8f:
    return;

  default:
    DEBUG_LOG(0, "COPS: unimplemented cops command %02x", data);
  }

  // does the lisa actually use this? - maybe we can ignore it... maybe...
  DEBUG_LOG(0, "SRC: COPS::0x%x Set Timer mode... uncompleted emulator code.", data);

  UNUSED(regnum);

  return;
}

/* read data from cops queue  */

uint8 via1_ira(uint8 regnum)
{
  uint8 data;
  register int16 i;
  copsqueuefull = 0; // since the Lisa is reading the queue, assume it's alive,
                     // clear the copsqueuefull watchdog.
  // ALERT_LOG(0,"IRA1 copsqueuelen:%d, mousequeuelen:%d cops_mouse:%d",copsqueuelen,mousequeuelen,cops_mouse);

#ifdef DEBUG

  if (debug_log_enabled)
  {
    fprintf(buglog, "IRA1 COPS Queue:");
    for (i = 0; i < MAXCOPSQUEUE; i++)
      fprintf(buglog, "%02x ", copsqueue[i]);
    fprintf(buglog, "\n");
  }
#endif

  VIA_CLEAR_IRQ_PORT_A(1); // clear CA1/CA2 on ORA/IRA access

  // if there are no keystrokes pending, send mouse events.
  if (copsqueuelen <= 0)
  {
    DEBUG_LOG(0, "copsqueuelen=%d so sending mouse events.", copsqueuelen);
    copsqueuelen = 0;

    DEBUG_LOG(0, "IRA1: since COPS queue<=0 (%d), will send mouse delta X,Y:[0x00],%d,%d", copsqueuelen, mouse_pending_x, mouse_pending_y);

    i = (uint8)(mouse_pending_x); /* if (!i) {flipflopdx^=0xff;  i=flipflopdx;} */
    SEND_COPS_CODE(i);

    i = (uint8)(mouse_pending_y);
    SEND_COPS_CODE(i);
    DEBUG_LOG(0, "IRA1: added mouse dx,dy above %02x, %02x", mouse_pending_x, mouse_pending_y);

    mouse_pending_x = 0;
    mouse_pending_y = 0;

    llast_ratx = last_ratx;
    llast_raty = last_raty;
    last_ratx = ratx;
    last_raty = raty;

    DEBUG_LOG(0, "Sending mouse 00, x,y to follow.")
    return 0x00; // this one is 1st, then dx, then dy to indicate mouse motion data. ie: 00,dx,dy
  }

  data = copsqueue[0];
  DEBUG_LOG(0, "SRC: IRA1: COPS: read from regnum: %d returning %08x, %d bytes in cops queue left", regnum, data, copsqueuelen);

  for (i = 0; i < copsqueuelen; i++)
    copsqueue[i] = copsqueue[i + 1];
  copsqueuelen--;
  copsqueue[i] = 0x00;
  //    ALERT_LOG(0,"returning %02x, %d bytes left.  %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,",data,copsqueuelen,
  //                copsqueue[0],copsqueue[1],copsqueue[2],copsqueue[3],
  //				copsqueue[4],copsqueue[5],copsqueue[6],copsqueue[7]);
  //    ALERT_LOG(0,".");
  UNUSED(regnum);
  return data;
}

void add_mouse_event(int16 x, int16 y, int8 button)
{
  if (mousequeuelen + 1 > MAXMOUSEQUEUE)
  {
    ALERT_LOG(0, "overflowed mouse queue!");
    mousequeuelen = 0;
  } // we overflowed, just dump the old ones 20060609 17:15

  if (!cops_mouse || contrast == 0xff)
    return;

  if (x < 0 || x > lisa_vid_size_x || y < 0 || y > lisa_vid_size_y)
    return;

  // if it's not a click, update the current one in the queue - if there is one.
  if (!button && mousequeuelen && !mousequeue[mousequeuelen].button)
  {
    mousequeue[mousequeuelen].x = x;
    mousequeue[mousequeuelen].y = y;
    return;
  }

  // Otherwise add the event to the current one.
  mousequeuelen++;
  mousequeue[mousequeuelen].x = x;
  mousequeue[mousequeuelen].y = y;
  mousequeue[mousequeuelen].button = button;

  //    if (button ==-1 ) ALERT_LOG(0,"button released added to queue");
  //    if (button == 1 ) ALERT_LOG(0,"button down added to queue");

  //
  //     mousequeuelen++;
  //      mousequeue[mousequeuelen].x=x;
  //      mousequeue[mousequeuelen].y=y;
  //      mousequeue[mousequeuelen].button=button;
  //    }
}

void seek_mouse_event(void)
{
  uint16 ratx, raty;
  int16 dx, dy, i;
  int xabort_opcode = abort_opcode;

  if (floppy_6504_wait > 0 && floppy_6504_wait < 128)
    floppy_6504_wait--;

  if (mouse_seek_count)
  {
    mouse_seek_count--;
    return;
  }

  mouse_pending = 0;

  if (!cops_mouse || contrast == 0xff || !mousequeuelen)
  {
    DEBUG_LOG(0, "skipping seek_mouse_event because:%d,%d,%d\n", cops_mouse, contrast, mousequeuelen);
    return;
  }
  if (!is_lisa_mouse_on())
  {
    DEBUG_LOG(0, "skipping because lisa isn't listening to the mouse");
    return;
  }

  abort_opcode = 2;
  GET_RAT_XY(0);
  if (abort_opcode == 1)
  {
    DEBUG_LOG(0, "Got abort opcode while reading the mouse - this should never happen");
#ifdef DEBUG
    debug_on("abort_opcode_reading_mouse");
#endif
    DEBUG_LOG(0, "Got abort opcode while reading the mouse");
    // exit(36);
  }

  dx = mousequeue[1].x - (int16)(ratx);
  dy = mousequeue[1].y - (int16)(raty);
  DEBUG_LOG(0, "ratx,y: %d,%d mousequeue:%d,%d dx,dy", ratx, raty, mousequeue[1].x, mousequeue[1].y, dx, dy);

  abort_opcode = xabort_opcode;
  if (ratx > lisa_vid_size_x || raty > lisa_vid_size_y)
  {
    DEBUG_LOG(0, "ratx,raty out of range:%d,%d", ratx, raty);
    return;
  }

  for (i = 0; anti_jitter_sample_dec1[i] && ((abs(dx) + abs(dy)) < anti_jitter_sample_dec1[i] && !mouse_seek_count); i++)
    mouse_seek_count = anti_jitter_sample_dec2[i];

  // 2020.09.14 I suspect this would help us here:
  // if our motion attempts have failed and mouse button is set, release it or click it already.
  // this means that acceleration is preventing us from moving close enough to the target pixel, so just give up.
  if ((ratx == last_ratx) && (last_ratx == llast_ratx) && (raty == last_raty) && (last_raty == llast_raty) && (!!mousequeue[1].button))
  {
    dx = 0;
    dy = 0;
  }

  // do we need to move some more?  If so move, otherwise see if there has been a click, and send that.
  if ((dx | dy))
  {

    for (i = 0; anti_jitter_decellerate_xt[i]; i++)
      if (abs(dx) < anti_jitter_decellerate_xt[i])
      {
        dx = dx >> anti_jitter_decellerate_xn[i];
        break;
      }

    for (i = 0; anti_jitter_decellerate_yt[i]; i++)
      if (abs(dy) < anti_jitter_decellerate_yt[i])
      {
        dy = dy >> anti_jitter_decellerate_yn[i];
        break;
      }

    bigm_delta(dx, dy);
    DEBUG_LOG(0, "mouse dx,dy is (%d,%d) non zero, won't send it yet - still moving.", dx, dy);
  }

  if (!(mouse_pending_x | mouse_pending_y) && mousequeuelen == 1 && copsqueuelen == 0)
    mousequeuelen = 0; // 20200131 as reported by Tom Stepleton
  // DEBUG_LOG(0,"mouse dx,dy is (%d,%d) button is %d so we might send it.",dx,dy,mousequeue[1].button);

  switch (mousequeue[1].button)
  {
  case -1:
    if (last_mouse_button_state != 0)
    {
      set_mouse_button(0);
      DEBUG_LOG(0, "mouse button set to 0 (up)"); // mouse is now up
      last_mouse_button_state = 0;
    }
    break;
  case +1:
    if (last_mouse_button_state != 1)
    {
      set_mouse_button(1);
      DEBUG_LOG(0, "mouse button set 1 (down)"); // mouse is now down
      last_mouse_button_state = 1;
    }

  case 0:
    break;
  default:
    EXIT(352, 0, "Bug in mousequeue 1 button is %d", mousequeue[1].button);
  }

  // shift the mouse queue over
  if (mousequeuelen > 1)
  {
    for (i = 0; i < mousequeuelen; i++)
    {
      mousequeue[i].x = mousequeue[i + 1].x;
      mousequeue[i].y = mousequeue[i + 1].y;
      mousequeue[i].button = mousequeue[i + 1].button;
    }

    mousequeuelen--;
  } // 2006.07.12 moved this inside of the if statement.
}

void set_loram_clk(void)
{
  // $1BA-1BF : Clock setting (Ey,dd,dh,hm,ms,st)
  lisa_ram_safe_setbyte(1, 0x1ba, (0xe0) | (lisa_clock.year & 0x0f));
  lisa_ram_safe_setbyte(1, 0x1bb, (((lisa_clock.days_h) & 0xf0)) | (lisa_clock.days_h & 0x0f));
  lisa_ram_safe_setbyte(1, 0x1bc, (((lisa_clock.days_l) & 0x0f) << 4) | (lisa_clock.hours_h & 0x0f));
  lisa_ram_safe_setbyte(1, 0x1bd, (((lisa_clock.hours_l) & 0x0f) << 4) | (lisa_clock.mins_h & 0x0f));
  lisa_ram_safe_setbyte(1, 0x1be, (((lisa_clock.mins_l) & 0x0f) << 4) | (lisa_clock.secs_h & 0x0f));
  lisa_ram_safe_setbyte(1, 0x1bf, (((lisa_clock.secs_l) & 0x0f) << 4) | (lisa_clock.tenths & 0x0f));
}

// "Devastation is on the way" - In memory of Dimebag Darrell 2004.12.08
