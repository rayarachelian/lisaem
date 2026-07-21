/**************************************************************************************\
*                                                                                      *
*                    dc42-to-raw - Dumps sectors and tags to files                     *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>

int min(uint32 a, uint32 b) { return a < b ? a : b; }

int main(int argc, char *argv[])
{
   int i, j, err, stat;
   uint32 x;
   DC42ImageType in, out;
   char *a = NULL;
   char *Image = NULL;
   uint8 *data, *tags;

   char nameout[8192];  // filename buffers
   char nameout1[8192]; // filename buffers

   puts("  ---------------------------------------------------------------------------");
   puts("    DC42 Resize to 400k v0.01                http://lisaem.sunder.net");
   puts("  ---------------------------------------------------------------------------");
   puts("          Copyright (C) MMXX, Ray A. Arachelian, All Rights Reserved.");
   puts("              Released under the GNU Public License, Version 2.0");
   puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
   puts("  ---------------------------------------------------------------------------\n");

   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
      {
         printf("dc42-resize-to-400k\n\n"
                //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
                //  0         1         2         3         4         5         6         7         8
                "Usage: dc42-resize-to-400k Disk_Image.dc42\n\n"

                "This utility is used to extend short dc42 images such as BLU or NeoWidEx\n"
                "created by other toolchains to full size 400k 3.5\" images. \n");

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

   err = dc42_auto_open(&in, Image, "w");
   if (err)
   {
      fprintf(stderr, "could not open: '%s' because:%s", Image, in.errormsg);
      exit(1);
   }

   snprintf(nameout1, 8192, "%s", Image);
   fprintf(stderr, "1: nameout1: %s\n", nameout1);

   a = strstr(nameout1, ".image");
   if (a != NULL)
      *a = 0;
   a = strstr(nameout1, ".Image");
   if (a != NULL)
      *a = 0;
   a = strstr(nameout1, ".IMAGE");
   if (a != NULL)
      *a = 0;
   a = strstr(nameout1, ".dc42");
   if (a != NULL)
      *a = 0;
   a = strstr(nameout1, ".Dc42");
   if (a != NULL)
      *a = 0;
   a = strstr(nameout1, ".DC42");
   if (a != NULL)
      *a = 0;

   snprintf(nameout, 8192, "%s.out.dc42", nameout1);
   fprintf(stderr, "6: nameout: %s\n", nameout);
   fflush(stderr);

   dc42_create(nameout, "-not a Macintosh disk-", 800 * 512, 800 * 12);
   err = dc42_auto_open(&out, nameout, "w");
   if (err)
   {
      fprintf(stderr, "could not open created: '%s' because:%s", nameout, out.errormsg);
      exit(1);
   }

   for (int x = 0; x < min(in.numblocks, 800u); x++)
   {
      data = in.read_sector_data(&in, x);
      if (!data)
      {
         fprintf(stderr, "Could not read data for sector %d from image!\n", x);
         in.close_image(&in);
         exit(2);
      }
      out.write_sector_data(&out, x, data);

      if (in.tagsize)
      {
         tags = in.read_sector_tags(&in, x);
         if (!tags)
         {
            fprintf(stderr, "Could not read tags for sector %d from image!\n", (int)x);
            in.close_image(&in);
            exit(2);
         }
         out.write_sector_tags(&out, x, tags);
      }
   }

   in.close_image(&in);
   out.close_image(&out);
   return 0;
}
