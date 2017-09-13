/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5Owl/ar5Owl_attach.c#6 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#if !defined(AH_SUPPORT_5112) && !defined(AH_SUPPORT_5111) && !defined(AH_SUPPORT_2413)
#error "No 5212 RF support defined"
#endif

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"

#include "ar5Owl/ar5Owl.h"
#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212phy.h"
#ifdef AH_SUPPORT_AR5311
#include "ar5212/ar5311reg.h"
#endif

static HAL_BOOL	ar5212GetChipPowerLimits(struct ath_hal *ah,
		HAL_CHANNEL *chans, u_int32_t nchans);

static const struct ath_hal_private ar5212hal = {{
	.ah_magic			= AR5212_MAGIC,
	.ah_abi				= HAL_ABI_VERSION,
	.ah_countryCode			= CTRY_DEFAULT,

	.ah_getRateTable		= ar5212GetRateTable,
	.ah_detach			= ar5212Detach,

	/* Reset Functions */
	.ah_reset			= ar5212Reset,
	.ah_phyDisable			= ar5212PhyDisable,
	.ah_setPCUConfig		= ar5212SetPCUConfig,
	.ah_perCalibration		= ar5212PerCalibration,
	.ah_setTxPowerLimit		= ar5212SetTxPowerLimit,

	/* AR/Radar Functions */
	.ah_arEnable			= ar5212ArEnable,
	.ah_arDisable			= ar5212ArDisable,
	.ah_arReset			= ar5212ResetAR,

	/* Transmit functions */
	.ah_updateTxTrigLevel		= ar5212UpdateTxTrigLevel,
	.ah_setupTxQueue		= ar5212SetupTxQueue,
	.ah_setTxQueueProps             = ar5212SetTxQueueProps,
	.ah_getTxQueueProps             = ar5212GetTxQueueProps,
	.ah_releaseTxQueue		= ar5212ReleaseTxQueue,
	.ah_resetTxQueue		= ar5212ResetTxQueue,
	.ah_getTxDP			= ar5212GetTxDP,
	.ah_setTxDP			= ar5212SetTxDP,
	.ah_numTxPending		= ar5212NumTxPending,
	.ah_startTxDma			= ar5212StartTxDma,
	.ah_stopTxDma			= ar5212StopTxDma,
	.ah_updateCTSForBursting	= ar5212UpdateCTSForBursting,	
	.ah_setupTxDesc			= ar5212SetupTxDesc,
	.ah_setupXTxDesc		= ar5212SetupXTxDesc,
	.ah_fillTxDesc			= ar5212FillTxDesc,
	.ah_procTxDesc			= ar5212ProcTxDesc,
	.ah_getTxIntrQueue		= ar5212GetTxIntrQueue,

	/* RX Functions */
	.ah_getRxDP			= ar5212GetRxDP,
	.ah_setRxDP			= ar5212SetRxDP,
	.ah_enableReceive		= ar5212EnableReceive,
	.ah_stopDmaReceive		= ar5212StopDmaReceive,
	.ah_startPcuReceive		= ar5212StartPcuReceive,
	.ah_stopPcuReceive		= ar5212StopPcuReceive,
	.ah_setMulticastFilter		= ar5212SetMulticastFilter,
	.ah_setMulticastFilterIndex	= ar5212SetMulticastFilterIndex,
	.ah_clrMulticastFilterIndex	= ar5212ClrMulticastFilterIndex,
	.ah_getRxFilter			= ar5212GetRxFilter,
	.ah_setRxFilter			= ar5212SetRxFilter,
	.ah_setupRxDesc			= ar5212SetupRxDesc,
	.ah_procRxDesc			= ar5212ProcRxDesc,
	.ah_rxMonitor			= ar5212AniArPoll,
	.ah_procMibEvent		= ar5212ProcessMibIntr,

	/* Misc Functions */
	.ah_getCapability		= ar5212GetCapability,
	.ah_setCapability		= ar5212SetCapability,
	.ah_getDiagState		= ar5212GetDiagState,
	.ah_getMacAddress		= ar5212GetMacAddress,
	.ah_setMacAddress		= ar5212SetMacAddress,
	.ah_getBssIdMask		= ar5212GetBssIdMask,
	.ah_setBssIdMask		= ar5212SetBssIdMask,
	.ah_setRegulatoryDomain		= ar5212SetRegulatoryDomain,
	.ah_setLedState			= ar5212SetLedState,
	.ah_writeAssocid		= ar5212WriteAssocid,
	.ah_gpioCfgInput		= ar5212GpioCfgInput,
	.ah_gpioCfgOutput		= ar5212GpioCfgOutput,
	.ah_gpioGet			= ar5212GpioGet,
	.ah_gpioSet			= ar5212GpioSet,
	.ah_gpioSetIntr			= ar5212GpioSetIntr,
	.ah_getTsf32			= ar5212GetTsf32,
	.ah_getTsf64			= ar5212GetTsf64,
	.ah_resetTsf			= ar5212ResetTsf,
	.ah_detectCardPresent		= ar5212DetectCardPresent,
	.ah_updateMibCounters		= ar5212UpdateMibCounters,
	.ah_getRfGain			= ar5212GetRfgain,
	.ah_getDefAntenna		= ar5212GetDefAntenna,
	.ah_setDefAntenna		= ar5212SetDefAntenna,
	.ah_setSlotTime			= ar5212SetSlotTime,
	.ah_getSlotTime			= ar5212GetSlotTime,
	.ah_setAckTimeout		= ar5212SetAckTimeout,
	.ah_getAckTimeout		= ar5212GetAckTimeout,
	.ah_setCTSTimeout		= ar5212SetCTSTimeout,
	.ah_getCTSTimeout		= ar5212GetCTSTimeout,
	.ah_setDecompMask               = ar5212SetDecompMask,
	.ah_setCoverageClass            = ar5212SetCoverageClass,
        .ah_getDescInfo                 = ar5212GetDescInfo,

	/* Key Cache Functions */
	.ah_getKeyCacheSize		= ar5212GetKeyCacheSize,
	.ah_resetKeyCacheEntry		= ar5212ResetKeyCacheEntry,
	.ah_isKeyCacheEntryValid	= ar5212IsKeyCacheEntryValid,
	.ah_setKeyCacheEntry		= ar5212SetKeyCacheEntry,
	.ah_setKeyCacheEntryMac		= ar5212SetKeyCacheEntryMac,

	/* Power Management Functions */
	.ah_setPowerMode		= ar5212SetPowerMode,
	.ah_getPowerMode		= ar5212GetPowerMode,

	/* Beacon Functions */
	.ah_beaconInit			= ar5212BeaconInit,
	.ah_setStationBeaconTimers	= ar5212SetStaBeaconTimers,
	.ah_resetStationBeaconTimers	= ar5212ResetStaBeaconTimers,
	.ah_waitForBeaconDone		= ar5212WaitForBeaconDone,

	/* Interrupt Functions */
	.ah_isInterruptPending		= ar5212IsInterruptPending,
	.ah_getPendingInterrupts	= ar5212GetPendingInterrupts,
	.ah_getInterrupts		= ar5212GetInterrupts,
	.ah_setInterrupts		= ar5212SetInterrupts,
    .ah_reset11n            = ar5212Reset11n,
    .ah_set11nTxDesc        = ar5212Set11nTxDesc,
    .ah_set11nRateScenario      = ar5212Set11nRateScenario,
    .ah_set11nAggrFirst     = ar5212Set11nAggrFirst,
    .ah_set11nAggrMiddle        = ar5212Set11nAggrMiddle,
    .ah_set11nAggrLast      = ar5212Set11nAggrLast,
    .ah_set11nBurstDuration     = ar5212Set11nBurstDuration,
    .ah_get11nExtBusy       = ar5212Get11nExtBusy,
    .ah_set11nMac2040	    = ar5212Set11nMac2040,

    },

	.ah_getChannelEdges		= ar5212GetChannelEdges,
	.ah_getWirelessModes		= ar5212GetWirelessModes,
	.ah_eepromRead			= ar5212EepromRead,
#ifdef AH_SUPPORT_WRITE_EEPROM
	.ah_eepromWrite			= ar5212EepromWrite,
#endif
	.ah_gpioCfgOutput		= ar5212GpioCfgOutput,
	.ah_gpioCfgInput		= ar5212GpioCfgInput,
	.ah_gpioGet			= ar5212GpioGet,
	.ah_gpioSet			= ar5212GpioSet,
	.ah_gpioSetIntr			= ar5212GpioSetIntr,
	.ah_getChipPowerLimits		= ar5212GetChipPowerLimits,
};


