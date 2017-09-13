/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_misc.c#29 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"
#ifdef AH_DEBUG
#include "ah_desc.h"                    /* NB: for HAL_PHYERR* */
#endif

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"
#include "ar5416/ar5416desc.h"

#define ANT_SWITCH_TABLE1               AR_PHY(88)
#define ANT_SWITCH_TABLE2               AR_PHY(89)
#define ar_set_led_mode(_mode)  (SM(_mode, AR_MAC_LED_MODE_SEL))

void
ar5416GetMacAddress(struct ath_hal *ah, u_int8_t *mac)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        OS_MEMCPY(mac, ahp->ah_macaddr, IEEE80211_ADDR_LEN);
}

HAL_BOOL
ar5416SetMacAddress(struct ath_hal *ah, const u_int8_t *mac)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        OS_MEMCPY(ahp->ah_macaddr, mac, IEEE80211_ADDR_LEN);
        return AH_TRUE;
}

void
ar5416GetBssIdMask(struct ath_hal *ah, u_int8_t *mask)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        OS_MEMCPY(mask, ahp->ah_bssidmask, IEEE80211_ADDR_LEN);
}

HAL_BOOL
ar5416SetBssIdMask(struct ath_hal *ah, const u_int8_t *mask)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        /* save it since it must be rewritten on reset */
        OS_MEMCPY(ahp->ah_bssidmask, mask, IEEE80211_ADDR_LEN);

        OS_REG_WRITE(ah, AR_BSSMSKL, LE_READ_4(ahp->ah_bssidmask));
        OS_REG_WRITE(ah, AR_BSSMSKU, LE_READ_2(ahp->ah_bssidmask + 4));
        return AH_TRUE;
}

/*
 * Attempt to change the cards operating regulatory domain to the given value
 * Returns: A_EINVAL for an unsupported regulatory domain.
 *          A_HARDWARE for an unwritable EEPROM or bad EEPROM version
 */
HAL_BOOL
ar5416SetRegulatoryDomain(struct ath_hal *ah,
        u_int16_t regDomain, HAL_STATUS *status)
{
#ifdef AH_SUPPORT_WRITE_REGDOMAIN
        struct ath_hal_5416 *ahp = AH5416(ah);
        HAL_STATUS ecode;

        if (AH_PRIVATE(ah)->ah_currentRD == regDomain) {
                ecode = HAL_EINVAL;
                goto bad;
        }
        if (ahp->ah_eeprotect & AR_EEPROM_PROTECT_WP_128_191) {
                ecode = HAL_EEWRITE;
                goto bad;
        }

        if (ath_hal_eepromWrite(ah, AR_EEPROM_REG_DOMAIN, regDomain)) {
                HALDEBUG(ah, "%s: set regulatory domain to %u (0x%x)\n",
                        __func__, regDomain, regDomain);
                AH_PRIVATE(ah)->ah_currentRD = regDomain;
                return AH_TRUE;
        }
        ecode = HAL_EIO;
bad:
        if (status)
                *status = ecode;
#endif
        return AH_FALSE;
}

/*
 * Return the wireless modes (a,b,g,t) supported by hardware.
 *
 * This value is what is actually supported by the hardware
 * and is unaffected by regulatory/country code settings.
 *
 */
u_int
ar5416GetWirelessModes(struct ath_hal *ah)
{
    return AH_PRIVATE(ah)->ah_caps.halWirelessModes;
}

/*
 * Accessor to get rfkill info from private EEPROM structures
 */
HAL_BOOL
ar5416GetRfKill(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    int rfSilent;

    rfSilent = ar5416EepromGet(ahp, EEP_RF_SILENT);

    if (rfSilent & EEP_RFSILENT_ENABLED) {
        ahp->ah_gpioSelect = MS(rfSilent, EEP_RFSILENT_GPIO_SEL);
        ahp->ah_polarity = MS(rfSilent, EEP_RFSILENT_POLARITY);
        ahp->ah_eepEnabled = AH_TRUE;
    } else {
        ahp->ah_gpioSelect = 0;
        ahp->ah_polarity = 0;
        ahp->ah_eepEnabled = AH_FALSE;
    }
    return (ahp->ah_eepEnabled);
}

/*
 * Set the interrupt and GPIO values so the ISR can disable RF
 * on a switch signal.  Assumes GPIO port and interrupt polarity
 * are set prior to call.
 */
void
ar5416EnableRfKill(struct ath_hal *ah)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        /*
         * Configure the desired GPIO port for input
         * and enable baseband rf silence.
         */
        ath_hal_gpioCfgInput(ah, ahp->ah_gpioSelect);
        OS_REG_SET_BIT(ah, AR_PHY_TEST, RFSILENT_BB);

        /*
         * If radio disable switch connection to GPIO bit x is enabled
         * program GPIO interrupt.
         * If rfkill bit on eeprom is 1, setupeeprommap routine has already
         * verified that it is a later version of eeprom, it has a place for
         * rfkill bit and it is set to 1, indicating that GPIO bit x hardware
         * connection is present.
         */
        ahp->ah_gpioBit = ath_hal_gpioGet(ah, ahp->ah_gpioSelect);
        ath_hal_gpioSetIntr(ah, ahp->ah_gpioSelect,
                (ahp->ah_gpioBit != ahp->ah_polarity));
}

