/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_eeprom.c#30 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"
#ifdef AH_DEBUG
#include "ah_desc.h"                    /* NB: for HAL_PHYERR* */
#endif
#include "ah_eeprom.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"

/*
 * Temporary cap on maximum power used in rate => power table
 * as workaround for board issues.
 */
#define TEMP_POWER_CAP      1

#define N(a)            (sizeof (a) / sizeof (a[0]))

/* reg_off = 4 * (eep_off) */
#define AR5416_EEPROM_S             2
#define AR5416_EEPROM_OFFSET        0x2000
#define AR5416_EEPROM_START_ADDR    0x503f1200
#define AR5416_EEPROM_MAX           0xae0
#define owl_get_eep_ver(_ahp)   \
    (((_ahp)->ah_eeprom.baseEepHeader.version >> 12) & 0xF)
#define owl_get_eep_rev(_ahp)   \
    (((_ahp)->ah_eeprom.baseEepHeader.version) & 0xFFF)
#define owl_get_ntxchains(_txchainmask) \
    (((_txchainmask >> 2) & 1) + ((_txchainmask >> 1) & 1) + (_txchainmask & 1))

static inline void ar5416FillEmuEeprom(struct ath_hal_5416 *ahp);
static inline HAL_BOOL ar5416FillEeprom(struct ath_hal *ah);
static inline HAL_STATUS ar5416CheckEeprom(struct ath_hal *ah);
static inline HAL_BOOL ar5416SetPowerPerRateTable(struct ath_hal *ah,
                           ar5416_eeprom_t *pEepData, HAL_HT *ht,
                           HAL_CHANNEL_INTERNAL *chan, int16_t *ratesArray,
                           u_int16_t cfgCtl, u_int16_t AntennaReduction,
                           u_int16_t twiceMaxRegulatoryPower, 
                           u_int16_t powerLimit);
static inline HAL_BOOL ar5416SetPowerCalTable(struct ath_hal *ah,
                          HAL_HT *ht, ar5416_eeprom_t *pEepData,
                          HAL_CHANNEL_INTERNAL *chan,
                          int16_t *pTxPowerIndexOffset);
static inline void ar5416GetTargetPowers(struct ath_hal *ah, HAL_HT *ht,
                     HAL_CHANNEL_INTERNAL *chan, CAL_TARGET_POWER_HT *powInfo,
                     u_int16_t numChannels, CAL_TARGET_POWER_HT *pNewPower,
                     u_int16_t numRates, HAL_BOOL isHt40Target);
static inline void ar5416GetTargetPowersLeg(struct ath_hal *ah, HAL_HT *ht,
                    HAL_CHANNEL_INTERNAL *chan, CAL_TARGET_POWER_LEG *powInfo,
                    u_int16_t numChannels, CAL_TARGET_POWER_LEG *pNewPower,
                    u_int16_t numRates, HAL_BOOL isExtTarget);
static inline u_int16_t ar5416GetMaxEdgePower(u_int16_t freq, 
                    CAL_CTL_EDGES *pRdEdgesPower, HAL_BOOL is2GHz);

static inline u_int16_t fbin2freq(u_int8_t fbin, HAL_BOOL is2GHz);
static inline int16_t interpolate(u_int16_t target, u_int16_t srcLeft,
                          u_int16_t srcRight, int16_t targetLeft,
                          int16_t targetRight);
static inline void ar5416GetGainBoundariesAndPdadcs(struct ath_hal *ah, 
                        HAL_HT *ht,
                        HAL_CHANNEL_INTERNAL *chan, 
                        CAL_DATA_PER_FREQ *pRawDataSet,
                        u_int8_t * bChans, u_int16_t availPiers,
                        u_int16_t tPdGainOverlap, int16_t *pMinCalPower,
                        u_int16_t * pPdGainBoundaries, u_int8_t * pPDADCValues,
                        u_int16_t numXpdGains);
static inline HAL_BOOL getLowerUpperIndex(u_int8_t target, u_int8_t *pList,
                          u_int16_t listSize,  u_int16_t *indexL,
                          u_int16_t *indexR);
static inline HAL_BOOL ar5416FillVpdTable(u_int8_t pwrMin, u_int8_t pwrMax,
                          u_int8_t *pPwrList, u_int8_t *pVpdList,
                          u_int16_t numIntercepts, u_int8_t *pRetVpdList);

#define EEPROM_DUMP 1   // TODO: Remove by FCS
#ifdef EEPROM_DUMP
void ar5416PrintPowerPerRate(struct ath_hal *ah, u_int16_t *pRatesPower,
                             u_int16_t red, u_int16_t reg, u_int16_t limit);
void ar5416EepromDump(struct ath_hal *ah, ar5416_eeprom_t *ar5416Eep);
#endif

/*****************************
 * Eeprom APIs for CB/XB only
 ****************************/

/*
 * Read 16 bits of data from offset into *data
 */
HAL_BOOL
ar5416EepromRead(struct ath_hal *ah, u_int off, u_int16_t *data)
{
    (void)OS_REG_READ(ah, AR5416_EEPROM_OFFSET + (off << AR5416_EEPROM_S));
    if (!ath_hal_wait(ah, AR_EEPROM_STATUS_DATA, AR_EEPROM_STATUS_DATA_BUSY
                      | AR_EEPROM_STATUS_DATA_PROT_ACCESS, 0)) {
        return AH_FALSE;
    }

    *data = MS(OS_REG_READ(ah, AR_EEPROM_STATUS_DATA), AR_EEPROM_STATUS_DATA_VAL);
    return AH_TRUE;
}

#ifdef AH_SUPPORT_WRITE_EEPROM
/*
 * Write 16 bits of data from data to the specified EEPROM offset.
 */
HAL_BOOL
ar5416EepromWrite(struct ath_hal *ah, u_int off, u_int16_t data)
{
        OS_REG_WR(ah, AR5416_EEPROM_OFFSET + (off << AR5416_EEPROM_S), data);
        return AH_TRUE;
}
#endif /* AH_SUPPORT_WRITE_EEPROM */

/*************************
 * Flash APIs for AP only
 *************************/

HAL_STATUS
ar5416FlashMap(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    ahp->ah_cal_mem = OS_REMAP(AR5416_EEPROM_START_ADDR, AR5416_EEPROM_MAX);
    if (!ahp->ah_cal_mem) {
        HALDEBUG(ah, "%s: cannot remap eeprom region \n", __func__);
        return HAL_EIO;
    }

    return HAL_OK;
}

HAL_BOOL
ar5416FlashRead(struct ath_hal *ah, u_int off, u_int16_t *data)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    *data = ((u_int16_t *)ahp->ah_cal_mem)[off];
    return AH_TRUE;
}

HAL_BOOL
ar5416FlashWrite(struct ath_hal *ah, u_int off, u_int16_t data)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    ((u_int16_t *)ahp->ah_cal_mem)[off] = data;
    return AH_TRUE;
}

/***************************
 * Common APIs for AP/CB/XB
 ***************************/

HAL_STATUS
ar5416EepromAttach(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    HAL_STATUS status;

    if (ar5416EepDataInFlash(ah))
        ar5416FlashMap(ah);

    if (!ar5416FillEeprom(ah))
        return HAL_EIO;

    status = ar5416CheckEeprom(ah);
    if (status != HAL_OK) {
        if (ath_hal_soft_eeprom) {
            ar5416FillEmuEeprom(ahp);
            ahp->ah_emu_eeprom = 1;
            return HAL_OK;
        } else {
            return status;
        }
    }

    return status;
}

u_int32_t
ar5416EepromGet(struct ath_hal_5416 *ahp, EEPROM_PARAM param)
{
    ar5416_eeprom_t *eep = &ahp->ah_eeprom;
    MODAL_EEP_HEADER *pModal = eep->modalHeader;
    BASE_EEP_HEADER  *pBase  = &eep->baseEepHeader;

    switch (param) {
        case EEP_NFTHRESH_5:
            return -pModal[0].noiseFloorThreshCh[0];
        case EEP_NFTHRESH_2:
            return -pModal[1].noiseFloorThreshCh[0];
        case AR_EEPROM_MAC(0):
            return pBase->macAddr[0] << 8 | pBase->macAddr[1];
        case AR_EEPROM_MAC(1):
            return pBase->macAddr[2] << 8 | pBase->macAddr[3];
        case AR_EEPROM_MAC(2):
            return pBase->macAddr[4] << 8 | pBase->macAddr[5];
        case EEP_REG_0:
            return pBase->regDmn[0];
        case EEP_REG_1:
            return pBase->regDmn[1];
        case EEP_OP_CAP:
            return pBase->deviceCap;
        case EEP_OP_MODE:
            return pBase->opCapFlags;
        case EEP_RF_SILENT:
            return pBase->rfSilent;
        case EEP_OB_5:
            return pModal[0].ob;
        case EEP_DB_5:
            return pModal[0].db;
        case EEP_OB_2:
            return pModal[1].ob;
        case EEP_DB_2:
            return pModal[1].db;
        case EEP_MINOR_REV:
            return pBase->version & AR5416_EEP_VER_MINOR_MASK;
        case EEP_TX_MASK:
            return pBase->txMask;
        case EEP_RX_MASK:
            return pBase->rxMask;
        default:
            HALASSERT(0);
            return 0;
    }
}

/*
 * Read EEPROM header info and program the device for correct operation
 * given the channel value.
 */
