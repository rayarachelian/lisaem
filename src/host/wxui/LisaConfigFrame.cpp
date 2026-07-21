/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2007.12.04                   *
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

#include <wx/wx.h>
#include <wx/defs.h>
#include <wx/config.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/simplebook.h>

#include "machine.h"

#include "LisaConfig.h"
#include "LisaConfigFrame.h"

#include "machine.h"

#include <errno.h>

// from vars.c
extern "C"
{
    extern int cheat_ram_test;
    extern int sound_effects_on;
    extern int skins_on_next_run;
    extern int hle;
    extern int macworks4mb;
    extern int double_sided_floppy;
    extern void save_configs(void);
    extern uint8 floppy_iorom;
    extern int consoletermwindow;
};

extern wxString get_config_filename(void);

extern void turn_skins_on(void);
extern void turn_skins_off(void);

extern "C" float hidpi_scale;

// fix for hidpi_scale <1.0 after 2020.01.26 changes
#define HIDPISCALE (hidpi_scale < 1.0 ? 1.0 : hidpi_scale)

enum
{
    ID_NOTEBOOK = 2001,
    ID_APPLY,
    ID_SLOT_CARD,
    ID_SLOT_PICK,
    ID_SERIAL_A,
    ID_SERIAL_B,
    ID_PICK_ROM,
    ID_PICK_DPROM,
    ID_PICK_KB_B,
    ID_PICK_IOROM_B,
    ID_PICK_PPORT_B,
    ID_PICK_PROFILE,
    ID_PICK_PROFILES1H,
    ID_PICK_PROFILES1L,
    ID_PICK_PROFILES2H,
    ID_PICK_PROFILES2L,
    ID_PICK_PROFILES3H,
    ID_PICK_PROFILES3L,
    ID_PICK_PROFILESB1H,
    ID_PICK_PROFILESB1L,
    ID_PICK_PROFILESB2H,
    ID_PICK_PROFILESB2L,
    ID_PICK_PROFILESB3H,
    ID_PICK_PROFILESB3L,
    ID_PICK_IWDIR,
    ID_SERNO_INFO,
    ID_ZAP_PRAM,
    ID_SAVE_PRAM,
    ID_LOAD_PRAM
};

BEGIN_EVENT_TABLE(LisaConfigFrame, wxDialog)
EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, LisaConfigFrame::OnNoteBook)
EVT_NOTEBOOK_PAGE_CHANGING(ID_NOTEBOOK, LisaConfigFrame::OnNoteBook)
EVT_BUTTON(ID_SERNO_INFO, LisaConfigFrame::OnSernoInfo)
EVT_BUTTON(wxID_APPLY, LisaConfigFrame::OnApply)  // dialog Apply: commit, stay open
EVT_BUTTON(wxID_OK, LisaConfigFrame::OnOK)        // dialog OK: commit & close
// slot card dropdown: show/hide that slot's port config (also marks dirty)
EVT_CHOICE(ID_SLOT_CARD, LisaConfigFrame::OnSlotCardChanged)
// slot 1/2/3 selector: switch which slot the slotbook shows (navigation, not an edit)
EVT_CHOICE(ID_SLOT_PICK, LisaConfigFrame::OnSlotPick)
// serial port dropdowns: keep the LisaTest loopback pair in sync in realtime
EVT_CHOICE(ID_SERIAL_A, LisaConfigFrame::OnSerialChanged)
EVT_CHOICE(ID_SERIAL_B, LisaConfigFrame::OnSerialChanged)
EVT_BUTTON(wxID_CANCEL, LisaConfigFrame::OnCancel) // Cancel: warn about unapplied edits
// any edit to a control marks the config dirty and re-enables Apply
EVT_TEXT(wxID_ANY, LisaConfigFrame::OnControlChanged)
EVT_CHECKBOX(wxID_ANY, LisaConfigFrame::OnControlChanged)
EVT_CHOICE(wxID_ANY, LisaConfigFrame::OnControlChanged)
EVT_RADIOBOX(wxID_ANY, LisaConfigFrame::OnControlChanged)
EVT_BUTTON(ID_ZAP_PRAM, LisaConfigFrame::OnZapPram)
EVT_BUTTON(ID_SAVE_PRAM, LisaConfigFrame::OnSavePram)
EVT_BUTTON(ID_LOAD_PRAM, LisaConfigFrame::OnLoadPram)

EVT_BUTTON(ID_PICK_ROM, LisaConfigFrame::OnPickRom)
EVT_BUTTON(ID_PICK_DPROM, LisaConfigFrame::OnPickDRom)

EVT_BUTTON(ID_PICK_PROFILE, LisaConfigFrame::OnPickProFile)

EVT_BUTTON(ID_PICK_PROFILESB1H, LisaConfigFrame::OnPickProFile1H)
EVT_BUTTON(ID_PICK_PROFILESB2H, LisaConfigFrame::OnPickProFile2H)
EVT_BUTTON(ID_PICK_PROFILESB3H, LisaConfigFrame::OnPickProFile3H)
EVT_BUTTON(ID_PICK_PROFILESB1L, LisaConfigFrame::OnPickProFile1L)
EVT_BUTTON(ID_PICK_PROFILESB2L, LisaConfigFrame::OnPickProFile2L)
EVT_BUTTON(ID_PICK_PROFILESB3L, LisaConfigFrame::OnPickProFile3L)
EVT_BUTTON(ID_PICK_IWDIR, LisaConfigFrame::OnPickIWDir)
END_EVENT_TABLE()

const int idth[4] = {0, ID_PICK_PROFILES1H, ID_PICK_PROFILES2H, ID_PICK_PROFILES3H};
const int idtl[4] = {0, ID_PICK_PROFILES1L, ID_PICK_PROFILES2L, ID_PICK_PROFILES3L}; // why is this not used? bug?
const int idbh[4] = {0, ID_PICK_PROFILESB1H, ID_PICK_PROFILESB2H, ID_PICK_PROFILESB3H};
const int idbl[4] = {0, ID_PICK_PROFILESB1L, ID_PICK_PROFILESB2L, ID_PICK_PROFILESB3L};
// idth - upper text field idbh - button upper
// using wxID_ANY for lower for some reason(why?), idbl - button lower line ~493

