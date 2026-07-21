/*
 * Copyright (C) 2003 Maxim Stepin ( maxst@hiend3d.com )
 *
 * Copyright (C) 2010 Cameron Zemek ( grom@zeminvaders.net)
 *
 * Edits for LisaEm use, Copyright (C) 2018 Ray Arachelian
 *                   <ray@arachelian.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdint.h>

#include <wx/wx.h>
#include <wx/defs.h>
#include <wx/image.h>
#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <wx/gdicmn.h>
#include <wx/pen.h>
#include <wx/scrolwin.h>
#include <wx/rawbmp.h>

#include <machine.h>
#include <extrablue.h>

#include <common.h>
#include <hqx.h>

// need to rewrite these, these (*dp) are the output buffer looks like one pixel at a time //
// push these to something wxPlot, either via rabits or draw, replace some of these with macros that
// decide between the two.  1M=middle, 1Up, 1Left, 0center
//   +----+----+----+
//   |    |    |    |
//   | w1 | w2 | w3 |
//   +----+----+----+
//   |    |    |    |
//   | w4 | w5 | w6 |
//   +----+----+----+
//   |    |    |    |
//   | w7 | w8 | w9 |
//   +----+----+----+
//
// dp = current pixel
// dp + 1 one to the right
// dp + 2 two to the right
// dp + dpL one down, etc.

/*
  need 9 iterators.  wow.

  macros should be replaced with http://docs.wxwidgets.org/3.0/classwx_pixel_data_1_1_iterator.html
       yx
       vv
  PIXEL00 is x  , y          1
  PIXEL01 is x+1, y          2
  PIXEL02 is x+2, y          3

  PIXEL10 is x  , y+1        4
  PIXEL11 is x+1, y+1        5
  PIXEL12 is x+2, y+1        6

  PIXEL20 is x  , y+2        7
  PIXEL21 is x+1, y+2        8
  PIXEL22 is x+2, y+2        9
       ^^
       yx

#include <wx/rawbmp.h>


template<class Image, class PixelFormat = wxPixelFormatFor<Image>>
class PixelData< Image, PixelFormat >::Iterator

Public Member Functions
void    Reset (const PixelData &data)   Reset the iterator to point to (0, 0). More...
     Iterator (PixelData &data)    Initializes the iterator to point to the origin of the given pixel data. More...
    Iterator (wxBitmap &bmp, PixelData &data)    Initializes the iterator to point to the origin of the given Bitmap. More...
    Iterator ()    Default constructor. More...

bool    IsOk () const    Return true if this iterator is valid. More...

Iterator &  operator++ ()    Advance the iterator to the next pixel, prefix version. More...

Iterator    operator++ (int)    Advance the iterator to the next pixel, postfix (hence less efficient – don't use it unless you absolutely must) version. More...

void    Offset (const PixelData &data, int x, int y)    Move x pixels to the right and y down. More...

void    OffsetX (const PixelData &data, int x)    Move x pixels to the right. More...

void    OffsetY (const PixelData &data, int y)    Move y rows to the bottom. More...

void    MoveTo (const PixelData &data, int x, int y)    Go to the given position. More...

ChannelType &   Red ()    Data Access: Access to individual colour components. More...
ChannelType &   Green ()    Data Access: Access to individual colour components. More...
ChannelType &   Blue ()    Data Access: Access to individual colour components. More...
ChannelType &   Alpha ()    Data Access: Access to individual colour components. More...


   typedef PixelData<wxBitmap,wxNativePixelFormat> PixelData;
   PixelData data(skins_on ? *my_skin:*my_lisabitmap);
   if (!data) return;
   PixelData::Iterator p(data);
   p.Reset(data);
   p.MoveTo(data,ox,oy);
   ALERT_LOG(0,"RepaintAAG f raw bitmap, origin:%d,%d",ox,oy);

   for ( int y = 0; y < o_effective_lisa_vid_size_y; ++y )   // effective_lisa_vid_size_y
   {
       PixelData::Iterator rowStart = p;              // save the x,y coordinates at the start of the line

       for ( int x = 0; x < o_effective_lisa_vid_size_x; )
           { SETRGB16_AAG(x,y,  {p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p;} , {++p;} );}

       p.Red()=0; p.Green()=0; p.Blue()=0;

       p = rowStart; p.OffsetY(data, 1);              // restore the x,y coords from start of line, then increment y to do y++;
   }




*/

