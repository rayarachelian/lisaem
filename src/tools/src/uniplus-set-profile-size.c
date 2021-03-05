/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) 2021  Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
*                     UniPlus Set ProFile Size Partition Table                         *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <libdc42.h>

uint32 astart, bstart, cstart, dstart, estart, fstart, gstart, hstart,
       a_size, b_size, c_size, d_size, e_size, f_size, g_size, h_size;

uint32 swap   = 2400;  // default swap for 5MB/10MB ProFile. Corvus=3959, Priam=4000 (these are physical disk geometry related)
uint32 root   = 16955; // default root for 10MB+ disks
uint32 disk   = 19456;
uint32 save   = 0;

uint8 sec[512];


void set_to_size(uint32 disk)
{
   uint32 max=disk;

   if (root==0xffffffff) root=disk-swap;

   astart =  101+swap; a_size =   root;  // root on 10MB disk
   bstart =  101;      b_size =   swap;  // swap (2400 blks normal
   cstart =    0;      c_size =       0; // root on 5MB disk

   dstart = astart+a_size; 
   d_size = max-dstart;              //  2nd fs on 10MB disk
   if (!d_size || (d_size & 0x80000000)) {dstart=0; d_size=0;}

   estart =    0;      e_size =       0; //  unused
   fstart =    0;      f_size =       0; //  old root - a
   gstart =    0;      g_size =       0; //  old swap -b
   hstart =  101;      h_size = max-101; //  whole disk (blocks 0-100 reserved for boot loader)
}

void reset_to_default(void)
{
   astart = 101+swap;  a_size =   16955; //  root on 10MB disk
   bstart =  101;      b_size =    swap; //  swap (2400 blks normal
   cstart = 101+swap;  c_size =    7227; //  root on 5MB disk
   dstart = 9728;      d_size =    9728; //  2nd fs on 10MB disk
   estart =    0;      e_size =       0; //  unused
   fstart =    0;      f_size =    7168; //  old root - a
   gstart = 7168;      g_size =    2496; //  old swap -b
   hstart =  101;      h_size =   19355; //  whole disk (blocks 0-100 reserved for boot loader)
}

char *h(uint32 size) {
    static char s[12];
    char *m[6]={"","KB","MB","GB","TB","EB"};
    int mag=0;

    while (size>1024 && mag<6) {size=size/1024; mag++;}

    snprintf(s,12,"%8d %s",size,m[mag]);
    return s;
}

void printpart(uint8 *fsec)
{
     printf("a: start: %10d size: %10d blocks (%12s) root on 10MB disk\n",      astart, a_size, h(a_size*512));
     printf("b: start: %10d size: %10d blocks (%12s) swap (2400 blks normal\n", bstart, b_size, h(b_size*512));
     printf("c: start: %10d size: %10d blocks (%12s) root on 5MB disk\n",       cstart, c_size, h(c_size*512));
     printf("d: start: %10d size: %10d blocks (%12s) 2nd fs on 10MB disk\n",    dstart, d_size, h(d_size*512));
     printf("e: start: %10d size: %10d blocks (%12s) unused\n",                 estart, e_size, h(e_size*512));
     printf("f: start: %10d size: %10d blocks (%12s) old root - a\n",           fstart, f_size, h(f_size*512));
     printf("g: start: %10d size: %10d blocks (%12s) old swap -b\n",            gstart, g_size, h(g_size*512));
     printf("h: start: %10d size: %10d blocks (%12s) whole disk\n\n",           hstart, h_size, h(h_size*512));
}