/*
 * Change the LED blinking pattern to correspond to the connectivity
 */
void
ar5416SetLedState(struct ath_hal *ah, HAL_LED_STATE state)
{
        static const u_int32_t ledbits[8] = {
                AR_MAC_LED_ASSOC_NONE | ar_set_led_mode(AR_MAC_LED_MODE_PROP), //INIT
                AR_MAC_LED_ASSOC_PENDING | ar_set_led_mode(AR_MAC_LED_MODE_PROP),//SCAN
                AR_MAC_LED_ASSOC_PENDING | ar_set_led_mode(AR_MAC_LED_MODE_PROP),//AUTH
                AR_MAC_LED_ASSOC_ACTIVE | ar_set_led_mode(AR_MAC_LED_MODE_PROP),//ASSOC
                AR_MAC_LED_ASSOC_ACTIVE | ar_set_led_mode(AR_MAC_LED_MODE_PROP),//RUN
                AR_MAC_LED_ASSOC_NONE | AR_MAC_LED_MODE_RAND,
                AR_MAC_LED_ASSOC_NONE | AR_MAC_LED_MODE_RAND,
                AR_MAC_LED_ASSOC_NONE | AR_MAC_LED_MODE_RAND,
        };
        OS_REG_WRITE(ah, AR_MAC_LED, (OS_REG_READ(ah, AR_MAC_LED) &~
                                                                 (AR_MAC_LED_ASSOC_CTL | AR_MAC_LED_MODE_SEL))
                                 | ledbits[state & 0x7]);
}

/*
 * Change association related fields programmed into the hardware.
 * Writing a valid BSSID to the hardware effectively enables the hardware
 * to synchronize its TSF to the correct beacons and receive frames coming
 * from that BSSID. It is called by the SME JOIN operation.
 */
void
ar5416WriteAssocid(struct ath_hal *ah, const u_int8_t *bssid, u_int16_t assocId)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        /* XXX save bssid for possible re-use on reset */
        OS_MEMCPY(ahp->ah_bssid, bssid, IEEE80211_ADDR_LEN);
        OS_REG_WRITE(ah, AR_BSS_ID0, LE_READ_4(ahp->ah_bssid));
        OS_REG_WRITE(ah, AR_BSS_ID1, LE_READ_2(ahp->ah_bssid+4) |
                                     ((assocId & 0x3fff)<<AR_BSS_ID1_AID_S));

        /*
         * XXX WAR for a hardware bug in Oahu. Write a 0 associd to prevent
         * hardware from parsing beyond end of TIM element. It will avoid
         * sending spurious PS-polls. Beacon/TIM processing done in software
         */
        OS_REG_RMW_FIELD(ah, AR_BSS_ID1, AR_BSS_ID1_AID, 0);
}

/*
 * Get the current hardware tsf for stamlme
 */
u_int64_t
ar5416GetTsf64(struct ath_hal *ah)
{
        u_int64_t tsf;

        /* XXX sync multi-word read? */
        tsf = OS_REG_READ(ah, AR_TSF_U32);
        tsf = (tsf << 32) | OS_REG_READ(ah, AR_TSF_L32);
        return tsf;
}

/*
 * Get the current hardware tsf for stamlme
 */
u_int32_t
ar5416GetTsf32(struct ath_hal *ah)
{
        return OS_REG_READ(ah, AR_TSF_L32);
}

/*
 * Reset the current hardware tsf for stamlme.
 */
void
ar5416ResetTsf(struct ath_hal *ah)
{
    int count;

    count = 0;
    while (OS_REG_READ(ah, AR_SLP32_MODE) & AR_SLP32_TSF_WRITE_STATUS) {
        count++;
        if (count > 10) {
            HALDEBUG(ah, "%s: AR_SLP32_TSF_WRITE_STATUS limit exceeded\n", __func__);
            break;
        }
        OS_DELAY(10);
    }
    OS_REG_WRITE(ah, AR_RESET_TSF, AR_RESET_TSF_ONCE);
}

/*
 * Set or clear hardware basic rate bit
 * Set hardware basic rate set if basic rate is found
 * and basic rate is equal or less than 2Mbps
 */
void
ar5416SetBasicRate(struct ath_hal *ah, HAL_RATE_SET *rs)
{
        HAL_CHANNEL_INTERNAL *chan = AH_PRIVATE(ah)->ah_curchan;
        u_int32_t reg;
        u_int8_t xset;
        int i;

        if (chan == AH_NULL || !IS_CHAN_CCK(chan))
                return;
        xset = 0;
        for (i = 0; i < rs->rs_count; i++) {
                u_int8_t rset = rs->rs_rates[i];
                /* Basic rate defined? */
                if ((rset & 0x80) && (rset &= 0x7f) >= xset)
                        xset = rset;
        }
        /*
         * Set the h/w bit to reflect whether or not the basic
         * rate is found to be equal or less than 2Mbps.
         */
        reg = OS_REG_READ(ah, AR_STA_ID1);
        if (xset && xset/2 <= 2)
                OS_REG_WRITE(ah, AR_STA_ID1, reg | AR_STA_ID1_BASE_RATE_11B);
        else
                OS_REG_WRITE(ah, AR_STA_ID1, reg &~ AR_STA_ID1_BASE_RATE_11B);
}

