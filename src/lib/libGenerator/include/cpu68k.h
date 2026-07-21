#include <vars.h>

extern t_iib *cpu68k_iibtable[65536];
extern void (*cpu68k_functable[65536 * 2])(t_ipc *ipc);
extern int cpu68k_totalinstr;
extern int cpu68k_totalfuncs;
extern unsigned int cpu68k_clocks_curevent;
extern unsigned int cpu68k_line;
extern t_regs regs;
extern uint8 movem_bit[256];
extern unsigned int cpu68k_adaptive;

extern t_iib iibs[];
extern int iibs_num;

int cpu68k_init(void);
void cpu68k_printipc(t_ipc *ipc);
void cpu68k_ipc(uint32 addr68k, t_iib *iib, t_ipc *ipc);
void cpu68k_reset(void);
void cpu68k_step(void);
void cpu68k_framestep(void);
t_ipc_table *cpu68k_makeipclist(uint32 pc);
void cpu68k_endfield(void);

#define V_RESETSSP 0
#define V_RESETPC 1
#define V_BUSERR 2
#define V_ADDRESS 3
#define V_ILLEGAL 4
#define V_ZERO 5
#define V_CHK 6
#define V_TRAPV 7
#define V_PRIVILEGE 8
#define V_TRACE 9
#define V_LINE10 10
#define V_LINE15 11
#define V_FORMAT 14 // added by RA for RTE Format/Vector word handling
#define V_UNINIT 15
#define V_SPURIOUS 24
#define V_AUTO 25
#define V_TRAP 32
#define V_USER 64
