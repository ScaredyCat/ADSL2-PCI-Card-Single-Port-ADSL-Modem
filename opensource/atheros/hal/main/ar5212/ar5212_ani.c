/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5212/ar5212_ani.c#2 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#include "ah.h"
#include "ah_internal.h"
#include "ah_desc.h"

#include "ar5212/ar5212.h"
#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212phy.h"

/*
 * Anti noise immunity support.  We track phy errors and react
 * to excessive errors by adjusting the noise immunity parameters.
 */

/******************************************************************************
 *
 * New Ani Algorithm for Station side only
 *
 *****************************************************************************/

#define HAL_NOISE_IMMUNE_MAX		4	/* Max noise immunity level */
#define HAL_SPUR_IMMUNE_MAX		7	/* Max spur immunity level */
#define HAL_FIRST_STEP_MAX		2	/* Max first step level */

#define HAL_ANI_OFDM_TRIG_HIGH		500
#define HAL_ANI_OFDM_TRIG_LOW		200
#define HAL_ANI_CCK_TRIG_HIGH		200
#define HAL_ANI_CCK_TRIG_LOW		100
#define HAL_ANI_NOISE_IMMUNE_LVL	4
#define HAL_ANI_USE_OFDM_WEAK_SIG	AH_TRUE
#define HAL_ANI_CCK_WEAK_SIG_THR	AH_FALSE
#define HAL_ANI_SPUR_IMMUNE_LVL		7
#define HAL_ANI_FIRSTEP_LVL		0
#define HAL_ANI_RSSI_THR_HIGH		40
#define HAL_ANI_RSSI_THR_LOW		7
#define HAL_ANI_PERIOD			100

#define HAL_EP_RND(x, mul) \
	((((x)%(mul)) >= ((mul)/2)) ? ((x) + ((mul) - 1)) / (mul) : (x)/(mul))
#define	BEACON_RSSI(ahp) \
	HAL_EP_RND(ahp->ah_stats.ast_nodestats.ns_avgbrssi, \
		HAL_RSSI_EP_MULTIPLIER)

#define	ah_mibStats	ah_stats.ast_mibstats

void
ar5212EnableMIBCounters(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	HALDEBUG(ah, "Enable mib counters\n");
	/* Clear the mib counters and save them in the stats */
	ar5212UpdateMibCounters(ah, &ahp->ah_mibStats);
	
	OS_REG_WRITE(ah, AR_FILTOFDM, 0);
	OS_REG_WRITE(ah, AR_FILTCCK, 0);
	OS_REG_WRITE(ah, AR_MIBC, 
		     ~(AR_MIBC_COW | AR_MIBC_FMC | AR_MIBC_CMC | AR_MIBC_MCS)
		     & 0x0f);
	OS_REG_WRITE(ah, AR_PHYCNTMASK1, AR_PHY_ERR_OFDM_TIMING);
	OS_REG_WRITE(ah, AR_PHYCNTMASK2, AR_PHY_ERR_CCK_TIMING);

}

void 
ar5212DisableMIBCounters(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	HALDEBUG(ah, "Disabling MIB counters\n");

	OS_REG_WRITE(ah, AR_MIBC,  AR_MIBC_FMC | AR_MIBC_CMC);
	
	/* Clear the mib counters and save them in the stats */
	ar5212UpdateMibCounters(ah, &ahp->ah_mibStats);
	
	OS_REG_WRITE(ah, AR_FILTOFDM, 0);
	OS_REG_WRITE(ah, AR_FILTCCK, 0);
}

/*
 * This routine returns the index into the aniState array that
 * corresponds to the channel in *chan.  If no match is found and the
 * array is still not fully utilized, a new entry is created for the
 * channel.  We assume the attach function has already initialized the
 * ah_ani values and only the channel field needs to be set.
 */
static int
ar5212GetAniChannelIndex(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
#define N(a)     (sizeof(a) / sizeof(a[0]))
	struct ath_hal_5212 *ahp = AH5212(ah);
	int i;

	for (i = 0; i < N(ahp->ah_ani); i++) {
		if (ahp->ah_ani[i].c.channel == chan->channel)
			return i;
		if (ahp->ah_ani[i].c.channel == 0) {
			ahp->ah_ani[i].c.channel = chan->channel;
			ahp->ah_ani[i].c.channelFlags = chan->channelFlags;
			return i;
		}
	}
	/* XXX statistic */
	HALDEBUG(ah, "No more channel states left. Using channel 0\n");
	return 0;		/* XXX gotta return something valid */
#undef N
}

/*
 * Return the current ANI state of the channel we're on
 */
struct ar5212AniState *
ar5212AniGetCurrentState(struct ath_hal *ah)
{
	return AH5212(ah)->ah_curani;
}

/*
 * Return the current statistics.
 */