// JD - Set the size of the frame here. Ideally 650x650.
LisaConfigFrame::LisaConfigFrame(const wxString &title, LisaConfig *lisaconfig)
    : wxDialog(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(650 * HIDPISCALE, 650 * HIDPISCALE),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLIP_CHILDREN)
{

    my_lisaconfig = lisaconfig;
    serialabox = NULL;
    serialbbox = NULL;
    m_dirty = false;
    slotbook = NULL;
    slotpick = NULL;
    for (int s = 0; s < 4; s++)
    {
        sloton[s] = NULL;
        slotports[s] = NULL;
        slotempty[s] = NULL;
    }

    pportopts[0] = wxT("ProFile");
    pportopts[1] = wxT("ADMP");
    pportopts[2] = wxT("Nothing");

    wpportopts[0] = wxT("Widget");
    wpportopts[1] = wxT("ADMP");
    wpportopts[2] = wxT("Nothing");

    nothingonly[0] = _T("Nothing");
    nothingonly[1] = _T("Loopback");

    serportopts[0] = _T("Nothing");
    serportopts[1] = _T("Loopback");
    serportopts[2] = _T("Pipe");
    serportopts[3] = _T("File");
    serportopts[4] = _T("ImageWriter");
    serportopts[5] = _T("Terminal");

#ifndef __MSVCRT__
    serportopts[6] = _T("TelnetD");
    serportopts[7] = _T("Shell");
    serportopts[8] = _T("Serial");
    serportopts[9] = _T("PseudoTTY");
    serialopts = 10;
#else
    serialopts = 6;
#endif

    slotcard[0] = _T("Dual Parallel");
    slotcard[1] = _T("Nothing");

    //  SetMinSize(wxSize(800,700)); //700,500));
    //  SetSize(wxSize(800,700));

    thenoteBook =
        new wxNotebook(this, ID_NOTEBOOK, wxDefaultPosition, wxSize(550, 650));
    CreateNotebook(thenoteBook);

    // All pages are sizer-based now, so let the height fit content; keep a
    // comfortable minimum width so the disk-image path fields aren't cramped.
    thenoteBook->SetMinSize(wxSize(550 * HIDPISCALE, -1));

    // Stage 1: wrap the notebook + a native OK/Cancel button bar so this is a proper modal dialog.
    wxBoxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(thenoteBook, 1, wxEXPAND | wxALL, 6);
    wxSizer *btnsizer = CreateButtonSizer(wxOK | wxCANCEL | wxAPPLY);
    if (btnsizer)
        topsizer->Add(btnsizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    SetSizerAndFit(topsizer);

    // Nothing edited yet: Apply stays dim until the user changes a value.
    SetApplyEnabled(false);
}

void LisaConfigFrame::OnNoteBook(wxNotebookEvent &WXUNUSED(event))
{
    // The LisaTest loopback adapter attaches to both serial ports at once.
    // if one is set to loopback so, must the other follow.
    if (!serialabox || !serialbbox)
        return;

    if (serialabox->GetSelection() == 1 || serialbbox->GetSelection() == 1)
    {
        serialabox->SetSelection(1);
        serialbbox->SetSelection(1);
    }
}

extern "C" uint8 floppy_ram[2048];
extern "C" int islisarunning(void);
// extern "C" void get_lisa_serialnumber(uint32 *plant, uint32 *year, uint32 *day, uint32 *sn, uint32 *prefix, uint32 *net)

void LisaConfigFrame::OnSernoInfo(wxCommandEvent &WXUNUSED(event))
{
    wxString myserno = serialtxt->GetValue();
    wxString hexserno, decserno;
    wxString text, trailer;
    unsigned long serno_i = 0;

    //    uint32 plant, year, day, sn, prefix, net;
    //    get_lisa_serialnumber(&plant, &year, &day, &sn, &prefix, &net);

    hexserno = _T("0x") + myserno.SubString(9, 12); // serial number
    hexserno.ToULong(&serno_i, 16);
    decserno << serno_i;

    if (myserno.SubString(8, 8) == '1')
        trailer = _T("st");
    else if (myserno.SubString(8, 8) == '2')
        trailer = _T("nd");
    else if (myserno.SubString(8, 8) == '3')
        trailer = _T("rd");
    else
        trailer = _T("th");

    //                                               01 23 45 67 89 ab cd ef 01 23 45 67 89 ab
    // floppy.c:get_lisa_serialnumber:1532:serial240: ff 02 83 08 10 40 50 ff 00 10 16 35 04 70 00 00
    //                                               XX XX XP XP XY XY XD XD XD XS XS XS XS XX XX XX
    //                                                      3  8 |0  0 |0  f  0| 0  6  5  4 |

    text =
        _T("Your Lisa's serial number was built\nin Apple Plant #") +
        myserno.SubString(2, 3) + // plant code
        _T(" on the ") +
        myserno.SubString(6, 8) + // day of year
        trailer + _T(" day of 19") +
        myserno.SubString(4, 5) + // year
        _T("\nwith serial #") +
        myserno.SubString(9, 12) + // serial number
        _T(" (") + decserno + _T(")\n") +
        _T("It has the applenet id: ") +
        myserno.SubString(16, 18) + // applenet prefix
        _T(":") +
        myserno.SubString(19, 23); // applenet node number

    wxMessageBox(text, _T("About your Lisa's Serial Numer"), wxICON_INFORMATION | wxOK);
}

void LisaConfigFrame::OnSavePram(wxCommandEvent &WXUNUSED(event))
{
    FILE *F;

    wxFileDialog x(NULL, wxT("Save the Lisa PRAM to a file"),
                   wxEmptyString,
                   wxT("lisaem.pram"),
                   wxT("PRAM (*.pram)|*.pram|All (*.*)|*.*"),
                   (long int)wxFD_SAVE | wxFD_OVERWRITE_PROMPT, wxDefaultPosition);

    if (x.ShowModal() != wxID_OK)
        return;
    // Keep the path in a named wxString: c_str() on the temporary GetPath()
    // returns a buffer that is freed at the end of the statement, so the old
    // code handed fopen() a dangling pointer.
    wxString path = x.GetPath();

    errno = 0;
    F = fopen((const char *)path.mb_str(), "wb");
    if (!F)
    {
        wxMessageBox(_T("Could not open the PRAM file for writing."), _T("File Error!"), wxICON_INFORMATION | wxOK);
        return;
    }

    fwrite(&floppy_ram[0x180 / 2], (0x200 - 0x180) / 2, 1, F);
    if (errno)
    {
        wxMessageBox(_T("An error occured while attempting to write to the PRAM file.  The PRAM was not saved!"),
                     _T("Save Error!"), wxICON_INFORMATION | wxOK);
    }

    fclose(F);
}

void LisaConfigFrame::OnLoadPram(wxCommandEvent &WXUNUSED(event))
{
    FILE *F;
    int count = 0;
    uint8 backup[(0x200 - 0x180) / 2];

    if (islisarunning())
    {
        wxMessageBox(_T("Cannot Load PRAM while the Lisa is running."), _T("Lisa is running!"), wxICON_INFORMATION | wxOK);
        return;
    }

    wxString text = wxT("Loading the PRAM will overwrite the current PRAM with the data saved in a file!\n\n"
                        "You should only use this in emergencies as this can cause more problems than fix.  "
                        "If you say Yes, this will happen even if you do not press the Apply button on the Preferences window and it cannot be undone!\n"
                        "\n\nReally Load PRAM?");
    wxString title = wxT("Load PRAM?");
    wxMessageDialog w(this, text, title, wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT, wxDefaultPosition);
    if (w.ShowModal() != wxID_YES)
        return;

    wxFileDialog x(NULL, wxT("Load a Lisa PRAM file"),
                   wxEmptyString,
                   wxT("lisaem.pram"),
                   wxT("PRAM (*.pram)|*.pram|All (*.*)|*.*"),
                   (long int)wxFD_OPEN | wxFD_FILE_MUST_EXIST, wxDefaultPosition);

    if (x.ShowModal() != wxID_OK)
        return;
    wxString path = x.GetPath(); // named string; GetPath().c_str() would dangle (see OnSavePram)

    memcpy(backup, &floppy_ram[0x180 / 2], (0x200 - 0x180) / 2); // make a backup
    errno = 0;
    F = fopen((const char *)path.mb_str(), "rb");
    if (!F)
    {
        wxMessageBox(_T("Could not open the PRAM file for reading."), _T("File Error!"), wxICON_INFORMATION | wxOK);
        return;
    }

    count = fread(&floppy_ram[0x180 / 2], (0x200 - 0x180) / 2, 1, F);
    if (errno || count != 1)
    {
        memcpy(&floppy_ram[0x180 / 2], backup, (0x200 - 0x180) / 2); // restore the backup if there was a failure.
        wxMessageBox(_T("An error occured while attempting to read from the PRAM file.  The PRAM was not overwrriten."),
                     _T("Load Error!"), wxICON_INFORMATION | wxOK);
    }
    fclose(F);
}

void LisaConfigFrame::OnZapPram(wxCommandEvent &WXUNUSED(event))
{
    if (islisarunning())
    {
        wxMessageBox(_T("Cannot Zap PRAM while the Lisa is running."), _T("Lisa is running!"), wxICON_INFORMATION | wxOK);
        return;
    }

    wxString text = wxT("Zapping the PRAM will zero out the Parameter RAM.\n\n"
                        "You should only use this in emergencies as this can cause more problems than fix.  "
                        "If you say Yes, this will happen even if you do not press the Apply button on the Preferences window.\n"
                        "\n\nReally ZAP PRAM?");
    wxString title = wxT("Zap PRAM?");
    wxMessageDialog w(this, text, title, wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT, wxDefaultPosition);
    if (w.ShowModal() != wxID_YES)
        return;

    memset(&floppy_ram[0x180 / 2], 0, (0x200 - 0x180) / 2);
}

// mount a (possibly changed) ProFile/printer to a VIA, and check power state - both defined in lisaem_wx.cpp
extern void connect_device_to_via(int v, wxString device, wxString *file, wxString profile_prefs_path);
extern "C" int lisa_is_powered_on(void);

void LisaConfigFrame::SetApplyEnabled(bool enabled)
{
    // Apply is enabled exactly when there are uncommitted edits, so this also
    // tracks the dirty state used by the OK/Cancel close warnings.
    m_dirty = enabled;
    // The Apply button is created by CreateButtonSizer(); look it up by id.
    wxWindow *b = FindWindow(wxID_APPLY);
    if (b)
        b->Enable(enabled);
}

void LisaConfigFrame::OnControlChanged(wxCommandEvent &event)
{
    SetApplyEnabled(true); // a value changed -> there's now something to apply
    event.Skip();          // let the control's normal processing continue
}

void LisaConfigFrame::OnSerialChanged(wxCommandEvent &event)
{
    // The LisaTest loopback adapter physically connects both serial ports, so
    // one port going to Loopback forces the other to Loopback as well. Warn and
    // do it in realtime. "Loopback" is index 1 in both nothingonly and serportopts.
    if (serialabox && serialbbox)
    {
        wxChoice *other = (event.GetId() == ID_SERIAL_A) ? serialbbox : serialabox;
        if (event.GetSelection() == 1 && other->GetSelection() != 1)
        {
            wxMessageBox(_T("The LisaTest loopback adapter connects both serial ports.\n\n"
                            "The other serial port will also be set to Loopback."),
                         _T("Loopback"), wxOK | wxICON_INFORMATION, this);
            other->SetSelection(1);
        }
    }
    SetApplyEnabled(true); // changing a serial port is an edit
    event.Skip();
}

void LisaConfigFrame::OnApply(wxCommandEvent &WXUNUSED(event))
{
    ApplyChanges();
    SetApplyEnabled(false); // committed; nothing left to apply until the next edit
}

void LisaConfigFrame::OnOK(wxCommandEvent &WXUNUSED(event))
{
    if (m_dirty)
    {
        wxMessageDialog dlg(this,
                            _T("You have changes that haven't been applied.\n\nApply them before closing?"),
                            _T("Apply changes?"),
                            wxYES_NO | wxCANCEL | wxICON_QUESTION);
        dlg.SetYesNoLabels(_T("Apply"), _T("Don't Apply"));
        int r = dlg.ShowModal();
        if (r == wxID_CANCEL)
            return; // keep the dialog open
        if (r == wxID_YES)
            ApplyChanges();
        // wxID_NO -> close without applying
    }
    EndModal(wxID_OK);
}

void LisaConfigFrame::OnCancel(wxCommandEvent &WXUNUSED(event))
{
    if (m_dirty)
    {
        wxMessageDialog dlg(this,
                            _T("Discard your unapplied changes?"),
                            _T("Discard changes?"),
                            wxYES_NO | wxICON_EXCLAMATION);
        dlg.SetYesNoLabels(_T("Discard"), _T("Keep Editing"));
        if (dlg.ShowModal() != wxID_YES)
            return; // keep editing
    }
    EndModal(wxID_CANCEL);
}

void LisaConfigFrame::ApplyChanges()
{
    // JD - Consider initating a restart of the application here if the ROM file changes.

    // Snapshot current ProFile image paths so we can hot-swap any that change (see end of function).
    // A disk-image change should behave like swapping a real drive: take effect immediately, no reset.
    wxString old_pp   = my_lisaconfig->parallelp;
    wxString old_s1hp = my_lisaconfig->s1hp, old_s1lp = my_lisaconfig->s1lp;
    wxString old_s2hp = my_lisaconfig->s2hp, old_s2lp = my_lisaconfig->s2lp;
    wxString old_s3hp = my_lisaconfig->s3hp, old_s3lp = my_lisaconfig->s3lp;

    // --- configuration page ----------------------------------------------

    // if it hasn't been built yet, don't touch!
    if (!serialtxt)
        return;
    if (!m_rompath)
        return;
    if (!kbbox)
        return;
    if (!serialabox)
        return;
    if (!serialbbox)
        return;
    if (!serialaxon)
        return;
    if (!serialbxon)
        return;
    if (!m_propath)
        return;
    if (!pportbox)
        return;

    if (!m_text_propathh[1])
        return;
    if (!m_text_propathl[1])
        return;
    if (!m_text_propathh[2])
        return;
    if (!m_text_propathl[2])
        return;
    if (!m_text_propathh[3])
        return;
    if (!m_text_propathl[3])
        return;

    // If a disk image is being changed while the Lisa is powered on, warn first: the hot-swap below
    // takes effect immediately, which is fine at the ROM boot menu but can misbehave under a live OS.
    if (lisa_is_powered_on() &&
        (!old_pp.IsSameAs(m_propath->GetValue()) ||
         !old_s1hp.IsSameAs(m_text_propathh[1]->GetValue()) || !old_s1lp.IsSameAs(m_text_propathl[1]->GetValue()) ||
         !old_s2hp.IsSameAs(m_text_propathh[2]->GetValue()) || !old_s2lp.IsSameAs(m_text_propathl[2]->GetValue()) ||
         !old_s3hp.IsSameAs(m_text_propathh[3]->GetValue()) || !old_s3lp.IsSameAs(m_text_propathl[3]->GetValue())))
    {
        wxMessageDialog warn(this,
                             _T("Changing the disk image without powering off the Lisa may result in unpredictable behavior.\n\nContinue?"),
                             _T("Disk image changed while powered on"),
                             wxYES_NO | wxNO_DEFAULT | wxICON_EXCLAMATION);
        if (warn.ShowModal() != wxID_YES)
            return; // user backed out - change nothing
    }

    my_lisaconfig->myserial = serialtxt->GetValue();
    my_lisaconfig->rompath = m_rompath->GetValue();
    my_lisaconfig->dualrom = m_dprompath->GetValue();

    my_lisaconfig->serial1xon = serialaxon->GetValue() ? "1" : "0";
    my_lisaconfig->serial2xon = serialbxon->GetValue() ? "1" : "0";

    consoletermwindow = console_term->GetValue() ? 1 : 0;
    /*
    fprintf(stderr,"myserial number:%s\n",my_lisaconfig->myserial.c_str());
    fprintf(stderr,"rompath        :%s\n",my_lisaconfig->rompath.c_str());
    fprintf(stderr,"dual rom path  :%s\n",my_lisaconfig->dualrom.c_str());
    */

    uint16 kbids[] = {0xBF2f, 0xAF2f, 0xAD2d, 0xAE2e};
    my_lisaconfig->kbid = kbids[kbbox->GetSelection()];

    uint8 ioromids[] = {0xa8, 0x88, 0x89, 0xa9, 0x40};
    my_lisaconfig->iorom = ioromids[iorombox->GetSelection()];

    int memsizes[] = {512, 1024, 1536, 2048};
    my_lisaconfig->mymaxlisaram = memsizes[cpurambox->GetSelection()];
    cheat_ram_test = cheats->GetValue() ? 1 : 0;

    hle = hle_cheats->GetValue() ? 1 : 0;
    macworks4mb = 0; // doesn't work yet // macwx4mb->GetValue() ? 1:0;

    sound_effects_on = soundeffects->GetValue() ? 1 : 0;
    double_sided_floppy = doublesided->GetValue() ? 1 : 0;

    int last_skins_on = skins_on_next_run; // disable this to require restart.
    skins_on_next_run = skinson->GetValue() ? 1 : 0;

    if (skins_on_next_run != last_skins_on)
    {
        if (skins_on_next_run)
            turn_skins_on();
        else
            turn_skins_off();
    }

    // --- ports ------------------------------------------------------------
    my_lisaconfig->serial1_setting = serportopts[serialabox->GetSelection()];
    my_lisaconfig->serial2_setting = serportopts[serialbbox->GetSelection()];
    my_lisaconfig->serial1_param = serialaparam->GetValue();
    my_lisaconfig->serial2_param = serialbparam->GetValue();

    if (my_lisaconfig->serial1_param.IsEmpty() && my_lisaconfig->serial1_setting.IsSameAs(_T("PseudoTTY"), false))
    {
        wxMessageDialog confirm(this,
                                _T("The Pseudo TTY port alias for Serial Port A is empty. A port will still be opened, "
                                    "but we also recommend specifying an alias, e.g. /tmp/lisaem-pseudo-tty-a, "
                                    "so that you don't need to look for the port name.\n\n""Are you sure you want to continue?"),
                                _T("Confirm empty Serial Port A alias name"),
                                wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        if (confirm.ShowModal() != wxID_YES)
            return; // Stop if user chooses No
    }
    if (my_lisaconfig->serial2_param.IsEmpty() && my_lisaconfig->serial2_setting.IsSameAs(_T("PseudoTTY"), false))
    {
        wxMessageDialog confirm(this,
                                _T("The Pseudo TTY port alias for Serial Port B is empty. A port will still be opened, "
                                    "but we also recommend specifying an alias, e.g. /tmp/lisaem-pseudo-tty-b, "
                                    "so that you don't need to look for the port name.\n\n""Are you sure you want to continue?"),
                                _T("Confirm empty Serial Port B alias name"),
                                wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        if (confirm.ShowModal() != wxID_YES)
            return; // Stop if user chooses No
    }

    if (my_lisaconfig->serial1_param.IsEmpty() && my_lisaconfig->serial1_setting.IsSameAs(_T("Serial"), false))
    {
        wxMessageDialog confirm(this,
                                _T("The serial port name for Serial Port A is empty. Please specify a valid physical serial port name, "
                                    "e.g. /dev/ttyUSB0, otherwise this functionality will be disabled.\n\n""Are you sure you want to continue?"),
                                _T("Confirm empty Serial Port A name"),
                                wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        if (confirm.ShowModal() != wxID_YES)
            return; // Stop if user chooses No
    }
    if (my_lisaconfig->serial2_param.IsEmpty() && my_lisaconfig->serial2_setting.IsSameAs(_T("Serial"), false))
    {
        wxMessageDialog confirm(this,
                                 _T("The serial port name for Serial Port B is empty. Please specify a valid physical serial port name, "
                                    "e.g. /dev/ttyUSB0, otherwise this functionality will be disabled.\n\n""Are you sure you want to continue?"),
                                _T("Confirm empty Serial Port B name"),
                                wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        if (confirm.ShowModal() != wxID_YES)
            return; // Stop if user chooses No
    }

    /*
    fprintf(stderr,"serial a setting:%s\n",my_lisaconfig->serial1_setting.c_str());
    fprintf(stderr,"serial b setting:%s\n",my_lisaconfig->serial2_setting.c_str());
    fprintf(stderr,"serial a param  :%s\n",my_lisaconfig->serial1_param.c_str());
    fprintf(stderr,"serial a param  :%s\n",my_lisaconfig->serial2_param.c_str());
    */

    my_lisaconfig->parallel = pportopts[pportbox->GetSelection()];
    my_lisaconfig->parallelp = m_propath->GetValue();

    // fprintf(stderr,"parallel port   :%s\n",my_lisaconfig->parallel.c_str());
    // fprintf(stderr,"parameter       :%s\n\n",my_lisaconfig->parallelp.c_str());
    // fflush(stderr);

    // --- slots ------------------------------------------------------------

    my_lisaconfig->slot1 = slotcard[sloton[1]->GetSelection()];
    my_lisaconfig->slot2 = slotcard[sloton[2]->GetSelection()];
    my_lisaconfig->slot3 = slotcard[sloton[3]->GetSelection()];

    my_lisaconfig->s1h = pportopts[pportboxh[1]->GetSelection()];
    my_lisaconfig->s1l = pportopts[pportboxl[1]->GetSelection()];
    my_lisaconfig->s2h = pportopts[pportboxh[2]->GetSelection()];
    my_lisaconfig->s2l = pportopts[pportboxl[2]->GetSelection()];
    my_lisaconfig->s3h = pportopts[pportboxh[3]->GetSelection()];
    my_lisaconfig->s3l = pportopts[pportboxl[3]->GetSelection()];

    my_lisaconfig->s1hp = m_text_propathh[1]->GetValue();
    my_lisaconfig->s1lp = m_text_propathl[1]->GetValue();
    my_lisaconfig->s2hp = m_text_propathh[2]->GetValue();
    my_lisaconfig->s2lp = m_text_propathl[2]->GetValue();
    my_lisaconfig->s3hp = m_text_propathh[3]->GetValue();
    my_lisaconfig->s3lp = m_text_propathl[3]->GetValue();

    // --- imagewriter settings ---------------------------------------------
    my_lisaconfig->iw_dipsw_1 = (dipsw1_123->GetSelection()) |
                                (dipsw1_4->GetSelection() << 3) |
                                (dipsw1_5->GetSelection() << 4) |
                                (dipsw1_67->GetSelection() << 5) |
                                (dipsw1_8->GetValue() ? 128 : 0);
    my_lisaconfig->iw_png_on = iw_img_box->GetValue();
    my_lisaconfig->iw_png_path = iw_img_path->GetValue();

    save_configs();

#ifdef DEBUG
    // Debug build only: dump the ProFile hot-swap decision (old -> new path, and whether the new
    // file exists) on every Apply. Raw stderr so it prints regardless of the runtime debug-log state.
    fprintf(stderr,
            "LisaEm hot-swap check:\n"
            "  pp : '%s' -> '%s' exists=%d\n"
            "  s1h: '%s' -> '%s' exists=%d\n"
            "  s1l: '%s' -> '%s' exists=%d\n"
            "  s2h: '%s' -> '%s' exists=%d\n"
            "  s2l: '%s' -> '%s' exists=%d\n"
            "  s3h: '%s' -> '%s' exists=%d\n"
            "  s3l: '%s' -> '%s' exists=%d\n",
            (const char *)old_pp.mb_str(),   (const char *)my_lisaconfig->parallelp.mb_str(), (int)wxFileExists(my_lisaconfig->parallelp),
            (const char *)old_s1hp.mb_str(), (const char *)my_lisaconfig->s1hp.mb_str(),       (int)wxFileExists(my_lisaconfig->s1hp),
            (const char *)old_s1lp.mb_str(), (const char *)my_lisaconfig->s1lp.mb_str(),       (int)wxFileExists(my_lisaconfig->s1lp),
            (const char *)old_s2hp.mb_str(), (const char *)my_lisaconfig->s2hp.mb_str(),       (int)wxFileExists(my_lisaconfig->s2hp),
            (const char *)old_s2lp.mb_str(), (const char *)my_lisaconfig->s2lp.mb_str(),       (int)wxFileExists(my_lisaconfig->s2lp),
            (const char *)old_s3hp.mb_str(), (const char *)my_lisaconfig->s3hp.mb_str(),       (int)wxFileExists(my_lisaconfig->s3hp),
            (const char *)old_s3lp.mb_str(), (const char *)my_lisaconfig->s3lp.mb_str(),       (int)wxFileExists(my_lisaconfig->s3lp));
    fflush(stderr);
#endif

    // Hot-swap any ProFile image whose path changed AND actually exists on disk: unmount the old
    // image and mount the new one right now, mimicking a real drive swap. The running ROM then boots
    // the new disk on its next "Startup from" without a reset/power-cycle. connect_device_to_via()
    // closes the old image first (no fd/mmap leak) and calls ProfileReset(); it is a no-op for
    // non-PROFILE devices. The wxFileExists() guard keeps a live swap from popping profile_mount's
    // "create a new hard disk?" (size-picker) dialog when a path points at a not-yet-created image.
    if (!old_pp.IsSameAs(my_lisaconfig->parallelp) && wxFileExists(my_lisaconfig->parallelp))
        connect_device_to_via(2, my_lisaconfig->parallel, &my_lisaconfig->parallelp, "/parallelport/path");
    if (!old_s1hp.IsSameAs(my_lisaconfig->s1hp) && wxFileExists(my_lisaconfig->s1hp))
        connect_device_to_via(3, my_lisaconfig->s1h, &my_lisaconfig->s1hp, "/cardslot1/highpath");
    if (!old_s1lp.IsSameAs(my_lisaconfig->s1lp) && wxFileExists(my_lisaconfig->s1lp))
        connect_device_to_via(4, my_lisaconfig->s1l, &my_lisaconfig->s1lp, "/cardslot1/lowpath");
    if (!old_s2hp.IsSameAs(my_lisaconfig->s2hp) && wxFileExists(my_lisaconfig->s2hp))
        connect_device_to_via(5, my_lisaconfig->s2h, &my_lisaconfig->s2hp, "/cardslot2/highpath");
    if (!old_s2lp.IsSameAs(my_lisaconfig->s2lp) && wxFileExists(my_lisaconfig->s2lp))
        connect_device_to_via(6, my_lisaconfig->s2l, &my_lisaconfig->s2lp, "/cardslot2/lowpath");
    if (!old_s3hp.IsSameAs(my_lisaconfig->s3hp) && wxFileExists(my_lisaconfig->s3hp))
        connect_device_to_via(7, my_lisaconfig->s3h, &my_lisaconfig->s3hp, "/cardslot3/highpath");
    if (!old_s3lp.IsSameAs(my_lisaconfig->s3lp) && wxFileExists(my_lisaconfig->s3lp))
        connect_device_to_via(8, my_lisaconfig->s3l, &my_lisaconfig->s3lp, "/cardslot3/lowpath");
}

wxPanel *LisaConfigFrame::CreateSlotConfigPage(wxWindow *parent, int slot)
{
    if (slot < 1 || slot > 3)
        return NULL;

    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *page = new wxBoxSizer(wxVERTICAL);
    const int B = 6 * HIDPISCALE;

    wxString u, l, cu, cl, s;
    switch (slot)
    {
    case 1: s = my_lisaconfig->slot1; u = my_lisaconfig->s1hp; l = my_lisaconfig->s1lp; cu = my_lisaconfig->s1h; cl = my_lisaconfig->s1l; break;
    case 2: s = my_lisaconfig->slot2; u = my_lisaconfig->s2hp; l = my_lisaconfig->s2lp; cu = my_lisaconfig->s2h; cl = my_lisaconfig->s2l; break;
    case 3: s = my_lisaconfig->slot3; u = my_lisaconfig->s3hp; l = my_lisaconfig->s3lp; cu = my_lisaconfig->s3h; cl = my_lisaconfig->s3l; break;
    }

    // ---- Installed card ------------------------------------------------
    {
        wxBoxSizer *cardrow = new wxBoxSizer(wxHORIZONTAL);
        cardrow->Add(new wxStaticText(panel, wxID_ANY, _T("Card:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        sloton[slot] = new wxChoice(panel, ID_SLOT_CARD, wxDefaultPosition, wxDefaultSize, 2, slotcard);
        sloton[slot]->SetSelection(s.IsSameAs(slotcard[0], false) ? 0 : 1);
        cardrow->Add(sloton[slot], 0, wxALIGN_CENTER_VERTICAL);
        page->Add(cardrow, 0, wxALL, B);
    }

    // ---- Port config (shown only when a Dual-Parallel card is installed) ----
    slotports[slot] = new wxPanel(panel);
    wxBoxSizer *ports = new wxBoxSizer(wxVERTICAL);

    // Upper (Connector 2 in LOS)
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, slotports[slot], _T("Upper Parallel Port (Connector 2 in LOS)"));
        pportboxh[slot] = new wxRadioBox(slotports[slot], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                         3, pportopts, 0, wxRA_SPECIFY_COLS);
        if (cu.IsSameAs(_T("PROFILE"), false))   pportboxh[slot]->SetSelection(0);
        else if (cu.IsSameAs(_T("ADMP"), false)) pportboxh[slot]->SetSelection(1);
        else                                     pportboxh[slot]->SetSelection(2);
        g->Add(pportboxh[slot], 0, wxALL, B);

        wxBoxSizer *r = new wxBoxSizer(wxHORIZONTAL);
        r->Add(new wxStaticText(slotports[slot], wxID_ANY, _T("Disk image:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        m_text_propathh[slot] = new wxTextCtrl(slotports[slot], idth[slot], u);
        r->Add(m_text_propathh[slot], 1, wxALIGN_CENTER_VERTICAL);
        r->Add(new wxButton(slotports[slot], idbh[slot], wxT("Browse...")), 0, wxLEFT, B);
        g->Add(r, 0, wxEXPAND | wxALL, B);
        ports->Add(g, 0, wxEXPAND | wxBOTTOM, B);
    }

    // Lower (Connector 1 in LOS)
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, slotports[slot], _T("Lower Parallel Port (Connector 1 in LOS)"));
        pportboxl[slot] = new wxRadioBox(slotports[slot], wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                         3, pportopts, 0, wxRA_SPECIFY_COLS);
        if (cl.IsSameAs(_T("PROFILE"), false))   pportboxl[slot]->SetSelection(0);
        else if (cl.IsSameAs(_T("ADMP"), false)) pportboxl[slot]->SetSelection(1);
        else                                     pportboxl[slot]->SetSelection(2);
        g->Add(pportboxl[slot], 0, wxALL, B);

        wxBoxSizer *r = new wxBoxSizer(wxHORIZONTAL);
        r->Add(new wxStaticText(slotports[slot], wxID_ANY, _T("Disk image:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        m_text_propathl[slot] = new wxTextCtrl(slotports[slot], idtl[slot], l);
        r->Add(m_text_propathl[slot], 1, wxALIGN_CENTER_VERTICAL);
        r->Add(new wxButton(slotports[slot], idbl[slot], wxT("Browse...")), 0, wxLEFT, B);
        g->Add(r, 0, wxEXPAND | wxALL, B);
        ports->Add(g, 0, wxEXPAND);
    }

    slotports[slot]->SetSizer(ports);
    page->Add(slotports[slot], 0, wxEXPAND | wxALL, B);

    // Placeholder shown instead of the port config when no card is installed.
    slotempty[slot] = new wxStaticText(panel, wxID_ANY, _T("No card installed in this slot."),
                                       wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    slotempty[slot]->SetForegroundColour(wxColour(0x6e, 0x6e, 0x73));
    page->Add(slotempty[slot], 0, wxEXPAND | wxALL, B * 3);

    // Only meaningful with a card installed (slotcard[0] == "Dual Parallel").
    bool installed = (sloton[slot]->GetSelection() == 0);
    slotports[slot]->Show(installed);
    slotempty[slot]->Show(!installed);

    panel->SetSizer(page);
    return panel;
}

void LisaConfigFrame::UpdateSlotVisibility(int slot)
{
    if (slot < 1 || slot > 3 || !sloton[slot] || !slotports[slot])
        return;
    bool installed = (sloton[slot]->GetSelection() == 0); // slotcard[0] == "Dual Parallel"
    if (slotports[slot]->IsShown() != installed)
    {
        slotports[slot]->Show(installed);
        if (slotempty[slot])
            slotempty[slot]->Show(!installed);
        slotports[slot]->GetParent()->Layout();
    }
}

void LisaConfigFrame::OnSlotCardChanged(wxCommandEvent &event)
{
    for (int s = 1; s <= 3; s++)
        UpdateSlotVisibility(s);
    SetApplyEnabled(true); // choosing a card counts as an edit
    event.Skip();
}

void LisaConfigFrame::OnSlotPick(wxCommandEvent &event)
{
    // Just navigation between the three slot panels; not an edit.
    if (slotbook)
        slotbook->SetSelection(event.GetSelection());
}

wxPanel *LisaConfigFrame::CreateMainConfigPage(wxNotebook *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *page = new wxBoxSizer(wxVERTICAL);

    const int B = 6 * HIDPISCALE; // common border

    // ---- Memory --------------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxHORIZONTAL, panel, _T("Memory"));

        wxString ramsize[] = {wxT("0.5 MB"), wxT("1 MB"), wxT("1.5 MB"), wxT("2 MB")};
        cpurambox = new wxRadioBox(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
#ifdef ALLOW2MBRAM
                                   4, // 3 to turn off 2mb / 4 to re-enable 2MB (also uncomment case 2048 below)
#else
                                   3,
#endif
                                   ramsize, 0, wxRA_SPECIFY_COLS);
        switch (my_lisaconfig->mymaxlisaram)
        {
        case 512:  cpurambox->SetSelection(0); break;
        case 1024: cpurambox->SetSelection(1); break;
        case 1536: cpurambox->SetSelection(2); break;
#ifdef ALLOW2MBRAM
        case 2048: cpurambox->SetSelection(3); break;
#endif
        default:   cpurambox->SetSelection(2);
        }
        g->Add(cpurambox, 0, wxALL, B);
        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- Identity ------------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("Identity"));

        wxBoxSizer *r1 = new wxBoxSizer(wxHORIZONTAL);
        r1->Add(new wxStaticText(panel, wxID_ANY, _T("Serial number:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        serialtxt = new wxTextCtrl(panel, wxID_ANY, my_lisaconfig->myserial);
        r1->Add(serialtxt, 1, wxALIGN_CENTER_VERTICAL);
        r1->Add(new wxButton(panel, ID_SERNO_INFO, wxT("Info...")), 0, wxLEFT, B);
        g->Add(r1, 0, wxEXPAND | wxALL, B);

        wxBoxSizer *r2 = new wxBoxSizer(wxHORIZONTAL);
        r2->Add(new wxStaticText(panel, wxID_ANY, _T("Keyboard:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        wxString kbid[] = {wxT("US"), wxT("UK"), wxT("FR"), wxT("DE")};
        kbbox = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 4, kbid);
        switch (my_lisaconfig->kbid)
        {
        case 0xAD2d: kbbox->SetSelection(2); break;
        case 0xAE2e: kbbox->SetSelection(3); break;
        case 0xAF2f: kbbox->SetSelection(1); break;
        case 0xBF2f:
        default:     kbbox->SetSelection(0);
        }
        r2->Add(kbbox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B * 3);
        r2->Add(new wxStaticText(panel, wxID_ANY, _T("I/O ROM:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        wxString iorom[] = {wxT("A8"), wxT("88"), wxT("89"), wxT("A9"), wxT("40")};
        iorombox = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 5, iorom);
        switch (my_lisaconfig->iorom)
        {
        case 0x88: iorombox->SetSelection(1); break;
        case 0x89: iorombox->SetSelection(2); break;
        case 0xa9: iorombox->SetSelection(3); break;
        case 0x40: iorombox->SetSelection(4); break;
        case 0xa8:
        default:   iorombox->SetSelection(0);
        }
        r2->Add(iorombox, 0, wxALIGN_CENTER_VERTICAL);
        g->Add(r2, 0, wxEXPAND | wxALL, B);
        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- ROMs ----------------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("ROMs"));

        wxFlexGridSizer *fg = new wxFlexGridSizer(2, 3, B, B);
        fg->AddGrowableCol(1, 1);

        fg->Add(new wxStaticText(panel, wxID_ANY, _T("Lisa boot ROM:")), 0, wxALIGN_CENTER_VERTICAL);
        m_rompath = new wxTextCtrl(panel, wxID_ANY, my_lisaconfig->rompath);
        fg->Add(m_rompath, 1, wxEXPAND);
        b_rompath = new wxButton(panel, ID_PICK_ROM, wxT("Browse..."));
        fg->Add(b_rompath, 0);

        fg->Add(new wxStaticText(panel, wxID_ANY, _T("Dual-Parallel:")), 0, wxALIGN_CENTER_VERTICAL);
        m_dprompath = new wxTextCtrl(panel, wxID_ANY, my_lisaconfig->dualrom);
        fg->Add(m_dprompath, 1, wxEXPAND);
        b_dprompath = new wxButton(panel, ID_PICK_DPROM, wxT("Browse..."));
        fg->Add(b_dprompath, 0);

        g->Add(fg, 0, wxEXPAND | wxALL, B);

        wxStaticText *warn = new wxStaticText(panel, wxID_ANY,
                                              _T("Note: changing the Lisa boot ROM requires restarting LisaEm."));
        warn->SetForegroundColour(wxColour(0xB2, 0x50, 0x00));
        g->Add(warn, 0, wxLEFT | wxRIGHT | wxBOTTOM, B);
        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- Behavior ------------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("Behavior"));
        wxFlexGridSizer *fg = new wxFlexGridSizer(3, 2, B, B * 4);
        fg->AddGrowableCol(0, 1);
        fg->AddGrowableCol(1, 1);

        cheats = new wxCheckBox(panel, wxID_ANY, wxT("Boot ROM speedup hacks"));
        cheats->SetValue((bool)(cheat_ram_test));
        fg->Add(cheats);
        hle_cheats = new wxCheckBox(panel, wxID_ANY, wxT("Hard drive acceleration"));
        hle_cheats->SetValue((bool)(hle));
        fg->Add(hle_cheats);
        console_term = new wxCheckBox(panel, wxID_ANY, wxT("Console terminal"));
        console_term->SetValue((bool)consoletermwindow);
        fg->Add(console_term);
        doublesided = new wxCheckBox(panel, wxID_ANY, wxT("Double-sided Sony floppy"));
        doublesided->SetValue((bool)(double_sided_floppy));
        fg->Add(doublesided);
        soundeffects = new wxCheckBox(panel, wxID_ANY, wxT("Sound effects"));
        soundeffects->SetValue((bool)(sound_effects_on));
        fg->Add(soundeffects);
        skinson = new wxCheckBox(panel, wxID_ANY, wxT("Lisa skins"));
        skinson->SetValue((bool)(skins_on_next_run));
        fg->Add(skinson);

        g->Add(fg, 0, wxEXPAND | wxALL, B);
        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- Parameter RAM -------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxHORIZONTAL, panel, _T("Parameter RAM (PRAM)"));
        g->AddStretchSpacer(1);
        g->Add(new wxButton(panel, ID_SAVE_PRAM, wxT("Save...")), 0, wxRIGHT, B);
        g->Add(new wxButton(panel, ID_LOAD_PRAM, wxT("Load...")), 0, wxRIGHT, B);
        g->Add(new wxButton(panel, ID_ZAP_PRAM, wxT("Zap")), 0, wxRIGHT, B);
        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- Prefs file location (footer) ---------------------------------
    wxStaticText *pf = new wxStaticText(panel, wxID_ANY, _T("Prefs file: ") + get_config_filename());
    pf->SetForegroundColour(wxColour(0x6e, 0x6e, 0x73));
    page->Add(pf, 0, wxALL, B);

    panel->SetSizer(page);
    return panel;
}

wxPanel *LisaConfigFrame::CreatePortsConfigPage(wxNotebook *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *page = new wxBoxSizer(wxVERTICAL);
    const int B = 6 * HIDPISCALE;
    int i;

    // ---- Serial Port A -------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("Serial Port A"));

        wxBoxSizer *r1 = new wxBoxSizer(wxHORIZONTAL);
        r1->Add(new wxStaticText(panel, wxID_ANY, _T("Port:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
#ifndef ALLOWSERIALA
        // Serial A is limited to Nothing / Loopback (the LisaTest loopback adapter).
        // nothingonly[0..1] mirror serportopts[0..1], so ApplyChanges' serportopts[]
        // lookup and the loopback sync (which selects index 1) both stay correct.
        serialabox = new wxChoice(panel, ID_SERIAL_A, wxDefaultPosition, wxDefaultSize, 2, nothingonly);
        serialabox->SetSelection(my_lisaconfig->serial1_setting.IsSameAs(_T("Loopback"), false) ? 1 : 0);
#else
        serialabox = new wxChoice(panel, ID_SERIAL_A, wxDefaultPosition, wxDefaultSize, serialopts, serportopts);
        for (i = 0; i < serialopts; i++)
            if (my_lisaconfig->serial1_setting.IsSameAs(serportopts[i], false))
                serialabox->SetSelection(i);
#endif
        r1->Add(serialabox, 0, wxALIGN_CENTER_VERTICAL);
        r1->AddStretchSpacer(1);
        serialaxon = new wxCheckBox(panel, wxID_ANY, wxT("Xon/Xoff flow control"));
        serialaxon->SetValue((bool)(my_lisaconfig->serial1xon.IsSameAs(_T("1"), false)));
        r1->Add(serialaxon, 0, wxALIGN_CENTER_VERTICAL);
        g->Add(r1, 0, wxEXPAND | wxALL, B);

        wxBoxSizer *r2 = new wxBoxSizer(wxHORIZONTAL);
        r2->Add(new wxStaticText(panel, wxID_ANY, _T("Settings:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        serialaparam = new wxTextCtrl(panel, wxID_ANY, my_lisaconfig->serial1_param);
        r2->Add(serialaparam, 1, wxALIGN_CENTER_VERTICAL);
        g->Add(r2, 0, wxEXPAND | wxALL, B);

        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- Serial Port B -------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("Serial Port B"));

        wxBoxSizer *r1 = new wxBoxSizer(wxHORIZONTAL);
        r1->Add(new wxStaticText(panel, wxID_ANY, _T("Port:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        serialbbox = new wxChoice(panel, ID_SERIAL_B, wxDefaultPosition, wxDefaultSize, serialopts, serportopts);
        for (i = 0; i < serialopts; i++)
            if (my_lisaconfig->serial2_setting.IsSameAs(serportopts[i], false))
                serialbbox->SetSelection(i);
        r1->Add(serialbbox, 0, wxALIGN_CENTER_VERTICAL);
        r1->AddStretchSpacer(1);
        serialbxon = new wxCheckBox(panel, wxID_ANY, wxT("Xon/Xoff flow control"));
        serialbxon->SetValue((bool)(my_lisaconfig->serial2xon.IsSameAs(_T("1"), false)));
        r1->Add(serialbxon, 0, wxALIGN_CENTER_VERTICAL);
        g->Add(r1, 0, wxEXPAND | wxALL, B);

        wxBoxSizer *r2 = new wxBoxSizer(wxHORIZONTAL);
        r2->Add(new wxStaticText(panel, wxID_ANY, _T("Settings:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        serialbparam = new wxTextCtrl(panel, wxID_ANY, my_lisaconfig->serial2_param);
        r2->Add(serialbparam, 1, wxALIGN_CENTER_VERTICAL);
        g->Add(r2, 0, wxEXPAND | wxALL, B);

        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- Built-in Parallel Port ----------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("Built-in Parallel Port"));

        // Radio labels read "Profile"/"Widget" depending on the I/O ROM version.
        pportbox = new wxRadioBox(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 3,
                                  (floppy_iorom == 0x88) ? wpportopts : pportopts,
                                  0, wxRA_SPECIFY_COLS);
        // default to profile for the builtin parallel port
        if (my_lisaconfig->parallel.IsSameAs(_T("Nothing"), false))
            pportbox->SetSelection(2);
        else if (my_lisaconfig->parallel.IsSameAs(_T("ADMP"), false))
            pportbox->SetSelection(1);
        else
            pportbox->SetSelection(0);
        g->Add(pportbox, 0, wxALL, B);

        wxBoxSizer *r = new wxBoxSizer(wxHORIZONTAL);
        r->Add(new wxStaticText(panel, wxID_ANY, _T("Disk image:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        m_propath = new wxTextCtrl(panel, wxID_ANY, my_lisaconfig->parallelp);
        r->Add(m_propath, 1, wxALIGN_CENTER_VERTICAL);
        b_propath = new wxButton(panel, ID_PICK_PROFILE, wxT("Browse..."));
        r->Add(b_propath, 0, wxLEFT, B);
        g->Add(r, 0, wxEXPAND | wxALL, B);

        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    panel->SetSizer(page);
    return panel;
}

wxPanel *LisaConfigFrame::CreatePrinterConfigPage(wxNotebook *parent)
{
    wxPanel *panel = new wxPanel(parent);
    wxBoxSizer *page = new wxBoxSizer(wxVERTICAL);
    const int B = 6 * HIDPISCALE;

    // Label text is cosmetic; ApplyChanges reads these by index, so the option
    // order below must match the DIP-switch bit encoding (see iw_dipsw_1).

    // ---- ImageWriter / ADMP - DIP Switch 1 -----------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("ImageWriter / ADMP - DIP Switch 1"));

        wxFlexGridSizer *fg = new wxFlexGridSizer(0, 2, B, B);
        fg->AddGrowableCol(1, 1);

        wxString fontopt[] = {wxT("American"), wxT("German"), wxT("American 2"), wxT("French"),
                              wxT("Italian"), wxT("Swedish"), wxT("British"), wxT("Spanish")};
        fg->Add(new wxStaticText(panel, wxID_ANY, _T("Font (pins 1-3):")), 0, wxALIGN_CENTER_VERTICAL);
        dipsw1_123 = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 8, fontopt);
        dipsw1_123->SetSelection(my_lisaconfig->iw_dipsw_1 & 7);
        fg->Add(dipsw1_123, 0, wxEXPAND);

        wxString bit67opt[] = {wxT("Elite Proportional"), wxT("Elite 12 cpi"),
                               wxT("Ultracondensed 17 cpi"), wxT("Pica 10 cpi")};
        fg->Add(new wxStaticText(panel, wxID_ANY, _T("Pitch (pins 6-7):")), 0, wxALIGN_CENTER_VERTICAL);
        dipsw1_67 = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 4, bit67opt);
        dipsw1_67->SetSelection((my_lisaconfig->iw_dipsw_1 >> 5) & 3);
        fg->Add(dipsw1_67, 0, wxEXPAND);

        g->Add(fg, 0, wxEXPAND | wxALL, B);

        wxBoxSizer *radios = new wxBoxSizer(wxHORIZONTAL);
        wxString bit4opt[] = {wxT("66 lines"), wxT("72 lines")};
        dipsw1_4 = new wxRadioBox(panel, wxID_ANY, _T("Lines (pin 4)"), wxDefaultPosition, wxDefaultSize,
                                  2, bit4opt, 1, wxRA_SPECIFY_ROWS);
        dipsw1_4->SetSelection(!!(my_lisaconfig->iw_dipsw_1 & 8));
        radios->Add(dipsw1_4, 0, wxRIGHT, B * 2);
        wxString bit5opt[] = {wxT("8-bit"), wxT("7-bit")};
        dipsw1_5 = new wxRadioBox(panel, wxID_ANY, _T("Data (pin 5)"), wxDefaultPosition, wxDefaultSize,
                                  2, bit5opt, 1, wxRA_SPECIFY_ROWS);
        dipsw1_5->SetSelection(!!(my_lisaconfig->iw_dipsw_1 & 16));
        radios->Add(dipsw1_5, 0);
        g->Add(radios, 0, wxALL, B);

        dipsw1_8 = new wxCheckBox(panel, wxID_ANY, _T("Auto LF after CR (pin 8)"));
        dipsw1_8->SetValue((bool)!!(my_lisaconfig->iw_dipsw_1 & 128));
        g->Add(dipsw1_8, 0, wxALL, B);

        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    // ---- Output --------------------------------------------------------
    {
        wxStaticBoxSizer *g = new wxStaticBoxSizer(wxVERTICAL, panel, _T("Output"));

        iw_img_box = new wxCheckBox(panel, wxID_ANY, _T("Print to images"));
        iw_img_box->SetValue((bool)(!!my_lisaconfig->iw_png_on));
        g->Add(iw_img_box, 0, wxALL, B);

        wxBoxSizer *r = new wxBoxSizer(wxHORIZONTAL);
        r->Add(new wxStaticText(panel, wxID_ANY, _T("Folder:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, B);
        iw_img_path = new wxTextCtrl(panel, wxID_ANY, my_lisaconfig->iw_png_path);
        r->Add(iw_img_path, 1, wxALIGN_CENTER_VERTICAL);
        iw_img_path_b = new wxButton(panel, ID_PICK_IWDIR, wxT("Browse..."));
        r->Add(iw_img_path_b, 0, wxLEFT, B);
        g->Add(r, 0, wxEXPAND | wxALL, B);

        page->Add(g, 0, wxEXPAND | wxALL, B);
    }

    panel->SetSizer(page);
    return panel;
}

void LisaConfigFrame::CreateNotebook(wxNotebook *parent)
{
    wxPanel *panel1 = CreateMainConfigPage(parent);
    wxPanel *panel2 = CreatePortsConfigPage(parent);

    // The three expansion slots share one tab: a compact "Slot 1/2/3" picker
    // drives a simplebook holding the three slot panels.
    const int B = 6 * HIDPISCALE;
    wxPanel *slotsTab = new wxPanel(parent);
    wxBoxSizer *sv = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *pickrow = new wxBoxSizer(wxHORIZONTAL);
    wxString slotnums[] = {wxT("Slot 1"), wxT("Slot 2"), wxT("Slot 3")};
    slotpick = new wxChoice(slotsTab, ID_SLOT_PICK, wxDefaultPosition, wxDefaultSize, 3, slotnums);
    slotpick->SetSelection(0);
    pickrow->Add(slotpick, 0, wxALIGN_CENTER_VERTICAL);
    sv->Add(pickrow, 0, wxALL, B);

    slotbook = new wxSimplebook(slotsTab, wxID_ANY);
    slotbook->AddPage(CreateSlotConfigPage(slotbook, 1), wxT("Slot 1"));
    slotbook->AddPage(CreateSlotConfigPage(slotbook, 2), wxT("Slot 2"));
    slotbook->AddPage(CreateSlotConfigPage(slotbook, 3), wxT("Slot 3"));
    sv->Add(slotbook, 1, wxEXPAND | wxALL, B);
    slotsTab->SetSizer(sv);

    wxPanel *panel6 = CreatePrinterConfigPage(parent);

    parent->AddPage(panel1, wxT("Machine"), false, -1);
    parent->AddPage(panel2, wxT("Ports"), false, -1);
    parent->AddPage(slotsTab, wxT("Slots"), false, -1);
    parent->AddPage(panel6, wxT("Printer"), false, -1);

    parent->SetSelection(0);
}

void LisaConfigFrame::OnPickRom(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog x(NULL, wxT("Open a Lisa Boot ROM"));
    if (x.ShowModal() == wxID_OK)
        m_rompath->SetValue(x.GetPath());
    // wxString  x=wxFileSelector( wxT("Open a Lisa Boot ROM") );
    // if (x.Len()>3) m_rompath->SetValue(x);
}

void LisaConfigFrame::OnPickDRom(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog x(NULL, wxT("Open a Lisa Dual Parallel Card ROM"));
    if (x.ShowModal() == wxID_OK)
        m_dprompath->SetValue(x.GetPath());
}

// Reject a floppy dc42 chosen for a ProFile slot. Raw/unknown (.image) is allowed here; the
// "unable to identify disk format" note is shown later at mount time. Defined in libdc42 (DC42_KIND_FLOPPY==2).
extern "C" int dc42_classify_image(char *filename);

// True if `path` is already set in another ProFile/Widget image field (not `except`).
// The same backing disk image must never be attached to two devices at once, as
// both would write to it and corrupt each other.
bool LisaConfigFrame::path_in_use_elsewhere(const wxString &path, wxTextCtrl *except)
{
    if (path.IsEmpty())
        return false;

    wxTextCtrl *fields[] = {
        m_propath,
        m_text_propathh[1], m_text_propathl[1],
        m_text_propathh[2], m_text_propathl[2],
        m_text_propathh[3], m_text_propathl[3]};

    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); i++)
    {
        if (!fields[i] || fields[i] == except)
            continue;
        if (path.IsSameAs(fields[i]->GetValue())) // paths are case-sensitive
            return true;
    }
    return false;
}

void LisaConfigFrame::set_profile_path_checked(wxTextCtrl *target, const wxString &path)
{
    if (path_in_use_elsewhere(path, target)) // don't attach the same image twice
    {
        wxMessageDialog(this,
                        _T("That disk image is already attached to another port or slot.\n\n"
                           "The same image cannot be used in two places at once."),
                        _T("Disk image already in use"),
                        wxOK | wxICON_EXCLAMATION)
            .ShowModal();
        return; // leave the field unchanged
    }

    int kind = dc42_classify_image((char *)(const char *)path.mb_str());
    if (kind == 2) // DC42_KIND_FLOPPY - a dc42 floppy is the wrong type for a ProFile slot: reject
    {
        wxMessageDialog(this,
                        _T("That's a floppy disk image, not a ProFile.\n\nPlease choose a ProFile image (or a raw image)."),
                        _T("Wrong image type for a ProFile slot"),
                        wxOK | wxICON_EXCLAMATION)
            .ShowModal();
        return; // leave the field unchanged
    }
    if (kind == 0) // DC42_KIND_RAW - not a recognizable dc42; allow it, but say the format is unknown
    {
        wxMessageDialog(this,
                        _T("Unable to identify disk format. It will be used as-is."),
                        _T("Unrecognized disk image"),
                        wxOK | wxICON_INFORMATION)
            .ShowModal();
    }
    target->SetValue(path);
}

void LisaConfigFrame::OnPickProFile(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog open(this, wxT("Select ProFile drive image:"),
                      wxEmptyString,
                      wxT("lisaem-profile.dc42"),
                      wxT("Disk image (*.dc42)|*.dc42|Raw image (*.image)|*.image|All files (*.*)|*.*"),
                      (long int)wxFD_OPEN, wxDefaultPosition);

    if (open.ShowModal() == wxID_OK)
        set_profile_path_checked(m_propath, open.GetPath());
}

// slot 1

void LisaConfigFrame::OnPickProFile1H(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog open(NULL, wxT("Select ProFile image for Slot 1 upper port:"),
                      wxEmptyString,
                      wxT("lisaem-profile-s1h.dc42"),
                      wxT("Disk image (*.dc42)|*.dc42|Raw image (*.image)|*.image|All files (*.*)|*.*"),
                      wxFD_OPEN);
    if (open.ShowModal() == wxID_OK)
        set_profile_path_checked(m_text_propathh[1], open.GetPath());
}

void LisaConfigFrame::OnPickProFile1L(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog open(NULL, wxT("Select ProFile image for Slot 1 lower port:"),
                      wxEmptyString,
                      wxT("lisaem-profile-s1l.dc42"),
                      wxT("Disk image (*.dc42)|*.dc42|Raw image (*.image)|*.image|All files (*.*)|*.*"),
                      wxFD_OPEN);
    if (open.ShowModal() == wxID_OK)
        set_profile_path_checked(m_text_propathl[1], open.GetPath());
}

// slot 2

void LisaConfigFrame::OnPickProFile2H(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog open(NULL, wxT("Select ProFile image for Slot 2 upper port:"),
                      wxEmptyString,
                      wxT("lisaem-profile-s2h.dc42"),
                      wxT("Disk image (*.dc42)|*.dc42|Raw image (*.image)|*.image|All files (*.*)|*.*"),
                      wxFD_OPEN);

    if (open.ShowModal() == wxID_OK)
        set_profile_path_checked(m_text_propathh[2], open.GetPath());
}

void LisaConfigFrame::OnPickProFile2L(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog open(NULL, wxT("Select ProFile image for Slot 2 lower port:"),
                      wxEmptyString,
                      wxT("lisaem-profile-s2l.dc42"),
                      wxT("Disk image (*.dc42)|*.dc42|Raw image (*.image)|*.image|All files (*.*)|*.*"),
                      wxFD_OPEN);

    if (open.ShowModal() == wxID_OK)
        set_profile_path_checked(m_text_propathl[2], open.GetPath());
}

// slot 3
void LisaConfigFrame::OnPickProFile3H(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog open(NULL, wxT("Select ProFile image for Slot 3 upper port:"),
                      wxEmptyString,
                      wxT("lisaem-profile-s3h.dc42"),
                      wxT("Disk image (*.dc42)|*.dc42|Raw image (*.image)|*.image|All files (*.*)|*.*"),
                      wxFD_OPEN);

    if (open.ShowModal() == wxID_OK)
        set_profile_path_checked(m_text_propathh[3], open.GetPath());
}

void LisaConfigFrame::OnPickProFile3L(wxCommandEvent &WXUNUSED(event))
{
    wxFileDialog open(NULL, wxT("Select ProFile image for Slot 3 lower port:"),
                      wxEmptyString,
                      wxT("lisaem-profile-s3l.dc42"),
                      wxT("Disk image (*.dc42)|*.dc42|Raw image (*.image)|*.image|All files (*.*)|*.*"),
                      wxFD_OPEN);

    if (open.ShowModal() == wxID_OK)
        set_profile_path_checked(m_text_propathl[3], open.GetPath());
}

void LisaConfigFrame::OnPickIWDir(wxCommandEvent &WXUNUSED(event))
{
    wxDirDialog dir(NULL, _T("Where should I save the print-out images?"));
    if (dir.ShowModal() == wxID_OK)
        iw_img_path->SetValue(dir.GetPath());
}

extern void invalidate_configframe(void);

LisaConfigFrame::~LisaConfigFrame()
{
    invalidate_configframe(); // prevent crash on reopening of preferences
}
