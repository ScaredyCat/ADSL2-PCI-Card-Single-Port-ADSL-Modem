/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5212/ar5212_misc.c#4 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"
#ifdef AH_DEBUG
#include "ah_desc.h"			/* NB: for HAL_PHYERR* */
#endif

#include "ar5212/ar5212.h"
#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212phy.h"
#include "ar5212/ar5212desc.h"
#ifdef AH_SUPPORT_AR5311
#include "ar5212/ar5311reg.h"
#endif

#define	AR_NUM_GPIO	6		/* 6 GPIO pins */
#define	AR_GPIOD_MASK	0x0000002F	/* GPIO data reg r/w mask */

#define	ANT_SWITCH_TABLE1	AR_PHY(88)
#define	ANT_SWITCH_TABLE2	AR_PHY(89)

extern void ar5212SetRateDurationTable(struct ath_hal *, HAL_CHANNEL *);

void
ar5212GetMacAddress(struct ath_hal *ah, u_int8_t *mac)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	OS_MEMCPY(mac, ahp->ah_macaddr, IEEE80211_ADDR_LEN);
}

HAL_BOOL
ar5212SetMacAddress(struct ath_hal *ah, const u_int8_t *mac)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	OS_MEMCPY(ahp->ah_macaddr, mac, IEEE80211_ADDR_LEN);
	return AH_TRUE;
}

void
ar5212GetBssIdMask(struct ath_hal *ah, u_int8_t *mask)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	OS_MEMCPY(mask, ahp->ah_bssidmask, IEEE80211_ADDR_LEN);
}

HAL_BOOL
ar5212SetBssIdMask(struct ath_hal *ah, const u_int8_t *mask)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

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
ar5212SetRegulatoryDomain(struct ath_hal *ah,
	u_int16_t regDomain, HAL_STATUS *status)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_STATUS ecode;

	if (AH_PRIVATE(ah)->ah_currentRD == regDomain) {
		ecode = HAL_EINVAL;
		goto bad;
	}
	if (ahp->ah_eeprotect & AR_EEPROM_PROTECT_WP_128_191) {
		ecode = HAL_EEWRITE;
		goto bad;
	}
#ifdef AH_SUPPORT_WRITE_REGDOMAIN
	if (ath_hal_eepromWrite(ah, AR_EEPROM_REG_DOMAIN, regDomain)) {
		HALDEBUG(ah, "%s: set regulatory domain to %u (0x%x)\n",
			__func__, regDomain, regDomain);
		AH_PRIVATE(ah)->ah_currentRD = regDomain;
		return AH_TRUE;
	}
#endif
	ecode = HAL_EIO;
bad:
	if (status)
		*status = ecode;
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
ar5212GetWirelessModes(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	u_int mode = 0;

	if (ahp->ah_Amode) {
		mode = HAL_MODE_11A;
		if (!ahp->ah_turbo5Disable)
			mode |= HAL_MODE_TURBO | HAL_MODE_108A;
	}
	if (ahp->ah_Bmode)
		mode |= HAL_MODE_11B;
	if (ahp->ah_Gmode &&
	    AH_PRIVATE(ah)->ah_subvendorid != AR_SUBVENDOR_ID_NOG) {
		mode |= HAL_MODE_11G;
		if (!ahp->ah_turbo2Disable) 
			mode |= HAL_MODE_108G;
	}
	return mode;
}

/*
 * Accessor to get rfkill info from private EEPROM structures
 */
HAL_BOOL
ar5212GetRfKill(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	if (ahp->ah_rfKill) {
		u_int16_t rfsilent;

		if (!ath_hal_eepromRead(ah, AR_EEPROM_RFSILENT, &rfsilent))
			return AH_FALSE;
		ahp->ah_gpioSelect = MS(rfsilent, AR_EEPROM_RFSILENT_GPIO_SEL);
		ahp->ah_polarity = MS(rfsilent, AR_EEPROM_RFSILENT_POLARITY);
		ahp->ah_eepEnabled = AH_TRUE;
	} else {
		ahp->ah_gpioSelect = 0;
		ahp->ah_polarity = 0;
		ahp->ah_eepEnabled = AH_FALSE;
	}
	return (ahp->ah_rfKill != 0);
}

