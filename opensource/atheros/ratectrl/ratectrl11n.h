/*
 * Copyright (c) 2000-2002 Atheros Communications, Inc., All Rights Reserved
 *
 * Definitions for core driver
 * This is a common header file for all platforms and operating systems.
 */
#ifndef _RATECTRL_H_
#define _RATECTRL_H_

#ifdef __linux__
#include <linux/config.h>
#include <linux/version.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>

#include <net80211/if_media.h>
#include <net80211/ieee80211_var.h>

#include "if_athrate.h"
#include "if_athvar.h"
#elif __FreeBSD__
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/errno.h>

#include <sys/socket.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net80211/ieee80211_var.h>

#include <netinet/in.h> 
#include <netinet/if_ether.h>
#include <dev/ath/if_athrate.h>
#include <dev/ath/if_athvar.h>
#else
#error "Don't know how to handle your operating system!"
#endif

/*
 * Set configuration parameters here; they cover multiple files.
 */
#define TURBO_PRIME     1
#define ATHEROS_DEBUG   1

/*
 * Compatibility shims.  We leverage the work already done for the hal.
 */
typedef u_int8_t        A_UINT8;
typedef int8_t          A_INT8;
typedef u_int16_t       A_UINT16;
typedef int16_t         A_INT16;
typedef u_int32_t       A_UINT32;
typedef int32_t         A_INT32;
typedef int             A_BOOL;

#define FALSE   0

/* For legacy validity */

#define TRUE    1

/* HT 20/40 rates. If 20 bit is enabled then that rate is
 * used only in 20 mode. If both 20/40 bits are enabled
 * then that rate can be used for both 20 and 40 mode */

#define TRUE_20	 	0x2	
#define TRUE_40 	0x4	
#define TRUE_2040	(TRUE_20|TRUE_40)
#define TRUE_ALL	(TRUE_2040|TRUE)

typedef int8_t          A_RSSI;
typedef int32_t         A_RSSI32;

typedef u_int8_t        WLAN_PHY;

#define INLINE          __inline

#ifndef A_MIN
#define A_MIN(a,b)      ((a)<(b)?(a):(b))
#endif

#ifndef A_MAX
#define A_MAX(a,b)      ((a)>(b)?(a):(b))
#endif

/*
 * Use the hal os glue code to get ms time; we supply
 * a null arg because we know it's not needed.
 */
#define A_MS_TICKGET()  OS_GETUPTIME(NULL)
#define A_MEM_ZERO(p,s) OS_MEMZERO(p,s)


enum {
    WLAN_RC_PHY_CCK,
    WLAN_RC_PHY_OFDM,
    WLAN_RC_PHY_TURBO,
    WLAN_RC_PHY_XR,
    WLAN_RC_PHY_HT_20_SS,
    WLAN_RC_PHY_HT_20_DS,
    WLAN_RC_PHY_HT_40_SS,
    WLAN_RC_PHY_HT_40_DS,
    WLAN_RC_PHY_HT_20_SS_HGI,
    WLAN_RC_PHY_HT_20_DS_HGI,
    WLAN_RC_PHY_HT_40_SS_HGI,
    WLAN_RC_PHY_HT_40_DS_HGI,
    WLAN_RC_PHY_MAX
};

enum {
    WLAN_RC_DS  = 0x01,
    WLAN_RC_40  = 0x02,
    WLAN_RC_SGI = 0x04,
    WLAN_RC_HT  = 0x08,
};


typedef enum {
    WLAN_RC_LEGACY      = 0,
    WLAN_RC_HT_LNPHY    = 1,
    WLAN_RC_HT_PLPHY    = 2,
    WLAN_RC_MAX         = 3
} WLAN_RC_VERS;


#define WLAN_RC_PHY_DS(_phy)   ((_phy == WLAN_RC_PHY_HT_20_DS)           \
                                || (_phy == WLAN_RC_PHY_HT_40_DS)        \
                                || (_phy == WLAN_RC_PHY_HT_20_DS_HGI)    \
                                || (_phy == WLAN_RC_PHY_HT_40_DS_HGI))   
