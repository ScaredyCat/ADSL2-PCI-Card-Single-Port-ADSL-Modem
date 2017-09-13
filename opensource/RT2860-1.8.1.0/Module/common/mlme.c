/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	mlme.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	John Chang	2004-08-25		Modify from RT2500 code base
	John Chang	2004-09-06		modified for RT2600
*/

#include "rt_config.h"
#include <stdarg.h>

UCHAR	CISCO_OUI[] = {0x00, 0x40, 0x96};

UCHAR	WPA_OUI[] = {0x00, 0x50, 0xf2, 0x01};
UCHAR	RSN_OUI[] = {0x00, 0x0f, 0xac};
UCHAR   WME_INFO_ELEM[]  = {0x00, 0x50, 0xf2, 0x02, 0x00, 0x01};
UCHAR   WME_PARM_ELEM[] = {0x00, 0x50, 0xf2, 0x02, 0x01, 0x01};
UCHAR	Ccx2QosInfo[] = {0x00, 0x40, 0x96, 0x04};
UCHAR   RALINK_OUI[]  = {0x00, 0x0c, 0x43};
UCHAR   BROADCOM_OUI[]  = {0x00, 0x90, 0x4c};
UCHAR   WPS_OUI[] = {0x00, 0x50, 0xf2, 0x04};

UCHAR RateSwitchTable[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x11, 0x00,  0,  0,  0,						// Initial used item after association
    0x00, 0x00,  0, 40, 101,
    0x01, 0x00,  1, 40, 50,
    0x02, 0x00,  2, 35, 45,
    0x03, 0x00,  3, 20, 45,
    0x04, 0x21,  0, 30, 50,
    0x05, 0x21,  1, 20, 50,
    0x06, 0x21,  2, 20, 50,
    0x07, 0x21,  3, 15, 50,
    0x08, 0x21,  4, 15, 30,
    0x09, 0x21,  5, 10, 25,
    0x0a, 0x21,  6,  8, 25,
    0x0b, 0x21,  7,  8, 25,
    0x0c, 0x20, 12,  15, 30,
    0x0d, 0x20, 13,  8, 20,
    0x0e, 0x20, 14,  8, 20,
    0x0f, 0x20, 15,  8, 25,
    0x10, 0x22, 15,  8, 25,
    0x11, 0x00,  0,  0,  0,
    0x12, 0x00,  0,  0,  0,
    0x13, 0x00,  0,  0,  0,
    0x14, 0x00,  0,  0,  0,
    0x15, 0x00,  0,  0,  0,
    0x16, 0x00,  0,  0,  0,
    0x17, 0x00,  0,  0,  0,
    0x18, 0x00,  0,  0,  0,
    0x19, 0x00,  0,  0,  0,
    0x1a, 0x00,  0,  0,  0,
    0x1b, 0x00,  0,  0,  0,
    0x1c, 0x00,  0,  0,  0,
    0x1d, 0x00,  0,  0,  0,
    0x1e, 0x00,  0,  0,  0,
    0x1f, 0x00,  0,  0,  0,
};

UCHAR RateSwitchTable11B[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x04, 0x03,  0,  0,  0,						// Initial used item after association
    0x00, 0x00,  0, 40, 101,
    0x01, 0x00,  1, 40, 50,
    0x02, 0x00,  2, 35, 45,
    0x03, 0x00,  3, 20, 45,
};

UCHAR RateSwitchTable11BG[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x0a, 0x00,  0,  0,  0,						// Initial used item after association
    0x00, 0x00,  0, 40, 101,
    0x01, 0x00,  1, 40, 50,
    0x02, 0x00,  2, 35, 45,
    0x03, 0x00,  3, 20, 45,
    0x04, 0x10,  2, 20, 35,
    0x05, 0x10,  3, 16, 35,
    0x06, 0x10,  4, 10, 25,
    0x07, 0x10,  5, 16, 25,
    0x08, 0x10,  6, 10, 25,
    0x09, 0x10,  7, 10, 13,
};

UCHAR RateSwitchTable11G[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x08, 0x00,  0,  0,  0,						// Initial used item after association
    0x00, 0x10,  0, 20, 101,
    0x01, 0x10,  1, 20, 35,
    0x02, 0x10,  2, 20, 35,
    0x03, 0x10,  3, 16, 35,
    0x04, 0x10,  4, 10, 25,
    0x05, 0x10,  5, 16, 25,
    0x06, 0x10,  6, 10, 25,
    0x07, 0x10,  7, 10, 13,
};

UCHAR RateSwitchTable11N1S[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x09, 0x00,  0,  0,  0,						// Initial used item after association
    0x00, 0x21,  0, 30, 101,
    0x01, 0x21,  1, 20, 50,
    0x02, 0x21,  2, 20, 50,
    0x03, 0x21,  3, 15, 50,
    0x04, 0x21,  4, 15, 30,
    0x05, 0x21,  5, 10, 25,
    0x06, 0x21,  6,  8, 14,
    0x07, 0x21,  7,  8, 14,
    0x08, 0x23,  7,  8, 14,
};

UCHAR RateSwitchTable11N2S[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x0a, 0x00,  0,  0,  0,      // Initial used item after association
    0x00, 0x21,  0, 30, 101,
    0x01, 0x21,  1, 20, 50,
    0x02, 0x21,  2, 20, 50,
    0x03, 0x21,  3, 15, 50,
    0x04, 0x21,  4, 15, 30,
    0x05, 0x20, 12,  15, 30,
    0x06, 0x20, 13,  8, 20,
    0x07, 0x20, 14,  8, 20,
    0x08, 0x20, 15,  8, 25,
    0x09, 0x22, 15,  8, 25,
};

UCHAR RateSwitchTable11N2SForABand[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x0b, 0x09,  0,  0,  0,						// Initial used item after association
    0x00, 0x21,  0, 30, 101,
    0x01, 0x21,  1, 20, 50,
    0x02, 0x21,  2, 20, 50,
    0x03, 0x21,  3, 15, 50,
    0x04, 0x21,  4, 15, 30,
    0x05, 0x21,  5, 15, 30,
    0x06, 0x20, 12,  15, 30,
    0x07, 0x20, 13,  8, 20,
    0x08, 0x20, 14,  8, 20,
    0x09, 0x20, 15,  8, 25,
    0x0a, 0x22, 15,  8, 25,
};

UCHAR RateSwitchTable11BGN1S[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x0d, 0x00,  0,  0,  0,						// Initial used item after association
    0x00, 0x00,  0, 40, 101,
    0x01, 0x00,  1, 40, 50,
    0x02, 0x00,  2, 35, 45,
    0x03, 0x00,  3, 20, 45,
    0x04, 0x21,  0, 30,101,	//50
    0x05, 0x21,  1, 20, 50,
    0x06, 0x21,  2, 20, 50,
    0x07, 0x21,  3, 15, 50,
    0x08, 0x21,  4, 15, 30,
    0x09, 0x21,  5, 10, 25,
    0x0a, 0x21,  6,  8, 14,
    0x0b, 0x21,  7,  8, 14,
	0x0c, 0x23,  7,  8, 14,
};

UCHAR RateSwitchTable11BGN2S[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x0a, 0x00,  0,  0,  0,						// Initial used item after association
    0x00, 0x21,  0, 30,101,	//50
    0x01, 0x21,  1, 20, 50,
    0x02, 0x21,  2, 20, 50,
    0x03, 0x21,  3, 15, 50,
    0x04, 0x21,  4, 15, 30,
    0x05, 0x20, 12, 15, 30,
    0x06, 0x20, 13,  8, 20,
    0x07, 0x20, 14,  8, 20,
    0x08, 0x20, 15,  8, 25,
    0x09, 0x22, 15,  8, 25,
};

UCHAR RateSwitchTable11BGN2SForABand[] = {
// Item No.   Mode   Curr-MCS   TrainUp   TrainDown		// Mode- Bit0: STBC, Bit1: Short GI, Bit4,5: Mode(0:CCK, 1:OFDM, 2:HT Mix, 3:HT GF)
    0x0b, 0x09,  0,  0,  0,						// Initial used item after association
    0x00, 0x21,  0, 30,101,	//50
    0x01, 0x21,  1, 20, 50,
    0x02, 0x21,  2, 20, 50,
    0x03, 0x21,  3, 15, 50,
    0x04, 0x21,  4, 15, 30,
    0x05, 0x21,  5, 15, 30,
    0x06, 0x20, 12, 15, 30,
    0x07, 0x20, 13,  8, 20,
    0x08, 0x20, 14,  8, 20,
    0x09, 0x20, 15,  8, 25,
    0x0a, 0x22, 15,  8, 25,
};

PUCHAR ReasonString[] = {
	/* 0  */	 "Reserved",
	/* 1  */	 "Unspecified Reason",
	/* 2  */	 "Previous Auth no longer valid",
	/* 3  */	 "STA is leaving / has left",
	/* 4  */	 "DIS-ASSOC due to inactivity",
	/* 5  */	 "AP unable to hanle all associations",
	/* 6  */	 "class 2 error",
	/* 7  */	 "class 3 error",
	/* 8  */	 "STA is leaving / has left",
	/* 9  */	 "require auth before assoc/re-assoc",
	/* 10 */	 "Reserved",
	/* 11 */	 "Reserved",
	/* 12 */	 "Reserved",
	/* 13 */	 "invalid IE",
	/* 14 */	 "MIC error",
	/* 15 */	 "4-way handshake timeout",
	/* 16 */	 "2-way (group key) handshake timeout",
	/* 17 */	 "4-way handshake IE diff among AssosReq/Rsp/Beacon",
	/* 18 */
};

extern UCHAR	 OfdmRateToRxwiMCS[];
// since RT61 has better RX sensibility, we have to limit TX ACK rate not to exceed our normal data TX rate.
// otherwise the WLAN peer may not be able to receive the ACK thus downgrade its data TX rate
ULONG BasicRateMask[12]				= {0xfffff001 /* 1-Mbps */, 0xfffff003 /* 2 Mbps */, 0xfffff007 /* 5.5 */, 0xfffff00f /* 11 */,
									  0xfffff01f /* 6 */	 , 0xfffff03f /* 9 */	  , 0xfffff07f /* 12 */ , 0xfffff0ff /* 18 */,
									  0xfffff1ff /* 24 */	 , 0xfffff3ff /* 36 */	  , 0xfffff7ff /* 48 */ , 0xffffffff /* 54 */};

UCHAR MULTICAST_ADDR[MAC_ADDR_LEN] = {0x1,  0x00, 0x00, 0x00, 0x00, 0x00};
UCHAR BROADCAST_ADDR[MAC_ADDR_LEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
UCHAR ZERO_MAC_ADDR[MAC_ADDR_LEN]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// e.g. RssiSafeLevelForTxRate[RATE_36]" means if the current RSSI is greater than 
//		this value, then it's quaranteed capable of operating in 36 mbps TX rate in 
//		clean environment.
//								  TxRate: 1   2   5.5	11	 6	  9    12	18	 24   36   48	54	 72  100
CHAR RssiSafeLevelForTxRate[] ={  -92, -91, -90, -87, -88, -86, -85, -83, -81, -78, -72, -71, -40, -40 };

UCHAR  RateIdToMbps[]	 = { 1, 2, 5, 11, 6, 9, 12, 18, 24, 36, 48, 54, 72, 100};
USHORT RateIdTo500Kbps[] = { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108, 144, 200};

UCHAR  SsidIe	 = IE_SSID;
UCHAR  SupRateIe = IE_SUPP_RATES;
UCHAR  ExtRateIe = IE_EXT_SUPP_RATES;
UCHAR  HtCapIe = IE_HT_CAP;
UCHAR  AddHtInfoIe = IE_ADD_HT;
UCHAR  NewExtChanIe = IE_NEW_EXT_CHA_OFFSET;
UCHAR  ErpIe	 = IE_ERP;
UCHAR  DsIe 	 = IE_DS_PARM;
UCHAR  TimIe	 = IE_TIM;
UCHAR  WpaIe	 = IE_WPA;
UCHAR  Wpa2Ie	 = IE_WPA2;
UCHAR  IbssIe	 = IE_IBSS_PARM;
UCHAR  Ccx2Ie	 = IE_CCX_V2;

extern UCHAR	WPA_OUI[];

UCHAR	SES_OUI[] = {0x00, 0x90, 0x4c};

UCHAR	ZeroSsid[32] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// Reset the RFIC setting to new series    
RTMP_RF_REGS RF2850RegTable[] = {
//		ch	 R1 		 R2 		 R3(TX0~4=0) R4
		{1,  0x98402ecc, 0x984c0786, 0x9816b455, 0x9800510b},
		{2,  0x98402ecc, 0x984c0786, 0x98168a55, 0x9800519f},
		{3,  0x98402ecc, 0x984c078a, 0x98168a55, 0x9800518b},
		{4,  0x98402ecc, 0x984c078a, 0x98168a55, 0x9800519f},
		{5,  0x98402ecc, 0x984c078e, 0x98168a55, 0x9800518b},
		{6,  0x98402ecc, 0x984c078e, 0x98168a55, 0x9800519f},
		{7,  0x98402ecc, 0x984c0792, 0x98168a55, 0x9800518b},
		{8,  0x98402ecc, 0x984c0792, 0x98168a55, 0x9800519f},
		{9,  0x98402ecc, 0x984c0796, 0x98168a55, 0x9800518b},
		{10, 0x98402ecc, 0x984c0796, 0x98168a55, 0x9800519f},
		{11, 0x98402ecc, 0x984c079a, 0x98168a55, 0x9800518b},
		{12, 0x98402ecc, 0x984c079a, 0x98168a55, 0x9800519f},
		{13, 0x98402ecc, 0x984c079e, 0x98168a55, 0x9800518b},
		{14, 0x98402ecc, 0x984c07a2, 0x98168a55, 0x98005193},

		// 802.11 UNI / HyperLan 2
		{36, 0x98402ecc, 0x984c099a, 0x98158a55, 0x980ed1a3},
		{38, 0x98402ecc, 0x984c099e, 0x98158a55, 0x980ed193},
		{40, 0x98402ec8, 0x984c0682, 0x98158a55, 0x980ed183},
		{44, 0x98402ec8, 0x984c0682, 0x98158a55, 0x980ed1a3},
		{46, 0x98402ec8, 0x984c0686, 0x98158a55, 0x980ed18b},
		{48, 0x98402ec8, 0x984c0686, 0x98158a55, 0x980ed19b},
		{52, 0x98402ec8, 0x984c068a, 0x98158a55, 0x980ed193},
		{54, 0x98402ec8, 0x984c068a, 0x98158a55, 0x980ed1a3},
		{56, 0x98402ec8, 0x984c068e, 0x98158a55, 0x980ed18b},
		{60, 0x98402ec8, 0x984c0692, 0x98158a55, 0x980ed183},
		{62, 0x98402ec8, 0x984c0692, 0x98158a55, 0x980ed193},
		{64, 0x98402ec8, 0x984c0692, 0x98158a55, 0x980ed1a3}, // Plugfest#4, Day4, change RFR3 left4th 9->5.

		// 802.11 HyperLan 2
		{100, 0x98402ec8, 0x984c06b2, 0x98178a55, 0x980ed783},
		{102, 0x98402ec8, 0x984c06b2, 0x98178a55, 0x980ed793},
		{104, 0x98402ec8, 0x984c06b2, 0x98178a55, 0x980ed1a3},
		{108, 0x98402ecc, 0x984c0a32, 0x98178a55, 0x980ed193},
		{110, 0x98402ecc, 0x984c0a36, 0x98178a55, 0x980ed183},
		{112, 0x98402ecc, 0x984c0a36, 0x98178a55, 0x980ed19b},
		{116, 0x98402ecc, 0x984c0a3a, 0x98178a55, 0x980ed1a3},
		{118, 0x98402ecc, 0x984c0a3e, 0x98178a55, 0x980ed193},
		{120, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed183},
		{124, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed193},
		{126, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed15b}, // 0x980ed1bb->0x980ed15b required by Rory 20070927
		{128, 0x98402ec4, 0x984c0382, 0x98178a55, 0x980ed1a3},
		{132, 0x98402ec4, 0x984c0386, 0x98178a55, 0x980ed18b},
		{134, 0x98402ec4, 0x984c0386, 0x98178a55, 0x980ed193},
		{136, 0x98402ec4, 0x984c0386, 0x98178a55, 0x980ed19b},
		{140, 0x98402ec4, 0x984c038a, 0x98178a55, 0x980ed183},

		// 802.11 UNII
		{149, 0x98402ec4, 0x984c038a, 0x98178a55, 0x980ed1a7},
		{151, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed187},
		{153, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed18f},
		{157, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed19f},
		{159, 0x98402ec4, 0x984c038e, 0x98178a55, 0x980ed1a7},
		{161, 0x98402ec4, 0x984c0392, 0x98178a55, 0x980ed187},
		{165, 0x98402ec4, 0x984c0392, 0x98178a55, 0x980ed197},

		// Japan
		{184, 0x95002ccc, 0x9500491e, 0x9509be55, 0x950c0a0b},
		{188, 0x95002ccc, 0x95004922, 0x9509be55, 0x950c0a13},
		{192, 0x95002ccc, 0x95004926, 0x9509be55, 0x950c0a1b},
		{196, 0x95002ccc, 0x9500492a, 0x9509be55, 0x950c0a23},
		{208, 0x95002ccc, 0x9500493a, 0x9509be55, 0x950c0a13},
		{212, 0x95002ccc, 0x9500493e, 0x9509be55, 0x950c0a1b},
		{216, 0x95002ccc, 0x95004982, 0x9509be55, 0x950c0a23},

		// still lack of MMAC(Japan) ch 34,38,42,46
};
UCHAR	NUM_OF_2850_CHNL = (sizeof(RF2850RegTable) / sizeof(RTMP_RF_REGS));

/*
	==========================================================================
	Description:
		initialize the MLME task and its data structure (queue, spinlock, 
		timer, state machines).

	IRQL = PASSIVE_LEVEL

	Return:
		always return NDIS_STATUS_SUCCESS

	==========================================================================
*/
NDIS_STATUS MlmeInit(
	IN PRTMP_ADAPTER pAd) 
{
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

	DBGPRINT(RT_DEBUG_TRACE, ("--> MLME Initialize\n"));

	do 
	{
		Status = MlmeQueueInit(&pAd->Mlme.Queue);
		if(Status != NDIS_STATUS_SUCCESS) 
			break;

		pAd->Mlme.bRunning = FALSE;
		NdisAllocateSpinLock(&pAd->Mlme.TaskLock);

		
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			// init AP state machines
			APAssocStateMachineInit(pAd, &pAd->Mlme.ApAssocMachine, pAd->Mlme.ApAssocFunc);
			APAuthStateMachineInit(pAd, &pAd->Mlme.ApAuthMachine, pAd->Mlme.ApAuthFunc);
			APAuthRspStateMachineInit(pAd, &pAd->Mlme.ApAuthRspMachine, pAd->Mlme.ApAuthRspFunc);
			APSyncStateMachineInit(pAd, &pAd->Mlme.ApSyncMachine, pAd->Mlme.ApSyncFunc);
			APWpaStateMachineInit(pAd, &pAd->Mlme.ApWpaMachine, pAd->Mlme.ApWpaFunc);

#ifdef WSC_AP_SUPPORT
	        // Init Wsc state machine		
			ASSERT(WSC_FUNC_SIZE == MAX_WSC_MSG * MAX_WSC_STATE);
			WscStateMachineInit(pAd, &pAd->Mlme.WscMachine, pAd->Mlme.WscFunc);
#endif // WSC_AP_SUPPORT //
	        
#ifdef APCLI_SUPPORT
			// init apcli state machines
			ASSERT(APCLI_AUTH_FUNC_SIZE == APCLI_MAX_AUTH_MSG * APCLI_MAX_AUTH_STATE);
			ApCliAuthStateMachineInit(pAd, &pAd->Mlme.ApCliAuthMachine, pAd->Mlme.ApCliAuthFunc);

			ASSERT(APCLI_ASSOC_FUNC_SIZE == APCLI_MAX_ASSOC_MSG * APCLI_MAX_ASSOC_STATE);
			ApCliAssocStateMachineInit(pAd, &pAd->Mlme.ApCliAssocMachine, pAd->Mlme.ApCliAssocFunc);

			ASSERT(APCLI_SYNC_FUNC_SIZE == APCLI_MAX_SYNC_MSG * APCLI_MAX_SYNC_STATE);
			ApCliSyncStateMachineInit(pAd, &pAd->Mlme.ApCliSyncMachine, pAd->Mlme.ApCliSyncFunc);

			ASSERT(APCLI_CTRL_FUNC_SIZE == APCLI_MAX_CTRL_MSG * APCLI_MAX_CTRL_STATE);
			ApCliCtrlStateMachineInit(pAd, &pAd->Mlme.ApCliCtrlMachine, pAd->Mlme.ApCliCtrlFunc);

#endif // APCLI_SUPPORT //
		}

#endif // CONFIG_AP_SUPPORT //

#ifdef MESH_SUPPORT
		ASSERT(MESH_CTRL_FUNC_SIZE == MESH_CTRL_MAX_EVENTS * MESH_CTRL_MAX_STATES);
		MeshCtrlStateMachineInit(pAd, &pAd->Mlme.MeshCtrlMachine, pAd->Mlme.MeshCtrlFunc);

		ASSERT(MESH_LINK_MNG_FUNC_SIZE == MESH_LINK_MNG_MAX_EVENTS * MESH_LINK_MNG_MAX_STATES);
		MeshLinkMngStateMachineInit(pAd, &pAd->Mlme.MeshLinkMngMachine, pAd->Mlme.MeshLinkMngFunc);
#endif // MESH_SUPPORT //

		ActionStateMachineInit(pAd, &pAd->Mlme.ActMachine, pAd->Mlme.ActFunc);

		// Init QuickResponseForRateUp timer.
		RTMPInitTimer(pAd, &pAd->CommonCfg.QuickResponeForRateUpTimerEx, GET_TIMER_FUNCTION(QuickResponeForRateUpExecEx), pAd, FALSE);
		pAd->CommonCfg.QuickResponeForRateUpTimerRunningEx = FALSE;

		// Init mlme periodic timer
		RTMPInitTimer(pAd, &pAd->Mlme.PeriodicTimer, GET_TIMER_FUNCTION(MlmePeriodicExec), pAd, TRUE);

		// Set mlme periodic timer
		RTMPSetTimer(&pAd->Mlme.PeriodicTimer, MLME_TASK_EXEC_INTV);

		// software-based RX Antenna diversity
		RTMPInitTimer(pAd, &pAd->Mlme.RxAntEvalTimer, GET_TIMER_FUNCTION(AsicRxAntEvalTimeout), pAd, FALSE);

#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			// Init APSD periodic timer
			RTMPInitTimer(pAd, &pAd->Mlme.APSDPeriodicTimer, GET_TIMER_FUNCTION(APSDPeriodicExec), pAd, TRUE);
			RTMPSetTimer(&pAd->Mlme.APSDPeriodicTimer, 50);
		}
#endif // CONFIG_AP_SUPPORT //


	} while (FALSE);

	DBGPRINT(RT_DEBUG_TRACE, ("<-- MLME Initialize\n"));

	return Status;
}

/*
	==========================================================================
	Description:
		main loop of the MLME
	Pre:
		Mlme has to be initialized, and there are something inside the queue
	Note:
		This function is invoked from MPSetInformation and MPReceive;
		This task guarantee only one MlmeHandler will run. 

	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID MlmeHandler(
	IN PRTMP_ADAPTER pAd) 
{
	MLME_QUEUE_ELEM 	   *Elem = NULL;
#ifdef APCLI_SUPPORT
	SHORT apcliIfIndex;
#endif

	// Only accept MLME and Frame from peer side, no other (control/data) frame should
	// get into this state machine

	NdisAcquireSpinLock(&pAd->Mlme.TaskLock);
	if(pAd->Mlme.bRunning) 
	{
		NdisReleaseSpinLock(&pAd->Mlme.TaskLock);
		return;
	} 
	else 
	{
		pAd->Mlme.bRunning = TRUE;
	}
	NdisReleaseSpinLock(&pAd->Mlme.TaskLock);

	while (!MlmeQueueEmpty(&pAd->Mlme.Queue)) 
	{
		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_MLME_RESET_IN_PROGRESS) ||
			RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS) ||
			RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Device Halted or Removed or MlmeRest, exit MlmeHandler! (queue num = %ld)\n", pAd->Mlme.Queue.Num));
			break;
		}
		
#ifdef RALINK_ATE			
		if(pAd->ate.Mode != ATE_STOP)
		{
			DBGPRINT(RT_DEBUG_TRACE, ("The driver is in ATE mode now in MlmeHandler\n"));
			break;
		}	
#endif // RALINK_ATE //

		//From message type, determine which state machine I should drive
		if (MlmeDequeue(&pAd->Mlme.Queue, &Elem)) 
		{

			// if dequeue success
			switch (Elem->Machine) 
			{
				// STA state machines
#ifdef	CONFIG_STA_SUPPORT
				case ASSOC_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.AssocMachine, Elem);
					break;
				case AUTH_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.AuthMachine, Elem);
					break;
				case AUTH_RSP_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.AuthRspMachine, Elem);
					break;
				case SYNC_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.SyncMachine, Elem);
					break;
				case MLME_CNTL_STATE_MACHINE:
					MlmeCntlMachinePerformAction(pAd, &pAd->Mlme.CntlMachine, Elem);
					break;
				case WPA_PSK_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.WpaPskMachine, Elem);
					break;	
#ifdef LEAP_SUPPORT
				case LEAP_STATE_MACHINE:
					LeapMachinePerformAction(pAd, &pAd->Mlme.LeapMachine, Elem);
					break;
#endif
				case AIRONET_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.AironetMachine, Elem);
					break;

						
#endif // CONFIG_STA_SUPPORT //						

				case ACTION_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.ActMachine, Elem);
					break;	

#ifdef CONFIG_AP_SUPPORT
				// AP state amchines

				case AP_ASSOC_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.ApAssocMachine, Elem);
					break;
				case AP_AUTH_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.ApAuthMachine, Elem);
					break;
				case AP_AUTH_RSP_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.ApAuthRspMachine, Elem);
					break;
				case AP_SYNC_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.ApSyncMachine, Elem);
					break;
				case AP_WPA_STATE_MACHINE:
					StateMachinePerformAction(pAd, &pAd->Mlme.ApWpaMachine, Elem);
					break;

#ifdef APCLI_SUPPORT
				case APCLI_AUTH_STATE_MACHINE:
					apcliIfIndex = Elem->Idx;
					if(isValidApCliIf(apcliIfIndex))
						StateMachinePerformActionEx(pAd, &pAd->Mlme.ApCliAuthMachine, Elem,
							apcliIfIndex, &(pAd->ApCfg.ApCliTab[apcliIfIndex].AuthCurrState));
					break;

				case APCLI_ASSOC_STATE_MACHINE:
					apcliIfIndex = Elem->Idx;
					if(isValidApCliIf(apcliIfIndex))		
						StateMachinePerformActionEx(pAd, &pAd->Mlme.ApCliAssocMachine, Elem,
							apcliIfIndex, &(pAd->ApCfg.ApCliTab[apcliIfIndex].AssocCurrState));
					break;

				case APCLI_SYNC_STATE_MACHINE:
					apcliIfIndex = Elem->Idx;
					if(isValidApCliIf(apcliIfIndex))
						StateMachinePerformActionEx(pAd, &pAd->Mlme.ApCliSyncMachine, Elem,
							apcliIfIndex, &(pAd->ApCfg.ApCliTab[apcliIfIndex].SyncCurrState));
					break;

				case APCLI_CTRL_STATE_MACHINE:
					apcliIfIndex = Elem->Idx;
					if(isValidApCliIf(apcliIfIndex))
						StateMachinePerformActionEx(pAd, &pAd->Mlme.ApCliCtrlMachine, Elem,
							apcliIfIndex, &(pAd->ApCfg.ApCliTab[apcliIfIndex].CtrlCurrState));
					break;
#if 0	// use AP_WPA_STATE_MACHINE. It's the same with AP.
				case APCLI_WPA_STATE_MACHINE:
					apcliIfIndex = Elem->ifIndex;
					if(isValidApCliIf(apcliIfIndex))
						StateMachinePerformActionEx(pAd, &pAd->Mlme.ApCliWpaPskMachine, Elem,
							apcliIfIndex, &(pAd->ApCfg.ApCliTab[apcliIfIndex].WpaPskCurrState));
					break;	
#endif // end - 0 //					
#endif // APCLI_SUPPORT //

#endif // CONFIG_AP_SUPPORT //

#ifdef WSC_INCLUDED
                case WSC_STATE_MACHINE:
#ifdef CONFIG_AP_SUPPORT                    
					IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
                    {
                        WpsApSmProcess(pAd, Elem);
                    }
#endif // CONFIG_AP_SUPPORT //                    
                    break;
#endif // WSC_INCLUDED //

#ifdef MESH_SUPPORT
				case MESH_CTRL_STATE_MACHINE:
					StateMachinePerformActionEx(pAd, &pAd->Mlme.MeshCtrlMachine, Elem,
							0, (PULONG)&(pAd->MeshTab.CtrlCurrentState));
					break;

				case MESH_LINK_MNG_STATE_MACHINE:
					StateMachinePerformActionEx(pAd, &pAd->Mlme.MeshLinkMngMachine, Elem,
							Elem->Idx, (PULONG)&(pAd->MeshTab.MeshLink[Elem->Idx].CurrentState));
					break;
#endif // MESH_SUPPORT //

				default:
					DBGPRINT(RT_DEBUG_TRACE, ("ERROR: Illegal machine %ld in MlmeHandler()\n", Elem->Machine));
					break;
			} // end of switch

			// free MLME element
			Elem->Occupied = FALSE;
			Elem->MsgLen = 0;

		}
		else {
			DBGPRINT_ERR(("MlmeHandler: MlmeQueue empty\n"));
		}
	}

	NdisAcquireSpinLock(&pAd->Mlme.TaskLock);
	pAd->Mlme.bRunning = FALSE;
	NdisReleaseSpinLock(&pAd->Mlme.TaskLock);
}

/*
	==========================================================================
	Description:
		Destructor of MLME (Destroy queue, state machine, spin lock and timer)
	Parameters:
		Adapter - NIC Adapter pointer
	Post:
		The MLME task will no longer work properly

	IRQL = PASSIVE_LEVEL

	==========================================================================
 */
VOID MlmeHalt(
	IN PRTMP_ADAPTER pAd) 
{
	BOOLEAN 	  Cancelled;

	DBGPRINT(RT_DEBUG_TRACE, ("==> MlmeHalt\n"));

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
	{
		// disable BEACON generation and other BEACON related hardware timers
		AsicDisableSync(pAd);
	}

	if (pAd->CommonCfg.QuickResponeForRateUpTimerRunningEx == TRUE)
		RTMPCancelTimer(&pAd->CommonCfg.QuickResponeForRateUpTimerEx, &Cancelled);


	RTMPCancelTimer(&pAd->Mlme.PeriodicTimer,		&Cancelled);
	RTMPCancelTimer(&pAd->Mlme.RxAntEvalTimer,		&Cancelled);

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		RTMPCancelTimer(&pAd->Mlme.APSDPeriodicTimer,	&Cancelled);

#ifdef APCLI_SUPPORT
		RTMPCancelTimer(&pAd->MlmeAux.ProbeTimer, &Cancelled);
		RTMPCancelTimer(&pAd->MlmeAux.ApCliAssocTimer, &Cancelled);
		RTMPCancelTimer(&pAd->MlmeAux.ApCliAuthTimer, &Cancelled);
#endif // APCLI_SUPPORT //
	}

#endif // CONFIG_AP_SUPPORT //

#ifdef MESH_SUPPORT
	MeshHalt(pAd);
#endif // MESH_SUPPORT //

	if (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_NIC_NOT_EXIST))
	{
		// Set LED
		RTMPSetLED(pAd, LED_HALT);
        RTMPSetSignalLED(pAd, -100);	// Force signal strength Led to be turned off, firmware is not done it.
	}

	RTMPusecDelay(5000);    //  5 msec to gurantee Ant Diversity timer canceled

	MlmeQueueDestroy(&pAd->Mlme.Queue);
	NdisFreeSpinLock(&pAd->Mlme.TaskLock);

	DBGPRINT(RT_DEBUG_TRACE, ("<== MlmeHalt\n"));
}

VOID MlmeResetRalinkCounters(
	IN  PRTMP_ADAPTER   pAd)
{
	pAd->RalinkCounters.LastOneSecRxOkDataCnt = pAd->RalinkCounters.OneSecRxOkDataCnt;
	// clear all OneSecxxx counters.
	pAd->RalinkCounters.OneSecBeaconSentCnt = 0;
	pAd->RalinkCounters.OneSecFalseCCACnt = 0;
	pAd->RalinkCounters.OneSecRxFcsErrCnt = 0;
	pAd->RalinkCounters.OneSecRxOkCnt = 0;
	pAd->RalinkCounters.OneSecTxFailCount = 0;
	pAd->RalinkCounters.OneSecTxNoRetryOkCount = 0;
	pAd->RalinkCounters.OneSecTxRetryOkCount = 0;
	pAd->RalinkCounters.OneSecRxOkDataCnt = 0;

	// TODO: for debug only. to be removed
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BE] = 0;
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_BK] = 0;
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VI] = 0;
	pAd->RalinkCounters.OneSecOsTxCount[QID_AC_VO] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_BE] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_BK] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_VI] = 0;
	pAd->RalinkCounters.OneSecDmaDoneCount[QID_AC_VO] = 0;
	pAd->RalinkCounters.OneSecTxDoneCount = 0;
	pAd->RalinkCounters.OneSecRxCount = 0;
	pAd->RalinkCounters.OneSecTxAggregationCount = 0;
	pAd->RalinkCounters.OneSecRxAggregationCount = 0;

	return;
}

unsigned long rx_AMSDU;
unsigned long rx_Total;

/*
	==========================================================================
	Description:
		This routine is executed periodically to -
		1. Decide if it's a right time to turn on PwrMgmt bit of all 
		   outgoiing frames
		2. Calculate ChannelQuality based on statistics of the last
		   period, so that TX rate won't toggling very frequently between a 
		   successful TX and a failed TX.
		3. If the calculated ChannelQuality indicated current connection not 
		   healthy, then a ROAMing attempt is tried here.

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
#define ADHOC_BEACON_LOST_TIME		(8*OS_HZ)  // 8 sec
VOID MlmePeriodicExec(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3) 
{
	ULONG			TxTotalCnt;
	PRTMP_ADAPTER	pAd = (RTMP_ADAPTER *)FunctionContext;


	// Do nothing if the driver is starting halt state.
	// This might happen when timer already been fired before cancel timer with mlmehalt
	if ((RTMP_TEST_FLAG(pAd, (fRTMP_ADAPTER_HALT_IN_PROGRESS |
								fRTMP_ADAPTER_RADIO_OFF |
								fRTMP_ADAPTER_RADIO_MEASUREMENT |
								fRTMP_ADAPTER_RESET_IN_PROGRESS))))
		return;
	
	RT28XX_MLME_PRE_SANITY_CHECK(pAd);

#ifdef RALINK_ATE
	/* Do not show RSSI until "Normal 1 second Mlme PeriodicExec". */
	if (pAd->ate.Mode != ATE_STOP)
	{
		if (pAd->Mlme.PeriodicRound % MLME_TASK_EXEC_MULTIPLE != (MLME_TASK_EXEC_MULTIPLE - 1))
	{
			pAd->Mlme.PeriodicRound ++;
			return;
		}
	}