/*
 * Grab a semi-random value from hardware registers - may not
 * change often
 */
u_int32_t
ar5416GetRandomSeed(struct ath_hal *ah)
{
        u_int32_t nf;

        nf = (OS_REG_READ(ah, AR_PHY(25)) >> 19) & 0x1ff;
        if (nf & 0x100)
                nf = 0 - ((nf ^ 0x1ff) + 1);
        return (OS_REG_READ(ah, AR_TSF_U32) ^
                OS_REG_READ(ah, AR_TSF_L32) ^ nf);
}

/*
 * Detect if our card is present
 */
HAL_BOOL
ar5416DetectCardPresent(struct ath_hal *ah)
{
        u_int16_t macVersion, macRev;
        u_int32_t v;

        /*
         * Read the Silicon Revision register and compare that
         * to what we read at attach time.  If the same, we say
         * a card/device is present.
         */
        v = OS_REG_READ(ah, AR_SREV) & AR_SREV_ID;
        macVersion = MS(v, AR_SREV_VERSION);
        macRev = v & AR_SREV_REVISION;
        return (AH_PRIVATE(ah)->ah_macVersion == macVersion &&
                AH_PRIVATE(ah)->ah_macRev == macRev);
}

/*
 * Update MIB Counters
 */
void
ar5416UpdateMibCounters(struct ath_hal *ah, HAL_MIB_STATS* stats)
{
        stats->ackrcv_bad += OS_REG_READ(ah, AR_ACK_FAIL);
        stats->rts_bad    += OS_REG_READ(ah, AR_RTS_FAIL);
        stats->fcs_bad    += OS_REG_READ(ah, AR_FCS_FAIL);
        stats->rts_good   += OS_REG_READ(ah, AR_RTS_OK);
        stats->beacons    += OS_REG_READ(ah, AR_BEACON_CNT);
}

/*
 * Detect if the HW supports spreading a CCK signal on channel 14
 */
HAL_BOOL
ar5416IsJapanChannelSpreadSupported(struct ath_hal *ah)
{
        return AH_TRUE;
}

/*
 * Get the rssi of frame curently being received.
 */
u_int32_t
ar5416GetCurRssi(struct ath_hal *ah)
{
        return (OS_REG_READ(ah, AR_PHY_CURRENT_RSSI) & 0xff);
}

u_int
ar5416GetDefAntenna(struct ath_hal *ah)
{
        return (OS_REG_READ(ah, AR_DEF_ANTENNA) & 0x7);
}

/* Setup coverage class */
void
ar5416SetCoverageClass(struct ath_hal *ah, u_int8_t coverageclass, int now)
{
    // TODO: owl emulation only
    OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, 0x00000168);
    OS_REG_WRITE(ah, AR_D_GBL_IFS_EIFS, 0x00000d98);
    OS_REG_WRITE(ah, AR_TIME_OUT,       0x0c000c00);
}

void
ar5416SetDefAntenna(struct ath_hal *ah, u_int antenna)
{
        OS_REG_WRITE(ah, AR_DEF_ANTENNA, (antenna & 0x7));
}

HAL_BOOL
ar5416IsSleepAfterBeaconBroken(struct ath_hal *ah)
{
        return AH_TRUE;
}

HAL_BOOL
ar5416SetSlotTime(struct ath_hal *ah, u_int us)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        if (us < HAL_SLOT_TIME_9 || us > ath_hal_mac_usec(ah, &ahp->ah_htcwm, 0xffff)) {
                HALDEBUG(ah, "%s: bad slot time %u\n", __func__, us);
                ahp->ah_slottime = (u_int) -1;  /* restore default handling */
                return AH_FALSE;
        } else {
                /* convert to system clocks */
                OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, ath_hal_mac_clks(ah, &ahp->ah_htcwm, us));
                ahp->ah_slottime = us;
                return AH_TRUE;
        }
}

u_int
ar5416GetSlotTime(struct ath_hal *ah)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        u_int clks = OS_REG_READ(ah, AR_D_GBL_IFS_SLOT) & 0xffff;
        return ath_hal_mac_usec(ah, &ahp->ah_htcwm, clks);      /* convert from system clocks */
}

HAL_BOOL
ar5416SetAckTimeout(struct ath_hal *ah, u_int us)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        if (us > ath_hal_mac_usec(ah, &ahp->ah_htcwm, MS(0xffffffff, AR_TIME_OUT_ACK))) {
                HALDEBUG(ah, "%s: bad ack timeout %u\n", __func__, us);
                ahp->ah_acktimeout = (u_int) -1; /* restore default handling */
                return AH_FALSE;
        } else {
                /* convert to system clocks */
                OS_REG_RMW_FIELD(ah, AR_TIME_OUT,
                        AR_TIME_OUT_ACK, ath_hal_mac_clks(ah, &ahp->ah_htcwm, us));
                ahp->ah_acktimeout = us;
                return AH_TRUE;
        }
}

u_int
ar5416GetAckTimeout(struct ath_hal *ah)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        u_int clks = MS(OS_REG_READ(ah, AR_TIME_OUT), AR_TIME_OUT_ACK);
        return ath_hal_mac_usec(ah, &ahp->ah_htcwm, clks);      /* convert from system clocks */
}

