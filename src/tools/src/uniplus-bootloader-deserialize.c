/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) 2021  Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
*                UniPlus v1.4 Boot Loader Deserialize Tool                             *
*                                                                                      *
*  This tool will disable the "Sorry, but this is not the right computer." error for   *
*  UniPlus floppy boot disk a, as well as profile/widget hard drives, to allow you     *
*  to run an existing UniPlus install, or install a new one on LisaEm or a Lisa.       *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>


int checkandpatch(DC42ImageType *F, int n) {

     uint8 sec[512];
     uint8 *fsec;
     
     fsec = F->read_sector_data(F,n);
     memcpy(sec,fsec,512);

     if (sec[0x90]==0x2f && sec[0x91]==0x3c && sec[0x92]==0x00 && sec[0x93]==0x06 && 
         sec[0x94]==0x56 && sec[0x95]==0xc0 && sec[0x96]==0x4e && sec[0x97]==0xb9 && 
         sec[0x98]==0x00 && sec[0x99]==0x06 && sec[0x9a]==0x04 && sec[0x9b]==0x38   )
     {
       sec[0x90]=0x42;  sec[0x91]=0x80;  sec[0x92]=0x42; sec[0x93]=0x87;
       sec[0x94]=0x4e;  sec[0x95]=0x71;  sec[0x96]=0x4e; sec[0x97]=0x71;
       sec[0x98]=0x4e;  sec[0x99]=0x71;  sec[0x9a]=0x4e; sec[0x9b]=0x71;
       printf("Found check and patched at sector %d\n",n);
       F->write_sector_data(F,n,sec);
       return 1;
     }

     return 0;
}


void deserialize(DC42ImageType *F) {
     
     int patched=0;

     uint8 sec[512];
     char *fsec;
     
     patched|=checkandpatch(F, 1);
     patched|=checkandpatch(F,13);
     
     if (!patched) fprintf(stderr,"Could not find anything to patch.\n");

}


int main(int argc, char *argv[])
{
  int i,j;

  DC42ImageType  F;
  char creatortype[10];

      puts("  ---------------------------------------------------------------------------");
      puts("    uniplus-bootloader-deserialize                  http://lisaem.sunder.net");
      puts("  ---------------------------------------------------------------------------");
      puts("          Copyright (C) 2021, Ray A. Arachelian, All Rights Reserved.");
      puts("              Released under the GNU Public License, Version 2.0");
      puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
      puts("  ---------------------------------------------------------------------------\n");

  if (argc<=1)
  {
      puts("");
      puts("   This tool will disable the \"Sorry, but this is not the right computer.\" error for");
      puts("   UniPlus floppy-boot-disk-a, as well as profile/widget hard drives.");
      puts("");
      puts("  Usage: uniplus-bootloader-deserialize {file 1} {file 2}");
      puts("  i.e.  ./uniplus-bootloader-deserialize boot-serialization-a.dc42 profile.dc42");
      exit(0);
  }


  for (i=1; i<argc; i++)
  {
     int ret=0;
     printf("\n: %-50s ",argv[i]);

     ret=dc42_auto_open(&F, argv[i], "wb");
     if (!ret) deserialize(&F);
     else      fprintf(stderr,"Could not open image %s because %s\n",argv[i],F.errormsg);
     F.close_image(&F);
  }

  return 0;
}

