/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                  Copyright (C) 1998, 2022 Ray A. Arachelian                          *
*                            All Rights Reserved                                       *
*                                                                                      *
*                     Release Project Name: LisaFSh Tool                               *
*                                                                                      *
\**************************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include <libdc42.h>

#ifdef HAVEREADLINE
#include <readline/readline.h>
#include <readline/history.h>

#endif

const uint16 dispsize = 16;

int tagsaresorted = 0;
uint32 sector = 0;
uint32 clustersize = 512;
uint32 havetags = 0L;

char cmd_line[8192]; // command line

typedef struct
{
  int sector; // which sector is this?
  uint8 *tagptr;
} sorttag_type;

int *sorttag = NULL; // sorted tags
uint8 *allocated = NULL;

DC42ImageType F;

char volumename[32];
uint16 fsversion = 0;
uint32 firstmddf = 65536;

char directory[65536];
// these arrays are super inefficient, but wtf
char filenames[65536][4][64]; // [fileid][size of file name][2] 0=Lisa name 1=sanitized name
                              // cached file names for all the tags - kind of big, I know.

uint32 size1[65536]; // one of these is physical size, the other logical
uint32 size2[65536];
uint32 fileflags[65536];
uint16 bozo[65536];
uint32 serialized[65536];
uint32 creationdate[65536];
uint32 modificationdate[65536];
uint32 accesstime[65536];
uint8 passworded[65536];
uint8 proccessed[65536];
// the first few sectors of a disk are reserved for the boot sector + os loader
#define RESERVED_BLOCKS 28

#define TAG_BOOTSECTOR 0xaaaa
#define TAG_OS_LOADER 0xbbbb
#define TAG_FREE_BLOCK 0x0000
#define TAG_ERASED_BLK 0x7fff

#define TAG_MDDF 0x0001
#define TAG_FREEBITMAP 0x0002
#define TAG_S_RECORDS 0x0003
#define TAG_DIRECTORY 0x0004
#define TAG_MAXTAG 0x7fff

#define MAXARGS 640
// command line user interface verbs  ////////////////////////////////////////////////////////////////
int lastarg = 0;

char cargsstorage[8192];

long iargs[MAXARGS];
long hargs[MAXARGS];
long dargs[MAXARGS];
char *cargs[MAXARGS];

enum command_enum
{
  nullcmd = -999,
  secprevcmd = -2,
  secnxt = -1, // blank -/+
               // these must match the cmdstrings and defines below in order
  sectorset = 0,
  cluster = 1,
  display = 2,
  setclustersize = 3,
  dump = 4,
  tagdump = 5,
  sorttagdump = 6,
  sortdump = 7,
  extract = 8,
  version = 9,
  help = 10,
  bitmap = 11,
  sortnext = 12,
  sortprevious = 13,
  dir = 14,
  dirx = 15,
  editsector = 16,
  edittag = 17,
  diff2img = 18,
  loadsec = 19,
  loadbin = 20,
  loadprofile = 21,
  volname = 22,
  quit = 23
};
enum command_enum command;

#define QUITCOMMAND 23
#define LASTCOMMAND 24

char *cmdstrings[LASTCOMMAND + 2] =
    {"sector", "cluster", "display", "setclustersize", "dump", "tagdump",
     "sorttagdump", "sortdump", "extract", "version", "help", "bitmap",
     "n", "p", "dir", "dirx", "editsector", "edittag", "difftoimg",
     "loadsec", "loadbin", "loadprofile", "volname", "quit", ""};

#define NULL_CMD -999
#define SECTOR_PRV -2
#define SECTOR_NXT -1

#define SECTOR_CMD 0
#define CLUSTER_CMD 1
#define DISPLAY_CMD 2
#define SETCLUSTERSIZE_CMD 3
#define DUMP_CMD 4
#define TAGDUMP_CMD 5

#define SORTTAGDUMP_CMD 6
#define SORTDUMP_CMD 7
#define EXTRACT_CMD 8
#define VERSION_CMD 9
#define HELP_CMD 10
#define BITMAP_CMD 11

#define SORT_NEXT 12
#define SORT_PREV 13
#define DIR_CMD 14
#define DIRX_CMD 15
#define EDITSECTOR_CMD 16
#define EDITTAG_CMD 17

#define DIFF2IMG_CMD 18
#define LOADSEC_CMD 19

#define LOADBIN_CMD 20
#define LOADPROFILE_CMD 21

#define VOLNAME_CMD 22
#define QUIT_CMD 23
#define ENULL_CMD 24

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

// Prototypes and Macros //////////////////////////////////////////////////////////////////////

// careful, cannot use ++/-- as parameters for TAGFILEID macro.
#define TAGFILEID(xmysect) (tagfileid(F, xmysect))
#define TAGFFILEID(xmysect) (tagfileid(&F, xmysect))

void getcommand(void);
int floppy_disk_copy_image_open(DC42ImageType *F);
void hexprint(FILE *out, char *x, int size, int ascii_print);
void printsectheader(FILE *out, DC42ImageType *F, uint32 sector);
void printtag(FILE *out, DC42ImageType *F, uint32 sector);
void printsector(FILE *out, DC42ImageType *F, uint32 sector, uint32 sectorsize);
void cli(DC42ImageType *F);
void filenamecleanup(char *in, char *out);

long interleave5(long sector)
{
  static const int offset[] = {0, 5, 10, 15, 4, 9, 14, 3, 8, 13, 2, 7, 12, 1, 6, 11, 16, 21, 26, 31, 20, 25, 30, 19, 24, 29, 18, 23, 28, 17, 22, 27};
  return offset[sector % 32] + sector - (sector % 32);
}

long deinterleave5(long sector)
{
  static const int offset[] = {0, 13, 10, 7, 4, 1, 14, 11, 8, 5, 2, 15, 12, 9, 6, 3, 16, 29, 26, 23, 20, 17, 30, 27, 24, 21, 18, 31, 28, 25, 22, 19};
  return offset[sector % 32] + sector - (sector % 32);
}

char niceascii(char c)
{
  c &= 127;
  if (c < 31)
    c |= 32;
  if (c == 127)
    c = '.';
  return c;
}

uint8 hex2nibble(uint8 n)
{

  if (n >= '0' && n <= '9')
    return (uint8)(n - '0');
  if (n >= 'a' && n <= 'f')
    return (uint8)(n - 'a' + 10);
  if (n >= 'A' && n <= 'F')
    return (uint8)(n - 'A' + 10);
  return 255;
}

uint8 hex2byte(uint8 m, uint8 l)
{
  // printf("Got:%c%c Returning:%02x:\n",m,l,(hex2nibble(m)<<4) | (hex2nibble(l)) );
  return (hex2nibble(m) << 4) | (hex2nibble(l));
}

uint32 hex2long(uint8 *s)
{
  char buffer[12];

  buffer[0] = '0';
  buffer[1] = 'x';
  memcpy(&buffer[2], s, 8);
  buffer[10] = 0;

  // printf("Transforming:%s into %08x\n",buffer,strtol(buffer,NULL,16));
  return strtol(buffer, NULL, 16);
}

uint16 tagfileid(DC42ImageType *F, int mysect)
{
  uint8 *tag = F->read_sector_tags(F, mysect);
  if (!tag)
    return 0;
  return ((tag[4] << 8) | (tag[5]));
}

uint32 tagpair(DC42ImageType *F, int mysect, int offset)
{
  uint8 *tag = F->read_sector_tags(F, mysect);

  // floppy?
  if (F->tagsize < 20)
    return ((tag[offset] << 8) | (tag[offset + 1]));

  // profile/widget
  return ((tag[offset] << 16) | (tag[offset + 1] << 8) | (tag[offset + 2]));
}

int tagcmp(const void *p1, const void *p2)
{
  uint32 fileid1, fileid2, abs1, abs2, next1, next2, prev1, prev2;

  // turn voids into sort tag types
  int i = (*(int *)p2);
  int j = (*(int *)p1);

  fileid1 = tagfileid(&F, i); // sadly we have to use the global variable here since qsort won't let us pass extra args
  fileid2 = tagfileid(&F, j);

  if (F.tagsize < 20) // floppy?
  {
    abs1 = tagpair(&F, i, 6) & 0x7ff;
    abs2 = tagpair(&F, j, 6) & 0x7ff;

    next1 = tagpair(&F, i, 8) & 0x7ff;
    next2 = tagpair(&F, j, 8) & 0x7ff;

    prev1 = tagpair(&F, i, 10) & 0x7ff;
    prev2 = tagpair(&F, j, 10) & 0x7ff;
  }
  else // hard drive
  {    // 012345
    abs1 = tagpair(&F, i, 8) & 0x7fffff;
    abs2 = tagpair(&F, j, 8) & 0x7fffff;

    next1 = tagpair(&F, i, 0x0e) & 0x7fffff;
    next2 = tagpair(&F, j, 0x0e) & 0x7fffff;

    prev1 = tagpair(&F, i, 0x12) & 0x7fffff;
    prev2 = tagpair(&F, j, 0x12) & 0x7fffff;
  }

  // sort keys in order: file id, absolute sector #, next sector #

  if (fileid1 > fileid2)
    return -1;
  if (fileid1 < fileid2)
    return +1;

  if (abs1 > abs2)
    return -1;
  if (abs1 < abs2)
    return +1;

  if (next1 > next2)
    return -1;
  if (next1 < next2)
    return +1;

  return 0; // if we made it here, they're equal, uh? problem possibly.
}

