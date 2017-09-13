/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5211/ar5211_misc.c#4 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5211

#include "ah.h"
#include "ah_internal.h"

#include "ar5211/ar5211.h"
#include "ar5211/ar5211reg.h"
#include "ar5211/ar5211phy.h"
#include "ar5211/ar5211desc.h"

#define	AR_NUM_GPIO	6		/* 6 GPIO bits */
#define	AR_GPIOD_MASK	0x2f		/* 6-bit mask */

#define	ANT_SWITCH_TABLE1	0x9960
#define	ANT_SWITCH_TABLE2	0x9964

void
ar5211GetMacAddress(struct ath_hal *ah, u_int8_t *mac)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	OS_MEMCPY(mac, ahp->ah_macaddr, IEEE80211_ADDR_LEN);
}

HAL_BOOL
ar5211SetMacAddress(struct ath_hal *ah, const u_int8_t *mac)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	OS_MEMCPY(ahp->ah_macaddr, mac, IEEE80211_ADDR_LEN);
	return AH_TRUE;
}

void
ar5211GetBssIdMask(struct ath_hal *ah, u_int8_t *mask)
{
	static const u_int8_t ones[IEEE80211_ADDR_LEN] =
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	OS_MEMCPY(mask, ones, IEEE80211_ADDR_LEN);
}

HAL_BOOL
ar5211SetBssIdMask(struct ath_hal *ah, const u_int8_t *mask)
{
	return AH_FALSE;
}

/*
 * Read 16 bits of data from the specified EEPROM offset.
 */
HAL_BOOL
ar5211EepromRead(struct ath_hal *ah, u_int off, u_int16_t *data)
{
	OS_REG_WRITE(ah, AR_EEPROM_ADDR, off);
	OS_REG_WRITE(ah, AR_EEPROM_CMD, AR_EEPROM_CMD_READ);

	if (!ath_hal_wait(ah, AR_EEPROM_STS,
	    AR_EEPROM_STS_READ_COMPLETE | AR_EEPROM_STS_READ_ERROR,
	    AR_EEPROM_STS_READ_COMPLETE)) {
		HALDEBUG(ah, "%s: read failed for entry 0x%x\n", __func__, off);
		return AH_FALSE;
	}
	*data = OS_REG_READ(ah, AR_EEPROM_DATA) & 0xffff;
	return AH_TRUE;
}

#ifdef AH_SUPPORT_WRITE_EEPROM
/*
 * Write 16 bits of data to the specified EEPROM offset.
 */
HAL_BOOL
ar5211EepromWrite(struct ath_hal *ah, u_int off, u_int16_t data)
{
	OS_REG_WRITE(ah, AR_EEPROM_ADDR, off);
	OS_REG_WRITE(ah, AR_EEPROM_DATA, data);
	OS_REG_WRITE(ah, AR_EEPROM_CMD, AR_EEPROM_CMD_WRITE);

	if (!ath_hal_wait(ah, AR_EEPROM_STS,
	    AR_EEPROM_STS_WRITE_COMPLETE | AR_EEPROM_STS_WRITE_ERROR,
	    AR_EEPROM_STS_WRITE_COMPLETE)) {
		HALDEBUG(ah, "%s: write failed for entry 0x%x, data 0x%x\n",
			__func__, off, data);
		return AH_FALSE;
	}
	return AH_TRUE;
}
#endif /* AH_SUPPORT_WRITE_EEPROM */

/*
 * Attempt to change the cards operating regulatory domain to the given value
 * Returns: A_EINVAL for an unsupported regulatory domain.
 *          A_HARDWARE for an unwritable EEPROM or bad EEPROM version
 */
