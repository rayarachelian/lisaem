;------------------------------------------------------------------------------------
; mmu-stack tester - tiny program to fill most of Lisa RAM with it's own address
; and then set mmu segment 125 so we can see the physical addresses and test
; access via Service Mode.
;------------------------------------------------------------------------------------


  ORG     $20000     ; starting address note spaces at start

START:
  MOVE.L #$20080,D0  ; start ~0x40 bytes after this code.
FILL:
  MOVE.L D0,A0
  MOVE.L D0,(A0)
  ADDQ.L #$04,D0
  CMP.L ($110),D0   ; fill RAM until screenbase
  BLE FILL
  ;--------------------------------------------------------------------------------------------------------------------------------
  ; uniplus sets mmu like this:
  ;
  ;    mmu[1][122].slr:0c00,sor:0000  00f40000-00f5ffff::-->(00000000)  type::unused:0c00
  ;    mmu[1][123].slr:0c00,sor:0000  00f60000-00f7ffff::-->(00000000)  type::unused:0c00
  ;    mmu[1][124].slr:0c00,sor:0000  00f80000-00f9ffff::-->(00000000)  type::unused:0c00 v- so is this a bug somewhere else?
  ;    mmu[1][125].slr:07fc,sor:05dd  00fa0000-00fbffff::-->(0003ba00)  type::rw_mem:07fc <- this is normal RAM, not stack!!!
  ;    mmu[1][126].slr:0900,sor:0000  00fc0000-00fdffff::-->(00000000)  type::io_spc:0900
  ;    mmu[1][127].slr:0f00,sor:0000  00fe0000-00ffffff::-->(00000000)  type::siospc:0f00 
  ;
  ;   2x of 7d (125)=0xfa => 0x00faxxxx == sor:00fa8008 slr:00fa8000
  ;   400=rostack, 500=romem, 600=rwstack, 700=rwmem, 900=iospace, c00=bad page, 0f00=sio/rom
  ;--------------------------------------------------------------------------------------------------------------------------------
  MOVEA.L #$00fa8008,A2       ;MMUSADRB,A2    ;SET MMU BASE REG PTR - use fa here as fc will cause crashing in ROM  - fa8008 is SOR
  MOVEA.L #$00fa8000,A3       ;MMUSADRL,A3    ;SET LIMIT REG PTR                                                    - fa8000 is SLR
  MOVE.W  #$0300,D0           ;SET BASE VALUE should be 300*200=60000
  MOVE.W  #$0600,D1           ;MEMLMT,D1      ;SET to stack, only 32 pages.
  MOVE.L  #$00fe0084,A4       ;return value is monitor entry
  JMP      $00fe008C          ;WRTMMU and return to A4

  END START                   ;need spaces before end keyword for it to work.
