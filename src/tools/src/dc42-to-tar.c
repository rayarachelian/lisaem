/**************************************************************************************\
*                                                                                      *
*           dc42-to-tar - Convert Xenix tar.dc42 floppies to a tar file                *
*                                                                                      *
*          Copyright (C) 2022, Ray A. Arachelian, All Rights Reserved.                 *
*              Released under the GNU Public License, Version 2.0                      *
*    There is absolutely no warranty for this program. Use at your own risk.           *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

unsigned char *tarball = NULL;
size_t memsize = 0;

typedef struct tarhdr
{
   char name[100];
   char mode[8];
   char uid[8];
   char gid[8];
   char size[12];
   char mtime[12];
   char chksum[8]; // calculated by sum of the unsigned byte values of the header record with the eight checksum bytes taken to be spaces (0x20)
                   // 6-digit octal leading zeroes followed by a NUL and a space. some tar implementations treated bytes as signed.
                   // Implementations typically calculate the checksum both ways, and treat it as good if either matches
   char link[1];   // '0' or NUL file, '1' 	Hard link, '2' 	Symbolic link
   char linkname[100];
   char padding[255];
} tarhdr;

tarhdr *get_tarhdr_ptr(tarhdr *t, size_t offset)
{
   char *c;

   c = (char *)((void *)(t));
   return (tarhdr *)(&c[offset]);
}

size_t get_tar_header_size(tarhdr *t, size_t offset)
{
   t = (tarhdr *)((void *)(t) + offset);

   char *s = t->size;
   char oct[18];
   memset(oct, 0, 17);
   oct[0] = '0';
   oct[1] = '0';
   strncpy(&oct[2], s, 12);

   if (t->link[0] == '1' || t->link[0] == '2')
   {
      fprintf(stderr, "%s:%s:%d file is a link size=0\n", __FILE__, __FUNCTION__, __LINE__);
      return 0;
   }

   for (int i = 2; i < 12; i++)
      if (oct[i] == ' ')
         oct[i] = '0';

#ifdef DEBUG
   fprintf(stderr, "%s:%s:%d size field :%12s:  oct string in:%s:  return: value:%08x (%ld)\n", __FILE__, __FUNCTION__, __LINE__, t->size, oct, strtoul(oct, NULL, 8), strtoul(oct, NULL, 8));
#endif
   return strtoul(oct, NULL, 8);
}

void print_header(tarhdr *t, size_t offset)
{
   char buf[102];
   t = get_tarhdr_ptr(t, offset);

   memset(buf, 0, 101);
   fprintf(stderr, "offset:%08x", offset);
   memcpy(buf, t->mode, 8);
   fprintf(stderr, "%8s  ", buf);
   memcpy(buf, t->uid, 8);
   fprintf(stderr, "%8s:", buf);
   memcpy(buf, t->gid, 8);
   fprintf(stderr, "%-8s   ", buf);
   fprintf(stderr, "%8ld ", get_tar_header_size(t, offset));
   memcpy(buf, t->name, 100);
   fprintf(stderr, "%s\n", buf);
}

size_t roundup512(size_t i)
{
   size_t j = i;
   if ((j & 0x1ff) != 0)
      j = (i + 512) & (0xffffe00);
   return j;
}

size_t get_cksum_field(tarhdr *t, size_t offset)
{
   char oct[18];
   t = get_tarhdr_ptr(t, offset);

   char *s = t->chksum;
   memset(oct, 0, 17);
   oct[0] = '0';
   oct[1] = '0';
   strncpy(&oct[2], s, 12);
   for (int i = 0; i < 17; i++)
      if (oct[i] == 0x20)
         oct[i] = 0x30; // replace spaces with zeros.
   return strtoul(oct, NULL, 8);
}

int is_null_header(tarhdr *t, size_t offset)
{
   long i = 0;
   unsigned char *c;
   t = get_tarhdr_ptr(t, offset);
   c = ((unsigned char *)&t->name[0]);
   for (i = 0; (size_t)i < sizeof(tarhdr); i++)
      if (*c != 0)
         return 0;
   return 1;
}

#ifdef DEBUG
char niceascii(unsigned char c)
{
   c &= 127;
   if (c < 31)
      c |= 32;
   if (c == 127)
      c = '.';
   return c;
}
#endif

int is_valid_header(tarhdr *t, size_t offset)
{
   int i, j, invalid = 0;
   t = get_tarhdr_ptr(t, offset);

#ifdef XXXDEBUG
   fprintf(stderr, "checking for valid header\n");
   for (i = 0; i < 256; i += 16)
   {
      fprintf(stderr, "%08x: ", offset + i);

      for (j = 0; j < 16; j++)
      {
         fprintf(stderr, "%02x ", (unsigned char)(t->name[i + j]));
      }

      fprintf(stderr, " | ");

      // :TODO: warning fix.
      // tar header is 512 bytes, name is only in the 1st 100 bytes, hence this error.
      // dc42-to-tar.c: In function ‘is_valid_header’:dc42-to-tar.c:138:83: warning: iteration 7 invokes undefined behavior [-Waggressive-loop-optimizations]
      // 138 |       for (j=0; j<16; j++) { fprintf(stderr,"%c",niceascii((unsigned char)(t->name[i+j])) ); }
      //    |                                                                           ~~~~~~~~^~~~~~dc42-to-tar.c:131:4: note: within this loop...
      //     t->name[100] does go over 116 chars at 7 maybe change this to (char *)(t)[q] since really we're printing the whole header and not the name
      for (j = 0; j < 16; j++)
      {
         int q = i + j;
         fprintf(stderr, "%c", niceascii((unsigned char)(t->name[q])));
      }
      fprintf(stderr, "\n");
   }
#endif

   for (i = 0; i < 8; i++)
      if ((t->mode[i] < '0' || t->mode[i] > '7') && (t->mode[i] != ' ' && t->mode[i] != 0))
      {
         invalid = 1; /* fprintf(stderr,"inv 1: offset:%ld %d (%c %d)\n",offset,i,t->mode[i],t->mode[i]);     */
      }
   for (i = 0; i < 12; i++)
      if ((t->size[i] < '0' || t->size[i] > '7') && (t->size[i] != ' ' && t->size[i] != 0))
      {
         invalid = 2; /* fprintf(stderr,"inv 2: offset:%ld %d (%c %d)\n",offset,i,t->size[i],t->size[i]);     */
      }
   for (i = 0; i < 12; i++)
      if ((t->mtime[i] < '0' || t->mtime[i] > '7') && (t->mtime[i] != ' ' && t->mtime[i] != 0))
      {
         invalid = 3; /* fprintf(stderr,"inv 3: offset:%ld %d (%c %d)\n",offset,i,t->mtime[i],t->mtime[i]);   */
      }
   for (i = 0; i < 8; i++)
      if ((t->uid[i] < '0' || t->uid[i] > '7') && (t->uid[i] != ' ' && t->uid[i] != 0))
      {
         invalid = 4; /* fprintf(stderr,"inv 4: offset:%ld %d (%c %d)\n",offset,i,t->uid[i],t->uid[i]);       */
      }
   for (i = 0; i < 8; i++)
      if ((t->gid[i] < '0' || t->gid[i] > '7') && (t->gid[i] != ' ' && t->gid[i] != 0))
      {
         invalid = 5; /* fprintf(stderr,"inv 5: offset:%ld %d (%c %d)\n",offset,i,t->gid[i],t->gid[i]);       */
      }
   for (i = 0; i < 8; i++)
      if ((t->chksum[i] < '0' || t->chksum[i] > '7') && (t->chksum[i] != ' ' && t->chksum[i] != 0))
      {
         invalid = 6; /* fprintf(stderr,"inv 6: offset:%ld %d (%c %d)\n",offset,i,t->chksum[i],t->chksum[i]); */
      }

   if (t->link[0] != '0' && t->link[0] != '1' && t->link[0] != '2' && t->link[0] != 0)
   {
      invalid = 7; /* fprintf(stderr,"inv 7: %d (%c %d)\n",t->link[0],t->link[0],i); */
   }

   return !invalid;
}

