/*
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/ratectrl11n/ratectrl_11n_ln.c#21 $
 *
 * Copyright (c) 2001-2003 Atheros Communications, Inc., All Rights Reserved
 *
 * DESCRIPTION
 *
 * This file contains the data structures and routines for transmit rate
 * control operating on the linearized rates of different phys
 *
 *
 */

#ifdef __FreeBSD__
#include <dev/ath/ath_rate/atheros/ratectrl.h>
#else
#include "ratectrl11n.h"
#endif

#ifndef DEBUG_PKTLOG
#define DEBUG_PKTLOG 1
#endif

#ifdef DEBUG_PKTLOG
#include <pktlog_rc.h>
extern struct ath_pktlog_rcfuncs *g_pktlog_rcfuncs;
#endif

#define MULTI_RATE_RETRY_ENABLE 1

#if 0

static void
dump_phy_rate_index(TX_RATE_CTRL *pRc)
{
    A_UINT32 i=0, j=0;

    printk("Valid PHY rate table:-\n");

    for (i=0; i<WLAN_RC_PHY_MAX;i++)
        for (j=0; j<pRc->validPhyRateCount[i]; j++)
            printk("Phy:%d, Index:%d, value:%d\n", i, j,
                   pRc->validPhyRateIndex[i][j]);
}


/* Dump all the valid rate index */

static void
dump_valid_rate_index(RATE_TABLE *pRateTable, A_UINT8 *validRateIndex)
{
    A_UINT32 i=0;

    printk("Valid Rate Table:-\n");
    for(i=0; i<MAX_TX_RATE_TBL; i++)
    printk(" Index:%d, value:%d, code:%x, rate:%d, flag:%x\n", i, (int)validRateIndex[i], 
           pRateTable->info[(int)validRateIndex[i]].rateCode, 
           pRateTable->info[(int)validRateIndex[i]].rateKbps,
           WLAN_RC_PHY_40(pRateTable->info[(int)validRateIndex[i]].phy));
}

#endif /* #if 0 */

static void
rcSortValidRates(const RATE_TABLE *pRateTable, TX_RATE_CTRL *pRc)
{
    A_UINT8 i,j;

    for (i=pRc->maxValidRate-1; i > 0; i--) {
        for (j=0; j <= i-1; j++) {
            if (pRateTable->info[pRc->validRateIndex[j]].rateKbps >
                pRateTable->info[pRc->validRateIndex[j+1]].rateKbps)
            {
                A_UINT8 tmp=0;
                tmp = pRc->validRateIndex[j];
                pRc->validRateIndex[j] = pRc->validRateIndex[j+1];
                pRc->validRateIndex[j+1] = tmp;
            }
        }
    }
}


/* Access functions for validTxRateMask */

static void
rcInitValidTxMask(TX_RATE_CTRL *pRc)
{
    A_UINT8 i;

    for (i = 0; i < pRc->rateTableSize; i++) {
        pRc->validRateIndex[i] = FALSE;
    }
}

static INLINE  void
rcSetValidTxMask(TX_RATE_CTRL *pRc, A_UINT8 index, A_BOOL validTxRate)
{
    ASSERT(index < pRc->rateTableSize);
    pRc->validRateIndex[index] = validTxRate ? TRUE : FALSE;

}

static INLINE A_BOOL
rcIsValidTxMask(TX_RATE_CTRL *pRc, A_UINT8 index)
{
    ASSERT(index < pRc->rateTableSize);
    return (pRc->validRateIndex[index]);
}

/* Iterators for validTxRateMask */
static INLINE A_BOOL
rcGetNextValidTxRate(const RATE_TABLE *pRateTable, TX_RATE_CTRL *pRc, 
                     A_UINT8 curValidTxRate, A_UINT8 *pNextIndex)
{
    A_UINT8 i;

    for (i = 0; i < pRc->maxValidRate-1; i++) {
        if (pRc->validRateIndex[i] == curValidTxRate) {
            *pNextIndex = pRc->validRateIndex[i+1];
            return TRUE;
        }
    }

    /* No more valid rates */
    *pNextIndex = 0;
    
    return FALSE;
}


INLINE A_BOOL
rcGetNextLowerValidTxRate(const RATE_TABLE *pRateTable, TX_RATE_CTRL *pRc,  
                          A_UINT8 curValidTxRate, A_UINT8 *pNextIndex)
{
    A_INT8 i;

    *pNextIndex = 0;

    for (i = 1; i < pRc->maxValidRate ; i++) {
        if (pRc->validRateIndex[i] == curValidTxRate) {
            *pNextIndex = pRc->validRateIndex[i-1];
            return TRUE;
        }
    }

    return FALSE;
}

/* Return true only for single stream */

static A_BOOL
rcIsValidPhyRate(A_UINT32 phy, A_UINT32 capflag, A_BOOL ignoreCW)
{
    if (WLAN_RC_PHY_HT(phy) & !(capflag & WLAN_RC_HT_FLAG)) {
        return FALSE;
    }

    if (WLAN_RC_PHY_DS(phy) && !(capflag & WLAN_RC_DS_FLAG))  {
        return FALSE;
    }

    if (WLAN_RC_PHY_SGI(phy) && !(capflag & WLAN_RC_SGI_FLAG)) {
        return FALSE;
    }

    if (!ignoreCW && WLAN_RC_PHY_HT(phy)) {
        if (WLAN_RC_PHY_40(phy) && !(capflag & WLAN_RC_40_FLAG)) {
            return FALSE;
        }

        if (!WLAN_RC_PHY_40(phy) && (capflag & WLAN_RC_40_FLAG)) {
            return FALSE;
        }
    }
    
    return TRUE;
}

/* 
 * Initialize the Valid Rate Index from valid entries in Rate Table 
 */
