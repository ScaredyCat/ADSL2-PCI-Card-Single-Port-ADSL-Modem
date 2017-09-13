/*
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2004 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5312/ar5312_power.c#2 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5312

#include "ah.h"
#include "ah_internal.h"

#include "ar5312/ar5312.h"
#include "ar5312/ar5312reg.h"
#include "ar5212/ar5212desc.h"

/*
 * Notify Power Mgt is enabled in self-generated frames.
 * If requested, force chip awake.
 *
 * Returns A_OK if chip is awake or successfully forced awake.
 *
 * WARNING WARNING WARNING
 * There is a problem with the chip where sometimes it will not wake up.
 */
static HAL_BOOL
ar5312SetPowerModeAwake(struct ath_hal *ah, int setChip)
{
        /* No need for this at the moment for APs */
	return AH_TRUE;
}

/*
 * Notify Power Mgt is disabled in self-generated frames.
 * If requested, force chip to sleep.
 */
static void
ar5312SetPowerModeSleep(struct ath_hal *ah, int setChip)
{
        /* No need for this at the moment for APs */
}

/*
 * Notify Power Management is enabled in self-generating
 * fames.  If request, set power mode of chip to
 * auto/normal.  Duration in units of 128us (1/8 TU).
 */
static void
ar5312SetPowerModeNetworkSleep(struct ath_hal *ah, int setChip)
{
        /* No need for this at the moment for APs */
}

/*
 * Set power mgt to the requested mode, and conditionally set
 * the chip as well
 */
HAL_BOOL
ar5312SetPowerMode(struct ath_hal *ah, HAL_POWER_MODE mode, int setChip)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
#ifdef AH_DEBUG
	static const char* modes[] = {
		"UNDEFINED",
		"AWAKE",
		"FULL-SLEEP",
		"NETWORK SLEEP"
	};
#endif
	int status = AH_TRUE;

#ifdef AH_DEBUG
	HALDEBUG(ah, "%s: %s -> %s (%ssleep duration %u)\n", __func__,
		modes[ahp->ah_powerMode], modes[mode],
		setChip ? "set chip " : "", sleepDuration);
#endif
	switch (mode) {
	case HAL_PM_AWAKE:
		status = ar5312SetPowerModeAwake(ah, setChip);
		break;
	case HAL_PM_FULL_SLEEP:
		ar5312SetPowerModeSleep(ah, setChip);
		break;
	case HAL_PM_NETWORK_SLEEP:
		ar5312SetPowerModeNetworkSleep(ah, setChip);
		break;
	default:
		HALDEBUG(ah, "%s: unknown power mode %u\n", __func__, mode);
		return AH_FALSE;
	}
	ahp->ah_powerMode = mode;
	return status; 
}

/*
 * Return the current sleep mode of the chip
 */

u_int32_t
ar5312GetPowerMode(struct ath_hal *ah)
{
        int wlanNum;

        if ((wlanNum = GETWMACNUM(ah)) == -1) {
                return (AH_FALSE);
        }
	/* Just so happens the h/w maps directly to the abstracted value */
	return AR5312_POWER_MODE_NORMAL;
}

/*
 * Return the current sleep state of the chip
 * TRUE = sleeping
 */
HAL_BOOL
ar5312GetPowerStatus(struct ath_hal *ah)
{
        int wlanNum;

        if ((wlanNum = GETWMACNUM(ah)) == -1) {
                return (AH_FALSE);
        }
        return(0);  // Currently, 5312 is never in sleep mode.
}

#endif /* AH_SUPPORT_AR5312 */
