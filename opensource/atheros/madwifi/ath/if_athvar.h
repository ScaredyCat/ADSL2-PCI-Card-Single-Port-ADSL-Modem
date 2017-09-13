/*-
 * Copyright (c) 2002-2004 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
    without modification.
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
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/madwifi/ath/if_athvar.h#62 $
 */

/*
 * Defintions for the Atheros Wireless LAN controller driver.
 */
#ifndef _DEV_ATH_ATHVAR_H
#define _DEV_ATH_ATHVAR_H

#include "ah.h"
#include "if_athioctl.h"
#include "net80211/ieee80211.h"     /* XXX for WME_NUM_AC */
#ifdef ATH_CWM
#include "if_ath_cwm.h"         /* Channel Width Management */
#endif
#include "if_athrate.h"         /* rate control */

#ifdef DEBUG_PKTLOG
#include "pktlog.h"
#endif

/*
 * Deduce if tasklets are available.  If not then
 * fall back to using the immediate work queue.
 */
#include <linux/interrupt.h>
#ifdef DECLARE_TASKLET          /* native tasklets */
#define tq_struct tasklet_struct
#define ATH_INIT_TQUEUE(a,b,c)      tasklet_init((a),(b),(unsigned long)(c))
#define ATH_SCHEDULE_TQUEUE(a,b)    tasklet_schedule((a))
typedef unsigned long TQUEUE_ARG;
#define mark_bh(a)
#else                   /* immediate work queue */
#define ATH_INIT_TQUEUE(a,b,c)      INIT_TQUEUE(a,b,c)
#define ATH_SCHEDULE_TQUEUE(a,b) do {       \
    *(b) |= queue_task((a), &tq_immediate); \
} while(0)
typedef void *TQUEUE_ARG;
#define tasklet_disable(t)  do { (void) t; local_bh_disable(); } while (0)
#define tasklet_enable(t)   do { (void) t; local_bh_enable(); } while (0)
#endif /* !DECLARE_TASKLET */

/*
 * Guess how the interrupt handler should work.
 */
#if !defined(IRQ_NONE)
typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#endif /* !defined(IRQ_NONE) */

#ifndef SET_MODULE_OWNER
#define SET_MODULE_OWNER(dev) do {      \
    dev->owner = THIS_MODULE;       \
} while (0)
#endif

#ifndef SET_NETDEV_DEV
#define SET_NETDEV_DEV(ndev, pdev)
#endif

/*
 * Deal with the sysctl handler api changing.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8)
#define ATH_SYSCTL_DECL(f, ctl, write, filp, buffer, lenp, ppos) \
    f(ctl_table *ctl, int write, struct file *filp, void *buffer, \
        size_t *lenp)
#define ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer, lenp, ppos) \
    proc_dointvec(ctl, write, filp, buffer, lenp)
#define ATH_SYSCTL_PROC_DOSTRING(ctl, write, filp, buffer, lenp, ppos) \
    proc_dostring(ctl, write, filp, buffer, lenp)
#else /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,8) */
#define ATH_SYSCTL_DECL(f, ctl, write, filp, buffer, lenp, ppos) \
    f(ctl_table *ctl, int write, struct file *filp, void *buffer,\
        size_t *lenp, loff_t *ppos)
#define ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer, lenp, ppos) \
    proc_dointvec(ctl, write, filp, buffer, lenp, ppos)
#define ATH_SYSCTL_PROC_DOSTRING(ctl, write, filp, buffer, lenp, ppos) \
    proc_dostring(ctl, write, filp, buffer, lenp, ppos)
#endif

#define ATH_TIMEOUT     1000

/*
 * Maximum acceptable MTU
 * MAXFRAMEBODY - WEP - QOS - RSN/WPA:
 * 2312 - 8 - 2 - 12 = 2290
 */
#define ATH_MAX_MTU     2290
#define ATH_MIN_MTU     32

#define ATH_RXBUF   256     /* number of RX buffers */
#define ATH_TXBUF   500     /* number of TX buffers */
#define ATH_BCBUF   4       /* number of beacon buffers */

/* free buffer threshold to restart net dev */
#define ATH_TXBUF_FREE_THRESHOLD  (ATH_TXBUF/20)

#define TAIL_DROP_COUNT 50             /* maximum number of queued frames allowed */

/*
 * The only case where we see skbuff chains is due to FF aggregation in
 * the driver.
 */
#ifdef ATH_SUPERG_FF
#define ATH_TXDESC  2       /* number of descriptors per buffer */
#else
#define ATH_TXDESC  1       /* number of descriptors per buffer */
#endif

#define ATH_TXMAXTRY    11      /* max number of transmit attempts */

/* Compress settings */
#define ATH_COMP_THRESHOLD  256         /* no compression for frames
                       longer than this threshold  */
#define ATH_COMP_PROC_NO_COMP_NO_CCS    3
#define ATH_COMP_PROC_NO_COMP_ADD_CCS   2
#define ATH_COMP_PROC_COMP_NO_OPTIAML   1
#define ATH_COMP_PROC_COMP_OPTIMAL      0
#define ATH_DEFAULT_COMP_PROC           ATH_COMP_PROC_COMP_OPTIMAL

#define INVALID_DECOMP_INDEX        0xFFFF

#define WEP_IV_FIELD_SIZE       4       /* wep IV field size */
#define WEP_ICV_FIELD_SIZE      4       /* wep ICV field size */
#define AES_ICV_FIELD_SIZE      8       /* AES ICV field size */
#define EXT_IV_FIELD_SIZE       4       /* ext IV field size */


/* XR specific macros */

#define XR_DEFAULT_GRPPOLL_RATE_STR "0.25 1 1 3 3 6 6 20"
#define GRPPOLL_RATE_STR_LEN  64
#define XR_SLOT_DELAY         30      // in usec
#define XR_AIFS               0
#define XR_NUM_RATES          5
#define XR_NUM_SUP_RATES      8
/* XR uplink should have same cwmin/cwmax value */
#define XR_CWMIN_CWMAX              7

#define XR_DATA_AIFS    3
#define XR_DATA_CWMIN   31
#define XR_DATA_CWMAX   1023

/* pick the threshold so that we meet most of the regulatory constraints */
#define XR_FRAGMENTATION_THRESHOLD            540
#define XR_TELEC_FRAGMENTATION_THRESHOLD      442

#define XR_MAX_GRP_POLL_PERIOD             1000 /* Maximum Group Poll Periodicity */

#define XR_DEFAULT_POLL_INTERVAL          100
#define XR_MIN_POLL_INTERVAL              30
#define XR_MAX_POLL_INTERVAL              1000
#define XR_DEFAULT_POLL_COUNT             32
#define XR_MIN_POLL_COUNT                 16
#define XR_MAX_POLL_COUNT                 64
#define XR_POLL_UPDATE_PERIOD             10 /* number of xr beacons */
#define XR_GRPPOLL_PERIOD_FACTOR          5 /* factor sed in calculating grp poll interval */

/*
 * Maximum Values in ms for group poll periodicty
 */
#define GRP_POLL_PERIOD_NO_XR_STA_MAX       100
#define GRP_POLL_PERIOD_XR_STA_MAX          30

 /*
 * Percentage of the configured poll periodicity
 */
#define GRP_POLL_PERIOD_FACTOR_XR_STA     30  /* When XR Stations associated freq is 30% higher */

#define A_MAX(a,b) ((a) > (b) ? (a) : (b))

/*
 * Macros to obtain the Group Poll Periodicty in various situations
 *
 * Curerntly there are the two cases
 * (a) When there are no XR STAs associated
 * (b) When there is atleast one XR STA associated
 */
#define GRP_POLL_PERIOD_NO_XR_STA(sc) (sc->sc_xrpollint)
#define GRP_POLL_PERIOD_XR_STA(sc)                                                   \
        A_MAX(GRP_POLL_PERIOD_FACTOR_XR_STA * (sc->sc_xrpollint / 100),GRP_POLL_PERIOD_XR_STA_MAX)

/*
 * When there are no XR STAs and a valid double chirp is received then the Group Polls are
 * transmitted for 10 seconds from the time of the last valid double double-chirp
 */
#define NO_XR_STA_GRPPOLL_TX_DUR    10000


/*
 * The key cache is used for h/w cipher state and also for
 * tracking station state such as the current tx antenna.
 * We also setup a mapping table between key cache slot indices
 * and station state to short-circuit node lookups on rx.
 * Different parts have different size key caches.  We handle
 * up to ATH_KEYMAX entries (could dynamically allocate state).
 */
#define ATH_KEYMAX  128     /* max key cache size we handle */
#define ATH_KEYBYTES    (ATH_KEYMAX/NBBY)   /* storage space in bytes */
#define ATH_MIN_FF_RATE 12000       /* min rate fof ff aggragattion.in Kbps  */

/*
 * owl a-mpdu enhancements
 */

