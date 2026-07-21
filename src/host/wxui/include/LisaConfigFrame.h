#ifndef __LISACONFIGFRAMEH__
#define __LISACONFIGFRAMEH__

#include <wx/wx.h>
#include <wx/notebook.h>

#include "LisaConfig.h"

class wxSimplebook;

/*
 * This class is the GUI equivalent of LisaConfig. You set it up by passing
 * it a LisaConfig instance and it provides an on screen editable
 * representation. Any changes made are applied to the LisaConfig instance.
 * (But it is up to other controlling code to Save() that in-memory
 * LisaConfig back to disc if it needs to.)
 */

class LisaConfigFrame : public wxDialog
{
public:
    LisaConfigFrame(const wxString &title, LisaConfig *lisaconfig);
    ~LisaConfigFrame();

    // creators
    wxPanel *CreateMainConfigPage(wxNotebook *parent);
    wxPanel *CreatePortsConfigPage(wxNotebook *parent);
    wxPanel *CreateSlotConfigPage(wxWindow *parent, int slot);
    wxPanel *CreatePrinterConfigPage(wxNotebook *parent);

    void CreateNotebook(wxNotebook *parent);

    //          event handlers
    void ApplyChanges();  // commit control values into the LisaConfig (no close)
    void OnApply(wxCommandEvent &event); // Apply button: commit, stay open
    void OnOK(wxCommandEvent &event);    // OK button: commit & close
    void OnControlChanged(wxCommandEvent &event); // any edit -> enable Apply
    void SetApplyEnabled(bool enabled);
    void OnCancel(wxCommandEvent &event);          // Cancel: warn if there are unapplied edits
    void OnSerialChanged(wxCommandEvent &event);   // serial port -> loopback pairing (realtime)
    void set_profile_path_checked(wxTextCtrl *target, const wxString &path); // validate + set an image path
    bool path_in_use_elsewhere(const wxString &path, wxTextCtrl *except);    // duplicate-image guard
    void OnSlotCardChanged(wxCommandEvent &event); // slot card dropdown -> show/hide ports
    void OnSlotPick(wxCommandEvent &event);        // slot 1/2/3 selector -> switch slotbook page
    void UpdateSlotVisibility(int slot);           // ports config only when a card is installed
    void OnZapPram(wxCommandEvent &event);
    void OnSavePram(wxCommandEvent &event);
    void OnLoadPram(wxCommandEvent &event);
    void OnSernoInfo(wxCommandEvent &event);
    void OnNoteBook(wxNotebookEvent &event);
    void OnPickRom(wxCommandEvent &event);
    void OnPickDRom(wxCommandEvent &event);
    void OnPickProFile(wxCommandEvent &event);
    void OnPickProFile1H(wxCommandEvent &event);
    void OnPickProFile2H(wxCommandEvent &event);
    void OnPickProFile3H(wxCommandEvent &event);
    void OnPickProFile1L(wxCommandEvent &event);
    void OnPickProFile2L(wxCommandEvent &event);
    void OnPickProFile3L(wxCommandEvent &event);
    void OnPickIWDir(wxCommandEvent &event);

    wxPanel *m_panel; // the panel itself
    wxNotebook *thenoteBook;

    wxChoice *sloton[4];   // per-slot installed-card dropdown (Dual Parallel / Nothing)
                           // using 4 here and skipping 0, so we can be clear about slot[1], slot[2], slot[3].
    wxPanel *slotports[4]; // per-slot container for the Upper/Lower port config; hidden when no card
    wxStaticText *slotempty[4]; // per-slot "No card installed" placeholder; shown when no card
    wxSimplebook *slotbook;     // holds the three slot panels; driven by slotpick
    wxChoice *slotpick;         // compact 1/2/3 slot selector

    wxTextCtrl *m_rompath; // Lisa Boot ROM path
    wxButton *b_rompath;   // button for picking it

    wxButton *b_apply; // save/apply button
    wxPoint applypoint;

    wxTextCtrl *serialtxt;
    wxTextCtrl *m_dprompath; // Dual Parallel Expansion Slot ROM
    wxButton *b_dprompath;

    wxChoice *kbbox; // keyboard ID

    wxChoice *iorombox; // I/O ROM version

    wxRadioBox *cpurambox;

    wxCheckBox *cheats;       // startup BOOT ROM cheats
    wxCheckBox *hle_cheats;   // ProFile acceleration
    wxCheckBox *console_term; // Display TerminalWx for LPW + UniPlus (and eventually Xenix)
                              // wxCheckBox *macwx4mb;
    wxCheckBox *soundeffects;
    wxCheckBox *doublesided;
    wxCheckBox *skinson;
    wxRadioBox *pportbox;  // Motherboard Parallel Port Connection type
    wxTextCtrl *m_propath; // ProFile file path attached to parallel Port.
    wxButton *b_propath;   // buttonto pick profile path

    wxRadioBox *pportboxh[4]; // dual parallel port card connections
    wxRadioBox *pportboxl[4];

    wxTextCtrl *m_text_propathh[4]; // profile paths
    wxTextCtrl *m_text_propathl[4];

    //    wxRadioBox *serialabox;
    //    wxRadioBox *serialbbox;
    wxChoice *serialabox;
    wxChoice *serialbbox;

    wxTextCtrl *serialaparam;
    wxTextCtrl *serialbparam;
    wxCheckBox *serialaxon;
    wxCheckBox *serialbxon;
    wxCheckBox *serialalimit;
    wxCheckBox *serialblimit;

    wxString pportopts[3];  // common to all parallel ports
    wxString wpportopts[3]; // Widget on Lisa 2/10

    wxString nothingonly[2];
    wxString serportopts[12];
    int serialopts;

    // ImageWriter settings
    wxChoice *dipsw1_123;
    wxRadioBox *dipsw1_4;
    wxRadioBox *dipsw1_5;
    wxChoice *dipsw1_67;
    wxCheckBox *dipsw1_8;

    wxCheckBox *iw_img_box;  // print to images or printer
    wxTextCtrl *iw_img_path; // dir path to store images
    wxButton *iw_img_path_b; // path browse button

private:
    LisaConfig *my_lisaconfig;
    wxString slotcard[2];
    bool m_dirty; // true when controls hold edits not yet committed via Apply/OK

    DECLARE_EVENT_TABLE()
};

#endif