HAL_BOOL
ar5416SetCTSTimeout(struct ath_hal *ah, u_int us)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        if (us > ath_hal_mac_usec(ah, &ahp->ah_htcwm, MS(0xffffffff, AR_TIME_OUT_CTS))) {
                HALDEBUG(ah, "%s: bad cts timeout %u\n", __func__, us);
                ahp->ah_ctstimeout = (u_int) -1; /* restore default handling */
                return AH_FALSE;
        } else {
                /* convert to system clocks */
                OS_REG_RMW_FIELD(ah, AR_TIME_OUT,
                        AR_TIME_OUT_CTS, ath_hal_mac_clks(ah, &ahp->ah_htcwm, us));
                ahp->ah_ctstimeout = us;
                return AH_TRUE;
        }
}

u_int
ar5416GetCTSTimeout(struct ath_hal *ah)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        u_int clks = MS(OS_REG_READ(ah, AR_TIME_OUT), AR_TIME_OUT_CTS);
        return ath_hal_mac_usec(ah, &ahp->ah_htcwm, clks);      /* convert from system clocks */
}

void
ar5416SetPCUConfig(struct ath_hal *ah)
{
        ar5416SetOperatingMode(ah, AH_PRIVATE(ah)->ah_opmode);
}

HAL_STATUS
ar5416GetCapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
        u_int32_t capability, u_int32_t *result)
{
        struct ath_hal_5416 *ahp = AH5416(ah);
        const HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;

        switch (type) {
        case HAL_CAP_CIPHER:            /* cipher handled in hardware */
                switch (capability) {
                case HAL_CIPHER_AES_CCM:
                case HAL_CIPHER_AES_OCB:
                case HAL_CIPHER_TKIP:
                case HAL_CIPHER_WEP:
                case HAL_CIPHER_MIC:
                case HAL_CIPHER_CLR:
                        return HAL_OK;
                default:
                        return HAL_ENOTSUPP;
                }
        case HAL_CAP_TKIP_MIC:          /* handle TKIP MIC in hardware */
                switch (capability) {
                case 0:                 /* hardware capability */
                        return HAL_OK;
                case 1:
                        return (ahp->ah_staId1Defaults &
                            AR_STA_ID1_CRPT_MIC_ENABLE) ?  HAL_OK : HAL_ENXIO;
                }
        case HAL_CAP_TKIP_SPLIT:        /* hardware TKIP uses split keys */
                /* XXX check rev when new parts are available */
                return HAL_OK;
        case HAL_CAP_WME_TKIPMIC:   /* hardware can do TKIP MIC when WMM is turned on */
                return HAL_OK;
        case HAL_CAP_PHYCOUNTERS:       /* hardware PHY error counters */
                return ahp->ah_hasHwPhyCounters ? HAL_OK : HAL_ENXIO;
        case HAL_CAP_DIVERSITY:         /* hardware supports fast diversity */
                switch (capability) {
                case 0:                 /* hardware capability */
                        return HAL_OK;
                case 1:                 /* current setting */
                        return (OS_REG_READ(ah, AR_PHY_CCK_DETECT) &
                                AR_PHY_CCK_DETECT_BB_ENABLE_ANT_FAST_DIV) ?
                                HAL_OK : HAL_ENXIO;
                }
                return HAL_EINVAL;
        case HAL_CAP_TPC:
                switch (capability) {
                case 0:                 /* hardware capability */
                        return HAL_OK;
                case 1:
                        return ahp->ah_tpcEnabled ? HAL_OK : HAL_ENXIO;
                }
                return HAL_OK;
        case HAL_CAP_PHYDIAG:           /* radar pulse detection capability */
                return HAL_OK;
        case HAL_CAP_MCAST_KEYSRCH:     /* multicast frame keycache search */
                switch (capability) {
                case 0:                 /* hardware capability */
                        return HAL_OK;
                case 1:
                        return (ahp->ah_staId1Defaults &
                            AR_STA_ID1_MCAST_KSRCH) ? HAL_OK : HAL_ENXIO;
                }
                return HAL_EINVAL;
        case HAL_CAP_TSF_ADJUST:        /* hardware has beacon tsf adjust */
                switch (capability) {
                case 0:                 /* hardware capability */
                        return pCap->halTsfAddSupport ? HAL_OK : HAL_ENOTSUPP;
                case 1:
                        return (ahp->ah_miscMode & AR_PCU_TX_ADD_TSF) ?
                                HAL_OK : HAL_ENXIO;
                }
                return HAL_EINVAL;
        default:
                return ath_hal_getcapability(ah, type, capability, result);
        }
}

