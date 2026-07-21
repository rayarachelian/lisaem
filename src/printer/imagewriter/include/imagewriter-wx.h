/**************************************************************************************\
*                                                                                      *
*                           Apple ImageWriter I Emulator.                              *
*                                 wxWidgets Edition                                    *
*                                                                                      *
*                       A Part of the Lisa Emulator Project                            *
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
****************************************************************************************
*                                                                                      *
*       Apple, the Apple logo, Lisa, Macintosh, ImageWriter, and Moof are trademarks   *
*        of Apple, Inc., registered in the U.S. and other countries.                   *
*                                                                                      *
*  PostScript is a trademark of Adobe Systems Incorporated which may be registered in  *
*                            certain juristictions.                                    *
*                                                                                      *
*  All other names mentioned herein may or may not be registered or unregistered       *
*                  trademarks held by their respective owners.                         *
*                                                                                      *
*                   Time is an illusion, lunch-time, doubly, so.                       *
*                                                                                      *
****************************************************************************************
*                                                                                      *
*   This program emulates an ImageWriter I printer and decodes all printed text and    *
*   graphics to a grayscale bit-mapped array which can then be printed to wxWidgets    *
*   DC's such as printers and window frames, or saved as images such as PNG's.         *
*                                                                                      *
\**************************************************************************************/

#define IWEMULATED 1

#define IW_TEXTMODE 0
#define IW_TEXT_ESC 1
#define IW_TEXT_CTRLA 2
#define IW_TEXT_CTRLUL 3
#define IW_GRAPHMODE 99
#define IW_TEXT_0X1D 0x1d

#define IW_REPEAT100 254 /* repeat 100's place */
#define IW_REPEAT10 253  /* repeat 10's  place */
#define IW_REPEAT1 252   /* repeat 1's   place */

#include <wx/metafile.h>
#include <wx/print.h>
#include <wx/printdlg.h>

class ImageWriter
{

public:
   int8 dipsw1; // Defaults for ImageWriter DIP switches.
   int8 dipsw2;
   uint32 iwid; // imagewriter id - for emulator use

   int8 iw_malloc(void);
   void iw_initialize(int full = 1);
   void EndDocument(void);
   ImageWriter(wxWindow *parent, int outputtype = 0, wxString outfname = _T(""), int dip1 = 210, float paperx = 8.5, float papery = 11.0);
   ~ImageWriter(void);

   void iw_destroy(void);

   void ImageWriterLoop(uint8 c);
   void iw_clear_page(void);
   void iw_formfeed(void);
   void iw_spitout_wx(void);

   void test(void);

private:
   wxWindow *parentwindow;
   wxPrintDialog *printdialog;
   wxDC *printerDC;
   long printer_y; // not sure I need th

   int desttype;
   wxString destination;
   wxColour *graylevel[16];
   wxPen *pen[16];

   uint8 mode;
   uint8 lastseq;   // last escape sequence.
   uint16 submode;  // submode used to track ESC sequences.
   uint16 submode1; // submode used to track ESC sequences.
   uint16 submode2; // submode used to track ESC sequences.
   uint16 submode3; // submode used to track ESC sequences.
   uint16 submode4; // submode used to track ESC sequences.

   int8 charwidth; // this is set to 0 for condensed, 1 for normal, 2 for double wide.
   int8 bold;      // 1 sets boldness.
   int8 headline;  // 1 sets (doublewide bold) headline mode.
   int8 underline; // 1 sets underline.

   int8 bidirectional; //
   int8 autoCRoff;     // 1 disables auto CR on linefeed.
   int8 autoLF;        // 1 sets an auto LF after a CR.
   int8 linefeedpitch; // 24=6 lines per feed, 18=8 lines per feed. (144/18 or 144/24) default is 24.

   int8 linefeedreverse; // 0 normal line feed down, 1 feed up