long tar_checksum1(tarhdr *h, size_t offset)
{
   unsigned long cksum = 0;
   int i;
   tarhdr n;
   h = get_tarhdr_ptr(h, offset);

   memcpy((void *)&n, h, sizeof(n));
   memset(n.chksum, ' ', 8);

   for (i = 0; (size_t)i < sizeof(n); i++)
      cksum += (unsigned char)n.chksum[i];

   return (signed)cksum;
}

long tar_checksum2(tarhdr *h, size_t offset)
{
   signed long cksum = 0;
   int i;
   tarhdr n;
   h = get_tarhdr_ptr(h, offset);

   memcpy(&n, h, sizeof(n));
   memset(n.chksum, ' ', 8);

   for (i = 0; (size_t)i < sizeof(n); i++)
      cksum += (signed char)n.chksum[i];

   return (signed)cksum;
}

int is_valid_checksum(tarhdr *t, size_t offset)
{
   long cksum = get_cksum_field(t, offset);

   if (tar_checksum1(t, offset) == cksum)
   { /* fprintf(stderr,"DEBUG: chksum1 is valid\n"); */
      return 1;
   }
   if (tar_checksum2(t, offset) == cksum)
   { /* fprintf(stderr,"DEBUG: chksum2 is valid\n"); */
      return 2;
   }
   return 0;
}

