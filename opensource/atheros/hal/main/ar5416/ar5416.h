/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416.h#43 $
 */
#ifndef _ATH_AR5416_H_
#define _ATH_AR5416_H_

#include "ah_eeprom.h"
#include "ah_devid.h"
#include <ar5416desc.h>

#define AR5416_MAGIC            0x19641014

/* DCU Transmit Filter macros */
#define CALC_MMR(dcu, idx) \
    ( (4 * dcu) + (idx < 32 ? 0 : (idx < 64 ? 1 : (idx < 96 ? 2 : 3))) )
#define TXBLK_FROM_MMR(mmr) \
    (AR_D_TXBLK_BASE + ((mmr & 0x1f) << 6) + ((mmr & 0x20) >> 3))
#define CALC_TXBLK_ADDR(dcu, idx)   (TXBLK_FROM_MMR(CALC_MMR(dcu, idx)))
#define CALC_TXBLK_VALUE(idx)       (1 << (idx & 0x1f))

/* MAC register values */


#ifdef AR5416_INT_MITIGATION
#define INIT_INTERRUPT_MASK \
    ( AR_IMR_TXERR  | AR_IMR_RXORN | AR_IMR_RXERR  | AR_IMR_TXURN | \
      AR_IMR_TXMINTR | AR_IMR_RXMINTR | AR_IMR_TXINTM | AR_IMR_RXINTM)
#else
#define INIT_INTERRUPT_MASK \
    ( AR_IMR_TXERR  | AR_IMR_TXOK | AR_IMR_RXORN | \
      AR_IMR_RXERR  | AR_IMR_RXOK | AR_IMR_TXURN | )
#endif

#define INIT_BEACON_CONTROL \
    ((INIT_RESET_TSF << 24)  | (INIT_BEACON_EN << 23) | \
      (INIT_TIM_OFFSET << 16) | INIT_BEACON_PERIOD)

#define INIT_CONFIG_STATUS  0x00000000
#define INIT_RSSI_THR       0x00000700  /* Missed beacon counter initialized to 0x7 (max is 0xff) */
#define INIT_IQCAL_LOG_COUNT_MAX    0xF
#define INIT_BCON_CNTRL_REG 0x00000000

/*
 * Various fifo fill before Tx start, in 64-byte units
 * i.e. put the frame in the air while still DMAing
 */
#define MIN_TX_FIFO_THRESHOLD   0x1
#define MAX_TX_FIFO_THRESHOLD   (( 4096 / 64) - 1)
#define INIT_TX_FIFO_THRESHOLD  MIN_TX_FIFO_THRESHOLD

#define IS_SPUR_CHAN(_chan) \
    ( (((_chan)->channel % 32) != 0) && \
        ((((_chan)->channel % 32) < 10) || (((_chan)->channel % 32) > 22)) )

/*
 * Gain support.
 */
#define NUM_CORNER_FIX_BITS_2133    7
#define CCK_OFDM_GAIN_DELTA         15

enum GAIN_PARAMS {
    GP_TXCLIP,
    GP_PD90,
    GP_PD84,
    GP_GSEL
};

enum GAIN_PARAMS_2133 {
    GP_MIXGAIN_OVR,
    GP_PWD_138,
    GP_PWD_137,
    GP_PWD_136,
    GP_PWD_132,
    GP_PWD_131,
    GP_PWD_130,
};

enum {
    HAL_RESET_POWER_ON,
    HAL_RESET_WARM,
    HAL_RESET_COLD,
};

typedef struct _gainOptStep {
    int16_t paramVal[NUM_CORNER_FIX_BITS_2133];
    int32_t stepGain;
    int8_t  stepName[16];
} GAIN_OPTIMIZATION_STEP;

typedef struct {
    u_int32_t   numStepsInLadder;
    u_int32_t   defaultStepNum;
    GAIN_OPTIMIZATION_STEP optStep[10];
} GAIN_OPTIMIZATION_LADDER;

typedef struct {
    u_int32_t   currStepNum;
    u_int32_t   currGain;
    u_int32_t   targetGain;
    u_int32_t   loTrig;
    u_int32_t   hiTrig;
    u_int32_t   gainFCorrection;
    u_int32_t   active;
    const GAIN_OPTIMIZATION_STEP *currStep;
} GAIN_VALUES;

typedef struct {
    u_int16_t   synth_center;
    u_int16_t   ctl_center;
    u_int16_t   ext_center;
} CHAN_CENTERS;

/* RF HAL structures */
typedef struct RfHalFuncs {
    void      (*rfDetach)(struct ath_hal *ah);
    void      (*writeRegs)(struct ath_hal *,
            u_int modeIndex, u_int freqIndex, int regWrites);
    HAL_BOOL  (*setChannel)(struct ath_hal *, HAL_CHANNEL_INTERNAL *, HAL_HT *);
    HAL_BOOL  (*setRfRegs)(struct ath_hal *,
              HAL_CHANNEL_INTERNAL *, u_int16_t modesIndex);
    HAL_BOOL  (*getChipPowerLim)(struct ath_hal *ah, HAL_CHANNEL *chans,
                       u_int32_t nchancs);
} RF_HAL_FUNCS;

/*
 * Per-channel ANI state private to the driver.
 */