/*
 * todo: move to ieee80211.h
 */
#define WME_NUM_TID     16
#define WME_BA_BMP_SIZE 64
#define WME_MAX_BA      WME_BA_BMP_SIZE

struct ath_softc;
struct ath_buf;
struct ath_txq;
struct ath_atx_ac;

/*
 * timeout structures
 */
typedef void (*ath_timer_func_t)(struct ath_softc *sc, void *tmr_arg);

typedef struct ath_timer {
    ath_timer_func_t    tmr_func;       /* timer function */
    void                *tmr_arg;       /* timer arg */
    int                 tmr_ticks;      /* timeout in ticks */
    int                 tmr_active;     /* is timer active */
    TAILQ_ENTRY(ath_timer) tmr_elem;    /* timer list element */
} ath_timer_t;

typedef TAILQ_HEAD(ath_timerq_s, ath_timer) ath_timerq_t;

typedef struct ath_timer_list {
    struct timer_list   timer;              /* linux os timer */
    int                 timer_active : 1;   /* timer is active */
    ath_timerq_t        timer_q;            /* ath timer elements */
} ath_timer_list_t;


#define ATH_CHAINMASK_SEL_TIMEOUT	6000

/* Default - Number of last RSSI values that is used for chainmask 
   selection */
#define ATH_CHAINMASK_SEL_RSSI_CNT	10

/* Means use 3x3 chainmask instead of configured chainmask */
#define ATH_CHAINMASK_SEL_3X3		7

/* Default - Rssi threshold below which we have to switch to 3x3 */
#define ATH_CHAINMASK_SEL_UP_RSSI_THRES	 20

/* Default - Rssi threshold above which we have to switch to user 
   configured values */
#define ATH_CHAINMASK_SEL_DOWN_RSSI_THRES  35

/* Struct to store the chainmask select related info */

typedef struct ath_chainmask_sel {
    struct ath_timer	timer;
    int	   		cur_tx_mask; /* user configured or 3x3 */
    int	   		cur_rx_mask; /* user configured or 3x3 */
    int			tx_avgrssi;
    int			switch_allowed:1, /* timer will set this */
			cm_sel_enabled:1;
} ath_chainmask_sel_t;

#ifdef ATH_FORCE_PPM
/* Force ppm tracking hw workaround */
#define ATH_FORCE_PPM_PERIOD        1000    /* ms,  support timer period. Def 1000 */
#define ATH_FORCE_PPM_TIMEOUT       2       /* sec, between sample window updates */
#define ATH_FORCE_PPM_WD            20      /* sec, wd timeout for rx frames */
#define ATH_FORCE_PPM_TO            12288   /* us, recovery timeout.  Def 12288 (12 ms) */

enum {
    ST_FORCE_PPM_INIT   = 0,
    ST_FORCE_PPM_ARMED  = 1,
    ST_FORCE_PPM_SEARCH = 2,
    ST_FORCE_PPM_SCAN   = 3,
    ST_FORCE_PPM_IDLE   = 4,
    ST_FORCE_PPM_NEXT   = 5,
};

enum halPpmEventTypes {
    EV_PPM_INIT = 0,
    EV_PPM_STOP,
    EV_PPM_RUN,
};

typedef struct ath_force_ppm {
    struct ath_timer    timer;
    int                 timer_running;
    int         timerStart1;
    u_int32_t   timerCount1;
    u_int32_t   timerThrsh1;
    int32_t     timerStart2;
    u_int32_t   timerCount2;
    u_int32_t   timerThrsh2;
    u_int32_t   lastTsf1;
    u_int32_t   lastTsf2;
    u_int32_t   latchedRssi;
    int         forceState;
    HAL_BOOL    isRunning;
    HAL_BOOL    addrValid;
    u_int8_t    staAddr[IEEE80211_ADDR_LEN];
    // DEBUG
    int         dumpForcePpmFlag;
} ath_force_ppm_t;
#endif /* ATH_FORCE_PPM */

#define ATH_TID_MAX_BUFS    (2 * WME_MAX_BA)

/*
 * per TID aggregate tx state for a destination
 */
typedef struct ath_atx_tid {
    int                     tidno;      /* TID number                       */
    u_int16_t               seq_start;  /* starting seq of BA window        */
    u_int16_t               seq_next;   /* next seq to be used              */
    u_int16_t               baw_size;   /* BA window size                   */
    int                     baw_head;   /* first un-acked tx buffer         */
    int                     baw_tail;   /* next unused tx buffer slot       */
    int                     sched;      /* TID is scheduled                 */
    TAILQ_HEAD(ath_tid_bq,ath_buf)    buf_q;      /* pending buffers        */
    TAILQ_ENTRY(ath_atx_tid) tid_qelem; /* round-robin tid entry            */
    int                      filtered;  /* TID has filtered pkts            */
    TAILQ_HEAD(,ath_buf)     fltr_q;    /* filtered buffers                 */
    TAILQ_ENTRY(ath_atx_tid) fltr_qelem;/* handle hwq filtering             */
    struct ath_node         *an;        /* parent node structure            */
    struct ath_atx_ac       *ac;        /* parent access category           */
    struct ath_buf          *tx_buf[ATH_TID_MAX_BUFS];  /* active tx buffers*/
    /*
     * ADDBA state
     */
    int             	    addba_exchangecomplete;
    int                     addba_exchangeinprogress;
    struct ath_timer        addba_requesttimer;
    int                     addba_exchangeattempts;
    u_int16_t         	    addba_exchangestatuscode;
} ath_atx_tid_t;

/*
 * per access-category aggregate tx state for a destination
 */
typedef struct ath_atx_ac {
    int                     sched;      /* dest-ac is scheduled         */
    int                     priority;   /* access category priority     */
    int                     hwqcnt;     /* count of pkts on hw queue    */
    TAILQ_ENTRY(ath_atx_ac) ac_qelem;   /* round-robin txq entry        */
    TAILQ_HEAD(,ath_atx_tid)tid_q;      /* queue of TIDs with buffers   */
    int                     filtered;   /* ac is filtered               */
    TAILQ_ENTRY(ath_atx_ac) fltr_qelem; /* handle hwq filtering         */
    TAILQ_HEAD(,ath_atx_tid)fltr_q;     /* queue of TIDs being filtered */
    struct sk_buff          *ff_skb;    /* ff staging area              */
} ath_atx_ac_t;

/*
 * per dest tx state
 */
struct ath_atx {
    int                 hwqcnt;         /* count of pkts on hw queue    */
    struct ath_atx_tid  tid[WME_NUM_TID];
    struct ath_atx_ac   ac[WME_NUM_AC];
};

struct ath_rxbuf {
    struct sk_buff      *rx_skb;    /* buffer */
    unsigned long       rx_time;    /* jiffies when received */
    u_int32_t           rx_tsf;     /* receive tsf from mac */
    int                 rx_rssi;    /* rssi of received frame */
};

/*
 * per TID aggregate receiver state for a node
 */
struct ath_arx_tid {
    struct ath_node     *an;        /* parent ath node */
    u_int16_t           seq_next;   /* next expected sequence       */
    u_int16_t           baw_size;   /* block-ack window size        */
    int                 baw_head;   /* seq_next at head             */
    int                 baw_tail;   /* tail of block-ack window     */
    int                 seq_reset;  /* need to reset start sequence */
    int                 timer_act;  /* is timer active */
    struct ath_timer    timer;      /* timer element */
    struct ath_rxbuf    rxbuf[ ATH_TID_MAX_BUFS ];
    /*
     * ADDBA response information
     */
    u_int16_t                       dialogtoken;
    u_int16_t                       statuscode;
    struct ieee80211_ba_parameterset    baparamset;
    u_int16_t                       batimeout;
    int             	    	    addba_exchangecomplete;
};

/*
 * per node receiver aggregate state
 */
struct ath_arx {
    struct ath_arx_tid  tid[WME_NUM_TID];
};

typedef void (*ath_tx_queue_fn_t)(struct ath_softc *sc, struct ath_txq *txq,
                                  struct sk_buff *skb);
typedef void (*ath_tx_sched_fn_t)(struct ath_softc *sc,
                                  struct ath_atx_tid *tid);
typedef void (*ath_tx_comp_fn_t)(struct ath_softc *sc, struct ath_buf *bf);
typedef void (*ath_tx_seqno_fn_t)(struct ath_buf *bf);
typedef void (*ath_txq_add_fn_t)(struct ath_softc *sc, struct ath_buf *bf);
typedef int  (*ath_input_fn_t)(struct ieee80211_node *ni, struct sk_buff *skb,
                               int rssi, u_int32_t rstamp);

