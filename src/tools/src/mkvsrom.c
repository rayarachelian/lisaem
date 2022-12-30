/*******************************************************************************\
*              Make an Apple Lisa VSROM from a Serial Number                    *
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
#include <machine.h>
#include <string.h>

uint16 get_sn_stored_checksum_bytes(uint8 *sn2)   {
      //fprintf(OUT,"sn2[24]=\n%02x, sn2[25]=%02x, sn3[26]=%02x\n",sn2[24], sn2[25], sn2[26]);
   return (sn2[24] * 100 + sn2[25] *10 + sn2[26]); }

uint16 get_sn_checksum_bytes(uint8 *sn2) { // based on the "H" ROM Source code @ 0cb2
       uint16 d2=0;
       for  (int d1=0; d1<24; d1++) {/*if (d1!=15)*/ d2 += (uint16)(sn2[d1] & 0x0f);}
       return d2+(uint16)(sn2[27] & 0x0f)-60; // wonder if the 60 is related to 60Hz?
}

FILE *OUT;
int main(int argc, char *argv[])
{
  int i,snbit;
  uint8 sn[64];
  uint8 sn2[64];
  #define SIZE 16

  puts(" =================================================================================" );
  puts(" =                                                                               =" );
  puts(" =              Make an Apple Lisa VSROM from a Serial Number                    =" );
  puts(" =       Copyright (C) 2022 by Ray Arachelian, All Rights Reserved               =" );
  puts(" =                      https://lisaem.sunder.net                                =" );
  puts(" =               Released under the terms of the GNU GPL 3.0                     =" );
  puts(" =                                                                               =" );
  puts(" =            see: https://www.gnu.org/licenses/gpl-3.0.en.html                  =" );
  puts(" =      https://lisalist2.com/index.php/topic,193.msg1486.html#msg1486           =" );
  puts(" =                                                                               =" );
  puts(" =================================================================================\n" );

  if  (argc!=3) {
      fprintf(stderr,"This tool creates a burnable (untested) Video State ROM for the Apple Lisa");
      fprintf(stderr,"given a specific hex serial number string (without the extra zeros).\n");
      fprintf(stderr,"This code has not been tested fully, YMMV.\n\n");
      fprintf(stderr,"Usage: mkvsrom {serial#} {filename.bin}\n");
      fprintf(stderr,"i.e.   mkvsrom ff000000000000ff0000000000000000 magiczero-vsrom.bin\n");
      fprintf(stderr,"       mkvsrom ff028308104050ff0010163504700000 rayslisa-vsrom.bin\n\n");
      exit(0);
  }

   //fprintf(OUT,"14:%c 15:%c 16:%c 17:%c len:%d\n\n",argv[1][14], argv[1][15],argv[1][16], argv[1][17], strlen(argv[1]));
   if (tolower(argv[1][ 0])=='f'  &&  tolower(argv[1][ 1])=='f' && 
       tolower(argv[1][14])=='f'  &&  tolower(argv[1][15])=='f' && strlen((const char *)(argv[1]))==32 ) {
       for  (i=0; i<32; i+=2) {
           uint8 c1=tolower(argv[1][i]), c2=tolower(argv[1][i+1]);
           
           if      (c1>='0' && c1<='9') c1=c1-'0';
           else if (c1>='a' && c1<='f') c1=c1-'a'+10;
           else    {fprintf(stderr,"error: Non-hex character detected %c in serial #",argv[1][i]); exit(3);}
           
           if      (c2>='0' && c2<='9') c2=c2-'0';
           else if (c2>='a' && c2<='f') c2=c2-'a'+10;
           else    {fprintf(stderr,"error: Non-hex character detected %c in serial #",argv[1][i+1]); exit(3);}

           sn[i/2 + 8]=(c1<<4) | c2;
           sn2[i]=c1 & 0x000f; sn2[i+1]=c2 & 0x000f;
       }
   }
   else {fprintf(stderr,"That serial number doesn't look correct. Are the ff sync bytes in the right place? Is the length correct?\n"); exit(4);}

   uint16 stored=get_sn_stored_checksum_bytes(sn2);
   uint16 calculated=get_sn_checksum_bytes(sn2);

   fprintf(stdout,"\nSN Checksum Stored:     %04d\n",  stored);
   fprintf(stdout,  "SN Checksum Calculated: %04d\n",  calculated);
   if (stored!=calculated) fprintf(OUT,"**WARNING: invalid checksum: will cause a CPU error in the Lisa's POST ***\n");


//uint8 magic_serialnumber[SIZE+1]= {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//                                   00   01   02   03   04   05   06   07   08   09   10   11   12   13   14   15


uint8 ROM[256]={ // this is common to all non 3A VSROMs - i.e. stripped out bit 7.
      0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
      0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
      0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x76, 0x76, 0x72, 0x62, 0x62, 
      0x62, 0x62, 0x62, 0x72, 0x72, 0x72, 0x72, 0x73, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
      0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
      0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 
      0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x76, 0x76, 0x72, 0x62, 0x62, 
      0x62, 0x62, 0x62, 0x72, 0x72, 0x72, 0x72, 0x73, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
      0x44, 0x40, 0x44, 0x40, 0x44, 0x40, 0x44, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
      0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
      0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x14, 0x14, 0x54, 0x54, 0x50, 0x40, 0x40, 
      0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 
      0x56, 0x52, 0x56, 0x52, 0x56, 0x52, 0x56, 0x12, 0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 
      0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 0x16, 0x12, 
      0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x52, 0x52, 0x52, 0x42, 0x42, 
      0x42, 0x42, 0x42, 0x52, 0x52, 0x52, 0x52, 0x5b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b, 0x3b};

#define RESET (1)
#define VSYNC (2)
#define BIT2  (4)
#define BIT3  (8)
#define CSYNC (16)
#define VSIR  (32)
#define HSYNC (64)
#define SERNO (128)


// extract serial number bit from array, if we go over, leave what's in the ROM
for (i=0; i<256; i++) { 
    if ( (i/8) < SIZE )  snbit=(sn[i / 8] & (1<<( i & 7) ) ) ? 128:0;
    else                 snbit=(sn[i] & SERNO) ? 128:0;

    ROM[i]=(ROM[i] & 127) | snbit;   // replace serial number bit  

    //printf("%3d:%02x RST:%d VSYNC:%d CSYNC:%d VSIR:%d HSYNC:%d SERNO:%d 2:%d 3:%d\n",i,vsrom[i],
    // (vsrom[i] & RESET) ? 1:0,
    // (vsrom[i] & VSYNC) ? 1:0,
    // (vsrom[i] & CSYNC) ? 1:0,
    // (vsrom[i] & VSIR ) ? 1:0,
    // (vsrom[i] & HSYNC) ? 1:0,
    // (vsrom[i] & SERNO) ? 1:0,
    // (vsrom[i] & BIT2 ) ? 1:0,
    // (vsrom[i] & BIT3 ) ? 1:0   );

  }

OUT=fopen(argv[2],"wb");               if (!OUT) {fprintf(stderr,"Could not create ROM file %s for writing\n",argv[2]); perror(""); exit(9);}
i=fwrite(ROM,256,1,OUT); fclose(OUT);  if (i!=1) {fprintf(stderr,"Could not write data to ROM file %s\n",     argv[2]); perror(""); exit(9);}
fprintf(stderr,"VSROM written successfully to %s\n",argv[2]);

exit(0);
}