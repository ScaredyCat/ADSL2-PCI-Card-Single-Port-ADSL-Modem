/*
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * Copyright (c) 2002-2005 Atheros Communications, Inc.
 * All rights reserved.
 *
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/hal/main/ar5416/ar5416_attach.c#41 $
 */
#include "opt_ah.h"

#ifdef AH_SUPPORT_AR5416

#if !defined(AH_SUPPORT_2133) && !defined(AH_SUPPORT_2122) && !defined(AH_SUPPORT_5133) && !defined(AH_SUPPORT_5122)
#error "No 5416 RF support defined"
#endif

#include "ah.h"
#include "ah_internal.h"
#include "ah_devid.h"

#include "ar5416/ar5416.h"
#include "ar5416/ar5416reg.h"
#include "ar5416/ar5416phy.h"

static HAL_BOOL ar5416GetChipPowerLimits(struct ath_hal *ah,
        HAL_CHANNEL *chans, u_int32_t nchans);

static inline void ar5416AniSetup(struct ath_hal *ah);
static inline int ar5416GetRadioRev(struct ath_hal *ah);
static inline HAL_STATUS ar5416RfAttach(struct ath_hal *ah);
static inline HAL_STATUS ar5416InitMacAddr(struct ath_hal *ah);
static inline HAL_STATUS ar5416HwAttach(struct ath_hal *ah);
static inline void ar5416HwDetach(struct ath_hal *ah);