void writepart(DC42ImageType *F, uint8 *fsec) {

     fsec[0x174]=(astart & 0xff000000)>>24;  fsec[0x175]=(astart & 0x00ff0000)>>16; fsec[0x176]=(astart & 0x0000ff00)>>8; fsec[0x177]=(astart & 0x000000ff);
     fsec[0x178]=(a_size & 0xff000000)>>24;  fsec[0x179]=(a_size & 0x00ff0000)>>16; fsec[0x17a]=(a_size & 0x0000ff00)>>8; fsec[0x17b]=(a_size & 0x000000ff);

     fsec[0x17c]=(bstart & 0xff000000)>>24;  fsec[0x17d]=(bstart & 0x00ff0000)>>16; fsec[0x17e]=(bstart & 0x0000ff00)>>8; fsec[0x17f]=(bstart & 0x000000ff);
     fsec[0x180]=(b_size & 0xff000000)>>24;  fsec[0x181]=(b_size & 0x00ff0000)>>16; fsec[0x182]=(b_size & 0x0000ff00)>>8; fsec[0x183]=(b_size & 0x000000ff);

     fsec[0x184]=(cstart & 0xff000000)>>24;  fsec[0x185]=(cstart & 0x00ff0000)>>16; fsec[0x186]=(cstart & 0x0000ff00)>>8; fsec[0x187]=(cstart & 0x000000ff);
     fsec[0x188]=(c_size & 0xff000000)>>24;  fsec[0x189]=(c_size & 0x00ff0000)>>16; fsec[0x18a]=(c_size & 0x0000ff00)>>8; fsec[0x18b]=(c_size & 0x000000ff);

     fsec[0x18c]=(dstart & 0xff000000)>>24;  fsec[0x18d]=(dstart & 0x00ff0000)>>16; fsec[0x18e]=(dstart & 0x0000ff00)>>8; fsec[0x18f]=(dstart & 0x000000ff);
     fsec[0x190]=(d_size & 0xff000000)>>24;  fsec[0x191]=(d_size & 0x00ff0000)>>16; fsec[0x192]=(d_size & 0x0000ff00)>>8; fsec[0x193]=(d_size & 0x000000ff);

     fsec[0x194]=(estart & 0xff000000)>>24;  fsec[0x195]=(estart & 0x00ff0000)>>16; fsec[0x196]=(estart & 0x0000ff00)>>8; fsec[0x197]=(estart & 0x000000ff);
     fsec[0x198]=(e_size & 0xff000000)>>24;  fsec[0x199]=(e_size & 0x00ff0000)>>16; fsec[0x19a]=(e_size & 0x0000ff00)>>8; fsec[0x19b]=(e_size & 0x000000ff);

     fsec[0x19c]=(fstart & 0xff000000)>>24;  fsec[0x19d]=(fstart & 0x00ff0000)>>16; fsec[0x19e]=(fstart & 0x0000ff00)>>8; fsec[0x19f]=(fstart & 0x000000ff);
     fsec[0x1a0]=(f_size & 0xff000000)>>24;  fsec[0x1a1]=(f_size & 0x00ff0000)>>16; fsec[0x1a2]=(f_size & 0x0000ff00)>>8; fsec[0x1a3]=(f_size & 0x000000ff);

     fsec[0x1a4]=(gstart & 0xff000000)>>24;  fsec[0x1a5]=(gstart & 0x00ff0000)>>16; fsec[0x1a6]=(gstart & 0x0000ff00)>>8; fsec[0x1a7]=(gstart & 0x000000ff);
     fsec[0x1a8]=(g_size & 0xff000000)>>24;  fsec[0x1a9]=(g_size & 0x00ff0000)>>16; fsec[0x1aa]=(g_size & 0x0000ff00)>>8; fsec[0x1ab]=(g_size & 0x000000ff);

     fsec[0x1ac]=(hstart & 0xff000000)>>24;  fsec[0x1ad]=(hstart & 0x00ff0000)>>16; fsec[0x1ae]=(hstart & 0x0000ff00)>>8; fsec[0x1af]=(hstart & 0x000000ff);
     fsec[0x1b0]=(h_size & 0xff000000)>>24;  fsec[0x1b1]=(h_size & 0x00ff0000)>>16; fsec[0x1b2]=(h_size & 0x0000ff00)>>8; fsec[0x1b3]=(h_size & 0x000000ff);

     printf("======= New Partition Table for profile: ==============\n");
     printpart(fsec);

     if (!save) {
         char answer[8];
         char *lf;
         fprintf(stderr,"\n\nWrite this table to the unix kernel? ");
         fgets(answer,7,stdin); lf=strchr(answer,10); if (lf) *lf=0; // remove LF

         //fprintf(stderr,"answer  ::%s:: %d\n",answer, strncasecmp(answer,"y",2) );

         if ((strncasecmp(answer,"yes",4)==0) || (strncasecmp(answer,"y",2)==0)) save=0xfffffffe;
         else fprintf(stderr,"Will not write changes.\n");
     }

     if (save==0xfffffffe) {
        int i=dc42_write_sector_data(F,526,fsec); // write the sector back to the image
        if (i) {fprintf(stderr,"\n\nError writing block data 526 to dc42 because %s\n",F->errormsg); dc42_close_image(F); exit(1); }
        fprintf(stderr,"Wrote changes to /unix uniplus kernel.\n");
     }
}

