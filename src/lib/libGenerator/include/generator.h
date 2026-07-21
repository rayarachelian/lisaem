#ifndef IN_GENERATOR_H
#define IN_GENERATOR_H

#include <machine.h>

#ifdef PROCESSOR_INTEL
#undef WORDS_BIGENDIAN
#undef BIG_ENDIAN
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#undef BYTESHIGHFIRST
#endif

#ifdef PROCESSOR_SPARC
#define WORDS_BIGENDIAN 1
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 1
#endif
#undef LITTLE_ENDIAN
#define BYTESHIGHFIRST 1
#endif

#ifdef __POWERPC__
#define BYTES_HIGHFIRST 1
#define WORDS_BIGENDIAN 1
#endif
#ifdef __sparc__
#define BYTES_HIGHFIRST 1
#define WORDS_BIGENDIAN
#endif
#ifdef sparc
#define BYTES_HIGHFIRST 1
#define WORDS_BIGENDIAN 1
#endif

#ifdef __CYGWIN__
#undef WORDS_BIGENDIAN
#undef BIG_ENDIAN
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#undef BYTESHIGHFIRST
#endif

#define VERSTRING VERSION
#define PROGNAME LisaEmulator

#define LEN_IPCLISTTABLE 4 * 1024

extern char gen_leafname[];

#ifdef WORDS_BIGENDIAN
#define LOCENDIAN16(y) (y)
#define LOCENDIAN32(y) (y)
#else
#define LOCENDIAN16(y) ((((y) >> 8) & 0x00FF) + ((((y) << 8) & 0xFF00)))

#define LOCENDIAN32(y) ((((y) >> 24) & 0x000000FF) + \
                        (((y) >> 8) & 0x0000FF00) +  \
                        (((y) << 8) & 0x00FF0000) +  \
                        (((y) << 24) & 0xFF000000))
#endif

#define SWAP16(y) ((((y) >> 8) & 0x00FF) + ((((y) << 8) & 0xFF00)))
#define SWAP32(y) ((((y) >> 24) & 0x000000FF) + \
                   (((y) >> 8) & 0x0000FF00) +  \
                   (((y) << 8) & 0x00FF0000) +  \
                   (((y) << 24) & 0xFF000000))

typedef enum
{
    tp_src,
    tp_dst
} t_type;

typedef enum
{
    sz_none,
    sz_byte,
    sz_word,
    sz_long
} t_size;

typedef enum
{
    //        0        1        2       3         4        5         6  <<--
    dt_Dreg,
    dt_Areg,
    dt_Aind,
    dt_Ainc,
    dt_Adec,
    dt_Adis,
    dt_Aidx,
    //        7        8        9       10
    dt_AbsW,
    dt_AbsL,
    dt_Pdis,
    dt_Pidx,
    //       11       12       13       14
    dt_ImmB,
    dt_ImmW,
    dt_ImmL,
    dt_ImmS,
    //       15       16       17       18        19
    dt_Imm3,
    dt_Imm4,
    dt_Imm8,
    dt_Imm8s,
    dt_ImmV,
    dt_Ill
    //      20
} t_datatype;

typedef enum
{
    ea_Dreg,
    ea_Areg,
    ea_Aind,
    ea_Ainc,
    ea_Adec,
    ea_Adis,
    ea_Aidx,
    ea_AbsW,
    ea_AbsL,
    ea_Pdis,
    ea_Pidx,
    ea_Imm
} t_eatypes;

typedef enum
{
    i_ILLG,
    i_OR,
    i_ORSR,
    i_AND,
    i_ANDSR,
    i_EOR,
    i_EORSR,
    i_SUB,
    i_SUBA,
    i_SUBX,
    i_ADD,
    i_ADDA,
    i_ADDX,
    i_MULU,
    i_MULS,
    i_CMP,
    i_CMPA,
    i_BTST,
    i_BCHG,
    i_BCLR,
    i_BSET,
    i_MOVE,
    i_MOVEA,
    i_MOVEPMR,
    i_MOVEPRM,
    i_MOVEFSR,
    i_MOVETSR,
    i_MOVEMRM,
    i_MOVEMMR,
    i_MOVETUSP,
    i_MOVEFUSP,
    i_NEG,
    i_NEGX,
    i_CLR,
    i_NOT,
    i_ABCD,
    i_SBCD,
    i_NBCD,
    i_SWAP,
    i_PEA,
    i_LEA,
    i_EXT,
    i_EXG,
    i_TST,
    i_TAS,
    i_CHK,
    i_TRAPV,
    i_TRAP,
    i_RESET,
    i_NOP,
    i_STOP,
    i_LINK,
    i_UNLK,
    i_RTE,
    i_RTS,
    i_RTR,
    i_JSR,
    i_JMP,
    i_Scc,
    i_SF,
    i_DBcc,
    i_DBRA,
    i_Bcc,
    i_BSR,
    i_DIVU,
    i_DIVS,
    i_ASR,
    i_LSR,
    i_ROXR,
    i_ROR,
    i_ASL,
    i_LSL,
    i_ROXL,
    i_ROL,
    i_LINE10,
    i_LINE15
} t_mnemonic;

