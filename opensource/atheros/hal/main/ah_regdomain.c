/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * This module contains the regulatory domain table and accessor functions
 * for the information in the table.
 * The channel list creation is also contained in this module.
 *
 * "The country table and respective Regulatory Domain channel and power
 * settings are based on available knowledge as of software release. The
 * underlying global regulatory and spectrum rules change on a regular basis,
 * therefore, no warranty is given that the channel and power information
 * herein is complete, accurate or up to date.  Developers are responsible
 * for regulatory compliance of end-products developed using the enclosed
 * data per all applicable national requirements.  Furthermore, data in this
 * table does not guarantee that spectrum is available and that regulatory
 * approval is possible in every case. Knowldegable regulatory compliance
 * or government contacts should be consulted by the manufacturer to ensure
 * that the most current and accurate settings are used in each end-product.
 * This table was designed so that developers are able to update the country
 * table mappings as well as the Regulatory Domain definitions in order to
 * incorporate the most current channel and power settings in the end-product."
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ah_regdomain.c#12 $
 * $Atheros: //depot/sw/src/common/wlanchannel.c#258 $
 */
#include "opt_ah.h"

#include "ah.h"
#include "ah_internal.h"

/*
 * XXX this code needs a audit+review
 */

/* used throughout this file... */
#define	N(a)	(sizeof (a) / sizeof (a[0]))

#define HAL_MODE_11A_TURBO	HAL_MODE_108A
#define HAL_MODE_11G_TURBO	HAL_MODE_108G

/* 10MHz is half the 11A bandwidth used to determine upper edge freq
   of the outdoor channel */
#define HALF_MAXCHANBW		10
					   
/* 
 * Used to set the RegDomain bitmask which chooses which frequency
 * band specs are used.
 */

#define BMLEN 2		/* Use 2 64 bit uint for channel bitmask
			   NB: Must agree with macro below (BM) */
#define BMZERO {0,0}	/* BMLEN zeros */
			
#define BM(_fa, _fb, _fc, _fd, _fe, _ff, _fg, _fh) \
      {((((_fa >= 0) && (_fa < 64)) ? (((u_int64_t) 1) << _fa) : 0) | \
	(((_fb >= 0) && (_fb < 64)) ? (((u_int64_t) 1) << _fb) : 0) | \
	(((_fc >= 0) && (_fc < 64)) ? (((u_int64_t) 1) << _fc) : 0) | \
	(((_fd >= 0) && (_fd < 64)) ? (((u_int64_t) 1) << _fd) : 0) | \
	(((_fe >= 0) && (_fe < 64)) ? (((u_int64_t) 1) << _fe) : 0) | \
	(((_ff >= 0) && (_ff < 64)) ? (((u_int64_t) 1) << _ff) : 0) | \
	(((_fg >= 0) && (_fg < 64)) ? (((u_int64_t) 1) << _fg) : 0) | \
	(((_fh >= 0) && (_fh < 64)) ? (((u_int64_t) 1) << _fh) : 0)), \
	       ((((_fa > 63) && (_fa < 128)) ? (((u_int64_t) 1) << (_fa - 64)) : 0) | \
		(((_fb > 63) && (_fb < 128)) ? (((u_int64_t) 1) << (_fb - 64)) : 0) | \
		(((_fc > 63) && (_fc < 128)) ? (((u_int64_t) 1) << (_fc - 64)) : 0) | \
		(((_fd > 63) && (_fd < 128)) ? (((u_int64_t) 1) << (_fd - 64)) : 0) | \
		(((_fe > 63) && (_fe < 128)) ? (((u_int64_t) 1) << (_fe - 64)) : 0) | \
		(((_ff > 63) && (_ff < 128)) ? (((u_int64_t) 1) << (_ff - 64)) : 0) | \
		(((_fg > 63) && (_fg < 128)) ? (((u_int64_t) 1) << (_fg - 64)) : 0) | \
		(((_fh > 63) && (_fh < 128)) ? (((u_int64_t) 1) << (_fh - 64)) : 0))}


/*
 * Country/Region Codes from MS WINNLS.H
 * Numbering from ISO 3166
 */
enum CountryCode {
    CTRY_ALBANIA              = 8,       /* Albania */
    CTRY_ALGERIA              = 12,      /* Algeria */
    CTRY_ARGENTINA            = 32,      /* Argentina */
    CTRY_ARMENIA              = 51,      /* Armenia */
    CTRY_AUSTRALIA            = 36,      /* Australia */
    CTRY_AUSTRIA              = 40,      /* Austria */
    CTRY_AZERBAIJAN           = 31,      /* Azerbaijan */
    CTRY_BAHRAIN              = 48,      /* Bahrain */
    CTRY_BELARUS              = 112,     /* Belarus */
    CTRY_BELGIUM              = 56,      /* Belgium */
    CTRY_BELIZE               = 84,      /* Belize */
    CTRY_BOLIVIA              = 68,      /* Bolivia */
    CTRY_BRAZIL               = 76,      /* Brazil */
    CTRY_BRUNEI_DARUSSALAM    = 96,      /* Brunei Darussalam */
    CTRY_BULGARIA             = 100,     /* Bulgaria */
    CTRY_CANADA               = 124,     /* Canada */
    CTRY_CHILE                = 152,     /* Chile */
    CTRY_CHINA                = 156,     /* People's Republic of China */
    CTRY_COLOMBIA             = 170,     /* Colombia */
    CTRY_COSTA_RICA           = 188,     /* Costa Rica */
    CTRY_CROATIA              = 191,     /* Croatia */
    CTRY_CYPRUS               = 196,
    CTRY_CZECH                = 203,     /* Czech Republic */
    CTRY_DENMARK              = 208,     /* Denmark */
    CTRY_DOMINICAN_REPUBLIC   = 214,     /* Dominican Republic */
    CTRY_ECUADOR              = 218,     /* Ecuador */
    CTRY_EGYPT                = 818,     /* Egypt */
    CTRY_EL_SALVADOR          = 222,     /* El Salvador */
    CTRY_ESTONIA              = 233,     /* Estonia */
    CTRY_FAEROE_ISLANDS       = 234,     /* Faeroe Islands */
    CTRY_FINLAND              = 246,     /* Finland */
    CTRY_FRANCE               = 250,     /* France */
    CTRY_FRANCE2              = 255,     /* France2 */
    CTRY_GEORGIA              = 268,     /* Georgia */
    CTRY_GERMANY              = 276,     /* Germany */
    CTRY_GREECE               = 300,     /* Greece */
    CTRY_GUATEMALA            = 320,     /* Guatemala */
    CTRY_HONDURAS             = 340,     /* Honduras */
    CTRY_HONG_KONG            = 344,     /* Hong Kong S.A.R., P.R.C. */
    CTRY_HUNGARY              = 348,     /* Hungary */
    CTRY_ICELAND              = 352,     /* Iceland */
    CTRY_INDIA                = 356,     /* India */
    CTRY_INDONESIA            = 360,     /* Indonesia */
    CTRY_IRAN                 = 364,     /* Iran */
    CTRY_IRAQ                 = 368,     /* Iraq */
    CTRY_IRELAND              = 372,     /* Ireland */
    CTRY_ISRAEL               = 376,     /* Israel */
    CTRY_ITALY                = 380,     /* Italy */
    CTRY_JAMAICA              = 388,     /* Jamaica */
    CTRY_JAPAN                = 392,     /* Japan */
    CTRY_JAPAN1               = 393,     /* Japan (JP1) */
    CTRY_JAPAN2               = 394,     /* Japan (JP0) */
    CTRY_JAPAN3               = 395,     /* Japan (JP1-1) */
    CTRY_JAPAN4               = 396,     /* Japan (JE1) */
    CTRY_JAPAN5               = 397,     /* Japan (JE2) */
    CTRY_JAPAN6		      = 399,	 /* Japan (JP6) */
    CTRY_JORDAN               = 400,     /* Jordan */
    CTRY_KAZAKHSTAN           = 398,     /* Kazakhstan */
    CTRY_KENYA                = 404,     /* Kenya */
    CTRY_KOREA_NORTH          = 408,     /* North Korea */
    CTRY_KOREA_ROC            = 410,     /* South Korea */
    CTRY_KOREA_ROC2           = 411,     /* South Korea */
    CTRY_KOREA_ROC3           = 412,     /* South Korea */
    CTRY_KUWAIT               = 414,     /* Kuwait */
    CTRY_LATVIA               = 428,     /* Latvia */
    CTRY_LEBANON              = 422,     /* Lebanon */
    CTRY_LIBYA                = 434,     /* Libya */
    CTRY_LIECHTENSTEIN        = 438,     /* Liechtenstein */
    CTRY_LITHUANIA            = 440,     /* Lithuania */
    CTRY_LUXEMBOURG           = 442,     /* Luxembourg */
    CTRY_MACAU                = 446,     /* Macau */
    CTRY_MACEDONIA            = 807,     /* the Former Yugoslav Republic of Macedonia */
    CTRY_MALAYSIA             = 458,     /* Malaysia */
    CTRY_MALTA		      = 470,	 /* Malta */
    CTRY_MEXICO               = 484,     /* Mexico */
    CTRY_MONACO               = 492,     /* Principality of Monaco */
    CTRY_MOROCCO              = 504,     /* Morocco */
    CTRY_NETHERLANDS          = 528,     /* Netherlands */
    CTRY_NEW_ZEALAND          = 554,     /* New Zealand */
    CTRY_NICARAGUA            = 558,     /* Nicaragua */
    CTRY_NORWAY               = 578,     /* Norway */
    CTRY_OMAN                 = 512,     /* Oman */
    CTRY_PAKISTAN             = 586,     /* Islamic Republic of Pakistan */
    CTRY_PANAMA               = 591,     /* Panama */
    CTRY_PARAGUAY             = 600,     /* Paraguay */
    CTRY_PERU                 = 604,     /* Peru */
    CTRY_PHILIPPINES          = 608,     /* Republic of the Philippines */
    CTRY_POLAND               = 616,     /* Poland */
    CTRY_PORTUGAL             = 620,     /* Portugal */
    CTRY_PUERTO_RICO          = 630,     /* Puerto Rico */
    CTRY_QATAR                = 634,     /* Qatar */
    CTRY_ROMANIA              = 642,     /* Romania */
    CTRY_RUSSIA               = 643,     /* Russia */
    CTRY_SAUDI_ARABIA         = 682,     /* Saudi Arabia */
    CTRY_SINGAPORE            = 702,     /* Singapore */
    CTRY_SLOVAKIA             = 703,     /* Slovak Republic */
    CTRY_SLOVENIA             = 705,     /* Slovenia */
    CTRY_SOUTH_AFRICA         = 710,     /* South Africa */
    CTRY_SPAIN                = 724,     /* Spain */
    CTRY_SWEDEN               = 752,     /* Sweden */
    CTRY_SWITZERLAND          = 756,     /* Switzerland */
    CTRY_SYRIA                = 760,     /* Syria */
    CTRY_TAIWAN               = 158,     /* Taiwan */
    CTRY_THAILAND             = 764,     /* Thailand */
    CTRY_TRINIDAD_Y_TOBAGO    = 780,     /* Trinidad y Tobago */
    CTRY_TUNISIA              = 788,     /* Tunisia */
    CTRY_TURKEY               = 792,     /* Turkey */
    CTRY_UAE                  = 784,     /* U.A.E. */
    CTRY_UKRAINE              = 804,     /* Ukraine */
    CTRY_UNITED_KINGDOM       = 826,     /* United Kingdom */
    CTRY_UNITED_STATES        = 840,     /* United States */
    CTRY_UNITED_STATES_FCC49  = 842,     /* United States (Public Safety)*/
    CTRY_URUGUAY              = 858,     /* Uruguay */
    CTRY_UZBEKISTAN           = 860,     /* Uzbekistan */
    CTRY_VENEZUELA            = 862,     /* Venezuela */
    CTRY_VIET_NAM             = 704,     /* Viet Nam */
    CTRY_YEMEN                = 887,     /* Yemen */
    CTRY_ZIMBABWE             = 716      /* Zimbabwe */
};


/* Mask to check whether a domain is a multidomain or a single
   domain */

#define MULTI_DOMAIN_MASK 0xFF00

/* Enumerated Regulatory Domain Information 8 bit values indicate that
 * the regdomain is really a pair of unitary regdomains.  12 bit values
 * are the real unitary regdomains and are the only ones which have the
 * frequency bitmasks and flags set.
 */

enum EnumRd {
	/*
	 * The following regulatory domain definitions are
	 * found in the EEPROM. Each regulatory domain
	 * can operate in either a 5GHz or 2.4GHz wireless mode or
	 * both 5GHz and 2.4GHz wireless modes.
	 * In general, the value holds no special
	 * meaning and is used to decode into either specific
	 * 2.4GHz or 5GHz wireless mode for that particular
	 * regulatory domain.
	 */
	NO_ENUMRD	= 0x00,
	NULL1_WORLD	= 0x03,		/* For 11b-only countries (no 11a allowed) */
	NULL1_ETSIB	= 0x07,		/* Israel */
	NULL1_ETSIC	= 0x08,
	FCC1_FCCA	= 0x10,		/* USA */
	FCC1_WORLD	= 0x11,		/* Hong Kong */
	FCC4_FCCA	= 0x12,		/* USA - Public Safety */

	FCC2_FCCA	= 0x20,		/* Canada */
	FCC2_WORLD	= 0x21,		/* Australia & HK */
	FCC2_ETSIC	= 0x22,
	FRANCE_RES	= 0x31,		/* Legacy France for OEM */
	FCC3_FCCA	= 0x3A,		/* USA & Canada w/5470 band, 11h, DFS enabled */

