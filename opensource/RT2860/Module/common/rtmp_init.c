/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	rtmp_init.c

	Abstract:
	Miniport generic portion header file

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
	Paul Lin    2002-08-01    created
    John Chang  2004-08-20    RT2561/2661 use scatter-gather scheme
    Jan Lee  2006-09-15    RT2860. Change for 802.11n , EEPROM, Led, BA, HT.
*/
#include	"rt_config.h"
#include 	"firmware.h"

//#define BIN_IN_FILE /* use *.bin firmware */

UCHAR    BIT8[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
ULONG    BIT32[] = {0x00000001, 0x00000002, 0x00000004, 0x00000008,
					0x00000010, 0x00000020, 0x00000040, 0x00000080,
					0x00000100, 0x00000200, 0x00000400, 0x00000800,
					0x00001000, 0x00002000, 0x00004000, 0x00008000,
					0x00010000, 0x00020000, 0x00040000, 0x00080000,
					0x00100000, 0x00200000, 0x00400000, 0x00800000,
					0x01000000, 0x02000000, 0x04000000, 0x08000000,
					0x10000000, 0x20000000, 0x40000000, 0x80000000};

char*   CipherName[] = {"none","wep64","wep128","TKIP","AES","CKIP64","CKIP128"};

const unsigned short ccitt_16Table[] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
#define ByteCRC16(v, crc) \
	(unsigned short)((crc << 8) ^  ccitt_16Table[((crc >> 8) ^ (v)) & 255])

unsigned char BitReverse(unsigned char x)
{
	int i;
	unsigned char Temp=0;
	for(i=0; ; i++)
	{
		if(x & 0x80)	Temp |= 0x80;
		if(i==7)		break;
		x	<<= 1;
		Temp >>= 1;
	}
	return Temp;
}


#ifdef WIN_NDIS
#define	STRING_OFFSET(field)			((UINT) FIELD_OFFSET(LOCAL_STRINGS, field))
#define	STRING_SIZE(field)				sizeof(((PLOCAL_STRINGS)0)->field)
#define INIT_NDIS_STR(str, buf, len)	{(str).Length = 0;(str).MaximumLength = len;(str).Buffer = buf;}

//
// Structure to first copy string values into
//
typedef	struct		_LOCAL_STRINGS {
	NDIS_STRING		Ssid;				// SSID network name
	WCHAR			SsidBuf[64];
	NDIS_STRING		VendorDesc;			// SSID network name
	WCHAR			VendorDescBuf[64];
	NDIS_STRING 	Key0;				// Key 0 in ascii format entered by user
	WCHAR			KeyBuf0[64];
	NDIS_STRING 	Key1;				// Key 1 in ascii format entered by user
	WCHAR			KeyBuf1[64];
	NDIS_STRING 	Key2;				// Key 2 in ascii format entered by user
	WCHAR			KeyBuf2[64];
	NDIS_STRING 	Key3;				// Key 3 in ascii format entered by user
	WCHAR			KeyBuf3[64];
	NDIS_STRING 	MAC;				// Local administration MAC Address
	WCHAR			LocalMacAddr[64];
}	LOCAL_STRINGS, *PLOCAL_STRINGS;

LOCAL_STRINGS	LocalStrings;

RTMP_REG_ENTRY	NICRegTable[] = {
//	reg value name						bRequired	Parameter Type			Offset in RTMP_ADAPTER			        Field size						    Default Value	Min			Max
	{NDIS_STRING_CONST("OpMode"),				0,	NdisParameterInteger,	RTMP_OFFSET(OpMode),					RTMP_SIZE(OpMode),						OPMODE_STA,	0,			1			},
	{NDIS_STRING_CONST("CountryRegion"),		0,	NdisParameterInteger,	COMMON_CFG_OFFSET(CountryRegion),		COMMON_CFG_SIZE(CountryRegion),			0,			0,			7			},
	{NDIS_STRING_CONST("CountryRegionABand"),	0,	NdisParameterInteger,	COMMON_CFG_OFFSET(CountryRegionForABand),COMMON_CFG_SIZE(CountryRegionForABand),7,			0,			8			},
	{NDIS_STRING_CONST("WirelessMode"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(PhyMode),				COMMON_CFG_SIZE(PhyMode),				0,			0,			9			},
	{NDIS_STRING_CONST("Channel"),				0,	NdisParameterInteger,	COMMON_CFG_OFFSET(Channel),				COMMON_CFG_SIZE(Channel),				1,			1,			255			},
	{NDIS_STRING_CONST("RtsThresh"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(RtsThreshold),		COMMON_CFG_SIZE(RtsThreshold),			2347,		0,			2347		},
	{NDIS_STRING_CONST("FragThresh"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(FragmentThreshold),	COMMON_CFG_SIZE(FragmentThreshold),		2346,		256,		2346		},
	{NDIS_STRING_CONST("Encryption"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(WepStatus),			COMMON_CFG_SIZE(WepStatus),				0,			0,			7			},
	{NDIS_STRING_CONST("AuthenType"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(AuthMode), 			COMMON_CFG_SIZE(AuthMode),				0,			0,			6			},
	{NDIS_STRING_CONST("WEPKeyUse"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(DefaultKeyId),		COMMON_CFG_SIZE(DefaultKeyId),			0,			1,			4			},
	{NDIS_STRING_CONST("RDG"),					0,	NdisParameterInteger,	COMMON_CFG_OFFSET(bRdg),				COMMON_CFG_SIZE(bRdg),					1,			0,			1			},
	{NDIS_STRING_CONST("SSID"),					0,	NdisParameterString,	STRING_OFFSET(Ssid),					STRING_SIZE(Ssid),						0,			0,			1			},
	{NDIS_STRING_CONST("VendorDesc"),			0,	NdisParameterString,	STRING_OFFSET(VendorDesc),				STRING_SIZE(VendorDesc),				0,			0,			1			},
	{NDIS_STRING_CONST("WEPKey1"),				0,	NdisParameterString,	STRING_OFFSET(Key0),					STRING_SIZE(Key0),						0,			0,			1			},
	{NDIS_STRING_CONST("WEPKey2"),				0,	NdisParameterString,	STRING_OFFSET(Key1),					STRING_SIZE(Key1),						0,			0,			1			},
	{NDIS_STRING_CONST("WEPKey3"),				0,	NdisParameterString,	STRING_OFFSET(Key2),					STRING_SIZE(Key2),						0,			0,			1			},
	{NDIS_STRING_CONST("WEPKey4"),				0,	NdisParameterString,	STRING_OFFSET(Key3),					STRING_SIZE(Key3),						0,			0,			1			},
	{NDIS_STRING_CONST("NetworkAddress"),		0,	NdisParameterString,	STRING_OFFSET(MAC),						STRING_SIZE(MAC),						0,			0,			1			},

	{NDIS_STRING_CONST("BGProtection"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(UseBGProtection),		COMMON_CFG_SIZE(UseBGProtection),		0,			0,			2			},
	{NDIS_STRING_CONST("ShortSlot"), 			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(bUseShortSlotTime),	COMMON_CFG_SIZE(bUseShortSlotTime),		0,			0,			1			},
	{NDIS_STRING_CONST("FrameAggregation"),		0,	NdisParameterInteger,	COMMON_CFG_OFFSET(bAggregationCapable), COMMON_CFG_SIZE(bAggregationCapable),	1,			0,			1			},
	{NDIS_STRING_CONST("IEEE80211H"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(bIEEE80211H),			COMMON_CFG_SIZE(bIEEE80211H),			0,			0,			1			},
	{NDIS_STRING_CONST("TXBurst"),				0,	NdisParameterInteger,	COMMON_CFG_OFFSET(bEnableTxBurst),		COMMON_CFG_SIZE(bEnableTxBurst),		0,			0,			1			},
	{NDIS_STRING_CONST("WMMCapable"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(bWmmCapable),			COMMON_CFG_SIZE(bWmmCapable),			0,			0,			1			},
	{NDIS_STRING_CONST("BACapability"),			0,	NdisParameterInteger,	COMMON_CFG_OFFSET(BACapability),		COMMON_CFG_SIZE(BACapability),			0x07022040,	2,			0xffffffff	},
	{NDIS_STRING_CONST("TransmitSetting"),		0,	NdisParameterInteger,	COMMON_CFG_OFFSET(RegTransmitSetting),	COMMON_CFG_SIZE(RegTransmitSetting),	0x8219,		2,			0xffffffff	},

	{NDIS_STRING_CONST("Radio"),				0,	NdisParameterInteger,	STA_CFG_OFFSET(bSwRadio),				STA_CFG_SIZE(bSwRadio),					0,			0,			1			},
	{NDIS_STRING_CONST("ShowHiddenSSID"),		0,	NdisParameterInteger,	STA_CFG_OFFSET(bShowHiddenSSID),		STA_CFG_SIZE(bShowHiddenSSID),			0,			0,			1			},
	{NDIS_STRING_CONST("AutoReconnect"),		0,	NdisParameterInteger,	STA_CFG_OFFSET(bAutoReconnect),			STA_CFG_SIZE(bAutoReconnect),			1,			0,			1			},
     	{NDIS_STRING_CONST("NetworkType"),			0,	NdisParameterInteger,	STA_CFG_OFFSET(BssType),				STA_CFG_SIZE(BssType),					0,			0,			1			},
     	{NDIS_STRING_CONST("AdhocOfdm"),			0,	NdisParameterInteger,	STA_CFG_OFFSET(AdhocMode),				STA_CFG_SIZE(AdhocMode), 				0,			0,			2			},
     	{NDIS_STRING_CONST("PSMode"), 				0,	NdisParameterInteger,	STA_CFG_OFFSET(WindowsPowerMode),		STA_CFG_SIZE(WindowsPowerMode),			0,			0,			2			},
     	{NDIS_STRING_CONST("CCX2Control"),			0,	NdisParameterInteger,	STA_CFG_OFFSET(CCX2Control),			STA_CFG_SIZE(CCX2Control),				0,			0,			0xffffffff	},
     	{NDIS_STRING_CONST("NHFactor"), 			0,	NdisParameterInteger,	STA_CFG_OFFSET(NHFactor),				STA_CFG_SIZE(NHFactor),					3,			1,			20			},
	{NDIS_STRING_CONST("CLFactor"), 			0,	NdisParameterInteger,	STA_CFG_OFFSET(CLFactor),				STA_CFG_SIZE(CLFactor),					9,			1,			20			},

	{NDIS_STRING_CONST("AutoChannelSelect"),	0,	NdisParameterInteger,	AP_CFG_OFFSET(bAutoChannelAtBootup),	AP_CFG_SIZE(bAutoChannelAtBootup),		0,			0,			1			},
	{NDIS_STRING_CONST("DtimPeriod"),			0,	NdisParameterInteger,	AP_CFG_OFFSET(DtimPeriod),				AP_CFG_SIZE(DtimPeriod),				1,			1,			5			},

	{NDIS_STRING_CONST("TEST"), 				0,	NdisParameterInteger,	STA_CFG_OFFSET(WhqlTest),			    STA_CFG_SIZE(WhqlTest), 			    0,			0,			1			},		
};

#define	NIC_NUM_REG_PARAMS	(sizeof (NICRegTable) / sizeof(RTMP_REG_ENTRY))
#endif

//
// BBP register initialization set
//
BBP_REG_PAIR   BBPRegTable[] = {
	{BBP_R65,		0x2C},		// fix rssi issue
	{BBP_R66,		0x38},	// Also set this default value to pAd->BbpTuning.R66CurrentValue at initial
#if 0
	{BBP_R69,		0x16},
	{BBP_R73,		0x12},
#else
	{BBP_R69,		0x12},
	{BBP_R70,		0xa},	// BBP_R70 will change to 0x8 in ApStartUp and LinkUp for rt2860C, otherwise value is 0xa
	{BBP_R73,		0x10},
#endif
	{BBP_R81,		0x37},
	{BBP_R82,		0x62},
	{BBP_R83,		0x6A},
	{BBP_R84,		0x99},	// 0x19 is for rt2860E and after. This is for extension channel overlapping IOT. 0x99 is for rt2860D and before
	{BBP_R86,		0x00},	// middle range issue, Rory @2008-01-28 	
	{BBP_R91,		0x04},	// middle range issue, Rory @2008-01-28
	{BBP_R92,		0x00},	// middle range issue, Rory @2008-01-28
	{BBP_R103,  	0x00},  // near range high-power issue, request from Gary @2008-05028
	{BBP_R105,		0x05},	// 0x05 is for rt2860E to turn on FEQ control. It is safe for rt2860D and before, because Bit 7:2 are reserved in rt2860D and before.
};
#define	NUM_BBP_REG_PARMS	(sizeof(BBPRegTable) / sizeof(BBP_REG_PAIR))

//
// ASIC register initialization sets
//

RTMP_REG_PAIR	MACRegTable[] =	{
#if defined(HW_BEACON_OFFSET) && (HW_BEACON_OFFSET == 0x200)
	{BCN_OFFSET0,			0xf8f0e8e0}, /* 0x3800(e0), 0x3A00(e8), 0x3C00(f0), 0x3E00(f8), 512B for each beacon */
	{BCN_OFFSET1,			0x6f77d0c8}, /* 0x3200(c8), 0x3400(d0), 0x1DC0(77), 0x1BC0(6f), 512B for each beacon */
#elif defined(HW_BEACON_OFFSET) && (HW_BEACON_OFFSET == 0x100)
	{BCN_OFFSET0,			0xece8e4e0}, /* 0x3800, 0x3A00, 0x3C00, 0x3E00, 512B for each beacon */
	{BCN_OFFSET1,			0xfcf8f4f0}, /* 0x3800, 0x3A00, 0x3C00, 0x3E00, 512B for each beacon */
#else
    #error You must re-calculate new value for BCN_OFFSET0 & BCN_OFFSET1 in MACRegTable[]!!!
#endif // HW_BEACON_OFFSET //

	{LEGACY_BASIC_RATE,		0x0000013f}, //  Basic rate set bitmap
	{HT_BASIC_RATE,		0x00008003}, // Basic HT rate set , 20M, MCS=3, MM. Format is the same as in TXWI.
	{MAC_SYS_CTRL,		0x00}, // 0x1004, , default Disable RX
	{RX_FILTR_CFG,		0x17f97}, //0x1400  , RX filter control,  
	{BKOFF_SLOT_CFG,	0x209}, // default set short slot time, CC_DELAY_TIME should be 2	 
	//{TX_SW_CFG0,		0x40a06}, // Gary,2006-08-23 
	{TX_SW_CFG0,		0x0}, 		// Gary,2008-05-21 for CWC test 
	{TX_SW_CFG1,		0x80606}, // Gary,2006-08-23 
	{TX_LINK_CFG,		0x1020},		// Gary,2006-08-23 
	//{TX_TIMEOUT_CFG,	0x00182090},	// CCK has some problem. So increase timieout value. 2006-10-09// MArvek RT
	{TX_TIMEOUT_CFG,	0x000a2090},	// CCK has some problem. So increase timieout value. 2006-10-09// MArvek RT , Modify for 2860E ,2007-08-01
	{MAX_LEN_CFG,		MAX_AGGREGATION_SIZE | 0x00001000},	// 0x3018, MAX frame length. Max PSDU = 16kbytes.
	{LED_CFG,		0x7f031e46}, // Gary, 2006-08-23
//#ifdef CONFIG_AP_SUPPORT
//	{WMM_AIFSN_CFG,		0x00001173},
//	{WMM_CWMIN_CFG,		0x00002344},
//	{WMM_CWMAX_CFG,		0x000034a6},
//	{WMM_TXOP0_CFG,		0x00100020},
//	{WMM_TXOP1_CFG,		0x002F0038},
//#endif // CONFIG_AP_SUPPORT //

	{PBF_MAX_PCNT,			0x1F3FBF9F}, 	//0x1F3f7f9f},		//Jan, 2006/04/20
	//{TX_RTY_CFG,			0x6bb80408},	// Jan, 2006/11/16
	{TX_RTY_CFG,			0x47d01f0f},	// Jan, 2006/11/16, Set TxWI->ACK =0 in Probe Rsp Modify for 2860E ,2007-08-03
	{AUTO_RSP_CFG,			0x00000013},	// Initial Auto_Responder, because QA will turn off Auto-Responder
	{CCK_PROT_CFG,			0x05740003 /*0x01740003*/},	// Initial Auto_Responder, because QA will turn off Auto-Responder. And RTS threshold is enabled. 
	{OFDM_PROT_CFG,			0x05740003 /*0x01740003*/},	// Initial Auto_Responder, because QA will turn off Auto-Responder. And RTS threshold is enabled. 
	{GF20_PROT_CFG,			0x01744004},    // set 19:18 --> Short NAV for MIMO PS
	{GF40_PROT_CFG,			0x03F44084},    
	{MM20_PROT_CFG,			0x01744004},    
#ifdef RT2860
	{MM40_PROT_CFG,			0x03F54084},	
#endif // RT2860 //
	{TXOP_CTRL_CFG,			0x0000583f, /*0x0000243f*/ /*0x000024bf*/},	//Extension channel backoff.
	{TX_RTS_CFG,			0x00092b20},	
//#ifdef WIFI_TEST
	{EXP_ACK_TIME,			0x002400ca},	// default value
//#else
//	{EXP_ACK_TIME,			0x005400ca},	// suggested by Gray @ 20070323 for 11n intel-sta throughput
//#endif // end - WIFI_TEST //
//#ifdef CONFIG_AP_SUPPORT
//	{TBTT_SYNC_CFG,			0x00422000},	// TBTT_ADJUST(7:0) == 0	
//	{TBTT_SYNC_CFG,			0x00012000},	// TBTT_ADJUST(7:0) == 0	
//#endif // CONFIG_AP_SUPPORT //
	{TXOP_HLDR_ET, 			0x00000002},

	/* Jerry comments 2008/01/16: we use SIFS = 10us in CCK defaultly, but it seems that 10us
		is too small for INTEL 2200bg card, so in MBSS mode, the delta time between beacon0
		and beacon1 is SIFS (10us), so if INTEL 2200bg card connects to BSS0, the ping
		will always lost. So we change the SIFS of CCK from 10us to 16us. */
	{XIFS_TIME_CFG,			0x33a41010},
	{PWR_PIN_CFG,			0x00000003},	// patch for 2880-E
};

#ifdef CONFIG_AP_SUPPORT
RTMP_REG_PAIR	APMACRegTable[] =	{
	{WMM_AIFSN_CFG,		0x00001173},
	{WMM_CWMIN_CFG,	0x00002344},
	{WMM_CWMAX_CFG,	0x000034a6},
	{WMM_TXOP0_CFG,		0x00100020},
	{WMM_TXOP1_CFG,		0x002F0038},
	{TBTT_SYNC_CFG,		0x00012000},
};
#endif // CONFIG_AP_SUPPORT //


#define	NUM_MAC_REG_PARMS		(sizeof(MACRegTable) / sizeof(RTMP_REG_PAIR))
#ifdef CONFIG_AP_SUPPORT
#define	NUM_AP_MAC_REG_PARMS		(sizeof(APMACRegTable) / sizeof(RTMP_REG_PAIR))
#endif // CONFIG_AP_SUPPORT //


#define FIRMWAREIMAGE_LENGTH		(sizeof (FirmwareImage) / sizeof(UCHAR))
#define FIRMWARE_MAJOR_VERSION	0

#ifdef RT2860
#define FIRMWARE_MINOR_VERSION	2
#endif // RT2860 //


/*
	========================================================================
	
	Routine Description:
		Allocate RTMP_ADAPTER data block and do some initialization

	Arguments:
		Adapter		Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS
		NDIS_STATUS_FAILURE

	IRQL = PASSIVE_LEVEL

	Note:
	
	========================================================================
*/
NDIS_STATUS	RTMPAllocAdapterBlock(
	IN  PVOID	handle,
	OUT	PRTMP_ADAPTER	*ppAdapter)
{
	PRTMP_ADAPTER	pAd;
	NDIS_STATUS		Status;
	INT 			index;
	UCHAR			*pBeaconBuf = NULL;

	DBGPRINT(RT_DEBUG_TRACE, ("--> RTMPAllocAdapterBlock\n"));

	*ppAdapter = NULL;

	do
	{
		// Allocate RTMP_ADAPTER memory block
#ifdef	WIN_NDIS
		Status = NdisAllocateMemoryWithTag((PVOID *)&pAd, sizeof(RTMP_ADAPTER), NIC_TAG);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGPRINT_ERR(("Failed to allocate memory - ADAPTER\n"));
			break;
		}

		// Clean up the memory block
		NdisZeroMemory(pAd, sizeof(RTMP_ADAPTER));

		RTMP_INC_REF(pAd);

		// Init Event
		NdisInitializeEvent(&pAd->ExitEvent);
		NdisInitializeEvent(&pAd->AllPacketsReturnedEvent);
		RTMP_INC_RCV_REF(pAd);
#else
		pBeaconBuf = kmalloc(MAX_BEACON_SIZE, MEM_ALLOC_FLAG);
		if (pBeaconBuf == NULL)
		{
			Status = NDIS_STATUS_FAILURE;
			DBGPRINT_ERR(("Failed to allocate memory - BeaconBuf!\n"));
			break;
		}

		Status = AdapterBlockAllocateMemory(handle, (PVOID *)&pAd);
		if (Status != NDIS_STATUS_SUCCESS)
		{
			DBGPRINT_ERR(("Failed to allocate memory - ADAPTER\n"));
			break;
		}
		pAd->BeaconBuf = pBeaconBuf;
		printk("\n\n=== pAd = %p, size = %d ===\n\n", pAd, (UINT32)sizeof(RTMP_ADAPTER));

		
		// Clean up the memory block
		// Dennis Lee Move into AdapterBlockAllocateMemory
		// NdisZeroMemory(pAd, sizeof(RTMP_ADAPTER));

//		RTMPAllocAdapterBlockHook();
#endif


		// Init spin locks
		NdisAllocateSpinLock(&pAd->MgmtRingLock);
#ifdef RT2860
#ifdef WIN_NDIS
		NdisAllocateSpinLock(&pAd->TxRingLock);
		NdisAllocateSpinLock(&pAd->LocalTxBufQueueLock);
#endif
		NdisAllocateSpinLock(&pAd->RxRingLock);
#endif // RT2860 //

		for (index =0 ; index < NUM_OF_TX_RING; index++)
		{
			NdisAllocateSpinLock(&pAd->TxSwQueueLock[index]);
			NdisAllocateSpinLock(&pAd->DeQueueLock[index]);
			pAd->DeQueueRunning[index] = FALSE;
		}

		NdisAllocateSpinLock(&pAd->irq_lock);

#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
        UAPSD_Init(pAd);
#endif // UAPSD_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
	} while (FALSE);

	if ((Status != NDIS_STATUS_SUCCESS) && (pBeaconBuf))
		kfree(pBeaconBuf);
	
	*ppAdapter = pAd;

	DBGPRINT_S(Status, ("<-- RTMPAllocAdapterBlock, Status=%x\n", Status));
	return Status;
}

#ifdef RT2860
#ifdef WIN_NDIS
/*
	========================================================================
	
	Routine Description:
		Find the adapter and get all the assigned resources

	Arguments:
		Adapter		Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS
		NDIS_STATUS_ADAPTER_NOT_FOUND (event is logged as well)

	IRQL = PASSIVE_LEVEL

	Note:
	
	========================================================================
*/
NDIS_STATUS	RTMPFindAdapter(
	IN	PRTMP_ADAPTER	pAd,
	IN	NDIS_HANDLE		WrapperConfigurationContext
	)
{
	NDIS_STATUS			Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
	ULONG				ErrorCode;
	ULONG				ErrorValue;

	ULONG				ulResult;
	UCHAR				buffer[NIC_PCI_HDR_LENGTH ];
	PPCI_COMMON_CONFIG	pPciConfig = (PPCI_COMMON_CONFIG) buffer;

	UCHAR				resBuf[NIC_RESOURCE_BUF_SIZE];
	PNDIS_RESOURCE_LIST	resList = (PNDIS_RESOURCE_LIST)resBuf;
	UINT				bufSize = NIC_RESOURCE_BUF_SIZE;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR	pResDesc;
	ULONG				index;
	BOOLEAN				bResPort = FALSE, bResInterrupt = FALSE, bResMemory = FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("---> RTMPFindAdapter\n"));

	do
	{
		//
		// Find our adapter - read in the device and vendor IDs
		//
		ulResult = NdisReadPciSlotInformation(
						pAd->AdapterHandle,
						0,          				// ignored
						FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
						buffer,
						NIC_PCI_HDR_LENGTH );

		if (ulResult != NIC_PCI_HDR_LENGTH )
		{
			DBGPRINT(RT_DEBUG_ERROR,
				("NdisReadPciSlotInformation (PCI_COMMON_CONFIG) ulResult=%d\n", ulResult));

			ErrorCode  = NDIS_ERROR_CODE_ADAPTER_NOT_FOUND;
			ErrorValue = ERRLOG_READ_PCI_SLOT_FAILED;

			break;
		}

		//
		// Right type of adapter?
		//
		if ((pPciConfig->VendorID != NIC_PCI_VENDOR_ID) ||
			((pPciConfig->DeviceID != NIC2860_PCI_DEVICE_ID) &&
			(pPciConfig->DeviceID != NIC2860_PCIe_DEVICE_ID)))
		{
			DBGPRINT(RT_DEBUG_ERROR, ("VendorID or DeviceID doesn't match - %x/%x\n",
				pPciConfig->VendorID, pPciConfig->DeviceID));

			ErrorCode  = NDIS_ERROR_CODE_ADAPTER_NOT_FOUND;
			ErrorValue = ERRLOG_VENDOR_DEVICE_NOMATCH;

			break;
		}

		// save info from config space
		pAd->RevsionID   = pPciConfig->RevisionID;
		pAd->VendorID    = pPciConfig->VendorID;
		pAd->DeviceID    = pPciConfig->DeviceID;
		pAd->SubVendorID = pPciConfig->u.type0.SubVendorID;
		pAd->SubSystemID = pPciConfig->u.type0.SubSystemID;

		DBGPRINT(RT_DEBUG_TRACE, ("Adapter found - VID=0x%04x, DevID=0x%04x, subVID=0x%04x, subSysID=0x%04x\n",
			pAd->VendorID, pAd->DeviceID, pAd->SubVendorID, pAd->SubSystemID));

		//
		// Adapter is found. Now get the assigned resources
		//
		NdisMQueryAdapterResources(
			&Status,
			WrapperConfigurationContext,
			resList,
			&bufSize);

		if (Status != NDIS_STATUS_SUCCESS)
		{
			ErrorCode = NDIS_ERROR_CODE_RESOURCE_CONFLICT;
			ErrorValue = ERRLOG_QUERY_ADAPTER_RESOURCES;
			break;
		}

		for (index=0; index < resList->Count; index++)
		{
			pResDesc = &resList->PartialDescriptors[index];

			switch(pResDesc->Type)
			{
				case CmResourceTypePort:		// ResType_IO
					pAd->IoBaseAddress = NdisGetPhysicalAddressLow(pResDesc->u.Port.Start);
					pAd->IoRange = pResDesc->u.Port.Length;
					bResPort = TRUE;

					DBGPRINT(RT_DEBUG_TRACE, ("IoBaseAddress = 0x%x\n", pAd->IoBaseAddress));
					DBGPRINT(RT_DEBUG_TRACE, ("IoRange = 0x%x\n", pAd->IoRange));
					break;

				case CmResourceTypeInterrupt:	// ResType_IRQ
					pAd->InterruptLevel = pResDesc->u.Interrupt.Level;
					bResInterrupt = TRUE;

					DBGPRINT(RT_DEBUG_TRACE, ("InterruptLevel = 0x%x\n", pAd->InterruptLevel));
					break;

				case CmResourceTypeMemory:		// ResType_Mem
					pAd->MemPhysAddress = pResDesc->u.Memory.Start;
					pAd->MemRange = pResDesc->u.Memory.Length;
					bResMemory = TRUE;

					DBGPRINT(RT_DEBUG_TRACE,
						("MemPhysAddress(Low) = 0x%0x\n", NdisGetPhysicalAddressLow(pAd->MemPhysAddress)));
					DBGPRINT(RT_DEBUG_TRACE,
						("MemPhysAddress(High) = 0x%0x\n", NdisGetPhysicalAddressHigh(pAd->MemPhysAddress)));
					DBGPRINT(RT_DEBUG_TRACE, ("MemRange = 0x%x\n", pAd->MemRange));
					break;
			}
		}

		if (!bResInterrupt || !bResMemory)
		{
			Status = NDIS_STATUS_RESOURCE_CONFLICT;
			ErrorCode = NDIS_ERROR_CODE_RESOURCE_CONFLICT;

			if (!bResInterrupt)
			{
				ErrorValue = ERRLOG_NO_INTERRUPT_RESOURCE;
			}
			else
			{
				ErrorValue = ERRLOG_NO_MEMORY_RESOURCE;
			}

			break;
		}

		Status = NDIS_STATUS_SUCCESS;
	}	while (FALSE);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		// Log error information
		NdisWriteErrorLogEntry(
			pAd->AdapterHandle,
			ErrorCode,
			1,
			ErrorValue);
	}

	DBGPRINT_S(Status, ("<--- RTMPFindAdapter, Status=%x\n", Status));
	return Status;
}

/*
	========================================================================
	
	Routine Description:
		Free all the resources and RTMP_ADAPTER data block

	Arguments:
		Adapter			Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:

	========================================================================
*/
VOID	RTMPFreeAdapter(
	IN	PRTMP_ADAPTER	pAd)
{
	INT			index, num;

	DBGPRINT(RT_DEBUG_TRACE, ("--> RTMPFreeAdapter\n"));

	//
	// No active and waiting sends
	// Send & Recv already checked in RTMPHalt routine
	//

	// Make sure no other pending operations
	ASSERT(RTMP_GET_REF(pAd) == 0);

	//
	// Free hardware resources
	//

	// Free Memory mapping PCI register & shared memory
	if (pAd->CSRBaseAddress)
	{
		NdisMUnmapIoSpace(
			pAd->AdapterHandle,
			pAd->CSRBaseAddress,
			pAd->MemRange);
		pAd->CSRBaseAddress = NULL;
	}

	// Free IO port resource
	if (pAd->PortOffset)
	{
		NdisMDeregisterIoPortRange(
			pAd->AdapterHandle,
			pAd->IoBaseAddress,
			pAd->IoRange,
			pAd->PortOffset);
		pAd->PortOffset = NULL;
	}

	//
	// Free common buffer share memory
	// Everything in reverse order
	//

	//
	// Free RX Ring related space
	//
	for (index = RX_RING_SIZE - 1 ; index >= 0; index--)
	{
		if (pAd->RxRing.Cell[index].DmaBuf.AllocVa)
		{
			NdisMFreeSharedMemory(
				pAd->AdapterHandle,
				pAd->RxRing.Cell[index].DmaBuf.AllocSize,
				FALSE,
				pAd->RxRing.Cell[index].DmaBuf.AllocVa,
				pAd->RxRing.Cell[index].DmaBuf.AllocPa);
		}
	}
	NdisZeroMemory(pAd->RxRing.Cell, RX_RING_SIZE * sizeof(RTMP_DMACB));

	if (pAd->RxDescRing.AllocVa)
	{
		NdisMFreeSharedMemory(
			pAd->AdapterHandle,
			pAd->RxDescRing.AllocSize,
			FALSE,
			pAd->RxDescRing.AllocVa,
			pAd->RxDescRing.AllocPa);
	}
	NdisZeroMemory(&pAd->RxDescRing, sizeof(RTMP_DMABUF));

	//
	// Free MGMT Ring related space
	//
	if (pAd->MgmtDescRing.AllocVa)
	{
		NdisMFreeSharedMemory(
			pAd->AdapterHandle,
			pAd->MgmtDescRing.AllocSize,
			FALSE,
			pAd->MgmtDescRing.AllocVa,
			pAd->MgmtDescRing.AllocPa);
	}
	NdisZeroMemory(&pAd->MgmtDescRing, sizeof(RTMP_DMABUF));

	//
	// Free TX Ring buffer
	//
	for (num = 0; num < NUM_OF_TX_RING; num++)
	{
		if (pAd->TxBufSpace[num].AllocVa)
		{
			NdisMFreeSharedMemory(
				pAd->AdapterHandle,
				pAd->TxBufSpace[num].AllocSize,
				FALSE,
				pAd->TxBufSpace[num].AllocVa,
				pAd->TxBufSpace[num].AllocPa);
		}
		NdisZeroMemory(&pAd->TxBufSpace[num], sizeof(RTMP_DMABUF));
		
		if (pAd->TxDescRing[num].AllocVa)
		{
			NdisMFreeSharedMemory(
				pAd->AdapterHandle,
				pAd->TxDescRing[num].AllocSize,
				FALSE,
				pAd->TxDescRing[num].AllocVa,
				pAd->TxDescRing[num].AllocPa);
		}
		NdisZeroMemory(&pAd->TxDescRing[num], sizeof(RTMP_DMABUF));
	}

	//
	// Free preallocated shared memory
	//
	for (index = 0; index < NUM_OF_LOCAL_TXBUF; index++)
	{
		if (! pAd->LocalTxBuf[index].AllocVa)
			break;
		NdisMFreeSharedMemory(
			pAd->AdapterHandle,
			pAd->LocalTxBuf[index].AllocSize,
			TRUE,
			pAd->LocalTxBuf[index].AllocVa,
			pAd->LocalTxBuf[index].AllocPa);
	}


		//
		// Free BA Rx reordering Ring and associated buffer memory
		//
	
	for (num = 0; num < MAX_BARECI_SESSION; num++)
		{
		pAd->LocalRxReorderBuf[num].InUse = FALSE;
		for (index = 0; index < MAX_RX_REORDERBUF; index++)
				{
			pDmaBuf = &pAd->LocalRxReorderBuf[num].MAP_RXBuf[index];
			pDmaBuf->IsFull = FALSE;
				if (pDmaBuf->AllocVa)
				{
					NdisMFreeSharedMemory(
						pAd->AdapterHandle,
						pDmaBuf->AllocSize,
						FALSE,
						pDmaBuf->AllocVa,
						pDmaBuf->AllocPa);
				}
			}
		}
	
#ifndef UNDER_CE
	//
	// Free map registers. This must be after all the shared memory is freed
	//
	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_MAP_REGISTER))
	{
		NdisMFreeMapRegisters(pAd->AdapterHandle);
		RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_MAP_REGISTER);
	}
#endif

	for (num = 0; num < NUM_OF_TX_RING; num++)
		NdisFreeSpinLock(&pAd->TxSwQueueLock[num]);

	NdisFreeSpinLock(&pAd->RxRingLock);
	NdisFreeSpinLock(&pAd->MgmtRingLock);
#ifdef WIN_NDIS
	NdisFreeSpinLock(&pAd->TxRingLock);
	NdisFreeSpinLock(&pAd->LocalTxBufQueueLock);
#endif
	NdisFreeSpinLock(&pAd->irq_lock);
#if ME_98	
	NdisFreeSpinLock(&pAd->DispatchLock);
#endif

#ifdef CONFIG_AP_SUPPORT
#ifdef UAPSD_AP_SUPPORT
    UAPSD_Release(pAd);
#endif // UAPSD_AP_SUPPORT //
#endif // CONFIG_AP_SUPPORT //

	// free the pre-allocated NDI PACKET/BUFFER poll
	if (pAd->FreeNdisPacketPoolHandle)
		NdisFreePacketPool(pAd->FreeNdisPacketPoolHandle);

	if (pAd->FreeNdisBufferPoolHandle)
		NdisFreeBufferPool(pAd->FreeNdisBufferPoolHandle);

	NdisFreeMemory(pAd->BeaconBuf, MAX_BEACON_SIZE);
	
	NdisFreeMemory(pAd, sizeof(RTMP_ADAPTER), 0);

	DBGPRINT(RT_DEBUG_TRACE, ("<-- RTMPFreeAdapter\n"));
}
#endif
#endif // RT2860 //

/*
	========================================================================
	
	Routine Description:
		Read initial Tx power per MCS and BW from EEPROM
		
	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		
	========================================================================
*/
VOID	RTMPReadTxPwrPerRate(
	IN	PRTMP_ADAPTER	pAd)
{
	ULONG		data, Adata, Gdata;
	USHORT		i, value, value2;
	INT			Apwrdelta, Gpwrdelta;
	UCHAR		t1,t2,t3,t4;
	BOOLEAN		bValid, bApwrdeltaMinus = TRUE, bGpwrdeltaMinus = TRUE;
	
	//
	// Get power delta for 20MHz and 40MHz.
	//
	DBGPRINT(RT_DEBUG_TRACE, ("Txpower per Rate\n"));
	value2 = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_DELTA);
	Apwrdelta = 0;
	Gpwrdelta = 0;

	if ((value2 & 0xff) != 0xff)
	{
		if ((value2 & 0x80))
			Gpwrdelta = (value2&0xf);
		
		if ((value2 & 0x40))
			bGpwrdeltaMinus = FALSE;
		else
			bGpwrdeltaMinus = TRUE;
	}
	if ((value2 & 0xff00) != 0xff00)
	{
		if ((value2 & 0x8000))
			Apwrdelta = ((value2&0xf00)>>8);

		if ((value2 & 0x4000))
			bApwrdeltaMinus = FALSE;
		else
			bApwrdeltaMinus = TRUE;
	}	
	DBGPRINT(RT_DEBUG_TRACE, ("Gpwrdelta = %x, Apwrdelta = %x .\n", Gpwrdelta, Apwrdelta));

	//
	// Get Txpower per MCS for 20MHz in 2.4G.
	//
	for (i=0; i<5; i++)
	{
		value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_20MHZ_2_4G + i*4);
		data = value;
		if (bApwrdeltaMinus == FALSE)
		{
			t1 = (value&0xf)+(Apwrdelta);
			if (t1 > 0xf)
				t1 = 0xf;
			t2 = ((value&0xf0)>>4)+(Apwrdelta);
			if (t2 > 0xf)
				t2 = 0xf;
			t3 = ((value&0xf00)>>8)+(Apwrdelta);
			if (t3 > 0xf)
				t3 = 0xf;
			t4 = ((value&0xf000)>>12)+(Apwrdelta);
			if (t4 > 0xf)
				t4 = 0xf;
		}
		else
		{
			if ((value&0xf) > Apwrdelta)
				t1 = (value&0xf)-(Apwrdelta);
			else
				t1 = 0;
			if (((value&0xf0)>>4) > Apwrdelta)
				t2 = ((value&0xf0)>>4)-(Apwrdelta);
			else
				t2 = 0;
			if (((value&0xf00)>>8) > Apwrdelta)
				t3 = ((value&0xf00)>>8)-(Apwrdelta);
			else
				t3 = 0;
			if (((value&0xf000)>>12) > Apwrdelta)
				t4 = ((value&0xf000)>>12)-(Apwrdelta);
			else
				t4 = 0;
		}				
		Adata = t1 + (t2<<4) + (t3<<8) + (t4<<12);
		if (bGpwrdeltaMinus == FALSE)
		{
			t1 = (value&0xf)+(Gpwrdelta);
			if (t1 > 0xf)
				t1 = 0xf;
			t2 = ((value&0xf0)>>4)+(Gpwrdelta);
			if (t2 > 0xf)
				t2 = 0xf;
			t3 = ((value&0xf00)>>8)+(Gpwrdelta);
			if (t3 > 0xf)
				t3 = 0xf;
			t4 = ((value&0xf000)>>12)+(Gpwrdelta);
			if (t4 > 0xf)
				t4 = 0xf;
		}
		else
		{
			if ((value&0xf) > Gpwrdelta)
				t1 = (value&0xf)-(Gpwrdelta);
			else
				t1 = 0;
			if (((value&0xf0)>>4) > Gpwrdelta)
				t2 = ((value&0xf0)>>4)-(Gpwrdelta);
			else
				t2 = 0;
			if (((value&0xf00)>>8) > Gpwrdelta)
				t3 = ((value&0xf00)>>8)-(Gpwrdelta);
			else
				t3 = 0;
			if (((value&0xf000)>>12) > Gpwrdelta)
				t4 = ((value&0xf000)>>12)-(Gpwrdelta);
			else
				t4 = 0;
		}				
		Gdata = t1 + (t2<<4) + (t3<<8) + (t4<<12);
		
		value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_20MHZ_2_4G + i*4 + 2);
		if (bApwrdeltaMinus == FALSE)
		{
			t1 = (value&0xf)+(Apwrdelta);
			if (t1 > 0xf)
				t1 = 0xf;
			t2 = ((value&0xf0)>>4)+(Apwrdelta);
			if (t2 > 0xf)
				t2 = 0xf;
			t3 = ((value&0xf00)>>8)+(Apwrdelta);
			if (t3 > 0xf)
				t3 = 0xf;
			t4 = ((value&0xf000)>>12)+(Apwrdelta);
			if (t4 > 0xf)
				t4 = 0xf;
		}
		else
		{
			if ((value&0xf) > Apwrdelta)
				t1 = (value&0xf)-(Apwrdelta);
			else
				t1 = 0;
			if (((value&0xf0)>>4) > Apwrdelta)
				t2 = ((value&0xf0)>>4)-(Apwrdelta);
			else
				t2 = 0;
			if (((value&0xf00)>>8) > Apwrdelta)
				t3 = ((value&0xf00)>>8)-(Apwrdelta);
			else
				t3 = 0;
			if (((value&0xf000)>>12) > Apwrdelta)
				t4 = ((value&0xf000)>>12)-(Apwrdelta);
			else
				t4 = 0;
		}				
		Adata |= ((t1<<16) + (t2<<20) + (t3<<24) + (t4<<28));
		if (bGpwrdeltaMinus == FALSE)
		{
			t1 = (value&0xf)+(Gpwrdelta);
			if (t1 > 0xf)
				t1 = 0xf;
			t2 = ((value&0xf0)>>4)+(Gpwrdelta);
			if (t2 > 0xf)
				t2 = 0xf;
			t3 = ((value&0xf00)>>8)+(Gpwrdelta);
			if (t3 > 0xf)
				t3 = 0xf;
			t4 = ((value&0xf000)>>12)+(Gpwrdelta);
			if (t4 > 0xf)
				t4 = 0xf;
		}
		else
		{
			if ((value&0xf) > Gpwrdelta)
				t1 = (value&0xf)-(Gpwrdelta);
			else
				t1 = 0;
			if (((value&0xf0)>>4) > Gpwrdelta)
				t2 = ((value&0xf0)>>4)-(Gpwrdelta);
			else
				t2 = 0;
			if (((value&0xf00)>>8) > Gpwrdelta)
				t3 = ((value&0xf00)>>8)-(Gpwrdelta);
			else
				t3 = 0;
			if (((value&0xf000)>>12) > Gpwrdelta)
				t4 = ((value&0xf000)>>12)-(Gpwrdelta);
			else
				t4 = 0;
		}				
		Gdata |= ((t1<<16) + (t2<<20) + (t3<<24) + (t4<<28));
		data |= (value<<16);
		
		pAd->Tx20MPwrCfgABand[i] = pAd->Tx40MPwrCfgABand[i] = Adata;
		pAd->Tx20MPwrCfgGBand[i] = pAd->Tx40MPwrCfgGBand[i] = Gdata;
		
		if (data != 0xffffffff)
			RTMP_IO_WRITE32(pAd, TX_PWR_CFG_0 + i*4, data);
		DBGPRINT_RAW(RT_DEBUG_TRACE, ("20MHz BW, 2.4G band-%lx,  Adata = %lx,  Gdata = %lx \n", data, Adata, Gdata));
	}

	//
	// Check this block is valid for 40MHz in 2.4G. If invalid, use parameter for 20MHz in 2.4G
	//
	bValid = TRUE;
	for (i=0; i<6; i++)
	{
		value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_40MHZ_2_4G + 2 + i*2);
		if (((value & 0x00FF) == 0x00FF) || ((value & 0xFF00) == 0xFF00))
		{
			bValid = FALSE;
			break;
		}
	}

	//
	// Get Txpower per MCS for 40MHz in 2.4G.
	//
	if (bValid)
	{
		for (i=0; i<4; i++)
		{
			value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_40MHZ_2_4G + i*4);
			if (bGpwrdeltaMinus == FALSE)
			{
				t1 = (value&0xf)+(Gpwrdelta);
				if (t1 > 0xf)
					t1 = 0xf;
				t2 = ((value&0xf0)>>4)+(Gpwrdelta);
				if (t2 > 0xf)
					t2 = 0xf;
				t3 = ((value&0xf00)>>8)+(Gpwrdelta);
				if (t3 > 0xf)
					t3 = 0xf;
				t4 = ((value&0xf000)>>12)+(Gpwrdelta);
				if (t4 > 0xf)
					t4 = 0xf;
			}
			else
			{
				if ((value&0xf) > Gpwrdelta)
					t1 = (value&0xf)-(Gpwrdelta);
				else
					t1 = 0;
				if (((value&0xf0)>>4) > Gpwrdelta)
					t2 = ((value&0xf0)>>4)-(Gpwrdelta);
				else
					t2 = 0;
				if (((value&0xf00)>>8) > Gpwrdelta)
					t3 = ((value&0xf00)>>8)-(Gpwrdelta);
				else
					t3 = 0;
				if (((value&0xf000)>>12) > Gpwrdelta)
					t4 = ((value&0xf000)>>12)-(Gpwrdelta);
				else
					t4 = 0;
			}				
			Gdata = t1 + (t2<<4) + (t3<<8) + (t4<<12);

			value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_40MHZ_2_4G + i*4 + 2);
			if (bGpwrdeltaMinus == FALSE)
			{
				t1 = (value&0xf)+(Gpwrdelta);
				if (t1 > 0xf)
					t1 = 0xf;
				t2 = ((value&0xf0)>>4)+(Gpwrdelta);
				if (t2 > 0xf)
					t2 = 0xf;
				t3 = ((value&0xf00)>>8)+(Gpwrdelta);
				if (t3 > 0xf)
					t3 = 0xf;
				t4 = ((value&0xf000)>>12)+(Gpwrdelta);
				if (t4 > 0xf)
					t4 = 0xf;
			}
			else
			{
				if ((value&0xf) > Gpwrdelta)
					t1 = (value&0xf)-(Gpwrdelta);
				else
					t1 = 0;
				if (((value&0xf0)>>4) > Gpwrdelta)
					t2 = ((value&0xf0)>>4)-(Gpwrdelta);
				else
					t2 = 0;
				if (((value&0xf00)>>8) > Gpwrdelta)
					t3 = ((value&0xf00)>>8)-(Gpwrdelta);
				else
					t3 = 0;
				if (((value&0xf000)>>12) > Gpwrdelta)
					t4 = ((value&0xf000)>>12)-(Gpwrdelta);
				else
					t4 = 0;
			}				
			Gdata |= ((t1<<16) + (t2<<20) + (t3<<24) + (t4<<28));

			if (i == 0)
				pAd->Tx40MPwrCfgGBand[i+1] = (pAd->Tx40MPwrCfgGBand[i+1] & 0x0000FFFF) | (Gdata & 0xFFFF0000);
			else
				pAd->Tx40MPwrCfgGBand[i+1] = Gdata;

			DBGPRINT_RAW(RT_DEBUG_TRACE, ("40MHz BW, 2.4G band, Gdata = %lx \n", Gdata));
		}
	}

	//
	// Check this block is valid for 20MHz in 5G. If invalid, use parameter for 20MHz in 2.4G
	//
	bValid = TRUE;
	for (i=0; i<8; i++)
	{
		value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_20MHZ_5G + 2 + i*2);
		if (((value & 0x00FF) == 0x00FF) || ((value & 0xFF00) == 0xFF00))
		{
			bValid = FALSE;
			break;
		}
	}

	//
	// Get Txpower per MCS for 20MHz in 5G.
	//
	if (bValid)
	{
		for (i=0; i<5; i++)
		{
			value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_20MHZ_5G + i*4);
			if (bApwrdeltaMinus == FALSE)
			{
				t1 = (value&0xf)+(Apwrdelta);
				if (t1 > 0xf)
					t1 = 0xf;
				t2 = ((value&0xf0)>>4)+(Apwrdelta);
				if (t2 > 0xf)
					t2 = 0xf;
				t3 = ((value&0xf00)>>8)+(Apwrdelta);
				if (t3 > 0xf)
					t3 = 0xf;
				t4 = ((value&0xf000)>>12)+(Apwrdelta);
				if (t4 > 0xf)
					t4 = 0xf;
			}
			else
			{
				if ((value&0xf) > Apwrdelta)
					t1 = (value&0xf)-(Apwrdelta);
				else
					t1 = 0;
				if (((value&0xf0)>>4) > Apwrdelta)
					t2 = ((value&0xf0)>>4)-(Apwrdelta);
				else
					t2 = 0;
				if (((value&0xf00)>>8) > Apwrdelta)
					t3 = ((value&0xf00)>>8)-(Apwrdelta);
				else
					t3 = 0;
				if (((value&0xf000)>>12) > Apwrdelta)
					t4 = ((value&0xf000)>>12)-(Apwrdelta);
				else
					t4 = 0;
			}				
			Adata = t1 + (t2<<4) + (t3<<8) + (t4<<12);
			
			value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_20MHZ_5G + i*4 + 2);
			if (bApwrdeltaMinus == FALSE)
			{
				t1 = (value&0xf)+(Apwrdelta);
				if (t1 > 0xf)
					t1 = 0xf;
				t2 = ((value&0xf0)>>4)+(Apwrdelta);
				if (t2 > 0xf)
					t2 = 0xf;
				t3 = ((value&0xf00)>>8)+(Apwrdelta);
				if (t3 > 0xf)
					t3 = 0xf;
				t4 = ((value&0xf000)>>12)+(Apwrdelta);
				if (t4 > 0xf)
					t4 = 0xf;
			}
			else
			{
				if ((value&0xf) > Apwrdelta)
					t1 = (value&0xf)-(Apwrdelta);
				else
					t1 = 0;
				if (((value&0xf0)>>4) > Apwrdelta)
					t2 = ((value&0xf0)>>4)-(Apwrdelta);
				else
					t2 = 0;
				if (((value&0xf00)>>8) > Apwrdelta)
					t3 = ((value&0xf00)>>8)-(Apwrdelta);
				else
					t3 = 0;
				if (((value&0xf000)>>12) > Apwrdelta)
					t4 = ((value&0xf000)>>12)-(Apwrdelta);
				else
					t4 = 0;
			}				
			Adata |= ((t1<<16) + (t2<<20) + (t3<<24) + (t4<<28));

			if (i == 0)
				pAd->Tx20MPwrCfgABand[i] = (pAd->Tx20MPwrCfgABand[i] & 0x0000FFFF) | (Adata & 0xFFFF0000);
			else
				pAd->Tx20MPwrCfgABand[i] = Adata;

			DBGPRINT_RAW(RT_DEBUG_TRACE, ("20MHz BW, 5GHz band, Adata = %lx \n", Adata));
		}
	}

	//
	// Check this block is valid for 40MHz in 5G. If invalid, use parameter for 20MHz in 2.4G
	//
	bValid = TRUE;
	for (i=0; i<6; i++)
	{
		value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_40MHZ_5G + 2 + i*2);
		if (((value & 0x00FF) == 0x00FF) || ((value & 0xFF00) == 0xFF00))
		{
			bValid = FALSE;
			break;
		}
	}

	//
	// Get Txpower per MCS for 40MHz in 5G.
	//
	if (bValid)
	{
		for (i=0; i<4; i++)
		{
			value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_40MHZ_5G + i*4);
			if (bApwrdeltaMinus == FALSE)
			{
				t1 = (value&0xf)+(Apwrdelta);
				if (t1 > 0xf)
					t1 = 0xf;
				t2 = ((value&0xf0)>>4)+(Apwrdelta);
				if (t2 > 0xf)
					t2 = 0xf;
				t3 = ((value&0xf00)>>8)+(Apwrdelta);
				if (t3 > 0xf)
					t3 = 0xf;
				t4 = ((value&0xf000)>>12)+(Apwrdelta);
				if (t4 > 0xf)
					t4 = 0xf;
			}
			else
			{
				if ((value&0xf) > Apwrdelta)
					t1 = (value&0xf)-(Apwrdelta);
				else
					t1 = 0;
				if (((value&0xf0)>>4) > Apwrdelta)
					t2 = ((value&0xf0)>>4)-(Apwrdelta);
				else
					t2 = 0;
				if (((value&0xf00)>>8) > Apwrdelta)
					t3 = ((value&0xf00)>>8)-(Apwrdelta);
				else
					t3 = 0;
				if (((value&0xf000)>>12) > Apwrdelta)
					t4 = ((value&0xf000)>>12)-(Apwrdelta);
				else
					t4 = 0;
			}				
			Adata = t1 + (t2<<4) + (t3<<8) + (t4<<12);
			
			value = RTMP_EEPROM_READ16(pAd, EEPROM_TXPOWER_BYRATE_40MHZ_5G + i*4 + 2);
			if (bApwrdeltaMinus == FALSE)
			{
				t1 = (value&0xf)+(Apwrdelta);
				if (t1 > 0xf)
					t1 = 0xf;
				t2 = ((value&0xf0)>>4)+(Apwrdelta);
				if (t2 > 0xf)
					t2 = 0xf;
				t3 = ((value&0xf00)>>8)+(Apwrdelta);
				if (t3 > 0xf)
					t3 = 0xf;
				t4 = ((value&0xf000)>>12)+(Apwrdelta);
				if (t4 > 0xf)
					t4 = 0xf;
			}
			else
			{
				if ((value&0xf) > Apwrdelta)
					t1 = (value&0xf)-(Apwrdelta);
				else
					t1 = 0;
				if (((value&0xf0)>>4) > Apwrdelta)
					t2 = ((value&0xf0)>>4)-(Apwrdelta);
				else
					t2 = 0;
				if (((value&0xf00)>>8) > Apwrdelta)
					t3 = ((value&0xf00)>>8)-(Apwrdelta);
				else
					t3 = 0;
				if (((value&0xf000)>>12) > Apwrdelta)
					t4 = ((value&0xf000)>>12)-(Apwrdelta);
				else
					t4 = 0;
			}				
			Adata |= ((t1<<16) + (t2<<20) + (t3<<24) + (t4<<28));

			if (i == 0)
				pAd->Tx40MPwrCfgABand[i+1] = (pAd->Tx40MPwrCfgABand[i+1] & 0x0000FFFF) | (Adata & 0xFFFF0000);
			else
				pAd->Tx40MPwrCfgABand[i+1] = Adata;

			DBGPRINT_RAW(RT_DEBUG_TRACE, ("40MHz BW, 5GHz band, Adata = %lx \n", Adata));
		}
	}
}


