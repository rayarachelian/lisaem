/**************************************************************************************\
*                                                                                      *
*               DC42 Checksum - report on tag/data checksums + repair                  *
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                     Copyright (C) 2022 Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
\**************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>

void process(char *Image, int do_fix);

int main(int argc, char *argv[])
{
  int do_fix=0, quiet=0, header=1;

  for (int i=1; i<argc; i++) {
     if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0 )
        {
            printf("DC42 Checksum and Fix \n\n"
                //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
                //  0         1         2         3         4         5         6         7         8
                   "Usage: dc42-checksum {-f|--fix} image1.dc42 image2.dc42 ...\n\n"
		   "          -f | --fix   recalculate and fix the checksums\n"
		   "          -h | --help  this help screen\n"
		   "          -q | --quiet don't show copyright banner\n\n"
                   "This utility is used to report DC42 image checksums and optionally fix them\n"
            );

            exit(0);
        }
     else if (strcmp(argv[i],"-f")==0 || strcmp(argv[i],"--fix")==0 )   do_fix=1;
     else if (strcmp(argv[i],"-q")==0 || strcmp(argv[i],"--quiet")==0 ) quiet=1;
  }

  if (!quiet) {
     puts(    "  ---------------------------------------------------------------------------");
     puts(    "    DC42 Checksum Report and Fix v0.01               http://lisaem.sunder.net");
     puts(    "  ---------------------------------------------------------------------------");
     puts(    "          Copyright (C) 2022, Ray A. Arachelian, All Rights Reserved.");
     puts(    "              Released under the GNU Public License, Version 2.0");
     puts(    "    There is absolutely no warranty for this program. Use at your own risk.  ");
     puts(    "  ---------------------------------------------------------------------------\n");
  }

  for (int i=1; i<argc; i++) {
     if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0 || // skip over options
         strcmp(argv[i],"-f")==0 || strcmp(argv[i],"--fix")==0  ||
         strcmp(argv[i],"-q")==0 || strcmp(argv[i],"--quiet")==0 )
	continue;

   if  (header) {
       fprintf(stderr,
	             // 0        1         2         3         4         5         6         7         8
	             //.12345678901234567890123456789012345678901234567890123456789012345678901234567890
                  "|==== tag checksum ====| ==== data checksum ====| === status ===|Filenames===\n"
                  "| header     calculated| header   ?? calculated | good|mismatch,|            \n"
                  "| value   ?? from tags | value    ?? from data  |     fixed?    |            \n"
                  "|______________________|________________________|_______________|____________\n"
         );
       header=0;
   }

   process(argv[i],do_fix);

  }

  exit(0);
}


// report on and optionally fix image checksums
void process(char *Image, int do_fix) {
  DC42ImageType  F;
  int failed=0;

  if (!Image)  {fprintf(stderr,"No image name passed\n");                             return;}

  if (dc42_auto_open(&F,Image,do_fix ? "w":"p")) 
               {fprintf(stderr,"could not open: '%s' because:%s\n",Image,F.errormsg); 
		perror("\n");                                                         return;}

  if (dc42_has_tags(&F)) {
     uint32 stored=dc42_get_tagchecksum(&F);
     uint32 calculated=dc42_calc_tag_checksum(&F);
     failed |= (stored != calculated);

     fprintf(stdout," %08x %s %08x     ", stored, (stored==calculated) ? "==":"!=", calculated);		     
  }
  else
     fprintf(stdout,"                          ");

  {  uint32 stored=dc42_get_datachecksum(&F);
     uint32 calculated=dc42_calc_data_checksum(&F);
     failed |= (stored != calculated);
     fprintf(stdout," %08x %s %08x         ", stored, (stored==calculated) ? "==":"!=", calculated);		     
  }

  //                       .123456789. .123456789.
  fprintf(stdout, failed ? "MISMATCH ":"good     ");

  //                                                                .123456.
  if (do_fix && failed) {dc42_calc_tag_checksum(&F); fprintf(stdout,"FIXED ");}
  else                  {                            fprintf(stdout,"      ");}

  fprintf(stdout,": %s\n",Image);

  dc42_close_image(&F);
}
