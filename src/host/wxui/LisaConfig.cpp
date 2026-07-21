/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.6      DEV 2007.12.04                   *
*                             http://lisaem.sunder.net                                 *
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
*           Foundation, Inc., 59 Temple Place #330, Boston, MA 02111-1307, USA.        *
*                                                                                      *
*                   or visit: http://www.gnu.org/licenses/gpl.html                     *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/stdpaths.h>

#include <machine.h>
#include <LisaConfig.h>

extern "C"
{
   typedef struct _lisa_clock
   {
      uint8 year;
      uint8 days_h;
      uint8 days_l;
      uint8 hours_h;
      uint8 hours_l;
      uint8 mins_h;
      uint8 mins_l;
      uint8 secs_h;
      uint8 secs_l;
      uint8 tenths;
   } t_lisa_clock;

   extern t_lisa_clock lisa_clock;

   extern int cheat_ram_test;

   extern float hidpi_scale;

   extern uint8 floppy_iorom;

   extern int double_sided_floppy;
   extern int hle;
   extern int macworks4mb;
   extern int consoletermwindow;
}

extern char *getDocumentsDir(void);

void LisaConfig::Load(wxFileConfig *config, uint8 *floppy_ram)
{

   iw_png_on = (int)config->Read(_T("/imagewriter/saveaspng"), (long)0);
   iw_png_path = config->Read(_T("/imagewriter/dirpath"  ));
   iw_dipsw_1 = (int)config->Read(_T("/imagewriter/dipsw1"   ), (long)210);
   if (iw_png_path.Len() < 1)
   {
      wxStandardPathsBase &stdp = wxStandardPaths::Get();
      iw_png_path = stdp.GetDocumentsDir();
   }

   parallel = config->Read(_T("/parallelport/parallelport"));
   parallelp = config->Read(_T("/parallelport/path"));
   if (parallel.Len() < 4 || parallelp.Len() < 4)
   {
      parallel = _T("PROFILE");
      parallelp = _T("lisaem-profile.dc42");
   }

   if (!config->Read(_T("/cardslot1/slot1"), &slot1))
      slot1 = _T("");
   else
   {
      config->Read(_T("/cardslot1/low"), &s1l);
      config->Read(_T("/cardslot1/lowpath"), &s1lp);
      config->Read(_T("/cardslot1/high"), &s1h);
      config->Read(_T("/cardslot1/highpath"), &s1hp);
   }

   if (!config->Read(_T("/cardslot2/slot2"), &slot2))
      slot2 = _T("");
   else
   {
      config->Read(_T("/cardslot2/low"), &s2l);
      config->Read(_T("/cardslot2/lowpath"), &s2lp);
      config->Read(_T("/cardslot2/high"), &s2h);
      config->Read(_T("/cardslot2/highpath"), &s2hp);
   }

   if (!config->Read(_T("/cardslot3/slot3"), &slot3))
      slot3 = _T("");
   else
   {
      config->Read(_T("/cardslot3/low"), &s3l);
      config->Read(_T("/cardslot3/lowpath"), &s3lp);
      config->Read(_T("/cardslot3/high"), &s3h);
      config->Read(_T("/cardslot3/highpath"), &s3hp);
   }

   config->SetPath(_T("/"));

   kbidstr = config->Read(_T("/keyboardid"));
   kbidstr = _T("0x") + kbidstr;
   if (kbidstr.ToULong(&kbid, 16) == false)
      kbid = 0xBF2f;
   kbidstr.Printf(_T("%04x"), (uint16)kbid);

   myserial = config->Read(_T("/serialnumber"), _T(LISA_CONFIG_DEFAULTSERIAL));

   cheat_ram_test = (int)config->Read(_T("/cheatromtests"), 1);
   double_sided_floppy = (int)config->Read(_T("/doublesided"  ), 0L);
   hle = (int)config->Read(_T("/hle"          ), 1);
   macworks4mb = (int)config->Read(_T("/4mbmacworks"  ), 1);

   serial1_setting = config->Read(_T("/seriala/connecta"));
   serial1xon = config->Read(_T("/seriala/xon"), "1");
   serial1_param = config->Read(_T("/seriala/parama"));
   serial2_setting = config->Read(_T("/serialb/connectb"));
   serial2xon = config->Read(_T("/serialb/xon"), "1");
   serial2_param = config->Read(_T("/serialb/paramb"));

   ioromstr = config->Read(_T("ioromver"));
   ioromstr = _T("0x") + ioromstr;
   if (ioromstr.ToULong(&iorom, 16) == false)
      iorom = 0xa8;
   floppy_ram[ROMVER] = (iorom & 0xff);
   floppy_iorom = (iorom & 0xff);
   ioromstr.sprintf(_T("%02x"), (uint8)iorom);

   mymaxlisaram = config->Read(_T("/MemoryKB"), 1536l);

   fprintf(stderr, "\nRead config RAM:%d\n\n", mymaxlisaram);

   saw_3a_warning = config->Read(_T("/no_warn_xl_rom"), 0L);
   consoletermwindow = config->Read(_T("/consoletermwindow"), 0L);

   config->Read(_T("/ROMFILE"), &rompath);
   config->Read(_T("/DUALPARALLELROM"), &dualrom);

   // Load parameter memory if inside ini file, else we'll load pram.bin later.
   wxString pramline;  // param  data
   wxString pramaddr;  // param  address
   unsigned int l[32]; // sscanf wants unsigned int, not bytes, so need this for temp space.
   for (int j = 0; j < 1024; j += 32)
   {
      pramaddr.Printf(_T("/pram/pram%04x"), j);
      config->Read(pramaddr, &pramline);

      if (pramline.Len() > (31 * 3))
      {
         char *s = (char *)(const char *)(pramline.c_str());

         sscanf(s, "%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x",
                &l[0], &l[1], &l[2], &l[3],
                &l[4], &l[5], &l[6], &l[7],
                &l[8], &l[9], &l[10], &l[11],
                &l[12], &l[13], &l[14], &l[15],
                &l[16], &l[17], &l[18], &l[19],
                &l[20], &l[21], &l[22], &l[23],
                &l[24], &l[25], &l[26], &l[27],
                &l[28], &l[29], &l[30], &l[31]);

         floppy_ram[j + 0] = (uint8)(l[0]);
         floppy_ram[j + 1] = (uint8)(l[1]);
         floppy_ram[j + 2] = (uint8)(l[2]);
         floppy_ram[j + 3] = (uint8)(l[3]);
         floppy_ram[j + 4] = (uint8)(l[4]);
         floppy_ram[j + 5] = (uint8)(l[5]);
         floppy_ram[j + 6] = (uint8)(l[6]);
         floppy_ram[j + 7] = (uint8)(l[7]);
         floppy_ram[j + 8] = (uint8)(l[8]);
         floppy_ram[j + 9] = (uint8)(l[9]);
         floppy_ram[j + 10] = (uint8)(l[10]);
         floppy_ram[j + 11] = (uint8)(l[11]);
         floppy_ram[j + 12] = (uint8)(l[12]);
         floppy_ram[j + 13] = (uint8)(l[13]);
         floppy_ram[j + 14] = (uint8)(l[14]);
         floppy_ram[j + 15] = (uint8)(l[15]);
         floppy_ram[j + 16] = (uint8)(l[16]);
         floppy_ram[j + 17] = (uint8)(l[17]);
         floppy_ram[j + 18] = (uint8)(l[18]);
         floppy_ram[j + 19] = (uint8)(l[19]);
         floppy_ram[j + 20] = (uint8)(l[20]);
         floppy_ram[j + 21] = (uint8)(l[21]);
         floppy_ram[j + 22] = (uint8)(l[22]);
         floppy_ram[j + 23] = (uint8)(l[23]);
         floppy_ram[j + 24] = (uint8)(l[24]);
         floppy_ram[j + 25] = (uint8)(l[25]);
         floppy_ram[j + 26] = (uint8)(l[26]);
         floppy_ram[j + 27] = (uint8)(l[27]);
         floppy_ram[j + 28] = (uint8)(l[28]);
         floppy_ram[j + 29] = (uint8)(l[29]);
         floppy_ram[j + 30] = (uint8)(l[30]);
         floppy_ram[j + 31] = (uint8)(l[31]);
      }
   }
}