void start(DC42ImageType *F) 
{
     uint8 *ftag;
     uint8 *fsec;

     char *wrong_disk="Error. This does not appear to be the UniPlus boot disk.\n";
     ftag=dc42_read_sector_tags(F,0);   if (ftag[4]!=0xaa && ftag[5]!=0xaa) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(10);}
     ftag=dc42_read_sector_tags(F,1);   if (ftag[4]!=0xaa && ftag[5]!=0xaa) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(1);}
     ftag=dc42_read_sector_tags(F,2);   if (ftag[4]!=0xaa && ftag[5]!=0x00) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(2);}
     ftag=dc42_read_sector_tags(F,3);   if (ftag[4]!=0xaa && ftag[5]!=0xaa) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(3);}
     ftag=dc42_read_sector_tags(F,4);   if (ftag[4]!=0xaa && ftag[5]!=0x00) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(4);}
     ftag=dc42_read_sector_tags(F,5);   if (ftag[4]!=0xaa && ftag[5]!=0xaa) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(5);}
     ftag=dc42_read_sector_tags(F,6);   if (ftag[4]!=0xaa && ftag[5]!=0x00) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(6);}
     ftag=dc42_read_sector_tags(F,7);   if (ftag[4]!=0xaa && ftag[5]!=0xaa) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(7);}
     ftag=dc42_read_sector_tags(F,8);   if (ftag[4]!=0xaa && ftag[5]!=0x00) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(8);}
     ftag=dc42_read_sector_tags(F,9);   if (ftag[4]!=0xaa && ftag[5]!=0xaa) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(9);}
     
     fsec=dc42_read_sector_data(F,526); if (fsec[0]!=0x65 || fsec[1]!=0x73) {fprintf(stderr,wrong_disk); dc42_close_image(F); exit(11);}

     astart=(fsec[0x174]<<24)|(fsec[0x175]<<16)|(fsec[0x176]<<8)|(fsec[0x177]);
     a_size=(fsec[0x178]<<24)|(fsec[0x179]<<16)|(fsec[0x17a]<<8)|(fsec[0x17b]);
     bstart=(fsec[0x17c]<<24)|(fsec[0x17d]<<16)|(fsec[0x17e]<<8)|(fsec[0x17f]);
     b_size=(fsec[0x180]<<24)|(fsec[0x181]<<16)|(fsec[0x182]<<8)|(fsec[0x183]);
     cstart=(fsec[0x184]<<24)|(fsec[0x185]<<16)|(fsec[0x186]<<8)|(fsec[0x187]);
     c_size=(fsec[0x188]<<24)|(fsec[0x189]<<16)|(fsec[0x18a]<<8)|(fsec[0x18b]);
     dstart=(fsec[0x18c]<<24)|(fsec[0x18d]<<16)|(fsec[0x18e]<<8)|(fsec[0x18f]);
     d_size=(fsec[0x190]<<24)|(fsec[0x191]<<16)|(fsec[0x192]<<8)|(fsec[0x193]);
     estart=(fsec[0x194]<<24)|(fsec[0x195]<<16)|(fsec[0x196]<<8)|(fsec[0x197]);
     e_size=(fsec[0x198]<<24)|(fsec[0x199]<<16)|(fsec[0x19a]<<8)|(fsec[0x19b]);
     fstart=(fsec[0x19c]<<24)|(fsec[0x19d]<<16)|(fsec[0x19e]<<8)|(fsec[0x19f]);
     f_size=(fsec[0x1a0]<<24)|(fsec[0x1a1]<<16)|(fsec[0x1a2]<<8)|(fsec[0x1a3]);
     gstart=(fsec[0x1a4]<<24)|(fsec[0x1a5]<<16)|(fsec[0x1a6]<<8)|(fsec[0x1a7]);
     g_size=(fsec[0x1a8]<<24)|(fsec[0x1a9]<<16)|(fsec[0x1aa]<<8)|(fsec[0x1ab]);
     hstart=(fsec[0x1ac]<<24)|(fsec[0x1ad]<<16)|(fsec[0x1ae]<<8)|(fsec[0x1af]);
     h_size=(fsec[0x1b0]<<24)|(fsec[0x1b1]<<16)|(fsec[0x1b2]<<8)|(fsec[0x1b3]);

     memcpy(sec,fsec,512);
     printf("======= Current ProFile Partition Table ==============\n");
     printpart(sec);
}


