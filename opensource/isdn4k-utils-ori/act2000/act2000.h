/* API only version generated from kernel drivers/isdn/act2000/act2000.h */

/* $Id: act2000.h,v 1.1 2002/07/19 19:03:49 keil Exp $
 *
 * ISDN lowlevel-module for the IBM ISDN-S0 Active 2000.
 *
 * Author       Fritz Elfert
 * Copyright    by Fritz Elfert      <fritz@isdn4linux.de>
 * 
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Thanks to Friedemann Baitinger and IBM Germany
 *
 */

#ifndef act2000_h
#define act2000_h

#define ACT2000_IOCTL_SETPORT    1
#define ACT2000_IOCTL_GETPORT    2
#define ACT2000_IOCTL_SETIRQ     3
#define ACT2000_IOCTL_GETIRQ     4
#define ACT2000_IOCTL_SETBUS     5
#define ACT2000_IOCTL_GETBUS     6
#define ACT2000_IOCTL_SETPROTO   7
#define ACT2000_IOCTL_GETPROTO   8
#define ACT2000_IOCTL_SETMSN     9
#define ACT2000_IOCTL_GETMSN    10
#define ACT2000_IOCTL_LOADBOOT  11
#define ACT2000_IOCTL_ADDCARD   12

#define ACT2000_IOCTL_TEST      98
#define ACT2000_IOCTL_DEBUGVAR  99

#define ACT2000_BUS_ISA          1
#define ACT2000_BUS_MCA          2
#define ACT2000_BUS_PCMCIA       3

/* Struct for adding new cards */
typedef struct act2000_cdef {
	int bus;
        int port;
        int irq;
        char id[10];
} act2000_cdef;

/* Struct for downloading firmware */
typedef struct act2000_ddef {
        int length;             /* Length of code */
        char *buffer;           /* Ptr. to code   */
} act2000_ddef;

typedef struct act2000_fwid {
        char isdn[4];
        char revlen[2];
        char revision[504];
} act2000_fwid;

#endif                          /* act2000_h */
