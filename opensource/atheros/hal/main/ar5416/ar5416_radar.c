/*
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 */

#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#include "ah.h"
#include "ah_desc.h"
#include "ah_internal.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416desc.h"
#include "ar5416/ar5416phy.h"
#include "ar5416/ar5416radar.h"

/*
 * Finds the radar state entry that matches the current channel
 */

struct ar5416RadarState *
ar5416GetRadarChanState(struct ath_hal *ah, u_int8_t *index)
{
	struct ath_hal_5416 *ahp=AH5416(ah);
	struct ar5416RadarState *rs=AH_NULL;
	int i;
	
	for (i = 0; i < HAL_NUMRADAR_STATES; i++) {
		if (ahp->ah_radar[i].rs_chan == AH_PRIVATE(ah)->ah_curchan) {
			if (index != AH_NULL)
				*index = (u_int8_t) i;
			return &(ahp->ah_radar[i]);
		}
	}
	/* No existing channel found, look for first free channel state entry */
	for (i=0; i < HAL_NUMRADAR_STATES; i++) {
		if (ahp->ah_radar[i].rs_chan == AH_NULL) {
			rs = &(ahp->ah_radar[i]);
			/* Found one, set channel info and default thresholds */
			rs->rs_chan = AH_PRIVATE(ah)->ah_curchan;
			rs->rs_firpwr = HAL_RADAR_FIRPWR;
			rs->rs_radarRssi = HAL_RADAR_RRSSI;
			rs->rs_height = HAL_RADAR_HEIGHT;
			rs->rs_pulseRssi = HAL_RADAR_PRSSI;
			rs->rs_inband = HAL_RADAR_INBAND;
			if (index != AH_NULL)
				*index = (u_int8_t) i;
			return (rs);
		}
	}
	HALDEBUG(ah, "%s: No more radar states left.\n", __func__);
	return(AH_NULL);
}

void
ar5416ResetAR(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp=AH5416(ah);

	OS_MEMZERO(&ahp->ah_ar, sizeof(ahp->ah_ar));
	ahp->ah_ar.ar_packetThreshold = HAL_AR_PKT_COUNT_THRESH;
	ahp->ah_ar.ar_parThreshold = HAL_AR_ACK_DETECT_PAR_THRESH;
}

static void
ar5416ResetArQ(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp = AH5416(ah);

	OS_MEMZERO(ahp->ah_arq, sizeof(struct ar5416RadarQElem)*HAL_ARQ_SIZE);
	OS_MEMZERO(&ahp->ah_arqInfo, sizeof(ahp->ah_arqInfo));
	ahp->ah_arqInfo.ri_qsize = HAL_ARQ_SIZE;
	ahp->ah_arqInfo.ri_seqSize = HAL_ARQ_SEQSIZE;
}

void
ar5416ResetRadar(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp = AH5416(ah);

	OS_MEMZERO(&(ahp->ah_radar[0]), sizeof(ahp->ah_radar));
}

static void
ar5416ResetRadarQ(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp = AH5416(ah);

	OS_MEMZERO(ahp->ah_radarq, sizeof(struct ar5416RadarQElem)*HAL_RADARQ_SIZE);
	OS_MEMZERO(&ahp->ah_radarqInfo, sizeof(ahp->ah_radarqInfo));
	ahp->ah_radarqInfo.ri_qsize = HAL_RADARQ_SIZE;
	ahp->ah_radarqInfo.ri_seqSize = HAL_RADARQ_SEQSIZE;
}

HAL_STATUS
ar5416RadarAttach(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp=AH5416(ah);
	
	if (ar5416GetCapability(ah, HAL_CAP_PHYDIAG, HAL_CAP_RADAR, AH_NULL) == HAL_OK) {
		ahp->ah_procPhyErr |= HAL_RADAR_EN;
		ahp->ah_radarq = ath_hal_malloc(sizeof(struct ar5416RadarQElem)*HAL_RADARQ_SIZE);
		if (ahp->ah_radarq == AH_NULL) {
			HALDEBUG(ah, "%s: cannot allocate memory for radar q\n",
				 __func__);
			return HAL_ENOMEM;
		}
	}
	if (ar5416GetCapability(ah, HAL_CAP_PHYDIAG, HAL_CAP_AR, AH_NULL) == HAL_OK) {
		ahp->ah_procPhyErr |= HAL_AR_EN;
		ahp->ah_arq = ath_hal_malloc(sizeof(struct ar5416RadarQElem)*HAL_ARQ_SIZE);
		if (ahp->ah_arq == AH_NULL) {
			HALDEBUG(ah, "%s: cannot allocate memory for AR q\n",
				 __func__);
			return HAL_ENOMEM;
		}
	}
	ahp->ah_curchanRadIndex = -1;
	ar5416ResetRadar(ah);
	ar5416ResetRadarQ(ah);
	ar5416ResetAR(ah);
	ar5416ResetArQ(ah);
	return HAL_OK;
};

