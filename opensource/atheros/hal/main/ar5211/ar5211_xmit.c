/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5211/ar5211_xmit.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5211

#include "ah.h"
#include "ah_internal.h"
#include "ah_desc.h"

#include "ar5211/ar5211.h"
#include "ar5211/ar5211reg.h"
#include "ar5211/ar5211desc.h"

/*
 * Update Tx FIFO trigger level.
 *
 * Set bIncTrigLevel to TRUE to increase the trigger level.
 * Set bIncTrigLevel to FALSE to decrease the trigger level.
 *
 * Returns TRUE if the trigger level was updated
 */
HAL_BOOL
ar5211UpdateTxTrigLevel(struct ath_hal *ah, HAL_BOOL bIncTrigLevel)
{
	u_int32_t curTrigLevel, txcfg;
	HAL_INT ints = ar5211GetInterrupts(ah);

	/*
	 * Disable chip interrupts. This is because halUpdateTxTrigLevel
	 * is called from both ISR and non-ISR contexts.
	 */
	ar5211SetInterrupts(ah, ints &~ HAL_INT_GLOBAL);
	txcfg = OS_REG_READ(ah, AR_TXCFG);
	curTrigLevel = (txcfg & AR_TXCFG_FTRIG_M) >> AR_TXCFG_FTRIG_S;
	if (bIncTrigLevel){
		/* increase the trigger level */
		curTrigLevel = curTrigLevel +
			((MAX_TX_FIFO_THRESHOLD - curTrigLevel) / 2);
	} else {
		/* decrease the trigger level if not already at the minimum */
		if (curTrigLevel > MIN_TX_FIFO_THRESHOLD) {
			/* decrease the trigger level */
			curTrigLevel--;
		} else {
			/* no update to the trigger level */
			/* re-enable chip interrupts */
			ar5211SetInterrupts(ah, ints);
			return AH_FALSE;
		}
	}
	/* Update the trigger level */
	OS_REG_WRITE(ah, AR_TXCFG, (txcfg &~ AR_TXCFG_FTRIG_M) |
		((curTrigLevel << AR_TXCFG_FTRIG_S) & AR_TXCFG_FTRIG_M));
	/* re-enable chip interrupts */
	ar5211SetInterrupts(ah, ints);
	return AH_TRUE;
}

/*
 * Set the properties of the tx queue with the parameters
 * from qInfo.  The queue must previously have been setup
 * with a call to ar5211SetupTxQueue.
 */
HAL_BOOL
ar5211SetTxQueueProps(struct ath_hal *ah, int q, const HAL_TXQ_INFO *qInfo)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	if (q >= HAL_NUM_TX_QUEUES) {
		HALDEBUG(ah, "%s: invalid queue num %u\n", __func__, q);
		return AH_FALSE;
	}
	return ath_hal_setTxQProps(ah, &ahp->ah_txq[q], qInfo);
}

/*
 * Return the properties for the specified tx queue.
 */
HAL_BOOL
ar5211GetTxQueueProps(struct ath_hal *ah, int q, HAL_TXQ_INFO *qInfo)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	if (q >= HAL_NUM_TX_QUEUES) {
		HALDEBUG(ah, "%s: invalid queue num %u\n", __func__, q);
		return AH_FALSE;
	}
	return ath_hal_getTxQProps(ah, qInfo, &ahp->ah_txq[q]);
}

/*
 * Allocate and initialize a tx DCU/QCU combination.
 */
