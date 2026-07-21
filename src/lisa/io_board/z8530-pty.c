/**************************************************************************************\
*                                                                                      *
*                            The Lisa Emulator Project                                 *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright © 2023-2025 by Friends of Ray Arachelian                  *
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
*           Foundation, Inc., 59 Temple Place #330, Boston, MA 02111-1307, USA.      *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
*                                                                                      *
*                                                                                      *
*             Z8530 SCC Pseudo TTY Functions for Lisa serial ports                     *
* How it works and what it does:                                                       *
* This feature works only when running LisaEm on Linux.                                *
* In the LisaEm File->Preferences window, go to the "ports" tab and set the "Serial B" *
* dropdown  to "PseudoTTY"; Choose a name, e.g. "/tmp/lisaem-port-b". Apply and launch.*
* Behind the scenes, it will create a new pseudo-TTY (a fake, software-based serial    *
* port on the host Linux OS), which is usually named "/dev/pts/1"; it will also create *
* a symbolic link for it named "/tmp/lisaem-port-b", so you can use it without needing *
* to figure out the actual port name.                                                  *
* Example of how to use this:                                                          *
* Boot Lisa Workshop and from the main menu type T to launch the Terminal app          *
* (there is another T(erminal) app under File Manager, do not use that (confusing, ya?)*
* Make sure your connection settings are "connector=PortB, baudrate=<any value>,       *
  parity=None, handshake=None, duplex=Full".                                           *
* On the Linux host nachine, in a terminal window, type "screen /tmp/lisaem-port-b"    *
* (you need to have the "screen" application installed, e.g. "sudo apt install screen")*
* Now you have established a two-way serial communication between Lisa and Linux:      *
* everything you type in the Lisa Terminal window will appear in "screen", and vice    *
* versa.                                                                               *
* Note: Lisa uses the "Carriage Return" (aka "\r", symbol 0x0D) for new lines, so when *
* you type "Enter" in the Lisa Terminal, on the Linux side you will not see a new line,*
* as that is not a new-line in Linux (is not a "Line Feed" "\n").                      *
*                                                                                      *
* Above is just an example of establishing a connection. You can do more useful stuff, *
* like launching programs on both sides to send and receive files.                     *
*                                                                                      *
* Does the baud rate need to match on both sides? No: the pseudo TTY uses a buffer, so *
* it works well regardless of the baud rate chosen in LisaEm.                          *
* What about other parameters, like "parity" and "handshake"? You can safely use       *
* parity=None and handshake=None on both sides, and there shouldn't be any data loss.  *
*                                                                                      *
* Note: there is a bug in z8530.c that prevents you from transfering files larger than *
* a few bytes, and you will get errors 643 "Unexpected RS-232 interrupt" on the Lisa   *
* side. The original author is gone. The code is undocumented and unreadable. Good luck*
\***************************************************************************************/

// Based on the similar code in z8530-shell.c

#ifndef __MSVCRT__

// Somehow prevents compilation warnings.
#define _GNU_SOURCE
#define _XOPEN_SOURCE 600
#define __USE_BSD

// Claude says O_NDELAY is equivalent to O_NONBLOCK on macOS systems
// Create a compatibility shim
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

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

#include <fcntl.h>
#include <errno.h>

#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

// Two internal serial ports (A and B)
#define NUMSERPORTS 2

static int fd[NUMSERPORTS]; // file descriptor
static char input[NUMSERPORTS][150]; // data buffer
static char pty_symlink_path[NUMSERPORTS][64];