HAL_BOOL
ar5416SetCapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
        u_int32_t capability, u_int32_t setting, HAL_STATUS *status)
{
        struct ath_hal_5416 *ahp = AH5416(ah);
        const HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;
        u_int32_t v;

        switch (type) {
        case HAL_CAP_TKIP_MIC:          /* handle TKIP MIC in hardware */
                if (setting)
                        ahp->ah_staId1Defaults |= AR_STA_ID1_CRPT_MIC_ENABLE;
                else
                        ahp->ah_staId1Defaults &= ~AR_STA_ID1_CRPT_MIC_ENABLE;
                return AH_TRUE;
        case HAL_CAP_DIVERSITY:
                v = OS_REG_READ(ah, AR_PHY_CCK_DETECT);
                if (setting)
                        v |= AR_PHY_CCK_DETECT_BB_ENABLE_ANT_FAST_DIV;
                else
                        v &= ~AR_PHY_CCK_DETECT_BB_ENABLE_ANT_FAST_DIV;
                OS_REG_WRITE(ah, AR_PHY_CCK_DETECT, v);
                return AH_TRUE;
        case HAL_CAP_DIAG:              /* hardware diagnostic support */
                /*
                 * NB: could split this up into virtual capabilities,
                 *     (e.g. 1 => ACK, 2 => CTS, etc.) but it hardly
                 *     seems worth the additional complexity.
                 */
#ifdef AH_DEBUG
                AH_PRIVATE(ah)->ah_diagreg = setting;
#else
                AH_PRIVATE(ah)->ah_diagreg = setting & 0x6;     /* ACK+CTS */
#endif
                OS_REG_WRITE(ah, AR_DIAG_SW, AH_PRIVATE(ah)->ah_diagreg);
                return AH_TRUE;
        case HAL_CAP_TPC:
                ahp->ah_tpcEnabled = (setting != 0);
                return AH_TRUE;
        case HAL_CAP_MCAST_KEYSRCH:     /* multicast frame keycache search */
                if (setting)
                        ahp->ah_staId1Defaults |= AR_STA_ID1_MCAST_KSRCH;
                else
                        ahp->ah_staId1Defaults &= ~AR_STA_ID1_MCAST_KSRCH;
                return AH_TRUE;
        case HAL_CAP_TSF_ADJUST:        /* hardware has beacon tsf adjust */
                if (pCap->halTsfAddSupport) {
                        if (setting)
                                ahp->ah_miscMode |= AR_PCU_TX_ADD_TSF;
                        else
                                ahp->ah_miscMode &= ~AR_PCU_TX_ADD_TSF;
                        return AH_TRUE;
                }
                /* fall thru... */
        default:
                return ath_hal_setcapability(ah, type, capability,
                                setting, status);
        }
}

HAL_BOOL
ar5416GetDiagState(struct ath_hal *ah, int request,
        const void *args, u_int32_t argsize,
        void **result, u_int32_t *resultsize)
{
        struct ath_hal_5416 *ahp = AH5416(ah);

        (void) ahp;
        if (ath_hal_getdiagstate(ah, request, args, argsize, result, resultsize))
                return AH_TRUE;
#ifdef AH_PRIVATE_DIAG
        switch (request) {
#if 0   // XXX - TODO
        const EEPROM_POWER_EXPN_2133 *pe;
#endif
        case HAL_DIAG_EEPROM:
                *result = &ahp->ah_eeprom;
                *resultsize = sizeof(HAL_EEPROM);
                return AH_TRUE;

#if 0   // XXX - TODO
        case HAL_DIAG_EEPROM_EXP_11A:
        case HAL_DIAG_EEPROM_EXP_11B:
        case HAL_DIAG_EEPROM_EXP_11G:
                pe = &ahp->ah_modePowerArray2133[
                        request - HAL_DIAG_EEPROM_EXP_11A];
                *result = pe->pChannels;
                *resultsize = (*result == AH_NULL) ? 0 :
                        roundup(sizeof(u_int16_t) * pe->numChannels,
                                sizeof(u_int32_t)) +
                        sizeof(EXPN_DATA_PER_CHANNEL_2133) * pe->numChannels;
                return AH_TRUE;
#endif
        case HAL_DIAG_RFGAIN:
                *result = &ahp->ah_gainValues;
                *resultsize = sizeof(GAIN_VALUES);
                return AH_TRUE;
        case HAL_DIAG_RFGAIN_CURSTEP:
                *result = (void *) ahp->ah_gainValues.currStep;
                *resultsize = (*result == AH_NULL) ?
                        0 : sizeof(GAIN_OPTIMIZATION_STEP);
                return AH_TRUE;
#if 0   // XXX - TODO
        case HAL_DIAG_PCDAC:
                *result = ahp->ah_pcdacTable;
                *resultsize = ahp->ah_pcdacTableSize;
                return AH_TRUE;
#endif
        case HAL_DIAG_TXRATES:
                *result = &ahp->ah_ratesArray[0];
                *resultsize = sizeof(ahp->ah_ratesArray);
                return AH_TRUE;
        case HAL_DIAG_ANI_CURRENT:
                *result = ar5416AniGetCurrentState(ah);
                *resultsize = (*result == AH_NULL) ?
                        0 : sizeof(struct ar5416AniState);
                return AH_TRUE;
        case HAL_DIAG_ANI_STATS:
                *result = ar5416AniGetCurrentStats(ah);
                *resultsize = (*result == AH_NULL) ?
                        0 : sizeof(struct ar5416Stats);
                return AH_TRUE;
        case HAL_DIAG_ANI_CMD:
                if (argsize != 2*sizeof(u_int32_t))
                        return AH_FALSE;
                ar5416AniControl(ah, ((const u_int32_t *)args)[0],
                        ((const u_int32_t *)args)[1]);
                return AH_TRUE;
        case HAL_DIAG_TXCONT:
        AR5416_CONTTXMODE(ah, (struct ath_desc *)args, argsize );
                return AH_TRUE;
        }
#endif /* AH_PRIVATE_DIAG */
        return AH_FALSE;
}