/*
 * Set the interrupt and GPIO values so the ISR can disable RF
 * on a switch signal.  Assumes GPIO port and interrupt polarity
 * are set prior to call.
 */
void
ar5212EnableRfKill(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	/*
	 * Configure the desired GPIO port for input
	 * and enable baseband rf silence.
	 */
	ath_hal_gpioCfgInput(ah, ahp->ah_gpioSelect);
	OS_REG_WRITE(ah, AR_PHY(0), OS_REG_READ(ah, AR_PHY(0)) | 0x00002000);
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
ar5212SetLedState(struct ath_hal *ah, HAL_LED_STATE state)
{
	static const u_int32_t ledbits[8] = {
		AR_PCICFG_LEDCTL_NONE|AR_PCICFG_LEDMODE_PROP, /* HAL_LED_INIT */
		AR_PCICFG_LEDCTL_PEND|AR_PCICFG_LEDMODE_PROP, /* HAL_LED_SCAN */
		AR_PCICFG_LEDCTL_PEND|AR_PCICFG_LEDMODE_PROP, /* HAL_LED_AUTH */
		AR_PCICFG_LEDCTL_ASSOC|AR_PCICFG_LEDMODE_PROP,/* HAL_LED_ASSOC*/
		AR_PCICFG_LEDCTL_ASSOC|AR_PCICFG_LEDMODE_PROP,/* HAL_LED_RUN */
		AR_PCICFG_LEDCTL_NONE|AR_PCICFG_LEDMODE_RAND,
		AR_PCICFG_LEDCTL_NONE|AR_PCICFG_LEDMODE_RAND,
		AR_PCICFG_LEDCTL_NONE|AR_PCICFG_LEDMODE_RAND,
	};
	OS_REG_WRITE(ah, AR_PCICFG,
		(OS_REG_READ(ah, AR_PCICFG) &~
			(AR_PCICFG_LEDCTL | AR_PCICFG_LEDMODE))
		| ledbits[state & 0x7]
	);
}

/*
 * Change association related fields programmed into the hardware.
 * Writing a valid BSSID to the hardware effectively enables the hardware
 * to synchronize its TSF to the correct beacons and receive frames coming
 * from that BSSID. It is called by the SME JOIN operation.
 */
void
ar5212WriteAssocid(struct ath_hal *ah, const u_int8_t *bssid, u_int16_t assocId)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

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
ar5212GetTsf64(struct ath_hal *ah)
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
ar5212GetTsf32(struct ath_hal *ah)
{
	return OS_REG_READ(ah, AR_TSF_L32);
}

/*
 * Reset the current hardware tsf for stamlme.
 */
void
ar5212ResetTsf(struct ath_hal *ah)
{

	u_int32_t val = OS_REG_READ(ah, AR_BEACON);

	OS_REG_WRITE(ah, AR_BEACON, val | AR_BEACON_RESET_TSF);
	/*
	 * workaround for hw bug! when resetting the TSF, write twice to the
	 * corresponding register; each write to the RESET_TSF bit toggles
	 * the internal signal to cause a reset of the TSF - but if the signal
	 * is left high, it will reset the TSF on the next chip reset also!
	 * writing the bit an even number of times fixes this issue
	 */
	OS_REG_WRITE(ah, AR_BEACON, val | AR_BEACON_RESET_TSF);
}

/*
 * Set or clear hardware basic rate bit
 * Set hardware basic rate set if basic rate is found
 * and basic rate is equal or less than 2Mbps
 */
void
ar5212SetBasicRate(struct ath_hal *ah, HAL_RATE_SET *rs)
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
ar5212GetRandomSeed(struct ath_hal *ah)
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
ar5212DetectCardPresent(struct ath_hal *ah)
{
	u_int16_t macVersion, macRev;
	u_int32_t v;

	/*
	 * Read the Silicon Revision register and compare that
	 * to what we read at attach time.  If the same, we say
	 * a card/device is present.
	 */
	v = OS_REG_READ(ah, AR_SREV) & AR_SREV_ID;
	macVersion = v >> AR_SREV_ID_S;
	macRev = v & AR_SREV_REVISION;
	return (AH_PRIVATE(ah)->ah_macVersion == macVersion &&
		AH_PRIVATE(ah)->ah_macRev == macRev);
}

/*
 * Update MIB Counters
 */
void
ar5212UpdateMibCounters(struct ath_hal *ah, HAL_MIB_STATS* stats)
{
	stats->ackrcv_bad += OS_REG_READ(ah, AR_ACK_FAIL);
	stats->rts_bad	  += OS_REG_READ(ah, AR_RTS_FAIL);
	stats->fcs_bad	  += OS_REG_READ(ah, AR_FCS_FAIL);
	stats->rts_good	  += OS_REG_READ(ah, AR_RTS_OK);
	stats->beacons	  += OS_REG_READ(ah, AR_BEACON_CNT);
}

/*
 * Detect if the HW supports spreading a CCK signal on channel 14
 */
HAL_BOOL
ar5212IsJapanChannelSpreadSupported(struct ath_hal *ah)
{
	return AH_TRUE;
}

/*
 * Get the rssi of frame curently being received.
 */
u_int32_t
ar5212GetCurRssi(struct ath_hal *ah)
{
	return (OS_REG_READ(ah, AR_PHY_CURRENT_RSSI) & 0xff);
}

u_int
ar5212GetDefAntenna(struct ath_hal *ah)
{   
	return (OS_REG_READ(ah, AR_DEF_ANTENNA) & 0x7);
}   

void
ar5212SetDefAntenna(struct ath_hal *ah, u_int antenna)
{
	OS_REG_WRITE(ah, AR_DEF_ANTENNA, (antenna & 0x7));
}

HAL_BOOL
ar5212SetAntennaSwitch(struct ath_hal *ah,
	HAL_ANT_SETTING settings, HAL_CHANNEL *chan)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	u_int32_t antSwitchA, antSwitchB;
	int ix;

	switch (chan->channelFlags & CHANNEL_ALL_NOTURBO) {
	case CHANNEL_A:		ix = 0; break;
	case CHANNEL_B:		ix = 1; break;
	case CHANNEL_PUREG:	ix = 2; break;
	default:
		HALDEBUG(ah, "%s: invalid channel flags 0x%x\n",
			__func__, chan->channelFlags);
		return AH_FALSE;
	}

	antSwitchA =  ahp->ah_antennaControl[1][ix]
		   | (ahp->ah_antennaControl[2][ix] << 6)
		   | (ahp->ah_antennaControl[3][ix] << 12) 
		   | (ahp->ah_antennaControl[4][ix] << 18)
		   | (ahp->ah_antennaControl[5][ix] << 24)
		   ;
	antSwitchB =  ahp->ah_antennaControl[6][ix]
		   | (ahp->ah_antennaControl[7][ix] << 6)
		   | (ahp->ah_antennaControl[8][ix] << 12)
		   | (ahp->ah_antennaControl[9][ix] << 18)
		   | (ahp->ah_antennaControl[10][ix] << 24)
		   ;
	/*
	 * For fixed antenna, give the same setting for both switch banks
	 */
	switch (settings) {
	case HAL_ANT_FIXED_A:
		antSwitchB = antSwitchA;
		break;
	case HAL_ANT_FIXED_B:
		antSwitchA = antSwitchB;
		break;
	case HAL_ANT_VARIABLE:
		break;
	default:
		HALDEBUG(ah, "%s: bad antenna setting %u\n",
			__func__, settings);
		return AH_FALSE;
	}
	ahp->ah_diversityControl = settings;

	OS_REG_WRITE(ah, ANT_SWITCH_TABLE1, antSwitchA);
	OS_REG_WRITE(ah, ANT_SWITCH_TABLE2, antSwitchB);

	return AH_TRUE;
}