/*
 * TODO: Need to talk to Praveen about this, these are
 * not valid 2.4 channels, either we change these
 * or I need to change the beanie coding to accept these
 */
static const u_int16_t channels11b[] = { 2412, 2447, 2484 };
static const u_int16_t channels11g[] = { 2312, 2412, 2484 };

u_int32_t
ar5212GetRadioRev(struct ath_hal *ah)
{
	u_int32_t val;
	int i;

	/* Read Radio Chip Rev Extract */
	OS_REG_WRITE(ah, AR_PHY(0x34), 0x00001c16);
	for (i = 0; i < 8; i++)
		OS_REG_WRITE(ah, AR_PHY(0x20), 0x00010000);
	val = (OS_REG_READ(ah, AR_PHY(256)) >> 24) & 0xff;
	val = ((val & 0xf0) >> 4) | ((val & 0x0f) << 4);
	return ath_hal_reverseBits(val, 8);
}

static void
ar5212AniSetup(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	int i;

	const int totalSizeDesired[] = { -55, -55, -55, -55, -62 };
	const int coarseHigh[]       = { -14, -14, -14, -14, -12 };
	const int coarseLow[]        = { -64, -64, -64, -64, -70 };
	const int firpwr[]           = { -78, -78, -78, -78, -80 };

	for (i = 0; i < 5; i++) {
		ahp->ah_totalSizeDesired[i] = totalSizeDesired[i];
		ahp->ah_coarseHigh[i] = coarseHigh[i];
		ahp->ah_coarseLow[i] = coarseLow[i];
		ahp->ah_firpwr[i] = firpwr[i];
	}
}

