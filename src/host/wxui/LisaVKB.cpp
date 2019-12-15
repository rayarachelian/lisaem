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
*                     Virtual Keyboard window and keyboard editor                      *
*                                                                                      *
\**************************************************************************************/

#include <wx/wx.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/panel.h>

#include "machine.h"
/*
keycap height: 74 - all are the same size vertically

normal key width 66 pixels, gap between keys 6 pixels

vertical gap between keys 5 pixels
tab key 113 pixels wide
caps lock 125 pixels wide
lshift key 161 pixels wide
left option 66 (normal key)
left apple 111
renter 111
roption 66 (normal key)
rshift 161 pixels wide
return 125 (same as capslock)
backspace 111 (same size as left apple, renter)
spacebar 520 pixels wide  (7 keys?)

numeric keypad sized keys (4 key spaces horizontal by 5 vertical
enter key is 2x vertical
0 key is 2x horizontal

3 normal keycaps space horizontally between keyboard and numeric keypad
*/

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
                             {"","OPT","CMD","SPACE","ENTER","OPT","","","","","","","",""        ,"" }  };
char *kb_us_n1_txt[ 5][5]= { {"CLR","-","+","*"    ,""  },
                             {  "7","8","9","/"    ,""  },
                             {  "4","5","6",","    ,""  },
                             {  "1","2","3","ENTER",""  },
                             {  "0",".","" ,""     ,""  }                                                };


int kb_us_r1_sz[5][15]= {    { 66,      66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,111        ,0 },
                             { 113,    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66         ,0 },
                             {125,      66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,-1 ,125        ,0 },
                             {161,-1,    66, 66, 66, 66, 66, 66, 66, 66, 66, 66,161   ,0          ,0 },
                             {66,66,   111,    520   ,111,  66,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0          ,0 }  };  // starts 1 button off to the right!

int kb_us_n1_sz[ 5][5]=    { {   66, 66, 66, 66    ,0  },
                             {   66, 66, 66, 66    ,0  },
                             {   66, 66, 66, 66    ,0  },
                             {   66, 66, 66,-74*2  ,0  }, // -74 means double long, and 66 pixels wide.
                             { 2*66, 66, 0 , 0     ,0  }                                                };

//-1=no key there, ignore the size, 0=end size, -74*2=double long



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
                             {0,0x7c,0x7f,0x5c,0x46,0x4e,   0,   0,   0,   0,   0,   0,   0,      0, 0 } }; // ****
//
//                              0    1    2    3   4
uint8 kbcodesn[ 5][5]=     { {0x20,0x21,0x22,0x23, 0},
                             {0x24,0x25,0x26,0x27, 0},
                             {0x28,0x29,0x2a,0x2b, 0},
                             {0x4d,0x2d,0x2e,0x2f, 0},
                             {0x49,0x2c,   0,   0, 0}                                                    };
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
   LISA_KEY_Ox20=0xf020,
   LISA_KEY_Ox21=0xf021,
   LISA_KEY_Ox22=0xf022,
   LISA_KEY_Ox23=0xf023,
   LISA_KEY_Ox24=0xf024,
   LISA_KEY_Ox25=0xf025,
   LISA_KEY_Ox26=0xf026,
   LISA_KEY_Ox27=0xf027,
   LISA_KEY_Ox28=0xf028,
   LISA_KEY_Ox29=0xf029,
   LISA_KEY_Ox2a=0xf02a,
   LISA_KEY_Ox2b=0xf02b,
   LISA_KEY_Ox2c=0xf02c,
   LISA_KEY_Ox2d=0xf02d,
   LISA_KEY_Ox2e=0xf02e,
   LISA_KEY_Ox2f=0xf02f,
   LISA_KEY_Ox40=0xf040,
   LISA_KEY_Ox41=0xf041,
   LISA_KEY_Ox42=0xf042,
   LISA_KEY_Ox43=0xf043,
   LISA_KEY_Ox44=0xf044,
   LISA_KEY_Ox45=0xf045,
   LISA_KEY_Ox46=0xf046,
   LISA_KEY_Ox48=0xf048,
   LISA_KEY_Ox49=0xf049,
   LISA_KEY_Ox4c=0xf04c,
   LISA_KEY_Ox4d=0xf04d,
   LISA_KEY_Ox4e=0xf04e,
   LISA_KEY_Ox50=0xf050,
   LISA_KEY_Ox51=0xf051,
   LISA_KEY_Ox52=0xf052,
   LISA_KEY_Ox53=0xf053,
   LISA_KEY_Ox54=0xf054,
   LISA_KEY_Ox55=0xf055,
   LISA_KEY_Ox56=0xf056,
   LISA_KEY_Ox57=0xf057,
   LISA_KEY_Ox58=0xf058,
   LISA_KEY_Ox59=0xf059,
   LISA_KEY_Ox5a=0xf05a,
   LISA_KEY_Ox5b=0xf05b,
   LISA_KEY_Ox5c=0xf05c,
   LISA_KEY_Ox5d=0xf05d,
   LISA_KEY_Ox5e=0xf05e,
   LISA_KEY_Ox5f=0xf05f,
   LISA_KEY_Ox60=0xf060,
   LISA_KEY_Ox61=0xf061,
   LISA_KEY_Ox62=0xf062,
   LISA_KEY_Ox63=0xf063,
   LISA_KEY_Ox64=0xf064,
   LISA_KEY_Ox65=0xf065,
   LISA_KEY_Ox66=0xf066,
   LISA_KEY_Ox67=0xf067,
   LISA_KEY_Ox68=0xf068,
   LISA_KEY_Ox69=0xf069,
   LISA_KEY_Ox6a=0xf06a,
   LISA_KEY_Ox6b=0xf06b,
   LISA_KEY_Ox6c=0xf06c,
   LISA_KEY_Ox6d=0xf06d,
   LISA_KEY_Ox6e=0xf06e,
   LISA_KEY_Ox6f=0xf06f,
   LISA_KEY_Ox70=0xf070,
   LISA_KEY_Ox71=0xf071,
   LISA_KEY_Ox72=0xf072,
   LISA_KEY_Ox73=0xf073,
   LISA_KEY_Ox74=0xf074,
   LISA_KEY_Ox75=0xf075,
   LISA_KEY_Ox76=0xf076,
   LISA_KEY_Ox77=0xf077,
   LISA_KEY_Ox78=0xf078,
   LISA_KEY_Ox79=0xf079,
   LISA_KEY_Ox7a=0xf07a,
   LISA_KEY_Ox7b=0xf07b,
   LISA_KEY_Ox7c=0xf07c,
   LISA_KEY_Ox7d=0xf07d,
   LISA_KEY_Ox7e=0xf07e,
   LISA_KEY_Ox7f=0xf07f
};