struct ar5416AniState {
    HAL_CHANNEL c;
    u_int8_t    noiseImmunityLevel;
    u_int8_t    spurImmunityLevel;
    u_int8_t    firstepLevel;
    u_int8_t    ofdmWeakSigDetectOff;
    u_int8_t    cckWeakSigThreshold;

    /* Thresholds */
    u_int32_t   listenTime;
    u_int32_t   ofdmTrigHigh;
    u_int32_t   ofdmTrigLow;
    int32_t     cckTrigHigh;
    int32_t     cckTrigLow;
    int32_t     rssiThrLow;
    int32_t     rssiThrHigh;

    u_int32_t   noiseFloor; /* The current noise floor */
    u_int32_t   txFrameCount;   /* Last txFrameCount */
    u_int32_t   rxFrameCount;   /* Last rx Frame count */
    u_int32_t   cycleCount; /* Last cycleCount (can detect wrap-around) */
    u_int32_t   ofdmPhyErrCount;/* OFDM err count since last reset */
    u_int32_t   cckPhyErrCount; /* CCK err count since last reset */
    u_int32_t   ofdmPhyErrBase; /* Base value for ofdm err counter */
    u_int32_t   cckPhyErrBase;  /* Base value for cck err counters */
    int16_t     pktRssi[2]; /* Average rssi of pkts for 2 antennas */
    int16_t     ofdmErrRssi[2]; /* Average rssi of ofdm phy errs for 2 ant */
    int16_t     cckErrRssi[2];  /* Average rssi of cck phy errs for 2 ant */
};

#define HAL_PROCESS_ANI     0x00000001  /* ANI state setup */
#define HAL_RADAR_EN        0x80000000  /* Radar detect is capable */
#define HAL_AR_EN       0x40000000  /* AR detect is capable */

#define DO_ANI(ah) \
    ((AH5416(ah)->ah_procPhyErr & HAL_PROCESS_ANI))

struct ar5416Stats {
    u_int32_t   ast_ani_niup;   /* ANI increased noise immunity */
    u_int32_t   ast_ani_nidown; /* ANI decreased noise immunity */
    u_int32_t   ast_ani_spurup; /* ANI increased spur immunity */
    u_int32_t   ast_ani_spurdown;/* ANI descreased spur immunity */
    u_int32_t   ast_ani_ofdmon; /* ANI OFDM weak signal detect on */
    u_int32_t   ast_ani_ofdmoff;/* ANI OFDM weak signal detect off */
    u_int32_t   ast_ani_cckhigh;/* ANI CCK weak signal threshold high */
    u_int32_t   ast_ani_ccklow; /* ANI CCK weak signal threshold low */
    u_int32_t   ast_ani_stepup; /* ANI increased first step level */
    u_int32_t   ast_ani_stepdown;/* ANI decreased first step level */
    u_int32_t   ast_ani_ofdmerrs;/* ANI cumulative ofdm phy err count */
    u_int32_t   ast_ani_cckerrs;/* ANI cumulative cck phy err count */
    u_int32_t   ast_ani_reset;  /* ANI parameters zero'd for non-STA */
    u_int32_t   ast_ani_lzero;  /* ANI listen time forced to zero */
    u_int32_t   ast_ani_lneg;   /* ANI listen time calculated < 0 */
    HAL_MIB_STATS   ast_mibstats;   /* MIB counter stats */
    HAL_NODE_STATS  ast_nodestats;  /* Latest rssi stats from driver */
};

struct ar5416RadReader {
    u_int16_t   rd_index;
    u_int16_t   rd_expSeq;
    u_int32_t   rd_resetVal;
    u_int8_t    rd_start;
};

struct ar5416RadWriter {
    u_int16_t   wr_index;
    u_int16_t   wr_seq;
};

struct ar5416RadarEvent {
    u_int32_t   re_ts;      /* 32 bit time stamp */
    u_int8_t    re_rssi;    /* rssi of radar event */
    u_int8_t    re_dur;     /* duration of radar pulse */
    u_int8_t    re_chanIndex;   /* Channel of event */
};

struct ar5416RadarQElem {
    u_int32_t   rq_seqNum;
    u_int32_t   rq_busy;        /* 32 bit to insure atomic read/write */
    struct ar5416RadarEvent rq_event;   /* Radar event */
};

struct ar5416RadarQInfo {
    u_int16_t   ri_qsize;       /* q size */
    u_int16_t   ri_seqSize;     /* Size of sequence ring */
    struct ar5416RadReader ri_reader;   /* State for the q reader */
    struct ar5416RadWriter ri_writer;   /* state for the q writer */
};

#define HAL_MAX_ACK_RADAR_DUR   511
#define HAL_MAX_NUM_PEAKS   3
#define HAL_ARQ_SIZE        4096        /* 8K AR events for buffer size */
#define HAL_ARQ_SEQSIZE     4097        /* Sequence counter wrap for AR */
#define HAL_RADARQ_SIZE     1024        /* 1K radar events for buffer size */
#define HAL_RADARQ_SEQSIZE  1025        /* Sequence counter wrap for radar */
#define HAL_NUMRADAR_STATES 64      /* Number of radar channels we keep state for */

struct ar5416ArState {
    u_int16_t   ar_prevTimeStamp;
    u_int32_t   ar_prevWidth;
    u_int32_t   ar_phyErrCount[HAL_MAX_ACK_RADAR_DUR];
    u_int32_t   ar_ackSum;
    u_int16_t   ar_peakList[HAL_MAX_NUM_PEAKS];
    u_int32_t   ar_packetThreshold; /* Thresh to determine traffic load */
    u_int32_t   ar_parThreshold;    /* Thresh to determine peak */
    u_int32_t   ar_radarRssi;       /* Rssi threshold for AR event */
};

