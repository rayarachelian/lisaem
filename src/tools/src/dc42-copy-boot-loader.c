/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) MMXX  Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
*                     Copy boot block and boot loader blocks                           *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>

int main(int argc, char *argv[])
{
  int i,j;

  DC42ImageType  Fsrc, Ftarget;

  puts(    "  ---------------------------------------------------------------------------");
  puts(    "    DC42 Boot Loader Copy v0.01                http://lisaem.sunder.net");
  puts(    "  ---------------------------------------------------------------------------");
  puts(    "          Copyright (C) MMXX, Ray A. Arachelian, All Rights Reserved.");
  puts(    "              Released under the GNU Public License, Version 2.0");
  puts(    "    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts(    "  ---------------------------------------------------------------------------\n");
  puts(    " WARNING: While this tool does what it says, the result is 10726 error when");
  puts(    " trying to boot, some other changes are needed to let LOS know the device has");
  puts(    " changed from Widget to profile.");

  if (argc!=3)
  {
    puts("");
    puts("  This program is used to copy the boot block and boot loader blocks from one");
    puts("  dc42 image to another in the hopes that you might be convert a disk image");
    puts("  made of a Widget with BLU to a ProFile one usable on Lisa, or vice versa.");
    puts("");
    puts("  Usage:  dc42-copy-boot-loader {source image} {target image}");
    puts("  i.e.   ./dc42-copy-boot-loader profile.dc42 widget.blu.dc42");
    exit(0);
  }

    int ret=0, ret1=0;

    ret=dc42_auto_open(&Fsrc, argv[1], "wb");
    if (ret) {fprintf(stderr,"Could not open source image %s because %s, returned:%d\n",argv[1],Fsrc.errormsg,ret);      Fsrc.close_image(&Fsrc); exit(1); }

    ret=dc42_auto_open(&Ftarget, argv[2], "wb");
    if (ret) {fprintf(stderr,"Could not open target image %s because %s\n",argv[2],Ftarget.errormsg);   Fsrc.close_image(&Fsrc); Ftarget.close_image(&Ftarget ); exit(1); }

    fprintf(stderr,"copying blk: ");
    for (i=0; i<80; i++)
    {
        uint8  *tag=Fsrc.read_sector_tags(&Fsrc,i);
        uint8 *ttag=Ftarget.read_sector_tags(&Ftarget,i);

        if (!tag) {fprintf(stderr,"Error reading tags from source image\n"); Fsrc.close_image(&Fsrc); Ftarget.close_image(&Ftarget ); exit(1);}
        if ( (( tag[4]==0xaa    &&  tag[5]==0xaa) || (tag[4]==0xbb && tag[5]==0xbb)) &&
              (ttag[4])==tag[4] && ttag[5]==tag[5] )  {
                uint8 *data = Fsrc.read_sector_data(&Fsrc,i);

                ret  = Ftarget.write_sector_tags(&Ftarget,i, tag);
                ret1 = Ftarget.write_sector_data(&Ftarget,i, data);

                if (ret || ret1) fprintf(stderr,"Error writing to block %d of target\n",i);

                fprintf(stderr,"%d ",i);
        }
    }
fprintf(stderr,"\ndone.\n");
Fsrc.close_image(&Fsrc);
Ftarget.close_image(&Ftarget);

return 0;
}

