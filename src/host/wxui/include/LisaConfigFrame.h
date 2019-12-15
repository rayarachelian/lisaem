#ifndef __LISACONFIGFRAMEH__
#define __LISACONFIGFRAMEH__

#include <wx/wx.h>
#include <wx/notebook.h>

#include "LisaConfig.h"

/*
 * This class is the GUI equivalent of LisaConfig. You set it up by passing
 * it a LisaConfig instance and it provides an on screen editable
 * representation. Any changes made are applied to the LisaConfig instance.
 * (But it is up to other controlling code to Save() that in-memory
 * LisaConfig back to disc if it needs to.)
 */

class LisaConfigFrame : public wxFrame
{
public:

    LisaConfigFrame(const wxString& title, LisaConfig *lisaconfig);
    ~LisaConfigFrame();

    // creators
    wxPanel    *CreateMainConfigPage( wxNotebook *parent);
    wxPanel    *CreatePortsConfigPage(wxNotebook *parent);
    wxPanel    *CreateSlotConfigPage( wxNotebook *parent, int slot);
    wxPanel    *CreatePrinterConfigPage(wxNotebook *parent);

    void        CreateNotebook(       wxNotebook *parent);

    //          event handlers
    void        OnApply(            wxCommandEvent& event);
    void        OnZapPram(          wxCommandEvent& event);
    void        OnSavePram(          wxCommandEvent& event);
    void        OnLoadPram(          wxCommandEvent& event);
    void        OnSernoInfo(         wxCommandEvent& event); 
    void        OnNoteBook(wxNotebookEvent& event);
    void        OnPickRom(            wxCommandEvent& event);
    void        OnPickDRom(           wxCommandEvent& event);
    void        OnPickProFile(        wxCommandEvent& event);
    void        OnPickProFile1H(      wxCommandEvent& event);
    void        OnPickProFile2H(      wxCommandEvent& event);
    void        OnPickProFile3H(      wxCommandEvent& event);
    void        OnPickProFile1L(      wxCommandEvent& event);
    void        OnPickProFile2L(      wxCommandEvent& event);
    void        OnPickProFile3L(      wxCommandEvent& event);
    void        OnPickIWDir    (      wxCommandEvent& event);

    wxPanel    *m_panel;           // the panel itself
    wxNotebook *thenoteBook;

    wxRadioBox *sloton[4];         // expansion slots, which have cards, and what cards are in them
                                   // using 4 here and skipping 0, so we can be clear about slot[1], slot[2], slot[3].

    wxTextCtrl *m_rompath;         // Lisa Boot ROM path
    wxButton   *b_rompath;         // button for picking it

    wxButton   *b_apply;           // save/apply button


    wxTextCtrl *serialtxt;
    wxTextCtrl *m_dprompath;       // Dual Parallel Expansion Slot ROM
    wxButton   *b_dprompath;

    wxRadioBox *kbbox;             // keyboard ID

  //wxRadioBox *ramsize;           // memory size for Lisa, forced to 1024K due to bugs

    wxRadioBox *iorombox;          // I/O ROM version

    wxCheckBox *cheats;            // startup BOOT ROM cheats
    wxCheckBox *soundeffects;
    wxCheckBox *skinson;
    wxRadioBox *pportbox;          // Motherboard Parallel Port Connection type
    wxTextCtrl *m_propath;         // ProFile file path attached to parallel Port.
    wxButton   *b_propath;         // buttonto pick profile path

    wxRadioBox *pportboxh[4];      // dual parallel port card connections
    wxRadioBox *pportboxl[4];

    wxTextCtrl *m_text_propathh[4]; // profile paths
    wxTextCtrl *m_text_propathl[4];

    wxRadioBox *serialabox;
    wxRadioBox *serialbbox;
    wxTextCtrl *serialaparam;
    wxTextCtrl *serialbparam;

    wxString pportopts[3];        // common to all parallel ports
    wxString wpportopts[3];       // Widget on Lisa 2/10

    wxString serportopts[6];
    int      serialopts;

    // ImageWriter settings
    wxChoice   *dipsw1_123;
    wxRadioBox *dipsw1_4;
    wxRadioBox *dipsw1_5;
    wxChoice   *dipsw1_67;
    wxCheckBox *dipsw1_8;

    wxCheckBox *iw_img_box;        // print to images or printer
    wxTextCtrl *iw_img_path;       // dir path to store images
    wxButton   *iw_img_path_b;     // path browse button


private:
    LisaConfig *my_lisaconfig;
    wxString slotcard[2];

    DECLARE_EVENT_TABLE()
};

#endif