struct ath_node_aggr {
    struct ath_atx      tx;         /* node transmit state          */
    struct ath_arx      rx;         /* node receive state           */
    int                 isap;       /* is ap                        */
    ath_tx_queue_fn_t   tx_queue;   /* tx queueing function         */
    ath_tx_sched_fn_t   tx_sched;   /* tx scheduling                */
    ath_tx_comp_fn_t    tx_comp;    /* tx completion                */
    ath_tx_seqno_fn_t   tx_seqno;   /* tx sequence number           */
    ath_input_fn_t      rx_input;   /* rx processing function       */
};
#define an_tx_queue     an_aggr.tx_queue
#define an_tx_sched     an_aggr.tx_sched
#define an_tx_comp      an_aggr.tx_comp
#define an_tx_seqno     an_aggr.tx_seqno
#define an_tx_tid       an_aggr.tx.tid
#define an_tx_ac        an_aggr.tx.ac
#define an_rx_tid       an_aggr.rx.tid
#define an_rx_input     an_aggr.rx_input
#define an_isap         an_aggr.isap
#define an_tx_hwqcnt    an_aggr.tx.hwqcnt

struct ath_buf_state {
    ath_tx_comp_fn_t        bfs_comp;   /* completion function          */
    ath_txq_add_fn_t        bfs_txq_add;/* txq buffer add function      */
    int                     bfs_pktlen; /* pktlen including crc         */
    int                     bfs_hdrlen; /* header length                */
    int                     bfs_nfl;    /* next fragment length         */
    int                     bfs_keyix;  /* key index                    */
    HAL_PKT_TYPE            bfs_atype;  /* packet type                  */
    int                     bfs_seqno;  /* sequence nuber               */
    int                     bfs_ndelim; /* # delims for padding         */
    int                     bfs_nframes;/* # frames in aggregate        */
    int                     bfs_tidno;  /* tid of the buffer            */
    u_int16_t               bfs_al;     /* length of aggregate          */
    u_int16_t               bfs_pktdur; /* packet duration              */
    const HAL_RATE_TABLE    *bfs_rt;    /* rate table                   */
    struct ath_rc_series    bfs_rcs[4]; /* rate series                  */
    u_int8_t                bfs_ft;     /* frame type                   */
    struct ath_txq          *bfs_txq;   /* transmit h/w queue           */
    enum ieee80211_protmode bfs_protmode;   /* protection mode          */
    HAL_KEY_TYPE            bfs_keytype;    /* encr key type            */
    int                     bfs_retries;        /* current retries      */
    int                     bfs_istxfrag : 1;   /* is fragemented       */
    int                     bfs_ismcast  : 1;   /* is multicast         */
    int                     bfs_shpream  : 1;   /* use short preamble   */
    int                     bfs_isaggr   : 1;   /* is an aggregate      */
    int                     bfs_isretried: 1;   /* is retried           */
};
#define bf_comp     bf_state.bfs_comp
#define bf_txq_add  bf_state.bfs_txq_add
#define bf_pktlen   bf_state.bfs_pktlen
#define bf_hdrlen   bf_state.bfs_hdrlen
#define bf_nfl      bf_state.bfs_nfl
#define bf_keyix    bf_state.bfs_keyix
#define bf_atype    bf_state.bfs_atype
#define bf_seqno    bf_state.bfs_seqno
#define bf_ndelim   bf_state.bfs_ndelim
#define bf_nframes  bf_state.bfs_nframes
#define bf_al       bf_state.bfs_al
#define bf_tidno    bf_state.bfs_tidno
#define bf_pktdur   bf_state.bfs_pktdur
#define bf_rt       bf_state.bfs_rt
#define bf_rcs      bf_state.bfs_rcs
#define bf_ft       bf_state.bfs_ft
#define bf_txq      bf_state.bfs_txq
#define bf_protmode bf_state.bfs_protmode
#define bf_keytype  bf_state.bfs_keytype
#define bf_istxfrag bf_state.bfs_istxfrag
#define bf_ismcast  bf_state.bfs_ismcast
#define bf_shpream  bf_state.bfs_shpream
#define bf_isaggr   bf_state.bfs_isaggr
#define bf_isretried    bf_state.bfs_isretried
#define bf_retries  bf_state.bfs_retries

/* driver-specific node state */
struct ath_node {
    struct ieee80211_node an_node;  /* base class */
    u_int16_t   an_decomp_index; /* decompression mask index */
    u_int32_t   an_avgrssi; /* average rssi over all rx frames */
    u_int8_t    an_prevdatarix; /* rate ix of last data frame */
    u_int16_t   an_minffrate;   /* mimum rate in kbps for ff to aggregate */
    HAL_NODE_STATS  an_halstats;    /* rssi statistics used by hal */
    struct ath_buf  *an_tx_ffbuf[WME_NUM_AC]; /* ff staging area */
        struct ath_node_aggr    an_aggr;        /* a-mpdu aggr state */
    /* variable-length rate control state follows */
};
#define ATH_NODE(_n)    ((struct ath_node *)(_n))
#define ATH_NODE_CONST(ni)  ((const struct ath_node *)(ni))
#define ATH_NODE_IS_11N(_an)        ((_an)->an_tx_queue == ath_tx_queue_aggr)

#define ATH_RSSI_LPF_LEN    10
#define ATH_RSSI_DUMMY_MARKER   0x127
#define ATH_EP_RND(x, mul)  ((((x)%(mul)) >= ((mul)/2))?((x) + ((mul) - 1))/(mul):(x)/(mul))
#define ATH_EP_MUL(x, mul)  ((x) * (mul))
#define ATH_RSSI_OUT(x)	    (ATH_EP_RND((x), HAL_RSSI_EP_MULTIPLIER))
#define ATH_RSSI_IN(x)      (ATH_EP_MUL((x), HAL_RSSI_EP_MULTIPLIER))
#define ATH_LPF_RSSI(x, y, len) \
    ((x != ATH_RSSI_DUMMY_MARKER) ? (((x) * ((len) - 1) + (y)) / (len)) : (y))
#define ATH_RSSI_LPF(x, y) do {                     \
    if ((y) >= -20)                         \
        x = ATH_LPF_RSSI((x), ATH_RSSI_IN((y)), ATH_RSSI_LPF_LEN);  \
} while (0)

struct ath_buf {
    TAILQ_ENTRY(ath_buf)    bf_list;
        struct ath_buf      *bf_next;   /* next buffer in the aggregate */
    struct ath_desc     *bf_desc;   /* virtual addr of desc */
    struct ath_desc     *bf_descarr;   /* virtual addr of desc */
    int             bf_descno;  /* beginning index of desc used */
    dma_addr_t      bf_daddr;   /* physical addr of desc */
    dma_addr_t      bf_daddrs[ATH_TXDESC];    /* physical addr of additional descs */
    struct sk_buff      *bf_skb;    /* skbuff for buf */
    dma_addr_t      bf_skbaddr; /* physical addr of skb data */
    dma_addr_t      bf_skbaddrs[ATH_TXDESC-1];  /* physical addr of additional skb data */
    struct ieee80211_node   *bf_node;   /* pointer to the node */
    struct ath_desc     *bf_lastds; /* last desc for comp status */
    u_int32_t       bf_queueage; /* "age" of txq when this buffer placed on stageq */
    u_int16_t       bf_numdesc; /* number of descs used */
    u_int16_t       bf_flags;   /* tx descriptor flags */
    struct ath_buf_state    bf_state;   /* buf tx state */
};
typedef TAILQ_HEAD(ath_bufhead_s, ath_buf)  ath_bufhead;

/*
 * DMA state for tx/rx descriptors.
 */
struct ath_descdma {
    const char*     dd_name;
    struct ath_desc     *dd_desc;   /* descriptors */
    dma_addr_t      dd_desc_paddr;  /* physical addr of dd_desc */
    size_t          dd_desc_len;    /* size of dd_desc */
    struct ath_buf      *dd_bufptr; /* associated buffers */
};

#ifdef ATH_TX99_DIAG
struct ath_txrx99 {
    u_int32_t       tx99mode;         /* tx99 mode  */
    u_int32_t       prefetch;         /* prefetch   */
    u_int32_t       txpower;          /* tx power   */
    u_int32_t       txrate;           /* tx rate    */
    u_int32_t       rx99mode;         /* rx99 mode  */
    void            *prev_hard_start;
    void            *prev_mgt_start;
    u_int32_t       imask;
};
#endif

struct ath_hal;
struct ath_desc;
struct ath_ratectrl;
struct proc_dir_entry;

/*
 * Data transmit queue state.  One of these exists for each
 * hardware transmit queue.  Packets sent to us from above
 * are assigned to queues based on their priority.  Not all
 * devices support a complete set of hardware transmit queues.
 * For those devices the array sc_ac2q will map multiple
 * priorities to fewer hardware queues (typically all to one
 * hardware queue).
 */
