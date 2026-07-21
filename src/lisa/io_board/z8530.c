/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2020 Ray A. Arachelian                          *
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
*                          Z8530 SCC Functions for                                     *
*                            Lisa serial ports                                         *
\**************************************************************************************/

/*--------------------------------------------------------------------------------------
 * only basic functionality exists here because this is a very complex chip.
 *
 * Only async serial is used, most things are virtualized and won't necessarily behave
 * as a real serial port.  Sync-serial, SDLC options, ESCC support, not enabled.
 * CRC function is for future use by SDLC, etc.
 *--------------------------------------------------------------------------------------
 * NOTES:
 *
 * WR2, WR9 are identical on both port A and port B.
 * port=0 is serial port B, port=1 is serial port A/
 *
 * RR8=receive buffer, RR0,1,10,15=status, 12,13=baud rate generator
 * RR3 is always 0 in CHB/port=0, so only valid in channel A/port=1
 *
 *
 *-------------------------------------------------------------------------------------*/

#define LISAEMSCCZ8530 1
#include <vars.h>
#include <z8530_structs.h>

#ifndef __MSVCRT__
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/poll.h>
#include <netinet/in.h>

#ifndef sun
#include <err.h>
#endif

#endif

#include <unistd.h>

sccfunc_t scc_fn[2];

static int scc_fifos_allocated = 0;

uint8 scc_bits_per_char_mask[2];
FLIFLO_QUEUE_t SCC_READ[16], SCC_WRITE[16]; // if changing this also change the extern in z8530-terminal.cpp!

int xoffflag[2];

int scc_interrupts_enabled = 0;

int irq_on_next_rx_char[2];

#define IMSK (reg68k_sr.sr_struct.i0 | (reg68k_sr.sr_struct.i1 << 1) | (reg68k_sr.sr_struct.i2 << 2))
static int sentbytes[2];
static XTIMER sentbytes_start[2];

// Defines for the above -- hmmm, not really using these...

#define CH_B_XMIT_BUFF_EMPTY 0
#define CH_B_EXT_STAT_CHANGE 1
#define CH_B_RECEIVE_CHAR_AVAILABLE 2
#define CH_B_SPECIAL_RECEIVE_COND 3

#define CH_A_XMIT_BUFF_EMPTY 4
#define CH_A_EXT_STAT_CHANGE 5
#define CH_A_RECEIVE_CHAR_AVAILABLE 6
#define CH_A_SPECIAL_RECEIVE_COND 7

#define RR3_IP_B_STAT 0x01 // Ext/status int pending, chan B
#define RR3_IP_B_TX 0x02   // Transmit int pending, chan B
#define RR3_IP_B_RX 0x04   // Receive int pending, chan B

#define RR3_IP_A_STAT 0x08 // Ditto for channel A
#define RR3_IP_A_TX 0x10
#define RR3_IP_A_RX 0x20

// 128 is an unused bit, it's used to signify that an IRQ has occured since the result
// is zero.

#define TX_BUFF_EMPTY(port)                                   \
  {                                                           \
    if (scc_w[port].s.wr1.r.txintenable)                      \
    {                                                         \
      if (port)                                               \
      {                                                       \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x38) | RR3_IP_B_TX; \
        z8530_last_irq_status_bits = 8;                       \
      }                                                       \
      else                                                    \
      {                                                       \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x07) | RR3_IP_A_TX; \
        z8530_last_irq_status_bits = 128;                     \
      }                                                       \
    }                                                         \
  }

#define EXT_STATUS_CHANGE(port)                                 \
  {                                                             \
    if (scc_w[port].s.wr1.r.extintenable)                       \
    {                                                           \
      if (port)                                                 \
      {                                                         \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x38) | RR3_IP_B_STAT; \
        z8530_last_irq_status_bits = 10;                        \
      }                                                         \
      else                                                      \
      {                                                         \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x07) | RR3_IP_A_STAT; \
        z8530_last_irq_status_bits = 2;                         \
      }                                                         \
    }                                                           \
  }

#define RX_CHAR_AVAILABLE(port)                                              \
  {                                                                          \
    if (scc_w[port].s.wr1.r.rxintmode && scc_w[port].s.wr1.r.rxintmode != 3) \
    {                                                                        \
      if (port)                                                              \
      {                                                                      \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x38) | RR3_IP_B_RX;                \
        z8530_last_irq_status_bits = 12;                                     \
      }                                                                      \
      else                                                                   \
      {                                                                      \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x07) | RR3_IP_A_RX;                \
        z8530_last_irq_status_bits = 4;                                      \
      }                                                                      \
    }                                                                        \
  }

#define RX_CHAR_NOT_AVAILABLE(port)                                          \
  {                                                                          \
    if (scc_w[port].s.wr1.r.rxintmode && scc_w[port].s.wr1.r.rxintmode != 3) \
    {                                                                        \
      if (port)                                                              \
      {                                                                      \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x38);                              \
        z8530_last_irq_status_bits = 12;                                     \
      }                                                                      \
      else                                                                   \
      {                                                                      \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x07);                              \
        z8530_last_irq_status_bits = 4;                                      \
      }                                                                      \
    }                                                                        \
  }

#define SPECIAL_RECV_COND(port)                                 \
  {                                                             \
    if (scc_w[port].s.wr1.r.rxintmode == 3)                     \
    {                                                           \
      if (port)                                                 \
      {                                                         \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0xf0) | RR3_IP_B_STAT; \
        z8530_last_irq_status_bits = 14;                        \
      }                                                         \
      else                                                      \
      {                                                         \
        scc_r[1].r[3] = (scc_r[1].r[3] & 0x0f) | RR3_IP_A_STAT; \
        z8530_last_irq_status_bits = 6;                         \
      }                                                         \
    }                                                           \
  }

// wrapper around fliflo_buff_has_data when software handshaking is enabled
#define HAS_DATA(port) ((xonenabled[port] && xoffflag[port]) ? 0 : fliflo_buff_has_data(&SCC_READ[port]))

// Protos for link to host devices - or emulation of ports

int get_scc_pending_irq(void);
void send_break(unsigned int port);
void set_dtr(unsigned int port, uint8 value);
void set_rts(unsigned int port, uint8 value);
void set_dtr_loopbackplug(unsigned int port, uint8 value);
void set_rts_loopbackplug(unsigned int port, uint8 value);
int get_dcd(unsigned int port);
int get_cts(unsigned int port);
int get_break(unsigned int port);
void signal_parity_error(unsigned int port);
void signal_crc_error(unsigned int port);
void set_even_parity(unsigned int port);
void set_odd_parity(unsigned int port);
void set_no_parity(unsigned int port);
void set_bits_per_char(unsigned int port, uint8 bitsperchar);
void set_stop_bits(unsigned int port, uint8 stopbits);
char read_serial_port(unsigned int port);
void write_serial_port(unsigned int port, char data);
void scc_hardware_reset_port(unsigned int port);
void scc_channel_reset_port(unsigned int port);
void initialize_scc(int actual);
void lisa_wb_Oxd200_sccz8530(uint32 address, uint8 data);
uint8 lisa_rb_Oxd200_sccz8530(uint32 address);
void scc_control_loop(void);
void dump_scc(void);

//char read_serial_port(unsigned int port); //(jamesdenton) This was declared twice (first in ~214)
char read_serial_port_nothing(unsigned int port);
char read_serial_port_imagewriter(unsigned int port);
char read_serial_port_loopbackplug(unsigned int port);
char read_serial_port_localport(unsigned int port);

void write_serial_port_nothing(unsigned int port, char data);
void write_serial_port_loopbackplug(unsigned int port, char data);
void write_serial_port_localport(unsigned int port, char data);
void write_serial_port_imagewriter(unsigned int port, char data);

uint32 get_baud_rate(unsigned int port);
void set_baud_rate(int port, uint32 baud);

//::TODO:: find a way to implement a windows version of telnetd and pty/tty
#ifndef __MSVCRT__

extern char read_serial_port_telnetd(unsigned int port);
extern void write_serial_port_telnetd(unsigned int port, char c);
extern int poll_telnet_serial_read(int portnum);
 
extern char read_serial_port_shell(unsigned int port);
extern void write_serial_port_shell(unsigned int port, char c);

extern char read_serial_port_pty(unsigned int port);
extern void write_serial_port_pty(unsigned int port, char c);
extern void set_dtr_pty(unsigned int port, uint8 value);

extern char read_serial_port_tty(unsigned int port);
extern void write_serial_port_tty(unsigned int port, char c);
extern void set_port_baud_tty(int port, uint32 baud);


#else
// temp disable for windows until we flesh these guys out
char read_serial_port_shell(unsigned int port) { return 0; }
char read_serial_port_pty(unsigned int port) { return 0; }
extern char read_serial_port_tty(unsigned int port);
int poll_telnet_serial_read(int portnum) { return 0; }
#endif

//void write_serial_port_terminal(int port, uint8 data); //(jamesdenton) type mismatch between uint8 and char; need to address this
void write_serial_port_terminal(unsigned int port, char data);
char read_serial_port_terminal(unsigned int port);

static inline void on_read_irq_handle(int port);

/*
 * There's a clever way to reverse all the bits in an 8-bit word
 * W = abcdefgh;
 * W = (W & 0xf0)>>4 | (W & 0x0f)<<4  = efghabcd
 * W = (W & 0xcc)>>2 | (W & 0x33)<<2  = ghefcdab
 * W = (W & 0xaa)>>1 | (W & 0x55)<<1  = hgfedcba
 */

static inline uint8 reversebit(uint8 c)
{
  c = (c & 0xf0) >> 4 | (c & 0x0f) << 4;
  c = (c & 0xcc) >> 2 | (c & 0x33) << 2;
  c = (c & 0xaa) >> 1 | (c & 0x55) << 1;
  return c;
}

static inline uint8 BITREVERSE(uint8 d)
{
#ifdef __POWERPC__
  return d;
#else
  return reversebit(d) & 0xff;
#endif
}

#ifdef BYTES_HIGHFIRST
#define BITORD(x) (BITREVERSE(x))
#else
#define BITORD(x) (x)
#endif

uint16 crc16(uint16 crc, uint8 data)
{
  crc = (SWAP16(crc)) ^ (BITREVERSE(data));
  crc ^= (crc & 0x00f0) >> 4;
  crc ^= (crc << 12);
  return crc ^ (((crc & 0x00ff) << 4) << 1);
}

int verifybaud(int baud)
{
  switch (baud)
  {
  case 50:
    break;
  case 75:
    break;
  case 110:
    break;
  case 134:
    break;
  case 150:
    break;
  case 200:
    break;
  case 300:
    break;
  case 600:
    break;
  case 1200:
    break;
  case 1800:
    break;
    //       case 2000:   break;
  case 2400:
    break;
    //       case 3600:   break;
  case 4800:
    break;
  case 9600:
    break;
  case 19200:
    break;
  case 38400:
    break;
  case 57600:
    break;
    //       case 76800:  break;
  case 115200:
    break;
  default:
    baud = 0;
  }
  return baud;
}

void scc_hardware_reset_port(unsigned int port)
{
  register uint8 d;

  d = BITORD(scc_r[port].r[0]);
  scc_r[port].r[0] = BITORD((d & (8 | 16 | 32 | 128)) | 64 | 4);
  d = BITORD(scc_r[port].r[1]);
  scc_r[port].r[1] = BITORD((d & 1) | 2 | 4);
  d = BITORD(scc_r[port].r[10]);
  scc_r[port].r[10] = BITORD(d & 64);

  scc_r[port].r[3] = 0; // no need to use bitord, 0 is 0 no matter how you flip it. :)
  scc_w[port].w[0] = 0;
  scc_w[port].w[10] = 0;

  d = BITORD(scc_w[port].w[1]);
  scc_w[port].w[1] = BITORD(d & (4 | 32));

  scc_w[port].w[11] = BITORD(16);
  scc_w[port].w[15] = BITORD(255 - 7);

  // wr2 is itself.
  // wr6 is itself
  // wr7 is itself
  // wr12 is itself
  // wr13 is itself

  d = BITORD(scc_w[port].w[3]);
  scc_w[port].w[3] = BITORD(d & 254);
  d = BITORD(scc_w[port].w[4]);
  scc_w[port].w[4] = BITORD(d | 4);
  d = BITORD(scc_w[port].w[5]);
  scc_w[port].w[5] = BITORD(d & (64 | 32 | 1));
  d = BITORD(scc_w[port].w[17]);
  scc_w[port].w[17] = BITORD(32);
  d = BITORD(scc_w[port].w[9]);
  scc_w[port].w[9] = BITORD((d & 3) | 128 | 64);

  d = BITORD(scc_w[port].w[14]);
  scc_w[port].w[14] = BITORD((d & (128 | 64)) | (32 | 16));

  irq_on_next_rx_char[port] = 0;

  scc_bits_per_char_mask[0] = 255;
  scc_bits_per_char_mask[1] = 255;
}