HAL_BOOL
ar5416EepromSetBoardValues(struct ath_hal *ah, HAL_HT *ht, HAL_CHANNEL_INTERNAL *chan)
{
    MODAL_EEP_HEADER *pModal;
    int i, regChainOffset;
    struct ath_hal_5416 *ahp = AH5416(ah);
    ar5416_eeprom_t *eep = &ahp->ah_eeprom;
    u_int8_t    txRxAttenLocal;    /* workaround for eeprom versions <= 14.2 */
    u_int8_t thresh62, thresh62_ext;

    HALASSERT(owl_get_eep_ver(ahp) == AR5416_EEP_VER);
    pModal = &(eep->modalHeader[IS_CHAN_2GHZ(chan)]);

    txRxAttenLocal = IS_CHAN_2GHZ(chan) ? 23 : 44;    /* workaround for eeprom versions < 14.3 */

    if (ahp->ah_emu_eeprom) {
        return AH_TRUE;
    }

    OS_REG_WRITE(ah, AR_PHY_SWITCH_COM, pModal->antCtrlCommon);

    for (i = 0; i < AR5416_MAX_CHAINS; i++) {
        if((AH_PRIVATE(ah)->ah_macRev >= AR_SREV_REVISION_OWL_20) && 
           ht &&
           (ht->misc.ht_rxchainmask == 5 || ht->misc.ht_txchainmask == 5) &&
           (i != 0))
        {
            /*
             * Regs are swapped from chain 2 to 1 for 5416 2_0 with 
             * only chains 0 and 2 populated
             */
            regChainOffset = (i == 1) ? 0x2000 : 0x1000;
        } else {
            regChainOffset = i * 0x1000;
        }

        OS_REG_WRITE(ah, AR_PHY_SWITCH_CHAIN_0 + regChainOffset, pModal->antCtrlChain[i]);

        OS_REG_WRITE(ah, AR_PHY_TIMING_CTRL4 + regChainOffset, 
            (OS_REG_READ(ah, AR_PHY_TIMING_CTRL4 + regChainOffset) &
                ~(AR_PHY_TIMING_CTRL4_IQCORR_Q_Q_COFF | AR_PHY_TIMING_CTRL4_IQCORR_Q_I_COFF)) |
                SM(pModal->iqCalICh[i], AR_PHY_TIMING_CTRL4_IQCORR_Q_I_COFF) |
                SM(pModal->iqCalQCh[i], AR_PHY_TIMING_CTRL4_IQCORR_Q_Q_COFF));

        if ((i == 0) || (AH_PRIVATE(ah)->ah_macRev >= AR_SREV_REVISION_OWL_20)) {

            if ((eep->baseEepHeader.version & AR5416_EEP_VER_MINOR_MASK) >= AR5416_EEP_MINOR_VER_3) {
                /*
                 * Large signal upgrade,
                 * If 14.3 or later EEPROM, use txRxAttenLocal = pModal->txRxAttenCh[i]
                 * else txRxAttenLocal is fixed value above.
                 */
                txRxAttenLocal = pModal->rxTxMarginCh[i];

                OS_REG_WRITE(ah, AR_PHY_GAIN_2GHZ + regChainOffset,
                    (OS_REG_READ(ah, AR_PHY_GAIN_2GHZ + regChainOffset) & ~AR_PHY_GAIN_2GHZ_BSW_MARGIN) |
                        SM(pModal->bswMargin[i], AR_PHY_GAIN_2GHZ_BSW_MARGIN));

                OS_REG_WRITE(ah, AR_PHY_GAIN_2GHZ + regChainOffset,
                    (OS_REG_READ(ah, AR_PHY_GAIN_2GHZ + regChainOffset) & ~AR_PHY_GAIN_2GHZ_BSW_ATTEN) |
                    SM(pModal->bswAtten[i], AR_PHY_GAIN_2GHZ_BSW_ATTEN));
            }

            OS_REG_WRITE(ah, AR_PHY_RXGAIN + regChainOffset, 
                (OS_REG_READ(ah, AR_PHY_RXGAIN + regChainOffset) & ~AR_PHY_RXGAIN_TXRX_ATTEN) |
            SM(txRxAttenLocal, AR_PHY_RXGAIN_TXRX_ATTEN));

            OS_REG_WRITE(ah, AR_PHY_GAIN_2GHZ + regChainOffset,
                (OS_REG_READ(ah, AR_PHY_GAIN_2GHZ + regChainOffset) & ~AR_PHY_GAIN_2GHZ_RXTX_MARGIN) |
                    SM(pModal->rxTxMarginCh[i], AR_PHY_GAIN_2GHZ_RXTX_MARGIN));
        }
    }

#if 0   /* Do not run IQ cal on AR5416 */
    /* write previous IQ results */
    OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4, AR_PHY_TIMING_CTRL4_IQCORR_ENABLE);
#endif

    OS_REG_RMW_FIELD(ah, AR_PHY_SETTLING, AR_PHY_SETTLING_SWITCH, pModal->switchSettling);
    OS_REG_RMW_FIELD(ah, AR_PHY_DESIRED_SZ, AR_PHY_DESIRED_SZ_ADC, pModal->adcDesiredSize);
    OS_REG_RMW_FIELD(ah, AR_PHY_DESIRED_SZ, AR_PHY_DESIRED_SZ_PGA, pModal->pgaDesiredSize);
    OS_REG_WRITE(ah, AR_PHY_RF_CTL4,
        SM(pModal->txEndToXpaOff, AR_PHY_RF_CTL4_TX_END_XPAA_OFF)
        | SM(pModal->txEndToXpaOff, AR_PHY_RF_CTL4_TX_END_XPAB_OFF)
        | SM(pModal->txFrameToXpaOn, AR_PHY_RF_CTL4_FRAME_XPAA_ON)
        | SM(pModal->txFrameToXpaOn, AR_PHY_RF_CTL4_FRAME_XPAB_ON));

    OS_REG_RMW_FIELD(ah, AR_PHY_RF_CTL3, AR_PHY_TX_END_TO_A2_RX_ON, pModal->txEndToRxOn);
    thresh62 = (ath_hal_thresh62 == 255) ? pModal->thresh62 : ath_hal_thresh62;
    thresh62_ext = (ath_hal_thresh62_ext == 255) ? 
                   pModal->thresh62 : ath_hal_thresh62_ext;
    OS_REG_RMW_FIELD(ah, AR_PHY_CCA, AR_PHY_CCA_THRESH62, thresh62);
    OS_REG_RMW_FIELD(ah, AR_PHY_EXT_CCA, AR_PHY_EXT_CCA_THRESH62, thresh62_ext);

    /* Minor Version Specific Application */
    if ((eep->baseEepHeader.version & AR5416_EEP_VER_MINOR_MASK) >= AR5416_EEP_MINOR_VER_2) {
        OS_REG_RMW_FIELD(ah, AR_PHY_RF_CTL2,  AR_PHY_TX_END_DATA_START, pModal->txFrameToDataStart);
        OS_REG_RMW_FIELD(ah, AR_PHY_RF_CTL2,  AR_PHY_TX_END_PA_ON, pModal->txFrameToPaOn);
    }

    if ((eep->baseEepHeader.version & AR5416_EEP_VER_MINOR_MASK) >= AR5416_EEP_MINOR_VER_3) {
        if (ht && ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
            /* Overwrite switch settling with HT40 value */
            OS_REG_RMW_FIELD(ah, AR_PHY_SETTLING, AR_PHY_SETTLING_SWITCH, pModal->swSettleHt40);
        }
    }

    return AH_TRUE;
}

/**************************************************************
 * ar5416EepromSetTransmitPower
 *
 * Set the transmit power in the baseband for the given
 * operating channel and mode.
 */
