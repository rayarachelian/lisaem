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

class LisaWin : public wxScrolledWindow
{
public:
     LisaWin(wxWindow *parent);
     ~LisaWin();

     int doubley; // this doubles the number of vertical lines

     int dirtyscreen; // indicated dirty lisa vidram

     uint8 brightness;

     // void SetStatusBarText(wxString &msg);

     int refresh_bytemap; // flag indicating contrast change
     int floppystate;     // animation state of floppy
     int powerstate;      // animation state of power button

     void RepaintNow(void);

     void RePaint_AAGray(void);

     int RePaint_AntiAliased_skinless(void);
     int RePaint_AntiAliased_skins(void);

     void RePaint_AntiAliased(void);
     void RePaint_DoubleY(void);
     void RePaint_SingleY(void);
     // void RePaint_Scaled(void);
     void RePaint_3A(void);
     void RePaint_2X3Y(void);

     void (LisaWin::*RePainter)(void); // pointer method to one of the above

     void SetVideoMode(int mode);

     void OnPaint_skinless(wxRect &rect);
     void OnPaint_skins(wxRect &rect);
     void OnPaint(wxPaintEvent &event);

     void OnErase(wxEraseEvent &event);

     long mousemoved;

     void OnMouseMove(wxMouseEvent &event);
     void OnKeyDown(wxKeyEvent &event);
     void OnKeyUp(wxKeyEvent &event);
     void OnChar(wxKeyEvent &event);

     void LogKeyEvent(const wxChar *name, wxKeyEvent &event, int keydir);
     void ContrastChange(void);

     int lastkeystroke;

     uint8 bright[8]; // brightness levels for ContrastChange and repaint routines.

     int repaintall;
     int ox, oy, ex, ey;
     int dwx, dwy;

     int rawcodes[128];
     int rawidx;

private:
     int lastkeyevent;
     wxScrolledWindow *myparent;
     wxCursor *m_dot_cursor;
     int lastcontrast;
     static inline wxChar GetChar(bool on, wxChar c) { return on ? c : _T('-'); }

     DECLARE_EVENT_TABLE()
};