void scc_channel_reset_port(unsigned int port)
{
  register uint8 d;

  d = BITORD(scc_r[port].r[0]);
  scc_r[port].r[0] = BITORD((d & (8 | 16 | 32 | 128)) | 64 | 4);
  d = BITORD(scc_r[port].r[1]);
  scc_r[port].r[1] = BITORD((d & 1) | 2 | 4);
  d = BITORD(scc_r[port].r[10]);
  scc_r[port].r[10] = BITORD(d & 64);

  scc_r[port].r[3] = 0;
  scc_w[port].w[0] = 0;

  // wr2=wr2
  // wr6=wr6
  // wr7=wr7
  // wr12=wr12
  // wr13=wr13

  d = BITORD(scc_w[port].w[1]);
  scc_w[port].w[1] = BITORD(d & (4 | 32));
  d = BITORD(scc_w[port].w[3]);
  scc_w[port].w[3] = BITORD(d & 254);
  d = BITORD(scc_w[port].w[4]);
  scc_w[port].w[4] = BITORD(d | 4);
  d = BITORD(scc_w[port].w[5]);
  scc_w[port].w[5] = BITORD(d & (64 | 32 | 1));

  d = BITORD(scc_w[port].w[17]);
  scc_w[port].w[17] = BITORD(32);

  d = BITORD(scc_w[0].w[9]);
  scc_w[port].w[9] = BITORD(d & (255 - 32));
  d = BITORD(scc_w[port].w[10]);
  scc_w[port].w[10] = BITORD(d & (64 | 32));
  d = BITORD(scc_w[port].w[11]);
  scc_w[port].w[11] = BITORD(16);

  d = BITORD(scc_w[port].w[14]);
  scc_w[port].w[14] = BITORD((d & (128 | 64)) | (32 | 16));
  d = BITORD(scc_w[port].w[15]);
  scc_w[port].w[15] = BITORD(255 - 7);

  irq_on_next_rx_char[port] = 0;
  scc_bits_per_char_mask[0] = 255;
  scc_bits_per_char_mask[1] = 255;
}

extern char read_serial_port_terminal(unsigned int port);
//extern void write_serial_port_terminal(int port, uint8 data); //(jamesdenton) type mismatch between uint8 and char; need to address this
extern void write_serial_port_terminal(unsigned int port, char data);

void read_port_if_ready_nothing(unsigned int port)
{
  (void)(port);
  return;
}

void rx_char_available(int port) { RX_CHAR_AVAILABLE(port); }

void read_port_if_ready_shell(unsigned int port)
{
  int data = read_serial_port_shell(port);
  if (data > -1)
  {
    // ALERT_LOG(0,"---vvv---------------------------------------------------------------------------------------------------------------");
    // fliflo_dump(stderr,&SCC_READ[0],"BEFORE get from queue");
    fliflo_buff_add(&SCC_READ[port], (uint8)(data)&scc_bits_per_char_mask[port]);
    RX_CHAR_AVAILABLE(port);
    DEBUG_LOG(0, "Received 0x%02x (%d) %c from scc port:%d (mask:%02x) serial_b fliflo size is:%d",
              (uint8)(data), data,
              ((data > 31) ? ((uint8)(data)) : '.'),
              port, scc_bits_per_char_mask[0],
              fliflo_buff_size(&SCC_READ[0]));
    // fliflo_dump(stderr,&SCC_READ[0],"AFTER buff_add to queue");
    // ALERT_LOG(0,"----^^^-------------------------------------------------------------------------------------------------------------");
  }
}

void read_port_if_ready_pty(unsigned int port)
{
  int data = read_serial_port_pty(port);
  if (data > -1)
  {
    // ALERT_LOG(0,"---vvv---------------------------------------------------------------------------------------------------------------");
    // fliflo_dump(stderr,&SCC_READ[0],"BEFORE get from queue");
    fliflo_buff_add(&SCC_READ[port], (uint8)(data)&scc_bits_per_char_mask[port]);
    rx_char_available(port);
    ALERT_LOG(0, "Received %c (%d) on scc port:%d (mask:%02x) ; new fliflo size is:%d",
              data, data,
              port, scc_bits_per_char_mask[0],
              fliflo_buff_size(&SCC_READ[0]));
    // fliflo_dump(stderr,&SCC_READ[0],"AFTER buff_add to queue");
    // ALERT_LOG(0,"----^^^-------------------------------------------------------------------------------------------------------------");
  }
}

void read_port_if_ready_terminal(unsigned int port)
{
  if (fliflo_buff_has_data(&SCC_READ[port]))
    RX_CHAR_AVAILABLE(port);
}

void read_port_if_ready_tty(unsigned int port)
{
  int data = read_serial_port_tty(port);
  if (data > -1)
  {
    fliflo_buff_add(&SCC_READ[port], (uint8)(data)&scc_bits_per_char_mask[port]);
    RX_CHAR_AVAILABLE(port);
    ALERT_LOG(0, "Received 0x%02x (%d) %c from scc port:%d (mask:%02x) serial_b fliflo size is:%d",
              (uint8)(data), data,
              ((data > 31) ? ((uint8)(data)) : '.'),
              port, scc_bits_per_char_mask[0],
              fliflo_buff_size(&SCC_READ[0]));
    ALERT_LOG(0, "----^^^-------------------------------------------------------------------------------------------------------------");
  }
}

void read_port_if_ready_telnetd(unsigned int port)
{
  int data = poll_telnet_serial_read(port);
  if (data > -1)
  {
    fliflo_buff_add(&SCC_READ[port], (uint8)(data)&scc_bits_per_char_mask[port]);
    RX_CHAR_AVAILABLE(port);
    DEBUG_LOG(0, "Received %02x %c from scc port:%d (mask:%02x) serial fliflo size is:%d",
              (uint8)(data),
              ((data > 31) ? ((uint8)(data)) : '.'),
              port, scc_bits_per_char_mask[0],
              fliflo_buff_size(&SCC_READ[0]));
    // fliflo_dump(stderr,&SCC_READ[0],"added to queue");
    // ALERT_LOG(0,"-----------------------------------------------");
  }
}

// can't allow actual scc connection to a live device such as pty which might send output
// as that will cause BOOT ROM error 56 for the I/O card as input comes in, so we need to
// do it twice. (Not necessary for to ROMless boot though)
void initialize_scc(int actual)
{
  int i;

  if (!scc_fifos_allocated)
  {
    xoffflag[0] = 0;
    xoffflag[1] = 0;
    // xonenabled[0]=0; xonenabled[1]=0;

    DEBUG_LOG(0, "Allocating FIFO's");
    if (fliflo_buff_create(&SCC_READ[0], SCC_BUFFER_SIZE))
    {
      EXIT(404, 0, "Out of memory!");
    }
    if (fliflo_buff_create(&SCC_WRITE[0], SCC_BUFFER_SIZE))
    {
      EXIT(405, 0, "Out of memory!");
    }
    if (fliflo_buff_create(&SCC_READ[1], SCC_BUFFER_SIZE))
    {
      EXIT(406, 0, "Out of memory!");
    }
    if (fliflo_buff_create(&SCC_WRITE[1], SCC_BUFFER_SIZE))
    {
      EXIT(407, 0, "Out of memory!");
    }
    scc_fifos_allocated = 1;

    for (i = 0; i < 18; i++)
    {
      scc_w[0].w[i] = 0;
      scc_w[1].w[i] = 0;
      scc_r[0].r[i] = 0;
      scc_r[0].r[i] = 0;
    }

    sentbytes[0] = 0;
    sentbytes_start[0] = cpu68k_clocks;
    sentbytes[1] = 0;
    sentbytes_start[1] = cpu68k_clocks;

    // set handlers to default methods
    scc_fn[0].set_dtr = set_dtr;                         //   void (*set_dtr)(unsigned int port, uint8 value);
    scc_fn[0].send_break = send_break;                   //   void (*send_break)(unsigned int port);
    scc_fn[0].set_rts = set_rts;                         //   void (*set_rts)(unsigned int port, uint8 value);
    scc_fn[0].get_dcd = get_dcd;                         //   int  (*get_dcd)(unsigned int port);
    scc_fn[0].get_cts = get_cts;                         //   int  (*get_cts)(unsigned int port);
    scc_fn[0].get_break = get_break;                     //   int  (*get_break)(unsigned int port);
    scc_fn[0].signal_parity_error = signal_parity_error; //   void (*signal_parity_error)(unsigned int port);
    scc_fn[0].signal_crc_error = signal_crc_error;       //   void (*signal_crc_error)(unsigned int port);
    scc_fn[0].set_even_parity = set_even_parity;         //   void (*set_even_parity)(unsigned int port);
    scc_fn[0].set_odd_parity = set_odd_parity;           //   void (*set_odd_parity)(unsigned int port);
    scc_fn[0].set_no_parity = set_no_parity;             //   void (*set_no_parity)(unsigned int port);
    scc_fn[0].set_bits_per_char = set_bits_per_char;     //   void (*set_bits_per_char)(unsigned int port, uint8 bitsperchar);
    scc_fn[0].set_stop_bits = set_stop_bits;             //   void (*set_stop_bits)(unsigned int port,uint8 stopbits);
    scc_fn[0].read_serial_port = read_serial_port;       //   char (*read_serial_port)(unsigned int port);
    scc_fn[0].read_port_if_ready = read_port_if_ready_nothing;
    scc_fn[0].write_serial_port = write_serial_port;             //   void (*write_serial_port)(unsigned int port, char data);
    scc_fn[0].scc_hardware_reset_port = scc_hardware_reset_port; //   void (*scc_hardware_reset_port)(unsigned int port);
    scc_fn[0].scc_channel_reset_port = scc_channel_reset_port;   //   void (*scc_channel_reset_port)(unsigned int port);
    scc_fn[0].set_baud_rate = set_baud_rate;

    scc_fn[1].send_break = send_break;
    scc_fn[1].set_dtr = set_dtr;
    scc_fn[1].set_rts = set_rts;
    scc_fn[1].get_dcd = get_dcd;
    scc_fn[1].get_cts = get_cts;
    scc_fn[1].get_break = get_break;
    scc_fn[1].signal_parity_error = signal_parity_error;
    scc_fn[1].signal_crc_error = signal_crc_error;
    scc_fn[1].set_even_parity = set_even_parity;
    scc_fn[1].set_odd_parity = set_odd_parity;
    scc_fn[1].set_no_parity = set_no_parity;
    scc_fn[1].set_bits_per_char = set_bits_per_char;
    scc_fn[1].set_stop_bits = set_stop_bits;
    scc_fn[1].read_serial_port = read_serial_port;
    scc_fn[1].read_port_if_ready = read_port_if_ready_nothing;
    scc_fn[1].write_serial_port = write_serial_port;
    scc_fn[1].scc_hardware_reset_port = scc_hardware_reset_port;
    scc_fn[1].scc_channel_reset_port = scc_channel_reset_port;
    scc_fn[1].set_baud_rate = set_baud_rate;

    scc_fn[0].scc_hardware_reset_port(0);
    scc_fn[1].scc_hardware_reset_port(1);

    irq_on_next_rx_char[0] = 0;
    irq_on_next_rx_char[1] = 0;
    scc_interrupts_enabled = 0;
    scc_bits_per_char_mask[0] = 255;
    scc_bits_per_char_mask[1] = 255;
  }

  int port, portkind[2];
  portkind[1] = actual ? serial_a : SCC_NOTHING;
  portkind[0] = actual ? serial_b : SCC_NOTHING;

  ALERT_LOG(0, "actual: %d", actual);
  ALERT_LOG(0, "serial a: %d", portkind[1]);
  ALERT_LOG(0, "serial b: %d", portkind[0]);

  for (port = 0; port < 2; port++)
    switch (portkind[port])
    {
    case SCC_LOCALPORT:
      scc_fn[port].read_serial_port = read_serial_port_localport;
      scc_fn[port].write_serial_port = write_serial_port_localport;
      break;
    case SCC_IMAGEWRITER_PS:
    case SCC_IMAGEWRITER_PCL:
    case SCC_IMAGEWRITER:
      scc_fn[port].read_serial_port = read_serial_port_imagewriter;
      scc_fn[port].write_serial_port = write_serial_port_imagewriter;
      break;
    case SCC_LOOPBACKPLUG:
      scc_fn[port].read_serial_port = read_serial_port_loopbackplug;
      scc_fn[port].write_serial_port = write_serial_port_loopbackplug;
      scc_fn[port].set_dtr = set_dtr_loopbackplug;
      scc_fn[port].set_rts = set_rts_loopbackplug;
      break;
#ifndef __MSVCRT__
    case SCC_TELNETD:
      scc_fn[port].read_serial_port = read_serial_port_telnetd;
      scc_fn[port].write_serial_port = write_serial_port_telnetd;
      scc_fn[port].read_port_if_ready = read_port_if_ready_telnetd;
      break;

    case SCC_SHELL:
      scc_fn[port].read_serial_port = read_serial_port_shell;
      scc_fn[port].write_serial_port = write_serial_port_shell;
      scc_fn[port].read_port_if_ready = read_port_if_ready_shell;
      break;

    case SCC_PTY:
      scc_fn[port].read_serial_port = read_serial_port_pty;
      scc_fn[port].write_serial_port = write_serial_port_pty;
      scc_fn[port].read_port_if_ready = read_port_if_ready_pty;
      scc_fn[port].set_dtr = set_dtr_pty;
      break;

    case SCC_TTY:
      scc_fn[port].read_serial_port = read_serial_port_tty;
      scc_fn[port].write_serial_port = write_serial_port_tty;
      scc_fn[port].read_port_if_ready = read_port_if_ready_tty;
      scc_fn[port].set_baud_rate = set_port_baud_tty;

      break;
    case SCC_TERMINAL:
      scc_fn[port].read_serial_port = read_serial_port_terminal;
      scc_fn[port].write_serial_port = write_serial_port_terminal;
      scc_fn[port].read_port_if_ready = read_port_if_ready_terminal;
      break;

#else
      // fallthrough to default for unsupported on windows
#endif
    default: // fallthrough, includes SCC_NOTHING.
      // ALERT_LOG(0,"Not sure what to connect to this port");
      scc_fn[port].read_serial_port = read_serial_port_nothing;
      scc_fn[port].write_serial_port = write_serial_port_nothing;
      scc_fn[port].read_port_if_ready = read_port_if_ready_nothing;
      break;
    }

  DEBUG_LOG(0, "r %p %p w %p %p", SCC_READ[0].buffer, &SCC_READ[1].buffer, SCC_WRITE[0].buffer, SCC_WRITE[1].buffer);
}

