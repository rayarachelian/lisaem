/**************************************************************************************\
*                                                                                      *
*              The Lisa Emulator Project  V1.3.0      REL 2010.01.24                   *
*                             http://lisaem.sunder.net                                 *
*                                                                                      *
*                  Copyright (C) 1998, 2010 Ray A. Arachelian                          *
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
*     CPU Core Tester - support code to compare multiple CPU core emulator results     *
*     You'd have one main core and upto MAXCACHE slave cores.  As opcodes execute on   *
*     the main core, memory accesses and register states before/after are recorded.    *
*     Then, the same opcode is executed on each test core, and the results compared.   *
*     Differences in opcode results and memory accesses are reported.                  *
*                                                                                      *
*                                                                                      *
*                                                                                      *
\**************************************************************************************/


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>


#define MAXCACHE 64
#define MAXCORES 1

static char thisopcode[65536];        // message buffer

typedef struct                        // indicates memory accesses and their order
{
  int         writeflag;
  int         readflag;
  int         size;
  uint32      address;
  uint32      physaddress;
  uint32      value;
}             corecache_t;


typedef struct                        // shmem structure shared between processes
{
uint32 gosignal;                      // semaphore allowing master and slave coordination

uint32 start_regs[32];
uint32 end_regs[32];
uint32 end_slave_regs[MAXCORES][32];  // written by the slave -- might not actually need this

uint32 corecacheidx=0;                // index into the last write
uint32 coreidx[MAXCORES];
uint32 mmucx;                         // MMU context that the access happened in (might not be used in all emulators)
uint32 failure=0;                     // failure recorded by slave if any
char   text[1024];                    // notes to pass back and forth - do we really need this? the slave can just output to thisopcode

corecache_t   corecache[MAXCACHE];    // the read/write cache
} core;

//------------------------------------------------------------------------------------------------------------------------------
// The slave will not be actually allowed to read or write data to memory, rather it will use the functions here to simulate
// memory calls via the corecache, any memory access that doesn't match what the master did will cause the slave to flag an
// error and report a differing result (as well as any differences in end register values, of course.)
// since interrupts of the M68K write to stack, and change the SR/SSR as well as PC, these are handled by the same corecache/
// regs mechanism.  So the slave doesn't have to be a full emulator, but rather can be just the emulator's CPU core.
//
// we can even check for the memory access order as this matters for I/O since on the 68K, I/O is handled via memory accesses.
//
// all communication is done over a shared memory segment, we don't bother to free it at the end.
//
// go signal is a sort of semaphore used by both master and slave.  When the master is done, it writes a 1 value here, then whenever
// the slave is done, it will write a 2 indicating that it's done.
//
//------------------------------------------------------------------------------------------------------------------------------
void corecache_flush(void)
{
 int i,j;
 failure=0;
 mmucx=-1;

 for (i=0; i<64; i++)
  {
   corecache[i].readflag     = 0;
   corecache[i].writeflag    = 0;
   corecache[i].size         = 0;
   corecache[i].address      = 0xffffffff;
   corecache[i].physaddress  = 0xffffffff;
   corecache[i].value        = 0xffffffff;

   for (i=0; i<MAXCORES; i++) corecache[i].touchedby[j]=0;
  }

 corecacheidx=0;
 for (i=0; i<MAXCORES; i++) coreidx[i]=0;
 memset(text,0,1024);
 memset(thisopcode,0,65535);
 mmucx=-1;
}