void
ar5416DmaRegDump(struct ath_hal *ah)
{
#define NUM_DMA_DEBUG_REGS  8
#define NUM_QUEUES          10

    u_int32_t val[NUM_DMA_DEBUG_REGS];
    int       qcuOffset = 0, dcuOffset = 0;
    u_int32_t *qcuBase  = &val[0], *dcuBase = &val[4];
    int       i;

    ath_hal_printf(ah, "Raw DMA Debug values:\n");
    for (i = 0; i < NUM_DMA_DEBUG_REGS; i++) {
        if (i % 4 == 0) {
            ath_hal_printf(ah, "\n");
        }

        val[i] = OS_REG_READ(ah, AR_DMADBG_0 + (i * sizeof(u_int32_t)));
        ath_hal_printf(ah, "%d: %08x ", i, val[i]);
    }

    ath_hal_printf(ah, "\n\n");
    ath_hal_printf(ah, "Num QCU: chain_st fsp_ok fsp_st DCU: chain_st\n");

    for (i = 0; i < NUM_QUEUES; i++, qcuOffset += 4, dcuOffset += 5) {
        if (i == 8) {
            /* only 8 QCU entries in val[0] */
            qcuOffset = 0;
            qcuBase++;
        }

        if (i == 6) {
            /* only 6 DCU entries in val[4] */
            dcuOffset = 0;
            dcuBase++;
        }
        
        ath_hal_printf(ah, "%2d          %2x      %1x     %2x           %2x\n",
                  i,
                  (*qcuBase & (0x7 << qcuOffset)) >> qcuOffset,
                  (*qcuBase & (0x8 << qcuOffset)) >> (qcuOffset + 3),
                  val[2] & (0x7 << (i * 3)) >> (i * 3),
                  (*dcuBase & (0x1f << dcuOffset)) >> dcuOffset);
    }

    ath_hal_printf(ah, "\n");
    ath_hal_printf(ah, "qcu_stitch state:   %2x    qcu_fetch state:        %2x\n",
              (val[3] & 0x003c0000) >> 18, (val[3] & 0x03c00000) >> 22);
    ath_hal_printf(ah, "qcu_complete state: %2x    dcu_complete state:     %2x\n",
              (val[3] & 0x1c000000) >> 26, (val[6] & 0x3));
    ath_hal_printf(ah, "dcu_arb state:      %2x    dcu_fp state:           %2x\n",
             (val[5] & 0x06000000) >> 25, (val[5] & 0x38000000) >> 27);
    ath_hal_printf(ah, "chan_idle_dur:     %3d    chan_idle_dur_valid:     %1d\n",
              (val[6] & 0x000003fc) >> 2, (val[6] & 0x00000400) >> 10);
    ath_hal_printf(ah, "txfifo_valid_0:      %1d    txfifo_valid_1:          %1d\n",
              (val[6] & 0x00000800) >> 11, (val[6] & 0x00001000) >> 12);
    ath_hal_printf(ah, "txfifo_dcu_num_0:   %2d    txfifo_dcu_num_1:       %2d\n",
              (val[6] & 0x0001e000) >> 13, (val[6] & 0x001e0000) >> 17);

}

/*
 * Return the busy for rx_frame, rx_clear, and tx_frame
 */
u_int32_t
ar5416GetCycleCounts(struct ath_hal *ah, u_int32_t *rxc_pcnt, u_int32_t *rxf_pcnt, u_int32_t *txf_pcnt)
{
    static u_int32_t cycles = 0, rx_clear = 0, rx_frame = 0, tx_frame= 0;
    u_int32_t good = 1;

    u_int32_t rc = OS_REG_READ(ah, AR_RCCNT);
    u_int32_t rf = OS_REG_READ(ah, AR_RFCNT);
    u_int32_t tf = OS_REG_READ(ah, AR_TFCNT);
    u_int32_t cc = OS_REG_READ(ah, AR_CCCNT); /* read cycles last */

    if (cycles == 0 || cycles > cc) {
        /*
         * Cycle counter wrap (or initial call); it's not possible
         * to accurately calculate a value because the registers
         * right shift rather than wrap--so punt and return 0.
         */
        HALDEBUG(ah, "%s: cycle counter wrap. ExtBusy = 0\n", __func__);
        good = 0;
    } else {
        u_int32_t cc_d = cc - cycles;
        u_int32_t rc_d = rc - rx_clear;
        u_int32_t rf_d = rf - rx_frame;
        u_int32_t tf_d = tf - tx_frame;

        if (cc_d != 0) {
            *rxc_pcnt = rc_d * 100 / cc_d;
            *rxf_pcnt = rf_d * 100 / cc_d;
            *txf_pcnt = tf_d * 100 / cc_d;
        } else {
            good = 0;
        }

#if 0
        HALDEBUG(ah, "%s: cc_d 0x%x, rc_d 0x%x, "
             "rf_d 0x%x, tf_d 0x%x\n",
              __func__, cc_d, rc_d, rf_d, tf_d);
#endif
    }

    cycles = cc;
    rx_frame = rf;
    rx_clear = rc;
    tx_frame = tf;
    
    return good;
}

