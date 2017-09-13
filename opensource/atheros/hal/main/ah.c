/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ah.c#26 $
 */
#include "opt_ah.h"

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"

#ifdef AH_SUPPORT_AR5210
extern  struct ath_hal *ar5210Attach(u_int16_t, HAL_SOFTC,
    HAL_BUS_TAG, HAL_BUS_HANDLE, HAL_STATUS*);
#endif
#ifdef AH_SUPPORT_AR5211
extern  struct ath_hal *ar5211Attach(u_int16_t, HAL_SOFTC,
    HAL_BUS_TAG, HAL_BUS_HANDLE, HAL_STATUS*);
#endif
#ifdef AH_SUPPORT_AR5212
extern  struct ath_hal *ar5212Attach(u_int16_t, HAL_SOFTC,
    HAL_BUS_TAG, HAL_BUS_HANDLE, HAL_STATUS*);
#endif
#ifdef AH_SUPPORT_AR5312
extern  struct ath_hal *ar5312Attach(u_int16_t, HAL_SOFTC,
    HAL_BUS_TAG, HAL_BUS_HANDLE, HAL_STATUS*);
#endif
#ifdef AH_SUPPORT_AR5416
extern  struct ath_hal *ar5416Attach(u_int16_t, HAL_SOFTC,
    HAL_BUS_TAG, HAL_BUS_HANDLE, u_int32_t, HAL_STATUS*);
#endif

#include "version.h"
char ath_hal_version[] = ATH_HAL_VERSION;
const char* ath_hal_buildopts[] = {
#ifdef AH_SUPPORT_AR5210
    "AR5210",
#endif
#ifdef AH_SUPPORT_AR5211
    "AR5211",
#endif
#ifdef AH_SUPPORT_AR5212
    "AR5212",
#endif
#ifdef AH_SUPPORT_AR5416
    "AR5416",
#endif
#ifdef AH_SUPPORT_AR5312
    "AR5312",
#endif
#ifdef AH_SUPPORT_5111
    "RF5111",
#endif
#ifdef AH_SUPPORT_5112
    "RF5112",
#endif
#ifdef AH_SUPPORT_2413
    "RF2413",
#endif
#ifdef AH_SUPPORT_2316
    "RF2316",
#endif
#ifdef AH_DEBUG
    "DEBUG",
#endif
#ifdef AH_ASSERT
    "ASSERT",
#endif
#ifdef AH_DEBUG_ALQ
    "DEBUG_ALQ",
#endif
#ifdef AH_REGOPS_FUNC
    "REGOPS_FUNC",
#endif
#ifdef AH_ENABLE_TX_TPC
    "ENABLE_TX_TPC",
#endif
#ifdef AH_PRIVATE_DIAG
    "PRIVATE_DIAG",
#endif
#ifdef AH_WRITE_EEPROM
    "WRITE_EEPROM",
#endif
#ifdef AH_WRITE_REGDOMAIN
    "WRITE_REGDOMAIN",
#endif
#ifdef AH_NEED_DESC_SWAP
    "TX_DESC_SWAP",
#endif
    AH_NULL
};

static const char*
ath_hal_devname(u_int16_t devid)
{
    switch (devid) {
    case AR5210_PROD:
    case AR5210_DEFAULT:
        return "Atheros 5210";

    case AR5211_DEVID:
    case AR5311_DEVID:
    case AR5211_DEFAULT:
        return "Atheros 5211";
    case AR5211_FPGA11B:
        return "Atheros 5211 (FPGA)";

    case AR5212_FPGA:
        return "Atheros 5212 (FPGA)";
    case AR5212_AR5312_REV2:
    case AR5212_AR5312_REV7:
        return "Atheros 5312 WiSoC";
    case AR5416_DEVID_EMU:
        return "Atheros 5416 Owl emulation";
    case AR5416_DEVID_EMU_PCIE:
        return "Atheros 5416 Owl PCIE emulation";
    case AR5416_DEVID_PCI:
        return "Atheros 5416 PCI";
    case AR5416_DEVID_PCIE:
        return "Atheros 5416 PCIE";
    case AR5212_AR2315_REV6:
    case AR5212_AR2315_REV7:
        return "Atheros 2315 WiSoC";
    case AR5212_AR2313_REV8:
        return "Atheros 2313 WiSoC";
    case AR5212_DEVID:
    case AR5212_DEVID_IBM:
    case AR5212_AR2413:
    case AR5212_DEFAULT:
        return "Atheros 5212";
    }
    return AH_NULL;
}