static const struct ath_hal_private ar5416hal_10 = {{
    .ah_magic           = AR5416_MAGIC,
    .ah_abi             = HAL_ABI_VERSION,
    .ah_countryCode         = CTRY_DEFAULT,

    .ah_getRateTable        = ar5416GetRateTable,
    .ah_detach          = ar5416Detach,

    /* Reset Functions */
    .ah_reset           = ar5416Reset,
    .ah_phyDisable          = ar5416PhyDisable,
    .ah_setPCUConfig        = ar5416SetPCUConfig,
    .ah_perCalibration      = ar5416PerCalibration,
    .ah_setTxPowerLimit     = ar5416SetTxPowerLimit,

    /* AR/Radar Functions */
    .ah_arEnable            = ar5416ArEnable,
    .ah_arDisable           = ar5416ArDisable,
    .ah_arReset         = ar5416ResetAR,

    /* Transmit functions */
    .ah_updateTxTrigLevel       = ar5416UpdateTxTrigLevel,
    .ah_setupTxQueue        = ar5416SetupTxQueue,
    .ah_setTxQueueProps             = ar5416SetTxQueueProps,
    .ah_getTxQueueProps             = ar5416GetTxQueueProps,
    .ah_releaseTxQueue      = ar5416ReleaseTxQueue,
    .ah_resetTxQueue        = ar5416ResetTxQueue,
    .ah_getTxDP         = ar5416GetTxDP,
    .ah_setTxDP         = ar5416SetTxDP,
    .ah_numTxPending        = ar5416NumTxPending,
    .ah_startTxDma          = ar5416StartTxDma,
    .ah_stopTxDma           = ar5416StopTxDma,
    .ah_abortTxDma          = ar5416AbortTxDma,
    .ah_updateCTSForBursting    = ar5416UpdateCTSForBursting_10,
    .ah_setupTxDesc         = ar5416SetupTxDesc_10,
    .ah_setupXTxDesc        = ar5416SetupXTxDesc_10,
    .ah_fillTxDesc          = ar5416FillTxDesc_10,
    .ah_procTxDesc          = ar5416ProcTxDesc_10,
    .ah_getTxIntrQueue      = ar5416GetTxIntrQueue,
    .ah_setGlobalTxTimeout  = ar5416SetGlobalTxTimeout,
    .ah_getGlobalTxTimeout  = ar5416GetGlobalTxTimeout,

    /* RX Functions */
    .ah_getRxDP         = ar5416GetRxDP,
    .ah_setRxDP         = ar5416SetRxDP,
    .ah_enableReceive       = ar5416EnableReceive,
    .ah_stopDmaReceive      = ar5416StopDmaReceive,
    .ah_startPcuReceive     = ar5416StartPcuReceive,
    .ah_stopPcuReceive      = ar5416StopPcuReceive,
    .ah_abortPcuReceive     = ar5416AbortPcuReceive,
    .ah_setMulticastFilter      = ar5416SetMulticastFilter,
    .ah_setMulticastFilterIndex = ar5416SetMulticastFilterIndex,
    .ah_clrMulticastFilterIndex = ar5416ClrMulticastFilterIndex,
    .ah_getRxFilter         = ar5416GetRxFilter,
    .ah_setRxFilter         = ar5416SetRxFilter,
    .ah_setupRxDesc         = ar5416SetupRxDesc_10,
    .ah_procRxDescFast      = ar5416ProcRxDescFast_10,
    .ah_rxMonitor           = ar5416AniArPoll,
    .ah_procMibEvent        = ar5416ProcessMibIntr,

    /* Misc Functions */
    .ah_getCapability       = ar5416GetCapability,
    .ah_setCapability       = ar5416SetCapability,
    .ah_getDiagState        = ar5416GetDiagState,
    .ah_getMacAddress       = ar5416GetMacAddress,
    .ah_setMacAddress       = ar5416SetMacAddress,
    .ah_getBssIdMask        = ar5416GetBssIdMask,
    .ah_setBssIdMask        = ar5416SetBssIdMask,
    .ah_setRegulatoryDomain     = ar5416SetRegulatoryDomain,
    .ah_setLedState         = ar5416SetLedState,
    .ah_writeAssocid        = ar5416WriteAssocid,
    .ah_gpioCfgInput        = ar5416GpioCfgInput,
    .ah_gpioCfgOutput       = ar5416GpioCfgOutput,
    .ah_gpioGet         = ar5416GpioGet,
    .ah_gpioSet         = ar5416GpioSet,
    .ah_gpioSetIntr         = ar5416GpioSetIntr,
    .ah_getTsf32            = ar5416GetTsf32,
    .ah_getTsf64            = ar5416GetTsf64,
    .ah_resetTsf            = ar5416ResetTsf,
    .ah_detectCardPresent       = ar5416DetectCardPresent,
    .ah_updateMibCounters       = ar5416UpdateMibCounters,
    .ah_getDefAntenna       = ar5416GetDefAntenna,
    .ah_setDefAntenna       = ar5416SetDefAntenna,
    .ah_setSlotTime         = ar5416SetSlotTime,
    .ah_getSlotTime         = ar5416GetSlotTime,
    .ah_setAckTimeout       = ar5416SetAckTimeout,
    .ah_getAckTimeout       = ar5416GetAckTimeout,
    .ah_setCTSTimeout       = ar5416SetCTSTimeout,
    .ah_getCTSTimeout       = ar5416GetCTSTimeout,
    .ah_setCoverageClass    = ar5416SetCoverageClass,
    .ah_getDescInfo         = ar5416GetDescInfo,

    /* Key Cache Functions */
    .ah_getKeyCacheSize     = ar5416GetKeyCacheSize,
    .ah_resetKeyCacheEntry      = ar5416ResetKeyCacheEntry,
    .ah_isKeyCacheEntryValid    = ar5416IsKeyCacheEntryValid,
    .ah_setKeyCacheEntry        = ar5416SetKeyCacheEntry,
    .ah_setKeyCacheEntryMac     = ar5416SetKeyCacheEntryMac,

    /* Power Management Functions */
    .ah_setPowerMode        = ar5416SetPowerMode,
    .ah_getPowerMode        = ar5416GetPowerMode,

    /* Beacon Functions */
    .ah_beaconInit          = ar5416BeaconInit,
    .ah_setStationBeaconTimers  = ar5416SetStaBeaconTimers,
    .ah_resetStationBeaconTimers    = ar5416ResetStaBeaconTimers,
    .ah_waitForBeaconDone       = ar5416WaitForBeaconDone,

    /* Interrupt Functions */
    .ah_isInterruptPending      = ar5416IsInterruptPending,
    .ah_getPendingInterrupts    = ar5416GetPendingInterrupts,
    .ah_getInterrupts       = ar5416GetInterrupts,
    .ah_setInterrupts       = ar5416SetInterrupts,

    /* 11n Functions */
    .ah_reset11n            = ar5416Reset11n,
    .ah_set11nTxDesc        = ar5416Set11nTxDesc_10,
    .ah_set11nRateScenario  = ar5416Set11nRateScenario_10,
    .ah_set11nAggrFirst     = ar5416Set11nAggrFirst_10,
    .ah_set11nAggrMiddle    = ar5416Set11nAggrMiddle_10,
    .ah_set11nAggrLast      = ar5416Set11nAggrLast_10,
    .ah_clr11nAggr          = ar5416Clr11nAggr_10,
    .ah_set11nBurstDuration = ar5416Set11nBurstDuration_10,
    .ah_get11nExtBusy       = ar5416Get11nExtBusy,
    .ah_set11nMac2040       = ar5416Set11nMac2040,
    .ah_get11nRxClear       = ar5416Get11nRxClear,
    .ah_set11nRxClear       = ar5416Set11nRxClear,
    .ah_get11nHwPlatform    = ar5416Get11nHwPlatform,
    .ah_getCycleCounts      = ar5416GetCycleCounts,
    .ah_dmaRegDump          = ar5416DmaRegDump,
#ifdef ATH_FORCE_PPM
    .ah_ppmGetRssiDump      = ar5416PpmGetRssiDump,
    .ah_ppmArmTrigger       = ar5416PpmArmTrigger,
    .ah_ppmGetTrigger       = ar5416PpmGetTrigger,
    .ah_ppmForce            = ar5416PpmForce,
    .ah_ppmUnForce          = ar5416PpmUnForce,
    .ah_ppmGetReg           = ar5416PpmGetReg,
#endif
},

    .ah_getChannelEdges     = ar5416GetChannelEdges,
    .ah_getWirelessModes    = ar5416GetWirelessModes,
    .ah_gpioCfgOutput       = ar5416GpioCfgOutput,
    .ah_gpioCfgInput        = ar5416GpioCfgInput,
    .ah_gpioGet             = ar5416GpioGet,
    .ah_gpioSet             = ar5416GpioSet,
    .ah_gpioSetIntr         = ar5416GpioSetIntr,
    .ah_getChipPowerLimits  = ar5416GetChipPowerLimits,
};