// Dump the data in the MDDF - Thanks to Chris McFall for this info.  :)
void dump_mddf(FILE *out, DC42ImageType *F)
{
  uint8 *sec;
  int i,j ;
  uint32 sector, sect;
  uint8 volname_len;

  if (!volumename[0]) // if we already did the work don't bother.
  {                   // fprintf(out,"Searching for MDDF block.\n");
    for (sector = 0; sector < F->numblocks; sector++)
    {
      sect = sorttag[sector];

      sec = (uint8 *) F->read_sector_data(F, sect); //&(sectors[sect*sectorsize]);

       if (TAGFILEID(sect)==TAG_MDDF)
       {
        volname_len = sec[0xc];
        for (j=0,i=0x0d; j<volname_len; i++,j++) volumename[j]=sec[i];

        fsversion = (sec[0] << 8) | sec[1];

        if (sect < firstmddf)
          firstmddf = sect; // keep track of the first one in the image for bitmap sizing.

        fprintf(out, "MDDF (Superblock) found at sector #%04x(%4d) fsversion:%02x\n", sect, sect, fsversion);
      }
    }
    if (!fsversion)
      fprintf(out, "MDDF not found.\n");
  }

  fprintf(out, "\n-----------------------------------------------------------------------------\n");
  fprintf(out, "MDDF Volume Name: \"%s\"\n", volumename);

  switch (fsversion)
  {
  case 0x00:
    fprintf(out, "FS Version information not found or MDDF version=0 or bad MMDF!\n");
    break;
  case 0x0e:
    fprintf(out, "Version 0x0e: Flat File System with simple table catalog Release 1.0\n");
    break;
  case 0x0f:
    fprintf(out, "Version 0x0f: Flat File System with hash table catalog Release 2.0 - Pepsi\n");
    break;
  case 0x11:
    fprintf(out, "Version 0x11: Hierarchial FS with B-Tree catalog Spring Release - 7/7\n");
    break;
  default:
    fprintf(out, "Unknown MDDF Version: %02x Could be we didn't find the right MDDF\n", version);
  }

  fprintf(out, "-----------------------------------------------------------------------------\n");
}

void get_allocation_bitmap(DC42ImageType *F)
{
  uint32 sector, i;
  unsigned int as = RESERVED_BLOCKS; // sector is the search for the allocation bitmap, as is the active sector
  uint16 thistagfileid;
  uint8 *sec;

  if (!allocated)
    allocated = malloc(F->numblocks * sizeof(int));

  // reserve all sectors initially as used. the code below will free them
  for (i = 0; i < F->numblocks; i++)
    allocated[i] = 9;

  // find and process all allocation bitmap block(s)
  // printf("Searching for block allocation bitmap blocks...\n");
  for (sector = 0; sector < F->numblocks; sector++)
  {
    if (tagfileid(F, sector) == TAG_FREEBITMAP) // is this a bitmap block?
    {
      printf("Found allocation bitmap block at sector #%04x(%d)\n", sector, sector);
      sec = (uint8 *) F->read_sector_data(F, sector);
      // do all the bits in this block until we go over the # of max sectors.
      for (i = 0; i < F->datasize; i++) // ??? might need to change this to match offsets ????
      {
        // This checks two things: one is the bitmap allocation bit corresponding to the active sector, and sets
        // it to 8 if it's on.  Then it checks the file id, if the file id=0, it'll set it to 1.  So if the
        // theory that the file id=0000 when the block is free holds true, the whole of allocated should
        // consists of only 0's and 9's.

        if (as > F->numblocks)
          break; // the allocation bitmap is much larger than can fit inside a single sector.
                 // so we have to ignore the rest of the bits therein.
                 // (since 1 sector=512 bytes, 512*(8 bits/byte)=4096 allocated bits are available
                 // but only 800 sectors in a 400k floppy, 1600 in an 800k, 2888 in a 1.4MB.
                 // So we need to bail out before we overrun the allocated array.

// Figure out which sectors are allocated or free.  We do this by both
// checking the allocation bitmap block, *AND* the tags so that we can
// detect errors in our code/assumptions.  For example, because of this,
// I've discovered the posibility that 0x7fff is the tag file ID for
// an erased block.

// Macros to make the below block simpler for humans to read. :)
#define USEDTAG (thistagfileid != TAG_ERASED_BLK && thistagfileid != TAG_FREE_BLOCK)
#define ALLOC allocated[as]

        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 1) ? 8 : 0) | USEDTAG);
        as++;
        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 2) ? 8 : 0) | USEDTAG);
        as++;
        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 4) ? 8 : 0) | USEDTAG);
        as++;
        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 8) ? 8 : 0) | USEDTAG);
        as++;
        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 16) ? 8 : 0) | USEDTAG);
        as++;
        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 32) ? 8 : 0) | USEDTAG);
        as++;
        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 64) ? 8 : 0) | USEDTAG);
        as++;
        thistagfileid = TAGFILEID(as);
        ALLOC = (((sec[i] & 128) ? 8 : 0) | USEDTAG);
        as++;

#undef USEDTAG
#undef ALLOC
      } // for sector size
    }
  }
}

// return the type of file we are dealing with.
char *getfileid(int sector)
{
  uint32 fileid;
  static char fileidtext[64]; // needs to be static so it doesn't fall off the heap when this fn exits

  fileid = TAGFFILEID(sector); //((tags[(tagsize *(sector))+4] & 0xff)<<8)|((tags[(tagsize *(sector))+5]) & 0xff);

  snprintf(fileidtext, 63, "file-%04x", fileid);

  if (fileid > 0xefff)
    snprintf(fileidtext, 63, "extent-%04x", (~fileid) & 0xffff);
  else if (fileid > TAG_MAXTAG)
    snprintf(fileidtext, 63, "UnKnowN-%04x", fileid);
  if (fileid == TAG_ERASED_BLK)
    snprintf(fileidtext, 63, "deleted-blocks-7fff");
  if (fileid == TAG_BOOTSECTOR)
    snprintf(fileidtext, 63, "bootsect-aaaa");
  if (fileid == TAG_OS_LOADER)
    snprintf(fileidtext, 63, "OSLoader-bbbb");
  if (fileid == TAG_FREE_BLOCK)
    snprintf(fileidtext, 63, "freeblocks-0000");
  if (fileid == TAG_MDDF)
    snprintf(fileidtext, 63, "MDDF-0001");
  if (fileid == TAG_FREEBITMAP)
    snprintf(fileidtext, 63, "alloc-bitmap-0002");
  if (fileid == TAG_S_RECORDS)
    snprintf(fileidtext, 63, "srecords-0003");
  if (fileid == TAG_DIRECTORY)
    snprintf(fileidtext, 63, "directory-0004");
  return fileidtext;
}

char *getfileidbyid(uint16 fileid)
{
  static char fileidtext[64]; // needs to be static so it doesn't fall off the heap when this fn exits
  snprintf(fileidtext, 63, "file-%04x", fileid);
  if (fileid > 0xefff)
    snprintf(fileidtext, 63, "extent-%04x", (~fileid) & 0xffff);
  else if (fileid > TAG_MAXTAG)
    snprintf(fileidtext, 63, "UnKnowN-%04x", fileid);
  if (fileid == TAG_BOOTSECTOR)
    snprintf(fileidtext, 63, "bootsect-aaaa");
  if (fileid == TAG_OS_LOADER)
    snprintf(fileidtext, 63, "OSLoader-bbbb");
  if (fileid == TAG_FREE_BLOCK)
    snprintf(fileidtext, 63, "freeblocks-0000");
  if (fileid == TAG_MDDF)
    snprintf(fileidtext, 63, "MDDF-0001");
  if (fileid == TAG_FREEBITMAP)
    snprintf(fileidtext, 63, "alloc-bitmap-0002");
  if (fileid == TAG_S_RECORDS)
    snprintf(fileidtext, 63, "srecords-0003");
  if (fileid == TAG_DIRECTORY)
    snprintf(fileidtext, 63, "directory-0004");
  return fileidtext;
}

char *get_bozo(uint16 b)
{
  static char bozo[12];
  memset(bozo, 0, 11);
  if (!b)
    return bozo;

  snprintf(bozo, 11, "bozo:%04x", b);
  return bozo;
}

char *get_fileflags(uint16 f)
{
  static char attr[17];
  // as seen in LPW file manager attributes
  attr[0] = (f & 0x0020) ? 'C' : ' '; // either C or O means read only, not sure
  attr[1] = (f & 0x0008) ? 'O' : ' ';
  attr[6] = (f & 0x0002) ? 'B' : ' '; // bozo (DRM flag)

  // unknown possible flags
  attr[2] = (f & 0x0010) ? 'a' : ' ';
  attr[3] = (f & 0x0040) ? 'b' : ' ';
  attr[4] = (f & 0x0080) ? 'd' : ' ';
  attr[5] = (f & 0x0001) ? 'e' : ' ';
  attr[7] = (f & 0x0004) ? 'g' : ' ';
  attr[8] = (f & 0x0100) ? ' ' : '-'; // - all seem to have this one turned on, maybe it means file, or maybe not deleted?
  attr[9] = (f & 0x0200) ? 'i' : ' ';
  attr[10] = (f & 0x0400) ? 'j' : ' ';
  attr[11] = (f & 0x0800) ? 'k' : ' ';
  attr[12] = (f & 0x1000) ? 'l' : ' ';
  attr[13] = (f & 0x2000) ? 'm' : ' ';
  attr[14] = (f & 0x4000) ? 'n' : ' ';
  attr[15] = (f & 0x8000) ? 'p' : ' ';

  attr[16] = 0;
  return attr;
}