// edit these two fn's for  your own CPU's
void corecpu_get_start_masterregs(void)
{
    start_regs[ 0]=mc68k_regs[ 0];
    start_regs[ 1]=mc68k_regs[ 1];
    start_regs[ 2]=mc68k_regs[ 2];
    start_regs[ 3]=mc68k_regs[ 3];
    start_regs[ 4]=mc68k_regs[ 4];
    start_regs[ 5]=mc68k_regs[ 5];
    start_regs[ 6]=mc68k_regs[ 6];
    start_regs[ 7]=mc68k_regs[ 7];
    start_regs[ 8]=mc68k_regs[ 8];
    start_regs[ 9]=mc68k_regs[ 9];
    start_regs[10]=mc68k_regs[10];
    start_regs[11]=mc68k_regs[11];
    start_regs[12]=mc68k_regs[12];
    start_regs[13]=mc68k_regs[13];
    start_regs[14]=mc68k_regs[14];
    start_regs[15]=mc68k_regs[15];

    start_regs[16]=mc68k_pc;
    start_regs[17]=mc68k_sr.sr_int;
    start_regs[18]=mc68k_sp;
}
// edit these two fn's for  your own CPU's

char *regnames[19]=
{
     "D0","D1","D2","D3","D4","D5","D6","D7",
     "A0","A1","A2","A3","A4","A5","A6","A7",
     "PC","SR","SP"
};


void corecpu_get_end_masterregs(void)
{
    end_regs[ 0]=mc68k_regs[ 0];
    end_regs[ 1]=mc68k_regs[ 1];
    end_regs[ 2]=mc68k_regs[ 2];
    end_regs[ 3]=mc68k_regs[ 3];
    end_regs[ 4]=mc68k_regs[ 4];
    end_regs[ 5]=mc68k_regs[ 5];
    end_regs[ 6]=mc68k_regs[ 6];
    end_regs[ 7]=mc68k_regs[ 7];
    end_regs[ 8]=mc68k_regs[ 8];
    end_regs[ 9]=mc68k_regs[ 9];
    end_regs[10]=mc68k_regs[10];
    end_regs[11]=mc68k_regs[11];
    end_regs[12]=mc68k_regs[12];
    end_regs[13]=mc68k_regs[13];
    end_regs[14]=mc68k_regs[14];
    end_regs[15]=mc68k_regs[15];

    end_regs[16]=mc68k_pc;
    end_regs[17]=mc68k_sr.sr_int;
    end_regs[18]=mc68k_sp;
}

end_slave_regs[i]
void corecpu_get_slave_masterregs(void)
{
 int i;


for (i=0; i<MAXCORES; i++)
   {
    end_slave_regs[i][ 0]=mc68k_regs[ 0];
    end_slave_regs[i][ 1]=mc68k_regs[ 1];
    end_slave_regs[i][ 2]=mc68k_regs[ 2];
    end_slave_regs[i][ 3]=mc68k_regs[ 3];
    end_slave_regs[i][ 4]=mc68k_regs[ 4];
    end_slave_regs[i][ 5]=mc68k_regs[ 5];
    end_slave_regs[i][ 6]=mc68k_regs[ 6];
    end_slave_regs[i][ 7]=mc68k_regs[ 7];
    end_slave_regs[i][ 8]=mc68k_regs[ 8];
    end_slave_regs[i][ 9]=mc68k_regs[ 9];
    end_slave_regs[i][10]=mc68k_regs[10];
    end_slave_regs[i][11]=mc68k_regs[11];
    end_slave_regs[i][12]=mc68k_regs[12];
    end_slave_regs[i][13]=mc68k_regs[13];
    end_slave_regs[i][14]=mc68k_regs[14];
    end_slave_regs[i][15]=mc68k_regs[15];
    end_slave_regs[i][16]=mc68k_pc;
    end_slave_regs[i][17]=mc68k_sr.sr_int;
    end_slave_regs[i][18]=mc68k_sp;
   }
}

int corecpu_compare_regs(void)
{
 int i,j,flag=0;


for (i=0; i<MAXCORES; i++)
   {

    for (j=0; j<19; j++)
     if (end_slave_regs[i][ j]!=end_regs[ j])
     {
       snprintf(text,1024,"Reg mistmatch %s master:%08x core%d:%08x\n",rename[j],end_regs[j],i,end_slave_regs[i]);
       strncat(thisopcode,65535,text);
       flag=1; failure=1;
     }
   }

return flag;
}

void corecpu_start_opcode(char *intext, int startmmucx)
{
    corecache_flush();
    strncpy(thisopcode,8192,intext);
    mmucx=startmmucx;
    corecpu_get_start_masterregs();
}

