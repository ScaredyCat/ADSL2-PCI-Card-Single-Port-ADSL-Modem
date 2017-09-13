/* $Id: ttyId.h,v 1.1 2000/08/30 18:27:01 armin Exp $
 *
 * ttyId - CAPI TTY AT-command emulator
 *
 * based on the AT-command emulator of the isdn4linux
 * kernel subsystem.
 *
 * Copyright 2000 by Armin Schindler (mac@melware.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: ttyId.h,v $
 * Revision 1.1  2000/08/30 18:27:01  armin
 * Okay, here is the first try for an user-land
 * ttyI daemon. Compilable but not useable.
 *
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/serial_reg.h>



#define ETX	0x03
#define DLE	0x10
#define XON     0x11
#define DC2     0x12
#define XOFF    0x13
#define DC4	0x14

/*
 * Definition of some special Registers of AT-Emulator
 */
#define REG_RINGATA   0
#define REG_RINGCNT   1  /* ring counter register */
#define REG_ESC       2
#define REG_CR        3
#define REG_LF        4
#define REG_BS        5

#define REG_WAITC     7

#define REG_RESP     12  /* show response messages register */
#define BIT_RESP      1  /* show response messages bit      */
#define REG_RESPNUM  12  /* show numeric responses register */
#define BIT_RESPNUM   2  /* show numeric responses bit      */
#define REG_ECHO     12
#define BIT_ECHO      4
#define REG_DCD      12
#define BIT_DCD       8
#define REG_CTS      12
#define BIT_CTS      16
#define REG_DTRR     12
#define BIT_DTRR     32
#define REG_DSR      12
#define BIT_DSR      64
#define REG_CPPP     12
#define BIT_CPPP    128

#define REG_DXMT     13
#define BIT_DXMT      1
#define REG_T70      13
#define BIT_T70       2
#define BIT_T70_EXT  32
#define REG_DTRHUP   13
#define BIT_DTRHUP    4
#define REG_RESPXT   13
#define BIT_RESPXT    8
#define REG_CIDONCE  13
#define BIT_CIDONCE  16
#define REG_RUNG     13  /* show RUNG message register      */
#define BIT_RUNG     64  /* show RUNG message bit           */
#define REG_DISPLAY  13
#define BIT_DISPLAY 128

#define REG_L2PROT   14
#define REG_L3PROT   15
#define REG_PSIZE    16
#define REG_WSIZE    17
#define REG_SI1      18
#define REG_SI2      19
#define REG_SI1I     20
#define REG_PLAN     21
#define REG_SCREEN   22

#define REG_CPN      23
#define BIT_CPN       1
#define BIT_CPNFCON   2

/* defines for result codes */
#define RESULT_OK               0
#define RESULT_CONNECT          1
#define RESULT_RING             2
#define RESULT_NO_CARRIER       3
#define RESULT_ERROR            4
#define RESULT_CONNECT64000     5
#define RESULT_NO_DIALTONE      6
#define RESULT_BUSY             7
#define RESULT_NO_ANSWER        8
#define RESULT_RINGING          9
#define RESULT_NO_MSN_EAZ       10
#define RESULT_VCON             11
#define RESULT_RUNG             12


/*
 * Values for Layer-2-protocol-selection
 */
#define ISDN_PROTO_L2_X75I   0   /* X75/LAPB with I-Frames            */
#define ISDN_PROTO_L2_X75UI  1   /* X75/LAPB with UI-Frames           */
#define ISDN_PROTO_L2_X75BUI 2   /* X75/LAPB with UI-Frames           */
#define ISDN_PROTO_L2_HDLC   3   /* HDLC                              */
#define ISDN_PROTO_L2_TRANS  4   /* Transparent (Voice)               */
#define ISDN_PROTO_L2_X25DTE 5   /* X25/LAPB DTE mode                 */
#define ISDN_PROTO_L2_X25DCE 6   /* X25/LAPB DCE mode                 */
#define ISDN_PROTO_L2_V11096 7   /* V.110 bitrate adaption 9600 Baud  */
#define ISDN_PROTO_L2_V11019 8   /* V.110 bitrate adaption 19200 Baud */
#define ISDN_PROTO_L2_V11038 9   /* V.110 bitrate adaption 38400 Baud */
#define ISDN_PROTO_L2_MODEM  10  /* Analog Modem on Board */
#define ISDN_PROTO_L2_FAX    11  /* Fax Group 2/3         */
#define ISDN_PROTO_L2_MAX    31  /* Max. 32 Protocols                 */

/*
 * Values for Layer-3-protocol-selection
 */