void LisaConfig::Save(wxFileConfig *config, uint8 *floppy_ram)
{
   //-------- save config back to file with modifications if needed ------------

   config->Write(_T("/imagewriter/saveaspng"), iw_png_on);
   config->Write(_T("/imagewriter/dirpath"  ), iw_png_path);
   config->Write(_T("/imagewriter/dipsw1"   ), iw_dipsw_1);

   config->Write(_T("/parallelport/parallelport"), parallel);
   config->Write(_T("/parallelport/path"), parallelp);

   config->Write(_T("/cardslot1/slot1"), slot1);
   config->Write(_T("/cardslot1/low"), s1l);
   config->Write(_T("/cardslot1/high"), s1h);
   config->Write(_T("/cardslot1/lowpath"), s1lp);
   config->Write(_T("/cardslot1/highpath"), s1hp);

   config->Write(_T("/cardslot2/slot2"), slot2);
   config->Write(_T("/cardslot2/low"), s2l);
   config->Write(_T("/cardslot2/high"), s2h);
   config->Write(_T("/cardslot2/lowpath"), s2lp);
   config->Write(_T("/cardslot2/highpath"), s2hp);

   config->Write(_T("/cardslot3/slot3"), slot3);
   config->Write(_T("/cardslot3/low"), s3l);
   config->Write(_T("/cardslot3/high"), s3h);
   config->Write(_T("/cardslot3/lowpath"), s3lp);
   config->Write(_T("/cardslot3/highpath"), s3hp);

   ioromstr.sprintf(_T("%02x"), (uint8)iorom);

   kbidstr.sprintf(_T("%04x"), (uint16)kbid);
   config->Write(_T("/keyboardid"), kbidstr);
   config->Write(_T("/serialnumber"), myserial);

   config->Write(_T("/cheatromtests"), cheat_ram_test);
   config->Write(_T("/hle"), hle);
   config->Write(_T("/4mbmacworks"), macworks4mb);

   config->Write(_T("/doublesided"), double_sided_floppy);

   config->Write(_T("/seriala/connecta"), serial1_setting);
   config->Write(_T("/seriala/xon"), serial1xon);
   config->Write(_T("/seriala/parama"), serial1_param);
   config->Write(_T("/serialb/connectb"), serial2_setting);
   config->Write(_T("/serialb/xon"), serial2xon);
   config->Write(_T("/serialb/paramb"), serial2_param);
   config->Write(_T("/ioromver"), ioromstr);
   config->Write(_T("/MemoryKB"), (long)mymaxlisaram);

   config->Write(_T("/ROMFILE"), rompath);
   config->Write(_T("/DUALPARALLELROM"), dualrom);

   config->Write(_T("/no_warn_xl_rom"), saw_3a_warning);

   config->Write(_T("/consoletermwindow"), consoletermwindow);

   // save parameter RAM
   wxString pramline;
   wxString pramaddr;
   int j;
   char tmp[2048];

   for (j = 0; j < 1024; j += 32)
   {
      pramaddr.Printf(_T("/pram/pram%04x"), j);
      snprintf(tmp, 2047, "%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x",
               (unsigned int)floppy_ram[j + 0], (unsigned int)floppy_ram[j + 1], (unsigned int)floppy_ram[j + 2], (unsigned int)floppy_ram[j + 3],
               (unsigned int)floppy_ram[j + 4], (unsigned int)floppy_ram[j + 5], (unsigned int)floppy_ram[j + 6], (unsigned int)floppy_ram[j + 7],
               (unsigned int)floppy_ram[j + 8], (unsigned int)floppy_ram[j + 9], (unsigned int)floppy_ram[j + 10], (unsigned int)floppy_ram[j + 11],
               (unsigned int)floppy_ram[j + 12], (unsigned int)floppy_ram[j + 13], (unsigned int)floppy_ram[j + 14], (unsigned int)floppy_ram[j + 15],
               (unsigned int)floppy_ram[j + 16], (unsigned int)floppy_ram[j + 17], (unsigned int)floppy_ram[j + 18], (unsigned int)floppy_ram[j + 19],
               (unsigned int)floppy_ram[j + 20], (unsigned int)floppy_ram[j + 21], (unsigned int)floppy_ram[j + 22], (unsigned int)floppy_ram[j + 23],
               (unsigned int)floppy_ram[j + 24], (unsigned int)floppy_ram[j + 25], (unsigned int)floppy_ram[j + 26], (unsigned int)floppy_ram[j + 27],
               (unsigned int)floppy_ram[j + 28], (unsigned int)floppy_ram[j + 29], (unsigned int)floppy_ram[j + 30], (unsigned int)floppy_ram[j + 31]);

      pramline.Printf(_T("%s"), tmp);

      config->Write(pramaddr, pramline);
   }

   config->Write(_T("/clock/days_h"), lisa_clock.days_h);
   config->Write(_T("/clock/days_l" ), lisa_clock.days_l);
   config->Write(_T("/clock/hours_l"), lisa_clock.hours_l);
   config->Write(_T("/clock/mins_l" ), lisa_clock.mins_l);
   config->Write(_T("/clock/secs_l" ), lisa_clock.secs_l);
   config->Write(_T("/clock/year"   ), lisa_clock.year);
   config->Write(_T("/clock/days_h" ), lisa_clock.days_h);
   config->Write(_T("/clock/hours_h"), lisa_clock.hours_h);
   config->Write(_T("/clock/mins_h" ), lisa_clock.mins_h);
   config->Write(_T("/clock/secs_h" ), lisa_clock.secs_h);
   config->Write(_T("/clock/tenths" ), lisa_clock.tenths);

   config->Flush();
}