/*

#define PIXEL00_1M  *dp = Interp1(w[5], w[1]);
#define PIXEL00_1U  *dp = Interp1(w[5], w[2]);
#define PIXEL00_1L  *dp = Interp1(w[5], w[4]);
#define PIXEL00_2   *dp = Interp2(w[5], w[4], w[2]);
#define PIXEL00_4   *dp = Interp4(w[5], w[4], w[2]);
#define PIXEL00_5   *dp = Interp5(w[4], w[2]);
#define PIXEL00_C   *dp   = w[5];

#define PIXEL01_1   *(dp+1) = Interp1(w[5], w[2]);
#define PIXEL01_3   *(dp+1) = Interp3(w[5], w[2]);
#define PIXEL01_6   *(dp+1) = Interp1(w[2], w[5]);
#define PIXEL01_C   *(dp+1) = w[5];

#define PIXEL02_1M  *(dp+2) = Interp1(w[5], w[3]);
#define PIXEL02_1U  *(dp+2) = Interp1(w[5], w[2]);
#define PIXEL02_1R  *(dp+2) = Interp1(w[5], w[6]);
#define PIXEL02_2   *(dp+2) = Interp2(w[5], w[2], w[6]);
#define PIXEL02_4   *(dp+2) = Interp4(w[5], w[2], w[6]);
#define PIXEL02_5   *(dp+2) = Interp5(w[2], w[6]);
#define PIXEL02_C   *(dp+2) = w[5];

#define PIXEL10_1   *(dp+dpL) = Interp1(w[5], w[4]);
#define PIXEL10_3   *(dp+dpL) = Interp3(w[5], w[4]);
#define PIXEL10_6   *(dp+dpL) = Interp1(w[4], w[5]);
#define PIXEL10_C   *(dp+dpL) = w[5];

#define PIXEL11     *(dp+dpL+1) = w[5];

#define PIXEL12_1   *(dp+dpL+2) = Interp1(w[5], w[6]);
#define PIXEL12_3   *(dp+dpL+2) = Interp3(w[5], w[6]);
#define PIXEL12_6   *(dp+dpL+2) = Interp1(w[6], w[5]);
#define PIXEL12_C   *(dp+dpL+2) = w[5];

#define PIXEL20_1M  *(dp+dpL+dpL) = Interp1(w[5], w[7]);
#define PIXEL20_1D  *(dp+dpL+dpL) = Interp1(w[5], w[8]);
#define PIXEL20_1L  *(dp+dpL+dpL) = Interp1(w[5], w[4]);
#define PIXEL20_2   *(dp+dpL+dpL) = Interp2(w[5], w[8], w[4]);
#define PIXEL20_4   *(dp+dpL+dpL) = Interp4(w[5], w[8], w[4]);
#define PIXEL20_5   *(dp+dpL+dpL) = Interp5(w[8], w[4]);
#define PIXEL20_C   *(dp+dpL+dpL) = w[5];

#define PIXEL21_1   *(dp+dpL+dpL+1) = Interp1(w[5], w[8]);
#define PIXEL21_3   *(dp+dpL+dpL+1) = Interp3(w[5], w[8]);
#define PIXEL21_6   *(dp+dpL+dpL+1) = Interp1(w[8], w[5]);
#define PIXEL21_C   *(dp+dpL+dpL+1) = w[5];

#define PIXEL22_1M  *(dp+dpL+dpL+2) = Interp1(w[5], w[9]);
#define PIXEL22_1D  *(dp+dpL+dpL+2) = Interp1(w[5], w[8]);
#define PIXEL22_1R  *(dp+dpL+dpL+2) = Interp1(w[5], w[6]);
#define PIXEL22_2   *(dp+dpL+dpL+2) = Interp2(w[5], w[6], w[8]);
#define PIXEL22_4   *(dp+dpL+dpL+2) = Interp4(w[5], w[6], w[8]);
#define PIXEL22_5   *(dp+dpL+dpL+2) = Interp5(w[6], w[8]);
#define PIXEL22_C   *(dp+dpL+dpL+2) = w[5];

*/
//-----------------------------------------------//

// #define SETCOLOR(pi,val) {uint32 register v=(val>>24) 0xff; pi.Red()=v; pi.Blue()=MIN(v+EXTRABLUE,255); pi.Green()=v;}
#define SETCOLOR(pi, val)                       \
    {                                           \
        uint32 register v = (val >> 24) & 0xff; \
        pi.Red() = v;                           \
        pi.Blue() = v + EXTRABLUE;              \
        pi.Green() = v;                         \
        pi.Alpha() = 255;                       \
    }

#define PIXEL00_1M SETCOLOR(p00, Interp1(w[5], w[1]));
#define PIXEL00_1U SETCOLOR(p00, Interp1(w[5], w[2]));
#define PIXEL00_1L SETCOLOR(p00, Interp1(w[5], w[4]));
#define PIXEL00_2 SETCOLOR(p00, Interp2(w[5], w[4], w[2]));
#define PIXEL00_4 SETCOLOR(p00, Interp4(w[5], w[4], w[2]));
#define PIXEL00_5 SETCOLOR(p00, Interp5(w[4], w[2]));
#define PIXEL00_C SETCOLOR(p00, w[5]);

#define PIXEL01_1 SETCOLOR(p01, Interp1(w[5], w[2]));
#define PIXEL01_3 SETCOLOR(p01, Interp3(w[5], w[2]));
#define PIXEL01_6 SETCOLOR(p01, Interp1(w[2], w[5]));
#define PIXEL01_C SETCOLOR(p01, w[5]);

