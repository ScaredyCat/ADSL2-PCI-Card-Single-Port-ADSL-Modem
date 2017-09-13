/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_power.c#7 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416desc.h"

HAL_BOOL
ar5416SetPowerModeAwake(struct ath_hal *ah, int setChip)
{
#define POWER_UP_TIME   200000
    u_int32_t val;
    int i;

    if (setChip) {
        OS_REG_SET_BIT(ah, AR_RTC_FORCE_WAKE, AR_RTC_FORCE_WAKE_EN);
        OS_DELAY(10);   /* Give chip the chance to awake */

        for (i = POWER_UP_TIME / 200; i != 0; i--) {
            val = OS_REG_READ(ah, AR_RTC_STATUS) & AR_RTC_STATUS_M;
            if (val == AR_RTC_STATUS_ON)
                break;
            OS_DELAY(200);
            OS_REG_SET_BIT(ah, AR_RTC_FORCE_WAKE, AR_RTC_FORCE_WAKE_EN);
        }
        if (i == 0) {
#ifdef AH_DEBUG
            ath_hal_printf(ah, "%s: Failed to wakeup in %ums\n",
                __func__, POWER_UP_TIME/20);
#endif
            return AH_FALSE;
        }
    }

    return AH_TRUE;
#undef POWER_UP_TIME
}

/*
 * 5416 doesn't support "force_sleep". The MAC goes to sleep automagically
 * when idle.
 */
static void
ar5416ClearPowerModeAwake(struct ath_hal *ah, int setChip)
{
    if (setChip) {
        OS_REG_CLR_BIT(ah, AR_RTC_FORCE_WAKE, AR_RTC_FORCE_WAKE_EN);
    }
}

/*
 * Set power mgt to the requested mode, and conditionally set
 * the chip as well
 */
HAL_BOOL
ar5416SetPowerMode(struct ath_hal *ah, HAL_POWER_MODE mode, int setChip)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
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
    HALDEBUG(ah, "%s: %s -> %s (%s)\n", __func__,
        modes[ahp->ah_powerMode], modes[mode],
        setChip ? "set chip " : "");
#endif
    switch (mode) {
    case HAL_PM_AWAKE:
        status = ar5416SetPowerModeAwake(ah, setChip);
        OS_REG_CLR_BIT(ah, AR_STA_ID1, AR_STA_ID1_PWR_SAV);
        break;
    case HAL_PM_FULL_SLEEP:
    case HAL_PM_NETWORK_SLEEP:
        OS_REG_SET_BIT(ah, AR_STA_ID1, AR_STA_ID1_PWR_SAV);
        ar5416ClearPowerModeAwake(ah, setChip);
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
HAL_POWER_MODE
ar5416GetPowerMode(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    return ahp->ah_powerMode;
}

#endif /* AH_SUPPORT_AR5416 */
