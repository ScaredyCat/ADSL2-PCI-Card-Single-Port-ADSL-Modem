/*-
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 * $FreeBSD: src/sys/dev/ath/if_athioctl.h,v 1.5 2003/12/28 07:00:32 sam Exp $
 */

/*
 * Ioctl-related defintions for the Atheros Wireless LAN controller driver.
 */
#ifndef _DEV_ATH_ATHIOCTL_H
#define _DEV_ATH_ATHIOCTL_H

/*
 * 11n tx/rx stats
 */
struct ath_11n_stats {
    u_int32_t   tx_pkts;            /* total tx data packets */
    u_int32_t   tx_checks;          /* tx drops in wrong state */
    u_int32_t   tx_drops;           /* tx drops due to qdepth limit */
    u_int32_t   tx_minqdepth;       /* tx when h/w queue depth is low */
    u_int32_t   tx_queue;           /* tx pkts when h/w queue is busy */
    u_int32_t   tx_comps;           /* tx completions */
    u_int32_t   tx_stopfiltered;    /* tx pkts filtered for requeueing */
    u_int32_t   tx_qnull;           /* txq empty occurences */
    u_int32_t   tx_noskbs;          /* tx no skbs for encapsulations */
    u_int32_t   tx_nobufs;          /* tx no descriptors */
    u_int32_t   tx_badsetups;       /* tx key setup failures */
    u_int32_t   tx_normnobufs;      /* tx no desc for legacy packets */
    u_int32_t   tx_schednone;       /* tx schedule pkt queue empty */
    u_int32_t   tx_bars;            /* tx bars sent */
    u_int32_t   tx_compunaggr;      /* tx unaggregated frame completions */
    u_int32_t   txunaggr_xretry;    /* tx unaggregated excessive retries */
    u_int32_t   tx_compaggr;        /* tx aggregated completions */
    u_int32_t   tx_bawadv;          /* tx block ack window advanced */
    u_int32_t   tx_bawretries;      /* tx block ack window retries */
    u_int32_t   tx_bawnorm;         /* tx block ack window additions */
    u_int32_t   tx_bawupdates;      /* tx block ack window updates */
    u_int32_t   tx_bawupdtadv;      /* tx block ack window advances */
    u_int32_t   tx_retries;         /* tx retries of sub frames */
    u_int32_t   tx_xretries;        /* tx excessive retries of aggregates */
    u_int32_t   txaggr_noskbs;      /* tx no skbs for aggr encapsualtion */
    u_int32_t   txaggr_nobufs;      /* tx no desc for aggr */
    u_int32_t   txaggr_badkeys;     /* tx enc key setup failures */
    u_int32_t   txaggr_schedwindow; /* tx no frame scheduled: baw limited */
    u_int32_t   txaggr_single;      /* tx frames not aggregated */
    u_int32_t   txaggr_compgood;    /* tx aggr good completions */
    u_int32_t   txaggr_compxretry;  /* tx aggr excessive retries */
    u_int32_t   txaggr_compretries; /* tx aggr unacked subframes */
    u_int32_t   txunaggr_compretries; /* tx non-aggr unacked subframes */
    u_int32_t   txaggr_prepends;    /* tx aggr old frames requeued */
    u_int32_t   txaggr_filtered;    /* filtered aggr packet */
    u_int32_t   txaggr_fifo;        /* fifo underrun of aggregate */
    u_int32_t   txaggr_xtxop;       /* txop exceeded for an aggregate */
    u_int32_t   txaggr_desc_cfgerr; /* aggregate descriptor config error */
    u_int32_t   txaggr_data_urun;   /* data underrun for an aggregate */
    u_int32_t   txaggr_delim_urun;  /* delimiter underrun for an aggregate */
    u_int32_t   txaggr_errlast;     /* tx aggr: last sub-frame failed */
    u_int32_t   txunaggr_errlast;   /* tx non-aggr: last frame failed */
    u_int32_t   txaggr_longretries; /* tx aggr h/w long retries */
    u_int32_t   txaggr_shortretries;/* tx aggr h/w short retries */
    u_int32_t   txaggr_timer_exp;   /* tx aggr : tx timer expired */
    u_int32_t   txaggr_babug;       /* tx aggr : BA bug */
    u_int32_t   rx_pkts;            /* rx pkts */
    u_int32_t   rx_aggr;            /* rx aggregated packets */
    u_int32_t   rx_aggrbadver;      /* rx pkts with bad version */
    u_int32_t   rx_bars;            /* rx bars */
    u_int32_t   rx_nonqos;          /* rx non qos-data frames */
    u_int32_t   rx_seqreset;        /* rx sequence resets */
    u_int32_t   rx_oldseq;          /* rx old packets */
    u_int32_t   rx_bareset;         /* rx block ack window reset */
    u_int32_t   rx_baresetpkts;     /* rx pts indicated due to baw resets */
    u_int32_t   rx_dup;             /* rx duplicate pkts */
    u_int32_t   rx_baadvance;       /* rx block ack window advanced */
    u_int32_t   rx_recvcomp;        /* rx pkt completions */
    u_int32_t   rx_bardiscard;      /* rx bar discarded */
    u_int32_t   rx_barcomps;        /* rx pkts unblocked on bar reception */
    u_int32_t   rx_barrecvs;        /* rx pkt completions on bar reception */
    u_int32_t   rx_skipped;         /* rx pkt sequences skipped on timeout */
    u_int32_t   rx_comp_to;         /* rx indications due to timeout */
    u_int32_t   wd_tx_active;       /* watchdog: tx is active */
    u_int32_t   wd_tx_inactive;     /* watchdog: tx is not active */
    u_int32_t   wd_tx_hung;         /* watchdog: tx is hung */
    u_int32_t   wd_spurious;        /* watchdog: spurious tx hang */
    u_int32_t   tx_requeue;         /* filter & requeue on 20/40 transitions */
    u_int32_t   tx_drain_txq;       /* draining tx queue on error */
    u_int32_t   tx_drain_tid;       /* draining tid buf queue on error */
    u_int32_t   tx_drain_bufs;      /* buffers drained from pending tid queue */
    u_int32_t   tx_unaggr_filtered; /* unaggregated tx pkts filtered */
    u_int32_t   tx_aggr_filtered;   /* aggregated tx pkts filtered */
    u_int32_t   tx_filtered;        /* total sub-frames filtered */
};

