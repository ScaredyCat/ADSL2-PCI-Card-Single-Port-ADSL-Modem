/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_beacon.c#8 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416desc.h"

#define TU_TO_USEC(_tu)		((_tu) << 10)

/*
 * Initializes all of the hardware registers used to
 * send beacons.  Note that for station operation the
 * driver calls ar5416SetStaBeaconTimers instead.
 */
void
ar5416BeaconInit(struct ath_hal *ah,
	u_int32_t next_beacon, u_int32_t beacon_period)
{
	int flags = 0;

	switch (AH_PRIVATE(ah)->ah_opmode) {
	case HAL_M_STA:
	case HAL_M_MONITOR:
		OS_REG_WRITE(ah, AR_NEXT_TBTT_TIMER, TU_TO_USEC(next_beacon));
		OS_REG_WRITE(ah, AR_NEXT_DMA_BEACON_ALERT, 0xffff);
		OS_REG_WRITE(ah, AR_NEXT_SWBA, 0x7ffff);
		flags = AR_TBTT_TIMER_EN;
		break;
	case HAL_M_IBSS:
	case HAL_M_HOSTAP:
		OS_REG_WRITE(ah, AR_NEXT_TBTT_TIMER, TU_TO_USEC(next_beacon));
		OS_REG_WRITE(ah, AR_NEXT_DMA_BEACON_ALERT,
			TU_TO_USEC(next_beacon - ath_hal_dma_beacon_response_time));
		OS_REG_WRITE(ah, AR_NEXT_SWBA,
			TU_TO_USEC(next_beacon - ath_hal_sw_beacon_response_time));
		flags = AR_TBTT_TIMER_EN | AR_DBA_TIMER_EN | AR_SWBA_TIMER_EN;
		break;
	}

	OS_REG_WRITE(ah, AR_BEACON_PERIOD, TU_TO_USEC(beacon_period));
	OS_REG_WRITE(ah, AR_DMA_BEACON_PERIOD, TU_TO_USEC(beacon_period));
	OS_REG_WRITE(ah, AR_SWBA_PERIOD, TU_TO_USEC(beacon_period));

	/* no adhoc power save/atim window in owl */

	/*
	 * reset TSF if required
	 */
	beacon_period &= ~HAL_BEACON_ENA;
	if (beacon_period & HAL_BEACON_RESET_TSF) {
		beacon_period &= ~HAL_BEACON_RESET_TSF;
		ar5416ResetTsf(ah);
	}

	/* enable timers */
	OS_REG_SET_BIT(ah, AR_TIMER_MODE, flags);
}

/*
 * Wait for the beacon and CAB queue to finish.
 */
HAL_BOOL
ar5416WaitForBeaconDone(struct ath_hal *ah, HAL_BUS_ADDR baddr)
{
	int i;

	for (i = 0; i < 1000; i++) {
		/* XXX no check of CAB */
		if (ar5416NumTxPending(ah, HAL_TX_QUEUE_BEACON) == 0)
			break;
		OS_DELAY(10);
	}
	return (i < 1000);
}

#define AR_BEACON_PERIOD_MAX	0xffff
void
ar5416ResetStaBeaconTimers(struct ath_hal *ah)
{
	u_int32_t val;

	OS_REG_WRITE(ah, AR_NEXT_TBTT_TIMER, 0);		/* no beacons */
	val = OS_REG_READ(ah, AR_STA_ID1);
	val |= AR_STA_ID1_PWR_SAV;		/* XXX */
	/* tell the h/w that the associated AP is not PCF capable */
	OS_REG_WRITE(ah, AR_STA_ID1,
		val & ~(AR_STA_ID1_USE_DEFANT | AR_STA_ID1_PCF));
	OS_REG_WRITE(ah, AR_BEACON_PERIOD, AR_BEACON_PERIOD_MAX);
	OS_REG_WRITE(ah, AR_DMA_BEACON_PERIOD, AR_BEACON_PERIOD_MAX);
}

/*
 * Set all the beacon related bits on the h/w for stations
 * i.e. initializes the corresponding h/w timers;
 */