struct ath_txq {
    u_int           axq_qnum;   /* hardware q number */
    u_int32_t       *axq_link;  /* link ptr in last TX desc */
    TAILQ_HEAD(,ath_buf) axq_q;     /* transmit queue */
    spinlock_t      axq_lock;   /* lock on q and link */
    u_int           axq_depth;  /* queue depth */
    u_int32_t       axq_totalqueued;/* total ever queued */
    u_int           axq_intrcnt;    /* count to determine if descriptor
                         * should generate int on this txq.
                         */
    /*
     * State for patching up CTS when bursting.
     */
    struct  ath_buf     *axq_linkbuf;   /* virtual addr of last buffer*/
    struct  ath_desc    *axq_lastdsWithCTS; /* first desc of the last descriptor
                             * that contains CTS
                             */
    struct  ath_desc    *axq_gatingds;  /* final desc of the gating desc
                         * that determines whether lastdsWithCTS has
                         * been DMA'ed or not
                         */
    /*
     * Staging queue for frames awaiting a fast-frame pairing.
     */
    TAILQ_HEAD(axq_headtype, ath_buf) axq_stageq;
    TAILQ_HEAD(,ath_atx_ac) axq_acq;
    TAILQ_HEAD(,ath_atx_ac) axq_fltrq;

        /* scratch compression buffer */
        char                    *axq_compbuf;   /* scratch comp buffer */
        dma_addr_t              axq_compbufp;  /* scratch comp buffer (phys)*/
        u_int                   axq_compbufsz; /* scratch comp buffer size */
};

/* driver-specific vap state */
struct ath_vap {
    struct ieee80211vap av_vap; /* base class */
    int     (*av_newstate)(struct ieee80211vap *,
                    enum ieee80211_state, int);
    /* XXX beacon state */
    struct ath_buf  *av_bcbuf;  /* beacon buffer */
    struct ieee80211_beacon_offsets av_boff;/* dynamic update state */
    int     av_bslot;   /* beacon slot index */
    struct ath_txq      av_mcastq; /* multicast transmit queue */
};
#define ATH_VAP(_v) ((struct ath_vap *)(_v))

#define ATH_BEACON_AIFS_DEFAULT     1  /* Default aifs for ap beacon q */
#define ATH_BEACON_CWMIN_DEFAULT    0  /* Default cwmin for ap beacon q */
#define ATH_BEACON_CWMAX_DEFAULT    0  /* Default cwmax for ap beacon q */

#define ATH_TXQ_INTR_PERIOD     5  /* axq_intrcnt period for intr gen */
#define ATH_TXQ_LOCK_INIT(_tq)      spin_lock_init(&(_tq)->axq_lock)
#define ATH_TXQ_LOCK_DESTROY(_tq)
#define ATH_TXQ_LOCK(_tq)       spin_lock(&(_tq)->axq_lock)
#define ATH_TXQ_UNLOCK(_tq)     spin_unlock(&(_tq)->axq_lock)
#define ATH_TXQ_LOCK_BH(_tq)        spin_lock_bh(&(_tq)->axq_lock)
#define ATH_TXQ_UNLOCK_BH(_tq)      spin_unlock_bh(&(_tq)->axq_lock)
#define ATH_TXQ_LOCK_ASSERT(_tq) \
    KASSERT(spin_is_locked(&(_tq)->axq_lock), ("txq not locked!"))
#define ATH_TXQ_INSERT_TAIL(_tq, _elm, _field) do { \
    TAILQ_INSERT_TAIL( &(_tq)->axq_q, (_elm), _field); \
    (_tq)->axq_depth++; \
    (_tq)->axq_totalqueued++; \
    (_tq)->axq_linkbuf = (_elm); \
} while (0)
#define ATH_TXQ_REMOVE_HEAD(_tq, _elm, _field) do { \
    TAILQ_REMOVE(&(_tq)->axq_q, (_elm), _field); \
    (_tq)->axq_depth--; \
} while (0)
/* move buffers from MCASTQ to CABQ */
#define ATH_TXQ_MOVE_MCASTQ(_tqs,_tqd) do { \
    (_tqd)->axq_depth += (_tqs)->axq_depth; \
    (_tqd)->axq_totalqueued += (_tqs)->axq_totalqueued; \
    (_tqd)->axq_linkbuf = (_tqs)->axq_linkbuf ; \
    (_tqd)->axq_link = (_tqs)->axq_link; \
    TAILQ_CONCAT(&(_tqd)->axq_q,&(_tqs)->axq_q, bf_list); \
    (_tqs)->axq_depth=0; \
    (_tqs)->axq_totalqueued = 0; \
    (_tqs)->axq_linkbuf = 0; \
    (_tqs)->axq_link = NULL; \
} while (0)

/*
 * concat buffers from one queue to other
 */
#define ATH_TXQ_MOVE_Q(_tqs,_tqd)  ATH_TXQ_MOVE_MCASTQ(_tqs,_tqd)

#define BSTUCK_THRESH   9   /* # of stuck beacons before resetting */

struct ath_softc {
    struct ieee80211com sc_ic;      /* NB: must be first */
    struct net_device*  sc_dev;
    struct semaphore    sc_lock;    /* dev-level lock */
    struct net_device_stats sc_devstats;    /* device statistics */
    struct ath_stats    sc_stats;   /* private statistics */
    int    noampdutrc;
    int         sc_debug;
    void            (*sc_recv_mgmt)(struct ieee80211_node *,
                    struct sk_buff *, int, int, u_int32_t);
    void            (*sc_node_free)(struct ieee80211_node *);
    void            *sc_bdev;   /* associated bus device */
    struct ath_hal      *sc_ah;     /* Atheros HAL */
    struct ath_ratectrl *sc_rc;     /* tx rate control support */
    void            (*sc_setdefantenna)(struct ath_softc *, u_int);
    unsigned int        sc_invalid : 1, /* being detached */
                sc_softled : 1, /* enable LED gpio status */
                sc_splitmic: 1, /* split TKIP MIC keys */
                sc_needmib : 1, /* enable MIB stats intr */
                sc_hasdiversity : 1,/* rx diversity available */
                sc_diversity : 1, /* enable rx diversity */
                sc_hasveol : 1, /* tx VEOL support */
                sc_hastpc  : 1, /* per-packet TPC support */
                sc_dturbo  : 1, /* dynamic turbo capable */
                sc_dturbo_switch: 1,/* turbo switch mode*/
                sc_ignore_ar: 1,/* ignore AR during transision*/
                sc_ledstate: 1, /* LED on/off state */
                sc_blinking: 1, /* LED blink operation active */
                sc_beacons : 1, /* beacons running */
                sc_hasbmask: 1, /* bssid mask support */
                sc_mcastkey: 1, /* mcast key cache search */
                sc_hastsfadd:1, /* tsf adjust support */
                sc_scanning: 1, /* scanning active */
                sc_nostabeacons: 1, /* no beacons used for station */
                sc_xrgrppoll: 1,/* xr group polls are active */
                sc_syncbeacon:1,/* sync/resync beacon timers */
                sc_hasclrkey: 1,/* CLR key supported */
                sc_devstopped:1;/* device stopped because of no tx bufs */
                        /* rate tables */
    const HAL_RATE_TABLE    *sc_rates[IEEE80211_MODE_MAX];
    const HAL_RATE_TABLE    *sc_currates;   /* current rate table */
    const HAL_RATE_TABLE    *sc_xr_rates; /* XR rate table */
    const HAL_RATE_TABLE    *sc_half_rates; /* half rate table */
    const HAL_RATE_TABLE    *sc_quarter_rates; /* quarter rate table */
    enum ieee80211_phymode  sc_curmode; /* current phy mode */
    u_int16_t       sc_curtxpow;    /* current tx power limit */
    u_int16_t       sc_curaid;  /* current association id */
    HAL_CHANNEL     sc_curchan; /* current h/w channel */
    u_int8_t        sc_curbssid[IEEE80211_ADDR_LEN];
    u_int8_t        sc_rixmap[256]; /* IEEE to h/w rate table ix */
    u_int8_t        sc_htrixmap[256];   /* IEEE to h/w rate table ix */
    struct {
        u_int8_t    ieeerate;   /* IEEE rate */
        u_int8_t    flags;      /* radiotap flags */
        u_int16_t   ledon;      /* softled on time */
        u_int16_t   ledoff;     /* softled off time */
    } sc_hwmap[32];             /* h/w rate ix mappings */
    struct {
        u_int8_t    ieeerate;   /* IEEE rate */
        u_int8_t    flags;      /* radiotap flags */
        u_int16_t   ledon;      /* softled on time */
        u_int16_t   ledoff;     /* softled off time */
    } sc_hthwmap[32];           /* h/w rate ix mappings */
    u_int8_t        sc_minrateix;   /* min h/w rate index */
    u_int8_t        sc_protrix; /* protection rate index */
    u_int8_t        sc_txantenna;   /* tx antenna (fixed or auto) */
    u_int16_t       sc_nvaps;   /* # of active virtual ap's */
    u_int16_t       sc_nstavaps;    /* # of active station vaps */
    u_int           sc_fftxqmin;    /* aggregation threshold */
    HAL_INT         sc_imask;   /* interrupt mask copy */
    u_int           sc_keymax;  /* size of key cache */
    u_int8_t        sc_keymap[ATH_KEYBYTES];/* key use bit map */
    struct ieee80211_node   *sc_keyixmap[ATH_KEYMAX];/* key ix->node map */
    u_int8_t        sc_bssidmask[IEEE80211_ADDR_LEN];