HAL_BOOL
ar5212IsSleepAfterBeaconBroken(struct ath_hal *ah)
{
	return AH_TRUE;
}

HAL_BOOL
ar5212SetSlotTime(struct ath_hal *ah, u_int us)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	if (us < HAL_SLOT_TIME_9 || us > ath_hal_mac_usec(ah, 0xffff)) {
		HALDEBUG(ah, "%s: bad slot time %u\n", __func__, us);
		ahp->ah_slottime = (u_int) -1;	/* restore default handling */
		return AH_FALSE;
	} else {
		/* convert to system clocks */
		OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, ath_hal_mac_clks(ah, us));
		ahp->ah_slottime = us;
		return AH_TRUE;
	}
}

u_int
ar5212GetSlotTime(struct ath_hal *ah)
{
	u_int clks = OS_REG_READ(ah, AR_D_GBL_IFS_SLOT) & 0xffff;
	return ath_hal_mac_usec(ah, clks);	/* convert from system clocks */
}

HAL_BOOL
ar5212SetAckTimeout(struct ath_hal *ah, u_int us)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	if (us > ath_hal_mac_usec(ah, MS(0xffffffff, AR_TIME_OUT_ACK))) {
		HALDEBUG(ah, "%s: bad ack timeout %u\n", __func__, us);
		ahp->ah_acktimeout = (u_int) -1; /* restore default handling */
		return AH_FALSE;
	} else {
		/* convert to system clocks */
		OS_REG_RMW_FIELD(ah, AR_TIME_OUT,
			AR_TIME_OUT_ACK, ath_hal_mac_clks(ah, us));
		ahp->ah_acktimeout = us;
		return AH_TRUE;
	}
}