static A_UINT8
rcSibInitValidRates(const RATE_TABLE *pRateTable, TX_RATE_CTRL *pRc, A_UINT32 capflag)
{
    A_UINT8 i, hi = 0;
    
    for (i = 0; i < pRateTable->rateCount; i++) {
        if (pRateTable->info[i].valid == TRUE) {
            A_UINT32 phy = pRateTable->info[i].phy;

            if (!rcIsValidPhyRate(phy, capflag, FALSE)) 
                continue;
            pRc->validPhyRateIndex[phy][pRc->validPhyRateCount[phy]] = i;
            pRc->validPhyRateCount[phy] += 1;
            rcSetValidTxMask(pRc, i, TRUE);
            hi = A_MAX(hi, i);
        }
    } 
    
    return hi;
}

/* 
 * Initialize the Valid Rate Index from Rate Set 
 */
static A_UINT8
rcSibSetValidRates(const RATE_TABLE *pRateTable, TX_RATE_CTRL *pRc, 
                   struct ieee80211_rateset *pRateSet, A_UINT32 capflag)
{
    A_UINT8 i, j, hi = 0;

    /* Use intersection of working rates and valid rates */
    for (i = 0; i < pRateSet->rs_nrates; i++) {
        for (j = 0; j < pRateTable->rateCount; j++) {
            A_UINT32 phy = pRateTable->info[j].phy;

            /*
             * We allow a rate only if its valid and the capflag matches one of
             * the validity (TRUE/TRUE_20/TRUE_40) flags
             */

            if (((pRateSet->rs_rates[i] & 0x7F) == 
                 (pRateTable->info[j].dot11Rate & 0x7F))
                && ((pRateTable->info[j].valid & WLAN_RC_CAP_MODE(capflag)) == 
                    WLAN_RC_CAP_MODE(capflag)) && !WLAN_RC_PHY_HT(phy))
            {
                if (!rcIsValidPhyRate(phy, capflag, FALSE)) 
                    continue;

                pRc->validPhyRateIndex[phy][pRc->validPhyRateCount[phy]] = j;
                pRc->validPhyRateCount[phy] += 1;
                rcSetValidTxMask(pRc, j, TRUE);
                hi = A_MAX(hi, j);
            }
        }
    }
    
    return hi;
}

static A_UINT8
rcSibSetValidHtRates(const RATE_TABLE *pRateTable, TX_RATE_CTRL *pRc, 
                     A_UINT8 *pMcsSet, A_UINT32 capflag)
{
    A_UINT8 i, j, hi = 0;

    /* Use intersection of working rates and valid rates */
    for (i = 0; i <  ((struct ieee80211_rateset *)pMcsSet)->rs_nrates; i++) {
        for (j = 0; j < pRateTable->rateCount; j++) {
            A_UINT32 phy = pRateTable->info[j].phy;
               
            if (((((struct ieee80211_rateset *)pMcsSet)->rs_rates[i] & 0x7F) 
                != (pRateTable->info[j].dot11Rate & 0x7F)) 
                || !WLAN_RC_PHY_HT(phy) 
                || !WLAN_RC_PHY_HT_VALID(pRateTable->info[j].valid, capflag))
            {
                continue;
            }
    
            if (!rcIsValidPhyRate(phy, capflag, FALSE)) 
                continue;

            pRc->validPhyRateIndex[phy][pRc->validPhyRateCount[phy]] = j;
            pRc->validPhyRateCount[phy] += 1;
            rcSetValidTxMask(pRc, j, TRUE);
            hi = A_MAX(hi, j);
        }
    }

    return hi;
}

/*
 *  Update the SIB's rate control information
 *
 *  This should be called when the supported rates change
 *  (e.g. SME operation, wireless mode change)
 *
 *  It will determine which rates are valid for use.
 */
static void
rcSibUpdate_ht(struct ath_softc *sc, struct ath_node *an, A_UINT32 capflag, A_BOOL keepState)
{
    RATE_TABLE                *pRateTable = 0;
    struct atheros_node       *pSib       = ATH_NODE_ATHEROS(an);
    struct atheros_softc      *asc        = (struct atheros_softc*)sc->sc_rc;
    struct ieee80211_rateset  *pRateSet   = &an->an_node.ni_rates;
    A_UINT8                   *phtMcs     = (A_UINT8*)&an->an_node.ni_htrates;
    TX_RATE_CTRL              *pRc         = (TX_RATE_CTRL *)(pSib);
    
    A_UINT8 i, j, k, hi = 0, htHi = 0;

    pRateTable = (RATE_TABLE*)asc->hwRateTable[sc->sc_curmode];

    /* Initial rate table size. Will change depending on the working rate set */
    pRc->rateTableSize = MAX_TX_RATE_TBL;

    pRc->switchCount = 0;
    /* Initialize thresholds according to the global rate table */
    for (i = 0 ; (i < pRc->rateTableSize) && (!keepState); i++) {
        pRc->state[i].rssiThres = pRateTable->info[i].rssiAckValidMin;
        pRc->state[i].per       = 0;
    }

    /* Determine the valid rates */
    rcInitValidTxMask(pRc);

    for (i = 0; i < WLAN_RC_PHY_MAX; i++) {
        for (j = 0; j < MAX_TX_RATE_PHY; j++) {
            pRc->validPhyRateIndex[i][j] = 0;
        }   
        pRc->validPhyRateCount[i] = 0;
    }

    pRc->rcPhyState = 0;
    pRc->rcPhyMode = (capflag & WLAN_RC_40_FLAG);

    if (!pRateSet->rs_nrates) {
        /* No working rate, just initialize valid rates */
        hi = rcSibInitValidRates(pRateTable, pRc, capflag);
    } else {
        /* Use intersection of working rates and valid rates */
        hi = rcSibSetValidRates(pRateTable, pRc, pRateSet, capflag);
        
        if (capflag & WLAN_RC_HT_FLAG) {
            htHi = rcSibSetValidHtRates(pRateTable, pRc, phtMcs, capflag);
        }

        hi = A_MAX(hi, htHi);
    }

    pRc->rateTableSize = hi + 1;
    pRc->rateMaxPhy    = 0;
    
    ASSERT(pRc->rateTableSize <= MAX_TX_RATE_TBL);

    for (i = 0, k = 0; i < WLAN_RC_PHY_MAX; i++) {
        for (j = 0; j < pRc->validPhyRateCount[i]; j++) {
            pRc->validRateIndex[k++] = pRc->validPhyRateIndex[i][j];
        }   

        if (!rcIsValidPhyRate(i, pRateTable->initialRateMax, TRUE) || !pRc->validPhyRateCount[i]) 
            continue;

        pRc->rateMaxPhy = pRc->validPhyRateIndex[i][j-1];
    }

    ASSERT(pRc->rateTableSize <= MAX_TX_RATE_TBL);
    ASSERT(k <= MAX_TX_RATE_TBL);

    pRc->rateMaxPhy = pRc->validRateIndex[k-4];

    pRc->maxValidRate = k;

    rcSortValidRates(pRateTable, pRc);

    //dump_valid_rate_index(pRateTable, pRc->validRateIndex);
    //printk("RateTable:%d, maxvalidrate:%d, ratemax:%d\n", pRc->rateTableSize,k,pRc->rateMaxPhy);
}

