/**************************************************************************************\
*                                     LibDC42                                          *
*                                                                                      *
*                       A Part of the Lisa Emulator Project                            *
*                                                                                      *
*                  Copyright (C) 2025 Friends of Ray Arachelian                        *
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
*                                                                                      *
*        Routines for access and manipulation of "raw" Apple Profile and Widget        *
*        hard disk image files, like the ones used by popular hardware emulators:      *
*        IDEFile, ESProfile, Cameo/Aphid and probably other future ones.               *
*                                                                                      *
*        The file format is: a set of 532-byte "sectors", where                        *
*        each sector has 20 bytes of tag data followed by 512 bytes of sector data.    *
*        The sectors are stored in a 5:1 interleave order, see                         *
*        function interleave5() below for details.                                     *
*                                                                                      *
*        The files usually have a ".image" file extension, but other extensions        *
*        can be used as well.                                                          *
*                                                                                      *
*        There is no way to validate that a given file is indeed of this type.          *
*                                                                                      *
*        A typical 5MB Profile hard disk image is of file size 5,176,320 bytes,        *
*        which has 9,729 sectors (of size 512+20) bytes, and 492 unused bytes          *
*        at the end (a file with 0 unused bytes would work just as good).              *
*                                                                                      *
*        A lot of the code here has been adapted from libdc42.c.                       *
*                                                                                      *
\**************************************************************************************/

// needed for LisaEm compatibility, you can remove this, but you must
// define int8, int16, int32, uint8, uint16, uint32.
#include <machine.h>

#include "libdc42.h"
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
#endif

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#ifndef fgetc
#define fgetc(xx) ((unsigned)(getc(xx)))
#endif

// Here we reuse the macros found in libdc42.c for error checking and returning. See some relevant comments there.
#define RAW_PROFILE_RET_CODE(F, code, msg, ret) \
   {                                     \
      if (F)                             \
      {                                  \
         F->retval = code;               \
         F->errormsg = msg;              \
         ret;                            \
      }                                  \
   }


#define RAW_PROFILE_CHECK_VALID_F(F)                                                                      \
   {                                                                                               \
      if (!F)                                                                                      \
         return -1;                                                                                \
      if (!F->RAM)                                                                                 \
      {                                                                                            \
         RAW_PROFILE_RET_CODE(F, -3, "Disk image closed or no memory allocated to it", return F->retval); \
      }                                                                                            \
      if (F->fd < 3 && !F->fh)                                                                     \
      {                                                                                            \
         RAW_PROFILE_RET_CODE(F, -5, "Invalid file descriptor", return F->retval);                        \
      }                                                                                            \
   }

#define RAW_PROFILE_CHECK_VALID_F_NUL(F)                                                             \
   {                                                                                          \
      if (!F)                                                                                 \
         return NULL;                                                                         \
      if (!F->RAM)                                                                            \
         RAW_PROFILE_RET_CODE(F, -3, "Disk image closed or no memory allocated to it", return NULL); \
      if (F->fd < 3 && !F->fh)                                                                \
         RAW_PROFILE_RET_CODE(F, -5, "Invalid file descriptor", return NULL);                        \
   }

#define RAW_PROFILE_CHECK_VALID_F_ZERO(F)                                                         \
   {                                                                                       \
      if (!F)                                                                              \
         return 0;                                                                         \
      if (!F->RAM)                                                                         \
         RAW_PROFILE_RET_CODE(F, -3, "Disk image closed or no memory allocated to it", return 0); \
      if (F->fd < 3 && !F->fh)                                                             \
         RAW_PROFILE_RET_CODE(F, -5, "Invalid file descriptor", return 0);                        \
   }

#define RAW_PROFILE_CHECK_WRITEABLE(F)                                        \
   {                                                                   \
      if (F->readonly)                                                 \
         RAW_PROFILE_RET_CODE(F, -8, "Image is Read Only", return F->retval); \
   }
#define RAW_PROFILE_CHECK_WRITEABLE1(F)                                       \
   {                                                                   \
      if (F->readonly == 1)                                            \
         RAW_PROFILE_RET_CODE(F, -8, "Image is Read Only", return F->retval); \
   }