HAL_BOOL
ar5211SetRegulatoryDomain(struct ath_hal *ah,
	u_int16_t regDomain, HAL_STATUS *status)
{
	struct ath_hal_5211 *ahp = AH5211(ah);
	HAL_STATUS ecode;

	if (AH_PRIVATE(ah)->ah_currentRD == regDomain) {
		ecode = HAL_EINVAL;
		goto bad;
	}
	/*
	 * Check if EEPROM is configured to allow this; must
	 * be a proper version and the protection bits must
	 * permit re-writing that segment of the EEPROM.
	 */
	if (ahp->ah_eeprotect & AR_EEPROM_PROTECT_WP_128_191) {
		ecode = HAL_EEWRITE;
		goto bad;
	}
#ifdef AH_SUPPORT_WRITE_REGDOMAIN
	if (ar5211EepromWrite(ah, AR_EEPROM_REG_DOMAIN, regDomain)) {
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
ar5211GetWirelessModes(struct ath_hal *ah)
{
	struct ath_hal_5211 *ahp = AH5211(ah);
	u_int mode = 0;

	if (ahp->ah_Amode) {
		mode = HAL_MODE_11A;
		if (!ahp->ah_turbo5Disable)
			mode |= HAL_MODE_TURBO | HAL_MODE_108A;
	}
	if (ahp->ah_Bmode)
		mode |= HAL_MODE_11B;
	return mode;
}

/*
 * Accessor to get rfkill from private EEPROM structure
 */
HAL_BOOL
ar5211GetRfKill(struct ath_hal *ah)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	if (ahp->ah_rfKill) {
		u_int16_t rfsilent;

		if (!ar5211EepromRead(ah, AR_EEPROM_RFSILENT, &rfsilent))
			return AH_FALSE;
		ahp->ah_gpioSelect = MS(rfsilent, AR_EEPROM_RFSILENT_GPIO_SEL);
		ahp->ah_polarity = MS(rfsilent, AR_EEPROM_RFSILENT_POLARITY);
		ahp->ah_eepEnabled = AH_TRUE;
	} else {
		ahp->ah_gpioSelect = ahp->ah_polarity = 0;
		ahp->ah_eepEnabled = AH_FALSE;
	}
	return (ahp->ah_rfKill != 0);
}

#if 0
HAL_BOOL
ar5211GetTurboDisable(struct ath_hal *ah)
{
	return (AH5211(ah)->ah_turboDisable != 0);
}
#endif

/*
 * Called if RfKill is supported (according to EEPROM).  Set the interrupt and
 * GPIO values so the ISR and can disable RF on a switch signal
 *
 * TODO - can this really be above the hal on the GPIO interface for
 * TODO - the client only?
 */
void
ar5211EnableRfKill(struct ath_hal *ah)
{
	struct ath_hal_5211 *ahp = AH5211(ah);
	u_int32_t val;

	/*
	 * If radio disable switch connection to GPIO bit 0 is enabled
	 * program GPIO interrupt.
	 * If rfkill bit on eeprom is 1, setupeeprommap routine has already
	 * verified that it is a later version of eeprom, it has a place for
	 * rfkill bit and it is set to 1, indicating that GPIO bit 0 hardware
	 * connection is present.
	 */
	val = ar5211GpioGet(ah, 0);
	ar5211GpioSetIntr(ah, 0, (val == 0));
	/*
	 * Configure the desired GPIO port for input
	 * and enable baseband rf silence.
	 */
	ar5211GpioCfgInput(ah, ahp->ah_gpioSelect);
#ifdef notdef
	A_REG_RMW(ah, PHY_BASE, 0x00002000, 0);
#else
	val = OS_REG_READ(ah, AR_PHY_BASE);
	OS_REG_WRITE(ah, AR_PHY_BASE, val &~ 0x2000);
#endif
	/*
	 * If radio disable switch connection to GPIO bit x is enabled
	 * program GPIO interrupt.
	 * If rfkill bit on eeprom is 1, setupeeprommap routine has already
	 * verified that it is a later version of eeprom, it has a place for
	 * rfkill bit and it is set to 1, indicating that GPIO bit x hardware
	 * connection is present.
	 */
	ahp->ah_gpioBit = ar5211GpioGet(ah, ahp->ah_gpioSelect);
	ar5211GpioSet(ah, ahp->ah_gpioSelect,
		(ahp->ah_gpioBit != ahp->ah_polarity));
}

/*
 * Configure GPIO Output lines
 */
HAL_BOOL
ar5211GpioCfgOutput(struct ath_hal *ah, u_int32_t gpio)
{
	u_int32_t reg;

	HALASSERT(gpio < AR_NUM_GPIO);

	reg =  OS_REG_READ(ah, AR_GPIOCR);
	reg &= ~(AR_GPIOCR_0_CR_A << (gpio * AR_GPIOCR_CR_SHIFT));
	reg |= AR_GPIOCR_0_CR_A << (gpio * AR_GPIOCR_CR_SHIFT);

	OS_REG_WRITE(ah, AR_GPIOCR, reg);
	return AH_TRUE;
}

/*
 * Configure GPIO Input lines
 */
HAL_BOOL
ar5211GpioCfgInput(struct ath_hal *ah, u_int32_t gpio)
{
	u_int32_t reg;

	HALASSERT(gpio < AR_NUM_GPIO);

	reg =  OS_REG_READ(ah, AR_GPIOCR);
	reg &= ~(AR_GPIOCR_0_CR_A << (gpio * AR_GPIOCR_CR_SHIFT));
	reg |= AR_GPIOCR_0_CR_N << (gpio * AR_GPIOCR_CR_SHIFT);

	OS_REG_WRITE(ah, AR_GPIOCR, reg);
	return AH_TRUE;
}

/*
 * Once configured for I/O - set output lines
 */
HAL_BOOL
ar5211GpioSet(struct ath_hal *ah, u_int32_t gpio, u_int32_t val)
{
	u_int32_t reg;

	HALASSERT(gpio < AR_NUM_GPIO);

	reg =  OS_REG_READ(ah, AR_GPIODO);
	reg &= ~(1 << gpio);
	reg |= (val&1) << gpio;

	OS_REG_WRITE(ah, AR_GPIODO, reg);
	return AH_TRUE;
}

/*
 * Once configured for I/O - get input lines
 */
u_int32_t
ar5211GpioGet(struct ath_hal *ah, u_int32_t gpio)
{
	if (gpio < AR_NUM_GPIO) {
		u_int32_t val = OS_REG_READ(ah, AR_GPIODI);
		val = ((val & AR_GPIOD_MASK) >> gpio) & 0x1;
		return val;
	} else  {
		return 0xffffffff;
	}
}

/*
 * Set the GPIO 0 Interrupt (gpio is ignored)
 */
void
ar5211GpioSetIntr(struct ath_hal *ah, u_int gpio, u_int32_t ilevel)
{
	u_int32_t val = OS_REG_READ(ah, AR_GPIOCR);

	/* Clear the bits that we will modify. */
	val &= ~(AR_GPIOCR_INT_SEL0 | AR_GPIOCR_INT_SELH | AR_GPIOCR_INT_ENA |
			AR_GPIOCR_0_CR_A);

	val |= AR_GPIOCR_INT_SEL0 | AR_GPIOCR_INT_ENA;
	if (ilevel)
		val |= AR_GPIOCR_INT_SELH;

	/* Don't need to change anything for low level interrupt. */
	OS_REG_WRITE(ah, AR_GPIOCR, val);

	/* Change the interrupt mask. */
	ar5211SetInterrupts(ah, AH5211(ah)->ah_maskReg | HAL_INT_GPIO);
}

/*
 * Change the LED blinking pattern to correspond to the connectivity
 */
void
ar5211SetLedState(struct ath_hal *ah, HAL_LED_STATE state)
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
ar5211WriteAssocid(struct ath_hal *ah, const u_int8_t *bssid, u_int16_t assocId)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	/* XXX save bssid for possible re-use on reset */
	OS_MEMCPY(ahp->ah_bssid, bssid, IEEE80211_ADDR_LEN);
	OS_REG_WRITE(ah, AR_BSS_ID0, LE_READ_4(ahp->ah_bssid));
	OS_REG_WRITE(ah, AR_BSS_ID1, LE_READ_2(ahp->ah_bssid+4) |
				     ((assocId & 0x3fff)<<AR_BSS_ID1_AID_S));
}

