/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5210/ar5210_phy.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5210

#include "ah.h"
#include "ah_internal.h"

#include "ar5210/ar5210.h"

/* shorthands to compact tables for readability */
#define	OFDM	IEEE80211_T_OFDM
#define	TURBO	IEEE80211_T_TURBO

HAL_RATE_TABLE ar5210_11a_table = {
	8,  /* number of rates */
	{ 0 },
	{
/*                                                  short            ctrl  */
/*                valid                 rateCode Preamble  dot11Rate Rate */
/*   6 Mb */ {  AH_TRUE, OFDM,    6000,     0x0b,    0x00, (0x80|12),   0 },
/*   9 Mb */ {  AH_TRUE, OFDM,    9000,     0x0f,    0x00,        18,   0 },
/*  12 Mb */ {  AH_TRUE, OFDM,   12000,     0x0a,    0x00, (0x80|24),   2 },
/*  18 Mb */ {  AH_TRUE, OFDM,   18000,     0x0e,    0x00,        36,   2 },
/*  24 Mb */ {  AH_TRUE, OFDM,   24000,     0x09,    0x00, (0x80|48),   4 },
/*  36 Mb */ {  AH_TRUE, OFDM,   36000,     0x0d,    0x00,        72,   4 },
/*  48 Mb */ {  AH_TRUE, OFDM,   48000,     0x08,    0x00,        96,   4 },
/*  54 Mb */ {  AH_TRUE, OFDM,   54000,     0x0c,    0x00,       108,   4 }
	},
};

HAL_RATE_TABLE ar5210_turbo_table = {
	8,  /* number of rates */
	{ 0 },
	{
/*                                                 short            ctrl  */
/*                valid                rateCode Preamble  dot11Rate Rate */
/*   6 Mb */ {  AH_TRUE, TURBO,   6000,    0x0b,    0x00, (0x80|12),   0 },
/*   9 Mb */ {  AH_TRUE, TURBO,   9000,    0x0f,    0x00,        18,   0 },
/*  12 Mb */ {  AH_TRUE, TURBO,  12000,    0x0a,    0x00, (0x80|24),   2 },
/*  18 Mb */ {  AH_TRUE, TURBO,  18000,    0x0e,    0x00,        36,   2 },
/*  24 Mb */ {  AH_TRUE, TURBO,  24000,    0x09,    0x00, (0x80|48),   4 },
/*  36 Mb */ {  AH_TRUE, TURBO,  36000,    0x0d,    0x00,        72,   4 },
/*  48 Mb */ {  AH_TRUE, TURBO,  48000,    0x08,    0x00,        96,   4 },
/*  54 Mb */ {  AH_TRUE, TURBO,  54000,    0x0c,    0x00,       108,   4 }
	},
};

#undef	OFDM
#undef	TURBO

const HAL_RATE_TABLE *
ar5210GetRateTable(struct ath_hal *ah, u_int mode)
{
	HAL_RATE_TABLE *rt;
	switch (mode) {
	case HAL_MODE_11A:
		rt = &ar5210_11a_table;
		break;
	case HAL_MODE_TURBO:
		rt =  &ar5210_turbo_table;
		break;
	default:
		HALDEBUG(ah, "%s: invalid mode 0x%x\n", __func__, mode);
		return AH_NULL;
	}
	ath_hal_setupratetable(ah, rt);
	return rt;
}
#endif /* AH_SUPPORT_AR5210 */
