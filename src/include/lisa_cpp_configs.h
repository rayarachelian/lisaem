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
\**************************************************************************************/

#define EMULATION_TICK 100 // this should be higher than the one below
#define EMULATION_TIME 75  // to allow other processing such as mouse events

// How many changes on the display before refreshing the entire display instead of the updated area?
// want to keep this value small, otherwise a clear desktop or such will be very slow.  The idea here
// is to avoid full repaints on small changes, such as the mouse moving.
//

// minimum skinned window size, this is smaller on purpose so that it will work with 12"
// notebook displays
#define IWINSIZEX 1020
#define IWINSIZEY 720

#define IWINSIZE IWINSIZEX, IWINSIZEY

// the size of the skin
#define ISKINSIZEX 1485
#define ISKINSIZEY 1031
#define ISKINSIZE ISKINSIZEX, ISKINSIZEY

#define FLOPPY_LEFT 1099
#define FLOPPY_TOP 481

#define POWER_LEFT 0
#define POWER_TOP 738

// padding for skinless mode
#define WINXDIFF 30
#define WINYDIFF 65

// binary AND filter for how often to update
// the skinless background filter.  has to be 3,7,15,31,63
// larger the value the longer the delay between updates.
#define SKINLESS_UPDATE_FILTER 3

#include <wx/wx.h>
#include <wx/defs.h>
#include <wx/rawbmp.h>
#include <wx/bitmap.h>
#include <wx/icon.h>
#include <wx/dcbuffer.h>
#include <wx/wxhtml.h>
#include <wx/fs_zip.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <wx/filedlg.h>
#include <wx/fileconf.h>
#include <wx/statusbr.h>
#include <wx/scrolwin.h>
#include <wx/sound.h>
#include <wx/config.h>
#include <wx/clipbrd.h>
#include <wx/datetime.h>
#include <wx/stopwatch.h>
#include <wx/display.h>
#include <wx/gdicmn.h>
#include <wx/pen.h>
#include <wx/scrolwin.h>
#include <wx/notebook.h>
#include <wx/aboutdlg.h>
#include <wx/stdpaths.h>
#include <wx/choicdlg.h>

#include "machine.h"
#include "keyscan.h"

// Bridge to C portions of the emulator.
extern "C"
{
#include "vars.h"
    int32 reg68k_external_execute(int32 clocks);
    void unvars(void);
}

#include "LisaConfig.h"
#include "LisaConfigFrame.h"

#include "lisawin.h"
#include "lisaemframe.h"

// sounds, images, etc.
#include "lisaem_static_resources.h"

#ifdef __WXOSX__
#include <CoreFoundation/CoreFoundation.h>
#include <sys/param.h>
#endif

#define MAXLOOPS 250

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