#define PIXEL02_1M SETCOLOR(p02, Interp1(w[5], w[3]));
#define PIXEL02_1U SETCOLOR(p02, Interp1(w[5], w[2]));
#define PIXEL02_1R SETCOLOR(p02, Interp1(w[5], w[6]));
#define PIXEL02_2 SETCOLOR(p02, Interp2(w[5], w[2], w[6]));
#define PIXEL02_4 SETCOLOR(p02, Interp4(w[5], w[2], w[6]));
#define PIXEL02_5 SETCOLOR(p02, Interp5(w[2], w[6]));
#define PIXEL02_C SETCOLOR(p02, w[5]);

#define PIXEL10_1 SETCOLOR(p10, Interp1(w[5], w[4]));
#define PIXEL10_3 SETCOLOR(p10, Interp3(w[5], w[4]));
#define PIXEL10_6 SETCOLOR(p10, Interp1(w[4], w[5]));
#define PIXEL10_C SETCOLOR(p10, w[5]);

#define PIXEL11 SETCOLOR(p11, w[5]);

#define PIXEL12_1 SETCOLOR(p12, Interp1(w[5], w[6]));
#define PIXEL12_3 SETCOLOR(p12, Interp3(w[5], w[6]));
#define PIXEL12_6 SETCOLOR(p12, Interp1(w[6], w[5]));
#define PIXEL12_C SETCOLOR(p12, w[5]);

#define PIXEL20_1M SETCOLOR(p20, Interp1(w[5], w[7]));
#define PIXEL20_1D SETCOLOR(p20, Interp1(w[5], w[8]));
#define PIXEL20_1L SETCOLOR(p20, Interp1(w[5], w[4]));
#define PIXEL20_2 SETCOLOR(p20, Interp2(w[5], w[8], w[4]));
#define PIXEL20_4 SETCOLOR(p20, Interp4(w[5], w[8], w[4]));
#define PIXEL20_5 SETCOLOR(p20, Interp5(w[8], w[4]));
#define PIXEL20_C SETCOLOR(p20, w[5]);

#define PIXEL21_1 SETCOLOR(p21, Interp1(w[5], w[8]));
#define PIXEL21_3 SETCOLOR(p21, Interp3(w[5], w[8]));
#define PIXEL21_6 SETCOLOR(p21, Interp1(w[8], w[5]));
#define PIXEL21_C SETCOLOR(p21, w[5]);

#define PIXEL22_1M SETCOLOR(p22, Interp1(w[5], w[9]));
#define PIXEL22_1D SETCOLOR(p22, Interp1(w[5], w[8]));
#define PIXEL22_1R SETCOLOR(p22, Interp1(w[5], w[6]));
#define PIXEL22_2 SETCOLOR(p22, Interp2(w[5], w[6], w[8]));
#define PIXEL22_4 SETCOLOR(p22, Interp4(w[5], w[6], w[8]));
#define PIXEL22_5 SETCOLOR(p22, Interp5(w[6], w[8]));
#define PIXEL22_C SETCOLOR(p22, w[5]);

// typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
typedef wxPixelData<wxBitmap, wxImagePixelData> PixelData;

// PixelData data(skins_on ? *my_skin:*my_lisabitmap);
// if (!data) return;
//     PixelData::Iterator p(data);
//     p.Reset(data);
//     p.MoveTo(data,ox,oy);

/*
hq3x.cpp:258:51: error: ‘typedef class PixelData<wxBitmap, wxPixelFormat<unsigned char, 24, 0, 1, 2> > PixelData’ redeclared as different kind of symbol
 typedef PixelData<wxBitmap,wxNativePixelFormat> PixelData;
                                                   ^~~~~~~~~~~
In file included from hq3x.cpp:35:0:
/usr/local/wx3.1.2-gtk/include/wx-3.1/wx/rawbmp.h:689:7: note: previous declaration ‘template<class Image, class PixelFormat> class PixelData’
 class PixelData :
       ^~~~~~~~~~~
hq3x.cpp:262:65: error: ‘PixelData’ is not a type
 HQX_API void HQX_CALLCONV hq3x_32_rb( uint8 * sp, uint32 srb,   PixelData *data, uint32 drb,       int Xres, int Yres, uint32 brightness )
                                                                 ^~~~~~~~~~~*/

// ::TODO:: - for future use - will have to adapt this to work properly here. this code expects
// two x 3 bytes in, and 16 bytes out, but hq3x works on individual bits which are lensed 2x wide
// an adapter version of this needs to be called before the 2x lens and passed to hq3x as a half
// bright value. We use 0x80+bright, so maybe 0x40+bright or 0x60+bright
// (and + the extra blue phosphor shift)
#ifdef __FUTURE_CODE_DO_NOT_ENABLE_YET_
/***************************************************************************************************\
    graymap[] are set to 8 which is the magical OR value into the color contrast table that forces
    gray replacement.  See ContrastChange() for details.  The graymap is stored as a 3 dimentional
    array, but we can make use of shifts and OR's to avoid multiplication that [][][] would be
    involved in dereferencing.  It's possible some compilers will optimize it properly, but why
    take a chance?  Access is equivalent to   color | graymap[(vup<<4) | (val<<2) | vdn];

    Since we work 2 bits at a time, we need to replace both bits with a gray together.  This is why
    we have retval[1]=retval[0] and so on.   It would be possible to modify the code to do something
    like retval & 14 to avoid the copy, but it wouldn't be any faster and would make the code more
    complicated.

    This function is only called once per every 16 horizontal pixels processed for speed which is
    why it needs to store its results in an array.

\***************************************************************************************************/