// invoked from lisaem_wx.cpp
void init_pty_serial_port(int port, char *desired_pty_port_symlink_name)
{
  int i, rc;
  char ptyname[1024];

  // Clear any previous symlink path
  memset(pty_symlink_path[port], 0, 64);

  // want to always set these for PTY
  scc_r[port].s.rr0.r.tx_buffer_empty = 1;
  scc_r[port].s.rr0.r.dcd = 1;
  scc_r[port].s.rr0.r.cts = 1;

  DEBUG_LOG(0, "openpt");
  fd[port] = posix_openpt(O_RDWR | O_NONBLOCK | O_NDELAY);
  if (fd[port] < 0)
  {
    fprintf(stderr, "Error %d on posix_openpt()\n", errno);
    return;
  }
  DEBUG_LOG(0, "grantpt");
  rc = grantpt(fd[port]);
  if (rc != 0)
  {
    fprintf(stderr, "Error %d on grantpt()\n", errno);
    return;
  }
  DEBUG_LOG(0, "unlockpt");
  rc = unlockpt(fd[port]);
  if (rc != 0)
  {
    fprintf(stderr, "Error %d on unlockpt()\n", errno);
    return;
  }
  
  // Make writes blocking to prevent data loss when writing to the PTY.
  // Reads will still be non-blocking due to the use of select() in poll_pty_serial_read().
  fcntl(fd[port], F_SETFL, 0);

  // Get the pty port name, it is e.g. "/dev/pts/1" 
  // Fix for macOS
  memset(ptyname, 0, 1023);
  #ifdef __APPLE__
    char *pts = ptsname(fd[port]);
    if (pts) strncpy(ptyname, pts, 1023);
  #else
    ptsname_r(fd[port], ptyname, 1023);
  #endif

  // Create a symbolic link to the PTY with the user-provided alias
  if (strlen(desired_pty_port_symlink_name) > 0)
  {
    strncpy(pty_symlink_path[port], desired_pty_port_symlink_name, 63);
    // Unlink any old symlink first
    unlink(desired_pty_port_symlink_name);
    // Note: this creates a symbolic link in your Linux file system (same as the "ln -s ..." Linux command).
    // It will remain there, once LisaEm exits.
    if (symlink(ptyname, desired_pty_port_symlink_name) == -1)
    {
      DEBUG_LOG(0, "Warning: could not create symlink '%s' -> '%s'. Error: %s", desired_pty_port_symlink_name, ptyname, strerror(errno));
    } else {
      DEBUG_LOG(0, "Created symlink for PTY: %s -> %s", desired_pty_port_symlink_name, ptyname);
    }
  }
  else
  {
     DEBUG_LOG(0, "The desired_pty_port_symlink_name is empty. Will not create a symlink.");
  }
}

int poll_pty_serial_read(int port)
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
  rc = select(fd[port] + 1, &fd_in, NULL, NULL, &tmo);
  if (rc < 0)
  {
    DEBUG_LOG(0, "got rc %d error from poll on port %d", rc, port);
    return 0;
  }
  return rc;
}

// Invoked from z8530.c
char read_serial_port_pty(int port)
{
  if (poll_pty_serial_read(port) > 0)
  {
    int num_bytes_read = read(fd[port], input[port], 1);
    if (num_bytes_read > 0)
    {
      DEBUG_LOG(0, "Received %c (%d) on serial port %d", input[port][0], input[port][0], port);
      return (int)(input[port][0]);
    }
    else if (num_bytes_read == 0)
    {
      DEBUG_LOG(0, "Received EOF on serial port %d", num_bytes_read, port);
    }
    else 
    {
      DEBUG_LOG(0, "Got %d error from poll on serial port %d", num_bytes_read, port);
    }
  }

  return -1;
}

// Invoked from z8530.c
int write_serial_port_pty(int port, uint8 data)
{
  input[port][0] = data;
  DEBUG_LOG(0, "Sending out %c (%d) to serial port %d", data, data, port);
  return write(fd[port], input, 1);
}

// Invoked from z8530.c
void set_dtr_pty(unsigned int port, uint8 value)
{
  DEBUG_LOG(0, "Set PTY DTR on port %d to %d, looping back to DCD", port, value);
  scc_r[port].s.rr0.r.dcd = value;
}

#endif
