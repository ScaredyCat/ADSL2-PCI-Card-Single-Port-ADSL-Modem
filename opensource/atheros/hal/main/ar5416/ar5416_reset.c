/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_reset.c#81 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_xr.h"
#include "ah_internal.h"
#include "ah_devid.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"

/* Add static register initialization vectors */
#define AH_5416_COMMON
#include "ar5416/ar5416.ini"

#define RIFS_EN 0   /* enable RIFS */

/* Additional Time delay to wait after activiting the Base band */
#define BASE_ACTIVATE_DELAY         100     /* 100 usec */
#define RTC_PLL_SETTLE_DELAY        1000    /* 1 ms     */
#define COEF_SCALE_S                24
#define HT40_CHANNEL_CENTER_SHIFT   10      /* MHz      */

#define NUM_NOISEFLOOR_READINGS 6       /* 3 chains * (ctl + ext) */

#ifdef AR5416_EMULATION
#define EMU_HAINAN_BASE_ACTIVATE_DELAY  1000  //1  msec (up from 100 usec for non-emulation)
#define EMU_HAINAN_PLL_SETTLE_DELAY     10000 //10 msec (up from 300 usec for non-emulation)
#endif

static inline HAL_BOOL ar5416ResetCommon(struct ath_hal *ah, HAL_OPMODE opmode,
        HAL_CHANNEL *chan, HAL_HT *ht, HAL_BOOL bChannelChange, HAL_STATUS *status);
static void ar5416Set11nRegs(struct ath_hal *ah, HAL_HT *ht);

static inline HAL_BOOL ar5416SetResetPowerOn(struct ath_hal *ah, 
                                             HAL_CHANNEL *chan);
static inline HAL_BOOL ar5416SetReset(struct ath_hal *ah, int type, 
                                      HAL_CHANNEL *chan);

int16_t         ar5416GetNf(struct ath_hal *, HAL_CHANNEL_INTERNAL *);
void            ar5416SetDeltaSlope(struct ath_hal *, HAL_CHANNEL_INTERNAL *,
                HAL_HT *ht);
#if 0
HAL_BOOL        ar5416SetTransmitPower(struct ath_hal *ah,
                                       HAL_CHANNEL_INTERNAL *chan);
static HAL_BOOL ar5416SetRateTable(struct ath_hal *,
                HAL_CHANNEL *, int16_t tpcScaleReduction, int16_t powerLimit,
                int16_t *minPower, int16_t *maxPower);
static void ar5416GetTargetPowers(struct ath_hal *, HAL_CHANNEL *,
                TRGT_POWER_INFO *pPowerInfo, u_int16_t numChannels,
                TRGT_POWER_INFO *pNewPower);
static u_int16_t ar5416GetMaxEdgePower(u_int16_t channel,
                RD_EDGES_POWER  *pRdEdgesPower);
#endif
static inline HAL_CHANNEL_INTERNAL* ar5416CheckChan(struct ath_hal *ah,
                                                    HAL_CHANNEL *chan,
						    HAL_HT *ht);
static int ar5416ValidateHt(HAL_CHANNEL *chan, HAL_HT *ht);
static inline HAL_STATUS ar5416ProcessIni(struct ath_hal *ah, HAL_HT *ht,
                                          HAL_CHANNEL *chan,
                                          HAL_CHANNEL_INTERNAL *ichan);
static inline void ar5416SetRfMode(struct ath_hal *ah, HAL_CHANNEL *chan);
static inline void ar5416GetDeltaSlopeValues(struct ath_hal *ah,
                                             u_int32_t coef_scaled,
                                             u_int32_t *coef_mantissa,
                                             u_int32_t *coef_exponent);

/* NB: public for RF backend use */
void    ar5416GetLowerUpperValues(u_int16_t value,
                u_int16_t *pList, u_int16_t listSize,
                u_int16_t *pLowerValue, u_int16_t *pUpperValue);
void    ar5416ModifyRfBuffer(u_int32_t *rfBuf, u_int32_t reg32,
                u_int32_t numBits, u_int32_t firstBit, u_int32_t column);

static inline HAL_BOOL ar5416ChannelChange(struct ath_hal *ah,
                                           HAL_CHANNEL *chan,
                                           HAL_CHANNEL_INTERNAL *ichan,
                                           HAL_HT *ht);

static inline void ar5416OverrideIniOwl2(struct ath_hal *ah);
static inline void ar5416InitHainan(struct ath_hal *ah);
static inline void ar5416SetupHainan(struct ath_hal *ah);
static inline void ar5416InitPLL(struct ath_hal *ah, HAL_CHANNEL *chan);
static inline void ar5416InitChainMasks(struct ath_hal *ah, HAL_HT *ht);
static inline HAL_BOOL ar5416InitCal(struct ath_hal *ah, HAL_HT *ht);
static inline HAL_BOOL ar5416NfCalInitHainan(struct ath_hal *ah, HAL_HT *ht);
static inline void ar5416SetDma(struct ath_hal *ah);
static inline void ar5416InitBB(struct ath_hal *ah, HAL_CHANNEL *chan);
static inline void ar5416InitInterruptMasks(struct ath_hal *ah,
                                            HAL_OPMODE opmode);
static inline void ar5416InitQOS(struct ath_hal *ah);
static inline void ar5416InitUserSettings(struct ath_hal *ah);
static inline void ar5416AttachHwPlatform(struct ath_hal *ah);
static inline HAL_BOOL ar5416AllowChannelChange(struct ath_hal *ah, 
                                                HAL_BOOL bChannelChange, 
                                                HAL_HT *ht, HAL_CHANNEL *chan);

/*
 * WAR for bug 6773.  OS_DELAY() does a PIO READ on the PCI bus which allows
 * other cards' DMA reads to complete in the middle of our reset.
 */
#define WAR_6773(x) do {                \
        if ((++(x) % 64) == 0)          \
                OS_DELAY(1);            \
} while (0)

#define REG_WRITE_ARRAY(regArray, column, regWr) do {                   \
        int r;                                                          \
        for (r = 0; r < sizeof(regArray)/sizeof(regArray[0]); r++) {    \
                OS_REG_WRITE(ah, (regArray)[r][0], (regArray)[r][(column)]);\
                WAR_6773(regWr);                                        \
        }                                                               \
} while (0)

#define ar5416CheckOpMode(_opmode) \
    ((_opmode == HAL_M_STA) || (_opmode == HAL_M_IBSS) ||\
     (_opmode == HAL_M_HOSTAP) || (_opmode == HAL_M_MONITOR))

#define IS_CHAN_SAME_BAND(_a, _b)               \
    ((IS_CHAN_2GHZ(_a) && IS_CHAN_2GHZ(_b)) ||  \
     (IS_CHAN_5GHZ(_a) && IS_CHAN_5GHZ(_b)))

HAL_BOOL
ar5416Reset(struct ath_hal *ah, HAL_OPMODE opmode,
        HAL_CHANNEL *chan, HAL_BOOL bChannelChange, HAL_STATUS *status)
{
    return ar5416ResetCommon(ah, opmode, chan, AH_NULL, bChannelChange, status);
}

HAL_BOOL
ar5416Reset11n(struct ath_hal *ah, HAL_OPMODE opmode,
        HAL_CHANNEL *chan, HAL_HT *ht, HAL_BOOL bChannelChange, HAL_STATUS *status)
{
   int         attempts = 0;
   HAL_BOOL    res;
#define RESET_CAL_ATTEMPTS  100

    do {
        res = ar5416ResetCommon(ah, opmode, chan, ht, bChannelChange, status);

        /* force full reset */
        bChannelChange = AH_FALSE;
        attempts++;
    } while ((res == AH_FALSE) && attempts < RESET_CAL_ATTEMPTS);

    return res;
}

/*
 * Places the device in and out of reset and then places sane
 * values in the registers based on EEPROM config, initialization
 * vectors (as determined by the mode), and station configuration
 *
 * bChannelChange is used to preserve DMA/PCU registers across
 * a HW Reset during channel change.
 */
static inline HAL_BOOL
ar5416ResetCommon(struct ath_hal *ah, HAL_OPMODE opmode,
        HAL_CHANNEL *chan, HAL_HT *ht, HAL_BOOL bChannelChange, HAL_STATUS *status)
{
#define FAIL(_code)     do { ecode = _code; goto bad; } while (0)
        u_int32_t saveLedState, softLedCfg;
        struct ath_hal_5416 *ahp = AH5416(ah);
        HAL_CHANNEL_INTERNAL *ichan;
        u_int32_t saveDefAntenna;
        u_int32_t macStaId1;
        HAL_STATUS ecode;
        int i, rx_chainmask;
#ifdef ATH_FORCE_PPM
        u_int32_t saveForceVal, tmpReg;
#endif
        HAL_BOOL cc_allowed;

        HALASSERT(ahp->ah_eeversion >= AR_EEPROM_VER3);
        HALASSERT(ar5416CheckOpMode(opmode));

        OS_MARK(ah, AH_MARK_RESET, bChannelChange);

        /*
         * Map public channel to private.
         */
        ichan = ar5416CheckChan(ah, chan, ht);
        if (ichan == AH_NULL) {
                HALDEBUG(ah, "%s: invalid channel %u/0x%x; no mapping\n",
                        __func__, chan->channel, chan->channelFlags);
                FAIL(HAL_EINVAL);
        }

        cc_allowed = ar5416AllowChannelChange(ah, bChannelChange, ht, chan);

        if (ht) {
            /* update HAL HT state */
            ahp->ah_htcwm = ht->cwm;
        }

        /* Bring out of sleep mode */
        if (!ar5416SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE))
            return AH_FALSE;

        if (cc_allowed) {
            if (ar5416ChannelChange(ah, chan, ichan, ht)) {
                chan->channelFlags = ichan->channelFlags;
                chan->privFlags = ichan->privFlags;
                /* If ChannelChange completed - skip the rest of reset */
                return AH_TRUE;
            }
        }

#ifdef ATH_FORCE_PPM
        /* Preserve force ppm state */
        saveForceVal = OS_REG_READ(ah, AR_PHY_TIMING2) &
                            (AR_PHY_TIMING2_USE_FORCE | AR_PHY_TIMING2_FORCE_VAL);
#endif

        /*
         * Preserve the antenna on a channel change
         */
        saveDefAntenna = OS_REG_READ(ah, AR_DEF_ANTENNA);
        if (saveDefAntenna == 0)
            saveDefAntenna = 1;

        /* Save hardware flag before chip reset clears the register */
        macStaId1 = OS_REG_READ(ah, AR_STA_ID1) & AR_STA_ID1_BASE_RATE_11B;

        /* Save led state from pci config register */
        saveLedState = OS_REG_READ(ah, AR_MAC_LED) &
                (AR_MAC_LED_ASSOC_CTL | AR_MAC_LED_MODE_SEL |
                 AR_MAC_LED_BLINK_THRESH_SEL | AR_MAC_LED_BLINK_SLOW);
        softLedCfg = OS_REG_READ(ah, AR_GPIO_INTR_OUT);

        if (!ar5416ChipReset(ah, chan)) {
                HALDEBUG(ah, "%s: chip reset failed\n", __func__);
                FAIL(HAL_EIO);
        }

        OS_MARK(ah, AH_MARK_RESET_LINE, __LINE__);

        /*
         * Note that ar5416InitChainMasks() is called from within ar5416ProcessIni()
         * to ensure the swap bit is set before the pdadc table is written.
         */

        ecode = ar5416ProcessIni(ah, ht, chan, ichan);
        if (ecode != HAL_OK)
            goto bad;

        /* Write delta slope for OFDM enabled modes (A, G, Turbo) */
        if (IS_CHAN_OFDM(chan)|| IS_CHAN_HT(chan))
                ar5416SetDeltaSlope(ah, ichan, ht);

        if (!ar5416EepromSetBoardValues(ah, ht, ichan)) {
            HALDEBUG(ah, "%s: error setting board options\n", __func__);
            FAIL(HAL_EIO);
        }

        OS_MARK(ah, AH_MARK_RESET_LINE, __LINE__);

        OS_REG_WRITE(ah, AR_STA_ID0, LE_READ_4(ahp->ah_macaddr));
        OS_REG_WRITE(ah, AR_STA_ID1, LE_READ_2(ahp->ah_macaddr + 4)
                | macStaId1
                | AR_STA_ID1_RTS_USE_DEF
                | (ath_hal_6mb_ack ? AR_STA_ID1_ACKCTS_6MB : 0)
                | ahp->ah_staId1Defaults
        );
        ar5416SetOperatingMode(ah, opmode);

        /* Set Venice BSSID mask according to current state */
        OS_REG_WRITE(ah, AR_BSSMSKL, LE_READ_4(ahp->ah_bssidmask));
        OS_REG_WRITE(ah, AR_BSSMSKU, LE_READ_2(ahp->ah_bssidmask + 4));

        /* Restore previous led state */
        OS_REG_WRITE(ah, AR_MAC_LED, OS_REG_READ(ah, AR_MAC_LED) | saveLedState);

        /* Restore soft Led state to GPIO */
        OS_REG_WRITE(ah, AR_GPIO_INTR_OUT, softLedCfg);

        /* Restore previous antenna */
        OS_REG_WRITE(ah, AR_DEF_ANTENNA, saveDefAntenna);