	ETSI1_WORLD	= 0x37,
	ETSI3_ETSIA	= 0x32,		/* France (optional) */
	ETSI2_WORLD	= 0x35,		/* Hungary & others */
	ETSI3_WORLD	= 0x36,		/* France & others */
	ETSI4_WORLD	= 0x30,
	ETSI4_ETSIC	= 0x38,
	ETSI5_WORLD	= 0x39,
	ETSI6_WORLD	= 0x34,		/* Bulgaria */
	ETSI_RESERVED	= 0x33,		/* Reserved (Do not used) */

	MKK1_MKKA	= 0x40,		/* Japan (JP1) */
	MKK1_MKKB	= 0x41,		/* Japan (JP0) */
	APL4_WORLD	= 0x42,		/* Singapore */
	MKK2_MKKA	= 0x43,		/* Japan with 4.9G channels */
	APL_RESERVED	= 0x44,		/* Reserved (Do not used)  */
	APL2_WORLD	= 0x45,		/* Korea */
	APL2_APLC	= 0x46,
	APL3_WORLD	= 0x47,
	MKK1_FCCA	= 0x48,		/* Japan (JP1-1) */
	APL2_APLD	= 0x49,		/* Korea with 2.3G channels */
	MKK1_MKKA1	= 0x4A,		/* Japan (JE1) */
	MKK1_MKKA2	= 0x4B,		/* Japan (JE2) */
	MKK1_MKKC	= 0x4F,		/* Japan (MKK1_MKKA,except Ch14) */  /* XXX - KCYU -need to resolve with MKK3_MKKC */
	MKK3_MKKA	= 0x4C,		/* Japan */
	MKK4_MKKA	= 0x4D,		/* Japan */
	MKK5_MKKA	= 0x4E,		/* Japan */

	APL1_WORLD	= 0x52,		/* Latin America */
	APL1_FCCA	= 0x53,
	APL1_APLA	= 0x54,
	APL1_ETSIC	= 0x55,
	APL2_ETSIC	= 0x56,		/* Venezuela */
	APL5_WORLD	= 0x58,		/* Chile */
	APL6_WORLD	= 0x5B,		/* Singapore */

	/*
	 * World mode SKUs
	 */
	WOR0_WORLD	= 0x60,		/* World0 (WO0 SKU) */
	WOR1_WORLD	= 0x61,		/* World1 (WO1 SKU) */
	WOR2_WORLD	= 0x62,		/* World2 (WO2 SKU) */
	WOR3_WORLD	= 0x63,		/* World3 (WO3 SKU) */
	WOR4_WORLD	= 0x64,		/* World4 (WO4 SKU) */	
	WOR5_ETSIC	= 0x65,		/* World5 (WO5 SKU) */    

	WOR01_WORLD	= 0x66,		/* World0-1 (WW0-1 SKU) */
	WOR02_WORLD	= 0x67,		/* World0-2 (WW0-2 SKU) */
	EU1_WORLD	= 0x68,		/* Same as World0-2 (WW0-2 SKU), except active scan ch1-13. No ch14 */

	WOR9_WORLD	= 0x69,		/* World9 (WO9 SKU) */	
	WORA_WORLD	= 0x6A,		/* WorldA (WOA SKU) */	

	/*
	 * Regulator domains ending in a number (e.g. APL1,
	 * MK1, ETSI4, etc) apply to 5GHz channel and power
	 * information.  Regulator domains ending in a letter
	 * (e.g. APLA, FCCA, etc) apply to 2.4GHz channel and
	 * power information.
	 */
	APL1		= 0x0150,	/* LAT & Asia */
	APL2		= 0x0250,	/* LAT & Asia */
	APL3		= 0x0350,	/* Taiwan */
	APL4		= 0x0450,	/* Jordan */
	APL5		= 0x0550,	/* Chile */
	APL6		= 0x0650,	/* Singapore */

	ETSI1		= 0x0130,	/* Europe & others */
	ETSI2		= 0x0230,	/* Europe & others */
	ETSI3		= 0x0330,	/* Europe & others */
	ETSI4		= 0x0430,	/* Europe & others */
	ETSI5		= 0x0530,	/* Europe & others */
	ETSI6		= 0x0630,	/* Europe & others */
	ETSIA		= 0x0A30,	/* France */
	ETSIB		= 0x0B30,	/* Israel */
	ETSIC		= 0x0C30,	/* Latin America */

	FCC1		= 0x0110,	/* US & others */
	FCC2		= 0x0120,	/* Canada, Australia & New Zealand */
	FCC3		= 0x0160,	/* US w/new middle band & DFS */    
	FCC4          	= 0x0165,     	/* US Public Safety */
	FCCA		= 0x0A10,	 

	APLD		= 0x0D50,	/* South Korea */

	MKK1		= 0x0140,	/* Japan */
	MKK2		= 0x0240,	/* Japan Extended */
	MKK3		= 0x0340,	/* Japan new 5GHz */
	MKK4		= 0x0440,	/* Japan new 5GHz */
	MKK5		= 0x0540,	/* Japan new 5GHz */

	MKKA		= 0x0A40,	/* Japan */
	MKKC		= 0x0A50,

	NULL1		= 0x0198,
	WORLD		= 0x0199,
	DEBUG_REG_DMN	= 0x01ff,
};

/* TODO: should be in a shared header file */
/* conformance test limits */
enum {
    /* test groups */
	FCC	       = 0x10,
	MKK	       = 0x40,
	ETSI	   = 0x30,
    SD_NO_CTL  = 0xe0,
	NO_CTL	   = 0xff,
    /* test modes */
    CTL_MODE_M = 0x0f,
    CTL_11A    = 0,
	CTL_11B	   = 1,
	CTL_11G	   = 2,
	CTL_TURBO  = 3,
	CTL_108G   = 4,
    CTL_2GHT20 = 5,
    CTL_5GHT20 = 6,
    CTL_2GHT40 = 7,
    CTL_5GHT40 = 8
};

/*
 * The following are flags for different requirements per reg domain.
 * These requirements are either inhereted from the reg domain pair or
 * from the unitary reg domain if the reg domain pair flags value is
 * 0
 */

enum {
	NO_REQ			= 0x00000000,
	DISALLOW_ADHOC_11A	= 0x00000001,
	DISALLOW_ADHOC_11A_TURB	= 0x00000002,
	NEED_NFC		= 0x00000004,

	ADHOC_PER_11D		= 0x00000008,  /* Start Ad-Hoc mode */
	ADHOC_NO_11A		= 0x00000010,

	PUBLIC_SAFETY_DOMAIN	= 0x00000020, 	/* public safety domain */
};

/*
 * The following describe the bit masks for different passive scan
 * capability/requirements per regdomain.
 */
#define	NO_PSCAN	0x0ULL
#define	PSCAN_FCC	0x0000000000000001ULL
#define	PSCAN_FCC_T	0x0000000000000002ULL
#define	PSCAN_ETSI	0x0000000000000004ULL
#define	PSCAN_MKK1	0x0000000000000008ULL
#define	PSCAN_MKK2	0x0000000000000010ULL
#define	PSCAN_MKKA	0x0000000000000020ULL
#define	PSCAN_MKKA_G	0x0000000000000040ULL
#define	PSCAN_ETSIA	0x0000000000000080ULL
#define	PSCAN_ETSIB	0x0000000000000100ULL
#define	PSCAN_ETSIC	0x0000000000000200ULL
#define	PSCAN_WWR	0x0000000000000400ULL
#define	PSCAN_MKKA1	0x0000000000000800ULL
#define	PSCAN_MKKA1_G	0x0000000000001000ULL
#define	PSCAN_MKKA2	0x0000000000002000ULL
#define	PSCAN_MKKA2_G	0x0000000000004000ULL
#define	PSCAN_MKK3	0x0000000000008000ULL
#define	PSCAN_DEFER	0x7FFFFFFFFFFFFFFFULL
#define	IS_ECM_CHAN	0x8000000000000000ULL

/*
 * THE following table is the mapping of regdomain pairs specified by
 * an 8 bit regdomain value to the individual unitary reg domains
 */

typedef struct reg_dmn_pair_mapping {
	HAL_REG_DOMAIN regDmnEnum;	/* 16 bit reg domain pair */
	HAL_REG_DOMAIN regDmn5GHz;	/* 5GHz reg domain */
	HAL_REG_DOMAIN regDmn2GHz;	/* 2GHz reg domain */
	u_int32_t flags5GHz;		/* Requirements flags (AdHoc
					   disallow, noise floor cal needed,
					   etc) */
	u_int32_t flags2GHz;		/* Requirements flags (AdHoc
					   disallow, noise floor cal needed,
					   etc) */
	u_int64_t pscanMask;		/* Passive Scan flags which
					   can override unitary domain
					   passive scan flags.  This
					   value is used as a mask on
					   the unitary flags*/
	u_int16_t singleCC;		/* Country code of single country if
					   a one-on-one mapping exists */
}  REG_DMN_PAIR_MAPPING;

