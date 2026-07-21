/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.12.12                   *
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
*                          Z8530 SCC tty Functions for                                 *
*                              Lisa serial ports                                       *
\**************************************************************************************/

// Based on code from: mypty3 http://rachid.koucha.free.fr/tech_corner/pty_pdip.html
// and https://www.cmrr.umn.edu/~strupp/serial.html#2_5_2

#ifndef __MSVCRT__

#include <vars.h>
#include <z8530_structs.h>

#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#ifndef sun
#include <err.h>
#endif

#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <errno.h>
#define __USE_BSD
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

// two internal serial ports, 4 port serial card * 3 slots = 14 future if Uni+/Xenix
// and Quad Port Cards are implemented, for now leave it at + 1 for BLU communications.
#define NUMSERPORTS 16

static int fd[NUMSERPORTS];
struct termios options;

static char input[NUMSERPORTS][150]; // data buffer

extern int verifybaud(int baud);

// config string:  "/dev/ttyUSBS0:9600N81"
// if baud rate isn't pinned, then let LisaTerminal set it, etc.

extern void disconnect_serial(int port);
extern uint32 get_baud_rate(unsigned int port);

void set_port_baud_tty(int port, uint32 baud)
{
  if (port < 0 || port > NUMSERPORTS)
    return;

  if (fd[port] < 2)
    return; // -1=error, 0=stdin, 1=stdout, 2=stderr; so fd should be at least 3 if valid.

  if (baudoverride[port])
    baud = baudoverride[port]; // if the user selected a specific baud rate, use that no matter what the Lisa says.

  tcgetattr(fd[port], &options);

  switch (baud)
  {
  case 50:
    cfsetispeed(&options, B50);
    cfsetospeed(&options, B50);
    break;
  case 75:
    cfsetispeed(&options, B75);
    cfsetospeed(&options, B75);
    break;
  case 110:
    cfsetispeed(&options, B110);
    cfsetospeed(&options, B110);
    break;
  case 134:
    cfsetispeed(&options, B134);
    cfsetospeed(&options, B134);
    break;
  case 150:
    cfsetispeed(&options, B150);
    cfsetospeed(&options, B150);
    break;
  case 200:
    cfsetispeed(&options, B200);
    cfsetospeed(&options, B200);
    break;
  case 300:
    cfsetispeed(&options, B300);
    cfsetospeed(&options, B300);
    break;
  case 600:
    cfsetispeed(&options, B600);
    cfsetospeed(&options, B600);
    break;
  case 1200:
    cfsetispeed(&options, B1200);
    cfsetospeed(&options, B1200);
    break;
  case 1800:
    cfsetispeed(&options, B1800);
    cfsetospeed(&options, B1800);
    break;
    //       case 2000:   cfsetispeed(&options, B2000  ); cfsetospeed(&options, B2000  );  break;
  case 2400:
    cfsetispeed(&options, B2400);
    cfsetospeed(&options, B2400);
    break;
    //       case 3600:   cfsetispeed(&options, B3600  ); cfsetospeed(&options, B3600  );  break;
  case 4800:
    cfsetispeed(&options, B4800);
    cfsetospeed(&options, B4800);
    break;
  case 9600:
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    break;
  case 19200:
    cfsetispeed(&options, B19200);
    cfsetospeed(&options, B19200);
    break;
  case 38400:
    cfsetispeed(&options, B38400);
    cfsetospeed(&options, B38400);
    break;
  case 57600:
    cfsetispeed(&options, B57600);
    cfsetospeed(&options, B57600);
    break;
    //       case 76800:  cfsetispeed(&options, B76800 ); cfsetospeed(&options, B76800 );  break;
  case 115200:
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    break;
  }
  tcsetattr(fd[port], TCSANOW, &options);
}

