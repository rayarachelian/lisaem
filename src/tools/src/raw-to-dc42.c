/**************************************************************************************\
*                                                                                      *
*                        raw2dc42 profile hd image converter                           *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                                                                                      *
*                        Copyright (C) 2011 Ray A. Arachelian                          *
*                                All Rights Reserved                                   *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <libdc42.h>


// ---------------------------------------------

long interleave5(long sector)
{
  static const int offset[]={0,5,10,15,4,9,14,3,8,13,2,7,12,1,6,11,16,21,26,31,20,25,30,19,24,29,18,23,28,17,22,27};
  return offset[sector&31] + sector-(sector&31);
}



long deinterleave5(long sector)
{
  static const int offset[]={0,13,10,7,4,1,14,11,8,5,2,15,12,9,6,3,16,29,26,23,20,17,30,27,24,21,18,31,28,25,22,19};
  return offset[sector&31] + sector-(sector&31);
}

int main(int argc, char *argv[])
{
    long numblocks,i,b;
    FILE *raw;
    DC42ImageType profile;

    char dc42filename[8192]; 
    uint8 block[533];


      puts("  ---------------------------------------------------------------------------");
      puts("    raw2dc42 V0.0.1 Profile Drive Converter          http://lisaem.sunder.net");
      puts("  ---------------------------------------------------------------------------");
      puts("          Copyright (C) 2011, Ray A. Arachelian, All Rights Reserved.");
      puts("              Released under the GNU Public License, Version 2.0");
      puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
      puts("  ---------------------------------------------------------------------------\n");


    if (argc<2)
    {
     puts("\n"
//345678901234567890123456789012345678901234567890123456789012345678901234567890
//       1         2         3         4         5         6         7
"\n"
"  Usage: raw2dc42 profile.raw\n"
"\n"
"  This program takes a Lisa Profile drive saved as 20 bytes of tag data,\n"
"  followed by 512 bytes of sector data, such as those created by the\n"
"  IDLE emulato, and converts them to the dc42 profile format usable by LisaEm\n"
"\n"
"\n");
    exit(1);
    }

    raw=fopen(argv[1],"rb");
    if (!raw) {perror("Could not open the raw image."); exit(2);}

    snprintf(dc42filename,8192,"%s.dc42",argv[1]);
    fseek(raw, 0L, SEEK_END);
    numblocks=ftell(raw)/(512+20);
    fseek(raw, 0L, SEEK_SET);
    
    i=dc42_create(dc42filename,"-lisaem.sunder.net hd-", numblocks*512,numblocks*20);
    if (i) {fprintf(stderr,"Could not create DC42 ProFile:%s\n",dc42filename); perror(" "); exit(1);}

    i=dc42_open(&profile,dc42filename,"wm");
    if (i) {fprintf(stderr,"Could not open created DC42 ProFile:%s because %s\n",dc42filename,profile.errormsg); exit(1); }

    for (b=0; b<numblocks; b++)
    {
      long b5=deinterleave5(b);

      printf ("reading raw block %04d, writing to profile block %04d \r",b,b5);
      i=fread(block,512+20,1,raw);
      if (i!=1) {fprintf(stderr,"\n\nWARNING: Error reading block # %d, fread size did not return 1 block, got %d blocks!\n",b,i); }

      i=dc42_write_sector_tags(&profile,b5, &block[0]);
      if (i) {fprintf(stderr,"\n\nError writing block tags %d to dc42 ProFile:%s because %s\n",b,dc42filename,profile.errormsg); dc42_close_image(&profile); exit(1); }

      i=dc42_write_sector_data(&profile,b5, &block[20]);
      if (i) {fprintf(stderr,"\n\nError writing block data %d to dc42 ProFile:%s because %s\n",b,dc42filename,profile.errormsg); dc42_close_image(&profile); exit(1); }
     }
     dc42_close_image(&profile);
     puts ("                                                               \rDone.");
     fclose(raw);
     return 0;
}
