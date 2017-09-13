/* API only version generated from kernel drivers/isdn/pcbit/pcbit.h */

/*
 * PCBIT-D device driver definitions
 *
 * Copyright (C) 1996 Universidade de Lisboa
 * 
 * Written by Pedro Roque Marques (roque@di.fc.ul.pt)
 *
 * This software may be used and distributed according to the terms of 
 * the GNU General Public License, incorporated herein by reference.
 */

#ifndef PCBIT_H
#define PCBIT_H

#define MAX_PCBIT_CARDS 4
#define BLOCK_TIMER


/* isdn_ctrl only allows a long sized argument */

struct pcbit_ioctl {
	union {
		struct byte_op {
			ushort addr;
			ushort value;
		} rdp_byte;
		unsigned long l2_status;
	} info;
};



#define PCBIT_IOCTL_GETSTAT  0x01    /* layer2 status */
#define PCBIT_IOCTL_LWMODE   0x02    /* linear write mode */
#define PCBIT_IOCTL_STRLOAD  0x03    /* start load mode */
#define PCBIT_IOCTL_ENDLOAD  0x04    /* end load mode */
#define PCBIT_IOCTL_SETBYTE  0x05    /* set byte */
#define PCBIT_IOCTL_GETBYTE  0x06    /* get byte */
#define PCBIT_IOCTL_RUNNING  0x07    /* set protocol running */
#define PCBIT_IOCTL_WATCH188 0x08    /* set watch 188 */
#define PCBIT_IOCTL_PING188  0x09    /* ping 188 */
#define PCBIT_IOCTL_FWMODE   0x0A    /* firmware write mode */
#define PCBIT_IOCTL_STOP     0x0B    /* stop protocol */
#define PCBIT_IOCTL_APION    0x0C    /* issue API_ON  */

#define PCBIT_GETSTAT  (PCBIT_IOCTL_GETSTAT  + IIOCDRVCTL)
#define PCBIT_LWMODE   (PCBIT_IOCTL_LWMODE   + IIOCDRVCTL)
#define PCBIT_STRLOAD  (PCBIT_IOCTL_STRLOAD  + IIOCDRVCTL)
#define PCBIT_ENDLOAD  (PCBIT_IOCTL_ENDLOAD  + IIOCDRVCTL)
#define PCBIT_SETBYTE  (PCBIT_IOCTL_SETBYTE  + IIOCDRVCTL)
#define PCBIT_GETBYTE  (PCBIT_IOCTL_GETBYTE  + IIOCDRVCTL)
#define PCBIT_RUNNING  (PCBIT_IOCTL_RUNNING  + IIOCDRVCTL)
#define PCBIT_WATCH188 (PCBIT_IOCTL_WATCH188 + IIOCDRVCTL)
#define PCBIT_PING188  (PCBIT_IOCTL_PING188  + IIOCDRVCTL)
#define PCBIT_FWMODE   (PCBIT_IOCTL_FWMODE   + IIOCDRVCTL)
#define PCBIT_STOP     (PCBIT_IOCTL_STOP     + IIOCDRVCTL)
#define PCBIT_APION    (PCBIT_IOCTL_APION    + IIOCDRVCTL)

#define MAXSUPERLINE 3000

#define L2_DOWN     0
#define L2_LOADING  1
#define L2_LWMODE   2
#define L2_FWMODE   3
#define L2_STARTING 4
#define L2_RUNNING  5
#define L2_ERROR    6

#endif