HAL_STATUS
ar5416EepromSetTransmitPower(struct ath_hal *ah, HAL_HT *ht,
     ar5416_eeprom_t *pEepData, HAL_CHANNEL_INTERNAL *chan, u_int16_t cfgCtl,
     u_int16_t twiceAntennaReduction, u_int16_t twiceMaxRegulatoryPower, 
     u_int16_t powerLimit)
{
#define POW_SM(_r, _s)     (((_r) & 0x3f) << (_s))

    MODAL_EEP_HEADER *pModal = &(pEepData->modalHeader[IS_CHAN_2GHZ(chan)]);
    struct ath_hal_5416 *ahp = AH5416(ah);
    int16_t ratesArray[Ar5416RateSize] = { 0 };
    int16_t  txPowerIndexOffset = 0;
    u_int8_t ht40PowerIncForPdadc = 2;
    int i;

    HALASSERT(owl_get_eep_ver(ahp) == AR5416_EEP_VER);

    if (ahp->ah_emu_eeprom)
        return HAL_OK;

    if ((pEepData->baseEepHeader.version & AR5416_EEP_VER_MINOR_MASK) >= AR5416_EEP_MINOR_VER_2) {
        ht40PowerIncForPdadc = pModal->ht40PowerIncForPdadc;
    }

    if (!ar5416SetPowerPerRateTable(ah, pEepData, ht, chan,
                                    &ratesArray[0],cfgCtl,
                                    twiceAntennaReduction, 
                                    twiceMaxRegulatoryPower, powerLimit)) {
        HALDEBUG(ah, "ar5416EepromSetTransmitPower: unable to set tx power per rate table\n");
        return HAL_EIO;
    }

    if (!ar5416SetPowerCalTable(ah, ht, pEepData, chan, &txPowerIndexOffset)) {
        HALDEBUG(ah, "ar5416EepromSetTransmitPower: unable to set power table\n");
        return HAL_EIO;
    }

    /*
     * txPowerIndexOffset is set by the SetPowerTable() call -
     *  adjust the rate table (0 offset if rates EEPROM not loaded)
     */
    for (i = 0; i < N(ratesArray); i++) {
        ratesArray[i] = (int16_t)(txPowerIndexOffset + ratesArray[i]);
        if (ratesArray[i] > AR5416_MAX_RATE_POWER)
            ratesArray[i] = AR5416_MAX_RATE_POWER;
#ifdef TEMP_POWER_CAP
        /*
         * workaround: cap on max power 
         * ratesArray table is in half dB steps
         */
        if (ratesArray[i] > 2 * ath_hal_maxTPC) {
            ratesArray[i] = 2 * ath_hal_maxTPC;
        }
#endif
    }

#ifdef EEPROM_DUMP
    ar5416PrintPowerPerRate(ah, ratesArray, twiceAntennaReduction,
                                twiceMaxRegulatoryPower, powerLimit);
#endif

    /* Write the OFDM power per rate set */
    OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE1,
        POW_SM(ratesArray[rate18mb], 24)
          | POW_SM(ratesArray[rate12mb], 16)
          | POW_SM(ratesArray[rate9mb],  8)
          | POW_SM(ratesArray[rate6mb],  0)
    );
    OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE2,
        POW_SM(ratesArray[rate54mb], 24)
          | POW_SM(ratesArray[rate48mb], 16)
          | POW_SM(ratesArray[rate36mb],  8)
          | POW_SM(ratesArray[rate24mb],  0)
    );

    if (IS_CHAN_2GHZ(chan)) {
        /* Write the CCK power per rate set */
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE3,
            POW_SM(ratesArray[rate2s], 24)
              | POW_SM(ratesArray[rate2l],  16)
              | POW_SM(ratesArray[rateXr],  8) /* XR target power */
              | POW_SM(ratesArray[rate1l],   0)
        );
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE4,
            POW_SM(ratesArray[rate11s], 24)
              | POW_SM(ratesArray[rate11l], 16)
              | POW_SM(ratesArray[rate5_5s],  8)
              | POW_SM(ratesArray[rate5_5l],  0)
        );
    }

    /* Write the HT20 power per rate set */
    OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE5,
        POW_SM(ratesArray[rateHt20_3], 24)
          | POW_SM(ratesArray[rateHt20_2],  16)
          | POW_SM(ratesArray[rateHt20_1],  8)
          | POW_SM(ratesArray[rateHt20_0],   0)
    );
    OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE6,
        POW_SM(ratesArray[rateHt20_7], 24)
          | POW_SM(ratesArray[rateHt20_6],  16)
          | POW_SM(ratesArray[rateHt20_5],  8)
          | POW_SM(ratesArray[rateHt20_4],   0)
    );

    if (ht && ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
        /* Write the HT40 power per rate set */
        /* Correct the difference between HT40 and HT20/Legacy */
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE7,
            POW_SM(ratesArray[rateHt40_3] + ht40PowerIncForPdadc, 24)
              | POW_SM(ratesArray[rateHt40_2] + ht40PowerIncForPdadc,  16)
              | POW_SM(ratesArray[rateHt40_1] + ht40PowerIncForPdadc,  8)
              | POW_SM(ratesArray[rateHt40_0] + ht40PowerIncForPdadc,   0)
        );
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE8,
            POW_SM(ratesArray[rateHt40_7] + ht40PowerIncForPdadc, 24)
              | POW_SM(ratesArray[rateHt40_6] + ht40PowerIncForPdadc,  16)
              | POW_SM(ratesArray[rateHt40_5] + ht40PowerIncForPdadc,  8)
              | POW_SM(ratesArray[rateHt40_4] + ht40PowerIncForPdadc,   0)
        );
        /* Write the Dup/Ext 40 power per rate set */
        OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE9,
            POW_SM(ratesArray[rateExtOfdm], 24)
              | POW_SM(ratesArray[rateExtCck],  16)
              | POW_SM(ratesArray[rateDupOfdm],  8)
              | POW_SM(ratesArray[rateDupCck],   0)
        );
    }

    /* Write the Power subtraction for dynamic chain changing, for per-packet powertx */
    OS_REG_WRITE(ah, AR_PHY_POWER_TX_SUB,
        POW_SM(pModal->pwrDecreaseFor3Chain, 6)
          | POW_SM(pModal->pwrDecreaseFor2Chain, 0)
    );

    /*
     * Return tx power used to iwconfig.
     * Since power is rate dependent, use one of the indices from the 
     * AR5416_RATES enum to select an entry from ratesArray[] to report.
     * Currently returns the power for HT40 MCS 0, HT20 MCS 0, or OFDM 6 Mbps
     * as CCK power is less interesting (?).
     */
    i = rate6mb;            /* legacy */
    if (ht && ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
        i = rateHt40_0;     /* ht40 */
    } else if (ht && chan && (chan->channelFlags & CHANNEL_HT20)) {
        i = rateHt20_0;     /* ht20 */
    }

    AH_PRIVATE(ah)->ah_maxPowerLevel = ratesArray[i];

#ifdef EEPROM_DUMP
    ath_hal_printf(ah,"2xMaxPowerLevel: %d (%s)\n",
        AH_PRIVATE(ah)->ah_maxPowerLevel,
        (i == rateHt40_0) ? "HT40" : (i == rateHt20_0) ? "HT20" : "LEG");
#endif
    return HAL_OK;
}

/***************************************
 * Helper functions common for AP/CB/XB
 **************************************/

/**************************************************************
 * ar5416SetPowerPerRateTable
 *
 * Sets the transmit power in the baseband for the given
 * operating channel and mode.
 */