/*
 * Attach for an AR5212 part.
 */
struct ath_hal_5212 *
ar5212NewState(u_int16_t devid, HAL_SOFTC sc,
	HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status)
{
#define	N(a)	(sizeof(a)/sizeof(a[0]))
	static const u_int8_t defbssidmask[IEEE80211_ADDR_LEN] =
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	struct ath_hal_5212 *ahp;
	struct ath_hal *ah;

	/* NB: memory is returned zero'd */
	ahp = ath_hal_malloc(sizeof (struct ath_hal_5212));
	if (ahp == AH_NULL) {
		HALDEBUG(AH_NULL, "%s: cannot allocate memory for "
			"state block\n", __func__);
		*status = HAL_ENOMEM;
		return AH_NULL;
	}
	ah = &ahp->ah_priv.h;
	/* set initial values */
	OS_MEMCPY(&ahp->ah_priv, &ar5212hal, sizeof(struct ath_hal_private));
	ah->ah_sc = sc;
	ah->ah_st = st;
	ah->ah_sh = sh;

	AH_PRIVATE(ah)->ah_devid = devid;
	AH_PRIVATE(ah)->ah_subvendorid = 0;	/* XXX */

	AH_PRIVATE(ah)->ah_powerLimit = MAX_RATE_POWER;
	AH_PRIVATE(ah)->ah_tpScale = HAL_TP_SCALE_MAX;	/* no scaling */

	ahp->ah_atimWindow = 0;			/* [0..1000] */
	ahp->ah_diversityControl = HAL_ANT_VARIABLE;
	ahp->ah_bIQCalibration = AH_FALSE;
	/*
	 * Enable MIC handling.
	 */
	ahp->ah_staId1Defaults = AR_STA_ID1_CRPT_MIC_ENABLE;
	ahp->ah_tpcEnabled = AH_FALSE;		/* disabled by default */
	ahp->ah_beaconInterval = 100;		/* XXX [20..1000] */
	ahp->ah_enable32kHzClock = DONT_USE_32KHZ;/* XXX */
	ahp->ah_slottime = (u_int) -1;
	ahp->ah_acktimeout = (u_int) -1;
	ahp->ah_ctstimeout = (u_int) -1;
	OS_MEMCPY(&ahp->ah_bssidmask, defbssidmask, IEEE80211_ADDR_LEN);

	/*
	 * 11g-specific stuff
	 */
	ahp->ah_gBeaconRate = 0;		/* adhoc beacon fixed rate */

	return ahp;
#undef N
}
       
/*
 * Attach for an AR5212 part.
 */
