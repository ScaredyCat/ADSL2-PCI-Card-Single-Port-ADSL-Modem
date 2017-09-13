/*
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5312/ar5312_attach.c#2 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5312

#if !defined(AH_SUPPORT_5112) && !defined(AH_SUPPORT_5111)
#error "No 5212 RF support defined"
#endif

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"

#include "ar5312/ar5312.h"
#include "ar5312/ar5312reg.h"
#include "ar5312/ar5312phy.h"

/*
 * TODO: Need to talk to Praveen about this, these are
 * not valid 2.4 channels, either we change these
 * or I need to change the beanie coding to accept these
 */
static const u_int16_t channels11b[] = { 2412, 2447, 2484 };
static const u_int16_t channels11g[] = { 2312, 2412, 2484 };

static void
ar5312AniSetup(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	int i;

	const int totalSizeDesired[] = { -41, -41, -48, -48, -48 };
	const int coarseHigh[]       = { -18, -18, -16, -14, -12 };
	const int coarseLow[]        = { -56, -56, -60, -60, -60 };
	const int firpwr[]           = { -72, -72, -75, -78, -80 };

	for (i = 0; i < 5; i++) {
		ahp->ah_totalSizeDesired[i] = totalSizeDesired[i];
		ahp->ah_coarseHigh[i] = coarseHigh[i];
		ahp->ah_coarseLow[i] = coarseLow[i];
		ahp->ah_firpwr[i] = firpwr[i];
	}
}

/*
 * Attach for an AR3212 part.
 */