BEGIN_EVENT_TABLE(LisaVirtualKBFrame, wxFrame)

    EVT_BUTTON(LISA_KEY_Ox20,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox21,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox22,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox23,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox24,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox25,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox26,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox27,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox28,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox29,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox2a,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox2b,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox2c,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox2d,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox2e,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox2f,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox40,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox41,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox42,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox43,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox44,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox45,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox46,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox48,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox49,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox4c,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox4d,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox4e,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox50,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox51,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox52,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox53,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox54,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox55,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox56,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox57,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox58,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox59,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox5a,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox5b,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox5c,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox5d,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox5e,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox5f,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox60,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox61,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox62,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox63,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox64,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox65,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox66,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox67,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox68,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox69,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox6a,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox6b,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox6c,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox6d,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox6e,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox6f,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox70,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox71,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox72,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox73,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox74,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox75,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox76,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox77,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox78,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox79,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox7a,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox7b,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox7c,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox7d,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox7e,       LisaVirtualKBFrame::OnKey)
    EVT_BUTTON(LISA_KEY_Ox7f,       LisaVirtualKBFrame::OnKey)

END_EVENT_TABLE()


LisaVirtualKBFrame::LisaVirtualKBFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(700,500), wxDEFAULT_FRAME_STYLE|wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE)
{

  pportopts[0] = wxT("ProFile");
  pportopts[1] = wxT("ADMP");
  pportopts[2] = wxT("Nothing");

  serportopts[0]="NOTHING";
  serportopts[1]="LOOPBACK";
  serportopts[2]="PIPE";
  serportopts[3]="FILE";
  serportopts[4]="ImageWriter";
  #ifndef __MSVCRT__
   serportopts[5]="TELNETD";
   serialopts=6;
  #else
   serialopts=5;
  #endif

  slotcard[0]="DualParallel";
  slotcard[1]="Nothing";

  SetMinSize(wxSize(700,500));
  SetSize(wxSize(700,500));

  thenoteBook =
            new wxNotebook(this, ID_NOTEBOOK,  wxDefaultPosition, wxSize(700, 500) );
  CreateNotebook(thenoteBook);

    m_text_propathl[slot] = new wxTextCtrl(panel, wxID_ANY, _T(l), wxPoint( 10,  y), wxSize( 400, 30), 0 );
    (void) new wxButton( panel, idbl[slot], wxT("browse"),  wxPoint( 420,  y), wxDefaultSize);

    (void) new wxButton( panel, ID_APPLY, wxT("Apply"), wxPoint(420,  400), wxDefaultSize );

    return panel;


   wxPanel *panel = new wxPanel(parent);

}


//LisaVirtualKBFrame::~LisaVirtualKBFrame()
//{
//
//}

void LisaVirtualKBFrame::OnKey(wxKeyEvent& event)
{
    wxString key;
    long keycode = event.GetKeyCode();
    int lisakey=0;

    int forcelisakey=0;

    // on linux for some reason I get keycode=-1 for ALT!!!
    #ifdef __WXX11__
    if (keycode==-1) keycode=WXK_ALT;
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
        case WXK_ALT:                lisakey=KEYCODE_COMMAND;         break;
        case WXK_CONTROL:            lisakey=KEYCODE_LOPTION;         break;
    }

}


