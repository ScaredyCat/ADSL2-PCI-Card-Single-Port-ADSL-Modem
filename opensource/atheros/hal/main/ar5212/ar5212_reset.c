/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5212/ar5212_reset.c#4 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5212

#include "ah.h"
#include "ah_xr.h"
#include "ah_internal.h"
#include "ah_devid.h"

#include "ar5212/ar5212.h"
#include "ar5212/ar5212reg.h"
#include "ar5212/ar5212phy.h"
#ifdef AH_SUPPORT_AR5311
#include "ar5212/ar5311reg.h"
#endif

/* Add static register initialization vectors */
#define AH_5212_COMMON
#include "ar5212/ar5212.ini"

/* Additional Time delay to wait after activiting the Base band */
#define BASE_ACTIVATE_DELAY	100	/* 100 usec */
#define PLL_SETTLE_DELAY	300	/* 300 usec */

static HAL_BOOL ar5212SetResetReg(struct ath_hal *, u_int32_t resetMask);
/* NB: public for 5312 use */
int16_t		ar5212GetNf(struct ath_hal *, HAL_CHANNEL_INTERNAL *);
HAL_BOOL	ar5212SetBoardValues(struct ath_hal *, HAL_CHANNEL_INTERNAL *);
void		ar5212SetDeltaSlope(struct ath_hal *, HAL_CHANNEL *);
HAL_BOOL	ar5212SetTransmitPower(struct ath_hal *ah,
		HAL_CHANNEL_INTERNAL *chan, u_int16_t *rfXpdGain);
static HAL_BOOL ar5212SetRateTable(struct ath_hal *, 
		HAL_CHANNEL *, int16_t tpcScaleReduction, int16_t powerLimit,
		int16_t *minPower, int16_t *maxPower);
static void ar5212CorrectGainDelta(struct ath_hal *, int twiceOfdmCckDelta);
static void ar5212GetTargetPowers(struct ath_hal *, HAL_CHANNEL *,
		TRGT_POWER_INFO *pPowerInfo, u_int16_t numChannels,
		TRGT_POWER_INFO *pNewPower);
static u_int16_t ar5212GetMaxEdgePower(u_int16_t channel,
		RD_EDGES_POWER  *pRdEdgesPower);
static void ar5212RequestRfgain(struct ath_hal *);
static HAL_BOOL ar5212InvalidGainReadback(struct ath_hal *, GAIN_VALUES *);
static HAL_BOOL ar5212IsGainAdjustNeeded(struct ath_hal *, const GAIN_VALUES *);
static int32_t ar5212AdjustGain(struct ath_hal *, GAIN_VALUES *);
void		ar5212SetRateDurationTable(struct ath_hal *, HAL_CHANNEL *);
static u_int32_t ar5212GetRfField(u_int32_t *rfBuf, u_int32_t numBits,
		u_int32_t firstBit, u_int32_t column);
static void ar5212GetGainFCorrection(struct ath_hal *ah);
HAL_BOOL ar5212SetXrMode(struct ath_hal *ah, HAL_OPMODE opmode,HAL_CHANNEL *chan);
void ar5212SetCompRegs(struct ath_hal *ah);
void ar5212SetIFSTiming(struct ath_hal *, HAL_CHANNEL *);

/* NB: public for RF backend use */
void	ar5212GetLowerUpperValues(u_int16_t value,
		u_int16_t *pList, u_int16_t listSize,
		u_int16_t *pLowerValue, u_int16_t *pUpperValue);
void	ar5212ModifyRfBuffer(u_int32_t *rfBuf, u_int32_t reg32,
		u_int32_t numBits, u_int32_t firstBit, u_int32_t column);

/*
 * WAR for bug 6773.  OS_DELAY() does a PIO READ on the PCI bus which allows
 * other cards' DMA reads to complete in the middle of our reset.
 */
#define WAR_6773(x) do {		\
	if ((++(x) % 64) == 0)		\
		OS_DELAY(1);		\
} while (0)

#define REG_WRITE_ARRAY(regArray, column, regWr) do {                  	\
	int r;								\
	for (r = 0; r < N(regArray); r++) {				\
		OS_REG_WRITE(ah, (regArray)[r][0], (regArray)[r][(column)]);\
		WAR_6773(regWr);					\
	}								\
} while (0)

/*
 * Places the device in and out of reset and then places sane
 * values in the registers based on EEPROM config, initialization
 * vectors (as determined by the mode), and station configuration
 *
 * bChannelChange is used to preserve DMA/PCU registers across
 * a HW Reset during channel change.
 */