void init_tty_serial_port(int port, char *config)
{
  int i, rc;
  char ptyname[1024];

  // want to always set these for PTY/TTY
  scc_r[port].s.rr0.r.tx_buffer_empty = 1;
  scc_r[port].s.rr0.r.dcd = 1;
  scc_r[port].s.rr0.r.cts = 1;

  int bps = get_baud_rate(port);

  char *s, *dev, *settings;
  char device[64];

  uint32 baud = 0;
  int bits = 8, stopbits = 1;
  char parity = 'N';
  // /dev/ttyUSB0
  // "/dev/ttyUSB0|9600,N,8,1"
  memset(device, 0, 63);
  strncpy(device, config, 63);
  s = strchr(device, '|');
  if (s)
    *s = 0;

  if (strlen(device) == 0)
  {
    disconnect_serial(port);
    return;
  }

  settings = strchr(config, '|');
  if (settings[0] == '|')
    settings++;

  ALERT_LOG(0, "Device: %s Settings:%s", device, settings);

  baud = verifybaud(atol(settings)); // ensure sane baud rates

  if (strstr(settings, ",8") != NULL)
    bits = 8;
  if (strstr(settings, ",7") != NULL)
    bits = 7;

  if (strstr(settings, ",1") != NULL)
    stopbits = 1;
  if (strstr(settings, ",2") != NULL)
    stopbits = 2;

  if (strstr(settings, ",N") != NULL)
    parity = 'N';
  if (strstr(settings, ",n") != NULL)
    parity = 'N';

  if (strstr(settings, ",E") != NULL)
    parity = 'E';
  if (strstr(settings, ",e") != NULL)
    parity = 'E';

  if (strstr(settings, ",O") != NULL)
    parity = 'E';
  if (strstr(settings, ",o") != NULL)
    parity = 'E';

  ALERT_LOG(0, "%d bits, %c parity, %d stop bits\n", bits, parity, stopbits);

  fd[port] = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd[port] == -1)
  {
    ALERT_LOG(0, "could not open %s - disconnecting\n", device);
    disconnect_serial(port);
    return;
  }
  fcntl(fd[port], F_SETFL, 0);

  if (baud)
  {
    baudoverride[port] = baud;
    set_port_baud_tty(port, baud);
  }

  tcgetattr(fd[port], &options);

  options.c_cflag |= (CLOCAL | CREAD); // Enable the receiver and set local mode

  if (parity == 'N' || parity == 'S')
  {
    options.c_cflag &= ~PARENB;
  }
  if (parity == 'E')
  {
    options.c_cflag |= PARENB;
    options.c_cflag &= ~PARODD;
  }
  if (parity == 'O')
  {
    options.c_cflag |= PARENB;
    options.c_cflag |= PARODD;
  }
  if (bits == '7')
  {
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS7;
  }
  else
  {
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
  }
  if (strstr(settings, ",hw") != NULL || strstr(settings, ",HW") != NULL)
  {
    options.c_cflag |= CRTSCTS;
  } // enable hw handshaking
  else
  {
    options.c_cflag &= ~CRTSCTS;
  } // disable hw handshaking

  if (strstr(settings, ",xon") != NULL || strstr(settings, ",XON") != NULL || xonenabled[port])
  {
    options.c_iflag |= (IXON | IXOFF | IXANY);
  } // enable software handshaking
  else
  {
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
  } // disable software handshaking

  if (strstr(settings, ",-xon") != NULL || strstr(settings, ",-XON"))
  {
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
  } // force disable software handshaking even if xonenabled

  options.c_oflag &= ~OPOST;                          // raw output
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // raw input

  tcsetattr(fd[port], TCSANOW, &options);
}

int poll_tty_serial_read(int port)
{
  int rc;
  fd_set fd_in;

  FD_ZERO(&fd_in);
  FD_SET(0, &fd_in);
  FD_SET(fd[port], &fd_in);
  struct timeval tmo;
  tmo.tv_sec = 0;
  tmo.tv_usec = 1;
  // tmo.tv_nsec=1;
  //           num fd's       fd in   fdout, fdexcept, timeout
  rc = select(fd[port] + 1, &fd_in, NULL, NULL, &tmo);
  if (rc < 0)
  {
    ALERT_LOG(0, "got rc %d error from poll on port %d", rc, port);
    disconnect_serial(port);
    return 0;
  }
  return rc; // return (FD_ISSET(0, &fd_in));
}

char read_serial_port_tty(int port)
{
  int rc = 0;

  if (poll_tty_serial_read(port) > 0)
  {
    rc = read(fd[port], input[port], 1);
    if (rc > 0)
    {
      ALERT_LOG(0, "read: char %c, %02x port:%d", input[port][0], input[port][0], port);
      return (int)(input[port][0]);
    }
    if (rc < 0)
      ALERT_LOG(0, "got rc %d error from poll on port %d", rc, port);
  }

  return -1;
}

int write_serial_port_tty(int port, uint8 data)
{
  input[port][0] = data;
  ALERT_LOG(0, "Sending character %c(%d 0x%2x) to tty port %d", data, data, data, port);
  return write(fd[port], input, 1);
}

void close_tty(int port)
{
  close(fd[port]);
  fd[port] = -1;
}

#else

#include <vars.h>
// fake out fn

char read_serial_port_tty(int port) { return 0; }
void write_serial_port_tty(int port, uint8 data){};
int poll_tty_serial_read(int port) { return 0; }

#endif