const char*
ath_hal_probe(u_int16_t vendorid, u_int16_t devid)
{
    return (vendorid == ATHEROS_VENDOR_ID ||
        vendorid == ATHEROS_3COM_VENDOR_ID ||
        vendorid == ATHEROS_3COM2_VENDOR_ID ?
            ath_hal_devname(devid) : 0);
}

/*
 * Attach detects device chip revisions, initializes the hwLayer
 * function list, reads EEPROM information,
 * selects reset vectors, and performs a short self test.
 * Any failures will return an error that should cause a hardware
 * disable.
 */
struct ath_hal*
ath_hal_attach(u_int16_t devid, HAL_SOFTC sc,
    HAL_BUS_TAG st, HAL_BUS_HANDLE sh, u_int32_t flags, HAL_STATUS *error)
{
    struct ath_hal *ah=AH_NULL;

    switch (devid) {
#ifdef AH_SUPPORT_AR5312
    case AR5212_AR5312_REV2:
    case AR5212_AR5312_REV7:
    case AR5212_AR2313_REV8:
    case AR5212_AR2315_REV6:
    case AR5212_AR2315_REV7:
        ah = ar5312Attach(devid, sc, st, sh, error);
        break;
#endif
#ifdef AH_SUPPORT_AR5212
    case AR5212_DEVID_IBM:
    case AR5212_AR2413:
        devid = AR5212_DEVID;
        /* fall thru... */
    case AR5212_DEVID:
    case AR5212_FPGA:
    case AR5212_DEFAULT:
        ah = ar5212Attach(devid, sc, st, sh, error);
        break;
#endif
#ifdef AH_SUPPORT_AR5211
    case AR5211_DEVID:
    case AR5311_DEVID:
    case AR5211_FPGA11B:
    case AR5211_DEFAULT:
        ah = ar5211Attach(devid, sc, st, sh, error);
        break;
#endif
#ifdef AH_SUPPORT_AR5210
    case AR5210_AP:
    case AR5210_PROD:
    case AR5210_DEFAULT:
        ah = ar5210Attach(devid, sc, st, sh, error);
        break;
#endif
#ifdef AH_SUPPORT_AR5416
        case AR5416_DEVID_EMU_PCIE:
        case AR5416_DEVID_EMU:
        case AR5416_DEVID_PCI:
        case AR5416_DEVID_PCIE:
                ah = ar5416Attach(devid, sc, st, sh, flags, error);
            break;
#endif
    default:
        ah = AH_NULL;
        *error = HAL_ENXIO;
        break;
    }
    if (ah != AH_NULL) {
        /* copy back private state to public area */
        ah->ah_devid = AH_PRIVATE(ah)->ah_devid;
        ah->ah_subvendorid = AH_PRIVATE(ah)->ah_subvendorid;
        ah->ah_macVersion = AH_PRIVATE(ah)->ah_macVersion;
        ah->ah_macRev = AH_PRIVATE(ah)->ah_macRev;
        ah->ah_phyRev = AH_PRIVATE(ah)->ah_phyRev;
        ah->ah_analog5GhzRev = AH_PRIVATE(ah)->ah_analog5GhzRev;
        ah->ah_analog2GhzRev = AH_PRIVATE(ah)->ah_analog2GhzRev;
    }
    return ah;
}

/*
 * Poll the register looking for a specific value.
 */
