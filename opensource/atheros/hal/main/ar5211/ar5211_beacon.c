/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5211/ar5211_beacon.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5211

#include "ah.h"
#include "ah_internal.h"

#include "ar5211/ar5211.h"
#include "ar5211/ar5211reg.h"
#include "ar5211/ar5211desc.h"

/*
 * Routines used to initialize and generated beacons for the AR5211/AR5311.
 */

/*
 * Initialize all of the hardware registers used to send beacons.
 */
void
ar5211BeaconInit(struct ath_hal *ah,
	u_int32_t next_beacon, u_int32_t beacon_period)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	OS_REG_WRITE(ah, AR_TIMER0, next_beacon);
	/* 
	 * TIMER1: in AP/adhoc mode this controls the DMA beacon
	 * alert timer; otherwise it controls the next wakeup time.
	 * TIMER2: in AP mode, it controls the SBA beacon alert
	 * interrupt; otherwise it sets the start of the next CFP.
	 */
	switch (AH_PRIVATE(ah)->ah_opmode) {
	case HAL_M_STA:
	case HAL_M_MONITOR:
		OS_REG_WRITE(ah, AR_TIMER1, 0xffff);
		OS_REG_WRITE(ah, AR_TIMER2, 0x7ffff);
		break;
	case HAL_M_IBSS:
	case HAL_M_HOSTAP:
		OS_REG_WRITE(ah, AR_TIMER1,
			(next_beacon - ath_hal_dma_beacon_response_time) << 3);
		OS_REG_WRITE(ah, AR_TIMER2, 
			(next_beacon - ath_hal_sw_beacon_response_time) << 3);
		break;
	}
	/*
	 * Set the ATIM window 
	 * Our hardware does not support an ATIM window of 0
	 * (beacons will not work).  If the ATIM windows is 0,
	 * force it to 1.
	 */
	OS_REG_WRITE(ah, AR_TIMER3, next_beacon +
		(ahp->ah_atimWindow ? ahp->ah_atimWindow : 1));

	/*
	 * Set the Beacon register after setting all timers.
	 */
	beacon_period &= AR_BEACON_PERIOD | AR_BEACON_RESET_TSF | AR_BEACON_EN;
	OS_REG_WRITE(ah, AR_BEACON, beacon_period);
}

/*
 * Wait for the beacon and CAB queue to finish.
 */
HAL_BOOL
ar5211WaitForBeaconDone(struct ath_hal *ah, HAL_BUS_ADDR baddr)
{
	int i;

	for (i = 0; i < 1000; i++) {
		/* XXX no check of CAB */
		if (ar5211NumTxPending(ah, HAL_TX_QUEUE_BEACON) == 0)
			break;
		OS_DELAY(10);
	}
	return (i < 1000);
}

void
ar5211ResetStaBeaconTimers(struct ath_hal *ah)
{
	u_int32_t val;

	OS_REG_WRITE(ah, AR_TIMER0, 0);		/* no beacons */
	val = OS_REG_READ(ah, AR_STA_ID1);
	val |= AR_STA_ID1_PWR_SAV;		/* XXX */
	/* tell the h/w that the associated AP is not PCF capable */
	OS_REG_WRITE(ah, AR_STA_ID1,
		val & ~(AR_STA_ID1_DEFAULT_ANTENNA | AR_STA_ID1_PCF));
	OS_REG_WRITE(ah, AR_BEACON, AR_BEACON_PERIOD);
}

/*
 * Set all the beacon related bits on the h/w for stations
 * i.e. initializes the corresponding h/w timers;
 * also tells the h/w whether to anticipate PCF beacons
 */
void
ar5211SetStaBeaconTimers(struct ath_hal *ah, const HAL_BEACON_STATE *bs)
{
	HALDEBUG(ah, "%s: setting beacon timers\n", __func__);

	HALASSERT(bs->bs_intval != 0);
	/* if the AP will do PCF */
	if (bs->bs_cfpmaxduration != 0) {
		/* tell the h/w that the associated AP is PCF capable */
		OS_REG_WRITE(ah, AR_STA_ID1,
			OS_REG_READ(ah, AR_STA_ID1) | AR_STA_ID1_PCF);

		/* set CFP_PERIOD(1.024ms) register */
		OS_REG_WRITE(ah, AR_CFP_PERIOD, bs->bs_cfpperiod);

		/* set CFP_DUR(1.024ms) register to max cfp duration */
		OS_REG_WRITE(ah, AR_CFP_DUR, bs->bs_cfpmaxduration);

		/* set TIMER2(128us) to anticipated time of next CFP */
		OS_REG_WRITE(ah, AR_TIMER2, bs->bs_cfpnext << 3);
	} else {
		/* tell the h/w that the associated AP is not PCF capable */
		OS_REG_WRITE(ah, AR_STA_ID1,
			OS_REG_READ(ah, AR_STA_ID1) &~ AR_STA_ID1_PCF);
	}

	/*
	 * Set TIMER0(1.024ms) to the anticipated time of the next beacon.
	 */
	OS_REG_WRITE(ah, AR_TIMER0, bs->bs_nexttbtt);

	/*
	 * Start the beacon timers by setting the BEACON register
	 * to the beacon interval; also write the tim offset which
	 * we should know by now.  The code, in ar5211WriteAssocid,
	 * also sets the tim offset once the AID is known which can
	 * be left as such for now.
	 */
	OS_REG_WRITE(ah, AR_BEACON, 
		(OS_REG_READ(ah, AR_BEACON) &~ (AR_BEACON_PERIOD|AR_BEACON_TIM))
		| SM(bs->bs_intval, AR_BEACON_PERIOD)
		| SM(bs->bs_timoffset ? bs->bs_timoffset + 4 : 0, AR_BEACON_TIM)
	);

	/*
	 * Configure the BMISS interrupt.  Note that we
	 * assume the caller blocks interrupts while enabling
	 * the threshold.
	 */
	HALASSERT(bs->bs_bmissthreshold <=
		(AR_RSSI_THR_BM_THR >> AR_RSSI_THR_BM_THR_S));
	OS_REG_RMW_FIELD(ah, AR_RSSI_THR,
		AR_RSSI_THR_BM_THR, bs->bs_bmissthreshold);

	/*
	 * Set the sleep duration in 1/8 TU's.
	 */
#define	SLEEP_SLOP	3
	OS_REG_RMW_FIELD(ah, AR_SCR, AR_SCR_SLDUR,
		(bs->bs_sleepduration - SLEEP_SLOP) << 3);
#undef SLEEP_SLOP
}
#endif /* AH_SUPPORT_AR5211 */
