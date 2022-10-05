/**************************************************************************************\
*                                                                                      *
*   DC42 Boot Block Checksum - return a checksum of the boot block for use in LisaEm   *
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                     Copyright (C) 2022 Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
\**************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>

void process(char *Image) {
  DC42ImageType  F;

  if (!Image)  {fprintf(stderr,"No image name passed\n");                     return;}

  if (dc42_auto_open(&F,Image,"wb"))
     {
       fprintf(stderr,"xxxxxxxx could not open: '%s' because:%s\n",Image,F.errormsg); 
		 perror("\n"); return;
     }

//  if  (!dc42_has_tags(&F)) {fprintf(stderr,"XXXXXXXX Non-bootable: NO TAGS %s\n",Image); return;}

  uint8 *tag=dc42_read_sector_tags(&F,0);
  if  (tag[4]==0xaa && tag[5]==0xaa)
      {
          uint8 *sec=dc42_read_sector_data(&F,0);
          uint32 bootblockchecksum=0;
          for (uint32 i=0; i<512; i++) bootblockchecksum=( (bootblockchecksum<<1) | ((bootblockchecksum & 0x80000000) ? 1:0) ) ^ sec[i] ^ i;
          fprintf(stdout,"%08x %s\n",  bootblockchecksum, Image);
      } else { fprintf(stdout,"xxxxxxxx Non-bootable: %s\n",Image); }

  dc42_close_image(&F);
}

int main(int argc, char *argv[])
{
  int quiet=0;

  for (int i=1; i<argc; i++) {
     if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0 )
        {
            printf("DC42 Boot Block Checksum\n\n"
                //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
                //  0         1         2         3         4         5         6         7         8
                   "Usage: dc42-bootblock-checksum image1.dc42 image2.dc42 ...\n\n"
                   "          -h | --help  this help screen\n"
                   "          -q | --quiet don't show copyright banner\n\n"
                   "This utility is used to report boot block checksums as used in LisaEm to detect OS\n"
            );

            exit(0);
        }
     else if (strcmp(argv[i],"-q")==0 || strcmp(argv[i],"--quiet")==0 ) quiet=1;
  }

  if (!quiet) {
     puts(    "  ---------------------------------------------------------------------------");
     puts(    "    DC42 Boot Block Checksum Report v0.01            http://lisaem.sunder.net");
     puts(    "  ---------------------------------------------------------------------------");
     puts(    "          Copyright (C) 2022, Ray A. Arachelian, All Rights Reserved.");
     puts(    "              Released under the GNU Public License, Version 2.0");
     puts(    "    There is absolutely no warranty for this program. Use at your own risk.  ");
     puts(    "  ---------------------------------------------------------------------------\n");
  }

  for (int i=1; i<argc; i++) {
     if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0 || // skip over options
         strcmp(argv[i],"-q")==0 || strcmp(argv[i],"--quiet")==0 )
	continue;


   process(argv[i]);

  }

  exit(0);
}

