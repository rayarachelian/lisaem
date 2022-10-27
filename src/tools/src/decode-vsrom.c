/*******************************************************************************\
*                  Decode Apple Lisa VSROM Serial Number                        *
*       Copyright (C) 2022 by Ray Arachelian, All Rights Reserved               *
*                      https://lisaem.sunder.net                                *
*                                                                               *
*               Released under the terms of the GNU GPL 3.0                     *
*            see: https://www.gnu.org/licenses/gpl-3.0.en.html                  *
*                                                                               *
*            "Oh boy, here I go codin' again! I just love codin'"               *
*                                                                               *
\*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <machine.h>

#define OUT stdout

uint8 vsrom[256];


uint16 get_sn_stored_checksum_bytes(uint8 *sn2)   { 
      //fprintf(OUT,"sn2[24]=\n%02x, sn2[25]=%02x, sn3[26]=%02x\n",sn2[24], sn2[25], sn2[26]);
   return (sn2[24] * 100 + sn2[25] *10 + sn2[26]); }

uint16 get_sn_checksum_bytes(uint8 *sn2) { // based on the "H" ROM Source code @ 0cb2
       uint16 d2=0;
       for  (int d1=0; d1<24; d1++) { d2 += (uint16)(sn2[d1] & 0x0f);}
       return d2+(uint16)(sn2[27] & 0x0f)-60; // wonder if the 60 is related to 60Hz?
}


int main(int argc, char *argv[])
{
  FILE *ROM;
  int i;

  uint8  sn[64];
  uint8  sn2[64];
  memset(sn, 0,64);
  memset(sn2,0,64);

  // VSROM is 256 bytes, serial is 16 bytes, applenet + checksum + unknown is also another 16 bytes after

  #define RESET (  1)
  #define VSYNC (  2)
  #define BIT2  (  4)
  #define BIT3  (  8)
  #define CSYNC ( 16)
  #define VSIR  ( 32)
  #define HSYNC ( 64)
  #define SERNO (128)


  puts(" =================================================================================" );
  puts(" =                                                                               =" );
  puts(" =                  Decode Apple Lisa VSROM Serial Number                        =" );
  puts(" =       Copyright (C) 2022 by Ray Arachelian, All Rights Reserved               =" );
  puts(" =                      https://lisaem.sunder.net                                =" );
  puts(" =               Released under the terms of the GNU GPL 3.0                     =" );
  puts(" =                                                                               =" );
  puts(" =            see: https://www.gnu.org/licenses/gpl-3.0.en.html                  =" );
  puts(" =      https://lisalist2.com/index.php/topic,193.msg1486.html#msg1486           =" );
  puts(" =                                                                               =" );
  puts(" =================================================================================\n" );

  if  (argc!=2) {
      fprintf(stderr,"Usage: decode-vsrom videostate.rom\n");
      fprintf(stderr,"or:    decode-vsrom ff028308104050ff0010163504700000\n\n");
      exit(1);
  }


  ROM=fopen(argv[1],"rb");
  if  (!ROM) { 
      //fprintf(OUT,"14:%c 15:%c 16:%c 17:%c len:%d\n\n",argv[1][14], argv[1][15],argv[1][16], argv[1][17], strlen(argv[1]));
      if (tolower(argv[1][ 0])=='f'  &&  tolower(argv[1][ 1])=='f' && 
          tolower(argv[1][14])=='f'  &&  tolower(argv[1][15])=='f' && strlen(argv[1])==32 ) {

         for  (i=0; i<32; i+=2) {
              uint8 c1=tolower(argv[1][i]), c2=tolower(argv[1][i+1]);
              
              if      (c1>='0' && c1<='9') c1=c1-'0';
              else if (c1>='a' && c1<='f') c1=c1-'a'+10;
              else    {fprintf(stderr,"error: Non-hex character detected %c",argv[1][i]); exit(3);}
              
              if      (c2>='0' && c2<='9') c2=c2-'0';
              else if (c2>='a' && c2<='f') c2=c2-'a'+10;
              else    {fprintf(stderr,"error: Non-hex character detected %c",argv[1][i+1]); exit(3);}
              
              sn[i/2 + 8]=(c1<<4) | c2;
              
              sn2[i]=c1 & 0x000f; sn2[i+1]=c2 & 0x000f;
      }
      for (i=0; i<8; i++) {sn[i]=sn[i+8];} // first 8 bytes are duplicated for some reason


  } else {fprintf(stderr,"Could not open VSROM: %s and that doesn't look like a hex serial #\nAre sync bytes in the right place? are there non-hex digits? is the length right?\n",argv[1]); exit(2);} 
  }
  else {
        i=fread(vsrom,256,1,ROM);
      
        for (i=0; i<256; i++)
         {
          if (vsrom[i ^ 128] & SERNO) sn[i/8] |=1<<(7-(i & 7));
      /*  printf("%3d:%02X RST:%d VSYNC:%d CSYNC:%d VSIR:%d HSYNC:%d SERNO:%d 2:%d 3:%d\n",i,vsrom[i],
           (vsrom[i] & RESET) ? 1:0,
           (vsrom[i] & VSYNC) ? 1:0,
           (vsrom[i] & CSYNC) ? 1:0,
           (vsrom[i] & VSIR ) ? 1:0,
           (vsrom[i] & HSYNC) ? 1:0,
           (vsrom[i] & SERNO) ? 1:0,
           (vsrom[i] & BIT2 ) ? 1:0,
           (vsrom[i] & BIT3 ) ? 1:0   ); */
         } 

        fclose(ROM);

        for  (i=0; i<16; i++) {
             sn2[i*2    ]=(sn[i+8] & 0xf0) >> 4; //fprintf(OUT,"%02x", i*2,   sn2[i*2]);
             sn2[i*2 + 1]=(sn[i+8] & 0x0f)     ; //fprintf(OUT,"%02x ",i*2+1, sn2[i*2+1]);
        }

}


  // ff000000000000ff0000000000000000
  // ff028308104050ff0010163504700000
  // ff108324300860ff0010208805405d2c - https://lisalist2.com/index.php/topic,313.0.html - checksum works on this one!
  //  0 1 2 3 4 5 6 7 8 9 a b c d e f


  fprintf(OUT,"S/N for use with LisaEm Preferences: ");
  for (i=0; i<32; i++) {fprintf(OUT,"%01x",sn2[i]);}
  fprintf(OUT,"\n");

  fprintf(OUT,"\n\nService Mode Display:\n\n");
  fprintf(OUT,"240 Serial #: ");
  for (i=0; i<32; i++) { 
     fprintf(OUT,"%02X",sn2[i]);
     if (i & 1) fprintf(OUT," ");
     if (i==15) fprintf(OUT,          "\n260 AppleNet: ");
  }

  uint16 stored=get_sn_stored_checksum_bytes(sn2);
  uint16 calculated=get_sn_stored_checksum_bytes(sn2);
  
  fprintf(OUT,"\nSN Checksum Stored:     %04d\n",  stored);
  fprintf(OUT,  "SN Checksum Calculated: %04d\n",  calculated);

  if (stored!=calculated) fprintf(OUT,"**WARNING: invalid checksum: will cause a CPU error in the Lisa's POST ***\n");

  fprintf(OUT,"\n\nSerial Number of this Lisa (BLU format)\n\n");

  fprintf(OUT,"    ssLLYYDD DUUUUXss\n");
  fprintf(OUT,"    %02X%02X%02X%02X %02X%02X%02X%02X\n", 
          sn[0],sn[1],sn[2],sn[3],sn[4],sn[5],sn[6],sn[7],
          sn[8],sn[9],sn[10],sn[11],sn[12],sn[13],sn[14],sn[15]);

  fprintf(OUT,"\n    PPPNNNNN cccdXXXX\n");
  fprintf(OUT,"    %02X%02X%02X%02X %02X%02X%02X%02X\n\n", 
              sn[16+1],sn[16+2],sn[16+3],sn[16+4],
              sn[16+5],sn[16+6],sn[16+7],sn[16+8]);

  //                     1         2         3         4         5         6         7         8
  //           012345678901234567890123456789012345678901234567890123456789012345678901234567890
  fprintf(OUT,"X    - Unused/Unknown      LL    - Plant Location  ss - Sync Bytes\n");
  fprintf(OUT,"YY   - Year manufactured   DDD   - Day of Year\n");
  fprintf(OUT,"UUUU - Unit Number         ccc   - cksum,           e - byte added to cksum\n");
  fprintf(OUT,"PPP  - AppleNet Prefix     NNNNN - AppleNet ID\n\n");
}