/*
 * Get the current hardware tsf for stamlme.
 */
u_int64_t
ar5211GetTsf64(struct ath_hal *ah)
{
	u_int64_t tsf;

	/* XXX sync multi-word read? */
	tsf = OS_REG_READ(ah, AR_TSF_U32);
	tsf = (tsf << 32) | OS_REG_READ(ah, AR_TSF_L32);
	return tsf;
}

/*
 * Get the current hardware tsf for stamlme.
 */
u_int32_t
ar5211GetTsf32(struct ath_hal *ah)
{
	return OS_REG_READ(ah, AR_TSF_L32);
}

/*
 * Reset the current hardware tsf for stamlme
 */
void
ar5211ResetTsf(struct ath_hal *ah)
{
	u_int32_t val = OS_REG_READ(ah, AR_BEACON);

	OS_REG_WRITE(ah, AR_BEACON, val | AR_BEACON_RESET_TSF);
}

/*
 * Grab a semi-random value from hardware registers - may not
 * change often
 */
u_int32_t
ar5211GetRandomSeed(struct ath_hal *ah)
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
ar5211DetectCardPresent(struct ath_hal *ah)
{
	u_int16_t macVersion, macRev;
	u_int32_t v;

	/*
	 * Read the Silicon Revision register and compare that
	 * to what we read at attach time.  If the same, we say
	 * a card/device is present.
	 */
	v = OS_REG_READ(ah, AR_SREV) & AR_SREV_ID_M;
	macVersion = v >> AR_SREV_ID_S;
	macRev = v & AR_SREV_REVISION_M;
	return (AH_PRIVATE(ah)->ah_macVersion == macVersion &&
		AH_PRIVATE(ah)->ah_macRev == macRev);
}

