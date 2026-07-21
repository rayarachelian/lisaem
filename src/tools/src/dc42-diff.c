/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) MMXXI  Ray A. Arachelian                            *
*                            All Rights Reserved                                       *
*                                                                                      *
*                            diff two disk images.                                     *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

char niceascii(char c)
{
  c &= 127;
  if (c < 31)
    c |= 32;
  if (c == 127)
    c = '.';
  return c;
}

char *niceprint(uint8 *offset)
{
  static char block[18];

  for (int i = 0; i < 16; i++)
    block[i] = niceascii((char)offset[i]);

  block[16] = 0;
  block[17] = 0;

  return block;
}

int main(int argc, char *argv[])
{
  uint32 i, j, k;

  DC42ImageType Fleft, Fright;

  puts("  ---------------------------------------------------------------------------");
  puts("    DC42 diff                                  http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) MMXXI, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");
  puts("                Compare two images and report differences.  ");

  if (argc < 3)
  {
    puts("");
    puts("  This program is used to compare two DC42 disk images and output the");
    puts("  differences between them.");
    puts("");
    puts("");
    puts("  Usage:  dc42-diff {image1.dc42} {image2.dc42}");
    puts("  i.e.   ./dc42-diff macworksxl.dc42 macworkslx3.0.dc42");
    exit(0);
  }

  int skiptags = 0, ret;
  uint32 blocks, tags;

  uint8 *ltag, *rtag;
  uint8 *ldata, *rdata;

  ret = dc42_auto_open(&Fleft, argv[1], "wb");
  if (ret)
  {
    fprintf(stderr, "Could not open left image  %s because %s, %d\n", argv[1], Fleft.errormsg, ret);
    Fleft.close_image(&Fleft);
    exit(1);
  }

  ret = dc42_auto_open(&Fright, argv[2], "wb");
  if (ret)
  {
    fprintf(stderr, "Could not open right image %s because %s, %d\n", argv[2], Fright.errormsg, ret);
    Fleft.close_image(&Fleft);
    Fright.close_image(&Fright);
    exit(1);
  }

  if ((Fleft.tagsizetotal & Fright.tagsizetotal) == 0)
  {
    skiptags = 1;
    if (!Fleft.tagsizetotal && Fright.tagsizetotal)
      printf("Left image does not have tags, but right does, will not compare tags.");
    if (Fleft.tagsizetotal && !Fright.tagsizetotal)
      printf("Right image does not have tags, but left does, will not compare tags.");
  }

  blocks = MIN(Fleft.numblocks, Fright.numblocks);
  tags = MIN(Fleft.tagsize, Fright.tagsize);

  if (Fleft.numblocks != Fright.numblocks)
    printf("Images have different number of blocks, will only compare the first %d blocks\n", blocks);
  if (Fleft.tagsize != Fright.tagsize)
    printf("Images have different number of tags, will only compare the first %d tags/sec\n", tags);

  for (i = 0; i < blocks; i++)
  {
    int delta = 0;
    ltag = Fleft.read_sector_tags(&Fleft, i);
    rtag = Fright.read_sector_tags(&Fright, i);
    ldata = Fleft.read_sector_data(&Fleft, i);
    rdata = Fright.read_sector_data(&Fright, i);

    if (!skiptags)
    {
      int delta = 0;
      for (j = 0; j < tags; j++)
        if (ltag[j] != rtag[j])
        {
          if (!delta)
          {
            delta = 1;
            printf("\n\nSector:0x%04x (%6d): tag @0x%02x (%d) <:%02x >:%02x", i, i, j, j, ltag[j], rtag[j]);
          }
          else
          {
            printf(" tag @0x%02x (%d) <:%02x >:%02x", i, i, j, j, ltag[j], rtag[j]);
          }
        }
      puts("");
    }

    int lempty = 1;
    for (j = 0; j < Fleft.datasize; j++)
    {
      if (ldata[j])
      {
        lempty = 0;
        break;
      };
    }
    int rempty = 1;
    for (j = 0; j < Fright.datasize; j++)
    {
      if (rdata[j])
      {
        rempty = 0;
        break;
      };
    }

    if (lempty ^ rempty)
    {
      if (lempty)
      {
        printf("Sector:0x%04x (%6d) < left image is empty, but right is not. Bad block?\n", i, i);
        continue;
      }
      if (rempty)
      {
        printf("Sector:0x%04x (%6d) > right image is empty, but left is not. Bad block?\n", i, i);
        continue;
      }
    }

    for (j = 0; j < Fleft.datasize; j += 16)
    {

      int diff = 0;
      for (k = 0; k < 16; k++)
        if (ldata[j + k] != rdata[j + k])
        {
          diff = 1;
          delta = 1;
        }

      if (diff)
      {
        printf("Sector:0x%04x (%6d) @+0x%03x (%3d) < %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c . %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c | %s\n",
               i, i, j, j,

               ((ldata[j + 0] == rdata[j + 0]) ? ' ' : ':'), ldata[j + 0], ((ldata[j + 0] == rdata[j + 0]) ? ' ' : ':'),
               ((ldata[j + 1] == rdata[j + 1]) ? ' ' : ':'), ldata[j + 1], ((ldata[j + 1] == rdata[j + 1]) ? ' ' : ':'),
               ((ldata[j + 2] == rdata[j + 2]) ? ' ' : ':'), ldata[j + 2], ((ldata[j + 2] == rdata[j + 2]) ? ' ' : ':'),
               ((ldata[j + 3] == rdata[j + 3]) ? ' ' : ':'), ldata[j + 3], ((ldata[j + 3] == rdata[j + 3]) ? ' ' : ':'),
               ((ldata[j + 4] == rdata[j + 4]) ? ' ' : ':'), ldata[j + 4], ((ldata[j + 4] == rdata[j + 4]) ? ' ' : ':'),
               ((ldata[j + 5] == rdata[j + 5]) ? ' ' : ':'), ldata[j + 5], ((ldata[j + 5] == rdata[j + 5]) ? ' ' : ':'),
               ((ldata[j + 6] == rdata[j + 6]) ? ' ' : ':'), ldata[j + 6], ((ldata[j + 6] == rdata[j + 6]) ? ' ' : ':'),
               ((ldata[j + 7] == rdata[j + 7]) ? ' ' : ':'), ldata[j + 7], ((ldata[j + 7] == rdata[j + 7]) ? ' ' : ':'),
               ((ldata[j + 8] == rdata[j + 8]) ? ' ' : ':'), ldata[j + 8], ((ldata[j + 8] == rdata[j + 8]) ? ' ' : ':'),
               ((ldata[j + 9] == rdata[j + 9]) ? ' ' : ':'), ldata[j + 9], ((ldata[j + 9] == rdata[j + 9]) ? ' ' : ':'),
               ((ldata[j + 10] == rdata[j + 10]) ? ' ' : ':'), ldata[j + 10], ((ldata[j + 10] == rdata[j + 10]) ? ' ' : ':'),
               ((ldata[j + 11] == rdata[j + 11]) ? ' ' : ':'), ldata[j + 11], ((ldata[j + 11] == rdata[j + 11]) ? ' ' : ':'),
               ((ldata[j + 12] == rdata[j + 12]) ? ' ' : ':'), ldata[j + 12], ((ldata[j + 12] == rdata[j + 12]) ? ' ' : ':'),
               ((ldata[j + 13] == rdata[j + 13]) ? ' ' : ':'), ldata[j + 13], ((ldata[j + 13] == rdata[j + 13]) ? ' ' : ':'),
               ((ldata[j + 14] == rdata[j + 14]) ? ' ' : ':'), ldata[j + 14], ((ldata[j + 14] == rdata[j + 14]) ? ' ' : ':'),
               ((ldata[j + 15] == rdata[j + 15]) ? ' ' : ':'), ldata[j + 15], ((ldata[j + 15] == rdata[j + 15]) ? ' ' : ':'),
               niceprint(&ldata[j]));

        printf("                                     > %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c . %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c %c%02x%c | %s\n",
               ((ldata[j + 0] == rdata[j + 0]) ? ' ' : ':'), rdata[j + 0], ((ldata[j + 0] == rdata[j + 0]) ? ' ' : ':'),
               ((ldata[j + 1] == rdata[j + 1]) ? ' ' : ':'), rdata[j + 1], ((ldata[j + 1] == rdata[j + 1]) ? ' ' : ':'),
               ((ldata[j + 2] == rdata[j + 2]) ? ' ' : ':'), rdata[j + 2], ((ldata[j + 2] == rdata[j + 2]) ? ' ' : ':'),
               ((ldata[j + 3] == rdata[j + 3]) ? ' ' : ':'), rdata[j + 3], ((ldata[j + 3] == rdata[j + 3]) ? ' ' : ':'),
               ((ldata[j + 4] == rdata[j + 4]) ? ' ' : ':'), rdata[j + 4], ((ldata[j + 4] == rdata[j + 4]) ? ' ' : ':'),
               ((ldata[j + 5] == rdata[j + 5]) ? ' ' : ':'), rdata[j + 5], ((ldata[j + 5] == rdata[j + 5]) ? ' ' : ':'),
               ((ldata[j + 6] == rdata[j + 6]) ? ' ' : ':'), rdata[j + 6], ((ldata[j + 6] == rdata[j + 6]) ? ' ' : ':'),
               ((ldata[j + 7] == rdata[j + 7]) ? ' ' : ':'), rdata[j + 7], ((ldata[j + 7] == rdata[j + 7]) ? ' ' : ':'),
               ((ldata[j + 8] == rdata[j + 8]) ? ' ' : ':'), rdata[j + 8], ((ldata[j + 8] == rdata[j + 8]) ? ' ' : ':'),
               ((ldata[j + 9] == rdata[j + 9]) ? ' ' : ':'), rdata[j + 9], ((ldata[j + 9] == rdata[j + 9]) ? ' ' : ':'),
               ((ldata[j + 10] == rdata[j + 10]) ? ' ' : ':'), rdata[j + 10], ((ldata[j + 10] == rdata[j + 10]) ? ' ' : ':'),
               ((ldata[j + 11] == rdata[j + 11]) ? ' ' : ':'), rdata[j + 11], ((ldata[j + 11] == rdata[j + 11]) ? ' ' : ':'),
               ((ldata[j + 12] == rdata[j + 12]) ? ' ' : ':'), rdata[j + 12], ((ldata[j + 12] == rdata[j + 12]) ? ' ' : ':'),
               ((ldata[j + 13] == rdata[j + 13]) ? ' ' : ':'), rdata[j + 13], ((ldata[j + 13] == rdata[j + 13]) ? ' ' : ':'),
               ((ldata[j + 14] == rdata[j + 14]) ? ' ' : ':'), rdata[j + 14], ((ldata[j + 14] == rdata[j + 14]) ? ' ' : ':'),
               ((ldata[j + 15] == rdata[j + 15]) ? ' ' : ':'), rdata[j + 15], ((ldata[j + 15] == rdata[j + 15]) ? ' ' : ':'),
               niceprint(&rdata[j]));
      }
      if (delta)
        puts("");
    }
  }

  fprintf(stderr, "\ndone.\n");

  Fleft.close_image(&Fleft);
  Fright.close_image(&Fright);

  return 0;
}