struct ar5212Stats *
ar5212AniGetCurrentStats(struct ath_hal *ah)
{
	return &AH5212(ah)->ah_stats;
}

/*
 * Setup ANI handling.  Sets all thresholds and levels to default level AND
 * resets the channel statistics
 */

void
ar5212AniAttach(struct ath_hal *ah)
{
#define N(a)     (sizeof(a) / sizeof(a[0]))
	struct ath_hal_5212 *ahp = AH5212(ah);
	int i;

	ahp->ah_hasHwPhyCounters =
	    (AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_VENICE &&
	    AH_PRIVATE(ah)->ah_macRev == AR_SREV_HAINAN);

	OS_MEMZERO(ahp->ah_ani, sizeof(ahp->ah_ani));
	for (i = 0; i < N(ahp->ah_ani); i++) {
		/* New ANI stuff */
		ahp->ah_ani[i].ofdmTrigHigh = HAL_ANI_OFDM_TRIG_HIGH;
		ahp->ah_ani[i].ofdmTrigLow = HAL_ANI_OFDM_TRIG_LOW;
		ahp->ah_ani[i].cckTrigHigh = HAL_ANI_CCK_TRIG_HIGH;
		ahp->ah_ani[i].cckTrigLow = HAL_ANI_CCK_TRIG_LOW;
		ahp->ah_ani[i].rssiThrHigh = HAL_ANI_RSSI_THR_HIGH;
		ahp->ah_ani[i].rssiThrLow = HAL_ANI_RSSI_THR_LOW;
		ahp->ah_ani[i].ofdmWeakSigDetectOff = !HAL_ANI_USE_OFDM_WEAK_SIG;
		ahp->ah_ani[i].cckWeakSigThreshold = HAL_ANI_CCK_WEAK_SIG_THR;
		ahp->ah_ani[i].spurImmunityLevel = HAL_ANI_SPUR_IMMUNE_LVL;
		ahp->ah_ani[i].firstepLevel = HAL_ANI_FIRSTEP_LVL;
		if (ahp->ah_hasHwPhyCounters) {
			ahp->ah_ani[i].ofdmPhyErrBase = 
				AR_PHY_COUNTMAX - HAL_ANI_OFDM_TRIG_HIGH;
			ahp->ah_ani[i].cckPhyErrBase = 
				AR_PHY_COUNTMAX - HAL_ANI_CCK_TRIG_HIGH;
		}
	}
	if (ahp->ah_hasHwPhyCounters) {
		HALDEBUG(ah, "Setting OfdmErrBase = 0x%08x\n",
			 ahp->ah_ani[0].ofdmPhyErrBase);
		HALDEBUG(ah, "Setting cckErrBase = 0x%08x\n",
			 ahp->ah_ani[0].cckPhyErrBase);
		/* Enable MIB Counters */
		OS_REG_WRITE(ah, AR_PHYCNT1, ahp->ah_ani[0].ofdmPhyErrBase);
		OS_REG_WRITE(ah, AR_PHYCNT2, ahp->ah_ani[0].cckPhyErrBase);
		ar5212EnableMIBCounters(ah);
	}
	ahp->ah_aniPeriod = HAL_ANI_PERIOD;
	ahp->ah_procPhyErr |= HAL_PROCESS_ANI;  /* Enable ani by default */
#undef N
}

/*
 * Cleanup any ANI state setup.
 */
void
ar5212AniDetach(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	HALDEBUG(ah, "Detaching Ani\n");
	if (ahp->ah_hasHwPhyCounters) {
		ar5212DisableMIBCounters(ah);
		OS_REG_WRITE(ah, AR_PHYCNT1, 0);
		OS_REG_WRITE(ah, AR_PHYCNT2, 0);
	}
}

/*
 * Control Adaptive Noise Immunity Parameters
 */
