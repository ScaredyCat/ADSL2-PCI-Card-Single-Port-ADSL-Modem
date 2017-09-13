/*
 * Copyright (c) 2002 2003 Sam Leffler, Errno Consulting
 * Copyright (c) 2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5312/ar5312phy.h#2 $
 */
#ifndef _DEV_ATH_AR5312PHY_H_
#define _DEV_ATH_AR5312PHY_H_

#include "ar5212/ar5212phy.h"

/* PHY registers */

#undef AR_PHY_PLL_CTL_44_5112                   /* Undefine the 5212 defs */
#undef AR_PHY_PLL_CTL_40_5112
#undef AR_PHY_PLL_CTL_40_5112_HALF              /* Undefine the 5212 defs */
#undef AR_PHY_PLL_CTL_40_5112_QUARTER

#define AR_PHY_PLL_CTL_44_5112  0x14d6          /* 44 MHz for 11b, 11g */
#define AR_PHY_PLL_CTL_40_5112  0x14d4          /* 40 MHz for 11a, turbos */
#define AR_PHY_PLL_CTL_40_5112_HALF  0x15d4	/* 40 MHz for 11a, turbos (Half)*/
#define AR_PHY_PLL_CTL_40_5112_QUARTER  0x16d4	/* 40 MHz for 11a, turbos (Quarter)*/

#endif	/* _DEV_ATH_AR5312PHY_H_ */
