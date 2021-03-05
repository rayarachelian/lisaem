/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.12.22                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2020 Ray A. Arachelian                          *
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
*                                                                                      *
*   Z8530 SCC terminal window (wxterm) interface functions for Lisa serial ports       *
*                                                                                      *
\**************************************************************************************/


#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <wx/log.h>
#include <terminalwx.h>
#include <wx/gdicmn.h>
#include <wx/menuitem.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>
#include <wx/string.h>
#include <wx/defs.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dir.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <wx/filedlg.h>
#include <wx/fileconf.h>
#include <wx/statusbr.h>
#include <wx/config.h>
#include <wx/clipbrd.h>
#include <wx/datetime.h>
#include <wx/stopwatch.h>
#include <wx/display.h>
#include <wx/gdicmn.h>
#include <wx/pen.h>
#include <wx/notebook.h>
#include <wx/stdpaths.h>
#include <wx/choicdlg.h>
#include <wx/utils.h> 
#include <wx/dnd.h>




#include <machine.h>
#include <fliflo_queue.hpp>
extern "C" {
    #include <z8530_structs.h>

    // from z8530-tty can reuse these for here for access to BLU
    void set_port_baud_tty(int port, int baud);
    void init_tty_serial_port(int port, char *config);
    int  poll_tty_serial_read(int port);
    int  read_serial_port_tty(int port);
    int  write_serial_port_tty(int port, uint8 data);
    void close_tty(int port);
}


#include <terminalinputevent.h>
#include <terminalwx_frame.h>
#include <terminalwx.h>
#include <wxterm.h>

extern "C" FLIFLO_QUEUE_t SCC_READ[16], SCC_WRITE[16]; //// if changing this also change in z8530.c!

#define CSTR(x) ((char *)(x.char_str()))
#define cSTR(x) ((char *)(x->char_str()))

// 2 motherboard serial ports, 3 slots x 4 port serial card, 1 for BLU, 1 spare
TerminalWx  *Terminal[16];
TerminalWxFrame *TerminalFrame[16];

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define ETB 0x17
#define CAN 0x18
#define SUB 0x1A
#define CRC 0x43


const char *BLULOADER="Lisa Simple Serial Loader\n"
"To bootstrap BLU on a Lisa if you don't have a floppy, do the following:\n"
"Usage:\n"
"  Connect this serial port to the Lisa's Serial Port B. Cablewise,\n"
"  The Lisa transmits on pin 2, receives on Pin 3, and pin 7 is ground.\n"
"  No other pins are needed/used by this routine, but if making a cable, you'll want to connect\n"
"  the handshake pins: Pin 6 is an input (to throttle Lisa transmit), pin 20 is an output (to \n"
"  indicate Lisa ready-to-receive). Since this utility runs at 57600 baud, you should use a\n"
"  shielded or short cable.\n"
"\n"
"Input the Loader into the Lisa's memory using Service Mode:\n"
"  Turn on Lisa, press the SPACE bar during the self test\n"
"  When you get to the 'startup from' menu, enter Service Mode by typing Apple-2 Apple-S\n"
"  Once In service mode, select 2 for 'Set Memory', for address type in 1900, then type in\n"
"  the words following for each of the 3 lines.input the loader using these 3 lines of keystrokes:\n"
"\n"
"  '2 Set Memory' address: 1900  data: 4DFA 0020 3456 227C 00FC D201 3C09 129E <return>\n"
"  '2 Set Memory' address: 1910  data: 66F4 0811 0000 67FA 14E9 0004 51CE FFF4 <return>\n"
"  '2 Set Memory' address: 1920  data: 4E75 09C0 0BD0 0444 0E01 03C1 05E8 0C00 <return>\n"
"\n"
"Note: Pressing '2' selects the 'set memory' function, so it won't appear on the screen,\n"
"      the '19x0' specifies the address, and the subsequent eight 4-letter words are the code.\n"
"      include the spaces between the words, lowercase letters are file.\n"
"Review the data entered above (so you can confirm it was entered correctly) using\n"
"    '1 Display memory' address: 1900 count: 30 <return>\n"
"Run the loader by selecting option '3 Call Program' at its address using\n"
"    1900 <return>\n"
"Once the loader is running (there's no indication it is, but if you did the above correctly, it will be),\n"
"start uploading the 'BLU v0.9.dc42' from this computer to the Lisa. (Upload your data file as plain text/binary,\n"
"NOT x-modem).\n";

