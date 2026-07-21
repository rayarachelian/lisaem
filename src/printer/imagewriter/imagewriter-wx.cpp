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
*                   Time is an illusion, lunch-time, doubly so.                        *
*                                                                                      *
****************************************************************************************
*                                                                                      *
*   This program emulates an ImageWriter I printer and decodes all printed text and    *
*   graphics to a grayscale bit-mapped array which can then be printed to wxWidgets    *
*   DC's such as printers and window frames, or saved as images such as PNG's.         *
*                                                                                      *
\**************************************************************************************/

// needed for wxWidgets.
// #define MIRRORX 1
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
#define BYTES_HIGHFIRST 1
#endif

#ifdef wxUint64
typedef wxUint64 uint64;
#endif
typedef wxUint32 uint32;
typedef wxUint16 uint16;
typedef wxUint8 uint8;

#ifdef wxInt64
typedef wxInt64 int64;
#endif
typedef wxInt32 int32;
typedef wxInt16 int16;
typedef wxInt8 int8;

typedef int8 sint8;
typedef int16 sint16;
typedef int32 sint32;

// Load the fonts up...
#include "iwfonts.h"

// Most OS's have this defined, for the ones that don't, 1024 is a reasonable value
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/*
   DIP Switches.
   see: http://alt-www.corvis.ru/html/tech/Printers/upm/printers/mapiw2.htm
   (if you can get a good connection to it.)

   Country Font Selection: (Can be software overridden)

   DIP SWITCH Bank One.

        bit (bit 1=2^0, bit 2 is 2^1, bit 3 is 2^2)
        1,2,3     Effect.         Software Setting for the same:
        000       American Font.    ESC Z,^G,^@
        110       British  Font.    ESC Z,^D,^@,ESC D,^C,^@
        001       German   Font.    ESC Z,^C,^@,ESC D,^D,^@
        011       French   Font.    ESC Z,^A,^@,ESC D,^F,^@
        101       Sweedish Font.    ESC Z,^B,^@,ESC D,^E,^@
        100       Italian  Font.    ESC Z,^F,^@,ESC D,^A,^@
        111       Spanish  Font.    ESC D,^G,^@
        010       American Font 2.  ESC Z,^E,^@,ESC D,^B,^@

        bit 4-    72 lines (on) 66 lines (off)
        bit 5-    If on, AND all data with 127 (strip 8th bit)

        bit 6,7
        00        Elite Proportional (upto 160dpi)
        01        Elite          12cpi
        10        Ultracondensed 17cpi
        11        Pica           10cpi

        bit-8     Auto Line Feed. 1=on (auto LF after CR)     , 0=off (requires LF char)

   DIP Switch Bank Two

        bits1,2
        00       9600 bps serial connection (we ignore the connection here.)
        01       1200 bps serial connection (we ignore the connection here.)
        10       2400 bps serial connection (we ignore the connection here.)
        11       300  bps serial connection (we ignore the connection here.)


        bit3 -   handshake:  0= Xon/Xoff, 1=Hardware handshake. (We ignore this too.)

        bit4-8   ignored.

 */

#include <imagewriter-wx.h>
/* max is 160hx144v dots per inch.
 *
 * Normal: 80 dpi, condensed is 136 dpi (8" across)
 *
 * custom chars can be upto 16 dots wide.  width is defined for each char...
 * and is either proportional or fixed. a custom char can be assigned to any keyboard position
 * that normally prints something (not control chars...) including upper/lower case
 *
 * the dot striking wires in the type head are 1-9 (1 top, 9 at bottom).  When designign
 * a custom char, you can use either top 8 (1-8) or bottom 8 (2-9)... Normal alphabet uses
 * 1-7 for caps, and 3-7 for lowercase with 8.9 being reserved for descenders such as tails
 * of g,y,p,q.  Wire 9 is used for underlining too...  Normal base for chars is wire 7
 *
 * LSB is top (wire 1 or 2), MSB is bottom (wire 8 or 9)
 *
 * to load a new char: (7 dots wide) assigning to "&" char:
 *
 *  ESC-ESC | & G Control-@ H H  H H Control-@ Control-D  (sans spaces)
 *
 * G is width code, it indicates that the top 8 wires print the symbol and that it is 7 dots wide
 * (G is 7th letter of alphabet).   After you load the new char by means of the sequence above, you
 * can print this can anytime by sending:
 *
 * [ESC] ['] [&] [ESC] [$]  These 5 chars change the printer's type style to the custom font, print
 * the symbol assigned, and change back to the normal font.
 *
 *
 * Printing graphics:
 * each line can be upto 1280 dots long.
 *
 * Column oriented graphics:
 *
 * The IW can print horizontal lines with dots in any pattern, each vertical column upto 8 dots is defined
 * by a separate ascii char, so a single line may require upto 1280 chars for its definition.  After it prints
 * each line the printer is ready to receive a new line...
 *
 * Each control code defining one horizontal line of dot graphics starts with one of the following 6 char
 * prefixes:
 *
 * (hex) 1B 47 n n n n - prints line corresponding to nnnn bytes.
 *       1b 53 n n n n - same as above (identical)
 *       1b 67 n n n n - print line corresponding to teh following nnnx8 data bytes (pattern repeated 8x?)
 *
 * The nnnn's are 4 ordinary numeric keyboard chars (0-9) and specifies the number of data bytes upto 9999
 * that follow.  This 6 char prefix plus the data bytes themselves constitute the complete control code
 * which may thus run upto 10,005 chars.
 *
 * The nnn after ESC g consist of 3 keyboard numerals and specifies the number of data bytes divided by eight
 * (upto 999) that follow.  ESC-g's are somewhat faster than ESC-S/ESC-G's. but otherwise is the same...
 * i.e. ESC g 010 and ESC G 0080 are equivalent.    *** In these and other ESC sequences you can replace the
 * leading 0's with spaces!
 *
 * Each of the data bytes defines a vertical column of eight dots printed by the type head.  A dot is printed
 * for each bit set to 1 in the data byte.  Bit 7 causes the dot at the bottom of the column to be printed, bit 0
 * causes the top to be printed.  i.e.
 *
 *  bit value
 *  1 0 * 1 lsbit
 *  2 1 * 1
 *  4 2   0
 *  8 3 * 1
 * 16 4   0
 * 32 5   0
 * 64 6 * 1
 *128 7 * 1 msbit
 *
 * The columns print from left to right starting at the left margin of the page.
 *
 * Dot Spacing ***
 *
 * The vertical spacing of the striker wires in the type of head is 1/72 of an inch.  The horizontal character
 * pitch that produces 72 dpi is extended, or(huh?) 9 chars per inch.  So if you select a vertical line feed
 * pitch of 16/144 of an inch, the result is a uniform matrix of dot positions -- 72 per inch in each direction
 * with a total density of 5184 dpi.  The dot size is such that the horz/vert lines appear connected for this
 * matrix... to produce 9 chars per inch horizontally and a lf pitch of 16/144 use:
 *
 *  ESC n ESC T16
 *
 * Although you can select finer scale horizontal dot spacing, it takes longer to print...
 *
 * (need to document big huge ESC code table...)
 *
 */

#define CSTR(x) ((char *)(x.char_str()))
#define cSTR(x) ((char *)(x->char_str()))

#define IW_AUTO_CR_AT_END 1

#define DIW_DEF_DPI (72)   /* Default DPI of the PS interpreter!  */
#define DIW_MAX_HDPI (74)  // 76 is good, 74 is perfect, 72 doesn't quite work //20070317//was144!  /* Our image's DPI heigh               */
#define DIW_MAX_WDPI (160) /* Our image's DPI width               */
#define DIW_PAPER_W (8.5)  /* paper size                          */
#define DIW_PAPER_H (11.0)
#define DIW_MAX_X (DIW_MAX_WDPI * DIW_PAPER_W)
#define DIW_MAX_Y (DIW_MAX_HDPI * DIW_PAPER_H)
#define DIW_OUT_HDPI (300.0)
#define DIW_OUT_WDPI (300.0)
#define DIW_MAX_OX (DIW_OUT_WDPI * DIW_PAPER_W)
#define DIW_MAX_OY (DIW_OUT_HDPI * DIW_PAPER_H)
#define DIW_BITS_PER_PIXEL (4) // this is for future expansion to say, color, or IW LQ emulation
                               // If you change this, you'll have to change the plot/pixel color  functions
                               // bytes/hline is rounded up to the next 32 bit word as per BMP rules

// #define DIW_BYTES_PER_HLINE ((uint32)(DIW_MAX_OX/2) + (3^((uint32)(DIW_MAX_OX/2)&3)) )
#define DIW_BYTES_PER_HLINE ((uint32)(DIW_MAX_OX / 2)) // + (3^((uint32)(DIW_MAX_OX/2)&3)) )
#define DIW_BYTES_PER_PAGE ((iw_bytes_per_line) * DIW_MAX_OY)

// These used to be defines, but they're better off as variables.

