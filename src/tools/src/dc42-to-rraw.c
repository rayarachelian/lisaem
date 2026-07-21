/**************************************************************************************\
*                                                                                      *
*                    dc42-to-raw - Dumps sectors and tags to files                     *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>

int interleave = 0;
int deinterleave = 0;

// ---------------------------------------------

long interleave5(long sector)
{
  static const int offset[] = {0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11, 16, 21, 26, 31, 20, 25, 30, 19, 24, 29, 18, 23, 28, 17, 22, 27};
  return offset[sector & 31] + sector - (sector & 31);
}

long deinterleave5(long sector)
{
  static const int offset[] = {0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 16, 29, 26, 23, 20, 17, 30, 27, 24, 21, 18, 31, 28, 25, 22, 19};
  return offset[sector & 31] + sector - (sector & 31);
}

void help(void)
{
  printf("dc42-to-raw\n\n"
         //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
         //  0         1         2         3         4         5         6         7         8
         "Usage: dc42-to-raw -i|-d|-s Disk_Image.dc42\n\n"
         " -i | --interleave   - turn on   interleave 5 translation\n"
         " -d | --deinterleave - turn on deinterleave 5 translation\n"
         " -s | --straight     - no translation (default)\n"
         " -h | --help         - this screen\n\n"

         "This utility is used to convert dc42 images to raw images\n"
         "does the opposite of raw-to-dc42.\n");
}

int main(int argc, char *argv[])
{
  int i, j, err, stat;
  unsigned int x;
  DC42ImageType F;
  char *Image = NULL;
  uint8 *data, *tags;
  int argn;

  int do_patch = 15;
  FILE *raw, *tag;
  char rawout[8192]; // filename buffers
  char rawtag[8192];

  puts("  ---------------------------------------------------------------------------");
  puts("    DC42 To Reverse Raw (tags+data) v0.02            http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) 2021, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");

  deinterleave = 0;
  interleave = 0; // defaults

  if (argc < 2)
  {
    help();
    exit(1);
  }

  for (argn = 1; argn < argc; argn++)
  {
    if (argv[argn][0] == '-')
    {
      if (argv[argn][1] == 'h')
      {
        help();
        exit(0);
      }
      else if (argv[argn][1] == 'i')
      {
        interleave = 1;
        deinterleave = 0;
      }
      else if (argv[argn][1] == 'd')
      {
        deinterleave = 1;
        interleave = 0;
      }
      else if (argv[argn][1] == 's')
      {
        deinterleave = 0;
        interleave = 0;
      }
      else if (argv[argn][1] == '-')
      {
        if (strncmp(argv[argn], "--help", 20) == 0)
        {
          help();
          exit(0);
        }
        else if (strncmp(argv[argn], "--interleave", 20) == 0)
        {
          interleave = 1;
          deinterleave = 0;
        }
        else if (strncmp(argv[argn], "--deinterleave", 20) == 0)
        {
          deinterleave = 1;
          interleave = 0;
        }
        else if (strncmp(argv[argn], "--straight", 20) == 0)
        {
          deinterleave = 0;
          interleave = 0;
        }
        else
        {
          help();
          fprintf(stderr, "\nUnknown option: %s\n\n", argv[argn]);
          exit(1);
        }
      }
    }
    else
      break;
  }
  Image = argv[argn];

  if (!Image)
  {
    fprintf(stderr, "I need a file name.  Run again with -h for help.\n");
    exit(5);
  }

  snprintf(rawout, 8192, "%s.raw", Image);

  raw = fopen(rawout, "wb");
  if (!raw)
  {
    fprintf(stderr, "Could not create %s\n", rawout);
    perror("");
    F.close_image(&F);
    exit(1);
  }

  err = dc42_auto_open(&F, Image, "w");
  if (err)
  {
    fprintf(stderr, "could not open: '%s' because:%s", Image, F.errormsg);
    exit(1);
  }

  for (x = 0; x < F.numblocks; x++)
  {
    long b5 = x;
    if (interleave)
      b5 = interleave5(x);
    if (deinterleave)
      b5 = deinterleave5(x);

    data = F.read_sector_data(&F, b5);
    if (F.tagsize)
    {
      tags = F.read_sector_tags(&F, b5);
      if (!tags)
      {
        fprintf(stderr, "Could not read tags for sector %d from image!\n", (int)x);
        F.close_image(&F);
        exit(2);
      }
      fwrite(tags, F.tagsize, 1, raw);
    }
    if (!data)
    {
      fprintf(stderr, "Could not read data for sector %d from image!\n", x);
      F.close_image(&F);
      exit(2);
    }
    fwrite(data, F.sectorsize, 1, raw);
  }

  F.close_image(&F);
  fclose(raw);
  return 0;
}