#ifdef ATH_FORCE_PPM
        /* Restore force ppm state */
        tmpReg = OS_REG_READ(ah, AR_PHY_TIMING2) &
                    ~(AR_PHY_TIMING2_USE_FORCE | AR_PHY_TIMING2_FORCE_VAL);
        OS_REG_WRITE(ah, AR_PHY_TIMING2, tmpReg | saveForceVal);
#endif

        /* then our BSSID */
        OS_REG_WRITE(ah, AR_BSS_ID0, LE_READ_4(ahp->ah_bssid));
        OS_REG_WRITE(ah, AR_BSS_ID1, LE_READ_2(ahp->ah_bssid + 4));

        OS_REG_WRITE(ah, AR_ISR, ~0);           /* cleared on write */

        OS_REG_WRITE(ah, AR_RSSI_THR, INIT_RSSI_THR);

        /*
         * Set Channel now modifies bank 6 parameters for FOWL workaround to force
         * rf_pwd_icsyndiv bias current as function of synth frequency.
         * Thus must be called after ar5416ProcessIni() to ensure analog register 
         * cache is valid.
         */
        if (!ahp->ah_rfHal.setChannel(ah, ichan, ht)) {
            FAIL(HAL_EIO);
        }

        OS_MARK(ah, AH_MARK_RESET_LINE, __LINE__);

        /* Set 1:1 QCU to DCU mapping for all queues */
        for (i = 0; i < AR_NUM_DCU; i++)
                OS_REG_WRITE(ah, AR_DQCUMASK(i), 1 << i);

        ahp->ah_intrTxqs = 0;
        for (i = 0; i < AH_PRIVATE(ah)->ah_caps.halTotalQueues; i++)
                ar5416ResetTxQueue(ah, i);

        ar5416InitInterruptMasks(ah, opmode);

        if (ar5416GetRfKill(ah))
            ar5416EnableRfKill(ah);

        ar5416InitQOS(ah);

        ar5416InitUserSettings(ah);

        AH_PRIVATE(ah)->ah_opmode = opmode;     /* record operating mode */

        HALDEBUG(ah, "%s: done\n", __func__);

        OS_MARK(ah, AH_MARK_RESET_DONE, 0);

        /*
         * For mac to mac emulation, turn on rate throttling
         * but not half-rate (encryption)
         */
        if (ar5416Get11nHwPlatform(ah) == HAL_MAC_TO_MAC_EMU) {
            OS_REG_WRITE(ah, AR_EMU, AR_EMU_RATETHROT);
        }

        /*
         * disable seq number generation in hw
         */
        OS_REG_WRITE(ah, AR_STA_ID1,
                     OS_REG_READ(ah, AR_STA_ID1) | AR_STA_ID1_PRESERVE_SEQNUM);

        ar5416SetDma(ah);

        /*
         * program OBS bus to see MAC interrupts
         */
        OS_REG_WRITE(ah, AR_OBS, 8);

        /*
         * configure MAC logic analyzer (emulation only)
         */
        if (IS_5416_EMU(ah)) {
            ar5416InitMacTrace(ah);
        }

        /*
         * GTT debug mode setting
         */
        // OS_REG_WRITE(ah, 0x64, 0x00320000);
        // OS_REG_WRITE(ah, 0x68, 7);
        // OS_REG_WRITE(ah, 0x4080, 0xC);

#ifdef AR5416_INT_MITIGATION
        OS_REG_WRITE(ah, AR_MIRT, 0);
        OS_REG_RMW_FIELD(ah, AR_RIMT, AR_RIMT_LAST, 500);
        OS_REG_RMW_FIELD(ah, AR_RIMT, AR_RIMT_FIRST, 2000);
#endif

        ar5416InitBB(ah, chan);

        if (!ar5416InitCal(ah, ht))
            FAIL(HAL_ESELFTEST);

        /*
         * WAR for owl 1.0 - restore chain mask for 2-chain cfgs after cal
         */
        rx_chainmask = (ht) ? ht->misc.ht_rxchainmask : AR5416_LEGACY_CHAINMASK;
        if ((rx_chainmask == 0x5) || (rx_chainmask == 0x3)) {
            OS_REG_WRITE(ah, AR_PHY_RX_CHAINMASK, rx_chainmask);
            OS_REG_WRITE(ah, AR_PHY_CAL_CHAINMASK, rx_chainmask);
        }

        /*
         * bring out rx clear on the AP71.
         */
        OS_REG_WRITE(ah, AR_GPIO_INTR_OUT, 0xf);
        OS_REG_WRITE(ah, AR_GPIO_OUTPUT_MUX1, ((4 << 5) | 3));

        /*
         * For big endian systems turn on swapping for descriptors
         */
#if AH_BYTE_ORDER == AH_BIG_ENDIAN
        OS_REG_WRITE(ah, AR_CFG, AR_CFG_SWTD | AR_CFG_SWRD);
#endif

        return AH_TRUE;
bad:
        OS_MARK(ah, AH_MARK_RESET_DONE, ecode);
        if (*status)
                *status = ecode;
        return AH_FALSE;
#undef FAIL
#undef N
}

/**************************************************************
 * ar5416ChannelChange
 * Assumes caller wants to change channel, and not reset.
 */
static inline HAL_BOOL
ar5416ChannelChange(struct ath_hal *ah, HAL_CHANNEL *chan,
                    HAL_CHANNEL_INTERNAL *ichan, HAL_HT *ht)
{
    u_int32_t synthDelay, qnum;
    struct ath_hal_5416 *ahp = AH5416(ah);

    ar5416GetNf(ah, ichan);

    /* TX must be stopped by now */
    for (qnum = 0; qnum < AR_NUM_QCU; qnum++) {
        if (ar5416NumTxPending(ah, qnum)) {
            HALASSERT(0);
            return AH_FALSE;
        }
    }

    /*
     * Kill last Baseband Rx Frame - Request analog bus grant
     */
    if (ar5416Get11nHwPlatform(ah) != HAL_MAC_TO_MAC_EMU) {
        OS_REG_WRITE(ah, AR_PHY_RFBUS_REQ, AR_PHY_RFBUS_REQ_EN);
        if (!ath_hal_wait(ah, AR_PHY_RFBUS_GRANT, AR_PHY_RFBUS_GRANT_EN,
                          AR_PHY_RFBUS_GRANT_EN))
            return AH_FALSE;
    }

    /* Setup 11n MAC/Phy mode registers */
    ar5416Set11nRegs(ah, ht);

    /*
     * Change the synth
     */
    if (!ahp->ah_rfHal.setChannel(ah, ichan, ht))
        return AH_FALSE;

    /*
     * Setup the transmit power values.
     *
     * After the public to private hal channel mapping, ichan contains the
     * valid regulatory power value.
     * ath_hal_getctl and ath_hal_getantennaallowed look up ichan from chan.
     */
    if (ar5416EepromSetTransmitPower(ah, ht, &ahp->ah_eeprom, ichan,
         ath_hal_getctl(ah,chan), ath_hal_getantennaallowed(ah, chan),
         ichan->maxRegTxPower * 2,
         AH_MIN(MAX_RATE_POWER, AH_PRIVATE(ah)->ah_powerLimit)) != HAL_OK) {
        HALDEBUG(ah, "%s: error init'ing transmit power\n", __func__);
        return AH_FALSE;
    }

    /*
     * Wait for the frequency synth to settle (synth goes on via PHY_ACTIVE_EN).
     * Read the phy active delay register. Value is in 100ns increments.
     */
    synthDelay = OS_REG_READ(ah, AR_PHY_RX_DELAY) & AR_PHY_RX_DELAY_DELAY;
    if (IS_CHAN_CCK(chan)) {
        synthDelay = (4 * synthDelay) / 22;
    } else {
        synthDelay /= 10;
    }

    if (ar5416Get11nHwPlatform(ah) == HAL_MAC_BB_EMU) {
        OS_DELAY(synthDelay + EMU_HAINAN_BASE_ACTIVATE_DELAY);
    } else {
        OS_DELAY(synthDelay + BASE_ACTIVATE_DELAY);
    }

    if (!ichan->oneTimeCalsDone) {
        /*
         * Start offset and carrier leak cals
         */
    }

    /*
     * Write spur immunity and delta slope for OFDM enabled modes (A, G, Turbo)
     */
    if (IS_CHAN_OFDM(chan)|| IS_CHAN_HT(chan)) {
        //ar5416SetSpurMitigation(pDev, pChval);
        ar5416SetDeltaSlope(ah, ichan, ht);
    }

    /*
     * Start Noise Floor Cal
     */
    OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);

    if (!ichan->oneTimeCalsDone) {
        /*
         * wait for end of offset and carrier leak cals
         */
        ichan->oneTimeCalsDone = AH_TRUE;
    }

    /*
     * Release the RFBus Grant
     */
    if (ar5416Get11nHwPlatform(ah) != HAL_MAC_TO_MAC_EMU)
        OS_REG_WRITE(ah, AR_PHY_RFBUS_REQ, 0);

#if 0
    if (chan->channelFlags & CHANNEL_108G)
        ar5416ArEnable(ah);
    else if ((chan->channelFlags &
             (CHANNEL_A|CHANNEL_ST|CHANNEL_A_HT20| CHANNEL_A_HT40))
             && (chan->privFlags & CHANNEL_DFS))
        ar5416RadarEnable(ah);
#endif

    return AH_TRUE;
}

static void
ar5416Set11nRegs(struct ath_hal *ah, HAL_HT *ht)
{
    u_int32_t phymode;

    if (ht == AH_NULL) {
        return;
    }

    /* Enable 11n HT, 20 MHz */
    phymode = AR_PHY_FC_HT_EN | AR_PHY_FC_SHORT_GI_40
              | AR_PHY_FC_SINGLE_HT_LTF1 | AR_PHY_FC_WALSH;

    /* Configure baseband for dynamic 20/40 operation */
    if (ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
        phymode |= AR_PHY_FC_DYN2040_EN;

        /* Configure control (primary) channel at +-10MHz */
        if (ht->cwm.ht_extoff == 1) {
            phymode |= AR_PHY_FC_DYN2040_PRI_CH;
        }

        /* Configure 20/25 spacing */
        if (ht->cwm.ht_extprotspacing == HAL_HT_EXTPROTSPACING_25) {
            phymode |= AR_PHY_FC_DYN2040_EXT_CH;
        }
    }
    OS_REG_WRITE(ah, AR_PHY_TURBO, phymode);

#if RIFS_EN
    /* RIFS support only in Owl 2.0 */
    if (ah->ah_macRev >= AR_SREV_REVISION_OWL_20) {
        /* RIFS. reg 70: search_start_delay, in CLKs */
        if (ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
                OS_REG_WRITE(ah, 0x9800+(70<<2), 40);
        } else {
                OS_REG_WRITE(ah, 0x9800+(70<<2), 20);
        }
    }
#endif

    /* Configure MAC for 20/40 operation */
    ar5416Set11nMac2040(ah, ht->cwm.ht_macmode);

    /* global transmit timeout (25 TUs default)*/
    /* XXX - put this elsewhere??? */
    OS_REG_WRITE(ah, AR_GTXTO, 25 << AR_GTXTO_TIMEOUT_LIMIT_S) ;

    /* carrier sense timeout */
    OS_REG_WRITE(ah, AR_CST, 0xF << AR_CST_TIMEOUT_LIMIT_S);
}

