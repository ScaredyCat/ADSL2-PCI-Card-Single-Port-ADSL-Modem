/*
 * if_owl.c - owl specific tx/rx code
 */
#define AGGR_NOSHORT      0                  /* enable for h/w workaround */
#define ATH_RX_TIMEOUT    MSECS(40)          /* 40 ms for rx timeout */
#define ATH_TIMER_TICKS   MSECS(20)          /* timer resolution 20 ms */

#include "opt_ah.h"

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/cache.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/if_arp.h>

#include <asm/uaccess.h>

#include "if_ethersubr.h"       /* for ETHER_IS_MULTICAST */
#include "if_media.h"
#include "if_llc.h"

#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_var.h>

#ifdef USE_HEADERLEN_RESV
#include <net80211/if_llc.h>
#endif

#define AR_DEBUG
#include "if_athrate.h"
#include "net80211/if_athproto.h"
#include "if_athvar.h"
#include "ah_desc.h"
#include "ah_devid.h"           /* XXX to identify IBM cards */

#ifdef ATH_PCI      /* PCI BUS */
#include "if_ath_pci.h"
#endif          /* PCI BUS */
#ifdef ATH_AHB      /* AHB BUS */
#include "if_ath_ahb.h"
#endif          /* AHB BUS */

#ifdef ATRC
#include "../atrc/atrc.h"
atrc_decl();
#else
#define atrc_init()
#define atrc_ent(_x)
#define assert(_x)
#endif

#ifdef DEBUG_PKTLOG
    extern struct ath_pktlog_funcs *g_pktlog_funcs;
#endif

enum {
    OWLTRC_TX   = 0x01,
    OWLTRC_RX   = 0x02,
    OWLTRC_NET  = 0x04,
} owltrc =
#ifdef EMU_AP
    0
#else
    0
#endif
   ;

#define ttrc_ent(_x) if (owltrc & OWLTRC_TX)  atrc_ent(_x)
#define rtrc_ent(_x) if (owltrc & OWLTRC_RX)  atrc_ent(_x)
#define ntrc_ent(_x) if (owltrc & OWLTRC_NET) atrc_ent(_x)

#ifdef CES_DEMO
extern u_int32_t   ath_htrate_0123;
extern u_int32_t   ath_legacyrate_0123;
extern u_int32_t   ath_rate_tries_0123;
#endif
extern u_int32_t   ath_aggr_prot;
extern u_int32_t   ath_aggr_prot_duration;
extern u_int32_t   ath_aggr_prot_max;

#ifdef ATH_CHAINMASK_SELECT
extern int ath_chainmask_sel_logic(struct ath_softc *);
#endif


/*
 * add to tail queue
 */
#define TAILQ_DEQ(_q, _elm, _field) do {        \
        (_elm) = TAILQ_FIRST((_q));             \
        if (_elm) {                             \
            TAILQ_REMOVE((_q), (_elm), _field); \
        }                                       \
    } while (0)

#define __stats(sc, _x)       sc->sc_stats.ast_11n_stats._x ++
#define __statsn(sc, _x, _n)  sc->sc_stats.ast_11n_stats._x += _n

#define IS_HT_RATE(_rate)     ((_rate) & 0x80)

typedef enum {
    ATH_AGGR_DONE,
    ATH_AGGR_BAW_CLOSED,
    ATH_AGGR_LIMITED,
    ATH_AGGR_SHORTPKT,
} ATH_AGGR_STATUS;

/*
 *----------------------------------------------------------------------
 * add to madwifi/net80211/ieee80211.h
 *----------------------------------------------------------------------
 */
#define IEEE80211_ATHC_11N      0x4000
#define IEEE80211_SEQ_MAX       4096
#define IEEE80211_FRAG_THRESH   2346

/*
 *----------------------------------------------------------------------
 * aggregation specific macros
 *----------------------------------------------------------------------
 */
#define MSECS(_n)         ((_n) * HZ / 1000)

#define BAW_WITHIN(_start, _bawsz, _seqno)      \
    ((((_seqno) - (_start)) & 4095) < (_bawsz))

#define PADBYTES(_len)   ((4 - ((_len) % 4)) % 4)
#define OWLMAX_RETRIES   10

#define ATH_DS_TOGGLE(_bf, _retry) do {                         \
    (_bf)->bf_descno = !(_bf)->bf_descno;                       \
    (_bf)->bf_desc   = (_bf)->bf_descarr + (_bf)->bf_descno;    \
    (_bf)->bf_lastds = (_bf)->bf_desc;                          \
    (_bf)->bf_daddr  = (_bf)->bf_daddrs[(_bf)->bf_descno];      \
    if (_retry)                                                 \
        *(_bf)->bf_desc = (_bf)->bf_descarr[!(_bf)->bf_descno]; \
} while (0)

/*
 *----------------------------------------------------------------------
 * move to if_athvar.h - public functions
 *----------------------------------------------------------------------
 */
int ath_hardstart(struct sk_buff *skb, struct net_device *dev);
int owl_hardstart(struct sk_buff *skb, struct net_device *dev);

/*
 *----------------------------------------------------------------------
 * local function declarations
 *----------------------------------------------------------------------
 */
static void ath_aggr_addba_timertimeout(struct ath_softc *sc, void *timerArg);
static void ath_aggr_addba_timerstart(struct ath_softc *sc, ath_atx_tid_t *tid);
static void ath_aggr_addba_timerstop(struct ath_softc *sc, ath_atx_tid_t *tid);
static u_int32_t ath_pkt_duration(struct ath_softc *sc, u_int8_t rix,
                                  struct ath_buf *bf, int width, int half_gi);
static void ath_txq_filter_done(struct ath_softc *sc, struct ath_txq *txq);
static void ath_tx_queue_normal(struct ath_softc *sc, struct ath_txq *txq,
                                struct sk_buff *skb);
static void ath_tx_queue_ff(struct ath_softc *sc, struct ath_txq *txq,
                            struct sk_buff *skb);
static int ath_tx_can_ff(struct ath_softc *sc, struct ath_node *an,
                         struct sk_buff *skb, struct sk_buff *pskb);
static void ath_tx_setds(struct ath_softc *sc, struct ath_buf *bf);
static void ath_tx_seqno_normal(struct ath_buf *bf);
static void ath_tx_seqno_ff(struct ath_buf *bf);
static void ath_tx_seqno_aggr(struct ath_buf *bf);
static void ath_tx_enqueue(struct ath_txq *txq, struct ath_node *an, int tidno);
static void ath_node_txsetup(struct ath_softc *sc, struct ath_node *an);
static void ath_transmit(struct ath_softc *sc, struct ath_txq *txq,
                         struct sk_buff *skb);
static void ath_buf_free(struct ath_softc *sc, ath_bufhead *bf_q);
static void ath_skb_free(struct sk_buff *skb);
static struct ath_buf * ath_buf_alloc(struct ath_softc *sc, ath_bufhead *bf_q,
                                      int framecnt);
static int ath_aggr_input(struct ieee80211_node *ni, struct sk_buff *skb,
                          int rssi, u_int32_t rstamp);
static void ath_rx_timer(struct ath_softc *sc, void *tmr_arg);
static void ath_rx_timer_start(struct ath_softc *sc, struct ath_arx_tid *rxtid);
static void ath_rx_timer_stop(struct ath_softc *sc, struct ath_arx_tid *rxtid);
static void ath_timer_run(unsigned long timer_arg);
static void ath_wd_run(struct ath_softc *sc, void *arg);
static void ath_wd_start(struct ath_softc *sc);
static void ath_wd_stop(struct ath_softc *sc);

static int ath_tx_check(struct net_device *dev, struct sk_buff *skb,
                        int *retval);
static void ath_buf_comp(struct ath_softc *sc, struct ath_buf *bf);
static int ath_txbuf_setup(struct ath_softc *sc, struct ath_buf *bf);
static void ath_set_frame_type(struct ath_softc *sc, struct ath_buf *bf);
static void ath_txq_add_ucast(struct ath_softc *sc, struct ath_buf *bf);
static void ath_txq_add_mcast(struct ath_softc *sc, struct ath_buf *bf);
static void ath_check_hidden_prot(struct ath_softc *sc, struct ath_buf *bf);
static void ath_ft_mgt_set_atype(struct ath_softc *sc, struct ath_buf *bf);
static void ath_ft_data_set_atype(struct ath_softc *sc, struct ath_buf *bf);
static void ath_ft_ctl_set_atype(struct ath_softc *sc, struct ath_buf *bf);
static void ath_ft_ctl_get_rate(struct ath_softc *sc, struct ath_buf *bf);
static void ath_ft_mgt_get_rate(struct ath_softc *sc, struct ath_buf *bf);
static void ath_ft_data_get_rate(struct ath_softc *sc, struct ath_buf *bf);
static void ath_dma_map(struct ath_softc *sc, struct ath_buf *bf);
static void ath_dma_unmap(struct ath_softc *sc, struct ath_buf *bf);
static int ath_key_setup(struct ieee80211_node *ni, struct ath_buf *bf);
static int ath_get_pktlen(struct ath_buf *bf, int hdrlen);
static u_int16_t ath_aggr_limit(struct ath_softc *sc, u_int32_t ratekbps);
static void ath_tx_sched_normal(struct ath_softc *sc, ath_atx_tid_t *tid);
static void ath_tx_sched_ff(struct ath_softc *sc, ath_atx_tid_t *tid);
static void ath_tx_sched_aggr(struct ath_softc *sc, ath_atx_tid_t *tid);
static void ath_txq_schedule(struct ath_softc *sc, struct ath_txq *txq);
static void ath_txq_drain(struct ath_softc *sc, struct ath_txq *txq);
static void ath_tid_drain(struct ath_softc *sc, struct ath_atx_tid *tid);
static void ath_tx_comp_normal(struct ath_softc *sc, struct ath_buf *bf);
static void ath_tx_comp_ff(struct ath_softc *sc, struct ath_buf *bf);
static void ath_tx_freebuf(struct ath_softc *sc, struct ath_buf *bf);
static void ath_tx_uc_comp(struct ath_softc *sc, struct ath_buf *bf);
static void ath_update_stats(struct ath_softc *sc, struct ath_buf *bf);
static void ath_bar_tx(struct ath_softc *sc, ath_atx_tid_t *tid,
                       struct ath_buf *bf);
static void ath_bar_tx_comp(struct ath_softc *sc, struct ath_buf *bf);
static int  ath_bar_rx(struct ieee80211_node *ni, struct sk_buff *skb,
                       int rssi, u_int32_t rstamp);
static void ath_tx_comp_filtered(struct ath_softc *sc, struct ath_buf *bf);
static void ath_tx_comp_aggr(struct ath_softc *sc, struct ath_buf *bf);
static void ath_update_aggr_stats(struct ath_softc *sc, struct ath_desc *ds,
                                  int nframes, int nbad);
static void ath_tx_addto_baw(ath_atx_tid_t *tid, struct ath_buf *bf);
static void ath_tx_update_baw(ath_atx_tid_t *tid, int seqno);
static void ath_tx_set_retry(struct ath_softc *sc, struct ath_buf *bf);
static inline void ath_tx_retry_subframe(struct ath_softc *sc,
                                         struct ath_buf *bf, ath_bufhead *bf_q,
                                         struct ath_buf **bar);
static inline void ath_tx_retry_unaggr(struct ath_softc *sc, 
                                       struct ath_buf *bf);

static ATH_AGGR_STATUS ath_tx_form_aggr(struct ath_softc *sc,
                                        ath_atx_tid_t *tid, ath_bufhead *bf_q);

void ath_set_nav(struct ath_softc *sc, struct ath_buf *bf);
void ath_set_cts_duration(struct ath_softc *sc, struct ath_buf *bf);

/*
 *----------------------------------------------------------------------
 * miscellaneous
 * consider moving these to the header file if_athvar.h so that these
 * definitions can be shared
 *----------------------------------------------------------------------
 */
static __inline void ath_desc_swap(struct ath_desc *ds);
static int ath_tx_findindex(const HAL_RATE_TABLE *rt, int rate);

/*
 *----------------------------------------------------------------------
 * updated rate adaptation interfaces
 * new rate adaptation interface for aggregate tx
 *----------------------------------------------------------------------
 */

#ifdef CES_DEMO
/*
 * Sets fixed rateseries based on node type(HT or legacy)
 */
static void
ath_set_rateseries_fixed(struct ath_softc *sc, struct ath_buf *bf,
                         HAL_11N_RATE_SERIES *series)
{
    int                     i;
    u_int32_t               ath_rate_0123;
    int                     is_htnode;

    is_htnode = (bf->bf_node->ni_flags & IEEE80211_NODE_HT);
    ath_rate_0123 = (is_htnode)? ath_htrate_0123 : ath_legacyrate_0123;

    series[0].Rate = (ath_rate_0123 >> 24) & 0xff;
    series[0].Tries = (ath_rate_tries_0123 >> 24) & 0xff;

    series[1].Rate = (ath_rate_0123 >> 16) & 0xff;
    series[1].Tries = (ath_rate_tries_0123 >> 16) & 0xff;

    series[2].Rate = (ath_rate_0123 >> 8) & 0xff;
    series[2].Tries = (ath_rate_tries_0123 >> 8) & 0xff;

    series[3].Rate = (ath_rate_0123) & 0xff;
    series[3].Tries = (ath_rate_tries_0123) & 0xff;

    for (i = 0; i < 4 ; i++) {

        if (!series[i].Tries)
            continue;

        /*
         * Clear short_gi and 20-40
         */
        series[i].RateFlags = 0;

        if(is_htnode && IS_HT_RATE(series[i].Rate)) {
        int is_40 = 0;
        if (sc->sc_ic.ic_cwm.cw_mode == IEEE80211_CWM_MODE40) {
            is_40 = 1;
        } else if (sc->sc_ic.ic_cwm.cw_mode == IEEE80211_CWM_MODE2040) {
            if ((bf->bf_node->ni_chwidth == IEEE80211_CWM_WIDTH40) &&
            (sc->sc_ic.ic_cwm.cw_width == IEEE80211_CWM_WIDTH40))
            {
                is_40 = 1;
            }
        }
        if (is_40) {
            series[i].RateFlags |= HAL_RATESERIES_2040;
            /*
             * Short GI is valid only for HT40 rates
             */
            if (sc->sc_ic.ic_htflags & IEEE80211_HTF_SHORTGI) {
                series[i].RateFlags |= HAL_RATESERIES_HALFGI;
            }
        }
    }
    }
}
#endif

#define OFDM_PLCP_BITS          22
#define HT_RC_2_MCS(_rc)        ((_rc) & 0x0f)
#define HT_RC_2_STREAMS(_rc)    ((((_rc) & 0x78) >> 3) + 1)
#define L_STF                   8
#define L_LTF                   8
#define L_SIG                   4
#define HT_SIG                  8
#define HT_STF                  4
#define HT_LTF(_ns)             (4 * (_ns))
#define SYMBOL_TIME(_ns)        ((_ns) << 2)            // ns * 4 us
#define SYMBOL_TIME_HALFGI(_ns) (((_ns) * 18 + 4) / 5)  // ns * 3.6 us

static u_int32_t bits_per_symbol[][2] = {
    /* 20MHz 40MHz */
    {    26,   54 },     //  0: BPSK
    {    52,  108 },     //  1: QPSK 1/2
    {    78,  162 },     //  2: QPSK 3/4
    {   104,  216 },     //  3: 16-QAM 1/2
    {   156,  324 },     //  4: 16-QAM 3/4
    {   208,  432 },     //  5: 64-QAM 2/3
    {   234,  486 },     //  6: 64-QAM 3/4
    {   260,  540 },     //  7: 64-QAM 5/6
    {    52,  108 },     //  8: BPSK
    {   104,  216 },     //  9: QPSK 1/2
    {   156,  324 },     // 10: QPSK 3/4
    {   208,  432 },     // 11: 16-QAM 1/2
    {   312,  648 },     // 12: 16-QAM 3/4
    {   416,  864 },     // 13: 64-QAM 2/3
    {   468,  972 },     // 14: 64-QAM 3/4
    {   520, 1080 },     // 15: 64-QAM 5/6
};

/*
 * ath_pkt_dur - compute packet duration
 * ref: depot/chips/owl/2.0/rtl/mac/doc/rate_to_duration_ht.xls
 *
 * rix - rate index
 * pktlen - total bytes (delims + data + fcs + pads + pad delims)
 * width  - 0 for 20 MHz, 1 for 40 MHz
 * half_gi - to use 4us v/s 3.6 us for symbol time
 */
static u_int32_t
ath_pkt_duration(struct ath_softc *sc, u_int8_t rix, struct ath_buf *bf,
                 int width, int half_gi)
{
    const HAL_RATE_TABLE    *rt = sc->sc_currates;
    u_int32_t               nbits, nsymbits, duration, nsymbols;
    u_int8_t                rc;
    int                     streams;
    int                     pktlen;

    pktlen = bf->bf_isaggr ? bf->bf_al : bf->bf_pktlen;
    rc = rt->info[rix].rateCode;

    /*
     * for legacy rates, use old function to compute packet duration
     */
    if (!IS_HT_RATE(rc))
        return ath_hal_computetxtime(sc->sc_ah, rt, pktlen, rix,
                                     bf->bf_shpream);

    /*
     * find number of symbols: PLCP + data
     */
    nbits = (pktlen << 3) + OFDM_PLCP_BITS;
    nsymbits = bits_per_symbol[HT_RC_2_MCS(rc)][width];
    nsymbols = (nbits + nsymbits - 1) / nsymbits;

    if (!half_gi)
        duration = SYMBOL_TIME(nsymbols);
    else
        duration = SYMBOL_TIME_HALFGI(nsymbols);

    /*
     * addup duration for legacy/ht training and signal fields
     */
    streams = HT_RC_2_STREAMS(rc);
    duration += L_STF + L_LTF + L_SIG + HT_SIG + HT_STF + HT_LTF(streams);
    return duration;
}


/*
 * ath_buf_set_rate
 *   Rate module function to set rate related fields in tx descriptor.
 *
 * args
 *   sc         - device instance
 */