HAL_BOOL
ar5212Reset(struct ath_hal *ah, HAL_OPMODE opmode,
	HAL_CHANNEL *chan, HAL_BOOL bChannelChange, HAL_STATUS *status)
{
#define	N(a)	(sizeof (a) / sizeof (a[0]))
#define	FAIL(_code)	do { ecode = _code; goto bad; } while (0)
	u_int32_t softLedCfg, softLedState;
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CHANNEL_INTERNAL *ichan;
	u_int32_t saveFrameSeqCount, saveDefAntenna, saveLedState;
	u_int32_t macStaId1, synthDelay, txFrm2TxDStart;
	u_int16_t rfXpdGain[2];
	int16_t cckOfdmPwrDelta = 0;
	u_int modesIndex, freqIndex;
	HAL_STATUS ecode;
	int i, regWrites = 0;

	OS_MARK(ah, AH_MARK_RESET, bChannelChange);
#define	IS(_c,_f)	(((_c)->channelFlags & _f) || 0)
	if ((IS(chan, CHANNEL_2GHZ) ^ IS(chan, CHANNEL_5GHZ)) == 0) {
		HALDEBUG(ah, "%s: invalid channel %u/0x%x; not marked as "
			 "2GHz or 5GHz\n", __func__,
			chan->channel, chan->channelFlags);
		FAIL(HAL_EINVAL);
	}
	if ((IS(chan, CHANNEL_OFDM) ^ IS(chan, CHANNEL_CCK)) == 0) {
		HALDEBUG(ah, "%s: invalid channel %u/0x%x; not marked as "
			"OFDM or CCK\n", __func__,
			chan->channel, chan->channelFlags);
		FAIL(HAL_EINVAL);
	}
#undef IS

	/* Bring out of sleep mode */
	if (!ar5212SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE))
		return AH_FALSE;

	/*
	 * Map public channel to private.
	 */
	ichan = ath_hal_checkchannel(ah, chan);
	if (ichan == AH_NULL) {
		HALDEBUG(ah, "%s: invalid channel %u/0x%x; no mapping\n",
			__func__, chan->channel, chan->channelFlags);
		FAIL(HAL_EINVAL);
	}
	switch (opmode) {
	case HAL_M_STA:
	case HAL_M_IBSS:
	case HAL_M_HOSTAP:
	case HAL_M_MONITOR:
		break;
	default:
		HALDEBUG(ah, "%s: invalid operating mode %u\n",
			__func__, opmode);
		FAIL(HAL_EINVAL);
		break;
	}
	HALASSERT(ahp->ah_eeversion >= AR_EEPROM_VER3);

	/* Preserve certain DMA hardware registers on a channel change */
	if (bChannelChange) {
		/*
		 * AR5212 WAR
		 *
		 * On Venice, the TSF is almost preserved across a reset;
		 * it requires the WAR of doubling writes to the RESET_TSF
		 * bit in the AR_BEACON register; it also has the quirk
		 * of the TSF going back in time on the station (station
		 * latches onto the last beacon's tsf during a reset 50%
		 * of the times); the latter is not a problem for adhoc
		 * stations since as long as the TSF is behind, it will
		 * get resynchronized on receiving the next beacon; the
		 * TSF going backwards in time could be a problem for the
		 * sleep operation (supported on infrastructure stations
		 * only) - the best and most general fix for this situation
		 * is to resynchronize the various sleep/beacon timers on
		 * the receipt of the next beacon i.e. when the TSF itself
		 * gets resynchronized to the AP's TSF - power save is
		 * needed to be temporarily disabled until that time
		 *
		 * Need to save the sequence number to restore it after
		 * the reset!
		 */
		saveFrameSeqCount = OS_REG_READ(ah, AR_D_SEQNUM);

		ar5212GetNf(ah, ichan);
	} else
		saveFrameSeqCount = 0;		/* NB: silence compiler */


	/*
	 * Preserve the antenna on a channel change
	 */
	saveDefAntenna = OS_REG_READ(ah, AR_DEF_ANTENNA);
	if (saveDefAntenna == 0)		/* XXX magic constants */
		saveDefAntenna = 1;

	/* Save hardware flag before chip reset clears the register */
	macStaId1 = OS_REG_READ(ah, AR_STA_ID1) & AR_STA_ID1_BASE_RATE_11B;

	/* Save led state from pci config register */
	saveLedState = OS_REG_READ(ah, AR_PCICFG) &
		(AR_PCICFG_LEDCTL | AR_PCICFG_LEDMODE | AR_PCICFG_LEDBLINK |
		 AR_PCICFG_LEDSLOW);
	softLedCfg = OS_REG_READ(ah, AR_GPIOCR);
	softLedState = OS_REG_READ(ah, AR_GPIODO);

	ar5212RestoreClock(ah, opmode);		/* move to refclk operation */

	/*
	 * Adjust gain parameters before reset if
	 * there's an outstanding gain updated.
	 */
	(void) ar5212GetRfgain(ah);

	if (!ar5212ChipReset(ah, chan)) {
		HALDEBUG(ah, "%s: chip reset failed\n", __func__);
		FAIL(HAL_EIO);
	}

	/* Setup the indices for the next set of register array writes */
	switch (chan->channelFlags & CHANNEL_ALL) {
	case CHANNEL_A:
	case CHANNEL_XR_A:
		modesIndex = 1;
		freqIndex  = 1;
		break;
	case CHANNEL_T:
	case CHANNEL_XR_T:
		modesIndex = 2;
		freqIndex  = 1;
		break;
	case CHANNEL_B:
		modesIndex = 3;
		freqIndex  = 2;
		break;
	case CHANNEL_PUREG:
	case CHANNEL_XR_G:
		modesIndex = 4;
		freqIndex  = 2;
		break;
	case CHANNEL_108G:
		modesIndex = 5;
		freqIndex  = 2;
		break;
	default:
		HALDEBUG(ah, "%s: invalid channel flags 0x%x\n",
			__func__, chan->channelFlags);
		FAIL(HAL_EINVAL);
	}

	OS_MARK(ah, AH_MARK_RESET_LINE, __LINE__);

	/* Set correct Baseband to analog shift setting to access analog chips. */
	OS_REG_WRITE(ah, AR_PHY(0), 0x00000007);

	REG_WRITE_ARRAY(ar5212Modes, modesIndex, regWrites);
	/* Write Common Array Parameters */
	for (i = 0; i < N(ar5212Common); i++) {
		u_int32_t reg = ar5212Common[i][0];
		/* XXX timer/beacon setup registers? */
		/* On channel change, don't reset the PCU registers */
		if (!(bChannelChange && (0x8000 <= reg && reg < 0x9000))) {
			OS_REG_WRITE(ah, reg, ar5212Common[i][1]);
			WAR_6773(regWrites);
		}
	}
	ahp->ah_rfHal.writeRegs(ah, modesIndex, freqIndex, regWrites);

	OS_MARK(ah, AH_MARK_RESET_LINE, __LINE__);

	if (IS_CHAN_HALF_RATE(chan) || IS_CHAN_QUARTER_RATE(chan)) {
		ar5212SetIFSTiming(ah, chan);
	}

	/* Overwrite INI values for revised chipsets */
	if (AH_PRIVATE(ah)->ah_phyRev >= AR_PHY_CHIP_ID_REV_2) {
		/* ADC_CTL */
		OS_REG_WRITE(ah, AR_PHY_ADC_CTL,
			SM(2, AR_PHY_ADC_CTL_OFF_INBUFGAIN) |
			SM(2, AR_PHY_ADC_CTL_ON_INBUFGAIN) |
			AR_PHY_ADC_CTL_OFF_PWDDAC |
			AR_PHY_ADC_CTL_OFF_PWDADC);

		/* TX_PWR_ADJ */
		if (chan->channel == 2484) {
		    cckOfdmPwrDelta = SCALE_OC_DELTA(ahp->ah_cckOfdmPwrDelta - ahp->ah_scaledCh14FilterCckDelta);
		} else {
		    cckOfdmPwrDelta = SCALE_OC_DELTA(ahp->ah_cckOfdmPwrDelta);
		}

		if (IS_CHAN_G(chan)) {
		    OS_REG_WRITE(ah, AR_PHY_TXPWRADJ,
			SM((ahp->ah_cckOfdmPwrDelta*-1), AR_PHY_TXPWRADJ_CCK_GAIN_DELTA) |
			SM((cckOfdmPwrDelta*-1), AR_PHY_TXPWRADJ_CCK_PCDAC_INDEX));
		} else {
		    OS_REG_WRITE(ah, AR_PHY_TXPWRADJ, 0);
		}

		/* Add barker RSSI thresh enable as disabled */
		OS_REG_CLR_BIT(ah, AR_PHY_DAG_CTRLCCK,
			AR_PHY_DAG_CTRLCCK_EN_RSSI_THR);
		OS_REG_RMW_FIELD(ah, AR_PHY_DAG_CTRLCCK,
			AR_PHY_DAG_CTRLCCK_RSSI_THR, 2);

		/* Set the mute mask to the correct default */
		OS_REG_WRITE(ah, AR_SEQ_MASK, 0x0000000F);
	}

	if (AH_PRIVATE(ah)->ah_phyRev >= AR_PHY_CHIP_ID_REV_3) {
		/* Clear reg to alllow RX_CLEAR line debug */
		OS_REG_WRITE(ah, AR_PHY_BLUETOOTH,  0);
	}
	if (AH_PRIVATE(ah)->ah_phyRev >= AR_PHY_CHIP_ID_REV_4) {
#ifdef notyet
		/* Enable burst prefetch for the data queues */
		OS_REG_RMW_FIELD(ah, AR_D_FPCTL, ... );
		/* Enable double-buffering */
		OS_REG_CLR_BIT(ah, AR_TXCFG, AR_TXCFG_DBL_BUF_DIS);
#endif
	}

        /* Set ADC/DAC select values */
        OS_REG_WRITE(ah, AR_PHY_SLEEP_SCAL, 0x0e);

	/* Setup the transmit power values. */
	if (!ar5212SetTransmitPower(ah, ichan, rfXpdGain)) {
		HALDEBUG(ah, "%s: error init'ing transmit power\n", __func__);
		FAIL(HAL_EIO);
	}

	/* Write the analog registers */
	if (!ahp->ah_rfHal.setRfRegs(ah, ichan, modesIndex, rfXpdGain)) {
		HALDEBUG(ah, "%s: ar5212SetRfRegs failed\n", __func__);
		FAIL(HAL_EIO);
	}

	/* Write delta slope for OFDM enabled modes (A, G, Turbo) */
	if (IS_CHAN_OFDM(chan))
		ar5212SetDeltaSlope(ah, chan);

	/* Setup board specific options for EEPROM version 3 */
	if (!ar5212SetBoardValues(ah, ichan)) {
		HALDEBUG(ah, "%s: error setting board options\n", __func__);
		FAIL(HAL_EIO);
	}

	/* Restore certain DMA hardware registers on a channel change */
	if (bChannelChange)
		OS_REG_WRITE(ah, AR_D_SEQNUM, saveFrameSeqCount);

	OS_MARK(ah, AH_MARK_RESET_LINE, __LINE__);

	OS_REG_WRITE(ah, AR_STA_ID0, LE_READ_4(ahp->ah_macaddr));
	OS_REG_WRITE(ah, AR_STA_ID1, LE_READ_2(ahp->ah_macaddr + 4)
		| macStaId1
		| AR_STA_ID1_RTS_USE_DEF
		| ahp->ah_staId1Defaults
	);
	ar5212SetOperatingMode(ah, opmode);

	/* Set Venice BSSID mask according to current state */
	OS_REG_WRITE(ah, AR_BSSMSKL, LE_READ_4(ahp->ah_bssidmask));
	OS_REG_WRITE(ah, AR_BSSMSKU, LE_READ_2(ahp->ah_bssidmask + 4));

	/* Restore previous led state */
	OS_REG_WRITE(ah, AR_PCICFG, OS_REG_READ(ah, AR_PCICFG) | saveLedState);

	/* Restore soft Led state to GPIO */
	OS_REG_WRITE(ah, AR_GPIOCR, softLedCfg);
	OS_REG_WRITE(ah, AR_GPIODO, softLedState);

	/* Restore previous antenna */
	OS_REG_WRITE(ah, AR_DEF_ANTENNA, saveDefAntenna);

	/* then our BSSID */
	OS_REG_WRITE(ah, AR_BSS_ID0, LE_READ_4(ahp->ah_bssid));
	OS_REG_WRITE(ah, AR_BSS_ID1, LE_READ_2(ahp->ah_bssid + 4));

	OS_REG_WRITE(ah, AR_ISR, ~0);		/* cleared on write */

	OS_REG_WRITE(ah, AR_RSSI_THR, INIT_RSSI_THR);

	if (!ahp->ah_rfHal.setChannel(ah, ichan))
		FAIL(HAL_EIO);

	OS_MARK(ah, AH_MARK_RESET_LINE, __LINE__);

	ar5212SetCoverageClass(ah, AH_PRIVATE(ah)->ah_coverageClass, 1);

	ar5212SetRateDurationTable(ah, chan);

	/* Set Tx frame start to tx data start delay */
	if (IS_5112(ah) && (IS_CHAN_HALF_RATE(AH_PRIVATE(ah)->ah_curchan) ||
			IS_CHAN_QUARTER_RATE(AH_PRIVATE(ah)->ah_curchan))) {
		txFrm2TxDStart = 
			(IS_CHAN_HALF_RATE(AH_PRIVATE(ah)->ah_curchan)) ?
					TX_FRAME_D_START_HALF_RATE:
					TX_FRAME_D_START_QUARTER_RATE;
		OS_REG_RMW_FIELD(ah, AR_PHY_TX_CTL, 
			AR_PHY_TX_FRAME_TO_TX_DATA_START, txFrm2TxDStart);
	}

	/*
	 * Setup fast diversity.
	 * Fast diversity can be enabled or disabled via regadd.txt.
	 * Default is enabled.
	 * For reference,
	 *    Disable: reg        val
	 *             0x00009860 0x00009d18 (if 11a / 11g, else no change)
	 *             0x00009970 0x192bb514
	 *             0x0000a208 0xd03e4648
	 *
	 *    Enable:  0x00009860 0x00009d10 (if 11a / 11g, else no change)
	 *             0x00009970 0x192fb514
	 *             0x0000a208 0xd03e6788
	 */

	/* XXX Setup pre PHY ENABLE EAR additions */
	/*
	 * Wait for the frequency synth to settle (synth goes on
	 * via AR_PHY_ACTIVE_EN).  Read the phy active delay register.
	 * Value is in 100ns increments.
	 */
	synthDelay = OS_REG_READ(ah, AR_PHY_RX_DELAY) & AR_PHY_RX_DELAY_DELAY;
	if (IS_CHAN_CCK(chan)) {
		synthDelay = (4 * synthDelay) / 22;
	} else {
		synthDelay /= 10;
	}

	/* Activate the PHY (includes baseband activate and synthesizer on) */
	OS_REG_WRITE(ah, AR_PHY_ACTIVE, AR_PHY_ACTIVE_EN);

	/* 
	 * There is an issue if the AP starts the calibration before
	 * the base band timeout completes.  This could result in the
	 * rx_clear false triggering.  As a workaround we add delay an
	 * extra BASE_ACTIVATE_DELAY usecs to ensure this condition
	 * does not happen.
	 */
	if (IS_CHAN_HALF_RATE(AH_PRIVATE(ah)->ah_curchan)) {
		OS_DELAY((synthDelay << 1) + BASE_ACTIVATE_DELAY);
	} else if (IS_CHAN_QUARTER_RATE(AH_PRIVATE(ah)->ah_curchan)) {
		OS_DELAY((synthDelay << 2) + BASE_ACTIVATE_DELAY);
	} else {
		OS_DELAY(synthDelay + BASE_ACTIVATE_DELAY);
	}

	/* Calibrate the AGC and start a NF calculation */
	OS_REG_WRITE(ah, AR_PHY_AGC_CONTROL,
		  OS_REG_READ(ah, AR_PHY_AGC_CONTROL)
		| AR_PHY_AGC_CONTROL_CAL
		| AR_PHY_AGC_CONTROL_NF);

	if (!IS_CHAN_B(chan) && ahp->ah_bIQCalibration != IQ_CAL_DONE) {
		/* Start IQ calibration w/ 2^(INIT_IQCAL_LOG_COUNT_MAX+1) samples */
		OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4, 
			AR_PHY_TIMING_CTRL4_IQCAL_LOG_COUNT_MAX,
			INIT_IQCAL_LOG_COUNT_MAX);
		OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4,
			AR_PHY_TIMING_CTRL4_DO_IQCAL);
		ahp->ah_bIQCalibration = IQ_CAL_RUNNING;
	} else
		ahp->ah_bIQCalibration = IQ_CAL_INACTIVE;

	/* Setup compression registers */
	ar5212SetCompRegs(ah);

	/* Set 1:1 QCU to DCU mapping for all queues */
	for (i = 0; i < AR_NUM_DCU; i++)
		OS_REG_WRITE(ah, AR_DQCUMASK(i), 1 << i);

	ahp->ah_intrTxqs = 0;
	for (i = 0; i < AH_PRIVATE(ah)->ah_caps.halTotalQueues; i++)
		ar5212ResetTxQueue(ah, i);

	/*
	 * Setup interrupt handling.  Note that ar5212ResetTxQueue
	 * manipulates the secondary IMR's as queues are enabled
	 * and disabled.  This is done with RMW ops to insure the
	 * settings we make here are preserved.
	 */
	ahp->ah_maskReg = AR_IMR_TXOK | AR_IMR_TXERR | AR_IMR_TXURN
			| AR_IMR_RXOK | AR_IMR_RXERR | AR_IMR_RXORN
			| AR_IMR_HIUERR
			;
	if (opmode == HAL_M_HOSTAP)
		ahp->ah_maskReg |= AR_IMR_MIB;
	OS_REG_WRITE(ah, AR_IMR, ahp->ah_maskReg);
	/* Enable bus errors that are OR'd to set the HIUERR bit */
	OS_REG_WRITE(ah, AR_IMR_S2,
		OS_REG_READ(ah, AR_IMR_S2)
		| AR_IMR_S2_MCABT | AR_IMR_S2_SSERR | AR_IMR_S2_DPERR);

	if (ar5212GetRfKill(ah))
		ar5212EnableRfKill(ah);

	if (!ath_hal_wait(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_CAL, 0)) {
		HALDEBUG(ah, "%s: offset calibration failed to complete in 1ms;"
			" noisy environment?\n", __func__);
	}

	/*
	 * Set clocks back to 32kHz if they had been using refClk, then
	 * use an external 32kHz crystal when sleeping, if one exists.
	 */
	ar5212SetupClock(ah, opmode);

	/*
	 * Writing to AR_BEACON will start timers. Hence it should
	 * be the last register to be written. Do not reset tsf, do
	 * not enable beacons at this point, but preserve other values
	 * like beaconInterval.
	 */
	OS_REG_WRITE(ah, AR_BEACON,
		(OS_REG_READ(ah, AR_BEACON) &~ (AR_BEACON_EN | AR_BEACON_RESET_TSF)));

	/* XXX Setup post reset EAR additions */

#ifdef AH_SUPPORT_XR
	/* it should be changed to IS_CHAN_XR once the reg domain sets the XR flags on channels */
	if ( (opmode != HAL_M_STA || IS_CHAN_XR(chan)) && !ar5212SetXrMode(ah, opmode,chan)) {
		HALDEBUG(ah, "%s: unable to setup XR mode\n", __func__);
		FAIL(HAL_EIO);
	}