#endif // RALINK_ATE //


	pAd->bUpdateBcnCntDone = FALSE;
	
//	RECBATimerTimeout(SystemSpecific1,FunctionContext,SystemSpecific2,SystemSpecific3);
	pAd->Mlme.PeriodicRound ++;

	// execute every 500ms 
	if ((pAd->Mlme.PeriodicRound % 5 == 0) && RTMPAutoRateSwitchCheck(pAd)/*(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))*/)
	{
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			APMlmeDynamicTxRateSwitching(pAd);
#endif // CONFIG_AP_SUPPORT //
	}

	// Normal 1 second Mlme PeriodicExec.
	if (pAd->Mlme.PeriodicRound %MLME_TASK_EXEC_MULTIPLE == 0)
	{
                pAd->Mlme.OneSecPeriodicRound ++;

#ifdef RALINK_ATE
    	if (pAd->ate.Mode != ATE_STOP)
    	{
			/* request from Baron : move this routine from later to here */
			/* for showing Rx error count in ATE RXFRAME */
            NICUpdateRawCounters(pAd);
			if (pAd->ate.bRxFer == 1)
			{
				pAd->ate.RxTotalCnt += pAd->ate.RxCntPerSec;
			    ate_print(KERN_EMERG "MlmePeriodicExec: Rx packet cnt = %d/%d\n", pAd->ate.RxCntPerSec, pAd->ate.RxTotalCnt);
				pAd->ate.RxCntPerSec = 0;

				if (pAd->ate.RxAntennaSel == 0)
					ate_print(KERN_EMERG "MlmePeriodicExec: Rx AvgRssi0=%d, AvgRssi1=%d, AvgRssi2=%d\n\n",
						pAd->ate.AvgRssi0, pAd->ate.AvgRssi1, pAd->ate.AvgRssi2);
				else
					ate_print(KERN_EMERG "MlmePeriodicExec: Rx AvgRssi=%d\n\n", pAd->ate.AvgRssi0);
			}
			MlmeResetRalinkCounters(pAd);
			return;
    	}
#endif // RALINK_ATE //

#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			dynamic_tune_be_tx_op(pAd, 50);	// change form 100 to 50 for WMM WiFi test @20070504
#endif // CONFIG_AP_SUPPORT //

#if 0
		if ((pAd->Mlme.OneSecPeriodicRound %3) == 0)
		{
			{
				printk("%03d/%03d/%03d/%03d\n",
					   pAd->TxSwQueue[0].Number,
					   pAd->TxSwQueue[1].Number,
					   pAd->TxSwQueue[2].Number,
					   pAd->TxSwQueue[3].Number);
			}
		}
#endif			

		if (rx_Total)
		{
			DBGPRINT(RT_DEBUG_LOUD,("%lu/%lu (%lu%%) , TxDone %ld\n", rx_AMSDU, rx_Total,  (rx_AMSDU*100)/rx_Total, pAd->RalinkCounters.OneSecDmaDoneCount[0]));

			// reset counters
			rx_AMSDU = 0;
			rx_Total = 0;
		}

		//ORIBATimerTimeout(pAd);
		
		// Media status changed, report to NDIS
		if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_MEDIA_STATE_CHANGE)) 
		{
			RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_MEDIA_STATE_CHANGE);
			if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
			{
				pAd->IndicateMediaState = NdisMediaStateConnected;
#ifdef WIN_NDIS
				NdisMIndicateStatus(pAd->AdapterHandle, NDIS_STATUS_MEDIA_CONNECT, (PVOID)NULL, 0);
#else
				RTMP_IndicateMediaState();
#endif
					
			}
			else
			{
				pAd->IndicateMediaState = NdisMediaStateDisconnected;
#ifdef WIN_NDIS
				NdisMIndicateStatus(pAd->AdapterHandle, NDIS_STATUS_MEDIA_DISCONNECT, (PVOID)NULL, 0);
#else
				RTMP_IndicateMediaState();
#endif
			}
#ifdef WIN_NDIS			
			NdisMIndicateStatusComplete(pAd->AdapterHandle);			
#endif
		}

		NdisGetSystemUpTime(&pAd->Mlme.Now32);

		// add the most up-to-date h/w raw counters into software variable, so that
		// the dynamic tuning mechanism below are based on most up-to-date information
		NICUpdateRawCounters(pAd);																										


   		// Need statistics after read counter. So put after NICUpdateRawCounters
		ORIBATimerTimeout(pAd);


		// if MGMT RING is full more than twice within 1 second, we consider there's
		// a hardware problem stucking the TX path. In this case, try a hardware reset
		// to recover the system
	//	if (pAd->RalinkCounters.MgmtRingFullCount >= 2)
	//		RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_HARDWARE_ERROR);
	//	else
	//		pAd->RalinkCounters.MgmtRingFullCount = 0;

		// The time period for checking antenna is according to traffic
		if (pAd->Mlme.bEnableAutoAntennaCheck)
		{
			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
							 pAd->RalinkCounters.OneSecTxRetryOkCount + 
							 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt > 50)
			{
				if (pAd->Mlme.OneSecPeriodicRound % 10 == 0)
				{
					AsicEvaluateRxAnt(pAd);
				}
			}
			else
			{
				if (pAd->Mlme.OneSecPeriodicRound % 3 == 0)
				{
					AsicEvaluateRxAnt(pAd);
				}
			}
		}

#if 0
		if (pAd->CommonCfg.bRcvBSSWidthTriggerEvents)
		{
			if (RTMP_TIME_AFTER(pAd->Mlme.Now32, pAd->CommonCfg.LastRcvBSSWidthTriggerEventsTime+(30*60*OS_HZ)))
			{				
				pAd->CommonCfg.bRcvBSSWidthTriggerEvents = FALSE;
				pAd->CommonCfg.AddHTInfo.AddHtInfo.RecomWidth = 1;	
				pAd->CommonCfg.AddHTInfo.AddHtInfo.ExtChanOffset = pAd->CommonCfg.RegTransmitSetting.field.EXTCHA;
				//Recvery_to_Forty_Mhz(pAd);
			}
		}
#endif

#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			APMlmePeriodicExec(pAd);

			// The watchdog will prevent the hardware MAC into a deadlock condition 
			// 1. No beacon frame sent within 2 seconds and
			// 2. No site-survey processing
			// 3. AP has started up
			// 4. WDS isn't bridge mode
			// 5. No Carrier signal exist.
			if ((pAd->RalinkCounters.OneSecBeaconSentCnt == 0)
				&& (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
				&& (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
#ifdef WDS_SUPPORT
				&& (pAd->WdsTab.Mode != WDS_BRIDGE_MODE)		
#endif // WDS_SUPPORT //
#ifdef CARRIER_DETECTION_SUPPORT
				&& (isCarrierDetectExist(pAd) == FALSE)
#endif // CARRIER_DETECTION_SUPPORT //
				)
				pAd->watchDogMacDeadlock ++;
			else
				pAd->watchDogMacDeadlock = 0;

			// When the beacon deadlock occurred over 2 seconds, reset MAC/BBP.
			if (pAd->watchDogMacDeadlock > 1)
			{
				RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x1);
				RTMPusecDelay(1);
				RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0xC);

				DBGPRINT(RT_DEBUG_WARN,("Warning, MAC specific condition occurs \n"));
			}
		}
#endif // CONFIG_AP_SUPPORT //

		MlmeResetRalinkCounters(pAd);


		RT28XX_MLME_HANDLER(pAd);
	}

	pAd->bUpdateBcnCntDone = FALSE;
}

VOID MlmeSelectTxRateTableEx(
	IN PRTMP_ADAPTER		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN PUCHAR				*ppTable,
	IN PUCHAR				pTableSize,
	IN PUCHAR				pInitTxRateIdx)
{
	// decide the rate table for tuning
	if (pAd->CommonCfg.TxRateTableSize > 0)
	{
		*ppTable = RateSwitchTable;
		*pTableSize = RateSwitchTable[0];
		*pInitTxRateIdx = RateSwitchTable[1];
	}
	else
	{
		if ((pEntry->RateLen == 12) && (pEntry->HTCapability.MCSSet[0] == 0xff) &&
			((pEntry->HTCapability.MCSSet[1] == 0) || (/*pAd->Antenna.field.TxPath*/pAd->CommonCfg.TxStream == 1)))
		{// 11BGN 1S
			*ppTable = RateSwitchTable11BGN1S;
			*pTableSize = RateSwitchTable11BGN1S[0];
			*pInitTxRateIdx = RateSwitchTable11BGN1S[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11BGN 1S STA \n"));
		}
		else if ((pEntry->RateLen == 12) && (pEntry->HTCapability.MCSSet[0] == 0xff) &&
			(pEntry->HTCapability.MCSSet[1] == 0xff) && (/*pAd->Antenna.field.TxPath*/pAd->CommonCfg.TxStream == 2))
		{// 11BGN 2S
			if (pAd->LatchRfRegs.Channel <= 14)
			{
				*ppTable = RateSwitchTable11BGN2S;
				*pTableSize = RateSwitchTable11BGN2S[0];
				*pInitTxRateIdx = RateSwitchTable11BGN2S[1];
				DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11BGN 2S STA \n"));
			}
			else
			{
				*ppTable = RateSwitchTable11BGN2SForABand;
				*pTableSize = RateSwitchTable11BGN2SForABand[0];
				*pInitTxRateIdx = RateSwitchTable11BGN2SForABand[1];
                		DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11AN 2S STA \n"));
			}
		}
		else if ((pEntry->HTCapability.MCSSet[0] == 0xff) && ((pEntry->HTCapability.MCSSet[1] == 0) || (/*pAd->Antenna.field.TxPath*/pAd->CommonCfg.TxStream == 1)))
		{// 11N 1S
			*ppTable = RateSwitchTable11N1S;
			*pTableSize = RateSwitchTable11N1S[0];
			*pInitTxRateIdx = RateSwitchTable11N1S[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11N 1S STA \n"));
		}
		else if ((pEntry->HTCapability.MCSSet[0] == 0xff) && (pEntry->HTCapability.MCSSet[1] == 0xff) && (/*pAd->Antenna.field.TxPath*/pAd->CommonCfg.TxStream == 2))
		{// 11N 2S
			if (pAd->LatchRfRegs.Channel <= 14)
			{
				*ppTable = RateSwitchTable11N2S;
				*pTableSize = RateSwitchTable11N2S[0];
				*pInitTxRateIdx = RateSwitchTable11N2S[1];
            }
			else
			{
				*ppTable = RateSwitchTable11N2SForABand;
				*pTableSize = RateSwitchTable11N2SForABand[0];
				*pInitTxRateIdx = RateSwitchTable11N2SForABand[1];
			}
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: 11N 2S STA \n"));
		}
		else if ((pEntry->RateLen == 4) && (pEntry->HTCapability.MCSSet[0] == 0) && (pEntry->HTCapability.MCSSet[1] == 0))
		{// B only
			*ppTable = RateSwitchTable11B;
			*pTableSize = RateSwitchTable11B[0];
			*pInitTxRateIdx = RateSwitchTable11B[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: B only STA \n"));
		}
		else if ((pEntry->RateLen > 8) && (pEntry->HTCapability.MCSSet[0] == 0) && (pEntry->HTCapability.MCSSet[1] == 0))
		{// B/G  mixed
			*ppTable = RateSwitchTable11BG;
			*pTableSize = RateSwitchTable11BG[0];
			*pInitTxRateIdx = RateSwitchTable11BG[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: B/G mixed STA \n"));
		}
		else if ((pEntry->RateLen == 8) && (pEntry->HTCapability.MCSSet[0] == 0) && (pEntry->HTCapability.MCSSet[1] == 0))
		{// G only
			*ppTable = RateSwitchTable11G;
			*pTableSize = RateSwitchTable11G[0];
			*pInitTxRateIdx = RateSwitchTable11G[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: A/G STA \n"));
		}
		else
		{
			*ppTable = RateSwitchTable11N2S;
			*pTableSize = RateSwitchTable11N2S[0];
			*pInitTxRateIdx = RateSwitchTable11N2S[1];
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: unkown mode \n"));
		}
	}
}

VOID MlmeSetTxRateEx(
	IN PRTMP_ADAPTER		pAd,
	IN PMAC_TABLE_ENTRY		pEntry,
	IN PRTMP_TX_RATE_SWITCH	pTxRate)
{
	if ((pTxRate->STBC) && (pEntry->MaxHTPhyMode.field.STBC))
		pEntry->HTPhyMode.field.STBC = STBC_USE;
	else
		pEntry->HTPhyMode.field.STBC = STBC_NONE;

	if (((pTxRate->ShortGI) && (pEntry->MaxHTPhyMode.field.ShortGI))
         || ( pAd->WIFItestbed.bShortGI &&  pEntry->MaxHTPhyMode.field.ShortGI) )
		pEntry->HTPhyMode.field.ShortGI = GI_400;
	else
		pEntry->HTPhyMode.field.ShortGI = GI_800;

	if (pTxRate->CurrMCS < MCS_AUTO)
		pEntry->HTPhyMode.field.MCS = pTxRate->CurrMCS;

	if (pTxRate->Mode <= MODE_HTGREENFIELD)
		pEntry->HTPhyMode.field.MODE = pTxRate->Mode;

	if ((pAd->WIFItestbed.bGreenField & pEntry->HTCapability.HtCapInfo.GF) && (pEntry->HTPhyMode.field.MODE == MODE_HTMIX))
	{
		// force Tx GreenField 
		pEntry->HTPhyMode.field.MODE = MODE_HTGREENFIELD;
	}

	if (pAd->CommonCfg.bRcvBSSWidthTriggerEvents)
	{
		pEntry->HTPhyMode.field.BW = BW_20;
	}

	pAd->LastTxRate = (USHORT)(pEntry->HTPhyMode.word);

	DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: Aid=%d, MlmeSetTxRateEx - CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, BW=%d \n",
		pEntry->Aid, pEntry->CurrTxRateIndex,
		pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.STBC, pEntry->HTPhyMode.field.ShortGI,
		pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW));
}

VOID MlmeDynamicTxRateSwitchingEx(
    IN PRTMP_ADAPTER pAd,
	IN USHORT FirstWcid)
{
	ULONG					i;
	PUCHAR					pTable;
	UCHAR					TableSize = 0;
	UCHAR					UpRateIdx, DownRateIdx, CurrRateIdx;
	ULONG					AccuTxTotalCnt, TxTotalCnt;
	ULONG					TxErrorRatio = 0;
	MAC_TABLE_ENTRY			*pEntry;
	PRTMP_TX_RATE_SWITCH	pCurrTxRate, pNextTxRate = NULL;
	BOOLEAN					bTxRateChanged = TRUE, bUpgradeQuality = FALSE;
	UCHAR					InitTxRateIdx, TrainUp, TrainDown;
	TX_STA_CNT1_STRUC		StaTx1;
	TX_STA_CNT0_STRUC		TxStaCnt0;
	CHAR					Rssi, RssiOffset = 0;
	ULONG					TxRetransmit = 0, TxSuccess = 0, TxFailCount = 0;
	
#ifdef RALINK_ATE
   	if (pAd->ate.Mode != ATE_STOP)
   	{
		return;
   	}
#endif // RALINK_ATE //

    //
    // walk through MAC table, see if need to change AP's TX rate toward each entry
    //
    
	DBGPRINT_RAW(RT_DEBUG_INFO,("==> APMlmeDynamicTxRateSwitching : Now time (%ld)\n", (jiffies/OS_HZ)));    


	for (i = FirstWcid; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
		pEntry = &pAd->MacTab.Content[i];

		// only associated STA counts
		if (((pEntry->ValidAsCLI == FALSE) || ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst != SST_ASSOC)))
#ifdef APCLI_SUPPORT
			&&(pEntry->ValidAsApCli == FALSE)
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
			&& ((pEntry->ValidAsWDS == FALSE) || !WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx))
#endif // WDS_SUPPORT //
#ifdef MESH_SUPPORT
			&& ((pEntry->ValidAsMesh == FALSE) || !MESH_ON(pAd))
#endif // MESH_SUPPORT //
			)
			continue;

		// check if this entry need to switch rate automatically
		if (RTMPCheckEntryEnableAutoRateSwitch(pAd, pEntry) == FALSE)
			continue;

		//NICUpdateFifoStaCounters(pAd);

		if (pAd->MacTab.Size == 1)
		{
#if 0	// test by gary
			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
				 pAd->RalinkCounters.OneSecTxRetryOkCount + 
				 pAd->RalinkCounters.OneSecTxFailCount;

            if (TxTotalCnt)
				TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) * 100) / TxTotalCnt;
#else

			// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			pAd->bUpdateBcnCntDone = TRUE;
			TxRetransmit = StaTx1.field.TxRetransmit;
			TxSuccess = StaTx1.field.TxSuccess;
			TxFailCount = TxStaCnt0.field.TxFailCount;
			TxTotalCnt = TxRetransmit + TxSuccess + TxFailCount;

			pAd->RalinkCounters.OneSecBeaconSentCnt += TxStaCnt0.field.TxBeaconCount;
			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;
			pAd->WlanCounters.TransmittedFragmentCount.LowPart += StaTx1.field.TxSuccess;
			pAd->WlanCounters.RetryCount.LowPart += StaTx1.field.TxRetransmit;
			pAd->WlanCounters.FailedCount.LowPart += TxStaCnt0.field.TxFailCount;

			AccuTxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((TxRetransmit + TxFailCount) * 100) / TxTotalCnt;			
#endif
		}
		else
		{
			TxTotalCnt = pEntry->OneSecTxNoRetryOkCount + 
				 pEntry->OneSecTxRetryOkCount + 
				 pEntry->OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((pEntry->OneSecTxRetryOkCount + pEntry->OneSecTxFailCount) * 100) / TxTotalCnt;
		}

		CurrRateIdx = UpRateIdx = DownRateIdx = pEntry->CurrTxRateIndex;

		Rssi = RTMPMaxRssi(pAd, (CHAR)pEntry->RssiSample.AvgRssi0, (CHAR)pEntry->RssiSample.AvgRssi1, (CHAR)pEntry->RssiSample.AvgRssi2);

		MlmeSelectTxRateTableEx(pAd, pEntry, &pTable, &TableSize, &InitTxRateIdx);

		// decide the next upgrade rate and downgrade rate, if any
		if ((CurrRateIdx > 0) && (CurrRateIdx < (TableSize - 1)))
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx -1;
		}
		else if (CurrRateIdx == 0)
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx;
		}
		else if (CurrRateIdx == (TableSize - 1))
		{
			UpRateIdx = CurrRateIdx;
			DownRateIdx = CurrRateIdx - 1;
		}

		pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(CurrRateIdx+1)*5];

		if ((Rssi > -65) && (pCurrTxRate->Mode >= MODE_HTMIX))
		{
			TrainUp		= (pCurrTxRate->TrainUp + (pCurrTxRate->TrainUp >> 1));
			TrainDown	= (pCurrTxRate->TrainDown + (pCurrTxRate->TrainDown >> 1));
		}
		else
		{
			TrainUp		= pCurrTxRate->TrainUp;
			TrainDown	= pCurrTxRate->TrainDown;
		}

		pEntry->LastTimeTxRateChangeAction = pEntry->LastSecTxRateChangeAction;

		if (pAd->MacTab.Size == 1)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS:Aid=%d, TxRetransmit=%ld, TxFailCount=%ld, TxSuccess=%ld \n",
			pEntry->Aid, TxRetransmit, TxFailCount,	TxSuccess));
		}
		else
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS:Aid=%d, OneSecTxRetryOkCount=%d, OneSecTxFailCount=%d, OneSecTxNoRetryOkCount=%d \n",
				pEntry->Aid,
				pEntry->OneSecTxRetryOkCount,
				pEntry->OneSecTxFailCount,
				pEntry->OneSecTxNoRetryOkCount));
		}
/*
		if (pAd->CommonCfg.bAutoTxRateSwitch == FALSE)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: Fixed - CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, BW=%d, PER=%ld%% \n\n",
				pEntry->CurrTxRateIndex, pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.STBC,
				pEntry->HTPhyMode.field.ShortGI, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW, TxErrorRatio));
			return;
		}
		else
 */		
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: Before- CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, TrainUp=%d, TrainDown=%d, NextUp=%d, NextDown=%d, CurrMCS=%d, PER=%ld%%\n",
				CurrRateIdx,
				pCurrTxRate->CurrMCS,
				pCurrTxRate->STBC,
				pCurrTxRate->ShortGI,
				pCurrTxRate->Mode,
				TrainUp,
				TrainDown,
				UpRateIdx,
				DownRateIdx,
				pEntry->HTPhyMode.field.MCS,
				TxErrorRatio));
		}

		//if (! OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
		//	continue;

        if (TxTotalCnt <= 15)
        {
   			CHAR	idx = 0;
			UCHAR	TxRateIdx;
			UCHAR	MCS0 = 0, MCS1 = 0, MCS2 = 0, MCS3 = 0, MCS4 = 0,  MCS5 =0, MCS6 = 0, MCS7 = 0;
			UCHAR	MCS12 = 0, MCS13 = 0, MCS14 = 0, MCS15 = 0;

			// check the existence and index of each needed MCS
			while (idx < pTable[0])
			{
				pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(idx+1)*5];

				if (pCurrTxRate->CurrMCS == MCS_0)
				{
					MCS0 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_1)
				{
					MCS1 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_2)
				{
					MCS2 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_3)
				{
					MCS3 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_4)
				{
					MCS4 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_5)
				{
					MCS5 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_6)
				{
					MCS6 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_7)
				{
					MCS7 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_12)
				{
					MCS12 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_13)
				{
					MCS13 = idx;
				}
				else if (pCurrTxRate->CurrMCS == MCS_14)
				{
					MCS14 = idx;
				}
				else if ((pCurrTxRate->CurrMCS == MCS_15) && (pCurrTxRate->ShortGI == GI_800))	//we hope to use ShortGI as initial rate
				{
					MCS15 = idx;
				}
				
				idx ++;
			}
#if 1 // test by gary //		
			if ((pTable == RateSwitchTable11BGN2S) || (pTable == RateSwitchTable11BGN2SForABand) || (pTable == RateSwitchTable11N2S) || (pTable == RateSwitchTable11N2SForABand) || (pTable == RateSwitchTable))
			{
				RssiOffset = 2;
			}
			else
			{
				RssiOffset = 5;
			}
#endif // test by gary //
			if ((pTable == RateSwitchTable11BGN2S) || (pTable == RateSwitchTable11N2S) || (pTable == RateSwitchTable))
			{// N mode with 2 stream
				if (MCS15 && (Rssi >= (-70+RssiOffset)))
					TxRateIdx = MCS15;
				else if (MCS14 && (Rssi >= (-72+RssiOffset)))
					TxRateIdx = MCS14;
				else if (MCS13 && (Rssi >= (-76+RssiOffset)))
					TxRateIdx = MCS13;
				else if (MCS12 && (Rssi >= (-78+RssiOffset)))
					TxRateIdx = MCS12;
				else if (MCS4 && (Rssi >= (-82+RssiOffset)))
					TxRateIdx = MCS4;
				else if (MCS3 && (Rssi >= (-84+RssiOffset)))
					TxRateIdx = MCS3;
				else if (MCS2 && (Rssi >= (-86+RssiOffset)))
					TxRateIdx = MCS2;
				else if (MCS1 && (Rssi >= (-88+RssiOffset)))
					TxRateIdx = MCS1;
				else
					TxRateIdx = MCS0;
			}
			else if ((pTable == RateSwitchTable11BGN1S) || (pTable == RateSwitchTable11N1S))
			{// N mode with 1 stream
				if (MCS7 && (Rssi > (-72+RssiOffset)))
					TxRateIdx = MCS7;
				else if (MCS6 && (Rssi > (-74+RssiOffset)))
					TxRateIdx = MCS6;
				else if (MCS5 && (Rssi > (-77+RssiOffset)))
					TxRateIdx = MCS5;
				else if (MCS4 && (Rssi > (-79+RssiOffset)))
					TxRateIdx = MCS4;
				else if (MCS3 && (Rssi > (-81+RssiOffset)))
					TxRateIdx = MCS3;
				else if (MCS2 && (Rssi > (-83+RssiOffset)))
					TxRateIdx = MCS2;
				else if (MCS1 && (Rssi > (-86+RssiOffset)))
					TxRateIdx = MCS1;
			else
					TxRateIdx = MCS0;
			}
			else
			{// Legacy mode
				if (MCS7 && (Rssi > -70))
				TxRateIdx = MCS7;
				else if (MCS6 && (Rssi > -74))
					TxRateIdx = MCS6;
				else if (MCS5 && (Rssi > -78))
					TxRateIdx = MCS5;
				else if (MCS4 && (Rssi > -82))
				TxRateIdx = MCS4;
				else if (MCS4 == 0)							// for B-only mode
					TxRateIdx = MCS3;
				else if (MCS3 && (Rssi > -85))
					TxRateIdx = MCS3;
				else if (MCS2 && (Rssi > -87))
					TxRateIdx = MCS2;
				else if (MCS1 && (Rssi > -90))
					TxRateIdx = MCS1;
			else
				TxRateIdx = MCS0;
			}

			if (TxRateIdx != pEntry->CurrTxRateIndex)
			{
				pEntry->CurrTxRateIndex = TxRateIdx;
				pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
				MlmeSetTxRateEx(pAd, pEntry, pNextTxRate);
			}

			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);
			pEntry->fLastSecAccordingRSSI = TRUE;

			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: TxTotalCnt <= 15, switch MCS according to RSSI (%d), RssiOffset=%d\n", Rssi, RssiOffset));
			continue;
        }

		if (pEntry->fLastSecAccordingRSSI == TRUE)
		{
			pEntry->fLastSecAccordingRSSI = FALSE;
			pEntry->LastSecTxRateChangeAction = 0;
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: MCS is according to RSSI, and ignore tuning this sec \n"));
			return;
		}

		do
		{
			BOOLEAN	bTrainUpDown = FALSE;
			
			pEntry->CurrTxRateStableTime ++;

			// downgrade TX quality if PER >= Rate-Down threshold
			if (TxErrorRatio >= TrainDown)
			{
				bTrainUpDown = TRUE;
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}
			// upgrade TX quality if PER <= Rate-Up threshold
			else if (TxErrorRatio <= TrainUp)
			{
				bTrainUpDown = TRUE;
				bUpgradeQuality = TRUE;
				if (pEntry->TxQuality[CurrRateIdx])
					pEntry->TxQuality[CurrRateIdx] --;  // quality very good in CurrRate

				if (pEntry->TxRateUpPenalty)
					pEntry->TxRateUpPenalty --;
				else if (pEntry->TxQuality[UpRateIdx])
					pEntry->TxQuality[UpRateIdx] --;    // may improve next UP rate's quality
			}

			pEntry->PER[CurrRateIdx] = (UCHAR)TxErrorRatio;

			if (bTrainUpDown == TRUE)
			{
				// perform DRS - consider TxRate Down first, then rate up.
				if ((CurrRateIdx != DownRateIdx) && (pEntry->TxQuality[CurrRateIdx] >= DRS_TX_QUALITY_WORST_BOUND))
				{
					pEntry->CurrTxRateIndex = DownRateIdx;
				}
				else if ((CurrRateIdx != UpRateIdx) && (pEntry->TxQuality[UpRateIdx] <= 0))
				{
					pEntry->CurrTxRateIndex = UpRateIdx;
				}
			}
		}while (FALSE);

		// if rate-up happen, clear all bad history of all TX rates
		if (pEntry->CurrTxRateIndex > CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: ++TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->CurrTxRateStableTime = 0;
			pEntry->TxRateUpPenalty = 0;
			pEntry->LastSecTxRateChangeAction = 1; // rate UP
			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);

			//
			// For TxRate fast train up
			// 
			if (!pAd->CommonCfg.QuickResponeForRateUpTimerRunningEx)
			{				
				RTMPSetTimer(&pAd->CommonCfg.QuickResponeForRateUpTimerEx, 100);

				pAd->CommonCfg.QuickResponeForRateUpTimerRunningEx = TRUE;
			}
		}
		// if rate-down happen, only clear DownRate's bad history
		else if (pEntry->CurrTxRateIndex < CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("DRS: --TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->CurrTxRateStableTime = 0;
			pEntry->TxRateUpPenalty = 0;           // no penalty
			pEntry->LastSecTxRateChangeAction = 2; // rate DOWN
			pEntry->TxQuality[pEntry->CurrTxRateIndex] = 0;
			pEntry->PER[pEntry->CurrTxRateIndex] = 0;

			//
			// For TxRate fast train down
			// 
			if (!pAd->CommonCfg.QuickResponeForRateUpTimerRunningEx)
			{
				RTMPSetTimer(&pAd->CommonCfg.QuickResponeForRateUpTimerEx, 100);

				pAd->CommonCfg.QuickResponeForRateUpTimerRunningEx = TRUE;
			}
		}
		else
		{
			pEntry->LastSecTxRateChangeAction = 0; // rate no change
			bTxRateChanged = FALSE;
		}
			
		if (pAd->MacTab.Size == 1)
		{
			//test by gary 
       		//pEntry->LastTxOkCount = pAd->RalinkCounters.OneSecTxNoRetryOkCount;
			pEntry->LastTxOkCount = TxSuccess;
		}
		else
		{
			pEntry->LastTxOkCount = pEntry->OneSecTxNoRetryOkCount;

			// reset all OneSecTx counters
			pEntry->OneSecTxRetryOkCount = 0;
			pEntry->OneSecTxFailCount = 0;
			pEntry->OneSecTxNoRetryOkCount = 0;
		}

		pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
		if (bTxRateChanged && pNextTxRate)
		{
			MlmeSetTxRateEx(pAd, pEntry, pNextTxRate);
		}
    }
}

/*
    ========================================================================
    Routine Description:
        AP side, Auto TxRate faster train up timer call back function.
        
    Arguments:
        SystemSpecific1         - Not used.
        FunctionContext         - Pointer to our Adapter context.
        SystemSpecific2         - Not used.
        SystemSpecific3         - Not used.
        
    Return Value:
        None
        
    ========================================================================
*/
VOID QuickResponeForRateUpExecEx(
    IN PVOID SystemSpecific1, 
    IN PVOID FunctionContext, 
    IN PVOID SystemSpecific2, 
    IN PVOID SystemSpecific3) 
{
	PRTMP_ADAPTER			pAd = (PRTMP_ADAPTER)FunctionContext;
	ULONG					i;
	PUCHAR					pTable;
	UCHAR					TableSize = 0;
	UCHAR					UpRateIdx, DownRateIdx, CurrRateIdx;
	ULONG					AccuTxTotalCnt, TxTotalCnt, TxCnt;
	ULONG					TxErrorRatio = 0;
	MAC_TABLE_ENTRY			*pEntry;
	PRTMP_TX_RATE_SWITCH	pCurrTxRate, pNextTxRate = NULL;
	BOOLEAN					bTxRateChanged = TRUE;
	UCHAR					InitTxRateIdx, TrainUp, TrainDown;
	TX_STA_CNT1_STRUC		StaTx1;
	TX_STA_CNT0_STRUC		TxStaCnt0;
	CHAR					Rssi, ratio;
	ULONG					TxRetransmit = 0, TxSuccess = 0, TxFailCount = 0;

	pAd->CommonCfg.QuickResponeForRateUpTimerRunningEx = FALSE;
	
    //
    // walk through MAC table, see if need to change AP's TX rate toward each entry
    //
   	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) 
	{
        pEntry = &pAd->MacTab.Content[i];

        // only associated STA counts
//        if ((pEntry->ValidAsCLI == FALSE) || (pEntry->Sst != SST_ASSOC))
        if (((pEntry->ValidAsCLI == FALSE) || ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst != SST_ASSOC)))
#ifdef CONFIG_AP_SUPPORT
#ifdef APCLI_SUPPORT
			&& (pEntry->ValidAsApCli == FALSE)
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
			&& (pEntry->ValidAsWDS == FALSE || !WDS_IF_UP_CHECK(pAd, pEntry->MatchWDSTabIdx))
#endif // WDS_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
#ifdef MESH_SUPPORT
			&& ((pEntry->ValidAsMesh == FALSE) || !MESH_ON(pAd))
#endif // MESH_SUPPORT //
			)
            continue;

    	Rssi = RTMPMaxRssi(pAd, (CHAR)pEntry->RssiSample.AvgRssi0, (CHAR)pEntry->RssiSample.AvgRssi1, (CHAR)pEntry->RssiSample.AvgRssi2);

		CurrRateIdx = UpRateIdx = DownRateIdx = pEntry->CurrTxRateIndex;


		if (pAd->MacTab.Size == 1)
		{
#if 0	// test by gary
            TX_STA_CNT1_STRUC		StaTx1;
			TX_STA_CNT0_STRUC		TxStaCnt0;

       		// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;

			TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
				pAd->RalinkCounters.OneSecTxRetryOkCount + 
				pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((pAd->RalinkCounters.OneSecTxRetryOkCount + pAd->RalinkCounters.OneSecTxFailCount) * 100) / TxTotalCnt;
#else
			// Update statistic counter
			RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
			RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
			TxRetransmit = StaTx1.field.TxRetransmit;
			TxSuccess = StaTx1.field.TxSuccess;
			TxFailCount = TxStaCnt0.field.TxFailCount;
			TxTotalCnt = TxRetransmit + TxSuccess + TxFailCount;

			pAd->RalinkCounters.OneSecBeaconSentCnt += TxStaCnt0.field.TxBeaconCount;
			pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
			pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
			pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;
			pAd->WlanCounters.TransmittedFragmentCount.LowPart += StaTx1.field.TxSuccess;
			pAd->WlanCounters.RetryCount.LowPart += StaTx1.field.TxRetransmit;
			pAd->WlanCounters.FailedCount.LowPart += TxStaCnt0.field.TxFailCount;

			AccuTxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxRetryOkCount + 
					 pAd->RalinkCounters.OneSecTxFailCount;

			if (TxTotalCnt)
				TxErrorRatio = ((TxRetransmit + TxFailCount) * 100) / TxTotalCnt;

			if (pAd->Antenna.field.TxPath > 1)
				Rssi = (pEntry->RssiSample.AvgRssi0 + pEntry->RssiSample.AvgRssi1) >> 1;
			else
				Rssi = pEntry->RssiSample.AvgRssi0;

			TxCnt = AccuTxTotalCnt;
#endif
		}
		else
		{
		TxTotalCnt = pEntry->OneSecTxNoRetryOkCount + 
			 pEntry->OneSecTxRetryOkCount + 
			 pEntry->OneSecTxFailCount;

		if (TxTotalCnt)
			TxErrorRatio = ((pEntry->OneSecTxRetryOkCount + pEntry->OneSecTxFailCount) * 100) / TxTotalCnt;
	
			TxCnt = TxTotalCnt;	
		}

		// decide the rate table for tuning
		MlmeSelectTxRateTableEx(pAd, pEntry, &pTable, &TableSize, &InitTxRateIdx);

		// decide the next upgrade rate and downgrade rate, if any
		if ((CurrRateIdx > 0) && (CurrRateIdx < (TableSize - 1)))
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx -1;
		}
		else if (CurrRateIdx == 0)
		{
			UpRateIdx = CurrRateIdx + 1;
			DownRateIdx = CurrRateIdx;
		}
		else if (CurrRateIdx == (TableSize - 1))
		{
			UpRateIdx = CurrRateIdx;
			DownRateIdx = CurrRateIdx - 1;
		}

		pCurrTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(CurrRateIdx+1)*5];

		if ((Rssi > -65) && (pCurrTxRate->Mode >= MODE_HTMIX))
		{
			TrainUp		= (pCurrTxRate->TrainUp + (pCurrTxRate->TrainUp >> 1));
			TrainDown	= (pCurrTxRate->TrainDown + (pCurrTxRate->TrainDown >> 1));
		}
		else
		{
			TrainUp		= pCurrTxRate->TrainUp;
			TrainDown	= pCurrTxRate->TrainDown;
		}

		if (pAd->MacTab.Size == 1)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS:Aid=%d, TxRetransmit=%ld, TxFailCount=%ld, TxSuccess=%ld \n",
			pEntry->Aid, TxRetransmit, TxFailCount,	TxSuccess));
		}
		else
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS:Aid=%d, OneSecTxRetryOkCount=%d, OneSecTxFailCount=%d, OneSecTxNoRetryOkCount=%d \n",
			pEntry->Aid,
			pEntry->OneSecTxRetryOkCount,
			pEntry->OneSecTxFailCount,
			pEntry->OneSecTxNoRetryOkCount));
		}

