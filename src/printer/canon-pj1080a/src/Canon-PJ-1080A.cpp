/**************************************************************************************\
*                                                                                      *
*                          Canon PJ-1080A InkJet Emulator                              *
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


// Load the fonts up...
#include "iwfonts.h"


// Most OS's have this defined, for the ones that don't, 1024 is a reasonable value
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* From: http://lisa.sunder.net/Canon%20PJ-8010A%20Color%20Ink-Jet%20Printer%20Operations%20Manual.pdf
DIP Switches
1-3 Country Code: 000-USA, 100-French, 010-German, 110-British, 001-Danish, 101-Sweedish, 011-Italian, 111-Japanese
4-  Line Feed/CR: 1=needs CR/LF, 0=just LF
5-  1" Perforation Skip, 0=1" skip, 1=off
6-  Solarize (inverted?), 0=standard, 1=inverse
ext Bold

Color Codes:
    Code Color    RGB  - Notes      Mix
----------------------------------------
    0x30 Black    000  - direct     K
    0x31 Red      100  - compound   Y+M
    0x32 Green    010  - compound   Y+C
    0x33 Yellow   110  - direct     Y
    0x34 Blue     001  - compound   M+C
    0x35 Magenta  101  - direct     M
    0x36 Cyan     011  - direct     C 
    0x37 White    111  - no output  .

Colors are substractive, i.e. white=0xffffff, when a dot is set, we substract that color, 2x makes it darker.
need to invert the actual color, i.e. C,Y,M from RGB and substract it from white or whatever is on the canvas.
bold doubles the substraction
note: "solarize" is inverse, take care with that

Nozzle is 0.065mm wide - is this the dot size?
font: 5x7
1.5mm W x 2.7mm H standard char
3.0mm W x 2.7mm H wide char
vertical 8 dot, 560 dots/line - image
horizontal 8 dot, 640 dots/line
pitch: 2.12mm (1/12") H X 4.23mm (1/6") V
buffer: 40 or 80 chars, image: 560 bytes x 4 colors, color: 640 bytes x 4 colors 
paper: max 216mm (8.25"), vertical max: 300mm or less

CR                      - 0x0d                      - unless LF is turned on, goes back to the same line
                                                      CR+LF will cancel enlarged mode set with SO code
CAN                     - 0x18                      - discard line buffer
LF                      - 0x0A                      - prints all data in buffer, moves paper up, does not go back to x=0,cancels enlarged mode, color codes set remain the same
                                                    - LF rate can be set with ESC-0 or ESC=2
FF                      - 0x0c                      - Form Feed - print all data in buffer, push to next page (clear page to all white)
HT                      - 0x09                      - Horizontal tab - ignored if tabs not set
VT                      - 0x0b                      - Vertical tab - print all data in buffer, move printhead down to next position, cancels enlarged mode
SO                      - 0x0e                      - Shift-Out: Print enlarged (40 col mode), cancelled after a line feed, etc. - 2X 1Y, note ESC-SO as well
DC1                     - 0x11                      - Printer Select - allows receiving of data from LisaEm
DC3                     - 0x13                      - Printer DeSelect - won't accept data from LisaEm until Online (or DC1 is received)
DC4                     - 0x14                      - Cancel Enlarged Mode, unless set with ESC "W" or ESC "!"
                                                      
ESC-0                   - 0x1B 0x30                 - 1/8th" vertical Line Spacing.
ESC-2                   - 0x18 0x32                 - 1/6th" vertical Line Spacing
ESC-C(NUL)+N            - 0x1b 0x43 0x00 N          - Set Page Length by inches - default is N=11 at power on
ESC-C+N                 - 0x1b 0x43 N               - Set Page Length by lines  - N*line spacing (1/6th or 1/8th) - default set to 66 lines on poweron
ESC-@                                               - Set top of page (maybe same as FF?)
ESC-Nn                  - 0x1b 0x4e N               - Set perforation skip lines (see dipsw 5). ESC "C"+n or ESC "C"+0+n cancels it
ESC-O                   - 0x1b 0x4f                 - Cancel perforation skip function
ESC-D+n1..nk+0          - 0x1b 0x44 n,n,n,...,0     - Set horizontal tab positions, position is current character width * N (any number of N's - I guess to buffer size)
                                                      default: every 8 chars, absolute positions are set, not changed even if print mode is changed
ESC B+n1..nk+0          - 0x1b 0x32 n,n,n,...,0     - Set vertical tab positions, position is current line spacing * N (any number of N's - I guess to buffer size)
                                                      n,n,n...,0must be in ascending order, rest discarded
ESC V n                 - 0x1b 0x56 n               - Set color - any other value is ignored, default black
                                                      Color Codes:
                                                          Code Color    RGB  - Notes      Mix
                                                          ------------------------------------
                                                          0x30 Black    000  - direct     K
                                                          0x31 Red      100  - compound   Y+M
                                                          0x32 Green    010  - compound   Y+C
                                                          0x33 Yellow   110  - direct     Y
                                                          0x34 Blue     001  - compound   M+C
                                                          0x35 Magenta  101  - direct     M
                                                          0x36 Cyan     011  - direct     C 
                                                          0x37 White    111  - no output  .
ESC g n                 - 0x1b 0x67 n               - Set background color - any other value is ignored. Default white.
ESC d n                 - 0x1b 0x64 n               - Set solarized mode when n=1, else disable it
ESC SO                  - 0x1b 0x0e                 - Set Enlarged Mode with auto cancel - same as just SO
ESC W n                 - 0x1b 0x57 n=1             - Set Enlarged mode when n=1, no auto cancel after form feed!
ESC ! n                 - 0x1b 0x21 n=0x20          - Set Enlarged mode when n=0x20, same as ESC W 1
ESC G                   - 0x1b 0x47                 - Set Bold Mode
ESC H                   - 0x1b 0x48                 - Cancel Bold Mode - also see dip sw 7
ESC - n                 - 0x1b 0x2D n               - Set Underline Mode when n=1, cancelled when n=0 (underline is under lowest pixel in font), so bit 8?
ESC K s1 s2 N[size]     - 0x1b 0x4b s1 s2 N[size]   - Set graphics mode s1=LSB size, s2=MSB size (little endian), this is then followed by N bytes of vertical binary data
                                                      bit 0 at the top, bit 7 at the bottom.  Q: does it print the background color for zero bits of N data?
ESC * m n1 n2           - 0x1b 0x2a m s1 s2 N[size] - Set Graphic Image Mode - same as ESC K + n1 + n2 when m=0, m can only be set to zero, all other values ignored
ESC X n                 - 0x1b 0x58 n r,g,b,r,g,b.. - Color Graphics Mode - prints in the horizontal direction, note colors are different!
                                                      also see ESC r for repeat, this code must be executed in pairs. wtf!
                                                                  RGB - note invert for Solarize/Inverse mode (does solarize work on color or just black & white?)
                                                          Black   000
                                                          Red     100
                                                          Green   010
                                                          Blue    001
                                                          Cyan    011
                                                          Yellow  110
                                                          Magenta 101
                                                          White   111
                                                      ESC x n red_data green_data blue_data, nothing printed if n=0, if n=1, need 3 bytes of data, etc.
                                                      after 3 bytes, next will print on the line below?
                                                      if n=0, we go down a line?
ESC r s1 s2               - 0x1b 0x72 s1 s2 N[size] - Color Graphics Image Repeat - s2 * 3 (rgb) bytes are printed on n1 dot lines, so s2, s1 are split?
                                                      when s1=0 same as ESC X+n code,
                                                      After executing, returns to previous mode                            
ESC e n                   - 0x1b 0x65 n             - Skip n dot lines, no paper feed when n=0, printer returned to previous mode
ESC @                     - 0x1b 0x40               - Printer Reset, returns to all defaults (should also do FF in LisaEm)


Unlike for CRT printing, we don't add R+G+B, instead we substract from white as the inkjet ink is laid down. So in order to caclulate
colors, we substract from paper color's RGB, this is 0x00ffffff by default. For normal we substract 1/3rd, for bold we substract half.
i.e. for yellow we substract R,G,B 0x00aaaa00 from 0x00ffffff for bold, or 0x00555500 for normal, but watch out for overflows.

Have to decide if we want round dots, or just use wxWidgets pixels as is. For the ImageWriter we used round pixels to plot gray.
Do we want to slightly offset some colors so as to have a bleed through effect like on a real printer, if a real printer does that?
have to zoom in with a microscope to see for sure on a real printout.

*/


