#ifndef TERMWXFRAME_H
#define TERMWXFRAME_H

#include <stdio.h>
#include <stdlib.h>

#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/event.h>
#include <wx/window.h>
#include <terminalwx.h>

class TerminalWxFrame : public wxFrame
{
public:
    TerminalWxFrame(wxWindow *parent, wxWindowID id = -1, int port = -1);
    virtual ~TerminalWxFrame();

    int portnum;
    wxTimer *XferTimer; // call to protocol xfer like xmodem - active only during a transfer
    FILE *capture;      // if not NULL, write display output to file;
    FILE *upload;       // if not NULL, file upload in progress
    FILE *download;     // if not NULL, file download in progress
    int xferproto;      // 0=terminal no protocol, 1=ascii upload, 2=xmodem download, 3=xmodem upload.
    size_t xferbytes;   // bytes or blocks transfered for Status bar

    wxMenu *FileMenu;
    wxMenuBar *MenuBar1;
    wxMenu *Menu2;
    wxMenu *editMenu;
    wxMenu *windowMenu;

    //(*Handlers(TerminalWxFrame)
    void OnQuit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnClose(wxCloseEvent &event);
    void OnCustom1Paint(wxPaintEvent &event);
    void OnTextUpload(wxCommandEvent &event);
    void OnXmodemUpload(wxCommandEvent &event);
    void OnXModemDownload(wxCommandEvent &event);
    void OnTimer(wxTimerEvent &event);

    void OnChar(wxKeyEvent &event); // RA for console intercept

    void OnCopy(wxCommandEvent &event);
    void OnPaste(wxCommandEvent &event);
    void OnSelectAll(wxCommandEvent &WXUNUSED(event));

    void OnFileCapture(wxCommandEvent &event);

    void XModemSendBlock(void);
    void XModemReceiveBlock(void);

    size_t xferPosition;        // file position ftell/fseek
    long int xferBlockNum;      // block number for block oriented protocols
    long int xferWindowSize;    // for flexible sized protocols
    long int xferRetryCount;    // current retry count
    long int xferLastTimeStamp; // timestamp for timeouts
    long int xferMode1;         // extra vars for each protocol
    long int xferMode2;
    char lastchars[3];

    wxStatusBar *StatusBar1;

    DECLARE_EVENT_TABLE()
};

#endif
