/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2005, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attempt
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    rtmp_info.c

    Abstract:
    IOCTL related subroutines

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Rory Chen   01-03-2003    created
	Rory Chen   02-14-2005    modify to support RT61
*/

#include "rt_config.h"
#include "rtmp.h"
#include <linux/wireless.h>

#define A_BAND_REGION_0				0
#define A_BAND_REGION_1				1
#define A_BAND_REGION_2				2
#define A_BAND_REGION_3				3
#define A_BAND_REGION_4				4
#define A_BAND_REGION_5				5
#define A_BAND_REGION_6				6
#define A_BAND_REGION_7				7
#define A_BAND_REGION_8				8
#define A_BAND_REGION_9				9
#define A_BAND_REGION_10			10

#define G_BAND_REGION_0				0
#define G_BAND_REGION_1				1
#define G_BAND_REGION_2				2
#define G_BAND_REGION_3				3
#define G_BAND_REGION_4				4
#define G_BAND_REGION_5				5
#define G_BAND_REGION_6				6

COUNTRY_CODE_TO_COUNTRY_REGION allCountry[] = {
	/* {Country Number, ISO Name, Country Name, Support 11A, 11A Country Region, Support 11G, 11G Country Region} */
	{0,		"DB",	"Debug",				TRUE,	A_BAND_REGION_7,	TRUE,	G_BAND_REGION_5},
	{8,		"AL",	"ALBANIA",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{12,	"DZ",	"ALGERIA",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{32,	"AR",	"ARGENTINA",			TRUE,	A_BAND_REGION_3,	TRUE,	G_BAND_REGION_1},
	{51,	"AM",	"ARMENIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{36,	"AU",	"AUSTRALIA",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{40,	"AT",	"AUSTRIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{31,	"AZ",	"AZERBAIJAN",			TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{48,	"BH",	"BAHRAIN",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{112,	"BY",	"BELARUS",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{56,	"BE",	"BELGIUM",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{84,	"BZ",	"BELIZE",				TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{68,	"BO",	"BOLIVIA",				TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{76,	"BR",	"BRAZIL",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{96,	"BN",	"BRUNEI DARUSSALAM",	TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{100,	"BG",	"BULGARIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{124,	"CA",	"CANADA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{152,	"CL",	"CHILE",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{156,	"CN",	"CHINA",				TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{170,	"CO",	"COLOMBIA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{188,	"CR",	"COSTA RICA",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{191,	"HR",	"CROATIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{196,	"CY",	"CYPRUS",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{203,	"CZ",	"CZECH REPUBLIC",		TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{208,	"DK",	"DENMARK",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{214,	"DO",	"DOMINICAN REPUBLIC",	TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{218,	"EC",	"ECUADOR",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{818,	"EG",	"EGYPT",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{222,	"SV",	"EL SALVADOR",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{233,	"EE",	"ESTONIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{246,	"FI",	"FINLAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{250,	"FR",	"FRANCE",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{268,	"GE",	"GEORGIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{276,	"DE",	"GERMANY",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{300,	"GR",	"GREECE",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{320,	"GT",	"GUATEMALA",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{340,	"HN",	"HONDURAS",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{344,	"HK",	"HONG KONG",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{348,	"HU",	"HUNGARY",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{352,	"IS",	"ICELAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{356,	"IN",	"INDIA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{360,	"ID",	"INDONESIA",			TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{364,	"IR",	"IRAN",					TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{372,	"IE",	"IRELAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{376,	"IL",	"ISRAEL",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{380,	"IT",	"ITALY",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{392,	"JP",	"JAPAN",				TRUE,	A_BAND_REGION_9,	TRUE,	G_BAND_REGION_1},
	{400,	"JO",	"JORDAN",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{398,	"KZ",	"KAZAKHSTAN",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{408,	"KP",	"KOREA DEMOCRATIC PEOPLE'S REPUBLIC OF",TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{410,	"KR",	"KOREA REPUBLIC OF",	TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{414,	"KW",	"KUWAIT",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{428,	"LV",	"LATVIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{422,	"LB",	"LEBANON",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{438,	"LI",	"LIECHTENSTEIN",		TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{440,	"LT",	"LITHUANIA",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{442,	"LU",	"LUXEMBOURG",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{446,	"MO",	"MACAU",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{807,	"MK",	"MACEDONIA",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{458,	"MY",	"MALAYSIA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{484,	"MX",	"MEXICO",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{492,	"MC",	"MONACO",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{504,	"MA",	"MOROCCO",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{528,	"NL",	"NETHERLANDS",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{554,	"NZ",	"NEW ZEALAND",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{578,	"NO",	"NORWAY",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{512,	"OM",	"OMAN",					TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{586,	"PK",	"PAKISTAN",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{591,	"PA",	"PANAMA",				TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{604,	"PE",	"PERU",					TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{608,	"PH",	"PHILIPPINES",			TRUE,	A_BAND_REGION_4,	TRUE,	G_BAND_REGION_1},
	{616,	"PL",	"POLAND",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{620,	"PT",	"PORTUGAL",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{630,	"PR",	"PUERTO RICO",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{634,	"QA",	"QATAR",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{642,	"RO",	"ROMANIA",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{643,	"RU",	"RUSSIA FEDERATION",	FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{682,	"SA",	"SAUDI ARABIA",			FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{702,	"SG",	"SINGAPORE",			TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{703,	"SK",	"SLOVAKIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{705,	"SI",	"SLOVENIA",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{710,	"ZA",	"SOUTH AFRICA",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{724,	"ES",	"SPAIN",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{752,	"SE",	"SWEDEN",				TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{756,	"CH",	"SWITZERLAND",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{760,	"SY",	"SYRIAN ARAB REPUBLIC",	FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{158,	"TW",	"TAIWAN",				TRUE,	A_BAND_REGION_3,	TRUE,	G_BAND_REGION_0},
	{764,	"TH",	"THAILAND",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{780,	"TT",	"TRINIDAD AND TOBAGO",	TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{788,	"TN",	"TUNISIA",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{792,	"TR",	"TURKEY",				TRUE,	A_BAND_REGION_2,	TRUE,	G_BAND_REGION_1},
	{804,	"UA",	"UKRAINE",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{784,	"AE",	"UNITED ARAB EMIRATES",	FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{826,	"GB",	"UNITED KINGDOM",		TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_1},
	{840,	"US",	"UNITED STATES",		TRUE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_0},
	{858,	"UY",	"URUGUAY",				TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{860,	"UZ",	"UZBEKISTAN",			TRUE,	A_BAND_REGION_1,	TRUE,	G_BAND_REGION_0},
	{862,	"VE",	"VENEZUELA",			TRUE,	A_BAND_REGION_5,	TRUE,	G_BAND_REGION_1},
	{704,	"VN",	"VIET NAM",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{887,	"YE",	"YEMEN",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{716,	"ZW",	"ZIMBABWE",				FALSE,	A_BAND_REGION_0,	TRUE,	G_BAND_REGION_1},
	{999,	"",	"",	0,	0,	0,	0}
};

#define NUM_OF_COUNTRIES	(sizeof(allCountry)/sizeof(COUNTRY_CODE_TO_COUNTRY_REGION))

// Dennis Lee move into rtmp_info.h
// Ralink defined OIDs
#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE                              0x8BE0
#endif
#define SIOCIWFIRSTPRIV								SIOCDEVPRIVATE
#endif

#define RT_PRIV_IOCTL								(SIOCIWFIRSTPRIV + 0x01)
#define RTPRIV_IOCTL_SET							(SIOCIWFIRSTPRIV + 0x02)
				
#ifdef DBG
#define RTPRIV_IOCTL_BBP                            (SIOCIWFIRSTPRIV + 0x03)
#define RTPRIV_IOCTL_MAC                            (SIOCIWFIRSTPRIV + 0x05)
#define RTPRIV_IOCTL_E2P                            (SIOCIWFIRSTPRIV + 0x07)
#endif

#ifdef RALINK_ATE
#ifdef RALINK_28xx_QA
#define RTPRIV_IOCTL_ATE							(SIOCIWFIRSTPRIV + 0x08)
#endif // RALINK_28xx_QA //
#endif // RALINK_ATE //

#define RTPRIV_IOCTL_STATISTICS                     (SIOCIWFIRSTPRIV + 0x09)
#define RTPRIV_IOCTL_ADD_PMKID_CACHE                (SIOCIWFIRSTPRIV + 0x0A)
#define RTPRIV_IOCTL_RADIUS_DATA                    (SIOCIWFIRSTPRIV + 0x0C)
#define RTPRIV_IOCTL_GSITESURVEY					(SIOCIWFIRSTPRIV + 0x0D)
#define RTPRIV_IOCTL_ADD_WPA_KEY                    (SIOCIWFIRSTPRIV + 0x0E)
#define RTPRIV_IOCTL_GET_MAC_TABLE					(SIOCIWFIRSTPRIV + 0x0F)
#define RTPRIV_IOCTL_STATIC_WEP_COPY                (SIOCIWFIRSTPRIV + 0x10)

#define RTPRIV_IOCTL_SHOW							(SIOCIWFIRSTPRIV + 0x11)
#define RTPRIV_IOCTL_WSC_PROFILE                    (SIOCIWFIRSTPRIV + 0x12)
#define RTPRIV_IOCTL_QUERY_BATABLE                  (SIOCIWFIRSTPRIV + 0x16)

#define OID_GET_SET_TOGGLE							0x8000

#define RT_QUERY_ATE_TXDONE_COUNT					0x0401
#define RT_QUERY_SIGNAL_CONTEXT						0x0402
#define RT_SET_IAPP_PID                 			0x0404
#define RT_SET_APD_PID								0x0405
#define RT_SET_DEL_MAC_ENTRY						0x0406

INT Set_CountryString_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT Set_CountryCode_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_ChGeography_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT Set_AP_SSID_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_TxRate_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT	Set_OLBCDetection_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

#ifdef IAPP_SUPPORT
INT	Set_IappPID_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif // IAPP_SUPPORT //

INT Set_AP_AuthMode_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AP_EncrypType_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AP_DefaultKeyID_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AP_Key1_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AP_Key2_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AP_Key3_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AP_Key4_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_AP_WPAPSK_Proc(
    IN  PRTMP_ADAPTER   pAdapter, 
    IN  PUCHAR          arg);

INT Set_BasicRate_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_BeaconPeriod_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_DtimPeriod_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_NoForwarding_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_NoForwardingBTNSSID_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_AP_WmmCapable_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_HideSSID_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_IEEE8021X_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_CSPeriod_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_PreAuth_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_VLANID_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_VLANPriority_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_AccessPolicy_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_AccessControlList_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT	Set_RadioOn_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg);

INT Set_SiteSurvey_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_BADecline_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Show_MacTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
	
INT	Show_DriverInfo_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Show_BaTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Show_Sat_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

#ifdef DBG_DIAGNOSE
INT Set_DiagOpt_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT Show_Diag_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif // DBG_DAIGNOSE //

INT	Show_Sat_Reset_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Show_MATTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
	
VOID RTMPIoctlAddPMKIDCache(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq);

VOID RTMPIoctlStaticWepCopy(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq);

VOID RTMPIoctlQueryRadiusConf(
	IN PRTMP_ADAPTER pAd, 
	IN struct iwreq *wrq);

VOID RTMPIoctlRadiusData(
	IN PRTMP_ADAPTER	pAd, 
	IN struct iwreq		*wrq);

VOID RTMPIoctlAddWPAKey(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq);

#ifdef DBG
VOID RTMPAPIoctlBBP(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  struct iwreq    *wrq);

VOID RTMPAPIoctlMAC(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  struct iwreq    *wrq);

VOID RTMPAPIoctlE2PROM(
    IN  PRTMP_ADAPTER   pAdapter,
    IN  struct iwreq    *wrq);
#endif

static VOID RTMPIoctlStatistics(
	IN PRTMP_ADAPTER pAd, 
	IN struct iwreq *wrq);

INT	Set_DisassociateSta_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);


VOID RTMPIoctlQueryBaTable(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq);
	
#ifdef APCLI_SUPPORT
INT Set_ApCli_Enable_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_Ssid_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_Bssid_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_DefaultKeyID_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_WPAPSK_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_Key1_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_Key2_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_Key3_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_ApCli_Key4_Proc(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
#ifdef WSC_AP_SUPPORT
INT Set_AP_WscSsid_Proc(IN PRTMP_ADAPTER	pAd, IN	PUCHAR arg);
#endif // WSC_AP_SUPPORT //
#endif // APCLI_SUPPORT //
#ifdef UAPSD_AP_SUPPORT
INT Set_UAPSD_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif // UAPSD_AP_SUPPORT //

#ifdef WSC_AP_SUPPORT
INT	Set_WscStatus_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_WscOOB_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_WscStop_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

VOID RTMPIoctlWscProfile(
	IN PRTMP_ADAPTER pAdapter, 
	IN struct iwreq *wrq);

BOOLEAN WscCheckEnrolleeNonceFromUpnp(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	UCHAR	        *pData,
	IN  USHORT			Length,
	IN  PWSC_CTRL       pWscControl);

UCHAR	WscRxMsgTypeFromUpnp(
	IN	PRTMP_ADAPTER		pAdapter,
	IN  PUCHAR				pData,
	IN	USHORT				Length);

INT	    WscGetConfForUpnp(
	IN	PRTMP_ADAPTER	pAd,
	IN  PWSC_CTRL       pWscControl);

INT	Set_AP_WscConfMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_AP_WscConfStatus_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_AP_WscMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_AP_WscGetConf_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);

INT	Set_AP_WscPinCode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg);
#endif // WSC_AP_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
#ifdef MCAST_RATE_SPECIFIC
INT Set_McastPhyMode(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Set_McastMcs(IN PRTMP_ADAPTER pAd, IN PUCHAR arg);
INT Show_McastRate(IN PRTMP_ADAPTER	pAd, IN PUCHAR arg);
#endif // MCAST_RATE_SPECIFIC //
#endif // CONFIG_AP_SUPPORT //


// Dennis Lee --
struct iw_priv_args ap_privtab[] = {
{ RTPRIV_IOCTL_SET, 
  IW_PRIV_TYPE_CHAR | 1024, 0,
  "set"},  
{ RTPRIV_IOCTL_SHOW,
  IW_PRIV_TYPE_CHAR | 1024, 0,
  "show"},
{ RTPRIV_IOCTL_GSITESURVEY,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_site_survey"}, 
{ RTPRIV_IOCTL_GET_MAC_TABLE,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_mac_table"}, 
#ifdef DBG
{ RTPRIV_IOCTL_BBP,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "bbp"},
{ RTPRIV_IOCTL_MAC,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "mac"},
{ RTPRIV_IOCTL_E2P,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "e2p"},
#endif
#ifdef WSC_AP_SUPPORT
{ RTPRIV_IOCTL_WSC_PROFILE,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_wsc_profile"},
#endif // WSC_AP_SUPPORT //
{ RTPRIV_IOCTL_QUERY_BATABLE,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024 ,
  "get_ba_table"},
{ RTPRIV_IOCTL_STATISTICS,
  IW_PRIV_TYPE_CHAR | 1024, IW_PRIV_TYPE_CHAR | 1024,
  "stat"}
};

static struct {
	CHAR *name;
	INT (*set_proc)(PRTMP_ADAPTER pAdapter, PUCHAR arg);
} *PRTMP_PRIVATE_SET_PROC, RTMP_PRIVATE_SUPPORT_PROC[] = {
	{"DriverVersion",				Set_DriverVersion_Proc},
	{"CountryRegion",				Set_CountryRegion_Proc},
	{"CountryRegionABand",			Set_CountryRegionABand_Proc},
	{"CountryString",				Set_CountryString_Proc},
	{"CountryCode",				Set_CountryCode_Proc},
	{"ChGeography",				Set_ChGeography_Proc},
	{"SSID",						Set_AP_SSID_Proc},
	{"WirelessMode",				Set_WirelessMode_Proc},
	{"TxRate",					Set_TxRate_Proc},
	{"BasicRate",					Set_BasicRate_Proc},
	{"ShortSlot",					Set_ShortSlot_Proc},
	{"Channel",					Set_Channel_Proc},
	{"BeaconPeriod",				Set_BeaconPeriod_Proc},
	{"DtimPeriod",					Set_DtimPeriod_Proc},
	{"TxPower",					Set_TxPower_Proc},
	{"BGProtection",				Set_BGProtection_Proc},
	{"DisableOLBC", 				Set_OLBCDetection_Proc},
	{"TxPreamble",				Set_TxPreamble_Proc},
	{"RTSThreshold",				Set_RTSThreshold_Proc},
	{"FragThreshold",				Set_FragThreshold_Proc},
	{"TxBurst",					Set_TxBurst_Proc},
	{"BASetup",					Set_BASetup_Proc},
	{"BADecline",					Set_BADecline_Proc},
	{"SendMIMOPS",				Set_SendPSMPAction_Proc},
	{"BAOriTearDown",				Set_BAOriTearDown_Proc},
	{"BARecTearDown",				Set_BARecTearDown_Proc},
	{"HtBw",						Set_HtBw_Proc},
	{"HtMcs",						Set_HtMcs_Proc},
	{"HtGi",						Set_HtGi_Proc},
	{"HtOpMode",					Set_HtOpMode_Proc},
	{"HtStbc",					Set_HtStbc_Proc},
	{"HtHtc",						Set_HtHtc_Proc},
	{"HtExtcha",					Set_HtExtcha_Proc},
	{"HtMpduDensity",				Set_HtMpduDensity_Proc},
	{"HtBaWinSize",				Set_HtBaWinSize_Proc},
	{"HtMIMOPS",					Set_HtMIMOPSmode_Proc},
	{"HtRdg",						Set_HtRdg_Proc},
	{"HtLinkAdapt",				Set_HtLinkAdapt_Proc},
	{"HtAmsdu",					Set_HtAmsdu_Proc},
	{"HtAutoBa",					Set_HtAutoBa_Proc},
	{"HtProtect",					Set_HtProtect_Proc},
	{"HtMimoPs",					Set_HtMimoPs_Proc},
	{"HtTxStream",				Set_HtTxStream_Proc},
	{"HtRxStream",				Set_HtRxStream_Proc},
	{"ForceShortGI",				Set_ForceShortGI_Proc},
	{"ForceGF",		        		Set_ForceGF_Proc},
	{"HtTxBASize",					Set_HtTxBASize_Proc},

#ifdef IAPP_SUPPORT
	{"IappPID",					Set_IappPID_Proc},
#endif // IAPP_SUPPORT //

#ifdef AGGREGATION_SUPPORT
	{"PktAggregate",				Set_PktAggregate_Proc},
#endif

#ifdef WMM_SUPPORT
	{"WmmCapable",				Set_AP_WmmCapable_Proc},
#endif
	{"NoForwarding",				Set_NoForwarding_Proc},
	{"NoForwardingBTNBSSID",		Set_NoForwardingBTNSSID_Proc},
	{"HideSSID",					Set_HideSSID_Proc},
	{"IEEE8021X",					Set_IEEE8021X_Proc},
	{"IEEE80211H",				Set_IEEE80211H_Proc},
	{"CSPeriod",					Set_CSPeriod_Proc},
	{"PreAuth",					Set_PreAuth_Proc},
	{"VLANID",					Set_VLANID_Proc},
	{"VLANPriority",				Set_VLANPriority_Proc},
	{"AuthMode",					Set_AP_AuthMode_Proc},
	{"EncrypType",				Set_AP_EncrypType_Proc},
	{"DefaultKeyID",				Set_AP_DefaultKeyID_Proc},
	{"Key1",						Set_AP_Key1_Proc},
	{"Key2",						Set_AP_Key2_Proc},
	{"Key3",						Set_AP_Key3_Proc},
	{"Key4",						Set_AP_Key4_Proc},
	{"AccessPolicy",				Set_AccessPolicy_Proc},
	{"AccessControlList",			Set_AccessControlList_Proc},
	{"WPAPSK",					Set_AP_WPAPSK_Proc},
	{"RadioOn",					Set_RadioOn_Proc},
	{"SiteSurvey",					Set_SiteSurvey_Proc},
	{"ResetCounter",				Set_ResetStatCounter_Proc},
	{"DisassociateSta",				Set_DisassociateSta_Proc},
#ifdef DBG	
	{"Debug",					Set_Debug_Proc},
#endif
#ifdef RALINK_ATE
	{"ATE",						Set_ATE_Proc},
	{"ATEDA",					Set_ATE_DA_Proc},
	{"ATESA",						Set_ATE_SA_Proc},
	{"ATEBSSID",					Set_ATE_BSSID_Proc},
	{"ATECHANNEL",				Set_ATE_CHANNEL_Proc},
	{"ATETXPOW0",				Set_ATE_TX_POWER0_Proc},
	{"ATETXPOW1",				Set_ATE_TX_POWER1_Proc},
	{"ATETXANT",					Set_ATE_TX_Antenna_Proc},
	{"ATERXANT",					Set_ATE_RX_Antenna_Proc},
	{"ATETXFREQOFFSET",			Set_ATE_TX_FREQOFFSET_Proc},
	{"ATETXBW",					Set_ATE_TX_BW_Proc},
	{"ATETXLEN",					Set_ATE_TX_LENGTH_Proc},
	{"ATETXCNT",					Set_ATE_TX_COUNT_Proc},
	{"ATETXMCS",					Set_ATE_TX_MCS_Proc},
	{"ATETXMODE",				Set_ATE_TX_MODE_Proc},
	{"ATETXGI",					Set_ATE_TX_GI_Proc},
	{"ATERXFER",					Set_ATE_RX_FER_Proc},
	{"ATERRF",					Set_ATE_Read_RF_Proc},
	{"ATEWRF1",					Set_ATE_Write_RF1_Proc},
	{"ATEWRF2",					Set_ATE_Write_RF2_Proc},
	{"ATEWRF3",					Set_ATE_Write_RF3_Proc},
	{"ATEWRF4",					Set_ATE_Write_RF4_Proc},
	{"ATELDE2P",					Set_ATE_Load_E2P_Proc},
	{"ATERE2P",						Set_ATE_Read_E2P_Proc},
	{"ATESHOW",					Set_ATE_Show_Proc},
	{"ATEHELP",					Set_ATE_Help_Proc},

#ifdef RALINK_28xx_QA
	{"TxStop",					Set_TxStop_Proc},
	{"RxStop",					Set_RxStop_Proc},
#endif // RALINK_28xx_QA //
#endif // RALINK_ATE //

#ifdef APCLI_SUPPORT
	{"ApCliEnable",				Set_ApCli_Enable_Proc},
	{"ApCliSsid",					Set_ApCli_Ssid_Proc},
	{"ApCliBssid",					Set_ApCli_Bssid_Proc},
	{"ApCliAuthMode",				Set_ApCli_AuthMode_Proc},
	{"ApCliEncrypType",				Set_ApCli_EncrypType_Proc},
	{"ApCliDefaultKeyID",			Set_ApCli_DefaultKeyID_Proc},	
	{"ApCliWPAPSK",				Set_ApCli_WPAPSK_Proc},
	{"ApCliKey1",					Set_ApCli_Key1_Proc},
	{"ApCliKey2",					Set_ApCli_Key2_Proc},
	{"ApCliKey3",					Set_ApCli_Key3_Proc},
	{"ApCliKey4",					Set_ApCli_Key4_Proc},
#ifdef WSC_AP_SUPPORT	
	{"ApCliWscSsid",				Set_AP_WscSsid_Proc},
#endif // WSC_AP_SUPPORTs //
#endif	// APCLI_SUPPORT //
#ifdef WSC_AP_SUPPORT
	{"WscConfMode",				Set_AP_WscConfMode_Proc},
	{"WscConfStatus",				Set_AP_WscConfStatus_Proc},
	{"WscMode",					Set_AP_WscMode_Proc},
	{"WscStatus",					Set_WscStatus_Proc},
	{"WscGetConf",				Set_AP_WscGetConf_Proc},
	{"WscPinCode",				Set_AP_WscPinCode_Proc},
	{"WscOOB",                      		Set_WscOOB_Proc},
	{"WscStop",                     		Set_WscStop_Proc},
	{"WscGenPinCode",               		Set_WscGenPinCode_Proc},
	{"WscVendorPinCode",            Set_WscVendorPinCode_Proc},
#endif // WSC_AP_SUPPORT //
#ifdef UAPSD_AP_SUPPORT
	{"UAPSDCapable",				Set_UAPSD_Proc},
#endif // UAPSD_AP_SUPPORT //
#ifdef IGMP_SNOOP_SUPPORT
	{"IgmpSnEnable",				Set_IgmpSn_Enable_Proc},
	{"IgmpAdd",					Set_IgmpSn_AddEntry_Proc},
	{"IgmpDel",					Set_IgmpSn_DelEntry_Proc},
#endif // IGMP_SNOOP_SUPPORT //
#ifdef CONFIG_AP_SUPPORT
	{"FastDfs",					Set_FastDfs_Proc},
	{"ChMovTime",					Set_ChMovingTime_Proc},
	{"LPRadarTh",					Set_LongPulseRadarTh_Proc},
#ifdef CARRIER_DETECTION_SUPPORT
	{"CarrierDetect",				Set_CarrierDetect_Proc},
#endif // CARRIER_DETECTION_SUPPORT //
#ifdef MCAST_RATE_SPECIFIC
	{"McastPhyMode",				Set_McastPhyMode},
	{"McastMcs",					Set_McastMcs},
#endif // MCAST_RATE_SPECIFIC //
#endif // CONFIG_AP_SUPPORT //
#ifdef CONFIG_APSTA_MIXED_SUPPORT
	{"OpMode",					Set_OpMode_Proc},
#endif // CONFIG_APSTA_MIXED_SUPPORT //
#ifdef MESH_SUPPORT
	{"MeshId",						Set_MeshId_Proc},
	{"MeshHostName",				Set_MeshHostName_Proc},
	{"MeshAutoLink",				Set_MeshAutoLink_Proc},
	{"MeshAddLink",					Set_MeshAddLink_Proc},
	{"MeshDelLink",					Set_MeshDelLink_Proc},	
	{"MeshMultiCastAgeOut",			Set_MeshMultiCastAgeOut_Proc},
	{"MeshAuthMode",				Set_MeshAuthMode_Proc},
	{"MeshEncrypType",				Set_MeshEncrypType_Proc},
	{"MeshDefaultkey",				Set_MeshDefaultkey_Proc},
	{"MeshWEPKEY",					Set_MeshWEPKEY_Proc},
	{"MeshWPAKEY",					Set_MeshWPAKEY_Proc},	
#endif // MESH_SUPPORT //
#ifdef DBG_DIAGNOSE
	{"DiagOpt",					Set_DiagOpt_Proc},
#endif // DBG_DIAGNOSE //
	{NULL,}
};


static struct {
	CHAR *name;
	INT (*set_proc)(PRTMP_ADAPTER pAdapter, PUCHAR arg);
} *PRTMP_PRIVATE_SHOW_PROC, RTMP_PRIVATE_SHOW_SUPPORT_PROC[] = {
	{"stainfo",			Show_MacTable_Proc},
	{"descinfo",			Show_DescInfo_Proc},
	{"driverinfo", 			Show_DriverInfo_Proc},
#ifdef WDS_SUPPORT
	{"wdsinfo",				Show_WdsTable_Proc},
#endif // WDS_SUPPORT //	
        {"bainfo",				Show_BaTable_Proc},
	{"stat",				Show_Sat_Proc}, 
#ifdef DBG_DIAGNOSE
	{"diag",				Show_Diag_Proc},
#endif // DBG_DIAGNOSE //
	{"stat_reset",			Show_Sat_Reset_Proc},
#ifdef IGMP_SNOOP_SUPPORT
	{"igmpinfo",			Set_IgmpSn_TabDisplay_Proc},
#endif // IGMP_SNOOP_SUPPORT //
#ifdef MCAST_RATE_SPECIFIC
	{"mcastrate",			Show_McastRate},
#endif // MCAST_RATE_SPECIFIC //
#ifdef MAT_SUPPORT
	{"matinfo",			Show_MATTable_Proc},
#endif // MAT_SUPPORT //
#ifdef MESH_SUPPORT
	{"meshinfo",			Set_MeshInfo_Display_Proc},
	{"neighinfo",			Set_NeighborInfo_Display_Proc},
	{"meshrouteinfo",		Set_MeshRouteInfo_Display_Proc},
	{"meshentryinfo",		Set_MeshEntryInfo_Display_Proc},
	{"multipathinfo",		Set_MultipathInfo_Display_Proc},
	{"multicastageoutinfo",	Set_MultiCastAgeOut_Display_Proc},
	{"pktsiginfo",			Set_PktSig_Display_Proc},
#endif // MESH_SUPPORT //
	{NULL,}
};

#ifdef CONFIG_APSTA_MIXED_SUPPORT
const struct iw_handler_def rt28xx_ap_iw_handler_def =
{
#define	N(a)	(sizeof (a) / sizeof (a[0]))
	.private_args	= (struct iw_priv_args *) ap_privtab,
	.num_private_args	= N(ap_privtab),
#if IW_HANDLER_VERSION >= 7
	.get_wireless_stats = rt28xx_get_wireless_stats,
#endif 
};
#endif // CONFIG_APSTA_MIXED_SUPPORT //

INT RTMPAPSetInformation(
	IN	PRTMP_ADAPTER pAd,
	IN	OUT	struct ifreq	*rq,
	IN	INT					cmd)
{
	struct iwreq						*wrq = (struct iwreq *) rq;
	UCHAR						        Addr[MAC_ADDR_LEN];
	INT									Status = NDIS_STATUS_SUCCESS;

#ifdef SNMP_SUPPORT	
	//snmp
    UINT						KeyIdx = 0;
    PNDIS_AP_802_11_KEY			pKey = NULL;
	TX_RTY_CFG_STRUC			tx_rty_cfg;
	ULONG						ShortRetryLimit, LongRetryLimit;
	UCHAR						ctmp;
#endif //snmp	

	
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	switch(cmd & 0x7FFF)
    {
#ifdef IAPP_SUPPORT
    	case RT_SET_IAPP_PID:
    		if (copy_from_user(&pObj->IappPid, wrq->u.data.pointer, wrq->u.data.length))
			{
				Status = -EFAULT; 	
			}
    		else
    		{
    			DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_APD_PID::(IappPid=%lu)\n", pObj->IappPid));
    		}
			break;
#endif // IAPP_SUPPORT //

    	case RT_SET_APD_PID:
    		if (copy_from_user(&pObj->apd_pid, wrq->u.data.pointer, wrq->u.data.length))
			{
				Status = -EFAULT; 	
			}
    		else
    		{
    			DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_APD_PID::(ApdPid=%lu)\n", pObj->apd_pid));
    		}
			break;
		case RT_SET_DEL_MAC_ENTRY:
    		if (copy_from_user(Addr, wrq->u.data.pointer, wrq->u.data.length))
			{
				Status = -EFAULT; 	
			}
    		else
    		{
				UCHAR HashIdx;
				MAC_TABLE_ENTRY *pEntry = NULL;
				
    			DBGPRINT(RT_DEBUG_TRACE, ("RT_SET_DEL_MAC_ENTRY::(%02x:%02x:%02x:%02x:%02x:%02x)\n", Addr[0],Addr[1],Addr[2],Addr[3],Addr[4],Addr[5]));

				HashIdx = MAC_ADDR_HASH_INDEX(Addr);
				pEntry = pAd->MacTab.Hash[HashIdx];
				
				if (pEntry)
				{
					DisAssocAction(pAd, pEntry, REASON_DISASSOC_STA_LEAVING);
//					MacTableDeleteEntry(pAd, pEntry->Aid, Addr);				
				}
    		}
			break;
#ifdef WSC_AP_SUPPORT
		case RT_OID_WSC_SET_SELECTED_REGISTRAR:
			{	
				PUCHAR      upnpInfo;
                UCHAR	    apidx = pObj->ioctl_if;
				
				DBGPRINT(RT_DEBUG_TRACE, ("WSC::RT_OID_WSC_SET_SELECTED_REGISTRAR, wrq->u.data.length=%d!\n", wrq->u.data.length));
				upnpInfo = kmalloc(wrq->u.data.length, GFP_KERNEL);
				if(upnpInfo)
				{
					int len;
					
					len = copy_from_user(upnpInfo, wrq->u.data.pointer, wrq->u.data.length);
					len = wrq->u.data.length;
					
					if((pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode & WSC_PROXY))
					{
						WscSelectedRegistrar(pAd, upnpInfo, len);

    					if (pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimerRunning == TRUE)
    					{
    					    BOOLEAN Cancelled;
    						RTMPCancelTimer(&pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimer, &Cancelled);
    					}
    	    			// 2mins time-out timer
        				RTMPSetTimer(&pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimer, WSC_TWO_MINS_TIME_OUT);
        				pAd->ApCfg.MBSSID[apidx].WscControl.Wsc2MinsTimerRunning = TRUE;
					}
                    kfree(upnpInfo);
				} 
                else {
					Status = -EINVAL;
				}
			}
			break;
		case RT_OID_WSC_EAPMSG:
			{
				RTMP_WSC_U2KMSG_HDR *msgHdr = NULL;
				PUCHAR pUPnPMsg = NULL;
				UINT msgLen = 0, Machine = 0, msgType = 0;
				int retVal, senderID = 0;

				DBGPRINT(RT_DEBUG_TRACE, ("WSC::RT_OID_WSC_EAPMSG, wrq->u.data.length=%d!\n", wrq->u.data.length));
			
				msgLen = wrq->u.data.length;				
				if((pUPnPMsg = kmalloc(msgLen, GFP_KERNEL)) == NULL)
					Status = -EINVAL;
				else
				{
					memset(pUPnPMsg, 0, msgLen);
					retVal = copy_from_user(pUPnPMsg, wrq->u.data.pointer, msgLen);
					
					msgHdr = (RTMP_WSC_U2KMSG_HDR *)pUPnPMsg;
					senderID = *((int *)&msgHdr->Addr2);
					//assign the STATE_MACHINE type
					{
						int HeaderLen = LENGTH_802_11 + LENGTH_802_1_H + sizeof(IEEE8021X_FRAME) + sizeof(EAP_FRAME);
						UCHAR *pWpsMsg = (PUCHAR) &pUPnPMsg[HeaderLen];
						UINT WpsMsgLen = msgLen - HeaderLen;
						
                        Machine = WSC_STATE_MACHINE;
						msgType = WSC_EAPOL_UPNP_MSG;
                        
                        // If AP is unconfigured, WPS state machine will be triggered after received M2.
                        if (pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfStatus == WSC_SCSTATE_UNCONFIGURED)
                        {
                            if (strstr(pWpsMsg, "SimpleConfig") &&
                                !pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EapMsgRunning &&
								!pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscUPnPNodeInfo.bUPnPInProgress)
                            {
                                // GetDeviceInfo
                                WscInit(pAd, FALSE, &pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl);
								// trigger wsc re-generate public key
    							pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.RegData.ReComputePke = 1;
                            }
                            else if (WscRxMsgTypeFromUpnp(pAd, pWpsMsg, WpsMsgLen) == WSC_MSG_M2 &&
									 !pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.EapMsgRunning &&
									 !pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscUPnPNodeInfo.bUPnPInProgress)
                            {
                                // Check Enrollee Nonce of M2
                                if (WscCheckEnrolleeNonceFromUpnp(pAd, pWpsMsg, WpsMsgLen, &pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl))
                                {
                                    WscGetConfWithoutTrigger(pAd, &pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl, TRUE);
									pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscState = WSC_STATE_SENT_M1;
                                }
                            }
                        }
                        
						retVal = MlmeEnqueueForWsc(pAd, msgHdr->envID, senderID, Machine, msgType, msgLen, pUPnPMsg);
						if((retVal == FALSE) && (msgHdr->envID != 0))
						{
							DBGPRINT(RT_DEBUG_TRACE, ("MlmeEnqueuForWsc return False and envID=0x%x!\n", msgHdr->envID));
							Status = -EINVAL;
						}
					}

					kfree(pUPnPMsg);
				}
				DBGPRINT(RT_DEBUG_TRACE, ("RT_OID_WSC_EAPMSG finished!\n"));
			}
			break;
#endif // WSC_AP_SUPPORT //

#ifdef SNMP_SUPPORT
		case OID_802_11_SHORTRETRYLIMIT:
			if (wrq->u.data.length != sizeof(ULONG))
				Status = -EINVAL;
			else
			{
				Status = copy_from_user(&ShortRetryLimit, wrq->u.data.pointer, wrq->u.data.length);
				RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
				tx_rty_cfg.field.ShortRtyLimit = ShortRetryLimit;
				RTMP_IO_WRITE32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_SHORTRETRYLIMIT (tx_rty_cfg.field.ShortRetryLimit=%d, ShortRetryLimit=%ld)\n", tx_rty_cfg.field.ShortRtyLimit, ShortRetryLimit));
			}
			break;

		case OID_802_11_LONGRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_LONGRETRYLIMIT \n"));
			if (wrq->u.data.length != sizeof(ULONG))
				Status = -EINVAL;
			else
			{
				Status = copy_from_user(&LongRetryLimit, wrq->u.data.pointer, wrq->u.data.length);
				RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
				tx_rty_cfg.field.LongRtyLimit = LongRetryLimit;
				RTMP_IO_WRITE32(pAd, TX_RTY_CFG, tx_rty_cfg.word);
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_LONGRETRYLIMIT (tx_rty_cfg.field.LongRetryLimit= %d,LongRetryLimit=%ld)\n", tx_rty_cfg.field.LongRtyLimit, LongRetryLimit));
			}
			break;

		case OID_802_11_WEPDEFAULTKEYVALUE:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEPDEFAULTKEYVALUE\n"));
			pKey = kmalloc(wrq->u.data.length, GFP_KERNEL);
			Status = copy_from_user(pKey, wrq->u.data.pointer, wrq->u.data.length);
			//pKey = &WepKey;
			
			if ( pKey->Length != wrq->u.data.length)
			{
				Status = -EINVAL;
				DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEPDEFAULTKEYVALUE, Failed!!\n"));
			}
			KeyIdx = pKey->KeyIndex & 0x0fffffff;
			DBGPRINT(RT_DEBUG_TRACE,("pKey->KeyIndex =%d, pKey->KeyLength=%d\n", pKey->KeyIndex, pKey->KeyLength));

			// it is a shared key
			if (KeyIdx > 4)
				Status = -EINVAL;
			else
			{
				pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen = (UCHAR) pKey->KeyLength;
				NdisMoveMemory(&pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].Key, &pKey->KeyMaterial, pKey->KeyLength);
				if (pKey->KeyIndex & 0x80000000)
				{
					// Default key for tx (shared key)
					pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId = (UCHAR) KeyIdx;
				}
				//RestartAPIsRequired = TRUE;
			}
			break;


		case OID_802_11_WEPDEFAULTKEYID:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_WEPDEFAULTKEYID \n"));

			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
				Status = copy_from_user(&pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId, wrq->u.data.pointer, wrq->u.data.length);

			break;


		case OID_802_11_CURRENTCHANNEL:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::OID_802_11_CURRENTCHANNEL \n"));
			if (wrq->u.data.length != sizeof(UCHAR))
				Status = -EINVAL;
			else
			{
				Status = copy_from_user(&ctmp, wrq->u.data.pointer, wrq->u.data.length);
				sprintf(&ctmp,"%d", ctmp);
				Set_Channel_Proc(pAd, &ctmp);
			}
			break;
#endif

   		default:
			DBGPRINT(RT_DEBUG_TRACE, ("Set::unknown IOCTL's subcmd = 0x%08x\n", cmd));
			Status = -EOPNOTSUPP;
			break;
    }
	
	return Status;
}

INT RTMPAPQueryInformation(
	IN	PRTMP_ADAPTER       pAd,
	IN	OUT	struct ifreq    *rq,
	IN	INT                 cmd)
{
	struct iwreq						*wrq = (struct iwreq *) rq;
    INT	Status = NDIS_STATUS_SUCCESS;
    POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	driverVersion[8];

#if DBG || WSC_AP_SUPPORT
	UCHAR	apidx = pObj->ioctl_if;
#endif
#ifdef WSC_AP_SUPPORT
	UINT	WscPinCode = 0;
#endif // WSC_AP_SUPPORT //

#ifdef SNMP_SUPPORT	
	//for snmp, kathy
	ULONG ulInfo;
	DefaultKeyIdxValue			*pKeyIdxValue;
	INT							valueLen;
	TX_RTY_CFG_STRUC			tx_rty_cfg;
	ULONG						ShortRetryLimit, LongRetryLimit;
	UCHAR						tmp[64];
#endif //SNMP	

    switch(cmd)
    {
		case RT_OID_VERSION_INFO:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_VERSION_INFO \n"));
			wrq->u.data.length = 8*sizeof(UCHAR);
			sprintf(&driverVersion[0], "%s", AP_DRIVER_VERSION);
			driverVersion[7] = '\0';
			if (copy_to_user(wrq->u.data.pointer, &driverVersion, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;

		case OID_802_11_NETWORK_TYPES_SUPPORTED:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_NETWORK_TYPES_SUPPORTED \n"));
			wrq->u.data.length = sizeof(UCHAR);
			if (copy_to_user(wrq->u.data.pointer, &pAd->RfIcType, wrq->u.data.length))
			{
				Status = -EFAULT; 	
			}
			break;

#ifdef RALINK_ATE
		case RT_QUERY_ATE_TXDONE_COUNT:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_QUERY_ATE_TXDONE_COUNT \n"));
			wrq->u.data.length = sizeof(UINT32);
			if (copy_to_user(wrq->u.data.pointer, &pAd->ate.TxDoneCount, wrq->u.data.length))
			{
				Status = -EFAULT; 	
			}
			break;
#endif // RALINK_ATE //

#ifdef IAPP_SUPPORT
		case RT_QUERY_SIGNAL_CONTEXT:
		{
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_QUERY_SIGNAL_CONTEXT \n"));
			wrq->u.data.length = sizeof(RT_SIGNAL_STRUC);
			if (copy_to_user(wrq->u.data.pointer, &pObj->RTSignal, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
		}
			break;
#endif // IAPP_SUPPORT //

#ifdef WSC_AP_SUPPORT
		case RT_OID_WSC_QUERY_STATUS:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_QUERY_STATUS \n"));
			wrq->u.data.length = sizeof(INT);
			if (copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[apidx].WscControl.WscStatus, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;

		case RT_OID_WSC_PIN_CODE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_PIN_CODE \n"));
			wrq->u.data.length = sizeof(UINT);
			//WscPinCode = WscGeneratePinCode(pAd, FALSE, apidx);
			WscPinCode = pAd->ApCfg.MBSSID[0].WscControl.WscEnrolleePinCode;
			
			if (copy_to_user(wrq->u.data.pointer, &WscPinCode, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
#ifdef APCLI_SUPPORT
        case RT_OID_APCLI_WSC_PIN_CODE:
            DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_APCLI_WSC_PIN_CODE \n"));
			wrq->u.data.length = sizeof(UINT);
			//WscPinCode = WscGeneratePinCode(pAd, TRUE, apidx);
			WscPinCode = pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode;
			
			if (copy_to_user(wrq->u.data.pointer, &WscPinCode, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
            break;
#endif // APCLI_SUPPORT //
		case RT_OID_WSC_UUID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_QUERY_UUID \n"));
			wrq->u.data.length = UUID_LEN_STR;
			if (copy_to_user(wrq->u.data.pointer, &pAd->Wsc_Uuid_Str[0], UUID_LEN_STR))
			{
				Status = -EFAULT;
			}
			break;
		case RT_OID_WSC_MAC_ADDRESS:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_WSC_MAC_ADDRESS \n"));
			wrq->u.data.length = MAC_ADDR_LEN;
			if (copy_to_user(wrq->u.data.pointer, pAd->ApCfg.MBSSID[apidx].Bssid, wrq->u.data.length))
			{
				Status = -EFAULT;
			}
			break;
#endif // WSC_AP_SUPPORT //
#ifdef LLTD_SUPPORT
        case RT_OID_GET_PHY_MODE:
            DBGPRINT(RT_DEBUG_TRACE, ("Query::Get phy mode (%02X) \n", pAd->CommonCfg.PhyMode));
            wrq->u.mode = (u32)pAd->CommonCfg.PhyMode;
            break;

        case RT_OID_GET_LLTD_ASSO_TABLE:
            DBGPRINT(RT_DEBUG_TRACE, ("Query::Get LLTD assoication table\n"));
            if ((wrq->u.data.pointer == NULL) || (apidx != MAIN_MBSSID))
            {
                Status = -EFAULT;
            }
            else
            {
                INT						    i;
                RT_LLTD_ASSOICATION_TABLE	AssocTab;

            	AssocTab.Num = 0;
            	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
            	{
            		if (pAd->MacTab.Content[i].ValidAsCLI && (pAd->MacTab.Content[i].Sst == SST_ASSOC))
            		{
            			COPY_MAC_ADDR(AssocTab.Entry[AssocTab.Num].Addr, &pAd->MacTab.Content[i].Addr);
                        AssocTab.Entry[AssocTab.Num].phyMode = pAd->CommonCfg.PhyMode;
                        AssocTab.Entry[AssocTab.Num].MOR = RateIdToMbps[pAd->ApCfg.MBSSID[apidx].MaxTxRate] * 2;
            			AssocTab.Num += 1;
            		}
            	}            
                wrq->u.data.length = sizeof(RT_LLTD_ASSOICATION_TABLE);
            	if (copy_to_user(wrq->u.data.pointer, &AssocTab, wrq->u.data.length))
            	{
            		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
                    Status = -EFAULT;
            	}
                DBGPRINT(RT_DEBUG_TRACE, ("AssocTab.Num = %d \n", AssocTab.Num));
            }
            break;
#ifdef APCLI_SUPPORT
		case RT_OID_GET_REPEATER_AP_LINEAGE:
#if 0			
			DBGPRINT(RT_DEBUG_TRACE, ("Query::Get repeater AP lineage.\n"));
			if (wrq->u.data.pointer == NULL)
			{
				Status = -EFAULT;
				break;
			}

			if (pAd->ApCfg.ApCliTab[apidx].Valid)
			{
				wrq->u.data.length = 6;
				if (copy_to_user(wrq->u.data.pointer,
					APCLI_GET_ROOT_BSSID(pAd, pAd->ApCfg.ApCliTab[apidx].MacTabWCID), wrq->u.data.length))
				{
					DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
					Status = -EFAULT;
				}
				DBGPRINT(RT_DEBUG_TRACE, ("%s: Root AP BSSID: \n", __FUNCTION__));
			}
			else
				wrq->u.data.length = 0;
#else
			DBGPRINT(RT_DEBUG_TRACE, ("Not Support : Get repeater AP lineage.\n"));
#endif
			break;
#endif // APCLI_SUPPORT //

#endif // LLTD_SUPPORT //
		case OID_802_11_RADIUS_QUERY_SETTING:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::Get Radius setting(%d)\n", sizeof(RADIUS_CONF)));
				RTMPIoctlQueryRadiusConf(pAd, wrq);	
			break;
#ifdef SNMP_SUPPORT
		case RT_OID_802_11_MAC_ADDRESS:
            wrq->u.data.length = MAC_ADDR_LEN;
            Status = copy_to_user(wrq->u.data.pointer, &pAd->CurrentAddress, wrq->u.data.length);
            DBGPRINT(RT_DEBUG_INFO, ("Query::RT_OID_802_11_MAC_ADDRESS \n"));
			break;

		case RT_OID_802_11_MANUFACTUREROUI:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_MANUFACTUREROUI \n"));
			wrq->u.data.length = ManufacturerOUI_LEN;
			Status = copy_to_user(wrq->u.data.pointer, &pAd->CurrentAddress, wrq->u.data.length);
			break;

		case RT_OID_802_11_MANUFACTURERNAME:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_MANUFACTURERNAME \n"));
			wrq->u.data.length = strlen(ManufacturerNAME);
			Status = copy_to_user(wrq->u.data.pointer, ManufacturerNAME, wrq->u.data.length);
			break;

		case RT_OID_802_11_RESOURCETYPEIDNAME:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_RESOURCETYPEIDNAME \n"));
			wrq->u.data.length = strlen(ResourceTypeIdName);
			Status = copy_to_user(wrq->u.data.pointer, ResourceTypeIdName, wrq->u.data.length);
			break;

		case RT_OID_802_11_PRIVACYOPTIONIMPLEMENTED:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_PRIVACYOPTIONIMPLEMENTED \n"));
			ulInfo = 1; // 1 is support wep else 2 is not support.
			wrq->u.data.length = sizeof(ulInfo);
			Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
			break;

		case RT_OID_802_11_POWERMANAGEMENTMODE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_POWERMANAGEMENTMODE \n"));
			ulInfo = 1; // 1 is power active else 2 is power save.
			wrq->u.data.length = sizeof(ulInfo);
			Status = copy_to_user(wrq->u.data.pointer, &ulInfo, wrq->u.data.length);
			break;

		case OID_802_11_WEPDEFAULTKEYVALUE:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_WEPDEFAULTKEYVALUE \n"));
			//KeyIdxValue.KeyIdx = pAd->PortCfg.MBSSID[pObj->ioctl_if].DefaultKeyId;
			pKeyIdxValue = wrq->u.data.pointer;
			DBGPRINT(RT_DEBUG_TRACE,("KeyIdxValue.KeyIdx = %d, \n",pKeyIdxValue->KeyIdx));

			valueLen = pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen;
			NdisMoveMemory(pKeyIdxValue->Value,
						   &pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].Key,
						   valueLen);
			pKeyIdxValue->Value[valueLen]='\0';

			wrq->u.data.length = sizeof(DefaultKeyIdxValue);

			Status = copy_to_user(wrq->u.data.pointer, pKeyIdxValue, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE,("DefaultKeyId = %d, total len = %d, str len=%d, KeyValue= %02x %02x %02x %02x \n", pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId, wrq->u.data.length, pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen,
			pAd->SharedKey[pObj->ioctl_if][0].Key[0],
			pAd->SharedKey[pObj->ioctl_if][1].Key[0],
			pAd->SharedKey[pObj->ioctl_if][2].Key[0],
			pAd->SharedKey[pObj->ioctl_if][3].Key[0]));
			break;

		case OID_802_11_WEPDEFAULTKEYID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_WEPDEFAULTKEYID \n"));
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer, &pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("DefaultKeyId =%d \n", pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId));
			break;

		case RT_OID_802_11_WEPKEYMAPPINGLENGTH:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_WEPKEYMAPPINGLENGTH \n"));
			wrq->u.data.length = sizeof(UCHAR);
			Status = copy_to_user(wrq->u.data.pointer,
									&pAd->SharedKey[pObj->ioctl_if][pAd->ApCfg.MBSSID[pObj->ioctl_if].DefaultKeyId].KeyLen,
									wrq->u.data.length);
			break;

		case OID_802_11_SHORTRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_SHORTRETRYLIMIT \n"));
			wrq->u.data.length = sizeof(ULONG);
			RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
			ShortRetryLimit = tx_rty_cfg.field.ShortRtyLimit;
			DBGPRINT(RT_DEBUG_TRACE, ("ShortRetryLimit =%ld,  tx_rty_cfg.field.ShortRetryLimit=%d\n", ShortRetryLimit, tx_rty_cfg.field.ShortRtyLimit));
			Status = copy_to_user(wrq->u.data.pointer, &ShortRetryLimit, wrq->u.data.length);
			break;

		case OID_802_11_LONGRETRYLIMIT:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_LONGRETRYLIMIT \n"));
			wrq->u.data.length = sizeof(ULONG);
			RTMP_IO_READ32(pAd, TX_RTY_CFG, &tx_rty_cfg.word);
			LongRetryLimit = tx_rty_cfg.field.LongRtyLimit;
			DBGPRINT(RT_DEBUG_TRACE, ("LongRetryLimit =%ld,  tx_rty_cfg.field.LongRtyLimit=%d\n", LongRetryLimit, tx_rty_cfg.field.LongRtyLimit));
			Status = copy_to_user(wrq->u.data.pointer, &LongRetryLimit, wrq->u.data.length);
			break;
			
		case RT_OID_802_11_PRODUCTID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_PRODUCTID \n"));
		
#ifdef RT2860
			{
			
				USHORT  device_id;
				if (((POS_COOKIE)pAd->OS_Cookie)->pci_dev != NULL)
			    	pci_read_config_word(((POS_COOKIE)pAd->OS_Cookie)->pci_dev, PCI_DEVICE_ID, &device_id);
				else 
					DBGPRINT(RT_DEBUG_TRACE, (" pci_dev = NULL\n"));
				sprintf(tmp, "%04x %04x\n", NIC_PCI_VENDOR_ID, device_id);
			}
#else
			sprintf(tmp, "%04x %04x\n", ((POS_COOKIE)pAd->OS_Cookie)->pUsb_Dev->descriptor.idVendor ,((POS_COOKIE)pAdapter->OS_Cookie)->pUsb_Dev->descriptor.idProduct);

#endif
			wrq->u.data.length = strlen(tmp);
			Status = copy_to_user(wrq->u.data.pointer, tmp, wrq->u.data.length);
			break;

		case RT_OID_802_11_MANUFACTUREID:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::RT_OID_802_11_MANUFACTUREID \n"));
			wrq->u.data.length = strlen(ManufacturerNAME);
			Status = copy_to_user(wrq->u.data.pointer, ManufacturerNAME, wrq->u.data.length);
			break;

		case OID_802_11_CURRENTCHANNEL:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::OID_802_11_CURRENTCHANNEL \n"));
			wrq->u.data.length = sizeof(UCHAR);
			DBGPRINT(RT_DEBUG_TRACE, ("sizeof UCHAR=%d, channel=%d \n", sizeof(UCHAR), pAd->CommonCfg.Channel));
			Status = copy_to_user(wrq->u.data.pointer, &pAd->CommonCfg.Channel, wrq->u.data.length);
			DBGPRINT(RT_DEBUG_TRACE, ("Status=%d\n", Status));
			break;
#endif //SNMP_SUPPORT
   		default:
			DBGPRINT(RT_DEBUG_TRACE, ("Query::unknown IOCTL's subcmd = 0x%08x, apidx=%d\n", cmd, apidx));
			Status = -EOPNOTSUPP;
			break;
    }

	return Status;
}

INT rt28xx_ap_ioctl(
	IN	struct net_device	*net_dev, 
	IN	OUT	struct ifreq	*rq, 
	IN	INT					cmd)
{
    VIRTUAL_ADAPTER	*pVirtualAd = NULL;
	RTMP_ADAPTER	*pAd = NULL;
    struct iwreq	*wrq = (struct iwreq *) rq;
    INT				Status = NDIS_STATUS_SUCCESS;
    USHORT			subcmd, index;
	POS_COOKIE		pObj;
	UCHAR			apidx=0;
	
	if (net_dev->priv_flags == INT_MAIN)
	{
		pAd = net_dev->priv;
	}
	else
	{
		pVirtualAd = net_dev->priv;
		pAd = pVirtualAd->RtmpDev->priv;
	}
	pObj = (POS_COOKIE) pAd->OS_Cookie;

	if (pAd == NULL)
	{
		/* if 1st open fail, pAd will be free;
		   So the net_dev->priv will be NULL in 2rd open */
		return -ENETDOWN;
	}

    //+ patch for SnapGear Request even the interface is down
    if(cmd== SIOCGIWNAME){
	    DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWNAME\n"));
#ifdef RT2860
	    strcpy(wrq->u.name, "RT2860 SoftAP");
#endif // RT2860 //
	    return Status;
    }//- patch for SnapGear
	
    if((net_dev->priv_flags == INT_MAIN) && !RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
    {
#ifdef CONFIG_APSTA_MIXED_SUPPORT
	if (wrq->u.data.pointer == NULL)
		return Status;

	if (cmd == RTPRIV_IOCTL_SET)
	{
		if (strstr(wrq->u.data.pointer, "OpMode") == NULL)
			return -ENETDOWN;
	}
	else
#endif // CONFIG_APSTA_MIXED_SUPPORT //
		return -ENETDOWN;
    }

    // determine this ioctl command is comming from which interface.
    if (net_dev->priv_flags == INT_MAIN)
    {
		pObj->ioctl_if_type = INT_MAIN;
        pObj->ioctl_if = MAIN_MBSSID;
//        DBGPRINT(RT_DEBUG_INFO, ("rt28xx_ioctl I/F(ra%d)(flags=%d): cmd = 0x%08x\n", pObj->ioctl_if, net_dev->priv_flags, cmd));
    }
    else if (net_dev->priv_flags == INT_MBSSID)
    {
		pObj->ioctl_if_type = INT_MBSSID;
//    	if (!RTMPEqualMemory(net_dev->name, pAd->net_dev->name, 3))  // for multi-physical card, no MBSSID
		if (strcmp(net_dev->name, pAd->net_dev->name) != 0) // sample
    	{
	        for (index = 1; index < pAd->ApCfg.BssidNum; index++)
	    	{
	    	    if (pAd->ApCfg.MBSSID[index].MSSIDDev == net_dev)
	    	    {
	    	        pObj->ioctl_if = index;
	    	        
//	    	        DBGPRINT(RT_DEBUG_INFO, ("rt28xx_ioctl I/F(ra%d)(flags=%d): cmd = 0x%08x\n", index, net_dev->priv_flags, cmd));
	    	        break;
	    	    }
	    	}
	        // Interface not found!
	        if(index == pAd->ApCfg.BssidNum)
	        {
//	        	DBGPRINT(RT_DEBUG_ERROR, ("rt28xx_ioctl can not find I/F\n"));
	            return -ENETDOWN;
	        }
	    }
	    else    // ioctl command from I/F(ra0)
	    {
	    	pAd= net_dev->priv;
    	    pObj->ioctl_if = MAIN_MBSSID;
//	        DBGPRINT(RT_DEBUG_ERROR, ("rt28xx_ioctl can not find I/F and use default: cmd = 0x%08x\n", cmd));
	    }
        MBSS_MR_APIDX_SANITY_CHECK(pObj->ioctl_if);
        apidx = pObj->ioctl_if;
    }
#ifdef APCLI_SUPPORT
	else if (net_dev->priv_flags == INT_APCLI)
	{
		pObj->ioctl_if_type = INT_APCLI;
		for (index = 0; index < MAX_APCLI_NUM; index++)
		{
			if (pAd->ApCfg.ApCliTab[index].dev == net_dev)
			{
				pObj->ioctl_if = index;

				DBGPRINT(RT_DEBUG_INFO, ("rt28xx_ioctl I/F(apcli%d)(flags=%x): cmd = 0x%08x\n", pObj->ioctl_if, net_dev->priv_flags, cmd));
				break;
			}

			if(index == MAX_APCLI_NUM)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("rt28xx_ioctl can not find I/F\n"));
				return -ENETDOWN;
			}
		}
		APCLI_MR_APIDX_SANITY_CHECK(pObj->ioctl_if);
	}
#endif // APCLI_SUPPORT //
#ifdef MESH_SUPPORT
	else if (net_dev->priv_flags & INT_MESH)
	{
		pObj->ioctl_if_type = INT_MESH;
		pObj->ioctl_if = 0;
	}
#endif // MESH_SUPPORT //
    else
    {
//    	DBGPRINT(RT_DEBUG_WARN, ("IOCTL is not supported in WDS interface\n"));
    	return -EOPNOTSUPP;
    }

	switch(cmd)
	{
#ifdef RALINK_ATE
#ifdef RALINK_28xx_QA
		case RTPRIV_IOCTL_ATE:
			{
				RtmpDoAte(pAd, wrq);
			}
			break;
#endif // RALINK_28xx_QA // 
#endif // RALINK_ATE //
        case SIOCGIFHWADDR:
			DBGPRINT(RT_DEBUG_TRACE, ("IOCTLIOCTLIOCTL::SIOCGIFHWADDR\n"));
			strcpy(wrq->u.name, pAd->ApCfg.MBSSID[pObj->ioctl_if].Bssid);
			break;
		case SIOCGIWNAME:
			DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWNAME\n"));
#ifdef RT2860
			strcpy(wrq->u.name, "RT2860 SoftAP");
#endif // RT2860 //
			break;
		case SIOCSIWESSID:  //Set ESSID
			Status = -EOPNOTSUPP;
			break;
		case SIOCGIWESSID:  //Get ESSID
			{
				struct iw_point *erq = &wrq->u.essid;
				PUCHAR pSsidStr = NULL;

				erq->flags=1;
              //erq->length = pAd->ApCfg.MBSSID[pObj->ioctl_if].SsidLen;
              
#ifdef APCLI_SUPPORT
				if (net_dev->priv_flags == INT_APCLI)
				{
					if (pAd->ApCfg.ApCliTab[pObj->ioctl_if].Valid == TRUE)
					{
						erq->length = pAd->ApCfg.ApCliTab[pObj->ioctl_if].SsidLen;
						pSsidStr = (PCHAR)&pAd->ApCfg.ApCliTab[pObj->ioctl_if].Ssid;
					}
					else {
						erq->length = 0;
						pSsidStr = NULL;
					}
				}
				else
#endif // APCLI_SUPPORT //
				{
				erq->length = pAd->ApCfg.MBSSID[apidx].SsidLen;
					pSsidStr = pAd->ApCfg.MBSSID[apidx].Ssid;
				}

				if((erq->pointer) && (pSsidStr != NULL))
				{
					//if(copy_to_user(erq->pointer, pAd->ApCfg.MBSSID[pObj->ioctl_if].Ssid, erq->length))
					if(copy_to_user(erq->pointer, pSsidStr, erq->length))
					{
						Status = -EFAULT;
						break;
					}
				}
				DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWESSID (Len=%d, ssid=%s...)\n", erq->length, (char *)erq->pointer));
			}
			break;
		case SIOCGIWNWID: // get network id 
		case SIOCSIWNWID: // set network id (the cell)
			Status = -EOPNOTSUPP;
			break;
		case SIOCGIWFREQ: // get channel/frequency (Hz)
			wrq->u.freq.m = pAd->CommonCfg.Channel;
			wrq->u.freq.e = 0;
			wrq->u.freq.i = 0;
			break; 
		case SIOCSIWFREQ: //set channel/frequency (Hz)
		case SIOCGIWNICKN:
		case SIOCSIWNICKN: //set node name/nickname
		case SIOCGIWRATE:  //get default bit rate (bps)
            {
                int rate_index = 0;
                __s32 ralinkrate[256] = {2,4,11,22, 12,18,24,36,48,72,96,  108,   109, 110, 111, 112, 13, 26, 39, 52,78,104, 117, 130, 26, 52, 78,104, 156, 208, 234, 260, 27, 54,81,108,162, 216, 243, 270, // Last 38
	            54, 108, 162, 216, 324, 432, 486, 540,  14, 29, 43, 57, 87, 115, 130, 144, 29, 59,87,115, 173, 230,260, 288, 30, 60,90,120,180,240,270,300,60,120,180,240,360,480,540,600, 0,1,2,3,4,5,6,7,8,9,10,
	            11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80};
				PHTTRANSMIT_SETTING		pHtPhyMode;
#ifdef MESH_SUPPORT
				if (net_dev->priv_flags == INT_MESH)
					pHtPhyMode = &pAd->MeshTab.HTPhyMode;
				else
#endif // MESH_SUPPORT //
#ifdef APCLI_SUPPORT
				if (net_dev->priv_flags == INT_APCLI)
					pHtPhyMode = &pAd->ApCfg.ApCliTab[pObj->ioctl_if].HTPhyMode;
				else
#endif // APCLI_SUPPORT //
#ifdef WDS_SUPPORT
				if (net_dev->priv_flags == INT_WDS)
					pHtPhyMode = &pAd->WdsTab.WdsEntry[pObj->ioctl_if].HTPhyMode;
				else
#endif // WDS_SUPPORT //
					pHtPhyMode = &pAd->ApCfg.MBSSID[pObj->ioctl_if].HTPhyMode;
                
                if (pHtPhyMode->field.MODE >= MODE_HTMIX)
                {
                	rate_index = 16 + ((UCHAR)pHtPhyMode->field.BW *16) + ((UCHAR)pHtPhyMode->field.ShortGI *32) + ((UCHAR)pHtPhyMode->field.MCS);
                }
                else if (pHtPhyMode->field.MODE == MODE_OFDM)
                	rate_index = (UCHAR)(pHtPhyMode->field.MCS) + 4;
                else 
                	rate_index = (UCHAR)(pHtPhyMode->field.MCS);
                if (rate_index < 0)
                    rate_index = 0;
                if (rate_index > 255)
                    rate_index = 255;
    
			    wrq->u.bitrate.value = ralinkrate[rate_index] * 500000;
			wrq->u.bitrate.disabled = 0;
            }
			break;
		case SIOCSIWRATE:  //set default bit rate (bps)
		case SIOCGIWRTS:  // get RTS/CTS threshold (bytes)
		case SIOCSIWRTS:  //set RTS/CTS threshold (bytes)
		case SIOCGIWFRAG:  //get fragmentation thr (bytes)
		case SIOCSIWFRAG:  //set fragmentation thr (bytes)
		case SIOCGIWENCODE:  //get encoding token & mode
		case SIOCSIWENCODE:  //set encoding token & mode
			Status = -EOPNOTSUPP;
			break;
		case SIOCGIWAP:  //get access point MAC addresses
			{
				PUCHAR pBssidStr;

				wrq->u.ap_addr.sa_family = ARPHRD_ETHER;
				//memcpy(wrq->u.ap_addr.sa_data, &pAd->ApCfg.MBSSID[pObj->ioctl_if].Bssid, ETH_ALEN);
#ifdef APCLI_SUPPORT
				if (net_dev->priv_flags == INT_APCLI)
				{
					if (pAd->ApCfg.ApCliTab[pObj->ioctl_if].Valid == TRUE)
						pBssidStr = (PCHAR)&APCLI_ROOT_BSSID_GET(pAd, pAd->ApCfg.ApCliTab[pObj->ioctl_if].MacTabWCID);
					else
						pBssidStr = NULL;
				}
				else
#endif // APCLI_SUPPORT //
				{
					pBssidStr = (PCHAR)&pAd->ApCfg.MBSSID[pObj->ioctl_if].Bssid;
				}

				if (pBssidStr != NULL)
				{
					memcpy(wrq->u.ap_addr.sa_data, pBssidStr, ETH_ALEN);
					DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::SIOCGIWAP(=%02x:%02x:%02x:%02x:%02x:%02x)\n",
						pBssidStr[0],pBssidStr[1],pBssidStr[2], pBssidStr[3],pBssidStr[4],pBssidStr[5]));
				}
				else
				{
					memset(wrq->u.ap_addr.sa_data, 0, ETH_ALEN);
				}
			}
			break;
		case SIOCGIWMODE:  //get operation mode
			wrq->u.mode = IW_MODE_INFRA;   //SoftAP always on INFRA mode.
			break;
		case SIOCSIWAP:  //set access point MAC addresses
		case SIOCSIWMODE:  //set operation mode
		case SIOCGIWSENS:   //get sensitivity (dBm)
		case SIOCSIWSENS:	//set sensitivity (dBm)
		case SIOCGIWPOWER:  //get Power Management settings
		case SIOCSIWPOWER:  //set Power Management settings
		case SIOCGIWTXPOW:  //get transmit power (dBm)
		case SIOCSIWTXPOW:  //set transmit power (dBm)
		//case SIOCGIWRANGE:	//Get range of parameters
		case SIOCGIWRETRY:	//get retry limits and lifetime
		case SIOCSIWRETRY:	//set retry limits and lifetime
			Status = -EOPNOTSUPP;
			break;
		case SIOCGIWRANGE:	//Get range of parameters
		    {
		        struct iw_range range;
				UINT32 len;

                        memset(&range, 0, sizeof(range));
	                range.we_version_compiled = WIRELESS_EXT;
	                range.we_version_source = 14;
				len = copy_to_user(wrq->u.data.pointer, &range, sizeof(range));
		    }
		    break;
		    
		case RT_PRIV_IOCTL:
			subcmd = wrq->u.data.flags;
			if (subcmd & OID_GET_SET_TOGGLE)
				Status = RTMPAPSetInformation(pAd, rq, subcmd);
			else
				Status = RTMPAPQueryInformation(pAd, rq, subcmd);
			break;
		
		case SIOCGIWPRIV:
			if (wrq->u.data.pointer) 
			{
				if ( access_ok(VERIFY_WRITE, wrq->u.data.pointer, sizeof(ap_privtab)) != TRUE)
					break;
				wrq->u.data.length = sizeof(ap_privtab) / sizeof(ap_privtab[0]);
				if (copy_to_user(wrq->u.data.pointer, ap_privtab, sizeof(ap_privtab)))
					Status = -EFAULT;
			}
			break;
		case RTPRIV_IOCTL_SET:
			{				
				CHAR *this_char;
				CHAR *value;

				if( access_ok(VERIFY_READ, wrq->u.data.pointer, wrq->u.data.length) != TRUE)
					break;

				while ((this_char = strsep((char **)&wrq->u.data.pointer, ",")) != NULL) 
				{
					if (!*this_char)
						 continue;

					if ((value = strchr(this_char, '=')) != NULL)
						*value++ = 0;

					if (!value 
#ifdef WSC_AP_SUPPORT                        
                        && (
                             (strcmp(this_char, "WscStop") != 0) &&
                             (strcmp(this_char, "WscGenPinCode")!= 0)
                           )
#endif // WSC_AP_SUPPORT //
                        )
						continue;  							

					for (PRTMP_PRIVATE_SET_PROC = RTMP_PRIVATE_SUPPORT_PROC; PRTMP_PRIVATE_SET_PROC->name; PRTMP_PRIVATE_SET_PROC++)
					{
						if (!strcmp(this_char, PRTMP_PRIVATE_SET_PROC->name)) 
						{						
							if(!PRTMP_PRIVATE_SET_PROC->set_proc(pAd, value))
							{   //FALSE:Set private failed then return Invalid argument 								
								Status = -EINVAL;							
							}
							break;  //Exit for loop.
						}
					}

					if(PRTMP_PRIVATE_SET_PROC->name == NULL)
					{  //Not found argument
						Status = -EINVAL;
						DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::(iwpriv) Command not Support [%s=%s]\n", this_char, value));
						break;
					}	
				}
			}
			break;
		    
		case RTPRIV_IOCTL_SHOW:
			{
				CHAR *this_char;
				CHAR *value = NULL;

				if( access_ok(VERIFY_READ, wrq->u.data.pointer, wrq->u.data.length) != TRUE)
					break;

				while ((this_char = strsep((char **)&wrq->u.data.pointer, ",")) != NULL) 
				{
					if (!*this_char)
						continue;

					for (PRTMP_PRIVATE_SHOW_PROC = RTMP_PRIVATE_SHOW_SUPPORT_PROC; PRTMP_PRIVATE_SHOW_PROC->name; PRTMP_PRIVATE_SHOW_PROC++)
					{
						if (!strcmp(this_char, PRTMP_PRIVATE_SHOW_PROC->name)) 
						{						
							if(!PRTMP_PRIVATE_SHOW_PROC->set_proc(pAd, value))
							{   //FALSE:Set private failed then return Invalid argument 								
								Status = -EINVAL;							
							}
							break;  //Exit for loop.
						}
					}

					if(PRTMP_PRIVATE_SHOW_PROC->name == NULL)
					{  //Not found argument
						Status = -EINVAL;
						DBGPRINT(RT_DEBUG_TRACE, ("IOCTL::(iwpriv) Command not Support [%s=%s]\n", this_char, value));
						break;
					}	
				}
			}
			break;		    

		case RTPRIV_IOCTL_GET_MAC_TABLE:
			RTMPIoctlGetMacTable(pAd,wrq);
		    break;

		case RTPRIV_IOCTL_GSITESURVEY:
			RTMPIoctlGetSiteSurvey(pAd,wrq);
			break;

		case RTPRIV_IOCTL_STATISTICS:
			RTMPIoctlStatistics(pAd, wrq);
			break;

		case RTPRIV_IOCTL_RADIUS_DATA:
		    RTMPIoctlRadiusData(pAd, wrq);
		    break;

		case RTPRIV_IOCTL_ADD_WPA_KEY:
		    RTMPIoctlAddWPAKey(pAd, wrq);
		    break;

		case RTPRIV_IOCTL_ADD_PMKID_CACHE:
		    RTMPIoctlAddPMKIDCache(pAd, wrq);
		    break;

		case RTPRIV_IOCTL_STATIC_WEP_COPY:
		    RTMPIoctlStaticWepCopy(pAd, wrq);
		    break;
#ifdef WSC_AP_SUPPORT
		case RTPRIV_IOCTL_WSC_PROFILE:
		    RTMPIoctlWscProfile(pAd, wrq);
		    break;
#endif // WSC_AP_SUPPORT //
		case RTPRIV_IOCTL_QUERY_BATABLE:
		    RTMPIoctlQueryBaTable(pAd, wrq);
		    break;
#ifdef DBG
		case RTPRIV_IOCTL_BBP:
			RTMPAPIoctlBBP(pAd, wrq);
			break;
			
		case RTPRIV_IOCTL_MAC:
			RTMPAPIoctlMAC(pAd, wrq);
			break;

		case RTPRIV_IOCTL_E2P:
			RTMPAPIoctlE2PROM(pAd, wrq);
			break;
#endif

		default:
//			DBGPRINT(RT_DEBUG_ERROR, ("IOCTL::unknown IOCTL's cmd = 0x%08x\n", cmd));
			Status = -EOPNOTSUPP;
			break;
	}

	return Status;
}

/*
char * __rstrtok;
char * rstrtok(char * s,const char * ct)
{
	char *sbegin, *send;

	sbegin  = s ? s : __rstrtok;
	if (!sbegin)
	{
		return NULL;
	}

	sbegin += strspn(sbegin,ct);
	if (*sbegin == '\0')
	{
		__rstrtok = NULL;
		return( NULL );
	}

	send = strpbrk( sbegin, ct);
	if (send && *send != '\0')
		*send++ = '\0';

	__rstrtok = send;

	return (sbegin);
}
*/

/* 
    ==========================================================================
    Description:
        Set Country Code.
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_CountryCode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{

	if(strlen(arg) == 2)
	{
		NdisMoveMemory(pAd->CommonCfg.CountryCode, arg, 2);
		pAd->CommonCfg.bCountryFlag = TRUE;
	}
	else
	{
		NdisZeroMemory(pAd->CommonCfg.CountryCode, 3);
		pAd->CommonCfg.bCountryFlag = FALSE;
	}	
		
	DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryCode_Proc::(bCountryFlag=%d, CountryCode=%s)\n", pAd->CommonCfg.bCountryFlag, pAd->CommonCfg.CountryCode));

	return TRUE;
}

INT Set_ChGeography_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG Geography;
		
	Geography = simple_strtol(arg, 0, 10);
	if (Geography <= BOTH)
		pAd->CommonCfg.Geography = Geography;
	else
		DBGPRINT(RT_DEBUG_ERROR, ("Set_ChannelGeography_Proc::(wrong setting. 0: Out-door, 1: in-door, 2: both)\n"));

	pAd->CommonCfg.CountryCode[2] =
		(pAd->CommonCfg.Geography == BOTH) ? ' ' : ((pAd->CommonCfg.Geography == IDOR) ? 'I' : 'O');

	DBGPRINT(RT_DEBUG_ERROR, ("Set_ChannelGeography_Proc:: Geography = %s\n", pAd->CommonCfg.Geography == ODOR ? "out-door" : (pAd->CommonCfg.Geography == IDOR ? "in-door" : "both")));
	
	// After Set ChGeography need invoke SSID change procedural again for Beacon update.
	// it's no longer necessary since APStartUp will rebuild channel again.
	//BuildChannelListEx(pAd);

	return TRUE;			
}

/*
    ==========================================================================
    Description:
        Set Country String.
        This command will not work, if the field of CountryRegion in eeprom is programmed.
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_CountryString_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT   index = 0;
	INT   success = TRUE;
	UCHAR  name_buffer[40] = {0};

#ifdef EXT_BUILD_CHANNEL_LIST
	return -EOPNOTSUPP;
#endif // EXT_BUILD_CHANNEL_LIST //

	if(strlen(arg) <= 38)
	{
		if (strlen(arg) < 4)
		{
			DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::Parameter of CountryString are too short !\n"));
			return FALSE;
		}
		
		for (index = 0; index < strlen(arg); index++)
		{
			if ((arg[index] >= 'a') && (arg[index] <= 'z'))
				arg[index] = toupper(arg[index]);
		}

		for (index = 0; index < NUM_OF_COUNTRIES; index++)
		{
			NdisZeroMemory(name_buffer, 40);
			sprintf(name_buffer, "\"%s\"", allCountry[index].CountryName);

			if (strncmp(allCountry[index].CountryName, arg, strlen(arg)) == 0)
				break;
			else if (strncmp(name_buffer, arg, strlen(arg)) == 0)
				break;
		}

		if (index == NUM_OF_COUNTRIES)
			success = FALSE;
	}
	else
	{
		success = FALSE;
	}			

	if (success == TRUE)
	{
		switch(pAd->CommonCfg.PhyMode)
		{
			case PHY_11BG_MIXED:	// 0
			case PHY_11B:			// 1
			case PHY_11G:			// 4
			case PHY_11N_2_4G:		// 6
			case PHY_11GN_MIXED:	// 7
			case PHY_11BGN_MIXED:	// 9
				if (pAd->CommonCfg.CountryRegionForABand & 0x80)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::parameter of CountryRegion in eeprom is programmed \n"));
					success = FALSE;
				}
				else
				{
					if (allCountry[index].SupportGBand == TRUE)
					{
						NdisZeroMemory(pAd->CommonCfg.CountryCode, 3);
						NdisMoveMemory(pAd->CommonCfg.CountryCode, allCountry[index].IsoName, 2);
						pAd->CommonCfg.CountryCode[2] = ' ';

						pAd->CommonCfg.bCountryFlag = TRUE;

						pAd->CommonCfg.CountryRegion = (UCHAR) allCountry[index].RegDomainNum11G;

						// After Set ChGeography need invoke SSID change procedural again for Beacon update.
						// it's no longer necessary since APStartUp will rebuild channel again.
						//BuildChannelList(pAd);

						success = TRUE;
					}
					else
					{
						success = FALSE;
						DBGPRINT(RT_DEBUG_TRACE, ("The Country are not Support G Band Channel\n"));
					}
				}

				break;
			case PHY_11A:			// 2
			case PHY_11AN_MIXED:	// 8
			case PHY_11N_5G:		// 11
				if (pAd->CommonCfg.CountryRegion & 0x80)
				{
					DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::parameter of CountryRegion in eeprom is programmed \n"));
					success = FALSE;
				}
				else
				{
					if (allCountry[index].SupportABand == TRUE)
					{
						NdisZeroMemory(pAd->CommonCfg.CountryCode, 3);
						NdisMoveMemory(pAd->CommonCfg.CountryCode, allCountry[index].IsoName, 2);
						pAd->CommonCfg.CountryCode[2] = ' ';

						pAd->CommonCfg.bCountryFlag = TRUE;

						pAd->CommonCfg.CountryRegionForABand = (UCHAR) allCountry[index].RegDomainNum11A;

						// After Set ChGeography need invoke SSID change procedural again for Beacon update.
						// it's no longer necessary since APStartUp will rebuild channel again.
						//BuildChannelList(pAd);

						success = TRUE;
					}
					else
					{
						success = FALSE;
						DBGPRINT(RT_DEBUG_TRACE, ("The Country are not Support A Band Channel\n"));
					}
				}
				break;

			default :
				success = FALSE;
				break;
		}
	}

	if (success == TRUE)
	{
		// if set country string, driver needs to be reset
		DBGPRINT(RT_DEBUG_TRACE, ("Set_CountryString_Proc::(CountryString=%s CountryRegin=%d CountryCode=%s)\n", 
							allCountry[index].CountryName, pAd->CommonCfg.CountryRegion, pAd->CommonCfg.CountryCode));
	}
	else
	{
		DBGPRINT(RT_DEBUG_ERROR, ("Set_CountryString_Proc::Parameters out of range\n"));
	}

	return success;
}

/* 
    ==========================================================================
    Description:
        Set SSID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_SSID_Proc(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	PUCHAR			arg)
{
	INT   success = FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAdapter->OS_Cookie;
	
	if(strlen(arg) <= MAX_LEN_OF_SSID)
	{
		NdisZeroMemory(pAdapter->ApCfg.MBSSID[pObj->ioctl_if].Ssid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pAdapter->ApCfg.MBSSID[pObj->ioctl_if].Ssid, arg, strlen(arg));
		pAdapter->ApCfg.MBSSID[pObj->ioctl_if].SsidLen = (UCHAR)strlen(arg);
		success = TRUE;

		// If in detection mode, need to stop detect first.
		if (pAdapter->CommonCfg.bIEEE80211H == FALSE)
		{
			APStop(pAdapter);
			APStartUp(pAdapter);
		}
		else
		{
			// each mode has different restart method
			if (pAdapter->CommonCfg.RadarDetect.RDMode == RD_SILENCE_MODE)
			{
				APStop(pAdapter);
				APStartUp(pAdapter);
			}
			else if (pAdapter->CommonCfg.RadarDetect.RDMode == RD_SWITCHING_MODE)
			{
			}
			else if (pAdapter->CommonCfg.RadarDetect.RDMode == RD_NORMAL_MODE)
			{
				APStop(pAdapter);
				APStartUp(pAdapter);
				AsicEnableBssSync(pAdapter);
			}
		}
		DBGPRINT(RT_DEBUG_TRACE, ("I/F(ra%d) Set_SSID_Proc::(Len=%d,Ssid=%s)\n", pObj->ioctl_if,
			pAdapter->ApCfg.MBSSID[pObj->ioctl_if].SsidLen, pAdapter->ApCfg.MBSSID[pObj->ioctl_if].Ssid));
	}
	else
		success = FALSE;

	return success;
}

/* 
    ==========================================================================
    Description:
        Set TxRate
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_TxRate_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	NdisZeroMemory(pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRates, MAX_LEN_OF_SUPPORTED_RATES);

	pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRatesIndex = simple_strtol(arg, 0, 10);
	// todo RTMPBuildDesireRate(pAd, pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].DesiredRatesIndex);
	
	//todo MlmeUpdateTxRates(pAd);

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set BasicRate
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT Set_BasicRate_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

    pAd->CommonCfg.BasicRateBitmap = (ULONG) simple_strtol(arg, 0, 10);

    if (pAd->CommonCfg.BasicRateBitmap > 4095) // (2 ^ MAX_LEN_OF_SUPPORTED_RATES) -1
        return FALSE;

    MlmeUpdateTxRates(pAd, FALSE, (UCHAR)pObj->ioctl_if);

    DBGPRINT(RT_DEBUG_TRACE, ("Set_BasicRate_Proc::(BasicRateBitmap=0x%08lx)\n", pAd->CommonCfg.BasicRateBitmap));
    
    return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set Beacon Period
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_BeaconPeriod_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	USHORT BeaconPeriod;
	INT   success = FALSE;

	BeaconPeriod = (USHORT) simple_strtol(arg, 0, 10);
	if((BeaconPeriod >= 20) && (BeaconPeriod < 1024))
	{
		pAd->CommonCfg.BeaconPeriod = BeaconPeriod;
		success = TRUE;		
	}
	else
		success = FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_BeaconPeriod_Proc::(BeaconPeriod=%d)\n", pAd->CommonCfg.BeaconPeriod));

	return success;
}

/* 
    ==========================================================================
    Description:
        Set Dtim Period
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_DtimPeriod_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	USHORT DtimPeriod;
	INT   success = FALSE;

	DtimPeriod = (USHORT) simple_strtol(arg, 0, 10);
	if((DtimPeriod >= 1) && (DtimPeriod <= 255))
	{
		pAd->ApCfg.DtimPeriod = DtimPeriod;
		success = TRUE;
	}
	else
		success = FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_DtimPeriod_Proc::(DtimPeriod=%d)\n", pAd->ApCfg.DtimPeriod));

	return success;
}

/* 
    ==========================================================================
    Description:
        Disable/enable OLBC detection manually
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_OLBCDetection_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	switch (simple_strtol(arg, 0, 10))
	{
		case 0: //enable OLBC detect
			pAd->CommonCfg.DisableOLBCDetect = 0;
			break;
		case 1: //disable OLBC detect
			pAd->CommonCfg.DisableOLBCDetect = 1;
			break;
		default:  //Invalid argument 
			return FALSE;
	}

	return TRUE;
}	

#ifdef WMM_SUPPORT
/* 
    ==========================================================================
    Description:
        Set WmmCapable Enable or Disable
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_WmmCapable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	BOOLEAN	bWmmCapable;
	POS_COOKIE	pObj= (POS_COOKIE)pAd->OS_Cookie;

	bWmmCapable = simple_strtol(arg, 0, 10);

	if (bWmmCapable == 1)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].bWmmCapable = TRUE;
	else if (bWmmCapable == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].bWmmCapable = FALSE;
	else
		return FALSE;  //Invalid argument 
	
#ifdef RTL865X_FAST_PATH	
	if (!isFastPathCapable(pAd)) {
		rtlairgo_fast_tx_unregister();
		rtl865x_extDev_unregisterUcastTxDev(pAd->net_dev);		
	}
#endif

	//Sync with the HT relate info. In N mode, we should re-enable it
	SetCommonHT(pAd);

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WmmCapable_Proc::(bWmmCapable=%d)\n", 
		pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].bWmmCapable));

	return TRUE;
}
#endif /* WMM_SUPPORT */

/* 
    ==========================================================================
    Description:
        Set No Forwarding Enable or Disable
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_NoForwarding_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG NoForwarding;

	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	NoForwarding = simple_strtol(arg, 0, 10);

	if (NoForwarding == 1)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].IsolateInterStaTraffic = TRUE;
	else if (NoForwarding == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].IsolateInterStaTraffic = FALSE;
	else
		return FALSE;  //Invalid argument 
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_NoForwarding_Proc::(NoForwarding=%ld)\n", 
		pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].IsolateInterStaTraffic));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set No Forwarding between each SSID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_NoForwardingBTNSSID_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG NoForwarding;

	NoForwarding = simple_strtol(arg, 0, 10);

	if (NoForwarding == 1)
		pAd->ApCfg.IsolateInterStaTrafficBTNBSSID = TRUE;
	else if (NoForwarding == 0)
		pAd->ApCfg.IsolateInterStaTrafficBTNBSSID = FALSE;
	else
		return FALSE;  //Invalid argument

	DBGPRINT(RT_DEBUG_TRACE, ("Set_NoForwardingBTNSSID_Proc::(NoForwarding=%ld)\n", pAd->ApCfg.IsolateInterStaTrafficBTNBSSID));

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set Hide SSID Enable or Disable
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_HideSSID_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	BOOLEAN bHideSsid;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	bHideSsid = simple_strtol(arg, 0, 10);

	if (bHideSsid == 1)
		bHideSsid = TRUE;
	else if (bHideSsid == 0)
		bHideSsid = FALSE;
	else
		return FALSE;  //Invalid argument 
	
	if (pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid != bHideSsid)
	{
		pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid = bHideSsid;
	}

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_HideSSID_Proc::(HideSSID=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].bHideSsid));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set IEEE8021X.
        This parameter is 1 when 802.1x-wep turn on, otherwise 0
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_IEEE8021X_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    ULONG ieee8021x;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	ieee8021x = simple_strtol(arg, 0, 10);

	if (ieee8021x == 1)
        pAd->ApCfg.MBSSID[pObj->ioctl_if].IEEE8021X = TRUE;
	else if (ieee8021x == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].IEEE8021X = FALSE;
	else
		return FALSE;  //Invalid argument 
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_IEEE8021X_Proc::(IEEE8021X=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].IEEE8021X));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set channel switch Period
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_CSPeriod_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	pAd->CommonCfg.RadarDetect.CSPeriod = (USHORT) simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("Set_CSPeriod_Proc::(CSPeriod=%d)\n", pAd->CommonCfg.RadarDetect.CSPeriod));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set pre-authentication enable or disable when WPA/WPA2 turn on
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_PreAuth_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    ULONG PreAuth;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	PreAuth = simple_strtol(arg, 0, 10);

	if (PreAuth == 1)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].PreAuth = TRUE;
	else if (PreAuth == 0)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].PreAuth = FALSE;
	else
		return FALSE;  //Invalid argument 
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_PreAuth_Proc::(PreAuth=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].PreAuth));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set VLAN's ID field
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_VLANID_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	pAd->ApCfg.MBSSID[pObj->ioctl_if].VLAN_VID = simple_strtol(arg, 0, 10);
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_VLANID_Proc::(VLAN_VID=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].VLAN_VID));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set VLAN's priority field
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_VLANPriority_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	pAd->ApCfg.MBSSID[pObj->ioctl_if].VLAN_Priority = simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_VLANPriority_Proc::(VLAN_Priority=%d)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].VLAN_Priority));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set Authentication mode
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_AuthMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG       i;//, sec_csr4 = 0;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;

	if ((strcmp(arg, "WEPAUTO") == 0) || (strcmp(arg, "wepauto") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if ((strcmp(arg, "OPEN") == 0) || (strcmp(arg, "open") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeOpen;
	else if ((strcmp(arg, "SHARED") == 0) || (strcmp(arg, "shared") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeShared;
	else if ((strcmp(arg, "WPAPSK") == 0) || (strcmp(arg, "wpapsk") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPAPSK;
	else if ((strcmp(arg, "WPA") == 0) || (strcmp(arg, "wpa") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA;
	else if ((strcmp(arg, "WPA2PSK") == 0) || (strcmp(arg, "wpa2psk") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA2PSK;
	else if ((strcmp(arg, "WPA2") == 0) || (strcmp(arg, "wpa2") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA2;
	else if ((strcmp(arg, "WPA1WPA2") == 0) || (strcmp(arg, "wpa1wpa2") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA1WPA2;
	else if ((strcmp(arg, "WPAPSKWPA2PSK") == 0) || (strcmp(arg, "wpapskwpa2psk") == 0))
		pAd->ApCfg.MBSSID[apidx].AuthMode = Ndis802_11AuthModeWPA1PSKWPA2PSK;
	else
		return FALSE;  

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (pAd->MacTab.Content[i].ValidAsCLI)
		{
			pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}
	pAd->ApCfg.MBSSID[apidx].PortSecured = WPA_802_1X_PORT_NOT_SECURED;
    RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx);

	pAd->ApCfg.MBSSID[apidx].DefaultKeyId  = 0;

	if(pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
		pAd->ApCfg.MBSSID[apidx].DefaultKeyId = 1;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_AuthMode_Proc::(AuthMode=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].AuthMode));		
	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set Encryption Type
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_EncrypType_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR		apidx = pObj->ioctl_if;
	
	if ((strcmp(arg, "NONE") == 0) || (strcmp(arg, "none") == 0))
		pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11WEPDisabled;
	else if ((strcmp(arg, "WEP") == 0) || (strcmp(arg, "wep") == 0))
		pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11WEPEnabled;
	else if ((strcmp(arg, "TKIP") == 0) || (strcmp(arg, "tkip") == 0))
		pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11Encryption2Enabled;
	else if ((strcmp(arg, "AES") == 0) || (strcmp(arg, "aes") == 0))
		pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11Encryption3Enabled;
	else if ((strcmp(arg, "TKIPAES") == 0) || (strcmp(arg, "tkipaes") == 0))
		pAd->ApCfg.MBSSID[apidx].WepStatus = Ndis802_11Encryption4Enabled;
	else
		return FALSE;

	if (pAd->ApCfg.MBSSID[apidx].WepStatus >= Ndis802_11Encryption2Enabled)
		pAd->ApCfg.MBSSID[apidx].DefaultKeyId = 1;

    RTMPMakeRSNIE(pAd, pAd->ApCfg.MBSSID[apidx].AuthMode, pAd->ApCfg.MBSSID[apidx].WepStatus, apidx);
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_EncrypType_Proc::(EncrypType=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].WepStatus));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set Default Key ID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_DefaultKeyID_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG KeyIdx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		pAd->ApCfg.MBSSID[apidx].DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  //Invalid argument 
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_DefaultKeyID_Proc::(DefaultKeyID(0~3)=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].DefaultKeyId));

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WEP KEY1
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key1_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
			pAd->SharedKey[apidx][0].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][0].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: //wep 40 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][0].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][0].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: //wep 104 Ascii type
			pAd->SharedKey[apidx][0].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][0].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][0].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][0].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::(Key1=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: //Invalid argument 
			pAd->SharedKey[apidx][0].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key1_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][0].CipherAlg = CipherAlg;

    // Set keys (into ASIC)
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   // not support
    else    // Old WEP stuff
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              0, 
                              pAd->SharedKey[apidx][0].CipherAlg, 
                              pAd->SharedKey[apidx][0].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WEP KEY2
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key2_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
			pAd->SharedKey[apidx][1].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][1].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: //wep 40 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][1].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][1].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: //wep 104 Ascii type
			pAd->SharedKey[apidx][1].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][1].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][1].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][1].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::(Key2=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: //Invalid argument 
			pAd->SharedKey[apidx][1].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key2_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][1].CipherAlg = CipherAlg;

	// Set keys (into ASIC)
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   // not support
    else    // Old WEP stuff
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              1, 
                              pAd->SharedKey[apidx][1].CipherAlg, 
                              pAd->SharedKey[apidx][1].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WEP KEY3
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key3_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;


	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
			pAd->SharedKey[apidx][2].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][2].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: //wep 40 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][2].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][2].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: //wep 104 Ascii type
			pAd->SharedKey[apidx][2].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][2].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][2].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][2].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key3_Proc::(Key3=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: //Invalid argument 
			pAd->SharedKey[apidx][2].KeyLen = 0;
			DBGPRINT(RT_DEBUG_ERROR, ("IF(ra%d) Set_Key3_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][2].CipherAlg = CipherAlg;

	// Set keys (into ASIC)
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   // not support
    else    // Old WEP stuff
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              2, 
                              pAd->SharedKey[apidx][2].CipherAlg, 
                              pAd->SharedKey[apidx][2].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Set WEP KEY4
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_Key4_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT		KeyLen;
	INT		i;
	UCHAR	CipherAlg = CIPHER_WEP64;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;

	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
			pAd->SharedKey[apidx][3].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][3].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 10: //wep 40 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][3].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][3].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP64;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		case 13: //wep 104 Ascii type
			pAd->SharedKey[apidx][3].KeyLen = KeyLen;
			NdisMoveMemory(pAd->SharedKey[apidx][3].Key, arg, KeyLen);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Ascii"));		
			break;
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pAd->SharedKey[apidx][3].KeyLen = KeyLen/2 ;
			AtoH(arg, pAd->SharedKey[apidx][3].Key, KeyLen/2);
			CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_Key4_Proc::(Key4=%s and type=%s)\n", apidx, arg, "Hex"));		
			break;
		default: //Invalid argument 
			pAd->SharedKey[apidx][3].KeyLen = 0;
			DBGPRINT(RT_DEBUG_ERROR, ("IF(ra%d) Set_Key4_Proc::Invalid argument (=%s)\n", apidx, arg));		
			return FALSE;
	}

	pAd->SharedKey[apidx][3].CipherAlg = CipherAlg;

	// Set keys (into ASIC)
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
        ;   // not support
    else    // Old WEP stuff
    {
        AsicAddSharedKeyEntry(pAd, 
                              apidx, 
                              3, 
                              pAd->SharedKey[apidx][3].CipherAlg, 
                              pAd->SharedKey[apidx][3].Key, 
                              NULL,
                              NULL);
    }

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set Access ctrol policy
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AccessPolicy_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	switch (simple_strtol(arg, 0, 10))
	{
		case 0: //Disable
			pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy = 0;
			break;
		case 1: // Allow All, and ACL is positive.
			pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy = 1;
			break;
		case 2: // Reject All, and ACL is negative.
			pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy = 2;
			break;
		default: //Invalid argument 
			DBGPRINT(RT_DEBUG_ERROR, ("Set_AccessPolicy_Proc::Invalid argument (=%s)\n", arg));		
			return FALSE;
	}

	// check if the change in ACL affects any existent association
	ApUpdateAccessControlList(pAd, pObj->ioctl_if);	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_AccessPolicy_Proc::(AccessPolicy=%ld)\n", pObj->ioctl_if, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy));

	return TRUE;	
}

/* 
    ==========================================================================
    Description:
        Set Access ctrol mac table list
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AccessControlList_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR					macAddr[MAC_ADDR_LEN];
	RT_802_11_ACL			acl;
	CHAR					*this_char;
	CHAR					*value;
	INT						i, j;
	BOOLEAN					isDuplicate=FALSE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	memset(&acl, 0x00, sizeof(RT_802_11_ACL));
	
	this_char = strsep((char **)&arg, ";");

	if (*this_char == '\0')
	{
		DBGPRINT(RT_DEBUG_WARN, ("The AccessControlList%d entered is NULL ===> Make the list empty!\n", pObj->ioctl_if));
	}
	else
	{
		do
		{
			if (strlen(this_char) != 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
			{
				return FALSE;
			}
	        for (i=0, value = rstrtok(this_char,":"); value; value = rstrtok(NULL,":")) 
			{
				if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				{
					return FALSE;  //Invalid
				}
				AtoH(value, &macAddr[i++], 2);
			}

			if (i != 6)
			{
				return FALSE;  //Invalid
			}

			//Check if this entry is duplicate.
			isDuplicate = FALSE;
			for (j=0; j<acl.Num; j++)
			{
				if (memcmp(acl.Entry[j].Addr, &macAddr, 6) == 0)
				{
					isDuplicate = TRUE;
				}
			}

			if (!isDuplicate)
			{
				NdisMoveMemory(acl.Entry[acl.Num++].Addr, &macAddr, 6);
			}

			if (acl.Num == MAX_NUM_OF_ACL_LIST)
		    {
				DBGPRINT(RT_DEBUG_WARN, ("The AccessControlList is full, and no more entry can join the list!\n"));
	        	DBGPRINT(RT_DEBUG_WARN, ("The last entry of ACL is %02x:%02x:%02x:%02x:%02x:%02x\n",
	        		macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]));
				break;
			}
		}while ((this_char = strsep((char **)&arg, ";")) != NULL);
	}
	ASSERT(acl.Num <= MAX_NUM_OF_ACL_LIST);
	acl.Policy = pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy;
	NdisMoveMemory(&pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList, &acl, sizeof(RT_802_11_ACL));

#if 0
	if (pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num > MAX_NUM_OF_ACL_LIST)
		pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num = MAX_NUM_OF_ACL_LIST;
	else
		pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num = acl.Num;
#endif

	// check if the change in ACL affects any existent assocition
	ApUpdateAccessControlList(pAd, pObj->ioctl_if);
	DBGPRINT(RT_DEBUG_TRACE, ("Set::Set_AccessControlList_Proc(Policy=%ld, Entry#=%ld\n",
        pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Policy, pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num));

#ifdef DBG
	DBGPRINT(RT_DEBUG_TRACE, ("=============== Entry ===============\n"));
	for (i=0; i<pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Num; i++)
	{
		printk("Entry #%02d: ", i+1);
		for (j=0; j<6; j++)
		   printk("%02X ", pAd->ApCfg.MBSSID[pObj->ioctl_if].AccessControlList.Entry[i].Addr[j]);
		printk("\n");
	}
#endif
	return TRUE;
}

#ifdef DBG
static void _rtmp_hexdump(int level, const char *title, const u8 *buf,
			 size_t len, int show)
{
	size_t i;
	if (level < RTDebugLevel)
		return;
	printk("%s - hexdump(len=%lu):", title, (unsigned long) len);
	if (show) {
		for (i = 0; i < len; i++)
			printk(" %02x", buf[i]);
	} else {
		printk(" [REMOVED]");
	}
	printk("\n");
}

void rtmp_hexdump(int level, const char *title, const u8 *buf, size_t len)
{
	_rtmp_hexdump(level, title, buf, len, 1);
}
#endif

/* 
    ==========================================================================
    Description:
        Set WPA PSK key

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 WPA pre-shared key string

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_AP_WPAPSK_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR       keyMaterial[40];
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR	apidx = pObj->ioctl_if;

	DBGPRINT(RT_DEBUG_TRACE, ("Set_WPAPSK_Proc::(WPAPSK=%s)\n", arg));

	if ((strlen(arg) < 8) || (strlen(arg) > 64))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set failed!!(WPAPSK=%s), WPAPSK key-string required 8 ~ 64 characters \n", arg));
		return FALSE;
	}

	if (strlen(arg) == 64)
	{
	    AtoH(arg, pAd->ApCfg.MBSSID[apidx].PMK, 32);
	}
	else
	{
	    PasswordHash((CHAR *)arg, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].SsidLen, keyMaterial);
	    NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].PMK, keyMaterial, 32);		
	}

#ifdef WSC_AP_SUPPORT
    NdisZeroMemory(pAd->ApCfg.MBSSID[apidx].WscControl.WpaPsk, 64);
    pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen = 0;
    pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen = strlen(arg);
    NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].WscControl.WpaPsk, arg, pAd->ApCfg.MBSSID[apidx].WscControl.WpaPskLen);    
#endif // WSC_AP_SUPPORT //    

	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Reset statistics counter

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/

INT	Set_RadioOn_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR radio;

	radio = simple_strtol(arg, 0, 10);

	if (radio)
	{
		MlmeRadioOn(pAd);
		DBGPRINT(RT_DEBUG_TRACE, ("==>Set_RadioOn_Proc (ON)\n"));
	}
	else
	{
		MlmeRadioOff(pAd);
		DBGPRINT(RT_DEBUG_TRACE, ("==>Set_RadioOn_Proc (OFF)\n"));
	}
	
	return TRUE;
}

/* 
    ==========================================================================
    Description:
        Issue a site survey command to driver
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 set site_survey
    ==========================================================================
*/
INT Set_SiteSurvey_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    ApSiteSurvey(pAd);
    
    DBGPRINT(RT_DEBUG_TRACE, ("Set_SiteSurvey_Proc\n"));

    return TRUE;
}

INT Show_DriverInfo_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    printk("Driver version: %s\n", AP_DRIVER_VERSION);
    
    return TRUE;
}

INT	Show_MacTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT i;//, QueIdx=0;
    UINT32 RegValue;
	//PRXD_STRUC pRxD;
	//PTXD_STRUC pTxD;
	//PRTMP_TX_RING	pTxRing = &pAd->TxRing[QueIdx];	
	//PRTMP_MGMT_RING	pMgmtRing = &pAd->MgmtRing;	
	//PRTMP_RX_RING	pRxRing = &pAd->RxRing;	
	
	printk("\n");
	RTMP_IO_READ32(pAd, BKOFF_SLOT_CFG, &RegValue);
	printk("BackOff Slot      : %s slot time, BKOFF_SLOT_CFG(0x1104) = 0x%08x\n", 
			OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SHORT_SLOT_INUSED) ? "short" : "long",
 			RegValue);

	printk("HT Operating Mode : %d\n", pAd->CommonCfg.AddHTInfo.AddHtInfo2.OperaionMode);
	printk("\n");
	
	printk("\n%-19s%-4s%-4s%-4s%-4s%-8s%-7s%-7s%-7s%-10s%-6s%-6s%-6s%-6s\n",
		   "MAC", "AID", "BSS", "PSM", "WMM", "MIMOPS", "RSSI0", "RSSI1", "RSSI2", "PhMd", "BW", "MCS", "SGI", "STBC");
	
	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if ((pEntry->ValidAsCLI || pEntry->ValidAsApCli) && (pEntry->Sst == SST_ASSOC))
		{
			printk("%02X:%02X:%02X:%02X:%02X:%02X  ",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			printk("%-4d", (int)pEntry->Aid);
			printk("%-4d", (int)pEntry->apidx);
			printk("%-4d", (int)pEntry->PsMode);
			printk("%-4d", (int)CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_WMM_CAPABLE));
			printk("%-8d", (int)pEntry->MmpsMode);
			printk("%-7d", pEntry->RssiSample.AvgRssi0);
			printk("%-7d", pEntry->RssiSample.AvgRssi1);
			printk("%-7d", pEntry->RssiSample.AvgRssi2);
			printk("%-10s", GetPhyMode(pEntry->HTPhyMode.field.MODE));
			printk("%-6s", GetBW(pEntry->HTPhyMode.field.BW));
			printk("%-6d", pEntry->HTPhyMode.field.MCS);
			printk("%-6d", pEntry->HTPhyMode.field.ShortGI);
			printk("%-6d", pEntry->HTPhyMode.field.STBC);
			printk("%-10d, %d, %d%%\n", pEntry->DebugFIFOCount, pEntry->DebugTxCount, 
						(pEntry->DebugTxCount) ? ((pEntry->DebugTxCount-pEntry->DebugFIFOCount)*100/pEntry->DebugTxCount) : 0);
			printk("\n");
		}
	} 

	return TRUE;
}

INT	Show_BaTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT i, j;
	BA_ORI_ENTRY *pOriBAEntry;
	BA_REC_ENTRY *pRecBAEntry;
	UCHAR		 tmpBuf[6];

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];
		if (((pEntry->ValidAsCLI || pEntry->ValidAsApCli) && (pEntry->Sst == SST_ASSOC))
			|| (pEntry->ValidAsWDS)
			|| (pEntry->ValidAsMesh))
		{
			if (pEntry->ValidAsApCli)
				strcpy(tmpBuf, "ApCli");
			else if (pEntry->ValidAsWDS)
				strcpy(tmpBuf, "WDS");
			else if (pEntry->ValidAsMesh)
				strcpy(tmpBuf, "Mesh");
			else
				strcpy(tmpBuf, "STA");
		
			printk("%02X:%02X:%02X:%02X:%02X:%02X (Aid = %d) (%s) -\n",
				pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5], pEntry->Aid, tmpBuf);
			
			printk("[Recipient]\n");
			for (j=0; j < NUM_OF_TID; j++)
			{
				if (pEntry->BARecWcidArray[j] != 0)
				{
					pRecBAEntry =&pAd->BATable.BARecEntry[pEntry->BARecWcidArray[j]];
					printk("TID=%d, BAWinSize=%d, LastIndSeq=%d, ReorderingPkts=%d\n", j, pRecBAEntry->BAWinSize, pRecBAEntry->LastIndSeq, pRecBAEntry->list.qlen);
				}
			}
			printk("\n");

			printk("[Originator]\n");
			for (j=0; j < NUM_OF_TID; j++)
			{
				if (pEntry->BAOriWcidArray[j] != 0)
				{
					pOriBAEntry =&pAd->BATable.BAOriEntry[pEntry->BAOriWcidArray[j]];
					printk("TID=%d, BAWinSize=%d, StartSeq=%d, CurTxSeq=%d\n", j, pOriBAEntry->BAWinSize, pOriBAEntry->Sequence, pEntry->TxSeq[j]);
				}
			}
			printk("\n\n");
		}
	}

	return TRUE;
}


#ifdef DBG_DIAGNOSE
INT Set_DiagOpt_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG diagOpt;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	diagOpt = simple_strtol(arg, 0, 10);

	DBGPRINT(RT_DEBUG_TRACE, ("DiagOpt=%d!\n", diagOpt));


	return TRUE;
}

INT Show_Diag_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	RtmpDiagStruct	*pDiag;
	UCHAR			i, start, stop, McsIdx, SwQNumLevel, TxDescNumLevel;
	unsigned long		irqFlags;
	
	pDiag = &pAd->DiagStruct;
	
	if (pDiag->inited == FALSE)
		return TRUE;

	RTMP_IRQ_LOCK(&pAd->irq_lock, irqFlags);
	start = pDiag->ArrayStartIdx;
	stop = pDiag->ArrayCurIdx;
	printk("Start=%d, stop=%d!\n\n", start, stop);
	printk("    %-12s", "Time(Sec)");
	for(i=1; i< DIAGNOSE_TIME; i++)
	{
		printk("%-7d", i);
	}
	printk("\n    -------------------------------------------------------------------------------\n");
	printk("Tx Info:\n");
	printk("    %-12s", "TxDataCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->TxDataCnt[i]);
	}
	printk("\n    %-12s", "TxFailCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->TxFailCnt[i]);
	}
	printk("\n    %-12s", "TxAggCnt");
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->TxAggCnt[i]);
	}
	printk("\n");

	
	printk("\n    %-12s\n", "Sw-Queued TxSwQCnt");
	for (SwQNumLevel = 0 ; SwQNumLevel < 9; SwQNumLevel++)
	{	
		if (SwQNumLevel == 8)
			printk("\t>%-5d",  SwQNumLevel);
		else
			printk("\t%-6d", SwQNumLevel);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			printk("%-7d", pDiag->TxSWQueCnt[i][SwQNumLevel]);
		}
		printk("\n");
	}
	
	printk("\n    %-12s\n", "DMA-Queued TxDescCnt");
	for(TxDescNumLevel = 0; TxDescNumLevel < 16; TxDescNumLevel++)
	{
		if (TxDescNumLevel == 15)
			printk("\t>%-5d",  TxDescNumLevel);
		else
			printk("\t%-6d",  TxDescNumLevel);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			printk("%-7d", pDiag->TxDescCnt[i][TxDescNumLevel]);
		}
		printk("\n");
	}
	
	printk("\n    %-12s\n", "Tx-Agged AMPDUCnt");
	for (McsIdx =0 ; McsIdx < 16; McsIdx++)
	{
		printk("\t%-6d", (McsIdx+1));
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{		
			printk("%d(%d%%)  ", pDiag->TxAMPDUCnt[i][McsIdx], pDiag->TxAMPDUCnt[i][McsIdx] ? (pDiag->TxAMPDUCnt[i][McsIdx] * 100 / pDiag->TxAggCnt[i]) : 0);
		}		
//		printk("\n\t%-6s", "R(%)");
//		for (i = start, count=0; count < DIAGNOSE_TIME;  i = (i+1) % DIAGNOSE_TIME, count++)
//		{
//			printk("%-5d", pDiag->TxAMPDUCnt[i][McsIdx] ? (pDiag->TxAMPDUCnt[i][McsIdx] * 100 / pDiag->TxTotalCnt[i]) : 0);
//		}
		printk("\n");
	}
	printk("\n    %-12s\n", "TxMcsCnt");
	for (McsIdx =0 ; McsIdx < 16; McsIdx++)
	{
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			printk("%-7d", pDiag->TxMcsCnt[i][McsIdx]);
		}
		printk("\n");
	}
	
	printk("Rx Info\n");
	printk("    %-12s", "RxDataCnt");	
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->RxDataCnt[i]);
	}
	printk("\n    %-12s", "RxCrcErrCnt");	
	for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
	{
		printk("%-7d", pDiag->RxCrcErrCnt[i]);
	}
	printk("\n    %-12s\n", "RxMcsCnt");
	for (McsIdx =0 ; McsIdx < 16; McsIdx++)
	{
		printk("\t%-6d", McsIdx);
		for (i = start; i != stop;  i = (i+1) % DIAGNOSE_TIME)
		{
			printk("%-7d", pDiag->RxMcsCnt[i][McsIdx]);
		}
		printk("\n");
	}
	printk("\n-------------\n");

	RTMP_IRQ_UNLOCK(&pAd->irq_lock, irqFlags);

	return TRUE;

}
#endif // DBG_DIAGNOSE //


INT	Show_Sat_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	// Sanity check for calculation of sucessful count

	printk("TransmittedFragmentCount = %d\n", pAd->WlanCounters.TransmittedFragmentCount.u.LowPart);
	printk("MulticastTransmittedFrameCount = %d\n", pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart);
	printk("FailedCount = %d\n", pAd->WlanCounters.FailedCount.u.LowPart);
	printk("RetryCount = %d\n", pAd->WlanCounters.RetryCount.u.LowPart);
	printk("MultipleRetryCount = %d\n", pAd->WlanCounters.MultipleRetryCount.u.LowPart);
	printk("RTSSuccessCount = %d\n", pAd->WlanCounters.RTSSuccessCount.u.LowPart);
	printk("RTSFailureCount = %d\n", pAd->WlanCounters.RTSFailureCount.u.LowPart);
	printk("ACKFailureCount = %d\n", pAd->WlanCounters.ACKFailureCount.u.LowPart);
	printk("FrameDuplicateCount = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart);
	printk("ReceivedFragmentCount = %d\n", pAd->WlanCounters.ReceivedFragmentCount.u.LowPart);
	printk("MulticastReceivedFrameCount = %d\n", pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart);
#if DBG 		
	printk("RealFcsErrCount = %d\n", pAd->RalinkCounters.RealFcsErrCount.u.LowPart);
#else
	printk("FCSErrorCount = %d\n", pAd->WlanCounters.FCSErrorCount.u.LowPart);
	printk("FrameDuplicateCount.LowPart = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart / 100);
#endif

	printk("\n===Some 11n statistics variables: \n");
	// Some 11n statistics variables
	printk("TransmittedAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedAMSDUCount.u.LowPart);
	printk("TransmittedOctetsInAMSDU = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedOctetsInAMSDU.QuadPart);
	printk("ReceivedAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.ReceivedAMSDUCount.u.LowPart);	
	printk("ReceivedOctesInAMSDUCount = %ld\n", (ULONG)pAd->RalinkCounters.ReceivedOctesInAMSDUCount.QuadPart);	
	printk("TransmittedAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedAMPDUCount.u.LowPart);
	printk("TransmittedMPDUsInAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedMPDUsInAMPDUCount.u.LowPart);
	printk("TransmittedOctetsInAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.TransmittedOctetsInAMPDUCount.u.LowPart);
	printk("MPDUInReceivedAMPDUCount = %ld\n", (ULONG)pAd->RalinkCounters.MPDUInReceivedAMPDUCount.u.LowPart);

{
	int apidx;
		
	for (apidx=0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		printk("-- IF-ra%d -- \n", apidx);
		printk("Packets Received = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].RxCount);
		printk("Packets Sent = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].TxCount);
		printk("Bytes Received = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].ReceivedByteCount);
		printk("Byte Sent = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].TransmittedByteCount);
		printk("Error Packets Received = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].RxErrorCount);
		printk("Drop Received Packets = %ld\n", (ULONG)pAd->ApCfg.MBSSID[apidx].RxDropCount);
		printk("-- IF-ra%d end -- \n", apidx);
	}
}

{
	int i, j, k;
	PMAC_TABLE_ENTRY pEntry;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &pAd->MacTab.Content[i];
		if (pEntry->ValidAsCLI && (pEntry->Sst == SST_ASSOC))
		{

			printk("\n%02X:%02X:%02X:%02X:%02X:%02X - ",
				   pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				   pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			printk("%-4d\n", (int)pEntry->Aid);

			for (j=15; j>=0; j--)
			{
				if ((pEntry->TXMCSExpected[j] != 0) || (pEntry->TXMCSFailed[j] !=0))
				{
					printk("MCS[%02d]: Expected %u, Successful %u (%d%%), Failed %u\n",
						   j, pEntry->TXMCSExpected[j], pEntry->TXMCSSuccessful[j], 
						   pEntry->TXMCSExpected[j] ? (100*pEntry->TXMCSSuccessful[j])/pEntry->TXMCSExpected[j] : 0,
						   pEntry->TXMCSFailed[j]);
					for(k=15; k>=0; k--)
					{
						if (pEntry->TXMCSAutoFallBack[j][k] != 0)
						{
							printk("\t\t\tAutoMCS[%02d]: %u (%d%%)\n", k, pEntry->TXMCSAutoFallBack[j][k],
								   (100*pEntry->TXMCSAutoFallBack[j][k])/pEntry->TXMCSExpected[j]);
						}
					}
				}
			}
		}
	}

}

{
	TX_AGG_CNT_STRUC	TxAggCnt;
	TX_AGG_CNT0_STRUC	TxAggCnt0;
	TX_AGG_CNT1_STRUC	TxAggCnt1;
	TX_AGG_CNT2_STRUC	TxAggCnt2;
	TX_AGG_CNT3_STRUC	TxAggCnt3;	
	UINT32				totalCount;
	
	RTMP_IO_READ32(pAd, TX_AGG_CNT, &TxAggCnt.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT0, &TxAggCnt0.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT1, &TxAggCnt1.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT2, &TxAggCnt2.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT3, &TxAggCnt3.word);

	totalCount = TxAggCnt.field.NonAggTxCount + TxAggCnt.field.AggTxCount;
	printk("Tx_Agg_Cnt->NonAggTxCount=%d!,  AggTxCount=%d!\n", TxAggCnt.field.NonAggTxCount, TxAggCnt.field.AggTxCount);
	printk("\tTx_Agg_Cnt 1 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize1Count, TxAggCnt0.field.AggSize1Count ? (TxAggCnt0.field.AggSize1Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 2 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize2Count, TxAggCnt0.field.AggSize2Count ? (TxAggCnt0.field.AggSize2Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 3 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize3Count, TxAggCnt1.field.AggSize3Count ? (TxAggCnt1.field.AggSize3Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 4 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize4Count, TxAggCnt1.field.AggSize4Count ? (TxAggCnt1.field.AggSize4Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 5 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize5Count, TxAggCnt2.field.AggSize5Count ? (TxAggCnt2.field.AggSize5Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 6 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize6Count, TxAggCnt2.field.AggSize6Count ? (TxAggCnt2.field.AggSize6Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 7 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize7Count, TxAggCnt3.field.AggSize7Count ? (TxAggCnt3.field.AggSize7Count * 100 / totalCount) : 0);
	printk("\tTx_Agg_Cnt 8 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize8Count, (TxAggCnt3.field.AggSize8Count ? (TxAggCnt3.field.AggSize8Count * 100 / totalCount) : 0));
	printk("====================\n");
	
}
	return TRUE;
}



INT	Show_Sat_Reset_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	// Sanity check for calculation of sucessful count

	printk("TransmittedFragmentCount = %d\n", pAd->WlanCounters.TransmittedFragmentCount.u.LowPart);
	printk("MulticastTransmittedFrameCount = %d\n", pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart);
	printk("FailedCount = %d\n", pAd->WlanCounters.FailedCount.u.LowPart);
	printk("RetryCount = %d\n", pAd->WlanCounters.RetryCount.u.LowPart);
	printk("MultipleRetryCount = %d\n", pAd->WlanCounters.MultipleRetryCount.u.LowPart);
	printk("RTSSuccessCount = %d\n", pAd->WlanCounters.RTSSuccessCount.u.LowPart);
	printk("RTSFailureCount = %d\n", pAd->WlanCounters.RTSFailureCount.u.LowPart);
	printk("ACKFailureCount = %d\n", pAd->WlanCounters.ACKFailureCount.u.LowPart);
	printk("FrameDuplicateCount = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart);
	printk("ReceivedFragmentCount = %d\n", pAd->WlanCounters.ReceivedFragmentCount.u.LowPart);
	printk("MulticastReceivedFrameCount = %d\n", pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart);
#ifdef DBG 		
	printk("RealFcsErrCount = %d\n", pAd->RalinkCounters.RealFcsErrCount.u.LowPart);
#else
	printk("FCSErrorCount = %d\n", pAd->WlanCounters.FCSErrorCount.u.LowPart);
	printk("FrameDuplicateCount.LowPart = %d\n", pAd->WlanCounters.FrameDuplicateCount.u.LowPart / 100);
#endif

	pAd->WlanCounters.TransmittedFragmentCount.u.LowPart = 0;
	pAd->WlanCounters.MulticastTransmittedFrameCount.u.LowPart = 0;
	pAd->WlanCounters.FailedCount.u.LowPart = 0;
	pAd->WlanCounters.RetryCount.u.LowPart = 0;
	pAd->WlanCounters.MultipleRetryCount.u.LowPart = 0;
	pAd->WlanCounters.RTSSuccessCount.u.LowPart = 0;
	pAd->WlanCounters.RTSFailureCount.u.LowPart = 0;
	pAd->WlanCounters.ACKFailureCount.u.LowPart = 0;
	pAd->WlanCounters.FrameDuplicateCount.u.LowPart = 0;
	pAd->WlanCounters.ReceivedFragmentCount.u.LowPart = 0;
	pAd->WlanCounters.MulticastReceivedFrameCount.u.LowPart = 0;
#ifdef DBG 		
	pAd->RalinkCounters.RealFcsErrCount.u.LowPart = 0;
#else
	pAd->WlanCounters.FCSErrorCount.u.LowPart = 0;
	pAd->WlanCounters.FrameDuplicateCount.u.LowPart = 0;
#endif



{
	int i, j, k;
	PMAC_TABLE_ENTRY pEntry;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		pEntry = &pAd->MacTab.Content[i];
		if (pEntry->ValidAsCLI && (pEntry->Sst == SST_ASSOC))
		{

			printk("\n%02X:%02X:%02X:%02X:%02X:%02X - ",
				   pEntry->Addr[0], pEntry->Addr[1], pEntry->Addr[2],
				   pEntry->Addr[3], pEntry->Addr[4], pEntry->Addr[5]);
			printk("%-4d\n", (int)pEntry->Aid);

			for (j=15; j>=0; j--)
			{
				if ((pEntry->TXMCSExpected[j] != 0) || (pEntry->TXMCSFailed[j] !=0))
				{
					printk("MCS[%02d]: Expected %u, Successful %u (%d%%), Failed %u\n",
						   j, pEntry->TXMCSExpected[j], pEntry->TXMCSSuccessful[j], 
						   pEntry->TXMCSExpected[j] ? (100*pEntry->TXMCSSuccessful[j])/pEntry->TXMCSExpected[j] : 0,
						   pEntry->TXMCSFailed[j]
						   );
					for(k=15; k>=0; k--)
					{
						if (pEntry->TXMCSAutoFallBack[j][k] != 0)
						{
							printk("\t\t\tAutoMCS[%02d]: %u (%d%%)\n", k, pEntry->TXMCSAutoFallBack[j][k],
								   (100*pEntry->TXMCSAutoFallBack[j][k])/pEntry->TXMCSExpected[j]);
						}
					}
				}
			}
		}
		for (j=0; j<16; j++)
		{
			pEntry->TXMCSExpected[j] = 0;
			pEntry->TXMCSSuccessful[j] = 0;
			pEntry->TXMCSFailed[j] = 0;
			for(k=15; k>=0; k--)
			{
				pEntry->TXMCSAutoFallBack[j][k] = 0;
			}
		}
	}

{
	TX_AGG_CNT_STRUC	TxAggCnt;
	TX_AGG_CNT0_STRUC	TxAggCnt0;
	TX_AGG_CNT1_STRUC	TxAggCnt1;
	TX_AGG_CNT2_STRUC	TxAggCnt2;
	TX_AGG_CNT3_STRUC	TxAggCnt3;	
	UINT32				totalCount, ratio1, ratio2, ratio3, ratio4, ratio5, ratio6, ratio7, ratio8;
	
	RTMP_IO_READ32(pAd, TX_AGG_CNT, &TxAggCnt.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT0, &TxAggCnt0.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT1, &TxAggCnt1.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT2, &TxAggCnt2.word);
	RTMP_IO_READ32(pAd, TX_AGG_CNT3, &TxAggCnt3.word);

	totalCount = TxAggCnt.field.NonAggTxCount + TxAggCnt.field.AggTxCount;
	ratio1 = TxAggCnt0.field.AggSize1Count ? (TxAggCnt0.field.AggSize1Count * 100 / totalCount) : 0;
	ratio2 = TxAggCnt0.field.AggSize2Count ? (TxAggCnt0.field.AggSize2Count * 100 / totalCount) : 0;
	ratio3 = TxAggCnt1.field.AggSize3Count ? (TxAggCnt1.field.AggSize3Count * 100 / totalCount) : 0;
	ratio4 = TxAggCnt1.field.AggSize4Count ? (TxAggCnt1.field.AggSize4Count * 100 / totalCount) : 0;
	ratio5 = TxAggCnt2.field.AggSize5Count ? (TxAggCnt2.field.AggSize5Count * 100 / totalCount) : 0;
	ratio6 = TxAggCnt2.field.AggSize6Count ? (TxAggCnt2.field.AggSize6Count * 100 / totalCount) : 0;
	ratio7 = TxAggCnt3.field.AggSize7Count ? (TxAggCnt3.field.AggSize7Count * 100 / totalCount) : 0;
	ratio8 = TxAggCnt3.field.AggSize8Count ? (TxAggCnt3.field.AggSize8Count * 100 / totalCount) : 0;

	printk("Tx_Agg_Cnt->NonAggTxCount=%d!,  AggTxCount=%d!\n", TxAggCnt.field.NonAggTxCount, TxAggCnt.field.AggTxCount);
	printk("\tTx_Agg_Cnt 1 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize1Count, ratio1);
	printk("\tTx_Agg_Cnt 2 MPDU=%d(%d%%)!\n", TxAggCnt0.field.AggSize2Count, ratio2);
	printk("\tTx_Agg_Cnt 3 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize3Count, ratio3);
	printk("\tTx_Agg_Cnt 4 MPDU=%d(%d%%)!\n", TxAggCnt1.field.AggSize4Count, ratio4);
	printk("\tTx_Agg_Cnt 5 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize5Count, ratio5);
	printk("\tTx_Agg_Cnt 6 MPDU=%d(%d%%)!\n", TxAggCnt2.field.AggSize6Count, ratio6);
	printk("\tTx_Agg_Cnt 7 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize7Count, ratio7);
	printk("\tTx_Agg_Cnt 8 MPDU=%d(%d%%)!\n", TxAggCnt3.field.AggSize8Count, ratio8);
	printk("\tRatio: 1(%d%%), 2(%d%%), 3(%d%%), 4(%d%%), 5(%d%%), 6(%d%%), 7(%d%%), 8(%d%%)!\n",
			ratio1+ratio2+ratio3+ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio2+ratio3+ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio3+ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio4+ratio5+ratio6+ratio7+ratio8,
			ratio5+ratio6+ratio7+ratio8,
			ratio6+ratio7+ratio8,
			ratio7+ratio8,
			ratio8);
	printk("====================\n");
	
}
}


	return TRUE;
}


#ifdef MAT_SUPPORT
INT	Show_MATTable_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	extern VOID dumpIPMacTb(MAT_STRUCT *pMatCfg, int index);
	extern NDIS_STATUS dumpSesMacTb(MAT_STRUCT *pMatCfg, int hashIdx);
	extern NDIS_STATUS dumpUidMacTb(MAT_STRUCT *pMatCfg, int hashIdx);
	extern NDIS_STATUS dumpIPv6MacTb(MAT_STRUCT *pMatCfg, int hashIdx);

	dumpIPMacTb(&pAd->MatCfg, -1);
	dumpSesMacTb(&pAd->MatCfg, -1);
	dumpUidMacTb(&pAd->MatCfg, -1);
	dumpIPv6MacTb(&pAd->MatCfg, -1);

	printk("Default BroadCast Address=%02x:%02x:%02x:%02x:%02x:%02x!\n", BROADCAST_ADDR[0], BROADCAST_ADDR[1],
			BROADCAST_ADDR[2], BROADCAST_ADDR[3], BROADCAST_ADDR[4], BROADCAST_ADDR[5]);
	return TRUE;
}
#endif // MAT_SUPPORT //


/* 
    ==========================================================================
    Description:
        It only shall be queried by 802.1x daemon for querying radius configuration.        
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlQueryRadiusConf(
	IN PRTMP_ADAPTER pAd, 
	IN struct iwreq *wrq)
{
	UCHAR	apidx, srv_idx, keyidx, KeyLen = 0;
	RADIUS_CONF	RadiusConf;

	DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlQueryRadiusConf==>\n"));
	
	NdisZeroMemory((PUCHAR)&RadiusConf, sizeof(RADIUS_CONF));

	// get MBSS number
	RadiusConf.mbss_num = pAd->ApCfg.BssidNum;

	// get own ip address
	RadiusConf.own_ip_addr = pAd->ApCfg.own_ip_addr;

	// get retry interval
	RadiusConf.retry_interval = pAd->ApCfg.retry_interval;

	// get session timeout interval
	RadiusConf.session_timeout_interval = pAd->ApCfg.session_timeout_interval;

	// get EAPifname
	if (pAd->ApCfg.EAPifname_len > 0)
	{
		RadiusConf.EAPifname_len = pAd->ApCfg.EAPifname_len;
		NdisMoveMemory(RadiusConf.EAPifname, pAd->ApCfg.EAPifname, pAd->ApCfg.EAPifname_len);
	}	

	// get PreAuthifname
	if (pAd->ApCfg.PreAuthifname_len > 0)
	{
		RadiusConf.PreAuthifname_len = pAd->ApCfg.PreAuthifname_len;
		NdisMoveMemory(RadiusConf.PreAuthifname, pAd->ApCfg.PreAuthifname, pAd->ApCfg.PreAuthifname_len);
	}	

	for (apidx = 0; apidx < pAd->ApCfg.BssidNum; apidx++)
	{
		RadiusConf.RadiusInfo[apidx].radius_srv_num = pAd->ApCfg.MBSSID[apidx].radius_srv_num;
	
		// prepare radius ip, port and key
		for (srv_idx = 0; srv_idx < pAd->ApCfg.MBSSID[apidx].radius_srv_num; srv_idx++)
		{
			if (pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_idx].radius_ip != 0)
		{
				RadiusConf.RadiusInfo[apidx].radius_srv_info[srv_idx].radius_ip = pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_idx].radius_ip;
				RadiusConf.RadiusInfo[apidx].radius_srv_info[srv_idx].radius_port = pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_idx].radius_port;
				RadiusConf.RadiusInfo[apidx].radius_srv_info[srv_idx].radius_key_len = pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_idx].radius_key_len;
				if (RadiusConf.RadiusInfo[apidx].radius_srv_info[srv_idx].radius_key_len > 0)
				{
					NdisMoveMemory(RadiusConf.RadiusInfo[apidx].radius_srv_info[srv_idx].radius_key, 
									pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_idx].radius_key, 
									pAd->ApCfg.MBSSID[apidx].radius_srv_info[srv_idx].radius_key_len);
				}
			}
		}
		
		RadiusConf.RadiusInfo[apidx].ieee8021xWEP = (pAd->ApCfg.MBSSID[apidx].IEEE8021X) ? 1 : 0;
		
		if (RadiusConf.RadiusInfo[apidx].ieee8021xWEP)
		{
			// Default Key index, length and material
			keyidx = pAd->ApCfg.MBSSID[apidx].DefaultKeyId;
			RadiusConf.RadiusInfo[apidx].key_index = keyidx;

			// Determine if the key is valid. 
			KeyLen = pAd->SharedKey[apidx][keyidx].KeyLen;
			if (KeyLen == 5 || KeyLen == 13)
			{
				RadiusConf.RadiusInfo[apidx].key_length = KeyLen;
				NdisMoveMemory(RadiusConf.RadiusInfo[apidx].key_material, pAd->SharedKey[apidx][keyidx].Key, KeyLen);
			}
		}
	}
				
	wrq->u.data.length = sizeof(RADIUS_CONF);
	if (copy_to_user(wrq->u.data.pointer, &RadiusConf, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}
}


/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlRadiusData(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;

	DBGPRINT(RT_DEBUG_INFO, ("RTMPIoctlRadiusData, IF(ra%d)\n", pObj->ioctl_if));
	
	if ((pAd->ApCfg.MBSSID[pObj->ioctl_if].AuthMode == Ndis802_11AuthModeWPA) 
    	|| (pAd->ApCfg.MBSSID[pObj->ioctl_if].AuthMode == Ndis802_11AuthModeWPA2)
    	|| (pAd->ApCfg.MBSSID[pObj->ioctl_if].AuthMode == Ndis802_11AuthModeWPA1WPA2) 
    	|| (pAd->ApCfg.MBSSID[pObj->ioctl_if].IEEE8021X == TRUE))
    	WpaSend(pAd, wrq->u.data.pointer, wrq->u.data.length);
}

/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlAddWPAKey(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq)
{
	NDIS_AP_802_11_KEY 	*pKey;
	ULONG				KeyIdx;
	MAC_TABLE_ENTRY  	*pEntry;
	UCHAR				apidx;

	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	apidx =	(UCHAR) pObj->ioctl_if;
		
    DBGPRINT(RT_DEBUG_INFO, ("RTMPIoctlAddWPAKey-IF(ra%d)\n", apidx));

	pKey = (PNDIS_AP_802_11_KEY) wrq->u.data.pointer;

	if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA)
	{
		if(pKey->KeyLength == 32)
		{
			NdisMoveMemory(pAd->ApCfg.MBSSID[apidx].PMK, pKey->KeyMaterial, 32);
            DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlAddWPAKey-IF(ra%d) : Add PMK=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x....\n", apidx,
            	pAd->ApCfg.MBSSID[apidx].PMK[0],pAd->ApCfg.MBSSID[apidx].PMK[1],pAd->ApCfg.MBSSID[apidx].PMK[2],pAd->ApCfg.MBSSID[apidx].PMK[3],
            	pAd->ApCfg.MBSSID[apidx].PMK[4],pAd->ApCfg.MBSSID[apidx].PMK[5],pAd->ApCfg.MBSSID[apidx].PMK[6],pAd->ApCfg.MBSSID[apidx].PMK[7]));
		}
	}
	else	// Old WEP stuff
	{
		UCHAR	CipherAlg;
    	PUCHAR	Key;

		if(pKey->KeyLength == 32)
			return;
		
		KeyIdx = pKey->KeyIndex & 0x0fffffff;

		if (KeyIdx < 4)
		{
			// it is a shared key
			if (pKey->KeyIndex & 0x80000000)
			{
				DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlAddWPAKey-IF(ra%d) : Set Group Key\n", apidx));

				// Default key for tx (shared key)
				pAd->ApCfg.MBSSID[apidx].DefaultKeyId = (UCHAR) KeyIdx;								
                     
				// set key material and key length
				pAd->SharedKey[apidx][KeyIdx].KeyLen = (UCHAR) pKey->KeyLength;
				NdisMoveMemory(pAd->SharedKey[apidx][KeyIdx].Key, &pKey->KeyMaterial, pKey->KeyLength);
				
				// Set Ciper type
				if (pKey->KeyLength == 5)
					pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_WEP64;
				else
					pAd->SharedKey[apidx][KeyIdx].CipherAlg = CIPHER_WEP128;
			
    			CipherAlg = pAd->SharedKey[apidx][KeyIdx].CipherAlg;
    			Key = pAd->SharedKey[apidx][KeyIdx].Key;

				// Set Group key material to Asic
    			AsicAddSharedKeyEntry(pAd, apidx, KeyIdx, CipherAlg, Key, NULL, NULL);
		
				// Update WCID attribute table and IVEIV table for this group key table  
				RTMPAddWcidAttributeEntry(pAd, apidx, KeyIdx, CipherAlg, NULL);
												
			}
			else	// For Pairwise key setting
			{
				pEntry = MacTableLookup(pAd, pKey->addr);

				if (pEntry)
				{
					DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlAddWPAKey-IF(ra%d) : Set Pair-wise Key\n", apidx));
		
					// set key material and key length
 					pEntry->PairwiseKey.KeyLen = (UCHAR)pKey->KeyLength;
					NdisMoveMemory(pEntry->PairwiseKey.Key, &pKey->KeyMaterial, pKey->KeyLength);
					
					// set Cipher type
					if (pKey->KeyLength == 5)
						pEntry->PairwiseKey.CipherAlg = CIPHER_WEP64;
					else
						pEntry->PairwiseKey.CipherAlg = CIPHER_WEP128;
						
					// Add Pair-wise key to Asic
					AsicAddPairwiseKeyEntry(
						pAd, 
						pEntry->Addr, 
						(UCHAR)pEntry->Aid,
                		&pEntry->PairwiseKey);

					// update WCID attribute table and IVEIV table for this entry
					RTMPAddWcidAttributeEntry(
						pAd, 
						apidx, 
						KeyIdx, // The value may be not zero
						pEntry->PairwiseKey.CipherAlg, 
						pEntry);

				}	
			}
		}
	}
}


/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlAddPMKIDCache(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq)
{
	UCHAR				apidx;
	NDIS_AP_802_11_KEY 	*pKey;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	apidx =	(UCHAR) pObj->ioctl_if;

	pKey = (PNDIS_AP_802_11_KEY) wrq->u.data.pointer;
    
    if (pAd->ApCfg.MBSSID[apidx].AuthMode >= Ndis802_11AuthModeWPA2)
	{
		if(pKey->KeyLength == 32)
		{
			UCHAR	digest[80], PMK_key[20], macaddr[MAC_ADDR_LEN];
			
			// Calculate PMKID
			NdisMoveMemory(&PMK_key[0], "PMK Name", 8);
			NdisMoveMemory(&PMK_key[8], pAd->ApCfg.MBSSID[apidx].Bssid, MAC_ADDR_LEN);
			NdisMoveMemory(&PMK_key[14], pKey->addr, MAC_ADDR_LEN);
			HMAC_SHA1(PMK_key, 20, pKey->KeyMaterial, PMK_LEN, digest);

			NdisMoveMemory(macaddr, pKey->addr, MAC_ADDR_LEN);
			RTMPAddPMKIDCache(pAd, apidx, macaddr, digest, pKey->KeyMaterial);
			
			DBGPRINT(RT_DEBUG_TRACE, ("WPA2(pre-auth):(%02x:%02x:%02x:%02x:%02x:%02x)Calc PMKID=%02x:%02x:%02x:%02x:%02x:%02x\n", 
				pKey->addr[0],pKey->addr[1],pKey->addr[2],pKey->addr[3],pKey->addr[4],pKey->addr[5],digest[0],digest[1],digest[2],digest[3],digest[4],digest[5]));
			DBGPRINT(RT_DEBUG_TRACE, ("PMK =%02x:%02x:%02x:%02x-%02x:%02x:%02x:%02x\n",pKey->KeyMaterial[0],pKey->KeyMaterial[1],
				pKey->KeyMaterial[2],pKey->KeyMaterial[3],pKey->KeyMaterial[4],pKey->KeyMaterial[5],pKey->KeyMaterial[6],pKey->KeyMaterial[7]));
		}
		else
            DBGPRINT(RT_DEBUG_ERROR, ("Set::RT_OID_802_11_WPA2_ADD_PMKID_CACHE ERROR or is wep key \n"));
	}
    
    DBGPRINT(RT_DEBUG_TRACE, ("<== RTMPIoctlAddPMKIDCache\n"));
}


/* 
    ==========================================================================
    Description:
        UI should not call this function, it only used by 802.1x daemon
	Arguments:
	    pAd		Pointer to our adapter
	    wrq		Pointer to the ioctl argument
    ==========================================================================
*/
VOID RTMPIoctlStaticWepCopy(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq)
{
	MAC_TABLE_ENTRY  *pEntry;
	UCHAR	MacAddr[MAC_ADDR_LEN];
	UCHAR			apidx;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	
	apidx =	(UCHAR) pObj->ioctl_if;
	
    DBGPRINT(RT_DEBUG_TRACE, ("RTMPIoctlStaticWepCopy-IF(ra%d)\n", apidx));

    if (wrq->u.data.length != sizeof(MacAddr))
    {
        DBGPRINT(RT_DEBUG_ERROR, ("RTMPIoctlStaticWepCopy: the length isn't match (%d)\n", wrq->u.data.length));
        return;
    }
    else
    {
    	UINT32 len;
		
        len = copy_from_user(&MacAddr, wrq->u.data.pointer, wrq->u.data.length);    
        pEntry = MacTableLookup(pAd, MacAddr);
        if (!pEntry)
        {
            DBGPRINT(RT_DEBUG_ERROR, ("RTMPIoctlStaticWepCopy: the mac address isn't match\n"));
            return;
        }
        else
        {
            UCHAR	KeyIdx;
            
            KeyIdx = pAd->ApCfg.MBSSID[apidx].DefaultKeyId;
            
            //need to copy the default shared-key to pairwise key table for this entry in 802.1x mode
			if (pAd->SharedKey[apidx][KeyIdx].KeyLen == 0)
			{
				DBGPRINT(RT_DEBUG_ERROR, ("ERROR: Can not get Default shared-key (index-%d)\n", KeyIdx));
				return;
			}
			else
        	{
            	pEntry->PairwiseKey.KeyLen = pAd->SharedKey[apidx][KeyIdx].KeyLen;
            	NdisMoveMemory(pEntry->PairwiseKey.Key, pAd->SharedKey[apidx][KeyIdx].Key, pEntry->PairwiseKey.KeyLen);
            	pEntry->PairwiseKey.CipherAlg = pAd->SharedKey[apidx][KeyIdx].CipherAlg;

				// Add Pair-wise key to Asic
            	AsicAddPairwiseKeyEntry(
                		pAd, 
                		pEntry->Addr, 
                		(UCHAR)pEntry->Aid,
                		&pEntry->PairwiseKey);

				// update WCID attribute table and IVEIV table for this entry
				RTMPAddWcidAttributeEntry(
						pAd, 
						apidx, 
						KeyIdx, // The value may be not zero
                		pEntry->PairwiseKey.CipherAlg, 
						pEntry);	
        	}
			
        }
	}
    return;
}

#ifdef DBG
/* 
    ==========================================================================
    Description:
        Read / Write BBP
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 bbp               ==> read all BBP
               2.) iwpriv ra0 bbp 1             ==> read BBP where RegID=1
               3.) iwpriv ra0 bbp 1=10		    ==> write BBP R1=0x10
    ==========================================================================
*/
VOID RTMPAPIoctlBBP(
	IN	PRTMP_ADAPTER	pAdapter,
	IN	struct iwreq	*wrq)
{
	CHAR				*this_char;
	CHAR				*value;
	UCHAR				regBBP = 0;
	CHAR				*mpool, *msg; //msg[2048];
	CHAR				*arg; //arg[255];
	CHAR				*ptr;
	INT					bbpId;
	LONG				bbpValue;
	BOOLEAN				bIsPrintAllBBP = FALSE;

	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlBBP\n"));

	mpool = (CHAR *) kmalloc(sizeof(CHAR)*(2048+256+12), MEM_ALLOC_FLAG);

	if (mpool == NULL) {
		return;
	}

	msg = (CHAR *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (CHAR *)((ULONG)(msg+2048+3) & (ULONG)~0x03);

	memset(msg, 0x00, 2048);
	if (wrq->u.data.length > 1) //No parameters.
	{
		NdisMoveMemory(arg, wrq->u.data.pointer, (wrq->u.data.length > 255) ? 255 : wrq->u.data.length);
		ptr = arg;
		sprintf(msg, "\n");
		//Parsing Read or Write
		while ((this_char = strsep((char **)&ptr, ",")) != NULL)
		{
			DBGPRINT(RT_DEBUG_INFO, ("this_char=%s\n", this_char));
			if (!*this_char)
				continue;

			if ((value = strchr(this_char, '=')) != NULL)
				*value++ = 0;

			if (!value || !*value)
			{ //Read
				DBGPRINT(RT_DEBUG_INFO, ("this_char=%s, value=%s\n", this_char, value));
				if (sscanf(this_char, "%d", &(bbpId)) == 1)
				{
					if (bbpId <= 136)
					{
						// In RT2860 ATE mode, we do not load 8051 firmware.
                                                //We must access BBP directly.
                        // For RT2870 ATE mode, ATE_BBP_IO_WRITE8(/READ8)_BY_REG_ID are redefined.
#ifdef RALINK_ATE
						if (pAdapter->ate.Mode != ATE_STOP)
							{
								ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
							}
							else
#endif // RALINK_ATE //
						// according to Andy, Gary, David require.
						// the command bbp shall read BBP register directly for dubug.
						BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
						sprintf(msg+strlen(msg), "R%02d[0x%02x]:%02X  ", bbpId, bbpId*2, regBBP);
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
					}
					else
					{//Invalid parametes, so default printk all bbp
						bIsPrintAllBBP = TRUE;
						break;
					}
				}
				else
				{ //Invalid parametes, so default printk all bbp
					bIsPrintAllBBP = TRUE;
					break;
				}
			}
			else
			{ //Write
				DBGPRINT(RT_DEBUG_INFO, ("this_char=%s, value=%s\n", this_char, value));
				if ((sscanf(this_char, "%d", &(bbpId)) == 1) && (sscanf(value, "%lx", &(bbpValue)) == 1))
				{
					DBGPRINT(RT_DEBUG_INFO, ("bbpID=%02d, value=0x%lx\n", bbpId, bbpValue));
					if (bbpId <= 136)
					{
						// In RT2860 ATE mode, we do not load 8051 firmware.
						// We should access BBP registers directly.
                        // For RT2870 ATE mode, ATE_BBP_IO_WRITE8/READ8_BY_REG_ID are redefined.
#ifdef RALINK_ATE
							if (pAdapter->ate.Mode != ATE_STOP)
							{
								ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
								ATE_BBP_IO_WRITE8_BY_REG_ID(pAdapter, (UCHAR)bbpId,(UCHAR) bbpValue);
								//Read it back for showing
								ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
								sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X\n", bbpId, bbpId*2, regBBP);
								DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
							}
							else
#endif // RALINK_ATE //
							{
								// according to Andy, Gary, David require.
								// the command bbp shall read/write BBP register directly for dubug.
								BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
						                BBP_IO_WRITE8_BY_REG_ID(pAdapter, (UCHAR)bbpId,(UCHAR) bbpValue);
						                //Read it back for showing
								BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
						                sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X\n", bbpId, bbpId*2, regBBP);
						                DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
					                }
					}
					else
					{//Invalid parametes, so default printk all bbp
						bIsPrintAllBBP = TRUE;
						break;
					}
				}
				else
				{ //Invalid parametes, so default printk all bbp
					bIsPrintAllBBP = TRUE;
					break;
				}
			}
		}
	}
	else
		bIsPrintAllBBP = TRUE;

	if (bIsPrintAllBBP)
	{
		memset(msg, 0x00, 2048);
		sprintf(msg, "\n");
		for (bbpId = 0; bbpId <= 136; bbpId++)
		{
			// In RT2860 ATE mode, we do not load 8051 firmware.
            // We should access BBP registers directly.
            // For RT2870 ATE mode, ATE_BBP_IO_WRITE8/READ8_BY_REG_ID are redefined.
#ifdef RALINK_ATE
				if (pAdapter->ate.Mode != ATE_STOP)
				{
					ATE_BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
				}
				else
#endif // RALINK_ATE //

			// according to Andy, Gary, David require.
			// the command bbp shall read/write BBP register directly for dubug.
			BBP_IO_READ8_BY_REG_ID(pAdapter, bbpId, &regBBP);
			sprintf(msg+strlen(msg), "R%02d[0x%02X]:%02X    ", bbpId, bbpId*2, regBBP);
			if (bbpId%5 == 4)
				sprintf(msg+strlen(msg), "\n");
		}
		// Copy the information into the user buffer
		DBGPRINT(RT_DEBUG_TRACE, ("strlen(msg)=%d\n", (UINT32)strlen(msg)));
		wrq->u.data.length = strlen(msg);
		if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length)) 
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));			
		}
	}
	else
	{
		DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
		// Copy the information into the user buffer
		DBGPRINT(RT_DEBUG_INFO, ("strlen(msg) =%d\n", (UINT32)strlen(msg)));
		wrq->u.data.length = strlen(msg);
		if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));			
		}

	}

	kfree(mpool);
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlBBP\n\n"));
}

/* 
    ==========================================================================
    Description:
        Read / Write MAC
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 mac 0        ==> read MAC where Addr=0x0
               2.) iwpriv ra0 mac 0=12     ==> write MAC where Addr=0x0, value=12
    ==========================================================================
*/
VOID RTMPAPIoctlMAC(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq)
{
	CHAR				*this_char;
	CHAR				*value;
	INT					j = 0, k = 0;
	CHAR				*mpool, *msg; //msg[1024];
	CHAR				*arg; //arg[255];
	CHAR				*ptr;
	UINT32				macAddr = 0;
	UCHAR				temp[16], temp2[16];
	UINT32				macValue;

	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlMAC\n"));

	mpool = (CHAR *) kmalloc(sizeof(CHAR)*(2048+256+12), MEM_ALLOC_FLAG);

	if (mpool == NULL) {
		return;
	}

	msg = (CHAR *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (CHAR *)((ULONG)(msg+1024+3) & (ULONG)~0x03);

	memset(msg, 0x00, 1024);
	if (wrq->u.data.length > 1) //No parameters.
	{
		NdisMoveMemory(arg, wrq->u.data.pointer, (wrq->u.data.length > 255) ? 255 : wrq->u.data.length);
		ptr = arg;
		sprintf(msg, "\n");
		//Parsing Read or Write
		while ((this_char = strsep((char **)&ptr, ",")) != NULL)
		{
			DBGPRINT(RT_DEBUG_INFO, ("this_char=%s\n", this_char));
			if (!*this_char)
				continue;

			if ((value = strchr(this_char, '=')) != NULL)
				*value++ = 0;

			if (!value || !*value)
			{ //Read
				DBGPRINT(RT_DEBUG_INFO, ("Read: this_char=%s, strlen=%d\n", this_char, (UINT32)strlen(this_char)));

				// Sanity check
				if(strlen(this_char) > 4)
					break;

				j = strlen(this_char);
				while(j-- > 0)
				{
					if(this_char[j] > 'f' || this_char[j] < '0')
						goto done; //return;
				}

				// Mac Addr
				k = j = strlen(this_char);
				while(j-- > 0)
				{
					this_char[4-k+j] = this_char[j];
				}
				
				while(k < 4)
					this_char[3-k++]='0';
				this_char[4]='\0';

				if(strlen(this_char) == 4)
				{
					AtoH(this_char, temp, 4);
					macAddr = *temp*256 + temp[1];					
					if (macAddr < 0xFFFF)
					{
						RTMP_IO_READ32(pAdapter, macAddr, &macValue);
						DBGPRINT(RT_DEBUG_TRACE, ("MacAddr=0x%x, MacValue=0x%x\n", macAddr, macValue));
						sprintf(msg+strlen(msg), "[0x%08x]:%08x  ", macAddr , macValue);
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
					}
					else
					{//Invalid parametes, so default printk all bbp
						break;
					}
				}
			}
			else
			{ //Write
				DBGPRINT(RT_DEBUG_INFO, ("Write: this_char=%s, strlen(value)=%d, value=%s\n", this_char, (UINT32)strlen(value), value));
				NdisMoveMemory(&temp2, value, strlen(value));
				temp2[strlen(value)] = '\0';

				// Sanity check
				if((strlen(this_char) > 4) || strlen(temp2) > 8)
					break;

				j = strlen(this_char);
				while(j-- > 0)
				{
					if(this_char[j] > 'f' || this_char[j] < '0')
						goto done; //return;
				}

				j = strlen(temp2);
				while(j-- > 0)
				{
					if(temp2[j] > 'f' || temp2[j] < '0')
						goto done; //return;
				}

				//MAC Addr
				k = j = strlen(this_char);
				while(j-- > 0)
				{
					this_char[4-k+j] = this_char[j];
				}

				while(k < 4)
					this_char[3-k++]='0';
				this_char[4]='\0';

				//MAC value
				k = j = strlen(temp2);
				while(j-- > 0)
				{
					temp2[8-k+j] = temp2[j];
				}
				
				while(k < 8)
					temp2[7-k++]='0';
				temp2[8]='\0';

				{
					AtoH(this_char, temp, 4);
					macAddr = *temp*256 + temp[1];

					AtoH(temp2, temp, 8);
					macValue = *temp*256*256*256 + temp[1]*256*256 + temp[2]*256 + temp[3];

					// debug mode
					if (macAddr == (HW_DEBUG_SETTING_BASE + 4))
					{
						// 0x2bf4: byte0 non-zero: enable R66 tuning, 0: disable R66 tuning
                        if (macValue & 0x000000ff) 
                        {
                            pAdapter->BbpTuning.bEnable = TRUE;
                            DBGPRINT(RT_DEBUG_TRACE, ("turn on R17 tuning\n"));
                        }
                        else
                        {
                            UCHAR R66;
                            pAdapter->BbpTuning.bEnable = FALSE;
                            R66 = 0x26 + GET_LNA_GAIN(pAdapter);
                            // todo RTMP_BBP_IO_WRITE8_BY_REG_ID(pAd, BBP_R66, (0x26 + GET_LNA_GAIN(pAd)));
                            DBGPRINT(RT_DEBUG_TRACE, ("turn off R66 tuning, restore to 0x%02x\n", R66));
                        }
						return;
					}

					DBGPRINT(RT_DEBUG_TRACE, ("MacAddr=%02x, MacValue=0x%x\n", macAddr, macValue));
					
					RTMP_IO_WRITE32(pAdapter, macAddr, macValue);
					sprintf(msg+strlen(msg), "[0x%08x]:%08x  ", macAddr, macValue);
					DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
				}
			}
		}
	}

	if(strlen(msg) == 1)
		sprintf(msg+strlen(msg), "===>Error command format!");
	DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
	// Copy the information into the user buffer
	DBGPRINT(RT_DEBUG_INFO, ("strlen(msg) =%d\n", (UINT32)strlen(msg)));
	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));			
	}

	
done:
	kfree(mpool);
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlMAC\n\n"));
}

/* 
    ==========================================================================
    Description:
        Read / Write E2PROM
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 e2p 0     	==> read E2PROM where Addr=0x0
               2.) iwpriv ra0 e2p 0=1234    ==> write E2PROM where Addr=0x0, value=1234
    ==========================================================================
*/
VOID RTMPAPIoctlE2PROM(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	struct iwreq	*wrq)
{
	CHAR				*this_char;
	CHAR				*value;
	INT					j = 0, k = 0;
	CHAR				*mpool, *msg;//msg[1024];
	CHAR				*arg; //arg[255];
	CHAR				*ptr;
	USHORT				eepAddr = 0;
	UCHAR				temp[16], temp2[16];
	USHORT				eepValue;

	DBGPRINT(RT_DEBUG_INFO, ("==>RTMPIoctlE2PROM\n"));

	mpool = (CHAR *) kmalloc(sizeof(CHAR)*(2048+256+12), MEM_ALLOC_FLAG);

	if (mpool == NULL) {
		return;
	}

	msg = (CHAR *)((ULONG)(mpool+3) & (ULONG)~0x03);
	arg = (CHAR *)((ULONG)(msg+1024+3) & (ULONG)~0x03);


	memset(msg, 0x00, 1024);
	if (wrq->u.data.length > 1) //No parameters.
	{
		NdisMoveMemory(arg, wrq->u.data.pointer, (wrq->u.data.length > 255) ? 255 : wrq->u.data.length);
		ptr = arg;
		sprintf(msg, "\n");
		//Parsing Read or Write
		while ((this_char = strsep((char **)&ptr, ",")) != NULL)
		{
			DBGPRINT(RT_DEBUG_INFO, ("this_char=%s\n", this_char));
			if (!*this_char)
				continue;

			if ((value = strchr(this_char, '=')) != NULL)
				*value++ = 0;

			if (!value || !*value)
			{ //Read
				DBGPRINT(RT_DEBUG_INFO, ("Read: this_char=%s, strlen=%d\n", this_char, (UINT32)strlen(this_char)));

				// Sanity check
				if(strlen(this_char) > 4)
					break;

				j = strlen(this_char);
				while(j-- > 0)
				{
					if(this_char[j] > 'f' || this_char[j] < '0')
						goto done; //return;
				}

				// E2PROM addr
				k = j = strlen(this_char);
				while(j-- > 0)
				{
					this_char[4-k+j] = this_char[j];
				}
				
				while(k < 4)
					this_char[3-k++]='0';
				this_char[4]='\0';

				if(strlen(this_char) == 4)
				{
					AtoH(this_char, temp, 4);
					eepAddr = *temp*256 + temp[1];					
					if (eepAddr < 0xFFFF)
					{
						RT28xx_EEPROM_READ16(pAdapter, eepAddr, eepValue);
						DBGPRINT(RT_DEBUG_INFO, ("eepAddr=%x, eepValue=0x%x\n", eepAddr, eepValue));
						sprintf(msg+strlen(msg), "[0x%04X]:0x%04X  ", eepAddr , eepValue);
						DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
					}
					else
					{//Invalid parametes, so default printk all bbp
						break;
					}
				}
			}
			else
			{ //Write
				DBGPRINT(RT_DEBUG_INFO, ("Write: this_char=%s, strlen(value)=%d, value=%s\n", this_char, (UINT32)strlen(value), value));
				NdisMoveMemory(&temp2, value, strlen(value));
				temp2[strlen(value)] = '\0';

				// Sanity check
				if((strlen(this_char) > 4) || strlen(temp2) > 8)
					break;

				j = strlen(this_char);
				while(j-- > 0)
				{
					if(this_char[j] > 'f' || this_char[j] < '0')
						goto done; //return;
				}
				j = strlen(temp2);
				while(j-- > 0)
				{
					if(temp2[j] > 'f' || temp2[j] < '0')
						goto done; //return;
				}

				//MAC Addr
				k = j = strlen(this_char);
				while(j-- > 0)
				{
					this_char[4-k+j] = this_char[j];
				}

				while(k < 4)
					this_char[3-k++]='0';
				this_char[4]='\0';

				//MAC value
				k = j = strlen(temp2);
				while(j-- > 0)
				{
					temp2[4-k+j] = temp2[j];
				}
				
				while(k < 4)
					temp2[3-k++]='0';
				temp2[4]='\0';

				AtoH(this_char, temp, 4);
				eepAddr = *temp*256 + temp[1];

				AtoH(temp2, temp, 4);
				eepValue = *temp*256 + temp[1];

				DBGPRINT(RT_DEBUG_INFO, ("eepAddr=%02x, eepValue=0x%x\n", eepAddr, eepValue));
				
				RT28xx_EEPROM_WRITE16(pAdapter, eepAddr, eepValue);
				sprintf(msg+strlen(msg), "[0x%02X]:%02X  ", eepAddr, eepValue);
				DBGPRINT(RT_DEBUG_INFO, ("msg=%s\n", msg));
			}
		}
	}

	if(strlen(msg) == 1)
		sprintf(msg+strlen(msg), "===>Error command format!");

	// Copy the information into the user buffer
	DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
	wrq->u.data.length = strlen(msg);
	if (copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));			
	}

done:	
	kfree(mpool);
	DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlE2PROM\n"));
}
#endif //#ifdef DBG

/* 
    ==========================================================================
    Description:
        Read statistics counter
Arguments:
    pAdapter                    Pointer to our adapter
    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage: 
               1.) iwpriv ra0 stat 0     	==> Read statistics counter
    ==========================================================================
*/
static VOID RTMPIoctlStatistics(
	IN PRTMP_ADAPTER pAd, 
	IN struct iwreq *wrq)
{
	INT					Status;
	char                *msg;
#ifdef WSC_AP_SUPPORT
#ifdef APCLI_SUPPORT
    UCHAR               idx = 0;
#endif // APCLI_SUPPORT //
#endif // WSC_AP_SUPPORT //

	msg = (CHAR *) kmalloc(sizeof(CHAR)*(2048), MEM_ALLOC_FLAG);
	if (msg == NULL) {
		return;
	}


    memset(msg, 0x00, 1600);
    sprintf(msg, "\n");

#ifdef RALINK_ATE//<========================PETER
	if(pAd->ate.Mode != ATE_STOP)
	{
	    sprintf(msg+strlen(msg), "Tx success                      = %ld\n", (ULONG)pAd->ate.TxDoneCount);
	    //sprintf(msg+strlen(msg), "Tx success without retry        = %ld\n", (ULONG)pAd->ate.TxDoneCount);
	}
	else
#endif // RALINK_ATE //
	{
    sprintf(msg+strlen(msg), "Tx success                      = %ld\n", (ULONG)pAd->WlanCounters.TransmittedFragmentCount.u.LowPart);
    //sprintf(msg+strlen(msg), "Tx success without retry        = %ld\n", (ULONG)pAd->WlanCounters.TransmittedFragmentCount.u.LowPart - (ULONG)pAd->WlanCounters.RetryCount.u.LowPart);
	}
    sprintf(msg+strlen(msg), "Tx retry count          			= %ld\n", (ULONG)pAd->WlanCounters.RetryCount.u.LowPart);
    sprintf(msg+strlen(msg), "Tx fail to Rcv ACK after retry  = %ld\n", (ULONG)pAd->WlanCounters.FailedCount.u.LowPart);
    sprintf(msg+strlen(msg), "RTS Success Rcv CTS             = %ld\n", (ULONG)pAd->WlanCounters.RTSSuccessCount.u.LowPart);
    sprintf(msg+strlen(msg), "RTS Fail Rcv CTS                = %ld\n", (ULONG)pAd->WlanCounters.RTSFailureCount.u.LowPart);

    sprintf(msg+strlen(msg), "Rx success                      = %ld\n", (ULONG)pAd->WlanCounters.ReceivedFragmentCount.QuadPart);
    sprintf(msg+strlen(msg), "Rx with CRC                     = %ld\n", (ULONG)pAd->WlanCounters.FCSErrorCount.u.LowPart);
    sprintf(msg+strlen(msg), "Rx drop due to out of resource  = %ld\n", (ULONG)pAd->Counters8023.RxNoBuffer);
    sprintf(msg+strlen(msg), "Rx duplicate frame              = %ld\n", (ULONG)pAd->WlanCounters.FrameDuplicateCount.u.LowPart);

    sprintf(msg+strlen(msg), "False CCA (one second)          = %ld\n", (ULONG)pAd->RalinkCounters.OneSecFalseCCACnt);
#ifdef RALINK_ATE
	if(pAd->ate.Mode != ATE_STOP)
	{
		if (pAd->ate.RxAntennaSel == 0)
		{
    		sprintf(msg+strlen(msg), "RSSI-A                          = %ld\n", (LONG)(pAd->ate.LastRssi0 - pAd->BbpRssiToDbmDelta));
			sprintf(msg+strlen(msg), "RSSI-B (if available)           = %ld\n", (LONG)(pAd->ate.LastRssi1 - pAd->BbpRssiToDbmDelta));
			sprintf(msg+strlen(msg), "RSSI-C (if available)           = %ld\n\n", (LONG)(pAd->ate.LastRssi2 - pAd->BbpRssiToDbmDelta));
		}
		else
		{
    		sprintf(msg+strlen(msg), "RSSI                            = %ld\n", (LONG)(pAd->ate.LastRssi0 - pAd->BbpRssiToDbmDelta));
		}
	}
	else
#endif // RALINK_ATE //
	{
    	sprintf(msg+strlen(msg), "RSSI-A                          = %ld\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi0 - pAd->BbpRssiToDbmDelta));
		sprintf(msg+strlen(msg), "RSSI-B (if available)           = %ld\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi1 - pAd->BbpRssiToDbmDelta));
		sprintf(msg+strlen(msg), "RSSI-C (if available)           = %ld\n\n", (LONG)(pAd->ApCfg.RssiSample.LastRssi2 - pAd->BbpRssiToDbmDelta));
	}

#ifdef WSC_AP_SUPPORT
	sprintf(msg+strlen(msg), "WPS Information:\n");
	// display pin code
	sprintf(msg+strlen(msg), "Enrollee PinCode(ra0)           %08u\n", pAd->ApCfg.MBSSID[0].WscControl.WscEnrolleePinCode);
#ifdef APCLI_SUPPORT
    sprintf(msg+strlen(msg), "\n");
    sprintf(msg+strlen(msg), "Enrollee PinCode(ApCli0)        %08u\n", pAd->ApCfg.ApCliTab[0].WscControl.WscEnrolleePinCode);
    sprintf(msg+strlen(msg), "Ap Client WPS Profile Count     = %d\n", pAd->ApCfg.ApCliTab[0].WscControl.WscProfile.ProfileCnt);
    for (idx = 0; idx < pAd->ApCfg.ApCliTab[0].WscControl.WscProfile.ProfileCnt ; idx++)
    {
        PWSC_CREDENTIAL pCredential = &pAd->ApCfg.ApCliTab[0].WscControl.WscProfile.Profile[idx];
        sprintf(msg+strlen(msg), "Profile[%d]:\n", idx);        
        sprintf(msg+strlen(msg), "SSID                            = %s\n", pCredential->SSID.Ssid);
        sprintf(msg+strlen(msg), "AuthType                        = %s\n", WscGetAuthTypeStr(pCredential->AuthType));
        sprintf(msg+strlen(msg), "EncrypType                      = %s\n", WscGetEncryTypeStr(pCredential->EncrType)); 
        sprintf(msg+strlen(msg), "KeyIndex                        = %d\n", pCredential->KeyIndex);
        if (pCredential->KeyLength != 0)
        {
            sprintf(msg+strlen(msg), "Key                             = %s\n", pCredential->Key);
        }
    }
    sprintf(msg+strlen(msg), "\n");
#endif // APCLI_SUPPORT //
#endif // WSC_AP_SUPPORT //
    
    // Copy the information into the user buffer
    DBGPRINT(RT_DEBUG_INFO, ("copy to user [msg=%s]\n", msg));
    wrq->u.data.length = strlen(msg);
    Status = copy_to_user(wrq->u.data.pointer, msg, wrq->u.data.length);

	kfree(msg);
    DBGPRINT(RT_DEBUG_TRACE, ("<==RTMPIoctlStatistics\n"));
}

/* 
    ==========================================================================
    Description:
        Get Block ACK Table
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage:
        		1.) iwpriv ra0 get_ba_table
        		3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
VOID RTMPIoctlQueryBaTable(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq)
{
	QUERYBA_TABLE		*BAT;
	//char *msg;
	UCHAR	TotalEntry, i, j, index;

	BAT = vmalloc(sizeof(QUERYBA_TABLE));

	RTMPZeroMemory(BAT, sizeof(QUERYBA_TABLE));

	TotalEntry = pAd->MacTab.Size;
	index = 0;
	for (i=0; ((i < MAX_LEN_OF_MAC_TABLE) && (TotalEntry > 0)); i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

		if ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst == SST_ASSOC) && (pEntry->TXBAbitmap))
		{
			NdisMoveMemory(BAT->BAOriEntry[index].MACAddr, pEntry->Addr, 6);
			for (j=0;j<8;j++)
			{
				if (pEntry->BAOriWcidArray[j] != 0)
					BAT->BAOriEntry[index].BufSize[j] = pAd->BATable.BAOriEntry[pEntry->BAOriWcidArray[j]].BAWinSize;
				else
					BAT->BAOriEntry[index].BufSize[j] = 0;
			}

			TotalEntry--;
			index++;
			BAT->OriNum++;
		}
	}

	TotalEntry = pAd->MacTab.Size;
	index = 0;
	for (i=0; ((i < MAX_LEN_OF_MAC_TABLE) && (TotalEntry > 0)); i++)
	{
		PMAC_TABLE_ENTRY pEntry = &pAd->MacTab.Content[i];

		if ((pEntry->ValidAsCLI == TRUE) && (pEntry->Sst == SST_ASSOC) && (pEntry->RXBAbitmap))
		{
			NdisMoveMemory(BAT->BARecEntry[index].MACAddr, pEntry->Addr, 6);
			BAT->BARecEntry[index].BaBitmap = (UCHAR)pEntry->RXBAbitmap;
			for (j = 0; j < 8; j++)
			{
				if (pEntry->BARecWcidArray[j] != 0)
					BAT->BARecEntry[index].BufSize[j] = pAd->BATable.BARecEntry[pEntry->BARecWcidArray[j]].BAWinSize;
				else
					BAT->BARecEntry[index].BufSize[j] = 0;
			}

			TotalEntry--;
			index++;
			BAT->RecNum++;
		}
	}

	wrq->u.data.length = sizeof(QUERYBA_TABLE);

	if (copy_to_user(wrq->u.data.pointer, BAT, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	vfree(BAT);

#if 0
	msg = (CHAR *) kmalloc(sizeof(CHAR)*(2048), MEM_ALLOC_FLAG);
	if (msg == NULL) {
		return;
	}

	memset(msg, 0x00, 2048);
	sprintf(msg,"%s","\n");

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s\n", "Configured", "SSID", "AuthMode", "EncrypType");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-13s%-26s\n", "Configured", "SSID", "AuthMode", "EncrypType", "DefaultKeyID", "Key");
	}
	else
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-64s\n", "Configured", "SSID", "AuthMode", "EncrypType", "Key");
	}

	if (Profile.WscConfigured == 1)
		sprintf(msg+strlen(msg),"%-12s", "No");
	else
		sprintf(msg+strlen(msg),"%-12s", "Yes");
	sprintf(msg+strlen(msg), "%-33s", Profile.WscSsid);
	sprintf(msg+strlen(msg), "%-12s", WscGetAuthTypeStr(Profile.WscAuthMode));
	sprintf(msg+strlen(msg), "%-12s", WscGetEncryTypeStr(Profile.WscEncrypType));

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg), "%s\n", "");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg), "%-13d",Profile.DefaultKeyIdx);
		sprintf(msg+strlen(msg), "%-26s\n",Profile.WscWPAKey);
	}
	else if (Profile.WscEncrypType >= 4)
	{
	    sprintf(msg+strlen(msg), "%-64s\n",Profile.WscWPAKey);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));

	kfree(msg);
#endif
}

#ifdef APCLI_SUPPORT
INT Set_ApCli_Enable_Proc(
	IN  PRTMP_ADAPTER pAd, 
	IN  PUCHAR arg)
{
	UINT Enable;
	POS_COOKIE pObj;
	UCHAR ifIndex;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	Enable = simple_strtol(arg, 0, 16);

	pAd->ApCfg.ApCliTab[ifIndex].Enable = (Enable > 0) ? TRUE : FALSE;

	DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_Enable_Proc::(enable = %d)\n", ifIndex, pAd->ApCfg.ApCliTab[ifIndex].Enable));
	
	ApCliIfDown(pAd);

	return TRUE;
}

INT Set_ApCli_Ssid_Proc(
	IN  PRTMP_ADAPTER pAd, 
	IN  PUCHAR arg)
{
	POS_COOKIE pObj;
	UCHAR ifIndex;
	BOOLEAN apcliEn;
	INT success = FALSE;
	UCHAR keyMaterial[40];
	UCHAR PskKey[100];

	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	if(strlen(arg) <= MAX_LEN_OF_SSID)
	{
		apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

		// bring apcli interface down first
		if(apcliEn == TRUE )
		{
			pAd->ApCfg.ApCliTab[ifIndex].Enable = FALSE;
			ApCliIfDown(pAd);
		}

		NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, MAX_LEN_OF_SSID);
		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, arg, strlen(arg));
		pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen = (UCHAR)strlen(arg);
		success = TRUE;

		// Upadte PMK and restart WPAPSK state machine for ApCli link
		if (((pAd->ApCfg.ApCliTab[ifIndex].AuthMode == Ndis802_11AuthModeWPAPSK) ||
				(pAd->ApCfg.ApCliTab[ifIndex].AuthMode == Ndis802_11AuthModeWPA2PSK)) && 
					pAd->ApCfg.ApCliTab[ifIndex].PSKLen > 0)
		{
			NdisZeroMemory(PskKey, 100);
			NdisMoveMemory(PskKey, pAd->ApCfg.ApCliTab[ifIndex].PSK, pAd->ApCfg.ApCliTab[ifIndex].PSKLen);
			
			if ((strlen(PskKey) >= 8) && (strlen(PskKey) < 64))
			{
	    		PasswordHash((char *)PskKey, pAd->ApCfg.ApCliTab[ifIndex].CfgSsid, pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen, keyMaterial);
	    		NdisMoveMemory(pAd->ApCfg.ApCliTab[ifIndex].PMK, keyMaterial, 32);
			}
			else if (strlen(PskKey) == 64)
			{
				AtoH(PskKey, pAd->ApCfg.ApCliTab[ifIndex].PMK, 32);
			}						
		}

		DBGPRINT(RT_DEBUG_TRACE, ("I/F(apcli%d) Set_ApCli_Ssid_Proc::(Len=%d,Ssid=%s)\n", ifIndex,
			pAd->ApCfg.ApCliTab[ifIndex].CfgSsidLen, pAd->ApCfg.ApCliTab[ifIndex].CfgSsid));

		pAd->ApCfg.ApCliTab[ifIndex].Enable = apcliEn;
	}
	else
		success = FALSE;

	return success;
}


INT Set_ApCli_Bssid_Proc(
	IN  PRTMP_ADAPTER pAd, 
	IN  PUCHAR arg)
{
	INT i;
	CHAR *value;
	UCHAR ifIndex;
	BOOLEAN apcliEn;
	POS_COOKIE pObj;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;
	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;
	
	apcliEn = pAd->ApCfg.ApCliTab[ifIndex].Enable;

	// bring apcli interface down first
	if(apcliEn == TRUE )
	{
		pAd->ApCfg.ApCliTab[ifIndex].Enable = FALSE;
		ApCliIfDown(pAd);
	}

	NdisZeroMemory(pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid, MAC_ADDR_LEN);

	if(strlen(arg) == 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
	{
		for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":"), i++) 
		{
			if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
				return FALSE;  //Invalid

			AtoH(value, &pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[i], 1);
		}

		if(i != 6)
			return FALSE;  //Invalid
	}

	DBGPRINT(RT_DEBUG_TRACE, ("Set_ApCli_Bssid_Proc (%2X:%2X:%2X:%2X:%2X:%2X)\n",
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[0],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[1],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[2],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[3],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[4],
		pAd->ApCfg.ApCliTab[ifIndex].CfgApCliBssid[5]));

	pAd->ApCfg.ApCliTab[ifIndex].Enable = apcliEn;

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set ApCli-IF Authentication mode
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_AuthMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG       i;
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	if ((strncmp(arg, "WEPAUTO", 7) == 0) || (strncmp(arg, "wepauto", 7) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeAutoSwitch;
	else if ((strncmp(arg, "SHARED", 6) == 0) || (strncmp(arg, "shared", 6) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeShared;
	else if ((strncmp(arg, "WPAPSK", 6) == 0) || (strncmp(arg, "wpapsk", 6) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeWPAPSK;
	else if ((strncmp(arg, "WPA2PSK", 7) == 0) || (strncmp(arg, "wpa2psk", 7) == 0))
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeWPA2PSK;
	else
		pAd->ApCfg.ApCliTab[ifIndex].AuthMode = Ndis802_11AuthModeOpen;

	for (i=0; i<MAX_LEN_OF_MAC_TABLE; i++)
	{
		if (pAd->MacTab.Content[i].ValidAsApCli)
		{
			pAd->MacTab.Content[i].PortSecured  = WPA_802_1X_PORT_NOT_SECURED;
		}
	}
		
    RTMPMakeRSNIE(pAd, pAd->ApCfg.ApCliTab[ifIndex].AuthMode, pAd->ApCfg.ApCliTab[ifIndex].WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));

	pAd->ApCfg.ApCliTab[ifIndex].DefaultKeyId  = 0;

	if(pAd->ApCfg.ApCliTab[ifIndex].AuthMode >= Ndis802_11AuthModeWPA)
		pAd->ApCfg.ApCliTab[ifIndex].DefaultKeyId = 1;

	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_AuthMode_Proc::(AuthMode=%d)\n", ifIndex, pAd->ApCfg.ApCliTab[ifIndex].AuthMode));		
	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set ApCli-IF Encryption Type
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_EncrypType_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE 	pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 		ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	pApCliEntry->WepStatus = Ndis802_11WEPDisabled; 
	if ((strncmp(arg, "WEP", 3) == 0) || (strncmp(arg, "wep", 3) == 0))
    {
		if (pApCliEntry->AuthMode < Ndis802_11AuthModeWPA)
			pApCliEntry->WepStatus = Ndis802_11WEPEnabled;				  
	}
	else if ((strncmp(arg, "TKIP", 4) == 0) || (strncmp(arg, "tkip", 4) == 0))
	{
		if (pApCliEntry->AuthMode >= Ndis802_11AuthModeWPA)
			pApCliEntry->WepStatus = Ndis802_11Encryption2Enabled;                       
    }
	else if ((strncmp(arg, "AES", 3) == 0) || (strncmp(arg, "aes", 3) == 0))
	{
		if (pApCliEntry->AuthMode >= Ndis802_11AuthModeWPA)
			pApCliEntry->WepStatus = Ndis802_11Encryption3Enabled;                            
	}    
	else
	{
		pApCliEntry->WepStatus = Ndis802_11WEPDisabled;                 
	}

	pApCliEntry->PairCipher     = pApCliEntry->WepStatus;
	pApCliEntry->GroupCipher    = pApCliEntry->WepStatus;
	pApCliEntry->bMixCipher		= FALSE;

	if (pApCliEntry->WepStatus >= Ndis802_11Encryption2Enabled)
		pApCliEntry->DefaultKeyId = 1;

    RTMPMakeRSNIE(pAd, pApCliEntry->AuthMode, pApCliEntry->WepStatus, (ifIndex + MIN_NET_DEVICE_FOR_APCLI));
	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_EncrypType_Proc::(EncrypType=%d)\n", ifIndex, pApCliEntry->WepStatus));

	return TRUE;
}



/* 
    ==========================================================================
    Description:
        Set Default Key ID
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_DefaultKeyID_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	ULONG 			KeyIdx;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR 			ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	KeyIdx = simple_strtol(arg, 0, 10);
	if((KeyIdx >= 1 ) && (KeyIdx <= 4))
		pApCliEntry->DefaultKeyId = (UCHAR) (KeyIdx - 1 );
	else
		return FALSE;  //Invalid argument 
	
	DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_DefaultKeyID_Proc::(DefaultKeyID(0~3)=%d)\n", ifIndex, pApCliEntry->DefaultKeyId));

	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WPA PSK key for ApCli link

    Arguments:
        pAdapter            Pointer to our adapter
        arg                 WPA pre-shared key string

    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_WPAPSK_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR keyMaterial[40];
	UCHAR ifIndex;
	POS_COOKIE pObj;
	PAPCLI_STRUCT   pApCliEntry = NULL;
	
	pObj = (POS_COOKIE) pAd->OS_Cookie;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;	
	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];
	
	DBGPRINT(RT_DEBUG_TRACE, ("Set_ApCli_WPAPSK_Proc::(WPAPSK=%s)\n", arg));

	if ((strlen(arg) < 8) || (strlen(arg) > 64))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set failed!!(WPAPSK=%s), WPAPSK key-string required 8 ~ 64 characters \n", ifIndex, arg));
		return FALSE;
	}

	NdisMoveMemory(pApCliEntry->PSK, arg, strlen(arg));
	pApCliEntry->PSKLen = strlen(arg);

	if (strlen(arg) == 64)
	{
	    AtoH(arg, pApCliEntry->PMK, 32);
	}
	else
	{
	    PasswordHash((char *)arg, pApCliEntry->CfgSsid, pApCliEntry->CfgSsidLen, keyMaterial);
	    NdisMoveMemory(pApCliEntry->PMK, keyMaterial, 32);
	}
			
	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY1 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key1_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT				KeyLen;
	INT				i;
	UCHAR			CipherAlg = CIPHER_NONE;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
		case 13: //wep 104 Ascii type
			pApCliEntry->SharedKey[0].KeyLen = KeyLen;
			NdisMoveMemory(pApCliEntry->SharedKey[0].Key, arg, KeyLen);
			if (KeyLen == 5)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key1_Proc::(Key1=%s ,type=%s, Alg=%s)\n", ifIndex, arg, "Ascii", CipherName[CipherAlg]));		
			break;
		case 10: //wep 40 Hex type
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pApCliEntry->SharedKey[0].KeyLen = KeyLen/2 ;
			AtoH(arg, pApCliEntry->SharedKey[0].Key, KeyLen/2);
			if (KeyLen == 10)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key1_Proc::(Key1=%s, type=%s, Alg=%s)\n", ifIndex, arg, "Hex", CipherName[CipherAlg]));		
			break;				
		default: //Invalid argument 
			pApCliEntry->SharedKey[0].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key1_Proc::Invalid argument (=%s)\n", ifIndex, arg));		
			return FALSE;
	}

	pApCliEntry->SharedKey[0].CipherAlg = CipherAlg;
    
	return TRUE;
}


/* 
    ==========================================================================
    Description:
        Set WEP KEY2 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key2_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT				KeyLen;
	INT				i;
	UCHAR			CipherAlg = CIPHER_NONE;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
		case 13: //wep 104 Ascii type
			pApCliEntry->SharedKey[1].KeyLen = KeyLen;
			NdisMoveMemory(pApCliEntry->SharedKey[1].Key, arg, KeyLen);
			if (KeyLen == 5)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key2_Proc::(Key2=%s ,type=%s, Alg=%s)\n", ifIndex, arg, "Ascii", CipherName[CipherAlg]));		
			break;
		case 10: //wep 40 Hex type
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pApCliEntry->SharedKey[1].KeyLen = KeyLen/2 ;
			AtoH(arg, pApCliEntry->SharedKey[1].Key, KeyLen/2);
			if (KeyLen == 10)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key2_Proc::(Key2=%s, type=%s, Alg=%s)\n", ifIndex, arg, "Hex", CipherName[CipherAlg]));		
			break;				
		default: //Invalid argument 
			pApCliEntry->SharedKey[1].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key2_Proc::Invalid argument (=%s)\n", ifIndex, arg));		
			return FALSE;
	}

	pApCliEntry->SharedKey[1].CipherAlg = CipherAlg;
    
	return TRUE;
}



/* 
    ==========================================================================
    Description:
        Set WEP KEY3 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key3_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT				KeyLen;
	INT				i;
	UCHAR			CipherAlg = CIPHER_NONE;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex;	
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
		case 13: //wep 104 Ascii type
			pApCliEntry->SharedKey[2].KeyLen = KeyLen;
			NdisMoveMemory(pApCliEntry->SharedKey[2].Key, arg, KeyLen);
			if (KeyLen == 5)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key3_Proc::(Key3=%s ,type=%s, Alg=%s)\n", ifIndex, arg, "Ascii", CipherName[CipherAlg]));		
			break;
		case 10: //wep 40 Hex type
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pApCliEntry->SharedKey[2].KeyLen = KeyLen/2 ;
			AtoH(arg, pApCliEntry->SharedKey[2].Key, KeyLen/2);
			if (KeyLen == 10)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key3_Proc::(Key3=%s, type=%s, Alg=%s)\n", ifIndex, arg, "Hex", CipherName[CipherAlg]));		
			break;				
		default: //Invalid argument 
			pApCliEntry->SharedKey[2].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key3_Proc::Invalid argument (=%s)\n", ifIndex, arg));		
			return FALSE;
	}

	pApCliEntry->SharedKey[2].CipherAlg = CipherAlg;
    
	return TRUE;
}



/* 
    ==========================================================================
    Description:
        Set WEP KEY4 for ApCli-IF
    Return:
        TRUE if all parameters are OK, FALSE otherwise
    ==========================================================================
*/
INT	Set_ApCli_Key4_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT				KeyLen;
	INT				i;
	UCHAR			CipherAlg = CIPHER_NONE;
	POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
	UCHAR			ifIndex;
	PAPCLI_STRUCT   pApCliEntry = NULL;

	if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;

	ifIndex = pObj->ioctl_if;

	pApCliEntry = &pAd->ApCfg.ApCliTab[ifIndex];

	KeyLen = strlen(arg);

	switch (KeyLen)
	{
		case 5: //wep 40 Ascii type
		case 13: //wep 104 Ascii type
			pApCliEntry->SharedKey[3].KeyLen = KeyLen;
			NdisMoveMemory(pApCliEntry->SharedKey[3].Key, arg, KeyLen);
			if (KeyLen == 5)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key4_Proc::(Key4=%s ,type=%s, Alg=%s)\n", ifIndex, arg, "Ascii", CipherName[CipherAlg]));		
			break;
		case 10: //wep 40 Hex type
		case 26: //wep 104 Hex type
			for(i=0; i < KeyLen; i++)
			{
				if( !isxdigit(*(arg+i)) )
					return FALSE;  //Not Hex value;
			}
			pApCliEntry->SharedKey[3].KeyLen = KeyLen/2 ;
			AtoH(arg, pApCliEntry->SharedKey[3].Key, KeyLen/2);
			if (KeyLen == 10)
				CipherAlg = CIPHER_WEP64;
			else
				CipherAlg = CIPHER_WEP128;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key4_Proc::(Key4=%s, type=%s, Alg=%s)\n", ifIndex, arg, "Hex", CipherName[CipherAlg]));		
			break;				
		default: //Invalid argument 
			pApCliEntry->SharedKey[3].KeyLen = 0;
			DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_ApCli_Key4_Proc::Invalid argument (=%s)\n", ifIndex, arg));		
			return FALSE;
	}

	pApCliEntry->SharedKey[3].CipherAlg = CipherAlg;
    
	return TRUE;
}

#ifdef WSC_AP_SUPPORT
INT Set_AP_WscSsid_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    POS_COOKIE 		pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR			ifIndex = pObj->ioctl_if;
	PWSC_CTRL	    pWscControl = &pAd->ApCfg.ApCliTab[ifIndex].WscControl;

    if (pObj->ioctl_if_type != INT_APCLI)
		return FALSE;
    
	NdisZeroMemory(&pWscControl->WscSsid, sizeof(NDIS_802_11_SSID));

	if( (strlen(arg) > 0) && (strlen(arg) <= MAX_LEN_OF_SSID))
    {
		NdisMoveMemory(pWscControl->WscSsid.Ssid, arg, strlen(arg));
		pWscControl->WscSsid.SsidLength = strlen(arg);

		DBGPRINT(RT_DEBUG_TRACE, ("Set_WscSsid_Proc:: (Select SsidLen=%d,Ssid=%s)\n", 
				pWscControl->WscSsid.SsidLength, pWscControl->WscSsid.Ssid));
	}
	else
		return FALSE;	//Invalid argument 

	return TRUE;	

}
#endif // WSC_AP_SUPPORT
#endif // APCLI_SUPPORT //


#ifdef WSC_AP_SUPPORT
INT	 Set_AP_WscConfMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT         ConfModeIdx;
	//INT         IsAPConfigured;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    BOOLEAN     bFromApCli = FALSE;
    PWSC_CTRL   pWscControl;

	ConfModeIdx = simple_strtol(arg, 0, 10);

    // Only support WPS in ra0 now, 2006.11.10
    if (apidx != MAIN_MBSSID)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfMode_Proc:: Only support WPS in ra0 or apcli0 now.\n", apidx));
        return FALSE;
    }

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif // APCLI_SUPPORT //
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfMode_Proc:: This command is from ra interface now.\n", apidx));
    }
        
    pWscControl->bWscTrigger = FALSE;
    if ((ConfModeIdx & WSC_ENROLLEE_PROXY_REGISTRAR) == WSC_DISABLE)
    {
        pWscControl->WscConfMode = WSC_DISABLE;
        if (bFromApCli)
        {
            DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: WPS is disabled.\n", apidx));
        }
        else
        {
            DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfMode_Proc:: WPS is disabled.\n", apidx));
            // Clear WPS IE in Beacon and ProbeResp
            pAd->ApCfg.MBSSID[apidx].WscIEBeacon.ValueLen = 0;
        	pAd->ApCfg.MBSSID[apidx].WscIEProbeResp.ValueLen = 0;
            APMakeAllBssBeacon(pAd);
            APUpdateAllBeaconFrame(pAd);
        }        
    }
    else
    {
        if (bFromApCli)
        {
            if (ConfModeIdx == WSC_ENROLLEE)
            {
                pWscControl->WscConfMode = WSC_ENROLLEE;
                WscInit(pAd, TRUE, pWscControl);
            }
            else
            {
                DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfMode_Proc:: Ap Client only supports Enrollee mode.(ConfModeIdx=%d)\n", apidx, ConfModeIdx));
                return FALSE;
            }
        }
        else
        {
        	pWscControl->WscConfMode = (ConfModeIdx & WSC_ENROLLEE_PROXY_REGISTRAR);
            WscInit(pAd, FALSE, pWscControl);
        }
        pWscControl->WscStatus = STATUS_WSC_IDLE;
    }

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
        memcpy(mac_addr, &pAd->ApCfg.ApCliTab[apidx].CurrentAddress[0], MAC_ADDR_LEN);
    }
    else
#endif // APCLI_SUPPORT //        
    {
        memcpy(mac_addr, &pAd->ApCfg.MBSSID[apidx].Bssid[0], MAC_ADDR_LEN);
    }

	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscConfMode_Proc::(WscConfMode(0~7)=%d)\n", 
                            mac_addr[0], 
                            mac_addr[1], 
                            mac_addr[2], 
                            mac_addr[3], 
                            mac_addr[4], 
                            mac_addr[5], 
                            pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode));
	return TRUE;
}

INT	Set_AP_WscConfStatus_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR       IsAPConfigured = 0;
	INT         IsSelectedRegistrar;
	USHORT      WscMode;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscConfStatus_Proc:: Ap Client doesn't need this command.\n", apidx));
        return FALSE;
    }
#endif // APCLI_SUPPORT //

    // Only support WPS in ra0 now, 2006.11.10
    if ((apidx != MAIN_MBSSID) || 
        (pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode == WSC_DISABLE))
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: Only support WPS in ra0 now.\n", apidx));
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: WPS is %s.\n", 
                 apidx,
                (pAd->ApCfg.MBSSID[apidx].WscControl.WscConfMode == WSC_DISABLE) ? "Disabled":"Enabled"));
        return FALSE;
    }

	IsAPConfigured = (UCHAR)simple_strtol(arg, 0, 10);
	IsSelectedRegistrar = pAd->ApCfg.MBSSID[apidx].WscControl.WscSelReg;
    if (pAd->ApCfg.MBSSID[apidx].WscControl.WscMode == 1)
		WscMode = DEV_PASS_ID_PIN;
	else
		WscMode = DEV_PASS_ID_PBC;

	if ((IsAPConfigured  > 0) && (IsAPConfigured  <= 2))
    {   
        pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus = IsAPConfigured;
        // Change SC State of WPS IE in Beacon and ProbeResp
        WscBuildBeaconIE(pAd, IsAPConfigured, IsSelectedRegistrar, WscMode, 0);
    	WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, IsSelectedRegistrar, WscMode, 0);

    	APMakeAllBssBeacon(pAd);
    	APUpdateAllBeaconFrame(pAd);
    }
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: Set failed!!(WscConfStatus=%s), WscConfStatus is 1 or 2 \n", apidx, arg));
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscConfStatus_Proc:: WscConfStatus is not changed (%d) \n", apidx, pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus));
		return FALSE;  //Invalid argument	
	}

	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscConfStatus_Proc::(WscConfStatus=%d)\n", 
                               pAd->ApCfg.MBSSID[apidx].Bssid[0],
                               pAd->ApCfg.MBSSID[apidx].Bssid[1],
                               pAd->ApCfg.MBSSID[apidx].Bssid[2],
                               pAd->ApCfg.MBSSID[apidx].Bssid[3],
                               pAd->ApCfg.MBSSID[apidx].Bssid[4],
                               pAd->ApCfg.MBSSID[apidx].Bssid[5],
                               pAd->ApCfg.MBSSID[apidx].WscControl.WscConfStatus));

	return TRUE;
}

INT	Set_AP_WscMode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT         WscMode;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    PWSC_CTRL   pWscControl;
    BOOLEAN     bFromApCli = FALSE;
    
    // Only support WPS in ra0 now, 2006.11.10
    if (apidx != MAIN_MBSSID)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscMode_Proc:: Only support WPS in ra0 or apcli0 now.\n", apidx));
        return FALSE;
    }

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif // APCLI_SUPPORT //
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscMode_Proc:: This command is from ra interface now.\n", apidx));
    }

	WscMode = simple_strtol(arg, 0, 10);
    
    if ((WscMode  > 0) && (WscMode  <= 2))
    {
        pWscControl->WscMode = WscMode;
        if (WscMode == 2)
        {
	        WscGetRegDataPIN(pAd, pWscControl->WscPinCode, pWscControl);
        }
    }
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set_WscMode_Proc:: Set failed!!(Set_WscMode_Proc=%s), WscConfStatus is 1 or 2 \n", arg));
        DBGPRINT(RT_DEBUG_TRACE, ("Set_WscMode_Proc:: WscMode is not changed (%d) \n", pWscControl->WscMode));
		return FALSE;  //Invalid argument
	}

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
        memcpy(mac_addr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress, MAC_ADDR_LEN);
    }
    else
#endif // APCLI_SUPPORT //        
    {
        memcpy(mac_addr, pAd->ApCfg.MBSSID[apidx].Bssid, MAC_ADDR_LEN);
    }
	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscMode_Proc::(WscMode=%d)\n", 
                                mac_addr[0],
                                mac_addr[1],
                                mac_addr[2],
                                mac_addr[3],
                                mac_addr[4],
                                mac_addr[5],
                                pWscControl->WscMode));

	return TRUE;
}

INT	Set_WscStatus_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;
    
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscStatus_Proc::(WscStatus=%d)\n", apidx, pAd->ApCfg.MBSSID[apidx].WscControl.WscStatus));
	return TRUE;
}

#define WSC_GET_CONF_MODE_EAP	1
#define WSC_GET_CONF_MODE_UPNP	2
INT	Set_AP_WscGetConf_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	INT                 WscMode, wscGetConfMode = 0;
	INT                 IsAPConfigured;
	PWSC_CTRL           pWscControl;
	PWSC_UPNP_NODE_INFO pWscUPnPNodeInfo;
    INT	                idx;
    POS_COOKIE          pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	            apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    BOOLEAN             bFromApCli = FALSE;

    // Only support WPS in ra0 now, 2006.11.10
    if (apidx != MAIN_MBSSID)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscGetConf_Proc:: Only support WPS in ra0 or apcli0 now.\n", apidx));
        return FALSE;
    }

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscMode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif // APCLI_SUPPORT //
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscMode_Proc:: This command is from ra interface now.\n", apidx));
    }

	wscGetConfMode = simple_strtol(arg, 0, 10);

    IsAPConfigured = pWscControl->WscConfStatus;
    pWscUPnPNodeInfo = &pWscControl->WscUPnPNodeInfo;

    if (pWscControl->WscConfMode == WSC_DISABLE)
    {
        pWscControl->bWscTrigger = FALSE;
        DBGPRINT(RT_DEBUG_TRACE, ("Set_WscGetConf_Proc: WPS is disabled.\n"));
		return FALSE;
    }

    if (bFromApCli)
	    WscStop(pAd, TRUE, pWscControl);
    else
        WscStop(pAd, FALSE, pWscControl);
    
	// trigger wsc re-generate public key
    pWscControl->RegData.ReComputePke = 1;

	if (pWscControl->WscMode == 1)
		WscMode = DEV_PASS_ID_PIN;
	else
		WscMode = DEV_PASS_ID_PBC;
    
	WscInitRegistrarPair(pAd, pWscControl);
    // Enrollee 192 random bytes for DH key generation
	for (idx = 0; idx < 192; idx++)
		pWscControl->RegData.EnrolleeRandom[idx] = RandomByte(pAd);
    
    if (!bFromApCli)
    {
    	WscBuildBeaconIE(pAd, IsAPConfigured, TRUE, WscMode, WSC_CONFIG_METHODS);
    	WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, TRUE, WscMode, WSC_CONFIG_METHODS);

    	APMakeAllBssBeacon(pAd);
    	APUpdateAllBeaconFrame(pAd);
    }
#ifdef APCLI_SUPPORT    
    else
    {
        BOOLEAN apcliEn = pAd->ApCfg.ApCliTab[apidx].Enable;

        NdisMoveMemory(pWscControl->RegData.EnrolleeInfo.MacAddr,
                       pAd->ApCfg.ApCliTab[apidx].CurrentAddress, 
                       6);
        
        // bring apcli interface down first
		if(apcliEn == TRUE )
		{
			pAd->ApCfg.ApCliTab[apidx].Enable = FALSE;
			ApCliIfDown(pAd);
		}
        pAd->ApCfg.ApCliTab[apidx].Enable = apcliEn;
    }
#endif // APCLI_SUPPORT //

    // 2mins time-out timer
    RTMPSetTimer(&pWscControl->Wsc2MinsTimer, WSC_TWO_MINS_TIME_OUT);
    pWscControl->Wsc2MinsTimerRunning = TRUE;
    pWscControl->WscStatus = STATUS_WSC_LINK_UP;

    if (!bFromApCli)
	    WscSendUPnPConfReqMsg(pAd, pAd->ApCfg.MBSSID[apidx].Ssid, pAd->ApCfg.MBSSID[apidx].Bssid, 3, 0);

    pWscControl->bWscTrigger = TRUE;

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
        memcpy(mac_addr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress, MAC_ADDR_LEN);
    }
    else
#endif // APCLI_SUPPORT //        
    {
        memcpy(mac_addr, pAd->ApCfg.MBSSID[apidx].Bssid, MAC_ADDR_LEN);
    }
	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscGetConf_Proc trigger WSC state machine, wscGetConfMode=%d\n", 
                                mac_addr[0],
                                mac_addr[1],
                                mac_addr[2],
                                mac_addr[3],
                                mac_addr[4],
                                mac_addr[5],
                                wscGetConfMode));

	return TRUE;
}

INT	Set_AP_WscPinCode_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UINT        PinCode = 0;
	BOOLEAN     validatePin, bFromApCli = FALSE;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR       apidx = pObj->ioctl_if, mac_addr[MAC_ADDR_LEN];
    PWSC_CTRL   pWscControl;
#define IsZero(c) ('0' == (c) ? TRUE:FALSE)
	PinCode = simple_strtol(arg, 0, 10); // When PinCode is 03571361, return value is 3571361.

    // Only support WPS in ra0 now, 2006.11.10
    if (apidx != MAIN_MBSSID)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscPinCode_Proc:: Only support WPS in ra0 or apcli0 now.\n", apidx));
        return FALSE;
    }

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscPinCode_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif // APCLI_SUPPORT //
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscPinCode_Proc:: This command is from ra interface now.\n", apidx));
    }
    
	validatePin = ValidateChecksum(PinCode);

	if ( validatePin )
	{
	    if (pWscControl->WscRejectSamePinFromEnrollee && 
            (PinCode == pWscControl->WscLastPinFromEnrollee))
        {
            DBGPRINT(RT_DEBUG_TRACE, ("PIN authentication or communication error occurs!!\n"
                                      "Registrar does NOT accept the same PIN again!(PIN:%s)\n", arg));
            return FALSE;
        }
        else
        {
    		pWscControl->WscPinCode = PinCode;
            pWscControl->WscLastPinFromEnrollee = pWscControl->WscPinCode;
            pWscControl->WscRejectSamePinFromEnrollee = FALSE;
            // PIN Code
            if (IsZero(*arg))
                NdisMoveMemory(pWscControl->RegData.PIN, arg, 8);
            else
    	        WscGetRegDataPIN(pAd, pWscControl->WscPinCode, pWscControl);
        }        
	}
	else
	{
		DBGPRINT(RT_DEBUG_TRACE, ("Set failed!!(Set_WscPinCode_Proc=%s), PinCode Checksum invalid \n", arg));
		return FALSE;  //Invalid argument
	}

#ifdef APCLI_SUPPORT
    if (bFromApCli)
    {
        memcpy(mac_addr, pAd->ApCfg.ApCliTab[apidx].CurrentAddress, MAC_ADDR_LEN);
    }
    else
#endif // APCLI_SUPPORT //        
    {
        memcpy(mac_addr, pAd->ApCfg.MBSSID[apidx].Bssid, MAC_ADDR_LEN);
    }
	DBGPRINT(RT_DEBUG_TRACE, ("IF(%02X:%02X:%02X:%02X:%02X:%02X) Set_WscPinCode_Proc::(PinCode=%d)\n", 
                                mac_addr[0],
                                mac_addr[1],
                                mac_addr[2],
                                mac_addr[3],
                                mac_addr[4],
                                mac_addr[5],
                                pWscControl->WscPinCode));

	return TRUE;
}

INT	Set_WscOOB_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    char        *pTempSsid = NULL;
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR       apidx = pObj->ioctl_if;
    
#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscPinCode_Proc:: Ap Client doesn't need this command.\n", apidx));
        return FALSE;
    }
#endif // APCLI_SUPPORT //

    // Only support WPS in ra0 now, 2006.11.10
    if (apidx != MAIN_MBSSID)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscOOB_Proc:: Only support WPS in ra0 now.\n", apidx));
        return FALSE;
    }

    Set_AP_WscConfStatus_Proc(pAd, "1");
    Set_AP_AuthMode_Proc(pAd, "WPAPSK");
    Set_AP_EncrypType_Proc(pAd, "TKIP");
    pTempSsid = vmalloc(33);
    if (pTempSsid)
    {
        memset(pTempSsid, 0, 33);
        sprintf(pTempSsid, "RalinkInitialAP%02X%02X%02X", pAd->ApCfg.MBSSID[apidx].Bssid[3],
                                                          pAd->ApCfg.MBSSID[apidx].Bssid[4],
                                                          pAd->ApCfg.MBSSID[apidx].Bssid[5]);
        Set_AP_SSID_Proc(pAd, pTempSsid);
        vfree(pTempSsid);
    }
	Set_AP_WPAPSK_Proc(pAd, "RalinkInitialAPxx1234");
    
	DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscOOB_Proc\n", apidx));
	return TRUE;
}

INT	Set_WscStop_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
    POS_COOKIE  pObj = (POS_COOKIE) pAd->OS_Cookie;
    UCHAR	    apidx = pObj->ioctl_if;
    PWSC_CTRL   pWscControl;
    BOOLEAN     bFromApCli = FALSE;
    
    // Only support WPS in ra0 now, 2006.11.10
    if (apidx != MAIN_MBSSID)
    {
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscStop_Proc:: Only support WPS in ra0 or apcli0 now.\n", apidx));
        return FALSE;
    }

#ifdef APCLI_SUPPORT
    if (pObj->ioctl_if_type == INT_APCLI)
    {
        bFromApCli = TRUE;
        pWscControl = &pAd->ApCfg.ApCliTab[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(apcli%d) Set_WscStop_Proc:: This command is from apcli interface now.\n", apidx));
    }
    else
#endif // APCLI_SUPPORT //
    {
        bFromApCli = FALSE;
        pWscControl = &pAd->ApCfg.MBSSID[apidx].WscControl;
        DBGPRINT(RT_DEBUG_TRACE, ("IF(ra%d) Set_WscStop_Proc:: This command is from ra interface now.\n", apidx));
    }

    if (bFromApCli)
	    WscStop(pAd, TRUE, pWscControl);
    else
    {
        INT	 IsAPConfigured = pWscControl->WscConfStatus;
        WscBuildBeaconIE(pAd, IsAPConfigured, FALSE, 0, 0);
		WscBuildProbeRespIE(pAd, WSC_MSGTYPE_AP_WLAN_MGR, IsAPConfigured, FALSE, 0, 0);
		APMakeAllBssBeacon(pAd);
		APUpdateAllBeaconFrame(pAd);
        WscStop(pAd, FALSE, pWscControl);
    }

    pWscControl->bWscTrigger = FALSE;
    DBGPRINT(RT_DEBUG_TRACE, ("<===== Set_WscStop_Proc"));
    return TRUE;
}

/* 
    ==========================================================================
    Description:
        Get WSC Profile
	Arguments:
	    pAdapter                    Pointer to our adapter
	    wrq                         Pointer to the ioctl argument

    Return Value:
        None

    Note:
        Usage:
        		1.) iwpriv ra0 get_wsc_profile
        		3.) UI needs to prepare at least 4096bytes to get the results
    ==========================================================================
*/
VOID RTMPIoctlWscProfile(
	IN	PRTMP_ADAPTER	pAd, 
	IN	struct iwreq	*wrq)
{
	WSC_CONFIGURED_VALUE Profile;
	char *msg;

	memset(&Profile, 0x00, sizeof(WSC_CONFIGURED_VALUE));
	Profile.WscConfigured = pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WscConfStatus;
	NdisZeroMemory(Profile.WscSsid, 32 + 1);
	NdisMoveMemory(Profile.WscSsid, pAd->ApCfg.MBSSID[MAIN_MBSSID].Ssid, 
								    pAd->ApCfg.MBSSID[MAIN_MBSSID].SsidLen);
	Profile.WscSsid[pAd->ApCfg.MBSSID[MAIN_MBSSID].SsidLen] = '\0';
	if (pAd->ApCfg.MBSSID[MAIN_MBSSID].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		Profile.WscAuthMode = WSC_AUTHTYPE_WPAPSK | WSC_AUTHTYPE_WPA2PSK;
	else
		Profile.WscAuthMode = WscGetAuthType(pAd->ApCfg.MBSSID[MAIN_MBSSID].AuthMode);
	if (pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus == Ndis802_11Encryption4Enabled)
		Profile.WscEncrypType = WSC_ENCRTYPE_TKIP |WSC_ENCRTYPE_AES;
	else
		Profile.WscEncrypType = WscGetEncryType(pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus);
	NdisZeroMemory(Profile.WscWPAKey, 64 + 1);

	if (Profile.WscEncrypType == 2)
	{
		Profile.DefaultKeyIdx = pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId + 1;
#if 0
		if (pAd->SharedKey[MAIN_MBSSID][pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId].WepKeyType == WEP_ASCII_TYPE)
		{
			int i;
			for (i=0; i<pAd->SharedKey[MAIN_MBSSID][pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId].KeyLen; i++)
			{
				sprintf(Profile.WscWPAKey,
						"%s%02x", Profile.WscWPAKey,
									pAd->SharedKey[MAIN_MBSSID][pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[MAIN_MBSSID][pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId].KeyLen)*2] = '\0';
	}
		else // Hex WEP Key
#endif // if 0
		{
			int i;
			for (i=0; i<pAd->SharedKey[MAIN_MBSSID][pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId].KeyLen; i++)
			{
				sprintf(Profile.WscWPAKey,
						"%s%02x", Profile.WscWPAKey,
									pAd->SharedKey[MAIN_MBSSID][pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId].Key[i]);
			}
			Profile.WscWPAKey[(pAd->SharedKey[MAIN_MBSSID][pAd->ApCfg.MBSSID[MAIN_MBSSID].DefaultKeyId].KeyLen)*2] = '\0';
		}
	}
	else if (Profile.WscEncrypType >= 4)
	{
		Profile.DefaultKeyIdx = 2;
		NdisMoveMemory(Profile.WscWPAKey, pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WpaPsk, 
						pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WpaPskLen);
		Profile.WscWPAKey[pAd->ApCfg.MBSSID[MAIN_MBSSID].WscControl.WpaPskLen] = '\0';
	}
	else
	{
		Profile.DefaultKeyIdx = 1;
	}

	wrq->u.data.length = sizeof(Profile);

	if (copy_to_user(wrq->u.data.pointer, &Profile, wrq->u.data.length))
	{
		DBGPRINT(RT_DEBUG_TRACE, ("%s: copy_to_user() fail\n", __FUNCTION__));
	}

	msg = (CHAR *) kmalloc(sizeof(CHAR)*(2048), MEM_ALLOC_FLAG);
	if (msg == NULL) {
		return;
	}

	memset(msg, 0x00, 2048);
	sprintf(msg,"%s","\n");

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s\n", "Configured", "SSID", "AuthMode", "EncrypType");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-13s%-26s\n", "Configured", "SSID", "AuthMode", "EncrypType", "DefaultKeyID", "Key");
	}
	else
	{
		sprintf(msg+strlen(msg),"%-12s%-33s%-12s%-12s%-64s\n", "Configured", "SSID", "AuthMode", "EncrypType", "Key");
	}

	if (Profile.WscConfigured == 1)
		sprintf(msg+strlen(msg),"%-12s", "No");
	else
		sprintf(msg+strlen(msg),"%-12s", "Yes");
	sprintf(msg+strlen(msg), "%-33s", Profile.WscSsid);
	if (pAd->ApCfg.MBSSID[MAIN_MBSSID].AuthMode == Ndis802_11AuthModeWPA1PSKWPA2PSK)
		sprintf(msg+strlen(msg), "%-12s", "WPAPSKWPA2PSK");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetAuthTypeStr(Profile.WscAuthMode));
	if (pAd->ApCfg.MBSSID[MAIN_MBSSID].WepStatus == Ndis802_11Encryption4Enabled)
		sprintf(msg+strlen(msg), "%-12s", "TKIPAES");
	else
		sprintf(msg+strlen(msg), "%-12s", WscGetEncryTypeStr(Profile.WscEncrypType));

	if (Profile.WscEncrypType == 1)
	{
		sprintf(msg+strlen(msg), "%s\n", "");
	}
	else if (Profile.WscEncrypType == 2)
	{
		sprintf(msg+strlen(msg), "%-13d",Profile.DefaultKeyIdx);
		sprintf(msg+strlen(msg), "%-26s\n",Profile.WscWPAKey);
	}
	else if (Profile.WscEncrypType >= 4)
	{
	    sprintf(msg+strlen(msg), "%-64s\n",Profile.WscWPAKey);
	}

	DBGPRINT(RT_DEBUG_TRACE, ("%s", msg));

	kfree(msg);
}

BOOLEAN WscCheckEnrolleeNonceFromUpnp(
	IN	PRTMP_ADAPTER	pAdapter, 
	IN	UCHAR	        *pData,
	IN  USHORT			Length,
	IN  PWSC_CTRL       pWscControl) 
{
	USHORT	WscType, WscLen;
    USHORT  WscId = WSC_ID_ENROLLEE_NONCE;

    DBGPRINT(RT_DEBUG_TRACE, ("check Enrollee Nonce\n"));
   
    // We have to look for WSC_IE_MSG_TYPE to classify M2 ~ M8, the remain size must large than 4
	while (Length > 4)
	{
		WSC_TLV_0B	TLV_Recv;
        char ZeroNonce[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        
		memcpy((u8 *)&TLV_Recv, pData, 4);
		WscType = cpu2be16(TLV_Recv.tag);
		WscLen  = cpu2be16(TLV_Recv.len);
		pData  += 4;
		Length -= 4;
        
		if (WscType == WscId)
		{
			if (RTMPCompareMemory(pWscControl->RegData.SelfNonce, pData, 16) == 0)
			{
			    DBGPRINT(RT_DEBUG_TRACE, ("Nonce match!!\n"));
                DBGPRINT(RT_DEBUG_TRACE, ("<----- WscCheckNonce\n"));
				return TRUE;
			}
            else if (NdisEqualMemory(pData, ZeroNonce, 16))
            {
                // Intel external registrar will send WSC_NACK with enrollee nonce
                // "10 1A 00 10 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
                // when AP is configured and user selects not to configure AP.
                DBGPRINT(RT_DEBUG_TRACE, ("Zero Enrollee Nonce!!\n"));
                DBGPRINT(RT_DEBUG_TRACE, ("<----- WscCheckNonce\n"));
                return TRUE;
            }
		}
        
		// Offset to net WSC Ie
		pData  += WscLen;
		Length -= WscLen;
	}

    DBGPRINT(RT_DEBUG_TRACE, ("Nonce mismatch!!\n"));
    return FALSE;
}

UCHAR	WscRxMsgTypeFromUpnp(
	IN	PRTMP_ADAPTER		pAdapter,
	IN  PUCHAR				pData,
	IN	USHORT				Length) 
{
	
	USHORT WscType, WscLen;
    
    {   // Eap-Esp(Messages)
        // the first TLV item in EAP Messages must be WSC_IE_VERSION
        NdisMoveMemory(&WscType, pData, 2);
        if (ntohs(WscType) != WSC_ID_VERSION)
            goto out;

        // Not Wsc Start, We have to look for WSC_IE_MSG_TYPE to classify M2 ~ M8, the remain size must large than 4
		while (Length > 4)
		{
			// arm-cpu has packet alignment issue, it's better to use memcpy to retrieve data
			NdisMoveMemory(&WscType, pData, 2);
			NdisMoveMemory(&WscLen,  pData + 2, 2);
			WscLen = ntohs(WscLen);
			if (ntohs(WscType) == WSC_ID_MSG_TYPE)
			{
				return(*(pData + 4));	// Found the message type
			}
			else
			{
				pData  += (WscLen + 4);
				Length -= (WscLen + 4);
			}
		}
    }

out:
	return  WSC_MSG_UNKNOWN;
}
#endif // WSC_AP_SUPPORT //

#ifdef IAPP_SUPPORT
INT	Set_IappPID_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	POS_COOKIE	pObj = (POS_COOKIE) pAd->OS_Cookie;


	pObj->IappPid = simple_strtol(arg, 0, 10);
	DBGPRINT(RT_DEBUG_TRACE, ("pObj->IappPid = %ld", pObj->IappPid));
	return TRUE;
} /* End of Set_IappPID_Proc */
#endif // IAPP_SUPPORT //

INT	Set_DisassociateSta_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	UCHAR					macAddr[MAC_ADDR_LEN];
	CHAR					*value;
	INT						i;
	UCHAR HashIdx;
	MAC_TABLE_ENTRY *pEntry = NULL;

	if(strlen(arg) != 17)  //Mac address acceptable format 01:02:03:04:05:06 length 17
		return FALSE;

	for (i=0, value = rstrtok(arg,":"); value; value = rstrtok(NULL,":")) 
	{
		if((strlen(value) != 2) || (!isxdigit(*value)) || (!isxdigit(*(value+1))) ) 
			return FALSE;  //Invalid

		AtoH(value, &macAddr[i++], 2);
	}

	HashIdx = MAC_ADDR_HASH_INDEX(macAddr);
	pEntry = pAd->MacTab.Hash[HashIdx];

	if (pEntry)
	{
		DisAssocAction(pAd, pEntry, REASON_DISASSOC_STA_LEAVING);
//		MacTableDeleteEntry(pAd, pEntry->Aid, Addr);				
	}

	return TRUE;
}


#ifdef UAPSD_AP_SUPPORT
INT Set_UAPSD_Proc(
	IN	PRTMP_ADAPTER	pAd, 
	IN	PUCHAR			arg)
{
	if (simple_strtol(arg, 0, 10) != 0)
		pAd->CommonCfg.bAPSDCapable = TRUE;
	else
		pAd->CommonCfg.bAPSDCapable = FALSE;
	/* End of if */

	return TRUE;
} /* End of Set_UAPSD_Proc */
#endif // UAPSD_AP_SUPPORT //

#ifdef CONFIG_AP_SUPPORT
#ifdef MCAST_RATE_SPECIFIC
INT Set_McastPhyMode(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR arg)
{
	UCHAR PhyMode = simple_strtol(arg, 0, 10);

	switch (PhyMode)
	{
		case 0: // disable
			pAd->CommonCfg.McastTransmitPhyMode = MCAST_DISABLE;
			break;
		case 1:	// CCK
			pAd->CommonCfg.McastTransmitPhyMode = MCAST_CCK;
			break;
		case 2:	// CCK
			pAd->CommonCfg.McastTransmitPhyMode = MCAST_OFDM;
			break;
		default:
			printk("unknow Muticast PhyMode %d.\n", PhyMode);
			printk("0:Disable 1:CCK, 2:OFDM.\n");
			break;
	}

	return TRUE;
}

INT Set_McastMcs(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR arg)
{
	UCHAR Mcs = simple_strtol(arg, 0, 10);

	if (Mcs <= 15)
		pAd->CommonCfg.McastTransmitMcs = Mcs;
	else
		printk("Mcs must in range of 0 to 15\n");

	return TRUE;
}

INT Show_McastRate(
	IN PRTMP_ADAPTER pAd,
	IN PUCHAR arg)
{
	printk("Mcast PhyMode =%d\n", pAd->CommonCfg.McastTransmitPhyMode);
	printk("Mcast Mcs =%d\n", pAd->CommonCfg.McastTransmitMcs);
	return TRUE;
}
#endif // MCAST_RATE_SPECIFIC //
#endif // CONFIG_AP_SUPPORT //