#define IW_DEF_DPI (iw_def_dpi)
#define IW_MAX_HDPI (iw_max_hdpi)
#define IW_MAX_WDPI (iw_max_wdpi)
#define IW_PAPER_W (iw_paper_w)
#define IW_PAPER_H (iw_paper_h)
#define IW_MAX_X (iw_max_x)
#define IW_MAX_Y (iw_max_y)
#define IW_BITS_PER_PIXEL (iw_bitsperpixel)
#define IW_BYTES_PER_HLINE (iw_bytes_per_line)
/*
 if (scalex==0.0 || xlens==NULL || ylens==NULL)
    {
     uint16 i;
     // DIW_OUT_WDPI=300 ,H=300.

    iw_max_hdpi       = DIW_MAX_HDPI        ;
    iw_max_wdpi       = DIW_MAX_WDPI        ;

     scalex=((float)(DIW_OUT_WDPI)/(float)(iw_max_wdpi));
     scaley=((float)(DIW_OUT_HDPI)/(float)(iw_max_hdpi));  // 300/72

     if (xlens==NULL) xlens=new uint16[IW_MAX_X];//(uint16 *)malloc(IW_MAX_X*sizeof(uint16));
        if (ylens==NULL) ylens=new uint16[IW_MAX_Y]; //(uint16 *)malloc(IW_MAX_Y*sizeof(uint16));

     for (i=0; i<IW_MAX_X; i++) xlens[i]=(uint16)(  (float)(i) * scalex );
     for (i=0; i<IW_MAX_Y; i++) ylens[i]=(uint16)(  (float)(i) * scaley );

*/

uint16 ImageWriter::iw_get_ypix(uint16 line) // need to correct this for current spacing.
{
    if (currentVCPIdiv == 0)
        currentVCPIdiv = 2;
    return (currentVCPI * line / currentVCPIdiv);
}

void ImageWriter::iw_cr(void)
{
    // if (debug) fprintf(stderr,"IW-CR lmargin:%d,ypixel:%d,yline:%d\n",leftmargin,linepixel,line);
    cursor = leftmargin;
}

// protos

uint8 iw_pixel_iwcolor(uint16 x, uint16 y);

void ImageWriter::iw_clear_page(void)
{
    if (page)
    {
        memset(page, 0, pagesize);
        isblank = 1;
    }
    return;
}

// This sends a scaled version of pixel color back to the caller.
uint8 ImageWriter::iw_pixel_color(uint16 x, uint16 y)
{
    if (x > width || y > height)
        return 255;

    uint16 sx, sy;

    if (scalex == 0.0 || xlens == NULL || ylens == NULL)
        return 255;
#ifdef MIRRORX
    x = width - x;
#endif
#ifdef MIRRORY
    y = height - y;
#endif

    sx = xlens[x];
    sy = ylens[y];

    return (
               iw_pixel_iwcolor(sx + 1, sy - 1) +
               iw_pixel_iwcolor(sx, sy - 1) +
               iw_pixel_iwcolor(sx - 1, sy - 1) +

               iw_pixel_iwcolor(sx + 1, sy + 1) +
               iw_pixel_iwcolor(sx, sy + 1) +
               iw_pixel_iwcolor(sx - 1, sy + 1) +

               iw_pixel_iwcolor(sx + 1, sy) +
               iw_pixel_iwcolor(sx, sy) +
               iw_pixel_iwcolor(sx - 1, sy) +

               iw_pixel_iwcolor(sx, sy) // raise the weight of the center pixel
               ) /
           10;
}

// Used to check the color of a pixel by other software.
uint8 ImageWriter::iw_pixel_iwcolor(uint16 x, uint16 y)
{
    uint16 color; // xbit, yline;
    uint32 pixbyte;

    if (x >= DIW_MAX_OX || y >= DIW_MAX_OY)
        return 255;
    if (scalex == 0.0 || xlens == NULL || ylens == NULL)
        return 255;

#ifdef MIRRORX
    x = width - x;
#endif
#ifdef MIRRORY
    y = height - y;
#endif

    pixbyte = (x >> 1) + (y * iw_bytes_per_line); // two pixels for each byte, so divide by two.

    if (x & 1)
    {
        color = (page[pixbyte] & 0x0f);
    }
    else
    {
        color = (page[pixbyte] & 0xf0) >> 4;
    }
    return color;
}

/* This should be the most used method.  When called it increases the
   color level of a pixels. Since the IW prints bold by printing twice over the
   same pixel, this will do the trick.  */

void ImageWriter::iw_plot_pin(int16 x, int16 y, int c)
{
    uint16 color; // yline, color,xbit;
    int32 pixbyte;

    if (x > width || y > height || page == NULL || x < 0 || y < 0)
    {
        if (debug)
            fprintf(stderr, "iw_plot_pin: out of range or null page:x,y(%d,%d) width,height(%ld,%ld), page:%p\n",
                    x, y, iwwidth, iwheight, page);

        return;
    }

    if (isblank && debug)
    {
        fprintf(stderr, "\n\niw_plot_pin: First plot on page x,y(%d,%d) width,height(%d,%d), page:%p\n\n",
                x, y, width, height, page);
        fflush(stderr);
    }
    isblank = 0;

    // mirror fixes
#ifdef MIRRORX
    x = width - x;
#endif
#ifdef MIRRORY
    y = height - y;
#endif

    pixbyte = (x + ((y * width) >> 1)); // 20190825 was     pixbyte=(x+(y*width)>>1);
    if ((uint32)pixbyte > pagesize || pixbyte < 0)
    {
        fprintf(stderr, "iw_plot_pin: pixbyte out of range or null page:x,y(%d,%d) width,height(%ld,%ld), pixbyte:%ld,pagesize:%ld,page:%p\n",
                x, y, iwwidth, iwheight, (long)pixbyte, (long)pagesize, page);

        return;
    }

    // fprintf(stderr,"iw_plot_pin: x,y:(%d,%d) pixbyte:%d iw_bytes_per_line:%d\n",x,y,pixbyte,iw_bytes_per_line);

    if (x & 1)
    {
        color = (page[pixbyte] & 0x0f) + c;
        if (color > 15)
            color = 15;
        page[pixbyte] = (page[pixbyte] & 0xf0) | color;
    }
    else
    {
        color = ((page[pixbyte] & 0xf0) >> 4) + c;
        if (color > 15)
            color = 15;
        page[pixbyte] = (page[pixbyte] & 0x0f) | (color << 4);
    }
    return;
}

void ImageWriter::iw_plot_inc(int16 x, int16 y)
{

    if (y > iwheight)
    {
        iw_formfeed();
        return;
    } // 20071110 - overflow onto a new page

    if (x > iwwidth || y > iwheight || x < 0 || y < 0)
    { // if (debug)
        fprintf(stderr, "OUT OF RANGE PLOTINC %d,%d  max:%ld,%ld\n", x, y, iwwidth, iwheight);
        return;
    }

    if (xlens == NULL || ylens == NULL)
    {
        if (debug)
            fprintf(stderr, "xlens or ylens is null!\n");
        return;
    }

    // if (ylens[y+1]-ylens[y]>1) fprintf(stderr,"Warning: ylens %d high gap found at ylens[%d]=%d ylens[%d]=%d\n",
    //                                            ylens[y+1]-ylens[y],  y, ylens[y], y+1, ylens[y+1]);

    x = xlens[x];
    y = ylens[y];

    // iw_plot_pin(x  ,y-3,1);
    // iw_plot_pin(x+1,y-3,1);
    // iw_plot_pin(x-1,y-3,1);
    // iw_plot_pin(x  ,y+3,1);
    // iw_plot_pin(x+1,y+3,1);
    // iw_plot_pin(x-1,y+3,1);
    iw_plot_pin(x, y - 2, 2);
    iw_plot_pin(x + 1, y - 2, 1);
    iw_plot_pin(x + 2, y - 2, 1);
    // iw_plot_pin(x+3,y-2,1);
    iw_plot_pin(x - 1, y - 2, 1);
    iw_plot_pin(x - 2, y - 2, 1);
    // iw_plot_pin(x-3,y-2,1);
    iw_plot_pin(x, y + 2, 2);
    iw_plot_pin(x + 1, y + 2, 1);
    iw_plot_pin(x + 2, y + 2, 1);
    // iw_plot_pin(x+3,y+2,1);
    iw_plot_pin(x - 1, y + 2, 1);
    iw_plot_pin(x - 2, y + 2, 1);
    // iw_plot_pin(x-3,y+2,1);
    iw_plot_pin(x, y, 3);
    iw_plot_pin(x + 1, y, 2);
    iw_plot_pin(x + 2, y, 1);
    // iw_plot_pin(x+3,y, 1 );
    // iw_plot_pin(x+4,y, 1 );
    iw_plot_pin(x - 1, y, 2);
    iw_plot_pin(x - 2, y, 1);
    // iw_plot_pin(x-3,y, 1 );
    // iw_plot_pin(x-4,y, 1 );
    iw_plot_pin(x, y - 1, 2);
    iw_plot_pin(x + 1, y - 1, 1);
    iw_plot_pin(x + 2, y - 1, 1);
    // iw_plot_pin(x+3,y-1,1);
    iw_plot_pin(x - 1, y - 1, 1);
    iw_plot_pin(x - 2, y - 1, 1);
    // iw_plot_pin(x-3,y-1,1);
    iw_plot_pin(x, y + 1, 2);
    iw_plot_pin(x + 1, y + 1, 1);
    iw_plot_pin(x + 2, y + 1, 1);
    // iw_plot_pin(x+3,y+1,1);
    iw_plot_pin(x - 1, y + 1, 1);
    iw_plot_pin(x - 2, y + 1, 1);
    // iw_plot_pin(x-3,y+1,1);
}

/*-----------------3/12/99 9:40PM-------------------
 * This function prints a graphic character vertically down
 * on the page based on the current cursor position, it then
 * increments the cursor position.
 * --------------------------------------------------*/