void
rcSibUpdate(struct ath_softc *sc, struct ath_node *pSib, 
            A_UINT32 capflag, A_BOOL keepState)
{
    rcSibUpdate_ht(sc, 
                   pSib, 
                   ((capflag & ATH_RC_DS_FLAG)   ? WLAN_RC_DS_FLAG  : 0) |
                   ((capflag & ATH_RC_SGI_FLAG)  ? WLAN_RC_SGI_FLAG : 0) | 
                   ((capflag & ATH_RC_HT_FLAG)   ? WLAN_RC_HT_FLAG  : 0) |
                   ((capflag & ATH_RC_CW40_FLAG) ? WLAN_RC_40_FLAG  : 0), 
                   keepState);
}

/*
 *  This routine is called to initialize the rate control parameters
 *  in the SIB. It is called initially during system initialization
 *  or when a station is associated with the AP.
 */
void
rcSibInit(struct ath_softc *sc, struct ath_node *an)
{
    struct atheros_node    *pSib = ATH_NODE_ATHEROS(an);
    TX_RATE_CTRL           *pRc  = (TX_RATE_CTRL *)(pSib);

    A_MEM_ZERO((char *)pRc, sizeof(*pRc));
    pRc->rssiDownTime = A_MS_TICKGET();
    
    //pSib->rateInit = FALSE;
}


/*
 * Return the median of three numbers
 */
INLINE A_RSSI
median(A_RSSI a, A_RSSI b, A_RSSI c)
{
    if (a >= b) {
        if (b >= c) {
            return b;
        } else if (a > c) {
            return c;
        } else {
            return a;
        }
    } else {
        if (a >= c) {
            return a;
        } else if (b >= c) {
            return c;
        } else {
            return b;
        }
    }
}