struct ath_hal *
ar5312Attach(u_int16_t devid, HAL_SOFTC sc,
	HAL_BUS_TAG st, HAL_BUS_HANDLE sh, HAL_STATUS *status)
{
	struct ath_hal_5212 *ahp = AH_NULL;
	struct ath_hal *ah;
	u_int i;
	u_int32_t sum, val, eepEndLoc;
	u_int16_t eeval, data16;
	HAL_STATUS ecode;
	HAL_BOOL rfStatus;

	HALDEBUG(AH_NULL, "%s: sc %p st %u sh %p\n",
		 __func__, sc, st, (void*) sh);

	/* Setup the flash table so the EEPROM emulation works */
	if (ar5312SetupFlash() == AH_FALSE) {
		HALDEBUG(AH_NULL, "%s: Cannot setup flash\n"
			 , __func__);
		ecode = HAL_ENOTSUPP;
		goto bad;
	}
	  
	/* NB: memory is returned zero'd */
	ahp = ar5212NewState(devid, sc, st, sh, status);
	if (ahp == AH_NULL)
		return AH_NULL;
	ah = &ahp->ah_priv.h;

	/* override 5212 methods for our needs */
	ah->ah_reset			= ar5312Reset;
	ah->ah_phyDisable		= ar5312PhyDisable;
	ah->ah_setLedState		= ar5312SetLedState;
	ah->ah_gpioCfgInput		= ar5312GpioCfgInput;
	ah->ah_gpioCfgOutput		= ar5312GpioCfgOutput;
	ah->ah_gpioGet			= ar5312GpioGet;
	ah->ah_gpioSet			= ar5312GpioSet;
	ah->ah_gpioSetIntr		= ar5312GpioSetIntr;
	ah->ah_detectCardPresent	= ar5312DetectCardPresent;
	ah->ah_setPowerMode		= ar5312SetPowerMode;
	ah->ah_getPowerMode		= ar5312GetPowerMode;
	ah->ah_isInterruptPending	= ar5312IsInterruptPending;

	ahp->ah_priv.ah_eepromRead	= ar5312EepromRead;
#ifdef AH_SUPPORT_WRITE_EEPROM
	ahp->ah_priv.ah_eepromWrite	= ar5312EepromWrite;
#endif
	ahp->ah_priv.ah_gpioCfgOutput	= ar5312GpioCfgOutput;
	ahp->ah_priv.ah_gpioCfgInput	= ar5312GpioCfgInput;
	ahp->ah_priv.ah_gpioGet		= ar5312GpioGet;
	ahp->ah_priv.ah_gpioSet		= ar5312GpioSet;
	ahp->ah_priv.ah_gpioSetIntr	= ar5312GpioSetIntr;

	if (!ar5312ChipReset(ah, AH_NULL)) {	/* reset chip */
		HALDEBUG(ah, "%s: chip reset failed\n", __func__);
		ecode = HAL_EIO;
		goto bad;
	}

#if AH_SUPPORT_2316
	if ((devid == AR5212_AR2315_REV6) || (devid == AR5212_AR2315_REV7)) {
		val = ((OS_REG_READ(ah, (AR5315_RSTIMER_BASE -((u_int32_t) sh)) + AR5315_WREV)) >> AR5315_WREV_S)
			& AR5315_WREV_ID;
		AH_PRIVATE(ah)->ah_macVersion = val >> AR5315_WREV_ID_S;
		AH_PRIVATE(ah)->ah_macRev = val & AR5315_WREV_REVISION;
	} 
    else 
#endif
	{
		val = OS_REG_READ(ah, (AR_RSTIMER_BASE - ((u_int32_t) sh)) + 0x0020);
		val = OS_REG_READ(ah, (AR_RSTIMER_BASE - ((u_int32_t) sh)) + 0x0080);
		/* Read Revisions from Chips */
		val = ((OS_REG_READ(ah, (AR_RSTIMER_BASE - ((u_int32_t) sh)) + AR_WREV)) >> AR_WREV_S) & AR_WREV_ID;
		AH_PRIVATE(ah)->ah_macVersion = val >> AR_WREV_ID_S;
		AH_PRIVATE(ah)->ah_macRev = val & AR_WREV_REVISION;
	}
	if ( ( (AH_PRIVATE(ah)->ah_macVersion != AR_SREV_VERSION_VENICE &&
              AH_PRIVATE(ah)->ah_macVersion != AR_SREV_VERSION_VENICE) ||
             AH_PRIVATE(ah)->ah_macRev < AR_SREV_D2PLUS) &&
              AH_PRIVATE(ah)->ah_macVersion != AR_SREV_VERSION_COBRA) {
		ath_hal_printf(ah, "%s: Mac Chip Rev 0x%02x.%x is not supported by "
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
#ifdef AH_DEBUG
	/* NB: silently accept anything in release code per Atheros */
	if ((AH_PRIVATE(ah)->ah_analog5GhzRev & 0xF0) !=
            AR_RAD5111_SREV_MAJOR &&
	    (AH_PRIVATE(ah)->ah_analog5GhzRev & 0xF0) !=
            AR_RAD5112_SREV_MAJOR &&
	    (AH_PRIVATE(ah)->ah_analog5GhzRev & 0xF0) !=
            AR_RAD2111_SREV_MAJOR &&
            (AH_PRIVATE(ah)->ah_analog5GhzRev & 0xF0) !=
            AR_RAD2112_SREV_MAJOR) {
		ath_hal_printf(ah, "%s: 5G Radio Chip Rev 0x%02X is not supported by "
                         "this driver\n", __func__,
                         AH_PRIVATE(ah)->ah_analog5GhzRev);
		ecode = HAL_ENOTSUPP;
		goto bad;
	}
#endif
	if (IS_5112(ah) && !IS_RADX112_REV2(ah)) {
		ath_hal_printf(ah, "%s: 5112 Rev 1 is not supported by this "
                         "driver (analog5GhzRev 0x%x)\n", __func__,
                         AH_PRIVATE(ah)->ah_analog5GhzRev);
		ecode = HAL_ENOTSUPP;
		goto bad;
	}
        
	if (!ar5312EepromRead(ah, AR_EEPROM_VERSION, &eeval)) {
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
        
        /* Read sizing information */
        if (!ar5312EepromRead(ah, AR_EEPROM_SIZE_UPPER, &data16)) {
                HALDEBUG(ah, "%s: cannot read eeprom upper size\n", __func__);
                ecode = HAL_EEREAD;
                goto bad;
        }
        
        if (data16 == 0) {
                eepEndLoc = AR_EEPROM_ATHEROS_MAX_LOC;
        } else {
                eepEndLoc = (data16 & AR_EEPROM_SIZE_UPPER_MASK) << AR_EEPROM_SIZE_UPPER_SHIFT;
                if (!ar5312EepromRead(ah, AR_EEPROM_SIZE_LOWER, &data16)) {
                        HALDEBUG(ah, "%s: cannot read eeprom lower size\n", __func__
                                 );
                        ecode = HAL_EEREAD;
                        goto bad;
                }
                eepEndLoc |= data16;
        }
        
        HALASSERT(eepEndLoc > AR_EEPROM_ATHEROS_BASE);
        
	if (!ar5312EepromRead(ah, AR_EEPROM_PROTECT, &eeval)) {
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
	sum = 0;
	for (i = 0; i < AR_EEPROM_ATHEROS_MAX; i++) {
		if (!ar5312EepromRead(ah, AR_EEPROM_ATHEROS(i), &eeval)) {
			HALDEBUG(ah, "Error reading during checksum\n");
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
			ath_hal_printf(ah, "%s: 2G Radio Chip Rev 0x%02X is not "
				"supported by this driver\n", __func__,
				AH_PRIVATE(ah)->ah_analog2GhzRev);
			ecode = HAL_ENOTSUPP;
			goto bad;
		}
	}

        if (!ar5312EepromRead(ah, AR_EEPROM_REG_DOMAIN, &eeval)) {
		HALDEBUG(ah, "%s: cannot read regulator domain from EEPROM\n",
			__func__);
		ecode = HAL_EEREAD;
		goto bad;
        }
	/* XXX record serial number */
	ahp->ah_regdomain = eeval;
	AH_PRIVATE(ah)->ah_currentRD = ahp->ah_regdomain;
	/* XXX other capabilities */
	/*
	 * Got everything we need now to setup the capabilities.
	 */
	if (!ar5212FillCapabilityInfo(ah)) {
		HALDEBUG(ah, "%s:failed ar5212FillCapabilityInfo\n", __func__);
		ecode = HAL_EEREAD;
		goto bad;
	}

	rfStatus = AH_FALSE;
        if(IS_2316(ah))
#ifdef AH_SUPPORT_2316
		rfStatus = ar2316RfAttach(ah, &ecode);
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

        /* BSP specific call for MAC address of this WMAC device */
        if (!ar5312GetMacAddr(ah)) {
                ecode = HAL_EEBADMAC;
                goto bad;
        }

	ar5312AniSetup(ah);
	ar5212AniAttach(ah);
	if ((ecode = ar5212RadarAttach(ah)) != HAL_OK) {	/* Setup 5212 Radar/AR structures */
		goto bad;
	}

	/* Disable ANI for all modes on the 5312 SoC */
	ahp->ah_procPhyErr &= ~HAL_PROCESS_ANI;
	/* XXX EAR stuff goes here */
	return ah;

bad:
	if (ahp)
		ar5212Detach((struct ath_hal *) ahp);
	if (status)
		*status = ecode;
	return AH_NULL;
}
#endif /* AH_SUPPORT_AR5312 */