void
ar5416RadarDetach(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp = AH5416(ah);

	if (ahp->ah_radarq != AH_NULL) {
		ar5416ResetRadar(ah);
		ath_hal_free(ahp->ah_radarq);
	}
	if (ahp->ah_arq != AH_NULL) {
		ar5416ResetAR(ah);
		ath_hal_free(ahp->ah_arq);
	}
}

static void
ar5416QueueRadarEvent(struct ath_hal *ah, struct ath_desc *ds,
                      struct ath_rx_status *rx_status, 
                      struct ar5416RadarQElem *q, struct ar5416RadarQInfo *qInfo)
{
	u_int32_t index,seqNum;
	volatile register u_int32_t *data;
	volatile u_int32_t busy,val;
	u_int16_t qsize, seqsize, wrSeqNum, rdSeqNum, lowerts, thisTimeStamp;
	u_int32_t ts;

	ts = ar5416GetTsf32(ah);
	qsize = qInfo->ri_qsize;
	seqsize = qInfo->ri_seqSize;
	qInfo->ri_reader.rd_start = 1;

	index = qInfo->ri_writer.wr_index;
	seqNum = qInfo->ri_writer.wr_seq;
	data = &q[index].rq_busy;
	busy = *data;
	if (busy) {
		/*
		 * If the entry is busy, we can still write to it if
		 * the entry was already processed.  If the reader has read the
		 * sequence numbers before we get here, then it will just wait till
		 * the next time. Otherwise, it will not know the difference.
		 */
		data = &q[index].rq_seqNum;
		val = *data;
		wrSeqNum  = val & HAL_RADAR_SMASK;
		rdSeqNum = (val >> HAL_RADAR_SSHIFT) & HAL_RADAR_SMASK;
		if (wrSeqNum != rdSeqNum) {
			index = (index + 1)%qsize;
			seqNum = (seqNum + 1)%seqsize;
			qInfo->ri_reader.rd_resetVal =
				((seqNum & HAL_RADAR_SMASK) << HAL_RADAR_ISHIFT) |
				(index & HAL_RADAR_IMASK);
		}
	} else {
		data = &q[index].rq_seqNum;
		val = *data;
		wrSeqNum  = val & HAL_RADAR_SMASK;
		rdSeqNum = (val >> HAL_RADAR_SSHIFT) & HAL_RADAR_SMASK;
		if (wrSeqNum != rdSeqNum) {
			val = (((index + (qsize >> 1)) % qsize) & HAL_RADAR_IMASK) |
				((((seqNum - (qsize >> 1)) % seqsize) & HAL_RADAR_SMASK) << HAL_RADAR_ISHIFT);
			qInfo->ri_reader.rd_resetVal = val;
		}
	}
	q[index].rq_event.re_rssi = (u_int8_t) rx_status->rs_rssi;
	q[index].rq_event.re_dur = (rx_status->rs_datalen ?
			   (u_int32_t)(*((u_int8_t *) ds->ds_vdata)) : 0) & 0xff;	/* Only lower 8 bits are valid */
	q[index].rq_event.re_chanIndex = (u_int8_t) AH5416(ah)->ah_curchanRadIndex;

	thisTimeStamp = (rx_status->rs_tstamp) & HAL_RADAR_TSMASK;
	lowerts = (u_int16_t) (ts & HAL_RADAR_TSMASK);
	ts &= ~HAL_RADAR_TSMASK;
	if (lowerts < thisTimeStamp)
		ts -= (u_int32_t) (1 << HAL_RADAR_TSSHIFT);
	ts |= (u_int32_t) thisTimeStamp;
	q[index].rq_event.re_ts = ts;

	q[index].rq_seqNum = (HAL_RADAR_SMASK << HAL_RADAR_SSHIFT) |
		(seqNum & HAL_RADAR_SMASK);
	qInfo->ri_writer.wr_index = (index + 1)%qsize;
	qInfo->ri_writer.wr_seq = (seqNum + 1)%seqsize;
}

