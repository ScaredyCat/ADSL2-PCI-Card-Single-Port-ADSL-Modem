#ifndef _ATH_AR5416_DESC_10_H_
#define _ATH_AR5416_DESC_10_H_

/* Owl 1.0 descriptor. */


struct ar5416_desc_10 {
    u_int32_t   ds_link;    /* link pointer */
    u_int32_t   ds_data;    /* data buffer pointer */
    u_int32_t   ds_ctl0;    /* DMA control 0 */
    u_int32_t   ds_ctl1;    /* DMA control 1 */
    union {
        struct { /* tx desc has 8 control words(for owl1.0)/12 
                    control words(for owl2.0) + 10 status words */
            u_int32_t   ctl2;
            u_int32_t   ctl3;
            u_int32_t   ctl4;
            u_int32_t   ctl5;
            u_int32_t   ctl6;
            u_int32_t   ctl7;
            u_int32_t   status0;
            u_int32_t   status1;
            u_int32_t   status2;
            u_int32_t   status3;
            u_int32_t   status4;
            u_int32_t   status5;
            u_int32_t   status6;
            u_int32_t   status7;
            u_int32_t   status8;
            u_int32_t   status9;
        } tx;
        struct { /* rx desc has 2 control words + 9 status words */
            u_int32_t   status0;
            u_int32_t   status1;
            u_int32_t   status2;
            u_int32_t   status3;
            u_int32_t   status4;
            u_int32_t   status5;
            u_int32_t   status6;
            u_int32_t   status7;
            u_int32_t   status8;
        } rx;
    } u;
} __packed;


#define AR5416DESC_10(_ds) ((struct ar5416_desc_10 *)(_ds))
#define AR5416DESC_CONST_10(_ds) ((const struct ar5416_desc_10 *)(_ds))

	/* TX functions */

extern  HAL_BOOL ar5416UpdateCTSForBursting_10(struct ath_hal *, struct ath_desc *,
         struct ath_desc *,struct ath_desc *, struct ath_desc *,
         u_int32_t, u_int32_t);
extern  HAL_BOOL ar5416SetupTxDesc_10(struct ath_hal *ah, struct ath_desc *ds,
        u_int pktLen, u_int hdrLen, HAL_PKT_TYPE type, u_int txPower,
        u_int txRate0, u_int txTries0,
        u_int keyIx, u_int antMode, u_int flags,
        u_int rtsctsRate, u_int rtsctsDuration,
        u_int compicvLen, u_int compivLen, u_int comp);
extern  HAL_BOOL ar5416SetupXTxDesc_10(struct ath_hal *, struct ath_desc *,
        u_int txRate1, u_int txRetries1,
        u_int txRate2, u_int txRetries2,
        u_int txRate3, u_int txRetries3);
extern  HAL_BOOL ar5416FillTxDesc_10(struct ath_hal *ah, struct ath_desc *ds,
        u_int segLen, HAL_BOOL firstSeg, HAL_BOOL lastSeg,
        const struct ath_desc *ds0);
extern  HAL_STATUS ar5416ProcTxDesc_10(struct ath_hal *ah, struct ath_desc *);

extern void ar5416Set11nTxDesc_10(struct ath_hal *ah, struct ath_desc *ds,
       u_int pktLen, HAL_PKT_TYPE type, u_int txPower,
       u_int keyIx, HAL_KEY_TYPE keyType, u_int flags);
extern void ar5416Set11nRateScenario_10(struct ath_hal *ah, struct ath_desc *ds,
       u_int durUpdateEn, u_int rtsctsRate, HAL_11N_RATE_SERIES series[], 
       u_int nseries, u_int flags);
extern void ar5416Set11nAggrFirst_10(struct ath_hal *ah, struct ath_desc *ds,
       u_int aggrLen, u_int numDelims);
extern void ar5416Set11nAggrMiddle_10(struct ath_hal *ah, struct ath_desc *ds,
       u_int numDelims);
extern void ar5416Set11nAggrLast_10(struct ath_hal *ah, struct ath_desc *ds);
extern void ar5416Clr11nAggr_10(struct ath_hal *ah, struct ath_desc *ds);

extern void ar5416Set11nBurstDuration_10(struct ath_hal *ah, struct ath_desc *ds,
       u_int burstDuration);

	/* RX Functions */

extern  HAL_BOOL ar5416SetupRxDesc_10(struct ath_hal *,
        struct ath_desc *, u_int32_t size, u_int flags);
extern  HAL_STATUS ar5416ProcRxDescFast_10(struct ath_hal *ah, 
                                          struct ath_desc *, u_int32_t,
                                          struct ath_desc *, 
                                          struct ath_rx_status *);

#endif