/*
 * Attach for an AR5416 part.
 */
struct ath_hal *
ar5416Attach(u_int16_t devid, HAL_SOFTC sc,
    HAL_BUS_TAG st, HAL_BUS_HANDLE sh, u_int32_t flags, HAL_STATUS *status)
{
    struct ath_hal_5416 *ahp;
    struct ath_hal *ah;
    u_int32_t val;
    HAL_STATUS ecode;

    HALDEBUG(AH_NULL, "%s: sc %p st %u sh %p\n", __func__, sc, st, (void*) sh);

    /* NB: memory is returned zero'd */
    ahp = ar5416NewState(devid, sc, st, sh, flags, status);
    if (ahp == AH_NULL)
        return AH_NULL;
    ah = &ahp->ah_priv.h;

    if (!ar5416SetResetReg(ah, HAL_RESET_POWER_ON, AH_NULL)) {  /* reset chip */
        HALDEBUG(ah, "%s: couldn't reset chip\n", __func__);
        ecode = HAL_EIO;
        goto bad;
    }

    if (!ar5416SetPowerMode(ah, HAL_PM_AWAKE, AH_TRUE)) {
        HALDEBUG(ah, "%s: couldn't wakeup chip\n", __func__);
        ecode = HAL_EIO;
        goto bad;
    }

    /* Read Revisions from Chips */
    val = OS_REG_READ(ah, AR_SREV) & AR_SREV_ID;
    AH_PRIVATE(ah)->ah_macVersion = MS(val, AR_SREV_VERSION);
    AH_PRIVATE(ah)->ah_macRev = val & AR_SREV_REVISION;

    /* add mac revision check when needed */
    if ((AH_PRIVATE(ah)->ah_macVersion != AR_SREV_VERSION_OWL_PCI) &&
        (AH_PRIVATE(ah)->ah_macVersion != AR_SREV_VERSION_OWL_PCIE)) {
        HALDEBUG(ah, "%s: Mac Chip Rev 0x%02x.%x is not supported by "
            "this driver\n", __func__,
            AH_PRIVATE(ah)->ah_macVersion,
            AH_PRIVATE(ah)->ah_macRev);
        ecode = HAL_ENOTSUPP;
        goto bad;
    }

    AH_PRIVATE(ah)->ah_phyRev = OS_REG_READ(ah, AR_PHY_CHIP_ID);

    /* If its a Owl 2.0 chip then change the hal structure to
      point to the Owl 2.0 ar5416_hal_20 structure */

    if (AH_PRIVATE(ah)->ah_macRev >= AR_SREV_REVISION_OWL_20) {
        ah->ah_set11nTxDesc          = ar5416Set11nTxDesc_20;
        ah->ah_set11nRateScenario  = ar5416Set11nRateScenario_20;
        ah->ah_set11nAggrFirst     = ar5416Set11nAggrFirst_20;
        ah->ah_set11nAggrMiddle    = ar5416Set11nAggrMiddle_20;
        ah->ah_set11nAggrLast      = ar5416Set11nAggrLast_20;
        ah->ah_clr11nAggr          = ar5416Clr11nAggr_20;
        ah->ah_set11nBurstDuration = ar5416Set11nBurstDuration_20;
        ah->ah_setupRxDesc         = ar5416SetupRxDesc_20;
        ah->ah_procRxDescFast      = ar5416ProcRxDescFast_20;
        ah->ah_updateCTSForBursting= ar5416UpdateCTSForBursting_20;
        ah->ah_setupTxDesc         = ar5416SetupTxDesc_20;
        ah->ah_setupXTxDesc        = ar5416SetupXTxDesc_20;
        ah->ah_fillTxDesc          = ar5416FillTxDesc_20;
        ah->ah_procTxDesc          = ar5416ProcTxDesc_20;

        HALDEBUG(ah, "%s: This Mac Chip Rev 0x%02x.%x is \n", __func__,
            AH_PRIVATE(ah)->ah_macVersion,
            AH_PRIVATE(ah)->ah_macRev);
    }

    HALDEBUG(ah, "%s: This Mac Chip Rev 0x%02x.%x is \n", __func__,
        AH_PRIVATE(ah)->ah_macVersion,
        AH_PRIVATE(ah)->ah_macRev);

    ecode = ar5416HwAttach(ah);
    if (ecode != HAL_OK)
        goto bad;

    /*
     * Got everything we need now to setup the capabilities.
     */
    if (!ar5416FillCapabilityInfo(ah)) {
        HALDEBUG(ah, "%s:failed ar5416FillCapabilityInfo\n", __func__);
        ecode = HAL_EEREAD;
        goto bad;
    }

    ecode = ar5416InitMacAddr(ah);
    if (ecode != HAL_OK) {
        HALDEBUG(ah, "%s: failed initializing mac address\n", __func__);
        goto bad;
    }

#if 0
    /* Setup 5416 Radar/AR structures */
    ecode = ar5416RadarAttach(ah);
    if (ecode != HAL_OK)
        goto bad;
#endif

    HALDEBUG(ah, "%s: return ah %p\n", __func__, ah);

    if (IS_5416_EMU(ah))
        ath_hal_printf(ah, "Changelist :%d Date: %x\n", OS_REG_READ(ah, 0x7c),
                       OS_REG_READ(ah, 0x70));

    return ah;

bad:
    if (ahp)
        ar5416Detach((struct ath_hal *) ahp);
    if (status)
        *status = ecode;
    return AH_NULL;
}

