/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar2133.c#13 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_2133

#include "ah.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"

/* Add static register initialization vectors */
#define AH_5416_2133
#include "ar5416/ar5416.ini"

#define N(a)    (sizeof(a)/sizeof(a[0]))

//#define DEBUG_PRINT_BANK_6 1

/*
 * WAR for bug 6773.  OS_DELAY() does a PIO READ on the PCI bus which allows
 * other cards' DMA reads to complete in the middle of our reset.
 */
#define WAR_6773(x) do {        \
    if ((++(x) % 64) == 0)      \
        OS_DELAY(1);        \
} while (0)

#define REG_WRITE_ARRAY(regArray, column, regWr) do {                   \
    int r;                              \
    for (r = 0; r < N(regArray); r++) {             \
        OS_REG_WRITE(ah, (regArray)[r][0], (regArray)[r][(column)]);\
        WAR_6773(regWr);                    \
    }                               \
} while (0)

#define REG_WRITE_RF_ARRAY(regArray, regData, regWr) do {               \
    int r;                              \
    for (r = 0; r < N(regArray); r++) {             \
        OS_REG_WRITE(ah, (regArray)[r][0], (regData)[r]);   \
        WAR_6773(regWr);                    \
    }                               \
} while (0)

#if 0
static  void ar5416GetLowerUpperIndex(u_int16_t v,
        u_int16_t *lp, u_int16_t listSize,
        u_int32_t *vlo, u_int32_t *vhi);
static HAL_BOOL getFullPwrTable(u_int16_t numPcdacs, u_int16_t *pcdacs,
        int16_t *power, int16_t maxPower, int16_t *retVals);
static int16_t getPminAndPcdacTableFromPowerTable(int16_t *pwrTableT4,
        u_int16_t retVals[]);
static int16_t getPminAndPcdacTableFromTwoPowerTables(int16_t *pwrTableLXpdT4,
        int16_t *pwrTableHXpdT4, u_int16_t retVals[], int16_t *pMid);
static int16_t interpolate_signed(u_int16_t target,
        u_int16_t srcLeft, u_int16_t srcRight,
        int16_t targetLeft, int16_t targetRight);
#endif

#ifdef ATH_FORCE_BIAS
static void ar5416ForceBiasCurrent(struct ath_hal *ah, u_int16_t synth_freq);
#endif

extern  void ar5416ModifyRfBuffer(u_int32_t *rfBuf, u_int32_t reg32,
        u_int32_t numBits, u_int32_t firstBit, u_int32_t column);

typedef struct {
    u_int32_t Bank0Data[N(ar5416Bank0)];
    u_int32_t Bank1Data[N(ar5416Bank1)];
    u_int32_t Bank2Data[N(ar5416Bank2)];
    u_int32_t Bank3Data[N(ar5416Bank3)];
    u_int32_t Bank6Data[N(ar5416Bank6)];
    u_int32_t Bank6TPCData[N(ar5416Bank6TPC)];
    u_int32_t Bank7Data[N(ar5416Bank7)];
} AR5416_RF_BANKS_2133;

static void
ar2133WriteRegs(struct ath_hal *ah, u_int modesIndex, u_int freqIndex, int regWrites)
{
#if 0
    REG_WRITE_ARRAY(ar5416Modes_2133, modesIndex, regWrites);
    REG_WRITE_ARRAY(ar5416Common_2133, 1, regWrites);
#endif
    REG_WRITE_ARRAY(ar5416BB_RfGain, freqIndex, regWrites);
}

/*
 * Take the MHz channel value and set the Channel value
 *
 * New for 2133: Workaround FOWL bug by forcing rf_pwd_icsyndiv
 * value as a function of synth frequency.
 *
 * ASSUMES: Writes enabled to analog bus and bank6 register cache in
 * ahp->ah_analogBanks is valid.
 */
