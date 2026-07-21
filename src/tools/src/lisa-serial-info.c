/**************************************************************************************\
*                   A part of the Apple Lisa 2 Emulator Project                        *
*                                                                                      *
*                    Copyright (C) 2020  Ray A. Arachelian                             *
*                            All Rights Reserved                                       *
*                                                                                      *
*                      Get Lisa Serial Number Information                              *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <machine.h>

uint8 serialnum240[32];

void get_lisa_serialnumber(uint8 *serialnumber240)
{

  uint32 plant;
  uint32 year;
  uint32 day;
  uint32 sn;
  uint32 prefix;
  uint32 net;
  /*
https://www.applefritter.com/content/apple-lisa-serial-number-info
Number of the Nibble in Hex 01 23 45 678 9ABC D EF

+         +0+1 +2+3 +4+5 +6+7 +8+9 +a+b +c+d +e+f
00000240: 0F0F 0002 0802 0002 0000 0400 0300 0F0F
          XXXX XPXP XYXY XDXD XDXS XSXS XSXX XXXX


Address 240: FF 02 82 020 0403 0 FF
             XX PP YY DDD SSSS X XX

a. Ignore nibbles 0,1,D,E and F, marked as XX or X above.
b. Nibbles 2 and 3 are the two digit plant code (PP).
c. Nibbles 4 and 5 are the two digit year code (YY).
d. Nibbles 6, 7 and 8 are the day of the year code (DDD).
e. Nibbles 9 thru C are the 4 digit serial number (SSSS).

The Applenet Number is similarly embedded in the first 8 bytes of the next
line of the memory dump. So, using the same method as step 4 above, we get:

+        +0+1 +2+3 +4+5 +6+7 +8+9 +a+b +c+d +e+f
00000250 0000|0100|0004|0102|0002|0900|0000|0000
     250: 0 0| 1 0| 0 4| 1 2| 0 2| 9 0| 0 0|0 0
          P P  P N  N N  N N
Number of the Nibble in Hex 012 34567 89ABCDEF

Address 250: 001 00412 02900000
             PPP NNNNN XXXXXXXX
             123 12345

7. To extract the Applenet Number:

a. Ignore nibbles 8 through F, marked as XXXXXXXX above.
b. Nibbles 0, 1 and 2 are the AppleNet prefix (PPP).
c. Nibbles 3 thru 7 are the AppleNet number (NNNNN).

          ff02 8308 1040 50ff 0010 1635 0470 0000
          XXXX XPXP XYXY XDXD XDXS XSXS XSXX XXXX
                              vvvv
mine:     ff028308104050ff0010163504700000
theirs:   ff02080202004030ff00100412029000
                              ^^^^
    0x0f,0x0f,0x00,0x02,0x08,0x03,0x00,0x08,0x01,0x00,0x04,0x00,0x05,0x00,0x0f,0x0f,  // 250
    0x00,0x00,0x01,0x00,0x01,0x06,0x03,0x05,0x00,0x04,0x07,0x00,0x00,0x00,0x00,0x00,  // 260


plant 38 year 00 day 0f0 0654
ff028308104050ff0010163504700000 <<- mine
ff028308104050ff0010163504700000
XXXXXPXPXYXYXDXDXDXSXSXSXSXXXXXX
     3 8|0 0|0 f 0|0 6 5 4     sn=654 -> 1620 dec.
FF028202004030FF <<- example
plant 38, year 0 (1983), day 240, sn 0654

this is all wrong I think:
Your Lisa's serial number was built
in Apple Plant #02 on the 081st day of 1983
with serial #0405 (1029)
It has the applenet id: 001:01635

floppy.c:get_lisa_serialnumber:1532:serial240: ff 02 83 08 10 40 50 ff 00 10 16 35 04 70 00 00
                                               XX XX XP XP XY XY XD XD XD XS XS XS XS XX XX XX
                                                      3  8 |0  0 |0  f  0| 0  6  5  4 |

floppy.c:get_lisa_serialnumber:1538:serial250: 00 00 00 00 00 00 00 00 00 05 08 00 00 00 00 00
floppy.c:get_lisa_serialnumber:1541:Lisa SN: plant:38 born on year:0x0 (0) day:0xf0 (240) sn:0x604 (1540) applenet 0-0
floppy.c:deserialize:1581:Disk signed by Lisa SN: 1 06 63
^- why does the LOS signature at 0x42-0x46 in the extents on tools not match the SN from here if this is correct?


*/
  plant = ((serialnumber240[0x02] & 0x0f) << 4) | (serialnumber240[0x03] & 0x0f);
  year = ((serialnumber240[0x04] & 0x0f) << 4) | (serialnumber240[0x05] & 0x0f);
  day = ((serialnumber240[0x06] & 0x0f) << 8) | ((serialnumber240[0x07] & 0x0f) << 4) | (serialnumber240[0x08] & 0x0f);
  sn = ((serialnumber240[0x09] & 0x0f) << 12) | ((serialnumber240[0x0a] & 0x0f) << 8) | ((serialnumber240[0x0b] << 4) & 0x0f) |
       (serialnumber240[0x0c] & 0x0f);
  // applenet
  prefix = ((serialnumber240[0x10] & 0x0f) << 8) | ((serialnumber240[0x11] & 0x0f) << 4) | (serialnumber240[0x12] & 0x0f);
  net = ((serialnumber240[0x13] & 0x0f) << 16) | ((serialnumber240[0x14] & 0x0f) << 12) | ((serialnumber240[0x15] & 0x0f) << 8) |
        ((serialnumber240[0x16] & 0x0f) << 4) | (serialnumber240[0x17] & 0x0f);

  printf("serial240: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
         serialnumber240[0], serialnumber240[1], serialnumber240[2], serialnumber240[3],
         serialnumber240[4], serialnumber240[5], serialnumber240[6], serialnumber240[7],
         serialnumber240[8], serialnumber240[9], serialnumber240[10], serialnumber240[11],
         serialnumber240[12], serialnumber240[13], serialnumber240[14], serialnumber240[15]);
  printf("serial250: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
         serialnumber240[16], serialnumber240[17], serialnumber240[18], serialnumber240[19],
         serialnumber240[20], serialnumber240[21], serialnumber240[22], serialnumber240[23],
         serialnumber240[24], serialnumber240[25], serialnumber240[26], serialnumber240[27],
         serialnumber240[28], serialnumber240[29], serialnumber240[30], serialnumber240[31]);

  printf("Lisa SN: plant:%x born on year:0x%x (%d) day:0x%x (%d) sn:0x%x (%d) applenet %x-%x\n\n",
         plant, year, year, day, day,
         sn, sn, prefix, net);
}