#define WLAN_RC_PHY_40(_phy)   ((_phy == WLAN_RC_PHY_HT_40_SS)           \
                                || (_phy == WLAN_RC_PHY_HT_40_DS)        \
                                || (_phy == WLAN_RC_PHY_HT_40_SS_HGI)    \
                                || (_phy == WLAN_RC_PHY_HT_40_DS_HGI))   
#define WLAN_RC_PHY_SGI(_phy)  ((_phy == WLAN_RC_PHY_HT_20_SS_HGI)      \
                                || (_phy == WLAN_RC_PHY_HT_20_DS_HGI)   \
                                || (_phy == WLAN_RC_PHY_HT_40_SS_HGI)   \
                                || (_phy == WLAN_RC_PHY_HT_40_DS_HGI))   

#define WLAN_RC_PHY_HT(_phy)    (_phy >= WLAN_RC_PHY_HT_20_SS)

	/* Returns the capflag mode */

#define WLAN_RC_CAP_MODE(capflag) (((capflag & WLAN_RC_HT_FLAG)?	\
				    (capflag & WLAN_RC_40_FLAG)?TRUE_40:TRUE_20:\
				    TRUE))

/* Return TRUE if flag supports HT20 && client supports HT20 or
 * return TRUE if flag supports HT40 && client supports HT40.
 * This is used becos some rates overlap between HT20/HT40.
 */

#define WLAN_RC_PHY_HT_VALID(flag, capflag) (((flag & TRUE_20) && !(capflag \
				& WLAN_RC_40_FLAG)) || ((flag & TRUE_40) && \
				  (capflag & WLAN_RC_40_FLAG)))


#define WLAN_RC_DS_FLAG         (0x01)
#define WLAN_RC_40_FLAG         (0x02)
#define WLAN_RC_SGI_FLAG        (0x04)
#define WLAN_RC_HT_FLAG         (0x08)

/* Index into the rate table */
#define INIT_RATE_MAX_20	23		
#define INIT_RATE_MAX_40	40

#define PKTLOG_RATE_CTL_FIND(_sc, log_data, flags)   ath_log_rcfind(_sc, log_data, flags);
#define PKTLOG_RATE_CTL_UPDATE(_sc, log_data, flags) ath_log_rcupdate(_sc, log_data, flags);
#define ASSERT(condition)

#define WIRELESS_MODE_11a       IEEE80211_MODE_11A
#define WIRELESS_MODE_11b       IEEE80211_MODE_11B
#define WIRELESS_MODE_11g       IEEE80211_MODE_11G
#define WIRELESS_MODE_TURBO     IEEE80211_MODE_TURBO_A  /* NB: diff reduction */
#define WIRELESS_MODE_108a      IEEE80211_MODE_TURBO_A
#define WIRELESS_MODE_108g      IEEE80211_MODE_TURBO_G
#define WIRELESS_MODE_11NA      IEEE80211_MODE_11NA
#define WIRELESS_MODE_11NG      IEEE80211_MODE_11NG
#define WIRELESS_MODE_XR        IEEE80211_MODE_MAX
#define WIRELESS_MODE_N         IEEE80211_MODE_MAX+1
#define WIRELESS_MODE_MAX       IEEE80211_MODE_MAX+2

#define RX_FLIP_THRESHOLD       3       /* XXX */
#define MAX_TX_RATE_TBL         64
#define MAX_TX_RATE_PHY 	48

/*
 * State structures for new rate adaptation code
 *
 * NOTE: Modifying these structures will impact
 * the Perl script that parses packet logging data.
 * See the packet logging module for more information.
 */
typedef struct TxRateCrtlState_s {
    A_RSSI rssiThres;           /* required rssi for this rate (dB) */
    A_UINT8 per;                /* recent estimate of packet error rate (%) */
} TxRateCtrlState;