    u_int           sc_ledpin;  /* GPIO pin for driving LED */
    u_int           sc_ledon;   /* pin setting for LED on */
    u_int           sc_ledidle; /* idle polling interval */
    int         sc_ledevent;    /* time of last LED event */
    u_int8_t        sc_rxrate;  /* current rx rate for LED */
    u_int8_t        sc_txrate;  /* current tx rate for LED */
    u_int16_t       sc_ledoff;  /* off time for current blink */
    struct timer_list   sc_ledtimer;    /* led off timer */

    struct tq_struct    sc_fataltq; /* fatal error intr tasklet */

    int         sc_rxbufsize;   /* rx size based on mtu */
    struct ath_descdma  sc_rxdma;   /* RX descriptors */
    ath_bufhead     sc_rxbuf;   /* receive buffer */
    struct ath_buf  *sc_rxbuf_held;   /* receive buffer to be posted next */
    u_int32_t       *sc_rxlink; /* link ptr in last RX desc */
    struct tq_struct    sc_rxtq;    /* rx intr tasklet */
    struct tq_struct    sc_rxorntq; /* rxorn intr tasklet */
    u_int8_t        sc_defant;  /* current default antenna */
    u_int8_t        sc_rxotherant;  /* rx's on non-default antenna*/
    u_int16_t       sc_cachelsz;    /* cache line size */

    struct ath_descdma  sc_txdma;   /* TX descriptors */
    ath_bufhead     sc_txbuf;   /* transmit buffer */
    spinlock_t      sc_txbuflock;   /* txbuf lock */
    u_int           sc_txqsetup;    /* h/w queues setup */
    u_int           sc_txintrperiod;/* tx interrupt batching */
    struct ath_txq      sc_txq[HAL_NUM_TX_QUEUES];
    struct ath_txq      *sc_ac2q[WME_NUM_AC];   /* WME AC -> h/w qnum */
    struct tq_struct    sc_txtq;    /* tx intr tasklet */
    u_int8_t        sc_grppoll_str[GRPPOLL_RATE_STR_LEN];
    struct ath_descdma  sc_bdma;    /* beacon descriptors */
    ath_bufhead     sc_bbuf;    /* beacon buffers */
    u_int           sc_bhalq;   /* HAL q for outgoing beacons */
    u_int           sc_bmisscount;  /* missed beacon transmits */
    u_int32_t       sc_ant_tx[8];   /* recent tx frames/antenna */
    struct ath_txq      *sc_cabq;   /* tx q for cab frames */
    struct ath_txq      sc_grpplq;  /* tx q for XR group polls */
    struct ath_txq      *sc_xrtxq;  /* tx q for XR data */
    struct ath_descdma  sc_grppolldma;  /* TX descriptors for grppoll */
    ath_bufhead     sc_grppollbuf;  /* transmit buffers for grouppoll  */
    u_int16_t       sc_xrpollint;   /* xr poll interval */
    u_int16_t       sc_xrpollcount; /* xr poll count */
    struct tq_struct    sc_bmisstq; /* bmiss intr tasklet */
    struct tq_struct    sc_bstucktq;    /* beacon stuck intr tasklet */
    struct tq_struct    sc_txtotq;  /* tx timeout tasklet */
    enum {
        OK,             /* no change needed */
        UPDATE,             /* update pending */
        COMMIT              /* beacon sent, commit change */
    } sc_updateslot;            /* slot time update fsm */
    struct ieee80211vap *sc_bslot[ATH_BCBUF];/* beacon xmit slots */
    int         sc_bnext;   /* next slot for beacon xmit */

    struct timer_list   sc_cal_ch;  /* calibration timer */
    HAL_NODE_STATS      sc_halstats;    /* station-mode rssi stats */
#ifdef CONFIG_SYSCTL
    struct ctl_table_header *sc_sysctl_header;
    struct ctl_table    *sc_sysctls;
#endif

    u_int16_t       sc_reapcount;  /* # of tx buffers reaped after net dev stopped */

#ifdef ATH_SUPERG_DYNTURBO
    struct timer_list   sc_dturbo_switch_mode;  /* AP scan timer */
    u_int32_t               sc_dturbo_tcount;     /* beacon intval count */
    u_int32_t               sc_dturbo_bytes;      /* bandwidth stats */
    u_int32_t               sc_dturbo_base_tmin;  /* min time in base */
    u_int32_t               sc_dturbo_turbo_tmax; /* max time in turbo */
    u_int32_t               sc_dturbo_bw_base;    /* bandwidth threshold */
    u_int32_t               sc_dturbo_bw_turbo;   /* bandwidth threshold */
#endif

#ifdef ATH_CWM
    struct ath_cwm      *sc_cwm;        /* Channel Width Management */
    HAL_HT      sc_currht;  /* current hardware HT configuration */
#endif

#ifdef ATH_CHAINMASK_SELECT
    ath_chainmask_sel_t  sc_chainmask_sel;
#endif

#ifdef ATH_FORCE_PPM
    ath_force_ppm_t  sc_force_ppm;  /* force ppm tracking workaround */
#endif

#ifdef ATH_TX99_DIAG
    struct ath_txrx99   sc_txrx99;
#endif
    u_int                   sc_chainsel;          /* chain selection    */
    struct ath_timer_list   sc_timer;
    struct ath_timer        sc_watchdog;
    u_int32_t               sc_mrretries;         /* Number of Mulirate retries*/
#ifdef DEBUG_PKTLOG
    struct ath_pktlog_info *pl_info;
#endif
};

typedef void (*ath_callback) (struct ath_softc *);
#define ATH_TXQ_SETUP(sc, i)    ((sc)->sc_txqsetup & (1<<i))

#define ATH_TXBUF_LOCK_INIT(_sc)    spin_lock_init(&(_sc)->sc_txbuflock)
#define ATH_TXBUF_LOCK_DESTROY(_sc)
#define ATH_TXBUF_LOCK(_sc)     spin_lock(&(_sc)->sc_txbuflock)
#define ATH_TXBUF_UNLOCK(_sc)       spin_unlock(&(_sc)->sc_txbuflock)
#define ATH_TXBUF_LOCK_BH(_sc)      spin_lock_bh(&(_sc)->sc_txbuflock)
#define ATH_TXBUF_UNLOCK_BH(_sc)    spin_unlock_bh(&(_sc)->sc_txbuflock)
#define ATH_TXBUF_LOCK_ASSERT(_sc) \
    KASSERT(spin_is_locked(&(_sc)->sc_txbuflock), ("txbuf not locked!"))

#define ATH_LOCK_INIT(_sc)      init_MUTEX(&(_sc)->sc_lock)
#define ATH_LOCK_DESTROY(_sc)
#define ATH_LOCK(_sc)           down(&(_sc)->sc_lock)
#define ATH_UNLOCK(_sc)         up(&(_sc)->sc_lock)

/*
 * new interfaces for 20/40 transitions
 */
#define ATH_NODE_HWQ_ACTIVE(_ni)        (ATH_NODE(_ni)->an_tx_hwqcnt)
#define ATH_NODE_AC_ACTIVE(_ni, _ac)    (ATH_NODE(_ni)->an_tx_ac[_ac].hwqcnt)

int ath_attach(u_int16_t, struct net_device *);
int ath_detach(struct net_device *);
void    ath_resume(struct net_device *);
void    ath_suspend(struct net_device *);
void    ath_shutdown(struct net_device *);
irqreturn_t ath_intr(int irq, void *dev_id, struct pt_regs *regs);
#ifdef CONFIG_SYSCTL
void    ath_sysctl_register(void);
void    ath_sysctl_unregister(void);
#endif /* CONFIG_SYSCTL */

enum {
    ATH_LED_TX,
    ATH_LED_RX,
    ATH_LED_POLL,
};

void ath_led_event(struct ath_softc *sc, int event);
int txqactive(struct ath_hal *ah, int qnum);
int  ath_chan_set(struct ath_softc *, struct ieee80211_channel *);
void ath_stoprecv(struct ath_softc *);
int  ath_startrecv(struct ath_softc *);

/*
 * owl specific functions
 */
typedef enum {
    OWL_TXQ_ACTIVE = 0,
    OWL_TXQ_STOPPED,
    OWL_TXQ_FILTERED,
} owl_txq_state_t;

int owl_hardstart(struct sk_buff *, struct net_device *);
void owl_tx_tasklet(TQUEUE_ARG data);
int owl_input(struct ieee80211_node *ni, struct sk_buff *skb,
              struct ath_rx_status *);