/*
		if (pAd->CommonCfg.bAutoTxRateSwitch == FALSE)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: Fixed - CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, BW=%d, PER=%ld%% \n\n",
				pEntry->CurrTxRateIndex, pEntry->HTPhyMode.field.MCS, pEntry->HTPhyMode.field.STBC,
				pEntry->HTPhyMode.field.ShortGI, pEntry->HTPhyMode.field.MODE, pEntry->HTPhyMode.field.BW, TxErrorRatio));
			return;
		}
		else
*/
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: Before- CurrTxRateIdx=%d, MCS=%d, STBC=%d, ShortGI=%d, Mode=%d, TrainUp=%d, TrainDown=%d, NextUp=%d, NextDown=%d, CurrMCS=%d, PER=%ld%%\n",
				CurrRateIdx,
				pCurrTxRate->CurrMCS,
				pCurrTxRate->STBC,
				pCurrTxRate->ShortGI,
				pCurrTxRate->Mode,
				TrainUp,
				TrainDown,
				UpRateIdx,
				DownRateIdx,
				pEntry->HTPhyMode.field.MCS,
				TxErrorRatio));
		}

//		if (! OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED))
//			continue;

        if (/*TxTotalCnt*/ TxCnt <= 15)
        {
			NdisZeroMemory(pAd->DrsCounters.TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pAd->DrsCounters.PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);

			// perform DRS - consider TxRate Down first, then rate up.
			if ((pEntry->LastSecTxRateChangeAction == 1) && (CurrRateIdx != DownRateIdx))
			{
				pEntry->CurrTxRateIndex = DownRateIdx;
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}
			else if ((pEntry->LastSecTxRateChangeAction == 2) && (CurrRateIdx != UpRateIdx))
			{
				pEntry->CurrTxRateIndex = UpRateIdx;
			}
			
            DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: TxTotalCnt <= 15, train back to original rate \n"));
			continue;
        }

		do
		{
			ULONG		OneSecTxNoRetryOKRationCount;

			// test by gary
			//if (pEntry->LastSecTxRateChangeAction == 0)
			if (pEntry->LastTimeTxRateChangeAction == 0)
				ratio = 5;
			else
				ratio = 4;

			// downgrade TX quality if PER >= Rate-Down threshold
			if (TxErrorRatio >= TrainDown)
			{
				pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
			}

			pEntry->PER[CurrRateIdx] = (UCHAR)TxErrorRatio;

			if (pAd->MacTab.Size == 1)
			{
   				//OneSecTxNoRetryOKRationCount = pAd->RalinkCounters.OneSecTxNoRetryOkCount * ratio + (pAd->RalinkCounters.OneSecTxNoRetryOkCount >> 1);
				// test by gary
				OneSecTxNoRetryOKRationCount = (TxSuccess * ratio);
			}
			else
			{
				OneSecTxNoRetryOKRationCount = pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1);
			}

			// perform DRS - consider TxRate Down first, then rate up.
			if ((pEntry->LastSecTxRateChangeAction == 1) && (CurrRateIdx != DownRateIdx))
			{
				//if ((pEntry->LastTxOkCount + 2) >= (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1)))
				if ((pEntry->LastTxOkCount + 2) >= OneSecTxNoRetryOKRationCount)
				{
					pEntry->CurrTxRateIndex = DownRateIdx;
					pEntry->TxQuality[CurrRateIdx] = DRS_TX_QUALITY_WORST_BOUND;
					//DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) bad tx ok count (L:%ld, C:%d)\n", pEntry->LastTxOkCount, (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1))));
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Up) bad tx ok count (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
				else
				{
					//DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) keep rate-up (L:%ld, C:%d)\n", pEntry->LastTxOkCount, (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1))));
					DBGPRINT_RAW(RT_DEBUG_TRACE,("QuickDRS: (Up) keep rate-up (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
			}
			else if ((pEntry->LastSecTxRateChangeAction == 2) && (CurrRateIdx != UpRateIdx))
			{
				//if ((TxErrorRatio >= 50) || (TxErrorRatio >= TrainDown))
				if ((TxErrorRatio >= 50) && (TxErrorRatio >= TrainDown))
				{
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Down) direct train down (TxErrorRatio >= TrainDown)\n"));
				}
				//else if ((pEntry->LastTxOkCount + 2) >= (pEntry->OneSecTxNoRetryOkCount * ratio + (pEntry->OneSecTxNoRetryOkCount >> 1)))
				else if ((pEntry->LastTxOkCount + 2) >= OneSecTxNoRetryOKRationCount)
				{
					pEntry->CurrTxRateIndex = UpRateIdx;
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Down) bad tx ok count (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
				else
				{
					DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: (Down) keep rate-down (L:%ld, C:%ld)\n", pEntry->LastTxOkCount, OneSecTxNoRetryOKRationCount));
				}
			}
		}while (FALSE);

		// if rate-up happen, clear all bad history of all TX rates
		if (pEntry->CurrTxRateIndex > CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: ++TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->TxRateUpPenalty = 0;
			NdisZeroMemory(pEntry->TxQuality, sizeof(USHORT) * MAX_STEP_OF_TX_RATE_SWITCH);
			NdisZeroMemory(pEntry->PER, sizeof(UCHAR) * MAX_STEP_OF_TX_RATE_SWITCH);
		}
		// if rate-down happen, only clear DownRate's bad history
		else if (pEntry->CurrTxRateIndex < CurrRateIdx)
		{
			DBGPRINT_RAW(RT_DEBUG_INFO,("QuickDRS: --TX rate from %d to %d \n", CurrRateIdx, pEntry->CurrTxRateIndex));
			
			pEntry->TxRateUpPenalty = 0;           // no penalty
			pEntry->TxQuality[pEntry->CurrTxRateIndex] = 0;
			pEntry->PER[pEntry->CurrTxRateIndex] = 0;
		}
		else
		{
			bTxRateChanged = FALSE;
		}

		pNextTxRate = (PRTMP_TX_RATE_SWITCH) &pTable[(pEntry->CurrTxRateIndex+1)*5];
		if (bTxRateChanged && pNextTxRate)
		{
			MlmeSetTxRateEx(pAd, pEntry, pNextTxRate);
		}
    }
}



/*
	==========================================================================
	Validate SSID for connection try and rescan purpose
	Valid SSID will have visible chars only.
	The valid length is from 0 to 32.
	IRQL = DISPATCH_LEVEL
	==========================================================================
 */
BOOLEAN MlmeValidateSSID(
	IN PUCHAR	pSsid,
	IN UCHAR	SsidLen)
{
	int	index;

	if (SsidLen > MAX_LEN_OF_SSID)
		return (FALSE);

	// Check each character value
	for (index = 0; index < SsidLen; index++)
	{
		if (pSsid[index] < 0x20)
			return (FALSE);
	}

	// All checked
	return (TRUE);
}

/*
	==========================================================================
	Description:
		This routine checks if there're other APs out there capable for
		roaming. Caller should call this routine only when Link up in INFRA mode
		and channel quality is below CQI_GOOD_THRESHOLD.

	IRQL = DISPATCH_LEVEL

	Output:
	==========================================================================
 */


// IRQL = DISPATCH_LEVEL
VOID MlmeSetTxPreamble(
	IN PRTMP_ADAPTER pAd, 
	IN USHORT TxPreamble)
{
	AUTO_RSP_CFG_STRUC csr4;

	//
	// Always use Long preamble before verifiation short preamble functionality works well.
	// Todo: remove the following line if short preamble functionality works
	//
	//TxPreamble = Rt802_11PreambleLong;
	
	RTMP_IO_READ32(pAd, AUTO_RSP_CFG, &csr4.word);
	if (TxPreamble == Rt802_11PreambleLong)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("MlmeSetTxPreamble (= LONG PREAMBLE)\n"));
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED); 
		csr4.field.AutoResponderPreamble = 0;
	}
	else
	{
		// NOTE: 1Mbps should always use long preamble
		DBGPRINT(RT_DEBUG_TRACE, ("MlmeSetTxPreamble (= SHORT PREAMBLE)\n"));
		OPSTATUS_SET_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);
		csr4.field.AutoResponderPreamble = 1;
	}

	RTMP_IO_WRITE32(pAd, AUTO_RSP_CFG, csr4.word);
}

/*
    ==========================================================================
    Description:
        Update basic rate bitmap
    ==========================================================================
 */
 
VOID UpdateBasicRateBitmap(
    IN  PRTMP_ADAPTER   pAdapter)
{
    INT  i, j;
                  /* 1  2  5.5, 11,  6,  9, 12, 18, 24, 36, 48,  54 */
    UCHAR rate[] = { 2, 4,  11, 22, 12, 18, 24, 36, 48, 72, 96, 108 };
    UCHAR *sup_p = pAdapter->CommonCfg.SupRate;
    UCHAR *ext_p = pAdapter->CommonCfg.ExtRate;
    ULONG bitmap = pAdapter->CommonCfg.BasicRateBitmap;


    /* if A mode, always use fix BasicRateBitMap */
    //if (pAdapter->CommonCfg.Channel == PHY_11A)
	if (pAdapter->CommonCfg.Channel > 14)
        pAdapter->CommonCfg.BasicRateBitmap = 0x150; /* 6, 12, 24M */
    /* End of if */

    if (pAdapter->CommonCfg.BasicRateBitmap > 4095)
    {
        /* (2 ^ MAX_LEN_OF_SUPPORTED_RATES) -1 */
        return;
    } /* End of if */

    for(i=0; i<MAX_LEN_OF_SUPPORTED_RATES; i++)
    {
        sup_p[i] &= 0x7f;
        ext_p[i] &= 0x7f;
    } /* End of for */

    for(i=0; i<MAX_LEN_OF_SUPPORTED_RATES; i++)
    {
        if (bitmap & (1 << i))
        {
            for(j=0; j<MAX_LEN_OF_SUPPORTED_RATES; j++)
            {
                if (sup_p[j] == rate[i])
                    sup_p[j] |= 0x80;
                /* End of if */
            } /* End of for */

            for(j=0; j<MAX_LEN_OF_SUPPORTED_RATES; j++)
            {
                if (ext_p[j] == rate[i])
                    ext_p[j] |= 0x80;
                /* End of if */
            } /* End of for */
        } /* End of if */
    } /* End of for */
} /* End of UpdateBasicRateBitmap */

// IRQL = PASSIVE_LEVEL
// IRQL = DISPATCH_LEVEL
// bLinkUp is to identify the inital link speed.
// TRUE indicates the rate update at linkup, we should not try to set the rate at 54Mbps.
VOID MlmeUpdateTxRates(
	IN PRTMP_ADAPTER 		pAd,
	IN 	BOOLEAN		 		bLinkUp,
	IN	UCHAR				apidx)
{
	int i, num;
	UCHAR Rate = RATE_6, MaxDesire = RATE_1, MaxSupport = RATE_1;
	UCHAR MinSupport = RATE_54;
	ULONG BasicRateBitmap = 0;
	UCHAR CurrBasicRate = RATE_1;
	UCHAR *pSupRate, SupRateLen, *pExtRate, ExtRateLen;
	PHTTRANSMIT_SETTING		pHtPhy = NULL;
	PHTTRANSMIT_SETTING		pMaxHtPhy = NULL;
	PHTTRANSMIT_SETTING		pMinHtPhy = NULL;	
	BOOLEAN 				*auto_rate_cur_p;
	UCHAR					HtMcs = MCS_AUTO;

	// find max desired rate
	UpdateBasicRateBitmap(pAd);
	
	num = 0;
	auto_rate_cur_p = NULL;
	for (i=0; i<MAX_LEN_OF_SUPPORTED_RATES; i++)
	{
		switch (pAd->CommonCfg.DesireRate[i] & 0x7f)
		{
			case 2:  Rate = RATE_1;   num++;   break;
			case 4:  Rate = RATE_2;   num++;   break;
			case 11: Rate = RATE_5_5; num++;   break;
			case 22: Rate = RATE_11;  num++;   break;
			case 12: Rate = RATE_6;   num++;   break;
			case 18: Rate = RATE_9;   num++;   break;
			case 24: Rate = RATE_12;  num++;   break;
			case 36: Rate = RATE_18;  num++;   break;
			case 48: Rate = RATE_24;  num++;   break;
			case 72: Rate = RATE_36;  num++;   break;
			case 96: Rate = RATE_48;  num++;   break;
			case 108: Rate = RATE_54; num++;   break;
			//default: Rate = RATE_1;   break;
		}
		if (MaxDesire < Rate)  MaxDesire = Rate;
	}

//===========================================================================
//===========================================================================
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
#ifdef MESH_SUPPORT	
		if (apidx >= MIN_NET_DEVICE_FOR_MESH)
		{			
			pHtPhy 		= &pAd->MeshTab.HTPhyMode;	
			pMaxHtPhy	= &pAd->MeshTab.MaxHTPhyMode;
			pMinHtPhy	= &pAd->MeshTab.MinHTPhyMode;

			auto_rate_cur_p = &pAd->MeshTab.bAutoTxRateSwitch;	
			HtMcs 		= pAd->MeshTab.DesiredTransmitSetting.field.MCS;
		}
		else
#endif // MESH_SUPPORT //
#ifdef APCLI_SUPPORT	
		if (apidx >= MIN_NET_DEVICE_FOR_APCLI)
		{			
			UCHAR	idx = apidx - MIN_NET_DEVICE_FOR_APCLI;
			
			pHtPhy 		= &pAd->ApCfg.ApCliTab[idx].HTPhyMode;	
			pMaxHtPhy	= &pAd->ApCfg.ApCliTab[idx].MaxHTPhyMode;
			pMinHtPhy	= &pAd->ApCfg.ApCliTab[idx].MinHTPhyMode;

			auto_rate_cur_p = &pAd->ApCfg.ApCliTab[idx].bAutoTxRateSwitch;	
			HtMcs 		= pAd->ApCfg.ApCliTab[idx].DesiredTransmitSetting.field.MCS;
		}
		else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
		if (apidx >= MIN_NET_DEVICE_FOR_WDS)
		{			
			UCHAR	idx = apidx - MIN_NET_DEVICE_FOR_WDS;
			
			pHtPhy 		= &pAd->WdsTab.WdsEntry[idx].HTPhyMode;	
			pMaxHtPhy	= &pAd->WdsTab.WdsEntry[idx].MaxHTPhyMode;
			pMinHtPhy	= &pAd->WdsTab.WdsEntry[idx].MinHTPhyMode;

			auto_rate_cur_p = &pAd->WdsTab.WdsEntry[idx].bAutoTxRateSwitch;	
			HtMcs 		= pAd->WdsTab.WdsEntry[idx].DesiredTransmitSetting.field.MCS;
		}
		else
#endif // WDS_SUPPORT //
		if (apidx < pAd->ApCfg.BssidNum)
		{								
			pHtPhy 		= &pAd->ApCfg.MBSSID[apidx].HTPhyMode;	
			pMaxHtPhy	= &pAd->ApCfg.MBSSID[apidx].MaxHTPhyMode;
			pMinHtPhy	= &pAd->ApCfg.MBSSID[apidx].MinHTPhyMode;

			auto_rate_cur_p = &pAd->ApCfg.MBSSID[apidx].bAutoTxRateSwitch;	
			HtMcs 		= pAd->ApCfg.MBSSID[apidx].DesiredTransmitSetting.field.MCS;
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("MlmeUpdateTxRates: invalid apidx(%d)\n", apidx));
			return;
		}			
	}	
#endif // CONFIG_AP_SUPPORT //


	pAd->CommonCfg.MaxDesiredRate = MaxDesire;
	pMinHtPhy->word = 0;
	pMaxHtPhy->word = 0;
	pHtPhy->word = 0;

	// Auto rate switching is enabled only if more than one DESIRED RATES are 
	// specified; otherwise disabled
	if (num <= 1)
	{
		//OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED);
		//pAd->CommonCfg.bAutoTxRateSwitch	= FALSE;
		*auto_rate_cur_p = FALSE;
	}
	else
	{
		//OPSTATUS_SET_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED); 
		//pAd->CommonCfg.bAutoTxRateSwitch	= TRUE;
		*auto_rate_cur_p = TRUE;
	}

#if 1
	if (HtMcs != MCS_AUTO)
	{
		//OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED);
		//pAd->CommonCfg.bAutoTxRateSwitch	= FALSE;
		*auto_rate_cur_p = FALSE;
	}
	else
	{
		//OPSTATUS_SET_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED); 
		//pAd->CommonCfg.bAutoTxRateSwitch	= TRUE;
		*auto_rate_cur_p = TRUE;
	}
#endif

	{
		pSupRate = &pAd->CommonCfg.SupRate[0];
		pExtRate = &pAd->CommonCfg.ExtRate[0];
		SupRateLen = pAd->CommonCfg.SupRateLen;
		ExtRateLen = pAd->CommonCfg.ExtRateLen;
	}

	// find max supported rate
	for (i=0; i<SupRateLen; i++)
	{
		switch (pSupRate[i] & 0x7f)
		{
			case 2:   Rate = RATE_1;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0001;	 break;
			case 4:   Rate = RATE_2;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0002;	 break;
			case 11:  Rate = RATE_5_5;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0004;	 break;
			case 22:  Rate = RATE_11;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0008;	 break;
			case 12:  Rate = RATE_6;	/*if (pSupRate[i] & 0x80)*/  BasicRateBitmap |= 0x0010;  break;
			case 18:  Rate = RATE_9;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0020;	 break;
			case 24:  Rate = RATE_12;	/*if (pSupRate[i] & 0x80)*/  BasicRateBitmap |= 0x0040;  break;
			case 36:  Rate = RATE_18;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0080;	 break;
			case 48:  Rate = RATE_24;	/*if (pSupRate[i] & 0x80)*/  BasicRateBitmap |= 0x0100;  break;
			case 72:  Rate = RATE_36;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0200;	 break;
			case 96:  Rate = RATE_48;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0400;	 break;
			case 108: Rate = RATE_54;	if (pSupRate[i] & 0x80) BasicRateBitmap |= 0x0800;	 break;
			default:  Rate = RATE_1;	break;
		}
		if (MaxSupport < Rate)	MaxSupport = Rate;

		if (MinSupport > Rate) MinSupport = Rate;		
	}
	
	for (i=0; i<ExtRateLen; i++)
	{
		switch (pExtRate[i] & 0x7f)
		{
			case 2:   Rate = RATE_1;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0001;	 break;
			case 4:   Rate = RATE_2;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0002;	 break;
			case 11:  Rate = RATE_5_5;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0004;	 break;
			case 22:  Rate = RATE_11;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0008;	 break;
			case 12:  Rate = RATE_6;	/*if (pExtRate[i] & 0x80)*/  BasicRateBitmap |= 0x0010;  break;
			case 18:  Rate = RATE_9;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0020;	 break;
			case 24:  Rate = RATE_12;	/*if (pExtRate[i] & 0x80)*/  BasicRateBitmap |= 0x0040;  break;
			case 36:  Rate = RATE_18;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0080;	 break;
			case 48:  Rate = RATE_24;	/*if (pExtRate[i] & 0x80)*/  BasicRateBitmap |= 0x0100;  break;
			case 72:  Rate = RATE_36;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0200;	 break;
			case 96:  Rate = RATE_48;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0400;	 break;
			case 108: Rate = RATE_54;	if (pExtRate[i] & 0x80) BasicRateBitmap |= 0x0800;	 break;
			default:  Rate = RATE_1;	break;
		}
		if (MaxSupport < Rate)	MaxSupport = Rate;

		if (MinSupport > Rate) MinSupport = Rate;		
	}
	RTMP_IO_WRITE32(pAd, LEGACY_BASIC_RATE, BasicRateBitmap);
	
	// bug fix 
	// pAd->CommonCfg.BasicRateBitmap = BasicRateBitmap;

	// calculate the exptected ACK rate for each TX rate. This info is used to caculate
	// the DURATION field of outgoing uniicast DATA/MGMT frame
	for (i=0; i<MAX_LEN_OF_SUPPORTED_RATES; i++)
	{
		if (BasicRateBitmap & (0x01 << i))
			CurrBasicRate = (UCHAR)i;
		pAd->CommonCfg.ExpectedACKRate[i] = CurrBasicRate;
		DBGPRINT(RT_DEBUG_INFO,("Exptected ACK rate[%d] = %d Mbps\n", RateIdToMbps[i], RateIdToMbps[CurrBasicRate]));
	}

	DBGPRINT(RT_DEBUG_TRACE,("MlmeUpdateTxRates[MaxSupport = %d] = MaxDesire %d Mbps\n", RateIdToMbps[MaxSupport], RateIdToMbps[MaxDesire]));
	// max tx rate = min {max desire rate, max supported rate}
	if (MaxSupport < MaxDesire)
		pAd->CommonCfg.MaxTxRate = MaxSupport;
	else
		pAd->CommonCfg.MaxTxRate = MaxDesire;

	pAd->CommonCfg.MinTxRate = MinSupport;
	// 2003-07-31 john - 2500 doesn't have good sensitivity at high OFDM rates. to increase the success
	// ratio of initial DHCP packet exchange, TX rate starts from a lower rate depending
	// on average RSSI
	//	 1. RSSI >= -70db, start at 54 Mbps (short distance)
	//	 2. -70 > RSSI >= -75, start at 24 Mbps (mid distance)
	//	 3. -75 > RSSI, start at 11 Mbps (long distance)
	//if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED)/* &&
	//	OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)*/)
	if (*auto_rate_cur_p)
	{
		short dbm = 0;
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			dbm =0;
#endif // CONFIG_AP_SUPPORT //
		if (bLinkUp == TRUE)
			pAd->CommonCfg.TxRate = RATE_24;
		else
			pAd->CommonCfg.TxRate = pAd->CommonCfg.MaxTxRate; 

		if (dbm < -75)
			pAd->CommonCfg.TxRate = RATE_11;
		else if (dbm < -70)
			pAd->CommonCfg.TxRate = RATE_24;

		// should never exceed MaxTxRate (consider 11B-only mode)
		if (pAd->CommonCfg.TxRate > pAd->CommonCfg.MaxTxRate)
			pAd->CommonCfg.TxRate = pAd->CommonCfg.MaxTxRate; 

		pAd->CommonCfg.TxRateIndex = 0;
		DBGPRINT(RT_DEBUG_INFO, (" MlmeUpdateTxRates (Rssi=%d, init TX rate = %d Mbps)\n", dbm, RateIdToMbps[pAd->CommonCfg.TxRate]));
	}
	else
	{
		pAd->CommonCfg.TxRate = pAd->CommonCfg.MaxTxRate;
		pHtPhy->field.MCS	= (pAd->CommonCfg.MaxTxRate > 3) ? (pAd->CommonCfg.MaxTxRate - 4) : pAd->CommonCfg.MaxTxRate;
		pHtPhy->field.MODE	= (pAd->CommonCfg.MaxTxRate > 3) ? MODE_OFDM : MODE_CCK;
		
		pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.STBC	= pHtPhy->field.STBC;
		pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.ShortGI	= pHtPhy->field.ShortGI;
		pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.MCS		= pHtPhy->field.MCS;
		pAd->MacTab.Content[BSSID_WCID].HTPhyMode.field.MODE	= pHtPhy->field.MODE;
	}

	if (pAd->CommonCfg.TxRate <= RATE_11)
	{
		pMaxHtPhy->field.MODE = MODE_CCK;
		pMaxHtPhy->field.MCS = pAd->CommonCfg.TxRate;
		pMinHtPhy->field.MCS = pAd->CommonCfg.MinTxRate;
	}
	else
	{
		pMaxHtPhy->field.MODE = MODE_OFDM;
		pMaxHtPhy->field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.TxRate];
		if (pAd->CommonCfg.MinTxRate >= RATE_6 && (pAd->CommonCfg.MinTxRate <= RATE_54))
			{pMinHtPhy->field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MinTxRate];}
		else
			{pMinHtPhy->field.MCS = pAd->CommonCfg.MinTxRate;}
	}

	pHtPhy->word = (pMaxHtPhy->word);
	if (bLinkUp && (pAd->OpMode == OPMODE_STA))
	{
			pAd->MacTab.Content[BSSID_WCID].HTPhyMode.word = pHtPhy->word;
			pAd->MacTab.Content[BSSID_WCID].MaxHTPhyMode.word = pMaxHtPhy->word;
			pAd->MacTab.Content[BSSID_WCID].MinHTPhyMode.word = pMinHtPhy->word;
	}
	else
	{
		switch (pAd->CommonCfg.PhyMode) 
		{
			case PHY_11BG_MIXED:
			case PHY_11B:
			case PHY_11BGN_MIXED:
				pAd->CommonCfg.MlmeRate = RATE_1;
				pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_CCK;
				pAd->CommonCfg.MlmeTransmit.field.MCS = RATE_1;
				
//#ifdef	WIFI_TEST			
				pAd->CommonCfg.RtsRate = RATE_11;
//#else
//				pAd->CommonCfg.RtsRate = RATE_1;
//#endif
				break;
			case PHY_11AGN_MIXED:
			case PHY_11GN_MIXED:
			case PHY_11N_2_4G:
			case PHY_11G:
			case PHY_11A:
			case PHY_11AN_MIXED:
			case PHY_11N_5G:	
				pAd->CommonCfg.MlmeRate = RATE_6;
				pAd->CommonCfg.RtsRate = RATE_6;
				pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
				pAd->CommonCfg.MlmeTransmit.field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MlmeRate];
				break;
			case PHY_11ABG_MIXED:
			case PHY_11ABGN_MIXED:
				if (pAd->CommonCfg.Channel <= 14)
				{
					pAd->CommonCfg.MlmeRate = RATE_1;
					pAd->CommonCfg.RtsRate = RATE_1;
					pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_CCK;
					pAd->CommonCfg.MlmeTransmit.field.MCS = RATE_1;
				}
				else
				{
					pAd->CommonCfg.MlmeRate = RATE_6;
					pAd->CommonCfg.RtsRate = RATE_6;
					pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
					pAd->CommonCfg.MlmeTransmit.field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MlmeRate];
				}
				break;
			default: // error
				pAd->CommonCfg.MlmeRate = RATE_6;
                        	pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
				pAd->CommonCfg.MlmeTransmit.field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MlmeRate];
				pAd->CommonCfg.RtsRate = RATE_1;
				break;
		}
		//
		// Keep Basic Mlme Rate.
		//
		pAd->MacTab.Content[MCAST_WCID].HTPhyMode.word = pAd->CommonCfg.MlmeTransmit.word;
		if (pAd->CommonCfg.MlmeTransmit.field.MODE == MODE_OFDM)
			pAd->MacTab.Content[MCAST_WCID].HTPhyMode.field.MCS = OfdmRateToRxwiMCS[RATE_24];
		else
			pAd->MacTab.Content[MCAST_WCID].HTPhyMode.field.MCS = RATE_1;
		pAd->CommonCfg.BasicMlmeRate = pAd->CommonCfg.MlmeRate;
	}

	DBGPRINT(RT_DEBUG_TRACE, (" MlmeUpdateTxRates (MaxDesire=%d, MaxSupport=%d, MaxTxRate=%d, MinRate=%d, Rate Switching =%d)\n", 
			 RateIdToMbps[MaxDesire], RateIdToMbps[MaxSupport], RateIdToMbps[pAd->CommonCfg.MaxTxRate], RateIdToMbps[pAd->CommonCfg.MinTxRate], 
			 /*OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED)*/*auto_rate_cur_p));
	DBGPRINT(RT_DEBUG_TRACE, (" MlmeUpdateTxRates (TxRate=%d, RtsRate=%d, BasicRateBitmap=0x%04lx)\n", 
			 RateIdToMbps[pAd->CommonCfg.TxRate], RateIdToMbps[pAd->CommonCfg.RtsRate], BasicRateBitmap));
	DBGPRINT(RT_DEBUG_TRACE, ("MlmeUpdateTxRates (MlmeTransmit=0x%x, MinHTPhyMode=%x, MaxHTPhyMode=0x%x, HTPhyMode=0x%x)\n", 
			 pAd->CommonCfg.MlmeTransmit.word, pAd->MacTab.Content[BSSID_WCID].MinHTPhyMode.word ,pAd->MacTab.Content[BSSID_WCID].MaxHTPhyMode.word ,pAd->MacTab.Content[BSSID_WCID].HTPhyMode.word ));
}

/*
	==========================================================================
	Description:
		This function update HT Rate setting.
		Input Wcid value is valid for 2 case :
		1. it's used for Station in infra mode that copy AP rate to Mactable.
		2. OR Station 	in adhoc mode to copy peer's HT rate to Mactable. 

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
VOID MlmeUpdateHtTxRates(
	IN PRTMP_ADAPTER 		pAd,
	IN	UCHAR				apidx)
{
	UCHAR	i, StbcMcs; //j, StbcMcs, bitmask;
	RT_HT_CAPABILITY 	*pRtHtCap = NULL;
	RT_HT_PHY_INFO		*pActiveHtPhy = NULL;	
	ULONG		BasicMCS;
	PRT_HT_PHY_INFO			pDesireHtPhy = NULL;
	PHTTRANSMIT_SETTING		pHtPhy = NULL;
	PHTTRANSMIT_SETTING		pMaxHtPhy = NULL;
	PHTTRANSMIT_SETTING		pMinHtPhy = NULL;	
	BOOLEAN 				*auto_rate_cur_p;
	
	DBGPRINT(RT_DEBUG_TRACE,("MlmeUpdateHtTxRates===> \n"));

	auto_rate_cur_p = NULL;
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
#ifdef MESH_SUPPORT	
		if (apidx >= MIN_NET_DEVICE_FOR_MESH)
		{
			pDesireHtPhy	= &pAd->MeshTab.DesiredHtPhyInfo;
			pActiveHtPhy	= &pAd->MeshTab.DesiredHtPhyInfo;
			pHtPhy 			= &pAd->MeshTab.HTPhyMode;	
			pMaxHtPhy		= &pAd->MeshTab.MaxHTPhyMode;
			pMinHtPhy		= &pAd->MeshTab.MinHTPhyMode;

			auto_rate_cur_p = &pAd->MeshTab.bAutoTxRateSwitch;
		}
		else
#endif // MESH_SUPPORT //
#ifdef APCLI_SUPPORT	
		if (apidx >= MIN_NET_DEVICE_FOR_APCLI)
		{
			UCHAR	idx = apidx - MIN_NET_DEVICE_FOR_APCLI;
		
			pDesireHtPhy	= &pAd->ApCfg.ApCliTab[idx].DesiredHtPhyInfo;
			pActiveHtPhy	= &pAd->ApCfg.ApCliTab[idx].DesiredHtPhyInfo;
			pHtPhy 			= &pAd->ApCfg.ApCliTab[idx].HTPhyMode;	
			pMaxHtPhy		= &pAd->ApCfg.ApCliTab[idx].MaxHTPhyMode;
			pMinHtPhy		= &pAd->ApCfg.ApCliTab[idx].MinHTPhyMode;

			auto_rate_cur_p = &pAd->ApCfg.ApCliTab[idx].bAutoTxRateSwitch;
		}
		else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
		if (apidx >= MIN_NET_DEVICE_FOR_WDS)
		{
			UCHAR	idx = apidx - MIN_NET_DEVICE_FOR_WDS;
		
			pDesireHtPhy	= &pAd->WdsTab.WdsEntry[idx].DesiredHtPhyInfo;
			pActiveHtPhy	= &pAd->WdsTab.WdsEntry[idx].DesiredHtPhyInfo;
			pHtPhy 			= &pAd->WdsTab.WdsEntry[idx].HTPhyMode;	
			pMaxHtPhy		= &pAd->WdsTab.WdsEntry[idx].MaxHTPhyMode;
			pMinHtPhy		= &pAd->WdsTab.WdsEntry[idx].MinHTPhyMode;

			auto_rate_cur_p = &pAd->WdsTab.WdsEntry[idx].bAutoTxRateSwitch;
		}
		else
#endif // WDS_SUPPORT //
		if (apidx < pAd->ApCfg.BssidNum)
		{		
			pDesireHtPhy	= &pAd->ApCfg.MBSSID[apidx].DesiredHtPhyInfo;
			pActiveHtPhy	= &pAd->ApCfg.MBSSID[apidx].DesiredHtPhyInfo;
			pHtPhy 			= &pAd->ApCfg.MBSSID[apidx].HTPhyMode;	
			pMaxHtPhy		= &pAd->ApCfg.MBSSID[apidx].MaxHTPhyMode;
			pMinHtPhy		= &pAd->ApCfg.MBSSID[apidx].MinHTPhyMode;

			auto_rate_cur_p = &pAd->ApCfg.MBSSID[apidx].bAutoTxRateSwitch;									
		}
		else
		{
			DBGPRINT(RT_DEBUG_ERROR, ("MlmeUpdateHtTxRates: invalid apidx(%d)\n", apidx));			
			return;
		}			
	}	
#endif // CONFIG_AP_SUPPORT //


	{
		if (pDesireHtPhy->bHtEnable == FALSE)
			return;

		pRtHtCap = &pAd->CommonCfg.DesiredHtPhy;
		StbcMcs = (UCHAR)pAd->CommonCfg.AddHTInfo.AddHtInfo3.StbcMcs;
		BasicMCS = pAd->CommonCfg.AddHTInfo.MCSSet[0]+(pAd->CommonCfg.AddHTInfo.MCSSet[1]<<8)+(StbcMcs<<16);
		if ((pAd->CommonCfg.DesiredHtPhy.TxSTBC) && (pRtHtCap->RxSTBC) && (pAd->Antenna.field.TxPath == 2))
			pMaxHtPhy->field.STBC = STBC_USE;
		else
			pMaxHtPhy->field.STBC = STBC_NONE;
	}

	// Decide MAX ht rate.
	if ((pRtHtCap->GF) && (pAd->CommonCfg.DesiredHtPhy.GF))
		pMaxHtPhy->field.MODE = MODE_HTGREENFIELD;
	else
		pMaxHtPhy->field.MODE = MODE_HTMIX;

    if ((pAd->CommonCfg.DesiredHtPhy.ChannelWidth) && (pRtHtCap->ChannelWidth))
		pMaxHtPhy->field.BW = BW_40;
	else
		pMaxHtPhy->field.BW = BW_20;

    if (pMaxHtPhy->field.BW == BW_20)
		pMaxHtPhy->field.ShortGI = (pAd->CommonCfg.DesiredHtPhy.ShortGIfor20 & pRtHtCap->ShortGIfor20);
	else
		pMaxHtPhy->field.ShortGI = (pAd->CommonCfg.DesiredHtPhy.ShortGIfor40 & pRtHtCap->ShortGIfor40);

	for  (i=0;i<8;i++)		
	{
		if ((pActiveHtPhy->MCSSet[1]&(0x80>>i)) && (pDesireHtPhy->MCSSet[1]&(0x80>>i)))
		{
			pMaxHtPhy->field.MCS = 15-i;
			break;
		}
	}
	if (i == 8)
	{
		for  (i=0;i<8;i++)		
		{
			if ((pActiveHtPhy->MCSSet[0]&(0x80>>i))&& (pDesireHtPhy->MCSSet[0]&(0x80>>i)))
			{
				pMaxHtPhy->field.MCS = 7-i;
				break;
			}
		}
	}

	// Copy MIN ht rate.  rt2860???
	pMinHtPhy->field.BW = BW_20;
	pMinHtPhy->field.MCS = 0;
	pMinHtPhy->field.STBC = 0;
	pMinHtPhy->field.ShortGI = 0;
	//If STA assigns fixed rate. update to fixed here.
	
	
	// Decide ht rate
	pHtPhy->field.STBC = pMaxHtPhy->field.STBC;
	pHtPhy->field.BW = pMaxHtPhy->field.BW;
	pHtPhy->field.MODE = pMaxHtPhy->field.MODE;
	pHtPhy->field.MCS = pMaxHtPhy->field.MCS;
	pHtPhy->field.ShortGI = pMaxHtPhy->field.ShortGI;

	// use default now. rt2860
	//RTMP_IO_WRITE32(pAd, HT_BASIC_RATE, BasicMCS);
	if (pDesireHtPhy->MCSSet[0] != 0xff)
		*auto_rate_cur_p = FALSE;
	else
		*auto_rate_cur_p = TRUE;
	
	DBGPRINT(RT_DEBUG_TRACE, (" MlmeUpdateHtTxRates<---.AMsduSize = %d  \n", pAd->CommonCfg.DesiredHtPhy.AmsduSize ));
	DBGPRINT(RT_DEBUG_TRACE,("TX: MCS[0] = %x (choose %d), BW = %d, ShortGI = %d, MODE = %d,  \n", pActiveHtPhy->MCSSet[0],pHtPhy->field.MCS,
		pHtPhy->field.BW, pHtPhy->field.ShortGI, pHtPhy->field.MODE));
	DBGPRINT(RT_DEBUG_TRACE,("MlmeUpdateHtTxRates<=== \n"));
}

// IRQL = DISPATCH_LEVEL
VOID MlmeRadioOff(
	IN PRTMP_ADAPTER pAd)
{
	RT28XX_MLME_RADIO_OFF(pAd);
}

// IRQL = DISPATCH_LEVEL
VOID MlmeRadioOn(
	IN PRTMP_ADAPTER pAd)
{	
	RT28XX_MLME_RADIO_ON(pAd);
}

// ===========================================================================================
// bss_table.c
// ===========================================================================================


/*! \brief initialize BSS table
 *	\param p_tab pointer to the table
 *	\return none
 *	\pre
 *	\post

 IRQL = PASSIVE_LEVEL
 IRQL = DISPATCH_LEVEL
  
 */
