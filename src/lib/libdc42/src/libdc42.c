/**************************************************************************************\
*                                     LibDC42                                          *
*                                                                                      *
*                            Version 0.9.7  2007.08.21                                 *
*                                                                                      *
*                       A Part of the Lisa Emulator Project                            *
*                                                                                      *
*                  Copyright (C) 1998, 2007 Ray A. Arachelian                          *
*                                All Rights Reserved                                   *
*                                                                                      *
*           This program is free software; you can redistribute it and/or              *
*           modify it under the terms of the GNU General Public License                *
*           as published by the Free Software Foundation; either version 2             *
*           of the License, or (at your option) any later version.                     *
*                                                                                      *
*           This program is distributed in the hope that it will be useful,            *
*           but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*           MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*           GNU General Public License for more details.                               *
*                                                                                      *
*           You should have received a copy of the GNU General Public License          *
*           along with this program;  if not, write to the Free Software               *
*           Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
*           Contact me if you need other licensing options, etc.                       *
*                                                                                      *
*           LICENSING NOTE: Portions of this code rely on the LZHUFF.C code            *
*                           released to usenet under the following conditions:         *
*                                                                                      *
*                                                                                      *
*       LZHUF.C (c)1989 by Haruyasu Yoshizaki, Haruhiko Okumura, and Kenji Rikitake.   *
*            All rights reserved. Permission granted for non-commercial use.           *
*                                                                                      *
*       If your application does not meet the non-commercial use clause, you must      *
*       remove the #define USE_LZHUFF line from this library, and you may not use      *
*       the LZHUFF routines, or you'll have to build your own equivalents.  Sorry.     *
* ------------------------------------------------------------------------------------ *
*                                                                                      *
*        Routines for access and manipulation of Disk Copy 4.2 Disk Images.            *
*                                                                                      *
* This code is unaffiliated with Apple, and Apple retains all trade marks for itself,  *
* and all versions of the Disk Copy and DART programs.  Whenever you see the text      *
* Disk Copy or DART, you are encouraged to visualize a trademark symbol following it,  *
* and chant We all we all worship Steve Jobs" (just kidding!)                          *
*                                                                                      *
\**************************************************************************************/


// The following define enables LZH decompression.  It's needed in order to open compressed DART
// files.  The code that enables this is not GPL, although it has been released in 1989
// as free for non-commercial use on usenet.  See the comments at the bottom of this file for details.
//
// Remove this if your code needs pure GPL compatibility, or is of a commercial nature.
//
#define USE_LZHUF  1


// needed for LisaEm compatibility, you can remove this, but you must
// define int8, int16, int32, uint8, uint16, uint32.
#include <machine.h>


// normal includes
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// Windows lacks MMAP, strcasestr functions
#ifndef __MSVCRT__
#include <sys/mman.h>
#include <glob.h>
#define HAVE_MMAPEDIO
#else
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#include <strcasestr.h>
#endif



#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>


////////////// headers ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef fgetc
#define fgetc(xx) ((unsigned)(getc(xx)))
#endif

#define DC42_HEADERSIZE 84
#define DART_CHUNK      20960
#define DART_CHUNK_TAGS 20480

#define TWIG860KFLOPPY 0
#define SONY800KFLOPPY 2
#define SONY400KFLOPPY 1



typedef struct                          // floppy type
{
  int    fd;                            // file descriptor (valid if >2, invalid if negative 0=stdin, 1=stdout, invalid too)
  FILE  *fh;                            // file handle on Win32


  uint32 size;                          // size in bytes of the disk image file on disk (i.e. the dc42 file itself incl. headers.)
                                        // used for mmap fn's

  char   fname[FILENAME_MAX+2];         // File name of this disk image (needed for future use to re-create/re-open the image)
                                        // FILENAME_MAX should be set by your OS via includes such as stdio.h, if not, you'll need
                                        // to set this to whatever is reasonable for your OS.  ie. 256 or 1024, etc.
                                        //
                                        // Also can be used to automatically copy a r/o master disk to a private r/w copy - not yet implemented
                                        // copy made on open.

  char  mfname[FILENAME_MAX+2];         // r/o master filename path - not yet implemented.


  uint8  readonly;                      // 0 read/write image - writes are sync'ed to disk image and checksums recalculated on close
                                        // 1 read only flag - writes return error and not saved, not even in RAM
                                        // 2 if writes saved in RAM, but not saved to disk when image closed

  uint8  synconwrite;                   // sync writes to disk immediately, but note that this is very very slow!
                                        // only used for mmap'ed I/O

  uint8  mmappedio;                     // 0 if disabled, 1 if enabled.

  uint8  ftype;                         // floppy type 0=twig, 1=sony400k, 2=sony800k, 3=freeform, 254/255=disabled


  uint32 tagsize;                       // must set to 12 - expect ProFile/Widget to use 24 byte tags
  uint32 datasize;                      // must set to 512

  uint32 datasizetotal;                 // data size (in bytes of all sectors added together)
  uint32 tagsizetotal;                  // tag size total in bytes

  uint32 sectoroffset;                  // how far into the file is the 1st sector
  uint16 sectorsize;                    // must set to 512  (Twiggies might be 256 bytes/sector, but unknown)
  uint32 tagstart;                      // how far into the file to 1st tag - similar to sectoroffset

  uint32 maxtrk, maxsec,maxside, numblocks;  // unused by these routines, but used by the Lisa Emulator

  uint8 *RAM;                           // memory mapped file pointer - or a single sector's tags + data

  long  dc42seekstart;                  // when opening an existing fd, points to start of dc42 image

  char returnmsg[256];                  // error message buffer - used internally for storage, instead, access error via errormsg
  char *errormsg;                       // pointer to error message, use this to read text of error returned.
  int retval;                           // error number of last operation

} DC42ImageType;




int dc42_open(DC42ImageType *F, char *filename, char *options);                    // open a disk image, map it and fill structure

int dc42_open_by_handle(DC42ImageType *F, int fd, FILE *fh, long seekstart, char *options);
                                                                                   // open an embedded dc42 image in an already
                                                                                   // opened file descriptor at the curren file
                                                                                   // position.


int dc42_close_image(DC42ImageType *F);                                            // close the image: fix checksums and sync data
int dc42_close_image_by_handle(DC42ImageType *F);                                  // close, but don't call close on the fd.

int dc42_create(char *filename,char *volname, uint32 datasize,uint32 tagsize);     // create a blank new disk image
                                                                                   // does not open the image, may not be called
                                                                                   // while the image file is open.

int dc42_add_tags(char *filename, uint32 tagsize);                                 // add tags to a dc42 image that lacks them.
                                                                                   // if tagsize is zero adds 12 bytes of tags for
                                                                                   // every 512 bytes of data.  Does not open the
                                                                                   // image, can be used pre-emptively when opening
                                                                                   // and image for access.  Call it with 0 as the tag
                                                                                   // size before calling dc42 open to ensure it has tags.
                                                                                   // does not open the image, may not be called
                                                                                   // while theimage file is open.

uint8 *dc42_read_sector_tags(DC42ImageType *F, uint32 sectornumber);               // read a sector's tag data
uint8 *dc42_read_sector_data(DC42ImageType *F, uint32 sectornumber);               // read a sector's data
int dc42_write_sector_tags(DC42ImageType *F, uint32 sectornumber, uint8 *tagdata); // write tag data to a sector
int dc42_write_sector_data(DC42ImageType *F, uint32 sectornumber, uint8 *data);    // write sector data to a sector

int dc42_sync_to_disk(DC42ImageType *F);                                           // like fsync, sync's writes back to file. Does
                                                                                   // NOT write proper tag/data checksums, as that
                                                                                   // would be too slow.  Call recalc_checksums yourself
                                                                                   // when you need it, or call dc42_close_image.

uint32 dc42_has_tags(DC42ImageType *F);                                            // returns 0 if no tags, 1 if it has tags

uint32 dc42_calc_tag_checksum(DC42ImageType *F);                                   // calculate the current tag checksum

uint32 dc42_calc_data_checksum(DC42ImageType *F);                                  // calculate the current sector data checksum

int dc42_recalc_checksums(DC42ImageType *F);                                       // calculate checksums and save'em in the image


int dart_to_dc42(char *dartfilename, char *dc42filename);                          // converts a DART fast-compressed/uncompressed
                                                                                   // image to a DiskCopy42 image.  Does
                                                                                   // work with LZH compressed images

int dc42_is_valid_image(char *filename);                                           // returns 0 if it can't open the image, or the
                                                                                   // image is not a valid dc42 image.  Returns 1
                                                                                   // if valid, or 2 if mac binary II enclosed but valid.

int dart_is_valid_image(char *dartfilename);                                       // returns 0 if it can't open the image, or
                                                                                   // the image is not a valid DART image. Returns 1
                                                                                   // if valid, or 2 if valid but macbinII encapsulated.

int dc42_is_valid_macbinii(char *infilename, char *creatortype);                   // returns 1 if file is macbinII encapsulated
                                                                                   // if creatortype is passed a non-NULL ponter
                                                                                   // the Macintosh creator and type are returned
                                                                                   // by thefunction.  This must be at leat 9 bytes!


int dc42_extract_macbinii(char *infilename);                                       // file name is overwritten by new file name!
                                                                                   // will return 1 if converted,
                                                                                   // 0 if not converted,
                                                                                   // -1 can't read file,
                                                                                   // -2 if out of memory - filename clobbered
                                                                                   // -3 if unable to create new file - filename clobbered


int dc42_set_volname(DC42ImageType *F,char *name); // set/get the disk image volume name
char *dc42_get_volname(DC42ImageType *F);

int searchseccount( DC42ImageType *F, int sector, int size, uint8 *s);
int replacesec(DC42ImageType *F, int sector, int size, uint8 *s, uint8 *r);


////////////// headers ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

char *DiskCopy42Sig="\026-not a Macintosh disk-";
char *NotAMacintoshDisk="-not a Macintosh disk-";
                    //   0123456789012345678901

static char copyleft[]=
"libdc42 0.8 2007.01.05 - A Part of the Lisa Emulator Project. see: http://lisaem.sunder.net/lisafsh/   \
Copyright (C) 2007 by Ray A. Arachelian, All Rights Reserved.  \
Released under the terms of the GPL2 or the LGPL 2.1 - licenses your choice. \
see http://www.gnu.org/licenses/gpl.txt or http://www.gnu.org/licenses/lgpl.txt for details";

// Note, you will get compiler warnings from some of these macros, this can't be (easily) helped.
// the ret value here sets the type to return, if ret=0 or ret>4, there will be no actual return
// from the function.  This code isn't super efficient, however, the real purpose of these macros
// isn't speed, but rather to keep the source code from being flodded with a ton of error checking
// code.  Instead, they help beautify the code and make it easier to read, thus, easier to debug.
//
// That said, I do recognize that there are lots of limitations to macros, and there I have avoided
// those, but they still exist.  Macros can be bad practice, however, in this case, I believe they
// are very helpful, AND make life a lot easier.  Inline code would not have worked, because a return
// from inside an inline function returns to the caller, I want to use these to exit the function
// without replicating a lot of code.  I believe copying and pasting code needlessly is a far worse
// evil than the judicious use of macros.  Also, since they are macros, I do hope that a smart
// C compiler should see that ret is passed as a constant, should optimize the unused cases out of the
// generated code, so it should be ok.
//
// The warning you'll see is: "Warning: return makes pointer from integer without a cast"
//
#define DC42_RET_CODE(F,code,msg,ret) { if (F) {F->retval=code; F->errormsg=msg; ret;}  }


uint32 dc42_ror32(uint32 data)   {uint32 carry=(data & 1)<<31; return (data>>1)|carry; }

#define DC42_CHECK_VALID_F(F)      { if (!F)       return -1;   \
                                     if (!F->RAM) {DC42_RET_CODE(F,-3,"Disk image closed or no memory allocated to it",return F->retval);}\
                                     if (F->fd<3 && !F->fh) {DC42_RET_CODE(F,-5,"Invalid file descriptor",return F->retval);}                       \
                                   }