/*
 * Returns the next radar event in the structure provided by *radarEvent.
 * Also returns AH_TRUE if the returned value is valid.
 * Otherwise, AH_FALSE is returned indicating no more data is available.
 */

HAL_BOOL
ar5416DeQueueRadarEvent(struct ar5416RadarQElem *q,
			struct ar5416RadarQInfo *qInfo,
			struct ar5416RadarEvent *radarEvent,
			HAL_BOOL *flush)
{
	volatile register u_int32_t *data;
	volatile u_int32_t rval;
	u_int16_t qsize, seqsize, index, expSeqNum;
	u_int16_t ws,rs;

	*flush = AH_FALSE;
	if (qInfo->ri_reader.rd_start) {
		qsize = qInfo->ri_qsize;
		seqsize = qInfo->ri_seqSize;

		index = qInfo->ri_reader.rd_index;
		expSeqNum = qInfo->ri_reader.rd_expSeq;

		while (1) {
			data = &q[index].rq_busy;
			*data = 1;
			data = &q[index].rq_seqNum;
			ws = ((*data) & HAL_RADAR_SMASK);
			rs = ((*data) >> HAL_RADAR_SSHIFT);
			if (ws == rs) {
				return AH_FALSE;
			} else {
				if (expSeqNum == ws) {
					*radarEvent = q[index].rq_event;
					qInfo->ri_reader.rd_index = (index + 1)%qsize;
					qInfo->ri_reader.rd_expSeq = (expSeqNum + 1)%seqsize;
					*data = 0;
					return AH_TRUE;
				} else {
					*flush = AH_TRUE;
					data = &qInfo->ri_reader.rd_resetVal;
					rval = *data;
					index = (rval & HAL_RADAR_IMASK);
					expSeqNum = (rval >> HAL_RADAR_ISHIFT) & HAL_RADAR_SMASK;
				}
			}
		}
	} else
		return AH_FALSE;
}

void
ar5416ArEnable(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp=AH5416(ah);
	u_int32_t rfilt,val;
	struct ar5416ArState *ar;
	HAL_CHANNEL_INTERNAL *chan=AH_PRIVATE(ah)->ah_curchan;

	ar = (struct ar5416ArState *) &ahp->ah_ar;
	if (ahp->ah_procPhyErr & (HAL_RADAR_EN | HAL_AR_EN)) {
		if (chan->channelFlags & CHANNEL_108G) {
			/* We are in turbo G, so enable AR*/
			ar5416ResetAR(ah);
			val = OS_REG_READ(ah, AR_PHY_RADAR_0);
			ar->ar_radarRssi = HAL_AR_RADAR_RSSI_THR;
			val |= SM(ar->ar_radarRssi, AR_PHY_RADAR_0_RRSSI);
			rfilt = ar5416GetRxFilter(ah);
			rfilt |= HAL_RX_FILTER_PHYRADAR;
			ar5416SetRxFilter(ah,rfilt);
			OS_REG_WRITE(ah, AR_PHY_RADAR_0, val|AR_PHY_RADAR_0_ENA);
		}
	} else {
		rfilt = ar5416GetRxFilter(ah);
		rfilt &= ~HAL_RX_FILTER_PHYRADAR;
		ar5416SetRxFilter(ah,rfilt);
	}
}

/*
 * Disable all Radar detection (AR or Radar)
 */

void
ar5416RadarDisable(struct ath_hal *ah)
{
	u_int32_t rfilt;

	rfilt = ar5416GetRxFilter(ah);
	rfilt &= ~HAL_RX_FILTER_PHYERR;
	ar5416SetRxFilter(ah,rfilt);
	OS_REG_WRITE(ah, AR_PHY_RADAR_0,
		     OS_REG_READ(ah, AR_PHY_RADAR_0) & ~AR_PHY_RADAR_0_ENA);
	ar5416ResetRadar(ah);
	ar5416ResetRadarQ(ah);

}