#endif /* AH_SUPPORT_XR */

	if (AH_PRIVATE(ah)->ah_macVersion > AR_SREV_VERSION_VENICE ||
	    (AH_PRIVATE(ah)->ah_macVersion == AR_SREV_VERSION_VENICE &&
	     AH_PRIVATE(ah)->ah_macRev >= AR_SREV_GRIFFIN_LITE)) {
		OS_REG_WRITE(ah, AR_QOS_CONTROL, 0x100aa);	/* XXX magic */
		OS_REG_WRITE(ah, AR_QOS_SELECT, 0x3210);	/* XXX magic */
	}

	/* Turn on NOACK Support for QoS packets */
	OS_REG_WRITE(ah, AR_NOACK,
		SM(2, AR_NOACK_2BIT_VALUE) |
		SM(5, AR_NOACK_BIT_OFFSET) |
		SM(0, AR_NOACK_BYTE_OFFSET));

	/* Restore user-specified settings */
	if (ahp->ah_miscMode != 0)
		OS_REG_WRITE(ah, AR_MISC_MODE, ahp->ah_miscMode);
	if (ahp->ah_slottime != (u_int) -1)
		ar5212SetSlotTime(ah, ahp->ah_slottime);
	if (ahp->ah_acktimeout != (u_int) -1)
		ar5212SetAckTimeout(ah, ahp->ah_acktimeout);
	if (ahp->ah_ctstimeout != (u_int) -1)
		ar5212SetCTSTimeout(ah, ahp->ah_ctstimeout);
	if (AH_PRIVATE(ah)->ah_diagreg != 0)
		OS_REG_WRITE(ah, AR_DIAG_SW, AH_PRIVATE(ah)->ah_diagreg);

	AH_PRIVATE(ah)->ah_opmode = opmode;	/* record operating mode */

	if (bChannelChange) {
		if (chan->channelFlags & CHANNEL_108G)
			ar5212ArEnable(ah);
		else if ((chan->channelFlags & (CHANNEL_A | CHANNEL_ST)) &&
			 (chan->privFlags & CHANNEL_DFS)) {
			ar5212RadarEnable(ah);
		}
	}

	HALDEBUG(ah, "%s: done\n", __func__);

	OS_MARK(ah, AH_MARK_RESET_DONE, 0);

	return AH_TRUE;
bad:
	OS_MARK(ah, AH_MARK_RESET_DONE, ecode);
	if (*status)
		*status = ecode;
	return AH_FALSE;
#undef FAIL
#undef N
}

void
ar5212SetOperatingMode(struct ath_hal *ah, int opmode)
{
	u_int32_t val;

	val = OS_REG_READ(ah, AR_STA_ID1);
	val &= ~(AR_STA_ID1_STA_AP | AR_STA_ID1_ADHOC);
	switch (opmode) {
	case HAL_M_HOSTAP:
		OS_REG_WRITE(ah, AR_STA_ID1, val | AR_STA_ID1_STA_AP
					| AR_STA_ID1_KSRCH_MODE);
		OS_REG_CLR_BIT(ah, AR_CFG, AR_CFG_AP_ADHOC_INDICATION);
		break;
	case HAL_M_IBSS:
		OS_REG_WRITE(ah, AR_STA_ID1, val | AR_STA_ID1_ADHOC
					| AR_STA_ID1_KSRCH_MODE);
		OS_REG_SET_BIT(ah, AR_CFG, AR_CFG_AP_ADHOC_INDICATION);
		break;
	case HAL_M_STA:
	case HAL_M_MONITOR:
		OS_REG_WRITE(ah, AR_STA_ID1, val | AR_STA_ID1_KSRCH_MODE);
		break;
	}
}

#ifdef AH_SUPPORT_XR
/*
 * Configure XR mode support.
 */
HAL_BOOL
ar5212SetXrMode(struct ath_hal *ah, HAL_OPMODE opmode,HAL_CHANNEL *chan)
{
#define MAC_CLKS(_usec) (IS_CHAN_5GHZ(chan) ? (_usec) * 40  :  (_usec) * 44)
	OS_REG_WRITE(ah, AR_XRMODE,
		  (OS_REG_READ(ah, AR_XRMODE) &~ (AR_XRMODE_XR_POLL_TYPE|AR_XRMODE_XR_POLL_SUBTYPE))
		| SM(FRAME_DATA, AR_XRMODE_XR_POLL_TYPE)
		| SM(SUBT_NODATA_CFPOLL, AR_XRMODE_XR_POLL_SUBTYPE)
	);
	/*
	 * Acks to XR frames require longer timeouts - we end
	 * up increasing timeout corresponding to all types of
	 * frames on the AP; the sta will anyway do XR frames
	 * only; given by HW guys to be 100usec
	 */
	OS_REG_RMW_FIELD(ah, AR_TIME_OUT, AR_TIME_OUT_ACK, MAC_CLKS(100));

	if (opmode == HAL_M_HOSTAP) {
		/*
		 * timeout values on the AP side corresponding to
		 * ctsChirp->xrData and grpPoll->rtsChirp; the former is
		 * from the detection of the rtsChirp to the detection
		 * of XR data - so a rtsChirp slot, a ctsChirp slot,
		 * the XR data detection time and a fudge; the latter
		 * i.e. the group poll timeout value depends on the
		 * XR mode aifs and cwMin/Max in slot time units plus
		 * time for a chirp and some fudge; the AP also needs
		 * the slot delay to figure out the timing between
		 * rtsChirp->ctsChirp
		 */
		OS_REG_WRITE(ah, AR_XRTO,
			  (OS_REG_READ(ah, AR_XRTO) &~
				(AR_XRTO_CHIRP_TO|AR_XRTO_POLL_TO))
			| SM(MAC_CLKS(2 * XR_SLOT_DELAY +
				XR_DATA_DETECT_DELAY + 80),
				AR_XRTO_CHIRP_TO)
			| SM(MAC_CLKS((INIT_AIFS_XR + INIT_CWMAX_XR) *
				XR_SLOT_DELAY + XR_CHIRP_DUR + 16),
				 AR_XRTO_POLL_TO)
		);
		OS_REG_WRITE(ah, AR_XRDEL,
			  (OS_REG_READ(ah, AR_XRDEL) &~ (AR_XRDEL_SLOT_DELAY))
					 | SM(MAC_CLKS(XR_SLOT_DELAY), AR_XRDEL_SLOT_DELAY));
	} else {
		/*
		 * used by the station to figure out timing between
		 * end of rtsChirp->xrData; there is one ctsChirp
		 * slot to be left in between - so two slots minus
		 * the chirp duration and a fudge
		 */
		OS_REG_RMW_FIELD(ah, AR_XRDEL, AR_XRDEL_CHIRP_DATA_DELAY,
		    MAC_CLKS(2 * XR_SLOT_DELAY - (XR_CHIRP_DUR + 1)));

		OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, MAC_CLKS(XR_SLOT_DELAY));
		OS_REG_WRITE(ah, AR_XRMODE,
			OS_REG_READ(ah, AR_XRMODE) | AR_XRMODE_XR_WAIT_FOR_POLL);
	}
	return AH_TRUE;
#undef MAC_CLKS
}
#endif /* AH_SUPPORT_XR */

/*
 * Places the PHY and Radio chips into reset.  A full reset
 * must be called to leave this state.  The PCI/MAC/PCU are
 * not placed into reset as we must receive interrupt to
 * re-enable the hardware.
 */
HAL_BOOL
ar5212PhyDisable(struct ath_hal *ah)
{
	return ar5212SetResetReg(ah, AR_RC_BB);
}

/*
 * Places all of hardware into reset
 */
HAL_BOOL
ar5212Disable(struct ath_hal *ah)
{
	if (!ar5212SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE))
		return AH_FALSE;
	/*
	 * Reset the HW - PCI must be reset after the rest of the
	 * device has been reset.
	 */
	return ar5212SetResetReg(ah, AR_RC_MAC | AR_RC_BB | AR_RC_PCI);
}

/*
 * Places the hardware into reset and then pulls it out of reset
 *
 * TODO: Only write the PLL if we're changing to or from CCK mode
 * 
 * WARNING: The order of the PLL and mode registers must be correct.
 */
HAL_BOOL
ar5212ChipReset(struct ath_hal *ah, HAL_CHANNEL *chan)
{

	OS_MARK(ah, AH_MARK_CHIPRESET, chan ? chan->channel : 0);

	/*
	 * Reset the HW - PCI must be reset after the rest of the
	 * device has been reset
	 */
	if (!ar5212SetResetReg(ah, AR_RC_MAC | AR_RC_BB | AR_RC_PCI))
		return AH_FALSE;

	/* Bring out of sleep mode (AGAIN) */
	if (!ar5212SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE))
		return AH_FALSE;

	/* Clear warm reset register */
	if (!ar5212SetResetReg(ah, 0))
		return AH_FALSE;

	/*
	 * Perform warm reset before the mode/PLL/turbo registers
	 * are changed in order to deactivate the radio.  Mode changes
	 * with an active radio can result in corrupted shifts to the
	 * radio device.
	 */

	/*
	 * Set CCK and Turbo modes correctly.
	 */
	if (chan != AH_NULL) {		/* NB: can be null during attach */
		u_int32_t rfMode, phyPLL = 0, curPhyPLL, turbo;

		if (IS_5112(ah)) {
			rfMode = AR_PHY_MODE_AR5112;
			if (IS_CHAN_CCK(chan) || IS_CHAN_G(chan)) {
				phyPLL = AR_PHY_PLL_CTL_44_5112;
			} else {
				if (IS_CHAN_HALF_RATE(chan)) {
					phyPLL = AR_PHY_PLL_CTL_40_5112_HALF;
				} else if (IS_CHAN_QUARTER_RATE(chan)) {
					phyPLL = AR_PHY_PLL_CTL_40_5112_QUARTER;
				} else {
					phyPLL = AR_PHY_PLL_CTL_40_5112;
				}
			}
		} else {
			rfMode = AR_PHY_MODE_AR5111;
			if (IS_CHAN_CCK(chan) || IS_CHAN_G(chan)) {
				phyPLL = AR_PHY_PLL_CTL_44;
			} else {
				if (IS_CHAN_HALF_RATE(chan)) {
					phyPLL = AR_PHY_PLL_CTL_40_HALF;
				} else if (IS_CHAN_QUARTER_RATE(chan)) {
					phyPLL = AR_PHY_PLL_CTL_40_QUARTER;
				} else {
					phyPLL = AR_PHY_PLL_CTL_40;
				}
			}
		}
		if (IS_CHAN_OFDM(chan) && (IS_CHAN_CCK(chan) || 
					   IS_CHAN_G(chan)))
			rfMode |= AR_PHY_MODE_DYNAMIC;
		else if (IS_CHAN_OFDM(chan))
			rfMode |= AR_PHY_MODE_OFDM;
		else
			rfMode |= AR_PHY_MODE_CCK;
		if (IS_CHAN_5GHZ(chan))
			rfMode |= AR_PHY_MODE_RF5GHZ;
		else
			rfMode |= AR_PHY_MODE_RF2GHZ;
		turbo = IS_CHAN_TURBO(chan) ?
			(AR_PHY_FC_TURBO_MODE | AR_PHY_FC_TURBO_SHORT) : 0;
		curPhyPLL = OS_REG_READ(ah, AR_PHY_PLL_CTL);
#ifdef AH_SUPPORT_XR
		rfMode |= AR_PHY_MODE_XR;
#endif
		/*
		 * PLL, Mode, and Turbo values must be written in the correct
		 * order to ensure:
		 * - The PLL cannot be set to 44 unless the CCK or DYNAMIC
		 *   mode bit is set
		 * - Turbo cannot be set at the same time as CCK or DYNAMIC
		 */
		if (IS_CHAN_CCK(chan) || IS_CHAN_G(chan)) {
			OS_REG_WRITE(ah, AR_PHY_TURBO, turbo);
			OS_REG_WRITE(ah, AR_PHY_MODE, rfMode);
			if (curPhyPLL != phyPLL) {
				OS_REG_WRITE(ah,  AR_PHY_PLL_CTL,  phyPLL);
				/* Wait for the PLL to settle */
				OS_DELAY(PLL_SETTLE_DELAY);
			}
		} else {
			if (curPhyPLL != phyPLL) {
				OS_REG_WRITE(ah,  AR_PHY_PLL_CTL,  phyPLL);
				/* Wait for the PLL to settle */
				OS_DELAY(PLL_SETTLE_DELAY);
			}
			OS_REG_WRITE(ah, AR_PHY_TURBO, turbo);
			OS_REG_WRITE(ah, AR_PHY_MODE, rfMode);
		}
	}
	return AH_TRUE;
}

/*
 * Recalibrate the lower PHY chips to account for temperature/environment
 * changes.
 */