HAL_BOOL
ar5212AniControl(struct ath_hal *ah, HAL_ANI_CMD cmd, int param)
{
#define N(a) (sizeof(a)/sizeof(a[0]))
	typedef int TABLE[];
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212AniState *aniState = ahp->ah_curani;

	switch (cmd) {
	case HAL_ANI_NOISE_IMMUNITY_LEVEL: {
		u_int level = param;

		if (level >= N(ahp->ah_totalSizeDesired)) {
			HALDEBUG(ah, "%s: level out of range (%u > %u)\n",
				__func__, level, N(ahp->ah_totalSizeDesired));
			return AH_FALSE;
		}

		OS_REG_RMW_FIELD(ah, AR_PHY_DESIRED_SZ,
			AR_PHY_DESIRED_SZ_TOT_DES, ahp->ah_totalSizeDesired[level]);
		OS_REG_RMW_FIELD(ah, AR_PHY_AGC_CTL1,
			  AR_PHY_AGC_CTL1_COARSE_LOW, ahp->ah_coarseLow[level]);
		OS_REG_RMW_FIELD(ah, AR_PHY_AGC_CTL1,
			  AR_PHY_AGC_CTL1_COARSE_HIGH, ahp->ah_coarseHigh[level]);
		OS_REG_RMW_FIELD(ah, AR_PHY_FIND_SIG,
			AR_PHY_FIND_SIG_FIRPWR, ahp->ah_firpwr[level]);

		if (level > aniState->noiseImmunityLevel)
			ahp->ah_stats.ast_ani_niup++;
		else if (level < aniState->noiseImmunityLevel)
			ahp->ah_stats.ast_ani_nidown++;
		aniState->noiseImmunityLevel = level;
		break;
	}
	case HAL_ANI_OFDM_WEAK_SIGNAL_DETECTION: {
		const TABLE m1ThreshLow   = { 127,   50 };
		const TABLE m2ThreshLow   = { 127,   40 };
		const TABLE m1Thresh      = { 127, 0x4d };
		const TABLE m2Thresh      = { 127, 0x40 };
		const TABLE m2CountThr    = {  31,   16 };
		const TABLE m2CountThrLow = {  63,   48 };
		u_int on = param ? 1 : 0;

		OS_REG_RMW_FIELD(ah, AR_PHY_SFCORR_LOW,
			AR_PHY_SFCORR_LOW_M1_THRESH_LOW, m1ThreshLow[on]);
		OS_REG_RMW_FIELD(ah, AR_PHY_SFCORR_LOW,
			AR_PHY_SFCORR_LOW_M2_THRESH_LOW, m2ThreshLow[on]);
		OS_REG_RMW_FIELD(ah, AR_PHY_SFCORR,
			AR_PHY_SFCORR_M1_THRESH, m1Thresh[on]);
		OS_REG_RMW_FIELD(ah, AR_PHY_SFCORR,
			AR_PHY_SFCORR_M2_THRESH, m2Thresh[on]);
		OS_REG_RMW_FIELD(ah, AR_PHY_SFCORR,
			AR_PHY_SFCORR_M2COUNT_THR, m2CountThr[on]);
		OS_REG_RMW_FIELD(ah, AR_PHY_SFCORR_LOW,
			AR_PHY_SFCORR_LOW_M2COUNT_THR_LOW, m2CountThrLow[on]);

		if (on) {
			OS_REG_SET_BIT(ah, AR_PHY_SFCORR_LOW,
				AR_PHY_SFCORR_LOW_USE_SELF_CORR_LOW);
		} else {
			OS_REG_CLR_BIT(ah, AR_PHY_SFCORR_LOW,
				AR_PHY_SFCORR_LOW_USE_SELF_CORR_LOW);
		}
		if (!on != aniState->ofdmWeakSigDetectOff) {
			if (on)
				ahp->ah_stats.ast_ani_ofdmon++;
			else
				ahp->ah_stats.ast_ani_ofdmoff++;
			aniState->ofdmWeakSigDetectOff = !on;
		}
		break;
	}
	case HAL_ANI_CCK_WEAK_SIGNAL_THR: {
		const TABLE weakSigThrCck = { 8, 6 };
		u_int high = param ? 1 : 0;

		OS_REG_RMW_FIELD(ah, AR_PHY_CCK_DETECT,
			AR_PHY_CCK_DETECT_WEAK_SIG_THR_CCK,
			weakSigThrCck[high]);
		if (high != aniState->cckWeakSigThreshold) {
			if (high)
				ahp->ah_stats.ast_ani_cckhigh++;
			else
				ahp->ah_stats.ast_ani_ccklow++;
			aniState->cckWeakSigThreshold = high;
		}
		break;
	}
	case HAL_ANI_FIRSTEP_LEVEL: {
		const TABLE firstep = { 0, 4, 8 };
		u_int level = param;

		if (level >= N(firstep)) {
			HALDEBUG(ah, "%s: level out of range (%u > %u)\n",
				__func__, level, N(firstep));
			return AH_FALSE;
		}
		OS_REG_RMW_FIELD(ah, AR_PHY_FIND_SIG,
			AR_PHY_FIND_SIG_FIRSTEP, firstep[level]);
		if (level > aniState->firstepLevel)
			ahp->ah_stats.ast_ani_stepup++;
		else if (level < aniState->firstepLevel)
			ahp->ah_stats.ast_ani_stepdown++;
		aniState->firstepLevel = level;
		break;
	}
	case HAL_ANI_SPUR_IMMUNITY_LEVEL: {
		const TABLE cycpwrThr1 = { 2, 4, 6, 8, 10, 12, 14, 16 };
		u_int level = param;

		if (level >= N(cycpwrThr1)) {
			HALDEBUG(ah, "%s: level out of range (%u > %u)\n",
				__func__, level, N(cycpwrThr1));
			return AH_FALSE;
		}
		OS_REG_RMW_FIELD(ah, AR_PHY_TIMING5,
			AR_PHY_TIMING5_CYCPWR_THR1, cycpwrThr1[level]);
		if (level > aniState->spurImmunityLevel)
			ahp->ah_stats.ast_ani_spurup++;
		else if (level < aniState->spurImmunityLevel)
			ahp->ah_stats.ast_ani_spurdown++;
		aniState->spurImmunityLevel = level;
		break;
	}
	case HAL_ANI_PRESENT:
		break;
#ifdef AH_PRIVATE_DIAG
	case HAL_ANI_MODE:
		if (param == 0) {
			ahp->ah_procPhyErr &= ~HAL_PROCESS_ANI;
			/* Turn off HW counters if we have them */
			ar5212AniDetach(ah);
			ar5212SetRxFilter(ah,
				ar5212GetRxFilter(ah) &~ HAL_RX_FILTER_PHYERR);
		} else {			/* normal/auto mode */
			ahp->ah_procPhyErr |= HAL_PROCESS_ANI;
			if (ahp->ah_hasHwPhyCounters) {
				ar5212SetRxFilter(ah,
					ar5212GetRxFilter(ah) &~ HAL_RX_FILTER_PHYERR);
			} else {
				ar5212SetRxFilter(ah,
					ar5212GetRxFilter(ah) | HAL_RX_FILTER_PHYERR);
			}
		}
		break;
	case HAL_ANI_PHYERR_RESET:
		ahp->ah_stats.ast_ani_ofdmerrs = 0;
		ahp->ah_stats.ast_ani_cckerrs = 0;
		break;
#endif /* AH_PRIVATE_DIAG */
	default:
		HALDEBUG(ah, "%s: invalid cmd %u\n", __func__, cmd);
		return AH_FALSE;
	}
	return AH_TRUE;
#undef	N
}