u_int
ar5212GetAckTimeout(struct ath_hal *ah)
{
	u_int clks = MS(OS_REG_READ(ah, AR_TIME_OUT), AR_TIME_OUT_ACK);
	return ath_hal_mac_usec(ah, clks);	/* convert from system clocks */
}

HAL_BOOL
ar5212SetCTSTimeout(struct ath_hal *ah, u_int us)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	if (us > ath_hal_mac_usec(ah, MS(0xffffffff, AR_TIME_OUT_CTS))) {
		HALDEBUG(ah, "%s: bad cts timeout %u\n", __func__, us);
		ahp->ah_ctstimeout = (u_int) -1; /* restore default handling */
		return AH_FALSE;
	} else {
		/* convert to system clocks */
		OS_REG_RMW_FIELD(ah, AR_TIME_OUT,
			AR_TIME_OUT_CTS, ath_hal_mac_clks(ah, us));
		ahp->ah_ctstimeout = us;
		return AH_TRUE;
	}
}

u_int
ar5212GetCTSTimeout(struct ath_hal *ah)
{
	u_int clks = MS(OS_REG_READ(ah, AR_TIME_OUT), AR_TIME_OUT_CTS);
	return ath_hal_mac_usec(ah, clks);	/* convert from system clocks */
}

/* Setup decompression for given key index */
HAL_BOOL
ar5212SetDecompMask(struct ath_hal *ah, u_int16_t keyidx, int en)
{
        if (keyidx > HAL_DECOMP_MASK_SIZE) {
                return HAL_EINVAL; 
        }

        OS_REG_WRITE(ah, AR_DCM_A, keyidx);
        OS_REG_WRITE(ah, AR_DCM_D, en ? AR_DCM_D_EN : 0);
        ah->ah_decompMask[keyidx] = en;

        return HAL_OK;
}

