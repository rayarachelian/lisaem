/* the ordering of these includes is important - stdio can have inline
   functions - so can mem68k.h - and registers.h must appear before them */

#include <generator.h>
#include <registers.h>

#include <stdio.h>

#include <cpu68k.h>
#include <reg68k.h>

// only needed for DEBUG()
#include <vars.h>

#define DATAREG(a) (reg68k_regs[a])
#define ADDRREG(a) (reg68k_regs[8+(a)])
#define PC         (reg68k_pc)
#define SR         (reg68k_sr.sr_int)
#define SP         (regs.sp)
#define STOP       (regs.stop)
#define TFLAG      (reg68k_sr.sr_struct.t)
#define SFLAG      (reg68k_sr.sr_struct.s)
#define XFLAG      (reg68k_sr.sr_struct.x)
#define NFLAG      (reg68k_sr.sr_struct.n)
#define ZFLAG      (reg68k_sr.sr_struct.z)
#define VFLAG      (reg68k_sr.sr_struct.v)
#define CFLAG      (reg68k_sr.sr_struct.c)
#define IMASK      ((SR>>8) & 7)


static __inline__ sint32 idxval_dst(t_ipc *ipc) {
  sint32 r;

  //switch( ((ipc->dst>>27) & 1) | ((ipc->dst>>30) & 2) ) {
  //  23,24,25,26,27,28,29,30,31
  //  -1, 0, 1, 2, 3, 4, 5, 6, 7
switch( ((ipc->reg>>3) & 1) | ((ipc->reg>>6) & 2) )
 {
  case 0: // data, word
  //return ((sint16)DATAREG((ipc->dst>>28)&7))  +  ((((sint32)(ipc->dst<<8)))>>8);
    r=((sint16)DATAREG((ipc->reg>>4)&7))  +  ((sint32)(ipc->dst)) ;
    DEBUG_LOG(10,"r=%08lx %ld =datareg(%ld)=%08lx + %08lx",(long)r,(long)r,
            (long)(ipc->reg&7),
            (long)((sint16)DATAREG(ipc->reg&7)),
            (long)((sint32)(ipc->dst))
            );
    return r;

  case 1: // data, long
    r=((sint32)DATAREG((ipc->reg>>4)&7))  +  ((sint32)(ipc->dst));
    DEBUG_LOG(10,"r=%08lx %ld = = datareg(%ld) %08lx + %08lx",(long)r,(long)r,
             (long)(ipc->reg&7),
             (long)(sint32)DATAREG(ipc->reg&7),
             (long)((sint32)(ipc->dst))
             );
    return r;

  case 2: // addr, word                         ** this seems very strange ***  ****BUGHERE?????**********
  //return ((sint16)ADDRREG((ipc->dst>>28)&7))  +  ((((sint32)(ipc->dst<<8)))>>8);
  //** This doesn't work because the above is dst>>28.  >>28 is equivalent to >>4
    r=((sint16)ADDRREG((ipc->reg>>4)&7))  +  ((sint32)(ipc->dst));
    DEBUG_LOG(0,"SUSPECT r=%08lx %ld = addrreg(%ld).w %04lx + %08lx  DANGER HERE DANGER HERE DANGER HERE!!!",(long)r,(long)r,
             (long)(ipc->reg&7),
             (long)(sint16)ADDRREG(ipc->reg&7),
             (long)((sint32)(ipc->dst))
            );
    return r;

  case 3: // addr, long
    r=((sint32)ADDRREG((ipc->reg>>4)&7)  +  ((sint32)(ipc->dst)) );
    DEBUG_LOG(10,"r=%08lx %ld = addrreg(%ld) %08lx + %08lx",(long)r,(long)r,
             (long)(ipc->reg&7),
             (long)(sint32)ADDRREG(ipc->reg&7),
             (long)((sint32)(ipc->dst))
            );
    return r;
  }
  return 0;
}