HAL_BOOL
ath_hal_wait(struct ath_hal *ah, u_int reg, u_int32_t mask, u_int32_t val)
{
#ifdef AR5416_EMULATION
#define AH_TIMEOUT 100000
#else
#define AH_TIMEOUT  1000
#endif
    int i;

    for (i = 0; i < AH_TIMEOUT; i++) {
        if ((OS_REG_READ(ah, reg) & mask) == val)
            return AH_TRUE;
        OS_DELAY(10);
    }
    HALDEBUG(ah, "%s: timeout on reg 0x%x: 0x%08x & 0x%08x != 0x%08x\n",
        __func__, reg, OS_REG_READ(ah, reg), mask, val);
    return AH_FALSE;
#undef AH_TIMEOUT
}

/*
 * Reverse the bits starting at the low bit for a value of
 * bit_count in size
 */
u_int32_t
ath_hal_reverseBits(u_int32_t val, u_int32_t n)
{
    u_int32_t retval;
    int i;

    for (i = 0, retval = 0; i < n; i++) {
        retval = (retval << 1) | (val & 1);
        val >>= 1;
    }
    return retval;
}

/*
 * Compute the time to transmit a frame of length frameLen bytes
 * using the specified rate, phy, and short preamble setting.
 */
u_int16_t
ath_hal_computetxtime(struct ath_hal *ah,
    const HAL_RATE_TABLE *rates, u_int32_t frameLen, u_int16_t rateix,
    HAL_BOOL shortPreamble)
{
    u_int32_t bitsPerSymbol, numBits, numSymbols, phyTime, txTime;
    u_int32_t kbps;

    kbps = rates->info[rateix].rateKbps;
    /*
     * index can be invalid duting dynamic Turbo transitions.
     */
    if(kbps == 0) return 0;
    switch (rates->info[rateix].phy) {

    case IEEE80211_T_CCK:
#define CCK_PREAMBLE_BITS   144
#define CCK_PLCP_BITS        48
        phyTime     = CCK_PREAMBLE_BITS + CCK_PLCP_BITS;
        if (shortPreamble && rates->info[rateix].shortPreamble)
            phyTime >>= 1;
        numBits     = frameLen << 3;
        txTime      = phyTime + ((numBits * 1000)/kbps);
        break;
#undef CCK_PREAMBLE_BITS
#undef CCK_PLCP_BITS

    case IEEE80211_T_OFDM:
#define OFDM_PREAMBLE_TIME    20
#define OFDM_PLCP_BITS        22
#define OFDM_SYMBOL_TIME       4

#define OFDM_SIFS_TIME_HALF 32
#define OFDM_PREAMBLE_TIME_HALF 40
#define OFDM_PLCP_BITS_HALF 22
#define OFDM_SYMBOL_TIME_HALF   8

#define OFDM_SIFS_TIME_QUARTER      64
#define OFDM_PREAMBLE_TIME_QUARTER  80
#define OFDM_PLCP_BITS_QUARTER      22
#define OFDM_SYMBOL_TIME_QUARTER    16

        if (AH_PRIVATE(ah)->ah_curchan &&
            IS_CHAN_QUARTER_RATE(AH_PRIVATE(ah)->ah_curchan)) {
            bitsPerSymbol   = (kbps * OFDM_SYMBOL_TIME_QUARTER) / 1000;
            HALASSERT(bitsPerSymbol != 0);

            numBits     = OFDM_PLCP_BITS + (frameLen << 3);
            numSymbols  = howmany(numBits, bitsPerSymbol);
            txTime      = OFDM_SIFS_TIME_QUARTER
                        + OFDM_PREAMBLE_TIME_QUARTER
                    + (numSymbols * OFDM_SYMBOL_TIME_QUARTER);
        } else if (AH_PRIVATE(ah)->ah_curchan &&
                IS_CHAN_HALF_RATE(AH_PRIVATE(ah)->ah_curchan)) {
            bitsPerSymbol   = (kbps * OFDM_SYMBOL_TIME_HALF) / 1000;
            HALASSERT(bitsPerSymbol != 0);

            numBits     = OFDM_PLCP_BITS + (frameLen << 3);
            numSymbols  = howmany(numBits, bitsPerSymbol);
            txTime      = OFDM_SIFS_TIME_HALF +
                        OFDM_PREAMBLE_TIME_HALF
                    + (numSymbols * OFDM_SYMBOL_TIME_HALF);
        } else { /* full rate channel */
            bitsPerSymbol   = (kbps * OFDM_SYMBOL_TIME) / 1000;
            HALASSERT(bitsPerSymbol != 0);

            numBits     = OFDM_PLCP_BITS + (frameLen << 3);
            numSymbols  = howmany(numBits, bitsPerSymbol);
            txTime      = OFDM_PREAMBLE_TIME + (numSymbols * OFDM_SYMBOL_TIME);
        }
        break;

#undef OFDM_PREAMBLE_TIME
#undef OFDM_PLCP_BITS
#undef OFDM_SYMBOL_TIME

    case IEEE80211_T_TURBO:
#define TURBO_SIFS_TIME         8
#define TURBO_PREAMBLE_TIME    14
#define TURBO_PLCP_BITS        22
#define TURBO_SYMBOL_TIME       4
        /* we still save OFDM rates in kbps - so double them */
        bitsPerSymbol = ((kbps << 1) * TURBO_SYMBOL_TIME) / 1000;
        HALASSERT(bitsPerSymbol != 0);

        numBits       = TURBO_PLCP_BITS + (frameLen << 3);
        numSymbols    = howmany(numBits, bitsPerSymbol);
        txTime        = TURBO_SIFS_TIME + TURBO_PREAMBLE_TIME
                  + (numSymbols * TURBO_SYMBOL_TIME);
        break;
#undef TURBO_SIFS_TIME
#undef TURBO_PREAMBLE_TIME
#undef TURBO_PLCP_BITS
#undef TURBO_SYMBOL_TIME

    case ATHEROS_T_XR:
#define XR_SIFS_TIME            16
#define XR_PREAMBLE_TIME(_kpbs) (((_kpbs) < 1000) ? 173 : 76)
#define XR_PLCP_BITS            22
#define XR_SYMBOL_TIME           4
        bitsPerSymbol = (kbps * XR_SYMBOL_TIME) / 1000;
        HALASSERT(bitsPerSymbol != 0);

        numBits       = XR_PLCP_BITS + (frameLen << 3);
        numSymbols    = howmany(numBits, bitsPerSymbol);
        txTime        = XR_SIFS_TIME + XR_PREAMBLE_TIME(kbps)
                   + (numSymbols * XR_SYMBOL_TIME);
        break;
#undef XR_SIFS_TIME
#undef XR_PREAMBLE_TIME
#undef XR_PLCP_BITS
#undef XR_SYMBOL_TIME

    default:
        HALDEBUG(ah, "%s: unknown phy %u (rate ix %u)\n",
            __func__, rates->info[rateix].phy, rateix);
        txTime = 0;
        break;
    }
    return txTime;
}

