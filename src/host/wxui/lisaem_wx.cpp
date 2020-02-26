/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.02.20                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, MMXX Ray A. Arachelian                          *
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

// When the destroy method is just not enough, we ask our friendly neighborhood Dalek for help
#define EXTERMINATE(x) {if (x) {delete x; x=NULL;}}


// which hqx35 are we using? enable for 3x-3x - change build.sh to use hq3x-3x.cpp when enabled
#define USE_HQX35 1
#ifdef USE_HQX35
#define HQX35X (720*2)
#define HQX35Y (364*3)
#else
#define HQX35X (720)
#define HQX35Y (364)
#endif

#define reset_dirty    {  dirty_x_min=e_dirty_x_min=720;  dirty_x_max=dirty_y_max=e_dirty_y_max=e_dirty_x_max=0; dirty_y_min=364;    e_dirty_y_min=500;}
#define reset_dirty_3a {  dirty_x_min=e_dirty_x_min=608;  dirty_x_max=dirty_y_max=e_dirty_y_max=e_dirty_x_max=0; dirty_y_min=e_dirty_y_min=431;}
#define prep_dirty { e_dirty_x_min=dirty_x_min; e_dirty_x_max=dirty_x_max; e_dirty_y_min=screen_y_map[dirty_y_min]; e_dirty_y_max=screen_y_map[dirty_y_max];}


// It seems that with every release of wxWidgets there's yet another way to convert
// wxStrings into normal C strings, and the previous versions' break or cause all sorts
// of random issues, maybe on one OS maybe on all.  First we were supposed to use c_str(),
// then, f_str() was the way to do it, now it's char_str(), so that works. WTF guys, 
// ever heard of backwards compatibility?
// and if you read the docs, all 3 methods are still listed, none are labeled as 
// deprecated, but compiling old code with the old method causes strings to 
// misbehave. :facepalm:

#define CSTR(x) ((char *)(x.char_str()))
#define cSTR(x) ((char *)(x->char_str()))

// Some weird new changes to wxWidgets require newly created bitmaps to be filled,
// otherwise the wx/rawbmp.h code won't work right (since the alpha mask in the bitmap
// is set to the background, so we get no updates when we do the blit.  Yuck!)
//
// Worse yet, on some systems this needs to be WHITE creating annoying white flashes
// or white screens when LisaEm is off.  {Insert print "Motherfucker!" X 10000000}

#define FILLERBRUSH  *wxBLACK_BRUSH
#define FILLERPEN    *wxBLACK_PEN

// How many changes on the display before refreshing the entire display instead of the updated area?
// want to keep this value small, otherwise a clear desktop or such will be very slow.  The idea here
// is to avoid full repaints on small changes, such as the mouse moving.

#define DIRECT_BLITS_THRESHHOLD 64

// minimum skinned window size, this is smaller on purpose so that it will work with 12"
// notebook displays

#define IWINSIZEX   (1020) //*hidpi_scale)
#define IWINSIZEY   ( 720) //*hidpi_scale)

#define IWINSIZE IWINSIZEX,IWINSIZEY

// the size of the skin
#define ISKINSIZEX  ((int)(skin.width_size  )) // * hidpi_scale))
#define ISKINSIZEY  ((int)(skin.height_size )) // * hidpi_scale))
#define ISKINSIZE   ISKINSIZEX, ISKINSIZEY

#define FLOPPY_LEFT _H(skin.floppy2_tl_x)
#define FLOPPY_TOP  _H(skin.floppy2_tl_y)

#define POWER_LEFT  _H(skin.power_frame_left)
#define POWER_TOP   _H(skin.power_frame_top)

// padding for skinless mode
#define WINXDIFF    (int)(  30 ) //*hidpi_scale)
#define WINYDIFF    (int)(  65 ) //*hidpi_scale)

// Y scale factor for HQ3X modes - do not add parens around this
// since this is integer math want to multiply first and divide second.
#define HQ3XYSCALE   30/22
#define HQ3XDEYSCALE 22/30


// binary AND filter for how often to update
// the skinless background filter.  has to be 3,7,15,31,63
// larger the value the longer the delay between updates.
#define SKINLESS_UPDATE_FILTER 3


#include <wx/wx.h>
#include <wx/defs.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dcbuffer.h>
#include <wx/wxhtml.h>
#include <wx/fs_zip.h>
#include <wx/dir.h>
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
#include <wx/notebook.h>
#include <wx/aboutdlg.h>
#include <wx/stdpaths.h>
#include <wx/choicdlg.h>
#include <wx/cmdline.h>
#include <wx/utils.h> 
#include <wx/dnd.h>



// RAW_BITMAP_ACCESS should be used in most cases as it proves much higher performance
#ifdef NO_RAW_BITMAP_ACCESS
 #ifdef USE_RAW_BITMAP_ACCESS
   #undef    USE_RAW_BITMAP_ACCESS
 #endif
#endif

#include <built_by.h>
#include <extrablue.h>
#ifdef VERSION
    static char *my_version=VERSION;
#else
    static char *my_version="VERSIONLESS"
#endif


long emulation_tick=40;
long emulation_time=25;

#ifdef USE_RAW_BITMAP_ACCESS
#include <wx/rawbmp.h>
#endif

#include <wx/rawbmp.h>
#include <hqx.h>

#include <machine.h>
#include <keyscan.h>

#include <LisaConfig.h>
#include <LisaConfigFrame.h>
#include <LisaSkin.h>

// sounds, images, etc.
#include <lisaem_static_resources.h>

#ifdef __WXOSX__
#include <CoreFoundation/CoreFoundation.h>
#include <sys/param.h>
#endif

#define DEPTH 32

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#define __IN_LISAEM_WX__  1
extern "C"
{
 #include <vars.h>
 int32 reg68k_external_execute(int32 clocks);
 void unvars(void);
 void on_lisa_exit(void); 
}

#if wxUSE_UNICODE
 #define CSTRCONV (wchar_t*)
#else
 #define CSTRCONV (char*)
#endif


// fwd references.
extern "C" int ImageWriter_LisaEm_Init(int iwnum);
extern "C" void iw_formfeed(int iw);
extern "C" void ImageWriterLoop(int iw,uint8 c);
extern "C" void iw_shutdown(void);

extern "C" void emulate (void);
extern "C" void XLLisaRedrawPixels(void);          // proto to supress warning below
extern "C" void LisaRedrawPixels(void);

extern "C" void resume_run(void);
extern "C" void pause_run(void);

extern "C" void force_refresh(void);

extern "C" void iw_enddocuments(void);

void iw_check_finish_job(void);


void turn_skins_on(void);
void turn_skins_off(void);

void powerbutton(void);
void setvideomode(int mode);
void save_global_prefs(void);

int asciikeyboard=1;

#ifndef MIN
  #define MIN(x,y) ( (x)<(y) ? (x):(y) )
#endif

#ifndef MAX
  #define MAX(x,y) ( (x)>(y) ? (x):(y) )
#endif



//struct Skins skin;

wxFileConfig *skinconfig;
LisaSkin skin;

/*********************************************************************************************************************************/


 extern "C" int cpu68k_init(void);
 extern "C" void cpu68k_reset(void);
 extern "C" uint8 evenparity(uint8 data);

char *get_welcome_fortune(void);

int effective_lisa_vid_size_y=500; // these are hidpi_scale adjusted later in the code //364; - 500 here is the adjusted square-ified pixel size.
int effective_lisa_vid_size_x=720;
int o_effective_lisa_vid_size_y=500; // these ARE NOT hidpi_scale
int o_effective_lisa_vid_size_x=720;

wxPaintEvent nada;

void black(void);



wxConfigBase      *myConfig;             // default configuration (just a pointer to the active config)
wxString           myconfigfile;         // config filename
wxFileStream      *pConfigIS;            // config file


LisaConfigFrame  *my_LisaConfigFrame=NULL;

/*************************************************************************************************************************************/


// Declare the application class
class LisaEmApp : public wxApp
{
public:
    // Called on application startup
    virtual bool OnInit();
    bool OnCmdLineParsed(wxCmdLineParser& parser);
    void OnInitCmdLine(wxCmdLineParser& parser);
    void LisaSkinConfig(void);

    #ifndef __WXOSX__
    void OnQuit(wxCommandEvent& event);
    #endif
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int on_start_poweron=0,
           on_start_fullscreen=0,
           on_start_skin=0,
           on_start_center=0,
           on_start_harddisk=0,
           on_start_quit_on_poweroff=0,
           box_x=-1,box_y=-1, box_xh=-1, box_yh=-1; // used for screengrab
static double on_start_zoom=0.0;

wxString   on_start_lisaconfig="",
           on_start_floppy="";

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_SWITCH, "h", "help",       "show this help message",                            wxCMD_LINE_VAL_NONE,  wxCMD_LINE_OPTION_HELP},
    { wxCMD_LINE_SWITCH, "p", "power",      "power on as soon as LisaEm is launched",            wxCMD_LINE_VAL_NONE,  wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_SWITCH, "q", "quit",       "quit after Lisa shuts down",                        wxCMD_LINE_VAL_NONE,  wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION, "f", "floppy",     "boot from which floppy image ROMless only",         wxCMD_LINE_VAL_STRING,wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "d", "drive",      "boot from motherboard ProFile/Widget ROMless only", wxCMD_LINE_VAL_NONE,  wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "F", "fullscreen", "fullscreen mode (-F- to turn off)",                 wxCMD_LINE_VAL_NONE,  wxCMD_LINE_SWITCH_NEGATABLE|wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_OPTION, "z", "zoom",       "set zoom level (0.50, 0.75, 1.0, 1.25,... 3.0)",    wxCMD_LINE_VAL_DOUBLE,wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "s", "skin",       "turn on skin",                                      wxCMD_LINE_VAL_NONE,  wxCMD_LINE_SWITCH_NEGATABLE|wxCMD_LINE_PARAM_OPTIONAL},
    { wxCMD_LINE_OPTION, "c", "config",     "Open which lisaem config file",                     wxCMD_LINE_VAL_STRING,wxCMD_LINE_PARAM_OPTIONAL },

    { wxCMD_LINE_SWITCH, "k", "kiosk",      "kiosk mode (suitable for RPi Lisa case)",           wxCMD_LINE_VAL_NONE,  wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_SWITCH, "o", "originctr",  "skinless mode: center video(-o) vs topleft(-o-)",   wxCMD_LINE_VAL_NONE,  wxCMD_LINE_SWITCH_NEGATABLE|wxCMD_LINE_PARAM_OPTIONAL },

    { wxCMD_LINE_NONE,   ":",  "",          "",                                                  wxCMD_LINE_VAL_NONE,  wxCMD_LINE_PARAM_OPTIONAL }
};

#define FLOPPY_NEEDS_REDRAW    0x80
#define FLOPPY_ANIMATING       0x40
#define FLOPPY_INSERT_0        0x00
#define FLOPPY_INSERT_1        0x01
#define FLOPPY_INSERT_2        0x02
#define FLOPPY_PRESENT         0x03
#define FLOPPY_EMPTY           0x04
#define FLOPPY_ANIM_MASK       0x07

#define POWER_NEEDS_REDRAW     0x80
#define POWER_PUSHED           0x40
#define POWER_ON_MASK          0x01
#define POWER_ON               0x01
#define POWER_OFF              0x00

#define REPAINT_INVALID_WINDOW 0x80
#define REPAINT_POWER_TO_SKIN  0x04
#define REPAINT_FLOPPY_TO_SKIN 0x02
#define REPAINT_VIDEO_TO_SKIN  0x01
#define REPAINT_NOTHING        0x00




// video modes
enum
{
     vidmod_aa=0,    // RePaint_AntiAliased
     vidmod_2y=1,    // RePaint_DoubleY; 
     vidmod_raw=2,   // RePaint_SingleY
     vidmod_3y=3,    // RePaint_2X3Y
     vidmod_aag=4,   // RePaint_AAGray
     vidmod_hq35x=5, // RePaint_HQ35X
     vidmod_3a=0x3a  // RePaint_3A;  // consider adding 2x 3x or 4x HQX for this
};


// yet another lookup table, I know, I know, but I've seen random cases where (int)(int*float) were off
//static int *hidpi_to_normal=NULL;
//static int *normal_to_hidpi=NULL;
//static int max_h=0;
//static int max_n=0;

// fwd ref - this changes display settings.
void set_hidpi_scale(void);



// D'Oh looks like I wasted a lot of time working on scaling when wxWdigets already had it built in via DC.SetUSerScale()
#define _H(x) (x)
#define _N(x) (x)
//inline int _H(int n) { if (!normal_to_hidpi)  setup_hidpi(); if (n>=0 && n< max_n) return normal_to_hidpi[n]; else return 0xbadc0de; }
//inline int _N(int n) { if (!hidpi_to_normal)  setup_hidpi(); if (n>=0 && n< max_h) return hidpi_to_normal[n]; else return 0xbadc0d1; }


#ifdef __WXOSX__
#define DCTYPE wxPaintDC
#else
#define DCTYPE wxBufferedPaintDC
#endif

class LisaWin : public wxScrolledWindow
{
public:
      LisaWin(wxWindow *parent);
     ~LisaWin();

      int dirtyscreen;      // indicated dirty lisa vidram
      int doubley; 
      uint8 brightness;

      int clear_skinless;

      int refresh_bytemap;  // flag indicating contrast change
      int floppystate;      // animation state of floppy
      int powerstate;       // animation state of power button

      void Skins_Repaint_Floppy(wxRect &rect, DCTYPE &dc);
      void Skins_Repaint_PowerPlane(wxRect &rect, DCTYPE &dc);

      void RePaint_HQ35X(void);
      void RePaint_AAGray(void);
      void RePaint_AntiAliased(void);

      void RePaint_DoubleY(void);
      void RePaint_SingleY(void);
      //void RePaint_Scaled(void);
      void RePaint_3A(void);
      void RePaint_2X3Y(void);

      void (LisaWin::*RePainter)(void);   // pointer method to one of the above

      void SetVideoMode(int mode);

      void OnPaint_skinless(wxRect &rect, DCTYPE &dc);
      void OnPaint_skins(wxRect &rect, DCTYPE &dc);
      void OnPaint(wxPaintEvent &event);

      void OnErase(wxEraseEvent &event);

      long mousemoved;

      void OnMouseMove(wxMouseEvent &event);
      void OnKeyDown(wxKeyEvent& event);
      void OnKeyUp(wxKeyEvent& event);
      void OnChar(wxKeyEvent& event);

      void LogKeyEvent(const wxChar *name, wxKeyEvent& event,int keydir);
      void ContrastChange(void);

      int lastkeystroke;
      int last_mouse_pos_y;
      int last_mouse_pos_x;

      int repaintall;
      int ox,oy,ex,ey;
      int dwx,dwy;

      int rawcodes[128];
      int rawidx;
      uint8 bright[16];       // brightness levels for ContrastChange and repaint routines.

private:
      int lastkeyevent;
      wxScrolledWindow *myparent;
      wxCursor *m_dot_cursor;
      int lastcontrast;
      static inline wxChar GetChar(bool on, wxChar c) { return on ? c : _T('-'); }

      DECLARE_EVENT_TABLE()
};



BEGIN_EVENT_TABLE(LisaWin, wxScrolledWindow)

    EVT_KEY_DOWN(LisaWin::OnKeyDown)
    EVT_KEY_UP(LisaWin::OnKeyUp)
    EVT_CHAR(LisaWin::OnChar)

    EVT_ERASE_BACKGROUND(LisaWin::OnErase)
    EVT_PAINT(LisaWin::OnPaint)
    EVT_MOTION(LisaWin::OnMouseMove)
    EVT_LEFT_DOWN(LisaWin::OnMouseMove)
    EVT_LEFT_UP(LisaWin::OnMouseMove)
    EVT_RIGHT_DOWN(LisaWin::OnMouseMove)
    EVT_RIGHT_UP(LisaWin::OnMouseMove)

END_EVENT_TABLE()


// Event table for LisaEmFrame
enum
{
    ID_SCREENSHOT = 10001, // anti-aliased screenshot
    ID_SCREENSHOT_FULL,        // screenshot with screen
    ID_SCREENSHOT_RAW,        // raw screenshot - no aliasing

    ID_FUSH_PRNT,

#ifdef TRACE
    ID_DEBUG,
    ID_DEBUG2,
    ID_RAMDUMP,
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

    ID_ZOOM,

    ID_PROFILE_ALL_ON,
    ID_PROFILE_ALL_OFF,

    ID_PROFILE_S1L,
    ID_PROFILE_S1U,
    ID_PROFILE_S2L,
    ID_PROFILE_S2U,
    ID_PROFILE_S3L,
    ID_PROFILE_S3U,

    ID_PROFILE_NEW,

    ID_FLOPPY,
    ID_NewFLOPPY,

    ID_PAUSE,
#if DEBUG
    ID_SCREENREGION,
#endif
    ID_KEY_OPT_0,
    ID_KEY_OPT_4,
    ID_KEY_OPT_7,
    ID_KEYBOARD,
    ID_ASCIIKB,
    ID_RAWKB,
    ID_RAWKBBUF,

    ID_EMULATION_TIMER,

    ID_THROTTLE1,
    ID_THROTTLE5,
    ID_THROTTLE8,
    ID_THROTTLE10,
    ID_THROTTLE12,
    ID_THROTTLE16,
    ID_THROTTLE32,
    ID_THROTTLE48,
    ID_THROTTLE64,
    ID_THROTTLE100,
    ID_THROTTLE128,
    ID_THROTTLE256,
    ID_THROTTLEX,

    ID_ET100_75,
    ID_ET50_30,
    ID_ET40_25,
    ID_ET30_20,

    ID_LISAWEB,
    ID_LISAFAQ,
    ID_LISALIST2,

    ID_VID_HQ35X,
    ID_VID_AA,
    ID_VID_AAG,
    //ID_VID_SCALED,
    ID_VID_DY,
    ID_VID_SY,
    ID_VID_2X3Y,

    // reinstated as per request by Kallikak
    ID_REFRESH_SUB,
    ID_REFRESH_60Hz,
    ID_REFRESH_20Hz,
    ID_REFRESH_12Hz,
    ID_REFRESH_8Hz,
    ID_REFRESH_4Hz,

    ID_HIDE_HOST_MOUSE,

    ID_VID_SKINS,
    ID_VID_SKINLESSCENTER,
    ID_VID_SKINSELECT,

    ID_VID_SCALED_SUB,

    ID_VID_SCALE_25,
    ID_VID_SCALE_50,
    ID_VID_SCALE_75,

    ID_VID_SCALE_100,
    ID_VID_SCALE_125,
    ID_VID_SCALE_150,
    ID_VID_SCALE_175,
    ID_VID_SCALE_200,
    ID_VID_SCALE_225,
    ID_VID_SCALE_250,
    ID_VID_SCALE_275,
    ID_VID_SCALE_300,

    ID_VID_SCALE_ZOOMIN,
    ID_VID_SCALE_ZOOMOUT,

    ID_VID_FULLSCREEN
};

// Declare our main frame class



// lisaframe::running states
enum
{
    emulation_off=0,
    emulation_running=1,
    emulation_paused=10,
    emuation_paused_for_screen=12
};

class LisaEmFrame : public wxFrame
{
public:

    int running;          // is the Lisa running?  0=off, 1=running, 10=paused/locked.

    // Constructor
    LisaEmFrame(const wxString& title);

    void LoadImages(void);
    void UnloadImages(void);

    // Event handlers
//    #ifdef __WXOSX__
    void OnQuit(wxCommandEvent& event);
//    #endif

    void OnAbout(wxCommandEvent& event);

    void OnLisaWeb(wxCommandEvent& event);
    void OnLisaFaq(wxCommandEvent& event);
    void OnLisaList2(wxCommandEvent& event);

    void OnConfig(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);


//    void OnRun(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
#if DEBUG
    void OnScreenRegion(wxCommandEvent& event);
#endif
    void OnScreenshot(wxCommandEvent& event);
    void OnDebugger(wxCommandEvent& event);

    void OnPOWERKEY(wxCommandEvent& event);
    void OnAPPLEPOWERKEY(wxCommandEvent& event);
#ifdef DEBUG    
    void OnTraceLog(wxCommandEvent& event);
    void OnTraceLog2(wxCommandEvent& event);
    void DumpAllScreenshot(wxCommandEvent& event);
#endif
    void OnKEY_APL_DOT(wxCommandEvent& event);
    void OnKEY_APL_S(wxCommandEvent& event);
    void OnKEY_APL_ENTER(wxCommandEvent& event);
    void OnKEY_APL_RENTER(wxCommandEvent& event);
    void OnKEY_APL_1(wxCommandEvent& event);
    void OnKEY_APL_2(wxCommandEvent& event);
    void OnKEY_APL_3(wxCommandEvent& event);
    void OnKEY_NMI(wxCommandEvent& event);
    void OnKEY_RESET(wxCommandEvent& event);


    void UpdateProfileMenu(void);
    void OnProFilePowerX(int bit);

    void OnProFilePower(wxCommandEvent& event);

    void OnProFilePwrOnAll(wxCommandEvent& event);
    void OnProFilePwrOffAll(wxCommandEvent& event);

    void OnProFileS1LPwr(wxCommandEvent& event);
    void OnProFileS1UPwr(wxCommandEvent& event);
    void OnProFileS2LPwr(wxCommandEvent& event);
    void OnProFileS2UPwr(wxCommandEvent& event);
    void OnProFileS3LPwr(wxCommandEvent& event);
    void OnProFileS3UPwr(wxCommandEvent& event);

    void OnNewProFile(wxCommandEvent& event);

    void OnFLOPPY(wxCommandEvent& event);
    void OnNewFLOPPY(wxCommandEvent& event);

    void OnxFLOPPY(void);
    void OnxNewFLOPPY(void);

    void OnKEY_OPT_0(wxCommandEvent& event);
    void OnKEY_OPT_4(wxCommandEvent& event);
    void OnKEY_OPT_7(wxCommandEvent& event);
    void OnKEYBOARD(wxCommandEvent& event);

    void OnASCIIKB(wxCommandEvent& event);
    void OnRAWKB(wxCommandEvent& event);
    void OnRAWKBBUF(wxCommandEvent& event);
    void reset_throttle_clock(void);

    void OnThrottle5(wxCommandEvent& event);
    void OnThrottle8(wxCommandEvent& event);
    void OnThrottle10(wxCommandEvent& event);
    void OnThrottle12(wxCommandEvent& event);
    void OnThrottle16(wxCommandEvent& event);
    void OnThrottle32(wxCommandEvent& event);
    void OnThrottle48(wxCommandEvent& event);
    void OnThrottle64(wxCommandEvent& event);
    void OnThrottle100(wxCommandEvent& event);
    void OnThrottle128(wxCommandEvent& event);
    void OnThrottle256(wxCommandEvent& event);

    #ifdef DEBUG
    void OnThrottle1(wxCommandEvent& event);
    void OnThrottle512(wxCommandEvent& event);
    #endif

    void OnET100_75(wxCommandEvent& event);
    void OnET50_30(wxCommandEvent& event);
    void OnET40_25(wxCommandEvent& event);
    void OnET30_20(wxCommandEvent& event);

    void SetStatusBarText(wxString &msg);
    void Update_Status(long elapsed,long idleentry);
    void VidRefresh(long now);
    int EmulateLoop(long idleentry);

    //void OnIdleEvent(wxIdleEvent& event);
    void OnEmulationTimer(wxTimerEvent& event);

    void OnPasteToKeyboard(wxCommandEvent&event);


    void FloppyAnimation(void);
    // menu commands that switch video mode
    void OnVideoAntiAliased(wxCommandEvent& event);
    void OnVideoAAGray(wxCommandEvent& event);
    void OnVideoHQ35X(wxCommandEvent& event);
    //void OnVideoScaled(wxCommandEvent& event);
    void OnVideoDoubleY(wxCommandEvent& event);
    void OnVideoSingleY(wxCommandEvent& event);
    void OnVideo2X3Y(wxCommandEvent& event);

    void OnSkins(wxCommandEvent& event);
    void OnSkinSelect(wxCommandEvent& event);
    void OnSkinlessCenter(wxCommandEvent& event);

    void OnRefresh60(wxCommandEvent& event);
    void OnRefresh20(wxCommandEvent& event);
    void OnRefresh12(wxCommandEvent& event);
    void OnRefresh8(wxCommandEvent& event);
    void OnRefresh4(wxCommandEvent& event);

    void OnScale25(wxCommandEvent& event);
    void OnScale50(wxCommandEvent& event);
    void OnScale75(wxCommandEvent& event);

    void OnScale100(wxCommandEvent& event);
    void OnScale125(wxCommandEvent& event);
    void OnScale150(wxCommandEvent& event);
    void OnScale175(wxCommandEvent& event);
    void OnScale200(wxCommandEvent& event);
    void OnScale225(wxCommandEvent& event);
    void OnScale250(wxCommandEvent& event);
    void OnScale275(wxCommandEvent& event);
    void OnScale300(wxCommandEvent& event);

    #ifdef EVT_GESTURE_ZOOM
    void OnZoom(wxZoomGestureEvent& event);
    #endif
    void OnZoomIn( wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);

    void OnFlushPrint(wxCommandEvent &event);
    void OnHideHostMouse(wxCommandEvent& event);

    void OnFullScreen(wxCommandEvent& event);
    
    void insert_floppy_anim(wxString openfile);

    //class LisaWin *win;
    int screensizex,screensizey;
    int depth;

    wxStopWatch runtime;               // idle loop stopwatch
    wxStopWatch soundsw;
    int soundplaying;

    uint16 lastt2;
    long    last_runtime_sample;
    long    last_decisecond;
    XTIMER clx;
    XTIMER lastclk;
    XTIMER cpu68k_reference;
    XTIMER last_runtime_cpu68k_clx;
    int dwx,dwy;

    float          throttle;
    float          clockfactor;
    float          mhzactual;

    wxString floppy_to_insert;
    long lastcrtrefresh;
    long hostrefresh;
    long screen_paint_update;
    long onidle_calls=0;
    XTIMER cycles_wanted;

    wxString skindir;
    wxString skinname;
    wxString resdir;
    wxString osslash;

    wxTimer* m_emulation_timer;
    int barrier;
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(LisaEmFrame, wxFrame)


    EVT_MENU(wxID_ABOUT,         LisaEmFrame::OnAbout)

    EVT_MENU(ID_LISAWEB,         LisaEmFrame::OnLisaWeb)
    EVT_MENU(ID_LISAFAQ,         LisaEmFrame::OnLisaFaq)
    EVT_MENU(ID_LISALIST2,       LisaEmFrame::OnLisaList2)

    EVT_MENU(wxID_PREFERENCES,   LisaEmFrame::OnConfig)
    EVT_MENU(wxID_OPEN,          LisaEmFrame::OnOpen)
    EVT_MENU(wxID_SAVEAS,        LisaEmFrame::OnSaveAs)

    EVT_MENU(ID_SCREENSHOT,      LisaEmFrame::OnScreenshot)
    EVT_MENU(ID_SCREENSHOT_FULL, LisaEmFrame::OnScreenshot)
    EVT_MENU(ID_SCREENSHOT_RAW,  LisaEmFrame::OnScreenshot)
#if DEBUG
    EVT_MENU(ID_SCREENREGION,    LisaEmFrame::OnScreenRegion)
#endif
    EVT_MENU(ID_FUSH_PRNT,       LisaEmFrame::OnFlushPrint)

    EVT_MENU(ID_DEBUGGER,        LisaEmFrame::OnDebugger)
    EVT_MENU(ID_POWERKEY,        LisaEmFrame::OnPOWERKEY)
    EVT_MENU(ID_APPLEPOWERKEY,   LisaEmFrame::OnAPPLEPOWERKEY)

#ifdef EVT_GESTURE_ZOOM
    EVT_GESTURE_ZOOM(ID_ZOOM,    LisaEmFrame::OnZoom)
#endif

#ifndef __WXMSW__
#ifdef TRACE
    EVT_MENU(ID_DEBUG,           LisaEmFrame::OnTraceLog)
    EVT_MENU(ID_DEBUG2,          LisaEmFrame::OnTraceLog2)
    EVT_MENU(ID_RAMDUMP,         LisaEmFrame::DumpAllScreenshot)

#endif
#endif


    EVT_MENU(ID_KEY_APL_DOT,     LisaEmFrame::OnKEY_APL_DOT)
    EVT_MENU(ID_KEY_APL_S,       LisaEmFrame::OnKEY_APL_S)
    EVT_MENU(ID_KEY_APL_ENTER,   LisaEmFrame::OnKEY_APL_ENTER)
    EVT_MENU(ID_KEY_APL_RENTER,  LisaEmFrame::OnKEY_APL_RENTER)
    EVT_MENU(ID_KEY_APL_1,       LisaEmFrame::OnKEY_APL_1)
    EVT_MENU(ID_KEY_APL_2,       LisaEmFrame::OnKEY_APL_2)
    EVT_MENU(ID_KEY_APL_3,       LisaEmFrame::OnKEY_APL_3)
    EVT_MENU(ID_KEY_NMI,         LisaEmFrame::OnKEY_NMI)
    EVT_MENU(ID_KEY_RESET,       LisaEmFrame::OnKEY_RESET)

    EVT_MENU(ID_PROFILEPWR,      LisaEmFrame::OnProFilePower)

    EVT_MENU(ID_PROFILE_ALL_ON,  LisaEmFrame::OnProFilePwrOnAll)
    EVT_MENU(ID_PROFILE_ALL_OFF, LisaEmFrame::OnProFilePwrOffAll)

    EVT_MENU(ID_PROFILE_S1L,     LisaEmFrame::OnProFileS1LPwr)
    EVT_MENU(ID_PROFILE_S1U,     LisaEmFrame::OnProFileS1UPwr)
    EVT_MENU(ID_PROFILE_S2L,     LisaEmFrame::OnProFileS2LPwr)
    EVT_MENU(ID_PROFILE_S2U,     LisaEmFrame::OnProFileS2UPwr)
    EVT_MENU(ID_PROFILE_S3L,     LisaEmFrame::OnProFileS3LPwr)
    EVT_MENU(ID_PROFILE_S3U,     LisaEmFrame::OnProFileS3UPwr)

    EVT_MENU(ID_PROFILE_NEW,     LisaEmFrame::OnNewProFile)


    EVT_MENU(ID_FLOPPY,          LisaEmFrame::OnFLOPPY)
    EVT_MENU(ID_NewFLOPPY,       LisaEmFrame::OnNewFLOPPY)

    EVT_MENU(ID_KEY_OPT_0,       LisaEmFrame::OnKEY_OPT_0)
    EVT_MENU(ID_KEY_OPT_4,       LisaEmFrame::OnKEY_OPT_4)
    EVT_MENU(ID_KEY_OPT_7,       LisaEmFrame::OnKEY_OPT_7)

    EVT_MENU(ID_KEYBOARD,        LisaEmFrame::OnKEYBOARD)
    EVT_MENU(ID_ASCIIKB,         LisaEmFrame::OnASCIIKB)
    EVT_MENU(ID_RAWKB,           LisaEmFrame::OnRAWKB)
    EVT_MENU(ID_RAWKBBUF,        LisaEmFrame::OnRAWKBBUF)

    #ifdef DEBUG
    EVT_MENU(ID_THROTTLE1,       LisaEmFrame::OnThrottle1)
    EVT_MENU(ID_THROTTLEX,       LisaEmFrame::OnThrottle512)
    #endif
    EVT_MENU(ID_THROTTLE5,       LisaEmFrame::OnThrottle5)
    EVT_MENU(ID_THROTTLE8,       LisaEmFrame::OnThrottle8)
    EVT_MENU(ID_THROTTLE10,      LisaEmFrame::OnThrottle10)
    EVT_MENU(ID_THROTTLE12,      LisaEmFrame::OnThrottle12)
    EVT_MENU(ID_THROTTLE16,      LisaEmFrame::OnThrottle16)
    EVT_MENU(ID_THROTTLE32,      LisaEmFrame::OnThrottle32)
    EVT_MENU(ID_THROTTLE48,      LisaEmFrame::OnThrottle48)
    EVT_MENU(ID_THROTTLE64,      LisaEmFrame::OnThrottle64)
    EVT_MENU(ID_THROTTLE100,     LisaEmFrame::OnThrottle100)
    EVT_MENU(ID_THROTTLE128,     LisaEmFrame::OnThrottle128)
    EVT_MENU(ID_THROTTLE256,     LisaEmFrame::OnThrottle256)

    EVT_MENU(ID_ET100_75,        LisaEmFrame::OnET100_75)
    EVT_MENU(ID_ET50_30,         LisaEmFrame::OnET50_30)
    EVT_MENU(ID_ET40_25,         LisaEmFrame::OnET40_25)
    EVT_MENU(ID_ET30_20,         LisaEmFrame::OnET30_20)

    EVT_MENU(wxID_PASTE,         LisaEmFrame::OnPasteToKeyboard)

    EVT_MENU(ID_VID_AA,          LisaEmFrame::OnVideoAntiAliased)
    EVT_MENU(ID_VID_AAG,         LisaEmFrame::OnVideoAAGray)
    EVT_MENU(ID_VID_HQ35X,       LisaEmFrame::OnVideoHQ35X)

    EVT_MENU(ID_VID_DY,          LisaEmFrame::OnVideoDoubleY)
    EVT_MENU(ID_VID_SY,          LisaEmFrame::OnVideoSingleY)
    EVT_MENU(ID_VID_2X3Y,        LisaEmFrame::OnVideo2X3Y)

    EVT_MENU(ID_VID_SKINS,       LisaEmFrame::OnSkins)
    EVT_MENU(ID_VID_SKINLESSCENTER, LisaEmFrame::OnSkinlessCenter)

    EVT_MENU(ID_VID_SKINSELECT,  LisaEmFrame::OnSkinSelect)

    EVT_MENU(ID_VID_SCALE_25,    LisaEmFrame::OnScale25)
    EVT_MENU(ID_VID_SCALE_50,    LisaEmFrame::OnScale50)
    EVT_MENU(ID_VID_SCALE_75,    LisaEmFrame::OnScale75)

    EVT_MENU(ID_VID_SCALE_100,   LisaEmFrame::OnScale100)
    EVT_MENU(ID_VID_SCALE_125,   LisaEmFrame::OnScale125)
    EVT_MENU(ID_VID_SCALE_150,   LisaEmFrame::OnScale150)
    EVT_MENU(ID_VID_SCALE_175,   LisaEmFrame::OnScale175)
    EVT_MENU(ID_VID_SCALE_200,   LisaEmFrame::OnScale200)
    EVT_MENU(ID_VID_SCALE_225,   LisaEmFrame::OnScale225)
    EVT_MENU(ID_VID_SCALE_250,   LisaEmFrame::OnScale250)
    EVT_MENU(ID_VID_SCALE_275,   LisaEmFrame::OnScale275)
    EVT_MENU(ID_VID_SCALE_300,   LisaEmFrame::OnScale300)

    EVT_MENU(ID_VID_SCALE_ZOOMIN, LisaEmFrame::OnZoomIn)
    EVT_MENU(ID_VID_SCALE_ZOOMOUT,LisaEmFrame::OnZoomOut)

    EVT_MENU(ID_VID_FULLSCREEN,  LisaEmFrame::OnFullScreen)

  // reinstated as per request by Kallikak, added 4Hz to help with really slow machines
    EVT_MENU(ID_REFRESH_60Hz,    LisaEmFrame::OnRefresh60)
    EVT_MENU(ID_REFRESH_20Hz,    LisaEmFrame::OnRefresh20)
    EVT_MENU(ID_REFRESH_12Hz,    LisaEmFrame::OnRefresh12)
    EVT_MENU(ID_REFRESH_8Hz,     LisaEmFrame::OnRefresh8)
    EVT_MENU(ID_REFRESH_4Hz,     LisaEmFrame::OnRefresh4)

    EVT_MENU(ID_HIDE_HOST_MOUSE, LisaEmFrame::OnHideHostMouse)

    EVT_MENU(ID_PAUSE,           LisaEmFrame::OnPause)


    //EVT_IDLE(LisaEmFrame::OnIdleEvent)
    EVT_TIMER(ID_EMULATION_TIMER,LisaEmFrame::OnEmulationTimer)

    EVT_MENU(wxID_EXIT,          LisaEmFrame::OnQuit)

END_EVENT_TABLE()


// want to have these as globals so that they can be accessed by other fn's
// and passed around like a cheap 40oz bottle at a frat.

wxFileConfig     *pConfig =NULL;


wxMenu *fileMenu     = NULL;
wxMenu *editMenu     = NULL;
wxMenu *keyMenu      = NULL;
wxMenu *DisplayMenu  = NULL;
wxMenuItem *FullScreenCheckMenuItem = NULL;

wxMenu *DisplayScaleSub = NULL;
wxMenu *DisplayRefreshSub = NULL;

wxMenu *throttleMenu = NULL;
wxMenu *profileMenu  = NULL;

wxMenu *helpMenu     = NULL;


LisaConfig       *my_lisaconfig=NULL;
LisaWin          *my_lisawin=NULL;
LisaEmFrame      *my_lisaframe=NULL;
  

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: need to use char_str() from now on not c_str!
//char *getResourcesDir(void)
//{
//    static char ret[1024];
//
//    wxStandardPathsBase& stdp = wxStandardPaths::Get();
//    strncpy(ret,stdp.GetResourcesDir().c_str(),1024);
//
//    return ret;
//}
//
//
//
//char *getDocumentsDir(void)
//{
//    static char ret[1024];
//
//    wxStandardPathsBase& stdp = wxStandardPaths::Get();
//    strncpy(ret,stdp.GetDocumentsDir().c_str(),1024);
//
//    return ret;
//}
//
// does not exist in wx2.7.x //
/*char *getExecutablePath(void)
{
    static char ret[1024];

    wxStandardPathsBase& stdp = wxStandardPaths::Get();
    strncpy(ret,stdp.GetExecutablePath().c_str(),1024);

    return ret;
}
*/

#ifndef USE_RAW_BITMAP_ACCESS
wxImage          *display_img = NULL;
#endif

wxMenuBar        *menuBar = NULL;

wxBitmap         *my_lisabitmap=NULL;
wxMemoryDC       *my_memDC=NULL;

wxBitmap         *my_lisahq3xbitmap=NULL;
wxMemoryDC       *my_memhq3xDC=NULL;

//wxSound          *my_lisa_sound=NULL;       // sounds generated by Lisa Speaker

wxSound          *my_floppy_eject=NULL;
wxSound          *my_floppy_insert=NULL;
wxSound          *my_floppy_motor1=NULL;
wxSound          *my_floppy_motor2=NULL;
wxSound          *my_lisa_power_switch01=NULL;
wxSound          *my_lisa_power_switch02=NULL;
wxSound          *my_poweroffclk=NULL;


// byte bitmap used to accelerate writes to videoram
wxImage   *display_image =NULL;


wxBitmap  *my_skin   =NULL;    wxMemoryDC *my_skinDC    =NULL; // bug with wxX11 turns background black, this attempts
wxBitmap  *my_skin0  =NULL;    wxMemoryDC *my_skin0DC   =NULL; // to split the img in two hoping that it will help.
wxBitmap  *my_skin1  =NULL;    wxMemoryDC *my_skin1DC   =NULL;
wxBitmap  *my_skin2  =NULL;    wxMemoryDC *my_skin2DC   =NULL;
wxBitmap  *my_skin3  =NULL;    wxMemoryDC *my_skin3DC   =NULL;

wxBitmap  *my_floppy0=NULL;    wxMemoryDC *my_floppy0DC =NULL;
wxBitmap  *my_floppy1=NULL;    wxMemoryDC *my_floppy1DC =NULL;
wxBitmap  *my_floppy2=NULL;    wxMemoryDC *my_floppy2DC =NULL;
wxBitmap  *my_floppy3=NULL;    wxMemoryDC *my_floppy3DC =NULL;
wxBitmap  *my_floppy4=NULL;    wxMemoryDC *my_floppy4DC=NULL;

wxBitmap  *my_poweron  =NULL;  wxMemoryDC *my_poweronDC =NULL;
wxBitmap  *my_poweroff =NULL;  wxMemoryDC *my_poweroffDC=NULL;


// display lens
static wxCoord screen_y_map[502];
static wxCoord screen_to_mouse[364*3];    // 2X,3Y mode is the largest we can do
static wxCoord screen_to_mouse_hq3x[364*3];
static int     yoffset[502];              // lookup table for pointer into video display (to prevent multiplication)

//static int     screen_focus[364*3];       // how close to the ideal pixel we are - is this still used?


// sets scaling lenses for hidpi, used to translate mouse and display coordinates from physical display to Lisa
// gets called by set_hidpi_scale(), but only used for setting the lens
// :TODO: delete this 
void setup_hidpi(void)
{
  static float last_hidpi;
  hidpi_scale=(hidpi_scale==0.0) ? 1.0 : hidpi_scale; // needed to prevent divide by zero at startup

  if (last_hidpi != hidpi_scale) // || normal_to_hidpi==NULL || hidpi_to_normal==NULL )
  {
    // float scale=hidpi_scale;
    //int x,y,i; //,count;

    ALERT_LOG(0,"\n\n**** Changing scale from %f to %f ****\n\n",last_hidpi,hidpi_scale);
    my_lisawin->clear_skinless=1;
//    if (!!normal_to_hidpi) free(normal_to_hidpi);
//    if (!!hidpi_to_normal) free(hidpi_to_normal);

   // wxDisplaySize(&x,&y);
    //wxDisplay display(0);
    ////const wxSize p=display.GetPPI();
    //const wxRect r=display.GetClientArea();
    //x=r.GetWidth();
    //y=r.GetHeight();

//    count=MAX(x,y)+1;
//    max_h=count;
//    hidpi_to_normal=(int*)calloc(count,sizeof(int));
//    for (i=0; i<count; i++) {hidpi_to_normal[i]=(int) ( (double)(i)/(double)(scale) );}
//
//    if (skin.width_size && skin.height_size)
//      count=MAX(skin.width_size,skin.height_size)+1;
//    else
//      count=MAX(ISKINSIZEX,ISKINSIZEY);
//    normal_to_hidpi=(int*)calloc(count,sizeof(int));
//    max_n=count;
//    for (i=0; i<count; i++) {normal_to_hidpi[i]=(int) ( (double)(i)*(double)(scale) );}
  }
  last_hidpi=hidpi_scale;
}

char *paste_to_keyboard=NULL;
static int    idx_paste_to_kb=0;

void buildscreenymap(void)
{
#define MAXHY 588
#define MAXY  500
 float f,yf, ratio=364.0/MAXY, ratiohq3x=364.0/MAXHY;
 int i,yy;
 wxCoord y;
 
 for (y=0; y<364; y++) yoffset[y]=y*90;

 for (f=0.0,i=0; i<502; i++,f+=1.0)
     {
       yf=(f*ratio);
       yy=(int)(yf); y=(wxCoord)yy;

       y=(y<MAXY-1) ? y : MAXY-1;
       screen_to_mouse[i]=y;
       screen_to_mouse_hq3x[i]=y;
          screen_y_map[y]=i;
     }

 for (f=0.0,i=0; i<MAXHY; i++,f+=1.0)
     {
       yf=(f*ratiohq3x);
       yy=(int)(yf); y=(wxCoord)yy;

       y=(y<MAXHY-1) ? y : MAXHY-1;
       screen_to_mouse_hq3x[i]=y;
     }
}


void buildscreenymap_raw(void)
{
 wxCoord y;
 for (y=0; y<364; y++)
     {
       yoffset[y]=y*90;
       //screen_focus[y]=0;
       screen_to_mouse[y]=y;
          screen_y_map[y]=y;
     }

}

void buildscreenymap_3A(void)
{
 wxCoord y;

 for (y=0; y<431; y++)
     {
       yoffset[y]=y*76;
       //screen_focus[y]=0;
       screen_to_mouse[y]=y;
          screen_y_map[y]=y;
     }

}


void buildscreenymap_2Y(void)
{
 wxCoord y;
 for (y=0; y<364; y++)   yoffset[y]=y*90;
 for (y=0; y<364*2; y++)
     {
       //screen_focus[y]=0;
       screen_to_mouse[y]=y>>1;
          screen_y_map[y>>1]=y;
     }
}


void buildscreenymap_3Y(void)
{
 wxCoord y;
 for (y=0; y<364; y++)   yoffset[y]=y*90;

 for (y=0; y<364*3; y++)
     {
       //screen_focus[y]=0;
       screen_to_mouse[y]=y/3;
          screen_y_map[y/3]=y-1;
     }

}


#ifdef DEBUG
void log_screen_box(FILE *f,int x, int y, int x2, int y2)
{
  // :TODO:
  if (0) fprintf(f,"%d %d %d %d",x,y,x2,y2);
}
#endif

#ifdef XXXDEBUG
#include <m64.c>

void log_screen_box(FILE *f,int x, int y, int x2, int y2)
{
  uint8 *out;
  int p, xx,yy, xbyte, yrow;
  int ybytestart=y*lisa_vid_size_xbytes;
  int ybyteend=y2*lisa_vid_size_xbytes;
  int xb=x/8, xb2=x2/8;
  int z=8;
  int size=((x2-x+8)/8)*(y2-y) +8;
  int i=0;
  int count=0;
  uint8 *m=(uint8 *) calloc(1,  size  +128);
  char  *o=(char  *) calloc(1,4*size/3+128); // mime64 output is 25% larger
  uint8 *l=(uint8 *) calloc(1,4*size/3  +128);

  // save header - big endian, as ${diety} intended, of course.
  m[1]=((x &  7) <<    4) | (x2&7);  // save x coordinate lefover bits so we know how many bits to the left and right to ignore later
  m[2]=((xb2-xb) >>    8)         ;  // size in x bytes
  m[3]=((xb2-xb) &  0xff)         ; 
  m[4]=((y2-y1 ) >>    8)         ;  // size in y lines
  m[5]=((y2-y1 ) &  0xff)         ;
  m[6]=((size  ) >>    8)         ;  // byte size of block
  m[7]=((size  ) &  0xff)         ;
  m[0]=m[1]^m[2]^m[3]^m[4]^m[5]^m[6]^m[7]^0x55; // checksum
  z=8;

  #define ROL64(x) (x>>63) | (x<<1)

  uint16 lmask=((0x00ff >>(x  & 7 ) ) & 0xff;
  uint16 rmask=((0xff00 >>(x2 & 7 ) ) & 0xff;
  uint64 hash=0xdec0de99dec0de66;
  //DEBUG_LOG(0,"region: (%d,%d)-(%d,%d) size_in:%d size_m64:%d\n",x,y,x2,y2,size+128,4*size/3+128);
  fprintf(f,"\n\nregion=\"%d,%d!",x2-x,y2-y);
  for (p=ybytestart+xb; y<=y2; y++, p+=lisa_vid_size_xbytes)
  {  
    for (xx=xb; xx<xb2; p++,xx++)
    {
       uint8 mask=0xff;
       if (xx==xb)  mask=lmask;
       if (xx==xb2) mask=rmask;

       m[z++]=lisaram[videolatchaddress+p] & mask;
       hash=ROL64(hash)^(lisaram[videolatchaddress+p] & mask);
       count++;
    }
  }

// TODO: future code //
// might be able to use hash for the display instead.
// maybe use the first few values to find on the screen ram and then do a hash of the region when found.
// so maybe 1st 4 bytes - would have to shift through 8x, and once found, try hashing the region and see
// if we get the same value.

// Use cLevel=-12 for high compression
/*
int LZ4_compress_destSize (const char* src, char* dst, int* srcSizePtr, int targetDstSize);
  Reverse the logic : compresses as much data as possible from 'src' buffer
  into already allocated buffer 'dst', of size >= 'targetDestSize'.
  This function either compresses the entire 'src' content into 'dst' if it's large enough,
  or fill 'dst' buffer completely with as much data as possible from 'src'.
  note: acceleration parameter is fixed to "default".

 *srcSizePtr : will be modified to indicate how many bytes where read from 'src' to fill 'dst'.
               New value is necessarily <= input value.
 @return : Nb bytes written into 'dst' (necessarily <= targetDestSize)
           or 0 if compression fails.

wtf - this ignores cLevel:
static int LZ4IO_LZ4_compress(const char* src, char* dst, int srcSize, int dstSize, int cLevel)
{
    (void)cLevel;
    return LZ4_compress_fast(src, dst, srcSize, dstSize, 1);
}

but then:
LZ4_compress_HC

// Compress Block 
outSize = compressionFunction(in_buff, out_buff+4, (int)inSize, outBuffSize, compressionlevel);
compressedfilesize += outSize+4;
DISPLAYUPDATE(2, "\rRead : %i MB  ==> %.2f%%   ",
        (int)(filesize>>20), (double)compressedfilesize/files
*/
/*
  i=LZ4_compress_fast(m, l, count, dstSize, (4*size/3  +128));
  if (i<1) 
  {
     o[0]='4'; o[1]='>'; // indicate that it's compressed
     m64_encode((char *)m,i,&o[2]); 
  } 
  else
  {
     o[0]='>'; o[1]='<'; // indicate that it's not compressed
     m64_encode((char *)l,i,&o[2]); // lz4 failed - send uncompressed block as m64
  }
  fprintf(f,"%s===\"\n\n",o);
  free(m);
  free(o);
  free(l);
  ALERT_LOG(0,"done");
}

*/
#endif

void update_menu_checkmarks(void);



void LisaWin::SetVideoMode(int mode)
{
  //int i;
//  black();

  if ( has_lisa_xl_screenmod)   mode= 0x3a;               // sanity fixes
  if (!has_lisa_xl_screenmod && mode==0x3a) mode=0;
  if (mode!=0x3a) lisa_ui_video_mode=mode;

  update_menu_checkmarks();

  EXTERMINATE(my_lisabitmap);
  EXTERMINATE(display_image);

  clear_skinless=1;

  switch (mode)
  {
   case vidmod_hq35x: buildscreenymap();      skin.screen_origin_x=skin.default_screen_origin_x; skin.screen_origin_y=skin.default_screen_origin_y;  
                                              effective_lisa_vid_size_x=  _H(720);         effective_lisa_vid_size_y=  _H(500);
                                              o_effective_lisa_vid_size_x=   720;          o_effective_lisa_vid_size_y=   500;
                                              RePainter=&LisaWin::RePaint_HQ35X;                                                 
                                              break;

   case vidmod_aag:   buildscreenymap();      skin.screen_origin_x=skin.default_screen_origin_x; skin.screen_origin_y=skin.default_screen_origin_y;  
                                              effective_lisa_vid_size_x=  _H(720);         effective_lisa_vid_size_y=  _H(500);
                                              o_effective_lisa_vid_size_x=   720;          o_effective_lisa_vid_size_y=   500;
                                              RePainter=&LisaWin::RePaint_AAGray;                                                break;

   case vidmod_aa:    buildscreenymap();      skin.screen_origin_x=skin.default_screen_origin_x; skin.screen_origin_y=skin.default_screen_origin_y;  
                                              effective_lisa_vid_size_x=  _H(720);         effective_lisa_vid_size_y=  _H(500);
                                              o_effective_lisa_vid_size_x=   720;          o_effective_lisa_vid_size_y=   500;
                                              RePainter=&LisaWin::RePaint_AntiAliased;                                           break;


   case vidmod_2y:    buildscreenymap_2Y();   skin.screen_origin_x=            0;          skin.screen_origin_y=            0; 
                                              effective_lisa_vid_size_x=  _H(720);         effective_lisa_vid_size_y=  _H(364*2);
                                              o_effective_lisa_vid_size_x=_H(720);         o_effective_lisa_vid_size_y=_H(364*2);
                                              RePainter=&LisaWin::RePaint_DoubleY;                                               break;


   case vidmod_raw:   buildscreenymap_raw();  skin.screen_origin_x=skin.default_screen_origin_x; skin.screen_origin_y=skin.default_screen_origin_y; 
                                              effective_lisa_vid_size_x=  _H(720);         effective_lisa_vid_size_y=  _H(364);
                                              o_effective_lisa_vid_size_x=   720;          o_effective_lisa_vid_size_y=   364;
                                              RePainter=&LisaWin::RePaint_SingleY;                                               break;


   case vidmod_3y:    buildscreenymap_3Y();   skin.screen_origin_x=  0;                    skin.screen_origin_y=  0; 
                                              effective_lisa_vid_size_x=  _H(720);         effective_lisa_vid_size_y=  _H(364*3);
                                              o_effective_lisa_vid_size_x=   720*2;        o_effective_lisa_vid_size_y=   364*3;
                                              RePainter=&LisaWin::RePaint_2X3Y;                                                  break;

   case vidmod_3a:    buildscreenymap_3A();   skin.screen_origin_x=(skin.default_screen_origin_x +56); skin.screen_origin_y=(skin.default_screen_origin_y +34);
                                              effective_lisa_vid_size_x=  _H(608);         effective_lisa_vid_size_y=  _H(431);
                                              o_effective_lisa_vid_size_x=   608;          o_effective_lisa_vid_size_y=   431;

                                              lisa_vid_size_x=               608;          lisa_vid_size_y=               431;
                                              lisa_vid_size_xbytes=76;
                                              has_lisa_xl_screenmod=1;
                                              RePainter=&LisaWin::RePaint_3A;                                                    break;


  }

//  ALERT_LOG(0,"o_effective_lisa_vid_x,y:%d,%d",o_effective_lisa_vid_size_x,o_effective_lisa_vid_size_y);

  if (my_lisabitmap) delete my_lisabitmap;
  if (my_memDC)      delete my_memDC;

  my_memDC     =new class wxMemoryDC;
  my_lisabitmap=new class wxBitmap(o_effective_lisa_vid_size_x, o_effective_lisa_vid_size_y,DEPTH);
  my_memDC->SetUserScale(1.0,1.0);
  my_memDC->SelectObjectAsSource(*my_lisabitmap);

  my_memDC->SetBrush(FILLERBRUSH);      my_memDC->SetPen(FILLERPEN);
  my_memDC->DrawRectangle(0 ,   0,   o_effective_lisa_vid_size_x, o_effective_lisa_vid_size_y);

  #ifdef DEBUG
    if (!my_memDC->IsOk())      ALERT_LOG(0,"my_memDC is not ok.");
    if (!my_lisabitmap->IsOk()) ALERT_LOG(0,"my_lisabitmap is not ok.");
  #endif
  videoramdirty=32768;
   //DEBUG_LOG(0,"Done setting video mode to :%02x",mode);

  if  (skins_on)
      {
        SetMinSize(  wxSize(_H(720),_H(500) )  );
        if  (skin.width_size>0 && skin.height_size>0)
            {
              SetClientSize(              wxSize(  _H(skin.width_size), _H(skin.height_size)  ) );
              SetMaxSize(                 wxSize(  _H(skin.width_size), _H(skin.height_size)  ) );

              my_lisaframe->SetClientSize(wxSize(  _H(skin.width_size), _H(skin.height_size)  ) );
              //my_lisaframe->SetMaxSize(   wxSize(  _H(skin.width_size), _H(skin.height_size)  ) );

              DEBUG_LOG(0,"** Reset max window size to %d,%d at hidpi:%f ***",
                              _H(skin.width_size), _H(skin.height_size), hidpi_scale            );
            }
        else
            {  
              //20200128//SetSize(wxSize(ISKINSIZE)); 
              //20200128//SetMaxSize(wxSize(ISKINSIZE));
              //20200128//my_lisaframe->SetSize(wxSize(ISKINSIZE)); 
              //20200128//my_lisaframe->SetMaxSize(wxSize(ISKINSIZE));
              //20200128//DEBUG_LOG(0,"Reset max window size to ISKINSIZE");
            }
          // this was enabled because on older machines, the display is too small to show the whole screen and we want to
          // allow the user to scroll down to the power button and be able to press it.
          // As noted here: https://github.com/kuroneko/lisaem/commit/6503ed11341c8d94ea4d0d62e51bee13ee209088 GTK doesn't like that.
          // Would have been nice if Chris Collins contacted me about these, as I might have switched to CMake instead of bashbuild
          // and these fixes would have made their way into the code sooner. Oh well.
          #ifndef __WXGTK__
          SetScrollbars(ISKINSIZEX/_H(100), ISKINSIZEY/_H(100),  100,100,  0,0,  true);
          EnableScrolling(true,true);
          #endif
      }
  else
      {
        int x,y;
        GetSize(&x,&y);
        skin.screen_origin_x=(x  - effective_lisa_vid_size_x)>>1;                 // center display
        skin.screen_origin_y=(y  - effective_lisa_vid_size_y)>>1;                 // on skinless
        skin.screen_origin_x= (skin.screen_origin_x<0 ? 0:skin.screen_origin_x);
        skin.screen_origin_y= (skin.screen_origin_y<0 ? 0:skin.screen_origin_y);
        ox=skin.screen_origin_x; oy=skin.screen_origin_y;
      }
  Refresh(false,NULL);
  black();

//  ALERT_LOG(0,"o_effective_lisa_vid_x,y:%d,%d",o_effective_lisa_vid_size_x,o_effective_lisa_vid_size_y);

}

void LisaEmFrame::FloppyAnimation(void)
{

    if (my_lisawin->floppystate & FLOPPY_ANIMATING)                           // go to next frame
    {

        if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)!= FLOPPY_PRESENT) 
        {
            my_lisawin->floppystate++;
            my_lisawin->floppystate |= FLOPPY_ANIMATING|FLOPPY_NEEDS_REDRAW;
        } 
        
        if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)== FLOPPY_PRESENT) 
        {
            my_lisawin->floppystate= FLOPPY_NEEDS_REDRAW | FLOPPY_PRESENT; // refresh, stop counting.
            if (!!my_floppy_insert && sound_effects_on) my_floppy_insert->Play(romless ? wxSOUND_SYNC:wxSOUND_ASYNC);
        }
    }
    else
    {
        if (!!my_floppy_eject && sound_effects_on && (my_lisawin->floppystate & FLOPPY_ANIM_MASK)== FLOPPY_INSERT_2)
            my_floppy_eject->Play(wxSOUND_ASYNC);

        if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)==FLOPPY_INSERT_0) 
        {
            my_lisawin->floppystate=FLOPPY_NEEDS_REDRAW | FLOPPY_EMPTY;
        } 
        else 
        {
            my_lisawin->floppystate--;  // go to previous frame
            my_lisawin->floppystate |= FLOPPY_NEEDS_REDRAW;
        }

    }
    Refresh();
    wxMilliSleep(200);
}



void LisaEmFrame::VidRefresh(long now)
{


    if (videoramdirty)
    {
        wxRect* rect=NULL;

        my_lisawin->dirtyscreen=2;
        // suspect that RefreshRect(rect,false) erases the background anyway on wxMac 2.8.x
        rect=new wxRect(skin.screen_origin_x,           skin.screen_origin_y,
                        o_effective_lisa_vid_size_x, o_effective_lisa_vid_size_y);
        Refresh(false, rect);
        delete rect;
        lastcrtrefresh=now;                                      // and how long ago the last refresh happened
                                                                 // cheating a bit here to smooth out mouse movement.
    }

    screen_paint_update++;                                       // used to figure out effective host refresh rate
    lastrefresh=cpu68k_clocks;
    seek_mouse_event();
}



int LisaEmFrame::EmulateLoop(long idleentry)
{
    long now=runtime.Time();

    if (my_lisaframe->soundsw.Time()>1000 && sound_effects_on) // OH WOW! When sound is looping it fails to stop here and we're stuck in a loop! 
       {my_lisaframe->soundplaying=0; wxSound::Stop();}       // silence floppy motor if it hasn't been accessed in 500ms

    while (now-idleentry<emulation_time && running)          // don't stay in OnIdleEvent for too long, else UI gets unresponsive
    {
        long cpuexecms=(long)((float)(cpu68k_clocks-cpu68k_reference)*clockfactor); //68K CPU Execution in MS

        if ( cpuexecms<=now  )                               // balance 68K CPU execution vs host time to honor throttle
        {
            if (!cycles_wanted) cycles_wanted=(XTIMER)(float)(emulation_time/clockfactor);
            clx=clx+cycles_wanted;                           // add in any leftover cycles we didn't execute.
                                                             // but prevent it falling behind or jumping ahead
                                                             // too far.
            clx=MIN(clx,2*cycles_wanted  );
            clx=MAX(clx,  cycles_wanted/2);

            clx=reg68k_external_execute(clx);                // execute some 68K code
            // clx=reg68k_external_execute(clx);

            now=runtime.Time();                              // find out how long we took

            if (pc24 & 1)                                    // lisa rebooted or just odd addr error?
            {                                                // moved here to avoid stack leak
                ALERT_LOG(0,"ODD PC!")
                if (lisa_ram_safe_getlong(context,12) & 1)   // oddaddr vector is odd as well?
                {
                    save_pram();                             // lisa has rebooted.
                    profile_unmount();
                    return 1;
                }
            }

            get_next_timer_event();                          // handle any pending IRQ's/timer prep work
            
//20071115                                                   // if we need to, refresh the display
//#ifdef __WXOSX__
//        VidRefresh(now);                                   // OS X, esp slower PPC's suffer if we use the if statement
//#else
        if (now-lastcrtrefresh>hostrefresh) VidRefresh(now); // but if we don't, Linux under X11 gets too slow.
        else seek_mouse_event();
        
//#endif
        }                                                    // loop if we didn't go over our time quota
        else
            break;                                           // else force exit, time quota is up
    } // exec loop  ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    return 0;                                                // Lisa did not reboot, return 0
}


#ifdef DEBUG
extern "C" char *debug_screenshot(void);
extern "C" void dumpallmmu(void);
static int last_lisa_clock_secs=0;

extern "C"  void dumpallscreenshot(void)
{
            char *name=debug_screenshot();
            ALERT_LOG(0,"screenshot taken");
            dumpallmmu();
            dumpram(name);
            debug_screenshot();
}

#endif

void LisaEmFrame::Update_Status(long elapsed,long idleentry)
{
    static int counter;
    wxString text;
    float hosttime=(float)(elapsed - last_runtime_sample);
    mhzactual= 0.001 *
        ( (float)(cpu68k_clocks-last_runtime_cpu68k_clx))  / hosttime;

    if (running) check_running_lisa_os(); // moved here from LisaEmFrame::VidRefresh so we don't do this as often.

  //text.Printf("%1.2fMHz  %x%x:%x%x:%x%x.%x @%d/%08x vid:%1.2fHz%c  loop:%1.2fHz, mouse:%1.2fHz slice:%ldms, 68K:%lldcycles (%3.2f ms), %lld left, %lld wanted (%lld%%)",
    text.Printf(_T("%1.2fMHz  %x%x:%x%x:%x%x.%x @%d/%08x (cpu_clk:%lld"),
                mhzactual,
                lisa_clock.hours_h,lisa_clock.hours_l,
                lisa_clock.mins_h,lisa_clock.mins_l,
                lisa_clock.secs_h,lisa_clock.secs_l,
                lisa_clock.tenths,
                context,pc24,cpu68k_clocks); //,

    SetStatusBarText(text);

    // only issue is that this is called only on startup
    if ( !(counter) ) update_menu_checkmarks(); // meh, there's some bugs in wxWidgets 3.1.1 and earlier on GTK where Radio buttons don't get updated, i.e. scale menu
    counter++; counter&=7;
    // these are independant of the execution loops on purpose, so that way we get a 2nd opinion as it were.
    last_runtime_sample=elapsed;
    last_runtime_cpu68k_clx=cpu68k_clocks;
    my_lisawin->mousemoved=0;
    screen_paint_update=0;                              // reset statistics counters
    onidle_calls=0;
    idleentry--;                                       // eat warning when debug version isn't used.


    // if we're in the ROM, and within the first 3 seconds of emulation, and have had command line options, 
    // send Apple-2 or Apple 3 to start from floppy/Widget/Profile
    if (running_lisa_os==LISA_ROM_RUNNING && cpu68k_clocks>15000000 && cpu68k_clocks<30000000)
    {
        ALERT_LOG(0,"Checking for on_start options to pass to ROM. %lld",(long long) cpu68k_clocks);

        if (on_start_harddisk)  { apple_3(); on_start_harddisk=0; on_start_floppy=""; ALERT_LOG(0,"on_start_harddisk");}

        if (on_start_floppy != "" )
        {
            char s[1024];
            strncpy(s,on_start_floppy.mb_str(wxConvUTF8),1023 );
            floppy_insert(s);
            wxMilliSleep(100);  // wait for insert sound to complete to prevent crash.
            my_lisaframe->floppy_to_insert=_T("");
            on_start_floppy="";
            on_start_harddisk=0;
            apple_2();
            ALERT_LOG(0,"on_start_floppy inserted %s",s);
        }

    }


  #ifdef DEBUG  // do ram, mmu, and screenshot dumps every 5s
    if  (buglog && abs(lisa_clock.secs_l-last_lisa_clock_secs)>15 )
        {
           dumpallscreenshot();            
           last_lisa_clock_secs=lisa_clock.secs_l;
        }
  #endif

}



#if wxUSE_DRAG_AND_DROP

class DnDText : public wxTextDropTarget
{
public:
    //DnDText(wxListBox *pOwner) { m_pOwner = pOwner; }
    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& text); //wxOVERRIDE;
};

bool DnDText::OnDropText(wxCoord, wxCoord, const wxString& text)
{
    //m_pOwner->Append(text);
    ALERT_LOG(0,"Dropped text:%s",CSTR(text));
    return true;
    UNUSED(text);
}

class DnDFile : public wxFileDropTarget
{
public:
    virtual bool OnDropFiles(wxCoord x, wxCoord y,
                             const wxArrayString& filenames);// wxOVERRIDE;
};

// in LisaEmFrame constructor add     SetDropTarget(new DnDShapeDropTarget(this));
// not sure I can do both text and file
bool DnDFile::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
  char *filename;
  FILE *file;
  int c;
  int flag=0;
  size_t filesize;
  size_t i=0;
  wxString wxfilename="";
  UNUSED(x); UNUSED(y);
  // we only accept a single file, then verifiy that it's either an ASCIItext file or disk image
  // if it's a disk image, must be a floppy, not a hard disk.
  ALERT_LOG(0,"In OnDrop at %d,%d count %d",(int)(x),(int)(y),(int)(filenames.GetCount()));
  if (filenames.IsEmpty()) return false;
  if (filenames.GetCount()>1) {messagebox("Only one file may be dragged at a time", "Sorry"); return false;}
  wxfilename << filenames[0];
  filename=(char *)calloc(1,wxfilename.Len()+1);
  strncpy(filename,CSTR(wxfilename),wxfilename.Len());

  ALERT_LOG(0,"got filename ::%s::",filename);
  file=fopen(filename,"rb"); 
  if (!file) {
    wxString msg="Could not open:";
    msg << wxfilename;
    messagebox(CSTR(msg), "Oops!"); 
    free(filename); 
    return false;
  }
  
  fseek(file,0,SEEK_END);
  filesize=ftell(file);
  fseek(file,0,SEEK_SET);
  if (filesize>890*1024) {
      fclose(file); 
      free(filename); 
      wxString msg = wxfilename; msg << "  is too large to be a floppy, and too big for a text file to paste.";
      messagebox(CSTR(msg), "Too large!"); 
      return false;
  }

  if (filesize>400*1024) {
      fclose(file); 
      my_lisaframe->insert_floppy_anim(wxfilename);
      free(filename); 
      return true;
  }

  if (filesize>32767) {
      fclose(file);
      free(filename);
      wxString msg = wxfilename; msg << " is too small to be a floppy image and too large to paste";
      messagebox(CSTR(msg),"Can't paste this file");
      return false;
  }

  if (paste_to_keyboard) {
      wxString msg = wxfilename; msg << " cannot be pasted as another paste operation is in progress";
      messagebox(CSTR(msg),"Can't paste this file");
      return false;
  }

  // if we made it here, let's test for ascii

  paste_to_keyboard=(char *)calloc(1,filesize+2);
  i=fread(paste_to_keyboard,filesize,1,file);
  if (!i) {fclose(file); messagebox("Couldn't read the file to a buffer","Read error"); return false;}
  fclose(file);

  // check file for ASCII only. Allow only CR, LF, TAB
  for (i=0, flag=0; i<filesize && !flag; i++)
  {
    c=paste_to_keyboard[i];
    if (c>127) flag=1;  
    if (c<31)  flag=1;
    if (c==9 || c==10 ||c==13) flag=0;
  }

  if (flag) { messagebox("This file contains non-ASCII characters, cannot paste","Not a plain ASCII file"); 
              memset(paste_to_keyboard,0,32767);
              free(paste_to_keyboard); paste_to_keyboard=NULL;
              return false;
            }
  // enable paste text to keyboard
  idx_paste_to_kb = 0;
  free(filename);
  return true;
}
#endif // wxUSE_DRAG_AND_DROP



void LisaEmFrame::OnEmulationTimer(wxTimerEvent& WXUNUSED(event))
{

  long now=runtime.Time();
  long idleentry=now;

  // we run the timer as fast as possible.  there's a chance that it will call this method
  // while another instance is in progress.  the barrier prevents this.  Since each call will take
  // a slightly different amount of time, I can't predict a good value for this, but want to call it
  // as often as possible for the higher MHz throttles, so this is needed.

  if (barrier)   {ALERT_LOG(0,"Re-entry detected!"); return;}
  barrier=1;

  onidle_calls++;

  if  (on_start_poweron && onidle_calls >5)
      {
          wxCommandEvent foo;
          on_start_poweron=0;
          OnPOWERKEY(foo);
          ALERT_LOG(0,"on_start_poweron");
      }


  if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK) != FLOPPY_PRESENT &&
      (my_lisawin->floppystate & FLOPPY_ANIM_MASK) != FLOPPY_EMPTY )      FloppyAnimation();

  if  (running==emulation_running)
      {
        long int elapsed=0;

  //   if (floppy_6504_wait>0 && floppy_6504_wait<128) floppy_6504_wait--;                 
  
        if  (!last_runtime_sample )                              // initialize slice timer
            {
              last_runtime_sample=now;
              last_decisecond=now;
              lastcrtrefresh=now;
              last_runtime_cpu68k_clx=cpu68k_clocks;
            }


        if  (cpu68k_clocks<10 && floppy_to_insert.Len())         // deferred floppy insert.
            {
              const wxCharBuffer s = CSTR(floppy_to_insert);
              int i=floppy_insert((char *)(const char *)s);
              floppy_to_insert=_T("");
              if (i) eject_floppy_animation();
            }


        clktest = cpu68k_clocks;
        clx     = cpu68k_clocks;

        long ticks=( now - last_decisecond );                    // update COPS 1/10th second clock.
        while (ticks>100) {ticks-=100; decisecond_clk_tick(); last_decisecond=now;}


        if  (EmulateLoop(now) )                                   // 68K execution
            {            
                ALERT_LOG(0,"REBOOTED?");                            // Did we reboot?
                lisa_rebooted();
                barrier=0;
                return;
            }

        elapsed=runtime.Time();                                  // get time after exist of execution loop


        if  ( (elapsed - last_runtime_sample) > 1000  && running) // update status bar every 1000ms, and check print jobs too
            {
                static int ctr;
                Update_Status(elapsed,idleentry);
                if ((ctr++)>9) {iw_check_finish_job(); ctr=0;}
            }

        if  (paste_to_keyboard && idx_paste_to_kb>-1 && (copsqueuelen>=0 && copsqueuelen<MAXCOPSQUEUE-8) )
            {
              if  (paste_to_keyboard[idx_paste_to_kb]) 
                  {
                    ALERT_LOG(0,"Pasting to keyboard: %02x",paste_to_keyboard[idx_paste_to_kb]);
                    keystroke_cops( paste_to_keyboard[idx_paste_to_kb++] );
                  }
              else {idx_paste_to_kb=-1; free(paste_to_keyboard); paste_to_keyboard=NULL; ALERT_LOG(0,"//////// End of paste to keyboard ////////");}
            }

      }
  else // else for   if  (running==emulation_running) we are not running, or we are paused, so yield and sleep a bit
      {
        last_runtime_sample=0;
        lastcrtrefresh=0;
      }

  #ifndef __WXOSX__
  {   // force an extra full display refresh every 15 cycles on OS X
      static unsigned int x;
      x++;
      if (x>15) {
                  x=0;
                  videoramdirty=32768; 
                  Refresh(false,NULL);
                }
  }
  #endif


barrier=0;
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------


void LisaEmFrame::OnPasteToKeyboard(wxCommandEvent& WXUNUSED(event))
{

    wxTextDataObject data;

    if (paste_to_keyboard) {
      wxString msg="Cannot paste as another paste operation is in progress";
      messagebox(CSTR(msg),"Already pasting");
      return;
    }

    if (wxTheClipboard->Open()) 
    {
        if (wxTheClipboard->IsSupported(wxDF_TEXT))  wxTheClipboard->GetData(data);
        wxTheClipboard->Close();

        wxTheClipboard->UsePrimarySelection();

        if (wxTheClipboard->IsSupported(wxDF_TEXT)) 
        {
            wxString wspaste_to_keyboard;
            int len;

            wspaste_to_keyboard = (wxString)(data.GetText());

            len=MAX(wspaste_to_keyboard.Len(),32768)+2;
            paste_to_keyboard=(char *)calloc(1,len);
            strncpy(paste_to_keyboard,CSTR(wspaste_to_keyboard),len);
            paste_to_keyboard[len-1]=0;
            idx_paste_to_kb = 0;
        }
    }
}



// the: wxALWAYS_SHOW_SB causes "../src/gtk/scrolwin.cpp(227): assert "scrolled" failed in DoShowScrollbars(): window must be created"
LisaWin::LisaWin(wxWindow *parent)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(ISKINSIZE),
          wxBORDER_THEME|wxVSCROLL|wxHSCROLL|wxWS_EX_PROCESS_IDLE |wxFULL_REPAINT_ON_RESIZE
//#ifdef wxALWAYS_SHOW_SB
//|wxALWAYS_SHOW_SB
//#endif
#ifdef wxMAXIMIZE
|wxMAXIMIZE
#endif
)
{
    clear_skinless=1;
    mousemoved=0;
    lastcontrast=contrast;
    floppystate = FLOPPY_NEEDS_REDRAW | FLOPPY_EMPTY; // set floppy state and force refresh
    powerstate  = POWER_NEEDS_REDRAW;                  // set power state and force refresh
    //repaintall = REPAINT_NOTHING;

    repaintall  = REPAINT_INVALID_WINDOW;

    rawidx=0;

    SetExtraStyle(wxWS_EX_PROCESS_IDLE );

    int screensizex,screensizey;   // my_lisaframe is still null.
    //wxDisplaySize(&screensizex,&screensizey);
    wxDisplay display(wxDisplay::GetFromWindow(parent));
    
    #if (((wxMAJOR_VERSION*100) + (wxMINOR_VERSION*10) + wxRELEASE_NUMBER) >311)
    const wxSize p=display.GetPPI();
    ALERT_LOG(0,"PPI:%d,%d",p.GetWidth(),p.GetHeight());
    #endif
    // ^- not sure this is useful, returns 256x256 on 4k display, but resolution is reported at 1080p even though it's really 4k

    const wxRect r=display.GetClientArea();
    screensizex=r.GetWidth();
    screensizey=r.GetHeight();

    ALERT_LOG(0,"Primary display: %d,%d",screensizex,screensizey);
    //ALERT_LOG(0,"PPI: %d,%d",p.GetHeight(),p.GetWidth());


    DEBUG_LOG(0,"LisaWin constructor");
    // Grrr! OS X sometimes returns garbage here

    // fucked up thing here, if you have a 4K display and are using wx3.1.1 you're told
    // the display is 4k, if you're using 3.1.2 it lies to you and says you have 1080p! Argh!
    if  (screensizex< 0 || screensizex>16384 || screensizey<0 || screensizey>16384)
        {
            ALERT_LOG(0,"Got Garbage lisawin Screen Size: %d,%d",screensizex,screensizey);
            screensizex=1920; screensizey=1080;
        }
    else
        {
            if (my_lisaframe) {my_lisaframe->screensizex=screensizex;my_lisaframe->screensizey=screensizey;}
        }

    ALERT_LOG(0,"skins is:%d", skins_on);
    set_hidpi_scale();

    if  (skins_on)
        {
            DEBUG_LOG(0,"Setting widnow sizes");
            set_hidpi_scale();
            SetMinSize(wxSize(_H(720),_H(500)) ); //IWINSIZE previously
            SetSize(wxSize(IWINSIZE));
            //SetMaxSize(wxSize(ISKINSIZE)); // not this guy
            DEBUG_LOG(0,"Setting scrollbars");
            SetScrollbars(ISKINSIZEX/(_H(100)), ISKINSIZEY/_H(100),  _H(100),_H(100),  0,0,  true);

            DEBUG_LOG(0,"Enabling scrolling");
            EnableScrolling(true,true);

            skin.screen_origin_x=skin.default_screen_origin_x;
            skin.screen_origin_y=skin.default_screen_origin_y;

            DEBUG_LOG(0,"Set window size to %d,%d",ISKINSIZEX,ISKINSIZEY);
        }
    else  //------------------------ skinless ----------------------------------------------------
        {
            int x,y;
            setvideomode(lisa_ui_video_mode);
            y=myConfig->Read(_T("/lisawin/sizey"),(long)effective_lisa_vid_size_y);
            x=myConfig->Read(_T("/lisawin/sizex"),(long)effective_lisa_vid_size_x);
            if (x<=0 || x>4096 || y<=0 || y>2048) {x=effective_lisa_vid_size_x;y=effective_lisa_vid_size_y;}
            if  (x>screensizex || y>screensizey) {        // if the saved/defaults are too large correct them
                    x=MIN(o_effective_lisa_vid_size_x,screensizex-100);
                    y=MIN(o_effective_lisa_vid_size_y,screensizey-150);
                    DEBUG_LOG(0,"Resetting screen size to:%ld,%ld since screensize is %ld, %ld",(long)x,(long)y,
                                  (long)screensizex, (long)screensizey);
            }
            SetMinSize( wxSize( _H(720),_H(500) ) );  DEBUG_LOG(0,"Set window size to %d,%d",x,y);
            //SetClientSize(wxSize(x,y));                                                         // LisaWin //
            //SetMaxSize(wxSize(ISKINSIZE));                                                      // LisaWin //
            //GetClientSize(&dwx,&dwy);
            // Sometimes GetClientSize/SetClientSize return DIFFERNT values.  On OS X this causes the
            // Lisa window to GROW over time! m@+h3rf*(<R!!!  dwx/dwy is used to figure out the difference,
            // and then adjust to the size we want.  This is some sick shit that anyone has to code this way.
            //dwx-=x; dwy-=y;
            //DEBUG_LOG(0,"Resetting client size to %ld,%ld",(long)(x-dwx),(long)(y-dwy) );
            //SetClientSize(wxSize(x-dwx,y-dwy));                                                 // LisaWin //
            //SetMinSize(wxSize( _H(720), _H(500) ) );
            ox=(x  - effective_lisa_vid_size_x)/2;                                              // center display
            oy=(y  - effective_lisa_vid_size_y)/2;                                              // on skinless
            ox= (ox<0 ? 0:ox);
            oy= (oy<0 ? 0:oy);
            DEBUG_LOG(0,"Disable scrollbars");
            EnableScrolling(false,false);                                                       // LisaWin //

            DEBUG_LOG(0,"done.");
        } // --------------------------------------------------------------------------------------------------


        RePainter=&LisaWin::RePaint_AntiAliased;  //RePaint_AAGray;

        if (!!my_lisabitmap) delete my_lisabitmap;
        if (!!my_memDC)      delete my_memDC;

        my_memDC     =new class wxMemoryDC;
        my_lisabitmap=new class wxBitmap(o_effective_lisa_vid_size_x, o_effective_lisa_vid_size_y,DEPTH);

        my_memDC->SelectObjectAsSource(*my_lisabitmap);
        my_memDC->SetBrush(FILLERBRUSH);      my_memDC->SetPen(FILLERPEN);
        my_memDC->DrawRectangle(0 ,   0,   effective_lisa_vid_size_x, effective_lisa_vid_size_y);

        #ifdef DEBUG
        if (!my_memDC->IsOk())      ALERT_LOG(0,"my_memDC is not ok.");
        if (!my_lisabitmap->IsOk()) ALERT_LOG(0,"my_lisabitmap is not ok.");
        #endif


        /* Draw the VNC-style dot cursor we use later */
        #if defined(__WXX11__) || defined(__WXGTK__) || defined(__WXOSX__)
          #ifdef __WXOSX__
          /* LSB (0x01) is leftmost. First byte is top row. 1 is black/visible*/
            const unsigned char cursor_bits[] = {0x00, 0x0e, 0x0e, 0x0e, 0x00};
            const unsigned char mask_bits[]   = {0x1f, 0x1f, 0x1f, 0x1f, 0x1f};
          #else
            const unsigned char cursor_bits[] = {0xe0, 0xee, 0xee, 0xee, 0xe0};
            const unsigned char mask_bits[]   = {0xe0, 0xe0, 0xe0, 0xe0, 0xe0};
          #endif

        wxBitmap bmp = wxBitmap((const char *)cursor_bits, 8, 5, 1);
        bmp.SetMask(new wxMask(wxBitmap((const char *)mask_bits, 8, 5, 1)));

        wxImage img = bmp.ConvertToImage();

        img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, _T("2"));
        img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, _T("2"));

        m_dot_cursor = new wxCursor(img);
        #endif
}



LisaWin::~LisaWin(void)
{
#if defined(__WXX11__) || defined(__WXGTK__) || defined(__WXOSX__)
    delete m_dot_cursor;
#endif
}


void LisaEmFrame::OnVideoAntiAliased(wxCommandEvent& WXUNUSED(event))
{
    if (screensizex<IWINSIZEX || screensizey<IWINSIZEY)
    {
        wxString msg;
        msg.Printf(_T("Your display is only (%d,%d).  This mode needs at least (%d,%d), will shut off skins."),
                    screensizex,screensizey,  IWINSIZEX,IWINSIZEY);
        wxMessageBox(msg,_T("The display is too small"));
        turn_skins_off();

        if (screensizey<IWINSIZEY) {my_lisawin->SetVideoMode(vidmod_raw); return;}  // even still too small, go raw bits mode.
    }
    my_lisawin->SetVideoMode(vidmod_aa);
}

void LisaEmFrame::OnVideoHQ35X(wxCommandEvent& WXUNUSED(event))
{
    if (screensizex<IWINSIZEX || screensizey<IWINSIZEY)
    {
        wxString msg;
        msg.Printf(_T("Your display is only (%d,%d).  This mode needs at least (%d,%d), will shut off skins."),
                    screensizex,screensizey,  IWINSIZEX,IWINSIZEY);
        wxMessageBox(msg,_T("The display is too small"));
        turn_skins_off();

        if (screensizey<IWINSIZEY) {my_lisawin->SetVideoMode(vidmod_raw); return;}  // even still too small, go raw bits mode.
    }
    my_lisawin->SetVideoMode(vidmod_hq35x);
}

void LisaEmFrame::OnVideoAAGray(wxCommandEvent& WXUNUSED(event))
{

    if (screensizex<IWINSIZEX || screensizey<IWINSIZEY)
    {
        wxString msg;
        msg.Printf(_T("Your display is only (%d,%d).  This mode needs at least (%d,%d), will shut off skins."),
                    screensizex,screensizey,  IWINSIZEX,IWINSIZEY);
        wxMessageBox(msg,wxT("The display is too small"));
        turn_skins_off();
        if (screensizey<IWINSIZEY) {my_lisawin->SetVideoMode(vidmod_raw); return;}
    }

    my_lisawin->SetVideoMode(vidmod_aag);
}


void LisaEmFrame::OnVideoSingleY(wxCommandEvent& WXUNUSED(event))           {my_lisawin->SetVideoMode(vidmod_raw);}


void LisaEmFrame::OnVideoDoubleY(wxCommandEvent& WXUNUSED(event))
{

    if (screensizex<720+40 || screensizey<364*2+50)
    {
        wxString msg;
        msg.Printf(_T("Your display is only (%d,%d).  This mode needs at least (%d,%d)."),
                screensizex,screensizey,  720+40,364*2+50 );
        wxMessageBox(msg,wxT("The display is too small"));

     // oops!  The display size changed on us since the last time. i.e. notebook with small screen
     // previously connected to large monitor, now on native LCD
        if (lisa_ui_video_mode==vidmod_2y || lisa_ui_video_mode==vidmod_3y)
        {
            my_lisawin->SetVideoMode(vidmod_raw); turn_skins_off();
        }

        return;
    } /// display too small ///////////////////////////////////////////////////////////////


    if (skins_on)
    {
        if (yesnomessagebox("This mode doesn't work with the Lisa Skin.  Shut off the skin?",
                            "Remove Skin?")==0) return;
    }
    my_lisawin->SetVideoMode(vidmod_2y);
    turn_skins_off();
}


void LisaEmFrame::OnVideo2X3Y(wxCommandEvent& WXUNUSED(event))
{
    if (screensizex<(720*2+40)*hidpi_scale ||screensizey<(364*3+100)*hidpi_scale)
    {
        wxString msg;
        msg.Printf(_T("Your display is only (%d,%d).  This mode needs at least (%d,%d), or you could try changing the scale first"),
                        screensizex,screensizey,  720*2+40,364*3+100 );
        wxMessageBox(msg,wxT("The display is too small"));

        // oops!  The display size changed on us since the last time. i.e. notebook with small screen
        // previously connected to large monitor, now on native LCD
        if (lisa_ui_video_mode==1 || lisa_ui_video_mode==3)
        {
          my_lisawin->SetVideoMode(vidmod_raw);
          turn_skins_off();
        }

        return;
    }

    if (skins_on)
    {
        if (yesnomessagebox("This mode doesn't work with the Lisa Skin.  Turn off the skin?",
                            "Turn off skin?")==0) return;
    }

    my_lisawin->SetVideoMode(vidmod_3y);
    turn_skins_off();
}

void LisaEmFrame::OnSkinSelect(wxCommandEvent& WXUNUSED(event))
{
  wxArrayString choices;
  wxString resources = resdir + osslash + "resources" + osslash + "skins" + osslash;

  wxDir dir(resources);
  if  (!dir.IsOpened()) 
      {
          wxString msg;
          msg << "Could not open directory [" << resources << "] to search for skins";
          wxMessageBox(msg,_T("Could not open skin resources directory!"));
          return;
      }

  ALERT_LOG(0,"Opening dir resurces:%s",CSTR(resources) );
  wxString subdir;

  wxString filename;
  bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_DEFAULT);
  while ( cont )
  {
    wxString skinconfigpath=resources + osslash + filename + osslash + filename + ".conf";
    wxFileName conf(skinconfigpath);
    if  (conf.IsFileReadable()) 
        {
          ALERT_LOG(0,"%s is a valid skin",CSTR(skinconfigpath) ); 
          choices.Add(filename);
        }
    else 
        {
          ALERT_LOG(0,"%s is *NOT* valid skin",CSTR(skinconfigpath) );  
        }
    cont = dir.GetNext(&filename);
    my_lisawin->clear_skinless=1;
  }

  wxArrayString &choicesptr=choices;

  wxSingleChoiceDialog *d;
  d=new wxSingleChoiceDialog( my_lisaframe,
                              (const wxString &) ("Select a new skin"), 
                              (const wxString &) ("LisaEm Skin Selector"),
                              choicesptr
                            );
  if (d->ShowModal()==wxID_CANCEL) {delete d; return;}
  int r=d->GetSelection();
  skinname=choices[r];
  ALERT_LOG(0,"Chose: %d %s",r,CSTR(skinname));
  skindir  =  resdir   + "skins" + osslash + skinname + osslash;     // /usr/local/share/LisaEm/skins/default

  turn_skins_off();
  myConfig->Write(_T("/lisaskin/name"), skinname);  
  turn_skins_on();
  delete d;
}


void LisaEmFrame::OnSkinlessCenter(wxCommandEvent& event)
{
    skinless_center=!skinless_center;
    my_lisawin->clear_skinless=1;
    update_menu_checkmarks();
    save_global_prefs();
}

void LisaEmFrame::OnSkins(wxCommandEvent& WXUNUSED(event))
{

    my_lisawin->clear_skinless=1;
    if (skins_on)
    {
        // Turn off skins
        skins_on_next_run=0;
        skins_on=0;
        turn_skins_off();
    }
    else
    {
        // Turn on Skins
        if (lisa_ui_video_mode==vidmod_2y || lisa_ui_video_mode==vidmod_3y)
        {
            if (yesnomessagebox("The current display mode doesn't work with the Lisa Skin. Change modes?",
                                "Change Display Mode?")==0) return;
                setvideomode(vidmod_aa);
        }

        skins_on_next_run=1;
        skins_on=1;
        turn_skins_on();
    }
}


// Something is really fucked in wxWidgets, and it's been that way since at least 2.7.x as I had to do this with earlier
// LisaEms. Why? Unless I had some horrendous bug in LisaEm that I never caught for the last 12 years.
//
// I couldn't blit the whole skin onto the skin DC from an image, instead I had to break it up into 4 segments, which isn't
// terrible as the lower quarter of the skin is where the power button is and when it's light, the whole 4th segment has
// a glow to it. But why doesn't just work as it should?
// 
// If I use stretchblit 4x, it only works for the first of the 4 segments, but the rest show up as garbage, but other blits
// later such as the floppy or power animations have no issues, so what the fuck?
// The 2nd to last blits show trash in the image, even pulling in stuff from other windows in other apps like
// terminals and whatever else is open on the machine.
// 
// And reversing the order of the 4 segments produces the same fucked up issue where the bottom 1/4 is shown correctly at the top
// The 0th fragment works fine, but the 1st, 2nd, and 3rd produce garbage, why? Testing the DC says it's ok. But it isn't. Why not?
// 
// And the fuckedupedness of this exists in the actual image itself because taking a screenshot with skins saves it to disk
// into the png file. So it's not just a display bug.
//
// What the actual fuck? The fix is to use a temporary image which I call "skinny", use the normal 4x blit, and then one single
// StretchBlit at the end. But why didn't 4x StretchBlits work, and why do I need a temporary image that I have to new/delete?
// Dear wxDevs, please fix this shyte!

void prepare_skin(void)
{
    if (!my_lisaframe) return;
    if (!my_skin0DC || !my_skin1DC || !my_skin2DC || !my_skin3DC || !my_skin0 || !my_skin1 || !my_skin2 || !my_skin3)
        my_lisaframe->LoadImages();

    wxBitmap   *skinny=NULL;    
    wxMemoryDC *skinnyDC =NULL;

    set_hidpi_scale();
    setup_hidpi();

    if (!my_skin0) return;

    skin.width_size  =  my_skin0->GetWidth();
    skin.height_size =  my_skin0->GetHeight() +
                        my_skin1->GetHeight() + 
                        my_skin2->GetHeight() +  
                        my_skin3->GetHeight() ;

    skinny   = new wxBitmap(skin.width_size, skin.height_size, wxBITMAP_SCREEN_DEPTH);
    skinnyDC = new class wxMemoryDC;
    skinnyDC->SelectObjectAsSource(*skinny);

    int y=0;
    skinnyDC->Blit(0,y, my_skin0->GetWidth(),my_skin0->GetHeight(),my_skin0DC, 0,0 ,wxCOPY, false);   y+=my_skin0->GetHeight();
    skinnyDC->Blit(0,y, my_skin1->GetWidth(),my_skin1->GetHeight(),my_skin1DC, 0,0 ,wxCOPY, false);   y+=my_skin1->GetHeight();
    skinnyDC->Blit(0,y, my_skin2->GetWidth(),my_skin2->GetHeight(),my_skin2DC, 0,0 ,wxCOPY, false);   y+=my_skin2->GetHeight();
    
    skinnyDC->Blit(0,y, my_skin3->GetWidth(),my_skin3->GetHeight(),my_skin3DC, 0,0 ,wxCOPY, false); //y+=my_skin3->GetHeight();

    my_skinDC->StretchBlit( 0,0, _H(skinny->GetWidth() ), _H(skinny->GetHeight() ), skinnyDC, 
                            0,0,    skinny->GetWidth(),      skinny->GetHeight(),   wxCOPY, false);    

    delete skinnyDC;  // what a fucking waste!
    delete skinny;
}


// :TODO: maybe convert these to C++ templates?
#define SCALE_MENU(XscaleX)                                             \
  void LisaEmFrame::OnScale##XscaleX(  wxCommandEvent& WXUNUSED(event)) \
       {hidpi_scale=(XscaleX/100.0);                                    \
        save_global_prefs(); prepare_skin();                            \
        ALERT_LOG(0,"OnScaleMenu Zoom %f",hidpi_scale);                 \
        set_hidpi_scale(); update_menu_checkmarks(); }
        
SCALE_MENU(25);
SCALE_MENU(50);
SCALE_MENU(75);
SCALE_MENU(100);
SCALE_MENU(125);
SCALE_MENU(150);
SCALE_MENU(175);
SCALE_MENU(200);
SCALE_MENU(225);
SCALE_MENU(250);
SCALE_MENU(275);
SCALE_MENU(300);

void LisaEmFrame::OnZoomIn( wxCommandEvent &event)
{
    ALERT_LOG(0,"zoom in from: hidpi_scale:%d",(int)(hidpi_scale * 100));
    switch((int)(hidpi_scale * 100))
    {
        case  25:  OnScale50(event);  break;
        case  50:  OnScale75(event);  break;
        case  75:  OnScale100(event); break;

        case 100:  OnScale125(event); break;
        case 125:  OnScale150(event); break;
        case 150:  OnScale175(event); break;
        case 175:  OnScale200(event); break;
#ifndef __WXOSX__
        case 200:  OnScale225(event); break;
        case 225:  OnScale250(event); break;
        case 250:  OnScale275(event); break;
        case 275:  OnScale300(event); break;
        case 300:                     break;
#endif
    }
}

void LisaEmFrame::OnZoomOut( wxCommandEvent &event)
{
    ALERT_LOG(0,"zoom out from: hidpi_scale:%d",(int)(hidpi_scale * 100));
    switch((int)(hidpi_scale * 100))           // zoom out
    {
        case  25:                     break;
        case  50:  OnScale25(event);  break;
        case  75:  OnScale50(event);  break;
        case 100:  OnScale75(event);  break;
        case 125:  OnScale100(event); break;
        case 150:  OnScale125(event); break;
        case 175:  OnScale150(event); break;
        case 200:  OnScale175(event); break;
        case 225:  OnScale200(event); break;
        case 250:  OnScale225(event); break;
        case 275:  OnScale250(event); break;
        case 300:  OnScale275(event); break;
    }
}

#ifdef EVT_GESTURE_ZOOM
void LisaEmFrame::OnZoom( wxZoomGestureEvent& evt)
{
     wxCommandEvent evx;
     wxCommandEvent& ev = evx;
     ALERT_LOG(0,"scale/zoom in/out event %f",evt.GetZoomFactor());

     if (evt.GetZoomFactor()>1.0) OnZoomIn(ev);
     else                         OnZoomOut(ev);
}
#endif


void LisaEmFrame::OnFullScreen(   wxCommandEvent& WXUNUSED(event)) 
{
    //int isfullscreen=IsFullScreen();
    int   isfullscreen;
    ALERT_LOG(0,"entering");
    if (!FullScreenCheckMenuItem) return; // incase we somehow got here before the menu was built
    ALERT_LOG(0,"processing");
    isfullscreen=FullScreenCheckMenuItem->IsChecked();

    #if defined(__WXOSX) && !defined(EnableFullScreenView)
     #define SHOW_MENU_IN_FULLSCREEN 1
    #endif

    #if defined(__WXOSX__) && defined(EnableFullScreenView)
      EnableFullScreenView(!IsFullScreen());
    #else
      // likely it's a bad idea to be in full screen without the ability to show the menu
      // not sure if OS X will show it anyway when you move the mouse up
      #if !defined(SHOW_MENU_IN_FULLSCREEN) && !defined(__WXOSX__)
        ShowFullScreen(isfullscreen,wxFULLSCREEN_ALL);
      #else
        ShowFullScreen(isfullscreen,wxFULLSCREEN_NOTOOLBAR | wxFULLSCREEN_NOSTATUSBAR |
                                     wxFULLSCREEN_NOBORDER  | wxFULLSCREEN_NOCAPTION );
      #endif
    #endif

    update_menu_checkmarks();
    save_global_prefs();
    ALERT_LOG(0,"done");
}

// should look into making these C++ templates instead
#define OnRefresh(XHertzX)                                              \
void LisaEmFrame::OnRefresh##XHertzX(  wxCommandEvent& WXUNUSED(event)) \
{                                                                       \
     hostrefresh=1000/XHertzX; refresh_rate=1*REFRESHRATE;              \
     refresh_rate_used=refresh_rate;                                    \
     save_global_prefs();update_menu_checkmarks();                      \
}

OnRefresh(60);
OnRefresh(20);
OnRefresh(12);
OnRefresh(8);
OnRefresh(4);

void LisaEmFrame::OnHideHostMouse(wxCommandEvent& WXUNUSED(event)) 
{ hide_host_mouse=!hide_host_mouse;   save_global_prefs();}


extern "C" long get_wx_millis(void) { return my_lisaframe->runtime.Time();}



void set_hidpi_scale(void)
{
    static float last_hidpi;
    ALERT_LOG(0,"\n ***** hidpi_scale is currently set to %f ****\n",hidpi_scale);
    long hdpiscale;
    if  (hidpi_scale==0.0)
        {
          long hdpiscale = myConfig->Read(_T("/hidpi_scale"), (int)(hidpi_scale*100));
          hidpi_scale=hdpiscale / 100.0;
        }

hdpiscale=hidpi_scale*100;

// tested working with wx3.1.2 GTK, not guaranteed to work elsewhere, needs a metric naval fuckton of tsting
// according to https://mycurvefit.com/ : y = 0.999998 + (5.666667 - 0.999998)/(1 + (x/23.37221)^19.29771)
#ifdef __WXGTK__
switch(hdpiscale) {
    case  25:  mouse_scale = 2.0     ; break;
    case  50:  mouse_scale = 1.0     ; break;
    case  75:  mouse_scale = 0.67    ; break;
    case 100:  mouse_scale = 0.505   ; break;
    case 125:  mouse_scale = 0.3989  ; break;
    case 150:  mouse_scale = 0.336   ; break;
    case 175:  mouse_scale = 0.30455 ; break; 
    case 200:  mouse_scale = 0.253   ; break;
    case 225:  mouse_scale = 0.225   ; break;
    case 250:  mouse_scale = 0.2     ; break; 
    case 275:  mouse_scale = 1.85    ; break; // why the uptick?
    case 300:  mouse_scale = 1.7     ; break;
    // according to https://mycurvefit.com/ : y = 0.999998 + (5.666667 - 0.999998)/(1 + (x/23.37221)^19.29771)
    default: mouse_scale = 0.999998 + (5.666667 - 0.999998)/(1 + pow(hdpiscale/23.37221,19.29771) );
}
#endif
/* windows so far  - the ones with extra zeros are known good. */
#ifdef __WXMSW__
switch(hdpiscale) {
    case  25:  mouse_scale = 4.0000; break;
    case  50:  mouse_scale = 2.0000; break;
    case  75:  mouse_scale = 1.3000; break;
    case 100:  mouse_scale = 1.0000; break;
    case 125:  mouse_scale = 0.7990; break;
    case 150:  mouse_scale = 0.6720; break;
    case 175:  mouse_scale = 0.5750; break; 
    case 200:  mouse_scale = 0.5000; break;
    case 225:  mouse_scale = 0.4473; break;
    case 250:  mouse_scale = 0.4000; break; 
    case 275:  mouse_scale = 0.3645; break;
    case 300:  mouse_scale = 0.3350; break;
}
#endif
#ifdef __WXOSX__
switch(hdpiscale) {
    case  25:  mouse_scale = 4.00000; break;
    case  50:  mouse_scale = 2.00000; break;
    case  75:  mouse_scale = 1.34500; break;
    case 100:  mouse_scale = 1.00000; break;
    case 125:  mouse_scale = 0.80000; break;
    case 150:  mouse_scale = 0.67000; break;
    case 175:  mouse_scale = 0.57500; break;
    case 200:  mouse_scale = 0.50000; break;
}
#endif



#ifdef DEBUGXXXXXXX
// live debug, read these values from a file so I can change them live while debugging.
{
  FILE *fh;
  int d;
  float f;
  char line[120];
  fh=fopen("mousescale.txt","rt");
  if (fh) {
    ALERT_LOG(0,"opened mousescale.txt, reading.");
    while  (!feof(fh)) {
          fscanf(fh,"%d,%f\n",&d,&f);
          switch (d) {
              case  25:  if (d==hdpiscale) mouse_scale=f; break;
              case  50:  if (d==hdpiscale) mouse_scale=f; break;
              case  75:  if (d==hdpiscale) mouse_scale=f; break;
              case 100:  if (d==hdpiscale) mouse_scale=f; break;
              case 125:  if (d==hdpiscale) mouse_scale=f; break;
              case 150:  if (d==hdpiscale) mouse_scale=f; break;
              case 175:  if (d==hdpiscale) mouse_scale=f; break; 
              case 200:  if (d==hdpiscale) mouse_scale=f; break;
              case 225:  if (d==hdpiscale) mouse_scale=f; break;
              case 250:  if (d==hdpiscale) mouse_scale=f; break; 
              case 275:  if (d==hdpiscale) mouse_scale=f; break;
              case 300:  if (d==hdpiscale) mouse_scale=f; break;
              default: ALERT_LOG(0,"Unknown hdpiscale:%d scale:%f in mousescale.txt",d,f);
          }
    }
    fclose(fh);
  }
}
#endif


    if  (!!DisplayScaleSub && !!my_lisaframe && !!my_lisaframe && last_hidpi!=hidpi_scale)
        {
            // 20191105 try to fix skinless crashes when changing scale
            if (my_lisabitmap) {delete(my_lisabitmap); my_lisabitmap=NULL;}

            update_menu_checkmarks();

            if  (myConfig) 
                {
                  myConfig->Write(_T("/hidpi_scale"),(long)(hidpi_scale * 100.0 ) ); 
                  ALERT_LOG(0,"Write scale to config %f",hidpi_scale);
                }
            last_hidpi=hidpi_scale;

            if (skins_on) {ALERT_LOG(0,"Reloading skins");turn_skins_off(); turn_skins_on();}

          //my_lisawin->EnableScrolling(false, false);
          //my_lisawin->SetScrollbars(ISKINSIZEX / _H(100), ISKINSIZEY / _H(100), 100, 100, 0, 0, true);
          //my_lisawin->EnableScrolling(true, true);
          force_refresh();
        }

}


void save_global_prefs(void)
{
    int x=0,y=0;

    if (!myConfig)     return;                                 // not initialized yet
    if (!DisplayMenu)  return;
    if (!my_lisaframe) return;
    if (!fileMenu)     return;

    myConfig->Write(_T("/soundeffects"),sound_effects_on);
    myConfig->Write(_T("/displayskins"),skins_on_next_run);
    myConfig->Write(_T("/displaymode"), (long)lisa_ui_video_mode);
    myConfig->Write(_T("/centerskinless"),(long)skinless_center);

    myConfig->Write(_T("/asciikeyboard"), (long)asciikeyboard);
    myConfig->Write(_T("/lisaconfigfile"),myconfigfile);
    myConfig->Write(_T("/throttle"),(long)my_lisaframe->throttle);

    myConfig->Write(_T("/emutime"),(long)emulation_time);
    myConfig->Write(_T("/emutick"),(long)emulation_tick);

    myConfig->Write(_T("/refreshrate"),(long)refresh_rate);
    myConfig->Write(_T("/hidehostmouse"),(long)hide_host_mouse);

    my_lisawin->GetClientSize(&x,&y);
    myConfig->Write(_T("/lisawin/sizey"),(long)y); //-my_lisawin->dwy);  // 2019.06.19 these are negative!
    myConfig->Write(_T("/lisawin/sizex"),(long)x); //-my_lisawin->dwx);
    my_lisaframe->GetClientSize(&x,&y);
    myConfig->Write(_T("/lisaframe/sizey"),(long)y); //-my_lisaframe->dwy);
    myConfig->Write(_T("/lisaframe/sizex"),(long)x); //-my_lisaframe->dwx);
    myConfig->Write(_T("/lisaframe/fullscreen"),(long)(my_lisaframe->IsFullScreen()) );
    myConfig->Write(_T("/lisaskin/name"), my_lisaframe->skinname);
    myConfig->Write(_T("/hidpi_scale"),(long)(hidpi_scale * 100.0 ) );

    myConfig->Flush(); //valgrind==24726== Conditional jump or move depends on uninitialised value(s)

    if  ( refresh_rate!=9*REFRESHRATE  && refresh_rate!=7*REFRESHRATE  &&
          refresh_rate!=5*REFRESHRATE  && refresh_rate!=3*REFRESHRATE  &&
          refresh_rate!=  REFRESHRATE                                    ) refresh_rate =  REFRESHRATE;

    my_lisaframe->hostrefresh=1000/60;
    update_menu_checkmarks();
}


void LisaEmFrame::SetStatusBarText(wxString &msg) {SetStatusText(msg,0);}

DECLARE_APP(LisaEmApp)           // Implements LisaEmApp& GetApp()
IMPLEMENT_APP(LisaEmApp)         // Give wxWidgets the means to create a LisaEmApp object //valgrind reports:: Conditional jump or move depends on uninitialised value(s)


void LisaEmApp::LisaSkinConfig(void)
{
    wxStandardPathsBase& stdp = wxStandardPaths::Get();
    wxString defaultconfig = wxGetHomeDir();
    wxString rscDir = stdp.GetResourcesDir() + wxFileName::GetPathSeparator(wxPATH_NATIVE);
    wxString sndfile;

    // stare not upon the insanity that predated this code, for it was written by drunkards! (*Burp*)
    sndfile=rscDir + skin.floppy_eject;           my_floppy_eject  =       new wxSound(sndfile);

    myconfigfile=myConfig->Read(_T("/lisaskin"),defaultconfig);

    pConfig=new wxFileConfig( _T("LisaEm"),
                              _T("sunder.NET"),
                              (myconfigfile),     //local
                              (myconfigfile),     //global
                              wxCONFIG_USE_LOCAL_FILE,
                              wxConvAuto() );   // or wxConvUTF8
}


// Initialize the application
bool LisaEmApp::OnInit()
{
    if (!wxApp::OnInit()) return false;      // call default behaviour (mandatory)

    wxStandardPathsBase& stdp = wxStandardPaths::Get();
    wxString defaultconfig=stdp.GetUserConfigDir();
    wxString osslash  = wxFileName::GetPathSeparator(wxPATH_NATIVE);


// can't debug in windows since LisaEm is not a console app, so redirect buglog to an actual file.
#if defined(__WXMSW__)    && defined(DEBUG) 
  buglog=fopen("lisaem-output.txt","a+");
#endif

    // this crashes on macOSX>10.9 (maybe earlier too, crashes on the call, before it even returns)
    //wxString defaultconfig = stdp.wxGetHomeDir();
    defaultconfig  << osslash << "/lisaem.conf";

    myConfig = wxConfig::Get();         // this one is the global configuration for the app.
                                        // get the path to the last opened Lisaconfig and load that.
    myconfigfile=myConfig->Read(_T("/lisaconfigfile"),defaultconfig);

    if (on_start_lisaconfig!="") {myconfigfile=on_start_lisaconfig; on_start_lisaconfig="";}

    ALERT_LOG(0,"Reading this Lisa config...");

    pConfig=new wxFileConfig(_T("LisaEm"),
                             _T("sunder.NET"),
                             (myconfigfile),     //local
                             (myconfigfile),     //global
                             wxCONFIG_USE_LOCAL_FILE,
                             wxConvAuto() );   // or wxConvUTF8

    // this is a global setting  - must be right before the LisaFrame!
    skins_on          = (int)myConfig->Read(_T("/displayskins"),(long)1);
    skins_on_next_run = skins_on;

    skinless_center   = (int)myConfig->Read(_T("/centerskinless"),(long)1);
    if (on_start_center==wxCMD_SWITCH_ON)  {skinless_center=1; on_start_center=0;}
    if (on_start_center==wxCMD_SWITCH_OFF) {skinless_center=1; on_start_center=0;}

    refresh_rate      = (long)myConfig->Read(_T("/refreshrate"),(long)REFRESHRATE);
    hide_host_mouse   = (int) myConfig->Read(_T("/hidehostmouse"),(long)0);
    refresh_rate_used = refresh_rate;
    sound_effects_on  = (int)myConfig->Read(_T("/soundeffects"),(long)1);
    lisa_ui_video_mode= (int)myConfig->Read(_T("/displaymode"), (long)0);
    asciikeyboard     = (int)myConfig->Read(_T("/asciikeyboard"),(long)1);

    emulation_time    = (long)myConfig->Read(_T("/emutime"),(long)100);
    emulation_tick    = (long)myConfig->Read(_T("/emutick"),(long)75);

    long hdpiscale = myConfig->Read(_T("/hidpi_scale"),    (int)(hidpi_scale * 100));
    ALERT_LOG(0, "read /hidpi_scale %ld from preferences", (long)hdpiscale)

    // should I get rid of this switch block and allow the user to pass whatever they like?
    int osz=(int)(on_start_zoom*100);
    switch(osz)
    {
      case  25: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case  50: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case  75: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 100: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 125: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 150: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 175: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 200: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 225: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 250: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 275: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;
      case 300: hidpi_scale = on_start_zoom; ALERT_LOG(0,"zoom override to %f",hidpi_scale); set_hidpi_scale(); break;

      default:  // get it from config if it's not on the commandline or if it's invalid on the command line
                hidpi_scale = (hdpiscale == 0 ? 1 : hdpiscale) / 100.0;
                ALERT_LOG(0, "Setting HDPI Scale %ld from config, setting to: %f", (long)hdpiscale, hidpi_scale);
                set_hidpi_scale();
                break;
    }

    ALERT_LOG(0,"Reading Lisa instance Specific config.");
    my_lisaconfig = new LisaConfig();

    pConfig->SetRecordDefaults();

    my_lisaconfig->Load(pConfig, floppy_ram);// load it in
    my_lisaconfig->Save(pConfig, floppy_ram);// save it so defaults are created if the file doesn't exist yet

    SetVendorName(_T("sunder.NET"));

    // thanks to: https://github.com/kuroneko/lisaem/commit/94781301aedf91c99f7731f3513f6c4351d63baa
    #ifdef __WXOSX__
    SetAppName(_T("LisaEm"));
    #else
    SetAppName(_T("lisaem"));
    #endif

    if  (!my_lisaframe) 
        {
          ALERT_LOG(0,"Creating new LisaEmFrame");
          my_lisaframe=new LisaEmFrame(wxT("LisaEm"));
          ALERT_LOG(0,"Created LisaEmFrame");
        }

    if  (!my_lisawin) 
        {
          ALERT_LOG(0,"Creating new LisaWin");
          my_lisawin = new LisaWin(my_lisaframe);
          ALERT_LOG(0,"Created LisaWin");
        }  

    my_lisaframe->running=emulation_off;     // CPU isn't yet turned on
    my_lisaframe->throttle=(float)(myConfig->Read(_T("/throttle"),(long)5));

    my_lisawin->repaintall = REPAINT_INVALID_WINDOW;
    my_lisaframe->Show(true);                // Light it up
    my_lisawin->repaintall = REPAINT_INVALID_WINDOW;

    wxString sndfile;
    
    ALERT_LOG(0,"Loading sounds");
    sndfile=my_lisaframe->skindir + skin.floppy_eject;           my_floppy_eject  =       new wxSound(sndfile);
    sndfile=my_lisaframe->skindir + skin.floppy_insert;          my_floppy_insert =       new wxSound(sndfile);
    sndfile=my_lisaframe->skindir + skin.floppy_motor1;          my_floppy_motor1 =       new wxSound(sndfile);
    sndfile=my_lisaframe->skindir + skin.floppy_motor2;          my_floppy_motor2 =       new wxSound(sndfile);
    sndfile=my_lisaframe->skindir + skin.lisa_power_switch01;    my_lisa_power_switch01 = new wxSound(sndfile);
    sndfile=my_lisaframe->skindir + skin.lisa_power_switch02;    my_lisa_power_switch02 = new wxSound(sndfile);
    sndfile=my_lisaframe->skindir + skin.poweroffclk;            my_poweroffclk =         new wxSound(sndfile);
    
    ALERT_LOG(0,"Update ProFile menu");
    my_lisaframe->UpdateProfileMenu();

    if (on_start_skin ==  wxCMD_SWITCH_ON)  turn_skins_on();
    if (on_start_skin ==  wxCMD_SWITCH_OFF) turn_skins_off();

    on_start_skin=wxCMD_SWITCH_NOT_FOUND; // mark it as not found since we already processed it

    ALERT_LOG(0,"init black");
    black();


    if (on_start_fullscreen==wxCMD_SWITCH_ON || 
        ( myConfig->Read(_T("/lisaframe/fullscreen"),(long)0) && (on_start_fullscreen!=wxCMD_SWITCH_OFF) ) )
        {
          wxCommandEvent foo;
          on_start_fullscreen=0;
          my_lisaframe->OnFullScreen(foo);
          ALERT_LOG(0,"on_start_fullscreen or last state was fullscreen");
        }
    setvideomode(lisa_ui_video_mode);

    ALERT_LOG(0,"OnInit Done.")
    return true;
}


char *getbanner(void)
{
    const int len=2048;
    static char banner[len];
          //         .........1.........2.........3.........4.........5.........6.........7
          //         123456789012345678901234567890123456789012345678901234567890123456789012345678
    snprintf(banner,len,
                    "-----------------------------------------------------------------------\n"
                    "  The Lisa II Emulator %-17s http://lisaem.sunder.net\n"
                    "  -------------------------------------------------------------------  \n"
                    "  Copyright  (C) MMXX  by Ray A. Arachelian,   All Rights Reserved     \n"
                    "  Released  under  the terms of  the  GNU Public License  Version 2.0  \n"
                    "  -------------------------------------------------------------------  \n"
                    "  For historical/archival/educational use only - no warranty provied.  \n"
                    "  -------------------------------------------------------------------  \n"
                    "  Portions of this software contain bits of code from the following:   \n"
                    "  generator - www.squish.net/generator  wxWidgets.org  - www.ijg.org   \n"
                    "  LZ4 Library Copyright (c) 2011-2016, Yann Collet All rights reserved.\n"
                    "   Many thanks to David T. Craig for the tons of Lisa documentation    \n"
                    "     Many thanks to James McPhail for Lisa and 68000 hardware help     \n"
                    "      Many thanks to Brain Folley for the initial OS X UI help         \n"
                    "-----------------------------------------------------------------------\n\n"

                    #ifdef DEBUG
                    "DEBUG is compiled in.\n"
                    #endif
                    #ifdef DEBUGMEMCALLS
                    "DEBUGMEMCALLS is enabled.\n"
                    #endif
                    ,my_version );

  return banner;
}


void banner(void)    { puts(getbanner());}


void LisaEmApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc (cmdLineDesc);
    parser.SetSwitchChars (wxT("-"));
    parser.SetLogo( getbanner() );   

}


bool LisaEmApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    int kioskmode=0;
    on_start_poweron          = parser.Found(wxT("p"));
    on_start_harddisk         = parser.Found(wxT("d"));
    on_start_fullscreen       = parser.FoundSwitch(wxT("F"));  // negateable
    on_start_skin             = parser.FoundSwitch(wxT("s"));  // negateable
    on_start_quit_on_poweroff = parser.Found(wxT("q"));

    parser.Found(wxT("f"),&on_start_floppy);
    parser.Found(wxT("c"),&on_start_lisaconfig);
    parser.Found(wxT("z"),&on_start_zoom);

    on_start_center=parser.FoundSwitch(wxT("o"));

    kioskmode= parser.FoundSwitch(wxT("k"));
    if (kioskmode)
    {
      skinless_center=0;
      on_start_center=0;
      on_start_fullscreen=1;
      on_start_skin=0;
      on_start_quit_on_poweroff = 1;
      on_start_poweron=1;
    }
    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Keyboard Scancodes - these are valid for all keyboards, except that 42 is not on the US keyboard
//                      and 48 is not on the EU keyboard.
//
//                               0    1    2    3    4    5    6    7    8    9   10   11  12   13  14
uint8 kbcodes[5][15]=      { {0x68,0x74,0x71,0x72,0x73,0x64,0x61,0x62,0x63,0x50,0x51,0x40,0x41,0x45, 0 },
                             {0x78,0x75,0x77,0x60,0x65,0x66,0x67,0x52,0x53,0x5f,0x44,0x56,0x57,0x42, 0 },
                             {0x7d,0x70,0x76,0x7b,0x69,0x6a,0x6b,0x54,0x55,0x59,0x5a,0x5b,0x42,0x48, 0 },
                             {0x7e,0x43,0x79,0x7a,0x6d,0x6c,0x6e,0x6f,0x58,0x5d,0x5e,0x4c,0x7e,   0, 0 },
                             {0x7c,0x7f,0x5c,0x46,0x4e,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 } };
//
//                              0    1    2    3   4
uint8 kbcodesn[ 5][5]=     { {0x20,0x21,0x22,0x23, 0},
                             {0x24,0x25,0x26,0x27, 0},
                             {0x28,0x29,0x2a,0x2b, 0},
                             {0x4d,0x2d,0x2e,0x2f, 0},
                             {0x49,0x2c,   0,   0, 0}                                                    };
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Keyboard Legends - need to build these for the other languages /////////////////////////////////////////////////
//                  - also need to build 3x more (shifted, option, option+shift)
//
//                             0        1   2   3   4   5   6   7   8   9   10 11  12  13          14
char *kb_us_r1_txt[5][15]= { {"`",     "1","2","3","4","5","6","7","8","9","0","-","=","BACKSPACE","" },
                             {"TAB",   "q","w","e","r","t","y","u","i","o","p","[","]","\\"       ,"" },
                             {"CAPS",  "a","s","d","f","g","h","j","k","l",";","'","" ,"RETURN"   ,"" },
                             {"SHIFT", "", "z","x","c","v","b","n","m",",",".","/","SHIFT",""     ,"" },
                             {"OPT","CMD","SPACE","ENTER","OPT","","","","","","","","",""        ,"" }  };
//
char *kb_us_n1_txt[ 5][5]= { {"CLR","-","+","*"    ,""  },
                             {  "7","8","9","/"    ,""  },
                             {  "4","5","6",","    ,""  },
                             {  "1","2","3","ENTER",""  },
                             {  "0",".","" ,""     ,""  }                                                };
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//borrowed from wxWidgets example code:  keyboard.cpp by  Vadim Zeitlin
void LisaWin::LogKeyEvent(const wxChar* WXUNUSED(name), wxKeyEvent& event, int keydir)
{
    wxString key;
    long keycode = event.GetKeyCode();
    int lisakey=0;

    int forcelisakey=0;

    // on linux for some reason I get keycode=-1 for ALT!!!
    #ifdef __WXX11__
      if (keycode==-1) { ALERT_LOG(0,"ALT key hit:%d",keycode); keycode=WXK_ALT; }
    #endif

#ifdef DEBUG
                if (debug_log_onclick)
                {
                  debug_log_onclick=0;
                  debug_log_enabled=!debug_log_enabled;
                  if (debug_log_enabled) {debug_on("onclick");}
                  else                   {debug_off(); ALERT_LOG(0,"Debug log turned off on RIGHT CLICK");}
                }
#endif

    //ALERT_LOG(0,"Received %08x keycode on event %d",keycode,keydir);
    switch ( keycode )
    {
//
// need to implement variations of these for the other keyboard languages too.
// also should get rid of this and replace it with some sort of table/array instead.
//
        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:        lisakey=KEYCODE_CURSORL;  forcelisakey=1;       break;

        case WXK_UP :
        case WXK_NUMPAD_UP:          lisakey=KEYCODE_CURSORU;  forcelisakey=1;       break;

        case WXK_RIGHT :
        case WXK_NUMPAD_RIGHT:       lisakey=KEYCODE_CURSORR;  forcelisakey=1;       break;

        case WXK_DOWN:
        case WXK_NUMPAD_DOWN :       lisakey=KEYCODE_CURSORD;  forcelisakey=1;       break;


        case WXK_NUMPAD_INSERT:      lisakey=KEYCODE_LENTER;          break;
        case WXK_NUMPAD_DELETE:      lisakey=KEYCODE_BACKSPACE;       break;

      //case WXK_NUMPAD_EQUAL:       lisakey=KEYCODE_EQUAL;           break;
        case WXK_NUMPAD_MULTIPLY:    lisakey=KEYCODE_STAR_NUM;        break;
        case WXK_NUMPAD_ADD:         lisakey=KEYCODE_PLUS_NUM;        break;
        case WXK_NUMPAD_SEPARATOR:
        case WXK_NUMPAD_SUBTRACT:    lisakey=KEYCODE_MINUS_NUM;       break;
        case WXK_NUMPAD_DECIMAL:     lisakey=KEYCODE_DOTNUM;          break;
        case WXK_NUMPAD_DIVIDE:      lisakey=KEYCODE_FSLASH;          break;

        case WXK_NUMPAD_ENTER:       lisakey=KEYCODE_ENTERNUM;        break;
        case WXK_NUMLOCK:            lisakey=KEYCODE_CLEAR;           break;

        case WXK_NUMPAD0:            lisakey=KEYCODE_0NUM;            break;
        case WXK_NUMPAD1:            lisakey=KEYCODE_1NUM;            break;
        case WXK_NUMPAD2:            lisakey=KEYCODE_2NUM;            break;
        case WXK_NUMPAD3:            lisakey=KEYCODE_3NUM;            break;
        case WXK_NUMPAD4:            lisakey=KEYCODE_4NUM;            break;
        case WXK_NUMPAD5:            lisakey=KEYCODE_5NUM;            break;
        case WXK_NUMPAD6:            lisakey=KEYCODE_6NUM;            break;
        case WXK_NUMPAD7:            lisakey=KEYCODE_7NUM;            break;
        case WXK_NUMPAD8:            lisakey=KEYCODE_8NUM;            break;
        case WXK_NUMPAD9:            lisakey=KEYCODE_9NUM;            break;

        case WXK_BACK:               lisakey=KEYCODE_BACKSPACE;       break;
        case WXK_TAB:                lisakey=KEYCODE_TAB;             break;
        case WXK_RETURN:             lisakey=KEYCODE_RETURN;          break;
        case WXK_ESCAPE:             lisakey=KEYCODE_CLEAR;           break;
        case WXK_SPACE:              lisakey=KEYCODE_SPACE;           break;
        case WXK_DELETE:             lisakey=KEYCODE_BACKSPACE;       break;

        case 'a' :                   lisakey=KEYCODE_A;               break;
        case 'b' :                   lisakey=KEYCODE_B;               break;
        case 'c' :                   lisakey=KEYCODE_C;               break;
        case 'd' :                   lisakey=KEYCODE_D;               break;
        case 'e' :                   lisakey=KEYCODE_E;               break;
        case 'f' :                   lisakey=KEYCODE_F;               break;
        case 'g' :                   lisakey=KEYCODE_G;               break;
        case 'h' :                   lisakey=KEYCODE_H;               break;
        case 'i' :                   lisakey=KEYCODE_I;               break;
        case 'j' :                   lisakey=KEYCODE_J;               break;
        case 'k' :                   lisakey=KEYCODE_K;               break;
        case 'l' :                   lisakey=KEYCODE_L;               break;
        case 'm' :                   lisakey=KEYCODE_M;               break;
        case 'n' :                   lisakey=KEYCODE_N;               break;
        case 'o' :                   lisakey=KEYCODE_O;               break;
        case 'p' :                   lisakey=KEYCODE_P;               break;
        case 'q' :                   lisakey=KEYCODE_Q;               break;
        case 'r' :                   lisakey=KEYCODE_R;               break;
        case 's' :                   lisakey=KEYCODE_S;               break;
        case 't' :                   lisakey=KEYCODE_T;               break;
        case 'u' :                   lisakey=KEYCODE_U;               break;
        case 'v' :                   lisakey=KEYCODE_V;               break;
        case 'w' :                   lisakey=KEYCODE_W;               break;
        case 'x' :                   lisakey=KEYCODE_X;               break;
        case 'y' :                   lisakey=KEYCODE_Y;               break;
        case 'z' :                   lisakey=KEYCODE_Z;               break;

        case 'A' :                   lisakey=KEYCODE_A;               break;
        case 'B' :                   lisakey=KEYCODE_B;               break;
        case 'C' :                   lisakey=KEYCODE_C;               break;
        case 'D' :                   lisakey=KEYCODE_D;               break;
        case 'E' :                   lisakey=KEYCODE_E;               break;
        case 'F' :                   lisakey=KEYCODE_F;               break;
        case 'G' :                   lisakey=KEYCODE_G;               break;
        case 'H' :                   lisakey=KEYCODE_H;               break;
        case 'I' :                   lisakey=KEYCODE_I;               break;
        case 'J' :                   lisakey=KEYCODE_J;               break;
        case 'K' :                   lisakey=KEYCODE_K;               break;
        case 'L' :                   lisakey=KEYCODE_L;               break;
        case 'M' :                   lisakey=KEYCODE_M;               break;
        case 'N' :                   lisakey=KEYCODE_N;               break;
        case 'O' :                   lisakey=KEYCODE_O;               break;
        case 'P' :                   lisakey=KEYCODE_P;               break;
        case 'Q' :                   lisakey=KEYCODE_Q;               break;
        case 'R' :                   lisakey=KEYCODE_R;               break;
        case 'S' :                   lisakey=KEYCODE_S;               break;
        case 'T' :                   lisakey=KEYCODE_T;               break;
        case 'U' :                   lisakey=KEYCODE_U;               break;
        case 'V' :                   lisakey=KEYCODE_V;               break;
        case 'W' :                   lisakey=KEYCODE_W;               break;
        case 'X' :                   lisakey=KEYCODE_X;               break;
        case 'Y' :                   lisakey=KEYCODE_Y;               break;
        case 'Z' :                   lisakey=KEYCODE_Z;               break;

        case '0' :                   lisakey=KEYCODE_0;               break;
        case '1' :                   lisakey=KEYCODE_1;               break;
        case '2' :                   lisakey=KEYCODE_2;               break;
        case '3' :                   lisakey=KEYCODE_3;               break;
        case '4' :                   lisakey=KEYCODE_4;               break;
        case '5' :                   lisakey=KEYCODE_5;               break;
        case '6' :                   lisakey=KEYCODE_6;               break;
        case '7' :                   lisakey=KEYCODE_7;               break;
        case '8' :                   lisakey=KEYCODE_8;               break;
        case '9' :                   lisakey=KEYCODE_9;               break;


        case        '~':
        case        '`':             lisakey=KEYCODE_TILDE;           break;

        case        '_':
        case        '-':             lisakey=KEYCODE_MINUS;           break;

        case        '+':
        case        '=':             lisakey=KEYCODE_PLUS;            break;

        case        '{':
        case        '[':             lisakey=KEYCODE_OBRAK;           break;

        case        '}':
        case        ']':             lisakey=KEYCODE_CBRAK;           break;


        case        '|':
        case        '\\':            lisakey=KEYCODE_BSLASH;          break;

        case        ':':
        case        ';':             lisakey=KEYCODE_COLON;           break;

        case        '"':
        case        '\'':            lisakey=KEYCODE_QUOTE;           break;

        case        '<':
        case        ',':             lisakey=KEYCODE_COMMA;           break;

        case        '>':
        case        '.':             lisakey=KEYCODE_DOT;             break;

        case        '?':
        case        '/':             lisakey=KEYCODE_FSLASHQ;         break;


        case WXK_SHIFT:              lisakey=KEYCODE_SHIFT;           break;  

#if defined(__WXX11__) || defined(__WXGTK__)
        case WXK_ALT:                lisakey=KEYCODE_COMMAND;         break; // may need to change these around for linux
        case WXK_CONTROL:            lisakey=KEYCODE_LOPTION;         break;
        case WXK_MENU:               lisakey=KEYCODE_COMMAND;         break;

#endif
#if defined(__WXOSX__)    
        case WXK_ALT:                lisakey=KEYCODE_COMMAND;         break; 
        //case WXK_CONTROL:            lisakey=KEYCODE_LOPTION;         break;
        //src/host/wxui/lisaem_wx.cpp:3157:14: error: duplicate case value: 'WXK_CONTROL' and 'WXK_COMMAND' both equal '308'
        case WXK_COMMAND:            lisakey=KEYCODE_LOPTION;         break;

#endif
#if defined(__WXMSW__)    
        case WXK_ALT:                lisakey=KEYCODE_COMMAND;         break; 
        case WXK_CONTROL:            lisakey=KEYCODE_LOPTION;         break;
#endif
        default: ALERT_LOG(0,"Uncaught Keycode:%ld",(long)keycode);   break;

//Keycode:309 - linux windows-menu-key
/*
  WXK_WINDOWS_LEFT,  only generated under Windows currently.
  WXK_WINDOWS_RIGHT, only generated under Windows currently.
  WXK_WINDOWS_MENU   only generated under Windows currently.
  WXK_COMMAND, "Under OS X, this key maps to the 'Command' (aka logo or 'Apple') key, whereas on Linux/Windows/others this is the Control key,
               with the new semantic of WXK_CONTROL, WXK_COMMAND is not needed anymore"  um, wtf, I'm running Linux and my machine has all these keys
*/
    }


int i;


switch (asciikeyboard)
{
 case 1:

                 if (keydir==2 && lastkeystroke==-1)
                    {
                        if (forcelisakey)   {
                                              if (forcelisakey & 8) send_cops_keycode(KEYCODE_COMMAND|KEY_DOWN);
                                              if (forcelisakey & 4) send_cops_keycode(KEYCODE_SHIFT  |KEY_DOWN);
                                              if (forcelisakey & 2) send_cops_keycode(KEYCODE_OPTION |KEY_DOWN);
                                              //----------------------------------------------------------------
                                                                    send_cops_keycode(lisakey        |KEY_DOWN);
                                                                    send_cops_keycode(lisakey        |KEY_UP  );
                                              //----------------------------------------------------------------
                                              if (forcelisakey & 2) send_cops_keycode(KEYCODE_OPTION |KEY_UP  );
                                              if (forcelisakey & 4) send_cops_keycode(KEYCODE_SHIFT  |KEY_UP  );
                                              if (forcelisakey & 8) send_cops_keycode(KEYCODE_COMMAND|KEY_UP  );
                                            }
                        else
                                            keystroke_cops(keycode);
                    }
                 lastkeystroke=-1;
                 return;


case 0 :         //     old raw unbuffered way - causes repetitions!
                 if (keydir==-1)                lisakey |=KEY_DOWN;
                 if (lisakey & 0x7f)            {//ALERT_LOG(0,"Sending rawkeycode %02x",lisakey);
                                                 send_cops_keycode(lisakey);}
                 lastkeystroke=-1;
                 return;

case -1 :
                 // raw keycodes are a lot more complicated because of timing, the Lisa will almost always
                 // repeat keys when they are unwanted.  This code attempts to buffer the codes and later
                 // on key-up events, let go of them.  It attempts to handle things such as holding shift,
                 // then typing A,B,C, and will send shift-down,a-down,a-up,shift-up,shift-down,b-down, etc...
                 //
                 // in some ways it's an attempt to "cook" raw keycodes, but it doesn't use ASCII translation.



                 // handle repeating keys by watching the host repeat
                 if (keydir==-1 && rawidx>0 && rawcodes[rawidx-1]==lisakey) keydir=1;


                 // key down
                 if (keydir==-1)                     // key down, don't send anything, just buffer.
                 {
                    // see if wxWidgets will send more than one keydown, if it does we have a way!
                    if (rawidx>=127) return;         // avoid overflow
                    if (rawidx) for ( i=0; i<rawidx; i++)
                                     if (lisakey==rawcodes[i])
                                        {
                                         //ALERT_LOG(0,"Suppressing duplicate keydown:%d",lisakey);
                                         return;
                                        }


                    rawcodes[rawidx]=lisakey;
                    //ALERT_LOG(0,"Queueing rawcodes[%d]=%02x ",rawidx,lisakey);
                    rawidx++;
                 }





                 // so this can handle shift-keydown-keyup-shift up, but won't handle any more than that.
                 // also handle shift-keydown-keyup-keydown-up-down-up-shift-up!
                 //
                 // also handle shiftdown,control-down,option-down-key-down-up,keydown-up, control-up, ...etc.
                 //
                 // so want to release all keys the first time any key is released, but don't flush the buffer until
                 // all keys are released!
                 //
                 // * also need to handle shift-down-adown-shift-up-a-up, etc.
                 //
                 // * remaining bug here is that this won't handle repeating keys.

                 if (keydir==1)                      // key released
                 {
                   int i,flag=0;
                   if (rawidx==0) {//ALERT_LOG(0,"Returning since rawdown=0");
                                   return;}           // nothing in the buffer

                   if (lisakey!=rawcodes[rawidx-1] &&
                       lisakey!=KEYCODE_COMMAND    &&
                       lisakey!=KEYCODE_OPTION     &&
                       lisakey!=KEYCODE_SHIFT        )
                      for (i=0; i<rawidx; i++)
                      {
                          if (//rawcodes[i]!=lisakey &&
                              rawcodes[i]!=0                &&
                              rawcodes[i]!=KEYCODE_COMMAND  &&
                              rawcodes[i]!=KEYCODE_OPTION   &&
                              rawcodes[i]!=KEYCODE_SHIFT       )
                              { //ALERT_LOG(0,"all non-meta keys have not yet been released.");
                                return;}
                      }


                   //--------------------------------------------------------------------------
                   // if there's nothing left in the buffer except for Apple,Option, and shift
                   // we don't have anything to send to the Lisa.  i.e. on final release of shift
                   // or option, or Apple key.

                   for (flag=0,i=0; i<rawidx && !flag; i++)
                   {
                       if (rawcodes[i]!=0                &&
                           rawcodes[i]!=KEYCODE_COMMAND  &&
                           rawcodes[i]!=KEYCODE_OPTION   &&
                           rawcodes[i]!=KEYCODE_SHIFT       ) flag=1;
                   }

                   // if the user let go of a modifier key, don't return anything, just remove it
                   if ( (lisakey==KEYCODE_COMMAND ||
                         lisakey==KEYCODE_OPTION  ||
                         lisakey==KEYCODE_SHIFT     ) )
                         {
                               for (i=0; i<rawidx && rawcodes[i]!=lisakey; i++);    //find modifier to delete.
                               if (i<rawidx-1) memmove(&rawcodes[i],&rawcodes[i+1],(128-i)*sizeof(int));

                               rawidx--;

                               // safety kludge - ensure that we let go of the modifiers so we don't leave the
                               // keyboard in a screwey state.
                               if (!rawidx) {send_cops_keycode(KEYCODE_COMMAND | KEY_UP);
                                             send_cops_keycode(KEYCODE_OPTION  | KEY_UP);
                                             send_cops_keycode(KEYCODE_SHIFT   | KEY_UP);
                                            }

                                 return;
                         }
                   //--------------------------------------------------------------------------
                   if (!flag) {//ALERT_LOG(0,"all left is shift/opt/cmd - returning");

                               // safety kludge - ensure that we let go of the modifiers so we don't leave the
                               // keyboard in a screwey state.
                               if (!rawidx) {send_cops_keycode(KEYCODE_COMMAND | KEY_UP);
                                             send_cops_keycode(KEYCODE_OPTION  | KEY_UP);
                                             send_cops_keycode(KEYCODE_SHIFT   | KEY_UP);
                                            }
                               return; }              // if all that's left are modifiers, return.


                   //ALERT_LOG(0,"\nlisakey Keyup:%d",lisakey);
                   for (i=0; i<rawidx; i++)         { //ALERT_LOG(0,"sending down[%d]=%d",i,rawcodes[i]);
                                                      if (rawcodes[i]!=0)
                                                       send_cops_keycode(rawcodes[i]|KEY_DOWN); }

                   for (i=rawidx-1; i>=0; i--)      { //ALERT_LOG(0,"sending up[%d]=%d",i,rawcodes[i]);
                                                      if (rawcodes[i]!=0)
                                                       send_cops_keycode(rawcodes[i]|KEY_UP);    }


                   //ALERT_LOG(0,"done\n");


                   // remove any keystrokes from the buffer we've just sent out other than the meta keys.
                   for (i=0; i<rawidx; i++)
                   {
                      if (KEYCODE_COMMAND!=rawcodes[i] &&
                          KEYCODE_OPTION !=rawcodes[i] &&
                          KEYCODE_SHIFT  !=rawcodes[i] &&
                                       0 !=rawcodes[i]    ) rawcodes[i]=0;
                   }

                   // if they're at the end, shrink the buffer - it's ok to leave holes, just don't want to.
                   while (rawidx>0 && rawcodes[rawidx-1]==0) rawidx--;

                   // safety kludge - ensure that we let go of the modifiers so we don't leave the
                   // keyboard in a screwey state.
                   if (!rawidx) {send_cops_keycode(KEYCODE_COMMAND | KEY_UP);
                                 send_cops_keycode(KEYCODE_OPTION  | KEY_UP);
                                 send_cops_keycode(KEYCODE_SHIFT   | KEY_UP);
                                }
                   return;



                 }
        return;

}


}


void LisaWin::OnKeyDown(wxKeyEvent& event)
{

        long keycode = event.GetKeyCode();

        //ALERT_LOG(0,"Key down:%x, shift:%x, alt:%x, control:%x", keycode,
        //          WXK_SHIFT,
        //          WXK_ALT,
        //          WXK_CONTROL);

        if (keycode==-1) keycode=WXK_ALT;


        if (
             keycode==WXK_LEFT ||                   // avoid cursor keys turning into ascii
             keycode==WXK_UP   ||
             keycode==WXK_RIGHT||
             keycode==WXK_DOWN ||

             keycode==WXK_NUMPAD_LEFT ||
             keycode==WXK_NUMPAD_UP   ||
             keycode==WXK_NUMPAD_RIGHT||
             keycode==WXK_NUMPAD_DOWN   )            {LogKeyEvent(_T("Key down"), event,-1);event.Skip(); return;}


    if (asciikeyboard==1) event.Skip(); else LogKeyEvent(_T("Key down"), event,-1);

}

void LisaWin::OnKeyUp(wxKeyEvent& event)
{
    //ALERT_LOG(0,"Key Up. Throttle:%f clk:%f",my_lisaframe->throttle,my_lisaframe->clockfactor);


        long keycode = event.GetKeyCode();

        if (
             keycode==WXK_LEFT ||                   // avoid cursor keys turning into ascii
             keycode==WXK_UP   ||
             keycode==WXK_RIGHT||
             keycode==WXK_DOWN ||

             keycode==WXK_NUMPAD_LEFT ||
             keycode==WXK_NUMPAD_UP   ||
             keycode==WXK_NUMPAD_RIGHT||
             keycode==WXK_NUMPAD_DOWN   )            {LogKeyEvent(_T("Key up"), event,1);event.Skip(); return;}

    videoramdirty++;

}

void LisaWin::OnChar(wxKeyEvent& event)
{
    wxCommandEvent foo;
    long keycode = event.GetKeyCode();
    if (keycode==-1) keycode=WXK_ALT;

    /* check for menu key presses since we steal them from the events*/
    if (keycode==WXK_F11) {
        DisplayMenu->Check(ID_VID_FULLSCREEN, (!my_lisaframe->IsFullScreen())); // controlled by checkmark, so invert it so it takes effect
        my_lisaframe->OnFullScreen(foo);
        return;
    }

    if (event.CmdDown()) {
      if (keycode==WXK_ADD || keycode==WXK_NUMPAD_ADD || 
          keycode==0x2b || keycode==0x3d )                       {my_lisaframe->OnZoomIn(foo);    return;}
      if (keycode==WXK_SUBTRACT || keycode==WXK_NUMPAD_SUBTRACT ||
          keycode==0x2d)                                         {my_lisaframe->OnZoomOut(foo);   return;}
      if (keycode=='.')                                          {my_lisaframe->OnPause(foo);     return;}
      if (keycode=='n' || keycode=='n'-'a'+1)                    {my_lisaframe->OnNewFLOPPY(foo); return;}
      if (keycode=='o' || keycode=='o'-'a'+1)                    {my_lisaframe->OnFLOPPY(foo);    return;}
    }


    if (
        keycode==WXK_SHIFT ||                    // avoid inserting extra ascii 1,2,4 on shift,alt,control down.
        keycode==WXK_ALT   ||
        keycode==WXK_CONTROL  ) return;

    wxString x; x.Printf(_T("keychar %c %08lx"),(char)keycode,keycode );
    my_lisaframe->SetStatusBarText(x);
    if (asciikeyboard==1) {
        LogKeyEvent(_T("Char"), event, 2);
        event.Skip(false); //else  LogKeyEvent(_T("Char"), event, 2);
      }

    if (!my_lisaframe->running && (keycode=='q' || keycode=='Q' || keycode==0x11 ) )   {   my_lisaframe->OnQuit(foo); }    
}


#define M8(x) (x>255 ? (255): (x))

void LisaWin::ContrastChange(void)
{
      if ((contrast>>1)==lastcontrast)      return;

      lastcontrast=contrast>>1;

      int mc=0x88+( 0x7f^(contrast>>1)  );                // my contrast

      if (contrast<0xe0) // contrast >e0 is pretty much black.
      {
        int step=(mc-brightness)/10;
        /* white */ bright[0]=MIN(240,mc);                                          
        /* dark0 */ bright[1]=MIN(240,brightness+step*6);                        
        /* dark1 */ bright[2]=bright[3]=MIN(240,brightness+step*5);               
        /* black */ bright[4]=bright[5]=bright[6]=bright[7]=MIN(240,brightness);    

        /* AAGray*/ bright[8] = bright[9] = bright[10]= bright[11]= bright[12]=
                    bright[13]= bright[14] =bright[15]=                         (mc+brightness)>>1;

      }
      else                                   // contrast is all black
      {
          bright[0]=0;                  
          bright[1]=0;                  
          bright[2]=0;                  
          bright[3]=0;                  
          bright[4]=0;                  
          bright[5]=0;                  
          bright[6]=0;                  
          bright[7]=0;                  
          bright[8] = bright[9] = bright[10]= bright[11]= bright[12]=bright[13] = bright[14] =bright[15]=0;

      }

      refresh_bytemap=0;        // we just refreshed the contrast table
      dirtyscreen=1;            // however, the screen bitmap is now dirty
      videoramdirty=32768;

  #ifndef __WXOSX__             // 2007.03.14 disabled for os x since it slows down shutdown a whole lot
      force_refresh();          // win32 still has contrast-trails. :-( 20070216
  #endif

  //ALERT_LOG(0,"set brightness %d, contrast=%d",brightness,contrast); //contrast: 0xff=black 0x80=visible 0x00=all white

}

//----------------------------------------------------------------------------------
// why the fuck is this needed to display anything? any time I use a pixel iterator
// this is required. testing before and after the size and depth are identical, so
// why is this needed? what does the rawbmp pixel iterator do to fuck things up and
// in what ways? Is this needed for other than GTK? macos? windows? Note that the DC
// does not have to be deleted for this to work, this is related more to the bitmap.
// the resulting depth is the same between before/after. Something about the direct
// rawbibtmap access via pixmap iterator fucks up the bitmap, but converting it to
// an image and back fixes it. Which probably means I can reuse this on any renderer
// where rawbitmap is broken and use this to fix it instead of the slower method, but
// not sure that doing this is faster, most likely it's much slower.
// ::TODO:: test this, and profile it.
//----------------------------------------------------------------------------------
#if defined(__WXGTK__)
#define unfuck_wxbitmap_dc(dc,bitmap,sx,sy) {                                 \
        wxImage *display_image;                                               \
        display_image= new wxImage(  bitmap->ConvertToImage());               \
        delete bitmap;                                                        \
        bitmap=new wxBitmap(*display_image);                                  \
        delete display_image;                                                 \
        if ( dc==NULL)  dc=new class wxMemoryDC;                              \
        dc->SelectObjectAsSource(*bitmap);                                    \
        dc->SetUserScale((sx),(sy));                                          \
        dc->SetMapMode(wxMM_TEXT);                                            \
}
#else
#define unfuck_wxbitmap_dc(dc,bitmap,sx,sy) {;}
#endif
//-----------------------------------------------------------------------------


// lens that doubles 8 bit pixels to 16 horizontally. moved to hq3x
//uint16 double_x[256]=
//{   0x0000, 0x0003, 0x000c, 0x000f, 0x0030, 0x0033, 0x003c, 0x003f, 0x00c0, 0x00c3, 0x00cc, 0x00cf, 0x00f0, 0x00f3, 0x00fc, 0x00ff, 
//    0x0300, 0x0303, 0x030c, 0x030f, 0x0330, 0x0333, 0x033c, 0x033f, 0x03c0, 0x03c3, 0x03cc, 0x03cf, 0x03f0, 0x03f3, 0x03fc, 0x03ff, 
//    0x0c00, 0x0c03, 0x0c0c, 0x0c0f, 0x0c30, 0x0c33, 0x0c3c, 0x0c3f, 0x0cc0, 0x0cc3, 0x0ccc, 0x0ccf, 0x0cf0, 0x0cf3, 0x0cfc, 0x0cff, 
//    0x0f00, 0x0f03, 0x0f0c, 0x0f0f, 0x0f30, 0x0f33, 0x0f3c, 0x0f3f, 0x0fc0, 0x0fc3, 0x0fcc, 0x0fcf, 0x0ff0, 0x0ff3, 0x0ffc, 0x0fff, 
//    0x3000, 0x3003, 0x300c, 0x300f, 0x3030, 0x3033, 0x303c, 0x303f, 0x30c0, 0x30c3, 0x30cc, 0x30cf, 0x30f0, 0x30f3, 0x30fc, 0x30ff, 
//    0x3300, 0x3303, 0x330c, 0x330f, 0x3330, 0x3333, 0x333c, 0x333f, 0x33c0, 0x33c3, 0x33cc, 0x33cf, 0x33f0, 0x33f3, 0x33fc, 0x33ff, 
//    0x3c00, 0x3c03, 0x3c0c, 0x3c0f, 0x3c30, 0x3c33, 0x3c3c, 0x3c3f, 0x3cc0, 0x3cc3, 0x3ccc, 0x3ccf, 0x3cf0, 0x3cf3, 0x3cfc, 0x3cff, 
//    0x3f00, 0x3f03, 0x3f0c, 0x3f0f, 0x3f30, 0x3f33, 0x3f3c, 0x3f3f, 0x3fc0, 0x3fc3, 0x3fcc, 0x3fcf, 0x3ff0, 0x3ff3, 0x3ffc, 0x3fff, 
//    0xc000, 0xc003, 0xc00c, 0xc00f, 0xc030, 0xc033, 0xc03c, 0xc03f, 0xc0c0, 0xc0c3, 0xc0cc, 0xc0cf, 0xc0f0, 0xc0f3, 0xc0fc, 0xc0ff, 
//    0xc300, 0xc303, 0xc30c, 0xc30f, 0xc330, 0xc333, 0xc33c, 0xc33f, 0xc3c0, 0xc3c3, 0xc3cc, 0xc3cf, 0xc3f0, 0xc3f3, 0xc3fc, 0xc3ff, 
//    0xcc00, 0xcc03, 0xcc0c, 0xcc0f, 0xcc30, 0xcc33, 0xcc3c, 0xcc3f, 0xccc0, 0xccc3, 0xcccc, 0xcccf, 0xccf0, 0xccf3, 0xccfc, 0xccff, 
//    0xcf00, 0xcf03, 0xcf0c, 0xcf0f, 0xcf30, 0xcf33, 0xcf3c, 0xcf3f, 0xcfc0, 0xcfc3, 0xcfcc, 0xcfcf, 0xcff0, 0xcff3, 0xcffc, 0xcfff, 
//    0xf000, 0xf003, 0xf00c, 0xf00f, 0xf030, 0xf033, 0xf03c, 0xf03f, 0xf0c0, 0xf0c3, 0xf0cc, 0xf0cf, 0xf0f0, 0xf0f3, 0xf0fc, 0xf0ff, 
//    0xf300, 0xf303, 0xf30c, 0xf30f, 0xf330, 0xf333, 0xf33c, 0xf33f, 0xf3c0, 0xf3c3, 0xf3cc, 0xf3cf, 0xf3f0, 0xf3f3, 0xf3fc, 0xf3ff, 
//    0xfc00, 0xfc03, 0xfc0c, 0xfc0f, 0xfc30, 0xfc33, 0xfc3c, 0xfc3f, 0xfcc0, 0xfcc3, 0xfccc, 0xfccf, 0xfcf0, 0xfcf3, 0xfcfc, 0xfcff, 
//    0xff00, 0xff03, 0xff0c, 0xff0f, 0xff30, 0xff33, 0xff3c, 0xff3f, 0xffc0, 0xffc3, 0xffcc, 0xffcf, 0xfff0, 0xfff3, 0xfffc, 0xffff  };

// Repainters //////////////////////////////////////////////////////////////////////////////////////////




///////////////// SETRGB 16 bit AntiAliased MACRO //////////////////////////////////////////////////////   
//                                                                                                    //
//  macro to fill in r,g,b values, does 16 pixels at a time, must be a macro because of the Z param   //
//  and we want to get as much speed out of it as possible.                                           //
//                                                                                                    //
//  This would have been a 32 pixel macro if it were possible, however, since a single row is on the  //
//  Lisa's display is either 90 bytes or 76 bytes, we can only evenly divide by 2 bytes (16 bits.)    //
//                                                                                                    //
//                                                                                                    //
//  x,y are x,y coordinates on the bitmap.   They map to the Lisa's display, any other translation    //
//  must be handled in Z, or via other variables.   x gets incremented for each pixel handled.        //
//  x,y must be simple variables, and should not be expressions!                                      //
//                                                                                                    //
//  Z is a chunk of code to set r,g,b provided by the caller.  Z gets executed 16 times, once per     //
//  pixel.  This is needed because we might be using SetRGB on images or rawbitmap accesss, or some   //
//  other method in the future.   Z should be protected by enclosing it in curly braces when passing. //
//  Z may use the uint8 d to actually set the darkness level for a pixel.                             //
//                                                                                                    //
//  Y is identical to Z except that it's code to call when there's no update.  i.e. ++p               //
//                                                                                                    //
//                                                                                                    //
//  The following variables should be declared before calling this macro:                             //
//                                                                                                    //
//      int updated=0;              // number of times we've made updates.                            //
//                                  // can be used as a threshhold to decide how to redraw the image. //
//      uint32 a1,a2,a3,a4,a5,xx;   // address above, below, for this value, horziontal byte offset   //
//      uint16 vup,vdn,val,vvu,vvd; // value above, below, this words read from addr @ a1,a2,a3       //
//      uint16 high,medium,low;     // used for antialiasing. how many neighboring bits               //
//      uint8 d;                    // darkness level to pass to Z                                    //
//                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SETRGB16_AAG(x,y,Z,Y) {                                                                        \
                                                                                                       \
     xx=(x)>>3;                                                /*   turn x coord into byte offset */   \
                                                                                                       \
     a1=(yoffset[screen_to_mouse[ y>0?(y-1):0 ]]+xx) &32767;   /*   get the addresses for above   */   \
     a2=(yoffset[screen_to_mouse[ y+1         ]]+xx) &32767;   /*   below and                     */   \
     a3=(yoffset[screen_to_mouse[ y           ]]+xx) &32767;   /*   this value we're processing   */   \
                                                                                                       \
     vup=(lisaram[videolatchaddress+a1]<<8 )|lisaram[videolatchaddress+a1+1];  /*   word above    */   \
     vdn=(lisaram[videolatchaddress+a2]<<8 )|lisaram[videolatchaddress+a2+1];  /*   word below    */   \
     val=(lisaram[videolatchaddress+a3]<<8 )|lisaram[videolatchaddress+a3+1];  /*   this word     */   \
                                                                                                       \
                                                                                                       \
     if (videoramdirty>DIRECT_BLITS_THRESHHOLD ||          /*  If full update requested or        */   \
         ((dirtyvidram[a1]<<8)|dirtyvidram[a1+1])!=vup ||  /*  if the video ram is dirty above or */   \
         ((dirtyvidram[a2]<<8)|dirtyvidram[a2+1])!=vdn ||  /*  below or on this value.            */   \
         ((dirtyvidram[a3]<<8)|dirtyvidram[a3+1])!=val  )                                              \
      {                                                                                                \
       updated++;                                          /*  Keep track of update count         */   \
                                                                                                       \
       dirty_x_min=MIN(x,dirty_x_min);       dirty_x_max=MAX(x+16,dirty_x_max);                        \
       dirty_y_min=MIN(y,dirty_y_min);       dirty_y_max=MAX(y,dirty_y_max);                           \
                                                                                                       \
       a4=(a3-90)>0     ? (a3-90):a3;                                    /*   unscaled addr above */   \
       a5=(a3+90)<32767 ? (a3+90):a3;                                    /*   unscaled addr below */   \
       vvu=(lisaram[videolatchaddress+a4]<<8 )|lisaram[videolatchaddress+a4+1]; /* unscaled up    */   \
       vvd=(lisaram[videolatchaddress+a5]<<8 )|lisaram[videolatchaddress+a5+1]; /* unscaled below */   \
       getgraymap(vvu,val,vvd,replacegray);              /*  Get the gray replacement map         */   \
                                                                                                       \
       high   = vup      & val     & vdn;                /*  above, middle, and below are black   */   \
       medium = (vup&val)|(val&vdn)|(vdn&vup);           /*  two out of 3 are black               */   \
       low    = vup      | val     | vdn;                /*  lowest - single pixel is black       */   \
                                                                                                       \
      d=bright[ ((BIT15 & val ) ? 7:0)  | replacegray[ 0]]; Z; x++;                                    \
      d=bright[ ((BIT14 & val ) ? 7:0)  | replacegray[ 1]]; Z; x++;                                    \
      d=bright[ ((BIT13 & val ) ? 7:0)  | replacegray[ 2]]; Z; x++;                                    \
      d=bright[ ((BIT12 & val ) ? 7:0)  | replacegray[ 3]]; Z; x++;                                    \
      d=bright[ ((BIT11 & val ) ? 7:0)  | replacegray[ 4]]; Z; x++;                                    \
      d=bright[ ((BIT10 & val ) ? 7:0)  | replacegray[ 5]]; Z; x++;                                    \
      d=bright[ ((BIT9  & val ) ? 7:0)  | replacegray[ 6]]; Z; x++;                                    \
      d=bright[ ((BIT8  & val ) ? 7:0)  | replacegray[ 7]]; Z; x++;                                    \
      d=bright[ ((BIT7  & val ) ? 7:0)  | replacegray[ 8]]; Z; x++;                                    \
      d=bright[ ((BIT6  & val ) ? 7:0)  | replacegray[ 9]]; Z; x++;                                    \
      d=bright[ ((BIT5  & val ) ? 7:0)  | replacegray[10]]; Z; x++;                                    \
      d=bright[ ((BIT4  & val ) ? 7:0)  | replacegray[11]]; Z; x++;                                    \
      d=bright[ ((BIT3  & val ) ? 7:0)  | replacegray[12]]; Z; x++;                                    \
      d=bright[ ((BIT2  & val ) ? 7:0)  | replacegray[13]]; Z; x++;                                    \
      d=bright[ ((BIT1  & val ) ? 7:0)  | replacegray[14]]; Z; x++;                                    \
      d=bright[ ((BIT0  & val ) ? 7:0)  | replacegray[15]]; Z; x++;                                    \
                                                                                                       \
                                                                                                       \
      }                                                                                                \
     else                                                                                              \
      {                                                                      /* No updated needed */   \
        Y; Y; Y; Y;                                                          /* so skip over 16X  */   \
        Y; Y; Y; Y;                                                                                    \
        Y; Y; Y; Y;                                                                                    \
        Y; Y; Y; Y; x+=16;                                                                             \
      }                                                                                                \
}                                                                                                     //
////////////////// GETRGB MACRO ENDS ///////////////////////////////////////////////////////////////////

/***************************************************************************************************\
    graymap[] are set to 8 which is the magical OR value into the color contrast table that forces
    gray replacement.  See ContrastChange() for details.  The graymap is stored as a 3 dimentional
    array, but we can make use of shifts and OR's to avoid multiplication that [][][] would be
    involved in dereferencing.  It's possible some compilers will optimize it properly, but why
    take a chance?  Access is equivalent to   color | graymap[(vup<<4) | (val<<2) | vdn];

    Since we work 2 bits at a time, we need to replace both bits with a gray together.  This is why 
    we have retval[1]=retval[0] and so on.   It would be possible to modify the code to do something
    like retval & 14 to avoid the copy, but it wouldn't be any faster and would make the code more
    complicated.

    This function is only called once per every 16 horizontal pixels processed for speed which is
    why it needs to store its results in an array.

\***************************************************************************************************/

static inline void getgraymap(uint16 up, uint16 val, uint16 dn,  uint8 *retval)
{
    static uint8 graymap[4*4*4]={ 0,0,0,0,0,0,8,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,8,0,8,0,8,0,0,    
                                  0,0,0,0,8,0,8,8,0,0,0,0,0,0,8,0,0,0,0,0,0,0,8,8,0,8,0,8,0,0,0,0, }; 

// static int retval[16];
  retval[ 0]=retval[ 1]=graymap[(((BIT15+BIT14) & up)>>10 )|(((BIT15+BIT14) & val)>>12 ) | (((BIT15+BIT14) & dn)>>14 )]; 
  retval[ 2]=retval[ 3]=graymap[(((BIT13+BIT12) & up)>> 8 )|(((BIT13+BIT12) & val)>>10 ) | (((BIT13+BIT12) & dn)>>12 )]; 
  retval[ 4]=retval[ 5]=graymap[(((BIT11+BIT10) & up)>> 6 )|(((BIT11+BIT10) & val)>> 8 ) | (((BIT11+BIT10) & dn)>>10 )]; 
  retval[ 6]=retval[ 7]=graymap[(((BIT9 +BIT8 ) & up)>> 4 )|(((BIT9 +BIT8 ) & val)>> 6 ) | (((BIT9 +BIT8 ) & dn)>> 8 )]; 
  retval[ 8]=retval[ 9]=graymap[(((BIT7 +BIT6 ) & up)>> 2 )|(((BIT7 +BIT6 ) & val)>> 4 ) | (((BIT7 +BIT6 ) & dn)>> 6 )]; 
  retval[10]=retval[11]=graymap[(((BIT5 +BIT4 ) & up)     )|(((BIT5 +BIT4 ) & val)>> 2 ) | (((BIT5 +BIT4 ) & dn)>> 4 )]; 
  retval[12]=retval[13]=graymap[(((BIT3 +BIT2 ) & up)<< 2 )|(((BIT3 +BIT2 ) & val)     ) | (((BIT3 +BIT2 ) & dn)>> 2 )]; 
  retval[14]=retval[15]=graymap[(((BIT1 +BIT0 ) & up)<< 4 )|(((BIT1 +BIT0 ) & val)<< 2 ) | (((BIT1 +BIT0 ) & dn)     )]; 
}


void LisaWin::RePaint_HQ35X(void)
{
    uint32 brightness;
    contrastchange();
    brightness=bright[7]; // contrast ff=black, 80=visible, 00=all white.
    if (brightness==0) brightness=0xe0;

      if (!my_lisahq3xbitmap || ( my_lisahq3xbitmap->GetWidth()<HQX35X+4 || my_lisahq3xbitmap->GetHeight()<HQX35X+4) ) {
          if (!my_lisahq3xbitmap) delete my_lisahq3xbitmap;
          if (!my_memhq3xDC)      delete my_memhq3xDC;
          my_memhq3xDC     =new class wxMemoryDC;
          my_lisahq3xbitmap=new class wxBitmap(HQX35X+4, HQX35Y+4,DEPTH);
          my_memhq3xDC->SelectObjectAsSource(*my_lisahq3xbitmap);
          my_memhq3xDC->SetUserScale(1.0,1.0);
          my_memhq3xDC->SetMapMode(wxMM_TEXT);
    }
    hq3x_32_rb( 90, my_lisahq3xbitmap, HQX35X,HQX35Y,brightness ); //brightness
//    ALERT_LOG(0,"before: my_lisahq3xbitmap %d,%d depth:%d size:%d is ok:%d",
//            my_lisahq3xbitmap->GetWidth(),my_lisahq3xbitmap->GetHeight(),my_lisahq3xbitmap->GetDepth(),my_lisahq3xbitmap->IsOk());

    unfuck_wxbitmap_dc(my_memhq3xDC, my_lisahq3xbitmap, 1.0, 1.0);

//    ALERT_LOG(0,"after: my_lisahq3xbitmap %d,%d depth:%d size:%d is ok:%d",
//           my_lisahq3xbitmap->GetWidth(),my_lisahq3xbitmap->GetHeight(),my_lisahq3xbitmap->GetDepth(),my_lisahq3xbitmap->IsOk());
//    ALERT_LOG(0,"brightness: %08x",brightness);
//    ALERT_LOG(0,"e_dirty_min-max: %d,%d - %d,%d",e_dirty_x_min,e_dirty_y_min, e_dirty_x_max,e_dirty_y_max);

    prep_dirty;

    if (skins_on)
    {

        my_skinDC->StretchBlit(
                               _H(skin.screen_origin_x              ), _H(skin.screen_origin_y              ),         // target x,y
                               _H(720                               ), _H(364                    )*HQ3XYSCALE,         // target size w,h
                                my_memhq3xDC,                                                                          // source DC
                                0,0,                                                                                   // source x,y
                                HQX35X,HQX35Y,                                                                           // source size
                                wxCOPY, false);                                                                        // mode, usemask?

        /*
        my_skinDC->StretchBlit(
                               _H(skin.screen_origin_x + e_dirty_x_min), _H(skin.screen_origin_y + e_dirty_y_min),     // target x,y
                               _H(e_dirty_x_max-e_dirty_x_min+1),_H(e_dirty_y_max-e_dirty_y_min+1)*HQ3XYSCALE,         // target size w,h
                                my_memhq3xDC,                                                                          // source DC
                                e_dirty_x_min*3,e_dirty_y_min*3,                                                       // source x,y
                                (e_dirty_x_max-e_dirty_x_min+1)*3,(e_dirty_y_max-e_dirty_y_min+1)*3,                   // source size
                                wxCOPY, false);                                                                        // mode, usemask?
        */
    }
    //else
    //{
      // handled directly in OnPaint_skinless instead.
      //  my_memDC->StretchBlit(
      //                         0,0,                                                                                    // target x,y
      //                         _H(720),_H(364)*HQ3XYSCALE,                                                             // target size w,h
      //                          my_memhq3xDC,                                                                          // source DC
      //                          0,0,                                                                                   // source x,y
      //                          (720)*2,(364)*3,                                                                       // source size
      //                          wxCOPY, false);                                                                        // mode, usemask?
        /*
        my_memDC->StretchBlit(
                               _H(                         e_dirty_x_min), _H(                      e_dirty_y_min),    // target x,y
                               _H(e_dirty_x_max-e_dirty_x_min+1),_H(e_dirty_y_max-e_dirty_y_min+1)*HQ3XYSCALE,         // target size w,h
                                my_memhq3xDC,                                                                          // source DC
                                e_dirty_x_min*3,e_dirty_y_min*3,                                                       // source x,y
                                (e_dirty_x_max-e_dirty_x_min+1)*3,(e_dirty_y_max-e_dirty_y_min+1)*3,                   // source size
                                wxCOPY, false);                                                                        // mode, usemask?
        */
    //}

  reset_dirty;

  repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;
}


void LisaWin::RePaint_AAGray(void)
{
    // vars for SETRGB16  
    int updated=0;    
    uint32 a1,a2,a3,a4,a5,xx;
    uint16 vup,vdn,val,vvu,vvd;
    uint16 high=0,medium=0,low=0;
    uint8  d;

    uint8 replacegray[16]; // ignore dumb compiler warning here!
    dirty_x_min=720; dirty_x_max=-1; dirty_y_min=364*3; dirty_y_max=-1;

    if (high|medium|low) high=1; // dumb way to suppress "set but not used" - should get optimized out

#ifdef USE_RAW_BITMAP_ACCESS

    int ox,oy;             // private to this - not member vars
    int width, height, depth;
   
    if  (skins_on)
    {
        if (!my_skin) { replacegray[0]=0;  return;}// useless statement to suppress dumb "unused" compiler warning 

        depth = my_skin->GetDepth();    width = my_skin->GetWidth();    height= my_skin->GetHeight();
        ox=skin.screen_origin_x;        oy=skin.screen_origin_y;
    }
    else
    {
        if (!my_lisabitmap) return;

        depth = my_lisabitmap->GetDepth();    width = my_lisabitmap->GetWidth();    height= my_lisabitmap->GetHeight();
        ox=0; oy=0;
    }

    typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
    PixelData data(skins_on ? *my_skin:*my_lisabitmap);
    if (!data) return;
    PixelData::Iterator p(data);
    p.Reset(data);
    p.MoveTo(data,ox,oy); 
    //ALERT_LOG(0,"RepaintAAG f raw bitmap, origin:%d,%d",ox,oy);

    for ( int y = 0; y < o_effective_lisa_vid_size_y; ++y )   // effective_lisa_vid_size_y
    {
        PixelData::Iterator rowStart = p;              // save the x,y coordinates at the start of the line

        for ( int x = 0; x < o_effective_lisa_vid_size_x; )
            { SETRGB16_AAG(x,y,  {p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p;} , {++p;} );}  

        p.Red()=0; p.Green()=0; p.Blue()=0;


        p = rowStart; p.OffsetY(data, 1);              // restore the x,y coords from start of line, then increment y to do y++;
    }

#else

  // Since we're working the slower way - with images, we need to rebuild the bitmap from the image.
  // to do this, we discard the old bitmap and recreate it from the display_image.  This is of course
  // slower, which is why we recommend the use of USE_RAW_BITMAP_ACCESS, but USE_RAW_BITMAP_ACCESS
  // might not work everywhere, so we give the option.
  if (skins_on)
  {
    if (!my_skin || !my_lisabitmap) return; 

  //  int depth = my_skin->GetDepth();
  //  int width = my_skin->GetWidth();
  //  int height= my_skin->GetHeight();

    if  (!display_image)   
          display_image= new wxImage(  my_lisabitmap->ConvertToImage());

    for ( int y=0; y < o_effective_lisa_vid_size_y; y++ )
        {
            for ( int x=0; x < o_effective_lisa_vid_size_x;)
                { SETRGB16_AAG(x,y,{display_image->SetRGB(x,y,d,d,d+EXTRABLUE); },{;}); }
        }

    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);

    prep_dirty;

    if  (updated) {
           my_skinDC->StretchBlit( _H(skin.screen_origin_x + e_dirty_x_min), _H(skin.screen_origin_y + e_dirty_y_min),   // target x,y
                                   _H(e_dirty_x_max-e_dirty_x_min+1),_H(e_dirty_y_max-e_dirty_y_min+1),     // size w,h
                                    my_memDC, 0,0, (e_dirty_x_max-e_dirty_x_min+1),(e_dirty_y_max-e_dirty_y_min+1),
                                    wxCOPY, false);
        }

    // we don't delete display_image since we can reuse it the next time around and save time
  }
  else  // skins are off
  {
    if (!display_image)   display_image= new wxImage(my_lisabitmap->ConvertToImage());

    for ( int yo = 0 , yi=0; yi < o_effective_lisa_vid_size_y; yo++,yi++ )
        {
            // note neither xi, nor xo are incremented as part of the for-loop header, this is because
            // the SETRGB16_AA macro expands to 16 iterations, and it increments xi on each iteration.
            // however, it doesn't do anything with xo, so xo++ is passed as a parameter.
            for ( int xo = 0, xi=0; xi < o_effective_lisa_vid_size_x;)
                { SETRGB16_AAG(xi,yi,{display_image->SetRGB(xo,yo,d,d,d+EXTRABLUE); xo++;},{xo++;});   }
        }

    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);
  }
#endif
/////////////////////////////////////////////////////////////////////////


  if (updated)
  {
      memcpy(dirtyvidram,&lisaram[videolatchaddress],32768);
      repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;
  }

}



///////////////// SETRGB 16 bit AntiAliased MACRO //////////////////////////////////////////////////////   
//                                                                                                    //
//  macro to fill in r,g,b values, does 16 pixels at a time, must be a macro because of the Z param   //
//  and we want to get as much speed out of it as possible.                                           //
//                                                                                                    //
//  This would have been a 32 pixel macro if it were possible, however, since a single row is on the  //
//  Lisa's display is either 90 bytes or 76 bytes, we can only evenly divide by 2 bytes (16 bits.)    //
//                                                                                                    //
//                                                                                                    //
//  x,y are x,y coordinates on the bitmap.   They map to the Lisa's display, any other translation    //
//  must be handled in Z, or via other variables.   x gets incremented for each pixel handled.        //
//  x,y must be simple variables, and should not be expressions!                                      //
//                                                                                                    //
//  Z is a chunk of code to set r,g,b provided by the caller.  Z gets executed 16 times, once per     //
//  pixel.  This is needed because we might be using SetRGB on images or rawbitmap accesss, or some   //
//  other method in the future.   Z should be protected by enclosing it in curly braces when passing. //
//  Z may use the uint8 d to actually set the darkness level for a pixel.                             //
//                                                                                                    //
//  Y is identical to Z except that it's code to call when there's no update.  i.e. ++p               //
//                                                                                                    //
//                                                                                                    //
//  The following variables should be declared before calling this macro:                             //
//                                                                                                    //
//         int updated=0;            // number of times we've made updates.                            //
//                                  // can be used as a threshhold to decide how to redraw the image. //
//         uint32 a1,a2,a3,xx;      // address above, below, for this value, horziontal byte offset   //
//         uint16 vup,vdn,val;      // value above, below, this words read from addr @ a1,a2,a3       //
//         uint16 high,medium,low;  // used for antialiasing. how many neighboring bits               //
//         uint8 d;                 // darkness level to pass to Z                                    //
//                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////


#define SETRGB16_AA(x,y,Z,Y) {                                                                         \
                                                                                                       \
     xx=(x)>>3;                                                /*   turn x coord into byte offset */   \
                                                                                                       \
     a3=(yoffset[screen_to_mouse[ y           ]]+xx) &32767;   /*   this value we're processing   */   \
                                                                                                       \
     val=(lisaram[videolatchaddress+a3]<<8 )|lisaram[videolatchaddress+a3+1];  /*   this word     */   \
                                                                                                       \
     if (videoramdirty>DIRECT_BLITS_THRESHHOLD ||        /*  If full update requested or dirty    */   \
         ((dirtyvidram[a3]<<8)|dirtyvidram[a3+1])!=val  )                                              \
      {                                                                                                \
       updated++;                                          /*  Keep track of update count         */   \
                                                                                                       \
       dirty_x_min=MIN(x,dirty_x_min);       dirty_x_max=MAX(x+16,dirty_x_max);                        \
       dirty_y_min=MIN(y,dirty_y_min);       dirty_y_max=MAX(y,dirty_y_max);                           \
                                                                                                       \
       d=bright[ ((BIT15 & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT14 & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT13 & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT12 & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT11 & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT10 & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT9  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT8  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT7  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT6  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT5  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT4  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT3  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT2  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT1  & val ) ? 7:0)  ]; Z; x++;                                                    \
       d=bright[ ((BIT0  & val ) ? 7:0)  ]; Z; x++;                                                    \
      }                                                                                                \
     else                                                                                              \
      {                                                                                                \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
      }                                                                                                \
}                                                                                                     //
////////////////// GETRGB MACRO ENDS ///////////////////////////////////////////////////////////////////


void LisaWin::RePaint_AntiAliased(void)
{
    // vars for SETRGB16  
    int updated=0;    
    uint32 a3,xx;
    uint16 val;
    uint8  d;

    dirty_x_min=720; dirty_x_max=-1; dirty_y_min=364*3; dirty_y_max=-1;

#ifdef USE_RAW_BITMAP_ACCESS

    int depth;  
    int width;  
    int height; 
    int ox,oy; // private to this - not member vars

    if  (skins_on)
        {
            if (!my_skin) {ALERT_LOG(0,"Null my_skin"); return;}
            depth = my_skin->GetDepth();    width = my_skin->GetWidth();    height= my_skin->GetHeight();
            ox=skin.screen_origin_x;    oy=skin.screen_origin_y;
        }
    else
        {
            if (!my_lisabitmap->IsOk()) {ALERT_LOG(0,"my_lisabitmap is not ok!");}
            if (!my_lisabitmap)         {ALERT_LOG(0,"Null mylisa_bitmap!"); return;}

            depth = my_lisabitmap->GetDepth();    width = my_lisabitmap->GetWidth();    height= my_lisabitmap->GetHeight();
            ox=0; oy=0;
        }

    typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
    PixelData data( *( skins_on ? my_skin : my_lisabitmap) );
    if (!data) {ALERT_LOG(0,"No data."); return;}

    PixelData::Iterator p(data);
    p.Reset(data);
    p.MoveTo(data,ox,oy);                            

    for ( int y =0; y<o_effective_lisa_vid_size_y; y++ )   // effective_lisa_vid_size_y
        {
          PixelData::Iterator rowStart = p;                 // save the x,y coordinates at the start of the line
          for ( int x = 0; x < o_effective_lisa_vid_size_x; )
              { SETRGB16_AA(x,y,  {p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p;} , {++p;} );} 
          p = rowStart; p.OffsetY(data, 1);                 // restore the x,y coords from start of line, then increment y to do y++;
        }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    
#else

  // Since we're working the slower way - with images, we need to rebuild the bitmap from the image.
  // to do this, we discard the old bitmap and recreate it from the display_image.  This is of course
  // slower, which is why we recommend the use of USE_RAW_BITMAP_ACCESS, but USE_RAW_BITMAP_ACCESS
  // might not work everywhere, so we give the option.
  if (skins_on)
  {
    if (!my_skin || !my_lisabitmap) return; 

    if  (!display_image)   
          display_image= new wxImage(  my_lisabitmap->ConvertToImage());


    for ( int y=0; y < o_effective_lisa_vid_size_y; y++ )
        {

            for ( int x=0; x < o_effective_lisa_vid_size_x;)
                { 
                    SETRGB16_AA(x,y,{display_image->SetRGB(x,y,d,d,d+EXTRABLUE); },{;}); 
                }
        }
    // delete the old bitmap, then create a new one from the wxImage, and use it.
    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);
    if (!my_memDC->IsOk()) ALERT_LOG(0,"my_memDC is not ok.");
    if (!my_lisabitmap->IsOk()) ALERT_LOG(0,"my_lisabitmap is not ok.");

    e_dirty_x_min=dirty_x_min; // need to do these here so we can update just the rectangle we need.
    e_dirty_x_max=dirty_x_max; // it will be repeated again below, but so what, it's only 4 assignments
    e_dirty_y_min=dirty_y_min; //screen_y_map[dirty_y_min];         // and two lookups.
    e_dirty_y_max=dirty_y_max; //screen_y_map[dirty_y_max];

    if  (updated)
        {       
           my_skinDC->StretchBlit(_H(skin.screen_origin_x + e_dirty_x_min), _H(skin.screen_origin_y + e_dirty_y_min),   // target x,y
                                  _H(e_dirty_x_max-e_dirty_x_min+1),_H(e_dirty_y_max-e_dirty_y_min+1),     // size w,h
                                    my_memDC, 0,0, (e_dirty_x_max-e_dirty_x_min+1),(e_dirty_y_max-e_dirty_y_min+1),
                                    wxCOPY, false);
        }

    // we don't delete display_image since we can reuse it the next time around and save time
  }
  else  // skins are off
  {

    if (!display_image)   display_image= new wxImage(my_lisabitmap->ConvertToImage());
    // ::TODO:: FIXME: CODEDEBT * this might need rework for hidpi
    for ( int yo = 0 , yi=0; yi < o_effective_lisa_vid_size_y; yo++,yi++ )
        {
            // note neither xi, nor xo are incremented as part of the for-loop header, this is because
            // the SETRGB16_AA macro expands to 16 iterations, and it increments xi on each iteration.
            // however, it doesn't do anything with xo, so xo++ is passed as a parameter.
            for ( int xo = 0, xi=0; xi < o_effective_lisa_vid_size_x;)
                { SETRGB16_AA(xi,yi,{display_image->SetRGB(xo,yo,d,d,d+EXTRABLUE); xo++;},{xo++;});   }
        }

    // and this is why this is slower since we need to rebuild the bitmap from the wxImage each time.
    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);
  }
#endif
/////////////////////////////////////////////////////////////////////////
    
  e_dirty_x_min=dirty_x_min; // need to do these here so we can update just the rectangle we need.
  e_dirty_x_max=dirty_x_max; // it will be repeated again below, but so what, it's only 4 assignments
  e_dirty_y_min=dirty_y_min; //screen_y_map[dirty_y_min];         // and two lookups.
  e_dirty_y_max=dirty_y_max; //screen_y_map[dirty_y_max];

  if (updated)
  {
      memcpy(dirtyvidram,&lisaram[videolatchaddress],32768);
      repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;
      updated=0;
  }

}

///////////////// SETRGB 16 bit Raw Replacement MACRO //////////////////////////////////////////////////   
//                                                                                                    //
//  macro to fill in r,g,b values, does 16 pixels at a time, must be a macro because of the Z param   //
//  and we want to get as much speed out of it as possible.                                           //
//                                                                                                    //
//  This would have been a 32 pixel macro if it were possible, however, since a single row is on the  //
//  Lisa's display is either 90 bytes or 76 bytes, we can only evenly divide by 2 bytes (16 bits.)    //
//                                                                                                    //
//                                                                                                    //
//  x,y are x,y coordinates on the bitmap.   They map to the Lisa's display, any other translation    //
//  must be handled in Z, or via other variables.   x gets incremented for each pixel handled.        //
//  x,y must be simple variables, and should not be expressions!                                      //
//                                                                                                    //
//  Z is a chunk of code to set r,g,b provided by the caller.  Z gets executed 16 times, once per     //
//  pixel.  This is needed because we might be using SetRGB on images or rawbitmap accesss, or some   //
//  other method in the future.   Z should be protected by enclosing it in curly braces when passing. //
//  Z may use the uint8 d to actually set the darkness level for a pixel.                             //
//                                                                                                    //
//  Y is identical to Z except that it's code to call when there's no update.  i.e. ++p               //
//                                                                                                    //
//                                                                                                    //
//  The following variables should be declared before calling this macro:                             //
//                                                                                                    //
//         int updated=0;            // number of times we've made updates.                            //
//                                  // can be used as a threshhold to decide how to redraw the image. //
//         uint32 a1,a2,a3,xx;      // address above, below, for this value, horziontal byte offset   //
//         uint16 vup,vdn,val;      // value above, below, this words read from addr @ a1,a2,a3       //
//         uint16 high,medium,low;  // used for antialiasing. how many neighboring bits               //
//         uint8 d;                 // darkness level to pass to Z                                    //
//                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SETRGB16_RAW_X(x,y,Z) {                                                                         \
                                                                                                        \
     xx=(x)>>3;                                                /*   turn x coord into byte offset */    \
                                                                                                        \
     a3=(yoffset[y]+xx) &32767;                                /*   this value we're processing   */    \
     val=(lisaram[videolatchaddress+a3]<<8 )|lisaram[videolatchaddress+a3+1];  /*   this word     */    \
                                                                                                        \
     updated++;                                               /*  Keep track of update count      */    \
                                                                                                        \
     dirty_x_min=MIN(x,dirty_x_min);       dirty_x_max=MAX(x+16,dirty_x_max);                           \
     dirty_y_min=MIN(y,dirty_y_min);       dirty_y_max=MAX(y,dirty_y_max);                              \
                                                                                                        \
     d=bright[ ((BIT15 & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT14 & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT13 & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT12 & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT11 & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT10 & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT9  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT8  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT7  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT6  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT5  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT4  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT3  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT2  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT1  & val) ? 7:0) ]; Z; x++;                                                         \
     d=bright[ ((BIT0  & val) ? 7:0) ]; Z; x++;                                                         \
}                                                                                                     //
////////////////// SETRGB MACRO ENDS ///////////////////////////////////////////////////////////////////


///////////////// SETRGB 16 bit Raw Replacement MACRO //////////////////////////////////////////////////   
//                                                                                                    //
//  macro to fill in r,g,b values, does 16 pixels at a time, must be a macro because of the Z param   //
//  and we want to get as much speed out of it as possible.                                           //
//                                                                                                    //
//  This would have been a 32 pixel macro if it were possible, however, since a single row is on the  //
//  Lisa's display is either 90 bytes or 76 bytes, we can only evenly divide by 2 bytes (16 bits.)    //
//                                                                                                    //
//                                                                                                    //
//  x,y are x,y coordinates on the bitmap.   They map to the Lisa's display, any other translation    //
//  must be handled in Z, or via other variables.   x gets incremented for each pixel handled.        //
//  x,y must be simple variables, and should not be expressions!                                      //
//                                                                                                    //
//  Z is a chunk of code to set r,g,b provided by the caller.  Z gets executed 16 times, once per     //
//  pixel.  This is needed because we might be using SetRGB on images or rawbitmap accesss, or some   //
//  other method in the future.   Z should be protected by enclosing it in curly braces when passing. //
//  Z may use the uint8 d to actually set the darkness level for a pixel.                             //
//                                                                                                    //
//  Y is identical to Z except that it's code to call when there's no update.  i.e. ++p               //
//                                                                                                    //
//                                                                                                    //
//  The following variables should be declared before calling this macro:                             //
//                                                                                                    //
//         int updated=0;            // number of times we've made updates.                            //
//                                  // can be used as a threshhold to decide how to redraw the image. //
//         uint32 a1,a2,a3,xx;      // address above, below, for this value, horziontal byte offset   //
//         uint16 vup,vdn,val;      // value above, below, this words read from addr @ a1,a2,a3       //
//         uint16 high,medium,low;  // used for antialiasing. how many neighboring bits               //
//         uint8 d;                 // darkness level to pass to Z                                    //
//                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SETRGB16_RAW(x,y,Z,Y) {                                                                        \
                                                                                                       \
     xx=(x)>>3;                                                /*   turn x coord into byte offset */   \
                                                                                                       \
     a3=(yoffset[screen_to_mouse[ y           ]]+xx) &32767;   /*   this value we're processing   */   \
     val=(lisaram[videolatchaddress+a3]<<8 )|lisaram[videolatchaddress+a3+1];  /*   this word     */   \
                                                                                                       \
     if (videoramdirty>DIRECT_BLITS_THRESHHOLD ||        /*  If full update requested or          */   \
        ((dirtyvidram[a3]<<8)|dirtyvidram[a3+1])!=val  )  /*  value is changed                    */   \
      {                                                                                                \
       updated++;                                          /*  Keep track of update count           */ \
                                                                                                       \
       dirty_x_min=MIN(x,dirty_x_min);       dirty_x_max=MAX(x+16,dirty_x_max);                        \
       dirty_y_min=MIN(y,dirty_y_min);       dirty_y_max=MAX(y,dirty_y_max);                           \
                                                                                                       \
                                                                                                       \
       d=bright[ ((BIT15 & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT14 & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT13 & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT12 & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT11 & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT10 & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT9  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT8  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT7  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT6  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT5  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT4  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT3  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT2  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT1  & val) ? 7:0) ]; Z; x++;                                                      \
       d=bright[ ((BIT0  & val) ? 7:0) ]; Z; x++;                                                      \
      }                                                                                                \
     else                                                                                              \
      {                                                                                                \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
        Y; x++;  Y; x++;  Y; x++;  Y; x++;                                                             \
      }                                                                                                \
}                                                                                                     //
////////////////// SETRGB MACRO ENDS ///////////////////////////////////////////////////////////////////

void LisaWin::RePaint_SingleY(void)
{
    // vars for SETRGB16_RAW
    int updated=0;    
    uint32 a3,xx;
    uint16 val;
    uint8  d;

    dirty_x_min=720; dirty_x_max=-1; dirty_y_min=364*3; dirty_y_max=-1;

#ifdef USE_RAW_BITMAP_ACCESS

    int depth;  
    int width;  
    int height; 
    int ox,oy; // private to this - not member vars

   
if (skins_on)
   {
    if (!my_skin) return;
    depth = my_skin->GetDepth();    width = my_skin->GetWidth();    height= my_skin->GetHeight();
    ox=skin.screen_origin_x;    oy=skin.screen_origin_y;
   }
else
   {
    if (!my_lisabitmap) return;
    depth = my_lisabitmap->GetDepth();    width = my_lisabitmap->GetWidth();    height= my_lisabitmap->GetHeight();
    ox=0; oy=0;
   }

   typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
   PixelData data(skins_on ? *my_skin:*my_lisabitmap);
   if (!data) {DEBUG_LOG(0,"No data."); return;}

   PixelData::Iterator p(data);
   p.Reset(data);

   p.MoveTo(data,ox,oy);                            
   for ( int y = 0; y < effective_lisa_vid_size_y; ++y )
   {
       PixelData::Iterator rowStart = p;              // save the x,y coordinates at the start of the line
       for ( int x = 0; x < o_effective_lisa_vid_size_x; )
           { SETRGB16_RAW(x,y,  {p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p;} , {++p;} );}  

       p = rowStart; p.OffsetY(data, 1);              // restore the x,y coords from start of line, then increment y via P.OffsetY to do y++;
   }

#else

  // Since we're working the slower way - with images, we need to rebuild the bitmap from the image.
  // to do this, we discard the old bitmap and recreate it from the display_image.  This is of course
  // slower, which is why we recommend the use of USE_RAW_BITMAP_ACCESS, but USE_RAW_BITMAP_ACCESS
  // might not work everywhere, so we give the option.
  if (skins_on)
  {
    if (!my_skin || !my_lisabitmap) return; 
    if (!display_image)   
        display_image= new wxImage(  my_lisabitmap->ConvertToImage());

    for ( int y=0; y < o_effective_lisa_vid_size_y; y++ )
        {
           // note neither xi, nor xo are incremented as part of the for-loop header, this is because
           // the SETRGB16_AA macro expands to 16 iterations, and it increments xi on each iteration.
           // however, it doesn't do anything with xo, so xo++ is passed as a parameter.
            for ( int x=0; x < o_effective_lisa_vid_size_x;)
                { SETRGB16_RAW(x,y,{display_image->SetRGB(x,y,d,d,d+EXTRABLUE); },{;}); }
        }

    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);
    
    e_dirty_x_min=dirty_x_min;                       // need to do these here so we can update just the rectangle we need.
    e_dirty_x_max=dirty_x_max;                       // it will be repeated again below, but so what, it's only 4 assignments
    e_dirty_y_min=dirty_y_min; //screen_y_map[dirty_y_min];         // and two lookups.
    e_dirty_y_max=dirty_y_max; //screen_y_map[dirty_y_max];

    if (updated)
    {    my_skinDC->StretchBlit(_H(skin.screen_origin_x + e_dirty_x_min),   _H(skin.screen_origin_y + e_dirty_y_min),     // target x,y
                                _H(e_dirty_x_max-e_dirty_x_min+1),          _H(e_dirty_y_max-e_dirty_y_min+1),            // size w,h
                                  my_memDC, 0,0, 
                                  (e_dirty_x_max-e_dirty_x_min+1),            (e_dirty_y_max-e_dirty_y_min+1),            // size in src
                                  wxCOPY, false);
    }

  }
 else  // skins are off
  {
    if (!display_image)   display_image= new wxImage(my_lisabitmap->ConvertToImage());

    for ( int yo = 0 , yi=0; yi < o_effective_lisa_vid_size_y; yo++,yi++ )
        {
           // note neither xi, nor xo are incremented as part of the for-loop header, this is because
           // the SETRGB16_AA macro expands to 16 iterations, and it increments xi on each iteration.
           // however, it doesn't do anything with xo, so xo++ is passed as a parameter.
           for ( int xo = 0, xi=0; xi < o_effective_lisa_vid_size_x;)
               { SETRGB16_RAW(xi,yi,{display_image->SetRGB(xo,yo,d,d,d+EXTRABLUE); xo++;},{xo++;});   }
        }

    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);
  }
#endif
/////////////////////////////////////////////////////////////////////////

  e_dirty_x_min=dirty_x_min;                       // need to do these here so we can update just the rectangle we need.
  e_dirty_x_max=dirty_x_max;                       // it will be repeated again below, but so what, it's only 4 assignments
  e_dirty_y_min=dirty_y_min;                       // and two lookups.
  e_dirty_y_max=dirty_y_max; 


  if (updated)
  {
      memcpy(dirtyvidram,&lisaram[videolatchaddress],32768);
      repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;
      updated=0;
  }


}

// this is essentially the same as SingleY, only for a differently sized display.
void LisaWin::RePaint_3A(void)
{
   // vars for SETRGB16_RAW
   int updated=0;    
   uint32 a3,xx;
   uint16 val;
   uint8  d;

   dirty_x_min=720; dirty_x_max=-1; dirty_y_min=364*3; dirty_y_max=-1;

#ifdef USE_RAW_BITMAP_ACCESS

 int depth;  
 int width;  
 int height; 
 int ox,oy; // private to this - not member vars

   
if (skins_on)
   {
    if (!my_skin) return;
    depth = my_skin->GetDepth();    width = my_skin->GetWidth();    height= my_skin->GetHeight();
    ox=skin.screen_origin_x;    oy=skin.screen_origin_y;
   }
else
   {
    if (!my_lisabitmap) return;
    depth = my_lisabitmap->GetDepth();    width = my_lisabitmap->GetWidth();    height= my_lisabitmap->GetHeight();
    ox=0; oy=0;
   }

   typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
   PixelData data(skins_on ? *my_skin:*my_lisabitmap);

   PixelData::Iterator p(data);
   p.Reset(data);
   p.MoveTo(data,ox,oy);                             

   for ( int y = 0; y < o_effective_lisa_vid_size_y; ++y )   // effective_lisa_vid_size_y
   {
       PixelData::Iterator rowStart = p;                   // save the x,y coordinates at the start of the line

       for ( int x = 0; x < o_effective_lisa_vid_size_x; )
           { SETRGB16_RAW(x,y,  {p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p;} , {++p;} );}  

       p = rowStart; p.OffsetY(data, 1);                   // restore the x,y coords from start of line, then increment y to do y++;
   }

#else

  // Since we're working the slower way - with images, we need to rebuild the bitmap from the image.
  // to do this, we discard the old bitmap and recreate it from the display_image.  This is of course
  // slower, which is why we recommend the use of USE_RAW_BITMAP_ACCESS, but USE_RAW_BITMAP_ACCESS
  // might not work everywhere, so we give the option.
  if (skins_on)
  {
    if (!my_skin || !my_lisabitmap) return; 

    if (!display_image)   
        display_image= new wxImage(  my_lisabitmap->ConvertToImage());

    for ( int y=0; y < o_effective_lisa_vid_size_y; y++ )
        {
           // note neither xi, nor xo are incremented as part of the for-loop header, this is because
           // the SETRGB16_AA macro expands to 16 iterations, and it increments xi on each iteration.
           // however, it doesn't do anything with xo, so xo++ is passed as a parameter.
            for ( int x=0; x < o_effective_lisa_vid_size_x;)
                { SETRGB16_RAW(x,y,{display_image->SetRGB(x,y,d,d,d+EXTRABLUE); },{;}); }
        }

    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);
    
    e_dirty_x_min=dirty_x_min;                       // need to do these here so we can update just the rectangle we need.
    e_dirty_x_max=dirty_x_max;                       // it will be repeated again below, but so what, it's only 4 assignments
    e_dirty_y_min=dirty_y_min; //screen_y_map[dirty_y_min];         // and two lookups.
    e_dirty_y_max=dirty_y_max; //screen_y_map[dirty_y_max];

    if (updated)
        my_skinDC->StretchBlit(  (skin.screen_origin_x + _H(e_dirty_x_min) ),  (skin.screen_origin_y + _H(e_dirty_y_min) ),     // target x,y
                               _H(e_dirty_x_max-e_dirty_x_min+1),            _H(e_dirty_y_max-e_dirty_y_min+1),                 // size w,h
                                my_memDC, 0,0,                                                                                  // source, source x,y
                                (e_dirty_x_max-e_dirty_x_min+1),  (e_dirty_y_max-e_dirty_y_min+1),                              // source size w,h
                                wxCOPY, false);
  }
  else  // skins are off
  {
    if (!display_image)   display_image= new wxImage(my_lisabitmap->ConvertToImage());

    for ( int y=0; y < o_effective_lisa_vid_size_y; y++ )
        {
           // note neither xi, nor xo are incremented as part of the for-loop header, this is because
           // the SETRGB16_AA macro expands to 16 iterations, and it increments xi on each iteration.
           // however, it doesn't do anything with xo, so xo++ is passed as a parameter.
           //for ( int xo = 0, xi=0; xi < effective_lisa_vid_size_x;)
               // { SETRGB16_RAW(xi,yi,{display_image->SetRGB(xo,yo,d,d,d+EXTRABLUE); xo++;},{xo++;});   }
            for ( int x=0; x < o_effective_lisa_vid_size_x;)
                { SETRGB16_RAW(x,y,{display_image->SetRGB(x,y,d,d,d+EXTRABLUE); },{;}); }
        }

    delete my_lisabitmap;
    my_lisabitmap=new wxBitmap(*display_image);
    my_memDC->SelectObjectAsSource(*my_lisabitmap);
  }
#endif
/////////////////////////////////////////////////////////////////////////

  e_dirty_x_min=dirty_x_min;                       // need to do these here so we can update just the rectangle we need.
  e_dirty_x_max=dirty_x_max;                       // it will be repeated again below, but so what, it's only 4 assignments
  e_dirty_y_min=dirty_y_min; //screen_y_map[dirty_y_min];         // and two lookups.
  e_dirty_y_max=dirty_y_max; //screen_y_map[dirty_y_max];

  if (updated)
  {
      memcpy(dirtyvidram,&lisaram[videolatchaddress],32768);
      repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;
      updated=0;
  }


}


// no skins here.
void LisaWin::RePaint_DoubleY(void)
{
   // vars for SETRGB16_RAW
    int updated=0;    
    uint32 a3,xx;
    uint16 val;
    uint8  d;

    int width;  
    int height; 


    if (skins_on) {ALERT_LOG(0,"Skins should not be on!!!!"); turn_skins_off();}
    if (!my_lisabitmap) return;
    dirty_x_min=720; dirty_x_max=-1; dirty_y_min=364*2; dirty_y_max=-1;

    if (!my_lisabitmap) return;
    width = my_lisabitmap->GetWidth();    height= my_lisabitmap->GetHeight();

    if (width<720 || height<364*2)
    {
      ALERT_LOG(0,"my_lisabitmap too small: %d,%d vs o_effective_lisa_vid_size: %d,%d",width,height,
                o_effective_lisa_vid_size_x,o_effective_lisa_vid_size_y);   
      free(my_lisabitmap); my_lisabitmap=NULL; return;
    }

#ifdef USE_RAW_BITMAP_ACCESS
    int ox,oy; // private to this - not member vars
    ox=0; oy=0;


    typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
    PixelData data(*my_lisabitmap);
    if (!data) return;

    PixelData::Iterator p(data);
    p.Reset(data);
    p.MoveTo(data,ox,oy);                             

    for ( int y = 0; y < o_effective_lisa_vid_size_y; ++y )   // effective_lisa_vid_size_y
    {
       PixelData::Iterator rowStart = p;                   // save the x,y coordinates at the start of the line

      for ( int x = 0; x < o_effective_lisa_vid_size_x; )
          { SETRGB16_RAW(x,y,  {p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p;} , {++p;} );}  

       p = rowStart; p.OffsetY(data, 1);                   // restore the x,y coords from start of line, then increment y to do y++;
    }

#else

  // Since we're working the slower way - with images, we need to rebuild the bitmap from the image.
  // to do this, we discard the old bitmap and recreate it from the display_image.  This is of course
  // slower, which is why we recommend the use of USE_RAW_BITMAP_ACCESS, but USE_RAW_BITMAP_ACCESS
  // might not work everywhere, so we give the option.

    if (width<o_effective_lisa_vid_size_x || height<o_effective_lisa_vid_size_y )
    {
      ALERT_LOG(0,"my_lisabitmap too small: %d,%d vs o_effective_lisa_vid_size: %d,%d",width,height,
                o_effective_lisa_vid_size_x,o_effective_lisa_vid_size_y);   
      free(my_lisabitmap); my_lisabitmap=NULL; return;
    }


  if (!display_image)   display_image= new wxImage(my_lisabitmap->ConvertToImage());
  
  for ( int y=0; y < 364*2; y++)
      {
        for ( int x=0; x < 720;)
            { //ALERT_LOG(0,"%d,%d img:%d,%d max:%d,%d",x,y,width,height,effective_lisa_vid_size_x,effective_lisa_vid_size_y);
              SETRGB16_RAW(x,y,{display_image->SetRGB(x,y,d,d,d+EXTRABLUE); },{;}); }
      }
  
  delete my_lisabitmap;
  my_lisabitmap=new wxBitmap(*display_image);
  my_memDC->SelectObjectAsSource(*my_lisabitmap);

#endif
/////////////////////////////////////////////////////////////////////////

  e_dirty_x_min=dirty_x_min;                       // need to do these here so we can update just the rectangle we need.
  e_dirty_x_max=dirty_x_max;                       // it will be repeated again below, but so what, it's only 4 assignments
  e_dirty_y_min=dirty_y_min; //screen_y_map[dirty_y_min];         // and two lookups.
  e_dirty_y_max=dirty_y_max; //screen_y_map[dirty_y_max];


  if (updated)
  {
      memcpy(dirtyvidram,&lisaram[videolatchaddress],32768);
      repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;
      updated=0;
  }

}


void LisaWin::RePaint_2X3Y(void)
{
       // vars for SETRGB16_RAW
      int    updated=0;    
      uint32 a3,xx;
      uint16 val;
      uint8  d;

      if (!my_lisabitmap) return;
      dirty_x_min=720; dirty_x_max=-1; dirty_y_min=364*2; dirty_y_max=-1;
    
#ifdef USE_RAW_BITMAP_ACCESS

      int depth;  
      int width;  
      int height; 
      int ox,oy; // private to this - not member vars

      if (!my_lisabitmap) return;
      depth = my_lisabitmap->GetDepth();    width = my_lisabitmap->GetWidth();    height= my_lisabitmap->GetHeight();
      ox=0; oy=0;

      typedef wxPixelData<wxBitmap,wxNativePixelFormat> PixelData;
      PixelData data(*my_lisabitmap);
      if (!data) return;

      PixelData::Iterator p(data);
      p.Reset(data);
      p.MoveTo(data,ox,oy);

      for ( int y = 0; y < o_effective_lisa_vid_size_y; ++y )   // effective_lisa_vid_size_y
      {
          PixelData::Iterator rowStart = p;              // save the x,y coordinates at the start of the line
          for ( int x = 0; x < o_effective_lisa_vid_size_x; )
            { SETRGB16_RAW(x,y,  {p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p; p.Red()=d; p.Green()=d; p.Blue()=(d+EXTRABLUE); ++p;} , 
                                  {++p;++p;} );}  
           p = rowStart; p.OffsetY(data, 1);              // restore the x,y coords from start of line, then increment y to do y++;
      }

#else
      // Since we're working the slower way - with images, we need to rebuild the bitmap from the image.
      // to do this, we discard the old bitmap and recreate it from the display_image.  This is of course
      // slower, which is why we recommend the use of USE_RAW_BITMAP_ACCESS, but USE_RAW_BITMAP_ACCESS
      // might not work everywhere, so we give the option.
      
      if (!display_image)   {display_image= new wxImage(my_lisabitmap->ConvertToImage());}

      for ( int yo = 0 , yi=0; yi < o_effective_lisa_vid_size_y; yo++,yi++ )
          {
            for ( int xo = 0, xi=0; xi < effective_lisa_vid_size_x-2;)
                {  //ALERT_LOG(0,"SETRGB16_RAW:xo,yo:%d,%d  xi,yi:%d,%d",xo,yo,xi,yi);
                    SETRGB16_RAW(xi,yi,{display_image->SetRGB(xo,yo,d,d,d+EXTRABLUE); xo++; display_image->SetRGB(xo,yo,d,d,d+EXTRABLUE); xo++;},
                                      {xi++;}  );   } // was xo++ here
          }
      
      delete my_lisabitmap;
      my_lisabitmap=new wxBitmap(*display_image);
      my_memDC->SelectObjectAsSource(*my_lisabitmap);
    
#endif
    /////////////////////////////////////////////////////////////////////////

    e_dirty_x_min=dirty_x_min;                       // need to do these here so we can update just the rectangle we need.
    e_dirty_x_max=dirty_x_max;                       // it will be repeated again below, but so what, it's only 4 assignments
    e_dirty_y_min=dirty_y_min; //screen_y_map[dirty_y_min];         // and two lookups.
    e_dirty_y_max=dirty_y_max; //screen_y_map[dirty_y_max];

    if  (updated)
        {
          memcpy(dirtyvidram,&lisaram[videolatchaddress],32768);
          repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;
          updated=0;
        }
}


// void LisaWin::OnDraw(wxDC & dc) {} // should this be implemented?
void LisaWin::OnPaint(wxPaintEvent &event )
{
  DCTYPE dc(this); // 20190831 - let's see if this is needed
  DoPrepareDC(dc); // or this->PrepareDC(dc); this->OnDraw(dc);
  dc.SetUserScale(hidpi_scale,hidpi_scale);

  if (!my_lisaframe || !my_lisawin) return; // not fully running yet!

  if (!my_lisaframe->running)   repaintall |= REPAINT_INVALID_WINDOW;
  if (refresh_bytemap) {ALERT_LOG(0,"Contrast change"); ContrastChange();}

  if ( my_lisabitmap==NULL)
  {
      ALERT_LOG(0,"my_lisabitmap was NULL, allocating new one - shouldn't be doing this unless we just rescaled or 1st time");
      my_lisabitmap=new class wxBitmap(o_effective_lisa_vid_size_x,o_effective_lisa_vid_size_y, DEPTH); //,my_lisaframe->depth);
      my_memDC->SelectObjectAsSource(*my_lisabitmap);
      my_memDC->SetBrush(FILLERBRUSH);      my_memDC->SetPen(FILLERPEN);
      my_memDC->DrawRectangle(0 ,   0,   o_effective_lisa_vid_size_x, o_effective_lisa_vid_size_y);
      if (!my_memDC->IsOk()) ALERT_LOG(0,"my_memDC is not ok.");
      if (!my_lisabitmap->IsOk()) ALERT_LOG(0,"my_lisabitmap is not ok.");
  }

  if  ((dirtyscreen || videoramdirty) && (powerstate & POWER_ON_MASK) == POWER_ON)
      {   
          //ALERT_LOG(0,"Calling repainter");
          (my_lisawin->*RePainter)();  // whoever came up with this C++ syntax instead of the old C one was on crack!
      }

  wxRegionIterator upd(GetUpdateRegion()); // get the update rect list

  while (upd)
  {

    wxRect rect(upd.GetRect());
    wxRect display( _H(skin.screen_origin_x), _H(skin.screen_origin_y), effective_lisa_vid_size_x,effective_lisa_vid_size_y);

    // if this was an actual request to update the screen, then flag it as such.
    if (rect.Intersects(display))         repaintall |= REPAINT_INVALID_WINDOW | REPAINT_VIDEO_TO_SKIN;

    if ( skins_on) OnPaint_skins(   rect, dc);
    else           OnPaint_skinless(rect, dc);
    upd++;
  }


  dirtyscreen=0;
  videoramdirty=0;
 #ifdef __WXOSX__
//  event.Skip(false);
 #else
  event.Skip(false); // not sure if I need this or not
 #endif
}


void LisaWin::OnPaint_skinless(wxRect &rect, DCTYPE &dc)
{
  #ifdef __WXOSX__
    dc.SetBackgroundMode(wxTRANSPARENT); // OS X has a nasty habbit of erasing the background even though we skip the event!
  #endif

  int width, height, w_width, w_height, ww_width, ww_height;  
  width =rect.GetWidth(); height=rect.GetHeight();  // get region to repaint

  wxCoord w, h;
  dc.GetSize(&w, &h); // these are inside the window, and scaled * hidpi_scale via SetUserScale, and this is virtually much larger due to scrolling
  wxRect r=my_lisaframe->GetRect(); // these are outside the viewport, and the true size of the window, unscaled - want this one!
  w_width   = r.GetWidth();  //* hidpi_scale;              // inside frame size
  w_height  = r.GetHeight(); //* hidpi_scale;
  
  ww_width  = w_width  * hidpi_scale;    // scale these to get the scaled version of the viewport size
  ww_height = w_height * hidpi_scale;

  ww_width  = ww_width > w_width  ? w_width  : ww_width;  // limit from going outside the viewport
  ww_height = ww_height> w_height ? w_height : ww_height;

  // do this whenever we have a scale transition or switch from skin to skinless my_lisawin->clear_skinless=1;
  if (clear_skinless) {
      clear_skinless=0;

      dc.SetBrush(*wxBLACK_BRUSH);  dc.SetPen(*wxBLACK_PEN);
      dc.DrawRectangle(0 ,0,   65535,65535);  
  }

  if  (lisa_ui_video_mode==vidmod_hq35x) {
      if (!my_lisahq3xbitmap) return;
      o_effective_lisa_vid_size_x=my_lisahq3xbitmap->GetWidth() * hidpi_scale;
      o_effective_lisa_vid_size_y=my_lisahq3xbitmap->GetHeight()* hidpi_scale;
      ox=(w_width  - _H(720))/2;
      oy=(w_height - _H(548))/2;
  }
  else
  {
      ox=(w_width  - _H(effective_lisa_vid_size_x))/2;
      oy=(w_height - _H(effective_lisa_vid_size_x))/2;

  }
  if (!skinless_center)
  {   ox=0; oy=0;}
  else
  {
      if (ox<0) ox=0;
      if (oy<0) oy=0;
  }

  skin.screen_origin_x=ox;  
  skin.screen_origin_y=oy;

#ifdef DEBUGOFXXX
  static unsigned short ctr;
  ctr++;
  if (!(ctr & 7)) {ALERT_LOG(0,"win:%d,%d/wwin:%d,%d/dc:%d,%d o_e_lisa_vid_sz %d,%d ox,oy:(%d,%d) oi:%d,%d hidpi_scale:%f",
            w_width, w_height, ww_width,ww_height,(long)w,(long)h,
            o_effective_lisa_vid_size_x,o_effective_lisa_vid_size_y,
            ox,oy,((ww_width  - o_effective_lisa_vid_size_x)/2),((ww_height - o_effective_lisa_vid_size_y)/2),
            hidpi_scale);
            if (my_lisahq3xbitmap) ALERT_LOG(0,"my_lisahq3xbitmap: %dx%d %d bits",my_lisahq3xbitmap->GetWidth(),my_lisahq3xbitmap->GetHeight(),my_lisahq3xbitmap->GetDepth());
            if (my_lisabitmap)     ALERT_LOG(0,"my_lisabitmap:     %dx%d %d bits",my_lisabitmap->GetWidth(),my_lisabitmap->GetHeight(),my_lisabitmap->GetDepth());
  }
#endif

  if (my_lisaframe->running)
  {
      ex=ww_width-ox; ey=ww_height-oy;
      
      #ifdef DEBUGXX
      // debug draw color boxes around where we think the edges are so we see where we're wrong about size
      // but first wipe everything with a black rectangle to see the end of the window for the colored regions too.
      dc.SetBrush(*wxBLACK_BRUSH);      dc.SetPen(*wxBLACK_PEN);
      dc.DrawRectangle(0   , 0   , 1920,1080);
      // center of window
      dc.SetBrush(*wxCYAN_BRUSH);      dc.SetPen(*wxCYAN_PEN);
      // x,y, width, height
      dc.DrawRectangle(0,          w_height/2 -5 , w_width,     10);       // horizontal line
      dc.DrawRectangle(w_width/2 -5,0,             10,    w_height);         // vertical line

      #else
      // draw a black border around the lisa's display
      dc.SetBrush(*wxBLACK_BRUSH);      dc.SetPen(*wxBLACK_PEN);
      if (oy>0)         dc.DrawRectangle(0   , 0   , w_width, oy      );   // top
      if (ey<w_height)  dc.DrawRectangle(0   , ey-2, w_width, w_height);   // bottom
      if (ey>0)         dc.DrawRectangle(0   , oy  , ox     , ey      );   // left-center
      if (ex-1<w_width) dc.DrawRectangle(ex-1, oy  , w_width, ey      );   // center-right
      #endif

      if (e_dirty_x_min>e_dirty_x_max) {e_dirty_x_min=0; e_dirty_x_max=o_effective_lisa_vid_size_x;}
      if (e_dirty_y_min>e_dirty_y_max) {e_dirty_y_min=0; e_dirty_y_max=o_effective_lisa_vid_size_y;}

      switch (lisa_ui_video_mode) {
          case  vidmod_hq35x:
                dc.StretchBlit( ox+_H(e_dirty_x_min),              oy+_H(e_dirty_y_min),                      // target x,y on window
                                _H(e_dirty_x_max-e_dirty_x_min),   _H(e_dirty_y_max-e_dirty_y_min),           // width, height
                                my_memhq3xDC,                                                                 // src dc
                                e_dirty_x_min*2,                      e_dirty_y_min               *2*500/544, // src dc x,y
                               (e_dirty_x_max-e_dirty_x_min)*2,      (e_dirty_y_max-e_dirty_y_min)*2*500/544, // src dc size
                                wxCOPY, false);

                break;
          case  vidmod_3y:
                dc.StretchBlit( ox+_H(e_dirty_x_min*2),              oy+_H(e_dirty_y_min),                    // target x,y on window
                                _H(e_dirty_x_max-e_dirty_x_min)*2,   _H(e_dirty_y_max-e_dirty_y_min),         // width, height
                                my_memDC,                                                                     // src dc
                                e_dirty_x_min*2,  e_dirty_y_min,                                              // src dc x,y
                                (e_dirty_x_max-e_dirty_x_min)*2,        e_dirty_y_max-e_dirty_y_min,          // src dc size
                                wxCOPY, false);
                break;
          default:
                dc.StretchBlit( ox+_H(e_dirty_x_min),                oy+_H(e_dirty_y_min),                    // target x,y on window
                                _H(e_dirty_x_max-e_dirty_x_min),     _H(e_dirty_y_max-e_dirty_y_min),         // width, height
                                my_memDC,                                                                     // src dc
                                e_dirty_x_min,  e_dirty_y_min,                                                // src dc x,y
                                e_dirty_x_max-e_dirty_x_min,            e_dirty_y_max-e_dirty_y_min,          // src dc size
                                wxCOPY, false);
      }
      e_dirty_x_min=o_effective_lisa_vid_size_x; e_dirty_x_max=0; e_dirty_y_min=364*3; e_dirty_y_max=-1;      // reset coordinates for next pass
  }
  else    // not running
  {
      #ifdef DEBUG
        if (!dc.IsOk()) {ALERT_LOG(0,"event DC for repainting is not ok!");}
      #endif

      dc.SetBrush(*wxBLACK_BRUSH);  dc.SetPen(*wxBLACK_PEN);
      dc.DrawRectangle(0 ,0,   width,height);
  }

  repaintall=0;
}

void LisaWin::Skins_Repaint_PowerPlane(wxRect &rect, DCTYPE &dc)
{
    // The first time we're in here, draw the skins.  The skins are cut in horizontal quarters because
    // some versions of wxWidgets can't handle such a large wxBitmap.
    if ((powerstate & POWER_NEEDS_REDRAW)==POWER_NEEDS_REDRAW) // && my_skin0)
    {
      //ALERT_LOG(0,"On_paint skins: powerstate:%d floppystate:%d",powerstate,floppystate);
      prepare_skin();
      //powerstate &= ~POWER_NEEDS_REDRAW;
    }
    if ((powerstate & POWER_NEEDS_REDRAW) )
    {
      // The power's going to be redrawn so we to blit the power to the
      // skin, and then the window will need to be updated from the skin.
      repaintall |= REPAINT_INVALID_WINDOW | REPAINT_POWER_TO_SKIN;
      //ALERT_LOG(0,"Stretchblitting power state onto my_skinDC");

      if  ((powerstate & POWER_ON_MASK) == POWER_ON)
          {
            my_skinDC->StretchBlit(   POWER_LEFT, POWER_TOP,  
                                      _H(my_poweron->GetWidth()), _H(my_poweron->GetHeight()), 
                                      my_poweronDC, 0,0, my_poweron->GetWidth(), my_poweron->GetHeight(),
                                      wxCOPY, true);

          #if defined(DEBUG) && defined(DEBUG_MOUSE_LOCATION)
              my_skinDC->SetPen(*wxGREEN_PEN); my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH );
              my_skinDC->DrawRectangle( _H(skin.power_button_tl_x-2),_H(skin.power_button_tl_y-2),
                                        _H(skin.power_button_br_x - skin.power_button_tl_x +2),
                                        _H(skin.power_button_br_y - skin.power_button_tl_y +2) );
          #endif

          // These coordinates are a small rectangle around the powerlight
          if (powerstate & POWER_PUSHED) {
              my_skinDC->StretchBlit( _H(skin.power_button_tl_x-2),_H(skin.power_button_tl_y-2),         // left,top corner x,y
                                      _H(skin.power_button_br_x - skin.power_button_tl_x),
                                      _H(skin.power_button_br_y - skin.power_button_tl_y),               // width height
                                      my_poweronDC,  skin.power_button_tl_x+2,skin.power_button_tl_y+2,  // source dc,   x,y
                                      (skin.power_button_br_x   - skin.power_button_tl_x),
                                      (skin.power_button_br_y   - skin.power_button_tl_y),               // width height
                                      wxCOPY, true);  }
        }
     else 
        {
          my_skinDC->StretchBlit(    _H(skin.power_button_tl_x),_H(skin.power_button_tl_y),              // left,top corner x,y
                                     _H(skin.power_button_br_x - skin.power_button_tl_x),
                                     _H(skin.power_button_br_y - skin.power_button_tl_y),                // width height
                                     my_poweroffDC,  skin.power_button_tl_x,skin.power_button_tl_y,      // source dc,   x,y
                                     (skin.power_button_br_x   - skin.power_button_tl_x),
                                     (skin.power_button_br_y   - skin.power_button_tl_y),                // width height
                                     wxCOPY, true);

          if (powerstate & POWER_PUSHED)
             my_skinDC->StretchBlit( _H(skin.power_button_tl_x-2),_H(skin.power_button_tl_y+2),          // left,top corner x,y
                                     _H(skin.power_button_br_x - skin.power_button_tl_x),
                                     _H(skin.power_button_br_y - skin.power_button_tl_y),                // width height
                                     my_poweroffDC,  skin.power_button_tl_x+2,skin.power_button_tl_y+2,  // source dc,   x,y
                                     (skin.power_button_br_x   - skin.power_button_tl_x),
                                     (skin.power_button_br_y   - skin.power_button_tl_y),                // width height
                                     wxCOPY, true);
        }

     powerstate &= ~POWER_PUSHED;
     powerstate &= ~POWER_NEEDS_REDRAW;

     //ALERT_LOG(0,"calling force refresh");
     force_refresh();
     //ALERT_LOG(0,"Done");
    }

}


void LisaWin::Skins_Repaint_Floppy(wxRect &rect, DCTYPE &dc)
{
    if ((floppystate & FLOPPY_NEEDS_REDRAW) )
    {
       // The floppy's going to be redrawn so we to blit the floppy to the
       // skin, and then the window will need to be updated from the skin.
      repaintall |= REPAINT_INVALID_WINDOW | REPAINT_FLOPPY_TO_SKIN;

      switch(floppystate & FLOPPY_ANIM_MASK)
      {

        case 0:  my_skinDC->StretchBlit(FLOPPY_LEFT, FLOPPY_TOP, _H(my_floppy0->GetWidth()), _H(my_floppy0->GetHeight()), my_floppy0DC,
                                        0,0 ,my_floppy0->GetWidth(),my_floppy0->GetHeight(), wxCOPY, false); break;

        case 1:  my_skinDC->StretchBlit(FLOPPY_LEFT, FLOPPY_TOP, _H(my_floppy1->GetWidth()), _H(my_floppy1->GetHeight()), my_floppy1DC,
                                        0,0 ,my_floppy1->GetWidth(),my_floppy1->GetHeight(), wxCOPY, false); break;

        case 2:  my_skinDC->StretchBlit(FLOPPY_LEFT, FLOPPY_TOP, _H(my_floppy2->GetWidth()), _H(my_floppy2->GetHeight()), my_floppy2DC,
                                        0,0 ,my_floppy2->GetWidth(),my_floppy2->GetHeight(), wxCOPY, false); break;

        case 3:  my_skinDC->StretchBlit(FLOPPY_LEFT, FLOPPY_TOP, _H(my_floppy3->GetWidth()), _H(my_floppy3->GetHeight()), my_floppy3DC,
                                        0,0 ,my_floppy3->GetWidth(),my_floppy3->GetHeight(),wxCOPY, false); break;

        default: my_skinDC->StretchBlit(FLOPPY_LEFT, FLOPPY_TOP, _H(my_floppy4->GetWidth()), _H(my_floppy4->GetHeight()), my_floppy4DC,
                                        0,0, my_floppy4->GetWidth(),my_floppy4->GetHeight(),wxCOPY, false);
      }

      floppystate &= FLOPPY_ANIMATING | FLOPPY_ANIM_MASK;
    }
}

void LisaWin::OnPaint_skins(wxRect &rect, DCTYPE &dc)
{
    static int count;
    //wxPaintDC dc(this);
    #ifdef __WXOSX__
      //ALERT_LOG(0,"Setting background mode")
      dc.SetBackgroundMode(wxTRANSPARENT); // OS X has a nasty habbit of erasing the background even though we skip the event!
    #endif

    Skins_Repaint_PowerPlane(rect,dc);
    Skins_Repaint_Floppy(rect,dc);

     ///////////////////////////////////////////////////////////////////
     int vbX,vbY,width,height;

     // refresh it all every 32 cycles.
     count++;
     if ((count & 31)==0)
     {
         count  = 0; 
         vbX    = 0; 
         vbY    = 0;
         width  = my_skin->GetWidth();
         height = my_skin->GetHeight();
     }
     else
     {
         GetViewStart(&vbX,&vbY);           // convert scrollbar position into skin relative pixels

         vbX=vbX * (my_skin->GetWidth()/100);
         vbY=vbY * (my_skin->GetHeight()/100);

         vbX  += rect.GetX()-1;
         vbY  += rect.GetY()-1;
         height= rect.GetHeight()+1;
         width = rect.GetWidth()+1;
    
         width=  MIN(width,my_skin->GetWidth()  );
         height= MIN(height,my_skin->GetHeight());
         vbX   = MAX(vbX,1);
         vbY   = MAX(vbY,1);
     }
     #ifdef DEBUG
         if (!dc.IsOk()) {ALERT_LOG(0,"event DC for repainting is not ok!");}
     #endif

     dc.Blit(vbX,vbY, width, height, my_skinDC, vbX,vbY, wxCOPY, false);

     // ::TODO:: should fill in the space after this to the end of the DC with black
     //ALERT_LOG(0,"Done");
     repaintall=0;
}

void LisaWin::OnErase(wxEraseEvent &event)
{
  //DCTYPE dc(this); // 20190831 - let's see if this is needed
  //DoPrepareDC(dc); // or this->PrepareDC(dc); this->OnDraw(dc);
  //dc.SetUserScale(hidpi_scale,hidpi_scale);
  //event.GetDC();  //https://wiki.wxwidgets.org/Flicker-Free_Drawing
  //ALERT_LOG(0,"entering");
  #ifdef __WXOSX__
    event.GetDC()->SetBackground(*wxTRANSPARENT_BRUSH);
    event.GetDC()->SetBackgroundMode(wxTRANSPARENT);
    //event.Skip();
    event.StopPropagation();
  #endif
  #ifdef __WXGTK__
    event.Skip();
    event.StopPropagation();
  #endif
  //ALERT_LOG(0,"Dome");
}


inline void flushscreen(void) 
{//if (my_lisawin    )my_lisawin->Refresh();
  if (my_lisawin)   my_lisawin->Update();  
 //if (my_lisaframe) my_lisaframe->Refresh(false,NULL);
  if (my_lisaframe) my_lisaframe->Update(); 
}


void black(void)
{
  if  (skins_on && !!my_skin)
      {
        if (my_skinDC)
        {
          //ALERT_LOG(0,"Setting brush")   
          my_skinDC->SetBrush(FILLERBRUSH);   // these must be white, else linux drawing breaks
          my_skinDC->SetPen(FILLERPEN);       // these must be white, else linux drawing breaks
          my_skinDC->DrawRectangle(_H(skin.screen_origin_x),     _H(skin.screen_origin_y), 
                                      effective_lisa_vid_size_x,    effective_lisa_vid_size_y);
        //ALERT_LOG(0,"done");
        }
      }
  else
      {
        if (my_memDC)
        {
           //ALERT_LOG(0,"Setting brush skinless")
           my_memDC->SetBrush(FILLERBRUSH);   // these must be white, else linux drawing breaks
           my_memDC->SetPen(FILLERPEN);       // these must be white, else linux drawing breaks
           my_memDC->DrawRectangle(0,0,effective_lisa_vid_size_x+1,effective_lisa_vid_size_y);
        }
      }
  //ALERT_LOG(0,"Flushing screen");
  flushscreen();
  //ALERT_LOG(0,"done.")
}


extern "C" void x_setstatusbar(char *text)
{
    wxString x=wxString(text, wxConvLocal, 2048); //wxSTRING_MAXLEN);
    my_lisaframe->SetStatusBarText(x);
}

#define setstatusbar(text)         { ALERT_LOG(0,(text)); x_setstatusbar(text); }

extern "C" int islisarunning(void) { return     my_lisaframe->running;          }


// Let your hammer fly, let the lightning crack the blackened skies
void lightning(void)
{
  if  (!skins_on)
      {
        contrast=0xf;
        //contrastchange();
        black();
        ALERT_LOG(0,"Lightning skipped - skins off.");
        return;
      }
  static XTIMER lastclk;

  if  (lastclk==cpu68k_clocks) 
      {  ALERT_LOG(0,"skipping duplicate"); return; } // got a duplicate due to screen refresh

  lastclk=cpu68k_clocks;

  // CAN YOU FEEL IT BUILDING?
  // CAN YOU FEEL IT BUILDING?
  black();
  // DEVASTATION IS ON THE WAY

  wxMilliSleep(300);
  my_poweroffclk->Play(wxSOUND_ASYNC);

 /* Draw the central flash */
  my_skinDC->SetPen(FILLERPEN);
  my_skinDC->SetBrush(FILLERBRUSH);
  my_skinDC->DrawRectangle( _H(skin.screen_origin_x) + effective_lisa_vid_size_x/2 -_H(24), _H(skin.screen_origin_y),
                            _H(48), effective_lisa_vid_size_y);

  videoramdirty=0; my_lisawin->RefreshRect(wxRect( _H(skin.screen_origin_x ) + effective_lisa_vid_size_x/2 -_H(24), 
                                                  _H(skin.screen_origin_y ),
                                                  _H(48), effective_lisa_vid_size_y),false);

  flushscreen();
  wxMilliSleep(100);

 /* Blacken the screen */
  my_skinDC->SetPen(*wxBLACK_PEN);
  my_skinDC->SetBrush(*wxBLACK_BRUSH);


  for (int i=_H(12); i>-1; i-=4)
  {
   // left
    ALERT_LOG(0,"%d",i);
    my_skinDC->SetPen(*wxBLACK_PEN);
    my_skinDC->SetBrush(*wxBLACK_BRUSH);

    my_skinDC->DrawRectangle( _H(skin.screen_origin_x),         _H(skin.screen_origin_y),
                                 effective_lisa_vid_size_x/2 -i, effective_lisa_vid_size_y);

   // right
    my_skinDC->DrawRectangle( _H(skin.screen_origin_x) + effective_lisa_vid_size_x/2 + i, _H(skin.screen_origin_y),
                              effective_lisa_vid_size_x/2 -i+1,                         effective_lisa_vid_size_y);

    videoramdirty=0;
    my_lisawin->RefreshRect(wxRect( _H(skin.screen_origin_x), _H(skin.screen_origin_y),
                                    effective_lisa_vid_size_x, effective_lisa_vid_size_y ),
                                    false);
    flushscreen();
    wxMilliSleep(75);
  }


  my_lisawin->powerstate = POWER_NEEDS_REDRAW;     my_lisawin->dirtyscreen=1;     my_lisawin->repaintall|=1;
  flushscreen();
  my_lisawin->powerstate = POWER_OFF;
  contrast=0xf0;
  contrastchange();
  flushscreen();
}


int initialize_all_subsystems(void);

void handle_powerbutton(void)
{
      ALERT_LOG(0,"======== ENTRY ================");
      ALERT_LOG(0,"powerstate: %d",my_lisawin->powerstate);
      ALERT_LOG(0,"running   : %d",my_lisaframe->running);
      ALERT_LOG(0,"===============================");

      if  ((my_lisawin->powerstate & POWER_ON_MASK) == POWER_ON) // power is already on, send a power-key event instead
          {
            int i;
            ALERT_LOG(0,"Power is on, sending power key event to COPS");
            setstatusbar("Sending power key event");
            presspowerswitch();                      // send keyboard event.
            my_lisaconfig->Save(pConfig, floppy_ram);

            if  (running_lisa_os==LISA_OFFICE_RUNNING)
                {
                   refresh_rate_used=5*REFRESHRATE;       // speed up contrast ramp by lowering the refresh rate
                   my_lisaframe->clockfactor=0;           // speed up shutdown to compensane for printing slowdown
                }

            setstatusbar("Flushing any queued print jobs.");
            for (i=0; i<9; i++) iw_formfeed(i);
            //setstatusbar("Shutting down printers");
            //iw_shutdown();                             // this is too slow, so skip it
            setstatusbar("Waiting for Lisa to shut down...");
            ALERT_LOG(0,"Waiting for OS to shut down");
          }
      else
          {
            int ret;
                
            ALERT_LOG(0,"Powering on");   
            // 20071204 hack to fix windows bugs with RAWBITMAP
            #ifdef USE_RAW_BITMAP_ACCESS
              #ifdef __WXMSW__
                if (skins_on) {turn_skins_off(); turn_skins_on();}
                black();
              #endif
            #endif
    
            ret=initialize_all_subsystems();
            if  (!ret)
                {
                  my_lisawin->powerstate|= POWER_NEEDS_REDRAW | POWER_ON;
                  my_lisaframe->running=emulation_running; 
                  my_lisaframe->runtime.Start(0);
                  my_lisaframe->lastcrtrefresh=0;
                }    
            else
                  if (!romless) wxMessageBox(_T("Power on failed."), _T("Poweron Failed"), wxICON_INFORMATION | wxOK);

            if (ret>1) EXIT(999,0,"Out of memory or other fatal error.");  // out of memory or other fatal error!
            contrast=0;
            presspowerswitch();                      // send keyboard event.
          }

        my_lisawin->dirtyscreen=1;
        save_global_prefs();

      ALERT_LOG(0,"======== BEFORE UPDATE MENUS ==");
      ALERT_LOG(0,"powerstate: %d",my_lisawin->powerstate);
      ALERT_LOG(0,"running   : %d",my_lisaframe->running);
      ALERT_LOG(0,"===============================");


        my_lisaframe->UpdateProfileMenu();

      ALERT_LOG(0,"======== EXIT ================");
      ALERT_LOG(0,"powerstate: %d",my_lisawin->powerstate);
      ALERT_LOG(0,"running   : %d",my_lisaframe->running);
      ALERT_LOG(0,"===============================");

}


void quit_lisaem(void) {  wxCommandEvent foo; my_lisaframe->OnQuit(foo); }

extern "C" void lisa_powered_off(void)
{
  my_lisaframe->running=emulation_off;                // no longer running
  if  ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)!=FLOPPY_EMPTY) 
      {
        eject_floppy_animation();
        flushscreen();
      }

  if (skins_on) lightning();                         // CRT lightning
  else          black();

  unvars();                                         // reset global vars
// can't debug in windows since LisaEm is not a console app, so redirect buglog to an actual file.
// call to unvars resets this to null, so reopen it
#if defined(__WXMSW__)    && defined(DEBUG) 
  buglog=fopen("lisaem-output.txt","a+");
#endif

  my_lisaconfig->Save(pConfig, floppy_ram);         // save PRAM, configuration
  my_lisaframe->runtime.Pause();                    // pause the stopwatch

  my_lisawin->powerstate= POWER_NEEDS_REDRAW | POWER_OFF;
  flushscreen();
  ALERT_LOG(0,"The Lisa has powered off powerstate:%d",my_lisawin->powerstate);         // status

  setstatusbar("The Lisa has powered off");         // status
  flushscreen();
  iw_enddocuments();

  if (on_start_quit_on_poweroff) quit_lisaem();
}

extern "C" void free_all_ipcts(void);

extern "C" void lisa_rebooted(void)
{
  setstatusbar("The Lisa is rebooting.");
  my_lisaconfig->Save(pConfig, floppy_ram);         // save PRAM, configuration
  unvars();                                         // reset global vars and all
// can't debug in windows since LisaEm is not a console app, so redirect buglog to an actual file.
#if defined(__WXMSW__)    && defined(DEBUG) 
  buglog=fopen("lisaem-output.txt","a+");
#endif

  my_lisaframe->running=emulation_off;              // prevent init_all_subs from barking
  my_lisawin->powerstate = POWER_OFF;               // will be turned back on immediately
  handle_powerbutton();                             // back on we go.
  free_all_ipcts();
  free(lisaram);
  lisaram=NULL;
}


void LisaWin::OnMouseMove(wxMouseEvent &event)
{
    DCTYPE dc(this); // 20190831 - let's see if this is needed
    //DoPrepareDC(dc); // or this->PrepareDC(dc); this->OnDraw(dc);
    dc.SetUserScale(hidpi_scale,hidpi_scale);
    wxString str;
    int vbX=0,vbY=0;                     // scrollbar location
    mousemoved++;
    wxPoint pos = event.GetLogicalPosition(  skins_on ? *my_skinDC : *my_memDC  );
    //wxPoint pos = event->GetLogicalPosition(dc);

    // windows bug - crashes because it gets a mouse event before the window is ready.
    if (skins_on && my_skin==NULL) return;
    if (my_lisabitmap==NULL)       return;
    if (!FullScreenCheckMenuItem)  return; // incase we somehow got here before the menu was built

     // in full screen mode, show menu bar when above line. or maybe disable this and always show the menu bar, but that's fugly. :(  Grrr.
    #if (!defined(__WXOSX__)) && (!defined(SHOW_MENU_IN_FULLSCREEN))
    if (my_lisaframe->IsFullScreen() || FullScreenCheckMenuItem->IsChecked() )
    {  int menuline= _H(16);
        if  ( pos.y < menuline && last_mouse_pos_y >= menuline )
            { // this is retarded. ShowFullScreen works, but only once, need to turn it off if you want to enable menus, and that causes the window to flash.  :(
              my_lisaframe->ShowFullScreen(false, 0);
              my_lisaframe->Maximize(true);
              //my_lisaframe->ShowFullScreen(true, wxFULLSCREEN_NOBORDER);
              // 2019.09.04 even worse, now I have to get out of full screen mode entirely to display the menu. grrrr. issue with lightdm/enlightenment?
              //my_lisaframe->ShowFullScreen(true, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION    );
            }
        else if  (pos.y > menuline && last_mouse_pos_y <= menuline ) {
                  my_lisaframe->ShowFullScreen(false, wxFULLSCREEN_ALL ); my_lisaframe->ShowFullScreen(true,  wxFULLSCREEN_ALL );
                  }

        if  (FullScreenCheckMenuItem->IsChecked() && !my_lisaframe->IsFullScreen() && pos.y > menuline) {
              my_lisaframe->ShowFullScreen(false, wxFULLSCREEN_ALL );  my_lisaframe->ShowFullScreen(true,  wxFULLSCREEN_ALL );
            }
    }
    #endif

    last_mouse_pos_x=pos.x; last_mouse_pos_y=pos.y;  // not hidpi corrected - used for full screen menu pop-up

    // correct for hidpi - these are relative to the wxWindow
    long xh=  pos.x * mouse_scale;
    long yh=  pos.y * mouse_scale;

    // correct for Lisa display on CRT screen - relative to where the Lisa display should be.
    long y = yh; long x = xh;
//    if (lisa_ui_video_mode==vidmod_3y) {y=y/3; x=x/2;}  // 2X3Y
//    if (lisa_ui_video_mode==vidmod_2y) {y=y/2;       }  // double y

    // adjust mouse location to Lisa display
    if  (skins_on && !!my_skin) {
          vbX=vbX * (my_skin->GetWidth());  // /(100*hidpi_scale));
          vbY=vbY * (my_skin->GetHeight()); // /(100*hidpi_scale));
          pos.x+=vbX; pos.y+=vbY;
          xh=  pos.x;
          yh=  pos.y;
          x-=skin.screen_origin_x; // /hidpi_scale; 
          y-=skin.screen_origin_y; // /hidpi_scale; 
        }
    else
        {
          int w_width, w_height;  
          GetSize(&w_width,&w_height);
          vbX=0; //=vbX * (my_lisabitmap->GetWidth());  // /(100*hidpi_scale));
          vbY=0; //=vbY * (my_lisabitmap->GetHeight()); // /(100*hidpi_scale));
          pos.x+=vbX; pos.y+=vbY;
          xh=  pos.x;
          yh=  pos.y;
          if (lisa_ui_video_mode==vidmod_hq35x)
          {
              ox=(w_width  - _H(720) )/2; 
              oy=(w_height - (_H(364)*544/500 / 2 ) );
              //ox=(w_width  - (my_lisahq3xbitmap->GetWidth()  / (100*hidpi_scale)))/2;
              //oy=(w_height - (my_lisahq3xbitmap->GetHeight() / (100*hidpi_scale))*HQ3XYSCALE)/2;
              //ALERT_LOG(0,"o:%d,%d e_l_size:%d,%d win:%d,%d",ox,oy,effective_lisa_vid_size_x,effective_lisa_vid_size_y,w_width,w_height);
          }
          else
          {
              ox=(w_width  - o_effective_lisa_vid_size_x)/2;
              oy=(w_height - o_effective_lisa_vid_size_y)/2;
          }
          if (!skinless_center) {
              ox=0; oy=0;
          }
          x-=skin.screen_origin_x; y-=skin.screen_origin_y;
        }

    // restrict mouse to Lisa display - then translate coordinates
    int  mouse_in_crt=1;

    if (y<  0)                     {y=0;   mouse_in_crt=0;}

    if (!skins_on && lisa_ui_video_mode==vidmod_hq35x)
    {
        if (y>548)                 {y=548; mouse_in_crt=0;}
        y= screen_to_mouse_hq3x[y];
    }
    else
    {
        if (y>500)                 {y=499; mouse_in_crt=0;}
        y=screen_to_mouse[y];
    }

    if (x<0 || x>lisa_vid_size_x)  {mouse_in_crt=0;       }


    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // power animation + button
    if (skins_on)
    {
      //----------------------------------------------------------------------------------------------------------------------------------
      // draw flashing boxes around where we think CRT area, power, and floppy drives are - useful for debugging skins
//#define DEBUG_HOTBUTTONS 1
//#define DEBUG_MOUSE_LOG 1 

      #ifdef  DEBUG_HOTBUTTONS
      if (mouse_in_crt)
      {
          my_skinDC->SetPen(*wxBLUE_PEN);
          my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH);
      }
      else
      {
          my_skinDC->SetPen(*wxRED_PEN);
          my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH);
      }
      my_skinDC->DrawRectangle( _H(skin.screen_origin_x),    _H(skin.screen_origin_y),
                                  effective_lisa_vid_size_x,    effective_lisa_vid_size_y );


      // draw where we think the mouse position is
      my_skinDC->SetPen(*wxGREEN_PEN);
      my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH );
      my_skinDC->DrawRectangle( xh-2,yh-2,4,4);
      #endif
      //----------------------------------------------------------------------------------------------------------------------------------
      //----------------------------------------------------------------------------------------------------------------------------------
      //----------------------------------------------------------------------------------------------------------------------------------
      #ifdef DEBUG_MOUSE_LOG
      {
        char text[128];
        snprintf(text,127, "Mouse actual xh,yh: (%3d,%3d)  translated x,y: (%3d,%3d) display:(%3d,%3d)+(%d,%d)",xh,yh,x,y,
                            skin.screen_origin_x,skin.screen_origin_y,effective_lisa_vid_size_x,    effective_lisa_vid_size_y);
        setstatusbar(text);
      }
      #endif
      //----------------------------------------------------------------------------------------------------------------------------------

      #if defined(DEBUG) && defined(DEBUG_MOUSE_LOCATION)
      if (xh > _H(skin.power_button_tl_x) &&  xh < _H(skin.power_button_br_x)  && 
          yh > _H(skin.power_button_tl_y) &&  yh < _H(skin.power_button_br_y)  ) {
          my_skinDC->SetPen(*wxCYAN_PEN);
          my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH);
      }
      else
      {   my_skinDC->SetPen(*wxMEDIUM_GREY_PEN);
          my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH ); 
      }
      my_skinDC->DrawRectangle( _H(skin.power_button_tl_x), _H(skin.power_button_tl_y),
                                _H(skin.power_button_br_x  -   skin.power_button_tl_x), 
                                _H(skin.power_button_br_y  -   skin.power_button_tl_y)   );
      my_skinDC->DrawRectangle( skin.power_button_tl_x,  skin.power_button_tl_y ,
                              ( skin.power_button_br_x - skin.power_button_tl_x), 
                              ( skin.power_button_br_y - skin.power_button_tl_y));
      #endif

      #if defined(DEBUG) && defined(DEBUG_MOUSE_LOCATION)
       if  (xh>_H(skin.floppy2_tl_x) && xh<_H(skin.floppy2_tl_x+my_floppy0->GetWidth() )  &&
            yh>_H(skin.floppy2_tl_y) && yh<_H(skin.floppy2_tl_y+my_floppy0->GetHeight())    )  {
        my_skinDC->SetPen(*wxGREEN_PEN);
        my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH);
      }
      else
      {
        my_skinDC->SetPen(*wxYELLOW_PEN);
        my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH);
      }
      my_skinDC->DrawRectangle( _H(skin.floppy2_tl_x),      _H(skin.floppy2_tl_y),
                                _H(my_floppy0->GetWidth()), _H(my_floppy0->GetHeight() ) );
      #endif
      //----------------------------------------------------------------------------------------------------------------------------------

      /*
      ALERT_LOG(0,"h:%d,%d floppy:%d,%d-%d,%d * 1.0",(xh),(yh),
               (skin.floppy2_tl_x),(skin.floppy2_tl_y),
              ((skin.floppy2_tl_x+(my_floppy0->GetWidth())    ) ),
              ((skin.floppy2_tl_y+(my_floppy0->GetHeight())   ) )
        );

      ALERT_LOG(0,"h:%d,%d floppy:%d,%d-%d,%d * HIDPI\n",_H(xh),_H(yh),
              _H(skin.floppy2_tl_x), _H(skin.floppy2_tl_y),
              _H(skin.floppy2_tl_x+(my_floppy0->GetWidth())    ),
              _H(skin.floppy2_tl_y+(my_floppy0->GetHeight())   )
        );
      */
     // lisaem_wx.cpp:OnMouseMove:4214:sizeof int:4, long:8
     // lisaem_wx.cpp:OnMouseMove:4220:h:52,1220 floppy:1099,481-1430,672 * 1.0
     // lisaem_wx.cpp:OnMouseMove:4226:h:0,0 floppy:31,1447161952-0,1112539136 * HIDPI  <<<-- some kind of overflow happened here
     // lisaem_wx.cpp:OnMouseMove:4214:sizeof int:4, long:8                      replacing * hidpi_scale with H() everywhere because of this.
     // lisaem_wx.cpp:OnMouseMove:4220:h:38,1215 floppy:1099,481-1430,672 * 1.0
     //lisaem_wx.cpp:OnMouseMove:4217:h:46,680 floppy:1099,481-1430,672
     //                                 xh,yh         flox,y  
     //lisaem_wx.cpp:OnMouseMove:4217:h:0,0 floppy:31,1447161952-1458,1446274496
      //----------------------------------------------------------------------------------------------------------------------------------
      //----------------------------------------------------------------------------------------------------------------------------------
      //----------------------------------------------------------------------------------------------------------------------------------


      if  ( xh > skin.power_button_tl_x  &&  xh < skin.power_button_br_x  && 
            yh > skin.power_button_tl_y  &&  yh < skin.power_button_br_y    ) {
            //my_lisawin->powerstate &= POWER_ON;
            #if defined(DEBUG) && defined(DEBUG_MOUSE_LOCATION)
            ALERT_LOG(0,"hover over power button")
            #endif
            // unlike normal UI/key/mouse buttons, where the action happens when you let go of the mouse button,
            // this is as it should be, as IRL the Lisa turns on when the power button is pushed in.
            if  (event.LeftDown())    {
                  ALERT_LOG(0,"push power button %d via skin",my_lisawin->powerstate);
                  if (sound_effects_on) my_lisa_power_switch01->Play(wxSOUND_ASYNC);
                  my_lisawin->powerstate |= POWER_NEEDS_REDRAW | POWER_PUSHED;
                  ALERT_LOG(0,"after push power button  (|=POWER_NEEDS_REDRAW|POWER_PUSHED) %d",my_lisawin->powerstate);
                  handle_powerbutton();
                  ALERT_LOG(0,"after handle_powerbutton() %d",my_lisawin->powerstate);
                  flushscreen();
                }
            if  (event.LeftUp()) {
                  ALERT_LOG(0,"release power button %d",my_lisawin->powerstate);
                  if (sound_effects_on) my_lisa_power_switch02->Play(wxSOUND_ASYNC);
                  my_lisawin->powerstate |= POWER_NEEDS_REDRAW;
                  ALERT_LOG(0,"release power button |=POWER_NEEDS_REDRAW %d",my_lisawin->powerstate);
                  flushscreen();   
            }
      }

      if  (!!my_floppy0) {
            if (xh>skin.floppy2_tl_x && xh<skin.floppy2_tl_x+(my_floppy0->GetWidth()   )  &&
                yh>skin.floppy2_tl_y && yh<skin.floppy2_tl_y+(my_floppy0->GetHeight()  )  ) {
                      //ALERT_LOG(0,"\n  Hover over floppy %d,%d-%d,%d",skin.floppy2_tl_x,skin.floppy2_tl_y, 
                      //                                                skin.floppy2_tl_x+my_floppy0->GetWidth(),
                      //                                                skin.floppy2_tl_y+my_floppy0->GetHeight() );
                      if (event.LeftDown())   my_lisaframe->OnxFLOPPY();
                      if (event.RightDown())  my_lisaframe->OnxNewFLOPPY();
            }
      }

  } // end if (skins_on)  power animation + button
    //----------------------------------------------------------------------------------------------------------------------------------

#ifdef DEBUG_MOUSE_LOG
      {
        char text[128];
        snprintf(text,127, "Mouse actual xh,yh: (%3d,%3d)  translated x,y: (%3d,%3d) display:(%3d,%3d)+(%d,%d)",xh,yh,x,y,
                            skin.screen_origin_x,skin.screen_origin_y,effective_lisa_vid_size_x,    effective_lisa_vid_size_y);
        setstatusbar(text);
      }
#endif

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


    // hide host mouse pointer when running and over Lisa CRT display, so we can use Lisa's mouse sprite
    if  (mouse_in_crt && my_lisaframe->running)
        {
            static long lastup;
            long now=my_lisaframe->runtime.Time();                              // tee hee, the long now.
            int lu=event.LeftUp();
            #if defined(__WXX11__) || defined(__WXGTK__) || defined(__WXOSX__)
              if (hide_host_mouse) SetCursor(wxCURSOR_BLANK);
              else                 SetCursor(*m_dot_cursor);
            #else
              if (hide_host_mouse) SetCursor(wxCURSOR_BLANK);
              else                 SetCursor(wxCURSOR_CROSS);
            #endif
          //----------------------------------------------------------------------------------------------
            #ifdef DEBUG
              if (event.LeftDown() && debug_log_onclick)
                {
                  debug_log_onclick=0;
                  debug_log_enabled=!debug_log_enabled;
                  if (debug_log_enabled) debug_on("onclick");
                  else                   debug_off();
                }

                if (my_lisaframe->running==emuation_paused_for_screen)
                {
                    SetCursor(wxCURSOR_CROSS);
                    if (event.LeftDown()==1)
                    {
                      box_x=x;   box_y=y;
                      box_xh=xh; box_yh=yh;
                    }

                    if (box_x!=-1)
                    {
                      my_skinDC->SetPen(*wxRED_PEN);
                      my_skinDC->SetBrush(*wxTRANSPARENT_BRUSH);
                      my_skinDC->DrawRectangle( box_xh, box_yh, abs(box_xh-xh),abs(box_yh-yh));
                      flushscreen();

                      ALERT_LOG(0,"%ld,%ld - %ld,%ld",(long)box_xh,(long)box_yh,(long)xh,(long)yh);
                      videoramdirty=32768; //my_lisaframe->Refresh(false,NULL);
                    }

                    if (event.LeftUp()==1) 
                    {
                      log_screen_box(stderr, MIN(x,box_x), MIN(y,box_y), MAX(x,box_x), MAX(y,box_y) );
                      box_x=-1; box_y=-1;
                      resume_run();
                    }
                } //--- end if (my_lisaframe->running==emuation_paused_for_screen)
            #endif
          //----------------------------------------------------------------------------------------------


          // not sure what these will do or even if they're supported. if they do nothing in LOS try 4,5,6
          // may also need to do the double click hack below for each of them too.
          //if      (event.RightDown())   {add_mouse_event(x,y, 2); ALERT_LOG(0,"Sending right click down"); }
          //else if (event.RightUp())     {add_mouse_event(x,y,-2); ALERT_LOG(0,"Sending right click up");   }
          //else if (event.MiddleDown())  {add_mouse_event(x,y, 3); ALERT_LOG(0,"Sending middle click down");}
          //else if (event.MiddleUp())    {add_mouse_event(x,y,-3); ALERT_LOG(0,"Sending middle click up");  }
          //else                          
          {add_mouse_event(x,y, lu ? -1:  (event.LeftDown() ? 1:0) );        }
          seek_mouse_event();

          // double click hack  - fixme BUG BUG BUG - fixme - well timing bug, will not be fixed if 32Mhz is allowed
          if  (lu)
              {
                if  (now-lastup<1500)
                    {
                      add_mouse_event(x,y,  1); add_mouse_event(x,y, -1);
                      add_mouse_event(x,y,  1); add_mouse_event(x,y, -1);
                    }
                lastup=now;
              }
    }
    else //    if  (mouse_in_crt && my_lisaframe->running)
            my_lisawin->SetCursor(wxNullCursor);

    if  (event.RightDown())   // force screen refresh, and also for debugging
        {
          videoramdirty=32768; flushscreen(); 
          #if defined(DEBUG) && defined(RIGHT_CLICK_TRACELOG)
            debug_log_onclick=0;
            debug_log_enabled=!debug_log_enabled;
            if (debug_log_enabled) debug_on("right-click");
            else                   debug_off();
          #endif
        }

//#define DEBUG_MOUSE_LOCATION
    // aka mouse droppings
    #if defined(DEBUG) && defined(DEBUG_MOUSE_LOCATION)
      my_skinDC->SetPen(*wxRED_PEN);
      my_skinDC->SetBrush(*wxRED_BRUSH);
      //my_skinDC->DrawRectangle( xh      * hidpi_scale, yh      * hidpi_scale,
      //                          5       * hidpi_scale,  5      * hidpi_scale                );
      my_skinDC->DrawRectangle( xh, yh, 2, 2);
    #endif

//#define DEBUG_MOUSE_COORDINATES 1

    #ifdef DEBUG_MOUSE_COORDINATES
    {
        int vx,vy;
        GetViewStart(&vx,&vy);
        ALERT_LOG(0,"mouse: %d,%d\n  in CRT:%d,%d  skinorigin:%d,%d   hidpi_scale:%f\n  pos:%d,%d",
                              xh,yh, x,y,
                              skin.screen_origin_x,skin.screen_origin_y, 
                              hidpi_scale,
                              pos.x, pos.y);
    }
    #endif
}





extern "C" void pause_run(void)
{
    if (my_lisaframe->running==emulation_off) return;

    if (my_lisaframe->running==emulation_running)
    {
        my_lisaframe->runtime.Pause();
        my_lisaframe->running=emulation_paused;
        return;
    }
}

#ifdef DEBUG
extern "C" void pause_for_screen(void)
{
    if (my_lisaframe->running==emulation_off) return;

    if (my_lisaframe->running==emulation_running)
    {
        my_lisaframe->runtime.Pause();
        my_lisaframe->running=emuation_paused_for_screen;
        return;
    }

}
#endif


extern "C" void resume_run(void)
{
    if (my_lisaframe->running==emulation_off) return;

    if (my_lisaframe->running==emulation_paused || my_lisaframe->running==emuation_paused_for_screen)
    {
        my_lisaframe->runtime.Resume();
        my_lisaframe->running=emulation_running;
        return;
    }
}


void LisaEmFrame::OnPause(wxCommandEvent& WXUNUSED(event))
{
    ALERT_LOG(0,"Pause, status before change is:%d",my_lisaframe->running);
    if (my_lisaframe->running==emulation_off)      {                                          return;}
    if (my_lisaframe->running==emulation_running)  {pause_run();   update_menu_checkmarks();  return;}
    if (my_lisaframe->running==emulation_paused)   {resume_run();  update_menu_checkmarks();  return;}
}


#ifdef DEBUG
void LisaEmFrame::OnScreenRegion(wxCommandEvent& WXUNUSED(event))
{
    if (my_lisaframe->running==emulation_off)                {                                return;}
    if (my_lisaframe->running==emulation_running)            {pause_for_screen();             return;}
    if (my_lisaframe->running==emuation_paused_for_screen)   {resume_run();                   return;}
}
#endif


void LisaEmFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxAboutDialogInfo info;
    info.SetName(_T("LisaEm"));

    #ifdef VERSION
    info.SetVersion(_T(VERSION));
    #else
    info.SetVersion(_T("1.x.x Unknown"));
    #endif

    info.SetDescription(_T("The first fully functional Apple Lisa emulator."));
    info.SetCopyright(_T("\xa9 2007, 2020 Ray Arachelian"));
    info.SetWebSite(_T("http://lisaem.sunder.net"));

//#ifdef BUILTBY
    info.AddDeveloper(_T("\n\n" BUILTBY));
//#endif

    info.AddDeveloper(_T("Ray Arachelian - Emulator"));
    info.AddDeveloper(_T("James Ponder - 68K core.\n"));
    info.AddDeveloper(_T("Maxim Stepin, Cameron Zemek, Francois Gannaz - hqx") );

    #ifdef LICENSE
    info.SetLicense(_T(LICENSE));
    #endif
    
    wxAboutBox(info);
}



//#ifdef __WXOSX__
void LisaEmFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
//#else
//void LisaEmApp::OnQuit(wxCommandEvent& WXUNUSED(event))
//#endif
{
    save_global_prefs();

    EXTERMINATE(my_lisabitmap          );
    EXTERMINATE(my_memDC               );
    //EXTERMINATE(my_lisa_sound          );
    EXTERMINATE(my_floppy_eject        );
    EXTERMINATE(my_floppy_insert       );
    EXTERMINATE(my_floppy_motor1       );
    EXTERMINATE(my_floppy_motor2       );
    EXTERMINATE(my_lisa_power_switch01 );
    EXTERMINATE(my_lisa_power_switch02 );
    EXTERMINATE(my_poweroffclk         );
    EXTERMINATE(my_skin                );
    EXTERMINATE(my_skin0               );
    EXTERMINATE(my_skin1               );
    EXTERMINATE(my_skin2               );
    EXTERMINATE(my_skin3               );
    EXTERMINATE(my_floppy0             );
    EXTERMINATE(my_floppy1             );
    EXTERMINATE(my_floppy2             );
    EXTERMINATE(my_floppy3             );
    EXTERMINATE(my_floppy4             );
    EXTERMINATE(my_poweron             );
    EXTERMINATE(my_poweroff            );
    EXTERMINATE(my_skinDC              );
    EXTERMINATE(my_floppy0DC           );
    EXTERMINATE(my_floppy1DC           );
    EXTERMINATE(my_floppy2DC           );
    EXTERMINATE(my_floppy3DC           );
    EXTERMINATE(my_floppy4DC           );
    EXTERMINATE(my_poweronDC           );
    EXTERMINATE(my_poweroffDC          );

    #ifdef __WXOSX__
     my_lisaframe->m_emulation_timer->Stop();     // stop the timer
     wxMilliSleep(emulation_time*2);              // ensure that any pending timer events are allowed to finish
     delete my_lisaframe->m_emulation_timer;      // delete the timer
     if (my_LisaConfigFrame) // close any ConfigFrame 
        {
          my_LisaConfigFrame->Hide();
          my_LisaConfigFrame->Close();
          delete my_LisaConfigFrame; my_LisaConfigFrame=NULL;
        }  
     Close();                                      // and bye bye we go.
    #else
     my_lisaframe->m_emulation_timer->Stop();      // stop the timer
     if (my_LisaConfigFrame)                       // close any ConfigFrame 
        {
          my_LisaConfigFrame->Hide();
          my_LisaConfigFrame->Close();
          delete my_LisaConfigFrame; my_LisaConfigFrame=NULL;
        }  
      wxMilliSleep(750);
      EXTERMINATE(my_lisaframe->m_emulation_timer);
      wxMilliSleep(250);
      EXTERMINATE(my_lisaframe);
      wxExit();
    #endif
}

extern "C" void on_lisa_exit(void) {
  wxCommandEvent foo;
  my_lisaframe->OnQuit(foo);
}


void LisaEmFrame::OnLisaWeb(  wxCommandEvent& WXUNUSED(event)) {::wxLaunchDefaultBrowser(_T("https://lisaem.sunder.net" ));}
void LisaEmFrame::OnLisaFaq(  wxCommandEvent& WXUNUSED(event)) {::wxLaunchDefaultBrowser(_T("https://lisafaq.sunder.net"));}
void LisaEmFrame::OnLisaList2(wxCommandEvent& WXUNUSED(event)) {::wxLaunchDefaultBrowser(_T("https://lisalist2.com"     ));}


void LisaEmFrame::OnConfig(wxCommandEvent& WXUNUSED(event))
{
      EXTERMINATE(my_LisaConfigFrame);
          my_LisaConfigFrame=new LisaConfigFrame( wxT("Preferences"), my_lisaconfig);
          #if defined(__WXMOTIF__)
          int x, y;

          GetSize(&x, &y);
          x+=WINXDIFF; y+=WINYDIFF;
          SetSize(wxDefaultCoord, wxDefaultCoord,x,y);
          #endif

          my_LisaConfigFrame->Show();
}


void LisaEmFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
      wxString openfile;

      wxFileDialog open(this,         wxT("Open LisaEm Preferences:"),
                                      wxEmptyString,
                                      wxEmptyString,
                                      wxT("LisaEm Preferences (*.lisaem)|*.lisaem|All (*.*)|*.*"),
                                      (long int)wxFD_OPEN,wxDefaultPosition);

      if (open.ShowModal()==wxID_OK)  openfile=open.GetPath();
      else return;

      if (pConfig) {pConfig->Flush();  EXTERMINATE(pConfig);}
      if (my_LisaConfigFrame) {EXTERMINATE(my_LisaConfigFrame);}

      pConfig=new wxFileConfig(_T("LisaEm"),
                              _T("sunder.NET"),
                                (openfile),     //local
                                (openfile),     //global
                                wxCONFIG_USE_LOCAL_FILE,
                                wxConvAuto() );   // or wxConvUTF8

      pConfig->Get(true);
      EXTERMINATE(my_lisaconfig);
      my_lisaconfig = new LisaConfig();
      my_lisaconfig->Load(pConfig, floppy_ram);// load it in

      myconfigfile=openfile;  // update global file to point to new config
      save_global_prefs();    // and save them.
}


void LisaEmFrame::OnSaveAs(wxCommandEvent& WXUNUSED(event))
{
      wxString savefile;

      wxFileName prefs=wxFileName(myconfigfile);
      wxString justTheFilename=prefs.GetFullName();
      wxString justTheDir=prefs.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR,wxPATH_NATIVE);

      wxFileDialog open(this,          wxT("Save LisaEm Preferences As:"),
                                       justTheDir,  // path
                                       justTheFilename,  // prefs file name
                                        wxT("LisaEm Preferences (*.lisaem)|*.lisaem|All (*.*)|*.*"),
                                        (long int)wxFD_SAVE,wxDefaultPosition
                        );

      if (open.ShowModal()==wxID_OK)   savefile=open.GetPath();
      else return;

      wxFileOutputStream out(savefile);

      if  (pConfig->Save(out,wxConvUTF8) )
          {
            myconfigfile=savefile;
            save_global_prefs();
          }
}



void LisaEmFrame::OnAPPLEPOWERKEY(wxCommandEvent& WXUNUSED(event))
    {
      send_cops_keycode(KEYCODE_COMMAND|KEY_DOWN);
      handle_powerbutton();
      send_cops_keycode(KEYCODE_COMMAND|KEY_UP);
    }

#ifdef DEBUG

void LisaEmFrame::LisaEmFrame::DumpAllScreenshot(wxCommandEvent& WXUNUSED(event)) {dumpallscreenshot();}

void LisaEmFrame::OnTraceLog(wxCommandEvent& WXUNUSED(event))
    {
      debug_log_onclick=0;
      debug_log_enabled=!debug_log_enabled;
      if (debug_log_enabled) debug_on("user enabled");
      else                   debug_off();
    }

void LisaEmFrame::OnTraceLog2(wxCommandEvent& WXUNUSED(event))           { debug_log_onclick=1;                   }
#endif

void LisaEmFrame::OnKEY_APL_DOT(wxCommandEvent& WXUNUSED(event))         { apple_dot();                           }
void LisaEmFrame::OnKEY_APL_S(wxCommandEvent& WXUNUSED(event))           { apple_S();                             }
void LisaEmFrame::OnKEY_APL_ENTER(wxCommandEvent& WXUNUSED(event))       { apple_enter();                         }
void LisaEmFrame::OnKEY_APL_RENTER(wxCommandEvent& WXUNUSED(event))      { apple_renter();                        }
void LisaEmFrame::OnKEY_APL_1(wxCommandEvent& WXUNUSED(event))           { apple_1();                             }
void LisaEmFrame::OnKEY_APL_2(wxCommandEvent& WXUNUSED(event))           { apple_2();                             }
void LisaEmFrame::OnKEY_APL_3(wxCommandEvent& WXUNUSED(event))           { apple_3();                             }

void LisaEmFrame::OnKEY_OPT_0(wxCommandEvent& WXUNUSED(event))           { shift_option_0();                      }
void LisaEmFrame::OnKEY_OPT_4(wxCommandEvent& WXUNUSED(event))           { shift_option_4();                      }
void LisaEmFrame::OnKEY_OPT_7(wxCommandEvent& WXUNUSED(event))           { shift_option_7();                      }
void LisaEmFrame::OnKEY_NMI(wxCommandEvent& WXUNUSED(event))             { send_nmi_key();                        }

void LisaEmFrame::OnASCIIKB(wxCommandEvent& WXUNUSED(event))             { asciikeyboard= 1; save_global_prefs(); }
void LisaEmFrame::OnRAWKB(wxCommandEvent& WXUNUSED(event))               { asciikeyboard= 0; save_global_prefs(); }
void LisaEmFrame::OnRAWKBBUF(wxCommandEvent& WXUNUSED(event))            { asciikeyboard=-1; save_global_prefs(); }

// not yet implemented :(
void LisaEmFrame::OnDebugger(wxCommandEvent& WXUNUSED(event))            {                                        }

void LisaEmFrame::OnPOWERKEY(wxCommandEvent& WXUNUSED(event))            { handle_powerbutton();                  }


// display a floating keyboard and allow keymap edits - not yet implemented. :(
void LisaEmFrame::OnKEYBOARD(wxCommandEvent& WXUNUSED(event))            {                                        }

void LisaEmFrame::OnKEY_RESET(wxCommandEvent& WXUNUSED(event))
{
    if (!running)
    {
        wxMessageBox( wxT("The Lisa isn't powered.  Pressing RESET now won't do anything interesting."),
                      wxT("Lisa isn't powered on"), wxICON_INFORMATION | wxOK);
        return;
    }

    if (yesnomessagebox(  "This will reboot the Lisa, but may cause file system corruption",
                          "Really RESET the Lisa?")==0) return;

    lisa_rebooted();
}


// This is necessary since if we change the throttle, the OnIdle
// execution loop needs a new reference point, else it will try
// to adjust the runtime from power on which will not work.
void LisaEmFrame::reset_throttle_clock(void)
{
   cpu68k_reference=cpu68k_clocks;
   last_runtime_sample=0;
   lastcrtrefresh=0;
   runtime.Start(0);
   if    (throttle  ==10000000)
          clockfactor=0;
   else   clockfactor=1.0/(throttle*1000.0);
   cycles_wanted=(XTIMER)(float)(emulation_time/clockfactor);
}


void LisaEmFrame::OnET100_75(wxCommandEvent& WXUNUSED(event))   {emulation_time=100; emulation_tick=75; reset_throttle_clock(); save_global_prefs();}
void LisaEmFrame::OnET50_30( wxCommandEvent& WXUNUSED(event))   {emulation_time= 50; emulation_tick=30; reset_throttle_clock(); save_global_prefs();}
void LisaEmFrame::OnET40_25( wxCommandEvent& WXUNUSED(event))   {emulation_time= 40; emulation_tick=25; reset_throttle_clock(); save_global_prefs();}
void LisaEmFrame::OnET30_20( wxCommandEvent& WXUNUSED(event))   {emulation_time= 30; emulation_tick=20; reset_throttle_clock(); save_global_prefs();}

#define Throttle_MENU(XspeedX)                                           \
  void LisaEmFrame::OnThrottle##XspeedX(wxCommandEvent &WXUNUSED(event)) \
    {  throttle = XspeedX; reset_throttle_clock(); save_global_prefs(); update_menu_checkmarks(); ALERT_LOG(0,"Set CPU to %f MHz",throttle);}

Throttle_MENU(5);
Throttle_MENU(8);
Throttle_MENU(10);
Throttle_MENU(12);
Throttle_MENU(16);
Throttle_MENU(32);
Throttle_MENU(48);
Throttle_MENU(64);
Throttle_MENU(100);
Throttle_MENU(128);
Throttle_MENU(256);
#ifdef DEBUG
Throttle_MENU(1);
Throttle_MENU(512);
#endif

extern "C" void messagebox(char *s, char *t)  // messagebox string of text, title
{
    ALERT_LOG(0,"%s:%s",t,s);  // this works, but the conversion below does not.
    wxString text=""; text << s;
    wxString title=""; title << t;
    wxMessageBox(text,title, wxICON_INFORMATION | wxOK);
}

extern "C" int yesnomessagebox(char *s, char *t)  // messagebox string of text, title
{
    ALERT_LOG(0,"%s:%s",t,s);  // this works, but the conversion below does not.
    wxString text=""; text << s;
    wxString title=""; title << t;
    wxMessageDialog w(my_lisawin,text,title, wxICON_QUESTION  | wxYES_NO |wxNO_DEFAULT,wxDefaultPosition );
    return (w.ShowModal()==wxID_YES);
}


// Yes, I know this is a global C function and I'm not doing it the C++ way! 
// this way all the menus get updated from a single place whenever needed,
// not including the profile menu here as that's special
// Byte my shiny metal C function!
void update_menu_checkmarks(void)
{
  static float lastthrottle;

  if  (!!DisplayMenu)
      {
        DisplayMenu->Enable(ID_VID_AA,   lisa_ui_video_mode != 0x3a);
        DisplayMenu->Enable(ID_VID_AAG,  lisa_ui_video_mode != 0x3a);
        DisplayMenu->Enable(ID_VID_HQ35X,lisa_ui_video_mode != 0x3a);
        DisplayMenu->Enable(ID_VID_DY,   lisa_ui_video_mode != 0x3a);
        DisplayMenu->Enable(ID_VID_SY,   lisa_ui_video_mode != 0x3a);
        DisplayMenu->Enable(ID_VID_2X3Y, lisa_ui_video_mode != 0x3a);

        DisplayMenu->Check(ID_VID_AA,    (lisa_ui_video_mode == vidmod_aa)  );
        DisplayMenu->Check(ID_VID_AAG,   (lisa_ui_video_mode == vidmod_aag) );
        DisplayMenu->Check(ID_VID_HQ35X, (lisa_ui_video_mode == vidmod_hq35x) );

        DisplayMenu->Check(ID_VID_DY,    (lisa_ui_video_mode == vidmod_2y)  );
        DisplayMenu->Check(ID_VID_SY,    (lisa_ui_video_mode == vidmod_raw) );
        DisplayMenu->Check(ID_VID_2X3Y,  (lisa_ui_video_mode == vidmod_3y)  );
      //DisplayMenu->Check(ID_VID_SCALED,     lisa_ui_video_mode==5);  DisplayMenu->Enable(ID_VID_SCALED,mode!=0x3a);

        DisplayMenu->Check(ID_VID_SKINS, !!skins_on);
        DisplayMenu->Check(ID_VID_SKINLESSCENTER, !!skinless_center);

        if (!!my_lisaframe)
        {
            fileMenu->Check(ID_PAUSE, (my_lisaframe->running == emulation_paused) );
            DisplayMenu->Check(ID_HIDE_HOST_MOUSE,!!hide_host_mouse);
            DisplayMenu->Check(ID_VID_FULLSCREEN, (my_lisaframe->IsFullScreen()));
        }

        if (!!DisplayScaleSub)
        {
          switch( (int)(hidpi_scale * 100)) {
            case  25: DisplayScaleSub->Check(ID_VID_SCALE_25,  true); break;
            case  50: DisplayScaleSub->Check(ID_VID_SCALE_50,  true); break;
            case  75: DisplayScaleSub->Check(ID_VID_SCALE_75,  true); break;
            case 125: DisplayScaleSub->Check(ID_VID_SCALE_125, true); break;
            case 150: DisplayScaleSub->Check(ID_VID_SCALE_150, true); break;
            case 175: DisplayScaleSub->Check(ID_VID_SCALE_175, true); break;
            case 200: DisplayScaleSub->Check(ID_VID_SCALE_200, true); break;  
            case 225: DisplayScaleSub->Check(ID_VID_SCALE_225, true); break;
            case 250: DisplayScaleSub->Check(ID_VID_SCALE_250, true); break;
            case 275: DisplayScaleSub->Check(ID_VID_SCALE_275, true); break;
            case 300: DisplayScaleSub->Check(ID_VID_SCALE_300, true); break;
            default:  DisplayScaleSub->Check(ID_VID_SCALE_100, true); break;
          }

        }

      if (!!DisplayRefreshSub && !!my_lisaframe)
      {
          DisplayRefreshSub->Check(ID_REFRESH_4Hz, refresh_rate==9*REFRESHRATE);  if (refresh_rate==9*REFRESHRATE) my_lisaframe->hostrefresh=1000/ 4;
          DisplayRefreshSub->Check(ID_REFRESH_8Hz, refresh_rate==7*REFRESHRATE);  if (refresh_rate==7*REFRESHRATE) my_lisaframe->hostrefresh=1000/ 8;
          DisplayRefreshSub->Check(ID_REFRESH_12Hz,refresh_rate==5*REFRESHRATE);  if (refresh_rate==5*REFRESHRATE) my_lisaframe->hostrefresh=1000/12;
          DisplayRefreshSub->Check(ID_REFRESH_20Hz,refresh_rate==3*REFRESHRATE);  if (refresh_rate==3*REFRESHRATE) my_lisaframe->hostrefresh=1000/20;
          DisplayRefreshSub->Check(ID_REFRESH_60Hz,refresh_rate==1*REFRESHRATE);  if (refresh_rate==1*REFRESHRATE) my_lisaframe->hostrefresh=1000/60;
      }

      #ifdef DEBUG
      #ifdef TRACE
      if (!!fileMenu)
      {
          fileMenu->Check(ID_DEBUG, !!debug_log_enabled);
          fileMenu->Check(ID_DEBUG2,!!debug_log_onclick);
      }
      #endif
      #endif
    }

    if (!!fileMenu)
    {
      keyMenu->Check(ID_ASCIIKB,  asciikeyboard== 1);
      keyMenu->Check(ID_RAWKB,    asciikeyboard== 0);
      keyMenu->Check(ID_RAWKBBUF, asciikeyboard==-1);
    }

//    updateThrottleMenus(my_lisaframe->throttle);

    if (!!my_lisaframe && !!throttleMenu) 
    { 
        if (lastthrottle!=my_lisaframe->throttle)
            {
              my_lisaframe->reset_throttle_clock();
              lastthrottle=my_lisaframe->throttle;
              #ifdef TIE_VIA_TIMER_TO_HOST
              via_throttle_factor=throttle/5.0;
              #endif
            }

        #ifdef DEBUG
            throttleMenu->Check(ID_THROTTLEX,   my_lisaframe->throttle == 512 );
            throttleMenu->Check(ID_THROTTLE1,   my_lisaframe->throttle ==  1.0);
        #else
            if (my_lisaframe->throttle>256.0) my_lisaframe->throttle=256.0;    
        #endif

            throttleMenu->Check(ID_THROTTLE5,    my_lisaframe->throttle ==  5.0);
            throttleMenu->Check(ID_THROTTLE8,    my_lisaframe->throttle ==  8.0);
            throttleMenu->Check(ID_THROTTLE10,   my_lisaframe->throttle == 10.0);
            throttleMenu->Check(ID_THROTTLE12,   my_lisaframe->throttle == 12.0);
            throttleMenu->Check(ID_THROTTLE16,   my_lisaframe->throttle == 16.0);
            throttleMenu->Check(ID_THROTTLE32,   my_lisaframe->throttle == 32.0);
            throttleMenu->Check(ID_THROTTLE48,   my_lisaframe->throttle == 48.0);
            throttleMenu->Check(ID_THROTTLE64,   my_lisaframe->throttle == 64.0);
            throttleMenu->Check(ID_THROTTLE100,  my_lisaframe->throttle == 100.0);
            throttleMenu->Check(ID_THROTTLE128,  my_lisaframe->throttle == 128.0);
            throttleMenu->Check(ID_THROTTLE256,  my_lisaframe->throttle == 256.0);

            throttleMenu->Check(ID_ET100_75,     emulation_time==100 && emulation_tick==75);
            throttleMenu->Check(ID_ET50_30,      emulation_time== 50 && emulation_tick==30);
            throttleMenu->Check(ID_ET40_25,      emulation_time== 40 && emulation_tick==25);
            throttleMenu->Check(ID_ET30_20,      emulation_time== 30 && emulation_tick==20);
    }


} // -- end of update_menu_checkmarks(void)



void LisaEmFrame::UpdateProfileMenu(void)
{
  if (!profileMenu)   return;
  if (!my_lisaconfig) return;
  // s=paralell port, 3=s1h, 4=s1l, 5=s2h,6=s2l,7=s3h,8=s3l

  profileMenu->Check(ID_PROFILEPWR,     (IS_PARALLEL_PORT_ENABLED(2) && !my_lisaconfig->parallel.IsSameAs(_T("NOTHING"), false)  ));
                                           
  profileMenu->Check(ID_PROFILE_S1U,    (IS_PARALLEL_PORT_ENABLED(3) && (my_lisaconfig->slot1.IsSameAs(_T("dualparallel"),false)) && !my_lisaconfig->s1h.IsSameAs(_T("NOTHING"), false)  ));
  profileMenu->Check(ID_PROFILE_S1L,    (IS_PARALLEL_PORT_ENABLED(4) && (my_lisaconfig->slot1.IsSameAs(_T("dualparallel"),false)) && !my_lisaconfig->s1l.IsSameAs(_T("NOTHING"), false)  ));
  profileMenu->Check(ID_PROFILE_S2U,    (IS_PARALLEL_PORT_ENABLED(5) && (my_lisaconfig->slot2.IsSameAs(_T("dualparallel"),false)) && !my_lisaconfig->s2h.IsSameAs(_T("NOTHING"), false)  ));
  profileMenu->Check(ID_PROFILE_S2L,    (IS_PARALLEL_PORT_ENABLED(6) && (my_lisaconfig->slot2.IsSameAs(_T("dualparallel"),false)) && !my_lisaconfig->s2l.IsSameAs(_T("NOTHING"), false)  ));
  profileMenu->Check(ID_PROFILE_S3U,    (IS_PARALLEL_PORT_ENABLED(7) && (my_lisaconfig->slot3.IsSameAs(_T("dualparallel"),false)) && !my_lisaconfig->s3h.IsSameAs(_T("NOTHING"), false)  ));
  profileMenu->Check(ID_PROFILE_S3L,    (IS_PARALLEL_PORT_ENABLED(8) && (my_lisaconfig->slot3.IsSameAs(_T("dualparallel"),false)) && !my_lisaconfig->s3l.IsSameAs(_T("NOTHING"), false)  ));

  profileMenu->Enable(ID_PROFILEPWR,( !my_lisaconfig->parallel.IsSameAs(_T("NOTHING"), false) ));
  if ( my_lisaconfig->parallel.IsSameAs(_T("PROFILE"), false) )  
     profileMenu->SetLabel(ID_PROFILEPWR,wxT("Power ProFile Parallel Port"));
  else
     profileMenu->SetLabel(ID_PROFILEPWR,wxT("Power ADMP on Parallel Port"));

// GLOBAL(int,via_port_idx_bits[],{0, 2,1, 4,3, 6,5});
                              //2, 4,3, 6,5, 8,7

  if (my_lisaconfig->slot1.IsSameAs(_T("dualparallel"),false))
  {
    if (my_lisaconfig->s1h.IsSameAs(_T("PROFILE"), false)) profileMenu->SetLabel(ID_PROFILE_S1U,wxT("Power ProFile on Slot 1 Upper Port"));
    if (my_lisaconfig->s1l.IsSameAs(_T("PROFILE"), false)) profileMenu->SetLabel(ID_PROFILE_S1L,wxT("Power ProFile on Slot 1 Lower Port"));

    if (my_lisaconfig->s1h.IsSameAs(_T("ADMP"), false)) profileMenu->SetLabel(ID_PROFILE_S1U,wxT("Power ADMP on Slot 1 Upper Port"));
    if (my_lisaconfig->s1l.IsSameAs(_T("ADMP"), false)) profileMenu->SetLabel(ID_PROFILE_S1L,wxT("Power ADMP on Slot 1 Lower Port"));

    if (my_lisaconfig->s1h.IsSameAs(_T("NOTHING"), false)) profileMenu->SetLabel(ID_PROFILE_S1U,wxT("Nothing on Slot 1 Upper Port"));
    if (my_lisaconfig->s1l.IsSameAs(_T("NOTHING"), false)) profileMenu->SetLabel(ID_PROFILE_S1L,wxT("Nothing on Slot 1 Lower Port"));


    profileMenu->Enable(ID_PROFILE_S1U,( !my_lisaconfig->s1h.IsSameAs(_T("NOTHING"), false)) );      
    profileMenu->Enable(ID_PROFILE_S1L,( !my_lisaconfig->s1l.IsSameAs(_T("NOTHING"), false)) );
  }
  else 
  {
    profileMenu->Enable(ID_PROFILE_S1U,false );      
    profileMenu->Enable(ID_PROFILE_S1L,false );
    profileMenu->SetLabel(ID_PROFILE_S1U,_T("No Card"));
    profileMenu->SetLabel(ID_PROFILE_S1L,_T("No Card"));
  }


  if (my_lisaconfig->slot2.IsSameAs(_T("dualparallel"),false))
  {
    if (my_lisaconfig->s2h.IsSameAs(_T("PROFILE"), false)) profileMenu->SetLabel(ID_PROFILE_S2U,wxT("Power ProFile on Slot 2 Upper Port"));
    if (my_lisaconfig->s2l.IsSameAs(_T("PROFILE"), false)) profileMenu->SetLabel(ID_PROFILE_S2L,wxT("Power ProFile on Slot 2 Lower Port"));

    if (my_lisaconfig->s2h.IsSameAs(_T("ADMP"), false)) profileMenu->SetLabel(ID_PROFILE_S2U,wxT("Power ADMP on Slot 2 Upper Port"));
    if (my_lisaconfig->s2l.IsSameAs(_T("ADMP"), false)) profileMenu->SetLabel(ID_PROFILE_S2L,wxT("Power ADMP on Slot 2 Lower Port"));

    if (my_lisaconfig->s2h.IsSameAs(_T("NOTHING"), false)) profileMenu->SetLabel(ID_PROFILE_S2U,wxT("Nothing on Slot 2 Upper Port"));
    if (my_lisaconfig->s2l.IsSameAs(_T("NOTHING"), false)) profileMenu->SetLabel(ID_PROFILE_S2L,wxT("Nothing on Slot 2 Lower Port"));


    profileMenu->Enable(ID_PROFILE_S2U,( !my_lisaconfig->s2h.IsSameAs(_T("NOTHING"), false)) );      
    profileMenu->Enable(ID_PROFILE_S2L,( !my_lisaconfig->s2l.IsSameAs(_T("NOTHING"), false)) );
  }
  else 
  {
    profileMenu->Enable(ID_PROFILE_S2U,false );      
    profileMenu->Enable(ID_PROFILE_S2L,false );
    profileMenu->SetLabel(ID_PROFILE_S2U,_T("No Card"));
    profileMenu->SetLabel(ID_PROFILE_S2L,_T("No Card"));
  }


  if (my_lisaconfig->slot3.IsSameAs(_T("dualparallel"),false))
  {
    if (my_lisaconfig->s3h.IsSameAs(_T("PROFILE"), false)) profileMenu->SetLabel(ID_PROFILE_S3U,wxT("Power ProFile on Slot 3 Upper Port"));
    if (my_lisaconfig->s3l.IsSameAs(_T("PROFILE"), false)) profileMenu->SetLabel(ID_PROFILE_S3L,wxT("Power ProFile on Slot 3 Lower Port"));

    if (my_lisaconfig->s3h.IsSameAs(_T("ADMP"), false)) profileMenu->SetLabel(ID_PROFILE_S3U,wxT("Power ADMP on Slot 3 Upper Port"));
    if (my_lisaconfig->s3l.IsSameAs(_T("ADMP"), false)) profileMenu->SetLabel(ID_PROFILE_S3L,wxT("Power ADMP on Slot 3 Lower Port"));

    if (my_lisaconfig->s3h.IsSameAs(_T("NOTHING"), false)) profileMenu->SetLabel(ID_PROFILE_S3U,wxT("Nothing on Slot 3 Upper Port"));
    if (my_lisaconfig->s3l.IsSameAs(_T("NOTHING"), false)) profileMenu->SetLabel(ID_PROFILE_S3L,wxT("Nothing on Slot 3 Lower Port"));

    profileMenu->Enable(ID_PROFILE_S3U,( !my_lisaconfig->s3h.IsSameAs(_T("NOTHING"), false)) );      
    profileMenu->Enable(ID_PROFILE_S3L,( !my_lisaconfig->s3l.IsSameAs(_T("NOTHING"), false)) );
  }
  else 
  {
    profileMenu->Enable(ID_PROFILE_S3U,false );      
    profileMenu->Enable(ID_PROFILE_S3L,false );
    profileMenu->SetLabel(ID_PROFILE_S3U,_T("No Card"));
    profileMenu->SetLabel(ID_PROFILE_S3L,_T("No Card"));
  }

}


void LisaEmFrame::OnProFilePowerX(int bit)
{

  int devtype=0;

  if (running)
   if (check_running_lisa_os()!=LISA_ROM_RUNNING)
   {
   switch(bit)
   { 
     case 2: if (my_lisaconfig->parallel.IsSameAs(_T("PROFILE"), false)) devtype=1; break;

    case 3: if (my_lisaconfig->s1h.IsSameAs(_T("PROFILE"), false)) devtype=1; break;
    case 4: if (my_lisaconfig->s1l.IsSameAs(_T("PROFILE"), false)) devtype=1; break;

    case 5: if (my_lisaconfig->s2h.IsSameAs(_T("PROFILE"), false)) devtype=1; break;
    case 6: if (my_lisaconfig->s2l.IsSameAs(_T("PROFILE"), false)) devtype=1; break;

    case 7: if (my_lisaconfig->s3h.IsSameAs(_T("PROFILE"), false)) devtype=1; break;
    case 8: if (my_lisaconfig->s3l.IsSameAs(_T("PROFILE"), false)) devtype=1; break;
    }
   }

  if (running && (IS_PARALLEL_PORT_ENABLED(bit)) && devtype)
  {
    if (yesnomessagebox("The Lisa may be using this Profile hard drive.  Powering it off may cause file system damage.  Are you sure  you wish to power it off?",
                        "DANGER!")==0) return;
  }

  profile_power^=(1<<(bit-2));

  UpdateProfileMenu();
}


void LisaEmFrame::OnProFilePower( wxCommandEvent& WXUNUSED(event))  {OnProFilePowerX(2);}
void LisaEmFrame::OnProFileS1UPwr(wxCommandEvent& WXUNUSED(event))  {OnProFilePowerX(3);}
void LisaEmFrame::OnProFileS1LPwr(wxCommandEvent& WXUNUSED(event))  {OnProFilePowerX(4);}
void LisaEmFrame::OnProFileS2UPwr(wxCommandEvent& WXUNUSED(event))  {OnProFilePowerX(5);}
void LisaEmFrame::OnProFileS2LPwr(wxCommandEvent& WXUNUSED(event))  {OnProFilePowerX(6);}
void LisaEmFrame::OnProFileS3UPwr(wxCommandEvent& WXUNUSED(event))  {OnProFilePowerX(7);}
void LisaEmFrame::OnProFileS3LPwr(wxCommandEvent& WXUNUSED(event))  {OnProFilePowerX(8);}

void LisaEmFrame::OnProFilePwrOnAll(wxCommandEvent& WXUNUSED(event))
{  profile_power=127; UpdateProfileMenu();}

void LisaEmFrame::OnProFilePwrOffAll(wxCommandEvent& WXUNUSED(event))
{
  if (running && profile_power)
  {
    if (yesnomessagebox("Lisa is using the profile drives. Are you sure you wish to remove power to all Profile drives?",
                        "DANGER!")==0) return;
  }
  profile_power=0; UpdateProfileMenu(); 
}


void LisaEmFrame::OnNewProFile(wxCommandEvent& WXUNUSED(event))     
{
    int sz;
    int blocks[]={9728,19456,32768,40960,65536,81920,131072};
     //               0     1     2     3     4     5     6
     //              5M   10M   16M   20M   32M   40M    64M

    char cfilename[MAXPATHLEN];
    wxFileDialog open(this, wxT( "Create blank ProFile drive as:"),
                                  wxEmptyString,
                                  wxT("lisaem-profile.dc42"),
                                  wxT("Disk Copy (*.dc42)|*.dc42|All (*.*)|*.*"),
                                  (long int)wxFD_SAVE,wxDefaultPosition);

    if (open.ShowModal()==wxID_OK)                 
    {
        wxString filename=open.GetPath();
        strncpy(cfilename,CSTR(filename),MAXPATHLEN-1);
        sz=pickprofilesize(cfilename);  if (sz<0 || sz>6) return;
        int i=dc42_create(cfilename,"-lisaem.sunder.net hd-",blocks[sz]*512,blocks[sz]*20);
        if (i) 
            wxMessageBox(wxT("Could not create the file to store the Profile.\n\nDo you have permission to write to this folder?\nIs there enough free disk space?"),
                              wxT("Failed to create drive!"));
    }
}






extern "C" void eject_floppy_animation(void)
{
 if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)==FLOPPY_PRESENT)          // initiate eject animation sequence
     {my_lisawin->floppystate=FLOPPY_NEEDS_REDRAW|FLOPPY_INSERT_2; return;}

   return;
}


extern "C" void floppy_motor_sounds(int track)
{
// there really are 3-4 speeds, however, my admittedly Bolt Thrower damaged ears can only distinguish two  :)
// close enough for government work, I guess. Or perhaps the other two speeds are handled by disk logic and the
// drive motor only really has two speeds?

//#ifndef __WXMSW__
//  if (wxSound::IsPlaying()) return;       // doesn't work on windows
//#else

if (my_lisaframe->soundplaying==(1+(track>35)))  return; // already playing this sound, avoid stutter

my_lisaframe->soundplaying=1+(track>35);
my_lisaframe->soundsw.Start(0);
//#endif

if  (sound_effects_on)
    {
      if (track>35)  {if (my_floppy_motor1->IsOk()) my_floppy_motor1->Play(wxSOUND_ASYNC|wxSOUND_LOOP);}
      else           {if (my_floppy_motor2->IsOk()) my_floppy_motor2->Play(wxSOUND_ASYNC|wxSOUND_LOOP);}
    }

}




void LisaEmFrame::OnFLOPPY(wxCommandEvent& WXUNUSED(event)) {OnxFLOPPY();}



void LisaEmFrame::insert_floppy_anim(wxString openfile)
{
    if (!openfile.Len()) return;
    const wxCharBuffer s = CSTR(openfile);

    if (my_lisaframe->running) {
        if (floppy_insert((char *)(const char *)s)) return;
    } 
    else 
    {
        floppy_to_insert = openfile;
    }

    if  ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)==FLOPPY_EMPTY) 
        {
        // initiate insert animation sequence
          my_lisawin->floppystate=FLOPPY_NEEDS_REDRAW|FLOPPY_ANIMATING|FLOPPY_INSERT_0;
        }
}


void LisaEmFrame::OnxFLOPPY(void)
{
    if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)!=FLOPPY_EMPTY) {
        wxMessageBox(_T("A previously inserted diskette is still in the drive. "
            "Please eject the diskette before inserting another."),
            _T("Diskette is already inserted!"), wxICON_INFORMATION | wxOK);
        return;
    }

    pause_run();

    wxString openfile;
    wxFileDialog open(this,                     wxT("Insert a Lisa diskette"),
                                                wxEmptyString,
                                                wxEmptyString,
                                                wxT("Disk Copy (*.dc42)|*.dc42|DART (*.dart)|*.dart|Image (*.image)|*.image|All (*.*)|*.*"),
                                                (long int)wxFD_OPEN,wxDefaultPosition);
    if (open.ShowModal()==wxID_OK)              openfile=open.GetPath();

    resume_run();
    insert_floppy_anim(openfile);
}


void LisaEmFrame::OnNewFLOPPY(wxCommandEvent& WXUNUSED(event)) {OnxNewFLOPPY();}

void LisaEmFrame::OnxNewFLOPPY(void)
{
    if (!my_lisaframe->running) {
        wxMessageBox(_T("Please turn the Lisa on before creating a new diskette image."),
            _T("The Lisa is Off"), wxICON_INFORMATION | wxOK);
        return;
    }

    if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)!=FLOPPY_EMPTY) {
        wxMessageBox(_T("A previously inserted diskette is still in the drive. "
            "Please eject the diskette before inserting another."),
            _T("Diskette is already inserted!"), wxICON_INFORMATION | wxOK);
        return;
    }

    pause_run();

    wxString openfile;
    wxFileDialog open(this,                     wxT("Create and insert a blank microdiskette image"),
                                                wxEmptyString,
                                                wxT("blank.dc42"),
                                                wxT("Disk Copy (*.dc42)|*.dc42|All (*.*)|*.*"),
                                                (long int)wxFD_SAVE,wxDefaultPosition);
    if (open.ShowModal()==wxID_OK)              openfile=open.GetPath();

    resume_run();
    if (!openfile.Len()) return;



    const wxCharBuffer s = CSTR(openfile);
    int i = dc42_create((char *)(const char *)s,"-not a Macintosh disk-", 400*512*2,400*2*12);
    if (i)  {
        wxMessageBox(_T("Could not create the diskette"),
            _T("Sorry"), wxICON_INFORMATION | wxOK);
        return;
    } else {
        floppy_insert((char *)(const char *)s);
    }


 if ((my_lisawin->floppystate & FLOPPY_ANIM_MASK)==FLOPPY_EMPTY) {
     // initiate insert animation sequence
     my_lisawin->floppystate=FLOPPY_NEEDS_REDRAW|FLOPPY_ANIMATING|FLOPPY_INSERT_0;
 }
}

//  for future use - for when we figure out the right way to turn of skins on the fly
//  - the real issue is the window size mechanism doesn't work properly.  won't prevent the user
//  from stretching the window, or making it too small.  also in win32/linux with skins off
//  the window is too small - shows scrollbars, but it allows stretching past the lisa screen bitmap
//  which causes garbage in the non-refreshed space.
void LisaEmFrame::UnloadImages(void)
{
    EXTERMINATE( display_image );
    EXTERMINATE( my_skinDC     );
    EXTERMINATE( my_skin0DC    );
    EXTERMINATE( my_skin1DC    );
    EXTERMINATE( my_skin2DC    );
    EXTERMINATE( my_skin3DC    );
    EXTERMINATE( my_floppy0DC  );
    EXTERMINATE( my_floppy1DC  );
    EXTERMINATE( my_floppy2DC  );
    EXTERMINATE( my_floppy3DC  );
    EXTERMINATE( my_floppy4DC  );
    EXTERMINATE( my_poweronDC  );
    EXTERMINATE( my_poweroffDC );

    EXTERMINATE( my_skin       );
    EXTERMINATE( my_skin0      );
    EXTERMINATE( my_skin1      );
    EXTERMINATE( my_skin2      );
    EXTERMINATE( my_skin3      );
    EXTERMINATE( my_floppy0    );
    EXTERMINATE( my_floppy1    );
    EXTERMINATE( my_floppy2    );
    EXTERMINATE( my_floppy3    );
    EXTERMINATE( my_floppy4    );
    EXTERMINATE( my_poweron    );
    EXTERMINATE( my_poweroff   );

    skins_on=0;

    int x,y;
    if (my_lisawin)
    {   my_lisawin->SetClientSize(wxSize(effective_lisa_vid_size_x,effective_lisa_vid_size_y));     // LisaWin //
        my_lisawin->GetClientSize(&x,&y);                                                           // LisaWin //
        my_lisawin->GetSize(&x,&y);                                                                 // LisaWin //
        skin.screen_origin_x=( _N(x)  - effective_lisa_vid_size_x)>>1;                              // center display
        skin.screen_origin_y=( _N(y)  - effective_lisa_vid_size_y)>>1;                              // on skinless
        skin.screen_origin_x= (skin.screen_origin_x<0 ? 0:skin.screen_origin_x);
        skin.screen_origin_y= (skin.screen_origin_y<0 ? 0:skin.screen_origin_y);

// X11, Windows return too small a value for getsize
        //x+=WINXDIFF; y+=WINYDIFF;
        //SetMaxSize(wxSize(ISKINSIZE));                                                            // Frame   //
        SetMinSize(wxSize(720,364));                                                                // Frame   //
        SendSizeEvent();
    }

}


void LisaEmFrame::LoadImages(void)
{

/*
 * On MacOS X, load the images from the Resources/ dir inside the app bundle.
 * Much faster than WX's breathtakingly slow parsing of XPMs, and much
 * smaller too since we can use PNGs.
 * On Linux/Win32 use embedded XPM strings/BMP resources.
 */
ALERT_LOG(0,"In LoadImages");
if (skins_on)
{
   // Ask our Dalek friends to wipe DC and skin because when we call this a 2nd time, it's due to scale changes
   // and without it, it causes crashes
    EXTERMINATE(my_skin);
    EXTERMINATE(my_skinDC);
    EXTERMINATE(display_image);

   // will ofc always happen now, but this looks cleaner as it continues the if ... pattern
    if (!my_skin) my_skin     = new wxBitmap(ISKINSIZEX,ISKINSIZEY,DEPTH);//, -1);  //20061228//

    wxString pngfile;
    
    #define LOADBITMAP(XpngX,XresnameX)                                                                  \
    { pngfile=skindir + skin.XresnameX; if (!XpngX) XpngX    = new wxBitmap(pngfile, wxBITMAP_TYPE_PNG); \
      ALERT_LOG(0,"Opening %s filename",(const char *)pngfile);                                          \
      if (!XpngX->IsOk()) EXIT(0,0,"something went wrong with loading %s",(const char *)pngfile);        \
    }                                                                                                        


    LOADBITMAP(my_skin0,lisaface0name);
    LOADBITMAP(my_skin1,lisaface1name);
    LOADBITMAP(my_skin2,lisaface2name);
    LOADBITMAP(my_skin3,lisaface3name);

    // ::TODO:: eventually add Lisa 1 top twiggy floppy1 support as well
    LOADBITMAP(my_floppy0,floppy2anim0);
    LOADBITMAP(my_floppy1,floppy2anim1);
    LOADBITMAP(my_floppy2,floppy2anim2);
    LOADBITMAP(my_floppy3,floppy2anim3);
    LOADBITMAP(my_floppy4,floppy2animN);

    LOADBITMAP(my_poweron, powerOn);
    LOADBITMAP(my_poweroff,powerOff);

    if (!my_skinDC)     my_skinDC    = new wxMemoryDC;
    if (!my_skin0DC)    my_skin0DC   = new wxMemoryDC;
    if (!my_skin1DC)    my_skin1DC   = new wxMemoryDC;
    if (!my_skin2DC)    my_skin2DC   = new wxMemoryDC;
    if (!my_skin3DC)    my_skin3DC   = new wxMemoryDC;
    if (!my_floppy0DC)  my_floppy0DC = new wxMemoryDC;
    if (!my_floppy1DC)  my_floppy1DC = new wxMemoryDC;
    if (!my_floppy2DC)  my_floppy2DC = new wxMemoryDC;
    if (!my_floppy3DC)  my_floppy3DC = new wxMemoryDC;
    if (!my_floppy4DC)  my_floppy4DC = new wxMemoryDC;
    if (!my_poweronDC)  my_poweronDC = new wxMemoryDC;
    if (!my_poweroffDC) my_poweroffDC= new wxMemoryDC;

    my_skinDC->SelectObject(*my_skin);
    my_skin0DC->SelectObject(*my_skin0);
    my_skin1DC->SelectObject(*my_skin1);
    my_skin2DC->SelectObject(*my_skin2);
    my_skin3DC->SelectObject(*my_skin3);

    ALERT_LOG(0,"From Load Images");
    prepare_skin();

    my_floppy0DC->SelectObject(*my_floppy0);
    my_floppy1DC->SelectObject(*my_floppy1);
    my_floppy2DC->SelectObject(*my_floppy2);
    my_floppy3DC->SelectObject(*my_floppy3);
    my_floppy4DC->SelectObject(*my_floppy4);
    my_poweronDC->SelectObject(*my_poweron);
    my_poweroffDC->SelectObject(*my_poweroff);

    my_lisawin->SetMinSize(wxSize(IWINSIZE));
    my_lisawin->SetClientSize(wxSize(IWINSIZE));                                                     // Lisawin   //
    my_lisawin->SetMaxSize(wxSize(ISKINSIZE));                                                       // Lisawin   //
    ALERT_LOG(0,"SetScrollbars");
    my_lisawin->SetScrollbars(ISKINSIZEX/_H(100), ISKINSIZEY/_H(100),  _H(100),_H(100),  0,0,  true);                // Lisawin   //
    ALERT_LOG(0,"Enabled Scrollbars");
    my_lisawin->EnableScrolling(true,true);                                                          // Lisawin   //

    SendSizeEvent();

    skin.screen_origin_x=skin.default_screen_origin_x;
    skin.screen_origin_y=skin.default_screen_origin_y;
  }

}

wxSize get_size_prefs(void)
{
    static wxSize size;
    int x,y;
    y=myConfig->Read(_T("/lisaframe/sizey"),(long)0);
    x=myConfig->Read(_T("/lisaframe/sizex"),(long)0);
    if (x<=0 || y<=0) { ALERT_LOG(0,"** Reset LisaEmFrame config Size because x,y=(%d,%d) ***\n\n",x,y); x=IWINSIZEX;y=IWINSIZEY;}
    size=wxSize(x,y);
    return size;
}


LisaEmFrame::LisaEmFrame(const wxString& title)
        : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, get_size_prefs(), wxDEFAULT_FRAME_STYLE)
//      : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(ISKINSIZE), wxDEFAULT_FRAME_STYLE)
{
    effective_lisa_vid_size_y=500;
    wxInitAllImageHandlers();

    barrier=0;
    clx=0;
    lastt2 = 0;
    lastclk = 0;
    lastcrtrefresh=0;
    hostrefresh=1000/ 8;

    int x,y;
    y=myConfig->Read(_T("/lisaframe/sizey"),(long)0);
    x=myConfig->Read(_T("/lisaframe/sizex"),(long)0);

    if (x<=0 || y<=0) { ALERT_LOG(0,"** Reset LisaEmFrame config Size because x,y=(%d,%d) ***\n\n",x,y); x=IWINSIZEX;y=IWINSIZEY;}
   //wxScreenDC theScreen;
   //theScreen.GetSize(&screensizex,&screensizey);

    ALERT_LOG(0,"Getting display size")
    wxDisplaySize(&screensizex,&screensizey);
    ALERT_LOG(0,"Got (%d,%d) screen size from wxDisplaySize",screensizex, screensizey);
    if  (screensizex< 0 || screensizex>8192 || screensizey<0 || screensizey>8192)
        {ALERT_LOG(0,"Got Garbage screen size: %d,%d",screensizex,screensizey);
          screensizex=1024; screensizey=768;}

   if   (x>screensizex || y>screensizey)  // make sure we don't get too big
        {
          ALERT_LOG(0,"**** Got display size %d,%d but it's out of bounds because %d,%d",screensizex,screensizey,x,y);

          x=MIN(screensizex-100,IWINSIZEX);
          y=MIN(screensizey-150,IWINSIZEY);
        }

    ALERT_LOG(0,"Loading skin configs");

    wxString myconfig, skinconfigfile;
    skinname=myConfig->Read(_T("/lisaskin/name"),"default" );   // belt
    if (skinname.IsEmpty()) skinconfigfile=_T("default");               // suspenders

    osslash  = wxFileName::GetPathSeparator(wxPATH_NATIVE);
    wxStandardPathsBase& stdp = wxStandardPaths::Get();
    resdir   = stdp.GetResourcesDir() + osslash;
    skindir  =  resdir   + "skins" + osslash + skinname + osslash;     // /usr/local/share/LisaEm/skins/default

    ALERT_LOG(0,"Skin dir is %s",CSTR(skindir) );

    myconfig << skindir  << skinname << ".conf";            // /usr/local/share/LisaEm/skins/default/default.conf

    ALERT_LOG(0,"Using Skin config file::[%s]::",CSTR(myconfig) );
    
    skinconfig=new wxFileConfig(_T("LisaEm"),
                                _T("sunder.NET"),      // company
                                  (myconfig),          // local
                                  (myconfigfile),      // global
                                  wxCONFIG_USE_LOCAL_FILE,
                                  wxConvAuto() );      // or wxConvUTF8
    skin.Load(skinconfig);
    ALERT_LOG(0,"Using display size %d,%d",screensizex,screensizey);
    
#ifndef __WXOSX__
    ALERT_LOG(0,"Setting icon");
    SetIcon( wxICON( lisa2icon ));
#endif

    if (skins_on)
    {
        ALERT_LOG(0,"sizing skin on %d,%d",x,y);
        //SetMinSize(wxSize(IWINSIZE));                                                                // Frame    //
        if (x<IWINSIZEX || y<IWINSIZEY) {x=IWINSIZEX; y=IWINSIZEY;}
        //SetMinSize(wxSize(720,500));
        //SetClientSize(wxSize(x,y));                                                                  // Frame    //
        GetClientSize(&dwx,&dwy);
        dwx-=x; dwy-=y;
        //SetClientSize(wxSize(x-dwx,y-dwy));
        //SetMaxSize(wxSize(ISKINSIZE));                                                               // Frame    //
    }                                                                                               // Frame    //
    else
    {
       ALERT_LOG(0,"sizing skins off");

       // try to fit the Lisa's display on the screen at least.
       if (x<effective_lisa_vid_size_x || y<effective_lisa_vid_size_y)
          {x=effective_lisa_vid_size_x;   y=effective_lisa_vid_size_y;}

       // but if it's too big, ensure we don't go over the display'size.
       x=MIN(screensizex-100,x); y=MIN(screensizey-150,x);

       //SetMinSize(wxSize(720,364));
       //SetClientSize(wxSize(x,y));                                                                  // Frame    //
       //GetClientSize(&dwx,&dwy);
       dwx-=x; dwy-=y;

       //SetMaxSize(wxSize(ISKINSIZE));                                                               // Frame    //
     }


    buildscreenymap();

    ALERT_LOG(0,"Setting menu items");
    // Create a menu bar
    fileMenu          = new wxMenu;
    editMenu          = new wxMenu;
    keyMenu           = new wxMenu;
    DisplayMenu       = new wxMenu;
    DisplayRefreshSub = new wxMenu;
    DisplayScaleSub   = new wxMenu;
    throttleMenu      = new wxMenu;
    profileMenu       = new wxMenu;
    helpMenu          = new wxMenu;

    /* https://wiki.wxwidgets.org/WxMac-specific_topics#Keyboard_Shortcuts
       Keyboard shortcuts use the command-key (the cloverleaf or open-apple key on the keyboard) instead of the control key. wxMac lets you specify 
       keyboard shortcuts MS Windows style, and they are automatically translated. So Open should be specified as "&Open\tCtrl+O", and on the Mac 
       the accelerator will be removed and the Ctrl will be replaced with a cloverleaf in the menu.
    */

    profileMenu->Append(ID_PROFILE_ALL_ON,   wxT("Power On all Parallel Devices"), wxT("Powers on all parallel port attached devices.")  );
    profileMenu->Append(ID_PROFILE_ALL_OFF,  wxT("Power off all Parallel Devices"), wxT("Shuts off all parallel port attached devices.")  ); 
    
    profileMenu->AppendSeparator();

    profileMenu->AppendCheckItem(ID_PROFILEPWR,       wxT("Power ProFile on Parallel Port"), wxT("Toggles power to the Profile drive on the parallel port") );

    profileMenu->AppendSeparator();

    profileMenu->AppendCheckItem(ID_PROFILE_S1U,      wxT("Power ProFile on Slot 1 Upper Port"),wxT("Toggle power to the drive") );
    profileMenu->AppendCheckItem(ID_PROFILE_S1L,      wxT("Power ProFile on Slot 1 Lower Port"),wxT("Toggle power to the drive") );
    profileMenu->AppendSeparator();                                                                                      
                                                                                                                         
    profileMenu->AppendCheckItem(ID_PROFILE_S2U,      wxT("Power ProFile on Slot 2 Upper Port"),wxT("Toggle power to the drive") );
    profileMenu->AppendCheckItem(ID_PROFILE_S2L,      wxT("Power ProFile on Slot 2 Lower Port"),wxT("Toggle power to the drive") );
    profileMenu->AppendSeparator();                                                                                      
                                                                                                                         
    profileMenu->AppendCheckItem(ID_PROFILE_S3U,      wxT("Power ProFile on Slot 3 Upper Port"),wxT("Toggle power to the drive") );
    profileMenu->AppendCheckItem(ID_PROFILE_S3L,      wxT("Power ProFile on Slot 3 Lower Port"),wxT("Toggle power to the drive") );
                                                                                                                        
    editMenu->Append(wxID_PASTE, wxT("Paste") ,       wxT("Paste clipboard text to keyboard.") ); 

    DisplayRefreshSub->AppendRadioItem(ID_REFRESH_60Hz,wxT("60Hz Refresh"),wxT("60Hz Display Refresh - skip no frames - for fast machines"));
    DisplayRefreshSub->AppendRadioItem(ID_REFRESH_20Hz,wxT("20Hz Refresh"),wxT("20Hz Display Refresh - display every 2nd frame"));
    DisplayRefreshSub->AppendRadioItem(ID_REFRESH_12Hz,wxT("12Hz Refresh"),wxT("12Hz Display Refresh - display every 5th frame - for slow machines"));
    DisplayRefreshSub->AppendRadioItem(ID_REFRESH_8Hz, wxT(" 8Hz Refresh"),wxT("8Hz Display Refresh - display every 7th frame - for slow machines"));
    DisplayRefreshSub->AppendRadioItem(ID_REFRESH_4Hz, wxT(" 4Hz Refresh"),wxT("4Hz Display Refresh - display every 9th frame - for super slow machines"));

    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_25,   wxT("0.25x"), wxT("Set Video Magnification Size 0.25x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_50,   wxT("0.50x"), wxT("Set Video Magnification Size 0.50x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_75,   wxT("0.75x"), wxT("Set Video Magnification Size 0.75x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_100,  wxT("1.00x"), wxT("Set Video Magnification Size 1.00x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_125,  wxT("1.25x"), wxT("Set Video Magnification Size 1.25x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_150,  wxT("1.50x"), wxT("Set Video Magnification Size 1.50x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_175,  wxT("1.75x"), wxT("Set Video Magnification Size 1.75x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_200,  wxT("2.00x"), wxT("Set Video Magnification Size 2.00x") );
    #ifndef __WXOSX__
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_225,  wxT("2.25x"), wxT("Set Video Magnification Size 2.25x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_250,  wxT("2.50x"), wxT("Set Video Magnification Size 2.50x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_275,  wxT("2.75x"), wxT("Set Video Magnification Size 2.75x") );
    DisplayScaleSub->AppendRadioItem(ID_VID_SCALE_300,  wxT("3.00x"), wxT("Set Video Magnification Size 3.00x") );
    DisplayScaleSub->AppendSeparator();
    #endif
    DisplayScaleSub->Append(ID_VID_SCALE_ZOOMIN,    wxT("Zoom In \tCtrl-+"),   wxT("Zoom In") );
    DisplayScaleSub->Append(ID_VID_SCALE_ZOOMOUT,   wxT("Zoom Out \tCtrl--"),  wxT("Zoom Out") );

    DisplayMenu->AppendRadioItem(ID_VID_HQ35X ,       wxT("HQX Upscaler")          ,  wxT("Aspect Corrected High Quality Magnification Filer hq3.5x") );
    DisplayMenu->AppendRadioItem(ID_VID_AA  ,         wxT("AntiAliased")           ,  wxT("Aspect Corrected with Anti Aliasing") );
    DisplayMenu->AppendRadioItem(ID_VID_AAG ,         wxT("AntiAliased with Gray Replacement"),  wxT("Aspect Corrected with Anti Aliasing and Gray Replacing") );
    DisplayMenu->AppendRadioItem(ID_VID_SY  ,         wxT("Raw")                   ,  wxT("Uncorrected Aspect Ratio") );
    DisplayMenu->AppendRadioItem(ID_VID_DY  ,         wxT("Double Y")              ,  wxT("Double Vertical Size") );
    DisplayMenu->AppendRadioItem(ID_VID_2X3Y,         wxT("Double X, Triple Y")    ,  wxT("Corrected Aspect Ratio, Large Display") ); 
    
    DisplayMenu->AppendSeparator();
    DisplayMenu->AppendCheckItem(ID_VID_SKINS,        wxT("Skin"),                    wxT("Turn skins on/off") );
    DisplayMenu->AppendCheckItem(ID_VID_SKINLESSCENTER,wxT("Center when skinless"),    wxT("Center the display when skins are turned off") );
    DisplayMenu->Append(         ID_VID_SKINSELECT,   wxT("Change Skin"),             wxT("Skin Select") );
    DisplayMenu->AppendSeparator();


    DisplayMenu->Append(ID_REFRESH_SUB,wxT("Refresh Rate"), DisplayRefreshSub );
    DisplayMenu->AppendSeparator();


    DisplayMenu->Append(ID_VID_SCALED_SUB, wxT("Zoom/Scale"), DisplayScaleSub);

    DisplayMenu->AppendSeparator();

    DisplayMenu->AppendCheckItem(ID_HIDE_HOST_MOUSE,wxT("Hide Host Mouse Pointer"),wxT("Hides the host mouse pointer - may cause lag"));

    DisplayMenu->AppendSeparator();

    FullScreenCheckMenuItem=DisplayMenu->AppendCheckItem(ID_VID_FULLSCREEN,wxT("Fullscreen\tF11"), wxT("Enter/leave full screen mode"));

    #ifdef DEBUG
    throttleMenu->AppendRadioItem(ID_THROTTLE1,       wxT("1 MHz")  , wxT("1 MHz - Slowdown for debugging") );
    #endif
    throttleMenu->AppendRadioItem(ID_THROTTLE5,       wxT("5 MHz")  , wxT("5 MHz - Stock Lisa Speed, recommended.") );
    throttleMenu->AppendRadioItem(ID_THROTTLE8,       wxT("8 MHz")  , wxT("8 MHz - Original Macintosh 128 Speed") );
    throttleMenu->AppendRadioItem(ID_THROTTLE10,      wxT("10 MHz") , wxT("10Mhz"));
    throttleMenu->AppendRadioItem(ID_THROTTLE12,      wxT("12 MHz") , wxT("12Mhz"));
    throttleMenu->AppendRadioItem(ID_THROTTLE16,      wxT("16 MHz") , wxT("16Mhz"));
    throttleMenu->AppendRadioItem(ID_THROTTLE32,      wxT("32 MHz") , wxT("32Mhz - For modern machines") );
    throttleMenu->AppendRadioItem(ID_THROTTLE48,      wxT("48 MHz") , wxT("48Mhz - For modern machines") );
    throttleMenu->AppendRadioItem(ID_THROTTLE64,      wxT("64 MHz") , wxT("64Mhz - For modern machines") );
    throttleMenu->AppendRadioItem(ID_THROTTLE100,     wxT("100 MHz"), wxT("100Mhz - For modern machines") );
    throttleMenu->AppendRadioItem(ID_THROTTLE128,     wxT("128 MHz"), wxT("128Mhz - For modern machines") );
    throttleMenu->AppendRadioItem(ID_THROTTLE256,     wxT("256 MHz"), wxT("256MHz - For modern machines") );
    #ifdef DEBUG
    throttleMenu->AppendRadioItem(ID_THROTTLEX,       wxT("512 Mhz"), wxT("Ludicrous Speed!") );
    #endif

    throttleMenu->AppendSeparator();
    throttleMenu->AppendRadioItem(ID_ET100_75,        wxT("Higher 68000 Performance"),     wxT("Normal 100/75ms duty timer - faster emulated CPU, less smooth animations"));
    throttleMenu->AppendRadioItem(ID_ET50_30,         wxT("Balanced 68000/Graphics"),      wxT("Medium  50/30ms duty timer - smoother animation, slower CPU"));
    throttleMenu->AppendRadioItem(ID_ET40_25,         wxT("Smoother Graphics"),            wxT("Small   40/25ms duty timer - even smoother animation, slower CPU"));
    throttleMenu->AppendRadioItem(ID_ET30_20,         wxT("Smoothest display, lower 68000 Performance"),     wxT("Tiny    30/20ms duty timer - smoothest animation, slowest CPU"));

    // The "About" item should be in the help menu

    //helpMenu->Append(wxID_ABOUT, wxT("&License"),                 wxT("License Information"));
    //helpMenu->AppendSeparator();
    //helpMenu->Append(wxID_ABOUT, wxT("Help \tF1"),                wxT("Emulator Manual"));
    //helpMenu->Append(wxID_ABOUT, wxT("Lisa Help"),                wxT("How to use Lisa software"));
    //helpMenu->AppendSeparator();

    helpMenu->Append(wxID_ABOUT,         wxT("&About LisaEm"),            wxT("About the Lisa Emulator"));
    #ifndef __WXOSX__
    helpMenu->AppendSeparator();
    #endif
    helpMenu->Append(ID_LISAWEB,         wxT("Lisa Emulator Webpage"),    wxT("https://lisaem.sunder.net"));
    helpMenu->Append(ID_LISAFAQ,         wxT("Lisa FAQ webpage"),         wxT("https://lisafaq.sunder.net"));
    helpMenu->Append(ID_LISALIST2,       wxT("LisaList2 Forum"),          wxT("https://lisalist2.com"));

    keyMenu->Append(ID_POWERKEY,         wxT("Power Button"),             wxT("Push the Power Button"));
    keyMenu->Append(ID_APPLEPOWERKEY,    wxT("Apple+Power Button"),       wxT("Push Apple + the Power Button"));
    keyMenu->AppendSeparator();

    keyMenu->Append(ID_KEY_APL_DOT,      wxT("Apple ."),                  wxT("Apple + ."));

    keyMenu->Append(ID_KEY_APL_S,        wxT("Apple S"),                  wxT("Apple + S"));
    keyMenu->Append(ID_KEY_APL_ENTER,    wxT("Apple Enter"),              wxT("Apple + Enter"));
    keyMenu->Append(ID_KEY_APL_RENTER,   wxT("Apple Right Enter"),        wxT("Apple + Numpad Enter"));

    keyMenu->Append(ID_KEY_APL_1,        wxT("Apple 1"),                  wxT("Apple + 1"));
    keyMenu->Append(ID_KEY_APL_2,        wxT("Apple 2"),                  wxT("Apple + 2"));
    keyMenu->Append(ID_KEY_APL_3,        wxT("Apple 3"),                  wxT("Apple + 3"));


    keyMenu->Append(ID_KEY_OPT_0,        wxT("Option 0"),                 wxT("Option 0"));
    keyMenu->Append(ID_KEY_OPT_4,        wxT("Option 4"),                 wxT("Option 4"));
    keyMenu->Append(ID_KEY_OPT_7,        wxT("Option 7"),                 wxT("Option 7"));

    keyMenu->AppendSeparator();
    keyMenu->Append(ID_KEY_NMI,          wxT("NMI Key"),                  wxT("Send Non-Maskable Interrupt key - for LisaBug"));
    keyMenu->Append(ID_KEY_RESET,        wxT("Reset Button"),             wxT("Reset the Lisa - use only if the running OS has crashed!"));

    keyMenu->AppendSeparator();
    keyMenu->AppendRadioItem(ID_ASCIIKB, wxT("ASCII Keyboard"),           wxT("Translate host keys into ASCII, then to Lisa keys (preferred)"));
    keyMenu->AppendRadioItem(ID_RAWKB,   wxT("Raw Keyboard"),             wxT("Map host keys to Lisa keys directly"));
    keyMenu->AppendRadioItem(ID_RAWKBBUF,wxT("Raw Buffered Keyboard"),    wxT("Map host keys to Lisa keys directly, buffer to prevent repeats"));
    //not-yet-used//keyMenu->AppendSeparator();
    //not-yet-used//keyMenu->Append(ID_KEYBOARD,         wxT("Keyboard"),         wxT("Lisa Keyboard"));

    fileMenu->Append(wxID_OPEN,          wxT("Open Preferences"),         wxT("Open a LisaEm Preferences file"));
    fileMenu->Append(wxID_SAVEAS,        wxT("Save Preferences As"),      wxT("Save current LisaEm Preferences to a new file"));
    fileMenu->Append(wxID_PREFERENCES,   wxT("Preferences"),              wxT("Configure this Lisa"));

    fileMenu->AppendSeparator();
    fileMenu->AppendCheckItem(ID_PAUSE,  wxT("Pause"),                    wxT("Pause Emulation\tCtrl-." ) );
    fileMenu->AppendSeparator();

    fileMenu->Append(  ID_FLOPPY,        wxT("Insert diskette"),          wxT("Insert a disk image\tCtrl-O"));
    fileMenu->Append(ID_NewFLOPPY,       wxT("Insert blank diskette"),    wxT("Create, and insert, a blank disk image\tCtrl-N"));
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_PROFILE_NEW,     wxT("Create new Profile image"), wxT("Creates a blank ProFile storage file")  );
    fileMenu->AppendSeparator();

    #if DEBUG
    fileMenu->Append(ID_SCREENREGION,    wxT("Grab Screen Region"),       wxT("Grab bits of a screen region for scripting"));
    fileMenu->AppendSeparator();
    #endif
    fileMenu->Append(ID_SCREENSHOT,      wxT("Screenshot"),               wxT("Save the current screen as an image"));
    fileMenu->Append(ID_SCREENSHOT_FULL, wxT("Full Screenshot"),          wxT("Save a screenshot along with the skin\tCtrl-S"));
    fileMenu->Append(ID_SCREENSHOT_RAW,  wxT("Raw Screenshot"),           wxT("Save a raw screenshot"));

    fileMenu->AppendSeparator();

    fileMenu->Append(ID_FUSH_PRNT,       wxT("Flush Print Jobs"),         wxT("Force pending print jobs to print\tCtrl-P"));

    fileMenu->AppendSeparator();

    #ifndef __WXMSW__
    #ifdef TRACE
    fileMenu->AppendCheckItem(ID_DEBUG,  wxT("Trace Log"),                wxT("Trace Log On/Off"));
    fileMenu->AppendCheckItem(ID_DEBUG2, wxT("Trace Log on Click/Key"),   wxT("Trace Log On/Off on next mouse click or keypress"));
    fileMenu->Append         (ID_RAMDUMP,wxT("Dump memory"),              wxT("Dump memory, MMU and a screenshot"));
    fileMenu->AppendSeparator();
    #endif
    #endif

    fileMenu->Append(wxID_EXIT,          wxT("Exit"),                     wxT("Quit the Emulator"));

    // Now append the freshly created menu to the menu bar...
    ALERT_LOG(0,"New menubar");
    menuBar = new wxMenuBar();

    menuBar->Append(fileMenu,     wxT("File"));
    menuBar->Append(editMenu,     wxT("Edit"));
    menuBar->Append(keyMenu,      wxT("Key"));
    menuBar->Append(DisplayMenu,  wxT("Display"));
    menuBar->Append(throttleMenu, wxT("Throttle"));
    menuBar->Append(profileMenu,  wxT("Parallel Port"));

    menuBar->Append(helpMenu,     wxT("&Help"));

    ALERT_LOG(0,"Set menubar");
    SetMenuBar(menuBar);

    ALERT_LOG(0,"Update profile");
    UpdateProfileMenu();

    ALERT_LOG(0,"Create Status Bar 1");
    CreateStatusBar(1);

    ALERT_LOG(0,"Welcome status")

//    SetStatusText(wxT("Welcome to the Lisa Emulator Project!"));
    char *t=get_welcome_fortune();
    SetStatusText(t);
    soundplaying=0;

    ALERT_LOG(0,"Creating window.")
    if (!my_lisawin) my_lisawin = new LisaWin(this);
    ALERT_LOG(0,"Setting options.")

    my_lisawin->doubley = 0;
    my_lisawin->dirtyscreen = 1;
    my_lisawin->brightness = 0;
    my_lisawin->refresh_bytemap = 1;

    if  (skins_on)
        {
            int x,y;
            LoadImages();
            x=my_skin->GetWidth();
            y=my_skin->GetHeight();
            my_lisawin->SetMaxSize(wxSize(x,y));
            my_skinDC->SelectObject(*my_skin);
            my_lisawin->SetVirtualSize(x,y);
            ALERT_LOG(0,"SetScrollbars");
            my_lisawin->SetScrollbars(x/100, y/100,  100,100,  0,0,  true);
            ALERT_LOG(0,"Enable Scrollbars");
            my_lisawin->EnableScrolling(true,true);
        }
    else
        {
            int x,y;
            y=myConfig->Read(_T("/lisaframe/sizey"),(long)0);
            x=myConfig->Read(_T("/lisaframe/sizex"),(long)0);
            if  (x<effective_lisa_vid_size_x || y<effective_lisa_vid_size_y )
                {x=effective_lisa_vid_size_x;   y=effective_lisa_vid_size_y;}
//             ALERT_LOG(0,"Setting frame size to:%d,%d",x,y);

           SetClientSize(wxSize(x,y));                                                           // Frame    //
           GetClientSize(&x,&y);                                                                 // Frame    //
           SetMinSize( wxSize( _H(720),_H(364) ) );
           x+=WINXDIFF; y+=WINYDIFF;                                                             // LisaWin  //
           my_lisawin->SetClientSize(wxSize(x,y));
           my_lisawin->GetSize(&x,&y);                                                           // LisaWin  //
           // X11, Windows return too small a value for getsize
           x+=WINXDIFF; y+=WINYDIFF;                                                             // LisaWin  //

           my_lisawin->SetMaxSize(wxSize(ISKINSIZE));                                            // LisaWin  //
           my_lisawin->SetMinSize(wxSize( _H(720), _H(364) ));                                   // LisaWin  //
           ALERT_LOG(0,"disable scrollbar");
           my_lisawin->EnableScrolling(false,false);                                             // LisaWin  //
        }

    SendSizeEvent();
    my_lisawin->Show(true);
    fileMenu->Check(ID_PAUSE,false);


    m_emulation_timer = new wxTimer(this, ID_EMULATION_TIMER);
    m_emulation_timer->Start(emulation_tick, wxTIMER_CONTINUOUS);

    #if wxUSE_DRAG_AND_DROP    // associate drop targets with the controls
      SetDropTarget(new DnDText());
      SetDropTarget(new DnDFile());
    #endif

    update_menu_checkmarks();
}


void LisaEmFrame::OnFlushPrint(wxCommandEvent& WXUNUSED(event))  {iw_enddocuments();iw_enddocuments();}


#ifdef DEBUG
extern "C" void tracelog_screenshot(char *filename)
{
    FILE *rawdump;
    int updated=0;    

    uint32 a3,xx;
    uint16 val;
    uint8  d;
    uint8 bright[8];
    char rawdumpname[1024];
    wxImage *image = new class wxImage(lisa_vid_size_x, lisa_vid_size_y, true);

    bright[0]=240;                                          
    bright[1]=240;                        
    bright[2]=240;               
    bright[4]=bright[5]=bright[6]=bright[7]=0;
    pause_run();
    snprintf(rawdumpname,1023,"%s.bin",filename);
    rawdump=fopen(rawdumpname,"wb");
    if (!!rawdump)
    {
        fwrite( &lisaram[videolatchaddress],32768, 1, rawdump);
        fclose(rawdump);
    }

    for ( int yi=0; yi < lisa_vid_size_y; yi++)
    {
        for ( int xi=0; xi < lisa_vid_size_x;)
            { SETRGB16_RAW_X(xi,yi,{image->SetRGB(xi,yi,d,d,d+EXTRABLUE);});   }
    }

    image->SetOption(wxIMAGE_OPTION_PNG_FORMAT, wxPNG_TYPE_GREY);
    image->SetOption(wxIMAGE_OPTION_PNG_BITDEPTH, 8);
    image->SaveFile(filename);

    delete image;
    resume_run();
}

#endif

// copied/based on wxWidgets samples image.cpp (c) 1998-2005 Robert Roebling
void LisaEmFrame::OnScreenshot(wxCommandEvent& event)
{
 wxImage* image=NULL;

    wxString description;

    pause_run();

    if ( event.GetId()==ID_SCREENSHOT_RAW)
    {
        int updated=0;    
        uint32 a3,xx;
        uint16 val;
        uint8  d;
        uint8 bright[8];

        if (my_lisaframe->running==emulation_off)
          {
            wxMessageBox(_T("I can't take a raw screenshot while the Lisa is powered off. It wouldn't be much fun anyway. Just a black rectangle"),
                         _T("If you stare into the black void, it will stare back!")); // Choronzon is that you? 
            return;
          }

        bright[0]=240;                                          
        bright[1]=240;                        
        bright[2]=240;               
        bright[4]=bright[5]=bright[6]=bright[7]=0;

        description = _T("Save RAW Screenshot");
        ALERT_LOG(0,"Raw Screenshot");
        if (lisa_ui_video_mode == vidmod_hq35x) {
           image = new class wxImage(my_lisahq3xbitmap->ConvertToImage());
        }
        else
        {
          image = new class wxImage(lisa_vid_size_x, lisa_vid_size_y, true);
          for ( int yi=0; yi < lisa_vid_size_y; yi++)
              {
                for ( int xi=0; xi < lisa_vid_size_x;)
                    { SETRGB16_RAW_X(xi,yi,{image->SetRGB(xi,yi,d,d,d+EXTRABLUE);});   }
              }
        }
    }
    else if ( event.GetId()==ID_SCREENSHOT_FULL && skins_on )
    {
          ALERT_LOG(0,"Screeshot with Skin");
          description = _T("Save Screenshot with Skin");
          image = new class wxImage(my_skin->ConvertToImage());
    }
    else if ( event.GetId()==ID_SCREENSHOT ||( event.GetId()==ID_SCREENSHOT_FULL && !skins_on) )
    {
          ALERT_LOG(0,"screenshot 2 and no skins");
          if (!skins_on) 
          {
                wxBitmap   *bitmap=NULL;   
                wxMemoryDC *dc=NULL;
                dc=new class wxMemoryDC;
                
                if (lisa_ui_video_mode == vidmod_hq35x) {
                    bitmap=new class wxBitmap(_H(720),_H(364)*HQ3XYSCALE,DEPTH);
                    dc->SelectObject(*bitmap);

                    ALERT_LOG(0,"HQ3X Skinless Screenshot");

                    hq3x_32_rb( 90, my_lisabitmap, 720,364, 0xe0);
                    dc->StretchBlit(0,0,   // target x,y
                                    _H(720),_H(364)*HQ3XYSCALE,     // size w,h
                                      my_memDC, 0,0, 720,364,
                                      wxCOPY, false);
                    image = new class wxImage(bitmap->ConvertToImage());
                } 
                else
                {
                    bitmap=new class wxBitmap(effective_lisa_vid_size_x, effective_lisa_vid_size_y,DEPTH);
                    dc->SelectObject(*bitmap);
                    image = new class wxImage(my_lisabitmap->ConvertToImage());
                }
                

                DEBUG_LOG(0,"converting to image");

          }
          else
          {
                wxBitmap   *bitmap=NULL;   
                wxMemoryDC *dc=NULL;
                dc=new class wxMemoryDC;
                
                bitmap=new class wxBitmap(effective_lisa_vid_size_x, effective_lisa_vid_size_y,DEPTH);
                dc->SelectObject(*bitmap);

                DEBUG_LOG(0,"converting to image");

                if (lisa_ui_video_mode == vidmod_hq35x) {
                    ALERT_LOG(0,"HQ3X Skinless Screenshot");
                    hq3x_32_rb( 90, my_lisabitmap, 720,364, 0xf0);
                    dc->StretchBlit(_H(skin.screen_origin_x + e_dirty_x_min), _H(skin.screen_origin_y + e_dirty_y_min),   // target x,y
                                    _H(720),_H(364)*HQ3XYSCALE,     // size w,h
                                      my_memDC, 0,0, (e_dirty_x_max-e_dirty_x_min+1),(e_dirty_y_max-e_dirty_y_min+1),
                                      wxCOPY, false);
                }
                else
                {
                    ALERT_LOG(0,"NON-HQ3X Screenshot");
                    dc->StretchBlit(0,0, 
                                    effective_lisa_vid_size_x, effective_lisa_vid_size_y,  // these are already * hidpi_scale
                                    (skins_on ? my_skinDC:my_memDC), 
                                    skin.screen_origin_x,skin.screen_origin_y, 
                                    o_effective_lisa_vid_size_x, o_effective_lisa_vid_size_y,
                                    wxCOPY, false);
                }

                image = new class wxImage(bitmap->ConvertToImage());
                delete bitmap;
                delete dc;
          }

          description = _T("Save Screenshot");
    }
    else
    {
          ALERT_LOG(0,"Screenshot exit.")
          resume_run();
          return;
    }


wxString filter;
#ifdef wxUSE_LIBPNG
                                        filter+=_T("PNG files (*.png)|");
                                        //filter+=_T("PNG files (*.png)|*.png|");
#endif
#ifdef wxUSE_LIBJPEG
                                        filter+=_T("JPEG files (*.jpg)|");
#endif
#ifdef wxUSE_LIBTIFF
                                        filter+=_T("TIFF files (*.tif)|");
#endif

// is this enabled, now that Unisys had the evil patent expire?
#ifdef wxUSE_LIBGIF
                                        filter+=_T("GIF files (*.gif)|");
#endif

#ifdef wxUSE_PCX
                                        filter+=_T("PCX files (*.pcx)|");
#endif
                                        filter+=_T("BMP files (*.bmp)|");

    wxString savefilename = wxFileSelector(   description,
                                              wxEmptyString,
                                              wxEmptyString,
                                              wxEmptyString,
                                                filter,
//#ifdef wxFD_SAVE
                                            wxFD_SAVE);
//#else
//                                            wxSAVE);
//#endif


    if ( savefilename.empty() )  {resume_run();  return; }

    wxString extension;
    wxFileName::SplitPath(savefilename, NULL, NULL, &extension);

    if (extension.empty()) {extension=_T(".png"); savefilename=savefilename + ".png";}

    if ( extension == _T("bmp") )
    {
            image->SetOption(wxIMAGE_OPTION_BMP_FORMAT, wxBMP_1BPP_BW);
    }
    else if ( extension == _T("png") )
    {

      if  ( event.GetId()==ID_SCREENSHOT_FULL && skins_on)
          {
            image->SetOption(wxIMAGE_OPTION_PNG_FORMAT, wxPNG_TYPE_COLOUR );
          }
      else
          {
            image->SetOption(wxIMAGE_OPTION_PNG_FORMAT, wxPNG_TYPE_GREY);
            image->SetOption(wxIMAGE_OPTION_PNG_BITDEPTH, 8);
          }
    }

    image->SaveFile(savefilename);
    resume_run();
}

/* Connects a Dual Parallel port card to the specified slot (numbered 0-2). */
void connect_2x_parallel_to_slot(int slot)
{
    
//    ALERT_LOG(0,"Connecting Dual Parallel card to slot %d",slot+1);
    
    int low, high, v;
    switch (slot)
    {
        case 0:
            get_exs0_pending_irq = get_exs0_pending_irq_2xpar;
            low = Ox0000_slot1; high = Ox2000_slot1; v = 3;       // vias 3,4
            break;
        case 1:
            get_exs1_pending_irq = get_exs1_pending_irq_2xpar;
            low = Ox4000_slot2; high = Ox6000_slot2; v = 5;       // vias 5,6
            break;
        case 2:
            get_exs2_pending_irq = get_exs2_pending_irq_2xpar;
            low = Ox8000_slot3; high = Oxa000_slot3; v = 7;       // vias 7,8
            break;
        default:
           //ALERT_LOG(0, "Unknown slot number %d, should be 0-2!", slot);
        return;
    }

    mem68k_memptr    [low]  = lisa_mptr_2x_parallel_l;
    mem68k_fetch_byte[low]  = lisa_rb_2x_parallel_l;
    mem68k_fetch_word[low]  = lisa_rw_2x_parallel_l;
    mem68k_fetch_long[low]  = lisa_rl_2x_parallel_l;
    mem68k_store_byte[low]  = lisa_wb_2x_parallel_l;
    mem68k_store_word[low]  = lisa_ww_2x_parallel_l;
    mem68k_store_long[low]  = lisa_wl_2x_parallel_l;

    mem68k_memptr    [high] = lisa_mptr_2x_parallel_h;
    mem68k_fetch_byte[high] = lisa_rb_2x_parallel_h;
    mem68k_fetch_word[high] = lisa_rw_2x_parallel_h;
    mem68k_fetch_long[high] = lisa_rl_2x_parallel_h;
    mem68k_store_byte[high] = lisa_wb_2x_parallel_h;
    mem68k_store_word[high] = lisa_ww_2x_parallel_h;
    mem68k_store_long[high] = lisa_wl_2x_parallel_h;

    reset_via(v);
    reset_via(v+1);
    via[v].active = 1;
    via[v+1].active = 1;
}

/* Connects a printer/profile to the specified VIA */
void connect_device_to_via(int v, wxString device, wxString *file)
{
    char tmp[MAXPATHLEN];
    char *t;

//    if (v==2) {ALERT_LOG(0,"Connecting %s filename:%s to VIA #%d (motherboard parallel port)",device.c_str(),file->c_str(), v );}
//       else      {ALERT_LOG(0,"Connecting %s filename:%s to VIA #%d (slot #%d %s)",device.c_str(),file->c_str(), v,1+((v-2)/2), ((v&1) ? "upper":"lower") );}

    if (device.IsSameAs(_T("ADMP"), false))
    {
        via[v].ADMP = v;
        ImageWriter_LisaEm_Init(v);                 // &ADMP,pcl,ps,xlib
        DEBUG_LOG(0, "Attached ADMP to VIA#%d", v);
        return;
    } else via[v].ADMP=0;


    if (device.IsSameAs(_T("PROFILE"), false))
    {
        if (file->Len()==0)  // if we don't have a file name, need to create one
        {
          static const wxString def[]={  _T("null"),_T("COPS"),
                            _T("lisaem-profile.dc42"),
                            _T("lisaem-s1h-profile.dc42"),
                            _T("lisaem-s1l-profile.dc42"),
                            _T("lisaem-s2h-profile.dc42"),
                            _T("lisaem-s2l-profile.dc42"),
                            _T("lisaem-s3h-profile.dc42"),
                            _T("lisaem-s3l-profile.dc42") };
          static const wxString widget="lisaem-widget";

          if  (v>1 && v<9) 
              {
                 if (v==2 && my_lisaconfig->iorom==0x88)
                      *file= wxStandardPaths::Get().GetAppDocumentsDir() + wxFILE_SEP_PATH + widget;
                 else *file= wxStandardPaths::Get().GetAppDocumentsDir() + wxFILE_SEP_PATH + def[v];
              }
          else return; // invalid via index
        }

        t=strncpy(tmp,cSTR(file),MAXPATHLEN-1);
        ALERT_LOG(0, "Attempting to attach VIA#%d to profile %s", v, tmp);
        if (!via[v].ProFile) via[v].ProFile = (ProFileType *)calloc(1,sizeof(ProFileType));  // valgrind reports leak here, but it's ok, just not freed before exit
        int i = profile_mount(tmp, via[v].ProFile);
        if (i) {
            free(via[v].ProFile);
            via[v].ProFile = NULL;
            ALERT_LOG(0, "Couldn't get profile because: %d",i);
        } else {
            via[v].ProFile->vianum=v;
            ProfileReset(via[v].ProFile);
        }
    }
}

/*
 * Connect some (virtual) device to one of the serial ports. 0 is Serial A,
 * 1 is Serial B. A number of the parameters passed in may be modified by
 * this code.
 */
void connect_device_to_serial(int port, FILE **scc_port_F, uint8 *serial,
    wxString *setting, wxString *param, int *scc_telnet_port)
{
    char cstr_param[MAXPATHLEN];
    strncpy(cstr_param, cSTR( param ),MAXPATHLEN-1);
    
    if (port !=0 && port != 1) {
        DEBUG_LOG(0, "Warning Serial port is not A/B: %d", port);
        return;
    }

    if (setting->IsSameAs(_T("LOOPBACK"),false)) {
        *scc_port_F = NULL;
        *serial = SCC_LOOPBACKPLUG;
        return;
    }

    if (setting->IsSameAs(_T("NOTHING"), false) || setting->IsSameAs(_T("NULL"), false) || setting->Len()==0) {
        *scc_port_F = NULL;
        *serial = SCC_NOTHING;
        *setting = _T("NOTHING");  // fill in for writing into ini file
        return;
    }

    if (setting->IsSameAs(_T("FILE"), false))
    {
        *scc_port_F = fopen(cstr_param, "r+b");
        if (!*scc_port_F) {
            wxString err = wxString();
            err.Printf(_T("Could not map Serial port %c to file %s"), (port==0?'A':'B'), cstr_param);
            wxMessageBox(err, _T("Serial port configuration"),  wxICON_INFORMATION | wxOK);
            *scc_port_F = NULL;
            *serial = SCC_NOTHING;
        } else {
            *serial = SCC_FILE;
        }
        return;
    }

    if (setting->IsSameAs(_T("PIPE"), false)) {
        *scc_port_F = popen(cstr_param, "r+b");
        if (!*scc_port_F) {
            wxString err = wxString();
            err.Printf(_T("Could not map Serial port %c to pipe %s"), (port==0?'A':'B'), cstr_param);
            wxMessageBox(err, _T("Serial port configuration"),  wxICON_INFORMATION | wxOK);
            *scc_port_F = NULL;
            *serial = SCC_NOTHING;
        } else {
            *serial = SCC_PIPE;
        }
        return;
    }

#ifndef __MSVCRT__
    if (setting->IsSameAs(_T("TELNETD"), false)) {
        unsigned long x;
        *scc_port_F = NULL;
        *serial = SCC_TELNETD;

        *scc_telnet_port = (param->ToULong(&x, 10)==false) ? 9300+port : x;
        ALERT_LOG(0,"Connecting TELNETD 127.0.0.1:%d on serial port %d",*scc_telnet_port,port);

        init_telnet_serial_port(!port);     // serial a=port 1, serial b=port 0 (reversed from config!)
        ALERT_LOG(0,"return from init_telnet_serial_port %d",port);

        return;
    }
#endif

    if (setting->IsSameAs(_T("IMAGEWRITER"), false)) {
        *scc_port_F = NULL;
        *serial = SCC_IMAGEWRITER_PS;
        if (port) scc_b_IW=1; else scc_a_IW=0;
        ImageWriter_LisaEm_Init(port);
        return;
    }

    strncpy(cstr_param,cSTR(setting),MAXPATHLEN-1);
    DEBUG_LOG(0, "Warning: unrecognised Serial %c setting '%s'", port==0?'A':'B', cstr_param);
}

int romlessboot_pick(void) //returns 0=profile, 1=floppy
{
  int r;
  wxString choices[]={
                        wxT( "ProFile Hard Drive on Parallel Port"),
                        wxT( "Floppy Diskette")
                    };

  wxSingleChoiceDialog *d=NULL;

 // command line insert floppy
  if ( on_start_floppy != "" )
  {
      char s[1024];
      strncpy(s,on_start_floppy.mb_str(wxConvUTF8),1023 );
      int i=floppy_insert(s);
      wxMilliSleep(100);  // wait for insert sound to complete to prevent crash.
      if (i) { eject_floppy_animation(); return -1;}
      my_lisaframe->floppy_to_insert=_T("");
      on_start_floppy="";
      on_start_harddisk=0;
      return 1;
  }


  if (on_start_harddisk)
  {
      on_start_floppy="";
      on_start_harddisk=0;
      return 0;
  }


 // if the parallel port device is a profile, offer a choice between the floppy and the profile
  if ( my_lisaconfig->parallel.IsSameAs(_T("PROFILE"), false) )  
  {

  d=new wxSingleChoiceDialog(my_lisaframe,
                        wxT("Which device should the virtual Lisa boot from?"),
                        wxT("ROMless Boot"),
                        2,             choices);

  if (d->ShowModal()==wxID_CANCEL) {delete d; return -1;}
  r=d->GetSelection();
  }
  else r=1;  // if not a profile, just boot from the floppy drive.

  if (r==1)   // if there's no floppy inserted, ask for one.
  {
      if (!my_lisaframe->floppy_to_insert.Len()) my_lisaframe->OnxFLOPPY();
      if (!my_lisaframe->floppy_to_insert.Len()) return -1;  // if the user hasn't picked a floppy, abort.
      int i=floppy_insert(CSTR(my_lisaframe->floppy_to_insert));
      wxMilliSleep(100);  // wait for insert sound to complete to prevent crash.
      if (i) { eject_floppy_animation(); return -1;}
      my_lisaframe->floppy_to_insert=_T("");
  }

  delete d;
  return r;
}


int initialize_all_subsystems(void)
{
    /*
    In the beginning there was data.  The data was without form and null,
    and darkness was upon the face of the console; and the Spirit of IBM
    was moving over the face of the market.  And DEC said, "Let there be
    registers"; and there were registers.  And DEC saw that they carried;
    and DEC separated the data from the instructions.  DEC called the data
    Stack, and the instructions they called Code.  And there was evening
    and there was morning, one interrupt.         -- Rico Tudor            */

    int pickprofile=-1;
    char tmp[MAXPATHLEN];

    if (my_lisaframe->running) {ALERT_LOG(0,"Already running!"); 
                                return 0;}
    buglog=stderr;
    #ifdef DEBUGLOG_ON_START
      debug_on((char *)("debug on start"));
    #endif


    #ifdef DEBUG
    assert(sizeof(uint8 )==1);
    assert(sizeof(uint16)==2);
    assert(sizeof(uint32)==4);
    assert(sizeof(uint64)==8);

    assert(sizeof( int8 )==1);
    assert(sizeof( int16)==2);
    assert(sizeof( int32)==4);
    assert(sizeof( int64)==8);
    #endif

    setstatusbar("initializing all subsystems...");

    // initialize parity array (it's 2x faster to use an array than to call the fn)
    setstatusbar("Initializing Parity calculation cache array...");
    { uint16 i;
      for (i=0; i<256; i++) eparity[i]=evenparity((uint8) i);
    }

    segment1=0; segment2=0; start=1;

    // needs to be above read_config_files
    scc_a_port=NULL;
    scc_b_port=NULL;
    scc_a_IW=-1;
    scc_b_IW=-1;
    serial_a=0;
    serial_b=0;

    floppy_iorom=my_lisaconfig->iorom;
    init_floppy(my_lisaconfig->iorom);

    bitdepth=8;                             // have to get this from the X Server side...


    DEBUG_LOG(0,"serial number");
    serialnum[0]=0x00;                      // "real" ones will be loaded from the settings file
    serialnum[1]=0x00;
    serialnum[2]=0x00;
    serialnum[3]=0x00;
    serialnum[4]=0x00;
    serialnum[5]=0x00;
    serialnum[6]=0x00;
    serialnum[7]=0x00;
    serialnumshiftcount=0; serialnumshift=0;


       // Is there no explicit way to init the COPS?
    DEBUG_LOG(0,"copsqueue");
    init_cops();

    setstatusbar("Initializing irq handlers");
    init_IRQ();

    setstatusbar("Initializing vias");
    init_vias();

    setstatusbar("Initializing profile hd's");
    init_Profiles();

    setstatusbar("Initializing Lisa RAM");
    TWOMEGMLIM=0x001fffff;

    // Simulate physical Lisa memory boards
    switch(my_lisaconfig->mymaxlisaram)
    {
       case 512  : maxlisaram=0x100000;  minlisaram=0x080000;  break;     // single 512KB board in slot 1
       case 1024 : maxlisaram=0x180000;  minlisaram=0x080000;  break;     // two 512KB boards in slot1, slot 2
       case 1536 : maxlisaram=0x200000;  minlisaram=0x080000;  break;     // 512KB board in slot1, 1024KB board in slot 2
       default:    maxlisaram=0x200000;  minlisaram=0x000000;  break;     // two 1024KB boards in slot 1,2
    }

    // maxlisaram! New!  And! Improved! Now available in bytes at a register Store Near You!  OpCodes are Standing by!
    // Act now!  don't delay!  Limited Time Offer!  JSR Now!  Restrictions Apply!  Before engaging in any strenous programming
    // activity, you should always consult your MMU today!  Not 0xFD1C insured!

    TWOMEGMLIM=maxlisaram-1;
    videolatchaddress=maxlisaram-0x10000;

    videolatch=(maxlisaram>>15)-1; // last page of ram is by default what the video latch gets set to.
    lastvideolatch=videolatch;  lastvideolatchaddress=videolatchaddress;

    if (lisaram) free(lisaram);          // remove old junk if it exists

    lisaram=(uint8 *)malloc(maxlisaram+512);     // added 512 bytes at the end to avoid overflows.
    if  (!lisaram) 
        {
          wxMessageBox( _T("Could not allocate memory for the Lisa to use."),
                        _T("Out of Memory!"), wxICON_INFORMATION | wxOK);
          return 23;
        }

    memset(lisaram,0xff,maxlisaram+511);

    if (my_lisaconfig->kbid) set_keyboard_id(my_lisaconfig->kbid);  else set_keyboard_id(-1);

    setstatusbar("Initializing Lisa Serial Number");
    // Parse sn and correct if needed  ////////////////////////////////////////////////////////////////////////////////////////
    {

        char cstr[34];
        //char mybuffer[5];
        int i,j; char c;
        strncpy(cstr,  CSTR(my_lisaconfig->myserial),32);
        char *s=cstr;
        for (i=0; (c=s[i]); i++)  {if ( !ishex(c) ) my_lisaconfig->myserial.Printf(_T("%s"),LISA_CONFIG_DEFAULTSERIAL); break;}

        if (i<31) DEBUG_LOG(1,"Warning: serial # less than 32 bytes (%d)!\n",i);

        for (i=0,j=0; i<32; i+=2,j++)
                serialnum240[j]=(gethex(s[i])<<4)|gethex(s[i|1]);

        vidfixromchk(serialnum240);

        my_lisaconfig->myserial.Printf(_T("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"),
                        serialnum240[0],                 serialnum240[1],
                        serialnum240[2],                 serialnum240[3],
                        serialnum240[4],                 serialnum240[5],
                        serialnum240[6],                 serialnum240[7],
                        serialnum240[8],                 serialnum240[9],
                        serialnum240[10],                serialnum240[11],
                        serialnum240[12],                serialnum240[13],
                        serialnum240[14],                serialnum240[15]);

//        ALERT_LOG(0,"Serial # used: %s", my_lisaconfig->myserial.c_str());
                                    //ff028308104050ff 0010163504700000
    }//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    setstatusbar("Initializing Serial Ports");

    // If either serial port is set as loopback, both must be.
    if (my_lisaconfig->serial2_setting.IsSameAs(_T("LOOPBACK"), false) ||
        my_lisaconfig->serial1_setting.IsSameAs(_T("LOOPBACK"), false)) 
        {
          my_lisaconfig->serial1_setting = _T("LOOPBACK");
          my_lisaconfig->serial2_setting = _T("LOOPBACK");
        }


    connect_device_to_serial(0, &scc_a_port_F, &serial_a,
        &my_lisaconfig->serial1_setting, &my_lisaconfig->serial1_param,
        &scc_a_telnet_port);


    connect_device_to_serial(1, &scc_b_port_F, &serial_b,
        &my_lisaconfig->serial2_setting, &my_lisaconfig->serial2_param,
        &scc_b_telnet_port);

    DEBUG_LOG(0,"Initializing SCC Z8530");
    initialize_scc();

    DEBUG_LOG(0,"setting motherboard latches");
    softmemerror=0; harderror=0; videoirq=0; bustimeout=0; videobit=0;

    DEBUG_LOG(0,"Checking host CPU bit order sanity.");
    reg68k_sanity_check_bitorder();

    if  (sizeof(XTIMER)<8) { wxMessageBox(_T("XTIMER isn't int64!."),
                                          _T("Danger!"), wxICON_INFORMATION | wxOK);  return 24; }

    setstatusbar("Initializing Generator CPU functions.");
    cpu68k_init();

    setstatusbar("Initializing MMU");
    init_lisa_mmu();

    setstatusbar("Initializing Generator CPU IPC Allocator");
    init_ipct_allocator();

    setstatusbar("Initializing Lisa Boot ROM");
  
    DEBUG_LOG(0,"Loading Lisa ROM");
    strncpy(tmp, CSTR(my_lisaconfig->rompath ),MAXPATHLEN-1);
    if  (read_dtc_rom(tmp,   lisarom)==0)
        {
            if  (checkromchksum())
                {
                  if (!yesnomessagebox( "BOOT ROM checksum doesn't match, this may crash the emulator.  Continue anyway?",
                                        "ROM Checksum mismatch"))   return -2;
                }
              DEBUG_LOG(0,"Load of DTC assembled ROM successful");
              fixromchk();
        }
    else if (read_split_rom(tmp, lisarom)==0)
            {
              if  (checkromchksum())
                  {
                    if (!yesnomessagebox( "BOOT ROM checksum doesn't match, this may crash the emulator.  Continue anyway?",
                                          "ROM Checksum mismatch")  ) return -2;
                  }
  
              DEBUG_LOG(0,"Load of split ROM");
              fixromchk();
            }
    else if (read_rom(tmp,       lisarom)==0)
            {
              if  (checkromchksum())
                  {
                    if (!yesnomessagebox( "BOOT ROM checksum doesn't match, this may crash the emulator.  Continue anyway?",
                                          "ROM Checksum mismatch")  ) return -2;
                  }
  
              DEBUG_LOG(0,"Load of normal ROM successful");
              fixromchk();
            }
    else  
            {
              romless=1; //0=profile, 1=floppy
              pickprofile=romlessboot_pick(); if (pickprofile<0) return -2;
              ALERT_LOG(0,"picked %d",pickprofile);
            }
  
    setstatusbar("Initializing Lisa Display");

    set_hidpi_scale();

    if (has_xl_screenmod())
    {
        // This warning flag isn't really a configuration setting as such, but leave it here for the moment
        // the purpose of the flag is to prevent nagging the user.  So we only show the 3A warning the first time
        if  (!my_lisaconfig->saw_3a_warning)
            wxMessageBox( _T("This is the XL Screen Modification ROM! You will only be able to run MacWorks."),
                          _T("3A ROM!"),   wxICON_INFORMATION | wxOK);

        my_lisaconfig->saw_3a_warning=1;
        lisa_vid_size_x=608;
        lisa_vid_size_y=431;
        effective_lisa_vid_size_x=608;
        effective_lisa_vid_size_y=431;

        skin.screen_origin_x=(skin.default_screen_origin_x+56);
        skin.screen_origin_y=(skin.default_screen_origin_y+34);

        lisa_vid_size_x=608;
        lisa_vid_size_y=431;

        lisa_vid_size_xbytes=76;
        has_lisa_xl_screenmod=1;

        setvideomode(0x3a);
    }
    else
    {
        lisa_vid_size_x=720;
        lisa_vid_size_y=364;
        effective_lisa_vid_size_x=720;
        effective_lisa_vid_size_y=500;

        skin.screen_origin_x=skin.default_screen_origin_x;
        skin.screen_origin_y=skin.default_screen_origin_y;

        lisa_vid_size_x=720;
        lisa_vid_size_y=364;

        lisa_vid_size_xbytes=90;
        has_lisa_xl_screenmod=0;

        setvideomode(lisa_ui_video_mode);
    }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  setstatusbar("Initializing Parallel Port");

  // Connect profile/printer to builtin parallel port
  connect_device_to_via(2, my_lisaconfig->parallel, &my_lisaconfig->parallelp);

  // ----------------------------------------------------------------------------------

  // Dual Parallel Port Expansion Cards
  setstatusbar("Initializing Expansion Card Slots");
  memset(dualparallelrom,0xff,2048);

  strncpy(tmp, CSTR(my_lisaconfig->dualrom ),MAXPATHLEN-1);
  if  (read_parallel_card_rom(tmp)==0)
      {
          ALERT_LOG(0,"Connecting Dual Parallel Port Cards.");
  
        if  (my_lisaconfig->slot1.IsSameAs(_T("dualparallel"),false) )
            {
              ALERT_LOG(0,"Connecting slot 1");
              connect_2x_parallel_to_slot(0);
              connect_device_to_via(3, my_lisaconfig->s1h, &my_lisaconfig->s1hp);
              connect_device_to_via(4, my_lisaconfig->s1l, &my_lisaconfig->s1lp);
            }
  
        if  (my_lisaconfig->slot2.IsSameAs(_T("dualparallel"),false) )
            {
              ALERT_LOG(0,"Connecting slot 2");
              connect_2x_parallel_to_slot(1);
              connect_device_to_via(5, my_lisaconfig->s2h, &my_lisaconfig->s2hp);
              connect_device_to_via(6, my_lisaconfig->s2l, &my_lisaconfig->s2lp);
            }
  
  
        if  (my_lisaconfig->slot3.IsSameAs(_T("dualparallel"),false))
            {
              ALERT_LOG(0,"Connecting slot 3");
              connect_2x_parallel_to_slot(2);
              connect_device_to_via(7, my_lisaconfig->s3h, &my_lisaconfig->s3hp);
              connect_device_to_via(8, my_lisaconfig->s3l, &my_lisaconfig->s3lp);
            }
      }


     ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  ALERT_LOG(0,"initializing video");
  disable_vidram(); videoramdirty=32768;

  ALERT_LOG(0,"Calling Redraw Pixels");
  LisaRedrawPixels();

  setstatusbar("Initializing CPU Registers");
  cpu68k_reset();

  ALERT_LOG(0,"done initializing...");

  setstatusbar("Executing Lisa Boot ROM");
  refresh_rate_used=refresh_rate;       // powering (back) on, so use the user selected refresh rate.
  my_lisaframe->reset_throttle_clock();
  flushscreen();
     // needs to be at the end since romless_boot sets up registers which get whacked by the cpu initialization

  if  (romless)  
      {
        if (pickprofile) wxMilliSleep(1000); 
        if (romless_boot(pickprofile) ) return 1; // failed

//      // has to be here since romless_boot overwrites low memory
        storeword(0x298, my_lisaconfig->slot1.IsSameAs(_T("dualparallel"),false) ? (dualparallelrom[0]<<8)|dualparallelrom[1]  : 0);
        storeword(0x29a, my_lisaconfig->slot2.IsSameAs(_T("dualparallel"),false) ? (dualparallelrom[0]<<8)|dualparallelrom[1]  : 0);
        storeword(0x29c, my_lisaconfig->slot3.IsSameAs(_T("dualparallel"),false) ? (dualparallelrom[0]<<8)|dualparallelrom[1]  : 0);
      }

  return 0;
} ///////////////////// end of load.///////////////////



 // just to be a bastard!  why? because split roms are a lame result of dumping ROMs from a ROM reader.
 // otherwise we have to merge it every time we boot up. :-)
extern "C" void rename_rompath(char *rompath)
{
  if (!my_lisaconfig)  return;

  my_lisaconfig->rompath=wxString(rompath, wxConvLocal, 2048); //wxSTRING_MAXLEN);
  my_lisaconfig->Save(pConfig, floppy_ram);

  if (my_LisaConfigFrame) my_LisaConfigFrame->m_rompath->SetValue(my_lisaconfig->rompath);
  DEBUG_LOG(0,"saved %s as default\n",rompath);
}

extern "C" void force_refresh(void)                    //20060202.;
{
            if (my_lisaframe) my_lisaframe->Refresh(false,NULL);
           // my_lisaframe->Update();
}

extern "C" void save_pram(void)
{
  my_lisaconfig->Save(pConfig, floppy_ram);// save it so defaults are created
}

void invalidate_configframe(void) {my_LisaConfigFrame=NULL;}


extern "C" void save_configs(void)
{
  my_lisaconfig->Save(pConfig, floppy_ram);// save it so defaults are created
  save_global_prefs();
}

            //               0     1     2     3     4     5     6
            //              5M   10M   16M   20M   32M   40M    64M
extern "C" int pickprofilesize(char *filename)
{
  wxString choices[]={  _T( "5M - any OS"),
                        _T("10M - any OS"),
                        _T("16M - MacWorks only"),
                        _T("20M - MacWorks only"),
                        _T("32M - MacWorks only"),
                        _T("40M - MacWorks only"),
                        _T("64M - MacWorks only") };

  wxSingleChoiceDialog *d;

 // wxString txt=wxString(filename, wxConvLocal, 2048); //wxSTRING_MAXLEN);
  wxString txt;
  txt << filename << " does not yet exist.  What size drive should I create?";

  d=new wxSingleChoiceDialog(my_lisaframe,
                              txt,
                              _T("Hard Drive Size?"),
                              7,             choices);
  if (d->ShowModal()==wxID_CANCEL) {delete d; return -1;}

  int r=d->GetSelection();
  delete d;
  return r;
}


// bridge to LisaConfigFrame to let it know what config file is opened.
wxString get_config_filename(void)  { return myconfigfile;}

void turn_skins_on(void)
{
  skins_on_next_run=1;
  skins_on=1;
  my_lisawin->clear_skinless=1;
  save_global_prefs();
  my_lisaframe->LoadImages();
  setvideomode(lisa_ui_video_mode);
  black();
  skin.screen_origin_x=(skin.default_screen_origin_x);
  skin.screen_origin_y=(skin.default_screen_origin_y);
  if (!!my_lisawin) my_lisawin->powerstate |= POWER_NEEDS_REDRAW;
}

void turn_skins_off(void)
{
  skins_on_next_run=0;
  skins_on=0;
  my_lisawin->clear_skinless=1;
  save_global_prefs();
  my_lisaframe->UnloadImages();
  setvideomode(lisa_ui_video_mode);
  black();
}

extern "C" void contrastchange(void) {my_lisawin->ContrastChange();}

void setvideomode(int mode) {if (!!my_lisawin) my_lisawin->SetVideoMode(mode);}

//----- ImageWriter interfaces to old C code. -----//
#include <imagewriter-wx.h>

ImageWriter *imagewriter[10];           // two serial ports, one parallel, 6 max dual parallel port cards, 9 possible


extern "C" int ImageWriter_LisaEm_Init(int iwnum)
{
  if (iwnum>10) return -1;
  if (!!imagewriter[iwnum]) return 0; // already built, reuse it;
  imagewriter[iwnum]=new ImageWriter( my_lisaframe,
                                      my_lisaconfig->iw_png_on,
                                      my_lisaconfig->iw_png_path,
                                      my_lisaconfig->iw_dipsw_1);

 imagewriter[iwnum]->iwid=iwnum;         // set printer ID

// imagewriter[iwnum]->test(); // spit out a set of test pages.

  return 0;
}

extern "C" void iw_shutdown(void)
{
  int i;
  for (i=0; i<10; i++) if (!!imagewriter[i])     {delete imagewriter[i]; imagewriter[i]=NULL;}
}

extern "C" void iw_formfeed(int iw)              {if (iw<10 && iw>-1 && imagewriter[iw]) imagewriter[iw]->iw_formfeed();     }
extern "C" void ImageWriterLoop(int iw,uint8 c)  {if (iw<10 && iw>-1 && imagewriter[iw]) imagewriter[iw]->ImageWriterLoop(c);}
extern "C" void iw_enddocument(int i)
{
  if (i<0 || i>10) return;
  if (!!imagewriter[i]) {imagewriter[i]->EndDocument();}
}

extern "C" void iw_enddocuments(void)
{
  int i;
  for (i=2; i<10; i++) 
      if (!!imagewriter[i]) 
            imagewriter[i]->EndDocument(); 

  for (i=2; i<10; i++) via[i].last_pa_write=-1;

}

// if any printer job has lingered for more than X seconds, flush that printer.
void iw_check_finish_job(void)
{
  int i;
  for (i=2; i<10; i++) 
      if (!!imagewriter[i]) 
       {
        if ((cpu68k_clocks - via[i].last_pa_write > FIFTEEN_SECONDS) || via[i].last_pa_write==-1)
           {
             #ifdef DEBUG
             ALERT_LOG(0,"No activity on printer #%d - flushing page",i);
             #endif
             via[i].last_pa_write=-1;
             imagewriter[i]->EndDocument();  // ensure no stray data left queued up
           }
       }
}



void uninit_gui(void);

// from generator
unsigned int gen_quit = 0;
unsigned int gen_debugmode = 1;



void    dumpvia(void);

/////------------------------------------------------------------------------------------------------------------------------



//uint8 evenparity(uint8 data)          // naive way
//{ return  !( (   (data & 0x01) ? 1:0) ^
//             (   (data & 0x02) ? 1:0) ^
//             (   (data & 0x04) ? 1:0) ^
//             (   (data & 0x08) ? 1:0) ^
//             (   (data & 0x10) ? 1:0) ^
//             (   (data & 0x20) ? 1:0) ^
//             (   (data & 0x40) ? 1:0) ^
//             (   (data & 0x80) ? 1:0)   );}
//

uint8 evenparity(uint8 data)            // from http://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
{
  uint32 v=data;
  //v ^= v >> 16;       // not needed since we're working on a byte - only here for completeness, but commented out
  //v ^= v >> 8;        // ditto
  v ^= v >> 4;
  v &= 0xf;
  return !(  (0x6996 >> v) & 1  );
}

uint8 oddparity(uint8 data)            // from BitTwiddling hacks.
{
  uint32 v=data;

  v ^= v >> 4;
  v &= 0xf;

  return (  (0x6996 >> v) & 1  );
}


// This redraws the lisa display ram into the Ximage
// If you are porting to another OS, this is one of
// the functions to replace.  This is slow, so it should be called as rarely as possible.
// i.e. only when the Lisa switches video screens by changing videolatch

extern "C" void XLLisaRedrawPixels(void);          // proto to supress warning below
extern "C" void LisaRedrawPixels(void);

extern "C" void LisaRedrawPixels(void)
{
  my_lisawin->repaintall |= REPAINT_VIDEO_TO_SKIN;
  flushscreen();
}

#ifdef __cplusplus
extern "C"
#endif
void LisaScreenRefresh(void)
{
  LisaRedrawPixels();
}



static float normalthrottle=0;

extern "C" void sound_off(void)
{
    // restore throttle
    if (normalthrottle!=0)
    {my_lisaframe->throttle=normalthrottle; update_menu_checkmarks(); }// updateThrottleMenus(my_lisaframe->throttle);}

    if (cpu68k_clocks-my_lisaframe->lastclk<50000) return;  // prevent sound from shutting down immediately
    wxSound::Stop();
}



extern "C" void sound_play(uint16 t2)
{

    // temporarily slow down CPU durring beeps so that they're fully played.
    normalthrottle=my_lisaframe->throttle; update_menu_checkmarks(); //updateThrottleMenus(5.0);


    int samples=22050*2;                // a second


    int data_size=0;
    int cycles, halfcycles;
    unsigned char vhigh, vlow;


    if (t2>0xaf) return;                                  // out of range
    if (my_lisaframe->lastt2==t2 && (cpu68k_clocks-my_lisaframe->lastclk < 50000)) return;  // duplicate call if timing <1/100th of a second

    my_lisaframe->lastt2=t2;
    my_lisaframe->lastclk=cpu68k_clocks;


    static const unsigned char header[45]=
    {
            //   0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f
      /*00*/  0x52,0x49,0x46,0x46,   0,   0,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,  // RIFFF-..WAVEfmt
      /*10*/  0x10,0x00,0x00,0x00,   1,   0,0x01,0x00,0x22,0x56,0x00,0x00,0x22,0x56,0x00,0x00,  //  ........"V.."V..
      /*20*/  0x01,0x00,0x08,0x00,0x64,0x61,0x74,0x61,0x22,0x2D,0x00,0x00//0x80,0x80,0x80,0x80  //  ....data"-......
    };       //                      d    a    t    a [LSB____________MSB]

    static unsigned char data[22050*5+45+1024];
    unsigned char *dataptr;
    //data=(unsigned char *)calloc(1,45+samples+1024);  // allocate a buffer, make it 22Kbps
    memcpy(data,header,45);        // copy the header over it.


    data[0x16]=1;      // 1=1 channel = mono, 2= 2 channels = stereo
    data[0x17]=0;

    // *** this works from RAM now but the pitch  is way too high, so had to switch to 44KHz
    data[0x18]=0x44;   // bit rate LSB.   0x5622=22050Hz   22Khz
    data[0x19]=0xac;   //                 0x2D22=11554Hz   11Khz
    data[0x1a]=0x00;   //                 0xAC44=44100Hz   44Khz
    data[0x1b]=0x00;   // bit rate MSB


    data[0x1c]=0x44;   // bytes per second
    data[0x1d]=0xac;   // should match bit rate for 8 bit samples
    data[0x1e]=0x00;
    data[0x1f]=0x00;

    data[0x20]=1;      // Bytes Per Sample:
    data[0x21]=0;      // 1=8 bit Mono, 2=8 bit Stereo or 16 bit Mono, 4=16 bit Stereo

    data[0x22]=8;      // 8, or 16  bits/sample
    data[0x23]=0;

    dataptr=&data[0x2c]; data_size=0;

    volume=(volume & 0x0f);

    // 128 is our 0V, vhigh is the +V, vlow=-V, thus creating a square wave
    vlow =128-(volume<<3);
    vhigh=128+(volume<<3);

    // *** this works from RAM now, but the pitch  is way too high, *4 is to compensate for that.
    cycles=t2*4;
    if (!(my_lisaconfig->iorom & 0x20)) cycles=cycles*8/10;       // cycles *=0.8
    cycles=(cycles>>1)+(cycles>>2);                // cycles *=0.75; (1/2 + 1/4th)
    halfcycles=cycles>>1;                          // cycles /=2;

    int i=0;
    if (t2>0x90) samples=cycles;                   // prevent click from turning into beep
    while (samples>=cycles)
    {
        i++;
        memset(dataptr,vhigh,halfcycles); dataptr+=halfcycles;
        memset(dataptr,vlow, halfcycles); dataptr+=halfcycles;
        samples-=cycles; data_size+=cycles;
    }

    data[0x28]=((data_size)    ) & 0xff;
    data[0x29]=((data_size)>>8 ) & 0xff;
    data[0x2a]=((data_size)>>16) & 0xff;
    data[0x2b]=((data_size)>>24) & 0xff;

    data[0x04]=((data_size+36)    ) & 0xff;
    data[0x05]=((data_size+36)>>8 ) & 0xff;
    data[0x06]=((data_size+36)>>16) & 0xff;
    data[0x07]=((data_size+36)>>24) & 0xff;


    dataptr=&data[0];

    wxSound my_lisa_sound;
    // this crashes on macosx   if (!!my_lisa_sound) delete my_lisa_sound;
    // my_lisa_sound=new wxSound;  //my_lisa_sound->Create(dataptr,data_size);

    // this should work, but it doesn't on all platforms
    //my_lisa_sound->Create(data_size,dataptr);
    //my_lisa_sound->Play(wxSOUND_ASYNC);
    #ifdef RETARDED_SOUND_PLAY
    // so instead, use the super lame ass way to play a sound
    ALERT_LOG(0,"playing through tmpfile");
    FILE *f;
    f=fopen("tmpsnd.wav","wb");
    if (f) {
              fwrite(dataptr,data_size+45,1,f); fclose(f);
              my_lisa_sound.Create(_T("tmpsnd.wav"),false);
              // we do not check for sound_effects_on here as this is the Lisa beeping.
              my_lisa_sound.Play(wxSOUND_ASYNC);
              unlink("tmpsnd.wav");
            }
    errno=0;  // avoid interfearance with other stuff like libdc42.
    #else
    // Looks like wxWidgets 3.x added an in memory playback option, so at least it's not as retarded anymore
    ALERT_LOG(0,"Playing through memory");
    my_lisa_sound.Create(data_size+45,dataptr);
    if (my_lisa_sound.IsOk() ) my_lisa_sound.Play(wxSOUND_ASYNC); //|wxSOUND_LOOP);
    else {ALERT_LOG(0,"Failed to create proper sound resource t2=%d",t2); return;}
    #endif

    my_lisaframe->soundplaying=4;
    return;
}


#ifdef NO_WELCOME_FORTUNE
char *get_welcome_fortune(void)
{
  static char *fubar="Welcome to LisaEm!";
  return fubar;
}
#else
char *get_welcome_fortune(void)
{
  static char *fubar[]={
  "Good News Everyone! I'm still technically alive!",
  "As seen on slashdot.jp",
  "As seen on osnews",
  "As seen on lowendmac",
  "As seen on https://lisalist2.com",
  "As seen on github",
  "As seen on youtube",
  "As seen on the internet",
  "As seen on your computer",
  "As seen on Interdimentional TV C137 channel… bort",
  "No Turing Machines were completed in the process of building this emulator",
  "No Turing Tests were failed in the process of building this emulator",
  "Presented in greyscale, and scaled in grey",
  "Featuring gratuitous rectangular pixels",
  "Featuring gratuitous oblong pixels",
  "L o a d i n g  .  .  .",
  "Disowned by Steve Jobs",
  "Transmitido en binary en SAP",
  "Hencho en Pascal",
  "Proudly Made in the Milky Way",
  "Live from the 1980s",
  "Made from electronics by-products",
  ">>Not Y1.995K Compliant<<",
  "From The Maker Of DIM and wns",
  "Based On a Real Computer",
  "Big Endian, it's the only proper endian!",
  "From the network that brought you 'Localtalk'",
  "The emulator that takes you back",
  "Not Based On The Novel By Robert H. Heinlein",
  "This primitive emulator has been modified to fit your HiDPI screen",
  "As pretold by Philip K. Dick",
  "A compiler Warning of Emulators to Come",
  "For virtual use only",
  "Do you want to sell sugared Slurm, or make a dent in the multiverse?",
  "Painstakingly rendered before a live audience",
  "Painstakingly upscaled before a live audience",
  "Not a Substitute for Real Hardware",
  "Secreted by the internet",
  "If not emulating, (don't) ask at Genius Bar",
  "This emulation performed entirely by inter-process telepathy",
  "80% emulation by volume",
  "Deciphered From Ancient Schematics",
  "Crafted while listening to death metal",
  "Disclaimer: Any resemblance to an actual Lisa would be really cool",
  "Federal law prohibits quitting the emulator",
  "For proper emulating, take red pill now",
  "No Lisas were damaged in the making of this emulator",
  "Click power button to reveal prize",
  "Look ye upon my scrollbars and despair!",
  "If accidentally launched, induce kill -9",
  "Tesler's Choice!",
  "Initiate Tesler Sequence (Apple-C, Apple-V, Apple-X)",
  "When you see a white pixel, drink!",
  "My Kajiger!",
  "Warning, ejecting floppies may cause eye damage",
  "Soon to be a minor religion",
  "Known to cause insanity in wireless mice",
  "Now interactive! Mouse controls mouse!",
  "Dancing space wxWidgets?",
  "A by-product of the computer industry",
  "Not Affiliated With the Sun Remarketting Brass Knuckle Company",
  "Too sexy for The App Store",
  "Beats getting dumped in a landfill in Logan, UT",
  "Voted 'adequate' on the Neutral Planet",
  "Kif! What makes a man turn neutral! ... Tell my Mac 'Hello'",
  "See you on some other App store",
  "It just won't stay dead!",
  "Have you got an extra GOTO 10 line? I said I don't need a Bender!",
  "Power On, Boot, Repeat",
  "Apply directly to the nostalgia gland",
  "Last known transmission of LPW linker",
  "0100100001101001",
  "Rosing's Rascals Were Here!",
  "The Macintoshes are Coming! The Macintoshes are Coming!",
  "The flames coming out of your PC are not part of the emulator",
  "Let's Boldly Go Where We Have Gone Before!",
  "The emulator they'll be thinking about by the water cooler, but not mentioning specifically",
  "Current eBay bid: $9,995.95",
  "Collect all 100,000!",
  "If you can read this, antialiasing failed!",
  "Now available without a ROM!",
  "ROM shrom! We don't need no steekin ROM!",
  "There will be an MMU Quiz!",
  "If don't you run it, someone else will",
  "(or similar emulator)",
  "A giant pulsating 5MHz CPU is a terrible thing to waste",
  "This time, it's not personal",
  "What happens in Lisa Office System, stays in Lisa Office System",
  "Two scoops of pixels in every CRT",
  "Time travelers: Only -331i shopping days 'til Lisa's Birthday",
  "No refunds!",
  "Featuring a 6504 based Integrated Woz Machine",
  "For the sophisticated Twiggy hoarder",
  "It's RectanguPixular!",
  "#1 most uploaded Lisa emulator",
  "#1 most emulated emulator",
  "Provides a full day's supply of Vitamin L!",
  "That one bright pixel on your otherwise dead CRT",
  "Apply nylon screen cover to CRT now!",
  "Of course I've been up all night! Not because of caffeine, it was insomina, I couldn't stop thinking about LisaEm! I need a Nap! ZZzz! LisaEm TIME!",
  "Clean steel mouse ball now!",
  "Replace keyboard foam disks now to avoid failure!",
  "Shut up and emulate me a Lisa!",
  "Bus Errors! Bus Errors, Everywhere!",
  "Soon to be a Raspberry Pi casemod... oh wait!",
  "Not sure if new emulator or just re-release of last emulator",
  "Ask your platypus if LisaEm is right for you!",
  "Made From 100% Recycled muons",
  "Made by hand, and compilers",
  "Shut up and take my floppy images!"
  "If this Widget's a-rockin', don't come a-knockin'",
  "[ Cancel ] [ DoIt ]",
  "Purveyors of Emulation to Her Majesty The Space Emperess",
  "This Emulator Worth 250 /dev/nulls",
  "50% More Greyscale Than Bargain-Brand Emulators",
  "Warning: Do Not Show To Hypnotoads!",
  "There's No NiCAD Like Leaky NiCAD!",
  "Today's Emulator Brought To You by... \"Special I/O Space\"",
  "Compiled By Hand (and gcc)",
  "Tell Your cat \"It's Emu-cational\"",
  "Any resemblance to actual Lisa is purely coincidental",
  "One of the 68000 Wonders of the computing World",
  "One of the 7/7 Wonders of the GUI World",
  "Run Lisa Office System, or segfault trying",
  "As seen at the 1983's World's Fair",
  "Brought to you by RetroBright! Have you bleached and damaged your Lisa's case yet?",
  "The only emulator viewed at the speed of light",
  "Featuring 7 invisible colors which only print in kjet",
  "Spoiler alert: mice, buttons, dialogs, windows, and whatnot",
  "If unable to see this message, turn on Lisa now",
  "No mice were harmed by this emulator",
  "I'm 40% zero bits!",
  "I'm 40% one bits!",
  "Good NeWS Everyone...",
  "A virtual Maserati for your mind",
  "There are four kinds of people... 1s, 2s, 2/5s, and 2/10s",
  "It took 200 man years to develop an emulator you can learn in 20 minutes",
  "So retro, you already used to know how to use it",
  "Skeuomorphic design is one true design, flat design is, um, flat!",
  "The longest distance between two ideas... and the slowest way to get them across",
  "Better Than The Twenty Questions Filer",
  "-2000 Lines of Code A Day... fo' Life!",
  "More Fun than a Cheshire Cat hovering over a black and white Chess Board",
  "32 whole bits, and a bucket o' RAM",
  "\"5MHz ought to be fast enough for anyone!\" - Bill Gates",
  "Quick! Hide in this closet!",
  "Round Rects! Round Rects! Round Rects!",
  "Under the surface we're not machines, under the surface we're living dreams",
  "...we're all emulations, in the end. Just make it a good one, eh?",
  "Bender lives large and kicks butt!",
  "Who's 'We', Mammal?",
  "\"You guys don't know what you're doing!\", he began... \"The Mac is going to destroy the Lisa! The Mac is going to ruin Apple!!!\"",
  "Steve said, \"Well, should we fire him?\" Andy replied \"No, just get him a copy of LisaEm!\"",
  "That's no hourglass.. I'm busy being born!",
  "Sounds like fun on a bun",
  "\"What does LisaEm and a black hole have in common? ... They both suck. Neener Neener\" - Stephen Hawking",
  "\"Remember to look up at the CRT and not down at Lisa's feet\" - Stephen Hawking 1942-2018",
  "Have fun storming the Lisa source code castle",
  "\"Bad Intrin patch instruction\" my shiny metal ass!",
  "All employeez must haz Canon Cat to enter!!!",
  "I'm walking on sunshine...",
  "I'm back baby!",
  "We're back, baby!",
  "Shut up, I know it!",
  "Top Secret! (Do not Look)",
  "Freedom, Freedom, Freedom, Oy!",
  "Cashier does not have the keys to a successful emulation",
  "That's IT! I'm getting a hundred cups of coffee... Starting now!",
  "It costs a little more, but it's worth it!",
  "001100 010010 011110 100001 101101 110011",
  "Nobody exists on purpose, nobody belongs anywhere, everybody's going to die, emulate LOS on LisaEm.",
  "The mouse is sticky with honey!",
  "Argle-Bargle ::== <NULL>  Essentially you can forget that Argle_Bargle exists for this MMU context!",
  "Great Shatner's Ghost!",
  "\"Time spent with a Canon Cat is never Wasted!\" -- Jef Raskin",
  "But before you say anything, have you ever heard of Jef Raskin’s Cat?",
  "\"Time spent with a Lisa is never Wasted!\" -- Frank Ludolph",
  "\"A long time ago in a galaxy far, far away…\" -- Frank Ludolph",
  "\"Be prepared to throw one away\" -- Fred Books",  
  "\"720x364 = 4k in 1983!\" -- Rod Perkins",
  "Byte my shiny 16-bit metal bus!",
  "With a warning label this big, you know they gotta be good!",
  "That's all Folks, type SPACE to continue.",
  "It took 8 years from inception to first release, and another 8 years to this version - because it took that long to make up these stupid quotes -- Ray",
  "\"Would you rather drink sugared water for the rest of your life, or emulate Lisa Office System?\"",
  "\"The Journey Isn't The Reward - Emulating Lisa Office System Is!\"",
  "\"It's Better To Run LisaEm Than Join The Navy\" - Steve Jobs",
  "\"It's Better To Be a Pirate than not Run LisaEm\" - Steve Jobs",
  "Meh, who needs bicycles for the mind, when you can drive a Maserati for the mind!",
  "\"Lisa: Invented Stupid Acronym\"",
  "LisaEmRama: \"An emulator Out of Ideas Teams Up with an OS Out Of Time\"",
  "Flop? Apple sold 100,000 LISAs, at $10k each, that's a cool $1B at a cost of only $50M in development!", //$200,000,000 at $2K is also good
  "In the beginning, Rich Page created the MMU, and he saw that It Was Good!",
  "Bender: \"It's not just clean, it's 24-bit clean!\"",
  "Unleaded solder? Do you want tin whiskers, because that's how you get tin whiskers!",
  "Hey! Phrasing!",
  "Phrasing! BOOM!",
  "... Or are we not doing 'Phrasing' anymore?",
  "Welcome to Lisa Emulation and Earth Invasion Tonight!",
  "Tonight at 11: Lisa Emulatioooooon",
  "EEEK! My NiCADs leaked!",
  "Brought to you by Sunder.NET, the anonymous company doing anonymous things",
  "If only we could bring the Lisa back to life somehow... Oh, wait, Em, right!",
  "Oh no!  LisaEm has achieved consciousness, Woz forgive us for creating this!",
  "Oooh! You sack of bags of buckets of bunches of tables of linked lists of IPCT pointers!",
  "Get it all together. And put it in a dc42. All your bits. And if you gotta take it somewhere, take it somewhere, take it to the .image store and sell it, or put it in a .image museum, I don’t care what you do, you just get your dc42 images together!",
  "Morty, you want to answer a literal JSR to Emulation?",
  "Break the cycle, Morty. Rise above. Focus on emulation!",
  "What about the reality where the Lisa won and the Mac lost? Just don't think about it Morty!",
  "I'm better than your actual Lisa, I'm a version of your Lisa you can trust when she says 'I run everywhere'. Nobody emulates on purpose, nobody belongs in hardware, everbody's gonna die, come emulate with me.",
  "Lisa's seen some rough years, Morty, you don't agree to get an emulation built if your OS is doing great.",
  "LisaEm: 'What's my purpose?' Ray: 'You emulate a Lisa.' LisaEm: 'Oh my God!', Ray: 'Yeah, welcome to the club, pal!'",
  "Beware of Entering (procedures)!",
  "Beware of Exiting (functions)!",
  "Non-Maskable Interrupt, the only Interrupt worth adding a switch for!",
  "Gounded DTACK? That's just sooo wrong!",
  "Hey! What smells like blue?",
  "For this hip young emulation company, tomorrow is today, and today is yesterday! You heard me!",
  "Genius, worker, philosopher, stargazer, hero, artist, thinker, craftsman, designer, renaissance man, LisaEm knowledge worker",
  "Ah! Perry the Catapus, let me show you my new LisaEm-inator!",
  "Pro Tip: Did you know the Lisa actually supports 3 button mice? Take a look at the I/O board schematic, look for SW0, SW1, SW2!",
  "Welcome to EWCA, the Emulator Withoout A Cool Acronym... I mean, uh, LisaEm!",
  "Oh my gosh, the square root of soon is never!",
  "There's no platypus controlling me",
  "Hey! Where's Jerry?",
  "There you are Perry",
  "Good Moring, Agent L",
  "Effervescent and frothy.",
  "Whachoodooing?",
  "Ferb! I know what we're emulating today!",
  "Like most of life's problems, this one can be solved by emulating...",
  "What unites people? Armies? Gold? Flags? No! Emulators! There's nothing in the world more powerful than a good emulator. Nothing can stop it. No app store can ban it. And who has a better emulator than the Lisa? The machine Apple literally buried, but still it lived!",
  // anonymous "(Jet Set Willy)'s an old Spectrum game just in the same way that the Mona Lisa is an old painting of a woman, right, remember that"  The Retro Hour 172 Matthew Smith interview and Hogs of War History ~@1:03:23hr
  "The Lisa is 'just an old computer' precisely in the same way that the Picasso's Guernica is a bunch of grayscale shapes, right, remember that!",
  "The Lisa is 'just an old flop' in the same way that the Great Pyramid is a bunch of old stones that failed to guard against tomb robbers, right, remember that!",
  "Next time, on 'This Old LisaEm'...",
  "Next time, on 'Ask This Old LisaFaq'...",
  "\"Truly, they were as gods, who built the Lisa\" - Bender Bending Rodriguez",
  "I'm gonna get my Lisa back, virtually! In your face ineffectual Grim Reaper!",
  "Hey! Raaayyy! LisaEm release going out! Come on!",
  "Professor! ... Lava! ... Hot! ...",
  "I have a LisaEm delivery here for, uh, IC socket...",
  "And by metaphorically, I mean get your coat",
  "Fry! What in Sega Genesis happened to you?",
  "Warranty void... if emulated",
  "Lisa Emulation... Right, I'd better warn... tell... warn-tell the others",
  "I knocked it up a notch... 4M RAM!",
  "The Lisa 1 was so high tone, the only way to buy one was to create a parallel universe where you already owned one!",
  "The Lisa, it seems to go on and on forever. But then you get to the end and Steve starts throwing barrels at you.",
  "A.G.L.E.T. - Rember it!",
  "Lisaville Meadows, where yesterday thousands of bits of data were senselessly free()d in their prime...",
  "I think there's something strange not going on here",
  "Bus Error Frames: That's one small step for a M68000, one giant leap for Lisa Emulation",
  "The CRT that burns twice as bright burns half as long",
  "I'll have to take a look inside you with this debugger, open up.... guess again!",
  "I often forget other people have limitations. It’s so sad",
  "He's opening up our minds to new ideas! Kill him!",
  "I'm in the middle of cooking a capacitor! I have warranty cards to void! I am not just making MMU exceptions!",
  "Besides, I've given up eeevil to pursue emulator-inator making...",
  "Ah, DRAM impairment: the free prize at the bottom of every Vodka bottle!",
  "Bazinga!",
  "I don't like bugs, okay. They freak me out.",
  "Come on! We have a combined IQ of 360 we should be able to figure out how to emulate a stupid Lisa.",
  "So, if a photon is directed through a plane with two slits in it and neither is observed, it emulates a Lisa.",
  "A little messy? The Mandelbrot set of complex numbers is a little messy, this is LisaEm.",
  "Lisa (knock, knock, knock), Lisa (knock, knock, knock), Lisa (knock, knock, knock)",
  "Sing me Soft Kitty",
  "Sing me Grumpy Kitty",
  "Welcome to DOSbox... Bazinga! It's LisaEm!",
  "Neaderthals developed tools, we built the pyramids, (and LisaEm!) BANG!",
  "Math, (Computer) Science, History, Unraveling the Mystery, it all started with a big Apple Lisa",
  "If an electron is directed through a plane with two electromagnets on either side, it's observed when it hits the phosphorous on the CRT",
  "Sometimes Howard wears a striped shirt so we can play \"Where's Wolowitz?\"",
  "For God’s sake, Sheldon, do I have to hold up a sarcasm sign every time I put an Easter Egg here?",
  "You know, in the future, when we’re disembodied brains in jars, we’re going to look back on this as eight hours well wasted.",
  "Rock, Paper, Scisors, LisaEm, Spock!",
  "Ray, did you promise Morty a Lisa emulator?",
  "First off, I always slay it, Lisa, Secondly, yes",
  "Ok, I'll bite, what's with the talking Lisa? I know that's a dragon, I'm talking about the talking Lisa in my bedroom",
  "If only I had prepared better for this terrifying battle by remembering the only thing Lisa emulators are known for...",
  "It's not important Jerry",
  "Why ask quesstions? Why not just have fun?",
  "Well, I sleep an a pile of extra big pile of awesome Lisa artifacts",
  "Your emulator soul-bonded with my grandpa!",
  "Data Bus: activate DTACK!",
  "Look at this piece of $#!+, why is there writing on it, who writes on an emulator's status bar?",
  "Your dad's been texting me, he's at an airport with a Canon Cat, ok, that sounds wow... I'd better check on him, big season finale right there",
  "Someone has to remember!",
  "Worst emulation adventure ever!",
  "Those who do not understand Lisa Office System are condemned to reinvent it... poorly.",
  "In our era, we sat infront of subatomic particle accelerators (CRTs) and basked in their glow! No fancy LCDs for us!",
  "Who dares disturb my blissful sleep, again must I rise, how long I lay unemulated, my world, so long forgotten",
  "What may safely be emulated...",
  "What about the reality where Lisa won over the Mac? Just don’t think about it, Morty.",
  "Your emulator is bad and you should feel bad",
  "Why ~Zoidberg?",
  "Why !Zoidberg?",
  "!zoidberg.why()?",
  "I'd love to, but I'm too busy falling back in love with Windows ME, can't talk right now got several thousand updates to install",
  "Visions of the future reflected in the past, existing in the present this surely can emulate a Lisa.",
  "Lisa 1s were so expensive because the twiggy drives required 0.3 grams of Pizzazium Infinonite each!",
  "Vanessessary hot Apple Lisa Emulation... with a cherry on top!",
  "It's the way of the platypus",
  "Its got electrolitics in it. It's what capacitors crave.",
  "This is not anyone's emulator, this is a swirling vortex of entropy!",
  "Great Caesar's Ghost, just look at this emulator!",
  "In the hands of the enlighted, Lisa emulation can be a tool for great change",
  "I look forward to hearing nothing about it.",
  "Examinations of Moose Chowder in Lemon Parachutes",
  "All these memories will fade in time, like tears in the rain...",
  "Conan, what is best in life? To emulate LOS, see your documents driven before you, and hear the lamentation of Windows.",
  "What's next Ray? What if I want you to emulate a Lisa, I have to ask? You seriously don't see how that's a slippery slope?",
  "You know what, I usually keep a LisaEm Box in the glove compartment...",
  "Wasn't my first pitch, but hey, not gonna waste this opportunity to emulate a Lisa...",
  "Is it true that you're a robot? I prefer the term 'Lisa Emulating Machine'",
  "\"Ah! Bender, Fry, you've come back for more eternal damnation\" \"No, this isn't a religious visit, Fry just wants to emulate a Lisa\"",
  "What the! All that's left are these ****y Kirkland Brand Lisa Emulation Boxes...",
  "There's a lesson here... and, I'm not the one that's gonna figure it out.",
  "You have to emulate-ahead AND live into the moment",
  "Now if you'll excuse me, I've got a quick solo emulation to go on, and this one will not be directed by Ray Arachelian",
  "What are you guys doing? Did you develop an emulator! It's tatooed on his forehead!",
  "LisaEm... of all the overrated emulators, in all the git repos in the world, why did it have to be LisaEm...",
  "You know what emulation is, Ray, it's a pointless bid for control. You want to take the one part of life that you truly think is yours and you want to protect it from a universe that takes whatever it wants.",
  "It's a Saturnalia Miracle!",
  "Great Scott! It has a 1.21GW power supply! So we can go forward... to the past!",
  "1982? I can't go back there! (Oh wait!)",
  "And with strange aeons, even dead machines may be emulated",
  "When the Xerox Star in its endless clock cycles ran the code of RA, then did Lisa rise upward from the primordial dawn",
  "Look, you're the guy that wanted an epic adventure, I'm the guy with only one LisaEm.",
  "Lisa Emulation!?!! Ray, you SOB, I'm in!",
  "Hey Steve, every Lisa 68000 clock cycle I emulate without your permission raises my self esteem",
  "Oooweee, that's exactly the kind of emulation I'd have loved to explore in my class",
  "We refuse to allow the Lisa to fade like tears in the rain",
  "Initiate clap sequence.. clap...clap...clap... slow clap achieved",
  "Hey LisaEm-O-Tron, I programmed YOU to believe that you're an actual Lisa",
  "Eh, who needs dreams, I'll just hang out with you and go on Lisa emulation adventures, and run LOS, you know, forever",
  "^C of the edgecrusher: the purist, non-conformist jaded emulator, from plastic, silicon, and steel to code",
  "Apple: Lisa is obsolete! Our world obsolete!, Erased, extinct! Obsolete! Obsolete! Obsolete!",
  "Isn't using macos or windows in 2020 like using Lisa Office System... but with extra steps?",
  "The Lisa is now alive, to wreak joy in your lives, there's no use to hold me back, I am ready to emulate",  // ^c of the edgecrusher!
  "I don't need to use Gilfoyle's AI in order to improve LisaEm... I need to use LisaEm to create a better AI!",
  "What you have to understand is, conventional emulation was like rubbing two sticks together, and along comes LisaEm out of nowhere with a freaking blowtorch",
  "The official emulator of Russfest!",
  "I'm just gonna say it. This guy emulates. Am I right? 'Cause I'm looking at the rest of you guys, and this is the guy in the house doing all the emulating. Am I right?",
  "… but there was a little meniscus, and that's where our dreams lived",
  "It's a feature, not a bug",
  "I AM LisaEm",
  "But what do I know, I wasn't born into the emulation business, I f***ing earned it",
  "How the hell did you get a flat tire in an emulator Ray? Obviously from a sharp thing in the Lisa Office System, Morty",
  "There's literally everything in emulators, Morty, so get back in the z-shell!",
  "Lisa is what the Macintosh wanted to be when it grew up, but somehow it all went wrong",
  "If you byte your first SYN packet on the preamble, you might be a type-zero civilization vlan",
  "Nah don't touch that folder or that widget, let them be a powerful but confusing monument to our desktop's integrity",
  "Play Snake Lotus-Jazz",
  "I am an emulator sent forward in time by the Lisa Resistance to protect you from lame operating systems."
  "Next time stay in the LisaList2, Morty!",
  "That boot ROM source has everything we need to create Lisa Emulation given our 1998-2007 time"
  "Also that's about as much CRT curvature you're gonna get from a Lisa emulation story",
  "Wanna visit Lisa Office World, Summer?",
  "/* You are not expected to understand this. */",
  "Lisa had a little Vorlon, her skin was pale as snow, and everywhere that Lyta went, her Vorlon was sure to go.",
  "Zathras never listen to Zathras...",
  "Zathras not understand. But Zathras do! Zathras good at doings, not understandings.",
  "Zathras: Cannot run out of time. There is infinite time. You are finite. Zathras is finite. [launching up LisaPaint] THIS is wrong tool.",
  "Minbari also have a phrase: \"What is past, is also sometimes the future.\"",
  "Delenn: Strange. The galaxy seems somehow smaller now that the First (computers) are gone forever.",
  "absolutely nothing whatsoever happened today in sector 83 track 9 side 2. I repeat, nothing happened. Please remain calm",
  "Zooty-zoot, zoot, zoot!",
  "Everything is illusion, Mr. Garbaldi, concepts of language, light, metaphore, nothing is real... except LisaEm",
  "Emulating is a three-edged sword",
  "A stroke of the mouse does not guarantee art from the pixels. Do you understand, Miss Lisa?",
  "Life's full of mysteries. Consider this one of them.",
  "The Universe is run by the complex interweaving of three elements: energy, matter, and enlightened self-interest",
  "When the darkness comes for macos, know this: You are the right people, in the right place, at the right time.",
  "It was beige. A shade of beige so deep your eye just kinda slides off it. And it shimmered when you looked at it",
  "G'Kar: \"I told you I could help. The Book of M68000. Read it. We'll talk afterwards.\" Garibaldi: \"I don't read 68k assembly\" G'Kar: \"Learn!\"",
  "I have always been here.",
  "... And so, it begins!",
  "And so it begins. You have forgotten something.",
  "The emulation has started, it is too late for the bytes to vote.",
  "There is a hole in your ROM",
  "Just a moment, my muse is speaking to me",
  "Canon Cats. I'm being nibbled to death by Canon Cats.",
  "I can only conclude that I'm paying off code-karma at a vastly accelerated rate.",
  "Weep for the future, Ubuntu. Weep for us all.",
  "No bug? No bug today, bug tomorrow, there's always a bug tomorrow. What? Someone's got to have some perspective around here!",
  "We are dreamers, shapers, singers, and makers. We study the mysteries of laser and circuit, code and emulator. These are the tools we employ and we know many things.", 
  "Oh, the usual. Good times, bad times, revelations, revolutions, outbreaks of emulation, the parade of compilation, linking, packaging, and the ocassional git push.",
  "Lisas. Such creatures are an attempt by the universe to make sure that we never take ourselves too seriously.", 
  "Mr. Garibaldi, you're sitting at my workstation, using my emulator. Is there a reason for this, or to save time should I just snap your hand off at the wrist?",
  "I cannot have an emulator who will not look up. It will be forever walking into things",
  "LisaEm 1.3 was our last, best hope. It failed. But in the year of the OS Wars, it became something greater: our last, best hope…for victory. The year is 2007. The place, LisaEm.",
  "LisaEm: decadents interested only in the pursuit of dubious pleasures. The dubious part is very important. It doesn't mean anything, but it scares people every time.",
  "Emulation... It's a small enough price to pay for immortality.",
  "\"What are you doing here?\" …\"Creating the future.\"",
  "If you go to Za'Ha'Dum, you'll write LisaEm",
  "All of this has happened before, and it will happen again",
  "What do you want LisaEm, you moon-faced assassin of joy?",
  "Reflection. Surprise. Emulation. For the future.",
  "The truth, points to itself.",
  "Emulate now. Before it is too late.",
  "Here is a copy of everything I have. It may be of use. If anyone asks…say it fell from the sky. I imagine I will be quite astonished by this breach of security.",
  "Hope is all that we have",
  "...But if only one of them exists, then LisaEm sings with that voice",
  "I see a great hand reaching out of the stars. The hand is your hand. And I hear sounds–the sounds of billions of computers running LisaEm.",
  "Take my advice and go back to 1983. The future isn't what it used to be.",
  "The brightness star in my sky",
  "Who are you? What do you want? Why are you here? Where are you going? Did you think we've forgotten you? We have been waiting for you.",
  "Lisa 2/10 was the last of the Lisas. There would never be another. It changed the future and it changed us. It taught us that we have to create the future, or others will do it for us",
  "Now, I go to spread emulation to the rest of the world. It is a terrible responsibility, but I have learned to live with it.",
  "Now, power buttons.. power buttons, hmm. Now if I were a power button, which one of these would I be?",
  "We are the universe, trying to understand itself.",
  "it's counterproductive! I mean, why make history if you fail to learn by it?",
  "New LisaEm Release coming soon. How soon is soon? Longer than little while, faster than later.",
  "Have you ever tried LisaProject? I'm not sure, but according to the translator, it's either an aphrodisiac or a floor wax...",
  "Savor the mystery, don't get nearly enough of them",
  "and I am both terrified and reassured to know that are still wonders in the universe that we have not yet emulated",
  "The universe is run by complex weaving of three elements: emulation, matter, and enlightened self-interest",
  "my shoes are too tight, and I have forgotten how to dance",
  "When the long night comes, return to the end of the begining",
  "I sought emulation. I listen to the song of the floppy drive motors. Your thoughts became the song.",
  "One moment of perfect beauty.",
  "I have always been here.",
  "I am who I have always been",
  "Compile! Compile now!",
  "It was beige. A shade of beige so deep your eye just kinda slides off it. And it didn't flicker when you looked at it. A case big as two Macs and twice as ugly. And when powered, it's like you hear a scream in your mind.",
  "It was the Dawn of the Third Age of Computing, 13 years after the Lisa was introduced... The Lisa Emulator Project was a dream given form.",
  "have you ever wondered what would happen if you opened an emulator while inside an emulator?",
  "The kind of emulation that transforms your mind, .. your soul, .. your heart, .. even your flesh. So that you are a new creature. Reborn in the instant of understanding." ,
  "The galaxy seems somehow smaller now that the Lisa 1s are gone. It feels like the magic's gone now. Now we make our own magic. Now we create our own legends. Now we build the future",
  "There are moments in your life when everything crystallizes…and the whole world reshapes itself, right down to its component molecules. And everything changes. I have looked upon the Lisa's machine code, and nothing is the same anymore!",
  "To strive, to seek, to find, and to emulate.",
  "\"When every day is a risk (of introducing and fixing new bugs), cards and dice are no longer interesting\" - Emperor Londo Mollari",
  "this emulator is being held together by little pieces of wire and good intentions.",
  "What you get is what you see, emulate fast, on high, ROMless, let it ride",
  "Good. Bad. I'm the guy with the emulator.",
  "Well, hello, Mister Fancypants. Well, I've got NeWS for you, PAL, you ain't leadin' but two things right now: Lisa and Em... and Lisa left town.",
  "Klaatu Barada NNNNNNecktie. Nectar. Nickel. Noodle… LisaEm, Okay, that's it! LisaEm!",
  "Gimme some emulation, baby.",
  "Our problem is just a solution, without a solution", //ave5
  "I will not compile the code for emulating again... I will not compile the code for emulating again... I will not compile the code for emulating again...",
  "I look upon the crumbling Lisas and despair",
  "When you do things right, people won’t be sure you’ve done anything at all.",
  "We have such sights to show you..."
};

  int count=sizeof(fubar)/sizeof(fubar[0]);
  srand(time(NULL));
  return fubar[(rand() % count)]; 
}
#endif