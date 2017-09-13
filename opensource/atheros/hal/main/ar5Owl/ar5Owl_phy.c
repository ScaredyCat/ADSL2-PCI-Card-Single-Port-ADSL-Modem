/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5Owl/ar5Owl_phy.c#3 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#include "ah.h"
#include "ah_internal.h"

#include "ar5212/ar5212.h"

/* shorthands to compact tables for readability */
#define	OFDM	IEEE80211_T_OFDM
#define	CCK	IEEE80211_T_CCK
#define	TURBO	IEEE80211_T_TURBO
#define	XR	ATHEROS_T_XR
#define HT      IEEE80211_T_HT

HAL_RATE_TABLE ar5212_11a_table = {
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

HAL_RATE_TABLE ar5212_11a_half_table = {
	8,  /* number of rates */
	{ 0 },
	{
/*                                                  short            ctrl  */
/*                valid                 rateCode Preamble  dot11Rate Rate */
/*   6 Mb */ {  AH_TRUE, OFDM,    3000,     0x0b,    0x00, (0x80|6),   0 },
/*   9 Mb */ {  AH_TRUE, OFDM,    4500,     0x0f,    0x00,        9,   0 },
/*  12 Mb */ {  AH_TRUE, OFDM,    6000,     0x0a,    0x00, (0x80|12),   2 },
/*  18 Mb */ {  AH_TRUE, OFDM,    9000,     0x0e,    0x00,        18,   2 },
/*  24 Mb */ {  AH_TRUE, OFDM,   12000,     0x09,    0x00, (0x80|24),   4 },
/*  36 Mb */ {  AH_TRUE, OFDM,   18000,     0x0d,    0x00,        36,   4 },
/*  48 Mb */ {  AH_TRUE, OFDM,   24000,     0x08,    0x00,        48,   4 },
/*  54 Mb */ {  AH_TRUE, OFDM,   27000,     0x0c,    0x00,       54,   4 }
	},
};

HAL_RATE_TABLE ar5212_11a_quarter_table = {
	8,  /* number of rates */
	{ 0 },
	{
/*                                                  short            ctrl  */
/*                valid                 rateCode Preamble  dot11Rate Rate */
/*   6 Mb */ {  AH_TRUE, OFDM,    1500,     0x0b,    0x00, (0x80|3),   0 },
/*   9 Mb */ {  AH_TRUE, OFDM,    2250,     0x0f,    0x00,        4,   0 },
/*  12 Mb */ {  AH_TRUE, OFDM,    3000,     0x0a,    0x00, (0x80|6),   2 },
/*  18 Mb */ {  AH_TRUE, OFDM,    4500,     0x0e,    0x00,        9,   2 },
/*  24 Mb */ {  AH_TRUE, OFDM,    6000,     0x09,    0x00, (0x80|12),   4 },
/*  36 Mb */ {  AH_TRUE, OFDM,    9000,     0x0d,    0x00,        18,   4 },
/*  48 Mb */ {  AH_TRUE, OFDM,   12000,     0x08,    0x00,        24,   4 },
/*  54 Mb */ {  AH_TRUE, OFDM,   13500,     0x0c,    0x00,       27,   4 }
	},
};

HAL_RATE_TABLE ar5212_turbo_table = {
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

HAL_RATE_TABLE ar5212_11b_table = {
	4,  /* number of rates */
	{ 0 },
	{
/*                                                 short            ctrl  */
/*                valid                rateCode Preamble  dot11Rate Rate */
/*   1 Mb */ {  AH_TRUE,  CCK,    1000,    0x1b,    0x00, (0x80| 2),   0 },
/*   2 Mb */ {  AH_TRUE,  CCK,    2000,    0x1a,    0x04, (0x80| 4),   1 },
/* 5.5 Mb */ {  AH_TRUE,  CCK,    5500,    0x19,    0x04, (0x80|11),   1 },
/*  11 Mb */ {  AH_TRUE,  CCK,   11000,    0x18,    0x04, (0x80|22),   1 }
	},
};


/* Venice TODO: roundUpRate() is broken when the rate table does not represent rates
 * in increasing order  e.g.  5.5, 11, 6, 9.    
 * An average rate of 6 Mbps will currently map to 11 Mbps. 
 */
HAL_RATE_TABLE ar5212_11g_table = {
	12,  /* number of rates */
	{ 0 },
	{
/*                                                 short            ctrl  */
/*                valid                rateCode Preamble  dot11Rate Rate */
/*   1 Mb */ {  AH_TRUE, CCK,     1000,    0x1b,    0x00, (0x80| 2),   0 },
/*   2 Mb */ {  AH_TRUE, CCK,     2000,    0x1a,    0x04, (0x80| 4),   1 },
/* 5.5 Mb */ {  AH_TRUE, CCK,     5500,    0x19,    0x04, (0x80|11),   2 },
/*  11 Mb */ {  AH_TRUE, CCK,    11000,    0x18,    0x04, (0x80|22),   3 },
/* Hardware workaround - remove rates 6, 9 from rate ctrl */
/*   6 Mb */ { AH_FALSE, OFDM,    6000,    0x0b,    0x00,        12,   4 },
/*   9 Mb */ { AH_FALSE, OFDM,    9000,    0x0f,    0x00,        18,   4 },
/*  12 Mb */ {  AH_TRUE, OFDM,   12000,    0x0a,    0x00,        24,   6 },
/*  18 Mb */ {  AH_TRUE, OFDM,   18000,    0x0e,    0x00,        36,   6 },
/*  24 Mb */ {  AH_TRUE, OFDM,   24000,    0x09,    0x00,        48,   8 },
/*  36 Mb */ {  AH_TRUE, OFDM,   36000,    0x0d,    0x00,        72,   8 },
/*  48 Mb */ {  AH_TRUE, OFDM,   48000,    0x08,    0x00,        96,   8 },
/*  54 Mb */ {  AH_TRUE, OFDM,   54000,    0x0c,    0x00,       108,   8 }
	},
};

HAL_RATE_TABLE ar5212_xr_table = {
	13,		/* number of rates */
	{ 0 },
	{
/*                                                 short            ctrl  */
/*                valid                rateCode Preamble  dot11Rate Rate */
/* 0.25 Mb */{  AH_TRUE,   XR,    250,     0x03,    0x00, (0x80| 1),   0, 612, 612 },
/* 0.5 Mb */ {  AH_TRUE,   XR,    500,     0x07,    0x00, (0x80| 1),   0, 457, 457},
/*   1 Mb */ {  AH_TRUE,   XR,   1000,     0x02,    0x00, (0x80| 2),   1, 228, 228 },
/*   2 Mb */ {  AH_TRUE,   XR,   2000,     0x06,    0x00, (0x80| 4),   2, 160, 160,},
/*   3 Mb */ {  AH_TRUE,   XR,   3000,     0x01,    0x00, (0x80| 6),   3, 140, 140 },
/*   6 Mb */ {  AH_TRUE, OFDM,    6000,    0x0b,    0x00, (0x80|12),   4, 60,  60  },
/*   9 Mb */ {  AH_TRUE, OFDM,    9000,    0x0f,    0x00,        18,   4, 60,  60  },
/*  12 Mb */ {  AH_TRUE, OFDM,   12000,    0x0a,    0x00, (0x80|24),   6, 48,  48  },
/*  18 Mb */ {  AH_TRUE, OFDM,   18000,    0x0e,    0x00,        36,   6, 48,  48  },
/*  24 Mb */ {  AH_TRUE, OFDM,   24000,    0x09,    0x00,        48,   8, 44,  44  },
/*  36 Mb */ {  AH_TRUE, OFDM,   36000,    0x0d,    0x00,        72,   8, 44,  44  },
/*  48 Mb */ {  AH_TRUE, OFDM,   48000,    0x08,    0x00,        96,   8, 44,  44  },
/*  54 Mb */ {  AH_TRUE, OFDM,   54000,    0x0c,    0x00,       108,   8, 44,  44  }
	},
};

HAL_RATE_TABLE ar5Owl_11ng_table = {

    28,  /* number of rates */
    { -1 },
    {
/*                                                 short            ctrl  */
/*                valid                rateCode Preamble  dot11Rate Rate */
/*   1 Mb */ {  AH_TRUE, CCK,     1000,    0x1b,    0x00, (0x80| 2),   0 },
/*   2 Mb */ {  AH_TRUE, CCK,     2000,    0x1a,    0x04, (0x80| 4),   1 },
/* 5.5 Mb */ {  AH_TRUE, CCK,     5500,    0x19,    0x04, (0x80|11),   2 },
/*  11 Mb */ {  AH_TRUE, CCK,    11000,    0x18,    0x04, (0x80|22),   3 },
/* Hardware workaround - remove rates 6, 9 from rate ctrl */
/*   6 Mb */ { AH_FALSE, OFDM,    6000,    0x0b,    0x00,        12,   4 },
/*   9 Mb */ { AH_FALSE, OFDM,    9000,    0x0f,    0x00,        18,   4 },
/*  12 Mb */ {  AH_TRUE, OFDM,   12000,    0x0a,    0x00,        24,   6 },
/*  18 Mb */ {  AH_TRUE, OFDM,   18000,    0x0e,    0x00,        36,   6 },
/*  24 Mb */ {  AH_TRUE, OFDM,   24000,    0x09,    0x00,        48,   8 },
/*  36 Mb */ {  AH_TRUE, OFDM,   36000,    0x0d,    0x00,        72,   8 },
/*  48 Mb */ {  AH_TRUE, OFDM,   48000,    0x08,    0x00,        96,   8 },
/*  54 Mb */ {  AH_TRUE, OFDM,   54000,    0x0c,    0x00,       108,   8 },
/* 6.5 Mb */ {  AH_TRUE, HT,      6500,    0x0b,    0x00,         0,   8 },
/*  13 Mb */ {  AH_TRUE, HT,  	 13000,    0x0a,    0x00,      	  1,   8 },
/*19.5 Mb */ {  AH_TRUE, HT,     19500,    0x0e,    0x00,         2,   8 },
/*  26 Mb */ {  AH_TRUE, HT,  	 26000,    0x09,    0x00,         3,   8 },
/*  39 Mb */ {  AH_TRUE, HT,  	 39000,    0x0d,    0x00,         4,   8 },
/*  52 Mb */ {  AH_TRUE, HT,   	 52000,    0x08,    0x00,         5,   8 },
/*58.5 Mb */ {  AH_TRUE, HT,  	 58500,    0x0c,    0x00,         6,   8 },
/*  65 Mb */ {  AH_TRUE, HT,  	 65000,    0x0c,    0x00,         7,   8 },
/*  13 Mb */ {  AH_TRUE, HT,  	 13000,    0x0b,    0x00,         8,   8 },
/*  26 Mb */ {  AH_TRUE, HT,  	 26000,    0x0a,    0x00,         9,   8 },
/*  39 Mb */ {  AH_TRUE, HT,  	 39000,    0x0e,    0x00,        10,   8 },
/*  52 Mb */ {  AH_TRUE, HT,  	 52000,    0x09,    0x00,        11,   8 },
/*  78 Mb */ {  AH_TRUE, HT,  	 78000,    0x0d,    0x00,        12,   8 },
/* 104 Mb */ {  AH_TRUE, HT, 	104000,    0x08,    0x00,        13,   8 },
/* 117 Mb */ {  AH_TRUE, HT, 	117000,    0x0c,    0x00,        14,   8 },
/* 130 Mb */ {  AH_TRUE, HT, 	130000,    0x0c,    0x00,        15,   8 },
	},
};

static HAL_RATE_TABLE ar5Owl_11na_table = {

    24,  /* number of rates */
    { -1 },
    {
/*                                                 short            ctrl  */
/*                valid                rateCode Preamble  dot11Rate Rate */
/*   6 Mb */ {  AH_TRUE, OFDM,    6000,    0x0b,    0x00, (0x80|12),   0 },
/*   9 Mb */ {  AH_TRUE, OFDM,    9000,    0x0f,    0x00,        18,   0 },
/*  12 Mb */ {  AH_TRUE, OFDM,   12000,    0x0a,    0x00, (0x80|24),   2 },
/*  18 Mb */ {  AH_TRUE, OFDM,   18000,    0x0e,    0x00,        36,   2 },
/*  24 Mb */ {  AH_TRUE, OFDM,   24000,    0x09,    0x00, (0x80|48),   4 },
/*  36 Mb */ {  AH_TRUE, OFDM,   36000,    0x0d,    0x00,        72,   8 },
/*  48 Mb */ {  AH_TRUE, OFDM,   48000,    0x08,    0x00,        96,   8 },
/*  54 Mb */ {  AH_TRUE, OFDM,   54000,    0x0c,    0x00,       108,   8 },
/* 6.5 Mb */ {  AH_TRUE, HT,      6500,    0x0b,    0x00,         0,   8 },
/*  13 Mb */ {  AH_TRUE, HT,  	 13000,    0x0a,    0x00,         1,   8 },
/*19.5 Mb */ {  AH_TRUE, HT,     19500,    0x0e,    0x00,         2,   8 },
/*  26 Mb */ {  AH_TRUE, HT,  	 26000,    0x09,    0x00,         3,   8 },
/*  39 Mb */ {  AH_TRUE, HT,  	 39000,    0x0d,    0x00,         4,   8 },
/*  52 Mb */ {  AH_TRUE, HT,   	 52000,    0x08,    0x00,         5,   8 },
/*58.5 Mb */ {  AH_TRUE, HT,  	 58500,    0x0c,    0x00,         6,   8 },
/*  65 Mb */ {  AH_TRUE, HT,  	 65000,    0x0c,    0x00,         7,   8 },
/*  13 Mb */ {  AH_TRUE, HT,  	 13000,    0x0b,    0x00,         8,   8 },
/*  26 Mb */ {  AH_TRUE, HT,  	 26000,    0x0a,    0x00,         9,   8 },
/*  39 Mb */ {  AH_TRUE, HT,  	 39000,    0x0e,    0x00,        10,   8 },
/*  52 Mb */ {  AH_TRUE, HT,  	 52000,    0x09,    0x00,        11,   8 },
/*  78 Mb */ {  AH_TRUE, HT,  	 78000,    0x0d,    0x00,        12,   8 },
/* 104 Mb */ {  AH_TRUE, HT, 	104000,    0x08,    0x00,        13,   8 },
/* 117 Mb */ {  AH_TRUE, HT, 	117000,    0x0c,    0x00,        14,   8 },
/* 130 Mb */ {  AH_TRUE, HT, 	130000,    0x0c,    0x00,        15,   8 },
	},
};

#undef	OFDM
#undef	CCK
#undef	TURBO
#undef	XR
#undef	HT
#undef	HT_HGI

const HAL_RATE_TABLE *
ar5212GetRateTable(struct ath_hal *ah, u_int mode)
{
	HAL_RATE_TABLE *rt;
	switch (mode) {
	case HAL_MODE_11A:
		rt = &ar5212_11a_table;
		if (AH_PRIVATE(ah)->ah_curchan) {
			if (IS_CHAN_HALF_RATE(AH_PRIVATE(ah)->ah_curchan)) {
				rt = &ar5212_11a_half_table;
			} else if (IS_CHAN_QUARTER_RATE(
					AH_PRIVATE(ah)->ah_curchan)) {
				rt = &ar5212_11a_quarter_table;
			}
		}
		break;
	case HAL_MODE_11B:
		rt = &ar5212_11b_table;
		break;
	case HAL_MODE_11G:
#ifdef notdef
	case HAL_MODE_PUREG:
#endif
		rt =  &ar5212_11g_table;
		break;
	case HAL_MODE_TURBO:
	case HAL_MODE_108G:
		rt =  &ar5212_turbo_table;
		break;
	case HAL_MODE_XR:
		rt = &ar5212_xr_table;
		break;
	case HAL_MODE_11A_HALF_RATE:
		rt = &ar5212_11a_half_table;
		break;
	case HAL_MODE_11A_QUARTER_RATE:
		rt = &ar5212_11a_quarter_table;
		break;
	case HAL_MODE_11NG:
		rt = &ar5Owl_11ng_table;
		break;
	case HAL_MODE_11NA:
		rt = &ar5Owl_11na_table;
		break;
	default:
		HALDEBUG(ah, "%s: invalid mode 0x%x\n", __func__, mode);
		return AH_NULL;
	}
	ath_hal_setupratetable(ah, rt);
	return rt;
}
#endif /* AH_SUPPORT_AR5212 */