#define DC42_CHECK_VALID_F_NR(F)   {if (!F)                                                                                return;       \
                                    if (!F->RAM) DC42_RET_CODE(F,-3,"Disk image closed or no memory allocated to it",      return);      \
                                    if (F->fd<3 && !F->fh) DC42_RET_CODE(F,-5,"Invalid file descriptor",return);                                   \
                                   }

#define DC42_CHECK_VALID_F_NUL(F)  {if (!F)                                                                                return NULL;  \
                                    if (!F->RAM) DC42_RET_CODE(F,-3,"Disk image closed or no memory allocated to it",      return NULL); \
                                    if (F->fd<3 && !F->fh) DC42_RET_CODE(F,-5,"Invalid file descriptor",return NULL);                              \
                                   }

#define DC42_CHECK_VALID_F_ZERO(F) {if (!F)                                                                                return 0;     \
                                    if (!F->RAM) DC42_RET_CODE(F,-3,"Disk image closed or no memory allocated to it",      return 0);    \
                                    if (F->fd<3 && !F->fh) DC42_RET_CODE(F,-5,"Invalid file descriptor",                             return 0);    \
                                   }


#define DC42_CHECK_WRITEABLE(F)    {if (F->readonly)    DC42_RET_CODE(F,-8,"Image is Read Only",                           return F->retval);}
#define DC42_CHECK_WRITEABLE1(F)   {if (F->readonly==1) DC42_RET_CODE(F,-8,"Image is Read Only",                           return F->retval);}


uint32 dc42_get_datachecksum(DC42ImageType *F)
{

  DC42_CHECK_VALID_F(F)

  if (!F->mmappedio) {
                       if ( F->fd>2)  {
			                int i;
                                        lseek(F->fd,72,SEEK_SET);
                                        i=read(F->fd,&F->RAM[72],(79-72) );
                                      }
                       if ( F->fh  )  {
			                int i;
			                fseek(F->fh,72,SEEK_SET);
                                        i=fread(&F->RAM[72],(79-72),1,F->fh);
                                      }
                     }



  return  (F->RAM[72]<<24) |
          (F->RAM[73]<<16) |
          (F->RAM[74]<< 8) |
          (F->RAM[75]    ) ;
}

uint32 dc42_get_tagchecksum(DC42ImageType *F)
{
  DC42_CHECK_VALID_F(F)

  if (!F->mmappedio) {
                       if ( F->fd>2)  {
			                int i;
                                        lseek(F->fd,72,SEEK_SET);
                                        i=read(F->fd,&F->RAM[72],(79-72) );
                                      }
                       if ( F->fh  )  {
			                int i;
				        fseek(F->fh,72,SEEK_SET);
                                        i=fread(&F->RAM[72],(79-72),1,F->fh);
                                      }
                     }


  return  (F->RAM[76]<<24) |
          (F->RAM[77]<<16) |
          (F->RAM[78]<< 8) |
          (F->RAM[79]    );
}


int dc42_recalc_checksums(DC42ImageType *F)
{
  uint32 newtagchks=0;
  uint32 newdatachks=0;


  DC42_CHECK_VALID_F(F)


  newtagchks =dc42_calc_tag_checksum(F);                                   // calculate the current tag checksum
  newdatachks=dc42_calc_data_checksum(F);                                  // calculate the current sector data checksum

  F->RAM[72]=((newdatachks>>24) & 0xff);                                   // save data checksum back to image
  F->RAM[73]=((newdatachks>>16) & 0xff);                                   // the image can now be resync'ed to the
  F->RAM[74]=((newdatachks>> 8) & 0xff);                                   // file system.
  F->RAM[75]=((newdatachks    ) & 0xff);

  F->RAM[76]=(( newtagchks>>24) & 0xff);
  F->RAM[77]=(( newtagchks>>16) & 0xff);
  F->RAM[78]=(( newtagchks>> 8) & 0xff);
  F->RAM[79]=(( newtagchks    ) & 0xff);

  if (!F->mmappedio) {
                       if ( F->fd>2)  {
                                        int i;
                                        lseek(F->fd,72,SEEK_SET);
                                        i=write(F->fd,&F->RAM[72],(79-72) );
                                      }
                       if ( F->fh  )  {
                                        int i;
                                        fseek(F->fh,72,SEEK_SET);
                                        i=fwrite(&F->RAM[72],(79-72),1,F->fh);
                                      }
                     }
  return 0;
}


int dc42_check_checksums(DC42ImageType *F)                                  // 0 if they match, 1 if tags, 2 if data, 3 if both don't match
{
  uint32 newtagchks=0;
  uint32 newdatachks=0;
  uint32 datachks;
  uint32 tagchks;
  DC42_CHECK_VALID_F(F)

  newtagchks =dc42_calc_tag_checksum(F);                                   // calculate the current tag checksum
  newdatachks=dc42_calc_data_checksum(F);                                  // calculate the current sector data checksum

  datachks=
  ((F->RAM[72]<<24 )|                                                      // save data checksum back to image
   (F->RAM[73]<<16 )|                                                      // the image can now be resync'ed to the
   (F->RAM[74]<<8  )|                                                      // file system.
   (F->RAM[75]     ));

  tagchks=
  ((F->RAM[76]<<24) |
   (F->RAM[77]<<16) |
   (F->RAM[78]<<8 ) |
   (F->RAM[79]    ) );


  return ((datachks!=newdatachks) ? 2:0) | ((tagchks!=newtagchks) ?1:0 );
}


int dc42_sync_to_disk(DC42ImageType *F)                                    // recalc checksums and synchronize in-memory
{                                                                          // disk image back to the disk.

  DC42_CHECK_VALID_F(F);
  DC42_CHECK_WRITEABLE(F);

  //// 20061223 - gprof catches the calculate checksums call that this calls to be very expensive
  //dc42_recalc_checksums(F);
  #ifdef HAVE_MMAPEDIO
  if (F->mmappedio==1) msync(F->RAM,F->size,MS_SYNC);
  #endif

  if (F->mmappedio==2 && F->readonly==0)
                       { int i;

                        if (F->fd>2)
                           {  lseek(F->fd,F->dc42seekstart,SEEK_SET);   // locate the dc42 image inside the FD
                              i=write(F->fd,F->RAM,F->size);              // save the whole file
                           }

                        if (F->fh)
                           {  fseek(F->fh,F->dc42seekstart,SEEK_SET);   // locate the dc42 image inside the FD
                              i=fwrite(     F->RAM,F->size,1,F->fh);      // save the whole file
                           }

                       }                                                // should do this with a dirty map instead as this is too slow.


   if (F->fh) fflush(F->fh);
#ifndef __MSVCRT__
   if (F->fd) fsync( F->fd);
#else
   if (F->fd) _commit( F->fd);          // fucking microsoft!
#endif

  return 0;
}



int dc42_close_image(DC42ImageType *F)
{
   DC42_CHECK_VALID_F(F);

   dc42_recalc_checksums(F);
   dc42_sync_to_disk(F);

  #ifdef HAVE_MMAPEDIO
  if (F->mmappedio==1) munmap(F->RAM,F->size);
  else
  #endif
  if (F->RAM) free(F->RAM);

  F->RAM=NULL;                                                                // decouple file and RAM, mark RAM as invalid

  if (F->fd>2)    { close(F->fd); F->fd=-1;}                                  // close the file handle.
  if (F->fh)      {fclose(F->fh); F->fh=NULL;}

  DC42_RET_CODE(F,0,"Image Closed",return F->retval);
  return F->retval;                                                          // suppress dumb compiler warning
}


int dc42_close_image_by_handle(DC42ImageType *F)
{
   DC42_CHECK_VALID_F(F);
   dc42_sync_to_disk(F);

   #ifdef HAVE_MMAPEDIO
   if (F->mmappedio==1) munmap(F->RAM,F->size);
   else
   #endif
   free(F->RAM);

   F->RAM=NULL;                                                                // decouple file and RAM, mark RAM as invalid

   if (F->fd>2)     F->fd=-1;                                                  // close the image, but not the file.
   if (F->fh)       F->fh=NULL;

   DC42_RET_CODE(F,0,"Image Closed",return F->retval);
   return F->retval;                   // suppress dumb compiler warning
}

// For file seeks
#define GET_TAG_POS(x)  (F->dc42seekstart+ ( (x)*F->tagsize   ) + F->tagstart    )
#define GET_DATA_POS(x) (F->dc42seekstart+ ( (x)*F->sectorsize) + F->sectoroffset)

// For in memory seeks
#define GET_TAG_IDX(x)  (                  ( (x)*F->tagsize   ) + F->tagstart    )
#define GET_DATA_IDX(x) (                  ( (x)*F->sectorsize) + F->sectoroffset)



uint8 *dc42_read_sector_tags(DC42ImageType *F, uint32 sectornumber)
{
   DC42_CHECK_VALID_F_NUL(F);
   F->retval=0;
   F->errormsg=F->returnmsg;
   *F->returnmsg=0;



   if (F->numblocks==0) F->numblocks=  (F->datasizetotal/F->sectorsize);
   if (sectornumber   > F->numblocks ) { DC42_RET_CODE(F,999,"invalid sector #",return NULL);}

   if (F->mmappedio==0)
      {  int i;
         if (F->fd>2)
            {
             lseek(F->fd,GET_TAG_POS(sectornumber),SEEK_SET);
             i= read(F->fd,&F->RAM[F->datasize],F->tagsize);         // put tags at the end of the buffer so we can overlap reads of tags

              //fprintf(stderr,"fd-read tag %4d at loc:%08x PTR:%p\n",sectornumber,GET_TAG_POS(sectornumber),F);

            }
         if (F->fh)
            {
             fseek(F->fh,GET_TAG_POS(sectornumber),SEEK_SET);
             i=fread(      &F->RAM[F->datasize],F->tagsize,1,F->fh); // put tags at the end of the buffer so we can overlap reads of tags

              //fprintf(stderr,"fh-read tag %4d at loc:%08x PTR:%p\n",sectornumber,GET_TAG_POS(sectornumber),F);

            }

         return &F->RAM[F->datasize];                         // with reads of sectors.
      }

   // otherwise it's either mmapped or fully in RAM, either way, access to them is the same.
   //fprintf(stderr,"mem-read tag %4d at loc:%08x PTR:%p\n",sectornumber,GET_TAG_IDX(sectornumber),F);

   return &F->RAM[ GET_TAG_IDX(sectornumber) ];
}


uint8 *dc42_read_sector_data(DC42ImageType *F, uint32 sectornumber)
{
   DC42_CHECK_VALID_F_NUL(F)
   F->retval=0;
   F->errormsg=F->returnmsg;
   *F->returnmsg=0;


   if (F->numblocks==0) F->numblocks=  (F->datasizetotal/F->sectorsize);
   if (sectornumber   > F->numblocks ) { DC42_RET_CODE(F,999,"invalid sector #",return NULL);}


   if (!F->mmappedio)
      { int i;
        if (F->fd>2)
         {
          lseek(F->fd,GET_DATA_POS(sectornumber),SEEK_SET);
          i= read(F->fd,F->RAM,F->sectorsize);

           //fprintf(stderr,"fd-read data %4d at loc:%08x PTR:%p\n",sectornumber,GET_DATA_POS(sectornumber),F);

         }

        if (F->fh)
         {
          fseek(F->fh,GET_DATA_POS(sectornumber),SEEK_SET);
          i=fread(      F->RAM,F->sectorsize,1,F->fh);

          //fprintf(stderr,"fh-read data %4d at loc:%08x PTR:%p\n",sectornumber,GET_DATA_POS(sectornumber),F);
         }

        return F->RAM;
      }

//   fprintf(stderr,"Returning code:%d string:%s\n",F->retval,F->errormsg); fflush(stderr);
//   fprintf(stderr,"mem-read data %4d at loc:%08x PTR:%p\n",sectornumber,GET_DATA_IDX(sectornumber),F);
   return &F->RAM[ GET_DATA_IDX(sectornumber) ];
}


