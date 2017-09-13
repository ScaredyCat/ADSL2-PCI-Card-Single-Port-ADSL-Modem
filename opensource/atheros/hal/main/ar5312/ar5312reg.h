/*
 * Copyright (c) 2002 2003 Sam Leffler, Errno Consulting
 * Copyright (c) 2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5312/ar5312reg.h#1 $
 */
#ifndef _DEV_ATH_AR5312REG_H_
#define _DEV_ATH_AR5312REG_H_

#include "ar5212/ar5212reg.h"
/*
 * Definitions for the Atheros 5312 chipset.
 */

/* Register base addresses for modules which are not wmac modules */
/* 531X has a fixed memory map */


#define REG_WRITE(_reg,_val)		*((volatile u_int32_t *)(_reg)) = (_val);
#define REG_READ(_reg)		*((volatile u_int32_t *)(_reg))
/* 
 * PCI-MAC Configuration registers 
 */
#define AR5315_RSTIMER_BASE 0xb1000000  /* Address for reset/timer registers */
#define AR5315_GPIO_BASE    0xb1000000  /* Address for GPIO registers */
#define AR5315_WLAN0            0xb0000000

#define AR5315_RESET   0x0004      /* Offset of reset control register */
#define AR5315_SREV    0x0014      /* Offset of reset control register */
#define AR5315_ENDIAN_CTL  0x000c  /* offset of the endian control register */
#define AR5315_CONFIG_WLAN     0x00000002      /* WLAN byteswap */
#undef AR5315_GPIOCR
#undef AR5315_GPIODO                    /* Undefine the 5212 defs */
#undef AR5315_GPIODI

#define AR5315_REV_MAJ                     0x00f0
#define AR5315_REV_MIN                     0x000f

#define AR5315_GPIOCR       0x0098      /* GPIO Control register */
#define AR5315_GPIODO       0x0090      /* GPIO data output access reg */
#define AR5315_GPIODI       0x0088      /* GPIO data input access reg*/

#define AR5315_WREV         (-0xefbfe0)      /* Revision ID register offset */
#define AR5315_WREV_S       0           /* Shift for WMAC revision info */
#define AR5315_WREV_ID      0x000000FF  /* Mask for WMAC revision info */
#define AR5315_WREV_ID_S    4           /* Shift for WMAC Rev ID */
#define AR5315_WREV_REVISION 0x0000000F /* Mask for WMAN Revsion version */

#define AR5315_RC_BB0_CRES   0x00000002  /* Cold reset to WMAC0 & WBB0 */
#define AR5315_RC_BB1_CRES   0x00000200  /* Cold reset to WMAC1 & WBB1n */
#define AR5315_RC_WMAC0_RES  0x00000001  /* Warm reset to WMAC 0 */
#define AR5315_RC_WBB0_RES  0x00000002  /* Warm reset to WBB0 */
#define AR5315_RC_WMAC1_RES  0x00020000  /* Warm reset to WMAC1 */
#define AR5315_RC_WBB1_RES   0x00040000  /* Warm reset to WBB */


#define AR_RSTIMER_BASE 0xbc003000  /* Address for reset/timer registers */
#define AR_GPIO_BASE    0xbc002000  /* Address for GPIO registers */
#define AR531X_WLAN0            0xb8000000
#define AR531X_WLAN1            0xb8500000

#define AR531X_RESET	0x0020      /* Offset of reset control register */
#define	AR531X_PCICFG	0x00B0	    /* MAC/PCI configuration reg (LEDs) */

#define AR531X_PCICFG_LEDMODE  0x0000001c	/* LED Mode mask */
#define AR531X_PCICFG_LEDMODE_S  2	/* LED Mode shift */
#define AR531X_PCICFG_LEDMOD0  0	/* Blnk prop to Tx and filtered Rx */
#define AR531X_PCICFG_LEDMOD1  1	/* Blnk prop to all Tx and Rx */
#define AR531X_PCICFG_LEDMOD2  2	/* DEBG flash */
#define AR531X_PCICFG_LEDMOD3  3	/* BLNK Randomly */

#define	AR531X_PCICFG_LEDSEL   0x000000e0 /* LED Throughput select */
#define AR531X_PCICFG_LEDSEL_S 5
#define AR531X_PCICFG_LEDSEL0  0	/* See blink rate table on p. 143 */
#define AR531X_PCICFG_LEDSEL1  1	/* of AR5212 data sheet */
#define AR531X_PCICFG_LEDSEL2  2
#define AR531X_PCICFG_LEDSEL3  3 
#define AR531X_PCICFG_LEDSEL4  4
#define AR531X_PCICFG_LEDSEL5  5
#define AR531X_PCICFG_LEDSEL6  6
#define AR531X_PCICFG_LEDSEL7  7

#define AR531X_PCICFG_LEDSBR   0x00000100 /* Slow blink rate if no
			   		     activity. 0 = blink @ lowest
					     rate */

#undef AR_GPIOCR
#undef AR_GPIODO                    /* Undefine the 5212 defs */
#undef AR_GPIODI

#define AR_GPIOCR       0x0008      /* GPIO Control register */
#define AR_GPIODO       0x0000      /* GPIO data output access reg */
#define AR_GPIODI       0x0004      /* GPIO data input access reg*/

#define AR_WREV         0x0090      /* Revision ID register offset */
#define AR_WREV_S       8           /* Shift for WMAC revision info */
#define AR_WREV_ID      0x000000FF  /* Mask for WMAC revision info */
#define AR_WREV_ID_S    4           /* Shift for WMAC Rev ID */
#define AR_WREV_REVISION 0x0000000F /* Mask for WMAN Revsion version */

#define AR_RC_BB0_CRES   0x00000004  /* Cold reset to WMAC0 & WBB0 */
#define AR_RC_BB1_CRES   0x00000200  /* Cold reset to WMAC1 & WBB1n */
#define AR_RC_WMAC0_RES  0x00002000  /* Warm reset to WMAC 0 */
#define AR_RC_WBB0_RES  0x00004000  /* Warm reset to WBB0 */
#define AR_RC_WMAC1_RES  0x00020000  /* Warm reset to WMAC1 */
#define AR_RC_WBB1_RES   0x00040000  /* Warm reset to WBB */


#define AR_RAD2112_SREV_MAJOR   0x40    /* 2112 Major Rev */

enum AR5312PowerMode {
    AR5312_POWER_MODE_FORCE_SLEEP  = 0,
    AR5312_POWER_MODE_FORCE_WAKE   = 1,
    AR5312_POWER_MODE_NORMAL       = 2,
};

#endif /* _DEV_AR5312REG_H_ */