static HAL_BOOL
ar2133SetChannel(struct ath_hal *ah,  HAL_CHANNEL_INTERNAL *chan, HAL_HT *ht)
{
    u_int32_t channelSel  = 0;
    u_int32_t bModeSynth  = 0;
    u_int32_t aModeRefSel = 0;
    u_int32_t reg32       = 0;
    u_int16_t freq;
    CHAN_CENTERS centers;

    OS_MARK(ah, AH_MARK_SETCHANNEL, chan->channel);

    ar5416GetChannelCenters(ah, ht, chan, &centers);
    freq = centers.synth_center;

    if (freq < 4800) {
        u_int32_t txctl;

        if (((freq - 2192) % 5) == 0) {
            channelSel = ((freq - 672) * 2 - 3040)/10;
            bModeSynth = 0;
        } else if (((freq - 2224) % 5) == 0) {
            channelSel = ((freq - 704) * 2 - 3040) / 10;
            bModeSynth = 1;
        } else {
            HALDEBUG(ah, "%s: invalid channel %u MHz\n",
                __func__, freq);
            return AH_FALSE;
        }

        channelSel = (channelSel << 2) & 0xff;
        channelSel = ath_hal_reverseBits(channelSel, 8);

        txctl = OS_REG_READ(ah, AR_PHY_CCK_TX_CTRL);
        if (freq == 2484) {
            /* Enable channel spreading for channel 14 */
            OS_REG_WRITE(ah, AR_PHY_CCK_TX_CTRL,
                txctl | AR_PHY_CCK_TX_CTRL_JAPAN);
        } else {
            OS_REG_WRITE(ah, AR_PHY_CCK_TX_CTRL,
                txctl &~ AR_PHY_CCK_TX_CTRL_JAPAN);
        }

#ifdef ATH_FORCE_BIAS
        /* FOWL orientation sensitivity workaround */
        ar5416ForceBiasCurrent(ah, freq);
#endif /* ATH_FORCE_BIAS */

    } else if ((freq % 20) == 0 && freq >= 5120) {
        channelSel = ath_hal_reverseBits(
            ((freq - 4800) / 20 << 2), 8);
        aModeRefSel = ath_hal_reverseBits(1, 2);
    } else if ((freq % 10) == 0) {
        channelSel = ath_hal_reverseBits(
            ((freq - 4800) / 10 << 1), 8);
        aModeRefSel = ath_hal_reverseBits(1, 2);
    } else if ((freq % 5) == 0) {
        channelSel = ath_hal_reverseBits(
            (freq - 4800) / 5, 8);
        aModeRefSel = ath_hal_reverseBits(1, 2);
    } else {
        HALDEBUG(ah, "%s: invalid channel %u MHz\n",
            __func__, freq);
        return AH_FALSE;
    }

    reg32 = (channelSel << 8) | (aModeRefSel << 2) | (bModeSynth << 1) |
            (1 << 5) | 0x1;

    OS_REG_WRITE(ah, AR_PHY(0x37), reg32);

    AH_PRIVATE(ah)->ah_curchan = chan;
#if 0
    if (chan->privFlags & CHANNEL_DFS) {
        struct ar5416RadarState *rs;
        u_int8_t index;

        rs = ar5416GetRadarChanState(ah, &index);
        if (rs != AH_NULL) {
            AH5416(ah)->ah_curchanRadIndex = (int16_t) index;
        } else {
            HALDEBUG(ah, "%s: Couldn't find radar state information\n",
                 __func__);
            return AH_FALSE;
        }
    } else
        AH5416(ah)->ah_curchanRadIndex = -1;
#endif
    return AH_TRUE;
}

#if 0
/*
 * Return a reference to the requested RF Bank.
 */
static u_int32_t *
ar2133GetRfBank(struct ath_hal *ah, int bank)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    AR5416_RF_BANKS_2133 *pRfBank2133 = ahp->ah_analogBanks;

    HALASSERT(ahp->ah_analogBanks != AH_NULL);
    switch (bank) {
    case 1: return pRfBank2133->Bank1Data;
    case 2: return pRfBank2133->Bank2Data;
    case 3: return pRfBank2133->Bank3Data;
    case 6: return pRfBank2133->Bank6Data;
    case 7: return pRfBank2133->Bank7Data;
    }
    HALDEBUG(ah, "%s: unknown RF Bank %d requested\n", __func__, bank);
    return AH_NULL;
}
#endif

/*
 * Reads EEPROM header info from device structure and programs
 * all rf registers
 *
 * REQUIRES: Access to the analog rf device
 */