static inline void getgraymap(uint16 up, uint16 val, uint16 dn, uint8 *retval)
{
    static uint8 graymap[4 * 4 * 4] = {
        0,
        0,
        0,
        0,
        0,
        0,
        8,
        0,
        0,
        8,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        8,
        8,
        0,
        8,
        0,
        8,
        0,
        0,
        0,
        0,
        0,
        0,
        8,
        0,
        8,
        8,
        0,
        0,
        0,
        0,
        0,
        0,
        8,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        8,
        8,
        0,
        8,
        0,
        8,
        0,
        0,
        0,
        0,
    };

    // maybe add a case here for the bit number and just return that bit
    // static int retval[16];
    retval[0] = retval[1] = graymap[(((BIT15 + BIT14) & up) >> 10) | (((BIT15 + BIT14) & val) >> 12) | (((BIT15 + BIT14) & dn) >> 14)];
    retval[2] = retval[3] = graymap[(((BIT13 + BIT12) & up) >> 8) | (((BIT13 + BIT12) & val) >> 10) | (((BIT13 + BIT12) & dn) >> 12)];
    retval[4] = retval[5] = graymap[(((BIT11 + BIT10) & up) >> 6) | (((BIT11 + BIT10) & val) >> 8) | (((BIT11 + BIT10) & dn) >> 10)];
    retval[6] = retval[7] = graymap[(((BIT9 + BIT8) & up) >> 4) | (((BIT9 + BIT8) & val) >> 6) | (((BIT9 + BIT8) & dn) >> 8)];
    // split here
    retval[8] = retval[9] = graymap[(((BIT7 + BIT6) & up) >> 2) | (((BIT7 + BIT6) & val) >> 4) | (((BIT7 + BIT6) & dn) >> 6)];
    retval[10] = retval[11] = graymap[(((BIT5 + BIT4) & up)) | (((BIT5 + BIT4) & val) >> 2) | (((BIT5 + BIT4) & dn) >> 4)];
    retval[12] = retval[13] = graymap[(((BIT3 + BIT2) & up) << 2) | (((BIT3 + BIT2) & val)) | (((BIT3 + BIT2) & dn) >> 2)];
    retval[14] = retval[15] = graymap[(((BIT1 + BIT0) & up) << 4) | (((BIT1 + BIT0) & val) << 2) | (((BIT1 + BIT0) & dn))];
}
#endif

// input and output originally expected as 4 and 3 bytes per pixel respectively.
//                                  *sourceptr,      rowbytesL=width*4, destpointer,   (rowbytesL*4)*3,    width,height,  ** add brightness since outside **

//     PixelData data(skins_on ? *my_skin:*my_lisabitmap);  <- data comes from the image