int dc42_write_sector_data(DC42ImageType *F, uint32 sectornumber, uint8 *data)
{
   DC42_CHECK_VALID_F(F);
   DC42_CHECK_WRITEABLE1(F);
   F->retval=0;
   F->errormsg=F->returnmsg;
   *F->returnmsg=0;

   if (F->numblocks==0) F->numblocks=  (F->datasizetotal/F->sectorsize);
   if (sectornumber   > F->numblocks ) { DC42_RET_CODE(F,999,"invalid sector #",return 999);}

   if (!F->mmappedio)
      {  int i;
         //{fprintf(stderr,"libdc42.c::wrote DIRECT! to block#%ld, returning:%ld\n",sectornumber, F->retval); fflush(stderr);}
         if (F->fd)
         {
            lseek(F->fd,GET_DATA_POS(sectornumber),SEEK_SET);
            i=write(F->fd,data,F->sectorsize);

            //fprintf(stderr,"fd-write data %4d at loc:%08x PTR:%p\n",sectornumber,GET_DATA_POS(sectornumber),F);
         }
         if (F->fh)
         {
            fseek(F->fh,GET_DATA_POS(sectornumber),SEEK_SET);
            i=fwrite(    data,F->sectorsize,1,F->fh);
            //fprintf(stderr,"fh-write data %4d at loc:%08x PTR:%p\n",sectornumber,GET_DATA_POS(sectornumber),F);
         }

         DC42_RET_CODE(F,0,"Sector Written",return F->retval);
      }

   //fprintf(stderr,"mem-write data %4d at loc:%08x  PTR:%p\n",sectornumber,GET_DATA_IDX(sectornumber),F);
   memcpy( &F->RAM[GET_DATA_IDX(sectornumber)], data ,F->sectorsize);

   if (F->synconwrite && F->readonly==0 ) dc42_sync_to_disk(F);

   //{fprintf(stderr,"libdc42.c::wrote to block#%ld, returning:%ld\n",sectornumber, F->retval); fflush(stderr);}

   DC42_RET_CODE(F,0,"Sector Written",return F->retval);
   return F->retval;                                          // suppress dumb compiler warning, even though we never reach here
}


int dc42_write_sector_tags(DC42ImageType *F, uint32 sectornumber, uint8 *tagdata)
{
  DC42_CHECK_VALID_F(F);
  DC42_CHECK_WRITEABLE1(F);
  F->retval=0;
  F->errormsg=F->returnmsg;
  *F->returnmsg=0;
  if (F->numblocks==0) F->numblocks=  (F->datasizetotal/F->sectorsize);
  if (sectornumber   > F->numblocks ) { DC42_RET_CODE(F,999,"invalid sector #",return 999);}

  if (!F->mmappedio)
     {
         if (F->fd>2)
            {int i;
             lseek(F->fd,GET_TAG_POS(sectornumber),SEEK_SET);
             i=write(F->fd,tagdata,F->tagsize);
             //fprintf(stderr,"fd-write tag %4d at loc:%08x PTR:%p\n",sectornumber,GET_TAG_POS(sectornumber),F);
            }

         if (F->fh)
            {int i;
             fseek(F->fh,GET_TAG_POS(sectornumber),SEEK_SET);
             i=fwrite(     tagdata,F->tagsize,1,F->fh);
             //fprintf(stderr,"fh-write tag %4d at loc:%08x PTR:%p\n",sectornumber,GET_TAG_POS(sectornumber),F);
            }

         return 0;
     }

  //fprintf(stderr,"mem-write tag %4d at loc:%08x PTR:%p\n",sectornumber,GET_TAG_IDX(sectornumber),F);
  if (F->tagsize>4 && F->tagsizetotal>0)                        // write tag data to the image, if it originally had tags
     memcpy( &F->RAM[ GET_TAG_IDX(sectornumber) ],  tagdata, F->tagsize);
  else {
        //{fprintf(stderr,"libdc42.c::FAILED TO WRITE %d tag bytes to block#%ld, returning:%d\n",F->tagsize, sectornumber, -4); fflush(stderr);}
                    return -4;
       }

  if (F->synconwrite && F->readonly==0 ) dc42_sync_to_disk(F);
  //{fprintf(stderr,"libdc42.c::wrote %d tag bytes to block#%ld, returning:%ld\n",F->tagsize, sectornumber, F->retval); fflush(stderr);}
  DC42_RET_CODE(F,0,"Sector Tag Written",return F->retval);
  return F->retval;                   // suppress dumb compiler warning
}


uint32 dc42_calc_data_checksum(DC42ImageType *F)
{
    uint32 i,j,mydatachks=0;
    uint8 *d;

    DC42_CHECK_VALID_F_ZERO(F);

    for (i=0; i<F->numblocks; i++)
    {
       d=dc42_read_sector_data(F,i);
       for (j=0; j<F->sectorsize ; j+=2)
           mydatachks=dc42_ror32(mydatachks+(uint32)(d[j]<<8) + (uint32)(d[j+1]));
    }

    DC42_RET_CODE(F,mydatachks,"Data Checksum returned",return F->retval);
    return F->retval;                   // suppress dumb compiler warning
}

uint32 dc42_calc_tag_checksum(DC42ImageType *F)
{
    uint32 i,j,mytagchks=0;
    uint8 *t;

    DC42_CHECK_VALID_F(F);

    for (i=1; i<F->numblocks; i++)      // this is not a bug! - used by DC42
    {
     t=dc42_read_sector_tags(F,i);
     for (j=0; j<F->tagsize; j+=2)
           mytagchks=dc42_ror32(mytagchks+(uint32)((uint32)(t[j]<<8)+(uint32)(t[j+1])));
    }
    DC42_RET_CODE(F,mytagchks,"Tag Checksum returned",return F->retval);
    return F->retval;                   // suppress dumb compiler warning
}

uint32 dc42_calc_tag0_checksum(DC42ImageType *F)  // used by DART
{
    uint32 i,j,mytagchks=0;
    uint8 *t;

    DC42_CHECK_VALID_F(F);

    for (i=0; i<F->numblocks; i++)
    {
     t=dc42_read_sector_tags(F,i);
     for (j=0; j<F->tagsize; j+=2)
           mytagchks=dc42_ror32(mytagchks+(uint32)((uint32)(t[j]<<8)+(uint32)(t[j+1])));
    }
    DC42_RET_CODE(F,mytagchks,"Tag Checksum returned",return F->retval);
    return F->retval;                   // suppress dumb compiler warning
}


uint32 dc42_has_tags(DC42ImageType *F)
{
    uint32 i,j,hastags=0;
    uint8 *t;

    DC42_CHECK_VALID_F_ZERO(F);

    for (i=1; i<F->numblocks && !hastags; i++)
    {
     t=dc42_read_sector_tags(F,i);
     for (j=0; j<12 && !hastags; j++) hastags|=t[j];
    }

    DC42_RET_CODE(F,(hastags!=0),"Tag State returned",return F->retval);
    return F->retval;                   // suppress dumb compiler warning
}


// returns 0 if it can't open the image, or if it doesn't like the image.
int dc42_is_valid_image(char *filename)
{
//    FILE *handle;
    static DC42ImageType FF, *F=&FF;
    int i,flag=0;
    long filesizetotal=0;
    uint8 tempbuf[2048];

    strncpy(F->fname,filename,FILENAME_MAX);

    int is_macbin=dc42_is_valid_macbinii(filename,NULL);


    // open the image as read only and grab it's header - we re-open it as r/w later if everything's happy.

#ifndef __MSVCRT__
    F->fd=open(F->fname,O_RDONLY);
    // why 3? because 0 is stdin, 1 is stdout, 2 is stderr.  Duh!
    if (F->fd<3)  DC42_RET_CODE(F,-6,"Cannot open the file.",return F->retval);
    F->fh=NULL;
#else
    F->fh=fopen(F->fname,"rb");
    if (!F->fh)   DC42_RET_CODE(F,-6,"Cannot open the file.",return F->retval);
    F->fd=0;
#endif


    if (F->fd>2)
       {
	    uint16 extras=0;

		 if (is_macbin) 
		    {int i;
			  uint8 buf[2];
			  lseek(F->fd,120,SEEK_SET);
			  i=read(F->fd,buf,2);
			  //  Length of a secondary header. If this is non-zero, skip this many bytes (rounded up to the next multiple of 128). 
			  extras=(buf[0]<<8) | (buf[1]);
			  if (extras & 127) extras=(extras|127)+1;
		    }

         filesizetotal=lseek(F->fd,0,SEEK_END);
         filesizetotal=lseek(F->fd,0,SEEK_CUR);
                       lseek(F->fd,is_macbin ? 128+extras: 0,SEEK_SET);
		 if (is_macbin) filesizetotal-=(128+extras);

       }

    if (F->fh)
       {
		uint16 extras=0;
	
		 if (is_macbin) 
		    {
			  uint8 ch, cl;

			  //  Length of a secondary header. If this is non-zero, skip this many bytes (rounded up to the next multiple of 128). 
			  fseek(F->fh,120,SEEK_SET);  ch=fgetc(F->fh); cl=fgetc(F->fh);
			  extras=(ch<<8) | (cl);
			  if (extras & 127) extras=(extras|127)+1;
		    }

                       fseek(F->fh,0,SEEK_END);
         filesizetotal=ftell(F->fh);
                       fseek(F->fh,0,SEEK_CUR);
                       fseek(F->fh,is_macbin ? 128+extras:0,SEEK_SET);

		 if (is_macbin) filesizetotal-=(128+extras);
       }





//    if (filesizetotal>4294967290) DC42_RET_CODE(F,88,"File too large to be valid disk image",return F->retval);

    if (F->fd>2)
       {
        int i;
        i=read(F->fd,tempbuf,2048);  // read 1st 2k of image
        close(F->fd);  F->fd=0;
       }
    if (F->fh)
       {int i;
        i=fread(     tempbuf,2048,1,F->fh);  // read 1st 2k of image
        fclose(F->fh); F->fh=NULL;
       }


    for (flag=0,i=0; i<1024 && !flag; i++) flag|=tempbuf[i];
    if (!flag)                          // Disk Copy 6.3.x uncompressed image
           {DC42_RET_CODE(F,-60,"This appears to be a Disk Copy 6.x uncompressed image",return F->retval);}

    if (tempbuf[0]==0x80 && tempbuf[1]==0x00 && tempbuf[2]==0x7f)
           {DC42_RET_CODE(F,-61,"This appears to be a Disk Copy 6.x compressed image",return F->retval);}

    // make sure that this is a DC42 image
    if (tempbuf[0]>64)                      DC42_RET_CODE(F,0,"This is not a Disk Copy 4.2 image or is corrupt (DC42 volume name size >64 bytes)",return F->retval);
    if (tempbuf[82]!=1 || tempbuf[83]!=0)   DC42_RET_CODE(F,0,"This is not a Disk Copy 4.2 image or is corrupt (version is not 0100)",return F->retval);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    F->sectoroffset=DC42_HEADERSIZE;               // data starts from byte 84 and continues on in 512 byte chunx

    F->datasizetotal=(tempbuf[64+0]<<24)|(tempbuf[64+1]<<16)|(tempbuf[64+2]<<8)|tempbuf[64+3];
    F->tagsizetotal =(tempbuf[68+0]<<24)|(tempbuf[68+1]<<16)|(tempbuf[68+2]<<8)|tempbuf[68+3];

    if ((DC42_HEADERSIZE+F->datasizetotal+F->tagsizetotal-filesizetotal) > 1024)
              DC42_RET_CODE(F,0,"File size does not match headers",return F->retval);

    DC42_RET_CODE(F,1+is_macbin,"Is valid DC42 Image",return F->retval);
    return F->retval;                   // suppress dumb compiler warnings.
}