void scc_control_loop(void)
{
  // If IRQ's are pending, this must be set before the IRQ gets issued.
  // after the IRQ is issued, they must be cleared?
  // if ( !port) scc_r[port].r[3]=0;
  // IRQ pending.  These must be set by loop IRQ routine!
  scc_running = 1;
  if (HAS_DATA(0) || HAS_DATA(1))
    scc_running = 2;
  if (fliflo_buff_is_full(&SCC_READ[0]) || fliflo_buff_is_full(&SCC_READ[1]))
    scc_running = 3;

  DEBUG_LOG(0, "r %p %p w %p %p", SCC_READ[0].buffer, &SCC_READ[1].buffer, SCC_WRITE[0].buffer, SCC_WRITE[1].buffer);
  // need to insert code in here that allows other fn's to handle their respective devices - i.e. read from ports, and fill buffers, etc.
}

#ifdef DEBUG
void dump_scc(void)
{
  int i;

  if (!debug_log_enabled)
    return;
  if (!buglog)
    buglog = stdout;

  fprintf(buglog, "\n\nSRC: scc read b: ");
  for (i = 0; i < 16; i++)
    fprintf(buglog, "%d:%02x ", i, scc_r[0].r[i]);
  fprintf(buglog, "SCR: scc write b:");
  for (i = 0; i < 16; i++)
    fprintf(buglog, "%d:%02x ", i, scc_w[0].w[i]);
  fprintf(buglog, "\n");
  fprintf(buglog, "SRC: scc read a: ");
  for (i = 0; i < 16; i++)
    fprintf(buglog, "%d:%02x ", i, scc_r[1].r[i]);
  fprintf(buglog, "SRC: scc write a:");
  for (i = 0; i < 16; i++)
    fprintf(buglog, "%d:%02x ", i, scc_w[1].w[i]);
  fprintf(buglog, "\n");
  fprintf(buglog, "SRC: size of port b rx queue:%d, port a:%d\n", fliflo_buff_size(&SCC_READ[0]), fliflo_buff_size(&SCC_READ[1]));
  fprintf(buglog, "SRC: size of port b tx queue:%d, port a:%d\n", fliflo_buff_size(&SCC_WRITE[0]), fliflo_buff_size(&SCC_WRITE[1]));
  fprintf(buglog, "SRC: port b rx data available:%d , port a:%d\n", scc_r[0].s.rr0.r.rx_char_available, scc_r[1].s.rr0.r.rx_char_available);
  fprintf(buglog, "SRC: internalloopback port b:%d port a:%d\n", scc_w[0].s.wr14.r.localloopback, scc_w[1].s.wr14.r.localloopback);
  fprintf(buglog, "SRC: autoecho port b:%d port a:%d\n", scc_w[0].s.wr14.r.auto_echo, scc_w[1].s.wr14.r.auto_echo);

  DEBUG_LOG(0, "r %p %p w %p %p", SCC_READ[0].buffer, &SCC_READ[1].buffer, SCC_WRITE[0].buffer, SCC_WRITE[1].buffer);

  DEBUG_LOG(0, "RR0[%d] 0:rx_char_available:%d,  2:tx_buffer_empty:%d, 3:dcd:%d, 5:cts:%d, 7:break_abort:%d, 4:sync_hunt:%d, 6:tx_underrun_eom:%d, 1:zero_count:%d xonenabled:%d xoffflag:%d fliflo_hasdata:%d",
            0,
            scc_r[0].s.rr0.r.rx_char_available,
            scc_r[0].s.rr0.r.tx_buffer_empty,
            scc_r[0].s.rr0.r.dcd,
            scc_r[0].s.rr0.r.cts,
            scc_r[0].s.rr0.r.break_abort,
            scc_r[0].s.rr0.r.sync_hunt,
            scc_r[0].s.rr0.r.tx_underrun_eom,
            scc_r[0].s.rr0.r.zero_count,
            xonenabled[0], xoffflag[0], fliflo_buff_has_data(&SCC_READ[0]));

  DEBUG_LOG(0, "RR0[%d] 0:rx_char_available:%d,  2:tx_buffer_empty:%d, 3:dcd:%d, 5:cts:%d, 7:break_abort:%d, 4:sync_hunt:%d, 6:tx_underrun_eom:%d, 1:zero_count:%d  xonenabled:%d xoffflag:%d fliflo_hasdata:%d\n\n",
            1,
            scc_r[1].s.rr0.r.rx_char_available,
            scc_r[1].s.rr0.r.tx_buffer_empty,
            scc_r[1].s.rr0.r.dcd,
            scc_r[1].s.rr0.r.cts,
            scc_r[1].s.rr0.r.break_abort,
            scc_r[1].s.rr0.r.sync_hunt,
            scc_r[1].s.rr0.r.tx_underrun_eom,
            scc_r[1].s.rr0.r.zero_count,
            xonenabled[1], xoffflag[1], fliflo_buff_has_data(&SCC_READ[1]));
}
#else
void dump_scc(void) {}
#endif

// this calculates the baud rate, but it also sets it if the port connection supports the set_baud_rate method.
// so the name isn't what it seems.
uint32 get_baud_rate(unsigned int port)
{
  static uint32 baud = 0, lastbaud[2];
  uint32 tc = 0, Clock_Freq;

  // SERB port=0,  SERA Port=1

  // Clock_Freq=port ? 3686400:4000000;
  Clock_Freq = port ? 125000 : 115200;

  tc = scc_w[port].w[12] + ((scc_w[port].w[13] << 8));

  DEBUG_LOG(0, "Freq:%ld TimeConst:%d (%02x.%02x) for port:%d", Clock_Freq, tc, scc_w[port].w[13], scc_w[port].w[12], port);

  switch (scc_w[port].s.wr4.r.clockmultipliermode)
  // =INT(3686400/(2*(tc)*16+2))
  {
  case 0:
    baud = Clock_Freq / (tc * (1) + 2);
    DEBUG_LOG(0, "clock multiplier=1  baud-with-x1  %d baud withx1:%d", baud, Clock_Freq / (tc + 2));
    break;
  case 1:
    baud = Clock_Freq / (tc * (16) + 2);
    DEBUG_LOG(0, "clock multiplier=16 baud-with-x16 %d baud withx1:%d", baud, Clock_Freq / (tc + 2));
    break;
  case 2:
    baud = Clock_Freq / (tc * (32) + 2);
    DEBUG_LOG(0, "clock multiplier=32 baud-with-x32 %d baud withx1:%d", baud, Clock_Freq / (tc + 2));
    break;
  case 3:
    baud = Clock_Freq / (tc * (64) + 2);
    DEBUG_LOG(0, "clock multiplier=64 baud-with-x64 %d baud withx1:%d", baud, Clock_Freq / (tc + 2));
    break;
  } // Baud           =K/(2*(TC(port)+2))

  if (!scc_w[port].s.wr14.r.br_generator_enable || !scc_w[port].s.wr14.r.br_generator_source)
  {
    DEBUG_LOG(0, "BAUD RATE GENERATOR DISABLED! PORT:%d w[12]=%02x w[13]=%02x clock multiplier=%02x (wr[4]=%02x) detected:%d",
              port,
              scc_w[port].w[12],
              scc_w[port].w[13],
              scc_w[port].s.wr4.r.clockmultipliermode,
              scc_w[port].w[4],
              baud);

    return lastbaud[port];
  }

  // correct weird differences between the formula and the table
  if (baud > 57600 - 9999)
    baud = 57600;
  else if (baud > 38400 - 5000)
    baud = 38400;
  else if (baud > 19200 - 2000)
    baud = 19200;
  else if (baud > 9600 - 800)
    baud = 9600;
  else if (baud > 4800 - 500)
    baud = 4800;
  else if (baud > 2400 - 400)
    baud = 2400;
  else if (baud > 1200 - 200)
    baud = 1200;
  else if (baud > 300 - 50)
    baud = 300;
  else if (baud > 110 - 30)
    baud = 110;
  else if (baud > 50 - 10)
    baud = 50;
  DEBUG_LOG(0, "Normalized Baud:%d", baud);

  switch (baud) // make sure we don't set it to some oddball value
  {
  case 110: // not always supported
  case 300: // not always supported
  case 1200:
  case 2400:
  case 4800:
  case 9600:
  case 19200:
  case 34800:
  case 57600:
  case 224000: // only Macs with serial ports will handle this one

    DEBUG_LOG(0, "Baud Rate:%d", baud);

    if (baud != lastbaud[port]) // was there a baud rate change?
    {
      if (scc_fn[port].set_baud_rate) // is there a handler to change bps on the port?
        scc_fn[port].set_baud_rate(port, baud);
    } // yup, use it's method

    lastbaud[port] = baud;

    break;
  default:
    DEBUG_LOG(0, "Weird Baud Rate:%d requested by Lisa or buggy code in emulation", baud);
  }

  DEBUG_LOG(0, "BAUD RATE PORT:%d w[12]=%02x w[13]=%02x clock multiplier=%02x (wr[4]=%02x) detected:%d",
            port,
            scc_w[port].w[12],
            scc_w[port].w[13],
            scc_w[port].s.wr4.r.clockmultipliermode,
            scc_w[port].w[4],
            baud);

  // debug_on("scc snapshot");
  // dump_scc();
  // debug_off();

  return baud;
}

// as above, incase it's useful later - the logarithm (given a value, the bit # of the highest 1 bit )
//
// uint8 bitlog[64]={0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
//                   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5}

static inline uint8 z8530_find_highest_ius(uint8 value, int clear_value)
{
  // uint8 bit=0;                                   // this is much faster with a simple lookup
  // if      (value & BIT5) bit=BIT5;               // I'm leaving this dead code here as an explanation of
  // else if (value & BIT4) bit=BIT4;               // how the lookup highest_bit[] and inverse were created.
  // else if (value & BIT3) bit=BIT3;               //
  // else if (value & BIT2) bit=BIT2;               // swap'em around if your architecture is faster w/o memory lookups
  // else if (value & BIT1) bit=BIT1;               // i.e. comment out the last return, then uncomment this block
  // else if (value & BIT0) bit=BIT0;               //
  //                                                //
  // return (clear_value ? (value & (~bit)) : bit); //
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // is clear_value turned on?
  return (clear_value ? (value & (highest_bit_val_inv[value & 63])) : // yes - return value with the highest 1 bit cleared.
              highest_bit_val[value & 63]);                           // no  - just return highest 1 bit from value, don't clear it
}