/* Setup coverage class */
void
ar5212SetCoverageClass(struct ath_hal *ah, u_int8_t coverageclass, int now)
{
	u_int32_t slot, timeout, eifs;
	u_int clkRate;

	AH_PRIVATE(ah)->ah_coverageClass = coverageclass;

	if (now) {

		if (AH_PRIVATE(ah)->ah_coverageClass == 0)
			return;

		/* Get clock rate */
		clkRate = ath_hal_mac_clks(ah, 1);

		/* Compute EIFS */
		if (IS_CHAN_HALF_RATE(AH_PRIVATE(ah)->ah_curchan)) {
			slot = IFS_SLOT_HALF_RATE + 
					(coverageclass * 3 * (clkRate >> 1));
			eifs = IFS_EIFS_HALF_RATE +
					(coverageclass * 6 * (clkRate >> 1));
		} else if (IS_CHAN_QUARTER_RATE(AH_PRIVATE(ah)->ah_curchan)) {
			slot = IFS_SLOT_QUARTER_RATE + 
				(coverageclass * 3 * (clkRate >> 2));
			eifs = IFS_EIFS_QUARTER_RATE +
				(coverageclass * 6 * (clkRate >> 2));
		} else { /* full rate */
			slot = IFS_SLOT_FULL_RATE + 
					(coverageclass * 3 * clkRate);
			eifs = IFS_EIFS_FULL_RATE + 
					(coverageclass * 6 * clkRate);
		}

		/* Add additional time for air propagation for ACK and CTS
		 * timeouts. This value is in core clocks.
  		 */
		timeout = (ACK_CTS_TIMEOUT_11A + 
			((coverageclass * 3) * clkRate)) & AR_TIME_OUT_ACK;
	
		/* Write the values back to registers: slot, eifs, ack/cts
		 * timeouts.
		 */
	
		OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, slot);
		OS_REG_WRITE(ah, AR_D_GBL_IFS_EIFS, eifs);
		OS_REG_WRITE(ah, AR_TIME_OUT, ((timeout << AR_TIME_OUT_CTS_S) 
							| timeout));
	}

	return;
}

void
ar5212SetPCUConfig(struct ath_hal *ah)
{
	ar5212SetOperatingMode(ah, AH_PRIVATE(ah)->ah_opmode);
}

/*
 * Return whether an external 32KHz crystal should be used
 * to reduce power consumption when sleeping.  We do so if
 * the crystal is present (obtained from EEPROM) and if we
 * are not running as an AP and are configured to use it.
 */
HAL_BOOL
ar5212Use32KHzclock(struct ath_hal *ah, HAL_OPMODE opmode)
{
	if (opmode != HAL_M_HOSTAP) {
		struct ath_hal_5212 *ahp = AH5212(ah);
		return ahp->ah_exist32kHzCrystal &&
		       (ahp->ah_enable32kHzClock == USE_32KHZ ||
		        ahp->ah_enable32kHzClock == AUTO_32KHZ);
	} else
		return AH_FALSE;
}

/*
 * If 32KHz clock exists, use it to lower power consumption during sleep
 *
 * Note: If clock is set to 32 KHz, delays on accessing certain
 *       baseband registers (27-31, 124-127) are required.
 */
