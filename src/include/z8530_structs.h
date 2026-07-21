/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.2.7      DEV 2020.09.30                   *
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
*              Zilog 8530 SCC Data Definitions and Structures for                      *
*                            Lisa serial ports                                         *
\**************************************************************************************/

#ifndef GOT_Z8530_REGS_H
#define GOT_Z8530_REGS_H 1

#define SCC_NOTHING 0
#define SCC_LOCALPORT 11
#define SCC_IMAGEWRITER 12
#define SCC_LOOPBACKPLUG 13
#define SCC_FILE 14 // same as localport???
#define SCC_PIPE 15
#define SCC_NAMED_PIPE 16 // not yet implemented
#define SCC_TELNETD 17
#define SCC_TELNETTO 18 // not yet implemented
#define SCC_IMAGEWRITER_PS 19
#define SCC_IMAGEWRITER_PCL 20
#define SCC_PTY 21      // pseudo terminal
#define SCC_TTY 22      // physical serial port
#define SCC_TERMINAL 23 // terminal window
#define SCC_SHELL 24    // shell

#define SCC_BUFFER_SIZE 32768

#define SERIAL_PORT_A_DATA 0xFCD247
#define SERIAL_PORT_A_CONTROL 0xFCD243
#define SERIAL_PORT_B_DATA 0xFCD245
#define SERIAL_PORT_B_CONTROL 0xFCD241

#define SCC_B_CONTROL 0
#define SCC_B_DATA 1
#define SCC_A_CONTROL 2
#define SCC_A_DATA 3

#define SCC_IRQ_B_EXT 1
#define SCC_IRQ_B_TX 2
#define SCC_IRQ_B_RX 4

#define SCC_IRQ_A_EXT 8
#define SCC_IRQ_A_TX 16
#define SCC_IRQ_A_RX 32

// These assume low bit is 1st, and will use reversebit otherwise.
typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8 reg : 3,
        cmd : 3,
        reset : 2;
#else
    uint8
        reset : 2,
        cmd : 3,
        reg : 3;

#endif

  } r;
  uint8 v;
} wr0_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        extintenable : 1,     // 0
        txintenable : 1,      // 1
        parityspecial : 1,    // 2
        rxintmode : 2,        // 34
        waitdmareqon : 1,     // 5
        waitdmareqfn : 1,     // 6
        waitdmareqenable : 1; // 7
#else
    uint8
        waitdmareqenable : 1, // 7
        waitdmareqfn : 1,     // 6
        waitdmareqon : 1,     // 5
        rxintmode : 2,        // 34
        parityspecial : 1,    // 2
        txintenable : 1,      // 1
        extintenable : 1;     // 0

#endif

  } r;
  uint8 v;
} wr1_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        v0 : 1,
        v1 : 1,
        v2 : 1,
        v3 : 1,
        v4 : 1,
        v5 : 1,
        v6 : 1,
        v7 : 1;
#else
    uint8
        v7 : 1,
        v6 : 1,
        v5 : 1,
        v4 : 1,
        v3 : 1,
        v2 : 1,
        v1 : 1,
        v0 : 1;

#endif
  } r;
  uint8 v;
} wr2_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8 rxenable : 1,
        syncloadinhibit : 1,    // 1 10 0 0 0 0 1
        sldcaddrsearchmode : 1, // 11000001
        rxcrcenable : 1,
        enterhuntmode : 1,
        autoenables : 1,
        rxbitsperchar : 2;
#else
    uint8
        rxbitsperchar : 2,
        autoenables : 1,
        enterhuntmode : 1,
        rxcrcenable : 1,
        sldcaddrsearchmode : 1, // 11000001
        syncloadinhibit : 1,    // 1 10 0 0 0 0 1
        rxenable : 1;
#endif
  } r;
  uint8 v;
} wr3_t;

