/**************************************************************************************\
*                                                                                      *
*                    Xenix Boot Floppy Patcher for X/ProFile                           *
*                                                                                      *
\**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <libdc42.h>

/*
From: http://sigmasevensystems.com/xpf_xenix.html

Lisa Xenix 3.0 ProFile Driver Patches as of 2006-02-10

Using FEdit or equivalent, make the following modifications to a copy of the Xenix Boot Floppy:

NOTE: Write your changes to disk before performing a new search or changing sectors.
FEdit does not warn you if you have made changes that will be lost by changing sectors.

a) To always send 6 command bytes

Sector:   $22 (#34)
Search:   0CAE 0000 0001 0010 6614 177C 000A 0009 1B7C 0002 0001 177C 0004 0009
Replace:  177C 000A 0009 177C 0004 0009 0CAE 0000 0001 0010 6608 1B7C 0002 0001

Sectors:  $CF (#207), $183 (#387)
Search:   082C 0000 0003 6714 177C 000A 0009 1B7C 0002 0001 177C 0004 0009
Replace:  177C 000A 0009 177C 0004 0009 082C 0000 0003 6708 1B7C 0002 0001

b) To not assert CMD before BSY interrupt is enabled

Sectors:  twice in $22 (#34), twice in $CF (#207), $183 (#387), $184 (#388)
Search:   0079 08EB 0004 0001
Replace:  0079 4E71 4E71 4E71 (change 6 bytes)

c) To assert CMD after BSY interrupt is enabled and accept fast response

Sectors:  $22 (#34), $CF (#207), $183 (#387)
Search:   0200 0002 0C00 0002 6680
Replace:  4E71 08EB 0004 0001 6080 (change 9 bytes)
*/