/*
	========================================================================
	
	Routine Description:
		Read initial channel power parameters from EEPROM
		
	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		
	========================================================================
*/
VOID	RTMPReadChannelPwr(
	IN	PRTMP_ADAPTER	pAd)
{
	UCHAR				i, choffset;
	EEPROM_TX_PWR_STRUC	    Power;
	EEPROM_TX_PWR_STRUC	    Power2;
	
	// Read Tx power value for all channels
	// Value from 1 - 0x7f. Default value is 24.
	// Power value : 2.4G 0x00 (0) ~ 0x1F (31)
	//             : 5.5G 0xF9 (-7) ~ 0x0F (15)
	
	// 0. 11b/g, ch1 - ch 14
	for (i = 0; i < 7; i++)
	{
//		Power.word = RTMP_EEPROM_READ16(pAd, EEPROM_G_TX_PWR_OFFSET + i * 2);
//		Power2.word = RTMP_EEPROM_READ16(pAd, EEPROM_G_TX2_PWR_OFFSET + i * 2);
		RT28xx_EEPROM_READ16(pAd, EEPROM_G_TX_PWR_OFFSET + i * 2, Power.word);
		RT28xx_EEPROM_READ16(pAd, EEPROM_G_TX2_PWR_OFFSET + i * 2, Power2.word);
		pAd->TxPower[i * 2].Channel = i * 2 + 1;
		pAd->TxPower[i * 2 + 1].Channel = i * 2 + 2;

		if ((Power.field.Byte0 > 31) || (Power.field.Byte0 < 0))
			pAd->TxPower[i * 2].Power = DEFAULT_RF_TX_POWER;
		else
			pAd->TxPower[i * 2].Power = Power.field.Byte0;

		if ((Power.field.Byte1 > 31) || (Power.field.Byte1 < 0))
			pAd->TxPower[i * 2 + 1].Power = DEFAULT_RF_TX_POWER;
		else
			pAd->TxPower[i * 2 + 1].Power = Power.field.Byte1;

		if ((Power2.field.Byte0 > 31) || (Power2.field.Byte0 < 0))
			pAd->TxPower[i * 2].Power2 = DEFAULT_RF_TX_POWER;
		else
			pAd->TxPower[i * 2].Power2 = Power2.field.Byte0;

		if ((Power2.field.Byte1 > 31) || (Power2.field.Byte1 < 0))
			pAd->TxPower[i * 2 + 1].Power2 = DEFAULT_RF_TX_POWER;
		else
			pAd->TxPower[i * 2 + 1].Power2 = Power2.field.Byte1;
	}
	
	// 1. U-NII lower/middle band: 36, 38, 40; 44, 46, 48; 52, 54, 56; 60, 62, 64 (including central frequency in BW 40MHz)
	// 1.1 Fill up channel
	choffset = 14;
	for (i = 0; i < 4; i++)
	{
		pAd->TxPower[3 * i + choffset + 0].Channel	= 36 + i * 8 + 0;
		pAd->TxPower[3 * i + choffset + 0].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 0].Power2	= DEFAULT_RF_TX_POWER;

		pAd->TxPower[3 * i + choffset + 1].Channel	= 36 + i * 8 + 2;
		pAd->TxPower[3 * i + choffset + 1].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 1].Power2	= DEFAULT_RF_TX_POWER;

		pAd->TxPower[3 * i + choffset + 2].Channel	= 36 + i * 8 + 4;
		pAd->TxPower[3 * i + choffset + 2].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 2].Power2	= DEFAULT_RF_TX_POWER;
	}

	// 1.2 Fill up power
	for (i = 0; i < 6; i++)
	{
//		Power.word = RTMP_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + i * 2);
//		Power2.word = RTMP_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + i * 2);
		RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + i * 2, Power.word);
		RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + i * 2, Power2.word);

		if ((Power.field.Byte0 < 16) && (Power.field.Byte0 >= -7))
			pAd->TxPower[i * 2 + choffset + 0].Power = Power.field.Byte0;

		if ((Power.field.Byte1 < 16) && (Power.field.Byte1 >= -7))
			pAd->TxPower[i * 2 + choffset + 1].Power = Power.field.Byte1;			

		if ((Power2.field.Byte0 < 16) && (Power2.field.Byte0 >= -7))
			pAd->TxPower[i * 2 + choffset + 0].Power2 = Power2.field.Byte0;

		if ((Power2.field.Byte1 < 16) && (Power2.field.Byte1 >= -7))
			pAd->TxPower[i * 2 + choffset + 1].Power2 = Power2.field.Byte1;			
	}
	
	// 2. HipperLAN 2 100, 102 ,104; 108, 110, 112; 116, 118, 120; 124, 126, 128; 132, 134, 136; 140 (including central frequency in BW 40MHz)
	// 2.1 Fill up channel
	choffset = 14 + 12;
	for (i = 0; i < 5; i++)
	{
		pAd->TxPower[3 * i + choffset + 0].Channel	= 100 + i * 8 + 0;
		pAd->TxPower[3 * i + choffset + 0].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 0].Power2	= DEFAULT_RF_TX_POWER;

		pAd->TxPower[3 * i + choffset + 1].Channel	= 100 + i * 8 + 2;
		pAd->TxPower[3 * i + choffset + 1].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 1].Power2	= DEFAULT_RF_TX_POWER;

		pAd->TxPower[3 * i + choffset + 2].Channel	= 100 + i * 8 + 4;
		pAd->TxPower[3 * i + choffset + 2].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 2].Power2	= DEFAULT_RF_TX_POWER;
	}
	pAd->TxPower[3 * 5 + choffset + 0].Channel		= 140;
	pAd->TxPower[3 * 5 + choffset + 0].Power		= DEFAULT_RF_TX_POWER;
	pAd->TxPower[3 * 5 + choffset + 0].Power2		= DEFAULT_RF_TX_POWER;

	// 2.2 Fill up power
	for (i = 0; i < 8; i++)
	{
//		Power.word = RTMP_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + i * 2);
//		Power2.word = RTMP_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + (choffset - 14) + i * 2);
		RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + i * 2, Power.word);
		RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + (choffset - 14) + i * 2, Power2.word);

		if ((Power.field.Byte0 < 16) && (Power.field.Byte0 >= -7))
			pAd->TxPower[i * 2 + choffset + 0].Power = Power.field.Byte0;

		if ((Power.field.Byte1 < 16) && (Power.field.Byte1 >= -7))
			pAd->TxPower[i * 2 + choffset + 1].Power = Power.field.Byte1;			

		if ((Power2.field.Byte0 < 16) && (Power2.field.Byte0 >= -7))
			pAd->TxPower[i * 2 + choffset + 0].Power2 = Power2.field.Byte0;

		if ((Power2.field.Byte1 < 16) && (Power2.field.Byte1 >= -7))
			pAd->TxPower[i * 2 + choffset + 1].Power2 = Power2.field.Byte1;			
	}

	// 3. U-NII upper band: 149, 151, 153; 157, 159, 161; 165 (including central frequency in BW 40MHz)
	// 3.1 Fill up channel
	choffset = 14 + 12 + 16;
	for (i = 0; i < 2; i++)
	{
		pAd->TxPower[3 * i + choffset + 0].Channel	= 149 + i * 8 + 0;
		pAd->TxPower[3 * i + choffset + 0].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 0].Power2	= DEFAULT_RF_TX_POWER;

		pAd->TxPower[3 * i + choffset + 1].Channel	= 149 + i * 8 + 2;
		pAd->TxPower[3 * i + choffset + 1].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 1].Power2	= DEFAULT_RF_TX_POWER;

		pAd->TxPower[3 * i + choffset + 2].Channel	= 149 + i * 8 + 4;
		pAd->TxPower[3 * i + choffset + 2].Power	= DEFAULT_RF_TX_POWER;
		pAd->TxPower[3 * i + choffset + 2].Power2	= DEFAULT_RF_TX_POWER;
	}
	pAd->TxPower[3 * 2 + choffset + 0].Channel		= 165;
	pAd->TxPower[3 * 2 + choffset + 0].Power		= DEFAULT_RF_TX_POWER;
	pAd->TxPower[3 * 2 + choffset + 0].Power2		= DEFAULT_RF_TX_POWER;

	// 3.2 Fill up power
	for (i = 0; i < 4; i++)
	{
//		Power.word = RTMP_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + i * 2);
//		Power2.word = RTMP_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + (choffset - 14) + i * 2);
		RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX_PWR_OFFSET + (choffset - 14) + i * 2, Power.word);
		RT28xx_EEPROM_READ16(pAd, EEPROM_A_TX2_PWR_OFFSET + (choffset - 14) + i * 2, Power2.word);

		if ((Power.field.Byte0 < 16) && (Power.field.Byte0 >= -7))
			pAd->TxPower[i * 2 + choffset + 0].Power = Power.field.Byte0;

		if ((Power.field.Byte1 < 16) && (Power.field.Byte1 >= -7))
			pAd->TxPower[i * 2 + choffset + 1].Power = Power.field.Byte1;			

		if ((Power2.field.Byte0 < 16) && (Power2.field.Byte0 >= -7))
			pAd->TxPower[i * 2 + choffset + 0].Power2 = Power2.field.Byte0;

		if ((Power2.field.Byte1 < 16) && (Power2.field.Byte1 >= -7))
			pAd->TxPower[i * 2 + choffset + 1].Power2 = Power2.field.Byte1;			
	}

	// 4. Print and Debug
	choffset = 14 + 12 + 16 + 7;
	for (i = 0; i < choffset; i++)
	{
		DBGPRINT(RT_DEBUG_INFO, ("E2PROM: TxPower[%03d], Channel = %d, Power = %d, Power2 = %d\n", i, pAd->TxPower[i].Channel, pAd->TxPower[i].Power, pAd->TxPower[i].Power2 ));
	}