static HAL_BOOL
ar2133SetRfRegs(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan,
                u_int16_t modesIndex)
{
#define RF_BANK_SETUP(_pb, _ix, _col) do {                  \
    int i;                                  \
    for (i = 0; i < N(ar5416Bank##_ix); i++)             \
        (_pb)->Bank##_ix##Data[i] = ar5416Bank##_ix[i][_col];\
} while (0)
    struct ath_hal_5416 *ahp = AH5416(ah);
#if 0
    u_int16_t rfXpdSel, gainI;
    GAIN_VALUES *gv = &ahp->ah_gainValues;
#endif

    u_int32_t eepMinorRev;
    u_int32_t ob5GHz = 0, db5GHz = 0;
    u_int32_t ob2GHz = 0, db2GHz = 0;

    AR5416_RF_BANKS_2133 *pRfBanks = ahp->ah_analogBanks;
    int regWrites = 0;

    HALASSERT(pRfBanks);

#if 0
    switch (chan->channelFlags & CHANNEL_ALL) {
    case CHANNEL_A_HT20:
    case CHANNEL_A_HT40:
    case CHANNEL_A:
    case CHANNEL_T:
    case CHANNEL_XR:
        if (chan->channel > 4000 && chan->channel < 5260) {
            ob5GHz = ahp->ah_ob1;
            db5GHz = ahp->ah_db1;
        } else if (chan->channel >= 5260 && chan->channel < 5500) {
            ob5GHz = ahp->ah_ob2;
            db5GHz = ahp->ah_db2;
        } else if (chan->channel >= 5500 && chan->channel < 5725) {
            ob5GHz = ahp->ah_ob3;
            db5GHz = ahp->ah_db3;
        } else if (chan->channel >= 5725) {
            ob5GHz = ahp->ah_ob4;
            db5GHz = ahp->ah_db4;
        } else {
            /* XXX else */
        }
        rfXpdSel = ahp->ah_xpd[headerInfo11A];
        gainI = ahp->ah_gainI[headerInfo11A];
        break;
    case CHANNEL_B:
        ob2GHz = ahp->ah_ob2GHz[0];
        db2GHz = ahp->ah_db2GHz[0];
        rfXpdSel = ahp->ah_xpd[headerInfo11B];
        gainI = ahp->ah_gainI[headerInfo11B];
        break;
    case CHANNEL_G_HT20:
    case CHANNEL_G_HT40:
    case CHANNEL_G:
    case CHANNEL_108G:
#ifdef notyet
    case CHANNEL_XR_G:
#endif
        ob2GHz = ahp->ah_ob2GHz[1];
        db2GHz = ahp->ah_ob2GHz[1];
        rfXpdSel = ahp->ah_xpd[headerInfo11G];
        gainI = ahp->ah_gainI[headerInfo11G];
        break;
    default:
        HALDEBUG(ah, "%s: invalid channel flags 0x%x\n",
            __func__, chan->channelFlags);
        return AH_FALSE;
    }
#endif

    /* Setup rf parameters */
    eepMinorRev = ar5416EepromGet(ahp, EEP_MINOR_REV);

    /* Setup Bank 0 Write */
    RF_BANK_SETUP(pRfBanks, 0, 1);

    /* Setup Bank 1 Write */
    RF_BANK_SETUP(pRfBanks, 1, 1);

    /* Setup Bank 2 Write */
    RF_BANK_SETUP(pRfBanks, 2, 1);

    /* Setup Bank 3 Write */
    RF_BANK_SETUP(pRfBanks, 3, modesIndex);

    /* Setup Bank 6 Write */
    if (!ath_hal_enableTPC || ahp->ah_emu_eeprom) {
        int i;
        for (i = 0; i < N(ar5416Bank6); i++) {
            pRfBanks->Bank6Data[i] = ar5416Bank6[i][modesIndex];
        }
    } else {
        int i;
        for (i = 0; i < N(ar5416Bank6TPC); i++) {
            pRfBanks->Bank6Data[i] = ar5416Bank6TPC[i][modesIndex];
        }
    }

    /* Only the 5 or 2 GHz OB/DB need to be set for a mode */
    if (eepMinorRev >= 2) {
        if (IS_CHAN_2GHZ(chan)) {
            ob2GHz = ar5416EepromGet(ahp, EEP_OB_2);
            db2GHz = ar5416EepromGet(ahp, EEP_DB_2);
            ar5416ModifyRfBuffer(pRfBanks->Bank6Data, ob2GHz, 3, 197, 0);
            ar5416ModifyRfBuffer(pRfBanks->Bank6Data, db2GHz, 3, 194, 0);
        } else {
            ob5GHz = ar5416EepromGet(ahp, EEP_OB_5);
            db5GHz = ar5416EepromGet(ahp, EEP_DB_5);
            ar5416ModifyRfBuffer(pRfBanks->Bank6Data, ob5GHz, 3, 203, 0);
            ar5416ModifyRfBuffer(pRfBanks->Bank6Data, db5GHz, 3, 200, 0);
        }
    }

    /* Setup Bank 7 Setup */
    RF_BANK_SETUP(pRfBanks, 7, 1);

    /* Write Analog registers */
    REG_WRITE_RF_ARRAY(ar5416Bank0, pRfBanks->Bank0Data, regWrites);
    REG_WRITE_RF_ARRAY(ar5416Bank1, pRfBanks->Bank1Data, regWrites);
    REG_WRITE_RF_ARRAY(ar5416Bank2, pRfBanks->Bank2Data, regWrites);
    REG_WRITE_RF_ARRAY(ar5416Bank3, pRfBanks->Bank3Data, regWrites);

    #ifdef DEBUG_PRINT_BANK_6
    {
        int ii;
        ath_hal_printf(ah, "DUMP BANK 6 Set Regs\n");
        for (ii = 0; ii < 33; ii++) {
            ath_hal_printf(ah, " %8.8x\n", pRfBanks->Bank6Data[ii]);
        }
    }
    #endif

    if (!ath_hal_enableTPC || ahp->ah_emu_eeprom) {
        REG_WRITE_RF_ARRAY(ar5416Bank6, pRfBanks->Bank6Data, regWrites);
        ath_hal_printf(ah,"TPC Disabled %d %d %d\n",
            ath_hal_enableTPC, ath_hal_soft_eeprom, ahp->ah_emu_eeprom);
    } else {
        REG_WRITE_RF_ARRAY(ar5416Bank6TPC, pRfBanks->Bank6Data, regWrites);
        ath_hal_printf(ah,"TPC Enabled %d %d %d\n",
            ath_hal_enableTPC, ath_hal_soft_eeprom, ahp->ah_emu_eeprom);
    }

    REG_WRITE_RF_ARRAY(ar5416Bank7, pRfBanks->Bank7Data, regWrites);

    return AH_TRUE;
#undef  RF_BANK_SETUP
}

/*
 * Read the transmit power levels from the structures taken from EEPROM
 * Interpolate read transmit power values for this channel
 * Organize the transmit power values into a table for writing into the hardware
 */
#if 0
static HAL_BOOL
ar2133SetPowerTable(struct ath_hal *ah,
    int16_t *pPowerMin, int16_t *pPowerMax, HAL_CHANNEL_INTERNAL *chan,
    u_int16_t *rfXpdGain)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    u_int32_t numXpdGain = IS_RADX112_REV2(ah) ? 2 : 1;
    u_int32_t    xpdGainMask = 0;
    int16_t     powerMid, *pPowerMid = &powerMid;

    EXPN_DATA_PER_CHANNEL_2133 *pRawCh;
    EEPROM_POWER_EXPN_2133     *pPowerExpn = AH_NULL;

    u_int32_t    ii, jj, kk;
    int16_t     minPwr_t4, maxPwr_t4, Pmin, Pmid;

    u_int32_t    chan_idx_L, chan_idx_R;
    u_int16_t    chan_L, chan_R;

    int16_t     pwr_table0[64];
    int16_t     pwr_table1[64];
    u_int16_t    pcdacs[10];
    int16_t     powers[10];
    u_int16_t    numPcd;
    int16_t     powTableLXPD[2][64];
    int16_t     powTableHXPD[2][64];
    int16_t     tmpPowerTable[64];
    u_int16_t    xgainList[2];
    u_int16_t    xpdMask;

    switch (chan->channelFlags & CHANNEL_ALL) {
    case CHANNEL_A_HT20:
    case CHANNEL_A_HT40:
    case CHANNEL_A:
    case CHANNEL_T:
    case CHANNEL_XR:
        pPowerExpn = &ahp->ah_modePowerArray2133[headerInfo11A];
        xpdGainMask = ahp->ah_xgain[headerInfo11A];
        break;
    case CHANNEL_B:
        pPowerExpn = &ahp->ah_modePowerArray2133[headerInfo11B];
        xpdGainMask = ahp->ah_xgain[headerInfo11B];
        break;
    case CHANNEL_G_HT20:
    case CHANNEL_G_HT40:
    case CHANNEL_G:
    case CHANNEL_108G:
#ifdef notyet
    case CHANNEL_XR_G:
#endif
        pPowerExpn = &ahp->ah_modePowerArray2133[headerInfo11G];
        xpdGainMask = ahp->ah_xgain[headerInfo11G];
        break;
    default:
        HALDEBUG(ah, "%s: unknown channel flags 0x%x\n",
            __func__, chan->channelFlags & CHANNEL_ALL);
        return AH_FALSE;
    }

    if ((xpdGainMask & pPowerExpn->xpdMask) < 1) {
        HALDEBUG(ah, "%s: desired xpdGainMask 0x%x not supported by "
            "calibrated xpdMask 0x%x\n", __func__,
            xpdGainMask, pPowerExpn->xpdMask);
        return AH_FALSE;
    }

    maxPwr_t4 = (int16_t)(2*(*pPowerMax));  /* pwr_t2 -> pwr_t4 */
    minPwr_t4 = (int16_t)(2*(*pPowerMin));  /* pwr_t2 -> pwr_t4 */

    xgainList[0] = 0xDEAD;
    xgainList[1] = 0xDEAD;

    kk = 0;
    xpdMask = pPowerExpn->xpdMask;
    for (jj = 0; jj < NUM_XPD_PER_CHANNEL; jj++) {
        if (((xpdMask >> jj) & 1) > 0) {
            if (kk > 1) {
                HALDEBUG(ah, "A maximum of 2 xpdGains supported"
                    "in pExpnPower data\n");
                return AH_FALSE;
            }
            xgainList[kk++] = (u_int16_t)jj;
        }
    }

    ar5416GetLowerUpperIndex(chan->channel, &pPowerExpn->pChannels[0],
        pPowerExpn->numChannels, &chan_idx_L, &chan_idx_R);

    kk = 0;
    for (ii = chan_idx_L; ii <= chan_idx_R; ii++) {
        pRawCh = &(pPowerExpn->pDataPerChannel[ii]);
        if (xgainList[1] == 0xDEAD) {
            jj = xgainList[0];
            numPcd = pRawCh->pDataPerXPD[jj].numPcdacs;
            OS_MEMCPY(&pcdacs[0], &pRawCh->pDataPerXPD[jj].pcdac[0],
                numPcd * sizeof(u_int16_t));
            OS_MEMCPY(&powers[0], &pRawCh->pDataPerXPD[jj].pwr_t4[0],
                numPcd * sizeof(int16_t));
            if (!getFullPwrTable(numPcd, &pcdacs[0], &powers[0],
                pRawCh->maxPower_t4, &tmpPowerTable[0])) {
                return AH_FALSE;
            }
            OS_MEMCPY(&powTableLXPD[kk][0], &tmpPowerTable[0],
                64*sizeof(int16_t));
        } else {
            jj = xgainList[0];
            numPcd = pRawCh->pDataPerXPD[jj].numPcdacs;
            OS_MEMCPY(&pcdacs[0], &pRawCh->pDataPerXPD[jj].pcdac[0],
                numPcd*sizeof(u_int16_t));
            OS_MEMCPY(&powers[0],
                &pRawCh->pDataPerXPD[jj].pwr_t4[0],
                numPcd*sizeof(int16_t));
            if (!getFullPwrTable(numPcd, &pcdacs[0], &powers[0],
                pRawCh->maxPower_t4, &tmpPowerTable[0])) {
                return AH_FALSE;
            }
            OS_MEMCPY(&powTableLXPD[kk][0], &tmpPowerTable[0],
                64 * sizeof(int16_t));

            jj = xgainList[1];
            numPcd = pRawCh->pDataPerXPD[jj].numPcdacs;
            OS_MEMCPY(&pcdacs[0], &pRawCh->pDataPerXPD[jj].pcdac[0],
                numPcd * sizeof(u_int16_t));
            OS_MEMCPY(&powers[0],
                &pRawCh->pDataPerXPD[jj].pwr_t4[0],
                numPcd * sizeof(int16_t));
            if (!getFullPwrTable(numPcd, &pcdacs[0], &powers[0],
                pRawCh->maxPower_t4, &tmpPowerTable[0])) {
                return AH_FALSE;
            }
            OS_MEMCPY(&powTableHXPD[kk][0], &tmpPowerTable[0],
                64 * sizeof(int16_t));
        }
        kk++;
    }

    chan_L = pPowerExpn->pChannels[chan_idx_L];
    chan_R = pPowerExpn->pChannels[chan_idx_R];
    kk = chan_idx_R - chan_idx_L;

    if (xgainList[1] == 0xDEAD) {
        for (jj = 0; jj < 64; jj++) {
            pwr_table0[jj] = interpolate_signed(
                chan->channel, chan_L, chan_R,
                powTableLXPD[0][jj], powTableLXPD[kk][jj]);
        }
        Pmin = getPminAndPcdacTableFromPowerTable(&pwr_table0[0],
                ahp->ah_pcdacTable);
        *pPowerMin = (int16_t) (Pmin / 2);
        *pPowerMid = (int16_t) (pwr_table0[63] / 2);
        *pPowerMax = (int16_t) (pwr_table0[63] / 2);
        rfXpdGain[0] = xgainList[0];
        rfXpdGain[1] = rfXpdGain[0];
    } else {
        for (jj = 0; jj < 64; jj++) {
            pwr_table0[jj] = interpolate_signed(
                chan->channel, chan_L, chan_R,
                powTableLXPD[0][jj], powTableLXPD[kk][jj]);
            pwr_table1[jj] = interpolate_signed(
                chan->channel, chan_L, chan_R,
                powTableHXPD[0][jj], powTableHXPD[kk][jj]);
        }
        if (numXpdGain == 2) {
            Pmin = getPminAndPcdacTableFromTwoPowerTables(
                &pwr_table0[0], &pwr_table1[0],
                ahp->ah_pcdacTable, &Pmid);
            *pPowerMin = (int16_t) (Pmin / 2);
            *pPowerMid = (int16_t) (Pmid / 2);
            *pPowerMax = (int16_t) (pwr_table0[63] / 2);
            rfXpdGain[0] = xgainList[0];
            rfXpdGain[1] = xgainList[1];
        } else if (minPwr_t4 <= pwr_table1[63] &&
               maxPwr_t4 <= pwr_table1[63]) {
            Pmin = getPminAndPcdacTableFromPowerTable(
                &pwr_table1[0], ahp->ah_pcdacTable);
            rfXpdGain[0] = xgainList[1];
            rfXpdGain[1] = rfXpdGain[0];
            *pPowerMin = (int16_t) (Pmin / 2);
            *pPowerMid = (int16_t) (pwr_table1[63] / 2);
            *pPowerMax = (int16_t) (pwr_table1[63] / 2);
        } else {
            Pmin = getPminAndPcdacTableFromPowerTable(
                &pwr_table0[0], ahp->ah_pcdacTable);
            rfXpdGain[0] = xgainList[0];
            rfXpdGain[1] = rfXpdGain[0];
            *pPowerMin = (int16_t) (Pmin/2);
            *pPowerMid = (int16_t) (pwr_table0[63] / 2);
            *pPowerMax = (int16_t) (pwr_table0[63] / 2);
        }
    }

    /*
     * Move 2133 rates to match power tables where the max
     * power table entry corresponds with maxPower.
     */
    HALASSERT(*pPowerMax <= PCDAC_STOP);
    ahp->ah_txPowerIndexOffset = PCDAC_STOP - *pPowerMax;

    return AH_TRUE;
}

