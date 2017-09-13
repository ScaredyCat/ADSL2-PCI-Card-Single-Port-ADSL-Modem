#ifndef _PKTLOG_HAL_H_
#define _PKTLOG_HAL_H_

#include "pktlog_fmt.h"

/* Parameter types for packet logging driver hooks */
struct log_ani {
    u_int8_t phyStatsDisable;
    u_int8_t noiseImmunLvl;
    u_int8_t spurImmunLvl;
    u_int8_t ofdmWeakDet;
    u_int8_t cckWeakThr;
    u_int16_t firLvl;
    u_int16_t listenTime;
    u_int32_t cycleCount;
    u_int32_t ofdmPhyErrCount;
    u_int32_t cckPhyErrCount;
    int8_t rssi;
    int32_t misc[8];            /* Can be used for HT specific or other misc info */
    /* TBD: Add other required log information */
};

struct ath_pktlog_halfuncs {
    void (*pktlog_ani) (HAL_SOFTC, struct log_ani *, u_int16_t);
};

#define ath_log_ani(_sc, _log_data, _flags)                         \
        do {                                                        \
            if(g_pktlog_halfuncs) {                                    \
                g_pktlog_halfuncs->pktlog_ani(_sc, _log_data, _flags); \
            }                                                       \
        }while(0)

#endif                          /* _PKTLOG_HAL_H_ */