void
ar5212SetupClock(struct ath_hal *ah, HAL_OPMODE opmode)
{
	if (ar5212Use32KHzclock(ah, opmode)) {
		/*
		 * Enable clocks to be turned OFF in BB during sleep
		 * and also enable turning OFF 32MHz/40MHz Refclk
		 * from A2.
		 */
		OS_REG_WRITE(ah, AR_PHY_SLEEP_CTR_CONTROL, 0x1f);
		OS_REG_WRITE(ah, AR_PHY_SLEEP_CTR_LIMIT,   0x0d);
		OS_REG_WRITE(ah, AR_PHY_SLEEP_SCAL,        0x0c);
		OS_REG_WRITE(ah, AR_PHY_M_SLEEP,           0x03);
		OS_REG_WRITE(ah, AR_PHY_REFCLKDLY,         0x05);
		OS_REG_WRITE(ah, AR_PHY_REFCLKPD, IS_5112(ah) ?  0x14 : 0x18);

		OS_REG_RMW_FIELD(ah, AR_USEC, AR_USEC_USEC32, 1);
		OS_REG_WRITE(ah, AR_TSF_PARM, 61);	/* 32 KHz TSF incr */

		/* # Set sleep clock rate to 32 KHz. */
		OS_REG_RMW_FIELD(ah, AR_PCICFG, AR_PCICFG_SCLK_SEL, 1);
		OS_REG_RMW_FIELD(ah, AR_PCICFG, AR_PCICFG_SCLK_RATE_IND, 0x3);
	} else {
		OS_REG_WRITE(ah, AR_PHY_SLEEP_CTR_CONTROL, 0x1f);
		OS_REG_WRITE(ah, AR_PHY_SLEEP_CTR_LIMIT,   0x7f);

		OS_REG_WRITE(ah, AR_PHY_SLEEP_SCAL,        0x0e);
		OS_REG_WRITE(ah, AR_PHY_M_SLEEP,           0x0c);
		OS_REG_WRITE(ah, AR_PHY_REFCLKDLY,         0xff);
		OS_REG_WRITE(ah, AR_PHY_REFCLKPD, IS_5112(ah) ?  0x14 : 0x18);
	}
}

/*
 * If 32KHz clock exists, turn it off and turn back on the 32Mhz
 */
void
ar5212RestoreClock(struct ath_hal *ah, HAL_OPMODE opmode)
{
	if (ar5212Use32KHzclock(ah, opmode)) {
		/* # Set sleep clock rate back to 32 MHz. */
		OS_REG_RMW_FIELD(ah, AR_PCICFG, AR_PCICFG_SCLK_RATE_IND, 0);
		OS_REG_RMW_FIELD(ah, AR_PCICFG, AR_PCICFG_SCLK_SEL, 0);

		OS_REG_WRITE(ah, AR_TSF_PARM, 1);	/* 32 MHz TSF incr */
		OS_REG_RMW_FIELD(ah, AR_USEC, AR_USEC_USEC32, 31);

		/*
		 * Restore BB registers to power-on defaults
		 */
		OS_REG_WRITE(ah, AR_PHY_SLEEP_CTR_CONTROL, 0x1f);
		OS_REG_WRITE(ah, AR_PHY_SLEEP_CTR_LIMIT,   0x7f);
		OS_REG_WRITE(ah, AR_PHY_SLEEP_SCAL,        0x0e);
		OS_REG_WRITE(ah, AR_PHY_M_SLEEP,           0x0c);
		OS_REG_WRITE(ah, AR_PHY_REFCLKDLY,         0xff);
		OS_REG_WRITE(ah, AR_PHY_REFCLKPD, IS_5112(ah) ?  0x14 : 0x18);
	}
}

HAL_STATUS
ar5212GetCapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
	u_int32_t capability, u_int32_t *result)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	const HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;