static A_UINT8
rcRateFind_ht(struct ath_softc *sc, struct atheros_node *pSib,
          const RATE_TABLE *pRateTable, A_BOOL probeAllowed, A_BOOL *isProbing)
{
    A_UINT32             dt;
    A_UINT32             bestThruput, thisThruput;
    A_UINT32             nowMsec;
    A_UINT8              rate, nextRate, bestRate;
    A_RSSI               rssiLast, rssiReduce = 0;
    A_UINT8              maxIndex, minIndex;
    A_INT8               index;
    TX_RATE_CTRL         *pRc = NULL;
#ifdef DEBUG_PKTLOG
    struct log_rcfind log_data;
    memset(&log_data, 0, sizeof(log_data));
#endif

    pRc = (TX_RATE_CTRL *)(pSib ? (pSib) : NULL);

    *isProbing = FALSE;
#ifdef DEBUG_PKTLOG
    log_data.isProbing = *isProbing;
#endif

    rssiLast = median(pRc->rssiLast, pRc->rssiLastPrev, pRc->rssiLastPrev2);

    /*
     * Age (reduce) last ack rssi based on how old it is.
     * The bizarre numbers are so the delta is 160msec,
     * meaning we divide by 16.
     *   0msec   <= dt <= 25msec:   don't derate
     *   25msec  <= dt <= 185msec:  derate linearly from 0 to 10dB
     *   185msec <= dt:             derate by 10dB
     */

    nowMsec = A_MS_TICKGET();
    dt = nowMsec - pRc->rssiTime;

    if (dt >= 185) {
        rssiReduce = 10;
    } else if (dt >= 25) {
        rssiReduce = (A_UINT8)((dt - 25) >> 4);
    }

    /* Now reduce rssiLast by rssiReduce */
    if (rssiLast < rssiReduce) {
        rssiLast = 0;
    } else {
        rssiLast -= rssiReduce;
    }

    /*
     * Now look up the rate in the rssi table and return it.
     * If no rates match then we return 0 (lowest rate)
     */

    bestThruput = 0;
    maxIndex    = pRc->maxValidRate-1;

    /* FIXME: XXX */
    minIndex    = 0;
    bestRate    = minIndex;
    
    /*
     * Try the higher rate first. It will reduce memory moving time
     * if we have very good channel characteristics.
     */
    for (index = maxIndex; index >= minIndex ; index--) {
        A_UINT8 perThres;
    
        rate = pRc->validRateIndex[index];

        if (rssiLast <= pRc->state[rate].rssiThres || rate > pRc->rateMaxPhy) {
            continue;
        }

        /*
         * For TCP the average collision rate is around 11%,
         * so we ignore PERs less than this.  This is to
         * prevent the rate we are currently using (whose
         * PER might be in the 10-15 range because of TCP
         * collisions) looking worse than the next lower
         * rate whose PER has decayed close to 0.  If we
         * used to next lower rate, its PER would grow to
         * 10-15 and we would be worse off then staying
         * at the current rate.
         */
        perThres = pRc->state[rate].per;
        if ( perThres < 12 ) {
            perThres = 12;
        }

        thisThruput = pRateTable->info[rate].userRateKbps * (100 - perThres);

        if (bestThruput <= thisThruput) {
            bestThruput = thisThruput;
            bestRate    = rate;
        }
    }

    rate = bestRate;

    pRc->rssiLastLkup = rssiLast;

    /*
     * Must check the actual rate (rateKbps) to account for non-monoticity of
     * 11g's rate table
     */

    if (rate >= pRc->rateMaxPhy && probeAllowed) {
        rate = pRc->rateMaxPhy;

        /* Probe the next allowed phy state */
        /* FIXME:XXXX Check to make sure ratMax is checked properly */
        if (rcGetNextValidTxRate( pRateTable, pRc, rate, &nextRate) && 
            (nowMsec - pRc->probeTime > pRateTable->probeInterval) &&
            (pRc->hwMaxRetryPktCnt >= 1))
        {
            rate                  = nextRate;
            pRc->probeRate        = rate;
            pRc->probeTime        = nowMsec;
            pRc->hwMaxRetryPktCnt = 0;
            *isProbing            = TRUE;

#ifdef DEBUG_PKTLOG
            log_data.isProbing = *isProbing;
#endif
        }
    }

    /*
     * Make sure rate is not higher than the allowed maximum.
     * We should also enforce the min, but I suspect the min is
     * normally 1 rather than 0 because of the rate 9 vs 6 issue
     * in the old code.
     */
    if (rate > (pRc->rateTableSize - 1)) {
        rate = pRc->rateTableSize - 1;
    }

#ifdef DEBUG_PKTLOG
    log_data.rc         = pRc;
    log_data.rateCode   = pRateTable->info[rate].rateCode;
    log_data.rate       = rate;
    log_data.rssiReduce = rssiReduce;
    log_data.misc[0]    = pRc->state[rate].per;
    log_data.misc[1]    = pRc->state[rate].rssiThres;
    PKTLOG_RATE_CTL_FIND(sc, &log_data, 0);
#endif

#if 0
    /*
     * Check duration here for regulatory compliance in Japan
     * Limiting to 3.8 ms to give a little header room for measurement error
     */
    if (pDesc->pVportBss->bss.telecSupportReqd) {
        // find the rate such that the duration is below approx 3.8 ms

        minRateKbps = ((A_UINT16)pDesc->hw.txControl.frameLength * 80) / 38;

        rateKbps = pRateTable->info[rate].rateKbps;

        if (rateKbps < minRateKbps) {
            rateKbps = pRateTable->info[rate].rateKbps;
            uiPrintf("LEN %d rate %d\n", (A_UINT16)pDesc->hw.txControl.frameLength, rateKbps); 
            if (isGrp(&pHdr->address1)) {
                rate = rcRegRateFind(pDesc->pVportBss, (A_UINT16) pDesc->hw.txControl.frameLength,
                                     rate, pRcTable->regDataLenTable);           

            } else if (pRc && pRc->rateTableSize) {//Rate table structures are set properly
                while (rateKbps < minRateKbps) {
                    if (!rcGetNextValidTxRate(pRc, rate, &nextRate, pRcTable,
                                              !isChanTurbo, WLAN_PHY_TURBO))
                    {
                        ASSERT(0);  // corner case: no valid rates above minRateKbps                      
                        break;
                    }
                    rate = nextRate;
                    rateKbps = pRcTable->info[rate].rateKbps;
                }
            } 
            if (pSib) {
                pSib->lastRateKbps = pRcTable->info[rate].rateKbps;
            }
            rateKbps = pRcTable->info[rate].rateKbps;
            uiPrintf("T %d\n", rateKbps); 
        }
    }
#endif /* #if 0 */
    
    /* record selected rate, which is used to decide if we want to do fast frame */
    if (!(*isProbing) && pSib) {
        pSib->lastRateKbps = pRateTable->info[rate].rateKbps;
    }

    ASSERT(pRateTable->info[rate].valid);

    return rate;
}

static void
rcRateSetseries(const RATE_TABLE *pRateTable ,
                struct ath_rc_series *series, A_UINT8 tries, A_UINT8 rix, A_BOOL rtsctsenable)
{
    series->tries = tries;
    series->flags = (rtsctsenable? ATH_RC_RTSCTS_FLAG : 0) | 
                    (WLAN_RC_PHY_DS(pRateTable->info[rix].phy) ? ATH_RC_DS_FLAG : 0) | 
                    (WLAN_RC_PHY_40(pRateTable->info[rix].phy) ? ATH_RC_CW40_FLAG : 0) | 
                    (WLAN_RC_PHY_SGI(pRateTable->info[rix].phy) ? ATH_RC_SGI_FLAG : 0);

    series->rix = pRateTable->info[rix].baseIndex;
}

static A_UINT8 
rcRateGetIndex(struct ath_softc *sc, struct ath_node *an,        
               const RATE_TABLE *pRateTable , 
               A_UINT8 rix, A_UINT16 stepDown, A_UINT16 minRate)
{
    A_UINT32                j;
    A_UINT8                 nextIndex;
    struct atheros_node     *pSib = ATH_NODE_ATHEROS(an);
    TX_RATE_CTRL            *pRc = (TX_RATE_CTRL *)(pSib);
    
    if (minRate) {
        for (j = RATE_TABLE_SIZE; j > 0; j-- ) {
            if (rcGetNextLowerValidTxRate(pRateTable, pRc, rix, &nextIndex)) {
                rix = nextIndex;
            } else {
                break;
            }
        }
    } else {
        for (j = stepDown; j > 0; j-- ) {
            if (rcGetNextLowerValidTxRate(pRateTable, pRc, rix, &nextIndex)) {
                rix = nextIndex;
            } else {
                break;
            }
        }
    }

    return rix;
}