// like fsync, sync's writes back to the file.
int raw_profile_sync_to_disk(DC42ImageType *F) 
{
   RAW_PROFILE_CHECK_VALID_F(F);
   RAW_PROFILE_CHECK_WRITEABLE(F);

#ifdef HAVE_MMAPEDIO
   if (F->mmappedio == 1)
      msync(F->RAM, F->size, MS_SYNC);
#endif

   if (F->mmappedio == 2 && F->readonly == 0)
   {
      int i;

      if (F->fd > 2)
      {
         lseek(F->fd, 0, SEEK_SET);
         i = write(F->fd, F->RAM, F->size);        // save the whole file
      }

      if (F->fh)
      {
         fseek(F->fh, 0, SEEK_SET);
         i = fwrite(F->RAM, F->size, 1, F->fh);    // save the whole file
      }

   }

   if (F->fh)
      fflush(F->fh);
#ifndef __MSVCRT__
   if (F->fd)
      fsync(F->fd);
#else
   if (F->fd)
      _commit(F->fd);
#endif

   return 0;
}

/**
What is "5:1" interleaving?
The original ProFile hard disk drive used a 5:1 interleaving scheme to optimize data retrieval times:
By the time the Lisa processed the sector it just read, the Profile disk platter has moved few sectors further along.
To avoid waiting for a nearly-full disk rotation to read the next sector, the sectors are interleaved. 
This optimization speeds up sequential reads.

For sector number  0: interleave(0)=0 
For sector number  1: interleave(1)=5
For sector number  2: interleave(2)=10 
For sector number  3: interleave(3)=15
For sector number  4: interleave(4)=4
For sector number  5: interleave(5)=9 
For sector number  6: interleave(6)=14
For sector number  7: interleave(7)=3 
For sector number  8: interleave(8)=8 
For sector number  9: interleave(9)=13 
For sector number 10: interleave(10)=2 
For sector number 11: interleave(11)=7 
For sector number 12: interleave(12)=12 
For sector number 13: interleave(13)=1 
For sector number 14: interleave(14)=6 
For sector number 15: interleave(15)=11 
For sector number 16: interleave(16)=16 
For sector number 17: interleave(17)=21 
For sector number 18: interleave(18)=26 
For sector number 19: interleave(19)=31 
For sector number 20: interleave(20)=20
... etc ...
 */
long interleave5(long sector)
{
  static const int offset_delta[] = {0, 4, 8, 12, 0, 4, 8, -4, 0, 4, -8, -4, 0, -12, -8, -4};
  return sector + offset_delta[(sector & 15)]; // "sector & 15" is an optimized version of "sector % 16"
}

// Gets the offset in the file (or in the F->RAM) where the tag data for this sectornumber is stored
// The file format is: a set of sectors, where each sector has 20-bytes tag data followed by 512 bytes sector data. Sectors are interleaved in 5:1 order.
long get_tag_pos(DC42ImageType *F, long sectornumber) {
   long interleaved_sector = interleave5(sectornumber);
   long pos = interleaved_sector * (F->sectorsize + F->tagsize);
   //fprintf(stderr,"get_tag_pos for sector number:%d ; interleaved_sector:%d ; pos:%d\n", sectornumber, interleaved_sector, pos);
   return pos;
}

// Gets the offset in the file (or in the F->RAM) where the sector data for this sectornumber is stored
long get_data_pos(DC42ImageType *F, long sectornumber) {
   long interleaved_sector = interleave5(sectornumber);
   long pos = (interleaved_sector * (F->sectorsize + F->tagsize)) + F->tagsize; // the sector data is after the tag data
   //fprintf(stderr,"get_data_pos for sector number:%d ; interleaved_sector:%d ; pos:%d\n", sectornumber, interleaved_sector, pos);
   return pos;
}

