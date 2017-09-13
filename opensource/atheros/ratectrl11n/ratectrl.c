/*
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/ratectrl11n/ratectrl.c#9 $
 *
 * Copyright (c) 2001-2003 Atheros Communications, Inc., All Rights Reserved
 *
 * DESCRIPTION
 *
 * This file contains the data structures and routines for transmit rate
 * control.
 *
 */

#ifdef __FreeBSD__
#include <dev/ath/ath_rate/atheros/ratectrl.h>
#else
#include "ratectrl.h"
#endif

#ifdef DEBUG_PKTLOG
#include "pktlog_rc.h"
#endif

#ifdef DEBUG_PKTLOG
extern struct ath_pktlog_rcfuncs *g_pktlog_rcfuncs;
#endif

/* 
 * Table indicates the preferred mode when rates of two phy modes 
 * overlap. First entry in the structures in the table indicate the 
 * preferred mode and the second entry indicates the mode overwhich 
 * this mode is preferred. 
 */
static RATE_OVL_POLICY rateOvlTable[] = {
    { WLAN_RC_PHY_OFDM,         WLAN_RC_PHY_CCK             }, 
    { WLAN_RC_PHY_HT_20_SS,     WLAN_RC_PHY_CCK             }, 
    { WLAN_RC_PHY_HT_40_SS,     WLAN_RC_PHY_CCK             }, 
    { WLAN_RC_PHY_HT_20_SS,     WLAN_RC_PHY_OFDM            }, 
    { WLAN_RC_PHY_HT_40_SS,     WLAN_RC_PHY_OFDM            }, 
    { WLAN_RC_PHY_HT_20_SS,     WLAN_RC_PHY_HT_20_DS        }, 
    { WLAN_RC_PHY_HT_40_SS,     WLAN_RC_PHY_HT_40_DS        }, 
    { WLAN_RC_PHY_HT_20_SS,     WLAN_RC_PHY_HT_20_SS_HGI    }, 
    { WLAN_RC_PHY_HT_20_DS,     WLAN_RC_PHY_HT_20_DS_HGI    }, 
    { WLAN_RC_PHY_HT_40_SS,     WLAN_RC_PHY_HT_40_SS_HGI    },
    { WLAN_RC_PHY_HT_40_DS,     WLAN_RC_PHY_HT_40_DS_HGI    },
};  

/* Access functions for validTxRateMask */

static void
rcInitValidTxMask(struct TxRateCtrl_s *pRc)
{
    A_UINT8     i;

    for (i = 0; i < pRc->rateTableSize; i++) {
        pRc->rcIndexvalid[i] = FALSE;
    }
}

static void
rcSetValidTxMask(struct TxRateCtrl_s *pRc, A_UINT8 index, A_BOOL validTxRate)
{
    ASSERT(index < pRc->rateTableSize);

    if (validTxRate) {
        pRc->rcIndexvalid[index] = TRUE;
    } else {
        pRc->rcIndexvalid[index] = FALSE;
    }
}

static INLINE A_BOOL
rcIsValidTxMask(struct TxRateCtrl_s *pRc, A_UINT8 index)
{
    ASSERT(index < pRc->rateTableSize);
    return (pRc->rcIndexvalid[index]);
}


static INLINE A_BOOL
rcGetNextValidTxRate(const RATE_TABLE *pRateTable, struct TxRateCtrl_s *pRc, 
                     A_UINT8 curValidTxRate, A_UINT8 *pNextIndex)
{
    A_UINT8     i;

    for (i = curValidTxRate + 1; i < pRateTable->rateCount; i++) {
        if (pRc->rcIndexvalid[i]) {
            *pNextIndex = i;
            return TRUE;
        }
    }
    /* No more valid rates */
    *pNextIndex = 0;
    return FALSE;
}


