/*
 * Copyright (C) 2003 Maxim Stepin ( maxst@hiend3d.com )
 *
 * Copyright (C) 2010 Cameron Zemek ( grom@zeminvaders.net)
 * Copyright (C) 2011 Francois Gannaz <mytskine@gmail.com>
 *
 * Edits for LisaEm use, Copyright (C) 2018 Ray Arachelian
 *                   <ray@arachelian.com>
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

#ifndef __HQX_COMMON_H_
#define __HQX_COMMON_H_

#include <stdlib.h>
#include <stdint.h>

// We're doing the equivalent of 2x3y mode, but using hqx3 (3x3) so this doubles a byte
// into a word horizontally
static uint16 const double_video_bits[] = {
    //            +0      +1      +2      +3      +4      +5      +6      +7      +8      +9      +a      +b      +c      +d      +e      +f
    /* 00 */ 0x0000, 0x0003, 0x000c, 0x000f, 0x0030, 0x0033, 0x003c, 0x003f, 0x00c0, 0x00c3, 0x00cc, 0x00cf, 0x00f0, 0x00f3, 0x00fc, 0x00ff,
    /* 10 */ 0x0300, 0x0303, 0x030c, 0x030f, 0x0330, 0x0333, 0x033c, 0x033f, 0x03c0, 0x03c3, 0x03cc, 0x03cf, 0x03f0, 0x03f3, 0x03fc, 0x03ff,
    /* 20 */ 0x0c00, 0x0c03, 0x0c0c, 0x0c0f, 0x0c30, 0x0c33, 0x0c3c, 0x0c3f, 0x0cc0, 0x0cc3, 0x0ccc, 0x0ccf, 0x0cf0, 0x0cf3, 0x0cfc, 0x0cff,
    /* 30 */ 0x0f00, 0x0f03, 0x0f0c, 0x0f0f, 0x0f30, 0x0f33, 0x0f3c, 0x0f3f, 0x0fc0, 0x0fc3, 0x0fcc, 0x0fcf, 0x0ff0, 0x0ff3, 0x0ffc, 0x0fff,
    /* 40 */ 0x3000, 0x3003, 0x300c, 0x300f, 0x3030, 0x3033, 0x303c, 0x303f, 0x30c0, 0x30c3, 0x30cc, 0x30cf, 0x30f0, 0x30f3, 0x30fc, 0x30ff,
    /* 50 */ 0x3300, 0x3303, 0x330c, 0x330f, 0x3330, 0x3333, 0x333c, 0x333f, 0x33c0, 0x33c3, 0x33cc, 0x33cf, 0x33f0, 0x33f3, 0x33fc, 0x33ff,
    /* 60 */ 0x3c00, 0x3c03, 0x3c0c, 0x3c0f, 0x3c30, 0x3c33, 0x3c3c, 0x3c3f, 0x3cc0, 0x3cc3, 0x3ccc, 0x3ccf, 0x3cf0, 0x3cf3, 0x3cfc, 0x3cff,
    /* 70 */ 0x3f00, 0x3f03, 0x3f0c, 0x3f0f, 0x3f30, 0x3f33, 0x3f3c, 0x3f3f, 0x3fc0, 0x3fc3, 0x3fcc, 0x3fcf, 0x3ff0, 0x3ff3, 0x3ffc, 0x3fff,
    /* 80 */ 0xc000, 0xc003, 0xc00c, 0xc00f, 0xc030, 0xc033, 0xc03c, 0xc03f, 0xc0c0, 0xc0c3, 0xc0cc, 0xc0cf, 0xc0f0, 0xc0f3, 0xc0fc, 0xc0ff,
    /* 90 */ 0xc300, 0xc303, 0xc30c, 0xc30f, 0xc330, 0xc333, 0xc33c, 0xc33f, 0xc3c0, 0xc3c3, 0xc3cc, 0xc3cf, 0xc3f0, 0xc3f3, 0xc3fc, 0xc3ff,
    /* a0 */ 0xcc00, 0xcc03, 0xcc0c, 0xcc0f, 0xcc30, 0xcc33, 0xcc3c, 0xcc3f, 0xccc0, 0xccc3, 0xcccc, 0xcccf, 0xccf0, 0xccf3, 0xccfc, 0xccff,
    /* b0 */ 0xcf00, 0xcf03, 0xcf0c, 0xcf0f, 0xcf30, 0xcf33, 0xcf3c, 0xcf3f, 0xcfc0, 0xcfc3, 0xcfcc, 0xcfcf, 0xcff0, 0xcff3, 0xcffc, 0xcfff,
    /* c0 */ 0xf000, 0xf003, 0xf00c, 0xf00f, 0xf030, 0xf033, 0xf03c, 0xf03f, 0xf0c0, 0xf0c3, 0xf0cc, 0xf0cf, 0xf0f0, 0xf0f3, 0xf0fc, 0xf0ff,
    /* d0 */ 0xf300, 0xf303, 0xf30c, 0xf30f, 0xf330, 0xf333, 0xf33c, 0xf33f, 0xf3c0, 0xf3c3, 0xf3cc, 0xf3cf, 0xf3f0, 0xf3f3, 0xf3fc, 0xf3ff,
    /* e0 */ 0xfc00, 0xfc03, 0xfc0c, 0xfc0f, 0xfc30, 0xfc33, 0xfc3c, 0xfc3f, 0xfcc0, 0xfcc3, 0xfccc, 0xfccf, 0xfcf0, 0xfcf3, 0xfcfc, 0xfcff,
    /* f0 */ 0xff00, 0xff03, 0xff0c, 0xff0f, 0xff30, 0xff33, 0xff3c, 0xff3f, 0xffc0, 0xffc3, 0xffcc, 0xffcf, 0xfff0, 0xfff3, 0xfffc, 0xffff};