struct ath_hal *
ar5212Attach(u_int16_t devid, HAL_SOFTC sc,
	HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status)
{
	struct ath_hal_5212 *ahp;
	struct ath_hal *ah;
	u_int i;
	u_int32_t sum, val, eepMax;
	u_int16_t eeval;
	HAL_STATUS ecode;
	HAL_BOOL rfStatus;

	HALDEBUG(AH_NULL, "%s: sc %p st %u sh %p\n",
		__func__, sc, st, (void*) sh);

	/* NB: memory is returned zero'd */
	ahp = ar5212NewState(devid, sc, st, sh, status);
	if (ahp == AH_NULL)
		return AH_NULL;
	ah = &ahp->ah_priv.h;

	if (!ar5212SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE)) {
		HALDEBUG(ah, "%s: couldn't wakeup chip\n", __func__);
		ecode = HAL_EIO;
		goto bad;
	}
	if (!ar5212ChipReset(ah, AH_NULL)) {	/* reset chip */
		HALDEBUG(ah, "%s: chip reset failed\n", __func__);
		ecode = HAL_EIO;
		goto bad;
	}

	/* Read Revisions from Chips */
	val = OS_REG_READ(ah, AR_SREV) & AR_SREV_ID;
	AH_PRIVATE(ah)->ah_macVersion = val >> AR_SREV_ID_S;
	AH_PRIVATE(ah)->ah_macRev = val & AR_SREV_REVISION;

	if ((AH_PRIVATE(ah)->ah_macVersion != AR_SREV_VERSION_VENICE &&
	     AH_PRIVATE(ah)->ah_macVersion != AR_SREV_VERSION_GRIFFIN) ||
	    AH_PRIVATE(ah)->ah_macRev < AR_SREV_D2PLUS) {
		HALDEBUG(ah, "%s: Mac Chip Rev 0x%02x.%x is not supported by "
			"this driver\n", __func__,
			AH_PRIVATE(ah)->ah_macVersion,
			AH_PRIVATE(ah)->ah_macRev);
		ecode = HAL_ENOTSUPP;
		goto bad;
	}

	AH_PRIVATE(ah)->ah_phyRev = OS_REG_READ(ah, AR_PHY_CHIP_ID);

	if (!ar5212ChipTest(ah)) {
		HALDEBUG(ah, "%s: hardware self-test failed\n", __func__);
		ecode = HAL_ESELFTEST;
		goto bad;
	}

	/*
	 * Set correct Baseband to analog shift
	 * setting to access analog chips.
	 */
	OS_REG_WRITE(ah, AR_PHY(0), 0x00000007);

	/* Read Radio Chip Rev Extract */
	AH_PRIVATE(ah)->ah_analog5GhzRev = ar5212GetRadioRev(ah);
	/* NB: silently accept anything in release code per Atheros */
	switch (AH_PRIVATE(ah)->ah_analog5GhzRev & AR_RADIO_SREV_MAJOR) {
	case AR_RAD5111_SREV_MAJOR:
	case AR_RAD5112_SREV_MAJOR:
	case AR_RAD2111_SREV_MAJOR:
	case AR_SREV_2413:
	case AR_RAD2413_SREV_MAJOR:
		break;
	default:
		if (AH_PRIVATE(ah)->ah_analog5GhzRev == 0) {
			/*
			 * WAR for bug 10062.  When RF_Silent is used, the
			 * analog chip is reset.  So when the system boots
			 * up with the radio switch off we cannot determine
			 * the RF chip rev.  To workaround this check the
			 * mac+phy revs and if Hainan, set the radio rev
			 * to Derby.
			 */
			if (AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_VENICE &&
			    AH_PRIVATE(ah)->ah_macRev == AR_SREV_HAINAN &&
			    AH_PRIVATE(ah)->ah_phyRev == AR_PHYREV_HAINAN) {
				AH_PRIVATE(ah)->ah_analog5GhzRev = AR_ANALOG5REV_HAINAN;
				break;
			}
			if (IS_2413(ah)) {		/* Griffin */
				AH_PRIVATE(ah)->ah_analog5GhzRev = 0x51;
				break;
			}
		}
#ifdef AH_DEBUG
		HALDEBUG(ah, "%s: 5G Radio Chip Rev 0x%02X is not supported by "
			"this driver\n", __func__,
			AH_PRIVATE(ah)->ah_analog5GhzRev);
		ecode = HAL_ENOTSUPP;
		goto bad;
#endif
	}
	if (IS_5112(ah) && IS_RAD5112_REV1(ah)) {
		HALDEBUG(ah, "%s: 5112 Rev 1 is not supported by this "
			"driver (analog5GhzRev 0x%x)\n", __func__,
			AH_PRIVATE(ah)->ah_analog5GhzRev);
		ecode = HAL_ENOTSUPP;
		goto bad;
	}
	if (!ar5212EepromRead(ah, AR_EEPROM_VERSION, &eeval)) {
		HALDEBUG(ah, "%s: unable to read EEPROM version\n", __func__);
		ecode = HAL_EEREAD;
		goto bad;
	}
	if (eeval < AR_EEPROM_VER3_2) {
		HALDEBUG(ah, "%s: unsupported EEPROM version %u (0x%x)\n",
			__func__, eeval, eeval);
		ecode = HAL_EEVERSION;
		goto bad;
	}
	ahp->ah_eeversion = eeval;

	val = OS_REG_READ(ah, AR_PCICFG);
	val = MS(val, AR_PCICFG_EEPROM_SIZE);
	if (val != AR_PCICFG_EEPROM_SIZE_16K) {
		HALDEBUG(ah, "%s: unsupported EEPROM size "
			"%u (0x%x) found\n", __func__, val, val);
		ecode = HAL_EESIZE;
		goto bad;
	}

	if (!ar5212EepromRead(ah, AR_EEPROM_PROTECT, &eeval)) {
		HALDEBUG(ah, "%s: cannot read EEPROM protection "
			"bits; read locked?\n", __func__);
		ecode = HAL_EEREAD;
		goto bad;
	}
	HALDEBUG(ah, "EEPROM protect 0x%x\n", eeval);
	ahp->ah_eeprotect = eeval;
	/* XXX check proper access before continuing */

	/*
	 * Read the Atheros EEPROM entries and calculate the checksum.
	 */
	if (!ar5212EepromRead(ah, AR_EEPROM_SIZE_UPPER, &eeval)) {
		HALDEBUG(ah, "%s: cannot read EEPROM upper size\n" , __func__);
		ecode = HAL_EEREAD;
		goto bad;
	}
	if (eeval != 0)	{
		eepMax = (eeval & AR_EEPROM_SIZE_UPPER_MASK) <<
			AR_EEPROM_SIZE_ENDLOC_SHIFT;
		if (!ar5212EepromRead(ah, AR_EEPROM_SIZE_LOWER, &eeval)) {
			HALDEBUG(ah, "%s: cannot read EEPROM lower size\n" ,
				__func__);
			ecode = HAL_EEREAD;
			goto bad;
		}
		eepMax = (eepMax | eeval) - AR_EEPROM_ATHEROS_BASE;
	} else
		eepMax = AR_EEPROM_ATHEROS_MAX;
	sum = 0;
	for (i = 0; i < eepMax; i++) {
		if (!ar5212EepromRead(ah, AR_EEPROM_ATHEROS(i), &eeval)) {
			ecode = HAL_EEREAD;
			goto bad;
		}
		sum ^= eeval;
	}
	if (sum != 0xffff) {
		HALDEBUG(ah, "%s: bad EEPROM checksum 0x%x\n", __func__, sum);
		ecode = HAL_EEBADSUM;
		goto bad;
	}

	ahp->ah_numChannels11a = NUM_11A_EEPROM_CHANNELS;
	ahp->ah_numChannels2_4 = NUM_2_4_EEPROM_CHANNELS;

	for (i = 0; i < NUM_11A_EEPROM_CHANNELS; i ++)
		ahp->ah_dataPerChannel11a[i].numPcdacValues = NUM_PCDAC_VALUES;

	/* the channel list for 2.4 is fixed, fill this in here */
	for (i = 0; i < NUM_2_4_EEPROM_CHANNELS; i++) {
		ahp->ah_channels11b[i] = channels11b[i];
		ahp->ah_channels11g[i] = channels11g[i];
		ahp->ah_dataPerChannel11b[i].numPcdacValues = NUM_PCDAC_VALUES;
		ahp->ah_dataPerChannel11g[i].numPcdacValues = NUM_PCDAC_VALUES;
	}

	if (!ath_hal_readEepromIntoDataset(ah, &ahp->ah_eeprom)) {
		ecode = HAL_EEREAD;		/* XXX */
		goto bad;
	}

	/*
	 * If Bmode and AR5212, verify 2.4 analog exists
	 */
	if (ahp->ah_Bmode &&
	    (AH_PRIVATE(ah)->ah_analog5GhzRev & 0xF0) == AR_RAD5111_SREV_MAJOR) {
		/*
		 * Set correct Baseband to analog shift
		 * setting to access analog chips.
		 */
		OS_REG_WRITE(ah, AR_PHY(0), 0x00004007);
		OS_DELAY(2000);
		AH_PRIVATE(ah)->ah_analog2GhzRev = ar5212GetRadioRev(ah);

		/* Set baseband for 5GHz chip */
		OS_REG_WRITE(ah, AR_PHY(0), 0x00000007);
		OS_DELAY(2000);
		if ((AH_PRIVATE(ah)->ah_analog2GhzRev & 0xF0) != AR_RAD2111_SREV_MAJOR) {
			HALDEBUG(ah, "%s: 2G Radio Chip Rev 0x%02X is not "
				"supported by this driver\n", __func__,
				AH_PRIVATE(ah)->ah_analog2GhzRev);
			ecode = HAL_ENOTSUPP;
			goto bad;
		}
	}

        if (!ar5212EepromRead(ah, AR_EEPROM_REG_DOMAIN, &eeval)) {
		HALDEBUG(ah, "%s: cannot read regulator domain from EEPROM\n",
			__func__);
		ecode = HAL_EEREAD;
		goto bad;
        }
	/* XXX record serial number */
	ahp->ah_regdomain = eeval;
	AH_PRIVATE(ah)->ah_currentRD = ahp->ah_regdomain;
	/*
	 * Got everything we need now to setup the capabilities.
	 */
	if (!ar5212FillCapabilityInfo(ah)) {
		HALDEBUG(ah, "%s:failed ar5212FillCapabilityInfo\n", __func__);
		ecode = HAL_EEREAD;
		goto bad;
	}

	rfStatus = AH_FALSE;
	if (IS_2413(ah))