void help(void) {
      puts("");
      puts("This tool allows you to use a modern ProFile replacement drive such as IDEFILE,");
      puts("X/ProFile, Aphid, etc. with much larger disks than the usual 5/10MB limits that");
      puts("are hard coded in the UniPlus ProFile driver in the kernel.                    ");

      puts("");
      puts("Usage:");
      puts(" view tables:");
      puts("              uniplus-set-profile-size boot-a.dc42\n");
      puts(" reset to default: (all other options are ignored)");
      puts("              uniplus-set-profile-size boot-a.dc42 reset\n");
      puts(" set specific size (20M...512MB default root=all/swap=2M):");
      puts("              uniplus-set-profile-size boot-a.dc42 disk=512MB\n");
      puts(" set specific size with root and swap (left over goes to partition d):");
      puts("              uniplus-set-profile-size boot-a.dc42 disk=512MB swap=2M root=16955");
      puts("              uniplus-set-profile-size boot-a.dc42 disk=512MB swap=4000 root=all");
      puts("              uniplus-set-profile-size boot-a.dc42 disk=512MB swap=4000 root=2M write=yes\n");
      puts(" suffixes for sizes: K=KB, M=MB, 512-byte blocks for no suffix.");
      puts(" if you add write=yes, it will save changes without asking, write=no will not write.");
      puts(" Using root=all will avoid allocating space to partition d, swap will still be added.\n");
      puts(" DANGER: Ensure that your storage device supports the size you specify!");
      puts(" If you plan to add external ProFile drives, note that they must match this size.");

}