void owl_tx_processq(struct ath_softc *sc, struct ath_txq *txq,
                            owl_txq_state_t txqstate);
void owl_tx_requeue(struct ath_softc *sc, struct ath_txq *txq);
void owl_txq_drain(struct ath_softc *sc, struct ath_txq *txq);
void owl_node_init(struct ath_softc *sc, struct ath_node *an);
void ath_tx_queue_aggr(struct ath_softc *sc, struct ath_txq *txq,
                       struct sk_buff *skb);

/*
 * ADDBA functions
 */
void ath_aggr_addba_requestsetup(struct ieee80211com *, struct ieee80211_node *, u_int8_t tidno,
                    struct ieee80211_ba_parameterset    *baparamset,
                    u_int16_t                       *batimeout,
                    struct ieee80211_ba_seqctrl     *basequencectrl,
                    u_int16_t                       buffersize);
void ath_aggr_addba_requestprocess(struct ieee80211com *ic, struct ieee80211_node *ni,
                    u_int8_t                        dialogtoken,
                    struct ieee80211_ba_parameterset    *baparamset,
                    u_int16_t                       batimeout,
                    struct ieee80211_ba_seqctrl         basequencectrl);
void ath_aggr_addba_responsesetup(struct ieee80211com *ic, struct ieee80211_node *ni,
            u_int8_t                tidno,
            u_int8_t                        *dialogtoken,
            u_int16_t                       *statuscode,
            struct ieee80211_ba_parameterset        *baparamset,
            u_int16_t                       *batimeout);
void ath_aggr_addba_responseprocess(struct ieee80211com *ic, struct ieee80211_node *ni,
                            u_int16_t                       statuscode,
                            struct ieee80211_ba_parameterset    *baparamset,
                            u_int16_t                       batimeout);
void ath_aggr_delba_process(struct ieee80211com *ic, struct ieee80211_node *ni,
                            struct ieee80211_delba_parameterset *delbaparamset,
                            u_int16_t                       reasoncode);
void ath_aggr_addba_clear(struct ath_node *an);
int  ath_aggr_addba_send(struct ieee80211com *ic, struct ieee80211_node *ni, u_int8_t ac,
                    u_int16_t buffersize);
void ath_aggr_delba_send(struct ieee80211com *ic, struct ieee80211_node *ni,
                    u_int8_t ac, u_int8_t initiator, u_int16_t reasoncode);
void ath_aggr_addba_status(struct ieee80211com *ic, struct ieee80211_node *ni,
                    u_int8_t ac, u_int16_t *status);

/*
 * new timer functions
 */
void ath_timer_init(struct ath_softc *sc);
void ath_timer_exit(struct ath_softc *sc);
void ath_timer_set(struct ath_timer *tmr, ath_timer_func_t tmr_func,
                   void *tmr_arg, int tmr_ticks);
void ath_timer_add(struct ath_softc *sc, struct ath_timer *tmr);
void ath_timer_del(struct ath_softc *sc, struct ath_timer *tmr);

/*
 * HAL definitions to comply with local coding convention.
 */
#define ath_hal_reset(_ah, _opmode, _chan, _outdoor, _pstatus) \
    ((*(_ah)->ah_reset)((_ah), (_opmode), (_chan), (_outdoor), (_pstatus)))
#define ath_hal_getratetable(_ah, _mode) \
    ((*(_ah)->ah_getRateTable)((_ah), (_mode)))
#define ath_hal_getmac(_ah, _mac) \
    ((*(_ah)->ah_getMacAddress)((_ah), (_mac)))
#define ath_hal_setmac(_ah, _mac) \
    ((*(_ah)->ah_setMacAddress)((_ah), (_mac)))
#define ath_hal_getbssidmask(_ah, _mask) \
    ((*(_ah)->ah_getBssIdMask)((_ah), (_mask)))
#define ath_hal_setbssidmask(_ah, _mask) \
    ((*(_ah)->ah_setBssIdMask)((_ah), (_mask)))
#define ath_hal_intrset(_ah, _mask) \
    ((*(_ah)->ah_setInterrupts)((_ah), (_mask)))
#define ath_hal_intrget(_ah) \
    ((*(_ah)->ah_getInterrupts)((_ah)))
#define ath_hal_intrpend(_ah) \
    ((*(_ah)->ah_isInterruptPending)((_ah)))
#define ath_hal_getisr(_ah, _pmask) \
    ((*(_ah)->ah_getPendingInterrupts)((_ah), (_pmask)))
#define ath_hal_updatetxtriglevel(_ah, _inc) \
    ((*(_ah)->ah_updateTxTrigLevel)((_ah), (_inc)))
#define ath_hal_setpower(_ah, _mode) \
    ((*(_ah)->ah_setPowerMode)((_ah), (_mode), AH_TRUE))
#define ath_hal_keycachesize(_ah) \
    ((*(_ah)->ah_getKeyCacheSize)((_ah)))
#define ath_hal_keyreset(_ah, _ix) \
    ((*(_ah)->ah_resetKeyCacheEntry)((_ah), (_ix)))
#define ath_hal_keyset(_ah, _ix, _pk, _mac) \
    ((*(_ah)->ah_setKeyCacheEntry)((_ah), (_ix), (_pk), (_mac), AH_FALSE))
#define ath_hal_keyisvalid(_ah, _ix) \
    (((*(_ah)->ah_isKeyCacheEntryValid)((_ah), (_ix))))
#define ath_hal_keysetmac(_ah, _ix, _mac) \
    ((*(_ah)->ah_setKeyCacheEntryMac)((_ah), (_ix), (_mac)))
#define ath_hal_getrxfilter(_ah) \
    ((*(_ah)->ah_getRxFilter)((_ah)))
#define ath_hal_setrxfilter(_ah, _filter) \
    ((*(_ah)->ah_setRxFilter)((_ah), (_filter)))
#define ath_hal_setmcastfilter(_ah, _mfilt0, _mfilt1) \
    ((*(_ah)->ah_setMulticastFilter)((_ah), (_mfilt0), (_mfilt1)))
#define ath_hal_waitforbeacon(_ah, _bf) \
    ((*(_ah)->ah_waitForBeaconDone)((_ah), (_bf)->bf_daddr))
#define ath_hal_putrxbuf(_ah, _bufaddr) \
    ((*(_ah)->ah_setRxDP)((_ah), (_bufaddr)))
#define ath_hal_gettsf32(_ah) \
    ((*(_ah)->ah_getTsf32)((_ah)))
#define ath_hal_gettsf64(_ah) \
    ((*(_ah)->ah_getTsf64)((_ah)))
#define ath_hal_resettsf(_ah) \
    ((*(_ah)->ah_resetTsf)((_ah)))
#define ath_hal_rxena(_ah) \
    ((*(_ah)->ah_enableReceive)((_ah)))
#ifdef ATH_FORCE_PPM
#define ath_hal_ppmGetRssiDump(_ah) \
    ((*(_ah)->ah_ppmGetRssiDump)((_ah)))
#define ath_hal_ppmArmTrigger(_ah) \
    ((*(_ah)->ah_ppmArmTrigger)((_ah)))
#define ath_hal_ppmGetTrigger(_ah) \
    ((*(_ah)->ah_ppmGetTrigger)((_ah)))
#define ath_hal_ppmForce(_ah) \
    ((*(_ah)->ah_ppmForce)((_ah)))
#define ath_hal_ppmUnForce(_ah) \
    ((*(_ah)->ah_ppmUnForce)((_ah)))
#define ath_hal_ppmGetReg(_ah, _arg) \
    ((*(_ah)->ah_ppmGetReg)((_ah), (_arg)))
#endif /* ATH_FORCE_PPM */
#define ath_hal_numtxpending(_ah, _q) \
    ((*(_ah)->ah_numTxPending)((_ah), (_q)))
#define ath_hal_puttxbuf(_ah, _q, _bufaddr) \
    ((*(_ah)->ah_setTxDP)((_ah), (_q), (_bufaddr)))
#define ath_hal_gettxbuf(_ah, _q) \
    ((*(_ah)->ah_getTxDP)((_ah), (_q)))
#define ath_hal_getrxbuf(_ah) \
    ((*(_ah)->ah_getRxDP)((_ah)))
#define ath_hal_txstart(_ah, _q) \
    ((*(_ah)->ah_startTxDma)((_ah), (_q)))
#define ath_hal_setchannel(_ah, _chan) \
    ((*(_ah)->ah_setChannel)((_ah), (_chan)))
#define ath_hal_calibrate(_ah, _chan) \
    ((*(_ah)->ah_perCalibration)((_ah), (_chan)))
#define ath_hal_setledstate(_ah, _state) \
    ((*(_ah)->ah_setLedState)((_ah), (_state)))
#define ath_hal_beaconinit(_ah, _nextb, _bperiod) \
    ((*(_ah)->ah_beaconInit)((_ah), (_nextb), (_bperiod)))