// ::TODO:: add params for rectangle to update to save cycles
HQX_API void HQX_CALLCONV hq3x_32_rb(int startx, int starty, int width, int height, int rowbytes, wxBitmap *mybitmap, int Xres, int Yres, uint32 brightness) // int xl,int yt,int xr,int yb
{
    int x, y, k; // x,y iterators, k is circumcentral pixel iterator, xx=pixel x coord

    uint8 *sp, *up, *dn;

    int prevline, nextline;
    uint32 w[10]; // source pixels around center
    uint32 yuv1, yuv2;

    uint32 bright = (brightness) << 24;
    uint32 black = 0x00000000;

    // fprintf(stderr,"bright:%08x brightness:%08x\n",bright,brightness);

    if (!rowbytes)
        rowbytes = 90;

    // might want to move these outside, not sure

    // typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
    typedef wxPixelData<wxBitmap, wxAlphaPixelFormat> PixelData;
    PixelData data(*mybitmap);
    if (!data)
        return;
    // data.UseAlpha();
    //  Reset pixel iterators - these are for the output wxBitmap so don't divide their sizes at all
    PixelData::Iterator p00(data);
    p00.Reset(data);
    p00.MoveTo(data, startx + 0, starty + 0);
    PixelData::Iterator p01(data);
    p01.Reset(data);
    p01.MoveTo(data, startx + 1, starty + 0);
    PixelData::Iterator p02(data);
    p02.Reset(data);
    p02.MoveTo(data, startx + 2, starty + 0);

    PixelData::Iterator p10(data);
    p10.Reset(data);
    p10.MoveTo(data, startx + 0, starty + 1);
    PixelData::Iterator p11(data);
    p11.Reset(data);
    p11.MoveTo(data, startx + 1, starty + 1);
    PixelData::Iterator p12(data);
    p12.Reset(data);
    p12.MoveTo(data, startx + 2, starty + 1);

    PixelData::Iterator p20(data);
    p20.Reset(data);
    p20.MoveTo(data, startx + 0, starty + 2);
    PixelData::Iterator p21(data);
    p21.Reset(data);
    p21.MoveTo(data, startx + 1, starty + 2);
    PixelData::Iterator p22(data);
    p22.Reset(data);
    p22.MoveTo(data, startx + 2, starty + 2);

    PixelData::Iterator p00rowStart(data);
    p00rowStart = p00;

    // j=y coordinate, i=x coordinate.
    //   +----+----+----+
    //   |    |    |    |
    //   | w1 | w2 | w3 |
    //   +----+----+----+
    //   |    |    |    |
    //   | w4 | w5 | w6 |
    //   +----+----+----+
    //   |    |    |    |
    //   | w7 | w8 | w9 |
    //   +----+----+----+

    // *** need to pass these ***
    extern uint8 *lisaram; // pointer to Lisa RAM
    extern uint32 videolatchaddress;

    // unused//int max_height=mybitmap->GetHeight();
    // unused//int max_width =mybitmap->GetWidth();

    sp = &lisaram[videolatchaddress + (starty / 3) * rowbytes]; // initially center and up are the same
    up = &lisaram[videolatchaddress + (starty / 3) * rowbytes];
    dn = &lisaram[videolatchaddress + (starty / 3) * rowbytes + rowbytes]; // down is one rowbytes line down

// x= x, dy=delta y, rowsize=76 or 90 - these are for the inputs from lisa video ram -
// might want to eventually add gray detection/replacement code here too
// #define GETPIXEL(x,ptr)   (  (double_video_bits[ptr[(((x)/2)>>4)        ]] & ( (1<<(15-(((x)/2) & 15))) )) ? black: bright  )
#define GETPIXEL(x, ptr) ((ptr[(((x) / 2) >> 3)] & ((1 << (7 - (((x) / 2) & 7))))) ? black : bright)

    // if (rowbytes==90) Xres=Xres * 2; // non-3A video

    //    fprintf(stderr,"start: %d,%d size:%d,%d\n",startx,starty,width,height);

    if ((starty + height) > (364 * 3) || (startx + width) > (720 * 2))
    {
        //      fprintf(stderr,"!!! got oversized update, reset to proper coordinates !!!");
        startx = 0;
        starty = 0;
        width = 720 * 2;
        height = 364 * 3;
    }

    // x and y are in wxBitmap scaled so 2x,3y
    for (y = starty; y < starty + height; y += 3) // y<Yres/3
    {
        // -----------------------------------------------------------------------------------------------------
        // to do don't update sp on every cursor right scan
        // might be able to optimize a bit more by adding a 3rd inner loop for X
        // where sp gets ++ on every 16 increments of x, not sure if it will make it worse
        // might be able to turn the whole block into a macro and eliminate these two Y if
        // statements by have 3 y blocks: one for y=0, one for loop between y=1..y<Yres-1, one for Y=Yres-1
        // -----------------------------------------------------------------------------------------------------

        // note that prevline is negative and it's added hence the meth below might look a bit weird if you
        // don't notice these two lines here.
        if (y / 3 > 0)
        {
            prevline = -rowbytes;
            up = sp - rowbytes;
        }
        else
        {
            prevline = 0;
            up = sp;
        }
        if (y / 3 < Yres - 1)
        {
            nextline = rowbytes;
            dn = sp + rowbytes;
        }
        else
        {
            nextline = 0;
            dn = sp;
        }

        for (int tripple = 0; tripple < 3; tripple++)
        { // this loop is used to tripple each horizontal line since we do y*3
          // without it there would be gaps
            for (x = startx; x < startx + width; x++)
            {
                // :TODO: maybe convert this entire set of blocks to a macro so we can skip the y==0, y==Yres-1 checks
                // and the X==0 X<Xres-1 checks - possibly premature optimization, but might be doable.
                w[2] = GETPIXEL(x, up); //(*(sp + prevline) & (1<<(7-((x  )&7) ))) ? bright:0x00;      // w[2] = *(sp + prevline);
                w[5] = GETPIXEL(x, sp); //(*(sp           ) & (1<<(7-((x  )&7) ))) ? bright:0x00;      // w[5] = *sp;
                w[8] = GETPIXEL(x, dn); // (*(sp + nextline) & (1<<(7-((x  )&7) ))) ? bright:0x00;      // w[8] = *(sp + nextline);

                if (x > 0) // 1,4,7 are the left column
                {
                    w[1] = GETPIXEL(x - 1, up); //(*(sp + prevline) & (1<<(7-((x-1)&7) ))) ? bright:0x00;  //*(sp + prevline - 1);
                    w[4] = GETPIXEL(x - 1, sp); //(*(sp           ) & (1<<(7-((x-1)&7) ))) ? bright:0x00;  //*(sp - 1);
                    w[7] = GETPIXEL(x - 1, dn); //(*(sp + nextline) & (1<<(7-((x-1)&7) ))) ? bright:0x00;  //*(sp + nextline - 1);
                }
                else
                {
                    w[1] = w[2];
                    w[4] = w[5];
                    w[7] = w[8];
                }

                if (x < Xres - 1)
                {
                    w[3] = GETPIXEL(x + 1, up); //*(sp + prevline + 1);
                    w[6] = GETPIXEL(x + 1, sp); //*(sp + 1);
                    w[9] = GETPIXEL(x + 1, dn); //*(sp + nextline + 1);
                }
                else
                {
                    w[3] = w[2];
                    w[6] = w[5];
                    w[9] = w[8];
                }

                int pattern = 0;
                int flag = 1;

                yuv1 = rgb_to_yuv(w[5]);

                for (k = 1; k <= 9; k++)
                {
                    if (k == 5)
                        continue;

                    if (w[k] != w[5])
                    {
                        yuv2 = rgb_to_yuv(w[k]);
                        if (yuv_diff(yuv1, yuv2))
                            pattern |= flag;
                    }
                    flag <<= 1;
                }

                switch (pattern)
                {
                case 0:
                case 1:
                case 4:
                case 32:
                case 128:
                case 5:
                case 132:
                case 160:
                case 33:
                case 129:
                case 36:
                case 133:
                case 164:
                case 161:
                case 37:
                case 165:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 2:
                case 34:
                case 130:
                case 162:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 16:
                case 17:
                case 48:
                case 49:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 64:
                case 65:
                case 68:
                case 69:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 8:
                case 12:
                case 136:
                case 140:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 3:
                case 35:
                case 131:
                case 163:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 6:
                case 38:
                case 134:
                case 166:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 20:
                case 21:
                case 52:
                case 53:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 144:
                case 145:
                case 176:
                case 177:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 192:
                case 193:
                case 196:
                case 197:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 96:
                case 97:
                case 100:
                case 101:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 40:
                case 44:
                case 168:
                case 172:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 9:
                case 13:
                case 137:
                case 141:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 18:
                case 50:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_1M
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 80:
                case 81:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 72:
                case 76:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_1M
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 10:
                case 138:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 66:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 24:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 7:
                case 39:
                case 135:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 148:
                case 149:
                case 180:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 224:
                case 228:
                case 225:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 41:
                case 169:
                case 45:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 22:
                case 54:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 208:
                case 209:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 104:
                case 108:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 11:
                case 139:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 19:
                case 51:
                {
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL00_1L
                        PIXEL01_C
                        PIXEL02_1M
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL00_2
                        PIXEL01_6
                        PIXEL02_5
                        PIXEL12_1
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 146:
                case 178:
                {
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_1M
                        PIXEL12_C
                        PIXEL22_1D
                    }
                    else
                    {
                        PIXEL01_1
                        PIXEL02_5
                        PIXEL12_6
                        PIXEL22_2
                    }
                    PIXEL00_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_2
                    PIXEL21_1
                    break;
                }
                case 84:
                case 85:
                {
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL02_1U
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL02_2
                        PIXEL12_6
                        PIXEL21_1
                        PIXEL22_5
                    }
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    break;
                }
                case 112:
                case 113:
                {
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL20_1L
                        PIXEL21_C
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL12_1
                        PIXEL20_2
                        PIXEL21_6
                        PIXEL22_5
                    }
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    break;
                }
                case 200:
                case 204:
                {
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_1M
                        PIXEL21_C
                        PIXEL22_1R
                    }
                    else
                    {
                        PIXEL10_1
                        PIXEL20_5
                        PIXEL21_6
                        PIXEL22_2
                    }
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL11
                    PIXEL12_1
                    break;
                }
                case 73:
                case 77:
                {
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL00_1U
                        PIXEL10_C
                        PIXEL20_1M
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL00_2
                        PIXEL10_6
                        PIXEL20_5
                        PIXEL21_1
                    }
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL11
                    PIXEL12_1
                    PIXEL22_1M
                    break;
                }
                case 42:
                case 170:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                        PIXEL01_C
                        PIXEL10_C
                        PIXEL20_1D
                    }
                    else
                    {
                        PIXEL00_5
                        PIXEL01_1
                        PIXEL10_6
                        PIXEL20_2
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 14:
                case 142:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                        PIXEL01_C
                        PIXEL02_1R
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_5
                        PIXEL01_6
                        PIXEL02_2
                        PIXEL10_1
                    }
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 67:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 70:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 28:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 152:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 194:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 98:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 56:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 25:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 26:
                case 31:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL10_3
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL11
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 82:
                case 214:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 88:
                case 248:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL22_4
                    }
                    break;
                }
                case 74:
                case 107:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                    }
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 27:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 86:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 216:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 106:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 30:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 210:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 120:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 75:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 29:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 198:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 184:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 99:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 57:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 71:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 156:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 226:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 60:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 195:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 102:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 153:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 58:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 83:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 92:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 202:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 78:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 154:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 114:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1L
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 89:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 90:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 55:
                case 23:
                {
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL00_1L
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL00_2
                        PIXEL01_6
                        PIXEL02_5
                        PIXEL12_1
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 182:
                case 150:
                {
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                        PIXEL22_1D
                    }
                    else
                    {
                        PIXEL01_1
                        PIXEL02_5
                        PIXEL12_6
                        PIXEL22_2
                    }
                    PIXEL00_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_2
                    PIXEL21_1
                    break;
                }
                case 213:
                case 212:
                {
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL02_1U
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL02_2
                        PIXEL12_6
                        PIXEL21_1
                        PIXEL22_5
                    }
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    break;
                }
                case 241:
                case 240:
                {
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL20_1L
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_1
                        PIXEL20_2
                        PIXEL21_6
                        PIXEL22_5
                    }
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    break;
                }
                case 236:
                case 232:
                {
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                        PIXEL22_1R
                    }
                    else
                    {
                        PIXEL10_1
                        PIXEL20_5
                        PIXEL21_6
                        PIXEL22_2
                    }
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL11
                    PIXEL12_1
                    break;
                }
                case 109:
                case 105:
                {
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL00_1U
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL00_2
                        PIXEL10_6
                        PIXEL20_5
                        PIXEL21_1
                    }
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL11
                    PIXEL12_1
                    PIXEL22_1M
                    break;
                }
                case 171:
                case 43:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                        PIXEL20_1D
                    }
                    else
                    {
                        PIXEL00_5
                        PIXEL01_1
                        PIXEL10_6
                        PIXEL20_2
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 143:
                case 15:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL02_1R
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_5
                        PIXEL01_6
                        PIXEL02_2
                        PIXEL10_1
                    }
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 124:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 203:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 62:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 211:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 118:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 217:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 110:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 155:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 188:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 185:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 61:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 157:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 103:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 227:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 230:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 199:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 220:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 158:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 234:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1R
                    break;
                }
                case 242:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1L
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 59:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 121:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 87:
                {
                    PIXEL00_1L
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1M
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 79:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1R
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 122:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 94:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_C
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 218:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_C
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 91:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 229:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 167:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 173:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 181:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 186:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 115:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1L
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 93:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 206:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 205:
                case 201:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_1M
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 174:
                case 46:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_1M
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 179:
                case 147:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_1M
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 117:
                case 116:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1L
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_1M
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 189:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 231:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 126:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 219:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                        PIXEL10_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 125:
                {
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL00_1U
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL00_2
                        PIXEL10_6
                        PIXEL20_5
                        PIXEL21_1
                    }
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL11
                    PIXEL12_C
                    PIXEL22_1M
                    break;
                }
                case 221:
                {
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL02_1U
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL02_2
                        PIXEL12_6
                        PIXEL21_1
                        PIXEL22_5
                    }
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1M
                    break;
                }
                case 207:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL02_1R
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_5
                        PIXEL01_6
                        PIXEL02_2
                        PIXEL10_1
                    }
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 238:
                {
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                        PIXEL22_1R
                    }
                    else
                    {
                        PIXEL10_1
                        PIXEL20_5
                        PIXEL21_6
                        PIXEL22_2
                    }
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL11
                    PIXEL12_1
                    break;
                }
                case 190:
                {
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                        PIXEL22_1D
                    }
                    else
                    {
                        PIXEL01_1
                        PIXEL02_5
                        PIXEL12_6
                        PIXEL22_2
                    }
                    PIXEL00_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1D
                    PIXEL21_1
                    break;
                }
                case 187:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                        PIXEL20_1D
                    }
                    else
                    {
                        PIXEL00_5
                        PIXEL01_1
                        PIXEL10_6
                        PIXEL20_2
                    }
                    PIXEL02_1M
                    PIXEL11
                    PIXEL12_C
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 243:
                {
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL20_1L
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_1
                        PIXEL20_2
                        PIXEL21_6
                        PIXEL22_5
                    }
                    PIXEL00_1L
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL10_1
                    PIXEL11
                    break;
                }
                case 119:
                {
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL00_1L
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL00_2
                        PIXEL01_6
                        PIXEL02_5
                        PIXEL12_1
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL20_1L
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 237:
                case 233:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_2
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 175:
                case 47:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_2
                    break;
                }
                case 183:
                case 151:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_2
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 245:
                case 244:
                {
                    PIXEL00_2
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1L
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 250:
                {
                    PIXEL00_1M
                    PIXEL01_C
                    PIXEL02_1M
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL22_4
                    }
                    break;
                }
                case 123:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                    }
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 95:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL10_3
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL11
                    PIXEL20_1M
                    PIXEL21_C
                    PIXEL22_1M
                    break;
                }
                case 222:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 252:
                {
                    PIXEL00_1M
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 249:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL22_4
                    }
                    break;
                }
                case 235:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                    }
                    PIXEL02_1M
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 111:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 63:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1M
                    break;
                }
                case 159:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL10_3
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 215:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 246:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1L
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 254:
                {
                    PIXEL00_1M
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_4
                    }
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_4
                    }
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL21_3
                        PIXEL22_2
                    }
                    break;
                }
                case 253:
                {
                    PIXEL00_1U
                    PIXEL01_1
                    PIXEL02_1U
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 251:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL01_3
                    }
                    PIXEL02_1M
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL10_C
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL10_3
                        PIXEL20_2
                        PIXEL21_3
                    }
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL12_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL12_3
                        PIXEL22_4
                    }
                    break;
                }
                case 239:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    PIXEL02_1R
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_1
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    PIXEL22_1R
                    break;
                }
                case 127:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL01_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_2
                        PIXEL01_3
                        PIXEL10_3
                    }
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL02_4
                        PIXEL12_3
                    }
                    PIXEL11
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                        PIXEL21_C
                    }
                    else
                    {
                        PIXEL20_4
                        PIXEL21_3
                    }
                    PIXEL22_1M
                    break;
                }
                case 191:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1D
                    PIXEL21_1
                    PIXEL22_1D
                    break;
                }
                case 223:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                        PIXEL10_C
                    }
                    else
                    {
                        PIXEL00_4
                        PIXEL10_3
                    }
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL01_C
                        PIXEL02_C
                        PIXEL12_C
                    }
                    else
                    {
                        PIXEL01_3
                        PIXEL02_2
                        PIXEL12_3
                    }
                    PIXEL11
                    PIXEL20_1M
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL21_C
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL21_3
                        PIXEL22_4
                    }
                    break;
                }
                case 247:
                {
                    PIXEL00_1L
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_1
                    PIXEL11
                    PIXEL12_C
                    PIXEL20_1L
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                case 255:
                {
                    if (Diff(w[4], w[2]))
                    {
                        PIXEL00_C
                    }
                    else
                    {
                        PIXEL00_2
                    }
                    PIXEL01_C
                    if (Diff(w[2], w[6]))
                    {
                        PIXEL02_C
                    }
                    else
                    {
                        PIXEL02_2
                    }
                    PIXEL10_C
                    PIXEL11
                    PIXEL12_C
                    if (Diff(w[8], w[4]))
                    {
                        PIXEL20_C
                    }
                    else
                    {
                        PIXEL20_2
                    }
                    PIXEL21_C
                    if (Diff(w[6], w[8]))
                    {
                        PIXEL22_C
                    }
                    else
                    {
                        PIXEL22_2
                    }
                    break;
                }
                }
                //            sp++; up++; dn++;
                // dp += 3;
                ++p00;
                ++p01;
                ++p02;
                ++p10;
                ++p11;
                ++p12;
                ++p20;
                ++p21;
                ++p22;
            }
            // sRowP += srb;
            //  next line
            // pblock
            p00 = p00rowStart;
            p00.OffsetY(data, 1);
            p01 = p00rowStart;
            p01.OffsetY(data, 1);
            ++p01;
            p02 = p00rowStart;
            p02.OffsetY(data, 1);
            ++p02;
            ++p02;

            p10 = p00rowStart;
            p10.OffsetY(data, 1);
            p11 = p00rowStart;
            p11.OffsetY(data, 1);
            ++p11;
            p12 = p00rowStart;
            p12.OffsetY(data, 1);
            ++p12;
            ++p12;

            p20 = p00rowStart;
            p20.OffsetY(data, 1);
            p21 = p00rowStart;
            p21.OffsetY(data, 1);
            ++p21;
            p22 = p00rowStart;
            p22.OffsetY(data, 1);
            ++p22;
            ++p22;

            // setup next rowstart
            p00rowStart = p00;
        }
        up = sp;
        sp = sp + rowbytes;
        dn = sp + rowbytes;

        // dRowP += drb * 3;
        // dp =  dRowP;
        //  reset all 9 pointers to the next row, and then adjust as needed
        //  pblock was here
    }

    // fprintf(stderr,"hq3x: testpixel:%08x\n",testpixel);
}

// this expects 4 bytes per pixel, we only do 1 bit per pixel.
//        hq3x_32(sp, dp, width, height);
// sp=source pointer, dp=dest pointer, expects 4 byte pixels.

// HQX_API void HQX_CALLCONV hq3x_32(wxBitmap *mybitmap, int Xres, int Yres, uint32 brightness )
//{
//     //uint32 rowBytesL = Xres * 4;                            // *** here here here //
//  HQX_API void HQX_CALLCONV hq3x_32_rb(       uint8 * sp, uint32 srb,     uint32 * dp, uint32 drb,       int Xres, int Yres, uint32 brightness )
//         hq3x_32_rb( 90,mybitmap, Xres,     Yres, brightness);
//
// }
//
// HQX_API void HQX_CALLCONV hq3x_32_3a(wxBitmap *mybitmap, int Xres, int Yres, uint32 brightness )
//{
//     //uint32 rowBytesL = Xres * 4;                            // *** here here here //
//// HQX_API void HQX_CALLCONV hq3x_32_rb(       uint8 * sp, uint32 srb,     uint32 * dp, uint32 drb,       int Xres, int Yres, uint32 brightness )
//        hq3x_32_rb( 76,mybitmap,  Xres,     Yres, brightness);
//
//}