WIRELESS_MODE
ath_hal_chan2wmode(struct ath_hal *ah, const HAL_CHANNEL *chan)
{
    if (IS_CHAN_CCK(chan))
        return WIRELESS_MODE_11b;
    if (IS_CHAN_G(chan))
        return WIRELESS_MODE_11g;
    if (IS_CHAN_108G(chan))
        return WIRELESS_MODE_108g;
    if (IS_CHAN_TURBO(chan))
        return WIRELESS_MODE_TURBO;
    if (IS_CHAN_XR(chan))
        return WIRELESS_MODE_XR;
    return WIRELESS_MODE_11a;
}

/*
 * Convert GHz frequency to IEEE channel number.
 */
u_int
ath_hal_mhz2ieee(struct ath_hal *ah, u_int freq, u_int flags)
{
    if (flags & CHANNEL_2GHZ) { /* 2GHz band */
        if (freq == 2484)
            return 14;
        if (freq < 2484)
            return (freq - 2407) / 5;
        else
            return 15 + ((freq - 2512) / 20);
    } else if (flags & CHANNEL_5GHZ) {/* 5Ghz band */
        if (ath_hal_ispublicsafetysku(ah) &&
            IS_CHAN_IN_PUBLIC_SAFETY_BAND(freq)) {
            return ((freq * 10) +
                (((freq % 5) == 2) ? 5 : 0) - 49400) / 5;
        } else if (((flags & CHANNEL_A) || (flags & CHANNEL_A_HT20) || (flags & CHANNEL_A_HT40))
               && (freq <= 5000)) {
            return (freq - 4000) / 5;
        } else {
            return (freq - 5000) / 5;
        }
    } else {            /* either, guess */
        if (freq == 2484)
            return 14;
        if (freq < 2484)
            return (freq - 2407) / 5;
        if (freq < 5000) {
            if (ath_hal_ispublicsafetysku(ah)
                && IS_CHAN_IN_PUBLIC_SAFETY_BAND(freq)) {
                return ((freq * 10) +
                    (((freq % 5) == 2) ? 5 : 0) - 49400)/5;
            } else if (freq > 4900) {
                return (freq - 4000) / 5;
            } else {
                return 15 + ((freq - 2512) / 20);
            }
        }
        return (freq - 5000) / 5;
    }
}