#define MASK_2 0x0000FF00
#define MASK_13 0x00FF00FF
#define MASK_RGB 0x00FFFFFF
#define MASK_ALPHA 0xFF000000

#define Ymask 0x00FF0000
#define Umask 0x0000FF00
#define Vmask 0x000000FF
#define trY 0x00300000
#define trU 0x00000700
#define trV 0x00000006

/* RGB to YUV lookup table */
// extern uint32 RGBtoYUV[16777216];  //RA20180410 no longer used

// 0xXXRRGGBB in, 0xYYUUVV out
// should always return brightness back in all 4.
// #define MASK_RGB   0x00FFFFFF
static inline uint32 rgb_to_yuv(uint32 c)
{
    // Mask against MASK_RGB to discard the alpha channel
    // return RGBtoYUV[MASK_RGB & c];
    return ((c & 0xff) << 16) | 0x8080; // RA20180410
}

/* Test if there is difference in color */
static inline int yuv_diff(uint32 yuv1, uint32 yuv2)
{
    return ((labs((long)((yuv1 & Ymask) - (yuv2 & Ymask))) > trY)); /* ||  // RA20180410 - Lisa is B&W so no color testing needed.
        ( labs((long) ((yuv1 & Umask) - (yuv2 & Umask)) ) > trU ) ||
        ( labs((long) ((yuv1 & Vmask) - (yuv2 & Vmask)) ) > trV ) ); */
}

static inline int Diff(uint32 c1, uint32 c2)
{
    return yuv_diff(rgb_to_yuv(c1), rgb_to_yuv(c2));
}

/* Interpolate functions */
static inline uint32 Interpolate_2(uint32 c1, int w1, uint32 c2, int w2, int s)
{
    if (c1 == c2)
    {
        return c1;
    }
    return (((((c1 & MASK_ALPHA) >> 24) * w1 + ((c2 & MASK_ALPHA) >> 24) * w2) << (24 - s)) & MASK_ALPHA) +
           ((((c1 & MASK_2) * w1 + (c2 & MASK_2) * w2) >> s) & MASK_2) +
           ((((c1 & MASK_13) * w1 + (c2 & MASK_13) * w2) >> s) & MASK_13);
}

static inline uint32 Interpolate_3(uint32 c1, int w1, uint32 c2, int w2, uint32 c3, int w3, int s)
{
    return (((((c1 & MASK_ALPHA) >> 24) * w1 + ((c2 & MASK_ALPHA) >> 24) * w2 + ((c3 & MASK_ALPHA) >> 24) * w3) << (24 - s)) & MASK_ALPHA) +
           ((((c1 & MASK_2) * w1 + (c2 & MASK_2) * w2 + (c3 & MASK_2) * w3) >> s) & MASK_2) +
           ((((c1 & MASK_13) * w1 + (c2 & MASK_13) * w2 + (c3 & MASK_13) * w3) >> s) & MASK_13);
}

static inline uint32 Interp1(uint32 c1, uint32 c2)
{
    //(c1*3+c2) >> 2;
    return Interpolate_2(c1, 3, c2, 1, 2);
}

static inline uint32 Interp2(uint32 c1, uint32 c2, uint32 c3)
{
    //(c1*2+c2+c3) >> 2;
    return Interpolate_3(c1, 2, c2, 1, c3, 1, 2);
}

static inline uint32 Interp3(uint32 c1, uint32 c2)
{
    //(c1*7+c2)/8;
    return Interpolate_2(c1, 7, c2, 1, 3);
}

static inline uint32 Interp4(uint32 c1, uint32 c2, uint32 c3)
{
    //(c1*2+(c2+c3)*7)/16;
    return Interpolate_3(c1, 2, c2, 7, c3, 7, 4);
}

static inline uint32 Interp5(uint32 c1, uint32 c2)
{
    //(c1+c2) >> 1;
    return Interpolate_2(c1, 1, c2, 1, 1);
}

static inline uint32 Interp6(uint32 c1, uint32 c2, uint32 c3)
{
    //(c1*5+c2*2+c3)/8;
    return Interpolate_3(c1, 5, c2, 2, c3, 1, 3);
}

static inline uint32 Interp7(uint32 c1, uint32 c2, uint32 c3)
{
    //(c1*6+c2+c3)/8;
    return Interpolate_3(c1, 6, c2, 1, c3, 1, 3);
}

static inline uint32 Interp8(uint32 c1, uint32 c2)
{
    //(c1*5+c2*3)/8;
    return Interpolate_2(c1, 5, c2, 3, 3);
}

static inline uint32 Interp9(uint32 c1, uint32 c2, uint32 c3)
{
    //(c1*2+(c2+c3)*3)/8;
    return Interpolate_3(c1, 2, c2, 3, c3, 3, 3);
}

static inline uint32 Interp10(uint32 c1, uint32 c2, uint32 c3)
{
    //(c1*14+c2+c3)/16;
    return Interpolate_3(c1, 14, c2, 1, c3, 1, 4);
}

#endif