#if 0
	// Init the 802.11j channel number for TX channel power
	// 0. 20MHz 
	for (i = 0; i < 3; i++)
	{
		pAd->TxPower11J[i].Channel = 8 + i * 4;
		pAd->TxPower11J[i].BW = BW_20;
	}
	
	for (i = 0; i < 4; i++)
	{
		pAd->TxPower11J[i + 3].Channel = 34 + i * 4;
		pAd->TxPower11J[i + 3].BW = BW_20;
	}

	for (i = 0; i < 4; i++)
	{
		pAd->TxPower11J[i + 7].Channel = 184 + i * 4;
		pAd->TxPower11J[i + 7].BW = BW_20;
	}

	// 0. 10MHz 
	for (i = 0; i < 2; i++)
	{
		pAd->TxPower11J[i + 11].Channel = 7 + i;
		pAd->TxPower11J[i + 11].BW = BW_10;
	}
	pAd->TxPower11J[13].Channel = 11;
	pAd->TxPower11J[13].BW = BW_10;
	
	for (i = 0; i < 3; i++)
	{
		pAd->TxPower11J[i + 14].Channel = 183 + i;
		pAd->TxPower11J[i + 14].BW= BW_10;
	}

	for (i = 0; i < 3; i++)
	{
		pAd->TxPower11J[i + 17].Channel = 187 + i;
		pAd->TxPower11J[i + 17].BW = BW_10;
	}
	for (i = 0; i < 10; i++)
	{
		Power.word = RTMP_EEPROM_READ16(pAd, EEPROM_Japan_TX_PWR_OFFSET + i * 2);
		Power2.word = RTMP_EEPROM_READ16(pAd, EEPROM_Japan_TX2_PWR_OFFSET + i * 2);

		if ((Power.field.Byte0 < 36) && (Power.field.Byte0 > -6))
			pAd->TxPower11J[i * 2].Power = Power.field.Byte0;

		if ((Power.field.Byte1 < 36) && (Power.field.Byte1 > -6))
			pAd->TxPower11J[i * 2 + 1].Power = Power.field.Byte1;			

		if ((Power2.field.Byte0 < 36) && (Power2.field.Byte0 > -6))
			pAd->TxPower11J[i * 2].Power2 = Power2.field.Byte0;

		if ((Power2.field.Byte1 < 36) && (Power2.field.Byte1 > -6))
			pAd->TxPower11J[i * 2 + 1].Power2 = Power2.field.Byte1;			
	}
#endif
}

/*
	========================================================================
	
	Routine Description:
		Read the following from the registry
		1. All the parameters
		2. NetworkAddres

	Arguments:
		Adapter						Pointer to our adapter
		WrapperConfigurationContext	For use by NdisOpenConfiguration

	Return Value:
		NDIS_STATUS_SUCCESS
		NDIS_STATUS_FAILURE
		NDIS_STATUS_RESOURCES

	IRQL = PASSIVE_LEVEL

	Note:
	
	========================================================================
*/
NDIS_STATUS	NICReadRegParameters(
	IN	PRTMP_ADAPTER		pAd,
	IN	NDIS_HANDLE			WrapperConfigurationContext
	)
{
	NDIS_STATUS						Status = NDIS_STATUS_SUCCESS;
	DBGPRINT_S(Status, ("<-- NICReadRegParameters, Status=%x\n", Status));
	return Status;
}