int dc42_open(DC42ImageType *F, char *filename, char *options)
{

    int i, flag;
    long filesizetotal=0;
    uint8 tempbuf[2048];
    F->dc42seekstart=0;
    F->retval=0;
    F->returnmsg[0] = 0;

    // copy the file name into the image structure for later use
    strncpy(F->fname,filename,FILENAME_MAX);

    // open the image as read only and grab it's header - we re-open it as r/w later if everything's happy.
#ifndef __MSVCRT__
    F->fd=open(F->fname,O_RDONLY);
    // why 3? because 0 is stdin, 1 is stdout, 2 is stderr.  Duh!
    if (F->fd<3)  DC42_RET_CODE(F,-6,"Cannot open the file.",return F->retval);
    F->fh=NULL;
#else
    F->fh=fopen(F->fname,"rb");
    if (!F->fh)   DC42_RET_CODE(F,-6,"Cannot open the file.",return F->retval);
    F->fd=0;
#endif

    if (F->fd>2)
       {
         filesizetotal=lseek(F->fd,0,SEEK_END);
         filesizetotal=lseek(F->fd,0,SEEK_CUR);
                       lseek(F->fd,0,SEEK_SET);
       }
    if (F->fh)
       {
                       fseek(F->fh,0,SEEK_END);
         filesizetotal=ftell(F->fh);
                       fseek(F->fh,0,SEEK_CUR);
                       fseek(F->fh,0,SEEK_SET);


       }

    F->dc42seekstart=0;

    //if (filesizetotal>4294967296) DC42_RET_CODE(F,88,"File too large to be a valid disk image",return F->retval);

    if (F->fd>2)
       { int i;
         i=read(F->fd,tempbuf,2048);  // read 1st 2k of image
         close(F->fd);  F->fd=0;
       }
    if (F->fh)
       { int i;
         i=fread(     tempbuf,2048,1,F->fh);  // read 1st 2k of image
         fclose(F->fh); F->fh=NULL;
       }

    // make sure that this is a DC42 image

    for (flag=0,i=0; i<1024 && !flag; i++) flag|=tempbuf[i];
    if (!flag)                          // Disk Copy 6.3.x uncompressed image
           {DC42_RET_CODE(F,88,"This appears to be a Disk Copy 6.x uncompressed image",return F->retval);}

    if (tempbuf[0]==0x80 && tempbuf[1]==0x00 && tempbuf[2]==0x7f)
           {DC42_RET_CODE(F,88,"This appears to be a Disk Copy 6.x compressed image",return F->retval);}


    //printf("\ntempbuf[0]=%d version:%02x%02x\n",tempbuf[0],tempbuf[82],tempbuf[83]);
    if (tempbuf[0]>64 || tempbuf[82]!=1 || tempbuf[83]!=0)   DC42_RET_CODE(F,88,"DC42 volume name size wrong, or version not 0100",return F->retval);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    F->sectoroffset=DC42_HEADERSIZE;               // data starts from byte 84 and continues on in 512 byte chunx

    F->datasizetotal=(tempbuf[64+0]<<24)|(tempbuf[64+1]<<16)|(tempbuf[64+2]<<8)|tempbuf[64+3];

    F->tagsizetotal =(tempbuf[68+0]<<24)|(tempbuf[68+1]<<16)|(tempbuf[68+2]<<8)|tempbuf[68+3];

    // be more lenient for larger file sizes
    if ((DC42_HEADERSIZE+F->datasizetotal+F->tagsizetotal-filesizetotal) > 1024)
       { filesizetotal=DC42_HEADERSIZE+F->datasizetotal+F->tagsizetotal; }

    F->datasize=512;   // we don't support other sizes
	 F->sectorsize=512;

    //F->numblocks=F->datasizetotal/F->sectorsize;   // for 400k disks=800 blocks, 800k disks=1600 - but get this from the image.  This is in bytes 1st
    F->numblocks=F->datasizetotal/F->sectorsize;
    F->tagsize  =F->tagsizetotal/F->numblocks;       // careful here since some images lack tags altogether!  This s/b 0 in that case.

    // tags start right after sector data
    F->tagstart=F->sectoroffset + (F->numblocks * F->sectorsize);

    if (F->numblocks ==  800) {  F->maxtrk=80; F->maxsec=15;F->maxside=1;  F->ftype=SONY400KFLOPPY; }
    if (F->numblocks == 1600) {  F->maxtrk=80; F->maxsec=15;F->maxside=2;  F->ftype=SONY800KFLOPPY; }
    if (F->numblocks == 1720) {  F->maxtrk=46; F->maxsec=24;F->maxside=2;  F->ftype=TWIG860KFLOPPY; }

    F->size=F->sectoroffset+(F->numblocks*(F->sectorsize+F->tagsize));



    // defaults for available options /////////////////////////////////////////////////////////////////////////////////////////////////
    F->readonly = 0;
    F->synconwrite = 0;

    #ifdef HAVE_MMAPEDIO
     F->mmappedio=1;
    #else
     F->mmappedio=0;
    #endif

    // parse options for opening image ////////////////////////////////////////////////////////////////////////////////////////////////
    while (*options)
    {

       //{fprintf(stderr,"parsing option %c\n",*options); fflush(stderr);}

       switch (tolower(*options))
       {
          case 'r' : F->readonly=1;  break;             // r=read only
          case 'w' : F->readonly=0;  break;             // w=read/write
          case 'p' : F->readonly=2;  break;             // p=private writes (not written back to disk, kept in RAM to fool emulator)

          case 'm' :                                    // m=memory mapped I/O if available, else just plain disk I/O
                    #ifdef HAVE_MMAPEDIO
                     F->mmappedio=1;
                    #else
                     F->mmappedio=0;
                    #endif
                    break;

          case 'n' : F->mmappedio=0; break;             // n=never use mmapped I/O, nor RAM.
          case 'a' : F->mmappedio=2; break;             // a=always in RAM. manage it ourselves, even if we have mmapped I/O available
          case 'b' :                                    // b=best choice (for speed):
                     #ifdef HAVE_MMAPEDIO
                      F->mmappedio=1;break;             //   if we have mmapped I/O in the OS, use that.
                     #else
                      F->mmappedio=2;break;             //   otherwise, load the whole image in RAM.
                     #endif

          case 's'  : F->synconwrite=1; break;


          default  : {
                       int i=strlen(F->returnmsg);
                       if (i<80)   {F->returnmsg[i]=*options;  F->returnmsg[i+1]=0;}
                     }
       }
       options++;
    }

    if (strlen(F->returnmsg)>0 && strlen(F->returnmsg)<(80-14)) strncat(F->returnmsg," unknown opts",80);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



    if (F->readonly==0)
    {

       if (F->fd>2)  close(F->fd);                                // close the file and reopen it as possibly writeable
       if (F->fh  ) fclose(F->fh);

#ifndef __MSVCRT__
       F->fd=open(F->fname,O_RDWR);
       if (F->fd<3) DC42_RET_CODE(F,-86,"Cannot re-open file for writing",return F->retval);
       F->fh=NULL;
#else
       F->fh=fopen(F->fname,"r+b");
       if (!F->fh)  DC42_RET_CODE(F,-86,"Cannot re-open file for writing",return F->retval);
       F->fd=0;
#endif



       #ifdef HAVE_MMAPEDIO
       if (F->mmappedio)  { errno=0;

                            F->RAM=mmap(0,F->size,PROT_READ|PROT_WRITE,MAP_SHARED,F->fd,0);   // writeable image
                            //{fprintf(stderr,"MMAPped writeable image errno:%d\n",errno); fflush(stderr);}
                          }
       #endif
    }
    else
    {

        #ifdef HAVE_MMAPEDIO
        if (F->mmappedio) {  errno=0;

                             F->RAM=mmap(0,F->size,PROT_READ|PROT_WRITE,MAP_PRIVATE,F->fd,0);  // read only/private  - fd is already opened rd only
                             //{fprintf(stderr,"MMAPped READ-ONLY/PRIVATE image errno:%d\n",errno); fflush(stderr);}
                          }
        #endif
    }

    if (F->mmappedio==0) {

                           F->RAM=malloc( (F->tagsize + F->sectorsize) );

                            //{fprintf(stderr,"Direct to disk image\n"); fflush(stderr);}
                         }
    if (F->mmappedio==2) {


                           F->RAM=malloc(F->size);
                           if (F->RAM)
                              {  int i;
                                 if (F->fd>2)
                                 {
                                   lseek(F->fd,F->dc42seekstart,SEEK_SET);
                                   i=read(F->fd,F->RAM,F->size);
                                 }
                                 if (F->fh)
                                 {
                                   fseek(F->fh,F->dc42seekstart,SEEK_SET);
                                   i=fread(     F->RAM,F->size,1,F->fh);
                                 }
                              }


                         }


    // oops! no ram, or mmap failed.
    if ( !F->RAM || (F->RAM==(void *)(-1)) )
       {
         if (F->fd>2) { close(F->fd);  F->fd=-1  ;}
         if (F->fh  ) {fclose(F->fh);  F->fh=NULL;}

        DC42_RET_CODE(F,-99,"Could not mmap the file or allocate memory",return F->retval);
       }


    DC42_RET_CODE(F,0,"DC42 Image opened",return F->retval);
    return F->retval;                   // silence compiler warning about lack of return value
}



int dc42_open_by_handle(DC42ImageType *F, int fd, FILE *fh, long seekstart, char *options)
{
    int i;
    int32 filesizetotal=0;
    uint8 tempbuf[2048];
    F->dc42seekstart=0;
    // copy the file name into the image structure for later use
    strncpy(F->fname,"open handle",FILENAME_MAX);
    F->retval=0;

    // open the image as read only and grab it's header - we re-open it as r/w later if everything's happy.
    F->fd=fd;  F->fh=fh;

    if (F->fd<3 &&  !F->fh) DC42_RET_CODE(F,-86,"Cannot open file",return F->retval);
    if (F->fd>2 && !!F->fh) DC42_RET_CODE(F,-86,"Cannot double open file, pick either fd or fh and set the other to null",return F->retval);


    // if we were given a start, use that, otherwise get the current position inside the fd.
    F->dc42seekstart= (seekstart!=0) ? seekstart :   lseek(F->fd, 0, 0);


    if (F->fd>2)       i= read(F->fd,tempbuf,2048);  // read 1st 2k of image
    if (F->fh)         i=fread(      tempbuf,2048,1,F->fh);  // read 1st 2k of image


    // make sure that this is a DC42 image

    //printf("\ntempbuf[0]=%d version:%02x%02x\n",tempbuf[0],tempbuf[82],tempbuf[83]);
    if (tempbuf[0]>64 || tempbuf[82]!=1 || tempbuf[83]!=0)   DC42_RET_CODE(F,88,"DC42 volume name size wrong, or version not 0100",return F->retval);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    F->sectoroffset=DC42_HEADERSIZE;               // data starts from byte 84 and continues on in 512 byte chunx

    F->datasizetotal=(tempbuf[64+0]<<24)|(tempbuf[64+1]<<16)|(tempbuf[64+2]<<8)|tempbuf[64+3];
    F->tagsizetotal =(tempbuf[68+0]<<24)|(tempbuf[68+1]<<16)|(tempbuf[68+2]<<8)|tempbuf[68+3];

    if ((DC42_HEADERSIZE+F->datasizetotal+F->tagsizetotal-filesizetotal) > 1024)
       {
         if (F->fd>2) F->fd=0;
         if (F->fd>2) F->fh=NULL;
         DC42_RET_CODE(F,89,"File size does not match headers",return F->retval);
       }

    F->datasize=512;
	F->sectorsize=512;

    //F->numblocks=F->datasizetotal/F->sectorsize;   // for 400k disks=800 blocks, 800k disks=1600 - but get this from the image.  This is in bytes 1st
    F->numblocks=(F->datasizetotal)>>9;
    F->tagsize=F->tagsizetotal/F->numblocks;       // careful here since some images lack tags altogether!  This s/b 0 in that case.

    // tags start right after sector data
    F->tagstart=F->sectoroffset + (F->numblocks * F->sectorsize);

    if (F->numblocks ==  800) {  F->maxtrk=80; F->maxsec=15;F->maxside=1;  F->ftype=SONY400KFLOPPY; }
    if (F->numblocks == 1600) {  F->maxtrk=80; F->maxsec=15;F->maxside=2;  F->ftype=SONY800KFLOPPY; }
    if (F->numblocks == 1720) {  F->maxtrk=46; F->maxsec=24;F->maxside=2;  F->ftype=TWIG860KFLOPPY; }

    F->size=F->sectoroffset+(F->numblocks*(F->sectorsize+F->tagsize));


    // defaults for available options /////////////////////////////////////////////////////////////////////////////////////////////////
    F->readonly=0;

    #ifdef HAVE_MMAPEDIO
     F->mmappedio=1;
    #else
     F->mmappedio=0;
    #endif

    if (F->fh) F->mmappedio=0;

    // parse options for opening image ////////////////////////////////////////////////////////////////////////////////////////////////
    while (*options)
    {
       switch (tolower(*options))
       {
          case 'r' : F->readonly=1;  break;             // r=read only
          case 'w' : F->readonly=0;  break;             // w=read/write
          case 'p' : F->readonly=2;  break;             // p=private writes (not written back to disk, kept in RAM to fool emulator)

          case 'm' :                                    // m=memory mapped I/O if available, else just plain disk I/O
                    #ifdef HAVE_MMAPEDIO
                      F->mmappedio=(F->fd>2) ?1:0;      // cannot use fh for mmapped, need fd!
                    #else
                      F->mmappedio=0;
                    #endif
                    break;

          case 'n' : F->mmappedio=0; break;             // n=never use mmapped I/O, nor RAM.
          case 'a' : F->mmappedio=2; break;             // a=always in RAM. manage it ourselves, even if we have mmapped I/O available
          case 'b' :                                    // b=best choice (for speed):
                     #ifdef HAVE_MMAPEDIO
                      F->mmappedio=(F->fd>2) ?1:2;      // cannot use fh for mmapped, need fd!
                     #else
                      F->mmappedio=2;break;             //   otherwise, load the whole image in RAM.
                     #endif
                     break;

          case 's'  : F->synconwrite=1; break;

          default  : {
                       int i=strlen(F->returnmsg);
                       if (i<80)   {F->returnmsg[i]=*options;  F->returnmsg[i+1]=0;}
                     }
       }
       options++;
    }
    if (strlen(F->returnmsg)>0 && strlen(F->returnmsg)<(80-14)) strncat(F->returnmsg," unknown opts",80);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (F->fd>2) lseek(F->fd,F->dc42seekstart,SEEK_SET);
    if (F->fh  ) fseek(F->fh,F->dc42seekstart,SEEK_SET);


    #ifdef HAVE_MMAPEDIO
    if (F->mmappedio) F->RAM=mmap(0,F->size,PROT_READ|PROT_WRITE,MAP_PRIVATE,F->fd,0);  // read only/private
    else
    #endif
                      F->RAM=malloc(F->numblocks * (F->tagsize + F->sectorsize) );


    // if we can't read the whole disk image in ram, give up
    if ( !F->RAM || (F->RAM==(void *)(-1)) )
       {
         if (F->fd)  close(F->fd);
         if (F->fh) fclose(F->fh);
         F->fd=-1;
         F->fh=NULL;
         DC42_RET_CODE(F,-99,"Could not memory map the file",return F->retval);
       }

    if (F->numblocks==0) F->numblocks=  (F->datasizetotal/F->sectorsize);

    DC42_RET_CODE(F,0,"DC42 Image opened",return F->retval);
    return F->retval;                   // silence compiler warning about lack of return value
}


