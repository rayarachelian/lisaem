/**************************************************************************************\
*                                     LibDC42                                          *
*                                                                                      *
*                            Version 0.9.6  2007.04.11                                 *
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
*           Contact me if you need other licensing options such as LGPL, etc.          *
*                                                                                      *
*           LICENSING NOTE: Portions of this code rely on the LZHUFF.C code            *
*                           released to usenet under the following conditions.         *
*                                                                                      *
*                                                                                      *
*       LZHUF.C (c)1989 by Haruyasu Yoshizaki, Haruhiko Okumura, and Kenji Rikitake.   *
*            All rights reserved. Permission granted for non-commercial use.           *
*                                                                                      *
*       If your application does not meet the non-commercial use clause, you must      *
*       remove the #define USE_LZHUFF line from this library, and you may not use      *
*       the LZHUFF routines.  Sorry.                                                   *
* ------------------------------------------------------------------------------------ *
*                                                                                      *
*        Routines for access and manipulation of Disk Copy 4.2 Disk Images.            *
*                                                                                      *
* This code is unaffiliated with Apple, and Apple retains all trade marks for itself,  *
* and all versions of the Disk Copy and DART programs.  Whenever you see the text      *
* Disk Copy or DART, you are encouraged to visualize a trademark symbol following it.  *
*                                                                                      *
* ************************************************************************************ *
*                                                                                      *
* Requirements:                                                                        *
*                                                                                      *
* This code currently requires OS support for POSIX memory mapping.  i.e. mmap.  Your  *
* OS must support this, for these functions to work. Future versions might remove this *
* requirement depending on requested needs of this library's users.                    *
*                                                                                      *
* A side effect of this is that your application may potentially use more memory if    *
* you attempt to use very large disk images (i.e. it might use 20MB of real RAM if your*
* application accesses the entire space of a 20MB disk image.                          *
*                                                                                      *
* ************************************************************************************ *
*                                                                                      *
* The word image refers to a disk image, not a picture or graphic.  In this context,   *
* disk image is a single file containing all of the sectors and associated metadata for*
* a storage medium - i.e. a floppy disk.                                               *
*                                                                                      *
*                                                                                      *
* These routines are file system agnostic, that is, they don't care what file system   *
* the disk images hold.  They allow you to use the DC42 format as a container for disk *
* images.  This would make it useful for various emulators of Apple hardware.          *
*                                                                                      *
* Note that this library can be used to create hard drive images as well, but that they*
* are incompatible with the real Disk Copy program.                                    *
*                                                                                      *
* DC42 images are ignorant of the physical layout of their media.  That is they only   *
* know the number of sectors within the container, not the number of sides, tracks, or *
* sectors per track of a specific storage device/media.  You may need to translate     *
* from head/track/sector to absolute sector depending on your (emulator's) needs.      *
*                                                                                      *
* These routines were created from publically released information about Apple's Disk  *
* Copy and DART programs from code and Apple IIgs tech notes found on the web along    *
* with some experimentation using disk images.  Experimental support for coverting     *
* DART disk images to DC42 images is included, but there is not support for LZH        *
* compressed DART images (i.e. Fast compression is supported, Best is not.)            *
*                                                                                      *
*                                                                                      *
* see  http://www.nulib.com/library/FTN.e00005.htm                                     *
*      http://web.pdx.edu/~heiss/technotes/ftyp/ftn.e0.0005.html                       *
*      http://ftp.uma.es/Mac/Mirrors/Stairwair/source/mungeimage-120-source.sit.bin    *
*      http://developer.apple.com/technotes/tn/tn1023.html                             *
*                                                                                      *
*      Above URL's are valid as of the time of this writing and contain Apple //       *
*      technical notes, and source code that access DC42/DART images.                  *
*                                                                                      *
*                                                                                      *
* WARNINGS:                                                                            *
*                                                                                      *
* Note that modern versions of Apple's Disk Copy program do not support tags, which    *
* the Lisa computer requires for proper operation.  It is unwise to convert DART disk  *
* images using modern versions of Disk Copy (i.e. 6.x) as this strips off all tags.    *
*                                                                                      *
* It's unswise to mount DiskCopy 4.2 images on a modern Mac, even if the image is Mac  *
* formatted without having a backup.  Note that some older Mac formats cannot be read  *
* by modern Mac's.  i.e if they're MFS instead of HFS. (This is circa Mac System 5 and *
* earlier.  MFS formatted disks are read-only accessible from under System 7, but not  *
* sure if OS X or System 9 can access them.)                                           *
*                                                                                      *
* If you wish to convert DART disk images to Disk Copy 4.2 images, you will need an    *
* old Macintosh with a GCR capable floppy drive, copies of both DART and Disk Copy 4.2.*
* You will have to restore the DART image to a real floppy using DART, then creating a *
* new image using the real Disk Copy 4.2 program.                                      *
*                                                                                      *
* You may also use the routine in this library to convert from DART to DiskCopy42      *
* format, but this only works for DART images compressed using RLE, and is experimental*
* at this time.  The safest way to convert DART images is by using real Mac hardware   *
* and the real DART and Disk Copy 4.2 applications.                                    *
*                                                                                      *
* These routines are experimental, so always excercise caution and make proper backups *
* of your disk images before allowing them to access your disk images.                 *
*                                                                                      *
* ------------------------------------------------------------------------------------ *
*                                                                                      *
* Usage of these routines:                                                             *
*                                                                                      *
* Allocate a DC42ImageType structure for each disk image you wish to have open.  Pass a   *
* pointer to this structure to the function calls that require it.                     *
*                                                                                      *
* Some of the routines do not access an opened DC42ImageType structure, and may only be   *
* used with the image file in the closed state.  No checking is done, so if you ignore *
* this warning, you'll find the image may be corrupted.                                *
*                                                                                      *
* These are: dc42_create, dart_to_dc42 and dc42_add_tags                               *
*                                                                                      *
* The operations are:                                                                  *
*                                                                                      *
* open - opens a disk image (and uses mmap to map in memory.)                          *
* close - closes a disk image and option saves changes made to it and the new checksums*
*         back to the disk image.                                                      *
*                                                                                      *
* read sector - read a sector                                                          *
* read tag    - read the tag data for a specific sector                                *
* write sector - write the sector (may be discarded depending on open mode)            *
* write tag    - write tag data to a sector (may be discarded depending on open mode)  *
*                                                                                      *

Note that these routines are not guaranteed to be thread-safe.  Use mutext locks around calls to them, or limit
access to a specific disk image access to a single thread.

You can open as many disk images as you'd like since each one will have their own DC42ImageType struct.

If you choose not to use the memory mapped, or RAM modes, the routines will allocate a buffer large enough for a single
sector and its tags.  So you must handle the data immediately.

If you use either the memory mapped or RAM modes, it is fairly safe, but not recommended to reuse the pointers to the
data and pass them around.  This is not advised, however.

-----------------------------------------------------------------------------------------------------------------------------------

int dc42_auto_open(DC42ImageType *F, char *filename, char *options)                // open a disk image, or a DART image.  If the
                                                                                   // image is a DART image, it will convert it,
                                                                                   // creating a new DC42 image, and then opens the
                                                                                   // DC42 image.
  or

int dc42_open(DC42ImageType *F,char *filename,char *options)                       // open a disk image, map it and fill structure

  or

int dc42_open_by_handle(DC42ImageType *F, int fd, FILE *fh, long seekstart, char *options)
                                                                                   // open an embedded dc42 image at the current
                                                                                   // location in the file (i.e. lseek).  Useful for
                                                                                   // embedded disk images.  i.e. inside a MacBinII
                                                                                   // wrapper.  Note that the previously opened file
                                                                                   // descriptor should match the permissions you use.
                                                                                   // i.e. if you use 'w', the fd must have been opened
                                                                                   // in read/write mode, not read only!


F: pointer to DC42ImageType - you must allocate this yourself before calling the open function.  You'll need to pass the pointer
                      to this structure to most of these calls.

filename: path/filename to the disk image.

fd: file descriptor.  For the _by_handle version of the function.  Allows you to open a disk image that's embedded inside another
                      file.  For example, one that is inside a MacBinaryII wrapper.  Note that you must call lseek to the position
                      of the start of the DC42 image before calling dc42_open_by_handle.  You must use the _by_handle version of the
                      close function, and then close the file descriptor yourself.  Of course, you should never close the file
                      descriptor before calling the close function.

                      If the file containing the DC42 image has checksums or CRC's it is your responsability to recalculate and
                      update those after closing the DC42 image.

                      When you issue the open call to get the file descript, you must ensure that it matches the permissions you
                      ask for in the options to dc42_open_by_handle.   i.e. if you use 'w', the fd must have been opened in r/w
                      mode, not read only!

start: for the open_by_handle call.  This is the offset into the file to the DC42 image.  If 0, will use the current position

options: string containing any of the following (last option takes precedence)

 r=read only        - attempts to write return an error.
 w=read/write       - normal read/write access
 p=private writes   - writes not written back to disk, they are kept in RAM to fool emulators however, will be lost
                      when the image is closed. If you change your mind and want to keep changes, you should create a identically
                      sized image, and copy all the data to it, then delete the original, and rename the new one to have the same
                      file name.)

                      This option will only work if you choose memory mapped I/O or RAM. i.e. use either 'b' or 'a' to guarantee
                      that it will work.  If you use 'm' and your OS doesn't have mmap/mmsync/mmunmap functions, it will fail.
                      If you use 'n', it will always fail.

 m=memory mapped    - use memory mapped I/O if available to your OS, otherwise, use just plain disk I/O (same as 'n').
 n=never use memory - never use mmapped I/O, nor RAM.  Suitable for systems that are low on memory.
 a=always in RAM.   - disk image will always remain in memory, we'll manage it ourselves, even if we have mmapped I/O available.
 b=best choice      - use the best choice available for speed.   if we have mmapped I/O in the OS, use that, otherwise load the
                      the whole image in RAM.


 If you want to simulate the 'p' option on a low memory system, you should first copy the disk image to a temporary file, then
 open the temporary file with "wn".  After you close the temporary file, delete it.




int dc42_close_image(DC42ImageType *F);

  or

int dc42_close_image_by_handle(DC42ImageType *F)



These functions close the disk image and if it was requested on the call, will write the data back to the disk, updating the
checksums.

Danger! You must call the appropriate function.  If you used dc42_open_by_handle, you must call dc42_close_image_by_handle.





int dc42_create(char *filename, char *volname, uint32 datasize, uint32 tagsize)    // create a blank new disk image
                                                                                   // sizes are in bytes.  pass *1024 for KB.
                                                                                   // does not open the image, only creates it, so
                                                                                   // call open if you want to open it after creating it.

uint8 *dc42_read_sector_tags(DC42ImageType *F, uint32 sectornumber);               // read a sector's tag data
uint8 *dc42_read_sector_data(DC42ImageType *F, uint32 sectornumber);               // read a sector's data
int dc42_write_sector_tags(DC42ImageType *F, uint32 sectornumber, uint8 *tagdata); // write tag data to a sector
int dc42_write_sector_data(DC42ImageType *F, uint32 sectornumber, uint8 *data);    // write sector data to a sector

   read calls return a pointer to the sector's data or tag.  Note that if you're not using mmapped I/O or RAM, the pointer to
   the data will be re-used between calls, and therefore old pointers from previous reads cannot be considered to be valid!

   write will write the data you provide to the image.  Write returns any error numbers.

   You can access F->errormsg and F->retval to get the results of the requested operation.


These next functions will NOT actually open a disk image, but will open a file handle to them.

uint32 dc42_has_tags(DC42ImageType *F);                                            // returns 0 if no tags, 1 if it has tags

uint32 dc42_calc_tag_checksum(DC42ImageType *F);                                   // calculate the current tag checksum

uint32 dc42_calc_data_checksum(DC42ImageType *F);                                  // calculate the current sector data checksum

int dc42_is_valid_image(char *filename);                                           // returns 0 if it can't access the file, or
                                                                                   // the image is not a valid dc42 image. 1 otherwise.

int dart_is_valid_image(char *dartfilename);                                       // returns 0 if it can't access the file, or
                                                                                   // the image is not a valid DART image.  1 otherwise.

int dart_to_dc42(char *dartfilename, char *dc42filename);                          // convert a DART file to a DC42 file.  It will
                                                                                   // open the DC42 image for you, only convert.

return codes:

 open:
 86=could not open image
 88=image is not likely a DiskCopy 4.2 file
 89=size of the physical disk image file does not match what the image size claims

 -1 null DC42ImageType
 -2 no sectors
 -3 image not in RAM/not allocated or mmaped
 -4 no tag data in this image
 -5 file descriptor is negative (file not open?)
 -6 could not open file for reading
 -7 could not create file

 -9 get EOF too soon in dart image
 -10 wrong file type (not dart)

\**************************************************************************************************************************/


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
extern char *strcasestr(const char *haystack, const char *needle);
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
int dc42_auto_open(DC42ImageType *F, char *filename, char *options);               // oops, was missing!

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
uint32 dc42_calc_tag0_checksum(DC42ImageType *F);                                  // used by DART