#define NTRENTS		128
struct ath_ampdu_trc_entry {
    u_int16_t   tre_seqst;  	    /* starting sequence of aggr */
    u_int16_t   tre_baseqst;  	    /* starting sequence of ba */
    u_int32_t   tre_npkts;          /* packets in aggregate */
    u_int32_t   tre_aggrlen;        /* aggregation length */
    u_int32_t   tre_bamap0;         /* block ack bitmap word 0 */
    u_int32_t   tre_bamap1;         /* block ack bitmap word 1 */
};
struct ath_ampdu_trc {
    u_int32_t   tr_head;
    u_int32_t   tr_tail;
    struct ath_ampdu_trc_entry tr_ents[NTRENTS];
};
#define AMPDU_TRCINIT(_sc) do {                \
    (_sc)->sc_stats.ast_ampdu_trc.tr_head = 0; \
    (_sc)->sc_stats.ast_ampdu_trc.tr_tail = 0; \
} while(0)

#define AMPDU_TRCINC(_pt) (_pt) = (((_pt) + 1) % NTRENTS)
#define AMPDU_TRCADD(_sc, _npkts, _al, _seqst, _seqstba, _bamap0, _bamap1) do {         \
    if (!(_sc)->noampdutrc) {                                                           \
    struct ath_ampdu_trc_entry *__e;                                                    \
    __e = &(_sc)->sc_stats.ast_ampdu_trc.tr_ents[(_sc)->sc_stats.ast_ampdu_trc.tr_tail];\
    __e->tre_seqst = (_seqst);								\
    __e->tre_baseqst = (_seqstba);								\
    __e->tre_npkts = (_npkts);                                                          \
    __e->tre_aggrlen = (_al);                                                           \
    __e->tre_bamap0 = (_bamap0);                                                        \
    __e->tre_bamap1 = (_bamap1);                                                        \
    AMPDU_TRCINC((_sc)->sc_stats.ast_ampdu_trc.tr_tail);                                \
    if ((_sc)->sc_stats.ast_ampdu_trc.tr_head == (_sc)->sc_stats.ast_ampdu_trc.tr_tail) \
        AMPDU_TRCINC((_sc)->sc_stats.ast_ampdu_trc.tr_head);                            \
    }                                                                                   \
} while (0)