void
ar5416SetOperatingMode(struct ath_hal *ah, int opmode)
{
        u_int32_t val;

        val = OS_REG_READ(ah, AR_STA_ID1);
        val &= ~(AR_STA_ID1_STA_AP | AR_STA_ID1_ADHOC);
        switch (opmode) {
        case HAL_M_HOSTAP:
                OS_REG_WRITE(ah, AR_STA_ID1, val | AR_STA_ID1_STA_AP
                                        | AR_STA_ID1_KSRCH_MODE);
                OS_REG_CLR_BIT(ah, AR_CFG, AR_CFG_AP_ADHOC_INDICATION);
                break;
        case HAL_M_IBSS:
                OS_REG_WRITE(ah, AR_STA_ID1, val | AR_STA_ID1_ADHOC
                                        | AR_STA_ID1_KSRCH_MODE);
                OS_REG_SET_BIT(ah, AR_CFG, AR_CFG_AP_ADHOC_INDICATION);
                break;
        case HAL_M_STA:
        case HAL_M_MONITOR:
                OS_REG_WRITE(ah, AR_STA_ID1, val | AR_STA_ID1_KSRCH_MODE);
                break;
        }
}

/*
 * Places the PHY and Radio chips into reset.  A full reset
 * must be called to leave this state.  The PCI/MAC/PCU are
 * not placed into reset as we must receive interrupt to
 * re-enable the hardware.
 */
HAL_BOOL
ar5416PhyDisable(struct ath_hal *ah)
{
    return ar5416SetResetReg(ah, HAL_RESET_WARM, AH_NULL);
}

/*
 * Places all of hardware into reset
 */
HAL_BOOL
ar5416Disable(struct ath_hal *ah)
{
    if (!ar5416SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE))
        return AH_FALSE;

    return ar5416SetResetReg(ah, HAL_RESET_COLD, AH_NULL);
}

/*
 * Places the hardware into reset and then pulls it out of reset
 */
HAL_BOOL
ar5416ChipReset(struct ath_hal *ah, HAL_CHANNEL *chan)
{

        OS_MARK(ah, AH_MARK_CHIPRESET, chan ? chan->channel : 0);

        /*
         * Warm reset is optimistic.
         */
        if (!ar5416SetResetReg(ah, HAL_RESET_WARM, chan))
                return AH_FALSE;

        /* Bring out of sleep mode (AGAIN) */
        if (!ar5416SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE))
                return AH_FALSE;

        /*
         * Perform warm reset before the mode/PLL/turbo registers
         * are changed in order to deactivate the radio.  Mode changes
         * with an active radio can result in corrupted shifts to the
         * radio device.
         */
        ar5416SetRfMode(ah, chan);

        return AH_TRUE;
}

/*
 * Recalibrate the lower PHY chips to account for temperature/environment
 * changes.
 */
HAL_BOOL
ar5416PerCalibration(struct ath_hal *ah,  HAL_CHANNEL *chan)
{
#define IQ_CAL_TRIES    10
        struct ath_hal_5416 *ahp = AH5416(ah);
        HAL_CHANNEL_INTERNAL *ichan;
        int32_t qCoff, qCoffDenom;
        int32_t iqCorrMeas, iCoff, iCoffDenom;
        u_int32_t powerMeasQ, powerMeasI;

        OS_MARK(ah, AH_MARK_PERCAL, chan->channel);
        ichan = ath_hal_checkchannel(ah, chan);
        if (ichan == AH_NULL) {
                HALDEBUG(ah, "%s: invalid channel %u/0x%x; no mapping\n",
                        __func__, chan->channel, chan->channelFlags);
                return AH_FALSE;
        }

        /* XXX EAR */

        /* IQ calibration in progress. Check to see if it has finished. */
        if (ahp->ah_bIQCalibration == IQ_CAL_RUNNING &&
            !(OS_REG_READ(ah, AR_PHY_TIMING_CTRL4) & AR_PHY_TIMING_CTRL4_DO_IQCAL)) {
                int i;

                /* IQ Calibration has finished. */
                ahp->ah_bIQCalibration = IQ_CAL_INACTIVE;

                /* workaround for misgated IQ Cal results */
                for (i = 0; i < IQ_CAL_TRIES; i++) {
                        /* Read calibration results. */
                        powerMeasI = OS_REG_READ(ah, AR_PHY_IQCAL_RES_PWR_MEAS_I);
                        powerMeasQ = OS_REG_READ(ah, AR_PHY_IQCAL_RES_PWR_MEAS_Q);
                        iqCorrMeas = OS_REG_READ(ah, AR_PHY_IQCAL_RES_IQ_CORR_MEAS);
                        if (powerMeasI && powerMeasQ)
                                break;
                        /* Do we really need this??? */
                        OS_REG_WRITE (ah,  AR_PHY_TIMING_CTRL4,
                                      OS_REG_READ(ah,  AR_PHY_TIMING_CTRL4) |
                                      AR_PHY_TIMING_CTRL4_DO_IQCAL);
                }

                /*
                 * Prescale these values to remove 64-bit operation
                 * requirement at the loss of a little precision.
                 */
                iCoffDenom = (powerMeasI / 2 + powerMeasQ / 2) / 128;
                qCoffDenom = powerMeasQ / 128;

                /* Protect against divide-by-0 and loss of sign bits. */
                if (iCoffDenom != 0 && qCoffDenom >= 2) {
                        iCoff = (int8_t)(-iqCorrMeas) / iCoffDenom;
                        /* IQCORR_Q_I_COFF is a signed 6 bit number */
                        if (iCoff < -32) {
                                iCoff = -32;
                        } else if (iCoff > 31) {
                                iCoff = 31;
                        }

                        /* IQCORR_Q_Q_COFF is a signed 5 bit number */
                        qCoff = (powerMeasI / qCoffDenom) - 128;
                        if (qCoff < -16) {
                                qCoff = -16;
                        } else if (qCoff > 15) {
                                qCoff = 15;
                        }
#ifdef CALIBRATION_DEBUG
                        HALDEBUG(ah, "****************** MISGATED IQ CAL! *******************\n");
                        HALDEBUG(ah, "time       = %d, i = %d, \n",
                                OS_GETUPTIME(ah), i);
                        HALDEBUG(ah, "powerMeasI = 0x%08x\n", powerMeasI);
                        HALDEBUG(ah, "powerMeasQ = 0x%08x\n", powerMeasQ);
                        HALDEBUG(ah, "iqCorrMeas = 0x%08x\n", iqCorrMeas);
                        HALDEBUG(ah, "iCoff      = %d\n", iCoff);
                        HALDEBUG(ah, "qCoff      = %d\n", qCoff);
#endif
                        /* Write values and enable correction */
                        OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
                                AR_PHY_TIMING_CTRL4_IQCORR_Q_I_COFF, iCoff);
                        OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
                                AR_PHY_TIMING_CTRL4_IQCORR_Q_Q_COFF, qCoff);
                        OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4,
                                AR_PHY_TIMING_CTRL4_IQCORR_ENABLE);

                        ahp->ah_bIQCalibration = IQ_CAL_DONE;
                        ichan->iqCalValid = AH_TRUE;
                        ichan->iCoff = iCoff;
                        ichan->qCoff = qCoff;
                }
        } else if (!IS_CHAN_B(chan) &&
                   ahp->ah_bIQCalibration == IQ_CAL_DONE &&
                   !ichan->iqCalValid) {
                /*
                 * Start IQ calibration if configured channel has changed.
                 * Use a magic number of 15 based on default value.
                 */
                OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
                        AR_PHY_TIMING_CTRL4_IQCAL_LOG_COUNT_MAX,
                        INIT_IQCAL_LOG_COUNT_MAX);
                OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4,
                        AR_PHY_TIMING_CTRL4_DO_IQCAL);
                ahp->ah_bIQCalibration = IQ_CAL_RUNNING;
        }
        /* XXX EAR */

        /* Check noise floor results */
        ar5416GetNf(ah, ichan);
        if ((ichan->channelFlags & CHANNEL_CW_INT) == 0) {
                /* run noise floor calibration */
                OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);
        } else {
                /* report up and clear internal state */
                chan->channelFlags |= CHANNEL_CW_INT;
                ichan->channelFlags &= ~CHANNEL_CW_INT;
        }
        return AH_TRUE;
#undef IQ_CAL_TRIES
}

/*
 * Write the given reset bit mask into the reset register
 */
HAL_BOOL
ar5416SetResetReg(struct ath_hal *ah, u_int32_t type, HAL_CHANNEL *chan)
{
    /*
     * Set force wake
     */
    OS_REG_WRITE(ah, AR_RTC_FORCE_WAKE,
             AR_RTC_FORCE_WAKE_EN | AR_RTC_FORCE_WAKE_ON_INT);

    switch (type) {
    case HAL_RESET_POWER_ON:
        return ar5416SetResetPowerOn(ah, chan);
        break;
    case HAL_RESET_WARM:
    case HAL_RESET_COLD:
        return ar5416SetReset(ah, type, chan);
        break;
    default:
        return AH_FALSE;
    }
}

static inline HAL_BOOL
ar5416SetResetPowerOn(struct ath_hal *ah, HAL_CHANNEL *chan)
{
    /*
     * RTC reset and clear
     */
    OS_REG_WRITE(ah, AR_RTC_RESET, 0);
    OS_REG_WRITE(ah, AR_RTC_RESET, 1);

    /*
     * Poll till RTC is ON
     */
    if (!ath_hal_wait(ah, AR_RTC_STATUS, AR_RTC_STATUS_M, AR_RTC_STATUS_ON)) {
        HALDEBUG(ah, "%s: RTC not waking up\n", __FUNCTION__);
        return AH_FALSE;
    }

    /*
     * Warm reset if we aren't really powering on,
     * just restarting the driver.
     */
    return ar5416SetReset(ah, HAL_RESET_WARM, chan);
}

#define WAR_19660

static inline HAL_BOOL
ar5416SetReset(struct ath_hal *ah, int type, HAL_CHANNEL *chan)
{
#ifdef WAR_19660
    /* (Owl 1.0) disable timers before reset.
     * Because of a hardware bug in Owl 1.0, timers are not disabled
     * on a warm reset.  See bug 19660.
     */
    if (ah->ah_macRev == AR_SREV_REVISION_OWL_10) {
        OS_REG_CLR_BIT(ah, AR_TIMER_MODE, 0xFF);
    }
#endif

    /*
     * Reset AHB
     */
    OS_REG_WRITE(ah, AR_RC, AR_RC_AHB);

    /*
     * Set Mac(BB,Phy) Warm Reset
     */
    switch (type) {
    case HAL_RESET_WARM:
            OS_REG_WRITE(ah, AR_RTC_RC, AR_RTC_RC_MAC_WARM);
            break;
        case HAL_RESET_COLD:
            OS_REG_WRITE(ah, AR_RTC_RC, AR_RTC_RC_MAC_WARM|AR_RTC_RC_MAC_COLD);
            break;
        default:
            HALASSERT(0);
            break;
    }

    /*
     * Clear resets and force wakeup
     */
    OS_REG_WRITE(ah, AR_RTC_RC, 0);
    if (!ath_hal_wait(ah, AR_RTC_RC, AR_RTC_RC_M, 0)) {
        HALDEBUG(ah, "%s: RTC stuck in MAC reset\n", __FUNCTION__);
        return AH_FALSE;
    }

    /* Clear AHB reset */
    OS_REG_WRITE(ah, AR_RC, 0);

    /*
     * Force wake
     */
    OS_REG_WRITE(ah, AR_RTC_FORCE_WAKE, AR_RTC_FORCE_WAKE_EN |
                 AR_RTC_FORCE_WAKE_ON_INT);

    ar5416AttachHwPlatform(ah);

    switch (ar5416Get11nHwPlatform(ah)) {
        case HAL_MAC_TO_MAC_EMU:
            break;
        case HAL_MAC_BB_EMU:
            ar5416InitHainan(ah);
            break;
        case HAL_TRUE_CHIP:
            ar5416InitPLL(ah, chan);
            break;
    }

    return AH_TRUE;
}