VOID BssTableInit(
	IN BSS_TABLE *Tab) 
{
	int i;

	Tab->BssNr = 0;
    Tab->BssOverlapNr = 0;
	for (i = 0; i < MAX_LEN_OF_BSS_TABLE; i++) 
	{
		NdisZeroMemory(&Tab->BssEntry[i], sizeof(BSS_ENTRY));
		Tab->BssEntry[i].Rssi = -127;	// initial the rssi as a minimum value 
	}
}

VOID BATableInit(
	IN PRTMP_ADAPTER pAd, 
    IN BA_TABLE *Tab) 
{
	int i;

	Tab->numAsOriginator = 0;
	Tab->numAsRecipient = 0;
	NdisAllocateSpinLock(&pAd->BATabLock);
	for (i = 0; i < MAX_LEN_OF_BA_REC_TABLE; i++) 
	{
		Tab->BARecEntry[i].REC_BA_Status = Recipient_NONE;
		NdisAllocateSpinLock(&(Tab->BARecEntry[i].RxReRingLock));
	}
	for (i = 0; i < MAX_LEN_OF_BA_ORI_TABLE; i++) 
	{
		Tab->BAOriEntry[i].ORI_BA_Status = Originator_NONE;
	}
}

/*! \brief search the BSS table by SSID
 *	\param p_tab pointer to the bss table
 *	\param ssid SSID string 
 *	\return index of the table, BSS_NOT_FOUND if not in the table
 *	\pre
 *	\post
 *	\note search by sequential search

 IRQL = DISPATCH_LEVEL

 */