static void
ar5212AniRestart(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212AniState *aniState;

	aniState = ahp->ah_curani;

	aniState->listenTime = 0;
	if (ahp->ah_hasHwPhyCounters) {
		if (aniState->ofdmTrigHigh > AR_PHY_COUNTMAX) {
			aniState->ofdmPhyErrBase = 0;
			HALDEBUG(ah, "OFDM Trigger is too high for hw counters\n");
		} else
			aniState->ofdmPhyErrBase =
				AR_PHY_COUNTMAX - aniState->ofdmTrigHigh;
		if (aniState->cckTrigHigh > AR_PHY_COUNTMAX) {
			aniState->cckPhyErrBase = 0;
			HALDEBUG(ah, "CCK Trigger is too high for hw counters\n");
		} else
			aniState->cckPhyErrBase =
				AR_PHY_COUNTMAX - aniState->cckTrigHigh;
		HALDEBUG(ah, "%s: Writing ofdmbase=%u   cckbase=%u\n", __func__,
			 aniState->ofdmPhyErrBase, aniState->cckPhyErrBase);
		OS_REG_WRITE(ah, AR_PHYCNT1, aniState->ofdmPhyErrBase);
		OS_REG_WRITE(ah, AR_PHYCNT2, aniState->cckPhyErrBase);
		OS_REG_WRITE(ah, AR_PHYCNTMASK1, AR_PHY_ERR_OFDM_TIMING);
		OS_REG_WRITE(ah, AR_PHYCNTMASK2, AR_PHY_ERR_CCK_TIMING);

		/* Clear the mib counters and save them in the stats */
		ar5212UpdateMibCounters(ah, &ahp->ah_mibStats);
	}
	aniState->ofdmPhyErrCount = 0;
	aniState->cckPhyErrCount = 0;
}