void get_dir_file_names(DC42ImageType *F)
{
  uint32 sector, mysect, i, j, k, l, m, ti, ts; // sector is the search for the allocation bitmap, as is the active sector
  uint16 fileid;
  uint8 *sec;
  char *f;
  // int offset=0x10;                                            // start offset
  char filename[128]; // current file name I'm working on

  /* DEBUG */ fprintf(stderr, "initializing directory names\n");
  for (i = 0; i < 65536; i++) // set default file names
  {
    f = getfileidbyid(i);
    strncpy(filenames[i][0], f, 63);
    filenamecleanup(filenames[i][0], filenames[i][1]);
    size1[i] = 0;
    size2[i] = 0;
    fileflags[i] = 0;
    serialized[i] = 0;
    bozo[i] = 0;
    creationdate[i] = 0;
    accesstime[i] = 0;
    modificationdate[i] = 0;
    proccessed[i] = 0;
  }

  /*
  *** redo this code ***
  directory entries don't always start at a fixed position, not sure.
  we can search for 24 00 00 [filename]
  if the file name starts with a zero we can skip

  24 00 starts at 0x0e, filename starts at 0x11, file id is at offset 0x34
  maybe can figure out the other fields as well such as dates and so on.
  need to find the logical file size.

  -----------------------------------------------------------------------------
  Sec 61:(0x003d)   Used Block Part of file directory-0004:"directory-0004"
  -----------------------------------------------------------------------------
              +0 +1 +2 +3 +4 +5 . +6 +7 +8 +9 +a +b +c +d +e +f+10+11+12+13
  tags:       00 00 25 00 00 04 82 00 00 00 . 17 f5 00 00 00 00 18 ff ff ff
             |volid|????|fileid||absnext|   ?   |    ?   |    next|previous
  -----------------------------------------------------------------------------
        +0 +1 +2 +3 +4 +5 +6 +7 . +8 +9 +a +b +c +d +e +f
  -----------------------------------------------------------------------------
  0000: 24 00 00 00 00 00 00 00 . 00 00 00 00 00 00 00 00   |  $
  0010: 00 00 00 00 00 00 00 00 . 00 00 00 00 00 00 00 00   |
  0020: 00 00 00 00 08 00 ff ff . 00 94 00 51 00 a4 00 44   |      (  4 Q $ D
  0030: 00 64 00 0d 00 1a 00 0d . 00 f7 6f 52 00 8a 24 6a   |   d - : - woR *$j

  0040: 6f ce 00 f7 00 03 00 f7 . 6e be 00 f7 6e 8a 24 00   |  oN w # wn> wn*$
  0050: 00 41 73 73 65 6d 62 6c . 65 72 2e 4f 62 6a 00 00   |   Assembler.Obj
  0060: 00 00 00 00 00 00 00 00 . 00 00 00 00 00 00 00 00   |
  0070: 00 00 03 00[00 2a|9d 27 . f9 96|a2 02 25 3a|00 00   |    #  *='y6""%:
                  |fileid|creationdate|modification|
  0080: a6 00|00 00 a6 00 00 01 . 00 00 00 00 00 00 24 00   |  &   &  !      $
        size |      size?|   ??
  0090: 00 43 6f 64 65 2e 4f 62 . 6a 00 00 00 00 00 00 00   |   Code.Obj

  00a0: 00 00 00 00 00 00 00 00 . 00 00 00 00 00 00 00 00   |
  00b0: 00 00 03 00 00 2b 9d 27 . f9 db a2 02 25 3a 00 00   |    #  +='y[""%:
  00c0: ca 00 00 00 ca 00 00 01 . 00 00 00 00 00 00 24 00   |  J   J  !      $

  00d0: 00 45 64 69 74 2e 4d 65 . 6e 75 73 2e 54 65 78 74   |   Edit.Menus.Text
  00e0: 00 00 00 00 00 00 00 00 . 00 00 00 00 00 00 00 00   |
  00f0: 00 00 03 00[00 2c}9c 25 . 53 36|a2 02 25 3a|00 00   |    #  ,<%S6""%:
                  |fileid|creationdate |lastmoddate|size?
  0100:[0c 00]00 00 0c 00|00 01 . 20 00 00 00 00 00 24 00   |  ,   ,  !      $
        size |size?      |        ^^=C flag?

  Pascal size=size/512

  search for 24 00 00 - after this is the file name.
            @4e ^       @51 filename,  @72 is a 03 00 (what's that?)
            @74->2 byte file id
            @76+4=creation date
            @7a+4=modification date
            @7e+4=size1
            @82+4=size2
            @86+2=00 01 ???
            @88="C" flag if 0x20

  What sets that "C" flag? Edit.Menus.Text has it, but not Assembler.obj

  from PHP: creation date: Monday, January 5, 1987 12:10:30 PM  https://www.epochconverter.com/mac?
  From LPW:                        January 6, 1984 12:10:xx PM
  PHP: $currenttimestamp = 2082844800+time();  but this is off by 3 years and one day.
  86400=1 day
  31536000=1 year (365)
  94608000=3 years
  94694400=3 years+1 day.

  2082844800-94694400=
  -94694400 for website
  time()+1988150400 - adjust to unix epoch time.

  24 00 00: 0filename: 3
  fileid    38
  creadate: 40
  moddate : 44
  size1   : 48
  size2   : 52
  flags   : 56

  */

  for (mysect = 0; mysect < F->numblocks; mysect++)
  {
    if (TAGFILEID(mysect) == TAG_DIRECTORY)
    {
      sec = (uint8 *) F->read_sector_data(F, mysect);

      for (int k = 0; k < 512 - 58; k++)
      {
        if ((sec[k] == 0x24 && sec[k + 1] == 0) && sec[k + 2] == 0)
        {
          i = k;
          fileid = (sec[38 + i] << 8) | sec[39 + i];
          for (int n = 0; n < 32; n++)
            filenames[fileid][0][n] = sec[i + n + 3]; // copy filename from directory entry
          filenamecleanup(filenames[fileid][0], filenames[fileid][1]);
          creationdate[fileid] = (sec[40 + i] << 24) | (sec[41 + i] << 16) | (sec[42 + i] << 8) | (sec[43 + i]);
          modificationdate[fileid] = (sec[44 + i] << 24) | (sec[45 + i] << 16) | (sec[46 + i] << 8) | (sec[47 + i]);
          size1[fileid] = (sec[48 + i] << 24) | (sec[49 + i] << 16) | (sec[50 + i] << 8) | (sec[51 + i]);
          size2[fileid] = (sec[52 + i] << 24) | (sec[53 + i] << 16) | (sec[54 + i] << 8) | (sec[55 + i]);
          fileflags[fileid] = (sec[56 + i] << 16) | (sec[57 + i] << 8) | (sec[58 + i]);

          for (l = 0; l < F->numblocks; l++) // find the extent/inode block for this file id, and fill the rest of the fields.
          {
            char temp[1024], buf1[128], buf2[128], buf3[128];

            uint16 fid = TAGFILEID(l);
            uint16 nfid = (0x10000 - fid) & 0xffff;
            if ((nfid == fileid) && (fileid > 4 && fileid < 0x8000) && !proccessed[fileid])
            { // check filename for match, then get the rest of the fields.
              uint8 *fsec = (uint8 *) F->read_sector_data(F, l);
              char inodename[33];
              uint8 len = fsec[0];

              time_t t;
              struct tm *ts;

              int match = 1;

              memset(inodename, 0, 32);
              proccessed[fileid] = 1;
              for (int n = 1; n <= len; n++)
              {
                inodename[n - 1] = fsec[n];
                if (fsec[n] != filenames[fileid][0][n - 1])
                  match = 0;
              }
              if (!match)
              {
                fprintf(stderr, "Warning: filename (%s) in extent for file %04x does not match directory entry %04x (%s)\n", inodename, nfid, fileid, filenames[nfid][0]);
              }
              creationdate[fileid] = (fsec[0x2e] << 24) | (fsec[0x2f] << 16) | (fsec[0x30] << 8) | fsec[0x31];
              accesstime[fileid] = (fsec[0x32] << 24) | (fsec[0x33] << 16) | (fsec[0x34] << 8) | fsec[0x35];
              modificationdate[fileid] = (fsec[0x36] << 24) | (fsec[0x37] << 16) | (fsec[0x38] << 8) | fsec[0x39];
              serialized[fileid] = (fsec[0x42] << 24) | (fsec[0x43] << 16) | (fsec[0x44] << 8) | fsec[0x45];
              bozo[fileid] = (fsec[0x48] << 8) | fsec[0x49];
              passworded[fileid] = fsec[0x62]; // 62=passwd length 0-7 actual length 8=(8-20 bytes), 63-6B hash

// epoc adjust b/w Lisa and unix time
#define ADJUST 2177452800

              t = (time_t)((time_t)(creationdate[fileid]) - (time_t)(ADJUST));
              ts = localtime(&t);
              strftime(buf1, sizeof(buf1), "%Y.%m.%d-%H:%M", ts);
              t = (time_t)((time_t)(modificationdate[fileid]) - (time_t)(ADJUST));
              ts = localtime(&t);
              strftime(buf2, sizeof(buf2), "%Y.%m.%d-%H:%M", ts);
              t = (time_t)((time_t)(accesstime[fileid]) - (time_t)(ADJUST));
              ts = localtime(&t);
              strftime(buf3, sizeof(buf3), "%Y.%m.%d-%H:%M", ts);

              if (serialized[fileid])
                snprintf(temp, 1023, "%04x    %-32s     %s %s %s  %10ld %10ld  %s %s %s branded:%08x\n",
                         fileid, filenames[fileid][0], buf1, buf2, buf3,
                         (long)size1[fileid], (long)size2[fileid],
                         (passworded[fileid] ? "PASSWD" : ""),
                         get_fileflags(fileflags[fileid]),
                         get_bozo(bozo[fileid]),
                         serialized[fileid]);
              else
                snprintf(temp, 1023, "%04x    %-32s     %s %s %s  %10ld %10ld  %s %s %s\n",
                         fileid, filenames[fileid][0], buf1, buf2, buf3, (long)size1[fileid], (long)size2[fileid],
                         (passworded[fileid] ? "PASSWD" : ""),
                         get_fileflags(fileflags[fileid]),
                         get_bozo(bozo[fileid]));

              strncat(directory, temp, 65535); // add filename to directory.

              filenamecleanup(filenames[fileid][0], // clean it up, now output name is set
                              filenames[fileid][1]);

              // aliasname=63 chars max starts at 0x182 (what's 180-181? set to 00 02, what are the fields following?)
              if (fsec[0x182])
              {
                int len = fsec[0x182];
                for (int x = 0; x < len; x++)
                  filenames[fileid][3][x] = fsec[0x183 + x];
                filenames[fileid][3][len] = 0;
                snprintf(temp, 1023, "        (%04x) +->DocumentName: %s\n",
                         (uint16)((fsec[0x180] << 8) | (fsec[0x180] << 8)),
                         filenames[fileid][3]);
                strncat(directory, temp, 65535);
              }
              else
              {
                filenames[fileid][3][0] = 0;
              }

              l = F->numblocks;
            }
          }
        }
      }
    }
  }
}

// Sort the tags by the keys above, then extract information that's tag sensitive.
void sorttags(DC42ImageType *F)
{
  unsigned int i;

  if (!sorttag)
    sorttag = malloc(F->numblocks * sizeof(int));

  for (i = 0; i < F->numblocks; i++)
    sorttag[i] = i;                                                  // initial index of tags is a 1:1 mapping
  qsort((void *)sorttag, (size_t)F->numblocks, sizeof(int), tagcmp); // sort'em

  get_allocation_bitmap(F);
  get_dir_file_names(F);
  dump_mddf(stdout, F);
}