void ar5416RadarEnable(struct ath_hal *ah)
{
	struct ath_hal_5416 *ahp=AH5416(ah);
	u_int32_t rfilt,val;
	struct ar5416RadarState *rs;
	HAL_CHANNEL_INTERNAL *chan=AH_PRIVATE(ah)->ah_curchan;

	rfilt = ar5416GetRxFilter(ah);
	if (ahp->ah_procPhyErr & (HAL_RADAR_EN | HAL_AR_EN)) {
		if (chan->privFlags & CHANNEL_DFS) {
			/*
			 * Disable radar detection in case we need to setup
			 * a new channel state and radars are somehow being
			 * reported. Avoids locking problem.
			 */

			ar5416RadarDisable(ah);
			if (ahp->ah_curchanRadIndex == -1) {
				u_int8_t index;
				rs = ar5416GetRadarChanState(ah, &index);
				if (rs != AH_NULL)
					ahp->ah_curchanRadIndex = (int16_t) index;
			} else
				rs = &ahp->ah_radar[ahp->ah_curchanRadIndex];
			if (rs != AH_NULL) {
				val = 0;
				val |= SM(rs->rs_firpwr, AR_PHY_RADAR_0_FIRPWR);
				val |= SM(rs->rs_radarRssi, AR_PHY_RADAR_0_RRSSI);
				val |= SM(rs->rs_height, AR_PHY_RADAR_0_HEIGHT);
				val |= SM(rs->rs_pulseRssi, AR_PHY_RADAR_0_PRSSI);
				val |= SM(rs->rs_inband, AR_PHY_RADAR_0_INBAND);

				rfilt |= HAL_RX_FILTER_PHYRADAR;
				ar5416SetRxFilter(ah,rfilt);
				OS_REG_WRITE(ah, AR_PHY_RADAR_0, val|AR_PHY_RADAR_0_ENA);
			} else
				HALDEBUG(ah, "%s: No more radar states left\n", __func__);
		} else {
			if (!(chan->channelFlags & CHANNEL_2GHZ)) {
				/* Disable Radar if not 2GHZ channel and not DFS */
				rfilt &= ~HAL_RX_FILTER_PHYRADAR;
				ar5416SetRxFilter(ah,rfilt);
			}
		}
	} else {
		/* Disable Radar if RADAR or AR not enabled */
		rfilt &= ~HAL_RX_FILTER_PHYRADAR;
		ar5416SetRxFilter(ah,rfilt);
	}
}

/*
 * Disable Radar if AR channel
 */

void
ar5416ArDisable(struct ath_hal *ah)
{
	u_int32_t rfilt;
	HAL_CHANNEL_INTERNAL *chan=AH_PRIVATE(ah)->ah_curchan;

	if (chan->channelFlags & CHANNEL_108G) {
		rfilt = ar5416GetRxFilter(ah);
		rfilt &= ~HAL_RX_FILTER_PHYERR;
		ar5416SetRxFilter(ah,rfilt);
		OS_REG_WRITE(ah, AR_PHY_RADAR_0,
			     OS_REG_READ(ah, AR_PHY_RADAR_0) & ~AR_PHY_RADAR_0_ENA);
		ar5416ResetAR(ah);
		ar5416ResetArQ(ah);
	}
}

void
ar5416ProcessRadar(struct ath_hal *ah, struct ath_desc *ds, 
                   struct ath_rx_status *rx_status)
{
	struct ath_hal_5416 *ahp=AH5416(ah);

	HAL_CHANNEL_INTERNAL *chan=AH_PRIVATE(ah)->ah_curchan;

	if ((chan->channelFlags & CHANNEL_108G) &&
	    (AH_PRIVATE(ah)->ah_opmode == HAL_M_HOSTAP)) {
		ar5416QueueRadarEvent(ah, ds, rx_status, ahp->ah_arq, 
                              &ahp->ah_arqInfo);
	}
	if (chan->privFlags & CHANNEL_DFS)
		ar5416QueueRadarEvent(ah, ds, rx_status, ahp->ah_radarq, 
                              &ahp->ah_radarqInfo);
}

#if 0
static void
ath_radar_event(struct net_device *dev, const struct ath_desc *ds,
		struct sk_buff *skb)
{
	struct ath_softc *sc = dev->priv;
	struct ath_phyerr_state *pe = NULL;
	struct ath_radar_state *radarState;

	KASSERT(sc->sc_phyerr_state != NULL,
		("NULL phyerr_state"));
	pe = (struct ath_phyerr_state *) sc->sc_phyerr_state;

	/* We might be in the middle of reset_radar */
	if (pe->pe_curRadar == NULL)
		return;

	radarState = pe->pe_curRadar;
	radarState->rad_numRadarEvents++;
#if 0
	if ((radarState->rad_numRadarEvents % 10) == 0)
		printk("Radar event count = %d\n",
		       radarState->rad_numRadarEvents);
#endif
}

#endif