void
ar5416GetNoiseFloor(struct ath_hal *ah, 
                    u_int16_t nfarray[NUM_NOISEFLOOR_READINGS])
{
    int16_t nf;

    nf = MS(OS_REG_READ(ah, AR_PHY_CCA), AR_PHY_MINCCA_PWR);
    if (nf & 0x100)
            nf = 0 - ((nf ^ 0x1ff) + 1);
    ath_hal_printf(ah, "NF calibrated [ctl] [chain 0] is %d\n", nf);
    nfarray[0] = nf;

    nf = MS(OS_REG_READ(ah, AR_PHY_EXT_CCA), AR_PHY_EXT_MINCCA_PWR);
    if (nf & 0x100)
            nf = 0 - ((nf ^ 0x1ff) + 1);
    ath_hal_printf(ah, "NF calibrated [ext] [chain 0] is %d\n", nf);
    nfarray[1] = nf;

    nf = MS(OS_REG_READ(ah, AR_PHY_CH1_CCA), AR_PHY_CH1_MINCCA_PWR);
    if (nf & 0x100)
            nf = 0 - ((nf ^ 0x1ff) + 1);
    ath_hal_printf(ah, "NF calibrated [ctl] [chain 1] is %d\n", nf);
    nfarray[2] = nf;

    nf = MS(OS_REG_READ(ah, AR_PHY_CH1_EXT_CCA), AR_PHY_CH1_EXT_MINCCA_PWR);
    if (nf & 0x100)
            nf = 0 - ((nf ^ 0x1ff) + 1);
    ath_hal_printf(ah, "NF calibrated [ext] [chain 1] is %d\n", nf);
    nfarray[3] = nf;

    nf = MS(OS_REG_READ(ah, AR_PHY_CH2_CCA), AR_PHY_CH2_MINCCA_PWR);
    if (nf & 0x100)
            nf = 0 - ((nf ^ 0x1ff) + 1);
    ath_hal_printf(ah, "NF calibrated [ctl] [chain 2] is %d\n", nf);
    nfarray[4] = nf;

    nf = MS(OS_REG_READ(ah, AR_PHY_CH2_EXT_CCA), AR_PHY_CH2_EXT_MINCCA_PWR);
    if (nf & 0x100)
            nf = 0 - ((nf ^ 0x1ff) + 1);
    ath_hal_printf(ah, "NF calibrated [ext] [chain 2] is %d\n", nf);
    nfarray[5] = nf;
}

static HAL_BOOL
getNoiseFloorThresh(struct ath_hal *ah, const HAL_CHANNEL_INTERNAL *chan,
        int16_t *nft)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        switch (chan->channelFlags & CHANNEL_ALL_NOTURBO) {
            case CHANNEL_A:
            case CHANNEL_A_HT20:
            case CHANNEL_A_HT40:
                *nft = ar5416EepromGet(ahp, EEP_NFTHRESH_5);
                break;
            case CHANNEL_B:
            case CHANNEL_G:
            case CHANNEL_G_HT20:
            case CHANNEL_G_HT40:
                *nft = ar5416EepromGet(ahp, EEP_NFTHRESH_2);
                break;
            default:
                HALDEBUG(ah, "%s: invalid channel flags 0x%x\n",
                        __func__, chan->channelFlags);
                return AH_FALSE;
        }
        return AH_TRUE;
}

/*
 * Read the NF and check it against the noise floor threshhold
 */
int16_t
ar5416GetNf(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
    int16_t nf, nfThresh;
    u_int16_t nfarray[NUM_NOISEFLOOR_READINGS]= {0};

    if (OS_REG_READ(ah, AR_PHY_AGC_CONTROL) & AR_PHY_AGC_CONTROL_NF) {
        HALDEBUG(ah, "%s: NF did not complete in calibration window\n",
                __func__);
        nf = 0;
    } else {
        /* Finished NF cal, check against threshold */
        ar5416GetNoiseFloor(ah, nfarray);
        /* TODO - enhance for multiple chains and ext ch */
        nf = nfarray[0];
        if (!IS_5416_EMU(ah) && getNoiseFloorThresh(ah, chan, &nfThresh) && 
            nf > nfThresh) {
            HALDEBUG(ah, "%s: noise floor failed detected; "
                    "detected %u, threshold %u\n", __func__,
                    nf, nfThresh);
            /*
             * NB: Don't discriminate 2.4 vs 5Ghz, if this
             *     happens it indicates a problem regardless
             *     of the band.
             */
            chan->channelFlags |= CHANNEL_CW_INT;
        }
    }
    return (chan->rawNoiseFloor = nf);
}

#define MAX_ANALOG_START        319             /* XXX */

/*
 * Delta slope coefficient computation.
 * Required for OFDM operation.
 */
void
ar5416SetDeltaSlope(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan, HAL_HT *ht)
{
        u_int32_t coef_scaled, ds_coef_exp, ds_coef_man;
        u_int32_t clockMhzScaled = 0x64000000; /* clock * 2.5 */
        CHAN_CENTERS centers;

        if (ar5416Get11nHwPlatform(ah) == HAL_MAC_BB_EMU) {
        clockMhzScaled = 0x36000000;
        if (ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
            clockMhzScaled /= 2;
        }
    }

        /* half and quarter rate can divide the scaled clock by 2 or 4 */
        /* scale for selected channel bandwidth */
        if (ath_hal_clksel <= 2) {
                clockMhzScaled = clockMhzScaled >> ath_hal_clksel;
        }

        /*
         * ALGO -> coef = 1e8/fcarrier*fclock/40;
         * scaled coef to provide precision for this floating calculation
         */
        ar5416GetChannelCenters(ah, ht, chan, &centers);
        coef_scaled = clockMhzScaled / centers.synth_center;

        ar5416GetDeltaSlopeValues(ah, coef_scaled, &ds_coef_man, &ds_coef_exp);

        OS_REG_RMW_FIELD(ah, AR_PHY_TIMING3,
                AR_PHY_TIMING3_DSC_MAN, ds_coef_man);
        OS_REG_RMW_FIELD(ah, AR_PHY_TIMING3,
                AR_PHY_TIMING3_DSC_EXP, ds_coef_exp);

        /*
         * For Short GI,
         * scaled coeff is 9/10 that of normal coeff
         */
        coef_scaled = (9 * coef_scaled)/10;

        ar5416GetDeltaSlopeValues(ah, coef_scaled, &ds_coef_man, &ds_coef_exp);

        /* for short gi */
        OS_REG_RMW_FIELD(ah, AR_PHY_HALFGI,
                AR_PHY_HALFGI_DSC_MAN, ds_coef_man);
        OS_REG_RMW_FIELD(ah, AR_PHY_HALFGI,
                AR_PHY_HALFGI_DSC_EXP, ds_coef_exp);
}

static inline void
ar5416GetDeltaSlopeValues(struct ath_hal *ah, u_int32_t coef_scaled,
                          u_int32_t *coef_mantissa, u_int32_t *coef_exponent)
{
    u_int32_t coef_exp, coef_man;

    /*
     * ALGO -> coef_exp = 14-floor(log2(coef));
     * floor(log2(x)) is the highest set bit position
     */
    for (coef_exp = 31; coef_exp > 0; coef_exp--)
            if ((coef_scaled >> coef_exp) & 0x1)
                    break;
    /* A coef_exp of 0 is a legal bit position but an unexpected coef_exp */
    HALASSERT(coef_exp);
    coef_exp = 14 - (coef_exp - COEF_SCALE_S);

    /*
     * ALGO -> coef_man = floor(coef* 2^coef_exp+0.5);
     * The coefficient is already shifted up for scaling
     */
    coef_man = coef_scaled + (1 << (COEF_SCALE_S - coef_exp - 1));

    *coef_mantissa = coef_man >> (COEF_SCALE_S - coef_exp);
    *coef_exponent = coef_exp - 16;
}

/*
 * Set a limit on the overall output power.  Used for dynamic
 * transmit power control and the like.
 *
 * NB: limit is in units of 0.5 dbM.
 */
HAL_BOOL
ar5416SetTxPowerLimit(struct ath_hal *ah, u_int32_t limit)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
    HAL_CHANNEL_INTERNAL *ichan = ahpriv->ah_curchan;
    HAL_CHANNEL *chan = (HAL_CHANNEL *)ichan;

    ahpriv->ah_powerLimit = AH_MIN(limit, MAX_RATE_POWER);

    if (ar5416EepromSetTransmitPower(ah, AH_NULL, &ahp->ah_eeprom, ichan,
        ath_hal_getctl(ah, chan), ath_hal_getantennaallowed(ah, chan),
        chan->maxRegTxPower * 2,
        AH_MIN(MAX_RATE_POWER, ahpriv->ah_powerLimit)) != HAL_OK)
        return AH_FALSE;

    return AH_TRUE;
}

#if 0
/*
 * Set the transmit power in the baseband for the given
 * operating channel and mode.
 */
HAL_BOOL
ar5416SetTransmitPower(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
#define POW_OFDM(_r, _s)        (((0 & 1)<< ((_s)+6)) | (((_r) & 0x3f) << (_s)))
#define POW_CCK(_r, _s)         (((_r) & 0x3f) << (_s))
#define N(a)                    (sizeof (a) / sizeof (a[0]))
        static const u_int16_t tpcScaleReductionTable[5] =
                { 0, 3, 6, 9, MAX_RATE_POWER };
        struct ath_hal_5416 *ahp = AH5416(ah);
        int16_t minPower, maxPower, tpcInDb, powerLimit;
        int i;

        OS_MEMZERO(ahp->ah_ratesArray, sizeof(ahp->ah_ratesArray));

        powerLimit = AH_MIN(MAX_RATE_POWER, AH_PRIVATE(ah)->ah_powerLimit);
        if (powerLimit >= MAX_RATE_POWER || powerLimit == 0)
                tpcInDb = tpcScaleReductionTable[AH_PRIVATE(ah)->ah_tpScale];
        else
                tpcInDb = 0;
        if (!ar5416SetRateTable(ah, (HAL_CHANNEL *) chan, tpcInDb, powerLimit,
                &minPower, &maxPower)) {
                HALDEBUG(ah, "%s: unable to set rate table\n", __func__);
                return AH_FALSE;
        }

        /*
         * txPowerIndexOffset is set by the SetPowerTable() call -
         *  adjust the rate table
         */
        for (i = 0; i < N(ahp->ah_ratesArray); i++) {
                ahp->ah_ratesArray[i] += ahp->ah_txPowerIndexOffset;
                if (ahp->ah_ratesArray[i] > 63)
                        ahp->ah_ratesArray[i] = 63;
        }

        /* Write the OFDM power per rate set */
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE1,
                POW_OFDM(ahp->ah_ratesArray[3], 24)
              | POW_OFDM(ahp->ah_ratesArray[2], 16)
              | POW_OFDM(ahp->ah_ratesArray[1],  8)
              | POW_OFDM(ahp->ah_ratesArray[0],  0)
        );
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE2,
                POW_OFDM(ahp->ah_ratesArray[7], 24)
              | POW_OFDM(ahp->ah_ratesArray[6], 16)
              | POW_OFDM(ahp->ah_ratesArray[5],  8)
              | POW_OFDM(ahp->ah_ratesArray[4],  0)
        );

        /* Write the CCK power per rate set */
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE3,
                POW_CCK(ahp->ah_ratesArray[10], 24)
              | POW_CCK(ahp->ah_ratesArray[9],  16)
              | POW_CCK(ahp->ah_ratesArray[15],  8)     /* XR target power */
              | POW_CCK(ahp->ah_ratesArray[8],   0)
        );
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE4,
                POW_CCK(ahp->ah_ratesArray[14], 24)
              | POW_CCK(ahp->ah_ratesArray[13], 16)
              | POW_CCK(ahp->ah_ratesArray[12],  8)
              | POW_CCK(ahp->ah_ratesArray[11],  0)
        );

        /*
         * Set max power to 30 dBm and, optionally,
         * enable TPC in tx descriptors.
         */
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE_MAX, MAX_RATE_POWER |
                (ahp->ah_tpcEnabled ? AR_PHY_POWER_TX_RATE_MAX_TPC_ENABLE : 0));
        AH_PRIVATE(ah)->ah_maxPowerLevel = ahp->ah_ratesArray[0];

        return AH_TRUE;