void dump_allocation_bitmap(FILE *out, DC42ImageType *F)
{
  int i;
  unsigned int j;
  char c;
  fprintf(out, "\n                  Block allocation bitmap\n");
  fprintf(out, "----------+----------------------------------------+\n");
  fprintf(out, "Sector    |..........1.........2........3..........|\n");
  fprintf(out, "hex dec  +|0123456789012345678901234567890123456789|\n");
  fprintf(out, "----------+----------------------------------------|\n");

  for (j = 0; j < F->numblocks; j += 40)
  {
    fprintf(out, "%04x(%04d)|", j, j);
    for (i = 0; i < 40; i++)
    {
      switch (allocated[i + j])
      {
      case 0:
        c = ' ';
        break;
      case 9:
        c = '#';
        break;
      case 1:
        c = '.';
        break;
      case 8:
        c = '8';
        break;
      default:
        c = '?';
        break;
      }
      fprintf(out, "%c", c);
    }
    fprintf(out, "|\n");
  }
  fprintf(out, "----------+----------------------------------------+\n\n");
}

void filenamecleanup(char *in, char *out)
{
  uint32 j;
  char c;
  for (j = 0; in[j] && j < 63; j++) // santize file name for most host OS's.
  {                                 // some of these are over cautious, i.e. spaces can be escaped in unix, etc...
                                    // but I want to make sure we don't cause needless trouble and require escaping
                                    // with lots of backslashes, etc.  In fact, you can even build file names containing
                                    // wildcards and slashes in unix, but accessing them from the commandline becomes
                                    // an excercise in backslash insanity
    c = in[j];
    switch (c)
    {
    case '"': // get rid of quotes
    case '`':
    case '\'':
    case '?': // wild cards
    case '*':
    case '/': // slashes
    case '\\':
      c = '-';
      break;
    case ':':
      c = '-';
      break; // colons for MSFT OS's

    case '!': // !'s for unix
    case '&': // this means background in most unix shells.
    case '$': // shell environment variables for unix + windows
    case '%':

    case '<': // redirects and pipe
    case '>':
    case '|':
    case '(':
    case ')':
    case '[':
    case ']':
      c = '_';
    }
    if (c < 33 || c > 126)
      c = '_'; // control and high chars

    out[j] = c;
  }
  out[63] = 0; // make double dog sure the string is terminated;
}

// Walk the directory catalog and read all of the file names stored in there
// then store them in the table.

void extract_files(DC42ImageType *F)
{
  /*-----------------2/15/2004 6:57PM-----------------
   * extract all files inside of a disk image
   *
   * This could use better error handling, and also needs
   * to extract the friendly+system file names.  For now
   * like all things, it's a damned hack. ;)
   * --------------------------------------------------*/

  FILE *fh = NULL, *fb = NULL, *fx = NULL; // file handles for header, binary, and hex
  unsigned int sector = 0;                 // sector number
  char newfile[FILENAME_MAX];              // name of file or name of header metadata
  char newfilex[FILENAME_MAX];             // name of hex/text file to create
  char newfileb[FILENAME_MAX];             // name of binary file to create
  char newfilemb[FILENAME_MAX];            // name of meta data binary file to create
  char newfilemh[FILENAME_MAX];            // name of meta data hex binary file to create

  char newdir[1024]; // name of new directory to create
  char *sub;         // substring search to chop extension
  uint8 *sec;        // pointer to sector data

  int chop0xf0 = 0; // type of file ID, and whether there's a metadata attached.
  uint16 fileid, oldfileid = 0xffff;
  int sect; // translated sector number
  int err;

  // create a directory with the same name as the image in the current directory and enter it ////////
  snprintf(newfile, FILENAME_MAX, F->fname);

  // whack off any extensions           // Butthead: "Huh huh, huh huh, he said 'Whack!'"
  // Beavis:   "Yeah, Yeah! Then he said 'Extensions'"
  // Butthead: "Huh huh, huh huh, that was cool!"

  sub = strstr(newfile, ".dc42");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".DC42");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".dc");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".DC");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".image");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".Image");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".img");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".IMG");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".dmg");
  if (sub)
    *sub = 0;
  sub = strstr(newfile, ".DMG");
  if (sub)
    *sub = 0;

  // add a .d to the file name, then create and enter the directory
  snprintf(newdir, 1023, "%s.d", newfile);
  printf("Creating directory %s to store extracted files\n", newdir);

#ifndef __MSVCRT__
  err = mkdir(newdir, 00755);
#else
  err = mkdir(newdir);
#endif

  // Might want to change this to ask if it's ok to overwrite this directory.
  if (err && errno != EEXIST)
  {
    fprintf(stderr, "Couldn't create directory %s because (%d)", newdir, err);
    perror("\n");
    return;
  }
  if (chdir(newdir))
  {
    fprintf(stderr, "Couldn't enter directory %s because", newdir);
    perror("\n");
    return;
  }

  // Extract the entire disk //////////////////////////////////////////////////////////////////////////

  errno = 0;

  // extract files -----------------------------------------------------------------------
  for (sector = 0; sector < F->numblocks; sector++)
  {
    sect = sorttag[sector]; // dump files from sectors in sorted order
    fileid = TAGFILEID(sect);
    sec = (uint8 *) F->read_sector_data(F, sect);

    if (fileid != oldfileid) // we have a file id we haven't seen before. open new file handles
    {
      fflush(stdout);
      fflush(stderr);
      if (fx)
      {
        fflush(fx);
        fclose(fx);
        fx = NULL;
      }
      if (fb)
      {
        fflush(fb);
        fclose(fb);
        fb = NULL;
      }
      if (fx)
      {
        fflush(fh);
        fclose(fh);
        fh = NULL;
      }

      printf("%04x!=%04x", fileid, oldfileid);

      if (strlen(filenames[fileid][1]))
        snprintf(newfile, 63, "%s", filenames[fileid][1]);
      else
        snprintf(newfile, 63, "%s", getfileid(sect));
      printf("Extracting: %s -> %s (.bin/.txt)\n", filenames[fileid][0], newfile);

      // 2021.02.24 meh seems we don't need to chop off the meta - esp for LPW files
      // might revisit this later to see what needs it.
      chop0xf0 = 0; // default for normal files is to have a header
                    // but special files do not, so catch those
                    // and don't chop the first 0xf0 bytes

      if (fileid == TAG_BOOTSECTOR)
        chop0xf0 = 0; // boot sector
      if (fileid == TAG_OS_LOADER)
        chop0xf0 = 0; // os loader
      if (fileid == TAG_ERASED_BLK)
        chop0xf0 = 0; // deleted blocks
      if (fileid == TAG_FREE_BLOCK)
        chop0xf0 = 0; // free blocks
      if (fileid == TAG_MDDF)
        chop0xf0 = 0; // mddf
      if (fileid == TAG_FREEBITMAP)
        chop0xf0 = 0; // allocation bitmap
      if (fileid == TAG_S_RECORDS)
        chop0xf0 = 0; // s-records
      if (fileid == TAG_DIRECTORY)
        chop0xf0 = 0; // directory
      if (fileid > TAG_MAXTAG)
        chop0xf0 = 0; // catch other unknown file id's

      snprintf(newfilex, 1024, "%s.txt", newfile);
      fx = fopen(newfilex, "wt");
      if (!fx)
      {
        fprintf(stderr, "Couldn't create file %s\n", newfilex);
        perror("\n");
        int i = chdir("..");
        return;
      }

      snprintf(newfileb, 1024, "%s.bin", newfile);
      fb = fopen(newfileb, "wb");
      if (!fb)
      {
        fprintf(stderr, "Couldn't create file %s\n", newfileb);
        perror("\n");
        int i = chdir("..");
        return;
      }

      // printf("File:%s starts at sector %d %s metadata\n",newfile, sect, chop0xf0 ? "has":"has no ");

      if (chop0xf0) // deal with meta data bearing files----------------------
      {
        // We need to chop off the 1st 0xf0 bytes as they're part of the metadata of the file.
        // create two metadata files, one binary, and one hex.
        snprintf(newfilemb, 1024, "%s.meta.bin", newfile);
        fh = fopen(newfilemb, "wb");
        if (!fh)
        {
          fprintf(stderr, "Couldn't create file %s\n", newfilemb);
          perror("\n");
          int i = chdir("..");
          return;
        }
        // sec=&(sectors[sect*sectorsize]);
        // sec=(uint8 *) F->read_sector_data(F, sect);
        fwrite(sec, 0xf0, 1, fh);
        fclose(fh);
        fh = NULL;
        if (errno)
        {
          fprintf(stderr, "An error occured on file %s", newfile);
          perror("");
          fclose(fb);
          fclose(fx);
          int i = chdir("..");
          return;
        }

        // write remainder of sector to the binary file
        // sec=&(sectors[sect*sectorsize+0xf0]);
        sec = (uint8 *) F->read_sector_data(F, sect);
        fwrite(sec + 0xf0, (F->datasize - 0xf0), 1, fb); // bug found by Rebecca Bettencourt
        if (errno)
        {
          fprintf(stderr, "An error occured on file %s", newfileb);
          perror("");
          fclose(fb);
          fclose(fx);
          int i = chdir("..");
          return;
        }

        snprintf(newfilemh, 1024, "%s.meta.txt", newfile);
        fh = fopen(newfilemh, "wb");
        if (!fh)
        {
          fprintf(stderr, "Couldn't create file %s\n", newfilemh);
          fclose(fb);
          fclose(fx);
          perror("\n");
          int i = chdir("..");
          return;
        }
        printsector(fh, F, sect, 0xf0);
        fclose(fh);
        fh = NULL;
        if (errno)
        {
          fprintf(stderr, "An error occured on file %s", newfileb);
          perror("");
          fclose(fb);
          fclose(fx);
          int i = chdir("..");
          return;
        }

        // dump the data to the hex file, but add a banner warning about metadata.
        fprintf(fx, "\n\n[Metadata from bytes 0x0000-0x00ef]\n");
        printsector(fx, F, sect, F->datasize);

        oldfileid = fileid; // set up for next round
        continue;           // skip to the next sector, this one is done ----------------
      }                     // end of chop0xf0 ----------------------------------------------------------------------

    } // end of new file comparison on oldfileid/fileid  ----------------------------------------

    // now, write the sector out in hex and binary. ---------------------------------------------
    // printf("Writing sector %04x(%d) to %s\n",sect,sect,newfile);
    // void printsector(FILE *out,DC42ImageType *F, uint32 sector,uint32 sectorsize);

    printsector(fx, F, sect, F->datasize);

    if (errno)
    {
      fprintf(stderr, "An error occured on file %s.txt", newfile);
      perror("");
      fclose(fb);
      fclose(fx);
      int i = chdir("..");
      return;
    }

    fwrite(sec, F->datasize, 1, fb);
    if (errno)
    {
      fprintf(stderr, "An error occured on file %s.bin", newfile);
      perror("");
      fclose(fb);
      fclose(fx);
      int i = chdir("..");
      return;
    }

    oldfileid = fileid; // set up for next round

  } // end of all sectors. ---------------------------------------------------------------------

  // cleanup and return;
  if (fx)
  {
    fflush(fx);
    fclose(fx);
    fx = NULL;
  }
  if (fb)
  {
    fflush(fb);
    fclose(fb);
    fb = NULL;
  }
  if (fx)
  {
    fflush(fh);
    fclose(fh);
    fh = NULL;
  }
  int i = chdir("..");
  return;
}

