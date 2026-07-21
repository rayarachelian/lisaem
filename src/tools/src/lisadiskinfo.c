/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                  Copyright (C) 1998, 2010 Ray A. Arachelian                          *
*                            All Rights Reserved                                       *
*                                                                                      *
*                            Lisa Disk Info tool                                       *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>

int main(int argc, char *argv[])
{
  int i, j;

  DC42ImageType F;
  char creatortype[10];

  puts("  ---------------------------------------------------------------------------");
  puts("    Lisa Disk Info Tool V0.01                       http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) 2007, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");

  if (argc <= 1)
  {
    puts("");
    puts("  This program is used to provide information about disk images that might be");
    puts("  useful on the Lisa Emulator, or with a real Lisa.  It will tell you whether");
    puts("  a file is encapsulated with MacBinII, and if it is a recognizeable DART or");
    puts("  Disk Copy 4.2 Disk image.");
    puts("");
    puts("  Usage: lisadiskinfo {filename1} {filename2} ... {filename N}");
    puts("  i.e.   ./lisadiskinfo SomeImage.dc42");
    exit(0);
  }

  for (i = 1; i < argc; i++)
  {
    int valid = 0;
    int ret = 0;
    printf("%-50s ", argv[i]);

    if (dc42_is_valid_macbinii(argv[i], creatortype))
    {
      for (j = 0; j < 8; j++)
      {
        if (creatortype[j] < 31)
          creatortype[j] = '?';
      }

      printf("MacBinII [%s] ", creatortype);
    }

    if (dart_is_valid_image(argv[i]))
    {
      printf("DART\n");
      valid = 1;
    }

    ret = dc42_is_valid_image(argv[i]);
    if (ret)
    {
      valid = 1;
      if (ret == -60)
        printf("Disk Copy 6.x uncompressed - will not work with LisaEm\n");
      else if (ret == -61)
        printf("Disk Copy 6.x compressed - will not work with LisaEm\n");
      else if (ret == 1 || ret == 2)
        printf("Disk Copy 4.2 image.\n");
      else
        valid = 0;
    }

    if (!valid)
    {
      FILE *file;
      int i = 0;
      unsigned char buffer[256];
      file = fopen(argv[i], "rb");
      i = fread(buffer, 256, 1, file);
      fclose(file);

      if (buffer[0] == 'S' && buffer[1] == 'I' && buffer[2] == 'T' && buffer[3] == '!')
      {
        valid = 1;
        printf("StuffIt! Archive\n");
      }
      else
      {

        if (dc42_is_valid_macbinii(argv[i], NULL))
        {
          if (buffer[128] == 'S' && buffer[128 + 1] == 'I' && buffer[128 + 2] == 'T' && buffer[128 + 3] == '!')
            printf("StuffIt! Archive\n");
          valid = 1;
        }
      }

      if (!valid)
        printf("Unrecognized\n");
    }
  }

  return 0;
}