struct ar5416RadarState {
    HAL_CHANNEL_INTERNAL *rs_chan;      /* Channel info */
    u_int8_t    rs_chanIndex;       /* Channel index in radar structure */
    u_int32_t   rs_numRadarEvents;  /* Number of radar events */
    int32_t     rs_firpwr;      /* Thresh to check radar sig is gone */
    u_int32_t   rs_radarRssi;       /* Thresh to start radar det (dB) */
    u_int32_t   rs_height;      /* Thresh for pulse height (dB)*/
    u_int32_t   rs_pulseRssi;       /* Thresh to check if pulse is gone (dB) */
    u_int32_t   rs_inband;      /* Thresh to check if pusle is inband (0.5 dB) */
};

#define AR5416_OPFLAGS_11A           0x01   /* if set, allow 11a */
#define AR5416_OPFLAGS_11G           0x02   /* if set, allow 11g */
#define AR5416_OPFLAGS_N_5G_HT40     0x04   /* if set, disable 5G HT40 */
#define AR5416_OPFLAGS_N_2G_HT40     0x08   /* if set, disable 2G HT40 */
#define AR5416_OPFLAGS_N_5G_HT20     0x10   /* if set, disable 5G HT20 */
#define AR5416_OPFLAGS_N_2G_HT20     0x20   /* if set, disable 2G HT20 */

/* RF silent fields in EEPROM */
#define EEP_RFSILENT_ENABLED        1
#define EEP_RFSILENT_POLARITY       0x0002
#define EEP_RFSILENT_POLARITY_S     1
#define EEP_RFSILENT_GPIO_SEL       0x001c
#define EEP_RFSILENT_GPIO_SEL_S     2

#define AR5416_EEP_NO_BACK_VER       0x1
#define AR5416_EEP_VER               0xE
#define AR5416_EEP_VER_MINOR_MASK    0x0FFF
#define AR5416_EEP_MINOR_VER_2       0x2
#define AR5416_EEP_MINOR_VER_3       0x3

// 16-bit offset location start of calibration struct
#define AR5416_EEP_START_LOC         256
#define AR5416_NUM_5G_CAL_PIERS      8
#define AR5416_NUM_2G_CAL_PIERS      4
#define AR5416_NUM_5G_20_TARGET_POWERS  8
#define AR5416_NUM_5G_40_TARGET_POWERS  8
#define AR5416_NUM_2G_CCK_TARGET_POWERS 3
#define AR5416_NUM_2G_20_TARGET_POWERS  4
#define AR5416_NUM_2G_40_TARGET_POWERS  4
#define AR5416_NUM_CTLS              24
#define AR5416_NUM_BAND_EDGES        8
#define AR5416_NUM_PD_GAINS          4
#define AR5416_PD_GAINS_IN_MASK      4
#define AR5416_PD_GAIN_ICEPTS        5
#define AR5416_EEPROM_MODAL_SPURS    5
#define AR5416_MAX_RATE_POWER        63
#define AR5416_NUM_PDADC_VALUES      128
#define AR5416_NUM_RATES             16
#define AR5416_BCHAN_UNUSED          0xFF
#define AR5416_MAX_PWR_RANGE_IN_HALF_DB 64
#define AR5416_EEPMISC_BIG_ENDIAN    0x01
#define FREQ2FBIN(x,y) ((y) ? ((x) - 2300) : (((x) - 4800) / 5))
#define AR5416_MAX_CHAINS            3
#define AR5416_ANT_16S               25

#define AR5416_NUM_ANT_CHAIN_FIELDS     7
#define AR5416_NUM_ANT_COMMON_FIELDS    4
#define AR5416_SIZE_ANT_CHAIN_FIELD     3
#define AR5416_SIZE_ANT_COMMON_FIELD    4
#define AR5416_ANT_CHAIN_MASK           0x7
#define AR5416_ANT_COMMON_MASK          0xf
#define AR5416_CHAIN_0_IDX              0
#define AR5416_CHAIN_1_IDX              1
#define AR5416_CHAIN_2_IDX              2

#define AR5416_LEGACY_CHAINMASK		1

typedef enum {
    EEP_NFTHRESH_5,
    EEP_NFTHRESH_2,
    EEP_MAC_MSW,
    EEP_MAC_MID,
    EEP_MAC_LSW,
    EEP_REG_0,
    EEP_REG_1,
    EEP_OP_CAP,
    EEP_OP_MODE,
    EEP_RF_SILENT,
    EEP_OB_5,
    EEP_DB_5,
    EEP_OB_2,
    EEP_DB_2,
    EEP_MINOR_REV,
    EEP_TX_MASK,
    EEP_RX_MASK,
} EEPROM_PARAM;