static inline void on_read_irq_handle(int port)
{
  // if Master IRQ Enable is off, nothing to do
  if (!scc_w[0].s.wr9.r.MIE)
    return;

  switch (scc_w[port].s.wr1.r.rxintmode)
  {
  case 1:
    scc_w[port].s.wr1.r.rxintmode = 0; // clear mode, then
                                       /* FALLTHRU */
  case 2:
    if (!port)
      scc_r[1].s.rr3.r.ch_b_rx_irq_pending = 1; // RR3A
    else
      scc_r[1].s.rr3.r.ch_a_rx_irq_pending = 1; /* FALLTHRU */
  default:
    return; // case 0,3 - just return, not applicable
  }
}

static inline void on_special_irq_handle(int port)
{
  // if Master IRQ Enable is off, or the mode is off, skip the rest
  if (!scc_w[port].s.wr9.r.MIE)
    return;

  switch (scc_w[port].s.wr1.r.rxintmode)
  {
  case 1:
    scc_w[port].s.wr1.r.rxintmode = 0; /* FALLTHROUGH */
  case 2:                              /* FALLTHROUGH */
  case 3:
    if (!port)
      scc_r[1].s.rr3.r.ch_b_ext_status_irq_pending = 1; // set IRQ for the port
    else
      scc_r[1].s.rr3.r.ch_a_ext_status_irq_pending = 1;
  }

  scc_r[1].s.rr0.r.rx_char_available = HAS_DATA(1);
  scc_r[0].s.rr0.r.rx_char_available = HAS_DATA(0);
}

extern void connect_serial_devices(void);

static int lastpc24test = -1;
static inline void avoid_rom_scc_tests(void)
{
  int test = ((pc24 & 0x00ff0000) == 0x00fe0000) && context == 1;

  scc_r[1].s.rr0.r.rx_char_available = HAS_DATA(1);
  scc_r[0].s.rr0.r.rx_char_available = HAS_DATA(0);

  if (test != lastpc24test)
    ALERT_LOG(0, "test:%d last:%d", test, lastpc24test);

  if (lastpc24test != test)
  {
    if ((pc24 & 0x00ff0000) == 0x00fe0000)
    {
      ALERT_LOG(0, "disconnecting for ROM test");
      initialize_scc(0);
    } // reconnect fn's to nothing
    else
    {
      ALERT_LOG(0, "Connecting to backend port");
      connect_serial_devices();
      initialize_scc(1);
    } // reconnect to actual ports
  }

  lastpc24test = test;
}

// want to set scc_r[port].s.rr0.r.rx_char_available=HAS_DATA, scc_r[port].s.rr0.r.tx_buffer_empty=1, scc_r[port].s.rr0.r.dcd=1, scc_r[port].s.rr0.r.cts=1

int get_scc_pending_irq(void)
{
  int data;

  if (!scc_w[0].s.wr9.r.MIE)
  {
    DEBUG_LOG(0, "MIE is off");
    return 0;
  } // Master IRQ Enable bit - if not set, no IRQ's to return.
  // prevent sending too many bytes in one shot for LisaTerminal
  if (running_lisa_os == LISA_OFFICE_RUNNING)
  {
    // LisaTerminal or perhaps LOS drops bytes if more than 64 are sent in a single chunk. Keep track so we can slow it down.
    // another issue is that any ANSI/vt100 seq it receives that it doesn't recognize causes it to abort all output until the next
    // line or something like that.
    // if (IMSK!=6) {sentbytes[0]=0; sentbytes[1]=0; sentbytes_start[0]=cpu68k_clocks; sentbytes_start[1]=cpu68k_clocks;}

#define CHARSLIM 16
#define CLKCYLIM 9500

    if (sentbytes[0] > CHARSLIM || sentbytes[1] > CHARSLIM)
    { // 12,15000 from seq 100 200 we get upto 111 + 2 bytes before it cuts off so 43 bytes (with nl's)
      // if we still lose bytes, change this threshhold of 1500 , or lower number of bytes
      if (cpu68k_clocks - sentbytes_start[0] < CLKCYLIM || cpu68k_clocks - sentbytes_start[1] < CLKCYLIM)
      {
        ALERT_LOG(0, "\n\nlimiting scc input");
        return 0;
      }
      ALERT_LOG(0, "\n\nresuming scc input");
      sentbytes[0] = 0;
      sentbytes[1] = 0;
    }
  }
  else
  {
    sentbytes[0] = 0;
    sentbytes[1] = 0;
  }

  if (sentbytes[0] < CHARSLIM)
  {
    scc_fn[0].read_port_if_ready(0);
  }
  if (sentbytes[1] < CHARSLIM)
  {
    scc_fn[1].read_port_if_ready(1);
  }

  // if IRQ on next char is enabled and the fliflo has data, flag IRQ
  if ((irq_on_next_rx_char[0] && HAS_DATA(0) && sentbytes[0] < CHARSLIM) ||
      ((scc_w[0].s.wr1.r.rxintmode) && HAS_DATA(0)))
  {
    DEBUG_LOG(0, "chb has data");
    on_read_irq_handle(0);
    scc_r[1].s.rr3.r.ch_b_ext_status_irq_pending = 1;
    RX_CHAR_AVAILABLE(0);
  }
  else
    scc_r[1].s.rr3.r.ch_b_ext_status_irq_pending = 0; // 20051111

  if ((irq_on_next_rx_char[1] && HAS_DATA(1) && sentbytes[1] < CHARSLIM) ||
      ((scc_w[1].s.wr1.r.rxintmode) && HAS_DATA(1)))
  {
    DEBUG_LOG(0, "cha has data");
    on_read_irq_handle(1);
    scc_r[1].s.rr3.r.ch_a_ext_status_irq_pending = 1;
    RX_CHAR_AVAILABLE(1);
  }
  else
    scc_r[1].s.rr3.r.ch_a_ext_status_irq_pending = 0; // 20051111

  // if there's no more data, don't trigger an IRQ for it.
  if (!HAS_DATA(0) && z8530_last_irq_status_bits == 4)
  {
    z8530_last_irq_status_bits = 0;
    scc_r[1].s.rr3.r.ch_b_rx_irq_pending = 0;
  }
  if (!HAS_DATA(1) && z8530_last_irq_status_bits == 64)
  {
    z8530_last_irq_status_bits = 0;
    scc_r[1].s.rr3.r.ch_a_rx_irq_pending = 0;
  }

  DEBUG_LOG(0, "Returning: %d or %d", (scc_r[1].r[3]), z8530_last_irq_status_bits);

  return (scc_r[1].r[3]) || z8530_last_irq_status_bits; // if zero, no irq, else irq
}