/*
 * Convert between microseconds and core system clocks.
 */

u_int
ath_hal_mac_clks(struct ath_hal *ah, HAL_HT_CWM *htcwm, u_int usecs)
{
    HAL_CHANNEL_INTERNAL *c = AH_PRIVATE(ah)->ah_curchan;

    /* NB: ah_curchan may be null when called attach time */
    if (c != AH_NULL) {
        if (IS_CHAN_TURBO(c))
            return usecs * 80;
        else if (IS_CHAN_A(c)) {
		if (htcwm && (htcwm->ht_phymode == HAL_HT_PHYMODE_2040)) {
			/* If A-HT20/40 mul by 80 Mhz clock */
			return usecs * 80; 
		} else {
			/* legacy 11a or 20 MHz 11na */
			return usecs * 40; 
		}
	}
        else if (IS_CHAN_G(c)) {
		if (htcwm && (htcwm->ht_phymode == HAL_HT_PHYMODE_2040)) {
			/* If G-HT20/40 mul by 88 Mhz clock */
			return usecs * 88; 
		} else {
			/* legacy 11g or 20 MHz 11ng */
			return usecs * 44; 
		}
	}
    }
    return usecs * 22;
}

u_int
ath_hal_mac_usec(struct ath_hal *ah, HAL_HT_CWM *htcwm, u_int clks)
{
    HAL_CHANNEL_INTERNAL *c = AH_PRIVATE(ah)->ah_curchan;

    /* NB: ah_curchan may be null when called attach time */
    if (c != AH_NULL) {
        if (IS_CHAN_TURBO(c))
            return clks / 80;
        else if (IS_CHAN_A(c)) {
		if (htcwm && (htcwm->ht_phymode == HAL_HT_PHYMODE_2040)) {
			return clks / 80;
		} else {
			return clks / 40;
		}
	}
        else if (IS_CHAN_G(c)) {
		if (htcwm && (htcwm->ht_phymode == HAL_HT_PHYMODE_2040)) {
			return clks / 88;

		} else {
			return clks / 44;
		}
	}
    }
    return clks / 22;
}

void
ath_hal_setupratetable(struct ath_hal *ah, HAL_RATE_TABLE *rt)
{
    return;
}