#ifdef USE_LZHUF  //fwd reference
 int LZHExpandBlock(uint8 *in, uint8 *out, int16 size, int sector);
#endif

int RLEExpandBlock(uint8 *in, uint8 *out, int16 size, int sector)
{

 int16 i=0, o=0, s, os=size*2;                 // in,out cursors, size word
 uint8 c1,c2;                                  // use chars to avoid endian issues, slower, but portable
 memset(out,0xaa,DART_CHUNK);


 while(o<DART_CHUNK && i<os)
 {
   c1=in[i];  c2=in[i+1];    i+=2; size--;
   s=(c1<<8)|(c2);                         // read a size word from input buffer



   if (s==0 || s>=DART_CHUNK || s<=-DART_CHUNK)
      {
         continue;
      }// return 0;}
   else if (s<0) // repeat same word if negative
      { s=-s;                       c1=in[i];  c2=in[i+1];    i+=2; size--; // read the word to repeat
        do
        {
                                    out[o]=c1; out[o+1]=c2; o+=2; s--;      // repeat it to output s times

        }
        while (s>0 && o<DART_CHUNK  );//&& i<os);

      }
   else
      {
        do
        {                           c1=in[i];  c2=in[i+1];    i+=2; size--; s--;
                                    out[o]=c1; out[o+1]=c2;   o+=2;
        }
        while (s>0 && o<DART_CHUNK); // && i<os);

      }
 }

 return 0;
}


int dc42_set_volname(DC42ImageType *F,char *name)
{
  //char volname[64];
  uint8 slen;
  int i;
  char *newname;

  DC42_CHECK_VALID_F(F);

  newname=(name==NULL ? NotAMacintoshDisk: name);

  slen=strlen(newname);
  if (slen>63) slen=63;

  memset(F->RAM,0,64);

  F->RAM[0]=slen;

  for (i=0; i<slen; i++) F->RAM[1+i]=newname[i];

  if (!F->mmappedio)
     { int i;
       if (F->fd)
       {
        lseek(  F->fd,F->dc42seekstart,SEEK_SET);
        i=write(F->fd,F->RAM,64);
       }
       if (F->fh)
       {
        fseek(  F->fh,F->dc42seekstart,SEEK_SET);
        i=fwrite(     F->RAM,64,1,F->fh);
       }
     }

  return 0;
}



char *dc42_get_volname(DC42ImageType *F)
{
  static char volname[64];
  int i,slen;
  DC42_CHECK_VALID_F_NUL(F);
  memset(volname,0,64);

  if (!F->mmappedio)
     {
       if (F->fd>2)
       {
          lseek( F->fd,F->dc42seekstart,SEEK_SET);
          i=read(F->fd,F->RAM,64);
       }
       if (F->fh  )
       {
          fseek(  F->fh,F->dc42seekstart,SEEK_SET);
          i=fread(      F->RAM,64,1,F->fh);
       }

     }
  slen=F->RAM[0];
  if (slen>63) slen=63;

  for (i=0; i<slen; i++) volname[i]=F->RAM[i+1];

  return volname;
}


// creates a new blank DC42 image, but does not open it.  Use the dc42_open to open it after call this
int dc42_create(char *filename, char *volname, uint32 datasize, uint32 tagsize)   // create a new disk image
{

  FILE *newimage;
  char pascalstring[64];
  uint8 i,dskformat, formatbyte;

  newimage=fopen(filename,"wb");
  if (!newimage) return -86;


   // fprintf(stderr,"\nlibdc42:Creating disk image:%s\n",filename); fflush(stderr);


  i=strlen(volname);  if (i>62) i=62;
  pascalstring[0]=i;                      // pstr length
  do
     {pascalstring[i+1]=volname[i];}      // copy the volume name
  while (i--);

  fwrite(pascalstring,64,1,newimage);     // filename

  fputc((datasize>>24) & 0xff,newimage);  // datasize
  fputc((datasize>>16) & 0xff,newimage);
  fputc((datasize>>8 ) & 0xff,newimage);
  fputc((datasize    ) & 0xff,newimage);

  fputc(( tagsize>>24) & 0xff,newimage);  // tagsize
  fputc(( tagsize>>16) & 0xff,newimage);
  fputc(( tagsize>>8 ) & 0xff,newimage);
  fputc(( tagsize    ) & 0xff,newimage);

  fputc(0,newimage);                      // data checksum
  fputc(0,newimage);
  fputc(0,newimage);
  fputc(0,newimage);

  fputc(0,newimage);                      // tag checksum
  fputc(0,newimage);
  fputc(0,newimage);
  fputc(0,newimage);

  dskformat=93;  formatbyte=0x22;
  if (datasize==800*512   && tagsize==800*12)   {dskformat=0; formatbyte=0x12;}
  if (datasize==1600*512  && tagsize==1600*12)   dskformat=1;

  if (datasize==800*512   && tagsize==0)        {dskformat=0; formatbyte=0x12;}
  if (datasize==1600*512  && tagsize==0)         dskformat=1;
  if (datasize==720*1024  && tagsize==0)         dskformat=2;
  if (datasize==1440*1024 && tagsize==0)         dskformat=3;

  if (dskformat==93) formatbyte=0x93;     // non-standard disk image


  fputc(dskformat,newimage);              //diskformat
  fputc(formatbyte,newimage);             //diskformat

  fputc(1,newimage); fputc(0,newimage);  // private flag

  fseek(newimage,84+datasize+tagsize-1,0);  // create a sparse file of the proper size
  fputc(0,newimage);

  fclose(newimage);

  return 0;
}

// This does not open an image, do not use this on an already opened image.

int dc42_add_tags(char *filename, uint32 tagsize) // tagsize is in bytes.  for a 400K disk use 400*2*12 for tagsize
{
  FILE *image;
  uint32 oldtagsize, olddatasize, filesize, newfilesize;

  if (!tagsize) return -22;

  image=fopen(filename,"rb");
  if (!image) return -86;

  fseek(image,64,SEEK_SET);
  olddatasize=(fgetc(image)<<24)|(fgetc(image)<<16)|(fgetc(image)<<8)|(fgetc(image) );
  oldtagsize =(fgetc(image)<<24)|(fgetc(image)<<16)|(fgetc(image)<<8)|(fgetc(image) );

  if (tagsize==0)                              // if tagsize=0, then we automatically add 12 tag bytes for every sector
      tagsize=(olddatasize/512)*12;

  if (oldtagsize<tagsize)                      // if the existing tag size is smaller than what we want, rewrite the size.
     {
       fseek(image,68,SEEK_SET);
       fputc(( tagsize>>24) & 0xff,image);  // tagsize
       fputc(( tagsize>>16) & 0xff,image);
       fputc(( tagsize>>8 ) & 0xff,image);
       fputc(( tagsize    ) & 0xff,image);
     }

  fseek(image,0,SEEK_END);                     // go to the end of the file, find out it's size
  filesize=ftell(image);
  newfilesize=84+tagsize+olddatasize;

  if (filesize<newfilesize)                    // if the current file size is too small, grow it so that mmap will work properly
     {fseek(image,newfilesize-1,SEEK_SET); fputc(0,image);}

  fclose(image);

  return 0;
}


// this does not open the image
int dart_is_valid_image(char *dartfilename)
{
 FILE *dart;
 int i;

 uint32 totalsizes=0, totalsizesec=0;

 uint8  dart_compression_type;          // 0=RLE, 1=LZH, 2=No compression
 uint8  dart_disk_type;                 // 1=Mac, 2=Lisa, 3=Apple II
 uint32 dart_size;
 int    dart_blocks;                    // how many DART_CHUNKS in the file?
 int16  dart_blocksize; //s[72];

 int is_macbin=dc42_is_valid_macbinii(dartfilename,NULL);


 dart=fopen(dartfilename,"rb");
 if (!dart) return 0; //DC42_RET_CODE(F,0,"Cannot open DART file for reading",return F->retval);

 // if the DART image is wrapped inside a MacBinary header, skip over the 1st 128 bytes + optional extra header before reading the DART header
 if (is_macbin) 
    {
	  uint16 extras; uint8 ch, cl;

	  //  Length of a secondary header. If this is non-zero, skip this many bytes (rounded up to the next multiple of 128). 
	  fseek(dart,120,SEEK_SET);  ch=fgetc(dart); cl=fgetc(dart);
	  extras=(ch<<8) | (cl);
 	  if ((extras & 127)) extras=(extras|127)+1;

	  fseek(dart,128+extras,SEEK_SET);
    }



 dart_compression_type=fgetc(dart); if (dart_compression_type>2) return 0;
 dart_disk_type=fgetc(dart);        if ((dart_disk_type>3 && dart_disk_type<16) || (dart_disk_type>18)) return 0;

 dart_size=((fgetc(dart)<<8) | (fgetc(dart)));  // size is in kb

 dart_blocks=(dart_size<801) ? 40:72;


 for (i=0; i<dart_blocks; i++)                                // read the DART chunk sizes
 {
    dart_blocksize=(int16)(((uint8)fgetc(dart)<<8) | (uint8)fgetc(dart));
    if (feof(dart)) return 0;
    if (dart_blocksize!=0) totalsizes+=DART_CHUNK_TAGS;   // calc disk size without tags
 }

 totalsizesec=totalsizes/512;

 // so far so good, is the size a known valid DART disk size?
 switch (totalsizesec)
 { case 800 :                 // 400K single sided double density  3.5" GCR
   case 1600 :                // 800K double sided double density  3.5" GCR
   case 1440 :                // 720KB single sided double density 3.5" MFM PC
   case 2880 :                // 1.4MB double sided high density   3.5" MFM PC
               return 1+is_macbin; //;DC42_RET_CODE(F,1,"File is valid DART disk image archive",return F->retval);
 }

 return 0;                              // here instead of in switch to avoid dumbass compiler warning.
}