#ifdef AH_SUPPORT_2413
		rfStatus = ar2413RfAttach(ah, &ecode);
#else
		ecode = HAL_ENOTSUPP;
#endif
	else if (IS_5112(ah))
#ifdef AH_SUPPORT_5112
		rfStatus = ar5112RfAttach(ah, &ecode);
#else
		ecode = HAL_ENOTSUPP;
#endif
	else
#ifdef AH_SUPPORT_5111
		rfStatus = ar5111RfAttach(ah, &ecode);
#else
		ecode = HAL_ENOTSUPP;
#endif
	if (!rfStatus) {
		HALDEBUG(ah, "%s: RF setup failed, status %u\n",
			__func__, ecode);
		goto bad;
	}

	/* Initialize gain ladder thermal calibration structure */
	ar5212InitializeGainValues(ah);

	sum = 0;
	for (i = 0; i < 3; i++) {
		if (!ar5212EepromRead(ah, AR_EEPROM_MAC(2-i), &eeval)) {
			HALDEBUG(ah, "%s: cannot read EEPROM "
				"location %u\n", __func__, i);
			ecode = HAL_EEREAD;
			goto bad;
		}
		sum += eeval;
		ahp->ah_macaddr[2*i] = eeval >> 8;
		ahp->ah_macaddr[2*i + 1] = eeval & 0xff;
	}
	if (sum == 0 || sum == 0xffff*3) {
		HALDEBUG(ah, "%s: mac address read failed: %s\n",
		    __func__, ath_hal_ether_sprintf(ahp->ah_macaddr));
		ecode = HAL_EEBADMAC;
		goto bad;
	}

	HALDEBUG(ah, "%s: return ah %p\n", __func__, ah);

	ar5212AniSetup(ah);	/* setup 5212-specific ANI tables */
	ar5212AniAttach(ah);
	if ((ecode = ar5212RadarAttach(ah)) != HAL_OK) {	/* Setup 5212 Radar/AR structures */
		goto bad;
	}
	/* XXX EAR stuff goes here */

	return ah;