void
ath_buf_set_rate(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_hal       *ah = sc->sc_ah;
    const HAL_RATE_TABLE *rt;
    struct ath_desc      *ds = bf->bf_desc;
    HAL_11N_RATE_SERIES  series[4];
    int                  i, flags = 0;
    u_int8_t             rix;

    /*
     * Set protection if aggregate protection on
     */
    if (ath_aggr_prot && (!bf->bf_isaggr || 
                          (bf->bf_isaggr && bf->bf_al < ath_aggr_prot_max))) {
        flags = HAL_TXDESC_RTSENA;
    }
    
    rt = sc->sc_currates;
    memset(series, 0, sizeof(HAL_11N_RATE_SERIES) * 4);

    for (i = 0; i < 4; i++) {
        if (!bf->bf_rcs[i].tries)
            continue;

        rix = bf->bf_rcs[i].rix;

        series[i].Rate = rt->info[rix].rateCode | 
                         (bf->bf_shpream ? rt->info[rix].shortPreamble : 0);
        series[i].Tries = bf->bf_rcs[i].tries;
        series[i].RateFlags = ((bf->bf_rcs[i].flags & ATH_RC_RTSCTS_FLAG) ?
                               HAL_RATESERIES_RTS_CTS:0 ) |
                              ((bf->bf_rcs[i].flags & ATH_RC_CW40_FLAG) ?
                               HAL_RATESERIES_2040 : 0 ) |
                              ((bf->bf_rcs[i].flags & ATH_RC_SGI_FLAG) ?
                               HAL_RATESERIES_HALFGI : 0 );
        series[i].PktDuration = ath_pkt_duration(sc, rix, bf,
                              (bf->bf_rcs[i].flags & ATH_RC_CW40_FLAG) != 0,
                              (bf->bf_rcs[i].flags & ATH_RC_SGI_FLAG));

	/* So the logic is - Set ChSel to user config if the client is 
	   HT or if the rate is CCK if the client is Legacy. Otherwise
	   for legacy OFDM set the ChSel to 1. */
        if (bf->bf_node && !(bf->bf_node->ni_flags & IEEE80211_NODE_HT)) {
	    if (rt->info[rix].phy != IEEE80211_T_CCK) {
            	series[i].ChSel = 1;	// If legacy STA, OFDM rate set 
	    }			        // tx chainmask to 1.
	    else {
		series[i].ChSel = sc->sc_ic.ic_tx_chainmask;
	    }
        } else {
#ifdef ATH_CHAINMASK_SELECT
            series[i].ChSel = ath_chainmask_sel_logic(sc);
#else
            series[i].ChSel = sc->sc_ic.ic_tx_chainmask;
#endif
        }

        /* 
         * Set protection if globally mandated
         */ 
        if (bf->bf_protmode != IEEE80211_PROT_NONE) {
            if (IS_HT_RATE(series[i].Rate))
                series[i].RateFlags |= HAL_RATESERIES_RTS_CTS;
        }

        if (flags)
            series[i].RateFlags |= HAL_RATESERIES_RTS_CTS;
    }

    /*
     * always set dur_update_en for l-sig computation. 
     */
#define DEFAULT_RTSCTS_RATECODE		0x18 
    ath_hal_set11n_ratescenario(ah, ds, 1, 
                                (sc->sc_ic.ic_rtscts_ratecode ? 
                                 sc->sc_ic.ic_rtscts_ratecode: 
                                 DEFAULT_RTSCTS_RATECODE),
                                series, 4, flags);
    if (ath_aggr_prot && flags)
        ath_hal_set11n_burstduration(ah, ds, ath_aggr_prot_duration);
}

/*
 *----------------------------------------------------------------------
 * some useful macros; could be used elsewhere
 *----------------------------------------------------------------------
 */

/*
 * some general macros
 */
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#define INCR(_l, _sz)   (_l) ++; (_l) &= ((_sz) - 1)
#define N(_a)           (sizeof(_a)/sizeof(_a[0]))

#define ATH_NI(_an)                 ((struct ieee80211_node *)(_an))
#define ATH_NI_2_SC(_ni)            \
    ((struct ath_softc *)((_ni)->ni_ic->ic_dev->priv))
#define ATH_AN_2_SC(_an)            ATH_NI_2_SC(&(_an)->an_node)

#define ATH_AN_2_AC(_an, _priority) &(_an)->an_aggr.tx.ac[(_priority)]
#define ATH_AN_2_TID(_an, _tidno)   (&(_an)->an_tx_tid[(_tidno)])
#define ATH_TXQ(_sc, _qi)           (&(_sc)->sc_txq[(_qi)])
#define ATH_AC_2_TXQ(_sc, _ac)      (_sc)->sc_ac2q[(_ac)]
#define ATH_SKB_2_NI(_skb)          (((struct ieee80211_cb *) (_skb)->cb)->ni)
#define ATH_SKB_2_AN(_skb)          ATH_NODE(ATH_SKB_2_NI(_skb))
#define ATH_SKB_2_WH(_skb)          ((struct ieee80211_frame *) (_skb)->data)
#define ATH_WH_IS_FRAG(_wh)                                         \
    (((_wh)->i_fc[1] & IEEE80211_FC1_MORE_FRAG) ||                  \
     (((le16toh(*(u_int16_t *) &(_wh)->i_seq[0]) >>                 \
        IEEE80211_SEQ_FRAG_SHIFT) & IEEE80211_SEQ_FRAG_MASK) > 0))

/*
 * init/setup buf
 */
#define ATH_BUF_SET(_bf, _ni, _txq, _skb, _nfl) do {    \
        (_bf)->bf_node  = (_ni);                        \
        (_bf)->bf_txq   = (_txq);                       \
        (_bf)->bf_skb   = (_skb);                       \
        (_bf)->bf_nfl   = (_nfl);                       \
        (_bf)->bf_comp  = ATH_NODE((_ni))->an_tx_comp;  \
    } while (0)

/*
 * new aggegation related macros
 */
#define ATH_AGGR_DELIM_SZ       4       /* delimiter size   */
#define ATH_AGGR_MINPLEN        256     /* in bytes, minimum packet length */
#define ATH_AGGR_ENCRYPTDELIM   10   /* number of delimiters for encryption padding */

/*
 * returns delimiter padding required given the packet length
 */
#define ATH_AGGR_GET_NDELIM(_len)                                   \
    (((((_len) + ATH_AGGR_DELIM_SZ) < ATH_AGGR_MINPLEN) ?           \
      (ATH_AGGR_MINPLEN - (_len) - ATH_AGGR_DELIM_SZ) : 0) >> 2)

/*
 * minimum h/w qdepth to be sustained to maximize aggregation
 */
#define ATH_AGGR_MIN_QDEPTH 2

/*
 * return block-ack bitmap index given sequence and starting sequence
 */
#define ATH_BA_INDEX(_st, _seq) (((_seq) - (_st)) & (IEEE80211_SEQ_MAX - 1))

/*
 * return whether a bit at index _n in bitmap _bm is set
 * _sz is the size of the bitmap
 */
#define ATH_BA_ISSET(_bm, _n)        (((_n) < (WME_BA_BMP_SIZE)) &&          \
                                     ((_bm)[(_n) >> 5] & (1 << ((_n) & 31))))

/*
 * desc accessor macros
 */
#define ATH_DS_BA_SEQ(_ds)          (_ds)->ds_us.tx.ts_seqnum
#define ATH_DS_BA_BITMAP(_ds)       (&(_ds)->ds_us.tx.ba_low)
#define ATH_DS_TX_BA(_ds)           ((_ds)->ds_us.tx.ts_flags & HAL_TX_BA)
#define ATH_DS_TX_STATUS(_ds)       (_ds)->ds_us.tx.ts_status
#define ATH_DS_TX_FLAGS(_ds)        (_ds)->ds_us.tx.ts_flags

/*
 *----------------------------------------------------------------------
 * insert a queue to the head of queue
 *----------------------------------------------------------------------
 */
#define TAILQ_INSERTQ_HEAD(head, tq, field) do {                \
        if ((head)->tqh_first) {                                \
            *(tq)->tqh_last = (head)->tqh_first;                \
            (head)->tqh_first->field.tqe_prev = (tq)->tqh_last; \
        } else {                                                \
            (head)->tqh_last = (tq)->tqh_last;                  \
        }                                                       \
        (head)->tqh_first = (tq)->tqh_first;                    \
        (tq)->tqh_first->field.tqe_prev = &(head)->tqh_first;   \
    } while (0)

/*
 *----------------------------------------------------------------------
 * static variables
 *----------------------------------------------------------------------
 */

/*
 * TX processing functions based on frame type
 */
typedef void (*ath_ft_set_atype_t)(struct ath_softc *sc, struct ath_buf *bf);
typedef void (*ath_ft_get_rate_t)(struct ath_softc *sc, struct ath_buf *bf);

static struct {
    ath_ft_set_atype_t  set_atype;
    ath_ft_get_rate_t   get_rate;
} ath_ft_process [] = {
    { ath_ft_mgt_set_atype,  ath_ft_mgt_get_rate  },
    { ath_ft_ctl_set_atype,  ath_ft_ctl_get_rate  },
    { ath_ft_data_set_atype, ath_ft_data_get_rate },
};

/*
 *----------------------------------------------------------------------
 * public functions
 *----------------------------------------------------------------------
 */

#define ADDBA_EXCHANGE_ATTEMPTS     10
#define ADDBA_TIMEOUT               MSECS(200)

/*
 * Aggregation allowed on this queue?
 *
 */
int
ath_aggr_query(struct ath_softc *sc, struct ath_node *an, struct sk_buff *skb)
{
    ath_atx_tid_t   *tid;
    u_int8_t    tidno;
    struct ieee80211_action_mgt_args actionargs;

    if (ATH_NODE_IS_11N(an)) {
        /* ADDBA exchange must be completed before sending aggregates */
        tidno   = WME_AC_TO_TID(skb->priority);
        tid = ATH_AN_2_TID(an, tidno);

        if (tid->addba_exchangecomplete) {
            return 1;
        } else if (sc->sc_ic.ic_addba_mode == ADDBA_MODE_MANUAL) {
            return 0;
        } else {
            if (!tid->addba_exchangeinprogress &&
             (tid->addba_exchangeattempts < ADDBA_EXCHANGE_ATTEMPTS)) {
            tid->addba_exchangeattempts++;

            /* Send ADDBA request */
            actionargs.category = IEEE80211_ACTION_CAT_BA;
            actionargs.action   = IEEE80211_ACTION_BA_ADDBA_REQUEST;
            actionargs.arg1     = tidno;
            actionargs.arg2     = WME_MAX_BA;
            actionargs.arg3     = 0;
            IEEE80211_SEND_MGMT(ATH_NI(an), IEEE80211_FC0_SUBTYPE_ACTION,
                                (int)&actionargs);
            }
        }
    }
    return 0;
}

/*
 * Send ADDBA request
 *
 */
int
ath_aggr_addba_send(struct ieee80211com *ic, struct ieee80211_node *ni, u_int8_t ac,
                    u_int16_t buffersize)
{
    ath_atx_tid_t   *tid;
    u_int8_t    tidno;
    struct ath_node *an  = ATH_NODE(ni);
    struct ieee80211_action_mgt_args actionargs;
    struct ath_softc *sc = ic->ic_dev->priv;

    if (!an->an_tx_queue)
        ath_node_txsetup(sc, an);

    if (ATH_NODE_IS_11N(an)) {
        tidno = WME_AC_TO_TID(ac);
        tid = ATH_AN_2_TID(an, tidno);

        /* Send ADDBA request */
        actionargs.category = IEEE80211_ACTION_CAT_BA;
        actionargs.action   = IEEE80211_ACTION_BA_ADDBA_REQUEST;
        actionargs.arg1     = tidno;
        actionargs.arg2     = buffersize;
        actionargs.arg3     = 0;
        IEEE80211_SEND_MGMT(ATH_NI(an), IEEE80211_FC0_SUBTYPE_ACTION, (int)&actionargs);

        return 0;
    } else {
        return 1;
    }
}

/*
 * Get status of ADDBA request
 *
 */
void
ath_aggr_addba_status(struct ieee80211com *ic, struct ieee80211_node *ni, u_int8_t ac,
                      u_int16_t *status)
{
    ath_atx_tid_t   *tid;
    u_int8_t    tidno;
    struct ath_node *an  = ATH_NODE(ni);

    tidno = WME_AC_TO_TID(ac);
    tid = ATH_AN_2_TID(an, tidno);

    /*
     * Report the ADDBA response status code.  Return a special status to indicate
     * that either ADDBA was not initiated, or the response has not been received yet.
     */
    if ((tid->addba_exchangestatuscode == IEEE80211_STATUS_SUCCESS) &&
        !tid->addba_exchangecomplete ) {
        *status = 0xFFFF;
    } else {
        *status = tid->addba_exchangestatuscode;
    }
}

/*
 * Setup ADDBA request
 *
 */

void
ath_aggr_addba_requestsetup(struct ieee80211com *ic, struct ieee80211_node *ni, u_int8_t tidno,
                    struct ieee80211_ba_parameterset    *baparamset,
                    u_int16_t                       *batimeout,
                    struct ieee80211_ba_seqctrl     *basequencectrl,
                    u_int16_t                       buffersize)
{
    struct ath_softc *sc = ic->ic_dev->priv;
    struct ath_node *an  = ATH_NODE(ni);
    ath_atx_tid_t   *tid = ATH_AN_2_TID(an, tidno);;

    baparamset->reserved0      = 0;
    baparamset->bapolicy       = IEEE80211_BA_POLICY_IMMEDIATE;
    baparamset->tid            = tidno;
    baparamset->buffersize     = buffersize;
    *batimeout                 = 0;
    basequencectrl->fragnum    = 0;
    basequencectrl->startseqnum = tid->seq_start;

    /* Start ADDBA request timer */
    ath_aggr_addba_timerstart(sc, tid);
}

/*
 * Process ADDBA request and save response information in per-TID data structure
 *
 */
void
ath_aggr_addba_requestprocess(struct ieee80211com *ic, struct ieee80211_node *ni,
                    u_int8_t                        dialogtoken,
                    struct ieee80211_ba_parameterset    *baparamset,
                    u_int16_t                       batimeout,
                    struct ieee80211_ba_seqctrl         basequencectrl)
{
    struct ath_node *an     = ATH_NODE(ni);
    u_int16_t       tidno   = baparamset->tid;
    struct ath_arx_tid  *rxtid  = &an->an_aggr.rx.tid[tidno];

    /* Allow aggregation reception */
    rxtid->addba_exchangecomplete = 1;

    /* adjust rx BA window size. Peer might indicate a zero buffer size for
     * a _dont_care_ condition.
     */
    if (baparamset->buffersize)
        rxtid->baw_size = MIN(baparamset->buffersize, rxtid->baw_size);

    /* set rx sequence number */
    rxtid->seq_next = basequencectrl.startseqnum;

    /* start processing aggregates */
    an->an_rx_input = ath_aggr_input;

    /* save ADDBA response parameters in rx TID */
    rxtid->dialogtoken      = dialogtoken;
    rxtid->statuscode               = IEEE80211_STATUS_SUCCESS;
    rxtid->baparamset.reserved0     = 0;
    rxtid->baparamset.bapolicy      = IEEE80211_BA_POLICY_IMMEDIATE;
    rxtid->baparamset.tid           = tidno;
    rxtid->baparamset.buffersize    = rxtid->baw_size;
    rxtid->batimeout                = 0;
}

/*
 * Setup ADDBA response
 *
 * Output: status code, BA parameter set and BA timeout
 *         for response
 */
void
ath_aggr_addba_responsesetup(struct ieee80211com *ic, struct ieee80211_node *ni,
            u_int8_t                tidno,
            u_int8_t                        *dialogtoken,
            u_int16_t                       *statuscode,
            struct ieee80211_ba_parameterset        *baparamset,
            u_int16_t                       *batimeout)
{
    struct ath_node     *an     = ATH_NODE(ni);
    struct ath_arx_tid      *rxtid  = &an->an_aggr.rx.tid[tidno];

    /* setup ADDBA response paramters */
    *dialogtoken    = rxtid->dialogtoken;
    *statuscode = rxtid->statuscode;
    *baparamset = rxtid->baparamset;
    *batimeout  = rxtid->batimeout;
}

/*
 * Process ADDBA response
 *
 */
void
ath_aggr_addba_responseprocess(struct ieee80211com *ic, struct ieee80211_node *ni,
                            u_int16_t                       statuscode,
                struct ieee80211_ba_parameterset    *baparamset,
                            u_int16_t                       batimeout)
{
    struct ath_softc *sc = ic->ic_dev->priv;
    struct ath_node *an  = ATH_NODE(ni);
    u_int16_t       tidno  = baparamset->tid;
    ath_atx_tid_t       *tid   = ATH_AN_2_TID(an, tidno);

    /* Stop ADDBA request timer only for auto-mode */
    if (ic->ic_addba_mode == ADDBA_MODE_AUTO) {
        ath_aggr_addba_timerstop(sc, tid);
    }

    tid->addba_exchangestatuscode = statuscode;

    if (statuscode == IEEE80211_STATUS_SUCCESS) {
        /* Enable aggregation! */
        tid->addba_exchangecomplete = 1;

        /* adjust transmitter's BAW size according to ADDBA response */
        tid->baw_size = MIN(baparamset->buffersize, tid->baw_size);
    }
}

/*
 * Process DELBA
 *
 */
void
ath_aggr_delba_process(struct ieee80211com *ic, struct ieee80211_node *ni,
                       struct ieee80211_delba_parameterset *delbaparamset,
                       u_int16_t reasoncode)
{
    struct ath_node 	*an  	= ATH_NODE(ni);
    u_int16_t       	tidno  	= delbaparamset->tid;
    struct ath_arx_tid  *rxtid;
    struct ath_atx_tid  *txtid;

    if (delbaparamset->initiator) {
        /* Teardown reception of aggregates (A-MPDU) */
        rxtid = &an->an_aggr.rx.tid[tidno];
        rxtid->addba_exchangecomplete = 0;
    } else {
        /* Teardown transmission of aggregates (A-MPDU) */
        txtid = ATH_AN_2_TID(an, tidno);
        txtid->addba_exchangecomplete = 0;
        txtid->addba_exchangeattempts = 0;
        txtid->addba_exchangestatuscode = 0;
    }
}

/*
 * Send DELBA
 *
 */