int raw_profile_close_image(DC42ImageType *F)
{
   fprintf(stderr,"lib_raw_profile_image.c: Closing raw profile image file %s\n", F->fname);
   RAW_PROFILE_CHECK_VALID_F(F);
   raw_profile_sync_to_disk(F);

#ifdef HAVE_MMAPEDIO
   if (F->mmappedio == 1)
      munmap(F->RAM, F->size);
   else
#endif
       if (F->RAM)
      free(F->RAM);

   F->RAM = NULL; // decouple file and RAM, mark RAM as invalid

   if (F->fd > 2)
   {
      close(F->fd);
      F->fd = -1;
   } // close the file handle.

   if (F->fh)
   {
      fclose(F->fh);
      F->fh = NULL;
   }

   RAW_PROFILE_RET_CODE(F, 0, "Image Closed", return F->retval);
   return F->retval; // suppress dumb compiler warning
}

// Reads the tag data for the specified sectornumber, returns pointer to it
uint8 *raw_profile_image_read_sector_tags(DC42ImageType *F, uint32 sectornumber)
{
   // fprintf(stderr,"lib_raw_profile_image.c: Reading tags for sector number:%d\n", sectornumber);
   RAW_PROFILE_CHECK_VALID_F_NUL(F);
   F->retval = 0;
   F->errormsg = F->returnmsg;
   *F->returnmsg = 0;

   if (sectornumber >= F->numblocks)
   {
      RAW_PROFILE_RET_CODE(F, 999, "invalid sector #", return NULL);
   }

   if (F->mmappedio == 0)
   {
      int i;
      if (F->fd > 2)
      {
         lseek(F->fd, get_tag_pos(F, sectornumber), SEEK_SET);
         // The RAM buffer is for just 1 sector's data + tag data (sector data is first, then tag data)
         i = read(F->fd, &F->RAM[F->sectorsize], F->tagsize); // put tag data after the sector data
         // fprintf(stderr,"fd-read tag %4d at loc:%08x PTR:%p\n",sectornumber,get_tag_pos(sectornumber),F);
      }
      if (F->fh)
      {
         fseek(F->fh, get_tag_pos(F, sectornumber), SEEK_SET);
         i = fread(&F->RAM[F->sectorsize], F->tagsize, 1, F->fh); // put tag data after the sector data
         // fprintf(stderr,"fh-read tag %4d at loc:%08x PTR:%p\n",sectornumber,get_tag_pos(sectornumber),F);
      }

      return &F->RAM[F->sectorsize]; // with reads of sectors.
   }

   // otherwise it's either mmapped or fully in RAM, either way, access to them is the same.
   // fprintf(stderr,"mem-read tag %4d at loc:%08x PTR:%p\n",sectornumber,get_tag_pos(sectornumber),F);
   return &F->RAM[get_tag_pos(F, sectornumber)];
}

// Reads the sector data for the specified sectornumber, returns pointer to it
uint8 *raw_profile_image_read_sector_data(DC42ImageType *F, uint32 sectornumber)
{
   // fprintf(stderr,"lib_raw_profile_image.c: Reading data for sector number:%d\n", sectornumber);
   RAW_PROFILE_CHECK_VALID_F_NUL(F)
   F->retval = 0;
   F->errormsg = F->returnmsg;
   *F->returnmsg = 0;

   if (sectornumber >= F->numblocks)
   {
      RAW_PROFILE_RET_CODE(F, 999, "invalid sector #", return NULL);
   }

   if (!F->mmappedio)
   {
      int i;
      if (F->fd > 2)
      {
         lseek(F->fd, get_data_pos(F, sectornumber), SEEK_SET);
         i = read(F->fd, F->RAM, F->sectorsize);
         // fprintf(stderr,"fd-read data %4d at loc:%08x PTR:%p\n",sectornumber,get_data_pos(sectornumber),F);
      }

      if (F->fh)
      {
         fseek(F->fh, get_data_pos(F, sectornumber), SEEK_SET);
         i = fread(F->RAM, F->sectorsize, 1, F->fh);
         // fprintf(stderr,"fh-read data %4d at loc:%08x PTR:%p\n",sectornumber,get_data_pos(sectornumber),F);
      }

      return F->RAM;
   }

   //   fprintf(stderr,"Returning code:%d string:%s\n",F->retval,F->errormsg); fflush(stderr);
   //   fprintf(stderr,"mem-read data %4d at loc:%08x PTR:%p\n",sectornumber,get_data_pos(sectornumber),F);
   return &F->RAM[get_data_pos(F, sectornumber)];
}