uint32 parse_blocks(char *v) {
  uint32 b=0;
  int i=0;

  if (strncasecmp(v,"all",3)==0) return 0xffffffff;
  if (strncasecmp(v,"yes",3)==0) return 0xfffffffe;
  if (strncasecmp(v,"no", 2)==0)  return 0xfffffffd;

  while (v[i]) {
     if (v[i]>='0' && v[i]<='9')                                    {b=b  *10  + (v[i]-'0'); i++; continue;}

     if (tolower(v[i])=='k' && v[i+1]==0)                           {b=b   *2; break;} // 1KB=2 512 byte blocks
     if (tolower(v[i])=='k' && tolower(v[i+1])=='b' && v[i+2]==0)   {b=b   *2; break;} // 1KB=2 512 byte blocks

     if (tolower(v[i])=='m' && v[i+1]==0)                           {b=b*2048; break;} // 1MB=2048 512 byte blocks
     if (tolower(v[i])=='m' && tolower(v[i+1])=='b' && v[i+2]==0)   {b=b*2048; break;} // 1MB=2048 512 byte blocks

     // for future reuse of this code.
     //if (tolower(v[i])=='g' && v[i+1]==0)                           {b=b*2048*1024; break;} // 1MB=2048 512 byte blocks
     //if (tolower(v[i])=='g' && tolower(v[i+1])=='b' && v[i+2]==0)   {b=b*2048*1024; break;} // 1MB=2048 512 byte blocks

     help(); fprintf(stderr,"I'll take 8! Sorry, no idea what this value is: %s got %ld for c=%c",v,(long) b,v[i]); exit(6);
  }

  return b;
}

void parse_arg(int argc, char *argv[]) {

  char *equal=NULL;
  char *syntax="Queso fromage! Didn't understand this parameter you passed, did you forget the '=' sign?: %s\n";
  char *syntax2="Hoc mihi omnium Graeca! Didn't understand this parameter you passed, why did you add the '=' sign?: %s\n";

  for (int i=2; i<argc; i++) {

      char *v=argv[i];

      if (strcasestr(v,"reset")!=NULL) {
          if (argc!=3) {fprintf(stderr,"Nope, nope, nope! Can't use any other arguements with 'reset'\n"); exit(2);}
          equal=strchr(v,'='); if (equal!=NULL) {help(); fprintf(stderr,syntax2,v); exit(1);}
          reset_to_default();
          return; // ignore all other options.
      }
    
      if ( (strcasestr(v,"disk=")!=NULL) || (strcasestr(v,"size=")!=NULL) ) {
         equal=strchr(v,'='); if (!equal) {help(); fprintf(stderr,syntax,v); exit(1);}
         equal++;
         disk=parse_blocks(equal);
         if (disk<10241) {
               fprintf(stderr,"\nWarning: disk size specified is 10MB or less, resetting to default table.\n");
               reset_to_default();
             }
         continue;
      }
    
      if (strcasestr(v,"root=")!=NULL) {
         equal=strchr(v,'='); if (!equal) {help(); fprintf(stderr,syntax,v); exit(1);}
         equal++;
         root=parse_blocks(equal);
         continue;    
      }
    
      if (strcasestr(v,"swap=")!=NULL) {
         equal=strchr(v,'=');
         equal++;
         swap=parse_blocks(equal);
         if (swap==0xffffffff) {fprintf(stderr,"Invalid size for swap.\n"); exit(7);}
         continue;
      }

      if (strcasestr(v,"write=")!=NULL) {
         equal=strchr(v,'=');
         equal++;
         save=parse_blocks(equal);
         if (save!=0xfffffffe && save!=0xfffffffd) {fprintf(stderr,"Well make up your mind, should I write it? yes or no?\n"); exit(7);}
         continue;
      }


      help();
      fprintf(stderr,"Woe to you, oh Mac and Windows, for Ray Sends the LisaEm with wrath!\nNo idea what %s means\n",v);
      exit(5);
  }

}

int main(int argc, char *argv[])
{
  int i,j;

  DC42ImageType  F;

  puts("  ---------------------------------------------------------------------------");
  puts("    UniPlus Set ProFile Size v0.0.1                  http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) 2021, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");

  if (argc<=1) {  help(); exit(0); }

  int ret=0;
  printf("\nopening: %-50s\n",argv[1]);

  ret=dc42_auto_open(&F, argv[1], "wb"); if (ret) {fprintf(stderr,"Cannot open %s because %s\n",argv[1],F.errormsg); exit(99);}

  start(&F);

  if (argc>2) {
      parse_arg(argc, argv);
      set_to_size(disk);
      writepart(&F,sec);
  }

  dc42_close_image(&F);

  return 0;
}