#define ath_hal_beaconreset(_ah) \
    ((*(_ah)->ah_resetStationBeaconTimers)((_ah)))
#define ath_hal_beacontimers(_ah, _bs) \
    ((*(_ah)->ah_setStationBeaconTimers)((_ah), (_bs)))
#define ath_hal_setassocid(_ah, _bss, _associd) \
    ((*(_ah)->ah_writeAssocid)((_ah), (_bss), (_associd)))
#define ath_hal_phydisable(_ah) \
    ((*(_ah)->ah_phyDisable)((_ah)))
#define ath_hal_setopmode(_ah) \
    ((*(_ah)->ah_setPCUConfig)((_ah)))
#define ath_hal_stoptxdma(_ah, _qnum) \
    ((*(_ah)->ah_stopTxDma)((_ah), (_qnum)))
#define ath_hal_aborttxdma(_ah) \
    ((*(_ah)->ah_abortTxDma)(_ah))
#define ath_hal_stoppcurecv(_ah) \
    ((*(_ah)->ah_stopPcuReceive)((_ah)))
#define ath_hal_abortpcurecv(_ah) \
    ((*(_ah)->ah_abortPcuReceive)((_ah)))
#define ath_hal_startpcurecv(_ah) \
    ((*(_ah)->ah_startPcuReceive)((_ah)))
#define ath_hal_stopdmarecv(_ah) \
    ((*(_ah)->ah_stopDmaReceive)((_ah)))
#define ath_hal_getdiagstate(_ah, _id, _indata, _insize, _outdata, _outsize) \
    ((*(_ah)->ah_getDiagState)((_ah), (_id), \
        (_indata), (_insize), (_outdata), (_outsize)))
#define ath_hal_gettxqueueprops(_ah, _q, _qi) \
    ((*(_ah)->ah_getTxQueueProps)((_ah), (_q), (_qi)))
#define ath_hal_settxqueueprops(_ah, _q, _qi) \
    ((*(_ah)->ah_setTxQueueProps)((_ah), (_q), (_qi)))
#define ath_hal_setuptxqueue(_ah, _type, _irq) \
    ((*(_ah)->ah_setupTxQueue)((_ah), (_type), (_irq)))
#define ath_hal_resettxqueue(_ah, _q) \
    ((*(_ah)->ah_resetTxQueue)((_ah), (_q)))
#define ath_hal_releasetxqueue(_ah, _q) \
    ((*(_ah)->ah_releaseTxQueue)((_ah), (_q)))
#define ath_hal_getrfgain(_ah) \
    ((*(_ah)->ah_getRfGain)((_ah)))
#define ath_hal_getdefantenna(_ah) \
    ((*(_ah)->ah_getDefAntenna)((_ah)))
#define ath_hal_setdefantenna(_ah, _ant) \
    ((*(_ah)->ah_setDefAntenna)((_ah), (_ant)))
#define ath_hal_rxmonitor(_ah, _arg, _chan) \
    ((*(_ah)->ah_rxMonitor)((_ah), (_arg), (_chan)))
#define ath_hal_mibevent(_ah, _stats) \
    ((*(_ah)->ah_procMibEvent)((_ah), (_stats)))
#define ath_hal_setslottime(_ah, _us) \
    ((*(_ah)->ah_setSlotTime)((_ah), (_us)))
#define ath_hal_getslottime(_ah) \
    ((*(_ah)->ah_getSlotTime)((_ah)))
#define ath_hal_setacktimeout(_ah, _us) \
    ((*(_ah)->ah_setAckTimeout)((_ah), (_us)))
#define ath_hal_getacktimeout(_ah) \
    ((*(_ah)->ah_getAckTimeout)((_ah)))
#define ath_hal_setctstimeout(_ah, _us) \
    ((*(_ah)->ah_setCTSTimeout)((_ah), (_us)))
#define ath_hal_getctstimeout(_ah) \
    ((*(_ah)->ah_getCTSTimeout)((_ah)))
#define ath_hal_setdecompmask(_ah, _keyid, _b) \
        ((*(_ah)->ah_setDecompMask)((_ah), (_keyid), (_b)))
#define ath_hal_enablePhyDiag(_ah) \
    ((*(_ah)->ah_enablePhyErrDiag)((_ah)))
#define ath_hal_disablePhyDiag(_ah) \
    ((*(_ah)->ah_disablePhyErrDiag)((_ah)))
#define ath_hal_getcapability(_ah, _cap, _param, _result) \
    ((*(_ah)->ah_getCapability)((_ah), (_cap), (_param), (_result)))
#define ath_hal_setcapability(_ah, _cap, _param, _v, _status) \
    ((*(_ah)->ah_setCapability)((_ah), (_cap), (_param), (_v), (_status)))
#define ath_hal_ciphersupported(_ah, _cipher) \
    (ath_hal_getcapability(_ah, HAL_CAP_CIPHER, _cipher, NULL) == HAL_OK)
#define ath_hal_fastframesupported(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_FASTFRAME, 0, NULL) == HAL_OK)
#define ath_hal_burstsupported(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_BURST, 0, NULL) == HAL_OK)
#define ath_hal_xrsupported(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_XR, 0, NULL) == HAL_OK)
#define ath_hal_compressionsupported(_ah) \
        (ath_hal_getcapability(_ah, HAL_CAP_COMPRESSION, 0, NULL) == HAL_OK)
#define ath_hal_htsupported(_ah) \
        (ath_hal_getcapability(_ah, HAL_CAP_HT, 0, NULL) == HAL_OK)
#define ath_hal_gttsupported(_ah) \
        (ath_hal_getcapability(_ah, HAL_CAP_GTT, 0, NULL) == HAL_OK)
#define ath_hal_turboagsupported(_ah) \
    (ath_hal_getwirelessmodes(_ah, ath_countrycode) & (HAL_MODE_108G|HAL_MODE_TURBO))
#define ath_hal_halfrate_chansupported(_ah) \
        (ath_hal_getcapability(_ah, HAL_CAP_CHAN_HALFRATE, 0, NULL) == HAL_OK)
#define ath_hal_quarterrate_chansupported(_ah) \
        (ath_hal_getcapability(_ah, HAL_CAP_CHAN_QUARTERRATE, 0, NULL) == HAL_OK)
#define ath_hal_getregdomain(_ah, _prd) \
    ath_hal_getcapability(_ah, HAL_CAP_REG_DMN, 0, (_prd))
#define ath_hal_getcountrycode(_ah, _pcc) \
    (*(_pcc) = (_ah)->ah_countryCode)
#define ath_hal_tkipsplit(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TKIP_SPLIT, 0, NULL) == HAL_OK)
#define ath_hal_wmetkipmic(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_WME_TKIPMIC, 0, NULL) == HAL_OK)
#define ath_hal_hwphycounters(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_PHYCOUNTERS, 0, NULL) == HAL_OK)
#define ath_hal_hasdiversity(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_DIVERSITY, 0, NULL) == HAL_OK)
#define ath_hal_getdiversity(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_DIVERSITY, 1, NULL) == HAL_OK)
#define ath_hal_setdiversity(_ah, _v) \
    ath_hal_setcapability(_ah, HAL_CAP_DIVERSITY, 1, _v, NULL)
#define ath_hal_getnumtxqueues(_ah, _pv) \
    (ath_hal_getcapability(_ah, HAL_CAP_NUM_TXQUEUES, 0, _pv) == HAL_OK)
#define ath_hal_hasveol(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_VEOL, 0, NULL) == HAL_OK)
#define ath_hal_hastxpowlimit(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TXPOW, 0, NULL) == HAL_OK)
#define ath_hal_settxpowlimit(_ah, _pow) \
    ((*(_ah)->ah_setTxPowerLimit)((_ah), (_pow)))
#define ath_hal_gettxpowlimit(_ah, _ppow) \
    (ath_hal_getcapability(_ah, HAL_CAP_TXPOW, 1, _ppow) == HAL_OK)
#define ath_hal_getmaxtxpow(_ah, _ppow) \
    (ath_hal_getcapability(_ah, HAL_CAP_TXPOW, 2, _ppow) == HAL_OK)
#define ath_hal_gettpscale(_ah, _scale) \
    (ath_hal_getcapability(_ah, HAL_CAP_TXPOW, 3, _scale) == HAL_OK)
#define ath_hal_settpscale(_ah, _v) \
    ath_hal_setcapability(_ah, HAL_CAP_TXPOW, 3, _v, NULL)
#define ath_hal_hastpc(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TPC, 0, NULL) == HAL_OK)
#define ath_hal_gettpc(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TPC, 1, NULL) == HAL_OK)
#define ath_hal_settpc(_ah, _v) \
    ath_hal_setcapability(_ah, HAL_CAP_TPC, 1, _v, NULL)