typedef struct TxRateCtrl_s {
    TxRateCtrlState state[MAX_TX_RATE_TBL];                         /* state for each rate */
    A_UINT8  validRateIndex[MAX_TX_RATE_TBL];                         /* rc Index is valid for this Sib */
    A_UINT8  validPhyRateCount[WLAN_RC_PHY_MAX];                    /* valid rate count */
    A_UINT8  validPhyRateIndex[WLAN_RC_PHY_MAX][MAX_TX_RATE_TBL];   /* valid rate index */
    A_UINT8  maxValidRate;
    A_UINT8  rcPhyState;
    A_UINT8  rcPhyMode;
    A_RSSI   rssiLast;            /* last ack rssi */
    A_RSSI   rssiLastLkup;        /* last ack rssi used for lookup */
    A_RSSI   rssiLastPrev;        /* previous last ack rssi */
    A_RSSI   rssiLastPrev2;       /* 2nd previous last ack rssi */
    A_RSSI32 rssiSumCnt;          /* count of rssiSum for averaging */
    A_RSSI32 rssiSumRate;         /* rate that we are averaging */
    A_RSSI32 rssiSum;             /* running sum of rssi for averaging */
    A_UINT32 validTxRateMask;     /* mask of valid rates */
    A_UINT8  rateTableSize;       /* rate table size */
    A_UINT8  rateMaxPhy;          /* Phy index for the max rate */   
    A_UINT8  rateMaxIndex;        /* index for the max within the phy */
    A_INT8   antFlipCnt;          /* number of consec times retry=2,3 */
    A_UINT8  misc[16];            /* miscellaneous state */
    A_UINT32 rssiTime;            /* msec timestamp for last ack rssi */
    A_UINT32 rssiDownTime;        /* msec timestamp for last down step */
    A_UINT32 probeTime;           /* msec timestamp for last probe */
    A_UINT32 probeState;          /* msec timestamp for last probe */
    A_UINT8  probeRate;           /* rate we are probing at */
    A_UINT8  probeIndex;          /* Index of the problem within negotiated set */
    A_UINT8 rate;                 /* last rate we returned */
    A_UINT8  hwMaxRetryRate;      /* rate of last max retry fail */
    A_UINT8  hwMaxRetryPktCnt;    /* num packets since we got HW max retry error */
    A_UINT8  antProbeCnt;         /* number of packets since ant probe */
    A_UINT32 perDownTime;         /* msec timstamp for last PER down step */
    A_UINT8  switchCount;
    A_UINT8  probe_alternate;	  /* For legacy we are calling ratefind twice.
				     so the code ignores the probe rate
				     that was selected before */
} TX_RATE_CTRL;

/* per-node state */
struct atheros_node {
    TX_RATE_CTRL txRateCtrl;    /* rate control state proper */
    A_UINT32 lastRateKbps;      /* last rate in Kb/s */
    A_UINT8 rixMap[MAX_TX_RATE_TBL]; /* map of rate ix -> negotiated
                                   rate set ix */
    A_UINT8 htrixMap[MAX_TX_RATE_TBL]; /* map of ht rate ix -> negotiated
                                   rate set ix */
    A_UINT32 htcap;            /* ht capabilites */
    A_UINT8 antTx;              /* current transmit antenna */
};


#define ATH_NODE_ATHEROS(an)    ((struct atheros_node *)&an[1])
#define ATH_VAP_ATHEROS(vap)    ((struct atheros_vap *)&((struct ath_vap *)vap)[1])

/*
 * Rate Table structure for various modes - 'b', 'a', 'g', 'xr';
 * order of fields in info structure is important because hardcoded
 * structures are initialized within the hal for these
 */
#define RATE_TABLE_SIZE             64
#define MAX_SUPPORTED_MCS 	    128

typedef struct regDataLenTable {
    A_UINT8     numEntries;
    A_UINT16    frameLenRateIndex[RATE_TABLE_SIZE];
} REG_DATALEN_TABLE;

typedef struct {
    int         rateCount;
    struct {
        A_BOOL    valid;            /* Valid for use in rate control */
        WLAN_PHY  phy;              /* CCK/OFDM/TURBO/XR */
        A_UINT32  rateKbps;         /* Rate in Kbits per second */
        A_UINT32  userRateKbps;     /* User rate in KBits per second */
        A_UINT8   rateCode;         /* rate that goes into hw descriptors */
        A_UINT8   shortPreamble;    /* Mask for enabling short preamble in rate code for CCK */
        A_UINT8   dot11Rate;        /* Value that goes into supported rates info element of MLME */
        A_UINT8   controlRate;      /* Index of next lower basic rate, used for duration computation */
        A_RSSI    rssiAckValidMin;  /* Rate control related information */
        A_RSSI    rssiAckDeltaMin;  /* Rate control related information */
        A_UINT8   baseIndex;        /* base rate index */
        A_UINT8   cw40Index;        /* 40cap rate index */
        A_UINT8   sgiIndex;         /* shortgi rate index */
        A_UINT8   htIndex;          /* shortgi rate index */
    } info[RATE_TABLE_SIZE];
    A_UINT32    probeInterval;        /* interval for ratectrl to probe for other rates */
    A_UINT32    rssiReduceInterval;   /* interval for ratectrl to reduce RSSI */
    A_UINT8     initialRateMax;   /* the initial rateMax value used in rcSibUpdate() */
    REG_DATALEN_TABLE *regDataLenTable; /* Frame Lengths that comply with Regulatory reqs. */
} RATE_TABLE;


