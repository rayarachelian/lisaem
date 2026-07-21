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
// Event table for LisaEmFrame
enum
{
    ID_SCREENSHOT = 10001, // anti-aliased screenshot
    ID_SCREENSHOT2,        // screenshot with screen
    ID_SCREENSHOT3,        // raw screenshot - no aliasing

    ID_FUSH_PRNT,

#ifndef __WXMSW__
#ifdef TRACE
    ID_DEBUG,
#endif
#endif

    ID_DEBUGGER,
    ID_POWERKEY,
    ID_APPLEPOWERKEY,

    ID_KEY_APL_DOT,
    ID_KEY_APL_S,
    ID_KEY_APL_ENTER,
    ID_KEY_APL_RENTER,
    ID_KEY_APL_1,
    ID_KEY_APL_2,
    ID_KEY_APL_3,
    ID_KEY_NMI,

    ID_KEY_RESET,

    ID_PROFILEPWR,
    ID_FLOPPY,
    ID_NewFLOPPY,

    ID_RUN,
    ID_PAUSE,

    ID_KEY_OPT_0,
    ID_KEY_OPT_4,
    ID_KEY_OPT_7,
    ID_KEYBOARD,
    ID_ASCIIKB,
    ID_RAWKB,
    ID_RAWKBBUF,

    ID_EMULATION_TIMER,

    ID_THROTTLE5,
    ID_THROTTLE8,
    ID_THROTTLE10,
    ID_THROTTLE12,
    ID_THROTTLE16,
    ID_THROTTLE32,
    ID_THROTTLEX,

    ID_LISAWEB,
    ID_LISAFAQ,

    ID_VID_AA,
    ID_VID_AAG,
    // ID_VID_SCALED,
    ID_VID_DY,
    ID_VID_SY,
    ID_VID_2X3Y,

    ID_REFRESH_60Hz,
    ID_REFRESH_20Hz,
    ID_REFRESH_12Hz,
    ID_REFRESH_8Hz,

    ID_HIDE_HOST_MOUSE,

    ID_VID_SKINS_ON,
    ID_VID_SKINS_OFF
};

// Declare our main frame class

class LisaEmFrame : public wxFrame
{
public:
    int running; // is the Lisa running?  0=off, 1=running, 10=paused/locked.

    // Constructor
    LisaEmFrame(const wxString &title);

    void LoadImages(void);
    void UnloadImages(void);

// Event handlers
#ifdef __WXOSX__
    void OnQuit(wxCommandEvent &event);
#endif

    void OnAbout(wxCommandEvent &event);

    void OnLisaWeb(wxCommandEvent &event);
    void OnLisaFaq(wxCommandEvent &event);

    void OnConfig(wxCommandEvent &event);
    void OnOpen(wxCommandEvent &event);
    void OnSaveAs(wxCommandEvent &event);

    void OnRun(wxCommandEvent &event);
    void OnPause(wxCommandEvent &event);

    void OnScreenshot(wxCommandEvent &event);
    void OnDebugger(wxCommandEvent &event);
    void OnPOWERKEY(wxCommandEvent &event);
    void OnAPPLEPOWERKEY(wxCommandEvent &event);
    void OnTraceLog(wxCommandEvent &event);
    void OnKEY_APL_DOT(wxCommandEvent &event);
    void OnKEY_APL_S(wxCommandEvent &event);
    void OnKEY_APL_ENTER(wxCommandEvent &event);
    void OnKEY_APL_RENTER(wxCommandEvent &event);
    void OnKEY_APL_1(wxCommandEvent &event);
    void OnKEY_APL_2(wxCommandEvent &event);
    void OnKEY_APL_3(wxCommandEvent &event);
    void OnKEY_NMI(wxCommandEvent &event);
    void OnKEY_RESET(wxCommandEvent &event);
    void OnProFilePower(wxCommandEvent &event);
    void OnFLOPPY(wxCommandEvent &event);
    void OnNewFLOPPY(wxCommandEvent &event);

    void OnxFLOPPY(void);
    void OnxNewFLOPPY(void);

    void OnKEY_OPT_0(wxCommandEvent &event);
    void OnKEY_OPT_4(wxCommandEvent &event);
    void OnKEY_OPT_7(wxCommandEvent &event);
    void OnKEYBOARD(wxCommandEvent &event);

    void OnASCIIKB(wxCommandEvent &event);
    void OnRAWKB(wxCommandEvent &event);
    void OnRAWKBBUF(wxCommandEvent &event);
    void reset_throttle_clock(void);

    void OnThrottle5(wxCommandEvent &event);
    void OnThrottle8(wxCommandEvent &event);
    void OnThrottle10(wxCommandEvent &event);
    void OnThrottle12(wxCommandEvent &event);
    void OnThrottle16(wxCommandEvent &event);
    void OnThrottle32(wxCommandEvent &event);
    void OnThrottleX(wxCommandEvent &event);

    void SetStatusBarText(wxString &msg);
    void Update_Status(long elapsed, long idleentry);
    void VidRefresh(long now);
    int EmulateLoop(long idleentry);

    // void OnIdleEvent(wxIdleEvent& event);
    void OnEmulationTimer(wxTimerEvent &event);

    void OnPasteToKeyboard(wxCommandEvent &event);

    void FloppyAnimation(void);
    // menu commands that switch video mode
    void OnVideoAntiAliased(wxCommandEvent &event);
    void OnVideoAAGray(wxCommandEvent &event);
    // void OnVideoScaled(wxCommandEvent& event);
    void OnVideoDoubleY(wxCommandEvent &event);
    void OnVideoSingleY(wxCommandEvent &event);
    void OnVideo2X3Y(wxCommandEvent &event);

    void OnSkinsOn(wxCommandEvent &event);
    void OnSkinsOff(wxCommandEvent &event);

    void OnRefresh60Hz(wxCommandEvent &event);
    void OnRefresh20Hz(wxCommandEvent &event);
    void OnRefresh12Hz(wxCommandEvent &event);
    void OnRefresh8Hz(wxCommandEvent &event);

    void OnFlushPrint(wxCommandEvent &event);
    void OnHideHostMouse(wxCommandEvent &event);

    // class LisaWin *win;
    int screensizex, screensizey;
    int depth;

    wxStopWatch runtime; // idle loop stopwatch
    wxStopWatch soundsw;
    int soundplaying;

    uint16 lastt2;
    long last_runtime_sample;
    long last_decisecond;
    XTIMER clx;
    XTIMER lastclk;
    XTIMER cpu68k_reference;
    XTIMER last_runtime_cpu68k_clx;
    int dwx, dwy;

    wxString wspaste_to_keyboard;
    volatile char *paste_to_keyboard;
    uint32 idx_paste_to_kb;
    int loops;
    float throttle;
    float clockfactor;
    float mhzactual;

    wxString floppy_to_insert;
    long lastcrtrefresh;
    long hostrefresh;
    long screen_paint_update;
    long onidle_calls;
    XTIMER cycles_wanted;

    wxTimer *m_emulation_timer;
    int barrier;
    DECLARE_EVENT_TABLE()
};