void
ar5416Detach(struct ath_hal *ah)
{
    HALDEBUG(ah, "%s: ah %p\n", __func__, ah);

    HALASSERT(ah != AH_NULL);
    HALASSERT(ah->ah_magic == AR5416_MAGIC);

    ar5416HwDetach(ah);

    ar5416SetPowerMode(ah, HAL_PM_FULL_SLEEP, AH_TRUE);

    ath_hal_free(ah);
}

extern int
ar5416Get11nHwPlatform(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    return ahp->ah_hwp;
}

struct ath_hal_5416 *
ar5416NewState(u_int16_t devid, HAL_SOFTC sc,
    HAL_BUS_TAG st, HAL_BUS_HANDLE sh, u_int32_t flags, HAL_STATUS *status)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    static const u_int8_t defbssidmask[IEEE80211_ADDR_LEN] =
        { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    struct ath_hal_5416 *ahp;
    struct ath_hal *ah;

    /* NB: memory is returned zero'd */
    ahp = ath_hal_malloc(sizeof (struct ath_hal_5416));
    if (ahp == AH_NULL) {
        HALDEBUG(AH_NULL, "%s: cannot allocate memory for "
            "state block\n", __func__);
        *status = HAL_ENOMEM;
        return AH_NULL;
    }
    ah = &ahp->ah_priv.h;
    /* set initial values */

    /* Attach Owl 1.0 structure as default hal structure */
    OS_MEMCPY(&ahp->ah_priv, &ar5416hal_10, sizeof(struct ath_hal_private));

    ah->ah_sc = sc;
    ah->ah_st = st;
    ah->ah_sh = sh;

    AH_PRIVATE(ah)->ah_devid = devid;
    AH_PRIVATE(ah)->ah_subvendorid = 0; /* XXX */
    AH_PRIVATE(ah)->ah_flags = flags;

    if (ar5416EepDataInFlash(ah)) {
        ahp->ah_priv.ah_eepromRead  = ar5416FlashRead;
#ifdef AH_SUPPORT_WRITE_EEPROM
        ahp->ah_priv.ah_eepromWrite = ar5416FlashWrite;
#endif
    } else {
        ahp->ah_priv.ah_eepromRead  = ar5416EepromRead;
#ifdef AH_SUPPORT_WRITE_EEPROM
        ahp->ah_priv.ah_eepromWrite = ar5416EepromWrite;
#endif
    }

    AH_PRIVATE(ah)->ah_powerLimit = MAX_RATE_POWER;
    AH_PRIVATE(ah)->ah_tpScale = HAL_TP_SCALE_MAX;  /* no scaling */

    ahp->ah_atimWindow = 0;         /* [0..1000] */
    ahp->ah_diversityControl = HAL_ANT_VARIABLE;
    ahp->ah_bIQCalibration = AH_FALSE;
    /*
     * Enable MIC handling.
     */
    ahp->ah_staId1Defaults = AR_STA_ID1_CRPT_MIC_ENABLE;
    ahp->ah_tpcEnabled = AH_FALSE;      /* disabled by default */
    ahp->ah_beaconInterval = 100;       /* XXX [20..1000] */
    ahp->ah_enable32kHzClock = DONT_USE_32KHZ;/* XXX */
    ahp->ah_slottime = (u_int) -1;
    ahp->ah_acktimeout = (u_int) -1;
    ahp->ah_ctstimeout = (u_int) -1;
    ahp->ah_globaltxtimeout = (u_int) -1;
    OS_MEMCPY(&ahp->ah_bssidmask, defbssidmask, IEEE80211_ADDR_LEN);

    /*
     * 11g-specific stuff
     */
    ahp->ah_gBeaconRate = 0;        /* adhoc beacon fixed rate */

    return ahp;
#undef N
}