/*
BLU is licensed for your use, free of charge by Sigma Seven Systems Ltd., with no warrantee of any kind. BLU may not work as described.
Using BLU may cause loss of data and have undesirable and undocumented effects. Use of BLU is at your own risk.
BLU is copyright 2013 by James MacPhail, portions copyright by Ray Arachelian. All rights are reserved.
*/

//    EVT_TIMER(ID_EMULATION_TIMER,LisaEmFrame::OnEmulationTimer)


const long TerminalWxFrame::ID_TERM         = wxNewId();
const long TerminalWxFrame::idMenuQuit      = wxNewId();
const long TerminalWxFrame::ID_STATUSBAR1   = wxNewId();
const long TerminalWxFrame::ID_FileCapture  = wxNewId();
const long TerminalWxFrame::ID_TextUpload   = wxNewId();
const long TerminalWxFrame::ID_XMODEM_UP    = wxNewId();
const long TerminalWxFrame::ID_XMODEM_DN    = wxNewId();


wxString getportname(int port)
{
    static char *name="unknown";
    switch(port) {
        case 0:  name="Serial B";         break;
        case 1:  name="Serial A";         break;    
        case 2:  name="slot 1 - port a";  break;
        case 3:  name="slot 1 - port b";  break;
        case 4:  name="slot 1 - port c";  break;
        case 5:  name="slot 1 - port d";  break;
        case 6:  name="slot 2 - port a";  break;
        case 7:  name="slot 2 - port b";  break;
        case 8:  name="slot 2 - port c";  break;
        case 9:  name="slot 2 - port d";  break;
        case 10: name="slot 3 - port a";  break;
        case 11: name="slot 3 - port b";  break;
        case 12: name="slot 3 - port c";  break;
        case 13: name="slot 3 - port d";  break;
        case 14: name="spare";            break;
        case 15: name="Terminal for BLU"; break;
    }
    return name;
}


