/**************************************************************************************\
*                                                                                      *
*                          Apple Daisy Wheel Printer Emulator                          *
*                                                                                      *
*                     A FUTURE Part of the Lisa Emulator Project                       *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                      Copyright (C) 2022 Ray A. Arachelian                            *
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
\**************************************************************************************/

// needed for wxWidgets.
//#define MIRRORX 1
#define MIRRORY 1

#include <wx/wx.h>
#include <wx/defs.h>

#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/dcprint.h>
#include <wx/image.h>
#include <wx/filedlg.h>
#include <wx/display.h>
#include <wx/gdicmn.h>
#include <wx/pen.h>
#include <wx/filename.h>


// But since wxWidgets has close enough cousins of these, use those instead.
#ifdef WORDS_BIGENDIAN
#define BYTES_HIGHFIRST  1
#endif

#ifdef wxUint64
typedef  wxUint64 uint64;
#endif
typedef  wxUint32 uint32;
typedef  wxUint16 uint16;
typedef  wxUint8 uint8;

#ifdef wxInt64
typedef  wxInt64 int64;
#endif
typedef  wxInt32 int32;
typedef  wxInt16 int16;
typedef  wxInt8 int8;

typedef int8 sint8;
typedef int16 sint16;
typedef int32 sint32;


// Load the fonts up... / ::TODO:: scan in ADWP fonts hires or use system fonts that may match
#include "iwfonts.h"

// Most OS's have this defined, for the ones that don't, 1024 is a reasonable value
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* https://vintageapple.org/macmanuals/pdf/Apple_Daisy_Wheel_Printer_029-0083-A_1983.pdf

132 cols at 10 cpi
158 cols at 12 cpi
198 cols at 15 cpi
120 positions per inch resolution
6 or 8 lines per inch (vertical)
130 chars / printwheel 

DIPSW Front Panel DIP SW
8: 1=8 lines per inch, 0=6 lines per inch
7: 1=AutoLF after CR, 0=no
6-3: Form Length:
     0000  3.00"
     0001  3.50"
     0010  4.00"
     0011  5.25"
     0100  6.00"
     0101  7.00"
     0110  8.00"
     0111  8.50"
     1000 11.00" *default*
     1001 11.66"
     1010 12.00"
     1011 14.00"
     1100  5.00" ???
     1101 ?
     1110 ?
     1111 16.00"
2-1:
     00: 10cpi
     01: 12cpi *default*
     10: 15cpi
     11: 11 PS

DIPSW Rear Panel DIP SW 1
8-7:  11=SPACE *DEFAULT*
      10=MARK
      01=EVEN
      00=ODD
6:    1=No Modem *default*, 0: Modem ???
5-4:  00: ETX/ACK/DTR *default*
      01: XON/XOFF
      10: DTR
      11: unused
3-1:  000: 110bps
      001: 150bps
      010: 600bps
      100: 1200bps *default*
      101: 2400bps
      110: 4800bps
      111: 9600bps

DIPSW Rear Panel DIP SW 2
8:   1=stop if paper out *default*, 0-no
7:   1=half duplex,0=full duplex *default*
6:   1=Auto CR/LF, 0=no *default*
5:   1=BiDirectional Print *default*, 0=unidirectional
4-1: 0000 ASCII Standard *default*
     0001 USA WP
     0010 Italian
     0011 Swedish
     0100 English (UK)
     0101 French
     0110 German
     0111 Spanish


Font Chars for ASCII hex: 23 40 5b 5c 5d 5e 60 7b 7c 7d 7e vary by language of font, rest are the same (circled numbers on quickref)

Note there's a hammer intensity setting, as well as directional setting (backwards print, etc. movement inhibit for underlines/overstrikes, etc.)
see ESC SO, ESC 6, etc.

p56

Command Set:

ESC 9                  1b 39                 - Set Left Margin at current position. Not column dependent. Set at absolute position!
                                               you can backspace to the left of it, it's not a barrier. Just sets where head goes after CR
                                               power on, or "ESC SUB I" (reset) will reset it to position 0.
