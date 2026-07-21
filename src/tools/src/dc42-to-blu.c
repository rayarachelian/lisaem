/**************************************************************************************\
*                                                                                      *
*                    dc42-to-blu - Dumps sectors and tags to blu                       *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>

int interleave = 0;
int deinterleave = 0;
int profile_ffffff = 0;
int widget = 0;

char ProFile_Spare_table[536] =
    {
        /*
         * The first 13 bytes show the device name, e.g. "PROFILE     ". Other drives are called "PROFILE 10M  " or "Widget-10   ".
         * The next three bytes hold the device number, which is $000000 for a 5MB ProFile, $000010 for a 10MB ProFile, and $001000 for a Widget 10 drive.
         * The next two bytes indicate  the firmware revision, e.g. $0398 for  3.98.
         * The next three bytes hold the total number of blocks available on the device. This is $002600 for 5MB and $004C00 for 10MB. You see, up to 8GB are possible. IDE reached its first limit at 520MB!
         * The next two bytes indicate the number of bytes per block: $0214 means 532. 532 means 512 byted user data and 20 bytes tag. This is the same format Macintosh MFS volumes and 400k/800k disks use. Send a MFS HD20SC with ST225N mechanism a read capacity command - the response will be 532 bytes per sector!
         * The next byte contains the total number of spare blocks available on the device, which is $20.
         * This is followed by the number of spare blocks currently allocated. A good drive uses less than three spares. When all these 32 blocks are allocated, the host will ask the user to call a qualified Apple Service technician for reformatting.
         * The next byte contains the number of bad blocks currently allocated. A bad block turns into a spared block during the next power-on self test.
         */

        //  0   1   2   3   4   5   6   7   8   9   a   b  c <- 13 bytes
        'P', 'R', 'O', 'F', 'I', 'L', 'E', ' ', ' ', ' ', ' ', ' ', 0, // device name
                                                                       //  d  e  f
        0, 0, 0,                                                       // 0,0,0x10-10MB ProFile                            // device #
                                                                       //  0x10,0x11
        0x03, 0x98,                                                    // ProFile firmware revsion
                                                                       //  0x12, 0x13, 0x14
        0x00, 0x26, 0x00,                                              // # of available blocks to user *play with this*
        /*20, 21 */
        0x02, 0x14,       // bytes/sector  =532
        32,               // number of spares (blocks)  byte 22
        0,                // byte 23
        0,                // byte 24
        0xFF, 0xFF, 0xFF, // byte 25,26,27
        0, 0, 0, 0,       // byte 28,29,30,31  (now we need 500 more bytes full of zeros)

        // 50 0 bytes * 10 lines = 500 bytes... for both spare and error tables. (virtual profiles are perfect!)
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 7
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // 10
};

/*

00000000  57 69 64 67 65 74 2d 31  30 20 20 20 20 00 01 00  |Widget-10    ...|
00000010  1a 45 00 4c 00 02 14 02  02 02 13 00 00 4c 00 00  |.E.L.........L..|
00000020  02 00 00 00 00 01 80 80  00 00 00 00 00 00 00 00  |................|
00000030  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000200  4c 69 73 61 20 48 44 20  49 6d 67 20 42 4c 55 56  |Lisa HD Img BLUV|
00000210  30 2e 39 30 4e fa 00 16  aa aa 08 50 22 4a 2d 0c  |0.90N......P"J-.|
*/

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
         " -f | --ffffff       - add ProFile -1 block header\n",
         " -b | --blu          - similar to -f, but also adds BLU signature\n"
         " -w | --widget       - for -f|-b change drive type to Widget instead of ProFile\n"
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
  puts("    DC42 To Raw Image (data+tags) v0.02              http://lisaem.sunder.net");
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
      else if (argv[argn][1] == 'f')
      {
        profile_ffffff = 1;
      }
      else if (argv[argn][1] == 'b')
      {
        profile_ffffff = 2;
      }
      else if (argv[argn][1] == 'w')
      {
        widget = 2;
        if (!profile_ffffff)
          profile_ffffff = 1;
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
        else if (strncmp(argv[argn], "--ffffff", 20) == 0)
        {
          profile_ffffff = 1;
        }
        else if (strncmp(argv[argn], "--blu", 20) == 0)
        {
          profile_ffffff = 2;
        }
        else if (strncmp(argv[argn], "--widget", 20) == 0)
        {
          widget = 2;
          if (!profile_ffffff)
            profile_ffffff = 1;
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

  if (profile_ffffff)
  {
    //                                                                 0123456789012345678901
    if (profile_ffffff == 2)
      memcpy((void *)(&ProFile_Spare_table[0]), "Widget-10    ", 13);
    if (widget)
      memcpy((void *)(&ProFile_Spare_table[0]), "Lisa HD Img BLUV0.90N", 21);
     fwrite(data,(F.tagsize ? (F.sectorsize+F.tagsize) : F.sectorsize,1,raw);
  }

  for (x = 0; x < F.numblocks; x++)
  {
    long b5 = x;
    if (interleave)
      b5 = interleave5(x);
    if (deinterleave)
      b5 = deinterleave5(x);

    data = F.read_sector_data(&F, b5);
    fwrite(data, F.sectorsize, 1, raw);
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
  }

  F.close_image(&F);
  fclose(raw);
  return 0;
}