// this does not open the image
int dart_to_dc42(char *dartfilename, char *dc42filename)
{

 DC42ImageType FL, *F=&FL;
 FILE *dart;
 int i,j;


 uint32 totalsizes=0, totalsizesec=0, sectornum=0;

 uint8  dart_compression_type;          // 0=RLE, 1=LZH, 2=No compression
 uint8  dart_disk_type;                 // 1=Mac, 2=Lisa, 3=Apple II
 uint32 dart_size;
 int    dart_blocks;                    // how many DART_CHUNKS in the file?
 int16  dart_blocksizes[72];

 //uint8  sector[512];
 //uint8  tags[12];


 uint8  dart_block_in[DART_CHUNK];
 uint8  dart_block_out[DART_CHUNK];
 char volname[65];
 char *volname_ptr=NotAMacintoshDisk;
 int is_macbin=dc42_is_valid_macbinii(dartfilename,NULL);

 dart=fopen(dartfilename,"rb");
 if (!dart) return -86; //DC42_RET_CODE(F,-86,"Cannot open DART file for writing",return F->retval);

 // if the DART image is wrapped inside a MacBinary header, skip over the 1st 128 bytes + optional extra header before reading the DART header
 if (is_macbin) 
    {
	  uint16 extras; uint8 ch, cl;

	  //  Length of a secondary header. If this is non-zero, skip this many bytes (rounded up to the next multiple of 128). 
	  fseek(dart,120,SEEK_SET);  ch=fgetc(dart); cl=fgetc(dart);
	  extras=(ch<<8) | (cl);
	  if (extras & 127) extras=(extras|127)+1;

	  fseek(dart,128+extras,SEEK_SET);
    }
 dart_compression_type=fgetc(dart); if (dart_compression_type>2) return -10;
 dart_disk_type=fgetc(dart);        if ((dart_disk_type>3 && dart_disk_type<16) || (dart_disk_type>18))
                                        return 10; //;DC42_RET_CODE(F,10,"Unrecognized DART image disk type",return F->retval);

 dart_size=((fgetc(dart)<<8) | (fgetc(dart)));  // size is in kb

 dart_blocks=(dart_size<801) ? 40:72;


 for (i=0; i<dart_blocks; i++)                                // read the DART chunk sizes
 {
    dart_blocksizes[i]=(int16)(((uint8)fgetc(dart)<<8) | (uint8)fgetc(dart));
    if (feof(dart)) return -9;
    if (dart_blocksizes[i]!=0) totalsizes+=DART_CHUNK_TAGS;   // calc disk size without tags
 }

 totalsizesec=totalsizes/512;

                                                                  //012345678901234567890
 if (dart_disk_type!=1 || dart_disk_type!=16) {snprintf(volname,62,"DART Converted image"); volname_ptr=volname;}

 // create the DC42 image - no tags for 720/1440KB disks, if there's an error, return the error back to caller.

 //printf("\n creating %s as %s \n",dc42filename,volname);

 i=dc42_create(dc42filename,
               volname,
               totalsizes,
               ((totalsizesec==1440 || totalsizesec==2880) ? 0:(totalsizesec*12) )             );

 if (i) return i;


 // open the DC42 image

 //printf("Opening %s \n",dc42filename);
 if (dc42_open(F,dc42filename,"w")) return -99;

 // decompress and copy DART data to the DC42 image.
 for (sectornum=0,i=0; i<dart_blocks && dart_blocksizes[i]; i++)
 {
   //printf("chunk:%d sectornum:%d,compression_type:%d feof:%d\n",i,sectornum,dart_compression_type,feof(dart));

   if (feof(dart)) return -99; //DC42_RET_CODE(F,-99,"Unexpected EOF while reading DART file",return F->retval);

   memset(dart_block_out,0x00,DART_CHUNK);  // empty the block
   if (dart_blocksizes[i]<0 || dart_compression_type==2)
      {     int ignore;
            // read the uncompressed block directly to the out buffer
            //printf("Blocksize <0:(%d %04x) so reading %d bytes direct\n",dart_blocksizes[i],dart_blocksizes[i],DART_CHUNK);
            ignore=fread(dart_block_out,DART_CHUNK,1,dart);
            //printf("Uncompressed block:%d sec:%d\n",i,sectornum);
      }
   else
      {     // read the compressed block and decompress it to the out buffer
            int ignore;
            memset(dart_block_in,0x33,DART_CHUNK);

            if (dart_compression_type==0)    ignore=fread(dart_block_in,dart_blocksizes[i]*2,1,dart);
            else                             ignore=fread(dart_block_in,dart_blocksizes[i]  ,1,dart);

            //printf("expanding blocks (#%d-%d) of type %d(0=RLE,2=uncompressed) size:%d \n",
            //        sectornum,
            //        sectornum+39,
            //        dart_compression_type,
            //        dart_blocksizes[i]);

            //printf("Compressed chunk:%d sec:%d type:%d size:%d\n",i,sectornum,dart_compression_type,dart_blocksizes[i]);

            if (dart_compression_type==0) RLEExpandBlock(dart_block_in, dart_block_out, dart_blocksizes[i], sectornum);
            #ifdef USE_LZHUF
            if (dart_compression_type==1) LZHExpandBlock(dart_block_in, dart_block_out, dart_blocksizes[i], sectornum);
            #else
            if (dart_compression_type==1)
            {   //LZHExpandBlock(dart_block_in,dart_block_out,dart_blocksizes[i]);
                return -3; //;DC42_RET_CODE(F,-3,"I don't know how to deal with LZH compressed DART files.  Sorry.",return F->retval);
            }
            #endif
      }


   // transfer tags and data to the DC42 image in 40 sector chunks at a time.
   for (j=0; j<40; j++)                 // 40 sectors in each DART chunk (512+12)*40=DART_CHUNK 20960)
    {
      //printf("transferring tags sec:%d j:%d\n",sectornum,j);
      if (sectornum<=totalsizesec) //400K images have 0'ed out sectors at the end so this fixes them.
         { dc42_write_sector_tags(F,sectornum,&dart_block_out[DART_CHUNK_TAGS+(j*12)]);
           dc42_write_sector_data(F,sectornum,&dart_block_out[j*512]); }

      sectornum++;
    }
 }


 dc42_close_image(F);
 fclose(dart);
 //printf("\n closed images \n");
 return 0;  //DC42_RET_CODE(F,0,"DART Image successfully converted",return F->retval);
}

// 1 if it is, 0 if it's not
int dc42_is_valid_macbinii(char *infilename, char *creatortype)
{
int ignore;
FILE *infile;
unsigned char buffer[128];
char *filename=infilename;
infile=fopen(filename,"rb"); if (!infile) return 0;
ignore=fread(buffer,128,1,infile);
fclose(infile);

if (!buffer[0] && buffer[1] && buffer[1]<64 && !buffer[74] && !buffer[82])
{
 int i,j;

   uint32 datasize=(buffer[83]<<24)|(buffer[84]<<16)|(buffer[85]<<8)|(buffer[86]);
   uint32 resrsize=(buffer[87]<<24)|(buffer[88]<<16)|(buffer[89]<<8)|(buffer[90]);
   uint16 header2 =(buffer[120]<<16)|(buffer[121]);
   if (header2 & 127) header2=(header2|127)+1;
   buffer[1]+=2;
   for (i=2; i<buffer[1]; i++)
        if (buffer[i]==':')                                 return 0;

   if (datasize>0x800000 || resrsize>0x800000)              return 0;

   if (creatortype!=NULL)
   {
       for (i=65; i<73; i++) creatortype[i-65]=buffer[i];
       creatortype[8]=0;
   }


   return 1;
}


return 0;
}



// file name is overwritten by new file name!
// will return 1 if converted,
// 0 if not converted,
// -1 can't read file,
// -2 if out of memory - filename clobbered
// -3 if unable to create new file - filename clobbered
int dc42_extract_macbinii(char *infilename)
{
int i;
FILE *infile;
char buffer[65536];
char *filename=infilename;
infile=fopen(filename,"rb");                       if (!infile) return 0;
i=fread(buffer,128,1,infile);

if (!buffer[0] && buffer[1] && buffer[1]<64 && !buffer[74] && !buffer[82])
{
 int i,j;

   uint32 datasize=(buffer[83]<<24)|(buffer[84]<<16)|(buffer[85]<<8)|(buffer[86]);
   uint32 resrsize=(buffer[87]<<24)|(buffer[88]<<16)|(buffer[89]<<8)|(buffer[90]);
   uint16 header2 =(buffer[120]<<16)|(buffer[121]);
   if (header2 & 127) header2=(header2|127)+1;
   buffer[1]+=2;
   for (i=2; i<buffer[1]; i++) if (buffer[i]==':')              {fclose(infile);return 0;}

   if (datasize>0x800000 || resrsize>0x800000)                  {fclose(infile);return 0;}

   // valid MacBinII image - we extract only the data fork!
   for (j=0,i=2; i<buffer[1]; i++,j++)
       {// normalize filename.
        if (buffer[i]=='/' || buffer[i]==' ' || buffer[i]=='\\' || buffer[i]=='<' ||
            buffer[i]=='*' || buffer[i]=='"' || buffer[i]=='?'  || buffer[i]=='>'   )
                       filename[j]='_';
        else           filename[j]=buffer[i];
       }
   filename[j]=0; // cstr end.

   FILE *datafork=fopen(filename,"wb"); if (!datafork)          {fclose(infile);return -3;}
   fseek(infile,128+header2,SEEK_SET);

   int32 datasz=(int32)datasize;

   while(datasz>=0)
   {
     int i;
     i=fread( buffer,datasz>=65536 ? 65536:datasize,1,infile);
     i=fwrite(buffer,datasz>=65536 ? 65536:datasize,1,datafork);
     datasz-=65536;
   }
   fclose(infile);
   fclose(datafork);

   return 1;
}

fclose(infile);
return 0;
}





// opens a DC42 image automatically converting it from DART, and/or stripping MacBinII headers if needed
int dc42_auto_open(DC42ImageType *F, char *infilename, char *options)
{
    int i=0;
    char filename[FILENAME_MAX+2];

    // we do it this way so that the macbinii extraction can change the file name
    // and get the filename passed to the next layer!

    strncpy(filename,infilename,FILENAME_MAX);
    if ( dc42_extract_macbinii(filename)<=0 )
         strncpy(filename,infilename,FILENAME_MAX);

    if (dart_is_valid_image(filename))
    {
     char dc42filename[FILENAME_MAX+2];
     strncpy(dc42filename,filename,FILENAME_MAX-2);
     if   (strlen(dc42filename)<FILENAME_MAX-6)   strcat(dc42filename,".dc42");
     else                                         strcpy( (char *)(dc42filename+FILENAME_MAX-6),".dc42");

     i=dart_to_dc42(filename,dc42filename);
     if (!i)      i=dc42_open(F, dc42filename, options);
     return i;
    }

    i=dc42_open(F, filename, options);
    return i;

}


int searchseccount( DC42ImageType *F, int sector, int size, uint8 *s)
{
    uint8 *d=dc42_read_sector_data(F,sector);
    if (!F) return -1;
    if (!d) return -1;

    int xsize=512-size;
    int i,j,count=0;

    for (i=0; i<xsize; i++)
    {
        if (d[i]==s[0]) { for (j=1; j<size && j>0; j++)
                              if (d[i+j]!=s[j]) j=-1;
                          if (j>0) count++;
                        }
    }

    return count;
}