void lisa_wb_Oxd200_sccz8530(uint32 address, uint8 data)
{
  uint8 access, port = 0, odata, regnum = 0; // d,

  avoid_rom_scc_tests();

  DEBUG_LOG(0, "write %02x to %08x", data, address);
  switch ((address & 0x000007) | 0x00FCD240)
  {
  case SERIAL_PORT_B_CONTROL:
    access = 0;
    port = 0;
    DEBUG_LOG(0, "SCC PORT B CONTROL");
    break;
  case SERIAL_PORT_B_DATA:
    access = 1;
    port = 0;
    DEBUG_LOG(0, "SCC PORT B DATA");
    break;
  case SERIAL_PORT_A_CONTROL:
    access = 0;
    port = 1;
    DEBUG_LOG(0, "SCC PORT A CONTROL");
    break;
  case SERIAL_PORT_A_DATA:
    access = 1;
    port = 1;
    DEBUG_LOG(0, "SCC PORT A DATA");
    break;
  default:
    ALERT_LOG(0, "Warning invalid access! %08x", address);
    return; // invalid address, just ignore it.
  }

#ifdef DEBUG
  char *multiplier;
  DEBUG_LOG(0, "---------------------------------------------------------------------------------------------------------------------");
  dump_scc();

  switch (scc_w[port].s.wr4.r.clockmultipliermode)
  {
  case 0:
    multiplier = "1";
    break;
  case 1:
    multiplier = "16";
    break;
  case 2:
    multiplier = "32";
    break;
  case 3:
    multiplier = "64";
    break;
  default:
    multiplier = "unknown";
  }
  DEBUG_LOG(0, "TC:%d, Baud:%ld %d bps %s parity Clock Multiplier:%s", (scc_w[port].w[12] + (scc_w[port].w[13] << 8)), get_baud_rate(port),
            (5 + scc_w[port].s.wr3.r.rxbitsperchar),
            ((scc_w[port].s.wr4.r.parityenable) ? ((scc_w[port].s.wr4.r.evenparity) ? "Even" : "Odd") : "No"),
            multiplier);
  DEBUG_LOG(0, "---------------------------------------------------------------------------------------------------------------------");
#endif

  if (access) // Write to data port.
  {
    if (scc_w[0].s.wr9.r.soft_int_ack)
    {
      scc_r[0].r[2] = 0;
      z8530_last_irq_status_bits = 0;
      scc_r[port].r[2] = 0;
    }
    DEBUG_LOG(0, "SRC:SCC: access:%d port %d", access, port);
    if (!scc_w[port].s.wr5.r.txenable)
    {
      DEBUG_LOG(0, "TX not enabled!");
      return;
    } // can't send until tx is enabled.

    if (scc_w[port].s.wr14.r.localloopback)
    {
      DEBUG_LOG(0, "loopback data: %02x", data & scc_bits_per_char_mask[port]);
      DEBUG_LOG(0, "SCC_READ[port] is %p", &SCC_READ[port]);
      DEBUG_LOG(0, "scc_bits_char_mask is:%02x", scc_bits_per_char_mask[port]);
      // if loopback is true, add the data to port's own receive buffer instead.
      fliflo_buff_add(&SCC_READ[port], data & scc_bits_per_char_mask[port]);
    }
    else // real writes - with option auto_echo
    {
      TX_BUFF_EMPTY(port); // pretend infinite speed output, well at least don't tell the Lisa that output buffer is full.

      if (z8530_event == -1)
        z8530_event = cpu68k_clocks + Z8530_XMIT_DELAY;

      // if we received XON/XOFF, flag it for future use, but fall through to send the handshake char to the other end of the prot as well
      if (xonenabled[port] && data == 19)
      {
        ALERT_LOG(0, "XOFF ^S  received port %d (pause) \n", port);
        xoffflag[port] = 1;
        TX_BUFF_EMPTY(port);
      }
      if (xonenabled[port] && data == 17)
      {
        ALERT_LOG(0, "XON  ^Q  received port %d (resume)\n", port);
        xoffflag[port] = 0;
        if (HAS_DATA(port))
          RX_CHAR_AVAILABLE(port);
      }

      if (scc_w[port].s.wr14.r.auto_echo)
      {
        DEBUG_LOG(0, "autoecho is on");
        fliflo_buff_add(&SCC_READ[port], data & scc_bits_per_char_mask[port]);
      } // copy to receive buffer too, as in loopback

      // if there's an attach function/method, call it.
      if (scc_fn[port].write_serial_port)
      {
        ALERT_LOG(0, "write to write_serial_port_method byte:0x%02x (%d) XON-enabled?:%d xoff:%d", data, data, xonenabled[port], xoffflag[port]);
        scc_fn[port].write_serial_port(port, data);
        TX_BUFF_EMPTY(port);

#ifdef DEBUG
        if (scc_fn[port].write_serial_port == write_serial_port_shell)
          ALERT_LOG(0, "sent to shell");
        if (scc_fn[port].write_serial_port == write_serial_port_pty)
          ALERT_LOG(0, "sent to pty");
        if (scc_fn[port].write_serial_port == write_serial_port_nothing)
          ALERT_LOG(0, "sent to NOTHING!");
#endif
      }
      else
      {
        ALERT_LOG(0, "write method is null, calling generic handler to write %02x", data);
        write_serial_port(port, data);
        TX_BUFF_EMPTY(port);
      }
    }

#ifdef DEBUG
    DEBUG_LOG(0, "SRC:SCC Write to port %d data %02x adjusted for bits/char:%02x", port, data, data & scc_bits_per_char_mask[port]);
    dump_scc();
#endif

    return;
  }

  odata = data;
  data = BITORD(data); // do this here, so the bit fields will work properly.  If done before dealing with data
                       // it would reverse the data, which would be a bad idea. :)

  regnum = scc_w[port].s.wr0.r.reg;
  if (scc_w[port].s.wr0.r.cmd == 1)
  {
    regnum |= 8;
    scc_w[port].s.wr0.r.cmd = 0;
  }
  DEBUG_LOG(0, "Register for SCC access is %02x - register 0 is: %02x, writing %02x to reg %d", regnum, scc_w[port].w[0],
            odata, regnum);

  scc_w[port].s.wr0.r.reg = 0; // reset register pointer back to zero for next round.

  switch (regnum)
  {
  case 0:
    DEBUG_LOG(0, "Write to 0 odata=%02x data=%02x", odata, data);
    scc_w[port].w[0] = data;
    // scc_w[port].w[regnum]=data & (0xff-7);

    if (scc_w[port].s.wr0.r.reset == 1)
    {
      scc_r[port].s.rr1.r.crc_framing_error = 0;
      DEBUG_LOG(0, "reset rx crc checker");
    }
    else if (scc_w[port].s.wr0.r.reset == 2)
    {
      scc_r[port].s.rr1.r.crc_framing_error = 0;
      DEBUG_LOG(0, "reset tx crc checker");
    }
    else if (scc_w[port].s.wr0.r.reset == 3)
    {
      scc_r[port].s.rr0.r.tx_underrun_eom = 0;
      DEBUG_LOG(0, "reset xmit_underrun_eom");
    }

    switch (scc_w[port].s.wr0.r.cmd)
    {

    case 0:
      break; // NULL Command

    case 2:
      DEBUG_LOG(0, "WR0:CMD=2  Reset IRQ status flags %02x", data); // cmd=2
      scc_r[0].s.rr2.v = 0;                                         // was data ??
      // scc_w[0].s.wr2.v=data;
      break; // reset irq status flags
    case 3:
      DEBUG_LOG(0, "WR0:CMD=send sdlc abort");
      // send_sdlc_abort(port);                                   // send sdlc abort
      scc_r[port].s.rr0.r.tx_underrun_eom = 1;
      scc_w[port].s.wr0.r.cmd = 0; // clear command for next cycle (except for previous highpoint)
      break;
    case 4:
      DEBUG_LOG(0, "WR0:irq on next rx char");
      scc_w[port].s.wr1.r.rxintmode = 1; // maybe this is all I'd need for this?

      irq_on_next_rx_char[port] = 1; // might not need this variable anymore.

      scc_w[port].s.wr0.r.cmd = 0; // clear command for next cycle (except for previous highpoint)
      break;
    case 5:
      DEBUG_LOG(0, "WR0:Clear next xmit pending irq");
      scc_r[port].s.rr3.r.ch_a_tx_irq_pending = 0;
      scc_r[!port].s.rr3.r.ch_a_tx_irq_pending = 0;
      scc_w[port].s.wr0.r.cmd = 0; // clear command for next cycle (except for previous highpoint)

      // 2006.08.07
      if ((port && z8530_last_irq_status_bits == 8) || (!port && z8530_last_irq_status_bits == 128))
        z8530_last_irq_status_bits = 0;
      break;
    case 6:
      DEBUG_LOG(0, "WR0:Reset Error bits");
      scc_r[port].s.rr1.r.all_sent = 0; // reset error bits
      scc_r[port].s.rr1.r.parity_error = 0;
      scc_r[port].s.rr1.r.rx_overrun_error = 0;
      scc_r[port].s.rr1.r.crc_framing_error = 0;
      scc_r[port].s.rr1.r.sdlc_end_of_frame = 0;
      scc_w[port].s.wr0.r.cmd = 0; // clear command for next cycle (except for previous highpoint)
      break;

    case 7:

      DEBUG_LOG(0, "WR0:Reset highest irq under service"); // reset highest Interrupt Under Service ????
      scc_r[1].r[3] = 0;                                   // z8530_find_highest_ius(scc_r[1].r[3], 1) & 63;         // the 2nd param tells  the fn to clear it
      scc_r[0].r[2] = 0;
      z8530_last_irq_status_bits = 0;
      scc_w[port].s.wr0.r.cmd = 0; // clear command for next cycle (except for previous highpoint)
      break;
    }

    DEBUG_LOG(0, "SRC:SCC port:%d set regnum %02x to data/bitrevdata:%02x/%02x", port, regnum, odata, data);
    dump_scc();
    return;

  case 1:
    scc_w[port].w[1] = data;
    DEBUG_LOG(0, "Write to 1 odata=%02x data=%02x", odata, data);

    DEBUG_LOG(0, "wr1:irq: extintenable:%d, txintenable:%d, parityspecial:%d, rxintmode:%d, waitdmareqon:%d, waitdmareqfn:%d, waitdmareqenable:%d",
              scc_w[port].s.wr1.r.extintenable,
              scc_w[port].s.wr1.r.txintenable,
              scc_w[port].s.wr1.r.parityspecial,
              scc_w[port].s.wr1.r.rxintmode,
              scc_w[port].s.wr1.r.waitdmareqon,
              scc_w[port].s.wr1.r.waitdmareqfn,
              scc_w[port].s.wr1.r.waitdmareqenable);

    if (scc_w[port].s.wr1.v && scc_w[0].s.wr9.r.MIE)
      scc_interrupts_enabled = 1;
    else
      scc_interrupts_enabled = 0;
    DEBUG_LOG(0, "IRQ enabled is:%d", scc_interrupts_enabled);
    break;

  case 2:
    DEBUG_LOG(0, "Storing %02x (irq vector bits) in register w2 %02x=odata", data, odata); // there is only one wr2 so shadow it

    scc_w[0].w[2] = data; // interrupt vector bits  - there is only one port 2
    scc_w[1].w[2] = data; // what does this actually do? enable the IRQ's????????????? is it an AND mask?
    DEBUG_LOG(0, "IRQ vector bits:%d,%d,%d,%d,%d,%d,%d,%d",
              scc_w[0].s.wr2.r.v0, scc_w[0].s.wr2.r.v1, scc_w[0].s.wr2.r.v2, scc_w[0].s.wr2.r.v3,
              scc_w[0].s.wr2.r.v4, scc_w[0].s.wr2.r.v5, scc_w[0].s.wr2.r.v6, scc_w[0].s.wr2.r.v7);
    break;

  case 3:
    scc_w[port].w[3] = data;
    DEBUG_LOG(0, "Write to 3 data=%02x,odata=%02x", data, odata);

    set_bits_per_char(port, 5 + scc_w[port].s.wr3.r.rxbitsperchar);
    DEBUG_LOG(0, "RX Bits per char is %d for port %d", (5 + scc_w[port].s.wr3.r.rxbitsperchar), port);

    break;

  case 4:
    scc_w[port].w[4] = data;
    DEBUG_LOG(0, "Write to 4 data=%02x odata=%02x", data, odata);

    if (scc_w[port].s.wr4.r.parityenable)
    {
      DEBUG_LOG(0, "Parity Enabled");
      if (scc_w[port].s.wr4.r.evenparity)
      {
        DEBUG_LOG(0, "Even parity");
        set_even_parity(port);
      }
      else
      {
        DEBUG_LOG(0, "Odd parity");
        set_odd_parity(port);
      }
    }
    else
      set_no_parity(port);

    set_stop_bits(port, scc_w[port].s.wr4.r.stopbits);
    DEBUG_LOG(0, "port:%d Stop bits set to:%d", port, scc_w[port].s.wr4.r.stopbits);

#ifdef DEBUG
    switch (scc_w[port].s.wr4.r.clockmultipliermode)
    {
    case 0:
      DEBUG_LOG(0, "clock multiplier X1  mode:%d", scc_w[port].s.wr4.r.clockmultipliermode);
      break;
    case 1:
      DEBUG_LOG(0, "clock multiplier X16 mode:%d", scc_w[port].s.wr4.r.clockmultipliermode);
      break;
    case 2:
      DEBUG_LOG(0, "clock multiplier X32 mode:%d", scc_w[port].s.wr4.r.clockmultipliermode);
      break;
    case 3:
      DEBUG_LOG(0, "clock multiplier X64 mode:%d", scc_w[port].s.wr4.r.clockmultipliermode);
      break;
    }
#endif

    get_baud_rate(port);

    break;

  case 5:
    scc_w[port].w[5] = data;
    DEBUG_LOG(0, "Write to 5 data=%02x odata=%02x", data, odata);

    DEBUG_LOG(0, "RTS:%d, DTR:%d, break:%d", scc_w[port].s.wr5.r.RTS, scc_w[port].s.wr5.r.DTR, scc_w[port].s.wr5.r.sendbreak);
    if (scc_fn[port].set_rts)
      scc_fn[port].set_rts(port, scc_w[port].s.wr5.r.RTS);
    if (scc_fn[port].set_dtr)
      scc_fn[port].set_dtr(port, scc_w[port].s.wr5.r.DTR);
    if (scc_w[port].s.wr5.r.sendbreak)
    {
      if (scc_fn[port].send_break)
        scc_fn[port].send_break(port);
    }

    switch (scc_w[port].s.wr5.r.txbitsperchar)
    {
    case 0:
      set_bits_per_char(port, 5);
      break;
    case 1:
      set_bits_per_char(port, 7);
      break;
    case 2:
      set_bits_per_char(port, 6);
      break;
    case 3:
      set_bits_per_char(port, 8);
      break;
    }
    break;

  case 6:
    scc_w[port].w[6] = data;
    DEBUG_LOG(0, "Unsupported - Write to 6 syncbits data=%02x, %02x=odata", data, odata);
    break; // sync bits
  case 7:
    scc_w[port].w[7] = data;
    DEBUG_LOG(0, "Unsupported - Write to 7 data=%02x odata=%02x", data, odata);

#ifdef ESCC
    if (scc_w[port].s.wr15.r.wr7prime)
    { // 7PRIME
      scc_w[port].w.w7prime = data;
      // whatever needs to be added

      DEBUG_LOG(0, "auto_tx_flag:%d", scc_w[port].s.wr7prime.r.autotx);
      DEBUG_LOG(0, "auto_EOM_reset:%d", scc_w[port].s.wr7prime.r.autoeomreset);
      DEBUG_LOG(0, "auto_RTS_deactivation:%d", scc_w[port].s.wr7prime.r.autortsdeactivation);
      DEBUG_LOG(0, "rx_FIFO_half_full:%d", scc_w[port].s.wr7prime.r.rxfifohalffull);
      DEBUG_LOG(0, "DTRREQ_timing_mode:%d", scc_w[port].s.wr7prime.r.dtr_req_timing_mode);
      DEBUG_LOG(0, "tx_FIFO_Empty:%d", scc_w[port].s.wr7prime.r.txfifoempty);
      DEBUG_LOG(0, "ext_read_enable:%d", scc_w[port].s.wr7prime.r.extendedreadenable);

      dump_scc();
      return;
    }
#endif
    break;

  case 8:
    scc_w[port].w[8] = data;
    DEBUG_LOG(0, "Write to 8 (XMIT REGISTER DATA - Unsuppported!) data=%02x odata=%02x", data, odata);

    break;

  case 9:
    DEBUG_LOG(0, "Write to 9 MIE data=%02x odata=%02x", data, odata); // there is only one port 9
    scc_w[0].w[9] = scc_w[1].w[9] = data;
    if (data & 1)
    {
      scc_channel_reset_port(0);
      DEBUG_LOG(0, "resetting port 0 (B)");
    }
    if (data & 12)
    {
      scc_channel_reset_port(1);
      DEBUG_LOG(0, "resetting port 1 (A)");
    }

    // possible bug in the docs MIE goes to 0 on a hw reset, but looks like it should not!

    DEBUG_LOG(0, "wr9 MIE VIS:%d", scc_w[0].s.wr9.r.VIS);
    DEBUG_LOG(0, "wr9 MIE NV:%d", scc_w[0].s.wr9.r.NV);
    DEBUG_LOG(0, "wr9 MIE DLC:%d", scc_w[0].s.wr9.r.DLC);
    DEBUG_LOG(0, "wr9 MIE MIE:%d", scc_w[0].s.wr9.r.MIE);
    DEBUG_LOG(0, "wr9 MIE status_hi_lo:%d", scc_w[0].s.wr9.r.status_hi_lo);
    DEBUG_LOG(0, "wr9 MIE soft_int_ack:%d", scc_w[0].s.wr9.r.soft_int_ack);
    DEBUG_LOG(0, "wr9 MIE reset:%d", scc_w[0].s.wr9.r.reset);

    scc_interrupts_enabled = (scc_w[port].w[1] && scc_w[0].s.wr9.r.MIE);

    scc_w[0].w[9] = data;
    scc_w[1].w[9] = data;

    break;

  case 10:
    scc_w[port].w[10] = data;
    ALERT_LOG(0, "Unsupported Write to WR10[%d] data=%02x odata=%02x", port, data, odata);
    break;
    // d0= 6/8 bit sync, d1=loop mode, d2=abort/flag on underrun, d3=mark/flag idle, d4=go active on poll, d6-5=NRZ/NRZI/FM1/FM0, d7=CRC-preset io

  case 11:
    scc_w[port].w[11] = data;
    DEBUG_LOG(0, "Write to 11 data=%02x odata=%02x", data, odata);

    DEBUG_LOG(0, "reg11: CLOCK Control!  xmit_extern_control:%d,trxc_pin_is_output:%d, xmit_clock_src:%d, rx_clock_src:%d, rtxc_xtal:%d",
              scc_w[port].s.wr11.r.xmit_extern_control,
              scc_w[port].s.wr11.r.trxc_pin_is_output,
              scc_w[port].s.wr11.r.xmit_clock_src,
              scc_w[port].s.wr11.r.rx_clock_src,
              scc_w[port].s.wr11.r.rtxc_xtal);
    break;

  case 12:
    scc_w[port].w[12] = data;
    DEBUG_LOG(0, "Write to 12 TimeConstant Low data=%02x odata=%02x::TimeConst:%d: Baud Rate:%ld", data, odata,
              (scc_w[port].w[12] + (scc_w[port].w[13] << 8)), get_baud_rate(port));
    break;

  case 13:
    scc_w[port].w[13] = data;
    DEBUG_LOG(0, "Write to 13 TimeConstant Low data=%02x odata=%02x::TimeConst:%d: Baud Rate:%ld", data, odata,
              (scc_w[port].w[12] + (scc_w[port].w[13] << 8)), get_baud_rate(port));
    break;

  case 14:
    scc_w[port].w[14] = data; // write of 0x13 should be baud rate generator + loopback: 00010011

    DEBUG_LOG(0, "autoecho: %d", scc_w[port].s.wr14.r.auto_echo);
    DEBUG_LOG(0, "loopback: %d", scc_w[port].s.wr14.r.localloopback);

    DEBUG_LOG(0, "baud rate generator enable: %d", scc_w[port].s.wr14.r.br_generator_enable);
    DEBUG_LOG(0, "baud rate generator source %d", scc_w[port].s.wr14.r.br_generator_source);

    scc_r[port].s.rr0.r.zero_count = 1;

    DEBUG_LOG(0, "extintenable:%d", scc_w[port].s.wr1.r.extintenable);
    DEBUG_LOG(0, "zc_irq_en:%d", scc_w[port].s.wr15.r.zero_count_interrupt_enable);
    DEBUG_LOG(0, "scc_w[port].s.wr1.r.rxintmode:%d", scc_w[port].s.wr1.r.rxintmode);

    if (scc_w[port].s.wr14.r.br_generator_enable && scc_w[port].s.wr1.r.extintenable &&
        scc_w[port].s.wr15.r.zero_count_interrupt_enable)
      on_special_irq_handle(port);

    DEBUG_LOG(0, "chb irq ext pending:%d", scc_r[1].s.rr3.r.ch_b_ext_status_irq_pending);
    DEBUG_LOG(0, "cha irq ext pending:%d", scc_r[1].s.rr3.r.ch_a_ext_status_irq_pending);
    DEBUG_LOG(0, "cheat on");

    if (!port)
      scc_r[1].s.rr3.r.ch_b_ext_status_irq_pending = 1;
    else
      scc_r[1].s.rr3.r.ch_a_ext_status_irq_pending = 1;

    DEBUG_LOG(0, "dtr req: %d", scc_w[port].s.wr14.r.dtr_req_fn);
    DEBUG_LOG(0, "autoecho: %d", scc_w[port].s.wr14.r.auto_echo);
    DEBUG_LOG(0, "loopback: %d", scc_w[port].s.wr14.r.localloopback);
    DEBUG_LOG(0, "dpll command: %d", scc_w[port].s.wr14.r.dpll_cmd);
    DEBUG_LOG(0, "Write to 14 data=%02x odata=%02x wr14.v=%02x", data, odata, scc_w[port].s.wr14.v);
    DEBUG_LOG(0, "w[14]=%p wr14]%p", &scc_w[port].w[14], &scc_w[port].s.wr14.v);
    DEBUG_LOG(0, "sizeof w14 is %d", sizeof(wr14_t));
    if (data & 16)
    {
      DEBUG_LOG(0, "loopback SHOULD be on.");
    }

    break;

  case 15:
    scc_w[port].w[15] = data;
    DEBUG_LOG(0, "Write to 15 data=%02x odata=%02x", data, odata);

    DEBUG_LOG(0, "wr7prime:%d", scc_w[port].s.wr15.r.wr7prime);
    DEBUG_LOG(0, "zero_count_interrupt_enable:%d", scc_w[port].s.wr15.r.zero_count_interrupt_enable);
    DEBUG_LOG(0, "sldc_fifo_enable:%d", scc_w[port].s.wr15.r.sldc_fifo_enable);
    DEBUG_LOG(0, "dcd_interrupt_enable:%d", scc_w[port].s.wr15.r.dcd_interrupt_enable);
    DEBUG_LOG(0, "sync_hunt_interrupt_enable:%d", scc_w[port].s.wr15.r.sync_hunt_interrupt_enable);
    DEBUG_LOG(0, "cts_interrupt_enable:%d", scc_w[port].s.wr15.r.cts_interrupt_enable);
    DEBUG_LOG(0, "tx_underrun_eom_interrupt_enable:%d", scc_w[port].s.wr15.r.tx_underrun_eom_interrupt_enable);
    DEBUG_LOG(0, "break_abort_interrupt_enable:%d", scc_w[port].s.wr15.r.break_abort_interrupt_enable);
    break;
  }
  dump_scc();
} // end of function