bad:
	if (ahp)
		ar5212Detach((struct ath_hal *) ahp);
	if (status)
		*status = ecode;
	return AH_NULL;
}

void
ar5212Detach(struct ath_hal *ah)
{
	HALDEBUG(ah, "%s: ah %p\n", __func__, ah);

	HALASSERT(ah != AH_NULL);
	HALASSERT(ah->ah_magic == AR5212_MAGIC);

	/* XXX EEPROM allocated state */
	ar5212AniDetach(ah);
	ar5212RfDetach(ah);
	ar5212RadarDetach(ah);

	ar5212SetPowerMode(ah, HAL_PM_FULL_SLEEP, AH_TRUE);

	ath_hal_free(ah);
}

HAL_BOOL
ar5212ChipTest(struct ath_hal *ah)
{
	u_int32_t regAddr[2] = { AR_STA_ID0, AR_PHY_BASE+(8 << 2) };
	u_int32_t regHold[2];
	u_int32_t patternData[4] =
	    { 0x55555555, 0xaaaaaaaa, 0x66666666, 0x99999999 };
	int i, j;

	/* Test PHY & MAC registers */
	for (i = 0; i < 2; i++) {
		u_int32_t addr = regAddr[i];
		u_int32_t wrData, rdData;

		regHold[i] = OS_REG_READ(ah, addr);
		for (j = 0; j < 0x100; j++) {
			wrData = (j << 16) | j;
			OS_REG_WRITE(ah, addr, wrData);
			rdData = OS_REG_READ(ah, addr);
			if (rdData != wrData) {
				HALDEBUG(ah,
"%s: address test failed addr: 0x%08x - wr:0x%08x != rd:0x%08x\n",
				__func__, addr, wrData, rdData);
				return AH_FALSE;
			}
		}
		for (j = 0; j < 4; j++) {
			wrData = patternData[j];
			OS_REG_WRITE(ah, addr, wrData);
			rdData = OS_REG_READ(ah, addr);
			if (wrData != rdData) {
				HALDEBUG(ah,
"%s: address test failed addr: 0x%08x - wr:0x%08x != rd:0x%08x\n",
					__func__, addr, wrData, rdData);
				return AH_FALSE;
			}
		}
		OS_REG_WRITE(ah, regAddr[i], regHold[i]);
	}
	OS_DELAY(100);
	return AH_TRUE;
}

