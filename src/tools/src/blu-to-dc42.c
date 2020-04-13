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

int widget=0;

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
    int blu=-1;
    DC42ImageType profile;

    char dc42filename[8192]; 
    uint8 block[537]; // 512 bytes data block 20 bytes tags, +2 tag if PRIAM +1 for padding

    if (argc<2)
    {
     puts("\n"
//345678901234567890123456789012345678901234567890123456789012345678901234567890
//       1         2         3         4         5         6         7
"                          blu2dc42 V0.0.2 Profile Drive Converter\n"
"                              http://lisaem.sunder.net\n"
"\n"
"                           Copyright (C) MMXX Ray A. Arachelian  \n"
"                                    All Rights Reserved.          \n"
"\n"
"  Usage: blu2dc42 profile.blu\n"
"\n"
"  This program takes a BLU ProFile/Widget/Priam disk image and\n"
"  converts it to dc42 format usable by LisaEm\n"
"\n"
"\n");
    exit(1);
    }

    blu=open(argv[1],O_RDONLY);
    if (!blu) {perror("Could not open the BLU image."); exit(2);}
    lseek(blu, 128L, SEEK_END);
    i=read(blu,block,32); // get around UTF issue?
    snprintf(dc42filename,8192,"%s.dc42",argv[1]);
    numblocks=lseek(blu, 0L, SEEK_END);
    lseek(blu, 0L, SEEK_SET);
    
    /* Read BLU header block - assume widget/profile and not priam */
    i=read(blu,block,512+20);
    if (i!=532) 
    {  
      fprintf(stderr,"\n\nWARNING: Error reading BLU header block, fread size did not return 532 bytes block, got %d blocks!\n",i);
      close(blu);
      exit(2);
    }

    char *blusign=(char *)&block[512];  // BLU signature is in the tags of the spare table
    uint8 x=blusign[15];
    blusign[15]=0; // force terminate string so we don't overrun
    int blublocks=( (block[0x12]<<16) | (block[0x13]<<8) | (block[0x14]    ) );
    int blocksize=(                     (block[0x15]<<8) | (block[0x16]    ) );
    numblocks=numblocks/blocksize; // correct to blocks

    if (strncmp(blusign,"Lisa HD Img BLU",15)!=0)
    {
      fprintf(stderr,"This doesn't appear to be a BLU image\n");
      close(blu);
      exit(2);
    }

    blusign[15]=x;
    if (strlen((char *)block)<16)
    {
      blusign[19]=0; // terminate version
      fprintf(stderr, "Created by %s\n",blusign);
      fprintf(stderr, "Drive image is from: %s of type %02x%02x%02x\n",block,block[0x0D],block[0x0E],block[0x0F]);
      fprintf(stderr, "Number of Blocks:%d size of each block:%d\n\n",blublocks,blocksize);

      if (strncmp(blusign,"Widget-10    ",15)==0) widget= 1;
      if (strncmp(blusign,"PROFILE      ",15)==0) widget=-1;
       //                  01234567801234
    }
    /*
Created by Lisa HD Img BLUV0.9
Drive image is from: Widget-10     of type 000100
Number of Blocks:19456 size of each block:532

00000000  57 69 64 67 65 74 2d 31  30 20 20 20 20 00 01 00  |Widget-10    ...|
00000010  1a 45 00 4c 00 02 14 02  02 02 13 00 00 4c 00 00  |.E.L.........L..|
00000020  02 00 00 00 00 01 80 80  00 00 00 00 00 00 00 00  |................|
00000030  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000200  4c 69 73 61 20 48 44 20  49 6d 67 20 42 4c 55 56  |Lisa HD Img BLUV|
00000210  30 2e 39 30|4e fa 00 16  aa aa 08 50 22 4a 2d 0c  |0.90N......P"J-.|
                      boot sector data (not tags)
00000220  28 76 00 1e 00 00 00 00  4e fa 00 16 aa aa 08 50  |(v......N......P|
00000230  22 4a 2d 0c 28 76 00 1e  00 00 00 00 00 00 00 00  |"J-.(v..........|
00000240  20 38 02 a8 e2 88 21 c0  02 1c 22 40 24 7c 00 02  | 8....!..."@$|..|
00000250  00 00 47 fa ff d4 d7 fc  00 00 02 00 b5 cb 6e 04  |..G...........n.|
00000260  22 da 60 f8 49 fa 00 0c  d9 c0 99 fc 00 02 00 00  |".`.I...........|
00000270  4e d4 43 fa ff c8 22 88  43 fa ff be 22 97 d1 91  |N.C...".C..."...|
00000280  04 91 00 02 00 00 2e 40  9e fc 01 00 2e 3c 00 ff  |.......@.....<..|
00000290  ff ff 2c 7c 00 00 08 00  31 fc 00 01 02 2e 61 00  |..,|....1.....a.|
000002a0  00 c0 10 2e 02 0e 67 06  31 fc 00 03 02 2e 4d fa  |......g.1.....M.|
000002b0  ff 78 20 4e dd fc 00 00  02 00 d0 fc 00 0a 3c 10  |.x N..........<.|
000002c0  04 46 02 00 7e 08 31 c7  02 16 52 47 0c 78 00 03  |.F..~.1...RG.x..|
000002d0  02 2e 66 06 9d fc 00 00  00 14 3c 06 6b 00 01 18  |..f.......<.k...|
000002e0  61 38 dc fc 02 00 04 46  02 00 52 47 4e fa ff ec  |a8.....F..RGN...|
000002f0  2c 00 24 7c 00 fc d9 01  08 12 00 01 67 fa 20 38  |,.$|........g. 8|
00000300  02 a8 04 80 00 00 80 00  21 c0 01 10 95 ca 20 0b  |........!..... .|
00000310  22 00 97 cb 4e f9 00 fe  00 84 61 44 30 2e 02 06  |"...N.....aD0...|
00000320  0c 78 00 03 02 2e 66 04  30 2e 02 04 02 40 80 00  |.x....f.0....@..|
00000330  67 2c 42 80 22 4e 34 3c  02 13 e4 4a 22 19 b3 80  |g,B."N4<...J"...|
00000340  51 ca ff fa 32 00 e0 88  e0 88 b3 40 12 00 e0 48  |Q...2......@...H|
00000350  b3 00 67 0a 26 7c 00 00  29 e6 70 ff 60 92 4e 75  |..g.&|..).p.`.Nu|
00000360  22 07 24 4e 22 4e 0c 78  00 03 02 2e 66 06 d4 fc  |".$N"N.x....f...|
00000370  00 14 60 04 d2 fc 02 00  36 3c 00 0a 38 3c 00 03  |..`.....6<..8<..|
00000380  24 3c 00 90 00 00 0c 81  00 ff ff ff 67 0a 0c 78  |$<..........g..x|
00000390  00 03 02 2e 67 02 61 34  48 e7 03 62 0c 38 00 02  |....g.a4H..b.8..|
000003a0  01 b3 6e 08 4e b9 00 fe  00 90 60 0e 41 fa fe 8a  |..n.N.....`.A...|
000003b0  26 50 41 fa fe 88 20 50  4e 93 4c df 46 c0 64 0a  |&PA... PN.L.F.d.|
000003c0  26 7c 00 00 29 e6 60 00  ff 28 4e 75 2f 00 2f 01  |&|..).`..(Nu/./.|
000003d0  70 f0 c0 01 02 41 00 0f  d0 3b 10 0c 1f 40 00 03  |p....A...;...@..|
000003e0  22 1f 20 1f 4e 75 00 05  0a 0f 04 09 0e 03 08 0d  |". .Nu..........|
000003f0  02 07 0c 01 06 0b 20 7c  00 00 01 b3 22 7c 00 fc  |...... |...."|..|
00000400  c0 31 10 11 6a 08 10 10  66 04 10 bc 00 02 41 fa  |.1..j...f.....A.|
00000410  fe 18 d0 fc 00 00 aa aa  82 00 ff ff ff 00 00 00  |................|
^tags         0  1  2    3     4   5
00000420  ff ff ff ff ff ff 24 2a  60 04 00 00 00 00 46 fc  |......$*`.....F.|
00000430  27 00 2e 7c 00 01 00 00  28 7c 00 fc d9 01 08 ec  |'..|....(|......|
..


blk:0000 tags::d0 fc 00 00 aa aa 82 00 ff ff ff 00 00 00 ff ff ff ff ff ff 4c00 ::  blu
  tags:        00 00 00 21 aa aa 82 00 ff ff ff 56 00 00 ff ff ff ff ff ff 

-----------------------------------------------------------------------------
Sec 0:(0x0000)   Used Block Part of file bootsect-aaaa:"bootsect-aaaa"
-----------------------------------------------------------------------------
            +0 +1 +2 +3 +4 +5 . +6 +7 +8 +9 +a +b +c +d +e +f+10+11+12+13
tags:       00 00 00 21 aa aa 82 00 ff ff . ff 56 00 00 ff ff ff ff ff ff 
           |volid|????|fileid||absnext|   ?   |    ?   |    next|previous
-----------------------------------------------------------------------------
      +0 +1 +2 +3 +4 +5 +6 +7 . +8 +9 +a +b +c +d +e +f                    
-----------------------------------------------------------------------------
0000: 4e fa 00 16 aa aa 08 50 . 22 4a 2c f8 28 76 00 1e   |  Nz 6**(P"J,x(v >
0010: 00 00 00 00 00 00 00 00 . 20 38 02 a8 e2 88 21 c0   |           8"(b(!@
0020: 02 1c 22 40 24 7c 00 02 . 00 00 47 fa ff d4 d7 fc   |  "<"@$| "  GzTW|


-----------------------------------------------------------------------------
Sec 9:(0x0009)   Used Block Part of file OSLoader-bbbb:"OSLoader-bbbb"
-----------------------------------------------------------------------------
            +0 +1 +2 +3 +4 +5 . +6 +7 +8 +9 +a +b +c +d +e +f+10+11+12+13
tags:       00 00 00 21 bb bb 82 00 ff ff . ff 6c 00 01 ff ff ff ff ff ff 
           |volid|????|fileid||absnext|   ?   |    ?   |    next|previous
-----------------------------------------------------------------------------
      +0 +1 +2 +3 +4 +5 +6 +7 . +8 +9 +a +b +c +d +e +f                    
-----------------------------------------------------------------------------
0000: 32 50 2f 09 42 a7 42 a7 . 42 a7 41 fa fd f4 22 48   |  2P/)B'B'B'Az}t"H


    */

    i=lseek(blu, blocksize, SEEK_SET); // 0x214 = 532  // 20200309 -2 to fix file id tag for bootblock, not sure why it's needed yet.
    if (i  !=    blocksize  )          // except that now the data is off by two breaking it. I think we're missing two tag bytes at the front
    {
      fprintf(stderr,"lseek failed to position to %d instead got %d\n",blocksize,i);
      numblocks=blublocks;
      close(blu);
      exit(33);
    }

    numblocks--; // skip spare table/ BLU header block

    if ((numblocks - blublocks)>2) // might have one extra block at the end due to xmodem buffering, but more than that is an error
    {
      fprintf(stderr,"Warning: number of blocks in BLU header (%d) doesn't match size of file (%d)\nUsing blublocks value, this is possibly just an xmodem artifact.\n",blublocks,numblocks);
      numblocks=blublocks;
    }

    i=dc42_create(dc42filename,"-lisaem.sunder.net hd-", numblocks*512,numblocks*20);
    if (i) {fprintf(stderr,"Could not create DC42 ProFile:%s\n",dc42filename); perror(" "); exit(1);}

    i=dc42_open(&profile,dc42filename,"wm");
    if (i) {fprintf(stderr,"Could not open created DC42 ProFile:%s because %s\n",dc42filename,profile.errormsg); exit(1); }

    for (b=0; b<numblocks; b++)
    {
      //long b5=deinterleave5(b);  // doesn't look like we need interleave5 for BLU
      long b5=b;
      fprintf (stderr,"reading BLU block %04d, writing to profile block %04d \r",b,b5);
      i=read(blu,block,blocksize);
      if (i!=blocksize) 
      {
          dc42_close_image(&profile);
          fprintf(stderr,"                                                               \rError.\n");
          close(blu);
          fprintf(stderr,"\nError reading block # %d, fread size did not return a %d bytes block, got %d blocks!\n",b,blocksize,i); 
          exit(1);
      }

      i=dc42_write_sector_data(&profile,b5, (uint8 *) (&block));
      if (i) {fprintf(stderr,"\n\nError writing block data %d to dc42 ProFile:%s because %s\n",b,dc42filename,profile.errormsg); dc42_close_image(&profile); exit(1); }

               //          xxxx tags: 00   01   02   03   04   05   06   07   08   09   0a   0b   0c   0d   0e   0f   10   11   12   13   
//      fprintf(stdout,"blk:%04lx tags::%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x::\n",b,
//              block[512+ 0],block[512+ 1],block[512+ 2],block[512+ 3],block[512+ 4],block[512+ 5],block[512+ 6],block[512+ 7],block[512+ 8],block[512+ 9],
//              block[512+10],block[512+11],block[512+12],block[512+13],block[512+14],block[512+15],block[512+16],block[512+17],block[512+18],block[512+19]
//              );
      // check AA,AA signature
      if (b5==0)
      {
          if (block[510+4]!=0xaa || block[510+5]!=0xaa)
          {
            fprintf(stderr,"Block 0 does not have AAAA file id! Got %02x%02x\n",block[512+4],block[512+5]);  // this should be a warning.
            dc42_close_image(&profile);
            close(blu);
            exit(5);
          }
      }

      block[510]=0; block[511]=0; // missing volid tags - so had it right the first time
      i=dc42_write_sector_tags(&profile,b5, &block[510]);  // yes this is off by 2 bytes  
      if (i) {fprintf(stderr,"\n\nError writing block tags %d to dc42 ProFile:%s because %s\n",b,dc42filename,profile.errormsg); dc42_close_image(&profile); exit(1); }
    }

    dc42_close_image(&profile);
    puts ("                                                               \rDone.");
    close(blu);
    return 0;
}