ESC 0                  1b 30                 - Set Right Margin at current position. Power on or "ESC SUB I" will reset it to rightmost column
ESC 1                  1b 31                 - Set Horizontal Tab Stop at current position
                                               198 tabs max at 15 cpi
                                               154 tabs max at 12 cpi
                                               132 tabs max at 10 cpi
                                               tab numbers don't change when CPI changes, but if they go past the end of the right side, head stops there
                                               i.e. if you set a tab position at 100 when at 15 cpi, it will be at position 100 at 10 cpi
                                               can probably just do an array of 198 uint8 to implement tab stops and same for vertical ones
ESC 8                  1b 38                 - Clear Horizontab Tab Stop at current position
ESC ( list 0x2E        1b 28 <list> 2e       - Set tab stops from list. List format: p1 p2 p3...p[n] where p is a tupple. 2e at end is "."
                                               i.e. ESC ( 0 5 , 4 3, 7 9, A 4, ,A 9 .    for 05 43 79 104 109999999999999999999999999999999999999999999
                                               p: <a1> <a0> : a1=asc(hex(p/10)) a0=asc(hex(p mod 10)). i.e.: "ESC ( A 0, 0 4 ."   # sets tabs at 4 and 100 
ESC ) list 0x2E        1b 29 <list> 2e       - Clear tab stops from list. List format: p1 p2 p3...p[n] where p is a tupple:
                                               p: <a1> <a0> : a1=asc(hex(p/10)) a0=asc(hex(p mod 10)). i.e.: "ESC ) F 9, 1 1 ."   # clears tabs at 11 and 159
ESC 2                  1b 32                 - Clear all horizontal tab stops
ESC F a1 a0            1b 46 <a1><a0>        - Set Form Length to current top of form length in n/6" 0<n<128, a1=asc(hex(p/10)) a0=asc(hex(p mod 10))
                                               i.e. "ESC F 6 6" for 11" form length, "ESC F A 8"  for 18" (108"/6") form length
ESC +                  1b 2b                 - Set TOP Margin at current print position, if y>bottom margin, go to next page top margin at this location
ESC -                  1b 2d                 - Set BOTTOM Margin at current print position, once past this, will FF to next page, top margin
ESC E a1 a0            1b 45 <a1><a0>        - Define Horizontal Spacing Increments, space n/120" units per SP 0<n<160 a1=asc(hex(p/10)) a0=asc(hex(p mod 10))
                                               i.e. "ESC E 1 2" to space 1/10th inch, "ESC E C O" to space 1"
ESC <US> <a>           1b 1f <a>             - Define Horizontal Spacing Increments n/120" 0<n<126, a=ascii(n+1)
                                               i.e. "ESC US VT" for 1/12" spacing, "ESC US US" will set to 1/4" (30).
ESC L <a1> <a0>        1b 4c <a1> <a0>       - Define vertical spacing increments n/48" units 0<n<160 a1=asc(hex(p/10)) a0=asc(hex(p mod 10))
                                               i.e. "ESC L 0 8" for 1/6th" linefeed, "ESC L F 9" for 159/48" linefeed
ESC RS <a>             1b 1E <a>             - Define vertical spacing increments n/48" 0<n<126  (n or a is ascii, 12 would be CR so +1)
                                               i.e. "ESC RS %" for 3/4ths" linefeed

ESC SUB I              1b 1a 49              - Reset printer settings (hard reset)
ESC CR P               1b 0d 50              - Initialize printer (soft reset)
                                               both ESC SUB I, ESC CR P:
                                                 carriage to column zero
                                                 print wheel syncrhonized with printer electronics
                                                 all tab stops cleared
                                                 right margin set to rightmost column
                                                 left margin set to column zero
                                                 top margin set to top
                                                 bottom margin set to DIP switch position form length
                                                 Both graphics modes off
                                                 forward print mode ON
                                                 Both program modes off
                                                 Right margin control off
                                                 underscore mode off
                                                 bold mode off
                                                 shadow mode off
                                                 no print mode off
                                                 user test mode off

ESC ,                  1b 2c                 - Turn on Auto LF after every CR
ESC Z                  1b 5a                 - Turn off Auto CR/LF
ESC W                  1b 57                 - Auto CR/LF at right margin on
ESC O                  1b 4f                 - Right Margin Control On - Auto CR if SP in hot zone within 5 cols before right margin
ESC .                  1b 2e                 - Turn off Auto Line Feed mode, overrides DIP
ESC Y                  1b 59                 - Right Margin Control off
ESC 5                  1b 35                 - Forward Print
ESC 6                  1b 36                 - Backward Print (until CR or ESC 5). BS will go from left to right, etc. Add flag for this for every char
                                               printed, same idea as the forward inhibit ("ESC N")
ESC <                  1b 3c                 - Automatic Bidirectional Print On
ESC >                  1b 3e                 - Automatic Bidirectional Print Off
ESC S                  1b 53                 - Disable Printer (offline) - add as a mode variable
ESC T                  1b 54                 - Enable Printer (online)
ESC X                  1b 58                 - Execute Pending Motions (print and flush print buffer?)
ESC l p                1b 6C <p>             - Select Language where p= A=ASCII Standard, B=USA WP, C=Italian, D=Swedish, 
                                               E=English UK, F=French, G=German, H=Spanish, Q=DIP SW setting
                                               note this is a lower case L
ESC $                  1b 24                 - Proportional Font Spacing Printwheel On    - WPS on
ESC %                  1b 25                 - Proportional Font Spacing Printwheel Off   - WPS off

ESC Q                  1b 51                 - Shadow Print On - print each char 2x (not bold!) 2nd is 1/120th" off from 1st 
                                               in WPS mode, chars have an 1/120" offset added, else width is the same (squeezed to same width)
                                               when bold is on, only the 1st char will use overstrike, shadow will be single strike only
ESC R                  1b 52                 - Shadow Print Off
ESC K a                1b 4b <a>             - Bold over Print n=number of times a=ascii(n) 0<n<5
                                               i.e. ESC K 3 - overstrike 3x
ESC M                  1b 4d                 - Bold over Print off
ESC SP                 1b 20                 - Print Character on Printwheel Position 004
ESC /                  1b 2f                 - Print Character on Printwheel Position 002
ESC I                  1b 49                 - Underline on
                                               bold will cause Nx underline (depending on bold mode number 1-4)
                                               tabbed over spaces will be underlined
                                               moving left past starting point of underline is undefined behavior
ESC J                  1b 4a                 - Underline off
ESC N                  1b 50                 - No carriage movement after printing next character (overstrike?) "ESC N a _" will print underlined 'a'
                                               need to set a flag to override auto right movement, which is turned off after each char is printed and
                                               the movement of the head in X to the right is completed (or suppressed)
                                               This cmd is affected by ESC 3 (1/60" graphics mode on), ESC G (1/120") graphics on, and ESC 4 (graphics off)
ESC C a1 a0            1b 43 <a1> <a0>       - Absolute Horizontal Tab, 0<n<160   a1=asc(hex(p/10)) a0=asc(hex(p mod 10))
                                               i.e. ESC C D 0 - location 130 (column width dependent)
ESC HT a               1b 09 <a>             - Absolute Horizontal Tab alternate 0<=n<126, a=ascii(n+1), "ESC HT LF" - tabs to column 10
                                               ESC HT 8 - tab to column 55
ESC H a2 a1 a0         1b 48 <a2> <a1> <a0>  - Relative Horizontal Motion 0<=n<1585
                                                 a2=asc(64+int(n/256)) if moving right, a2=asc(80+int(n/256)) if moving left
                                                 a1=asc(64+int(n mod 256)/16)
                                                 a0=asc(64+int(n mod 16))
                                               i.e. "ESC H @ @ C" - spaces right 3/120"
                                                    "ESC H R M @" - spaces left 6" (720/120")
                                                    "ESC H Q I J" - 3.5" = 420
                                               commands that would cause the head to go beyond limits are ignored