int patchXenix(DC42ImageType *F, int do_patch)
{
   int retval = 0;

   // a - always send 6 bytes
   uint8 srch1_always_send_6cmdbytes[] = {0x0C, 0xAE, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x66, 0x14, 0x17, 0x7C, 0x00, 0x0A, 0x00, 0x09, 0x1B, 0x7C, 0x00, 0x02, 0x00, 0x01, 0x17, 0x7C, 0x00, 0x04, 0x00, 0x09};
   uint8 repl1_always_send_6cmdbytes[] = {0x17, 0x7C, 0x00, 0x0A, 0x00, 0x09, 0x17, 0x7C, 0x00, 0x04, 0x00, 0x09, 0x0C, 0xAE, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x66, 0x08, 0x1B, 0x7C, 0x00, 0x02, 0x00, 0x01};
   uint8 srch2_always_send_6cmdbytes[] = {0x08, 0x2C, 0x00, 0x00, 0x00, 0x03, 0x67, 0x14, 0x17, 0x7C, 0x00, 0x0A, 0x00, 0x09, 0x1B, 0x7C, 0x00, 0x02, 0x00, 0x01, 0x17, 0x7C, 0x00, 0x04, 0x00, 0x09};
   uint8 repl2_always_send_6cmdbytes[] = {0x17, 0x7C, 0x00, 0x0A, 0x00, 0x09, 0x17, 0x7C, 0x00, 0x04, 0x00, 0x09, 0x08, 0x2C, 0x00, 0x00, 0x00, 0x03, 0x67, 0x08, 0x1B, 0x7C, 0x00, 0x02, 0x00, 0x01};
   // b don't assert CMD before BSY IRQ enabled
   uint8 srch_nocmd_b4_bsy_irq[] = {0x00, 0x79, 0x08, 0xEB, 0x00, 0x04, 0x00, 0x01};
   uint8 repl_nocmd_b4_bsy_irq[] = {0x00, 0x79, 0x4E, 0x71, 0x4E, 0x71, 0x4E, 0x71};
   // c To assert CMD after BSY interrupt is enabled and accept fast response
   uint8 srch_cmd_after_bsy_irq[] = {0x02, 0x00, 0x00, 0x02, 0x0C, 0x00, 0x00, 0x02, 0x66, 0x80};
   uint8 repl_cmd_after_bsy_irq[] = {0x4E, 0x71, 0x08, 0xEB, 0x00, 0x04, 0x00, 0x01, 0x60, 0x80};

   // if (sizeof(srch1_always_send_6cmdbytes) !=  sizeof(repl1_always_send_6cmdbytes)) ALERT_LOG(0,"Size mismatch! 1_always_send_6cmdbytes");
   // if (sizeof(srch2_always_send_6cmdbytes) !=  sizeof(repl2_always_send_6cmdbytes)) ALERT_LOG(0,"Size mismatch! 2_always_send_6cmdbytes");
   // if (sizeof(srch_nocmd_b4_bsy_irq)       !=  sizeof(repl_nocmd_b4_bsy_irq))       ALERT_LOG(0,"Size mismatch! nocmd_b4_bsy_irq");
   // if (sizeof(srch_cmd_after_bsy_irq)      !=  sizeof(repl_cmd_after_bsy_irq))      ALERT_LOG(0,"Size mismatch! cmd_after_bsy_irq");

   // a
   int s1034 = searchseccount(F, 34, sizeof(srch1_always_send_6cmdbytes), srch1_always_send_6cmdbytes);
   int s2207 = searchseccount(F, 207, sizeof(srch2_always_send_6cmdbytes), srch2_always_send_6cmdbytes);
   int s2387 = searchseccount(F, 387, sizeof(srch2_always_send_6cmdbytes), srch2_always_send_6cmdbytes);
   // b
   int s3034 = searchseccount(F, 34, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq);
   int s3207 = searchseccount(F, 207, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq);
   int s3387 = searchseccount(F, 387, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq);
   int s3388 = searchseccount(F, 388, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq);
   // c
   int s4034 = searchseccount(F, 34, sizeof(srch_cmd_after_bsy_irq), srch_cmd_after_bsy_irq);
   int s4207 = searchseccount(F, 207, sizeof(srch_cmd_after_bsy_irq), srch_cmd_after_bsy_irq);
   int s4387 = searchseccount(F, 387, sizeof(srch_cmd_after_bsy_irq), srch_cmd_after_bsy_irq);

   // a
   int r1034 = searchseccount(F, 34, sizeof(repl1_always_send_6cmdbytes), repl1_always_send_6cmdbytes);
   int r2207 = searchseccount(F, 207, sizeof(repl2_always_send_6cmdbytes), repl2_always_send_6cmdbytes);
   int r2387 = searchseccount(F, 387, sizeof(repl2_always_send_6cmdbytes), repl2_always_send_6cmdbytes);
   // b
   int r3034 = searchseccount(F, 34, sizeof(repl_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq);
   int r3207 = searchseccount(F, 207, sizeof(repl_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq);
   int r3387 = searchseccount(F, 387, sizeof(repl_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq);
   int r3388 = searchseccount(F, 388, sizeof(repl_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq);
   // c
   int r4034 = searchseccount(F, 34, sizeof(repl_cmd_after_bsy_irq), repl_cmd_after_bsy_irq);
   int r4207 = searchseccount(F, 207, sizeof(repl_cmd_after_bsy_irq), repl_cmd_after_bsy_irq);
   int r4387 = searchseccount(F, 387, sizeof(repl_cmd_after_bsy_irq), repl_cmd_after_bsy_irq);

   if (s1034 == 1 && s2207 == 1 && s2387 == 1)
      retval |= 1;
   if (s3034 == 2 && s3207 == 2 && s3387 == 1 && s3388 == 1)
      retval |= 2;
   if (s4034 && s4207 && s4387)
      retval |= 4;

   if (r1034 == 1 && r2207 == 1 && r2387 == 1)
      retval |= 8;
   if (r3034 == 2 && r3207 == 2 && r3387 == 1 && r3388 == 1)
      retval |= 16;
   if (r4034 && r4207 && r4387)
      retval |= 32;

   if (do_patch)
   {
      // apply patches a,b,c
      if (s1034 == 1 && s2207 == 1 && s2387 == 1 && (do_patch & 1))
      {
         replacesec(F, 34, sizeof(srch1_always_send_6cmdbytes), srch1_always_send_6cmdbytes, repl1_always_send_6cmdbytes);
         replacesec(F, 207, sizeof(srch2_always_send_6cmdbytes), srch2_always_send_6cmdbytes, repl2_always_send_6cmdbytes);
         replacesec(F, 387, sizeof(srch2_always_send_6cmdbytes), srch2_always_send_6cmdbytes, repl2_always_send_6cmdbytes);
      }

      if (s3034 == 2 && s3207 == 2 && s3387 == 1 && s3388 == 1 && (do_patch & 2))
      {
         replacesec(F, 34, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq, repl_nocmd_b4_bsy_irq);
         replacesec(F, 207, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq, repl_nocmd_b4_bsy_irq);
         replacesec(F, 387, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq, repl_nocmd_b4_bsy_irq);
         replacesec(F, 388, sizeof(srch_nocmd_b4_bsy_irq), srch_nocmd_b4_bsy_irq, repl_nocmd_b4_bsy_irq);
      }

      if (s4034 && s4207 && s4387 && (do_patch & 4))
      {
         replacesec(F, 34, sizeof(srch_cmd_after_bsy_irq), srch_cmd_after_bsy_irq, repl_cmd_after_bsy_irq);
         replacesec(F, 207, sizeof(srch_cmd_after_bsy_irq), srch_cmd_after_bsy_irq, repl_cmd_after_bsy_irq);
         replacesec(F, 387, sizeof(srch_cmd_after_bsy_irq), srch_cmd_after_bsy_irq, repl_cmd_after_bsy_irq);
      }

      // revert patches a,b,c

      if (r1034 == 1 && r2207 == 1 && r2387 == 1 && (do_patch & 8))
      {
         replacesec(F, 34, sizeof(srch1_always_send_6cmdbytes), repl1_always_send_6cmdbytes, srch1_always_send_6cmdbytes);
         replacesec(F, 207, sizeof(srch2_always_send_6cmdbytes), repl2_always_send_6cmdbytes, srch2_always_send_6cmdbytes);
         replacesec(F, 387, sizeof(srch2_always_send_6cmdbytes), repl2_always_send_6cmdbytes, srch2_always_send_6cmdbytes);
      }

      if (r3034 == 2 && r3207 == 2 && r3387 == 1 && r3388 == 1 && (do_patch & 16))
      {
         replacesec(F, 34, sizeof(srch_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq, srch_nocmd_b4_bsy_irq);
         replacesec(F, 207, sizeof(srch_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq, srch_nocmd_b4_bsy_irq);
         replacesec(F, 387, sizeof(srch_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq, srch_nocmd_b4_bsy_irq);
         replacesec(F, 388, sizeof(srch_nocmd_b4_bsy_irq), repl_nocmd_b4_bsy_irq, srch_nocmd_b4_bsy_irq);
      }

      if (r4034 && r4207 && r4387 && (do_patch & 32))
      {
         replacesec(F, 34, sizeof(srch_cmd_after_bsy_irq), repl_cmd_after_bsy_irq, srch_cmd_after_bsy_irq);
         replacesec(F, 207, sizeof(srch_cmd_after_bsy_irq), repl_cmd_after_bsy_irq, srch_cmd_after_bsy_irq);
         replacesec(F, 387, sizeof(srch_cmd_after_bsy_irq), repl_cmd_after_bsy_irq, srch_cmd_after_bsy_irq);
      }
   }
   return retval;
}

int main(int argc, char *argv[])
{
   int i, j, err, stat;
   DC42ImageType F;
   char *Image = NULL;

   if (argc < 2)
   {
      fprintf(stderr, "Usage: %s xenixdisk.dc42\n", argv[0]);
      exit(0);
   }

   int do_patch = 15;
   for (i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "-a") == 0)
      {
         if (do_patch == 15)
            do_patch = 0;
         do_patch |= 1;
         do_patch &= ~8;
      }
      else if (strcmp(argv[i], "-b") == 0)
      {
         if (do_patch == 15)
            do_patch = 0;
         do_patch |= 2;
         do_patch &= ~16;
      }
      else if (strcmp(argv[i], "-c") == 0)
      {
         if (do_patch == 15)
            do_patch = 0;
         do_patch |= 4;
         do_patch &= ~32;
      }
      else if (strcmp(argv[i], "+a") == 0)
      {
         if (do_patch == 15)
            do_patch = 0;
         do_patch |= 8;
         do_patch &= ~1;
      }
      else if (strcmp(argv[i], "+b") == 0)
      {
         if (do_patch == 15)
            do_patch = 0;
         do_patch |= 16;
         do_patch &= ~2;
      }
      else if (strcmp(argv[i], "+c") == 0)
      {
         if (do_patch == 15)
            do_patch = 0;
         do_patch |= 32;
         do_patch &= ~4;
      }
      else if (strcmp(argv[i], "-n") == 0)
      {
         do_patch = -1;
      }
      else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
      {
         printf("Patch Xenix Utility. see: http://sigmasevensystems.com/xpf_xenix.html\n\n"
                //  01234567890123456789012345678901234567890123456789012345678901234567890123456789
                //  0         1         2         3         4         5         6         7         8
                "Usage: patchxenix {-a} {-b} {-c} {-n} Xenix_Boot_Disk.dc42\n\n"

                "       -a always send 6 command bytes.                +a revert patch a\n"
                "       -b don't assert CMD before BSY IRQ enable      +b revert patch b\n"
                "       -c assert CMD after BSY IRQ + Fast             +c revert patch c\n"
                "       -n Don't patch, just report status.      \n\n"
                "This utility is used to patch the Microsoft/SCO Xenix Boot Disk\n"
                "in order to make it more compatible with Sigma Seven Systems' X/ProFile.\n"
                "and other software such as LisaEm.\n");

         exit(0);
      }
      else
      {
         if (!Image)
            Image = argv[i];
         else
         {
            fprintf(stderr, "Can only specify one filename.  Surround it by quotes if it contains spaces.\n");
            exit(2);
         }
      }
   }
   if (do_patch < 0)
      do_patch = 0;

   err = dc42_auto_open(&F, Image, "wb");
   if (err)
   {
      fprintf(stderr, "could not open: '%s' because:%s\n", Image, F.errormsg);
      exit(1);
   }

   if (do_patch)
      patchXenix(&F, do_patch);

   stat = patchXenix(&F, 0);
   printf("%s is %spatched to (a) always send 6 command bytes\n", Image, (stat & 1 ? "un" : ""));
   printf("%s is %spatched to (b) not assert CMD before the BSY IRQ Enable\n", Image, (stat & 2 ? "un" : ""));
   printf("%s is %spatched to (c) assert CMD after the BSY IRQ Enable\n", Image, (stat & 4 ? "un" : ""));

   F.close_image(&F);
   return 0;
}