TerminalWxFrame::TerminalWxFrame(wxWindow* parent,wxWindowID id, int port)
{
    //(*Initialize(TerminalWxFrame)
    FileMenu    = new wxMenu();
    editMenu    = new wxMenu;
    windowMenu  = new wxMenu;

    //MenuItem2 = new wxMenuItem( editMenu, wxID_CUT, wxString( wxT("Cut") ) , wxEmptyString, wxITEM_NORMAL );

    editMenu->Append(wxID_COPY, _("&Copy\tCtrl+C"));
    editMenu->Append(wxID_PASTE, _("&Paste\tCtrl+V"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_SELECTALL, _("Select A&ll\tCtrl+A"));

    Create(parent, id, getportname(port), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE, _T("id"));
    //Term1 = new TerminalWx(this,ID_TERM,port,wxPoint(72,56),80,24,_T("ID_TERM"));
    MenuBar1 = new wxMenuBar();


    //MenuItem1 = new wxMenuItem(Menu1, idMenuQuit, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
    FileMenu->AppendCheckItem(ID_FileCapture, _("Capture"), _("Capture Terminal output to a file") ) ;
    FileMenu->Append(ID_TextUpload, _("Upload Text"), _("Upload a text/ascii file") ) ;
    FileMenu->AppendSeparator();
    FileMenu->Append(ID_XMODEM_UP, _("Upload XModem"), _("Upload a file using XModem Protocol") );
    FileMenu->Append(ID_XMODEM_DN, _("Download XModem"), _("Download a file using XModem Protocol") );

    MenuBar1->Append(FileMenu,  _("&File"));
    MenuBar1->Append(editMenu,  _("&Edit"));
//  MenuBar1->Append(windowMenu,_("&Window"));  // ::TODO:: add code for this later

    //Menu2 = new wxMenu();
    //MenuBar1->Append(Menu2, _("Help"));
    SetMenuBar(MenuBar1);
    StatusBar1 = new wxStatusBar(this, ID_STATUSBAR1, 0, _T("ID_STATUSBAR1"));
    int __wxStatusBarWidths_1[1] = { -1 };
    int __wxStatusBarStyles_1[1] = { wxSB_NORMAL };
    StatusBar1->SetFieldsCount(1,__wxStatusBarWidths_1);
    StatusBar1->SetStatusStyles(1,__wxStatusBarStyles_1);
    SetStatusBar(StatusBar1);

    //Connect(idMenuQuit,wxEVT_COMMAND_MENU_SELECTED,(wxObjectEventFunction)&TerminalWxFrame::OnQuit);
    // should change this with minimize instead::
    Connect(wxID_ANY,wxEVT_CLOSE_WINDOW,(wxObjectEventFunction)&TerminalWxFrame::OnClose);
    //*)
}

TerminalWxFrame::~TerminalWxFrame()
{
    //(*Destroy(TerminalWxFrame)
    //*)
}

void TerminalWxFrame::OnQuit(wxCommandEvent& event)
{
    Close();
}

// maybe replace this with minimize instead incase the user closes the window by accident.
void TerminalWxFrame::OnClose(wxCloseEvent& event)
{
    int port=this->portnum;
    if (Terminal[port]) {Terminal[port]->Destroy(); Terminal[port]=NULL;}

    TerminalFrame[port]=NULL;
    Destroy();
}


/** Constructor for the terminal widget.   @param width The width of the terminal in characters    @param height The height of the terminal in character
    @param name   The terminal name, which is not displayed but stored internally, and might be changed by VT commands. */


TerminalWx::TerminalWx(wxWindow* parent, wxWindowID id, int port,
                                const wxPoint& pos, int width, int height,
                                const wxString& name):  wxTerm(parent,id,pos,width,height,name) {
    portnum=port;
}


extern "C" void init_terminal_serial_port(int port) {
    wxString name="";
    name << getportname(port);

    if  (port<2) {
        scc_r[port].s.rr0.r.tx_buffer_empty=1;
        scc_r[port].s.rr0.r.dcd=1;
        scc_r[port].s.rr0.r.cts=1;
    }
    #ifdef DEBUG
    fprintf(stderr,"\n\n\n\n********* init_terminal_serial_port: %d ***********\n\n\n\n",port);
    #endif

    if  (!TerminalFrame[port])
        {
            TerminalFrame[port]=new TerminalWxFrame(NULL,wxID_ANY, port);
            #ifdef DEBUG
            fprintf(stderr,"Created new TerminalWxFrame for port %d\n",port);
            #endif
        }

    if  (!Terminal[port])
        {
            Terminal[port]=new TerminalWx(TerminalFrame[port], wxID_ANY, port, wxDefaultPosition, 80, 25, (const wxString) name);
            #ifdef DEBUG
            fprintf(stderr,"Created new TerminalWindow for port %d\n",port);
            #endif
        }

        TerminalFrame[port]->Show();
        //SetTopWindow(TerminalFrame[port]);
}

/**    Called whenever the user has input - this resulted in sending garbage to the port, use SendBack instead */
//void TerminalWx::OnUserInput(wxString input) {
    //wxString in=input;
    //int port=this->portnum;
    //char *c=CSTR(in);
    //
    //fprintf(stderr,"\n UserInput: ::%s::\n",c);
    //
    //while (*c && !fliflo_buff_is_full(&SCC_READ[port])) 
    //      { fliflo_buff_add(&SCC_READ[port], *c); 
    //        fprintf(stderr,"added character:%c to fliflo for port:%d\n",c,port);
    //        c++; }
//}

extern "C" void rx_char_available(int port);

void TerminalWx::SendBack(int len, char* data) { 

    #ifdef DEBUG
    fprintf(stderr,"\nSendBackLength: %d\n",len);
    #endif

    for (int i=0; i<len; i++)
        if (data[i] && !fliflo_buff_is_full(&SCC_READ[this->portnum]))
        {
            fliflo_buff_add(&SCC_READ[this->portnum], data[i]);
            rx_char_available(this->portnum);
            #ifdef DEBUG
            fprintf(stderr,"SendBack: added i=%d character:%c (%d %02x) to fliflo for port:%d\n\n",i, data[i], data[i], data[i], this->portnum);
            #endif
        }

        #ifdef DEBUG
        if (!fliflo_buff_has_data(&SCC_READ[this->portnum]))
            fprintf(stderr,"*** FLIFLO BUFF HAS NO DATA FOR PORT %d WTF!\n",this->portnum);
        if (fliflo_buff_is_full(&SCC_READ[this->portnum]))
            fprintf(stderr,"*** FLIFLO BUFF IS FULL FOR PORT %d WTF!\n",this->portnum);
        #endif
}

void TerminalWx::SendBack(char* data) { 
    for (int i=0; data[i]!=0; i++)
        if (data[i] && !fliflo_buff_is_full(&SCC_READ[this->portnum]))
        {
            fprintf(stderr,"\nSendBack: char\n");
            fliflo_buff_add(&SCC_READ[this->portnum], data[i]);
            rx_char_available(this->portnum);
            #ifdef DEBUG
            fprintf(stderr,"SendBack: added i=%d character:%c (%d %02x) to fliflo for port:%d\n\n",i, data[i], data[i], data[i], this->portnum);
            #endif
        }

        #ifdef DEBUG
        if (!fliflo_buff_has_data(&SCC_READ[this->portnum]))
            fprintf(stderr,"*** FLIFLO BUFF HAS NO DATA FOR PORT %d WTF!\n",this->portnum);
        if (fliflo_buff_is_full(&SCC_READ[this->portnum]))
            fprintf(stderr,"*** FLIFLO BUFF IS FULL FOR PORT %d WTF!\n",this->portnum);
        #endif
    }

void TerminalWxFrame::OnPaste(wxCommandEvent& WXUNUSED(event))
{
    wxTextDataObject data;
    if  (xferproto) {wxMessageBox(_("Cannot paste as file transfer operation is in progress"),_T("Cannot Paste"), wxICON_INFORMATION | wxOK); return;}

    if  (wxTheClipboard->Open()) 
        {
            if (wxTheClipboard->IsSupported(wxDF_TEXT))  wxTheClipboard->GetData(data);
            wxTheClipboard->Close();
            wxTheClipboard->UsePrimarySelection();
            if (wxTheClipboard->IsSupported(wxDF_TEXT)) 
            {
                wxString wspaste_to_terminal;
                int len;
                char *d;
                wspaste_to_terminal = (wxString)(data.GetText());
                d=CSTR(wspaste_to_terminal);
                for (int i=0; d[i]!=0; i++)
                    if  (d[i] && !fliflo_buff_is_full(&SCC_READ[this->portnum]))
                        { fliflo_buff_add(&SCC_READ[this->portnum], d[i]);     }

                rx_char_available(this->portnum);
            }
        }
}

/* CRC used in BLU
CRC16:      MOVE.W D0,D2            ; swap16 D0 (CRC) - wonder if ROL.W #8,D2 would work better?

			LSR.W #8,D2
			LSL.W #8,D0
			CLR.B D0
            OR.B D2,D0

            EOR.W D1,D0             ; CRC=SWAP16(CRC)^DATA
            CLR.W D2                ; CRC=CRC^((CRC & 0x00f0)>>4)

			MOVE.B D0,D2
			LSR.B #4,D2
			AND.W #$00F0,D2
			EOR.W D2,D0

            MOVE.W D0,D2            ; crc^=(crc<<12);
			LSL.W #12,D2
			AND.W #$0FF0,D2
			EOR.W D2,D0

            MOVE.W D0,D2            ; return crc^(((crc & 0x00ff)<<4)<<1);
			AND.W #$00ff,D2
			LSL.W #4,D2
			LSL.W #1,D2
			EOR.W D0,D2

			RTS
*/
uint16 xmcrc16(uint8 *ptr, int count)
{
    uint16  crc;
    int i,j;

    crc = 0;
    for (i=0; i<count; i++)
    {
        crc = crc ^ ptr[i] << 8;

        for (j=0; j<8; j++)
           { crc= (crc & 0x8000) ? (crc << 1 ^ 0x1021) : (crc << 1); }
    }
    return crc;
}

uint8 xmchksum (uint8 *ptr, int count)
{
    uint8 sum=0;
    for (int i=0; i<count; i++) 
        sum=(sum+ptr[i]) & 255;
    return sum;
}

/*


Byte 1  | Byte 2 | Byte 3    |Bytes 4-131 | Bytes 132-133
SOH     | packet#|255-packet#|packet data | CRC 16 bit

The following defines are used for protocol flow control.

Symbol | Description        | Value
SOH    | Start of Header    | 0x01
EOT    | End of Transmission| 0x04
ACK    | Acknowledge        | 0x06
NAK    | Not Acknowledge    | 0x15
ETB    | End of Transmission Block (Return to Amulet OS mode) | 0x17
CAN    | Cancel (Force receiver to start sending C's) | 0x18
C      | ASCII “C”          |0x43

Byte 1 of the XmodemCRC packet can only have a value of SOH, EOT, CAN or ETB anything else is an error. Bytes 2 and 3 form a packet number with checksum, add the two 
bytes together and they should always equal 0xff. Please note that the packet number starts out at 1 and rolls over to 0 if there are more than 255 packets to be 
received. Bytes 4 - 131 form the data packet and can be anything. Bytes 132 and 133 form the 16-bit CRC. The high byte of the CRC is located in byte 132. The CRC is 
calculated only on the data packet bytes (4 - 131) .

Synchronization
The receiver starts by sending an ASCII “C” (0x43) character to the sender indicating it wishes to use the CRC method of block validating. After sending the 
initial “C” the receiver waits for either a 3 second time out or until a buffer full flag is set. If the receiver is timed out then another “C” is sent to the 
sender and the 3 second time out starts again. This process continues until the receiver receives a complete 133-byte packet. 


Receiver Considerations
This protocol NAKs the following conditions: 1. Framing error on any byte 2. Overrun error on any byte 3. Duplicate packet 4. CRC error 5. Receiver
timed out (didn't receive packet within 1 second) On any NAK, the sender will re-transmit the last packet. Items 1 and 2 should be considered serious
hardware failures. Verify that sender and receiver are using the samebaud rate, start bits and stop bits. Item 3 is usually the sender getting an ACK garbled
and re-transmitting the packet. Item 4 is found in noisy environments. And the last issue should be self-correcting after the receiver NAKs the sender.


Mixed 1k+128byte blocks
SENDER                                      RECEIVER

                                        <-- C
STX 01 FE Data[1024] CRC CRC            -->
                                        <-- ACK
STX 02 FD Data[1024] CRC CRC            -->
                                        <-- ACK
SOH 03 FC Data[128] CRC CRC             -->
                                        <-- ACK
SOH 04 FB Data[100] CPMEOF[28] CRC CRC  -->
                                        <-- ACK
EOT                                     -->
                                        <-- ACK
size_t   xferPosition;      // file position ftell/fseek
long int xferBlockNum;      // block number for block oriented protocols
long int xferWindowSize;    // for flexible sized protocols
long int xferRetryCount;    // current retry count
long int xferLastTimeStamp; // timestamp for timeouts
long int xferMode1;         // extra vars for each protocol
long int xferMode2;
*/

//0        1   2   3-131    132 133
//SOH|STX/BLK/!BLK/[128data]CRC CRC


// Macros for use with XModem - port 15 is really a USB/Serial port (real tty) for BLU, otherwise, we use fliflo to enqueue data
// to emulated ports on LisaEm.
#define XREADPORT()        ( (portnum==15) ? read_serial_port_tty(portnum)       : fliflo_buff_get(&SCC_READ[portnum])      )
#define XWRITEPORT(data)   ( (portnum==15) ? write_serial_port_tty(portnum,data) : fliflo_buff_add(&SCC_WRITE[portnum],data))

void TerminalWxFrame::XModemReceiveBlock(void)
{
    size_t count, size;
    uint8 buffer[4096+5];
    uint8 *data=&(buffer[4]);
    uint16 crc;
    uint8 chksum;
    int buff_size=fliflo_buff_size(&SCC_READ[portnum]);

    // ensure we have enough bytes, use xferMode2 byte as a retry count.
    if (buff_size>3 && buff_size<(131)) {
        xferMode2++; if (xferMode2<3) return;
        xferMode2=0;
        write_serial_port_tty(portnum,NAK); return;
    }

    xferMode2=0;
    for (int i=0; i<buff_size; i++) {
        int c=XREADPORT();
        if (c>-1) buffer[i]=c;
        else { XWRITEPORT(NAK); 
        
        if (i==0 && c==EOT) {
            fclose(download);
            xferproto=0;
            return;
        }
        
        }
    }
    if (buffer[0]!=(255-buffer[1]) || buffer[0]!=xferBlockNum) {XWRITEPORT(NAK); return;}

    crc=xmcrc16(&buffer[2],xferWindowSize);
    chksum=xmchksum((uint8 *)&buffer[2],xferWindowSize);

    if (crc==((buffer[132]<<8)|(buffer[133])) || (buffer[132]==chksum) ) 
    { 
        xferBlockNum=(xferBlockNum+1) & 255;
        fseek(download,xferPosition,SEEK_SET);
        fwrite(&buffer[2],xferWindowSize,1,download);
        xferPosition+=xferWindowSize;
        XWRITEPORT(ACK);
        return;
    }    
    XWRITEPORT(NAK);
}


void TerminalWxFrame::XModemSendBlock(void)
{
    size_t count, size;
    uint8 buffer[4096+5];
    uint8 *data=&(buffer[4]);
    uint8 checksum=0;
    int c;

    if (!upload || feof(upload)) {
        if (xferMode1!=2) XWRITEPORT(EOT);
        else {XWRITEPORT(13); XWRITEPORT(10);}// end is sending one EOT, then ACK, then EOB // EOB=13,10
        
        if (xferMode1<2) xferMode1=2;
        size=1;
    }
    else
    {
        memset(buffer,26,4096+4); // 26 is used for padding when last block is smaller than a block
        buffer[0]=SOH; //xferMode1 ? STX:SOH;
        buffer[1]=(uint8) xferBlockNum;
        buffer[2]=(uint8) (255-xferBlockNum);
        fseek(upload,xferPosition,SEEK_SET);
        count=fread(data,xferWindowSize,1,upload);
        size=3+xferWindowSize;
        
        c=XREADPORT(); if (c=='C') xferMode1=1;
        
        if  (xferMode1) 
            {
                checksum=xmcrc16(data,xferWindowSize);  
                data[xferWindowSize]=(uint8)((checksum & 0xff)>>8); data[xferWindowSize+1]=(uint8)((checksum & 0x00ff));
                size++;
            }
        else
            {
                checksum=xmchksum(data,xferWindowSize); 
                data[xferWindowSize]=(uint8)((checksum & 0xff)   );
            }
    }
    for (int i=0; i<size; i++) XWRITEPORT(buffer[i]);
}


void TerminalWxFrame::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    // 0=terminal no protocol, 1=ascii upload, 2=xmodem download, 3=xmodem download.
    // 16,32,64,128= state flags
    switch(xferproto & 15) {
//      case 0: ; break; // normal terminal - disable timer - fall through to default

        case 1: ; break; // ascii upload

        case 2: ; break; // xmodem download

        case 3: ; break; // xmodem upload

        default:         // disable xfer timer
                    xferproto=0;
                    XferTimer->Stop();
                    delete XferTimer;
                    XferTimer=NULL;
                    return;
    }
}