int
ar5211SetupTxQueue(struct ath_hal *ah, HAL_TX_QUEUE type,
	const HAL_TXQ_INFO *qInfo)
{
	struct ath_hal_5211 *ahp = AH5211(ah);
	HAL_TX_QUEUE_INFO *qi;
	int q;

	switch (type) {
	case HAL_TX_QUEUE_BEACON:
		q = 9;
		break;
	case HAL_TX_QUEUE_CAB:
		q = 8;
		break;
	case HAL_TX_QUEUE_PSPOLL:
		q = 1;
		break;
	case HAL_TX_QUEUE_DATA:
		q = 0;
		if (ahp->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE)
			return q;
		break;
	default:
		HALDEBUG(ah, "%s: bad tx queue type %u\n", __func__, type);
		return -1;
	}

	HALDEBUG(ah, "%s: queue %u\n", __func__, q);

	qi = &ahp->ah_txq[q];
	if (qi->tqi_type != HAL_TX_QUEUE_INACTIVE) {
		HALDEBUG(ah, "%s: tx queue %u already active\n", __func__, q);
		return -1;
	}
	OS_MEMZERO(qi, sizeof(HAL_TX_QUEUE_INFO));
	qi->tqi_type = type;
	if (qInfo == AH_NULL) {
		/* by default enable OK+ERR+DESC+URN interrupts */
		qi->tqi_qflags =
			  TXQ_FLAG_TXOKINT_ENABLE
			| TXQ_FLAG_TXERRINT_ENABLE
			| TXQ_FLAG_TXDESCINT_ENABLE
			| TXQ_FLAG_TXURNINT_ENABLE
			;
		qi->tqi_aifs = INIT_AIFS;
		qi->tqi_cwmin = HAL_TXQ_USEDEFAULT;	/* NB: do at reset */
		qi->tqi_cwmax = INIT_CWMAX;
		qi->tqi_shretry = INIT_SH_RETRY;
		qi->tqi_lgretry = INIT_LG_RETRY;
	} else
		(void) ar5211SetTxQueueProps(ah, q, qInfo);
	return q;
}

/*
 * Update the h/w interrupt registers to reflect a tx q's configuration.
 */
static void
setTxQInterrupts(struct ath_hal *ah, HAL_TX_QUEUE_INFO *qi)
{
	struct ath_hal_5211 *ahp = AH5211(ah);

	HALDEBUG(ah, "%s: tx ok 0x%x err 0x%x desc 0x%x eol 0x%x urn 0x%x\n"
		, __func__
		, ahp->ah_txOkInterruptMask
		, ahp->ah_txErrInterruptMask
		, ahp->ah_txDescInterruptMask
		, ahp->ah_txEolInterruptMask
		, ahp->ah_txUrnInterruptMask
	);

	OS_REG_WRITE(ah, AR_IMR_S0,
		  SM(ahp->ah_txOkInterruptMask, AR_IMR_S0_QCU_TXOK)
		| SM(ahp->ah_txDescInterruptMask, AR_IMR_S0_QCU_TXDESC)
	);
	OS_REG_WRITE(ah, AR_IMR_S1,
		  SM(ahp->ah_txErrInterruptMask, AR_IMR_S1_QCU_TXERR)
		| SM(ahp->ah_txEolInterruptMask, AR_IMR_S1_QCU_TXEOL)
	);
	OS_REG_RMW_FIELD(ah, AR_IMR_S2,
		AR_IMR_S2_QCU_TXURN, ahp->ah_txUrnInterruptMask);
}


/*
 * Free a tx DCU/QCU combination.
 */
HAL_BOOL
ar5211ReleaseTxQueue(struct ath_hal *ah, u_int q)
{
	struct ath_hal_5211 *ahp = AH5211(ah);
	HAL_TX_QUEUE_INFO *qi;

	if (q >= HAL_NUM_TX_QUEUES) {
		HALDEBUG(ah, "%s: invalid queue num %u\n", __func__, q);
		return AH_FALSE;
	}
	qi = &ahp->ah_txq[q];
	if (qi->tqi_type == HAL_TX_QUEUE_INACTIVE) {
		HALDEBUG(ah, "%s: inactive queue %u\n", __func__, q);
		return AH_FALSE;
	}

	HALDEBUG(ah, "%s: release queue %u\n", __func__, q);

	qi->tqi_type = HAL_TX_QUEUE_INACTIVE;
	ahp->ah_txOkInterruptMask &= ~(1 << q);
	ahp->ah_txErrInterruptMask &= ~(1 << q);
	ahp->ah_txDescInterruptMask &= ~(1 << q);
	ahp->ah_txEolInterruptMask &= ~(1 << q);
	ahp->ah_txUrnInterruptMask &= ~(1 << q);
	setTxQInterrupts(ah, qi);

	return AH_TRUE;
}

/*
 * Set the retry, aifs, cwmin/max, readyTime regs for specified queue
 */