/*
 * Returns interpolated or the scaled up interpolated value
 */
static int16_t
interpolate_signed(u_int16_t target, u_int16_t srcLeft, u_int16_t srcRight,
    int16_t targetLeft, int16_t targetRight)
{
    int16_t rv;

    if (srcRight != srcLeft) {
        rv = ((target - srcLeft)*targetRight +
              (srcRight - target)*targetLeft) / (srcRight - srcLeft);
    } else {
        rv = targetLeft;
    }
    return rv;
}
#endif

/*
 * Return indices surrounding the value in sorted integer lists.
 *
 * NB: the input list is assumed to be sorted in ascending order
 */
#if 0
static void
ar5416GetLowerUpperIndex(u_int16_t v, u_int16_t *lp, u_int16_t listSize,
                          u_int32_t *vlo, u_int32_t *vhi)
{
    u_int32_t target = v;
    u_int16_t *ep = lp+listSize;
    u_int16_t *tp;

    /*
     * Check first and last elements for out-of-bounds conditions.
     */
    if (target < lp[0]) {
        *vlo = *vhi = 0;
        return;
    }
    if (target >= ep[-1]) {
        *vlo = *vhi = listSize - 1;
        return;
    }

    /* look for value being near or between 2 values in list */
    for (tp = lp; tp < ep; tp++) {
        /*
         * If value is close to the current value of the list
         * then target is not between values, it is one of the values
         */
        if (*tp == target) {
            *vlo = *vhi = tp - lp;
            return;
        }
        /*
         * Look for value being between current value and next value
         * if so return these 2 values
         */
        if (target < tp[1]) {
            *vlo = tp - lp;
            *vhi = *vlo + 1;
            return;
        }
    }
}