ULONG BssTableSearch(
	IN BSS_TABLE *Tab, 
	IN PUCHAR	 pBssid,
	IN UCHAR	 Channel) 
{
	UCHAR i;

	for (i = 0; i < Tab->BssNr; i++) 
	{
		//
		// Some AP that support A/B/G mode that may used the same BSSID on 11A and 11B/G.
		// We should distinguish this case.
		//		
		if ((((Tab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			 ((Tab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(Tab->BssEntry[i].Bssid, pBssid)) 
		{ 
			return i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}

ULONG BssSsidTableSearch(
	IN BSS_TABLE *Tab, 
	IN PUCHAR	 pBssid,
	IN PUCHAR	 pSsid,
	IN UCHAR	 SsidLen,
	IN UCHAR	 Channel) 
{
	UCHAR i;

	for (i = 0; i < Tab->BssNr; i++) 
	{
		//
		// Some AP that support A/B/G mode that may used the same BSSID on 11A and 11B/G.
		// We should distinguish this case.
		//		
		if ((((Tab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			 ((Tab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(Tab->BssEntry[i].Bssid, pBssid) &&
			SSID_EQUAL(pSsid, SsidLen, Tab->BssEntry[i].Ssid, Tab->BssEntry[i].SsidLen)) 
		{ 
			return i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}

ULONG BssTableSearchWithSSID(
	IN BSS_TABLE *Tab, 
	IN PUCHAR	 Bssid,
	IN PUCHAR	 pSsid,
	IN UCHAR	 SsidLen,
	IN UCHAR	 Channel)
{
	UCHAR i;

	for (i = 0; i < Tab->BssNr; i++) 
	{
		if ((((Tab->BssEntry[i].Channel <= 14) && (Channel <= 14)) ||
			((Tab->BssEntry[i].Channel > 14) && (Channel > 14))) &&
			MAC_ADDR_EQUAL(&(Tab->BssEntry[i].Bssid), Bssid) &&
			(SSID_EQUAL(pSsid, SsidLen, Tab->BssEntry[i].Ssid, Tab->BssEntry[i].SsidLen) ||
			(NdisEqualMemory(pSsid, ZeroSsid, SsidLen)) || 
			(NdisEqualMemory(Tab->BssEntry[i].Ssid, ZeroSsid, Tab->BssEntry[i].SsidLen))))
		{ 
			return i;
		}
	}
	return (ULONG)BSS_NOT_FOUND;
}

// IRQL = DISPATCH_LEVEL
VOID BssTableDeleteEntry(
	IN OUT	BSS_TABLE *Tab, 
	IN		PUCHAR	  pBssid,
	IN		UCHAR	  Channel)
{
	UCHAR i, j;

	for (i = 0; i < Tab->BssNr; i++) 
	{
		//printf("comparing %s and %s\n", p_tab->bss[i].ssid, ssid);
		if ((Tab->BssEntry[i].Channel == Channel) && 
			(MAC_ADDR_EQUAL(Tab->BssEntry[i].Bssid, pBssid)))
		{
			for (j = i; j < Tab->BssNr - 1; j++)
			{
				NdisMoveMemory(&(Tab->BssEntry[j]), &(Tab->BssEntry[j + 1]), sizeof(BSS_ENTRY));
			}
			Tab->BssNr -= 1;
			return;
		}
	}
}

/*
	========================================================================
	Routine Description:
		Delete the Originator Entry in BAtable. Or decrease numAs Originator by 1 if needed.
		
	Arguments:
	// IRQL = DISPATCH_LEVEL
	========================================================================
*/
VOID BATableDeleteORIEntry(
	IN OUT	PRTMP_ADAPTER pAd, 
	IN		BA_ORI_ENTRY	*pBAORIEntry)
{

	if (pBAORIEntry->ORI_BA_Status != Originator_NONE)
	{
		NdisAcquireSpinLock(&pAd->BATabLock);
		if (pBAORIEntry->ORI_BA_Status == Originator_Done)
		{
			pAd->BATable.numAsOriginator -= 1;
			DBGPRINT(RT_DEBUG_TRACE, ("BATableDeleteORIEntry numAsOriginator= %ld\n", pAd->BATable.numAsRecipient));
			// Erase Bitmap flag.
		}
		pAd->MacTab.Content[pBAORIEntry->Wcid].TXBAbitmap &= (~(1<<(pBAORIEntry->TID) ));	// If STA mode,  erase flag here
		pAd->MacTab.Content[pBAORIEntry->Wcid].BAOriWcidArray[pBAORIEntry->TID] = 0;	// If STA mode,  erase flag here
		pBAORIEntry->ORI_BA_Status = Originator_NONE;
		pBAORIEntry->Token = 1;
		// Not clear Sequence here.
		NdisReleaseSpinLock(&pAd->BATabLock);
	}
}

#if 0
// IRQL = DISPATCH_LEVEL
VOID BATableDeleteRECEntry(
	IN OUT	PRTMP_ADAPTER pAd, 
	IN		BA_REC_ENTRY	*pBARECEntry)
{
	UCHAR i, k;
	PRTMP_REORDERBUF	pBuf;
	UCHAR	Curindidx;
	ULONG 	offset;	
	ULONG	VALUE;

	if (pBARECEntry->REC_BA_Status == Recipient_Accept)
	{
#ifdef WIN_NDIS
	DBGPRINT(RT_DEBUG_TRACE,("BATableDeleteRECEntry,  wcid=%d \n", 
			pBARECEntry->Wcid));
		

#endif
		NdisAcquireSpinLock(&pAd->BATabLock);
		
		pBARECEntry->REC_BA_Status = Recipient_NONE;

#ifdef WIN_NDIS
		Curindidx = pBARECEntry->Curindidx;
#endif

		/* flush all pending reordering mpdus */
		ba_refresh_reordering_mpdus(pAd, pBARECEntry);

#ifdef WIN_NDIS
		pBARECEntry->RxBufIdxUsed = 0;
		//reset this BARECEntry content
		NdisZeroMemory(pBARECEntry->pAddr, MAC_ADDR_LEN);		
#endif
		// Erase Bitmap flag.
		pBARECEntry->LastIndSeq = 0xffff;
		pBARECEntry->LastIndSeqAtTimer= 0xffff;
		pBARECEntry->BAWinSize = 0;
		// Erase Bitmap flag at software mactable
		pAd->MacTab.Content[pBARECEntry->Wcid].RXBAbitmap &= (~(1<<(pBARECEntry->TID)));
		pAd->MacTab.Content[pBARECEntry->Wcid].BARecWcidArray[pBARECEntry->TID] = 0;	// If STA mode,  erase flag here
		pAd->BATable.numAsRecipient -= 1;
		offset = MAC_WCID_BASE + (pBARECEntry->Wcid) * HW_WCID_ENTRY_SIZE + 4;
		RTMP_IO_READ32(pAd, offset, &VALUE);
		// bitmap field starts at 0x10000 in ASIC WCID table
		VALUE &= (~(0x10000<<(pBARECEntry->TID) ));
		RTMP_IO_WRITE32(pAd, offset, VALUE);
		NdisReleaseSpinLock(&pAd->BATabLock);
	}
}
#endif

#if 0 // BAOriSessionTearDown() & BARecSessionTearDown(); 
/*
	========================================================================
	Routine Description:
		Invoke DELBA Action frame to tear down current BA session.
	Arguments:
		PUCHAR	  pAddr,  MAC Address
		UCHAR TID,	TID <=16
				pAddr and TID together are used to find exact one BARECEntry.
		BOOLEAN ALL  
			if ALL == TRUE. Ignore TID. Delete All BARECEntries with the same pAddr. 
				Used when delete this MAC Client's association.
				if ALL == FALSE, pAddr and TID together are used to find exact one BARECEntry.
					when DELBA sent or DELBA received.
	========================================================================
*/
VOID BATableTearRECEntry(
	IN OUT	PRTMP_ADAPTER pAd, 
	IN		UCHAR TID, 
	IN		UCHAR Wcid, 
	IN		BOOLEAN ALL) 
{
	ULONG	Idx;
	BA_REC_ENTRY	*pBARECEntry;
	UCHAR	TIDStart, TIDEnd, CurTID;
	
	DBGPRINT(RT_DEBUG_TRACE,("BATableTearRECEntry===> Wcid=%d.TID=%d \n", Wcid, TID));

	if (Wcid >= MAX_LEN_OF_MAC_TABLE)
	{
		return;
	}

	//
	// 0 . If to delete all TID with the same pAddr, loop for all TID.
	//
	if (ALL == TRUE)
	{
		TIDStart = 0;
		TIDEnd = 8;
	}
	else
	{
		TIDStart = TID;
		TIDEnd = TID+1;
	}
		
	
	for (CurTID = TIDStart; CurTID < TIDEnd; CurTID++)
	{
		pBARECEntry = NULL;
		//
		// 2. Find corresponding BA Recipient Entry in BATable using Aid we obtained above.
		//

			// if this receiving packet is from SA that is in our OriEntry. Since WCID <9 has direct mapping. no need search.
			Idx = pAd->MacTab.Content[Wcid].BARecWcidArray[CurTID];
		
		pBARECEntry = &pAd->BATable.BARecEntry[Idx];
		//
		// 2.1  Always send DELBA. 
		//
		{
			if (pBARECEntry->TID == CurTID)
				BATableDeleteRECEntry(pAd, pBARECEntry);
			
			if ((ALL == FALSE) || ((ALL == TRUE) && (pBARECEntry->REC_BA_Status == Recipient_Accept)))
		{
			MLME_DELBA_REQ_STRUCT	DelbaReq;	

			NdisZeroMemory(&DelbaReq, sizeof(DelbaReq));
			COPY_MAC_ADDR(DelbaReq.Addr, pAd->MacTab.Content[pBARECEntry->Wcid].Addr);
			DelbaReq.Wcid = Wcid;
			DelbaReq.TID = pBARECEntry->TID;
			DelbaReq.Initiator = RECIPIENT;
			MlmeEnqueue(pAd, ACTION_STATE_MACHINE, MT2_MLME_ORI_DELBA_CATE, sizeof(MLME_DELBA_REQ_STRUCT), (PVOID)&DelbaReq);
			RT28XX_MLME_HANDLER(pAd);
			}
		}
	}		
}

/*
	========================================================================
	Routine Description:
		Invoke DELBA Action frame to tear down current BA session.

	Arguments:
		PUCHAR	  pAddr,  MAC Address
		UCHAR TID,	TID <=16
				pAddr and TID together are used to find exact one BARECEntry.
		BOOLEAN ALL  
			if ALL == TRUE. Ignore TID. Delete All BAORIEntries with the same pAddr. 
				Used when delete this MAC Client's association.
				if ALL == FALSE, pAddr and TID together are used to find exact one BAORIEntry.
					when DELBA sent or DELBA received.
	========================================================================
*/
VOID BATableTearORIEntry(
	IN OUT	PRTMP_ADAPTER pAd, 
	IN		UCHAR TID, 
	IN		UCHAR Wcid, 
	IN		BOOLEAN bForceDelete, 
	IN		BOOLEAN ALL) 
{
	ULONG	Idx = 0;
	BOOLEAN		Cancelled;
	BA_ORI_ENTRY	*pBAORIEntry;
	UCHAR	TIDStart, TIDEnd, CurTID;

	DBGPRINT(RT_DEBUG_TRACE,("BATableTearORIEntry===> Wcid=%d.TID=%d, numAsOriginator= %d \n", Wcid, TID,pAd->BATable.numAsOriginator));
	if (Wcid >= MAX_LEN_OF_MAC_TABLE)
	{
		return;
	}
	//
	// 0 . If wish to delete all TID with the same pAddr, loop for all TID.
	//
	if (ALL == TRUE)
	{
		TIDStart = 0;
		TIDEnd = 8;
	}
	else
	{
		TIDStart = TID;
		TIDEnd = TID+1;
	}
	
	for (CurTID = TIDStart; CurTID < TIDEnd; CurTID++)
	{
		//
		// 2. Locate corresponding BA Originator Entry in BA Table with the (pAddr,TID).
		//
			Idx = pAd->MacTab.Content[Wcid].BAOriWcidArray[CurTID];
			pBAORIEntry = &pAd->BATable.BAOriEntry[Idx];
		//
		// 2.1  Always send DELBA. 
		//
			{
			MLME_DELBA_REQ_STRUCT	DelbaReq;	

			if (pBAORIEntry->TID == CurTID)
				BATableDeleteORIEntry(pAd, pBAORIEntry);
			if ((ALL == FALSE) || ((ALL == TRUE) && (pBAORIEntry->ORI_BA_Status == Originator_Done)))
			{
			NdisZeroMemory(&DelbaReq, sizeof(DelbaReq));
			COPY_MAC_ADDR(DelbaReq.Addr, pAd->MacTab.Content[pBAORIEntry->Wcid].Addr);
			DelbaReq.Wcid = Wcid;
			DelbaReq.TID = pBAORIEntry->TID;
			DelbaReq.Initiator = ORIGINATOR;
			MlmeEnqueue(pAd, ACTION_STATE_MACHINE, MT2_MLME_ORI_DELBA_CATE, sizeof(MLME_DELBA_REQ_STRUCT), (PVOID)&DelbaReq);
			RT28XX_MLME_HANDLER(pAd);
			}
			if ((pAd->CommonCfg.BACapability.field.AutoBA == 1) && (bForceDelete == FALSE))
			{
				BATableInsertEntry(pAd, Wcid,  0, 0x1, CurTID, pAd->MacTab.Content[Wcid].BaSizeInUse, Originator_SetAutoAdd,  FALSE);
				pAd->MacTab.Content[Wcid].TXAutoBAbitmap |= ((1<<(pBAORIEntry->TID) ));	// If STA mode,  erase flag here
			}
			
			}
		}
}
#endif // replaced by BAOriSessionTearDown() & BARecSessionTearDown(); 

/*! \brief
 *	\param 
 *	\return
 *	\pre
 *	\post
	 
 IRQL = DISPATCH_LEVEL
 
 */
VOID BssEntrySet(
	IN PRTMP_ADAPTER	pAd, 
	OUT BSS_ENTRY *pBss, 
	IN PUCHAR pBssid, 
	IN CHAR Ssid[], 
	IN UCHAR SsidLen, 
	IN UCHAR BssType, 
	IN USHORT BeaconPeriod, 
	IN PCF_PARM pCfParm, 
	IN USHORT AtimWin, 
	IN USHORT CapabilityInfo, 
	IN UCHAR SupRate[], 
	IN UCHAR SupRateLen,
	IN UCHAR ExtRate[], 
	IN UCHAR ExtRateLen,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN ADD_HT_INFO_IE *pAddHtInfo,	// AP might use this additional ht info IE 
	IN UCHAR			HtCapabilityLen,
	IN UCHAR			AddHtInfoLen,
	IN UCHAR			NewExtChanOffset,
	IN UCHAR Channel,
	IN CHAR Rssi,
	IN LARGE_INTEGER TimeStamp,
	IN UCHAR CkipFlag,
	IN PEDCA_PARM pEdcaParm,
	IN PQOS_CAPABILITY_PARM pQosCapability,
	IN PQBSS_LOAD_PARM pQbssLoad,
	IN USHORT LengthVIE,	
	IN PNDIS_802_11_VARIABLE_IEs pVIE) 
{
	COPY_MAC_ADDR(pBss->Bssid, pBssid);
	// Default Hidden SSID to be TRUE, it will be turned to FALSE after coping SSID
	pBss->Hidden = 1;	
	if (SsidLen > 0)
	{
		// For hidden SSID AP, it might send beacon with SSID len equal to 0
		// Or send beacon /probe response with SSID len matching real SSID length,
		// but SSID is all zero. such as "00-00-00-00" with length 4.
		// We have to prevent this case overwrite correct table
		if (NdisEqualMemory(Ssid, ZeroSsid, SsidLen) == 0)
		{
		    NdisZeroMemory(pBss->Ssid, MAX_LEN_OF_SSID);
			NdisMoveMemory(pBss->Ssid, Ssid, SsidLen);
			pBss->SsidLen = SsidLen;
			pBss->Hidden = 0;
		}
	}
	pBss->BssType = BssType;
	pBss->BeaconPeriod = BeaconPeriod;
	if (BssType == BSS_INFRA) 
	{
		if (pCfParm->bValid) 
		{
			pBss->CfpCount = pCfParm->CfpCount;
			pBss->CfpPeriod = pCfParm->CfpPeriod;
			pBss->CfpMaxDuration = pCfParm->CfpMaxDuration;
			pBss->CfpDurRemaining = pCfParm->CfpDurRemaining;
		}
	} 
	else 
	{
		pBss->AtimWin = AtimWin;
	}

	pBss->CapabilityInfo = CapabilityInfo;
	// The privacy bit indicate security is ON, it maight be WEP, TKIP or AES
	// Combine with AuthMode, they will decide the connection methods.
	pBss->Privacy = CAP_IS_PRIVACY_ON(pBss->CapabilityInfo);
	ASSERT(SupRateLen <= MAX_LEN_OF_SUPPORTED_RATES);
	NdisMoveMemory(pBss->SupRate, SupRate, SupRateLen);
	pBss->SupRateLen = SupRateLen;
	ASSERT(ExtRateLen <= MAX_LEN_OF_SUPPORTED_RATES);
	NdisMoveMemory(pBss->ExtRate, ExtRate, ExtRateLen);
	NdisMoveMemory(&pBss->HtCapability, pHtCapability, HtCapabilityLen);
	NdisMoveMemory(&pBss->AddHtInfo, pAddHtInfo, AddHtInfoLen);
	pBss->NewExtChanOffset = NewExtChanOffset;
	pBss->ExtRateLen = ExtRateLen;
	pBss->Channel = Channel;
	pBss->CentralChannel = Channel;
	pBss->Rssi = Rssi;
	// Update CkipFlag. if not exists, the value is 0x0
	pBss->CkipFlag = CkipFlag;

	// New for microsoft Fixed IEs
	NdisMoveMemory(pBss->FixIEs.Timestamp, &TimeStamp, 8);
	pBss->FixIEs.BeaconInterval = BeaconPeriod;
	pBss->FixIEs.Capabilities = CapabilityInfo;

	// New for microsoft Variable IEs
	if (LengthVIE != 0)
	{
		pBss->VarIELen = LengthVIE;
		NdisMoveMemory(pBss->VarIEs, pVIE, pBss->VarIELen);
	}
	else
	{
		pBss->VarIELen = 0;
	}

	if (HtCapabilityLen> 0)
	{
		pBss->HtCapabilityLen = HtCapabilityLen;
		NdisMoveMemory(&pBss->HtCapability, pHtCapability, HtCapabilityLen);
	if (AddHtInfoLen > 0)
	{
		pBss->AddHtInfoLen = AddHtInfoLen;
		NdisMoveMemory(&pBss->AddHtInfo, pAddHtInfo, AddHtInfoLen);
		
 			if ((pAddHtInfo->ControlChan > 2)&& (pAddHtInfo->AddHtInfo.ExtChanOffset == EXTCHA_BELOW) && (pHtCapability->HtCapInfo.ChannelWidth == BW_40))
 			{
 				pBss->CentralChannel = pAddHtInfo->ControlChan - 2;
 			}
 			else if ((pAddHtInfo->AddHtInfo.ExtChanOffset == EXTCHA_ABOVE) && (pHtCapability->HtCapInfo.ChannelWidth == BW_40))
		{
 				pBss->CentralChannel = pAddHtInfo->ControlChan + 2;
		}
	}
	else
		pBss->AddHtInfoLen = 0;
	}
	else
		pBss->HtCapabilityLen = 0;
	
	BssCipherParse(pBss);

	// new for QOS
	if (pEdcaParm)
		NdisMoveMemory(&pBss->EdcaParm, pEdcaParm, sizeof(EDCA_PARM));
	else
		pBss->EdcaParm.bValid = FALSE;
	if (pQosCapability)
		NdisMoveMemory(&pBss->QosCapability, pQosCapability, sizeof(QOS_CAPABILITY_PARM));
	else
		pBss->QosCapability.bValid = FALSE;
	if (pQbssLoad)
		NdisMoveMemory(&pBss->QbssLoad, pQbssLoad, sizeof(QBSS_LOAD_PARM));
	else
		pBss->QbssLoad.bValid = FALSE;

}

/*! 
 *	\brief insert an entry into the bss table
 *	\param p_tab The BSS table
 *	\param Bssid BSSID
 *	\param ssid SSID
 *	\param ssid_len Length of SSID
 *	\param bss_type
 *	\param beacon_period
 *	\param timestamp
 *	\param p_cf
 *	\param atim_win
 *	\param cap
 *	\param rates
 *	\param rates_len
 *	\param channel_idx
 *	\return none
 *	\pre
 *	\post
 *	\note If SSID is identical, the old entry will be replaced by the new one
	 
 IRQL = DISPATCH_LEVEL
 
 */
ULONG BssTableSetEntry(
	IN	PRTMP_ADAPTER	pAd, 
	OUT BSS_TABLE *Tab, 
	IN PUCHAR pBssid, 
	IN CHAR Ssid[], 
	IN UCHAR SsidLen, 
	IN UCHAR BssType, 
	IN USHORT BeaconPeriod, 
	IN CF_PARM *CfParm, 
	IN USHORT AtimWin, 
	IN USHORT CapabilityInfo, 
	IN UCHAR SupRate[],
	IN UCHAR SupRateLen,
	IN UCHAR ExtRate[],
	IN UCHAR ExtRateLen,
	IN HT_CAPABILITY_IE *pHtCapability,
	IN ADD_HT_INFO_IE *pAddHtInfo,	// AP might use this additional ht info IE 
	IN UCHAR			HtCapabilityLen,
	IN UCHAR			AddHtInfoLen,
	IN UCHAR			NewExtChanOffset,
	IN UCHAR ChannelNo,
	IN CHAR Rssi,
	IN LARGE_INTEGER TimeStamp,
	IN UCHAR CkipFlag,
	IN PEDCA_PARM pEdcaParm,
	IN PQOS_CAPABILITY_PARM pQosCapability,
	IN PQBSS_LOAD_PARM pQbssLoad,
	IN USHORT LengthVIE,	
	IN PNDIS_802_11_VARIABLE_IEs pVIE)
{
	ULONG	Idx;

	Idx = BssTableSearchWithSSID(Tab, pBssid,  Ssid, SsidLen, ChannelNo);
	if (Idx == BSS_NOT_FOUND) 
	{
		if (Tab->BssNr >= MAX_LEN_OF_BSS_TABLE)
	    {
			//
			// It may happen when BSS Table was full.
			// The desired AP will not be added into BSS Table
			// In this case, if we found the desired AP then overwrite BSS Table.
			//
			if(!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
			{
				if (MAC_ADDR_EQUAL(pAd->MlmeAux.Bssid, pBssid) ||
					SSID_EQUAL(pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen, Ssid, SsidLen))
				{
					Idx = Tab->BssOverlapNr;
					BssEntrySet(pAd, &Tab->BssEntry[Idx], pBssid, Ssid, SsidLen, BssType, BeaconPeriod, CfParm, AtimWin, 
						CapabilityInfo, SupRate, SupRateLen, ExtRate, ExtRateLen,pHtCapability, pAddHtInfo,HtCapabilityLen, AddHtInfoLen,
						NewExtChanOffset, ChannelNo, Rssi, TimeStamp, CkipFlag, pEdcaParm, pQosCapability, pQbssLoad, LengthVIE, pVIE);
                    Tab->BssOverlapNr = (Tab->BssOverlapNr++) % MAX_LEN_OF_BSS_TABLE;
				}
				return Idx;
			}
			else
			{
			return BSS_NOT_FOUND;
			}
		}
		Idx = Tab->BssNr;
		BssEntrySet(pAd, &Tab->BssEntry[Idx], pBssid, Ssid, SsidLen, BssType, BeaconPeriod, CfParm, AtimWin, 
					CapabilityInfo, SupRate, SupRateLen, ExtRate, ExtRateLen,pHtCapability, pAddHtInfo,HtCapabilityLen, AddHtInfoLen,
					NewExtChanOffset, ChannelNo, Rssi, TimeStamp, CkipFlag, pEdcaParm, pQosCapability, pQbssLoad, LengthVIE, pVIE);
		Tab->BssNr++;
	} 
	else
	{
		BssEntrySet(pAd, &Tab->BssEntry[Idx], pBssid, Ssid, SsidLen, BssType, BeaconPeriod,CfParm, AtimWin, 
					CapabilityInfo, SupRate, SupRateLen, ExtRate, ExtRateLen,pHtCapability, pAddHtInfo,HtCapabilityLen, AddHtInfoLen,
					NewExtChanOffset, ChannelNo, Rssi, TimeStamp, CkipFlag, pEdcaParm, pQosCapability, pQbssLoad, LengthVIE, pVIE);
	}

	return Idx;
}

#if 0
/*
============================================================
Description:  

If insert as recipient, it means accept the ADDBA request, and start receive A-MPDU right now.

If insert as Originator, the initial state is wait_for_response, if ADDBA Rsp not receive or feedback status code is not zero,
the baentry will be deleted, otherwise, change state to originaotr_done and start send A-MPDU.

============================================================
*/
VOID BATableInsertEntry(
	IN	PRTMP_ADAPTER	pAd, 
	IN USHORT Aid, 
	IN USHORT		TimeOutValue,
	IN USHORT		StartingSeq,
	IN UCHAR TID, 
	IN UCHAR BAWinSize, 
	IN UCHAR OriginatorStatus, 
	IN BOOLEAN Recipient)
{
	UCHAR	Idx;
	UCHAR 	HashIdx, index;
	UCHAR i;
	MAC_TABLE_ENTRY *pEntry;
	ULONG 	offset;
	ULONG	Value;
	PRTMP_REORDERBUF	pDmaBuf;

	if (Aid >= MAX_LEN_OF_MAC_TABLE)
		return;
	pEntry = &pAd->MacTab.Content[Aid];
	if (Recipient == TRUE)
	{
		BA_REC_ENTRY	*pBAEntry = NULL, *pCurrEntry;
		UCHAR	ZeroBSSID[6]={0,0,0,0,0,0};

		DBGPRINT(RT_DEBUG_TRACE, ("BATableInsert Recipient Entry ===>Aid = %d\n", Aid));
		// concurrent Recipient # is limited by how many reordering buffer we allocate at initialization stage.
		if (pAd->BATable.numAsRecipient >= MAX_BARECI_SESSION) 
		{
			return;
		}
		
		Idx = pAd->MacTab.Content[Aid].BARecWcidArray[TID];
		if ((pAd->MacTab.Content[Aid].BARecWcidArray[TID]) == 0)
		{
			for (i=1; i <MAX_LEN_OF_BA_REC_TABLE;i++)
				{
					pBAEntry =& pAd->BATable.BARecEntry[i];
					if (pBAEntry->REC_BA_Status == Recipient_NONE)
					{
						// find vacant entry with Zero Addr,
						Idx = i;
					pAd->MacTab.Content[Aid].BARecWcidArray[TID] = i;
						pBAEntry =& pAd->BATable.BARecEntry[Idx];
						DBGPRINT(RT_DEBUG_TRACE, ("BATableInsertEntry[%d] ===>Wcid = %d\n", Idx, Aid));
						break;
					}
					pBAEntry = NULL;
				}
			}
		else if ((pAd->BATable.BARecEntry[Idx].REC_BA_Status == Recipient_Accept))
		{
			if ((pAd->BATable.BARecEntry[Idx].BAWinSize != BAWinSize))
			{
				// Update existing entry's parameter. Need to tear down current BA session and set BA session again.
				// shoudle we handle here? why not ask GUI to tear duplicate entry then insert.
			}
			return;
		}
		// Start fill in parameters.
		if ((pBAEntry != NULL))
		{
			NdisAcquireSpinLock(&pAd->BATabLock);
			if (pBAEntry->REC_BA_Status == Recipient_NONE)
			{
				pAd->BATable.numAsRecipient++;
				// Allocate reordering buffer to this Recipient Entry.
				for (index = 0; index < MAX_BARECI_SESSION; index++)
				{
					if (pAd->LocalRxReorderBuf[index].InUse == FALSE)
					{
						pAd->LocalRxReorderBuf[index].InUse = TRUE;
						pAd->LocalRxReorderBuf[index].ByBaRecIndex = Idx;
						pBAEntry->RxBufIdxUsed = index;
						break;
					}
				}
			}
			pBAEntry->Wcid = (UCHAR)Aid;
			pBAEntry->BAWinSize = (UCHAR)BAWinSize;
			pBAEntry->Curindidx = 0;
			//pBAEntry->NumOfRxPkt = 0;
			pBAEntry->pAdapter = pAd;
			pBAEntry->TID = TID;
			pBAEntry->TimeOutValue = TimeOutValue;
			pBAEntry->REC_BA_Status = Recipient_Accept;
			//
			pBAEntry->LastIndSeq = StartingSeq;
			pBAEntry->LastIndSeqAtTimer= StartingSeq;
			// Set Bitmap flag.
			pAd->MacTab.Content[Aid].RXBAbitmap |= (1<<TID );

			// Set BA session mask in WCID table.
			offset = MAC_WCID_BASE + Aid * HW_WCID_ENTRY_SIZE + 4;
			RTMP_IO_READ32(pAd, offset, &Value);
			Value |= (0x10000<<TID);
			RTMP_IO_WRITE32(pAd, offset, Value);

			DBGPRINT(RT_DEBUG_TRACE,("Recipient : pAddr =%x, %x,  %x, %x, %x, %x. assign %dth reorder buf\n", 
				pAd->MacTab.Content[Aid].Addr[0], pAd->MacTab.Content[Aid].Addr[1], pAd->MacTab.Content[Aid].Addr[2], pAd->MacTab.Content[Aid].Addr[3], pAd->MacTab.Content[Aid].Addr[4], 
				pAd->MacTab.Content[Aid].Addr[5], pBAEntry->RxBufIdxUsed));
			DBGPRINT(RT_DEBUG_TRACE,("MACEntry[%d]StartingSeq = %x, RXBAbitmap = 0x%x. BARecWcidArray=%d\n", 
				Aid, StartingSeq, pAd->MacTab.Content[Aid].RXBAbitmap, pAd->MacTab.Content[Aid].BARecWcidArray[TID]));

			NdisReleaseSpinLock(&pAd->BATabLock);
		}
	} 
	else
	{
		BA_ORI_ENTRY	*pBAEntry = NULL, *pCurrEntry;
		// if FULL, return
		if (pAd->BATable.numAsOriginator >= MAX_LEN_OF_BA_ORI_TABLE) 
			return;

		Idx = pAd->MacTab.Content[Aid].BAOriWcidArray[TID];
		//Since we want to insert this Entry, 
		if ((pAd->MacTab.Content[Aid].BAOriWcidArray[TID]) == 0)
		{
			for (i = 1; i < MAX_LEN_OF_BA_ORI_TABLE;i++)
				{
					pBAEntry =&pAd->BATable.BAOriEntry[i];
					if ((pBAEntry->ORI_BA_Status == Originator_NONE))
					{
						// Find an empty pair. Use this.
						Idx = i;
						//Record this index. So at Hardtransmit, only need to search MAC Etnry wcid.
						//Doesn't need to search the corresponding BA Orient entry .
						pAd->MacTab.Content[Aid].BAOriWcidArray[TID] = i;
					NdisAcquireSpinLock(&pAd->BATabLock);
					pBAEntry->ORI_BA_Status = OriginatorStatus;
					pBAEntry->BAWinSize = BAWinSize;
					pBAEntry->Token = 1;	// Start from non-zero value.
					NdisReleaseSpinLock(&pAd->BATabLock);
					DBGPRINT(RT_DEBUG_TRACE, ("BATableInsertEntry [%d]\n",i));
					break;
				}
				else if ((pBAEntry->ORI_BA_Status == Originator_SetAutoAdd) && (OriginatorStatus == Originator_WaitRes))
				{
					// Set via OID. OID has higher priority of auto add originator. :)
					// Find an empty pair. Use this.
					Idx = i;
					//Record this index. So at Hardtransmit, only need to search MAC Etnry wcid.
					//Doesn't need to search the corresponding BA Orient entry .
					pAd->MacTab.Content[Aid].BAOriWcidArray[TID] = i;
					NdisAcquireSpinLock(&pAd->BATabLock);
					pBAEntry->ORI_BA_Status = OriginatorStatus;
					pBAEntry->BAWinSize = BAWinSize;
					pBAEntry->Token = 1;	// Start from non-zero value.
					NdisReleaseSpinLock(&pAd->BATabLock);
					DBGPRINT(RT_DEBUG_TRACE, ("BATableInsertEntry[%d]. BAWinSize= %d\n",i, BAWinSize));
						break;
					}
					pBAEntry = NULL;
				}
		}
		else if ((pAd->BATable.BAOriEntry[Idx].ORI_BA_Status == Originator_WaitRes))
		{
			// handshake in progress. return.
			DBGPRINT(RT_DEBUG_TRACE, ("BATableInsertEntry handshake in progress\n"));
			return;
		}
		else if ((pAd->BATable.BAOriEntry[Idx].ORI_BA_Status == Originator_Done))
		{
			if ((pAd->BATable.BAOriEntry[Idx].BAWinSize != BAWinSize))
			{
				// Update existing entry's parameter. Need to tear down current BA session and set BA session again.
				// shoudle we handle here? why not ask GUI to tear duplicate entry then insert.
			}
			DBGPRINT(RT_DEBUG_TRACE, ("BATableInsertEntry - already exists. Aid=%d, BAIdx=%d \n", Aid, Idx));
			return;
		}

		// Fill in parameters.
		if ((pBAEntry != NULL))
		{
			NdisAcquireSpinLock(&pAd->BATabLock);
			pBAEntry->Wcid = (UCHAR)Aid;
			pBAEntry->BAWinSize = (UCHAR)BAWinSize;
			pBAEntry->pAdapter = pAd;
			pBAEntry->TID = TID;
			pBAEntry->Token = 0;
			// set Status to Done immediately. 
			DBGPRINT(RT_DEBUG_TRACE,("MACEntry[%d]TXBAbitmap = 0x%x. BAOriWcidArray=%d\n", 
				Aid, pAd->MacTab.Content[Aid].TXBAbitmap, pAd->MacTab.Content[Aid].BAOriWcidArray[TID]));
			NdisReleaseSpinLock(&pAd->BATabLock);
		}
	}
	DBGPRINT(RT_DEBUG_TRACE, ("<===BATableInsertEntry \n"));
}
#endif



VOID BssCipherParse(
	IN OUT	PBSS_ENTRY	pBss)
{
	PEID_STRUCT 		 pEid;
	PUCHAR				pTmp;
	PRSN_IE_HEADER_STRUCT			pRsnHeader;
	PCIPHER_SUITE_STRUCT			pCipher;
	PAKM_SUITE_STRUCT				pAKM;
	USHORT							Count;
	INT								Length;
	NDIS_802_11_ENCRYPTION_STATUS	TmpCipher;

	//
	// WepStatus will be reset later, if AP announce TKIP or AES on the beacon frame.
	//
	if (pBss->Privacy)
	{
		pBss->WepStatus 	= Ndis802_11WEPEnabled;
	}
	else
	{
		pBss->WepStatus 	= Ndis802_11WEPDisabled;
	}
	// Set default to disable & open authentication before parsing variable IE
	pBss->AuthMode		= Ndis802_11AuthModeOpen;
	pBss->AuthModeAux	= Ndis802_11AuthModeOpen;

	// Init WPA setting
	pBss->WPA.PairCipher	= Ndis802_11WEPDisabled;
	pBss->WPA.PairCipherAux = Ndis802_11WEPDisabled;
	pBss->WPA.GroupCipher	= Ndis802_11WEPDisabled;
	pBss->WPA.RsnCapability = 0;
	pBss->WPA.bMixMode		= FALSE;

	// Init WPA2 setting
	pBss->WPA2.PairCipher	 = Ndis802_11WEPDisabled;
	pBss->WPA2.PairCipherAux = Ndis802_11WEPDisabled;
	pBss->WPA2.GroupCipher	 = Ndis802_11WEPDisabled;
	pBss->WPA2.RsnCapability = 0;
	pBss->WPA2.bMixMode 	 = FALSE;
	Length = (INT) pBss->VarIELen;

	while (Length > 0)
	{
		// Parse cipher suite base on WPA1 & WPA2, they should be parsed differently
		pTmp = ((PUCHAR) pBss->VarIEs) + pBss->VarIELen - Length;
		pEid = (PEID_STRUCT) pTmp;
		switch (pEid->Eid)
		{
			case IE_WPA:
				//Parse Cisco IE_WPA (LEAP, CCKM, etc.)
				if ( NdisEqualMemory((pTmp+8), CISCO_OUI, 3))
				{
					pTmp   += 11;
					switch (*pTmp)
					{
						case 1:
						case 5:	// Although WEP is not allowed in WPA related auth mode, we parse it anyway
							pBss->WepStatus = Ndis802_11Encryption1Enabled;
							pBss->WPA.PairCipher = Ndis802_11Encryption1Enabled;
							pBss->WPA.GroupCipher = Ndis802_11Encryption1Enabled;
							break;
						case 2:
							pBss->WepStatus = Ndis802_11Encryption2Enabled;
							pBss->WPA.PairCipher = Ndis802_11Encryption1Enabled;
							pBss->WPA.GroupCipher = Ndis802_11Encryption1Enabled;
							break;
						case 4:
							pBss->WepStatus = Ndis802_11Encryption3Enabled;
							pBss->WPA.PairCipher = Ndis802_11Encryption1Enabled;
							pBss->WPA.GroupCipher = Ndis802_11Encryption1Enabled;
							break;
						default:
							break;
					}	
	
					// if Cisco IE_WPA, break
					break;
				}
				else if (NdisEqualMemory(pEid->Octet, SES_OUI, 3) && (pEid->Len == 7))
				{
					pBss->bSES = TRUE;
					break;
				}				
				else if (NdisEqualMemory(pEid->Octet, WPA_OUI, 4) != 1)
				{
					// if unsupported vendor specific IE
					break;
				}				
				// Skip OUI, version, and multicast suite
				// This part should be improved in the future when AP supported multiple cipher suite.
				// For now, it's OK since almost all APs have fixed cipher suite supported.
				// pTmp = (PUCHAR) pEid->Octet;
				pTmp   += 11;

				// Cipher Suite Selectors from Spec P802.11i/D3.2 P26.
				//	Value	   Meaning
				//	0			None 
				//	1			WEP-40
				//	2			Tkip
				//	3			WRAP
				//	4			AES
				//	5			WEP-104
				// Parse group cipher
				switch (*pTmp)
				{
					case 1:
					case 5:	// Although WEP is not allowed in WPA related auth mode, we parse it anyway
						pBss->WPA.GroupCipher = Ndis802_11Encryption1Enabled;
						break;
					case 2:
						pBss->WPA.GroupCipher = Ndis802_11Encryption2Enabled;
						break;
					case 4:
						pBss->WPA.GroupCipher = Ndis802_11Encryption3Enabled;
						break;
					default:
						break;
				}
				// number of unicast suite
				pTmp   += 1;

				// skip all unicast cipher suites
				//Count = *(PUSHORT) pTmp;				
				Count = (pTmp[1]<<8) + pTmp[0];
				pTmp   += sizeof(USHORT);

				// Parsing all unicast cipher suite
				while (Count > 0)
				{
					// Skip OUI
					pTmp += 3;
					TmpCipher = Ndis802_11WEPDisabled;
					switch (*pTmp)
					{
						case 1:
						case 5: // Although WEP is not allowed in WPA related auth mode, we parse it anyway
							TmpCipher = Ndis802_11Encryption1Enabled;
							break;
						case 2:
							TmpCipher = Ndis802_11Encryption2Enabled;
							break;
						case 4:
							TmpCipher = Ndis802_11Encryption3Enabled;
							break;
						default:
							break;
					}
					if (TmpCipher > pBss->WPA.PairCipher)
					{
						// Move the lower cipher suite to PairCipherAux
						pBss->WPA.PairCipherAux = pBss->WPA.PairCipher;
						pBss->WPA.PairCipher	= TmpCipher;
					}
					else
					{
						pBss->WPA.PairCipherAux = TmpCipher;
					}
					pTmp++;
					Count--;
				}
				
				// 4. get AKM suite counts
				//Count	= *(PUSHORT) pTmp;
				Count = (pTmp[1]<<8) + pTmp[0];
				pTmp   += sizeof(USHORT);
				pTmp   += 3;
				
				switch (*pTmp)
				{
					case 1:
						// Set AP support WPA mode
						if (pBss->AuthMode == Ndis802_11AuthModeOpen)
							pBss->AuthMode = Ndis802_11AuthModeWPA;
						else
							pBss->AuthModeAux = Ndis802_11AuthModeWPA;
						break;
					case 2:
						// Set AP support WPA mode
						if (pBss->AuthMode == Ndis802_11AuthModeOpen)
							pBss->AuthMode = Ndis802_11AuthModeWPAPSK;
						else
							pBss->AuthModeAux = Ndis802_11AuthModeWPAPSK;
						break;
					default:
						break;
				}
				pTmp   += 1;

				// Fixed for WPA-None
				if (pBss->BssType == BSS_ADHOC)
				{
					pBss->AuthMode	  = Ndis802_11AuthModeWPANone;
					pBss->AuthModeAux = Ndis802_11AuthModeWPANone;
					pBss->WepStatus   = pBss->WPA.GroupCipher;
					// Patched bugs for old driver
					if (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled)
						pBss->WPA.PairCipherAux = pBss->WPA.GroupCipher;
				}
				else
					pBss->WepStatus   = pBss->WPA.PairCipher;					
				
				// Check the Pair & Group, if different, turn on mixed mode flag
				if (pBss->WPA.GroupCipher != pBss->WPA.PairCipher)
					pBss->WPA.bMixMode = TRUE;
				
				break;

			case IE_RSN:
				pRsnHeader = (PRSN_IE_HEADER_STRUCT) pTmp;
				
				// 0. Version must be 1
				if (le2cpu16(pRsnHeader->Version) != 1)
					break;
				pTmp   += sizeof(RSN_IE_HEADER_STRUCT);

				// 1. Check group cipher
				pCipher = (PCIPHER_SUITE_STRUCT) pTmp;
				if (!RTMPEqualMemory(pTmp, RSN_OUI, 3))
					break;

				// Parse group cipher
				switch (pCipher->Type)
				{
					case 1:
					case 5:	// Although WEP is not allowed in WPA related auth mode, we parse it anyway
						pBss->WPA2.GroupCipher = Ndis802_11Encryption1Enabled;
						break;
					case 2:
						pBss->WPA2.GroupCipher = Ndis802_11Encryption2Enabled;
						break;
					case 4:
						pBss->WPA2.GroupCipher = Ndis802_11Encryption3Enabled;
						break;
					default:
						break;
				}
				// set to correct offset for next parsing
				pTmp   += sizeof(CIPHER_SUITE_STRUCT);

				// 2. Get pairwise cipher counts
				//Count = *(PUSHORT) pTmp;
				Count = (pTmp[1]<<8) + pTmp[0];
				pTmp   += sizeof(USHORT);			

				// 3. Get pairwise cipher
				// Parsing all unicast cipher suite
				while (Count > 0)
				{
					// Skip OUI
					pCipher = (PCIPHER_SUITE_STRUCT) pTmp;
					TmpCipher = Ndis802_11WEPDisabled;
					switch (pCipher->Type)
					{
						case 1:
						case 5: // Although WEP is not allowed in WPA related auth mode, we parse it anyway
							TmpCipher = Ndis802_11Encryption1Enabled;
							break;
						case 2:
							TmpCipher = Ndis802_11Encryption2Enabled;
							break;
						case 4:
							TmpCipher = Ndis802_11Encryption3Enabled;
							break;
						default:
							break;
					}
					if (TmpCipher > pBss->WPA2.PairCipher)
					{
						// Move the lower cipher suite to PairCipherAux
						pBss->WPA2.PairCipherAux = pBss->WPA2.PairCipher;
						pBss->WPA2.PairCipher	 = TmpCipher;
					}
					else
					{
						pBss->WPA2.PairCipherAux = TmpCipher;
					}
					pTmp += sizeof(CIPHER_SUITE_STRUCT);
					Count--;
				}
				
				// 4. get AKM suite counts
				//Count	= *(PUSHORT) pTmp;
				Count = (pTmp[1]<<8) + pTmp[0];
				pTmp   += sizeof(USHORT);

				// 5. Get AKM ciphers
				pAKM = (PAKM_SUITE_STRUCT) pTmp;
				if (!RTMPEqualMemory(pTmp, RSN_OUI, 3))
					break;

				switch (pAKM->Type)
				{
					case 1:
						// Set AP support WPA mode
						if (pBss->AuthMode == Ndis802_11AuthModeOpen)
							pBss->AuthMode = Ndis802_11AuthModeWPA2;
						else
							pBss->AuthModeAux = Ndis802_11AuthModeWPA2;
						break;
					case 2:
						// Set AP support WPA mode
						if (pBss->AuthMode == Ndis802_11AuthModeOpen)
							pBss->AuthMode = Ndis802_11AuthModeWPA2PSK;
						else
							pBss->AuthModeAux = Ndis802_11AuthModeWPA2PSK;
						break;
					default:
						break;
				}
				pTmp   += (Count * sizeof(AKM_SUITE_STRUCT));

				// Fixed for WPA-None
				if (pBss->BssType == BSS_ADHOC)
				{
					pBss->AuthMode = Ndis802_11AuthModeWPANone;
					pBss->AuthModeAux = Ndis802_11AuthModeWPANone;
					pBss->WPA.PairCipherAux = pBss->WPA2.PairCipherAux;
					pBss->WPA.GroupCipher	= pBss->WPA2.GroupCipher;
					pBss->WepStatus 		= pBss->WPA.GroupCipher;
					// Patched bugs for old driver
					if (pBss->WPA.PairCipherAux == Ndis802_11WEPDisabled)
						pBss->WPA.PairCipherAux = pBss->WPA.GroupCipher;
				}
				pBss->WepStatus   = pBss->WPA2.PairCipher;					
				
				// 6. Get RSN capability
				//pBss->WPA2.RsnCapability = *(PUSHORT) pTmp;
				pBss->WPA2.RsnCapability = (pTmp[1]<<8) + pTmp[0];
				pTmp += sizeof(USHORT);
				
				// Check the Pair & Group, if different, turn on mixed mode flag
				if (pBss->WPA2.GroupCipher != pBss->WPA2.PairCipher)
					pBss->WPA2.bMixMode = TRUE;
				
				break;

			default:
				break;
		}
		Length -= (pEid->Len + 2);
	}
}

// ===========================================================================================
// mac_table.c
// ===========================================================================================

/*! \brief generates a random mac address value for IBSS BSSID
 *	\param Addr the bssid location
 *	\return none
 *	\pre
 *	\post
 */
VOID MacAddrRandomBssid(
	IN PRTMP_ADAPTER pAd, 
	OUT PUCHAR pAddr) 
{
	INT i;

	for (i = 0; i < MAC_ADDR_LEN; i++) 
	{
		pAddr[i] = RandomByte(pAd);
	}

	pAddr[0] = (pAddr[0] & 0xfe) | 0x02;  // the first 2 bits must be 01xxxxxxxx
}

/*! \brief init the management mac frame header
 *	\param p_hdr mac header
 *	\param subtype subtype of the frame
 *	\param p_ds destination address, don't care if it is a broadcast address
 *	\return none
 *	\pre the station has the following information in the pAd->StaCfg
 *	 - bssid
 *	 - station address
 *	\post
 *	\note this function initializes the following field

 IRQL = PASSIVE_LEVEL
 IRQL = DISPATCH_LEVEL
  
 */
VOID MgtMacHeaderInit(
	IN	PRTMP_ADAPTER	pAd, 
	IN OUT PHEADER_802_11 pHdr80211, 
	IN UCHAR SubType, 
	IN UCHAR ToDs, 
	IN PUCHAR pDA, 
	IN PUCHAR pBssid) 
{
	NdisZeroMemory(pHdr80211, sizeof(HEADER_802_11));
	
	pHdr80211->FC.Type = BTYPE_MGMT;
	pHdr80211->FC.SubType = SubType;
//	if (SubType == SUBTYPE_ACK)	// sample, no use, it will conflict with ACTION frame sub type
//		pHdr80211->FC.Type = BTYPE_CNTL;
	pHdr80211->FC.ToDs = ToDs;
	COPY_MAC_ADDR(pHdr80211->Addr1, pDA);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		COPY_MAC_ADDR(pHdr80211->Addr2, pBssid);
#endif // CONFIG_AP_SUPPORT //
	COPY_MAC_ADDR(pHdr80211->Addr3, pBssid);
}

// ===========================================================================================
// mem_mgmt.c
// ===========================================================================================

/*!***************************************************************************
 * This routine build an outgoing frame, and fill all information specified 
 * in argument list to the frame body. The actual frame size is the summation 
 * of all arguments.
 * input params:
 *		Buffer - pointer to a pre-allocated memory segment
 *		args - a list of <int arg_size, arg> pairs.
 *		NOTE NOTE NOTE!!!! the last argument must be NULL, otherwise this
 *						   function will FAIL!!!
 * return:
 *		Size of the buffer
 * usage:  
 *		MakeOutgoingFrame(Buffer, output_length, 2, &fc, 2, &dur, 6, p_addr1, 6,p_addr2, END_OF_ARGS);

 IRQL = PASSIVE_LEVEL
 IRQL = DISPATCH_LEVEL
  
 ****************************************************************************/
ULONG MakeOutgoingFrame(
	OUT CHAR *Buffer, 
	OUT ULONG *FrameLen, ...) 
{
	CHAR   *p;
	int 	leng;
	ULONG	TotLeng;
	va_list Args;

	// calculates the total length
	TotLeng = 0;
	va_start(Args, FrameLen);
	do 
	{
		leng = va_arg(Args, int);
		if (leng == END_OF_ARGS) 
		{
			break;
		}
		p = va_arg(Args, PVOID);
		NdisMoveMemory(&Buffer[TotLeng], p, leng);
		TotLeng = TotLeng + leng;
	} while(TRUE);

	va_end(Args); /* clean up */
	*FrameLen = TotLeng;
	return TotLeng;
}

// ===========================================================================================
// mlme_queue.c
// ===========================================================================================

/*! \brief	Initialize The MLME Queue, used by MLME Functions
 *	\param	*Queue	   The MLME Queue
 *	\return Always	   Return NDIS_STATE_SUCCESS in this implementation
 *	\pre
 *	\post
 *	\note	Because this is done only once (at the init stage), no need to be locked

 IRQL = PASSIVE_LEVEL
 
 */
NDIS_STATUS MlmeQueueInit(
	IN MLME_QUEUE *Queue) 
{
	INT i;

	NdisAllocateSpinLock(&Queue->Lock);

	Queue->Num	= 0;
	Queue->Head = 0;
	Queue->Tail = 0;

	for (i = 0; i < MAX_LEN_OF_MLME_QUEUE; i++) 
	{
		Queue->Entry[i].Occupied = FALSE;
		Queue->Entry[i].MsgLen = 0;
		NdisZeroMemory(Queue->Entry[i].Msg, MGMT_DMA_BUFFER_SIZE);
	}

	return NDIS_STATUS_SUCCESS;
}

/*! \brief	 Enqueue a message for other threads, if they want to send messages to MLME thread
 *	\param	*Queue	  The MLME Queue
 *	\param	 Machine  The State Machine Id
 *	\param	 MsgType  The Message Type
 *	\param	 MsgLen   The Message length
 *	\param	*Msg	  The message pointer
 *	\return  TRUE if enqueue is successful, FALSE if the queue is full
 *	\pre
 *	\post
 *	\note	 The message has to be initialized

 IRQL = PASSIVE_LEVEL
 IRQL = DISPATCH_LEVEL
  
 */
BOOLEAN MlmeEnqueue(
	IN	PRTMP_ADAPTER	pAd,
	IN ULONG Machine, 
	IN ULONG MsgType, 
	IN ULONG MsgLen, 
	IN VOID *Msg) 
{
	INT Tail;
	MLME_QUEUE	*Queue = (MLME_QUEUE *)&pAd->Mlme.Queue;

	// Do nothing if the driver is starting halt state.
	// This might happen when timer already been fired before cancel timer with mlmehalt
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
		return FALSE;

	// First check the size, it MUST not exceed the mlme queue size
	if (MsgLen > MGMT_DMA_BUFFER_SIZE)
	{
		DBGPRINT_ERR(("MlmeEnqueue: msg too large, size = %ld \n", MsgLen));
		return FALSE;
	}
	
	if (MlmeQueueFull(Queue)) 
	{
		DBGPRINT_ERR(("MlmeEnqueue: full, msg dropped and may corrupt MLME\n"));
		return FALSE;
	}

	NdisAcquireSpinLock(&(Queue->Lock));
	Tail = Queue->Tail;
	Queue->Tail++;
	Queue->Num++;
	if (Queue->Tail == MAX_LEN_OF_MLME_QUEUE) 
	{
		Queue->Tail = 0;
	}
	
	Queue->Entry[Tail].Wcid = RESERVED_WCID;
	Queue->Entry[Tail].Occupied = TRUE;
	Queue->Entry[Tail].Machine = Machine;
	Queue->Entry[Tail].MsgType = MsgType;
	Queue->Entry[Tail].MsgLen  = MsgLen;	
	
	if (Msg != NULL)
	{
		NdisMoveMemory(Queue->Entry[Tail].Msg, Msg, MsgLen);
	}
		
	NdisReleaseSpinLock(&(Queue->Lock));
	DBGPRINT(RT_DEBUG_INFO, ("MlmeEnqueue, num=%ld\n",Queue->Num));
	return TRUE;
}

/*! \brief	 This function is used when Recv gets a MLME message
 *	\param	*Queue			 The MLME Queue
 *	\param	 TimeStampHigh	 The upper 32 bit of timestamp
 *	\param	 TimeStampLow	 The lower 32 bit of timestamp
 *	\param	 Rssi			 The receiving RSSI strength
 *	\param	 MsgLen 		 The length of the message
 *	\param	*Msg			 The message pointer
 *	\return  TRUE if everything ok, FALSE otherwise (like Queue Full)
 *	\pre
 *	\post
 
 IRQL = DISPATCH_LEVEL
 
 */
BOOLEAN MlmeEnqueueForRecv(
	IN	PRTMP_ADAPTER	pAd, 
	IN ULONG Wcid, 
	IN ULONG TimeStampHigh, 
	IN ULONG TimeStampLow,
	IN UCHAR Rssi0, 
	IN UCHAR Rssi1, 
	IN UCHAR Rssi2, 
	IN ULONG MsgLen, 
	IN VOID *Msg,
	IN UCHAR Signal) 
{
	INT 		 Tail, Machine;
	PFRAME_802_11 pFrame = (PFRAME_802_11)Msg;
	INT		 MsgType;
	MLME_QUEUE	*Queue = (MLME_QUEUE *)&pAd->Mlme.Queue;

#ifdef RALINK_ATE			
	/* Nothing to do in ATE mode */
	if(pAd->ate.Mode != ATE_STOP)
		return FALSE;
#endif // RALINK_ATE //

	// Do nothing if the driver is starting halt state.
	// This might happen when timer already been fired before cancel timer with mlmehalt
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
	{
		DBGPRINT_ERR(("MlmeEnqueueForRecv: fRTMP_ADAPTER_HALT_IN_PROGRESS\n"));
		return FALSE;
	}

	// First check the size, it MUST not exceed the mlme queue size
	if (MsgLen > MGMT_DMA_BUFFER_SIZE)
	{
		DBGPRINT_ERR(("MlmeEnqueueForRecv: frame too large, size = %ld \n", MsgLen));
		return FALSE;
	}

	if (MlmeQueueFull(Queue)) 
	{
		// Can't add debug print here. It'll make Mlme Stat machine freeze.
		DBGPRINT(RT_DEBUG_INFO, ("MlmeEnqueueForRecv: full and dropped\n"));
		return FALSE;
	}
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
#ifdef APCLI_SUPPORT
		// Beacon must be handled by ap-sync state machine.
		// Probe-rsp must be handled by apcli-sync state machine.
		// Those packets don't need to check its MAC address.
		do
		{
			if (preCheckMsgTypeSubset(pAd, pFrame, &Machine, &MsgType))
				break;

			if (!MAC_ADDR_EQUAL(pAd->MlmeAux.Bssid, ZERO_MAC_ADDR)
				&& MAC_ADDR_EQUAL(pAd->MlmeAux.Bssid, pFrame->Hdr.Addr2))
			{
				if (ApCliMsgTypeSubst(pAd, pFrame, &Machine, &MsgType))
					break;
			}
			else
			{
				if (APMsgTypeSubst(pAd, pFrame, &Machine, &MsgType))
					break;
			}

			DBGPRINT_ERR(("MlmeEnqueueForRecv: un-recongnized mgmt->subtype=%d, STA-%02x:%02x:%02x:%02x:%02x:%02x\n", 
						pFrame->Hdr.FC.SubType, pFrame->Hdr.Addr2[0], pFrame->Hdr.Addr2[1], pFrame->Hdr.Addr2[2],
						pFrame->Hdr.Addr2[3], pFrame->Hdr.Addr2[4], pFrame->Hdr.Addr2[5]));
			return FALSE;

		} while (FALSE);
#else
		if (!APMsgTypeSubst(pAd, pFrame, &Machine, &MsgType)) 
		{
			DBGPRINT_ERR(("MlmeEnqueueForRecv: un-recongnized mgmt->subtype=%d\n",pFrame->Hdr.FC.SubType));
			return FALSE;
		}
#endif // APCLI_SUPPORT //
	}
#endif // CONFIG_AP_SUPPORT //	

	// OK, we got all the informations, it is time to put things into queue
	NdisAcquireSpinLock(&(Queue->Lock));
	Tail = Queue->Tail;
	Queue->Tail++;
	Queue->Num++;
	if (Queue->Tail == MAX_LEN_OF_MLME_QUEUE) 
	{
		Queue->Tail = 0;
	}

	DBGPRINT(RT_DEBUG_INFO, ("MlmeEnqueueForRecv, num=%ld\n",Queue->Num));

	Queue->Entry[Tail].Occupied = TRUE;
	Queue->Entry[Tail].Machine = Machine;
	Queue->Entry[Tail].MsgType = MsgType;
	Queue->Entry[Tail].MsgLen  = MsgLen;
	Queue->Entry[Tail].TimeStamp.u.LowPart = TimeStampLow;
	Queue->Entry[Tail].TimeStamp.u.HighPart = TimeStampHigh;
	Queue->Entry[Tail].Rssi0 = Rssi0;
	Queue->Entry[Tail].Rssi1 = Rssi1;
	Queue->Entry[Tail].Rssi2 = Rssi2;
	Queue->Entry[Tail].Signal = Signal;
	Queue->Entry[Tail].Wcid = (UCHAR)Wcid;

	if (Msg != NULL)
	{
		NdisMoveMemory(Queue->Entry[Tail].Msg, Msg, MsgLen);
	}

	NdisReleaseSpinLock(&(Queue->Lock));

	RT28XX_MLME_HANDLER(pAd);

	return TRUE;
}

#ifdef WSC_INCLUDED
/*! \brief   Enqueue a message for other threads, if they want to send messages to MLME thread
 *  \param  *Queue    The MLME Queue
 *  \param   TimeStampLow    The lower 32 bit of timestamp, here we used for eventID.
 *  \param   Machine  The State Machine Id
 *  \param   MsgType  The Message Type
 *  \param   MsgLen   The Message length
 *  \param  *Msg      The message pointer
 *  \return  TRUE if enqueue is successful, FALSE if the queue is full
 *  \pre
 *  \post
 *  \note    The message has to be initialized
 */
BOOLEAN MlmeEnqueueForWsc(
	IN	PRTMP_ADAPTER	pAd,
	IN ULONG eventID,
	IN LONG senderID,
	IN ULONG Machine, 
	IN ULONG MsgType, 
	IN ULONG MsgLen, 
	IN VOID *Msg) 
{
    INT Tail;
    //ULONG			IrqFlags;
	MLME_QUEUE	*Queue = (MLME_QUEUE *)&pAd->Mlme.Queue;

	DBGPRINT(RT_DEBUG_TRACE, ("-----> MlmeEnqueueForWsc\n"));
    // Do nothing if the driver is starting halt state.
    // This might happen when timer already been fired before cancel timer with mlmehalt
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_HALT_IN_PROGRESS))
        return FALSE;

	// First check the size, it MUST not exceed the mlme queue size
	if (MsgLen > MGMT_DMA_BUFFER_SIZE)
	{
        DBGPRINT_ERR(("MlmeEnqueueForWsc: msg too large, size = %ld \n", MsgLen));
		return FALSE;
	}
	
    if (MlmeQueueFull(Queue)) 
    {
        DBGPRINT_ERR(("MlmeEnqueueForWsc: full, msg dropped and may corrupt MLME\n"));
        return FALSE;
    }

    // OK, we got all the informations, it is time to put things into queue
	NdisAcquireSpinLock(&(Queue->Lock));
    Tail = Queue->Tail;
    Queue->Tail++;
    Queue->Num++;
    if (Queue->Tail == MAX_LEN_OF_MLME_QUEUE) 
    {
        Queue->Tail = 0;
    }
    DBGPRINT(RT_DEBUG_INFO, ("MlmeEnqueueForWsc, num=%ld\n",Queue->Num));
 
    Queue->Entry[Tail].Occupied = TRUE;
    Queue->Entry[Tail].Machine = Machine;
    Queue->Entry[Tail].MsgType = MsgType;
    Queue->Entry[Tail].MsgLen  = MsgLen;
	Queue->Entry[Tail].TimeStamp.u.LowPart = eventID;
	Queue->Entry[Tail].TimeStamp.u.HighPart = senderID;
    if (Msg != NULL)
        NdisMoveMemory(Queue->Entry[Tail].Msg, Msg, MsgLen);

    NdisReleaseSpinLock(&(Queue->Lock));

	DBGPRINT(RT_DEBUG_TRACE, ("<----- MlmeEnqueueForWsc\n"));
	
    return TRUE;
}
#endif // WSC_INCLUDED //

/*! \brief	 Dequeue a message from the MLME Queue
 *	\param	*Queue	  The MLME Queue
 *	\param	*Elem	  The message dequeued from MLME Queue
 *	\return  TRUE if the Elem contains something, FALSE otherwise
 *	\pre
 *	\post

 IRQL = DISPATCH_LEVEL

 */
BOOLEAN MlmeDequeue(
	IN MLME_QUEUE *Queue, 
	OUT MLME_QUEUE_ELEM **Elem) 
{
	NdisAcquireSpinLock(&(Queue->Lock));
	*Elem = &(Queue->Entry[Queue->Head]);    
	Queue->Num--;
	Queue->Head++;
	if (Queue->Head == MAX_LEN_OF_MLME_QUEUE) 
	{
		Queue->Head = 0;
	}
	NdisReleaseSpinLock(&(Queue->Lock));
	DBGPRINT(RT_DEBUG_INFO, ("MlmeDequeue, num=%ld\n",Queue->Num));

	return TRUE;
}

// IRQL = DISPATCH_LEVEL
VOID	MlmeRestartStateMachine(
	IN	PRTMP_ADAPTER	pAd)
{
#ifdef RT2860
	MLME_QUEUE_ELEM		*Elem = NULL;
#endif // RT2860 //
	
	DBGPRINT(RT_DEBUG_TRACE, ("MlmeRestartStateMachine \n"));

#ifdef RT2860
	NdisAcquireSpinLock(&pAd->Mlme.TaskLock);
	if(pAd->Mlme.bRunning) 
	{
		NdisReleaseSpinLock(&pAd->Mlme.TaskLock);
		return;
	} 
	else 
	{
		pAd->Mlme.bRunning = TRUE;
	}
	NdisReleaseSpinLock(&pAd->Mlme.TaskLock);

	// Remove all Mlme queues elements
	while (!MlmeQueueEmpty(&pAd->Mlme.Queue)) 
	{
		//From message type, determine which state machine I should drive
		if (MlmeDequeue(&pAd->Mlme.Queue, &Elem)) 
		{
			// free MLME element
			Elem->Occupied = FALSE;
			Elem->MsgLen = 0;

		}
		else {
			DBGPRINT_ERR(("MlmeRestartStateMachine: MlmeQueue empty\n"));
		}
	}
#endif // RT2860 //


	// Change back to original channel in case of doing scan
	AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
	AsicLockChannel(pAd, pAd->CommonCfg.Channel);

	// Resume MSDU which is turned off durning scan
	RTMPResumeMsduTransmission(pAd);

	
#ifdef RT2860	
	// Remove running state
	NdisAcquireSpinLock(&pAd->Mlme.TaskLock);
	pAd->Mlme.bRunning = FALSE;
	NdisReleaseSpinLock(&pAd->Mlme.TaskLock);
#endif // RT2860 //
}

/*! \brief	test if the MLME Queue is empty
 *	\param	*Queue	  The MLME Queue
 *	\return TRUE if the Queue is empty, FALSE otherwise
 *	\pre
 *	\post
 
 IRQL = DISPATCH_LEVEL
 
 */
BOOLEAN MlmeQueueEmpty(
	IN MLME_QUEUE *Queue) 
{
	BOOLEAN Ans;

	NdisAcquireSpinLock(&(Queue->Lock));
	Ans = (Queue->Num == 0);
	NdisReleaseSpinLock(&(Queue->Lock));

	return Ans;
}

/*! \brief	 test if the MLME Queue is full
 *	\param	 *Queue 	 The MLME Queue
 *	\return  TRUE if the Queue is empty, FALSE otherwise
 *	\pre
 *	\post

 IRQL = PASSIVE_LEVEL
 IRQL = DISPATCH_LEVEL

 */
BOOLEAN MlmeQueueFull(
	IN MLME_QUEUE *Queue) 
{
	BOOLEAN Ans;

	NdisAcquireSpinLock(&(Queue->Lock));
	Ans = (Queue->Num == MAX_LEN_OF_MLME_QUEUE || Queue->Entry[Queue->Tail].Occupied);
	NdisReleaseSpinLock(&(Queue->Lock));

	return Ans;
}

/*! \brief	 The destructor of MLME Queue
 *	\param 
 *	\return
 *	\pre
 *	\post
 *	\note	Clear Mlme Queue, Set Queue->Num to Zero.

 IRQL = PASSIVE_LEVEL
 
 */
VOID MlmeQueueDestroy(
	IN MLME_QUEUE *pQueue) 
{
	NdisAcquireSpinLock(&(pQueue->Lock));
	pQueue->Num  = 0;
	pQueue->Head = 0;
	pQueue->Tail = 0;
	NdisReleaseSpinLock(&(pQueue->Lock));
	NdisFreeSpinLock(&(pQueue->Lock));
}

/*! \brief	 To substitute the message type if the message is coming from external
 *	\param	pFrame		   The frame received
 *	\param	*Machine	   The state machine
 *	\param	*MsgType	   the message type for the state machine
 *	\return TRUE if the substitution is successful, FALSE otherwise
 *	\pre
 *	\post

 IRQL = DISPATCH_LEVEL

 */

// ===========================================================================================
// state_machine.c
// ===========================================================================================

/*! \brief Initialize the state machine.
 *	\param *S			pointer to the state machine 
 *	\param	Trans		State machine transition function
 *	\param	StNr		number of states 
 *	\param	MsgNr		number of messages 
 *	\param	DefFunc 	default function, when there is invalid state/message combination 
 *	\param	InitState	initial state of the state machine 
 *	\param	Base		StateMachine base, internal use only
 *	\pre p_sm should be a legal pointer
 *	\post

 IRQL = PASSIVE_LEVEL
 
 */
VOID StateMachineInit(
	IN STATE_MACHINE *S, 
	IN STATE_MACHINE_FUNC Trans[], 
	IN ULONG StNr, 
	IN ULONG MsgNr, 
	IN STATE_MACHINE_FUNC DefFunc, 
	IN ULONG InitState, 
	IN ULONG Base) 
{
	ULONG i, j;

	// set number of states and messages
	S->NrState = StNr;
	S->NrMsg   = MsgNr;
	S->Base    = Base;

	S->TransFunc  = Trans;

	// init all state transition to default function
	for (i = 0; i < StNr; i++) 
	{
		for (j = 0; j < MsgNr; j++) 
		{
			S->TransFunc[i * MsgNr + j] = DefFunc;
		}
	}

	// set the starting state
	S->CurrState = InitState;
}

/*! \brief This function fills in the function pointer into the cell in the state machine 
 *	\param *S	pointer to the state machine
 *	\param St	state
 *	\param Msg	incoming message
 *	\param f	the function to be executed when (state, message) combination occurs at the state machine
 *	\pre *S should be a legal pointer to the state machine, st, msg, should be all within the range, Base should be set in the initial state
 *	\post

 IRQL = PASSIVE_LEVEL
 
 */
VOID StateMachineSetAction(
	IN STATE_MACHINE *S, 
	IN ULONG St, 
	IN ULONG Msg, 
	IN STATE_MACHINE_FUNC Func) 
{
	ULONG MsgIdx;

	MsgIdx = Msg - S->Base;

	if (St < S->NrState && MsgIdx < S->NrMsg) 
	{
		// boundary checking before setting the action
		S->TransFunc[St * S->NrMsg + MsgIdx] = Func;
	} 
}

/*! \brief	 This function does the state transition
 *	\param	 *Adapter the NIC adapter pointer
 *	\param	 *S 	  the state machine
 *	\param	 *Elem	  the message to be executed
 *	\return   None
 
 IRQL = DISPATCH_LEVEL
 
 */
VOID StateMachinePerformAction(
	IN	PRTMP_ADAPTER	pAd, 
	IN STATE_MACHINE *S, 
	IN MLME_QUEUE_ELEM *Elem) 
{
	(*(S->TransFunc[S->CurrState * S->NrMsg + Elem->MsgType - S->Base]))(pAd, Elem);
}

/*
	==========================================================================
	Description:
		The drop function, when machine executes this, the message is simply 
		ignored. This function does nothing, the message is freed in 
		StateMachinePerformAction()
	==========================================================================
 */
VOID Drop(
	IN PRTMP_ADAPTER pAd, 
	IN MLME_QUEUE_ELEM *Elem) 
{
}

// ===========================================================================================
// lfsr.c
// ===========================================================================================

/*
	==========================================================================
	Description:

	IRQL = PASSIVE_LEVEL

	==========================================================================
 */
VOID LfsrInit(
	IN PRTMP_ADAPTER pAd, 
	IN ULONG Seed) 
{
	if (Seed == 0) 
		pAd->Mlme.ShiftReg = 1;
	else 
		pAd->Mlme.ShiftReg = Seed;
}

/*
	==========================================================================
	Description:
	==========================================================================
 */
UCHAR RandomByte(
	IN PRTMP_ADAPTER pAd) 
{
	ULONG i;
	UCHAR R, Result;

	R = 0;

	if (pAd->Mlme.ShiftReg == 0)
	NdisGetSystemUpTime((ULONG *)&pAd->Mlme.ShiftReg);

	for (i = 0; i < 8; i++) 
	{
		if (pAd->Mlme.ShiftReg & 0x00000001) 
		{
			pAd->Mlme.ShiftReg = ((pAd->Mlme.ShiftReg ^ LFSR_MASK) >> 1) | 0x80000000;
			Result = 1;
		} 
		else 
		{
			pAd->Mlme.ShiftReg = pAd->Mlme.ShiftReg >> 1;
			Result = 0;
		}
		R = (R << 1) | Result;
	}

	return R;
}

VOID AsicUpdateAutoFallBackTable(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			pRateTable)
{
	UCHAR					i;
	HT_FBK_CFG0_STRUC		HtCfg0;
	HT_FBK_CFG1_STRUC		HtCfg1;
	LG_FBK_CFG0_STRUC		LgCfg0;
	LG_FBK_CFG1_STRUC		LgCfg1;
	PRTMP_TX_RATE_SWITCH	pCurrTxRate, pNextTxRate;

	// set to initial value
	HtCfg0.word = 0x65432100;
	HtCfg1.word = 0xedcba988;
	LgCfg0.word = 0xedcba988;
	LgCfg1.word = 0x00002100;

	pNextTxRate = (PRTMP_TX_RATE_SWITCH)pRateTable+1;
	for (i = 1; i < *((PUCHAR) pRateTable); i++)
	{
		pCurrTxRate = (PRTMP_TX_RATE_SWITCH)pRateTable+1+i;
		switch (pCurrTxRate->Mode)
		{
			case 0:		//CCK
				break;
			case 1:		//OFDM
				{
					switch(pCurrTxRate->CurrMCS)
					{
						case 0:
							LgCfg0.field.OFDMMCS0FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
						case 1:
							LgCfg0.field.OFDMMCS1FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
						case 2:
							LgCfg0.field.OFDMMCS2FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
						case 3:
							LgCfg0.field.OFDMMCS3FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
						case 4:
							LgCfg0.field.OFDMMCS4FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
						case 5:
							LgCfg0.field.OFDMMCS5FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
						case 6:
							LgCfg0.field.OFDMMCS6FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
						case 7:
							LgCfg0.field.OFDMMCS7FBK = (pNextTxRate->Mode == MODE_OFDM) ? (pNextTxRate->CurrMCS+8): pNextTxRate->CurrMCS;
							break;
					}
				}
				break;
			case 2:		//HT-MIX
			case 3:		//HT-GF
				{
					if ((pNextTxRate->Mode >= MODE_HTMIX) && (pCurrTxRate->CurrMCS != pNextTxRate->CurrMCS))
					{
						switch(pCurrTxRate->CurrMCS)
						{
							case 0:
								HtCfg0.field.HTMCS0FBK = pNextTxRate->CurrMCS;
								break;
							case 1:
								HtCfg0.field.HTMCS1FBK = pNextTxRate->CurrMCS;
								break;
							case 2:
								HtCfg0.field.HTMCS2FBK = pNextTxRate->CurrMCS;
								break;
							case 3:
								HtCfg0.field.HTMCS3FBK = pNextTxRate->CurrMCS;
								break;
							case 4:
								HtCfg0.field.HTMCS4FBK = pNextTxRate->CurrMCS;
								break;
							case 5:
								HtCfg0.field.HTMCS5FBK = pNextTxRate->CurrMCS;
								break;
							case 6:
								HtCfg0.field.HTMCS6FBK = pNextTxRate->CurrMCS;
								break;
							case 7:
								HtCfg0.field.HTMCS7FBK = pNextTxRate->CurrMCS;
								break;
							case 8:
								HtCfg1.field.HTMCS8FBK = pNextTxRate->CurrMCS;
								break;
							case 9:
								HtCfg1.field.HTMCS9FBK = pNextTxRate->CurrMCS;
								break;
							case 10:
								HtCfg1.field.HTMCS10FBK = pNextTxRate->CurrMCS;
								break;
							case 11:
								HtCfg1.field.HTMCS11FBK = pNextTxRate->CurrMCS;
								break;
							case 12:
								HtCfg1.field.HTMCS12FBK = pNextTxRate->CurrMCS;
								break;
							case 13:
								HtCfg1.field.HTMCS13FBK = pNextTxRate->CurrMCS;
								break;
							case 14:
								HtCfg1.field.HTMCS14FBK = pNextTxRate->CurrMCS;
								break;
							case 15:
								HtCfg1.field.HTMCS15FBK = pNextTxRate->CurrMCS;
								break;
							default:
								DBGPRINT(RT_DEBUG_ERROR, ("AsicUpdateAutoFallBackTable: not support CurrMCS=%d\n", pCurrTxRate->CurrMCS));
						}
					}
				}
				break;
		}

		pNextTxRate = pCurrTxRate;
	}

	RTMP_IO_WRITE32(pAd, HT_FBK_CFG0, HtCfg0.word);
	RTMP_IO_WRITE32(pAd, HT_FBK_CFG1, HtCfg1.word);
	RTMP_IO_WRITE32(pAd, LG_FBK_CFG0, LgCfg0.word);
	RTMP_IO_WRITE32(pAd, LG_FBK_CFG1, LgCfg1.word);

	DBGPRINT(RT_DEBUG_INFO, ("AsicUpdateAutoFallBackTable: HtCfg0=0x%x, HtCfg1=0x%x, LgCfg0=0x%x, LgCfg1=0x%x \n", HtCfg0.word, HtCfg1.word, LgCfg0.word, LgCfg1.word));
}

/*
	========================================================================

	Routine Description:
		Set MAC register value according operation mode.
		OperationMode AND bNonGFExist are for MM and GF Proteciton.
		If MM or GF mask is not set, those passing argument doesn't not take effect.
		
		Operation mode meaning:
		= 0 : Pure HT, no preotection.
		= 0x01; there may be non-HT devices in both the control and extension channel, protection is optional in BSS.
		= 0x10: No Transmission in 40M is protected.
		= 0x11: Transmission in both 40M and 20M shall be protected
		if (bNonGFExist)
			we should choose not to use GF. But still set correct ASIC registers.
	========================================================================
*/
VOID 	AsicUpdateProtect(
	IN		PRTMP_ADAPTER	pAd,
	IN 		USHORT			OperationMode,
	IN 		UCHAR			SetMask,
	IN		BOOLEAN			bDisableBGProtect,
	IN		BOOLEAN			bNonGFExist)	
{
	PROT_CFG_STRUC	ProtCfg, ProtCfg4;
	UINT32 Protect[6];
	USHORT			offset;
	UCHAR			i;
	UINT32 MacReg = 0;

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

	if (!(pAd->CommonCfg.bHTProtect) && (OperationMode != 8))
	{
		return;
	}

	if (pAd->BATable.numAsOriginator)
	{
		// 
		// enable the RTS/CTS to avoid channel collision
		// 
		SetMask = ALLN_SETPROTECT;
		OperationMode = 8;
	}


	// Config ASIC RTS threshold register
	RTMP_IO_READ32(pAd, TX_RTS_CFG, &MacReg);
	MacReg &= 0xFF0000FF;
#if 0
	MacReg |= (pAd->CommonCfg.RtsThreshold << 8);
#else
	// If the user want disable RtsThreshold and enbale Amsdu/Ralink-Aggregation, set the RtsThreshold as 4096
        if (((pAd->CommonCfg.BACapability.field.AmsduEnable) || (pAd->CommonCfg.bAggregationCapable == TRUE))
            && pAd->CommonCfg.RtsThreshold == MAX_RTS_THRESHOLD)
        {
			MacReg |= (0x1000 << 8);
        }
        else
        {
			MacReg |= (pAd->CommonCfg.RtsThreshold << 8);
        }
#endif

	RTMP_IO_WRITE32(pAd, TX_RTS_CFG, MacReg);

	// Initial common protection settings
	RTMPZeroMemory(Protect, sizeof(Protect));
	ProtCfg4.word = 0;
	ProtCfg.word = 0;
	ProtCfg.field.TxopAllowGF40 = 1;
	ProtCfg.field.TxopAllowGF20 = 1;
	ProtCfg.field.TxopAllowMM40 = 1;
	ProtCfg.field.TxopAllowMM20 = 1;
	ProtCfg.field.TxopAllowOfdm = 1;
	ProtCfg.field.TxopAllowCck = 1;
	ProtCfg.field.RTSThEn = 1;
	ProtCfg.field.ProtectNav = ASIC_SHORTNAV;

	// update PHY mode and rate
	if (pAd->CommonCfg.Channel > 14)
		ProtCfg.field.ProtectRate = 0x4000;
	ProtCfg.field.ProtectRate |= pAd->CommonCfg.RtsRate;	

	// Handle legacy(B/G) protection
	if (bDisableBGProtect)
	{
		//ProtCfg.field.ProtectRate = pAd->CommonCfg.RtsRate;
		ProtCfg.field.ProtectCtrl = 0;
		Protect[0] = ProtCfg.word;
		Protect[1] = ProtCfg.word;
	}
	else
	{
		//ProtCfg.field.ProtectRate = pAd->CommonCfg.RtsRate;
		ProtCfg.field.ProtectCtrl = 0;			// CCK do not need to be protected
		Protect[0] = ProtCfg.word;
		ProtCfg.field.ProtectCtrl = ASIC_CTS;	// OFDM needs using CCK to protect
		Protect[1] = ProtCfg.word;
	}

	// Decide HT frame protection.
	if ((SetMask & ALLN_SETPROTECT) != 0)
	{
		//pAd->CommonCfg.BACapability.field.RxBAWinLimit = pAd->CommonCfg.REGBACapability.field.RxBAWinLimit;
		DBGPRINT(RT_DEBUG_INFO, ("AsicUpdateProtect===>OperationMode = %d. \n", OperationMode));
		switch(OperationMode)
		{
			case 0x0:
				// NO PROTECT 
				// 1.All STAs in the BSS are 20/40 MHz HT
				// 2. in ai 20/40MHz BSS
				// 3. all STAs are 20MHz in a 20MHz BSS
				// Pure HT. no protection.

				// MM20_PROT_CFG
				//	Reserved (31:27)
				// 	PROT_TXOP(25:20) -- 010111
				//	PROT_NAV(19:18)  -- 01 (Short NAV protection)
				//  PROT_CTRL(17:16) -- 00 (None)
				// 	PROT_RATE(15:0)  -- 0x4004 (OFDM 24M)
				Protect[2] = 0x01744004;	

				// MM40_PROT_CFG
				//	Reserved (31:27)
				// 	PROT_TXOP(25:20) -- 111111
				//	PROT_NAV(19:18)  -- 01 (Short NAV protection)
				//  PROT_CTRL(17:16) -- 00 (None) 
				// 	PROT_RATE(15:0)  -- 0x4084 (duplicate OFDM 24M)
				Protect[3] = 0x03f44084;

				// CF20_PROT_CFG
				//	Reserved (31:27)
				// 	PROT_TXOP(25:20) -- 010111
				//	PROT_NAV(19:18)  -- 01 (Short NAV protection)
				//  PROT_CTRL(17:16) -- 00 (None)
				// 	PROT_RATE(15:0)  -- 0x4004 (OFDM 24M)
				Protect[4] = 0x01744004;

				// CF40_PROT_CFG
				//	Reserved (31:27)
				// 	PROT_TXOP(25:20) -- 111111
				//	PROT_NAV(19:18)  -- 01 (Short NAV protection)
				//  PROT_CTRL(17:16) -- 00 (None)
				// 	PROT_RATE(15:0)  -- 0x4084 (duplicate OFDM 24M)
				Protect[5] = 0x03f44084;

				if (bNonGFExist)
				{
					// PROT_NAV(19:18)  -- 01 (Short NAV protectiion)
					// PROT_CTRL(17:16) -- 01 (RTS/CTS)
					Protect[4] = 0x01754004;
					Protect[5] = 0x03f54084;
				}
				pAd->CommonCfg.IOTestParm.bRTSLongProtOn = FALSE;
				break;
				
 			case 1:
				// This is "HT non-member protection mode."
				// If there may be non-HT STAs my BSS
				ProtCfg.word = 0x01744004;	// PROT_CTRL(17:16) : 0 (None)
				ProtCfg4.word = 0x03f44084; // duplicaet legacy 24M. BW set 1.
				if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED))
				{
					ProtCfg.word = 0x01740003;	//ERP use Protection bit is set, use protection rate at Clause 18..
					ProtCfg4.word = 0x03f40083; 
				}
				//Assign Protection method for 20&40 MHz packets
				ProtCfg.field.ProtectCtrl = ASIC_RTS;
				ProtCfg.field.ProtectNav = ASIC_SHORTNAV;
				ProtCfg4.field.ProtectCtrl = ASIC_RTS;
				ProtCfg4.field.ProtectNav = ASIC_SHORTNAV;
				Protect[2] = ProtCfg.word;
				Protect[3] = ProtCfg4.word;
				Protect[4] = ProtCfg.word;
				Protect[5] = ProtCfg4.word;
				pAd->CommonCfg.IOTestParm.bRTSLongProtOn = TRUE;
				break;
				
			case 2:
				// If only HT STAs are in BSS. at least one is 20MHz. Only protect 40MHz packets
				ProtCfg.word = 0x01744004;  // PROT_CTRL(17:16) : 0 (None)
				ProtCfg4.word = 0x03f44084; // duplicaet legacy 24M. BW set 1.

				//Assign Protection method for 40MHz packets
				ProtCfg4.field.ProtectCtrl = ASIC_RTS;
				ProtCfg4.field.ProtectNav = ASIC_SHORTNAV;
				Protect[2] = ProtCfg.word;
				Protect[3] = ProtCfg4.word;
				if (bNonGFExist)
				{
					ProtCfg.field.ProtectCtrl = ASIC_RTS;
					ProtCfg.field.ProtectNav = ASIC_SHORTNAV;
				}
				Protect[4] = ProtCfg.word;
				Protect[5] = ProtCfg4.word;

				pAd->CommonCfg.IOTestParm.bRTSLongProtOn = FALSE;
				break;
				
			case 3:
				// HT mixed mode.	 PROTECT ALL!
				// Assign Rate
				ProtCfg.word = 0x01744004;	//duplicaet legacy 24M. BW set 1.
				ProtCfg4.word = 0x03f44084;
				// both 20MHz and 40MHz are protected. Whether use RTS or CTS-to-self depends on the
				if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED))
				{
					ProtCfg.word = 0x01740003;	//ERP use Protection bit is set, use protection rate at Clause 18..
					ProtCfg4.word = 0x03f40083;
				}
				//Assign Protection method for 20&40 MHz packets
				ProtCfg.field.ProtectCtrl = ASIC_RTS;
				ProtCfg.field.ProtectNav = ASIC_SHORTNAV;
				ProtCfg4.field.ProtectCtrl = ASIC_RTS;
				ProtCfg4.field.ProtectNav = ASIC_SHORTNAV;
				Protect[2] = ProtCfg.word;
				Protect[3] = ProtCfg4.word;
				Protect[4] = ProtCfg.word;
				Protect[5] = ProtCfg4.word;
				pAd->CommonCfg.IOTestParm.bRTSLongProtOn = TRUE;
				break;	
				
			case 8:
				// Special on for Atheros problem n chip.
				Protect[2] = 0x01754004;
				Protect[3] = 0x03f54084;
				Protect[4] = 0x01754004;
				Protect[5] = 0x03f54084;
				pAd->CommonCfg.IOTestParm.bRTSLongProtOn = TRUE;
				break;		
		}
	}
	
	offset = CCK_PROT_CFG;
	for (i = 0;i < 6;i++)
	{
			if ((SetMask & (1<< i)))
		{
		RTMP_IO_WRITE32(pAd, offset + i*4, Protect[i]);
	}
}
}

/*
	==========================================================================
	Description:

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicSwitchChannel(
					  IN PRTMP_ADAPTER pAd, 
	IN	UCHAR			Channel,
	IN	BOOLEAN			bScan) 
{
	ULONG			R2 = 0, R3 = DEFAULT_RF_TX_POWER, R4 = 0;
	CHAR    TxPwer = 0, TxPwer2 = DEFAULT_RF_TX_POWER; //Bbp94 = BBPR94_DEFAULT, TxPwer2 = DEFAULT_RF_TX_POWER;
	UCHAR	index;
	UINT32 	Value = 0; //BbpReg, Value;
	RTMP_RF_REGS *RFRegTable;

	// Search Tx power value
	for (index = 0; index < pAd->ChannelListNum; index++)
	{
		if (Channel == pAd->ChannelList[index].Channel)
		{
			TxPwer = pAd->ChannelList[index].Power;
			TxPwer2 = pAd->ChannelList[index].Power2;
			break;
		}
	}

	RFRegTable = RF2850RegTable;

	switch (pAd->RfIcType)
	{
		case RFIC_2820:
		case RFIC_2850:
		case RFIC_2720:
		case RFIC_2750:

			for (index = 0; index < NUM_OF_2850_CHNL; index++)
			{
				if (Channel == RFRegTable[index].Channel)
				{
					R2 = RFRegTable[index].R2;
					if (pAd->Antenna.field.TxPath == 1)
					{
						R2 |= 0x4000;	// If TXpath is 1, bit 14 = 1;
					}

					if (pAd->Antenna.field.RxPath == 2)
					{
						R2 |= 0x40;	// write 1 to off Rxpath.
					}
					else if (pAd->Antenna.field.RxPath == 1)
					{
						R2 |= 0x20040;	// write 1 to off RxPath
					}

					if (Channel > 14)
					{
#if 0					
						// When 5G band the LSB of TxPwr must always be one and shit one bit
						TxPwer = (TxPwer > 0xF) ? 0xF : TxPwer;
						TxPwer2 = (TxPwer2 > 0xF) ? 0xF : TxPwer2;
						R3 = (RFRegTable[index].R3 & 0xffffc1ff) | (TxPwer << 10) | (1 << 9); // set TX power0
						R4 = (RFRegTable[index].R4 & (~0x001f87c0)) | (pAd->RfFreqOffset << 15) | (TxPwer2 << 7) | (1 << 6);// Set freq Offset & TxPwr1
#endif
						// initialize R3, R4
						R3 = (RFRegTable[index].R3 & 0xffffc1ff);
						R4 = (RFRegTable[index].R4 & (~0x001f87c0)) | (pAd->RfFreqOffset << 15);

						// 5G band power range: 0xF9~0X0F, TX0 Reg3 bit9/TX1 Reg4 bit6="0" means the TX power reduce 7dB
						// R3
						if ((TxPwer >= -7) && (TxPwer < 0))
						{
							TxPwer = (7+TxPwer);
							TxPwer = (TxPwer > 0xF) ? (0xF) : (TxPwer);
							R3 |= (TxPwer << 10);
							DBGPRINT(RT_DEBUG_ERROR, ("AsicSwitchChannel: TxPwer=%d \n", TxPwer));
						}
						else
						{
							TxPwer = (TxPwer > 0xF) ? (0xF) : (TxPwer);
							R3 |= (TxPwer << 10) | (1 << 9);
						}

						// R4
						if ((TxPwer2 >= -7) && (TxPwer2 < 0))
						{
							TxPwer2 = (7+TxPwer2);
							TxPwer2 = (TxPwer2 > 0xF) ? (0xF) : (TxPwer2);
							R4 |= (TxPwer2 << 7);
							DBGPRINT(RT_DEBUG_ERROR, ("AsicSwitchChannel: TxPwer2=%d \n", TxPwer2));
						}
						else
						{
							TxPwer2 = (TxPwer2 > 0xF) ? (0xF) : (TxPwer2);
							R4 |= (TxPwer2 << 7) | (1 << 6);
						}                        
					}
					else
					{
						R3 = (RFRegTable[index].R3 & 0xffffc1ff) | (TxPwer << 9); // set TX power0
					R4 = (RFRegTable[index].R4 & (~0x001f87c0)) | (pAd->RfFreqOffset << 15) | (TxPwer2 <<6);// Set freq Offset & TxPwr1
					}

					// Based on BBP current mode before changing RF channel.
					if (!bScan && (pAd->CommonCfg.BBPCurrentBW == BW_40))
					{
						R4 |=0x200000;
					}

					// Update variables
					pAd->LatchRfRegs.Channel = Channel;
					pAd->LatchRfRegs.R1 = RFRegTable[index].R1;
					pAd->LatchRfRegs.R2 = R2;
					pAd->LatchRfRegs.R3 = R3;
					pAd->LatchRfRegs.R4 = R4;

					// Set RF value 1's set R3[bit2] = [0]
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
					RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

					RTMPusecDelay(200);

					// Set RF value 2's set R3[bit2] = [1]
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
					RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 | 0x04));
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

					RTMPusecDelay(200);

					// Set RF value 3's set R3[bit2] = [0]
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R1);
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R2);
					RTMP_RF_IO_WRITE32(pAd, (pAd->LatchRfRegs.R3 & (~0x04)));
					RTMP_RF_IO_WRITE32(pAd, pAd->LatchRfRegs.R4);

					break;
				}
			}
			break;

		default:
			break;
	}

	// Change BBP setting during siwtch from a->g, g->a
	if (Channel <= 14)
	{
	    ULONG	TxPinCfg = 0x00050F0A;//Gary 2007/08/09 0x050A0A
		
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, (0x37 - GET_LNA_GAIN(pAd)));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R63, (0x37 - GET_LNA_GAIN(pAd)));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, (0x37 - GET_LNA_GAIN(pAd)));
		//RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R86, (0x44 - GET_LNA_GAIN(pAd)));	// According the Rory's suggestion to solve the middle range issue.
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R82, 0x62);

		// 5G band selection PIN, bit1 and bit2 are complement
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value &= (~0x6);
		Value |= (0x04);
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

        // Turn off unused PA or LNA when only 1T or 1R
		if (pAd->Antenna.field.TxPath == 1)
		{
			TxPinCfg &= 0xFFFFFFF3;
		}
		if (pAd->Antenna.field.RxPath == 1)
		{
			TxPinCfg &= 0xFFFFF3FF;
		}

		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, TxPinCfg);
	}
	else
	{
	    ULONG	TxPinCfg = 0x00050F05;//Gary 2007/8/9 0x050505
		
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R62, (0x37 - GET_LNA_GAIN(pAd)));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R63, (0x37 - GET_LNA_GAIN(pAd)));
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R64, (0x37 - GET_LNA_GAIN(pAd)));
		//RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R86, (0x44 - GET_LNA_GAIN(pAd)));   // According the Rory's suggestion to solve the middle range issue.     
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R82, 0xF2);

		// 5G band selection PIN, bit1 and bit2 are complement
		RTMP_IO_READ32(pAd, TX_BAND_CFG, &Value);
		Value &= (~0x6);
		Value |= (0x02);
		RTMP_IO_WRITE32(pAd, TX_BAND_CFG, Value);

        // Turn off unused PA or LNA when only 1T or 1R
		if (pAd->Antenna.field.TxPath == 1)
		{
			TxPinCfg &= 0xFFFFFFF3;
	}
		if (pAd->Antenna.field.RxPath == 1)
		{
			TxPinCfg &= 0xFFFFF3FF;
	}

		RTMP_IO_WRITE32(pAd, TX_PIN_CFG, TxPinCfg);
	}

    // R66 should be set according to Channel and use 20MHz when scanning
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, (0x2E + GET_LNA_GAIN(pAd)));

	//
	// On 11A, We should delay and wait RF/BBP to be stable
	// and the appropriate time should be 1000 micro seconds 
	// 2005/06/05 - On 11G, We also need this delay time. Otherwise it's difficult to pass the WHQL.
	//
	RTMPusecDelay(1000);  

	DBGPRINT(RT_DEBUG_TRACE, ("SwitchChannel#%d(RF=%d, Pwr0=%lu, Pwr1=%lu, %dT) to , R1=0x%08lx, R2=0x%08lx, R3=0x%08lx, R4=0x%08lx\n",
							  Channel, 
							  pAd->RfIcType, 
							  (R3 & 0x00003e00) >> 9,
							  (R4 & 0x000007c0) >> 6,
							  pAd->Antenna.field.TxPath,
							  pAd->LatchRfRegs.R1, 
							  pAd->LatchRfRegs.R2, 
							  pAd->LatchRfRegs.R3, 
							  pAd->LatchRfRegs.R4));
}

/*
	==========================================================================
	Description:
		This function is required for 2421 only, and should not be used during
		site survey. It's only required after NIC decided to stay at a channel
		for a longer period.
		When this function is called, it's always after AsicSwitchChannel().

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicLockChannel(
	IN PRTMP_ADAPTER pAd, 
	IN UCHAR Channel) 
{
}

/*
	==========================================================================
	Description:

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID	AsicAntennaSelect(
	IN	PRTMP_ADAPTER	pAd,
	IN	UCHAR			Channel) 
{
}

/*
	========================================================================
	
	Routine Description:
		Antenna miscellaneous setting.

	Arguments:
		pAd						Pointer to our adapter
		BandState				Indicate current Band State.

	Return Value:
		None

	IRQL <= DISPATCH_LEVEL
	
	Note:
		1.) Frame End type control
			only valid for G only (RF_2527 & RF_2529)
			0: means DPDT, set BBP R4 bit 5 to 1
			1: means SPDT, set BBP R4 bit 5 to 0
			

	========================================================================
*/
VOID	AsicAntennaSetting(
	IN	PRTMP_ADAPTER	pAd,
	IN	ABGBAND_STATE	BandState)
{
}

VOID AsicRfTuningExec(
	IN PVOID SystemSpecific1,
	IN PVOID FunctionContext,
	IN PVOID SystemSpecific2,
	IN PVOID SystemSpecific3)
{
}

/*
	==========================================================================
	Description:
		Gives CCK TX rate 2 more dB TX power.
		This routine works only in LINK UP in INFRASTRUCTURE mode.

		calculate desired Tx power in RF R3.Tx0~5,	should consider -
		0. if current radio is a noisy environment (pAd->DrsCounters.fNoisyEnvironment)
		1. TxPowerPercentage
		2. auto calibration based on TSSI feedback
		3. extra 2 db for CCK
		4. -10 db upon very-short distance (AvgRSSI >= -40db) to AP

	NOTE: Since this routine requires the value of (pAd->DrsCounters.fNoisyEnvironment),
		it should be called AFTER MlmeDynamicTxRatSwitching()
	==========================================================================
 */
VOID AsicAdjustTxPower(
	IN PRTMP_ADAPTER pAd) 
{
	INT			i, j;
	CHAR		DeltaPwr = 0;
	BOOLEAN		bAutoTxAgc = FALSE;
	UCHAR		TssiRef, *pTssiMinusBoundary, *pTssiPlusBoundary, TxAgcStep;
	UCHAR		BbpR1 = 0, BbpR49 = 0, idx;
	PCHAR		pTxAgcCompensate;
	ULONG		TxPwr[5];
	CHAR		Value;

	if (pAd->CommonCfg.BBPCurrentBW == BW_40)
	{
		if (pAd->CommonCfg.CentralChannel > 14)
		{
			TxPwr[0] = pAd->Tx40MPwrCfgABand[0];
			TxPwr[1] = pAd->Tx40MPwrCfgABand[1];
			TxPwr[2] = pAd->Tx40MPwrCfgABand[2];
			TxPwr[3] = pAd->Tx40MPwrCfgABand[3];
			TxPwr[4] = pAd->Tx40MPwrCfgABand[4];
		}
		else
		{
			TxPwr[0] = pAd->Tx40MPwrCfgGBand[0];
			TxPwr[1] = pAd->Tx40MPwrCfgGBand[1];
			TxPwr[2] = pAd->Tx40MPwrCfgGBand[2];
			TxPwr[3] = pAd->Tx40MPwrCfgGBand[3];
			TxPwr[4] = pAd->Tx40MPwrCfgGBand[4];
		}
	}
	else
	{
		TxPwr[0] = pAd->TxPwrCfg[0];
		TxPwr[1] = pAd->TxPwrCfg[1];
		TxPwr[2] = pAd->TxPwrCfg[2];
		TxPwr[3] = pAd->TxPwrCfg[3];
		TxPwr[4] = pAd->TxPwrCfg[4];
	}

	// TX power compensation for temperature variation based on TSSI. try every 4 second
	if (pAd->Mlme.OneSecPeriodicRound % 4 == 0)
	{
		if (pAd->CommonCfg.Channel <= 14)
		{
			/* bg channel */
			bAutoTxAgc         = pAd->bAutoTxAgcG;
			TssiRef            = pAd->TssiRefG;
			pTssiMinusBoundary = &pAd->TssiMinusBoundaryG[0];
			pTssiPlusBoundary  = &pAd->TssiPlusBoundaryG[0];
			TxAgcStep          = pAd->TxAgcStepG;
			pTxAgcCompensate   = &pAd->TxAgcCompensateG;
		}
		else
		{
			/* a channel */
			bAutoTxAgc         = pAd->bAutoTxAgcA;
			TssiRef            = pAd->TssiRefA;
			pTssiMinusBoundary = &pAd->TssiMinusBoundaryA[0];
			pTssiPlusBoundary  = &pAd->TssiPlusBoundaryA[0];
			TxAgcStep          = pAd->TxAgcStepA;
			pTxAgcCompensate   = &pAd->TxAgcCompensateA;
		}

		if (bAutoTxAgc)
		{
			/* BbpR1 is unsigned char */
			RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R49, &BbpR49);

			/* (p) TssiPlusBoundaryG[0] = 0 = (m) TssiMinusBoundaryG[0] */
			/* compensate: +4     +3   +2   +1    0   -1   -2   -3   -4 * steps */
			/* step value is defined in pAd->TxAgcStepG for tx power value */

			/* [4]+1+[4]   p4     p3   p2   p1   o1   m1   m2   m3   m4 */
			/* ex:         0x00 0x15 0x25 0x45 0x88 0xA0 0xB5 0xD0 0xF0
			   above value are examined in mass factory production */
			/*             [4]    [3]  [2]  [1]  [0]  [1]  [2]  [3]  [4] */

			/* plus (+) is 0x00 ~ 0x45, minus (-) is 0xa0 ~ 0xf0 */
			/* if value is between p1 ~ o1 or o1 ~ s1, no need to adjust tx power */
			/* if value is 0xa5, tx power will be -= TxAgcStep*(2-1) */

			if (BbpR49 > pTssiMinusBoundary[1])
			{
				// Reading is larger than the reference value
				// check for how large we need to decrease the Tx power
				for (idx = 1; idx < 5; idx++)
				{
					if (BbpR49 <= pTssiMinusBoundary[idx])  // Found the range
						break;
				}
				// The index is the step we should decrease, idx = 0 means there is nothing to compensate
//				if (R3 > (ULONG) (TxAgcStep * (idx-1)))
					*pTxAgcCompensate = -(TxAgcStep * (idx-1));
//				else
//					*pTxAgcCompensate = -((UCHAR)R3);
				
				DeltaPwr += (*pTxAgcCompensate);
				DBGPRINT(RT_DEBUG_TRACE, ("-- Tx Power, BBP R1=%x, TssiRef=%x, TxAgcStep=%x, step = -%d\n",
					BbpR49, TssiRef, TxAgcStep, idx-1));                    
			}
			else if (BbpR49 < pTssiPlusBoundary[1])
			{
				// Reading is smaller than the reference value
				// check for how large we need to increase the Tx power
				for (idx = 1; idx < 5; idx++)
				{
					if (BbpR49 >= pTssiPlusBoundary[idx])   // Found the range
						break;
				}
				// The index is the step we should increase, idx = 0 means there is nothing to compensate
				*pTxAgcCompensate = TxAgcStep * (idx-1);
				DeltaPwr += (*pTxAgcCompensate);
				DBGPRINT(RT_DEBUG_TRACE, ("++ Tx Power, BBP R1=%x, TssiRef=%x, TxAgcStep=%x, step = +%d\n",
					BbpR49, TssiRef, TxAgcStep, idx-1));
			}
			else
			{
				*pTxAgcCompensate = 0;
				DBGPRINT(RT_DEBUG_TRACE, ("   Tx Power, BBP R49=%x, TssiRef=%x, TxAgcStep=%x, step = +%d\n",
					BbpR49, TssiRef, TxAgcStep, 0));
			}
		}
	}
	else
	{
		if (pAd->CommonCfg.Channel <= 14)
		{
			bAutoTxAgc         = pAd->bAutoTxAgcG;
			pTxAgcCompensate   = &pAd->TxAgcCompensateG;
		}
		else
		{
			bAutoTxAgc         = pAd->bAutoTxAgcA;
			pTxAgcCompensate   = &pAd->TxAgcCompensateA;
		}

		if (bAutoTxAgc)
			DeltaPwr += (*pTxAgcCompensate);
	}

	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R1, &BbpR1);
	BbpR1 &= 0xFC;

	/* calculate delta power based on the percentage specified from UI */
	// E2PROM setting is calibrated for maximum TX power (i.e. 100%)
	// We lower TX power here according to the percentage specified from UI
	if (pAd->CommonCfg.TxPowerPercentage == 0xffffffff)       // AUTO TX POWER control
		;
	else if (pAd->CommonCfg.TxPowerPercentage > 90)  // 91 ~ 100% & AUTO, treat as 100% in terms of mW
		;
	else if (pAd->CommonCfg.TxPowerPercentage > 60)  // 61 ~ 90%, treat as 75% in terms of mW		// DeltaPwr -= 1;
	{
		DeltaPwr -= 1;
	}
	else if (pAd->CommonCfg.TxPowerPercentage > 30)  // 31 ~ 60%, treat as 50% in terms of mW		// DeltaPwr -= 3;
	{
		DeltaPwr -= 3;
	}
	else if (pAd->CommonCfg.TxPowerPercentage > 15)  // 16 ~ 30%, treat as 25% in terms of mW		// DeltaPwr -= 6;
	{
		BbpR1 |= 0x01;
	}
	else if (pAd->CommonCfg.TxPowerPercentage > 9)   // 10 ~ 15%, treat as 12.5% in terms of mW		// DeltaPwr -= 9;
	{
		BbpR1 |= 0x01;
		DeltaPwr -= 3;
	}
	else                                           // 0 ~ 9 %, treat as MIN(~3%) in terms of mW		// DeltaPwr -= 12;
	{
		BbpR1 |= 0x02;
	}

	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R1, BbpR1);

	/* reset different new tx power for different TX rate */
	for(i=0; i<5; i++)
	{
		if (TxPwr[i] != 0xffffffff)
		{
			for (j=0; j<8; j++)
			{
				Value = (CHAR)((TxPwr[i] >> j*4) & 0x0F); /* 0 ~ 15 */

				if ((Value + DeltaPwr) < 0)
				{
					Value = 0; /* min */
				}
				else if ((Value + DeltaPwr) > 0xF)
				{
					Value = 0xF; /* max */
				}
				else
				{
					Value += DeltaPwr; /* temperature compensation */
				}

				/* fill new value to CSR offset */
				TxPwr[i] = (TxPwr[i] & ~(0x0000000F << j*4)) | (Value << j*4);
			}

			/* write tx power value to CSR */
			/* TX_PWR_CFG_0 (8 tx rate) for	TX power for OFDM 12M/18M
											TX power for OFDM 6M/9M
											TX power for CCK5.5M/11M
											TX power for CCK1M/2M */
			/* TX_PWR_CFG_1 ~ TX_PWR_CFG_4 */
			RTMP_IO_WRITE32(pAd, TX_PWR_CFG_0 + i*4, TxPwr[i]);

			DBGPRINT(RT_DEBUG_INFO, ("AsicAdjustTxPower - DeltaPwr=%d, offset=0x%x, TxPwr=%lx, BbpR1=%x, BbpR49=%x, round=%ld, pTxAgcCompensate=%d \n",
				DeltaPwr, TX_PWR_CFG_0 + i*4, TxPwr[i], BbpR1, BbpR49, pAd->Mlme.OneSecPeriodicRound, *pTxAgcCompensate));
		}
	}

	DBGPRINT(RT_DEBUG_INFO, ("<-- AsicAdjustTxPower, DeltaPwr=%d\n", DeltaPwr));
}

/*
	==========================================================================
	Description:
		Set My BSSID

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
VOID AsicSetBssid(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR pBssid) 
{
	ULONG		  Addr4;
	DBGPRINT(RT_DEBUG_TRACE, ("==============> AsicSetBssid %x:%x:%x:%x:%x:%x\n",
		pBssid[0],pBssid[1],pBssid[2],pBssid[3], pBssid[4],pBssid[5]));
	
	Addr4 = (ULONG)(pBssid[0])		 | 
			(ULONG)(pBssid[1] << 8)  | 
			(ULONG)(pBssid[2] << 16) |
			(ULONG)(pBssid[3] << 24);
	RTMP_IO_WRITE32(pAd, MAC_BSSID_DW0, Addr4);

	Addr4 = 0;
	// always one BSSID in STA mode
	Addr4 = (ULONG)(pBssid[4]) | (ULONG)(pBssid[5] << 8);

	RTMP_IO_WRITE32(pAd, MAC_BSSID_DW1, Addr4);
}

VOID AsicSetMcastWC(
	IN PRTMP_ADAPTER pAd) 
{
	MAC_TABLE_ENTRY *pEntry = &pAd->MacTab.Content[MCAST_WCID];
	USHORT		offset;
	
	pEntry->Sst        = SST_ASSOC;
	pEntry->Aid        = MCAST_WCID;	// Softap supports 1 BSSID and use WCID=0 as multicast Wcid index
	pEntry->PsMode     = PWR_ACTIVE;
	pEntry->CurrTxRate = pAd->CommonCfg.MlmeRate; 
	offset = MAC_WCID_BASE + BSS0Mcast_WCID * HW_WCID_ENTRY_SIZE;
}
#if 0	// remove by AlbertY. it can be cancelled.
/*
	==========================================================================
	Description:  Bssid also is a Wireless Client . So add Bssid MAC.

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
VOID AsicSetBssidWC(
	IN PRTMP_ADAPTER pAd, 
	IN PUCHAR pBssid) 
{
	ULONG		  Addr4;
	ULONG 		offset;
	UCHAR	BCASTADDR[6]={0x1, 0, 0,0,0,0};

	DBGPRINT(RT_DEBUG_TRACE, ("==============> AsicSetBssidWC %x:%x:%x:%x:%x:%x\n",
		pBssid[0],pBssid[1],pBssid[2],pBssid[3], pBssid[4],pBssid[5]));

	offset = MAC_WCID_BASE + BSSID_WCID * HW_WCID_ENTRY_SIZE;
	Addr4 = (ULONG)(pBssid[0])		 | 
		(ULONG)(pBssid[1] << 8)  | 
		(ULONG)(pBssid[2] << 16) |
		(ULONG)(pBssid[3] << 24);
	RTMP_IO_WRITE32(pAd, offset, Addr4);
	offset += 4;
	// always one BSSID in STA mode
	Addr4 = (ULONG)(pBssid[4]) | (ULONG)(pBssid[5] << 8);
	RTMP_IO_WRITE32(pAd, offset, Addr4);
	//
	//  Let's always set this no matter INFRA or ADHOC
	//
#ifdef CONFIG_AP_SUPPORT	
	//if (pAd->OpMode == OPMODE_AP)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("==============AsicSetMcastWC1 %x:%x:%x:%x:%x:%x\n",
			BCASTADDR[0],BCASTADDR[1],BCASTADDR[2],BCASTADDR[3], BCASTADDR[4],BCASTADDR[5]));
		offset = MAC_WCID_BASE + BSS0Mcast_WCID * HW_WCID_ENTRY_SIZE;
		Addr4 = (ULONG)(BCASTADDR[0])		 | 
			(ULONG)(BCASTADDR[1] << 8)  | 
			(ULONG)(BCASTADDR[2] << 16) |
			(ULONG)(BCASTADDR[3] << 24);
		RTMP_IO_WRITE32(pAd, offset, Addr4);
		offset += 4;
		// always one BSSID in STA mode
		Addr4 = (ULONG)(BCASTADDR[4]) | (ULONG)(BCASTADDR[5] << 8);
		RTMP_IO_WRITE32(pAd, offset, Addr4);
	}
#endif // CONFIG_AP_SUPPORT //

}
#endif
/*
	==========================================================================
	Description:   

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */
VOID AsicDelWcidTab(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR	Wcid) 
{
	ULONG		  Addr0 = 0x0, Addr1 = 0x0;
	ULONG 		offset;

	DBGPRINT(RT_DEBUG_TRACE, ("AsicDelWcidTab==>Wcid = 0x%x\n",Wcid));
	offset = MAC_WCID_BASE + Wcid * HW_WCID_ENTRY_SIZE;
	RTMP_IO_WRITE32(pAd, offset, Addr0);
	offset += 4;
	RTMP_IO_WRITE32(pAd, offset, Addr1);
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicEnableRDG(
	IN PRTMP_ADAPTER pAd) 
{
	TX_LINK_CFG_STRUC	TxLinkCfg;
	UINT32				Data = 0;

	RTMP_IO_READ32(pAd, TX_LINK_CFG, &TxLinkCfg.word);
	TxLinkCfg.field.TxRDGEn = 1;
	RTMP_IO_WRITE32(pAd, TX_LINK_CFG, TxLinkCfg.word);

	RTMP_IO_READ32(pAd, EDCA_AC0_CFG, &Data);
	Data  &= 0xFFFFFF00;
	Data  |= 0x80;
	RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, Data);

	//OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED);
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicDisableRDG(
	IN PRTMP_ADAPTER pAd) 
{
	TX_LINK_CFG_STRUC	TxLinkCfg;
	UINT32				Data = 0;

	DBGPRINT(RT_DEBUG_INFO, ("--->AsicDisableRDG \n"));

	RTMP_IO_READ32(pAd, TX_LINK_CFG, &TxLinkCfg.word);
	TxLinkCfg.field.TxRDGEn = 0;
	RTMP_IO_WRITE32(pAd, TX_LINK_CFG, TxLinkCfg.word);

	RTMP_IO_READ32(pAd, EDCA_AC0_CFG, &Data);
	
	Data  &= 0xFFFFFF00;
	//Data  |= 0x20;
#ifndef WIFI_TEST
	//if ( pAd->CommonCfg.bEnableTxBurst )	
	//	Data |= 0x60; // for performance issue not set the TXOP to 0
#endif	
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_DYNAMIC_BE_TXOP_ACTIVE) && (pAd->MacTab.fAnyStationMIMOPSDynamic == FALSE))
	{
		Data |= 0x30;
	}
	RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, Data);
}

/*
	==========================================================================
	Description:

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicDisableSync(
	IN PRTMP_ADAPTER pAd) 
{
	BCN_TIME_CFG_STRUC csr;
	
	DBGPRINT(RT_DEBUG_TRACE, ("--->Disable TSF synchronization\n"));

	// 2003-12-20 disable TSF and TBTT while NIC in power-saving have side effect
	//			  that NIC will never wakes up because TSF stops and no more 
	//			  TBTT interrupts
	pAd->TbttTickCount = 0;
	RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr.word);
	csr.field.bBeaconGen = 0;
	csr.field.bTBTTEnable = 0;
	csr.field.TsfSyncMode = 0;
	csr.field.bTsfTicking = 0;
	RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr.word);

}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicEnableBssSync(
	IN PRTMP_ADAPTER pAd) 
{
	BCN_TIME_CFG_STRUC csr;

	DBGPRINT(RT_DEBUG_TRACE, ("--->AsicEnableBssSync(INFRA mode)\n"));

	RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr.word);
//	RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, 0x00000000);
#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		csr.field.BeaconInterval = pAd->CommonCfg.BeaconPeriod << 4; // ASIC register in units of 1/16 TU
		csr.field.bTsfTicking = 1;
		csr.field.TsfSyncMode = 3; // sync TSF similar as in ADHOC mode?
		csr.field.bBeaconGen  = 1; // AP should generate BEACON
		csr.field.bTBTTEnable = 1;
	}
#endif // CONFIG_AP_SUPPORT //
	RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr.word);
}

/*
	==========================================================================
	Description:
	Note: 
		BEACON frame in shared memory should be built ok before this routine
		can be called. Otherwise, a garbage frame maybe transmitted out every
		Beacon period.

	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicEnableIbssSync(
	IN PRTMP_ADAPTER pAd)
{
	BCN_TIME_CFG_STRUC csr9;
	PUCHAR			ptr;
	UINT i;

	DBGPRINT(RT_DEBUG_TRACE, ("--->AsicEnableIbssSync(ADHOC mode. MPDUtotalByteCount = %d)\n", pAd->BeaconTxWI.MPDUtotalByteCount));

	RTMP_IO_READ32(pAd, BCN_TIME_CFG, &csr9.word);
	csr9.field.bBeaconGen = 0;
	csr9.field.bTBTTEnable = 0;
	csr9.field.bTsfTicking = 0;
	RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr9.word);

#ifdef RT2860
	// move BEACON TXD and frame content to on-chip memory
	ptr = (PUCHAR)&pAd->BeaconTxWI;
	for (i=0; i<TXWI_SIZE; i+=4)  // 16-byte TXWI field
	{
		UINT32 longptr =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, HW_BEACON_BASE0 + i, longptr);
		ptr += 4;
	}

	// start right after the 16-byte TXWI field
	ptr = pAd->BeaconBuf;
	for (i=0; i< pAd->BeaconTxWI.MPDUtotalByteCount; i+=4)
	{
		UINT32 longptr =  *ptr + (*(ptr+1)<<8) + (*(ptr+2)<<16) + (*(ptr+3)<<24);
		RTMP_IO_WRITE32(pAd, HW_BEACON_BASE0 + TXWI_SIZE + i, longptr);
		ptr +=4;
	}
#endif // RT2860 //


	//
	// For Wi-Fi faily generated beacons between participating stations. 
	// Set TBTT phase adaptive adjustment step to 8us (default 16us)
	// don't change settings 2006-5- by Jerry
	//RTMP_IO_WRITE32(pAd, TBTT_SYNC_CFG, 0x00001010);
	
	// start sending BEACON
	csr9.field.BeaconInterval = pAd->CommonCfg.BeaconPeriod << 4; // ASIC register in units of 1/16 TU
	csr9.field.bTsfTicking = 1;
	csr9.field.TsfSyncMode = 2; // sync TSF in IBSS mode
	csr9.field.bTBTTEnable = 1;
	csr9.field.bBeaconGen = 1;
	RTMP_IO_WRITE32(pAd, BCN_TIME_CFG, csr9.word);
}

/*
	==========================================================================
	Description:

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID AsicSetEdcaParm(
	IN PRTMP_ADAPTER pAd,
	IN PEDCA_PARM	 pEdcaParm)
{
	EDCA_AC_CFG_STRUC   Ac0Cfg, Ac1Cfg, Ac2Cfg, Ac3Cfg;
	AC_TXOP_CSR0_STRUC csr0;
	AC_TXOP_CSR1_STRUC csr1;
	AIFSN_CSR_STRUC    AifsnCsr;
	CWMIN_CSR_STRUC    CwminCsr;
	CWMAX_CSR_STRUC    CwmaxCsr;
	int i;

	Ac0Cfg.word = 0;
	Ac1Cfg.word = 0;
	Ac2Cfg.word = 0;
	Ac3Cfg.word = 0;
	if ((pEdcaParm == NULL) || (pEdcaParm->bValid == FALSE))
	{
		DBGPRINT(RT_DEBUG_TRACE,("AsicSetEdcaParm\n"));
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_WMM_INUSED);
		for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
		{
			if (pAd->MacTab.Content[i].ValidAsCLI || pAd->MacTab.Content[i].ValidAsApCli)
				CLIENT_STATUS_CLEAR_FLAG(&pAd->MacTab.Content[i], fCLIENT_STATUS_WMM_CAPABLE);
		}

		//========================================================
		//      MAC Register has a copy .
		//========================================================
//#ifndef WIFI_TEST
		if( pAd->CommonCfg.bEnableTxBurst )		
			Ac0Cfg.field.AcTxop = 0x30; // Suggest by John for TxBurst in HT Mode
		else
			Ac0Cfg.field.AcTxop = 0;	// QID_AC_BE
//#else
//		Ac0Cfg.field.AcTxop = 0;	// QID_AC_BE
//#endif					
		Ac0Cfg.field.Cwmin = CW_MIN_IN_BITS;
		Ac0Cfg.field.Cwmax = CW_MAX_IN_BITS;
		Ac0Cfg.field.Aifsn = 2;
		RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, Ac0Cfg.word);

		Ac1Cfg.field.AcTxop = 0;	// QID_AC_BK
		Ac1Cfg.field.Cwmin = CW_MIN_IN_BITS;
		Ac1Cfg.field.Cwmax = CW_MAX_IN_BITS;
		Ac1Cfg.field.Aifsn = 2;
		RTMP_IO_WRITE32(pAd, EDCA_AC1_CFG, Ac1Cfg.word);

		if (pAd->CommonCfg.PhyMode == PHY_11B)
		{
			Ac2Cfg.field.AcTxop = 192;	// AC_VI: 192*32us ~= 6ms
			Ac3Cfg.field.AcTxop = 96;	// AC_VO: 96*32us  ~= 3ms
		}
		else
		{
			Ac2Cfg.field.AcTxop = 96;	// AC_VI: 96*32us ~= 3ms
			Ac3Cfg.field.AcTxop = 48;	// AC_VO: 48*32us ~= 1.5ms
		}
		Ac2Cfg.field.Cwmin = CW_MIN_IN_BITS;
		Ac2Cfg.field.Cwmax = CW_MAX_IN_BITS;
		Ac2Cfg.field.Aifsn = 2;
		RTMP_IO_WRITE32(pAd, EDCA_AC2_CFG, Ac2Cfg.word);
		Ac3Cfg.field.Cwmin = CW_MIN_IN_BITS;
		Ac3Cfg.field.Cwmax = CW_MAX_IN_BITS;
		Ac3Cfg.field.Aifsn = 2;
		RTMP_IO_WRITE32(pAd, EDCA_AC3_CFG, Ac3Cfg.word);

		//========================================================
		//      DMA Register has a copy too.
		//========================================================
		csr0.field.Ac0Txop = 0;		// QID_AC_BE
		csr0.field.Ac1Txop = 0;		// QID_AC_BK
		RTMP_IO_WRITE32(pAd, WMM_TXOP0_CFG, csr0.word);
		if (pAd->CommonCfg.PhyMode == PHY_11B)
		{
			csr1.field.Ac2Txop = 192;		// AC_VI: 192*32us ~= 6ms
			csr1.field.Ac3Txop = 96;		// AC_VO: 96*32us  ~= 3ms
		}
		else
		{
			csr1.field.Ac2Txop = 96;		// AC_VI: 96*32us ~= 3ms
			csr1.field.Ac3Txop = 48;		// AC_VO: 48*32us ~= 1.5ms
		}
		RTMP_IO_WRITE32(pAd, WMM_TXOP1_CFG, csr1.word);

		CwminCsr.word = 0;
		CwminCsr.field.Cwmin0 = CW_MIN_IN_BITS;
		CwminCsr.field.Cwmin1 = CW_MIN_IN_BITS;
		CwminCsr.field.Cwmin2 = CW_MIN_IN_BITS;
		CwminCsr.field.Cwmin3 = CW_MIN_IN_BITS;
		RTMP_IO_WRITE32(pAd, WMM_CWMIN_CFG, CwminCsr.word);

		CwmaxCsr.word = 0;
		CwmaxCsr.field.Cwmax0 = CW_MAX_IN_BITS;
		CwmaxCsr.field.Cwmax1 = CW_MAX_IN_BITS;
		CwmaxCsr.field.Cwmax2 = CW_MAX_IN_BITS;
		CwmaxCsr.field.Cwmax3 = CW_MAX_IN_BITS;
		RTMP_IO_WRITE32(pAd, WMM_CWMAX_CFG, CwmaxCsr.word);

		RTMP_IO_WRITE32(pAd, WMM_AIFSN_CFG, 0x00002222);

		NdisZeroMemory(&pAd->CommonCfg.APEdcaParm, sizeof(EDCA_PARM));
	}
	else
	{
		OPSTATUS_SET_FLAG(pAd, fOP_STATUS_WMM_INUSED);
		//========================================================
		//      MAC Register has a copy.
		//========================================================
		//
		// Modify Cwmin/Cwmax/Txop on queue[QID_AC_VI], Recommend by Jerry 2005/07/27
		// To degrade our VIDO Queue's throughput for WiFi WMM S3T07 Issue.
		//
		//pEdcaParm->Txop[QID_AC_VI] = pEdcaParm->Txop[QID_AC_VI] * 7 / 10; // rt2860c need this		

		Ac0Cfg.field.AcTxop =  pEdcaParm->Txop[QID_AC_BE];
		Ac0Cfg.field.Cwmin= pEdcaParm->Cwmin[QID_AC_BE];
		Ac0Cfg.field.Cwmax = pEdcaParm->Cwmax[QID_AC_BE];
		Ac0Cfg.field.Aifsn = pEdcaParm->Aifsn[QID_AC_BE]; //+1;

		Ac1Cfg.field.AcTxop =  pEdcaParm->Txop[QID_AC_BK];
		Ac1Cfg.field.Cwmin = pEdcaParm->Cwmin[QID_AC_BK]; //+2; 
		Ac1Cfg.field.Cwmax = pEdcaParm->Cwmax[QID_AC_BK];
		Ac1Cfg.field.Aifsn = pEdcaParm->Aifsn[QID_AC_BK]; //+1;

		Ac2Cfg.field.AcTxop = (pEdcaParm->Txop[QID_AC_VI] * 6) / 10;
		Ac2Cfg.field.Cwmin = pEdcaParm->Cwmin[QID_AC_VI];
		Ac2Cfg.field.Cwmax = pEdcaParm->Cwmax[QID_AC_VI];
		Ac2Cfg.field.Aifsn = pEdcaParm->Aifsn[QID_AC_VI];

		Ac3Cfg.field.AcTxop = pEdcaParm->Txop[QID_AC_VO];
		Ac3Cfg.field.Cwmin = pEdcaParm->Cwmin[QID_AC_VO];
		Ac3Cfg.field.Cwmax = pEdcaParm->Cwmax[QID_AC_VO];
		Ac3Cfg.field.Aifsn = pEdcaParm->Aifsn[QID_AC_VO];

//#ifdef WIFI_TEST
		if (pAd->CommonCfg.bWiFiTest)
		{
			if (Ac3Cfg.field.AcTxop == 102)
			{
			Ac0Cfg.field.AcTxop = pEdcaParm->Txop[QID_AC_BE] ? pEdcaParm->Txop[QID_AC_BE] : 10;
				Ac0Cfg.field.Aifsn  = pEdcaParm->Aifsn[QID_AC_BE]-1; /* AIFSN must >= 1 */
			Ac1Cfg.field.AcTxop = pEdcaParm->Txop[QID_AC_BK];
				Ac1Cfg.field.Aifsn  = pEdcaParm->Aifsn[QID_AC_BK];
			Ac2Cfg.field.AcTxop = pEdcaParm->Txop[QID_AC_VI];
			} /* End of if */
		}
//#endif // WIFI_TEST //

		RTMP_IO_WRITE32(pAd, EDCA_AC0_CFG, Ac0Cfg.word);
		RTMP_IO_WRITE32(pAd, EDCA_AC1_CFG, Ac1Cfg.word);
		RTMP_IO_WRITE32(pAd, EDCA_AC2_CFG, Ac2Cfg.word);
		RTMP_IO_WRITE32(pAd, EDCA_AC3_CFG, Ac3Cfg.word);


		//========================================================
		//      DMA Register has a copy too.
		//========================================================
		csr0.field.Ac0Txop = Ac0Cfg.field.AcTxop;
		csr0.field.Ac1Txop = Ac1Cfg.field.AcTxop;
		RTMP_IO_WRITE32(pAd, WMM_TXOP0_CFG, csr0.word);

		csr1.field.Ac2Txop = Ac2Cfg.field.AcTxop;
		csr1.field.Ac3Txop = Ac3Cfg.field.AcTxop;
		RTMP_IO_WRITE32(pAd, WMM_TXOP1_CFG, csr1.word);

		CwminCsr.word = 0;
		CwminCsr.field.Cwmin0 = pEdcaParm->Cwmin[QID_AC_BE];
		CwminCsr.field.Cwmin1 = pEdcaParm->Cwmin[QID_AC_BK];
		CwminCsr.field.Cwmin2 = pEdcaParm->Cwmin[QID_AC_VI];
#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			CwminCsr.field.Cwmin3 = pEdcaParm->Cwmin[QID_AC_VO];
#endif // CONFIG_AP_SUPPORT //
		RTMP_IO_WRITE32(pAd, WMM_CWMIN_CFG, CwminCsr.word);

		CwmaxCsr.word = 0;
		CwmaxCsr.field.Cwmax0 = pEdcaParm->Cwmax[QID_AC_BE];
		CwmaxCsr.field.Cwmax1 = pEdcaParm->Cwmax[QID_AC_BK];
		CwmaxCsr.field.Cwmax2 = pEdcaParm->Cwmax[QID_AC_VI];
		CwmaxCsr.field.Cwmax3 = pEdcaParm->Cwmax[QID_AC_VO];
		RTMP_IO_WRITE32(pAd, WMM_CWMAX_CFG, CwmaxCsr.word);

		AifsnCsr.word = 0;
		AifsnCsr.field.Aifsn0 = Ac0Cfg.field.Aifsn; //pEdcaParm->Aifsn[QID_AC_BE];
		AifsnCsr.field.Aifsn1 = Ac1Cfg.field.Aifsn; //pEdcaParm->Aifsn[QID_AC_BK];
		AifsnCsr.field.Aifsn2 = Ac2Cfg.field.Aifsn; //pEdcaParm->Aifsn[QID_AC_VI];

#ifdef CONFIG_AP_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			AifsnCsr.field.Aifsn3 = Ac3Cfg.field.Aifsn; //pEdcaParm->Aifsn[QID_AC_VO]
#endif // CONFIG_AP_SUPPORT //
		RTMP_IO_WRITE32(pAd, WMM_AIFSN_CFG, AifsnCsr.word);

		NdisMoveMemory(&pAd->CommonCfg.APEdcaParm, pEdcaParm, sizeof(EDCA_PARM));
		if (!ADHOC_ON(pAd))
		{
			DBGPRINT(RT_DEBUG_TRACE,("EDCA [#%d]: AIFSN CWmin CWmax  TXOP(us)  ACM\n", pEdcaParm->EdcaUpdateCount));
			DBGPRINT(RT_DEBUG_TRACE,("     AC_BE      %2d     %2d     %2d      %4d     %d\n",
									 pEdcaParm->Aifsn[0],
									 pEdcaParm->Cwmin[0],
									 pEdcaParm->Cwmax[0],
									 pEdcaParm->Txop[0]<<5,
									 pEdcaParm->bACM[0]));
			DBGPRINT(RT_DEBUG_TRACE,("     AC_BK      %2d     %2d     %2d      %4d     %d\n",
									 pEdcaParm->Aifsn[1],
									 pEdcaParm->Cwmin[1],
									 pEdcaParm->Cwmax[1],
									 pEdcaParm->Txop[1]<<5,
									 pEdcaParm->bACM[1]));
			DBGPRINT(RT_DEBUG_TRACE,("     AC_VI      %2d     %2d     %2d      %4d     %d\n",
									 pEdcaParm->Aifsn[2],
									 pEdcaParm->Cwmin[2],
									 pEdcaParm->Cwmax[2],
									 pEdcaParm->Txop[2]<<5,
									 pEdcaParm->bACM[2]));
			DBGPRINT(RT_DEBUG_TRACE,("     AC_VO      %2d     %2d     %2d      %4d     %d\n",
									 pEdcaParm->Aifsn[3],
									 pEdcaParm->Cwmin[3],
									 pEdcaParm->Cwmax[3],
									 pEdcaParm->Txop[3]<<5,
									 pEdcaParm->bACM[3]));
		}
	}
}