/*
 * Store the channel edges for the requested operational mode
 */
HAL_BOOL
ar5212GetChannelEdges(struct ath_hal *ah,
	u_int16_t flags, u_int16_t *low, u_int16_t *high)
{
	if (flags & CHANNEL_5GHZ) {
		*low = 4915;
		*high = 6100;
		return AH_TRUE;
	}
	if ((flags & CHANNEL_2GHZ) && AH5212(ah)->ah_Bmode) {
		*low = 2312;
		*high = 2732;
		return AH_TRUE;
	}
	return AH_FALSE;
}

static HAL_BOOL
ar5212GetChipPowerLimits(struct ath_hal *ah, HAL_CHANNEL *chans, u_int32_t nchans)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	return ahp->ah_rfHal.getChipPowerLim(ah, chans, nchans);
}

/*
 * Fill all software cached or static hardware state information.
 * Return failure if capabilities are to come from EEPROM and
 * cannot be read.
 */
HAL_BOOL
ar5212FillCapabilityInfo(struct ath_hal *ah)
{
#define	AR_KEYTABLE_SIZE	128
#define	IS_GRIFFIN_LITE(ah) \
    (AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_GRIFFIN && \
     AH_PRIVATE(ah)->ah_macRev == AR_SREV_GRIFFIN_LITE)
#define	IS_COBRA(ah) \
    (AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_COBRA)
#define IS_2112(ah) \
	((AH_PRIVATE(ah)->ah_analog5GhzRev & 0xF0) == AR_RAD2112_SREV_MAJOR)

	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
	HAL_CAPABILITIES *pCap = &ahpriv->ah_caps;
	u_int16_t capField;

	/* Read the capability EEPROM location */
	capField = 0;
	if (ahp->ah_eeversion >= AR_EEPROM_VER5_1 &&
	    !ath_hal_eepromRead(ah, AR_EEPROM_CAPABILITIES_OFFSET, &capField)) {
		HALDEBUG(ah, "%s: unable to read caps from eeprom\n", __func__);
		return AH_FALSE;
	}
	if (IS_2112(ah))
		ahp->ah_Amode = AH_FALSE;
	if (capField == 0 && IS_GRIFFIN_LITE(ah)) {
		/*
		 * WAR for griffin-lite cards with unprogrammed capabilities.
		 */
		capField = AR_EEPROM_EEPCAP_COMPRESS_DIS
			 | AR_EEPROM_EEPCAP_FASTFRAME_DIS
			 ;
		ahp->ah_turbo5Disable = AH_TRUE;
		ahp->ah_turbo2Disable = AH_TRUE;
		HALDEBUG(ah, "%s: override caps for griffin-lite, "
			"now 0x%x (+no turbo)\n", __func__, capField);
	}

	/* Modify reg domain on newer cards that need to work with older sw */
	if (ahpriv->ah_opmode != HAL_M_HOSTAP &&
	    ahpriv->ah_subvendorid == AR_SUBVENDOR_ID_NEW_A) {
		if (ahpriv->ah_currentRD == 0x64 ||
		    ahpriv->ah_currentRD == 0x65)
			ahpriv->ah_currentRD += 5;
		else if (ahpriv->ah_currentRD == 0x41)
			ahpriv->ah_currentRD = 0x43;
		HALDEBUG(ah, "%s: regdomain mapped to 0x%x\n",
			__func__, ahpriv->ah_currentRD);
	}

	/* Construct wireless mode from EEPROM */
	pCap->halWirelessModes = 0;
	if (ahp->ah_Amode) {
		pCap->halWirelessModes |= HAL_MODE_11A;
		if (!ahp->ah_turbo5Disable)
			pCap->halWirelessModes |= HAL_MODE_TURBO;
	}
	if (ahp->ah_Bmode)
		pCap->halWirelessModes |= HAL_MODE_11B;
	if (ahp->ah_Gmode &&
	    ahpriv->ah_subvendorid != AR_SUBVENDOR_ID_NOG) {
		pCap->halWirelessModes |= HAL_MODE_11G;
		if (!ahp->ah_turbo2Disable)
			pCap->halWirelessModes |= HAL_MODE_108G;
	}

	pCap->halLow2GhzChan = 2312;
	if (IS_5112(ah) || IS_2413(ah))
		pCap->halHigh2GhzChan = 2500;
	else
		pCap->halHigh2GhzChan = 2732;

	pCap->halLow5GhzChan = 4915;
	pCap->halHigh5GhzChan = 6100;

	pCap->halCipherCkipSupport = AH_FALSE;
	pCap->halCipherTkipSupport = AH_TRUE;
	pCap->halCipherAesCcmSupport =
		(!(capField & AR_EEPROM_EEPCAP_AES_DIS) &&
		 ((AH_PRIVATE(ah)->ah_macVersion > AR_SREV_VERSION_VENICE) ||
		  ((AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_VENICE) &&
		   (AH_PRIVATE(ah)->ah_macRev >= AR_SREV_VERSION_OAHU))));

	pCap->halMicCkipSupport    = AH_FALSE;
	pCap->halMicTkipSupport    = AH_TRUE;
	pCap->halMicAesCcmSupport  = !(capField & AR_EEPROM_EEPCAP_AES_DIS);

	pCap->halChanSpreadSupport = AH_TRUE;
	pCap->halSleepAfterBeaconBroken = AH_TRUE;

	if ((ahpriv->ah_macRev > 1) || IS_COBRA(ah)) {
		pCap->halCompressSupport   =
			!(capField & AR_EEPROM_EEPCAP_COMPRESS_DIS) &&
			(pCap->halWirelessModes & (HAL_MODE_11A|HAL_MODE_11G)) != 0;
		pCap->halBurstSupport =
			!(capField & AR_EEPROM_EEPCAP_BURST_DIS);
		pCap->halFastFramesSupport =
			!(capField & AR_EEPROM_EEPCAP_FASTFRAME_DIS) &&
			(pCap->halWirelessModes & (HAL_MODE_11A|HAL_MODE_11G)) != 0;
		pCap->halChapTuningSupport = AH_TRUE;
		pCap->halTurboPrimeSupport = AH_TRUE;
	}
	pCap->halTurboGSupport = pCap->halWirelessModes & HAL_MODE_108G;
	/* Give XR support unless both disable bits are set */
	pCap->halXrSupport = !(ahp->ah_disableXr5 && ahp->ah_disableXr2);

    pCap->halHTSupport = AH_TRUE;
	pCap->halPSPollBroken = AH_TRUE;	/* XXX fixed in later revs? */
	pCap->halVEOLSupport = AH_TRUE;
	pCap->halBssIdMaskSupport = AH_TRUE;
	pCap->halMcastKeySrchSupport = AH_TRUE;
	if ((ahpriv->ah_macVersion == AR_SREV_VERSION_VENICE &&
	     ahpriv->ah_macRev == 8) ||
	    ahpriv->ah_macVersion > AR_SREV_VERSION_VENICE)
		pCap->halTsfAddSupport = AH_TRUE;

	if (capField & AR_EEPROM_EEPCAP_MAXQCU)
		pCap->halTotalQueues = MS(capField, AR_EEPROM_EEPCAP_MAXQCU);
	else
		pCap->halTotalQueues = HAL_NUM_TX_QUEUES;

	if (capField & AR_EEPROM_EEPCAP_KC_ENTRIES)
		pCap->halKeyCacheSize =
			1 << MS(capField, AR_EEPROM_EEPCAP_KC_ENTRIES);
	else
		pCap->halKeyCacheSize = AR_KEYTABLE_SIZE;

	if (IS_5112(ah)) {
		pCap->halChanHalfRate = AH_TRUE;
		pCap->halChanQuarterRate = AH_TRUE;
	} else {
		pCap->halChanHalfRate = AH_FALSE;
		pCap->halChanQuarterRate = AH_FALSE;
	}
    pCap->halNumMRRetries   = 4;

	return AH_TRUE;
#undef IS_COBRA
#undef IS_GRIFFIN_LITE
#undef AR_KEYTABLE_SIZE
}
#endif /* AH_SUPPORT_AR5212 */