uint8 lisa_rb_Oxd200_sccz8530(uint32 address)
{
  uint8 access, port, regnum = 0; // d

  // note if  scc_w[port].w14.localloopback external data should be ignored!

  DEBUG_LOG(0, "read byte from %08x", address);

  DEBUG_LOG(0, "---------------------------------------------------------------------------------------------------------------------");
  dump_scc();

  avoid_rom_scc_tests();

  /* fcd241 = control-b   FCD243 = control-a
     FCD245 = data-b      fcd247 = data-a
  #define SERIAL_PORT_A_DATA    0xFCD247
  #define SERIAL_PORT_A_CONTROL 0xFCD243
  #define SERIAL_PORT_B_DATA    0xFCD245
  #define SERIAL_PORT_B_CONTROL 0xFCD241
  */

  switch ((address & 0x000007) | 0x00FCD240)
  {
  case SERIAL_PORT_B_CONTROL:
    access = 0;
    port = 0;
    DEBUG_LOG(0, "SCC PORT B CONTROL");
    break;
  case SERIAL_PORT_B_DATA:
    access = 1;
    port = 0;
    DEBUG_LOG(0, "SCC PORT B DATA");
    break;

  case SERIAL_PORT_A_CONTROL:
    access = 0;
    port = 1;
    DEBUG_LOG(0, "SCC PORT A CONTROL");
    break;
  case SERIAL_PORT_A_DATA:
    access = 1;
    port = 1;
    DEBUG_LOG(0, "SCC PORT A DATA");
    break;

  default:
    ALERT_LOG(0, "Warning invalid access! %08x", address);
    return 0; // invalid address, just ignore it.
  }
  DEBUG_LOG(0, "SRC:SCC: access:%d port %d", access, port);
  DEBUG_LOG(0, "---------------------------------------------------------------------------------------------------------------------");

  if (access) //--------------- Data Port ---------------------------------------------------------------------------------
  {
    if (running_lisa_os == LISA_OFFICE_RUNNING)
    {
      if (!sentbytes[port])
      {
        sentbytes_start[port] = cpu68k_clocks;
      }

      sentbytes[port]++;
      ALERT_LOG(0, "port:%d sentbytes:%d startclk:%lld now:%lld dif:%lld", port, sentbytes[port], sentbytes_start[port], cpu68k_clocks, cpu68k_clocks - sentbytes_start[port]);
    }

    if (scc_w[0].s.wr9.r.soft_int_ack)
    {
      scc_r[0].r[2] = 0;
      z8530_last_irq_status_bits = 0;
    } // only one wr9

    if ((!port && z8530_last_irq_status_bits == 12) || (port && z8530_last_irq_status_bits == 4))
      z8530_last_irq_status_bits = 0; // 20060804

    if (!scc_w[port].s.wr3.r.rxenable)
    {
      DEBUG_LOG(0, "read failed rx disabled on port %d", port);
      return 0;
    } // can't read anything unless this is on.
    else
    {
      uint8 c;

      if (HAS_DATA(port))
      {
        /// ALERT_LOG(0,"------------------------------ reading from fliflo for port %d---------------",port);
        // fliflo_dump(stderr,&SCC_READ[0],"BEFORE get from queue");
        c = fliflo_buff_get(&SCC_READ[port]) & scc_bits_per_char_mask[port];
        // ALERT_LOG(0,"Read %02x from queue",c);
        // fliflo_dump(stderr,&SCC_READ[0],"AFTER get from queue");
        // ALERT_LOG(0,"-----------------------------------------------------------------------------");
      }
      else
      {
        ALERT_LOG(0, "fliflo is empty reading from port directly -------------------------------------");
        c = scc_fn[port].read_serial_port(port); // if nothing already waiting in fliflo, call read fn as needed.
      }
      ALERT_LOG(0, "read char %02x %c from fifo or fn port:%d fliflo size is:%d pc:%08x",
                c, (c > 31 ? c : '.'),
                port, fliflo_buff_size(&SCC_READ[port]), pc24);

      return c;
    }
  }

  regnum = scc_w[port].s.wr0.r.reg;
  if (scc_w[port].s.wr0.r.cmd == 1)
  {
    regnum |= 8;
    scc_w[port].s.wr0.r.cmd = 0;
  }
  DEBUG_LOG(0, "SRC:SCC: access:%d port %d regnum:%d  wr0 is %02x", access, port, regnum, scc_w[port].w[0]);
  scc_w[port].s.wr0.r.reg = 0; // reset register pointer back to zero for next round.
  if (scc_w[port].s.wr0.r.cmd == 1)
    scc_w[port].s.wr0.r.cmd = 0; // reset highpoint for next command

#ifdef ENHANCED_Z85C30
  if ((scc_r[port].w[15] & 4) == 0) // WR15 D2=0
#endif
    switch (regnum)
    {
    case 4:
      DEBUG_LOG(0, "Read from register 4");
#ifdef ENHANCED_Z85C30
      if (scc_r[port].w[15] & 4)
        return BITORD((scc_w[port].w[4]));
#endif
      // to return rr0
      /* FALLTHRU */

    case 0:
      DEBUG_LOG(0, "Read from register 0");

      // need to insert other pollers here for polled I/O
      // poll_telnet_serial_read(port);
#ifndef __MSVCRT__
      if (serial_b == SCC_TELNETD)
        read_port_if_ready_telnetd(0);
      //               { int data=poll_telnet_serial_read(0);
      //                 if (data>-1)
      //                        {
      //                            fliflo_buff_add(&SCC_READ[0],(uint8)(data) & scc_bits_per_char_mask[0]);
      //                            ALERT_LOG(0,"Received %02x %c from scc port:%d",
      //                                    (uint8)(data),
      //                                    ((data >31) ? ((uint8)(data)):'.'),
      //                                    port);
      //                        }
      //               }

      if (serial_a == SCC_TELNETD)
        read_port_if_ready_telnetd(1);
      //               {
      //                 int data=poll_telnet_serial_read(1);
      //                 if (data>-1) {fliflo_buff_add(&SCC_READ[1],(uint8)(data) & scc_bits_per_char_mask[1]);
      //                               ALERT_LOG(0,"Received %02x %c from scc port:%d",
      //                                    (uint8)(data),
      //                                    ((data >31) ? ((uint8)(data)):'.'),
      //                                    port);
      //                        }
      //               }
 
      if (serial_b == SCC_SHELL)
        read_port_if_ready_shell(0);
      //               { int data=read_serial_port_shell(0); // ::TODO:: this sometimes hangs - read gets called when it should not be even though we check for presence of data avail
      //                 if (data>-1)
      //                        {
      //                            fliflo_buff_add(&SCC_READ[0],(uint8)(data) & scc_bits_per_char_mask[0]);
      //                            ALERT_LOG(0,"Received %02x %c from scc port:%d pc:%08x",
      //                                    (uint8)(data),
      //                                    ((data >31) ? ((uint8)(data)):'.'),
      //                                   port,pc24);
      //                        }
      //               }
 
      if (serial_a == SCC_SHELL)
        read_port_if_ready_shell(1);
      //               {
      //                 int data=read_serial_port_shell(1);
      //                 if (data>-1) {fliflo_buff_add(&SCC_READ[1],(uint8)(data) & scc_bits_per_char_mask[1]);
      //                               ALERT_LOG(0,"Received %02x %c from scc port:%d",
      //                                    (uint8)(data),
      //                                    ((data >31) ? ((uint8)(data)):'.'),
      //                                    port);
      //                        }
      //               }

      if (serial_b == SCC_PTY)
        read_port_if_ready_pty(0);
      if (serial_a == SCC_PTY)
        read_port_if_ready_pty(1);

      if (serial_b == SCC_TTY)
        read_port_if_ready_tty(0);
      if (serial_a == SCC_TTY)
        read_port_if_ready_tty(1);
#endif

      if (serial_b == SCC_TERMINAL)
        read_port_if_ready_terminal(0);
      if (serial_a == SCC_TERMINAL)
        read_port_if_ready_terminal(1);

      scc_r[port].s.rr0.r.rx_char_available = HAS_DATA(port);
      scc_r[port].s.rr0.r.tx_buffer_empty = fliflo_buff_is_empty(&SCC_WRITE[port]);
      scc_r[port].s.rr0.r.dcd = get_dcd(port);
      scc_r[port].s.rr0.r.cts = get_cts(port);
      scc_r[port].s.rr0.r.break_abort = get_break(port);
      scc_r[port].s.rr0.r.sync_hunt = 0;
      scc_r[port].s.rr0.r.tx_underrun_eom = (!scc_w[port].s.wr5.r.txenable);
      scc_r[port].s.rr0.r.zero_count = 0;

      // sync hunt fakeout
      scc_r[port].s.rr0.r.sync_hunt = (scc_r[port].s.rr0.r.cts && scc_r[port].s.rr0.r.dcd);
      DEBUG_LOG(0, "RR0[%d] 0:rx_char_available:%d,  2:tx_buffer_empty:%d, 3:dcd:%d, 5:cts:%d, 7:break_abort:%d, 4:sync_hunt:%d, 6:tx_underrun_eom:%d, 1:zero_count:%d xonenable:%d xoffflag:%d fliflo_has_data:%d",
                port,
                scc_r[port].s.rr0.r.rx_char_available,
                scc_r[port].s.rr0.r.tx_buffer_empty,
                scc_r[port].s.rr0.r.dcd,
                scc_r[port].s.rr0.r.cts,
                scc_r[port].s.rr0.r.break_abort,
                scc_r[port].s.rr0.r.sync_hunt,
                scc_r[port].s.rr0.r.tx_underrun_eom,
                scc_r[port].s.rr0.r.zero_count,
                xonenabled[port], xoffflag[port], fliflo_buff_has_data(&SCC_READ[port]));

      if (scc_r[port].s.rr0.r.rx_char_available)
      {
        DEBUG_LOG(0, "RR0[%d] rcv buffer size is:%d  bit0:rx_char_available:%d,  2:tx_buffer_empty:%d, 3:dcd:%d, 5:cts:%d, 7:break_abort:%d, 4:sync_hunt:%d, 6:tx_underrun_eom:%d, 1:zero_count:%d xonenable:%d xoffflag:%d fliflo_has_data:%d",
                  port, fliflo_buff_size(&SCC_READ[port]),
                  scc_r[port].s.rr0.r.rx_char_available,
                  scc_r[port].s.rr0.r.tx_buffer_empty,
                  scc_r[port].s.rr0.r.dcd,
                  scc_r[port].s.rr0.r.cts,
                  scc_r[port].s.rr0.r.break_abort,
                  scc_r[port].s.rr0.r.sync_hunt,
                  scc_r[port].s.rr0.r.tx_underrun_eom,
                  scc_r[port].s.rr0.r.zero_count,
                  xonenabled[port], xoffflag[port], fliflo_buff_has_data(&SCC_READ[port]));
      }

      return BITORD(scc_r[port].r[0]);

    case 5:
      DEBUG_LOG(0, "Read from register 5");
#ifdef ENHANCED_Z85C30
      if (scc_r[port].w[15] & 4)
        return BITORD((scc_w[port].w[5]));
#endif
      // fall through to return rr1
      /* FALLTHROUGH */
    case 1:
      DEBUG_LOG(0, "Read from register 1");

      scc_r[port].s.rr1.r.rx_overrun_error = fliflo_buff_is_full(&SCC_READ[port]);
      scc_r[port].s.rr1.r.parity_error = 0;      // should we check for this?  We can check this one
      scc_r[port].s.rr1.r.crc_framing_error = 0; // only for sdlc?

      scc_r[port].s.rr1.r.all_sent = fliflo_buff_is_empty(&SCC_WRITE[port]);

      scc_r[port].s.rr1.r.residue_code_2 = 0; // for sdlc only, not implemented here.
      scc_r[port].s.rr1.r.residue_code_1 = 1;
      scc_r[port].s.rr1.r.residue_code_0 = 1;

      DEBUG_LOG(0, "RR1: all_sent:%d, residue_code_2:%d, residue_code_1:%d, residue_code_0:%d, parity_error:%d, rx_overrun_error:%d, crc_framing_error:%d, sdlc_end_of_frame:%d",
                scc_r[port].s.rr1.r.all_sent,
                scc_r[port].s.rr1.r.residue_code_2,
                scc_r[port].s.rr1.r.residue_code_1,
                scc_r[port].s.rr1.r.residue_code_0,
                scc_r[port].s.rr1.r.parity_error,
                scc_r[port].s.rr1.r.rx_overrun_error,
                scc_r[port].s.rr1.r.crc_framing_error,
                scc_r[port].s.rr1.r.sdlc_end_of_frame);

      return BITORD(scc_r[port].r[1]);

    case 6:
      DEBUG_LOG(0, "Read from register 6");
#ifdef ENHANCED_Z85C30
      if (scc_r[port].w[15] & 4)
        return BITORD((scc_r[port].r[6]));
#endif
      // fall through to rr2
      /* FALLTHROUGH */
    case 2:
      DEBUG_LOG(0, "Read from register 2");

      // scc_r[port].r[2]=scc_w[0].w[2];  // there is only one port 2!
      DEBUG_LOG(0, "IRQ vector bits:%d,%d,%d,%d,%d,%d,%d,%d",
                scc_w[0].s.wr2.r.v0, scc_w[0].s.wr2.r.v1, scc_w[0].s.wr2.r.v2, scc_w[0].s.wr2.r.v3,
                scc_w[0].s.wr2.r.v4, scc_w[0].s.wr2.r.v5, scc_w[0].s.wr2.r.v6, scc_w[0].s.wr2.r.v7);

      /*-----------------3/31/2005 2:24PM-----------------
       *  Unimplemented code
       * --------------------------------------------------
       if (scc_w[0].s.wr9.r.soft_int_ack)    scc_r[port].r[3]=0;  ???????????
       *
       *     ***************
             *** Bit 5: Software Interrupt Acknowledge control bit
             *** If bit D5 is set, reading Read Register 2 (RR2) results in an
             *** interrupt acknowledge cycle to be executed internally. Like
             *** a hardware INTACK cycle, a software acknowledge caus-
             *** es the INT pin to return High, the IEO pin to go Low, and
             *** sets the IUS latch for the highest priority interrupt pending.
       */
      // V3|V2|V1 Status High/Status Low =0
      // V4|V5|V6 Status High/Status Low =1
      DEBUG_LOG(0, "About to read register 2 which contains %02x, (reordered is %02x) on port %d - z8530_last_irq_status_bits is:%02x",
                scc_r[port].r[2], BITORD(scc_r[port].r[2]),
                port, z8530_last_irq_status_bits);

      if (port)
      {
        // if software interrupt ack is on, it clears the IRQ on access to rr2.
        if (scc_w[0].s.wr9.r.soft_int_ack)
        {
          scc_r[0].r[2] = 0;
          z8530_last_irq_status_bits = 0;
          scc_r[port].r[2] = 0;
        }
        return scc_w[0].s.wr2.v;
      }
      else
      {
        /*

        */
        uint8 r2 = BITORD(scc_r[port].r[2]);

        scc_r[0].r[2] = 0; /// 200608-8 <--- here here here here
        DEBUG_LOG(0, "PORT B, will include status bits");
        if (scc_w[0].s.wr9.r.status_hi_lo)
        {
          DEBUG_LOG(0, "HILO is HI - status bits in v4,5,6");
          scc_r[0].s.rr2.r.v4 = (z8530_last_irq_status_bits & 8) ? 1 : 0;
          scc_r[0].s.rr2.r.v5 = (z8530_last_irq_status_bits & 4) ? 1 : 0;
          scc_r[0].s.rr2.r.v6 = (z8530_last_irq_status_bits & 2) ? 1 : 0;
        }
        else
        {
          DEBUG_LOG(0, "HILO is LO - status bits in v1,2,3");
          scc_r[0].s.rr2.r.v1 = (z8530_last_irq_status_bits & 2) ? 1 : 0;
          scc_r[0].s.rr2.r.v2 = (z8530_last_irq_status_bits & 4) ? 1 : 0;
          scc_r[0].s.rr2.r.v3 = (z8530_last_irq_status_bits & 8) ? 1 : 0;
        }
        DEBUG_LOG(0, "Returning %02x which reordered is %02x from register %d on port %d",
                  scc_r[port].r[2], BITORD(scc_r[port].r[2]),
                  2, port);

        // if software interrupt ack is on, it clears the IRQ on access to rr2.
        if (scc_w[0].s.wr9.r.soft_int_ack)
        {
          scc_r[0].r[2] = 0;
          z8530_last_irq_status_bits = 0;
          scc_r[port].r[2] = 0;
        }

        DEBUG_LOG(0, "before exit, z8530_last_irq_status_bits:%d rr2=%02x returning:%02x", z8530_last_irq_status_bits, scc_r[port].r[2], r2);

        return r2;
      }

    case 7:
      DEBUG_LOG(0, "Read from register 7");
#ifdef ENHANCED_Z85C30
      if (scc_r[port].w[15] & 4)
        return BITORD((scc_r[port].r[7]));
#endif
      // fall through to rr3
      /* FALLTHROUGH */
    case 3:
      DEBUG_LOG(0, "Read from register 3 irq pending");
      {
        uint8 ret = BITORD(scc_r[port].r[3]); // IRQ pending.  These must be set by loop IRQ routine!
        scc_r[port].r[3] = 0;
        return ret;
      }

    case 8:
      DEBUG_LOG(0, "Read from register 8");
      scc_r[port].r[8] = fliflo_buff_peek(&SCC_READ[port]);
      return scc_r[port].r[8];

    case 14:
      DEBUG_LOG(0, "Read from register 14");
#ifdef ENHANCED_Z85C30
      if (scc_r[port].w[15] & 4)
        return BITORD((scc_w[port].w.w7prime));
#endif
      /* FALLTHROUGH */
    case 10:
      DEBUG_LOG(0, "Read from register 10 - returning 0 - sdlc not implemented");
      return 0;
      // return  BITORD(scc_r[port].r[10]);             // sdlc not implemented

    case 12:
      DEBUG_LOG(0, "Read from register 12");
      return BITORD(scc_w[port].w[12]); // return wr12

    case 9:
      DEBUG_LOG(0, "Read from register 9");
#ifdef ENHANCED_Z85C30
      if (scc_r[port].w[15] & 4)
        return BITORD((scc_w[port].w[3]));
#endif
      /* FALLTHROUGH */
    case 13:
      DEBUG_LOG(0, "Read from register 13"); // return wr13
      return BITORD(scc_w[port].w[13]);

    case 11:
      DEBUG_LOG(0, "Read from register 11");
#ifdef ENHANCED_Z85C30
      if (scc_r[port].w[15] & 4)
        return BITORD((scc_w[port].w[10]));
#endif
      /* FALLTHROUGH */
    case 15:
      DEBUG_LOG(0, "Read from register 15");
      return BITORD(scc_w[port].w[15]); // return wr15
    }

#ifdef ENHANCED_Z85C30
  // This is not needed for the Lisa, but it's here for future reuse of this code.
  if ((scc_w[port].w[15] & 4) == 4 && (scc[port].w[17] & 32) == 0) // WR15 D2=1
    switch (regnum)
    {
    case 0:
      return (scc_r[port].r[0]);
    case 1:
      return (scc_r[port].r[1]);
    case 2:
      return (scc_r[port].r[2]);
    case 3:
      return (scc_r[port].r[3]);
    case 4:
      return (scc_r[port].r[0]);
    case 5:
      return (scc_r[port].r[1]);
    case 6:
      return (scc_r[port].r[6]);
    case 7:
      return (scc_r[port].r[7]);
    case 8:
      return (scc_r[port].r[8]);
    case 9:
      return (scc_r[port].r[13]);
    case 10:
      return (scc_r[port].r[10]);
    case 11:
      return (scc_r[port].r[15]);
    case 12:
      return (scc_r[port].r[12]);
    case 13:
      return (scc_r[port].r[13]);
    case 14:
      return (scc_r[port].r[14]);
    case 15:
      return (scc_r[port].r[15]);
    }

  if ((scc_w[port].w[15] & 4) == 4 && (scc_w[port].w[17] & 32) == 32) // WR15 D2=1, WR7' D6=1
    switch (regnum)
    {
    case 0:
      return (scc_r[port].r[0]);
    case 1:
      return (scc_r[port].r[1]);
    case 2:
      return (scc_r[port].r[2]);
    case 3:
      return (scc_r[port].r[3]);
    case 4:
      return (scc_w[port].w[4]);
    case 5:
      return (scc_w[port].w[5]);
    case 6:
      return (scc_r[port].r[6]);
    case 7:
      return (scc_r[port].r[7]);
    case 8:
      return (scc_r[port].r[8]);
    case 9:
      return (scc_w[port].w[3]);
    case 10:
      return (scc_r[port].r[10]);
    case 11:
      return (scc_w[port].w[10]);
    case 12:
      return (scc_r[port].r[12]);
    case 13:
      return (scc_r[port].r[13]);
    case 14:
      return (scc_w[port].w[17]);
    case 15:
      return (scc_r[port].r[15]);
    }

} // end of read fn
#endif
DEBUG_LOG(0, "Oops! There's a bug somewhere! cases failed!");
return 0;
}