void extract_file_extents_from_tags(DC42ImageType *F)
{
  // Same logic as extract_files but all we do
  // is get the sector extents.

  char *sub;  // substring search to chop extension
  uint8 *sec; // pointer to sector data

  int chop0xf0 = 0; // type of file ID, and whether there's a metadata attached.
  uint16 fileid, oldfileid = 0xffff;
  int sect; // translated sector number
  int err;
  int is_range = 0;    // flag for extent seq sector run
  int lastsector = -1; // last sector seen, along with sect are used to detect seq
                       // sector runs

  errno = 0;

  // walk files -----------------------------------------------------------------------
  for (sector = 0; sector < F->numblocks; sector++)
  {
    sect = sorttag[sector]; // dump files from sectors in sorted order
    fileid = TAGFILEID(sect);
    sec = (uint8 *) F->read_sector_data(F, sect);

    if (fileid != oldfileid) // we have a file id we haven't seen before. open new file handles
    {
      printf("file id: %04x %s:\n", fileid, filenames[fileid][0]);

      chop0xf0 = 1; // default for normal files is to have a header
                    // but special files do not, so catch those
                    // and don't chop the first 0xf0 bytes

      if (fileid == TAG_BOOTSECTOR)
        chop0xf0 = 0; // boot sector
      if (fileid == TAG_OS_LOADER)
        chop0xf0 = 0; // os loader
      if (fileid == TAG_ERASED_BLK)
        chop0xf0 = 0; // deleted blocks
      if (fileid == TAG_FREE_BLOCK)
        chop0xf0 = 0; // free blocks
      if (fileid == TAG_MDDF)
        chop0xf0 = 0; // mddf
      if (fileid == TAG_FREEBITMAP)
        chop0xf0 = 0; // allocation bitmap
      if (fileid == TAG_S_RECORDS)
        chop0xf0 = 0; // s-records
      if (fileid == TAG_DIRECTORY)
        chop0xf0 = 0; // directory
      if (fileid > TAG_MAXTAG)
        chop0xf0 = 0; // catch other unknown file id's

      if (is_range)
      {
        printf(" - %04x(%d)\n", lastsector);
      } // output any dangling extent range ends

      is_range = 0;
      lastsector = -1;
    } // end of new file comparison on oldfileid/fileid  ----------------------------------------

    // now, write the sector number -------------------------------------------------------------
    if (lastsector + 1 == sect) // squeeze down ranges into extent lists
    {
      is_range = 1;
    }
    else
    {
      printf(" - %04x(%d)\n", lastsector);
      printf("%04x(%d) ", sect, sect);
    }

    lastsector = sect;
    oldfileid = fileid; // set up for next round

  } // end of all sectors. ---------------------------------------------------------------------

  return;
}

#ifdef HAVEREADLINE
char **command_name_completion(const char *, int, int);

char *command_names[] = {
    "!",
    "help",
    "?",
    "version",
    "+",
    "-"
    "editsector",
    "edittag",
    "difftoimg",
    "loadsec",
    "loadbin",
    "loadprofile"
    "display"
    "dump"
    "n",
    "p",
    "tagdump"
    "sorttagdump",
    "sortdump",
    "bitmap",
    "extract",
    "dir",
    "dirx",
    "volname",
    "quit",
    NULL};

char *
command_name_generator(const char *text, int state)
{
  static int list_index, len;
  char *name;

  if (!state)
  {
    list_index = 0;
    len = strlen(text);
  }

  while ((name = command_names[list_index++]))
  {
    if (strncmp(name, text, len) == 0)
    {
      return strdup(name);
    }
  }

  return NULL;
}

char **
command_name_completion(const char *text, int start, int end)
{
  rl_attempted_completion_over = 1;
  return rl_completion_matches(text, command_name_generator);
}
#endif

void getcommand(void)
{
  char *space, *s, *ss;
  int i;
  long len, l;
  char line[8192];
  char *cur = cargsstorage;

  // clear arguements
  for (i = 0; i < MAXARGS; i++)
    cargs[i] = NULL;
  memset(cargsstorage, 0, 8192);

  command = LASTCOMMAND;

#ifdef HAVEREADLINE
  rl_attempted_completion_function = command_name_completion;

  cmd_line[0] = 0;
  line[0] = 0;
  ss = readline("lisafsh> ");
  if (ss != NULL)
  {
    strncpy(cmd_line, ss, 8192);
    strncpy(line, ss, 8192);
    add_history(ss);

    // strip off any CR/LF from the end
    for (int i = 0; cmd_line[i] != 0; i++)
    {
      if (cmd_line[i] < 31)
        cmd_line[i] = ' ';
      if (line[i] < 31)
        line[i] = ' ';
    }
  }
  else
  {
    strncpy(cmd_line, "quit", 6);
    strncpy(line, "quit", 6);
    add_history("quit");
  }

  line[8191] = 0;
  cmd_line[8191] = 0;

  len = strlen(cmd_line);

  if (ss)
    free(ss);
  ss = line;
// fprintf(stderr,"\nreadline in use\n");
#else
  printf("lisafsh> ");
  ss = fgets(line, 8192, stdin);
  line[8191] = 0;
  strncpy(cmd_line, line, 8192);
  len = strlen(line);

  // strip off any CR/LF from the end
  for (int i = 0; cmd_line[i] != 0; i++)
  {
    if (cmd_line[i] < 31)
      cmd_line[i] = ' ';
    if (line[i] < 31)
      line[i] = ' ';
  }

#endif

  if (feof(stdin))
  {
    puts("EOF");
    exit(1);
  }

  if (!len)
  {
    command = -1;
    return;
  } // shortcut for next sector.
  if (line[0] == '!')
  {
    if (len == 1)
      i = system("sh");
    else
      i = system(&line[1]);
    command = NULL_CMD;
  } // shell out
  if (line[0] == '?')
  {
    strncpy(line, "help", 6);
  } // help synonym
  if (line[0] == '+' && len == 1)
  {
    command = SECTOR_NXT;
    return;
  } // next sector
  if (line[0] == '-' && len == 1)
  {
    command = SECTOR_PRV;
    return;
  } // previous sector
  if (line[0] == '+' || line[0] == '-')
  {
    l = strtol(line, NULL, 0);
    sector += l;
    command = DISPLAY_CMD;
    return;
  } // delta jump

  space = strchr(line, 32);
  if (space)
    space[0] = 0;

  for (i = 0; i < LASTCOMMAND; i++)
    if (strncmp(line, cmdstrings[i], 16) == 0)
      command = i;

  // shortcut for sector number
  iargs[0] = strtol(line, NULL, 0);
  if (line[0] >= '0' && line[0] <= '9')
  {
    command = 0;
    return;
  }

  if (command == LASTCOMMAND)
  {
    puts("Say what?  Type in help for help...\n");
    return;
  }
  if (command == QUITCOMMAND)
  {
    puts("Quit: Closing image");
    F.close_image(&F);
    puts("Bye");
    exit(0);
  }
  if (!space)
    return;
  line[len] = ' ';
  line[len + 1] = 0;

  s = space + 1;

  lastarg = 0;
  while ((space = strchr(s, (int)' ')) != NULL && lastarg < MAXARGS)
  {
    *space = 0;
    cargs[lastarg] = cur;
    strncpy(cargs[lastarg], s, 255);
    cur += strlen(cargs[lastarg]) + 1;

    iargs[lastarg] = strtol(s, NULL, 0);
    hargs[lastarg] = strtol(s, NULL, 16);
    dargs[lastarg] = strtol(s, NULL, 10);
    s = space + 1;

    lastarg++;
  }
}

void hexprint(FILE *out, char *x, int size, int ascii_print)
{
  int i, half;
  unsigned char c;
  char ascii[130];
  half = (size / 2) - 1;
  if (size > 128)
  {
    fprintf(stderr, "hexprintf given illegal size %d\n", size);
    exit(1);
  };
  memset(ascii, 0, 130);
  for (i = 0; i < size; i++)
  {
    c = x[i];

    if (i == half)
      fprintf(out, "%02x . ", c);
    else
      fprintf(out, "%02x ", c);

    if (ascii_print)
    {
      if (c > 126)
        c &= 127;
      if (c < 31)
        c |= 32;
      ascii[i] = c;
    }
  }
  if (size < 16)
    while (16 - size)
    {
      fprintf(out, "   ");
      size++;
    }
  if (ascii_print)
    fprintf(out, "  |  %s\n", ascii);
}