HAL_STATUS
ath_hal_getcapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
    u_int32_t capability, u_int32_t *result)
{
    const HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;

    switch (type) {
    case HAL_CAP_REG_DMN:       /* regulatory domain */
        *result = AH_PRIVATE(ah)->ah_currentRD;
        return HAL_OK;
    case HAL_CAP_CIPHER:        /* cipher handled in hardware */
    case HAL_CAP_TKIP_MIC:      /* handle TKIP MIC in hardware */
        return HAL_ENOTSUPP;
    case HAL_CAP_TKIP_SPLIT:    /* hardware TKIP uses split keys */
        return HAL_ENOTSUPP;
    case HAL_CAP_WME_TKIPMIC:   /* hardware can do TKIP MIC when WMM is turned on */
        return HAL_ENOTSUPP;
    case HAL_CAP_DIVERSITY:     /* hardware supports fast diversity */
        return HAL_ENOTSUPP;
    case HAL_CAP_KEYCACHE_SIZE: /* hardware key cache size */
        *result =  pCap->halKeyCacheSize;
        return HAL_OK;
    case HAL_CAP_NUM_TXQUEUES:  /* number of hardware tx queues */
        *result = pCap->halTotalQueues;
        return HAL_OK;
    case HAL_CAP_VEOL:      /* hardware supports virtual EOL */
        return pCap->halVEOLSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_PSPOLL:        /* hardware PS-Poll support works */
        return pCap->halPSPollBroken ? HAL_ENOTSUPP : HAL_OK;
    case HAL_CAP_COMPRESSION:
        return pCap->halCompressSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_BURST:
        return pCap->halBurstSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_FASTFRAME:
        return pCap->halFastFramesSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_DIAG:      /* hardware diagnostic support */
        *result = AH_PRIVATE(ah)->ah_diagreg;
        return HAL_OK;
    case HAL_CAP_TXPOW:     /* global tx power limit  */
        switch (capability) {
        case 0:         /* facility is supported */
            return HAL_OK;
        case 1:         /* current limit */
            *result = AH_PRIVATE(ah)->ah_powerLimit;
            return HAL_OK;
        case 2:         /* current max tx power */
            *result = AH_PRIVATE(ah)->ah_maxPowerLevel;
            return HAL_OK;
        case 3:         /* scale factor */
            *result = AH_PRIVATE(ah)->ah_tpScale;
            return HAL_OK;
        }
        return HAL_ENOTSUPP;
    case HAL_CAP_BSSIDMASK:     /* hardware supports bssid mask */
        return pCap->halBssIdMaskSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_MCAST_KEYSRCH: /* multicast frame keycache search */
        return pCap->halMcastKeySrchSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_TSF_ADJUST:    /* hardware has beacon tsf adjust */
        return HAL_ENOTSUPP;
    case HAL_CAP_XR:
        return pCap->halXrSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_CHAN_HALFRATE:
        return pCap->halChanHalfRate ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_CHAN_QUARTERRATE:
        return pCap->halChanQuarterRate ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_HT:
        return pCap->halHTSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_GTT:
        return pCap->halGTTSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_FAST_CC:
        return pCap->halFastCCSupport ? HAL_OK : HAL_ENOTSUPP;
    case HAL_CAP_NUM_MR_RETRIES:
        *result = pCap->halNumMRRetries;
        return HAL_OK;
    case HAL_CAP_TX_CHAINMASK:
        *result = pCap->halTxChainMask;
        return HAL_OK;
    case HAL_CAP_RX_CHAINMASK:
        *result = pCap->halRxChainMask;
        return HAL_OK;
    default:
        return HAL_EINVAL;
    }
}

HAL_BOOL
ath_hal_setcapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
    u_int32_t capability, u_int32_t setting, HAL_STATUS *status)
{

    switch (type) {
    case HAL_CAP_TXPOW:
        switch (capability) {
        case 3:
            if (setting <= HAL_TP_SCALE_MIN) {
                AH_PRIVATE(ah)->ah_tpScale = setting;
                return AH_TRUE;
            }
            break;
        }
        break;
    default:
        break;
    }
    if (status)
        *status = HAL_EINVAL;
    return AH_FALSE;
}

/*
 * Common support for getDiagState method.
 */

static u_int
ath_hal_getregdump(struct ath_hal *ah, const HAL_REGRANGE *regs,
    void *dstbuf, int space)
{
    u_int32_t *dp = dstbuf;
    int i;

    for (i = 0; space >= 2*sizeof(u_int32_t); i++) {
        u_int r = regs[i].start;
        u_int e = regs[i].end;
        *dp++ = (r<<16) | e;
        space -= sizeof(u_int32_t);
        do {
            *dp++ = OS_REG_READ(ah, r);
            r += sizeof(u_int32_t);
            space -= sizeof(u_int32_t);
        } while (r <= e && space >= sizeof(u_int32_t));
    }
    return (char *) dp - (char *) dstbuf;
}