/*
 * Return approximation of extension channel busy over an time interval
 * 0% (clear) -> 100% (busy)
 *
 */
u_int32_t
ar5416Get11nExtBusy(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    u_int32_t busy; /* percentage */
    u_int32_t cycleCount, ctlBusy, extBusy;

    ctlBusy = OS_REG_READ(ah, AR_RCCNT);
    extBusy = OS_REG_READ(ah, AR_EXTRCCNT);
    cycleCount = OS_REG_READ(ah, AR_CCCNT);

    if ((ahp->ah_cycleCount == 0) || (ahp->ah_cycleCount > cycleCount) ||
	(ahp->ah_ctlBusy > ctlBusy) ||(ahp->ah_extBusy > extBusy)) {
        /*
         * Cycle counter wrap (or initial call); it's not possible
         * to accurately calculate a value because the registers
         * right shift rather than wrap--so punt and return 0.
         */
        busy = 0;
        HALDEBUG(ah, "%s: cycle counter wrap. ExtBusy = 0\n", __func__);

    } else {
        u_int32_t cycleDelta = cycleCount - ahp->ah_cycleCount;
        u_int32_t ctlBusyDelta = ctlBusy - ahp->ah_ctlBusy;
        u_int32_t extBusyDelta = extBusy - ahp->ah_extBusy;
        u_int32_t ctlClearDelta = 0;

        /* Compute control channel rxclear.
         * The cycle delta may be less than the control channel delta.
         * This could be solved by freezing the timers (or an atomic read,
         * if one was available). Checking for the condition should be
         * sufficient.
         */
        if (cycleDelta > ctlBusyDelta) {
            ctlClearDelta = cycleDelta - ctlBusyDelta;
        }

        /* Compute ratio of extension channel busy to control channel clear
         * as an approximation to extension channel cleanliness.
         *
         * According to the hardware folks, ext rxclear is undefined
         * if the ctrl rxclear is de-asserted (i.e. busy)
         */
        if (ctlClearDelta) {
            busy = (extBusyDelta * 100) / ctlClearDelta;
        } else {
            busy = 0;
        }
        if (busy > 100) {
            busy = 100;
        }
#if 0
        HALDEBUG(ah, "%s: cycleDelta 0x%x, ctlBusyDelta 0x%x, "
             "extBusyDelta 0x%x, ctlClearDelta 0x%x, "
             "busy %d\n",
              __func__, cycleDelta, ctlBusyDelta, extBusyDelta, ctlClearDelta, busy);
#endif
    }

    ahp->ah_cycleCount = cycleCount;
    ahp->ah_ctlBusy = ctlBusy;
    ahp->ah_extBusy = extBusy;

    return busy;
}

/*
 * Configure 20/40 operation
 *
 * 20/40 = joint rx clear (control and extension)
 * 20    = rx clear (control)
 *
 * - NOTE: must stop MAC (tx) and requeue 40 MHz packets as 20 MHz when changing
 *         from 20/40 => 20 only
 */
void
ar5416Set11nMac2040(struct ath_hal *ah, HAL_HT_MACMODE mode)
{
    u_int32_t macmode;

    /* Configure MAC for 20/40 operation */
    if (mode == HAL_HT_MACMODE_2040 && !ath_hal_cwmIgnoreExtCCA) {
        macmode = AR_2040_JOINED_RX_CLEAR;
    } else {
        macmode = 0;
    }
    OS_REG_WRITE(ah, AR_2040_MODE, macmode);
}

/*
 * Get Rx clear (control/extension channel)
 *
 * Returns active low (busy) for ctrl/ext channel
 * Owl 2.0
 */
HAL_HT_RXCLEAR
ar5416Get11nRxClear(struct ath_hal *ah)
{
    HAL_HT_RXCLEAR rxclear = 0;
    u_int32_t val;

    val = OS_REG_READ(ah, AR_DIAG_SW);

    /* control channel */
    if (val & AR_DIAG_RX_CLEAR_CTL_LOW) {
        rxclear |= HAL_RX_CLEAR_CTL_LOW;
    }
    /* extension channel */
    if (val & AR_DIAG_RX_CLEAR_CTL_LOW) {
        rxclear |= HAL_RX_CLEAR_EXT_LOW;
    }
    return rxclear;
}

/*
 * Set Rx clear (control/extension channel)
 *
 * Useful for forcing the channel to appear busy for
 * debugging/diagnostics
 * Owl 2.0
 */
void
ar5416Set11nRxClear(struct ath_hal *ah, HAL_HT_RXCLEAR rxclear)
{
    /* control channel */
    if (rxclear & HAL_RX_CLEAR_CTL_LOW) {
        OS_REG_SET_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_CLEAR_CTL_LOW);
    } else {
        OS_REG_CLR_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_CLEAR_CTL_LOW);
    }
    /* extension channel */
    if (rxclear & HAL_RX_CLEAR_EXT_LOW) {
        OS_REG_SET_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_CLEAR_EXT_LOW);
    } else {
        OS_REG_CLR_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_CLEAR_EXT_LOW);
    }
}