typedef union
{
  struct
  { //   16x
#ifndef BYTES_HIGHFIRST
    uint8
        parityenable : 1, // 4d=01  00 11 01 == x16 clk, 2 stop bits, odd parity
        evenparity : 1,   // want=00  00 01 00 =4
        stopbits : 2,
        synchcharsize : 2,
        clockmultipliermode : 2;
#else
    uint8
        clockmultipliermode : 2,
        synchcharsize : 2,
        stopbits : 2,
        evenparity : 1,   // want=00  00 01 00 =4
        parityenable : 1; // 4d=01  00 11 01 == x16 clk, 2 stop bits, odd parity
#endif
  } r;
  uint8 v;
} wr4_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        txcrcenable : 1,   // 1  // 01101000
        RTS : 1,           // 2
        sldc_crc16 : 1,    // 4
        txenable : 1,      // 8
        sendbreak : 1,     // 16
        txbitsperchar : 2, // 32+64
        DTR : 1;           // 128
#else
    uint8
        DTR : 1,           // 128
        txbitsperchar : 2, // 32+64
        sendbreak : 1,     // 16
        txenable : 1,      // 8
        sldc_crc16 : 1,    // 4
        RTS : 1,           // 2
        txcrcenable : 1;   // 1  // 01101000
#endif

  } r;
  uint8 v;
} wr5_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        sync0 : 1,
        sync1 : 1,
        sync2 : 1,
        sync3 : 1,
        sync4 : 1,
        sync5 : 1,
        sync6 : 1,
        sync7 : 1;
#else
    uint8
        sync7 : 1,
        sync6 : 1,
        sync5 : 1,
        sync4 : 1,
        sync3 : 1,
        sync2 : 1,
        sync1 : 1,
        sync0 : 1;

#endif
  } r_monosync8;

  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        sync0 : 1,
        sync1 : 1,
        sync2 : 1,
        sync3 : 1,
        sync4 : 1,
        sync5 : 1,
        sync0a : 1,
        sync1a : 1;
#else
    uint8
        sync1a : 1,
        sync0a : 1,
        sync5 : 1,
        sync4 : 1,
        sync3 : 1,
        sync2 : 1,
        sync1 : 1,
        sync0 : 1;

#endif
  } r_monosync6;
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        sync0 : 1,
        sync1 : 1,
        sync2 : 1,
        sync3 : 1,
        sync4 : 1,
        sync5 : 1,
        sync6 : 1,
        sync7 : 1;
#else
    uint8
        sync7 : 1,
        sync6 : 1,
        sync5 : 1,
        sync4 : 1,
        sync3 : 1,
        sync2 : 1,
        sync1 : 1,
        sync0 : 1;

#endif
  } r_bisync16;
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8 : 4, // these are all 1 bits
        sync1 : 1,
        sync2 : 1,
        sync3 : 1,
        sync4 : 1;
#else
    uint8
        sync4 : 1,
        sync3 : 1,
        sync2 : 1,
        sync1 : 1,
        : 4; // these are all 1 bits
#endif
  } r_bisync12;

  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        adr0 : 1,
        adr1 : 1,
        adr2 : 1,
        adr3 : 1,
        adr4 : 1,
        adr5 : 1,
        adr6 : 1,
        adr7 : 1;
#else
    uint8
        adr7 : 1,
        adr6 : 1,
        adr5 : 1,
        adr4 : 1,
        adr3 : 1,
        adr2 : 1,
        adr1 : 1,
        adr0 : 1;
#endif
  } r_sldc;
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8 : 4,
        adr4 : 1,
        adr5 : 1,
        adr6 : 1,
        adr7 : 1;
#else
    uint8
        adr7 : 1,
        adr6 : 1,
        adr5 : 1,
        adr4 : 1, : 4;

#endif
  } r_sldc_addr_range;
  uint8 v;
} wr6_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        sync0 : 1,
        sync1 : 1,
        sync2 : 1,
        sync3 : 1,
        sync4 : 1,
        sync5 : 1,
        sync6 : 1,
        sync7 : 1;
#else
    uint8
        sync7 : 1,
        sync6 : 1,
        sync5 : 1,
        sync4 : 1,
        sync3 : 1,
        sync2 : 1,
        sync1 : 1,
        sync0 : 1;