ESC BS                 1b 08                 - backspace 1/120th" 
ESC P a1 a0            1b 50 <a1> <a0>       - Absolute Vertical Tab (TOF=line 0),  0<n<128   a1=asc(hex(p/10)) a0=asc(hex(p mod 10))
                                               i.e. "ESC P C 7" - tab down to line 127 (maximum)
                                               moves up or down to the line, top is 00. Line 117 is "ESC P B 7"
ESC VT a               1b 0b <a>             - Absolute Vertical Tab Alternative 0<=n<126, a=ascii(n+1)
                                               i.e. "ESC VT !" - tabs to line 32
ESC V a2 a1 a0         1b 56 <a2> <a1> <a0>  - Relative Vertical Motion 0<=n<179 n/48" units
                                                 a2=asc(64+int(n/256)) if moving right, a2=asc(80+int(n/256)) if moving left
                                                 a1=asc(64+int(n mod 256)/16)
                                                 a0=asc(64+int(n mod 16))
                                               i.e. "ESC V @ @ A" - spaces down (paper up) 1/48"
                                                    "ESC V Q B @" - spaces up (paper down) 6" (288/48")
ESC LF                 1b 0a                 - Negative Line Feed (reverse line feed move back up the paper) 1/6" or 1/8"
ESC U                  1b 55                 - 1/2 line feed int(linefeed_units/2) - rounded to next lower1/48"
ESC D                  1b 44                 - 1/2 line feed up (negative/reverse, linefeed_units/2)
ESC SO                 1b 0e                 - shift to program mode - ESC SO a1 b1 a2 b2 a3 b3...an bn.  see page 56
                                               need to implement table from Appendix C in the emulator for this.
                                               a=ascii char to print, b=hammer intensity+spacing