void TerminalWxFrame::OnXModemDownload(wxCommandEvent& WXUNUSED(event))
{
        if (xferproto) {wxMessageBox(_("Cannot start XModem as another file transfer operation is in progress"),_("Cannot Download with XModem now"), wxICON_INFORMATION | wxOK); return;}

        wxString openfile;
        wxFileDialog open(this,         wxT("Download File As:"), wxEmptyString, wxEmptyString,
                                        "BLU files (*.dc42;*.blu;*.rom;*.bin)|All (*.*)|*.*)",
                                        (long int)wxFD_SAVE,wxDefaultPosition);
        if  (open.ShowModal()==wxID_OK)  {
            wxString filename=open.GetPath();
            download=fopen(CSTR(filename),"rw");
            if (!download) {wxMessageBox(_("Could not create file for download"),_("Cannot Download"), wxICON_INFORMATION | wxOK); return;}

            xferproto=2;
            delete XferTimer;
            XferTimer = new wxTimer(this,60+this->portnum);
            XferTimer->Start(10,true);
        }
}

void TerminalWxFrame::OnTextUpload(wxCommandEvent& WXUNUSED(event))
{
        if (xferproto) {wxMessageBox(_("Cannot start text upload as another file transfer operation is in progress"),_("Cannot Upload now"), wxICON_INFORMATION | wxOK); return;}

        wxString openfile;
        wxFileDialog open(this, wxT("Upload Text File:"), wxEmptyString, wxEmptyString,
                                "Text files (*.txt;*.text;*.TXT)|All (*.*)|*.*)",
                                (long int)wxFD_OPEN,wxDefaultPosition);

        if  (open.ShowModal()==wxID_OK)  {
            wxString filename=open.GetPath();
            upload=fopen(CSTR(filename),"rb");
            if (!upload) {wxMessageBox(_("Could not open file for upload"),_("Cannot Upload"), wxICON_INFORMATION | wxOK); return;}

            xferproto=1;
            delete XferTimer;
            XferTimer = new wxTimer(this,60+this->portnum);
            XferTimer->Start(10,true);
        }
}

