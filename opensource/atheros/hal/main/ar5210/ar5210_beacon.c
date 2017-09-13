/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5210/ar5210_beacon.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5210

#include "ah.h"
#include "ah_internal.h"
#include "ah_desc.h"

#include "ar5210/ar5210.h"
#include "ar5210/ar5210reg.h"
#include "ar5210/ar5210desc.h"

/*
 * Initialize all of the hardware registers used to send beacons.
 */
void
ar5210BeaconInit(struct ath_hal *ah,
	u_int32_t next_beacon, u_int32_t beacon_period)
{
	struct ath_hal_5210 *ahp = AH5210(ah);

	OS_REG_WRITE(ah, AR_TIMER0, next_beacon);

	if (AH_PRIVATE(ah)->ah_opmode != HAL_M_STA) {
		OS_REG_WRITE(ah, AR_TIMER1,
			(next_beacon - ath_hal_dma_beacon_response_time)<<3);
		OS_REG_WRITE(ah, AR_TIMER2,
			(next_beacon - ath_hal_sw_beacon_response_time) <<3);
		/*
		 * The SWBA interrupt is not used for beacons in ad hoc mode
		 * as we don't yet support ATIMs. So since the beacon never
		 * changes, the beacon descriptor is set up once and read
		 * into a special HW buffer, from which it will be
		 * automagically retrieved at each DMA Beacon Alert (DBA).
		 */

		/* Set the ATIM window */
		OS_REG_WRITE(ah, AR_TIMER3, next_beacon + ahp->ah_atimWindow);
	} else {
		OS_REG_WRITE(ah, AR_TIMER1, ~0);
		OS_REG_WRITE(ah, AR_TIMER2, ~0);
		OS_REG_WRITE(ah, AR_TIMER3, 1);
	}

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
ar5210WaitForBeaconDone(struct ath_hal *ah, HAL_BUS_ADDR baddr)
{
	int ticks = 50;			/* wait up to 50 usec for beacon */

	while ((OS_REG_READ(ah, AR_BSR) & AR_BSR_TXQ1F) != 0 ||
	       (OS_REG_READ(ah, AR_CR) & AR_CR_TXE1) != 0) {
		OS_DELAY(1);
		if (--ticks == 0) {
			/*
			 * Avoid a race condition imposed by HW where we need to
			 * ensure that BDMAE is not set if TQ1V is not set. 
			 * Since TQ1V is automatically cleared by HW at each
			 * DBA, we need to make sure both bits are either set or
			 * cleared by the time the next DBA rolls around. If the
			 * DMA engine becomes hung waiting with BDMAE set and
			 * TQ1V clear, we can only become unhung by setting TQ1V
			 * (and ensuring that TXDP1 points to a valid
			 * descriptor).
			 */
			HALDEBUG(ah, "%s: alt queue active at next SWBA\n",
				__func__);
			OS_REG_WRITE(ah, AR_TXDP1, (u_int32_t) baddr);
			OS_REG_WRITE(ah, AR_BCR, AR_BCR_TQ1V | AR_BCR_BDMAE);
			return AH_FALSE;
		}
	}
	return AH_TRUE;
}

void
ar5210ResetStaBeaconTimers(struct ath_hal *ah)
{
	u_int32_t val;

	OS_REG_WRITE(ah, AR_TIMER0, 0);		/* no beacons */
	val = OS_REG_READ(ah, AR_STA_ID1);
	val |= AR_STA_ID1_NO_PSPOLL;		/* XXX */
	/* tell the h/w that the associated AP is not PCF capable */
	OS_REG_WRITE(ah, AR_STA_ID1,
		val & ~(AR_STA_ID1_DEFAULT_ANTENNA | AR_STA_ID1_PCF));
	OS_REG_WRITE(ah, AR_BEACON, AR_BEACON_PERIOD);
}

/*
 * Set all the beacon related bits on the h/w for stations
 * i.e. initializes the corresponding h/w timers;
 * also tells the h/w whether to anticipate PCF beacons
 *
 * dtim_count and cfp_count from the current beacon - their current
 * values aren't necessarily maintained in the device struct
 */
void
ar5210SetStaBeaconTimers(struct ath_hal *ah, const HAL_BEACON_STATE *bs)
{
	HALDEBUG(ah, "%s: setting beacon timers\n", __func__);

	HALASSERT(bs->bs_intval != 0);
	/* if the AP will do PCF */
	if (bs->bs_cfpmaxduration != 0) {
		/* tell the h/w that the associated AP is PCF capable */
		OS_REG_WRITE(ah, AR_STA_ID1,
			(OS_REG_READ(ah, AR_STA_ID1) &~ AR_STA_ID1_DEFAULT_ANTENNA)
			| AR_STA_ID1_PCF);

		/* set CFP_PERIOD(1.024ms) register */
		OS_REG_WRITE(ah, AR_CFP_PERIOD, bs->bs_cfpperiod);

		/* set CFP_DUR(1.024ms) register to max cfp duration */
		OS_REG_WRITE(ah, AR_CFP_DUR, bs->bs_cfpmaxduration);

		/* set TIMER2(128us) to anticipated time of next CFP */
		OS_REG_WRITE(ah, AR_TIMER2, bs->bs_cfpnext << 3);
	} else {
		/* tell the h/w that the associated AP is not PCF capable */
		OS_REG_WRITE(ah, AR_STA_ID1,
			OS_REG_READ(ah, AR_STA_ID1) &~ (AR_STA_ID1_DEFAULT_ANTENNA | AR_STA_ID1_PCF));
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

	/*
	 * Interrupt works only on Crete.
	 */
	if (AH_PRIVATE(ah)->ah_macRev < AR_SREV_CRETE)
		return;
	/*
	 * Counter is only 3-bits.
	 * Count of 0 with BMISS interrupt enabled will hang the system
	 * with too many interrupts
	 */
	if (AH_PRIVATE(ah)->ah_macRev >= AR_SREV_CRETE &&
	    (bs->bs_bmissthreshold&7) == 0) {
#ifdef AH_DEBUG
		ath_hal_printf(ah, "%s: invalid beacon miss threshold %u\n",
			__func__, bs->bs_bmissthreshold);
#endif
		return;
	}
	HALASSERT(bs->bs_bmissthreshold <=
		(AR_RSSI_THR_BM_THR >> AR_RSSI_THR_BM_THR_S));
	OS_REG_RMW_FIELD(ah, AR_RSSI_THR,
		AR_RSSI_THR_BM_THR, bs->bs_bmissthreshold);
}
#endif /* AH_SUPPORT_AR5210 */