/*
	==========================================================================
	Description:

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	==========================================================================
 */
VOID 	AsicSetSlotTime(
	IN PRTMP_ADAPTER pAd,
	IN BOOLEAN bUseShortSlotTime) 
{
	ULONG	SlotTime;
	UINT32	RegValue = 0;

#if 0	
	bUseShortSlotTime = TRUE; // 2005-04-15 john
	return;
	if (bUseShortSlotTime && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED))
		return;
	else if ((!bUseShortSlotTime) && (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED)))
		return;
#endif

	if (bUseShortSlotTime)
		OPSTATUS_SET_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED);
	else
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED);

	SlotTime = (bUseShortSlotTime)? 9 : 20;


	//
	// For some reasons, always set it to short slot time.
	// 
	// ToDo: Should consider capability with 11B
	//

	RTMP_IO_READ32(pAd, BKOFF_SLOT_CFG, &RegValue);
	RegValue = RegValue & 0xFFFFFF00;

	RegValue |= SlotTime;

	RTMP_IO_WRITE32(pAd, BKOFF_SLOT_CFG, RegValue);

	DBGPRINT(RT_DEBUG_INFO, ("AsicSetSlotTime(=%ld us)\n", SlotTime));
}

#if 0 // removed by Albert
// BssIndex = 0 in windows driver. Call this when under WPA auth mode.
VOID AsicAddWcidCipherEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR		 WCID,
	IN UCHAR		 BssIndex,
	IN UCHAR		 KeyTable,
	IN UCHAR		 CipherAlg,
	IN PUCHAR		 pAddr,
	IN CIPHER_KEY		 *pCipherKey)
		{
	ULONG 	WCIDAttri;
	ULONG 	offset;
	UCHAR	i;
	CIPHER_KEY	CipherKey;
				
	DBGPRINT(RT_DEBUG_TRACE, ("AsicAddWcidCipherEntry===>KeyTable=%d, CipherAlg = %d\n", KeyTable, CipherAlg));
	RTMPZeroMemory(&CipherKey, sizeof(CipherKey));
	// pAddr
	if (pAddr)
			{
		offset = MAC_WCID_BASE + (WCID * HW_WCID_ENTRY_SIZE);	
		for (i=0; i<MAC_ADDR_LEN; i++)
			{
			RTMP_IO_WRITE8(pAd, offset+i, pAddr[i]);
			}

		}

	// WCID Attribute UDF:3, BSSIdx:3, Alg:3, Keytable:1=PAIRWISE KEY, BSSIdx is 0



	offset = MAC_WCID_ATTRIBUTE_BASE + (WCID * HW_WCID_ATTRI_SIZE);

	WCIDAttri = (CipherAlg<<1)|KeyTable;
	RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
		
	CipherKey.CipherAlg = CipherAlg;

	AsicAddPairwiseKeyEntry(pAd, pAddr, WCID, pCipherKey);
}
#endif
/*
	========================================================================
	Description:
		Add Shared key information into ASIC. 
		Update shared key, TxMic and RxMic to Asic Shared key table
		Update its cipherAlg to Asic Shared key Mode.
		
    Return:
	========================================================================
*/
VOID AsicAddSharedKeyEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR		 BssIndex,
	IN UCHAR		 KeyIdx,
	IN UCHAR		 CipherAlg,
	IN PUCHAR		 pKey,
	IN PUCHAR		 pTxMic,
	IN PUCHAR		 pRxMic)
{
	ULONG offset; //, csr0;
	SHAREDKEY_MODE_STRUC csr1;
#ifdef RT2860
	INT   i;
#endif // RT2860 //

	DBGPRINT(RT_DEBUG_TRACE, ("AsicAddSharedKeyEntry BssIndex=%d, KeyIdx=%d\n", BssIndex,KeyIdx));
//============================================================================================

#if 0
	if ( !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_START_UP))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("AsicAddSharedKeyEntry rejected before Startup\n"));
		return;
	}