HAL_BOOL
ar5211ResetTxQueue(struct ath_hal *ah, u_int q)
{
	struct ath_hal_5211 *ahp = AH5211(ah);
	HAL_CHANNEL_INTERNAL *chan = AH_PRIVATE(ah)->ah_curchan;
	HAL_TX_QUEUE_INFO *qi;
	u_int32_t cwMin, chanCwMin, value;

	if (q >= HAL_NUM_TX_QUEUES) {
		HALDEBUG(ah, "%s: invalid queue num %u\n", __func__, q);
		return AH_FALSE;
	}
	qi = &ahp->ah_txq[q];
	if (qi->tqi_type == HAL_TX_QUEUE_INACTIVE) {
		HALDEBUG(ah, "%s: inactive queue %u\n", __func__, q);
		return AH_TRUE;		/* XXX??? */
	}

	if (qi->tqi_cwmin == HAL_TXQ_USEDEFAULT) {
		/*
		 * Select cwmin according to channel type.
		 * NB: chan can be NULL during attach
		 */
		if (chan && IS_CHAN_B(chan))
			chanCwMin = INIT_CWMIN_11B;
		else
			chanCwMin = INIT_CWMIN;
		/* make sure that the CWmin is of the form (2^n - 1) */
		for (cwMin = 1; cwMin < chanCwMin; cwMin = (cwMin << 1) | 1)
			;
	} else
		cwMin = qi->tqi_cwmin;

	/* set cwMin/Max and AIFS values */
	OS_REG_WRITE(ah, AR_DLCL_IFS(q),
		  SM(cwMin, AR_D_LCL_IFS_CWMIN)
		| SM(qi->tqi_cwmax, AR_D_LCL_IFS_CWMAX)
		| SM(qi->tqi_aifs, AR_D_LCL_IFS_AIFS));

	/* Set retry limit values */
	OS_REG_WRITE(ah, AR_DRETRY_LIMIT(q), 
		   SM(INIT_SSH_RETRY, AR_D_RETRY_LIMIT_STA_SH)
		 | SM(INIT_SLG_RETRY, AR_D_RETRY_LIMIT_STA_LG)
		 | SM(qi->tqi_lgretry, AR_D_RETRY_LIMIT_FR_LG)
		 | SM(qi->tqi_shretry, AR_D_RETRY_LIMIT_FR_SH)
	);

	/* enable early termination on the QCU */
	OS_REG_WRITE(ah, AR_QMISC(q), AR_Q_MISC_DCU_EARLY_TERM_REQ);

	if (AH_PRIVATE(ah)->ah_macVersion < AR_SREV_VERSION_OAHU) {
		/* Configure DCU to use the global sequence count */
		OS_REG_WRITE(ah, AR_DMISC(q), AR5311_D_MISC_SEQ_NUM_CONTROL);
	}
	/* multiqueue support */
	if (qi->tqi_cbrPeriod) {
		OS_REG_WRITE(ah, AR_QCBRCFG(q), 
			  SM(qi->tqi_cbrPeriod,AR_Q_CBRCFG_CBR_INTERVAL)
			| SM(qi->tqi_cbrOverflowLimit, AR_Q_CBRCFG_CBR_OVF_THRESH));
		OS_REG_WRITE(ah, AR_QMISC(q),
			OS_REG_READ(ah, AR_QMISC(q)) |
			AR_Q_MISC_FSP_CBR |
			(qi->tqi_cbrOverflowLimit ?
				AR_Q_MISC_CBR_EXP_CNTR_LIMIT : 0));
	}
	if (qi->tqi_readyTime) {
		OS_REG_WRITE(ah, AR_QRDYTIMECFG(q),
			SM(qi->tqi_readyTime, AR_Q_RDYTIMECFG_INT) | 
			AR_Q_RDYTIMECFG_EN);
	}
	if (qi->tqi_burstTime) {
		OS_REG_WRITE(ah, AR_DCHNTIME(q),
			SM(qi->tqi_burstTime, AR_D_CHNTIME_DUR) |
			AR_D_CHNTIME_EN);
		if (qi->tqi_qflags & TXQ_FLAG_RDYTIME_EXP_POLICY_ENABLE) {
			OS_REG_WRITE(ah, AR_QMISC(q),
			     OS_REG_READ(ah, AR_QMISC(q)) |
			     AR_Q_MISC_RDYTIME_EXP_POLICY);
		}
	}

	if (qi->tqi_qflags & TXQ_FLAG_BACKOFF_DISABLE) {
		OS_REG_WRITE(ah, AR_DMISC(q),
			OS_REG_READ(ah, AR_DMISC(q)) |
			AR_D_MISC_POST_FR_BKOFF_DIS);
	}
	if (qi->tqi_qflags & TXQ_FLAG_FRAG_BURST_BACKOFF_ENABLE) {
		OS_REG_WRITE(ah, AR_DMISC(q),
			OS_REG_READ(ah, AR_DMISC(q)) |
			AR_D_MISC_FRAG_BKOFF_EN);
	}
	switch (qi->tqi_type) {
	case HAL_TX_QUEUE_BEACON:
		/* Configure QCU for beacons */
		OS_REG_WRITE(ah, AR_QMISC(q),
			OS_REG_READ(ah, AR_QMISC(q))
			| AR_Q_MISC_FSP_DBA_GATED
			| AR_Q_MISC_BEACON_USE
			| AR_Q_MISC_CBR_INCR_DIS1);
		/* Configure DCU for beacons */
		value = (AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL << AR_D_MISC_ARB_LOCKOUT_CNTRL_S)
			| AR_D_MISC_BEACON_USE | AR_D_MISC_POST_FR_BKOFF_DIS;
		if (AH_PRIVATE(ah)->ah_macVersion < AR_SREV_VERSION_OAHU)
			value |= AR5311_D_MISC_SEQ_NUM_CONTROL;
		OS_REG_WRITE(ah, AR_DMISC(q), value);
		break;
	case HAL_TX_QUEUE_CAB:
		/* Configure QCU for CAB (Crap After Beacon) frames */
		OS_REG_WRITE(ah, AR_QMISC(q),
			OS_REG_READ(ah, AR_QMISC(q))
			| AR_Q_MISC_FSP_DBA_GATED | AR_Q_MISC_CBR_INCR_DIS1
			| AR_Q_MISC_CBR_INCR_DIS0 | AR_Q_MISC_RDYTIME_EXP_POLICY);

		value = (ahp->ah_beaconInterval
			- (ath_hal_sw_beacon_response_time - ath_hal_dma_beacon_response_time)
			- ath_hal_additional_swba_backoff) * 1024;
		OS_REG_WRITE(ah, AR_QRDYTIMECFG(q), value | AR_Q_RDYTIMECFG_EN);

		/* Configure DCU for CAB */
		value = (AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL << AR_D_MISC_ARB_LOCKOUT_CNTRL_S);
		if (AH_PRIVATE(ah)->ah_macVersion < AR_SREV_VERSION_OAHU)
			value |= AR5311_D_MISC_SEQ_NUM_CONTROL;
		OS_REG_WRITE(ah, AR_QMISC(q), value);
		break;
	case HAL_TX_QUEUE_PSPOLL:
		/* configure psPoll QCU to be TIM-gated */
		OS_REG_WRITE(ah, AR_QMISC(q),
			OS_REG_READ(ah, AR_QMISC(q))
			| AR_Q_MISC_FSP_TIM_GATED
			| AR_Q_MISC_CBR_INCR_DIS1);
		break;
	default:
		/* NB: silence compiler */
		break;
	}

#ifndef AH_DISABLE_WME
	/*
	 * Yes, this is a hack and not the right way to do it, but
	 * it does get the lockout bits and backoff set for the
	 * high-pri WME queues for testing.  We need to either extend
	 * the meaning of queueInfo->mode, or create something like
	 * queueInfo->dcumode.
	 */
	if (qi->tqi_intFlags & HAL_TXQ_USE_LOCKOUT_BKOFF_DIS) {
		OS_REG_WRITE(ah, AR_DMISC(q),
			 OS_REG_READ(ah, AR_DMISC(q)) |
			 SM(AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL,
			    AR_D_MISC_ARB_LOCKOUT_CNTRL)|
			 AR_D_MISC_POST_FR_BKOFF_DIS);
	}
#endif

	/*
	 * Always update the secondary interrupt mask registers - this
	 * could be a new queue getting enabled in a running system or
	 * hw getting re-initialized during a reset!
	 *
	 * Since we don't differentiate between tx interrupts corresponding
	 * to individual queues - secondary tx mask regs are always unmasked;
	 * tx interrupts are enabled/disabled for all queues collectively
	 * using the primary mask reg
	 */
	if (qi->tqi_qflags & TXQ_FLAG_TXOKINT_ENABLE)
		ahp->ah_txOkInterruptMask |= 1 << q;
	else
		ahp->ah_txOkInterruptMask &= ~(1 << q);
	if (qi->tqi_qflags & TXQ_FLAG_TXERRINT_ENABLE)
		ahp->ah_txErrInterruptMask |= 1 << q;
	else
		ahp->ah_txErrInterruptMask &= ~(1 << q);
	if (qi->tqi_qflags & TXQ_FLAG_TXDESCINT_ENABLE)
		ahp->ah_txDescInterruptMask |= 1 << q;
	else
		ahp->ah_txDescInterruptMask &= ~(1 << q);
	if (qi->tqi_qflags & TXQ_FLAG_TXEOLINT_ENABLE)
		ahp->ah_txEolInterruptMask |= 1 << q;
	else
		ahp->ah_txEolInterruptMask &= ~(1 << q);
	if (qi->tqi_qflags & TXQ_FLAG_TXURNINT_ENABLE)
		ahp->ah_txUrnInterruptMask |= 1 << q;
	else
		ahp->ah_txUrnInterruptMask &= ~(1 << q);
	setTxQInterrupts(ah, qi);

	return AH_TRUE;
}