#include <PJ1080A.hpp>
// move this to the above include
class PJ1080A {
public:

    int8 dipsw1;                // Defaults for Canon PJ-1080A DIP switches.
    int8 boldsw;                // discrete bold switch
    uint32 pjid;                // PJ id - if we have more than one

    int8 pj_malloc(void);
    void pj_initialize(int full=1);
    void EndDocument(void);
    PJ1080A(wxWindow *parent, int outputtype=0, wxString outfname=_T(""), int dip1=210, float paperx=8.5, float papery=11.0);
    ~PJ1080A(void);

    void pj_destroy(void);

    void PJ1080ALoop( uint8 c);
    void pj_clear_page(void);
    void pj_formfeed(void);
    void pj_spitout_wx(void);

    void test(void);

private:
    wxWindow      *parentwindow;
    wxPrintDialog *printdialog;
    wxDC          *printerDC;
    long           printer_y;           // not sure I need th

    int desttype;
    wxString destination;
    wxColour *graylevel[16];
    wxPen    *pen[16];


    uint8  mode;
    uint8  lastseq;               // last escape sequence.
    uint16 submode;               // submode used to track ESC sequences.
    uint16 submode1;              // submode used to track ESC sequences.
    uint16 submode2;              // submode used to track ESC sequences.
    uint16 submode3;              // submode used to track ESC sequences.
    uint16 submode4;              // submode used to track ESC sequences.

