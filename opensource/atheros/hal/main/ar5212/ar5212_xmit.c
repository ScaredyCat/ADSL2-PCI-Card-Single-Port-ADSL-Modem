/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5212/ar5212_xmit.c#2 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#include "ah.h"
#include "ah_xr.h"
#include "ah_internal.h"

#include "ar5212/ar5212.h"
#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212desc.h"
#include "ar5212/ar5212phy.h"
#ifdef AH_SUPPORT_5311
#include "ar5212/ar5311reg.h"
#endif

#ifdef AH_NEED_DESC_SWAP
static void ar5212SwapTxDesc(struct ath_desc *ds);
#endif

/*
 * Update Tx FIFO trigger level.
 *
 * Set bIncTrigLevel to TRUE to increase the trigger level.
 * Set bIncTrigLevel to FALSE to decrease the trigger level.
 *
 * Returns TRUE if the trigger level was updated
 */
HAL_BOOL
ar5212UpdateTxTrigLevel(struct ath_hal *ah, HAL_BOOL bIncTrigLevel)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	u_int32_t txcfg, curLevel, newLevel;
	HAL_INT omask;

	/*
	 * Disable interrupts while futzing with the fifo level.
	 */
	omask = ar5212SetInterrupts(ah, ahp->ah_maskReg &~ HAL_INT_GLOBAL);

	txcfg = OS_REG_READ(ah, AR_TXCFG);
	curLevel = MS(txcfg, AR_FTRIG);
	newLevel = curLevel;
	if (bIncTrigLevel)		/* increase the trigger level */
		newLevel += (MAX_TX_FIFO_THRESHOLD - curLevel) / 2;
	else if (curLevel > MIN_TX_FIFO_THRESHOLD)
		newLevel--;
	if (newLevel != curLevel)
		/* Update the trigger level */
		OS_REG_WRITE(ah, AR_TXCFG,
			(txcfg &~ AR_FTRIG) | SM(newLevel, AR_FTRIG));

	/* re-enable chip interrupts */
	ar5212SetInterrupts(ah, omask);

	return (newLevel != curLevel);
}

/*
 * Set the properties of the tx queue with the parameters
 * from qInfo.  
 */
HAL_BOOL
ar5212SetTxQueueProps(struct ath_hal *ah, int q, const HAL_TXQ_INFO *qInfo)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;

	if (q >= pCap->halTotalQueues) {
		HALDEBUG(ah, "%s: invalid queue num %u\n", __func__, q);
		return AH_FALSE;
	}
	return ath_hal_setTxQProps(ah, &ahp->ah_txq[q], qInfo);
}

/*
 * Return the properties for the specified tx queue.
 */
HAL_BOOL
ar5212GetTxQueueProps(struct ath_hal *ah, int q, HAL_TXQ_INFO *qInfo)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;


	if (q >= pCap->halTotalQueues) {
		HALDEBUG(ah, "%s: invalid queue num %u\n", __func__, q);
		return AH_FALSE;
	}
	return ath_hal_getTxQProps(ah, qInfo, &ahp->ah_txq[q]);
}

/*
 * Allocate and initialize a tx DCU/QCU combination.
 */
int
ar5212SetupTxQueue(struct ath_hal *ah, HAL_TX_QUEUE type,
	const HAL_TXQ_INFO *qInfo)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_TX_QUEUE_INFO *qi;
	HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;
	int q;

	/* XXX move queue assignment to driver */
	switch (type) {
	case HAL_TX_QUEUE_BEACON:
		q = pCap->halTotalQueues-1;	/* highest priority */
		break;
	case HAL_TX_QUEUE_CAB:
		q = pCap->halTotalQueues-2;	/* next highest priority */
		break;
	case HAL_TX_QUEUE_PSPOLL:
		q = 1;
		break;
	case HAL_TX_QUEUE_DATA:
		for (q = 0; q < pCap->halTotalQueues; q++)
			if (ahp->ah_txq[q].tqi_type == HAL_TX_QUEUE_INACTIVE)
				break;
		if (q == pCap->halTotalQueues) {
			HALDEBUG(ah, "%s: no available tx queue\n", __func__);
			return -1;
		}
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
		qi->tqi_physCompBuf = 0;
	} else {
		qi->tqi_physCompBuf = qInfo->tqi_compBuf;
		(void) ar5212SetTxQueueProps(ah, q, qInfo);
	}
	/* NB: must be followed by ar5212ResetTxQueue */
	return q;
}

/*
 * Update the h/w interrupt registers to reflect a tx q's configuration.
 */