void corecpu_complete_opcode(int endmmucx)
{
    int i,j;

    // don't bother printing anything on MMU context changes as too much stuff is in flux for it to work.
    if (endmmucx!=mmucx) return;

    for (i=0; i<corecacheidx; i++)
    {

       for (j=0; i<MAXCORES; j++)
          if (!corecache[i].touchedby[j])
             {
               failure=1;
               snprintf(text,1024,"core:%d did not access memory event %d\n",j,i)
                }
    }

    corecpu_get_end_masterregs();
    corecpu_get_end_slaveregs();
    corecpu_compare_regs(void)

    if (failure)
    {
        puts("--CPU-CORE-CACHE-----------------------------------------");
        puts(thisopcode);
        puts("memory events:")
        for (i=0; i<corecacheidx; i++)
         printf("%d: %s%s %d/%08x (%08x) %s %x\n",
                 i,
                 corecache[i].readflag  ? "r":"",
                  corecache[i].writeflag ? "w":"",
                 mmucx,
                 corecache[i].address,corecache[i].physaddr,
                 corecache[i].size==1 ?"byte":(corecache[i].size==2 ?"word":(corecahce[i].size==3 ?"long":"UNKNOWN")),
                 corecache[i].value
               );
        puts("--------------------------------------------------------");

    }

}


// only called by C1
void record_access(uint32 address, int size, int write, uint32 value)
{
   if (corecacheidx+2>MAXCACHE)  {ALERT_LOG(0,"RAN OUT OF CORECACHE!!!!!!"); EXIT(411);}

   if (write) {
               corecache[corecacheidx].writeflag=1; corecache[corecacheidx].value=value;
                                                    corecache[corecacheidx].size=size;
              }

   else       {
               corecache[corecacheidx].readflag=1;  corecache[corecacheidx].value=value;
                                                    corecache[corecacheidx].size=size;
              }

   corecache[corecacheidx].address=address;
   corecache[corecacheidx].physaddress=physaddr;
   corecache[corecacheidx].touchedby[0]++;

   corecacheidx++;
}


uint8 corecpu_rb(uint32 address, int core)
{
    #define SIZE 1
    #define FILTER 0xff

    int i,j=-1, flag=0;
    i=coreidx[core];
    coreidx[core]++;

    if (corecache[i].address!=addess && corecache[i].size==SIZE  && corecache[i].write==0)
       {
        j=i;
       }
    else
        for (i=0; i<corecacheidx; i++)
             if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write==0)
                {j=i; i=corecacheidx;}

    snprintf(text,1024,"c%d rb@%08x=%02x:",core,address,(uint8)corecache[j].value);
    if (j==-1) { strncat(text,1024,"REQUESTED ACCESS NOT FOUND."); corecache_alert(core,text); return 0xff; }
    if (j!=coreidx[core]) {strncat(text,1024,"OUT OF ORDER."); flag=0;}

    corecache[j].touchedby[core]++; // record access
    CHK_RAM_LIMITS(address);
     if (corecache[j].writeflag) {strncat(text,1024,"READ FROM WRITE! "); flag=1;}
    if (physaddr!=corecache[j].physaddress) {strncat(text,1024,"PHYSICAL ADDRESS MISMATCH! "); flag=1;}

    if (flag) corecache_alert(core,text);
    return (uint8)(corecache[j].value & FILTER);
}
#undef SIZE
#undef FILTER