#endif

	DBGPRINT(RT_DEBUG_TRACE,("AsicAddSharedKeyEntry: %s key #%d\n", CipherName[CipherAlg], BssIndex*4 + KeyIdx));
	DBGPRINT_RAW(RT_DEBUG_TRACE, (" 	Key = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
		pKey[0],pKey[1],pKey[2],pKey[3],pKey[4],pKey[5],pKey[6],pKey[7],pKey[8],pKey[9],pKey[10],pKey[11],pKey[12],pKey[13],pKey[14],pKey[15]));
	if (pRxMic)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, (" 	Rx MIC Key = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			pRxMic[0],pRxMic[1],pRxMic[2],pRxMic[3],pRxMic[4],pRxMic[5],pRxMic[6],pRxMic[7]));
	}
	if (pTxMic)
	{
		DBGPRINT_RAW(RT_DEBUG_TRACE, (" 	Tx MIC Key = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			pTxMic[0],pTxMic[1],pTxMic[2],pTxMic[3],pTxMic[4],pTxMic[5],pTxMic[6],pTxMic[7]));
	}
//============================================================================================
	//
	// fill key material - key + TX MIC + RX MIC
	//
#ifdef RT2860
	offset = SHARED_KEY_TABLE_BASE + (4*BssIndex + KeyIdx)*HW_KEY_ENTRY_SIZE;
	for (i=0; i<MAX_LEN_OF_SHARE_KEY; i++) 
	{
		RTMP_IO_WRITE8(pAd, offset + i, pKey[i]);
	}

	offset += MAX_LEN_OF_SHARE_KEY;
	if (pTxMic)
	{
		for (i=0; i<8; i++)
		{
			RTMP_IO_WRITE8(pAd, offset + i, pTxMic[i]);
		}
	}

	offset += 8;
	if (pRxMic)
	{
		for (i=0; i<8; i++)
		{
			RTMP_IO_WRITE8(pAd, offset + i, pRxMic[i]);
		}
	}
#endif // RT2860 //


	//
	// Update cipher algorithm. WSTA always use BSS0
	//
	RTMP_IO_READ32(pAd, SHARED_KEY_MODE_BASE+4*(BssIndex/2), &csr1.word);
	DBGPRINT(RT_DEBUG_TRACE,("Read: SHARED_KEY_MODE_BASE at this Bss[%d] KeyIdx[%d]= 0x%x \n", BssIndex,KeyIdx, csr1.word));
	if ((BssIndex%2) == 0)
	{
		if (KeyIdx == 0)
			csr1.field.Bss0Key0CipherAlg = CipherAlg;
		else if (KeyIdx == 1)
			csr1.field.Bss0Key1CipherAlg = CipherAlg;
		else if (KeyIdx == 2)
			csr1.field.Bss0Key2CipherAlg = CipherAlg;
		else
			csr1.field.Bss0Key3CipherAlg = CipherAlg;
	}
	else
	{
		if (KeyIdx == 0)
			csr1.field.Bss1Key0CipherAlg = CipherAlg;
		else if (KeyIdx == 1)
			csr1.field.Bss1Key1CipherAlg = CipherAlg;
		else if (KeyIdx == 2)
			csr1.field.Bss1Key2CipherAlg = CipherAlg;
		else
			csr1.field.Bss1Key3CipherAlg = CipherAlg;
	}
	DBGPRINT(RT_DEBUG_TRACE,("Write: SHARED_KEY_MODE_BASE at this Bss[%d] = 0x%x \n", BssIndex, csr1.word));
	RTMP_IO_WRITE32(pAd, SHARED_KEY_MODE_BASE+4*(BssIndex/2), csr1.word);
		
}

//	IRQL = DISPATCH_LEVEL
VOID AsicRemoveSharedKeyEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR		 BssIndex,
	IN UCHAR		 KeyIdx)
{
	//ULONG SecCsr0;
	SHAREDKEY_MODE_STRUC csr1;

	DBGPRINT(RT_DEBUG_TRACE,("AsicRemoveSharedKeyEntry: #%d \n", BssIndex*4 + KeyIdx));

	RTMP_IO_READ32(pAd, SHARED_KEY_MODE_BASE+4*(BssIndex/2), &csr1.word);
	if ((BssIndex%2) == 0)
	{
		if (KeyIdx == 0)
			csr1.field.Bss0Key0CipherAlg = 0;
		else if (KeyIdx == 1)
			csr1.field.Bss0Key1CipherAlg = 0;
		else if (KeyIdx == 2)
			csr1.field.Bss0Key2CipherAlg = 0;
		else
			csr1.field.Bss0Key3CipherAlg = 0;
	}
	else
	{
		if (KeyIdx == 0)
			csr1.field.Bss1Key0CipherAlg = 0;
		else if (KeyIdx == 1)
			csr1.field.Bss1Key1CipherAlg = 0;
		else if (KeyIdx == 2)
			csr1.field.Bss1Key2CipherAlg = 0;
		else
			csr1.field.Bss1Key3CipherAlg = 0;
	}
	DBGPRINT(RT_DEBUG_TRACE,("Write: SHARED_KEY_MODE_BASE at this Bss[%d] = 0x%x \n", BssIndex, csr1.word));
	RTMP_IO_WRITE32(pAd, SHARED_KEY_MODE_BASE+4*(BssIndex/2), csr1.word);
	ASSERT(BssIndex < 4);
	ASSERT(KeyIdx < 4);

}