#endif
  } r_monosync8;
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8 : 2,
        sync0 : 1,
        sync1 : 1,
        sync2 : 1,
        sync3 : 1,
        sync4 : 1,
        sync5 : 1;
#else
    uint8
        sync5 : 1,
        sync4 : 1,
        sync3 : 1,
        sync2 : 1,
        sync1 : 1,
        sync0 : 1, : 2;

#endif
  } r_monosync6;
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        sync8 : 1,
        sync9 : 1,
        sync10 : 1,
        sync11 : 1,
        sync12 : 1,
        sync13 : 1,
        sync14 : 1,
        sync15 : 1;
#else
    uint8
        sync15 : 1,
        sync14 : 1,
        sync13 : 1,
        sync12 : 1,
        sync11 : 1,
        sync10 : 1,
        sync9 : 1,
        sync8 : 1;

#endif
  } r_bisync16;
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        sync4 : 1,
        sync5 : 1,
        sync6 : 1,
        sync7 : 1,
        sync8 : 1,
        sync9 : 1,
        sync10 : 1,
        sync11 : 1;
#else
    uint8
        sync11 : 1,
        sync10 : 1,
        sync9 : 1,
        sync8 : 1,
        sync7 : 1,
        sync6 : 1,
        sync5 : 1,
        sync4 : 1;

#endif
  } r_bisync12;
  // sdlc is 01111110
  uint8 v;
} wr7_t;

typedef union
{
  uint8 txbuffer;
  uint8 v;
} wr8_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        VIS : 1,          // bit0
        NV : 1,           // bit1
        DLC : 1,          // bit2
        MIE : 1,          // bit3
        status_hi_lo : 1, // controls R22B selection
        soft_int_ack : 1,
        reset : 2;
#else
    uint8
        reset : 2,
        soft_int_ack : 1,
        status_hi_lo : 1, // controls R22B selection
        MIE : 1,          // bit3
        DLC : 1,          // bit2
        NV : 1,           // bit1
        VIS : 1;          // bit0

#endif
  } r;
  uint8 v;
} wr9_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        six_eight_bit_sync : 1,
        loopbackmode : 1,
        abort_flag_on_underrun : 1,
        sdlc_mark_flag_idle : 1,
        go_active_on_poll : 1,
        data_encoding : 2,
        crc_preset_i_o : 1;
#else
    uint8
        crc_preset_i_o : 1,
        data_encoding : 2,
        go_active_on_poll : 1,
        sdlc_mark_flag_idle : 1,
        abort_flag_on_underrun : 1,
        loopbackmode : 1,
        six_eight_bit_sync : 1;
#endif
  } r;
  uint8 v;
} wr10_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        xmit_extern_control : 2, // $50=0 10 10 0 00
        trxc_pin_is_output : 1,
        xmit_clock_src : 2,
        rx_clock_src : 2,
        rtxc_xtal : 1;
#else
    uint8
        rtxc_xtal : 1,
        rx_clock_src : 2,
        xmit_clock_src : 2,
        trxc_pin_is_output : 1,
        xmit_extern_control : 2; // $50=0 10 10 0 00
#endif
  } r;
  uint8 v;
} wr11_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        tc0 : 1, // low byte of time constant baud generator
        tc1 : 1,
        tc2 : 1,
        tc3 : 1,
        tc4 : 1,
        tc5 : 1,
        tc6 : 1,
        tc7 : 1;
#else
    uint8
        tc7 : 1, // low byte of time constant baud generator
        tc6 : 1,
        tc5 : 1,
        tc4 : 1,
        tc3 : 1,
        tc2 : 1,
        tc1 : 1,
        tc0 : 1;
#endif
  } r;
  uint8 v;
} wr12_t;