void set_header_checksum(tarhdr *t, size_t cursor, int method)
{
   unsigned int cksum = method ? tar_checksum1(t, cursor) : tar_checksum2(t, cursor);
   t = get_tarhdr_ptr(t, cursor);

   char oct[8];
   snprintf(oct, 8, "%8o", cksum);
   // meh, no zero termination so have to copy this manually
   for (int i = 0; i < 8; i++)
      t->chksum[i] = oct[i];
}

char *get_filename(tarhdr *t, size_t cursor)
{
   t = get_tarhdr_ptr(t, cursor);
#ifdef DEBUG
   fprintf(stderr, "%s:%s:%d filename at offset %d is:%s\n", __FILE__, __FUNCTION__, __LINE__, cursor, t->name);
#endif
   return t->name;
}

void set_file_size(tarhdr *t, size_t cursor, size_t newsize)
{
   int which_checksum = is_valid_checksum(t, cursor);
   char oct[13];
   snprintf(oct, 12, "%12o", newsize);
   tarhdr *h = get_tarhdr_ptr(t, cursor);

   // meh, no zero termination so have to copy this manually
   for (int i = 0; i < 12; i++)
      h->size[i] = oct[i];

   set_header_checksum(t, cursor, which_checksum == 1);
}

int is_end_block(tarhdr *t, size_t offset)
{
   int i;
   t = get_tarhdr_ptr(t, offset);

   unsigned char *c = (void *)t;

   for (i = 0; i < 512; i++)
      if (c[i] != 0)
         return 0;
   return 1;
}

int is_2x_end_block(tarhdr *t, size_t offset)
{
   int i;
   t = get_tarhdr_ptr(t, offset);

   unsigned char *c = (void *)t;

   for (i = 0; i < 1024; i++)
      if (c[i] != 0)
         return 0;
   return 1;
}

size_t get_end_of_file_block(tarhdr *h, size_t cursor)
{
   size_t size;
#ifdef DEBUG
   fprintf(stderr, ":%s:%s:%d previous offset:%08x (%d) is_valid:%d\n", __FILE__, __FUNCTION__, __LINE__, cursor, cursor, is_valid_header(h, cursor));
#endif
   size = get_tar_header_size(h, cursor);
#ifdef DEBUG
   fprintf(stderr, "%s:%s:%d got back size: %08x (%d)\n", __FILE__, __FUNCTION__, __LINE__, size, size);
#endif
   size = size + 512;
   size = roundup512(size);

#ifdef DEBUG
   fprintf(stderr, ":%s:%s:%d returning size:%08x (%ld) (+ 512 rounded up + cursor:%08x (%ld))\n", __FILE__, __FUNCTION__, __LINE__, size, size, cursor, cursor);
#endif

   cursor += size;

   return cursor;
}

size_t find_next_header(tarhdr *h, size_t cursor)
{
   size_t size;

   cursor = get_end_of_file_block(h, cursor);

#ifdef DEBUG
   fprintf(stderr, ":%s:%s:%d cursor:%d cursor_last_bits:%d\n", __FILE__, __FUNCTION__, __LINE__, cursor, cursor & 511);
#endif

   return cursor;
   // return -1;
}

void help(void)
{
   printf("dc42-to-tar\n\n"
          //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
          //  0         1         2         3         4         5         6         7         8
          "Usage: dc42-to-raw file1.dc42 file2.dc42 file3.dc42 (order is critical)\n\n"
          "output will be written to floppy-tar.tar\n"
          "This utility is used to convert dc42 floppy tar disks to tar files\n"
          "you may need to merge the tar file segments together at the end\n"
          "(Basically it just strips off all the DC42 headers and tags)\n");
   exit(0);
}