static void
setTxQInterrupts(struct ath_hal *ah, HAL_TX_QUEUE_INFO *qi)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

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
ar5212ReleaseTxQueue(struct ath_hal *ah, u_int q)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;
	HAL_TX_QUEUE_INFO *qi;

	if (q >= pCap->halTotalQueues) {
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
 * Assumes:
 *  phwChannel has been set to point to the current channel
 */
HAL_BOOL
ar5212ResetTxQueue(struct ath_hal *ah, u_int q)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CAPABILITIES *pCap = &AH_PRIVATE(ah)->ah_caps;
	HAL_CHANNEL_INTERNAL *chan = AH_PRIVATE(ah)->ah_curchan;
	HAL_TX_QUEUE_INFO *qi;
	u_int32_t cwMin, chanCwMin, value;

	if (q >= pCap->halTotalQueues) {
		HALDEBUG(ah, "%s: invalid queue num %u\n", __func__, q);
		return AH_FALSE;
	}
	qi = &ahp->ah_txq[q];
	if (qi->tqi_type == HAL_TX_QUEUE_INACTIVE) {
		HALDEBUG(ah, "%s: inactive queue %u\n", __func__, q);
		return AH_TRUE;		/* XXX??? */
	}

	HALDEBUG(ah, "%s: reset queue %u\n", __func__, q);

	if (qi->tqi_cwmin == HAL_TXQ_USEDEFAULT) {
		/*
		 * Select cwmin according to channel type.
		 * NB: chan can be NULL during attach
		 */
#ifdef AH_SUPPORT_XR
		if (chan && IS_CHAN_XR(chan))
			chanCwMin = INIT_CWMIN_XR;
		else
#endif /* AH_SUPPORT_XR */
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

	/* enable DCU to wait for next fragment from QCU */
	OS_REG_WRITE(ah, AR_DMISC(q), AR_D_MISC_FRAG_WAIT_EN);

#ifdef AH_SUPPORT_5311
	if (AH_PRIVATE(ah)->ah_macVersion < AR_SREV_VERSION_OAHU) {
		/* Configure DCU to use the global sequence count */
		OS_REG_WRITE(ah, AR_DMISC(q), AR5311_D_MISC_SEQ_NUM_CONTROL);
	}
#endif
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
			AR_Q_RDYTIMECFG_ENA);
	}
	
	OS_REG_WRITE(ah, AR_DCHNTIME(q),
			SM(qi->tqi_burstTime, AR_D_CHNTIME_DUR) |
			(qi->tqi_burstTime ? AR_D_CHNTIME_EN : 0));
			
	if (qi->tqi_burstTime && qi->tqi_qflags & TXQ_FLAG_RDYTIME_EXP_POLICY_ENABLE) {
			OS_REG_WRITE(ah, AR_QMISC(q),
			     OS_REG_READ(ah, AR_QMISC(q)) |
			     AR_Q_MISC_RDYTIME_EXP_POLICY);
		
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
	case HAL_TX_QUEUE_BEACON:		/* beacon frames */
		OS_REG_WRITE(ah, AR_QMISC(q),
			OS_REG_READ(ah, AR_QMISC(q))
			| AR_Q_MISC_FSP_DBA_GATED
			| AR_Q_MISC_BEACON_USE
			| AR_Q_MISC_CBR_INCR_DIS1);

		OS_REG_WRITE(ah, AR_DMISC(q),
			OS_REG_READ(ah, AR_DMISC(q))
			| (AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL <<
				AR_D_MISC_ARB_LOCKOUT_CNTRL_S)
			| AR_D_MISC_BEACON_USE
			| AR_D_MISC_POST_FR_BKOFF_DIS);
		break;
	case HAL_TX_QUEUE_CAB:			/* CAB  frames */
		/* 
		 * No longer Enable AR_Q_MISC_RDYTIME_EXP_POLICY,
		 * bug #6079.  There is an issue with the CAB Queue
		 * not properly refreshing the Tx descriptor if
		 * the TXE clear setting is used.
		 */
		OS_REG_WRITE(ah, AR_QMISC(q),
			OS_REG_READ(ah, AR_QMISC(q))
			| AR_Q_MISC_FSP_DBA_GATED
			| AR_Q_MISC_CBR_INCR_DIS1
			| AR_Q_MISC_CBR_INCR_DIS0);

		value = (ahp->ah_beaconInterval
			- (ath_hal_sw_beacon_response_time -
				ath_hal_dma_beacon_response_time)
			- ath_hal_additional_swba_backoff) * 1024;
		OS_REG_WRITE(ah, AR_QRDYTIMECFG(q), value | AR_Q_RDYTIMECFG_ENA);

		OS_REG_WRITE(ah, AR_DMISC(q),
			OS_REG_READ(ah, AR_DMISC(q))
			| (AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL <<
				AR_D_MISC_ARB_LOCKOUT_CNTRL_S));
		break;
	case HAL_TX_QUEUE_PSPOLL:
		/*
		 * We may configure psPoll QCU to be TIM-gated in the
		 * future; TIM_GATED bit is not enabled currently because
		 * of a hardware problem in Oahu that overshoots the TIM
		 * bitmap in beacon and may find matching associd bit in
		 * non-TIM elements and send PS-poll PS poll processing
		 * will be done in software
		 */
		OS_REG_WRITE(ah, AR_QMISC(q),
			OS_REG_READ(ah, AR_QMISC(q)) | AR_Q_MISC_CBR_INCR_DIS1);
		break;
	default:			/* NB: silence compiler */
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

	/* Setup compression scratchpad buffer */
        if (qi->tqi_physCompBuf) {
                HALASSERT(type == HAL_TX_QUEUE_DATA);
                OS_REG_WRITE(ah, AR_Q_CBBS, (80 + 2*q));
                OS_REG_WRITE(ah, AR_Q_CBBA, qi->tqi_physCompBuf);
                OS_REG_WRITE(ah, AR_Q_CBC,  HAL_COMP_BUF_MAX_SIZE/1024);
                OS_REG_WRITE(ah, AR_Q0_MISC + 4*q,
                                OS_REG_READ(ah, AR_Q0_MISC + 4*q)
                                        | AR_Q_MISC_QCU_COMP_EN);
        }

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
 * Get the TXDP for the specified queue
 */
u_int32_t
ar5212GetTxDP(struct ath_hal *ah, u_int q)
{
	HALASSERT(q < AH_PRIVATE(ah)->ah_caps.halTotalQueues);
	return OS_REG_READ(ah, AR_QTXDP(q));
}

/*
 * Set the TxDP for the specified queue
 */
HAL_BOOL
ar5212SetTxDP(struct ath_hal *ah, u_int q, u_int32_t txdp)
{
	HALASSERT(q < AH_PRIVATE(ah)->ah_caps.halTotalQueues);
	HALASSERT(AH5212(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	/*
	 * Make sure that TXE is deasserted before setting the TXDP.  If TXE
	 * is still asserted, setting TXDP will have no effect.
	 */
	HALASSERT((OS_REG_READ(ah, AR_Q_TXE) & (1 << q)) == 0);

	OS_REG_WRITE(ah, AR_QTXDP(q), txdp);

	return AH_TRUE;
}

/*
 * Set Transmit Enable bits for the specified queue
 */
HAL_BOOL
ar5212StartTxDma(struct ath_hal *ah, u_int q)
{
	HALASSERT(q < AH_PRIVATE(ah)->ah_caps.halTotalQueues);

	HALASSERT(AH5212(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	HALDEBUGn(ah, 2, "%s: queue %u\n", __func__, q);

	/* Check to be sure we're not enabling a q that has its TXD bit set. */
	HALASSERT((OS_REG_READ(ah, AR_Q_TXD) & (1 << q)) == 0);

	OS_REG_WRITE(ah, AR_Q_TXE, 1 << q);
	return AH_TRUE;
}

/*
 * Return the number of pending frames or 0 if the specified
 * queue is stopped.
 */
u_int32_t
ar5212NumTxPending(struct ath_hal *ah, u_int q)
{
	u_int32_t npend;

	HALASSERT(q < AH_PRIVATE(ah)->ah_caps.halTotalQueues);
	HALASSERT(AH5212(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	npend = OS_REG_READ(ah, AR_QSTS(q)) & AR_Q_STS_PEND_FR_CNT;
	if (npend == 0) {
		/*
		 * Pending frame count (PFC) can momentarily go to zero
		 * while TXE remains asserted.  In other words a PFC of
		 * zero is not sufficient to say that the queue has stopped.
		 */
		if (OS_REG_READ(ah, AR_Q_TXE) & (1 << q))
			npend = 1;		/* arbitrarily return 1 */
	}
#ifdef DEBUG
	if (npend && (AH5212(ah)->ah_txq[q].tqi_type == HAL_TX_QUEUE_CAB)) {
		if (OS_REG_READ(ah, AR_Q_RDYTIMESHDN) & (1 << q)) {
			isrPrintf("RTSD on CAB queue\n");
			/* Clear the ReadyTime shutdown status bits */
			OS_REG_WRITE(ah, AR_Q_RDYTIMESHDN, 1 << q);
		}
	}
#endif
	return npend;
}

/*
 * Stop transmit on the specified queue
 */
HAL_BOOL
ar5212StopTxDma(struct ath_hal *ah, u_int q)
{
	u_int i;

	HALASSERT(q < AH_PRIVATE(ah)->ah_caps.halTotalQueues);

	HALASSERT(AH5212(ah)->ah_txq[q].tqi_type != HAL_TX_QUEUE_INACTIVE);

	OS_REG_WRITE(ah, AR_Q_TXD, 1 << q);
	for (i = 1000; i != 0; i--) {
		if (ar5212NumTxPending(ah, q) == 0)
			break;
		OS_DELAY(100);        /* XXX get actual value */
	}
#ifdef AH_DEBUG
	if (i == 0) {
		HALDEBUG(ah, "%s: queue %u DMA did not stop in 100 msec\n",
			__func__, q);
		HALDEBUG(ah, "%s: QSTS 0x%x Q_TXE 0x%x Q_TXD 0x%x Q_CBR 0x%x\n"
			, __func__
			, OS_REG_READ(ah, AR_QSTS(q))
			, OS_REG_READ(ah, AR_Q_TXE)
			, OS_REG_READ(ah, AR_Q_TXD)
			, OS_REG_READ(ah, AR_QCBRCFG(q))
		);
		HALDEBUG(ah, "%s: Q_MISC 0x%x Q_RDYTIMECFG 0x%x Q_RDYTIMESHDN 0x%x\n"
			, __func__
			, OS_REG_READ(ah, AR_QMISC(q))
			, OS_REG_READ(ah, AR_QRDYTIMECFG(q))
			, OS_REG_READ(ah, AR_Q_RDYTIMESHDN)
		);
	}
#endif /* AH_DEBUG */
	OS_REG_WRITE(ah, AR_Q_TXD, 0);
	return (i != 0);
}

/* Set the Interrupt Req and VEOL for the frame */
/* Note: caller must ensure (_ds) is not NULL   */
#define AR5212_SET_INT_VEOL_IN_TXDESC(_ds, _value) do {					\
		u_int8_t moreData = ((_ds)->ds_ctl1 & AR_More);				\
		struct ar5212_desc *currds = (_ds); 					\
		while (currds && moreData) {						\
			currds->ds_ctl0 = (_value) ? 					\
						(currds->ds_ctl0 | (AR_TxInterReq))	\
					      : (currds->ds_ctl0 & ~(AR_TxInterReq));	\
			currds++;							\
			if (currds) {							\
				moreData = (currds->ds_ctl1 & AR_More);			\
			}								\
		}									\
		HALASSERT(currds);							\
		/* Set the Last Desc interrupt Req and VEOL bit */                      \
		currds->ds_ctl0 = (_value) ? 						\
					(currds->ds_ctl0 | (AR_TxInterReq | AR_VEOL))	\
				      : (currds->ds_ctl0 & ~(AR_TxInterReq | AR_VEOL));	\
} while(0)

HAL_BOOL
ar5212UpdateCTSForBursting(struct ath_hal *ah, struct ath_desc *ds,
	struct ath_desc *prevds,
	struct ath_desc *prevdsWithCTS,
	struct ath_desc *gatingds,	
	u_int32_t txOpLimit /* in us */,
	u_int32_t ctsDuration)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	struct ar5212_desc *ads = AR5212DESC(ds);
	struct ar5212_desc *aprevds = AR5212DESC(prevds);	
	struct ar5212_desc *aprevdsWithCTS = AR5212DESC(prevdsWithCTS);	
	struct ar5212_desc *agatingds = AR5212DESC(gatingds);
	u_int32_t	aprevRTSCTSDur = 0;
	u_int32_t	decCTSdur = 0;
	u_int8_t	doneBit = 1;
	u_int8_t	disableRTSCTS = 0;
	u_int8_t	aprevRTSEnable = 0;
	u_int32_t	adsRTSCTSDur = 0;
	u_int32_t	adsRTSCTSRate = 0;
	u_int32_t	adsRTSCTSEnable = 0;

	HALASSERT(ads);
    
    	if (txOpLimit) {
		/* Check if previous RTS/CTS has been sent yet.   */
		/* If not, record its RTS/CTS duration 		  */
		HAL_INT omask;
		/*
	 	 * Disable interrupts while futzing with the live queue.
	 	 */
		omask = ar5212SetInterrupts(ah, ahp->ah_maskReg &~ HAL_INT_GLOBAL);
		
		if (agatingds) {
			doneBit = (agatingds->ds_txstatus1 & AR_Done);
                	aprevRTSCTSDur = MS(aprevdsWithCTS->ds_ctl2, AR_RTSCTSDuration);
                	aprevRTSEnable = (aprevdsWithCTS->ds_ctl0 & AR_RTSCTSEnable);
		} 

        	/* Record current frame's RTS/CTS properties:        */
		/* duration, rate, enable 			     */
            	adsRTSCTSRate = MS(ads->ds_ctl3, AR_RTSCTSRate);
            	adsRTSCTSDur = MS(ads->ds_ctl2, AR_RTSCTSDuration);
            	adsRTSCTSEnable = (ads->ds_ctl0 & (AR_RTSCTSEnable | AR_CTSEnable));
         
		/*  If the previous CTS packet has been sent, exit early */
		if (!doneBit && adsRTSCTSEnable) {
			/* Account for extra RTS duration that covers the CTS packet */
            		if (aprevRTSEnable) {
				decCTSdur = ctsDuration;
			}
		
			/*
		 	 *  If the previous CTS packet's CTS duration can be extended to cover
		 	 *  the current packet while not exceeding the TxOpLimit, 
		 	 *  extend this CTS duration and disable CTS
		 	 *  on the current packet ---> burst extention
		 	 */
			if ((aprevRTSCTSDur + adsRTSCTSDur - decCTSdur) <= txOpLimit) {
				aprevRTSCTSDur += (adsRTSCTSDur - decCTSdur);
                		if (aprevdsWithCTS) {
                        		aprevdsWithCTS->ds_ctl2 = 
						(aprevdsWithCTS->ds_ctl2 & ~AR_RTSCTSDuration) |
						SM(aprevRTSCTSDur, AR_RTSCTSDuration);
                    			if (!(agatingds->ds_txstatus1 & AR_Done)) {
                        			disableRTSCTS = 1;
                    			}
                		}

                		if (disableRTSCTS) {
                        		ads->ds_ctl0 = (ads->ds_ctl0 & ~(AR_RTSCTSEnable | AR_CTSEnable));
                        		ads->ds_ctl2 = (ads->ds_ctl2 & ~AR_RTSCTSDuration);
                    		}
                	}
            	}
		
        	AR5212_SET_INT_VEOL_IN_TXDESC(ads, 1);
        	if (disableRTSCTS && aprevds) {
			AR5212_SET_INT_VEOL_IN_TXDESC(aprevds, 0);
        	}
		
		/* re-enable chip interrupts */
		ar5212SetInterrupts(ah, omask);		
	}
    
	return disableRTSCTS;
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
ar5212SetupTxDesc(struct ath_hal *ah, struct ath_desc *ds,
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
#define	RTSCTS	(HAL_TXDESC_RTSENA|HAL_TXDESC_CTSENA)
	struct ar5212_desc *ads = AR5212DESC(ds);
	struct ath_hal_5212 *ahp = AH5212(ah);

	(void) hdrLen;

	HALASSERT(txTries0 != 0);
	HALASSERT(isValidPktType(type));
	HALASSERT(isValidTxRate(txRate0));
	HALASSERT((flags & RTSCTS) != RTSCTS);
	/* XXX validate antMode */

        txPower = (txPower + ahp->ah_txPowerIndexOffset );
        if(txPower > 63)  txPower=63;

	ads->ds_ctl0 = (pktLen & AR_FrameLen)
		     | (txPower << AR_XmitPower_S)
		     | (flags & HAL_TXDESC_VEOL ? AR_VEOL : 0)
		     | (flags & HAL_TXDESC_CLRDMASK ? AR_ClearDestMask : 0)
		     | SM(antMode, AR_AntModeXmit)
		     | (flags & HAL_TXDESC_INTREQ ? AR_TxInterReq : 0)
		     ;
	ads->ds_ctl1 = (type << AR_FrmType_S)
		     | (flags & HAL_TXDESC_NOACK ? AR_NoAck : 0)
                     | (comp << AR_CompProc_S)
                     | (compicvLen << AR_CompICVLen_S)
                     | (compivLen << AR_CompIVLen_S)
                     ;
	ads->ds_ctl2 = SM(txTries0, AR_XmitDataTries0)
		     ;
	ads->ds_ctl3 = (txRate0 << AR_XmitRate0_S)
		     ;
	if (keyIx != HAL_TXKEYIX_INVALID) {
		/* XXX validate key index */
		ads->ds_ctl1 |= SM(keyIx, AR_DestIdx);
		ads->ds_ctl0 |= AR_DestIdxValid;
	}
	if (flags & RTSCTS) {
		if (!isValidTxRate(rtsctsRate)) {
			HALDEBUG(ah, "%s: invalid rts/cts rate 0x%x\n",
				__func__, rtsctsRate);
			return AH_FALSE;
		}
		/* XXX validate rtsctsDuration */
		ads->ds_ctl0 |= (flags & HAL_TXDESC_CTSENA ? AR_CTSEnable : 0)
			     | (flags & HAL_TXDESC_RTSENA ? AR_RTSCTSEnable : 0)
			     ;
		ads->ds_ctl2 |= SM(rtsctsDuration, AR_RTSCTSDuration);
		ads->ds_ctl3 |= (rtsctsRate << AR_RTSCTSRate_S);
	}
	return AH_TRUE;
#undef RTSCTS
}

HAL_BOOL
ar5212SetupXTxDesc(struct ath_hal *ah, struct ath_desc *ds,
	u_int txRate1, u_int txTries1,
	u_int txRate2, u_int txTries2,
	u_int txRate3, u_int txTries3)
{
	struct ar5212_desc *ads = AR5212DESC(ds);

	if (txTries1) {
		HALASSERT(isValidTxRate(txRate1));
		ads->ds_ctl2 |= SM(txTries1, AR_XmitDataTries1);
		ads->ds_ctl3 |= (txRate1 << AR_XmitRate1_S);
	}
	if (txTries2) {
		HALASSERT(isValidTxRate(txRate2));
		ads->ds_ctl2 |= SM(txTries2, AR_XmitDataTries2);
		ads->ds_ctl3 |= (txRate2 << AR_XmitRate2_S);
	}
	if (txTries3) {
		HALASSERT(isValidTxRate(txRate3));
		ads->ds_ctl2 |= SM(txTries3, AR_XmitDataTries3);
		ads->ds_ctl3 |= (txRate3 << AR_XmitRate3_S);
	}
	return AH_TRUE;
}

HAL_BOOL
ar5212FillTxDesc(struct ath_hal *ah, struct ath_desc *ds,
	u_int segLen, HAL_BOOL firstSeg, HAL_BOOL lastSeg,
	const struct ath_desc *ds0)
{
	struct ar5212_desc *ads = AR5212DESC(ds);

	HALASSERT((segLen &~ AR_BufLen) == 0);

	if (firstSeg) {
		/*
		 * First descriptor, don't clobber xmit control data
		 * setup by ar5212SetupTxDesc.
		 */
		ads->ds_ctl1 |= segLen | (lastSeg ? 0 : AR_More);
	} else if (lastSeg) {		/* !firstSeg && lastSeg */
		/*
		 * Last descriptor in a multi-descriptor frame,
		 * copy the multi-rate transmit parameters from
		 * the first frame for processing on completion. 
		 */
		ads->ds_ctl0 = 0;
		ads->ds_ctl1 = segLen;
#ifdef AH_NEED_DESC_SWAP
		ads->ds_ctl2 = __bswap32(AR5212DESC_CONST(ds0)->ds_ctl2);
		ads->ds_ctl3 = __bswap32(AR5212DESC_CONST(ds0)->ds_ctl3);
#else
		ads->ds_ctl2 = AR5212DESC_CONST(ds0)->ds_ctl2;
		ads->ds_ctl3 = AR5212DESC_CONST(ds0)->ds_ctl3;
#endif
	} else {			/* !firstSeg && !lastSeg */
		/*
		 * Intermediate descriptor in a multi-descriptor frame.
		 */
		ads->ds_ctl0 = 0;
		ads->ds_ctl1 = segLen | AR_More;
		ads->ds_ctl2 = 0;
		ads->ds_ctl3 = 0;
	}
	ads->ds_txstatus0 = ads->ds_txstatus1 = 0;
	return AH_TRUE;
}

#ifdef AH_NEED_DESC_SWAP
/* Swap transmit descriptor */
static __inline void
ar5212SwapTxDesc(struct ath_desc *ds)
{
	ds->ds_data = __bswap32(ds->ds_data);
        ds->ds_ctl0 = __bswap32(ds->ds_ctl0);
        ds->ds_ctl1 = __bswap32(ds->ds_ctl1);
        ds->ds_hw[0] = __bswap32(ds->ds_hw[0]);
        ds->ds_hw[1] = __bswap32(ds->ds_hw[1]);
        ds->ds_hw[2] = __bswap32(ds->ds_hw[2]);
        ds->ds_hw[3] = __bswap32(ds->ds_hw[3]);
}
#endif

/*
 * Processing of HW TX descriptor.
 */
HAL_STATUS
ar5212ProcTxDesc(struct ath_hal *ah, struct ath_desc *ds)
{
	struct ar5212_desc *ads = AR5212DESC(ds);

#ifdef AH_NEED_DESC_SWAP
	if ((ads->ds_txstatus1 & __bswap32(AR_Done)) == 0)
                return HAL_EINPROGRESS;

	ar5212SwapTxDesc(ds);
#else
	if ((ads->ds_txstatus1 & AR_Done) == 0)
		return HAL_EINPROGRESS;
#endif

	/* Update software copies of the HW status */
	ds->ds_txstat.ts_seqnum = MS(ads->ds_txstatus1, AR_SeqNum);
	ds->ds_txstat.ts_tstamp = MS(ads->ds_txstatus0, AR_SendTimestamp);
	ds->ds_txstat.ts_status = 0;
	if ((ads->ds_txstatus0 & AR_FrmXmitOK) == 0) {
		if (ads->ds_txstatus0 & AR_ExcessiveRetries)
			ds->ds_txstat.ts_status |= HAL_TXERR_XRETRY;
		if (ads->ds_txstatus0 & AR_Filtered)
			ds->ds_txstat.ts_status |= HAL_TXERR_FILT;
		if (ads->ds_txstatus0 & AR_FIFOUnderrun)
			ds->ds_txstat.ts_status |= HAL_TXERR_FIFO;
	}
	/*
	 * Extract the transmit rate used and mark the rate as
	 * ``alternate'' if it wasn't the series 0 rate.
	 */
    ds->ds_txstat.ts_rate = MS(ads->ds_txstatus1, AR_FinalTSIndex);
	ds->ds_txstat.ts_rssi = MS(ads->ds_txstatus1, AR_AckSigStrength);
	ds->ds_txstat.ts_shortretry = MS(ads->ds_txstatus0, AR_RTSFailCnt);
	ds->ds_txstat.ts_longretry = MS(ads->ds_txstatus0, AR_DataFailCnt);
	/*
	 * The retry count has the number of un-acked tries for the
	 * final series used.  When doing multi-rate retry we must
	 * fixup the retry count by adding in the try counts for
	 * each series that was fully-processed.  Beware that this
	 * takes values from the try counts in the final descriptor.
	 * These are not required by the hardware.  We assume they
	 * are placed there by the driver as otherwise we have no
	 * access and the driver can't do the calculation because it
	 * doesn't know the descriptor format.
	 */
	switch (MS(ads->ds_txstatus1, AR_FinalTSIndex)) {
	case 3:
		ds->ds_txstat.ts_longretry +=
			MS(ads->ds_ctl2, AR_XmitDataTries2);
	case 2:
		ds->ds_txstat.ts_longretry +=
			MS(ads->ds_ctl2, AR_XmitDataTries1);
	case 1:
		ds->ds_txstat.ts_longretry +=
			MS(ads->ds_ctl2, AR_XmitDataTries0);
	}
	ds->ds_txstat.ts_virtcol = MS(ads->ds_txstatus0, AR_VirtCollCnt);
	ds->ds_txstat.ts_antenna = (ads->ds_txstatus1 & AR_XmitAtenna ? 2 : 1);

	return HAL_OK;
}

/*
 * Determine which tx queues need interrupt servicing.
 */
void
ar5212GetTxIntrQueue(struct ath_hal *ah, u_int32_t *txqs)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	*txqs &= ahp->ah_intrTxqs;
	ahp->ah_intrTxqs &= ~(*txqs);
}

#ifdef AH_PRIVATE_DIAG
void
ar5212_ContTxMode(struct ath_hal *ah, struct ath_desc *ds, int mode)
{
	static int qnum =0;
	int i;
	unsigned int qbits, val, val1, val2;
	int prefetch;
	struct ar5212_desc *ads = AR5212DESC(ds);

	if(mode == 10) return;

    if (mode==7) {			// print status from the cont tx desc
		if (ads) {
			val1 = ads->ds_txstatus0;
			val2 = ads->ds_txstatus1;
			ath_hal_printf(ah, "s0(%x) s1(%x)\n", 
						   (unsigned)val1, (unsigned)val2);
		}
		ath_hal_printf(ah, "txe(%x) txd(%x)\n",
					   OS_REG_READ(ah, AR_Q_TXE),
					   OS_REG_READ(ah, AR_Q_TXD)
			);
		for(i=0;i<HAL_NUM_TX_QUEUES; i++) {
			val = OS_REG_READ(ah, AR_QTXDP(i));
			val2 = OS_REG_READ(ah, AR_QSTS(i)) & AR_Q_STS_PEND_FR_CNT;
			ath_hal_printf(ah, "[%d] %x %d\n", i, val, val2);
		}
		return;
    }
    if (mode==8) {			// set TXE for qnum
		OS_REG_WRITE(ah, AR_Q_TXE, 1<<qnum);
		return;
    }
    if (mode==9) {
		prefetch = (int)ds;
		return;
    }
	
    if (mode >= 1) {	    		// initiate cont tx operation 
		/* Disable AGC to A2 */
		qnum = (int) ds;  

		OS_REG_WRITE(ah, AR_PHY_TEST,
					 (OS_REG_READ(ah, AR_PHY_TEST) | PHY_AGC_CLR) );
        
		OS_REG_WRITE(ah, 0x9864, OS_REG_READ(ah, 0x9864) | 0x7f000);
		OS_REG_WRITE(ah, 0x9924, OS_REG_READ(ah, 0x9924) | 0x7f00fe);
		OS_REG_WRITE(ah, AR_DIAG_SW,
					 (OS_REG_READ(ah, AR_DIAG_SW) | (DIAG_FORCE_RXCLR+DIAG_IGNORE_NAV)) );
	

		OS_REG_WRITE(ah, AR_CR, AR_CR_RXD);	// set receive disable

		if (mode == 3 || mode == 4) {
			int txcfg;

			if (mode == 3) {
				OS_REG_WRITE(ah, AR_DLCL_IFS(qnum), 0);
				OS_REG_WRITE(ah, AR_DRETRY_LIMIT(qnum), 0xffffffff);
				OS_REG_WRITE(ah, AR_D_GBL_IFS_SIFS, 100);
				OS_REG_WRITE(ah, AR_D_GBL_IFS_EIFS, 100);
				OS_REG_WRITE(ah, AR_TIME_OUT, 2);
				OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, 100);
			}
			OS_REG_WRITE(ah, AR_DRETRY_LIMIT(qnum), 0xffffffff);
			OS_REG_WRITE(ah, AR_D_FPCTL, 0x10|qnum);	// enable prefetch on qnum
			txcfg = 5 | (6<<AR_FTRIG_S);
			OS_REG_WRITE(ah, AR_TXCFG, txcfg);

			OS_REG_WRITE(ah, AR_QMISC(qnum),	// set QCU modes
						 AR_Q_MISC_DCU_EARLY_TERM_REQ
						 +AR_Q_MISC_FSP_ASAP
						 +AR_Q_MISC_CBR_INCR_DIS1
						 +AR_Q_MISC_CBR_INCR_DIS0
				);

			/* stop tx dma all all except qnum */
			qbits = 0x3ff;
			qbits &= ~(1<<qnum);
			for (i=0; i<10; i++) {
				if (i==qnum) continue;
				OS_REG_WRITE(ah, AR_Q_TXD, 1<<i);
			}
			OS_REG_WRITE(ah, AR_Q_TXD, qbits);

			/* clear and freeze MIB counters */
			OS_REG_WRITE(ah, AR_MIBC, AR_MIBC_CMC);
			OS_REG_WRITE(ah, AR_MIBC, AR_MIBC_FMC);
		
			OS_REG_WRITE(ah, AR_DMISC(qnum),
						 (AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL << AR_D_MISC_ARB_LOCKOUT_CNTRL_S)
						 +(AR_D_MISC_ARB_LOCKOUT_IGNORE)
						 +(AR_D_MISC_POST_FR_BKOFF_DIS)
						 +(AR_D_MISC_VIRT_COLL_POLICY)
						 +(AR_D_MISC_VIR_COL_HANDLING_IGNORE)
				);

			for(i=0; i<HAL_NUM_TX_QUEUES+2; i++) {	// disconnect QCUs
				if (i==qnum) continue;
				OS_REG_WRITE(ah, AR_DQCUMASK(i), 0);
			}
		}
    }
    if (mode == 0) {

		OS_REG_WRITE(ah, AR_PHY_TEST,
					 (OS_REG_READ(ah, AR_PHY_TEST) & ~PHY_AGC_CLR) );
		OS_REG_WRITE(ah, AR_DIAG_SW,
					 (OS_REG_READ(ah, AR_DIAG_SW) & ~(DIAG_FORCE_RXCLR+DIAG_IGNORE_NAV)) );
    }
}
#endif

#endif /* AH_SUPPORT_AR5212 */
