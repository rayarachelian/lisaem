/**************************************************************************************\
*                                                                                      *
*                        blu2dc42 profile hd image converter                           *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                                                                                      *
*                        Copyright (C) 2020 Ray A. Arachelian                          *
*                                All Rights Reserved                                   *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <libdc42.h>

/*

Sorry no, BLU does know (or thinks it knows) where the tags are.

If you directly access the parallel port to read data from a ProFile, for each block you'll find the first 20 bytes are tags followed by 512 bytes data, with the blocks interleaved.

If you do the same with a Widget using the ProFile compatible commands (dunno about the Widget specific commands), for each block you'll find the first 512 bytes are data followed by 20 bytes of tags, with the blocks not interleaved (ie. interleaving is typically done transparently at the drive level).

However (IIRC), when BLU reads/writes hard disk images, it:

    deinterleaves/reinterleaves blocks when the device is a ProFile
    puts/expects tags at the end in each 532 bytes of the image (ie. re-orders tags and data when a ProFile is involved)

The consequence of this is (supposed to be) that the image file always has the blocks in the OS level order, and the tags are always in the same place.
*/

int widget = 0;

int main(int argc, char *argv[])
{
  long numblocks, i, b;
  int blu = -1;
  DC42ImageType profile;

  char dc42filename[8192];
  uint8 block[537]; // 512 bytes data block 20 bytes tags, +2 tag if PRIAM +1 for padding

  puts("\n"
       // 345678901234567890123456789012345678901234567890123456789012345678901234567890
       //        1         2         3         4         5         6         7
       "                    blu2dc42 V0.0.3 DC42 to Profile Drive Converter\n"
       "                              http://lisaem.sunder.net\n"
       "\n"
       "                           Copyright (C) MMXX Ray A. Arachelian  \n"
       "                                    All Rights Reserved.          \n"
       "  This program takes a BLU ProFile/Widget disk image and\n"
       "  converts it to dc42 format usable by LisaEm\n"
       "\n"
       "\n");

  if (argc < 2)
  {

    puts("  Usage: blu2dc42 profile.blu\n"
         "\n"
         "\n");
    exit(1);
  }

  blu = open(argv[1], O_RDONLY);
  if (!blu)
  {
    perror("Could not open the BLU image.");
    exit(2);
  }
  lseek(blu, 128L, SEEK_END);
  i = read(blu, block, 32); // get around UTF issue?
  snprintf(dc42filename, 8192, "%s.dc42", argv[1]);
  numblocks = lseek(blu, 0L, SEEK_END);
  lseek(blu, 0L, SEEK_SET);

  /* Read BLU header block - assume widget/profile and not priam */
  i = read(blu, block, 512 + 20);
  if (i != 532)
  {
    fprintf(stderr, "\n\nWARNING: Error reading BLU header block, fread size did not return 532 bytes block, got %d blocks!\n", i);
    close(blu);
    exit(2);
  }

  char *blusign = (char *)&block[512]; // BLU signature is in the tags of the spare table
  uint8 x = blusign[15];
  blusign[15] = 0; // force terminate string so we don't overrun
  int blublocks = ((block[0x12] << 16) | (block[0x13] << 8) | (block[0x14]));
  int blocksize = ((block[0x15] << 8) | (block[0x16]));
  numblocks = numblocks / blocksize; // correct to blocks

  if (strncmp(blusign, "Lisa HD Img BLU", 15) != 0)
  {
    fprintf(stderr, "This doesn't appear to be a BLU image\n");
    close(blu);
    exit(2);
  }

  blusign[15] = x;
  if (strlen((char *)block) < 16)
  {
    blusign[19] = 0; // terminate version
    fprintf(stderr, "Created by %s\n", blusign);
    fprintf(stderr, "Drive image is from: %s of type %02x%02x%02x\n", block, block[0x0D], block[0x0E], block[0x0F]);
    fprintf(stderr, "Number of Blocks:%d size of each block:%d\n\n", blublocks, blocksize);

    //                         0123456
    if (strncmp((char *)block, "Widget-", 6) == 0)
    {
      widget = 1;
      fprintf(stderr, "\n widget detected \n");
    }
    //                         01234567801234
  }

  i = lseek(blu, blocksize, SEEK_SET); // 0x214 = 532  // 20200309 -2 to fix file id tag for bootblock, not sure why it's needed yet.
  if (i != blocksize)                  // except that now the data is off by two breaking it. I think we're missing two tag bytes at the front
  {
    fprintf(stderr, "lseek failed to position to %d instead got %d\n", blocksize, i);
    numblocks = blublocks;
    close(blu);
    exit(33);
  }

  numblocks--; // skip spare table/ BLU header block

  if ((numblocks - blublocks) > 2) // might have one extra block at the end due to xmodem buffering, but more than that is an error
  {
    fprintf(stderr, "Warning: number of blocks in BLU header (%d) doesn't match size of file (%d)\nUsing blublocks value, this is possibly just an xmodem artifact.\n", blublocks, numblocks);
    numblocks = blublocks;
  }

  i = dc42_create(dc42filename, "-lisaem.sunder.net hd-", numblocks * 512, numblocks * (blocksize - 512));
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

  for (b = 0; b < numblocks; b++)
  {
    fprintf(stderr, "reading BLU block %04d, writing to profile block %04d \r", b, b);
    //      lseek(blu, (b+1)*(blocksize), SEEK_SET);

    i = read(blu, block, blocksize);
    if (i != blocksize)
    {
      profile.close_image(&profile);
      fprintf(stderr, "                                                               \rError.\n");
      close(blu);
      fprintf(stderr, "\nError reading block # %d, fread size did not return a %d bytes block, got %d blocks!\n", b, blocksize, i);
      exit(1);
    }

    i = profile.write_sector_data(&profile, b, (uint8 *)(&block[0]));
    if (i)
    {
      fprintf(stderr, "\n\nError writing block data %d to dc42 ProFile:%s because %s\n", b, dc42filename, profile.errormsg);
      profile.close_image(&profile);
      exit(1);
    }

    // hacks to fix widget images
    int o = 512;

    // check AA,AA signature
    if (b == 0)
    {
      if (widget)
        o = 0;

      if (block[o + 4] != 0xaa || block[o + 5] != 0xaa)
      {
        fprintf(stderr, "\nWarning:Block 0 does not have AAAA file id! Got %02x%02x - this image will not boot.\n\n", block[512 + 4], block[512 + 5]); // this should be a warning.
      }
    }
    else
    {
      if (widget)
      {
        o -= 2;
        block[510] = block[511] = 0;
      }
    }

    i = profile.write_sector_tags(&profile, b, &block[o]);
    if (i)
    {
      fprintf(stderr, "\n\nError writing block tags %d to dc42 ProFile:%s because %s\n", b, dc42filename, profile.errormsg);
      profile.close_image(&profile);
      exit(1);
    }
  }

  profile.close_image(&profile);
  puts("                                                               \rDone.");
  close(blu);
  return 0;
}