void
rcRateFind(struct ath_softc *sc, struct ath_node *an, 
           int numTries, int numRates, int stepDnInc,
           unsigned int rcflag, struct ath_rc_series series[], int *isProbe)
{
    A_UINT8               i=0; 
    A_UINT8               tryPerRate  = 0;
    struct atheros_softc  *asc        = (struct atheros_softc*)sc->sc_rc;
    RATE_TABLE            *pRateTable = (RATE_TABLE *)asc->hwRateTable[sc->sc_curmode];
    struct atheros_node   *asn        = ATH_NODE_ATHEROS(an);
    A_UINT8               rix, nrix;

    rix = rcRateFind_ht(sc, asn, pRateTable, (rcflag & ATH_RC_PROBE_ALLOWED) ? 1 : 0, 
                        isProbe);
    nrix = rix;

    if ((rcflag & ATH_RC_PROBE_ALLOWED) && (*isProbe)) {
        /* set one try for probe rates. For the probes don't enable rts */
        rcRateSetseries(pRateTable, &series[i++], 1, nrix, FALSE);
          
        tryPerRate = (numTries/numRates);
        /*
         * Get the next tried/allowed rate. No RTS for the next series
         * after the probe rate
         */
        nrix = rcRateGetIndex( sc, an, pRateTable, nrix, 1, FALSE);
        rcRateSetseries(pRateTable, &series[i++], tryPerRate, nrix, 0);
    } else {
        tryPerRate = (numTries/numRates);
        /* Set the choosen rate. No RTS for first series entry. */
        rcRateSetseries(pRateTable, &series[i++], tryPerRate, nrix, FALSE);
     }
            /* Fill in the other rates for multirate retry */
    for (; i < numRates; i++) {
        A_UINT8 tryNum;
        A_UINT8 minRate;

        tryNum  = ((i + 1) == numRates) ? numTries - (tryPerRate * i) : tryPerRate ;
        minRate = (((i + 1) == numRates) && (rcflag & ATH_RC_MINRATE_LASTRATE)) ? 1 : 0;
                
        nrix = rcRateGetIndex(sc, an, pRateTable, nrix, stepDnInc, minRate);
        /* All series has RTS enabled */
        rcRateSetseries(pRateTable, &series[i], tryNum, nrix, FALSE);
    }
}