HAL_BOOL
ar5416ChipTest(struct ath_hal *ah)
{
    u_int32_t regAddr[2] = { AR_STA_ID0, AR_PHY_BASE+(8 << 2) };
    u_int32_t regHold[2];
    u_int32_t patternData[4] =
        { 0x55555555, 0xaaaaaaaa, 0x66666666, 0x99999999 };
    int i, j;

    /* Test PHY & MAC registers */
    for (i = 0; i < 2; i++) {
        u_int32_t addr = regAddr[i];
        u_int32_t wrData, rdData;

        regHold[i] = OS_REG_READ(ah, addr);
        for (j = 0; j < 0x100; j++) {
            wrData = (j << 16) | j;
            OS_REG_WRITE(ah, addr, wrData);
            rdData = OS_REG_READ(ah, addr);
            if (rdData != wrData) {
                HALDEBUG(ah,
"%s: address test failed addr: 0x%08x - wr:0x%08x != rd:0x%08x\n",
                __func__, addr, wrData, rdData);
                return AH_FALSE;
            }
        }
        for (j = 0; j < 4; j++) {
            wrData = patternData[j];
            OS_REG_WRITE(ah, addr, wrData);
            rdData = OS_REG_READ(ah, addr);
            if (wrData != rdData) {
                HALDEBUG(ah,
"%s: address test failed addr: 0x%08x - wr:0x%08x != rd:0x%08x\n",
                    __func__, addr, wrData, rdData);
                return AH_FALSE;
            }
        }
        OS_REG_WRITE(ah, regAddr[i], regHold[i]);
    }
    OS_DELAY(100);
    return AH_TRUE;
}