/*
 * Update MIB Counters
 */
void
ar5211UpdateMibCounters(struct ath_hal *ah, HAL_MIB_STATS *stats)
{
	stats->ackrcv_bad += OS_REG_READ(ah, AR_ACK_FAIL);
	stats->rts_bad	  += OS_REG_READ(ah, AR_RTS_FAIL);
	stats->fcs_bad	  += OS_REG_READ(ah, AR_FCS_FAIL);
	stats->rts_good	  += OS_REG_READ(ah, AR_RTS_OK);
	stats->beacons	  += OS_REG_READ(ah, AR_BEACON_CNT);
}

HAL_BOOL
ar5211SetSlotTime(struct ath_hal *ah, u_int us)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	if (us < HAL_SLOT_TIME_9 || us > ath_hal_mac_usec(ah, 0xffff)) {
		HALDEBUG(ah, "%s: bad slot time %u\n", __func__, us);
		ahp->ah_slottime = us;	/* restore default handling */
		return AH_FALSE;
	} else {
		/* convert to system clocks */
		OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, ath_hal_mac_clks(ah, us));
		ahp->ah_slottime = us;
		return AH_TRUE;
	}
}

u_int
ar5211GetSlotTime(struct ath_hal *ah)
{
	u_int clks = OS_REG_READ(ah, AR_D_GBL_IFS_SLOT) & 0xffff;
	return ath_hal_mac_usec(ah, clks);	/* convert from system clocks */
}

HAL_BOOL
ar5211SetAckTimeout(struct ath_hal *ah, u_int us)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

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
ar5211GetAckTimeout(struct ath_hal *ah)
{
	u_int clks = MS(OS_REG_READ(ah, AR_TIME_OUT), AR_TIME_OUT_ACK);
	return ath_hal_mac_usec(ah, clks);	/* convert from system clocks */
}