uint16 corecpu_rw(uint32 address, int core)
{
    #define SIZE 2
    #define FILTER 0xffff

    int i,j=-1, flag=0;
    i=coreidx[core];
    coreidx[core]++;

    if (corecache[i].address!=addess && corecache[i].size==SIZE  && corecache[i].write==0)
       {
        j=i;
       }
    else
        for (i=0; i<corecacheidx; i++)
             if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write==0)
                {j=i; i=corecacheidx;}

    snprintf(text,1024,"c%d rw@%08x=%04x:",core,address,(uint16)corecache[j].value);
    if (j==-1) { strncat(text,1024,"REQUESTED ACCESS NOT FOUND."); corecache_alert(core,text); return 0xff; }
    if (j!=coreidx[core]) {strncat(text,1024,"OUT OF ORDER."); flag=0;}

    corecache[j].touchedby[core]++; // record access
    CHK_RAM_LIMITS(address);
     if (corecache[j].writeflag) {strncat(text,1024,"READ FROM WRITE! "); flag=1;}
    if (physaddr!=corecache[j].physaddress) {strncat(text,1024,"PHYSICAL ADDRESS MISMATCH! "); flag=1;}

    if (flag) corecache_alert(core,text);
    return (uint16)(corecache[j].value & FILTER);
}
#undef SIZE
#undef FILTER

uint16 corecpu_rl(uint32 address, int core)
{
    #define SIZE 2
    #define FILTER 0xffffffff

    int i,j=-1, flag=0;
    i=coreidx[core];
    coreidx[core]++;

    if (corecache[i].address!=addess && corecache[i].size==SIZE  && corecache[i].write==0)
       {
        j=i;
       }
    else
        for (i=0; i<corecacheidx; i++)
             if (corecache[i].address!=addess && corecache[i].size==SIZE  && corecache[i].write==0)
                {j=i; i=corecacheidx;}

    snprintf(text,1024,"c%d rl@%08x=%08x:",core,address,corecache[j].value);
    if (j==-1) { strncat(text,1024,"REQUESTED ACCESS NOT FOUND."); corecache_alert(core,text); return 0xff; }
    if (j!=coreidx[core]) {strncat(text,1024,"OUT OF ORDER."); flag=0;}

    corecache[j].touchedby[core]++; // record access
    CHK_RAM_LIMITS(address);
     if (corecache[j].writeflag) {strncat(text,1024,"READ FROM WRITE! "); flag=1;}
    if (physaddr!=corecache[j].physaddress) {strncat(text,1024,"PHYSICAL ADDRESS MISMATCH! "); flag=1;}

    if (flag) corecache_alert(core,text);
    return (corecache[j].value & FILTER);
}
#undef SIZE
#undef FILTER





void corecpu_wb(uint32 address, uint8 value, int core)
{
    #define SIZE 1
    #define FILTER 0xff

    int i,j=-1, flag=0;
    i=coreidx[core];
    coreidx[core]++;

    if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write)
       {
        j=i;
       }
    else
        for (i=0; i<corecacheidx; i++)
             if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write)
                {j=i; i=corecacheidx;}

    snprintf(text,1024,"c%d wb@%08x=%02x:",core,address,(uint8)corecache[j].value);
    if (j==-1) { strncat(text,1024,"REQUESTED ACCESS NOT FOUND."); corecache_alert(core,text); return 0xff; }
    if (j!=coreidx[core]) {strncat(text,1024,"OUT OF ORDER."); flag=0;}

    corecache[j].touchedby[core]++; // record access
    CHK_RAM_LIMITS(address);
     if (corecache[j].writeflag) {strncat(text,1024,"READ FROM WRITE! "); flag=1;}
    if (physaddr!=corecache[j].physaddress) {strncat(text,1024,"PHYSICAL ADDRESS MISMATCH! "); flag=1;}

    if (corecache[j].value !=value)
       {
           flag=1;
           snprintf(text,1024,"Write value mismatch. Core:%d attempted to write %02x, but master had %02x",core,value,corecache[i].value);
       }

    if (flag) corecache_alert(core,text);
}
#undef SIZE
#undef FILTER