/*
 * Store the channel edges for the requested operational mode
 */
HAL_BOOL
ar5416GetChannelEdges(struct ath_hal *ah,
    u_int16_t flags, u_int16_t *low, u_int16_t *high)
{
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
    HAL_CAPABILITIES *pCap = &ahpriv->ah_caps;

    if (flags & CHANNEL_5GHZ) {
        *low = pCap->halLow5GhzChan;
        *high = pCap->halHigh5GhzChan;
        return AH_TRUE;
    }
    if ((flags & CHANNEL_2GHZ)) {
        *low = pCap->halLow2GhzChan;
        *high = pCap->halHigh2GhzChan;

        return AH_TRUE;
    }
    return AH_FALSE;
}

/*
 * Fill all software cached or static hardware state information.
 * Return failure if capabilities are to come from EEPROM and
 * cannot be read.
 */
HAL_BOOL
ar5416FillCapabilityInfo(struct ath_hal *ah)
{
#define AR_KEYTABLE_SIZE    128
    struct ath_hal_5416 *ahp = AH5416(ah);
    struct ath_hal_private *ahpriv = AH_PRIVATE(ah);
    HAL_CAPABILITIES *pCap = &ahpriv->ah_caps;
    u_int16_t capField = 0, eeval;

    eeval = ar5416EepromGet(ahp, EEP_REG_0);

    /* XXX record serial number */
    AH_PRIVATE(ah)->ah_currentRD = eeval;

    /* Read the capability EEPROM location */
    capField = ar5416EepromGet(ahp, EEP_OP_CAP);

    /* Modify reg domain on newer cards that need to work with older sw */
    if (ahpriv->ah_opmode != HAL_M_HOSTAP &&
        ahpriv->ah_subvendorid == AR_SUBVENDOR_ID_NEW_A) {
        if (ahpriv->ah_currentRD == 0x64 ||
            ahpriv->ah_currentRD == 0x65)
            ahpriv->ah_currentRD += 5;
        else if (ahpriv->ah_currentRD == 0x41)
            ahpriv->ah_currentRD = 0x43;
        HALDEBUG(ah, "%s: regdomain mapped to 0x%x\n",
            __func__, ahpriv->ah_currentRD);
    }

    /* Construct wireless mode from EEPROM */
    pCap->halWirelessModes = 0;
    eeval = ar5416EepromGet(ahp, EEP_OP_MODE);

    /*
     * TODO: disable HT40 modes if AR5416_OPFLAGS_N_5G_HT40 or
     * AR5416_OPFLAGS_N_2G_HT40 are set
     */
    if (eeval & AR5416_OPFLAGS_11A) {
        pCap->halWirelessModes |= HAL_MODE_11A | ((eeval & AR5416_OPFLAGS_N_5G_HT20) ? 0 : HAL_MODE_11NA);
    }
    if (eeval & AR5416_OPFLAGS_11G) {
        pCap->halWirelessModes |= HAL_MODE_11G | ((eeval & AR5416_OPFLAGS_N_2G_HT20) ? 0 : HAL_MODE_11NG);
        /* WAR for 20833 */
        /* propagates previous hack for bug 20833 */
        pCap->halWirelessModes |= HAL_MODE_11A | ((eeval & AR5416_OPFLAGS_N_5G_HT20) ? 0 : HAL_MODE_11NA);
    }

    pCap->halTxChainMask = ar5416EepromGet(ahp, EEP_TX_MASK);
#ifdef __LINUX_ARM_ARCH__ /* AP71 */
    pCap->halRxChainMask = ar5416EepromGet(ahp, EEP_RX_MASK);
#else /* CB71 */
    /*
     * Don't depend on CB EEPROMs for rx chain mask. Use GPIO 0
     * till that is fixed.
     */
    pCap->halRxChainMask = (ar5416GpioGet(ah, 0)) ? 0x5 : 0x7;
#endif

    pCap->halLow2GhzChan = 2312;
    pCap->halHigh2GhzChan = 2732;

    pCap->halLow5GhzChan = 4920;
    pCap->halHigh5GhzChan = 6100;

    pCap->halCipherCkipSupport = AH_FALSE;
    pCap->halCipherTkipSupport = AH_TRUE;
    pCap->halCipherAesCcmSupport = AH_TRUE;

    pCap->halMicCkipSupport    = AH_FALSE;
    pCap->halMicTkipSupport    = AH_TRUE;
    pCap->halMicAesCcmSupport  = AH_TRUE;

    pCap->halChanSpreadSupport = AH_TRUE;
    pCap->halSleepAfterBeaconBroken = AH_TRUE;

    pCap->halCompressSupport   = AH_FALSE;
    pCap->halBurstSupport = AH_TRUE;
    pCap->halFastFramesSupport = AH_FALSE;
    pCap->halChapTuningSupport = AH_TRUE;
    pCap->halTurboPrimeSupport = AH_TRUE;

    pCap->halTurboGSupport = pCap->halWirelessModes & HAL_MODE_108G;

    pCap->halXrSupport = AH_FALSE;

    pCap->halHTSupport = AH_TRUE;
    pCap->halGTTSupport = AH_TRUE;
    pCap->halPSPollBroken = AH_TRUE;    /* XXX fixed in later revs? */
    pCap->halVEOLSupport = AH_TRUE;
    pCap->halBssIdMaskSupport = AH_TRUE;
    pCap->halMcastKeySrchSupport = AH_TRUE;
    pCap->halTsfAddSupport = AH_TRUE;

    if (capField & AR_EEPROM_EEPCAP_MAXQCU)
        pCap->halTotalQueues = MS(capField, AR_EEPROM_EEPCAP_MAXQCU);
    else
        pCap->halTotalQueues = HAL_NUM_TX_QUEUES;

    if (capField & AR_EEPROM_EEPCAP_KC_ENTRIES)
        pCap->halKeyCacheSize =
            1 << MS(capField, AR_EEPROM_EEPCAP_KC_ENTRIES);
    else
        pCap->halKeyCacheSize = AR_KEYTABLE_SIZE;

    pCap->halFastCCSupport = AH_TRUE;
    pCap->halNumMRRetries   = 4;

    return AH_TRUE;
#undef AR_KEYTABLE_SIZE
}

