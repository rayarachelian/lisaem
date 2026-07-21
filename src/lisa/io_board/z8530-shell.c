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
*           Foundation, Inc., 59 Temple Place #330, Boston, MA 02111-1307, USA.      *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
*                Z8530 SCC shell Functions for Lisa serial ports                       *
* How it works and what it does:                                                       *
* This feature works only when running LisaEm on Linux.                                *
* In the LisaEm File->Preferences window, go to the "ports" tab and set the "Serial B" *
* dropdown  to "Shell". Apply and launch LisaEm.                                       *
* Behind the scenes, it will launch a new pseudo-TTY (a fake, software-based serial    *
* port on the host Linux OS), and also will launch a new invisible bash/shell process, *
* whose stdin and stdout are connected to that pseudo TTY.                             *
* Example of how to use this:                                                          *
* Boot Lisa Workshop and from the main menu type T to launch the Terminal app          *
* (there is another T(erminal) app under File Manager, do not use that (confusing, ya?)*
* Make sure your connection settings are "connector=PortB, baudrate=<any value>,       *
  parity=None, handshake=None, duplex=Full". Now type "pwd<enter>" in the shell window.*
* It will execute this shell command on the host OS (Linux), and will print the output *
* of it in the Lisa Terminal.                                                          *
* Note: be extremely careful with what you type, as you may inadvertently delete files,*
* or cause other self-harm.                                                            *
* How do I find the name of the pseudo TTY port (in case Im a curious about this)?     *
*  Try "ls -l /dev/pts/", it will be usually "/dev/pts/1".                             *
* How cool is this? 10 out of 10.                                                      *
* How useful is this? 0 out of 10.                                                     *
\***************************************************************************************/

// Based on code from: mypty3 http://rachid.koucha.free.fr/tech_corner/pty_pdip.html

#ifndef __MSVCRT__

// Somehow prevents compilation warnings.
#define _GNU_SOURCE
#define _XOPEN_SOURCE 600
#define __USE_BSD

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
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

#ifndef sun
#include <err.h>
#endif

#include <fcntl.h>
#include <errno.h>

// #define __USE_BSD
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

// Claude says O_NDELAY is equivalent to O_NONBLOCK on macOS systems
// Create a compatibility shim
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif

// two internal serial ports, 4 port serial card * 3 slots = 14 future if Uni+/Xenix
// and Quad Port Cards are implemented, for now leave it at 2.
#define NUMSERPORTS 2

static int fdm[NUMSERPORTS], // file descriptor master
    fds[NUMSERPORTS],        // file descriptor slave
    myportnum,               // child's port number
    mypid = 0,               // child's pid
    parentpid = 0;           // parent pid

static char input[NUMSERPORTS][150]; // data buffer

pid_t child_pid[NUMSERPORTS]; // pid of children

#ifdef __sun
void cfmakeraw(struct termios *t)
{
  t->c_iflag &= ~(IMAXBEL | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  t->c_oflag &= ~OPOST;
  t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  t->c_cflag &= ~(CSIZE | PARENB);
  t->c_cflag |= CS8;
}
#endif

// :TODO: do we add signal handling code for sigchild ? or ignore?
// :TODO: force kill child on close by calling close fn AtExit
// :TODO: do we want to recreate child on child error/exit in a loop?
//       pid_t getpid(void);
//       pid_t getppid(void);

static int init_child(int port)
{
#ifdef DEBUG
  FILE *log;
  // ALERT_LOG is not available anymore since we forked (it would got to stderr of the new pty when initialized), so log to a file instead
  // open, append, flush and close the log on each entry since we have multiple ports, so multiple children might be writing here.

#define LOG(fmt, args...)                                                \
  {                                                                      \
    log = fopen("shell.log", "a+");                                        \
    fprintf(log, "%s:%s:%d port:%d ", __FILE__, __FUNCTION__, __LINE__); \
    fprintf(log, fmt, ##args);                                           \
    fprintf(log, "\n");                                                  \
    fflush(log);                                                         \
    fclose(log);                                                         \
  }

#define LOGBITS(fmt, val)                                                \
  {                                                                      \
    log = fopen("shell.log", "a+");                                        \
    fprintf(log, "%s:%s:%d port:%d ", __FILE__, __FUNCTION__, __LINE__); \
    fprintf(log, fmt, val);                                              \
    fprintf(log, "\n");                                                  \
    fflush(log);                                                         \
    fclose(log);                                                         \
  }

#else
#define LOG(x, args...) \
  {                     \
  }
#define LOGBITS(x, args...) \
  {                         \
  }

#endif

  int rc, ignore;
  struct termios slave_orig_term_settings; // Saved terminal settings
  struct termios new_term_settings;        // Current terminal settings

  // CHILD
  close(fdm[port]); // Close the master side of the PTY

  // Save the defaults parameters of the slave side of the PTY
  rc = tcgetattr(fds[port], &slave_orig_term_settings);
  // Set RAW mode on slave side of PTY
  new_term_settings = slave_orig_term_settings;

  LOGBITS("slave_orig_term_settings: c_iflag: %08x", slave_orig_term_settings.c_iflag);
  LOGBITS("slave_orig_term_settings: c_oflag: %08x", slave_orig_term_settings.c_oflag);
  LOGBITS("slave_orig_term_settings: c_cflag: %08x", slave_orig_term_settings.c_cflag);
  LOGBITS("slave_orig_term_settings: c_lflag: %08x", slave_orig_term_settings.c_lflag);

  // if we don't make raw, it will hang on input forever, so we want some combo of settings from raw and cooked here for it to behave.
  cfmakeraw(&new_term_settings);
  // raw:-ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr -icrnl -ixon -ixoff -icanon -opost -isig -iuclc -ixany -imaxbel -xcase min 1 time 0

  LOGBITS("new_term_settings: c_iflag: %08x", new_term_settings.c_iflag);
  LOGBITS("new_term_settings: c_oflag: %08x", new_term_settings.c_oflag);
  LOGBITS("new_term_settings: c_cflag: %08x", new_term_settings.c_cflag);
  LOGBITS("new_term_settings: c_lflag: %08x", new_term_settings.c_lflag);

  // might need to change these for macos, *bsd?
  new_term_settings.c_iflag |= ICRNL | IXON;
  new_term_settings.c_oflag |= OPOST;
  new_term_settings.c_lflag |= ECHO | ECHOCTL | ECHOE | ECHOK | ECHOKE | ICANON | ISIG;

  // both raw and sane turn these off, no need to do it ^^^ here: -inlcr -igncr -ixoff -iuclc -ixany
  // stty sane off: -ignbrk -echonl -noflsh -ixoff -iutf8 -iuclc -ixany -xcase -olcuc -ocrnl -ofill -onocr -onlret -tostop -ofdel -echoprt -extproc -flusho

  tcsetattr(fds[port], TCSANOW, &new_term_settings);
  //       sane   same  as  cread  -ignbrk brkint -inlcr -igncr icrnl icanon iexten echo echoe echok -echonl -noflsh -ixoff -iutf8 -iuclc -ixany imaxbel -xcase -olcuc -ocrnl
  //        opost -ofill onlcr -onocr -onlret nl0
  //            cr0 tab0 bs0 vt0 ff0 isig -tostop -ofdel -echoprt echoctl echoke -extproc -flusho, all special characters to their default values

  // raw:-ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr -icrnl -ixon -ixoff -icanon -opost -isig -iuclc -ixany -imaxbel -xcase min 1 time 0
  // stty sane off: -ignbrk -inlcr -igncr -echonl -noflsh -ixoff -iutf8 -iuclc -ixany -xcase -olcuc -ocrnl -ofill -onocr -onlret -tostop -ofdel -echoprt -extproc -flusho

  // The slave side of the PTY becomes the standard input and outputs of the child process
  close(0); // Close standard input  (current terminal)
  close(1); // Close standard output (current terminal)
  close(2); // Close standard error  (current terminal)

  ignore = dup(fds[port]); // PTY becomes standard input (0)
  ignore = dup(fds[port]); // PTY becomes standard output (1)
  ignore = dup(fds[port]); // PTY becomes standard error (2)

  // Now the original file descriptor is useless
  close(fds[port]);

  // Make the current process a new session leader
  setsid();

  // As the child is a session leader, set the controlling terminal to be the slave side of the PTY
  // (Mandatory for programs like the shell to make them manage correctly their outputs)
  ioctl(0, TIOCSCTTY, 1);
  // Execute a shell
  {

    char *child_av[2];
    int i;
    char temp[1024];
    char *shellname = NULL;

    shellname = (port) ? scc_b_port : scc_a_port;

    // if options were passed, use them as the program to run
    if (shellname != NULL)
    {
      if (strlen(shellname) < 1)
        shellname = NULL;
    }

    // otherwise, get it from the environment, and if it's not there, fall back to bash.
    if (!shellname)
      shellname = getenv("SHELL");
    if (!shellname)
      shellname = "bash";

    // LisaTerm doesn't ignore ANSI/vt100 color codes, it instead aborts all output until the buffer is flushed! wtf!
    // this is helpful, but not all CLI software honors NO_COLOR. see: https://no-color.org/
    LOG("setting TERM and rest of environment");
    setenv("TERM", "vt100", 1); // LisaTerm, Xenix, UniPlus aren't going to understand modern xterm-color256 here.
    setenv("NO_COLOR", "nocolor", 1);
    setenv("LS_COLORS", "none", 1); // color output breaks LisaTerm.
    // setenv("GREP_OPTIONS","--color=never",1); // meh, grep warns that GREP_OPTIONS is deprecated and aliases must be used.
    setenv("LC_ALL", "C", 1);
    // unfortunately I don't have a way to disable color in vim/nvi from the environment, end user will need to
    // use a special .bashrc and .vimrc, etc. to do that.

    child_av[0] = shellname;
    child_av[1] = NULL;

    LOG("about to exec shell:%s", child_av[0]);

    rc = execvp(child_av[0], child_av);
    // for some reason, bash becomes a zombie immediately here rather than just waiting for input from the port.
    LOG("OOPS! execvp failed with return:%d errno:%d %s", rc, errno, strerror(errno));
    exit(1);
    // int execvp(const char *file, char *const argv[]); - maybe change this to while(1) system(child_av[0]);  to keep it in a loop?
    // and then sigterm will kill it. this way we don't need to worry about sigchild and recreating the pty, etc. if it's disconnected in the middle of a session.
    // for (i=0; i<10; i++) system(shellname); // maybe instead of while(1) give up after tries, maybe the shell is broken, etc.
  }

  close(fds[port]); // Close if exec failed
  // if Error...
  return -1;
}

static int init_parent(int port)
{
  // int i, rc;
  // fd_set fd_in;

  ALERT_LOG(0, "initialize parent, closing slave side");
  close(fds[port]);

  //  ALERT_LOG(0,"FD_ZERO");  FD_ZERO(    &fd_in);
  //  ALERT_LOG(0,"FD_SET");   FD_SET(0,   &fd_in);
  //  ALERT_LOG(0,"FD_SET");   FD_SET(fdm[port], &fd_in);
  //  ALERT_LOG(0,"SELECT:");
  //  rc = select(fdm[port] + 1, &fd_in, NULL, NULL, NULL); // this causes it to wait for hitting enter in the terminal before it continues, freezing lisaem on poweron
  //  ALERT_LOG(0,"select result:%d",rc);
  //  if (rc<0) return -1;
  return 0;
}

void init_shell_serial_port(int port)
{
  int i, rc;
  char ptyname[1024];

  // want to always set these for PTY
  scc_r[port].s.rr0.r.tx_buffer_empty = 1;
  scc_r[port].s.rr0.r.dcd = 1;
  scc_r[port].s.rr0.r.cts = 1;

  // for some reason, compilation warnings are issued on posix_openpt, grantpt, unlockpt but fnctl.h and stdlib.h are both included, wtf?
  // Check arguments is O_NDELAY something we want?                 // 2020.12.07 added
  ALERT_LOG(0, "openpt");
  fdm[port] = posix_openpt(O_RDWR | O_NONBLOCK | O_NDELAY);
  if (fdm[port] < 0)
  {
    fprintf(stderr, "Error %d on posix_openpt()\n", errno);
    return;
  }
  ALERT_LOG(0, "grantpt");
  rc = grantpt(fdm[port]);
  if (rc != 0)
  {
    fprintf(stderr, "Error %d on grantpt()\n", errno);
    return;
  }
  ALERT_LOG(0, "unlockpt");
  rc = unlockpt(fdm[port]);
  if (rc != 0)
  {
    fprintf(stderr, "Error %d on unlockpt()\n", errno);
    return;
  }

  memset(ptyname, 0, 1023);
  // Open the slave side ot the PTY
#ifdef __linux__
  ptsname_r(fdm[port], ptyname, 1023);
#else
  strncpy(ptyname, ptsname(fdm[port]), 1023);
#endif
  ALERT_LOG(0, "open_ptsname: %s for port:%d", ptyname, port);
  fds[port] = open(ptyname, O_RDWR, O_NONBLOCK);
  ALERT_LOG(0, "got fd# %d for port %d", fds[port], port);
  ALERT_LOG(00, "Getting ready to fork...");
  // Create the child process

  if ((child_pid[port] = fork()))
    i = init_parent(port);
  else
    i = init_child(port);
  ALERT_LOG(0, "done with fork");
  return;
} // main

// int poll_shell_serial_read(int port) { return (FD_ISSET(fdm[port], &fd_in[port])); }

int poll_shell_serial_read(int port)
{
  int rc;
  fd_set fd_in;

  FD_ZERO(&fd_in);
  FD_SET(0, &fd_in);
  FD_SET(fdm[port], &fd_in);
  struct timeval tmo;
  tmo.tv_sec = 0;
  tmo.tv_usec = 1;
  // tmo.tv_nsec=1;
  //           num fd's       fd in   fdout, fdexcept, timeout
  rc = select(fdm[port] + 1, &fd_in, NULL, NULL, &tmo); // was fdm[port] + 1, &fd_in, NULL, NULL, &tmo)
  if (rc < 0)
  {
    ALERT_LOG(0, "got rc %d error from poll on port %d", rc, port);
    return 0;
  }
  // if (rc) rc=FD_ISSET(fdm[port]+1,&fd_in);  //2020.11.27
  return rc; // return (FD_ISSET(0, &fd_in));
}

char read_serial_port_shell(int port)
{
  int rc = 0;

  if (poll_shell_serial_read(port) > 0)
  {
    rc = read(fdm[port], input[port], 1);
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

int write_serial_port_shell(int port, uint8 data)
{
  input[port][0] = data;
  ALERT_LOG(0, "Writing %c(%d) to port %d", data, data, port);
  return write(fdm[port], input, 1);
}

void close_shell(int port)
{
  close(fdm[port]);
  fdm[port] = -1;
  kill(child_pid[port], SIGTERM);
}

#endif