static INLINE A_BOOL
rcGetNextLowerValidTxRate(const RATE_TABLE *pRateTable, struct TxRateCtrl_s *pRc,  
                          A_UINT8 curValidTxRate, A_UINT8 *pNextIndex)
{
    A_INT8     i;

    *pNextIndex = 0;
    for (i = (A_INT8)curValidTxRate - 1; i >= 0 ; i--) {
        if (pRc->rcIndexvalid[i]) {
            *pNextIndex = i;
            return TRUE;
        }
    }
    *pNextIndex = 0;
    return FALSE;
}

static A_INT8
rcGetBestRateIndex (const RATE_TABLE *pRateTable, struct TxRateCtrl_s *pRc, 
                    A_UINT8 rcPhyState, A_UINT8 maxIndex, A_UINT8 minIndex, A_RSSI  rssiLast)
{   
    A_INT8              index;
    A_UINT32            bestThruput,thisThruput;
    A_INT8              rate, bestRateIndex = -1;

    bestThruput = 0;
    /*
     * Try the higher rate first. It will reduce memory moving time
     * if we have very good channel characteristics.
     */
    for (index = maxIndex; index >= minIndex ; index--) {
        rate = pRc->validPhyRateIndex[rcPhyState][index];

        if (rssiLast < pRc->state[rate].rssiThres) {
            continue;
        }

        thisThruput = pRateTable->info[rate].userRateKbps * 
                      (100 - pRc->state[rate].per);

        if (bestThruput <= thisThruput) {
            bestThruput = thisThruput;
            bestRateIndex = index;
        }
    }
    return bestRateIndex;
}

static void
rcRemoveOvlRates(const RATE_TABLE *pRateTable, struct TxRateCtrl_s *pRc, A_UINT8 phy, A_UINT8 ovlPhy)
{
    A_INT8  i, j;

    if (!pRc->validPhyRateCount[ovlPhy] || !pRc->validPhyRateCount[phy]) {
        return;
    }
    for (i = pRc->validPhyRateCount[ovlPhy] - 1; i >= 0; i--) {

        A_UINT8 rateIndex = pRc->validPhyRateIndex[ovlPhy][i];

        if ((pRateTable->info[rateIndex].rateKbps 
             < pRateTable->info[pRc->validPhyRateIndex[phy][0]].rateKbps) || 
            (pRateTable->info[rateIndex].rateKbps  
             > pRateTable->info[pRc->validPhyRateIndex[phy][pRc->validPhyRateCount[phy]-1]].rateKbps)) 
        {
            continue;
        }
        else {
            rcSetValidTxMask(pRc, rateIndex, FALSE);
            for (j = 1; i+j <= pRc->validPhyRateCount[ovlPhy]; j++) {
                pRc->validPhyRateIndex[ovlPhy][i+j-1] = pRc->validPhyRateIndex[ovlPhy][i+j];
            }
            pRc->validPhyRateCount[ovlPhy] -= 1;
        }
    }
}

static A_BOOL
rcIsValidPhyRate(A_UINT32 phy, A_UINT32 capflag, A_BOOL ignoreCw)
{
    if (WLAN_RC_PHY_HT(phy) & !(capflag & WLAN_RC_HT_FLAG))
        return FALSE;
    if (WLAN_RC_PHY_DS(phy) && !(capflag & WLAN_RC_DS_FLAG)) 
        return FALSE;
    if (WLAN_RC_PHY_SGI(phy) && !(capflag & WLAN_RC_SGI_FLAG))
        return FALSE;
    if (!ignoreCw && WLAN_RC_PHY_HT(phy)) {
        if (WLAN_RC_PHY_40(phy) && !(capflag & WLAN_RC_40_FLAG))
            return FALSE;
        if (!WLAN_RC_PHY_40(phy) && (capflag & WLAN_RC_40_FLAG))
            return FALSE;
    }
    return TRUE;
}

/* 
 * Initialize the Valid Rate Index from valid entries in Rate Table 
 */