/*
  rs232 loopback plug emulation
  TX->RX
  RTS->CTS
  DTR->CD+DSR+RI
 */

// interface functions to host machine -- need to fill these in!

void send_break(unsigned int port)
{
  DEBUG_LOG(0, "generic Send Break on port %d", port);
  UNUSED(port);
  return;
}
void set_dtr(unsigned int port, uint8 value)
{
  DEBUG_LOG(0, "generic Set DTR on port %d to %d", port, value);
  UNUSED(port);
  UNUSED(value);
  return;
}
void set_rts(unsigned int port, uint8 value)
{
  DEBUG_LOG(0, "generic Set RTS on port %d to %d", port, value);
  UNUSED(port);
  UNUSED(value);
  return;
}

int get_dcd(unsigned int port)
{
  DEBUG_LOG(0, "generic Get Carrier Detect on port %d is %d", port, scc_r[port].s.rr0.r.dcd);
  scc_r[port].s.rr0.r.dcd = 1;
  if ((port && scc_a_IW != -1) || (!port && scc_b_IW != -1))
    return 1; // 0,0  1,1 no good
  return scc_r[port].s.rr0.r.dcd;
}

int get_cts(unsigned int port)
{
  DEBUG_LOG(0, "generic Get Clear to send on port %d is %d", port, scc_r[port].s.rr0.r.cts);
  if ((port && scc_a_IW != -1) || (!port && scc_b_IW != -1))
    return 0;
  // return scc_r[port].s.rr0.r.cts;
  return 1; /*2006.07.11*/
}

