/**************************************************************************************\
*                                                                                      *
*                    dc42-to-raw - Dumps sectors and tags to files                     *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>


int main(int argc, char *argv[])
{
  int i,j,err,stat;
  unsigned int x;
  DC42ImageType  F;
  char *Image=NULL;
  uint8 *data, *tags;

  int do_patch=15;
  FILE *raw, *tag;
  char rawout[8192]; // filename buffers
  char rawtag[8192];

  for (i=1; i<argc; i++)
   {
     if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0 )
        {
            printf("Sector Dumper.\n\n"
                //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
                //  0         1         2         3         4         5         6         7         8
                   "Usage: dc42-to-raw Disk_Image.dc42\n\n"

                   "This utility is used to dump the data and tags to two separate files.\n"
                   "Does the opposite of raw2dc42.\n"
            );

            exit(0);
        }
     else
        {
            if (!Image) Image=argv[i];
            else {fprintf(stderr,"Can only specify one filename.  Surround it by quotes if it contains spaces.\n"); exit(2);}
        }
   }

  if (!Image) {fprintf(stderr,"I need a file name.  Run again with -h for help.\n"); exit(5);}

  snprintf(rawout,8192,"%s.raw",Image);
  snprintf(rawtag,8192,"%s.tags",Image);

  raw=fopen(rawout,"wb");
  tag=fopen(rawtag,"wb");
  if (!raw) {fprintf(stderr,"Could not create %s\n",rawout); perror(""); dc42_close_image(&F); exit(1);}
  if (!tag) {fprintf(stderr,"Could not create %s\n",rawtag); perror(""); dc42_close_image(&F); exit(1);}

  err=dc42_auto_open(&F,Image,"w");
  if (err)
     {
          fprintf(stderr,"could not open: '%s' because:%s",Image,F.errormsg);
          exit(1);
     }

  for (x=0; x<F.numblocks; x++)
  {
    data=dc42_read_sector_data(&F,x);
    if (!data) {fprintf(stderr,"Could not read data for sector %d from image!\n",x); dc42_close_image(&F);exit(2);}
    fwrite(data,F.sectorsize,1,raw);
  }

 if (F.tagsize)
  {
  for (x=0; x<F.numblocks; x++)
  {
    tags=dc42_read_sector_tags(&F,x);
    if (!tags) {fprintf(stderr,"Could not read tags for sector %d from image!\n",(int)x); dc42_close_image(&F);exit(2);}
    fwrite(tags,F.tagsize,1,tag);
  }
 }

 dc42_close_image(&F);
 fclose(raw);
 fclose(tag);
 return 0;
}