static HAL_BOOL
getFullPwrTable(u_int16_t numPcdacs, u_int16_t *pcdacs, int16_t *power, int16_t maxPower, int16_t *retVals)
{
    u_int16_t    ii;
    u_int16_t    idxL = 0;
    u_int16_t    idxR = 1;

    if (numPcdacs < 2) {
        HALDEBUG(AH_NULL, "%s: at least 2 pcdac values needed [%d]\n",
            __func__, numPcdacs);
        return AH_FALSE;
    }
    for (ii = 0; ii < 64; ii++) {
        if (ii>pcdacs[idxR] && idxR < numPcdacs-1) {
            idxL++;
            idxR++;
        }
        retVals[ii] = interpolate_signed(ii,
            pcdacs[idxL], pcdacs[idxR], power[idxL], power[idxR]);
        if (retVals[ii] >= maxPower) {
            while (ii < 64)
                retVals[ii++] = maxPower;
        }
    }
    return AH_TRUE;
}

/*
 * Takes a single calibration curve and creates a power table.
 * Adjusts the new power table so the max power is relative
 * to the maximum index in the power table.
 *
 * WARNING: rates must be adjusted for this relative power table
 */
static int16_t
getPminAndPcdacTableFromPowerTable(int16_t *pwrTableT4, u_int16_t retVals[])
{
    int16_t ii, jj, jjMax;
    int16_t pMin, currPower, pMax;

    /* If the spread is > 31.5dB, keep the upper 31.5dB range */
    if ((pwrTableT4[63] - pwrTableT4[0]) > 126) {
        pMin = pwrTableT4[63] - 126;
    } else {
        pMin = pwrTableT4[0];
    }

    pMax = pwrTableT4[63];
    jjMax = 63;

    /* Search for highest pcdac 0.25dB below maxPower */
    while ((pwrTableT4[jjMax] > (pMax - 1) ) && (jjMax >= 0)) {
        jjMax--;
    }

    jj = jjMax;
    currPower = pMax;
    for (ii = 63; ii >= 0; ii--) {
        while ((jj < 64) && (jj > 0) && (pwrTableT4[jj] >= currPower)) {
            jj--;
        }
        if (jj == 0) {
            while (ii >= 0) {
                retVals[ii] = retVals[ii + 1];
                ii--;
            }
            break;
        }
        retVals[ii] = jj;
        currPower -= 2;  // corresponds to a 0.5dB step
    }
    return pMin;
}