/*
	========================================================================
	
	Routine Description:
		Read initial parameters from EEPROM
		
	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		
	========================================================================
*/
VOID	NICReadEEPROMParameters(
	IN	PRTMP_ADAPTER	pAd,
	IN	PUCHAR			mac_addr)
{
	UINT32			data = 0;
	USHORT			i, value, value2;
	UCHAR			TmpPhy;
	EEPROM_TX_PWR_STRUC	    Power;
	EEPROM_VERSION_STRUC    Version;
	EEPROM_ANTENNA_STRUC	Antenna;
	EEPROM_NIC_CONFIG2_STRUC    NicConfig2;

	DBGPRINT(RT_DEBUG_TRACE, ("--> NICReadEEPROMParameters\n"));	

	// Init EEPROM Address Number, before access EEPROM; if 93c46, EEPROMAddressNum=6, else if 93c66, EEPROMAddressNum=8
	RTMP_IO_READ32(pAd, E2PROM_CSR, &data);
	DBGPRINT(RT_DEBUG_TRACE, ("--> E2PROM_CSR = 0x%x\n", data));

	if((data & 0x30) == 0)
		pAd->EEPROMAddressNum = 6;		// 93C46
	else if((data & 0x30) == 0x10)
		pAd->EEPROMAddressNum = 8;     // 93C66
	else
		pAd->EEPROMAddressNum = 8;     // 93C86
	DBGPRINT(RT_DEBUG_TRACE, ("--> EEPROMAddressNum = %d\n", pAd->EEPROMAddressNum ));

	// RT2860 MAC no longer auto load MAC address from E2PROM. Driver has to intialize
	// MAC address registers according to E2PROM setting
	if (mac_addr == NULL ||
		strlen(mac_addr) != 17 || 
		mac_addr[2] != ':'  || mac_addr[5] != ':'  || mac_addr[8] != ':' ||
		mac_addr[11] != ':' || mac_addr[14] != ':')
	{
		USHORT  Addr01,Addr23,Addr45 ;

		RT28xx_EEPROM_READ16(pAd, 0x04, Addr01);
		RT28xx_EEPROM_READ16(pAd, 0x06, Addr23);
		RT28xx_EEPROM_READ16(pAd, 0x08, Addr45);

		pAd->PermanentAddress[0] = (UCHAR)(Addr01 & 0xff);
		pAd->PermanentAddress[1] = (UCHAR)(Addr01 >> 8);
		pAd->PermanentAddress[2] = (UCHAR)(Addr23 & 0xff);
		pAd->PermanentAddress[3] = (UCHAR)(Addr23 >> 8);
		pAd->PermanentAddress[4] = (UCHAR)(Addr45 & 0xff);
		pAd->PermanentAddress[5] = (UCHAR)(Addr45 >> 8);

		DBGPRINT(RT_DEBUG_TRACE, ("Initialize MAC Address from E2PROM \n"));
	}
	else
	{
		INT		j;
		PUCHAR	macptr;

		macptr = mac_addr;

		for (j=0; j<MAC_ADDR_LEN; j++)
		{
			AtoH(macptr, &pAd->PermanentAddress[j], 1);
			macptr=macptr+3;
		}	
		
		DBGPRINT(RT_DEBUG_TRACE, ("Initialize MAC Address from module parameter \n"));
	}
	
	
	{
#if 0		
		USHORT  Addr01,Addr23,Addr45 ;

		Addr01=RTMP_EEPROM_READ16(pAd, 0x04);
		Addr23=RTMP_EEPROM_READ16(pAd, 0x06);
		Addr45=RTMP_EEPROM_READ16(pAd, 0x08);

		pAd->PermanentAddress[0] = (UCHAR)(Addr01 & 0xff);
		pAd->PermanentAddress[1] = (UCHAR)(Addr01 >> 8);
		pAd->PermanentAddress[2] = (UCHAR)(Addr23 & 0xff);
		pAd->PermanentAddress[3] = (UCHAR)(Addr23 >> 8);
		pAd->PermanentAddress[4] = (UCHAR)(Addr45 & 0xff);
		pAd->PermanentAddress[5] = (UCHAR)(Addr45 >> 8);
#endif
		//more conveninet to test mbssid, so ap's bssid &0xf1
		if (pAd->PermanentAddress[0] == 0xff)
			pAd->PermanentAddress[0] = RandomByte(pAd)&0xf8;
		
		//if (pAd->PermanentAddress[5] == 0xff)
		//	pAd->PermanentAddress[5] = RandomByte(pAd)&0xf8;
			
		DBGPRINT_RAW(RT_DEBUG_TRACE,("E2PROM MAC: =%02x:%02x:%02x:%02x:%02x:%02x\n",
			pAd->PermanentAddress[0], pAd->PermanentAddress[1], 
			pAd->PermanentAddress[2], pAd->PermanentAddress[3], 
			pAd->PermanentAddress[4], pAd->PermanentAddress[5]));
		if (pAd->bLocalAdminMAC == FALSE)
		{
			MAC_DW0_STRUC csr2;
			MAC_DW1_STRUC csr3;
			COPY_MAC_ADDR(pAd->CurrentAddress, pAd->PermanentAddress);
			csr2.field.Byte0 = pAd->CurrentAddress[0];
			csr2.field.Byte1 = pAd->CurrentAddress[1];
			csr2.field.Byte2 = pAd->CurrentAddress[2];
			csr2.field.Byte3 = pAd->CurrentAddress[3];
			RTMP_IO_WRITE32(pAd, MAC_ADDR_DW0, csr2.word);
			csr3.word = 0;
			csr3.field.Byte4 = pAd->CurrentAddress[4];
			csr3.field.Byte5 = pAd->CurrentAddress[5];
			csr3.field.U2MeMask = 0xff;
			RTMP_IO_WRITE32(pAd, MAC_ADDR_DW1, csr3.word);
			DBGPRINT_RAW(RT_DEBUG_TRACE,("E2PROM MAC: =%02x:%02x:%02x:%02x:%02x:%02x\n",
				pAd->PermanentAddress[0], pAd->PermanentAddress[1], 
				pAd->PermanentAddress[2], pAd->PermanentAddress[3], 
				pAd->PermanentAddress[4], pAd->PermanentAddress[5]));
		}
	}   

	// if not return early. cause fail at emulation.
	// Init the channel number for TX channel power	
	RTMPReadChannelPwr(pAd);

	// if E2PROM version mismatch with driver's expectation, then skip
	// all subsequent E2RPOM retieval and set a system error bit to notify GUI
	RT28xx_EEPROM_READ16(pAd, EEPROM_VERSION_OFFSET, Version.word);
	pAd->EepromVersion = Version.field.Version + Version.field.FaeReleaseNumber * 256;
	DBGPRINT(RT_DEBUG_TRACE, ("E2PROM: Version = %d, FAE release #%d\n", Version.field.Version, Version.field.FaeReleaseNumber));

	if (Version.field.Version > VALID_EEPROM_VERSION)
	{
		DBGPRINT_ERR(("E2PROM: WRONG VERSION 0x%x, should be %d\n",Version.field.Version, VALID_EEPROM_VERSION));
		/*pAd->SystemErrorBitmap |= 0x00000001;

		// hard-code default value when no proper E2PROM installed
		pAd->bAutoTxAgcA = FALSE;
		pAd->bAutoTxAgcG = FALSE;

		// Default the channel power
		for (i = 0; i < MAX_NUM_OF_CHANNELS; i++)
			pAd->TxPower[i].Power = DEFAULT_RF_TX_POWER;

		// Default the channel power
		for (i = 0; i < MAX_NUM_OF_11JCHANNELS; i++)
			pAd->TxPower11J[i].Power = DEFAULT_RF_TX_POWER;
		
		for(i = 0; i < NUM_EEPROM_BBP_PARMS; i++)
			pAd->EEPROMDefaultValue[i] = 0xffff;
		return;  */
	}

	// Read BBP default value from EEPROM and store to array(EEPROMDefaultValue) in pAd
	RT28xx_EEPROM_READ16(pAd, EEPROM_NIC1_OFFSET, value);
	pAd->EEPROMDefaultValue[0] = value;

	RT28xx_EEPROM_READ16(pAd, EEPROM_NIC2_OFFSET, value);
	pAd->EEPROMDefaultValue[1] = value;

	RT28xx_EEPROM_READ16(pAd, 0x38, value);	// Country Region
	pAd->EEPROMDefaultValue[2] = value;

	for(i = 0; i < 8; i++)
	{
		RT28xx_EEPROM_READ16(pAd, EEPROM_BBP_BASE_OFFSET + i*2, value);
		pAd->EEPROMDefaultValue[i+3] = value;
	}

	// We have to parse NIC configuration 0 at here.
	// If TSSI did not have preloaded value, it should reset the TxAutoAgc to false
	// Therefore, we have to read TxAutoAgc control beforehand.
	// Read Tx AGC control bit
	Antenna.word = pAd->EEPROMDefaultValue[0];
	if (Antenna.word == 0xFFFF)
	{
		Antenna.word = 0;
		Antenna.field.RfIcType = RFIC_2820;
		Antenna.field.TxPath = 1;
		Antenna.field.RxPath = 2;
		DBGPRINT(RT_DEBUG_WARN, ("E2PROM error, hard code as 0x%04x\n", Antenna.word));
	}

	// Choose the desired Tx&Rx stream.
	if ((pAd->CommonCfg.TxStream == 0) || (pAd->CommonCfg.TxStream > Antenna.field.TxPath))
		pAd->CommonCfg.TxStream = Antenna.field.TxPath;

	if ((pAd->CommonCfg.RxStream == 0) || (pAd->CommonCfg.RxStream > Antenna.field.RxPath))
	{
		pAd->CommonCfg.RxStream = Antenna.field.RxPath;
	
		if ((pAd->MACVersion < RALINK_2883_VERSION) &&
			(pAd->CommonCfg.RxStream > 2))
		{
			// only 2 Rx streams for RT2860 series
			pAd->CommonCfg.RxStream = 2;
		}
	}

	// 3*3
	// read value from EEPROM and set them to CSR174 ~ 177 in chain0 ~ chain2
	// yet implement
	for(i=0; i<3; i++)
	{
	}

	NicConfig2.word = pAd->EEPROMDefaultValue[1];

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (NicConfig2.word == 0xffff)
		{
			NicConfig2.word = 0;
		}
	}
#endif // CONFIG_AP_SUPPORT //	


	if (NicConfig2.field.DynamicTxAgcControl == 1)
		pAd->bAutoTxAgcA = pAd->bAutoTxAgcG = TRUE;
	else
		pAd->bAutoTxAgcA = pAd->bAutoTxAgcG = FALSE;
	
	DBGPRINT_RAW(RT_DEBUG_TRACE, ("NICReadEEPROMParameters: RxPath = %d, TxPath = %d\n", Antenna.field.RxPath, Antenna.field.TxPath));

	// Save the antenna for future use
	pAd->Antenna.word = Antenna.word;

	//
	// Reset PhyMode if we don't support 802.11a
	//
	if ((pAd->CommonCfg.PhyMode == PHY_11ABG_MIXED) || (pAd->CommonCfg.PhyMode == PHY_11ABGN_MIXED) ||
		(pAd->CommonCfg.PhyMode == PHY_11A) || (pAd->CommonCfg.PhyMode == PHY_11AN_MIXED) || (pAd->CommonCfg.PhyMode == PHY_11AGN_MIXED))
	{
		//
		// Only RFIC_2850 & RFIC_2750 support 802.11a
		//
		if ((Antenna.field.RfIcType != RFIC_2850) && (Antenna.field.RfIcType != RFIC_2750))
			pAd->CommonCfg.PhyMode = PHY_11BGN_MIXED;
	}
	
	// Read TSSI reference and TSSI boundary for temperature compensation. This is ugly
	// 0. 11b/g
	{
		/* these are tempature reference value (0x00 ~ 0xFE)
		   ex: 0x00 0x15 0x25 0x45 0x88 0xA0 0xB5 0xD0 0xF0
		   TssiPlusBoundaryG [4] [3] [2] [1] [0] (smaller) +
		   TssiMinusBoundaryG[0] [1] [2] [3] [4] (larger) */
		RT28xx_EEPROM_READ16(pAd, 0x6E, Power.word);
		pAd->TssiMinusBoundaryG[4] = Power.field.Byte0;
		pAd->TssiMinusBoundaryG[3] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0x70, Power.word);
		pAd->TssiMinusBoundaryG[2] = Power.field.Byte0;
		pAd->TssiMinusBoundaryG[1] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0x72, Power.word);
		pAd->TssiRefG   = Power.field.Byte0; /* reference value [0] */
		pAd->TssiPlusBoundaryG[1] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0x74, Power.word);
		pAd->TssiPlusBoundaryG[2] = Power.field.Byte0;
		pAd->TssiPlusBoundaryG[3] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0x76, Power.word);
		pAd->TssiPlusBoundaryG[4] = Power.field.Byte0;
		pAd->TxAgcStepG = Power.field.Byte1;    
		pAd->TxAgcCompensateG = 0;
		pAd->TssiMinusBoundaryG[0] = pAd->TssiRefG;
		pAd->TssiPlusBoundaryG[0]  = pAd->TssiRefG;

		// Disable TxAgc if the based value is not right
		if (pAd->TssiRefG == 0xff)
			pAd->bAutoTxAgcG = FALSE;

		DBGPRINT(RT_DEBUG_TRACE,("E2PROM: G Tssi[-4 .. +4] = %d %d %d %d - %d -%d %d %d %d, step=%d, tuning=%d\n",
			pAd->TssiMinusBoundaryG[4], pAd->TssiMinusBoundaryG[3], pAd->TssiMinusBoundaryG[2], pAd->TssiMinusBoundaryG[1],
			pAd->TssiRefG,
			pAd->TssiPlusBoundaryG[1], pAd->TssiPlusBoundaryG[2], pAd->TssiPlusBoundaryG[3], pAd->TssiPlusBoundaryG[4],
			pAd->TxAgcStepG, pAd->bAutoTxAgcG));
	}	
	// 1. 11a
	{
		RT28xx_EEPROM_READ16(pAd, 0xD4, Power.word);
		pAd->TssiMinusBoundaryA[4] = Power.field.Byte0;
		pAd->TssiMinusBoundaryA[3] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0xD6, Power.word);
		pAd->TssiMinusBoundaryA[2] = Power.field.Byte0;
		pAd->TssiMinusBoundaryA[1] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0xD8, Power.word);
		pAd->TssiRefA   = Power.field.Byte0;
		pAd->TssiPlusBoundaryA[1] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0xDA, Power.word);
		pAd->TssiPlusBoundaryA[2] = Power.field.Byte0;
		pAd->TssiPlusBoundaryA[3] = Power.field.Byte1;
		RT28xx_EEPROM_READ16(pAd, 0xDC, Power.word);
		pAd->TssiPlusBoundaryA[4] = Power.field.Byte0;
		pAd->TxAgcStepA = Power.field.Byte1;    
		pAd->TxAgcCompensateA = 0;
		pAd->TssiMinusBoundaryA[0] = pAd->TssiRefA;
		pAd->TssiPlusBoundaryA[0]  = pAd->TssiRefA;

		// Disable TxAgc if the based value is not right
		if (pAd->TssiRefA == 0xff)
			pAd->bAutoTxAgcA = FALSE;

		DBGPRINT(RT_DEBUG_TRACE,("E2PROM: A Tssi[-4 .. +4] = %d %d %d %d - %d -%d %d %d %d, step=%d, tuning=%d\n",
			pAd->TssiMinusBoundaryA[4], pAd->TssiMinusBoundaryA[3], pAd->TssiMinusBoundaryA[2], pAd->TssiMinusBoundaryA[1],
			pAd->TssiRefA,
			pAd->TssiPlusBoundaryA[1], pAd->TssiPlusBoundaryA[2], pAd->TssiPlusBoundaryA[3], pAd->TssiPlusBoundaryA[4],
			pAd->TxAgcStepA, pAd->bAutoTxAgcA));
	}	
	pAd->BbpRssiToDbmDelta = 0x0;
	
	// Read frequency offset setting for RF
	RT28xx_EEPROM_READ16(pAd, EEPROM_FREQ_OFFSET, value);
	if ((value & 0x00FF) != 0x00FF)
		pAd->RfFreqOffset = (ULONG) (value & 0x00FF);
	else
		pAd->RfFreqOffset = 0;
	DBGPRINT(RT_DEBUG_TRACE, ("E2PROM: RF FreqOffset=0x%lx \n", pAd->RfFreqOffset));

	//CountryRegion byte offset (38h)
	value = pAd->EEPROMDefaultValue[2] >> 8;		// 2.4G band
	value2 = pAd->EEPROMDefaultValue[2] & 0x00FF;	// 5G band
	
	if ((value <= REGION_MAXIMUM_BG_BAND) && (value2 <= REGION_MAXIMUM_A_BAND))
	{
		pAd->CommonCfg.CountryRegion = ((UCHAR) value) | 0x80;
		pAd->CommonCfg.CountryRegionForABand = ((UCHAR) value2) | 0x80;
		TmpPhy = pAd->CommonCfg.PhyMode;
		pAd->CommonCfg.PhyMode = 0xff;
		RTMPSetPhyMode(pAd, TmpPhy);
		SetCommonHT(pAd);
	}

	//
	// Get RSSI Offset on EEPROM 0x9Ah & 0x9Ch.
	// The valid value are (-10 ~ 10) 
	// 
	RT28xx_EEPROM_READ16(pAd, EEPROM_RSSI_BG_OFFSET, value);
	pAd->BGRssiOffset0 = value & 0x00ff;
	pAd->BGRssiOffset1 = (value >> 8);
	RT28xx_EEPROM_READ16(pAd, EEPROM_RSSI_BG_OFFSET+2, value);
	pAd->BGRssiOffset2 = value & 0x00ff;
	pAd->ALNAGain1 = (value >> 8);
	RT28xx_EEPROM_READ16(pAd, EEPROM_LNA_OFFSET, value);
	pAd->BLNAGain = value & 0x00ff;
	pAd->ALNAGain0 = (value >> 8);
	
	// Validate 11b/g RSSI_0 offset.
	if ((pAd->BGRssiOffset0 < -10) || (pAd->BGRssiOffset0 > 10))
		pAd->BGRssiOffset0 = 0;

	// Validate 11b/g RSSI_1 offset.
	if ((pAd->BGRssiOffset1 < -10) || (pAd->BGRssiOffset1 > 10))
		pAd->BGRssiOffset1 = 0;

	// Validate 11b/g RSSI_2 offset.
	if ((pAd->BGRssiOffset2 < -10) || (pAd->BGRssiOffset2 > 10))
		pAd->BGRssiOffset2 = 0;
		
	RT28xx_EEPROM_READ16(pAd, EEPROM_RSSI_A_OFFSET, value);
	pAd->ARssiOffset0 = value & 0x00ff;
	pAd->ARssiOffset1 = (value >> 8);
	RT28xx_EEPROM_READ16(pAd, (EEPROM_RSSI_A_OFFSET+2), value);
	pAd->ARssiOffset2 = value & 0x00ff;
	pAd->ALNAGain2 = (value >> 8);

	if (((UCHAR)pAd->ALNAGain1 == 0xFF) || (pAd->ALNAGain1 == 0x00))
		pAd->ALNAGain1 = pAd->ALNAGain0;
	if (((UCHAR)pAd->ALNAGain2 == 0xFF) || (pAd->ALNAGain2 == 0x00))
		pAd->ALNAGain2 = pAd->ALNAGain0;

	// Validate 11a RSSI_0 offset.
	if ((pAd->ARssiOffset0 < -10) || (pAd->ARssiOffset0 > 10))
		pAd->ARssiOffset0 = 0;

	// Validate 11a RSSI_1 offset.
	if ((pAd->ARssiOffset1 < -10) || (pAd->ARssiOffset1 > 10))
		pAd->ARssiOffset1 = 0;

	//Validate 11a RSSI_2 offset.
	if ((pAd->ARssiOffset2 < -10) || (pAd->ARssiOffset2 > 10))
		pAd->ARssiOffset2 = 0;

	//
	// Get LED Setting.
	//
	RT28xx_EEPROM_READ16(pAd, 0x3a, value);
	pAd->LedCntl.word = (value&0xff00) >> 8;
	RT28xx_EEPROM_READ16(pAd, EEPROM_LED1_OFFSET, value);
	pAd->Led1 = value;
	RT28xx_EEPROM_READ16(pAd, EEPROM_LED2_OFFSET, value);
	pAd->Led2 = value;
	RT28xx_EEPROM_READ16(pAd, EEPROM_LED3_OFFSET, value);
	pAd->Led3 = value;
		
	RTMPReadTxPwrPerRate(pAd);	
	DBGPRINT(RT_DEBUG_TRACE, ("<-- NICReadEEPROMParameters\n"));
}

/*
	========================================================================
	
	Routine Description:
		Set default value from EEPROM
		
	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	
	Note:
		
	========================================================================
*/
VOID	NICInitAsicFromEEPROM(
	IN	PRTMP_ADAPTER	pAd)
{
	USHORT					i;
	EEPROM_ANTENNA_STRUC	Antenna;
	EEPROM_NIC_CONFIG2_STRUC    NicConfig2;
	UCHAR	BBPR3 = 0;
	
	DBGPRINT(RT_DEBUG_TRACE, ("--> NICInitAsicFromEEPROM\n"));
	for(i = 3; i < NUM_EEPROM_BBP_PARMS; i++)
	{
		UCHAR BbpRegIdx, BbpValue;
	
		if ((pAd->EEPROMDefaultValue[i] != 0xFFFF) && (pAd->EEPROMDefaultValue[i] != 0))
		{
			BbpRegIdx = (UCHAR)(pAd->EEPROMDefaultValue[i] >> 8);
			BbpValue  = (UCHAR)(pAd->EEPROMDefaultValue[i] & 0xff);
			RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BbpRegIdx, BbpValue);
		}
	}

	Antenna.word = pAd->Antenna.word;
	pAd->Mlme.RealRxPath = (UCHAR) Antenna.field.RxPath;
	pAd->RfIcType = (UCHAR) Antenna.field.RfIcType;

	NicConfig2.word = pAd->EEPROMDefaultValue[1];

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		if (NicConfig2.word == 0xffff)
		{
			NicConfig2.word = 0;
		}
	}
#endif // CONFIG_AP_SUPPORT //

	
	// Save the antenna for future use
	pAd->NicConfig2.word = NicConfig2.word;

	//
	// Send LED Setting to MCU.
	//
	if (pAd->LedCntl.word == 0xFF)
	{
		pAd->LedCntl.word = 0x01;
		pAd->Led1 = 0x5555;
		pAd->Led2 = 0x2221;

#ifdef RT2860
		pAd->Led3 = 0xA9F8;
#endif // RT2860 //
	}

	AsicSendCommandToMcu(pAd, 0x52, 0xff, (UCHAR)pAd->Led1, (UCHAR)(pAd->Led1 >> 8));
	AsicSendCommandToMcu(pAd, 0x53, 0xff, (UCHAR)pAd->Led2, (UCHAR)(pAd->Led2 >> 8));
	AsicSendCommandToMcu(pAd, 0x54, 0xff, (UCHAR)pAd->Led3, (UCHAR)(pAd->Led3 >> 8));
    pAd->LedIndicatorStregth = 0xFF;
    RTMPSetSignalLED(pAd, -100);	// Force signal strength Led to be turned off, before link up


	// Turn off patching for cardbus controller
	if (NicConfig2.field.CardbusAcceleration == 1)
	{
//		pAd->bTest1 = TRUE;
	}

	if (NicConfig2.field.DynamicTxAgcControl == 1)
		pAd->bAutoTxAgcA = pAd->bAutoTxAgcG = TRUE;
	else
		pAd->bAutoTxAgcA = pAd->bAutoTxAgcG = FALSE;
	//
	// Since BBP has been progamed, to make sure BBP setting will be 
	// upate inside of AsicAntennaSelect, so reset to UNKNOWN_BAND!!
	//
	pAd->CommonCfg.BandState = UNKNOWN_BAND;
	
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

	DBGPRINT(RT_DEBUG_TRACE, ("TxPath = %d, RxPath = %d, RFIC=%d, Polar+LED mode=%x\n", pAd->Antenna.field.TxPath, pAd->Antenna.field.RxPath, pAd->RfIcType, pAd->LedCntl.word));
	DBGPRINT(RT_DEBUG_TRACE, ("<-- NICInitAsicFromEEPROM\n"));
}