static void
ar5212AniOfdmErrTrigger(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CHANNEL_INTERNAL *chan = AH_PRIVATE(ah)->ah_curchan;
	struct ar5212AniState *aniState;
	WIRELESS_MODE mode;
	int32_t rssi;

	HALASSERT(chan != AH_NULL);

	if (!DO_ANI(ah))
		return;

	aniState = ahp->ah_curani;
	/* First, raise noise immunity level, up to max */
	if (aniState->noiseImmunityLevel < HAL_NOISE_IMMUNE_MAX) {
		ar5212AniControl(ah, HAL_ANI_NOISE_IMMUNITY_LEVEL, 
				 aniState->noiseImmunityLevel + 1);
		return;
	}
	/* then, raise spur immunity level, up to max */
	if (aniState->spurImmunityLevel < HAL_SPUR_IMMUNE_MAX) {
		ar5212AniControl(ah, HAL_ANI_SPUR_IMMUNITY_LEVEL,
				 aniState->spurImmunityLevel + 1);
		return;
	}
	rssi = BEACON_RSSI(ahp);
	if (rssi > aniState->rssiThrHigh) {
		/*
		 * Beacon rssi is high, can turn off ofdm weak sig detect.
		 */
		if (!aniState->ofdmWeakSigDetectOff) {
			ar5212AniControl(ah, HAL_ANI_OFDM_WEAK_SIGNAL_DETECTION,
					 AH_FALSE);
			ar5212AniControl(ah, HAL_ANI_SPUR_IMMUNITY_LEVEL, 0);
			return;
		}
		/* 
		 * If weak sig detect is already off, as last resort, raise
		 * first step level 
		 */
		if (aniState->firstepLevel < HAL_FIRST_STEP_MAX) {
			ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL,
					 aniState->firstepLevel + 1);
			return;
		}
	} else if (rssi > aniState->rssiThrLow) {
		/* 
		 * Beacon rssi in mid range, need ofdm weak signal detect,
		 * but we can raise firststepLevel 
		 */
		if (aniState->ofdmWeakSigDetectOff)
			ar5212AniControl(ah, HAL_ANI_OFDM_WEAK_SIGNAL_DETECTION,
					 AH_TRUE);
		if (aniState->firstepLevel < HAL_FIRST_STEP_MAX)
			ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL,
					 aniState->firstepLevel + 1);
		return;
	} else {
		/* 
		 * Beacon rssi is low, if in 11b/g mode, turn off ofdm
		 * weak sign detction and zero firstepLevel to maximize
		 * CCK sensitivity 
		 */
		mode = ath_hal_chan2wmode(ah, (HAL_CHANNEL *) chan);
		if (mode == WIRELESS_MODE_11g || mode == WIRELESS_MODE_11b) {
			if (!aniState->ofdmWeakSigDetectOff)
				ar5212AniControl(ah, HAL_ANI_OFDM_WEAK_SIGNAL_DETECTION,
						 AH_FALSE);
			if (aniState->firstepLevel > 0)
				ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL, 0);
			return;
		}
	}
}

static void
ar5212AniCckErrTrigger(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CHANNEL_INTERNAL *chan = AH_PRIVATE(ah)->ah_curchan;
	struct ar5212AniState *aniState;
	WIRELESS_MODE mode;
	int32_t rssi;

	HALASSERT(chan != AH_NULL);

	if (!DO_ANI(ah))
		return;

	/* first, raise noise immunity level, up to max */
	aniState = ahp->ah_curani;
	if (aniState->noiseImmunityLevel < HAL_NOISE_IMMUNE_MAX) {
		ar5212AniControl(ah, HAL_ANI_NOISE_IMMUNITY_LEVEL,
				 aniState->noiseImmunityLevel + 1);
		return;
	}
	rssi = BEACON_RSSI(ahp);
	if (rssi >  aniState->rssiThrLow) {
		/*
		 * Beacon signal in mid and high range, raise firsteplevel.
		 */
		if (aniState->firstepLevel < HAL_FIRST_STEP_MAX)
			ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL,
					 aniState->firstepLevel + 1);
	} else {
		/*
		 * Beacon rssi is low, zero firstepLevel to maximize
		 * CCK sensitivity.
		 */
		mode = ath_hal_chan2wmode(ah, (HAL_CHANNEL *) chan);
		if (mode == WIRELESS_MODE_11g || mode == WIRELESS_MODE_11b) {
			if (aniState->firstepLevel > 0)
				ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL, 0);
		}
	}
}

/*
 * Restore the ANI parameters in the HAL and reset the statistics.
 * This routine should be called for every hardware reset and for
 * every channel change.  NOTE: This must be called for every channel
 * change for ah_curani to be set correctly.
 */
