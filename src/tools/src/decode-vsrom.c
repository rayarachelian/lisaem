/*******************************************************************************\
*                  Decode Apple Lisa VSROM Serial Number                        *
*       Copyright (C) 2021 by Ray Arachelian, All Rights Reserved               *
*                      https://lisaem.sunder.net                                *
*                                                                               *
*               Released under the terms of the GNU GPL 3.0                     *
*            see: https://www.gnu.org/licenses/gpl-3.0.en.html                  *
*                                                                               *
*            "Oh boy, here I go codin' again! I just love codin'"               *
*                                                                               *
\*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT stdout

unsigned char vsrom[256];

int main(int argc, char *argv[])
{
   FILE *ROM;
   int i;

#define SIZE 32

   unsigned char sn[33];

   // VSROM is 256 bytes, serial is 16 bytes, applenet is also another 16 bytes after

#define RESET (1)
#define VSYNC (2)
#define BIT2 (4)
#define BIT3 (8)
#define CSYNC (16)
#define VSIR (32)
#define HSYNC (64)
#define SERNO (128)

   puts(" =================================================================================");
   puts(" =                                                                               =");
   puts(" =                  Decode Apple Lisa VSROM Serial Number                        =");
   puts(" =       Copyright (C) 2021 by Ray Arachelian, All Rights Reserved               =");
   puts(" =                      https://lisaem.sunder.net                                =");
   puts(" =               Released under the terms of the GNU GPL 3.0                     =");
   puts(" =                                                                               =");
   puts(" =            see: https://www.gnu.org/licenses/gpl-3.0.en.html                  =");
   puts(" =      https://lisalist2.com/index.php/topic,193.msg1486.html#msg1486           =");
   puts(" =                                                                               =");
   puts(" =================================================================================\n");

   if (argc != 2)
   {
      fprintf(stderr, "Usage: decode-vsrom videostate.rom\n\n");
      exit(1);
   }

   ROM = fopen(argv[1], "rb");
   i = fread(vsrom, 256, 1, ROM);

   memset(sn, 0, 32);

   for (i = 0; i < 256; i++)
   {
      if (vsrom[i ^ 128] & SERNO)
         sn[i / 8] |= 1 << (7 - (i & 7));

      /*  printf("%3d:%02X RST:%d VSYNC:%d CSYNC:%d VSIR:%d HSYNC:%d SERNO:%d 2:%d 3:%d\n",i,vsrom[i],
           (vsrom[i] & RESET) ? 1:0,
           (vsrom[i] & VSYNC) ? 1:0,
           (vsrom[i] & CSYNC) ? 1:0,
           (vsrom[i] & VSIR ) ? 1:0,
           (vsrom[i] & HSYNC) ? 1:0,
           (vsrom[i] & SERNO) ? 1:0,
           (vsrom[i] & BIT2 ) ? 1:0,
           (vsrom[i] & BIT3 ) ? 1:0   ); */
   }

   fclose(ROM);

   // fix end bytes - they're 00 in the physical VSROM, but displayed as 0x0f0f in service mode

   // ff000000000000ff0000000000000000
   // ff028308104050ff0010163504700000
   //  0 1 2 3 4 5 6 7 8 9 a b c d e f
   sn[15] = 0xff;
   fprintf(OUT, "For use with LisaEm Preferences: ");
   for (i = 0; i < 17; i++)
   {
      if (i != 7)
         fprintf(OUT, "%02x", sn[i]);
   }
   fprintf(OUT, "\n");
   fprintf(OUT, "Magic Serial Number:             ff000000000000ff0000000000000000\n");
   fprintf(OUT, "My Lisa's Serial:                ff028308104050ff0010163504700000\n");
   fprintf(OUT, "                                  0 1 2 3 4 5 6 7 8 9 a b c d e f\n");

   sn[31] = 0xff;

   fprintf(OUT, "\n\nService Mode Display:\n\n");
   fprintf(OUT, "240 Serial #: ");
   for (i = 0; i < 32; i++)
   {
      if ((i & 15) < 8)
         fprintf(OUT, "%02X%02X ", (sn[i] & 0xf0) >> 4, sn[i] & 0x0f);
      if (i == 15)
         fprintf(OUT, "\n260 AppleNet: ");
   }
   fprintf(OUT, "\n\nSerial Number of this Lisa (BLU format)\n\n");

   sn[7] = 0xff;
   sn[16 + 7] = 0xff;
   fprintf(OUT, "    XXLLYYDD DUUUUXXX\n");
   fprintf(OUT, "    %02X%02X%02X%02X %02X%02X%02X%02X\n",
           sn[0], sn[1], sn[2], sn[3], sn[4], sn[5], sn[6], sn[7],
           sn[8], sn[9], sn[10], sn[11], sn[12], sn[13], sn[14], sn[15]);

   fprintf(OUT, "\n    PPPNNNNN XXXXXXXX\n");
   fprintf(OUT, "    %02X%02X%02X%02X %02X%02X%02X%02X\n\n",
           sn[16 + 1], sn[16 + 2], sn[16 + 3], sn[16 + 4],
           sn[16 + 5], sn[16 + 6], sn[16 + 7], sn[16 + 8]);

   fprintf(OUT, "X    - Unused/Unknown      LL    - Plant Location\n");
   fprintf(OUT, "YY   - Year manufactured   DDD   - Day of Year\n");
   fprintf(OUT, "UUUU - Unit Number\n");
   fprintf(OUT, "PPP  - AppleNet Prefix     NNNNN - AppleNet ID\n\n");
}