void printsectheader(FILE *out, DC42ImageType *F, uint32 sector)
{
  uint32 fileid;
  fprintf(out, "\n-----------------------------------------------------------------------------\n");
  // fprintf(out,"Sec %d:(0x%04x) Cluster:%d, csize:%d ",sector,sector,sector/clustersize,clustersize);

  fprintf(out, "Sec %d:(0x%04x)  ", sector, sector);

  if (havetags)
  {
    if (tagsaresorted) // when tags are sorted, allocation bitmap is extracted.
      fprintf(out, " %s Block ",
              ((allocated[sector] & 8) ? "Used" : "Free"));
    fileid = TAGFILEID(sector);
    fprintf(out, "Part of file %s:\"%s\"", getfileid(sector), filenames[fileid][0]);
  }

  fprintf(out, "\n-----------------------------------------------------------------------------\n");
  if (F->tagsize)
  {
    if (F->tagsize == 12)
      fprintf(out, "            +0 +1 +2 +3 +4 +5 . +6 +7 +8 +9 +a +b\n");
    else
      fprintf(out, "            +0 +1 +2 +3 +4 +5 . +6 +7 +8 +9 +a +b +c +d +e +f+10+11+12+13\n");

    fprintf(out, "tags:       ");
    hexprint(out, (char *)F->read_sector_tags(F, sector), F->tagsize, 0);

    if (F->tagsize == 12)
      fprintf(out, "\n           |volid| ??  |fileid|absnext|next|previous\n");
    else
      fprintf(out, "\n           |volid|????|fileid||absnext|   ?   |    ?   |    next|previous\n");
  }
  // fprintf(out,"\n\n");
  fprintf(out, "-----------------------------------------------------------------------------\n");
  fprintf(out, "      +0 +1 +2 +3 +4 +5 +6 +7 . +8 +9 +a +b +c +d +e +f                    \n");
  fprintf(out, "-----------------------------------------------------------------------------\n");
}

void printtag(FILE *out, DC42ImageType *F, uint32 sector)
{
  char *s;

  switch (allocated[sector])
  {
  case 0:
    s = "free";
    break;
  case 9:
    s = "used";
    break;
  case 1:
    s = "ufid";
    break;
  case 8:
    s = "ubit";
    break;
  default:
    s = "????";
    break;
  }

  fprintf(out, "%4d(%04x): ", sector, sector);
  hexprint(out, (char *)F->read_sector_tags(F, sector), F->tagsize, 0);
  fprintf(out, " %s\n", s);
}

void printsector(FILE *out, DC42ImageType *F, uint32 sector, uint32 sectorsize)
{

  uint16 i;
  char *sec;

  // fprintf(stderr,"\n:sector#: %d sectorsize: %d\n",sector,sectorsize);

  if (sector > F->numblocks)
  {
    fprintf(out, "not that many sectors!\n");
    sector = F->numblocks - 1;
  }

  printsectheader(out, F, sector);
  // sec=&(sectors[sector*sectorsize]);
  sec = (char *) F->read_sector_data(F, sector);
  for (i = 0; i < F->datasize; i += dispsize)
  {
    fprintf(out, "%04x: ", i);
    hexprint(out, (char *)(&sec[i]), dispsize, 1);
  }
  fprintf(out, "\n");

  // printf("\nResults:%d :%s pointer:%p\n",i,F->errormsg,F->RAM);
}

void version_banner(void)
{
  //   ..........1.........2.........3.........4.........5.........6.........7.........8
  //   012345678901234567890123456789012345678901234567890123456789012345678901234567890
  puts("  ---------------------------------------------------------------------------");
  puts("    Lisa File System Shell Tool  v0.99     http://lisaem.sunder.net/lisafsh  ");
  puts("  ---------------------------------------------------------------------------");
  puts("         Copyright (C) MMXXI, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");
}