void TerminalWxFrame::OnXmodemUpload(wxCommandEvent& event)
{
        if (xferproto) {wxMessageBox(_("Cannot start XModem upload as another file transfer operation is in progress"),_("Cannot Upload now"), wxICON_INFORMATION | wxOK); return;}

        wxString openfile;
        wxFileDialog open(this, wxT("Upload File with XModem:"), wxEmptyString, wxEmptyString,
                                "BLU files (*.dc42;*.blu;*.bin;*.image;*.Image)|All (*.*)|*.*)",
                                (long int)wxFD_OPEN,wxDefaultPosition);

        if  (open.ShowModal()==wxID_OK)  {
            wxString filename=open.GetPath();
            upload=fopen(CSTR(filename),"rb");
            if (!upload) {wxMessageBox(_("Could not open file for upload"),_("Cannot Upload"), wxICON_INFORMATION | wxOK); return;}

            xferproto=3;
            delete XferTimer;
            XferTimer = new wxTimer(this,60+this->portnum);
            XferTimer->Start(10,true);
        }
}

void TerminalWxFrame::OnFileCapture(wxCommandEvent& WXUNUSED(event))
{

    if (capture) {
        fclose(capture);
        FileMenu->Check(ID_FileCapture, false);
        return;
    }

    wxString openfile;
    wxFileDialog open(this, wxT("Capture text as:"), wxEmptyString, wxEmptyString,
                            "Text files (*.txt;*.ascii;*.text;*.ansi)|All (*.*)|*.*)",
                            (long int)wxFD_SAVE,wxDefaultPosition);
    if (open.ShowModal()==wxID_OK) {
        wxString filename=open.GetPath();
        capture=fopen(CSTR(filename),"rw");
        if (!capture) {
            wxMessageBox(_T("Couldn't create the file for writing"),
            _T("Error creating/opening file"), wxICON_INFORMATION | wxOK);
        }
        else
            FileMenu->Check(ID_FileCapture, true);
    }
}