#define ath_hal_hasbursting(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_BURST, 0, NULL) == HAL_OK)
#define ath_hal_hascompression(_ah) \
        (ath_hal_getcapability(_ah, HAL_CAP_COMPRESSION, 0, NULL) == HAL_OK)
#define ath_hal_hasgtt(_ah) \
        (ath_hal_getcapability(_ah, HAL_CAP_GTT, 0, NULL) == HAL_OK)
#define ath_hal_hasfastframes(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_FASTFRAME, 0, NULL) == HAL_OK)
#define ath_hal_hasbssidmask(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_BSSIDMASK, 0, NULL) == HAL_OK)
#define ath_hal_hasmcastkeysearch(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_MCAST_KEYSRCH, 0, NULL) == HAL_OK)
#define ath_hal_getmcastkeysearch(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_MCAST_KEYSRCH, 1, NULL) == HAL_OK)
#define ath_hal_hastkipmic(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TKIP_MIC, 0, NULL) == HAL_OK)
#define ath_hal_gettkipmic(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TKIP_MIC, 1, NULL) == HAL_OK)
#define ath_hal_settkipmic(_ah, _v) \
    ath_hal_setcapability(_ah, HAL_CAP_TKIP_MIC, 1, _v, NULL)
#define ath_hal_hastsfadjust(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TSF_ADJUST, 0, NULL) == HAL_OK)
#define ath_hal_gettsfadjust(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_TSF_ADJUST, 1, NULL) == HAL_OK)
#define ath_hal_settsfadjust(_ah, _v) \
    ath_hal_setcapability(_ah, HAL_CAP_TSF_ADJUST, 1, _v, NULL)
#define ath_hal_hasfastcc(_ah) \
    (ath_hal_getcapability(_ah, HAL_CAP_FAST_CC, 0, NULL) == HAL_OK)
#define ath_hal_getnummrretries(_ah, _pv) \
    (ath_hal_getcapability(_ah, HAL_CAP_NUM_MR_RETRIES, 0, _pv) == HAL_OK)
#define ath_hal_gettxchainmask(_ah, _pv)  \
    (ath_hal_getcapability(_ah, HAL_CAP_TX_CHAINMASK, 0, _pv) == HAL_OK)
#define ath_hal_getrxchainmask(_ah, _pv)  \
    (ath_hal_getcapability(_ah, HAL_CAP_RX_CHAINMASK, 0, _pv) == HAL_OK)

#define ath_hal_setuprxdesc(_ah, _ds, _size, _intreq) \
    ((*(_ah)->ah_setupRxDesc)((_ah), (_ds), (_size), (_intreq)))
#define ath_hal_rxprocdesc(_ah, _ds, _dspa, _dsnext) \
    ((*(_ah)->ah_procRxDesc)((_ah), (_ds), (_dspa), (_dsnext)))
#define ath_hal_rxprocdescfast(_ah, _ds, _dspa, _dsnext, _rx_stats) \
    ((*(_ah)->ah_procRxDescFast)((_ah), (_ds), (_dspa), (_dsnext), (_rx_stats)))
#define ath_hal_updateCTSForBursting(_ah, _ds, _prevds, _prevdsWithCTS, _gatingds,    \
                                     _txOpLimit, _ctsDuration)                \
    ((*(_ah)->ah_updateCTSForBursting)((_ah), (_ds), (_prevds), (_prevdsWithCTS), \
                                       (_gatingds), (_txOpLimit), (_ctsDuration)))
#define ath_hal_setuptxdesc(_ah, _ds, _plen, _hlen, _atype, _txpow, \
        _txr0, _txtr0, _keyix, _ant, _flags, \
        _rtsrate, _rtsdura, \
        _compicvlen, _compivlen, _comp) \
    ((*(_ah)->ah_setupTxDesc)((_ah), (_ds), (_plen), (_hlen), (_atype), \
        (_txpow), (_txr0), (_txtr0), (_keyix), (_ant), \
        (_flags), (_rtsrate), (_rtsdura), \
        (_compicvlen), (_compivlen), (_comp)))
#define ath_hal_setupxtxdesc(_ah, _ds, \
        _txr1, _txtr1, _txr2, _txtr2, _txr3, _txtr3) \
    ((*(_ah)->ah_setupXTxDesc)((_ah), (_ds), \
        (_txr1), (_txtr1), (_txr2), (_txtr2), (_txr3), (_txtr3)))
#define ath_hal_filltxdesc(_ah, _ds, _l, _first, _last, _ds0) \
    ((*(_ah)->ah_fillTxDesc)((_ah), (_ds), (_l), (_first), (_last), (_ds0)))
#define ath_hal_txprocdesc(_ah, _ds) \
    ((*(_ah)->ah_procTxDesc)((_ah), (_ds)))
#define ath_hal_gettxintrtxqs(_ah, _txqs) \
    ((*(_ah)->ah_getTxIntrQueue)((_ah), (_txqs)))
#define ath_hal_setglobaltxtimeout(_ah, _tu) \
    ((*(_ah)->ah_setGlobalTxTimeout)((_ah), (_tu)))
#define ath_hal_getglobaltxtimeout(_ah) \
    ((*(_ah)->ah_getGlobalTxTimeout)((_ah)))

#define ath_hal_gpioCfgOutput(_ah, _gpio) \
        ((*(_ah)->ah_gpioCfgOutput)((_ah), (_gpio)))
#define ath_hal_gpioset(_ah, _gpio, _b) \
        ((*(_ah)->ah_gpioSet)((_ah), (_gpio), (_b)))
#define ath_hal_ar_enable(_ah) \
    ((*(_ah)->ah_arEnable)((_ah)))
#define ath_hal_ar_disable(_ah) \
    ((*(_ah)->ah_arDisable)((_ah)))
#define ath_hal_ar_reset(_ah) \
    ((*(_ah)->ah_arReset)((_ah)))
#define ath_hal_setcoverageclass(_ah, _coverageclass, _now) \
    ((*(_ah)->ah_setCoverageClass)((_ah), (_coverageclass), (_now)))

#define ath_hal_getdescinfo(_ah, _descinfo) \
    ((*(_ah)->ah_getDescInfo)((_ah), (_descinfo)))

#define ath_hal_reset11n(_ah, _opmode, _chan, _htinfo, _outdoor, _pstatus) \
    ((*(_ah)->ah_reset11n)((_ah), (_opmode), (_chan), (_htinfo), (_outdoor), (_pstatus)))
#define ath_hal_set11n_txdesc(_ah, _ds, _pktlen, _type, _txpower,\
                             _keyix, _keytype, _flags) \
    ((*(_ah)->ah_set11nTxDesc)(_ah, _ds, _pktlen, _type, _txpower, _keyix,\
                               _keytype, _flags))
#define ath_hal_set11n_ratescenario(_ah, _ds, _durupdate, _rtsctsrate, \
                                    _series, _nseries, _flags)         \
    ((*(_ah)->ah_set11nRateScenario)(_ah, _ds, _durupdate, _rtsctsrate,\
                                     _series, _nseries, _flags))
#define ath_hal_set11n_aggr_first(_ah, _ds, _aggrlen, _numdelims) \
    ((*(_ah)->ah_set11nAggrFirst)(_ah, _ds, _aggrlen, _numdelims))
#define ath_hal_set11n_aggr_middle(_ah, _ds, _numdelims) \
    ((*(_ah)->ah_set11nAggrMiddle)(_ah, _ds, _numdelims))
#define ath_hal_set11n_aggr_last(_ah, _ds) \
    ((*(_ah)->ah_set11nAggrLast)(_ah, _ds))
#define ath_hal_clr11n_aggr(_ah, _ds) \
    ((*(_ah)->ah_clr11nAggr)(_ah, _ds))
#define ath_hal_set11n_burstduration(_ah, _ds, _burstduration) \
    ((*(_ah)->ah_set11nBurstDuration)(_ah, _ds, _burstduration))
#define ath_hal_get11nextbusy(_ah) \
    ((*(_ah)->ah_get11nExtBusy)((_ah)))
#define ath_hal_set11nmac2040(_ah, _mode) \
    ((*(_ah)->ah_set11nMac2040)((_ah), (_mode)))
#define ath_hal_get11nRxClear(_ah) \
    ((*(_ah)->ah_get11nRxClear)((_ah)))
#define ath_hal_set11nRxClear(_ah, _rxclear) \
    ((*(_ah)->ah_set11nRxClear)((_ah), (_rxclear)))
#define ath_hal_get11n_hwplatform(_ah) \
    ((*(_ah)->ah_get11nHwPlatform)((_ah)))
#define ath_hal_getCycleCounts(_ah, _rxclear, _rxframe, _txframe) \
    ((*(_ah)->ah_getCycleCounts)((_ah), (_rxclear), (_rxframe), (_txframe)))
#define ath_hal_dmaRegDump(_ah) \
    ((*(_ah)->ah_dmaRegDump)((_ah)))
#endif /* _DEV_ATH_ATHVAR_H */