/*
 * Get the TXDP for the specified data queue.
 */
u_int32_t
ar5211GetTxDP(struct ath_hal *ah, u_int q)
{
	HALASSERT(q < HAL_NUM_TX_QUEUES);
	return OS_REG_READ(ah, AR_QTXDP(q));
}

/*
 * Set the TxDP for the specified tx queue.
 */
HAL_BOOL
ar5211SetTxDP(struct ath_hal *ah, u_int q, u_int32_t txdp)
{
	HALASSERT(q < HAL_NUM_TX_QUEUES);
	HALASSERT(AH5211(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	/*
	 * Make sure that TXE is deasserted before setting the TXDP.  If TXE
	 * is still asserted, setting TXDP will have no effect.
	 */
	HALASSERT((OS_REG_READ(ah, AR_Q_TXE) & (1 << q)) == 0);

	OS_REG_WRITE(ah, AR_QTXDP(q), txdp);

	return AH_TRUE;
}

/*
 * Set Transmit Enable bits for the specified queues.
 */
HAL_BOOL
ar5211StartTxDma(struct ath_hal *ah, u_int q)
{
	HALASSERT(q < HAL_NUM_TX_QUEUES);
	HALASSERT(AH5211(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	/* Check that queue is not already active */
	HALASSERT((OS_REG_READ(ah, AR_Q_TXD) & (1<<q)) == 0);

	HALDEBUGn(ah, 2, "%s: queue %u\n", __func__, q);

	/* Check to be sure we're not enabling a q that has its TXD bit set. */
	HALASSERT((OS_REG_READ(ah, AR_Q_TXD) & (1 << q)) == 0);

	OS_REG_WRITE(ah, AR_Q_TXE, 1 << q);
	return AH_TRUE;
}

/*
 * Return the number of frames pending on the specified queue.
 */
u_int32_t
ar5211NumTxPending(struct ath_hal *ah, u_int q)
{
	u_int32_t n;

	HALASSERT(q < HAL_NUM_TX_QUEUES);
	HALASSERT(AH5211(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	n = OS_REG_READ(ah, AR_QSTS(q)) & AR_Q_STS_PEND_FR_CNT_M;
	/*
	 * Pending frame count (PFC) can momentarily go to zero
	 * while TXE remains asserted.  In other words a PFC of
	 * zero is not sufficient to say that the queue has stopped.
	 */
	if (n == 0 && (OS_REG_READ(ah, AR_Q_TXE) & (1<<q)))
		n = 1;			/* arbitrarily pick 1 */
	return n;
}

/*
 * Stop transmit on the specified queue
 */
HAL_BOOL
ar5211StopTxDma(struct ath_hal *ah, u_int q)
{
	int i;

	HALASSERT(q < HAL_NUM_TX_QUEUES);
	HALASSERT(AH5211(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	OS_REG_WRITE(ah, AR_Q_TXD, 1<<q);
	for (i = 0; i < 10000; i++) {
		if (ar5211NumTxPending(ah, q) == 0)
			break;
		OS_DELAY(10);
	}
	OS_REG_WRITE(ah, AR_Q_TXD, 0);

	return (i < 10000);
}

HAL_BOOL
ar5211UpdateCTSForBursting(struct ath_hal *ah, struct ath_desc *ds,
	struct ath_desc *prevds,
	struct ath_desc *prevdsWithCTS,
	struct ath_desc *gatingds,	
	u_int32_t txOpLimit,
	u_int32_t ctsDuration)
{
	return 1;
}

/*
 * Descriptor Access Functions
 */

#define	VALID_PKT_TYPES \
	((1<<HAL_PKT_TYPE_NORMAL)|(1<<HAL_PKT_TYPE_ATIM)|\
	 (1<<HAL_PKT_TYPE_PSPOLL)|(1<<HAL_PKT_TYPE_PROBE_RESP)|\
	 (1<<HAL_PKT_TYPE_BEACON))
#define	isValidPktType(_t)	((1<<(_t)) & VALID_PKT_TYPES)
#define	VALID_TX_RATES \
	((1<<0x0b)|(1<<0x0f)|(1<<0x0a)|(1<<0x0e)|(1<<0x09)|(1<<0x0d)|\
	 (1<<0x08)|(1<<0x0c)|(1<<0x1b)|(1<<0x1a)|(1<<0x1e)|(1<<0x19)|\
	 (1<<0x1d)|(1<<0x18)|(1<<0x1c))
#define	isValidTxRate(_r)	((1<<(_r)) & VALID_TX_RATES)

HAL_BOOL
ar5211SetupTxDesc(struct ath_hal *ah, struct ath_desc *ds,
	u_int pktLen,
	u_int hdrLen,
	HAL_PKT_TYPE type,
	u_int txPower,
	u_int txRate0, u_int txTries0,
	u_int keyIx,
	u_int antMode,
	u_int flags,
	u_int rtsctsRate,
	u_int rtsctsDuration,
	u_int compicvLen, 
	u_int compivLen,
	u_int comp)
{
	struct ar5211_desc *ads = AR5211DESC(ds);

	(void) hdrLen;
	(void) txPower;
	(void) rtsctsRate; (void) rtsctsDuration;

	HALASSERT(txTries0 != 0);
	HALASSERT(isValidPktType(type));
	HALASSERT(isValidTxRate(txRate0));
	/* XXX validate antMode */

	ads->ds_ctl0 = (pktLen & AR_FrameLen)
		     | (txRate0 << AR_XmitRate_S)
		     | (antMode << AR_AntModeXmit_S)
		     | (flags & HAL_TXDESC_CLRDMASK ? AR_ClearDestMask : 0)
		     | (flags & HAL_TXDESC_INTREQ ? AR_TxInterReq : 0)
		     | (flags & HAL_TXDESC_RTSENA ? AR_RTSCTSEnable : 0)
		     | (flags & HAL_TXDESC_VEOL ? AR_VEOL : 0)
		     ;
	ads->ds_ctl1 = (type << 26)
		     | (flags & HAL_TXDESC_NOACK ? AR_NoAck : 0)
		     ;

	if (keyIx != HAL_TXKEYIX_INVALID) {
		ads->ds_ctl1 |=
			(keyIx << AR_EncryptKeyIdx_S) & AR_EncryptKeyIdx;
		ads->ds_ctl0 |= AR_EncryptKeyValid;
	}
	return AH_TRUE;
#undef RATE
}

HAL_BOOL
ar5211SetupXTxDesc(struct ath_hal *ah, struct ath_desc *ds,
	u_int txRate1, u_int txTries1,
	u_int txRate2, u_int txTries2,
	u_int txRate3, u_int txTries3)
{
	(void) ah; (void) ds;
	(void) txRate1; (void) txTries1;
	(void) txRate2; (void) txTries2;
	(void) txRate3; (void) txTries3;
	return AH_FALSE;
}

HAL_BOOL
ar5211FillTxDesc(struct ath_hal *ah, struct ath_desc *ds,
	u_int segLen, HAL_BOOL firstSeg, HAL_BOOL lastSeg,
	const struct ath_desc *ds0)
{
	struct ar5211_desc *ads = AR5211DESC(ds);

	HALASSERT((segLen &~ AR_BufLen) == 0);

	if (firstSeg) {
		/*
		 * First descriptor, don't clobber xmit control data
		 * setup by ar5211SetupTxDesc.
		 */
		ads->ds_ctl1 |= segLen | (lastSeg ? 0 : AR_More);
	} else if (lastSeg) {		/* !firstSeg && lastSeg */
		/*
		 * Last descriptor in a multi-descriptor frame,
		 * copy the transmit parameters from the first
		 * frame for processing on completion. 
		 */
		ads->ds_ctl0 = AR5211DESC_CONST(ds0)->ds_ctl0;
		ads->ds_ctl1 = segLen;
	} else {			/* !firstSeg && !lastSeg */
		/*
		 * Intermediate descriptor in a multi-descriptor frame.
		 */
		ads->ds_ctl0 = 0;
		ads->ds_ctl1 = segLen | AR_More;
	}
	ads->ds_status0 = ads->ds_status1 = 0;
	return AH_TRUE;
}

/*
 * Processing of HW TX descriptor.
 */
HAL_STATUS
ar5211ProcTxDesc(struct ath_hal *ah, struct ath_desc *ds)
{
	struct ar5211_desc *ads = AR5211DESC(ds);

	if ((ads->ds_status1 & AR_Done) == 0)
		return HAL_EINPROGRESS;

	/* Update software copies of the HW status */
	ds->ds_txstat.ts_seqnum = MS(ads->ds_status1, AR_SeqNum);
	ds->ds_txstat.ts_tstamp = MS(ads->ds_status0, AR_SendTimestamp);
	ds->ds_txstat.ts_status = 0;
	if ((ads->ds_status0 & AR_FrmXmitOK) == 0) {
		if (ads->ds_status0 & AR_ExcessiveRetries)
			ds->ds_txstat.ts_status |= HAL_TXERR_XRETRY;
		if (ads->ds_status0 & AR_Filtered)
			ds->ds_txstat.ts_status |= HAL_TXERR_FILT;
		if (ads->ds_status0 & AR_FIFOUnderrun)
			ds->ds_txstat.ts_status |= HAL_TXERR_FIFO;
	}
	ds->ds_txstat.ts_rate = MS(ads->ds_ctl0, AR_XmitRate);
	ds->ds_txstat.ts_rssi = MS(ads->ds_status1, AR_AckSigStrength);
	ds->ds_txstat.ts_shortretry = MS(ads->ds_status0, AR_ShortRetryCnt);
	ds->ds_txstat.ts_longretry = MS(ads->ds_status0, AR_LongRetryCnt);
	ds->ds_txstat.ts_virtcol = MS(ads->ds_status0, AR_VirtCollCnt);
	ds->ds_txstat.ts_antenna = 0;		/* NB: don't know */
	/*
	 * WAR for h/w bug where the number of retries is one
	 * less than it should be.  Also, 0 retries and 1 retry
	 * are both reported as 0 retries.
	 */
	if (ds->ds_txstat.ts_shortretry > 0)
		ds->ds_txstat.ts_shortretry++;
	if (ds->ds_txstat.ts_longretry > 0)
		ds->ds_txstat.ts_longretry++;

	return HAL_OK;
}

/*
 * Determine which tx queues need interrupt servicing.
 * STUB.
 */
void
ar5211GetTxIntrQueue(struct ath_hal *ah, u_int32_t *txqs)
{
	return;
}
#endif /* AH_SUPPORT_AR5211 */