/*
 * Combines the XPD curves from two calibration sets into a single
 * power table and adjusts the power table so the max power is relative
 * to the maximum index in the power table
 *
 * WARNING: rates must be adjusted for this relative power table
 */
static int16_t
getPminAndPcdacTableFromTwoPowerTables(int16_t *pwrTableLXpdT4,
    int16_t *pwrTableHXpdT4, u_int16_t retVals[], int16_t *pMid)
{
    int16_t     ii, jj, jjMax;
    int16_t     pMin, pMax, currPower;
    int16_t     *pwrTableT4;
    u_int16_t    msbFlag = 0x40;  // turns on the 7th bit of the pcdac

    /* If the spread is > 31.5dB, keep the upper 31.5dB range */
    if ((pwrTableLXpdT4[63] - pwrTableHXpdT4[0]) > 126) {
        pMin = pwrTableLXpdT4[63] - 126;
    } else {
        pMin = pwrTableHXpdT4[0];
    }

    pMax = pwrTableLXpdT4[63];
    jjMax = 63;
    /* Search for highest pcdac 0.25dB below maxPower */
    while ((pwrTableLXpdT4[jjMax] > (pMax - 1) ) && (jjMax >= 0)){
        jjMax--;
    }

    *pMid = pwrTableHXpdT4[63];
    jj = jjMax;
    ii = 63;
    currPower = pMax;
    pwrTableT4 = &(pwrTableLXpdT4[0]);
    while (ii >= 0) {
        if ((currPower <= *pMid) || ( (jj == 0) && (msbFlag == 0x40))){
            msbFlag = 0x00;
            pwrTableT4 = &(pwrTableHXpdT4[0]);
            jj = 63;
        }
        while ((jj > 0) && (pwrTableT4[jj] >= currPower)) {
            jj--;
        }
        if ((jj == 0) && (msbFlag == 0x00)) {
            while (ii >= 0) {
                retVals[ii] = retVals[ii+1];
                ii--;
            }
            break;
        }
        retVals[ii] = jj | msbFlag;
        currPower -= 2;  // corresponds to a 0.5dB step
        ii--;
    }
    return pMin;
}
#endif

/*
 * Free memory for analog bank scratch buffers
 */