HAL_BOOL
ar5212PerCalibration(struct ath_hal *ah,  HAL_CHANNEL *chan)
{
#define IQ_CAL_TRIES    10
	struct ath_hal_5212 *ahp = AH5212(ah);
	HAL_CHANNEL_INTERNAL *ichan;
	int32_t qCoff, qCoffDenom;
	int32_t iqCorrMeas, iCoff, iCoffDenom;
	u_int32_t powerMeasQ, powerMeasI;

	OS_MARK(ah, AH_MARK_PERCAL, chan->channel);
	ichan = ath_hal_checkchannel(ah, chan);
	if (ichan == AH_NULL) {
		HALDEBUG(ah, "%s: invalid channel %u/0x%x; no mapping\n",
			__func__, chan->channel, chan->channelFlags);
		return AH_FALSE;
	}

	/* XXX EAR */

	/* IQ calibration in progress. Check to see if it has finished. */
	if (ahp->ah_bIQCalibration == IQ_CAL_RUNNING &&
	    !(OS_REG_READ(ah, AR_PHY_TIMING_CTRL4) & AR_PHY_TIMING_CTRL4_DO_IQCAL)) {
		int i;

		/* IQ Calibration has finished. */
		ahp->ah_bIQCalibration = IQ_CAL_INACTIVE;

		/* workaround for misgated IQ Cal results */
		for (i = 0; i < IQ_CAL_TRIES; i++) {
			/* Read calibration results. */
			powerMeasI = OS_REG_READ(ah, AR_PHY_IQCAL_RES_PWR_MEAS_I);
			powerMeasQ = OS_REG_READ(ah, AR_PHY_IQCAL_RES_PWR_MEAS_Q);
			iqCorrMeas = OS_REG_READ(ah, AR_PHY_IQCAL_RES_IQ_CORR_MEAS);
			if (powerMeasI && powerMeasQ)
				break;
			/* Do we really need this??? */
			OS_REG_WRITE (ah,  AR_PHY_TIMING_CTRL4,
				      OS_REG_READ(ah,  AR_PHY_TIMING_CTRL4) |
				      AR_PHY_TIMING_CTRL4_DO_IQCAL);
		}

		/*
		 * Prescale these values to remove 64-bit operation
		 * requirement at the loss of a little precision.
		 */
		iCoffDenom = (powerMeasI / 2 + powerMeasQ / 2) / 128;
		qCoffDenom = powerMeasQ / 128;

		/* Protect against divide-by-0 and loss of sign bits. */
		if (iCoffDenom != 0 && qCoffDenom >= 2) {
			iCoff = (int8_t)(-iqCorrMeas) / iCoffDenom;
			/* IQCORR_Q_I_COFF is a signed 6 bit number */
			if (iCoff < -32) {
				iCoff = -32;
			} else if (iCoff > 31) {
				iCoff = 31;
			}

			/* IQCORR_Q_Q_COFF is a signed 5 bit number */
			qCoff = (powerMeasI / qCoffDenom) - 128;
			if (qCoff < -16) {
				qCoff = -16;
			} else if (qCoff > 15) {
				qCoff = 15;
			}
#ifdef CALIBRATION_DEBUG
			HALDEBUG(ah, "****************** MISGATED IQ CAL! *******************\n");
			HALDEBUG(ah, "time       = %d, i = %d, \n",
				OS_GETUPTIME(ah), i);
			HALDEBUG(ah, "powerMeasI = 0x%08x\n", powerMeasI);
			HALDEBUG(ah, "powerMeasQ = 0x%08x\n", powerMeasQ);
			HALDEBUG(ah, "iqCorrMeas = 0x%08x\n", iqCorrMeas);
			HALDEBUG(ah, "iCoff      = %d\n", iCoff);
			HALDEBUG(ah, "qCoff      = %d\n", qCoff);
#endif
			/* Write values and enable correction */
			OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
				AR_PHY_TIMING_CTRL4_IQCORR_Q_I_COFF, iCoff);
			OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
				AR_PHY_TIMING_CTRL4_IQCORR_Q_Q_COFF, qCoff);
			OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4, 
				AR_PHY_TIMING_CTRL4_IQCORR_ENABLE);

			ahp->ah_bIQCalibration = IQ_CAL_DONE;
			ichan->iqCalValid = AH_TRUE;
			ichan->iCoff = iCoff;
			ichan->qCoff = qCoff;
		}
	} else if (!IS_CHAN_B(chan) &&
		   ahp->ah_bIQCalibration == IQ_CAL_DONE &&
		   !ichan->iqCalValid) {
		/*
		 * Start IQ calibration if configured channel has changed.
		 * Use a magic number of 15 based on default value.
		 */
		OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
			AR_PHY_TIMING_CTRL4_IQCAL_LOG_COUNT_MAX,
			INIT_IQCAL_LOG_COUNT_MAX);
		OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4,
			AR_PHY_TIMING_CTRL4_DO_IQCAL);
		ahp->ah_bIQCalibration = IQ_CAL_RUNNING;
	}
	/* XXX EAR */

	/* Check noise floor results */
	ar5212GetNf(ah, ichan);
	if ((ichan->channelFlags & CHANNEL_CW_INT) == 0) {
		/* run noise floor calibration */
		OS_REG_SET_BIT(ah, AR_PHY_AGC_CONTROL, AR_PHY_AGC_CONTROL_NF);

		/* Perform calibration for 5Ghz channels and any OFDM on 5112 */
		if ((IS_CHAN_5GHZ(chan) ||
		    (IS_5112(ah) && IS_CHAN_OFDM(chan))) && !IS_2413(ah))
			ar5212RequestRfgain(ah);

		/* XXX EAR */
	} else {
		/* report up and clear internal state */
		chan->channelFlags |= CHANNEL_CW_INT;
		ichan->channelFlags &= ~CHANNEL_CW_INT;
	}
	return AH_TRUE;
#undef IQ_CAL_TRIES
}

/*
 * Write the given reset bit mask into the reset register
 */
static HAL_BOOL
ar5212SetResetReg(struct ath_hal *ah, u_int32_t resetMask)
{
	u_int32_t mask = resetMask ? resetMask : ~0;
	HAL_BOOL rt;

	/* XXX ar5212MacStop & co. */

	(void) OS_REG_READ(ah, AR_RXDP);/* flush any pending MMR writes */
	OS_REG_WRITE(ah, AR_RC, resetMask);
	OS_DELAY(15);			/* need to wait at least 128 clocks
					   when reseting PCI before read */
	mask &= (AR_RC_MAC | AR_RC_BB);
	resetMask &= (AR_RC_MAC | AR_RC_BB);
	rt = ath_hal_wait(ah, AR_RC, mask, resetMask);
        if ((resetMask & AR_RC_MAC) == 0) {
		if (isBigEndian()) {
			/*
			 * Set CFG, little-endian for register
			 * and descriptor accesses.
			 */
			mask = INIT_CONFIG_STATUS | AR_CFG_SWRD | AR_CFG_SWRG;
#ifndef AH_NEED_DESC_SWAP
			mask |= AR_CFG_SWTD;
#endif
			OS_REG_WRITE(ah, AR_CFG, LE_READ_4(&mask));
		} else
			OS_REG_WRITE(ah, AR_CFG, INIT_CONFIG_STATUS);
		if (ar5212SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE))
			(void) OS_REG_READ(ah, AR_ISR_RAC);
	}
	return rt;
}

int16_t
ar5212GetNoiseFloor(struct ath_hal *ah)
{
	int16_t nf = (OS_REG_READ(ah, AR_PHY(25)) >> 19) & 0x1ff;
	if (nf & 0x100)
		nf = 0 - ((nf ^ 0x1ff) + 1);
	return nf;
}

static HAL_BOOL
getNoiseFloorThresh(struct ath_hal *ah, const HAL_CHANNEL_INTERNAL *chan,
	int16_t *nft)
{
	struct ath_hal_5212 *ahp = AH5212(ah);

	switch (chan->channelFlags & CHANNEL_ALL_NOTURBO) {
	case CHANNEL_A:
		*nft = ahp->ah_noiseFloorThresh[headerInfo11A];
		break;
	case CHANNEL_B:
		*nft = ahp->ah_noiseFloorThresh[headerInfo11B];
		break;
	case CHANNEL_PUREG:
		*nft = ahp->ah_noiseFloorThresh[headerInfo11G];
		break;
	default:
		HALDEBUG(ah, "%s: invalid channel flags 0x%x\n",
			__func__, chan->channelFlags);
		return AH_FALSE;
	}
	return AH_TRUE;
}

/*
 * Read the NF and check it against the noise floor threshhold
 */
int16_t
ar5212GetNf(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
	int16_t nf, nfThresh;

	if (OS_REG_READ(ah, AR_PHY_AGC_CONTROL) & AR_PHY_AGC_CONTROL_NF) {
		HALDEBUG(ah, "%s: NF did not complete in calibration window\n",
			__func__);
		nf = 0;
	} else {
		/* Finished NF cal, check against threshold */
		nf = ar5212GetNoiseFloor(ah);
		if (getNoiseFloorThresh(ah, chan, &nfThresh) && nf > nfThresh) {
			HALDEBUG(ah, "%s: noise floor failed detected; "
				"detected %u, threshold %u\n", __func__,
				nf, nfThresh);
			/*
			 * NB: Don't discriminate 2.4 vs 5Ghz, if this
			 *     happens it indicates a problem regardless
			 *     of the band.
			 */
			chan->channelFlags |= CHANNEL_CW_INT;
		}
	}
	return (chan->rawNoiseFloor = nf);
}

/*
 * Set up compression configuration registers
 */
void
ar5212SetCompRegs(struct ath_hal *ah)
{
	u_int32_t i;

        /* Check if h/w supports compression */
	if (ath_hal_getcapability(ah, HAL_CAP_COMPRESSION, 0, 0) != HAL_OK) {
		return;
	}

	OS_REG_WRITE(ah, AR_DCCFG, 1);

	OS_REG_WRITE(ah, AR_CCFG,
		(AR_COMPRESSION_WINDOW_SIZE >> 8) & AR_CCFG_WIN_M);

	OS_REG_WRITE(ah, AR_CCFG,
		OS_REG_READ(ah, AR_CCFG) | AR_CCFG_MIB_INT_EN);
	OS_REG_WRITE(ah, AR_CCUCFG,
		AR_CCUCFG_RESET_VAL | AR_CCUCFG_CATCHUP_EN);

	OS_REG_WRITE(ah, AR_CPCOVF, 0);

	/* reset decompression mask */
	for (i = 0; i < HAL_DECOMP_MASK_SIZE; i++) {
		OS_REG_WRITE(ah, AR_DCM_A, i);
		OS_REG_WRITE(ah, AR_DCM_D, ah->ah_decompMask[i]);
	}
}

#define	MAX_ANALOG_START	319		/* XXX */

/*
 * Find analog bits of given parameter data and return a reversed value
 */
static u_int32_t
ar5212GetRfField(u_int32_t *rfBuf, u_int32_t numBits, u_int32_t firstBit, u_int32_t column)
{
	u_int32_t reg32 = 0, mask, arrayEntry, lastBit;
	u_int32_t bitPosition, bitsShifted;
	int32_t bitsLeft;

	HALASSERT(column <= 3);
	HALASSERT(numBits <= 32);
	HALASSERT(firstBit + numBits <= MAX_ANALOG_START);

	arrayEntry = (firstBit - 1) / 8;
	bitPosition = (firstBit - 1) % 8;
	bitsLeft = numBits;
	bitsShifted = 0;
	while (bitsLeft > 0) {
		lastBit = (bitPosition + bitsLeft > 8) ?
			(8) : (bitPosition + bitsLeft);
		mask = (((1 << lastBit) - 1) ^ ((1 << bitPosition) - 1)) <<
			(column * 8);
		reg32 |= (((rfBuf[arrayEntry] & mask) >> (column * 8)) >>
			bitPosition) << bitsShifted;
		bitsShifted += lastBit - bitPosition;
		bitsLeft -= (8 - bitPosition);
		bitPosition = 0;
		arrayEntry++;
	}
	reg32 = ath_hal_reverseBits(reg32, numBits);
	return reg32;
}

/*
 * Read EEPROM header info and program the device for correct operation
 * given the channel value.
 */
