/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5212/ar5212_interrupts.c#1 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#include "ah.h"
#include "ah_internal.h"

#include "ar5212/ar5212.h"
#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212phy.h"


/*
 * Checks to see if an interrupt is pending on our NIC
 *
 * Returns: TRUE    if an interrupt is pending
 *          FALSE   if not
 */
HAL_BOOL
ar5212IsInterruptPending(struct ath_hal *ah)
{
	/* 
	 * Some platforms trigger our ISR before applying power to
	 * the card, so make sure the INTPEND is really 1, not 0xffffffff.
	 */
	return (OS_REG_READ(ah, AR_INTPEND) == AR_INTPEND_TRUE);
}

/*
 * Reads the Interrupt Status Register value from the NIC, thus deasserting
 * the interrupt line, and returns both the masked and unmasked mapped ISR
 * values.  The value returned is mapped to abstract the hw-specific bit
 * locations in the Interrupt Status Register.
 *
 * Returns: A hardware-abstracted bitmap of all non-masked-out
 *          interrupts pending, as well as an unmasked value
 */
HAL_BOOL
ar5212GetPendingInterrupts(struct ath_hal *ah, HAL_INT *masked)
{
	u_int32_t isr;

	isr = OS_REG_READ(ah, AR_ISR_RAC);
	if (isr == 0xffffffff) {
		*masked = 0;
		return AH_FALSE;;
	}

	*masked = isr & HAL_INT_COMMON;

	if (isr & AR_ISR_HIUERR) {
		HALDEBUG(ah, "%s: fatal error, ISR_RAC=0x%x ISR_S2_S=0x%x\n",
			__func__, isr, OS_REG_READ(ah, AR_ISR_S2_S));
		*masked |= HAL_INT_FATAL;
	}
	if (isr & (AR_ISR_RXOK | AR_ISR_RXERR))
		*masked |= HAL_INT_RX;
	if (isr & (AR_ISR_TXOK | AR_ISR_TXDESC | AR_ISR_TXERR | AR_ISR_TXEOL)) {
		struct ath_hal_5212 *ahp = AH5212(ah);
		*masked |= HAL_INT_TX;
		ahp->ah_intrTxqs |= MS(OS_REG_READ(ah, AR_ISR_S0_S), AR_ISR_S0_QCU_TXOK);
		ahp->ah_intrTxqs |= MS(OS_REG_READ(ah, AR_ISR_S0_S), AR_ISR_S0_QCU_TXDESC);
		ahp->ah_intrTxqs |= MS(OS_REG_READ(ah, AR_ISR_S1_S), AR_ISR_S1_QCU_TXERR);
		ahp->ah_intrTxqs |= MS(OS_REG_READ(ah, AR_ISR_S1_S), AR_ISR_S1_QCU_TXEOL);
	}

	/*
	 * Receive overrun is usually non-fatal on Oahu/Spirit.
	 *
	 * BUT early silicon had a bug which causes the rx to fail
	 * and the chip must be reset.
	 *
	 * AND even AR5311 v3.1 and AR5211 v4.2 silicon seems to
	 * have a problem with RXORN which hangs the receive path
	 * and requires a chip reset to proceed (see bug 3996).
	 * So for now, we force a hardware reset in all cases.
	 */
	if (isr & AR_ISR_RXORN) {
		HALDEBUG(ah, "%s: receive FIFO overrun interrupt\n", __func__);
		*masked |= HAL_INT_FATAL;
	}
	return AH_TRUE;
}

HAL_INT
ar5212GetInterrupts(struct ath_hal *ah)
{
	return AH5212(ah)->ah_maskReg;
}

/*
 * Atomically enables NIC interrupts.  Interrupts are passed in
 * via the enumerated bitmask in ints.
 */
HAL_INT
ar5212SetInterrupts(struct ath_hal *ah, HAL_INT ints)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	u_int32_t omask = ahp->ah_maskReg;
	u_int32_t mask;

	HALDEBUG(ah, "%s: 0x%x => 0x%x\n", __func__, omask, ints);

	if (omask & HAL_INT_GLOBAL) {
		HALDEBUG(ah, "%s: disable IER\n", __func__);
		OS_REG_WRITE(ah, AR_IER, AR_IER_DISABLE);
		(void) OS_REG_READ(ah, AR_IER);   /* flush write to HW */
	}

	mask = ints & HAL_INT_COMMON;
	if (ints & HAL_INT_TX) {
		if (ahp->ah_txOkInterruptMask)
			mask |= AR_IMR_TXOK;
		if (ahp->ah_txErrInterruptMask)
			mask |= AR_IMR_TXERR;
		if (ahp->ah_txDescInterruptMask)
			mask |= AR_IMR_TXDESC;
		if (ahp->ah_txEolInterruptMask)
			mask |= AR_IMR_TXEOL;
	}
	if (ints & HAL_INT_RX)
		mask |= AR_IMR_RXOK | AR_IMR_RXERR | AR_IMR_RXDESC;
	if (ints & HAL_INT_FATAL) {
		/*
		 * NB: ar5212Reset sets MCABT+SSERR+DPERR in AR_IMR_S2
		 *     so enabling HIUERR enables delivery.
		 */
		mask |= AR_IMR_HIUERR;
	}

	/* Write the new IMR and store off our SW copy. */
	HALDEBUG(ah, "%s: new IMR 0x%x\n", __func__, mask);
	OS_REG_WRITE(ah, AR_IMR, mask);
	ahp->ah_maskReg = ints;

	/* Re-enable interrupts if they were enabled before. */
	if (ints & HAL_INT_GLOBAL) {
		HALDEBUG(ah, "%s: enable IER\n", __func__);
		OS_REG_WRITE(ah, AR_IER, AR_IER_ENABLE);
	}

	return omask;
}
#endif /* AH_SUPPORT_AR5212 */