HAL_BOOL
ar5211SetCTSTimeout(struct ath_hal *ah, u_int us)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

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
ar5211GetCTSTimeout(struct ath_hal *ah)
{
	u_int clks = MS(OS_REG_READ(ah, AR_TIME_OUT), AR_TIME_OUT_CTS);
	return ath_hal_mac_usec(ah, clks);	/* convert from system clocks */
}

HAL_BOOL
ar5211SetDecompMask(struct ath_hal *ah, u_int16_t keyidx, int en)
{
	/* nothing to do */
        return AH_TRUE;
}

void
ar5211SetCoverageClass(struct ath_hal *ah, u_int8_t coverageclass, int now)
{
}

/*
 * Control Adaptive Noise Immunity Parameters
 */
HAL_BOOL
ar5211AniControl(struct ath_hal *ah, HAL_ANI_CMD cmd, int param)
{
	return AH_FALSE;
}

void
ar5211AniPoll(struct ath_hal *ah, const HAL_NODE_STATS *stats, HAL_CHANNEL *chan)
{
}

void
ar5211MibEvent(struct ath_hal *ah, const HAL_NODE_STATS *stats)
{
}

/*
 * Get the rssi of frame curently being received.
 */
u_int32_t
ar5211GetCurRssi(struct ath_hal *ah)
{
	return (OS_REG_READ(ah, AR_PHY_CURRENT_RSSI) & 0xff);
}

u_int
ar5211GetDefAntenna(struct ath_hal *ah)
{   
	return (OS_REG_READ(ah, AR_DEF_ANTENNA) & 0x7);
}   

void
ar5211SetDefAntenna(struct ath_hal *ah, u_int antenna)
{
	OS_REG_WRITE(ah, AR_DEF_ANTENNA, (antenna & 0x7));
}

HAL_BOOL
ar5211SetAntennaSwitch(struct ath_hal *ah, HAL_ANT_SETTING settings,
                       HAL_CHANNEL *chan)
{
	struct ath_hal_5211 *ahp = AH5211(ah);
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

HAL_STATUS
ar5211GetCapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
	u_int32_t capability, u_int32_t *result)
{

	switch (type) {
	case HAL_CAP_CIPHER:		/* cipher handled in hardware */
		switch (capability) {
		case HAL_CIPHER_AES_OCB:
		case HAL_CIPHER_WEP:
		case HAL_CIPHER_CLR:
			return HAL_OK;
		default:
			return HAL_ENOTSUPP;
		}
	default:
		return ath_hal_getcapability(ah, type, capability, result);
	}
}

HAL_BOOL
ar5211SetCapability(struct ath_hal *ah, HAL_CAPABILITY_TYPE type,
	u_int32_t capability, u_int32_t setting, HAL_STATUS *status)
{
	switch (type) {
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
	default:
		return ath_hal_setcapability(ah, type, capability,
			setting, status);
	}
}

HAL_BOOL
ar5211GetDiagState(struct ath_hal *ah, int request,
	const void *args, u_int32_t argsize,
	void **result, u_int32_t *resultsize)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	(void) ahp;
	if (ath_hal_getdiagstate(ah, request, args, argsize, result, resultsize))
		return AH_TRUE;
#ifdef AH_PRIVATE_DIAG
	switch (request) {
	case HAL_DIAG_EEPROM:
		*result = &ahp->ah_eeprom;
		*resultsize = sizeof(HAL_EEPROM);
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
	}
#endif /* AH_PRIVATE_DIAG */
	return AH_FALSE;
}

void
ar5211ArDisable(struct ath_hal *ah)
{
}

void
ar5211ResetAR(struct ath_hal *ah)
{
}

void
ar5211ArEnable(struct ath_hal *ah)
{
}

void
ar5211GetDescInfo(struct ath_hal *ah, HAL_DESC_INFO *desc_info)
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

#endif /* AH_SUPPORT_AR5211 */