void
ar5212AniReset(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212AniState *aniState;
	HAL_CHANNEL_INTERNAL *chan = AH_PRIVATE(ah)->ah_curchan;
	int index;

	HALASSERT(chan != AH_NULL);

	index = ar5212GetAniChannelIndex(ah, chan);
	aniState = &ahp->ah_ani[index];
	ahp->ah_curani = aniState;

	/* If ANI follows hardware, we don't care what mode we're
	   in, we should keep the ani parameters */
	/*
	 * ANI is enabled but we're not operating in station
	 * mode.  Reset all parameters.  This can happen, for
	 * example, when starting up AP operation.
	 */
	if (DO_ANI(ah) && AH_PRIVATE(ah)->ah_opmode != HAL_M_STA) {
		HALDEBUG(ah,"%s: Reset ANI state opmode %u\n",
			__func__, AH_PRIVATE(ah)->ah_opmode);
		ahp->ah_stats.ast_ani_reset++;
		ar5212AniControl(ah, HAL_ANI_NOISE_IMMUNITY_LEVEL, 0);
		ar5212AniControl(ah, HAL_ANI_SPUR_IMMUNITY_LEVEL, 0);
		ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL, 0);
		ar5212AniControl(ah, HAL_ANI_OFDM_WEAK_SIGNAL_DETECTION,
			!HAL_ANI_USE_OFDM_WEAK_SIG);
		ar5212AniControl(ah, HAL_ANI_CCK_WEAK_SIGNAL_THR,
			HAL_ANI_CCK_WEAK_SIG_THR);
		/* NB: enable phy frames so the driver gets stats */
		ar5212SetRxFilter(ah,
			ar5212GetRxFilter(ah) | HAL_RX_FILTER_PHYERR);
		ar5212AniRestart(ah);
		return;
	}

	if (aniState->noiseImmunityLevel != 0)
		ar5212AniControl(ah, HAL_ANI_NOISE_IMMUNITY_LEVEL,
				 aniState->noiseImmunityLevel);
	if (aniState->spurImmunityLevel != 0)
		ar5212AniControl(ah, HAL_ANI_SPUR_IMMUNITY_LEVEL,
				 aniState->spurImmunityLevel);
	if (aniState->ofdmWeakSigDetectOff)
		ar5212AniControl(ah, HAL_ANI_OFDM_WEAK_SIGNAL_DETECTION,
				 !aniState->ofdmWeakSigDetectOff);
	if (aniState->cckWeakSigThreshold)
		ar5212AniControl(ah, HAL_ANI_CCK_WEAK_SIGNAL_THR,
				 aniState->cckWeakSigThreshold);
	if (aniState->firstepLevel != 0)
		ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL, aniState->firstepLevel);
	if (ahp->ah_hasHwPhyCounters) {
		ar5212SetRxFilter(ah,
			  ar5212GetRxFilter(ah) &~ HAL_RX_FILTER_PHYERR);
		ar5212AniRestart(ah);
		OS_REG_WRITE(ah, AR_PHYCNTMASK1, AR_PHY_ERR_OFDM_TIMING);
		OS_REG_WRITE(ah, AR_PHYCNTMASK2, AR_PHY_ERR_CCK_TIMING);

	} else {
		ar5212AniRestart(ah);
		ar5212SetRxFilter(ah, ar5212GetRxFilter(ah) | HAL_RX_FILTER_PHYERR);
	}
}

/*
 * Process a MIB interrupt.  We may potentially be invoked because
 * any of the MIB counters overflow/trigger so don't assume we're
 * here because a PHY error counter triggered.
 */
void
ar5212ProcessMibIntr(struct ath_hal *ah, const HAL_NODE_STATS *stats)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	u_int32_t phyCnt1, phyCnt2;

	HALDEBUG(ah, "Processing Mib Intr\n");
	/* Reset these counters regardless */
	OS_REG_WRITE(ah, AR_FILTOFDM, 0);
	OS_REG_WRITE(ah, AR_FILTCCK, 0);

	/* Clear the mib counters and save them in the stats */
	ar5212UpdateMibCounters(ah, &ahp->ah_mibStats);
	ahp->ah_stats.ast_nodestats = *stats;

	/* NB: these are not reset-on-read */
	phyCnt1 = OS_REG_READ(ah, AR_PHYCNT1);
	phyCnt2 = OS_REG_READ(ah, AR_PHYCNT2);
	if (((phyCnt1 & AR_MIBCNT_INTRMASK) == AR_MIBCNT_INTRMASK) || 
	    ((phyCnt2 & AR_MIBCNT_INTRMASK) == AR_MIBCNT_INTRMASK)) {
		struct ar5212AniState *aniState = ahp->ah_curani;
		u_int32_t ofdmPhyErrCnt, cckPhyErrCnt;

		/* NB: only use ast_ani_*errs with AH_PRIVATE_DIAG */
		ofdmPhyErrCnt = phyCnt1 - aniState->ofdmPhyErrBase;
		ahp->ah_stats.ast_ani_ofdmerrs +=
			ofdmPhyErrCnt - aniState->ofdmPhyErrCount;
		aniState->ofdmPhyErrCount = ofdmPhyErrCnt;

		cckPhyErrCnt = phyCnt2 - aniState->cckPhyErrBase;
		ahp->ah_stats.ast_ani_cckerrs +=
			cckPhyErrCnt - aniState->cckPhyErrCount;
		aniState->cckPhyErrCount = cckPhyErrCnt;

		/*
		 * NB: figure out which counter triggered.  If both
		 * trigger we'll only deal with one as the processing
		 * clobbers the error counter so the trigger threshold
		 * check will never be true.
		 */
		if (aniState->ofdmPhyErrCount > aniState->ofdmTrigHigh)
			ar5212AniOfdmErrTrigger(ah);
		if (aniState->cckPhyErrCount > aniState->cckTrigHigh)
			ar5212AniCckErrTrigger(ah);
		/* NB: always restart to insure the h/w counters are reset */
		ar5212AniRestart(ah);
	}
}

