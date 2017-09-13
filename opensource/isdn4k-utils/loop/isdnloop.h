/* API only version generated from kernel drivers/isdn/isdnloop/isdnloop.h */

/* $Id: isdnloop.h,v 1.1 2002/07/19 19:03:56 keil Exp $
 *
 * Loopback lowlevel module for testing of linklevel.
 *
 * Copyright 1997 by Fritz Elfert (fritz@isdn4linux.de)
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */

#ifndef isdnloop_h
#define isdnloop_h

#define ISDNLOOP_IOCTL_DEBUGVAR  0
#define ISDNLOOP_IOCTL_ADDCARD   1
#define ISDNLOOP_IOCTL_LEASEDCFG 2
#define ISDNLOOP_IOCTL_STARTUP   3

/* Struct for adding new cards */
typedef struct isdnloop_cdef {
	char id1[10];
} isdnloop_cdef;

/* Struct for configuring cards */
typedef struct isdnloop_sdef {
	int ptype;
	char num[3][20];
} isdnloop_sdef;

#endif                          /* isdnloop_h */