void
ath_aggr_delba_send(struct ieee80211com *ic, struct ieee80211_node *ni,
                u_int8_t ac, u_int8_t initiator, u_int16_t reasoncode)
{
    struct ath_softc *sc = ic->ic_dev->priv;
    struct ath_node *an  = ATH_NODE(ni);
    u_int8_t    tidno = WME_AC_TO_TID(ac);
    ath_atx_tid_t   *tid = ATH_AN_2_TID(an, tidno);
    struct ieee80211_action_mgt_args actionargs;

    /* Stop ADDBA request timer (if ADDBA in progress) */
    ath_aggr_addba_timerstop(sc, tid);

    /* Reset ADDBA information */
    tid->addba_exchangecomplete = 0;
    tid->addba_exchangeattempts = 0;
    tid->addba_exchangestatuscode = 0;

    /* Send DELBA request */
    actionargs.category = IEEE80211_ACTION_CAT_BA;
    actionargs.action   = IEEE80211_ACTION_BA_DELBA;
    actionargs.arg1     = tidno;
    actionargs.arg2     = initiator;
    actionargs.arg3     = reasoncode;
    IEEE80211_SEND_MGMT(ATH_NI(an), IEEE80211_FC0_SUBTYPE_ACTION, (int)&actionargs);
}

/*
 * Clear ADDBA for all tids in this node
 */ 
void 
ath_aggr_addba_clear(struct ath_node *an)
{
    int i;
    ath_atx_tid_t *tid;

    for (i = 0; i < N(an->an_tx_tid); i++) {
        tid = &an->an_tx_tid[i];
        if (tid->addba_exchangecomplete) {
            tid->addba_exchangecomplete = 0;
            tid->addba_exchangeattempts = 0;
            tid->addba_exchangestatuscode = 0;
        }
    }
}

/*
 * ADDBA request timer - timeout
 *
 */
static void
ath_aggr_addba_timertimeout(struct ath_softc *sc, void *timerArg)
{
    ath_atx_tid_t *tid   = ( ath_atx_tid_t *)timerArg;
    tid->addba_exchangeinprogress = 0;
}

/*
 * Start ADDBA request timer
 *
 */
static void
ath_aggr_addba_timerstart(struct ath_softc *sc, ath_atx_tid_t *tid)
{
    if (tid->addba_exchangeinprogress) {
        return;
    }

    tid->addba_exchangeinprogress = 1;

    ath_timer_set(&tid->addba_requesttimer, ath_aggr_addba_timertimeout, tid, ADDBA_TIMEOUT);
    ath_timer_add(sc, &tid->addba_requesttimer);
}

/*
 * Stop ADDBA request timer (eg. if ADDBA response is received before timeout)
 *
 */
static void
ath_aggr_addba_timerstop(struct ath_softc *sc, ath_atx_tid_t *tid)
{
    if (!tid->addba_exchangeinprogress) {
        return;
    }
    tid->addba_exchangeinprogress = 0;

    ath_timer_del(sc, &tid->addba_requesttimer);
}

#define DEV_KFREE_SKB(_skb) do {            \
    struct ieee80211_node*__ni = ATH_SKB_2_NI(skb); \
    dev_kfree_skb(_skb);                \
    if (__ni)                       \
        ieee80211_free_node(__ni);          \
} while (0)

/*
 * driver tx entry
 */
int
owl_hardstart(struct sk_buff *skb, struct net_device *dev)
{
    struct ath_softc    *sc = dev->priv;
    struct ath_node     *an = ATH_NODE(ATH_SKB_2_NI(skb));
    struct ath_txq      *txq;
    int                 retval;

    atrc_init();
    __stats(sc, tx_pkts);

    if (!ath_tx_check(dev, skb, &retval)) {
        __stats(sc, tx_checks);
        return retval;
    }

    /*
     * queue depth too high; do not queue too many packets
     */
    txq = ATH_AC_2_TXQ(sc, skb->priority);
    ATH_TXQ_LOCK_BH(txq);

    if (txq->axq_depth > TAIL_DROP_COUNT) {
        __stats(sc, tx_drops);
        sc->sc_stats.ast_tx_discard++;
        DEV_KFREE_SKB(skb);
        ATH_TXQ_UNLOCK_BH(txq);
        return 0;
    }

    /*
     * check aggregation enabled (HT node and ADDBA exchange completed)
     */
    if (!ath_aggr_query(sc, an, skb)) {
        ath_transmit(sc, txq, skb);
        ATH_TXQ_UNLOCK_BH(txq);
        return 0;
    }

    /*
     * h/w queue depth is low; queue packet now
     */
    if (txq->axq_depth < ATH_AGGR_MIN_QDEPTH) {
        __stats(sc, tx_minqdepth);
        ath_transmit(sc, txq, skb);
        ATH_TXQ_UNLOCK_BH(txq);
        return 0;
    }

    __stats(sc, tx_queue);

    /*
     * h/w queue is deep enough
     * queue packet and schedule destination for tx
     */
    an->an_tx_queue(sc, txq, skb);
    ATH_TXQ_UNLOCK_BH(txq);

    return 0;
}

/*
 * Deferred processing of transmit interrupt.
 */
void
owl_tx_tasklet(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;
    struct ath_softc *sc = dev->priv;
    int i;
    u_int32_t         qcumask = ((1 << HAL_NUM_TX_QUEUES) - 1);
    unsigned long     flags;

    /*
     * avoid race with interrupt handler setting the qcumask
     */
    local_irq_save(flags);
    ath_hal_gettxintrtxqs(sc->sc_ah, &qcumask);
    local_irq_restore(flags);

    /*
     * Process each active queue.
     */
    for (i = 0; i < HAL_NUM_TX_QUEUES; i++)
        if (ATH_TXQ_SETUP(sc, i) && (qcumask & (1 << i))) {
            owl_tx_processq(sc, ATH_TXQ(sc, i), OWL_TXQ_ACTIVE);
        }

    if (sc->sc_softled)
        ath_led_event(sc, ATH_LED_TX);

    /* re-enable tx interrupts */
    local_irq_save(flags);
    sc->sc_imask |= HAL_INT_TX;
    ath_hal_intrset(sc->sc_ah, sc->sc_imask);
    local_irq_restore(flags);
}

void
owl_txq_drain(struct ath_softc *sc, struct ath_txq *txq)
{
    __stats(sc, tx_drain_txq);
    owl_tx_processq(sc, txq, OWL_TXQ_STOPPED);
    ath_txq_drain(sc, txq);
}

/*
 * tx completion processing
 */
void
owl_tx_requeue(struct ath_softc *sc, struct ath_txq *txq)
{
    __stats(sc, tx_requeue);

    /*
     * notify end of filtering to all relevant nodes
     */
    ath_txq_filter_done(sc, txq);

    /*
     * kick start h/w queue again
     */
    ath_txq_schedule(sc, txq);
}

/*
 * notify end of filtering, filtered packets are moved
 * to the beginning of pending packet queue
 */
static void
ath_txq_filter_done(struct ath_softc *sc, struct ath_txq *txq)
{
    struct ath_atx_ac   *ac;
    struct ath_atx_tid  *tid;

    while (!TAILQ_EMPTY(&txq->axq_fltrq)) {
        TAILQ_DEQ(&txq->axq_fltrq, ac, fltr_qelem);
        ac->filtered = AH_FALSE;

        while (!TAILQ_EMPTY(&ac->fltr_q)) {
            TAILQ_DEQ(&ac->fltr_q, tid, fltr_qelem);
            tid->filtered = AH_FALSE;

            /*
             * queue all filtered pkts at the head of the queue
             */
            assert(!TAILQ_EMPTY(&tid->fltr_q));
            TAILQ_INSERTQ_HEAD(&tid->buf_q, &tid->fltr_q, bf_list);
            TAILQ_INIT(&tid->fltr_q);
            ath_tx_enqueue(txq, tid->an, tid->tidno);
        }
    }
}

/*
 * tx completion processing
 */
void
owl_tx_processq(struct ath_softc *sc, struct ath_txq *txq,
                owl_txq_state_t txqstate)
{
    struct ath_buf  *bf;
    struct ath_desc *ds;
    HAL_STATUS      status;

    ATH_TXQ_LOCK_BH(txq);

    for (;;) {
        /*
         * return if no tx pending
         */
        if (TAILQ_EMPTY(&txq->axq_q)) {
            txq->axq_link = NULL;
            txq->axq_linkbuf = NULL;
            break;
        }

        /*
         * look at first buffer in the transmit queue
         */
        bf = TAILQ_FIRST(&txq->axq_q);

        /*
         * check the last desc of buf for completion status
         */
        ds = bf->bf_lastds;

        status = ath_hal_txprocdesc(sc->sc_ah, ds);
        if (status == HAL_EINPROGRESS) {
            if (txqstate == OWL_TXQ_ACTIVE)
                break;
            else if (txqstate == OWL_TXQ_STOPPED) {
                /*
                 * treat the frame as if it completed with the replacement
                 * status rstatus
                 */
                __stats(sc, tx_stopfiltered);
                ds->ds_txstat.ts_flags  = 0;
                ds->ds_txstat.ts_status = HAL_OK;
            } else {
                ds->ds_txstat.ts_flags  = HAL_TX_SW_FILTERED;
            }
        }

        __stats(sc, tx_comps);
        ttrc_ent(bf->bf_seqno);

        /*
         * dequeue the completed buffer from the active txq
         */
        ATH_TXQ_REMOVE_HEAD(txq, bf, bf_list);
        if ((TAILQ_EMPTY(&txq->axq_q))) {
            __stats(sc, tx_qnull);
            txq->axq_link = NULL;
            txq->axq_linkbuf = NULL;
        }
#ifdef DEBUG_PKTLOG
        if(!bf->bf_isaggr) {
            struct log_tx log_data;

            log_data.firstds = bf->bf_desc;
            log_data.bf = bf;
            ath_log_txctl(sc, &log_data, 0);

            log_data.lastds = bf->bf_lastds;
            ath_log_txstatus(sc, &log_data, 0);
        }
#endif

        if (bf->bf_comp)
            bf->bf_comp(sc, bf);
        else
            ath_buf_comp(sc, bf);

        /*
         * schedule any pending packets
         */
        if (txqstate == OWL_TXQ_ACTIVE)
            ath_txq_schedule(sc, txq);
    }

    ATH_TXQ_UNLOCK_BH(txq);
}

/*
 * aggregation receive processing
 */
int
owl_input(struct ieee80211_node *ni, struct sk_buff *skb,
          struct ath_rx_status *rx_stats)
{
    struct ath_node     *an = ATH_NODE(ni);

    __stats(ATH_NI_2_SC(ni), rx_pkts);
    if (rx_stats->rs_isaggr)
        __stats(ATH_NI_2_SC(ni), rx_aggr);

    rtrc_ent(rx_stats->rs_isaggr | (rx_stats->rs_moreaggr << 1));

    return an->an_rx_input(ni, skb, rx_stats->rs_rssi,
                           rx_stats->rs_tstamp);
}

static int
ath_aggr_input(struct ieee80211_node *ni, struct sk_buff *skb, int rssi,
               u_int32_t rstamp)
{
    struct ieee80211_frame      *wh;
    struct ieee80211_qosframe   *whqos;
    u_int8_t                    type, subtype;
    int                         ismcast;
    int                         tid;
    struct ath_arx_tid          *rxtid;
    struct ath_node             *an;
    int                         index, cindex, rxdiff;
    u_int16_t                   rxseq;
    struct ath_rxbuf            *rxbuf;

    an = ATH_NODE(ni);

    wh = (struct ieee80211_frame *)skb->data;

    type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
    subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    ismcast = IEEE80211_IS_MULTICAST(wh->i_addr1);

    if ((type == IEEE80211_FC0_TYPE_CTL) &&
        (subtype == IEEE80211_FC0_SUBTYPE_BAR)) {
        return ath_bar_rx(ni, skb, rssi, rstamp);
    }

    /*
     * collect stats of frames with non-zero version
     */
    if (wh->i_fc[0] & 3) {
        __stats(ATH_NI_2_SC(ni), rx_aggrbadver);
    }

    /*
     * special aggregate processing only for qos unicast data frames
     */
    if (type != IEEE80211_FC0_TYPE_DATA ||
        subtype != IEEE80211_FC0_SUBTYPE_QOS || (ismcast)) {
        __stats(ATH_NI_2_SC(ni), rx_nonqos);
        return ieee80211_input(ni, skb, rssi, rstamp);
    }

    /*
     * lookup rx tid state
     */
    whqos = (struct ieee80211_qosframe *) wh;
    tid = whqos->i_qos[0] & IEEE80211_QOS_TID;
    rxtid = &an->an_aggr.rx.tid[tid];
    rxdiff = (rxtid->baw_tail - rxtid->baw_head) &
             (ATH_TID_MAX_BUFS - 1);

    /*
     * If the ADDBA exchange has not been completed by the source,
     * process via legacy path (i.e. no reordering buffer is needed)
     */
    if (!rxtid->addba_exchangecomplete) {
        __stats(ATH_NI_2_SC(ni), rx_nonqos);
        return ieee80211_input(ni, skb, rssi, rstamp);
    }

    /*
     * extract sequence number from recvd frame
     */
    rxseq = le16toh(*(u_int16_t *)wh->i_seq) >> IEEE80211_SEQ_SEQ_SHIFT;
    rtrc_ent(rxseq);

    if (rxtid->seq_reset) {
        __stats(ATH_NI_2_SC(ni), rx_seqreset);
        rxtid->seq_reset = 0;
        rxtid->seq_next = rxseq;
    }

    index = ATH_BA_INDEX(rxtid->seq_next, rxseq);

    /*
     * drop frame if old sequence (index is too large)
     */
    if (index > (IEEE80211_SEQ_MAX - (rxtid->baw_size << 2))) {
        /*
         * discard frame, ieee layer may not treat frame as a dup
         */
        __stats(ATH_NI_2_SC(ni), rx_oldseq);
        rtrc_ent(rxseq);
        dev_kfree_skb(skb);
        return IEEE80211_FC0_TYPE_DATA;
    }

    /*
     * sequence number is beyond block-ack window
     */
    if (index >= rxtid->baw_size) {

        __stats(ATH_NI_2_SC(ni), rx_bareset);
        rtrc_ent(rxseq);

        /*
         * complete receive processing for all pending frames
         */
        while (index >= rxtid->baw_size) {

            rxbuf = rxtid->rxbuf + rxtid->baw_head;

            if (rxbuf->rx_skb) {
                __stats(ATH_NI_2_SC(ni), rx_baresetpkts);
                ieee80211_input(ni, rxbuf->rx_skb, rxbuf->rx_rssi,
                                rxbuf->rx_tsf);
                rxbuf->rx_skb = NULL;
            }

            INCR(rxtid->baw_head, ATH_TID_MAX_BUFS);
            INCR(rxtid->seq_next, IEEE80211_SEQ_MAX);

            index --;
        }
    }


    /*
     * add buffer to the recv ba window
     */
    cindex = (rxtid->baw_head + index) & (ATH_TID_MAX_BUFS - 1);
    rxbuf = rxtid->rxbuf + cindex;

    if (rxbuf->rx_skb) {
        /*
         *duplicate frame
         */
        __stats(ATH_NI_2_SC(ni), rx_dup);
        rtrc_ent(rxseq);
        dev_kfree_skb(skb);
        return IEEE80211_FC0_TYPE_DATA;
    }

    rxbuf->rx_skb  = skb;
    rxbuf->rx_tsf  = rstamp;
    rxbuf->rx_time = jiffies;
    rxbuf->rx_rssi = rssi;

    /*
     * advance tail if sequence received is newer than any received so far
     */
    if (index >= rxdiff) {
        __stats(ATH_NI_2_SC(ni), rx_baadvance);
        rxtid->baw_tail = cindex;
        INCR(rxtid->baw_tail, ATH_TID_MAX_BUFS);
    }

    /*
     * indicate all in-order received frames
     */
    while (rxtid->baw_head != rxtid->baw_tail) {
        rxbuf = rxtid->rxbuf + rxtid->baw_head;
        if (!rxbuf->rx_skb)
            break;

        rtrc_ent(rxseq);
        rtrc_ent(rxbuf->rx_skb->len);

        __stats(ATH_NI_2_SC(ni), rx_recvcomp);
        ieee80211_input(ni, rxbuf->rx_skb, rxbuf->rx_rssi, rxbuf->rx_tsf);
        rxbuf->rx_skb = NULL;

        INCR(rxtid->baw_head, ATH_TID_MAX_BUFS);
        INCR(rxtid->seq_next, IEEE80211_SEQ_MAX);
    }

    /*
     * start a timer to flush all received frames if there are pending
     * receive frames
     */
    if (rxtid->baw_head != rxtid->baw_tail)
        ath_rx_timer_start(ATH_NI_2_SC(ni), rxtid);
    else
        ath_rx_timer_stop(ATH_NI_2_SC(ni), rxtid);

    return IEEE80211_FC0_TYPE_DATA;
}

static void
ath_rx_timer(struct ath_softc *sc, void *tmr_arg)
{
    struct ath_arx_tid          *rxtid = (struct ath_arx_tid *) tmr_arg;
    unsigned long               now    = jiffies;
    unsigned long               tstamp;
    struct ieee80211_frame      *wh;
    struct ath_rxbuf            *rxbuf;

    rxtid->timer_act = 0;

    while (rxtid->baw_head != rxtid->baw_tail) {

        rxbuf = rxtid->rxbuf + rxtid->baw_head;
        if (!rxbuf->rx_skb) {
            INCR(rxtid->baw_head, ATH_TID_MAX_BUFS);
            INCR(rxtid->seq_next, IEEE80211_SEQ_MAX);
            __stats(sc, rx_skipped);
            continue;
        }

        /*
         * stop if the next one is a very recent frame
         */
        tstamp = rxbuf->rx_time + ATH_RX_TIMEOUT;
        if (time_before(now, tstamp))
            break;

        wh = (struct ieee80211_frame *)rxbuf->rx_skb->data;
        rtrc_ent(le16toh(*(u_int16_t *)wh->i_seq) >> IEEE80211_SEQ_SEQ_SHIFT);
        rtrc_ent(rxbuf->rx_skb->len);

        __stats(sc, rx_recvcomp);
        __stats(sc, rx_comp_to);
        ieee80211_input(ATH_NI(rxtid->an), rxbuf->rx_skb, rxbuf->rx_rssi,
                        rxbuf->rx_tsf);
        rxbuf->rx_skb = NULL;

        INCR(rxtid->baw_head, ATH_TID_MAX_BUFS);
        INCR(rxtid->seq_next, IEEE80211_SEQ_MAX);
    }

    /*
     * start a timer to flush all received frames if there are pending
     * receive frames
     */
    if (rxtid->baw_head != rxtid->baw_tail)
        ath_rx_timer_start(sc, rxtid);
    else
        ath_rx_timer_stop(sc, rxtid);

}