HAL_BOOL
ath_hal_getdiagstate(struct ath_hal *ah, int request,
    const void *args, u_int32_t argsize,
    void **result, u_int32_t *resultsize)
{
    switch (request) {
    case HAL_DIAG_DMADBG:
	ah->ah_dmaRegDump(ah);
	return AH_TRUE;
    case HAL_DIAG_REVS:
        *result = &AH_PRIVATE(ah)->ah_devid;
        *resultsize = sizeof(HAL_REVS);
        return AH_TRUE;
    case HAL_DIAG_REGS:
        *resultsize = ath_hal_getregdump(ah, args, *result,*resultsize);
        return AH_TRUE;
	case HAL_DIAG_REGREAD: {
        if (argsize != sizeof(u_int))
		   return AH_FALSE;
		**(u_int32_t **)result = OS_REG_READ(ah, *(const u_int *)args);
		*resultsize = sizeof(u_int32_t);
		return AH_TRUE;	   
	}
    case HAL_DIAG_GET_REGBASE: {
		/* Should be HAL_BUS_HANDLE but compiler warns and hal build set to
		   treat warnings as errors. */
		**(u_int **)result =(u_int )ah->ah_sh;
		*resultsize = sizeof(HAL_BUS_HANDLE);
		return AH_TRUE;
	}
	case HAL_DIAG_REGWRITE: {
        const HAL_DIAG_REGVAL *rv;
		if (argsize != sizeof(HAL_DIAG_REGVAL))
			return AH_FALSE;
		rv = (const HAL_DIAG_REGVAL *)args;
		OS_REG_WRITE(ah, rv->offset, rv->val);
		return AH_TRUE;	  
	}
    case HAL_DIAG_EEREAD:
        if (argsize != sizeof(u_int16_t))
            return AH_FALSE;
        if (!ath_hal_eepromRead(ah, *(const u_int16_t *)args, *result))
            return AH_FALSE;
        *resultsize = sizeof(u_int16_t);
        return AH_TRUE;
    case HAL_DIAG_EEPROM:
        {
            extern int ar5416EepromDumpSupport(struct ath_hal *ah, void **ep);
            void *dstbuf;
            *resultsize = ar5416EepromDumpSupport(ah, &dstbuf);
            *result = dstbuf;
        }
        return AH_TRUE;
#ifdef AH_SUPPORT_WRITE_EEPROM
    case HAL_DIAG_EEWRITE: {
        const HAL_DIAG_EEVAL *ee;

        if (argsize != sizeof(HAL_DIAG_EEVAL))
            return AH_FALSE;
        ee = (const HAL_DIAG_EEVAL *)args;
        return ath_hal_eepromWrite(ah, ee->ee_off, ee->ee_data);
    }
#endif /* AH_SUPPORT_WRITE_EEPROM */
#ifdef AH_PRIVATE_DIAG
    case HAL_DIAG_SETKEY: {
        const HAL_DIAG_KEYVAL *dk;

        if (argsize != sizeof(HAL_DIAG_KEYVAL))
            return AH_FALSE;
        dk = (const HAL_DIAG_KEYVAL *)args;
        return ah->ah_setKeyCacheEntry(ah, dk->dk_keyix,
            &dk->dk_keyval, dk->dk_mac, dk->dk_xor);
    }
    case HAL_DIAG_RESETKEY:
        if (argsize != sizeof(u_int16_t))
            return AH_FALSE;
        return ah->ah_resetKeyCacheEntry(ah, *(const u_int16_t *)args);
#endif /* AH_PRIVATE_DIAG */
    }
    return AH_FALSE;
}

/*
 * Set the properties of the tx queue with the parameters
 * from qInfo.
 */