ESC #                  1b 23                 - Enter Secondary Program Mode a1,b1,pl
                                               See Appendix F, this sets hammer intensity, ribbon advance (can skip), an as per table there, so need
                                               to store the table in the emulation. see page 56
ESC SI                 1b 0f                 - Return to Normal Mode, deselects program mode, user test mode
ESC 3                  1b 33                 - Graphics On: 1/60" horizontal spacing, 1/48" vertical spacing: auto CR/LF ignored in graphics modes, 
                                               CR deselects graphics mode. SPACE/BACKSPACE move head 1/60th" left/right, LF, reverse LF move head 1/48th" up/down
                                               ESC 4, CR, ESC SUB I (reset), power turn off graphics mode
ESC G                  1b 47                 - Graphics On: 1/120" horizontal spacing, 1/48" vertical spacing: auto CR/LF ignored in graphics modes, CR deselects graphics mode
ESC 4                  1b 34                 - Graphics Mode Off
ESC SUB ENQ            1b 1a 05              - Status Request
                                               ESC : [n] is the reply.
                                               bit 1=1 successful self test
                                               bit 2=0 standard character sequence selected
                                               bit 3=0 no operator check
                                               bit 4=0 printer not in CHECK
                                               bit 5=0 auto line feed not selected
                                               bit 6=0 printer idle
                                               bit 7=1 always                 
ESC SUB SO                                   - do self test and then send status back                       
ESC : status           1b 3a <status>        - Status Reply (what is it?)
ESC @ T                                      - User test mode - upto 223 chars printer, end with EOT, NUL erases last char
                                               STX - run test once
                                               SOH - run continously
                                               ENQ - halt test
                                               leave test ESC SI
US d                   1f d                  - Program Mode Carriage Command
                                               move either left or right between 1/120" and 63/120" d is ASCII char table 5, appendix C
                                               i.e. "US VT" means 12/120"
LF                     0a                    - Line Feed advance 1/6" or 1/8th" (DIP SW)
FF                     0c                    - Form Feed - turns off underline mode
CR                     0d                    - carriage return, deselects graphic modes
SP                     20                    - Space
BS                     08                    - Backspace
HT                     09                    - Horizontal Tab 
SO                     0e                    - Shift Out - change character set to extended character codes
SI                     0f                    - Shift In - change character set to standard 94 char sequence
BELL                   07                    - causes printer to beep
XON DC1                11                    - printer sends these in XON/XOFF handshaking mode - tell host to resume sending
XOFF DC3               13                    - printer sends these in XON/XOFF handshaking mode - tell host to pause sending (buffer almost full 64 chars away)
ETX                    03                    - End of Text - host must send ETX as last char, then if sent by host, printer will send ACK back to host
ACK                    06                    - Acknowledge buffer empty - when done printing current buffer, printer sends back ACK if received ETX as last char


Status Bits:  Normal status is just "@" page 79

???   0=unsuccessful or just initialized, what's a 1?
b2    bit 2 not used
b3    1=operator attention required
b4    1=printer in check
b5    1=auto line feed selected, 0=not
b6    1=printer busy
b7    always 1


*/