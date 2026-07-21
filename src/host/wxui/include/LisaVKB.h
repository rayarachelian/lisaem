#ifndef __LISAVKBH__
#define __LISAVKBH__

#include <wx/wx.h>

#include "LisaConfig.h"

class LisaVirtualKBFrame : public wxFrame
{
public:
    LisaVirtualKBFrame(const wxString &title, LisaConfig *lisaconfig);
    ~LisaVirtualKBFrame();

    //          event handlers
    void OnApply(wxCommandEvent &event);

    void OnKey(wxCommandEvent &event);

    wxPanel *m_panel; // the panel itself

    wxButton *b_dprompath;

    wxButton *lisakey_Ox20;
    wxButton *lisakey_Ox21;
    wxButton *lisakey_Ox22;
    wxButton *lisakey_Ox23;
    wxButton *lisakey_Ox24;
    wxButton *lisakey_Ox25;
    wxButton *lisakey_Ox26;
    wxButton *lisakey_Ox27;
    wxButton *lisakey_Ox28;
    wxButton *lisakey_Ox29;
    wxButton *lisakey_Ox2a;
    wxButton *lisakey_Ox2b;
    wxButton *lisakey_Ox2c;
    wxButton *lisakey_Ox2d;
    wxButton *lisakey_Ox2e;
    wxButton *lisakey_Ox2f;
    wxButton *lisakey_Ox40;
    wxButton *lisakey_Ox41;
    wxButton *lisakey_Ox42;
    wxButton *lisakey_Ox42_2;
    wxButton *lisakey_Ox43;
    wxButton *lisakey_Ox44;
    wxButton *lisakey_Ox45;
    wxButton *lisakey_Ox46;
    wxButton *lisakey_Ox48;
    wxButton *lisakey_Ox49;
    wxButton *lisakey_Ox4c;
    wxButton *lisakey_Ox4d;
    wxButton *lisakey_Ox4e;
    wxButton *lisakey_Ox50;
    wxButton *lisakey_Ox51;
    wxButton *lisakey_Ox52;
    wxButton *lisakey_Ox53;
    wxButton *lisakey_Ox54;
    wxButton *lisakey_Ox55;
    wxButton *lisakey_Ox56;
    wxButton *lisakey_Ox57;
    wxButton *lisakey_Ox58;
    wxButton *lisakey_Ox59;
    wxButton *lisakey_Ox5a;
    wxButton *lisakey_Ox5b;
    wxButton *lisakey_Ox5c;
    wxButton *lisakey_Ox5d;
    wxButton *lisakey_Ox5e;
    wxButton *lisakey_Ox5f;
    wxButton *lisakey_Ox60;
    wxButton *lisakey_Ox61;
    wxButton *lisakey_Ox62;
    wxButton *lisakey_Ox63;
    wxButton *lisakey_Ox64;
    wxButton *lisakey_Ox65;
    wxButton *lisakey_Ox66;
    wxButton *lisakey_Ox67;
    wxButton *lisakey_Ox68;
    wxButton *lisakey_Ox69;
    wxButton *lisakey_Ox6a;
    wxButton *lisakey_Ox6b;
    wxButton *lisakey_Ox6c;
    wxButton *lisakey_Ox6d;
    wxButton *lisakey_Ox6e;
    wxButton *lisakey_Ox6f;
    wxButton *lisakey_Ox70;
    wxButton *lisakey_Ox71;
    wxButton *lisakey_Ox72;
    wxButton *lisakey_Ox73;
    wxButton *lisakey_Ox74;
    wxButton *lisakey_Ox75;
    wxButton *lisakey_Ox76;
    wxButton *lisakey_Ox77;
    wxButton *lisakey_Ox78;
    wxButton *lisakey_Ox79;
    wxButton *lisakey_Ox7a;
    wxButton *lisakey_Ox7b;
    wxButton *lisakey_Ox7c;
    wxButton *lisakey_Ox7d;
    wxButton *lisakey_Ox7e;
    wxButton *lisakey_Ox7e_2;
    wxButton *lisakey_Ox7f;

private:
    DECLARE_EVENT_TABLE()
};

// need 76 of these

#endif