HAL_BOOL
ath_hal_setTxQProps(struct ath_hal *ah,
    HAL_TX_QUEUE_INFO *qi, const HAL_TXQ_INFO *qInfo)
{
    u_int32_t cw;

    if (qi->tqi_type == HAL_TX_QUEUE_INACTIVE) {
        HALDEBUG(ah, "%s: inactive queue\n", __func__);
        return AH_FALSE;
    }

    HALDEBUG(ah, "%s: queue %p\n", __func__, qi);

    /* XXX validate parameters */
    qi->tqi_ver = qInfo->tqi_ver;
    qi->tqi_subtype = qInfo->tqi_subtype;
    qi->tqi_qflags = qInfo->tqi_qflags;
    qi->tqi_priority = qInfo->tqi_priority;
    if (qInfo->tqi_aifs != HAL_TXQ_USEDEFAULT)
        qi->tqi_aifs = AH_MIN(qInfo->tqi_aifs, 255);
    else
        qi->tqi_aifs = INIT_AIFS;
    if (qInfo->tqi_cwmin != HAL_TXQ_USEDEFAULT) {
        cw = AH_MIN(qInfo->tqi_cwmin, 1024);
        /* make sure that the CWmin is of the form (2^n - 1) */
        qi->tqi_cwmin = 1;
        while (qi->tqi_cwmin < cw)
            qi->tqi_cwmin = (qi->tqi_cwmin << 1) | 1;
    } else
        qi->tqi_cwmin = qInfo->tqi_cwmin;
    if (qInfo->tqi_cwmax != HAL_TXQ_USEDEFAULT) {
        cw = AH_MIN(qInfo->tqi_cwmax, 1024);
        /* make sure that the CWmax is of the form (2^n - 1) */
        qi->tqi_cwmax = 1;
        while (qi->tqi_cwmax < cw)
            qi->tqi_cwmax = (qi->tqi_cwmax << 1) | 1;
    } else
        qi->tqi_cwmax = INIT_CWMAX;
    /* Set retry limit values */
    if (qInfo->tqi_shretry != 0)
        qi->tqi_shretry = AH_MIN(qInfo->tqi_shretry, 15);
    else
        qi->tqi_shretry = INIT_SH_RETRY;
    if (qInfo->tqi_lgretry != 0)
        qi->tqi_lgretry = AH_MIN(qInfo->tqi_lgretry, 15);
    else
        qi->tqi_lgretry = INIT_LG_RETRY;
    qi->tqi_cbrPeriod = qInfo->tqi_cbrPeriod;
    qi->tqi_cbrOverflowLimit = qInfo->tqi_cbrOverflowLimit;
    qi->tqi_burstTime = qInfo->tqi_burstTime;
    qi->tqi_readyTime = qInfo->tqi_readyTime;

    switch (qInfo->tqi_subtype) {
    case HAL_WME_UPSD:
        if (qi->tqi_type == HAL_TX_QUEUE_DATA)
            qi->tqi_intFlags = HAL_TXQ_USE_LOCKOUT_BKOFF_DIS;
        break;
    default:
        break;      /* NB: silence compiler */
    }
    return AH_TRUE;
}

HAL_BOOL
ath_hal_getTxQProps(struct ath_hal *ah,
    HAL_TXQ_INFO *qInfo, const HAL_TX_QUEUE_INFO *qi)
{
    if (qi->tqi_type == HAL_TX_QUEUE_INACTIVE) {
        HALDEBUG(ah, "%s: inactive queue\n", __func__);
        return AH_FALSE;
    }

    qInfo->tqi_qflags = qi->tqi_qflags;
    qInfo->tqi_ver = qi->tqi_ver;
    qInfo->tqi_subtype = qi->tqi_subtype;
    qInfo->tqi_qflags = qi->tqi_qflags;
    qInfo->tqi_priority = qi->tqi_priority;
    qInfo->tqi_aifs = qi->tqi_aifs;
    qInfo->tqi_cwmin = qi->tqi_cwmin;
    qInfo->tqi_cwmax = qi->tqi_cwmax;
    qInfo->tqi_shretry = qi->tqi_shretry;
    qInfo->tqi_lgretry = qi->tqi_lgretry;
    qInfo->tqi_cbrPeriod = qi->tqi_cbrPeriod;
    qInfo->tqi_cbrOverflowLimit = qi->tqi_cbrOverflowLimit;
    qInfo->tqi_burstTime = qi->tqi_burstTime;
    qInfo->tqi_readyTime = qi->tqi_readyTime;
    return AH_TRUE;
}