void TerminalWxFrame::OnCopy(wxCommandEvent& event)
{
    wxString text=Terminal[portnum]->GetSelection();
    if(wxTheClipboard->Open())
	{
		wxTheClipboard->Clear();
		wxTheClipboard->SetData( new wxTextDataObject( text ) );
		wxTheClipboard->Flush();
		wxTheClipboard->Close();      
	}
    event.Skip();
}



// Processes characters sent from the backend This function is thread safe and can be called from any thread at any time
void TerminalWx::DisplayChars(const wxString& str) {this->QueueEvent(new TerminalInputEvent(str)); }

// Processes characters sent from the backend This function is not thread safe and can *only* safely be called from the main event loop
void TerminalWx::DisplayCharsUnsafe(const wxString& str) 
{
    #ifdef DEBUG
    fprintf(stderr,"Received from Lisa :::: %s :::\n",CSTR(str)); 
    #endif
    ProcessInput(str.length(),(unsigned char*)const_cast<char*>((const char*)str.mb_str()));  
}

void TerminalWx::OnTerminalInput(TerminalInputEvent& evt) { DisplayCharsUnsafe(evt.GetString()); }


extern "C" void write_serial_port_terminal(int portnum, uint8 data) {
    wxString s=_("");

    char *lastchars=TerminalFrame[portnum]->lastchars;

    if (data==19 || data==17) return; // avoid sending xon/xoff to display or capture

    s<< (char)(data);
    Terminal[portnum]->DisplayCharsUnsafe(s);

    // if capture is turned on record to output file, but strip off ANSI/vt1xx/xterm sequences
    // not perfect, but it should do
    if  (TerminalFrame[portnum]->capture) { // http://ascii-table.com/ansi-escape-sequences-vt-100.php
        // ANSI escape codes are ESC[number;number;number Letter and also ESC= and ESC> (keypad modes)
        // need to add <  ESCOP to ESCOS (P,Q,R,S)=PF1..PF4 and ESCOp-z (numpad)
        if (data==27) {lastchars[0]=27; lastchars[1]=0; return;}
        if (lastchars[0]==27 && data=='[' && lastchars[1]==0) {lastchars[1]='['; return;}
        if (lastchars[0]==27 && lastchars[1]==0 && data=='=' || data=='>') {lastchars[0]=0; return;}
        if (lastchars[0]==27 && lastchars[1]=='[')
        {
            if (data==';' || (data>='0' && data<='9') ) return; // can have lots of numbers separated by semicolons, until we receive a letter or other symbol
        }
        lastchars[0]=0;
        #ifdef __MSVCRT__
        fputc(data,capture);             // on windows append LF to CR
        if (data=13) fputc(10,capture);
        #else
        if (data==13) data=10;                            // on unixy systems,replace CR with NL
        fputc(data,TerminalFrame[portnum]->capture);
        #endif
    }
}

