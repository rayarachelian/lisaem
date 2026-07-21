#ifndef __LISASKINH__
#define __LISASKINH__

#include <wx/wx.h>
#include <wx/fileconf.h>
#include "machine.h"

class LisaSkin
{
public:
    // box around the actual power button area  // 2587,1879 - 2673,1958 top left - 1724,1252 ->1782,834
    // 2572,1877
    int power_button_tl_x, //       = 1143,
        power_button_tl_y, //        = 836,
        power_button_br_x, //        = 1185,
        power_button_br_y, //        = 876,
        power_frame_left,  //         = 0,  /* 4th segment of skin */
        power_frame_top,   //          = 736,
        // floppy top left x,y (bottom right is calculated from the image)
        // floppy 1 only exists on Lisa 1
        floppy1_tl_x, //             = 1099,
        floppy1_tl_y, //             = 481,

        // floppy 2 is the only floppy on a Lisa 2 or XL
        floppy2_tl_x, //             = 1099,
        floppy2_tl_y, //             = 481,

        screen_origin_x, //          = 177, // was 205,175 // was 230,195 //was 140 not hidpi_scaled
        screen_origin_y, //          = 153,

        default_screen_origin_x, //  = 177, // was 205,175
        default_screen_origin_y, //  = 153,

        width_size,  //               = 1484,
        height_size; //              = 1026;

    wxString lisaface0name; //   = "lisaface0.png";
    wxString lisaface1name; //   = "lisaface1.png";
    wxString lisaface2name; //   = "lisaface2.png";
    wxString lisaface3name; //   = "lisaface3.png";

    // reserve for future
    wxString floppy1anim0; //    = "";
    wxString floppy1anim1; //    = "";
    wxString floppy1anim2; //    = "";
    wxString floppy1anim3; //    = "";
    wxString floppy1animN; //    = "";

    wxString floppy2anim0; //    = "floppy0.png";
    wxString floppy2anim1; //    = "floppy1.png";
    wxString floppy2anim2; //    = "floppy2.png";
    wxString floppy2anim3; //    = "floppy3.png";
    wxString floppy2animN; //    = "floppyN.png";

    wxString powerOn;  //         = "power_on.png";
    wxString powerOff; //        = "power_off.png";

    wxString floppy_eject;  //    = "floppy_eject.wav";
    wxString floppy_insert; //   = "floppy_insert_sound.wav";
    wxString floppy_insert_nopower;
    wxString floppy_motor1;       //   = "floppy_motor1.wav";
    wxString floppy_motor2;       //   = "floppy_motor2.wav";
    wxString poweroffclk;         //     = "poweroffclk.wav";
    wxString lisa_power_switch01; // = "lisa_power_switch01.wav";
    wxString lisa_power_switch02; // = "lisa_power_switch02.wav";

    void Load(wxFileConfig *config);
    void Save(wxFileConfig *config);
};

#endif