int replacesec(DC42ImageType *F, int sector, int size, uint8 *s, uint8 *r)
{
    int xsize=512-size;
    int i,j,count=0;
    uint8 d[512];
    if (!F) return -1;
    uint8 *din=dc42_read_sector_data(F,sector);
    if (!din) return -1;

    memcpy(d,din,512);

    for (i=0; i<xsize; i++)
    {
        if (d[i]==s[0]) { for (j=1; j<size && j>0; j++)
                              if (d[i+j]!=s[j]) j=-1;
                          if (j>0) {
                                     count++;
                                     if (r) memcpy(&d[i],r,size);
                                   }
                        }
    }

    if (count && r!=NULL) dc42_write_sector_data(F,sector,d);

    return count;
}



//////
////// NOTE: I do not claim any sort of copyright for the following code, nor does it necessarily fall
////// under the GPL.  This section of the code was found on the web and on usenet.  If you have problems
////// with this, disable it and do not use it!
//////
////// Modifications were made in order to allow this code to work with the rest of libdc42, I do not claim
////// any rights to said modifications either.  The technical comments regarding this code may no longer apply
////// YMMV, use with care.  I've forced some of this code to be static so that it will not affect anything you
////// link it to.
//////
////// The code was found here: http://www.moon-soft.com/download/download.asp?id=3522&no=0 which downloads as
////// LZHSRC10.ZIP.  It can also be found here: http://cd.textfiles.com/blackphiles/PHILES/COMPUTER/LZHSRC10.ZIP
//////
////// Another version of this code came with the following comment:
/*
LZHUF.C (c)1989 by Haruyasu Yoshizaki, Haruhiko Okumura, and Kenji Rikitake.
All rights reserved. Permission granted for non-commercial use.
*/
/////// I did not use that version because I didn't want the 8086 assembly, but rather a C version.
///////
////// The .zip file came bundled with the following comments:
//////
/* MESSAGE:

From: Kenji Rikitake <c31293%tansei.cc.u-tokyo.ac.jp@RELAY.CS.NET>
Subject: lzhuf.c (LHarc algorithm source code)

Dear Rahul,

Following is a source code of single-file compression/uncompresssion
program. It has the same compression algorithm with LHarc.
I have got a permission to post this to the USENET community
from Haruyasu Yoshizaki.
-- Kenji

* POSTER:

Subject: v02i042: C source for lharc's LZHuff compression algorithm
Summary: lzhsrc10.arc, C source for lharc's LZHuff compression algorithm
Date: 6 Apr 89 01:00:33 GMT
Posted: Wed Apr  5 20:00:33 1989

Posting-number: Volume 02, Issue 042
Originally-from: Haruhiko Okumura, Haruyasu Yoshizaki, & Kenji Rikitake
Submitted-by: Kenji Rikitake <c31293%tansei.cc.u-tokyo.ac.jp@RELAY.CS.NET>
Archive-name: lharc/lzhsrc10.uue

[ US users should note that the cost of international email is usually
borne by the non-US recipient or sender.  -- R.D. ]

Following is a source code of single-file compression/uncompresssion
program. It has the same compression algorithm with LHarc.  I have got
a permission to post this to the USENET community from Haruyasu
Yoshizaki.
-- Kenji

[
>From the source:

    * LZHUF.C English version 1.0
    * Based on Japanese version 29-NOV-1988
    * LZSS coded by Haruhiko OKUMURA
    * Adaptive Huffman Coding coded by Haruyasu YOSHIZAKI
    * Edited and translated to English by Kenji RIKITAKE

The source is fairly generic but has a few ANSI-specific features such
as memmove() and function prototypes.

It compiles with Turbo C 1.0.  The resulting program is quite fast,
considering it is written purely in C.  It seems to compress exactly as
well as lharc itself.  I did not try to compile it on any other
system.

This would have been the perfect text posting except that there are
some control characters in the source, so I archived it.

-- R.D.


*/
//////////////////

// RA2006.09.08 Notes: might need to change the 4096 to 20960 or near it. 0x51e0

#ifdef USE_LZHUF
//------------------------- >8 cut here if you do not want this 8< -----------------------------------------------------

#ifdef getc
#undef getc
#endif

#ifdef putc
#undef putc
#endif

#define getc(infile)    ((FilePosition >= EndOfFilePosition) ? ((int) -1) : (uint8) *FilePosition++)
#define putc(z,outfile) { if ( PutFilePosition <= EndOfPutFilePosition) *PutFilePosition++ = z;  }


static uint8  *FilePosition;
static uint8  *EndOfFilePosition;
static uint8  *PutFilePosition;
static uint8  *EndOfPutFilePosition;
static uint32 textsize = 0, codesize = 0, printcount = 0;

static FILE *infile, *outfile;

// RA2006.09.08 Notes: might need to change the 4096 to 20960 or near it. 0x51e0

/*
 * LZHUF.C English version 1.0
 * Based on Japanese version 29-NOV-1988
 * LZSS coded by Haruhiko OKUMURA
 * Adaptive Huffman Coding coded by Haruyasu YOSHIZAKI
 * Edited and translated to English by Kenji RIKITAKE
 */

//2006.09.08//#include <stdio.h>
//2006.09.08//#include <stdlib.h>
//2006.09.08//#include <string.h>
//2006.09.08//#include <ctype.h>



/* These values are Turbo-C dependent;
   EXIT_SUCCESS, EXIT_FAILURE
   renamed by Kenji */

#define EXIT_OK 0
#define EXIT_FAILED -1


static void Error(char *message)
{
	printf("\n%s\n", message);
    exit(EXIT_FAILED);
}

/* LZSS Parameters */

#define N       4096 //was 4096    /* Size of string buffer */
#define F       60 //was 60  /* Size of look-ahead buffer */
#define THRESHOLD	2
#define NIL		N	/* End of tree's node  */

static uint8        text_buf[N + F - 1];
static int16         match_position, match_length,    lson[N + 1], rson[N + 257], dad[N + 1];

// RA20060915
static int16 *saved_son=NULL, *saved_lson=NULL, *saved_rson=NULL, *saved_dad=NULL, *saved_freq=NULL,* saved_prnt=NULL;
// A DART image uses 40 block chunks at a time.  each time it's called the above arrays are re-processed which is very slow
// this prevent that from happening by storing them the first time they are initialized.
static void restore_huff(void);         // fwd reference




static void InitTree(void)  /* Initializing tree */
{
    int16  i;

	for (i = N + 1; i <= N + 256; i++)
		rson[i] = NIL;			/* root */
	for (i = 0; i < N; i++)
		dad[i] = NIL;			/* node */
}

static void InsertNode(int16 r)  /* Inserting node to the tree */
{
    int16  i, p, cmp;
    uint8       *key;
    uint16 c;

	cmp = 1;
	key = &text_buf[r];
	p = N + 1 + key[0];
	rson[r] = lson[r] = NIL;
	match_length = 0;
	for ( ; ; ) {
		if (cmp >= 0) {
			if (rson[p] != NIL)
				p = rson[p];
			else {
				rson[p] = r;
				dad[r] = p;
				return;
			}
		} else {
			if (lson[p] != NIL)
				p = lson[p];
			else {
				lson[p] = r;
				dad[r] = p;
				return;
			}
		}
		for (i = 1; i < F; i++)
			if ((cmp = key[i] - text_buf[p + i]) != 0)
				break;
		if (i > THRESHOLD) {
			if (i > match_length) {
				match_position = ((r - p) & (N - 1)) - 1;
				if ((match_length = i) >= F)
					break;
			}
			if (i == match_length) {
				if ((c = ((r - p) & (N - 1)) - 1) < match_position) {
					match_position = c;
				}
			}
		}
	}
	dad[r] = dad[p];
	lson[r] = lson[p];
	rson[r] = rson[p];
	dad[lson[p]] = r;
	dad[rson[p]] = r;
	if (rson[dad[p]] == p)
		rson[dad[p]] = r;
	else
		lson[dad[p]] = r;
	dad[p] = NIL;  /* remove p */
}

static void DeleteNode(int16 p)  /* Deleting node from the tree */
{
    int16 q;

	if (dad[p] == NIL)
		return;			/* unregistered */
	if (rson[p] == NIL)
		q = lson[p];
	else
	if (lson[p] == NIL)
		q = rson[p];
	else {
		q = lson[p];
		if (rson[q] != NIL) {
			do {
				q = rson[q];
			} while (rson[q] != NIL);
			rson[dad[q]] = lson[q];
			dad[lson[q]] = dad[q];
			lson[q] = lson[p];
			dad[lson[p]] = q;
		}
		rson[q] = rson[p];
		dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p)
		rson[dad[p]] = q;
	else
		lson[dad[p]] = q;
	dad[p] = NIL;
}

/* Huffman coding parameters */

#define N_CHAR  	(256 - THRESHOLD + F)
				/* character code (= 0..N_CHAR-1) */
#define T 		(N_CHAR * 2 - 1)	/* Size of table */
#define R 		(T - 1)			/* root position */
#define MAX_FREQ	0x8000
					/* update when cumulative frequency */
					/* reaches to this value */

typedef unsigned char uchar;

/*
 * Tables for encoding/decoding upper 6 bits of
 * sliding dictionary pointer
 */
