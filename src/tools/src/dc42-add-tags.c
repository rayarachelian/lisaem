/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) MMXXI  Ray A. Arachelian                            *
*                            All Rights Reserved                                       *
*                                                                                      *
*                         Add tags to tagless images.                                  *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>

int main(int argc, char *argv[])
{
  uint32 i;
  int j;

  DC42ImageType Fsrc, Ftarget;

  puts("  ---------------------------------------------------------------------------");
  puts("    DC42 Add Tags                              http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) MMXXI, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");
  puts("                Add empty tags to a dc42 image without them.");

  if (argc != 3)
  {
    puts("");
    puts("  This program is used to add empty tags to a source image that does not have");
    puts("  them. Block zero file id tags are set to AAAA to allow booting a non-LOS ");
    puts("  operating system.\n");
    puts("  Warning:  The target image will be overwritten if it already exists!");
    puts("");
    puts("  Usage:  dc42-add-tags  {source image} {target image}");
    puts("  i.e.   ./dc42-copy-boot-loader macworksxl.dc42 macworkslx-with-tags.dc42");
    exit(0);
  }

  int ret = 0, ret1 = 0;
  uint8 mytags[22];

  memset(mytags, 0, 20);
  uint8 *tag = mytags;
  uint8 *data;

  ret = dc42_auto_open(&Fsrc, argv[1], "wb");
  if (ret)
  {
    fprintf(stderr, "Could not open source image %s because %s, returned:%d\n", argv[1], Fsrc.errormsg, ret);
    Fsrc.close_image(&Fsrc);
    exit(1);
  }

  j = dc42_create(argv[2], "-lisaem.sunder.net hd-", Fsrc.numblocks * 512, Fsrc.numblocks * (Fsrc.numblocks < 1600 ? 12 : 20));

  ret = dc42_auto_open(&Ftarget, argv[2], "wb");
  if (ret)
  {
    fprintf(stderr, "Could not open created target image %s because %s\n", argv[2], Ftarget.errormsg);
    Fsrc.close_image(&Fsrc);
    Ftarget.close_image(&Ftarget);
    exit(1);
  }

  tag[4] = 0xaa;
  tag[5] = 0xaa;

  fprintf(stderr, "copying blk: ");
  for (i = 0; i < Fsrc.numblocks; i++)
  {
    data = Fsrc.read_sector_data(&Fsrc, i);

    ret = Ftarget.write_sector_tags(&Ftarget, i, tag);
    ret1 = Ftarget.write_sector_data(&Ftarget, i, data);

    if (ret || ret1)
      fprintf(stderr, "Error writing to block %d of target\n", i);
    fprintf(stderr, "%4d\b\b\b\b", i);

    tag[4] = 0;
    tag[5] = 0;
  }

  fprintf(stderr, "\ndone.\n");

  Fsrc.close_image(&Fsrc);
  Ftarget.close_image(&Ftarget);

  return 0;
}
