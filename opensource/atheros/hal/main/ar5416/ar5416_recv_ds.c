/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_recv_ds.c#5 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_desc.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416desc_10.h"
#include "ar5416/ar5416desc_20.h"


/*
 * Initialize RX descriptor, by clearing the status and setting
 * the size (and any other flags).
 */
HAL_BOOL
FN(ar5416SetupRxDesc)(struct ath_hal *ah, struct ath_desc *ds,
    u_int32_t size, u_int flags)
{
    struct ar5416_desc *ads = AR5416DESC(ds);

    HALASSERT((size &~ AR_BufLen) == 0);

    ads->ds_ctl1 = size & AR_BufLen;
    if (flags & HAL_RXDESC_INTREQ)
        ads->ds_ctl1 |= AR_RxIntrReq;

    /* this should be enough */
    ads->ds_rxstatus8 &= ~AR_RxDone;

    return AH_TRUE;
}

/*
 * Process an RX descriptor, and return the status to the caller.
 * Copy some hardware specific items into the software portion
 * of the descriptor.
 *
 * NB: the caller is responsible for validating the memory contents
 *     of the descriptor (e.g. flushing any cached copy).
 */
HAL_STATUS
FN(ar5416ProcRxDescFast)(struct ath_hal *ah, struct ath_desc *ds,
    u_int32_t pa, struct ath_desc *nds, struct ath_rx_status *rx_stats)
{
    struct ar5416_desc ads;
    struct ar5416_desc *adsp = AR5416DESC(ds);
    struct ar5416_desc *ands = AR5416DESC(nds);

    if ((adsp->ds_rxstatus8 & AR_RxDone) == 0)
        return HAL_EINPROGRESS;
    /*
     * Given the use of a self-linked tail be very sure that the hw is
     * done with this descriptor; the hw may have done this descriptor
     * once and picked it up again...make sure the hw has moved on.
     */
    if ((ands->ds_rxstatus8 & AR_RxDone) == 0
        && OS_REG_READ(ah, AR_RXDP) == pa)
        return HAL_EINPROGRESS;

    /*
     * Now we need to get the stats from the descriptor. Since desc are 
     * uncached, lets make a copy of the stats first. Note that, since we
     * touch most of the rx stats, a memcpy would always be more efficient
     *
     * Next we fill in all values in a caller passed stack variable.
     * This reduces the number of uncached accesses.
     * Do this copy here, after the check so that when the checks fail, we
     * dont end up copying the entire stats uselessly.
     */
    ads.u.rx = adsp->u.rx;

    rx_stats->rs_status = 0;
    rx_stats->rs_flags = 0;

    rx_stats->rs_datalen = ads.ds_rxstatus1 & AR_DataLen;
    rx_stats->rs_tstamp =  ads.AR_RcvTimestamp;

    /* XXX what about KeyCacheMiss? */
    rx_stats->rs_rssi_combined = 
                                MS(ads.ds_rxstatus4, AR_RxRSSICombined);
    rx_stats->rs_rssi_ctl0 = MS(ads.ds_rxstatus0, AR_RxRSSIAnt00);
    rx_stats->rs_rssi_ctl1 = MS(ads.ds_rxstatus0, AR_RxRSSIAnt01);
    rx_stats->rs_rssi_ctl2 = MS(ads.ds_rxstatus0, AR_RxRSSIAnt02);
    rx_stats->rs_rssi_ext0 = MS(ads.ds_rxstatus4, AR_RxRSSIAnt10);
    rx_stats->rs_rssi_ext1 = MS(ads.ds_rxstatus4, AR_RxRSSIAnt11);
    rx_stats->rs_rssi_ext2 = MS(ads.ds_rxstatus4, AR_RxRSSIAnt12);
    if (ads.ds_rxstatus8 & AR_RxKeyIdxValid)
        rx_stats->rs_keyix = MS(ads.ds_rxstatus8, AR_KeyIdx);
    else
        rx_stats->rs_keyix = HAL_RXKEYIX_INVALID;
    /* NB: caller expected to do rate table mapping */
    rx_stats->rs_rate = RXSTATUS_RATE(ah, (&ads));
    rx_stats->rs_more = (ads.ds_rxstatus1 & AR_RxMore) ? 1 : 0;

    rx_stats->rs_isaggr = (ads.ds_rxstatus8 & AR_RxAggr) ? 1 : 0;
    rx_stats->rs_moreaggr = (ads.ds_rxstatus8 & AR_RxMoreAggr) ? 1 : 0;
    rx_stats->rs_flags  |= (ads.ds_rxstatus3 & AR_GI) ? HAL_RX_GI : 0;
    rx_stats->rs_flags  |= (ads.ds_rxstatus3 & AR_2040) ? HAL_RX_2040 : 0;

    if (ads.ds_rxstatus8 & AR_PreDelimCRCErr)
        rx_stats->rs_flags |= HAL_RX_DELIM_CRC_PRE;
    if (ads.ds_rxstatus8 & AR_PostDelimCRCErr)
        rx_stats->rs_flags |= HAL_RX_DELIM_CRC_POST;
    if (ads.ds_rxstatus8 & AR_DecryptBusyErr)
        rx_stats->rs_flags |= HAL_RX_DECRYPT_BUSY;

    if ((ads.ds_rxstatus8 & AR_RxFrameOK) == 0) {
        /*
         * These four bits should not be set together.  The
         * 5416 spec states a Michael error can only occur if
         * DecryptCRCErr not set (and TKIP is used).  Experience
         * indicates however that you can also get Michael errors
         * when a CRC error is detected, but these are specious.
         * Consequently we filter them out here so we don't
         * confuse and/or complicate drivers.
         */
        if (ads.ds_rxstatus8 & AR_CRCErr)
            rx_stats->rs_status |= HAL_RXERR_CRC;
        else if (ads.ds_rxstatus8 & AR_PHYErr) {
            u_int phyerr;

            rx_stats->rs_status |= HAL_RXERR_PHY;
            phyerr = MS(ads.ds_rxstatus8, AR_PHYErrCode);
            rx_stats->rs_phyerr = phyerr;
#if 0
            if ((!AH5416(ah)->ah_hasHwPhyCounters) &&
                (phyerr != HAL_PHYERR_RADAR))
                ar5416AniPhyErrReport(ah, rx_stats);
            if (phyerr == HAL_PHYERR_RADAR)
                ar5416ProcessRadar(ah, ds, rx_stats);
#endif
        } else if (ads.ds_rxstatus8 & AR_DecryptCRCErr)
            rx_stats->rs_status |= HAL_RXERR_DECRYPT;
        else if (ads.ds_rxstatus8 & AR_MichaelErr)
            rx_stats->rs_status |= HAL_RXERR_MIC;
    }
    return HAL_OK;
}
#endif /* AH_SUPPORT_AR5416 */