#undef N
#undef POW_CCK
#undef POW_OFDM
}

/*
 * Sets the transmit power in the baseband for the given
 * operating channel and mode.
 */
static HAL_BOOL
ar5416SetRateTable(struct ath_hal *ah, HAL_CHANNEL *chan,
                   int16_t tpcScaleReduction, int16_t powerLimit,
                   int16_t *pMinPower, int16_t *pMaxPower)
{
        struct ath_hal_5416 *ahp = AH5416(ah);
        u_int16_t *rpow = ahp->ah_ratesArray;
        u_int16_t twiceMaxEdgePower = MAX_RATE_POWER;
        u_int16_t twiceMaxEdgePowerCck = MAX_RATE_POWER;
        u_int16_t twiceMaxRDPower = MAX_RATE_POWER;
        int i;
        u_int8_t cfgCtl;
        int8_t twiceAntennaGain, twiceAntennaReduction;
        RD_EDGES_POWER *rep;
        TRGT_POWER_INFO targetPowerOfdm, targetPowerCck;
        int16_t scaledPower, maxAvailPower = 0;

        twiceMaxRDPower = chan->maxRegTxPower * 2;
        *pMaxPower = -MAX_RATE_POWER;
        *pMinPower = MAX_RATE_POWER;

        /* Get conformance test limit maximum for this channel */
        cfgCtl = ath_hal_getctl(ah, chan);

        for (i = 0; i < ahp->ah_numCtls; i++) {
                u_int16_t twiceMinEdgePower;

                if (ahp->ah_ctl[i] == 0)
                        continue;
                if (ahp->ah_ctl[i] == cfgCtl ||
                    cfgCtl == ((ahp->ah_ctl[i] & CTL_MODE_M) | SD_NO_CTL)) {
                        rep = &ahp->ah_rdEdgesPower[i * NUM_EDGES];
                        twiceMinEdgePower = ar5416GetMaxEdgePower(chan->channel, rep);
                        if ((cfgCtl & ~CTL_MODE_M) == SD_NO_CTL) {
                                /* Find the minimum of all CTL edge powers that apply to this channel */
                                twiceMaxEdgePower = AH_MIN(twiceMaxEdgePower, twiceMinEdgePower);
                        } else {
                                twiceMaxEdgePower = twiceMinEdgePower;
                                break;
                        }
                }
        }

        if (IS_CHAN_G(chan)) {
                /* Check for a CCK CTL for 11G CCK powers */
                cfgCtl = (cfgCtl &~ ~CTL_MODE_M) | CTL_11B;
                for (i = 0; i < ahp->ah_numCtls; i++) {
                        u_int16_t twiceMinEdgePowerCck;

                        if (ahp->ah_ctl[i] == 0)
                                continue;
                        if (ahp->ah_ctl[i] == cfgCtl ||
                            cfgCtl == ((ahp->ah_ctl[i] & CTL_MODE_M) | SD_NO_CTL)) {
                                rep = &ahp->ah_rdEdgesPower[i * NUM_EDGES];
                                twiceMinEdgePowerCck = ar5416GetMaxEdgePower(chan->channel, rep);
                                if ((cfgCtl & ~CTL_MODE_M) == SD_NO_CTL) {
                                        /* Find the minimum of all CTL edge powers that apply to this channel */
                                        twiceMaxEdgePowerCck = AH_MIN(twiceMaxEdgePowerCck, twiceMinEdgePowerCck);
                                } else {
                                        twiceMaxEdgePowerCck = twiceMinEdgePowerCck;
                                        break;
                                }
                        }
                }
        } else {
                /* Set the 11B cck edge power to the one found before */
                twiceMaxEdgePowerCck = twiceMaxEdgePower;
        }

        /* Get Antenna Gain reduction */
        if (IS_CHAN_5GHZ(chan)) {
                twiceAntennaGain = ahp->ah_antennaGainMax[0];
        } else {
                twiceAntennaGain = ahp->ah_antennaGainMax[1];
        }
        twiceAntennaReduction =
                ath_hal_getantennareduction(ah, chan, twiceAntennaGain);

        if (IS_CHAN_OFDM(chan) || IS_CHAN_HT(chan)) {
                /* Get final OFDM target powers */
                if (IS_CHAN_2GHZ(chan)) {
                        ar5416GetTargetPowers(ah, chan, ahp->ah_trgtPwr_11g,
                                ahp->ah_numTargetPwr_11g, &targetPowerOfdm);
                } else {
                        ar5416GetTargetPowers(ah, chan, ahp->ah_trgtPwr_11a,
                                ahp->ah_numTargetPwr_11a, &targetPowerOfdm);
                }

                /* Get Maximum OFDM power */
                /* Minimum of target and edge powers */
                scaledPower = AH_MIN(twiceMaxEdgePower,
                                twiceMaxRDPower - twiceAntennaReduction);

                /*
                 * If turbo is set, reduce power to keep power
                 * consumption under 2 Watts.  Note that we always do
                 * this unless specially configured.  Then we limit
                 * power only for non-AP operation.
                 */
                if (IS_CHAN_TURBO(chan)
#ifdef AH_ENABLE_AP_SUPPORT
                    && AH_PRIVATE(ah)->ah_opmode != HAL_M_HOSTAP
#endif
                ) {
                        /*
                         * If turbo is set, reduce power to keep power
                         * consumption under 2 Watts
                         */
                        if (ahp->ah_eeversion >= AR_EEPROM_VER3_1)
                                scaledPower = AH_MIN(scaledPower,
                                        ahp->ah_turbo2WMaxPower5);
                        /*
                         * EEPROM version 4.0 added an additional
                         * constraint on 2.4GHz channels.
                         */
                        if (ahp->ah_eeversion >= AR_EEPROM_VER4_0 &&
                            IS_CHAN_2GHZ(chan))
                                scaledPower = AH_MIN(scaledPower,
                                        ahp->ah_turbo2WMaxPower2);
                }

                maxAvailPower = AH_MIN(scaledPower,
                                        targetPowerOfdm.twicePwr6_24);

                /* Reduce power by max regulatory domain allowed restrictions */
                scaledPower = maxAvailPower - (tpcScaleReduction * 2);
                scaledPower = (scaledPower < 0) ? 0 : scaledPower;
                scaledPower = AH_MIN(scaledPower, powerLimit);

                /* Set OFDM rates 9, 12, 18, 24 */
                rpow[0] = rpow[1] = rpow[2] = rpow[3] = rpow[4] = scaledPower;

                /* Set OFDM rates 36, 48, 54, XR */
                rpow[5] = AH_MIN(rpow[0], targetPowerOfdm.twicePwr36);
                rpow[6] = AH_MIN(rpow[0], targetPowerOfdm.twicePwr48);
                rpow[7] = AH_MIN(rpow[0], targetPowerOfdm.twicePwr54);

                if (ahp->ah_eeversion >= AR_EEPROM_VER4_0) {
                        /* Setup XR target power from EEPROM */
                        rpow[15] = AH_MIN(scaledPower, IS_CHAN_2GHZ(chan) ?
                                ahp->ah_xrTargetPower2 : ahp->ah_xrTargetPower5);
                } else {
                        /* XR uses 6mb power */
                        rpow[15] = rpow[0];
                }

                *pMinPower = rpow[7];
                *pMaxPower = rpow[0];

                ahp->ah_ofdmTxPower = *pMaxPower;

                HALDEBUG(ah, "%s: MaxRD: %d TurboMax: %d MaxCTL: %d "
                        "TPC_Reduction %d\n",
                        __func__,
                        twiceMaxRDPower, ahp->ah_turbo2WMaxPower5,
                        twiceMaxEdgePower, tpcScaleReduction * 2);
        }

        if (IS_CHAN_CCK(chan) || IS_CHAN_G(chan)) {
                /* Get final CCK target powers */
                ar5416GetTargetPowers(ah, chan, ahp->ah_trgtPwr_11b,
                        ahp->ah_numTargetPwr_11b, &targetPowerCck);

                /* Reduce power by max regulatory domain allowed restrictions */
                scaledPower = AH_MIN(twiceMaxEdgePowerCck,
                        twiceMaxRDPower - twiceAntennaReduction);
                if (maxAvailPower < AH_MIN(scaledPower, targetPowerCck.twicePwr6_24))
                        maxAvailPower = AH_MIN(scaledPower, targetPowerCck.twicePwr6_24);

                /* Reduce power by user selection */
                scaledPower = AH_MIN(scaledPower, targetPowerCck.twicePwr6_24) - (tpcScaleReduction * 2);
                scaledPower = (scaledPower < 0) ? 0 : scaledPower;
                scaledPower = AH_MIN(scaledPower, powerLimit);

                /* Set CCK rates 2L, 2S, 5.5L, 5.5S, 11L, 11S */
                rpow[8]  = AH_MIN(scaledPower, targetPowerCck.twicePwr6_24);
                rpow[9]  = AH_MIN(scaledPower, targetPowerCck.twicePwr36);
                rpow[10] = rpow[9];
                rpow[11] = AH_MIN(scaledPower, targetPowerCck.twicePwr48);
                rpow[12] = rpow[11];
                rpow[13] = AH_MIN(scaledPower, targetPowerCck.twicePwr54);
                rpow[14] = rpow[13];

                /* Set min/max power based off OFDM values or initialization */
                if (rpow[13] < *pMinPower)
                    *pMinPower = rpow[13];
                if (rpow[9] > *pMaxPower)
                    *pMaxPower = rpow[9];

        }
        ahp->ah_tx6PowerInHalfDbm = *pMaxPower;
        return AH_TRUE;
}

/*
 * Find the maximum conformance test limit for the given channel and CTL info
 */
static u_int16_t
ar5416GetMaxEdgePower(u_int16_t channel, RD_EDGES_POWER *pRdEdgesPower)
{
        /* temp array for holding edge channels */
        u_int16_t tempChannelList[NUM_EDGES];
        u_int16_t clo, chi, twiceMaxEdgePower;
        int i, numEdges;

        /* Get the edge power */
        for (i = 0; i < NUM_EDGES; i++) {
                if (pRdEdgesPower[i].rdEdge == 0)
                        break;
                tempChannelList[i] = pRdEdgesPower[i].rdEdge;
        }
        numEdges = i;

        ar5416GetLowerUpperValues(channel, tempChannelList,
                numEdges, &clo, &chi);
        /* Get the index for the lower channel */
        for (i = 0; i < numEdges && clo != tempChannelList[i]; i++)
                ;
        /* Is lower channel ever outside the rdEdge? */
        HALASSERT(i != numEdges);

        if ((clo == chi && clo == channel) || (pRdEdgesPower[i].flag)) {
                /*
                 * If there's an exact channel match or an inband flag set
                 * on the lower channel use the given rdEdgePower
                 */
                twiceMaxEdgePower = pRdEdgesPower[i].twice_rdEdgePower;
                HALASSERT(twiceMaxEdgePower > 0);
        } else
                twiceMaxEdgePower = MAX_RATE_POWER;
        return twiceMaxEdgePower;
}

/*
 * Returns interpolated or the scaled up interpolated value
 */
static u_int16_t
interpolate(u_int16_t target, u_int16_t srcLeft, u_int16_t srcRight,
        u_int16_t targetLeft, u_int16_t targetRight)
{
        u_int16_t rv;
        int16_t lRatio;

        /* to get an accurate ratio, always scale, if want to scale, then don't scale back down */
        if ((targetLeft * targetRight) == 0)
                return 0;

        if (srcRight != srcLeft) {
                /*
                 * Note the ratio always need to be scaled,
                 * since it will be a fraction.
                 */
                lRatio = (target - srcLeft) * EEP_SCALE / (srcRight - srcLeft);
                if (lRatio < 0) {
                    /* Return as Left target if value would be negative */
                    rv = targetLeft;
                } else if (lRatio > EEP_SCALE) {
                    /* Return as Right target if Ratio is greater than 100% (SCALE) */
                    rv = targetRight;
                } else {
                        rv = (lRatio * targetRight + (EEP_SCALE - lRatio) *
                                        targetLeft) / EEP_SCALE;
                }
        } else {
                rv = targetLeft;
        }
        return rv;
}