/*
	========================================================================
	
	Routine Description:
		Initialize NIC hardware

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		
	========================================================================
*/
NDIS_STATUS	NICInitializeAdapter(
	IN	PRTMP_ADAPTER	pAd,
	IN   BOOLEAN    bHardReset)
{
	NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
	WPDMA_GLO_CFG_STRUC	GloCfg;
#ifdef RT2860
	UINT32			Value;
	DELAY_INT_CFG_STRUC	IntCfg;
#endif // RT2860 //
//	INT_MASK_CSR_STRUC		IntMask;
	ULONG	i =0, j=0;
	AC_TXOP_CSR0_STRUC	csr0;

	DBGPRINT(RT_DEBUG_TRACE, ("--> NICInitializeAdapter\n"));

	// 3. Set DMA global configuration except TX_DMA_EN and RX_DMA_EN bits:
retry:
	i = 0;
	do
	{
		RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);
		if ((GloCfg.field.TxDMABusy == 0)  && (GloCfg.field.RxDMABusy == 0))
			break;
		
		RTMPusecDelay(1000);
		i++;
	}while ( i<100);
	DBGPRINT(RT_DEBUG_TRACE, ("<== DMA offset 0x208 = 0x%x\n", GloCfg.word));	
	GloCfg.word &= 0xff0;
	GloCfg.field.EnTXWriteBackDDONE =1;
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);
	
	// Record HW Beacon offset
	pAd->BeaconOffset[0] = HW_BEACON_BASE0;
	pAd->BeaconOffset[1] = HW_BEACON_BASE1;
	pAd->BeaconOffset[2] = HW_BEACON_BASE2;
	pAd->BeaconOffset[3] = HW_BEACON_BASE3;
	pAd->BeaconOffset[4] = HW_BEACON_BASE4;
	pAd->BeaconOffset[5] = HW_BEACON_BASE5;
	pAd->BeaconOffset[6] = HW_BEACON_BASE6;
	pAd->BeaconOffset[7] = HW_BEACON_BASE7;
	
	//
	// write all shared Ring's base address into ASIC
	//

	// asic simulation sequence put this ahead before loading firmware.
	// pbf hardware reset
#ifdef RT2860
	RTMP_IO_WRITE32(pAd, WPDMA_RST_IDX, 0x1003f);	// 0x10000 for reset rx, 0x3f resets all 6 tx rings.
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0xe1f);
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0xe00);
#endif // RT2860 //

	// Initialze ASIC for TX & Rx operation
	if (NICInitializeAsic(pAd , bHardReset) != NDIS_STATUS_SUCCESS)
	{
		if (j++ == 0)
		{
			NICLoadFirmware(pAd);
			goto retry;
		}
		return NDIS_STATUS_FAILURE;
	}


#ifdef RT2860
	// Write AC_BK base address register
#ifdef WIN_NDIS
	Value = NdisGetPhysicalAddressLow(pAd->TxRing[QID_AC_BK].Cell[0].AllocPa);
#else
	Value = RTMP_GetPhysicalAddressLow(pAd->TxRing[QID_AC_BK].Cell[0].AllocPa);
#endif		
	RTMP_IO_WRITE32(pAd, TX_BASE_PTR1, Value);
	DBGPRINT(RT_DEBUG_TRACE, ("--> TX_BASE_PTR1 : 0x%x\n", Value));

	// Write AC_BE base address register
#ifdef WIN_NDIS
	Value = NdisGetPhysicalAddressLow(pAd->TxRing[QID_AC_BE].Cell[0].AllocPa);
#else
	Value = RTMP_GetPhysicalAddressLow(pAd->TxRing[QID_AC_BE].Cell[0].AllocPa);
#endif
	RTMP_IO_WRITE32(pAd, TX_BASE_PTR0, Value);
	DBGPRINT(RT_DEBUG_TRACE, ("--> TX_BASE_PTR0 : 0x%x\n", Value));

	// Write AC_VI base address register
#ifdef WIN_NDIS
	Value = NdisGetPhysicalAddressLow(pAd->TxRing[QID_AC_VI].Cell[0].AllocPa);
#else
	Value = RTMP_GetPhysicalAddressLow(pAd->TxRing[QID_AC_VI].Cell[0].AllocPa);
#endif
	RTMP_IO_WRITE32(pAd, TX_BASE_PTR2, Value);
	DBGPRINT(RT_DEBUG_TRACE, ("--> TX_BASE_PTR2 : 0x%x\n", Value));

	// Write AC_VO base address register
#ifdef WIN_NDIS
	Value = NdisGetPhysicalAddressLow(pAd->TxRing[QID_AC_VO].Cell[0].AllocPa);
#else
	Value = RTMP_GetPhysicalAddressLow(pAd->TxRing[QID_AC_VO].Cell[0].AllocPa);	
#endif

	RTMP_IO_WRITE32(pAd, TX_BASE_PTR3, Value);
	DBGPRINT(RT_DEBUG_TRACE, ("--> TX_BASE_PTR3 : 0x%x\n", Value));

	// Write HCCA base address register
#ifdef WIN_NDIS
	  Value = NdisGetPhysicalAddressLow(pAd->TxRing[QID_HCCA].Cell[0].AllocPa);
#else
	  Value = RTMP_GetPhysicalAddressLow(pAd->TxRing[QID_HCCA].Cell[0].AllocPa);
#endif
	  RTMP_IO_WRITE32(pAd, TX_BASE_PTR4, Value);
	DBGPRINT(RT_DEBUG_TRACE, ("--> TX_BASE_PTR4 : 0x%x\n", Value));

	// Write MGMT_BASE_CSR register
#ifdef WIN_NDIS
	Value = NdisGetPhysicalAddressLow(pAd->MgmtRing.Cell[0].AllocPa);
#else
	Value = RTMP_GetPhysicalAddressLow(pAd->MgmtRing.Cell[0].AllocPa);
#endif
	RTMP_IO_WRITE32(pAd, TX_BASE_PTR5, Value);
	DBGPRINT(RT_DEBUG_TRACE, ("--> TX_BASE_PTR5 : 0x%x\n", Value));

	// Write RX_BASE_CSR register
#ifdef WIN_NDIS
	Value = NdisGetPhysicalAddressLow(pAd->RxRing.Cell[0].AllocPa);
#else
	Value = RTMP_GetPhysicalAddressLow(pAd->RxRing.Cell[0].AllocPa);
#endif		
	RTMP_IO_WRITE32(pAd, RX_BASE_PTR, Value);
	DBGPRINT(RT_DEBUG_TRACE, ("--> RX_BASE_PTR : 0x%x\n", Value));

	// Init RX Ring index pointer
	pAd->RxRing.RxSwReadIdx = 0;
	pAd->RxRing.RxCpuIdx = RX_RING_SIZE-1;
	RTMP_IO_WRITE32(pAd, RX_CRX_IDX, pAd->RxRing.RxCpuIdx);
	
	// Init TX rings index pointer
	{
		for (i=0; i<NUM_OF_TX_RING; i++)
		{
			pAd->TxRing[i].TxSwFreeIdx = 0;
			pAd->TxRing[i].TxCpuIdx = 0;
			RTMP_IO_WRITE32(pAd, (TX_CTX_IDX0 + i * 0x10) ,  pAd->TxRing[i].TxCpuIdx);
		}
	}

	// init MGMT ring index pointer
	pAd->MgmtRing.TxSwFreeIdx = 0;
	pAd->MgmtRing.TxCpuIdx = 0;
	RTMP_IO_WRITE32(pAd, TX_MGMTCTX_IDX,  pAd->MgmtRing.TxCpuIdx);

	//
	// set each Ring's SIZE  into ASIC. Descriptor Size is fixed by design.
	//

	// Write TX_RING_CSR0 register
	Value = TX_RING_SIZE;
	RTMP_IO_WRITE32(pAd, TX_MAX_CNT0, Value);
	RTMP_IO_WRITE32(pAd, TX_MAX_CNT1, Value);
	RTMP_IO_WRITE32(pAd, TX_MAX_CNT2, Value);
	RTMP_IO_WRITE32(pAd, TX_MAX_CNT3, Value);
	RTMP_IO_WRITE32(pAd, TX_MAX_CNT4, Value);
	Value = MGMT_RING_SIZE;
	RTMP_IO_WRITE32(pAd, TX_MGMTMAX_CNT, Value);

	// Write RX_RING_CSR register
	Value = RX_RING_SIZE;
	RTMP_IO_WRITE32(pAd, RX_MAX_CNT, Value);
#endif // RT2860 //


	// WMM parameter
	csr0.word = 0;
	RTMP_IO_WRITE32(pAd, WMM_TXOP0_CFG, csr0.word);
	if (pAd->CommonCfg.PhyMode == PHY_11B)
	{
		csr0.field.Ac0Txop = 192;	// AC_VI: 192*32us ~= 6ms
		csr0.field.Ac1Txop = 96;	// AC_VO: 96*32us  ~= 3ms
	}
	else
	{
		csr0.field.Ac0Txop = 96;	// AC_VI: 96*32us ~= 3ms
		csr0.field.Ac1Txop = 48;	// AC_VO: 48*32us ~= 1.5ms
	}
	RTMP_IO_WRITE32(pAd, WMM_TXOP1_CFG, csr0.word);


#ifdef RT2860
	// 3. Set DMA global configuration except TX_DMA_EN and RX_DMA_EN bits:
	i = 0;
	do
	{
		RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);
		if ((GloCfg.field.TxDMABusy == 0)  && (GloCfg.field.RxDMABusy == 0))
			break;
		
		RTMPusecDelay(1000);
		i++;
	}while ( i < 100);

	GloCfg.word &= 0xff0;
	GloCfg.field.EnTXWriteBackDDONE =1;
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);
	
	IntCfg.word = 0;
	RTMP_IO_WRITE32(pAd, DELAY_INT_CFG, IntCfg.word);
#endif // RT2860 //


	// reset action
	// Load firmware
	//  Status = NICLoadFirmware(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("<-- NICInitializeAdapter\n"));
	return Status;
}

/*
	========================================================================
	
	Routine Description:
		Initialize ASIC

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		
	========================================================================
*/
NDIS_STATUS	NICInitializeAsic(
	IN	PRTMP_ADAPTER	pAd,
	IN  BOOLEAN		bHardReset)
{
	ULONG			Index = 0;
	UCHAR			R0 = 0xff;
	UINT32			MacCsr12 = 0, Counter = 0;
	USHORT			KeyIdx;
	INT				i,apidx;

	DBGPRINT(RT_DEBUG_TRACE, ("--> NICInitializeAsic\n"));

#ifdef RT2860
	if (bHardReset == TRUE)
	{
		RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x3);
	}
	else
		RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x1);

	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x0);
	// Initialize MAC register to default value
	for (Index = 0; Index < NUM_MAC_REG_PARMS; Index++)
	{
		RTMP_IO_WRITE32(pAd, MACRegTable[Index].Register, MACRegTable[Index].Value);
	}

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		for (Index = 0; Index < NUM_AP_MAC_REG_PARMS; Index++)
		{
			RTMP_IO_WRITE32(pAd, APMACRegTable[Index].Register, APMACRegTable[Index].Value);
		}
	}
#endif // CONFIG_AP_SUPPORT //

#endif // RT2860 //


	//
	// Before program BBP, we need to wait BBP/RF get wake up.
	//
	Index = 0;
	do
	{
		RTMP_IO_READ32(pAd, MAC_STATUS_CFG, &MacCsr12);

		if ((MacCsr12 & 0x03) == 0)	// if BB.RF is stable
			break;
		
		DBGPRINT(RT_DEBUG_TRACE, ("Check MAC_STATUS_CFG  = Busy = %x\n", MacCsr12));
		RTMPusecDelay(1000);
	} while (Index++ < 100);

    // The commands to firmware should be after these commands, these commands will init firmware
	// PCI and USB are not the same because PCI driver needs to wait for PCI bus ready
	RTMP_IO_WRITE32(pAd, H2M_BBP_AGENT, 0);	// initialize BBP R/W access agent
	RTMP_IO_WRITE32(pAd, H2M_MAILBOX_CSR, 0);
	RTMPusecDelay(10000);

	// Read BBP register, make sure BBP is up and running before write new data
	Index = 0;
	do 
	{
		RTMP_BBP_IO_READ8_BY_REG_ID(pAd, BBP_R0, &R0);
		DBGPRINT(RT_DEBUG_TRACE, ("BBP version = %x\n", R0));
	} while ((++Index < 20) && ((R0 == 0xff) || (R0 == 0x00)));
	//ASSERT(Index < 20); //this will cause BSOD on Check-build driver

	if ((R0 == 0xff) || (R0 == 0x00))
		return NDIS_STATUS_FAILURE;

	// Initialize BBP register to default value
	for (Index = 0; Index < NUM_BBP_REG_PARMS; Index++)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBPRegTable[Index].Register, BBPRegTable[Index].Value);
	}
	
	// for rt2860E and after, init BBP_R84 with 0x19. This is for extension channel overlapping IOT.
	if ((pAd->MACVersion&0xffff) != 0x0101)
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R84, 0x19);

	if (pAd->MACVersion == 0x28600100)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R69, 0x16);
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R73, 0x12);
    }
	
	if (pAd->MACVersion >= RALINK_2880E_VERSION) // 3*3
	{
		// enlarge MAX_LEN_CFG
		UINT32 csr;
		RTMP_IO_READ32(pAd, MAX_LEN_CFG, &csr);
		csr &= 0xFFF;
		csr |= 0x2000;
		RTMP_IO_WRITE32(pAd, MAX_LEN_CFG, csr);
	}


	// Add radio off control

	// Clear raw counters
	RTMP_IO_READ32(pAd, RX_STA_CNT0, &Counter);
	RTMP_IO_READ32(pAd, RX_STA_CNT1, &Counter);
	RTMP_IO_READ32(pAd, RX_STA_CNT2, &Counter);
	RTMP_IO_READ32(pAd, TX_STA_CNT0, &Counter);
	RTMP_IO_READ32(pAd, TX_STA_CNT1, &Counter);
	RTMP_IO_READ32(pAd, TX_STA_CNT2, &Counter);
	
	// ASIC will keep garbage value after boot
	// Clear all seared key table when initial
	// This routine can be ignored in radio-ON/OFF operation. 
	if (bHardReset)
	{
		for (KeyIdx = 0; KeyIdx < 4; KeyIdx++)
		{
			RTMP_IO_WRITE32(pAd, SHARED_KEY_MODE_BASE + 4*KeyIdx, 0);
		}
	}

	// Clear all pairwise key table when initial
	for (KeyIdx = 0; KeyIdx < 256; KeyIdx++)
	{
		RTMP_IO_WRITE32(pAd, MAC_WCID_ATTRIBUTE_BASE + (KeyIdx * HW_WCID_ATTRI_SIZE), 1);
	}
	
	// assert HOST ready bit
//  RTMP_IO_WRITE32(pAd, MAC_CSR1, 0x0); // 2004-09-14 asked by Mark
//  RTMP_IO_WRITE32(pAd, MAC_CSR1, 0x4);

	// It isn't necessary to clear this space when not hard reset. 	
	if (bHardReset == TRUE)
	{
		// clear all on-chip BEACON frame space			
		for (apidx = 0; apidx < HW_BEACON_MAX_COUNT; apidx++)
		{
			for (i = 0; i < HW_BEACON_OFFSET>>2; i+=4)
				RTMP_IO_WRITE32(pAd, pAd->BeaconOffset[apidx] + i, 0x00); 
#ifdef CONFIG_AP_SUPPORT
#endif // CONFIG_AP_SUPPORT //
		}
	}


	DBGPRINT(RT_DEBUG_TRACE, ("<-- NICInitializeAsic\n"));
	return NDIS_STATUS_SUCCESS;
}

/*
	========================================================================
	
	Routine Description:
		Reset NIC Asics

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		Reset NIC to initial state AS IS system boot up time.
		
	========================================================================
*/
VOID	NICIssueReset(
	IN	PRTMP_ADAPTER	pAd)
{
	UINT32	Value = 0;
	DBGPRINT(RT_DEBUG_TRACE, ("--> NICIssueReset\n"));

	// Abort Tx, prevent ASIC from writing to Host memory
	//RTMP_IO_WRITE32(pAd, TX_CNTL_CSR, 0x001f0000);
	
	// Disable Rx, register value supposed will remain after reset
	RTMP_IO_READ32(pAd, MAC_SYS_CTRL, &Value);
	Value &= (0xfffffff3);
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, Value);

	// Issue reset and clear from reset state
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x03); // 2004-09-17 change from 0x01
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x00);

	DBGPRINT(RT_DEBUG_TRACE, ("<-- NICIssueReset\n"));
}

/*
	========================================================================
	
	Routine Description:
		Check ASIC registers and find any reason the system might hang

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
BOOLEAN	NICCheckForHang(
	IN	PRTMP_ADAPTER	pAd)
{
	return (FALSE);
}

VOID NICUpdateFifoStaCounters(
	IN PRTMP_ADAPTER pAd)
{
	TX_STA_FIFO_STRUC	StaFifo;
	MAC_TABLE_ENTRY		*pEntry;
	UCHAR				i = 0;
	UCHAR			pid = 0, wcid = 0;
	CHAR				reTry;
	UCHAR				succMCS;

#ifdef RALINK_ATE		
	/* Nothing to do in ATE mode */
	if (ATE_ON(pAd))
		return;
#endif // RALINK_ATE //

		do
		{
			RTMP_IO_READ32(pAd, TX_STA_FIFO, &StaFifo.word);

			if (StaFifo.field.bValid == 0)
				break;
		
			wcid = (UCHAR)StaFifo.field.wcid;

#ifdef CONFIG_AP_SUPPORT
			IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
			{
#ifdef DFS_SUPPORT
				/* the CTS frame for DFS and Carrier-Sense. */
				if ((pAd->CommonCfg.Channel > 14)
					&& (pAd->CommonCfg.bIEEE80211H == TRUE)
					&& (pAd->CommonCfg.RadarDetect.RDMode == RD_NORMAL_MODE)
					&& (wcid == DFS_CTS_WCID))
				{
					i++;
					DFSStartTrigger(pAd);
					continue;
				}
#endif // DFS_SUPPORT //

#ifdef CARRIER_DETECTION_SUPPORT
				if ((pAd->CommonCfg.CarrierDetect.Enable == TRUE)
					&& (wcid == CS_CTS_WCID))
				{
					i++;
					CarrierDetectStartTrigger(pAd);
					continue;
				}
#endif // CARRIER_DETECTION_SUPPORT //
			}
#endif // CONFIG_AP_SUPPORT //

		/* ignore NoACK and MGMT frame use 0xFF as WCID */
			if ((StaFifo.field.TxAckRequired == 0) || (wcid >= MAX_LEN_OF_MAC_TABLE))
			{
				i++;
				continue;
			}

			/* PID store Tx MCS Rate */
			pid = (UCHAR)StaFifo.field.PidType;

			pEntry = &pAd->MacTab.Content[wcid];

			pEntry->DebugFIFOCount++;

			if (StaFifo.field.TxBF) // 3*3
				pEntry->TxBFCount++;
			if (!StaFifo.field.TxSuccess)
			{
				pEntry->FIFOCount++;
				pEntry->OneSecTxFailCount++;
				if (pEntry->FIFOCount >= 1)
				{			
					DBGPRINT(RT_DEBUG_TRACE, ("#"));
#if 0
					SendRefreshBAR(pAd, pEntry);
					pEntry->NoBADataCountDown = 64;
#else
					if(pEntry->PsMode == PWR_ACTIVE)
					{
						int tid;
						for (tid=0; tid<8; tid++)
						{
							BAOriSessionTearDown(pAd, pEntry->Aid,  tid, FALSE, FALSE);
						}
					}
					else
					{    
						// Clear the FIFOCount when sta in Power Save mode. Basically we assume 
						//     this tx error happened due to sta just go to sleep.
						pEntry->FIFOCount = 0;
					}
#endif
					//pEntry->FIFOCount = 0;
				}
				//pEntry->bSendBAR = TRUE;
			}
			else
			{
				if ((pEntry->PsMode != PWR_SAVE) && (pEntry->NoBADataCountDown > 0))
				{
					pEntry->NoBADataCountDown--;
					if (pEntry->NoBADataCountDown==0)
					{
						DBGPRINT(RT_DEBUG_TRACE, ("@\n"));
					}
				}
				pEntry->FIFOCount = 0;
				pEntry->OneSecTxNoRetryOkCount++;
				// update NoDataIdleCount when sucessful send packet to STA.
				pEntry->NoDataIdleCount = 0;
			}

			succMCS = StaFifo.field.SuccessRate & 0x7F;

			reTry = pid - succMCS;

			if (StaFifo.field.TxSuccess)
			{
				pEntry->TXMCSExpected[pid]++;
				if (pid == succMCS)
				{
					pEntry->TXMCSSuccessful[pid]++;
				}
				else 
				{
					pEntry->TXMCSAutoFallBack[pid][succMCS]++;
				}
			}
			else
			{
				pEntry->TXMCSFailed[pid]++;
			}

			if (reTry > 0)
			{
				if ((pid >= 12) && succMCS <=7)
				{
					reTry -= 4;
				} 
				pEntry->OneSecTxRetryOkCount += reTry;
			}
			else if (reTry < 0)
			{
				DBGPRINT(RT_DEBUG_INFO, ("(%d): reTry %d , (TxMCS = %d, SuccessRate = %d)\n", 
				wcid, reTry, pid, StaFifo.field.SuccessRate & 0x7F));
			}

			i++;
			// ASIC store 16 stack
		} while ( i < (2*TX_RING_SIZE) );

}