int main(int argc, char *argv[])
{
   int i, j, err, stat;
   unsigned int x;
   DC42ImageType F;
   char *Image = NULL;
   uint8 *data;

   size_t cursor = 0, lastfilecursor = 0, nextcursor;
   size_t filesize = 0;
   size_t lasthdrlocation = 0;

   tarhdr *t;

   FILE *raw;

   puts("  ---------------------------------------------------------------------------");
   puts("    DC42 To tar file  v0.01                     http://lisaem.sunder.net");
   puts("  ---------------------------------------------------------------------------");
   puts("          Copyright (C) MMXX, Ray A. Arachelian, All Rights Reserved.");
   puts("              Released under the GNU Public License, Version 2.0");
   puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
   puts("  ---------------------------------------------------------------------------");
   puts("  Use this to extract a xenix/uniplus tar archive spanning several floppies  ");
   puts("     Takes a list of dc42 files as parameters outputs to floppy-tar.tar      ");
   puts("     File order is critical to properly convert the disks.                   ");
   puts("  ---------------------------------------------------------------------------\n");

   if (argc < 2)
      help();
   else
      for (i = 1; i < argc; i++)
      {
         if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
            help();
      }

   raw = fopen("floppy-tar.tar", "wb");
   if (!raw)
   {
      fprintf(stderr, "Could not create %s\n", raw);
      perror("");
      F.close_image(&F);
      exit(1);
   }

   // add up all the disk images together to get a rough idea of how much memory to allocate
   // so we can have the whole tar file in RAM that way we can use memmove as needed and easily
   // edit headers and remove the unused ends of the disk images.

#ifndef __MSVCRT__
   for (int file = 1; file < argc; file++)
   {
      struct stat statbuf;
      int i = lstat(argv[file], &statbuf);
      if (i == 0)
         memsize += statbuf.st_size;
      else
      {
         fprintf(stderr, "Error accessing %s ", argv[file]);
         perror("");
         exit(1);
      }
   }
#else
   // estimate it for windows
   memsize = argc * 860 * 1024;
#endif

   tarball = calloc(2, memsize); // allocate 2x as much so we'll zero fill the end. Not efficient, but helps when we'll do memmove as zero filled records are EOF markers in tar
   if (!tarball)
   {
      fprintf(stderr, "Couldn't allocate %ld bytes\n", memsize);
      exit(2);
   }

   for (int file = 1; file < argc; file++)
   {
      Image = argv[file];
      err = dc42_auto_open(&F, Image, "w");
      if (err)
      {
         fprintf(stderr, "could not open: '%s' because:%s", Image, F.errormsg);
         exit(1);
      }

      if (F.sectorsize != 512)
      {
         fprintf(stderr, "Error: disk image :%s: sector size is not 512 bytes!\n", Image);
      }

      printf("Copying blocks from %s\n", Image);
      for (x = 0; x < F.numblocks; x++)
      {
         data = F.read_sector_data(&F, x);
         if (!data)
         {
            fprintf(stderr, "Could not read data for sector %d from image!\n", x);
            F.close_image(&F);
            exit(2);
         }
         memcpy(&tarball[cursor], data, 512);
         cursor += 512;
      }
      F.close_image(&F);
   }

   printf("Analyzing in memory data to consolidate spanned files and remove gaps.\n");

   fseek(raw, 0, SEEK_SET);
   fwrite(tarball, memsize, 1, raw);
   fflush(raw);

   // walk the file, header by header, add the of each file rounded up to 512 and go to the next header.
   // delete blocks of empty 512 bytes (ends of disks) by memmove the entire archive up over these and
   // substract the difference from memsize.
   // if the previous file name is the same as the current file, merge them together at the byte sizes.
   // move the 3rd file after the 2nd one to the 512 byte rounded off next location, zero fill to this.
   // at the end of the file, provide two blank 512 byte areas.
   int filenum = 1;
   cursor = 0;

   if (is_valid_header((tarhdr *)tarball, cursor))
      fprintf(stderr, "valid: ");
   else
   {
      fprintf(stderr, "invalid header, not sure this was a tar file.\n");
      exit(1);
   }

   char *lastfilename = NULL, *currentfilename = NULL;
   lastfilecursor = cursor;
   t = get_tarhdr_ptr((tarhdr *)tarball, cursor);
   lastfilename = get_filename((tarhdr *)tarball, cursor);
   fprintf(stderr, "entering loop - cursor:%08x (%d) name:%s\n", cursor, cursor, lastfilename);
   lastfilecursor = cursor;

   while (cursor < memsize)
   {
#ifdef DEBUG
      fprintf(stderr, "start of loop, next cursor:%08x (%d)\n\n\n", nextcursor, nextcursor);
#endif

      if (is_null_header((tarhdr *)tarball, cursor))
      {
         fprintf(stderr, "found null block at %08x (%d) - crunching it size:%ld\n", cursor + 512, cursor + 512, memsize);
         memmove((char *)(&tarball[cursor]), (char *)(&tarball[cursor + 512]), memsize); // crunch empty headers that may exist between diskettes
         memsize -= 512;
         continue;
      }

      nextcursor = find_next_header((tarhdr *)tarball, cursor);

      // here's the important bit. Xenix can split a single file across two disks, it does this by
      // putting the same file name, but with two different sizes. If you do this with modern tar,
      // the 2nd half of the file will OVERWRITE the first. Instead we want to merge them together.
      // that's why there's a need for this tool. Otherwise you can strip off the tags off all the
      // xenix floppy dc42 images and then just run tar on each one, but that would result in data loss.
      if (!is_valid_header((tarhdr *)tarball, nextcursor))
      {
         fprintf(stderr, "invalid header, not sure this was a tar file.\n");
         exit(1);
      }

      cursor = nextcursor;
      currentfilename = get_filename((tarhdr *)tarball, cursor);

      fprintf(stderr, "%s:%s:%d comparing previous file name to current :%s: :%s:\n", __FILE__, __FUNCTION__, __LINE__, lastfilename, currentfilename);
      if (0 == strncmp(lastfilename, currentfilename, 100))
      {
         /* Want to go from this:          :      to this:
            header file-part1              | header file-part1+part2 *
            .... N blocks ....             | .... N+M blocks ....
            { possible X empty blocks }    | header other file...
            header file-part2              | ...
            .... M blocks ....             |
            header other file...           : *- sum file sizes and update tar header checksum
         */

         size_t lastfilesize = get_tar_header_size((tarhdr *)tarball, lastfilecursor);
         size_t currentfilesize = get_tar_header_size((tarhdr *)tarball, cursor);
         size_t lastfileends = roundup512(lastfilecursor + 512 + get_tar_header_size((tarhdr *)tarball, lastfilecursor)); // new target

         fprintf(stderr, "found continued file %100s merging\n", lastfilename);
         // target end of lastfile        source - start of current file (512 after header) - memsize to zero fill the end
         memmove((char *)&tarball[lastfileends], (char *)&tarball[cursor + 512], memsize);

         memsize = memsize - (cursor + 512 - lastfileends);

         set_file_size((tarhdr *)tarball, lastfilecursor, lastfilesize + currentfilesize);

         fprintf(stderr, "Restarting on %s as was merged\n", currentfilename);
         cursor = lastfilecursor;
         lastfilename = currentfilename;
         continue;
         // point cursor to our newly created aggregate file, and restart again. need to do it with
         // the last file because the file could potentially span another disk yet again.
      }
      else
         fprintf(stderr, "%s:%s:%d previous file name to current differed :%s: :%s:\n", lastfilename, currentfilename);

      lastfilecursor = cursor;
      lastfilename = currentfilename;
      fprintf(stderr, "========= following to next %08x (%d) ==========\n", cursor, cursor);
   }
   /* https://en.wikipedia.org/wiki/Tar_(computing)
   A tar archive consists of a series of file objects, hence the popular term tarball, referencing how a tarball collects objects of all kinds that stick to
   its surface. Each file object includes any file data, and is preceded by a 512-byte header record. The file data is written unaltered except that its
   length is rounded up to a multiple of 512 bytes. The original tar implementation did not care about the contents of the padding bytes, and left the
   buffer data unaltered, but most modern tar implementations fill the extra space with zeros.[9] The end of an archive is marked by at least two
   consecutive zero-filled records. (The origin of tar's record size appears to be the 512-byte disk sectors used in the Version 7 Unix file system.)
   The final block of an archive is padded out to full length with zeros.  */

   fseek(raw, 0, SEEK_SET);
   fwrite(tarball, memsize + 1024, 1, raw); // add 2 512 byte null blocks at the end to signal valid end of tar.
   fflush(raw);
   fclose(raw);

   return 0;
}