#define AMPDU_TRCEND(_sc) (_sc)->noampdutrc = 1

struct ath_stats {
    u_int32_t   ast_watchdog;   /* device reset by watchdog */
    u_int32_t   ast_hardware;   /* fatal hardware error interrupts */
    u_int32_t   ast_bmiss;  /* beacon miss interrupts */
    u_int32_t   ast_rxorn;  /* rx overrun interrupts */
    u_int32_t   ast_rxeol;  /* rx eol interrupts */
    u_int32_t   ast_txurn;  /* tx underrun interrupts */
    u_int32_t   ast_txto;   /* tx timeout interrupts */
    u_int32_t   ast_cst;    /* carrier sense timeout interrupts */
    u_int32_t   ast_mib;    /* mib interrupts */
    u_int32_t   ast_tx_packets; /* packet sent on the interface */
    u_int32_t   ast_tx_mgmt;    /* management frames transmitted */
    u_int32_t   ast_tx_discard; /* frames discarded prior to assoc */
    u_int32_t   ast_tx_invalid; /* frames discarded 'cuz device gone */
    u_int32_t   ast_tx_qstop;   /* tx queue stopped 'cuz full */
    u_int32_t   ast_tx_encap;   /* tx encapsulation failed */
    u_int32_t   ast_tx_nonode;  /* tx failed 'cuz no node */
    u_int32_t   ast_tx_nobuf;   /* tx failed 'cuz no tx buffer (data) */
    u_int32_t   ast_tx_nobufmgt;/* tx failed 'cuz no tx buffer (mgmt)*/
    u_int32_t   ast_tx_xretries;/* tx failed 'cuz too many retries */
    u_int32_t   ast_tx_fifoerr; /* tx failed 'cuz FIFO underrun */
    u_int32_t   ast_tx_filtered;/* tx failed 'cuz xmit filtered */
    u_int32_t   ast_tx_timer_exp;/* tx timer expired */
    u_int32_t   ast_tx_shortretry;/* tx on-chip retries (short) */
    u_int32_t   ast_tx_longretry;/* tx on-chip retries (long) */
    u_int32_t   ast_tx_badrate; /* tx failed 'cuz bogus xmit rate */
    u_int32_t   ast_tx_noack;   /* tx frames with no ack marked */
    u_int32_t   ast_tx_rts; /* tx frames with rts enabled */
    u_int32_t   ast_tx_cts; /* tx frames with cts enabled */
    u_int32_t   ast_tx_shortpre;/* tx frames with short preamble */
    u_int32_t   ast_tx_altrate; /* tx frames with alternate rate */
    u_int32_t   ast_tx_protect; /* tx frames with protection */
    u_int32_t   ast_rx_orn; /* rx failed 'cuz of desc overrun */
    u_int32_t   ast_rx_crcerr;  /* rx failed 'cuz of bad CRC */
    u_int32_t   ast_rx_fifoerr; /* rx failed 'cuz of FIFO overrun */
    u_int32_t   ast_rx_badcrypt;/* rx failed 'cuz decryption */
    u_int32_t   ast_rx_badmic;  /* rx failed 'cuz MIC failure */
    u_int32_t   ast_rx_phyerr;  /* rx PHY error summary count */
    u_int32_t   ast_rx_phy[64]; /* rx PHY error per-code counts */
    u_int32_t   ast_rx_tooshort;/* rx discarded 'cuz frame too short */
    u_int32_t   ast_rx_toobig;  /* rx discarded 'cuz frame too large */
    u_int32_t   ast_rx_nobuf;   /* rx setup failed 'cuz no skbuff */
    u_int32_t   ast_rx_packets; /* packet recv on the interface */
    u_int32_t   ast_rx_mgt; /* management frames received */
    u_int32_t   ast_rx_ctl; /* control frames received */
    int8_t      ast_tx_rssi_combined;/* tx rssi of last ack [combined] */
    int8_t      ast_tx_rssi_ctl0;    /* tx rssi of last ack [ctl, chain 0] */
    int8_t      ast_tx_rssi_ctl1;    /* tx rssi of last ack [ctl, chain 1] */
    int8_t      ast_tx_rssi_ctl2;    /* tx rssi of last ack [ctl, chain 2] */
    int8_t      ast_tx_rssi_ext0;    /* tx rssi of last ack [ext, chain 0] */
    int8_t      ast_tx_rssi_ext1;    /* tx rssi of last ack [ext, chain 1] */
    int8_t      ast_tx_rssi_ext2;    /* tx rssi of last ack [ext, chain 2] */
    int8_t      ast_rx_rssi_combined;/* rx rssi from histogram [combined]*/
    int8_t      ast_rx_rssi_ctl0; /* rx rssi from histogram [ctl, chain 0] */
    int8_t      ast_rx_rssi_ctl1; /* rx rssi from histogram [ctl, chain 1] */
    int8_t      ast_rx_rssi_ctl2; /* rx rssi from histogram [ctl, chain 2] */
    int8_t      ast_rx_rssi_ext0; /* rx rssi from histogram [ext, chain 0] */
    int8_t      ast_rx_rssi_ext1; /* rx rssi from histogram [ext, chain 1] */
    int8_t      ast_rx_rssi_ext2; /* rx rssi from histogram [ext, chain 2] */
    u_int32_t   ast_be_xmit;    /* beacons transmitted */
    u_int32_t   ast_be_nobuf;   /* no skbuff available for beacon */
    u_int32_t   ast_per_cal;    /* periodic calibration calls */
    u_int32_t   ast_per_calfail;/* periodic calibration failed */
    u_int32_t   ast_per_rfgain; /* periodic calibration rfgain reset */
    u_int32_t   ast_rate_calls; /* rate control checks */
    u_int32_t   ast_rate_raise; /* rate control raised xmit rate */
    u_int32_t   ast_rate_drop;  /* rate control dropped xmit rate */
    u_int32_t   ast_ant_defswitch;/* rx/default antenna switches */
    u_int32_t   ast_ant_txswitch;/* tx antenna switches */
    u_int32_t   ast_ant_rx[8];  /* rx frames with antenna */
    u_int32_t   ast_ant_tx[8];  /* tx frames with antenna */
    u_int32_t   ast_suspend;    /* driver suspend calls */
    u_int32_t   ast_resume;     /* driver resume calls  */
    u_int32_t   ast_shutdown;   /* driver shutdown calls  */
    u_int32_t   ast_init;       /* driver init calls  */
    u_int32_t   ast_stop;       /* driver stop calls  */
    u_int32_t   ast_reset;      /* driver resets      */
    u_int32_t   ast_nodealloc;  /* nodes allocated    */
    u_int32_t   ast_nodefree;   /* nodes deleted      */
    u_int32_t   ast_keyalloc;   /* keys allocated     */
    u_int32_t   ast_keydelete;  /* keys deleted       */
    u_int32_t   ast_bstuck;     /* beacon stuck       */
    u_int32_t   ast_draintxq;   /* drain tx queue     */
    u_int32_t   ast_stopdma;    /* stop tx queue dma  */
    u_int32_t   ast_stoprecv;   /* stop recv          */
    u_int32_t   ast_startrecv;  /* start recv         */
    u_int32_t   ast_flushrecv;  /* flush recv         */
    u_int32_t   ast_chanchange; /* channel changes    */
    u_int32_t   ast_fastcc;     /* Number of fast channel changes */
    u_int32_t   ast_fastcc_errs;/* Number of failed fast channel changes */
    u_int32_t   ast_chanset;    /* channel sets       */
    u_int32_t   ast_cwm_mac;    /* CWM - mac mode switch */
    u_int32_t   ast_cwm_phy;    /* CWM - phy mode switch */
    u_int32_t   ast_cwm_requeue;/* CWM - requeue dest node packets */
    u_int32_t   ast_rx_delim_pre_crcerr; /* pre-delimiter crc errors */
    u_int32_t   ast_rx_delim_post_crcerr; /* post-delimiter crc errors */
    u_int32_t   ast_rx_decrypt_busyerr; /* decrypt busy errors */
    struct ath_11n_stats ast_11n_stats; /* 11n statistics */
    struct ath_ampdu_trc ast_ampdu_trc; /* ampdu trc */
};