void ImageWriter::iw_printbar(uint8 c, uint8 lower)
{
    uint16 x, y;

    x = cursor;
    y = linepixel;

    // if (y>IW_MAX_Y) fprintf(stderr,"crossed over IW_MAX_Y\n");

    if (x >= IW_MAX_X || y >= IW_MAX_Y)
        return;
    if (lower > 1)
        return;

    // for (i=0,j=1; i<8; i++, j=j<<1)  if (c & j) iw_plot_inc(x,y+i+lower);

    y += lower;
    if (c & 1)
        iw_plot_inc(x, y);
    y++;
    if (c & 2)
        iw_plot_inc(x, y);
    y++;
    if (c & 4)
        iw_plot_inc(x, y);
    y++;
    if (c & 8)
        iw_plot_inc(x, y);
    y++;
    if (c & 16)
        iw_plot_inc(x, y);
    y++;
    if (c & 32)
        iw_plot_inc(x, y);
    y++;
    if (c & 64)
        iw_plot_inc(x, y);
    y++;
    if (c & 128)
        iw_plot_inc(x, y);

    cursor++;

    // maybe this shouldn't be on... not sure.
#ifdef IW_AUTO_CR_AT_END
    if ((unsigned)cursor > IW_MAX_X)
    {
        iw_cr();
        iw_lf();
    }
#else
    if ((unsigned)cursor > IW_MAX_X)
    {
        cursor = IW_MAX_X;
    }
#endif
}

#define IW_FONT_SIZE (126 - 31)

void ImageWriter::iw_formfeed(void)
{
    // fprintf(stderr,"FormFeed\n");

    if (LFVTFFareCR)
        iw_cr(); // we only set vertical position here, should we do a CR too

    if (isblank)
    {
        if (debug)
            fprintf(stderr, "Page is blank, skipping iw_spitout - page:%d cursor:%d lmargin:%d line:%d linepixel:%d\n",
                    pagenum, cursor, leftmargin, line, linepixel);
    }
    else // if (!isblank)
    {
        pagenum++;
        if (debug)
            fprintf(stderr, "creating page:%d cursor:%d lmargin:%d line:%d linepixel:%d\n",
                    pagenum, cursor, leftmargin, line, linepixel);
        iw_spitout_wx();
        if (debug)
            fprintf(stderr, "after page clear: cursor:%d line:%d\n\n", cursor, line);
    }
    cursor = leftmargin; // call to iw_cr() also sets this, not sure this is needed???

    iw_reset_page();
    return;
}

// Zap VTAB's
void ImageWriter::iw_reset_vtabs(void)
{
    int8 i, j; //,k;

    for (j = 0; j < 5; j++)
        for (i = 0; i < 96; i++)
        {
            vtabs[j][i][0] = 0;
            vtabs[j][i][1] = 0;
            vtabs[j][i][2] = 0;
            vtabs[j][i][3] = 0;
            vtabs[j][i][4] = 0;
            vtabs[j][i][5] = 0;
            if ((i % 6) == 5)
                vtabs[j][i][0] = 1; // set B vtab by default every 6 lines.
            if (i == 66 && !(dipsw1 & 16))
                vtabs[j][i][5] = 1; // set bottom of page for 66 lines
            if (i == 72 && (dipsw1 & 16))
                vtabs[j][i][5] = 1; // set bottom of page for 72 lines
        }
}

void ImageWriter::iw_lf(void)
{

    if (isblank && line == 1)
        return; // can't go backwards when already at the top of the page

    if (currentVCPIdiv == 0)
        currentVCPIdiv = 2; // avoid divide by zeros

    if (linefeedreverse) // reverse line feed?
    {
        // if (debug) printf("reverse LF!!!\n");
        if (line == 0 && !(dipsw1 & 16))
        {
            if (debug)
                fprintf(stderr, "Reverse line feed rolled over 66\n\n");
            line = 66;
            linepixel = iw_get_ypix(line);
            return;
        }
        else if (line == 0 && (dipsw1 & 16))
        {
            if (debug)
                fprintf(stderr, "Reverse line feed rolled over 77\n\n");
            line = 77;
            linepixel = iw_get_ypix(line);
            return;
        }
        else
        {
            linepixel -= (currentVCPI / currentVCPIdiv);
            line--;
        }
    }
    else // normal line feeds.
    {
        int y;

        if (debug)
            printf("IW-LF %d,%d+%d\n", leftmargin, linepixel, currentVCPI / currentVCPIdiv);

        y = (currentVCPI / currentVCPIdiv);

        linepixel += y;
        line++;

        if (linepixel > iwheight && (dipsw1 & 16)) // 71  // 1584
        {

            if (debug)
                fprintf(stdout, "LF triggering FF since line>%d isblank:%d linepixel:%d iwheight:%ld y:%d\n",
                        line, isblank, linepixel, iwheight, y);
            iw_formfeed();
            return;
        }
        if (linepixel > iwheight && !(dipsw1 & 16)) // 65
        {
            if (debug)
                fprintf(stdout, "LF triggering FF since line>%d isblank:%d linepixel:%d iwheight:%ld y:%d\n",
                        line, isblank, linepixel, iwheight, y);
            iw_formfeed();
            return;
        }
    }
}

void ImageWriter::iw_set_vline_pos(uint16 line)
{
    linepixel = iw_get_ypix(line);
}

// Print a character in the current font...
void ImageWriter::iw_printchar(uint8 c)
{
    uint16 i, j, k, l;

    if (c < 31)
        return; // ignore control chars -- if the main loop didn't process'em, we ignore'em.

    if (c == '0' && slashzero)
        c = 0; // if Slashed zero use alternate char that lives in zero.

    if (debug)
        fprintf(stdout, "Char width: %d\n", charwidth);

    for (i = 0; i < livefntwidth[c]; i++)
    {

        j = livefont[c][i];

        if (underline)
            j = j | 128; // handle underlines.
        if (j > 256)
        {
            j = j >> 1;
            k = 1;
        } // low font chars (y,j,g,q,p, etc.)
        else
            k = 0;

        if (!charwidth)
            charwidth = 1;

        for (l = 0; l < charwidth; l++)
        {
            iw_printbar(j, k); // plain
            if (bold || headline)
            {
                cursor--;
                iw_printbar(j, k);
            } // print over the same spot if bold or headline.

            // if headline, we print twice as wide.
            if (headline)
            {
                iw_printbar(j, k);
                cursor--;
                iw_printbar(j, k);
            }
        }
    }

    // handle spaces around proportional fonts.
    if (fontnumber == 7)
    {
        for (l = 0; l <= elitespacing; l++)
            iw_printbar(0, 0);
    }
    else if (propspacing)
    {
        for (l = 0; l <= propspacing; l++)
            iw_printbar(0, 0);
    }
}

void ImageWriter::test(void)
{
    char buffer[80];
    int i, l, lines, lasty = 0;

    iw_cr();

    for (lines = 0; lines < 400; lines++)
    {
        snprintf(buffer, 80, "yline=%d line #=%d height=%ld lineheight=%ld", linepixel, lines, iwheight,
                 (long)(linepixel - lasty));
        l = strlen(buffer);

        for (i = 0; i < l; i++)
            iw_printchar(buffer[i]);
        lasty = linepixel;
        iw_cr();
        iw_lf();
    }
}

void ImageWriter::iw_settopofform(void)
{
    // going to ignore this for now...
    vtabs[currentform][line][5] = 1; // ???
    fprintf(stdout, "IW_SetTopOfForm - not fully implemented!\n");
}

ImageWriter::~ImageWriter(void)
{
    if (xlens)
    {
        delete xlens;
        xlens = NULL;
    }
    if (ylens)
    {
        delete ylens;
        ylens = NULL;
    }
    if (page)
    {
        delete page;
        page = NULL;
    }
    if (printerDC)
    {
        printerDC->EndDoc();
        delete printerDC;
        printerDC = NULL;
    }
}

void ImageWriter::EndDocument(void)
{

    iw_formfeed();
    if (!printerDC)
        return;
    printerDC->EndDoc();

    delete printerDC;
    printerDC = NULL;
    if (!printdialog)
        return;
    delete printdialog;
    printdialog = NULL;
}

ImageWriter::ImageWriter(wxWindow *parent, int outputtype, wxString outfname, int dipsw1, float paperx, float papery)
{
    // int16 i,j; //,k;
    debug = 0;
    xlens = NULL;
    ylens = NULL;
    pagenum = 0;
    isblank = 1;

    parentwindow = parent;
    printdialog = (wxPrintDialog *)NULL;
    printerDC = (wxDC *)NULL;

    if ((dipsw1 & 16) && !papery)
        papery = 14.0;
    if (!paperx)
        paperx = 8.5;
    if (!papery)
        papery = 11.0;

    iw_def_dpi = DIW_DEF_DPI;
    iw_max_hdpi = DIW_MAX_HDPI;
    iw_max_wdpi = DIW_MAX_WDPI;
    iw_paper_w = paperx;
    iw_paper_h = papery;
    iw_max_x = (uint32)DIW_MAX_X;
    iw_max_y = (uint32)DIW_MAX_Y;
    iw_bitsperpixel = DIW_BITS_PER_PIXEL;
    iw_bytes_per_line = (uint32)(DIW_BYTES_PER_HLINE);

    iwwidth = (long)((float)(iw_max_wdpi)*paperx);
    iwheight = (long)((float)(iw_max_hdpi)*papery);

    owidth = (int)(paperx * DIW_OUT_WDPI);
    width = owidth + (8 - (owidth & 7));   // round up to next 32 bit boundary (*2=8 since 4 bits)
    height = (int)(papery * DIW_OUT_HDPI); //-2

    pagesize = (uint32)(width * height / 2);
    if (debug)
        fprintf(stderr, "size:%d,%d pagesize:%d paperx,y:%f,%f\n", width, height, pagesize, paperx, papery);
    fflush(stderr);

    page = new uint8[pagesize + 256]; //(uint8 *) malloc(pagesize+256);

    desttype = outputtype;
    destination = outfname;

    iw_initialize(1);
}