// Writes the sector data for the specified sectornumber to the file
int raw_profile_image_write_sector_data(DC42ImageType *F, uint32 sectornumber, uint8 *data)
{
   // fprintf(stderr,"lib_raw_profile_image.c: Writing data for sector number:%d\n", sectornumber);
   RAW_PROFILE_CHECK_VALID_F(F);
   RAW_PROFILE_CHECK_WRITEABLE1(F);
   F->retval = 0;
   F->errormsg = F->returnmsg;
   *F->returnmsg = 0;

   if (sectornumber >= F->numblocks)
   {
      RAW_PROFILE_RET_CODE(F, 999, "invalid sector #", return 999);
   }

   if (!F->mmappedio)
   {
      int i;
      //{fprintf(stderr,"lib_raw_profile_image.c::wrote DIRECT! to block#%ld, returning:%ld\n",sectornumber, F->retval); fflush(stderr);}
      if (F->fd)
      {
         lseek(F->fd, get_data_pos(F, sectornumber), SEEK_SET);
         i = write(F->fd, data, F->sectorsize);

         // fprintf(stderr,"fd-write data %4d at loc:%08x PTR:%p\n",sectornumber,get_data_pos(sectornumber),F);
      }
      if (F->fh)
      {
         fseek(F->fh, get_data_pos(F, sectornumber), SEEK_SET);
         i = fwrite(data, F->sectorsize, 1, F->fh);
         // fprintf(stderr,"fh-write data %4d at loc:%08x PTR:%p\n",sectornumber,get_data_pos(sectornumber),F);
      }

      RAW_PROFILE_RET_CODE(F, 0, "Sector Written", return F->retval);
   }

   // fprintf(stderr,"mem-write data %4d at loc:%08x  PTR:%p\n",sectornumber,get_data_pos(sectornumber),F);
   memcpy(&F->RAM[get_data_pos(F, sectornumber)], data, F->sectorsize);

   if (F->synconwrite && F->readonly == 0)
      raw_profile_sync_to_disk(F);

   //{fprintf(stderr,"lib_raw_profile_image.c::wrote to block#%ld, returning:%ld\n",sectornumber, F->retval); fflush(stderr);}

   RAW_PROFILE_RET_CODE(F, 0, "Sector Written", return F->retval);
   return F->retval; // suppress dumb compiler warning, even though we never reach here
}

// Writes the tagdata for the specified sectornumber to the file
int raw_profile_image_write_sector_tags(DC42ImageType *F, uint32 sectornumber, uint8 *tagdata)
{
   // fprintf(stderr,"lib_raw_profile_image.c: Writing tags for sector number:%d\n", sectornumber);
   RAW_PROFILE_CHECK_VALID_F(F);
   RAW_PROFILE_CHECK_WRITEABLE1(F);
   F->retval = 0;
   F->errormsg = F->returnmsg;
   *F->returnmsg = 0;

   if (sectornumber >= F->numblocks)
   {
      RAW_PROFILE_RET_CODE(F, 999, "invalid sector #", return 999);
   }

   if (!F->mmappedio)
   {
      if (F->fd > 2)
      {
         int i;
         lseek(F->fd, get_tag_pos(F, sectornumber), SEEK_SET);
         i = write(F->fd, tagdata, F->tagsize);
         // fprintf(stderr,"fd-write tag %4d at loc:%08x PTR:%p\n",sectornumber,get_tag_pos(sectornumber),F);
      }

      if (F->fh)
      {
         int i;
         fseek(F->fh, get_tag_pos(F, sectornumber), SEEK_SET);
         i = fwrite(tagdata, F->tagsize, 1, F->fh);
         // fprintf(stderr,"fh-write tag %4d at loc:%08x PTR:%p\n",sectornumber,get_tag_pos(sectornumber),F);
      }

      return 0;
   }

   memcpy(&F->RAM[get_tag_pos(F, sectornumber)], tagdata, F->tagsize);

   if (F->synconwrite && F->readonly == 0)
      raw_profile_sync_to_disk(F);
   //{fprintf(stderr,"lib_raw_profile_image.c::wrote %d tag bytes to block#%ld, returning:%ld\n",F->tagsize, sectornumber, F->retval); fflush(stderr);}
   RAW_PROFILE_RET_CODE(F, 0, "Sector Tag Written", return F->retval);
   return F->retval; // suppress dumb compiler warning
}