static inline HAL_BOOL
ar5416SetPowerPerRateTable(struct ath_hal *ah, ar5416_eeprom_t *pEepData,
                           HAL_HT *ht, HAL_CHANNEL_INTERNAL *chan,
                           int16_t *ratesArray, u_int16_t cfgCtl,
                           u_int16_t AntennaReduction, 
                           u_int16_t twiceMaxRegulatoryPower,
                           u_int16_t powerLimit)
{
/* Local defines to distinguish between extension and control CTL's */
#define EXT_ADDITIVE (0x8000)
#define CTL_11A_EXT (CTL_11A | EXT_ADDITIVE)
#define CTL_11G_EXT (CTL_11G | EXT_ADDITIVE)
#define CTL_11B_EXT (CTL_11B | EXT_ADDITIVE)
    u_int16_t twiceMaxEdgePower = AR5416_MAX_RATE_POWER;
    int i;
    int16_t  twiceLargestAntenna;
    CAL_CTL_DATA *rep;
    CAL_TARGET_POWER_LEG targetPowerOfdm, targetPowerCck = {0, {0, 0, 0, 0}};
    CAL_TARGET_POWER_LEG targetPowerOfdmExt = {0, {0, 0, 0, 0}}, targetPowerCckExt = {0, {0, 0, 0, 0}};
    CAL_TARGET_POWER_HT  targetPowerHt20, targetPowerHt40 = {0, {0, 0, 0, 0}};
    int16_t scaledPower, minCtlPower;
    #define SUB_NUM_CTL_MODES_AT_5G_40 2    /* excluding HT40, EXT-OFDM */
    #define SUB_NUM_CTL_MODES_AT_2G_40 3    /* excluding HT40, EXT-OFDM, EXT-CCK */
    u_int16_t ctlModesFor11a[] = {CTL_11A, CTL_5GHT20, CTL_11A_EXT, CTL_5GHT40};
    u_int16_t ctlModesFor11g[] = {CTL_11B, CTL_11G, CTL_2GHT20, CTL_11B_EXT, CTL_11G_EXT, CTL_2GHT40};
    u_int16_t numCtlModes, *pCtlMode, ctlMode, freq;
    CHAN_CENTERS centers;
    int tx_chainmask;
    u_int16_t twiceMinEdgePower;
    int isHt40;

    tx_chainmask = (ht) ? ht->misc.ht_txchainmask : AR5416_LEGACY_CHAINMASK;

    ar5416GetChannelCenters(ah, ht, chan, &centers);

    /* Compute TxPower reduction due to Antenna Gain */
    twiceLargestAntenna = AH_MAX(AH_MAX(pEepData->modalHeader[IS_CHAN_2GHZ(chan)].antennaGainCh[0],
                                        pEepData->modalHeader[IS_CHAN_2GHZ(chan)].antennaGainCh[1]),
                                        pEepData->modalHeader[IS_CHAN_2GHZ(chan)].antennaGainCh[2]);
    twiceLargestAntenna =  (int16_t)AH_MIN((AntennaReduction) - twiceLargestAntenna, 0);

    /* scaledPower is the minimum of the user input power level and the regulatory allowed power level */
    scaledPower = AH_MIN(powerLimit, twiceMaxRegulatoryPower + twiceLargestAntenna);

    /* Reduce scaled Power by number of chains active to get to per chain tx power level */
    /* TODO: better value than these? */
    switch (owl_get_ntxchains(tx_chainmask)) {
    case 1:
        break;
    case 2:
        scaledPower -= pEepData->modalHeader[IS_CHAN_2GHZ(chan)].pwrDecreaseFor2Chain;
        break;
    case 3:
        scaledPower -= pEepData->modalHeader[IS_CHAN_2GHZ(chan)].pwrDecreaseFor3Chain;
        break;
    default:
        HALASSERT(0); /* Unsupported number of chains */
    }

    scaledPower = AH_MAX(0, scaledPower);
    isHt40 = (ht && ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) ? 1 : 0;

    /*
     * Get target powers from EEPROM - our baseline for TX Power
     */
    if (IS_CHAN_2GHZ(chan)) {
        /* Setup for CTL modes */
        numCtlModes = N(ctlModesFor11g) - SUB_NUM_CTL_MODES_AT_2G_40; /* CTL_11B, CTL_11G, CTL_2GHT20 */
        pCtlMode = ctlModesFor11g;

        ar5416GetTargetPowersLeg(ah, ht, chan, pEepData->calTargetPowerCck,
                              AR5416_NUM_2G_CCK_TARGET_POWERS, &targetPowerCck, 4, AH_FALSE);
        ar5416GetTargetPowersLeg(ah, ht, chan, pEepData->calTargetPower2G,
                              AR5416_NUM_2G_20_TARGET_POWERS, &targetPowerOfdm, 4, AH_FALSE);
        ar5416GetTargetPowers(ah, ht, chan, pEepData->calTargetPower2GHT20,
                              AR5416_NUM_2G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);

        if (isHt40) {
            numCtlModes = N(ctlModesFor11g);    /* All 2G CTL's */
            ar5416GetTargetPowers(ah, ht, chan, pEepData->calTargetPower2GHT40,
                                  AR5416_NUM_2G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            ar5416GetTargetPowersLeg(ah, ht, chan, pEepData->calTargetPowerCck,
                                     AR5416_NUM_2G_CCK_TARGET_POWERS, &targetPowerCckExt, 4, AH_TRUE);
            ar5416GetTargetPowersLeg(ah, ht, chan, pEepData->calTargetPower2G,
                                  AR5416_NUM_2G_20_TARGET_POWERS, &targetPowerOfdmExt, 4, AH_TRUE);
        }
    } else {
        /* Setup for CTL modes */
        numCtlModes = N(ctlModesFor11a) - SUB_NUM_CTL_MODES_AT_5G_40; /* CTL_11A, CTL_5GHT20 */
        pCtlMode = ctlModesFor11a;

        ar5416GetTargetPowersLeg(ah, ht, chan, pEepData->calTargetPower5G,
                              AR5416_NUM_5G_20_TARGET_POWERS, &targetPowerOfdm, 4, AH_FALSE);
        ar5416GetTargetPowers(ah, ht, chan, pEepData->calTargetPower5GHT20,
                              AR5416_NUM_5G_20_TARGET_POWERS, &targetPowerHt20, 8, AH_FALSE);

        if (isHt40) {
            numCtlModes = N(ctlModesFor11a); /* All 5G CTL's */
            ar5416GetTargetPowers(ah, ht, chan, pEepData->calTargetPower5GHT40,
                                  AR5416_NUM_5G_40_TARGET_POWERS, &targetPowerHt40, 8, AH_TRUE);
            ar5416GetTargetPowersLeg(ah, ht, chan, pEepData->calTargetPower5G,
                                  AR5416_NUM_5G_20_TARGET_POWERS, &targetPowerOfdmExt, 4, AH_TRUE);
        }
    }

    /* DEBUG */
    ath_hal_printf(ah, "ar5416SetPowerPerRateTable() syn %4d ctl %4d ext %4d is40 %d\n",
            centers.synth_center, centers.ctl_center, centers.ext_center, isHt40);

    /*
     * For MIMO, need to apply regulatory caps individually across dynamically
     * running modes: CCK, OFDM, HT20, HT40
     *
     * The outer loop walks through each possible applicable runtime mode.
     * The inner loop walks through each ctlIndex entry in EEPROM.
     * The ctl value is encoded as [7:4] == test group, [3:0] == test mode.
     *
     */
    for (ctlMode = 0; ctlMode < numCtlModes; ctlMode++) {

        HAL_BOOL isHt40CtlMode = (pCtlMode[ctlMode] == CTL_5GHT40) || (pCtlMode[ctlMode] == CTL_2GHT40);
        if (isHt40CtlMode) {
            freq = centers.ctl_center;
        } else if (pCtlMode[ctlMode] & EXT_ADDITIVE) {
            freq = centers.ext_center;
        } else {
            freq = centers.ctl_center;
        }

        /*
         * WAR for AP71 eeprom <= 14.2, does not have HT ctls's 
         * as a workaround, keep previously calculated twiceMaxEdgePower for
         * those CTL modes that do not match in EEPROM.  This is hacky and
         * works because 11g is processed before any HT -- but there is already 
         * other hacky code that has the same dependency.
         * To remove WAR, uncomment the following line.
         */
//        twiceMaxEdgePower = AR5416_MAX_RATE_POWER;
        // TODO: Does 14.3 still have this restriction???

#ifdef PRINT_CTL_POWER
        ath_hal_printf(ah, "LOOP-Mode ctlMode %d < %d, isHt40CtlMode %d, EXT_ADDITIVE %d\n",
            ctlMode, numCtlModes, isHt40CtlMode, (pCtlMode[ctlMode] & EXT_ADDITIVE));
#endif
        /* walk through each CTL index stored in EEPROM */
        for (i = 0; (i < AR5416_NUM_CTLS) && pEepData->ctlIndex[i]; i++) {

#ifdef PRINT_CTL_POWER
            ath_hal_printf(ah, "  LOOP-Ctlidx %d: cfgCtl 0x%2.2x pCtlMode 0x%2.2x ctlIndex 0x%2.2x chan %d chanctl 0x%x\n",
                i, cfgCtl, pCtlMode[ctlMode], pEepData->ctlIndex[i], chan->channel, chan->conformanceTestLimit);
#endif

            /* compare test group from regulatory channel list with test mode from pCtlMode list */
            if ((((cfgCtl & ~CTL_MODE_M) | (pCtlMode[ctlMode] & CTL_MODE_M)) == pEepData->ctlIndex[i]) ||
                (((cfgCtl & ~CTL_MODE_M) | (pCtlMode[ctlMode] & CTL_MODE_M)) == ((pEepData->ctlIndex[i] & CTL_MODE_M) | SD_NO_CTL)))
            {
                rep = &(pEepData->ctlData[i]);
                twiceMinEdgePower = ar5416GetMaxEdgePower(freq, rep->ctlEdges[owl_get_ntxchains(tx_chainmask)], IS_CHAN_2GHZ(chan));

#ifdef PRINT_CTL_POWER
                ath_hal_printf(ah, "    MATCH-EE_IDX %d: ch %d is2 %d 2xMinEdge %d chainmask %d chains %d\n",
                    i, freq, IS_CHAN_2GHZ(chan), twiceMinEdgePower, tx_chainmask, owl_get_ntxchains(tx_chainmask));
#endif

                if ((cfgCtl & ~CTL_MODE_M) == SD_NO_CTL) {
                    /* Find the minimum of all CTL edge powers that apply to this channel */
                    twiceMaxEdgePower = AH_MIN(twiceMaxEdgePower, twiceMinEdgePower);
                } else {
                    /* specific */
                    twiceMaxEdgePower = twiceMinEdgePower;
                    break;
                }
            }
        }

        minCtlPower = (u_int8_t)AH_MIN(twiceMaxEdgePower, scaledPower);

#ifdef PRINT_CTL_POWER
        ath_hal_printf(ah, "    SEL-Min ctlMode %d pCtlMode %d 2xMaxEdge %d sP %d minCtlPwr %d\n",
            ctlMode, pCtlMode[ctlMode], twiceMaxEdgePower, scaledPower, minCtlPower);
#endif


        /* Apply ctl mode to correct target power set */
        switch(pCtlMode[ctlMode]) {
        case CTL_11B:
            for (i = 0; i < N(targetPowerCck.tPow2x); i++) {
                targetPowerCck.tPow2x[i] = (u_int8_t)AH_MIN(targetPowerCck.tPow2x[i], minCtlPower);
            }
            break;
        case CTL_11A:
        case CTL_11G:
            for (i = 0; i < N(targetPowerOfdm.tPow2x); i++) {
                targetPowerOfdm.tPow2x[i] = (u_int8_t)AH_MIN(targetPowerOfdm.tPow2x[i], minCtlPower);
            }
            break;
        case CTL_5GHT20:
        case CTL_2GHT20:
            for (i = 0; i < N(targetPowerHt20.tPow2x); i++) {
                targetPowerHt20.tPow2x[i] = (u_int8_t)AH_MIN(targetPowerHt20.tPow2x[i], minCtlPower);
            }
            break;
        case CTL_11B_EXT:
            targetPowerCckExt.tPow2x[0] = (u_int8_t)AH_MIN(targetPowerCckExt.tPow2x[0], minCtlPower);
            break;
        case CTL_11A_EXT:
        case CTL_11G_EXT:
            targetPowerOfdmExt.tPow2x[0] = (u_int8_t)AH_MIN(targetPowerOfdmExt.tPow2x[0], minCtlPower);
            break;
        case CTL_5GHT40:
        case CTL_2GHT40:
            for (i = 0; i < N(targetPowerHt40.tPow2x); i++) {
                targetPowerHt40.tPow2x[i] = (u_int8_t)AH_MIN(targetPowerHt40.tPow2x[i], minCtlPower);
            }
            break;
        default:
            HALASSERT(0);
            break;
        }
    } /* end ctl mode checking */

    /* Set rates Array from collected data */
    ratesArray[rate6mb] = ratesArray[rate9mb] = ratesArray[rate12mb] = ratesArray[rate18mb] = ratesArray[rate24mb] = targetPowerOfdm.tPow2x[0];
    ratesArray[rate36mb] = targetPowerOfdm.tPow2x[1];
    ratesArray[rate48mb] = targetPowerOfdm.tPow2x[2];
    ratesArray[rate54mb] = targetPowerOfdm.tPow2x[3];
    ratesArray[rateXr] = targetPowerOfdm.tPow2x[0];

    for (i = 0; i < N(targetPowerHt20.tPow2x); i++) {
        ratesArray[rateHt20_0 + i] = targetPowerHt20.tPow2x[i];
    }

    if (IS_CHAN_2GHZ(chan)) {
        ratesArray[rate1l]  = targetPowerCck.tPow2x[0];
        ratesArray[rate2s] = ratesArray[rate2l]  = targetPowerCck.tPow2x[1];
        ratesArray[rate5_5s] = ratesArray[rate5_5l] = targetPowerCck.tPow2x[2];;
        ratesArray[rate11s] = ratesArray[rate11l] = targetPowerCck.tPow2x[3];;
    }
    if (ht && ht->cwm.ht_phymode == HAL_HT_PHYMODE_2040) {
        for (i = 0; i < N(targetPowerHt40.tPow2x); i++) {
            ratesArray[rateHt40_0 + i] = targetPowerHt40.tPow2x[i];
        }
        ratesArray[rateDupOfdm] = targetPowerHt40.tPow2x[0];
        ratesArray[rateDupCck]  = targetPowerHt40.tPow2x[0];
        ratesArray[rateExtOfdm] = targetPowerOfdmExt.tPow2x[0];
        if (IS_CHAN_2GHZ(chan)) {
            ratesArray[rateExtCck]  = targetPowerCckExt.tPow2x[0];
        }
    }
    return AH_TRUE;
#undef EXT_ADDITIVE
#undef CTL_11A_EXT
#undef CTL_11G_EXT
#undef CTL_11B_EXT
}
/**************************************************************
 * ar5416GetMaxEdgePower
 *
 * Find the maximum conformance test limit for the given channel and CTL info
 */
static inline u_int16_t
ar5416GetMaxEdgePower(u_int16_t freq, CAL_CTL_EDGES *pRdEdgesPower, HAL_BOOL is2GHz)
{
    u_int16_t twiceMaxEdgePower = AR5416_MAX_RATE_POWER;
    int      i;

    /* Get the edge power */
    for (i = 0; (i < AR5416_NUM_BAND_EDGES) && (pRdEdgesPower[i].bChannel != AR5416_BCHAN_UNUSED) ; i++) {
        /*
         * If there's an exact channel match or an inband flag set
         * on the lower channel use the given rdEdgePower
         */
        if (freq == fbin2freq(pRdEdgesPower[i].bChannel, is2GHz)) {
            twiceMaxEdgePower = pRdEdgesPower[i].tPower;
            break;
        } else if ((i > 0) && (freq < fbin2freq(pRdEdgesPower[i].bChannel, is2GHz))) {
            if (fbin2freq(pRdEdgesPower[i - 1].bChannel, is2GHz) < freq && pRdEdgesPower[i - 1].flag) {
                twiceMaxEdgePower = pRdEdgesPower[i - 1].tPower;
            }
            /* Leave loop - no more affecting edges possible in this monotonic increasing list */
            break;
        }
    }
    HALASSERT(twiceMaxEdgePower > 0);
    return twiceMaxEdgePower;
}

/**************************************************************
 * ar5416GetTargetPowers
 *
 * Return the rates of target power for the given target power table
 * channel, and number of channels
 */
static inline void
ar5416GetTargetPowers(struct ath_hal *ah, HAL_HT *ht, HAL_CHANNEL_INTERNAL *chan,
                      CAL_TARGET_POWER_HT *powInfo, u_int16_t numChannels,
                      CAL_TARGET_POWER_HT *pNewPower, u_int16_t numRates,
                      HAL_BOOL isHt40Target)
{
    u_int16_t clo, chi;
    int i;
    int matchIndex = -1, lowIndex = -1;
    u_int16_t freq;
    CHAN_CENTERS centers;

    ar5416GetChannelCenters(ah, ht, chan, &centers);
    freq = isHt40Target ? centers.synth_center : centers.ctl_center;

    /* Copy the target powers into the temp channel list */
    if (freq <= fbin2freq(powInfo[0].bChannel, IS_CHAN_2GHZ(chan))) {
        matchIndex = 0;
    } else {
        for (i = 0; (i < numChannels) && (powInfo[i].bChannel != AR5416_BCHAN_UNUSED); i++) {
            if (freq == fbin2freq(powInfo[i].bChannel, IS_CHAN_2GHZ(chan))) {
                matchIndex = i;
                break;
            } else if ((freq < fbin2freq(powInfo[i].bChannel, IS_CHAN_2GHZ(chan))) &&
                       (freq > fbin2freq(powInfo[i - 1].bChannel, IS_CHAN_2GHZ(chan))))
            {
                lowIndex = i - 1;
                break;
            }
        }
        if ((matchIndex == -1) && (lowIndex == -1)) {
            HALASSERT(freq > fbin2freq(powInfo[i - 1].bChannel, IS_CHAN_2GHZ(chan)));
            matchIndex = i - 1;
        }
    }

    if (matchIndex != -1) {
        *pNewPower = powInfo[matchIndex];
    } else {
        HALASSERT(lowIndex != -1);
        /*
         * Get the lower and upper channels, target powers,
         * and interpolate between them.
         */
        clo = fbin2freq(powInfo[lowIndex].bChannel, IS_CHAN_2GHZ(chan));
        chi = fbin2freq(powInfo[lowIndex + 1].bChannel, IS_CHAN_2GHZ(chan));

        for (i = 0; i < numRates; i++) {
            pNewPower->tPow2x[i] = (u_int8_t)interpolate(freq, clo, chi,
                                   powInfo[lowIndex].tPow2x[i], powInfo[lowIndex + 1].tPow2x[i]);
        }
    }
}

/**************************************************************
 * ar5416GetTargetPowersLeg
 *
 * Return the four rates of target power for the given target power table
 * channel, and number of channels
 */
static inline void
ar5416GetTargetPowersLeg(struct ath_hal *ah, HAL_HT *ht,
                         HAL_CHANNEL_INTERNAL *chan,
                         CAL_TARGET_POWER_LEG *powInfo, u_int16_t numChannels,
                         CAL_TARGET_POWER_LEG *pNewPower, u_int16_t numRates,
                         HAL_BOOL isExtTarget)
{
    u_int16_t clo, chi;
    int i;
    int matchIndex = -1, lowIndex = -1;
    u_int16_t freq;
    CHAN_CENTERS centers;

    ar5416GetChannelCenters(ah, ht, chan, &centers);
    freq = (isExtTarget) ? centers.ext_center : centers.ctl_center;

    /* Copy the target powers into the temp channel list */
    if (freq <= fbin2freq(powInfo[0].bChannel, IS_CHAN_2GHZ(chan))) {
        matchIndex = 0;
    } else {
        for (i = 0; (i < numChannels) && (powInfo[i].bChannel != AR5416_BCHAN_UNUSED); i++) {
            if (freq == fbin2freq(powInfo[i].bChannel, IS_CHAN_2GHZ(chan))) {
                matchIndex = i;
                break;
            } else if ((freq < fbin2freq(powInfo[i].bChannel, IS_CHAN_2GHZ(chan))) &&
                       (freq > fbin2freq(powInfo[i - 1].bChannel, IS_CHAN_2GHZ(chan))))
            {
                lowIndex = i - 1;
                break;
            }
        }
        if ((matchIndex == -1) && (lowIndex == -1)) {
            HALASSERT(freq > fbin2freq(powInfo[i - 1].bChannel, IS_CHAN_2GHZ(chan)));
            matchIndex = i - 1;
        }
    }

    if (matchIndex != -1) {
        *pNewPower = powInfo[matchIndex];
    } else {
        HALASSERT(lowIndex != -1);
        /*
         * Get the lower and upper channels, target powers,
         * and interpolate between them.
         */
        clo = fbin2freq(powInfo[lowIndex].bChannel, IS_CHAN_2GHZ(chan));
        chi = fbin2freq(powInfo[lowIndex + 1].bChannel, IS_CHAN_2GHZ(chan));

        for (i = 0; i < numRates; i++) {
            pNewPower->tPow2x[i] = (u_int8_t)interpolate(freq, clo, chi,
                                   powInfo[lowIndex].tPow2x[i], powInfo[lowIndex + 1].tPow2x[i]);
        }
    }
}

/**************************************************************
 * ar5416SetPowerCalTable
 *
 * Pull the PDADC piers from cal data and interpolate them across the given
 * points as well as from the nearest pier(s) to get a power detector
 * linear voltage to power level table.
 */
static inline HAL_BOOL
ar5416SetPowerCalTable(struct ath_hal *ah, HAL_HT *ht, ar5416_eeprom_t *pEepData, HAL_CHANNEL_INTERNAL *chan, int16_t *pTxPowerIndexOffset)
{
    CAL_DATA_PER_FREQ *pRawDataset;
    u_int8_t  *pCalBChans = AH_NULL;
    u_int16_t pdGainOverlap_t2;
    static u_int8_t  pdadcValues[AR5416_NUM_PDADC_VALUES];
    u_int16_t gainBoundaries[AR5416_PD_GAINS_IN_MASK];
    u_int16_t numPiers, i, j;
    int16_t  tMinCalPower;
    u_int16_t numXpdGain, xpdMask;
    u_int16_t xpdGainValues[AR5416_NUM_PD_GAINS] = {0, 0, 0, 0};
    u_int32_t reg32, regOffset, regChainOffset;
    int16_t   modalIdx;

    modalIdx = IS_CHAN_2GHZ(chan) ? 1 : 0;
    xpdMask = pEepData->modalHeader[modalIdx].xpdGain;

    if ((pEepData->baseEepHeader.version & AR5416_EEP_VER_MINOR_MASK) >= AR5416_EEP_MINOR_VER_2) {
        pdGainOverlap_t2 = pEepData->modalHeader[modalIdx].pdGainOverlap;
    } else {
        pdGainOverlap_t2 = (u_int16_t)(MS(OS_REG_READ(ah, AR_PHY_TPCRG5), AR_PHY_TPCRG5_PD_GAIN_OVERLAP));
    }

    if (IS_CHAN_2GHZ(chan)) {
        pCalBChans = pEepData->calFreqPier2G;
        numPiers = AR5416_NUM_2G_CAL_PIERS;
    } else {
        pCalBChans = pEepData->calFreqPier5G;
        numPiers = AR5416_NUM_5G_CAL_PIERS;
    }

    numXpdGain = 0;
    /* Calculate the value of xpdgains from the xpdGain Mask */
    for (i = 1; i <= AR5416_PD_GAINS_IN_MASK; i++) {
        if ((xpdMask >> (AR5416_PD_GAINS_IN_MASK - i)) & 1) {
            if (numXpdGain >= AR5416_NUM_PD_GAINS) {
                HALASSERT(0);
                break;
            }
            xpdGainValues[numXpdGain] = (u_int16_t)(AR5416_PD_GAINS_IN_MASK - i);
            numXpdGain++;
        }
    }

    /* write the detector gain biases and their number */
    OS_REG_RMW_FIELD(ah, AR_PHY_TPCRG1, AR_PHY_TPCRG1_NUM_PD_GAIN, (numXpdGain - 1) & 0x3);
    OS_REG_RMW_FIELD(ah, AR_PHY_TPCRG1, AR_PHY_TPCRG1_PD_GAIN_1, xpdGainValues[0]);
    OS_REG_RMW_FIELD(ah, AR_PHY_TPCRG1, AR_PHY_TPCRG1_PD_GAIN_2, xpdGainValues[1]);
    OS_REG_RMW_FIELD(ah, AR_PHY_TPCRG1, AR_PHY_TPCRG1_PD_GAIN_3, xpdGainValues[2]);

    for (i = 0; i < AR5416_MAX_CHAINS; i++) {
        if ((AH_PRIVATE(ah)->ah_macRev >= AR_SREV_REVISION_OWL_20) && 
            ht &&
            (ht->misc.ht_rxchainmask == 5 || ht->misc.ht_txchainmask == 5) &&
            (i != 0))
        {
            /* Regs are swapped from chain 2 to 1 for 5416 2_0 with 
             * only chains 0 and 2 populated 
             */
            regChainOffset = (i == 1) ? 0x2000 : 0x1000;
        } else {
            regChainOffset = i * 0x1000;
        }
        if (pEepData->baseEepHeader.txMask & (1 << i)) {
            if (IS_CHAN_2GHZ(chan)) {
                pRawDataset = pEepData->calPierData2G[i];
            } else {
                pRawDataset = pEepData->calPierData5G[i];
            }

            ar5416GetGainBoundariesAndPdadcs(ah, ht, chan, pRawDataset,
                                             pCalBChans, numPiers,
                                             pdGainOverlap_t2,
                                             &tMinCalPower, gainBoundaries,
                                             pdadcValues, numXpdGain);

            if ((i == 0) || 
                (AH_PRIVATE(ah)->ah_macRev >= AR_SREV_REVISION_OWL_20)) {
                /*
                 * Note the pdadc table may not start at 0 dBm power, could be
                 * negative or greater than 0.  Need to offset the power
                 * values by the amount of minPower for griffin
                 */

                OS_REG_WRITE(ah, AR_PHY_TPCRG5 + regChainOffset,
                     SM(pdGainOverlap_t2, AR_PHY_TPCRG5_PD_GAIN_OVERLAP) |
                     SM(gainBoundaries[0], AR_PHY_TPCRG5_PD_GAIN_BOUNDARY_1)  |
                     SM(gainBoundaries[1], AR_PHY_TPCRG5_PD_GAIN_BOUNDARY_2)  |
                     SM(gainBoundaries[2], AR_PHY_TPCRG5_PD_GAIN_BOUNDARY_3)  |
                     SM(gainBoundaries[3], AR_PHY_TPCRG5_PD_GAIN_BOUNDARY_4));
            }

            /*
             * Write the power values into the baseband power table
             */
            regOffset = AR_PHY_BASE + (672 << 2) + regChainOffset;
            for (j = 0; j < 32; j++) {
                reg32 = ((pdadcValues[4*j + 0] & 0xFF) << 0)  |
                    ((pdadcValues[4*j + 1] & 0xFF) << 8)  |
                    ((pdadcValues[4*j + 2] & 0xFF) << 16) |
                    ((pdadcValues[4*j + 3] & 0xFF) << 24) ;
                OS_REG_WRITE(ah, regOffset, reg32);

#ifdef PDADC_DUMP_RAW
                ath_hal_printf(ah, "PDADC (%d,%4x): %4.4x %8.8x\n", i, regChainOffset, regOffset, reg32);
#endif
#ifdef PDADC_DUMP
                ath_hal_printf(ah, "PDADC: Chain %d | PDADC %3d Value %3d | PDADC %3d Value %3d | PDADC %3d Value %3d | PDADC %3d Value %3d |\n",
                    i,
                    4*j, pdadcValues[4*j],
                    4*j+1, pdadcValues[4*j + 1],
                    4*j+2, pdadcValues[4*j + 2],
                    4*j+3, pdadcValues[4*j + 3]);
#endif
                regOffset += 4;
            }
        }
    }
    *pTxPowerIndexOffset = 0;

    return AH_TRUE;
}

/**************************************************************
 * ar5416GetGainBoundariesAndPdadcs
 *
 * Uses the data points read from EEPROM to reconstruct the pdadc power table
 * Called by ar5416SetPowerCalTable only.
 */
static inline void
ar5416GetGainBoundariesAndPdadcs(struct ath_hal *ah, HAL_HT *ht,
                                 HAL_CHANNEL_INTERNAL *chan, CAL_DATA_PER_FREQ *pRawDataSet,
                                 u_int8_t * bChans,  u_int16_t availPiers,
                                 u_int16_t tPdGainOverlap, int16_t *pMinCalPower, u_int16_t * pPdGainBoundaries,
                                 u_int8_t * pPDADCValues, u_int16_t numXpdGains)
{
    int       i, j, k;
    int16_t   ss;         /* potentially -ve index for taking care of pdGainOverlap */
    u_int16_t  idxL, idxR, numPiers; /* Pier indexes */

    /* filled out Vpd table for all pdGains (chanL) */
    static u_int8_t   vpdTableL[AR5416_NUM_PD_GAINS][AR5416_MAX_PWR_RANGE_IN_HALF_DB];

    /* filled out Vpd table for all pdGains (chanR) */
    static u_int8_t   vpdTableR[AR5416_NUM_PD_GAINS][AR5416_MAX_PWR_RANGE_IN_HALF_DB];

    /* filled out Vpd table for all pdGains (interpolated) */
    static u_int8_t   vpdTableI[AR5416_NUM_PD_GAINS][AR5416_MAX_PWR_RANGE_IN_HALF_DB];

    u_int8_t   *pVpdL, *pVpdR, *pPwrL, *pPwrR;
    u_int8_t   minPwrT4[AR5416_NUM_PD_GAINS];
    u_int8_t   maxPwrT4[AR5416_NUM_PD_GAINS];
    int16_t   vpdStep;
    int16_t   tmpVal;
    u_int16_t  sizeCurrVpdTable, maxIndex, tgtIndex;
    HAL_BOOL    match;
    int16_t  minDelta = 0;
    CHAN_CENTERS centers;

    ar5416GetChannelCenters(ah, ht, chan, &centers);

    /* Trim numPiers for the number of populated channel Piers */
    for (numPiers = 0; numPiers < availPiers; numPiers++) {
        if (bChans[numPiers] == AR5416_BCHAN_UNUSED) {
            break;
        }
    }

    /* Find pier indexes around the current channel */
    match = getLowerUpperIndex((u_int8_t)FREQ2FBIN(centers.synth_center, IS_CHAN_2GHZ(chan)),
        bChans, numPiers, &idxL,
        &idxR);

    if (match) {
        /* Directly fill both vpd tables from the matching index */
        for (i = 0; i < numXpdGains; i++) {
            minPwrT4[i] = pRawDataSet[idxL].pwrPdg[i][0];
            maxPwrT4[i] = pRawDataSet[idxL].pwrPdg[i][4];
            ar5416FillVpdTable(minPwrT4[i], maxPwrT4[i], pRawDataSet[idxL].pwrPdg[i],
                               pRawDataSet[idxL].vpdPdg[i], AR5416_PD_GAIN_ICEPTS, vpdTableI[i]);
        }
    } else {
        for (i = 0; i < numXpdGains; i++) {
            pVpdL = pRawDataSet[idxL].vpdPdg[i];
            pPwrL = pRawDataSet[idxL].pwrPdg[i];
            pVpdR = pRawDataSet[idxR].vpdPdg[i];
            pPwrR = pRawDataSet[idxR].pwrPdg[i];

            /* Start Vpd interpolation from the max of the minimum powers */
            minPwrT4[i] = AH_MAX(pPwrL[0], pPwrR[0]);

            /* End Vpd interpolation from the min of the max powers */
            maxPwrT4[i] = AH_MIN(pPwrL[AR5416_PD_GAIN_ICEPTS - 1], pPwrR[AR5416_PD_GAIN_ICEPTS - 1]);
            HALASSERT(maxPwrT4[i] > minPwrT4[i]);

            /* Fill pier Vpds */
            ar5416FillVpdTable(minPwrT4[i], maxPwrT4[i], pPwrL, pVpdL, AR5416_PD_GAIN_ICEPTS, vpdTableL[i]);
            ar5416FillVpdTable(minPwrT4[i], maxPwrT4[i], pPwrR, pVpdR, AR5416_PD_GAIN_ICEPTS, vpdTableR[i]);

            /* Interpolate the final vpd */
            for (j = 0; j <= (maxPwrT4[i] - minPwrT4[i]) / 2; j++) {
                vpdTableI[i][j] = (u_int8_t)(interpolate((u_int16_t)FREQ2FBIN(centers.synth_center, IS_CHAN_2GHZ(chan)),
                    bChans[idxL], bChans[idxR], vpdTableL[i][j], vpdTableR[i][j]));
            }
        }
    }
    *pMinCalPower = (int16_t)(minPwrT4[0] / 2);

    k = 0; /* index for the final table */
    for (i = 0; i < numXpdGains; i++) {
        if (i == (numXpdGains - 1)) {
            pPdGainBoundaries[i] = (u_int16_t)(maxPwrT4[i] / 2);
        } else {
            pPdGainBoundaries[i] = (u_int16_t)((maxPwrT4[i] + minPwrT4[i+1]) / 4);
        }

        pPdGainBoundaries[i] = (u_int16_t)AH_MIN(AR5416_MAX_RATE_POWER, pPdGainBoundaries[i]);


        /*
         * WORKAROUND for 5416 1.0 until we get a per chain gain boundary 
         * register. This is not the best solution
         */
        if ((i == 0) && 
            !(AH_PRIVATE(ah)->ah_macRev >= AR_SREV_REVISION_OWL_20)) {
            //fix the gain delta, but get a delta that can be applied to min to
            //keep the upper power values accurate, don't think max needs to
            //be adjusted because should not be at that area of the table?
            minDelta = pPdGainBoundaries[0] - 23;
            pPdGainBoundaries[0] = 23;
        } else {
            minDelta = 0;
        }

        /* Find starting index for this pdGain */
        if (i == 0) {
            ss = 0; /* for the first pdGain, start from index 0 */
        } else {
            /* Need overlap entries extrapolated below */
            ss = (int16_t)((pPdGainBoundaries[i-1] - (minPwrT4[i] / 2)) - tPdGainOverlap + 1 + minDelta);
        }
        vpdStep = (int16_t)(vpdTableI[i][1] - vpdTableI[i][0]);
        vpdStep = (int16_t)((vpdStep < 1) ? 1 : vpdStep);
        /*
         *-ve ss indicates need to extrapolate data below for this pdGain
         */
        while ((ss < 0) && (k < (AR5416_NUM_PDADC_VALUES - 1))) {
            tmpVal = (int16_t)(vpdTableI[i][0] + ss * vpdStep);
            pPDADCValues[k++] = (u_int8_t)((tmpVal < 0) ? 0 : tmpVal);
            ss++;
        }

        sizeCurrVpdTable = (u_int8_t)((maxPwrT4[i] - minPwrT4[i]) / 2 + 1);
        tgtIndex = (u_int8_t)(pPdGainBoundaries[i] + tPdGainOverlap - (minPwrT4[i] / 2));
        maxIndex = (tgtIndex < sizeCurrVpdTable) ? tgtIndex : sizeCurrVpdTable;

        while ((ss < maxIndex) && (k < (AR5416_NUM_PDADC_VALUES - 1))) {
            pPDADCValues[k++] = vpdTableI[i][ss++];
        }

        vpdStep = (int16_t)(vpdTableI[i][sizeCurrVpdTable - 1] - vpdTableI[i][sizeCurrVpdTable - 2]);
        vpdStep = (int16_t)((vpdStep < 1) ? 1 : vpdStep);
        /*
         * for last gain, pdGainBoundary == Pmax_t2, so will
         * have to extrapolate
         */
        if (tgtIndex > maxIndex) {  /* need to extrapolate above */
            while ((ss <= tgtIndex) && (k < (AR5416_NUM_PDADC_VALUES - 1))) {
                tmpVal = (int16_t)((vpdTableI[i][sizeCurrVpdTable - 1] +
                          (ss - maxIndex + 1) * vpdStep));
                pPDADCValues[k++] = (u_int8_t)((tmpVal > 255) ? 255 : tmpVal);
                ss++;
            }
        }               /* extrapolated above */
    }                   /* for all pdGainUsed */

    /* Fill out pdGainBoundaries - only up to 2 allowed here, but hardware allows up to 4 */
    while (i < AR5416_PD_GAINS_IN_MASK) {
        pPdGainBoundaries[i] = pPdGainBoundaries[i-1];
        i++;
    }

    while (k < AR5416_NUM_PDADC_VALUES) {
        pPDADCValues[k] = pPDADCValues[k-1];
        k++;
    }
    return;
}

/**************************************************************
 * getLowerUppderIndex
 *
 * Return indices surrounding the value in sorted integer lists.
 * Requirement: the input list must be monotonically increasing
 *     and populated up to the list size
 * Returns: match is set if an index in the array matches exactly
 *     or a the target is before or after the range of the array.
 */
static inline HAL_BOOL
getLowerUpperIndex(u_int8_t target, u_int8_t *pList, u_int16_t listSize,
                   u_int16_t *indexL, u_int16_t *indexR)
{
    u_int16_t i;

    /*
     * Check first and last elements for beyond ordered array cases.
     */
    if (target <= pList[0]) {
        *indexL = *indexR = 0;
        return AH_TRUE;
    }
    if (target >= pList[listSize-1]) {
        *indexL = *indexR = (u_int16_t)(listSize - 1);
        return AH_TRUE;
    }

    /* look for value being near or between 2 values in list */
    for (i = 0; i < listSize - 1; i++) {
        /*
         * If value is close to the current value of the list
         * then target is not between values, it is one of the values
         */
        if (pList[i] == target) {
            *indexL = *indexR = i;
            return AH_TRUE;
        }
        /*
         * Look for value being between current value and next value
         * if so return these 2 values
         */
        if (target < pList[i + 1]) {
            *indexL = i;
            *indexR = (u_int16_t)(i + 1);
            return AH_FALSE;
        }
    }
    HALASSERT(0);
    return AH_FALSE;
}

/**************************************************************
 * ar5416FillVpdTable
 *
 * Fill the Vpdlist for indices Pmax-Pmin
 * Note: pwrMin, pwrMax and Vpdlist are all in dBm * 4
 */
static inline HAL_BOOL
ar5416FillVpdTable(u_int8_t pwrMin, u_int8_t pwrMax, u_int8_t *pPwrList,
                   u_int8_t *pVpdList, u_int16_t numIntercepts, u_int8_t *pRetVpdList)
{
    u_int16_t  i, k;
    u_int8_t   currPwr = pwrMin;
    u_int16_t  idxL, idxR;

    HALASSERT(pwrMax > pwrMin);
    for (i = 0; i <= (pwrMax - pwrMin) / 2; i++) {
        getLowerUpperIndex(currPwr, pPwrList, numIntercepts,
                           &(idxL), &(idxR));
        if (idxR < 1)
            idxR = 1;           /* extrapolate below */
        if (idxL == numIntercepts - 1)
            idxL = (u_int16_t)(numIntercepts - 2);   /* extrapolate above */
        if (pPwrList[idxL] == pPwrList[idxR])
            k = pVpdList[idxL];
        else
            k = (u_int16_t)( ((currPwr - pPwrList[idxL]) * pVpdList[idxR] + (pPwrList[idxR] - currPwr) * pVpdList[idxL]) /
                  (pPwrList[idxR] - pPwrList[idxL]) );
        HALASSERT(k < 256);
        pRetVpdList[i] = (u_int8_t)k;
        currPwr += 2;               /* half dB steps */
    }

    return AH_TRUE;
}

/**************************************************************************
 * interpolate
 *
 * Returns signed interpolated or the scaled up interpolated value
 */
static inline int16_t
interpolate(u_int16_t target, u_int16_t srcLeft, u_int16_t srcRight,
            int16_t targetLeft, int16_t targetRight)
{
    int16_t rv;

    if (srcRight == srcLeft) {
        rv = targetLeft;
    } else {
        rv = (int16_t)( ((target - srcLeft) * targetRight +
              (srcRight - target) * targetLeft) / (srcRight - srcLeft) );
    }
    return rv;
}

/**************************************************************************
 * fbin2freq
 *
 * Get channel value from binary representation held in eeprom
 * RETURNS: the frequency in MHz
 */
static inline u_int16_t
fbin2freq(u_int8_t fbin, HAL_BOOL is2GHz)
{
    /*
     * Reserved value 0xFF provides an empty definition both as
     * an fbin and as a frequency - do not convert
     */
    if (fbin == AR5416_BCHAN_UNUSED) {
        return fbin;
    }

    return (u_int16_t)((is2GHz) ? (2300 + fbin) : (4800 + 5 * fbin));
}

static inline HAL_STATUS
ar5416CheckEeprom(struct ath_hal *ah)
{
    u_int32_t sum = 0, el;
    u_int16_t *eepdata;
    int i;
    struct ath_hal_5416 *ahp = AH5416(ah);
    HAL_BOOL need_swap = AH_FALSE;

#define AR5416_EEPROM_MAGIC         0x5aa5
#define AR5416_EEPROM_MAGIC_OFFSET  0x0

    /* eeprom size is hardcoded to 4k */
#if 0
    if (!owl_eeprom_read(ah, AR_EEPROM_PROTECT, &eeval)) {
        HALDEBUG(ah, "%s: cannot read EEPROM protection "
            "bits; read locked?\n", __func__);
        return HAL_EEREAD;
    }
    HALDEBUG(ah, "EEPROM protect 0x%x\n", eeval);
    ahp->ah_eeprotect = eeval;
    /* XXX check proper access before continuing */
#endif

    if (!ar5416EepDataInFlash(ah)) {
        u_int16_t magic, magic2;
        int addr;
 
        if (!ahp->ah_priv.ah_eepromRead(ah, AR5416_EEPROM_MAGIC_OFFSET, 
                                        &magic)) {
            HALDEBUG(ah, "%s: Reading Magic # failed\n", __func__);
            return AH_FALSE;
        }
        HALDEBUG(ah, "%s: Read Magic = 0x%04X\n", __func__, magic);
 
        if (magic != AR5416_EEPROM_MAGIC) {
 
            magic2 = __bswap16(magic);
            if (magic2 == AR5416_EEPROM_MAGIC) {
                need_swap = AH_TRUE;
    eepdata = (u_int16_t *)(&ahp->ah_eeprom);
                for (addr=0; addr<sizeof(ar5416_eeprom_t)/sizeof(u_int16_t); 
                     addr++)  {
                    u_int16_t temp;
                    temp = __bswap16(*eepdata);
                    *eepdata = temp;
                    eepdata++;
 
                    HALDEBUG(ah, "0x%04X  ", *eepdata);
                    if (((addr+1)%6) == 0) 
                        HALDEBUG(ah, "\n");
                }
            }
            else {
                HALDEBUG(ah, "Invalid EEPROM Magic. endianness missmatch.\n");
                return HAL_EEBADSUM;
            }
        }
    }
    HALDEBUG(ah, "need_swap = %s.\n", need_swap?"True":"False");
 
    if (need_swap) {
        el = __bswap16(ahp->ah_eeprom.baseEepHeader.length);
    } else {
        el = ahp->ah_eeprom.baseEepHeader.length;
    }

    eepdata = (u_int16_t *)(&ahp->ah_eeprom);
    for (i = 0; i < AH_MIN(el, sizeof(ar5416_eeprom_t))/sizeof(u_int16_t); i++)
        sum ^= *eepdata++;

    if (need_swap) {
        /*
         *  preddy: EEPROM endianness does not match. So change it
         *  8bit values in eeprom data structure does not need to be swapped
         *  Only >8bits (16 & 32) values need to be swapped
         *  If a new 16 or 32 bit field is added to the EEPROM contents, 
         *  please make sure to swap the field here
         */
        u_int32_t integer,i,j;
        u_int16_t word;
        MODAL_EEP_HEADER *pModal;
        ar5416_eeprom_t *eep = (ar5416_eeprom_t *)&ahp->ah_eeprom;

        HALDEBUG(ah, "EEPROM Endianness is not native.. Changing \n");

        /* convert Base Eep header */
        word = __bswap16(eep->baseEepHeader.length);
        eep->baseEepHeader.length = word;
  
        word = __bswap16(eep->baseEepHeader.checksum);
        eep->baseEepHeader.checksum = word;
  
        word = __bswap16(eep->baseEepHeader.version);
        eep->baseEepHeader.version = word;
  
        word = __bswap16(eep->baseEepHeader.regDmn[0]);
        eep->baseEepHeader.regDmn[0] = word;
  
        word = __bswap16(eep->baseEepHeader.regDmn[1]);
        eep->baseEepHeader.regDmn[1] = word;
  
        word = __bswap16(eep->baseEepHeader.rfSilent);
        eep->baseEepHeader.rfSilent = word;
  
        word = __bswap16(eep->baseEepHeader.blueToothOptions);
        eep->baseEepHeader.blueToothOptions = word;
  
        word = __bswap16(eep->baseEepHeader.deviceCap);
        eep->baseEepHeader.deviceCap = word;

        /* convert Modal Eep header */
        for (j = 0; j < N(eep->modalHeader); j++) {
            pModal = &eep->modalHeader[j];

            integer = __bswap32(pModal->antCtrlCommon);
            pModal->antCtrlCommon = integer;

            for (i = 0; i < AR5416_MAX_CHAINS; i++) {
                integer = __bswap32(pModal->antCtrlChain[i]);
                pModal->antCtrlChain[i] = integer;
            }

            for (i = 0; i < AR5416_EEPROM_MODAL_SPURS; i++) {
                word = __bswap16(pModal->spurChans[i].spurChan);
                pModal->spurChans[i].spurChan = word;
        }
        }
    }

    /* Check CRC - Attach should fail on a bad checksum */
    if (sum != 0xffff || owl_get_eep_ver(ahp) != AR5416_EEP_VER ||
        owl_get_eep_rev(ahp) < AR5416_EEP_NO_BACK_VER) {
        HALDEBUG(ah, "Bad EEPROM checksum 0x%x or revision 0x%04x\n",
                 sum, owl_get_eep_ver(ahp));
        return HAL_EEBADSUM;
    }

    return HAL_OK;
}

static inline void
ar5416FillEmuEeprom(struct ath_hal_5416 *ahp)
{
    ar5416_eeprom_t *eep = &ahp->ah_eeprom;

    eep->baseEepHeader.version = AR5416_EEP_VER << 12;
    eep->baseEepHeader.macAddr[0] = 0x00;
    eep->baseEepHeader.macAddr[1] = 0x03;
    eep->baseEepHeader.macAddr[2] = 0x7F;
    eep->baseEepHeader.macAddr[3] = 0xBA;
    eep->baseEepHeader.macAddr[4] = 0xD0;
    eep->baseEepHeader.regDmn[0] = 0;
    eep->baseEepHeader.opCapFlags = AR5416_OPFLAGS_11G | AR5416_OPFLAGS_11A;
    eep->baseEepHeader.deviceCap = 0;
    eep->baseEepHeader.rfSilent = 0;
    eep->modalHeader[0].noiseFloorThreshCh[0] = -1;
    eep->modalHeader[1].noiseFloorThreshCh[0] = -1;
}

static inline HAL_BOOL
ar5416FillEeprom(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    ar5416_eeprom_t *eep = &ahp->ah_eeprom;
    u_int16_t *eep_data;
    int addr, owl_eep_start_loc = 0;

    if (!ar5416EepDataInFlash(ah)) {
        HALDEBUG(ah, "%s: Reading from EEPROM, not flash\n", __func__);
        owl_eep_start_loc = 256;
    }

    eep_data = (u_int16_t *)eep;
    for (addr = 0; addr < sizeof(ar5416_eeprom_t) / sizeof(u_int16_t); 
         addr++) {
        if (!ahp->ah_priv.ah_eepromRead(ah, addr + owl_eep_start_loc, 
                                        eep_data)) {
           HALDEBUG(ah, "%s: Unable to read eeprom region \n", __func__);
           return AH_FALSE;
        }

        eep_data ++;
    }

    return AH_TRUE;
}

#ifdef EEPROM_DUMP
void
ar5416PrintPowerPerRate(struct ath_hal *ah, u_int16_t *pRatesPower,
                             u_int16_t red, u_int16_t reg, u_int16_t limit)
{
    const u_int8_t *rateString[] = {" 6mb OFDM", " 9mb OFDM", "12mb OFDM", "18mb OFDM",
                                   "24mb OFDM", "36mb OFDM", "48mb OFDM", "54mb OFDM",
                                   "1L   CCK ", "2L   CCK ", "2S   CCK ", "5.5L CCK ",
                                   "5.5S CCK ", "11L  CCK ", "11S  CCK ", "XR       ",
                                   "HT20mcs 0", "HT20mcs 1", "HT20mcs 2", "HT20mcs 3",
                                   "HT20mcs 4", "HT20mcs 5", "HT20mcs 6", "HT20mcs 7",
                                   "HT40mcs 0", "HT40mcs 1", "HT40mcs 2", "HT40mcs 3",
                                   "HT40mcs 4", "HT40mcs 5", "HT40mcs 6", "HT40mcs 7",
                                   "Dup CCK  ", "Dup OFDM ", "Ext CCK  ", "Ext OFDM ",
    };
    int i;

    for (i = 0; i < Ar5416RateSize; i+=4) {
        ath_hal_printf(ah, " %s %3d.%1d dBm | %s %3d.%1d dBm | %s %3d.%1d dBm | %s %3d.%1d dBm\n",
            rateString[i], pRatesPower[i] / 2, (pRatesPower[i] % 2) * 5,
            rateString[i + 1], pRatesPower[i + 1] / 2, (pRatesPower[i + 1] % 2) * 5,
            rateString[i + 2], pRatesPower[i + 2] / 2, (pRatesPower[i + 2] % 2) * 5,
            rateString[i + 3], pRatesPower[i + 3] / 2, (pRatesPower[i + 3] % 2) * 5);
    }
    ath_hal_printf(ah, "2xAntennaReduction: %d, 2xMaxRegulatory: %d, 2xPowerLimit: %d\n",
                        red, reg, limit);
}
#endif /* EEPROM_DUMP */

int ar5416EepromDumpSupport(struct ath_hal *ah, void **ppE)
{
    *ppE = &(AH5416(ah)->ah_eeprom);
    return sizeof(ar5416_eeprom_t);
}

#undef N
#undef POW_SM
#endif /* AH_SUPPORT_AR5416 */