struct ath_diag {
    char    ad_name[IFNAMSIZ];  /* if name, e.g. "ath0" */
    u_int16_t ad_id;
#define ATH_DIAG_DYN    0x8000      /* allocate buffer in caller */
#define ATH_DIAG_IN 0x4000      /* copy in parameters */
#define ATH_DIAG_OUT    0x0000      /* copy out results (always) */
#define ATH_DIAG_ID 0x0fff
    u_int16_t ad_in_size;       /* pack to fit, yech */
    caddr_t ad_in_data;
    caddr_t ad_out_data;
    u_int   ad_out_size;

};

struct ath_cwmdbg {
    u_int32_t   dc_cmd;
    u_int32_t   dc_arg;
};

#define ATH_DBGCWM_CMD_EVENT    0   /* send event */
#define ATH_DBGCWM_CMD_CTL  1   /* set control channel busy indicator */
#define ATH_DBGCWM_CMD_EXT  2   /* set extension channel busy indicator */
#define ATH_DBGCWM_CMD_VCTL 3   /* set virtual control channel busy indicator */
#define ATH_DBGCWM_CMD_VEXT 4   /* set virtual extension channel busy indicator */

struct ath_cwminfo {
    /* current state */
    u_int32_t   ci_chwidth; /* channel width */
    u_int32_t   ci_macmode; /* MAC mode */
    u_int32_t   ci_phymode; /* PHY mode */
    u_int32_t	ci_extbusyper; /* extension busy (percent) */
};

#ifdef __linux__
#define SIOCGATHSTATS       (SIOCDEVPRIVATE+0)
#define SIOCGATHDIAG        (SIOCDEVPRIVATE+1)
#define SIOCGATHCWMINFO     (SIOCDEVPRIVATE+2)
#define SIOCGATHCWMDBG      (SIOCDEVPRIVATE+3)
#define SIOCGATHSTATSCLR    (SIOCDEVPRIVATE+4)
#else
#define SIOCGATHSTATS       _IOWR('i', 137, struct ifreq)
#define SIOCGATHDIAG        _IOWR('i', 138, struct ath_diag)
#define SIOCGATHCWMINFO     _IOWR('i', 139, struct ath_cwminfo)
#define SIOCGATHCWMDBG 	    _IOWR('i', 140, struct ath_cwmdbg)
#define SIOCGATHSTATSCLR    _IOWR('i', 141, struct ifreq)
#endif
#endif /* _DEV_ATH_ATHIOCTL_H */
