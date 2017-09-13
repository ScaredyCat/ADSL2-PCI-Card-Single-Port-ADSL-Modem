/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_interrupts.c#12 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"


#define AR_INTR_SPURIOUS        0xffffffff

/*
 * Checks to see if an interrupt is pending on our NIC
 *
 * Returns: TRUE    if an interrupt is pending
 *          FALSE   if not
 */
HAL_BOOL
ar5416IsInterruptPending(struct ath_hal *ah)
{
    u_int32_t host_isr = OS_REG_READ(ah, AR_INTR_ASYNC_CAUSE);
    /*
     * Some platforms trigger our ISR before applying power to
     * the card, so make sure.
     */
    return ((host_isr != AR_INTR_SPURIOUS) && (host_isr & AR_INTR_MAC_IRQ));
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
ar5416GetPendingInterrupts(struct ath_hal *ah, HAL_INT *masked)
{
    u_int32_t isr;

    isr = OS_REG_READ(ah, AR_ISR_RAC);
    if (isr == 0xffffffff) {
        *masked = 0;
        return AH_FALSE;
    }

    *masked = isr & HAL_INT_COMMON;

#ifdef AR5416_INT_MITIGATION
    if (isr & (AR_ISR_RXMINTR | AR_ISR_RXINTM)) {
        *masked |= HAL_INT_RX;
    }
    if (isr & (AR_ISR_TXMINTR | AR_ISR_TXINTM)) {
        *masked |= HAL_INT_TX;
    }
#endif

    if (isr & AR_ISR_BCNMISC) {
        u_int32_t s2_s;

        s2_s = OS_REG_READ(ah, AR_ISR_S2_S);
        HALDEBUG(ah, "%s: BCNMISC, ISR_RAC=0x%x ISR_S2_S=0x%x\n", __func__,
                 isr, s2_s);

        if (s2_s & AR_ISR_S2_GTT) {
            *masked |= HAL_INT_GTT;
        }
    }

    if (isr & (AR_ISR_RXOK | AR_ISR_RXERR))
        *masked |= HAL_INT_RX;
    if (isr & (AR_ISR_TXOK | AR_ISR_TXDESC | AR_ISR_TXERR | AR_ISR_TXEOL)) {
        struct ath_hal_5416 *ahp = AH5416(ah);
        u_int32_t           s0_s, s1_s;

        *masked |= HAL_INT_TX;
        s0_s = OS_REG_READ(ah, AR_ISR_S0_S);
        s1_s = OS_REG_READ(ah, AR_ISR_S1_S);
        ahp->ah_intrTxqs |= MS(s0_s, AR_ISR_S0_QCU_TXOK);
        ahp->ah_intrTxqs |= MS(s0_s, AR_ISR_S0_QCU_TXDESC);
        ahp->ah_intrTxqs |= MS(s1_s, AR_ISR_S1_QCU_TXERR);
        ahp->ah_intrTxqs |= MS(s1_s, AR_ISR_S1_QCU_TXEOL);
    }

    /*
     * Do not treat receive overflows as fatal for owl.
     */
    if (isr & AR_ISR_RXORN) {
        HALDEBUG(ah, "%s: receive FIFO overrun interrupt\n", __func__);
        // *masked |= HAL_INT_FATAL;
    }

    return AH_TRUE;
}

HAL_INT
ar5416GetInterrupts(struct ath_hal *ah)
{
    return AH5416(ah)->ah_maskReg;
}

/*
 * Atomically enables NIC interrupts.  Interrupts are passed in
 * via the enumerated bitmask in ints.
 */
HAL_INT
ar5416SetInterrupts(struct ath_hal *ah, HAL_INT ints)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
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
#ifdef AR5416_INT_MITIGATION
        mask |= AR_IMR_TXMINTR | AR_IMR_TXINTM;
#else
        if (ahp->ah_txOkInterruptMask)
            mask |= AR_IMR_TXOK;
        if (ahp->ah_txDescInterruptMask)
            mask |= AR_IMR_TXDESC;
#endif
        if (ahp->ah_txErrInterruptMask)
            mask |= AR_IMR_TXERR;
        if (ahp->ah_txEolInterruptMask)
            mask |= AR_IMR_TXEOL;
    }
    if (ints & HAL_INT_RX) {
        mask |= AR_IMR_RXERR;
#ifdef AR5416_INT_MITIGATION
        mask |=  AR_IMR_RXMINTR | AR_IMR_RXINTM;
#else
        mask |= AR_IMR_RXOK | AR_IMR_RXDESC;
#endif
    }

    if (ints & (HAL_INT_GTT | HAL_INT_CST)) {
        mask |= AR_IMR_BCNMISC;
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

    OS_REG_WRITE(ah, AR_INTR_ASYNC_ENABLE, AR_INTR_MAC_IRQ);
    OS_REG_WRITE(ah, AR_INTR_ASYNC_MASK, AR_INTR_MAC_IRQ);


    /*
     * debug - enable to see all synchronous interrupts status
     */
    OS_REG_WRITE(ah, AR_INTR_SYNC_ENABLE, AR_INTR_SYNC_ALL);

    return omask;
}
#endif /* AH_SUPPORT_AR5416 */