typedef struct
{
    uint16 mask;         /* mask of bits that are static */
    uint16 bits;         /* bit values corresponding to bits in mask */
    t_mnemonic mnemonic; /* instruction mnemonic */
    struct
    {
        unsigned int priv : 1;        /* instruction is privileged if set */
        unsigned int endblk : 1;      /* instruction ends a block if set */
        unsigned int imm_notzero : 1; /* immediate data cannot be 0 (if applicable) */
        unsigned int used : 5;        /* bitmap of XNZVC flags inspected */
        unsigned int set : 5;         /* bitmap of XNZVC flags altered */
    } flags;
    t_size size;              /* size of instruction */
    t_datatype stype;         /* type of source */
    t_datatype dtype;         /* type of destination */
    unsigned int sbitpos : 4; /* bit pos of imm data or reg part of EA */
    unsigned int dbitpos : 4; /* reg part of EA */
    unsigned int immvalue;    /* if stype is ImmS this is the value */
    unsigned int cc;          /* condition code if mnemonic is Scc/Dbcc/Bcc */
    unsigned int funcnum;     /* function number for this instruction */
    unsigned int wordlen;     /* length in words of this instruction */
    unsigned int clocks;      /* number of external clock periods */
} t_iib;                      /* instruction information block */

#define IIB_FLAG_X 1 << 0
#define IIB_FLAG_N 1 << 1
#define IIB_FLAG_Z 1 << 2
#define IIB_FLAG_V 1 << 3
#define IIB_FLAG_C 1 << 4

typedef struct
{
    t_mnemonic mnemonic;
    const char *name;
} t_mnemonic_table;

extern t_mnemonic_table mnemonic_table[];

extern char *condition_table[];

typedef union
{ // Possible bug  BYTES_HIGHFIRST is not dealt with in autoconf???!!!
    struct
    {
#ifndef BYTES_HIGHFIRST
        uint16 c : 1;  // this should be 1   as per MC68000 book fig 1-3 status reg.
        uint16 v : 1;  // this should be 2
        uint16 z : 1;  // this should be 4   // these must be uint16, and not unsigned int else the struct/union
        uint16 n : 1;  // this should be 8   // will fail to line up!
        uint16 x : 1;  // this should be 16
        uint16 : 3;    // 32.64.128
        uint16 i0 : 1; // 256,512,1024
        uint16 i1 : 1;
        uint16 i2 : 1;
        uint16 : 2;   // 2048,4096
        uint16 s : 1; // 8192
        uint16 : 1;   // 16384
        uint16 t : 1; // 32768
#else
        uint16 t : 1; // I believe this section is correct for sparc
        uint16 : 1;
        uint16 s : 1;
        uint16 : 2;
        uint16 i2 : 1;
        uint16 i1 : 1;
        uint16 i0 : 1;
        uint16 : 3;
        uint16 x : 1; // this should be 16
        uint16 n : 1; // this should be 8
        uint16 z : 1; // this should be 4
        uint16 v : 1; // this should be 2
        uint16 c : 1; // this should be 1
#endif
    } sr_struct;
    uint16 sr_int;
} t_sr;

typedef struct
{
    uint32 pc;
    uint32 sp;
    t_sr sr;
    uint16 stop;
    uint32 regs[16];
    uint16 pending;
} t_regs;

#define SR_CFLAG (1 << 0)
#define SR_VFLAG (1 << 1)
#define SR_ZFLAG (1 << 2)
#define SR_NFLAG (1 << 3)
#define SR_XFLAG (1 << 4)
#define SR_SFLAG (1 << 13)
#define SR_TFLAG (1 << 15)

#define LOG_DEBUG3(x) /* */
#define LOG_DEBUG2(x) /* */
#define LOG_DEBUG1(x) /* */
// next 5lines fixed by RA for Lisa emulator
#define LOG_USER(fmt, args...) ui_log_user(fmt, ##args)
#define LOG_VERBOSE(fmt, args...) ui_log_verbose(fmt, ##args)
#define LOG_NORMAL(fmt, args...) ui_log_normal(fmt, ##args)
#define LOG_CRITICAL(fmt, args...) ui_log_critical(fmt, ##args)
#define LOG_REQUEST(fmt, args...) ui_log_request(fmt, ##args)

#endif
