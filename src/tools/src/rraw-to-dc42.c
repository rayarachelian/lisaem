/**************************************************************************************\
*                                                                                      *
*                        raw2dc42 profile hd image converter                           *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                                                                                      *
*                        Copyright (C) 2018 Ray A. Arachelian                          *
*                                All Rights Reserved                                   *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <libdc42.h>

int interleave = 0;
int deinterleave = 0;

// ---------------------------------------------

/**
For sector number  0: interleave(0)=0 
For sector number  1: interleave(1)=5
For sector number  2: interleave(2)=10 
For sector number  3: interleave(3)=15
For sector number  4: interleave(4)=4
For sector number  5: interleave(5)=9 
For sector number  6: interleave(6)=14
For sector number  7: interleave(7)=3 
For sector number  8: interleave(8)=8 
For sector number  9: interleave(9)=13 
For sector number 10: interleave(10)=2 
For sector number 11: interleave(11)=7 
For sector number 12: interleave(12)=12 
For sector number 13: interleave(13)=1 
For sector number 14: interleave(14)=6 
For sector number 15: interleave(15)=11 
For sector number 16: interleave(16)=16 
For sector number 17: interleave(17)=21 
For sector number 18: interleave(18)=26 
For sector number 19: interleave(19)=31 
For sector number 20: interleave(20)=20
... etc ...
 */
long interleave5(long sector)
{
  static const int offset_delta[] = {0, 4, 8, 12, 0, 4, 8, -4, 0, 4, -8, -4, 0, -12, -8, -4};
  return sector + offset_delta[(sector & 15)]; // "sector & 15" is an optimized version of "sector % 16"
}

long interleave5_suboptimal(long sector)
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
  puts("\n"
       // 345678901234567890123456789012345678901234567890123456789012345678901234567890
       //        1         2         3         4         5         6         7
       "\n"
       "  Usage: rraw2dc42 {-d|-i|-s|--interleave|--deinterleave|--straight} {profile.raw}\n"
       "  will create {profile.raw}.dc42 as output\n"
       "\n"
       " -d|--deinterleave - turn on deinterleave5 block mapping (default)\n"
       " -i|--interleave   - turn on interleave5 block mapping\n"
       " -s|--straight     - no block number translation at all\n"
       " -h|--help         - this help screen\n"
       "  Note: you cannot use both -i|-d and --interleave|--deinterleave at the\n"
       "        same time, last option will take precedence if both are given.\n\n"
       "  This program takes a reverse raw drive image saved as 512 bytes of sector\n"
       "  data followed by 20 bytes of tag data and converts it to dc42 format\n"
       "  usable by LisaEm\n"
       "\n"
       "\n");
}

int main(int argc, char *argv[])
{
  int argn;
  long numblocks, i, b;
  FILE *raw;
  DC42ImageType profile;

  char dc42filename[8192];
  uint8 block[533];

  puts("  ---------------------------------------------------------------------------");
  puts("  rraw2dc42 (reverse data+tag raw) V0.0.2 Converter  http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) 2021, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");

  if (argc < 2)
  {
    puts("\n"
         // 345678901234567890123456789012345678901234567890123456789012345678901234567890
         //        1         2         3         4         5         6         7
         "\n"
         "  Usage: rraw2dc42 profile.raw\n"
         "\n"
         "  This program takes a reverse raw drive image saved as 512 bytes of sector\n"
         "  data followed by 20 bytes of tag data and converts it to dc42 format\n"
         "  usable by LisaEm\n"
         "\n"
         "\n");
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

  raw = fopen(argv[argn], "rb");
  if (!raw)
  {
    perror("Could not open the reverse raw image.");
    exit(2);
  }

  snprintf(dc42filename, 8192, "%s.dc42", argv[argn]);
  fseek(raw, 0L, SEEK_END);
  numblocks = ftell(raw) / (512 + 20);
  fseek(raw, 0L, SEEK_SET);

  i = dc42_create(dc42filename, "-lisaem.sunder.net hd-", numblocks * 512, numblocks * 20);
  if (i)
  {
    fprintf(stderr, "Could not create DC42 ProFile:%s\n", dc42filename);
    perror(" ");
    exit(1);
  }

  i = dc42_open(&profile, dc42filename, "wm");
  if (i)
  {
    fprintf(stderr, "Could not open created DC42 ProFile:%s because %s\n", dc42filename, profile.errormsg);
    exit(1);
  }

  fprintf(stderr, "Created %s\n", dc42filename);

  for (b = 0; b < numblocks; b++)
  {
    long b5 = b;
    if (interleave)
      b5 = interleave5(b);
    if (deinterleave)
      b5 = deinterleave5(b);

    printf("reading reverse raw block %04d, writing to profile block %04d \r", b, b5);
    i = fread(block, 512 + 20, 1, raw);
    if (i != 1)
    {
      fprintf(stderr, "\n\nWARNING: Error reading block # %d, fread size did not return 1 block, got %d blocks!\n", b, i);
    }

    i = profile.write_sector_data(&profile, b5, &block[0]);
    if (i)
    {
      fprintf(stderr, "\n\nError writing block data %d to dc42 ProFile:%s because %s\n", b, dc42filename, profile.errormsg);
      profile.close_image(&profile);
      exit(1);
    }

    i = profile.write_sector_tags(&profile, b5, &block[512]);
    if (i)
    {
      fprintf(stderr, "\n\nError writing block tags %d to dc42 ProFile:%s because %s\n", b, dc42filename, profile.errormsg);
      profile.close_image(&profile);
      exit(1);
    }
  }
  profile.close_image(&profile);
  puts("                                                               \rDone.");
  fclose(raw);
  return 0;
}
