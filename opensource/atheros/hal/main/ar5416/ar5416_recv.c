/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_recv.c#14 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_desc.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416desc.h"

/*
 * Get the RXDP.
 */
u_int32_t
ar5416GetRxDP(struct ath_hal *ath)
{
    return OS_REG_READ(ath, AR_RXDP);
}

/*
 * Set the RxDP.
 */
void
ar5416SetRxDP(struct ath_hal *ah, u_int32_t rxdp)
{
    OS_REG_WRITE(ah, AR_RXDP, rxdp);
    HALASSERT(OS_REG_READ(ah, AR_RXDP) == rxdp);
}

/*
 * Set Receive Enable bits.
 */
void
ar5416EnableReceive(struct ath_hal *ah)
{
    OS_REG_WRITE(ah, AR_CR, AR_CR_RXE);
}

/*
 * Stop Receive at the DMA engine
 */
HAL_BOOL
ar5416StopDmaReceive(struct ath_hal *ah)
{
    OS_REG_WRITE(ah, AR_CR, AR_CR_RXD); /* Set receive disable bit */
    if (!ath_hal_wait(ah, AR_CR, AR_CR_RXE, 0)) {
#ifdef AH_DEBUG
        ath_hal_printf(ah, "%s: dma failed to stop in 10ms\n"
            "AR_CR=0x%08x\nAR_DIAG_SW=0x%08x\n",
            __func__,
            OS_REG_READ(ah, AR_CR),
            OS_REG_READ(ah, AR_DIAG_SW));
#endif
        return AH_FALSE;
    } else {
        return AH_TRUE;
    }
}

/*
 * Start Transmit at the PCU engine (unpause receive)
 */
void
ar5416StartPcuReceive(struct ath_hal *ah)
{
    OS_REG_CLR_BIT(ah, AR_DIAG_SW,
                   (AR_DIAG_RX_DIS | AR_DIAG_RX_ABORT));

    ar5416EnableMIBCounters(ah);

#if 0
    ar5416AniReset(ah);
#endif
}

/*
 * Stop Transmit at the PCU engine (pause receive)
 */
void
ar5416StopPcuReceive(struct ath_hal *ah)
{
    OS_REG_SET_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_DIS);

    ar5416DisableMIBCounters(ah);

#if 0
    ar5416RadarDisable(ah);
#endif
}

/*
 * Abort current ingress frame at the PCU
 */
void
ar5416AbortPcuReceive(struct ath_hal *ah)
{
    OS_REG_SET_BIT(ah, AR_DIAG_SW, AR_DIAG_RX_ABORT | AR_DIAG_RX_DIS);

    ar5416DisableMIBCounters(ah);

#if 0
    ar5416RadarDisable(ah);
#endif
}

/*
 * Set multicast filter 0 (lower 32-bits)
 *               filter 1 (upper 32-bits)
 */
void
ar5416SetMulticastFilter(struct ath_hal *ah, u_int32_t filter0, u_int32_t filter1)
{
    OS_REG_WRITE(ah, AR_MCAST_FIL0, filter0);
    OS_REG_WRITE(ah, AR_MCAST_FIL1, filter1);
}

/*
 * Clear multicast filter by index
 */
HAL_BOOL
ar5416ClrMulticastFilterIndex(struct ath_hal *ah, u_int32_t ix)
{
    u_int32_t val;

    if (ix >= 64)
        return AH_FALSE;
    if (ix >= 32) {
        val = OS_REG_READ(ah, AR_MCAST_FIL1);
        OS_REG_WRITE(ah, AR_MCAST_FIL1, (val &~ (1<<(ix-32))));
    } else {
        val = OS_REG_READ(ah, AR_MCAST_FIL0);
        OS_REG_WRITE(ah, AR_MCAST_FIL0, (val &~ (1<<ix)));
    }
    return AH_TRUE;
}

/*
 * Set multicast filter by index
 */
HAL_BOOL
ar5416SetMulticastFilterIndex(struct ath_hal *ah, u_int32_t ix)
{
    u_int32_t val;

    if (ix >= 64)
        return AH_FALSE;
    if (ix >= 32) {
        val = OS_REG_READ(ah, AR_MCAST_FIL1);
        OS_REG_WRITE(ah, AR_MCAST_FIL1, (val | (1<<(ix-32))));
    } else {
        val = OS_REG_READ(ah, AR_MCAST_FIL0);
        OS_REG_WRITE(ah, AR_MCAST_FIL0, (val | (1<<ix)));
    }
    return AH_TRUE;
}

/*
 * Get the receive filter.
 */
u_int32_t
ar5416GetRxFilter(struct ath_hal *ah)
{
    u_int32_t bits = OS_REG_READ(ah, AR_RX_FILTER);
    u_int32_t phybits = OS_REG_READ(ah, AR_PHY_ERR);
    if (phybits & AR_PHY_ERR_RADAR)
        bits |= HAL_RX_FILTER_PHYRADAR;
    if (phybits & (AR_PHY_ERR_OFDM_TIMING|AR_PHY_ERR_CCK_TIMING))
        bits |= HAL_RX_FILTER_PHYERR;
    return bits;
}

/*
 * Set the receive filter.
 */
void
ar5416SetRxFilter(struct ath_hal *ah, u_int32_t bits)
{
    u_int32_t phybits;

    OS_REG_WRITE(ah, AR_RX_FILTER, (bits & 0xff) | AR_RX_COMPR_BAR);
    phybits = 0;
    if (bits & HAL_RX_FILTER_PHYRADAR)
        phybits |= AR_PHY_ERR_RADAR;
    if (bits & HAL_RX_FILTER_PHYERR)
        phybits |= AR_PHY_ERR_OFDM_TIMING | AR_PHY_ERR_CCK_TIMING;
    OS_REG_WRITE(ah, AR_PHY_ERR, phybits);
    if (phybits) {
        OS_REG_WRITE(ah, AR_RXCFG,
            OS_REG_READ(ah, AR_RXCFG) | AR_RXCFG_ZLFDMA);
    } else {
        OS_REG_WRITE(ah, AR_RXCFG,
            OS_REG_READ(ah, AR_RXCFG) &~ AR_RXCFG_ZLFDMA);
    }
}

#endif /* AH_SUPPORT_AR5416 */