static void
ath_rx_timer_start(struct ath_softc *sc, struct ath_arx_tid *rxtid)
{
    if (rxtid->timer_act)
        return;
    rxtid->timer_act = 1;

    ath_timer_set(&rxtid->timer, ath_rx_timer, rxtid, ATH_RX_TIMEOUT);
    ath_timer_add(sc, &rxtid->timer);
}

static void
ath_rx_timer_stop(struct ath_softc *sc, struct ath_arx_tid *rxtid)
{
    if (!rxtid->timer_act)
        return;
    rxtid->timer_act = 0;

    ath_timer_del(sc, &rxtid->timer);
}

/*
 * timer functions
 */

void
ath_timer_init(struct ath_softc *sc)
{
    struct ath_timer_list *ath_timer = &sc->sc_timer;
    struct timer_list *timer = &ath_timer->timer;

    /*
     * stop timer if already active
     */
    if (ath_timer->timer_active)
        ath_timer_exit(sc);

    /*
     * init timer before use
     */
    init_timer(timer);

    /*
     * setup timer params
     */
    timer->data     = (unsigned long) sc;
    timer->function = ath_timer_run;
    timer->expires  = jiffies + ATH_TIMER_TICKS;

    /*
     * initialize ath timer fields
     */
    TAILQ_INIT(&ath_timer->timer_q);
    ath_timer->timer_active = 1;

    /*
     * start the periodic timer
     */
    add_timer(timer);

    /*
     * also start watchdog timer
     */
    ath_wd_start(sc);
}

static void
ath_timer_run(unsigned long timer_arg)
{
    struct ath_softc      *sc        = (struct ath_softc *) timer_arg;
    struct ath_timer_list *ath_timer = &sc->sc_timer;
    struct timer_list     *timer     = &ath_timer->timer;
    ath_timerq_t          timer_q;
    struct ath_timer      *tmr, *tmr_next;

    TAILQ_INIT(&timer_q);

    /*
     * iterate through all pending timer elements and adjust ticks left
     */
    TAILQ_FOREACH_SAFE(tmr, &ath_timer->timer_q, tmr_elem, tmr_next) {
        tmr->tmr_ticks -= ATH_TIMER_TICKS;
        if (tmr->tmr_ticks <= 0) {
            TAILQ_REMOVE(&ath_timer->timer_q, tmr, tmr_elem);
            TAILQ_INSERT_TAIL(&timer_q, tmr, tmr_elem);
        }
    }

    /*
     * fire all expired timers
     */
    while (!TAILQ_EMPTY(&timer_q)) {
        TAILQ_DEQ(&timer_q, tmr, tmr_elem);
        assert(tmr->tmr_active);
        tmr->tmr_active = 0;
        tmr->tmr_func(sc, tmr->tmr_arg);
    }

    /*
     * re-arm timer if timer is not shutdown
     */
    if (ath_timer->timer_active) {
        timer->expires += ATH_TIMER_TICKS;
        add_timer(timer);
    }
}

void
ath_timer_exit(struct ath_softc *sc)
{
    /*
     * stop watchdog timer first
     */
    ath_wd_stop(sc);

    del_timer(&sc->sc_timer.timer);
    sc->sc_timer.timer_active = 0;
}

void
ath_timer_set(struct ath_timer *tmr, ath_timer_func_t tmr_func, void *tmr_arg,
              int tmr_ticks)
{
    tmr->tmr_func  = tmr_func;
    tmr->tmr_arg   = tmr_arg;
    tmr->tmr_ticks = tmr_ticks;
}

void
ath_timer_add(struct ath_softc *sc, struct ath_timer *tmr)
{
    if (tmr->tmr_active)
        return;

    tmr->tmr_active = 1;
    TAILQ_INSERT_TAIL(&sc->sc_timer.timer_q, tmr, tmr_elem);
}

void
ath_timer_del(struct ath_softc *sc, struct ath_timer *tmr)
{
    if (!tmr->tmr_active)
        return;

    tmr->tmr_active = 0;
    TAILQ_REMOVE(&sc->sc_timer.timer_q, tmr, tmr_elem);
}

/*
 * watchdog timer for linux
 * linux built-in watchdog timer is not sufficient to catch h/w hangs
 * when device queue is not stopped, see bug 19084
 */
#define ATH_WATCHDOG_TIMEOUT MSECS(3000)

static void
ath_wd_run(struct ath_softc *sc, void *arg)
{
    struct net_device   *dev = sc->sc_dev;
    struct ath_txq      *txq;
    int                 ac, nact = 0;
    u_int32_t           txe;
#define AR_Q_TXE 0x840

    /*
     * if no timeout, restart watchdog timer
     */
    if ((jiffies - dev->trans_start) <= dev->watchdog_timeo) {
        __stats(sc, wd_tx_active);
        ath_wd_start(sc);
        return;
    }

    /*
     * timeout, look at active tx count
     */
    for (ac = WME_AC_BE; ac <= WME_AC_VO; ac++) {
        txq = ATH_AC_2_TXQ(sc, ac);
        nact += txq->axq_depth;
    }

    /*
     * no tx packets queued
     */
    if (!nact) {
        __stats(sc, wd_tx_inactive);
        ath_wd_start(sc);
        return;
    }

    txe = ath_hal_reg_read(sc->sc_ah, AR_Q_TXE);
    if (!txe) {
        __stats(sc, wd_spurious);
        ath_wd_start(sc);
        return;
    }

    /*
     * tx hung
     */
    __stats(sc, wd_tx_hung);
    dev->tx_timeout(dev);

    /*
     * restart watchdog timer again
     */
    ath_wd_start(sc);
#undef AR_Q_TXE
}

static void
ath_wd_start(struct ath_softc *sc)
{
    ath_timer_set(&sc->sc_watchdog, ath_wd_run, sc, ATH_WATCHDOG_TIMEOUT);
    ath_timer_add(sc, &sc->sc_watchdog);
}

static void
ath_wd_stop(struct ath_softc *sc)
{
    ath_timer_del(sc, &sc->sc_watchdog);
}


/*
 *----------------------------------------------------------------------
 * local functions
 *----------------------------------------------------------------------
 */

/*
 * check state of driver before processing tx requests
 */
static int
ath_tx_check(struct net_device *dev, struct sk_buff *skb, int *retval)
{
    struct ath_softc        *sc = dev->priv;
    struct ieee80211_node   *ni;
    struct ath_node         *an;

    /*
     * check device and sc state
     */
    if ((dev->flags & IFF_RUNNING) == 0 || sc->sc_invalid) {
        sc->sc_stats.ast_tx_invalid++;
        *retval = -ENETDOWN;
        DEV_KFREE_SKB(skb);
        return AH_FALSE;
    }

    /*
     * check if node info is setup
     */
    ni = ((struct ieee80211_cb *) skb->cb)->ni;
    if (ni == NULL) {
        *retval = 0;
        DEV_KFREE_SKB(skb);
        return AH_FALSE;
    }

    /*
     * setup tx functions
     */
    an = ATH_NODE(ni);
    if (!an->an_tx_queue)
        ath_node_txsetup(sc, an);

    /*
     * clear any FF vestiges from past usage
     */
    ATH_FF_MAGIC_CLR(skb);
    return AH_TRUE;
}

/*
 * setup/initialize node struct before first transmit
 */
static void
ath_node_txsetup(struct ath_softc *sc, struct ath_node *an)
{
    struct ieee80211_node   *ni = (struct ieee80211_node *) an;

    an->an_tx_queue = ath_tx_queue_normal;
    an->an_tx_sched = ath_tx_sched_normal;
    an->an_tx_comp  = ath_tx_comp_normal;
    an->an_tx_seqno = ath_tx_seqno_normal;

    /*
     * enable fast-frames or a-mpdu aggregation only if fragmentation
     * is disabled
     */
    if (ni->ni_vap->iv_fragthreshold >= IEEE80211_FRAG_THRESH) {
        if (0 && ni->ni_ath_flags & IEEE80211_ATHC_FF) {
            an->an_tx_queue = ath_tx_queue_ff;
            an->an_tx_sched = ath_tx_sched_ff;
            an->an_tx_comp  = ath_tx_comp_ff;
            an->an_tx_seqno = ath_tx_seqno_ff;
            an->an_rx_input = ieee80211_input;
        }
        else if ((sc->sc_ic.ic_flags_ext & IEEE80211_C_AMPDU) &&
                 (ni->ni_flags & IEEE80211_NODE_QOS) &&
                 (ni->ni_flags & IEEE80211_NODE_HT)) {
            an->an_tx_queue = ath_tx_queue_aggr;
            an->an_tx_sched = ath_tx_sched_aggr;
            an->an_tx_comp  = ath_tx_comp_aggr;
            an->an_tx_seqno = ath_tx_seqno_aggr;
            an->an_rx_input = ath_aggr_input;
        }
    }
}

/*
 * init node struct before first use (tx or rx)
 */
void
owl_node_init(struct ath_softc *sc, struct ath_node *an)
{
    struct ath_atx_tid      *tid;
    struct ath_atx_ac       *ac;
    struct ath_arx_tid      *tidrx;
    int                     tidno, acno, i;

    /* Init tx */
    an->an_tx_hwqcnt = 0;
    an->an_tx_queue = 0;
    an->an_tx_sched = 0;
    an->an_tx_comp  = 0;
    an->an_tx_seqno = 0;

    /* Init rx */
    an->an_rx_input = ieee80211_input;

    /*
     * init per tid tx state
     */
    for (tidno = 0, tid = &an->an_tx_tid[tidno]; tidno < WME_NUM_TID;
         tidno++, tid++) {
        tid->tidno     = tidno;
        tid->seq_start = tid->seq_next = 0;
        tid->baw_size  = WME_MAX_BA;
        tid->baw_head  = tid->baw_tail = 0;
        tid->sched     = AH_FALSE;
        tid->filtered  = AH_FALSE;
        TAILQ_INIT(&tid->buf_q);
        TAILQ_INIT(&tid->fltr_q);
        tid->an        = an;
        acno = TID_TO_WME_AC(tidno);
        tid->ac        = &an->an_tx_ac[acno];
        for (i = 0; i < ATH_TID_MAX_BUFS; i++)
            tid->tx_buf[i] = NULL;
    /*
     * ADDBA state
     */
    tid->addba_exchangecomplete     = 0;
    tid->addba_exchangeinprogress   = 0;
    tid->addba_exchangeattempts     = 0;
    tid->addba_exchangestatuscode   = 0;
    }

    /*
     * init per ac tx state
     */
    for (acno = 0, ac = &an->an_tx_ac[acno]; acno < WME_NUM_AC; acno++, ac++) {
        ac->sched    = AH_FALSE;
        ac->filtered = AH_FALSE;
        ac->priority = acno;
        ac->hwqcnt   = 0;
        ac->ff_skb   = NULL;
        TAILQ_INIT(&ac->tid_q);
        TAILQ_INIT(&ac->fltr_q);
    }

    /*
     * init per tid rx state
     */
    for (tidno = 0, tidrx = &an->an_rx_tid[tidno]; tidno < WME_NUM_TID;
         tidno++, tidrx++) {
        tidrx->an        = an;
        tidrx->seq_reset = 1;
        tidrx->seq_next  = 0;
        tidrx->baw_size  = WME_MAX_BA;
        tidrx->baw_head  = tidrx->baw_tail = 0;
        tidrx->timer_act = 0;
        for (i = 0; i < ATH_TID_MAX_BUFS; i++)
            tidrx->rxbuf[i].rx_skb = NULL;
	/*
	 * ADDBA state
	 */
	tidrx->addba_exchangecomplete     = 0;

    }
}

/*
 * prepare a tx frame for queueing
 */
static struct ath_buf *
ath_tx_prepare(struct ath_softc *sc, struct ath_txq *txq, struct sk_buff *skb,
               ath_bufhead *bf_q)
{
    struct ieee80211_node   *ni = ATH_SKB_2_NI(skb);
    struct ath_buf          *bf;
    struct sk_buff          *tskb;
    int                     framecnt;
    int                     nfl;

    /*
     * 802.11 encapsulate
     */
    skb = ieee80211_encap(ni, skb, &framecnt);
    if (skb == NULL) {
        __stats(sc, tx_noskbs);
        sc->sc_stats.ast_tx_encap ++;
        return NULL;
    }

    /*
     * allocate buffers, one for each fragment
     */
    bf = ath_buf_alloc(sc, bf_q, framecnt);
    if (!bf) {
        __stats(sc, tx_nobufs);
        ath_skb_free(skb);
        return NULL;
    }

    do {
        tskb = skb->next;
        skb->next = NULL;
        nfl = 0;
        if (tskb)
            nfl = tskb->len;

        ATH_BUF_SET(bf, ni, txq, skb, nfl);
	//sometimes the ni value is not set properly
	// we set it explicitely to avoid ni->vap giving a crash

	ATH_SKB_2_NI(bf->bf_skb)=ni; 
        if (ath_txbuf_setup(sc, bf) < 0)
            goto prep_failed;

        /*
         * map skb for dma
         */
        ath_dma_map(sc, bf);

        /*
         * next fragment
         */
        bf  = TAILQ_NEXT(bf, bf_list);
        skb = tskb;
    } while (bf);

    return TAILQ_FIRST(bf_q);

 prep_failed:
    ttrc_ent(bf->bf_seqno);
    __stats(sc, tx_badsetups);
    /*
     * free remaining skbs
     */
    skb->next = tskb;
    ath_skb_free(skb);
    bf->bf_skb = NULL;

    /*
     * unmap and free all previous skbs
     */
    for (bf = TAILQ_PREV(bf, ath_bufhead_s, bf_list); bf;
         bf = TAILQ_PREV(bf, ath_bufhead_s, bf_list)) {
        ath_dma_unmap(sc, bf);
        ath_skb_free(bf->bf_skb);
        bf->bf_skb = NULL;
    }

    ath_buf_free(sc, bf_q);
    return NULL;
}

/*
 * ath_transmit - queue a packet directly to h/w when queue depth is low
 */
static void
ath_transmit(struct ath_softc *sc, struct ath_txq *txq, struct sk_buff *skb)
{
    ath_bufhead     bf_q;
    struct ath_buf  *bf, *bfnext;
    struct ath_node *an;
    ath_atx_tid_t   *tid;
    struct ath_rc_series rcs[4];
    int             prate, rf = 0;

    bf = ath_tx_prepare(sc, txq, skb, &bf_q);
    if (!bf) {
        __stats(sc, tx_nobufs);
        return;
    }

    an = ATH_NODE(bf->bf_node);
    while (bf) {
        bfnext = TAILQ_NEXT(bf, bf_list);
        ath_tx_setds(sc, bf);

        if (!bf->bf_ismcast && ath_aggr_query(sc, an, skb)) {
            tid = ATH_AN_2_TID(an, bf->bf_tidno);

            /*
             * do not queue to h/w ahead of any pending frames
             * also do not not queue to h/w if not within block-ack window
             */
            if (!TAILQ_EMPTY(&tid->buf_q) ||
                !BAW_WITHIN(tid->seq_start, tid->baw_size, bf->bf_seqno)) {
                TAILQ_INSERT_TAIL(&tid->buf_q, bf, bf_list);
                ath_tx_enqueue(txq, an, bf->bf_tidno);
                return;
            }

            ath_tx_addto_baw(tid, bf);
        }

        if (bf->bf_ismcast) {
            bf->bf_rcs[1].tries = bf->bf_rcs[2].tries = bf->bf_rcs[3].tries = 0;
            bf->bf_rcs[1].rix = bf->bf_rcs[2].rix = bf->bf_rcs[3].rix = 0;
            bf->bf_rcs[0].rix   = 0;    /* lowest rate index */
            bf->bf_rcs[0].tries = ATH_TXMAXTRY - 1;
            bf->bf_rcs[0].flags = 0;
        } else {
            /*
             * We decided to send this unicast buffer. So, do _one_ rate lookup.
             */
            if (!rf) {
                ath_rate_findrate(sc, an, skb->len, ATH_TXMAXTRY-1,
                                  sc->sc_mrretries, 1, ATH_RC_PROBE_ALLOWED,
                                  rcs, &prate);
                rf = 1;
            }
            memcpy(bf->bf_rcs, rcs, sizeof(rcs));
        }

        ttrc_ent(bf->bf_seqno);
        ttrc_ent(bf->bf_pktlen);

        ath_buf_set_rate(sc, bf);
        bf->bf_txq_add(sc, bf);

        bf = bfnext;
    }
}

/*
 * s/w queue tx packets to legacy destinations
 */
static void
ath_tx_queue_normal(struct ath_softc *sc, struct ath_txq *txq,
                    struct sk_buff *skb)
{
    struct ath_node         *an = ATH_SKB_2_AN(skb);
    ath_bufhead             bf_q;
    struct ath_buf          *bf;
    int                     tidno;
    struct ath_atx_tid      *tid;

    /*
     * prepare tx buffers
     */
    bf = ath_tx_prepare(sc, txq, skb, &bf_q);
    if (!bf) {
        __stats(sc, tx_normnobufs);
        return;
    }

    tidno = bf->bf_tidno;
    tid   = ATH_AN_2_TID(an, tidno);

    /*
     * preset s/w queue time ath desc fields
     */
    while (bf) {
        ath_tx_setds(sc, bf);
        bf = TAILQ_NEXT(bf, bf_list);
    }

    /*
     * add bufs to tid and schedule dest/ac pair
     */
    TAILQ_CONCAT(&tid->buf_q, &bf_q, bf_list);
    ath_tx_enqueue(txq, an, tidno);
}

/*
 * s/w queue tx packets to legacy destinations capable of fast-frames
 */