HAL_BOOL
ar5212SetBoardValues(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan)
{
#define NO_FALSE_DETECT_BACKOFF   2
#define CB22_FALSE_DETECT_BACKOFF 6
#define	AR_PHY_BIS(_ah, _reg, _mask, _val) \
	OS_REG_WRITE(_ah, AR_PHY(_reg), \
		(OS_REG_READ(_ah, AR_PHY(_reg)) & _mask) | (_val));
	struct ath_hal_5212 *ahp = AH5212(ah);
	int arrayMode, falseDectectBackoff;
	int is2GHz = IS_CHAN_2GHZ(chan);
	int8_t adcDesiredSize, pgaDesiredSize;
	u_int16_t switchSettling, txrxAtten, rxtxMargin;
	int iCoff, qCoff;

	switch (chan->channelFlags & CHANNEL_ALL) {
	case CHANNEL_A:
	case CHANNEL_T:
	case CHANNEL_XR_A:
	case CHANNEL_XR_T:
		arrayMode = headerInfo11A;
		if (!IS_5112(ah) && !IS_2413(ah))
			OS_REG_RMW_FIELD(ah, AR_PHY_FRAME_CTL,
				AR_PHY_FRAME_CTL_TX_CLIP,
				ahp->ah_gainValues.currStep->paramVal[GP_TXCLIP]);
		break;
	case CHANNEL_B:
		arrayMode = headerInfo11B;
		break;
	case CHANNEL_G:
	case CHANNEL_108G:
	case CHANNEL_XR_G:
		arrayMode = headerInfo11G;
		break;
	default:
		HALDEBUG(ah, "%s: invalid channel flags 0x%x\n",
			__func__, chan->channelFlags);
		return AH_FALSE;
	}

	/* Set the antenna register(s) correctly for the chip revision */
	AR_PHY_BIS(ah, 68, 0xFFFFFC06,
		(ahp->ah_antennaControl[0][arrayMode] << 4) | 0x1);

	ar5212SetAntennaSwitch(ah, ahp->ah_diversityControl,
		(HAL_CHANNEL *) chan);

	/* Set the Noise Floor Thresh on ar5211 devices */
	OS_REG_WRITE(ah, AR_PHY(90),
		(ahp->ah_noiseFloorThresh[arrayMode] & 0x1FF)
		| (1 << 9));

	if (ahp->ah_eeversion >= AR_EEPROM_VER5_0 && IS_CHAN_TURBO(chan)) {
		switchSettling = ahp->ah_switchSettlingTurbo[is2GHz];
		adcDesiredSize = ahp->ah_adcDesiredSizeTurbo[is2GHz];
		pgaDesiredSize = ahp->ah_pgaDesiredSizeTurbo[is2GHz];
		txrxAtten = ahp->ah_txrxAttenTurbo[is2GHz];
		rxtxMargin = ahp->ah_rxtxMarginTurbo[is2GHz];
	} else {
		switchSettling = ahp->ah_switchSettling[arrayMode];
		adcDesiredSize = ahp->ah_adcDesiredSize[arrayMode];
		pgaDesiredSize = ahp->ah_pgaDesiredSize[is2GHz];
		txrxAtten = ahp->ah_txrxAtten[is2GHz];
		rxtxMargin = ahp->ah_rxtxMargin[is2GHz];
	}

	OS_REG_RMW_FIELD(ah, AR_PHY_SETTLING, 
			 AR_PHY_SETTLING_SWITCH, switchSettling);
	OS_REG_RMW_FIELD(ah, AR_PHY_DESIRED_SZ,
			 AR_PHY_DESIRED_SZ_ADC, adcDesiredSize);
	OS_REG_RMW_FIELD(ah, AR_PHY_DESIRED_SZ,
			 AR_PHY_DESIRED_SZ_PGA, pgaDesiredSize);
	OS_REG_RMW_FIELD(ah, AR_PHY_RXGAIN,
			 AR_PHY_RXGAIN_TXRX_ATTEN, txrxAtten);
	OS_REG_WRITE(ah, AR_PHY(13),
		(ahp->ah_txEndToXPAOff[arrayMode] << 24)
		| (ahp->ah_txEndToXPAOff[arrayMode] << 16)
		| (ahp->ah_txFrameToXPAOn[arrayMode] << 8)
		| ahp->ah_txFrameToXPAOn[arrayMode]);
	AR_PHY_BIS(ah, 10, 0xFFFF00FF,
		ahp->ah_txEndToXLNAOn[arrayMode] << 8);
	AR_PHY_BIS(ah, 25, 0xFFF80FFF,
		(ahp->ah_thresh62[arrayMode] << 12) & 0x7F000);

	/*
	 * False detect backoff - suspected 32 MHz spur causes false
	 * detects in OFDM, causing Tx Hangs.  Decrease weak signal
	 * sensitivity for this card.
	 */
	falseDectectBackoff = NO_FALSE_DETECT_BACKOFF;
	if (ahp->ah_eeversion < AR_EEPROM_VER3_3) {
		/* XXX magic number */
		if (AH_PRIVATE(ah)->ah_subvendorid == 0x1022 &&
		    IS_CHAN_OFDM(chan))
			falseDectectBackoff += CB22_FALSE_DETECT_BACKOFF;
	} else {
		if (IS_SPUR_CHAN(chan))
			falseDectectBackoff += ahp->ah_falseDetectBackoff[arrayMode];
	}
	AR_PHY_BIS(ah, 73, 0xFFFFFF01, (falseDectectBackoff << 1) & 0xF7);

	if (chan->iqCalValid) {
		iCoff = chan->iCoff;
		qCoff = chan->qCoff;
	} else {
		iCoff = ahp->ah_iqCalI[is2GHz];
		qCoff = ahp->ah_iqCalQ[is2GHz];
	}

	/* write previous IQ results */
	OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
		AR_PHY_TIMING_CTRL4_IQCORR_Q_I_COFF, iCoff);
	OS_REG_RMW_FIELD(ah, AR_PHY_TIMING_CTRL4,
		AR_PHY_TIMING_CTRL4_IQCORR_Q_Q_COFF, qCoff);
	OS_REG_SET_BIT(ah, AR_PHY_TIMING_CTRL4,
		AR_PHY_TIMING_CTRL4_IQCORR_ENABLE);

	if (ahp->ah_eeversion >= AR_EEPROM_VER4_1) {
		if (!IS_CHAN_108G(chan) ||
		    ahp->ah_eeversion >= AR_EEPROM_VER5_0)
			OS_REG_RMW_FIELD(ah, AR_PHY_GAIN_2GHZ,
				AR_PHY_GAIN_2GHZ_RXTX_MARGIN, rxtxMargin);
	}
	if (ahp->ah_eeversion >= AR_EEPROM_VER5_1) {
		/* for now always disabled */
		OS_REG_WRITE(ah,  AR_PHY_HEAVY_CLIP_ENABLE,  0);
	}

	return AH_TRUE;
#undef AR_PHY_BIS
#undef NO_FALSE_DETECT_BACKOFF
#undef CB22_FALSE_DETECT_BACKOFF
}

/*
 * Delta slope coefficient computation.
 * Required for OFDM operation.
 */
void
ar5212SetDeltaSlope(struct ath_hal *ah, HAL_CHANNEL *chan)
{
#define COEF_SCALE_S 24
#define INIT_CLOCKMHZSCALED	0x64000000
	unsigned long coef_scaled, coef_exp, coef_man, ds_coef_exp, ds_coef_man;
	unsigned long clockMhzScaled = INIT_CLOCKMHZSCALED;

	if (IS_CHAN_TURBO(chan))
		clockMhzScaled *= 2;
	/* half and quarter rate can divide the scaled clock by 2 or 4 respectively */
	/* scale for selected channel bandwidth */ 
	if (IS_CHAN_HALF_RATE(chan)) {
		clockMhzScaled = clockMhzScaled >> 1;
	} else if (IS_CHAN_QUARTER_RATE(chan)) {
		clockMhzScaled = clockMhzScaled >> 2;
	} 

	/*
	 * ALGO -> coef = 1e8/fcarrier*fclock/40;
	 * scaled coef to provide precision for this floating calculation 
	 */
	coef_scaled = clockMhzScaled / chan->channel;

	/*
	 * ALGO -> coef_exp = 14-floor(log2(coef)); 
	 * floor(log2(x)) is the highest set bit position
	 */
	for (coef_exp = 31; coef_exp > 0; coef_exp--)
		if ((coef_scaled >> coef_exp) & 0x1)
			break;
	/* A coef_exp of 0 is a legal bit position but an unexpected coef_exp */
	HALASSERT(coef_exp);
	coef_exp = 14 - (coef_exp - COEF_SCALE_S);

	/*
	 * ALGO -> coef_man = floor(coef* 2^coef_exp+0.5);
	 * The coefficient is already shifted up for scaling
	 */
	coef_man = coef_scaled + (1 << (COEF_SCALE_S - coef_exp - 1));
	ds_coef_man = coef_man >> (COEF_SCALE_S - coef_exp);
	ds_coef_exp = coef_exp - 16;

	OS_REG_RMW_FIELD(ah, AR_PHY_TIMING3,
		AR_PHY_TIMING3_DSC_MAN, ds_coef_man);
	OS_REG_RMW_FIELD(ah, AR_PHY_TIMING3,
		AR_PHY_TIMING3_DSC_EXP, ds_coef_exp);
#undef INIT_CLOCKMHZSCALED
#undef COEF_SCALE_S
}

/*
 * Set a limit on the overall output power.  Used for dynamic
 * transmit power control and the like.
 *
 * NB: limit is in units of 0.5 dbM.
 */
HAL_BOOL
ar5212SetTxPowerLimit(struct ath_hal *ah, u_int32_t limit)
{
	u_int16_t dummyXpdGains[2];

	AH_PRIVATE(ah)->ah_powerLimit = AH_MIN(limit, MAX_RATE_POWER);
	return ar5212SetTransmitPower(ah, AH_PRIVATE(ah)->ah_curchan,
			dummyXpdGains);
}

/*
 * Set the transmit power in the baseband for the given
 * operating channel and mode.
 */