#define	IS_COBRA(ah) \
    (AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_COBRA)

	switch (type) {
	case HAL_CAP_CIPHER:		/* cipher handled in hardware */
		switch (capability) {
		case HAL_CIPHER_AES_CCM:
			/* CCM non-compliant upto rev 3 */
			return (((AH_PRIVATE(ah)->ah_macRev >= AR_SREV_CRETE) ||
				IS_COBRA(ah)) ?
				HAL_OK : HAL_ENOTSUPP);
		case HAL_CIPHER_AES_OCB:
		case HAL_CIPHER_TKIP:
		case HAL_CIPHER_WEP:
		case HAL_CIPHER_MIC:
		case HAL_CIPHER_CLR:
			return HAL_OK;
		default:
			return HAL_ENOTSUPP;
		}
	case HAL_CAP_TKIP_MIC:		/* handle TKIP MIC in hardware */
		switch (capability) {
		case 0:			/* hardware capability */
			return HAL_OK;
		case 1:
			return (ahp->ah_staId1Defaults &
			    AR_STA_ID1_CRPT_MIC_ENABLE) ?  HAL_OK : HAL_ENXIO;
		}
	case HAL_CAP_TKIP_SPLIT:	/* hardware TKIP uses split keys */
		/* XXX check rev when new parts are available */
		return HAL_OK;
	case HAL_CAP_WME_TKIPMIC:   /* hardware can do TKIP MIC when WMM is turned on */
		return ((AH_PRIVATE(ah)->ah_macVersion > AR_SREV_VERSION_VENICE) ||
				((AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_VENICE) &&
				 (AH_PRIVATE(ah)->ah_macRev >= 8))) ? HAL_OK : HAL_ENOTSUPP;
	case HAL_CAP_PHYCOUNTERS:	/* hardware PHY error counters */
		return ahp->ah_hasHwPhyCounters ? HAL_OK : HAL_ENXIO;
	case HAL_CAP_DIVERSITY:		/* hardware supports fast diversity */
		switch (capability) {
		case 0:			/* hardware capability */
			return HAL_OK;
		case 1:			/* current setting */
			return (OS_REG_READ(ah, AR_PHY_CCK_DETECT) &
				AR_PHY_CCK_DETECT_BB_ENABLE_ANT_FAST_DIV) ?
				HAL_OK : HAL_ENXIO;
		}
		return HAL_EINVAL;
	case HAL_CAP_TPC:
		switch (capability) {
		case 0:			/* hardware capability */
			return HAL_OK;
		case 1:
			return ahp->ah_tpcEnabled ? HAL_OK : HAL_ENXIO;
		}
		return HAL_OK;
	case HAL_CAP_PHYDIAG:		/* radar pulse detection capability */
		return HAL_OK;
	case HAL_CAP_MCAST_KEYSRCH:	/* multicast frame keycache search */
		switch (capability) {
		case 0:			/* hardware capability */
			return HAL_OK;
		case 1:
			return (ahp->ah_staId1Defaults &
			    AR_STA_ID1_MCAST_KSRCH) ? HAL_OK : HAL_ENXIO;
		}
		return HAL_EINVAL;
	case HAL_CAP_TSF_ADJUST:	/* hardware has beacon tsf adjust */
		switch (capability) {
		case 0:			/* hardware capability */
			return pCap->halTsfAddSupport ? HAL_OK : HAL_ENOTSUPP;
		case 1:
			return (ahp->ah_miscMode & AR_MISC_MODE_TX_ADD_TSF) ?
				HAL_OK : HAL_ENXIO;
		}
		return HAL_EINVAL;
	default:
		return ath_hal_getcapability(ah, type, capability, result);
	}
#undef IS_COBRA
}

HAL_BOOL
ar5212SetCapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
	u_int32_t capability, u_int32_t setting, HAL_STATUS *status)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	const HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;
	u_int32_t v;

	switch (type) {
	case HAL_CAP_TKIP_MIC:		/* handle TKIP MIC in hardware */
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
	case HAL_CAP_DIAG:		/* hardware diagnostic support */
		/*
		 * NB: could split this up into virtual capabilities,
		 *     (e.g. 1 => ACK, 2 => CTS, etc.) but it hardly
		 *     seems worth the additional complexity.
		 */
#ifdef AH_DEBUG
		AH_PRIVATE(ah)->ah_diagreg = setting;
#else
		AH_PRIVATE(ah)->ah_diagreg = setting & 0x6;	/* ACK+CTS */
#endif
		OS_REG_WRITE(ah, AR_DIAG_SW, AH_PRIVATE(ah)->ah_diagreg);
		return AH_TRUE;
	case HAL_CAP_TPC:
		ahp->ah_tpcEnabled = (setting != 0);
		return AH_TRUE;
	case HAL_CAP_MCAST_KEYSRCH:	/* multicast frame keycache search */
		if (setting)
			ahp->ah_staId1Defaults |= AR_STA_ID1_MCAST_KSRCH;
		else
			ahp->ah_staId1Defaults &= ~AR_STA_ID1_MCAST_KSRCH;
		return AH_TRUE;
	case HAL_CAP_TSF_ADJUST:	/* hardware has beacon tsf adjust */
		if (pCap->halTsfAddSupport) {
			if (setting)
				ahp->ah_miscMode |= AR_MISC_MODE_TX_ADD_TSF;
			else
				ahp->ah_miscMode &= ~AR_MISC_MODE_TX_ADD_TSF;
			return AH_TRUE;
		}
		/* fall thru... */
	default:
		return ath_hal_setcapability(ah, type, capability,
				setting, status);
	}
}