typedef union
{
  struct
  { // high byte of time constant baud generator
#ifndef BYTES_HIGHFIRST
    uint8 tc8 : 1,
        tc9 : 1,
        tc10 : 1,
        tc11 : 1,
        tc12 : 1,
        tc13 : 1,
        tc14 : 1,
        tc15 : 1;
#else
    uint8
        tc15 : 1,
        tc14 : 1,
        tc13 : 1,
        tc12 : 1,
        tc11 : 1,
        tc10 : 1,
        tc9 : 1;
#endif
  } r;
  uint8 v;
} wr13_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        br_generator_enable : 1, // 1
        br_generator_source : 1, // 2
        dtr_req_fn : 1,          // 4
        auto_echo : 1,           // 8
        localloopback : 1,       // 16
        dpll_cmd : 3;            // 32-
#else
    uint8
        dpll_cmd : 3,            // 32-
        localloopback : 1,       // 16
        auto_echo : 1,           // 8
        dtr_req_fn : 1,          // 4
        br_generator_source : 1, // 2
        br_generator_enable : 1; // 1
#endif
  } r;
  uint8 v;
} wr14_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        wr7prime : 1,
        zero_count_interrupt_enable : 1,
        sldc_fifo_enable : 1,
        dcd_interrupt_enable : 1,
        sync_hunt_interrupt_enable : 1,
        cts_interrupt_enable : 1,
        tx_underrun_eom_interrupt_enable : 1,
        break_abort_interrupt_enable : 1;
#else
    uint8
        break_abort_interrupt_enable : 1,
        tx_underrun_eom_interrupt_enable : 1,
        cts_interrupt_enable : 1,
        sync_hunt_interrupt_enable : 1,
        dcd_interrupt_enable : 1,
        sldc_fifo_enable : 1,
        zero_count_interrupt_enable : 1,
        wr7prime : 1;

#endif
  } r;
  uint8 v;
} wr15_t;

typedef union
{
  struct
  { // status bits
#ifndef BYTES_HIGHFIRST
    uint8
        rx_char_available : 1, // 1     //bit0
        zero_count : 1,        // 2     //bit1
        tx_buffer_empty : 1,   // 4     //bit2
        dcd : 1,               // 8     //bit3
        sync_hunt : 1,         // 16    //bit4
        cts : 1,               // 32    //bit5
        tx_underrun_eom : 1,   // bit6
        break_abort : 1;       // bit7
#else
    uint8
        break_abort : 1,       // bit7
        tx_underrun_eom : 1,   // bit6
        cts : 1,               // 32    //bit5
        sync_hunt : 1,         // 16    //bit4
        dcd : 1,               // 8     //bit3
        tx_buffer_empty : 1,   // 4     //bit2
        zero_count : 1,        // 2     //bit1
        rx_char_available : 1; // 1     //bit0
#endif
  } r;
  uint8 v;
} rr0_t;

typedef union
{
  struct
  { // special receive condition bits
#ifndef BYTES_HIGHFIRST
    uint8
        all_sent : 1,
        residue_code_2 : 1,
        residue_code_1 : 1,
        residue_code_0 : 1,
        parity_error : 1,
        rx_overrun_error : 1,
        crc_framing_error : 1,
        sdlc_end_of_frame : 1;
#else
    uint8
        sdlc_end_of_frame : 1,
        crc_framing_error : 1,
        rx_overrun_error : 1,
        parity_error : 1,
        residue_code_0 : 1,
        residue_code_1 : 1,
        residue_code_2 : 1,
        all_sent : 1;
#endif
  } r;
  uint8 v;
} rr1_t;