static A_UINT8
rcSibInitValidRates(const RATE_TABLE *pRateTable, struct atheros_node *pSib, A_UINT32 capflag)
{
    struct TxRateCtrl_s     *pRc = &pSib->txRateCtrl;
    A_UINT8                 i, hi = 0;
    
    for (i = 0; i < pRateTable->rateCount; i++) {
        if (pRateTable->info[i].valid == TRUE)
        {
            A_UINT32 phy = pRateTable->info[i].phy;
            pSib->rixMap[i] = 0;
            pSib->htrixMap[i] = 0;
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
rcSibSetValidRates(const RATE_TABLE *pRateTable, struct atheros_node *pSib, 
                        struct ieee80211_rateset *pRateSet, A_UINT32 capflag, A_BOOL htRate)
{
    struct TxRateCtrl_s     *pRc = &pSib->txRateCtrl;
    A_UINT8                 i, j, hi = 0;

    /* Use intersection of working rates and valid rates */
    for (i = 0; i < pRateSet->rs_nrates; i++) {
        for (j = 0; j < pRateTable->rateCount; j++) {
            A_UINT32 phy = pRateTable->info[j].phy;

            if (((pRateSet->rs_rates[i] & 0x7F) == (pRateTable->info[j].dot11Rate & 0x7F))
                && (pRateTable->info[j].valid == TRUE) && (!(htRate ^ WLAN_RC_PHY_HT(phy))))
            {
                if (!rcIsValidPhyRate(phy, capflag, FALSE)) 
                    continue;
                if (WLAN_RC_PHY_HT(phy)) {
                    pSib->htrixMap[j] = i;
                } else {
                    pSib->rixMap[j] = i;
                }
                pRc->validPhyRateIndex[phy][pRc->validPhyRateCount[phy]] = j;
                pRc->validPhyRateCount[phy] += 1;
                rcSetValidTxMask(pRc, j, TRUE);
                hi = A_MAX(hi, j);
            }
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
void
rcSibUpdate(struct ath_softc *sc, struct ath_node *an, A_UINT32 capflag)
{
    struct atheros_node         *pSib = ATH_NODE_ATHEROS(an);
    struct atheros_softc        *asc = (struct atheros_softc *) sc->sc_rc;
    const RATE_TABLE            *pRateTable;
    struct ieee80211_rateset    *pRateSet = &an->an_node.ni_rates;
    struct ieee80211_rateset    *phtRateSet = &an->an_node.ni_htrates;
    struct TxRateCtrl_s         *pRc = &pSib->txRateCtrl;
    A_UINT8                     i, j, hi = 0, htHi = 0;

    pRateTable = asc->hwRateTable[sc->sc_curmode];
    /* Initial rate table size. Will change depending on the working rate set */
    pRc->rateTableSize = MAX_TX_RATE_TBL;

    /* Initialize thresholds according to the global rate table */
    for (i = 0 ; (i < pRc->rateTableSize); i++) {
        pRc->state[i].rssiThres = pRateTable->info[i].rssiAckValidMin;
        pRc->state[i].per       = 0;
    }

    /* Determine the valid rates */
    rcInitValidTxMask(pRc);
    for (i = 0; i < WLAN_RC_PHY_MAX; i++) {
        for (j = 0; j < MAX_TX_RATE_TBL; j++) {
            pRc->validPhyRateIndex[i][j] = 0;
        }   
        pRc->validPhyRateCount[i] = 0;
    }
    pRc->rcPhyState = 0;

    if (!pRateSet->rs_nrates) {
        /* No working rate, just initialize valid rates */
        hi = rcSibInitValidRates(pRateTable, pSib, capflag);
    } else {
        /* Use intersection of working rates and valid rates */
        hi = rcSibSetValidRates(pRateTable, pSib, pRateSet, capflag, FALSE);
        if (capflag & WLAN_RC_HT_FLAG) {
            htHi = rcSibSetValidRates(pRateTable, pSib, phtRateSet, capflag, TRUE);
        }
        hi = A_MAX(hi, htHi);
    }
    pRc->rateTableSize = hi + 1;
    pRc->rateMaxPhy  = 0;
    pRc->rateMaxIndex = 0;
    ASSERT(pRc->rateTableSize <= MAX_TX_RATE_TBL);
    
    for (i = 0; i < (sizeof(rateOvlTable)/sizeof(RATE_OVL_POLICY)); i++) {
        rcRemoveOvlRates(pRateTable, pRc, rateOvlTable[i].prefMode, rateOvlTable[i].ovlMode);
    }

    for (i = 0; i < WLAN_RC_PHY_MAX; i++) {
        if (pRc->validPhyRateCount[i]) {
            if (!rcIsValidPhyRate(i, pRateTable->initialRateMax, TRUE)) 
                continue;
            pRc->rateMaxPhy = i;
            pRc->rateMaxIndex = pRc->validPhyRateCount[i] - 1;
        }   
    }
    pRc->rcPhyState = pRc->rateMaxPhy;
}

/*
 *  This routine is called to initialize the rate control parameters
 *  in the SIB. It is called initially during system initialization
 *  or when a station is associated with the AP.
 */
void
rcSibInit(struct ath_softc *sc, struct ath_node *an)
{
    struct atheros_node *pSib       = ATH_NODE_ATHEROS(an);
    struct TxRateCtrl_s *pRc        = &pSib->txRateCtrl;

#if 0
    /* NB: caller assumed to zero state */
    A_MEM_ZERO((char *)pSib, sizeof(*pSib));
#endif
    pRc->rssiDownTime = A_MS_TICKGET();
}


A_BOOL
rcGetNextIndex(struct ath_softc *sc, struct atheros_node *pSib, 
          const RATE_TABLE *pRateTable,  A_UINT8 rateIdx, A_UINT8 *pNextIndex)
{
    return (rcGetNextLowerValidTxRate(pRateTable, &pSib->txRateCtrl, 
                                      rateIdx, pNextIndex));
}

/*
 * Return the median of three numbers
 */
static INLINE A_RSSI
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

/*
 * Determines and returns the new Tx rate index.
 */
A_UINT16
rcRateFind(struct ath_softc *sc, struct atheros_node *pSib, A_UINT32 frameLen,
           const RATE_TABLE *pRateTable, A_BOOL probeAllowed, A_BOOL *isProbing)
{
#ifdef notyet
    WLAN_STA_CONFIG      *pConfig     = &pdevInfo->staConfig;
    VPORT_BSS            *pVportXrBss = GET_XR_BSS(pdevInfo);
    WLAN_DATA_MAC_HEADER *pHdr        = pDesc->pBufferVirtPtr.header;
#endif
    struct TxRateCtrl_s  *pRc;
    A_UINT32             dt;
    A_UINT32             nowMsec;
    A_UINT8              rate;
    A_RSSI               rssiLast, rssiReduce;
#if TURBO_PRIME
    A_UINT8              primeInUse        = sc->sc_dturbo;
#else
    A_UINT8              primeInUse        = 0;
    A_UINT8              currentPrimeState = 0;
#endif
    A_UINT8              maxIndex, minIndex;
    A_INT8               index, i, prevIndex, nextIndex;

    *isProbing = FALSE;
    /* have the real rate control logic kick in */
    pRc = &pSib->txRateCtrl;

    rssiLast   = median(pRc->rssiLast, pRc->rssiLastPrev, pRc->rssiLastPrev2);
    rssiLast   = 50;
    rssiReduce = 0;

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
    maxIndex = pRc->validPhyRateCount[pRc->rcPhyState] - 1;
    minIndex = 0;
    index = rcGetBestRateIndex (pRateTable, pRc, pRc->rcPhyState, maxIndex, minIndex, rssiLast);
    rate = pRc->validPhyRateIndex[pRc->rcPhyState][(index < 0) ? minIndex : index];
   
#if 0 
    /* Following are recorded as a part of pktLog feature */
    pRc->misc[4]  = primeInUse;
    pRc->misc[5]  = 0;
    pRc->misc[9]  = pRc->rateTableSize;
    pRc->misc[8]  = rcGetNextValidTxRate(pRateTable, pRc, rate, &pRc->misc[7]);
    pRc->misc[10] = rate;
#endif

    pRc->rssiLastLkup = rssiLast;
    if ((index == maxIndex) && (pRc->rcPhyState < pRc->rateMaxPhy)) {

        /* Move to the next Phy if we hit the maxIndex */
        for (i = pRc->rcPhyState + 1; i < WLAN_RC_PHY_MAX; i++) {
            if (pRc->validPhyRateCount[i]) {
                break;
            }   
        }
        if (i < WLAN_RC_PHY_MAX) {
            nextIndex = rcGetBestRateIndex (pRateTable, pRc, i, 
                                            pRc->validPhyRateCount[i] - 1, 
                                            0, rssiLast);
            if (nextIndex >= 0) {
                pRc->rcPhyState = i;
                maxIndex        = pRc->validPhyRateCount[pRc->rcPhyState] - 1;
                minIndex        = 0;
                index           = nextIndex;
            }
            rate = pRc->validPhyRateIndex[pRc->rcPhyState][index];
        }
    } else if (index < 0) {

        /* Move to the lower Phy mode if we hit the minIndex */
        prevIndex = -1;
        for (i = pRc->rcPhyState -1; i >= 0; i--) {

            if (pRc->validPhyRateCount[i]) {
                prevIndex = rcGetBestRateIndex (pRateTable, pRc, i, 
                                                pRc->validPhyRateCount[i] - 1, 
                                                0, rssiLast);
                if (prevIndex > 0) {
                    break;
                }
            }
        }
        if (prevIndex < 0) {
            prevIndex = 0;
            for (i = 0; i < WLAN_RC_PHY_MAX; i++) {
                if (pRc->validPhyRateCount[i]) {
                    break;
                }   
            }
        }
        index           = prevIndex;
        pRc->rcPhyState = i;
        maxIndex        = pRc->validPhyRateCount[pRc->rcPhyState] - 1;
        minIndex        = 0;
        rate = pRc->validPhyRateIndex[pRc->rcPhyState][index];
    }
    /*
     * Must check the actual rate (rateKbps) to account for non-monoticity of
     * 11g's rate table
     */
    if (probeAllowed) {
        if ((index == maxIndex) && (pRc->rcPhyState == pRc->rateMaxPhy) 
                && (maxIndex <= pRc->rateMaxIndex)) 
        {
            /* Probe the next allowed phy state */
            if ((nowMsec - pRc->probeTime > pRateTable->probeInterval) &&
                (pRc->hwMaxRetryPktCnt >= 4))
            {
                 for (i = pRc->rcPhyState + 1; i < WLAN_RC_PHY_MAX; i++) {
                    if (pRc->validPhyRateCount[i]) {
                        index = 0;
                        rate = pRc->validPhyRateIndex[i][index];
                        pRc->probeState = i;
                        pRc->probeRate  = rate;
                        pRc->probeIndex = 0;
                        *isProbing 	    = TRUE;
                        break;
                    }   
                }
            }
        } else if ((pRc->rcPhyState == pRc->rateMaxPhy) && (index > pRc->rateMaxIndex)) {
    
            index = pRc->rateMaxIndex;
            rate = pRc->validPhyRateIndex[pRc->rateMaxPhy][index];
    
            if ((nowMsec - pRc->probeTime > pRateTable->probeInterval) &&
                (pRc->hwMaxRetryPktCnt >= 4))
            {
                rate = pRc->validPhyRateIndex[pRc->rcPhyState][index + 1];
                pRc->probeRate        = (A_UINT8)rate;
                pRc->probeTime        = nowMsec;
                pRc->probeIndex       = index + 1;
                pRc->probeState       = pRc->rateMaxPhy;
                pRc->hwMaxRetryPktCnt = 0;
                *isProbing 	          = TRUE;
            }
        }
    }

    /*
     * Make sure rate is not higher than the allowed maximum.
     * We should also enforce the min, but I suspect the min is
     * normally 1 rather than 0 because of the rate 9 vs 6 issue
     * in the old code.
     */
    if (rate > (pSib->txRateCtrl.rateTableSize - 1)) {
        rate = pSib->txRateCtrl.rateTableSize - 1;
    }

#if 0
    PKTLOG_RATE_CTL_GET(sc, pRc->misc, *pRc, (A_UINT16)frameLen,
                        (A_UINT8)rate, (A_UINT8)0, 
                        (A_UINT8)(halGetDefAntenna(pdevInfo) == 1) ? 0 : 1);
#endif

#ifdef DEBUG_PKTLOG
    {
        struct log_rcfind log_data;
        log_data.rc = pRc;
        log_data.rate = rate;
        log_data.rssiReduce = rssiReduce;
        log_data.isProbing = *isProbing;
        log_data.primeInUse = primeInUse;

        ath_log_rcfind(sc, &log_data, 0);
    }
#endif    

    /* record selected rate, which is used to decide if we want to do fast frame */
    if (!(*isProbing)) {
        pSib->lastRateKbps = pRateTable->info[rate].rateKbps;
    }
    return rate;
}

/*
 * This routine is called by the Tx interrupt service routine to give
 * the status of previous frames.
 */
void
rcUpdate(struct ath_softc *sc, struct ath_node *an,
    A_BOOL Xretries, int txRate, int retries, A_RSSI rssiAck, 
         A_UINT8 curTxAnt, const RATE_TABLE *pRateTable, 
         A_UINT16 nFrames, A_UINT16 nBad)
{
    struct ieee80211com  *ic        = &sc->sc_ic;
    struct atheros_node *pSib       = ATH_NODE_ATHEROS(an);
    struct TxRateCtrl_s *pRc;
    A_UINT32            nowMsec     = A_MS_TICKGET();
    A_BOOL              stateChange = FALSE;
    A_UINT8             lastPer;
    int                 rate,count;
    int                 i, j;

    static A_UINT32 nRetry2PerLookup[10] = {
        100 * 0 / 1,
        100 * 1 / 8,
        100 * 1 / 2,
        100 * 3 / 4,
        100 * 4 / 5,
        100 * 5 / 6,
        100 * 6 / 7,
        100 * 7 / 8,
        100 * 8 / 9,
        100 * 9 / 10
    };


    pRc        = &pSib->txRateCtrl;

    lastPer = pRc->state[txRate].per;

    ASSERT(retries >= 0 && retries < MAX_TX_RETRIES);
    ASSERT(txRate >= 0 && txRate < pRc->rateTableSize);

    if (Xretries) {
        /* Update the PER. */
        if (Xretries == 1) {
            pRc->state[txRate].per = 35 * (1 + (nFrames * 3 / 10));
            if (pRc->state[txRate].per > 100) {
                pRc->state[txRate].per = 100;
            }
        } else {
            count = sizeof(nRetry2PerLookup) / sizeof(nRetry2PerLookup[0]);
            if (retries >= count) {
                retries = count - 1;
            }
            /* new_PER = 3/4*old_PER + 1/4*(currentPER) */
            pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per - 
                                     (pRc->state[txRate].per >> 2) + 
                                     (nRetry2PerLookup[retries] >> 2) * (1 + nFrames * 3 / 10));
        }
        for (rate = txRate + 1; rate < pRc->rateTableSize; rate++) {
            if (pRateTable->info[rate].phy != pRateTable->info[txRate].phy) {
                break;
            }

            if (pRc->state[rate].per < pRc->state[txRate].per) {
                pRc->state[rate].per = pRc->state[txRate].per;
            }
        }

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
        pRc->probeState = 0;
        pRc->probeRate  = 0;
        pRc->probeIndex = 0;
    } else {
        /* Update the PER. */
        /* Make sure it doesn't index out of array's bounds. */
        count = sizeof(nRetry2PerLookup) / sizeof(nRetry2PerLookup[0]);
        if (retries >= count) {
            retries = count - 1;
        }

        if (nBad) {
            /* new_PER = 3/4*old_PER + 1/4*(currentPER)  */
            pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per - 
                                     (pRc->state[txRate].per >> 2) + 
                                     (nRetry2PerLookup[retries] >> 2) * (1 + nBad * 3 / 10));
        } else {
            /* new_PER = 7/8*old_PER + 1/8*(currentPER) */
            pRc->state[txRate].per = (A_UINT8)(pRc->state[txRate].per - (pRc->state[txRate].per >> 3) +
                                     (nRetry2PerLookup[retries] >> 3));
        }

        /* Update the RSSI threshold */
        if (pSib->antTx != curTxAnt) {
            /*
             * Hw does AABBAA on transmit attempts, and has flipped on this transmit.
             */
            pSib->antTx = curTxAnt;     /* 0 or 1 */
            sc->sc_stats.ast_ant_txswitch++;
            pRc->antFlipCnt = 1;

            if (ic->ic_opmode != IEEE80211_M_HOSTAP && !sc->sc_diversity) {
                /*
                 * Update rx ant (default) to this transmit antenna if:
                 *   1. The very first try on the other antenna succeeded and
                 *      with a very good ack rssi.
                 *   2. Or if we find ourselves succeeding for RX_FLIP_THRESHOLD
                 *      consecutive transmits on the other antenna;
                 * NOTE that the default antenna is preserved across a chip
                 *      reset by the hal software
                 */
                if (retries == 2 && rssiAck >= pRc->rssiLast + 2) {
                        (*sc->sc_setdefantenna)(sc, 
                                curTxAnt = curTxAnt ? 2 : 1);
                }
            }
        } else if (ic->ic_opmode != IEEE80211_M_HOSTAP) {
            if (!sc->sc_diversity && pRc->antFlipCnt < RX_FLIP_THRESHOLD) {
                pRc->antFlipCnt++;
                if (pRc->antFlipCnt == RX_FLIP_THRESHOLD) {
                        (*sc->sc_setdefantenna)(sc,
                                curTxAnt = curTxAnt ? 2 : 1);
                }
            }
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
            if (retries > 1) {
                pRc->probeRate  = 0;
                pRc->probeState = 0;
                pRc->probeIndex = 0;
            } else {
                pRc->rateMaxIndex = pRc->probeIndex;
                pRc->rateMaxPhy   = pRc->probeState;

                if (pRc->state[pRc->probeRate].per > 45) {
                    pRc->state[pRc->probeRate].per = 20;
                }

                pRc->probeRate = 0;
                pRc->probeState = 0;
                pRc->probeIndex = 0;
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
        } else {
            /*
             * It worked with no retries.  First ignore bogus (small)
             * rssiAck values.
             */
            if (pRc->hwMaxRetryPktCnt < 255) {
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

                if (pRc->rssiSumCnt >= 4) {
                    A_RSSI32 rssiAckAvg = (pRc->rssiSum + 2) / 4;

                    pRc->rssiSum = pRc->rssiSumCnt = 0;

                    /* Now reduce the current rssi threshold. */
                    if ((rssiAckAvg < pRc->state[txRate].rssiThres + 2) &&
                        (pRc->state[txRate].rssiThres > pRateTable->info[txRate].rssiAckValidMin))
                    {
                        pRc->state[txRate].rssiThres--;
                    }

                    stateChange = TRUE;
                }
            }
        }
    }

    /*
     * If this rate looks bad (high PER) then stop using it for
     * a while (except if we are probing).
     */
    if (pRc->state[txRate].per > 60 && txRate > 0 && 
        ((pRateTable->info[txRate].phy < pRc->rateMaxPhy) || 
         (txRate <= pRc->validPhyRateIndex[pRc->rateMaxPhy][pRc->rateMaxIndex])))
    {
        A_UINT8 rateIndex;
        rcGetNextLowerValidTxRate(pRateTable, pRc, txRate, &rateIndex);
        pRc->rateMaxPhy = 0;
        pRc->rateMaxIndex = 0;

        for (i = 0; i < WLAN_RC_PHY_MAX; i++) {
            if (pRc->validPhyRateCount[i]) {
                for (j = 0; j < pRc->validPhyRateCount[i]; j++) {
                    if (pRc->validPhyRateIndex[i][j] == rateIndex) {
                        pRc->rateMaxPhy = i;
                        pRc->rateMaxIndex = j;
                        break;
                    }
                }
                if (pRc->rateMaxPhy || pRc->rateMaxIndex) {
                    break;
                }
            }   
        }
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
    if (nowMsec - pRc->perDownTime >= (pRateTable->rssiReduceInterval/2)) {
        for (rate = 0; rate < pRc->rateTableSize; rate++) {
            pRc->state[rate].per = 7*pRc->state[rate].per/8;
        }

        pRc->perDownTime = nowMsec;
    }

#if 0
    PKTLOG_RATE_CTL_UPDATE(sc, *pRc, (A_UINT8)pSib->totalTxPending, 
                           pDevInfo->turboPrimeInfo.currentBoostState,
                           pDevInfo->bssDescr->athAdvCapElement.info.useTurboPrime,
                           pRc->rateTableSize, (A_UINT8)txRate, (A_UINT8)Xretries,
                           (A_UINT8)retries, (A_UINT16)frameLen, rssiAck, 
                           (A_UINT8)0, (A_UINT8)pSib->antTx) ;
#endif

#ifdef DEBUG_PKTLOG
    {
        struct log_rcupdate log_data;

        log_data.rc = pRc;
        log_data.txRate = txRate;
        log_data.Xretries = Xretries;
#ifdef TURBO_PRIME
        log_data.useTurboPrime = sc->sc_dturbo;        
#else
        log_data.useTurboPrime = 0;        
#endif         
        log_data.retries = retries;
        log_data.rssiAck = rssiAck;

        ath_log_rcupdate(sc, &log_data, 0);
    }
#endif
}

A_UINT8
rcRateValueToIndex(A_UINT32 txRateKbps, struct ath_softc *sc)
{
    struct atheros_softc *asc = (struct atheros_softc *) sc->sc_rc;
    const RATE_TABLE *pRateTable;
    A_UINT8 rate  = 0;
    A_UINT8 index = 0;

    pRateTable = asc->hwRateTable[sc->sc_curmode];

    while (rate < pRateTable->rateCount) {
        if (pRateTable->info[rate].valid &&
            txRateKbps == pRateTable->info[rate].rateKbps)
        {
            index = rate;
            break;
        }
        rate++;
    }

    return index;
}

A_UINT8
rcGetSigQuality(A_UINT8 txRate, struct ath_softc *sc, struct ath_node *an)
{
    struct atheros_node *pSib = ATH_NODE_ATHEROS(an);
    struct atheros_softc *asc = (struct atheros_softc *) sc->sc_rc;
    const RATE_TABLE *pRateTable;
    A_UINT32     rateKbps;
    A_UINT32     maxRateKbps;
    A_UINT8      qual = 0;
    A_UINT8      per;

    pRateTable  = asc->hwRateTable[sc->sc_curmode];
    maxRateKbps = pRateTable->info[pRateTable->rateCount - 1].rateKbps;
    rateKbps    = pRateTable->info[txRate].rateKbps;
    per         = pSib->txRateCtrl.state[txRate].per;

    qual = (A_UINT8)((rateKbps * (100 - per)) / maxRateKbps);

    return qual;
}