void
ar5416SetStaBeaconTimers(struct ath_hal *ah, const HAL_BEACON_STATE *bs)
{
	u_int32_t nextTbtt, beaconintval, dtimperiod;

	HALASSERT(bs->bs_intval != 0);

	/* no cfp setting since h/w automatically takes care */

    OS_REG_WRITE(ah, AR_NEXT_TBTT_TIMER, bs->bs_nexttbtt);

	/*
	 * Start the beacon timers by setting the BEACON register
	 * to the beacon interval; no need to write tim offset since
	 * h/w parses IEs.
	 */
	OS_REG_WRITE(ah, AR_BEACON_PERIOD,
				 TU_TO_USEC(bs->bs_intval & HAL_BEACON_PERIOD));
	OS_REG_WRITE(ah, AR_DMA_BEACON_PERIOD,
				 TU_TO_USEC(bs->bs_intval & HAL_BEACON_PERIOD));
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
	 * Program the sleep registers to correlate with the beacon setup.
	 */

	/*
	 * Oahu beacons timers on the station were used for power
	 * save operation (waking up in anticipation of a beacon)
	 * and any CFP function; Venice does sleep/power-save timers
	 * differently - so this is the right place to set them up;
	 * don't think the beacon timers are used by venice sta hw
	 * for any useful purpose anymore
	 * Setup venice's sleep related timers
	 * Current implementation assumes sw processing of beacons -
	 *   assuming an interrupt is generated every beacon which
	 *   causes the hardware to become awake until the sw tells
	 *   it to go to sleep again; beacon timeout is to allow for
	 *   beacon jitter; cab timeout is max time to wait for cab
	 *   after seeing the last DTIM or MORE CAB bit
	 */
#define CAB_TIMEOUT_VAL     10 /* in TU */
#define BEACON_TIMEOUT_VAL  10 /* in TU */
#define SLEEP_SLOP          3  /* in TU */

	/*
	 * For max powersave mode we may want to sleep for longer than a
	 * beacon period and not want to receive all beacons; modify the
	 * timers accordingly; make sure to align the next TIM to the
	 * next DTIM if we decide to wake for DTIMs only
	 */
	beaconintval = bs->bs_intval & HAL_BEACON_PERIOD;
	HALASSERT(beaconintval != 0);
	if (bs->bs_sleepduration > beaconintval) {
		HALASSERT(roundup(bs->bs_sleepduration, beaconintval) ==
				bs->bs_sleepduration);
		beaconintval = bs->bs_sleepduration;
	}
	dtimperiod = bs->bs_dtimperiod;
	if (bs->bs_sleepduration > dtimperiod) {
		HALASSERT(dtimperiod == 0 ||
			roundup(bs->bs_sleepduration, dtimperiod) ==
				bs->bs_sleepduration);
		dtimperiod = bs->bs_sleepduration;
	}
	HALASSERT(beaconintval <= dtimperiod);
	if (beaconintval == dtimperiod)
		nextTbtt = bs->bs_nextdtim;
	else
		nextTbtt = bs->bs_nexttbtt;

	HALDEBUG(ah, "%s: next DTIM %d\n", __func__, bs->bs_nextdtim);
	HALDEBUG(ah, "%s: next beacon %d\n", __func__, nextTbtt);
	HALDEBUG(ah, "%s: beacon period %d\n", __func__, beaconintval);
	HALDEBUG(ah, "%s: DTIM period %d\n", __func__, dtimperiod);

	OS_REG_WRITE(ah, AR_NEXT_DTIM, TU_TO_USEC(bs->bs_nextdtim - SLEEP_SLOP));
	OS_REG_WRITE(ah, AR_NEXT_TIM, TU_TO_USEC(nextTbtt - SLEEP_SLOP));

	/* cab timeout is now in 1/8 TU */
	OS_REG_WRITE(ah, AR_SLEEP1,
		SM((CAB_TIMEOUT_VAL << 3), AR_SLEEP1_CAB_TIMEOUT)
		| AR_SLEEP1_ASSUME_DTIM);
	/* beacon timeout is now in 1/8 TU */
	OS_REG_WRITE(ah, AR_SLEEP2,
		SM((BEACON_TIMEOUT_VAL << 3), AR_SLEEP2_BEACON_TIMEOUT));

	OS_REG_WRITE(ah, AR_TIM_PERIOD, beaconintval);
	OS_REG_WRITE(ah, AR_DTIM_PERIOD, dtimperiod);

    OS_REG_SET_BIT(ah, AR_TIMER_MODE, AR_TBTT_TIMER_EN | AR_TIM_TIMER_EN 
                   | AR_DTIM_TIMER_EN);
	
#undef CAB_TIMEOUT_VAL
#undef BEACON_TIMEOUT_VAL
#undef SLEEP_SLOP
}
#endif /* AH_SUPPORT_AR5416 */
