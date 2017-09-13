/* API only generated from: */

/* $Id: icn.h,v 1.1 2002/07/19 19:03:52 keil Exp $
 *
 * ISDN lowlevel-module for the ICN active ISDN-Card.
 *
 * Copyright 1994 by Fritz Elfert (fritz@isdn4linux.de)
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */

#ifndef icn_h
#define icn_h

#define ICN_IOCTL_SETMMIO   0
#define ICN_IOCTL_GETMMIO   1
#define ICN_IOCTL_SETPORT   2
#define ICN_IOCTL_GETPORT   3
#define ICN_IOCTL_LOADBOOT  4
#define ICN_IOCTL_LOADPROTO 5
#define ICN_IOCTL_LEASEDCFG 6
#define ICN_IOCTL_GETDOUBLE 7
#define ICN_IOCTL_DEBUGVAR  8
#define ICN_IOCTL_ADDCARD   9

/* Struct for adding new cards */
typedef struct icn_cdef {
	int port;
	char id1[10];
	char id2[10];
} icn_cdef;

#endif /* icn_h */