/* encoder table */
static uchar p_len[64] = {
	0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

static uchar p_code[64] = {
	0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
	0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
	0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
	0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
	0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
	0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* decoder table */
static uchar d_code[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
	0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
	0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
	0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
	0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
	0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
	0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
	0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
	0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
	0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
	0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
	0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B,
	0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};

static uchar d_len[256] = {
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

static uint16 freq[T + 1];  /* cumulative freq table */

/*
 * pointing parent nodes.
 * area [T..(T + N_CHAR - 1)] are pointers for leaves
 */
static int16 prnt[T + N_CHAR];

/* pointing children nodes (son[], son[] + 1)*/
static int16 son[T];

static uint16 getbuf = 0;
static uchar getlen = 0;

static int16 GetBit(void)    /* get one bit */
{
    int16 i;

	while (getlen <= 8) {
		if ((i = getc(infile)) < 0) i = 0;
		getbuf |= i << (8 - getlen);
		getlen += 8;
	}
	i = getbuf;
	getbuf <<= 1;
	getlen--;
	return (i < 0);
}

static int16 GetByte(void)   /* get a byte */
{
    uint16 i;
        int q;

	while (getlen <= 8) {
		if ((q = getc(infile)) < 0) q = 0;
		i=q;
		getbuf |= i << (8 - getlen);
		getlen += 8;
	}
	i = getbuf;
	getbuf <<= 8;
	getlen -= 8;
	return i >> 8;
}

static uint16 putbuf = 0;
static uchar putlen = 0;

static void Putcode(int16 l, uint16 c)        /* output c bits */
{
	putbuf |= c >> putlen;
	if ((putlen += l) >= 8) {
		putc(putbuf >> 8, outfile);
		if ((putlen -= 8) >= 8) {
			putc(putbuf, outfile);
			codesize += 2;
			putlen -= 8;
			putbuf = c << (l - putlen);
		} else {
			putbuf <<= 8;
			codesize++;
		}
	}
}


/* initialize freq tree */

static void StartHuff(void)
{
    int16 i, j;

	for (i = 0; i < N_CHAR; i++) {
		freq[i] = 1;
		son[i] = i + T;
		prnt[i + T] = i;
	}
	i = 0; j = N_CHAR;
	while (j <= R) {
		freq[j] = freq[i] + freq[i + 1];
		son[j] = i;
		prnt[i] = prnt[i + 1] = j;
		i += 2; j++;
	}
	freq[T] = 0xffff;
	prnt[R] = 0;
}


/* reconstruct freq tree */

static void reconst(void)
{
    int16 i, j, k;
    uint16 f, l;

	/* halven cumulative freq for leaf nodes */
	j = 0;
	for (i = 0; i < T; i++) {
		if (son[i] >= T) {
			freq[j] = (freq[i] + 1) / 2;
			son[j] = son[i];
			j++;
		}
	}
	/* make a tree : first, connect children nodes */
	for (i = 0, j = N_CHAR; j < T; i += 2, j++) {
		k = i + 1;
		f = freq[j] = freq[i] + freq[k];
		for (k = j - 1; f < freq[k]; k--);
		k++;
		l = (j - k) * 2;

		/* movmem() is Turbo-C dependent
		   rewritten to memmove() by Kenji */

		/* movmem(&freq[k], &freq[k + 1], l); */
		(void)memmove(&freq[k + 1], &freq[k], l);
		freq[k] = f;
		/* movmem(&son[k], &son[k + 1], l); */
		(void)memmove(&son[k + 1], &son[k], l);
		son[k] = i;
	}
	/* connect parent nodes */
	for (i = 0; i < T; i++) {
		if ((k = son[i]) >= T) {
			prnt[k] = i;
		} else {
			prnt[k] = prnt[k + 1] = i;
		}
	}
}


/* update freq tree */

static void update(int16 c)
{
    int16 i, j, k, l;

	if (freq[R] == MAX_FREQ) {
		reconst();
	}
	c = prnt[c + T];
	do {
		k = ++freq[c];

		/* swap nodes to keep the tree freq-ordered */
		if (k > freq[l = c + 1]) {
			while (k > freq[++l]);
			l--;
			freq[c] = freq[l];
			freq[l] = k;

			i = son[c];
			prnt[i] = l;
			if (i < T) prnt[i + 1] = l;

			j = son[l];
			son[l] = i;

			prnt[j] = c;
			if (j < T) prnt[j + 1] = c;
			son[c] = j;

			c = l;
		}
	} while ((c = prnt[c]) != 0);	/* do it until reaching the root */
}

static uint16 code, len;

static void EncodeChar(uint16 c)
{
    uint16 i;
    int16 j, k;

	i = 0;
	j = 0;
	k = prnt[c + T];

	/* search connections from leaf node to the root */
	do {
		i >>= 1;

		/*
		if node's address is odd, output 1
		else output 0
		*/
		if (k & 1) i += 0x8000;

		j++;
	} while ((k = prnt[k]) != R);
	Putcode(j, i);
	code = i;
	len = j;
	update(c);
}

static void EncodePosition(uint16 c)
{
    uint16 i;

	/* output upper 6 bits with encoding */
	i = c >> 6;
    Putcode(p_len[i], (uint16)p_code[i] << 8);

	/* output lower 6 bits directly */
	Putcode(6, (c & 0x3f) << 10);
}

static void EncodeEnd(void)
{
	if (putlen) {
		putc(putbuf >> 8, outfile);
		codesize++;
	}
}

static int16 DecodeChar(void)
{
    uint16 c;

	c = son[R];

	/*
	 * start searching tree from the root to leaves.
	 * choose node #(son[]) if input bit == 0
	 * else choose #(son[]+1) (input bit == 1)
	 */
	while (c < T) {
		c += GetBit();
		c = son[c];
	}
	c -= T;
	update(c);
	return c;
}

static int16 DecodePosition(void)
{
    uint16 i, j, c;

	/* decode upper 6 bits from given table */
	i = GetByte();
    c = (uint16)d_code[i] << 6;
	j = d_len[i];

	/* input lower 6 bits directly */
	j -= 2;
	while (j--) {
		i = (i << 1) + GetBit();
	}
    return c | (i & 0x3f);      //RA 20061214 - not 100% sure if this is (c|i) or c | (i & 0x3f) - tests seem to indicate this is right.
}

/* Compression */

static void Encode(void)  /* Encoding/Compressing */
{
    int16  i, c, len, r, s, last_match_length;

	fseek(infile, 0L, 2);
	textsize = ftell(infile);
	if (fwrite(&textsize, sizeof textsize, 1, outfile) < 1)
		Error("Unable to write");	/* write size of original text */
	if (textsize == 0)
		return;
	rewind(infile);
	textsize = 0;			/* rewind and rescan */
	StartHuff();
	InitTree();
	s = 0;
	r = N - F;
	for (i = s; i < r; i++)
		text_buf[i] = ' ';
	for (len = 0; len < F && (c = getc(infile)) != EOF; len++)
		text_buf[r + len] = c;
	textsize = len;
	for (i = 1; i <= F; i++)
		InsertNode(r - i);
	InsertNode(r);
	do {
		if (match_length > len)
			match_length = len;
		if (match_length <= THRESHOLD) {
			match_length = 1;
			EncodeChar(text_buf[r]);
		} else {
			EncodeChar(255 - THRESHOLD + match_length);
			EncodePosition(match_position);
		}
		last_match_length = match_length;
		for (i = 0; i < last_match_length &&
				(c = getc(infile)) != EOF; i++) {
			DeleteNode(s);
			text_buf[s] = c;
			if (s < F - 1)
				text_buf[s + N] = c;
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			InsertNode(r);
		}
		if ((textsize += i) > printcount) {
            //printf("%12ld\r", textsize);
			printcount += 1024;
		}
		while (i++ < last_match_length) {
			DeleteNode(s);
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			if (--len) InsertNode(r);
		}
	} while (len > 0);
	EncodeEnd();
    // printf("input: %ld bytes\n", textsize);
    // printf("output: %ld bytes\n", codesize);
    // printf("output/input: %.3f\n", (double)codesize / textsize);
}

/*
static void Decode(void)  // Decoding/Uncompressing
{
    int16  i, j, k, r, c;
    uint32          count;

	if (fread(&textsize, sizeof textsize, 1, infile) < 1)
        Error("Unable to read");  // read size of original text
	if (textsize == 0)
		return;
	StartHuff();
	for (i = 0; i < N - F; i++)
		text_buf[i] = ' ';
	r = N - F;
	for (count = 0; count < textsize; ) {
		c = DecodeChar();
		if (c < 256) {
			putc(c, outfile);
			text_buf[r++] = c;
			r &= (N - 1);
			count++;
		} else {
			i = (r - DecodePosition() - 1) & (N - 1);
			j = c - 255 + THRESHOLD;
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				putc(c, outfile);
				text_buf[r++] = c;
				r &= (N - 1);
				count++;
			}
		}
		if (count > printcount) {
            //printf("%12ld\r", count);
			printcount += 1024;
		}
	}
    //printf("%12ld\n", count);
}


int main(int argc, char *argv[])
{
	char  *s;

	if (argc != 4) {
		printf("Usage:lzhuf e(compression)|d(uncompression)"
			" infile outfile\n");
		return EXIT_FAILED;
	}
	if ((s = argv[1], s[1] || strpbrk(s, "DEde") == NULL)
	 || (s = argv[2], (infile  = fopen(s, "rb")) == NULL)
	 || (s = argv[3], (outfile = fopen(s, "wb")) == NULL)) {
		printf("$@HHH(J %s\n", s);
		return EXIT_FAILED;
	}
	if (toupper(*argv[1]) == 'E')
		Encode();
	else
		Decode();
	fclose(infile);
	fclose(outfile);
	return EXIT_OK;
}
*/

//------------------------- >8 cut here if you do not want this 8< -----------------------------------------------------



int LZHExpandBlock(uint8 *in, uint8 *out, int16 size, int sector)
{
    // this code based on the above Decode function.  It does what the commented out Decode function does,
    // except that it does it from a memory block.
    int16  i, j, k, r, c;
    uint32          count;

    if (in==NULL || out==NULL)        return -1;

    FilePosition=in;
    EndOfFilePosition=&in[size];
    EndOfPutFilePosition=&out[20960];
    PutFilePosition=out;

    textsize = 20960;// size of DART block - ignore size param.
    codesize = 0; printcount = 0;


    //StartHuff();   //RA20060915 - speedup hack
    restore_huff();
    for (i = 0; i < N - F; i++)    text_buf[i] = ' ';
	r = N - F;
	for (count = 0; count < textsize; ) {
		c = DecodeChar();
		if (c < 256) {
			putc(c, outfile);
			text_buf[r++] = c;
			r &= (N - 1);
			count++;
		} else {
			i = (r - DecodePosition() - 1) & (N - 1);
			j = c - 255 + THRESHOLD;
			for (k = 0; k < j; k++) {
				c = text_buf[(i + k) & (N - 1)];
				putc(c, outfile);
				text_buf[r++] = c;
				r &= (N - 1);
				count++;
			}
		}
	}
return 0;
}


// RA20060915
static void save_huff(void)
{
 if (!saved_lson )  {saved_lson = malloc((N +   1) *sizeof(int16));  memcpy(saved_lson, lson  ,(N +   1) *sizeof(int16) );  }
 if (!saved_rson )  {saved_rson = malloc((N + 257) *sizeof(int16));  memcpy(saved_rson, rson  ,(N + 257) *sizeof(int16) );  }
 if (!saved_dad  )  {saved_dad  = malloc((N +   1) *sizeof(int16));  memcpy(saved_dad , dad   ,(N +   1) *sizeof(int16) );  }
 if (!saved_freq )  {saved_freq = malloc((T + 1)   *sizeof(uint16)); memcpy(saved_freq, freq  ,(T + 1)   *sizeof(uint16));  }
 if (!saved_son  )  {saved_son  = malloc((T)       *sizeof(int16));  memcpy(saved_son , son   ,(T)       *sizeof(int16) );  }
 if (!saved_prnt )  {saved_prnt = malloc((T+N_CHAR)*sizeof(int16));  memcpy(saved_prnt, prnt  ,(T+N_CHAR)*sizeof(int16));  }
}

// RA20060915
static void free_huff(void)
{

 if (saved_lson )  {free(saved_lson ); saved_lson =  NULL; }
 if (saved_rson )  {free(saved_rson ); saved_rson =  NULL; }
 if (saved_dad  )  {free(saved_dad  ); saved_dad  =  NULL; }
 if (saved_freq )  {free(saved_freq ); saved_freq =  NULL; }
 if (saved_son  )  {free(saved_son  ); saved_son  =  NULL; }
 if (saved_prnt )  {free(saved_prnt ); saved_prnt =  NULL; }
}

// RA20060915
static void restore_huff(void)
{
 // if we haven't built it yet, build it.
 if (!saved_lson ||
     !saved_rson ||
     !saved_son  ||
     !saved_dad  ||
     !saved_freq ||
     !saved_prnt   )
      {
         StartHuff();
      // InitTree();     // only used for encoding and we're not implementing that (yet)?
         save_huff();
         return;
      }

 memcpy(lson  ,saved_lson, (N +   1) *sizeof(int16) );
 memcpy(rson  ,saved_rson, (N + 257) *sizeof(int16) );
 memcpy(dad   ,saved_dad , (N +   1) *sizeof(int16) );
 memcpy(freq  ,saved_freq, (T + 1)   *sizeof(uint16));
 memcpy(son   ,saved_son , (T)       *sizeof(int16) );
 memcpy(prnt  ,saved_prnt, (T+N_CHAR)*sizeof(int16));
}





#endif



/*
diskName (+000): 64 Bytes
A Pascal String containing the name of the disk. This field takes 64 bytes regardless of the length of the String.

dataSize (+064): Rev. Long
The number of bytes (not blocks) of user data. User data is the 512 bytes of each block that a normal block-reading command returns.

tagSize (+068): Rev. Long
The number of bytes of tag data. Tag data is the extra 12 bytes of "scavenger" information present on 400K and 800K Macintosh disks. Apple II operating systems always leave these bytes zeroed, and they're not present on 720K or 1440K disks. If there are no tag bytes, this field will be zero.

dataChecksum (+072): Checksum
Checksum of all the user data on the disk. The checksum algorithm is called for the entire disk, not on a block-by-block or sector-by-sector basis. This is in Reverse order (most significant byte first).

tagChecksum (+076): Checksum
Checksum of all the tag data on the disk. If there is no tag data, this should be zero. This is in Reverse order (most significant byte first).


diskFormat (+080): Byte
0 = 400K
1 = 800K
2 = 720K
3 = 1440K (all other values are reserved)

formatByte (+081): Byte
$12 = 400K
$22 = >400K Macintosh (DiskCopy uses this value for all Apple II disks not 800K in size, and even for some of those)
$24 = 800K Apple II disk

private: (+082): Rev. Word
Must be $0100. If this field is not $0100, the file may be in a different format.

userData (+084): dataSize Bytes
The data blocks for the disk. These are in order from block zero through the end of the disk.

tagData (+xxx): tagSize Bytes
The tag data for this disk, starting with the tag data for the first block and proceeding in order. This field is not present for 720K and 1440K disks, but it is present for all other formats even if all the data is zeroes.


 */