static void
ath_tx_queue_ff(struct ath_softc *sc, struct ath_txq *txq, struct sk_buff *skb)
{
    struct ieee80211_node   *ni = ATH_SKB_2_NI(skb);
    struct ath_node         *an = ATH_NODE(ni);
    struct ath_atx_ac       *ac = ATH_AN_2_AC(an, skb->priority);
    struct sk_buff          *pskb;
    struct ath_buf          *bf;
    struct ath_atx_tid      *tid;
    ath_bufhead             bf_q;
    int                     framecnt;
    int                     tidno = WME_AC_TO_TID(skb->priority);

    /*
     * pending frame
     */
    pskb = ac->ff_skb;
    ac->ff_skb = NULL;

    /*
     * if cannot aggregate the current frame, queue any pending frame
     * and current frame without fast-framing
     */
    if (!ath_tx_can_ff(sc, an, skb, pskb)) {
        if (pskb)
            ath_tx_queue_normal(sc, txq, pskb);
        ath_tx_queue_normal(sc, txq, skb);
        return;
    }

    /*
     * if no pending frame, hold frame for fast-framing with
     * another frame
     */
    if (!pskb) {
        ac->ff_skb = skb;
        ath_tx_enqueue(txq, an, tidno);
        return;
    }

    /*
     * 2 frames available for fast framing
     * magic so that fast-frame encapsulation is done
     */
    pskb->next = skb;
    ATH_FF_MAGIC_PUT(pskb);
    skb = ieee80211_encap(ni, pskb, &framecnt);
    if (skb == NULL) {
        sc->sc_stats.ast_tx_encap ++;
        return;
    }

    /*
     * allocate buffer and ...
     */
    bf = ath_buf_alloc(sc, &bf_q, framecnt);
    if (!bf) {
        ath_skb_free(skb);
        return;
    }

    /*
     * prepare buffer and ...
     */
    ATH_BUF_SET(bf, ni, txq, skb, 0);
    if (ath_txbuf_setup(sc, bf) < 0) {
        ath_skb_free(skb);
        bf->bf_skb = NULL;
        ath_buf_free(sc, &bf_q);
        return;
    }
    ath_dma_map(sc, bf);

    /*
     * preset s/w queue time ath desc fields
     */
    ath_tx_setds(sc, bf);

    /*
     * add buf to tid buffer queue and schedule dest/ac pair
     */
    tid = ATH_AN_2_TID(an, tidno);
    TAILQ_INSERT_TAIL(&tid->buf_q, bf, bf_list);
    ath_tx_enqueue(bf->bf_txq, an, tidno);
}

/*
 * s/w queue tx packet to destinations with support for a-mpdu aggregation
 */
void
ath_tx_queue_aggr(struct ath_softc *sc, struct ath_txq *txq,
                  struct sk_buff *skb)
{
    struct ieee80211_node   *ni;
    struct ath_node         *an;
    ath_bufhead             bf_q;
    struct ath_buf          *bf;
    int                     framecnt;
    struct ath_atx_tid      *tid;

    ni = ATH_SKB_2_NI(skb);
    an = ATH_NODE(ni);

    /*
     * encapsulate first
     */
    skb = ieee80211_encap(ni, skb, &framecnt);
    if (skb == NULL) {
        __stats(sc, txaggr_noskbs);
        sc->sc_stats.ast_tx_encap ++;
        return;
    }

    /*
     * next allocate buffer
     */
    bf = ath_buf_alloc(sc, &bf_q, framecnt);
    if (!bf) {
        __stats(sc, txaggr_nobufs);
        ath_skb_free(skb);
        return;
    }

    /*
     * then prepare buffer
     */
    ATH_BUF_SET(bf, ni, txq, skb, 0);
    if (ath_txbuf_setup(sc, bf) < 0) {
        __stats(sc, txaggr_badkeys);
        ath_skb_free(skb);
        bf->bf_skb = NULL;
        ath_buf_free(sc, &bf_q);
        return;
    }
    ath_dma_map(sc, bf);

    /*
     * preset s/w queue time ath desc fields
     */
    ath_tx_setds(sc, bf);

    ttrc_ent(bf->bf_seqno);
    ttrc_ent(bf->bf_pktlen);

    /*
     * add buf to tid buffer queue and schedule dest/ac pair
     */
    tid = ATH_AN_2_TID(an, bf->bf_tidno);
    TAILQ_INSERT_TAIL(&tid->buf_q, bf, bf_list);
    ath_tx_enqueue(txq, an, bf->bf_tidno);
}

/*
 * for legacy destinations, use seqno from ieee80211 layer
 */
static void
ath_tx_seqno_normal(struct ath_buf *bf)
{
    struct ieee80211_frame  *wh = ATH_SKB_2_WH(bf->bf_skb);

    bf->bf_seqno = le16toh(*(u_int16_t *)wh->i_seq >> IEEE80211_SEQ_SEQ_SHIFT);
}

/*
 * for legacy ff destinations, use seqno from ieee80211 layer
 */
static void
ath_tx_seqno_ff(struct ath_buf *bf)
{
    ath_tx_seqno_normal(bf);
}

/*
 * for 11n aggregate destinations generate sequence number
 */
static void
ath_tx_seqno_aggr(struct ath_buf *bf)
{
    struct ath_node         *an  = ATH_SKB_2_AN(bf->bf_skb);
    struct ieee80211_frame  *wh  = ATH_SKB_2_WH(bf->bf_skb);
    struct ath_atx_tid      *tid = ATH_AN_2_TID(an, bf->bf_tidno);

    *(u_int16_t *)wh->i_seq = htole16(tid->seq_next << IEEE80211_SEQ_SEQ_SHIFT);
    bf->bf_seqno = tid->seq_next;
    INCR(tid->seq_next, IEEE80211_SEQ_MAX);
}

static u_int32_t
ath_ff_approx_txtime(struct ath_softc *sc, struct ath_node *an,
                     struct sk_buff *skb, struct sk_buff *pskb)
{
    u_int32_t txtime;
    u_int32_t framelen;

    /*
     * Approximate the frame length to be transmitted. A swag to add
     * the following maximal values to the skb payload:
     *   - 32: 802.11 encap + CRC
     *   - 24: encryption overhead (if wep bit)
     *   - 4 + 6: fast-frame header and padding
     *   - 16: 2 LLC FF tunnel headers
     *   - 14: 1 802.3 FF tunnel header (skb already accounts for 2nd)
     */
    framelen = skb->len + 32 + 4 + 6 + 16 + 14;
    if (sc->sc_ic.ic_flags & IEEE80211_F_PRIVACY)
        framelen += 24;

    if (pskb)
        framelen += pskb->len;

    txtime = ath_hal_computetxtime(sc->sc_ah, sc->sc_currates, framelen,
                                   an->an_prevdatarix, AH_FALSE);
    return txtime;
}


/*
 * return whether a packet can be fast-framed
 */
static int
ath_tx_can_ff(struct ath_softc *sc, struct ath_node *an, struct sk_buff *skb,
              struct sk_buff *pskb)
{
    struct ieee80211com *ic = &sc->sc_ic;
    struct ether_header *eh = (struct ether_header *) skb->data;
    u_int32_t           txoplimit;

    /*
     * do not aggregate multicast frames on AP
     */
    if ((ic->ic_opmode == IEEE80211_M_HOSTAP) &&
        ETHER_IS_MULTICAST(eh->ether_dhost))
        return AH_FALSE;

    if (sc->sc_currates->info[an->an_prevdatarix].rateKbps < an->an_minffrate)
        return AH_FALSE;

    txoplimit =
        IEEE80211_TXOP_TO_US(ic->ic_wme.wme_chanParams.cap_wmeParams
                             [skb->priority].wmep_txopLimit);

    if (txoplimit != 0 && ath_ff_approx_txtime(sc, an, skb, pskb) > txoplimit)
        return AH_FALSE;

    return AH_TRUE;
}

/*
 * setup descriptors for a normal or fast-frame packet
 * dslink - set to next buffer's desc for chaining
 */
static void
ath_tx_setds(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_desc *ds = bf->bf_desc;
    struct sk_buff  *skb;
    int             dscnt = 1;

    /*
     * first descriptor
     */
    skb = bf->bf_skb;

    switch (bf->bf_protmode) {
    	case IEEE80211_PROT_RTSCTS:
            bf->bf_flags |= HAL_TXDESC_RTSENA;
            break;
    	case IEEE80211_PROT_CTSONLY:
            bf->bf_flags |= HAL_TXDESC_CTSENA;
            break;
    	default:
            break;
    }

    ath_hal_set11n_txdesc(sc->sc_ah, ds
                          , bf->bf_pktlen
                          , bf->bf_atype
                          , MIN(bf->bf_node->ni_txpower, 60)
                          , bf->bf_keyix
                          , bf->bf_keytype
                          , bf->bf_flags | HAL_TXDESC_INTREQ);

    ds->ds_data = bf->bf_skbaddr;
    ds->ds_link = skb->next ? bf->bf_daddrs[dscnt] : 0;
    ath_hal_filltxdesc(sc->sc_ah, ds, skb->len, AH_TRUE, !skb->next, ds);
    bf->bf_lastds = ds;
#ifdef DEBUG_PKTLOG
    ds->ds_vdata = (u_int32_t)skb->data;
#endif

    while (skb->next) {
        skb = skb->next;
        ds ++;

        ds->ds_data = bf->bf_skbaddrs[dscnt];
        dscnt ++;

        ds->ds_link = skb->next ? bf->bf_daddrs[dscnt] : 0;
        ath_hal_filltxdesc(sc->sc_ah, ds, skb->len, AH_FALSE, !skb->next, ds);
        bf->bf_lastds = ds;
    }
}

/*
 * queue up a dest/ac pair for tx scheduling
 */
static void
ath_tx_enqueue(struct ath_txq *txq, struct ath_node *an, int tidno)

{
    struct ath_atx_tid  *tid;
    struct ath_atx_ac   *ac;

    tid = ATH_AN_2_TID(an, tidno);
    ac  = tid->ac;

    /*
     * add tid to ac atmost once
     */
    if (tid->sched)
        return;

    tid->sched = AH_TRUE;
    TAILQ_INSERT_TAIL(&ac->tid_q, tid, tid_qelem);

    /*
     * add node ac to txq atmost once
     */
    if (ac->sched)
        return;

    ac->sched = AH_TRUE;
    TAILQ_INSERT_TAIL(&txq->axq_acq, ac, ac_qelem);
}

/*
 * free a skb chain (linked by skb->next)
 */
static void
ath_skb_free(struct sk_buff *skb)
{
    struct sk_buff *tskb;

    while (skb) {
        tskb = skb->next;
        skb->next = NULL;
        DEV_KFREE_SKB(skb);
        skb = tskb;
    }
}

/*
 * allocate a list of (framecnt number of) buf structures
 * return the first of the buf from the list
 * return NULL on failure to allocate framecnt buffers
 */
static struct ath_buf *
ath_buf_alloc(struct ath_softc *sc, ath_bufhead *bf_q, int framecnt)
{
    struct ath_buf *bf = NULL;

    TAILQ_INIT(bf_q);

    ATH_TXBUF_LOCK_BH(sc);

    /*
     * allocate "framecnt" buffers
     */
    while (framecnt && !TAILQ_EMPTY(&sc->sc_txbuf)) {
        bf = TAILQ_FIRST(&sc->sc_txbuf);
        bf->bf_isaggr = bf->bf_isretried = bf->bf_retries = 0;
        TAILQ_REMOVE(&sc->sc_txbuf, bf, bf_list);
        TAILQ_INSERT_TAIL(bf_q, bf, bf_list);
        framecnt --;
    }

    /*
     * all or nothing; put back partial allocations on free list
     */
    if (framecnt)
        TAILQ_CONCAT(&sc->sc_txbuf, bf_q, bf_list);

    /*
     * stop higher layers if no more buffers
     */
    if (TAILQ_EMPTY(&sc->sc_txbuf)) {
        sc->sc_stats.ast_tx_qstop++;
        netif_stop_queue(sc->sc_dev);
        sc->sc_devstopped = 1;
    }

    ATH_TXBUF_UNLOCK_BH(sc);

    /*
     * could not allocate all
     */
    if (framecnt)
        return NULL;

    return TAILQ_FIRST(bf_q);
}

#define ATH_NETIF_WAKE(_sc, _bf_cnt)                                    \
    do {                                                                \
        if ((_sc)->sc_devstopped) {                                     \
            (_sc)->sc_reapcount += (_bf_cnt);                           \
            if ((_sc)->sc_reapcount > ATH_TXBUF_FREE_THRESHOLD) {       \
                netif_wake_queue((_sc)->sc_dev);                        \
                (_sc)->sc_reapcount  = 0;                               \
                (_sc)->sc_devstopped = 0;                               \
            }                                                           \
        }                                                               \
    } while (0)

/*
 * free the list of buffers allocated; any associated skbs should
 * be separately freed
 */
static void
ath_buf_free(struct ath_softc *sc, ath_bufhead *bf_q)
{
    struct ath_buf  *bf;
    int             bf_cnt;

    ATH_TXBUF_LOCK_BH(sc);

    bf_cnt = 0;
    bf = TAILQ_FIRST(bf_q);
    while (bf) {
        ATH_DS_TOGGLE(bf, 0);
        bf->bf_skb  = NULL;
        bf->bf_comp = NULL;
        bf_cnt ++;
        bf = TAILQ_NEXT(bf, bf_list);
    }

    ATH_NETIF_WAKE(sc, bf_cnt);
    TAILQ_CONCAT(&sc->sc_txbuf, bf_q, bf_list);

    ATH_TXBUF_UNLOCK_BH(sc);
}

/*
 * free buffer
 */
static void
ath_buf_comp(struct ath_softc *sc, struct ath_buf *bf)
{
    ATH_TXBUF_LOCK_BH(sc);

    ath_dma_unmap(sc, bf);
    ath_skb_free(bf->bf_skb);
    bf->bf_skb  = NULL;
    ATH_DS_TOGGLE(bf, 0);

    ATH_NETIF_WAKE(sc, 1);
    TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf, bf_list);

    ATH_TXBUF_UNLOCK_BH(sc);
}

/*
 * do all rate independent processing/setup
 */
static int
ath_txbuf_setup(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_node         *an = ATH_NODE(bf->bf_node);
    struct ieee80211_frame  *wh = ATH_SKB_2_WH(bf->bf_skb);
    struct ieee80211com     *ic = &sc->sc_ic;
    struct ieee80211_node   *ni = ATH_SKB_2_NI(bf->bf_skb);
    struct ieee80211vap     *vap = ni->ni_vap;
    int                     retval;

    /*
     * set TID based on packet priority
     */
    bf->bf_tidno = WME_AC_TO_TID(bf->bf_skb->priority);

    /*
     * setup tx sequence number
     */
    an->an_tx_seqno(bf);

    /*
     * setup frag, mcast state
     */
    bf->bf_istxfrag = ATH_WH_IS_FRAG(wh);
    bf->bf_ismcast  = IEEE80211_IS_MULTICAST(wh->i_addr1);

    bf->bf_txq_add = ath_txq_add_ucast;
    if (bf->bf_ismcast && an->an_isap) {
        bf->bf_txq_add = ath_txq_add_mcast;
        bf->bf_comp    = NULL;
    }

    /*
     * get header and total length of packet
     */
    bf->bf_hdrlen = ieee80211_anyhdrsize(wh);
    bf->bf_pktlen = ath_get_pktlen(bf, bf->bf_hdrlen);

    /*
     * encryption setup for the frame(or fragment)
     */
    if ((retval = ath_key_setup(bf->bf_node, bf)) < 0)
        return retval;

    /*
     * setup rate table
     */
    bf->bf_rt = sc->sc_currates;

    /*
     * short preamble usage
     */
    bf->bf_shpream = AH_FALSE;

    if ((ic->ic_flags & IEEE80211_F_SHPREAMBLE) &&
        (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE)) {
        bf->bf_shpream = AH_TRUE;
        sc->sc_stats.ast_tx_shortpre++;
    }

    /*
     * set flags to default
     */
    bf->bf_flags = HAL_TXDESC_CLRDMASK;

    /*
     * rate independent processing
     */
    ath_set_frame_type(sc, bf);
    ath_check_hidden_prot(sc, bf);

    /*
     * multicast processing
     */
    if (bf->bf_ismcast && an->an_isap) {
        bf->bf_txq = &ATH_VAP(vap)->av_mcastq;
        bf->bf_flags |= HAL_TXDESC_NOACK;
        sc->sc_stats.ast_tx_noack++;
    }

    /*
     * check ofdm protection flags
     */
    ic = &sc->sc_ic;
    bf->bf_protmode = IEEE80211_PROT_NONE;
    if (((ic->ic_flags & IEEE80211_F_USEPROT) || 
         (ic->ic_flags_ext & IEEE80211_C_HTPROT)) && 
        !(bf->bf_flags & HAL_TXDESC_NOACK) && 
        !ath_aggr_prot) {

        if (ic->ic_protmode == IEEE80211_PROT_RTSCTS)
            bf->bf_protmode = IEEE80211_PROT_RTSCTS;
        else if (ic->ic_protmode == IEEE80211_PROT_CTSONLY)
            bf->bf_protmode = IEEE80211_PROT_CTSONLY;

        sc->sc_stats.ast_tx_protect++;
    }

    return 0;
}

static void
ath_set_frame_type(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ieee80211_frame  *wh;
    u_int8_t                ft;

    wh  = ATH_SKB_2_WH(bf->bf_skb);

    /*
     * get frame type and do corresponding processing
     */
    ft = (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) >> IEEE80211_FC0_TYPE_SHIFT;
    ath_ft_process[ft].set_atype(sc, bf);
    bf->bf_ft = ft;
}

static void
ath_txq_add_ucast(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_hal  *ah  = sc->sc_ah;
    struct ath_txq  *txq = bf->bf_txq;
    struct ath_node *an;
    ath_atx_ac_t    *ac;

    ATH_TXQ_INSERT_TAIL(txq, bf, bf_list);

    if (txq->axq_link == NULL) {
        ath_hal_puttxbuf(ah, txq->axq_qnum, bf->bf_daddr);
    } else {
        *txq->axq_link = bf->bf_daddr;
    }

    txq->axq_link = &bf->bf_lastds->ds_link;
    ath_hal_txstart(ah, txq->axq_qnum);
    sc->sc_dev->trans_start = jiffies;

    sc->sc_devstats.tx_packets++;
    sc->sc_devstats.tx_bytes += bf->bf_pktlen;

    /*
     * collect active packet counts for 20/40 transitions
     * exclude unicast but non-data pkts (eg. bar)
     */
    if (bf->bf_comp) {
        an = ATH_NODE(bf->bf_node);
        ac = ATH_AN_2_TID(an, bf->bf_tidno)->ac;
        an->an_tx_hwqcnt ++;
        ac->hwqcnt ++;
    }
}

