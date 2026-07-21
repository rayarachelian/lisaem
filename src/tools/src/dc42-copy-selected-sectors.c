/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) MMXXI  Ray A. Arachelian                            *
*                            All Rights Reserved                                       *
*                                                                                      *
*             Copy a list of sectors from one image to another for repairs             *
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
  int ret, blocks, tags;
  DC42ImageType Fsrc, Fdest;

  puts("  ---------------------------------------------------------------------------");
  puts("    DC42 copy-selected-sectors                 http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) MMXXI, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");
  puts("     Copy a list of sectors from one disk image to another for repairs. ");

  if (argc < 4)
  {
    puts("");
    puts("  This program copies a list of sectors from a source disk image to a destination  ");
    puts("  so as to help with repairs.");
    puts("");
    puts("");
    puts("  Usage: dc42-copy-selected-sectors {source.dc42} {dest.dc42} {sec1} {sec2} .. {secN}");
    puts("  i.e.   dc42-copy-selected-sectors source.dc42 dest.dc42 0 0x10 11 12 15 22");
    puts("         will copy sectors 0, 16, 11, 12, 15, and 22 from source.dc42 to");
    puts("         dest.dc42, overwriting the original ones in dest.dc42.");
    exit(0);
  }

  uint8 *ltag;
  uint8 *ldata;

  ret = dc42_auto_open(&Fsrc, argv[1], "wb");
  if (ret)
  {
    fprintf(stderr, "Could not open source image  %s because %s, %d\n", argv[1], Fsrc.errormsg, ret);
    Fsrc.close_image(&Fsrc);
    exit(1);
  }

  ret = dc42_auto_open(&Fdest, argv[2], "wb");
  if (ret)
  {
    fprintf(stderr, "Could not open dest image %s because %s, %d\n", argv[2], Fdest.errormsg, ret);
    Fsrc.close_image(&Fsrc);
    Fdest.close_image(&Fdest);
    exit(1);
  }

  blocks = MIN(Fsrc.numblocks, Fdest.numblocks);
  tags = MIN(Fsrc.tagsize, Fdest.tagsize);

  for (i = 3; i < (uint32)argc; i++)
  {

    char *endptr;
    long sector = -1;
    int ret;

    sector = strtol(argv[i], &endptr, 0);

    if (endptr[0] != 0)
    {
      Fsrc.close_image(&Fsrc);
      Fdest.close_image(&Fdest);
      fprintf(stderr, "Could not parse parameter #%d: \"%s\"\n", i, argv[i]);
      exit(1);
    }

    ltag = Fsrc.read_sector_tags(&Fsrc, sector);
    ldata = Fsrc.read_sector_data(&Fsrc, sector);

    ret = Fdest.write_sector_tags(&Fdest, sector, ltag);
    if (ret)
    {
      fprintf(stderr, "Failed to write tag to sector #%d\n", sector);
      Fsrc.close_image(&Fsrc);
      Fdest.close_image(&Fdest);
      exit(10);
    }

    ret = Fdest.write_sector_data(&Fdest, sector, ldata);
    if (ret)
    {
      fprintf(stderr, "Failed to write data to sector #%d\n", sector);
      Fsrc.close_image(&Fsrc);
      Fdest.close_image(&Fdest);
      exit(10);
    }

    printf("Copied sector %d\n", sector);
  }

  Fsrc.close_image(&Fsrc);
  Fdest.close_image(&Fdest);

  return 0;
}
