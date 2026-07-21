#ifndef __LISACONFIGH__
#define __LISACONFIGH__

#include <wx/wx.h>
#include <wx/fileconf.h>

#include "machine.h"

// v- switched to magic serial number
#define LISA_CONFIG_DEFAULTSERIAL "ff000000000000ff0000000000000000"
//                                "ff028308104050ff0010163504700000" - original S/N for LisaEm from one of my original two Lisas
//                                 012345678901234567890123 5678901
//                                           11111111112222 222

#define ROMVER (0x18) // ROM Version

class LisaConfig
{
public:
    wxString slot1, slot2, slot3,
        s1l, s1h, s2l, s2h, s3l, s3h,
        s1lp, s1hp, s2lp, s2hp, s3lp, s3hp;

    wxString parallel, parallelp;

    int mymaxlisaram;
    unsigned long kbid;
    unsigned long iorom;

    wxString kbidstr;
    wxString rompath;
    wxString dualrom;
    wxString myserial;
    wxString serial1_setting;
    wxString serial1_param;
    wxString serial2_setting;
    wxString serial2_param;
    wxString ioromstr;
    wxString serial1xon, serial2xon;

    wxString iw_png_path; // path to directory to store printouts if png is on
    int iw_png_on;        // do we save printouts as PNG's or to the host printer?
    int iw_dipsw_1;       // dip switch for imagewriter

    long int saw_3a_warning;

    /* XXX Requiring floppy_ram like this is ugly. A temporary measure, I hope. */
    void Load(wxFileConfig *config, uint8 *floppy_ram);
    void Save(wxFileConfig *config, uint8 *floppy_ram);
};
#endif