typedef enum Ar5416_Rates {
    rate6mb,  rate9mb,  rate12mb, rate18mb,
    rate24mb, rate36mb, rate48mb, rate54mb,
    rate1l,   rate2l,   rate2s,   rate5_5l,
    rate5_5s, rate11l,  rate11s,  rateXr,
    rateHt20_0, rateHt20_1, rateHt20_2, rateHt20_3,
    rateHt20_4, rateHt20_5, rateHt20_6, rateHt20_7,
    rateHt40_0, rateHt40_1, rateHt40_2, rateHt40_3,
    rateHt40_4, rateHt40_5, rateHt40_6, rateHt40_7,
    rateDupCck, rateDupOfdm, rateExtCck, rateExtOfdm,
    Ar5416RateSize
} AR5416_RATES;

typedef struct BaseEepHeader {
    u_int16_t  length;
    u_int16_t  checksum;
    u_int16_t  version;
    u_int8_t   opCapFlags;
    u_int8_t   eepMisc;
    u_int16_t  regDmn[2];
    u_int8_t   macAddr[6];
    u_int8_t   rxMask;
    u_int8_t   txMask;
    u_int16_t  rfSilent;
    u_int16_t  blueToothOptions;
    u_int16_t  deviceCap;
    u_int32_t  binBuildNumber;
    u_int8_t   deviceType;
    u_int8_t   futureBase[33];
} __packed BASE_EEP_HEADER; // 64 B

typedef struct spurChanStruct {
    u_int16_t spurChan;
    u_int8_t  spurRangeLow;
    u_int8_t  spurRangeHigh;
} __packed SPUR_CHAN;

typedef struct ModalEepHeader {
    u_int32_t  antCtrlChain[AR5416_MAX_CHAINS];       // 12
    u_int32_t  antCtrlCommon;                         // 4
    u_int8_t   antennaGainCh[AR5416_MAX_CHAINS];      // 3
    u_int8_t   switchSettling;                        // 1
    u_int8_t   txRxAttenCh[AR5416_MAX_CHAINS];        // 3
    u_int8_t   rxTxMarginCh[AR5416_MAX_CHAINS];       // 3
    u_int8_t   adcDesiredSize;                        // 1
    u_int8_t   pgaDesiredSize;                        // 1
    u_int8_t   xlnaGainCh[AR5416_MAX_CHAINS];         // 3
    u_int8_t   txEndToXpaOff;                         // 1
    u_int8_t   txEndToRxOn;                           // 1
    u_int8_t   txFrameToXpaOn;                        // 1
    u_int8_t   thresh62;                              // 1
    u_int8_t   noiseFloorThreshCh[AR5416_MAX_CHAINS]; // 3
    u_int8_t   xpdGain;                               // 1
    u_int8_t   xpd;                                   // 1
    u_int8_t   iqCalICh[AR5416_MAX_CHAINS];           // 1
    u_int8_t   iqCalQCh[AR5416_MAX_CHAINS];           // 1
    u_int8_t   pdGainOverlap;                         // 1
    u_int8_t   ob;                                    // 1
    u_int8_t   db;                                    // 1
    u_int8_t   xpaBiasLvl;                            // 1
    u_int8_t   pwrDecreaseFor2Chain;                  // 1
    u_int8_t   pwrDecreaseFor3Chain;                  // 1 -> 48 B
    u_int8_t   txFrameToDataStart;                    // 1
    u_int8_t   txFrameToPaOn;                         // 1
    u_int8_t   ht40PowerIncForPdadc;                  // 1
    u_int8_t   bswAtten[AR5416_MAX_CHAINS];           // 3
    u_int8_t   bswMargin[AR5416_MAX_CHAINS];          // 3
    u_int8_t   swSettleHt40;                          // 1
    u_int8_t   futureModal[22];                       // 22

    SPUR_CHAN spurChans[AR5416_EEPROM_MODAL_SPURS];   // 20 B
} __packed MODAL_EEP_HEADER;                          // == 100 B

typedef struct calDataPerFreq {
    u_int8_t pwrPdg[AR5416_NUM_PD_GAINS][AR5416_PD_GAIN_ICEPTS];
    u_int8_t vpdPdg[AR5416_NUM_PD_GAINS][AR5416_PD_GAIN_ICEPTS];
} __packed CAL_DATA_PER_FREQ;

typedef struct CalTargetPowerLegacy {
    u_int8_t  bChannel;
    u_int8_t  tPow2x[4];
} __packed CAL_TARGET_POWER_LEG;

typedef struct CalTargetPowerHt {
    u_int8_t  bChannel;
    u_int8_t  tPow2x[8];
} __packed CAL_TARGET_POWER_HT;

#if AH_BYTE_ORDER == AH_BIG_ENDIAN
typedef struct CalCtlEdges {
    u_int8_t  bChannel;
    u_int8_t  flag   :2,
             tPower :6;
} __packed CAL_CTL_EDGES;
#else
typedef struct CalCtlEdges {
    u_int8_t  bChannel;
    u_int8_t  tPower :6,
             flag   :2;
} __packed CAL_CTL_EDGES;
#endif

typedef struct CalCtlData {
    CAL_CTL_EDGES  ctlEdges[AR5416_MAX_CHAINS][AR5416_NUM_BAND_EDGES];
} __packed CAL_CTL_DATA;