#ifdef AR5416_EMULATION
/* MAC tracing (emulation only)
 * - stop tracing (ar5416StopMacTrace)
 *   and then use "dumpregs -l" to display MAC trace buffer
 */
void
ar5416InitMacTrace(struct ath_hal *ah)
{
    OS_REG_WRITE(ah, AR_MAC_PCU_LOGIC_ANALYZER_32L, 0xFFFFFFFF);
    OS_REG_WRITE(ah, AR_MAC_PCU_LOGIC_ANALYZER_16U, 0xFFFF);
}

void
ar5416StopMacTrace(struct ath_hal *ah)
{
    OS_REG_CLR_BIT(ah, AR_MAC_PCU_LOGIC_ANALYZER, 
		   (AR_MAC_PCU_LOGIC_ANALYZER_CTL | AR_MAC_PCU_LOGIC_ANALYZER_QCU_SEL));
    OS_REG_SET_BIT(ah, AR_MAC_PCU_LOGIC_ANALYZER, AR_MAC_PCU_LOGIC_ANALYZER_ENABLE | AR_MAC_PCU_LOGIC_ANALYZER_HOLD |
                        (1 << AR_MAC_PCU_LOGIC_ANALYZER_QCU_SEL_S));
}
#endif //AR5416_EMULATION

void
ar5416GetDescInfo(struct ath_hal *ah, HAL_DESC_INFO *desc_info)
{
    desc_info->txctl_numwords = TXCTL_NUMWORDS(ah);
    desc_info->txctl_offset = TXCTL_OFFSET(ah);
    desc_info->txstatus_numwords = TXSTATUS_NUMWORDS(ah);
    desc_info->txstatus_offset = TXSTATUS_OFFSET(ah);

    desc_info->rxctl_numwords = RXCTL_NUMWORDS(ah);
    desc_info->rxctl_offset = RXCTL_OFFSET(ah);
    desc_info->rxstatus_numwords = RXSTATUS_NUMWORDS(ah);
    desc_info->rxstatus_offset = RXSTATUS_OFFSET(ah);
}

#ifdef ATH_FORCE_PPM
/*
 * HAL support code for force ppm tracking workaround.
 */

#define PHY_ADDR004     (0x9800 +   4 * 4)
#define PHY_ADDR119     (0x9800 + 119 * 4)
#define PHY_ADDR317     (0x9800 + 317 * 4)
#define PHY_ADDR318     (0x9800 + 318 * 4)
#define PHY_ADDR319     (0x9800 + 319 * 4)


u_int32_t
ar5416PpmGetRssiDump(struct ath_hal *ah)
{
    u_int32_t retval;
    u_int32_t off1;
    u_int32_t off2;

    if (OS_REG_READ(ah, AR_PHY_ANALOG_SWAP) & AR_PHY_SWAP_ALT_CHAIN) {
        off1 = 0x2000;
        off2 = 0x1000;
    } else {
        off1 = 0x1000;
        off2 = 0x2000;
    }

    retval = ((0xff & OS_REG_READ(ah, PHY_ADDR319       )) << 0) |
             ((0xff & OS_REG_READ(ah, PHY_ADDR319 + off1)) << 8) |
             ((0xff & OS_REG_READ(ah, PHY_ADDR319 + off2)) << 16);

    return retval;
}

u_int32_t
ar5416PpmForce(struct ath_hal *ah)
{
    u_int32_t data_fine;
    u_int32_t data4;
    u_int32_t off1;
    u_int32_t off2;

    if (OS_REG_READ(ah, AR_PHY_ANALOG_SWAP) & AR_PHY_SWAP_ALT_CHAIN) {
        off1 = 0x2000;
        off2 = 0x1000;
    } else {
        off1 = 0x1000;
        off2 = 0x2000;
    }

    data_fine = 0xfff & OS_REG_READ(ah, PHY_ADDR317);

    /* write value */
    data4 = OS_REG_READ(ah, PHY_ADDR004) & ~0x1fff;
    OS_REG_WRITE(ah, PHY_ADDR004, data4 | data_fine | 0x1000);

    return data_fine;
 }

void
ar5416PpmUnForce(struct ath_hal *ah)
{
    u_int32_t data4;

    data4 = OS_REG_READ(ah, PHY_ADDR004) & ~0x1000;
    OS_REG_WRITE(ah, PHY_ADDR004, data4);
}

u_int32_t 
ar5416PpmArmTrigger(struct ath_hal *ah)
{
    u_int32_t val;
    u_int32_t ret;

    val = OS_REG_READ(ah, PHY_ADDR119);
    ret = OS_REG_READ(ah, AR_TSF_L32);
    OS_REG_WRITE(ah, PHY_ADDR119, val | 1);

    /* return low word of TSF at arm time */
    return ret;
}

int
ar5416PpmGetTrigger(struct ath_hal *ah)
{
    if (OS_REG_READ(ah, PHY_ADDR119) & 1) {
        /* has not triggered yet, return false */
        return 0;
    }

    /* else triggered, return true */
    return 1;
}

// DEBUG
u_int32_t
ar5416PpmGetReg(struct ath_hal *ah, int reg)
{
    return (OS_REG_READ(ah, reg));
}
#endif /* ATH_FORCE_PPM */

#endif /* AH_SUPPORT_AR5416 */