HAL_STATUS
ar5416RadioAttach(struct ath_hal *ah)
{
    u_int32_t val;

    /*
     * Set correct Baseband to analog shift
     * setting to access analog chips.
     */
    OS_REG_WRITE(ah, AR_PHY(0), 0x00000007);

    val = ar5416GetRadioRev(ah);
    switch (val & AR_RADIO_SREV_MAJOR) {
        case 0:
            /* TODO:
             * WAR for bug 10062.  When RF_Silent is used, the
             * analog chip is reset.  So when the system boots
             * up with the radio switch off we cannot determine
             * the RF chip rev.  To workaround this check the mac/
             * phy revs and set radio rev.
             */
            val = AR_RAD5133_SREV_MAJOR;
            break;
        case AR_RAD5133_SREV_MAJOR:
        case AR_RAD5122_SREV_MAJOR:
        case AR_RAD2133_SREV_MAJOR:
        case AR_RAD2122_SREV_MAJOR:
            break;
        default:
#ifdef AH_DEBUG
            HALDEBUG(ah, "%s: 5G Radio Chip Rev 0x%02X is not supported by "
                "this driver\n", __func__,
                AH_PRIVATE(ah)->ah_analog5GhzRev);
#endif
            return HAL_ENOTSUPP;
    }

    AH_PRIVATE(ah)->ah_analog5GhzRev = val;

    return HAL_OK;
}