typedef union
{
  struct
  { // interrupt vector written to wr2a, wr2b is status
#ifndef BYTES_HIGHFIRST
    uint8 v0 : 1, // Table 5-6. Interrupt Vector Modification
                  // V3|V2|V1 Status High/Status Low =0
        v1 : 1,   // V4|V5|V6 Status High/Status Low =1
        v2 : 1,   // ------------------------------------------
        v3 : 1,   // 0  0  0  Ch B Transmit Buffer Empty
                  // 0  0  1  Ch B External/Status Change
        v4 : 1,   // 0  1  0  Ch B Receive Char. Available
        v5 : 1,   // 0  1  1  Ch B Special Receive Condition
        v6 : 1,   // 1  0  0  Ch A Transmit Buffer Empty
                  // 1  0  1  Ch A External/Status Change
        v7 : 1;   // 1  1  0  Ch A Receive Char. Available
                  // 1  1  1  Ch A Special Receive Condition
#else
    uint8 v7 : 1,

        v6 : 1,
        v5 : 1,
        v4 : 1,

        v3 : 1,
        v2 : 1,
        v1 : 1,

        v0 : 1;
#endif

  } r;
  uint8 v;
} rr2_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        ch_b_ext_status_irq_pending : 1, // pending IRQ's
        ch_b_tx_irq_pending : 1,
        ch_b_rx_irq_pending : 1,
        ch_a_ext_status_irq_pending : 1,
        ch_a_tx_irq_pending : 1,
        ch_a_rx_irq_pending : 1, : 2;
#else
    uint8 : 2,
        ch_a_rx_irq_pending : 1,
        ch_a_tx_irq_pending : 1,
        ch_a_ext_status_irq_pending : 1,
        ch_b_rx_irq_pending : 1,
        ch_b_tx_irq_pending : 1,
        ch_b_ext_status_irq_pending : 1; // pending IRQ's
#endif
  } r;
  uint8 v;
} rr3_t;

typedef union
{
  uint8 r; // same as rr0 on 8530, escc returns wr4
  uint8 v;
} rr4_t;

typedef union
{
  uint8 r; // on escc - ext read - wr5, else rr1
  uint8 v;
} rr5_t;

// rr4 should return wr4 if extended read is on, otherwise rr0.
// rr5 should return wr5 if extended read is on, otherwise rr1

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        bc0 : 1, // returns rr2 on 8530, on escc lsb of frame count on top of fifo
        bc1 : 1,
        bc2 : 1,
        bc3 : 1,
        bc4 : 1,
        bc5 : 1,
        bc6 : 1,
        bc7 : 1;
#else
    uint8
        bc7 : 1, // returns rr2 on 8530, on escc lsb of frame count on top of fifo
        bc6 : 1,
        bc5 : 1,
        bc4 : 1,
        bc3 : 1,
        bc2 : 1,
        bc1 : 1,
        bc0 : 1;

#endif
  } r;
  uint8 v;
} rr6_t;

typedef union
{
  struct
  { // on escc msb of frame count on top of fifo - 8530 image of rr3
#ifndef BYTES_HIGHFIRST
    uint8 bc8 : 1,
        bc9 : 1,
        bc10 : 1,
        bc11 : 1,
        bc12 : 1,
        bc13 : 1,
        fifo_data_avial : 1,
        fifo_overflow_status : 1;
#else
    uint8
        fifo_overflow_status : 1,
        fifo_data_avial : 1,
        bc13 : 1,
        bc12 : 1,
        bc11 : 1,
        bc10 : 1,
        bc9 : 1,
        bc8 : 1;
#endif
  } r;
  uint8 v;
} rr7_t;

// rr8=received_Data_register
typedef union // received data register
{
  uint8 rx_data;
  uint8 v;
} rr8_t;

typedef union // on escc wr3 if extended read, else + 8530= rr13
{
  uint8 r;
  uint8 v;
} rr9_t;

// rr9 reflects wr3 if extended read option, else rr13

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8 : 1,      // misc status bits
        onloop : 1, // always return 0 for these in normal ops - only used in sdlc/dpll/fm modes
        : 1, : 1,
        loopsending : 1, : 1,
        two_clocks_missing : 1,
        one_clock_missing : 1;
#else
    uint8
        one_clock_missing : 1,
        two_clocks_missing : 1, : 1,
        loopsending : 1, : 1, : 1,
        onloop : 1, // always return 0 for these in normal ops - only used in sdlc/dpll/fm modes
        : 1;        // misc status bits
#endif
  } r;
  uint8 v;
} rr10_t;

typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8 clksel : 2, // image of rr15 on 8530, on escc ext read - return wr10
        TRxC : 1,
        xmtclksrc : 2,
        rcvclksrc : 2,
        RTxCXtal : 1;
#else
    uint8
        RTxCXtal : 1,
        rcvclksrc : 2,
        xmtclksrc : 2,
        TRxC : 1,
        clksel : 2; // image of rr15 on 8530, on escc ext read - return wr10

#endif
  } r;
  uint8 v;
} rr11_t;

typedef union
{
  uint8 r; // returns wr12
  uint8 v;
} rr12_t;

typedef union
{
  uint8 r; // returns wr13
  uint8 v;
} rr13_t;

typedef union
{
  uint8 r; // return rr10, escc ext read - return wr7prime
  uint8 v;
} rr14_t;

typedef union
{
  uint8 r; // returns wr15 status IE bits
  uint8 v;
} rr15_t;

// rr11 returns wr10 if extended read is on, else rr15

// rr12 returns value in wr12
// rr13 returns value in wr13
// rr14 returns wr7prime if extended read, else rr10
// rr15 returns wr15

#ifdef ESCC
typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        autotx : 1,
        autoeomreset : 1,
        autortsdeactivation : 1,
        rxfifohalffull : 1,      // on Z85C30 this is force TXD high
        dtr_req_timing_mode : 1, // on Z85C30 this is dtr_req_fastmode
        txfifoempty : 1,         // on Z85c30 this is complete CRC reception
        extendedreadenable : 1,
        : 1; // reserved bit must be 0
#else
    uint8 : 1, // reserved bit must be 0
        extendedreadenable : 1,
        txfifoempty : 1,         // on Z85c30 this is complete CRC reception
        dtr_req_timing_mode : 1, // on Z85C30 this is dtr_req_fastmode
        rxfifohalffull : 1,      // on Z85C30 this is force TXD high
        autortsdeactivation : 1,
        autoeomreset : 1,
        autotx : 1;
#endif
  } r;
  uint8 v;
} rr7prime_t;

{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        autotx : 1,
        autoeomreset : 1,
        autortsdeactivation : 1,
        rxfifohalffull : 1,      // on Z85C30 this is force TXD high
        dtr_req_timing_mode : 1, // on Z85C30 this is dtr_req_fastmode
        txfifoempty : 1,         // on Z85c30 this is complete CRC reception
        extendedreadenable : 1,
        : 1; // reserved bit must be 0
#else
    uint8 : 1, // reserved bit must be 0
        extendedreadenable : 1,
        txfifoempty : 1,         // on Z85c30 this is complete CRC reception
        dtr_req_timing_mode : 1, // on Z85C30 this is dtr_req_fastmode
        rxfifohalffull : 1,      // on Z85C30 this is force TXD high
        autortsdeactivation : 1,
        autoeomreset : 1,
        autotx : 1;

#endif
  } r;
  uint8 v;
}
wr7prime_t;

#endif

#ifdef Z85C30
typedef union
{
  struct
  {
#ifndef BYTES_HIGHFIRST
    uint8
        autotx : 1,
        autoeomreset : 1,
        autortsdeactivation : 1,
        forcetxdhigh : 1,
        dtr_req_fastmode : 1,
        completecrcreception : 1,
        extendedreadenable : 1,
        : 1; // reserved bit must be 0
#else
    uint8 : 1, // reserved bit must be 0
        extendedreadenable : 1,
        completecrcreception : 1,
        dtr_req_fastmode : 1,
        forcetxdhigh : 1,
        autortsdeactivation : 1,
        autoeomreset : 1,
        autotx : 1;

#endif
  } r;
  uint8 v;
} rr7prime_t;
#endif