int get_break(unsigned int port)
{
  UNUSED(port);
  DEBUG_LOG(0, "generic Get brk status on port %d", port);
  // if break - also set this::: use this code when enabling break support
  // if (scc_w[port].wr15.s.break_abort_interrupt_enable) scc_r[1].s.rr3.ch_b_ext_status_irq_pending=1;
  return 0;
}

void signal_parity_error(unsigned int port)
{
  DEBUG_LOG(0, "generic Signal parity error port %d", port);
  UNUSED(port);
  return;
}
void signal_crc_error(unsigned int port)
{
  DEBUG_LOG(0, "generic Signal CRC error on port %d", port);
  UNUSED(port);
  return;
}

void set_even_parity(unsigned int port)
{
  DEBUG_LOG(0, "generic Set even parity  on port %d", port);
  UNUSED(port);
  return;
}
void set_odd_parity(unsigned int port)
{
  DEBUG_LOG(0, "generic Set  odd parity  on port %d", port);
  UNUSED(port);
  return;
}
void set_no_parity(unsigned int port)
{
  DEBUG_LOG(0, "generic Set   no parity  on port %d", port);
  UNUSED(port);
  return;
}

void set_dtr_loopbackplug(unsigned int port, uint8 value)
{
  scc_r[port].s.rr0.r.dcd = value;
  DEBUG_LOG(0, "Set loopback DTR on port %d to %d", port, value);
  return;
}

void set_rts_loopbackplug(unsigned int port, uint8 value)
{
  scc_r[port].s.rr0.r.cts = value;
  // weird loopback test cable
  if (0 == port && value == 0)
    scc_r[port].s.rr0.r.cts = 1;
  if (0 == port && 0 == scc_w[port].s.wr5.r.RTS && 0 != scc_w[port].s.wr5.r.DTR)
    scc_r[port].s.rr0.r.cts = 0;
  DEBUG_LOG(0, "Set loopback RTS on port %d to %d", port, value);
  return;
}

void set_baud_rate(int port, uint32 baud)
{
  UNUSED(port);
  UNUSED(baud);
} // dummy function

void set_bits_per_char(unsigned int port, uint8 bitsperchar)
{
  DEBUG_LOG(0, "Set bits/char on port %d to %d", port, bitsperchar);
  switch (bitsperchar)
  {
  case 5:
    scc_bits_per_char_mask[port] = 31;
    return;
  case 6:
    scc_bits_per_char_mask[port] = 63;
    return;
  case 7:
    scc_bits_per_char_mask[port] = 127;
    return;
  case 8:
    scc_bits_per_char_mask[port] = 255;
    return;
  default:
    DEBUG_LOG(0, "*BUG* bits/char was set to %d", bitsperchar);
    scc_bits_per_char_mask[port] = 255;
    return;
  }
}

void set_stop_bits(unsigned int port, uint8 stopbits)
{
  UNUSED(port);
  UNUSED(stopbits);
  return;
} // 0=0 no stop bits,1=1 stop bit,2=1.5 stop bits,3=2 stop bits

char read_serial_port_nothing(unsigned int port)
{
  UNUSED(port);
  return 0;
}
char read_serial_port_imagewriter(unsigned int port)
{
  UNUSED(port);
  return 0;
}
char read_serial_port_loopbackplug(unsigned int port)
{
  UNUSED(port);
  return -1;
} // this should never be called!
char read_serial_port_localport(unsigned int port)
{
  if (port)
  {
    if (scc_a_port_F)
      return fgetc(scc_a_port_F);
    else
      return 0;
  }
  else
  {
    if (scc_b_port_F)
      return fgetc(scc_b_port_F);
    else
      return 0;
  }
  return 0;
}

char read_serial_port(unsigned int port) // generic handler
{
  ALERT_LOG(0, "r %p %p w %p %p", SCC_READ[0].buffer, &SCC_READ[1].buffer, SCC_WRITE[0].buffer, SCC_WRITE[1].buffer);
  ALERT_LOG(0, "SRC:port:%d", port);
  if (port)
  {
    if (serial_a == SCC_NOTHING)
      return 0;
    if (serial_a == SCC_IMAGEWRITER)
      return 0;
    if (serial_a == SCC_LOCALPORT)
    {
      if (scc_a_port_F)
        return fgetc(scc_a_port_F);
      else
        return 0;
    }
  }
  else
  {
    if (serial_b == SCC_NOTHING)
      return 0;
    if (serial_b == SCC_IMAGEWRITER)
      return 0;
    if (serial_b == SCC_LOCALPORT)
    {
      if (scc_b_port_F)
        return fgetc(scc_b_port_F);
    }
  }
  return 0;
}

void disconnect_serial(int port)
{
  if (port != 0 && port != 1)
  {
    DEBUG_LOG(0, "Warning Serial port is not A/B: %d", port);
    return;
  }

  switch (port)
  {
  case 0:
    serial_b = SCC_NOTHING;
    break;
  case 1:
    serial_a = SCC_NOTHING;
    break;
    break;
  }

  scc_fn[port].read_serial_port = NULL;
  scc_fn[port].write_serial_port = NULL;
  scc_fn[port].read_port_if_ready = NULL;

  return;
}

void write_serial_port_nothing(unsigned int port, char data)
{
  UNUSED(port);
  UNUSED(data);
  DEBUG_LOG(0, "wrote %02x to port %d", data, port);
  return;
}
void write_serial_port_loopbackplug(unsigned int port, char data)
{
  DEBUG_LOG(0, "wrote %02x to port %d", data, port);
  fliflo_buff_add(&SCC_READ[(!port) & 1], data & scc_bits_per_char_mask[(!port) & 1]);
}

void write_serial_port_imagewriter(unsigned int port, char data)
{
  if (port)
  {
    DEBUG_LOG(0, "wrote %02x to port %d", data, port);
    if (scc_a_IW != -1)
    {
      DEBUG_LOG(0, "Write %02x to imagewriter", data);
      ImageWriterLoop(scc_a_IW, data);
    }
  }
  else
  {
    DEBUG_LOG(0, "wrote %02x to port %d", data, port);
    if (scc_b_IW != -1)
    {
      DEBUG_LOG(0, "Write %02x to imagewriter", data);
      ImageWriterLoop(scc_b_IW, data);
    }
  }
}

void write_serial_port_localport(unsigned int port, char data)
{
  if (port)
  {
    DEBUG_LOG(0, "wrote %02x to port %d", data, port);
    if (serial_a == SCC_LOCALPORT)
    {
      if (scc_a_port_F)
        fputc(data, scc_a_port_F);
    }
  }
  else
  {
    DEBUG_LOG(0, "wrote %02x to port %d", data, port);
    if (serial_b == SCC_LOCALPORT)
    {
      if (scc_b_port_F)
        fputc(data, scc_b_port_F);
    }
  }
}

void write_serial_port(unsigned port, char data)
{
  DEBUG_LOG(0, "r %p %p w %p %p", SCC_READ[0].buffer, &SCC_READ[1].buffer, SCC_WRITE[0].buffer, SCC_WRITE[1].buffer);
  DEBUG_LOG(0, "SRC:port:%d", port);
  if (port)
  {
    if (serial_a == SCC_NOTHING)
      return;
    if (serial_a == SCC_LOOPBACKPLUG)
    {
      fliflo_buff_add(&SCC_READ[!port & 1], data & scc_bits_per_char_mask[port]);
      return;
    }
    if (serial_a == SCC_IMAGEWRITER)
    {
      if (scc_a_IW != -1)
        ImageWriterLoop(scc_a_IW, data);
      return;
    }

    if (serial_a == SCC_LOCALPORT)
    {
      if (scc_a_port_F)
        fputc(data, scc_a_port_F);
    }
    return;
  }
  else
  {

    if (serial_b == SCC_NOTHING)
      return;
    if (serial_b == SCC_LOOPBACKPLUG)
    {
      fliflo_buff_add(&SCC_READ[!port & 1], data & scc_bits_per_char_mask[port]);
      return;
    }
    if (serial_b == SCC_IMAGEWRITER)
    {
      if (scc_b_IW != -1)
        ImageWriterLoop(scc_b_IW, data);
      return;
    }
    if (serial_b == SCC_LOCALPORT)
    {
      if (scc_b_port_F)
        fputc(data, scc_b_port_F);
    }
  }

  return;
}

// 30157-/usr/bin/x86_64-w64-mingw32-ld: obj/z8530.o:z8530.c:(.text+0x8e9): undefined reference to `poll_telnet_serial_read'