// Called from profile.c to open a raw ProFile/Widget hard disk image file; it calls it with options="wb" (w=open in read/write mode, b=make best choice for mmapped I/O or RAM)
int raw_profile_image_open(DC42ImageType *F, char *filename, char *options)
{
   fprintf(stderr,"lib_raw_profile_image.c: Starting in raw_profile_image_open for filename:%s with options:%s\n",filename,options);
   fflush(stderr);

   int i, flag;
   long filesizetotal = 0; // total size of the file in bytes
   F->retval = 0; // success by default
   F->returnmsg[0] = 0;

   // "configure" the function pointers in DC42ImageType that are used for reading/writing sectors/tags and for closing the image file.
   // to point to the functions defined in this file. They are being invoked from profile.c
   F->read_sector_tags = raw_profile_image_read_sector_tags;
   F->write_sector_tags = raw_profile_image_write_sector_tags;
   F->read_sector_data = raw_profile_image_read_sector_data;
   F->write_sector_data = raw_profile_image_write_sector_data;
   F->close_image = raw_profile_close_image;
   F->close_image_by_handle = NULL; // not supported for raw profile images

   // copy the file name into the image structure for later use
   strncpy(F->fname, filename, FILENAME_MAX);

   // open the image file just to get its file size and close it; we'll re-open it again later if everything's happy.
#ifndef __MSVCRT__
   F->fd = open(F->fname, O_RDONLY);
   // why 3? because 0 is stdin, 1 is stdout, 2 is stderr.  Duh!
   if (F->fd < 3)
      RAW_PROFILE_RET_CODE(F, -6, "Cannot open the file.", return F->retval);
   F->fh = NULL;
   filesizetotal = lseek(F->fd, 0, SEEK_END);
   filesizetotal = lseek(F->fd, 0, SEEK_CUR);
   close(F->fd);
   F->fd = 0;
#else
   F->fh = fopen(F->fname, "rb");
   if (!F->fh)
      RAW_PROFILE_RET_CODE(F, -6, "Cannot open the file.", return F->retval);
   F->fd = 0;
   fseek(F->fh, 0, SEEK_END);
   filesizetotal = ftell(F->fh);
   fclose(F->fh);
   F->fh = NULL;
#endif

   F->size = filesizetotal;
   F->numblocks = filesizetotal/(512+24); // number of sectors in the image; files often have some extra padding at the end, which is unused.
   F->datasize = 512; // we don't support other sizes
   F->sectorsize = 512;
   F->tagsize = 20; // each sector has a 20-byte tag; we don't support other sizes
   F->datasizetotal = F->numblocks * F->sectorsize;
   F->tagsizetotal = F->numblocks * F->tagsize;

   fprintf(stderr,"lib_raw_profile_image.c: the file size is %ld bytes; numblocks is %ld.\n", F->size, F->numblocks);

   // We (and the rest of the Lisaem code) don't need/use these:
   F->dc42seekstart = -1;
   F->sectoroffset = -1;
   F->tagstart = -1;
   F->maxtrk = -1;
   F->maxsec = -1;
   F->maxside = -1;
   F->ftype = -1;

   F->readonly = 0; // read/write by default
   F->synconwrite = 0; // no sync on write by default

#ifdef HAVE_MMAPEDIO
   F->mmappedio = 1;
#else
   F->mmappedio = 0;
#endif

   // Parse options for opening image file
   while (*options)
   {
      //{fprintf(stderr,"parsing option %c\n",*options); fflush(stderr);}
      switch (tolower(*options))
      {
      case 'r':
         F->readonly = 1;
         break; // r=read only
      case 'w':
         F->readonly = 0;
         break; // w=read/write
      case 'p':
         F->readonly = 2;
         break; // p=private writes (not written back to disk, kept in RAM to fool emulator)

      case 'm': // m=memory mapped I/O if available, else just plain disk I/O
#ifdef HAVE_MMAPEDIO
         F->mmappedio = 1;
#else
         F->mmappedio = 0;
#endif
         break;

      case 'n':
         F->mmappedio = 0;
         break; // n=never use mmapped I/O, nor RAM.
      case 'a':
         F->mmappedio = 2;
         break; // a=always in RAM. manage it ourselves, even if we have mmapped I/O available
      case 'b': // b=best choice (for speed):
#ifdef HAVE_MMAPEDIO
         F->mmappedio = 1;
         break; //   if we have mmapped I/O in the OS, use that.
#else
         F->mmappedio = 2;
                  break; //   otherwise, load the whole image in RAM.
#endif

      case 's':
         F->synconwrite = 1;
         break;
      default:
      {
         int i = strlen(F->returnmsg);
         if (i < 80)
         {
            F->returnmsg[i] = *options;
            F->returnmsg[i + 1] = 0;
         }
      }
      }
      options++;
   }

   if (strlen(F->returnmsg) > 0 && strlen(F->returnmsg) < (80 - 14))
      strncat(F->returnmsg, " unknown opts", 80);

   if (F->readonly == 0)
   {
      // Open the image file in read/write mode
      if (F->fd > 2)
         close(F->fd); // close the file and reopen it as possibly writeable
      if (F->fh)
         fclose(F->fh);

#ifndef __MSVCRT__
      F->fd = open(F->fname, O_RDWR);
      if (F->fd < 3)
         RAW_PROFILE_RET_CODE(F, -86, "Cannot re-open file for writing", return F->retval);
      F->fh = NULL;
#else
      F->fh = fopen(F->fname, "r+b");
      if (!F->fh)
         RAW_PROFILE_RET_CODE(F, -86, "Cannot re-open file for writing", return F->retval);
      F->fd = 0;
#endif

#ifdef HAVE_MMAPEDIO
      if (F->mmappedio)
      {
         errno = 0;
         F->RAM = mmap(0, F->size, PROT_READ | PROT_WRITE, MAP_SHARED, F->fd, 0); // writeable image
         //{fprintf(stderr,"MMAPped writeable image errno:%d\n",errno); fflush(stderr);}
      }
#endif
   }
   else
   {
      // Open the image file in read-only mode
#ifdef HAVE_MMAPEDIO
      if (F->mmappedio)
      {
         errno = 0;
         F->RAM = mmap(0, F->size, PROT_READ | PROT_WRITE, MAP_PRIVATE, F->fd, 0); // read only/private  - fd is already opened rd only
         //{fprintf(stderr,"MMAPped READ-ONLY/PRIVATE image errno:%d\n",errno); fflush(stderr);}
      }
#endif
   }

   if (F->mmappedio == 0)
   {
      // Allocate RAM for just 1 sector's data + tag data (we'll store sector data first, then tag data)
      F->RAM = malloc((F->tagsize + F->sectorsize)); 
      fprintf(stderr,"Direct to disk image\n"); fflush(stderr);
   }
   if (F->mmappedio == 2) // "2" means "always use RAM, do not use mmapped I/O"
   {
      // Allocate as much  RAM as the file size
      F->RAM = malloc(F->size);
      if (F->RAM)
      {
         int i;
         if (F->fd > 2)
         {
            // Read the file into memory, on non-Windows OS:
            lseek(F->fd, 0, SEEK_SET);
            i = read(F->fd, F->RAM, F->size);
         }
         if (F->fh)
         {
            // Read the file into memory, on Windows:
            fseek(F->fh, 0, SEEK_SET);
            i = fread(F->RAM, F->size, 1, F->fh);
         }
      }
   }

   // oops! no ram, or mmap failed.
   if (!F->RAM || (F->RAM == (void *)(-1)))
   {
      if (F->fd > 2)
      {
         close(F->fd);
         F->fd = -1;
      }
      if (F->fh)
      {
         fclose(F->fh);
         F->fh = NULL;
      }

      RAW_PROFILE_RET_CODE(F, -99, "Could not mmap the file or allocate memory", return F->retval);
   }

   RAW_PROFILE_RET_CODE(F, 0, "Raw Profile image opened", return F->retval);
   return F->retval; // silence compiler warning about lack of return value
}