VOID AsicUpdateWCIDAttribute(
	IN PRTMP_ADAPTER pAd,
	IN USHORT		WCID,
	IN UCHAR		BssIndex,
	IN UCHAR        CipherAlg,
	IN BOOLEAN		bUsePairewiseKeyTable)
{
	ULONG   WCIDAttri = 0, offset;

	//
	// Update WCID attribute.
	// Only TxKey could update WCID attribute.
	//
	offset = MAC_WCID_ATTRIBUTE_BASE + (WCID * HW_WCID_ATTRI_SIZE);
	WCIDAttri = (BssIndex << 4) | (CipherAlg << 1) | (bUsePairewiseKeyTable);
	RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
}

VOID AsicUpdateWCIDIVEIV(
	IN PRTMP_ADAPTER pAd,
	IN USHORT		WCID,
	IN ULONG        uIV,
	IN ULONG        uEIV)
{
	ULONG	offset;

	offset = MAC_IVEIV_TABLE_BASE + (WCID * HW_IVEIV_ENTRY_SIZE);

	RTMP_IO_WRITE32(pAd, offset, uIV);
	RTMP_IO_WRITE32(pAd, offset + 4, uEIV);
}

VOID AsicUpdateRxWCIDTable(
	IN PRTMP_ADAPTER pAd,
	IN USHORT		WCID,
	IN PUCHAR        pAddr)
{
	ULONG offset;
	ULONG Addr;
	
	offset = MAC_WCID_BASE + (WCID * HW_WCID_ENTRY_SIZE);	
	Addr = pAddr[0] + (pAddr[1] << 8) +(pAddr[2] << 16) +(pAddr[3] << 24);
	RTMP_IO_WRITE32(pAd, offset, Addr);
	Addr = pAddr[4] + (pAddr[5] << 8);
	RTMP_IO_WRITE32(pAd, offset + 4, Addr);	
}
	

/*
    ========================================================================

    Routine Description:
        Set Cipher Key, Cipher algorithm, IV/EIV to Asic

    Arguments:
        pAd                     Pointer to our adapter
        WCID                    WCID Entry number.
        BssIndex                BSSID index, station or none multiple BSSID support 
                                this value should be 0.
        KeyIdx                  This KeyIdx will set to IV's KeyID if bTxKey enabled
        pCipherKey              Pointer to Cipher Key.
        bUsePairewiseKeyTable   TRUE means saved the key in SharedKey table, 
                                otherwise PairewiseKey table
        bTxKey                  This is the transmit key if enabled.

    Return Value:
        None 

    Note:
        This routine will set the relative key stuff to Asic including WCID attribute,
        Cipher Key, Cipher algorithm and IV/EIV.

        IV/EIV will be update if this CipherKey is the transmission key because 
        ASIC will base on IV's KeyID value to select Cipher Key.

        If bTxKey sets to FALSE, this is not the TX key, but it could be 
        RX key

    	For AP mode bTxKey must be always set to TRUE.
    ========================================================================
*/
VOID AsicAddKeyEntry(
	IN PRTMP_ADAPTER pAd,
	IN USHORT		WCID,
	IN UCHAR		BssIndex,
	IN UCHAR		KeyIdx,
	IN PCIPHER_KEY	pCipherKey,	
	IN BOOLEAN		bUsePairewiseKeyTable,
	IN BOOLEAN		bTxKey)
{
	ULONG	offset;
//	ULONG   WCIDAttri = 0;
	UCHAR	IV4 = 0;
	PUCHAR		pKey = pCipherKey->Key;
//	ULONG		KeyLen = pCipherKey->KeyLen;
	PUCHAR		pTxMic = pCipherKey->TxMic;
	PUCHAR		pRxMic = pCipherKey->RxMic;
	PUCHAR		pTxtsc = pCipherKey->TxTsc;
	UCHAR		CipherAlg = pCipherKey->CipherAlg;
	SHAREDKEY_MODE_STRUC csr1;
#ifdef RT2860
	UCHAR		i; 
#endif // RT2860 //

//	ASSERT(KeyLen <= MAX_LEN_OF_PEER_KEY);

	DBGPRINT(RT_DEBUG_TRACE, ("==> AsicAddKeyEntry\n"));
	//
	// 1.) decide key table offset
	//
	if (bUsePairewiseKeyTable)
		offset = PAIRWISE_KEY_TABLE_BASE + (WCID * HW_KEY_ENTRY_SIZE);
	else
		offset = SHARED_KEY_TABLE_BASE + (4 * BssIndex + KeyIdx) * HW_KEY_ENTRY_SIZE;

	//
	// 2.) Set Key to Asic
	//
	//for (i = 0; i < KeyLen; i++)
#ifdef RT2860
	for (i = 0; i < MAX_LEN_OF_PEER_KEY; i++)
	{
		RTMP_IO_WRITE8(pAd, offset + i, pKey[i]);
	}
	offset += MAX_LEN_OF_PEER_KEY;

	//
	// 3.) Set MIC key if available
	//
	if (pTxMic)
	{
		for (i = 0; i < 8; i++)
		{
			RTMP_IO_WRITE8(pAd, offset + i, pTxMic[i]);
		}
	}
	offset += LEN_TKIP_TXMICK;
		
	if (pRxMic)
	{
		for (i = 0; i < 8; i++)
		{
			RTMP_IO_WRITE8(pAd, offset + i, pRxMic[i]);
		}
	}
#endif // RT2860 //


	//
	// 4.) Modify IV/EIV if needs
	//     This will force Asic to use this key ID by setting IV.
	//
	if (bTxKey)
	{
#ifdef RT2860
		offset = MAC_IVEIV_TABLE_BASE + (WCID * HW_IVEIV_ENTRY_SIZE);
		//
		// Write IV
		//
		RTMP_IO_WRITE8(pAd, offset, pTxtsc[1]);
		RTMP_IO_WRITE8(pAd, offset + 1, ((pTxtsc[1] | 0x20) & 0x7f));
		RTMP_IO_WRITE8(pAd, offset + 2, pTxtsc[0]);

		IV4 = (KeyIdx << 6);
		if ((CipherAlg == CIPHER_TKIP) || (CipherAlg == CIPHER_TKIP_NO_MIC) ||(CipherAlg == CIPHER_AES))
			IV4 |= 0x20;  // turn on extension bit means EIV existence

		RTMP_IO_WRITE8(pAd, offset + 3, IV4);

		//
		// Write EIV
		//
		offset += 4;
		for (i = 0; i < 4; i++)
		{
			RTMP_IO_WRITE8(pAd, offset + i, pTxtsc[i + 2]);
		}
#endif // RT2860 //

#if 0
		//
		// Update WCID attribute.
		// Only TxKey could update WCID attribute.
		//
		offset = MAC_WCID_ATTRIBUTE_BASE + (WCID * HW_WCID_ATTRI_SIZE);
		WCIDAttri = (BssIndex << 4) | (CipherAlg << 1) | (bUsePairewiseKeyTable);
		RTMP_IO_WRITE32(pAd, offset, WCIDAttri);
#endif
		AsicUpdateWCIDAttribute(pAd, WCID, BssIndex, CipherAlg, bUsePairewiseKeyTable);
	}

	if (!bUsePairewiseKeyTable)
	{
		//
		// Only update the shared key security mode
		//
		RTMP_IO_READ32(pAd, SHARED_KEY_MODE_BASE + 4 * (BssIndex / 2), &csr1.word);
		if ((BssIndex % 2) == 0)
		{
			if (KeyIdx == 0)
				csr1.field.Bss0Key0CipherAlg = CipherAlg;
			else if (KeyIdx == 1)
				csr1.field.Bss0Key1CipherAlg = CipherAlg;
			else if (KeyIdx == 2)
				csr1.field.Bss0Key2CipherAlg = CipherAlg;
			else
				csr1.field.Bss0Key3CipherAlg = CipherAlg;
		}
		else
		{
			if (KeyIdx == 0)
				csr1.field.Bss1Key0CipherAlg = CipherAlg;
			else if (KeyIdx == 1)
				csr1.field.Bss1Key1CipherAlg = CipherAlg;
			else if (KeyIdx == 2)
				csr1.field.Bss1Key2CipherAlg = CipherAlg;
			else
				csr1.field.Bss1Key3CipherAlg = CipherAlg;
		}
		RTMP_IO_WRITE32(pAd, SHARED_KEY_MODE_BASE + 4 * (BssIndex / 2), csr1.word);			
	}

	DBGPRINT(RT_DEBUG_TRACE, ("<== AsicAddKeyEntry\n"));	
}


/*
	========================================================================
	Description:
		Add Pair-wise key material into ASIC. 
		Update pairwise key, TxMic and RxMic to Asic Pair-wise key table
				
    Return:
	========================================================================
*/
VOID AsicAddPairwiseKeyEntry(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR        pAddr,
	IN UCHAR		WCID,
	IN CIPHER_KEY		 *pCipherKey)
{
	INT i;
	ULONG 		offset;
	PUCHAR		 pKey = pCipherKey->Key;
	PUCHAR		 pTxMic = pCipherKey->TxMic;
	PUCHAR		 pRxMic = pCipherKey->RxMic;
#ifdef DBG    
	UCHAR		CipherAlg = pCipherKey->CipherAlg;
#endif // DBG //
	
	// EKEY
	offset = PAIRWISE_KEY_TABLE_BASE + (WCID * HW_KEY_ENTRY_SIZE);
#ifdef RT2860
	for (i=0; i<MAX_LEN_OF_PEER_KEY; i++)
	{
		RTMP_IO_WRITE8(pAd, offset + i, pKey[i]);
	}
#endif // RT2860 //
	for (i=0; i<MAX_LEN_OF_PEER_KEY; i+=4)
	{
		UINT32 Value;
		RTMP_IO_READ32(pAd, offset + i, &Value);
	}

	offset += MAX_LEN_OF_PEER_KEY;
	
	//  MIC KEY
	if (pTxMic)
	{
#ifdef RT2860
		for (i=0; i<8; i++)
		{
			RTMP_IO_WRITE8(pAd, offset+i, pTxMic[i]);
		}
#endif // RT2860 //
	}
	offset += 8;
	if (pRxMic)
	{
#ifdef RT2860
		for (i=0; i<8; i++)
		{
			RTMP_IO_WRITE8(pAd, offset+i, pRxMic[i]);
		}
#endif // RT2860 //
	}

	DBGPRINT(RT_DEBUG_TRACE,("AsicAddPairwiseKeyEntry: WCID #%d Alg=%s\n",WCID, CipherName[CipherAlg]));
	DBGPRINT(RT_DEBUG_TRACE,("	Key = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
		pKey[0],pKey[1],pKey[2],pKey[3],pKey[4],pKey[5],pKey[6],pKey[7],pKey[8],pKey[9],pKey[10],pKey[11],pKey[12],pKey[13],pKey[14],pKey[15]));
	if (pRxMic)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("	Rx MIC Key = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			pRxMic[0],pRxMic[1],pRxMic[2],pRxMic[3],pRxMic[4],pRxMic[5],pRxMic[6],pRxMic[7]));
	}
	if (pTxMic)
	{
		DBGPRINT(RT_DEBUG_TRACE, ("	Tx MIC Key = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			pTxMic[0],pTxMic[1],pTxMic[2],pTxMic[3],pTxMic[4],pTxMic[5],pTxMic[6],pTxMic[7]));
	}
}
/*
	========================================================================
	Description:
		Remove Pair-wise key material from ASIC. 

    Return:
	========================================================================
*/	
VOID AsicRemovePairwiseKeyEntry(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR		 BssIdx,
	IN UCHAR		 Wcid)
{
	ULONG		WCIDAttri;
	USHORT		offset;

	// re-set the entry's WCID attribute as OPEN-NONE.
	offset = MAC_WCID_ATTRIBUTE_BASE + (Wcid * HW_WCID_ATTRI_SIZE);
	WCIDAttri = (BssIdx<<4) | PAIRWISEKEYTABLE;		
	RTMP_IO_WRITE32(pAd, offset, WCIDAttri);

	DBGPRINT(RT_DEBUG_INFO, ("AsicRemovePairwiseKeyEntry: Wcid #%d \n", Wcid));
}

BOOLEAN AsicSendCommandToMcu(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR		 Command,
	IN UCHAR		 Token,
	IN UCHAR		 Arg0,
	IN UCHAR		 Arg1)
{
	HOST_CMD_CSR_STRUC	H2MCmd;
	H2M_MAILBOX_STRUC	H2MMailbox;
	ULONG				i = 0;
#ifdef RT2860
#ifdef RALINK_ATE
	static UINT32 j = 0;
#endif // RALINK_ATE //
#endif // RT2860 //
	do
	{
		RTMP_IO_READ32(pAd, H2M_MAILBOX_CSR, &H2MMailbox.word);
		if (H2MMailbox.field.Owner == 0)
			break;

		RTMPusecDelay(2);
		DBGPRINT(RT_DEBUG_INFO, ("AsicSendCommandToMcu::Mail box is busy\n"));
	} while(i++ < 100);

	if (i >= 100)
	{
#ifdef RT2860
#ifdef RALINK_ATE
		if (pAd->ate.bFWLoading == TRUE)
		{
			/* reloading firmware when received iwpriv cmd "ATE=ATESTOP" */
			if (j > 0)
			{
				if (j % 64 != 0)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("#"));
				}
				else
				{
					DBGPRINT(RT_DEBUG_ERROR, ("\n"));
				}
				++j;
			}
			else if (j == 0)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("Loading firmware. Please wait for a moment...\n"));
				++j;
			}
		}
		else
#endif // RALINK_ATE //
#endif // RT2860 //
		{
		DBGPRINT_ERR(("H2M_MAILBOX still hold by MCU. command fail\n"));
		}
		return FALSE;
	}

#ifdef RT2860
#ifdef RALINK_ATE
	else if (pAd->ate.bFWLoading == TRUE)
	{
		/* reloading of firmware is completed */
		pAd->ate.bFWLoading = FALSE;
		DBGPRINT(RT_DEBUG_ERROR, ("\n"));
		j = 0;
	}
#endif // RALINK_ATE //
#endif // RT2860 //

	H2MMailbox.field.Owner	  = 1;	   // pass ownership to MCU
	H2MMailbox.field.CmdToken = Token;
	H2MMailbox.field.HighByte = Arg1;
	H2MMailbox.field.LowByte  = Arg0;
	RTMP_IO_WRITE32(pAd, H2M_MAILBOX_CSR, H2MMailbox.word);

	H2MCmd.word 			  = 0;
	H2MCmd.field.HostCommand  = Command;
	RTMP_IO_WRITE32(pAd, HOST_CMD_CSR, H2MCmd.word);

	if (Command != 0x80)
	{
		DBGPRINT(RT_DEBUG_INFO, ("SW interrupt MCU (cmd=0x%02x, token=0x%02x, arg1,arg0=0x%02x,0x%02x)\n",
			H2MCmd.field.HostCommand, Token, Arg1, Arg0));
	}
	
	return TRUE;
}

#ifdef RT2860
BOOLEAN AsicCheckCommanOk(
	IN PRTMP_ADAPTER pAd,
	IN UCHAR		 Command)
{
	UINT32	CmdStatus = 0, CID = 0, i;
	UINT32	ThisCIDMask = 0;
	
	i = 0;
	do
	{
		RTMP_IO_READ32(pAd, H2M_MAILBOX_CID, &CID);
		if ((CID & CID0MASK) == Command)
		{
			ThisCIDMask = CID0MASK;
			break;
		}
		else if ((CID & CID1MASK) == Command)
		{
			ThisCIDMask = CID1MASK;
			break;
		}
		else if ((CID & CID2MASK) == Command)
		{
			ThisCIDMask = CID2MASK;
			break;
		}
		else if ((CID & CID3MASK) == Command)
		{
			ThisCIDMask = CID3MASK;
			break;
		}

		RTMPusecDelay(100);
		i++;
	}while (i < 100);

	RTMP_IO_READ32(pAd, H2M_MAILBOX_STATUS, &CmdStatus);
	if (i < 100)
	{
		if (((CmdStatus & ThisCIDMask) == 0x1) || ((CmdStatus & ThisCIDMask) == 0x100) 
			|| ((CmdStatus & ThisCIDMask) == 0x10000) || ((CmdStatus & ThisCIDMask) == 0x1000000))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("--> AsicCheckCommanOk CID = 0x%x, CmdStatus= 0x%x \n", CID, CmdStatus));
			RTMP_IO_WRITE32(pAd, H2M_MAILBOX_STATUS, 0xffffffff);
			RTMP_IO_WRITE32(pAd, H2M_MAILBOX_CID, 0xffffffff);
			return TRUE;
		}
		DBGPRINT(RT_DEBUG_TRACE, ("--> AsicCheckCommanFail1 CID = 0x%x, CmdStatus= 0x%x \n", CID, CmdStatus));
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("--> AsicCheckCommanFail2 Timeout Command = %d, CmdStatus= 0x%x \n", Command, CmdStatus));
	}
	RTMP_IO_WRITE32(pAd, H2M_MAILBOX_STATUS, 0xffffffff);
	RTMP_IO_WRITE32(pAd, H2M_MAILBOX_CID, 0xffffffff);
    
	return FALSE;
}
#endif // RT2860 //

/*
	========================================================================

	Routine Description:
		Verify the support rate for different PHY type

	Arguments:
		pAd 				Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	========================================================================
*/
VOID	RTMPCheckRates(
	IN		PRTMP_ADAPTER	pAd,
	IN OUT	UCHAR			SupRate[],
	IN OUT	UCHAR			*SupRateLen)
{
	UCHAR	RateIdx, i, j;
	UCHAR	NewRate[12], NewRateLen;
	
	NewRateLen = 0;
	
	if (pAd->CommonCfg.PhyMode == PHY_11B)
		RateIdx = 4;
//	else if ((pAd->CommonCfg.PhyMode == PHY_11BG_MIXED) && 
//		(pAd->StaCfg.BssType == BSS_ADHOC)			 &&
//		(pAd->StaCfg.AdhocMode == 0))
//		RateIdx = 4;
	else
		RateIdx = 12;

	// Check for support rates exclude basic rate bit	
	for (i = 0; i < *SupRateLen; i++)
		for (j = 0; j < RateIdx; j++)
			if ((SupRate[i] & 0x7f) == RateIdTo500Kbps[j])
				NewRate[NewRateLen++] = SupRate[i];
			
	*SupRateLen = NewRateLen;
	NdisMoveMemory(SupRate, NewRate, NewRateLen);
}



BOOLEAN		RTMPCheckAddHtInfoIe(
	IN		PRTMP_ADAPTER	pAd,
	IN OUT	ADD_HT_INFO_IE	*pAddHTInfo)
{
	INT		i; //, j;
	ADD_HTINFO		TempAddHTInfo;

	RTMPMoveMemory(&TempAddHTInfo, pAddHTInfo, SIZE_ADD_HT_INFO_IE);
	RTMPZeroMemory(pAddHTInfo, SIZE_ADD_HT_INFO_IE);

	// basic MCS that should be supported for this BSSID.
	for ( i=0 ; i< 16 ; i++)
	{
		if ((pAddHTInfo->MCSSet[i] & pAd->CommonCfg.HtCapability.MCSSet[i]) != pAddHTInfo->MCSSet[i] )
		return	FALSE;
	}

	// Check Controlled access for PSMP only. rt2860 not suppor PSMP yet.
	if (pAddHTInfo->AddHtInfo.CntlAccOnly == 1)
		return	FALSE;

	return	TRUE;
}

/*
	========================================================================

	Routine Description:
		Verify the support rate for different PHY type

	Arguments:
		pAd 				Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	========================================================================
*/
VOID RTMPUpdateMlmeRate(
	IN PRTMP_ADAPTER	pAd)
{
	UCHAR	MinimumRate;
	UCHAR	ProperMlmeRate = RATE_54;
	UCHAR	i, j, RateIdx = 12; //1, 2, 5.5, 11, 6, 9, 12, 18, 24, 36, 48, 54

	switch (pAd->CommonCfg.PhyMode) 
	{
		case PHY_11BG_MIXED:
		case PHY_11B:
		case PHY_11ABGN_MIXED:
		case PHY_11BGN_MIXED:
			MinimumRate = RATE_1;
			break;
		case PHY_11A:
		case PHY_11N_2_4G:	// rt2860 need to check mlmerate for 802.11n
		case PHY_11GN_MIXED:
		case PHY_11AGN_MIXED:
		case PHY_11N_5G:	
			MinimumRate = RATE_6;
			break;
		case PHY_11ABG_MIXED:
			if (pAd->CommonCfg.Channel <= 14)
			{
			   MinimumRate = RATE_1;
			}
			else
			{
				MinimumRate = RATE_6;
			}
			break;
		default: // error
			MinimumRate = RATE_1;
			break;
	}

	for (i = 0; i < pAd->MlmeAux.SupRateLen; i++)
	{
		for (j = 0; j < RateIdx; j++)
		{
			if ((pAd->MlmeAux.SupRate[i] & 0x7f) == RateIdTo500Kbps[j])
			{
				if (j < ProperMlmeRate)
					ProperMlmeRate = j;  //Zero base.
				else if (j == MinimumRate)
					ProperMlmeRate = MinimumRate;
			}			
		}

		if (ProperMlmeRate == MinimumRate)
			break;
	}

	for (i = 0; i < pAd->MlmeAux.ExtRateLen; i++)
	{
		for (j = 0; j < RateIdx; j++)
		{
			if ((pAd->MlmeAux.ExtRate[i] & 0x7f) == RateIdTo500Kbps[j])
			{
				if (j < (ProperMlmeRate))
					ProperMlmeRate = j;  //Zero base.
				else if (j == MinimumRate)
					ProperMlmeRate = MinimumRate;
			}				
		}
		
		if (ProperMlmeRate == MinimumRate)
			break;		
	}

	pAd->CommonCfg.MlmeRate = ProperMlmeRate;
	pAd->CommonCfg.RtsRate = ProperMlmeRate;
	if (pAd->CommonCfg.MlmeRate >= RATE_6)
	{
		pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
		pAd->CommonCfg.MlmeTransmit.field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MlmeRate];
		pAd->MacTab.Content[BSS0Mcast_WCID].HTPhyMode.field.MODE = MODE_OFDM;
		pAd->MacTab.Content[BSS0Mcast_WCID].HTPhyMode.field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MlmeRate];
	}
	else
	{
		pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_CCK;
		pAd->CommonCfg.MlmeTransmit.field.MCS = pAd->CommonCfg.MlmeRate;
		pAd->MacTab.Content[BSS0Mcast_WCID].HTPhyMode.field.MODE = MODE_CCK;
		pAd->MacTab.Content[BSS0Mcast_WCID].HTPhyMode.field.MCS = pAd->CommonCfg.MlmeRate;
	}
	DBGPRINT(RT_DEBUG_TRACE, ("RTMPUpdateMlmeRate ==>   MlmeTransmit = 0x%x  \n" , pAd->CommonCfg.MlmeTransmit.word));
}

CHAR RTMPMaxRssi(
	IN PRTMP_ADAPTER	pAd,
	IN CHAR				Rssi0,
	IN CHAR				Rssi1,
	IN CHAR				Rssi2)
{
	CHAR	larger = -127;
	
	if (pAd->Antenna.field.RxPath == 1)
	{
		larger = Rssi0;
	}

	if (pAd->Antenna.field.RxPath >= 2)
	{
		larger = max(Rssi0, Rssi1);
	}
	
	if (pAd->Antenna.field.RxPath == 3)
	{
		larger = max(larger, Rssi2);
	}

	return larger;
}

/*
    ========================================================================
    Routine Description:
        Periodic evaluate antenna link status
        
    Arguments:
        pAd         - Adapter pointer
        
    Return Value:
        None
        
    ========================================================================
*/
VOID AsicEvaluateRxAnt(
	IN PRTMP_ADAPTER	pAd)
{
	UCHAR	BBPR3 = 0;

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		APAsicEvaluateRxAnt(pAd);
		return;
	}
#endif // CONFIG_AP_SUPPORT //


	DBGPRINT(RT_DEBUG_INFO, ("AsicEvaluateRxAnt : RealRxPath=%d \n", pAd->Mlme.RealRxPath));
	
	RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R3, &BBPR3);
	BBPR3 &= (~0x18);
	if(pAd->Antenna.field.RxPath == 3)
	{
		BBPR3 |= (0x10);
	}
	else if(pAd->Antenna.field.RxPath == 2)
	{
		BBPR3 |= (0x8);
	}
	else if(pAd->Antenna.field.RxPath == 1)
	{
		BBPR3 |= (0x0);
	}
	RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R3, BBPR3);
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
	{
		ULONG	TxTotalCnt = pAd->RalinkCounters.OneSecTxNoRetryOkCount + 
								pAd->RalinkCounters.OneSecTxRetryOkCount + 
								pAd->RalinkCounters.OneSecTxFailCount;

		if (TxTotalCnt > 50)
		{
			RTMPSetTimer(&pAd->Mlme.RxAntEvalTimer, 20);
			pAd->Mlme.bLowThroughput = FALSE;
		}
		else
		{
			RTMPSetTimer(&pAd->Mlme.RxAntEvalTimer, 300);
			pAd->Mlme.bLowThroughput = TRUE;
		}
	}
}

/*
    ========================================================================
    Routine Description:
        After evaluation, check antenna link status
        
    Arguments:
        pAd         - Adapter pointer
        
    Return Value:
        None
        
    ========================================================================
*/
VOID AsicRxAntEvalTimeout(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3) 
{
	RTMP_ADAPTER	*pAd = (RTMP_ADAPTER *)FunctionContext;

#ifdef RALINK_ATE
	if (pAd->ate.Mode != ATE_STOP)
		return;
#endif // RALINK_ATE //

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		APAsicRxAntEvalTimeout(pAd);
		return;
	}
#endif // CONFIG_AP_SUPPORT //


}



VOID APSDPeriodicExec(
	IN PVOID SystemSpecific1, 
	IN PVOID FunctionContext, 
	IN PVOID SystemSpecific2, 
	IN PVOID SystemSpecific3) 
{
	RTMP_ADAPTER *pAd = (RTMP_ADAPTER *)FunctionContext;

	if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED))
		return;

	pAd->CommonCfg.TriggerTimerCount++;

// Driver should not send trigger frame, it should be send by application layer
/*
	if (pAd->CommonCfg.bAPSDCapable && pAd->CommonCfg.APEdcaParm.bAPSDCapable
		&& (pAd->CommonCfg.bNeedSendTriggerFrame ||
		(((pAd->CommonCfg.TriggerTimerCount%20) == 19) && (!pAd->CommonCfg.bAPSDAC_BE || !pAd->CommonCfg.bAPSDAC_BK || !pAd->CommonCfg.bAPSDAC_VI || !pAd->CommonCfg.bAPSDAC_VO))))
	{
		DBGPRINT(RT_DEBUG_TRACE,("Sending trigger frame and enter service period when support APSD\n"));
		RTMPSendNullFrame(pAd, pAd->CommonCfg.TxRate, TRUE);
		pAd->CommonCfg.bNeedSendTriggerFrame = FALSE;
		pAd->CommonCfg.TriggerTimerCount = 0;
		pAd->CommonCfg.bInServicePeriod = TRUE;
	}*/
}

/*
    ========================================================================
    Routine Description:
        Set/reset MAC registers according to bPiggyBack parameter
        
    Arguments:
        pAd         - Adapter pointer
        bPiggyBack  - Enable / Disable Piggy-Back

    Return Value:
        None
        
    ========================================================================
*/
VOID RTMPSetPiggyBack(
    IN PRTMP_ADAPTER    pAd,
    IN BOOLEAN          bPiggyBack)
{
	TX_LINK_CFG_STRUC  TxLinkCfg;
    
	RTMP_IO_READ32(pAd, TX_LINK_CFG, &TxLinkCfg.word);
//	printk("TX_LINK_CFG = %08x\n", TxLinkCfg.word);
	TxLinkCfg.field.TxCFAckEn = bPiggyBack;
	RTMP_IO_WRITE32(pAd, TX_LINK_CFG, TxLinkCfg.word);
}

/*
    ========================================================================
    Routine Description:
        check if this entry need to switch rate automatically
        
    Arguments:
        pAd         
        pEntry 	 	

    Return Value:
        TURE
        FALSE
        
    ========================================================================
*/
BOOLEAN RTMPCheckEntryEnableAutoRateSwitch(
	IN PRTMP_ADAPTER    pAd,
	IN PMAC_TABLE_ENTRY	pEntry)	
{
	BOOLEAN		result = TRUE;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	if (pEntry)
	{
		if (pEntry->ValidAsCLI)
			result = pAd->ApCfg.MBSSID[pEntry->apidx].bAutoTxRateSwitch;
#ifdef WDS_SUPPORT
		else if (pEntry->ValidAsWDS)
			result = pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].bAutoTxRateSwitch;
#endif // WDS_SUPPORT //
#ifdef APCLI_SUPPORT
		else if (pEntry->ValidAsApCli)
			result = pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx].bAutoTxRateSwitch;
#endif // APCLI_SUPPORT //
	}
#endif // CONFIG_AP_SUPPORT //


	return result;
}


BOOLEAN RTMPAutoRateSwitchCheck(
	IN PRTMP_ADAPTER    pAd)	
{			
#ifdef CONFIG_AP_SUPPORT		
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		INT	apidx = 0;
	
		for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
		{
			if (pAd->ApCfg.MBSSID[apidx].bAutoTxRateSwitch)
				return TRUE;
		}			
#ifdef WDS_SUPPORT
		for (apidx = 0; apidx < MAX_WDS_ENTRY; apidx++)
		{
			if (pAd->WdsTab.WdsEntry[apidx].bAutoTxRateSwitch)
				return TRUE;
		}		
#endif // WDS_SUPPORT //
#ifdef APCLI_SUPPORT
		for (apidx = 0; apidx < MAX_APCLI_NUM; apidx++)
		{
			if (pAd->ApCfg.ApCliTab[apidx].bAutoTxRateSwitch)
				return TRUE;
		}		
#endif // APCLI_SUPPORT //
	}
#endif // CONFIG_AP_SUPPORT //

	return FALSE;
}


/*
    ========================================================================
    Routine Description:
        check if this entry need to fix tx legacy rate
        
    Arguments:
        pAd         
        pEntry 	 	

    Return Value:
        TURE
        FALSE
        
    ========================================================================
*/
UCHAR RTMPStaFixedTxMode(
	IN PRTMP_ADAPTER    pAd,
	IN PMAC_TABLE_ENTRY	pEntry)	
{
	UCHAR	tx_mode = FIXED_TXMODE_HT;

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	if (pEntry)
	{
		if (pEntry->ValidAsCLI)
			tx_mode = (UCHAR)pAd->ApCfg.MBSSID[pEntry->apidx].DesiredTransmitSetting.field.FixedTxMode;
#ifdef WDS_SUPPORT
		else if (pEntry->ValidAsWDS)
			tx_mode = (UCHAR)pAd->WdsTab.WdsEntry[pEntry->MatchWDSTabIdx].DesiredTransmitSetting.field.FixedTxMode;
#endif // WDS_SUPPORT //
#ifdef APCLI_SUPPORT
		else if (pEntry->ValidAsApCli)
			tx_mode = (UCHAR)pAd->ApCfg.ApCliTab[pEntry->MatchAPCLITabIdx].DesiredTransmitSetting.field.FixedTxMode;
#endif // APCLI_SUPPORT //
	}
#endif // CONFIG_AP_SUPPORT //


	return tx_mode;
}

/*
    ========================================================================
    Routine Description:
        Overwrite HT Tx Mode by Fixed Legency Tx Mode, if specified.
        
    Arguments:
        pAd         
        pEntry 	 	

    Return Value:
        TURE
        FALSE
        
    ========================================================================
*/
VOID RTMPUpdateLegacyTxSetting(
		UCHAR				fixed_tx_mode,
		PMAC_TABLE_ENTRY	pEntry)
{
	HTTRANSMIT_SETTING TransmitSetting;
	
	if (fixed_tx_mode == FIXED_TXMODE_HT)
		return;
							 				
	TransmitSetting.word = 0;

	TransmitSetting.field.MODE = pEntry->HTPhyMode.field.MODE;
	TransmitSetting.field.MCS = pEntry->HTPhyMode.field.MCS;
						
	if (fixed_tx_mode == FIXED_TXMODE_CCK)
	{
		TransmitSetting.field.MODE = MODE_CCK;
		// CCK mode allow MCS 0~3
		if (TransmitSetting.field.MCS > MCS_3)
			TransmitSetting.field.MCS = MCS_3;
	}
	else 
	{
		TransmitSetting.field.MODE = MODE_OFDM;
		// OFDM mode allow MCS 0~7
		if (TransmitSetting.field.MCS > MCS_7)
			TransmitSetting.field.MCS = MCS_7;
	}
	
	if (pEntry->HTPhyMode.field.MODE >= TransmitSetting.field.MODE)
	{
		pEntry->HTPhyMode.word = TransmitSetting.word;
		DBGPRINT(RT_DEBUG_TRACE, ("RTMPUpdateLegacyTxSetting : wcid-%d, MODE=%s, MCS=%d \n", 
				pEntry->Aid, GetPhyMode(pEntry->HTPhyMode.field.MODE), pEntry->HTPhyMode.field.MCS));		
	}													
}