#define ISDN_PROTO_L3_TRANS     0       /* Transparent */
#define ISDN_PROTO_L3_TRANSDSP  1       /* Transparent with DSP */
#define ISDN_PROTO_L3_FCLASS2   2       /* Fax Group 2/3 CLASS 2 */
#define ISDN_PROTO_L3_FCLASS1   3       /* Fax Group 2/3 CLASS 1 */
#define ISDN_PROTO_L3_MAX       31      /* Max. 32 Protocols */

#define ISDN_MODEM_WINSIZE 8

#define ISDN_MODEM_NUMREG    99  /* Number of Modem-Registers        */
#define ISDN_LMSNLEN         255 /* Length of Listen-MSN string */
#define ISDN_CMSGLEN         80  /* Length of CONNECT-Message to add for Modem */
#define ISDN_MSNLEN          32  /* Length of MSN string */

#include "ttyId_fax.h"

/* data of AT-command-interpreter */
typedef struct atemu {
        u_char       profile[ISDN_MODEM_NUMREG]; /* Modem-Regs. Profile 0              */
        u_char       mdmreg[ISDN_MODEM_NUMREG];  /* Modem-Registers                    */
        char         pmsn[ISDN_MSNLEN];          /* EAZ/MSNs Profile 0                 */
        char         msn[ISDN_MSNLEN];           /* EAZ/MSN                            */
        char         plmsn[ISDN_LMSNLEN];        /* Listening MSNs Profile 0           */
        char         lmsn[ISDN_LMSNLEN];         /* Listening MSNs                     */
        char         cpn[ISDN_MSNLEN];           /* CalledPartyNumber on incoming call */
        char         connmsg[ISDN_CMSGLEN];      /* CONNECT-Msg from HL-Driver         */
        u_char       vpar[10];                   /* Voice-parameters                   */
        int          lastDLE;                    /* Flag for voice-coding: DLE seen    */
        int          mdmcmdl;                    /* Length of Modem-Commandbuffer      */
        int          pluscount;                  /* Counter for +++ sequence           */
        int          lastplus;                   /* Timestamp of last +                */
        int          carrierwait;                /* Seconds of carrier waiting         */
        char         mdmcmd[255];                /* Modem-Commandbuffer                */
        unsigned int charge;                     /* Charge units of current connection */
} atemu;

typedef struct modem_info {
  int                   x_char;          /* xon/xoff character             */
  int                   mcr;             /* Modem control register         */
  int                   msr;             /* Modem status register          */
  int                   lsr;             /* Line status register           */
  int                   online;          /* 1 = B-Channel is up, drop data */
                                         /* 2 = B-Channel is up, deliver d.*/
  int                   dialing;         /* Dial in progress or ATA        */
  int                   ncarrier;        /* Flag: schedule NO CARRIER      */
  unsigned char         OAD[ISDN_MSNLEN];
  unsigned char         OSA[ISDN_MSNLEN];
  unsigned char         CPN[ISDN_MSNLEN];
  unsigned char         CSA[ISDN_MSNLEN];
  unsigned char         last_cause[8];   /* Last cause message             */
  unsigned char         last_num[ISDN_MSNLEN];
                                         /* Last phone-number              */
  unsigned char         last_l2;         /* Last layer-2 protocol          */
  unsigned char         last_si;         /* Last service                   */
  unsigned char         last_lhup;       /* Last hangup local?             */
  unsigned char         last_dir;        /* Last direction (in or out)     */
  int                   xmit_count;      /* # of chars in xmit_buf         */
  unsigned char         *xmit_buf;       /* transmit buffer                */
  int                   vonline;         /* Voice-channel status           */
                                         /* Bit 0 = recording              */
                                         /* Bit 1 = playback               */
                                         /* Bit 2 = playback, DLE-ETX seen */
  void                  *adpcms;         /* state for adpcm decompression  */
  void                  *adpcmr;         /* state for adpcm compression    */
  void                  *dtmf_state;     /* state for dtmf decoder         */
  void                  *silence_state;  /* state for silence detection    */
  struct T30_s          fax;             /* T30 Fax Group 3 data/interface */  
  atemu                 emu;             /* AT-emulator data               */
  struct termios        normal_termios;  /* For saving termios structs     */
} modem_info;

/* Utility-Macros */
#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a>b)?a:b)

/* main */
extern int pty_fd;
extern int debug;
extern modem_info info;

/* utils */
extern void logit(int level, char *fmt, ...);
extern void CopyString(char *Destination, char *Source, int Len);
extern int getnum(char **p);
extern void getdial(char *p, char *q,int cnt);
extern void get_msnstr(char *n, char **p);

/* profile */
extern void modem_init(void);
extern void reset_profile(void);

/* emulator */
extern int tty_edit_at(const char *p, int count);
extern void tty_cmd_ATA(void);
extern void modem_reset_regs(int force);

/* communicate */
extern int tty_write(unsigned char *buf, int len);

/* pty */
extern int writepty(unsigned char *buf, int len);