   int8 slashzero;         // 1 sets slashedzeros.
   int8 propspacing;       // Sets the number of pixels between proportional chars.
   int8 LFVTFFareCR;       // Do Line Feeds, Vertical Tabs, Form Feeds act as CR as well?
   int8 AutoLFonFull;      // AutoLF when buffer is full? (Not implemented here since we don't use any buffers at all...)
   int8 elitespacing;      // elite proportional font spacing.
   int16 repeatchar;       // how many times to print a character.  * set to 1 when done *
   int8 fontnumber;        // current font number.
   int8 countryfont;       // current font number.
   int16 leftmargin;       // measured in cpi based on current font. For proportional: if elite=10cpi, if pica=9cpi
   int16 tabs[32];         // tab positions.
   int8 currentform;       // current form number (0-4)
   int8 vtabs[5][96][6];   // upto 96 vertical tabs can be set on upto 5 pages. Each vtab contains one of 5 tabs (BCDEF)
   int8 currentformnumber; // which of the 6 forms am I on?
                           // IW will jump up or down to the next vtab.

   int16 currentCPI;     // Current Character Per Inch setting (in pixels for tabs.)
   int16 currentVCPI;    // Vertical Character CPI
   int16 currentVCPIdiv; // Vertical Character CPI divisor (depends on HDPI)

   // this is the user programable font buffer.  It's really 175 chars, controls are ignored, etc.  To make life
   // easier I set this to 255, it's a bit wasteful, yes, but saves checking code.

   int16 progfont[256][19]; // programable fonts (95-8*16 or 175-8*8) 175 chars max 16 lines each
   int16 progfntwidth[175]; // what's the width of each char?
   int16 proglastwidth;     // what was the last character width sent to us?

   // this changes as fonts are switch.  This is the actual font used to print.
   int16 livefont[256][19]; // live font (95-8*16 or 175-8*8) 175 chars max 16 lines each
   int16 livefntwidth[256]; // what's the width of each char?

   int8 progfnoffset;      // 0 print on last bottom wires, 1 print on top wires
   int8 fourteeninchpaper; // zero if 11", non-zero: 14"

   int16 cursor;    // current X cursor position of the print head in pixels.
   int16 line;      // current Y cursor pos in lines (depends on font and pitch)
   int16 linepixel; // current Y pos in pixels

   // wxBitmap   *page;           // this holds the memory for a given page - was page, renamed to
   // wxMemoryDC *pageDC;         // ensure I catch any code using the wrong type.

   uint32 pagenum;  // this holds the page number in bits.
   uint8 *page;     // this holds the memory for a given page.
   uint32 pagesize; // this holds the page size in bytes.

   int owidth, width, height; // paper size in pixels. width is padded as per BMP rules (to next 32 bit word.)
                              // owdith is the actual
   long iwwidth, iwheight;    // size of paper in imagewriter pixels (i.e. 72dpi)

   uint8 outputps, outputpcl, // What kind of output do we create?
       outputxlib, xlib_skip;

   uint8 landscape; // print out landscape if printer supports it?

   int isblank; // is the page blank

   int debug;

   // Command line options...
   uint32 iw_def_dpi, iw_max_hdpi, iw_max_wdpi;
   double iw_paper_w, iw_paper_h;
   uint32 iw_max_x, iw_max_y, iw_bitsperpixel, iw_bytes_per_line;

   float scalex, scaley;               // Scaling factors for output
   uint32 outmaxx, outmaxy, outputdpi; // Maximum size of output page, output page DPI.

   int pagefd;

   uint16 iw_get_ypix(uint16 line);
   uint8 iw_pixel_color(uint16 x, uint16 y);
   uint8 iw_pixel_iwcolor(uint16 x, uint16 y);
   void iw_plot_inc(int16 x, int16 y);
   void iw_plot(int16 x, int16 y, uint8 color);
   void iw_plot_pin(int16 x, int16 y, int c = 1);

   void iw_cr(void);
   void iw_lf(void);

   void iw_printbar(uint8 c, uint8 lower);
   void iw_reset_vtabs(void);
   void iw_set_vline_pos(uint16 line);
   void iw_printchar(uint8 c);
   void iw_settopofform(void);
   void iw_reset_page(void);

   uint16 *xlens;
   uint16 *ylens;
};