void cli(DC42ImageType *F)
{
  int i;
  uint16 newsector = 0;

  while (1)
  {
    fflush(stderr);
    fflush(stdout);

    getcommand();

    switch (command)
    {
    case NULL_CMD:
      break;
    case SECTOR_NXT:
      sector++;
      printsector(stdout, F, sector, F->datasize);
      break;
    case SECTOR_PRV:
      sector--;
      printsector(stdout, F, sector, F->datasize);
      break;
    case SECTOR_CMD:
      sector = iargs[0];
      printsector(stdout, F, sector, F->datasize);
      break;
    case CLUSTER_CMD:
      sector = iargs[0] * clustersize / 512;
      printsector(stdout, F, sector, F->datasize);
      break;
    case DISPLAY_CMD:
      printsector(stdout, F, sector, F->datasize);
      break;
    case SETCLUSTERSIZE_CMD:
      clustersize = iargs[0];
      break;
    case DUMP_CMD:
    {
      for (sector = 0; sector < F->numblocks; sector++)
        printsector(stdout, F, sector, F->datasize);
    }
    break;
    case TAGDUMP_CMD:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      {
        puts("\n\n      +0 +1 +2 +3 +4 +5   +6 +7 +8 +9 +a +b");
        puts("-----------------------------------------------------------------------------");
        for (sector = 0; sector < F->numblocks; sector++)
          printtag(stdout, F, sector);
      }
      break;
    case SORTTAGDUMP_CMD:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      puts("\n\n      +0 +1 +2 +3 +4 +5   +6 +7 +8 +9 +a +b");
      puts("-----------------------------------------------------------------------------");
      for (sector = 0; sector < F->numblocks; sector++)
        printtag(stdout, F, sorttag[sector]);
      break;
    case SORTDUMP_CMD:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      for (sector = 0; sector < F->numblocks; sector++)
        printsector(stdout, F, sorttag[sector], F->datasize);
      break;
    case EXTRACT_CMD:
      // extract all Lisa files in disk based on the tags.
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      extract_files(F);
      break;

    case BITMAP_CMD:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      dump_allocation_bitmap(stdout, F);
      break;

    case SORT_NEXT:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      {
        unsigned int i;
        for (i = 0; i < F->numblocks - 1; i++)
          if ((unsigned)sorttag[i] == sector)
          {
            newsector = sorttag[i + 1];
            printsector(stdout, F, sector, F->datasize);
            break;
          }
      }

      if (newsector <= F->numblocks && newsector != sector)
        sector = newsector;
      else
        puts("This sector is the end of this chain.");
      break;

    case SORT_PREV:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      {
        unsigned int i;
        for (i = 1; i <= F->numblocks - 1; i++)
          if ((unsigned)sorttag[i] == sector)
          {
            newsector = sorttag[i - 1];
            printsector(stdout, F, sector, F->datasize);
            break;
          }
      }

      if (newsector <= F->numblocks && newsector != sector)
        sector = newsector;
      else
        puts("This sector is the start of this chain.");
      break;

    case DIR_CMD:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      //             01234567890123456789012345678912
      printf("\nExtent   File Name                           Date Created     Date Modified    Last Access Date        filesizes        attr DRM\n");
      printf("------------------------------------------------------------------------------------------------------------------------------\n%s\n", directory);
      break;

    case DIRX_CMD:
      if (!havetags)
      {
        puts("I can't do that, this image doesn't have tags.");
        break;
      }
      if (!tagsaresorted)
      {
        sorttags(F);
        tagsaresorted = 1;
      }
      extract_file_extents_from_tags(F);
      break;

    case VERSION_CMD:
      version_banner();
      break;

    case EDITSECTOR_CMD:
    {
      int offset = iargs[0];
      int i, j;
      uint8 mysector[2048];

      if (lastarg < 3)
      {
        puts("editsector {offset} {hexdata}");
        puts("                - edit the current sector by writing the hex data at offset");
        puts("                  the change is immediately written to the file.");
        puts("             i.e. 'editsector 0x0F CA FE 01' writes bytes CA FE 01 offset 15");
        break;
      }
      // read the sector into a buffer
      memcpy(mysector, F->read_sector_data(F, sector), F->datasize);

      // modify
      {
        unsigned int i;
        for (i = offset, j = 1; i < F->datasize && cargs[j] != NULL; i++, j++)
          mysector[i] = hargs[j];
      }
      // write
      i = F->write_sector_data(F, sector, mysector);
      printsector(stdout, F, sector, F->datasize);
      if (i)
        printf("Results:%d :%s\n", i, F->errormsg);
    }
    break;

    case EDITTAG_CMD:
    {
      int offset = iargs[0];
      int i, j;
      uint8 mytag[64];

      if (lastarg < 3)
      {
        puts("edittag    {offset} {hexdata}");
        puts("                - edit the current sector's tags by writing the hex data at the");
        puts("                  the offset.  the change is immediately written to the file.");
        puts("             i.e. 'editsector 4 00 00' writes bytes 0000 at offset 4 ");
        puts("                  (it sets the FILEID tag such that it marks the block as free)");

        break;
      }
      // read the sector into a buffer
      memcpy(mytag, F->read_sector_tags(F, sector), F->tagsize);

      { // modify
        unsigned int i;
        for (i = offset, j = 1; i < F->tagsize && cargs[j]; i++, j++)
          mytag[i] = hargs[j];
      }
      // write
      i = F->write_sector_tags(F, sector, mytag);
      if (i)
        printf("****WARNING, WRITE FAILED!****\n");

      printsector(stdout, F, sector, F->datasize);
    }
    break;
    case LOADSEC_CMD:
    {
      int x;
      uint8 mysector[2048];
      FILE *binfile;
      uint32 offset = iargs[0];

      binfile = fopen(cargs[1], "rb");

      if (cargs[1] == NULL)
      {
        printf("Missing parameters\n");
        break;
      }
      if (!binfile)
      {
        printf("Could not open file: %s to load sector at offset:%d (%x)\n", cargs[1], iargs[0]);
        break;
      }

      fseek(binfile, offset, SEEK_SET);
      x = fread(mysector, F->datasize, 1, binfile);
      fclose(binfile);
      i = F->write_sector_data(F, sector, mysector);
      printsector(stdout, F, sector, F->datasize);
    }
    break;

    case LOADBIN_CMD:
    {
      uint8 mysector[2048];
      FILE *binfile;
      uint32 offset = iargs[0];
      uint32 osector = sector;

      errno = 0;
      binfile = fopen(cargs[1], "rb");
      printf("Opening binary file: %s for loading into disk image.\n", cargs[1]);
      if (errno)
        perror("Could not open file.\n");

      if (cargs[1] == NULL)
      {
        printf("Missing parameters\n");
        break;
      }
      if (!binfile)
      {
        printf("Could not open file: %s to load at offset:%d (%x)\n", cargs[1], iargs[0]);
        break;
      }

      fseek(binfile, offset, SEEK_SET);

      while (fread(mysector, F->datasize, 1, binfile) && sector < F->numblocks && !feof(binfile) && !ferror(binfile))
      {
        i = F->write_sector_data(F, sector, mysector); // write the data tothe sector
        printf("Loaded %d bytes into sector:%d from file offset:%ld\n", F->datasize, sector, offset);
        sector++;                // prepare next sector to load
        offset = ftell(binfile); // grab next offset addr so we can print it
      }
      fclose(binfile);

      sector = osector;
    }
    break;

    case LOADPROFILE_CMD:
    {
      FILE *file;
      uint8 mysector[512], tags[24], line[1024];
      unsigned long sector_number = 0, last_sector = 0x00fffffd, offset = -4;

      file = fopen(cargs[0], "r");

      printf("Opening profile dump: %s for reading into disk image.\n", cargs[0]);
      if (errno)
        perror("Could not open file.\n");

      while (!feof(file))
      {
        char *i;
        i = fgets((char *)line, 1024, file);

        if (strncmp((char *)line, "TAGS: ", 5) == 0)
        {
          int i = 6, j = 0;

          while (line[i] && j < 20)
          {
            tags[j] = hex2byte(line[i], line[i + 1]); // printf("Tag[%d]=[%c%c]==%02x\n",j,line[i],line[i+1],tags[j]);
            i += 2;
            j++;

            while (line[i] == ',')
              i++;
          }
        }

        if (strncmp((char *)line, "Sector Number: ", 14) == 0)
        {
          sector_number = hex2long(&line[15]);

          if (sector_number & 0x00800000)
            sector_number |= 0xff000000; // ProFile sector #'s are 24 bit, so sign extend.

          // printf("New sector:%08x previous sector:%08x\n",sector_number,last_sector);

          if (last_sector + 1 != sector_number)
            printf("DANGER! unexpected sector #%d wanted %d\n", sector_number, last_sector);

          offset = -4;
          last_sector = sector_number;
        }

        if (line[8] == ':' && line[17] == ',' && line[26] == ',' && line[35] == ',' && line[44] == ',')
        {
          uint32 newoffset, c1, c2, c3, c4;

          newoffset = hex2long(line);

          c1 = hex2long(&line[9]);
          c2 = hex2long(&line[18]);
          c3 = hex2long(&line[27]);
          c4 = hex2long(&line[36]);

          if (newoffset < 0x7d)
          {
            int i;

            mysector[(newoffset + 0) * 4 + 0] = (uint8)((c1 >> 24) & 0x000000ff);
            mysector[(newoffset + 0) * 4 + 1] = (uint8)((c1 >> 16) & 0x000000ff);
            mysector[(newoffset + 0) * 4 + 2] = (uint8)((c1 >> 8) & 0x000000ff);
            mysector[(newoffset + 0) * 4 + 3] = (uint8)((c1) & 0x000000ff);

            mysector[(newoffset + 1) * 4 + 0] = (uint8)((c2 >> 24) & 0x000000ff);
            mysector[(newoffset + 1) * 4 + 1] = (uint8)((c2 >> 16) & 0x000000ff);
            mysector[(newoffset + 1) * 4 + 2] = (uint8)((c2 >> 8) & 0x000000ff);
            mysector[(newoffset + 1) * 4 + 3] = (uint8)((c2) & 0x000000ff);

            mysector[(newoffset + 2) * 4 + 0] = (uint8)((c3 >> 24) & 0x000000ff);
            mysector[(newoffset + 2) * 4 + 1] = (uint8)((c3 >> 16) & 0x000000ff);
            mysector[(newoffset + 2) * 4 + 2] = (uint8)((c3 >> 8) & 0x000000ff);
            mysector[(newoffset + 2) * 4 + 3] = (uint8)((c3) & 0x000000ff);

            mysector[(newoffset + 3) * 4 + 0] = (uint8)((c4 >> 24) & 0x000000ff);
            mysector[(newoffset + 3) * 4 + 1] = (uint8)((c4 >> 16) & 0x000000ff);
            mysector[(newoffset + 3) * 4 + 2] = (uint8)((c4 >> 8) & 0x000000ff);
            mysector[(newoffset + 3) * 4 + 3] = (uint8)((c4) & 0x000000ff);

            // printf("%08x:(%08x) data:",newoffset*4,newoffset);
            // for (i=newoffset*4; i<(newoffset*4+16) ; i++)
            //        printf("%02x,",mysector[i]);
            // printf("|");
            // for (i=newoffset*4; i<(newoffset*4+16) ; i++) printf("%c",niceascii(mysector[i]));
            // puts("");

            if (newoffset == 0x7c && (sector_number < F->numblocks || sector_number == 0xffffffff))
            {
              i = F->write_sector_tags(F, sector_number + 1, tags);
              if (i)
                printf("Wrote sector #%ld tags as sector #%ld tags status:%d %s", sector_number, sector_number + 1, i, F->errormsg);

              i = F->write_sector_data(F, sector_number + 1, mysector);
              if (i)
                printf("Wrote sector #%ld as sector #%ld status:%d %s", sector_number, sector_number + 1, i, F->errormsg);
            }
          }

          if (offset + 4 != newoffset)
            printf("Sector#:%d Unexpected offset:%08x (wanted:%08x) data might be corrupted\n", newoffset, offset + 4, sector_number);
          offset = newoffset;
        }
      }
      fclose(file);
    }
    break;

    case DIFF2IMG_CMD:
    {
      DC42ImageType F2;
      int sec, count, i, tagsize, secsize, differs = 0;
      uint8 *img1, *img2;
      { // open/convert the second image.
        if (dart_is_valid_image(cargs[0]))
        {
          char dc42filename[FILENAME_MAX];

          fprintf(stderr, "Checking to see if this is a DART image\n");

          strncpy(dc42filename, cargs[0], FILENAME_MAX - 2);
          if (strlen(dc42filename) < FILENAME_MAX - 6)
            strcat(dc42filename, ".dc42");
          else
            strcpy((char *)(dc42filename + FILENAME_MAX - 6), ".dc42");

          printf("Converting DART image %s to DiskCopy42 Image:%s\n", cargs[0], dc42filename);
          i = dart_to_dc42(cargs[0], dc42filename);
          if (!i)
            i = dc42_open(&F2, dc42filename, "w");
          if (i)
          {
            perror("Couldn't open the disk image.");
            break;
          }
        }
        else if (dc42_is_valid_image(cargs[0]))
        {
          fprintf(stderr, "This is a recognized DC42 image, opening it.");
          i = dc42_open(&F2, cargs[0], "w");
          if (i)
          {
            perror("Couldn't open the disk image.");
            break;
          }
        }
        else
        {
          fprintf(stderr, "Cannot recognize this disk image as a valid DC42 or DART image\n");
          exit(1);
        }
        if (i)
          break;
      }

      if (F2.numblocks != F->numblocks)
      {
        printf("Warning! Disk images are different sizes!\n %5d blocks in %s,\n%5d blocks in %s\n\n",
               F->numblocks, F->fname, F2.numblocks, F2.fname);
        differs = 1;
      }

      if (F2.datasize != F->datasize)
      {
        printf("Warning! Disk images have different sized sectors!\n %5d bytes/sec in %s,\n%5d bytes/tag in %s\n\n",
               F->datasize, F->fname, F2.datasize, F2.fname);
        differs = 1;
      }

      if (F2.tagsize != F->tagsize)
      {
        printf("Warning! Disk images have different sized tags!\n %5d bytes/tag in %s,\n%5d bytes/tag in %s\n\n",
               F->tagsize, F->fname, F2.tagsize, F2.fname);
        differs = 0;
      }

      printf("Legend:<%s\n>>%s\n", F->fname, F2.fname);

      count = MIN(F2.numblocks, F->numblocks);
      tagsize = MIN(F2.tagsize, F->tagsize);
      secsize = MIN(F2.datasize, F->datasize);

      if (F2.tagsize == F->tagsize)
        for (sec = 0; sec < count; sec++)
        {
          if (tagsize)
          {
            img1 = F->read_sector_tags(F, sec);
            img2 = F2.read_sector_tags(&F2, sec);
            if (img1 != NULL && img2 != NULL)
              for (i = 0; i < tagsize; i++)
                if (img1[i] != img2[i])
                {
                  printf("sec:%4d tag offset #%2d <%02x >%02x\n", sec, i, img1[i], img2[i]);
                  differs = 1;
                }
          }

          if (secsize)
          {
            img1 = F->read_sector_data(F, sec);
            img2 = F2.read_sector_data(&F2, sec);
            if (img1 != NULL && img2 != NULL)
              for (i = 0; i < secsize; i++)
                if (img1[i] != img2[i])
                {
                  printf("sec:%4d data offset #%03x <%02x >%02x and more...\n", sec, i, img1[i], img2[i]);
                  i = secsize;
                  differs = 1;
                }
          }
        }

      F2.close_image(&F2);
      if (!differs)
        puts("Sectors and tags between these images are identical.");
    }
    break;
    case VOLNAME_CMD:
    {
      int i;
      printf("Current volume name is:\"%s\"\n", dc42_get_volname(F));
      if (cargs[0] != NULL)
      {
        // 012345678
        // volname_
        i = dc42_set_volname(F, &cmd_line[8]);
        if (i)
          printf("Could not set new volume name to \"%s\".\n", &cmd_line[8]);
        else
          printf("New volume name is:\"%s\"\n", dc42_get_volname(F));
      }
    }
    break;
    case QUIT_CMD:
    {
      fprintf(stderr, "Closing Disk Image\n");
      F->close_image(F);
      fprintf(stderr, "bye.\n");
      exit(0);
    }

    case HELP_CMD:

      version_banner();
      //              1         2         3         4         5         6         7         8
      //    0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
      puts("NOTE: This program relies on tag data to perform commands marked by a + sign.");
      puts("It also requires that the disk images be imaged by Apple Disk Copy 4.2.  It");
      puts("can't use DART images.  If you know DART's format details, please contact me.");
      puts("Versions of ADC newer than 4.2 do not extract tags!  Although they claim to be");
      puts("able to convert DART to DC4.2, they strip off tags, rendering them useless.");
      puts("");
      puts("    It only understands Lisa disk images with tags made by Disk Copy 4.2");
      puts("");
      puts("!command        - shell out and run command");
      puts("help            - displays this help screen.");
      puts("version         - displays version and copyright.");
      puts("{sector#}       - jump and display a sector #. (press ENTER for next one.)");
      puts("+|-{#}          - skip forward/backward by # of sectors.");

      puts("editsector {offset} {hexdata...}");
      puts("                - edit the current sector by writing the hex data at offset");
      puts("                  the change is immediately written to the file.");
      puts("             i.e. 'editsector 0x0F CA FE 01' writes bytes CA FE 01 offset 15");

      puts("edittag    offset {hexdata...}");
      puts("                - edit the current sector's tags by writing the hex data at the");
      puts("                  the offset.  the change is immediately written to the file.");
      puts("             i.e. 'editsector 4 00 00' writes bytes 0000 at offset 4 ");
      puts("                  (the example marks the FILEID tag as an unallocated block)");

      puts("difftoimg filename");
      puts("                - compare the currently opened disk image to another.");

      puts("loadsec offset filename");
      puts("                - load 512 bytes from the file {filename} at {offset} into");
      puts("                  the current sector number, and write that sector to the");
      puts("                  currently opened disk image. {offset} may be hex/dec/octal");
      puts("                  i.e. 'loadsec 0x1000 myfile.bin' opens the myfile.bin,");
      puts("                  reads the 512 bytes at offset 4096-4607 into the current");
      puts("                  sector, and writes the data to the currently opened image.");
      puts("loadbin offset filename");
      puts("                - load an entire file {filename} starting at {offset} into");
      puts("                  the current sector number and subsequent sectors of the");
      puts("                  currently opened disk image. {offset} may be hex/dec/octal");
      puts("                  i.e. 'loadbin 0x1000 myfile.bin' opens the myfile.bin,");
      puts("                  reads the first 512 bytes from offset 4096-4607 into the");
      puts("                  current sector, the next into the next sectorm until the");
      puts("                  end of file or last sector in the image is reached.");
      puts("loadprofile filename");
      puts("                - parse and load a ProFile hex dump received from nanoBug");
      puts("                  into the current disk image.  Sector FFFFFF from the dump");
      puts("                  becomes sector 0, sector 0 in the dump becomes sec 1, etc.");
      puts("                  Format of the hex dump is:\n");
      puts("                  Sector Number: 00FFFFFF");
      puts("                  TAGS: 00000000,00000000,00000000,00000000,00000000");
      puts("                  00000000:00021420,03000001,00000281,0002C1FF,");
      puts("                  00000004:FFFFFFFF,FFFFFFFF,FFFFFFFF,FFFFFFFF,");
      puts("                  00000008:FFFFFFFF,FFFFFFFF,FFFFFFFF,FFFFFFFF,");
      puts("                  ...");
      puts("                  0000007C:FFFFFFFF,FFFFDB6D,DB6DDB6D,DB6DDB6D,");
      puts("                  data is index:data,data,data,data where index");
      puts("                  is the offset/4.");
      puts("");

      //              1         2         3         4         5         6         7         8
      //    0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789

      puts("display         - display a sector #");
      // puts("cluster         - display the 1st sector in a cluster.");
      // puts("setclustersize  - set the cluster size.");
      puts("dump            - dump all sectors and tags in hex display");
      puts("n/p             + next/previous display in sorted chain.");
      puts("tagdump         + dump tags");
      puts("sorttagdump     + sort tags by file ID and sector #, then dump them");
      puts("sortdump        + same as above, but also output actual sectors");
      puts("bitmap          + show block allocation bitmap");
      puts("extract         + extracts all files on disk based on tag data.");
      puts("                  files are written to the current directory and named based");
      puts("                  on the file id and Disk Copy image name.");
      puts("dir             + list tag file ID's and extracted file names from catalog.");
      puts("volname {newvolname}");
      puts("                - if a new volume name is provided, it's written to the image");
      puts("                  otherwise, the current volume name is displayed.  The default");
      puts("                  volname is \"-not a Macintosh disk-\" for non MFS/HFS img's");
      puts("quit            - exit program.");
      puts("");
#ifdef HAVEREADLINE
      puts("    GNU Readline support is compiled in, you can use history and tab completion.");
#endif
      break;

      //              1         2         3         4         5         6         7         8
      //    0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789
    }
  }
}