/*
 * Return the four rates of target power for the given target power table
 * channel, and number of channels
 */
static void
ar5416GetTargetPowers(struct ath_hal *ah, HAL_CHANNEL *chan,
        TRGT_POWER_INFO *powInfo,
        u_int16_t numChannels, TRGT_POWER_INFO *pNewPower)
{
        /* temp array for holding target power channels */
        u_int16_t tempChannelList[NUM_TEST_FREQUENCIES];
        u_int16_t clo, chi, ixlo, ixhi;
        int i;

        /* Copy the target powers into the temp channel list */
        for (i = 0; i < numChannels; i++)
                tempChannelList[i] = powInfo[i].testChannel;

        ar5416GetLowerUpperValues(chan->channel, tempChannelList,
                numChannels, &clo, &chi);

        /* Get the indices for the channel */
        ixlo = ixhi = 0;
        for (i = 0; i < numChannels; i++) {
                if (clo == tempChannelList[i]) {
                        ixlo = i;
                }
                if (chi == tempChannelList[i]) {
                        ixhi = i;
                        break;
                }
        }

        /*
         * Get the lower and upper channels, target powers,
         * and interpolate between them.
         */
        pNewPower->twicePwr6_24 = interpolate(chan->channel, clo, chi,
                powInfo[ixlo].twicePwr6_24, powInfo[ixhi].twicePwr6_24);
        pNewPower->twicePwr36 = interpolate(chan->channel, clo, chi,
                powInfo[ixlo].twicePwr36, powInfo[ixhi].twicePwr36);
        pNewPower->twicePwr48 = interpolate(chan->channel, clo, chi,
                powInfo[ixlo].twicePwr48, powInfo[ixhi].twicePwr48);
        pNewPower->twicePwr54 = interpolate(chan->channel, clo, chi,
                powInfo[ixlo].twicePwr54, powInfo[ixhi].twicePwr54);
}
#endif

/*
 * Search a list for a specified value v that is within
 * EEP_DELTA of the search values.  Return the closest
 * values in the list above and below the desired value.
 * EEP_DELTA is a factional value; everything is scaled
 * so only integer arithmetic is used.
 *
 * NB: the input list is assumed to be sorted in ascending order
 */
void
ar5416GetLowerUpperValues(u_int16_t v, u_int16_t *lp, u_int16_t listSize,
                          u_int16_t *vlo, u_int16_t *vhi)
{
        u_int32_t target = v * EEP_SCALE;
        u_int16_t *ep = lp+listSize;

        /*
         * Check first and last elements for out-of-bounds conditions.
         */
        if (target < (u_int32_t)(lp[0] * EEP_SCALE - EEP_DELTA)) {
                *vlo = *vhi = lp[0];
                return;
        }
        if (target > (u_int32_t)(ep[-1] * EEP_SCALE + EEP_DELTA)) {
                *vlo = *vhi = ep[-1];
                return;
        }

        /* look for value being near or between 2 values in list */
        for (; lp < ep; lp++) {
                /*
                 * If value is close to the current value of the list
                 * then target is not between values, it is one of the values
                 */
                if (abs(lp[0] * EEP_SCALE - target) < EEP_DELTA) {
                        *vlo = *vhi = lp[0];
                        return;
                }
                /*
                 * Look for value being between current value and next value
                 * if so return these 2 values
                 */
                if (target < (u_int32_t)(lp[1] * EEP_SCALE - EEP_DELTA)) {
                        *vlo = lp[0];
                        *vhi = lp[1];
                        return;
                }
        }
}

/*
 * Perform analog "swizzling" of parameters into their location
 */
void
ar5416ModifyRfBuffer(u_int32_t *rfBuf, u_int32_t reg32, u_int32_t numBits,
                     u_int32_t firstBit, u_int32_t column)
{
        u_int32_t tmp32, mask, arrayEntry, lastBit;
        int32_t bitPosition, bitsLeft;

        HALASSERT(column <= 3);
        HALASSERT(numBits <= 32);
        HALASSERT(firstBit + numBits <= MAX_ANALOG_START);

        tmp32 = ath_hal_reverseBits(reg32, numBits);
        arrayEntry = (firstBit - 1) / 8;
        bitPosition = (firstBit - 1) % 8;
        bitsLeft = numBits;
        while (bitsLeft > 0) {
                lastBit = (bitPosition + bitsLeft > 8) ?
                        8 : bitPosition + bitsLeft;
                mask = (((1 << lastBit) - 1) ^ ((1 << bitPosition) - 1)) <<
                        (column * 8);
                rfBuf[arrayEntry] &= ~mask;
                rfBuf[arrayEntry] |= ((tmp32 << bitPosition) <<
                        (column * 8)) & mask;
                bitsLeft -= 8 - bitPosition;
                tmp32 = tmp32 >> (8 - bitPosition);
                bitPosition = 0;
                arrayEntry++;
        }
}

void
ar5416GetChannelCenters(struct ath_hal *ah, HAL_HT *ht,
                       HAL_CHANNEL_INTERNAL *chan, CHAN_CENTERS *centers)
{
    if (!ht || (ht->cwm.ht_phymode != HAL_HT_PHYMODE_2040)) {
        centers->ctl_center = centers->ext_center =
        centers->synth_center = chan->channel;
        return;
    }

    /*
     * In 20/40 phy mode, the center frequency is
     * "between" the primary and extension channels.
     */
     switch (ht->cwm.ht_extoff) {
         case 1:
             centers->synth_center = chan->channel + HT40_CHANNEL_CENTER_SHIFT;
             break;
         case -1:
             centers->synth_center = chan->channel - HT40_CHANNEL_CENTER_SHIFT;
             break;
         default:
             HALASSERT(0);
             centers->ctl_center = centers->ext_center =
             centers->synth_center = chan->channel;
             return;
     }

     centers->ctl_center = centers->synth_center -
                (ht->cwm.ht_extoff * HT40_CHANNEL_CENTER_SHIFT);
     centers->ext_center = centers->synth_center +
                (ht->cwm.ht_extoff *
                        (ht->cwm.ht_extprotspacing == HAL_HT_EXTPROTSPACING_20 ? 10 : 15));
}

#define IS(_c,_f)       (((_c)->channelFlags & _f) || 0)

static inline HAL_CHANNEL_INTERNAL*
ar5416CheckChan(struct ath_hal *ah, HAL_CHANNEL *chan, HAL_HT *ht)
{
    if ((IS(chan, CHANNEL_2GHZ) ^ IS(chan, CHANNEL_5GHZ)) == 0) {
        HALDEBUG(ah, "%s: invalid channel %u/0x%x; not marked as "
                 "2GHz or 5GHz\n", __func__,
                chan->channel, chan->channelFlags);
        return AH_NULL;
    }

    if ((IS(chan, CHANNEL_OFDM) ^ IS(chan, CHANNEL_CCK) ^
         IS(chan, CHANNEL_HT20)) == 0) {
        HALDEBUG(ah, "%s: invalid channel %u/0x%x; not marked as "
                "OFDM or CCK\n", __func__,
                chan->channel, chan->channelFlags);
        return AH_NULL;
    }

    /* XXX 
     * Validation of HT information should be within ath_hal_checkchannel
     * using the private channel structure.
     * This is a temporary workaround that is necessary because the 20 MHz
     * and 40 MHz channel flags were combined in the atheros layer, not the HAL.
     */
    if (ht && !ar5416ValidateHt(chan, ht)) {
	    HALDEBUG(ah, "%s: invalid channel %u/0x%x; HT configuration "
		    "does not meet regulatory\n", __func__,
		    chan->channel, chan->channelFlags);
	    HALDEBUG(ah, "%s: phy mode %d, ext offset %d\n", __func__,
		    ht->cwm.ht_phymode, ht->cwm.ht_extoff);
	    return AH_NULL;
    }

    return (ath_hal_checkchannel(ah, chan));
}

static int
ar5416ValidateHt(HAL_CHANNEL *chan, HAL_HT *ht)
{
    if (ht->cwm.ht_phymode == HAL_HT_PHYMODE_20) {
        /* 20 MHz */
        if (ht->cwm.ht_extoff != 0) {
            return 0;
        }
    } else {
        /* 20/40 MHz */
	switch (ht->cwm.ht_extoff) {
	case 0:
		return 0;
		break;
	case 1:
		/* extension channel above control channel */
		if (!IS(chan, CHANNEL_HT40L)) {
		    return 0;
		}
		break;
	case -1:
		/* extension channel below control channel */
		if (!IS(chan, CHANNEL_HT40U)) {
		    return 0;
		}
		break;

	default:
		return 0;
	}
    }
    return 1;
}
#undef IS

static inline HAL_BOOL 
ar5416AllowChannelChange(struct ath_hal *ah, HAL_BOOL bChannelChange, 
                         HAL_HT *ht, HAL_CHANNEL *chan)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);

    /* Upper layer wants to reset */
    if (!bChannelChange)
        return AH_FALSE;

    /* Changing HT phy modes */
    if (ht && ht->cwm.ht_phymode != ahp->ah_htcwm.ht_phymode)
        return AH_FALSE;

    /* PowerOn reset to set channel */
    if (!ahpriv->ah_curchan)
        return AH_FALSE;

    /* Switching bands */
    if (!IS_CHAN_SAME_BAND(chan, ahpriv->ah_curchan))
            return AH_FALSE;

    return AH_TRUE;
}

/*
 * TODO: Only write the PLL if we're changing to or from CCK mode
 *
 * WARNING: The order of the PLL and mode registers must be correct.
 */
static inline void
ar5416SetRfMode(struct ath_hal *ah, HAL_CHANNEL *chan)
{
    u_int32_t rfMode = 0;

    if (chan == AH_NULL)
        return;

    switch (ar5416Get11nHwPlatform(ah)) {
        case HAL_MAC_TO_MAC_EMU:
            break;
        case HAL_MAC_BB_EMU:
        case HAL_TRUE_CHIP:
            rfMode |= (IS_CHAN_G(chan))
                      ? AR_PHY_MODE_DYNAMIC : AR_PHY_MODE_OFDM;
            break;
    }

    rfMode |= (IS_CHAN_5GHZ(chan)) ? AR_PHY_MODE_RF5GHZ : AR_PHY_MODE_RF2GHZ;

    OS_REG_WRITE(ah, AR_PHY_MODE, rfMode);
}