static void
rcUpdate_ht(struct ath_softc *sc, struct ath_node *an, int txRate, 
            A_BOOL Xretries, int retries, A_RSSI rssiAck, A_UINT8 curTxAnt, 
            A_UINT16 nFrames, A_UINT16 nBad)
{
    TX_RATE_CTRL *pRc;
    A_UINT32     nowMsec     = A_MS_TICKGET();
    A_BOOL       stateChange = FALSE;
    A_UINT8      lastPer;
    int          rate,count;
    //struct ieee80211com   *ic         = &sc->sc_ic;
    struct atheros_node   *pSib       = ATH_NODE_ATHEROS(an);
    struct atheros_softc  *asc        = (struct atheros_softc*)sc->sc_rc;
    RATE_TABLE            *pRateTable = (RATE_TABLE *)asc->hwRateTable[sc->sc_curmode];
#ifdef DEBUG_PKTLOG
    struct log_rcupdate   log_data;
#endif

    static A_UINT32 nRetry2PerLookup[10] = {
        100 * 0 / 1,
        100 * 1 / 4,
        100 * 1 / 2,
        100 * 3 / 4,
        100 * 4 / 5,
        100 * 5 / 6,
        100 * 6 / 7,
        100 * 7 / 8,
        100 * 8 / 9,
        100 * 9 / 10
    };

#ifdef DEBUG_PKTLOG
    memset(&log_data, 0, sizeof(log_data));
#endif

    if (!pSib)
        return;

    pRc = (TX_RATE_CTRL *)(pSib);

    ASSERT(retries >= 0 && retries < MAX_TX_RETRIES);
    ASSERT(txRate >= 0);
    
    if (txRate < 0) {
        //printk("%s: txRate value of 0x%x is bad.\n", __FUNCTION__, txRate);
        return;
    }

    /* To compensate for some imbalance between ctrl and ext. channel */

    if (WLAN_RC_PHY_40(pRateTable->info[txRate].phy))
        rssiAck = rssiAck < 3? 0: rssiAck - 3; 

    lastPer = pRc->state[txRate].per;

    if (Xretries) {
        /* Update the PER. */
        if (Xretries == 1) {
            pRc->state[txRate].per += 30 ; 
            if (pRc->state[txRate].per > 100) {
                pRc->state[txRate].per = 100;
            }
#if 0
            /* Update the RSSI threshold */
            /*
            * Oops, it didn't work at all.  Set the required rssi
            * to the rssiAck we used to lookup the rate, plus 4dB.
            * The immediate effect is we won't try this rate again
            * until we get an rssi at least 4dB higher.
            */
            if (txRate > 0) {
                A_RSSI rssi = A_MAX(pRc->rssiLastLkup, pRc->rssiLast - 2);

                if (pRc->state[txRate].rssiThres + 2 < rssi) {
                    pRc->state[txRate].rssiThres += 4;
                } else if (pRc->state[txRate].rssiThres < rssi + 2) {
                    pRc->state[txRate].rssiThres = rssi + 2;
                } else {
                    pRc->state[txRate].rssiThres += 2;
                }

                pRc->hwMaxRetryRate = (A_UINT8)txRate;
                stateChange         = TRUE;
            }

            /*
            * Also, if we are not doing a probe, we force a significant
            * backoff by reducing rssiLast.  This doesn't have a big
            * impact since we will bump the rate back up as soon as we
            * get any good ACK.  RateMax will also be set to the current
            * txRate if the failed frame is not a probe.
            */
            if (pRc->probeRate == 0 || pRc->probeRate != txRate) {
                pRc->rssiLast      = 10 * pRc->state[txRate].rssiThres / 16;
                pRc->rssiLastPrev  = pRc->rssiLast;
                pRc->rssiLastPrev2 = pRc->rssiLast;
            }
#endif /* #if 0 */

        } else {
            /* Xretries == 2 */
#ifdef MULTI_RATE_RETRY_ENABLE
            count = sizeof(nRetry2PerLookup) / sizeof(nRetry2PerLookup[0]);
            if (retries >= count) {
                retries = count - 1;
            }

            /* new_PER = 7/8*old_PER + 1/8*(currentPER) */
            pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per - 
                (pRc->state[txRate].per / 8) + ((100) / 8));
#endif
        }

        /* Xretries == 1 or 2 */

        for (rate = txRate + 1; rate < pRc->rateTableSize; rate++) {
            if (pRc->state[rate].per < pRc->state[txRate].per) {
                pRc->state[rate].per = pRc->state[txRate].per;
            }
        }

        if (pRc->probeRate == txRate)
            pRc->probeRate = 0;

    } else {
        /* Xretries == 0 */

        /*
         * Update the PER.  Make sure it doesn't index out of array's bounds.
         */
        count = sizeof(nRetry2PerLookup) / sizeof(nRetry2PerLookup[0]);
        if (retries >= count) {
            retries = count - 1;
        }

        if (nBad) {
            /* new_PER = 7/8*old_PER + 1/8*(currentPER)  */
            /*
             * Assuming that nFrames is not 0.  The current PER
             * from the retries is 100 * retries / (retries+1),
             * since the first retries attempts failed, and the
             * next one worked.  For the one that worked, nBad
             * subframes out of nFrames wored, so the PER for
             * that part is 100 * nBad / nFrames, and it contributes
             * 100 * nBad / (nFrames * (retries+1)) to the above
             * PER.  The expression below is a simplified version
             * of the sum of these two terms.
             */
        if (nFrames > 0)
            pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per - 
                (pRc->state[txRate].per / 8) + 
                ((100*(retries*nFrames + nBad)/(nFrames*(retries+1))) / 8));
        } else {
            /* new_PER = 7/8*old_PER + 1/8*(currentPER) */

            pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per - 
                (pRc->state[txRate].per / 8) + (nRetry2PerLookup[retries] / 8));
        }

        pRc->rssiLastPrev2 = pRc->rssiLastPrev;
        pRc->rssiLastPrev  = pRc->rssiLast;
        pRc->rssiLast      = rssiAck;
        pRc->rssiTime      = nowMsec;

        /*
         * If we got at most one retry then increase the max rate if
         * this was a probe.  Otherwise, ignore the probe.
         */

        if (pRc->probeRate && pRc->probeRate == txRate) {
            if (retries > 0 || 2 * nBad > nFrames) {
                /*
                 * Since we probed with just a single attempt,
                 * any retries means the probe failed.  Also,
                 * if the attempt worked, but more than half
                 * the subframes were bad then also consider
                 * the probe a failure.
                 */
                pRc->probeRate = 0;
            } else {
                pRc->rateMaxPhy = pRc->probeRate;

                if (pRc->state[pRc->probeRate].per > 30) {
                    pRc->state[pRc->probeRate].per = 20;
                }

                pRc->probeRate = 0;

                /*
                 * Since this probe succeeded, we allow the next probe
                 * twice as soon.  This allows the maxRate to move up
                 * faster if the probes are succesful.
                 */
                pRc->probeTime = nowMsec - pRateTable->probeInterval / 2;
            }
        }

        if (retries > 0) {
            /*
             * Don't update anything.  We don't know if this was because
             * of collisions or poor signal.
             *
             * Later: if rssiAck is close to pRc->state[txRate].rssiThres
             * and we see lots of retries, then we could increase
             * pRc->state[txRate].rssiThres.
             */
             pRc->hwMaxRetryPktCnt = 0;
        } else {
            /*
             * It worked with no retries.  First ignore bogus (small)
             * rssiAck values.
             */
            if (txRate == pRc->rateMaxPhy && pRc->hwMaxRetryPktCnt < 255) {
                pRc->hwMaxRetryPktCnt++;
            }

            if (rssiAck >= pRateTable->info[txRate].rssiAckValidMin) {
                /* Average the rssi */
                if (txRate != pRc->rssiSumRate) {
                    pRc->rssiSumRate = txRate;
                    pRc->rssiSum     = pRc->rssiSumCnt = 0;
                }

                pRc->rssiSum += rssiAck;
                pRc->rssiSumCnt++;

                if (pRc->rssiSumCnt > 4) {
                    A_RSSI32 rssiAckAvg = (pRc->rssiSum + 2) / 4;

                    pRc->rssiSum = pRc->rssiSumCnt = 0;
                    
                    /* Now reduce the current rssi threshold. */
                    if ((rssiAckAvg < pRc->state[txRate].rssiThres + 2) &&
                        (pRc->state[txRate].rssiThres > 
                         pRateTable->info[txRate].rssiAckValidMin))
                    {
                        pRc->state[txRate].rssiThres--;
                    }

                    stateChange = TRUE;
                }
            }
        }
    }

    /* For all cases */

    ASSERT((pRc->rateMaxPhy >= 0 && pRc->rateMaxPhy <= pRc->rateTableSize && 
            pRc->rateMaxPhy != INVALID_RATE_MAX));
    
    /*
     * If this rate looks bad (high PER) then stop using it for
     * a while (except if we are probing).
     */
    if (pRc->state[txRate].per >= 55 && txRate > 0 &&
        pRateTable->info[txRate].rateKbps <= 
            pRateTable->info[pRc->rateMaxPhy].rateKbps)
    {
        rcGetNextLowerValidTxRate(pRateTable, pRc, (A_UINT8) txRate, 
                                  &pRc->rateMaxPhy);

        /* Don't probe for a little while. */
        pRc->probeTime = nowMsec;
    }

    if (stateChange) {
        /*
         * Make sure the rates above this have higher rssi thresholds.
         * (Note:  Monotonicity is kept within the OFDM rates and within the CCK rates.
         *         However, no adjustment is made to keep the rssi thresholds monotonically
         *         increasing between the CCK and OFDM rates.)
         */
        for (rate = txRate; rate < pRc->rateTableSize - 1; rate++) {
            if (pRateTable->info[rate+1].phy != pRateTable->info[txRate].phy) {
                break;
            }

            if (pRc->state[rate].rssiThres + pRateTable->info[rate].rssiAckDeltaMin >
                pRc->state[rate+1].rssiThres)
            {
                pRc->state[rate+1].rssiThres =
                    pRc->state[rate].rssiThres + pRateTable->info[rate].rssiAckDeltaMin;
            }
        }

        /* Make sure the rates below this have lower rssi thresholds. */
        for (rate = txRate - 1; rate >= 0; rate--) {
            if (pRateTable->info[rate].phy != pRateTable->info[txRate].phy) {
                break;
            }

            if (pRc->state[rate].rssiThres + pRateTable->info[rate].rssiAckDeltaMin >
                pRc->state[rate+1].rssiThres)
            {
                if (pRc->state[rate+1].rssiThres < pRateTable->info[rate].rssiAckDeltaMin) {
                    pRc->state[rate].rssiThres = 0;
                } else {
                    pRc->state[rate].rssiThres =
                        pRc->state[rate+1].rssiThres - pRateTable->info[rate].rssiAckDeltaMin;
                }

                if (pRc->state[rate].rssiThres < pRateTable->info[rate].rssiAckValidMin) {
                    pRc->state[rate].rssiThres = pRateTable->info[rate].rssiAckValidMin;
                }
            }
        }
    }

    /* Make sure the rates below this have lower PER */
    /* Monotonicity is kept only for rates below the current rate. */
    if (pRc->state[txRate].per < lastPer) {
        for (rate = txRate - 1; rate >= 0; rate--) {
            if (pRateTable->info[rate].phy != pRateTable->info[txRate].phy) {
                break;
            }

            if (pRc->state[rate].per > pRc->state[rate+1].per) {
                pRc->state[rate].per = pRc->state[rate+1].per;
            }
        }
    }

    /* Every so often, we reduce the thresholds and PER (different for CCK and OFDM). */
    if (nowMsec - pRc->rssiDownTime >= pRateTable->rssiReduceInterval) {
        for (rate = 0; rate < pRc->rateTableSize; rate++) {
            if (pRc->state[rate].rssiThres > pRateTable->info[rate].rssiAckValidMin) {
                pRc->state[rate].rssiThres -= 1;
            }
//            pRc->state[rate].per = 7*pRc->state[rate].per/8;
        }

        pRc->rssiDownTime = nowMsec;
    }

    /* Every so often, we reduce the thresholds and PER (different for CCK and OFDM). */
    if (nowMsec - pRc->perDownTime >= pRateTable->rssiReduceInterval) {
        for (rate = 0; rate < pRc->rateTableSize; rate++) {
            pRc->state[rate].per = 7*pRc->state[rate].per/8;
        }

        pRc->perDownTime = nowMsec;
    }


