/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) 2021  Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
*                     Depassword Lisa Office System Documents                          *
*                                                                                      *
*  WARNING! DO NOT USE THIS CODE! IT DOES NOT WORK! EACH LOS Document has a checksum   *
*  that's tied to the document and the password. If you tamper with the password/      *
*  length of checksum field, LOS will say the file is damaged and will refuse to open  *
*  it, or it will ask for a password you cannot type in.                               *
*                                                                                      *
*  While the document is not encrypted with the password, you'll be able extract it    *
*  using lisafsh-tool, but you just won't be able to open it.                          *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>

void depassword(DC42ImageType *F)
{
         uint8 *ftag;
         uint8 *fsec;
         
         for (int sec=32; (unsigned)sec<F->numblocks; sec++)
         {
               char name[64];
               ftag=F->read_sector_tags(F,sec);

               if (ftag[4]==0xff)         // extent tags have tag 4 as ff
               {
                   uint32 fileid=(uint16)(ftag[4]<<8)|(ftag[5]);
                   fileid=(uint32)(0x10000-fileid) & 0xffff;

                   fsec=F->read_sector_data(F,sec);

                   if  (fsec[0x62]!=0) {

                       char filename[64];
                       char deskname[64];

                       memset(filename,0,63);
                       memset(deskname,0,63);

                       uint8 buf[512];
                       memcpy(buf,fsec,512);

                       uint8 len=fsec[0];
		                   for (int i=0; i<len && i<32; i++) filename[i]=fsec[i+1];

                       len=fsec[0x182];
		                   for (int i=0; i<len && i<64; i++) deskname[i]=fsec[i+0x183];

                       fprintf(stderr,"Found and de-passworded document fileid: %04x, extent sector #%d (0x%40x) passwd length:%d%s\n   filename: %s\n   DesktopName: %s\n\n",
                               fileid, sec,sec, fsec[0x62],
                               ((fsec[0x62]>7) ? " or more":" exactly"),
                               filename, deskname);

		                   buf[0x62]=0; // set password length to zero in our copy

		                   buf[0x62+1]=0; // zero out password
		                   buf[0x62+2]=0; // zero out password
		                   buf[0x62+3]=0; // zero out password
		                   buf[0x62+4]=0; // zero out password
		                   buf[0x62+5]=0; // zero out password
		                   buf[0x62+6]=0; // zero out password
		                   buf[0x62+7]=0; // zero out password
		                   buf[0x62+8]=0; // zero out password

                       F->write_sector_data(F,sec,buf); // write the sector back to the image

		               } // password length >0

               } // has extent like tag

        } // sector loop
}



int main(int argc, char *argv[])
{
  int i,j;

  DC42ImageType  F;
  char creatortype[10];

      puts("  ---------------------------------------------------------------------------");
      puts("    Lisa Tool Depassword v0.0.1                     http://lisaem.sunder.net");
      puts("  ---------------------------------------------------------------------------");
      puts("          Copyright (C) 2021, Ray A. Arachelian, All Rights Reserved.");
      puts("              Released under the GNU Public License, Version 2.0");
      puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
      puts("  ---------------------------------------------------------------------------\n");

  if (argc<=1)
  {
      puts("");
      puts("  This program is used to remove all document passwords from a specific disk image");
      puts("  and allow opening them from Lisa Office System without having to type a password.");
      puts("");
      puts("  Usage:   los-depassword {filename1} {filename2} ... {filename N}");
      puts("  i.e.   ./los-depassword SomeImage.dc42 SomeOtherImage.dc42");
      exit(0);
  }


  for (i=1; i<argc; i++)
  {
     int ret=0;
     printf("\nDepasswording: %-50s ",argv[i]);

     ret=dc42_auto_open(&F, argv[i], "wb");
     if (!ret) depassword(&F);
     else      fprintf(stderr,"Could not open image %s because %s\n",argv[i],F.errormsg);
     F.close_image(&F);
  }

return 0;
}