HAL_BOOL
ar5212SetTransmitPower(struct ath_hal *ah, HAL_CHANNEL_INTERNAL *chan,
	u_int16_t *rfXpdGain)
{
#define	POW_OFDM(_r, _s)	(((0 & 1)<< ((_s)+6)) | (((_r) & 0x3f) << (_s)))
#define	POW_CCK(_r, _s)		(((_r) & 0x3f) << (_s))
#define	N(a)			(sizeof (a) / sizeof (a[0]))
	static const u_int16_t tpcScaleReductionTable[5] =
		{ 0, 3, 6, 9, MAX_RATE_POWER };
	struct ath_hal_5212 *ahp = AH5212(ah);
	int16_t minPower, maxPower, tpcInDb, powerLimit;
	int i;

	OS_MEMZERO(ahp->ah_pcdacTable, ahp->ah_pcdacTableSize);
	OS_MEMZERO(ahp->ah_ratesArray, sizeof(ahp->ah_ratesArray));

	powerLimit = AH_MIN(MAX_RATE_POWER, AH_PRIVATE(ah)->ah_powerLimit);
	if (powerLimit >= MAX_RATE_POWER || powerLimit == 0)
		tpcInDb = tpcScaleReductionTable[AH_PRIVATE(ah)->ah_tpScale];
	else
		tpcInDb = 0;
	if (!ar5212SetRateTable(ah, (HAL_CHANNEL *) chan, tpcInDb, powerLimit,
		&minPower, &maxPower)) {
		HALDEBUG(ah, "%s: unable to set rate table\n", __func__);
		return AH_FALSE;
	}
	if (!ahp->ah_rfHal.setPowerTable(ah,
		&minPower, &maxPower, chan, rfXpdGain)) {
		HALDEBUG(ah, "%s: unable to set power table\n", __func__);
		return AH_FALSE;
	}

	/* 
	 * Adjust XR power/rate up by 2 dB to account for greater peak
	 * to avg ratio - except in newer avg power designs
	 */
	if (!IS_2413(ah))
		ahp->ah_ratesArray[15] += 4;
	/* 
	 * txPowerIndexOffset is set by the SetPowerTable() call -
	 *  adjust the rate table 
	 */
	for (i = 0; i < N(ahp->ah_ratesArray); i++) {
		ahp->ah_ratesArray[i] += ahp->ah_txPowerIndexOffset;
		if (ahp->ah_ratesArray[i] > 63) 
			ahp->ah_ratesArray[i] = 63;
	}

	if (ahp->ah_eepMap < 2) {
		/* 
		 * WAR to correct gain deltas for 5212 G operation -
		 * Removed with revised chipset
		 */
		if (AH_PRIVATE(ah)->ah_phyRev < AR_PHY_CHIP_ID_REV_2 &&
		    IS_CHAN_G(chan)) {
			u_int16_t cckOfdmPwrDelta;

			if (chan->channel == 2484) 
				cckOfdmPwrDelta = SCALE_OC_DELTA(
					ahp->ah_cckOfdmPwrDelta - 
					ahp->ah_scaledCh14FilterCckDelta);
			else 
				cckOfdmPwrDelta = SCALE_OC_DELTA(
					ahp->ah_cckOfdmPwrDelta);
			ar5212CorrectGainDelta(ah, cckOfdmPwrDelta);
		}
		/* 
		 * Finally, write the power values into the
		 * baseband power table
		 */
		for (i = 0; i < (PWR_TABLE_SIZE/2); i++) {
			OS_REG_WRITE(ah, AR_PHY_PCDAC_TX_POWER(i),
				 ((((ahp->ah_pcdacTable[2*i + 1] << 8) | 0xff) & 0xffff) << 16)
				| (((ahp->ah_pcdacTable[2*i]     << 8) | 0xff) & 0xffff)
			);
		}
	}

	/* Write the OFDM power per rate set */
	OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE1, 
		POW_OFDM(ahp->ah_ratesArray[3], 24)
	      | POW_OFDM(ahp->ah_ratesArray[2], 16)
	      | POW_OFDM(ahp->ah_ratesArray[1],  8)
	      | POW_OFDM(ahp->ah_ratesArray[0],  0)
	);
	OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE2, 
		POW_OFDM(ahp->ah_ratesArray[7], 24)
	      | POW_OFDM(ahp->ah_ratesArray[6], 16)
	      | POW_OFDM(ahp->ah_ratesArray[5],  8)
	      | POW_OFDM(ahp->ah_ratesArray[4],  0)
	);

	/* Write the CCK power per rate set */
	OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE3,
		POW_CCK(ahp->ah_ratesArray[10], 24)
	      | POW_CCK(ahp->ah_ratesArray[9],  16)
	      | POW_CCK(ahp->ah_ratesArray[15],  8)	/* XR target power */
	      | POW_CCK(ahp->ah_ratesArray[8],   0)
	);
	OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE4,
		POW_CCK(ahp->ah_ratesArray[14], 24)
	      | POW_CCK(ahp->ah_ratesArray[13], 16)
	      | POW_CCK(ahp->ah_ratesArray[12],  8)
	      | POW_CCK(ahp->ah_ratesArray[11],  0)
	);

	/*
	 * Set max power to 30 dBm and, optionally,
	 * enable TPC in tx descriptors.
	 */
	OS_REG_WRITE(ah, AR_PHY_POWER_TX_RATE_MAX, MAX_RATE_POWER |
		(ahp->ah_tpcEnabled ? AR_PHY_POWER_TX_RATE_MAX_TPC_ENABLE : 0));
	AH_PRIVATE(ah)->ah_maxPowerLevel = ahp->ah_ratesArray[0];

	return AH_TRUE;
#undef N
#undef POW_CCK
#undef POW_OFDM
}

/*
 * Sets the transmit power in the baseband for the given
 * operating channel and mode.
 */
static HAL_BOOL
ar5212SetRateTable(struct ath_hal *ah, HAL_CHANNEL *chan,
		   int16_t tpcScaleReduction, int16_t powerLimit,
                   int16_t *pMinPower, int16_t *pMaxPower)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	u_int16_t *rpow = ahp->ah_ratesArray;
	u_int16_t twiceMaxEdgePower = MAX_RATE_POWER;
	u_int16_t twiceMaxEdgePowerCck = MAX_RATE_POWER;
	u_int16_t twiceMaxRDPower = MAX_RATE_POWER;
	int i;
	u_int8_t cfgCtl;
	int8_t twiceAntennaGain, twiceAntennaReduction;
	RD_EDGES_POWER *rep;
	TRGT_POWER_INFO targetPowerOfdm, targetPowerCck;
	int16_t scaledPower, maxAvailPower = 0;

	twiceMaxRDPower = chan->maxRegTxPower * 2;
	*pMaxPower = -MAX_RATE_POWER;
	*pMinPower = MAX_RATE_POWER;

	/* Get conformance test limit maximum for this channel */
	cfgCtl = ath_hal_getctl(ah, chan);

	for (i = 0; i < ahp->ah_numCtls; i++) {
		u_int16_t twiceMinEdgePower;

		if (ahp->ah_ctl[i] == 0)
			continue;
		if (ahp->ah_ctl[i] == cfgCtl ||
		    cfgCtl == ((ahp->ah_ctl[i] & CTL_MODE_M) | SD_NO_CTL)) {
			rep = &ahp->ah_rdEdgesPower[i * NUM_EDGES];
			twiceMinEdgePower = ar5212GetMaxEdgePower(chan->channel, rep);
			if ((cfgCtl & ~CTL_MODE_M) == SD_NO_CTL) {
				/* Find the minimum of all CTL edge powers that apply to this channel */
				twiceMaxEdgePower = AH_MIN(twiceMaxEdgePower, twiceMinEdgePower);
			} else {
				twiceMaxEdgePower = twiceMinEdgePower;
				break;
			}
		}
	}

	if (IS_CHAN_G(chan)) {
		/* Check for a CCK CTL for 11G CCK powers */
		cfgCtl = (cfgCtl &~ ~CTL_MODE_M) | CTL_11B;
		for (i = 0; i < ahp->ah_numCtls; i++) {
			u_int16_t twiceMinEdgePowerCck;

			if (ahp->ah_ctl[i] == 0)
				continue;
			if (ahp->ah_ctl[i] == cfgCtl ||
			    cfgCtl == ((ahp->ah_ctl[i] & CTL_MODE_M) | SD_NO_CTL)) {
				rep = &ahp->ah_rdEdgesPower[i * NUM_EDGES];
				twiceMinEdgePowerCck = ar5212GetMaxEdgePower(chan->channel, rep);
				if ((cfgCtl & ~CTL_MODE_M) == SD_NO_CTL) {
					/* Find the minimum of all CTL edge powers that apply to this channel */
					twiceMaxEdgePowerCck = AH_MIN(twiceMaxEdgePowerCck, twiceMinEdgePowerCck);
				} else {
					twiceMaxEdgePowerCck = twiceMinEdgePowerCck;
					break;
				}
			}
		}
	} else {
		/* Set the 11B cck edge power to the one found before */
		twiceMaxEdgePowerCck = twiceMaxEdgePower;
	}

	/* Get Antenna Gain reduction */
	if (IS_CHAN_5GHZ(chan)) {
		twiceAntennaGain = ahp->ah_antennaGainMax[0];
	} else {
		twiceAntennaGain = ahp->ah_antennaGainMax[1];
	}
	twiceAntennaReduction =
		ath_hal_getantennareduction(ah, chan, twiceAntennaGain);

	if (IS_CHAN_OFDM(chan)) {
		/* Get final OFDM target powers */
		if (IS_CHAN_2GHZ(chan)) { 
			ar5212GetTargetPowers(ah, chan, ahp->ah_trgtPwr_11g,
				ahp->ah_numTargetPwr_11g, &targetPowerOfdm);
		} else {
			ar5212GetTargetPowers(ah, chan, ahp->ah_trgtPwr_11a,
				ahp->ah_numTargetPwr_11a, &targetPowerOfdm);
		}

		/* Get Maximum OFDM power */
		/* Minimum of target and edge powers */
		scaledPower = AH_MIN(twiceMaxEdgePower,
				twiceMaxRDPower - twiceAntennaReduction);

		/*
		 * If turbo is set, reduce power to keep power
		 * consumption under 2 Watts.  Note that we always do
		 * this unless specially configured.  Then we limit
		 * power only for non-AP operation.
		 */
		if (IS_CHAN_TURBO(chan)
#ifdef AH_ENABLE_AP_SUPPORT
		    && AH_PRIVATE(ah)->ah_opmode != HAL_M_HOSTAP
#endif
		) {
			/*
			 * If turbo is set, reduce power to keep power
			 * consumption under 2 Watts
			 */
			if (ahp->ah_eeversion >= AR_EEPROM_VER3_1)
				scaledPower = AH_MIN(scaledPower,
					ahp->ah_turbo2WMaxPower5);
			/*
			 * EEPROM version 4.0 added an additional
			 * constraint on 2.4GHz channels.
			 */
			if (ahp->ah_eeversion >= AR_EEPROM_VER4_0 &&
			    IS_CHAN_2GHZ(chan))
				scaledPower = AH_MIN(scaledPower,
					ahp->ah_turbo2WMaxPower2);
		}

		maxAvailPower = AH_MIN(scaledPower,
					targetPowerOfdm.twicePwr6_24);

		/* Reduce power by max regulatory domain allowed restrictions */
		scaledPower = maxAvailPower - (tpcScaleReduction * 2);
		scaledPower = (scaledPower < 0) ? 0 : scaledPower;
		scaledPower = AH_MIN(scaledPower, powerLimit);

		/* Set OFDM rates 9, 12, 18, 24 */
		rpow[0] = rpow[1] = rpow[2] = rpow[3] = rpow[4] = scaledPower;

		/* Set OFDM rates 36, 48, 54, XR */
		rpow[5] = AH_MIN(rpow[0], targetPowerOfdm.twicePwr36);
		rpow[6] = AH_MIN(rpow[0], targetPowerOfdm.twicePwr48);
		rpow[7] = AH_MIN(rpow[0], targetPowerOfdm.twicePwr54);

		if (ahp->ah_eeversion >= AR_EEPROM_VER4_0) {
			/* Setup XR target power from EEPROM */
			rpow[15] = AH_MIN(scaledPower, IS_CHAN_2GHZ(chan) ?
				ahp->ah_xrTargetPower2 : ahp->ah_xrTargetPower5);
		} else {
			/* XR uses 6mb power */
			rpow[15] = rpow[0];
		}

		*pMinPower = rpow[7];
		*pMaxPower = rpow[0];

		ahp->ah_ofdmTxPower = *pMaxPower;

		HALDEBUG(ah, "%s: MaxRD: %d TurboMax: %d MaxCTL: %d "
			"TPC_Reduction %d\n",
			__func__,
			twiceMaxRDPower, ahp->ah_turbo2WMaxPower5,
			twiceMaxEdgePower, tpcScaleReduction * 2);
	}

	if (IS_CHAN_CCK(chan) || IS_CHAN_G(chan)) {
		/* Get final CCK target powers */
		ar5212GetTargetPowers(ah, chan, ahp->ah_trgtPwr_11b,
			ahp->ah_numTargetPwr_11b, &targetPowerCck);

		/* Reduce power by max regulatory domain allowed restrictions */
		scaledPower = AH_MIN(twiceMaxEdgePowerCck,
			twiceMaxRDPower - twiceAntennaReduction);
		if (maxAvailPower < AH_MIN(scaledPower, targetPowerCck.twicePwr6_24))
			maxAvailPower = AH_MIN(scaledPower, targetPowerCck.twicePwr6_24);

		/* Reduce power by user selection */
		scaledPower = AH_MIN(scaledPower, targetPowerCck.twicePwr6_24) - (tpcScaleReduction * 2);
		scaledPower = (scaledPower < 0) ? 0 : scaledPower;
		scaledPower = AH_MIN(scaledPower, powerLimit);

		/* Set CCK rates 2L, 2S, 5.5L, 5.5S, 11L, 11S */
		rpow[8]  = AH_MIN(scaledPower, targetPowerCck.twicePwr6_24);
		rpow[9]  = AH_MIN(scaledPower, targetPowerCck.twicePwr36);
		rpow[10] = rpow[9];
		rpow[11] = AH_MIN(scaledPower, targetPowerCck.twicePwr48);
		rpow[12] = rpow[11];
		rpow[13] = AH_MIN(scaledPower, targetPowerCck.twicePwr54);
		rpow[14] = rpow[13];

		/* Set min/max power based off OFDM values or initialization */
		if (rpow[13] < *pMinPower)
		    *pMinPower = rpow[13];
		if (rpow[9] > *pMaxPower)
		    *pMaxPower = rpow[9];

	}
	ahp->ah_tx6PowerInHalfDbm = *pMaxPower;
	return AH_TRUE;
}