#ifdef DEBUG_PKTLOG
    {
        struct log_rcupdate log_data;

        log_data.rc                = pRc;
        log_data.txRate            = txRate;
        log_data.rateCode          = pRateTable->info[txRate].rateCode;
        log_data.Xretries          = Xretries;
        log_data.currentBoostState = 0;
        log_data.useTurboPrime     = 0;
        log_data.retries           = retries;
        log_data.rssiAck           = rssiAck;
        PKTLOG_RATE_CTL_UPDATE(sc, &log_data, 0);
   }
#endif
}

/*
 * This routine is called by the Tx interrupt service routine to give
 * the status of previous frames.
 */
void
rcUpdate(struct ath_softc *sc, struct ath_node *an, A_RSSI rssiAck, A_UINT8 curTxAnt, 
         int finalTSIdx, int Xretries, struct ath_rc_series rcs[], int nFrames, 
         int nBad, int long_retry)
{
    A_UINT32              series      = 0;
    A_UINT32              rix;
    struct atheros_softc  *asc        = (struct atheros_softc*)sc->sc_rc;
    RATE_TABLE            *pRateTable = (RATE_TABLE *)asc->hwRateTable[sc->sc_curmode];
    struct atheros_node   *pSib       = ATH_NODE_ATHEROS(an);
    TX_RATE_CTRL          *pRc        = (TX_RATE_CTRL *)(pSib);
    A_UINT8               flags;


    if (!an) {
        panic("rcUpdate an is NULL");
        return;
    }

    if (rcs[series].tries &&  series != finalTSIdx) {

        /* Process intermediate rates that failed.*/
        for (series = 0; series < finalTSIdx ; series++) {
            if (rcs[series].tries != 0) {
                flags = rcs[series].flags;
                /* If HT40 and we have switched mode from 40 to 20 => don't update */
                if ((flags & ATH_RC_CW40_FLAG) && 
                    (pRc->rcPhyMode != (flags & ATH_RC_CW40_FLAG)))
                {
                    return;
                }

                if ((flags & ATH_RC_CW40_FLAG) && (flags & ATH_RC_SGI_FLAG)) {
                    rix = pRateTable->info[rcs[series].rix].htIndex;
                } else if (flags & ATH_RC_SGI_FLAG) {
                    rix = pRateTable->info[rcs[series].rix].sgiIndex;
                } else if (flags & ATH_RC_CW40_FLAG) {
                    rix = pRateTable->info[rcs[series].rix].cw40Index;
                } else {
                    rix = pRateTable->info[rcs[series].rix].baseIndex;
                }

                /* FIXME:XXXX, too many args! */
                rcUpdate_ht(sc, an, rix, Xretries? 1 : 2, rcs[series].tries, 
                            rssiAck, curTxAnt, nFrames, nFrames);
            }
        }
    }

    flags = rcs[series].flags;
    /* If HT40 and we have switched mode from 40 to 20 => don't update */
    if ((flags & ATH_RC_CW40_FLAG) && 
        (pRc->rcPhyMode != (flags & ATH_RC_CW40_FLAG)))
    {
        return;
    }

    if ((flags & ATH_RC_CW40_FLAG) && (flags & ATH_RC_SGI_FLAG)) {
        rix = pRateTable->info[rcs[series].rix].htIndex;
    } else if (flags & ATH_RC_SGI_FLAG) {
        rix = pRateTable->info[rcs[series].rix].sgiIndex;
    } else if (flags & ATH_RC_CW40_FLAG) {
        rix = pRateTable->info[rcs[series].rix].cw40Index;
    } else {
        rix = pRateTable->info[rcs[series].rix].baseIndex;
    }

    /* FIXME:XXXX, too many args! */
    rcUpdate_ht(sc, an, rix, Xretries, long_retry, rssiAck, curTxAnt, 
                nFrames, nBad);
}