static void
ar2133Detach(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

#if 0
    if (ahp->ah_pcdacTable != AH_NULL) {
        ath_hal_free(ahp->ah_pcdacTable);
        ahp->ah_pcdacTable = AH_NULL;
    }
#endif
    if (ahp->ah_analogBanks != AH_NULL) {
        ath_hal_free(ahp->ah_analogBanks);
        ahp->ah_analogBanks = AH_NULL;
    }
}

#if 0
static int16_t
ar2133GetMinPower(struct ath_hal *ah, EXPN_DATA_PER_CHANNEL_5112 *data)
{
    int i, minIndex;
    int16_t minGain,minPwr,minPcdac,retVal;

    /* Assume NUM_POINTS_XPD0 > 0 */
    minGain = data->pDataPerXPD[0].xpd_gain;
    for (minIndex=0,i=1; i<NUM_XPD_PER_CHANNEL; i++) {
        if (data->pDataPerXPD[i].xpd_gain < minGain) {
            minIndex = i;
            minGain = data->pDataPerXPD[i].xpd_gain;
        }
    }
    minPwr = data->pDataPerXPD[minIndex].pwr_t4[0];
    minPcdac = data->pDataPerXPD[minIndex].pcdac[0];
    for (i=1; i<NUM_POINTS_XPD0; i++) {
        if (data->pDataPerXPD[minIndex].pwr_t4[i] < minPwr) {
            minPwr = data->pDataPerXPD[minIndex].pwr_t4[i];
            minPcdac = data->pDataPerXPD[minIndex].pcdac[i];
        }
    }
    retVal = minPwr - (minPcdac*2);
    return(retVal);
}

static HAL_BOOL
ar2133GetChannelMaxMinPower(struct ath_hal *ah, HAL_CHANNEL *chan, int16_t *maxPow,
                int16_t *minPow)
{
    struct ath_hal_5416 *ahp = (struct ath_hal_5416 *) ah;
    int numChannels=0,i,last;
    int totalD, totalF,totalMin;
    EXPN_DATA_PER_CHANNEL_5112 *data=AH_NULL;
    EEPROM_POWER_EXPN_5112 *powerArray=AH_NULL;

    *maxPow = 0;
    if (IS_CHAN_A(chan)) {
        powerArray = ahp->ah_modePowerArray5112;
        data = powerArray[headerInfo11A].pDataPerChannel;
        numChannels = powerArray[headerInfo11A].numChannels;
    } else if (IS_CHAN_G(chan) || IS_CHAN_108G(chan)) {
        /* XXX - is this correct? Should we also use the same power for turbo G? */
        powerArray = ahp->ah_modePowerArray5112;
        data = powerArray[headerInfo11G].pDataPerChannel;
        numChannels = powerArray[headerInfo11G].numChannels;
    } else if (IS_CHAN_B(chan)) {
        powerArray = ahp->ah_modePowerArray5112;
        data = powerArray[headerInfo11B].pDataPerChannel;
        numChannels = powerArray[headerInfo11B].numChannels;
    } else {
        return (AH_TRUE);
    }
    /* Make sure the channel is in the range of the TP values
     *  (freq piers)
     */
    if ((numChannels < 1) ||
        (chan->channel < data[0].channelValue) ||
        (chan->channel > data[numChannels-1].channelValue))
        return(AH_FALSE);

    /* Linearly interpolate the power value now */
    for (last=0,i=0;
         (i<numChannels) && (chan->channel > data[i].channelValue);
         last=i++);
    totalD = data[i].channelValue - data[last].channelValue;
    if (totalD > 0) {
        totalF = data[i].maxPower_t4 - data[last].maxPower_t4;
        *maxPow = (int8_t) ((totalF*(chan->channel-data[last].channelValue) + data[last].maxPower_t4*totalD)/totalD);

        totalMin = ar2133GetMinPower(ah,&data[i]) - ar2133GetMinPower(ah, &data[last]);
        *minPow = (int8_t) ((totalMin*(chan->channel-data[last].channelValue) + ar2133GetMinPower(ah, &data[last])*totalD)/totalD);
        return (AH_TRUE);
    } else {
        if (chan->channel == data[i].channelValue) {
            *maxPow = data[i].maxPower_t4;
            *minPow = ar2133GetMinPower(ah, &data[i]);
            return(AH_TRUE);
        } else
            return(AH_FALSE);
    }
}
#endif

HAL_BOOL
ar2133GetChipPowerLimits(struct ath_hal *ah, HAL_CHANNEL *chans, u_int32_t nchans)
{
    HAL_BOOL retVal = AH_TRUE;
    int i;
    //int16_t maxPow,minPow;

    for (i=0; i < nchans; i ++) {
        chans[i].maxTxPower = AR5416_MAX_RATE_POWER;
        chans[i].minTxPower = AR5416_MAX_RATE_POWER;
    }
    return (retVal);

#if 0
    for (i=0; i<nchans; i++) {
        if (ar2133GetChannelMaxMinPower(ah, &chans[i], &maxPow, &minPow)) {
            /* XXX -Need to adjust pcdac values to indicate dBm */
            chans[i].maxTxPower = maxPow;
            chans[i].minTxPower = minPow;
        } else {
            HALDEBUG(ah, "Failed setting power table for nchans=%d\n",i);
            retVal= AH_FALSE;
        }
    }
#ifdef AH_DEBUG
    for (i=0; i<nchans; i++) {
        ath_hal_printf(ah,"Chan %d: MaxPow = %d MinPow = %d\n",
             chans[i].channel,chans[i].maxTxPower, chans[i].minTxPower);
    }
#endif
#endif
}

