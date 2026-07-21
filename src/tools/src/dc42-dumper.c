/**************************************************************************************\
*                                                                                      *
*                    Sector Dumper - Dumps each sector (and tags)                      *
*                  to separate binary files.  As requested by claunia                  *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>

int main(int argc, char *argv[])
{
  int i, j, err, stat;
  unsigned int x;
  DC42ImageType F;
  char *Image = NULL;
  uint8 *data, *tags;

  puts("  ---------------------------------------------------------------------------");
  puts("    DC42 Sectors to files Dumper v0.01               http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) MMXX, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");

  int do_patch = 15;
  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
    {
      printf("Sector Dumper.\n\n"
             //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
             //  0         1         2         3         4         5         6         7         8
             "Usage: dc42-dumper Disk_Image.dc42\n\n"

             "This utility is used to dump each sector for a DC42 image out to a separate\n"
             "file, along with the tags. File names are sector-0001.bin sector-0001-tag.bin\n"
             "and so on.\n");

      exit(0);
    }
    else
    {
      if (!Image)
        Image = argv[i];
      else
      {
        fprintf(stderr, "Can only specify one filename.  Surround it by quotes if it contains spaces.\n");
        exit(2);
      }
    }
  }

  if (!Image)
  {
    fprintf(stderr, "I need a file name.  Run again with -h for help.\n");
    exit(5);
  }

  err = dc42_auto_open(&F, Image, "w");
  if (err)
  {
    fprintf(stderr, "could not open: '%s' because:%s", Image, F.errormsg);
    exit(1);
  }

  puts("Writing data to sector-xxxx.bin");
  for (x = 0; x < F.numblocks; x++)
  {
    FILE *f;
    char filename[24];
    data = F.read_sector_data(&F, x);
    if (!data)
    {
      fprintf(stderr, "Could not read data for sector %d from image!\n", x);
      F.close_image(&F);
      exit(2);
    }
    snprintf(filename, 24, "sector-%04d.bin", x);
    f = fopen(filename, "wb");
    if (!f)
    {
      fprintf(stderr, "Could not create %s\n", filename);
      perror("");
      F.close_image(&F);
      exit(1);
    }
    fwrite(data, F.sectorsize, 1, f);
    fclose(f);
  }

  if (F.tagsize)
  {
    puts("Writing tags to sector-xxxx-tag.bin");
    for (x = 0; x < F.numblocks; x++)
    {
      FILE *f;
      char filename[24];
      tags = F.read_sector_tags(&F, x);
      if (!tags)
      {
        fprintf(stderr, "Could not read tags for sector %d from image!\n", (int)x);
        F.close_image(&F);
        exit(2);
      }
      snprintf(filename, 24, "sector-%04d-tag.bin", x);
      f = fopen(filename, "wb");
      if (!f)
      {
        fprintf(stderr, "Could not create %s\n", filename);
        perror("");
        F.close_image(&F);
        exit(1);
      }
      fwrite(tags, F.tagsize, 1, f);
      fclose(f);
    }
  }

  F.close_image(&F);
  return 0;
}