int main(int argc, char *argv[])
{
  long i, j, k;

  char *temp = NULL;

  temp = malloc(8);
  strncpy(temp, "0xff", 8);
  //            0123
  puts("  ---------------------------------------------------------------------------");
  puts("    Lisa Serial Number Information Tool v0.0.1       http://lisaem.sunder.net");
  puts("  ---------------------------------------------------------------------------");
  puts("          Copyright (C) 2020, Ray A. Arachelian, All Rights Reserved.");
  puts("              Released under the GNU Public License, Version 2.0");
  puts("    There is absolutely no warranty for this program. Use at your own risk.  ");
  puts("  ---------------------------------------------------------------------------\n");

  if (argc <= 1)
  {
    puts("");
    puts("  This program can be fed your Lisa's serial number that you can find via");
    puts("  service mode and give you back information about it. To get this value,");
    puts("  try to boot off the floppy drive without having a floppy inserted, and press");
    puts("  Command-S to enter Service Mode. Then press 1 for display and enter 240 for");
    puts("  the address and 40 for the count. Copy all the alternate byte values down.");
    puts("  It will start with ff at 240. i.e. 0f0f 0002 0803 0106 0200 0205 0600 0f0f");
    puts("  should be copied as ff028316202560ff. The next line contains the AppleNet ID");
    puts("");
    puts("  Usage:   lisa-serial-info hex-string-from-service-mode");
    puts("  i.e.     lisa-serial-info ff028316202560ff0010420404600000");
    puts("                            ^^ Lisa SN #  ^^AppleNet ID #\n");
    exit(0);
  }

  for (i = 1; i < argc; i++)
  {
    memset(serialnum240, 0, 32);
    for (j = 0; ((size_t)j < strlen(argv[i]) && j < 32); j++)
    {
      temp[2] = '0';
      temp[3] = argv[i][j];
      k = strtol(temp, NULL, 16);
      serialnum240[j] = k;
    }
    get_lisa_serialnumber(serialnum240);
  }

  return 0;
}