/*
	========================================================================
	
	Routine Description:
		Read statistical counters from hardware registers and record them
		in software variables for later on query

	Arguments:
		pAd					Pointer to our adapter

	Return Value:
		None

	IRQL = DISPATCH_LEVEL
	
	========================================================================
*/
VOID NICUpdateRawCounters(
	IN PRTMP_ADAPTER pAd)
{
	UINT32	OldValue;//, Value2;
	//ULONG	PageSum, OneSecTransmitCount;
	//ULONG	TxErrorRatio, Retry, Fail;
	RX_STA_CNT0_STRUC	 RxStaCnt0;
	RX_STA_CNT1_STRUC   RxStaCnt1;
	RX_STA_CNT2_STRUC   RxStaCnt2;
	TX_STA_CNT0_STRUC 	 TxStaCnt0;
	TX_STA_CNT1_STRUC	 StaTx1;
	TX_STA_CNT2_STRUC	 StaTx2;
	TX_AGG_CNT_STRUC	TxAggCnt;
	TX_AGG_CNT0_STRUC	TxAggCnt0;
	TX_AGG_CNT1_STRUC	TxAggCnt1;
	TX_AGG_CNT2_STRUC	TxAggCnt2;
	TX_AGG_CNT3_STRUC	TxAggCnt3;
	TX_AGG_CNT4_STRUC	TxAggCnt4;
	TX_AGG_CNT5_STRUC	TxAggCnt5;
	TX_AGG_CNT6_STRUC	TxAggCnt6;
	TX_AGG_CNT7_STRUC	TxAggCnt7;

#if 0	
//==================================================================
//Read PBF for emulation debug.
	RTMP_IO_READ32(pAd, PBF_DBG, &OldValue);
	RTMP_IO_READ32(pAd, TXRXQ_PCNT, &Value2);
	PageSum = (OldValue&0xff);
	PageSum += ((Value2&0xff));
	PageSum += ((Value2&0xff00)>>8);
	PageSum += ((Value2&0xff0000)>>16);
	PageSum += ((Value2&0xff000000)>>24);
	
//==================================================================
#endif

	RTMP_IO_READ32(pAd, RX_STA_CNT0, &RxStaCnt0.word);
	RTMP_IO_READ32(pAd, RX_STA_CNT2, &RxStaCnt2.word);

#ifdef CONFIG_AP_SUPPORT
#ifdef CARRIER_DETECTION_SUPPORT
	if ((pAd->CommonCfg.CarrierDetect.Enable == FALSE) || (pAd->OpMode == OPMODE_STA))
#endif // CARRIER_DETECTION_SUPPORT //
#endif // CONFIG_AP_SUPPORT //
	{
		RTMP_IO_READ32(pAd, RX_STA_CNT1, &RxStaCnt1.word);
	    // Update RX PLCP error counter
	    pAd->PrivateInfo.PhyRxErrCnt += RxStaCnt1.field.PlcpErr;
		// Update False CCA counter
		pAd->RalinkCounters.OneSecFalseCCACnt += RxStaCnt1.field.FalseCca;
	}

	// Update FCS counters
	OldValue= pAd->WlanCounters.FCSErrorCount.u.LowPart;
	pAd->WlanCounters.FCSErrorCount.u.LowPart += (RxStaCnt0.field.CrcErr); // >> 7);
	if (pAd->WlanCounters.FCSErrorCount.u.LowPart < OldValue)
		pAd->WlanCounters.FCSErrorCount.u.HighPart++;

	// Add FCS error count to private counters
	pAd->RalinkCounters.OneSecRxFcsErrCnt += RxStaCnt0.field.CrcErr;
	OldValue = pAd->RalinkCounters.RealFcsErrCount.u.LowPart;
	pAd->RalinkCounters.RealFcsErrCount.u.LowPart += RxStaCnt0.field.CrcErr;
	if (pAd->RalinkCounters.RealFcsErrCount.u.LowPart < OldValue)
		pAd->RalinkCounters.RealFcsErrCount.u.HighPart++;

	// Update Duplicate Rcv check
	pAd->RalinkCounters.DuplicateRcv += RxStaCnt2.field.RxDupliCount;
	pAd->WlanCounters.FrameDuplicateCount.u.LowPart += RxStaCnt2.field.RxDupliCount;
	// Update RX Overflow counter
	pAd->Counters8023.RxNoBuffer += (RxStaCnt2.field.RxFifoOverflowCount);
	
	//pAd->RalinkCounters.RxCount = 0;

	
	//if (!OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED) || 
	//	(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_TX_RATE_SWITCH_ENABLED) && (pAd->MacTab.Size != 1)))
	if (!pAd->bUpdateBcnCntDone)
	{
	// Update BEACON sent count
	RTMP_IO_READ32(pAd, TX_STA_CNT0, &TxStaCnt0.word);
	RTMP_IO_READ32(pAd, TX_STA_CNT1, &StaTx1.word);
	RTMP_IO_READ32(pAd, TX_STA_CNT2, &StaTx2.word);
	pAd->RalinkCounters.OneSecBeaconSentCnt += TxStaCnt0.field.TxBeaconCount;
	pAd->RalinkCounters.OneSecTxRetryOkCount += StaTx1.field.TxRetransmit;
	pAd->RalinkCounters.OneSecTxNoRetryOkCount += StaTx1.field.TxSuccess;
	pAd->RalinkCounters.OneSecTxFailCount += TxStaCnt0.field.TxFailCount;
	pAd->WlanCounters.TransmittedFragmentCount.u.LowPart += StaTx1.field.TxSuccess;
	pAd->WlanCounters.RetryCount.u.LowPart += StaTx1.field.TxRetransmit;
	pAd->WlanCounters.FailedCount.u.LowPart += TxStaCnt0.field.TxFailCount;
	}

#if 0	
	Retry = StaTx1.field.TxRetransmit;
	Fail = TxStaCnt0.field.TxFailCount;
	TxErrorRatio = 0;
	OneSecTransmitCount = pAd->WlanCounters.TransmittedFragmentCount.u.LowPart- pAd->WlanCounters.LastTransmittedFragmentCount.u.LowPart;
	if ((OneSecTransmitCount+Retry + Fail) > 0)
		TxErrorRatio = (( Retry + Fail) *100) / (OneSecTransmitCount+Retry + Fail);

	if ((OneSecTransmitCount+Retry + Fail) > 0)
		TxErrorRatio = (( Retry + Fail) *100) / (OneSecTransmitCount+Retry + Fail);
	DBGPRINT(RT_DEBUG_INFO, ("TX ERROR Rate = %ld %%, Retry = %ld, Fail = %ld, Total = %ld  \n",TxErrorRatio, Retry, Fail, (OneSecTransmitCount+Retry + Fail)));
	pAd->WlanCounters.LastTransmittedFragmentCount.u.LowPart = pAd->WlanCounters.TransmittedFragmentCount.u.LowPart;
#endif

	//if (pAd->bStaFifoTest == TRUE)
	{
		RTMP_IO_READ32(pAd, TX_AGG_CNT, &TxAggCnt.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT0, &TxAggCnt0.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT1, &TxAggCnt1.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT2, &TxAggCnt2.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT3, &TxAggCnt3.word);
		RTMP_IO_READ32(pAd, TX_AGG_CNT4, &TxAggCnt4.word);
		RTMP_IO_READ32(pAd, TX_AGG_CNT5, &TxAggCnt5.word);
		RTMP_IO_READ32(pAd, TX_AGG_CNT6, &TxAggCnt6.word);
		RTMP_IO_READ32(pAd, TX_AGG_CNT7, &TxAggCnt7.word);
		pAd->RalinkCounters.TxAggCount += TxAggCnt.field.AggTxCount;
		pAd->RalinkCounters.TxNonAggCount += TxAggCnt.field.NonAggTxCount;
		pAd->RalinkCounters.TxAgg1MPDUCount += TxAggCnt0.field.AggSize1Count;
		pAd->RalinkCounters.TxAgg2MPDUCount += TxAggCnt0.field.AggSize2Count;
		
		pAd->RalinkCounters.TxAgg3MPDUCount += TxAggCnt1.field.AggSize3Count;
		pAd->RalinkCounters.TxAgg4MPDUCount += TxAggCnt1.field.AggSize4Count;
		pAd->RalinkCounters.TxAgg5MPDUCount += TxAggCnt2.field.AggSize5Count;
		pAd->RalinkCounters.TxAgg6MPDUCount += TxAggCnt2.field.AggSize6Count;
	
		pAd->RalinkCounters.TxAgg7MPDUCount += TxAggCnt3.field.AggSize7Count;
		pAd->RalinkCounters.TxAgg8MPDUCount += TxAggCnt3.field.AggSize8Count;
		pAd->RalinkCounters.TxAgg9MPDUCount += TxAggCnt4.field.AggSize9Count;
		pAd->RalinkCounters.TxAgg10MPDUCount += TxAggCnt4.field.AggSize10Count;

		pAd->RalinkCounters.TxAgg11MPDUCount += TxAggCnt5.field.AggSize11Count;
		pAd->RalinkCounters.TxAgg12MPDUCount += TxAggCnt5.field.AggSize12Count;
		pAd->RalinkCounters.TxAgg13MPDUCount += TxAggCnt6.field.AggSize13Count;
		pAd->RalinkCounters.TxAgg14MPDUCount += TxAggCnt6.field.AggSize14Count;

		pAd->RalinkCounters.TxAgg15MPDUCount += TxAggCnt7.field.AggSize15Count;
		pAd->RalinkCounters.TxAgg16MPDUCount += TxAggCnt7.field.AggSize16Count;

		// Calculate the transmitted A-MPDU count
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += TxAggCnt0.field.AggSize1Count;
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt0.field.AggSize2Count / 2);

		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt1.field.AggSize3Count / 3);
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt1.field.AggSize4Count / 4);

		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt2.field.AggSize5Count / 5);
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt2.field.AggSize6Count / 6);

		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt3.field.AggSize7Count / 7);
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt3.field.AggSize8Count / 8);

		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt4.field.AggSize9Count / 9);
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt4.field.AggSize10Count / 10);

		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt5.field.AggSize11Count / 11);
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt5.field.AggSize12Count / 12);

		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt6.field.AggSize13Count / 13);
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt6.field.AggSize14Count / 14);

		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt7.field.AggSize15Count / 15);
		pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart += (TxAggCnt7.field.AggSize16Count / 16);				
	}

#ifdef DBG_DIAGNOSE
	{
		RtmpDiagStruct	*pDiag;
		COUNTER_RALINK	*pRalinkCounters;
		UCHAR			ArrayCurIdx, i;
		
		pDiag = &pAd->DiagStruct;
		pRalinkCounters = &pAd->RalinkCounters;
		ArrayCurIdx = pDiag->ArrayCurIdx;
		
		if (pDiag->inited == 0)
		{
			NdisZeroMemory(pDiag, sizeof(struct _RtmpDiagStrcut_));
			pDiag->ArrayStartIdx = pDiag->ArrayCurIdx = 0;
			pDiag->inited = 1;
		}
		else
		{
			// Tx
			pDiag->TxFailCnt[ArrayCurIdx] = TxStaCnt0.field.TxFailCount;
			pDiag->TxAggCnt[ArrayCurIdx] = TxAggCnt.field.AggTxCount;
			pDiag->TxNonAggCnt[ArrayCurIdx] = TxAggCnt.field.NonAggTxCount;
			pDiag->TxAMPDUCnt[ArrayCurIdx][0] = TxAggCnt0.field.AggSize1Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][1] = TxAggCnt0.field.AggSize2Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][2] = TxAggCnt1.field.AggSize3Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][3] = TxAggCnt1.field.AggSize4Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][4] = TxAggCnt2.field.AggSize5Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][5] = TxAggCnt2.field.AggSize6Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][6] = TxAggCnt3.field.AggSize7Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][7] = TxAggCnt3.field.AggSize8Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][8] = TxAggCnt4.field.AggSize9Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][9] = TxAggCnt4.field.AggSize10Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][10] = TxAggCnt5.field.AggSize11Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][11] = TxAggCnt5.field.AggSize12Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][12] = TxAggCnt6.field.AggSize13Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][13] = TxAggCnt6.field.AggSize14Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][14] = TxAggCnt7.field.AggSize15Count;
			pDiag->TxAMPDUCnt[ArrayCurIdx][15] = TxAggCnt7.field.AggSize16Count;

			pDiag->RxCrcErrCnt[ArrayCurIdx] = RxStaCnt0.field.CrcErr;
			
			INC_RING_INDEX(pDiag->ArrayCurIdx,  DIAGNOSE_TIME);
			ArrayCurIdx = pDiag->ArrayCurIdx;
			for (i =0; i < 9; i++)
			{
				pDiag->TxDescCnt[ArrayCurIdx][i]= 0;
				pDiag->TxSWQueCnt[ArrayCurIdx][i] =0;
				pDiag->TxMcsCnt[ArrayCurIdx][i] = 0;
				pDiag->RxMcsCnt[ArrayCurIdx][i] = 0;
			}
			pDiag->TxDataCnt[ArrayCurIdx] = 0;
			pDiag->TxFailCnt[ArrayCurIdx] = 0;
			pDiag->RxDataCnt[ArrayCurIdx] = 0;
			pDiag->RxCrcErrCnt[ArrayCurIdx]  = 0;
//			for (i = 9; i < 16; i++)
			for (i = 9; i < 24; i++) // 3*3
			{
				pDiag->TxDescCnt[ArrayCurIdx][i] = 0;
				pDiag->TxMcsCnt[ArrayCurIdx][i] = 0;
				pDiag->RxMcsCnt[ArrayCurIdx][i] = 0;
}

			if (pDiag->ArrayCurIdx == pDiag->ArrayStartIdx)
				INC_RING_INDEX(pDiag->ArrayStartIdx,  DIAGNOSE_TIME);
		}
		
	}
#endif // DBG_DIAGNOSE //


}


/*
	========================================================================
	
	Routine Description:
		Reset NIC from error

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		Reset NIC from error state
		
	========================================================================
*/
VOID	NICResetFromError(
	IN	PRTMP_ADAPTER	pAd)
{
	// Reset BBP (according to alex, reset ASIC will force reset BBP
	// Therefore, skip the reset BBP
	// RTMP_IO_WRITE32(pAd, MAC_CSR1, 0x2);
		
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x1);
	// Remove ASIC from reset state
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x0);

	NICInitializeAdapter(pAd, FALSE);	
	NICInitAsicFromEEPROM(pAd);

	// Switch to current channel, since during reset process, the connection should remains on.	
	AsicSwitchChannel(pAd, pAd->CommonCfg.CentralChannel, FALSE);
	AsicLockChannel(pAd, pAd->CommonCfg.CentralChannel);
}

/*
	========================================================================
	
	Routine Description:
		erase 8051 firmware image in MAC ASIC

	Arguments:
		Adapter						Pointer to our adapter

	IRQL = PASSIVE_LEVEL
		
	========================================================================
*/
VOID NICEraseFirmware(
	IN PRTMP_ADAPTER pAd)
{
	ULONG i;

	for(i=0; i<MAX_FIRMWARE_IMAGE_SIZE; i+=4)
		RTMP_IO_WRITE32(pAd, FIRMWARE_IMAGE_BASE + i, 0);

}/* End of NICEraseFirmware */

/*
	========================================================================
	
	Routine Description:
		Load 8051 firmware RT2561.BIN file into MAC ASIC

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS         firmware image load ok
		NDIS_STATUS_FAILURE         image not found

	IRQL = PASSIVE_LEVEL
		
	========================================================================
*/
NDIS_STATUS NICLoadFirmware(
	IN PRTMP_ADAPTER pAd)
{
#ifdef BIN_IN_FILE
#define NICLF_DEFAULT_USE()	\
	flg_default_firm_use = TRUE; \
	printk("%s - Use default firmware!\n", __FUNCTION__);

	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	PUCHAR			src;
	struct file		*srcf;
	INT 			retval, orgfsuid, orgfsgid, i;
   	mm_segment_t	orgfs;
	PUCHAR			pFirmwareImage;
	UINT			FileLength = 0;
	UINT32			MacReg;
	ULONG			Index;
	ULONG			firm;
	BOOLEAN			flg_default_firm_use = FALSE;


	DBGPRINT(RT_DEBUG_TRACE, ("===> %s\n", __FUNCTION__));

	/* init */
	pFirmwareImage = NULL;
	src = RTMP_FIRMWARE_FILE_NAME;

	/* save uid and gid used for filesystem access.
	   set user and group to 0 (root) */
	orgfsuid = current->fsuid;
	orgfsgid = current->fsgid;
	current->fsuid = current->fsgid = 0;
    orgfs = get_fs();
    set_fs(KERNEL_DS);

	pAd->FirmwareVersion = (FIRMWARE_MAJOR_VERSION << 8) + \
						   FIRMWARE_MINOR_VERSION;


	/* allocate firmware buffer */
    pFirmwareImage = kmalloc(MAX_FIRMWARE_IMAGE_SIZE, MEM_ALLOC_FLAG);
    if (pFirmwareImage == NULL)
	{
		/* allocate fail, use default firmware array in firmware.h */
		printk("%s - Allocate memory fail!\n", __FUNCTION__);
		NICLF_DEFAULT_USE();
    }
	else
	{
		/* allocate ok! zero the firmware buffer */
		memset(pFirmwareImage, 0x00, MAX_FIRMWARE_IMAGE_SIZE);
	} /* End of if */


	/* if ok, read firmware file from *.bin file */
	if (flg_default_firm_use == FALSE)
	{
		do
		{
			/* open the bin file */
			srcf = filp_open(src, O_RDONLY, 0);

			if (IS_ERR(srcf)) 
			{
				printk("%s - Error %ld opening %s\n",
					   __FUNCTION__, -PTR_ERR(srcf), src);
				NICLF_DEFAULT_USE();
				break;
			} /* End of if */

			/* the object must have a read method */
			if ((srcf->f_op == NULL) || (srcf->f_op->read == NULL))
			{
				printk("%s - %s does not have a write method\n", __FUNCTION__, src);
				NICLF_DEFAULT_USE();
				break;
			} /* End of if */

			/* read the firmware from the file *.bin */
			FileLength = srcf->f_op->read(srcf,
										  pFirmwareImage,
										  MAX_FIRMWARE_IMAGE_SIZE,
										  &srcf->f_pos);

			if (FileLength != MAX_FIRMWARE_IMAGE_SIZE)
			{
				printk("%s: error file length (=%d) in RT2860AP.BIN\n",
					   __FUNCTION__, FileLength);
				NICLF_DEFAULT_USE();
				break;
			}
			else
			{
				PUCHAR ptr = pFirmwareImage;
				USHORT crc = 0xffff;


				/* calculate firmware CRC */
				for(i=0; i<(MAX_FIRMWARE_IMAGE_SIZE-2); i++, ptr++)
					crc = ByteCRC16(BitReverse(*ptr), crc);
				/* End of for */

				if ((pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-2] != \
								(UCHAR)BitReverse((UCHAR)(crc>>8))) ||
					(pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-1] != \
								(UCHAR)BitReverse((UCHAR)crc)))
				{
					/* CRC fail */
					printk("%s: CRC = 0x%02x 0x%02x "
						   "error, should be 0x%02x 0x%02x\n",
						   __FUNCTION__,
						   pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-2],
						   pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-1],
						   (UCHAR)(crc>>8), (UCHAR)(crc));
					NICLF_DEFAULT_USE();
					break;
				}
				else
				{
					/* firmware is ok */
					pAd->FirmwareVersion = \
						(pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-4] << 8) +
						pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-3];

					/* check if firmware version of the file is too old */
					if ((pAd->FirmwareVersion) < \
											((FIRMWARE_MAJOR_VERSION << 8) +
									  	 	 FIRMWARE_MINOR_VERSION))
					{
						printk("%s: firmware version too old!\n", __FUNCTION__);
						NICLF_DEFAULT_USE();
						break;
					} /* End of if */
				} /* End of if */

				DBGPRINT(RT_DEBUG_TRACE,
						 ("NICLoadFirmware: CRC ok, ver=%d.%d\n",
						  pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-4],
						  pFirmwareImage[MAX_FIRMWARE_IMAGE_SIZE-3]));
			} /* End of if (FileLength == MAX_FIRMWARE_IMAGE_SIZE) */
			break;
		} while(TRUE);

		/* close firmware file */
		if (IS_ERR(srcf))
			;
		else
		{
			retval = filp_close(srcf, NULL);			
			if (retval)
			{
				DBGPRINT(RT_DEBUG_ERROR,
						 ("--> Error %d closing %s\n", -retval, src));
			} /* End of if */
		} /* End of if */
	} /* End of if */


	/* write firmware to ASIC */
	if (flg_default_firm_use == TRUE)
	{
		/* use default fimeware, free allocated buffer */
		if (pFirmwareImage != NULL)
			kfree(pFirmwareImage);
		/* End of if */

		/* use default *.bin array */
		pFirmwareImage = FirmwareImage;
		FileLength = sizeof(FirmwareImage);
	} /* End of if */

	/* enable Host program ram write selection */
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0x10000); 

	for(i=0; i<FileLength; i+=4)
	{
		firm = pFirmwareImage[i] +
			   (pFirmwareImage[i+3] << 24) +
			   (pFirmwareImage[i+2] << 16) +
			   (pFirmwareImage[i+1] << 8);

		RTMP_IO_WRITE32(pAd, FIRMWARE_IMAGE_BASE + i, firm);
	} /* End of for */

	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0x00000);
	RTMP_IO_WRITE32(pAd, PBF_SYS_CTRL, 0x00001);

	/* initialize BBP R/W access agent */
	RTMP_IO_WRITE32(pAd, H2M_BBP_AGENT, 0);
	RTMP_IO_WRITE32(pAd, H2M_MAILBOX_CSR, 0);

	if (flg_default_firm_use == FALSE)
	{
		/* use file firmware, free allocated buffer */
		if (pFirmwareImage != NULL)
			kfree(pFirmwareImage);
		/* End of if */
	} /* End of if */

	set_fs(orgfs);
	current->fsuid = orgfsuid;
	current->fsgid = orgfsgid;
#else

	NDIS_STATUS		Status = NDIS_STATUS_SUCCESS;
	PUCHAR			pFirmwareImage;
	ULONG			FileLength, Index;
	//ULONG			firm;
	UINT32			MacReg = 0;


	pFirmwareImage = FirmwareImage;
	FileLength = sizeof(FirmwareImage);

#if 0
	/* enable Host program ram write selection */
	RT28XX_FIRMUD_INIT(pAd);

	for(i=0; i<FileLength; i+=4)
	{
		firm = pFirmwareImage[i] +
			   (pFirmwareImage[i+3] << 24) +
			   (pFirmwareImage[i+2] << 16) +
			   (pFirmwareImage[i+1] << 8);

		RTMP_IO_WRITE32(pAd, FIRMWARE_IMAGE_BASE + i, firm);
	} /* End of for */

	RT28XX_FIRMUD_END(pAd);
#else
	RT28XX_WRITE_FIRMWARE(pAd, pFirmwareImage, FileLength);
#endif

#endif

	/* check if MCU is ready */
	Index = 0;
	do
	{
		RTMP_IO_READ32(pAd, PBF_SYS_CTRL, &MacReg);

		if (MacReg & 0x80)
			break;
		
		RTMPusecDelay(1000);
	} while (Index++ < 1000);

    if (Index >= 1000)
	{
		Status = NDIS_STATUS_FAILURE;
		DBGPRINT(RT_DEBUG_ERROR, ("NICLoadFirmware: MCU is not ready\n\n\n"));
	} /* End of if */

#if 0
    DBGPRINT(RT_DEBUG_TRACE,
			 ("<=== %s (src=%s, status=%d)\n", __FUNCTION__, src, Status));
#else
    DBGPRINT(RT_DEBUG_TRACE,
			 ("<=== %s (status=%d)\n", __FUNCTION__, Status));
#endif
    return Status;
} /* End of NICLoadFirmware */