#define UPDATE_TOP_THREE_PEAKS(_histo, _peakPtrList, _currWidth) { \
	if ((_histo)[(_peakPtrList)[0]] < (_histo)[(_currWidth)]) {	\
		(_peakPtrList)[2] = (_currWidth != (_peakPtrList)[1]) ?	\
					(_peakPtrList)[1] : (_peakPtrList)[2];  \
		(_peakPtrList)[1] = (_peakPtrList)[0]; \
		(_peakPtrList)[0] = (_currWidth); \
	} else if ((_currWidth != (_peakPtrList)[0])	\
			&& ((_histo)[(_peakPtrList)[1]] < (_histo)[(_currWidth)])) { \
		(_peakPtrList)[2] = (_peakPtrList)[1]; \
		(_peakPtrList)[1] = (_currWidth);      \
	} else if ((_currWidth != (_peakPtrList)[1])   \
			&& (_currWidth != (_peakPtrList)[0])  \
			&& ((_histo)[(_peakPtrList)[2]] < (_histo)[(_currWidth)])) { \
		(_peakPtrList)[2] = (_currWidth);  \
	} \
}

/*
 * This routine builds the histogram based on radar duration and does pattern matching
 * on incoming radars to determine if neighboring traffic is present.
 */

void
ar5416ProcessArEvent(struct ath_hal *ah, HAL_CHANNEL *chan)
{
	struct ath_hal_5416 *ahp=AH5416(ah);
	struct ar5416ArState *ar;
	u_int32_t sumPeak=0,numPeaks,rssi,width,origRegionSum=0, i;
	u_int16_t thisTimeStamp;
	struct ar5416RadarEvent re;
	HAL_BOOL isFlush=AH_FALSE;


	ar = (struct ar5416ArState *) &(ahp->ah_ar);
	while (ar5416DeQueueRadarEvent(ahp->ah_arq, &ahp->ah_arqInfo, &re, &isFlush)) {
		if (isFlush) {
			ar5416ResetAR(ah);
		}
		thisTimeStamp = re.re_ts & HAL_RADAR_TSMASK;
		rssi = re.re_rssi;
		width = re.re_dur;

		/* determine if current radar is an extension of previous radar */
		if (ar->ar_prevWidth == 255) {
			/* tag on previous width for consideraion of low data rate ACKs */
			ar->ar_prevWidth += width;
			width = (width == 255) ? 255 : ar->ar_prevWidth;
		} else if ((width == 255) &&
			   (ar->ar_prevWidth == 510 ||
			    ar->ar_prevWidth == 765 ||
			    ar->ar_prevWidth == 1020)) {
			/* Aggregate up to 5 consecuate max radar widths
			 * to consider 11Mbps long preamble 1500-byte pkts
			 */
			ar->ar_prevWidth += width;
		} else if (ar->ar_prevWidth == 1275 && width != 255) {
			/* Found 5th consecute maxed out radar, reset history */
			width += ar->ar_prevWidth;
			ar->ar_prevWidth = 0;
		} else if (ar->ar_prevWidth > 255) {
			/* Ignore if there are less than 5 consecutive maxed out radars */
			ar->ar_prevWidth = width;
			width = 255;
		} else {
			ar->ar_prevWidth = width;
		}
		/* For ignoring noises with radar duration in ranges of 3-30: AP4x */
		if ((width >= 257 && width <= 278) ||	/* Region 7 - 5.5Mbps (long pre) ACK = 270 = 216 us */
		    (width >= 295 && width <= 325) ||	/* Region 8 - 2Mbps (long pre) ACKC = 320 = 256us */
		    (width >= 1280 && width <= 1300)) {
			u_int16_t wrapAroundAdj=0;
			u_int16_t base = (width >= 1280) ? 1275 : 255;
			if (thisTimeStamp < ar->ar_prevTimeStamp) {
				wrapAroundAdj = 32768;
			}
			if ((thisTimeStamp + wrapAroundAdj - ar->ar_prevTimeStamp) !=
			    (width - base)) {
				width = 1;
			}
		}
		if (width <= 10)
			continue;
		
		/*
		 * Overloading the width=2 in: Store a count of radars w/max duration
		 * and high RSSI (not noise)
		 */
		if ((width == 255) && (rssi > HAL_AR_RSSI_THRESH_STRONG_PKTS))
			width = 2;
		
		/*
		 * Overloading the width=3 bin:
		 *   Double and store a count of rdars of durtaion that matches 11Mbps (long preamble)
		 *   TCP ACKs or 1500-byte data packets
		 */
		if ((width >= 1280 && width <= 1300) ||
		    (width >= 318 && width <= 325)) {
			width = 3;
			ar->ar_phyErrCount[3] += 2;
			ar->ar_ackSum += 2;
		}
		
		/* build histogram of radar duration */
		if (width > 0 && width <= 510)
			ar->ar_phyErrCount[width]++;
		else
			/* invalid radar width, throw it away */
			continue;
		/* Received radar of interest (i.e., signature match), proceed to check if
		 * there is enough neighboring traffic to drop out of Turbo
		 */
		if ((width >= 33 && width <= 38) ||          /* Region 0: 24Mbps ACK = 35 = 28us */
		    (width >= 39 && width <= 44) ||          /* Region 1: 12Mbps ACK = 40 = 32us */
		    (width >= 53 && width <= 58) ||          /* Region 2:  6Mbps ACK = 55 = 44us */
		    (width >= 126 && width <= 140) ||        /* Region 3: 11Mbps ACK = 135 = 108us */
		    (width >= 141 && width <= 160) ||        /* Region 4: 5.5Mbps ACK = 150 = 120us */
		    (width >= 189 && width <= 210) ||        /* Region 5:  2Mbps ACK = 200 = 160us */
		    (width >= 360 && width <= 380) ||        /* Region 6   1Mbps ACK = 400 = 320us */
		    (width >= 257 && width <= 270) ||        /* Region 7   5.5Mbps (Long Pre) ACK = 270 = 216us */
		    (width >= 295 && width <= 302) ||        /* Region 8   2Mbps (Long Pre) ACK = 320 = 256us */
		    /* Ignoring Region 9 due to overlap with 255 which is same as board noise */
		    /* Region 9  11Mbps (Long Pre) ACK = 255 = 204us */
		    (width == 3)) {
			ar->ar_ackSum++;
			
			/* double the count for strong radars that match one of the ACK signatures */
			if (rssi > HAL_AR_RSSI_DOUBLE_THRESHOLD) {
				ar->ar_phyErrCount[width]++;
				ar->ar_ackSum++;
			}
			UPDATE_TOP_THREE_PEAKS(ar->ar_phyErrCount,
					       ar->ar_peakList, width);
			/* sum the counts of these peaks */
			numPeaks = HAL_MAX_NUM_PEAKS;
			origRegionSum = ar->ar_ackSum;
			for (i=0; i<= HAL_MAX_NUM_PEAKS; i++) {
				if (ar->ar_peakList[i] > 0) {
					if ((i==0) &&
					    (ar->ar_peakList[i] == 3) &&
					    (ar->ar_phyErrCount[3] <
					     ar->ar_phyErrCount[2]) &&
					    (ar->ar_phyErrCount[3] > 6)) {
						/*
						 * If the top peak is one that
						 * maches the 11Mbps long
						 * preamble TCP Ack/1500-byte
						 * data, include the count for
						 * radars that hav emax
						 * duration and high rssi
						 * (width = 2) to boost the
						 * sum for the PAR test that
						 * follows */
						sumPeak += (ar->ar_phyErrCount[2]
							    + ar->ar_phyErrCount[3]);
						ar->ar_ackSum += (ar->ar_phyErrCount[2]
								  + ar->ar_phyErrCount[3]);
					} else {
						sumPeak += ar->ar_phyErrCount[ar->ar_peakList[i]];
					}
				} else
					numPeaks--;
			}
			
			/*
			 * If sum of patterns matches exceeds packet threshold,
			 * perform comparison between peak-to-avg ratio against parThreshold
			 */
			if ((ar->ar_ackSum > ar->ar_packetThreshold) &&
			    ((sumPeak * HAL_AR_REGION_WIDTH) > (ar->ar_parThreshold * numPeaks *
								ar->ar_ackSum))) {
				/* neighboring traffic detected, get out of Turbo */
				chan->privFlags |= CHANNEL_INTERFERENCE;
				
				OS_MEMZERO(ar->ar_peakList, sizeof(ar->ar_peakList));
				ar->ar_ackSum = 0;
				OS_MEMZERO(ar->ar_phyErrCount, sizeof(ar->ar_phyErrCount));
			} else {
				/*
				 * reset sum of matches to discount the count of
				 * strong radars with max duration
				 */
				ar->ar_ackSum = origRegionSum;
			}
		}
		ar->ar_prevTimeStamp = thisTimeStamp;
	}
}

#endif /* AH_SUPPORT_AR5416 */