static void
ath_txq_add_mcast(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_hal      *ah     = sc->sc_ah;
    struct ath_txq      *txq    = bf->bf_txq;

    /*
     * The CAB queue is started from the SWBA handler since
     * frames only go out on DTIM and to avoid possible races.
     */
    ath_hal_intrset(ah, sc->sc_imask & ~HAL_INT_SWBA);
    ATH_TXQ_INSERT_TAIL(txq, bf, bf_list);

    if (txq->axq_link != NULL) {
        *txq->axq_link = bf->bf_daddr;
    }
    txq->axq_link = &bf->bf_lastds->ds_link;
    ath_hal_intrset(ah, sc->sc_imask);

    sc->sc_devstats.tx_packets++;
    sc->sc_devstats.tx_bytes += bf->bf_pktlen;
}

static void
ath_check_hidden_prot(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ieee80211_node   *ni = bf->bf_node;
    struct ieee80211vap     *vap = ni->ni_vap;

    /*
     * rts-cts based on threshold; ignore if fast-frames enabled
     */
    if (!bf->bf_ismcast && (bf->bf_pktlen > vap->iv_rtsthreshold)) {
        if(!(vap->iv_ath_cap & bf->bf_node->ni_ath_flags
             & IEEE80211_ATHC_FF)) {
            bf->bf_flags |= HAL_TXDESC_RTSENA;
            sc->sc_stats.ast_tx_rts++;
        }
    }
}

static void
ath_ft_mgt_set_atype(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ieee80211_frame  *wh = ATH_SKB_2_WH(bf->bf_skb);
    u_int32_t               subtype;

    subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    switch (subtype) {
    case IEEE80211_FC0_SUBTYPE_BEACON:
        bf->bf_atype = HAL_PKT_TYPE_BEACON;
        break;

    case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
        bf->bf_atype = HAL_PKT_TYPE_PROBE_RESP;
        break;

    case IEEE80211_FC0_SUBTYPE_ATIM:
        bf->bf_atype = HAL_PKT_TYPE_ATIM;
        break;

    default:
        bf->bf_atype = HAL_PKT_TYPE_NORMAL;
    }

    if (bf->bf_node->ni_flags & IEEE80211_NODE_QOS)
        bf->bf_txq = ATH_AC_2_TXQ(sc, WME_AC_VO);
    else
        bf->bf_txq = ATH_AC_2_TXQ(sc, WME_AC_BE);
}

static void
ath_ft_mgt_get_rate(struct ath_softc *sc, struct ath_buf *bf)
{
    bf->bf_rcs[1].tries = bf->bf_rcs[2].tries = bf->bf_rcs[3].tries = 0;
    bf->bf_rcs[0].rix  = sc->sc_minrateix;
    bf->bf_rcs[0].tries = ATH_TXMAXTRY;
    bf->bf_rcs[0].flags = 0;
}

static void
ath_ft_ctl_set_atype(struct ath_softc *sc, struct ath_buf *bf)
{
    bf->bf_atype = HAL_PKT_TYPE_PSPOLL;

    if (bf->bf_node->ni_flags & IEEE80211_NODE_QOS)
        bf->bf_txq = ATH_AC_2_TXQ(sc, WME_AC_VO);
    else
        bf->bf_txq = ATH_AC_2_TXQ(sc, WME_AC_BE);
}

static void
ath_ft_ctl_get_rate(struct ath_softc *sc, struct ath_buf *bf)
{
    bf->bf_rcs[1].tries = bf->bf_rcs[2].tries = bf->bf_rcs[3].tries = 0;
    bf->bf_rcs[0].rix  = sc->sc_minrateix;
    bf->bf_rcs[0].tries = ATH_TXMAXTRY;
    bf->bf_rcs[0].flags = 0;
}

static void
ath_ft_data_set_atype(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ieee80211_frame  *wh = ATH_SKB_2_WH(bf->bf_skb);
    int                     priority = bf->bf_skb->priority;
    struct ath_node         *an = ATH_NODE(bf->bf_node);

    bf->bf_atype = HAL_PKT_TYPE_NORMAL;

    if (bf->bf_ismcast && an->an_isap) {
        bf->bf_txq = sc->sc_cabq;
        bf->bf_flags |= HAL_TXDESC_NOACK;
        return;
    }

    /*
     * Default all non-QoS traffic to the best-effort queue.
     */
    if (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_QOS) {
        bf->bf_txq = ATH_AC_2_TXQ(sc, priority & 0x3);
        if (sc->sc_ic.ic_wme.wme_wmeChanParams.
            cap_wmeParams[priority].wmep_noackPolicy) {
            bf->bf_flags |= HAL_TXDESC_NOACK;
            sc->sc_stats.ast_tx_noack++;
        }
    } else {
        bf->bf_txq = ATH_AC_2_TXQ(sc, WME_AC_BE);
    }
}

static void
ath_ft_data_get_rate(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_node         *an = ATH_NODE(bf->bf_node);
    struct ieee80211vap     *vap = bf->bf_node->ni_vap;
    int                     prate;

    KASSERT(0, ("get_rate switched on frame type?"));
    if (!bf->bf_ismcast || !an->an_isap)
        ath_rate_findrate(sc, an, bf->bf_skb->len, ATH_TXMAXTRY-1, sc->sc_mrretries, 1,
                      ATH_RC_PROBE_ALLOWED , bf->bf_rcs, &prate);
    else {
        bf->bf_rcs[1].tries = bf->bf_rcs[2].tries = bf->bf_rcs[3].tries = 0;
        bf->bf_rcs[0].rix   = ath_tx_findindex(bf->bf_rt, vap->iv_mcast_rate);
        bf->bf_rcs[0].tries = ATH_TXMAXTRY;
        bf->bf_rcs[0].flags = 0;
    }
}

static void
ath_dma_map(struct ath_softc *sc, struct ath_buf *bf)
{
    struct sk_buff  *skb = bf->bf_skb;
    int             i;

    bf->bf_skbaddr = bus_map_single(sc->sc_bdev, skb->data, skb->len,
                                    BUS_DMA_TODEVICE);

    for (i = 0, skb = skb->next; skb; skb = skb->next, i++) {
        bf->bf_skbaddrs[i] = bus_map_single(sc->sc_bdev, skb->data, skb->len,
                                            BUS_DMA_TODEVICE);
    }
    bf->bf_numdesc = i + 1;
}

static void
ath_dma_unmap(struct ath_softc *sc, struct ath_buf *bf)
{
    struct sk_buff  *skb = bf->bf_skb;
    int             i;

    bus_unmap_single(sc->sc_bdev, bf->bf_skbaddr, skb->len, BUS_DMA_TODEVICE);

    for (i = 0, skb = skb->next; skb; skb = skb->next, i++) {
        bus_unmap_single(sc->sc_bdev, bf->bf_skbaddrs[i], skb->len,
                         BUS_DMA_TODEVICE);
    }
}

static int
ath_key_setup(struct ieee80211_node *ni, struct ath_buf *bf)
{
    struct ieee80211_frame          *wh = ATH_SKB_2_WH(bf->bf_skb);
    const struct ieee80211_cipher   *cip;
    struct ieee80211_key            *k;

    bf->bf_keytype = HAL_KEY_TYPE_CLEAR;

    if (!(wh->i_fc[1] & IEEE80211_FC1_WEP)) {
        /*
         * no encryption needed
         */
        bf->bf_keyix = HAL_TXKEYIX_INVALID;
        if (ni->ni_ucastkey.wk_cipher == &ieee80211_cipher_none) {
            /*
             * Use station key cache slot, if assigned.
             */
            if (ni->ni_ucastkey.wk_keyix != IEEE80211_KEYIX_NONE)
                bf->bf_keyix = ni->ni_ucastkey.wk_keyix;
        }
        return 0;
    }

    /*
     * encrypted packets
     */
    k = ieee80211_crypto_encap(ni, bf->bf_skb);
    if (k == NULL)
        return -EIO;

    cip = k->wk_cipher;
    bf->bf_hdrlen += cip->ic_header;
    bf->bf_pktlen += cip->ic_header + cip->ic_trailer;

    if ((k->wk_flags & IEEE80211_KEY_SWMIC) == 0) {
        if ( ! bf->bf_istxfrag )
            bf->bf_pktlen += cip->ic_miclen;
        else {
            if (cip->ic_cipher != IEEE80211_CIPHER_TKIP)
                bf->bf_pktlen += cip->ic_miclen;
        }
    }
    bf->bf_keyix = k->wk_keyix;

    /*
     * owl requires key type now
     */
    switch (cip->ic_cipher) {
    case IEEE80211_CIPHER_WEP:
    case IEEE80211_CIPHER_CKIP:
        bf->bf_keytype = HAL_KEY_TYPE_WEP;
        break;
    case IEEE80211_CIPHER_TKIP:
        bf->bf_keytype = HAL_KEY_TYPE_TKIP;
        break;
    case IEEE80211_CIPHER_AES_OCB:
    case IEEE80211_CIPHER_AES_CCM:
        bf->bf_keytype = HAL_KEY_TYPE_AES;
        break;
    default:
        bf->bf_keytype = HAL_KEY_TYPE_CLEAR;
    }

    return 0;
}

static int
ath_get_pktlen(struct ath_buf *bf, int hdrlen)
{
    struct sk_buff *skb = bf->bf_skb;
    int         pktlen;

    pktlen = skb->len;

    while (skb->next) {
        skb = skb->next;
        pktlen += skb->len;
    }
    pktlen -= (hdrlen & 3);
    pktlen += IEEE80211_CRC_LEN;

    return pktlen;
}

/*
 *----------------------------------------------------------------------
 * TX aggregation logic
 *----------------------------------------------------------------------
 */

/*
 * return length of mac frame
 */
static u_int16_t
ath_aggr_limit(struct ath_softc *sc, u_int32_t ratekBps)
{

    /*
     * TODO:
     * 1. get txop
     * 2. subtract const HT overhead per frame
     * 3. this give time left for mac frame
     * 4. covert that into length using ratekbps
     * 5. return length in bytes
     */

    /* Currently harcoded to 2ms duration for each
     ** aggregate */
    return (MIN(ratekBps*4, IEEE80211_AMPDU_LIMIT_MAX));
}

/*
 * looks up the rate
 * returns aggr limit based on lowest of the rates
 */
static u_int32_t
ath_lookup_rate(struct ath_softc *sc, struct ath_node *an, struct ath_buf *bf)
{
    int                     i, prate;
    u_int32_t               rateKbps=0, minrate = 1;
    u_int16_t               aggr_limit, legacy=0;
    const HAL_RATE_TABLE    *rt = sc->sc_currates;
    struct ieee80211_node   *ieee_node = (struct ieee80211_node *)an;

    if (bf->bf_ismcast) {
        bf->bf_rcs[1].tries = bf->bf_rcs[2].tries = bf->bf_rcs[3].tries = 0;
        bf->bf_rcs[0].rix   = 0xb;
        bf->bf_rcs[0].tries = ATH_TXMAXTRY - 1;
        bf->bf_rcs[0].flags = 0;
    } else {
        ath_rate_findrate(sc, an, 0, ATH_TXMAXTRY-1, sc->sc_mrretries, 1,
                          ATH_RC_PROBE_ALLOWED, bf->bf_rcs, &prate);
    }

    for (i = 0; i < 4; i++) {
        if (bf->bf_rcs[i].tries) {
            rateKbps = rt->info[bf->bf_rcs[i].rix].rateKbps;

            if (bf->bf_rcs[i].flags & ATH_RC_CW40_FLAG) {
                rateKbps = rateKbps  * 27 / 13;
            }

            if (rt->info[bf->bf_rcs[i].rix].phy != IEEE80211_T_HT) {
                legacy = 1;
                break;
            }
            if (bf->bf_rcs[i].flags & ATH_RC_SGI_FLAG) {
                rateKbps = rateKbps  * 10 / 9;
            }
            /* Assume first series entry is minimum value */
            if (!i)
                minrate     = rateKbps;

            minrate = MIN(minrate, rateKbps);
        }
    }

    /*
     * limit aggregate size by the minimum rate if rate selected is
     * not a probe rate, if rate selected is a probe rate then
     * avoid aggregation of this packet.
     */
    if (prate || legacy)
        return 0;

    aggr_limit = ath_aggr_limit(sc, minrate/8);

    aggr_limit = MIN(aggr_limit, sc->sc_ic.ic_ampdu_limit);

    /*
     * h/w can accept aggregates upto 16 bit lengths (65535). The IE, however
     * can hold upto 65536, which shows up here as zero. Ignore 65536 since we 
     * are constrained by hw.
     */ 
    if (ieee_node->ni_maxampdu)
        aggr_limit = MIN(aggr_limit, ieee_node->ni_maxampdu);

    return aggr_limit;
}

/*
 * process pending frames possibly doing a-mpdu aggregation
 * - called with txq lock held
 */
static void
ath_tx_sched_aggr(struct ath_softc *sc, ath_atx_tid_t *tid)
{
    struct ath_buf  *bf, *bf_last;
    ATH_AGGR_STATUS status;
    ath_bufhead bf_q;
    struct ath_txq *txq;

    txq = ATH_AC_2_TXQ(sc, tid->ac->priority);

    do {
        if (TAILQ_EMPTY(&tid->buf_q))
            return;

        TAILQ_INIT(&bf_q);

        status = ath_tx_form_aggr(sc, tid, &bf_q);

        /*
         * no frames picked up to be aggregated; block-ack window is not open
         */
        if (TAILQ_EMPTY(&bf_q))
            return;

        bf = TAILQ_FIRST(&bf_q);
        bf_last = TAILQ_LAST(&bf_q, ath_bufhead_s);

        /*
         * if only frame, send as non-aggregate
         */
        if (bf->bf_nframes == 1) {
            __stats(sc, txaggr_single);
            bf->bf_isaggr = 0;
            bf->bf_lastds = bf->bf_desc;
            bf->bf_desc->ds_link = 0;

            ttrc_ent(bf->bf_seqno);

            ath_hal_clr11n_aggr(sc->sc_ah, bf->bf_desc);
            ath_buf_set_rate(sc, bf);
            bf->bf_txq_add(sc, bf);
            continue;
        }

        /*
         * no padding required for last packet
         */
        bf_last->bf_next = NULL;
        bf_last->bf_desc->ds_link = 0;
        bf_last->bf_ndelim = 0;

        /*
         * setup first desc with rate and aggr info
         */
        bf->bf_isaggr  = 1;
        ath_buf_set_rate(sc, bf);
        ath_hal_set11n_aggr_first(sc->sc_ah, bf->bf_desc, bf->bf_al,
                                  bf->bf_ndelim);
        bf->bf_lastds = bf_last->bf_desc;

        ttrc_ent(bf->bf_al);
        ttrc_ent(bf->bf_nframes);

        /*
         * anchor last frame of aggregate correctly
         */
        ath_hal_set11n_aggr_last(sc->sc_ah, bf_last->bf_desc);

        /*
         * queue aggregate to hardware
         */
        bf->bf_txq_add(sc, bf);
    } while (txq->axq_depth < ATH_AGGR_MIN_QDEPTH &&
             status != ATH_AGGR_BAW_CLOSED);
}

/*
 * process pending frames for a legacy destination
 * - called with txq lock held
 */
static void
ath_tx_sched_normal(struct ath_softc *sc, ath_atx_tid_t *tid)
{
    struct ath_buf  *bf;
    struct ath_node *an = tid->an;
    struct ath_rc_series rcs[4];
    int    prate;
    /*
     * lookup rate first
     */
    KASSERT(0, ("Legacy tx scheduling for an aggregate destination?"));
    ath_rate_findrate(sc, an, 0, ATH_TXMAXTRY-1, sc->sc_mrretries, 1,
                  ATH_RC_PROBE_ALLOWED , rcs, &prate);

    TAILQ_DEQ(&tid->buf_q, bf, bf_list);
    while (bf) {
        memcpy(bf->bf_rcs, rcs, sizeof(rcs));
        ath_buf_set_rate(sc, bf);
        bf->bf_txq_add(sc, bf);

        TAILQ_DEQ(&tid->buf_q, bf, bf_list);
    }
}

/*
 * process pending frames for a destination capable of fast-frames
 * - called with txq lock held
 */
static void
ath_tx_sched_ff(struct ath_softc *sc, ath_atx_tid_t *tid)
{
    struct sk_buff  *skb;
    struct ath_txq  *txq;

    if (TAILQ_EMPTY(&tid->buf_q)) {
        skb = tid->ac->ff_skb;
        tid->ac->ff_skb = NULL;

        txq = ATH_AC_2_TXQ(sc, skb->priority);
        ATH_TXQ_UNLOCK_BH(txq);
        ath_transmit(sc, txq, skb);
        ATH_TXQ_LOCK_BH(txq);
        return;
    }

    ath_tx_sched_normal(sc, tid);
}

/*
 * TX scheduling logic
 * - called with txq lock held
 */
static void
ath_txq_schedule(struct ath_softc *sc, struct ath_txq *txq)
{
    struct ath_atx_ac   *ac;
    struct ath_atx_tid  *tid;

    /*
     * check if nothing to schedule
     */
    if (TAILQ_EMPTY(&txq->axq_acq)) {
        __stats(sc, tx_schednone);
        return;
    }

    /*
     * dequeue first node/ac entry on the queue
     */
    TAILQ_DEQ(&txq->axq_acq, ac, ac_qelem);
    ac->sched = AH_FALSE;

    /*
     * process a single tid per destination
     */
    TAILQ_DEQ(&ac->tid_q, tid, tid_qelem);
    tid->sched = AH_FALSE;

    tid->an->an_tx_sched(sc, tid);

    /*
     * add tid to round-robin queue if more frames are pending for the tid
     */
    if (!TAILQ_EMPTY(&tid->buf_q))
        ath_tx_enqueue(txq, tid->an, tid->tidno);
}

/*
 * Drain all pending buffers
 */