    int8  charwidth;             // this is set to 0 for condensed, 1 for normal, 2 for double wide.
    int8  bold;                  // 1 sets boldness.
    int8  headline;              // 1 sets (doublewide bold) headline mode.
    int8  underline;             // 1 sets underline.

    int8  bidirectional;         //
    int8  autoCRoff;             // 1 disables auto CR on linefeed.
    int8  autoLF;                // 1 sets an auto LF after a CR.
    int8  linefeedpitch;         // 24=6 lines per feed, 18=8 lines per feed. (144/18 or 144/24) default is 24.

    int8  linefeedreverse;       // 0 normal line feed down, 1 feed up

    int8  slashzero;             // 1 sets slashedzeros.
    int8  propspacing;           // Sets the number of pixels between proportional chars.
    int8  LFVTFFareCR;           // Do Line Feeds, Vertical Tabs, Form Feeds act as CR as well?
    int8  AutoLFonFull;          // AutoLF when buffer is full? (Not implemented here since we don't use any buffers at all...)
    int8  elitespacing;          // elite proportional font spacing.
    int16 repeatchar;            // how many times to print a character.  * set to 1 when done *
    int8  fontnumber;            // current font number.
    int8  countryfont;           // current font number.
    int16 leftmargin;            // measured in cpi based on current font. For proportional: if elite=10cpi, if pica=9cpi
    int16 tabs[32];              // tab positions.
    int8  currentform;           // current form number (0-4)
    int8  vtabs[5][96][6];       // upto 96 vertical tabs can be set on upto 5 pages. Each vtab contains one of 5 tabs (BCDEF)
    int8  currentformnumber;     // which of the 6 forms am I on?
                              // IW will jump up or down to the next vtab.

    int16 currentCPI;            // Current Character Per Inch setting (in pixels for tabs.)
    int16 currentVCPI;           // Vertical Character CPI
    int16 currentVCPIdiv;        // Vertical Character CPI divisor (depends on HDPI)

 // this is the user programable font buffer.  It's really 175 chars, controls are ignored, etc.  To make life
 // easier I set this to 255, it's a bit wasteful, yes, but saves checking code.

    int16 progfont[256][19];     // programable fonts (95-8*16 or 175-8*8) 175 chars max 16 lines each
    int16 progfntwidth[175];     // what's the width of each char?
    int16 proglastwidth;         // what was the last character width sent to us?


 // this changes as fonts are switch.  This is the actual font used to print.
    int16 livefont[256][19];     // live font (95-8*16 or 175-8*8) 175 chars max 16 lines each
    int16 livefntwidth[256];     // what's the width of each char?

    int8  progfnoffset;          // 0 print on last bottom wires, 1 print on top wires
    int8  fourteeninchpaper;     // zero if 11", non-zero: 14"

    int16 cursor;                // current X cursor position of the print head in pixels.
    int16 line;                  // current Y cursor pos in lines (depends on font and pitch)
    int16 linepixel;             // current Y pos in pixels



    //wxBitmap   *page;           // this holds the memory for a given page - was page, renamed to
    //wxMemoryDC *pageDC;         // ensure I catch any code using the wrong type.


    uint32 pagenum;              // this holds the page number in bits.
    uint8* page;                 // this holds the memory for a given page.
    uint32 pagesize;             // this holds the page size in bytes.

    int owidth, width, height;   // paper size in pixels. width is padded as per BMP rules (to next 32 bit word.)
                                 // owdith is the actual
    long iwwidth, iwheight;      // size of paper in imagewriter pixels (i.e. 72dpi)

    uint8 outputps,outputpcl,    // What kind of output do we create?
    outputxlib, xlib_skip;

    uint8 landscape;             // print out landscape if printer supports it?

    int  isblank;                // is the page blank

    int  debug;

 // Command line options...
    uint32 pj_def_dpi, pj_max_hdpi, pj_max_wdpi;
    double pj_paper_w, pj_paper_h;
    uint32 pj_max_x, pj_max_y, pj_bitsperpixel, pj_bytes_per_line;

    float   scalex, scaley;          // Scaling factors for output
    uint32  outmaxx, outmaxy, outputdpi;// Maximum size of output page, output page DPI.


    int pagefd;

    uint16 pj_get_ypix(uint16 line);
    uint8 pj_pixel_color(uint16 x, uint16 y);
    uint8 pj_pixel_iwcolor(uint16 x, uint16 y);
    void pj_plot_inc( int16 x,  int16 y);
    void pj_plot( int16 x,  int16 y, uint8 color);
    void pj_plot_pin(int16 x, int16 y, int c=1);

    void pj_cr(void);
    void pj_lf(void);

    void pj_printbar(uint8 c,uint8 lower);
    void pj_reset_vtabs(void);
    void pj_set_vline_pos(uint16 line);
    void pj_printchar( uint8 c);
    void pj_settopofform(void);
    void pj_reset_page(void);

    uint16 *xlens;
    uint16 *ylens;
};