void corecpu_ww(uint32 address, uint16 value, int core)
{
    #define SIZE 2
    #define FILTER 0xffff

    int i,j=-1, flag=0;
    i=coreidx[core];
    coreidx[core]++;

    if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write)
       {
        j=i;
       }
    else
        for (i=0; i<corecacheidx; i++)
             if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write)
                {j=i; i=corecacheidx;}

    snprintf(text,1024,"c%d ww@%08x=%04x:",core,address,(uint8)corecache[j].value);
    if (j==-1) { strncat(text,1024,"REQUESTED ACCESS NOT FOUND."); corecache_alert(core,text); return 0xff; }
    if (j!=coreidx[core]) {strncat(text,1024,"OUT OF ORDER."); flag=0;}

    corecache[j].touchedby[core]++; // record access
    CHK_RAM_LIMITS(address);
     if (corecache[j].writeflag) {strncat(text,1024,"READ FROM WRITE! "); flag=1;}
    if (physaddr!=corecache[j].physaddress) {strncat(text,1024,"PHYSICAL ADDRESS MISMATCH! "); flag=1;}

    if (corecache[j].value !=value)
       {
          flag=1;
          snprintf(text,1024,"Write value mismatch. Core:%d attempted to write %04x, but master had %04x",core,value,corecache[i].value);
       }

    if (flag) corecache_alert(core,text);
}
#undef SIZE
#undef FILTER


void corecpu_wl(uint32 address, uint32 value, int core)
{
    #define SIZE 3
    #define FILTER 0xffffffff

    int i,j=-1, flag=0;
    i=coreidx[core];
    coreidx[core]++;

    if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write)
       {
        j=i;
       }
    else
        for (i=0; i<corecacheidx; i++)
             if (corecache[i].address!=addess && corecache[i].size==SIZE && corecache[i].write)
                {j=i; i=corecacheidx;}

    snprintf(text,1024,"c%d wl@%08x=%08x:",core,address,(uint8)corecache[j].value);
    if (j==-1) { strncat(text,1024,"REQUESTED ACCESS NOT FOUND."); corecache_alert(core,text); return 0xff; }
    if (j!=coreidx[core]) {strncat(text,1024,"OUT OF ORDER."); flag=0;}

    corecache[j].touchedby[core]++; // record access
    CHK_RAM_LIMITS(address);
     if (corecache[j].writeflag) {strncat(text,1024,"READ FROM WRITE! "); flag=1;}
    if (physaddr!=corecache[j].physaddress) {strncat(text,1024,"PHYSICAL ADDRESS MISMATCH! "); flag=1;}

    if (corecache[j].value !=value)
       {
           flag=1;
           snprintf(text,1024,"Write value mismatch. Core:%d attempted to write %08x, but master had %08x",core,value,corecache[i].value);
       }

    if (flag) corecache_alert(core,text);
}
#undef SIZE
#undef FILTER

void corecache_alert(int core, char *text)
{
    strncat(thisopcode,65536,text);
    strncat(thisopcode,65536,"\n");
    falilure=1;
}


void shm_client(void)
{

      int shmid;
    key_t key;
    char *shm, *s;

    /*
     * We need to get the segment named
     * "5678", created by the server.
     */
    key = 0x4D36386B; // "M68k"

    /*
     * Locate the segment.
     */
    if ((shmid = shmget(key, SHMSZ, 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    /*
     * Now read what the server put in the memory.
     */
    for (s = shm; *s != NULL; s++)
        putchar(*s);
    putchar('\n');

    /*
     * Finally, change the first character of the
     * segment to '*', indicating we have read
     * the segment.
     */
    *shm = '*';

}

void shm_server(void)
{
    char c;
    int shmid;
    key_t key;
    char *shm, *s;

    /*
     * We'll name our shared memory segment
     * "5678".
     */
    key = 0x4D36386B; // "M68k" 

    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    /*
     * Now put some things into the memory for the
     * other process to read.
     */
    s = shm;

    for (c = 'a'; c <= 'z'; c++)
        *s++ = c;
    *s = NULL;

    /*
     * Finally, we wait until the other process
     * changes the first character of our memory
     * to '*', indicating that it has read what
     * we put there.
     */
    while (*shm != '*')
        sleep(1);


}
// TODO's skip tracking on MMU changes
// disable these calls on icache loads.

//still need to copy start regs to slave input and end of slave output to code here.