static void
ath_txq_drain(struct ath_softc *sc, struct ath_txq *txq)
{
    struct ath_atx_ac   *ac;
    struct ath_atx_tid  *tid;

    while (!TAILQ_EMPTY(&txq->axq_acq)) {
        TAILQ_DEQ(&txq->axq_acq, ac, ac_qelem);
        ac->sched = AH_FALSE;

        while (!TAILQ_EMPTY(&ac->tid_q)) {
            TAILQ_DEQ(&ac->tid_q, tid, tid_qelem);
            tid->sched = AH_FALSE;

            ath_tid_drain(sc, tid);
        }
    }
}

static void
ath_tid_drain(struct ath_softc *sc, struct ath_atx_tid *tid)
{
    struct ath_buf  *bf;

    __stats(sc, tx_drain_tid);

    while (!TAILQ_EMPTY(&tid->buf_q)) {
        TAILQ_DEQ(&tid->buf_q, bf, bf_list);

        ath_tx_freebuf(sc, bf);
        __stats(sc, tx_drain_bufs);
    }

    /*
     * TODO: For frame(s) that are in the retry state, we will reuse the 
     * sequence number(s) without setting the retry bit. The alternative is to
     * give up on these and BAR the receiver's window forward.
     */
    tid->seq_next = tid->seq_start;
    tid->baw_tail = tid->baw_head;
}

static void
ath_tx_comp_normal(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_node *an;
    ath_atx_ac_t    *ac;
    struct ath_desc *ds = bf->bf_lastds;

    /*
     * unicast completions
     */
    if (bf->bf_node) {
        /*
         * update active pkt count in hw queues
         */
        an = ATH_NODE(bf->bf_node);
        ac = ATH_AN_2_TID(an, bf->bf_tidno)->ac;
        an->an_tx_hwqcnt --;
        ac->hwqcnt --;

        /*
         * queue filtered packets
         */
        if (ds->ds_txstat.ts_flags == HAL_TX_SW_FILTERED)
            return ath_tx_comp_filtered(sc, bf);

        ath_tx_uc_comp(sc, bf);
    }

    /*
     * free buffer resources
     */
    ath_tx_freebuf(sc, bf);
}

/*
 * completion processing for destinations capable of fast frames
 */
static void
ath_tx_comp_ff(struct ath_softc *sc, struct ath_buf *bf)
{
    bf->bf_lastds = bf->bf_desc;
    ath_tx_comp_normal(sc, bf);
}

static void
ath_tx_freebuf(struct ath_softc *sc, struct ath_buf *bf)
{
    ath_dma_unmap(sc, bf);

    ath_skb_free(bf->bf_skb);
    bf->bf_skb  = NULL;
    bf->bf_comp = NULL;
    ATH_DS_TOGGLE(bf, 0);

    ATH_TXBUF_LOCK_BH(sc);

    ATH_NETIF_WAKE(sc, 1);
    TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf, bf_list);

    ATH_TXBUF_UNLOCK_BH(sc);
}

/*
 * Handle unicast completions
 * - update completion statistics
 * - hand the descriptor to the rate control algorithm
 * - reclaim reference to node.
 */
static void
ath_tx_uc_comp(struct ath_softc *sc, struct ath_buf *bf)
{
    ath_update_stats(sc, bf);
    ath_rate_tx_complete(sc, ATH_NODE(bf->bf_node), bf->bf_lastds, bf->bf_rcs, 1, 0);
}

static void
ath_update_stats(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ieee80211_node   *ni = bf->bf_node;
    struct ath_desc         *ds = bf->bf_desc;
    struct ath_node         *an = ATH_NODE(ni);
    u_int32_t               sr, lr;

    if (ds->ds_txstat.ts_status == 0) {
        u_int8_t txant = ds->ds_txstat.ts_antenna;
        sc->sc_stats.ast_ant_tx[txant]++;
        sc->sc_ant_tx[txant]++;

        if (bf->bf_numdesc>1)
            ni->ni_vap->iv_stats.is_tx_ffokcnt++;
        if (ds->ds_txstat.ts_rate & HAL_TXSTAT_ALTRATE)
            sc->sc_stats.ast_tx_altrate++;
        sc->sc_stats.ast_tx_rssi_combined = ds->ds_txstat.ts_rssi;
        sc->sc_stats.ast_tx_rssi_ctl0 = ds->ds_txstat.ts_rssi_ctl0;
        sc->sc_stats.ast_tx_rssi_ctl1 = ds->ds_txstat.ts_rssi_ctl1;
        sc->sc_stats.ast_tx_rssi_ctl2 = ds->ds_txstat.ts_rssi_ctl2;
        sc->sc_stats.ast_tx_rssi_ext0 = ds->ds_txstat.ts_rssi_ext0;
        sc->sc_stats.ast_tx_rssi_ext1 = ds->ds_txstat.ts_rssi_ext1;
        sc->sc_stats.ast_tx_rssi_ext2 = ds->ds_txstat.ts_rssi_ext2;
        ATH_RSSI_LPF(an->an_halstats.ns_avgtxrssi,
                     ds->ds_txstat.ts_rssi);
        if (bf->bf_skb->priority == WME_AC_VO ||
            bf->bf_skb->priority == WME_AC_VI)
            ni->ni_ic->ic_wme.wme_hipri_traffic++;
        ni->ni_inact = ni->ni_inact_reload;
    } else {
        if (bf->bf_numdesc>1)
            ni->ni_vap->iv_stats.is_tx_fferrcnt++;
        if (ds->ds_txstat.ts_status & HAL_TXERR_XRETRY)
            sc->sc_stats.ast_tx_xretries++;
        if (ds->ds_txstat.ts_status & HAL_TXERR_FIFO)
            sc->sc_stats.ast_tx_fifoerr++;
        if (ds->ds_txstat.ts_status & HAL_TXERR_FILT)
            sc->sc_stats.ast_tx_filtered++;
        if (ds->ds_txstat.ts_status & HAL_TXERR_TIMER_EXPIRED)
            sc->sc_stats.ast_tx_timer_exp ++;
    }
    sr = ds->ds_txstat.ts_shortretry;
    lr = ds->ds_txstat.ts_longretry;
    sc->sc_stats.ast_tx_shortretry += sr;
    sc->sc_stats.ast_tx_longretry += lr;
}

/*
 *----------------------------------------------------------------------
 * Aggregate completion processing
 *----------------------------------------------------------------------
 */

static void
ath_bar_tx(struct ath_softc *sc, ath_atx_tid_t *tid, struct ath_buf *bf)
{
    struct sk_buff              *skb = bf->bf_skb;
    struct ieee80211_frame_bar  *bar;
    u_int8_t                    minrate = 0x0B; // todo
    struct ath_desc             *ds;
    HAL_11N_RATE_SERIES         series[4] = {{ 0 }};

    __stats(sc, tx_bars);

    bar = (struct ieee80211_frame_bar *) skb->data;

    /*
     * free all resources
     */
    ath_dma_unmap(sc, bf);
    if (skb->next) {
        ath_skb_free(skb->next);
        skb->next = NULL;
    }

    /*
     * form the bar frame
     */
    bar->i_fc[1]  = IEEE80211_FC1_DIR_NODS;
    bar->i_fc[0]  = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_CTL |
        IEEE80211_FC0_SUBTYPE_BAR;
    bar->i_ctl    = tid->tidno << IEEE80211_BAR_CTL_TID_S |
        IEEE80211_BAR_CTL_COMBA;
    bar->i_seq    = htole16(tid->seq_start << IEEE80211_SEQ_SEQ_SHIFT);
    bf->bf_seqno  = tid->seq_start;

    /*
     * set len to bar frame
     */
    skb->len = sizeof(struct ieee80211_frame_bar);

    /*
     * setup buf
     */
    bf->bf_comp = ath_bar_tx_comp;
    ath_dma_map(sc, bf);

    /*
     * setup desc
     */
    ds = bf->bf_desc;
#ifdef DEBUG_PKTLOG
    ds->ds_vdata = (u_int32_t)skb->data;
#endif
    ath_hal_setuptxdesc(sc->sc_ah, ds
                        , skb->len + IEEE80211_CRC_LEN      /* pktlen   */
                        , 0                                 /* hdrlen   */
                        , HAL_PKT_TYPE_NORMAL               /* atype    */
                        , MIN(bf->bf_node->ni_txpower, 60)  /* tx power */
                        , minrate                           /* rate0    */
                        , ATH_TXMAXTRY                      /* tries    */
                        , bf->bf_keyix                      /* keyindex */
                        , 0                                 /* antenna  */
                        , HAL_TXDESC_INTREQ 
                        | HAL_TXDESC_CLRDMASK               /* flags    */
                        , 0, 0, 0, 0
                        , ATH_COMP_PROC_NO_COMP_NO_CCS
                        );

    ds->ds_data = bf->bf_skbaddr;
    ds->ds_link = 0;
    ath_hal_filltxdesc(sc->sc_ah, ds, skb->len, AH_TRUE, AH_TRUE, ds);
    bf->bf_lastds = ds;

    series[0].Tries = ATH_TXMAXTRY;
    series[0].Rate = minrate;
    series[0].ChSel = sc->sc_ic.ic_tx_chainmask;
    ath_hal_set11n_ratescenario(sc->sc_ah, ds, 0, 0, series, 4, 0);

    ttrc_ent(tid->seq_start);
    ath_txq_add_ucast(sc, bf);
}

static void
ath_bar_tx_comp(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_desc         *ds  = bf->bf_lastds;
    u_int16_t               seq_st;
    u_int32_t               *ba;

    seq_st = ATH_DS_BA_SEQ(ds);
    ba     = ATH_DS_BA_BITMAP(ds);

    AMPDU_TRCADD(sc, 0, 0, bf->bf_seqno, seq_st, ba[0], ba[1]);
    bf->bf_comp = NULL;
    ath_buf_comp(sc, bf);

    // AMPDU_TRCEND(sc);
}

static int
ath_bar_rx(struct ieee80211_node *ni, struct sk_buff *skb, int rssi,
           u_int32_t rstamp)
{
    struct ath_node                *an;
    struct ieee80211_frame_bar    *bar;
    int                            tidno;
    u_int16_t                     seqno;
    struct ath_arx_tid            *rxtid;
    int                         index, cindex;
    struct sk_buff              *tskb;

    __stats(ATH_NI_2_SC(ni), rx_bars);

    an = ATH_NODE(ni);

    /*
     * look at BAR contents
     */
    bar = (struct ieee80211_frame_bar *) skb->data;
    tidno = (bar->i_ctl & IEEE80211_BAR_CTL_TID_M) >> IEEE80211_BAR_CTL_TID_S;
    seqno = le16toh(bar->i_seq) >> IEEE80211_SEQ_SEQ_SHIFT;

    /*
     * process BAR - indicate all pending RX frames till the BAR seqno
     */
    rxtid = &an->an_aggr.rx.tid[tidno];

    /*
     * get relative index
     */
    index = ATH_BA_INDEX(rxtid->seq_next, seqno);

    /*
     * drop BAR if old sequence (index is too large)
     */
    if ((index > rxtid->baw_size) &&
        (index > (IEEE80211_SEQ_MAX - (rxtid->baw_size << 2)))) {
        /*
         * discard frame, ieee layer may not treat frame as a dup
         */
        __stats(ATH_NI_2_SC(ni), rx_bardiscard);
        rtrc_ent(seqno);
        dev_kfree_skb(skb);
        return IEEE80211_FC0_TYPE_CTL;
    }

    /*
     * complete receive processing for all pending frames upto BAR seqno
     */
    cindex = (rxtid->baw_head + index) & (ATH_TID_MAX_BUFS - 1);
    while ((rxtid->baw_head != rxtid->baw_tail) &&
           (rxtid->baw_head != cindex)) {
        tskb = rxtid->rxbuf[rxtid->baw_head].rx_skb;
        rxtid->rxbuf[rxtid->baw_head].rx_skb = NULL;

        if (tskb) {
            __stats(ATH_NI_2_SC(ni), rx_barcomps);
            ieee80211_input(ni, tskb, rssi, rstamp);
        }

        INCR(rxtid->baw_head, ATH_TID_MAX_BUFS);
        INCR(rxtid->seq_next, IEEE80211_SEQ_MAX);
    }

    /*
     * ... and indicate rest of the frames in-order
     */
    while (rxtid->baw_head != rxtid->baw_tail &&
           rxtid->rxbuf[rxtid->baw_head].rx_skb) {
        tskb = rxtid->rxbuf[rxtid->baw_head].rx_skb;
        rxtid->rxbuf[rxtid->baw_head].rx_skb = NULL;

        __stats(ATH_NI_2_SC(ni), rx_barrecvs);
        ieee80211_input(ni, tskb, rssi, rstamp);

        INCR(rxtid->baw_head, ATH_TID_MAX_BUFS);
        INCR(rxtid->seq_next, IEEE80211_SEQ_MAX);
    }

    /*
     * free bar itself
     */
    dev_kfree_skb(skb);
    return IEEE80211_FC0_TYPE_CTL;
}


static void
ath_tx_comp_filtered(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_node *an = ATH_NODE(bf->bf_node);
    ath_atx_tid_t   *tid = ATH_AN_2_TID(an, bf->bf_tidno);
    ath_atx_ac_t    *ac = tid->ac;
    struct ath_txq  *txq = ATH_AC_2_TXQ(sc, ac->priority);
    struct ath_buf  *bf_next;

    if (!bf->bf_isaggr) {
        __stats(sc, tx_unaggr_filtered);
        bf->bf_next = NULL;
    } else {
        __stats(sc, tx_aggr_filtered);
    }

    while (bf) {
        __stats(sc, tx_filtered);
        bf_next  = bf->bf_next;

        /*
         * avoid updating block-ack window again
         */
        ath_tx_set_retry(sc, bf);
        TAILQ_INSERT_TAIL(&tid->fltr_q, bf, bf_list);
        bf = bf_next;
    }

    /*
     * record that ac/tid has fileterd pkts
     */
    if (tid->filtered)
        return;

    tid->filtered = AH_TRUE;
    TAILQ_INSERT_TAIL(&ac->fltr_q, tid, fltr_qelem);

    if (ac->filtered)
        return;

    ac->filtered = AH_TRUE;
    TAILQ_INSERT_TAIL(&txq->axq_fltrq, ac, fltr_qelem);
}

/*
 * unaggregated pkt completion for an aggregate destination
 */
static void
ath_tx_comp_unaggr(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_node     *an = ATH_NODE(bf->bf_node);
    ath_atx_tid_t       *tid = ATH_AN_2_TID(an, bf->bf_tidno);
    struct ath_desc     *ds  = bf->bf_lastds;

    ath_update_stats(sc, bf);

    AMPDU_TRCADD(sc, 1, bf->bf_pktlen, bf->bf_seqno, 0, 0, 0);
    ath_rate_tx_complete(sc, an, ds, bf->bf_rcs, 1, 0);

    if (ATH_DS_TX_STATUS(ds) & HAL_TXERR_XRETRY) {
        ath_tx_retry_unaggr(sc, bf);
        return;
    }

    __stats(sc, tx_compunaggr);
    ttrc_ent(bf->bf_seqno);

    ath_tx_update_baw(tid, bf->bf_seqno);
    ath_tx_freebuf(sc, bf);
}

static inline void
ath_tx_retry_unaggr(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_node     *an = ATH_NODE(bf->bf_node);
    ath_atx_tid_t       *tid = ATH_AN_2_TID(an, bf->bf_tidno);
    struct ath_txq      *txq = ATH_AC_2_TXQ(sc, tid->ac->priority);

    if (bf->bf_retries == OWLMAX_RETRIES) {
        ttrc_ent(bf->bf_seqno);
        __stats(sc, txunaggr_xretry);

        ath_tx_update_baw(tid, bf->bf_seqno);
        ath_bar_tx(sc, tid, bf);            
        return;
    }

    __stats(sc, txunaggr_compretries);
    ttrc_ent(bf->bf_seqno);

    if (!bf->bf_desc->ds_link) {
        __stats(sc, txunaggr_errlast);
        ATH_DS_TOGGLE(bf, 1);
    }

    ath_tx_set_retry(sc, bf);

    TAILQ_INSERT_HEAD(&tid->buf_q, bf, bf_list);
    ath_tx_enqueue(txq, tid->an, tid->tidno);
    return;
}

/*
 * error pkt completion for an aggregate destination
 */
static void
ath_tx_comp_aggr_error(struct ath_softc *sc, struct ath_buf *bf,
                       ath_atx_tid_t *tid)
{
    struct ath_desc        *ds  = bf->bf_lastds;
    struct ath_buf         *bar = NULL;
    struct ath_buf         *bf_next;
    struct ath_rc_series   *rcs = bf->bf_rcs;
    int                    nframes = bf->bf_nframes;
    ath_bufhead            bf_q;
    struct ath_txq         *txq;

    TAILQ_INIT(&bf_q);
    txq = ATH_AC_2_TXQ(sc, tid->ac->priority);

    while (bf) {
        bf_next = bf->bf_next;

#ifdef DEBUG_PKTLOG
        {
            struct log_tx log_data;

            log_data.firstds = bf->bf_desc;
            log_data.bf = bf;
            ath_log_txctl(sc, &log_data, 0);

            if(!bf_next) {
                log_data.lastds = bf->bf_lastds;
                ath_log_txstatus(sc, &log_data, 0);
            }
        }
#endif
        /*
         * retry all subframes
         */
        ath_tx_retry_subframe(sc, bf, &bf_q, &bar);
        bf = bf_next;
    }

    /*
     * update rate module about aggregation
     */
    ath_update_aggr_stats(sc, ds, nframes, nframes);
    ath_rate_tx_complete(sc, tid->an, ds, rcs, nframes, nframes);

    /*
     * send bar if we dropped any frame after excessive retries
     */
    if (bar)
        ath_bar_tx(sc, tid, bar);

    if (!TAILQ_EMPTY(&bf_q)) {
        __stats(sc, txaggr_prepends);
        TAILQ_INSERTQ_HEAD(&tid->buf_q, &bf_q, bf_list);
        ath_tx_enqueue(txq, tid->an, tid->tidno);
    }
}

/*
 * called on a successful completions for an aggregate destination
 */