/* per-vap state */
struct atheros_vap {
        const RATE_TABLE        *rateTable;
};
/* per-device state */
struct atheros_softc {
        struct ath_ratectrl     arc;
        /* phy tables that contain rate control data */
        const RATE_TABLE        *hwRateTable[WIRELESS_MODE_MAX];
        int                     fixedrix;       /* -1 or index of fixed rate */
};

/* preferred Mode when rates overlap */
typedef struct {
    A_UINT8 prefMode;           /* Mode of the rates preferred over overlapping rates of other specifed mode */
    A_UINT8 ovlMode;            /* Mode with Overlapping rates */
} RATE_OVL_POLICY;

#ifdef ATHEROS_DEBUG
#define DPRINTF(sc, _fmt, ...) do {                             \
        if (sc->sc_debug & 0x10)                                \
                ath_hal_printf(NULL, _fmt, __VA_ARGS__);        \
} while (0)

extern  void ath_hal_printf(struct ath_hal *, const char*, ...);
#else
#define DPRINTF(sc, _fmt, ...)
#endif

/*
 *  Update the SIB's rate control information
 *
 *  This should be called when the supported rates change
 *  (e.g. SME operation, wireless mode change)
 *
 *  It will determine which rates are valid for use.
 */
void
rcSibUpdate(struct ath_softc *, struct ath_node *, A_UINT32 capflag, 
						A_BOOL keepState);

/*
 *  This routine is called to initialize the rate control parameters
 *  in the SIB. It is called initially during system initialization
 *  or when a station is associated with the AP.
 */
void    rcSibInit(struct ath_softc *, struct ath_node *);

/*
 * Determines and returns the new Tx rate index.
 */ 
void
rcRateFind(struct ath_softc *sc, struct ath_node *an,
              int numTries, int numRates, int stepDnInc,
              unsigned int rcflag, struct ath_rc_series series[], int *isProbe);

/*
 * This routine is called by the Tx interrupt service routine to give
 * the status of previous frames.
 */
void    
rcUpdate(struct ath_softc *sc, struct ath_node *an,
            A_RSSI rssiAck, A_UINT8 curTxAnt, int finalTSIdx, int Xretries, struct ath_rc_series rcs[],
            int nFrames, int nBad, int sh_lo_retry);

A_UINT8 rcRateValueToIndex(A_UINT32 txRateKbps, struct ath_softc *);

A_UINT8 rcGetSigQuality(A_UINT8 txRate, struct ath_softc *, struct ath_node *);

A_BOOL  rcGetNextLowerValidTxRate(const RATE_TABLE *pRateTable, struct TxRateCtrl_s *pRc,
                                   A_UINT8 curValidTxRate, A_UINT8 *pNextIndex);


void    atheros_setuptable(RATE_TABLE *rt);
void    ar5211SetupRateTables(void);
void    ar5211AttachRateTables(struct atheros_softc *sc);
void    ar5212SetupRateTables(void);
void    ar5212AttachRateTables(struct atheros_softc *sc);
void    ar5212SetFullRateTable(struct atheros_softc *sc);
void    ar5212SetHalfRateTable(struct atheros_softc *sc);
void    ar5212SetQuarterRateTable(struct atheros_softc *sc);
void    ar5416SetupRateTables(void);
void    ar5416AttachRateTables(struct atheros_softc *sc);
void    ar5416SetFullRateTable(struct atheros_softc *sc);
void    ar5416SetHalfRateTable(struct atheros_softc *sc);
void    ar5416SetQuarterRateTable(struct atheros_softc *sc);
#endif /* _RATECTRL_H_ */