/*
 * Correct for the gain-delta between ofdm and cck mode target
 * powers. Write the results to the rate table and the power table.
 *
 *   Conventions :
 *   1. rpow[ii] is the integer value of 2*(desired power
 *    for the rate ii in dBm) to provide 0.5dB resolution. rate
 *    mapping is as following :
 *     [0..7]  --> ofdm 6, 9, .. 48, 54
 *     [8..14] --> cck 1L, 2L, 2S, .. 11L, 11S
 *     [15]    --> XR (all rates get the same power)
 *   2. powv[ii]  is the pcdac corresponding to ii/2 dBm.
 */
static void
ar5212CorrectGainDelta(struct ath_hal *ah, int twiceOfdmCckDelta)
{
#define	N(_a)	(sizeof(_a) / sizeof(_a[0]))
	struct ath_hal_5212 *ahp = AH5212(ah);
	int16_t ratesIndex[N(ahp->ah_ratesArray)];
	u_int16_t ii, jj, iter;
	int32_t cckIndex;
	int16_t gainDeltaAdjust = ahp->ah_cckOfdmGainDelta;

	/* make a local copy of desired powers as initial indices */
	OS_MEMCPY(ratesIndex, ahp->ah_ratesArray, sizeof(ratesIndex));

	/* fix only the CCK indices */
	for (ii = 8; ii < 15; ii++) {
		/* apply a gain_delta correction of -15 for CCK */
		ratesIndex[ii] -= gainDeltaAdjust;

		/* Now check for contention with all ofdm target powers */
		jj = 0;
		iter = 0;
		/* indicates not all ofdm rates checked forcontention yet */
		while (jj < 16) {
			if (ratesIndex[ii] < 0)
				ratesIndex[ii] = 0;
			if (jj == 8) {		/* skip CCK rates */
				jj = 15;
				continue;
			}
			if (ratesIndex[ii] == ahp->ah_ratesArray[jj]) {
				if (ahp->ah_ratesArray[jj] == 0)
					ratesIndex[ii]++;
				else if (iter > 50) {
					/*
					 * To avoid pathological case of of
					 * dm target powers 0 and 0.5dBm
					 */
					ratesIndex[ii]++;
				} else
					ratesIndex[ii]--;
				/* check with all rates again */
				jj = 0;
				iter++;
			} else
				jj++;
		}
		if (ratesIndex[ii] >= PWR_TABLE_SIZE)
			ratesIndex[ii] = PWR_TABLE_SIZE -1;
		cckIndex = ahp->ah_ratesArray[ii] - twiceOfdmCckDelta;
		if (cckIndex < 0)
			cckIndex = 0;

		/* 
		 * Validate that the indexes for the powv are not
		 * out of bounds. BUG 6694
		 */
		HALASSERT(cckIndex < PWR_TABLE_SIZE);
		HALASSERT(ratesIndex[ii] < PWR_TABLE_SIZE);
		ahp->ah_pcdacTable[ratesIndex[ii]] =
			ahp->ah_pcdacTable[cckIndex];
	}
	/* Override rate per power table with new values */
	for (ii = 8; ii < 15; ii++)
		ahp->ah_ratesArray[ii] = ratesIndex[ii];
#undef N
}

/*
 * Find the maximum conformance test limit for the given channel and CTL info
 */
static u_int16_t
ar5212GetMaxEdgePower(u_int16_t channel, RD_EDGES_POWER *pRdEdgesPower)
{
	/* temp array for holding edge channels */
	u_int16_t tempChannelList[NUM_EDGES];
	u_int16_t clo, chi, twiceMaxEdgePower;
	int i, numEdges;

	/* Get the edge power */
	for (i = 0; i < NUM_EDGES; i++) {
		if (pRdEdgesPower[i].rdEdge == 0)
			break;
		tempChannelList[i] = pRdEdgesPower[i].rdEdge;
	}
	numEdges = i;

	ar5212GetLowerUpperValues(channel, tempChannelList,
		numEdges, &clo, &chi);
	/* Get the index for the lower channel */
	for (i = 0; i < numEdges && clo != tempChannelList[i]; i++)
		;
	/* Is lower channel ever outside the rdEdge? */
	HALASSERT(i != numEdges);

	if ((clo == chi && clo == channel) || (pRdEdgesPower[i].flag)) {
		/* 
		 * If there's an exact channel match or an inband flag set
		 * on the lower channel use the given rdEdgePower 
		 */
		twiceMaxEdgePower = pRdEdgesPower[i].twice_rdEdgePower;
		HALASSERT(twiceMaxEdgePower > 0);
	} else
		twiceMaxEdgePower = MAX_RATE_POWER;
	return twiceMaxEdgePower;
}

/*
 * Returns interpolated or the scaled up interpolated value
 */
static u_int16_t
interpolate(u_int16_t target, u_int16_t srcLeft, u_int16_t srcRight,
	u_int16_t targetLeft, u_int16_t targetRight)
{
	u_int16_t rv;
	int16_t lRatio;

	/* to get an accurate ratio, always scale, if want to scale, then don't scale back down */
	if ((targetLeft * targetRight) == 0)
		return 0;

	if (srcRight != srcLeft) {
		/*
		 * Note the ratio always need to be scaled,
		 * since it will be a fraction.
		 */
		lRatio = (target - srcLeft) * EEP_SCALE / (srcRight - srcLeft);
		if (lRatio < 0) {
		    /* Return as Left target if value would be negative */
		    rv = targetLeft;
		} else if (lRatio > EEP_SCALE) {
		    /* Return as Right target if Ratio is greater than 100% (SCALE) */
		    rv = targetRight;
		} else {
			rv = (lRatio * targetRight + (EEP_SCALE - lRatio) *
					targetLeft) / EEP_SCALE;
		}
	} else {
		rv = targetLeft;
	}
	return rv;
}

/*
 * Return the four rates of target power for the given target power table 
 * channel, and number of channels
 */
static void
ar5212GetTargetPowers(struct ath_hal *ah, HAL_CHANNEL *chan,
	TRGT_POWER_INFO *powInfo,
	u_int16_t numChannels, TRGT_POWER_INFO *pNewPower)
{
	/* temp array for holding target power channels */
	u_int16_t tempChannelList[NUM_TEST_FREQUENCIES];
	u_int16_t clo, chi, ixlo, ixhi;
	int i;

	/* Copy the target powers into the temp channel list */
	for (i = 0; i < numChannels; i++)
		tempChannelList[i] = powInfo[i].testChannel;

	ar5212GetLowerUpperValues(chan->channel, tempChannelList,
		numChannels, &clo, &chi);

	/* Get the indices for the channel */
	ixlo = ixhi = 0;
	for (i = 0; i < numChannels; i++) {
		if (clo == tempChannelList[i]) {
			ixlo = i;
		}
		if (chi == tempChannelList[i]) {
			ixhi = i;
			break;
		}
	}

	/*
	 * Get the lower and upper channels, target powers,
	 * and interpolate between them.
	 */
	pNewPower->twicePwr6_24 = interpolate(chan->channel, clo, chi,
		powInfo[ixlo].twicePwr6_24, powInfo[ixhi].twicePwr6_24);
	pNewPower->twicePwr36 = interpolate(chan->channel, clo, chi,
		powInfo[ixlo].twicePwr36, powInfo[ixhi].twicePwr36);
	pNewPower->twicePwr48 = interpolate(chan->channel, clo, chi,
		powInfo[ixlo].twicePwr48, powInfo[ixhi].twicePwr48);
	pNewPower->twicePwr54 = interpolate(chan->channel, clo, chi,
		powInfo[ixlo].twicePwr54, powInfo[ixhi].twicePwr54);
}

/*
 * Search a list for a specified value v that is within
 * EEP_DELTA of the search values.  Return the closest
 * values in the list above and below the desired value.
 * EEP_DELTA is a factional value; everything is scaled
 * so only integer arithmetic is used.
 *
 * NB: the input list is assumed to be sorted in ascending order
 */
void
ar5212GetLowerUpperValues(u_int16_t v, u_int16_t *lp, u_int16_t listSize,
                          u_int16_t *vlo, u_int16_t *vhi)
{
	u_int32_t target = v * EEP_SCALE;
	u_int16_t *ep = lp+listSize;

	/*
	 * Check first and last elements for out-of-bounds conditions.
	 */
	if (target < (u_int32_t)(lp[0] * EEP_SCALE - EEP_DELTA)) {
		*vlo = *vhi = lp[0];
		return;
	}
	if (target > (u_int32_t)(ep[-1] * EEP_SCALE + EEP_DELTA)) {
		*vlo = *vhi = ep[-1];
		return;
	}

	/* look for value being near or between 2 values in list */
	for (; lp < ep; lp++) {
		/*
		 * If value is close to the current value of the list
		 * then target is not between values, it is one of the values
		 */
		if (abs(lp[0] * EEP_SCALE - target) < EEP_DELTA) {
			*vlo = *vhi = lp[0];
			return;
		}
		/*
		 * Look for value being between current value and next value
		 * if so return these 2 values
		 */
		if (target < (u_int32_t)(lp[1] * EEP_SCALE - EEP_DELTA)) {
			*vlo = lp[0];
			*vhi = lp[1];
			return;
		}
	}
}

static const GAIN_OPTIMIZATION_LADDER gainLadder = {
	9,					/* numStepsInLadder */
	4,					/* defaultStepNum */
	{ { {4, 1, 1, 1},  6, "FG8"},
	  { {4, 0, 1, 1},  4, "FG7"},
	  { {3, 1, 1, 1},  3, "FG6"},
	  { {4, 0, 0, 1},  1, "FG5"},
	  { {4, 1, 1, 0},  0, "FG4"},	/* noJack */
	  { {4, 0, 1, 0}, -2, "FG3"},	/* halfJack */
	  { {3, 1, 1, 0}, -3, "FG2"},	/* clip3 */
	  { {4, 0, 0, 0}, -4, "FG1"},	/* noJack */
	  { {2, 1, 1, 0}, -6, "FG0"} 	/* clip2 */
	}
};

const static GAIN_OPTIMIZATION_LADDER gainLadder5112 = {
	8,					/* numStepsInLadder */
	1,					/* defaultStepNum */
	{ { {3, 0,0,0, 0,0,0},   6, "FG7"},	/* most fixed gain */
	  { {2, 0,0,0, 0,0,0},   0, "FG6"},
	  { {1, 0,0,0, 0,0,0},  -3, "FG5"},
	  { {0, 0,0,0, 0,0,0},  -6, "FG4"},
	  { {0, 1,1,0, 0,0,0},  -8, "FG3"},
	  { {0, 1,1,0, 1,1,0}, -10, "FG2"},
	  { {0, 1,0,1, 1,1,0}, -13, "FG1"},
	  { {0, 1,0,1, 1,0,1}, -16, "FG0"},	/* least fixed gain */
	}
};

/*
 * Initialize the gain structure to good values
 */
void
ar5212InitializeGainValues(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	GAIN_VALUES *gv = &ahp->ah_gainValues;

	/* initialize gain optimization values */
	if (IS_5112(ah)) {
		gv->currStepNum = gainLadder5112.defaultStepNum;
		gv->currStep =
			&gainLadder5112.optStep[gainLadder5112.defaultStepNum];
		gv->active = AH_TRUE;
		gv->loTrig = 20;
		gv->hiTrig = 85;
	} else {
		gv->currStepNum = gainLadder.defaultStepNum;
		gv->currStep = &gainLadder.optStep[gainLadder.defaultStepNum];
		gv->active = AH_TRUE;
		gv->loTrig = 20;
		gv->hiTrig = 35;
	}
}

static HAL_BOOL
ar5212InvalidGainReadback(struct ath_hal *ah, GAIN_VALUES *gv)
{
	u_int32_t gStep, g, mixOvr;
	u_int32_t L1, L2, L3, L4;

	if (IS_5112(ah)) {
		mixOvr = ar5212GetRfField(ar5212GetRfBank(ah, 7), 1, 36, 0);
		L1 = 0;
		L2 = 107;
		L3 = 0;
		L4 = 107;
		if (mixOvr == 1) {
			L2 = 83;
			L4 = 83;
			gv->hiTrig = 55;
		}
	} else {
		gStep = ar5212GetRfField(ar5212GetRfBank(ah, 7), 6, 37, 0);

		L1 = 0;
		L2 = (gStep == 0x3f) ? 50 : gStep + 4;
		L3 = (gStep != 0x3f) ? 0x40 : L1;
		L4 = L3 + 50;

		gv->loTrig = L1 + (gStep == 0x3f ? DYN_ADJ_LO_MARGIN : 0);
		/* never adjust if != 0x3f */
		gv->hiTrig = L4 - (gStep == 0x3f ? DYN_ADJ_UP_MARGIN : -5);
	}
	g = gv->currGain;

	return !((g >= L1 && g<= L2) || (g >= L3 && g <= L4));
}