static void
ath_tx_comp_aggr(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_node         *an = ATH_NODE(bf->bf_node);
    ath_atx_tid_t           *tid = ATH_AN_2_TID(an, bf->bf_tidno);
    ath_atx_ac_t            *ac = tid->ac;
    struct ath_txq          *txq;
    struct ath_desc         *ds  = bf->bf_lastds;
    struct ath_rc_series    *rcs = bf->bf_rcs;
    u_int16_t               seq_st;
    u_int32_t               *ba;
    int                     ba_index;
    int                     nbad = 0;
    int                     nframes = bf->bf_nframes;
    u_int16_t               al = bf->bf_al;
    struct ath_buf          *bf_next;
    ath_bufhead             bf_q;
    int                     tx_ok = 1;
    struct ath_buf          *bar = NULL;

    /*
     * update active pkt count in hw queues
     */
    an->an_tx_hwqcnt --;
    ac->hwqcnt --;

    /*
     * queue filtered packets
     */
    if (ds->ds_txstat.ts_flags == HAL_TX_SW_FILTERED)
        return ath_tx_comp_filtered(sc, bf);

    if (!bf->bf_isaggr) {
        /*
         * packet was not aggregated
         */
        ath_tx_comp_unaggr(sc, bf);
        return;
    }

    ttrc_ent(bf->bf_seqno);
    __stats(sc, tx_compaggr);

    txq = ATH_AC_2_TXQ(sc, ac->priority);
    TAILQ_INIT(&bf_q);

    /*
     * extract starting sequence and block-ack bitmap
     */
    seq_st = ATH_DS_BA_SEQ(ds);
    ba     = ATH_DS_BA_BITMAP(ds);
    tx_ok  = (ATH_DS_TX_STATUS(ds) == HAL_OK);
    ttrc_ent(seq_st);
    ttrc_ent(ATH_DS_TX_BA(ds));
    ttrc_ent(ba[0]);
    ttrc_ent(ba[1]);

    if (tx_ok && !ATH_DS_TX_BA(ds))
        __stats(sc, txaggr_babug);

    AMPDU_TRCADD(sc, nframes, al, bf->bf_seqno, seq_st, ba[0], ba[1]);

    /*
     * handle errors first
     */
    if (ATH_DS_TX_STATUS(ds) & HAL_TXERR_XRETRY) {
        ath_tx_comp_aggr_error(sc, bf, tid);
        return;
    }

    while (bf) {
        ba_index = ATH_BA_INDEX(seq_st, bf->bf_seqno);
        bf_next  = bf->bf_next;

#ifdef DEBUG_PKTLOG
        {
            struct log_tx log_data;

            log_data.firstds = bf->bf_desc;
            log_data.bf = bf;
            ath_log_txctl(sc, &log_data, 0);

            if(!bf_next) {
                log_data.lastds = bf->bf_lastds;
                ath_log_txstatus(sc, &log_data, 0);
            }
        }
#endif

        if (tx_ok && ATH_BA_ISSET(ba, ba_index)) {
            /*
             * complete the acked-ones; update block-ack window
             */
            ttrc_ent(bf->bf_seqno);
            __stats(sc, txaggr_compgood);
            ath_tx_update_baw(tid, bf->bf_seqno);
            ath_tx_freebuf(sc, bf);
        } else {
            /*
             * retry the un-acked ones
             */
            ath_tx_retry_subframe(sc, bf, &bf_q, &bar);
            nbad ++;
        }
        bf = bf_next;
    }

    /*
     * update rate module about aggregation
     */
    ath_update_aggr_stats(sc, ds, nframes, nbad);
    ath_rate_tx_complete(sc, an, ds, rcs, nframes, nbad);

    /*
     * send bar if we dropped any frame after excessive retries
     */
    if (bar)
        ath_bar_tx(sc, tid, bar);

    /*
     * prepend un-acked frames to the beginning of the pending frame queue
     */
    if (!TAILQ_EMPTY(&bf_q)) {
        __stats(sc, txaggr_prepends);
        TAILQ_INSERTQ_HEAD(&tid->buf_q, &bf_q, bf_list);
        ath_tx_enqueue(txq, tid->an, tid->tidno);
    }
}

/*
 * Common code for aggregate excessive retry/subframe retry.
 * If retrying, queues buffers to bf_q. If not, reuses one buffer
 * for BAR, and frees the rest.
 */
static inline void 
ath_tx_retry_subframe(struct ath_softc *sc, struct ath_buf *bf, 
                      ath_bufhead *bf_q, struct ath_buf **bar)
{
    struct ath_node         *an = ATH_NODE(bf->bf_node);
    ath_atx_tid_t           *tid = ATH_AN_2_TID(an, bf->bf_tidno);

    __stats(sc, txaggr_compretries);

    if (bf->bf_retries == OWLMAX_RETRIES) { 
        ttrc_ent(bf->bf_seqno);
        __stats(sc, tx_xretries);

        ath_tx_update_baw(tid, bf->bf_seqno);

        if (!*bar)
            *bar = bf;
        else
            ath_tx_freebuf(sc, bf);
        return;
    }

    ttrc_ent(bf->bf_seqno);

    if (!bf->bf_next) {
        __stats(sc, txaggr_errlast);
        ATH_DS_TOGGLE(bf, 1);
    }

    ath_tx_set_retry(sc, bf);
    TAILQ_INSERT_TAIL(bf_q, bf, bf_list);
}


static void
ath_update_aggr_stats(struct ath_softc *sc, struct ath_desc *ds, int nframes,
                      int nbad)
{
    u_int8_t    status = ATH_DS_TX_STATUS(ds);
    u_int8_t    txflags = ATH_DS_TX_FLAGS(ds);

    __statsn(sc, txaggr_longretries, ds->ds_txstat.ts_longretry);
    __statsn(sc, txaggr_shortretries, ds->ds_txstat.ts_shortretry);

    if (txflags & HAL_TX_DESC_CFG_ERR)
        __stats(sc, txaggr_desc_cfgerr);

    if (txflags & HAL_TX_DATA_UNDERRUN)
        __stats(sc, txaggr_data_urun);

    if (txflags & HAL_TX_DELIM_UNDERRUN)
        __stats(sc, txaggr_delim_urun);

    if (!status) {
        sc->sc_stats.ast_tx_rssi_combined = ds->ds_txstat.ts_rssi;
        sc->sc_stats.ast_tx_rssi_ctl0 = ds->ds_txstat.ts_rssi_ctl0;
        sc->sc_stats.ast_tx_rssi_ctl1 = ds->ds_txstat.ts_rssi_ctl1;
        sc->sc_stats.ast_tx_rssi_ctl2 = ds->ds_txstat.ts_rssi_ctl2;
        sc->sc_stats.ast_tx_rssi_ext0 = ds->ds_txstat.ts_rssi_ext0;
        sc->sc_stats.ast_tx_rssi_ext1 = ds->ds_txstat.ts_rssi_ext1;
        sc->sc_stats.ast_tx_rssi_ext2 = ds->ds_txstat.ts_rssi_ext2;
        return;
    }

    if (status & HAL_TXERR_XRETRY)
        __stats(sc, txaggr_compxretry);

    if (status & HAL_TXERR_FILT)
        __stats(sc, txaggr_filtered);

    if (status & HAL_TXERR_FIFO)
        __stats(sc, txaggr_fifo);

    if (status & HAL_TXERR_XTXOP)
        __stats(sc, txaggr_xtxop);

    if (status & HAL_TXERR_TIMER_EXPIRED)
        __stats(sc, txaggr_timer_exp);
}

static void
ath_tx_addto_baw(ath_atx_tid_t *tid, struct ath_buf *bf)
{
    int     index, cindex;

    if (bf->bf_isretried) {
        __stats(ATH_AN_2_SC(tid->an), tx_bawretries);
        return;
    }

    __stats(ATH_AN_2_SC(tid->an), tx_bawnorm);

    index  = ATH_BA_INDEX(tid->seq_start, bf->bf_seqno);
    cindex = (tid->baw_head + index) & (ATH_TID_MAX_BUFS - 1);

    assert(tid->tx_buf[cindex] == NULL);
    tid->tx_buf[cindex] = bf;

    if (index >= ((tid->baw_tail - tid->baw_head) & (ATH_TID_MAX_BUFS - 1))) {
        __stats(ATH_AN_2_SC(tid->an), tx_bawadv);
        tid->baw_tail = cindex;
        INCR(tid->baw_tail, ATH_TID_MAX_BUFS);
    }
}

static void
ath_tx_update_baw(ath_atx_tid_t *tid, int seqno)
{
    int        index;
    int        cindex;

    __stats(ATH_AN_2_SC(tid->an), tx_bawupdates);

    index  = ATH_BA_INDEX(tid->seq_start, seqno);
    cindex = (tid->baw_head + index) & (ATH_TID_MAX_BUFS - 1);

    tid->tx_buf[cindex] = NULL;

    while (tid->baw_head != tid->baw_tail && !tid->tx_buf[tid->baw_head]) {
        __stats(ATH_AN_2_SC(tid->an), tx_bawupdtadv);
        INCR(tid->seq_start, IEEE80211_SEQ_MAX);
        INCR(tid->baw_head, ATH_TID_MAX_BUFS);
    }
}

static void
ath_tx_set_retry(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ieee80211_frame  *wh;

    __stats(sc, tx_retries);

    bf->bf_isretried = 1;
    bf->bf_retries ++;
    wh = ATH_SKB_2_WH(bf->bf_skb);
    wh->i_fc[1] |= IEEE80211_FC1_RETRY;
}


/*
 *----------------------------------------------------------------------
 * miscellaneous
 *----------------------------------------------------------------------
 */
static __inline void
ath_desc_swap(struct ath_desc *ds)
{
#ifdef AH_NEED_DESC_SWAP
    ds->ds_link = cpu_to_le32(ds->ds_link);
    ds->ds_data = cpu_to_le32(ds->ds_data);
    ds->ds_ctl0 = cpu_to_le32(ds->ds_ctl0);
    ds->ds_ctl1 = cpu_to_le32(ds->ds_ctl1);
    ds->ds_hw[0] = cpu_to_le32(ds->ds_hw[0]);
    ds->ds_hw[1] = cpu_to_le32(ds->ds_hw[1]);
#endif
}

/*
 * Get transmit rate index using rate in Kbps
 */
static int
ath_tx_findindex(const HAL_RATE_TABLE *rt, int dot11code)
{
#define RV(v)   ((v) & IEEE80211_RATE_VAL)
    int i;
    int ndx = 0;

    for (i = 0; i < rt->rateCount; i++) {
        if (dot11code & IEEE80211_RATE_BASIC) {
            if ((RV(rt->info[i].dot11Rate) == RV(dot11code)) &&
                (rt->info[i].phy == IEEE80211_T_HT)) {
                ndx = i;
                break;
            }
        } else {
            if ((RV(rt->info[i].dot11Rate) == dot11code) &&
                (rt->info[i].phy != IEEE80211_T_HT)) {
                ndx = i;
                break;
            }
        }
    }

    return ndx;
#undef RV
}

static ATH_AGGR_STATUS
ath_tx_form_aggr(struct ath_softc *sc, ath_atx_tid_t *tid, ath_bufhead *bf_q)
{
    struct ath_buf *bf, *bf_first, *bf_prev = NULL;
    int rl = 0, nframes = 0;
    u_int16_t aggr_limit = 0, al = 0, bpad = 0, al_delta, h_baw = tid->baw_size/2;

    bf_first = TAILQ_FIRST(&tid->buf_q);

    do {
        bf = TAILQ_FIRST(&tid->buf_q);

        ttrc_ent(bf->bf_seqno);

        /*
         * do not step over block-ack window
         */
        if (!BAW_WITHIN(tid->seq_start, tid->baw_size, bf->bf_seqno)) {
            ttrc_ent(tid->seq_start);
            bf_first->bf_al= al;
            bf_first->bf_nframes = nframes;
            return ATH_AGGR_BAW_CLOSED;
        }

        if (!rl) {
            aggr_limit = ath_lookup_rate(sc, tid->an, bf);
            rl = 1;
        }

        /*
         * do not exceed aggregation limit
         */
        al_delta = ATH_AGGR_DELIM_SZ + bf->bf_pktlen;
        if (nframes && (aggr_limit < (al + bpad + al_delta))) {
            bf_first->bf_al= al;
            bf_first->bf_nframes = nframes;
            return ATH_AGGR_LIMITED;
        }

        /*
         * do not exceed subframe limit
         */
        if (nframes >= MIN(h_baw, sc->sc_ic.ic_ampdu_subframes)) {
            bf_first->bf_al= al;
            bf_first->bf_nframes = nframes;
            return ATH_AGGR_LIMITED;
        }

        /*
         * this packet is part of aggregate
         */
        ath_tx_addto_baw(tid, bf);
        TAILQ_REMOVE(&tid->buf_q, bf, bf_list);
        TAILQ_INSERT_TAIL(bf_q, bf, bf_list);
        nframes ++;

        /*
         * add padding for previous frame to aggregation length
         */
        al += bpad + al_delta;
        bf->bf_ndelim = ATH_AGGR_GET_NDELIM(bf->bf_pktlen);
        /*
         * If encryption enabled, hardware requires some more padding between
         * subframes.
         * TODO - this could be improved to be dependent on the rate.
         *      The hardware can keep up at lower rates, but not higher rates
         *      See bug 20205.
         */
        if (bf->bf_keytype != HAL_KEY_TYPE_CLEAR)
            bf->bf_ndelim += ATH_AGGR_ENCRYPTDELIM;

        bpad = PADBYTES(al_delta) + (bf->bf_ndelim << 2);

        /*
         * link current buffer to the aggregate
         */
        if (bf_prev) {
            bf_prev->bf_next = bf;
            bf_prev->bf_desc->ds_link = bf->bf_daddr;
        }
        bf_prev = bf;
        ath_hal_set11n_aggr_middle(sc->sc_ah, bf->bf_desc, bf->bf_ndelim);

#if AGGR_NOSHORT
        /*
         * terminate aggregation on a small packet boundary
         */
        if (bf->bf_pktlen < ATH_AGGR_MINPLEN) {
            bf_first->bf_al= al;
            bf_first->bf_nframes = nframes;
            return ATH_AGGR_SHORTPKT;
        }
#endif
    } while (!TAILQ_EMPTY(&tid->buf_q));

    bf_first->bf_al= al;
    bf_first->bf_nframes = nframes;
    return ATH_AGGR_DONE;
}


/*
 *----------------------------------------------------------------------
 * Debug only:
 *----------------------------------------------------------------------
 */

#ifdef OWLDEBUG

/*
 * trigger the remote node
 */
void
owl_trigger(struct ath_node *an, struct sk_buff *skbtrig)
{
    ath_bufhead                 bf_q;
    struct ath_buf              *bf;
    struct ieee80211_frame_bar  *bar, *bartrig;
    struct ath_desc             *ds;
    u_int8_t                    addrb;
    int                         i;
    struct sk_buff              *skb;

    atrc_ent(0x66666666);
    skb = dev_alloc_skb(1024);
    if (!skb)
        return;

    ath_hal_reg_read(owltrig_sc->sc_ah, 0x910);
    assert(0)

    bf = ath_buf_alloc(owltrig_sc, &bf_q, 1);
    if (!bf)
        return;

    bf->bf_skb = skb;
    bf->bf_node = (struct ieee80211_node *) an;
    bf->bf_txq = ATH_AC_2_TXQ(owltrig_sc, 0);

    /*
     * form the bar frame
     */
    bartrig = (struct ieee80211_frame_bar *) skbtrig->data;
    bar = (struct ieee80211_frame_bar *) skb->data;
    *bar = *bartrig;

    bar->i_fc[1]  = IEEE80211_FC1_DIR_NODS;
    bar->i_fc[0]  = IEEE80211_FC0_VERSION_0 | IEEE80211_FC0_TYPE_CTL |
        IEEE80211_FC0_SUBTYPE_BAR;
    bar->i_ctl    = (IEEE80211_BAR_CTL_TID_S | IEEE80211_BAR_CTL_COMBA);
    bar->i_seq    = htole16(0x666 << IEEE80211_SEQ_SEQ_SHIFT);

    /*
     * swap addresses
     */
    for (i = 0; i < 6; i++) {
        addrb = bar->i_ta[i];
        bar->i_ta[i] = bar->i_ra[i];
        bar->i_ra[i] = addrb;
    }

    /*
     * set len to bar frame
     */
    skb->len = sizeof(struct ieee80211_frame_bar);

    /*
     * setup buf
     */
    bf->bf_comp = NULL;
    ath_dma_map(owltrig_sc, bf);

    /*
     * setup desc
     */
    ds = bf->bf_desc;
#ifdef DEBUG_PKTLOG
    ds->ds_vdata = (u_int32_t)skb->data;
#endif
    ath_hal_setuptxdesc(owltrig_sc->sc_ah, ds
                        , skb->len + IEEE80211_CRC_LEN      /* pktlen   */
                        , 0                                 /* hdrlen   */
                        , HAL_PKT_TYPE_NORMAL               /* atype    */
                        , 60                                /* tx power */
                        , 0x0b                              /* rate0    */
                        , 1                                 /* tries    */
                        , bf->bf_keyix                      /* keyindex */
                        , 0                                 /* antenna  */
                        , HAL_TXDESC_INTREQ                 /* flags    */
                        , 0, 0, 0, 0
                        , ATH_COMP_PROC_NO_COMP_NO_CCS
                        );

    ds->ds_data = bf->bf_skbaddr;
    ds->ds_link = 0;
    ath_hal_filltxdesc(owltrig_sc->sc_ah, ds, skb->len, AH_TRUE, AH_TRUE, ds);
    bf->bf_lastds = ds;

    ath_txq_add_ucast(owltrig_sc, bf);
}

#endif

/*
 *----------------------------------------------------------------------
 * Others:
 *----------------------------------------------------------------------
 *
 * TODO:
 *
 * Rate module:
 * o  ath_get_txtime(_rkbps, _phy)
 *    return
 * o  aggregate tx completion update - new entry point
 *
 * Init/setup:
 * o  bf->bf_daddr[] during alloc
 * o  ac & tid in ath_node structs
 *
 * Aggregation:
 * o tx completion - both normal & aggregate
 * o baw negotiation
 *
 * Fragmentation:
 * o ath_transmit() - possible to queue part of fragment train
 *
 * Other notes:
 * o  If aggregating, protected bursts do not make sense. This is due to
 *    the fact that NAV setting in an aggregate is not understood by legacy
 *    stations.
 *
 * Issues:
 * o  BAR - verify wrap around sequences
 * o  BA  - verify windowing logic for TCP traffic
 */