uint32 dc42_calc_data_checksum(DC42ImageType *F);                                  // calculate the current sector data checksum

int dc42_recalc_checksums(DC42ImageType *F);                                       // calculate checksums and save'em in the image

int dc42_check_checksums(DC42ImageType *F);                                        // 0 if both data and tags match
                                                                                   // 1 if tags don't match
                                                                                   // 2 if data
                                                                                   // 3 if both data and tags don't match


uint32 dc42_get_tagchecksum(DC42ImageType *F);                                     // return the image's stored tag checksum
uint32 dc42_get_datachecksum(DC42ImageType *F);                                    // return the image's stored data checksum





int dart_to_dc42(char *dartfilename, char *dc42filename);                          // converts a DART fast-compressed/uncompressed
                                                                                   // image to a DiskCopy42 image.  Does not (yet)
                                                                                   // work with LZH compressed images


int dc42_is_valid_image(char *filename);    // returns 0 if it can't open the image, or the image is not a valid dc42 image.

int dart_is_valid_image(char *dartfilename); // returns 0 if it can't open the image, or the image is not a valid DART image

int dc42_is_valid_macbinii(char *infilename, char *creatortype);                   // returns 1 if file is macbinII encapsulated
                                                                                   // if creatortype is passed a non-NULL ponter
                                                                                   // the Macintosh creator and type are returned
                                                                                   // by thefunction.  This must be at leat 9 bytes!

int dc42_set_volname(DC42ImageType *F,char *name); // set/get the disk image volume name
char *dc42_get_volname(DC42ImageType *F);



int dc42_extract_macbinii(char *infilename);                                       // extracts macbin2 header if one exists
                                                                                   // returns 1 if converted, 0 if it's not
                                                                                   // a macbinII header. negative on error
                                                                                   // NOTE: filename is overwritten with
                                                                                   // extracted file name!  On negative return
                                                                                   // the filename has been altered!

int searchseccount( DC42ImageType *F, int sector, int size, uint8 *s);
int replacesec(DC42ImageType *F, int sector, int size, uint8 *s, uint8 *r);

////////////// headers ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