static REG_DMN_PAIR_MAPPING regDomainPairs[] = {
	{NO_ENUMRD,	DEBUG_REG_DMN,	DEBUG_REG_DMN, NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{NULL1_WORLD,	NULL1,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{NULL1_ETSIB,	NULL1,		ETSIB,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{NULL1_ETSIC,	NULL1,		ETSIC,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },

	{FCC2_FCCA,	FCC2,		FCCA,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{FCC2_WORLD,	FCC2,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{FCC2_ETSIC,	FCC2,		ETSIC,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{FCC3_FCCA,	FCC3,		FCCA,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{FCC4_FCCA,	FCC4,		FCCA,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },

	{ETSI1_WORLD,	ETSI1,		WORLD,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{ETSI2_WORLD,	ETSI2,		WORLD,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{ETSI3_WORLD,	ETSI3,		WORLD,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{ETSI4_WORLD,	ETSI4,		WORLD,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{ETSI5_WORLD,	ETSI5,		WORLD,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{ETSI6_WORLD,	ETSI6,		WORLD,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },

	{ETSI3_ETSIA,	ETSI3,		WORLD,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{FRANCE_RES,	ETSI3,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },

	{FCC1_WORLD,	FCC1,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{FCC1_FCCA,	FCC1,		FCCA,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL1_WORLD,	APL1,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL2_WORLD,	APL2,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL3_WORLD,	APL3,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL4_WORLD,	APL4,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL5_WORLD,	APL5,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL6_WORLD,	APL6,		WORLD,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },

	{APL1_ETSIC,	APL1,		ETSIC,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL2_ETSIC,	APL2,		ETSIC,		NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{APL2_APLD,	APL2,		APLD,		NO_REQ, NO_REQ, PSCAN_DEFER,  },

	{MKK1_MKKA,	MKK1,		MKKA,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, NO_PSCAN, CTRY_JAPAN },
	{MKK1_MKKB,	MKK1,		MKKA,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, PSCAN_MKK1 | PSCAN_MKKA | PSCAN_MKKA_G, CTRY_JAPAN1 },
	{MKK2_MKKA,	MKK2,		MKKA,		DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, PSCAN_MKK2 | PSCAN_MKKA | PSCAN_MKKA_G, CTRY_JAPAN3 },
	{MKK1_FCCA,	MKK1,		FCCA,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, NO_PSCAN, CTRY_JAPAN2 },
	{MKK1_MKKA1,	MKK1,		MKKA,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, PSCAN_MKKA1 | PSCAN_MKKA1_G, CTRY_JAPAN4 },
	{MKK1_MKKA2,	MKK1,		MKKA,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, PSCAN_MKKA2 | PSCAN_MKKA2_G, CTRY_JAPAN5 },
	{MKK1_MKKC,	MKK1,		MKKC,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, NO_PSCAN, CTRY_JAPAN6 },
	{MKK3_MKKA,	MKK3,		MKKA,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, PSCAN_DEFER, 0 },
	{MKK4_MKKA,	MKK4,		MKKA,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, PSCAN_DEFER, 0 },
	{MKK5_MKKA,	MKK5,		MKKA,		DISALLOW_ADHOC_11A_TURB | NEED_NFC, NEED_NFC, PSCAN_DEFER, 0 },

		/* These are super domains */
	{WOR0_WORLD,	WOR0_WORLD,	WOR0_WORLD,	NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{WOR1_WORLD,	WOR1_WORLD,	WOR1_WORLD,	DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{WOR2_WORLD,	WOR2_WORLD,	WOR2_WORLD,	DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{WOR3_WORLD,	WOR3_WORLD,	WOR3_WORLD,	NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{WOR4_WORLD,	WOR4_WORLD,	WOR4_WORLD,	DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{WOR5_ETSIC,	WOR5_ETSIC,	WOR5_ETSIC,	DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{WOR01_WORLD,	WOR01_WORLD,	WOR01_WORLD,	NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{WOR02_WORLD,	WOR02_WORLD,	WOR02_WORLD,	NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{EU1_WORLD,	EU1_WORLD,	EU1_WORLD,	NO_REQ, NO_REQ, PSCAN_DEFER, 0 },
	{WOR9_WORLD,	WOR9_WORLD,	WOR9_WORLD,	DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
	{WORA_WORLD,	WORA_WORLD,	WORA_WORLD,	DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB, NO_REQ, PSCAN_DEFER, 0 },
};
		
/* 
 * The following table is the master list for all different freqeuncy
 * bands with the complete matrix of all possible flags and settings
 * for each band if it is used in ANY reg domain.
 */

#define DEF_REGDMN		FCC1_FCCA
#define	DEF_DMN_5		FCC1
#define	DEF_DMN_2		FCCA
#define	COUNTRY_ERD_FLAG        0x8000
#define WORLDWIDE_ROAMING_FLAG  0x4000
#define	SUPER_DOMAIN_MASK	0x0fff
#define	COUNTRY_CODE_MASK	0x03ff
#define CF_INTERFERENCE		(CHANNEL_CW_INT | CHANNEL_RADAR_INT)
#define CHANNEL_14		(2484)	/* 802.11g operation is not permitted on channel 14 */
#define IS_11G_CH14(_ch,_cf) \
	(((_ch) == CHANNEL_14) && ((_cf) == CHANNEL_G))

#define	YES	AH_TRUE
#define	NO	AH_FALSE

typedef struct {
	HAL_CTRY_CODE		countryCode;	   
	HAL_REG_DOMAIN		regDmnEnum;
	const char*		isoName;
	const char*		name;
	HAL_BOOL		allow11g;
	HAL_BOOL		allow11aTurbo;  
	HAL_BOOL		allow11gTurbo;
    HAL_BOOL        allow11na;      /* HT-40 allowed in 5GHz? */
    HAL_BOOL        allow11ng;      /* HT-40 allowed in 2GHz? */
	u_int16_t		outdoorChanStart;
} COUNTRY_CODE_TO_ENUM_RD;

static COUNTRY_CODE_TO_ENUM_RD allCountries[] = {
    {CTRY_DEBUG,       NO_ENUMRD,     "DB", "DEBUG",          YES, YES, YES, YES, YES, 7000 },
    {CTRY_DEFAULT,     DEF_REGDMN,    "NA", "NO_COUNTRY_SET", YES, YES, YES, YES, YES, 7000 },
    {CTRY_ALBANIA,     NULL1_WORLD,   "AL", "ALBANIA",        YES, NO,  YES, NO, YES, 7000 },
    {CTRY_ALGERIA,     NULL1_WORLD,   "DZ", "ALGERIA",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_ARGENTINA,   APL3_WORLD,    "AR", "ARGENTINA",      NO,  NO,  NO,  NO,  NO,  7000 },
    {CTRY_ARMENIA,     ETSI4_WORLD,   "AM", "ARMENIA",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_AUSTRALIA,   FCC2_WORLD,    "AU", "AUSTRALIA",      YES, YES, YES, YES, YES, 7000 },
    {CTRY_AUSTRIA,     ETSI5_WORLD,   "AT", "AUSTRIA",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_AZERBAIJAN,  ETSI4_WORLD,   "AZ", "AZERBAIJAN",     YES, YES, YES, YES, YES, 7000 },
    {CTRY_BAHRAIN,     NULL1_WORLD,   "BH", "BAHRAIN",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_BELARUS,     NULL1_WORLD,   "BY", "BELARUS",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_BELGIUM,     ETSI4_WORLD,   "BE", "BELGIUM",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_BELIZE,      APL1_ETSIC,    "BZ", "BELIZE",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_BOLIVIA,     APL1_ETSIC,    "BO", "BOLVIA",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_BRAZIL,      NULL1_ETSIC,   "BR", "BRAZIL",         NO,  NO,  NO,  NO,  NO,  7000 },
    {CTRY_BRUNEI_DARUSSALAM,APL1_WORLD,"BN", "BRUNEI DARUSSALAM", YES, YES, YES,  YES, YES, 7000 },
    {CTRY_BULGARIA,    ETSI6_WORLD,   "BG", "BULGARIA",       YES, NO,  YES, YES, YES, 7000 },
    {CTRY_CANADA,      FCC2_FCCA,     "CA", "CANADA",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_CHILE,       APL5_WORLD,    "CL", "CHILE",          NO,  YES, NO,  YES, YES, 7000 },
    {CTRY_CHINA,       APL1_WORLD,    "CN", "CHINA",          YES, YES, YES, YES, YES, 7000 },
    {CTRY_COLOMBIA,    FCC1_FCCA,     "CO", "COLOMBIA",       YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_COSTA_RICA,  NULL1_WORLD,   "CR", "COSTA RICA",     YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_CROATIA,     ETSI3_WORLD,   "HR", "CROATIA",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_CYPRUS,      ETSI3_WORLD,   "CY", "CYPRUS",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_CZECH,       ETSI3_WORLD,   "CZ", "CZECH REPUBLIC", NO,  NO,  NO,  YES, YES, 7000 },
    {CTRY_DENMARK,     ETSI1_WORLD,   "DK", "DENMARK",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_DOMINICAN_REPUBLIC,FCC1_FCCA,"DO", "DOMINICAN REPUBLIC", YES, YES, YES, YES, YES, 7000 },
    {CTRY_ECUADOR,     NULL1_WORLD,   "EC", "ECUADOR",        NO,  NO,  NO,  NO,  NO,  7000 },
    {CTRY_EGYPT,       ETSI3_WORLD,   "EG", "EGYPT",          YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_EL_SALVADOR, NULL1_WORLD,   "SV", "EL SALVADOR",    YES, NO,  YES, NO,  YES, 7000 },    
    {CTRY_ESTONIA,     ETSI1_WORLD,   "EE", "ESTONIA",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_FINLAND,     ETSI1_WORLD,   "FI", "FINLAND",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_FRANCE,      ETSI3_WORLD,   "FR", "FRANCE",         YES, NO,  YES, YES, YES, 7000 },
    {CTRY_FRANCE2,     ETSI3_WORLD,   "F2", "FRANCE_RES",     YES, NO,  YES, NO,  NO,  7000 },
    {CTRY_GEORGIA,     ETSI4_WORLD,   "GE", "GEORGIA",        YES, YES, YES, YES, YES, 7000 },
    {CTRY_GERMANY,     ETSI1_WORLD,   "DE", "GERMANY",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_GREECE,      NULL1_WORLD,   "GR", "GREECE",         YES, NO,  YES, YES, YES, 7000 },
    {CTRY_GUATEMALA,   FCC1_FCCA,     "GT", "GUATEMALA",      YES, YES, YES, YES, YES, 7000 },
    {CTRY_HONDURAS,    NULL1_WORLD,   "HN", "HONDURAS",       YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_HONG_KONG,   FCC2_WORLD,    "HK", "HONG KONG",      YES, YES, YES, YES, YES, 7000 },
    {CTRY_HUNGARY,     ETSI4_WORLD,   "HU", "HUNGARY",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_ICELAND,     ETSI1_WORLD,   "IS", "ICELAND",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_INDIA,       NULL1_WORLD,   "IN", "INDIA",          YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_INDONESIA,   APL1_WORLD,    "ID", "INDONESIA",      YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_IRAN,        APL1_WORLD,    "IR", "IRAN",           YES, YES, YES, YES, YES, 7000 },
    {CTRY_IRELAND,     ETSI1_WORLD,   "IE", "IRELAND",        YES, NO,  YES, YES, YES, 7000 },
    {CTRY_ISRAEL,      NULL1_WORLD,   "IL", "ISRAEL",         YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_ITALY,       ETSI1_WORLD,   "IT", "ITALY",          YES, NO,  YES, YES, YES, 7000 },
    {CTRY_JAPAN,       MKK1_MKKA,     "JP", "JAPAN",          YES, NO,  NO,  NO,  NO,  7000 },
    {CTRY_JAPAN1,      MKK1_MKKB,     "J1", "JAPAN1",         YES, NO,  NO,  NO,  NO,  7000 },
    {CTRY_JAPAN2,      MKK1_FCCA,     "J2", "JAPAN2",         YES, NO,  NO,  NO,  NO,  7000 },    
    {CTRY_JAPAN3,      MKK2_MKKA,     "J3", "JAPAN3",         YES, NO,  NO,  NO,  NO,  7000 },
    {CTRY_JAPAN4,      MKK1_MKKA1,    "J4", "JAPAN4",         YES, NO,  NO,  NO,  NO,  7000 },
    {CTRY_JAPAN5,      MKK1_MKKA2,    "J5", "JAPAN5",         YES, NO,  NO,  NO,  NO,  7000 },    
    {CTRY_JAPAN6,      MKK1_MKKC,     "J6", "JAPAN6",         YES, NO,  NO,  NO,  NO,  7000 },    
    {CTRY_JORDAN,      APL4_WORLD,    "JO", "JORDAN",         YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_KAZAKHSTAN,  NULL1_WORLD,   "KZ", "KAZAKHSTAN",     YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_KOREA_NORTH, APL2_WORLD,    "KP", "NORTH KOREA",    YES, YES, YES, YES, YES, 7000 },
    {CTRY_KOREA_ROC,   APL2_WORLD,    "KR", "KOREA REPUBLIC", YES, NO,  YES, NO,  NO,  7000 },
    {CTRY_KOREA_ROC2,  APL2_APLD,     "K2", "KOREA REPUBLIC2",YES, NO,  YES, NO,  NO,  7000 },
    {CTRY_KOREA_ROC3,  APL6_WORLD,    "K3", "KOREA REPUBLIC3",YES, NO,  YES, NO,  NO,  7000 },
    {CTRY_KUWAIT,      NULL1_WORLD,   "KW", "KUWAIT",         YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_LATVIA,      ETSI1_WORLD,   "LV", "LATVIA",         YES, NO,  YES, YES, YES, 7000 },
    {CTRY_LEBANON,     NULL1_WORLD,   "LB", "LEBANON",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_LIECHTENSTEIN,ETSI4_WORLD,  "LI", "LIECHTENSTEIN",  YES, NO,  YES, YES, YES, 7000 },
    {CTRY_LITHUANIA,   ETSI1_WORLD,   "LT", "LITHUANIA",      YES, NO,  YES, YES, YES, 7000 },
    {CTRY_LUXEMBOURG,  ETSI1_WORLD,   "LU", "LUXEMBOURG",     YES, NO,  YES, YES, YES, 7000 },
    {CTRY_MACAU,       FCC2_WORLD,    "MO", "MACAU",          YES, YES, YES, YES, YES, 7000 },
    {CTRY_MACEDONIA,   NULL1_WORLD,   "MK", "MACEDONIA",      YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_MALAYSIA,    NULL1_WORLD,   "MY", "MALAYSIA",       NO,  NO,  NO,  NO,  NO,  7000 },
    {CTRY_MALTA,       ETSI1_WORLD,   "MT", "MALTA",          YES, NO,  YES, YES, YES, 7000 },
    {CTRY_MEXICO,      FCC1_FCCA,     "MX", "MEXICO",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_MONACO,      ETSI4_WORLD,   "MC", "MONACO",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_MOROCCO,     NULL1_WORLD,   "MA", "MOROCCO",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_NETHERLANDS, ETSI1_WORLD,   "NL", "NETHERLANDS",    YES, NO,  YES, YES, YES, 7000 },
    {CTRY_NEW_ZEALAND, FCC2_ETSIC,    "NZ", "NEW ZEALAND",    YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_NORWAY,      ETSI1_WORLD,   "NO", "NORWAY",         YES, NO,  YES, YES, YES, 7000 },
    {CTRY_OMAN,        ETSI3_WORLD,   "OM", "OMAN",           YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_PAKISTAN,    NULL1_WORLD,   "PK", "PAKISTAN",       YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_PANAMA,      FCC1_FCCA,     "PA", "PANAMA",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_PERU,        APL1_WORLD,    "PE", "PERU",           YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_PHILIPPINES, APL1_WORLD,    "PH", "PHILIPPINES",    YES, YES, YES, YES, YES, 7000 },
    {CTRY_POLAND,      ETSI1_WORLD,   "PL", "POLAND",         YES, NO,  YES, YES, YES, 7000 },
    {CTRY_PORTUGAL,    ETSI1_WORLD,   "PT", "PORTUGAL",       YES, NO,  YES, YES, YES, 7000 },
    {CTRY_PUERTO_RICO, FCC1_FCCA,     "PR", "PUERTO RICO",    YES, YES, YES, YES, YES, 7000 },
    {CTRY_QATAR,       NULL1_WORLD,   "QA", "QATAR",          YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_ROMANIA,     NULL1_WORLD,   "RO", "ROMANIA",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_RUSSIA,      NULL1_WORLD,   "RU", "RUSSIA",         YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_SAUDI_ARABIA,NULL1_WORLD,   "SA", "SAUDI ARABIA",   YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_SINGAPORE,   APL6_WORLD,    "SG", "SINGAPORE",      YES, YES, YES, YES, YES, 7000 },
    {CTRY_SLOVAKIA,    ETSI3_WORLD,   "SK", "SLOVAK REPUBLIC",YES, NO,  YES, YES, YES, 7000 },
    {CTRY_SLOVENIA,    ETSI1_WORLD,   "SI", "SLOVENIA",       YES, NO,  YES, YES, YES, 7000 },
    {CTRY_SOUTH_AFRICA,ETSI1_WORLD,   "ZA", "SOUTH AFRICA",   YES, YES, YES, NO,  YES, 7000 },
    {CTRY_SPAIN,       ETSI1_WORLD,   "ES", "SPAIN",          YES, NO,  YES, YES, YES, 7000 },
    {CTRY_SWEDEN,      ETSI1_WORLD,   "SE", "SWEDEN",         YES, NO,  YES, YES, YES, 7000 },
    {CTRY_SWITZERLAND, ETSI4_WORLD,   "CH", "SWITZERLAND",    YES, NO,  YES, YES, YES, 7000 },
    {CTRY_SYRIA,       NULL1_WORLD,   "SY", "SYRIA",          YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_TAIWAN,      APL3_WORLD,    "TW", "TAIWAN",         YES, YES, YES, YES, YES, 7000 },
    {CTRY_THAILAND,    APL2_WORLD,    "TH", "THAILAND",       YES, YES, YES, NO,  YES, 7000 },
    {CTRY_TRINIDAD_Y_TOBAGO,ETSI4_WORLD,"TT", "TRINIDAD & TOBAGO", YES, NO, YES, NO, YES, 7000 },
    {CTRY_TUNISIA,     ETSI3_WORLD,   "TN", "TUNISIA",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_TURKEY,      ETSI3_WORLD,   "TR", "TURKEY",         YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_UKRAINE,     NULL1_WORLD,   "UA", "UKRAINE",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_UAE,         NULL1_WORLD,   "AE", "UNITED ARAB EMIRATES", YES, NO, YES, NO, YES, 7000 },
    {CTRY_UNITED_KINGDOM, ETSI1_WORLD,"GB", "UNITED KINGDOM", YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_UNITED_STATES, FCC1_FCCA,   "US", "UNITED STATES",  YES, YES, YES, YES, YES, 5825 },
    {CTRY_UNITED_STATES_FCC49, FCC4_FCCA,   "PS", "UNITED STATES (PUBLIC SAFETY)",  YES, YES, YES, YES, YES, 7000 },
    {CTRY_URUGUAY,     APL2_WORLD,    "UY", "URUGUAY",        YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_UZBEKISTAN,  FCC3_FCCA,     "UZ", "UZBEKISTAN",     YES, YES, YES, YES, YES, 7000 },    
    {CTRY_VENEZUELA,   APL2_ETSIC,    "VE", "VENEZUELA",      YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_VIET_NAM,    NULL1_WORLD,   "VN", "VIET NAM",       YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_YEMEN,       NULL1_WORLD,   "YE", "YEMEN",          YES, NO,  YES, NO,  YES, 7000 },
    {CTRY_ZIMBABWE,    NULL1_WORLD,   "ZW", "ZIMBABWE",       YES, NO,  YES, NO,  YES, 7000 }    
};

typedef struct RegDmnFreqBand {
	u_int16_t	lowChannel;	/* Low channel center in MHz */
	u_int16_t	highChannel;	/* High Channel center in MHz */
	u_int8_t	powerDfs;	/* Max power (dBm) for channel
					   range when using DFS */
	u_int8_t	antennaMax;	/* Max allowed antenna gain */
	u_int8_t	channelBW;	/* Bandwidth of the channel */
	u_int8_t	channelSep;	/* Channel separation within
					   the band */
	u_int64_t	useDfs;		/* Use DFS in the RegDomain
					   if corresponding bit is set */
	u_int64_t	usePassScan;	/* Use Passive Scan in the RegDomain
					   if corresponding bit is set */
	u_int8_t	regClassId;	/* Regulatory class id */
} REG_DMN_FREQ_BAND;

/* Bit masks for DFS per regdomain */

enum {
	NO_DFS   = 0x00000000,
	DFS_FCC3 = 0x00000001,
	DFS_ETSI = 0x00000002,
	DFS_MKK3 = 0x00000004,
};

/* The table of frequency bands is indexed by a bitmask.  The ordering
 * must be consistent with the enum below.  When adding a new
 * frequency band, be sure to match the location in the enum with the
 * comments 
 */

/*
 * 5GHz 11A channel tags
 */

enum {
	F1_4915_4925,
	F1_4935_4945,
	F1_4920_4980,
	F1_4942_4987,
	F1_4945_4985,
	F1_4950_4980,
	F1_5035_5040,
	F1_5040_5080,
	F1_5055_5055,

	F1_5120_5240,

	F1_5170_5230,

	F1_5180_5240,
	F2_5180_5240,
	F3_5180_5240,
	F4_5180_5240,
	F5_5180_5240,
	F6_5180_5240,

	F1_5240_5280,

	F1_5260_5280,

	F1_5260_5320,
	F2_5260_5320,
	F3_5260_5320,
	F4_5260_5320,
	F5_5260_5320,

	F1_5260_5700,

	F1_5280_5320,

	F1_5500_5700,
	F2_5500_5700,
	F3_5500_5700,
	F4_5500_5700,

	F1_5745_5805,
	F2_5745_5805,

	F1_5745_5825,
	F2_5745_5825,
	F3_5745_5825,
	F4_5745_5825,
	F5_5745_5825,
	F6_5745_5825,

	W1_4920_4980,
	W1_5040_5080,
	W1_5170_5230,
	W1_5180_5240,
	W1_5260_5320,
	W1_5745_5825,
	W1_5500_5700,
};

static REG_DMN_FREQ_BAND regDmn5GhzFreq[] = {
	{ 4915, 4925, 23, 0, 10, 5, NO_DFS, PSCAN_MKK2, 16 },				/* F1_4915_4925 */
	{ 4935, 4945, 23, 0, 10, 5, NO_DFS, PSCAN_MKK2, 16 },				/* F1_4935_4945 */
	{ 4920, 4980, 23, 0, 20, 20, NO_DFS, PSCAN_MKK2, 7 },				/* F1_4920_4980 */
	{ 4942, 4987, 27, 9, 5, 5, NO_DFS, PSCAN_FCC, 0 },				/* F1_4942_4987 */
	{ 4945, 4985, 30, 9, 10, 5, NO_DFS, PSCAN_FCC, 0 },				/* F1_4945_4985 */
	{ 4950, 4980, 30, 9, 20, 5, NO_DFS, PSCAN_FCC, 0 },				/* F1_4950_4980 */
	{ 5035, 5040, 23, 0, 10, 5, NO_DFS, PSCAN_MKK2, 12 },				/* F1_5035_5040 */
	{ 5040, 5080, 23, 0, 20, 20, NO_DFS, PSCAN_MKK2, 2 },				/* F1_5040_5080 */
	{ 5055, 5055, 23, 0, 10, 5, NO_DFS, PSCAN_MKK2, 12 },				/* F1_5055_5055 */

	{ 5120, 5240, 5,  6, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F1_5120_5240 */

	{ 5170, 5230, 23, 0, 20, 20, NO_DFS, PSCAN_MKK1 | PSCAN_MKK2, 1 },		/* F1_5170_5230 */

	{ 5180, 5240, 15, 0, 20, 20, NO_DFS, PSCAN_FCC | PSCAN_ETSI, 0 },		/* F1_5180_5240 */
	{ 5180, 5240, 17, 6, 20, 20, NO_DFS, PSCAN_FCC, 1 },				/* F2_5180_5240 */
	{ 5180, 5240, 18, 0, 20, 20, NO_DFS, PSCAN_FCC | PSCAN_ETSI, 0 },		/* F3_5180_5240 */
	{ 5180, 5240, 20, 0, 20, 20, NO_DFS, PSCAN_FCC | PSCAN_ETSI, 0 },		/* F4_5180_5240 */
	{ 5180, 5240, 23, 0, 20, 20, NO_DFS, PSCAN_FCC | PSCAN_ETSI, 0 },		/* F5_5180_5240 */
	{ 5180, 5240, 23, 6, 20, 20, NO_DFS, PSCAN_FCC, 0 },				/* F6_5180_5240 */

	{ 5240, 5280, 23, 0, 20, 20, DFS_FCC3, PSCAN_FCC | PSCAN_ETSI, 0 },		/* F1_5240_5280 */

	{ 5260, 5280, 23, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC | PSCAN_ETSI, 0 },	/* F1_5260_5280 */

	{ 5260, 5320, 18, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC | PSCAN_ETSI, 0 },	/* F1_5260_5320 */

	{ 5260, 5320, 20, 0, 20, 20, DFS_FCC3 | DFS_ETSI | DFS_MKK3, PSCAN_FCC | PSCAN_ETSI | PSCAN_MKK3, 0 },
											/* F2_5260_5320 */

	{ 5260, 5320, 20, 6, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC, 2 },		/* F3_5260_5320 */
	{ 5260, 5320, 23, 6, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC, 2 },		/* F4_5260_5320 */
	{ 5260, 5320, 23, 6, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC, 0 },		/* F5_5260_5320 */

	{ 5260, 5700, 5,  6, 20, 20, DFS_FCC3 | DFS_ETSI, NO_PSCAN, 0 },		/* F1_5260_5700 */

	{ 5280, 5320, 17, 6, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC, 0 },		/* F1_5280_5320 */

	{ 5500, 5700, 20, 6, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC, 4 },		/* F1_5500_5700 */
	{ 5500, 5700, 27, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC | PSCAN_ETSI, 0 },	/* F2_5500_5700 */
	{ 5500, 5700, 30, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_FCC | PSCAN_ETSI, 0 },	/* F3_5500_5700 */
	{ 5500, 5700, 23, 0, 20, 20, DFS_FCC3 | DFS_ETSI | DFS_MKK3, PSCAN_MKK3 | PSCAN_FCC, 0 },
											/* F4_5500_5700 */

	{ 5745, 5805, 23, 0, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F1_5745_5805 */
	{ 5745, 5805, 30, 6, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F2_5745_5805 */
	{ 5745, 5825, 5,  6, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F1_5745_5825 */
	{ 5745, 5825, 17, 0, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F2_5745_5825 */
	{ 5745, 5825, 20, 0, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F3_5745_5825 */	
	{ 5745, 5825, 30, 0, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F4_5745_5825 */
	{ 5745, 5825, 30, 6, 20, 20, NO_DFS, NO_PSCAN, 3 },				/* F5_5745_5825 */
	{ 5745, 5825, 30, 6, 20, 20, NO_DFS, NO_PSCAN, 0 },				/* F6_5745_5825 */

	/* Below are the world roaming channels */

	{ 4920, 4980, 17, 0, 20, 20, NO_DFS, PSCAN_WWR, 0 },				/* W1_4920_4980 */
	{ 5040, 5080, 17, 0, 20, 20, NO_DFS, PSCAN_WWR, 0 },				/* W1_5040_5080 */
	{ 5170, 5230, 15, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, 0 },		/* W1_5170_5230 */
	{ 5180, 5240, 15, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, 0 },		/* W1_5180_5240 */
	{ 5260, 5320, 18, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, 0 },		/* W1_5260_5320 */
	{ 5745, 5825, 20, 0, 20, 20, NO_DFS, PSCAN_WWR, 0 },				/* W1_5745_5825 */
	{ 5500, 5700, 20, 0, 20, 20, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, 0 },		/* W1_5500_5700 */
};

/*
 * 5GHz Turbo (dynamic & static) tags
 */

enum {
	T1_5130_5210,
	T1_5250_5330,
	T1_5370_5490,
	T1_5530_5650,

	T1_5150_5190,
	T1_5230_5310,
	T1_5350_5470,
	T1_5510_5670,

	T1_5200_5200,
	T1_5210_5210,
	T2_5210_5210,

	T1_5240_5280,
	T1_5250_5290,
	T2_5250_5290,

	T1_5540_5660,
	T1_5760_5800,
	T2_5760_5800,

	T1_5765_5805,

	WT1_5210_5250,
	WT1_5290_5290,
	WT1_5540_5660,
	WT1_5760_5800,
};

static REG_DMN_FREQ_BAND regDmn5GhzTurboFreq[] = {
	{ 5130, 5210, 5,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5130_5210 */
	{ 5250, 5330, 5,  6, 40, 40, DFS_FCC3, NO_PSCAN, 0},	/* T1_5250_5330 */
	{ 5370, 5490, 5,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5370_5490 */
	{ 5530, 5650, 5,  6, 40, 40, DFS_FCC3, NO_PSCAN, 0},	/* T1_5530_5650 */

	{ 5150, 5190, 5,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5150_5190 */
	{ 5230, 5310, 5,  6, 40, 40, DFS_FCC3, NO_PSCAN, 0},	/* T1_5230_5310 */
	{ 5350, 5470, 5,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5350_5470 */
	{ 5510, 5670, 5,  6, 40, 40, DFS_FCC3, NO_PSCAN, 0},	/* T1_5510_5670 */

	{ 5200, 5200, 17, 6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5200_5200 */ 
	{ 5210, 5210, 17, 6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5210_5210 */
	{ 5210, 5210, 20, 0, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T2_5210_5210 */

	{ 5240, 5280, 23, 6, 40, 40, DFS_FCC3, PSCAN_FCC_T, 0},	/* T1_5240_5280 */
	{ 5250, 5290, 20, 0, 40, 40, DFS_FCC3, PSCAN_FCC_T, 0},	/* T1_5250_5290 */
	{ 5250, 5290, 23, 6, 40, 40, DFS_FCC3, PSCAN_FCC_T, 0},	/* T2_5250_5290 */

	{ 5540, 5660, 30, 6, 40, 40, DFS_FCC3, PSCAN_FCC_T, 0},	/* T1_5540_5660 */
	{ 5760, 5800, 20, 0, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5760_5800 */
	{ 5760, 5800, 30, 6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T2_5760_5800 */

	{ 5765, 5805, 30, 6, 40, 40, NO_DFS, NO_PSCAN, 0},	/* T1_5765_5805 */

	/* Below are the WWR frequencies */
	{ 5210, 5250, 15, 0, 40, 40, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, 0}, /* WT1_5210_5250 */
	{ 5290, 5290, 18, 0, 40, 40, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, 0}, /* WT1_5290_5290 */
	{ 5540, 5660, 20, 0, 40, 40, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, 0}, /* WT1_5540_5660 */
	{ 5760, 5800, 20, 0, 40, 40, NO_DFS, PSCAN_WWR, 0},	/* WT1_5760_5800 */
};

/*
 * 2GHz 11b channel tags
 */
enum {
	F1_2312_2372,
	F2_2312_2372,

	F1_2412_2472,
	F2_2412_2472,
	F3_2412_2472,

	F1_2412_2462,
	F2_2412_2462,

	F1_2432_2442,

	F1_2457_2472,

	F1_2467_2472,

	F1_2484_2484,
	F2_2484_2484,

	F1_2512_2732,

	W1_2312_2372,
	W1_2412_2412,
	W1_2417_2432,
	W1_2437_2442,
	W1_2447_2457,
	W1_2462_2462,
	W1_2467_2467,
	W2_2467_2467,
	W1_2472_2472,
	W2_2472_2472,
	W1_2484_2484,
	W2_2484_2484,
};

static REG_DMN_FREQ_BAND regDmn2GhzFreq[] = {
	{ 2312, 2372, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F1_2312_2372 */
	{ 2312, 2372, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F2_2312_2372 */

	{ 2412, 2472, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F1_2412_2472 */
	{ 2412, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA, 0},	/* F2_2412_2472 */
	{ 2412, 2472, 30, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F3_2412_2472 */

	{ 2412, 2462, 27, 6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F1_2412_2462 */
	{ 2412, 2462, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA, 0},	/* F2_2412_2462 */
	
	{ 2432, 2442, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F1_2432_2442 */

	{ 2457, 2472, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F1_2457_2472 */

	{ 2467, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA2 | PSCAN_MKKA, 0}, /* F1_2467_2472 */

	{ 2484, 2484, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F1_2484_2484 */
	{ 2484, 2484, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA | PSCAN_MKKA1 | PSCAN_MKKA2, 0},	/* F2_2484_2484 */

	{ 2512, 2732, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* F1_2512_2732 */

	{ 2312, 2372, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* W1_2312_2372 */
	{ 2412, 2412, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* W1_2412_2412 */
	{ 2417, 2432, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* W1_2417_2432 */
	{ 2437, 2442, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* W1_2437_2442 */
	{ 2447, 2457, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* W1_2447_2457 */
	{ 2462, 2462, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* W1_2462_2462 */
	{ 2467, 2467, 18, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0}, /* W1_2467_2467 */
	{ 2467, 2467, 18, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},	/* W2_2467_2467 */
	{ 2472, 2472, 18, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0}, /* W1_2472_2472 */
	{ 2472, 2472, 18, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},	/* W2_2472_2472 */
	{ 2484, 2484, 18, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0}, /* W1_2484_2484 */
	{ 2484, 2484, 18, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},	/* W2_2484_2484 */

};

/*
 * 2GHz 11g channel tags
 */

enum {
	G1_2312_2372,
	G2_2312_2372,

	G1_2412_2472,
	G2_2412_2472,
	G3_2412_2472,

	G1_2412_2462,
	G2_2412_2462,

	G1_2432_2442,

	G1_2457_2472,

	G1_2512_2732,

	G1_2467_2472 ,

	WG1_2312_2372,
	WG1_2412_2412,
	WG1_2417_2432,
	WG1_2437_2442,
	WG1_2447_2457,
	WG1_2462_2462,
	WG1_2467_2467,
	WG2_2467_2467,
	WG1_2472_2472,
	WG2_2472_2472,

};
	
static REG_DMN_FREQ_BAND regDmn2Ghz11gFreq[] = {
	{ 2312, 2372, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G1_2312_2372 */
	{ 2312, 2372, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G2_2312_2372 */

	{ 2412, 2472, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G1_2412_2472 */
	{ 2412, 2472, 20, 0, 20, 5,  NO_DFS, PSCAN_MKKA_G, 0},	/* G2_2412_2472 */
	{ 2412, 2472, 30, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G3_2412_2472 */

	{ 2412, 2462, 27, 6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G1_2412_2462 */
	{ 2412, 2462, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA_G, 0},	/* G2_2412_2462 */
	
	{ 2432, 2442, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G1_2432_2442 */

	{ 2457, 2472, 20, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G1_2457_2472 */

	{ 2512, 2732, 5,  6, 20, 5, NO_DFS, NO_PSCAN, 0},	/* G1_2512_2732 */

	{ 2467, 2472, 20, 0, 20, 5, NO_DFS, PSCAN_MKKA2 | PSCAN_MKKA, 0 }, /* G1_2467_2472 */

	{ 2312, 2372, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* WG1_2312_2372 */
	{ 2412, 2412, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* WG1_2412_2412 */
	{ 2417, 2432, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* WG1_2417_2432 */
	{ 2437, 2442, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* WG1_2437_2442 */
	{ 2447, 2457, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* WG1_2447_2457 */
	{ 2462, 2462, 18, 0, 20, 5, NO_DFS, NO_PSCAN, 0},	/* WG1_2462_2462 */
	{ 2467, 2467, 18, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0}, /* WG1_2467_2467 */
	{ 2467, 2467, 18, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},	/* WG2_2467_2467 */
	{ 2472, 2472, 18, 0, 20, 5, NO_DFS, PSCAN_WWR | IS_ECM_CHAN, 0}, /* WG1_2472_2472 */
	{ 2472, 2472, 18, 0, 20, 5, NO_DFS, NO_PSCAN | IS_ECM_CHAN, 0},	/* WG2_2472_2472 */
};

/*
 * 2GHz Dynamic turbo tags
 */

enum {
	T1_2312_2372,
	T1_2437_2437,
	T2_2437_2437,
	T3_2437_2437,
	T1_2512_2732
};

static REG_DMN_FREQ_BAND regDmn2Ghz11gTurboFreq[] = {
	{ 2312, 2372, 5,  6, 40, 40, NO_DFS, NO_PSCAN, 0},  /* T1_2312_2372 */
	{ 2437, 2437, 5,  6, 40, 40, NO_DFS, NO_PSCAN, 0},  /* T1_2437_2437 */
	{ 2437, 2437, 20, 6, 40, 40, NO_DFS, NO_PSCAN, 0},  /* T2_2437_2437 */
	{ 2437, 2437, 18, 6, 40, 40, NO_DFS, PSCAN_WWR, 0}, /* T3_2437_2437 */
	{ 2512, 2732, 5,  6, 40, 40, NO_DFS, NO_PSCAN, 0},  /* T1_2512_2732 */
};


/*
 * 2GHz 11n frequency tags 
 */
enum {
    NG1_2422_2452,
    NG2_2422_2452,
    NG3_2422_2452,

    NG_DEMO_ALL_CHANNELS,
};

static REG_DMN_FREQ_BAND regDmn2Ghz11ngFreq[] = {
    { 2422, 2452, 20, 0, 40, 5, NO_DFS, NO_PSCAN, 0},    /* NG1_2422_2452 */
    { 2422, 2452, 27, 0, 40, 5, NO_DFS, NO_PSCAN, 0},    /* NG2_2422_2452 */
    { 2422, 2452, 30, 0, 40, 5, NO_DFS, NO_PSCAN, 0},    /* NG3_2422_2452 */

	{ 2312, 2732, 27, 6, 20, 5, NO_DFS, NO_PSCAN, 0},    /* NG_DEMO_ALL_CHANNELS */
};


/*
 * 5GHz 11n frequency tags
 */
enum {
    NA_DEMO_ALL_CHANNELS,
    NA1_5190_5230,
    NA2_5190_5230,
    NA3_5190_5230,
    NA4_5190_5230,
    NA5_5190_5230,

    NA1_5270_5310,
    NA2_5270_5310,
    NA3_5270_5310,
    NA4_5270_5310,

    NA1_5310_5310,

    NA1_5510_5630,

    NA1_5510_5670,
    NA2_5510_5670,
    NA3_5510_5670,

    NA1_5755_5795,
    NA2_5755_5795,
    NA3_5755_5795,
    NA4_5755_5795,

    NA1_5795_5795,
};

static REG_DMN_FREQ_BAND regDmn5Ghz11naFreq[] = {
	{ 4920, 6100, 30,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA_DEMO_ALL_CHANNELS */
#ifdef NOTYET
    /* 
     * ToDo: This table needs to be completely populated with 5GHz 11n properties
     */
	{ 5190, 5230, 15,  0, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA1_5190_5230 */
	{ 5190, 5230, 17,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA2_5190_5230 */
	{ 5190, 5230, 18,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA3_5190_5230 */
	{ 5190, 5230, 20,  0, 40, 40, DFS_ETSI, NO_PSCAN, 0},	/* NA4_5190_5230 */ 
	{ 5190, 5230, 23,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA5_5190_5230 */


	{ 5270, 5310, 18,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA1_5270_5310 */
	{ 5270, 5310, 20,  0, 40, 40, DFS_ETSI, NO_PSCAN, 0},	/* NA2_5270_5310 */
	{ 5270, 5310, 23,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA3_5270_5310 */
	{ 5270, 5310, 30,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA4_5270_5310 */

	{ 5310, 5310, 17,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA1_5310_5310 */

	{ 5510, 5630, 30,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA1_5510_5630 */

	{ 5510, 5670, 20,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA1_5510_5670 */
	{ 5510, 5670, 27,  0, 40, 40, DFS_ETSI, NO_PSCAN, 0},	/* NA2_5510_5670 */
	{ 5510, 5670, 30,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA3_5510_5670 */

	{ 5755, 5795, 17,  0, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA1_5755_5795 */ 
	{ 5755, 5795, 20,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA2_5755_5795 */
	{ 5755, 5795, 23,  0, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA3_5755_5795 */
	{ 5755, 5795, 30,  0, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA4_5755_5795 */

	{ 5795, 5795, 30,  6, 40, 40, NO_DFS, NO_PSCAN, 0},	    /* NA1_5795_5795 */
#endif
};


typedef struct regDomain {
	u_int16_t regDmnEnum;	/* value from EnumRd table */
	u_int8_t conformanceTestLimit;
	u_int64_t dfsMask;	/* DFS bitmask for 5Ghz tables */
	u_int64_t pscan;	/* Bitmask for passive scan */
	u_int32_t flags;	/* Requirement flags (AdHoc disallow, noise
				   floor cal needed, etc) */
	u_int64_t chan11a[BMLEN];/* 128 bit bitmask for channel/band
				   selection */
	u_int64_t chan11a_turbo[BMLEN];/* 128 bit bitmask for channel/band
				   selection */
	u_int64_t chan11a_dyn_turbo[BMLEN]; /* 128 bit bitmask for channel/band
					       selection */
	u_int64_t chan11b[BMLEN];/* 128 bit bitmask for channel/band
				   selection */
	u_int64_t chan11g[BMLEN];/* 128 bit bitmask for channel/band
				   selection */
	u_int64_t chan11g_turbo[BMLEN];/* 128 bit bitmask for channel/band
					  selection */
	u_int64_t chan11ng[BMLEN];/* 128 bit bitmask for 11n in 2GHz */
	u_int64_t chan11na[BMLEN];/* 128 bit bitmask for 11n in 5GHz */
} REG_DOMAIN;

static REG_DOMAIN regDomains[] = {

	{DEBUG_REG_DMN, FCC, DFS_FCC3, NO_PSCAN, NO_REQ,
	 BM(F1_5120_5240, F1_5260_5700, F1_5745_5825, -1, -1, -1, -1, -1),
	 BM(T1_5130_5210, T1_5250_5330, T1_5370_5490, T1_5530_5650, T1_5150_5190, T1_5230_5310, T1_5350_5470, T1_5510_5670),
	 BM(T1_5200_5200, T1_5240_5280, T1_5540_5660, T1_5765_5805, -1, -1, -1, -1),
	 BM(F1_2312_2372, F1_2412_2472, F1_2484_2484, F1_2512_2732, -1, -1, -1, -1),
	 BM(G1_2312_2372, G1_2412_2472, G1_2512_2732, -1, -1, -1, -1, -1),
	 BM(T1_2312_2372, T1_2437_2437, T1_2512_2732, -1, -1, -1, -1, -1),
     BM(NG_DEMO_ALL_CHANNELS,-1, -1, -1, -1, -1, -1, -1),
     BM(NA_DEMO_ALL_CHANNELS,-1, -1, -1, -1, -1, -1, -1)},

	{APL1, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BM(F4_5745_5825, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{APL2, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BM(F1_5745_5805, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{APL3, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BM(F1_5280_5320, F2_5745_5805, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{APL4, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BM(F4_5180_5240,  F3_5745_5825, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{APL5, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BM(F2_5745_5825, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{APL6, ETSI, DFS_ETSI, PSCAN_FCC_T | PSCAN_FCC , NO_REQ,
	 BM(F4_5180_5240, F2_5260_5320, F3_5745_5825, -1, -1, -1, -1, -1),
	 BM(T2_5210_5210, T1_5250_5290, T1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{ETSI1, ETSI, DFS_ETSI, PSCAN_ETSI, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BM(F4_5180_5240, F2_5260_5320, F2_5500_5700, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{ETSI2, ETSI, DFS_ETSI, PSCAN_ETSI, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BM(F3_5180_5240, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{ETSI3, ETSI, DFS_ETSI, PSCAN_ETSI, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BM(F4_5180_5240, F2_5260_5320, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{ETSI4, ETSI, DFS_ETSI, PSCAN_ETSI, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BM(F3_5180_5240, F1_5260_5320, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{ETSI5, ETSI, DFS_ETSI, PSCAN_ETSI, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BM(F1_5180_5240, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{ETSI6, ETSI, DFS_ETSI, PSCAN_ETSI, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BM(F5_5180_5240,F1_5260_5280, F3_5500_5700, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{FCC1, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BM(F2_5180_5240,F4_5260_5320,F5_5745_5825, -1, -1, -1, -1, -1),
	 BM(T1_5210_5210,T2_5250_5290,T2_5760_5800, -1, -1, -1, -1, -1),
	 BM(T1_5200_5200,T1_5240_5280,T1_5765_5805, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
     BM(NA_DEMO_ALL_CHANNELS,-1, -1, -1, -1, -1, -1, -1)},

	{FCC2, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BM(F6_5180_5240,F5_5260_5320,F6_5745_5825, -1, -1, -1, -1, -1),
	 BM(-1, -1, -1, -1, -1, -1, -1, -1),
	 BM(T1_5200_5200,T1_5240_5280,T1_5765_5805, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{FCC3, FCC, DFS_FCC3, PSCAN_FCC | PSCAN_FCC_T, NO_REQ,
	 BM(F2_5180_5240, F3_5260_5320, F1_5500_5700, F5_5745_5825, -1, -1, -1, -1),
	 BM(T1_5210_5210,T2_5250_5290,T1_5540_5660,T2_5760_5800, -1, -1, -1, -1),
	 BM(T1_5200_5200, T1_5240_5280, T1_5540_5660, T1_5765_5805, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{FCC4, FCC, DFS_FCC3, PSCAN_FCC | PSCAN_FCC_T, NO_REQ,
	 BM(F1_4942_4987, F1_4945_4985, F1_4950_4980, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{MKK1, MKK, NO_DFS, PSCAN_MKK1, DISALLOW_ADHOC_11A_TURB,
	 BM(F1_5170_5230, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{MKK2, MKK, NO_DFS, PSCAN_MKK2, DISALLOW_ADHOC_11A_TURB,
	 BM(F1_4920_4980, F1_5040_5080, F1_5170_5230, F1_4915_4925, F1_4935_4945, F1_5035_5040, F1_5055_5055, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{MKK3, MKK, DFS_MKK3, PSCAN_MKK3, DISALLOW_ADHOC_11A_TURB,
	 BM(F4_5180_5240, F2_5260_5320, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{MKK4, MKK, DFS_MKK3, PSCAN_MKK3, DISALLOW_ADHOC_11A_TURB,
	 BM(F4_5180_5240, F2_5260_5320, F4_5500_5700, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{MKK5, MKK, DFS_MKK3, PSCAN_MKK3, DISALLOW_ADHOC_11A_TURB,
	 BM(F1_4920_4980, F1_5040_5080, F4_5180_5240, F2_5260_5320, F4_5500_5700, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO},
	 
	{APLD, NO_CTL, NO_DFS, NO_PSCAN, NO_REQ,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F2_2312_2372,F2_2412_2472, -1, -1, -1, -1, -1, -1),
	 BM(G2_2312_2372,G2_2412_2472, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BMZERO},

	{ETSIA, NO_CTL, NO_DFS, PSCAN_ETSIA, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F1_2457_2472,-1, -1, -1, -1, -1, -1, -1),
	 BM(G1_2457_2472,-1, -1, -1, -1, -1, -1, -1),
	 BM(T2_2437_2437,-1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{ETSIB, ETSI, NO_DFS, PSCAN_ETSIB, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F1_2432_2442,-1,-1,-1,-1,-1,-1,-1),
	 BM(G1_2432_2442,-1,-1,-1,-1,-1,-1,-1),
	 BM(T2_2437_2437,-1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{ETSIC, ETSI, NO_DFS, PSCAN_ETSIC, DISALLOW_ADHOC_11A | DISALLOW_ADHOC_11A_TURB,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F3_2412_2472,-1,-1,-1,-1,-1,-1,-1),
	 BM(G3_2412_2472,-1,-1,-1,-1,-1,-1,-1),
	 BM(T2_2437_2437,-1, -1, -1, -1, -1, -1, -1),
     BM(NG3_2422_2452,-1, -1, -1, -1, -1, -1, -1),
	 BMZERO},

	{FCCA, FCC, NO_DFS, NO_PSCAN, NO_REQ,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F1_2412_2462,-1,-1,-1,-1,-1,-1,-1),
	 BM(G1_2412_2462,-1,-1,-1,-1,-1,-1,-1),
	 BM(T2_2437_2437,-1, -1, -1, -1, -1, -1, -1),
     BM(NG2_2422_2452,-1, -1, -1, -1, -1, -1, -1),
	 BMZERO},

	{MKKA, MKK, NO_DFS, PSCAN_MKKA | PSCAN_MKKA_G | PSCAN_MKKA1 | PSCAN_MKKA1_G | PSCAN_MKKA2 | PSCAN_MKKA2_G, DISALLOW_ADHOC_11A_TURB,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F2_2412_2462, F1_2467_2472, F2_2484_2484, -1, -1, -1, -1, -1),
	 BM(G2_2412_2462, G1_2467_2472, -1, -1, -1, -1, -1, -1),
	 BM(T2_2437_2437,-1, -1, -1, -1, -1, -1, -1),
	 BM(NG1_2422_2452,-1, -1, -1, -1, -1, -1, -1),
	 BMZERO},

	{MKKC, MKK, NO_DFS, NO_PSCAN, NO_REQ,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F2_2412_2472,-1,-1,-1,-1,-1,-1,-1),
	 BM(G2_2412_2472,-1,-1,-1,-1,-1,-1,-1),
	 BM(T2_2437_2437,-1, -1, -1, -1, -1, -1, -1),
	 BM(NG1_2422_2452,-1,-1, -1, -1, -1, -1, -1),
	 BMZERO},

	{WORLD, ETSI, NO_DFS, NO_PSCAN, NO_REQ,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BM(F2_2412_2472,-1,-1,-1,-1,-1,-1,-1),
	 BM(G2_2412_2472,-1,-1,-1,-1,-1,-1,-1),
	 BM(T2_2437_2437,-1, -1, -1, -1, -1, -1, -1),
	 BM(NG1_2422_2452,-1, -1, -1, -1, -1, -1, -1), 
	 BMZERO},

	{WOR0_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_PER_11D,
	 BM(W1_5260_5320, W1_5180_5240, W1_5170_5230, W1_5745_5825, W1_5500_5700, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412,W1_2437_2442,W1_2462_2462,W1_2472_2472,W1_2417_2432, W1_2447_2457, W1_2467_2467, W1_2484_2484),
	 BM(WG1_2412_2412,WG1_2437_2442,WG1_2462_2462,WG1_2472_2472,WG1_2417_2432,WG1_2447_2457,WG1_2467_2467, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR01_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_PER_11D,
	 BM(W1_5260_5320, W1_5180_5240, W1_5170_5230, W1_5745_5825, W1_5500_5700, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412, W1_2437_2442, W1_2462_2462, W1_2417_2432, W1_2447_2457, -1, -1, -1),
	 BM(WG1_2412_2412, WG1_2437_2442, WG1_2462_2462, WG1_2417_2432, WG1_2447_2457, -1, -1, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR02_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_PER_11D,
	 BM(W1_5260_5320, W1_5180_5240,W1_5170_5230,W1_5745_5825,W1_5500_5700, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412,W1_2437_2442,W1_2462_2462, W1_2472_2472,W1_2417_2432, W1_2447_2457, W1_2467_2467, -1),
	 BM(WG1_2412_2412,WG1_2437_2442,WG1_2462_2462, WG1_2472_2472,WG1_2417_2432, WG1_2447_2457, WG1_2467_2467, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{EU1_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_PER_11D,
	 BM(W1_5260_5320, W1_5180_5240,W1_5170_5230,W1_5745_5825,W1_5500_5700, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412,W1_2437_2442,W1_2462_2462, W2_2472_2472,W1_2417_2432, W1_2447_2457, W2_2467_2467, -1),
	 BM(WG1_2412_2412,WG1_2437_2442,WG1_2462_2462, WG2_2472_2472,WG1_2417_2432, WG1_2447_2457, WG2_2467_2467, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR1_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_NO_11A,
	 BM(W1_5260_5320, W1_5180_5240, W1_5170_5230, W1_5745_5825, W1_5500_5700, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BM(W1_2412_2412,W1_2437_2442,W1_2462_2462,W1_2472_2472,W1_2417_2432, W1_2447_2457, W1_2467_2467, W1_2484_2484),
	 BM(WG1_2412_2412,WG1_2437_2442,WG1_2462_2462,WG1_2472_2472,WG1_2417_2432,WG1_2447_2457,WG1_2467_2467, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR2_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_NO_11A,
	 BM(W1_5260_5320, W1_5180_5240, W1_5170_5230, W1_5745_5825, W1_5500_5700, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412,W1_2437_2442,W1_2462_2462,W1_2472_2472,W1_2417_2432, W1_2447_2457, W1_2467_2467, W1_2484_2484),
	 BM(WG1_2412_2412,WG1_2437_2442,WG1_2462_2462,WG1_2472_2472,WG1_2417_2432,WG1_2447_2457,WG1_2467_2467, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR3_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_PER_11D,
	 BM(W1_5260_5320, W1_5180_5240, W1_5170_5230, W1_5745_5825, -1, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412,W1_2437_2442,W1_2462_2462,W1_2472_2472,W1_2417_2432, W1_2447_2457, W1_2467_2467, -1),
	 BM(WG1_2412_2412,WG1_2437_2442,WG1_2462_2462,WG1_2472_2472,WG1_2417_2432,WG1_2447_2457,WG1_2467_2467,-1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR4_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_NO_11A,
	 BM(W1_5260_5320, W1_5180_5240,W1_5745_5825, -1, -1, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412,W1_2437_2442,W1_2462_2462, W1_2417_2432,W1_2447_2457,-1, -1, -1),
	 BM(WG1_2412_2412,WG1_2437_2442,WG1_2462_2462, WG1_2417_2432,WG1_2447_2457,-1, -1, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR5_ETSIC, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_NO_11A,
	 BM(W1_5260_5320, W1_5180_5240,W1_5745_5825, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BM(W1_2412_2412, W1_2437_2442, W1_2462_2462, W1_2472_2472, W1_2417_2432, W1_2447_2457, W1_2467_2467, -1),
	 BM(WG1_2412_2412, WG1_2437_2442, WG1_2462_2462, WG1_2472_2472, WG1_2417_2432, WG1_2447_2457, WG1_2467_2467, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WOR9_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_NO_11A,
	 BM(W1_5260_5320, W1_5180_5240, W1_5745_5825, W1_5500_5700, -1, -1, -1, -1),
	 BM(WT1_5210_5250, WT1_5290_5290, WT1_5760_5800, -1, -1, -1, -1, -1),
	 BMZERO,
	 BM(W1_2412_2412, W1_2437_2442, W1_2462_2462, W1_2417_2432, W1_2447_2457, -1, -1, -1),
	 BM(WG1_2412_2412, WG1_2437_2442, WG1_2462_2462, WG1_2417_2432, WG1_2447_2457, -1, -1, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{WORA_WORLD, NO_CTL, DFS_FCC3 | DFS_ETSI, PSCAN_WWR, ADHOC_NO_11A,
	 BM(W1_5260_5320, W1_5180_5240, W1_5745_5825, W1_5500_5700, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO,
	 BM(W1_2412_2412, W1_2437_2442, W1_2462_2462, W1_2472_2472, W1_2417_2432, W1_2447_2457, W1_2467_2467, -1),
	 BM(WG1_2412_2412, WG1_2437_2442, WG1_2462_2462, WG1_2472_2472, WG1_2417_2432, WG1_2447_2457, WG1_2467_2467, -1),
	 BM(T3_2437_2437, -1, -1, -1, -1, -1, -1, -1),
	 BMZERO,
	 BMZERO},

	{NULL1, NO_CTL, NO_DFS, NO_PSCAN, NO_REQ,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO,
	 BMZERO}
};

struct cmode {
	u_int16_t	mode;
	u_int32_t	flags;
};

static const struct cmode modes[] = {
	{ HAL_MODE_TURBO,	CHANNEL_ST},	/* TURBO means 11a Static Turbo */
	{ HAL_MODE_11A,		CHANNEL_A},
	{ HAL_MODE_11B,		CHANNEL_B},
	{ HAL_MODE_11G,		CHANNEL_G},
	{ HAL_MODE_11G_TURBO,	CHANNEL_108G},
	{ HAL_MODE_11A_TURBO,	CHANNEL_108A},
	{ HAL_MODE_11NA,	CHANNEL_A_HT40},
	{ HAL_MODE_11NA,	CHANNEL_A_HT20},
	{ HAL_MODE_11NG,	CHANNEL_G_HT40},
	{ HAL_MODE_11NG,	CHANNEL_G_HT20},
};

static int
chansort(const void *a, const void *b)
{
#define CHAN_FLAGS	(CHANNEL_ALL|CHANNEL_HALF|CHANNEL_QUARTER)
	const HAL_CHANNEL_INTERNAL *ca = a;
	const HAL_CHANNEL_INTERNAL *cb = b;

	return (ca->channel == cb->channel) ?
		(ca->channelFlags & CHAN_FLAGS) -
			(cb->channelFlags & CHAN_FLAGS) :
		ca->channel - cb->channel;
#undef CHAN_FLAGS
}
typedef int ath_hal_cmp_t(const void *, const void *);
static	void ath_hal_sort(void *a, size_t n, size_t es, ath_hal_cmp_t *cmp);
static COUNTRY_CODE_TO_ENUM_RD* findCountry(HAL_CTRY_CODE countryCode);
static HAL_BOOL getWmRD(struct ath_hal *ah, COUNTRY_CODE_TO_ENUM_RD *country, u_int16_t channelFlag, REG_DOMAIN *rd);


static u_int16_t
getEepromRD(struct ath_hal *ah)
{
	return AH_PRIVATE(ah)->ah_currentRD &~ WORLDWIDE_ROAMING_FLAG;
}

/*
 * Test to see if the bitmask array is all zeros
 */
static HAL_BOOL
isChanBitMaskZero(u_int64_t *bitmask)
{
	int i;

	for (i=0; i<BMLEN; i++) {
		if (bitmask[i] != 0)
			return AH_FALSE;
	}
	return AH_TRUE;
}

/*
 * Return whether or not the regulatory domain/country in EEPROM
 * is acceptable.
 */
static HAL_BOOL
isEepromValid(struct ath_hal *ah)
{
	u_int16_t rd = getEepromRD(ah);
	int i;

	if (rd & COUNTRY_ERD_FLAG) {
		u_int16_t cc = rd &~ COUNTRY_ERD_FLAG;
		for (i = 0; i < N(allCountries); i++)
			if (allCountries[i].countryCode == cc)
				return AH_TRUE;
	} else {
		for (i = 0; i < N(regDomainPairs); i++)
			if (regDomainPairs[i].regDmnEnum == rd)
				return AH_TRUE;
	}
	HALDEBUG(ah, "%s: invalid regulatory domain/country code 0x%x\n",
		__func__, rd);
	return AH_FALSE;
}

/*
 * Returns whether or not the specified country code
 * is allowed by the EEPROM setting
 */
static HAL_BOOL
isCountryCodeValid(struct ath_hal *ah, HAL_CTRY_CODE cc)
{
	u_int16_t rd;

	/* Default setting requires no checks */
	if (cc == CTRY_DEFAULT)
		return AH_TRUE;
#ifdef AH_DEBUG_COUNTRY
	if (cc == CTRY_DEBUG)
		return AH_TRUE;
#endif
	rd = getEepromRD(ah);
	HALDEBUG(ah, "%s: EEPROM regdomain 0x%x\n", __func__, rd);

	if (rd & COUNTRY_ERD_FLAG) {
		/* EEP setting is a country - config shall match */
		HALDEBUG(ah, "%s: EEPROM setting is country code %u\n",
			__func__, rd &~ COUNTRY_ERD_FLAG);
		return (cc == (rd & ~COUNTRY_ERD_FLAG));
	} else if (rd == DEBUG_REG_DMN || rd == NO_ENUMRD) {
		/* Set to Debug or AllowAnyCountry mode - allow any setting */
		HALDEBUG(ah, "%s: DEBUG or NO\n", __func__);
		return AH_TRUE;
	} else {
		int i;
		for (i = 0; i < N(allCountries); i++) {
			if (cc == allCountries[i].countryCode &&
				allCountries[i].regDmnEnum == rd)
				return AH_TRUE;
		}
	}
	return AH_FALSE;
}

/*
 * Return the mask of available modes based on the hardware
 * capabilities and the specified country code and reg domain.
 */
static u_int
ath_hal_getwmodesnreg(struct ath_hal *ah, COUNTRY_CODE_TO_ENUM_RD *country,
			 REG_DOMAIN *rd5GHz)
{
	u_int modesAvail;

	/* Get modes that HW is capable of */
	modesAvail = ath_hal_getWirelessModes(ah);

	/* Check country regulations for allowed modes */
	if ((modesAvail & HAL_MODE_11A_TURBO) &&
	    (!country->allow11aTurbo))
		modesAvail &= ~HAL_MODE_11A_TURBO;
	if ((modesAvail & HAL_MODE_11G_TURBO) &&
	    (!country->allow11gTurbo))
		modesAvail &= ~HAL_MODE_11G_TURBO;
	if ((modesAvail & HAL_MODE_11G) &&
	    (!country->allow11g))
		modesAvail &= ~HAL_MODE_11G;
	if ((modesAvail & HAL_MODE_11A) &&
	    (isChanBitMaskZero(rd5GHz->chan11a)))
		modesAvail &= ~HAL_MODE_11A;
	if ((modesAvail & HAL_MODE_11NG) &&
	    (!country->allow11g)) 
		modesAvail &= ~HAL_MODE_11NG;
	if ((modesAvail & HAL_MODE_11NA) &&
	        ((!country->allow11na) ||
             (isChanBitMaskZero(rd5GHz->chan11na))) ) {
		modesAvail &= ~HAL_MODE_11NA;
	}
	return modesAvail;
}

/*
 * Return the mask of available modes based on the hardware
 * capabilities and the specified country code.
 */

u_int
ath_hal_getwirelessmodes(struct ath_hal *ah, HAL_CTRY_CODE cc)
{
	COUNTRY_CODE_TO_ENUM_RD *country=AH_NULL;
	u_int mode=0;
	REG_DOMAIN rd;
	
	country = findCountry(cc);
	if (country != AH_NULL) {
		if (getWmRD(ah, country, ~CHANNEL_2GHZ, &rd))
			mode = ath_hal_getwmodesnreg(ah, country, &rd);
	}
	return(mode);
}

/*
 * Return if device is public safety.
 */
HAL_BOOL
ath_hal_ispublicsafetysku(struct ath_hal *ah)
{
	u_int16_t rd;

	rd = getEepromRD(ah);

	switch (rd) {
		case FCC4_FCCA:
		case (CTRY_UNITED_STATES_FCC49 | COUNTRY_ERD_FLAG):
			return AH_TRUE;

		case DEBUG_REG_DMN:
		case NO_ENUMRD:
			if (AH_PRIVATE(ah)->ah_countryCode == 
						CTRY_UNITED_STATES_FCC49) {
				return AH_TRUE;
			}
			break;
	}

	return AH_FALSE;
}

/*
 * Find the pointer to the country element in the country table
 * corresponding to the country code
 */
static COUNTRY_CODE_TO_ENUM_RD*
findCountry(HAL_CTRY_CODE countryCode)
{
	int i;

	for (i=0; i<N(allCountries); i++) {
		if (allCountries[i].countryCode == countryCode)
			return (&allCountries[i]);
	}
	return (AH_NULL);		/* Not found */
}

/*
 * Calculate a default country based on the EEPROM setting.
 */
static HAL_CTRY_CODE
getDefaultCountry(struct ath_hal *ah)
{
	u_int16_t rd;
	int i;

	rd =getEepromRD(ah);
	if (rd & COUNTRY_ERD_FLAG) {
		COUNTRY_CODE_TO_ENUM_RD *country=AH_NULL;
		u_int16_t cc= rd & ~COUNTRY_ERD_FLAG;
		
		country = findCountry(cc);
		if (country != AH_NULL)
			return cc;
	}
	/*
	 * Check reg domains that have only one country
	 */
	for (i = 0; i < N(regDomainPairs); i++)
		if (regDomainPairs[i].regDmnEnum == rd) {
			if (regDomainPairs[i].singleCC != 0)
				return regDomainPairs[i].singleCC;
			else
				i = N(regDomainPairs);
		}
	return CTRY_DEFAULT;
}

static HAL_BOOL
isValidRegDmn(int regDmn, REG_DOMAIN *rd)
{
	int i;

	for (i=0;i<N(regDomains); i++) {
		if (regDomains[i].regDmnEnum == regDmn) {
			if (rd != AH_NULL) {
				*rd = regDomains[i];
			}
			return AH_TRUE;
		}
	}
	return AH_FALSE;
}

static HAL_BOOL
isValidRegDmnPair(int regDmnPair)
{
	int i;

	if (regDmnPair == NO_ENUMRD)
		return AH_FALSE;
	for (i=0; i<N(regDomainPairs); i++) {
		if (regDomainPairs[i].regDmnEnum == regDmnPair)
			return AH_TRUE;
	}
	return AH_FALSE;
}

/*
 * Return the Wireless Mode Regulatory Domain based
 * on the country code and the wireless mode.
 */
static HAL_BOOL
getWmRD(struct ath_hal *ah, COUNTRY_CODE_TO_ENUM_RD *country, u_int16_t channelFlag, REG_DOMAIN *rd)
{
	int i, found, regDmn;
	u_int64_t flags=NO_REQ;
	REG_DMN_PAIR_MAPPING *regPair=AH_NULL;
	struct ath_hal_private *ahp;

	ahp = AH_PRIVATE(ah);
	if (country->countryCode == CTRY_DEFAULT) {
		u_int16_t rd;
		rd =getEepromRD(ah);

		if (!(rd & COUNTRY_ERD_FLAG)) {
			if (isValidRegDmn(rd, AH_NULL) ||
			    isValidRegDmnPair(rd)) {
				regDmn = rd;
			} else
				regDmn = country->regDmnEnum;
		} else
			regDmn = country->regDmnEnum;
	} else
		regDmn = country->regDmnEnum;
	if ((regDmn & MULTI_DOMAIN_MASK) == 0) {

		for (i=0, found=0; (i<N(regDomainPairs))&&(!found); i++) {
			if (regDomainPairs[i].regDmnEnum == regDmn) {
				regPair = &regDomainPairs[i];
				found = 1;
			}
		}
		if (!found) {
			HALDEBUG(ah, "%s: Failed to find reg domain pair %u\n",
				 __func__, regDmn);
			return AH_FALSE;
		}
		if (!(channelFlag & CHANNEL_2GHZ)) {
			regDmn = regPair->regDmn5GHz;
			flags = regPair->flags5GHz;
		}
		if (channelFlag & CHANNEL_2GHZ) {
			regDmn = regPair->regDmn2GHz;
			flags = regPair->flags2GHz;
		}
	}

	/*
	 * We either started with a unitary reg domain or we've found the 
	 * unitary reg domain of the pair
	 */

	found = isValidRegDmn(regDmn, rd);
	if (!found) {
		HALDEBUG(ah, "%s: Failed to find unitary reg domain %u\n",
			 __func__, country->regDmnEnum);
		return AH_FALSE;
	} else {
		rd->pscan &= regPair->pscanMask;
		if (((country->regDmnEnum & MULTI_DOMAIN_MASK) == 0) &&
		    (flags != NO_REQ)) {
			rd->flags = flags;
		}
		return AH_TRUE;
	}
}

HAL_BOOL
IS_BIT_SET(int bit, u_int64_t *bitmask)
{
	int byteOffset, bitnum;
	u_int64_t val;

	byteOffset = bit/64;
	bitnum = bit - byteOffset*64;
	val = ((u_int64_t) 1) << bitnum;
	if (bitmask[byteOffset] & val)
		return AH_TRUE;
	else
		return AH_FALSE;
}
	
/* Add given regclassid into regclassids array upto max of maxregids */
static void
ath_add_regclassid(u_int8_t *regclassids, u_int maxregids, u_int *nregids, u_int8_t regclassid)
{
	int i;

	/* Is regclassid valid? */
	if (regclassid == 0)
		return;

	for (i=0; i < maxregids; i++) {
		if (regclassids[i] == regclassid)
			return;
		if (regclassids[i] == 0)
			break;
	}

	if (i == maxregids)
		return;
	else {
		regclassids[i] = regclassid;
		*nregids += 1;
	}

	return;
}

/*
 * Setup the channel list based on the information in the EEPROM and
 * any supplied country code.  Note that we also do a bunch of EEPROM
 * verification here and setup certain regulatory-related access
 * control data used later on.
 */

HAL_BOOL
ath_hal_init_channels(struct ath_hal *ah,
		      HAL_CHANNEL *chans, u_int maxchans, u_int *nchans,
		      u_int8_t *regclassids, u_int maxregids, u_int *nregids,
		      HAL_CTRY_CODE cc, u_int16_t modeSelect,
		      HAL_BOOL enableOutdoor, HAL_BOOL enableExtendedChannels, int ht_mode)
{
#define CHANNEL_HALF_BW		10
#define CHANNEL_QUARTER_BW	5
	u_int modesAvail;
	u_int16_t maxChan=7000;
	COUNTRY_CODE_TO_ENUM_RD *country=AH_NULL;
	REG_DOMAIN rd5GHz, rd2GHz;
	const struct cmode *cm;
	HAL_CHANNEL_INTERNAL *ichans=&AH_PRIVATE(ah)->ah_channels[0];
	int next=0,b;
	u_int8_t ctl = 0;
	int is_quarterchan_cap, is_halfchan_cap; 
    int ht_g_mode=0, ht_a_mode=0;

	HALDEBUG(ah, "%s: cc %u mode 0x%x%s%s\n", __func__,
		 cc, modeSelect, enableOutdoor? "Enable outdoor" : " ",
		 enableExtendedChannels ? " Enable ecm" : "");
	/*
	 * Validate the EEPROM setting and setup defaults
	 */
	if (!isEepromValid(ah)) {
		/*
		 * Don't return any channels if the EEPROM has an
		 * invalid regulatory domain/country code setting.
		 */
		HALDEBUG(ah, "%s: invalid EEPROM contents\n",__func__);
		return AH_FALSE;
	}
	AH_PRIVATE(ah)->ah_countryCode = getDefaultCountry(ah);
	
	/* 
	 * We now have enough state to validate any country code
	 * passed in by the caller.  XXX - TODO: Switch based on Japan
	 * country code to new reg domain if valid japan card
	 */
	if (AH_PRIVATE(ah)->ah_countryCode == CTRY_DEFAULT) {
		if (!isCountryCodeValid(ah, cc)) {
			/* NB: Atheros silently ignores invalid country codes */
			HALDEBUG(ah, "%s: invalid country code %d\n",
				 __func__, cc);
			return AH_FALSE;
		}
		AH_PRIVATE(ah)->ah_countryCode = cc;
	}
	/* Get pointers to the country element and the reg domain elements */
	country = findCountry(AH_PRIVATE(ah)->ah_countryCode);
	HALASSERT(country != NULL);

	if (!getWmRD(ah, country, ~CHANNEL_2GHZ, &rd5GHz)) {
		HALDEBUG(ah,"%s: couldn't find unitary 5GHz reg domain for country %u\n",
			 __func__, AH_PRIVATE(ah)->ah_countryCode);
		return AH_FALSE;
	}
	if (!getWmRD(ah, country, CHANNEL_2GHZ, &rd2GHz)) {
		HALDEBUG(ah,"%s: couldn't find unitary 2GHz reg domain for country %u\n",
			 __func__, AH_PRIVATE(ah)->ah_countryCode);
		return AH_FALSE;
	}

	modesAvail = ath_hal_getwmodesnreg(ah, country, &rd5GHz);

	if (!enableOutdoor)
		maxChan = country->outdoorChanStart;

	next = 0;

	if (maxchans > N(AH_PRIVATE(ah)->ah_channels))
		maxchans = N(AH_PRIVATE(ah)->ah_channels);

    /* Turbo and 11n don't go togather */
    if(modeSelect & HAL_MODE_11NG  || modeSelect & HAL_MODE_11NA)
        modesAvail &= ~(HAL_MODE_TURBO | HAL_MODE_11G_TURBO | HAL_MODE_11A_TURBO );

	is_halfchan_cap = 
		(((*(ah->ah_getCapability))(ah, HAL_CAP_CHAN_HALFRATE, 0, AH_NULL)) == HAL_OK);
	is_quarterchan_cap = 
		(((*(ah->ah_getCapability))(ah, HAL_CAP_CHAN_QUARTERRATE, 0, AH_NULL)) == HAL_OK);
	for (cm = modes; cm < &modes[N(modes)]; cm++) {
		u_int16_t c, c_hi, c_lo;
		u_int64_t *channelBM=AH_NULL;
		REG_DOMAIN *rd=AH_NULL;
		REG_DMN_FREQ_BAND *fband=AH_NULL,*freqs=AH_NULL;

		if ((cm->mode & modeSelect) == 0) {
			HALDEBUG(ah, "%s: skip mode 0x%x flags 0x%x\n",
				 __func__, cm->mode, cm->flags);
			continue;
		}
		if ((cm->mode & modesAvail) == 0) {
			HALDEBUG(ah, "%s: !avail mode 0x%x (0x%x) flags 0x%x\n",
				 __func__, modesAvail, cm->mode, cm->flags);
			continue;
		}
		if (!ath_hal_getChannelEdges(ah, cm->flags, &c_lo, &c_hi)) {
			/* channel not supported by hardware, skip it */
			HALDEBUG(ah, "%s: channels 0x%x not supported by hardware\n",
				 __func__,cm->flags);
			continue;
		}
		switch (cm->mode) {
		case HAL_MODE_TURBO:
			rd = &rd5GHz;
			channelBM = rd->chan11a_turbo;
			freqs = &regDmn5GhzTurboFreq[0];
			ctl = rd->conformanceTestLimit | CTL_TURBO;
			break;
		case HAL_MODE_11A:
			rd = &rd5GHz;
			channelBM = rd->chan11a;
			freqs = &regDmn5GhzFreq[0];
			ctl = rd->conformanceTestLimit | CTL_11A;
			break;
		case HAL_MODE_11B:
			rd = &rd2GHz;
			channelBM = rd->chan11b;
			freqs = &regDmn2GhzFreq[0];
			ctl = rd->conformanceTestLimit | CTL_11B;
			break;
		case HAL_MODE_11NG:
			rd = &rd2GHz;
            if(ht_mode & CHANNEL_HT20 &&
                cm->flags == CHANNEL_G_HT20 &&
                !(ht_g_mode & CHANNEL_HT20))
            {
                /* Get 11ng-HT20 channels */
                ht_g_mode |= CHANNEL_HT20;
			    channelBM = rd->chan11g;
			    freqs = &regDmn2Ghz11gFreq[0];
			    ctl = rd->conformanceTestLimit | CTL_2GHT20;
            }
            else if(ht_mode & CHANNEL_HT40 &&
                        cm->flags == CHANNEL_G_HT40 &&
                        !(ht_g_mode & CHANNEL_HT40))
            {
                if(!country->allow11ng)
                    continue;

                /* Get 11ng-HT40 channels */
                ht_g_mode |= CHANNEL_HT40;
			    channelBM = rd->chan11ng;
			    freqs = &regDmn2Ghz11ngFreq[0];
			    ctl = rd->conformanceTestLimit | CTL_2GHT40;
            }
            break;
		case HAL_MODE_11NA:
			rd = &rd5GHz;
            if(ht_mode & CHANNEL_HT20 &&
                 cm->flags == CHANNEL_A_HT20 &&
                 !(ht_a_mode & CHANNEL_HT20))  
            {
                /* Get 11ng-HT20 channels */
                ht_a_mode |= CHANNEL_HT20;
			    channelBM = rd->chan11a;
			    freqs = &regDmn5GhzFreq[0];
			    ctl = rd->conformanceTestLimit | CTL_5GHT20;
            }
            else if(ht_mode & CHANNEL_HT40 &&
                       cm->flags == CHANNEL_A_HT40 &&
                       !(ht_a_mode & CHANNEL_HT40)) 
            {
                if(!country->allow11na)
                    continue;
                /* Get 11ng-HT40 channels */
                ht_a_mode |= CHANNEL_HT40;
			    channelBM = rd->chan11na;
			    freqs = &regDmn5Ghz11naFreq[0];
			    ctl = rd->conformanceTestLimit | CTL_5GHT40;
            }
            break;
		case HAL_MODE_11G:
			rd = &rd2GHz;
			channelBM = rd->chan11g;
			freqs = &regDmn2Ghz11gFreq[0];
			ctl = rd->conformanceTestLimit | CTL_11G;
			break;
		case HAL_MODE_11G_TURBO:
			rd = &rd2GHz;
			channelBM = rd->chan11g_turbo;
			freqs = &regDmn2Ghz11gTurboFreq[0];
			ctl = rd->conformanceTestLimit | CTL_108G;
			break;
		case HAL_MODE_11A_TURBO:
			rd = &rd5GHz;
			channelBM = rd->chan11a_dyn_turbo;
			freqs = &regDmn5GhzTurboFreq[0];
			ctl = rd->conformanceTestLimit | CTL_108G;
			break;
		default:
			HALDEBUG(ah, "%s: Unkonwn HAL mode 0x%x\n",
				__func__, cm->mode);
			continue;
		}
		if (isChanBitMaskZero(channelBM))
			continue;
		for (b=0;b<64*BMLEN; b++) {
			if (IS_BIT_SET(b,channelBM)) {
				fband = &freqs[b];

				ath_add_regclassid(regclassids, maxregids, 
						nregids, fband->regClassId);

				for (c=fband->lowChannel; c <= fband->highChannel;
				     c += fband->channelSep) {
					HAL_CHANNEL_INTERNAL icv;
					
					if (!(c_lo <= c && c <= c_hi)) {
						HALDEBUG(ah, "%s: c %u out of range [%u..%u]\n",
							 __func__, c, c_lo, c_hi);
						continue;
					}
					if ((fband->channelBW == 
							CHANNEL_HALF_BW) && 
						!is_halfchan_cap) {
						HALDEBUG(ah, "%s: Skipping %u half rate channel\n", 
								__func__, c); 
						continue;
					}

					if ((fband->channelBW == 
						CHANNEL_QUARTER_BW) && 
						!is_quarterchan_cap) {
						HALDEBUG(ah, "%s: Skipping %u quarter rate channel\n", 
								__func__, c); 
						continue;
					}

					if (((c+fband->channelSep)/2) > (maxChan+HALF_MAXCHANBW)) {
						HALDEBUG(ah, "%s: c %u > maxChan %u\n",
							 __func__, c, maxChan);
						continue;
					}
					if (next >= maxchans){
						HALDEBUG(ah, "%s: too many channels for channel table\n",
							 __func__);
						goto done;
					}
					if ((fband->usePassScan & IS_ECM_CHAN) &&
					    !enableExtendedChannels) {
						HALDEBUG(ah, "Skipping ecm channel\n");
						continue;
					}

					icv.channel = c;
					icv.channelFlags = cm->flags;

					switch (fband->channelBW) {
						case CHANNEL_HALF_BW:
							icv.channelFlags |= CHANNEL_HALF;
							break;
						case CHANNEL_QUARTER_BW:
							icv.channelFlags |= CHANNEL_QUARTER;
							break;
					}

					icv.maxRegTxPower = fband->powerDfs;
					icv.antennaMax = fband->antennaMax;
					icv.regDmnFlags = rd->flags;
					icv.conformanceTestLimit = ctl;
					if (fband->usePassScan & rd->pscan)
						icv.channelFlags |= CHANNEL_PASSIVE;
					else
						icv.channelFlags &= ~CHANNEL_PASSIVE;
					if (fband->useDfs & rd->dfsMask)
						icv.privFlags = CHANNEL_DFS;
					else
						icv.privFlags = 0;
					
					ichans[next++] = icv;
				}
			}
		}
	}
done:	if (next != 0) {
		int i;

		/* XXX maxchans set above so this cannot happen? */
		if (next > N(AH_PRIVATE(ah)->ah_channels)) {
			HALDEBUG(ah, "%s: too many channels %u; truncating to %u\n",
				 __func__, next, N(AH_PRIVATE(ah)->ah_channels));
			next = N(AH_PRIVATE(ah)->ah_channels);
		}

		/*
		 * Keep a private copy of the channel list so we can
		 * constrain future requests to only these channels
		 */
		ath_hal_sort(ichans, next, sizeof(HAL_CHANNEL_INTERNAL), chansort);
		AH_PRIVATE(ah)->ah_nchan = next;

		/*
		 * Copy the channel list to the public channel list
		 */
		for (i=0; i<next; i++) {
			chans[i].channel = ichans[i].channel;
			chans[i].channelFlags = ichans[i].channelFlags;
			chans[i].privFlags = ichans[i].privFlags;
			chans[i].maxRegTxPower = ichans[i].maxRegTxPower;
		}
		/*
		 * Retrieve power limits.
		 */
		ath_hal_getpowerlimits(ah, chans, next);
	}
	*nchans = next;
	/* XXX copy private setting to public area */
	ah->ah_countryCode = AH_PRIVATE(ah)->ah_countryCode;
	return (next != 0);
#undef CHANNEL_HALF_BW
#undef CHANNEL_QUARTER_BW
}

/*
 * Return whether or not the specified channel is ok to use
 * based on the current regulatory domain constraints.
 */
HAL_CHANNEL_INTERNAL *
ath_hal_checkchannel(struct ath_hal *ah, const HAL_CHANNEL *c)
{
#define CHAN_FLAGS	(CHANNEL_ALL|CHANNEL_HALF|CHANNEL_QUARTER)
	HAL_CHANNEL_INTERNAL *base, *cc;
	/* NB: be wary of user-specified channel flags */
	int flags = c->channelFlags & CHAN_FLAGS;
	int n, lim;

	HALDEBUG(ah, "%s: channel %u/0x%x (0x%x) requested\n",
		__func__, c->channel, c->channelFlags, flags);

	/*
	 * Check current channel to avoid the lookup.
	 */
	cc = AH_PRIVATE(ah)->ah_curchan;
	if (cc != AH_NULL && cc->channel == c->channel &&
	    (cc->channelFlags & CHAN_FLAGS) == flags)
		return cc;

	/* binary search based on known sorting order */
	base = AH_PRIVATE(ah)->ah_channels;
	n = AH_PRIVATE(ah)->ah_nchan;
	/* binary search based on known sorting order */
	for (lim = n; lim != 0; lim >>= 1) {
		HAL_CHANNEL_INTERNAL *cc = &base[lim>>1];
		int d = c->channel - cc->channel;
		if (d == 0) {
			if ((cc->channelFlags & CHAN_FLAGS) == flags) 
				return cc;
			d = flags - (cc->channelFlags & CHAN_FLAGS);
		}
		HALDEBUG(ah, "%s: channel %u/0x%x d %d\n", __func__,
			cc->channel, cc->channelFlags, d);
		if (d > 0) {
			base = cc + 1;
			lim--;
		}
	}
	HALDEBUG(ah, "%s: no match\n", __func__);
	return AH_NULL;
#undef CHAN_FLAGS
}

/*
 * Return the max allowed antenna gain and apply any regulatory
 * domain specific changes.
 *
 * NOTE: a negative reduction is possible in RD's that only
 * measure radiated power (e.g., ETSI) which would increase
 * that actual conducted output power (though never beyond
 * the calibrated target power).
 */
u_int
ath_hal_getantennareduction(struct ath_hal *ah, HAL_CHANNEL *chan, u_int twiceGain)
{
	HAL_CHANNEL_INTERNAL *ichan=AH_NULL;
	int8_t antennaMax;

	if ((ichan = ath_hal_checkchannel(ah, chan)) != AH_NULL) {
		antennaMax = twiceGain - ichan->antennaMax*2;
		return (antennaMax < 0) ? 0 : antennaMax;
	} else {
		/* Failed to find the correct index - may be a debug channel */
		return 0;
	}
}

u_int
ath_hal_getantennaallowed(struct ath_hal *ah, HAL_CHANNEL *chan)
{
	HAL_CHANNEL_INTERNAL *ichan = AH_NULL;

    ichan = ath_hal_checkchannel(ah, chan);
    if (!ichan)
        return 0;

    return ichan->antennaMax;
}


/* XXX - KCYU - fix this code ... maybe move ctl decision into channel set area or
 into the tables so no decision is needed in the code */


/*
 * Return the test group from the specified channel from
 * the regulatory table.
 *
 * TODO: CTL for 11B CommonMode when regulatory domain is unknown
 */
u_int
ath_hal_getctl(struct ath_hal *ah, HAL_CHANNEL *chan)
{
	u_int ctl=NO_CTL;
	HAL_CHANNEL_INTERNAL *ichan;

#ifdef notyet
	/* Special CTL to signify WWR SKU without a known country */
	if (AH_PRIVATE(ah)->ah_countryCode == CTRY_DEFAULT && isWwrSKU(ah)) {
		if (IS_CHAN_B(cflags)) {
			ctl = SD_NO_CTL | CTL_11B;
		} else if (IS_CHAN_G(cflags)) {
			ctl = SD_NO_CTL | CTL_11G;
		} else if (IS_CHAN_108G(cflags)) {
			ctl = SD_NO_CTL | CTL_108G;
		} else if (IS_CHAN_T(cflags)) {
			ctl = SD_NO_CTL | CTL_TURBO;
		} else {
			ctl = SD_NO_CTL | CTL_11A;
		}
	} else {
#endif
		if ((ichan = ath_hal_checkchannel(ah, chan)) != AH_NULL) {
			ctl = ichan->conformanceTestLimit;
			/* Atheros change# 73449: limit 11G OFDM power */
			if (IS_CHAN_PUREG(chan) && (ctl & 0xf) == CTL_11B)
				ctl = (ctl &~ 0xf) | CTL_11G;
		}
#ifdef notyet
	}
#endif
	return ctl;
}

/*
 * Return whether or not a noise floor check is required in
 * the current regulatory domain for the specified channel.
 */

HAL_BOOL
ath_hal_getnfcheckrequired(struct ath_hal *ah, HAL_CHANNEL *chan)
{
	HAL_CHANNEL_INTERNAL *ichan;

	if ((ichan = ath_hal_checkchannel(ah, chan)) != AH_NULL) {
		return ((ichan->regDmnFlags & NEED_NFC) ? AH_TRUE : AH_FALSE);
	}
	return AH_FALSE;
}


/*
 * Insertion sort.
 */
#define swap(_a, _b, _size) {			\
	u_int8_t *s = _b;			\
	int i = _size;				\
	do {					\
		u_int8_t tmp = *_a;		\
		*_a++ = *s;			\
		*s++ = tmp;			\
	} while (--i);				\
	_a -= _size;				\
}

static void
ath_hal_sort(void *a, size_t n, size_t size, ath_hal_cmp_t *cmp)
{
	u_int8_t *aa = a;
	u_int8_t *ai, *t;

	for (ai = aa+size; --n >= 1; ai += size)
		for (t = ai; t > aa; t -= size) {
			u_int8_t *u = t - size;
			if (cmp(u, t) <= 0)
				break;
			swap(u, t, size);
		}
}

#undef N