void ImageWriter::iw_reset_page(void)
{
    iw_clear_page();
    line = 1;      // current Y cursor pos in lines (depends on font and pitch)
    linepixel = 1; // current Y pos in pixels
}
// cursor=leftmargin;    // call to iw_cr() also sets this, not sure this is needed???

void ImageWriter::iw_initialize(int full)
{
    int i, j;

    cursor = 0; // current X cursor position of the print head in pixels.

    if (full)
        iw_reset_page();

    mode = IW_TEXTMODE;
    lastseq = 0;
    submode1 = 0;
    submode2 = 0;
    submode3 = 0;

    charwidth = 1;     // this is set to 0 for condensed, 1 for normal, 2 for double wide.
    bold = 0;          // 1 sets boldness.
    headline = 0;      // 1 sets (doublewide bold) headline mode.
    underline = 0;     // 1 sets underline.
    bidirectional = 0; //

    autoCRoff = 0;       // 1 disables auto CR on linefeed.
    linefeedreverse = 0; // 0 normal line feed down, 1 feed up

    propspacing = 0;         // Sets the number of pixels between proportional chars.  0=non prop font.
    LFVTFFareCR = 0;         // Do Line Feeds, Vertical Tabs, Form Feeds act as CR as well?
    AutoLFonFull = 0;        // AutoLF when buffer is full? (Not implemented here.)
    elitespacing = 2;        // elite proportional font spacing.
    repeatchar = 1;          // how many times to print a character.  * set to 1 when done *
    fontnumber = dipsw1 & 7; // current font number copy from dipswitch
    leftmargin = 5;          // measured in cpi based on current font. For proportional: if elite=10cpi, if pica=9cpi

    currentform = 0; // current form number (0-4)
    currentCPI = 16; // Current Character Per Inch setting (in pixels for tabs.)

    // Vertical Character CPI (divide by two to get pixels for ESC T nn and Dipswitch1,)
    currentVCPI = 18; // ((dipsw1 & 16) ? 18: 16);
    currentVCPIdiv = 2;

    fourteeninchpaper = (dipsw1 & 16);

    if ((fontnumber == 3 || fontnumber == 7) || propspacing) // proportional?
    {
        for (i = 0; i < 128; i++)
        {
            livefntwidth[i] = iwpropw[fontnumber][i];
            for (j = 0; j < 19; j++)
                livefont[i][j] = iwprop[fontnumber][i][j];
        }
    }
    else
    {
        for (i = 0; i < 128; i++)
        {
            livefntwidth[i] = 8;
            for (j = 0; j < 8; j++)
            {
                livefont[i][j] = iwfixed[fontnumber][i][j];
            }
        }
    }

    iw_reset_vtabs();

    if (scalex == 0.0 || xlens == NULL || ylens == NULL)
    {
        uint16 i;
        // DIW_OUT_WDPI=300 ,H=300.
        scalex = ((float)(DIW_OUT_WDPI) / (float)(iw_max_wdpi));
        scaley = ((float)(DIW_OUT_HDPI) / (float)(iw_max_hdpi));

        if (xlens == NULL)
            xlens = new uint16[IW_MAX_X]; //(uint16 *)malloc(IW_MAX_X*sizeof(uint16));
        if (ylens == NULL)
            ylens = new uint16[IW_MAX_Y]; //(uint16 *)malloc(IW_MAX_Y*sizeof(uint16));

        for (i = 0; i < IW_MAX_X; i++)
            xlens[i] = (uint16)((float)(i)*scalex);
        for (i = 0; i < IW_MAX_Y; i++)
            ylens[i] = (uint16)((float)(i)*scaley);

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // FILE *f=fopen("ylens.txt","wt");  long dy=0;
        // for (i=0; i<IW_MAX_Y; i++) {fprintf(f,"ylens[%d]=%d diff=%ld\n",i,ylens[i],ylens[i]-dy); dy=ylens[i];}
        // fclose(f);
        //////////////////////////////////////////////////////////////////////////////////////////////////////
    }
}