int main(int argc, char *argv[])
{
  int i = 0, argoffset = 0;

  version_banner();
  if (argc < 2)
  {
    puts("Usage: lisafsh file.dc42 to open an existing DiskCopy 4.2 image");
    puts(" or    lisafsh --new file.dc42 size to create a new image and open it.");
    puts("");
    puts("valid sizes are: 400k 800k 860k 720k 1440k 2880k 5m 10m 20m");
    puts("a default of 12 byte tags will be added to 400k/800k/860K images.");
    puts("if the image size is 720k/1440k/2880k - no tags will be added.");
    puts("if the image size is over 5mb, it will be created with 24 byte tags.");
    puts("");
    puts("i.e.   lisafsh --new file.dc42 400k - create a single sided new image");
    puts("i.e.   lisafsh --new profile5.dc42 5m - create a 5mb image");
    exit(0);
  }

  // F.fhandle=fopen(argv[1],"rb");
  // F.filename=argv[1];

  if (strncmp(argv[1], "--new", 8) == 0)
  {
    int i, size = -1, tagsize = -1;
    argoffset = 1;

    // GCR Lisa/Mac disk sizes
    if (strncmp(argv[3], "400k", 7) == 0)
    {
      size = 800 * 512;
      tagsize = 800 * 12;
    }
    if (strncmp(argv[3], "800k", 7) == 0)
    {
      size = 1600 * 512;
      tagsize = 1600 * 12;
    }
    if (strncmp(argv[3], "860k", 7) == 0)
    {
      size = 1720 * 512;
      tagsize = 1720 * 12;
    }

    // MFM PC disk sizes
    if (strncmp(argv[3], "720k", 7) == 0)
    {
      size = 1440 * 512;
      tagsize = 0;
    }
    if (strncmp(argv[3], "1440k", 7) == 0)
    {
      size = 2880 * 512;
      tagsize = 0;
    }
    if (strncmp(argv[3], "2880k", 7) == 0)
    {
      size = 5760 * 512;
      tagsize = 0;
    }

    // ProFile/Widget.  9728+1 for spares
    if (strncmp(argv[3], "5m", 7) == 0)
    {
      size = 9728 * 512;
      tagsize = 9729 * 24;
    }
    if (strncmp(argv[3], "10m", 7) == 0)
    {
      size = 2 * 9728 * 512;
      tagsize = size / 512 * 24;
    }
    if (strncmp(argv[3], "20m", 7) == 0)
    {
      size = 20 * 1024 * 1024;
      tagsize = size / 512 * 24;
    }
    if (strncmp(argv[3], "30m", 7) == 0)
    {
      size = 30 * 1024 * 1024;
      tagsize = size / 512 * 24;
    }

    if (size == -1)
    {
      printf("Invalid size parameter. Expected 400k 800k 860k 720k 1440k 2880k 5m 10m 20m\n");
      exit(3);
    }

    i = dc42_create(argv[1 + argoffset], "-not a Macintosh disk-", size, tagsize);
    if (!i)
    {
      printf("New Disk Image:%s created successfully. (%d data bytes, %d tag bytes)\n",
             argv[1 + argoffset],
             size,
             tagsize);
    }
    else
    {
      printf("Could not create the disk image. error %d returned\n", i);
      perror("");
      exit(3);
    }
  }

  i = dc42_auto_open(&F, argv[1 + argoffset], "wb");
  if (i)
  {
    fflush(stderr);
    fflush(stdout);
    fprintf(stderr, "Cannot recognize this disk image as a valid DC42 or DART (Fast-Compressed) image:%d\n%s", i, F.errormsg);
    fflush(stderr);
    fflush(stdout);
    exit(1);
  }

  printf("File size:     %d\n", F.size);
  printf("Filename:      %s\n", F.fname);
  printf("Sectors:       %d\n", F.numblocks);

  printf("Tag size each: %d\n", F.tagsize);
  printf("Sec size each: %d\n", F.sectorsize);
  printf("Sector bytes:  %d\n", F.datasizetotal);
  printf("Tag bytes:     %d\n", F.tagsizetotal);

  printf("Image Type:    %d (0=twig, 1=sony400k, 2=sony800k)\n", F.ftype);
  printf("Tagstart @     %d\n", F.tagstart);
  printf("Sectorstart @  %d\n", F.sectoroffset);
  printf("Maxtrack:      %d\n", F.maxtrk);
  printf("Maxsec/track:  %d\n", F.maxsec);
  printf("Max heads:     %d\n", F.maxside);
  // printf("Image mmapped  @%p\n", F.RAM);
  printf("Calc DataChks: %08x\n", dc42_calc_data_checksum(&F));
  printf("Img DataChks:  %02x%02x%02x%02x\n",
         F.RAM[72],
         F.RAM[73],
         F.RAM[74],
         F.RAM[75]);

  printf("Calc TagChks:  %08x\n", dc42_calc_tag_checksum(&F));
  printf("Calc TagChk0:  %08x\n", dc42_calc_tag0_checksum(&F));

  printf("Img TagChks:   %02x%02x%02x%02x\n",
         F.RAM[76],
         F.RAM[77],
         F.RAM[78],
         F.RAM[79]);

  // clear the volume name, sorting the tags will set it.
  memset(volumename, 0, 31);
  memset(directory, 0, 65535);

  if (dc42_has_tags(&F))
    if (!tagsaresorted)
    {
      fprintf(stderr, "Sorting tags\n");
      sorttags(&F);
      tagsaresorted = 1;
      havetags = 1;
    }

  if (i)
  {
    fprintf(stderr, "Had trouble reading the disk image, code:%d", i);
  }

  // printf("Pointer to RAM:%p",F.RAM);
  cli(&F);
  F.close_image(&F);
  return 0;
}