extern "C" char read_serial_port_terminal(int port) {
    if  (fliflo_buff_has_data(&SCC_READ[port]))
        {
            #ifdef DEBUG
            fprintf(stderr,"read_serial_port_terminal port:%d has data, returning character\n");
            #endif
            rx_char_available(port); 
            return (int)fliflo_buff_get(&SCC_READ[port]);
         }
    return -1;
}

extern "C" void close_terminalwx(int port) {
    if (port<0 || port>16)         return;
    if (Terminal[port]!=NULL)      {Terminal[port]->Destroy();  Terminal[port]=NULL; }
    if (TerminalFrame[port]!=NULL) {TerminalFrame[port]->Destroy(); TerminalFrame[port]=NULL; }
}

// used at shutdown to prevent segfaults
extern "C" void close_all_terminalwx(void) { for  (int i; i<16; i++) close_terminalwx(i); }


TerminalWx::~TerminalWx()  { 
//    //dtor 
}

/*
wxBEGIN_EVENT_TABLE(TerminalWxFrame, wxTerm)
EVT_TERMINAL_INPUT(TerminalWx::OnTerminalInput)
EVT_MENU(wxID_CUT, wxTextCtrl::OnCut)
EVT_MENU(wxID_COPY, wxTextCtrl::OnCopy)
EVT_MENU(wxID_PASTE, OnPaste)
EVT_MENU(wxID_SELECTALL, wxTerm::SelectAll)	
EVT_MENU(ID_FileCapture,OnFileCapture)
EVT_MENU(ID_TextUpload, OnTextUpload)
EVT_MENU(ID_XMODEM_UP,OnXmodemUpload)
EVT_MENU(ID_XMODEM_DN,OnXmodemDownload)
EVT_TIMER(ID_Timer,OnTimer)
wxEND_EVENT_TABLE()
*/
//BEGIN_EVENT_TABLE(TerminalWxFrame,wxFrame)
    //(*EventTable(TerminalWxFrame)
    //*)
//END_EVENT_TABLE()