static inline void
ar5416AniSetup(struct ath_hal *ah)
{
    struct ath_hal_5416 *ahp = AH5416(ah);
    int i;

    const int totalSizeDesired[] = { -55, -55, -55, -55, -62 };
    const int coarseHigh[]       = { -14, -14, -14, -14, -12 };
    const int coarseLow[]        = { -64, -64, -64, -64, -70 };
    const int firpwr[]           = { -78, -78, -78, -78, -80 };

    for (i = 0; i < 5; i++) {
        ahp->ah_totalSizeDesired[i] = totalSizeDesired[i];
        ahp->ah_coarseHigh[i] = coarseHigh[i];
        ahp->ah_coarseLow[i] = coarseLow[i];
        ahp->ah_firpwr[i] = firpwr[i];
    }
}

static HAL_BOOL
ar5416GetChipPowerLimits(struct ath_hal *ah, HAL_CHANNEL *chans, u_int32_t nchans)
{
    struct ath_hal_5416 *ahp = AH5416(ah);

    return ahp->ah_rfHal.getChipPowerLim(ah, chans, nchans);
}

static inline int
ar5416GetRadioRev(struct ath_hal *ah)
{
    u_int32_t val;
    int i;

    /* Read Radio Chip Rev Extract */
    OS_REG_WRITE(ah, AR_PHY(0x36), 0x00007058);
    for (i = 0; i < 8; i++)
        OS_REG_WRITE(ah, AR_PHY(0x20), 0x00010000);
    val = (OS_REG_READ(ah, AR_PHY(256)) >> 24) & 0xff;
    val = ((val & 0xf0) >> 4) | ((val & 0x0f) << 4);
    return ath_hal_reverseBits(val, 8);
}

static inline HAL_STATUS
ar5416RfAttach(struct ath_hal *ah)
{
    HAL_BOOL rfStatus = AH_FALSE;
    HAL_STATUS ecode = HAL_OK;

    rfStatus = ar2133RfAttach(ah, &ecode);
    if (!rfStatus) {
        HALDEBUG(ah, "%s: RF setup failed, status %u\n",
            __func__, ecode);
        return ecode;
    }

    return HAL_OK;
}

static inline HAL_STATUS
ar5416InitMacAddr(struct ath_hal *ah)
{
    u_int32_t sum;
    int i;
    u_int16_t eeval;
    struct ath_hal_5416 *ahp = AH5416(ah);

    sum = 0;
    for (i = 0; i < 3; i++) {
        eeval = ar5416EepromGet(ahp, AR_EEPROM_MAC(i));
        sum += eeval;
        ahp->ah_macaddr[2*i] = eeval >> 8;
        ahp->ah_macaddr[2*i + 1] = eeval & 0xff;
    }
    if (sum == 0 || sum == 0xffff*3) {
        HALDEBUG(ah, "%s: mac address read failed: %s\n",
            __func__, ath_hal_ether_sprintf(ahp->ah_macaddr));
        return HAL_EEBADMAC;
    }

    return HAL_OK;
}

/*
 * Code for the "real" chip i.e. non-emulation. Review and revisit
 * when actual hardware is at hand.
 */
static inline HAL_STATUS
ar5416HwAttach(struct ath_hal *ah)
{
    HAL_STATUS ecode;

    if (!ar5416ChipTest(ah)) {
        HALDEBUG(ah, "%s: hardware self-test failed\n", __func__);
        return HAL_ESELFTEST;
    }

    ecode = ar5416RadioAttach(ah);
    if (ecode != HAL_OK)
        return ecode;

    ecode = ar5416EepromAttach(ah);
    if (ecode != HAL_OK)
        return ecode;

    ecode = ar5416RfAttach(ah);
    if (ecode != HAL_OK)
        return ecode;

#if 0
    ar5416AniSetup(ah); /* setup 5416-specific ANI tables */
    ar5416AniAttach(ah);
#endif

    return HAL_OK;
}

static inline void
ar5416HwDetach(struct ath_hal *ah)
{
    /* XXX EEPROM allocated state */
    ar5416AniDetach(ah);
    ar5416RfDetach(ah);
    ar5416RadarDetach(ah);
}

#endif /* AH_SUPPORT_AR5416 */