typedef struct
{
  wr0_t wr0;
  wr1_t wr1;
  wr2_t wr2;
  wr3_t wr3;
  wr4_t wr4;
  wr5_t wr5;
  wr6_t wr6;
  wr7_t wr7;
  wr8_t wr8;
  wr9_t wr9;
  wr10_t wr10;
  wr11_t wr11;
  wr12_t wr12;
  wr13_t wr13;
  wr14_t wr14;
  wr15_t wr15;
  uint8 placeholder16;
  uint8 placeholder16b;

#ifdef ESCC
  wr7prime_t wr7prime;
#else
#ifdef Z85C30
  wr7prime_t wr7prime;
#else
  uint8 placeholder;
#endif
#endif

} wr_t;

typedef struct
{
  rr0_t rr0;
  rr1_t rr1;
  rr2_t rr2;
  rr3_t rr3;
  rr4_t rr4;
  rr5_t rr5;
  rr6_t rr6;
  rr7_t rr7;
  rr8_t rr8;
  rr9_t rr9;
  rr10_t rr10;
  rr11_t rr11;
  rr12_t rr12;
  rr13_t rr13;
  rr14_t rr14;
  rr15_t rr15;

} rr_t;

// SCC function hooks/(method overrides) to external device drivers
typedef struct
{
  void (*send_break)(unsigned int port);
  void (*set_dtr)(unsigned int port, uint8 value);
  void (*set_rts)(unsigned int port, uint8 value);
  int (*get_dcd)(unsigned int port);
  int (*get_cts)(unsigned int port);
  int (*get_break)(unsigned int port);
  void (*signal_parity_error)(unsigned int port);
  void (*signal_crc_error)(unsigned int port);
  void (*set_even_parity)(unsigned int port);
  void (*set_odd_parity)(unsigned int port);
  void (*set_no_parity)(unsigned int port);
  void (*set_bits_per_char)(unsigned int port, uint8 bitsperchar);
  void (*set_stop_bits)(unsigned int port, uint8 stopbits);
  char (*read_serial_port)(unsigned int port);
  void (*write_serial_port)(unsigned int port, char data);
  void (*read_port_if_ready)(unsigned int port);
  void (*scc_hardware_reset_port)(unsigned int port);
  void (*scc_channel_reset_port)(unsigned int port);
  void (*set_baud_rate)(int port, uint32 baud);
} sccfunc_t;

#ifdef LISAEMSCCZ8530

union
{
  uint8 r[18]; // access will be scc_r[port].r[reg] or scc_r[port].s.rr0
  rr_t s;
} scc_r[2];

union
{
  uint8 w[18]; // access will be scc_w[port].w[reg] or scc_w[port].s.wr0
  wr_t s;
} scc_w[2];

#else

extern union
{
  uint8 r[18]; // access will be scc_r[port].r[reg] or scc_r[port].s.rr0
  rr_t s;
} scc_r[2];

extern union
{
  uint8 w[18]; // access will be scc_w[port].w[reg] or scc_w[port].s.wr0
  wr_t s;
} scc_w[2];

// prototypes
extern void send_break(unsigned int port);
extern void set_dtr(unsigned int port, uint8 value);
extern void set_rts(unsigned int port, uint8 value);
extern int get_dcd(unsigned int port);
extern int get_cts(unsigned int port);
extern int get_break(unsigned int port);
extern void signal_parity_error(unsigned int port);
extern void signal_crc_error(unsigned int port);
extern void set_even_parity(unsigned int port);
extern void set_odd_parity(unsigned int port);
extern void set_no_parity(unsigned int port);
extern void set_bits_per_char(unsigned int port, uint8 bitsperchar);
extern void set_stop_bits(unsigned int port, uint8 stopbits);
extern char read_serial_port(unsigned int port);
extern void write_serial_port(unsigned int port, char data);
extern void scc_hardware_reset_port(unsigned int port);
extern void scc_channel_reset_port(unsigned int port);
extern void initialize_scc(int actual);
extern void lisa_wb_Oxd200_sccz8530(uint32 address, uint8 data);
extern void lisa_wb_Oxd200_sccz8530(uint32 address, uint8 data);
extern void scc_control_loop(void);
extern void dump_scc(void);

#endif

#endif