void 
ar5212AniPhyErrReport(struct ath_hal *ah, const struct ath_rx_status *rs)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212AniState *aniState;

	HALASSERT(!ahp->ah_hasHwPhyCounters && rs != AH_NULL);

	aniState = ahp->ah_curani;
	if (rs->rs_phyerr == HAL_PHYERR_OFDM_TIMING) {
		aniState->ofdmPhyErrCount++;
		ahp->ah_stats.ast_ani_ofdmerrs++;
		if (aniState->ofdmPhyErrCount > aniState->ofdmTrigHigh) {
			ar5212AniOfdmErrTrigger(ah);
			ar5212AniRestart(ah);
		}
	} else if (rs->rs_phyerr == HAL_PHYERR_CCK_TIMING) {
		aniState->cckPhyErrCount++;
		ahp->ah_stats.ast_ani_cckerrs++;
		if (aniState->cckPhyErrCount > aniState->cckTrigHigh) {
			ar5212AniCckErrTrigger(ah);
			ar5212AniRestart(ah);
		}
	}
}

static void
ar5212AniLowerImmunity(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212AniState *aniState;
	int32_t rssi;
	
	aniState = ahp->ah_curani;
	
	rssi = BEACON_RSSI(ahp);
	if (rssi > aniState->rssiThrHigh) {
		/* 
		 * Beacon signal is high, leave ofdm weak signal detection off
		 * or it may oscillate. Let it fall through.
		 */
	} else if (rssi > aniState->rssiThrLow) {
		/*
		 * Beacon rssi in mid range, turn on ofdm weak signal
		 * detection or lower first step level.
		 */
		if (aniState->ofdmWeakSigDetectOff) {
			ar5212AniControl(ah, HAL_ANI_OFDM_WEAK_SIGNAL_DETECTION,
					 AH_TRUE);
			return;
		}
		if (aniState->firstepLevel > 0) {
			ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL,
					 aniState->firstepLevel - 1);
			return;
		}
	} else {
		/*
		 * Beacon rssi is low, reduce first step level.
		 */
		if (aniState->firstepLevel > 0) {
			ar5212AniControl(ah, HAL_ANI_FIRSTEP_LEVEL,
					 aniState->firstepLevel - 1);
			return;
		}
	}
	/* then lower spur immunity level, down to zero */
	if (aniState->spurImmunityLevel > 0) {
		ar5212AniControl(ah, HAL_ANI_SPUR_IMMUNITY_LEVEL,
				 aniState->spurImmunityLevel - 1);
		return;
	}
	/* 
	 * if all else fails, lower noise immunity level down to a min value
	 * zero for now
	 */
	if (aniState->noiseImmunityLevel > 0) {
		ar5212AniControl(ah, HAL_ANI_NOISE_IMMUNITY_LEVEL,
				 aniState->noiseImmunityLevel - 1);
		return;
	}
}

#define CLOCK_RATE 44000	/* XXX use mac_usec or similar */
/* convert HW counter values to ms using 11g clock rate, goo9d enough
   for 11a and Turbo */

/* 
 * Return an approximation of the time spent ``listening'' by
 * deducting the cycles spent tx'ing and rx'ing from the total
 * cycle count since our last call.  A return value <0 indicates
 * an invalid/inconsistent time.
 */
static int32_t
ar5212AniGetListenTime(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212AniState *aniState;
	u_int32_t txFrameCount, rxFrameCount, cycleCount;
	int32_t listenTime;

	txFrameCount = OS_REG_READ(ah, AR_TFCNT);
	rxFrameCount = OS_REG_READ(ah, AR_RFCNT);
	cycleCount = OS_REG_READ(ah, AR_CCCNT);

	aniState = ahp->ah_curani;
	if (aniState->cycleCount == 0 || aniState->cycleCount > cycleCount) {
		/*
		 * Cycle counter wrap (or initial call); it's not possible
		 * to accurately calculate a value because the registers
		 * right shift rather than wrap--so punt and return 0.
		 */
		listenTime = 0;
		ahp->ah_stats.ast_ani_lzero++;
	} else {
		int32_t ccdelta = cycleCount - aniState->cycleCount;
		int32_t rfdelta = rxFrameCount - aniState->rxFrameCount;
		int32_t tfdelta = txFrameCount - aniState->txFrameCount;
		listenTime = (ccdelta - rfdelta - tfdelta) / CLOCK_RATE;
	}
	aniState->cycleCount = cycleCount;
	aniState->txFrameCount = txFrameCount;
	aniState->rxFrameCount = rxFrameCount;
	return listenTime;
}

/*
 * Do periodic processing.  This routine is called from the
 * driver's rx interrupt handler after processing frames.
 */