/*
	========================================================================
	
	Routine Description:
		Load Tx rate switching parameters

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		NDIS_STATUS_SUCCESS         firmware image load ok
		NDIS_STATUS_FAILURE         image not found

	IRQL = PASSIVE_LEVEL

	Rate Table Format:
		1. (B0: Valid Item number) (B1:Initial item from zero)
		2. Item Number(Dec)      Mode(Hex)     Current MCS(Dec)    TrainUp(Dec)    TrainDown(Dec)
		
	========================================================================
*/
NDIS_STATUS NICLoadRateSwitchingParams(
	IN PRTMP_ADAPTER pAd)
{
#if 0
	NDIS_STATUS Status;

	NDIS_HANDLE FileHandle;
	UINT FileLength = 0, i, j;
	PUCHAR pFirmwareImage;
	NDIS_STRING FileName;
	NDIS_PHYSICAL_ADDRESS HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1);

	DBGPRINT(RT_DEBUG_TRACE,("===> NICLoadRateSwitchingParams \n"));
	pAd->CommonCfg.TxRateTableSize = 0;

	if ((pAd->DeviceID == NIC2860_PCI_DEVICE_ID) || (pAd->DeviceID == NIC2860_PCIe_DEVICE_ID))
	{
		NdisInitializeString(&FileName,"rate.bin");
		DBGPRINT(RT_DEBUG_TRACE, ("NICLoadRateSwitchingParams: load file - rate.bin for tx rate switch \n"));
	}
	else
	{
		DBGPRINT_ERR(("NICLoadRateSwitchingParams: wrong DeviceID = 0x%04x, can't find Tx rate switch parameters file\n", pAd->DeviceID));
		return NDIS_STATUS_SUCCESS;
	}
	NdisOpenFile(&Status, &FileHandle, &FileLength, &FileName, HighestAcceptableMax);
	NdisFreeString(FileName);

	if (Status != NDIS_STATUS_SUCCESS)
	{
		DBGPRINT(RT_DEBUG_ERROR, ("NICLoadRateSwitchingParams: NdisOpenFile() failed, used RateSwitchTable instead\n"));
		return NDIS_STATUS_SUCCESS;
	}

	if ((FileLength == 0) || (FileLength > (MAX_STEP_OF_TX_RATE_SWITCH+1)*16))
	{
		DBGPRINT(RT_DEBUG_ERROR, ("NICLoadRateSwitchingParams: file size is not reasonable, used RateSwitchTable instead\n"));
		
		NdisCloseFile(FileHandle);
		return NDIS_STATUS_SUCCESS;
	}
	else
	{
		//
		// NDIS_STATUS_SUCCESS means 
		// The handle at FileHandle is valid for a subsequent call to NdisMapFile.
		//
		NdisMapFile(&Status, &pFirmwareImage, FileHandle);
		DBGPRINT(RT_DEBUG_TRACE, ("NdisMapFile FileLength=%d\n", FileLength));
	}

	for (i=0, j=0; i<FileLength; i++)
	{
		if ((i%16) <= 4)	// trim reserved field
		{
			if (i%16 == 1)	// deal with DEC and HEX, only row0 is Hex, others are Dec
			{
				RateSwitchTable[j] = *(pFirmwareImage + i);
			}
			else
			{
				RateSwitchTable[j] = (*(pFirmwareImage + i)>>4) * 10 + (*(pFirmwareImage + i) & 0x0F);
			}
			
			j++;
		}
	}

	pAd->CommonCfg.TxRateTableSize = RateSwitchTable[0];		// backup table size

	if (Status == NDIS_STATUS_SUCCESS)
	{
		NdisUnmapFile(FileHandle);
		NdisCloseFile(FileHandle);
	}

	DBGPRINT(RT_DEBUG_TRACE,("<=== NICLoadRateSwitchingParams(Valid TxRateTable item number=%d)\n", pAd->CommonCfg.TxRateTableSize));
#endif
	return NDIS_STATUS_SUCCESS;
}


#if 0
/*
	========================================================================
	
	Routine Description:
		Compare two memory block

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		1:			memory are equal
		0:			memory are not equal

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
ULONG	RTMPEqualMemory(
	IN	PVOID	pSrc1,
	IN	PVOID	pSrc2,
	IN	ULONG	Length)
{
	PUCHAR	pMem1;
	PUCHAR	pMem2;
	ULONG	Index = 0;

	pMem1 = (PUCHAR) pSrc1;
	pMem2 = (PUCHAR) pSrc2;

	for (Index = 0; Index < Length; Index++)
	{
		if (pMem1[Index] != pMem2[Index])
		{
			break;
		}
	}

	if (Index == Length)
	{
		return (1);
	}
	else
	{
		return (0);
	}
}
#endif

/*
	========================================================================
	
	Routine Description:
		if  pSrc1 all zero with length Length, return 0.
		If not all zero, return 1

	Arguments:
		pSrc1						 

	Return Value:
		1:			not all zero
		0:			all zero

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
ULONG	RTMPNotAllZero(
	IN	PVOID	pSrc1,
	IN	ULONG	Length)
{
	PUCHAR	pMem1;
	ULONG	Index = 0;

	pMem1 = (PUCHAR) pSrc1;

	for (Index = 0; Index < Length; Index++)
	{
		if (pMem1[Index] != 0x0)
		{
			break;
		}
	}

	if (Index == Length)
	{
		return (0);
	}
	else
	{
		return (1);
	}
}

/*
	========================================================================
	
	Routine Description:
		Compare two memory block

	Arguments:
		pSrc1		Pointer to first memory address
		pSrc2		Pointer to second memory address
		
	Return Value:
		0:			memory is equal
		1:			pSrc1 memory is larger
		2:			pSrc2 memory is larger

	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
ULONG	RTMPCompareMemory(
	IN	PVOID	pSrc1,
	IN	PVOID	pSrc2,
	IN	ULONG	Length)
{
	PUCHAR	pMem1;
	PUCHAR	pMem2;
	ULONG	Index = 0;

	pMem1 = (PUCHAR) pSrc1;
	pMem2 = (PUCHAR) pSrc2;

	for (Index = 0; Index < Length; Index++)
	{
		if (pMem1[Index] > pMem2[Index])
			return (1);
		else if (pMem1[Index] < pMem2[Index])
			return (2);
	}

	// Equal
	return (0);
}

/*
	========================================================================
	
	Routine Description:
		Zero out memory block

	Arguments:
		pSrc1		Pointer to memory address
		Length		Size

	Return Value:
		None
		
	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
VOID	RTMPZeroMemory(
	IN	PVOID	pSrc,
	IN	ULONG	Length)
{
	PUCHAR	pMem;
	ULONG	Index = 0;

	pMem = (PUCHAR) pSrc;

	for (Index = 0; Index < Length; Index++)
	{
		pMem[Index] = 0x00;
	}
}

VOID	RTMPFillMemory(
	IN	PVOID	pSrc,
	IN	ULONG	Length,
	IN	UCHAR	Fill)
{
	PUCHAR	pMem;
	ULONG	Index = 0;

	pMem = (PUCHAR) pSrc;

	for (Index = 0; Index < Length; Index++)
	{
		pMem[Index] = Fill;
	}
}

/*
	========================================================================
	
	Routine Description:
		Copy data from memory block 1 to memory block 2

	Arguments:
		pDest		Pointer to destination memory address
		pSrc		Pointer to source memory address
		Length		Copy size
		
	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
VOID	RTMPMoveMemory(
	OUT	PVOID	pDest,
	IN	PVOID	pSrc,
	IN	ULONG	Length)
{
	PUCHAR	pMem1;
	PUCHAR	pMem2;
	UINT	Index;

	ASSERT((Length==0) || (pDest && pSrc));

	pMem1 = (PUCHAR) pDest;
	pMem2 = (PUCHAR) pSrc;

	for (Index = 0; Index < Length; Index++)
	{
		pMem1[Index] = pMem2[Index];
	}
}

/*
	========================================================================
	
	Routine Description:
		Initialize port configuration structure

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL

	Note:
		
	========================================================================
*/
VOID	UserCfgInit(
	IN	PRTMP_ADAPTER pAd)
{
#ifdef CONFIG_AP_SUPPORT
	UINT i, j;
#endif // CONFIG_AP_SUPPORT //
//	EDCA_PARM DefaultEdcaParm;
    UINT key_index, bss_index;

	DBGPRINT(RT_DEBUG_TRACE, ("--> UserCfgInit\n"));    
	 
	//
	//  part I. intialize common configuration
	//

    for(key_index=0; key_index<SHARE_KEY_NUM; key_index++)
    {
        for(bss_index = 0; bss_index < MAX_MBSSID_NUM; bss_index++)
        {
            pAd->SharedKey[bss_index][key_index].KeyLen = 0;
            pAd->SharedKey[bss_index][key_index].CipherAlg = CIPHER_NONE;
        } /* End of for */
    } /* End of for */

	pAd->Antenna.word = 0; 
	pAd->CommonCfg.BBPCurrentBW = BW_20;

	pAd->LedCntl.word = 0;
#ifdef RT2860
	pAd->LedIndicatorStregth = 0;
    pAd->RLnkCtrlOffset = 0;
    pAd->HostLnkCtrlOffset = 0;
#endif // RT2860 //

#ifdef DOT11N_DRAFT3
	pAd->CommonCfg.Dot11OBssScanPassiveDwell = dot11OBSSScanPassiveDwell;	// Unit : TU. 5~1000
	pAd->CommonCfg.Dot11OBssScanActiveDwell = dot11OBSSScanActiveDwell;	// Unit : TU. 10~1000
	pAd->CommonCfg.Dot11BssWidthTriggerScanInt = dot11BSSWidthTriggerScanInterval;	// Unit : Second	
	pAd->CommonCfg.Dot11OBssScanPassiveTotalPerChannel = dot11OBSSScanPassiveTotalPerChannel;	// Unit : TU. 200~10000
	pAd->CommonCfg.Dot11OBssScanActiveTotalPerChannel = dot11OBSSScanActiveTotalPerChannel;	// Unit : TU. 20~10000
	pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor = dot11BSSWidthChannelTransactionDelayFactor;
	pAd->CommonCfg.Dot11OBssScanActivityThre = dot11BSSScanActivityThreshold;	// Unit : percentage

	pAd->CommonCfg.Dot11BssWidthChanTranDelay = (pAd->CommonCfg.Dot11BssWidthTriggerScanInt * pAd->CommonCfg.Dot11BssWidthChanTranDelayFactor);
#endif  // DOT11N_DRAFT3 //

	pAd->bAutoTxAgcA = FALSE;			// Default is OFF
	pAd->bAutoTxAgcG = FALSE;			// Default is OFF
	pAd->RfIcType = RFIC_2820;

	// Init timer for reset complete event
	pAd->CommonCfg.CentralChannel = 1;
#ifdef WIN_NDIS
	RTMPInitTimer(pAd, &pAd->ResetCompleteTimer, GET_TIMER_FUNCTION(RTMPResetComplete), pAd, FALSE);
#endif
	pAd->bBroadComHT = FALSE;
	pAd->HTCEnable = FALSE;
	pAd->bForcePrintTX = FALSE;
	pAd->bForcePrintRX = FALSE;
	pAd->bStaFifoTest = FALSE;
	pAd->bProtectionTest = FALSE;
	pAd->bHCCATest = FALSE;
	pAd->bGenOneHCCA = FALSE;
	pAd->CommonCfg.bRdg = FALSE;
	pAd->CommonCfg.Dsifs = 10;      // in units of usec 
	pAd->CommonCfg.TxPower = 100; //mW
	pAd->CommonCfg.TxPowerPercentage = 0xffffffff; // AUTO
	pAd->CommonCfg.TxPowerDefault = 0xffffffff; // AUTO
	pAd->CommonCfg.TxPreamble = Rt802_11PreambleAuto; // use Long preamble on TX by defaut
	pAd->CommonCfg.bUseZeroToDisableFragment = FALSE;
	pAd->CommonCfg.RtsThreshold = 2347;
	pAd->CommonCfg.FragmentThreshold = 2346;
	pAd->CommonCfg.UseBGProtection = 0;    // 0: AUTO
	pAd->CommonCfg.bEnableTxBurst = TRUE; //0;    	
	pAd->CommonCfg.PhyMode = 0xff;     // unknown
	pAd->CommonCfg.BandState = UNKNOWN_BAND;
	pAd->CommonCfg.RadarDetect.CSPeriod = 10;
	pAd->CommonCfg.RadarDetect.CSCount = 0;
	pAd->CommonCfg.RadarDetect.RDMode = RD_NORMAL_MODE;
	pAd->CommonCfg.RadarDetect.ChMovingTime = 65;
	pAd->CommonCfg.RadarDetect.LongPulseRadarTh = 3;
	pAd->CommonCfg.bAPSDCapable = FALSE;
	pAd->CommonCfg.bNeedSendTriggerFrame = FALSE;
	pAd->CommonCfg.TriggerTimerCount = 0;
	pAd->CommonCfg.bAPSDForcePowerSave = FALSE;
	//pAd->CommonCfg.bAutoTxRateSwitch = TRUE;
	pAd->CommonCfg.bCountryFlag = FALSE;
	//pAd->CommonCfg.FixedTxMode	= FIXED_TXMODE_HT;
	pAd->CommonCfg.TxStream = 0;
	pAd->CommonCfg.RxStream = 0;
//	pAd->bPromiscuous = FALSE;

	NdisZeroMemory(&pAd->CommonCfg.HtCapability, sizeof(pAd->CommonCfg.HtCapability));
	NdisZeroMemory(&pAd->CommonCfg.AddHTInfo, sizeof(pAd->CommonCfg.AddHTInfo));
	NdisZeroMemory(&pAd->BeaconTxWI, sizeof(pAd->BeaconTxWI));

	pAd->CommonCfg.BACapability.field.MMPSmode = MMPS_ENABLE;
	pAd->CommonCfg.BACapability.field.MpduDensity = 0;
	pAd->CommonCfg.BACapability.field.Policy = IMMED_BA;
	pAd->CommonCfg.BACapability.field.RxBAWinLimit = 64; //32;
	pAd->CommonCfg.BACapability.field.TxBAWinLimit = 64; //32;
	//pAd->CommonCfg.HTPhyMode.field.BW = BW_20;
	//pAd->CommonCfg.HTPhyMode.field.MCS = MCS_AUTO;
	//pAd->CommonCfg.HTPhyMode.field.ShortGI = GI_800;
	//pAd->CommonCfg.HTPhyMode.field.STBC = STBC_NONE;
	pAd->CommonCfg.TxRate = RATE_6;
	DBGPRINT(RT_DEBUG_TRACE, ("--> UserCfgInit. BACapability = 0x%x\n", pAd->CommonCfg.BACapability.word));    
	
	pAd->CommonCfg.MlmeTransmit.field.MCS = MCS_RATE_6;
	pAd->CommonCfg.MlmeTransmit.field.BW = BW_20;
	pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
	pAd->CommonCfg.BACapability.field.AutoBA = FALSE;	
	BATableInit(pAd, &pAd->BATable);

	pAd->CommonCfg.BeaconPeriod = 100;     // in mSec

	//
	// part II. intialize STA specific configuration
	//

	// global variables mXXXX used in MAC protocol state machines
	OPSTATUS_SET_FLAG(pAd, fOP_STATUS_RECEIVE_DTIM);
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_ADHOC_ON);
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_INFRA_ON);

	// PHY specification
	pAd->CommonCfg.PhyMode = PHY_11BG_MIXED;		// default PHY mode
//  RTMPSetPhyMode(pAd, PHY_11BG_MIXED);   // default in 11BG mixed mode
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SHORT_PREAMBLE_INUSED);  // CCK use LONG preamble


	// Default for extra information is not valid
	pAd->ExtraInfo = EXTRA_INFO_CLEAR;
	
	// Default Config change flag
	pAd->bConfigChanged = FALSE;

	// 
	// part III. AP configurations
	//

#ifdef CONFIG_AP_SUPPORT
	IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
	{
		// Set MBSS Default Configurations
		pAd->ApCfg.BssidNum = MAX_MBSSID_NUM;
		for(j = BSS0; j < pAd->ApCfg.BssidNum; j++)
		{
			pAd->ApCfg.MBSSID[j].AuthMode = Ndis802_11AuthModeOpen;
			pAd->ApCfg.MBSSID[j].WepStatus = Ndis802_11EncryptionDisabled;
			pAd->ApCfg.MBSSID[j].DefaultKeyId = 0;

			pAd->ApCfg.MBSSID[j].IEEE8021X = FALSE;
			pAd->ApCfg.MBSSID[j].PreAuth = FALSE;
			NdisZeroMemory(&pAd->ApCfg.MBSSID[j].PMKIDCache, sizeof(NDIS_AP_802_11_PMKID));

			pAd->ApCfg.MBSSID[j].radius_srv_num = 0;

			/* VLAN related */
        		pAd->ApCfg.MBSSID[j].VLAN_VID = 0;

			// Default MCS as AUTO
			pAd->ApCfg.MBSSID[j].bAutoTxRateSwitch = TRUE;
			pAd->ApCfg.MBSSID[j].DesiredTransmitSetting.field.MCS = MCS_AUTO;

#ifdef WSC_AP_SUPPORT
			pAd->ApCfg.MBSSID[j].WscControl.WscConfMode = 0;
			pAd->ApCfg.MBSSID[j].WscControl.WscMode = 1;
			pAd->ApCfg.MBSSID[j].WscControl.WscConfStatus = 1;
			pAd->ApCfg.MBSSID[j].WscControl.WscStatus = 0;
			pAd->ApCfg.MBSSID[j].WscControl.WscState = 0;
			pAd->ApCfg.MBSSID[j].WscControl.WscPinCode = 0;
			pAd->ApCfg.MBSSID[j].WscControl.WscLastPinFromEnrollee = 0;
			pAd->ApCfg.MBSSID[j].WscControl.WscEnrolleePinCode = 0;
			pAd->ApCfg.MBSSID[j].WscControl.WscSelReg = 0;
			NdisZeroMemory(&pAd->ApCfg.MBSSID[j].WscControl.RegData, sizeof(WSC_REG_DATA));
			pAd->ApCfg.MBSSID[j].WscControl.RegData.ReComputePke = 1;
			pAd->ApCfg.MBSSID[j].WscControl.lastId = 1;
			pAd->ApCfg.MBSSID[j].WscControl.WscUseUPnP = 0;
			pAd->ApCfg.MBSSID[j].WscControl.EntryApIdx = WSC_INIT_ENTRY_APIDX;    
			pAd->ApCfg.MBSSID[j].WscControl.pAd = pAd;
			pAd->ApCfg.MBSSID[j].WscControl.WscRejectSamePinFromEnrollee = FALSE;
			pAd->ApCfg.MBSSID[j].WscControl.WscPBCOverlap= FALSE;
			NdisZeroMemory(&pAd->ApCfg.MBSSID[j].WscControl.WscStaPbcProbeInfo, sizeof(WSC_STA_PBC_PROBE_INFO));
#endif // WSC_AP_SUPPORT //        

			for(i = 0; i < WLAN_MAX_NUM_OF_TIM; i++)
	        		pAd->ApCfg.MBSSID[j].TimBitmaps[i] = 0;
		}
		pAd->ApCfg.DtimCount  = 0;
		pAd->ApCfg.DtimPeriod = DEFAULT_DTIM_PERIOD;

		pAd->ApCfg.PMKCachePeriod = (10 * 60 * HZ); // unit : tick(default: 10 minute)

		pAd->ApCfg.ErpIeContent = 0;

#ifdef IDS_SUPPORT
		// Default disable IDS threshold and reset all IDS counters
		pAd->ApCfg.IdsEnable = FALSE;
		pAd->ApCfg.AuthFloodThreshold = 0;
		pAd->ApCfg.AssocReqFloodThreshold = 0;
		pAd->ApCfg.ReassocReqFloodThreshold = 0;
		pAd->ApCfg.ProbeReqFloodThreshold = 0;
		pAd->ApCfg.DisassocFloodThreshold = 0;
		pAd->ApCfg.DeauthFloodThreshold = 0;
		pAd->ApCfg.EapReqFooldThreshold = 0;
		RTMPClearAllIdsCounter(pAd);
#endif // IDS_SUPPORT //

#ifdef WDS_SUPPORT
		APWdsInitialize(pAd);
#endif // WDS_SUPPORT

#ifdef WSC_AP_SUPPORT
		pAd->WriteWscCfgToDatFile = FALSE;
#endif // WSC_AP_SUPPORT //

#ifdef APCLI_SUPPORT
		for(j = 0; j < MAX_APCLI_NUM; j++) 
		{
			pAd->ApCfg.ApCliTab[j].bAutoTxRateSwitch = TRUE;
			pAd->ApCfg.ApCliTab[j].DesiredTransmitSetting.field.MCS = MCS_AUTO;
		}
#endif // APCLI_SUPPORT //
	}
#endif // CONFIG_AP_SUPPORT //

#ifdef ETH_CONVERT_SUPPORT
	if (pAd->OpMode == OPMODE_STA)
	{
		NdisZeroMemory(pAd->EthConvert.EthCloneMac, MAC_ADDR_LEN);
		pAd->EthConvert.ECMode = ETH_CONVERT_MODE_DISABLE;
		pAd->EthConvert.CloneMacVaild = FALSE;
		//pAd->EthConvert.nodeCount = 0;
		NdisZeroMemory(pAd->EthConvert.SSIDStr, MAX_LEN_OF_SSID);
		pAd->EthConvert.SSIDStrLen = 0;
		pAd->EthConvert.macAutoLearn = FALSE;
	}
#endif // ETH_CONVERT_SUPPORT //

	//
	// part IV. others
	//
	// dynamic BBP R66:sensibity tuning to overcome background noise
	pAd->BbpTuning.bEnable                = TRUE;  
	pAd->BbpTuning.FalseCcaLowerThreshold = 100;
	pAd->BbpTuning.FalseCcaUpperThreshold = 512;
	pAd->BbpTuning.R66Delta               = 4;
	pAd->Mlme.bEnableAutoAntennaCheck = TRUE;
	
	//
	// Also initial R66CurrentValue, RTUSBResumeMsduTransmission might use this value.
	// if not initial this value, the default value will be 0.
	//
	pAd->BbpTuning.R66CurrentValue = 0x38;

	pAd->Bbp94 = BBPR94_DEFAULT;
	pAd->BbpForCCK = FALSE;
	
	// Default is FALSE for test bit 1
	//pAd->bTest1 = FALSE;
	
	// initialize MAC table and allocate spin lock
	NdisZeroMemory(&pAd->MacTab, sizeof(MAC_TABLE));
	InitializeQueueHeader(&pAd->MacTab.McastPsQueue);
	NdisAllocateSpinLock(&pAd->MacTabLock);

	//RTMPInitTimer(pAd, &pAd->RECBATimer, RECBATimerTimeout, pAd, TRUE);
	//RTMPSetTimer(&pAd->RECBATimer, REORDER_EXEC_INTV);

#ifdef RALINK_ATE
	NdisZeroMemory(&pAd->ate, sizeof(ATE_INFO));
	pAd->ate.Mode = ATE_STOP;
	pAd->ate.TxCount = 200;/* to exceed TX_RING_SIZE ... */
	pAd->ate.TxLength = 1024;
	pAd->ate.TxWI.ShortGI = 0;// LONG GI : 800 ns
	pAd->ate.TxWI.PHYMODE = MODE_CCK;
	pAd->ate.TxWI.MCS = 3;
	pAd->ate.TxWI.BW = BW_20;
	pAd->ate.Channel = 1;
	pAd->ate.QID = QID_AC_BE;
	pAd->ate.Addr1[0] = 0x00;
	pAd->ate.Addr1[1] = 0x11;
	pAd->ate.Addr1[2] = 0x22;
	pAd->ate.Addr1[3] = 0xAA;
	pAd->ate.Addr1[4] = 0xBB;
	pAd->ate.Addr1[5] = 0xCC;
	NdisMoveMemory(pAd->ate.Addr2, pAd->ate.Addr1, ETH_LENGTH_OF_ADDRESS);
	NdisMoveMemory(pAd->ate.Addr3, pAd->ate.Addr1, ETH_LENGTH_OF_ADDRESS);
	pAd->ate.bRxFer = 0;
	pAd->ate.bQATxStart = FALSE;
	pAd->ate.bQARxStart = FALSE;