HAL_BOOL
ar5212GetDiagState(struct ath_hal *ah, int request,
	const void *args, u_int32_t argsize,
	void **result, u_int32_t *resultsize)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	(void) ahp;
	if (ath_hal_getdiagstate(ah, request, args, argsize, result, resultsize))
		return AH_TRUE;
#ifdef AH_PRIVATE_DIAG
	switch (request) {
	const EEPROM_POWER_EXPN_5112 *pe;

	case HAL_DIAG_EEPROM:
		*result = &ahp->ah_eeprom;
		*resultsize = sizeof(HAL_EEPROM);
		return AH_TRUE;
	case HAL_DIAG_EEPROM_EXP_11A:
	case HAL_DIAG_EEPROM_EXP_11B:
	case HAL_DIAG_EEPROM_EXP_11G:
		pe = &ahp->ah_modePowerArray5112[
			request - HAL_DIAG_EEPROM_EXP_11A];
		*result = pe->pChannels;
		*resultsize = (*result == AH_NULL) ? 0 :
			roundup(sizeof(u_int16_t) * pe->numChannels,
				sizeof(u_int32_t)) +
			sizeof(EXPN_DATA_PER_CHANNEL_5112) * pe->numChannels;
		return AH_TRUE;
	case HAL_DIAG_RFGAIN:
		*result = &ahp->ah_gainValues;
		*resultsize = sizeof(GAIN_VALUES);
		return AH_TRUE;
	case HAL_DIAG_RFGAIN_CURSTEP:
		*result = (void *) ahp->ah_gainValues.currStep;
		*resultsize = (*result == AH_NULL) ?
			0 : sizeof(GAIN_OPTIMIZATION_STEP);
		return AH_TRUE;
	case HAL_DIAG_PCDAC:
		*result = ahp->ah_pcdacTable;
		*resultsize = ahp->ah_pcdacTableSize;
		return AH_TRUE;
	case HAL_DIAG_TXRATES:
		*result = &ahp->ah_ratesArray[0];
		*resultsize = sizeof(ahp->ah_ratesArray);
		return AH_TRUE;
	case HAL_DIAG_ANI_CURRENT:
		*result = ar5212AniGetCurrentState(ah);
		*resultsize = (*result == AH_NULL) ?
			0 : sizeof(struct ar5212AniState);
		return AH_TRUE;
	case HAL_DIAG_ANI_STATS:
		*result = ar5212AniGetCurrentStats(ah);
		*resultsize = (*result == AH_NULL) ?
			0 : sizeof(struct ar5212Stats);
		return AH_TRUE;
	case HAL_DIAG_ANI_CMD:
		if (argsize != 2*sizeof(u_int32_t))
			return AH_FALSE;
		ar5212AniControl(ah, ((const u_int32_t *)args)[0],
			((const u_int32_t *)args)[1]);
		return AH_TRUE;
	case HAL_DIAG_TXCONT:
		ar5212_ContTxMode(ah,(struct ath_desc *)args,argsize );
		return AH_TRUE;
	}
#endif /* AH_PRIVATE_DIAG */
	return AH_FALSE;
}
void
ar5212GetDescInfo(struct ath_hal *ah, HAL_DESC_INFO *desc_info)
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

#endif /* AH_SUPPORT_AR5212 */