static inline HAL_STATUS
ar5416ProcessIni(struct ath_hal *ah, HAL_HT *ht, HAL_CHANNEL *chan,
                 HAL_CHANNEL_INTERNAL *ichan)
{
    int i, regWrites = 0;
    struct ath_hal_5416 *ahp = AH5416(ah);
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
    u_int modesIndex, freqIndex;
    HAL_STATUS status;

    if (ar5416Get11nHwPlatform(ah) == HAL_MAC_BB_EMU)
        ar5416SetupHainan(ah);

    /* Setup the indices for the next set of register array writes */
    switch (chan->channelFlags & CHANNEL_ALL) {
        /* TODO:
         * If the channel marker is indicative of the current mode rather
         * than capability, we do not need to check the phy mode below.
         */
        case CHANNEL_A:
        case CHANNEL_A_HT20:
        case CHANNEL_A_HT40:
            if (ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
                modesIndex = 2;
                freqIndex  = 1;
            } else {
                modesIndex = 1;
                freqIndex  = 1;
            }
            break;
        case CHANNEL_PUREG:
        case CHANNEL_G_HT20:
        case CHANNEL_G_HT40:
            if (ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
                modesIndex = 3;
                freqIndex  = 2;
            } else {
                modesIndex = 4;
                freqIndex  = 2;
            }
            break;
        case CHANNEL_108G:
            modesIndex = 5;
            freqIndex  = 2;
            break;
        default:
            HALASSERT(0);
            return HAL_EINVAL;
    }

    /* Set correct Baseband to analog shift setting to access analog chips. */
    OS_REG_WRITE(ah, AR_PHY(0), 0x00000007);

    /*
     * Write addac shifts
    */
    if (ar5416Get11nHwPlatform(ah) == HAL_TRUE_CHIP) {
        OS_REG_WRITE(ah, AR_PHY_ADC_SERIAL_CTL, AR_PHY_SEL_EXTERNAL_RADIO);
        if (ah->ah_macRev >= AR_SREV_REVISION_OWL_22) {
        	REG_WRITE_ARRAY(ar5416Addac, 1, regWrites);
        } else {
		/* Owl 2.1/2.0 */
		#define ADDAC_SIZE (sizeof(ar5416Addac) / sizeof(*(ar5416Addac)))
		u_int32_t ar5416AddacOwl21[ADDAC_SIZE][2];
		OS_MEMCPY(ar5416AddacOwl21, ar5416Addac, sizeof(ar5416Addac));
		/* override CLKDRV value */
		ar5416AddacOwl21[31][1] = 0;
        	REG_WRITE_ARRAY(ar5416AddacOwl21, 1, regWrites);
        }
        OS_REG_WRITE(ah, AR_PHY_ADC_SERIAL_CTL, AR_PHY_SEL_INTERNAL_ADDAC);
    }

    REG_WRITE_ARRAY(ar5416Modes, modesIndex, regWrites);
    /* Write Common Array Parameters */
    for (i = 0; i < sizeof(ar5416Common)/sizeof(ar5416Common[0]); i++) {
        u_int32_t reg = ar5416Common[i][0];
        /* XXX timer/beacon setup registers? */
        OS_REG_WRITE(ah, reg, ar5416Common[i][1]);
        WAR_6773(regWrites);
    }

    ahp->ah_rfHal.writeRegs(ah, modesIndex, freqIndex, regWrites);

    /* Owl 2 specific configuration */
    ar5416OverrideIniOwl2(ah);

    /* Setup 11n MAC/Phy mode registers */
    ar5416Set11nRegs(ah, ht);

    /*
     * Moved ar5416InitChainMasks() here to ensure the swap bit is set before
     * the pdadc table is written.  Swap must occur before any radio dependent
     * replicated register access.  The pdadc curve addressing in particular
     * depends on the consistent setting of the swap bit.
     */
     ar5416InitChainMasks(ah, ht);

    /*
     * Setup the transmit power values.
     *
     * After the public to private hal channel mapping, ichan contains the
     * valid regulatory power value.
     * ath_hal_getctl and ath_hal_getantennaallowed look up ichan from chan.
     */
    status = ar5416EepromSetTransmitPower(ah, ht, &ahp->ah_eeprom, ichan,
             ath_hal_getctl(ah, chan), ath_hal_getantennaallowed(ah, chan),
             ichan->maxRegTxPower * 2,
             AH_MIN(MAX_RATE_POWER, ahpriv->ah_powerLimit));
    if (status != HAL_OK) {
        HALDEBUG(ah, "%s: error init'ing transmit power\n", __func__);
        return HAL_EIO;
    }

    /* Write the analog registers */
    if (!ahp->ah_rfHal.setRfRegs(ah, ichan, freqIndex)) {
        HALDEBUG(ah, "%s: ar5416SetRfRegs failed\n", __func__);
        return HAL_EIO;
    }

    return HAL_OK;
}

static inline void
ar5416InitHainan(struct ath_hal *ah)
{
    OS_REG_WRITE(ah, 0xd47c, 0xe6);
    OS_DELAY(EMU_HAINAN_PLL_SETTLE_DELAY);

    OS_REG_WRITE(ah, 0x404c, 0x30);       // Enable GPIO2 to be output
    OS_REG_WRITE(ah, 0x4048, 0x0);
    OS_REG_WRITE(ah, 0x4048, 0x4);        // Pulse GPIO2 to cause DCM Reset
    OS_DELAY(1000);

    OS_REG_WRITE(ah, 0x4048, 0x0);
    OS_DELAY(1000);
}

static inline void
ar5416SetupHainan(struct ath_hal *ah)
{
    int i;

    /* Setup external Hainan baseband chains */
    OS_REG_WRITE(ah, 0xd400, 0x00000047);
    // Just turbo and short20 in this new register
    OS_REG_WRITE(ah, 0xd404, 0x00000000);
    // TX Source is TSTDAC and enable TSTADC
    OS_REG_WRITE(ah, 0xd408, 0x00000502);
    OS_REG_WRITE(ah, 0xd40c, 0x0b849233);
    OS_REG_WRITE(ah, 0xd410, 0x3d32e000);
    OS_REG_WRITE(ah, 0xd414, 0x0000076b);
    OS_REG_WRITE(ah, 0xd420, 0x04040402);
    OS_REG_WRITE(ah, 0xd424, 0x00000e0e);
    OS_REG_WRITE(ah, 0xd428, 0x0a020201);
    // DAC and ADC always ON
    //OS_REG_WRITE(ah, 0xd82c + (i * 0x400), 0x00020002);
    OS_REG_WRITE(ah, 0xd42c, 0x10021002);
    OS_REG_WRITE(ah, 0xd430, 0x00000000);
    OS_REG_WRITE(ah, 0xd434, 0x00000e0e);
    OS_REG_WRITE(ah, 0xd438, 0x0000c003);
    OS_REG_WRITE(ah, 0xd440, 0xaaa86420);
    OS_REG_WRITE(ah, 0xd444, 0x137216ad);
    OS_REG_WRITE(ah, 0xd448, 0x0018fab3);
    OS_REG_WRITE(ah, 0xd44c, 0x1284613c);
    OS_REG_WRITE(ah, 0xd450, 0x0de8a8d5);
    OS_REG_WRITE(ah, 0xd454, 0x00074859);
    OS_REG_WRITE(ah, 0xd458, 0x7e80bd3a);
    OS_REG_WRITE(ah, 0xd45c, 0x3277665e);
    OS_REG_WRITE(ah, 0xd460, 0x0000b910);
    OS_REG_WRITE(ah, 0xd464, 0x000347a6);
    OS_REG_WRITE(ah, 0xd468, 0x409a4190);
    // Enable Low SNR detection
    OS_REG_WRITE(ah, 0xd46c, 0x050cb481);

    OS_REG_WRITE(ah, 0xd470, 0x00000000);
    OS_REG_WRITE(ah, 0xd474, 0x00000080);
    // OS_REG_WRITE(ah, 0xd878 + (i * 0x400), 0x00000008);    // DAC and ADC clock select
    // DAC and ADC clock select
    OS_REG_WRITE(ah, 0xd478, 0x00000004);

    OS_REG_WRITE(ah, 0xd51c, 2346);          // reg 71
    OS_REG_WRITE(ah, 0xd520, 0x00000000);    // reg 72

    OS_REG_WRITE(ah, 0xd524, 0x000a8f15);    // reg 73
    // (phy warm reset deasserted)
    OS_REG_WRITE(ah, 0xd528, 0x00000001);    // reg 74
    //36=18dBm
    OS_REG_WRITE(ah, 0xd52c, 0x00000000);    // reg 75
    //rrrr rrr- ---- ---- 0 100100 010 00001 1, probe off
    OS_REG_WRITE(ah, 0xd530, 0x00004883);    // reg 76
    //30,31,32,34 = 15.0, 15.5, 16.0, 17.0dBm
    OS_REG_WRITE(ah, 0xd534, 0x1e1f2022);    // reg 77
    //26,27,28,29 = 13.0, 13.5, 14.0, 14.5dBm
    OS_REG_WRITE(ah, 0xd538, 0x1a1b1c1d);    // reg 78
    //33 = 16.5dBm
    OS_REG_WRITE(ah, 0xd53c, 0x0000003f);    // reg 79
    //time4_rd_gainf delay
    OS_REG_WRITE(ah, 0xd540, 0x00000004);    // reg 80

    // OS_REG_WRITE(ah, 0xd944, 0x4fe010e0);    // reg 81; Previous reg 1
    //Previous reg 1
    OS_REG_WRITE(ah, 0xd544, 0x4fe01040);    // reg 81
    //Service 0
    OS_REG_WRITE(ah, 0xd548, 0x00000000);    // reg 82
    //Service 1
    OS_REG_WRITE(ah, 0xd54c, 0x00000000);    // reg 83
    //Service 2
    OS_REG_WRITE(ah, 0xd550, 0x00000000);    // reg 84
    //Radar detection
    OS_REG_WRITE(ah, 0xd554, 0x5d50f14c);    // reg 85

    OS_REG_WRITE(ah, 0xd50c, 0x00000000);    // reg 67

    OS_REG_WRITE(ah, 0xd41c, 0x00000001);    // Activate D2
    for (i=0; i < 100; i++) {
        OS_REG_READ(ah, 0x4000);
    }
}

static inline void
ar5416InitPLL(struct ath_hal *ah, HAL_CHANNEL *chan)
{
    int pll = AR_RTC_PLL_REFDIV_5 | AR_RTC_PLL_DIV2;

    if(ath_hal_clksel == 1) {
        pll |= SM(0x1, AR_RTC_PLL_CLKSEL);
    } else if(ath_hal_clksel == 2) {
        pll |= SM(0x2, AR_RTC_PLL_CLKSEL);
    }

    if (chan && IS_CHAN_5GHZ(chan)) {
        OS_REG_WRITE(ah, AR_RTC_PLL_CONTROL, (pll | SM(0xa, AR_RTC_PLL_DIV)));
    } else {
        OS_REG_WRITE(ah, AR_RTC_PLL_CONTROL, (pll | SM(0xb, AR_RTC_PLL_DIV)));
    }

    /* TODO:
     * For multi-band owl, switch between bands by reiniting the PLL.
     */

    OS_DELAY(RTC_PLL_SETTLE_DELAY);

    OS_REG_WRITE(ah, AR_RTC_SLEEP_CLK, AR_RTC_FORCE_DERIVED_CLK);
}

static inline void
ar5416InitChainMasks(struct ath_hal *ah, HAL_HT *ht)
{
    int rx_chainmask, tx_chainmask;

    if (ht) {
        rx_chainmask = ht->misc.ht_rxchainmask;
        tx_chainmask = ht->misc.ht_txchainmask;
    } else {
        rx_chainmask = tx_chainmask = AR5416_LEGACY_CHAINMASK;
    }

    switch (rx_chainmask) {
        case 0x5:
            OS_REG_WRITE(ah, AR_PHY_ANALOG_SWAP, AR_PHY_SWAP_ALT_CHAIN);
            /*
             * fall through !
             */
        case 0x3:
            if (ar5416Get11nHwPlatform(ah) == HAL_TRUE_CHIP) {
                /*
                 * workaround for OWL 1.0 cal failure, always cal 3 chains for
                 * multi chain -- then after cal set true mask value
                 */
                OS_REG_WRITE(ah, AR_PHY_RX_CHAINMASK, 0x7);
                OS_REG_WRITE(ah, AR_PHY_CAL_CHAINMASK, 0x7);
                break;
            }
            /*
             * fall through !
             */
        case 0x1:
        case 0x7:
            OS_REG_WRITE(ah, AR_PHY_RX_CHAINMASK, rx_chainmask);
            OS_REG_WRITE(ah, AR_PHY_CAL_CHAINMASK, rx_chainmask);
            break;
        default:
            break;
    }

    OS_REG_WRITE(ah, AR_SELFGEN_MASK, tx_chainmask);
    if (tx_chainmask == 0x5) {
        OS_REG_WRITE(ah, AR_PHY_ANALOG_SWAP, AR_PHY_SWAP_ALT_CHAIN);
    }
}