wxString LisaVirtualKBFrame::GetWXKeyName(long int keycode)
{
    switch ( keycode )
    {                              
        case WXK_LEFT:             return "LEFT";             
        case WXK_NUMPAD_LEFT:      return "NUMPAD_LEFT";    
                                                      
                                                     
        case WXK_UP :              return "UP";      
        case WXK_NUMPAD_UP:        return "NUMPAD_UP"        
                                                     
        case WXK_RIGHT :           return "RIGHT";             
        case WXK_NUMPAD_RIGHT:     return "NUMPAD_RIGHT";     
                                                     
        case WXK_DOWN:             return "DOWN";             
        case WXK_NUMPAD_DOWN :     return "NUMPAD_DOWN";     
                                                     
                                                     
        case WXK_NUMPAD_INSERT:    return "NUMPAD_INSERT";    
        case WXK_NUMPAD_DELETE:    return "NUMPAD_DELETE";
                                                     
      //case WXK_NUMPAD_EQUAL:     return "NUMPAD_EQUAL";     
        case WXK_NUMPAD_MULTIPLY:  return "NUMPAD_MULTIPLY";
        case WXK_NUMPAD_ADD:       return "NUMPAD_ADD"; 
        case WXK_NUMPAD_SEPARATOR: return "NUMPAD_SEPARATOR"; 
        case WXK_NUMPAD_SUBTRACT:  return "NUMPAD_SUBTRACT";
        case WXK_NUMPAD_DECIMAL:   return "NUMPAD_DECIMAL"; 
        case WXK_NUMPAD_DIVIDE:    return "NUMPAD_DIVIDE";  
                                                     
        case WXK_NUMPAD_ENTER:     return "NUMPAD_ENTER";   
        case WXK_NUMLOCK:          return "NUMLOCK";    
                                                     
        case WXK_NUMPAD0:          return "NUMPAD0";         
        case WXK_NUMPAD1:          return "NUMPAD1";         
        case WXK_NUMPAD2:          return "NUMPAD2";         
        case WXK_NUMPAD3:          return "NUMPAD3";         
        case WXK_NUMPAD4:          return "NUMPAD4";         
        case WXK_NUMPAD5:          return "NUMPAD5";         
        case WXK_NUMPAD6:          return "NUMPAD6";         
        case WXK_NUMPAD7:          return "NUMPAD7";         
        case WXK_NUMPAD8:          return "NUMPAD8";         
        case WXK_NUMPAD9:          return "NUMPAD9";         
                                                     
        case WXK_BACK:             return "BACK";         
        case WXK_TAB:              return "TAB";            
        case WXK_RETURN:           return "RETURN";           
        case WXK_ESCAPE:           return "ESCAPE";          
        case WXK_SPACE:            return "SPACE";          
        case WXK_DELETE:           return "DELETE";           
                                                   
        case 'a':                   
        case 'b':                   
        case 'c':                   
        case 'd':                   
        case 'e':                   
        case 'f':                   
        case 'g':                   
        case 'h':                   
        case 'i':                   
        case 'j':                   
        case 'k':                   
        case 'l':                   
        case 'm':                   
        case 'n':                   
        case 'o':                   
        case 'p':                   
        case 'q':                   
        case 'r':                   
        case 's':                   
        case 't':                   
        case 'u':                   
        case 'v':                   
        case 'w':                   
        case 'x':                   
        case 'y':                   
        case 'z':                   
        case 'A':                   
        case 'B':                   
        case 'C':                   
        case 'D':                   
        case 'E':                   
        case 'F':                   
        case 'G':                   
        case 'H':                   
        case 'I':                   
        case 'J':                   
        case 'K':                   
        case 'L':                   
        case 'M':                   
        case 'N':                   
        case 'O':                   
        case 'P':                   
        case 'Q':                   
        case 'R':                   
        case 'S':                   
        case 'T':                   
        case 'U':                   
        case 'V':                   
        case 'W':                   
        case 'X':                   
        case 'Y':                   
        case 'Z':                   
        case '0':                   
        case '1':                   
        case '2':                   
        case '3':                   
        case '4':                   
        case '5':                   
        case '6':                   
        case '7':                   
        case '8':                   
        case '9':                  
        case '~':
        case '`':            
        case '_':
        case '-':            
        case '+':
        case '=':             
        case '{':
        case '[':            
        case '}':
        case ']':            
        case '|':
        case '\\':           
        case ':':
        case ';':            
        case '"':
        case '\'':           
        case '<':
        case ',':            
        case '>':
        case '.':            
        case '?':
        case '/':             wxString k; wxChar c=(wxChar) keycode;
                                     k+=c; return k;

        case WXK_SHIFT:              return "SHIFT";
        case WXK_ALT:                return "ALT";
        case WXK_CONTROL:            return "CONTROL";
    }
    return 0;
}

NOTES: So WXK_* goes above 300.  So a small array, say 1024 can act as a lookup table for the keystrokes.