void
ar5212AniArPoll(struct ath_hal *ah, const HAL_NODE_STATS *stats,
		HAL_CHANNEL *chan)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212AniState *aniState;
	int32_t listenTime;

	/* 
 	 * Since we're called from end of rx tasklet, we also check for
 	 * AR processing now
 	 */
 
 	if ((ahp->ah_procPhyErr & HAL_AR_EN) &&
 	    (((AH_PRIVATE(ah)->ah_curchan)->channelFlags) & CHANNEL_108G))
 		ar5212ProcessArEvent(ah, chan);

	aniState = ahp->ah_curani;
	ahp->ah_stats.ast_nodestats = *stats;		/* XXX optimize? */

	listenTime = ar5212AniGetListenTime(ah);
	if (listenTime < 0) {
		ahp->ah_stats.ast_ani_lneg++;
		/* restart ANI period if listenTime is invalid */
		ar5212AniRestart(ah);
		return;
	}
	/* XXX beware of overflow? */
	aniState->listenTime += listenTime;

	if (ahp->ah_hasHwPhyCounters) {
		u_int32_t phyCnt1, phyCnt2;
		u_int32_t ofdmPhyErrCnt, cckPhyErrCnt;

		/* Clear the mib counters and save them in the stats */
		ar5212UpdateMibCounters(ah, &ahp->ah_mibStats);
		/* NB: these are not reset-on-read */
		phyCnt1 = OS_REG_READ(ah, AR_PHYCNT1);
		phyCnt2 = OS_REG_READ(ah, AR_PHYCNT2);
		/* XXX sometimes zero, why? */
		if (phyCnt1 < aniState->ofdmPhyErrBase ||
		    phyCnt2 < aniState->cckPhyErrBase) {
			if (phyCnt1 < aniState->ofdmPhyErrBase) {
				HALDEBUG(ah, "%s: phyCnt1 0x%x, resetting "
					"counter value to 0x%x\n", __func__,
					phyCnt1, aniState->ofdmPhyErrBase);
				OS_REG_WRITE(ah, AR_PHYCNT1, aniState->ofdmPhyErrBase);
				OS_REG_WRITE(ah, AR_PHYCNTMASK1, AR_PHY_ERR_OFDM_TIMING);
			}
			if (phyCnt2 < aniState->cckPhyErrBase) {
				HALDEBUG(ah, "%s: phyCnt2 0x%x, resetting "
					"counter value to 0x%x\n", __func__,
					phyCnt2, aniState->cckPhyErrBase);
				OS_REG_WRITE(ah, AR_PHYCNT2, aniState->cckPhyErrBase);
				OS_REG_WRITE(ah, AR_PHYCNTMASK2, AR_PHY_ERR_CCK_TIMING);
			}
			return;		/* XXX */
		}
		/* NB: only use ast_ani_*errs with AH_PRIVATE_DIAG */
		ofdmPhyErrCnt = phyCnt1 - aniState->ofdmPhyErrBase;
		ahp->ah_stats.ast_ani_ofdmerrs +=
			ofdmPhyErrCnt - aniState->ofdmPhyErrCount;
		aniState->ofdmPhyErrCount = ofdmPhyErrCnt;

		cckPhyErrCnt = phyCnt2 - aniState->cckPhyErrBase;
		ahp->ah_stats.ast_ani_cckerrs +=
			cckPhyErrCnt - aniState->cckPhyErrCount;
		aniState->cckPhyErrCount = cckPhyErrCnt;
	}
	/*
	 * If ani is not enabled, return after we've collected
	 * statistics
	 */
	if (!(DO_ANI(ah)))
		return;
	if (aniState->listenTime > 5 * ahp->ah_aniPeriod) {
		/* 
		 * Check to see if need to lower immunity if
		 * 5 aniPeriods have passed
		 */
		if (aniState->ofdmPhyErrCount <= aniState->listenTime *
		     aniState->ofdmTrigLow/1000 &&
		    aniState->cckPhyErrCount <= aniState->listenTime *
		     aniState->cckTrigLow/1000)
			ar5212AniLowerImmunity(ah);
		ar5212AniRestart(ah);
	} else if (aniState->listenTime > ahp->ah_aniPeriod) {
		/* check to see if need to raise immunity */
		if (aniState->ofdmPhyErrCount > aniState->listenTime *
		    aniState->ofdmTrigHigh / 1000) {
			ar5212AniOfdmErrTrigger(ah);
			ar5212AniRestart(ah);
		} else if (aniState->cckPhyErrCount > aniState->listenTime *
			   aniState->cckTrigHigh / 1000) {
			ar5212AniCckErrTrigger(ah);
			ar5212AniRestart(ah);
		}
	}
}
#endif /* AH_SUPPORT_AR5212 */