#if 0
static A_UINT8
rcRateValueToIndex_ht(struct ath_softc *sc, A_UINT32 txRateKbps, 
                      struct ath_node *an, A_BOOL turboFlag)
{
    A_UINT8    rate  = 0;
    A_UINT8    index = 0;
    RATE_TABLE *pRcTable;

    if (NULL == pSib) {
        return index;
    }

    pRcTable = (RATE_TABLE *)pdevInfo->pRcInfo->rcTable[pSib->pVportBss->bss.wMode];
    while (rate < pRcTable->rateCount) {
        if (pRcTable->info[rate].valid && txRateKbps == pRcTable->info[rate].rateKbps) {
            index = rate;
            break;
        }
        rate++;
    }

    return index;
}

static A_UINT8
rcGetSigQuality_ht(struct ath_softc *sc, A_UINT8 txRate, struct ath_node *an)
{
    const RATE_TABLE    *pRcTable;
    A_UINT32            rateKbps;
    A_UINT32            maxRateKbps;
    A_UINT8             qual = 0;
    A_UINT8             per;
    TX_RATE_CTRL        *pRc;

    if (NULL == pSib) {
        return qual;
    }

    /* FIXME : XXX */
    pRc = (TX_RATE_CTRL *)(an + 1);

    pRcTable    = (RC_TABLE_HT *)pdevInfo->pRcInfo->rcTable[pSib->pVportBss->bss.wMode];
    maxRateKbps = pRcTable->info[pRcTable->rateCount - 1].rateKbps;
    rateKbps    = pRcTable->info[txRate].rateKbps;
    per         = pRc->state[txRate].per;

    qual = (A_UINT8)((rateKbps * (100 - per)) / maxRateKbps);

    return qual;
}

/*
 * rcGetBestCckRate - given a current tx rate index and tx descriptor,
 * find the previous CCK rate from the working rate table.  Only
 * meaningful in 11g mode.  Returns the best CCK rate if found
 * else original rate.
 */
static A_UINT16
rcGetBestCckRate_ht(struct ath_softc *sc, struct ath_node *an, 
                    A_UINT16 curRateIndex)
{
    const RC_TABLE_HT   *pRcTable;
    TX_RATE_CTRL        *pRc;
    A_UINT8             prevIndex;
    SIB_ENTRY           *pSib = pDesc->pDestSibEntry;

    if (!pSib) {
        return curRateIndex;
    }

    pRcTable  = (RC_TABLE_HT *)pdevInfo->pRcInfo->rcTable[pDesc->pVportBss->bss.wMode];
    pRc       = (TX_RATE_CTRL *)(pSib);
    prevIndex = (A_UINT8)curRateIndex;
    
    do {
        if (pRcTable->info[prevIndex].phy == WLAN_RC_PHY_CCK) {
            return (A_UINT16)prevIndex;
        }
    } while (rcGetNextLowerValidTxRate(pRcTable, pRc, prevIndex, &prevIndex));

    return curRateIndex;
}

/*******************************************************************************
 * roundUpRate
 *
 * Given a rate (in kbps), returns the rate (in kbps) that corresponds the the
 * next highest rate.  This is useful for displaying transmit rate from a LPFed
 * value, so that the user is not confused by the LPFed value.
 */
static A_UINT32
rcRoundUpRate_ht(struct ath_softc *sc, struct ath_node *an, 
                        A_UINT32 rate, A_BOOL validChk)
{
    A_UINT8     i;
    A_UINT32    thisRate = 0;
    RC_TABLE_HT *pRcTable;

    if (pDev->staConfig.rateCtrlEnable == FALSE) {
        return rate;
    }

    /* 
     * Check the rates based on the rate table from the virtual bss instead 
     * of the local station's channel flags. 
     */
   
    /* FIXME: XXX */ 
    pRcTable = (RATE_TABLE *)pDev->pRcInfo->rcTable[pVportBss->bss.wMode];

    /*
     * Since the rates in the Rate Table may not be monotonically increasing,
     * the entire table must be traversed to find the closest matching rate.
     */
    if (rate > pRcTable->info[pRcTable->rateCount-1].rateKbps) {
        /* This case should never happen but needed for defensive programming */
        thisRate = pRcTable->info[pRcTable->rateCount-1].rateKbps;
    } else {
        for (i = 0; i < pRcTable->rateCount; i++) {
            if (!validChk || pRcTable->info[i].valid || !WLAN_RC_PHY_LEGACY(pRcTable->info[i].phy)) {
                if (i == 0 && rate <= pRcTable->info[i].rateKbps) {
                    thisRate = pRcTable->info[i].rateKbps;
                    break;
                }

                if (pRcTable->info[i].rateKbps >= rate) {
                    /* Must account for possible non-monotinicity in  rate table. */
                    if (thisRate <= 0 || pRcTable->info[i].rateKbps < thisRate) {
                        thisRate = pRcTable->info[i].rateKbps;
                    }
                }
            }
        }
    }

    return thisRate;
}

#endif /* #if 0 */