/*
 * Enable the probe gain check on the next packet
 */
static void
ar5212RequestRfgain(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	u_int32_t probePowerIndex;

	/* Enable the gain readback probe */
	probePowerIndex = ahp->ah_ofdmTxPower + ahp->ah_txPowerIndexOffset;
	OS_REG_WRITE(ah, AR_PHY_PAPD_PROBE,
		  SM(probePowerIndex, AR_PHY_PAPD_PROBE_POWERTX)
		| AR_PHY_PAPD_PROBE_NEXT_TX);

	ahp->ah_rfgainState = HAL_RFGAIN_READ_REQUESTED;
}

/*
 * Exported call to check for a recent gain reading and return
 * the current state of the thermal calibration gain engine.
 */
HAL_RFGAIN
ar5212GetRfgain(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	GAIN_VALUES *gv = &ahp->ah_gainValues;
	u_int32_t rddata, probeType;

	if (!gv->active)
		return HAL_RFGAIN_INACTIVE;

	if (ahp->ah_rfgainState == HAL_RFGAIN_READ_REQUESTED) {
		/* Caller had asked to setup a new reading. Check it. */
		rddata = OS_REG_READ(ah, AR_PHY_PAPD_PROBE);

		if ((rddata & AR_PHY_PAPD_PROBE_NEXT_TX) == 0) {
			/* bit got cleared, we have a new reading. */
			gv->currGain = rddata >> AR_PHY_PAPD_PROBE_GAINF_S;
			probeType = MS(rddata, AR_PHY_PAPD_PROBE_TYPE);
			if (probeType == AR_PHY_PAPD_PROBE_TYPE_CCK) {
				HALASSERT(IS_5112(ah));
				if (AH_PRIVATE(ah)->ah_phyRev >= AR_PHY_CHIP_ID_REV_2)
					gv->currGain += ahp->ah_cckOfdmGainDelta;
				else
					gv->currGain += PHY_PROBE_CCK_CORRECTION;
			}
			if (IS_5112(ah)) {
				ar5212GetGainFCorrection(ah);
				if (gv->currGain >= gv->gainFCorrection)
					gv->currGain -= gv->gainFCorrection;
				else
					gv->currGain = 0;
			}
			/* inactive by default */
			ahp->ah_rfgainState = HAL_RFGAIN_INACTIVE;

			if (!ar5212InvalidGainReadback(ah, gv) &&
			    ar5212IsGainAdjustNeeded(ah, gv) &&
			    ar5212AdjustGain(ah, gv) > 0) {
				/*
				 * Change needed. Copy ladder info
				 * into eeprom info.
				 */
				ahp->ah_rfgainState = HAL_RFGAIN_NEED_CHANGE;
				/* Request IQ recalibration for temperature chang */
				ahp->ah_bIQCalibration = IQ_CAL_INACTIVE;
			}
		}
	}
	return ahp->ah_rfgainState;
}

/*
 * Check to see if our readback gain level sits within the linear
 * region of our current variable attenuation window
 */
static HAL_BOOL
ar5212IsGainAdjustNeeded(struct ath_hal *ah, const GAIN_VALUES *gv)
{
	return (gv->currGain <= gv->loTrig || gv->currGain >= gv->hiTrig);
}

/*
 * Move the rabbit ears in the correct direction.
 */
static int32_t 
ar5212AdjustGain(struct ath_hal *ah, GAIN_VALUES *gv)
{
	const GAIN_OPTIMIZATION_LADDER *gl;

	if (IS_5112(ah))
		gl = &gainLadder5112;
	else
		gl = &gainLadder;
	gv->currStep = &gl->optStep[gv->currStepNum];
	if (gv->currGain >= gv->hiTrig) {
		if (gv->currStepNum == 0) {
			HALDEBUG(ah, "%s: Max gain limit.\n", __func__);
			return -1;
		}
		HALDEBUG(ah, "%s: Adding gain: currG=%d [%s] --> ",
			__func__, gv->currGain, gv->currStep->stepName);
		gv->targetGain = gv->currGain;
		while (gv->targetGain >= gv->hiTrig && gv->currStepNum > 0) {
			gv->targetGain -= 2 * (gl->optStep[--(gv->currStepNum)].stepGain -
				gv->currStep->stepGain);
			gv->currStep = &gl->optStep[gv->currStepNum];
		}
		HALDEBUG(ah, "targG=%d [%s]\n",
			gv->targetGain, gv->currStep->stepName);
		return 1;
	}
	if (gv->currGain <= gv->loTrig) {
		if (gv->currStepNum == gl->numStepsInLadder-1) {
			HALDEBUG(ah, "%s: Min gain limit.\n", __func__);
			return -2;
		}
		HALDEBUG(ah, "%s: Deducting gain: currG=%d [%s] --> ",
			__func__, gv->currGain, gv->currStep->stepName);
		gv->targetGain = gv->currGain;
		while (gv->targetGain <= gv->loTrig &&
		      gv->currStepNum < (gl->numStepsInLadder - 1)) {
			gv->targetGain -= 2 *
				(gl->optStep[++(gv->currStepNum)].stepGain - gv->currStep->stepGain);
			gv->currStep = &gl->optStep[gv->currStepNum];
		}
		HALDEBUG(ah, "targG=%d [%s]\n",
			gv->targetGain, gv->currStep->stepName);
		return 2;
	}
	return 0;		/* caller didn't call needAdjGain first */
}

/*
 * Read rf register to determine if gainF needs correction
 */
static void
ar5212GetGainFCorrection(struct ath_hal *ah)
{
	struct ath_hal_5212 *ahp = AH5212(ah);
	GAIN_VALUES *gv = &ahp->ah_gainValues;

	HALASSERT(IS_RADX112_REV2(ah));

	gv->gainFCorrection = 0;
	if (ar5212GetRfField(ar5212GetRfBank(ah, 7), 1, 36, 0) == 1) {
		u_int32_t mixGain = gv->currStep->paramVal[0];
		u_int32_t gainStep =
			ar5212GetRfField(ar5212GetRfBank(ah, 7), 4, 32, 0);
		switch (mixGain) {
		case 0 :
			gv->gainFCorrection = 0;
			break;
		case 1 :
			gv->gainFCorrection = gainStep;
			break;
		case 2 :
			gv->gainFCorrection = 2 * gainStep - 5;
			break;
		case 3 :
			gv->gainFCorrection = 2 * gainStep;
			break;
		}
	}
}

/*
 * Perform analog "swizzling" of parameters into their location
 */
void
ar5212ModifyRfBuffer(u_int32_t *rfBuf, u_int32_t reg32, u_int32_t numBits,
                     u_int32_t firstBit, u_int32_t column)
{
	u_int32_t tmp32, mask, arrayEntry, lastBit;
	int32_t bitPosition, bitsLeft;

	HALASSERT(column <= 3);
	HALASSERT(numBits <= 32);
	HALASSERT(firstBit + numBits <= MAX_ANALOG_START);

	tmp32 = ath_hal_reverseBits(reg32, numBits);
	arrayEntry = (firstBit - 1) / 8;
	bitPosition = (firstBit - 1) % 8;
	bitsLeft = numBits;
	while (bitsLeft > 0) {
		lastBit = (bitPosition + bitsLeft > 8) ?
			8 : bitPosition + bitsLeft;
		mask = (((1 << lastBit) - 1) ^ ((1 << bitPosition) - 1)) <<
			(column * 8);
		rfBuf[arrayEntry] &= ~mask;
		rfBuf[arrayEntry] |= ((tmp32 << bitPosition) <<
			(column * 8)) & mask;
		bitsLeft -= 8 - bitPosition;
		tmp32 = tmp32 >> (8 - bitPosition);
		bitPosition = 0;
		arrayEntry++;
	}
}

/*
 * Sets the rate to duration values in MAC - used for multi-
 * rate retry.
 * The rate duration table needs to cover all valid rate codes;
 * the XR table covers all ofdm and xr rates, while the 11b table
 * covers all cck rates => all valid rates get covered between
 * these two mode's ratetables!
 * But if we're turbo, the ofdm phy is replaced by the turbo phy
 * and xr or cck is not valid with turbo => all rates get covered
 * by the turbo ratetable only
 */
void
ar5212SetRateDurationTable(struct ath_hal *ah, HAL_CHANNEL *chan)
{
#define	WLAN_CTRL_FRAME_SIZE	(2+2+6+4)	/* ACK+FCS */
	const HAL_RATE_TABLE *rt;
	int i;

	if (IS_CHAN_HALF_RATE(chan)) {
		rt = ar5212GetRateTable(ah, HAL_MODE_11A_HALF_RATE);
	} else if (IS_CHAN_QUARTER_RATE(chan)) {
		rt = ar5212GetRateTable(ah, HAL_MODE_11A_QUARTER_RATE);
	} else {
		rt = ar5212GetRateTable(ah,
			IS_CHAN_TURBO(chan) ? HAL_MODE_TURBO : HAL_MODE_XR);
	}

	for (i = 0; i < rt->rateCount; ++i)
		OS_REG_WRITE(ah,
			AR_RATE_DURATION(rt->info[i].rateCode),
			ath_hal_computetxtime(ah, rt,
				WLAN_CTRL_FRAME_SIZE,
				rt->info[i].controlRate, AH_FALSE));
	if (!IS_CHAN_TURBO(chan)) {
		rt = ar5212GetRateTable(ah, HAL_MODE_11B);
		for (i = 0; i < rt->rateCount; ++i) {
			u_int32_t reg = AR_RATE_DURATION(rt->info[i].rateCode);
			OS_REG_WRITE(ah, reg,
				ath_hal_computetxtime(ah, rt,
					WLAN_CTRL_FRAME_SIZE,
					rt->info[i].controlRate, AH_FALSE));
			/* cck rates have short preamble option also */
			if (rt->info[i].shortPreamble) {
				reg += rt->info[i].shortPreamble << 2;
				OS_REG_WRITE(ah, reg,
					ath_hal_computetxtime(ah, rt,
						WLAN_CTRL_FRAME_SIZE,
						rt->info[i].controlRate,
						AH_TRUE));
			}
		}
	}
#undef WLAN_CTRL_FRAME_SIZE
}

/* Adjust various register settings based on half/quarter rate clock setting.
 * This includes: +USEC, TX/RX latency, 
 *                + IFS params: slot, eifs, misc etc.
 */
void 
ar5212SetIFSTiming(struct ath_hal *ah, HAL_CHANNEL *chan)
{
	u_int32_t txLat, rxLat, usec, slot, refClock, eifs, init_usec;

	refClock = OS_REG_READ(ah, AR_USEC) & AR_USEC_USEC32;
	if (IS_CHAN_HALF_RATE(chan)) {
		slot = IFS_SLOT_HALF_RATE;
		rxLat = RX_NON_FULL_RATE_LATENCY << AR5212_USEC_RX_LAT_S;
		txLat = TX_HALF_RATE_LATENCY << AR5212_USEC_TX_LAT_S;
		usec = HALF_RATE_USEC;
		eifs = IFS_EIFS_HALF_RATE;
		init_usec = INIT_USEC >> 1;
	} else { /* quarter rate */
		slot = IFS_SLOT_QUARTER_RATE;
		rxLat = RX_NON_FULL_RATE_LATENCY << AR5212_USEC_RX_LAT_S;
		txLat = TX_QUARTER_RATE_LATENCY << AR5212_USEC_TX_LAT_S;
		usec = QUARTER_RATE_USEC;
		eifs = IFS_EIFS_QUARTER_RATE;
		init_usec = INIT_USEC >> 2;
	}

	OS_REG_WRITE(ah, AR_USEC, (usec | refClock | txLat | rxLat));
	OS_REG_WRITE(ah, AR_D_GBL_IFS_SLOT, slot);
	OS_REG_WRITE(ah, AR_D_GBL_IFS_EIFS, eifs);
	OS_REG_RMW_FIELD(ah, AR_D_GBL_IFS_MISC,
				AR_D_GBL_IFS_MISC_USEC_DURATION, init_usec);
	return;
}

#endif /* AH_SUPPORT_AR5212 */