/*
 * Allocate memory for analog bank scratch buffers
 * Scratch Buffer will be reinitialized every reset so no need to zero now
 */
HAL_BOOL
ar2133RfAttach(struct ath_hal *ah, HAL_STATUS *status)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    HALASSERT(ahp->ah_analogBanks == AH_NULL);
    ahp->ah_analogBanks = ath_hal_malloc(sizeof(AR5416_RF_BANKS_2133));
    if (ahp->ah_analogBanks == AH_NULL) {
        ath_hal_printf(ah, "%s: cannot allocate RF banks\n", __func__);
        *status = HAL_ENOMEM;       /* XXX */
        return AH_FALSE;
    }
#if 0
    HALASSERT(ahp->ah_pcdacTable == AH_NULL);

    ahp->ah_pcdacTableSize = PWR_TABLE_SIZE * sizeof(u_int16_t);
    ahp->ah_pcdacTable = ath_hal_malloc(ahp->ah_pcdacTableSize);
    if (ahp->ah_pcdacTable == AH_NULL) {
        ath_hal_printf(ah, "%s: cannot allocate PCDAC table\n", __func__);
        *status = HAL_ENOMEM;       /* XXX */
        return AH_FALSE;
    }

    ahp->ah_pcdacTableSize = PWR_TABLE_SIZE;
#endif
    ahp->ah_rfHal.rfDetach      = ar2133Detach;
    ahp->ah_rfHal.writeRegs     = ar2133WriteRegs;
#if 0
    ahp->ah_rfHal.getRfBank     = ar2133GetRfBank;
#endif
    ahp->ah_rfHal.setChannel    = ar2133SetChannel;
    ahp->ah_rfHal.setRfRegs     = ar2133SetRfRegs;
#if 0
    ahp->ah_rfHal.setPowerTable = ar2133SetPowerTable;
#endif
    ahp->ah_rfHal.getChipPowerLim   = ar2133GetChipPowerLimits;
    return AH_TRUE;
}

#ifdef ATH_FORCE_BIAS
/*
 * Workaround FOWL orientation sensitivity bug by increasing rf_pwd_icsyndiv.
 * Call from ar2133SetChannel().
 *
 * Theoretical Rules:
 *   if 2 GHz band
 *      if forceBiasAuto
 *         if synth_freq < 2412
 *            bias = 0
 *         else if 2412 <= synth_freq <= 2422
 *            bias = 1
 *         else // synth_freq > 2422
 *            bias = 2
 *      else if forceBias > 0
 *         bias = forceBias & 7
 *      else
 *         no change, use value from ini file
 *   else
 *      no change, invalid band
 *
 *  1st Mod:
 *    2422 also uses value of 2
 *    <approved>
 *
 *  2nd Mod:
 *    Less than 2412 uses value of 0, 2412 and above uses value of 2
 *    <not approved>
 */
static void
ar5416ForceBiasCurrent(struct ath_hal *ah, u_int16_t synth_freq)
{
    u_int32_t            tmpReg;
    AR5416_RF_BANKS_2133 *pRfBanks;
    int                  regWrites = 0;
    u_int32_t            newBias = 0;
    struct ath_hal_5416  *ahp = AH5416(ah);


    if (synth_freq >= 3000) {
        /* force not valid outside of 2.4 band, return with no change */
        return;
    }

    pRfBanks = ahp->ah_analogBanks;

    if (ath_hal_forceBiasAuto) {
        if (synth_freq < 2412) {
            newBias = 0;
        } else if (synth_freq < 2422) {
            newBias = 1;
        } else {
            newBias = 2;
        }
    }
    else if (ath_hal_forceBias) {
        newBias = ath_hal_forceBias & 7;
    } else {
        /* (!forceBiasAuto && !forceBias): take no action */
        return;
    }

    /* pre-reverse this field */
    tmpReg = ath_hal_reverseBits(newBias, 3);

    /* DEBUG */
    ath_hal_printf(ah, "Force rf_pwd_icsyndiv to %1d on %4d (%1d %1d)\n",
            newBias, synth_freq, ath_hal_forceBiasAuto, ath_hal_forceBias);

    /* swizzle rf_pwd_icsyndiv */
    ar5416ModifyRfBuffer(pRfBanks->Bank6Data, tmpReg, 3, 181, 3);

    #ifdef DEBUG_PRINT_BANK_6
    {
        int ii;
        ath_hal_printf(ah, "DUMP BANK 6 Force Bias\n");
        for (ii = 0; ii < 33; ii++) {
            ath_hal_printf(ah, " %8.8x\n", pRfBanks->Bank6Data[ii]);
        }
    }
    #endif

    /* write Bank 6 with new params */
    REG_WRITE_RF_ARRAY(ar5416Bank6, pRfBanks->Bank6Data, regWrites);
    }
#endif /* ATH_FORCE_BIAS */


#endif /* AH_SUPPORT_2133 */