/**********************************************************************************\
* Printer State Machine.                                                           *
*                                                                                  *
* This interprets various IW commands and functions, switches between different    *
* modes of the printer.                                                            *
*                                                                                  *
* Danger, deeply ugly code here.                                                   *
*                                                                                  *
*                                                                                  *
\**********************************************************************************/
void ImageWriter::ImageWriterLoop(uint8 c)
{

    //    #ifdef DEBUG
    //     char filename[256];
    //     FILE *iwdebug;
    //    #endif

    char c2;
    int16 page = 0, line = 0, i, j;

    if (dipsw1 & 32)
    {
        c = c & 0x7f;
    } // are we filtering the high bit?

    // fprintf(stderr,"%02x  ",c);
    switch (mode)
    {
    case IW_TEXT_ESC: // Escape Mode Text.

        if (debug)
            printf("in ESCmode c=%c,0x%x LastSeq: %c 0x%2x submodes:%d,%d,%d,%d\n\n", (c > ' ' && c < 126 ? c : '?'), c, lastseq, lastseq, submode1, submode2, submode3, submode4);

        if (lastseq) // are we already in the process of handling a long escape code?  If so resume it.
        {
            switch (lastseq)
            {

            case 'L': // Set Left Margin.  // here I use c & 15 to get numbers since 0 may be replaced by a space. 32 & 15=0 '0' & 15=0.
                if (debug)
                    fprintf(stdout, "ESC L - Set Left Margin %d\n", leftmargin);
                if (submode1 == 0)
                {
                    leftmargin = (c & 15) * 100;
                    submode1 = 1;
                    return;
                }
                if (submode1 == 1)
                {
                    leftmargin += (c & 15) * 10;
                    submode1 = 2;
                    return;
                }
                if (submode1 == 2)
                {
                    leftmargin += (c & 15);
                    return;
                }
                mode = IW_TEXTMODE;
                return;

            case 'F': // fine position the print head at nnnn from left margin.

                if (submode1 < 4)
                {
                    submode1++;
                    if (c == ' ')
                        c = '0';
                    if (c < '0' || c > '9')
                    {
                        mode = IW_TEXTMODE;
                        return;
                    }
                    submode2 = (submode2 * 10) + (c - '0');
                    if (submode1 < 4)
                        return; // return on n's 0,1,2 only, on 3rd fall through.
                }
                if (debug)
                    fprintf(stdout, "ESC F %04d\n", submode2);

                cursor = (leftmargin + submode2);
                mode = IW_TEXTMODE;
                return;

            case 'T': // Set Line spacing.
                      // 16/144
                if (c == ' ')
                    c = '0';
                if (c < '0' || c > '9')
                {
                    mode = IW_TEXTMODE;
                    return;
                }
                if (submode1 == 0)
                {
                    submode2 = c - '0';
                    submode1++;
                    return;
                }
                if (submode1 == 1)
                {
                    currentVCPI = (c - '0') + (submode2 * 10) + 1;
                    // if (!currentVCPI) currentVCPI=1;
                    // fprintf(stderr,"ESC T:%d\n",currentVCPI);
                }
                mode = IW_TEXTMODE;
                return;

            case 'V': // print a repeating graphics upto nnnn bars of the same pattern for lines

                if (submode1 < 4)
                {
                    submode1++;
                    if (c == ' ')
                        c = '0';
                    if (c < '0' || c > '9')
                    {
                        mode = IW_TEXTMODE;
                        return;
                    }
                    submode2 = (submode2 * 10) + (c - '0');
                    if (submode1 < 4)
                        return; // return on n's 0,1,2 on 3rd one fall through.
                }

                if (submode2)
                    for (i = 0; i < submode2; i++)
                        iw_printbar(c, 0);
                if (debug)
                    fprintf(stdout, "ESC V - Repeating graphic bar %x times %d\n", c, submode);

                mode = IW_TEXTMODE;
                return;

            case 'G': // print raw graphics
                if (submode1 < 4)
                {
                    submode1++;
                    if (c == ' ')
                        c = '0';
                    if (c < '0' || c > '9')
                    {
                        if (debug)
                            printf("Exiting Graphics mode due to bad char: sub:%d,char %2x, nnn:%d\n", submode1, c, submode2);
                        mode = IW_TEXTMODE;
                        return;
                    }
                    submode2 = (submode2 * 10) + (c - '0');
                    return;
                }

                if (submode2)
                {
                    iw_printbar(c, 0);
                    submode2--;
                    if (debug)
                        fprintf(stdout, "Graphics Mode: cursor(%d,%d:%d)\n", cursor, linepixel, line);
                    if (!submode2)
                    {
                        if (debug)
                            printf("Normal graphics exit\n");
                        mode = IW_TEXTMODE; // if we run out of glyphs, bail back to text mode.
                    }
                }
                else
                {
                    mode = IW_TEXTMODE;
                    if (debug)
                        printf("Normal graphics exit\n");
                }

                return;

            case 'g': // print graphics
                if (submode1 < 3)
                {
                    submode1++;
                    if (c == ' ')
                        c = '0';
                    if (c < '0' || c > '9')
                    {
                        mode = IW_TEXTMODE;
                        return;
                    }
                    submode2 = (submode2 * 10) + (c - '0');
                    if (submode1 < 3)
                        return; // return on n's 0,1,2, on 3 fall through
                }
                submode2 = submode2 << 3; // multiply by 8 for this mode.

                if (debug)
                    fprintf(stdout, "ESC g - Graphics *8  %d submode2:%d\n", c, submode2);
                lastseq = 'G';
                submode1 = 4;
                return; // continue in 'G' mode.

            case 115: // proportional text mode spacing between chars.  (ESC s)
                if (debug)
                    fprintf(stdout, "ESC s  - Proportional Text:%x  \n", c);
                if (c == ' ')
                    c = '0';
                if (c < '0' || c > '9')
                    return;
                propspacing = (c - '0');
                if (debug)
                    fprintf(stdout, "Valid ESC s  - Proportional Text:%x  \n", c);

                mode = IW_TEXTMODE;
                return;

            case 'I': // about to load new character set.
                      // first character sent picks ascii of char to replace.

                if (debug)
                    fprintf(stdout, "ESC I  - Load new font (current:%x) char to change:%x Width:%d \n", c, submode1, submode2);

                if (submode1 == 0)
                {
                    submode1 = c;
                    return;
                }                  // submode 1 is which char. -- had a bug here return was not inside {}
                if (submode2 == 0) // next the width of this particular character
                {
                    if (c >= 'a' && c <= 'p') // bottom
                    {
                        submode2 = c - 'a';
                        progfntwidth[submode1] = c - 'a';
                        submode2 = 1 + (progfnoffset = 0);
                        return;
                    }
                    else if (c >= 'A' && c <= 'P') // top
                    {
                        submode2 = c - 'A';
                        progfntwidth[submode1] = c - 'A';
                        submode2 = 1 + (progfnoffset = 1);
                        return;
                    }
                    else
                    {
                        mode = IW_TEXTMODE;
                        return;
                    }
                }
                // at this point we got ESC I,char,its width.
                // bit 0 is top wire, bit 7 is bottom.

                // bail out if we already got our bits and a control D.
                if (c == 0x04 && (submode3 == progfntwidth[submode1]))
                {
                    mode = IW_TEXTMODE;
                    return;
                }
                else if (submode3 == progfntwidth[submode1])
                {
                    // If not ^D, we're expecting more, so set the char to change to C
                    // and switch modes back.
                    submode1 = c;
                    submode2 = 0;
                    submode3 = 0;
                    return;
                }

                //           char          line

                progfont[submode1][submode3++] = c;
                if (submode3 == progfntwidth[submode1])
                {
                    submode1 = c;
                    submode2 = 0;
                    submode3 = 0;
                }
                return;

            case IW_REPEAT100: // repeat 100's place.
                if (debug)
                    fprintf(stdout, "Repeat a character 100 place. %x \n", c);
                if (c == ' ')
                    c = 0;
                if (c < '0' || c > '9')
                {
                    mode = IW_TEXTMODE;
                    return;
                }
                repeatchar = 0;
                repeatchar += (c - '0') * 100;
                lastseq = IW_REPEAT10;
                return;

            case IW_REPEAT10: // repeat 100's place.
                if (debug)
                    fprintf(stdout, "Repeat a character 10 place. %x \n", c);
                if (c == ' ')
                    c = 0;
                if (c < '0' || c > '9')
                {
                    mode = IW_TEXTMODE;
                    return;
                }
                repeatchar += (c - '0') * 10;
                lastseq = IW_REPEAT1;
                return;

            case IW_REPEAT1: // repeat 1's place.
                if (debug)
                    fprintf(stdout, "Repeat a character 1 place. %x \n", c);
                if (c == ' ')
                    c = 0;
                if (c < '0' || c > '9')
                {
                    mode = IW_TEXTMODE;
                    return;
                }
                repeatchar += (c - '0');
                lastseq = 0;
                return;
            case 'Z': // ESC Z mode.
                if (debug)
                    fprintf(stdout, "ESC Z mode 8 bit mode if null space, @NUL sets LF to be CR %x \n", c);
                if (submode1 == 0) // ESC Z Control-A
                {
                    if (c == 0)
                        submode1 = 1280; // ESC Z NULL SPACE sets 8 bit on or off!
                    if (c == 1)
                        submode1 = 1;
                    if (c == 64)
                        submode1 = 64; // ESCZ@NUL sets LF,VT,FF to be recognized as CR
                    if (c == ' ')
                        submode1 = ' '; // ESCZ space NUL sets no LineFeed when buffer full (ignored in emulator.)
                    return;
                }

                if (submode1 == 1) // ESC Z Control-A,Control@
                    if (c == 0)
                    {
                        if (debug)
                            fprintf(stdout, "ESC Z ^A ^@ - End Slashed Zero  %x \n", c);
                        submode1 = 0;
                        lastseq = 0;
                        slashzero = 0;
                        mode = IW_TEXTMODE;
                        return;
                    }

                if (submode1 == 64) // ESC Z @ NUL
                    if (c == 0)
                    {
                        submode1 = 0;
                        lastseq = 0;
                        LFVTFFareCR = 0;
                        mode = IW_TEXTMODE;
                        if (debug)
                            fprintf(stdout, "ESC Z @ NUL - LF,VT,FF-> CR  %x \n", c);
                        return;
                    }
                if (submode1 == 1280) // ESC Z  NUL 32
                    if (c == 32)
                    {
                        submode1 = 0;
                        lastseq = 0;
                        dipsw1 = dipsw1 & (255 - 32); // disable SW1-5 allowing 8 bit data.
                        mode = IW_TEXTMODE;
                        if (debug)
                            fprintf(stdout, "ESC Z NUL 32 - Allow 8 bit data %x \n", c);
                        return;
                    }
                if (submode1 == ' ') // ESC Z _ NUL
                    if (c == 0)
                    {
                        submode1 = 0;
                        lastseq = 0;
                        AutoLFonFull = 0;
                        mode = IW_TEXTMODE;
                        if (debug)
                            fprintf(stdout, "ESC Z _ NUL - Disable Auto LF on buffer full %x \n", c);
                        return;
                    }

                if (debug)
                    fprintf(stdout, "ESC Z - Disable Auto LF after each CR. %x \n", c);

                if (submode1 == 128) // Disable Auto LF after each CR?
                    if (c == 1)
                    {
                        autoLF = 0;
                        lastseq = 0;
                        mode = IW_TEXTMODE;
                        return;
                    }
                if (c == 128)
                {
                    submode1 = 128;
                    return;
                }

                mode = IW_TEXTMODE;
                return;

            case 'D':              // ESC D mode.
                if (submode1 == 0) // ESC D Control-A
                {
                    if (c == 0)
                        submode1 = 1280; // ESC Z NULL SPACE sets 8 bit on or off!
                    if (c == 1)
                        submode1 = 1;
                    if (c == 64)
                        submode1 = 64;
                    if (c == ' ')
                        submode1 = ' '; // ESCZ space NUL sets no LineFeed when buffer full (ignored in emulator.)
                    return;
                }

                if (submode1 == 1) // ESC D Control-A,Control@
                    if (c == 0)
                    {
                        submode1 = 0;
                        lastseq = 0;
                        slashzero = 1;
                        mode = IW_TEXTMODE;
                        if (debug)
                            fprintf(stdout, "ESC D ^A ^@ Slashed Zero on   %x \n", c);
                        return;
                    }

                if (submode1 == 64) // ESC Z @ NUL
                    if (c == 0)
                    {
                        submode1 = 0;
                        lastseq = 0;
                        LFVTFFareCR = 1;
                        mode = IW_TEXTMODE;
                        if (debug)
                            fprintf(stdout, "ESC D @ NUL - LF,VT,FF ->CR on %x  \n", c);
                        return;
                    }

                if (submode1 == ' ') // ESC Z space NUL
                    if (c == 0)
                    {
                        submode1 = 0;
                        lastseq = 0;
                        AutoLFonFull = 1;
                        mode = IW_TEXTMODE;
                        if (debug)
                            fprintf(stdout, "ESC D 32 NUL - AutoLF on buffer full on  %x  \n", c);
                        return;
                    }

                if (submode1 == 1280) // ESC Z  NUL 32
                    if (c == 32)
                    {
                        submode1 = 0;
                        lastseq = 0;
                        dipsw1 |= 32; // enable SW1-5 filtering 8 bit data.
                        mode = IW_TEXTMODE;
                        if (debug)
                            fprintf(stdout, "ESC D NUL 32 - Ignore 8th bit on data!   %x  \n", c);
                        return;
                    }
                //     if (submode1==' ')  // ESC Z _ NUL
                if (debug)
                    fprintf(stdout, "ESC D _ NUL Auto LF after CR?            %x  \n", c);

                if (submode1 == 128) // Auto LF after each CR?
                    if (c == 1)
                    {
                        autoLF = 1;
                        lastseq = 0;
                        mode = IW_TEXTMODE;
                        return;
                    }
                if (c == 128)
                {
                    submode1 = 128;
                    return;
                }

                mode = IW_TEXTMODE;
                return;

            case 0x6c: // Disable autoCR after LF // ESC l

                if (debug)
                    fprintf(stdout, "ESC l CR before LF?= %x  \n", c);
                if (c == '1')
                {
                    autoCRoff = 0;
                    lastseq = 0;
                    return;
                }
                if (c == '0')
                {
                    autoCRoff = 1;
                    lastseq = 0;
                    return;
                }
                return; // 20190831 return was missing, fallthrough
                        /*                           case 0x54: // set line feed pitch.
                                                              if (submode1==0) {submode1=1; submode2=((c % 10) *10); return;}
                                                              if (submode1==1) {submode1=0; linefeedpitch=submode2+(c % 10); return;}
                        */

            case '(': // set horizontal tabs.
                if (debug)
                    fprintf(stdout, "ESC ( Set HTABS...   %x  \n", c);
                if (c == '.')
                {
                    mode = IW_TEXTMODE;
                    return;
                }
                if ((c >= '0' && c <= '9') || c == ' ')
                {
                    if (c == ' ')
                        c = '0';
                    submode2 = (c - '0' + (submode2 * 10)) & 255;
                    tabs[submode1] = submode2 * currentCPI;
                    return;
                }
                if (c == ',')
                {
                    submode1++;
                    submode2 = 0;
                    submode1 &= 0x1f;
                    return;
                }

                mode = IW_TEXTMODE;
                return;

            case ')': // clear an individual horizontal tab
                if (debug)
                    fprintf(stdout, "ESC ( Clea an HTABS...   %x  \n", c);
                if (c == ' ')
                    c = '0';
                if (c >= '0' && c <= '9')
                    submode2 = c - '0' + (10 * submode2);
                submode1++;
                if (submode1 >= 3)
                {
                    for (i = 0; i < 32; i++)
                        if (tabs[i] == submode2)
                            tabs[i] = 0;
                    mode = IW_TEXTMODE;
                }

                return;

            default:
                mode = IW_TEXTMODE;
                fprintf(stdout, "** BAD ESC Sequence!!!  %x  \n", c);
                return;
            }
        }
        lastseq = 0;
        submode1 = 0;
        submode2 = 0;
        submode3 = 0;

        switch (c)
        {

            // Monospace font sizes:
        case 'n':
            fontnumber = 0;
            mode = IW_TEXTMODE;
            return; // 9 cpi    72dpi  576dots per line extended
        case 'N':
            fontnumber = 1;
            mode = IW_TEXTMODE;
            return; // 10cpi    80dp   640dots per line pica
        case 'E':
            fontnumber = 2;
            mode = IW_TEXTMODE;
            return; // 12cpi    96dp   768dots per line elite  *** Elite ***

        case 'p':
            fontnumber = 3;
            propspacing = 1;
            mode = IW_TEXTMODE;
            return; // Pica Prop       144dpi 1152dots per line elite proportional.
        case 'P':
            fontnumber = 7;
            propspacing = 1;
            mode = IW_TEXTMODE;
            return; // Elite Prop      144dpi 1152dots per line elite proportional.

        case 'e':
            fontnumber = 4;
            mode = IW_TEXTMODE;
            return; // 13.4cpi 107dpi  856dots per line semicondensed.
        case 'q':
            fontnumber = 5;
            mode = IW_TEXTMODE;
            return; // 15cpi   120dpi  960dots per line Condensed.
        case 'Q':
            fontnumber = 6;
            mode = IW_TEXTMODE;
            return; // 17cpi   136dpi 1088dots per line UltraCondensed.

        case 115: // proportional text mode.  Next char is size of dots between chars.
            lastseq = 115;
            return;
            // proportional spacing for selected chars in elite font only.
        case '1':
            if (fontnumber == 3)
                elitespacing = 1;
            mode = IW_TEXTMODE;
            return;
        case '2':
            if (fontnumber == 3)
                elitespacing = 2;
            mode = IW_TEXTMODE;
            return;
        case '3':
            if (fontnumber == 3)
                elitespacing = 3;
            mode = IW_TEXTMODE;
            return;
        case '4':
            if (fontnumber == 3)
                elitespacing = 4;
            mode = IW_TEXTMODE;
            return;
        case '5':
            if (fontnumber == 3)
                elitespacing = 5;
            mode = IW_TEXTMODE;
            return;
        case '6':
            if (fontnumber == 3)
                elitespacing = 6;
            mode = IW_TEXTMODE;
            return;
        case '7':
            if (fontnumber == 3)
                elitespacing = 7;
            mode = IW_TEXTMODE;
            return;
        case '8':
            if (fontnumber == 3)
                elitespacing = 8;
            mode = IW_TEXTMODE;
            return;

            // direct graphics printing.

        case 'G': // both ESC G and ESC S are identical in function.
        case 'S':
            if (debug)
                fprintf(stdout, "ESC S = ESC G Switching lastseq to G\n");
            lastseq = 'G';
            return;

        case 'g':
            lastseq = 'g';
            return; // print graphics corresponding to a pattern.

        case 'V':
            lastseq = 'V';
            return; // repeating line of graphics upto nnnn bits.

        case 'F':
            lastseq = 'F';
            return; // fine position printhead at nnnn offset from left margin.

            // programable fonts

        case '+':
            proglastwidth = 8;
            mode = IW_TEXTMODE;
            return;

        case '-':
            proglastwidth = 16;
            mode = IW_TEXTMODE;
            return;

        case 'I':
            lastseq = 'I';
            return; // load fonts

            // custom or normal font selection.

        case '\'': // switch to custom font

            memcpy(livefont, progfont, (128 * 16 * sizeof(int16)));
            memcpy(livefntwidth, progfntwidth, (sizeof(int8) * 128));
            mode = IW_TEXTMODE;
            return;
        case '*': // switch to custom font high bit chars.

            memcpy(livefont, &progfont[128], (128 * 16 * sizeof(int16)));
            memcpy(livefntwidth, &progfntwidth[128], (sizeof(int8) * 128));
            mode = IW_TEXTMODE;
            return;

        case '$': // switch to normal font.

            if ((fontnumber == 3 || fontnumber == 7) || propspacing) // which font? Proportional or fixed?
            {
                for (int i = 0; i < 128; i++)
                {
                    livefntwidth[i] = iwpropw[fontnumber][i];
                    for (int j = 0; j < 18; j++)
                        livefont[i][j] = iwprop[fontnumber][i][j];
                }
            }
            else
            {
                // getting a warning here but: imagewriter-wx.h:    int16 livefont[256][19];     // live font (95-8*16 or 175-8*8) 175 chars max 16 lines each
                //
                // imagewriter-wx.cpp:1354:89: warning: iteration 8u invokes undefined behavior [-Waggressive-loop-optimizations]
                // uint16 iwfixed[8][128][8] = {
                // uint16 iwprop [8][128][19] =
                //  int16 livefont  [256][19];     // live font (95-8*16 or 175-8*8) 175 chars max 16 lines each

                for (int i = 0; i < 128; i++)
                {
                    livefntwidth[i] = 8; // 20190825 vvv was j<18 in for loop below
                    for (int j = 0; j < 8; j++)
                        livefont[i][j] = iwfixed[fontnumber][i][j];
                }
            }
            mode = IW_TEXTMODE;
            return;

            // Left Margin. ESC L, nnn
        case 'L':
            lastseq = 'L';
            return;

            // repeated character.
        case 'R':
            lastseq = IW_REPEAT100;
            return;

            // to slash or not to slash zeros, that is the question.
        case 'Z':
            lastseq = 'Z';
            return;
        case 'D':
            lastseq = 'D';
            return;

            // To underline or not to underline?
        case 'X':
            underline = 1;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return;
        case 'Y':
            underline = 0;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return;

            // To bold or not to bold?
        case '!':
            bold = 1;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return;
        case '"':
            bold = 0;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return;

            // To bidir or not
        case '<':
            bidirectional = 1;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return;
        case '>':
            bidirectional = 0;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return;

        case 0x6c:
            lastseq = 0x6c;
            return;
            // case 0x54: lastseq=0x54; return;

            // Line Feed Pitch.
        case 'A':
            linefeedpitch = 24;
            currentVCPI = 18 / 2;
            lastseq = 0;
            mode = IW_TEXTMODE; // 20070329 - added /2
            fprintf(stderr, "Line Feed Pitch 18 ESC-A\n");

            return;
        case 'B':
            linefeedpitch = 18;
            currentVCPI = 16 / 2;
            lastseq = 0;
            mode = IW_TEXTMODE; // 20070329 - added /2
            fprintf(stderr, "Line Feed Pitch 16 ESC-B\n");
            return;

            // Forward or Reverse Line Feeds.
        case 'r':
        {
            linefeedreverse = 1;
            mode = IW_TEXTMODE;
            if (debug)
                fprintf(stdout, "!! Reverse Line Feed used !!\n");
            return;
        }
        case 'f':
        {
            linefeedreverse = 0;
            mode = IW_TEXTMODE;
            if (debug)
                fprintf(stdout, "Forward LF selected.!!\n");
            return;
        }

            // ESC v is Set Top of Form.  we can likely ignore this.
        case 'v':
            iw_settopofform(); // might need to implement this -- or not.
            mode = IW_TEXTMODE;
            return;

            // Set line spacing usually 16.
        case 'T':
            lastseq = 'T';
            return;

            // Paper Detector Error light.  We can ignore this totally.  These codes enable
            // or disable the paper error light on the IW console.  They don't return shit to the Lisa.
            // Since we don't care about the light on the console, these are ignored.
        case 'o':
            mode = IW_TEXTMODE;
            return;
        case 'O':
            mode = IW_TEXTMODE;
            return;

        case 'c':
            // fprintf(stderr,"Reinitialize Printer to Soft defaults ESC c\n");
            // 20070320//iw_formfeed();
            iw_initialize(0);
            mode = IW_TEXTMODE;
            return;

            // Set Horizontal tabs.  ESC ( nnn,nnn,nnn. Leading n's can be 0's or spaces. End with a dot.
        case '(':
            lastseq = '(';
            submode1 = 0;
            submode2 = 0;
            submode3 = 0;
            return;

        case 'u': // set the current position as a tab stop.
            if (cursor)
            {
                for (i = 0; i < 32; i++)
                    if (tabs[i] == 0)
                    {
                        tabs[i] = cursor;
                        i = 32;
                    }
            }
            mode = IW_TEXTMODE;
            return;

        case '0': // clear all tabs
            for (i = 0; i < 32; i++)
                tabs[i] = 0;
            mode = IW_TEXTMODE;
            return;

        case ')': // clear a specific tab
            lastseq = ')';
            submode1 = 0;
            submode2 = 0;
            submode3 = 0;
            return;

        default:
            fprintf(stderr, "IW:*** UNKNOWN ESC SEQUENCE %c %x %d\n", c, c, c);

            mode = IW_TEXTMODE;
            return; // ignore unknown esc sequences.
        }

    case IW_TEXT_CTRLA: // so far this is only to set slashed zeros or not.  Might need to expand this, this is ignored now.

        if (c == 0)
        {
            mode = IW_TEXTMODE;
            return;
        }

        mode = IW_TEXTMODE;
        return;

    case IW_TEXTMODE:
        switch (c)
        {
        case 27: // escape character.
            lastseq = 0;
            submode1 = 0;
            submode2 = 0;
            submode3 = 0;
            mode = IW_TEXT_ESC;
            return;

        case 1: // control-A;          // also used to go to next top of page or bottom of page vtab???
            mode = IW_TEXT_CTRLA;
            return;

        case 0x1f: // control_ (control under line)
            mode = IW_TEXT_CTRLUL;
            return;

        case 12: // control L -- jump to next page (page feed.)
            page = currentformnumber;
            currentformnumber = ((page + 1) % 5);

            for (i = 0; i < 96; i++)
                if (vtabs[page][i][5])
                {
                    iw_set_vline_pos(i);
                    return;
                }

            // incase we didn't find the next form, spit out the whole page.
            iw_formfeed();
            return;

        case 0x1d: // 0x1d mode - used to set top of form
            mode = IW_TEXT_0X1D;
            submode = 0;
            submode1 = 0;
            submode2 = 0;
            return;

        case 10: // line feed
            if (debug)
                fprintf(stdout, "Got LF from IW, AutoCRoff:%d, LFVTFFareCR:%d AutoCR Condition %d\n", autoCRoff, LFVTFFareCR, ((!autoCRoff || LFVTFFareCR)));
            iw_lf();
            if (!autoCRoff || LFVTFFareCR)
                iw_cr();
            return;

        case 9: // tab ^I=TAB  horizontal tab
            // Find nearest tab to the right of the print head.
            j = 0;
            for (i = 0; i < 32; i++)
                if (tabs[i] > cursor)
                    if (tabs[i] < j)
                        j = tabs[i];
            // jump to that if its found.

            if (debug)
                fprintf(stdout, "^I (Tab) from %d to %d (ignore if 0)\n", cursor, j);

            if (j)
                cursor = j;
            return;

            /*    case 12: // form feed. OLD CODE... Handled above a better way...
                           if (LFVTFFareCR) // If LFVTFFareCR mode feed a CR first.
                           {
                            if (!autoLF) iw_lf();
                            iw_cr();
                           }
            */
        case 13: // carriage return
            if (debug)
                fprintf(stdout, "Got CR from IW, AutoLF is %d\n", autoLF);
            if (autoLF)
                iw_lf();
            iw_cr();
            return;

            /* case 31: // multiple line feeds.  Gonna treat this like a fake escape code.
                     mode=IW_TEXT_ESC;
                     lastseq=27;
                     return;
            */

            // To headline or not?
        case 14:
            headline = 1;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return; // ^N
        case 15:
            headline = 0;
            lastseq = 0;
            mode = IW_TEXTMODE;
            return; // ^O

        case 24: // Control X cancels any unprinted characters in the buffer.
                 // for now we're gonna ignore this.  We can implement it later, but
                 // our IW model is that we print all we receive as we receive it.
                 // We'll implement this if it's found that it's used by real software.
                 // Note that the Lisa sends a bunch of these before every print job to
                 // flush any jobs in the buffer...  Shouldn't affect us at all.
            lastseq = 0;
            mode = IW_TEXTMODE;
            return;

        default:
            iw_printchar(c);
            return;
        }
    case IW_TEXT_0X1D: // TOP of FORM for setting vertical tabs is 0x1d 0x41 0x40 -- not sure how this should be...
        if (c == 65 && submode == 0)
        {
            submode = 1;
            submode1 = 0;
            iw_settopofform();
            return;
        }
        if (c == 64 && submode == 1)
        {
            submode = 2;
            submode1 = 0;
            submode2 = 0;
            submode3 = 0;
            submode4 = (uint16)-1;
            return;
        }

        page = submode3;
        line = submode4 + 1;

        if (submode == 2)
        {
            if (submode2 == 0)
            {
                submode2 = c;
                return;
            }

            c2 = submode2;
            submode2 = 0;
            submode3 = 0;

            if (c == '@' && c2 == '@')
            {
                vtabs[page][line][0] = 0;
                vtabs[page][line][1] = 0;
                vtabs[page][line][2] = 0;
                vtabs[page][line][3] = 0;
                vtabs[page][line][4] = 0;
                submode3 = page;
                submode4 = line;
                return;
            } // sets no tabs

            if (c == 'B' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // sets tab B
            if (c == 'D' && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // sets tab C
            if (c == 'H' && c2 == '@')
            {
                vtabs[page][line][2] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // sets tab D
            if (c == 'P' && c2 == '@')
            {
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // sets tab E
            if (c == 96 && c2 == '@')
            {
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // sets tab F '

            if (c == 'F' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // B&C
            if (c == 'J' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][2] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // B&D
            if (c == 'R' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // B&E
            if (c == 'b' && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // B&F

            if (c == 'L' && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                vtabs[page][line][2] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // C&D
            if (c == 'T' && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // C&E
            if (c == 'd' && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // C&F

            if (c == 'X' && c2 == '@')
            {
                vtabs[page][line][2] = 1;
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // D&E
            if (c == 'h' && c2 == '@')
            {
                vtabs[page][line][2] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // D&E

            if (c == 'p' && c2 == '@')
            {
                vtabs[page][line][3] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // E&F

            if (c == 'N' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                vtabs[page][line][2] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BCD
            if (c == 'V' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BCE
            if (c == 'f' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BCF
            if (c == 'Z' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BDE
            if (c == 'j' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BDF

            if (c == 'j' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BDF
            if (c == 'r' && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][3] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BEF
            if (c == 92 && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // CDE
            if (c == 108 && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // CDF
            if (c == 116 && c2 == '@')
            {
                vtabs[page][line][1] = 1;
                vtabs[page][line][3] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // CEF

            if (c == 94 && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][3] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BCDE

            if (c == 110 && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BCDF

            if (c == 118 && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                vtabs[page][line][3] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BCEF

            if (c == 122 && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][3] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BDEF

            if (c == 126 && c2 == '@')
            {
                vtabs[page][line][0] = 1;
                vtabs[page][line][1] = 1;
                vtabs[page][line][2] = 1;
                vtabs[page][line][3] = 1;
                vtabs[page][line][4] = 1;
                submode3 = page;
                submode4 = line;
                return;
            } // BCDEF

            if (c == 'C' && c2 == '@')
            { // set bottom of form
                vtabs[page][line][5] = 1;
                submode3 = page;
                submode4 = line;
                return;
            }

            if (c == 'A' && c2 == '@')
            { // A@(Control-) (30 or 0x1e) set next page;
                submode3 = page;
                submode4 = line;
                return;
            }

            if (c == 0x1e)
            {
                submode3 = page;
                submode4 = line;
                return;
            } // eat control - from A@^- this is cheating, I know.
        }
        return; // 20190901

    case IW_TEXT_CTRLUL:

        if (c >= 'A' && c <= 'F')
        {
            if (c == 'A')
                c = 'G';
            c -= 'B';
            for (i = 0; i < 96; i++)
                if (vtabs[page][i][c])
                {
                    iw_set_vline_pos(i);
                    mode = IW_TEXTMODE;
                    submode1 = 0;
                    submode2 = 0;
                    submode3 = 0;
                    submode4 = 0;
                    return;
                }
            // at this point, we didn't find what we were looking for, so we have to jump to next BOF.
            for (i = 0; i < 96; i++)
                if (vtabs[page][i][5])
                {
                    iw_set_vline_pos(i);
                    mode = IW_TEXTMODE;
                    submode1 = 0;
                    submode2 = 0;
                    submode3 = 0;
                    submode4 = 0;
                    return;
                }

            // we didn't find the next BOF, we go to the next page
            // fprintf(stderr,"FF via CONTROL_ (bottom of form)\n");
            fprintf(stderr, "formFeed due to not finding next bottom of form");
            iw_formfeed();
            mode = IW_TEXTMODE;
            submode1 = 0;
            submode2 = 0;
            submode3 = 0;
            submode4 = 0;
            return;
        }
        mode = IW_TEXTMODE;
        submode1 = 0;
        submode2 = 0;
        submode3 = 0;
        submode4 = 0;
        return;

        /* case IW_TEXT_GS:                      // set vertical tabs to power on status, TOF to current position.
                    if ( c=='0')
                       {
                        ***

                       }
      */
        mode = IW_TEXTMODE;
        submode1 = 0;
        submode2 = 0;
        submode3 = 0;
        submode4 = 0;
        return;
    default:
        mode = IW_TEXTMODE;
        submode1 = 0;
        submode2 = 0;
        submode3 = 0;
        submode4 = 0;
        return;
    }
}

/******************* To invoke this as a library *********************************************

 To initialize the printer:

   ImageWriter *IW;
   ...
   IW=new ImageWriter(int outputtype, wxString outfname, int dip1=210, float paperx=8.5, float papery=11.0);




 To send a printjob, setup a loop like this:
   for (i=0; i<printjobsize; i++) ImageWriterLoop(printjob[c]);

 To send more jobs, be sure to re-initialize the printer first.
 And when you're done with the printer, call the destructor


*********************************************************************************************/

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

void ImageWriter::iw_spitout_wx(void)
{
#define IW_PRINT_OUT 0
#define IW_PNG_OUT 1
// future reserved types.
#define IW_PDF_OUT 2
#define IW_CALLBACK 3
    // callback should pass pagenum as int and the wxBitmap

    if (isblank)
        return;

    uint8 bmphdr[] =
        {
            // these are low endian since this is a windows format

            'B', 'M', // signature                         0-1

            0, 0, 0, 0, // size of file in bytes             2-5

            0, 0, // reserved                          6-7
            0, 0, // reserved                          8-9

            118, 0, 0, 0, // offset to start of data           10-13

            // image header.
            40, 0, 0, 0, // header size                       14-17

            0, 0, 0, 0, // image width in pixels             18-21
            0, 0, 0, 0, // image height in pixels            22-25

            1, 0,       // bi-planes, must always be 1       26-27
            4, 0,       // depth                             28-29
            0, 0, 0, 0, // compression - 0=none              30-33
            0, 0, 0, 0, // image size - 0 for uncompressed   34-37

            0x23, 0x2e, 0, 0, // X resolution per meter?? (300dpi) 38-41
            0x23, 0x2e, 0, 0, // Y resolution per meter?? (300dpi) 42-45

            15, 0, 0, 0, // 16,0,0,0                           // number of colors used             46-49
            15, 0, 0, 0, // 16,0,0,0                           //                                   50-53

            // color table                                                             54-...118
            // blue,gree,red,alpha
            0xff, 0xff, 0xff, 0xff, // color 0                           16*4=64+54=118 118 is offset to data.

            0x10, 0x10, 0x10, 0xff, // color 1
            0x08, 0x08, 0x08, 0xff, // color 2
            0x00, 0x00, 0x00, 0xff, // color 3
            0x00, 0x00, 0x00, 0xff, // color 4

            0x00, 0x00, 0x00, 0xff, // color 5
            0x00, 0x00, 0x00, 0xff, // color 6
            0x00, 0x00, 0x00, 0xff, // color 7
            0x00, 0x00, 0x00, 0xff, // color 8

            0x00, 0x00, 0x00, 0xff, // color 9
            0x00, 0x00, 0x00, 0xff, // color 10
            0x00, 0x00, 0x00, 0xff, // color 11

            0x00, 0x00, 0x00, 0xff, // color 12
            0x00, 0x00, 0x00, 0xff, // color 13
            0x00, 0x00, 0x00, 0xff, // color 14
            0x00, 0x00, 0x00, 0xff, // color 15

            0x00, 0x00, 0x00, 0, // color 15
            0x00, 0x00, 0x00, 0, // color 15
            0x00, 0x00, 0x00, 0, // color 15

            0, 0, 0, 0, 0, 0, 0 // padding
                                // data starts here -------------------------------------
        };

    /*-----------------3/4/2007 9:26AM------------------------------------------------
     * this is the most idiotic thing I've ever had to do in order to get this to
     * work.  Seems wxWidgets can't handle very large bitmaps, and worse, on X11, it
     * sends them to the Xserver when they're wxMemoryDC's.
     *
     * So I've reverted back to the old code of maintaining my own 4 bit bitmap, then
     * in order to get that data back into wxland, I have to save it as a windows BMP
     * and THEN convert it to whatever I need.  Fugly.  Very fugly.
     *
     * ------------------------------------------------------------------------------*/

    uint32 filesize;
    filesize = pagesize + 118;
    char tmp[MAXPATHLEN];

    bmphdr[6] = (filesize) & 0xff;
    bmphdr[7] = (filesize >> 8) & 0xff;
    bmphdr[8] = (filesize >> 16) & 0xff;
    bmphdr[9] = (filesize >> 24) & 0xff;

    bmphdr[34] = (pagesize) & 0xff;
    bmphdr[35] = (pagesize >> 8) & 0xff;
    bmphdr[36] = (pagesize >> 16) & 0xff;
    bmphdr[37] = (pagesize >> 24) & 0xff;

    bmphdr[18] = ((owidth)) & 0xff;
    bmphdr[19] = ((owidth) >> 8) & 0xff;
    bmphdr[20] = ((owidth) >> 16) & 0xff;
    bmphdr[21] = ((owidth) >> 24) & 0xff;

    bmphdr[22] = ((height)) & 0xff;
    bmphdr[23] = ((height) >> 8) & 0xff;
    bmphdr[24] = ((height) >> 16) & 0xff;
    bmphdr[25] = ((height) >> 24) & 0xff;

    wxString savefilename;
    wxDateTime now = wxDateTime::Now();

    // time contains : which is anathema to win32 (and classic mac os if we ever port there)
    char time[40];
    char date[40];
    wxString T, D;
    T = now.FormatISOTime();
    D = now.FormatISODate();
    char *t, *d;
    t = (char *)(const char *)(T.c_str());
    d = (char *)(const char *)(D.c_str());

    int i, j;

    for (i = 0, j = 0; i < 39 && d[i]; i++)
    {
        if (t[i] != ':' && t[i] != '/' && t[i] != '\\')
        {
            time[j] = t[i];
            j++;
        }
        time[j] = 0;
    }
    for (i = 0, j = 0; i < 39 && d[i]; i++)
    {
        if (d[i] != ':' && d[i] != '/' && d[i] != '\\')
        {
            date[j] = d[i];
            j++;
        }
        date[j] = 0;
    }

    wxString pagenumber;
    pagenumber.Printf(_T("%04d"), ((int)(pagenum)));
    savefilename = destination + wxFileName::GetPathSeparator() +
                   _T("iw") +
                   // iwid +
                   wxString(date, wxConvLocal, 2048) + // wxSTRING_MAXLEN) +
                   wxString(time, wxConvLocal, 2048 /* wxSTRING_MAXLEN */) + pagenumber;

    wxString tempfilename;
    tempfilename = savefilename + _T(".bmp");

    FILE *bmp;
    strncpy(tmp, CSTR(tempfilename), MAXPATHLEN);
    bmp = fopen(tmp, "wb");
    fwrite(bmphdr, 118, 1, bmp);
    fwrite(page, pagesize, 1, bmp);
    fclose(bmp);

    wxImage *img;
    wxBitmap *bits;
    wxMemoryDC *pageDC;

    img = new wxImage(tempfilename, wxBITMAP_TYPE_BMP);
    bits = new wxBitmap(*img);
    pageDC = new wxMemoryDC();
    pageDC->SelectObject(*bits);
    delete img;

    switch (desttype)
    {
    case IW_PRINT_OUT:
    {
        // this is the first page that we're printing
        if (!printerDC)
        {
            if (!printdialog)
            {
                printdialog = new wxPrintDialog(parentwindow);
                if (printdialog->ShowModal() == wxID_CANCEL)
                    return;
            }
            printerDC = printdialog->GetPrintDC();
            printerDC->StartDoc(wxT("LisaEm"));
        }

        int width, height;
        printerDC->GetSize(&width, &height);
        double scaleX = (double)((double)width / (double)bits->GetWidth());
        double scaleY = (double)((double)height / (double)bits->GetHeight());
        printerDC->SetUserScale(MIN(scaleX, scaleY), MIN(scaleX, scaleY));

        // fprintf(stderr,"Printer Paper size:%d,%d xdpi,ydpi:%d,%d",
        //         width,     height,
        //         (width/17)>>1, (height/11) );

        if (!printerDC)
        {
            delete bits;
            delete pageDC;
            return;
        }

        printerDC->StartPage();
        printerDC->Blit(0, 0, bits->GetWidth(), bits->GetHeight(), pageDC, 0, 0, wxCOPY, false);
        printerDC->EndPage();
        // printer_y+=page->GetHeight();

        delete bits;
        delete pageDC;
        unlink(tmp);
        return;
    }
    case IW_PDF_OUT:

        // not yet implemented.

    case IW_PNG_OUT:
    default:
        wxImage image = bits->ConvertToImage();

#ifdef wxUSE_LIBPNG
        wxString pngfilename = savefilename + _T(".png");

        // image.SetMask(NULL);
        // image.SetMaskColour(255,255,255);
        image.Replace(102, 102, 102, 255, 255, 255); // hack to fix the gray background.
        image.SetOption(wxIMAGE_OPTION_PNG_FORMAT, wxPNG_TYPE_GREY_RED);
        image.SetOption(wxIMAGE_OPTION_PNG_BITDEPTH, 4);

        bool result = image.SaveFile(pngfilename, wxBITMAP_TYPE_PNG);
        if (result)
            unlink(tmp);

        delete bits;
        delete pageDC;
        return;

#else
#ifdef wxUSE_LIBTIFF
        wxString tiffilename = savefilename + ".tif";

        bool result = image->SaveFile(tiffilename, wxBITMAP_TYPE_TIF);
        if (result)
            unlink(tempfilename.c_str());

        delete bits;
        delete pageDC;
        return;
#else
// do nothing, it's already a BMP file.
#endif

#endif

        delete bits;
        delete pageDC;
        return;
    }
}