static __inline__ sint32 idxval_src(t_ipc *ipc) {
  sint32 r;
//  switch( ((ipc->src>>27) & 1) | ((ipc->src>>30) & 2) ) {  // ra uncommented 20070704
  //  24,25,26,27,28,29,30,31
  //   0  1  2  3  4  5  6  7
switch( ((ipc->reg>>3) & 1) | ((ipc->reg>>6) & 2) )
 {
  case 0: // data, word
    //20060130-RA// r=((sint16)DATAREG((ipc->src>>28)&7))  +  ((((sint32)(ipc->src<<8)))>>8);
    //20060203-RA// r=((sint16)DATAREG((ipc->src>>28)&7))  +  ((sint32)(ipc->src));

//  r=((sint16)DATAREG((ipc->dst>>28)&7))+((((sint32)(ipc->dst<<8)))>>8); // <-0.35+cbierre patch
//      r=((sint16)DATAREG((ipc->reg)&7))+((((sint32)(ipc->dst<<8)))>>8); // <-0.35+cbierre patch lisaem 1.2.7

//    r=((sint16)DATAREG((ipc->reg>>4)&7)  +  ((sint32)(ipc->src))   );  //lisaem1.2.x
      r=((sint16)DATAREG((ipc->reg>>4 )&7)  +  ((sint32)(ipc->src))   );  //lisaem1.2.x

    DEBUG_LOG(10,"idxvsrc:0:data,word D%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx",(long)((ipc->reg>>4)&7), (long)DATAREG((ipc->reg>>4)&7) ,(long)r,(long)r,(long)ipc->src,(long)ipc->dst);
    DEBUG_LOG(10,"idxval_src=%08lx (%ld) = %04lx+%08lx",(unsigned long int)r,(long)r,
             (unsigned long int)(((sint32)(sint16)DATAREG(ipc->reg&7))),((unsigned long int)((sint32)(ipc->src))) );

    return r;
  case 1: // data, long
    //20060130-RA// r=((sint32)DATAREG((ipc->src>>28)&7))  +  ((((sint32)(ipc->src<<8)))>>8);
    //20060203-RA// r=((sint32)DATAREG((ipc->src>>28)&7))  +  ((sint32)(ipc->src) );

    r=((sint32)DATAREG((ipc->reg>>4)&7)  +   ((sint32)(ipc->src) ));

    DEBUG_LOG(10,"idxvsrc:1:data,long D%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx",(long)((ipc->reg>>4)&7), (long)DATAREG((ipc->reg>>4)&7) ,(long)r,(long)r,(long)ipc->src,(long)ipc->dst);
    DEBUG_LOG(10,"idxval_src=%08lx (%ld) = %08lx+%08lx",(long)r,(long)r,
             (unsigned long)((sint32)DATAREG(ipc->reg&7)),(unsigned long)((sint32)(ipc->src) )  );
    return r;

  case 2: // addr, word
    //20060130-RA// r=((sint16)ADDRREG((ipc->src>>28)&7)) +  ((((sint32)(ipc->src<<8)))>>8);
    //20060203-RA// r=((sint16)ADDRREG((ipc->src>>28)&7))  +  ((sint32)(ipc->src) );

    r=((sint16)ADDRREG((ipc->reg>>4)&7))  +  ((sint32)(ipc->src))   ;
    DEBUG_LOG(10,"idxvsrc:2:addr,word A%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx",(long)((ipc->reg>>4)&7), (unsigned long)ADDRREG((ipc->reg  )&7),(long)r,(unsigned long)r,(unsigned long)ipc->src,(unsigned long)ipc->dst);
    DEBUG_LOG(10,"idxval_src=%08lx (%ld) = %04lx+%08lx",(unsigned long)r,(long)r,
              (long)((sint16)ADDRREG(ipc->reg&7)),(unsigned long)((sint32)(ipc->src) )  );
    return r;
  case 3: // addr, long
    //20060130-RA// r=((sint32)ADDRREG((ipc->src>>28)&7))  +  ((((sint32)(ipc->src<<8)))>>8);
    //20060203-RA// r=((sint32)ADDRREG((ipc->src>>28)&7))  +  ((sint32)(ipc->src) );

    r=((sint32)ADDRREG((ipc->reg>>4)&7)  +  ((sint32)(ipc->src))  );
    DEBUG_LOG(10,"idxvsrc:3:addr,long A%ld=%08lx r:%08lx (%ld) src:%08lx dst:%08lx",(long)((ipc->reg>>4)&7), (long)ADDRREG((ipc->reg)&7),(long)r,(long)r,(unsigned long)ipc->src,(unsigned long)ipc->dst);

    DEBUG_LOG(10,"idxval_src=%08lx (%ld) = %08lx+%08lx",(long)r,(long)r,
      (long)((sint32)ADDRREG(ipc->reg&7)),(unsigned long)((sint32)(ipc->src)  ) );
    return r;
  }
  return 0;
}



// Added by RA
#define SWAP_USP_SSP()     {ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP; DEBUG_LOG(5,"S mode change SP:%08lx/A7:%08lx swapped.",(unsigned long)ADDRREG(7),(unsigned long)SP);}
#define SYNC_PC_SR()       {regs.pc = reg68k_pc; regs.sr = reg68k_sr;}


// Added by Ray Arachelian for Lisa Emulator to handle address errors and such without completion of the operation.
extern int abort_opcode;
#define ABORT_OPCODE_CHK() { if (abort_opcode==1) {DEBUG_LOG(0,"ABORTING OPCODE AT %08lx abort_opcode=1",(long)PC); return;}  }

// Added by Ray Arachelian for Lisa Emulator: on S flag change, mmu may change context, etc.
// this does the following: 1. swap USP with SSP, 2. synchronize PC and SR to setjmp land, 3. does an MMU flush

#define SR_CHANGE() { SWAP_USP_SSP();                                \
                      SYNC_PC_SR();                                  \
                      mmuflush(0x2000|(SFLAG ? 0x1000:0));           \
                     }

////// #define IRQMASKLOWER() { DEBUG_LOG(0,"IRQMASK lowered, setting clocks_stop from:%016llx to %016llx ",cpu68k_clocks_stop,cpu68k_clocks-1); cpu68k_clocks_stop=cpu68k_clocks-1;}
#define IRQMASKLOWER() { DEBUG_LOG(0,"IRQMASK lowered"); }