typedef struct eepMap {
    BASE_EEP_HEADER      baseEepHeader;         // 64 B
    u_int8_t             custData[64];                   // 64 B
    MODAL_EEP_HEADER     modalHeader[2];        // 200 B
    u_int8_t             calFreqPier5G[AR5416_NUM_5G_CAL_PIERS];
    u_int8_t             calFreqPier2G[AR5416_NUM_2G_CAL_PIERS];
    CAL_DATA_PER_FREQ    calPierData5G[AR5416_MAX_CHAINS][AR5416_NUM_5G_CAL_PIERS];
    CAL_DATA_PER_FREQ    calPierData2G[AR5416_MAX_CHAINS][AR5416_NUM_2G_CAL_PIERS];
    CAL_TARGET_POWER_LEG calTargetPower5G[AR5416_NUM_5G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower5GHT20[AR5416_NUM_5G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower5GHT40[AR5416_NUM_5G_40_TARGET_POWERS];
    CAL_TARGET_POWER_LEG calTargetPowerCck[AR5416_NUM_2G_CCK_TARGET_POWERS];
    CAL_TARGET_POWER_LEG calTargetPower2G[AR5416_NUM_2G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower2GHT20[AR5416_NUM_2G_20_TARGET_POWERS];
    CAL_TARGET_POWER_HT  calTargetPower2GHT40[AR5416_NUM_2G_40_TARGET_POWERS];
    u_int8_t             ctlIndex[AR5416_NUM_CTLS];
    CAL_CTL_DATA         ctlData[AR5416_NUM_CTLS];
    u_int8_t             padding;
} __packed ar5416_eeprom_t;  // EEP_MAP

struct ath_hal_5416 {
    struct ath_hal_private  ah_priv;    /* base class */

    /*
     * Information retrieved from EEPROM.
     */
    ar5416_eeprom_t  ah_eeprom;

    GAIN_VALUES ah_gainValues;

    u_int8_t    ah_macaddr[IEEE80211_ADDR_LEN];
    u_int8_t    ah_bssid[IEEE80211_ADDR_LEN];
    u_int8_t    ah_bssidmask[IEEE80211_ADDR_LEN];

    int16_t     ah_curchanRadIndex; /* cur. channel radar index */

    /*
     * Runtime state.
     */
    u_int32_t   ah_maskReg;     /* copy of AR_IMR */
    struct ar5416Stats ah_stats;        /* various statistics */
    RF_HAL_FUNCS    ah_rfHal;
    u_int32_t   ah_txDescMask;      /* mask for TXDESC */
    u_int32_t   ah_txOkInterruptMask;
    u_int32_t   ah_txErrInterruptMask;
    u_int32_t   ah_txDescInterruptMask;
    u_int32_t   ah_txEolInterruptMask;
    u_int32_t   ah_txUrnInterruptMask;
    HAL_TX_QUEUE_INFO ah_txq[HAL_NUM_TX_QUEUES];
    HAL_POWER_MODE  ah_powerMode;
    u_int32_t   ah_atimWindow;
    HAL_ANT_SETTING ah_diversityControl;    /* antenna setting */
    enum {
        IQ_CAL_INACTIVE,
        IQ_CAL_RUNNING,
        IQ_CAL_DONE
    } ah_bIQCalibration;            /* IQ calibrate state */
    u_int32_t   ah_tx6PowerInHalfDbm;   /* power output for 6Mb tx */
    u_int32_t   ah_staId1Defaults;  /* STA_ID1 default settings */
    u_int32_t   ah_miscMode;        /* MISC_MODE settings */
    HAL_BOOL    ah_tpcEnabled;      /* per-packet tpc enabled */
    u_int32_t   ah_beaconInterval;  /* XXX */
    enum {
        AUTO_32KHZ,     /* use it if 32kHz crystal present */
        USE_32KHZ,      /* do it regardless */
        DONT_USE_32KHZ,     /* don't use it regardless */
    } ah_enable32kHzClock;          /* whether to sleep at 32kHz */
    void        *ah_analogBanks;    /* RF register banks */
    u_int32_t   ah_ofdmTxPower;
    int16_t     ah_txPowerIndexOffset;

    u_int       ah_slottime;        /* user-specified slot time */
    u_int       ah_acktimeout;      /* user-specified ack timeout */
    u_int       ah_ctstimeout;      /* user-specified cts timeout */
    u_int       ah_globaltxtimeout; /* user-specified global tx timeout */
    /*
     * XXX
     * 11g-specific stuff; belongs in the driver.
     */
    u_int8_t    ah_gBeaconRate;     /* fixed rate for G beacons */
    /*
     * RF Silent handling; setup according to the EEPROM.
     */
    u_int32_t   ah_gpioSelect;      /* GPIO pin to use */
    u_int32_t   ah_polarity;        /* polarity to disable RF */
    u_int32_t   ah_gpioBit;     /* after init, prev value */
    HAL_BOOL    ah_eepEnabled;      /* EEPROM bit for capability */
    /*
     * ANI & Radar support.
     */
    u_int32_t   ah_procPhyErr;      /* Process Phy errs */
    HAL_BOOL    ah_hasHwPhyCounters;    /* Hardware has phy counters */
    u_int32_t   ah_aniPeriod;       /* ani update list period */
    struct ar5416AniState   *ah_curani; /* cached last reference */
    struct ar5416AniState   ah_ani[64]; /* per-channel state */
    struct ar5416RadarState ah_radar[HAL_NUMRADAR_STATES];  /* Per-Channel Radar detector state */
    struct ar5416RadarQElem *ah_radarq; /* radar event queue */
    struct ar5416RadarQInfo ah_radarqInfo;  /* radar event q read/write state */
    struct ar5416ArState    ah_ar;      /* AR detector state */
    struct ar5416RadarQElem *ah_arq;    /* AR event queue */
    struct ar5416RadarQInfo ah_arqInfo; /* AR event q read/write state */

    /*
     * Ani tables that change between the 5416 and 5312.
     * These get set at attach time.
     * XXX don't belong here
     * XXX need better explanation
     */
        int     ah_totalSizeDesired[5];
        int     ah_coarseHigh[5];
        int     ah_coarseLow[5];
        int     ah_firpwr[5];

    /*
     * Transmit power state.  Note these are maintained
     * here so they can be retrieved by diagnostic tools.
     */
    u_int16_t   ah_ratesArray[16];

    /*
     * Tx queue interrupt state.
     */
    u_int32_t   ah_intrTxqs;

    /*
     * Extension Channel Rx Clear State
     */
    u_int32_t   ah_cycleCount;
    u_int32_t   ah_ctlBusy;
    u_int32_t   ah_extBusy;

    /* HT CWM state */
    HAL_HT_CWM  ah_htcwm;

    int         ah_hwp;
    void        *ah_cal_mem;
    HAL_BOOL    ah_emu_eeprom;
};
#define AH5416(_ah) ((struct ath_hal_5416 *)(_ah))

#define IS_5416_EMU(ah) \
    ((AH_PRIVATE(ah)->ah_devid == AR5416_DEVID_EMU) || \
     (AH_PRIVATE(ah)->ah_devid == AR5416_DEVID_EMU_PCIE))

#define ar5416RfDetach(ah) do {             \
    if (AH5416(ah)->ah_rfHal.rfDetach != AH_NULL)   \
        AH5416(ah)->ah_rfHal.rfDetach(ah);  \
} while (0)

#define ar5416GetRfBank(ah, b) \
    AH5416(ah)->ah_rfHal.getRfBank(ah, b)

#define ar5416EepDataInFlash(_ah) \
    (!AH_PRIVATE(_ah)->ah_flags & AH_USE_EEPROM)

extern  HAL_BOOL ar2133RfAttach(struct ath_hal *, HAL_STATUS *);

struct ath_hal;

extern  u_int32_t ar5416RadioAttach(struct ath_hal *ah);
extern  struct ath_hal_5416 * ar5416NewState(u_int16_t devid, HAL_SOFTC sc,
        HAL_BUS_TAG st, HAL_BUS_HANDLE sh, u_int32_t flags, HAL_STATUS *status);
extern  struct ath_hal * ar5416Attach(u_int16_t devid, HAL_SOFTC sc,
        HAL_BUS_TAG st, HAL_BUS_HANDLE sh, u_int32_t flags, HAL_STATUS *status);
extern  void ar5416Detach(struct ath_hal *ah);
extern  HAL_BOOL ar5416ChipTest(struct ath_hal *ah);
extern  HAL_BOOL ar5416GetChannelEdges(struct ath_hal *ah,
                u_int16_t flags, u_int16_t *low, u_int16_t *high);
extern  HAL_BOOL ar5416FillCapabilityInfo(struct ath_hal *ah);

extern  void ar5416BeaconInit(struct ath_hal *ah,
        u_int32_t next_beacon, u_int32_t beacon_period);
extern  HAL_BOOL ar5416WaitForBeaconDone(struct ath_hal *, HAL_BUS_ADDR baddr);
extern  void ar5416ResetStaBeaconTimers(struct ath_hal *ah);
extern  void ar5416SetStaBeaconTimers(struct ath_hal *ah,
        const HAL_BEACON_STATE *);

extern  HAL_BOOL ar5416IsInterruptPending(struct ath_hal *ah);
extern  HAL_BOOL ar5416GetPendingInterrupts(struct ath_hal *ah, HAL_INT *);
extern  HAL_INT ar5416GetInterrupts(struct ath_hal *ah);
extern  HAL_INT ar5416SetInterrupts(struct ath_hal *ah, HAL_INT ints);

extern  u_int32_t ar5416GetKeyCacheSize(struct ath_hal *);
extern  HAL_BOOL ar5416IsKeyCacheEntryValid(struct ath_hal *, u_int16_t entry);
extern  HAL_BOOL ar5416ResetKeyCacheEntry(struct ath_hal *ah, u_int16_t entry);
extern  HAL_BOOL ar5416SetKeyCacheEntryMac(struct ath_hal *,
            u_int16_t entry, const u_int8_t *mac);
extern  HAL_BOOL ar5416SetKeyCacheEntry(struct ath_hal *ah, u_int16_t entry,
                       const HAL_KEYVAL *k, const u_int8_t *mac, int xorKey);

extern  void ar5416GetMacAddress(struct ath_hal *ah, u_int8_t *mac);
extern  HAL_BOOL ar5416SetMacAddress(struct ath_hal *ah, const u_int8_t *);
extern  void ar5416GetBssIdMask(struct ath_hal *ah, u_int8_t *mac);
extern  HAL_BOOL ar5416SetBssIdMask(struct ath_hal *, const u_int8_t *);
extern  HAL_STATUS ar5416EepromAttach(struct ath_hal *);
extern  u_int32_t ar5416EepromGet(struct ath_hal_5416 *ahp, EEPROM_PARAM param);
extern  HAL_STATUS ar5416EepromSetTransmitPower(struct ath_hal *ah, HAL_HT *ht,
                     ar5416_eeprom_t *pEepData, HAL_CHANNEL_INTERNAL *chan,
                     u_int16_t cfgCtl, u_int16_t twiceAntennaReduction,
                     u_int16_t twiceMaxRegulatoryPower, u_int16_t powerLimit);
extern  HAL_BOOL ar5416EepromSetBoardValues(struct ath_hal *, HAL_HT *, HAL_CHANNEL_INTERNAL *);
extern  HAL_BOOL ar5416EepromRead(struct ath_hal *, u_int off, u_int16_t *data);
extern  HAL_BOOL ar5416FlashRead(struct ath_hal *, u_int off, u_int16_t *data);
extern  HAL_BOOL ar5416EepromWrite(struct ath_hal *, u_int off, u_int16_t data);
extern  HAL_BOOL ar5416FlashWrite(struct ath_hal *, u_int off, u_int16_t data);
extern  HAL_BOOL ar5416SetRegulatoryDomain(struct ath_hal *ah,
        u_int16_t regDomain, HAL_STATUS *stats);
extern  u_int ar5416GetWirelessModes(struct ath_hal *ah);
extern  HAL_BOOL ar5416GetRfKill(struct ath_hal *ah);
extern  void ar5416EnableRfKill(struct ath_hal *);
extern  HAL_BOOL ar5416GpioCfgOutput(struct ath_hal *, u_int32_t gpio);
extern  HAL_BOOL ar5416GpioCfgInput(struct ath_hal *, u_int32_t gpio);
extern  HAL_BOOL ar5416GpioSet(struct ath_hal *, u_int32_t gpio, u_int32_t val);
extern  u_int32_t ar5416GpioGet(struct ath_hal *ah, u_int32_t gpio);
extern  void ar5416GpioSetIntr(struct ath_hal *ah, u_int, u_int32_t ilevel);
extern  void ar5416SetLedState(struct ath_hal *ah, HAL_LED_STATE state);
extern  void ar5416WriteAssocid(struct ath_hal *ah, const u_int8_t *bssid,
        u_int16_t assocId);
#ifdef ATH_FORCE_PPM
extern  u_int32_t ar5416PpmGetRssiDump(struct ath_hal *);
extern  u_int32_t ar5416PpmArmTrigger(struct ath_hal *);
extern  int ar5416PpmGetTrigger(struct ath_hal *);
extern  u_int32_t ar5416PpmForce(struct ath_hal *);
extern  void ar5416PpmUnForce(struct ath_hal *);
extern  u_int32_t ar5416PpmGetReg(struct ath_hal *, int);
#endif /* ATH_FORCE_PPM */
extern  u_int32_t ar5416GetTsf32(struct ath_hal *ah);
extern  u_int64_t ar5416GetTsf64(struct ath_hal *ah);
extern  void ar5416ResetTsf(struct ath_hal *ah);
extern  void ar5416SetBasicRate(struct ath_hal *ah, HAL_RATE_SET *pSet);
extern  u_int32_t ar5416GetRandomSeed(struct ath_hal *ah);
extern  HAL_BOOL ar5416DetectCardPresent(struct ath_hal *ah);
extern  void ar5416UpdateMibCounters(struct ath_hal *ah, HAL_MIB_STATS* stats);
extern  HAL_BOOL ar5416IsJapanChannelSpreadSupported(struct ath_hal *ah);
extern  u_int32_t ar5416GetCurRssi(struct ath_hal *ah);
extern  u_int ar5416GetDefAntenna(struct ath_hal *ah);
extern  void ar5416SetDefAntenna(struct ath_hal *ah, u_int antenna);
extern  HAL_BOOL ar5416IsSleepAfterBeaconBroken(struct ath_hal *ah);
extern  HAL_BOOL ar5416SetSlotTime(struct ath_hal *, u_int);
extern  u_int ar5416GetSlotTime(struct ath_hal *);
extern  HAL_BOOL ar5416SetAckTimeout(struct ath_hal *, u_int);
extern  u_int ar5416GetAckTimeout(struct ath_hal *);
extern  HAL_BOOL ar5416SetCTSTimeout(struct ath_hal *, u_int);
extern  u_int ar5416GetCTSTimeout(struct ath_hal *);
extern  void ar5416SetPCUConfig(struct ath_hal *);
extern  HAL_STATUS ar5416GetCapability(struct ath_hal *, HAL_CAPABILITY_TYPE,
        u_int32_t, u_int32_t *);
extern  HAL_BOOL ar5416SetCapability(struct ath_hal *, HAL_CAPABILITY_TYPE,
        u_int32_t, u_int32_t, HAL_STATUS *);
extern  HAL_BOOL ar5416GetDiagState(struct ath_hal *ah, int request,
        const void *args, u_int32_t argsize,
        void **result, u_int32_t *resultsize);
extern void ar5416GetDescInfo(struct ath_hal *ah, HAL_DESC_INFO *desc_info);
extern  u_int32_t ar5416Get11nExtBusy(struct ath_hal *ah);
extern  void ar5416Set11nMac2040(struct ath_hal *ah, HAL_HT_MACMODE mode);
extern  HAL_HT_RXCLEAR ar5416Get11nRxClear(struct ath_hal *ah);
extern  void ar5416Set11nRxClear(struct ath_hal *ah, HAL_HT_RXCLEAR rxclear);
extern  HAL_BOOL ar5416SetPowerMode(struct ath_hal *ah, HAL_POWER_MODE mode,
        int setChip);
extern  HAL_POWER_MODE ar5416GetPowerMode(struct ath_hal *ah);
extern HAL_BOOL ar5416SetPowerModeAwake(struct ath_hal *ah, int setChip);

extern  HAL_BOOL ar5416Reset(struct ath_hal *ah, HAL_OPMODE opmode,
        HAL_CHANNEL *chan, HAL_BOOL bChannelChange, HAL_STATUS *status);
extern  HAL_BOOL ar5416SetResetReg(struct ath_hal *ah, u_int32_t type,
                                   HAL_CHANNEL *chan);
extern  void ar5416SetOperatingMode(struct ath_hal *ah, int opmode);
extern  HAL_BOOL ar5416PhyDisable(struct ath_hal *ah);
extern  HAL_BOOL ar5416Disable(struct ath_hal *ah);
extern  HAL_BOOL ar5416ChipReset(struct ath_hal *ah, HAL_CHANNEL *);
extern  HAL_BOOL ar5416PerCalibration(struct ath_hal *ah,  HAL_CHANNEL *chan);
extern  void ar5416GetNoiseFloor(struct ath_hal *ah, u_int16_t nfarray[]);
extern  HAL_BOOL ar5416SetTxPowerLimit(struct ath_hal *ah, u_int32_t limit);

extern  const HAL_RATE_TABLE *ar5416GetRateTable(struct ath_hal *, u_int mode);

extern  void ar5416EnableMIBCounters(struct ath_hal *);
extern  void ar5416DisableMIBCounters(struct ath_hal *);
extern  void ar5416AniAttach(struct ath_hal *);
extern  void ar5416AniDetach(struct ath_hal *);
extern  struct ar5416AniState *ar5416AniGetCurrentState(struct ath_hal *);
extern  struct ar5416Stats *ar5416AniGetCurrentStats(struct ath_hal *);
extern  HAL_BOOL ar5416AniControl(struct ath_hal *, HAL_ANI_CMD cmd, int param);
struct ath_rx_status;
extern  void ar5416AniPhyErrReport(struct ath_hal *ah,
        const struct ath_rx_status *rs);
extern  void ar5416ProcessMibIntr(struct ath_hal *, const HAL_NODE_STATS *);
extern  void ar5416AniArPoll(struct ath_hal *, const HAL_NODE_STATS *,
                 HAL_CHANNEL *);
extern  void ar5416AniReset(struct ath_hal *);

extern  void ar5416ResetAR(struct ath_hal *ah);
extern  void ar5416ArEnable(struct ath_hal *ah);
extern  HAL_STATUS ar5416RadarAttach(struct ath_hal *ah);
extern  void ar5416RadarDetach(struct ath_hal *ah);
extern  void ar5416RadarEnable(struct ath_hal *ah);
extern  void ar5416ArDisable(struct ath_hal *ah);
extern  void ar5416RadarDisable(struct ath_hal *ah);
extern  HAL_BOOL ar5416DeQueueRadarEvent(struct ar5416RadarQElem *q,
        struct ar5416RadarQInfo *qInfo, struct ar5416RadarEvent *radarEvent,
        HAL_BOOL *flush);
extern  struct ar5416RadarState *ar5416GetRadarChanState(struct ath_hal *ah,
                             u_int8_t *index);
extern  void ar5416ResetRadar(struct ath_hal *ah);
extern  void ar5416ResetAr(struct ath_hal *ah);
extern  void ar5416ProcessRadar(struct ath_hal *ah, struct ath_desc *ds, 
                                struct ath_rx_status *rx_status);
extern  void ar5416ProcessArEvent(struct ath_hal *ah, HAL_CHANNEL *chan);
extern  HAL_BOOL ar5416Reset11n(struct ath_hal *ah, HAL_OPMODE opmode,
        HAL_CHANNEL *chan, HAL_HT *ht, HAL_BOOL bChannelChange, HAL_STATUS *status);
extern void ar5416SetCoverageClass(struct ath_hal *ah, u_int8_t coverageclass, int now);

extern int ar5416Get11nHwPlatform(struct ath_hal *ah);
extern void ar5416GetChannelCenters(struct ath_hal *ah, HAL_HT *ht,
                                    HAL_CHANNEL_INTERNAL *chan,
                                    CHAN_CENTERS *centers);
extern u_int16_t ar5416GetCtlCenter(struct ath_hal *ah, HAL_HT *ht,
                                        HAL_CHANNEL_INTERNAL *chan);
extern u_int16_t ar5416GetExtCenter(struct ath_hal *ah, HAL_HT *ht,
                                        HAL_CHANNEL_INTERNAL *chan);
extern  u_int32_t ar5416GetCycleCounts(struct ath_hal *, u_int32_t *,
        u_int32_t *, u_int32_t *);
extern  void ar5416DmaRegDump(struct ath_hal *);


#ifdef AR5416_EMULATION
/* XXX - AR5416 Emulation only. Remove when emulation complete */
extern u_int32_t ar5416GetEmu(struct ath_hal *ah);
extern void ar5416SetEmu(struct ath_hal *ah, u_int32_t value);

extern void ar5416InitMacTrace(struct ath_hal *ah);
extern void ar5416StopMacTrace(struct ath_hal *ah);
#endif

#endif  /* _ATH_AR5416_H_ */