#ifdef RT2860 
	pAd->ate.bFWLoading = FALSE;
#endif // RT2860 //
#ifdef RALINK_28xx_QA
	//pAd->ate.Repeat = 0;
	pAd->ate.TxStatus = 0;
	pAd->ate.AtePid = 0;
#endif // RALINK_28xx_QA //
#endif // RALINK_ATE //


	pAd->CommonCfg.bExtChannelSwitchAnnouncement = 1;
	pAd->CommonCfg.bHTProtect = 1;
	pAd->CommonCfg.bMIMOPSEnable = TRUE;
	pAd->CommonCfg.bBADecline = FALSE;
	pAd->CommonCfg.bDisableReordering = FALSE;

	pAd->CommonCfg.TxBASize = 7;

    pAd->CommonCfg.REGBACapability.word = pAd->CommonCfg.BACapability.word;
	pAd->CommonCfg.bWiFiTest = FALSE;
#ifdef RT2860    
    pAd->bPCIclkOff = FALSE;
#endif // RT2860 //
	DBGPRINT(RT_DEBUG_TRACE, ("<-- UserCfgInit\n"));
}

// IRQL = PASSIVE_LEVEL
UCHAR BtoH(char ch)
{
	if (ch >= '0' && ch <= '9') return (ch - '0');        // Handle numerals
	if (ch >= 'A' && ch <= 'F') return (ch - 'A' + 0xA);  // Handle capitol hex digits
	if (ch >= 'a' && ch <= 'f') return (ch - 'a' + 0xA);  // Handle small hex digits
	return(255);
}

//
//  FUNCTION: AtoH(char *, UCHAR *, int)
//
//  PURPOSE:  Converts ascii string to network order hex
//
//  PARAMETERS:
//    src    - pointer to input ascii string
//    dest   - pointer to output hex
//    destlen - size of dest
//
//  COMMENTS:
//
//    2 ascii bytes make a hex byte so must put 1st ascii byte of pair
//    into upper nibble and 2nd ascii byte of pair into lower nibble.
//
// IRQL = PASSIVE_LEVEL

void AtoH(char * src, UCHAR * dest, int destlen)
{
	char * srcptr;
	PUCHAR destTemp;

	srcptr = src;	
	destTemp = (PUCHAR) dest; 

	while(destlen--)
	{
		*destTemp = BtoH(*srcptr++) << 4;    // Put 1st ascii byte in upper nibble.
		*destTemp += BtoH(*srcptr++);      // Add 2nd ascii byte to above.
		destTemp++;
	}
}

VOID	RTMPPatchMacBbpBug(
	IN	PRTMP_ADAPTER	pAd)
{
	ULONG	Index;

	// Initialize BBP register to default value
	for (Index = 0; Index < NUM_BBP_REG_PARMS; Index++)
	{
		RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBPRegTable[Index].Register, (UCHAR)BBPRegTable[Index].Value);
	}
	
	// Initialize RF register to default value
	AsicSwitchChannel(pAd, pAd->CommonCfg.Channel, FALSE);
	AsicLockChannel(pAd, pAd->CommonCfg.Channel);

	// Re-init BBP register from EEPROM value
	NICInitAsicFromEEPROM(pAd);
}

#ifdef WIN_NDIS
// Patch to enable cardbus controller pre-fetch mode
//
// IRQL = PASSIVE_LEVEL
VOID RTMPPatchCardBus(
	IN	PRTMP_ADAPTER	pAdapter)
{
	USHORT	Bus;
	USHORT	Slot;
	USHORT	Func;
	ULONG	Configuration;
	ULONG	Vendor;
	ULONG	Device;
	ULONG	Class;
	ULONG	Interrupt;
	ULONG	PciLantency;
	ULONG	CacheLine;
	ULONG	CardbusLantency;
	ULONG	SubBus;
	ULONG	CardBus;

	DBGPRINT(RT_DEBUG_TRACE, ("RTMPPatchCardBus-->\n"));
	// Try to find the Cardbus controller we connected to.
	// We should not touch other Cardbus controller which we did not use
	// The cardbus controller should have bus number smaller than our device
	// We should skip Bus 0 & 1, since it is for PCI & VGA chipset.
	// Cardbus controller should start from bus = 2.
	for (Bus = 0; Bus < MAX_PCI_BUS; Bus++)
	{
		for (Slot = 0; Slot < MAX_PCI_DEVICE; Slot++)
		{
			for (Func = 0; Func < MAX_FUNC_NUM; Func++)
			{
				Vendor    = RTMPReadCBConfig(Bus, Slot, Func, 0x00) & 0x0000FFFF;
				// Skip non-exist deice right away
				if (Vendor == UNKNOWN)
					continue;
				
				Device    = RTMPReadCBConfig(Bus, Slot, Func, 0x00) >> 16;
				Interrupt = RTMPReadCBConfig(Bus, Slot, Func, 0x3c) & 0x000000FF;
				Class     = RTMPReadCBConfig(Bus, Slot, Func, 0x08) >> 16;
				
				// First of all, filter non-cardbus bridge and not matching INT line device
				// Ralink ASIC has to be modified for latency timer and cache line size too.
				if (Interrupt != pAdapter->InterruptLevel)
					continue;
				// Check the class for cardbus bridge, otherwise check RALINK device	
				if (Class != CARD_BRIDGE_CLASS)
				{
					if (Vendor != RALINK)
						continue;
					else
					{
						// Found Ralink device before cardbus bridge indicate this is not in cardbus system
						// We should stop scanning and patch right away
						DBGPRINT(RT_DEBUG_TRACE, ("This indicate this is not cardbus device!\n"));
						return;
					}
				}
				
				// Second, Modify Latency timer and cache line size to 0x40, 0x08 respectly
				Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0x0c);
				CacheLine   = Configuration & 0x000000ff;
				PciLantency = (Configuration & 0x0000ff00) >> 8;
				if (CacheLine == 0)
					Configuration = (Configuration & 0xffffff00) | 0x08;
				if (PciLantency == 0)
					Configuration = (Configuration & 0xffff00ff) | 0xa800;
					
				RTMPWriteCBConfig(Bus, Slot, Func, 0x0c, Configuration);
				DBGPRINT(RT_DEBUG_TRACE, ("Configuration 0x0c is %x\n", Configuration));

				// Read Subordinate bus number for our device, it must be there
				Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0x18);
				SubBus  = (Configuration & 0x00ff0000) >> 16;
				CardBus = (Configuration & 0x0000ff00) >> 8;
				
				// Second check for prefecth bit modification requirement
				switch (Vendor)
				{
					case	RICOH:
						// Do prefectch configuration
						// Config register
						Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0x80);
						Configuration |= 0x01;
						RTMPWriteCBConfig(Bus, Slot, Func, 0x80, Configuration);
						DBGPRINT(RT_DEBUG_TRACE, ("RICOH 0x80 is %x\n", Configuration));
						break;
					
					case	O2MICRO:
						// Do prefectch configuration
						Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0x94);
						// Do prefectch configuration
						Configuration |= 0x02;
						RTMPWriteCBConfig(Bus, Slot, Func, 0x94, Configuration);
						DBGPRINT(RT_DEBUG_TRACE, ("O2MICRO 6933 0x94 is %x\n", Configuration));
						// Do unknown 6912 patch
						Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0xd4);
						Configuration |= 0x02;
						RTMPWriteCBConfig(Bus, Slot, Func, 0xd4, Configuration);
						DBGPRINT(RT_DEBUG_TRACE, ("O2MICRO 6912 0xd4 is %x\n", Configuration));
						break;
						
					case	TI:
						// Do prefectch configuration
						// Config register
						Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0x18);
						CardbusLantency = (Configuration & 0xff000000) >> 24;
						if (CardbusLantency == 0)
						{
							Configuration = (Configuration & 0x00ffffffff) | 0xb0000000;
							RTMPWriteCBConfig(Bus, Slot, Func, 0x18, Configuration);
							DBGPRINT(RT_DEBUG_TRACE, ("TI 0x18 is %x\n", Configuration));
						}
						break;
						
					case	ENE:
						// Do patch for ENE cardbus issue found on ASUS notebook
						// Change offset address 0xCB to 0x1f
						Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0xC8);
						Configuration = (Configuration & 0x00ffffff) | 0x1f000000;
						RTMPWriteCBConfig(Bus, Slot, Func, 0xC8, Configuration);
						DBGPRINT(RT_DEBUG_TRACE, ("ENE 0xc8 is %x\n", Configuration));
						//
						// ENE need to path with following code by Max 2005/07
						//
						Configuration = RTMPReadCBConfig(Bus, Slot, Func, 0x80);
						Configuration |= 0x01;
						RTMPWriteCBConfig(Bus, Slot, Func, 0x80, Configuration);
						DBGPRINT(RT_DEBUG_TRACE, ("ENE 0x80 is %x\n", Configuration));
					default:
						break;
				}
				// Go to config ralink device after setup cardbus controller
				RTMPPatchRalinkCardBus(pAdapter, SubBus);
				RTMPPatchRalinkCardBus(pAdapter, CardBus);
			}
		}
	}
	DBGPRINT(RT_DEBUG_TRACE, ("<---RTMPPatchCardBus\n"));
}

// IRQL = PASSIVE_LEVEL
VOID RTMPPatchRalinkCardBus(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	ULONG			Bus)
{
	USHORT	Slot;
	ULONG	Configuration;
	ULONG	Vendor;
	ULONG	PciLantency;
	ULONG	CacheLine;
	
	for (Slot = 0; Slot < MAX_PCI_DEVICE; Slot++)
	{
		// There is only one function for Ralink device which is 0
		Vendor	  = RTMPReadCBConfig(Bus, Slot, 0, 0x00) & 0x0000FFFF;
		if (Vendor != RALINK)
			continue;
		// Second, Modify Latency timer and cache line size to 0x40, 0x08 respectly
		Configuration = RTMPReadCBConfig(Bus, Slot, 0, 0x0c);
		CacheLine	= Configuration & 0x000000ff;
		PciLantency = (Configuration & 0x0000ff00) >> 8;
		if (CacheLine == 0)
			Configuration = (Configuration & 0xffffff00) | 0x08;
		if (PciLantency == 0)
			Configuration = (Configuration & 0xffff00ff) | 0xa800;
				
		RTMPWriteCBConfig(Bus, Slot, 0, 0x0c, Configuration);
		DBGPRINT(RT_DEBUG_TRACE, ("Patch Ralink Cardbus 0x0c is %x\n", Configuration));
	}
}

//
// This routine will read a long (32 bits) from configuration registers.
//
// IRQL = PASSIVE_LEVEL
ULONG RTMPReadCBConfig(
	IN	ULONG	Bus,
	IN	ULONG	Slot,
	IN	ULONG	Func,
	IN	ULONG	Offset)
{
	ULONG	Value;
	ULONG	ConfigAddr;

	ConfigAddr = (Bus << 16) | (Slot << 11) | (Func << 8) | Offset | 0x80000000L;
	
	NdisRawWritePortUlong(PCI_CFG_ADDR_PORT, ConfigAddr);
	RTMPusecDelay(10);
	NdisRawReadPortUlong (PCI_CFG_DATA_PORT, &Value);

	return(Value);
}

//
// This routine will write a long (32 bits) from configuration registers.
//
// IRQL = PASSIVE_LEVEL
VOID RTMPWriteCBConfig(
	IN	ULONG	Bus,
	IN	ULONG	Slot,
	IN	ULONG	Func,
	IN	ULONG	Offset,
	IN	ULONG	Value)
{
	ULONG	ConfigAddr;

	ConfigAddr = (Bus << 16) | (Slot << 11) | (Func << 8) | Offset | 0x80000000L;
	
	NdisRawWritePortUlong(PCI_CFG_ADDR_PORT, ConfigAddr);
	RTMPusecDelay(10);
	NdisRawWritePortUlong(PCI_CFG_DATA_PORT, Value);
}
#endif 
/*
	========================================================================
	
	Routine Description:
		Init timer objects

	Arguments:
		pAd			Pointer to our adapter
		pTimer				Timer structure
		pTimerFunc			Function to execute when timer expired
		Repeat				Ture for period timer

	Return Value:
		None

	Note:
		
	========================================================================
*/
VOID	RTMPInitTimer(
	IN	PRTMP_ADAPTER			pAd,
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	PVOID					pTimerFunc,
	IN	PVOID					pData,
	IN	BOOLEAN					Repeat)
{
	//
	// Set Valid to TRUE for later used.
	// It will crash if we cancel a timer or set a timer 
	// that we haven't initialize before.
	// 
	pTimer->Valid      = TRUE;
	
	pTimer->PeriodicType = Repeat;
	pTimer->State      = FALSE;
	pTimer->cookie = (ULONG) pData;


#ifdef WIN_NDIS
	NdisMInitializeTimer(&pTimer->TimerObj,
		pAd->AdapterHandle,
		pTimerFunc,
		pTimer->cookie);
#else
	RTMP_OS_Init_Timer(pAd,	&pTimer->TimerObj,	pTimerFunc, (PVOID) pTimer);
#endif
	
}

/*
	========================================================================
	
	Routine Description:
		Init timer objects

	Arguments:
		pTimer				Timer structure
		Value				Timer value in milliseconds

	Return Value:
		None

	Note:
		To use this routine, must call RTMPInitTimer before.
		
	========================================================================
*/
VOID	RTMPSetTimer(
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	ULONG					Value)
{
	if (pTimer->Valid)
	{
		pTimer->TimerValue = Value;
		pTimer->State      = FALSE;
		if (pTimer->PeriodicType == TRUE)
		{
			pTimer->Repeat = TRUE;
#ifdef WIN_NDIS
			NdisMSetPeriodicTimer(&pTimer->TimerObj, Value);
#else
			RTMP_SetPeriodicTimer(&pTimer->TimerObj, Value);
#endif
		}
		else
		{
			pTimer->Repeat = FALSE;
#ifdef WIN_NDIS
			NdisMSetTimer(&pTimer->TimerObj, Value);
#else
			RTMP_OS_Add_Timer(&pTimer->TimerObj, Value);
#endif
		}
	}
	else
	{
		DBGPRINT_ERR(("RTMPSetTimer failed, Timer hasn't been initialize!\n"));
	}
}


/*
	========================================================================
	
	Routine Description:
		Init timer objects

	Arguments:
		pTimer				Timer structure
		Value				Timer value in milliseconds

	Return Value:
		None

	Note:
		To use this routine, must call RTMPInitTimer before.
		
	========================================================================
*/
VOID	RTMPModTimer(
	IN	PRALINK_TIMER_STRUCT	pTimer,
	IN	ULONG					Value)
{
	BOOLEAN	Cancel;

	if (pTimer->Valid)
	{
		pTimer->TimerValue = Value;
		pTimer->State      = FALSE;
		if (pTimer->PeriodicType == TRUE)
		{
			RTMPCancelTimer(pTimer, &Cancel);
			RTMPSetTimer(pTimer, Value);
		}
		else
		{
			RTMP_OS_Mod_Timer(&pTimer->TimerObj, Value);
		}
	}
	else
	{
		DBGPRINT_ERR(("RTMPModTimer failed, Timer hasn't been initialize!\n"));
	}
}

/*
	========================================================================
	
	Routine Description:
		Cancel timer objects

	Arguments:
		Adapter						Pointer to our adapter

	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
		1.) To use this routine, must call RTMPInitTimer before.
		2.) Reset NIC to initial state AS IS system boot up time.
		
	========================================================================
*/
VOID	RTMPCancelTimer(
	IN	PRALINK_TIMER_STRUCT	pTimer,
	OUT	BOOLEAN					*pCancelled)
{
	if (pTimer->Valid)
	{
		if (pTimer->State == FALSE)
			pTimer->Repeat = FALSE;
#ifdef WIN_NDIS
			NdisMCancelTimer(&pTimer->TimerObj, pCancelled);
#else
			RTMP_OS_Del_Timer(&pTimer->TimerObj, pCancelled);
#endif
		if (*pCancelled == TRUE)
			pTimer->State = TRUE;

	}
	else
	{
		//
		// NdisMCancelTimer just canced the timer and not mean release the timer.
		// And don't set the "Valid" to False. So that we can use this timer again.	
		//
		DBGPRINT_ERR(("RTMPCancelTimer failed, Timer hasn't been initialize!\n"));
	}
}

/*
	========================================================================
	
	Routine Description:
		Set LED Status

	Arguments:
		pAd						Pointer to our adapter
		Status					LED Status

	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL
	
	Note:
		
	========================================================================
*/
VOID RTMPSetLED(
	IN PRTMP_ADAPTER 	pAd, 
	IN UCHAR			Status)
{
	//ULONG			data;
	UCHAR			HighByte = 0;
	UCHAR			LowByte;

// In ATE mode of RT2860 AP/STA, we have erased 8051 firmware.
// So LED mode is not supported when ATE is running.
#ifdef RALINK_ATE
	if (ATE_ON(pAd))
		return;
#endif // RALINK_ATE //

	LowByte = pAd->LedCntl.field.LedMode&0x7f;
	switch (Status)
	{
		case LED_LINK_DOWN:
			HighByte = 0x20;
			AsicSendCommandToMcu(pAd, 0x50, 0xff, LowByte, HighByte);
			pAd->LedIndicatorStregth = 0; 
			break;
		case LED_LINK_UP:
			if (pAd->CommonCfg.Channel > 14)
				HighByte = 0xa0;
			else
				HighByte = 0x60;
			AsicSendCommandToMcu(pAd, 0x50, 0xff, LowByte, HighByte);
			break;
		case LED_RADIO_ON:
			HighByte = 0x20;
			AsicSendCommandToMcu(pAd, 0x50, 0xff, LowByte, HighByte);
			break;
		case LED_HALT: 
			LowByte = 0; // Driver sets MAC register and MAC controls LED
		case LED_RADIO_OFF:
			HighByte = 0;
			AsicSendCommandToMcu(pAd, 0x50, 0xff, LowByte, HighByte);
			break;
        case LED_WPS:
			HighByte = 0x10;
			AsicSendCommandToMcu(pAd, 0x50, 0xff, LowByte, HighByte);
			break;
		case LED_ON_SITE_SURVEY:
			HighByte = 0x08;
			AsicSendCommandToMcu(pAd, 0x50, 0xff, LowByte, HighByte);
			break;
		case LED_POWER_UP:
			HighByte = 0x04;
			AsicSendCommandToMcu(pAd, 0x50, 0xff, LowByte, HighByte);
			break;
		default:
			DBGPRINT(RT_DEBUG_WARN, ("RTMPSetLED::Unknown Status %d\n", Status));
			break;
	}

    //
	// Keep LED status for LED SiteSurvey mode.
	// After SiteSurvey, we will set the LED mode to previous status.
	//
	if ((Status != LED_ON_SITE_SURVEY) && (Status != LED_POWER_UP))
		pAd->LedStatus = Status;
    
	DBGPRINT(RT_DEBUG_TRACE, ("RTMPSetLED::Mode=%d,HighByte=0x%02x,LowByte=0x%02x\n", pAd->LedCntl.field.LedMode, HighByte, LowByte));
}

/*
	========================================================================
	
	Routine Description:
		Set LED Signal Stregth 

	Arguments:
		pAd						Pointer to our adapter
		Dbm						Signal Stregth

	Return Value:
		None

	IRQL = PASSIVE_LEVEL
	
	Note:
		Can be run on any IRQL level. 

		According to Microsoft Zero Config Wireless Signal Stregth definition as belows.
		<= -90  No Signal
		<= -81  Very Low
		<= -71  Low
		<= -67  Good
		<= -57  Very Good
		 > -57  Excellent		
	========================================================================
*/
VOID RTMPSetSignalLED(
	IN PRTMP_ADAPTER 	pAd, 
	IN NDIS_802_11_RSSI Dbm)
{
	UCHAR		nLed = 0;

	//
	// if not Signal Stregth, then do nothing.
	//
	if (pAd->LedCntl.field.LedMode != LED_MODE_SIGNAL_STREGTH)
	{
		return;
	}

	if (Dbm <= -90)
		nLed = 0;
	else if (Dbm <= -81)
		nLed = 1;
	else if (Dbm <= -71)
		nLed = 3;
	else if (Dbm <= -67)
		nLed = 7;
	else if (Dbm <= -57)
		nLed = 15;
	else 
		nLed = 31;

	//
	// Update Signal Stregth to firmware if changed.
	//
	if (pAd->LedIndicatorStregth != nLed)
	{
		AsicSendCommandToMcu(pAd, 0x51, 0xff, nLed, pAd->LedCntl.field.Polarity);
		pAd->LedIndicatorStregth = nLed;
	}
}

/*
	========================================================================
	
	Routine Description:
		Enable RX 

	Arguments:
		pAd						Pointer to our adapter

	Return Value:
		None

	IRQL <= DISPATCH_LEVEL
	
	Note:
		Before Enable RX, make sure you have enabled Interrupt.
	========================================================================
*/
VOID RTMPEnableRxTx(
	IN PRTMP_ADAPTER	pAd)
{
//	WPDMA_GLO_CFG_STRUC	GloCfg;
//	ULONG	i = 0;

	DBGPRINT(RT_DEBUG_TRACE, ("==> RTMPEnableRxTx\n"));

#if 0
	// Enable Rx DMA.
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0x4);
	do
	{
		RTMP_IO_READ32(pAd, WPDMA_GLO_CFG, &GloCfg.word);
		if ((GloCfg.field.TxDMABusy == 0)  && (GloCfg.field.RxDMABusy == 0))
			break;
		
		DBGPRINT(RT_DEBUG_TRACE, ("==>  DMABusy\n"));
		RTMPusecDelay(1000);
		i++;
	}while ( i <200);

	RTMPusecDelay(50);
	RT28XX_DMA_WRITE_INIT(GloCfg);
	DBGPRINT(RT_DEBUG_TRACE, ("<== WRITE DMA offset 0x208 = 0x%x\n", GloCfg.word));	
	RTMP_IO_WRITE32(pAd, WPDMA_GLO_CFG, GloCfg.word);

	RT28XX_DMA_POST_WRITE(pAd);
#else
	// Enable Rx DMA.
	RT28XXDMAEnable(pAd);
#endif

	// enable RX of MAC block
	if (pAd->OpMode == OPMODE_AP)
	{
		UINT32 rx_filter_flag = APNORMAL;

#ifdef CONFIG_AP_SUPPORT
#ifdef IDS_SUPPORT
		IF_DEV_CONFIG_OPMODE_ON_AP(pAd)
		{
			if (pAd->ApCfg.IdsEnable)
				rx_filter_flag &= (~0x4);	// Don't drop those not-U2M frames
		}
#endif // IDS_SUPPORT //			
#endif // CONFIG_AP_SUPPORT //
				
		RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, rx_filter_flag);     // enable RX of DMA block
	}
	else
	{
		RTMP_IO_WRITE32(pAd, RX_FILTR_CFG, STANORMAL);     // Staion not drop control frame will fail WiFi Certification.
	}
	
	RTMP_IO_WRITE32(pAd, MAC_SYS_CTRL, 0xc);
	DBGPRINT(RT_DEBUG_TRACE, ("<== RTMPEnableRxTx\n"));	
}