static inline HAL_BOOL
ar5416InitCal(struct ath_hal *ah, HAL_HT *ht)
{
    /* Calibrate the AGC */
    OS_REG_WRITE(ah, AR_PHY_AGC_CONTROL,
              OS_REG_READ(ah, AR_PHY_AGC_CONTROL)
            | AR_PHY_AGC_CONTROL_CAL);

    /* NF calibration */
    if (ar5416Get11nHwPlatform(ah) == HAL_MAC_BB_EMU) {
        /* NF calibration is unreliable on Owl MAC/BB emulation
         * Needs special handling
         */
        if (!ar5416NfCalInitHainan(ah, ht)) {
            ath_hal_printf(ah, "%s: NF calibration failed to complete;"
                " noisy environment?\n", __func__);
            return AH_FALSE;
        }
    } else {
        /* Start NF calibration */
        OS_REG_WRITE(ah, AR_PHY_AGC_CONTROL,
              OS_REG_READ(ah, AR_PHY_AGC_CONTROL)
            | AR_PHY_AGC_CONTROL_NF);
    }


    /*
     * TODO:
     * IQ-Cal not working yet. All dynamic cals off for now.
     */
#if 0
    if (!IS_CHAN_B(chan) && ahp->ah_bIQCalibration != IQ_CAL_DONE) {
            /* Start IQ calibration w/ 2^(INIT_IQCAL_LOG_COUNT_MAX+1) samples */
            OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
                    AR_PHY_TIMING_CTRL4_IQCAL_LOG_COUNT_MAX,
                    INIT_IQCAL_LOG_COUNT_MAX);
            OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4,
                    AR_PHY_TIMING_CTRL4_DO_IQCAL);
            ahp->ah_bIQCalibration = IQ_CAL_RUNNING;
    } else
            ahp->ah_bIQCalibration = IQ_CAL_INACTIVE;

#endif

    /* Poll for offset calibration complete */
    if (!ath_hal_wait(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_CAL, 0)) {
        HALDEBUG(ah, "%s: offset calibration failed to complete in 1ms;"
            " noisy environment?\n", __func__);
     return AH_FALSE;
    }

    return AH_TRUE;
}

/*
 * MAC/BB Emulation Only (Hainan)
 *
 * Run NF calibration until chain 0 and chain 1 noise floors are similar
 *
 */
static inline HAL_BOOL
ar5416NfCalInitHainan(struct ath_hal *ah, HAL_HT *ht)
{
    int         nfInSync = 0;
    int         nfFinish = 0;
    int         syncCount;
    int         nfWaitCount, rx_chainmask;
    int16_t     nf[NUM_NOISEFLOOR_READINGS] = {0};
    int16_t     nfDiff;

#define ATH_EMU_MAXLOOP_NFSYNC     10       /* Number of noise floor attempts to synchronize between chains */
#define ATH_EMU_MAXLOOP_NFWAIT     500        /* Number of 1 msec loops */
#define ATH_EMU_NFDIFF             3     /* max NF delta between chain 0 and chain 1 */

    for (syncCount = 0; syncCount < ATH_EMU_MAXLOOP_NFSYNC; syncCount++) {
        /* Start NF calibration */
        OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);

        /* Wait for NF calibration to finish */
        nfFinish = 0;
        for (nfWaitCount = 0; nfWaitCount < ATH_EMU_MAXLOOP_NFWAIT; nfWaitCount++) {
            if (ath_hal_wait(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF, 0)) {
                nfFinish = 1;
                break;
            }
        }

        if (!nfFinish) {
            ath_hal_printf(ah, "%s: NF calibration failed to complete - trying again\n;", __func__);
            continue;
        }

        /* single chain - no need to check for in sync */
        rx_chainmask = (ht) ? ht->misc.ht_rxchainmask : AR5416_LEGACY_CHAINMASK;
        if (rx_chainmask != 0x3) {
            nfInSync = 1;
            break;

        }

        /* Read NF */
        ar5416GetNoiseFloor(ah, nf);

        /* two chain - calculate NF diff */
        if (ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
            /* Dyn 20/40 mode => use NF values from the extension channel */
            nfDiff = nf[1] - nf[3];
        } else {
            /* Static 20 mode => use NF values from the control channel */
            nfDiff = nf[0] - nf[2];
        }

        if (abs(nfDiff) < ATH_EMU_NFDIFF) {
            ath_hal_printf(ah, "%s: NF calibration synchronized for two chains\n;", __func__);
            nfInSync = 1;
            break;
        } else {
            ath_hal_printf(ah, "%s: NF calibration synchronization failed => retrying\n;", __func__);
            /* reinitialize Hainan PLL */
            OS_REG_WRITE(ah, 0xd47c, 0xe6);
            OS_DELAY(EMU_HAINAN_PLL_SETTLE_DELAY);
        }
    }

    if (!nfInSync) {
        ath_hal_printf(ah, "%s: NF calibration failed to synchronize chains (ERROR)\n;", __func__);
        return AH_FALSE;
    }

    return AH_TRUE;
}

static inline void
ar5416SetDma(struct ath_hal *ah)
{
    u_int32_t   regval;

    /*
     * set AHB_MODE not to do cacheline prefetches
     */
    regval = OS_REG_READ(ah, AR_AHB_MODE);
    OS_REG_WRITE(ah, AR_AHB_MODE, regval | AR_AHB_PREFETCH_RD_EN);

    /*
     * let mac dma reads be in 128 byte chunks
     */
    regval = OS_REG_READ(ah, AR_TXCFG) & ~AR_TXCFG_DMASZ_MASK;
    OS_REG_WRITE(ah, AR_TXCFG, regval | AR_TXCFG_DMASZ_128B);

    /*
     * let mac dma writes be in 128 byte chunks
     */
    regval = OS_REG_READ(ah, AR_RXCFG) & ~AR_RXCFG_DMASZ_MASK;
    OS_REG_WRITE(ah, AR_RXCFG, regval | AR_RXCFG_DMASZ_128B);

    /*
     * Setup receive FIFO threshold to hold off TX activities
     */
    OS_REG_WRITE(ah, AR_RXFIFO_CFG, 0x200);

    /*
     * reduce the number of usable entries in PCU TXBUF to avoid
     * wrap around bugs. (bug 20428)
     */
    OS_REG_WRITE(ah, AR_PCU_TXBUF_CTRL, AR_PCU_TXBUF_CTRL_USABLE_SIZE);
}

/* Owl 2.0 - override INI values with Owl 2.0 specific configuration.
 *          Based on MDK Owl 2.0 specific configuration
 *
 * TODO - remove when we have an INI that supports Owl 2.0 and Owl 1.0
 */
static inline void
ar5416OverrideIniOwl2(struct ath_hal *ah)
{
#if RIFS_EN
    u_int32_t rddata, wrdata;
#endif

    if (ah->ah_macRev < AR_SREV_REVISION_OWL_20) {
        return;
    }

#if RIFS_EN
   /* Enable Reset TDOMAIN */
   OS_REG_WRITE(ah, 0x9800+(738<<2), 0x0);

   rddata = OS_REG_READ(ah, 0x9800+(738<<2));
   wrdata = rddata | (0x1 << 26) | (0x1 << 27);

   /* activate reset_tdomain to enable RIFS rx */
   OS_REG_WRITE(ah, 0x9800+(738<<2), wrdata);

   /* Enable external init gain */
   OS_REG_WRITE(ah, 0x9850, 0xec28b0da);
   OS_REG_WRITE(ah, 0x985c, 0x31395d5e);
#endif

    /* Disable BB clock gating
     * Necessary to avoid hangs in Owl 2.0
     */
     OS_REG_WRITE(ah, 0x9800+(651<<2), 0x11);
}

static inline void
ar5416InitBB(struct ath_hal *ah, HAL_CHANNEL *chan)
{
    u_int32_t synthDelay;

    /*
     * Wait for the frequency synth to settle (synth goes on
     * via AR_PHY_ACTIVE_EN).  Read the phy active delay register.
     * Value is in 100ns increments.
     */
    synthDelay = OS_REG_READ(ah, AR_PHY_RX_DELAY) & AR_PHY_RX_DELAY_DELAY;
    if (IS_CHAN_CCK(chan)) {
            synthDelay = (4 * synthDelay) / 22;
    } else {
            synthDelay /= 10;
    }

    /* Activate the PHY (includes baseband activate + synthesizer on) */
    OS_REG_WRITE(ah, AR_PHY_ACTIVE, AR_PHY_ACTIVE_EN);

    /*
     * There is an issue if the AP starts the calibration before
     * the base band timeout completes.  This could result in the
     * rx_clear false triggering.  As a workaround we add delay an
     * extra BASE_ACTIVATE_DELAY usecs to ensure this condition
     * does not happen.
     */
    if (ar5416Get11nHwPlatform(ah) == HAL_MAC_BB_EMU) {
        OS_DELAY(synthDelay + EMU_HAINAN_BASE_ACTIVATE_DELAY);
    } else {
        OS_DELAY(synthDelay + BASE_ACTIVATE_DELAY);
    }
}

static inline void
ar5416InitInterruptMasks(struct ath_hal *ah, HAL_OPMODE opmode)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    /*
     * Setup interrupt handling.  Note that ar5416ResetTxQueue
     * manipulates the secondary IMR's as queues are enabled
     * and disabled.  This is done with RMW ops to insure the
     * settings we make here are preserved.
     */
    ahp->ah_maskReg = AR_IMR_TXERR | AR_IMR_TXURN | AR_IMR_RXERR | AR_IMR_RXORN
                    | AR_IMR_BCNMISC;

#ifdef AR5416_INT_MITIGATION
    ahp->ah_maskReg |= AR_IMR_TXINTM | AR_IMR_RXINTM
                    | AR_IMR_TXMINTR | AR_IMR_RXMINTR;
#else
    ahp->ah_maskReg |= AR_IMR_TXOK | AR_IMR_RXOK;
#endif

    if (opmode == HAL_M_HOSTAP)
        ahp->ah_maskReg |= AR_IMR_MIB;

    OS_REG_WRITE(ah, AR_IMR, ahp->ah_maskReg);

    OS_REG_WRITE(ah, AR_IMR_S2, OS_REG_READ(ah, AR_IMR_S2) | AR_IMR_S2_GTT);
}

static inline void
ar5416InitQOS(struct ath_hal *ah)
{
    OS_REG_WRITE(ah, AR_MIC_QOS_CONTROL, 0x100aa);  /* XXX magic */
    OS_REG_WRITE(ah, AR_MIC_QOS_SELECT, 0x3210);    /* XXX magic */

    /* Turn on NOACK Support for QoS packets */
    OS_REG_WRITE(ah, AR_QOS_NO_ACK,
            SM(2, AR_QOS_NO_ACK_TWO_BIT) |
            SM(5, AR_QOS_NO_ACK_BIT_OFF) |
            SM(0, AR_QOS_NO_ACK_BYTE_OFF));

    /*
     * initialize TXOP for all TIDs
     */
    OS_REG_WRITE(ah, AR_TXOP_X, AR_TXOP_X_VAL);
    OS_REG_WRITE(ah, AR_TXOP_0_3, 0xFFFFFFFF);
    OS_REG_WRITE(ah, AR_TXOP_4_7, 0xFFFFFFFF);
    OS_REG_WRITE(ah, AR_TXOP_8_11, 0xFFFFFFFF);
    OS_REG_WRITE(ah, AR_TXOP_12_15, 0xFFFFFFFF);
}

static inline void
ar5416InitUserSettings(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    /* Restore user-specified settings */
    if (ahp->ah_miscMode != 0)
            OS_REG_WRITE(ah, AR_PCU_MISC, ahp->ah_miscMode);
    if (ahp->ah_slottime != (u_int) -1)
            ar5416SetSlotTime(ah, ahp->ah_slottime);
    if (ahp->ah_acktimeout != (u_int) -1)
            ar5416SetAckTimeout(ah, ahp->ah_acktimeout);
    if (ahp->ah_ctstimeout != (u_int) -1)
            ar5416SetCTSTimeout(ah, ahp->ah_ctstimeout);
    if (ahp->ah_globaltxtimeout != (u_int) -1)
        ar5416SetGlobalTxTimeout(ah, ahp->ah_globaltxtimeout);
    if (AH_PRIVATE(ah)->ah_diagreg != 0)
        OS_REG_SET_BIT(ah, AR_DIAG_SW, AH_PRIVATE(ah)->ah_diagreg);
}

static inline void
ar5416AttachHwPlatform(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    u_int32_t   regval;

    if (OS_REG_READ(ah, AR_EMU) == 0) {
         ahp->ah_hwp = HAL_TRUE_CHIP;
         return;
    }

    regval = OS_REG_READ(ah, AR_PHY_CHIP_ID);
    if ((regval == AR_PHY_CHIP_ID_REV_0) || (regval == AR_PHY_CHIP_ID_REV_1)) {
        ahp->ah_hwp = HAL_MAC_BB_EMU;
        return;
    }

    ahp->ah_hwp = HAL_MAC_TO_MAC_EMU;
}

#endif /* AH_SUPPORT_AR5416 */
