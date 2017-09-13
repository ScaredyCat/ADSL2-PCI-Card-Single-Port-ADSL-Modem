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
 * $Id: //depot/sw/branches/owl_sw_dev/src/linux/madwifi/madwifi/ath/if_ath.c#117 $
 */

/*
 * Driver for the Atheros Wireless LAN controller.
 *
 * This software is derived from work of Atsushi Onoe; his contribution
 * is greatly appreciated.
 */
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
//hsumc#include <linux/jiffies.h>

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

#ifdef __LINUX_MIPS32_ARCH__
//hsumc #include <ar7100.h>
#endif


extern  void bus_read_cachesize(struct ath_softc *sc, u_int8_t *csz);
extern  void sort(void *base, size_t num, size_t size, int (*cmp)(const void *, const void *), 
                void (*swap)(void *, void *, int size));

/* unalligned little endian access */
#define LE_READ_2(p)                            \
    ((u_int16_t)                            \
     ((((u_int8_t *)(p))[0]      ) | (((u_int8_t *)(p))[1] <<  8)))
#define LE_READ_4(p)                            \
    ((u_int32_t)                            \
     ((((u_int8_t *)(p))[0]      ) | (((u_int8_t *)(p))[1] <<  8) | \
      (((u_int8_t *)(p))[2] << 16) | (((u_int8_t *)(p))[3] << 24)))

static struct ieee80211vap *ath_vap_create(struct ieee80211com *,
            const char *name, int unit, int opmode, int flags);
static void ath_vap_delete(struct ieee80211vap *);
static int  ath_init(struct net_device *);
static int  ath_reset(struct net_device *);
static int  ath_resetinternal(struct net_device *, int);
#define RESET_NONFATAL	0
#define RESET_FATAL	1
static void ath_fatal_tasklet(TQUEUE_ARG);
static void ath_rxorn_tasklet(TQUEUE_ARG);
static void ath_bmiss_tasklet(TQUEUE_ARG);
static void ath_bstuck_tasklet(TQUEUE_ARG);
static void ath_txto_tasklet(TQUEUE_ARG);
static int  ath_stop_locked(struct net_device *);
static int  ath_stop(struct net_device *);
#if 0
static void ath_initkeytable(struct ath_softc *);
#endif
static int  ath_key_alloc(struct ieee80211vap *,
            const struct ieee80211_key *);
static int  ath_key_delete(struct ieee80211vap *,
            const struct ieee80211_key *,
            struct ieee80211_node *);
static int  ath_key_set(struct ieee80211vap *, const struct ieee80211_key *,
            const u_int8_t mac[IEEE80211_ADDR_LEN]);
static void ath_key_update_begin(struct ieee80211vap *);
static void ath_key_update_end(struct ieee80211vap *);
static void ath_mode_init(struct net_device *);
static void ath_setslottime(struct ath_softc *);
static void ath_updateslot(struct net_device *);
static int  ath_beaconq_setup(struct ath_hal *);
static int  ath_beacon_alloc(struct ath_softc *, struct ieee80211_node *);
#ifdef ATH_SUPERG_DYNTURBO
static void     ath_beacon_dturbo_update(struct ieee80211vap *vap, int *);
static void     ath_beacon_dturbo_config(struct ieee80211vap *, u_int32_t);
static void     ath_turbo_switch_mode(unsigned long);
static int      ath_check_beacon_done(struct ath_softc *sc);
#endif
static void ath_beacon_tasklet(struct ath_softc *, int *needmark);
static void ath_beacon_start_adhoc(struct ath_softc *,
            struct ieee80211vap *);
static void ath_beacon_return(struct ath_softc *, struct ath_buf *);
static void ath_beacon_free(struct ath_softc *);
static void ath_beacon_config(struct ath_softc *, struct ieee80211vap *);
static int  ath_desc_alloc(struct ath_softc *);
static void ath_desc_free(struct ath_softc *);
static void ath_desc_swap(struct ath_desc *);
static struct ieee80211_node *ath_node_alloc(struct ieee80211_node_table *,struct ieee80211vap *vap);
static void ath_node_free(struct ieee80211_node *);
static u_int8_t ath_node_getrssi(const struct ieee80211_node *);
static int  ath_rxbuf_init(struct ath_softc *, struct ath_buf *);
static void ath_recv_mgmt(struct ieee80211_node *, struct sk_buff *,
            int subtype, int rssi, u_int32_t rstamp);
#ifdef ATH_FORCE_PPM
static void ath_force_ppm_notify(struct ieee80211_node *, int);
#endif
static void ath_setdefantenna(struct ath_softc *sc, u_int antenna);
static struct ath_txq *ath_txq_setup(struct ath_softc*, int qtype, int subtype);
static void ath_rx_tasklet(TQUEUE_ARG data);
static int  ath_hardstart(struct sk_buff *, struct net_device *);
static int  ath_mgtstart(struct ieee80211com *ic, struct sk_buff *skb);
#ifdef ATH_SUPERG_COMP
static u_int32_t ath_get_icvlen(struct ieee80211_key *k);
static u_int32_t ath_get_ivlen(struct ieee80211_key *k);
static void     ath_setup_comp(struct ieee80211_node *, int);
static void ath_comp_set(struct ieee80211vap *, struct ieee80211_node *ni,
                int en);
#endif
static int  ath_tx_setup(struct ath_softc *, int ac, int haltype);
static int  ath_wme_update(struct ieee80211com *);
static void ath_tx_cleanupq(struct ath_softc *, struct ath_txq *);
static void ath_tx_cleanup(struct ath_softc *);

static int  ath_tx_start(struct net_device *, struct ieee80211_node *,
                 struct ath_buf *, struct sk_buff *, int);
static void ath_tx_tasklet_q0(TQUEUE_ARG data);
static void ath_tx_tasklet_q0123(TQUEUE_ARG data);
static void ath_tx_tasklet(TQUEUE_ARG data);
static void ath_tx_timeout(struct net_device *);
static void ath_tx_draintxq(struct ath_softc *, struct ath_txq *);
static void ath_draintxq(struct ath_softc *);
static void ath_flushrecv(struct ath_softc *);
static void ath_chan_change(struct ath_softc *, struct ieee80211_channel *);
static void ath_calibrate(unsigned long);
static int  ath_newstate(struct ieee80211vap *, enum ieee80211_state, int);

static void ath_scan_start(struct ieee80211com *);
static void ath_scan_end(struct ieee80211com *);
static void ath_set_channel(struct ieee80211com *);
static void ath_set_coverageclass(struct ieee80211com *);
static u_int    ath_mhz2ieee(struct ieee80211com *, u_int freq, u_int flags);
#ifdef ATH_SUPERG_FF
static int  athff_can_aggregate(struct ath_softc *sc, struct ether_header *eh,
                    struct ath_node *an, struct sk_buff *skb, u_int16_t fragthreshold, int *flushq);
#endif
static struct net_device_stats *ath_getstats(struct net_device *);
static void ath_setup_stationkey(struct ieee80211_node *);
static void     ath_setup_stationwepkey(struct ieee80211_node *);
static void     ath_setup_keycacheslot(struct ath_softc *,
            struct ieee80211_node *);
static void ath_newassoc(struct ieee80211_node *, int);
static int  ath_getchannels(struct net_device *, u_int cc,
            HAL_BOOL outdoor, HAL_BOOL xchanmode);
static void ath_update_txpow(struct ath_softc *);

static int  ath_set_mac_address(struct net_device *, void *);
static int  ath_change_mtu(struct net_device *, int);
static int  ath_ioctl(struct net_device *, struct ifreq *, int);

static int  ath_rate_setup(struct net_device *, u_int mode);
static void ath_setup_subrates(struct net_device *dev);
#ifdef ATH_SUPERG_XR
static int  ath_xr_rate_setup(struct net_device *);
static void ath_grppoll_txq_setup(struct ath_softc *sc, int qtype, int period);
static void ath_grppoll_start(struct ieee80211vap *vap,int pollcount);
static void ath_grppoll_stop(struct ieee80211vap *vap);
static u_int8_t ath_node_move_data(const struct ieee80211_node *);
static void ath_grppoll_txq_update(struct ath_softc *sc, int period);
static void ath_grppoll_period_update(struct ath_softc *sc);
#endif
static void ath_setcurmode(struct ath_softc *, enum ieee80211_phymode);

#ifdef CONFIG_SYSCTL
static void ath_dynamic_sysctl_register(struct ath_softc *);
static void ath_dynamic_sysctl_unregister(struct ath_softc *);
#endif /* CONFIG_SYSCTL */
static void ath_announce(struct net_device *);
#ifdef ATH_TX99_DIAG
static int ath_tx99ctl(int mode,int val,struct ath_softc *sc);
static int ath_rx99ctl(int mode,int val,struct ath_softc *sc);
#endif
static int ath_descdma_setup(struct ath_softc *sc,
                 struct ath_descdma *dd, ath_bufhead *head,
                 const char *name, int nbuf, int ndesc);
static void ath_descdma_cleanup(struct ath_softc *sc,
                struct ath_descdma *dd, ath_bufhead *head, int dir);

static HAL_BOOL ath_fastcc_check(struct ath_softc *sc,
                             struct ieee80211_channel *chan);
static HAL_BOOL ath_aborttxq(struct ath_softc *sc);
static HAL_BOOL ath_abortrecv(struct ath_softc *sc);
static int chanCompare(const void *, const void *);

#ifdef ATH_CHAINMASK_SELECT
static void
ath_chainmask_sel_timertimeout(struct ath_softc *sc, void *timerArg);
static void
ath_chainmask_sel_timerstart(struct ath_softc *sc, ath_chainmask_sel_t *cm);
void
ath_chainmask_sel_timerstop(struct ath_softc *sc, ath_chainmask_sel_t *cm);
int
ath_chainmask_sel_logic(struct ath_softc *sc);
void
ath_chainmask_sel_init(struct ath_softc *sc);
#endif

#ifdef ATH_FORCE_PPM
static void
ath_force_ppm_timertimeout(struct ath_softc *sc, void *timerArg);
static void
ath_force_ppm_timerstart(struct ath_softc *sc, ath_force_ppm_t *afp);
void
ath_force_ppm_timerstop(struct ath_softc *sc, ath_force_ppm_t *afp);
int
ath_force_ppm_logic(struct ath_softc *sc, struct ath_buf *, HAL_STATUS, struct ath_rx_status *, int32_t *);
void
ath_force_ppm_init(struct ath_softc *sc, int cold);
#endif

static void
ath_get_hwstate(struct ath_softc *sc, HAL_HT *ht);

static  int ath_noreset = 0;       /* do not reset chip on bcn stuck / wd */
static  int ath_calinterval = 30;       /* calibrate every 30 secs */
static  int ath_countrycode = CTRY_DEFAULT; /* country code */
static  int ath_regdomain = 0;          /* regulatory domain */
static  int ath_outdoor = AH_FALSE;     /* enable outdoor use */
static  int ath_xchanmode = AH_TRUE;        /* enable extended channels */
#ifdef CES_DEMO
u_int32_t   ath_htrate_0123 = 0x80808080;
u_int32_t   ath_legacyrate_0123 = 0x0b0b0b0b;
u_int32_t   ath_rate_tries_0123 = 0x04040202;
#endif
u_int32_t   ath_aggr_prot = 0;
u_int32_t   ath_aggr_prot_duration = 8192;
u_int32_t   ath_aggr_prot_max = 8192;

#define MSECS(_n)         ((_n) * HZ / 1000)

#ifdef ATH_CHAINMASK_SELECT
u_int32_t   ath_chainmask_sel_enable=1;
u_int32_t   ath_chainmask_sel_up_rssi_thres=ATH_CHAINMASK_SEL_UP_RSSI_THRES;
u_int32_t   ath_chainmask_sel_down_rssi_thres=ATH_CHAINMASK_SEL_DOWN_RSSI_THRES;
u_int32_t   ath_chainmask_sel_period=ATH_CHAINMASK_SEL_TIMEOUT;
#endif

#ifdef ATH_FORCE_PPM
u_int32_t   ath_force_ppm_enable = 0;   /* currently off by default */
u_int32_t   ath_force_ppm_period = ATH_FORCE_PPM_TIMEOUT;
u_int32_t   ath_force_ppm_wd     = ATH_FORCE_PPM_WD;
u_int32_t   ath_force_ppm_to     = ATH_FORCE_PPM_TO;
 
/* Bit values for ath_force_ppm_enable */
#define ATH_DEBUG_PPM_ENABLE    0x0001  /* any bit set will enable function */
#define ATH_DEBUG_PPM_PRINT     0x0002  /* print update mesages to console */
#define ATH_DEBUG_PPM_LOG       0x0004  /* log activity in packet log */
#define ATH_DEBUG_PPM_NOUPDATE  0x0008  /* do not actually force the new value */
#endif

static  int countrycode = -1;
MODULE_PARM(countrycode, "i");
MODULE_PARM_DESC(countrycode, "Override default country code");
static  int outdoor = -1;
MODULE_PARM(outdoor, "i");
MODULE_PARM_DESC(outdoor, "Enable/disable outdoor use");
static  int xchanmode = -1;
MODULE_PARM(xchanmode, "i");
MODULE_PARM_DESC(xchanmode, "Enable/disable extended channel mode");
//hsumc static int use_eeprom = -1;
static int use_eeprom = 1;
MODULE_PARM(use_eeprom, "i");
MODULE_PARM_DESC(use_eeprom, "Calibration data in EEPROM, not flash");
static int force_11a_channels = 0;
MODULE_PARM(force_11a_channels, "i");
//hsumc 
MODULE_PARM_DESC(force_11a_channels, "Force use of 11a channels only");

#ifdef AR_DEBUG

int ath_debug = 0x00000000;
MODULE_PARM(ath_debug, "i");
MODULE_PARM_DESC(ath_debug, "Load-time debug output enable");

#define IFF_DUMPPKTS(sc, _m) \
    ((sc->sc_debug & _m))
static  void ath_printrxbuf(struct ath_buf *bf, int);
static  void ath_printtxbuf(struct ath_buf *bf, int);
enum {
    ATH_DEBUG_XMIT      = 0x00000001,   /* basic xmit operation */
    ATH_DEBUG_XMIT_DESC = 0x00000002,   /* xmit descriptors */
    ATH_DEBUG_RECV      = 0x00000004,   /* basic recv operation */
    ATH_DEBUG_RECV_DESC = 0x00000008,   /* recv descriptors */
    ATH_DEBUG_RATE      = 0x00000010,   /* rate control */
    ATH_DEBUG_RESET     = 0x00000020,   /* reset processing */
    /* 0x00000040 was ATH_DEBUG_MODE */
    ATH_DEBUG_BEACON    = 0x00000080,   /* beacon handling */
    ATH_DEBUG_WATCHDOG  = 0x00000100,   /* timeout */
    ATH_DEBUG_INTR      = 0x00001000,   /* ISR */
    ATH_DEBUG_TX_PROC   = 0x00002000,   /* tx ISR proc */
    ATH_DEBUG_RX_PROC   = 0x00004000,   /* rx ISR proc */
    ATH_DEBUG_BEACON_PROC   = 0x00008000,   /* beacon ISR proc */
    ATH_DEBUG_CALIBRATE = 0x00010000,   /* periodic calibration */
    ATH_DEBUG_KEYCACHE  = 0x00020000,   /* key cache management */
    ATH_DEBUG_STATE     = 0x00040000,   /* 802.11 state transitions */
    ATH_DEBUG_NODE      = 0x00080000,   /* node management */
    ATH_DEBUG_LED       = 0x00100000,   /* led management */
    ATH_DEBUG_FF        = 0x00200000,   /* fast frames */
    ATH_DEBUG_TURBO     = 0x00400000,   /* turbo/dynamice turbo */
    ATH_DEBUG_DOTH      = 0x01000000,   /* 11.h */
    ATH_DEBUG_CWM       = 0x02000000,   /* Channel Width Management */
    ATH_DEBUG_11N       = 0x04000000,   /* 11.n */
    ATH_DEBUG_FATAL     = 0x80000000,   /* fatal errors */
    ATH_DEBUG_ANY       = 0xffffffff
};
#define DPRINTF(sc, _m, _fmt, ...) do {             \
    if (sc->sc_debug & (_m))                \
        printk(_fmt, __VA_ARGS__);          \
} while (0)
#define KEYPRINTF(sc, ix, hk, mac) do {             \
    if (sc->sc_debug & ATH_DEBUG_KEYCACHE)          \
        ath_keyprint(__func__, ix, hk, mac);        \
} while (0)
#else
#define IFF_DUMPPKTS(sc, _m)    netif_msg_dumppkts(&sc->sc_ic)
#define DPRINTF(sc, _m, _fmt, ...)
#define KEYPRINTF(sc, k, ix, mac)
#endif

/*
 * Define the scheme that we select MAC address for multiple BSS on the same radio.
 * The very first VAP will just use the MAC address from the EEPROM.
 * For the next 3 VAPs, we set the U/L bit (bit 1) in MAC address,
 * and use the next two bits as the index of the VAP.
 */
#define ATH_SET_VAP_BSSID_MASK(bssid_mask)      ((bssid_mask)[0] &= ~(((ATH_BCBUF-1)<<2)|0x02))
#define ATH_GET_VAP_ID(bssid)                   ((bssid)[0] >> 2)
#define ATH_SET_VAP_BSSID(bssid, id)            \
    do {                                    \
        if (id) {                           \
            (bssid)[0] |= (((id)<<2)|0x02); \
        }                                   \
    } while(0)

#ifdef DEBUG_PKTLOG
    struct ath_pktlog_funcs *g_pktlog_funcs = NULL;

EXPORT_SYMBOL(g_pktlog_funcs);
#endif

int
ath_attach(u_int16_t devid, struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah;
    HAL_STATUS status;
    int error = 0, i, flags = 0;
    u_int8_t csz;
    u_int32_t chainmask;

    sc->sc_debug = ath_debug;
    DPRINTF(sc, ATH_DEBUG_ANY, "%s: devid 0x%x\n", __func__, devid);

    /*
     * Cache line size is used to size and align various
     * structures used to communicate with the hardware.
     */
    bus_read_cachesize(sc, &csz);
    /* XXX assert csz is non-zero */
    sc->sc_cachelsz = csz << 2;     /* convert to bytes */

    AMPDU_TRCINIT(sc);

    ATH_LOCK_INIT(sc);
    ATH_TXBUF_LOCK_INIT(sc);

    ATH_INIT_TQUEUE(&sc->sc_rxtq,    ath_rx_tasklet,    dev);
    ATH_INIT_TQUEUE(&sc->sc_txtq,    ath_tx_tasklet,    dev);
#define OWLAGGR
#ifdef OWLAGGR
    ATH_INIT_TQUEUE(&sc->sc_txtq,    owl_tx_tasklet,    dev);
#endif
    ATH_INIT_TQUEUE(&sc->sc_bmisstq, ath_bmiss_tasklet, dev);
    ATH_INIT_TQUEUE(&sc->sc_bstucktq,ath_bstuck_tasklet,    dev);
    ATH_INIT_TQUEUE(&sc->sc_rxorntq, ath_rxorn_tasklet, dev);
#ifdef OWLAGGR
    ATH_INIT_TQUEUE(&sc->sc_rxorntq, ath_rx_tasklet, dev);
#endif
    ATH_INIT_TQUEUE(&sc->sc_fataltq, ath_fatal_tasklet, dev);
    ATH_INIT_TQUEUE(&sc->sc_txtotq,  ath_txto_tasklet,  dev);

    /*
     * Attach the hal and verify ABI compatibility by checking
     * the hal's ABI signature against the one the driver was
     * compiled with.  A mismatch indicates the driver was
     * built with an ah.h that does not correspond to the hal
     * module loaded in the kernel.
     */
    if (use_eeprom == 1)
        flags |= AH_USE_EEPROM;
    ah = _ath_hal_attach(devid, sc, 0, (void *) dev->mem_start, flags, &status);
    if (ah == NULL) {
        printk(KERN_ERR "%s: unable to attach hardware; HAL status %u\n",
            dev->name, status);
        error = ENXIO;
        goto bad;
    }
    if (ah->ah_abi != HAL_ABI_VERSION) {
        printk(KERN_ERR "%s: HAL ABI msmatch; "
            "driver expects 0x%x, HAL reports 0x%x\n",
            dev->name, HAL_ABI_VERSION, ah->ah_abi);
        error = ENXIO;      /* XXX */
        goto bad;
    }
    sc->sc_ah = ah;

    /*
     * Check if the device has hardware counters for PHY
     * errors.  If so we need to enable the MIB interrupt
     * so we can act on stat triggers.
     */
    if (ath_hal_hwphycounters(ah))
        sc->sc_needmib = 1;

    /*
     * Get the hardware key cache size.
     */
    sc->sc_keymax = ath_hal_keycachesize(ah);
    if (sc->sc_keymax > ATH_KEYMAX) {
        printk("%s: Warning, using only %u entries in %u key cache\n",
            dev->name, ATH_KEYMAX, sc->sc_keymax);
        sc->sc_keymax = ATH_KEYMAX;
    }
    /*
     * Reset the key cache since some parts do not
     * reset the contents on initial power up.
     */
    for (i = 0; i < sc->sc_keymax; i++)
        ath_hal_keyreset(ah, i);
    /*
     * Mark key cache slots associated with global keys
     * as in use.  If we knew TKIP was not to be used we
     * could leave the +32, +64, and +32+64 slots free.
     * XXX only for splitmic.
     */
    for (i = 0; i < IEEE80211_WEP_NKID; i++) {
        setbit(sc->sc_keymap, i);
        setbit(sc->sc_keymap, i+32);
        setbit(sc->sc_keymap, i+64);
        setbit(sc->sc_keymap, i+32+64);
    }

    /*
     * Collect the channel list using the default country
     * code and including outdoor channels.  The 802.11 layer
     * is resposible for filtering this list based on settings
     * like the phy mode.
     */
    if (countrycode != -1)
        ath_countrycode = countrycode;
    if (outdoor != -1)
        ath_outdoor = outdoor;
    if (xchanmode != -1)
        ath_xchanmode = xchanmode;
    error = ath_getchannels(dev, ath_countrycode,
            ath_outdoor, ath_xchanmode);
    if (error != 0)
        goto bad;

    ic->ic_country_code = ath_countrycode;
    ic->ic_country_outdoor = ath_outdoor;

    /*
     * Setup rate tables for all potential media types.
     */
    ath_rate_setup(dev, IEEE80211_MODE_11A);
    ath_rate_setup(dev, IEEE80211_MODE_11B);
    ath_rate_setup(dev, IEEE80211_MODE_11G);
    ath_rate_setup(dev, IEEE80211_MODE_TURBO_A);
    ath_rate_setup(dev, IEEE80211_MODE_TURBO_G);
    ath_rate_setup(dev, IEEE80211_MODE_11NA);
    ath_rate_setup(dev, IEEE80211_MODE_11NG);

    /* Setup for half/quarter rates */
    ath_setup_subrates(dev);

    /* NB: setup here so ath_rate_update is happy */
    ath_setcurmode(sc, IEEE80211_MODE_11A);

    /*
     * Allocate tx+rx descriptors and populate the lists.
     */
    error = ath_desc_alloc(sc);
    if (error != 0) {
        printk(KERN_ERR "%s: failed to allocate descriptors: %d\n",
            dev->name, error);
        goto bad;
    }

    /*
     * Allocate hardware transmit queues: one queue for
     * beacon frames and one data queue for each QoS
     * priority.  Note that the hal handles reseting
     * these queues at the needed time.
     *
     * XXX PS-Poll
     */
    sc->sc_bhalq = ath_beaconq_setup(ah);
    if (sc->sc_bhalq == (u_int) -1) {
        printk(KERN_ERR "%s: unable to setup a beacon xmit queue!\n",
            dev->name);
        error = EIO;
        goto bad2;
    }
    sc->sc_cabq = ath_txq_setup(sc, HAL_TX_QUEUE_CAB, 0);
    if (sc->sc_cabq == NULL) {
        printk(KERN_ERR "%s: unable to setup CAB xmit queue!\n",
            dev->name);
        error = EIO;
        goto bad2;
    }
    /* NB: insure BK queue is the lowest priority h/w queue */
    if (!ath_tx_setup(sc, WME_AC_BK, HAL_WME_AC_BK)) {
        printk(KERN_ERR "%s: unable to setup xmit queue for %s traffic!\n",
            dev->name, ieee80211_wme_acnames[WME_AC_BK]);
        error = EIO;
        goto bad2;
    }
    if (!ath_tx_setup(sc, WME_AC_BE, HAL_WME_AC_BE) ||
        !ath_tx_setup(sc, WME_AC_VI, HAL_WME_AC_VI) ||
        !ath_tx_setup(sc, WME_AC_VO, HAL_WME_AC_VO)) {
        /*
         * Not enough hardware tx queues to properly do WME;
         * just punt and assign them all to the same h/w queue.
         * We could do a better job of this if, for example,
         * we allocate queues when we switch from station to
         * AP mode.
         */
        if (sc->sc_ac2q[WME_AC_VI] != NULL)
            ath_tx_cleanupq(sc, sc->sc_ac2q[WME_AC_VI]);
        if (sc->sc_ac2q[WME_AC_BE] != NULL)
            ath_tx_cleanupq(sc, sc->sc_ac2q[WME_AC_BE]);
        sc->sc_ac2q[WME_AC_BE] = sc->sc_ac2q[WME_AC_BK];
        sc->sc_ac2q[WME_AC_VI] = sc->sc_ac2q[WME_AC_BK];
        sc->sc_ac2q[WME_AC_VO] = sc->sc_ac2q[WME_AC_BK];
    }
#ifdef ATH_SUPERG_XR
    ath_xr_rate_setup(dev);
    sc->sc_xrpollint =  XR_DEFAULT_POLL_INTERVAL;
    sc->sc_xrpollcount = XR_DEFAULT_POLL_COUNT;
    strcpy(sc->sc_grppoll_str,XR_DEFAULT_GRPPOLL_RATE_STR);
    sc->sc_grpplq.axq_qnum=-1;
    sc->sc_xrtxq = ath_txq_setup(sc, HAL_TX_QUEUE_DATA,HAL_XR_DATA );
#endif

    /*
     * Special case certain configurations.  Note the
     * CAB queue is handled by these specially so don't
     * include them when checking the txq setup mask.
     */
    switch (sc->sc_txqsetup &~ (1<<sc->sc_cabq->axq_qnum)) {
    case 0x01:
        ATH_INIT_TQUEUE(&sc->sc_txtq, ath_tx_tasklet_q0, dev);
        break;
    case 0x0f:
        ATH_INIT_TQUEUE(&sc->sc_txtq, ath_tx_tasklet_q0123, dev);
        break;
    }

    sc->sc_setdefantenna = ath_setdefantenna;
    sc->sc_rc = ath_rate_attach(sc);
    if (sc->sc_rc == NULL) {
        error = EIO;
        goto bad2;
    }

    init_timer(&sc->sc_cal_ch);
    sc->sc_cal_ch.function = ath_calibrate;
    sc->sc_cal_ch.data = (unsigned long) dev;

#ifdef ATH_SUPERG_DYNTURBO
    init_timer(&sc->sc_dturbo_switch_mode);
    sc->sc_dturbo_switch_mode.function = ath_turbo_switch_mode;
    sc->sc_dturbo_switch_mode.data = (unsigned long) dev;
#endif

    sc->sc_blinking = 0;
    sc->sc_ledstate = 1;
    sc->sc_ledon = 0;           /* low true */
    sc->sc_ledidle = (2700*HZ)/1000;    /* 2.7sec */
    init_timer(&sc->sc_ledtimer);
    sc->sc_ledtimer.data = (unsigned long) sc;
    /*
     * Auto-enable soft led processing for IBM cards and for
     * 5211 minipci cards.  Users can also manually enable/disable
     * support with a sysctl.
     */
    sc->sc_softled = (devid == AR5212_DEVID_IBM || devid == AR5211_DEVID);
    if (sc->sc_softled) {
        ath_hal_gpioCfgOutput(ah, sc->sc_ledpin);
        ath_hal_gpioset(ah, sc->sc_ledpin, !sc->sc_ledon);
    }

    /* NB: ether_setup is done by bus-specific code */
    dev->open = ath_init;
    dev->stop = ath_stop;
    dev->hard_start_xmit = ath_hardstart;
#ifdef OWLAGGR
    dev->hard_start_xmit = owl_hardstart;
#endif
    dev->tx_timeout = ath_tx_timeout;
    dev->watchdog_timeo = 5 * HZ;           /* XXX */
    dev->set_multicast_list = ath_mode_init;
    dev->do_ioctl = ath_ioctl;
    dev->get_stats = ath_getstats;
    dev->set_mac_address = ath_set_mac_address;
    dev->change_mtu = ath_change_mtu;
    dev->tx_queue_len = ATH_TXBUF-1;        /* 1 for mgmt frame */
#ifdef USE_HEADERLEN_RESV
    dev->hard_header_len += sizeof (struct ieee80211_qosframe) + sizeof(struct llc) + IEEE80211_ADDR_LEN + IEEE80211_WEP_IVLEN + IEEE80211_WEP_KIDLEN;
#ifdef ATH_SUPERG_FF
    dev->hard_header_len += ATH_FF_MAX_HDR;
#endif
#endif
    ic->ic_dev = dev;
    ic->ic_mgtstart = ath_mgtstart;
    ic->ic_init = ath_init;
    ic->ic_reset = ath_reset;
    ic->ic_newassoc = ath_newassoc;
    ic->ic_updateslot = ath_updateslot;

    ic->ic_wme.wme_update = ath_wme_update;
    ic->ic_addba_requestsetup = ath_aggr_addba_requestsetup;
    ic->ic_addba_requestprocess = ath_aggr_addba_requestprocess;
    ic->ic_addba_responseprocess = ath_aggr_addba_responseprocess;
    ic->ic_addba_responsesetup = ath_aggr_addba_responsesetup;
    ic->ic_delba_process = ath_aggr_delba_process;
    ic->ic_addba_send = ath_aggr_addba_send;
    ic->ic_delba_send = ath_aggr_delba_send;
    ic->ic_addba_status = ath_aggr_addba_status;

    /* XXX not right but it's not used anywhere important */
    ic->ic_phytype = IEEE80211_T_OFDM;
    ic->ic_opmode = IEEE80211_M_STA;
    /*
     * Set the Atheros Advanced Capabilities from station config before
     * starting 802.11 state machine.  Currently, set only fast-frames
     * capability.
     */
    ic->ic_ath_cap = 0;
    sc->sc_fftxqmin = ATH_FF_TXQMIN;
#ifdef ATH_SUPERG_FF
    ic->ic_ath_cap |= (ath_hal_fastframesupported(ah) ? IEEE80211_ATHC_FF : 0);
#endif
    ic->ic_ath_cap |= (ath_hal_burstsupported(ah) ? IEEE80211_ATHC_BURST : 0);

#ifdef ATH_SUPERG_COMP
    ic->ic_ath_cap |= (ath_hal_compressionsupported(ah) ? IEEE80211_ATHC_COMP : 0);
#endif

#ifdef ATH_SUPERG_DYNTURBO
    ic->ic_ath_cap |= (ath_hal_turboagsupported(ah) ?(IEEE80211_ATHC_TURBOP |
                            IEEE80211_ATHC_AR):0);
#endif
#ifdef ATH_SUPERG_XR
    ic->ic_ath_cap |= (ath_hal_xrsupported(ah) ? IEEE80211_ATHC_XR : 0);
#endif

    ic->ic_caps =
          IEEE80211_C_IBSS      /* ibss, nee adhoc, mode */
        | IEEE80211_C_HOSTAP        /* hostap mode */
        | IEEE80211_C_MONITOR       /* monitor mode */
        | IEEE80211_C_SHPREAMBLE    /* short preamble supported */
        | IEEE80211_C_SHSLOT        /* short slot time supported */
        | IEEE80211_C_WPA       /* capable of WPA1+WPA2 */
        | IEEE80211_C_BGSCAN        /* capable of bg scanning */
        ;
    /*
     * Query the hal to figure out h/w crypto support.
     */
    if (ath_hal_ciphersupported(ah, HAL_CIPHER_WEP))
        ic->ic_caps |= IEEE80211_C_WEP;
    if (ath_hal_ciphersupported(ah, HAL_CIPHER_AES_OCB))
        ic->ic_caps |= IEEE80211_C_AES;
    if (ath_hal_ciphersupported(ah, HAL_CIPHER_AES_CCM))
        ic->ic_caps |= IEEE80211_C_AES_CCM;
    if (ath_hal_ciphersupported(ah, HAL_CIPHER_CKIP))
        ic->ic_caps |= IEEE80211_C_CKIP;
    if (ath_hal_ciphersupported(ah, HAL_CIPHER_TKIP)) {
        ic->ic_caps |= IEEE80211_C_TKIP;
        /*
         * Check if h/w does the MIC and/or whether the
         * separate key cache entries are required to
         * handle both tx+rx MIC keys.
         */
        if (ath_hal_ciphersupported(ah, HAL_CIPHER_MIC)) {
            ic->ic_caps |= IEEE80211_C_TKIPMIC;
            /*
             * Check if h/w does MIC correctly when
             * WMM is turned on.
             */
            if (ath_hal_wmetkipmic(ah))
                ic->ic_caps |= IEEE80211_C_WME_TKIPMIC;
        }

        if (ath_hal_tkipsplit(ah))
            sc->sc_splitmic = 1;
    }
    sc->sc_hasclrkey = ath_hal_ciphersupported(ah, HAL_CIPHER_CLR);
#if 0
    sc->sc_mcastkey = ath_hal_getmcastkeysearch(ah);
#endif
    /*
     * TPC support can be done either with a global cap or
     * per-packet support.  The latter is not available on
     * all parts.  We're a bit pedantic here as all parts
     * support a global cap.
     */
    sc->sc_hastpc = ath_hal_hastpc(ah);
    if (sc->sc_hastpc || ath_hal_hastxpowlimit(ah))
        ic->ic_caps |= IEEE80211_C_TXPMGT;

    /*
     * Mark WME capability only if we have sufficient
     * hardware queues to do proper priority scheduling.
     */
    if (sc->sc_ac2q[WME_AC_BE] != sc->sc_ac2q[WME_AC_BK])
        ic->ic_caps |= IEEE80211_C_WME;

    /*
     * For now, default 11.h to start disabled.
     */
    ic->ic_flags &= ~IEEE80211_F_DOTH;

    ic->ic_htflags = 0;
    /* 11n Capabilities */
    if (ath_hal_htsupported(ah)) {
        ic->ic_htcap    = IEEE80211_HTCAP_C_SHORTGI40
                        | IEEE80211_HTCAP_C_CHWIDTH40
                        | IEEE80211_HTCAP_C_MIMOPWRSAVE_ALL
                        | IEEE80211_HTCAP_C_DSSSCCK40;
        ic->ic_htextcap = IEEE80211_HTCAP_EXTC_TRANS_TIME_5000
                        | IEEE80211_HTCAP_EXTC_MCS_FEEDBACK_NONE;
        ic->ic_maxampdu = IEEE80211_HTCAP_MAXRXAMPDU_65536;
        ic->ic_mpdudensity = IEEE80211_HTCAP_MPDUDENSITY_NA;
        ic->ic_flags_ext |= IEEE80211_C_AMPDU;
        ic->ic_ampdu_limit = IEEE80211_AMPDU_LIMIT_MAX;
        ic->ic_ampdu_subframes = IEEE80211_AMPDU_SUBFRAME_DEFAULT;
    }
    if (ic->ic_htcap & IEEE80211_HTCAP_C_SHORTGI40) {
        ic->ic_htflags |= IEEE80211_HTF_SHORTGI;
    }

#ifdef ATH_CWM
    /* Channel Width Management */
    error = ath_cwm_attach(sc);
    if (error != 0) {
        goto bad3;
    }
#endif

    if (ath_hal_hasfastcc(ah))
        ic->ic_flags_ext |= IEEE80211_FAST_CC;

    /*
     * Check for misc other capabilities.
     */
    if (ath_hal_hasbursting(ah))
        ic->ic_caps |= IEEE80211_C_BURST;
    if (ath_hal_hasfastcc(ah))
        ic->ic_caps |= IEEE80211_C_FASTCC;
    sc->sc_hasbmask = ath_hal_hasbssidmask(ah);
    sc->sc_hastsfadd = ath_hal_hastsfadjust(ah);
    /*
     * Indicate we need the 802.11 header padded to a
     * 32-bit boundary for 4-address and QoS frames.
     */
    ic->ic_flags |= IEEE80211_F_DATAPAD;

    ath_hal_getnummrretries(ah, &sc->sc_mrretries);

    if (ath_hal_gettxchainmask(ah, &chainmask))
        ic->ic_tx_chainmask = chainmask;

    if (ath_hal_getrxchainmask(ah, &chainmask))
        ic->ic_rx_chainmask = chainmask;

    /*
     * Query the hal about antenna support
     */
    if (ath_hal_hasdiversity(ah)) {
        sc->sc_hasdiversity = 1;
        sc->sc_diversity = ath_hal_getdiversity(ah);
    }
    sc->sc_defant = ath_hal_getdefantenna(ah);

    /*
     * Not all chips have the VEOL support we want to
     * use with IBSS beacons; check here for it.
     */
    sc->sc_hasveol = ath_hal_hasveol(ah);

    /* get mac address from hardware */
    ath_hal_getmac(ah, ic->ic_myaddr);
    if (sc->sc_hasbmask) {
        ath_hal_getbssidmask(ah, sc->sc_bssidmask);
        ATH_SET_VAP_BSSID_MASK(sc->sc_bssidmask);
        ath_hal_setbssidmask(ah, sc->sc_bssidmask);
    }
    IEEE80211_ADDR_COPY(dev->dev_addr, ic->ic_myaddr);

    /* call MI attach routine. */
    ieee80211_ifattach(ic);
    /* override default methods */
    ic->ic_node_alloc = ath_node_alloc;
    sc->sc_node_free = ic->ic_node_free;
    ic->ic_node_free = ath_node_free;
    ic->ic_node_getrssi = ath_node_getrssi;
#ifdef ATH_SUPERG_XR
    ic->ic_node_move_data = ath_node_move_data;
#endif
    sc->sc_recv_mgmt = ic->ic_recv_mgmt;
    ic->ic_recv_mgmt = ath_recv_mgmt;

    #ifdef ATH_FORCE_PPM
    ic->ic_force_ppm_notify = ath_force_ppm_notify;
    #endif

    ic->ic_vap_create = ath_vap_create;
    ic->ic_vap_delete = ath_vap_delete;

    ic->ic_scan_start = ath_scan_start;
    ic->ic_scan_end = ath_scan_end;
    ic->ic_set_channel = ath_set_channel;

    ic->ic_set_coverageclass = ath_set_coverageclass;
    ic->ic_mhz2ieee = ath_mhz2ieee;

    if (register_netdev(dev)) {
        printk(KERN_ERR "%s: unable to register device\n", dev->name);
        goto bad4;
    }
    /*
     * Attach dynamic MIB vars and announce support
     * now that we have a device name with unit number.
     */
#ifdef CONFIG_SYSCTL
    ath_dynamic_sysctl_register(sc);
#endif /* CONFIG_SYSCTL */
    ieee80211_announce(ic);
    ath_announce(dev);
#ifdef DEBUG_PKTLOG
        ath_pktlog_attach(sc);
#endif
    return 0;
bad4:
    ieee80211_ifdetach(ic);
#ifdef ATH_CWM
    ath_cwm_detach(sc);
#endif
bad3:
    ath_rate_detach(sc->sc_rc);
bad2:
    ath_tx_cleanup(sc);
    ath_desc_free(sc);
bad:
    if (ah)
        ath_hal_detach(ah);
    ATH_TXBUF_LOCK_DESTROY(sc);
    ATH_LOCK_DESTROY(sc);
    sc->sc_invalid = 1;

    return error;
}

int
ath_detach(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;

    DPRINTF(sc, ATH_DEBUG_ANY, "%s: flags %x\n", __func__, dev->flags);
    ath_stop(dev);

    ath_hal_setpower(sc->sc_ah, HAL_PM_AWAKE);

    sc->sc_invalid = 1;

    /*
     * NB: the order of these is important:
     * o call the 802.11 layer before detaching the hal to
     *   insure callbacks into the driver to delete global
     *   key cache entries can be handled
     * o reclaim the tx queue data structures after calling
     *   the 802.11 layer as we'll get called back to reclaim
     *   node state and potentially want to use them
     * o to cleanup the tx queues the hal is called, so detach
     *   it last
     * Other than that, it's straightforward...
     */
    ieee80211_ifdetach(&sc->sc_ic);
#ifdef ATH_CWM
    ath_cwm_detach(sc);
#endif
    ath_rate_detach(sc->sc_rc);
    ath_desc_free(sc);
    ath_tx_cleanup(sc);
    ath_hal_detach(ah);

#ifdef CONFIG_SYSCTL
    ath_dynamic_sysctl_unregister(sc);
#endif /* CONFIG_SYSCTL */
#ifdef DEBUG_PKTLOG
        if(sc->pl_info)
		ath_pktlog_detach(sc);
#endif
    ATH_LOCK_DESTROY(sc);
    unregister_netdev(dev);
#ifdef ATH_TX99_DIAG
        ath_tx99ctl(0,0,NULL);
        ath_rx99ctl(0,0,NULL);
#endif
    return 0;
}

static void 
ath_get_chainmask(struct ath_softc *sc, HAL_HT_MISC *misc)
{
   ath_chainmask_sel_t *cm  = &sc->sc_chainmask_sel;
   misc->ht_txchainmask = cm->cur_tx_mask;
   misc->ht_rxchainmask = cm->cur_rx_mask;
}


#ifdef ATH_CHAINMASK_SELECT

static void
ath_chainmask_sel_timertimeout(struct ath_softc *sc, void *timerArg)
{
    ath_chainmask_sel_t *cm = ( ath_chainmask_sel_t *)timerArg;
    cm->switch_allowed = 1;
}

/*
 * Start chainmask select timer
 *
 */
static void
ath_chainmask_sel_timerstart(struct ath_softc *sc, ath_chainmask_sel_t *cm)
{
    cm->switch_allowed = 0;
    ath_timer_set(&cm->timer, ath_chainmask_sel_timertimeout, cm, 
						MSECS(ath_chainmask_sel_period));
    ath_timer_add(sc, &cm->timer);
}

/*
 * Stop chainmask select timer
 *
 */
void
ath_chainmask_sel_timerstop(struct ath_softc *sc, ath_chainmask_sel_t *cm)
{
	/* Timer is running currently */
    if (!cm->switch_allowed)
		return;
    cm->switch_allowed = 0;
    ath_timer_del(sc, &cm->timer);
}

int 
ath_chainmask_sel_logic(struct ath_softc *sc)
{
    ath_chainmask_sel_t *cm  = &sc->sc_chainmask_sel; 

    /* Disable auto-swtiching in one of the following if conditions.
     * cm->cm_sel_enabled is used for internal global auto-switching
     * enabled/disabled setting */

    if (!ath_chainmask_sel_enable) {
	return cm->cur_tx_mask;
    }

    if (cm->tx_avgrssi == ATH_RSSI_DUMMY_MARKER)
	return cm->cur_tx_mask;

    if (cm->switch_allowed) {

	/* Switch down from tx 3 to tx 2. */

    	if (cm->cur_tx_mask == ATH_CHAINMASK_SEL_3X3  &&
	    ATH_RSSI_OUT(cm->tx_avgrssi) >= 
				 	ath_chainmask_sel_down_rssi_thres) {

		cm->cur_tx_mask = sc->sc_ic.ic_tx_chainmask;

		/* Don't let another switch happen until this
		   timer expires */

		ath_chainmask_sel_timerstart(sc, cm);
    	} 
	/* Switch up from tx 2 to 3. */
    	else if (cm->cur_tx_mask == sc->sc_ic.ic_tx_chainmask  &&
		ATH_RSSI_OUT(cm->tx_avgrssi) <= 
					ath_chainmask_sel_up_rssi_thres) {

		cm->cur_tx_mask =  ATH_CHAINMASK_SEL_3X3;

		/* Don't let another switch happen until this
		   timer expires */

		ath_chainmask_sel_timerstart(sc, cm);
    	}
    }

    return cm->cur_tx_mask;
}

void
ath_chainmask_sel_init(struct ath_softc *sc)
{
    ath_chainmask_sel_t *cm  = &sc->sc_chainmask_sel; 

    memset(cm, 0, sizeof(ath_chainmask_sel_t));

    cm->cur_tx_mask = sc->sc_ic.ic_tx_chainmask;
    cm->cur_rx_mask = sc->sc_ic.ic_rx_chainmask;
    cm->tx_avgrssi = ATH_RSSI_DUMMY_MARKER;
}

#endif

#ifdef ATH_FORCE_PPM
/*
 * Force PPM tracking hack as workaround for FOWL bug.
 */

/*
 * One second periodic timer to support force ppm tracking.
 * Divides down for local timer1 and timer2 use.
 */
static void
ath_force_ppm_timertimeout(struct ath_softc *sc, void *timerArg)
{
    ath_force_ppm_t *afp = ( ath_force_ppm_t *)timerArg;

    if (ath_force_ppm_enable && afp->isRunning) {

        /*
         * timer1, one-shot, time when next sample window is to be started
         */
        if (afp->timerStart1) {
            afp->timerCount1++;
            if (afp->timerCount1 >= afp->timerThrsh1) {
                afp->timerStart1 = 0;
                afp->timerCount1 = 0;
            }
        }

        /*
         * timer2, retrig, times out on lack of receive activity
         */
        if (afp->timerStart2) {
            afp->timerStart2 = 0;
            afp->timerCount2 = 0;
        } else {
            afp->timerCount2++;
            if (afp->timerCount2 >= afp->timerThrsh2) {
                afp->timerCount2 = 0;
                /* re-start */
                ath_force_ppm_init(sc, 0);
                afp->isRunning = 1;
            }
        }
    }

    /* next period */
    ath_force_ppm_timerstart(sc, &sc->sc_force_ppm);
}

/*
 * Start 1 sec timer used for force ppm
 */
static void
ath_force_ppm_timerstart(struct ath_softc *sc, ath_force_ppm_t *afp)
{
    afp->timer_running = 1;
    ath_timer_set(&afp->timer, ath_force_ppm_timertimeout, afp, MSECS(ATH_FORCE_PPM_PERIOD));
    ath_timer_add(sc, &afp->timer);
}

/*
 * Stop 1 sec timer used for force ppm
 */
void
ath_force_ppm_timerstop(struct ath_softc *sc, ath_force_ppm_t *afp)
{
    if (afp->timer_running) {
        afp->timer_running = 0;
        /* Timer is running currently */
        ath_timer_del(sc, &afp->timer);
    }
}

/* 12 bit 2's complement */
int
getTwosComplement12(int val)
{
    int tmpval = val;
    int invval;

    if (val > 0x7ff) {
        tmpval = val & 0x7ff;
        invval = (~tmpval + 1) & 0x7ff;
        tmpval = 0 - invval;
    }
    return tmpval;
}

static int
ath_PpmIsRssi(struct ath_softc *sc, u_int32_t latched_val, u_int32_t desc_val)
{
    u_int32_t        cmpMask;
    HAL_BOOL         useSwap = 0;

    // if (A_REG_RD(pDev, PHY_ANALOG_SWAP) & PHY_SWAP_ALT_CHAIN) {
    if (sc->sc_ic.ic_tx_chainmask == 5 || sc->sc_ic.ic_rx_chainmask == 5) {
        useSwap = 1;
    }

    switch (sc->sc_ic.ic_rx_chainmask) {
    case 1:
        cmpMask = 0x000000ff;
        break;
    case 3:
    case 5:
        cmpMask = useSwap ? 0x00ff00ff : 0x0000ffff;
        break;
    case 7:
        cmpMask = 0x00ffffff;
        break;
    default:
        cmpMask = 0x00000000;
        return 0;
        break;
    }

    if ((latched_val & cmpMask) == (desc_val & cmpMask)) {
        return 1;
    }
    return 0;
}

/*
 * Try a comparison of the rx timestamp against the tsf of the trigger arm.
 * Rx timestamp is 32 bits, tsf is 64 bits at 1 uSec resolution.
 * If no frames received within a timeout, external timer resets state.
 */
static int
ath_PpmCheckTsf(u_int32_t test, u_int32_t rxTimestamp)
{
    if ((rxTimestamp >= test) || 
        ((rxTimestamp < test) && ((rxTimestamp - test) < 0x80000000)))
    {
        return 1;
    }
    return 0;
}

void
ath_force_ppm_init(struct ath_softc *sc, int cold)
{
    ath_force_ppm_t *afp  = &sc->sc_force_ppm; 
    struct ath_hal *ah = sc->sc_ah;

    if (cold) {
        memset(&afp->timer, 0, sizeof(struct ath_timer));
        afp->timer_running = 0;
    }

    afp->isRunning   = 0;
    afp->timerStart1 = 0;
    afp->timerCount1 = 0;
    afp->timerThrsh1 = ath_force_ppm_period & 0xff;
    afp->timerStart2 = 0;
    afp->timerCount2 = 0;
    afp->timerThrsh2 = ath_force_ppm_wd;
    afp->forceState  = ST_FORCE_PPM_INIT;
    afp->lastTsf1    = 0;
    afp->lastTsf2    = 0;
    afp->latchedRssi = 0x00808080;
    afp->addrValid = 0;

    /* remove force */
    ath_hal_ppmUnForce(ah);

    /* DEBUG */
    afp->dumpForcePpmFlag = 0;
    if (ath_force_ppm_enable & ATH_DEBUG_PPM_PRINT) {
        printk("FORCE_PPM Init %d %10u\n", cold, jiffies_to_msecs(jiffies));
    }
}

/*
 * State machine to track fine PPM and force Course PPM.
 * Clocked on rx frame activity.
 */
int 
ath_force_ppm_logic(struct ath_softc *sc, struct ath_buf *bf, HAL_STATUS status, 
                    struct ath_rx_status *rx_stats, int32_t *pLogArray)
{
    ath_force_ppm_t     *afp = &sc->sc_force_ppm; 
    struct ath_hal      *ah  = sc->sc_ah;
    struct ieee80211com *ic  = &sc->sc_ic;
    int ppmIsTrig, ppmIsAggr, ppmIsTsf, ppmIsRssi, ppmIsSrcOk, ppmIsTsf2, ppmIsArm;
    u_int32_t ppmDescRssi;
    u_int32_t lastTsfLog;
    int forcePpmStateCur;
    u_int32_t   thistsf;
    struct ieee80211_frame *wh;
    struct sk_buff *skb;
    u_int32_t tempFine = -2047;
    int type;
    int statErr;

    ppmIsTrig = 0;
    ppmIsTsf = 0;
    ppmIsAggr = 0;
    ppmIsRssi = 0;
    ppmIsSrcOk = 0;
    ppmDescRssi = 0x00808080;
    ppmIsTsf2 = 0;
    ppmIsArm = 0;
    statErr = 1;

    /* kick rx timeout */
    afp->timerStart2 = 1;

    lastTsfLog = afp->lastTsf1;
    forcePpmStateCur = afp->forceState;

    skb = bf->bf_skb;
    wh = (struct ieee80211_frame *)skb->data;
    type = (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) >> IEEE80211_FC0_TYPE_SHIFT;

    if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
        struct ieee80211_node *ni1;

        /* ic_sta_assoc must already be verified == 1 before getting here */
        ni1 = ieee80211_find_rxnode(ic, (const struct ieee80211_frame_min *) skb->data);
        if (ni1 != NULL) {
            if (ni1->ni_associd != 0 &&
                ni1->ni_vap->iv_state == IEEE80211_S_RUN)
            {
                ppmIsSrcOk = 1;
            }
            ieee80211_free_node(ni1);
        }
    }

    thistsf = ath_hal_gettsf32(ah);

    /* state */
    switch (forcePpmStateCur) {
    case ST_FORCE_PPM_INIT:
        /* INIT */
        if (afp->isRunning) {
            afp->forceState = ST_FORCE_PPM_ARMED;
            ppmIsArm = 1;
            afp->lastTsf1 = ath_hal_ppmArmTrigger(ah);
        }
        break;

    case ST_FORCE_PPM_ARMED:
        /* ARMED */
        if (ath_hal_ppmGetTrigger(ah)) {
            afp->lastTsf2 = thistsf;
            ppmIsTrig = 1;
            afp->latchedRssi = ath_hal_ppmGetRssiDump(ah);
            ppmDescRssi = ((rx_stats->rs_rssi_ctl0 & 0xff) <<  0) |
                          ((rx_stats->rs_rssi_ctl1 & 0xff) <<  8) |
                          ((rx_stats->rs_rssi_ctl2 & 0xff) << 16);
            ppmIsRssi = ath_PpmIsRssi(sc, afp->latchedRssi, ppmDescRssi);
            ppmIsTsf = ath_PpmCheckTsf(afp->lastTsf1, rx_stats->rs_tstamp);
            ppmIsAggr = (rx_stats->rs_isaggr && rx_stats->rs_moreaggr) ? 1 : 0;
            if (ppmIsRssi && ppmIsTsf && !ppmIsAggr) {

                if ((status == HAL_OK) &&
                   ((rx_stats->rs_status & ~(HAL_RXERR_DECRYPT | HAL_RXERR_MIC)) == 0) &&
                   ((rx_stats->rs_flags & (HAL_RX_DELIM_CRC_PRE | HAL_RX_DELIM_CRC_POST | HAL_RX_DECRYPT_BUSY)) == 0))
                {
                    statErr = 0;
                }

                /* Frame originated from our one specific STA, is OK, and is Data or Mgmt type */
                if (ppmIsSrcOk && !statErr && !(type & 1)) {
                    afp->forceState = ST_FORCE_PPM_NEXT;
                    tempFine = ath_hal_ppmForce(ah);

                    // DEBUG ONLY
                    afp->dumpForcePpmFlag = 1;

                } else {
                    /* match but not us, restart */
                    afp->forceState = ST_FORCE_PPM_ARMED;
                    ppmIsArm = 1;
                    afp->lastTsf1 = ath_hal_ppmArmTrigger(ah);
                }
            } else {
                /* rssi not match  or tsf not reached, or in aggr */
                /* check next pkt until timeout, then retrigger */
                afp->forceState = ST_FORCE_PPM_SEARCH;
            }
        }
        /* else not triggered, stay in this state */
        break;

    case ST_FORCE_PPM_SEARCH:
        /* SEARCHING */
        ppmDescRssi = ((rx_stats->rs_rssi_ctl0 & 0xff) <<  0) |
                      ((rx_stats->rs_rssi_ctl1 & 0xff) <<  8) |
                      ((rx_stats->rs_rssi_ctl2 & 0xff) << 16);
        ppmIsRssi = ath_PpmIsRssi(sc, afp->latchedRssi, ppmDescRssi);
        ppmIsTsf = ath_PpmCheckTsf(afp->lastTsf1, rx_stats->rs_tstamp);
        ppmIsAggr = (rx_stats->rs_isaggr && rx_stats->rs_moreaggr) ? 1 : 0;
        ppmIsTsf2 = ath_PpmCheckTsf(ath_force_ppm_to + afp->lastTsf2, rx_stats->rs_tstamp);

        if (ppmIsRssi && ppmIsTsf && !ppmIsAggr) {

            if ((status == HAL_OK) &&
               ((rx_stats->rs_status & ~(HAL_RXERR_DECRYPT | HAL_RXERR_MIC)) == 0) &&
               ((rx_stats->rs_flags & (HAL_RX_DELIM_CRC_PRE | HAL_RX_DELIM_CRC_POST | HAL_RX_DECRYPT_BUSY)) == 0))
            {
                statErr = 0;
            }

            /* Frame originated from our one specific STA, is OK, and is Data or Mgmt type */
            if (ppmIsSrcOk && !statErr && !(type & 1)) {
                afp->forceState = ST_FORCE_PPM_NEXT;
                tempFine = ath_hal_ppmForce(ah);

                // DEBUG ONLY
                afp->dumpForcePpmFlag = 1;

            } else {
                /* match but not us, restart */
                afp->forceState = ST_FORCE_PPM_ARMED;
                ppmIsArm = 1;
                afp->lastTsf1 = ath_hal_ppmArmTrigger(ah);
            }
        } else if (ppmIsTsf2 && !ppmIsAggr) {
            /*
             * do not need to try any more as these frames are at
             * least 12 ms newer than the most recent trigger
             * event, can restart
             */
            afp->forceState = ST_FORCE_PPM_ARMED;
            ppmIsArm = 1;
            afp->lastTsf1 = ath_hal_ppmArmTrigger(ah);
        }
        /* else stay in this state */
        break;

    case ST_FORCE_PPM_IDLE:
        /* IDLE */
        if (!afp->timerStart1) {
            afp->forceState = ST_FORCE_PPM_ARMED;
            ppmIsArm = 1;
            afp->lastTsf1 = ath_hal_ppmArmTrigger(ah);
        }
        break;

    case ST_FORCE_PPM_NEXT:
        /* START NEXT TIMER */
        afp->forceState = ST_FORCE_PPM_IDLE;
        afp->timerStart1 = 1;
        break;

    default:
        KASSERT(0, ("PPM STATE ILLEGAL %x %x\n", forcePpmStateCur, afp->forceState));
        ath_force_ppm_init(sc, 0);
        break;
    }

#ifdef DEBUG_PKTLOG
    pLogArray[0] = thistsf;
    pLogArray[1] = lastTsfLog;
    pLogArray[2] = afp->lastTsf2;
    pLogArray[3] = (ppmIsTrig          << 31)  | (ppmIsArm   << 30) |
                   (afp->isRunning     << 29)  | (ppmIsRssi  << 28) | 
                   (ppmIsTsf           << 27)  | (ppmIsAggr  << 26) |
                   (afp->timerStart1   << 25)  | (ppmIsSrcOk << 24);
    pLogArray[3] |= afp->latchedRssi;
    pLogArray[4] = (forcePpmStateCur << 28)    | (afp->forceState << 24);
    pLogArray[4] |= ((rx_stats->rs_rssi_ctl0 & 0xff) <<  0) |
                    ((rx_stats->rs_rssi_ctl1 & 0xff) <<  8) |
                    ((rx_stats->rs_rssi_ctl2 & 0xff) << 16);
    pLogArray[5] = tempFine;
    pLogArray[6] = (statErr      << 31) |
                   (type         << 28) |
                   (wh->i_addr2[3] << 16) |
                   (wh->i_addr2[4] <<  8) | 
                   (wh->i_addr2[5] <<  0) ;
#endif /* DEBUG_PKTLOG */

    // DEBUG
    if ((ath_force_ppm_enable & ATH_DEBUG_PPM_PRINT) && afp->dumpForcePpmFlag) {
        afp->dumpForcePpmFlag = 0;
        /* tempFine_2C, st-desc_rssi, src, thisTSF, armTSF, fine_reg, check_reg ts_ms */
        printk("FORCE_PPM Update %4d %6.6x %8.8x %8.8x %8.8x %3.3x %4.4x %10u\n",
            getTwosComplement12(pLogArray[5]),
            pLogArray[4],
            pLogArray[6],
            pLogArray[0],
            pLogArray[1],
            tempFine,
            ath_hal_ppmGetReg(ah, 0x9800 + (4 << 2)) & 0x1fff,
            jiffies_to_msecs(jiffies));
    }

    return 0;
}

static void
ath_force_ppm_notify(struct ieee80211_node *ni, int ev)
{
    struct ath_softc *sc;
    struct ieee80211com *ic;
    ath_force_ppm_t *afp;

    if (ath_force_ppm_enable) {

        ic = ni->ni_ic;
        sc = ni->ni_ic->ic_dev->priv;
        afp = &sc->sc_force_ppm; 

        if (ic->ic_sta_assoc == 1) {
            afp->addrValid = 1;
            afp->isRunning = 1;
        } else {
            afp->addrValid = 0;
            ath_force_ppm_init(sc, 0);
        }

        /* DEBUG */
        if (ath_force_ppm_enable & ATH_DEBUG_PPM_PRINT) {
            printk("FORCE_PPM Notify %d %d %d  %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x %10u\n",
                    ev,
                    ic->ic_sta_assoc,
                    afp->addrValid,
                    ni->ni_macaddr[0], ni->ni_macaddr[1], ni->ni_macaddr[2],
                    ni->ni_macaddr[3], ni->ni_macaddr[4], ni->ni_macaddr[5],
                    jiffies_to_msecs(jiffies));
        }
    }
}
#endif /* ATH_FORCE_PPM */


static void 
ath_get_hwstate(struct ath_softc *sc, HAL_HT *ht)
{

   /* Get CWM state to configure in HAL */
   ath_cwm_gethwstate(sc, &ht->cwm);

   /* Get TX/RX chainmask */
   ath_get_chainmask(sc, &ht->misc);

    return;
}

static struct ieee80211vap *
ath_vap_create(struct ieee80211com *ic, const char *name, int unit,
    int opmode, int flags)
{
    struct ath_softc *sc = ic->ic_dev->priv;
    struct net_device *dev;
    struct ath_vap *avp;
    struct ieee80211vap *vap;
    int ic_opmode;

    /* XXX ic unlocked and race against add */
    switch (opmode) {
    case IEEE80211_M_STA:   /* ap+sta for repeater application */
        if (sc->sc_nstavaps != 0)  /* only one sta regardless */
            return NULL;
        if ((sc->sc_nvaps != 0) && (!(flags & IEEE80211_NO_STABEACONS)))
            return NULL;   /* If using station beacons, must first up */
        if (flags & IEEE80211_NO_STABEACONS) {
            sc->sc_nostabeacons = 1;
            ic_opmode = IEEE80211_M_HOSTAP; /* Run with chip in AP mode */
        } else
            ic_opmode = opmode;
        break;
    case IEEE80211_M_IBSS:
    case IEEE80211_M_MONITOR:
        if (sc->sc_nvaps != 0)      /* only one */
            return NULL;
        ic_opmode = opmode;
        break;
    case IEEE80211_M_HOSTAP:
    case IEEE80211_M_WDS:
        /* permit multiple ap's and/or wds links */
        /* XXX sta+ap for repeater/bridge application */
        if ((sc->sc_nvaps != 0) && (ic->ic_opmode == IEEE80211_M_STA))
            return NULL;
        /* XXX not right, beacon buffer is allocated on RUN trans */
        if (opmode == IEEE80211_M_HOSTAP && TAILQ_EMPTY(&sc->sc_bbuf)) {
            return NULL;
        }
        /*
         * XXX Not sure if this is correct when operating only
         * with WDS links.
         */
        ic_opmode = IEEE80211_M_HOSTAP;

        break;
    default:
        return NULL;
    }
    dev = alloc_etherdev(sizeof(struct ath_vap) + sc->sc_rc->arc_vap_space);
    if (dev == NULL) {
        /* XXX msg */
        return NULL;
    }
    avp = dev->priv;
    ieee80211_vap_setup(ic, dev, name, unit, opmode, flags);
    /* override with driver methods */
    vap = &avp->av_vap;
    avp->av_newstate = vap->iv_newstate;
    vap->iv_newstate = ath_newstate;
    vap->iv_key_alloc = ath_key_alloc;
    vap->iv_key_delete = ath_key_delete;
    vap->iv_key_set = ath_key_set;
    vap->iv_key_update_begin = ath_key_update_begin;
    vap->iv_key_update_end = ath_key_update_end;
#ifdef ATH_SUPERG_COMP
    vap->iv_comp_set = ath_comp_set;
#endif

    /*
     * Change the interface type for monitor mode.
     */
    if (ic_opmode == IEEE80211_M_MONITOR)
        dev->type = ARPHRD_IEEE80211_PRISM;
    if ((flags & IEEE80211_CLONE_BSSID) &&
        sc->sc_nvaps != 0 && opmode != IEEE80211_M_WDS && sc->sc_hasbmask) {
        struct ieee80211vap *v;
        int id_mask, id;

        /*
         * Hardware supports the bssid mask and a unique
         * bssid was requested.  Assign a new mac address
         * and expand our bssid mask to cover the active
         * virtual ap's with distinct addresses.
         */
        KASSERT(sc->sc_nvaps <= ATH_BCBUF,
            ("too many virtual ap's: %d", sc->sc_nvaps));

        /* do a full search to mark all the allocated vaps */
        id_mask = 0;
        TAILQ_FOREACH(v, &ic->ic_vaps, iv_next)
            id_mask |= (1 << ATH_GET_VAP_ID(v->iv_myaddr));

        for (id = 0; id < ATH_BCBUF; id++) {
            /* get the first available slot */
            if ((id_mask & (1 << id)) == 0) {
                ATH_SET_VAP_BSSID(vap->iv_myaddr, id);
                break;
            }
        }
    }
    avp->av_bslot = -1;
    TAILQ_INIT(&avp->av_mcastq.axq_q);
    ATH_TXQ_LOCK_INIT(&avp->av_mcastq);
    if (opmode == IEEE80211_M_HOSTAP || opmode == IEEE80211_M_IBSS) {
        /*
         * Allocate beacon state for hostap/ibss.  We know
         * a buffer is available because of the check above.
         */
        avp->av_bcbuf = TAILQ_FIRST(&sc->sc_bbuf);
        TAILQ_REMOVE(&sc->sc_bbuf, avp->av_bcbuf, bf_list);
        if (opmode == IEEE80211_M_HOSTAP || !sc->sc_hasveol) {
            int slot;
            /*
             * Assign the vap to a beacon xmit slot.  As
             * above, this cannot fail to find one.
             */
            avp->av_bslot = 0;
            for (slot = 0; slot < ATH_BCBUF; slot++)
                if (sc->sc_bslot[slot] == NULL) {
                    /*
                     * XXX hack, space out slots to better
                     * deal with misses
                     */
                    if (slot+1 < ATH_BCBUF &&
                        sc->sc_bslot[slot+1] == NULL) {
                        avp->av_bslot = slot+1;
                        break;
                    }
                    avp->av_bslot = slot;
                    /* NB: keep looking for a double slot */
                }
            KASSERT(sc->sc_bslot[avp->av_bslot] == NULL,
                ("beacon slot %u not empty?", avp->av_bslot));
            sc->sc_bslot[avp->av_bslot] = vap;
        }
        if (sc->sc_hastsfadd)
            ath_hal_settsfadjust(sc->sc_ah, 1);
    } else {
        if (sc->sc_hastsfadd)
            ath_hal_settsfadjust(sc->sc_ah, 0);
    }
    /* complete setup */
    (void) ieee80211_vap_attach(vap,
        ieee80211_media_change, ieee80211_media_status);

    ic->ic_opmode = ic_opmode;
    sc->sc_nvaps++;
    if (opmode == IEEE80211_M_STA)
        sc->sc_nstavaps++;

#ifdef ATH_SUPERG_XR
    if ( vap->iv_flags & IEEE80211_F_XR ) {
        if (ath_descdma_setup(sc, &sc->sc_grppolldma, &sc->sc_grppollbuf,
                              "grppoll",(sc->sc_xrpollcount+1)*HAL_ANTENNA_MAX_MODE,1) != 0) {
            printk("%s:grppoll Buf allocation failed \n",__func__);
        }
        if(!sc->sc_xrtxq)
            sc->sc_xrtxq = ath_txq_setup(sc, HAL_TX_QUEUE_DATA,HAL_XR_DATA );
    }
#endif

    return vap;
}

void
ath_vap_delete(struct ieee80211vap *vap)
{
    struct net_device *dev = vap->iv_ic->ic_dev;
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    struct ath_vap *avp = ATH_VAP(vap);

    KASSERT(vap->iv_state == IEEE80211_S_INIT, ("vap not stopped"));

    if (dev->flags & IFF_RUNNING) {
        /*
         * Quiesce the hardware while we remove the vap.  In
         * particular we need to reclaim all references to the
         * vap state by any frames pending on the tx queues.
         *
         * XXX can we do this w/o affecting other vap's?
         */
        ath_hal_intrset(ah, 0);     /* disable interrupts */
        ath_draintxq(sc);       /* stop xmit side */
        ath_stoprecv(sc);       /* stop recv side */
    }

    /*
     * Reclaim any pending mcast bufs on the vap.
     */
    ath_tx_draintxq(sc, &avp->av_mcastq);
    ATH_TXQ_LOCK_DESTROY(&avp->av_mcastq);

    /*
     * Reclaim beacon state.  Note this must be done before
     * vap instance is reclaimed as we may have a reference
     * to it in the buffer for the beacon frame.
     */
    if (avp->av_bcbuf != NULL) {
        if (avp->av_bslot != -1)
            sc->sc_bslot[avp->av_bslot] = NULL;
        ath_beacon_return(sc, avp->av_bcbuf);
        avp->av_bcbuf = NULL;
    }
    if (vap->iv_opmode == IEEE80211_M_STA) {
        sc->sc_nstavaps--;
        if (sc->sc_nostabeacons)
            sc->sc_nostabeacons = 0;
    }
    ieee80211_vap_detach(vap);
    /* NB: memory is reclaimed through dev->destructor callback */
    sc->sc_nvaps--;

#ifdef ATH_SUPERG_XR
    /*
     * if its an XR vap ,free the memory allocated explicitly.
     * since the XR vap is not registered , OS can not free the memory.
     */
    if(vap->iv_flags & IEEE80211_F_XR) {
        ath_grppoll_stop(vap);
        ath_descdma_cleanup(sc,&sc->sc_grppolldma,&sc->sc_grppollbuf, BUS_DMA_FROMDEVICE);
        memset(&sc->sc_grppollbuf , 0 , sizeof(sc->sc_grppollbuf));
        memset(&sc->sc_grppolldma , 0 , sizeof(sc->sc_grppolldma));
        if(vap->iv_xrvap)
            vap->iv_xrvap->iv_xrvap=NULL;
        kfree(vap->iv_dev);
        ath_tx_cleanupq(sc,sc->sc_xrtxq);
        sc->sc_xrtxq=NULL;
    }
#endif

    if (dev->flags & IFF_RUNNING) {
        /*
         * Restart rx+tx machines if device is still running.
         */
        if (ath_startrecv(sc) != 0) /* restart recv */
            printk("%s: %s: unable to start recv logic\n",
                dev->name, __func__);
        if (sc->sc_beacons) {
            ath_beacon_config(sc, NULL);    /* restart beacons */
        }
        ath_hal_intrset(ah, sc->sc_imask);
    }
}

void
ath_suspend(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;

    sc->sc_stats.ast_suspend ++;

    DPRINTF(sc, ATH_DEBUG_ANY, "%s: flags %x\n", __func__, dev->flags);
    ath_stop(dev);
}

void
ath_resume(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;

    sc->sc_stats.ast_resume ++;

    DPRINTF(sc, ATH_DEBUG_ANY, "%s: flags %x\n", __func__, dev->flags);
    ath_init(dev);
}

void
ath_shutdown(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;

    sc->sc_stats.ast_shutdown ++;

    DPRINTF(sc, ATH_DEBUG_ANY, "%s: flags %x\n", __func__, dev->flags);
    ath_stop(dev);
}

/*
 * Interrupt handler.  Most of the actual processing is deferred.
 */
irqreturn_t
ath_intr(int irq, void *dev_id, struct pt_regs *regs)
{
    struct net_device *dev = dev_id;
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    HAL_INT status;
    int needmark;

    if (sc->sc_invalid) {
        /*
         * The hardware is not ready/present, don't touch anything.
         * Note this can happen early on if the IRQ is shared.
         */
        return IRQ_NONE;
    }
    if (!ath_hal_intrpend(ah)) {      /* shared irq, not for us */
#ifdef __LINUX_ARM_ARCH__
        /*
         * a temporary(!) workaround for spurious interrupt problem.
         * Since AP71 doesnt share IRQ lines, we always return handled.
         * We could also turn off linux spurious int warnings - which would 
         * be equivalent to this.
         */
        return IRQ_HANDLED;
#else
        return IRQ_NONE;
#endif
    }

    if ((dev->flags & (IFF_RUNNING|IFF_UP)) != (IFF_RUNNING|IFF_UP)) {
        DPRINTF(sc, ATH_DEBUG_INTR, "%s: flags 0x%x\n",
            __func__, dev->flags);
        ath_hal_getisr(ah, &status);    /* clear ISR */
        ath_hal_intrset(ah, 0);     /* disable further intr's */
        return IRQ_HANDLED;
    }
    needmark = 0;
    /*
     * Figure out the reason(s) for the interrupt.  Note
     * that the hal returns a pseudo-ISR that may include
     * bits we haven't explicitly enabled so we mask the
     * value to insure we only process bits we requested.
     */
    ath_hal_getisr(ah, &status);        /* NB: clears ISR too */
    DPRINTF(sc, ATH_DEBUG_INTR, "%s: status 0x%x\n", __func__, status);
    status &= sc->sc_imask;         /* discard unasked for bits */
    if (status & HAL_INT_FATAL) {
        sc->sc_stats.ast_hardware++;
        ath_hal_intrset(ah, 0);     /* disable intr's until reset */
        ATH_SCHEDULE_TQUEUE(&sc->sc_fataltq, &needmark);
    } else {
        if (status & HAL_INT_RXORN) {
            sc->sc_stats.ast_rxorn++;
#ifndef OWLAGGR
            /*
             * owl: rx overflow is not treated fatal
             */
            ath_hal_intrset(ah, 0);     /* disable intr's until reset */
#endif
            ATH_SCHEDULE_TQUEUE(&sc->sc_rxorntq, &needmark);
        }

        if (status & HAL_INT_SWBA) {
            /*
             * Software beacon alert--time to send a beacon.
             * Handle beacon transmission directly; deferring
             * this is too slow to meet timing constraints
             * under load.
             */
            ath_beacon_tasklet(sc, &needmark);
        }

#ifdef __LINUX_MIPS32_ARCH__
        /*
         * Hydra needs DDR FIFO flush before any desc/dma data can be read.
         */
        //hsumc ar7100_flush_pci();
#endif

        if (status & HAL_INT_RXEOL) {
            sc->sc_stats.ast_rxeol++;
#ifdef OWLAGGR
            /*
             * owl: do receive processing and replenish rx buffers
             */
            ATH_SCHEDULE_TQUEUE(&sc->sc_rxtq, &needmark);
#else
            sc->sc_rxlink = NULL;
#endif
        }
        if (status & HAL_INT_TXURN) {
            sc->sc_stats.ast_txurn++;
            /* bump tx trigger level */
            ath_hal_updatetxtriglevel(ah, AH_TRUE);
        }
	if (status & HAL_INT_RX) {
		/* disable interrupt */
		sc->sc_imask &= ~HAL_INT_RX;
		ath_hal_intrset(ah, sc->sc_imask);
		ATH_SCHEDULE_TQUEUE(&sc->sc_rxtq, &needmark);
	}
        if (status & HAL_INT_TX) {
#ifdef ATH_SUPERG_DYNTURBO
            /*
             * Check if the beacon queue caused the interrupt
             * when a dynamic turbo switch
             * is pending so we can initiate the change.
             * XXX must wait for all vap's beacons
             */

            if (sc->sc_dturbo_switch) {
                u_int32_t txqs= (1 << sc->sc_bhalq);
                ath_hal_gettxintrtxqs(ah,&txqs);
                if(txqs & (1 << sc->sc_bhalq)) {
                    sc->sc_dturbo_switch = 0;
                    /*
                     * Hack: defer switch for 10ms to permit slow
                     * clients time to track us.  This especially
                     * noticeable with Windows clients.
                     */
                    mod_timer(&sc->sc_dturbo_switch_mode,
                              jiffies + ((HZ * 10) / 1000));
                }
            }
#endif
	    /* disable interrupt */
	    sc->sc_imask &= ~HAL_INT_TX;
	    ath_hal_intrset(ah, sc->sc_imask);
            ATH_SCHEDULE_TQUEUE(&sc->sc_txtq, &needmark);
        }
        if (status & HAL_INT_BMISS) {
            sc->sc_stats.ast_bmiss++;
            ATH_SCHEDULE_TQUEUE(&sc->sc_bmisstq, &needmark);
        }

        /* tx timeout interrupt */
        if (status & HAL_INT_GTT) {
            /* disable interrupt */
            ath_hal_intrset(ah, sc->sc_imask & ~HAL_INT_GTT);
            sc->sc_stats.ast_txto++;
        }

	/* carrier sense timeout */
	if (status & HAL_INT_CST) {
	    /* disable interrupt */
	    ath_hal_intrset(ah, sc->sc_imask & ~HAL_INT_CST);

	    sc->sc_stats.ast_cst++;
	    ATH_SCHEDULE_TQUEUE(&sc->sc_txtotq, &needmark);
	}

        if (status & HAL_INT_MIB) {
            sc->sc_stats.ast_mib++;
            /*
             * Disable interrupts until we service the MIB
             * interrupt; otherwise it will continue to fire.
             */
            ath_hal_intrset(ah, 0);
            /*
             * Let the hal handle the event.  We assume it will
             * clear whatever condition caused the interrupt.
             */
            ath_hal_mibevent(ah, &sc->sc_halstats);
            ath_hal_intrset(ah, sc->sc_imask);
        }
    }
    if (needmark)
        mark_bh(IMMEDIATE_BH);
    return IRQ_HANDLED;
}

static void
ath_fatal_tasklet(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;

    printk("%s: hardware error; reseting\n", dev->name);
    ath_resetinternal(dev, RESET_FATAL);
}

static void
ath_rxorn_tasklet(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;

    printk("%s: rx FIFO overrun; reseting\n", dev->name);
    ath_resetinternal(dev, RESET_FATAL);
}

static void
ath_bmiss_tasklet(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;
    struct ath_softc *sc = dev->priv;

    DPRINTF(sc, ATH_DEBUG_ANY, "%s\n", __func__);

    ieee80211_beacon_miss(&sc->sc_ic);
}

static void
ath_txto_tasklet(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;
    struct ath_softc *sc = dev->priv;

    DPRINTF(sc, ATH_DEBUG_XMIT, "%s\n", __func__);

    /* Notify CWM */
    ath_cwm_txtimeout(sc);
}


static u_int
ath_chan2flags(struct ieee80211_channel *chan)
{
    u_int flags;
    static const u_int modeflags[] = {
        0,          /* IEEE80211_MODE_AUTO    */
        CHANNEL_A,      /* IEEE80211_MODE_11A     */
        CHANNEL_B,      /* IEEE80211_MODE_11B     */
        CHANNEL_PUREG,      /* IEEE80211_MODE_11G     */
        0,                      /* IEEE80211_MODE_FH      */
        CHANNEL_108A,       /* IEEE80211_MODE_TURBO_A */
        CHANNEL_108G,       /* IEEE80211_MODE_TURBO_G */
        CHANNEL_A_HT20,     /* IEEE80211_MODE_11NA */
        CHANNEL_G_HT20,     /* IEEE80211_MODE_11NG */

    };

    flags = modeflags[ieee80211_chan2mode(chan)];
    flags = chan->ic_flags;

    if (IEEE80211_IS_CHAN_HALF(chan)) {
        flags |= CHANNEL_HALF;
    } else if (IEEE80211_IS_CHAN_QUARTER(chan)) {
        flags |= CHANNEL_QUARTER;
    }

    return flags;
}

static int
ath_init(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;
    HAL_STATUS status;
    int error = 0;

    ath_timer_init(sc);

    sc->sc_stats.ast_init ++;

    ATH_LOCK(sc);

    DPRINTF(sc, ATH_DEBUG_RESET, "%s: mode %d\n", __func__, ic->ic_opmode);

    /*
     * Stop anything previously setup.  This is safe
     * whether this is the first time through or not.
     */
    ath_stop_locked(dev);

#if defined(ATH_CAP_TPC)
    ath_hal_setcapability(sc->sc_ah, HAL_CAP_TPC, 0, 1, NULL);
#endif

    /* Whether we should enable h/w TKIP MIC */
    if ((ic->ic_caps & IEEE80211_C_WME) == 0)
        ath_hal_setcapability(sc->sc_ah, HAL_CAP_TKIP_MIC, 0, 0, NULL);
    else {
        if (((ic->ic_caps & IEEE80211_C_WME_TKIPMIC) == 0) &&
            (ic->ic_flags & IEEE80211_F_WME)) {
            ath_hal_setcapability(sc->sc_ah, HAL_CAP_TKIP_MIC, 0, 0, NULL);
        }
        else
            ath_hal_setcapability(sc->sc_ah, HAL_CAP_TKIP_MIC, 0, 1, NULL);
    }

    /*
     * Flush the skb's allocated for receive in case the rx
     * buffer size changes.  This could be optimized but for
     * now we do it each time under the assumption it does
     * not happen often.
     */
    ath_flushrecv(sc);

#ifdef ATH_CWM
    ath_cwm_init(sc);
#endif
#ifdef ATH_CHAINMASK_SELECT
    ath_chainmask_sel_init(sc);
#endif

#ifdef ATH_FORCE_PPM
    /* Init logic used for force ppm workaround */
    ath_force_ppm_init(sc, 1);
#endif


    /*
     * The basic interface to setting the hardware in a good
     * state is ``reset''.  On return the hardware is known to
     * be powered up and with interrupts disabled.  This must
     * be followed by initialization of the appropriate bits
     * and then setup of the interrupt mask.
     */
    sc->sc_curchan.channel = ic->ic_curchan->ic_freq;
    sc->sc_curchan.channelFlags = ath_chan2flags(ic->ic_curchan);

    if (ath_hal_htsupported(ah)) {
        HAL_HT ht;
        ath_get_hwstate(sc, &ht);
        if (!ath_hal_reset11n(ah, ic->ic_opmode, &sc->sc_curchan,
                       &ht, AH_FALSE, &status)) {
            printk("%s: unable to reset hardware; hal status %u "
                "(freq %u flags 0x%x) "
                    "(ht.cwm.mac %d, ht.cwm.phy %d, ht.cwm.extoffset %d\n", dev->name, status,
                sc->sc_curchan.channel, sc->sc_curchan.channelFlags,
                    ht.cwm.ht_macmode, ht.cwm.ht_phymode, ht.cwm.ht_extoff);
            error = -EIO;
            goto done;
        }
    sc->sc_currht = ht;
    } else {
        if (!ath_hal_reset(ah, ic->ic_opmode, &sc->sc_curchan, AH_FALSE, &status)) {
            printk("%s: unable to reset hardware; hal status %u "
                "(freq %u flags 0x%x)\n", dev->name, status,
                sc->sc_curchan.channel, sc->sc_curchan.channelFlags);
            error = -EIO;
            goto done;
        }
    }

    /*
     * This is needed only to setup initial state
     * but it's best done after a reset.
     */
    ath_update_txpow(sc);

    /*
     * Setup the hardware after reset: the key cache
     * is filled as needed and the receive engine is
     * set going.  Frame transmit is handled entirely
     * in the frame output path; there's nothing to do
     * here except setup the interrupt mask.
     */
#if 0
    ath_initkeytable(sc);       /* XXX still needed? */
#endif
    if (ath_startrecv(sc) != 0) {
        printk("%s: unable to start recv logic\n", dev->name);
        error = -EIO;
        goto done;
    }
#ifdef ATH_TX99_DIAG
    /* reset the tx99 state */
    sc->sc_txrx99.tx99mode=0;
    if(sc->sc_txrx99.rx99mode)
                ath_rx99ctl(1,0,sc);
#endif
    /*
     * Enable interrupts.
     */
    sc->sc_imask = HAL_INT_RX | HAL_INT_TX
          | HAL_INT_RXEOL | HAL_INT_RXORN
          | HAL_INT_FATAL | HAL_INT_GLOBAL;
    if (ath_hal_gttsupported(ah)) {
        sc->sc_imask |= HAL_INT_GTT;
    }
    if (ath_hal_htsupported(ah)) {
	sc->sc_imask |= HAL_INT_CST;
    }

    /*
     * Enable MIB interrupts when there are hardware phy counters.
     * Note we only do this (at the moment) for station mode.
     */
    if (sc->sc_needmib && ic->ic_opmode == IEEE80211_M_STA)
        sc->sc_imask |= HAL_INT_MIB;

    /*
     * The hardware should be ready to go now so it's safe
     * to kick the 802.11 state machine as it's likely to
     * immediately call back to us to send mgmt frames.
     */
    ath_chan_change(sc, ic->ic_curchan);
    dev->flags |= IFF_RUNNING;      /* we are ready to go */
    ieee80211_start_running(ic);        /* start all vap's */

    /*
     * Now that we're mostly set, enable our irq line
     */
    if (request_irq(dev->irq, ath_intr, SA_SHIRQ, dev->name, dev)) {
        printk(KERN_WARNING "%s: request_irq failed\n", dev->name);
        error = -EIO;
    }

#ifdef ATH_CHAINMASK_SELECT
    ath_chainmask_sel_timerstart(sc, &sc->sc_chainmask_sel);
#endif

#ifdef  ATH_FORCE_PPM
    /* Start timer used for force ppm workaround */
    ath_force_ppm_timerstart(sc, &sc->sc_force_ppm);
#endif

    ath_hal_intrset(ah, sc->sc_imask);

done:
    ATH_UNLOCK(sc);
    return error;
}

static int
ath_stop_locked(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;

    sc->sc_stats.ast_stop ++;

    DPRINTF(sc, ATH_DEBUG_RESET, "%s: invalid %u flags 0x%x\n",
        __func__, sc->sc_invalid, dev->flags);

    if (dev->flags & IFF_RUNNING) {
        /*
         * Shutdown the hardware and driver:
         *    stop output from above
         *    reset 802.11 state machine
         *  (sends station deassoc/deauth frames)
         *    turn off timers
         *    disable interrupts
         *    clear transmit machinery
         *    clear receive machinery
         *    turn off the radio
         *    reclaim beacon resources
         *
         * Note that some of this work is not possible if the
         * hardware is gone (invalid).
         */
        netif_stop_queue(dev);  /* XXX re-enabled by ath_newstate */
        dev->flags &= ~IFF_RUNNING; /* NB: avoid recursion */
        ieee80211_stop_running(ic); /* stop all vap's */
        if (!sc->sc_invalid) {
            ath_hal_intrset(ah, 0);
            if (sc->sc_softled) {
                del_timer(&sc->sc_ledtimer);
                ath_hal_gpioset(ah, sc->sc_ledpin, 1);
            }
        }
        ath_draintxq(sc);
        if (!sc->sc_invalid) {
            ath_stoprecv(sc);
            ath_hal_phydisable(ah);
        } else
            sc->sc_rxlink = NULL;
        ath_beacon_free(sc);        /* XXX needed? */ 

        if (dev->irq)
            free_irq(dev->irq, dev);
    }
    return 0;
}

/*
 * Stop the device, grabbing the top-level lock to protect
 * against concurrent entry through ath_init (which can happen
 * if another thread does a system call and the thread doing the
 * stop is preempted).
 */
static int
ath_stop(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    int error;

    ATH_LOCK(sc);

    ath_timer_exit(sc);
    ath_cwm_stop(sc);

    if (!sc->sc_invalid)
        ath_hal_setpower(sc->sc_ah, HAL_PM_AWAKE);

    error = ath_stop_locked(dev);
#if 0
    if (error == 0 && !sc->sc_invalid) {
        /*
         * Set the chip in full sleep mode.  Note that we are
         * careful to do this only when bringing the interface
         * completely to a stop.  When the chip is in this state
         * it must be carefully woken up or references to
         * registers in the PCI clock domain may freeze the bus
         * (and system).  This varies by chip and is mostly an
         * issue with newer parts that go to sleep more quickly.
         */
        ath_hal_setpower(sc->sc_ah, HAL_PM_FULL_SLEEP);
    }
#endif
    ATH_UNLOCK(sc);

    return error;
}

static int
ath_resetinternal(struct net_device *dev, int fatal)
{
	struct ath_softc *sc = dev->priv;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ath_hal *ah = sc->sc_ah;
	struct ieee80211_channel *c;
	HAL_STATUS status;
	HAL_BOOL chanchange;

	DPRINTF(sc, ATH_DEBUG_RESET, "%s: fatal %u\n",
	    __func__, fatal);

	sc->sc_stats.ast_reset++;
	chanchange = fatal ? AH_FALSE : AH_TRUE;

	/*
	 * Convert to a HAL channel description with the flags
	 * constrained to reflect the current operating mode.
	 */
	c = ic->ic_curchan;
	sc->sc_curchan.channel = c->ic_freq;
	sc->sc_curchan.channelFlags = ath_chan2flags(c);

	netif_stop_queue(dev);  

	ath_hal_intrset(ah, 0);     /* disable interrupts */
	ath_draintxq(sc);       /* stop xmit side */
	ath_stoprecv(sc);       /* stop recv side */
	/* NB: indicate channel change so we do a full reset */
	if (ath_hal_htsupported(ah)) {
	    HAL_HT ht;
	    ath_get_hwstate(sc, &ht);
	    if (!ath_hal_reset11n(ah, ic->ic_opmode, &sc->sc_curchan,
			   &ht, chanchange, &status)) {
		printk("%s: %s: unable to reset hardware; hal status %u "
		       "(ht.cwm.mac %d, ht.cwm.phy %d, ht.cwm.extoffset %d\n",
		    dev->name, __func__, status,
			ht.cwm.ht_macmode, ht.cwm.ht_phymode, ht.cwm.ht_extoff);
	    }
	    sc->sc_currht = ht;
	} else {
	    if (!ath_hal_reset(ah, ic->ic_opmode, &sc->sc_curchan, chanchange, &status))
		printk("%s: %s: unable to reset hardware; hal status %u\n",
		    dev->name, __func__, status);
	}
	ath_update_txpow(sc);       /* update tx power state */
	if (ath_startrecv(sc) != 0) /* restart recv */
	    printk("%s: %s: unable to start recv logic\n",
		dev->name, __func__);
	/*
	 * We may be doing a reset in response to an ioctl
	 * that changes the channel so update any state that
	 * might change as a result.
	 */
	ath_chan_change(sc, c);
	if (sc->sc_beacons)
	    ath_beacon_config(sc, NULL);    /* restart beacons */
	ath_hal_intrset(ah, sc->sc_imask);

	netif_wake_queue(dev);      /* restart xmit */

    #ifdef ATH_SUPERG_XR
	/*
	 * restart  the group polls.
	 */
	if(sc->sc_xrgrppoll) {
	    struct ieee80211vap *vap;
	    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
		if(vap && (vap->iv_flags & IEEE80211_F_XR)) break;
	    ath_grppoll_stop(vap);
	    ath_grppoll_start(vap,sc->sc_xrpollcount);
	}
    #endif
    
    return 0;
}


/*
 * Reset the hardware w/o losing operational state.  This is
 * basically a more efficient way of doing ath_stop, ath_init,
 * followed by state transitions to the current 802.11
 * operational state.  Used to recover from errors rx overrun
 * and to reset the hardware when rf gain settings must be reset.
 */
static int
ath_reset(struct net_device *dev)
{
	/* ath_reset defaults to non-fatal reset */
	return ath_resetinternal(dev, RESET_NONFATAL);
}

#ifdef ATH_SUPERG_FF
/*
 * Flush FF staging queue.
 */
static int
ath_ff_neverflushtestdone(struct ath_txq *txq, struct ath_buf *bf)
{
    return 0;
}

static int
ath_ff_ageflushtestdone(struct ath_txq *txq, struct ath_buf *bf)
{
    if ( (txq->axq_totalqueued - bf->bf_queueage) < ATH_FF_STAGEQAGEMAX ) {
        return 1;
    }
    return 0;
}

static void
ath_ffstageq_flush(struct ath_softc *sc, struct ath_txq *txq,
                   int (*ath_ff_flushdonetest)(struct ath_txq *txq, struct ath_buf *bf))
{
    struct ath_buf *bf_ff;
    struct ieee80211_node *ni;
    int pktlen;
    int framecnt;

    /*
     * NB: using _BH style locking even though this function may be called
     *     at interrupt time (within tasklet or bh). This should be harmless
     *     and this function calls others (i.e., ath_tx_start()) which do
     *     the same.
     */
    for (;;) {
        ATH_TXQ_LOCK_BH(txq);

        bf_ff = TAILQ_LAST(&txq->axq_stageq, axq_headtype);
        if ((!bf_ff) || ath_ff_flushdonetest(txq, bf_ff)) {
            break;
        }

        ni = bf_ff->bf_node;
        KASSERT(ATH_NODE(ni)->an_tx_ffbuf[bf_ff->bf_skb->priority], ("no bf_ff on staging queue %p", bf_ff));
        ATH_NODE(ni)->an_tx_ffbuf[bf_ff->bf_skb->priority] = NULL;
        TAILQ_REMOVE(&txq->axq_stageq, bf_ff, bf_list);

        ATH_TXQ_UNLOCK_BH(txq);

        /* encap and xmit */
        bf_ff->bf_skb = ieee80211_encap(ni, bf_ff->bf_skb, &framecnt);
        if (bf_ff->bf_skb == NULL) {
            DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
                    "%s: discard, encapsulation failure\n", __func__);
            sc->sc_stats.ast_tx_encap++;
            goto bad;
        }
        pktlen = bf_ff->bf_skb->len;    /* NB: don't reference skb below */
        if (ath_tx_start(sc->sc_dev, ni, bf_ff,
                 bf_ff->bf_skb, 0) == 0)
            continue;
    bad:
        ieee80211_free_node(ni);
        if (bf_ff->bf_skb != NULL) {
            dev_kfree_skb(bf_ff->bf_skb);
            bf_ff->bf_skb = NULL;
        }
        bf_ff->bf_node = NULL;

        ATH_TXBUF_LOCK_BH(sc);
        TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf_ff, bf_list);
        ATH_TXBUF_UNLOCK_BH(sc);
    }
    ATH_TXQ_UNLOCK_BH(txq);
}
#endif

#define ATH_HARDSTART_GET_TX_BUF_WITH_LOCK              \
    ATH_TXBUF_LOCK_BH(sc);                      \
    bf = TAILQ_FIRST(&sc->sc_txbuf);                \
    if (bf != NULL) {                       \
        TAILQ_REMOVE(&sc->sc_txbuf, bf, bf_list);           \
        TAILQ_INSERT_TAIL(&bf_head, bf, bf_list);                  \
    }                                                               \
    /* XXX use a counter and leave at least one for mgmt frames */  \
    if (TAILQ_EMPTY(&sc->sc_txbuf)) {               \
        DPRINTF(sc, ATH_DEBUG_XMIT, "%s: stop queue\n", __func__);  \
        sc->sc_stats.ast_tx_qstop++;                \
        netif_stop_queue(dev);                  \
        sc->sc_devstopped=1;                    \
        ATH_SCHEDULE_TQUEUE(&sc->sc_txtq, NULL); \
    }                               \
    ATH_TXBUF_UNLOCK_BH(sc);                    \
    if (bf == NULL) {       /* NB: should not happen */ \
        DPRINTF(sc,ATH_DEBUG_XMIT,"%s: discard, no xmit buf\n", __func__);      \
        sc->sc_stats.ast_tx_nobuf++;                \
        goto hardstart_fail;                            \
    }

/*
 * Transmit a data packet.  On failure caller is
 * assumed to reclaim the resources.
 */
static int
ath_hardstart(struct sk_buff *skb, struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    struct ieee80211_node *ni = NULL;
    struct ath_buf *bf = NULL;
    struct ieee80211_cb *cb = (struct ieee80211_cb *) skb->cb;
    struct ether_header *eh;
    int pktlen;
    TAILQ_HEAD(tmp_bf_head, ath_buf) bf_head;
    struct ath_buf *tbf, *tempbf;
    struct sk_buff *tskb;
    int framecnt;
#ifdef ATH_SUPERG_FF
    struct ath_node *an;
    struct ath_txq *txq;
    int ff_flush;
    struct ieee80211vap *vap;
#endif

    if ((dev->flags & IFF_RUNNING) == 0 || sc->sc_invalid) {
        DPRINTF(sc, ATH_DEBUG_XMIT,
            "%s: discard, invalid %d flags %x\n",
            __func__, sc->sc_invalid, dev->flags);
        sc->sc_stats.ast_tx_invalid++;
        return -ENETDOWN;
    }

    TAILQ_INIT(&bf_head);
    eh = (struct ether_header *)skb->data;
    ni = cb->ni;        /* NB: always passed down by 802.11 layer */
    if (ni == NULL) {
        /* NB: this happens if someone marks the underlying device up */
        DPRINTF(sc, ATH_DEBUG_XMIT,
            "%s: discard, no node in cb\n", __func__);
        goto hardstart_fail;
    }
#ifdef ATH_SUPERG_FF
    vap = ni->ni_vap;

    /*
     * Fast frames check.
     */
    ATH_FF_MAGIC_CLR(skb);
    an = ATH_NODE(ni);

    /* NB: use this lock to protect an->an_ff_txbuf in athff_can_aggregate()
     *     call too.
     */
    txq = sc->sc_ac2q[skb->priority];

    if (txq->axq_depth > TAIL_DROP_COUNT) {
        sc->sc_stats.ast_tx_discard++;
        goto hardstart_fail;
    }

    ATH_TXQ_LOCK_BH(txq);
    if (athff_can_aggregate(sc, eh, an, skb, vap->iv_fragthreshold, &ff_flush)) {

        if (an->an_tx_ffbuf[skb->priority]) { /* i.e., frame on the staging queue */
            bf = an->an_tx_ffbuf[skb->priority];

            /* get (and remove) the frame from staging queue */
            TAILQ_REMOVE(&txq->axq_stageq, bf, bf_list);
            an->an_tx_ffbuf[skb->priority] = NULL;

            ATH_TXQ_UNLOCK_BH(txq);

            /*
             * chain skbs and add FF magic
             *
             * NB: the arriving skb should not be on a list (skb->list),
             *     so "re-using" the skb next field should be OK.
             */
            bf->bf_skb->next = skb;
            skb->next = NULL;
            skb = bf->bf_skb;
            ATH_FF_MAGIC_PUT(skb);

            /* decrement extra node reference made when an_tx_ffbuf[] was set */
            ieee80211_free_node(ni);

            DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
                "%s: aggregating fast-frame\n", __func__);
        }
        else {
            /* NB: careful grabbing the TX_BUF lock since still holding the txq lock.
             *     this could be avoided by always obtaining the txbuf earlier,
             *     but the "if" portion of this "if/else" clause would then need
             *     to give the buffer back.
             */
            ATH_HARDSTART_GET_TX_BUF_WITH_LOCK;
            DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
                "%s: adding to fast-frame stage Q\n", __func__);

            bf->bf_skb = skb;
            bf->bf_node = ni;
            bf->bf_queueage = txq->axq_totalqueued;
            an->an_tx_ffbuf[skb->priority] = bf;

            TAILQ_INSERT_HEAD(&txq->axq_stageq, bf, bf_list);

            ATH_TXQ_UNLOCK_BH(txq);

            return 0;
        }
    }
    else {
        if (ff_flush) {
            struct ath_buf *bf_ff = an->an_tx_ffbuf[skb->priority];

            TAILQ_REMOVE(&txq->axq_stageq, bf_ff, bf_list);
            an->an_tx_ffbuf[skb->priority] = NULL;

            ATH_TXQ_UNLOCK_BH(txq);

            /* encap and xmit */
            bf_ff->bf_skb = ieee80211_encap(ni, bf_ff->bf_skb, &framecnt);

            if (bf_ff->bf_skb == NULL) {
                DPRINTF(sc, ATH_DEBUG_XMIT,
                    "%s: discard, ff flush encap failure\n", __func__);
                sc->sc_stats.ast_tx_encap++;
                goto ff_flushbad;
            }
            pktlen = bf_ff->bf_skb->len;    /* NB: don't reference skb below */
            /* NB: ath_tx_start() will use ATH_TXBUF_LOCK_BH(). The _BH
             *     portion is not needed here since we're running at
             *     interrupt time, but should be harmless.
             */
            if (ath_tx_start(dev, ni, bf_ff, bf_ff->bf_skb, 0)) {
                goto ff_flushbad;
            }
            goto ff_flushdone;
        ff_flushbad:
            DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
                "%s: ff stageq flush failure\n", __func__);
            ieee80211_free_node(ni);
            if (bf_ff->bf_skb) {
                dev_kfree_skb(bf_ff->bf_skb);
                bf_ff->bf_skb = NULL;
            }
            bf_ff->bf_node = NULL;

            ATH_TXBUF_LOCK_BH(sc);
            TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf_ff, bf_list);
            ATH_TXBUF_UNLOCK_BH(sc);
            goto ff_flushdone;
        }
        /*
         * XXX: out-of-order condition only occurs for AP mode and multicast.
         *      But, there may be no valid way to get this condition.
         */
        else if (an->an_tx_ffbuf[skb->priority]) {
            DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
                "%s: Out-Of-Order fast-frame\n", __func__);
            ATH_TXQ_UNLOCK_BH(txq);
        }
        else {
            ATH_TXQ_UNLOCK_BH(txq);
        }

    ff_flushdone:
        ATH_HARDSTART_GET_TX_BUF_WITH_LOCK;
    }

#else /* ATH_SUPERG_FF */

    ATH_HARDSTART_GET_TX_BUF_WITH_LOCK;

#endif /* ATH_SUPERG_FF */

    /*
     * Encapsulate the packet for transmission.
     */
    skb = ieee80211_encap(ni, skb, &framecnt);
    if (skb == NULL) {
        DPRINTF(sc, ATH_DEBUG_XMIT,
            "%s: discard, encapsulation failure\n", __func__);
        sc->sc_stats.ast_tx_encap++;
        goto hardstart_fail;
    }

    if (framecnt > 1) {
        int bfcnt;

        /*
        **  Allocate 1 ath_buf for each frame given 1 was
        **  already alloc'd
        */
        ATH_TXBUF_LOCK_BH(sc);
        for (bfcnt = 1; bfcnt < framecnt; ++bfcnt) {
            if ((tbf = TAILQ_FIRST(&sc->sc_txbuf)) != NULL) {
                TAILQ_REMOVE(&sc->sc_txbuf, tbf, bf_list);
                TAILQ_INSERT_TAIL(&bf_head, tbf, bf_list);
            }
            else
                break;

            ieee80211_node_incref(ni);
        }

        if (bfcnt != framecnt) {
            if ( ! TAILQ_EMPTY(&bf_head)) {
                /*
                **  Failed to alloc enough ath_bufs;
                **  return to sc_txbuf list
                */
                TAILQ_FOREACH_SAFE(tbf, &bf_head, bf_list, tempbf) {
                    TAILQ_INSERT_TAIL(&sc->sc_txbuf, tbf, bf_list);
                }
            }
            ATH_TXBUF_UNLOCK_BH(sc);
            TAILQ_INIT(&bf_head);
            goto hardstart_fail;
        }
        ATH_TXBUF_UNLOCK_BH(sc);

        while ((bf = TAILQ_FIRST(&bf_head)) != NULL && skb != NULL) {
            int nextfraglen = 0;

            TAILQ_REMOVE(&bf_head, bf, bf_list);
            tskb = skb->next;
            skb->next = NULL;
            if (tskb)
                nextfraglen = tskb->len;

            if (ath_tx_start(dev, ni, bf, skb, nextfraglen) != 0) {
                TAILQ_INSERT_TAIL(&bf_head, bf, bf_list);
                skb->next = tskb;
                goto hardstart_fail;
            }
            skb = tskb;
        }
    }
    else {
        if (ath_tx_start(dev, ni, bf, skb, 0) != 0) {
            TAILQ_INSERT_TAIL(&bf_head, bf, bf_list);
            goto hardstart_fail;
        }
    }

#ifdef ATH_SUPERG_FF
    /*
     * flush out stale FF from staging Q for applicable operational modes.
     */
    /* XXX: ADHOC mode too? */
    if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
        ath_ffstageq_flush(sc, txq, ath_ff_ageflushtestdone);
    }
#endif

    return 0;

hardstart_fail:
    if ( ! TAILQ_EMPTY(&bf_head)) {
        ATH_TXBUF_LOCK_BH(sc);
        TAILQ_FOREACH_SAFE(tbf, &bf_head, bf_list, tempbf) {
            tbf->bf_skb = NULL;
            tbf->bf_node = NULL;
            TAILQ_INSERT_TAIL(&sc->sc_txbuf, tbf, bf_list);

            if (ni != NULL)
                ieee80211_free_node(ni);
        }
        ATH_TXBUF_UNLOCK_BH(sc);
    }

    /* free sk_buffs */
    while (skb) {
        tskb = skb->next;
        skb->next = NULL;
        dev_kfree_skb(skb);
        skb = tskb;
    }

    return 0;   /* NB: return !0 only in a ``hard error condition'' */
}
#undef ATH_HARDSTART_GET_TX_BUF_WITH_LOCK

/*
 * Transmit a management frame.  On failure we reclaim the skbuff.
 * Note that management frames come directly from the 802.11 layer
 * and do not honor the send queue flow control.  Need to investigate
 * using priority queueing so management frames can bypass data.
 */
static int
ath_mgtstart(struct ieee80211com *ic, struct sk_buff *skb)
{
    struct net_device *dev = ic->ic_dev;
    struct ath_softc *sc = dev->priv;
    struct ieee80211_node *ni = NULL;
    struct ath_buf *bf = NULL;
    struct ieee80211_cb *cb;
    int error;

    /*
     * NB: the referenced node pointer is in the
     * control block of the sk_buff.  This is
     * placed there by ieee80211_mgmt_output because
     * we need to hold the reference with the frame.
     */
    cb = (struct ieee80211_cb *)skb->cb;
    ni = cb ? cb->ni : NULL;

    if ((dev->flags & IFF_RUNNING) == 0 || sc->sc_invalid) {
        DPRINTF(sc, ATH_DEBUG_XMIT,
            "%s: discard, invalid %d flags %x\n",
            __func__, sc->sc_invalid, dev->flags);
        sc->sc_stats.ast_tx_invalid++;
        error = -ENETDOWN;
        goto bad;
    }
    /*
     * Grab a TX buffer and associated resources.
     */
    ATH_TXBUF_LOCK_BH(sc);
    bf = TAILQ_FIRST(&sc->sc_txbuf);
    if (bf != NULL)
        TAILQ_REMOVE(&sc->sc_txbuf, bf, bf_list);
    if (TAILQ_EMPTY(&sc->sc_txbuf)) {
        DPRINTF(sc, ATH_DEBUG_XMIT, "%s: stop queue\n", __func__);
        sc->sc_stats.ast_tx_qstop++;
        netif_stop_queue(dev);
        sc->sc_devstopped=1;
        ATH_SCHEDULE_TQUEUE(&sc->sc_txtq, NULL);
    }
    ATH_TXBUF_UNLOCK_BH(sc);
    if (bf == NULL) {
        printk("ath_mgtstart: discard, no xmit buf\n");
        sc->sc_stats.ast_tx_nobufmgt++;
        error = -ENOBUFS;
        goto bad;
    }


    error = ath_tx_start(dev, ni, bf, skb, 0);
    if (error == 0) {
        sc->sc_stats.ast_tx_mgmt++;
        return 0;
    }
    /* fall thru... */
bad:
    if (ni != NULL)
        ieee80211_free_node(ni);
    if (bf != NULL) {
        bf->bf_skb = NULL;
        bf->bf_node = NULL;

        ATH_TXBUF_LOCK_BH(sc);
        TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf, bf_list);
        ATH_TXBUF_UNLOCK_BH(sc);
    }
    dev_kfree_skb(skb);
    return error;
}

#ifdef AR_DEBUG
static void
ath_keyprint(const char *tag, u_int ix,
    const HAL_KEYVAL *hk, const u_int8_t mac[IEEE80211_ADDR_LEN])
{
    static const char *ciphers[] = {
        "WEP",
        "AES-OCB",
        "AES-CCM",
        "CKIP",
        "TKIP",
        "CLR",
    };
    int i, n;

    printk("%s: [%02u] %-7s ", tag, ix, ciphers[hk->kv_type]);
    for (i = 0, n = hk->kv_len; i < n; i++)
        printk("%02x", hk->kv_val[i]);
    printk(" mac %s", ether_sprintf(mac));
    if (hk->kv_type == HAL_CIPHER_TKIP) {
        printk(" mic ");
        for (i = 0; i < sizeof(hk->kv_mic); i++)
            printk("%02x", hk->kv_mic[i]);
    }
    printk("\n");
}
#endif

/*
 * Set a TKIP key into the hardware.  This handles the
 * potential distribution of key state to multiple key
 * cache slots for TKIP.
 */
static int
ath_keyset_tkip(struct ath_softc *sc, const struct ieee80211_key *k,
    HAL_KEYVAL *hk, const u_int8_t mac[IEEE80211_ADDR_LEN])
{
#define IEEE80211_KEY_XR    (IEEE80211_KEY_XMIT | IEEE80211_KEY_RECV)
    static const u_int8_t zerobssid[IEEE80211_ADDR_LEN];
    struct ath_hal *ah = sc->sc_ah;

    KASSERT(k->wk_cipher->ic_cipher == IEEE80211_CIPHER_TKIP,
        ("got a non-TKIP key, cipher %u", k->wk_cipher->ic_cipher));
    KASSERT(sc->sc_splitmic, ("key cache !split"));
    if ((k->wk_flags & IEEE80211_KEY_XR) == IEEE80211_KEY_XR) {
        /*
         * TX key goes at first index, RX key at +32.
         * The hal handles the MIC keys at index+64.
         */
        memcpy(hk->kv_mic, k->wk_txmic, sizeof(hk->kv_mic));
        KEYPRINTF(sc, k->wk_keyix, hk, zerobssid);
        if (!ath_hal_keyset(ah, k->wk_keyix, hk, zerobssid))
            return 0;

        memcpy(hk->kv_mic, k->wk_rxmic, sizeof(hk->kv_mic));
        KEYPRINTF(sc, k->wk_keyix+32, hk, mac);
        /* XXX delete tx key on failure? */
        return ath_hal_keyset(ah, k->wk_keyix+32, hk, mac);
    } else if (k->wk_flags & IEEE80211_KEY_XR) {
        /*
         * TX/RX key goes at first index.
         * The hal handles the MIC keys are index+64.
         */
        memcpy(hk->kv_mic, k->wk_flags & IEEE80211_KEY_XMIT ?
            k->wk_txmic : k->wk_rxmic, sizeof(hk->kv_mic));
        KEYPRINTF(sc, k->wk_keyix, hk, mac);
        return ath_hal_keyset(ah, k->wk_keyix, hk, mac);
    }
    return 0;
#undef IEEE80211_KEY_XR
}

/*
 * Set a net80211 key into the hardware.  This handles the
 * potential distribution of key state to multiple key
 * cache slots for TKIP with hardware MIC support.
 */
static int
ath_keyset(struct ath_softc *sc, const struct ieee80211_key *k,
    const u_int8_t mac0[IEEE80211_ADDR_LEN],
    struct ieee80211_node *bss)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    static const u_int8_t ciphermap[] = {
        HAL_CIPHER_WEP,     /* IEEE80211_CIPHER_WEP */
        HAL_CIPHER_TKIP,    /* IEEE80211_CIPHER_TKIP */
        HAL_CIPHER_AES_OCB, /* IEEE80211_CIPHER_AES_OCB */
        HAL_CIPHER_AES_CCM, /* IEEE80211_CIPHER_AES_CCM */
        (u_int8_t) -1,      /* 4 is not allocated */
        HAL_CIPHER_CKIP,    /* IEEE80211_CIPHER_CKIP */
        HAL_CIPHER_CLR,     /* IEEE80211_CIPHER_NONE */
    };
    struct ath_hal *ah = sc->sc_ah;
    const struct ieee80211_cipher *cip = k->wk_cipher;
    u_int8_t gmac[IEEE80211_ADDR_LEN];
    const u_int8_t *mac;
    HAL_KEYVAL hk;

    memset(&hk, 0, sizeof(hk));
    /*
     * Software crypto uses a "clear key" so non-crypto
     * state kept in the key cache are maintained and
     * so that rx frames have an entry to match.
     */
    if ((k->wk_flags & IEEE80211_KEY_SWCRYPT) == 0) {
        KASSERT(cip->ic_cipher < N(ciphermap),
            ("invalid cipher type %u", cip->ic_cipher));
        hk.kv_type = ciphermap[cip->ic_cipher];
        hk.kv_len = k->wk_keylen;
        memcpy(hk.kv_val, k->wk_key, k->wk_keylen);
    } else
        hk.kv_type = HAL_CIPHER_CLR;

    if ((k->wk_flags & IEEE80211_KEY_GROUP) && sc->sc_mcastkey) {
        /*
         * Group keys on hardware that supports multicast frame
         * key search use a mac that is the sender's address with
         * the high bit set instead of the app-specified address.
         */
        IEEE80211_ADDR_COPY(gmac, bss->ni_macaddr);
        gmac[0] |= 0x80;
        mac = gmac;
    } else
        mac = mac0;

    if (hk.kv_type == HAL_CIPHER_TKIP &&
        (k->wk_flags & IEEE80211_KEY_SWMIC) == 0 &&
        sc->sc_splitmic) {
        return ath_keyset_tkip(sc, k, &hk, mac);
    } else {
        KEYPRINTF(sc, k->wk_keyix, &hk, mac);
        return ath_hal_keyset(ah, k->wk_keyix, &hk, mac);
    }
#undef N
}

/*
 * Allocate tx/rx key slots for TKIP.  We allocate two slots for
 * each key, one for decrypt/encrypt and the other for the MIC.
 */
static u_int16_t
key_alloc_2pair(struct ath_softc *sc)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    u_int i, keyix;

    KASSERT(sc->sc_splitmic, ("key cache !split"));
    /* XXX could optimize */
    for (i = 0; i < N(sc->sc_keymap)/4; i++) {
        u_int8_t b = sc->sc_keymap[i];
        if (b != 0xff) {
            /*
             * One or more slots in this byte are free.
             */
            keyix = i*NBBY;
            while (b & 1) {
        again:
                keyix++;
                b >>= 1;
            }
            /* XXX IEEE80211_KEY_XMIT | IEEE80211_KEY_RECV */
            if (isset(sc->sc_keymap, keyix+32) ||
                isset(sc->sc_keymap, keyix+64) ||
                isset(sc->sc_keymap, keyix+32+64)) {
                /* full pair unavailable */
                /* XXX statistic */
                if (keyix == (i+1)*NBBY) {
                    /* no slots were appropriate, advance */
                    continue;
                }
                goto again;
            }
            setbit(sc->sc_keymap, keyix);
            setbit(sc->sc_keymap, keyix+64);
            setbit(sc->sc_keymap, keyix+32);
            setbit(sc->sc_keymap, keyix+32+64);
            DPRINTF(sc, ATH_DEBUG_KEYCACHE,
                "%s: key pair %u,%u %u,%u\n",
                __func__, keyix, keyix+64,
                keyix+32, keyix+32+64);
            return keyix;
        }
    }
    DPRINTF(sc, ATH_DEBUG_KEYCACHE, "%s: out of pair space\n", __func__);
    return IEEE80211_KEYIX_NONE;
#undef N
}

/*
 * Allocate a single key cache slot.
 */
static u_int16_t
key_alloc_single(struct ath_softc *sc)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    u_int i, keyix;

    /* XXX try i,i+32,i+64,i+32+64 to minimize key pair conflicts */
    for (i = 0; i < N(sc->sc_keymap); i++) {
        u_int8_t b = sc->sc_keymap[i];
        if (b != 0xff) {
            /*
             * One or more slots are free.
             */
            keyix = i*NBBY;
            while (b & 1)
                keyix++, b >>= 1;
            setbit(sc->sc_keymap, keyix);
            DPRINTF(sc, ATH_DEBUG_KEYCACHE, "%s: key %u\n",
                __func__, keyix);
            return keyix;
        }
    }
    DPRINTF(sc, ATH_DEBUG_KEYCACHE, "%s: out of space\n", __func__);
    return IEEE80211_KEYIX_NONE;
#undef N
}

/*
 * Allocate one or more key cache slots for a uniacst key.  The
 * key itself is needed only to identify the cipher.  For hardware
 * TKIP with split cipher+MIC keys we allocate two key cache slot
 * pairs so that we can setup separate TX and RX MIC keys.  Note
 * that the MIC key for a TKIP key at slot i is assumed by the
 * hardware to be at slot i+64.  This limits TKIP keys to the first
 * 64 entries.
 */
static int
ath_key_alloc(struct ieee80211vap *vap, const struct ieee80211_key *k)
{
    struct net_device *dev = vap->iv_ic->ic_dev;
    struct ath_softc *sc = dev->priv;

    sc->sc_stats.ast_keyalloc ++;

    /*
     * Group key allocation must be handled specially for
     * parts that do not support multicast key cache search
     * functionality.  For those parts the key id must match
     * the h/w key index so lookups find the right key.  On
     * parts w/ the key search facility we install the sender's
     * mac address (with the high bit set) and let the hardware
     * find the key w/o using the key id.  This is preferred as
     * it permits us to support multiple users for adhoc and/or
     * multi-station operation.
     */
    if ((k->wk_flags & IEEE80211_KEY_GROUP) && !sc->sc_mcastkey) {
        u_int keyix;

        if (!(&vap->iv_nw_keys[0] <= k &&
              k < &vap->iv_nw_keys[IEEE80211_WEP_NKID])) {
            /* should not happen */
            DPRINTF(sc, ATH_DEBUG_KEYCACHE,
                "%s: bogus group key\n", __func__);
            return IEEE80211_KEYIX_NONE;
        }
        keyix = k - vap->iv_nw_keys;
        /*
         * XXX we pre-allocate the global keys so
         * have no way to check if they've already been allocated.
         */
        return keyix;
    }
    /*
     * We allocate two pair for TKIP when using the h/w to do
     * the MIC.  For everything else, including software crypto,
     * we allocate a single entry.  Note that s/w crypto requires
     * a pass-through slot on the 5211 and 5212.  The 5210 does
     * not support pass-through cache entries and we map all
     * those requests to slot 0.
     *
     * Allocate 1 pair of keys for WEP case. Make sure the key
     * is not a shared-key.
     */
    if (k->wk_flags & IEEE80211_KEY_SWCRYPT) {
        return key_alloc_single(sc);
    } else if (k->wk_cipher->ic_cipher == IEEE80211_CIPHER_TKIP &&
        (k->wk_flags & IEEE80211_KEY_SWMIC) == 0 && sc->sc_splitmic) {
        return key_alloc_2pair(sc);
    }
    else {
        return key_alloc_single(sc);
    }
}

/*
 * Delete an entry in the key cache allocated by ath_key_alloc.
 */
static int
ath_key_delete(struct ieee80211vap *vap, const struct ieee80211_key *k,
                struct ieee80211_node *ninfo)
{
    struct net_device *dev = vap->iv_ic->ic_dev;
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    const struct ieee80211_cipher *cip = k->wk_cipher;
    struct ieee80211_node *ni;
    u_int keyix = k->wk_keyix;
    int   rxkeyoff = 0;

    sc->sc_stats.ast_keydelete ++;

    DPRINTF(sc, ATH_DEBUG_KEYCACHE, "%s: delete key %u\n", __func__, keyix);

    ath_hal_keyreset(ah, keyix);
    /*
     * Check the key->node map and flush any ref.
     */
    ni = sc->sc_keyixmap[keyix];
    if (ni != NULL) {
        ieee80211_free_node(ni);
        sc->sc_keyixmap[keyix] = NULL;
    }
    /*
     * Handle split tx/rx keying required for TKIP with h/w MIC.
     */
    if (cip->ic_cipher == IEEE80211_CIPHER_TKIP &&
        (k->wk_flags & IEEE80211_KEY_SWMIC) == 0 && sc->sc_splitmic) {
        ath_hal_keyreset(ah, keyix+32);     /* RX key */
        ni = sc->sc_keyixmap[keyix+32];
        if (ni != NULL) {           /* as above... */
            ieee80211_free_node(ni);
            sc->sc_keyixmap[keyix+32] = NULL;
        }
    }

    /* Remove receive key entry if one exists for static WEP case */
    if (ninfo != NULL) {
        rxkeyoff = ninfo->ni_rxkeyoff;
        if (rxkeyoff != 0) {
            ninfo->ni_rxkeyoff = 0;
            ath_hal_keyreset(ah, keyix + rxkeyoff);
            ni = sc->sc_keyixmap[keyix+rxkeyoff];
            if (ni != NULL) {   /* as above... */
                ieee80211_free_node(ni);
                sc->sc_keyixmap[keyix+rxkeyoff] = NULL;
            }
        }
    }

    if (keyix >= IEEE80211_WEP_NKID) {
        /*
         * Don't touch keymap entries for global keys so
         * they are never considered for dynamic allocation.
         */
        clrbit(sc->sc_keymap, keyix);
        if (cip->ic_cipher == IEEE80211_CIPHER_TKIP &&
            (k->wk_flags & IEEE80211_KEY_SWMIC) == 0 &&
            sc->sc_splitmic) {
            clrbit(sc->sc_keymap, keyix+64);    /* TX key MIC */
            clrbit(sc->sc_keymap, keyix+32);    /* RX key */
            clrbit(sc->sc_keymap, keyix+32+64); /* RX key MIC */
        }

        if (rxkeyoff != 0) {
            clrbit(sc->sc_keymap, keyix+rxkeyoff);/*RX Key */
        }
    }
    return 1;
}

/*
 * Set the key cache contents for the specified key.  Key cache
 * slot(s) must already have been allocated by ath_key_alloc.
 */
static int
ath_key_set(struct ieee80211vap *vap, const struct ieee80211_key *k,
    const u_int8_t mac[IEEE80211_ADDR_LEN])
{
    struct net_device *dev = vap->iv_ic->ic_dev;
    struct ath_softc *sc = dev->priv;

    return ath_keyset(sc, k, mac, vap->iv_bss);
}

/*
 * Block/unblock tx+rx processing while a key change is done.
 * We assume the caller serializes key management operations
 * so we only need to worry about synchronization with other
 * uses that originate in the driver.
 */
static void
ath_key_update_begin(struct ieee80211vap *vap)
{
    struct net_device *dev = vap->iv_ic->ic_dev;
    struct ath_softc *sc = dev->priv;

    DPRINTF(sc, ATH_DEBUG_KEYCACHE, "%s:\n", __func__);
    /*
     * When called from the rx tasklet we cannot use
     * tasklet_disable because it will block waiting
     * for us to complete execution.
     *
     * XXX Using in_softirq is not right since we might
     * be called from other soft irq contexts than
     * ath_rx_tasklet.
     */
    if (!in_softirq())
        tasklet_disable(&sc->sc_rxtq);
    netif_stop_queue(dev);
}

static void
ath_key_update_end(struct ieee80211vap *vap)
{
    struct net_device *dev = vap->iv_ic->ic_dev;
    struct ath_softc *sc = dev->priv;

    DPRINTF(sc, ATH_DEBUG_KEYCACHE, "%s:\n", __func__);
    netif_start_queue(dev);
    if (!in_softirq())      /* NB: see above */
        tasklet_enable(&sc->sc_rxtq);
}

/*
 * Calculate the receive filter according to the
 * operating mode and state:
 *
 * o always accept unicast, broadcast, and multicast traffic
 * o maintain current state of phy error reception (the hal
 *   may enable phy error frames for noise immunity work)
 * o probe request frames are accepted only when operating in
 *   hostap, adhoc, or monitor modes
 * o enable promiscuous mode according to the interface state
 * o accept beacons:
 *   - when operating in adhoc mode so the 802.11 layer creates
 *     node table entries for peers,
 *   - when operating in station mode for collecting rssi data when
 *     the station is otherwise quiet, or
 *   - when operating as a repeater so we see repeater-sta beacons
 *   - when scanning
 *   - when operating in hostap mode and protection mode is turned on
 */
static u_int32_t
ath_calcrxfilter(struct ath_softc *sc)
{
#define RX_FILTER_PRESERVE  (HAL_RX_FILTER_PHYERR | HAL_RX_FILTER_PHYRADAR)
    struct ieee80211com *ic = &sc->sc_ic;
    struct net_device *dev = ic->ic_dev;
    struct ath_hal *ah = sc->sc_ah;
    u_int32_t rfilt;

    rfilt = (ath_hal_getrxfilter(ah) & RX_FILTER_PRESERVE)
          | HAL_RX_FILTER_UCAST | HAL_RX_FILTER_BCAST | HAL_RX_FILTER_MCAST;
    if (ic->ic_opmode != IEEE80211_M_STA)
        rfilt |= HAL_RX_FILTER_PROBEREQ;
    if (ic->ic_opmode != IEEE80211_M_HOSTAP && (dev->flags & IFF_PROMISC))
        rfilt |= HAL_RX_FILTER_PROM;
    if (ic->ic_opmode == IEEE80211_M_STA ||
        ic->ic_opmode == IEEE80211_M_IBSS ||
        (sc->sc_nostabeacons) || sc->sc_scanning)
        rfilt |= HAL_RX_FILTER_BEACON;
    if ((ic->ic_opmode == IEEE80211_M_HOSTAP) && 
	(ic->ic_protmode != IEEE80211_PROT_NONE)) {
	rfilt |= HAL_RX_FILTER_BEACON;
    }
    return rfilt;
#undef RX_FILTER_PRESERVE
}

/*
 * Merge multicast addresses from all vap's to form the
 * hardware filter.  Ideally we should only inspect our
 * own list and the 802.11 layer would merge for us but
 * that's a bit difficult so for now we put the onus on
 * the driver.
 */
static void
ath_merge_mcast(struct ath_softc *sc, u_int32_t mfilt[2])
{
    struct ieee80211com *ic = &sc->sc_ic;
    struct ieee80211vap *vap;
    struct dev_mc_list *mc;
    u_int32_t val;
    u_int8_t pos;

    mfilt[0] = mfilt[1] = 0;
    /* XXX locking */
    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        struct net_device *dev = vap->iv_dev;
        for (mc = dev->mc_list; mc; mc = mc->next) {
            /* calculate XOR of eight 6bit values */
            val = LE_READ_4(mc->dmi_addr + 0);
            pos = (val >> 18) ^ (val >> 12) ^ (val >> 6) ^ val;
            val = LE_READ_4(mc->dmi_addr + 3);
            pos ^= (val >> 18) ^ (val >> 12) ^ (val >> 6) ^ val;
            pos &= 0x3f;
            mfilt[pos / 32] |= (1 << (pos % 32));
        }
    }
}

static void
ath_mode_init(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    u_int32_t rfilt, mfilt[2];

    /* configure rx filter */
    rfilt = ath_calcrxfilter(sc);
    ath_hal_setrxfilter(ah, rfilt);

    /* configure bssid mask */
    if (sc->sc_hasbmask)
        ath_hal_setbssidmask(ah, sc->sc_bssidmask);

    /* configure operational mode */
    ath_hal_setopmode(ah);

    /* calculate and install multicast filter */
    if ((dev->flags & IFF_ALLMULTI) == 0) {
        ath_merge_mcast(sc, mfilt);
    } else {
        mfilt[0] = mfilt[1] = ~0;
    }
    ath_hal_setmcastfilter(ah, mfilt[0], mfilt[1]);
    DPRINTF(sc, ATH_DEBUG_STATE,
         "%s: RX filter 0x%x, MC filter %08x:%08x\n",
         __func__, rfilt, mfilt[0], mfilt[1]);
}

/*
 * Set the slot time based on the current setting.
 */
static void
ath_setslottime(struct ath_softc *sc)
{
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;

    if (ic->ic_flags & IEEE80211_F_SHSLOT)
        ath_hal_setslottime(ah, HAL_SLOT_TIME_9);
    else
        ath_hal_setslottime(ah, HAL_SLOT_TIME_20);
    sc->sc_updateslot = OK;
}

/*
 * Callback from the 802.11 layer to update the
 * slot time based on the current setting.
 */
static void
ath_updateslot(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;

    /*
     * When not coordinating the BSS, change the hardware
     * immediately.  For other operation we defer the change
     * until beacon updates have propagated to the stations.
     */
    if (ic->ic_opmode == IEEE80211_M_HOSTAP)
        sc->sc_updateslot = UPDATE;
    else if (dev->flags & IFF_RUNNING)
        ath_setslottime(sc);
}

#ifdef ATH_SUPERG_DYNTURBO
/*
 * Dynamic turbo support.
 * XXX much of this could be moved up to the net80211 layer.
 */

/*
 * Configure dynamic turbo state on beacon setup.
 */
static void
ath_beacon_dturbo_config(struct ieee80211vap *vap, u_int32_t intval)
{
#define IS_CAPABLE(vap) \
    (vap->iv_bss && (vap->iv_bss->ni_ath_flags & (IEEE80211_ATHC_TURBOP )) == \
        (IEEE80211_ATHC_TURBOP))
    struct ieee80211com *ic = vap->iv_ic;
    struct ath_softc *sc = ic->ic_dev->priv;

    if (ic->ic_opmode == IEEE80211_M_HOSTAP && IS_CAPABLE(vap) ){

        /* Dynamic Turbo is supported on this channel. */
        sc->sc_dturbo = 1;
        sc->sc_dturbo_tcount = 0;
        sc->sc_dturbo_switch = 0;
        sc->sc_ignore_ar = 0;

        /* Set the initial ATHC_BOOST capability. */
        if (ic->ic_bsschan->ic_flags & CHANNEL_TURBO)
            ic->ic_ath_cap |=  IEEE80211_ATHC_BOOST;
        else
            ic->ic_ath_cap &= ~IEEE80211_ATHC_BOOST;

        /*
         * Calculate time & bandwidth thresholds
         *
         * sc_dturbo_base_tmin  :  ~70 seconds
         * sc_dturbo_turbo_tmax : ~120 seconds
         *
         * NB: scale calculated values to account for staggered
         *     beacon handling
         */
        sc->sc_dturbo_base_tmin  = 70  * 1024 / ic->ic_lintval;
        sc->sc_dturbo_turbo_tmax = 120 * 1024 / ic->ic_lintval;
        sc->sc_dturbo_bw_base    = 0;  /* XXX: TBD */
        sc->sc_dturbo_bw_turbo   = 0;  /* XXX: TBD */
    } else {
        sc->sc_dturbo = 0;
        ic->ic_ath_cap &= ~IEEE80211_ATHC_BOOST;
    }
#undef IS_CAPABLE
}

/*
 * Update dynamic turbo state at SWBA.  We assume we care
 * called only if dynamic turbo has been enabled (sc_turbo).
 */
static void
ath_beacon_dturbo_update(struct ieee80211vap *vap, int *needmark)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ath_softc *sc = ic->ic_dev->priv;
    u_int32_t bss_traffic;

    /* TBD: Age out CHANNEL_INTERFERENCE */
    if (sc->sc_ignore_ar) {
        /*
         * Ignore AR for this beacon; a dynamic turbo
         * switch just happened and the information
         * is invalid.  Notify AR support of the channel
         * change.
         */
        sc->sc_ignore_ar = 0;
        ath_hal_ar_enable(sc->sc_ah);
    }
    sc->sc_dturbo_tcount++;
    /*
     * Calculate BSS traffic over the previous interval.
     */
    bss_traffic = (sc->sc_devstats.tx_bytes + sc->sc_devstats.rx_bytes)
            - sc->sc_dturbo_bytes;
    sc->sc_dturbo_bytes = sc->sc_devstats.tx_bytes
                + sc->sc_devstats.rx_bytes;
    if (ic->ic_ath_cap & IEEE80211_ATHC_BOOST) {
        /*
         * Current Mode: Turbo (i.e. BOOST)
         *
         * Transition to base occurs when one of the following
         * is true:
         *    1. Maximum time in BOOST has elapsed (120 secs).
         *    2. Channel is marked with interference
         *    3. Average BSS traffic falls below 4Mbps (TBD)
         *    4. RSSI cannot support at least 18 Mbps rate (TBD)
         * XXX do bw checks at true beacon interval?
         */
         if (sc->sc_dturbo_tcount >= sc->sc_dturbo_turbo_tmax ||
             ((vap->iv_bss->ni_ath_flags & IEEE80211_ATHC_AR) &&
               (sc->sc_curchan.channelFlags & CHANNEL_INTERFERENCE)   &&
                IEEE80211_IS_CHAN_2GHZ(ic->ic_curchan))          ||
             bss_traffic < sc->sc_dturbo_bw_base) {
             DPRINTF(sc, ATH_DEBUG_TURBO, "%s: Leaving turbo\n",
                 sc->sc_dev->name);
             ic->ic_ath_cap &= ~IEEE80211_ATHC_BOOST;
             vap->iv_bss->ni_ath_flags &= ~IEEE80211_ATHC_BOOST;
             sc->sc_dturbo_tcount = 0;
             sc->sc_dturbo_switch = 1;
        }
    } else {
        /*
         * Current Mode: BASE
         *
         * Transition to Turbo (i.e. BOOST) when all of the
         * following are true:
         *
         * 1. Dwell time at base has exceeded minimum (70 secs)
         * 2. Only DT-capable stations are associated
         * 3. Channel supports TURBO (always true if we are here)
         * 4. Channel is marked interference-free.
         * 5. BSS data traffic averages at least 6Mbps (TBD)
         * 6. RSSI is good enough to support 36Mbps (TBD)
         * XXX do bw+rssi checks at true beacon interval?
         */
        if (sc->sc_dturbo_tcount >= sc->sc_dturbo_base_tmin &&
            (ic->ic_dt_sta_assoc != 0 &&
             ic->ic_sta_assoc == ic->ic_dt_sta_assoc) &&
            ((vap->iv_bss->ni_ath_flags & IEEE80211_ATHC_AR) == 0 ||
             (sc->sc_curchan.privFlags & CHANNEL_INTERFERENCE) == 0) &&
            bss_traffic >= sc->sc_dturbo_bw_turbo) {
             DPRINTF(sc, ATH_DEBUG_TURBO, "%s: Entering turbo\n",
                 sc->sc_dev->name);
            ic->ic_ath_cap |= IEEE80211_ATHC_BOOST;
            vap->iv_bss->ni_ath_flags |= IEEE80211_ATHC_BOOST;
            sc->sc_dturbo_tcount = 0;
            sc->sc_dturbo_switch = 1;
        }
    }
}


static int
ath_check_beacon_done(struct ath_softc *sc)
{
    struct ieee80211vap *vap=NULL;
    struct ath_vap *avp;
    struct ath_buf *bf;
    struct sk_buff *skb;
    struct ath_desc *ds;
    struct ath_hal *ah = sc->sc_ah;
    int slot;

    /*
     * check if the last beacon went out with the mode change flag set.
     */
    for (slot = 0; slot < ATH_BCBUF; slot++) {
        if(sc->sc_bslot[slot]) {
            vap = sc->sc_bslot[slot];
            break;
        }
    }
    if (!vap)
         return 0;
    avp = ATH_VAP(vap);
    bf = avp->av_bcbuf;
    skb = bf->bf_skb;
    ds = bf->bf_desc;

    return(ath_hal_txprocdesc(ah, ds) != HAL_EINPROGRESS);

}

/*
 * Effect a turbo mode switch when operating in dynamic
 * turbo mode.wait for beacon to go out before switching.
 */
static void
ath_turbo_switch_mode(unsigned long data)
{
    struct net_device  *dev = (struct net_device *)data;
    struct ath_softc    *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    int newflags;

    KASSERT(ic->ic_opmode == IEEE80211_M_HOSTAP,
        ("unexpected operating mode %d", ic->ic_opmode));

    DPRINTF(sc, ATH_DEBUG_STATE, "%s: dynamic turbo switch to %s mode\n",
        dev->name,
        ic->ic_ath_cap & IEEE80211_ATHC_BOOST ? "turbo" : "base");

    if(!ath_check_beacon_done(sc)) {
        /*
         * beacon did not go out. reschedule tasklet.
         */
        mod_timer(&sc->sc_dturbo_switch_mode,
                      jiffies + ((HZ * 2) / 1000));
        return;
    }

    /* TBD: DTIM adjustments, delay CAB queue tx until after transmit */

    newflags = ic->ic_bsschan->ic_flags;
    if (ic->ic_ath_cap & IEEE80211_ATHC_BOOST) {
        if (IEEE80211_IS_CHAN_2GHZ(ic->ic_bsschan)) {
                        /*
                         * Ignore AR next beacon. the AR detection
                         * code detects the traffic in normal channel
                         * from stations during transition delays
                         * between AP and station.
                         */
                        sc->sc_ignore_ar = 1;
            ath_hal_ar_disable(sc->sc_ah);
        }
        newflags |= IEEE80211_CHAN_TURBO;
    } else
        newflags &= ~IEEE80211_CHAN_TURBO;
    ieee80211_dturbo_switch(ic, newflags);
    /*
     * XXX: patch up. without the following code
     *      some times after mode switch SWBA interrupts
     *      are turned off completely.
     *      need to fix the source of the problem and
     *      remove this patch up.
     */
    if (sc->sc_beacons)
        ath_beacon_config(sc, NULL);    /* restart beacons */
    /* XXX ieee80211_reset_erp? */
}
#endif /* ATH_SUPERG_DYNTURBO */

/* Swap transmit descriptor.
 * if AH_NEED_DESC_SWAP flag is not defined this becomes a "null"
 * function.
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
 * Setup a h/w transmit queue for beacons.
 */
static int
ath_beaconq_setup(struct ath_hal *ah)
{
    HAL_TXQ_INFO qi;

    memset(&qi, 0, sizeof(qi));
    qi.tqi_aifs = HAL_TXQ_USEDEFAULT;
    qi.tqi_cwmin = HAL_TXQ_USEDEFAULT;
    qi.tqi_cwmax = HAL_TXQ_USEDEFAULT;
#ifdef ATH_SUPERG_DYNTURBO
    qi.tqi_qflags = TXQ_FLAG_TXDESCINT_ENABLE;
#endif
    /* NB: don't enable any interrupts */
    return ath_hal_setuptxqueue(ah, HAL_TX_QUEUE_BEACON, &qi);
}

/*
 * Configure IFS parameter for the beacon queue.
 */
static int
ath_beaconq_config(struct ath_softc *sc)
{
#define ATH_EXPONENT_TO_VALUE(v)    ((1<<v)-1)
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;
    HAL_TXQ_INFO qi;

    ath_hal_gettxqueueprops(ah, sc->sc_bhalq, &qi);
    if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
        /*
         * Always burst out beacon and CAB traffic.
         */
        qi.tqi_aifs = ATH_BEACON_AIFS_DEFAULT;
        qi.tqi_cwmin = ATH_BEACON_CWMIN_DEFAULT;
        qi.tqi_cwmax = ATH_BEACON_CWMAX_DEFAULT;
    } else {
        struct wmeParams *wmep =
            &ic->ic_wme.wme_chanParams.cap_wmeParams[WME_AC_BE];
        /*
         * Adhoc mode; important thing is to use 2x cwmin.
         */
        qi.tqi_aifs = wmep->wmep_aifsn;
        qi.tqi_cwmin = 2*ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmin);
        qi.tqi_cwmax = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmax);
    }

    if (!ath_hal_settxqueueprops(ah, sc->sc_bhalq, &qi)) {
        printk("%s: unable to update h/w beacon queue parameters\n",
            sc->sc_dev->name);
        return 0;
    } else {
        ath_hal_resettxqueue(ah, sc->sc_bhalq); /* push to h/w */
        return 1;
    }
#undef ATH_EXPONENT_TO_VALUE
}

/*
 * Allocate and setup an initial beacon frame.
 */
static int
ath_beacon_alloc(struct ath_softc *sc, struct ieee80211_node *ni)
{
    struct ath_vap *avp = ATH_VAP(ni->ni_vap);
    struct ieee80211_frame *wh;
    struct ath_buf *bf;
    struct sk_buff *skb;
    uint64_t tsfadjust;     /* staggered beacon adjust */

    /*
    Release the previous beacon skbuf and node, if they already exist.
    */
    bf = avp->av_bcbuf;
    if (bf->bf_skb != NULL) {
        bus_unmap_single(sc->sc_bdev,
            bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);
        dev_kfree_skb(bf->bf_skb);
        bf->bf_skb = NULL;
    }
    if (bf->bf_node != NULL) {
        ieee80211_free_node(bf->bf_node);
        bf->bf_node = NULL;
    }




    /*
     * NB: the beacon data buffer must be 32-bit aligned;
     * we assume the mbuf routines will return us something
     * with this alignment (perhaps should assert).
     */
    skb = ieee80211_beacon_alloc(ni, &avp->av_boff);
    if (skb == NULL) {
        DPRINTF(sc, ATH_DEBUG_BEACON, "%s: cannot get sk_buff\n",
            __func__);
        sc->sc_stats.ast_be_nobuf++;
        return ENOMEM;
    }

    /*
     * Calculate an TSF adjustment factor required for
     * staggered beacons.  Note that we assume the format
     * of the beacon frame leaves the tstamp field immediately
     * following the header.
     */
    if (sc->sc_hastsfadd && avp->av_bslot > 0) {
        /* NB: tsf adjustment suitable for hardware */
        tsfadjust = cpu_to_le64(((uint64_t)(ni->ni_intval *
            (ATH_BCBUF - avp->av_bslot - 1) / ATH_BCBUF)) << 10);
    } else
        tsfadjust = 0;
    wh = (struct ieee80211_frame *) skb->data;
    memcpy(&wh[1], &tsfadjust, sizeof(tsfadjust));

    bf = avp->av_bcbuf;
    bf->bf_node = ieee80211_ref_node(ni);
    bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
        skb->data, skb->len, BUS_DMA_TODEVICE);
    bf->bf_skb = skb;

    return 0;
}

/*
 * Setup the beacon frame for transmit.
 */
static void
ath_beacon_setup(struct ath_softc *sc, struct ath_buf *bf)
{
#define USE_SHPREAMBLE(_ic) \
    (((_ic)->ic_flags & (IEEE80211_F_SHPREAMBLE | IEEE80211_F_USEBARKER))\
        == IEEE80211_F_SHPREAMBLE)
    struct ieee80211_node *ni = bf->bf_node;
    struct ieee80211com *ic = ni->ni_ic;
    struct sk_buff *skb = bf->bf_skb;
    struct ath_hal *ah = sc->sc_ah;
    struct ath_desc *ds;
    int flags, antenna;
    const HAL_RATE_TABLE *rt;
    u_int8_t rix, rate;
    int ctsrate=0;
    int ctsduration=0;
    HAL_11N_RATE_SERIES  series[4] = {{ 0 }};

    DPRINTF(sc, ATH_DEBUG_BEACON, "%s: m %p len %u\n",
        __func__, skb, skb->len);

    /* setup descriptors */
    ds = bf->bf_desc;

    flags = HAL_TXDESC_NOACK;
#ifdef ATH_SUPERG_DYNTURBO
        if (sc->sc_dturbo_switch)
                flags |= HAL_TXDESC_INTREQ;
#endif

    if (ic->ic_opmode == IEEE80211_M_IBSS && sc->sc_hasveol) {
        ds->ds_link = bf->bf_daddr; /* self-linked */
        flags |= HAL_TXDESC_VEOL;
        /*
         * Let hardware handle antenna switching.
         */
        antenna = 0;
    } else {
        ds->ds_link = 0;
        /*
         * Switch antenna every 4 beacons.
         * XXX assumes two antenna
         */
        antenna = (sc->sc_stats.ast_be_xmit & 4 ? 2 : 1);
    }

    ds->ds_data = bf->bf_skbaddr;
    /*
     * Calculate rate code.
     * XXX everything at min xmit rate
     */
    rix = sc->sc_minrateix;
    rt = sc->sc_currates;
    rate = rt->info[rix].rateCode;
    if (USE_SHPREAMBLE(ic))
        rate |= rt->info[rix].shortPreamble;
#ifdef ATH_SUPERG_XR
    if(bf->bf_node->ni_vap->iv_flags & IEEE80211_F_XR) {
        u_int8_t cix;
        int pktlen;
        pktlen = skb->len + IEEE80211_CRC_LEN;
        cix = rt->info[sc->sc_protrix].controlRate;
        /* for XR VAP use different RTSCTS rates and calculate duration */
        ctsrate = rt->info[cix].rateCode;
        if (USE_SHPREAMBLE(ic))
            ctsrate |= rt->info[cix].shortPreamble;
        flags |= HAL_TXDESC_CTSENA;
        rt = sc->sc_xr_rates;
        ctsduration = ath_hal_computetxtime(ah,rt, pktlen, IEEE80211_XR_DEFAULT_RATE_INDEX, AH_FALSE);
        rate = rt->info[IEEE80211_XR_DEFAULT_RATE_INDEX].rateCode;
    }
#endif

#ifdef DEBUG_PKTLOG
        ds->ds_vdata = (u_int32_t)skb->data;
#endif

    ath_hal_setuptxdesc(ah, ds
        , skb->len + IEEE80211_CRC_LEN  /* frame length */
        , sizeof(struct ieee80211_frame)/* header length */
        , HAL_PKT_TYPE_BEACON       /* Atheros packet type */
        , ni->ni_txpower        /* txpower XXX */
        , rate, 1           /* series 0 rate/tries */
        , HAL_TXKEYIX_INVALID       /* no encryption */
        , antenna           /* antenna mode */
        , flags             /* no ack, veol for beacons */
        , ctsrate       /* rts/cts rate */
        , ctsduration           /* rts/cts duration */
        , 0             /* comp icv len */
        , 0             /* comp iv len */
        , ATH_COMP_PROC_NO_COMP_NO_CCS  /* comp scheme */
    );

    /* NB: beacon's BufLen must be a multiple of 4 bytes */
    ath_hal_filltxdesc(ah, ds
        , roundup(skb->len, 4)      /* buffer length */
        , AH_TRUE           /* first segment */
        , AH_TRUE           /* last segment */
        , ds                /* first descriptor */
    );

    series[0].Tries = 1;
    series[0].Rate = rate;
    series[0].ChSel = sc->sc_ic.ic_tx_chainmask;
    series[0].RateFlags = (ctsrate) ? HAL_RATESERIES_RTS_CTS : 0;
    ath_hal_set11n_ratescenario(ah, ds, 0, ctsrate, series, 4, 0);

    /* NB: The desc swap function becomes void,
     * if descriptor swapping is not enabled
     */
    ath_desc_swap(ds);
#undef USE_SHPREAMBLE
}

/*
 * Since we turn off sequence number in the h/w
 * and data seq no. is taken care by the data xmit; for
 * beacons, we need to update the seq no. 
 */

static inline void 
ath_beacon_sequence_update(struct sk_buff *skb) 
{
    /* 
     * Beacon Seq number is 16 bits with 12 bits as number and 4 
     * bits for fragment control. 
     */
    struct ieee80211_frame  *wh;
    static u_int16_t seq = 0;
    wh = (struct ieee80211_frame *)skb->data;
    *(u_int16_t *)wh->i_seq = htole16(seq << IEEE80211_SEQ_SEQ_SHIFT);  
    /* max 4095 (12 bits ! ) */
    seq = (seq < 4095) ? seq+1:0; 
}

/*
 * Transmit a beacon frame at SWBA.  Dynamic updates to the
 * frame contents are done as needed and the slot time is
 * also adjusted based on current state.
 */
static void
ath_beacon_tasklet(struct ath_softc *sc, int *needmark)
{
    struct ath_hal *ah = sc->sc_ah;
    struct ath_buf *bf;
    struct ieee80211_node *ni;
    struct ieee80211vap *vap;
    struct ath_vap *avp;
    struct sk_buff *skb;
    int ncabq, otherant;
    unsigned int curlen;
    u_int32_t rx_clear, rx_frame, tx_frame;
    u_int32_t show_cycles = 0;
    struct ieee80211com *ic = &sc->sc_ic;

    vap = sc->sc_bslot[sc->sc_bnext];
    DPRINTF(sc, ATH_DEBUG_BEACON_PROC, "%s: bnext %d vap %p\n",
        __func__, sc->sc_bnext, vap);
    sc->sc_bnext = (sc->sc_bnext + 1) % ATH_BCBUF;

#ifdef ATH_SUPERG_XR
    if (vap && (vap->iv_flags & IEEE80211_F_XR)) {
        vap->iv_xrbcnwait++;
        /* wait for XR_BEACON_FACTOR times before sending the beacon */
        if (vap->iv_xrbcnwait < IEEE80211_XR_BEACON_FACTOR) return;
        vap->iv_xrbcnwait=0;
    }
#endif

    /*
     * Check if the previous beacon has gone out.  If
     * not don't try to post another, skip this period
     * and wait for the next.  Missed beacons indicate
     * a problem and should not occur.  If we miss too
     * many consecutive beacons reset the device.
     */
    if (ath_noreset) {
        show_cycles = ath_hal_getCycleCounts(ah, 
                      &rx_clear, &rx_frame, &tx_frame);
    }

    if (ath_hal_numtxpending(ah, sc->sc_bhalq) != 0) {
        sc->sc_bmisscount++;

        /* XXX: doth needs the chanchange IE countdown decremented.
         *      We should consider adding a net80211 call to indicate
         *      a beacon miss so appropriate action could be taken
         *      (in that layer).
         */
        if (sc->sc_bmisscount < BSTUCK_THRESH) {
            if (ath_noreset) {
                printk("%s: missed %u consecutive beacons\n",
                       __func__, sc->sc_bmisscount);
                if (show_cycles) {
                    /*
                     * Display cycle counter stats from HAL to
                     * aide in debug of stickiness.
                     */
                    printk("%s: busy times: rx_clear=%d, rx_frame=%d, tx_frame=%d\n",
                           __func__, rx_clear, rx_frame, tx_frame);
                } else {
                    printk("%s: unable to obtain busy times\n", __func__);
                }
            } else {
                DPRINTF(sc, ATH_DEBUG_BEACON_PROC,
                        "%s: missed %u consecutive beacons\n",
                        __func__, sc->sc_bmisscount);
            }
        } else if (sc->sc_bmisscount == BSTUCK_THRESH) {
            if (ath_noreset) {
                printk("%s: beacon is officially stuck\n",
                       __func__);
                ath_hal_dmaRegDump(ah);
            } else {
                DPRINTF(sc, ATH_DEBUG_BEACON_PROC,
                        "%s: beacon is officially stuck\n",
                        __func__);
                ATH_SCHEDULE_TQUEUE(&sc->sc_bstucktq, needmark);
            }
        }

        return;
    }

    if (ic->ic_flags_ext & IEEE80211_C_RESET) {
        DPRINTF(sc, ATH_DEBUG_BEACON_PROC, "%s: forcing beacon stuck\n", 
                __func__);
        ATH_SCHEDULE_TQUEUE(&sc->sc_bstucktq, needmark);
        return;
    }

    if (sc->sc_bmisscount != 0) {
        if (ath_noreset) {
            printk("%s: resume beacon xmit after %u misses\n",
                   __func__, sc->sc_bmisscount);
        } else {
            DPRINTF(sc, ATH_DEBUG_BEACON_PROC,
                    "%s: resume beacon xmit after %u misses\n",
                    __func__, sc->sc_bmisscount);
        }
        sc->sc_bmisscount = 0;
    }
    
    /*
     * Slot may be unassigned or the virtual ap may not be running.
     */
    if (vap == NULL) {
        DPRINTF(sc, ATH_DEBUG_BEACON, "%s: skip empty slot\n",
            __func__);
        sc->sc_bmisscount = 0;
        return;
    }
    if (vap->iv_state != IEEE80211_S_RUN) {
        DPRINTF(sc, ATH_DEBUG_BEACON, "%s: skip vap in %s state\n",
            __func__, ieee80211_state_name[vap->iv_state]);
        sc->sc_bmisscount = 0;
        return;
    }

    avp = ATH_VAP(vap);
    if (avp == NULL || avp->av_bcbuf == NULL) {
        DPRINTF(sc, ATH_DEBUG_ANY, "%s: avp=%p av_bcbuf=%p\n",
             __func__, avp, avp != NULL ? avp->av_bcbuf : NULL);
        sc->sc_bmisscount = 0;
        return;
    }
    bf = avp->av_bcbuf;
    ni = bf->bf_node;

#ifdef ATH_SUPERG_DYNTURBO
    /*
     * If we are using dynamic turbo, update the
     * capability info and arrange for a mode change
     * if needed.
     */
    if (sc->sc_dturbo)
        ath_beacon_dturbo_update(vap, needmark);
#endif
    /*
     * Update dynamic beacon contents.  If this returns
     * non-zero then we need to remap the memory because
     * the beacon frame changed size (probably because
     * of the TIM bitmap).
     */
    skb = bf->bf_skb;
    curlen = skb->len;
    ncabq = avp->av_mcastq.axq_depth;
    if (ieee80211_beacon_update(ni, &avp->av_boff, skb, ncabq)) {
        bus_unmap_single(sc->sc_bdev,
            bf->bf_skbaddr, curlen, BUS_DMA_TODEVICE);
        bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
            skb->data, skb->len, BUS_DMA_TODEVICE);
    }

    /* Update the beacon sequence number  */
    ath_beacon_sequence_update(skb);

    /*
     * if the CABQ traffic from previous DTIM is pending and the current
     *  beacon is also a DTIM.
     *  1) if there is only one vap let the cab traffic continue.
     *  2) if there are more than one vap drain the cabq by
     *     dropping all the frames in the cabq so that the
     *     current vaps cab traffic can be scheduled.
     */
    if(ncabq && (avp->av_boff.bo_tim[4] & 1) && sc->sc_cabq->axq_depth) {
        if(sc->sc_nvaps > 1) {
            ath_tx_draintxq(sc,sc->sc_cabq);
        }
    }

    if (sc->sc_bmisscount != 0) {
        DPRINTF(sc, ATH_DEBUG_BEACON,
            "%s: resume beacon xmit after %u misses\n",
            __func__, sc->sc_bmisscount);
        sc->sc_bmisscount = 0;
    }

    /*
     * Handle slot time change when a non-ERP station joins/leaves
     * an 11g network.  The 802.11 layer notifies us via callback,
     * we mark updateslot, then wait one beacon before effecting
     * the change.  This gives associated stations at least one
     * beacon interval to note the state change.
     */
    /* XXX locking */
    if (sc->sc_updateslot == UPDATE)
        sc->sc_updateslot = COMMIT; /* commit next beacon */
    else if (sc->sc_updateslot == COMMIT)
        ath_setslottime(sc);        /* commit change to h/w */

    /*
     * Check recent per-antenna transmit statistics and flip
     * the default antenna if noticeably more frames went out
     * on the non-default antenna.
     * XXX assumes 2 anntenae
     */
    otherant = sc->sc_defant & 1 ? 2 : 1;
    if (sc->sc_ant_tx[otherant] > sc->sc_ant_tx[sc->sc_defant] + 2)
        ath_setdefantenna(sc, otherant);
    sc->sc_ant_tx[1] = sc->sc_ant_tx[2] = 0;

    /*
     * Construct tx descriptor.
     */
    ath_beacon_setup(sc, bf);

    /*
     * Stop any current dma and put the new frame on the queue.
     * This should never fail since we check above that no frames
     * are still pending on the queue.
     */
    if (!ath_hal_stoptxdma(ah, sc->sc_bhalq)) {
        DPRINTF(sc, ATH_DEBUG_ANY,
            "%s: beacon queue %u did not stop?\n",
            __func__, sc->sc_bhalq);
        /* NB: the HAL still stops DMA, so proceed */
    }
    bus_dma_sync_single(sc->sc_bdev,
        bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);

    /*
     * Enable the CAB queue before the beacon queue to
     * insure cab frames are triggered by this beacon.
     */
    if (avp->av_boff.bo_tim[4] & 1) {   /* NB: only at DTIM */
        struct ath_buf *bfmcast;
        /*
         * move all the list of buffers from the vap mcast queue
         * to the cab queue
         */
        ATH_TXQ_LOCK(&avp->av_mcastq);
        ATH_TXQ_LOCK(sc->sc_cabq);
        bfmcast = TAILQ_FIRST(&(avp->av_mcastq.axq_q));
        /* link the descriptors */
        if(sc->sc_cabq->axq_link == NULL) {
            ath_hal_puttxbuf(ah, sc->sc_cabq->axq_qnum, bfmcast->bf_daddr);
        } else {
#ifdef AH_NEED_DESC_SWAP
            *(sc->sc_cabq->axq_link) =
                    cpu_to_le32(bfmcast->bf_daddr);
#else
            *(sc->sc_cabq->axq_link) = bfmcast->bf_daddr;
#endif
        }
        /* append the private vap mcast list to  the cabq */
        ATH_TXQ_MOVE_MCASTQ(&(avp->av_mcastq),sc->sc_cabq);
        ath_hal_txstart(ah, sc->sc_cabq->axq_qnum);
        ATH_TXQ_UNLOCK(sc->sc_cabq);
        ATH_TXQ_UNLOCK(&avp->av_mcastq);
    }
#ifdef DEBUG_PKTLOG
        {
            struct log_tx log_data;
            log_data.firstds = bf->bf_desc;
            log_data.bf = bf;
            ath_log_txctl(sc, &log_data, 0);
            log_data.lastds = bf->bf_desc;
            ath_log_txstatus(sc, &log_data, 0);
        }
#endif
    ath_hal_puttxbuf(ah, sc->sc_bhalq, bf->bf_daddr);
    ath_hal_txstart(ah, sc->sc_bhalq);
    DPRINTF(sc, ATH_DEBUG_BEACON_PROC, "%s: TXDP%u = %p (%p)\n", __func__,
        sc->sc_bhalq, (caddr_t)bf->bf_daddr, bf->bf_desc);

    sc->sc_stats.ast_be_xmit++;     /* XXX per-vap? */
}

/*
 * Reset the hardware after detecting beacons have stopped.
 */
static void
ath_bstuck_tasklet(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;

    sc->sc_stats.ast_bstuck ++;

    /*
     * XXX:if the bmisscount is cleared while the
     *     tasklet execution is pending, the following
     *     check for bmisscount will be true , in which case return
     *     without resetting the driver.
     */
    if (ic->ic_flags_ext & IEEE80211_C_RESET) {
        ic->ic_flags_ext &= ~IEEE80211_C_RESET;
    } else if (sc->sc_bmisscount <= BSTUCK_THRESH) {
        return;
    }

    printk("%s: stuck beacon; resetting (bmiss count %u)\n",
           dev->name, sc->sc_bmisscount);
    ath_resetinternal(dev, RESET_FATAL);
}

/*
 * Startup beacon transmission for adhoc mode when
 * they are sent entirely by the hardware using the
 * self-linked descriptor + veol trick.
 */
static void
ath_beacon_start_adhoc(struct ath_softc *sc, struct ieee80211vap *vap)
{
    struct ath_hal *ah = sc->sc_ah;
    struct ath_buf *bf;
    struct ieee80211_node *ni;
    struct ath_vap *avp;
    struct sk_buff *skb;

    avp = ATH_VAP(vap);
    if (avp == NULL || avp->av_bcbuf == NULL) {
        DPRINTF(sc, ATH_DEBUG_ANY, "%s: avp=%p av_bcbuf=%p\n",
             __func__, avp, avp != NULL ? avp->av_bcbuf : NULL);
        return;
    }
    bf = avp->av_bcbuf;
    ni = bf->bf_node;

    /*
     * Update dynamic beacon contents.  If this returns
     * non-zero then we need to remap the memory because
     * the beacon frame changed size (probably because
     * of the TIM bitmap).
     */
    skb = bf->bf_skb;
    if (ieee80211_beacon_update(ni, &avp->av_boff, skb, 0)) {
        bus_unmap_single(sc->sc_bdev,
            bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);
        bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
            skb->data, skb->len, BUS_DMA_TODEVICE);
    }

    /*
     * Construct tx descriptor.
     */
    ath_beacon_setup(sc, bf);

    bus_dma_sync_single(sc->sc_bdev,
        bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);

    /* NB: caller is known to have already stopped tx dma */
    ath_hal_puttxbuf(ah, sc->sc_bhalq, bf->bf_daddr);
    ath_hal_txstart(ah, sc->sc_bhalq);
    DPRINTF(sc, ATH_DEBUG_BEACON_PROC, "%s: TXDP%u = %p (%p)\n", __func__,
        sc->sc_bhalq, (caddr_t)bf->bf_daddr, bf->bf_desc);
}

/*
 * Reclaim beacon resources and return buffer to the pool.
 */
static void
ath_beacon_return(struct ath_softc *sc, struct ath_buf *bf)
{
    if (bf->bf_skb != NULL) {
        bus_unmap_single(sc->sc_bdev,
            bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);
        dev_kfree_skb(bf->bf_skb);
        bf->bf_skb = NULL;
    }
    if (bf->bf_node != NULL) {
        ieee80211_free_node(bf->bf_node);
        bf->bf_node = NULL;
    }
    TAILQ_INSERT_TAIL(&sc->sc_bbuf, bf, bf_list);
}

/*
 * Reclaim all beacon resources.
 */
static void
ath_beacon_free(struct ath_softc *sc)
{
    struct ath_buf *bf;

    TAILQ_FOREACH(bf, &sc->sc_bbuf, bf_list) {
        if (bf->bf_skb != NULL) {
            bus_unmap_single(sc->sc_bdev,
                bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);
            dev_kfree_skb(bf->bf_skb);
            bf->bf_skb = NULL;
        }
        if (bf->bf_node != NULL) {
            ieee80211_free_node(bf->bf_node);
            bf->bf_node = NULL;
        }
    }
}

/*
 * Configure the beacon and sleep timers.
 *
 * When operating as an AP this resets the TSF and sets
 * up the hardware to notify us when we need to issue beacons.
 *
 * When operating in station mode this sets up the beacon
 * timers according to the timestamp of the last received
 * beacon and the current TSF, configures PCF and DTIM
 * handling, programs the sleep registers so the hardware
 * will wakeup in time to receive beacons, and configures
 * the beacon miss handling so we'll receive a BMISS
 * interrupt when we stop seeing beacons from the AP
 * we've associated with.
 */
static void
ath_beacon_config(struct ath_softc *sc, struct ieee80211vap *vap)
{
#define TSF_TO_TU(_h,_l)    (((_h) << 22) | ((_l) >> 10))
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;
    struct ieee80211_node *ni;
    u_int32_t nexttbtt, intval;

    if (vap == NULL) {
        vap = TAILQ_FIRST(&ic->ic_vaps);   /* XXX */
    }

    ni = vap->iv_bss;

    /* extract tstamp from last beacon and convert to TU */
    nexttbtt = TSF_TO_TU(LE_READ_4(ni->ni_tstamp.data + 4),
                 LE_READ_4(ni->ni_tstamp.data));
    /* XXX conditionalize multi-bss support? */
    if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
        /*
         * For multi-bss ap support beacons are staggered
         * evenly over N slots so arrange for the SWBA to
         * be delivered for each slot.  Slots that are not
         * occupied will generate nothing.  This increases
         * the interrupt load but is preferred to batching
         * the beacons as that has too much jitter.
         */
        /* NB: the beacon interval is kept internally in TU's */
        intval = ic->ic_lintval & HAL_BEACON_PERIOD;
        intval /= ATH_BCBUF;    /* for staggered beacons */
        if ((sc->sc_nostabeacons) &&
            (vap->iv_opmode == IEEE80211_M_HOSTAP))
            nexttbtt = 0;
    } else {
        intval = ni->ni_intval & HAL_BEACON_PERIOD;
    }
    if (nexttbtt == 0)      /* e.g. for ap mode */
        nexttbtt = intval;
    else if (intval)        /* NB: can be 0 for monitor mode */
        nexttbtt = roundup(nexttbtt, intval);
    DPRINTF(sc, ATH_DEBUG_BEACON, "%s: nexttbtt %u intval %u (%u)\n",
        __func__, nexttbtt, intval, ni->ni_intval);
    if ((ic->ic_opmode == IEEE80211_M_STA) ||
        ((ic->ic_opmode == IEEE80211_M_HOSTAP) &&
         (vap->iv_opmode == IEEE80211_M_STA) &&
         (sc->sc_nostabeacons))) {
        HAL_BEACON_STATE bs;
        u_int64_t tsf;
        u_int32_t tsftu;
        int dtimperiod, dtimcount;
        int cfpperiod, cfpcount;

        /*
         * Setup dtim and cfp parameters according to
         * last beacon we received (which may be none).
         */
        dtimperiod = vap->iv_dtim_period;
        if (dtimperiod <= 0)        /* NB: 0 if not known */
            dtimperiod = 1;
        dtimcount = vap->iv_dtim_count;
        if (dtimcount >= dtimperiod)    /* NB: sanity check */
            dtimcount = 0;      /* XXX? */
        cfpperiod = 1;          /* NB: no PCF support yet */
        cfpcount = 0;
#define FUDGE   2
        /*
         * Pull nexttbtt forward to reflect the current
         * TSF and calculate dtim+cfp state for the result.
         */
        tsf = ath_hal_gettsf64(ah);
        tsftu = TSF_TO_TU((u_int32_t)(tsf>>32), (u_int32_t)tsf) + FUDGE;
        do {
            nexttbtt += intval;
            if (--dtimcount < 0) {
                dtimcount = dtimperiod - 1;
                if (--cfpcount < 0)
                    cfpcount = cfpperiod - 1;
            }
        } while (nexttbtt < tsftu);
#undef FUDGE
        memset(&bs, 0, sizeof(bs));
        bs.bs_intval = intval;
        bs.bs_nexttbtt = nexttbtt;
        bs.bs_dtimperiod = dtimperiod*intval;
        bs.bs_nextdtim = bs.bs_nexttbtt + dtimcount*intval;
        bs.bs_cfpperiod = cfpperiod*bs.bs_dtimperiod;
        bs.bs_cfpnext = bs.bs_nextdtim + cfpcount*bs.bs_dtimperiod;
        bs.bs_cfpmaxduration = 0;
#if 0
        /*
         * The 802.11 layer records the offset to the DTIM
         * bitmap while receiving beacons; use it here to
         * enable h/w detection of our AID being marked in
         * the bitmap vector (to indicate frames for us are
         * pending at the AP).
         * XXX do DTIM handling in s/w to WAR old h/w bugs
         * XXX enable based on h/w rev for newer chips
         */
        bs.bs_timoffset = ni->ni_timoff;
#endif
        /*
         * Calculate the number of consecutive beacons to miss
         * before taking a BMISS interrupt.  The configuration
         * is specified in TU so we only need calculate based
         * on the beacon interval.  Note that we clamp the
         * result to at most 10 beacons.
         */
        bs.bs_bmissthreshold = howmany(ic->ic_bmisstimeout, intval);
        if (bs.bs_bmissthreshold > 10)
            bs.bs_bmissthreshold = 10;
        else if (bs.bs_bmissthreshold <= 0)
            bs.bs_bmissthreshold = 1;

        /*
         * Calculate sleep duration.  The configuration is
         * given in ms.  We insure a multiple of the beacon
         * period is used.  Also, if the sleep duration is
         * greater than the DTIM period then it makes senses
         * to make it a multiple of that.
         *
         * XXX fixed at 100ms
         */
        bs.bs_sleepduration =
            roundup(IEEE80211_MS_TO_TU(100), bs.bs_intval);
        if (bs.bs_sleepduration > bs.bs_dtimperiod)
            bs.bs_sleepduration = roundup(bs.bs_sleepduration, bs.bs_dtimperiod);

        DPRINTF(sc, ATH_DEBUG_BEACON,
            "%s: tsf %llu tsf:tu %u intval %u nexttbtt %u dtim %u nextdtim %u bmiss %u sleep %u cfp:period %u maxdur %u next %u timoffset %u\n"
            , __func__
            , tsf, tsftu
            , bs.bs_intval
            , bs.bs_nexttbtt
            , bs.bs_dtimperiod
            , bs.bs_nextdtim
            , bs.bs_bmissthreshold
            , bs.bs_sleepduration
            , bs.bs_cfpperiod
            , bs.bs_cfpmaxduration
            , bs.bs_cfpnext
            , bs.bs_timoffset
        );
        if (!(sc->sc_nostabeacons)) {
            ath_hal_intrset(ah, 0);
            ath_hal_beacontimers(ah, &bs);
            sc->sc_imask |= HAL_INT_BMISS;
            ath_hal_intrset(ah, sc->sc_imask);
        }
    } else {
        ath_hal_intrset(ah, 0);
        if (nexttbtt == intval)
            intval |= HAL_BEACON_RESET_TSF;
        if (ic->ic_opmode == IEEE80211_M_IBSS) {
            /*
             * In IBSS mode enable the beacon timers but only
             * enable SWBA interrupts if we need to manually
             * prepare beacon frames.  Otherwise we use a
             * self-linked tx descriptor and let the hardware
             * deal with things.
             */
            intval |= HAL_BEACON_ENA;
            if (!sc->sc_hasveol)
                sc->sc_imask |= HAL_INT_SWBA;
            ath_beaconq_config(sc);
        } else if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
            /*
             * In AP mode we enable the beacon timers and
             * SWBA interrupts to prepare beacon frames.
             */
            intval |= HAL_BEACON_ENA;
            sc->sc_imask |= HAL_INT_SWBA;   /* beacon prepare */
            ath_beaconq_config(sc);
        }
#ifdef ATH_SUPERG_DYNTURBO
        ath_beacon_dturbo_config(vap, intval &
                ~(HAL_BEACON_RESET_TSF | HAL_BEACON_ENA));
#endif
        ath_hal_beaconinit(ah, nexttbtt, intval);
        sc->sc_bmisscount = 0;
        ath_hal_intrset(ah, sc->sc_imask);
        /*
         * When using a self-linked beacon descriptor in
         * ibss mode load it once here.
         */
        if (ic->ic_opmode == IEEE80211_M_IBSS && sc->sc_hasveol)
            ath_beacon_start_adhoc(sc, vap);
    }
    sc->sc_syncbeacon = 0;
#undef TSF_TO_TU
}

static int
ath_descdma_setup(struct ath_softc *sc,
    struct ath_descdma *dd, ath_bufhead *head,
    const char *name, int nbuf, int ndesc)
{
#define DS2PHYS(_dd, _ds) \
    ((_dd)->dd_desc_paddr + ((caddr_t)(_ds) - (caddr_t)(_dd)->dd_desc))
    struct ath_desc *ds;
    struct ath_buf *bf;
    int i, bsize, error;

    DPRINTF(sc, ATH_DEBUG_RESET, "%s: %s DMA: %u buffers %u desc/buf\n",
        __func__, name, nbuf, ndesc);

    dd->dd_name = name;
    dd->dd_desc_len = sizeof(struct ath_desc) * nbuf * ndesc;

    /* allocate descriptors */
    dd->dd_desc = bus_alloc_consistent(sc->sc_bdev,
                dd->dd_desc_len, &dd->dd_desc_paddr);
    if (dd->dd_desc == NULL) {
        error = -ENOMEM;
        goto fail;
    }
    ds = dd->dd_desc;
    DPRINTF(sc, ATH_DEBUG_RESET, "%s: %s DMA map: %p (%lu) -> %p (%lu)\n",
        __func__, dd->dd_name, ds, (u_long) dd->dd_desc_len,
        (caddr_t) dd->dd_desc_paddr, /*XXX*/ (u_long) dd->dd_desc_len);

    /* allocate buffers */
    bsize = sizeof(struct ath_buf) * nbuf;
    bf = kmalloc(bsize, GFP_KERNEL);
    if (bf == NULL) {
        error = -ENOMEM;        /* XXX different code */
        goto fail2;
    }
    memset(bf, 0, bsize);
    dd->dd_bufptr = bf;

    TAILQ_INIT(head);
    for (i = 0; i < nbuf; i++, bf++, ds += ndesc) {
        int j;

        bf->bf_descno = 0;
        bf->bf_desc = bf->bf_descarr = bf->bf_lastds = ds;
        for (j = 0; j < ndesc; j++)
            bf->bf_daddrs[j] = DS2PHYS(dd, (ds + j));
        bf->bf_daddr = bf->bf_daddrs[0];

        TAILQ_INSERT_TAIL(head, bf, bf_list);
    }
    return 0;
fail2:
    bus_free_consistent(sc->sc_bdev, dd->dd_desc_len,
        dd->dd_desc, dd->dd_desc_paddr);
fail:
    memset(dd, 0, sizeof(*dd));
    return error;
#undef DS2PHYS
}

static void
ath_descdma_cleanup(struct ath_softc *sc,
    struct ath_descdma *dd, ath_bufhead *head, int dir)
{
    struct ath_buf *bf;
    struct ieee80211_node *ni;

    TAILQ_FOREACH(bf, head, bf_list) {
        if (bf->bf_skb != NULL) {
            /* XXX is skb->len good enough? */
            bus_unmap_single(sc->sc_bdev,
                bf->bf_skbaddr, bf->bf_skb->len, dir);
            dev_kfree_skb(bf->bf_skb);
            bf->bf_skb = NULL;
        }
        ni = bf->bf_node;
        bf->bf_node = NULL;
        if (ni != NULL) {
            /*
             * Reclaim node reference.
             */
            ieee80211_free_node(ni);
        }
    }

    /* Free memory associated with descriptors */
    bus_free_consistent(sc->sc_bdev, dd->dd_desc_len,
        dd->dd_desc, dd->dd_desc_paddr);

    TAILQ_INIT(head);
    kfree(dd->dd_bufptr);
    memset(dd, 0, sizeof(*dd));
}

static int
ath_desc_alloc(struct ath_softc *sc)
{
    int error;

    error = ath_descdma_setup(sc, &sc->sc_rxdma, &sc->sc_rxbuf,
            "rx", ATH_RXBUF, 1);
    if (error != 0)
        return error;

    error = ath_descdma_setup(sc, &sc->sc_txdma, &sc->sc_txbuf,
            "tx", ATH_TXBUF, ATH_TXDESC);
    if (error != 0) {
        ath_descdma_cleanup(sc, &sc->sc_rxdma, &sc->sc_rxbuf,
            BUS_DMA_FROMDEVICE);
        return error;
    }

    /* XXX allocate beacon state together with vap */
    error = ath_descdma_setup(sc, &sc->sc_bdma, &sc->sc_bbuf,
            "beacon", ATH_BCBUF, 1);
    if (error != 0) {
        ath_descdma_cleanup(sc, &sc->sc_txdma, &sc->sc_txbuf,
            BUS_DMA_TODEVICE);
        ath_descdma_cleanup(sc, &sc->sc_rxdma, &sc->sc_rxbuf,
            BUS_DMA_FROMDEVICE);
        return error;
    }
    return 0;
}

static void
ath_desc_free(struct ath_softc *sc)
{
    if (sc->sc_bdma.dd_desc_len != 0)
        ath_descdma_cleanup(sc, &sc->sc_bdma, &sc->sc_bbuf,
            BUS_DMA_TODEVICE);
    if (sc->sc_txdma.dd_desc_len != 0)
        ath_descdma_cleanup(sc, &sc->sc_txdma, &sc->sc_txbuf,
            BUS_DMA_TODEVICE);
    if (sc->sc_rxdma.dd_desc_len != 0)
        ath_descdma_cleanup(sc, &sc->sc_rxdma, &sc->sc_rxbuf,
            BUS_DMA_FROMDEVICE);
}

static struct ieee80211_node *
ath_node_alloc(struct ieee80211_node_table *nt,struct ieee80211vap *vap)
{
    struct ath_softc *sc = nt->nt_ic->ic_dev->priv;
    const size_t space = sizeof(struct ath_node) + sc->sc_rc->arc_space;
    struct ath_node *an;

    sc->sc_stats.ast_nodealloc ++;

    an = kmalloc(space, GFP_ATOMIC);
    if (an == NULL)
        return NULL;
    memset(an, 0, space);
    an->an_decomp_index = INVALID_DECOMP_INDEX;
    an->an_avgrssi = ATH_RSSI_DUMMY_MARKER;
    an->an_halstats.ns_avgbrssi = ATH_RSSI_DUMMY_MARKER;
    an->an_halstats.ns_avgrssi = ATH_RSSI_DUMMY_MARKER;
    an->an_halstats.ns_avgtxrssi = ATH_RSSI_DUMMY_MARKER;
    /* ath rate_node_init needs a vap pointer in node to decide which mgt rate to use */
    an->an_node.ni_vap = vap;
    ath_rate_node_init(sc, an);

    /*
     * initialize default functions
     */
    an->an_rx_input = ieee80211_input;
    an->an_isap     = (sc->sc_ic.ic_opmode == IEEE80211_M_HOSTAP);

    owl_node_init(sc, an);

    DPRINTF(sc, ATH_DEBUG_NODE, "%s: an %p\n", __func__, an);
    return &an->an_node;
}

static void
ath_node_free(struct ieee80211_node *ni)
{
    struct ath_softc *sc = ni->ni_ic->ic_dev->priv;

    sc->sc_stats.ast_nodefree ++;

    ath_rate_node_cleanup(sc, ATH_NODE(ni));
    sc->sc_node_free(ni);
#ifdef ATH_SUPERG_XR
    ath_grppoll_period_update(sc);
#endif
}

static u_int8_t
ath_node_getrssi(const struct ieee80211_node *ni)
{
#define HAL_EP_RND(x, mul) \
    ((((x)%(mul)) >= ((mul)/2)) ? ((x) + ((mul) - 1)) / (mul) : (x)/(mul))
    u_int32_t avgrssi = ATH_NODE_CONST(ni)->an_avgrssi;
    int32_t rssi;

    /*
     * When only one frame is received there will be no state in
     * avgrssi so fallback on the value recorded by the 802.11 layer.
     */
    if (avgrssi != ATH_RSSI_DUMMY_MARKER)
        rssi = HAL_EP_RND(avgrssi, HAL_RSSI_EP_MULTIPLIER);
    else
        rssi = ni->ni_rssi;
    /* NB: theoretically we shouldn't need this, but be paranoid */
    return rssi < 0 ? 0 : rssi > 127 ? 127 : rssi;
#undef HAL_EP_RND
}


#if defined(ATH_SUPERG_XR)
/*
 * stops the txqs and moves data between XR and Normal queues.
 * also adjusts the rate info in the descriptors .
 */

static u_int8_t
ath_node_move_data(const struct ieee80211_node *ni)
{
#ifdef NOT_YET
    struct ath_txq      *txq=NULL;
    struct ieee80211com *ic = ni->ni_ic;
    struct ath_softc *sc = ic->ic_dev->priv;
    struct ath_buf *bf,*prev,*bf_tmp,*bf_tmp1;
    struct ath_hal *ah = sc->sc_ah;
    struct sk_buff *skb=NULL;
    struct ath_desc *ds;
    HAL_STATUS status;
    int index;

    if(ni->ni_vap->iv_flags & IEEE80211_F_XR) {
        struct ath_txq      tmp_q;
        memset(&tmp_q, 0, sizeof(tmp_q));
        TAILQ_INIT(&tmp_q.axq_q);
        /*
         * move data from Normal txqs to XR queue.
         */
        printk("move data from Normal to XR \n");
        /*
         * collect all the data towards the node
         * in to the tmp_q.
         */
        index = WME_AC_VO;
        while(index >= WME_AC_BE && txq != sc->sc_ac2q[index]) {
            txq = sc->sc_ac2q[index];
            ATH_TXQ_LOCK(txq);
            ath_hal_stoptxdma(ah, txq->axq_qnum);
            bf = prev = TAILQ_FIRST(&txq->axq_q);
            /*
             * skip all the buffers that are done .
             * until the first one that is in progress
             */
            while(bf) {
#ifdef ATH_SUPERG_FF
                ds = &bf->bf_desc[bf->bf_numdesc - 1];
#else
                ds = bf->bf_desc;       /* NB: last decriptor */
#endif
                status = ath_hal_txprocdesc(ah, ds);
                if (status == HAL_EINPROGRESS) break;
                prev = bf;
                bf = TAILQ_NEXT(bf,bf_list);
            }
            /*
             * save the pointer to the last buf thats
             * done .
             */
            if(prev == bf) {
                bf_tmp = NULL;
            } else {
                bf_tmp=prev;
            }
            while(bf) {
                if(ni == bf->bf_node) {
                    if(prev == bf) {
                        ATH_TXQ_REMOVE_HEAD(txq, bf_list);
                        TAILQ_INSERT_TAIL(&tmp_q.axq_q,bf,bf_list);
                        bf =  TAILQ_FIRST(&txq->axq_q);
                        prev = bf;
                    } else {
                        TAILQ_REMOVE_AFTER(&(txq->axq_q),prev,bf_list);
                        txq->axq_depth--;
                        TAILQ_INSERT_TAIL(&tmp_q.axq_q, bf, bf_list);
                        bf = TAILQ_NEXT(prev,bf_list);
                        /*
                         * after deleting the node.
                         * link the descriptors .
                         */
#ifdef ATH_SUPERG_FF
                        ds = &prev->bf_desc[prev->bf_numdesc - 1];
#else
                        ds = prev->bf_desc;     /* NB: last decriptor */
#endif
#ifdef AH_NEED_DESC_SWAP
                        ds->ds_link = cpu_to_le32(bf->bf_daddr);
#else
                        ds->ds_link = bf->bf_daddr;
#endif
                    }
                } else {
                    prev = bf;
                    bf = TAILQ_NEXT(bf,bf_list);
                }
            }
            /*
             * if the last buf was deleted.
             * set the pointer to the last descriptor.
             */
            bf = TAILQ_FIRST(&txq->axq_q);
            if(bf) {
                if(prev) {
                    bf = TAILQ_NEXT(prev,bf_list);
                    if(!bf) { /* prev is the last one on the list */
#ifdef ATH_SUPERG_FF
                        ds = &prev->bf_desc[prev->bf_numdesc - 1];
#else
                        ds = prev->bf_desc;     /* NB: last decriptor */
#endif
                        status = ath_hal_txprocdesc(ah, ds);
                        if (status == HAL_EINPROGRESS)
                            txq->axq_link = &ds->ds_link;
                        else
                            txq->axq_link = NULL;

                    }
                }
            } else {
                    txq->axq_link = NULL;
            }
            ATH_TXQ_UNLOCK(txq);
            /*
             * restart the DMA from the first
             * buffer that was not DMAd.
             */
            if(bf_tmp) {
                bf = TAILQ_NEXT(bf_tmp,bf_list);
            } else {
                bf = TAILQ_FIRST(&txq->axq_q);
            }
            if(bf) {
                ath_hal_puttxbuf(ah, txq->axq_qnum, bf->bf_daddr);
                ath_hal_txstart(ah, txq->axq_qnum);
            }
        }
        /*
         * queue them on to the XR txqueue.
         * can not directly put them on to the XR txq. since the
         * skb data size may be greater than the XR fragmentation
         * threshold size.
         */
        bf  = TAILQ_FIRST(&tmp_q.axq_q);
        index=0;
        while(bf) {
            skb = bf->bf_skb;
            bf->bf_skb = NULL;
            bf->bf_node = NULL;
            ATH_TXBUF_LOCK(sc);
            TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf, bf_list);
            ATH_TXBUF_UNLOCK(sc);
            ath_hardstart(skb,sc->sc_dev);
            ATH_TXQ_REMOVE_HEAD(&tmp_q, bf_list);
            bf  = TAILQ_FIRST(&tmp_q.axq_q);
            ++index;
        }
        printk("moved %d buffers from Normal to XR \n",index);
    } else {
        struct ath_txq      wme_tmp_qs[WME_AC_VO+1];
        struct ath_txq      *wmeq=NULL,*prevq;
        struct ieee80211_frame *wh;
        struct ath_desc *ds=NULL;
        int count=0;

        /*
         * move data from XR txq to Normal txqs.
         */
        printk("move buffers from  XR to NORMAL \n");
        memset(&wme_tmp_qs, 0, sizeof(wme_tmp_qs));
        for(index=0;index<=WME_AC_VO;++index) {
            TAILQ_INIT(&wme_tmp_qs[index].axq_q);
        }
        txq = sc->sc_xrtxq;
        ATH_TXQ_LOCK(txq);
        ath_hal_stoptxdma(ah, txq->axq_qnum);
        bf = prev = TAILQ_FIRST(&txq->axq_q);
        /*
         * skip all the buffers that are done .
         * until the first one that is in progress
         */
        while(bf) {
#ifdef ATH_SUPERG_FF
            ds = &bf->bf_desc[bf->bf_numdesc - 1];
#else
            ds = bf->bf_desc;       /* NB: last decriptor */
#endif
            status = ath_hal_txprocdesc(ah, ds);
            if (status == HAL_EINPROGRESS) break;
            prev= bf;
            bf = TAILQ_NEXT(bf,bf_list);
        }
        /*
         * save the pointer to the last buf thats
         * done .
         */
        if(prev == bf) {
            bf_tmp1 = NULL;
        } else {
            bf_tmp1=prev;
        }
        /*
         * colect all the data in to four temp SW queues.
         */
        while(bf) {
            if(ni == bf->bf_node) {
                if(prev == bf) {
                    TAILQ_REMOVE(&txq->axq_q,bf_list);
                    bf_tmp=bf;
                    bf =  TAILQ_FIRST(&txq->axq_q);
                    prev = bf;
                } else {
                    TAILQ_REMOVE_AFTER(&(txq->axq_q),prev,bf_list);
                    bf_tmp=bf;
                    bf = TAILQ_NEXT(prev,bf_list);
                }
                ++count;
                skb = bf_tmp->bf_skb;
                wh = (struct ieee80211_frame *) skb->data;
                if (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_QOS) {
                    /* XXX validate skb->priority, remove mask */
                    wmeq = &wme_tmp_qs[skb->priority & 0x3];
                } else {
                    wmeq = &wme_tmp_qs[WME_AC_BE];
                }
                TAILQ_INSERT_TAIL(&wmeq->axq_q, bf_tmp, bf_list);
                ds = bf_tmp->bf_desc;
                /*
                 * link the descriptors .
                 */
                if (wmeq->axq_link != NULL) {
#ifdef AH_NEED_DESC_SWAP
                    *wmeq->axq_link = cpu_to_le32(bf_tmp->bf_daddr);
#else
                    *wmeq->axq_link = bf_tmp->bf_daddr;
#endif
                    DPRINTF(sc, ATH_DEBUG_XMIT, "%s: link[%u](%p)=%p (%p)\n",
                            __func__,
                            wmeq->axq_qnum, wmeq->axq_link,
                            (caddr_t)bf_tmp->bf_daddr, bf_tmp->bf_desc);
                }
                wmeq->axq_link = &ds->ds_link;
                /*
                 * update the rate information
                 */
            } else {
                prev = bf;
                bf = TAILQ_NEXT(bf,bf_list);
            }
        }
        /*
         * reset the axq_link pointer to the last descriptor.
         */
        bf = TAILQ_FIRST(&txq->axq_q);
        if(bf) {
            if(prev) {
                bf = TAILQ_NEXT(prev,bf_list);
                if(!bf) { /* prev is the last one on the list */
#ifdef ATH_SUPERG_FF
                    ds = &prev->bf_desc[prev->bf_numdesc - 1];
#else
                    ds = prev->bf_desc;     /* NB: last decriptor */
#endif
                    status = ath_hal_txprocdesc(ah, ds);
                    if (status == HAL_EINPROGRESS)
                        txq->axq_link = &ds->ds_link;
                    else
                        txq->axq_link = NULL;
                }
            }
        } else {
            /*
             * if the list is empty reset the pointer.
             */
            txq->axq_link = NULL;
        }
        ATH_TXQ_UNLOCK(txq);
        /*
         * restart the DMA from the first
         * buffer that was not DMAd.
         */
        if(bf_tmp1) {
            bf = TAILQ_NEXT(bf_tmp1,bf_list);
        } else {
            bf = TAILQ_FIRST(&txq->axq_q);
        }
        if(bf) {
            ath_hal_puttxbuf(ah, txq->axq_qnum, bf->bf_daddr);
            ath_hal_txstart(ah, txq->axq_qnum);
        }

        /*
         * move (concant) the lists from the temp sw queues in to
         * wme queues.
         */
        index = WME_AC_VO;
        txq=NULL;
        while(index >= WME_AC_BE ) {
            prevq = txq;
            txq = sc->sc_ac2q[index];
            if(txq  != prevq) {
                ATH_TXQ_LOCK(txq);
                ath_hal_stoptxdma(ah, txq->axq_qnum);
            }

            wmeq = &wme_tmp_qs[index];
            bf =TAILQ_FIRST(&wmeq->axq_q);
            if(bf) {
                ATH_TXQ_MOVE_Q(wmeq,txq);
                if(txq->axq_link != NULL) {
#ifdef AH_NEED_DESC_SWAP
                    *(txq->axq_link) = cpu_to_le32(bf->bf_daddr);
#else
                    *(txq->axq_link) = bf->bf_daddr;
#endif
                }
            }
            if(index == WME_AC_BE || txq  != prevq ) {
                /*
                 * find the first buffer to be DMAd.
                 */
                bf =TAILQ_FIRST(&txq->axq_q);
                while(bf) {
#ifdef ATH_SUPERG_FF
                    ds = &bf->bf_desc[bf->bf_numdesc - 1];
#else
                    ds = bf->bf_desc;       /* NB: last decriptor */
#endif
                    status = ath_hal_txprocdesc(ah, ds);
                    if (status == HAL_EINPROGRESS) break;
                    bf = TAILQ_NEXT(bf,bf_list);
                }
                if(bf) {
                    ath_hal_puttxbuf(ah, txq->axq_qnum, bf->bf_daddr);
                    ath_hal_txstart(ah, txq->axq_qnum);
                }
                ATH_TXQ_UNLOCK(txq);
            }
            --index;
        }
        printk("moves %d buffers from  XR to NORMAL \n",count);
    }
#endif
    return 0;
}
#endif

static struct sk_buff *
ath_alloc_skb(u_int size, u_int align)
{
    struct sk_buff *skb;
    u_int off;

    skb = dev_alloc_skb(size + align-1);
    if (skb != NULL) {
        off = ((unsigned long) skb->data) % align;
        if (off != 0)
            skb_reserve(skb, align - off);
    }
    return skb;
}

static int
ath_rxbuf_init(struct ath_softc *sc, struct ath_buf *bf)
{
    struct ath_hal  *ah = sc->sc_ah;
    struct sk_buff  *skb;
    struct ath_desc *ds;
    struct ath_buf  *bf_held;

    /*
     * hold the current one and post the last one now
     */
    if (!sc->sc_rxbuf_held) {
        sc->sc_rxbuf_held = bf;
        return 0;
    }
    bf_held = sc->sc_rxbuf_held;
    sc->sc_rxbuf_held = bf;
    bf = bf_held;

    skb = bf->bf_skb;
    if (skb == NULL) {
        if (sc->sc_ic.ic_opmode == IEEE80211_M_MONITOR) {
            u_int off;
            /*
             * Allocate buffer for monitor mode with space for the
             * wlan-ng style physical layer header at the start.
             */
            skb = dev_alloc_skb(sc->sc_rxbufsize +
                        sizeof(wlan_ng_prism2_header) +
                        sc->sc_cachelsz - 1);
            if (skb == NULL) {
                DPRINTF(sc, ATH_DEBUG_ANY,
                    "%s: skbuff alloc of size %u failed\n",
                    __func__,
                    sc->sc_rxbufsize
                    + sizeof(wlan_ng_prism2_header)
                    + sc->sc_cachelsz -1);
                sc->sc_stats.ast_rx_nobuf++;
                return ENOMEM;
            }
            /*
             * Reserve space for the Prism header.
             */
            skb_reserve(skb, sizeof(wlan_ng_prism2_header));
            /*
             * Align to cache line.
             */
            off = ((unsigned long) skb->data) % sc->sc_cachelsz;
            if (off != 0)
                skb_reserve(skb, sc->sc_cachelsz - off);
        } else {
            /*
             * Cache-line-align.  This is important (for the
             * 5210 at least) as not doing so causes bogus data
             * in rx'd frames.
             */
            skb = ath_alloc_skb(sc->sc_rxbufsize, sc->sc_cachelsz);
            if (skb == NULL) {
                DPRINTF(sc, ATH_DEBUG_ANY,
                    "%s: skbuff alloc of size %u failed\n",
                    __func__, sc->sc_rxbufsize);
                sc->sc_stats.ast_rx_nobuf++;
                return ENOMEM;
            }
        }
        skb->dev = sc->sc_dev;
        bf->bf_skb = skb;
        bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
            skb->data, sc->sc_rxbufsize, BUS_DMA_FROMDEVICE);
    }

    /*
     * owl: we can no longer afford to self link the last descriptor.
     * MAC acknowledges BA status as long as it copies frames to host
     * buffer (or rx fifo). This can incorrectly acknowledge packets
     * to a sender if last desc is self-linked.
     */
    ds = bf->bf_desc;
    ds->ds_link = 0;     /* link to null */
    ds->ds_data = bf->bf_skbaddr;
    ds->ds_vdata = (u_int32_t) skb->data;   /* virt addr of buffer */
    ath_hal_setuprxdesc(ah, ds
        , skb_tailroom(skb)     /* buffer size */
        , 0
    );
    if (sc->sc_rxlink == NULL)
        ath_hal_putrxbuf(ah, bf->bf_daddr);
    else
        *sc->sc_rxlink = bf->bf_daddr;

    sc->sc_rxlink = &ds->ds_link;
    ath_hal_rxena(ah);
    return 0;
}

/*
 * Add a prism2 header to a received frame and
 * dispatch it to capture tools like kismet.
 */
static void
ath_rx_capture(struct net_device *dev, struct sk_buff *skb, 
               struct ath_rx_status *rx_stats)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    int len = rx_stats->rs_datalen;
    struct ieee80211_frame *wh;
    u_int32_t tsf;

    skb->protocol = ETH_P_CONTROL;
    skb->pkt_type = PACKET_OTHERHOST;
    skb_put(skb, len);

    KASSERT(ic->ic_flags & IEEE80211_F_DATAPAD,
        ("data padding not enabled?"));
    /* Remove pad bytes */
    wh = (struct ieee80211_frame *) skb->data;
    if (IEEE80211_QOS_HAS_SEQ(wh)) {
        int headersize = ieee80211_hdrsize(wh);
        int padbytes = roundup(headersize,4) - headersize;

        /*
         * Copy up 802.11 header and strip h/w padding.
         */
        if (padbytes > 0) {
            memmove(skb->data + padbytes, skb->data, headersize);
            skb_pull(skb, padbytes);
        }
    }

    /* Pass up tsf clock in mactime */
    /*
     * Rx descriptor has the low 15 bits of the tsf at
     * the time the frame was received.  Use the current
     * tsf to extend this to 32 bits.
     */
    tsf = ath_hal_gettsf32(sc->sc_ah);
    if ((tsf & 0x7fff) < rx_stats->rs_tstamp)
        tsf -= 0x8000;
    tsf = rx_stats->rs_tstamp | (tsf &~ 0x7fff);

    ieee80211_input_monitor(ic, skb, tsf,
        0, rx_stats->rs_rssi,
        (rx_stats->rs_rate & 0x80) ?
        sc->sc_hthwmap[rx_stats->rs_rate & 0x7F].ieeerate :
        sc->sc_hwmap[rx_stats->rs_rate].ieeerate);

    sc->sc_devstats.rx_packets++;
    sc->sc_devstats.rx_bytes += len;
}

/*
 * Extend 15-bit time stamp from rx descriptor to
 * a full 64-bit TSF using the current h/w TSF.
 */
static __inline u_int64_t
ath_extend_tsf(struct ath_hal *ah, u_int32_t rstamp)
{
    u_int64_t tsf;

    tsf = ath_hal_gettsf64(ah);
    if ((tsf & 0x7fff) < rstamp)
        tsf -= 0x8000;
    return ((tsf &~ 0x7fff) | rstamp);
}

/*
 * Intercept management frames to collect beacon rssi data
 * and to do ibss merges.
 */
static void
ath_recv_mgmt(struct ieee80211_node *ni, struct sk_buff *skb,
    int subtype, int rssi, u_int32_t rstamp)
{
    struct ath_softc *sc = ni->ni_ic->ic_dev->priv;
    struct ieee80211vap *vap = ni->ni_vap;

    /*
     * Call up first so subsequent work can use information
     * potentially stored in the node (e.g. for ibss merge).
     */
    sc->sc_recv_mgmt(ni, skb, subtype, rssi, rstamp);
    switch (subtype) {
    case IEEE80211_FC0_SUBTYPE_BEACON:
        /* update rssi statistics for use by the hal */
        ATH_RSSI_LPF(ATH_NODE(ni)->an_halstats.ns_avgbrssi, rssi);
        if (sc->sc_syncbeacon &&
            ni == vap->iv_bss && vap->iv_state == IEEE80211_S_RUN) {
            /*
             * Resync beacon timers using the tsf of the
             * beacon frame we just received.
             */
            ath_beacon_config(sc, vap);
        }

         /*
          * node's channel width changed?
          */
        if (ni->ni_newchwidth) {
            ni->ni_newchwidth = 0;
            ath_cwm_newchwidth(ni);
        }
        break;

        /* fall thru... */
    case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
        if (vap->iv_opmode == IEEE80211_M_IBSS &&
            vap->iv_state == IEEE80211_S_RUN) {
            u_int64_t tsf = ath_extend_tsf(sc->sc_ah, rstamp);
            /*
             * Handle ibss merge as needed; check the tsf on the
             * frame before attempting the merge.  The 802.11 spec
             * says the station should change it's bssid to match
             * the oldest station with the same ssid, where oldest
             * is determined by the tsf.  Note that hardware
             * reconfiguration happens through callback to
             * ath_newstate as the state machine will go from
             * RUN -> RUN when this happens.
             */
            if (le64_to_cpu(ni->ni_tstamp.tsf) >= tsf) {
                DPRINTF(sc, ATH_DEBUG_STATE,
                    "ibss merge, rstamp %u tsf %llu "
                    "tstamp %llu\n", rstamp, tsf,
                    ni->ni_tstamp.tsf);
                (void) ieee80211_ibss_merge(ni);
            }
        }
        break;
    case IEEE80211_FC0_SUBTYPE_ACTION:
         /* HT Action management frame (tx channel width)
          * node's channel width changed?
          */
        if (ni->ni_newchwidth) {
            ni->ni_newchwidth = 0;
            ath_cwm_newchwidth(ni);
        }
        break;
    case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
    case IEEE80211_FC0_SUBTYPE_REASSOC_REQ:
        /*
         * Clear addba state when assoc/reassoc is seen.
         */ 
        if (ATH_NODE_IS_11N(ATH_NODE(ni)))
            ath_aggr_addba_clear(ATH_NODE(ni));
        break;
    }
}

static void
ath_setdefantenna(struct ath_softc *sc, u_int antenna)
{
    struct ath_hal *ah = sc->sc_ah;

    /* XXX block beacon interrupts */
    ath_hal_setdefantenna(ah, antenna);
    if (sc->sc_defant != antenna)
        sc->sc_stats.ast_ant_defswitch++;
    sc->sc_defant = antenna;
    sc->sc_rxotherant = 0;
}

static void
ath_rx_tasklet(TQUEUE_ARG data)
{
#define PA2DESC(_sc, _pa) \
    ((struct ath_desc *)((caddr_t)(_sc)->sc_rxdma.dd_desc + \
        ((_pa) - (_sc)->sc_rxdma.dd_desc_paddr)))
    struct net_device *dev = (struct net_device *)data;
    struct ath_buf *bf;
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;
    struct ath_desc *ds;
    struct sk_buff *skb;
    struct ieee80211_node *ni;
    int len, type;
    u_int phyerr;
    int mic_fail = 0;
    HAL_STATUS status;
    struct ath_rx_status rx_stats = {0};
    unsigned long flags;
#ifdef DEBUG_PKTLOG
    u_int16_t   pktLogMiscCnt = 0;
#ifdef ATH_FORCE_PPM
    int32_t   pktlog_misc[8];
#endif
#endif

    DPRINTF(sc, ATH_DEBUG_RX_PROC, "%s\n", __func__);
    do {
        bf = TAILQ_FIRST(&sc->sc_rxbuf);
        if (bf == NULL) {       /* XXX ??? can this happen */
            printk("%s: no buffer (%s)\n", dev->name, __func__);
            break;
        }
        ds = bf->bf_desc;
        if (ds->ds_link == bf->bf_daddr) {
            /* NB: never process the self-linked entry at the end */
            break;
        }
        skb = bf->bf_skb;
        if (skb == NULL) {      /* XXX ??? can this happen */
            printk("%s: no skbuff (%s)\n", dev->name, __func__);
            continue;
        }
        /* XXX sync descriptor memory */
        /*
         * Must provide the virtual address of the current
         * descriptor, the physical address, and the virtual
         * address of the next descriptor in the h/w chain.
         * This allows the HAL to look ahead to see if the
         * hardware is done with a descriptor by checking the
         * done bit in the following descriptor and the address
         * of the current descriptor the DMA engine is working
         * on.  All this is necessary because of our use of
         * a self-linked list to avoid rx overruns.
         */
        status = ath_hal_rxprocdescfast(ah, ds,
                        bf->bf_daddr, PA2DESC(sc, ds->ds_link),
                        &rx_stats);
#ifdef AR_DEBUG
        if (sc->sc_debug & ATH_DEBUG_RECV_DESC)
            ath_printrxbuf(bf, status == HAL_OK);
#endif
        if (status == HAL_EINPROGRESS)
            break;
        TAILQ_REMOVE(&sc->sc_rxbuf, bf, bf_list);


#ifdef ATH_FORCE_PPM
        /* SM clocked on rx frames */
        if (ic->ic_sta_assoc == 1 && ath_force_ppm_enable) {
#ifdef DEBUG_PKTLOG
            ath_force_ppm_logic(sc, bf, status, &rx_stats, &pktlog_misc[0]);
            pktLogMiscCnt = 7;
#else
            ath_force_ppm_logic(sc, bf, status, &rx_stats, 0);
#endif
        }
#endif /* ATH_FORCE_PPM */

#ifdef DEBUG_PKTLOG
        {
            struct log_rx log_data;
            log_data.ds = ds;
            log_data.status = &rx_stats;

#ifdef ATH_FORCE_PPM
            log_data.misc[0] = pktlog_misc[0];
            log_data.misc[1] = pktlog_misc[1];
            log_data.misc[2] = pktlog_misc[2];
            log_data.misc[3] = pktlog_misc[3];
            log_data.misc[4] = pktlog_misc[4];
            log_data.misc[5] = pktlog_misc[5];
            log_data.misc[6] = pktlog_misc[6];
#endif

            ath_log_rx(sc, &log_data, pktLogMiscCnt);
        }
#endif
        /*
         * last one for receive
         */
        if (TAILQ_EMPTY(&sc->sc_rxbuf))
            sc->sc_rxlink = NULL;

        if (rx_stats.rs_flags & HAL_RX_DELIM_CRC_PRE)
            sc->sc_stats.ast_rx_delim_pre_crcerr++;
        if (rx_stats.rs_flags & HAL_RX_DELIM_CRC_POST)
            sc->sc_stats.ast_rx_delim_post_crcerr++;
        if (rx_stats.rs_flags & HAL_RX_DECRYPT_BUSY)
            sc->sc_stats.ast_rx_decrypt_busyerr++;

        if (rx_stats.rs_more) {
            /*
             * Frame spans multiple descriptors; this
             * cannot happen yet as we don't support
             * jumbograms.  If not in monitor mode,
             * discard the frame.
             */
#ifndef ERROR_FRAMES
            /*
             * Enable this if you want to see
             * error frames in Monitor mode.
             */
            if (ic->ic_opmode != IEEE80211_M_MONITOR) {
                sc->sc_stats.ast_rx_toobig++;
                goto rx_next;
            }
#endif
            /* fall thru for monitor mode handling... */
        } else if (rx_stats.rs_status != 0) {
            if (rx_stats.rs_status & HAL_RXERR_CRC)
                sc->sc_stats.ast_rx_crcerr++;
            if (rx_stats.rs_status & HAL_RXERR_FIFO)
                sc->sc_stats.ast_rx_fifoerr++;
            if (rx_stats.rs_status & HAL_RXERR_PHY) {
                sc->sc_stats.ast_rx_phyerr++;
                phyerr = rx_stats.rs_phyerr & 0x1f;
                sc->sc_stats.ast_rx_phy[phyerr]++;
                goto rx_next;
            }
            if (rx_stats.rs_status & HAL_RXERR_DECRYPT) {
                /*
                 * Decrypt error.  If the error occurred
                 * because there was no hardware key, then
                 * let the frame through so the upper layers
                 * can process it.  This is necessary for 5210
                 * parts which have no way to setup a ``clear''
                 * key cache entry.
                 *
                 * XXX do key cache faulting
                 */
                if (rx_stats.rs_keyix == HAL_RXKEYIX_INVALID)
                    goto rx_accept;
                sc->sc_stats.ast_rx_badcrypt++;
            }
            if (rx_stats.rs_status & HAL_RXERR_MIC) {
                mic_fail = 1;
                sc->sc_stats.ast_rx_badmic++;
            }
            /*
             * Reject error frames at the exception of MIC failure.
             * We normally don't want
             * to see them in monitor mode (in monitor mode
             * allow through packets that have crypto problems).
             */
             if (sc->sc_ic.ic_opmode == IEEE80211_M_MONITOR) {
                if (rx_stats.rs_status &
                        ~(HAL_RXERR_DECRYPT | HAL_RXERR_MIC)) {
                        goto rx_next;
                }
             } else {
                if (rx_stats.rs_status & ~(HAL_RXERR_MIC)) {
                        goto rx_next;
                }
             }
        }
rx_accept:
        /*
         * Sync and unmap the frame.  At this point we're
         * committed to passing the sk_buff somewhere so
         * clear buf_skb; this means a new sk_buff must be
         * allocated when the rx descriptor is setup again
         * to receive another frame.
         */
        len = rx_stats.rs_datalen;
        bus_dma_sync_single(sc->sc_bdev,
            bf->bf_skbaddr, len, BUS_DMA_FROMDEVICE);
        bus_unmap_single(sc->sc_bdev, bf->bf_skbaddr,
            sc->sc_rxbufsize, BUS_DMA_FROMDEVICE);
        bf->bf_skb = NULL;

        sc->sc_stats.ast_ant_rx[rx_stats.rs_antenna]++;
        sc->sc_devstats.rx_packets++;
        sc->sc_devstats.rx_bytes += len;

        sc->sc_stats.ast_rx_rssi_combined  = rx_stats.rs_rssi_combined;
        sc->sc_stats.ast_rx_rssi_ctl0  = rx_stats.rs_rssi_ctl0;
        sc->sc_stats.ast_rx_rssi_ctl1  = rx_stats.rs_rssi_ctl1;
        sc->sc_stats.ast_rx_rssi_ctl2  = rx_stats.rs_rssi_ctl2;
        sc->sc_stats.ast_rx_rssi_ext0  = rx_stats.rs_rssi_ext0;
        sc->sc_stats.ast_rx_rssi_ext1  = rx_stats.rs_rssi_ext1;
        sc->sc_stats.ast_rx_rssi_ext2  = rx_stats.rs_rssi_ext2;

        if (ic->ic_opmode == IEEE80211_M_MONITOR) {
            /*
             * Monitor mode: discard anything shorter than
             * an ack or cts, clean the skbuff, fabricate
             * the Prism header existing tools expect,
             * and dispatch.
             */
            if (len < IEEE80211_ACK_LEN) {
                DPRINTF(sc, ATH_DEBUG_RECV,
                    "%s: runt packet %d\n", __func__, len);
                sc->sc_stats.ast_rx_tooshort++;
                dev_kfree_skb(skb);
                goto rx_next;
            }
            ath_rx_capture(dev, skb, &rx_stats);
            goto rx_next;
        }

        /*
         * From this point on we assume the frame is at least
         * as large as ieee80211_frame_min; verify that.
         */
        if (len < IEEE80211_MIN_LEN) {
            DPRINTF(sc, ATH_DEBUG_RECV, "%s: short packet %d\n",
                __func__, len);
            sc->sc_stats.ast_rx_tooshort++;
            dev_kfree_skb(skb);
            goto rx_next;
        }

        /*
         * Normal receive.
         */
        skb_put(skb, len - IEEE80211_CRC_LEN);
        skb->protocol = ETH_P_CONTROL;      /* XXX */

        if (IFF_DUMPPKTS(sc, ATH_DEBUG_RECV)) {
            ieee80211_dump_pkt(ic, skb->data, len,
                   (rx_stats.rs_rate & 0x80) ?
                   sc->sc_hthwmap[rx_stats.rs_rate & IEEE80211_RATE_VAL].ieeerate :
                   sc->sc_hwmap[rx_stats.rs_rate].ieeerate,
                   rx_stats.rs_rssi);
        }

        /* MIC failure. Drop the packet in any case */
        if (mic_fail) {
                ni = ieee80211_find_rxnode(ic,
                        (const struct ieee80211_frame_min *) skb->data);
                if (ni != NULL) {
                        ieee80211_check_mic(ni, skb);
                        ieee80211_free_node(ni);
                }
                dev_kfree_skb(skb);
                goto rx_next;
        }

        /*
         * Locate the node for sender, track state, and then
         * pass the (referenced) node up to the 802.11 layer
         * for its use.  If the sender is unknown spam the
         * frame; it'll be dropped where it's not wanted.
         */
        if (rx_stats.rs_keyix != HAL_RXKEYIX_INVALID &&
            (ni = sc->sc_keyixmap[rx_stats.rs_keyix]) != NULL) {
            struct ath_node *an;
            /*
             * Fast path: node is present in the key map;
             * grab a reference for processing the frame.
             */
            an = ATH_NODE(ieee80211_ref_node(ni));
            ATH_RSSI_LPF(an->an_avgrssi, rx_stats.rs_rssi);
#ifdef OWLAGGR
            type = owl_input(ni, skb, &rx_stats);
#else
            type = ieee80211_input(ni, skb,
                rx_stats.rs_rssi, rx_stats.rs_tstamp);
#endif
            ieee80211_free_node(ni);
        } else {
            /*
             * No key index or no entry, do a lookup and
             * add the node to the mapping table if possible.
             */
            ni = ieee80211_find_rxnode(ic,
                (const struct ieee80211_frame_min *) skb->data);
            if (ni != NULL) {
                struct ath_node *an = ATH_NODE(ni);
                u_int16_t keyix;

                ATH_RSSI_LPF(an->an_avgrssi,
                    rx_stats.rs_rssi);
#ifdef OWLAGGR
                type = owl_input(ni, skb, &rx_stats);
#else
                type = ieee80211_input(ni, skb,
                    rx_stats.rs_rssi,
                    rx_stats.rs_tstamp);
#endif
                /*
                 * If the station has a key cache slot assigned
                 * update the key->node mapping table.
                 */
                keyix = ni->ni_ucastkey.wk_keyix;
                if (keyix != IEEE80211_KEYIX_NONE &&
                    sc->sc_keyixmap[keyix] == NULL)
                    sc->sc_keyixmap[keyix] =
                        ieee80211_ref_node(ni);
                ieee80211_free_node(ni);
            } else
                type = ieee80211_input_all(ic, skb,
                    rx_stats.rs_rssi,
                    rx_stats.rs_tstamp);
        }

        if (sc->sc_diversity) {
            /*
             * When using fast diversity, change the default rx
             * antenna if diversity chooses the other antenna 3
             * times in a row.
             */
            if (sc->sc_defant != rx_stats.rs_antenna) {
                if (++sc->sc_rxotherant >= 3)
                    ath_setdefantenna(sc,
                        rx_stats.rs_antenna);
            } else
                sc->sc_rxotherant = 0;
        }
        if (sc->sc_softled) {
            /*
             * Blink for any data frame.  Otherwise do a
             * heartbeat-style blink when idle.  The latter
             * is mainly for station mode where we depend on
             * periodic beacon frames to trigger the poll event.
             */
            if (type == IEEE80211_FC0_TYPE_DATA) {
                sc->sc_rxrate = rx_stats.rs_rate;
                ath_led_event(sc, ATH_LED_RX);
            } else if (jiffies - sc->sc_ledevent >= sc->sc_ledidle)
                ath_led_event(sc, ATH_LED_POLL);
        }
rx_next:
        TAILQ_INSERT_TAIL(&sc->sc_rxbuf, bf, bf_list);
    } while (ath_rxbuf_init(sc, bf) == 0);

#ifndef AR5416_EMULATION
    /* rx signal state monitoring */
    ath_hal_rxmonitor(ah, &sc->sc_halstats, &sc->sc_curchan);
#endif

    /* re-enable receive interrupts */
    local_irq_save(flags);
    sc->sc_imask |= HAL_INT_RX;
    ath_hal_intrset(ah, sc->sc_imask);
    local_irq_restore(flags);

#undef PA2DESC
}


#ifdef ATH_SUPERG_XR

static void
ath_grppoll_period_update(struct ath_softc *sc)
{
    struct ieee80211com *ic = &sc->sc_ic;
    u_int16_t interval;
    u_int16_t xrsta;
    u_int16_t normalsta;
    u_int16_t allsta;

    xrsta = ic->ic_xr_sta_assoc;

    /*
     * if no stations are in XR mode.
     * use default poll interval.
     */
    if(xrsta == 0) {
        if(sc->sc_xrpollint != XR_DEFAULT_POLL_INTERVAL) {
            sc->sc_xrpollint = XR_DEFAULT_POLL_INTERVAL;
            ath_grppoll_txq_update(sc,XR_DEFAULT_POLL_INTERVAL);
        }
        return;
    }

    allsta = ic->ic_sta_assoc;
    /*
     * if all the stations are in XR mode.
     * use minimum poll interval.
     */
    if(allsta == xrsta) {
        if(sc->sc_xrpollint != XR_MIN_POLL_INTERVAL) {
            sc->sc_xrpollint = XR_MIN_POLL_INTERVAL;
            ath_grppoll_txq_update(sc,XR_MIN_POLL_INTERVAL);
        }
        return;
    }

    normalsta = allsta-xrsta;
    /*
     * if stations are in both XR and normal mode.
     * use some fudge factor.
     */
    interval =   XR_DEFAULT_POLL_INTERVAL -
          ((XR_DEFAULT_POLL_INTERVAL - XR_MIN_POLL_INTERVAL) * xrsta)/(normalsta * XR_GRPPOLL_PERIOD_FACTOR);
    if(interval < XR_MIN_POLL_INTERVAL)
        interval = XR_MIN_POLL_INTERVAL;

    if(sc->sc_xrpollint != interval) {
            sc->sc_xrpollint = interval;
            ath_grppoll_txq_update(sc,interval);
    }

    /*
     * XXX: what if stations go to sleap.
     * ideally  the interval should be adjusted dynamically based on
     * xr and normal upstream traffic.
     */

}

/*
 * update grppoll period.
 */
static void
ath_grppoll_txq_update(struct ath_softc *sc, int period)
{
    struct ath_hal *ah = sc->sc_ah;
    HAL_TXQ_INFO qi;
    struct ath_txq *txq = &sc->sc_grpplq;

    if(sc->sc_grpplq.axq_qnum == -1) return;

    memset(&qi, 0, sizeof(qi));
    qi.tqi_subtype = 0;
    qi.tqi_aifs = XR_AIFS;
    qi.tqi_cwmin = XR_CWMIN_CWMAX;
    qi.tqi_cwmax = XR_CWMIN_CWMAX;
    qi.tqi_compBuf = 0;
    qi.tqi_cbrPeriod = IEEE80211_TU_TO_MS(period)*1000; /* usec */
    qi.tqi_cbrOverflowLimit = 2 ;
    ath_hal_settxqueueprops(ah, txq->axq_qnum,&qi);
    ath_hal_resettxqueue(ah, txq->axq_qnum); /* push to h/w */

}

/*
 * Setup grppoll  h/w transmit queue.
 */
static void
ath_grppoll_txq_setup(struct ath_softc *sc, int qtype, int period)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    struct ath_hal *ah = sc->sc_ah;
    HAL_TXQ_INFO qi;
    int qnum;
    u_int compbufsz = 0;
    char *compbuf = NULL;
    dma_addr_t compbufp = 0;
    struct ath_txq *txq = &sc->sc_grpplq;

    memset(&qi, 0, sizeof(qi));
    qi.tqi_subtype = 0;
    qi.tqi_aifs = XR_AIFS;
    qi.tqi_cwmin = XR_CWMIN_CWMAX;
    qi.tqi_cwmax = XR_CWMIN_CWMAX;
    qi.tqi_compBuf = 0;
    qi.tqi_cbrPeriod = IEEE80211_TU_TO_MS(period)*1000; /* usec */
    qi.tqi_cbrOverflowLimit = 2 ;

    if(sc->sc_grpplq.axq_qnum == -1) {
        qnum = ath_hal_setuptxqueue(ah, qtype, &qi);
        if (qnum == -1) {
            return ;
        }
        if (qnum >= N(sc->sc_txq)) {
            printk("%s: hal qnum %u out of range, max %u!\n",
                   sc->sc_dev->name, qnum, N(sc->sc_txq));
            ath_hal_releasetxqueue(ah, qnum);
            return;
        }

        txq->axq_qnum = qnum;
    }
    txq->axq_link = NULL;
    TAILQ_INIT(&txq->axq_q);
    ATH_TXQ_LOCK_INIT(txq);
    txq->axq_depth = 0;
    txq->axq_totalqueued = 0;
    txq->axq_intrcnt = 0;
    txq->axq_linkbuf = NULL;
    TAILQ_INIT(&txq->axq_stageq);
    TAILQ_INIT(&txq->axq_acq);
    TAILQ_INIT(&txq->axq_fltrq);
    txq->axq_compbuf = compbuf;
    txq->axq_compbufsz = compbufsz;
    txq->axq_compbufp = compbufp;
    ath_hal_resettxqueue(ah, txq->axq_qnum); /* push to h/w */
#undef N

}

/*
 * Setup group poll frames on the group poll queue.
 */
static void ath_grppoll_start(struct ieee80211vap *vap,int pollcount)
{

    int i,amode;
    int flags;
    struct sk_buff *skb = NULL;
    struct ath_buf *bf,*head=NULL;
    struct ieee80211com *ic = vap->iv_ic;
    struct ath_softc *sc = ic->ic_dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    u_int8_t rate;
    int ctsrate=0;
    int ctsduration=0;
    const HAL_RATE_TABLE *rt;
    u_int8_t cix,rtindex=0;
    u_int type;
    struct ath_txq *txq=&sc->sc_grpplq;
    struct ath_desc *ds=NULL;
    int pktlen=0,keyix=0;
    int pollsperrate,pos;
    int rates[XR_NUM_RATES];
    u_int8_t ratestr[16],numpollstr[16];
    typedef struct rate_to_str_map {
        u_int8_t  str[4];
        int  ratekbps;
    } RATE_TO_STR_MAP;

    static const RATE_TO_STR_MAP ratestrmap[] = {
            {"0.25",    250},
            { ".25",    250},
            {"0.5",     500},
            { ".5",     500},
            {  "1",    1000},
            {  "3",    3000},
            {  "6",    6000},
            {  "?",    0},
    };



#define MAX_GRPPOLL_RATE 5
#define USE_SHPREAMBLE(_ic) \
    (((_ic)->ic_flags & (IEEE80211_F_SHPREAMBLE | IEEE80211_F_USEBARKER))\
        == IEEE80211_F_SHPREAMBLE)

    if(sc->sc_xrgrppoll) return;

    memset(&rates, 0, sizeof(rates));
    pos=0;
    while(sscanf(&(sc->sc_grppoll_str[pos]),"%s %s",ratestr,numpollstr) == 2) {
        int i=0;
        while(ratestrmap[i].ratekbps != 0) {
            if(strcmp(ratestrmap[i].str,ratestr) == 0 )
                break;
            ++i;
        }
        sscanf(numpollstr,"%d",&(rates[i]));
        pos += strlen(ratestr) + strlen(numpollstr) + 2;
    }
    if (!sc->sc_grppolldma.dd_bufptr) {
        printk("grppoll_start: grppoll Buf allocation failed \n");
        return;
    }
    rt = sc->sc_currates;
    cix = rt->info[sc->sc_protrix].controlRate;
    ctsrate = rt->info[cix].rateCode;
    if (USE_SHPREAMBLE(ic))
            ctsrate |= rt->info[cix].shortPreamble;
    rt = sc->sc_xr_rates;
    /*
     * queue the group polls for each antenna mode. set the right keycache index for the
     * broadcast packets. this will ensure that if the first poll
     * does not elicit a single chirp from any XR station, hardware will
     * not send the subsequent polls
     */
    pollsperrate=0;
    for(amode = HAL_ANTENNA_FIXED_A; amode < HAL_ANTENNA_MAX_MODE ; ++amode) {
        for(i=0;i<(pollcount+1);++i) {

            flags = HAL_TXDESC_NOACK;
            rate = rt->info[rtindex].rateCode;
            /*
             * except for the last one every thing else is a CF poll.
             * last one is  the CF End  frame.
             */

            if(i == pollcount) {
                skb=ieee80211_getcfframe(vap,IEEE80211_FC0_SUBTYPE_CF_END);
                rate=ctsrate;
                ctsduration = ath_hal_computetxtime(ah,sc->sc_currates,pktlen,sc->sc_protrix, AH_FALSE);
            } else {
                skb=ieee80211_getcfframe(vap,IEEE80211_FC0_SUBTYPE_CFPOLL);
                pktlen = skb->len + IEEE80211_CRC_LEN;
                /*
                 * the very first group poll ctsduration  should be enough to allow
                 * an auth frame from station. This is to pass the wifi testing (as
                 * some stations in testing do not honor CF_END and rely on CTS duration)
                 */
                if(i == 0 && amode == HAL_ANTENNA_FIXED_A) {
                    ctsduration = ath_hal_computetxtime(ah,rt, pktlen,rtindex, AH_FALSE) /*cf-poll time */
                        + (XR_AIFS +  (XR_CWMIN_CWMAX * XR_SLOT_DELAY))
                        + ath_hal_computetxtime(ah,rt,2*(sizeof(struct ieee80211_frame_min) + 6),
                                                IEEE80211_XR_DEFAULT_RATE_INDEX,AH_FALSE) /*auth packet time */
                        + ath_hal_computetxtime(ah,rt,IEEE80211_ACK_LEN,
                                                IEEE80211_XR_DEFAULT_RATE_INDEX, AH_FALSE); /*ack frame time */
                } else {

                    ctsduration = ath_hal_computetxtime(ah,rt, pktlen,rtindex, AH_FALSE) /*cf-poll time */
                        + (XR_AIFS +  (XR_CWMIN_CWMAX * XR_SLOT_DELAY))
                        + ath_hal_computetxtime(ah,rt,XR_FRAGMENTATION_THRESHOLD ,
                                                IEEE80211_XR_DEFAULT_RATE_INDEX,AH_FALSE) /*data packet time */
                        + ath_hal_computetxtime(ah,rt,IEEE80211_ACK_LEN,
                                                IEEE80211_XR_DEFAULT_RATE_INDEX, AH_FALSE); /*ack frame time */
                }
                if((vap->iv_flags & IEEE80211_F_PRIVACY) && keyix == 0)  {
                    struct ieee80211_key *k;
                    k = ieee80211_crypto_encap(vap->iv_bss, skb);
                    if(k)
                      keyix = k->wk_keyix;
                }
            }
            ATH_TXBUF_LOCK_BH(sc);
            bf = TAILQ_FIRST(&sc->sc_grppollbuf);
            if (bf != NULL) {
                TAILQ_REMOVE(&sc->sc_grppollbuf, bf, bf_list);
            } else {
                DPRINTF(sc, ATH_DEBUG_XMIT, "%s: No more TxBufs \n", __func__);
                return;
            }
            /* XXX use a counter and leave at least one for mgmt frames */
            if (TAILQ_EMPTY(&sc->sc_grppollbuf)) {
                DPRINTF(sc, ATH_DEBUG_XMIT, "%s: No more TxBufs left\n", __func__);
                return;
            }
            ATH_TXBUF_UNLOCK_BH(sc);
            bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
                                            skb->data, skb->len, BUS_DMA_TODEVICE);
            bf->bf_skb = skb;
            ATH_TXQ_INSERT_TAIL(txq, bf, bf_list);
            ds = bf->bf_desc;
            ds->ds_data = bf->bf_skbaddr;
            if(i == pollcount && amode ==  (HAL_ANTENNA_MAX_MODE -1)) {
                type = HAL_PKT_TYPE_NORMAL;
                flags |= (HAL_TXDESC_CLRDMASK | HAL_TXDESC_VEOL);
            } else {
                flags |= HAL_TXDESC_CTSENA;
                type = HAL_PKT_TYPE_GRP_POLL;
            }
            if(i == 0 && amode == HAL_ANTENNA_FIXED_A ) {
                flags |= HAL_TXDESC_CLRDMASK;
                head = bf;
            }

#ifdef DEBUG_PKTLOG
                        ds->ds_vdata = (u_int32_t)skb->data;
#endif

            ath_hal_setuptxdesc(ah, ds
                                , skb->len + IEEE80211_CRC_LEN  /* frame length */
                                , sizeof(struct ieee80211_frame)/* header length */
                                , type      /* Atheros packet type */
                                , ic->ic_txpowlimit     /* max txpower */
                                , rate, 0           /* series 0 rate/tries */
                                , keyix /* HAL_TXKEYIX_INVALID */       /* use key index */
                                , amode         /* antenna mode */
                                , flags
                                , ctsrate       /* rts/cts rate */
                                , ctsduration           /* rts/cts duration */
                                , 0             /* comp icv len */
                                , 0             /* comp iv len */
                                , ATH_COMP_PROC_NO_COMP_NO_CCS  /* comp scheme */
                );
            ath_hal_filltxdesc(ah, ds
                               , roundup(skb->len, 4)       /* buffer length */
                               , AH_TRUE            /* first segment */
                               , AH_TRUE            /* last segment */
                               , ds             /* first descriptor */
                );
            /* NB: The desc swap function becomes void,
            * if descriptor swapping is not enabled
            */
            ath_desc_swap(ds);
            if(txq->axq_link) {
#ifdef AH_NEED_DESC_SWAP
            *txq->axq_link = cpu_to_le32(bf->bf_daddr);
#else
            *txq->axq_link = bf->bf_daddr;
#endif
            }
            txq->axq_link = &ds->ds_link;
            ++pollsperrate;
            if(pollsperrate  > rates[rtindex]) {
                rtindex = (rtindex + 1) % MAX_GRPPOLL_RATE;
                pollsperrate=0;
            }
        }
    }
    /* make it circular */
#ifdef AH_NEED_DESC_SWAP
    ds->ds_link = cpu_to_le32(head->bf_daddr);
#else
    ds->ds_link = head->bf_daddr;
#endif
    /* start the queue */
    ath_hal_puttxbuf(ah, txq->axq_qnum, head->bf_daddr);
    ath_hal_txstart(ah, txq->axq_qnum);
    sc->sc_xrgrppoll=1;
#undef USE_SHPREAMBLE
}

static void ath_grppoll_stop(struct ieee80211vap *vap)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ath_softc *sc = ic->ic_dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    struct ath_txq *txq=&sc->sc_grpplq;
    struct ath_buf *bf;


    if(!sc->sc_xrgrppoll) return;
    ath_hal_stoptxdma(ah, txq->axq_qnum);

    /* move the grppool bufs back to  the grppollbuf */
    for (;;) {
        ATH_TXQ_LOCK(txq);
        bf = TAILQ_FIRST(&txq->axq_q);
        if (bf == NULL) {
            txq->axq_link = NULL;
            txq->axq_linkbuf = NULL;
            ATH_TXQ_UNLOCK(txq);
            break;
        }
        ATH_TXQ_REMOVE_HEAD(txq, bf, bf_list);
        ATH_TXQ_UNLOCK(txq);
        bus_unmap_single(sc->sc_bdev,
                         bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);
        dev_kfree_skb(bf->bf_skb);
        bf->bf_skb = NULL;
        bf->bf_node = NULL;

        ATH_TXBUF_LOCK(sc);
        TAILQ_INSERT_TAIL(&sc->sc_grppollbuf, bf, bf_list);
        ATH_TXBUF_UNLOCK(sc);
    }
    TAILQ_INIT(&txq->axq_q);
    ATH_TXQ_LOCK_INIT(txq);
    txq->axq_depth = 0;
    txq->axq_totalqueued = 0;
    txq->axq_intrcnt = 0;
    TAILQ_INIT(&txq->axq_stageq);
    TAILQ_INIT(&txq->axq_acq);
    TAILQ_INIT(&txq->axq_fltrq);
    sc->sc_xrgrppoll=0;
}
#endif

/*
 * Setup a h/w transmit queue.
 */
static struct ath_txq *
ath_txq_setup(struct ath_softc *sc, int qtype, int subtype)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    struct ath_hal *ah = sc->sc_ah;
    HAL_TXQ_INFO qi;
    int qnum;
    u_int compbufsz = 0;
    char *compbuf = NULL;
    dma_addr_t compbufp = 0;

    memset(&qi, 0, sizeof(qi));
    qi.tqi_subtype = subtype;
    qi.tqi_aifs = HAL_TXQ_USEDEFAULT;
    qi.tqi_cwmin = HAL_TXQ_USEDEFAULT;
    qi.tqi_cwmax = HAL_TXQ_USEDEFAULT;
    qi.tqi_compBuf = 0;
#ifdef ATH_SUPERG_XR
    if(subtype == HAL_XR_DATA) {
        qi.tqi_aifs  = XR_DATA_AIFS;
        qi.tqi_cwmin = XR_DATA_CWMIN;
        qi.tqi_cwmax = XR_DATA_CWMAX;
    }
#endif

#ifdef ATH_SUPERG_COMP
    /* allocate compression scratch buffer for data queues */
    if ((qtype == HAL_TX_QUEUE_DATA)
            && ath_hal_compressionsupported(ah)) {
        compbufsz = roundup(HAL_COMP_BUF_MAX_SIZE,
            HAL_COMP_BUF_ALIGN_SIZE) + HAL_COMP_BUF_ALIGN_SIZE;
        compbuf = (char *)bus_alloc_consistent(sc->sc_bdev,
                    compbufsz, &compbufp);
        if (compbuf == NULL) {
            sc->sc_ic.ic_ath_cap &= ~IEEE80211_ATHC_COMP;
        } else {
            qi.tqi_compBuf = (u_int32_t)compbufp;
        }
    }
#endif
    /*
     * Enable interrupts only for EOL and DESC conditions.
     * We mark tx descriptors to receive a DESC interrupt
     * when a tx queue gets deep; otherwise waiting for the
     * EOL to reap descriptors.  Note that this is done to
     * reduce interrupt load and this only defers reaping
     * descriptors, never transmitting frames.  Aside from
     * reducing interrupts this also permits more concurrency.
     * The only potential downside is if the tx queue backs
     * up in which case the top half of the kernel may backup
     * due to a lack of tx descriptors.
     */
    qi.tqi_qflags = TXQ_FLAG_TXOKINT_ENABLE | TXQ_FLAG_TXEOLINT_ENABLE | TXQ_FLAG_TXDESCINT_ENABLE;
    qnum = ath_hal_setuptxqueue(ah, qtype, &qi);
    if (qnum == -1) {
        /*
         * NB: don't print a message, this happens
         * normally on parts with too few tx queues
         */
#ifdef ATH_SUPERG_COMP
        if (compbuf) {
            bus_free_consistent(sc->sc_bdev, compbufsz,
                    compbuf, compbufp);
        }
#endif
        return NULL;
    }
    if (qnum >= N(sc->sc_txq)) {
        printk("%s: hal qnum %u out of range, max %u!\n",
            sc->sc_dev->name, qnum, N(sc->sc_txq));
#ifdef ATH_SUPERG_COMP
        if (compbuf) {
            bus_free_consistent(sc->sc_bdev, compbufsz,
                    compbuf, compbufp);
        }
#endif
        ath_hal_releasetxqueue(ah, qnum);
        return NULL;
    }
    if (!ATH_TXQ_SETUP(sc, qnum)) {
        struct ath_txq *txq = &sc->sc_txq[qnum];

        txq->axq_qnum = qnum;
        txq->axq_link = NULL;
        TAILQ_INIT(&txq->axq_q);
        ATH_TXQ_LOCK_INIT(txq);
        txq->axq_depth = 0;
        txq->axq_totalqueued = 0;
        txq->axq_intrcnt = 0;
        txq->axq_linkbuf = NULL;
        TAILQ_INIT(&txq->axq_stageq);
        TAILQ_INIT(&txq->axq_acq);
        TAILQ_INIT(&txq->axq_fltrq);
        txq->axq_compbuf = compbuf;
        txq->axq_compbufsz = compbufsz;
        txq->axq_compbufp = compbufp;
        sc->sc_txqsetup |= 1<<qnum;
    }
    return &sc->sc_txq[qnum];
#undef N
}

/*
 * Setup a hardware data transmit queue for the specified
 * access control.  The hal may not support all requested
 * queues in which case it will return a reference to a
 * previously setup queue.  We record the mapping from ac's
 * to h/w queues for use by ath_tx_start and also track
 * the set of h/w queues being used to optimize work in the
 * transmit interrupt handler and related routines.
 */
static int
ath_tx_setup(struct ath_softc *sc, int ac, int haltype)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    struct ath_txq *txq;

    if (ac >= N(sc->sc_ac2q)) {
        printk("%s: AC %u out of range, max %zu!\n",
            sc->sc_dev->name, ac, N(sc->sc_ac2q));
        return 0;
    }
    txq = ath_txq_setup(sc, HAL_TX_QUEUE_DATA, haltype);
    if (txq != NULL) {
        sc->sc_ac2q[ac] = txq;
        return 1;
    } else
        return 0;
#undef N
}

/*
 * Update WME parameters for a transmit queue.
 */
static int
ath_txq_update(struct ath_softc *sc, int ac)
{
#define ATH_EXPONENT_TO_VALUE(v)    ((1<<v)-1)
#define ATH_TXOP_TO_US(v)       (v<<5)
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_txq *txq = sc->sc_ac2q[ac];
    struct wmeParams *wmep = &ic->ic_wme.wme_chanParams.cap_wmeParams[ac];
    struct ath_hal *ah = sc->sc_ah;
    HAL_TXQ_INFO qi;

    ath_hal_gettxqueueprops(ah, txq->axq_qnum, &qi);
    qi.tqi_aifs = wmep->wmep_aifsn;
    qi.tqi_cwmin = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmin);
    qi.tqi_cwmax = ATH_EXPONENT_TO_VALUE(wmep->wmep_logcwmax);
    qi.tqi_burstTime = ATH_TXOP_TO_US(wmep->wmep_txopLimit);

    if (!ath_hal_settxqueueprops(ah, txq->axq_qnum, &qi)) {
        printk("%s: unable to update hardware queue "
            "parameters for %s traffic!\n",
            sc->sc_dev->name, ieee80211_wme_acnames[ac]);
        return 0;
    } else {
        ath_hal_resettxqueue(ah, txq->axq_qnum); /* push to h/w */
        return 1;
    }
#undef ATH_TXOP_TO_US
#undef ATH_EXPONENT_TO_VALUE
}

/*
 * Callback from the 802.11 layer to update WME parameters.
 */
static int
ath_wme_update(struct ieee80211com *ic)
{
    struct ath_softc *sc = ic->ic_dev->priv;

    return !ath_txq_update(sc, WME_AC_BE) ||
        !ath_txq_update(sc, WME_AC_BK) ||
        !ath_txq_update(sc, WME_AC_VI) ||
        !ath_txq_update(sc, WME_AC_VO) ? EIO : 0;
}

/*
 * Reclaim resources for a setup queue.
 */
static void
ath_tx_cleanupq(struct ath_softc *sc, struct ath_txq *txq)
{

#ifdef ATH_SUPERG_COMP
    /* Release compression buffer */
    if (txq->axq_compbuf) {
        bus_free_consistent(sc->sc_bdev, txq->axq_compbufsz,
            txq->axq_compbuf, txq->axq_compbufp);
        txq->axq_compbuf = NULL;
    }
#endif
    ath_hal_releasetxqueue(sc->sc_ah, txq->axq_qnum);
    ATH_TXQ_LOCK_DESTROY(txq);
    sc->sc_txqsetup &= ~(1<<txq->axq_qnum);
}

/*
 * Reclaim all tx queue resources.
 */
static void
ath_tx_cleanup(struct ath_softc *sc)
{
    int i;

    ATH_TXBUF_LOCK_DESTROY(sc);
    for (i = 0; i < HAL_NUM_TX_QUEUES; i++)
        if (ATH_TXQ_SETUP(sc, i))
            ath_tx_cleanupq(sc, &sc->sc_txq[i]);
}

#ifdef ATH_SUPERG_COMP
static u_int32_t
ath_get_icvlen(struct ieee80211_key *k)
{
        const struct ieee80211_cipher *cip = k->wk_cipher;

        if (cip->ic_cipher == IEEE80211_CIPHER_AES_CCM ||
            cip->ic_cipher == IEEE80211_CIPHER_AES_OCB) {
            return AES_ICV_FIELD_SIZE;
        }

        return WEP_ICV_FIELD_SIZE;
}

static u_int32_t
ath_get_ivlen(struct ieee80211_key *k)
{
    const struct ieee80211_cipher *cip = k->wk_cipher;
    u_int32_t ivlen;

        ivlen = WEP_IV_FIELD_SIZE;

        if (cip->ic_cipher == IEEE80211_CIPHER_AES_CCM ||
            cip->ic_cipher == IEEE80211_CIPHER_AES_OCB) {
            ivlen += EXT_IV_FIELD_SIZE;
        }

    return ivlen;
}
#endif

/*
 * Get transmit rate index using rate in Kbps
 */
static inline int
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

/*
 * Insert a buffer on a txq
 *
 */
static inline void
ath_tx_txqaddbuf(struct ath_softc *sc, struct ieee80211_node *ni,
         struct ath_txq *txq, struct ath_buf *bf,
         struct ath_desc *lastds, int framelen)
{
    struct ath_hal *ah = sc->sc_ah;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ath_vap *avp = ATH_VAP(vap);

    /*
     * Insert the frame on the outbound list and
     * pass it on to the hardware.
     */
    ATH_TXQ_LOCK_BH(txq);
    if (txq == &avp->av_mcastq) {
        /*
         * The CAB queue is started from the SWBA handler since
         * frames only go out on DTIM and to avoid possible races.
         */
        ath_hal_intrset(ah, sc->sc_imask & ~HAL_INT_SWBA);
        ATH_TXQ_INSERT_TAIL(txq, bf, bf_list);
        DPRINTF(sc,ATH_DEBUG_TX_PROC, "%s: txq depth = %d\n", __func__, txq->axq_depth);
        if (txq->axq_link != NULL) {
#ifdef AH_NEED_DESC_SWAP
            *txq->axq_link = cpu_to_le32(bf->bf_daddr);
#else
            *txq->axq_link = bf->bf_daddr;
#endif
            DPRINTF(sc, ATH_DEBUG_XMIT, "%s: link[%u](%p)=%p (%p)\n",
                __func__,
                txq->axq_qnum, txq->axq_link,
                (caddr_t)bf->bf_daddr, bf->bf_desc);
        }
        txq->axq_link = &lastds->ds_link;
        ath_hal_intrset(ah, sc->sc_imask);
    } else {
        ATH_TXQ_INSERT_TAIL(txq, bf, bf_list);
        DPRINTF(sc, ATH_DEBUG_TX_PROC, "%s: txq depth = %d\n", __func__, txq->axq_depth);
        if (txq->axq_link == NULL) {
            ath_hal_puttxbuf(ah, txq->axq_qnum, bf->bf_daddr);
            DPRINTF(sc, ATH_DEBUG_XMIT, "%s: TXDP[%u] = %p (%p)\n",
                __func__,
                txq->axq_qnum, (caddr_t)bf->bf_daddr, bf->bf_desc);
        } else {
#ifdef AH_NEED_DESC_SWAP
            *txq->axq_link = cpu_to_le32(bf->bf_daddr);
#else
            *txq->axq_link = bf->bf_daddr;
#endif
            DPRINTF(sc, ATH_DEBUG_XMIT, "%s: link[%u] (%p)=%p (%p)\n",
                __func__,
                txq->axq_qnum, txq->axq_link,
                (caddr_t)bf->bf_daddr, bf->bf_desc);
        }
        txq->axq_link = &lastds->ds_link;
        ath_hal_txstart(ah, txq->axq_qnum);
        sc->sc_dev->trans_start = jiffies;
    }
    ATH_TXQ_UNLOCK_BH(txq);

    sc->sc_devstats.tx_packets++;
    sc->sc_devstats.tx_bytes += framelen;
}

static int
ath_tx_start(struct net_device *dev, struct ieee80211_node *ni, struct ath_buf *bf, struct sk_buff *skb, int nextfraglen)
{
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ath_hal *ah = sc->sc_ah;
    int iswep, ismcast, keyix, hdrlen, pktlen, try0;
    u_int8_t rix, txrate, ctsrate;
    u_int32_t ivlen = 0, icvlen = 0;
    int comp = ATH_COMP_PROC_NO_COMP_NO_CCS;
    u_int8_t cix = 0xff;        /* NB: silence compiler */
    struct ath_desc *ds=NULL;
    struct ath_txq *txq=NULL;
    struct ieee80211_frame *wh;
    u_int subtype, flags, ctsduration;
    HAL_PKT_TYPE atype;
    const HAL_RATE_TABLE *rt;
    HAL_BOOL shortPreamble;
    struct ath_node *an;
    struct ath_vap *avp = ATH_VAP(vap);
    int istxfrag;
    struct ath_rc_series rcs[4] = {{0}};
    HAL_11N_RATE_SERIES series[4] = {{ 0 }};

    wh = (struct ieee80211_frame *) skb->data;
    iswep = wh->i_fc[1] & IEEE80211_FC1_WEP;
    ismcast = IEEE80211_IS_MULTICAST(wh->i_addr1);
    hdrlen = ieee80211_anyhdrsize(wh);
    istxfrag = (wh->i_fc[1] & IEEE80211_FC1_MORE_FRAG) ||
        (((le16toh(*(u_int16_t *) &wh->i_seq[0]) >>
           IEEE80211_SEQ_FRAG_SHIFT) & IEEE80211_SEQ_FRAG_MASK) > 0);

    pktlen = skb->len;
    rcs[1].tries = rcs[2].tries = rcs[3].tries = 0;
#ifdef ATH_SUPERG_FF
    {
        struct sk_buff *skbtmp = skb;
        while ((skbtmp = skbtmp->next)) {
            pktlen += skbtmp->len;
        }
    }
#endif
    /*
     * Packet length must not include any
     * pad bytes; deduct them here.
     */
    pktlen -= (hdrlen & 3);

    if (iswep) {
        const struct ieee80211_cipher *cip;
        struct ieee80211_key *k;

        /*
         * Construct the 802.11 header+trailer for an encrypted
         * frame. The only reason this can fail is because of an
         * unknown or unsupported cipher/key type.
         */

        /* FFXXX: change to handle linked skbs */
        k = ieee80211_crypto_encap(ni, skb);
        if (k == NULL) {
            /*
             * This can happen when the key is yanked after the
             * frame was queued.  Just discard the frame; the
             * 802.11 layer counts failures and provides
             * debugging/diagnostics.
             */
            return -EIO;
        }
        /*
         * Adjust the packet + header lengths for the crypto
         * additions and calculate the h/w key index.  When
         * a s/w mic is done the frame will have had any mic
         * added to it prior to entry so skb->len above will
         * account for it. Otherwise we need to add it to the
         * packet length.
         */
        cip = k->wk_cipher;
        hdrlen += cip->ic_header;
        pktlen += cip->ic_header + cip->ic_trailer;
        if ((k->wk_flags & IEEE80211_KEY_SWMIC) == 0) {
            if ( ! istxfrag)
                pktlen += cip->ic_miclen;
            else {
                if (cip->ic_cipher != IEEE80211_CIPHER_TKIP)
                    pktlen += cip->ic_miclen;
            }
        }
        keyix = k->wk_keyix;

#ifdef ATH_SUPERG_COMP
        icvlen = ath_get_icvlen(k)/4;
        ivlen = ath_get_ivlen(k)/4;
#endif
        /* packet header may have moved, reset our local pointer */
        wh = (struct ieee80211_frame *) skb->data;
    } else if (ni->ni_ucastkey.wk_cipher == &ieee80211_cipher_none) {
        /*
         * Use station key cache slot, if assigned.
         */
        keyix = ni->ni_ucastkey.wk_keyix;
        if (keyix == IEEE80211_KEYIX_NONE)
            keyix = HAL_TXKEYIX_INVALID;
    } else
        keyix = HAL_TXKEYIX_INVALID;

    pktlen += IEEE80211_CRC_LEN;

    /*
     * Load the DMA map so any coalescing is done.  This
     * also calculates the number of descriptors we need.
     */
#ifndef ATH_SUPERG_FF
    bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
        skb->data, pktlen, BUS_DMA_TODEVICE);
    DPRINTF(sc, ATH_DEBUG_XMIT, "%s: skb %p [data %p len %u] skbaddr %x\n",
        __func__, skb, skb->data, skb->len, bf->bf_skbaddr);
#else /* ATH_SUPERG_FF case */
    /* NB: insure skb->len had been updated for each skb so we don't need pktlen */
    {
        struct sk_buff *skbtmp = skb;
        int i=0;

        bf->bf_skbaddr = bus_map_single(sc->sc_bdev,
                                        skb->data, skb->len, BUS_DMA_TODEVICE);
        DPRINTF(sc, ATH_DEBUG_XMIT, "%s: skb%d %p [data %p len %u] skbaddr %x\n",
                __func__, i, skb, skb->data, skb->len, bf->bf_skbaddr);
        while ((skbtmp = skbtmp->next)) {
            bf->bf_skbaddrs[i++] = bus_map_single(sc->sc_bdev,
                        skbtmp->data, skbtmp->len, BUS_DMA_TODEVICE);
            DPRINTF(sc, ATH_DEBUG_XMIT, "%s: skb%d %p [data %p len %u] skbaddr %x\n",
                    __func__, i, skbtmp, skbtmp->data, skbtmp->len, bf->bf_skbaddrs[i-1]);
        }
        bf->bf_numdesc = i + 1;
    }
#endif /* ATH_SUPERG_FF */
    bf->bf_skb = skb;
    bf->bf_node = ni;

    /* setup descriptors */
    ds = bf->bf_desc;
#ifdef ATH_SUPERG_XR
    if(vap->iv_flags & IEEE80211_F_XR )
        rt = sc->sc_xr_rates;
    else
        rt = sc->sc_currates;
#else
    rt = sc->sc_currates;
#endif
    KASSERT(rt != NULL, ("no rate table, mode %u", sc->sc_curmode));

    /*
     * NB: the 802.11 layer marks whether or not we should
     * use short preamble based on the current mode and
     * negotiated parameters.
     */
    if ((ic->ic_flags & IEEE80211_F_SHPREAMBLE) &&
        (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE)) {
        shortPreamble = AH_TRUE;
        sc->sc_stats.ast_tx_shortpre++;
    } else {
        shortPreamble = AH_FALSE;
    }

    an = ATH_NODE(ni);
    flags = HAL_TXDESC_CLRDMASK;        /* XXX needed for crypto errs */
    /*
     * Calculate Atheros packet type from IEEE80211 packet header,
     * setup for rate calculations, and select h/w transmit queue.
     */
    switch (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) {
    case IEEE80211_FC0_TYPE_MGT:
        subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
        if (subtype == IEEE80211_FC0_SUBTYPE_BEACON)
            atype = HAL_PKT_TYPE_BEACON;
        else if (subtype == IEEE80211_FC0_SUBTYPE_PROBE_RESP)
            atype = HAL_PKT_TYPE_PROBE_RESP;
        else if (subtype == IEEE80211_FC0_SUBTYPE_ATIM)
            atype = HAL_PKT_TYPE_ATIM;
        else
            atype = HAL_PKT_TYPE_NORMAL;    /* XXX */
        rcs[0].rix   = sc->sc_minrateix;;
        rcs[0].tries = ATH_TXMAXTRY;
        rcs[0].flags = 0;

        if (ni->ni_flags & IEEE80211_NODE_QOS) {
            /* NB: force all management frames to highest queue */
            txq = sc->sc_ac2q[WME_AC_VO];
        } else {
            txq = sc->sc_ac2q[WME_AC_BE];
        }
        break;
    case IEEE80211_FC0_TYPE_CTL:
        atype = HAL_PKT_TYPE_PSPOLL;    /* stop setting of duration */
        rcs[0].rix   = sc->sc_minrateix;;
        rcs[0].tries = ATH_TXMAXTRY;
        rcs[0].flags = 0;

        if (ni->ni_flags & IEEE80211_NODE_QOS) {
            /* NB: force all ctl frames to highest queue */
            txq = sc->sc_ac2q[WME_AC_VO];
        } else {
            txq = sc->sc_ac2q[WME_AC_BE];
        }
        break;
    case IEEE80211_FC0_TYPE_DATA:
        atype = HAL_PKT_TYPE_NORMAL;        /* default */

        if (ismcast) {
            rcs[0].rix   = ath_tx_findindex(rt, vap->iv_mcast_rate);
            rcs[0].tries = ATH_TXMAXTRY;
            rcs[0].flags = 0;
        }
        else {

            int isProbe;
            /* Data frames; consult the rate control module */
            ath_rate_findrate(sc, an, skb->len, ATH_TXMAXTRY-1, sc->sc_mrretries, 1, 
                              ATH_RC_PROBE_ALLOWED | ATH_RC_MINRATE_LASTRATE, rcs, &isProbe);

            /* Ratecontrol sometimes returns invalid rate index */
            if (rcs[0].rix != 0xff)
                an->an_prevdatarix = rcs[0].rix;
            else
                rcs[0].rix = an->an_prevdatarix;

        }

        /*
         * Default all non-QoS traffic to the best-effort queue.
         */
        if (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_QOS) {
            /* XXX validate skb->priority, remove mask */
            txq = sc->sc_ac2q[skb->priority & 0x3];
            if (ic->ic_wme.wme_wmeChanParams.cap_wmeParams[skb->priority].wmep_noackPolicy) {
                flags |= HAL_TXDESC_NOACK;
                sc->sc_stats.ast_tx_noack++;
            }
        } else {
            txq = sc->sc_ac2q[WME_AC_BE];
        }
        break;
    default:
        printk("%s: bogus frame type 0x%x (%s)\n", dev->name,
            wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK, __func__);
        /* XXX statistic */
        return -EIO;
    }

    memcpy(bf->bf_rcs, rcs, sizeof(rcs));
    rix = rcs[0].rix;
    try0 = rcs[0].tries;
    txrate = rt->info[rix].rateCode;
    if (shortPreamble)
        txrate |= rt->info[rix].shortPreamble;
#ifdef ATH_SUPERG_XR
    if(vap->iv_flags & IEEE80211_F_XR ) {
        txq = sc->sc_xrtxq;
        if(!txq) txq = sc->sc_ac2q[WME_AC_BK];
        flags |= HAL_TXDESC_CTSENA;
        cix = rt->info[sc->sc_protrix].controlRate;
    }
#endif
    /*
     * When servicing one or more stations in power-save mode (or)
     * if there is some mcast data waiting on mcast queue
     * (to prevent out of order delivery of mcast,bcast packets)
     * multicast frames must be buffered until after the beacon.
     * We use the private mcast queue for that.
     */
    if (ismcast   && (vap->iv_ps_sta || avp->av_mcastq.axq_depth)) {
        txq = &avp->av_mcastq;
        /* XXX? more bit in 802.11 frame header */
    }

    /*
     * Calculate miscellaneous flags.
     */
    if (ismcast) {
        flags |= HAL_TXDESC_NOACK;  /* no ack on broad/multicast */
        sc->sc_stats.ast_tx_noack++;
        /* turn off multi-rate retry for multicast traffic */
        try0 = ATH_TXMAXTRY;
     } else if (pktlen > vap->iv_rtsthreshold) {
#ifdef ATH_SUPERG_FF
        /* we could refine to only check that the frame of interest
         * is a FF, but this seems inconsistent.
         */
        if( !(vap->iv_ath_cap & ni->ni_ath_flags & IEEE80211_ATHC_FF)) {
#endif
            flags |= HAL_TXDESC_RTSENA; /* RTS based on frame length */
            cix = rt->info[rix].controlRate;
            sc->sc_stats.ast_tx_rts++;
#ifdef ATH_SUPERG_FF
        }
#endif
    }

    /*
     * If 802.11g protection is enabled, determine whether
     * to use RTS/CTS or just CTS.  Note that this is only
     * done for OFDM unicast frames.
     */
    if ((ic->ic_flags & IEEE80211_F_USEPROT) &&
        rt->info[rix].phy == IEEE80211_T_OFDM &&
        (flags & HAL_TXDESC_NOACK) == 0) {
        /* XXX fragments must use CCK rates w/ protection */
        if (ic->ic_protmode == IEEE80211_PROT_RTSCTS)
            flags |= HAL_TXDESC_RTSENA;
        else if (ic->ic_protmode == IEEE80211_PROT_CTSONLY)
            flags |= HAL_TXDESC_CTSENA;

        if (istxfrag)
            /*
            **  if Tx fragment, it would be desirable to
            **  use highest CCK rate for RTS/CTS.
            **  However, stations farther away may detect it
            **  at a lower CCK rate. Therefore, use the
            **  configured protect rate, which is 2 Mbps
            **  for 11G.
            */
            cix = rt->info[sc->sc_protrix].controlRate;
        else
            cix = rt->info[sc->sc_protrix].controlRate;
        sc->sc_stats.ast_tx_protect++;
    }

    /*
     * Calculate duration.  This logically belongs in the 802.11
     * layer but it lacks sufficient information to calculate it.
     */
    if ((flags & HAL_TXDESC_NOACK) == 0 &&
        (wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_CTL) {
        u_int16_t dur;
        /*
         * XXX not right with fragmentation.
         */
        if (shortPreamble)
            dur = rt->info[rix].spAckDuration;
        else
            dur = rt->info[rix].lpAckDuration;

        if (wh->i_fc[1] & IEEE80211_FC1_MORE_FRAG) {
            dur += dur;  /* Add additional 'SIFS + ACK' */

            /*
            ** Compute size of next fragment in order to compute
            ** durations needed to update NAV.
            ** The last fragment uses the ACK duration only.
            ** Add time for next fragment.
            */
            dur += ath_hal_computetxtime(ah, rt, nextfraglen,
                             rix, shortPreamble);
        }

        if (istxfrag) {
            /*
            **  Force hardware to use computed duration for next
            **  fragment by disabling multi-rate retry, which
            **  updates duration based on the multi-rate
            **  duration table.
            */
            try0 = ATH_TXMAXTRY;
        }

        *(u_int16_t *)wh->i_dur = cpu_to_le16(dur);
    }

    /*
     * Calculate RTS/CTS rate and duration if needed.
     */
    ctsduration = 0;
    if (flags & (HAL_TXDESC_RTSENA|HAL_TXDESC_CTSENA)) {
        /*
         * CTS transmit rate is derived from the transmit rate
         * by looking in the h/w rate table.  We must also factor
         * in whether or not a short preamble is to be used.
         */
        /* NB: cix is set above where RTS/CTS is enabled */
        KASSERT(cix != 0xff, ("cix not setup"));
        ctsrate = rt->info[cix].rateCode;
        /*
         * Compute the transmit duration based on the frame
         * size and the size of an ACK frame.  We call into the
         * HAL to do the computation since it depends on the
         * characteristics of the actual PHY being used.
         *
         * NB: CTS is assumed the same size as an ACK so we can
         *     use the precalculated ACK durations.
         */
        if (shortPreamble) {
            ctsrate |= rt->info[cix].shortPreamble;
            if (flags & HAL_TXDESC_RTSENA)      /* SIFS + CTS */
                ctsduration += rt->info[cix].spAckDuration;
            ctsduration += ath_hal_computetxtime(ah,
                                 rt, pktlen, rix, AH_TRUE);
            if ((flags & HAL_TXDESC_NOACK) == 0)    /* SIFS + ACK */
                ctsduration += rt->info[cix].spAckDuration;
        } else {
            if (flags & HAL_TXDESC_RTSENA)      /* SIFS + CTS */
                ctsduration += rt->info[cix].lpAckDuration;
            ctsduration += ath_hal_computetxtime(ah,
                                 rt, pktlen, rix, AH_FALSE);
            if ((flags & HAL_TXDESC_NOACK) == 0)    /* SIFS + ACK */
                ctsduration += rt->info[cix].lpAckDuration;
        }
        /*
         * Must disable multi-rate retry when using RTS/CTS.
         */
        try0 = ATH_TXMAXTRY;
    } else
        ctsrate = 0;

    if (IFF_DUMPPKTS(sc, ATH_DEBUG_XMIT))
        /* FFXXX: need multi-skb version to dump entire FF */
        ieee80211_dump_pkt(ic, skb->data, skb->len,
                   (txrate & 0x80) ?
                   sc->sc_hthwmap[txrate & IEEE80211_RATE_VAL].ieeerate :
                   sc->sc_hwmap[txrate].ieeerate, -1);

    /*
     * Determine if a tx interrupt should be generated for
     * this descriptor.  We take a tx interrupt to reap
     * descriptors when the h/w hits an EOL condition or
     * when the descriptor is specifically marked to generate
     * an interrupt.  We periodically mark descriptors in this
     * way to insure timely replenishing of the supply needed
     * for sending frames.  Defering interrupts reduces system
     * load and potentially allows more concurrent work to be
     * done but if done to aggressively can cause senders to
     * backup.
     *
     * NB: use >= to deal with sc_txintrperiod changing
     *     dynamically through sysctl.
     */
    if (++txq->axq_intrcnt >= sc->sc_txintrperiod) {
        flags |= HAL_TXDESC_INTREQ;
        txq->axq_intrcnt = 0;
    }

#ifdef ATH_SUPERG_COMP
    if (ATH_NODE(ni)->an_decomp_index != INVALID_DECOMP_INDEX &&
        !ismcast &&
        ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_DATA)
        && ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK)
        != IEEE80211_FC0_SUBTYPE_NODATA)) {
        if (pktlen > ATH_COMP_THRESHOLD) {
            comp = ATH_COMP_PROC_COMP_OPTIMAL;
        } else {
            comp = ATH_COMP_PROC_NO_COMP_ADD_CCS;
        }
    }
#endif

#ifdef DEBUG_PKTLOG
        ds->ds_vdata = (u_int32_t)skb->data;
#endif

    /*
     * Formulate first tx descriptor with tx controls.
     */
    /* XXX check return value? */
    ath_hal_setuptxdesc(ah, ds
                , pktlen        /* packet length */
                , hdrlen        /* header length */
                , atype         /* Atheros packet type */
                , MIN(ni->ni_txpower,60)/* txpower */
                , txrate, try0      /* series 0 rate/tries */
                , keyix         /* key cache index */
                , sc->sc_txantenna  /* antenna mode */
                , flags         /* flags */
                , ctsrate       /* rts/cts rate */
                , ctsduration       /* rts/cts duration */
                , icvlen        /* comp icv len */
                , ivlen         /* comp iv len */
                , comp          /* comp scheme */
        );
    bf->bf_flags = flags;           /* record for post-processing */

    /*
     * Setup the multi-rate retry state only when we're
     * going to use it.  This assumes ath_hal_setuptxdesc
     * initializes the descriptors (so we don't have to)
     * when the hardware supports multi-rate retry and
     * we don't use it.
     */
    if (try0 != ATH_TXMAXTRY)
        ath_hal_setupxtxdesc(ah, ds,
                             rt->info[rcs[1].rix].rateCode, rcs[1].tries,
                             rt->info[rcs[2].rix].rateCode, rcs[2].tries,
                             rt->info[rcs[3].rix].rateCode, rcs[3].tries
                             );
#ifndef ATH_SUPERG_FF
    ds->ds_link = 0;
    ds->ds_data = bf->bf_skbaddr;

    ath_hal_filltxdesc(ah, ds
               , skb->len       /* segment length */
               , AH_TRUE        /* first segment */
               , AH_TRUE        /* last segment */
               , ds         /* first descriptor */
        );

    if (flags & (HAL_TXDESC_RTSENA | HAL_TXDESC_CTSENA)) {
        if (!ath_hal_updateCTSForBursting(ah, ds
             , (txq->axq_linkbuf) ? txq->axq_linkbuf->bf_desc : NULL
             , txq->axq_lastdsWithCTS
             , txq->axq_gatingds
             , IEEE80211_TXOP_TO_US(ic->ic_chanParams.cap_wmeParams[skb->priority].wmep_txopLimit)
             , ath_hal_computetxtime(ah, rt, IEEE80211_ACK_LEN, cix, AH_TRUE))) {
                ATH_TXQ_LOCK_BH(txq);
                txq->axq_lastdsWithCTS = ds;
            /* set gating Desc to final desc */
            txq->axq_gatingds = (struct ath_desc *)txq->axq_link;
            ATH_TXQ_UNLOCK_BH(txq);
        }
    }

    /* NB: The desc swap function becomes void,
     * if descriptor swapping is not enabled
     */
    ath_desc_swap(ds);

    DPRINTF(sc, ATH_DEBUG_XMIT, "%s: Q%d: %08x %08x %08x %08x %08x %08x\n",
        __func__, txq->axq_qnum, ds->ds_link, ds->ds_data,
        ds->ds_ctl0, ds->ds_ctl1, ds->ds_hw[0], ds->ds_hw[1]);
#else /* ATH_SUPERG_FF */
    {
        struct sk_buff *skbtmp = skb;
        struct ath_desc *ds0 = ds;
        int i;

        ds->ds_data = bf->bf_skbaddr;
        ds->ds_link = (skb->next == NULL) ? 0 : bf->bf_daddr + sizeof(*ds);

        ath_hal_filltxdesc(ah, ds
            , skbtmp->len       /* segment length */
            , AH_TRUE       /* first segment */
            , skbtmp->next == NULL  /* last segment */
            , ds            /* first descriptor */
        );
        if (flags & (HAL_TXDESC_RTSENA | HAL_TXDESC_CTSENA)) {
            if (!ath_hal_updateCTSForBursting(ah, ds
                 , (txq->axq_linkbuf) ? txq->axq_linkbuf->bf_desc : NULL
                 , txq->axq_lastdsWithCTS
                 , txq->axq_gatingds
                 , IEEE80211_TXOP_TO_US(ic->ic_wme.wme_chanParams.cap_wmeParams[skb->priority].wmep_txopLimit)
                 , ath_hal_computetxtime(ah, rt, IEEE80211_ACK_LEN, cix, AH_TRUE))) {
                    ATH_TXQ_LOCK_BH(txq);
                    txq->axq_lastdsWithCTS = ds;
                /* set gating Desc to final desc */
                    txq->axq_gatingds = (struct ath_desc *)txq->axq_link;
                ATH_TXQ_UNLOCK_BH(txq);
            }
        }

        /* NB: The desc swap function becomes void,
         * if descriptor swapping is not enabled
         */
        ath_desc_swap(ds);

        DPRINTF(sc, ATH_DEBUG_XMIT, "%s: Q%d: %08x %08x %08x %08x %08x %08x\n",
            __func__, txq->axq_qnum, ds->ds_link, ds->ds_data,
            ds->ds_ctl0, ds->ds_ctl1, ds->ds_hw[0], ds->ds_hw[1]);
        for (i=0, skbtmp = skbtmp->next; i<bf->bf_numdesc - 1; i++, skbtmp = skbtmp->next) {
            ds++;
            ds->ds_link = skbtmp->next == NULL ? 0 : bf->bf_daddr + sizeof(*ds)*(i+2);
            ds->ds_data = bf->bf_skbaddrs[i];
            ath_hal_filltxdesc(ah, ds
                , skbtmp->len       /* segment length */
                , AH_FALSE      /* first segment */
                , skbtmp->next == NULL  /* last segment */
                , ds0           /* first descriptor */
            );

            /* NB: The desc swap function becomes void,
             * if descriptor swapping is not enabled
             */
            ath_desc_swap(ds);

            DPRINTF(sc, ATH_DEBUG_XMIT, "%s: Q%d: %08x %08x %08x %08x %08x %08x\n",
                __func__, txq->axq_qnum, ds->ds_link, ds->ds_data,
                ds->ds_ctl0, ds->ds_ctl1, ds->ds_hw[0], ds->ds_hw[1]);
        }
    }
#endif

    series[0].Tries = try0;
    series[0].Rate = txrate;
    series[0].ChSel = sc->sc_ic.ic_tx_chainmask;
    series[0].RateFlags = (ctsrate) ? HAL_RATESERIES_RTS_CTS : 0;
    ath_hal_set11n_ratescenario(ah, ds, 0, ctsrate, series, 4, 0);

    ath_tx_txqaddbuf(sc, ni, txq, bf, ds, pktlen);

    return 0;
#undef MIN
}

/*
 * Process completed xmit descriptors from the specified queue.
 */
static void
ath_tx_processq(struct ath_softc *sc, struct ath_txq *txq)
{
    struct ath_hal *ah = sc->sc_ah;
    struct ath_buf *bf;
    struct ath_desc *ds;
    struct ieee80211_node *ni;
    struct ath_node *an;
    int sr, lr;
    HAL_STATUS status;

    DPRINTF(sc, ATH_DEBUG_TX_PROC, "%s: tx queue %p, link %p\n", __func__,
        (caddr_t) ath_hal_gettxbuf(sc->sc_ah, txq->axq_qnum),
        txq->axq_link);

    for (;;) {
        ATH_TXQ_LOCK(txq);
        txq->axq_intrcnt = 0; /* reset periodic desc intr count */
        bf = TAILQ_FIRST(&txq->axq_q);
        if (bf == NULL) {
            txq->axq_link = NULL;
            txq->axq_linkbuf = NULL;
            ATH_TXQ_UNLOCK(txq);
            break;
        }

#ifdef ATH_SUPERG_FF
        ds = &bf->bf_desc[bf->bf_numdesc - 1];
        DPRINTF(sc, ATH_DEBUG_TX_PROC, "%s: frame's last desc: %p\n", __func__, ds);
#else
        ds = bf->bf_desc;       /* NB: last decriptor */
#endif
        status = ath_hal_txprocdesc(ah, ds);
#ifdef AR_DEBUG
        if (sc->sc_debug & ATH_DEBUG_XMIT_DESC)
            ath_printtxbuf(bf, status == HAL_OK);
#endif
        if (status == HAL_EINPROGRESS) {
            ATH_TXQ_UNLOCK(txq);
            break;
        }
        if (bf->bf_desc == txq->axq_lastdsWithCTS) {
            txq->axq_lastdsWithCTS = NULL;
        }
        if (ds == txq->axq_gatingds) {
            txq->axq_gatingds = NULL;
        }

        ATH_TXQ_REMOVE_HEAD(txq, bf, bf_list);
        ATH_TXQ_UNLOCK(txq);

        ni = bf->bf_node;
        if (ni != NULL) {
            an = ATH_NODE(ni);
            if (ds->ds_txstat.ts_status == 0) {
                u_int8_t txant = ds->ds_txstat.ts_antenna;
                sc->sc_stats.ast_ant_tx[txant]++;
                sc->sc_ant_tx[txant]++;
#ifdef ATH_SUPERG_FF
            if (bf->bf_numdesc>1)
                ni->ni_vap->iv_stats.is_tx_ffokcnt++;
#endif
                if (ds->ds_txstat.ts_rate & HAL_TXSTAT_ALTRATE)
                    sc->sc_stats.ast_tx_altrate++;
                sc->sc_stats.ast_tx_rssi_combined =
                    ds->ds_txstat.ts_rssi;
                ATH_RSSI_LPF(an->an_halstats.ns_avgtxrssi,
                    ds->ds_txstat.ts_rssi);
                if (bf->bf_skb->priority == WME_AC_VO ||
                    bf->bf_skb->priority == WME_AC_VI)
                    ni->ni_ic->ic_wme.wme_hipri_traffic++;
                ni->ni_inact = ni->ni_inact_reload;
            } else {
#ifdef ATH_SUPERG_FF
                if (bf->bf_numdesc>1)
                    ni->ni_vap->iv_stats.is_tx_fferrcnt++;
#endif
                if (ds->ds_txstat.ts_status & HAL_TXERR_XRETRY)
                    sc->sc_stats.ast_tx_xretries++;
                if (ds->ds_txstat.ts_status & HAL_TXERR_FIFO)
                    sc->sc_stats.ast_tx_fifoerr++;
                if (ds->ds_txstat.ts_status & HAL_TXERR_FILT)
                    sc->sc_stats.ast_tx_filtered++;
            }
            sr = ds->ds_txstat.ts_shortretry;
            lr = ds->ds_txstat.ts_longretry;
            sc->sc_stats.ast_tx_shortretry += sr;
            sc->sc_stats.ast_tx_longretry += lr;
            /*
             * Hand the descriptor to the rate control algorithm
             * if the frame wasn't dropped for filtering or sent
             * w/o waiting for an ack.  In those cases the rssi
             * and retry counts will be meaningless.
             */
            if ((ds->ds_txstat.ts_status & HAL_TXERR_FILT) == 0 &&
                (bf->bf_flags & HAL_TXDESC_NOACK) == 0)
                ath_rate_tx_complete(sc, an, ds, bf->bf_rcs, 1, 0);
            /*
             * Reclaim reference to node.
             *
             * NB: the node may be reclaimed here if, for example
             *     this is a DEAUTH message that was sent and the
             *     node was timed out due to inactivity.
             */
            ieee80211_free_node(ni);
        }
#ifdef ATH_SUPERG_FF
        {
            struct sk_buff *skbfree, *skb = bf->bf_skb;
            int i;

            bus_unmap_single(sc->sc_bdev,
                             bf->bf_skbaddr, skb->len, BUS_DMA_TODEVICE);
            skbfree = skb;
            skb = skb->next;
            DPRINTF(sc, ATH_DEBUG_TX_PROC, "%s: free skb %p\n", __func__, skbfree);
            dev_kfree_skb(skbfree);
            for (i=1; i<bf->bf_numdesc; i++) {
                bus_unmap_single(sc->sc_bdev, bf->bf_skbaddrs[i-1], bf->bf_skb->len, BUS_DMA_TODEVICE);
                skbfree = skb;
                skb = skb->next;
                DPRINTF(sc, ATH_DEBUG_TX_PROC, "%s: free skb %p\n", __func__, skbfree);
                dev_kfree_skb(skbfree);
            }
        }
        bf->bf_numdesc = 0;
#else
        bus_unmap_single(sc->sc_bdev,
            bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);
        DPRINTF(sc, ATH_DEBUG_TX_PROC, "%s: free skb %p\n", __func__, bf->bf_skb);
        dev_kfree_skb(bf->bf_skb);
#endif
        bf->bf_skb = NULL;
        bf->bf_node = NULL;

        ATH_TXBUF_LOCK(sc);
        TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf, bf_list);
        if(sc->sc_devstopped) {
            ++sc->sc_reapcount;
            if(sc->sc_reapcount > ATH_TXBUF_FREE_THRESHOLD) {
                netif_start_queue(sc->sc_dev);
                sc->sc_reapcount=0;
                sc->sc_devstopped=0;
            } else {
                ATH_SCHEDULE_TQUEUE(&sc->sc_txtq, NULL);
            }
        }
        ATH_TXBUF_UNLOCK(sc);
    }
#ifdef ATH_SUPERG_FF
    /* flush ff staging queue if buffer low */
    if (txq->axq_depth <= sc->sc_fftxqmin - 1) {
        /* NB: consider only flushing a preset number based on age. */
        ath_ffstageq_flush(sc, txq, ath_ff_neverflushtestdone);
    }
#endif /* ATH_SUPERG_FF */
}

int
txqactive(struct ath_hal *ah, int qnum)
{
    u_int32_t txqs = 1<<qnum;
    ath_hal_gettxintrtxqs(ah, &txqs);
    return (txqs & (1<<qnum));
}

/*
 * Deferred processing of transmit interrupt; special-cased
 * for a single hardware transmit queue (e.g. 5210 and 5211).
 */
static void
ath_tx_tasklet_q0(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;
    struct ath_softc *sc = dev->priv;

    if (txqactive(sc->sc_ah, 0))
        ath_tx_processq(sc, &sc->sc_txq[0]);
    if (txqactive(sc->sc_ah, sc->sc_cabq->axq_qnum))
        ath_tx_processq(sc, sc->sc_cabq);

    netif_wake_queue(dev);

    if (sc->sc_softled)
        ath_led_event(sc, ATH_LED_TX);
}

/*
 * Deferred processing of transmit interrupt; special-cased
 * for four hardware queues, 0-3 (e.g. 5212 w/ WME support).
 */
static void
ath_tx_tasklet_q0123(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;
    struct ath_softc *sc = dev->priv;

    /*
     * Process each active queue.
     */
    if (txqactive(sc->sc_ah, 0))
        ath_tx_processq(sc, &sc->sc_txq[0]);
    if (txqactive(sc->sc_ah, 1))
        ath_tx_processq(sc, &sc->sc_txq[1]);
    if (txqactive(sc->sc_ah, 2))
        ath_tx_processq(sc, &sc->sc_txq[2]);
    if (txqactive(sc->sc_ah, 3))
        ath_tx_processq(sc, &sc->sc_txq[3]);
    if (txqactive(sc->sc_ah, sc->sc_cabq->axq_qnum))
        ath_tx_processq(sc, sc->sc_cabq);
#ifdef ATH_SUPERG_XR
    if (sc->sc_xrtxq && txqactive(sc->sc_ah, sc->sc_xrtxq->axq_qnum))
        ath_tx_processq(sc, sc->sc_xrtxq);
#endif

    netif_wake_queue(dev);

    if (sc->sc_softled)
        ath_led_event(sc, ATH_LED_TX);
}

/*
 * Deferred processing of transmit interrupt.
 */
static void
ath_tx_tasklet(TQUEUE_ARG data)
{
    struct net_device *dev = (struct net_device *)data;
    struct ath_softc *sc = dev->priv;
    int i;

    /*
     * Process each active queue.
     */
    for (i = 0; i < HAL_NUM_TX_QUEUES; i++)
        if (ATH_TXQ_SETUP(sc, i) && txqactive(sc->sc_ah, i))
            ath_tx_processq(sc, &sc->sc_txq[i]);

#ifdef ATH_SUPERG_XR
    if (sc->sc_xrtxq && txqactive(sc->sc_ah, sc->sc_xrtxq->axq_qnum))
        ath_tx_processq(sc, sc->sc_xrtxq);
#endif

    netif_wake_queue(dev);

    if (sc->sc_softled)
        ath_led_event(sc, ATH_LED_TX);
}

static void
ath_watchdog(struct net_device *dev)
{
    if (!ath_noreset) {
        printk("%s: ath_watchdog: hardware error; reseting\n", dev->name);
        ath_resetinternal(dev, RESET_FATAL);
    }
}

static void
ath_tx_timeout(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;

    DPRINTF(sc, ATH_DEBUG_WATCHDOG, "%s: %sRUNNING %svalid\n",
        __func__, (dev->flags & IFF_RUNNING) ? "" : "!",
        sc->sc_invalid ? "in" : "");

    if ((dev->flags & IFF_RUNNING) && !sc->sc_invalid) {
        sc->sc_stats.ast_watchdog++;
        ath_watchdog(dev);
    }
}

static void
ath_tx_draintxq(struct ath_softc *sc, struct ath_txq *txq)
{
#ifdef OWLAGGR
    owl_txq_drain(sc, txq);
#else
    struct ath_hal *ah = sc->sc_ah;
    struct ath_buf *bf;

    sc->sc_stats.ast_draintxq ++;

    /*
     * NB: this assumes output has been stopped and
     *     we do not need to block ath_tx_tasklet
     */
    for (;;) {
        ATH_TXQ_LOCK(txq);
        bf = TAILQ_FIRST(&txq->axq_q);
        if (bf == NULL) {
            txq->axq_link = NULL;
            txq->axq_linkbuf = NULL;
            ATH_TXQ_UNLOCK(txq);
            break;
        }
        ATH_TXQ_REMOVE_HEAD(txq, bf, bf_list);
        ATH_TXQ_UNLOCK(txq);
#ifdef AR_DEBUG
        if (sc->sc_debug & ATH_DEBUG_RESET)
            ath_printtxbuf(bf,
                ath_hal_txprocdesc(ah, bf->bf_desc) == HAL_OK);
#endif /* AR_DEBUG */
        bus_unmap_single(sc->sc_bdev,
            bf->bf_skbaddr, bf->bf_skb->len, BUS_DMA_TODEVICE);
        dev_kfree_skb(bf->bf_skb);
        ieee80211_free_node(bf->bf_node);

        bf->bf_skb = NULL;
        bf->bf_node = NULL;

        ATH_TXBUF_LOCK(sc);
        TAILQ_INSERT_TAIL(&sc->sc_txbuf, bf, bf_list);
        ATH_TXBUF_UNLOCK(sc);
    }
#endif
}

static void
ath_tx_stopdma(struct ath_softc *sc, struct ath_txq *txq)
{
    struct ath_hal *ah = sc->sc_ah;

    sc->sc_stats.ast_stopdma ++;

    (void) ath_hal_stoptxdma(ah, txq->axq_qnum);
    DPRINTF(sc, ATH_DEBUG_RESET, "%s: tx queue [%u] %p, link %p\n",
        __func__, txq->axq_qnum,
        (caddr_t) ath_hal_gettxbuf(ah, txq->axq_qnum), txq->axq_link);
}

/*
 * Drain the transmit queues and reclaim resources.
 */
static void
ath_draintxq(struct ath_softc *sc)
{
    struct ath_hal *ah = sc->sc_ah;
    int i;

    /* XXX return value */
    if (!sc->sc_invalid) {
        (void) ath_hal_stoptxdma(ah, sc->sc_bhalq);
        DPRINTF(sc, ATH_DEBUG_RESET, "%s: beacon queue %p\n", __func__,
            (caddr_t) ath_hal_gettxbuf(ah, sc->sc_bhalq));
        for (i = 0; i < HAL_NUM_TX_QUEUES; i++)
            if (ATH_TXQ_SETUP(sc, i))
                ath_tx_stopdma(sc, &sc->sc_txq[i]);
    }
    sc->sc_dev->trans_start = jiffies;
    netif_start_queue(sc->sc_dev);      /* XXX move to callers */
    for (i = 0; i < HAL_NUM_TX_QUEUES; i++)
        if (ATH_TXQ_SETUP(sc, i))
            ath_tx_draintxq(sc, &sc->sc_txq[i]);
}

/*
 * Abort the transmit queues and reclaim resources.
 */
static HAL_BOOL
ath_aborttxq(struct ath_softc *sc)
{
    int i;

    if (sc->sc_invalid)
        return AH_TRUE;

    if (!ath_hal_aborttxdma(sc->sc_ah))
        return AH_FALSE;

    sc->sc_dev->trans_start = jiffies;
    netif_start_queue(sc->sc_dev);

    for (i = 0; i < HAL_NUM_TX_QUEUES; i++) {
        if (ATH_TXQ_SETUP(sc, i))
            ath_tx_draintxq(sc, &sc->sc_txq[i]);
    }

    return AH_TRUE;
}

/*
 * Disable the receive h/w in preparation for a reset.
 */
void
ath_stoprecv(struct ath_softc *sc)
{
    struct ath_rx_status rx_stats;
#define PA2DESC(_sc, _pa) \
    ((struct ath_desc *)((caddr_t)(_sc)->sc_rxdma.dd_desc + \
        ((_pa) - (_sc)->sc_rxdma.dd_desc_paddr)))
    struct ath_hal *ah = sc->sc_ah;

    sc->sc_stats.ast_stoprecv ++;

    ath_hal_stoppcurecv(ah);    /* disable PCU */
    ath_hal_setrxfilter(ah, 0); /* clear recv filter */
    ath_hal_stopdmarecv(ah);    /* disable DMA engine */

#ifdef AR_DEBUG
    if (sc->sc_debug & (ATH_DEBUG_RESET | ATH_DEBUG_FATAL)) {
        struct ath_buf *bf;

        printk("ath_stoprecv: rx queue %p, link %p\n",
            (caddr_t) ath_hal_getrxbuf(ah), sc->sc_rxlink);
        TAILQ_FOREACH(bf, &sc->sc_rxbuf, bf_list) {
            struct ath_desc *ds = bf->bf_desc;
            HAL_STATUS status = ath_hal_rxprocdescfast(ah, ds,
                bf->bf_daddr, PA2DESC(sc, ds->ds_link), &rx_stats);
            if (status == HAL_OK || (sc->sc_debug & ATH_DEBUG_FATAL))
                ath_printrxbuf(bf, status == HAL_OK);
        }
    }
#endif
    sc->sc_rxlink = NULL;       /* just in case */
#undef PA2DESC
}

/*
 * Abort the receive h/w for a fast channel change
 */
HAL_BOOL
ath_abortrecv(struct ath_softc *sc)
{
    struct ath_hal *ah = sc->sc_ah;

    ath_hal_abortpcurecv(ah);   /* Abort Rx */
    ath_hal_setrxfilter(ah, 0); /* clear recv filter */
    /* disable DMA engine */
    if (!ath_hal_stopdmarecv(ah))
        return AH_FALSE;

    sc->sc_rxlink = NULL;
    return AH_TRUE;
}

/*
 * Enable the receive h/w following a reset.
 */
int
ath_startrecv(struct ath_softc *sc)
{
    struct ath_hal *ah = sc->sc_ah;
    struct net_device *dev = sc->sc_dev;
    struct ath_buf *bf;

    /*
     * Cisco's VPN software requires that drivers be able to
     * receive encapsulated frames that are larger than the MTU.
     * Since we can't be sure how large a frame we'll get, setup
     * to handle the larges on possible.
     */
#ifdef ATH_SUPERG_FF
    sc->sc_rxbufsize = roundup(ATH_FF_MAX_LEN, sc->sc_cachelsz);
#else
    sc->sc_rxbufsize = roundup(IEEE80211_MAX_LEN, sc->sc_cachelsz);
#endif
#ifdef CES_DEMO
    sc->sc_rxbufsize = roundup(1024+512+128, sc->sc_cachelsz);
#endif

    DPRINTF(sc,ATH_DEBUG_RESET, "%s: mtu %u cachelsz %u rxbufsize %u\n",
        __func__, dev->mtu, sc->sc_cachelsz, sc->sc_rxbufsize);

    sc->sc_rxlink = NULL;
    sc->sc_rxbuf_held = NULL;
    TAILQ_FOREACH(bf, &sc->sc_rxbuf, bf_list) {
        int error = ath_rxbuf_init(sc, bf);
        if (error != 0)
            return error;
    }

    bf = TAILQ_FIRST(&sc->sc_rxbuf);
    ath_hal_putrxbuf(ah, bf->bf_daddr);
    ath_hal_rxena(ah);      /* enable recv descriptors */
    ath_mode_init(dev);     /* set filters, etc. */
    ath_hal_startpcurecv(ah);   /* re-enable PCU/DMA engine */
    return 0;
}

/*
 * Flush skb's allocate for receive.
 */
static void
ath_flushrecv(struct ath_softc *sc)
{
    struct ath_buf *bf;

    sc->sc_stats.ast_flushrecv ++;

    TAILQ_FOREACH(bf, &sc->sc_rxbuf, bf_list)
        if (bf->bf_skb != NULL) {
            bus_unmap_single(sc->sc_bdev,
                bf->bf_skbaddr, sc->sc_rxbufsize,
                BUS_DMA_FROMDEVICE);
            dev_kfree_skb(bf->bf_skb);
            bf->bf_skb = NULL;
        }
}

/*
 * Update internal state after a channel change.
 */
static void
ath_chan_change(struct ath_softc *sc, struct ieee80211_channel *chan)
{
    struct ieee80211com *ic = &sc->sc_ic;
    struct net_device *dev = sc->sc_dev;
    enum ieee80211_phymode mode;

    sc->sc_stats.ast_chanchange ++;

    mode = ieee80211_chan2mode(chan);
    ath_rate_setup(dev, mode);
    ath_setcurmode(sc, mode);

#ifdef notyet
    /*
     * Update BPF state.
     */
    sc->sc_tx_th.wt_chan_freq = sc->sc_rx_th.wr_chan_freq =
        htole16(chan->ic_freq);
    sc->sc_tx_th.wt_chan_flags = sc->sc_rx_th.wr_chan_flags =
        htole16(chan->ic_flags);
#endif
    if (ic->ic_curchanmaxpwr == 0) {
        ic->ic_curchanmaxpwr = chan->ic_maxregpower;
    }
}


/*
 * Set/change channels.  If the channel is really being changed,
 * it's done by reseting the chip.  To accomplish this we must
 * first cleanup any pending DMA, then restart stuff after a la
 * ath_init.
 */
int
ath_chan_set(struct ath_softc *sc, struct ieee80211_channel *chan)
{
    struct ath_hal *ah = sc->sc_ah;
    struct ieee80211com *ic = &sc->sc_ic;
    struct net_device *dev = sc->sc_dev;
    HAL_CHANNEL hchan;
    HAL_HT ht;
    int cwmchange = 0;

    sc->sc_stats.ast_chanset ++;

    /* Check for CWM mode change */
    if (ath_hal_htsupported(ah)) {
            ath_get_hwstate(sc, &ht);
        if (sc->sc_currht.cwm.ht_phymode != ht.cwm.ht_phymode ||
        sc->sc_currht.cwm.ht_extoff != ht.cwm.ht_extoff ||
        sc->sc_currht.cwm.ht_extprotspacing != ht.cwm.ht_extprotspacing) {
            cwmchange = 1;
        }
    }

    /*
     * Convert to a HAL channel description with
     * the flags constrained to reflect the current
     * operating mode.
     */
    hchan.channel = chan->ic_freq;
    hchan.channelFlags = ath_chan2flags(chan);
    KASSERT(hchan.channel != 0,
        ("bogus channel %u/0x%x", hchan.channel, hchan.channelFlags));

    DPRINTF(sc, ATH_DEBUG_RESET, "%s: %u (%u MHz) -> %u (%u MHz)\n",
        __func__,
        ath_hal_mhz2ieee(ah, sc->sc_curchan.channel,
        sc->sc_curchan.channelFlags),
            sc->sc_curchan.channel,
        ath_hal_mhz2ieee(ah, hchan.channel, hchan.channelFlags), hchan.channel);
#ifdef CES_DEMO
    if (1) { // force channel set for edge channels
#else
    if (hchan.channel != sc->sc_curchan.channel ||
        hchan.channelFlags != sc->sc_curchan.channelFlags || cwmchange) {
#endif
        HAL_STATUS status, s1, s2;

        /*
         * To switch channels clear any pending DMA operations;
         * wait long enough for the RX fifo to drain, reset the
         * hardware at the new frequency, and then re-enable
         * the relevant bits of the h/w.
         */
        ath_hal_intrset(ah, 0);     /* disable interrupts */
        if (ath_fastcc_check(sc, chan)) {
            s1 = ath_aborttxq(sc);  /* abort pending tx frames */
            s2 = ath_abortrecv(sc); /* abort cur frame + disable frame recv */
            if (s1 && s2)
                sc->sc_stats.ast_fastcc++;
            else
                sc->sc_stats.ast_fastcc_errs++;
        } else {
            ath_draintxq(sc);  /* clear pending tx frames */
            ath_stoprecv(sc);      /* turn off frame recv */
        }

        /* Set coverage class */
        if (sc->sc_scanning || !IEEE80211_IS_CHAN_A(chan) || !IEEE80211_IS_CHAN_11NA(chan)) {
            ath_hal_setcoverageclass(sc->sc_ah, 0, 0);
        } else {
            ath_hal_setcoverageclass(sc->sc_ah,
                        ic->ic_coverageclass, 0);
        }

        if (ath_hal_htsupported(ah)) {
            ath_get_hwstate(sc, &ht);
            if (!ath_hal_reset11n(ah, ic->ic_opmode, &hchan,
                           &ht, AH_TRUE, &status)) {
                   printk("%s: %s: unable to reset channel %u (%uMhz) "
                      "flags 0x%x hal status %u "
                      "(ht.cwm.mac %d, ht.cwm.phy %d, ht.cwm.extoffset %d\n",
                    dev->name, __func__,
                    ieee80211_chan2ieee(ic, chan), chan->ic_freq,
                    hchan.channelFlags, status,
                        ht.cwm.ht_macmode, ht.cwm.ht_phymode, ht.cwm.ht_extoff);
                return EIO;
            }
        sc->sc_currht = ht;
        } else {
            if (!ath_hal_reset(ah, ic->ic_opmode, &hchan, AH_TRUE, &status)) {
                printk("%s: %s: unable to reset channel %u (%uMhz) "
                       "flags 0x%x hal status %u\n",
                    dev->name, __func__,
                    ieee80211_chan2ieee(ic, chan), chan->ic_freq,
                    hchan.channelFlags, status);
                return EIO;
            }
        }
        sc->sc_curchan = hchan;
        ath_update_txpow(sc);       /* update tx power state */

        /*
         * Re-enable rx framework.
         */
        if (ath_startrecv(sc) != 0) {
            printk("%s: %s: unable to restart recv logic\n",
                dev->name, __func__);
            return EIO;
        }

        /*
         * Change channels and update the h/w rate map
         * if we're switching; e.g. 11a to 11b/g.
         */
        ath_chan_change(sc, chan);

        /*
         * Re-enable interrupts.
         */
        ath_hal_intrset(ah, sc->sc_imask);
    }
    return 0;
}

/*
 * Periodically recalibrate the PHY to account
 * for temperature/environment changes.
 */
static void
ath_calibrate(unsigned long arg)
{
#ifndef AR5416_EMULATION
    struct net_device *dev = (struct net_device *) arg;
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;

    sc->sc_stats.ast_per_cal++;

    DPRINTF(sc, ATH_DEBUG_CALIBRATE, "%s: channel %u/%x\n",
        __func__, sc->sc_curchan.channel, sc->sc_curchan.channelFlags);

    if (ath_hal_getrfgain(ah) == HAL_RFGAIN_NEED_CHANGE) {
        /*
         * Rfgain is out of bounds, reset the chip
         * to load new gain values.
         */
        sc->sc_stats.ast_per_rfgain++;
	ath_reset(dev);
    }
    if (!ath_hal_calibrate(ah, &sc->sc_curchan)) {
        DPRINTF(sc, ATH_DEBUG_ANY,
            "%s: calibration of channel %u failed\n",
            __func__, sc->sc_curchan.channel);
        sc->sc_stats.ast_per_calfail++;
    }
    sc->sc_cal_ch.expires = jiffies + (ath_calinterval * HZ);
    add_timer(&sc->sc_cal_ch);
#endif
}

static void
ath_scan_start(struct ieee80211com *ic)
{
    struct net_device *dev = ic->ic_dev;
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    u_int32_t rfilt;

    /* XXX calibration timer? */

    sc->sc_scanning = 1;
    sc->sc_syncbeacon = 0;
    rfilt = ath_calcrxfilter(sc);
    ath_hal_setrxfilter(ah, rfilt);
    ath_hal_setassocid(ah, dev->broadcast, 0);

    DPRINTF(sc, ATH_DEBUG_STATE, "%s: RX filter 0x%x bssid %s aid 0\n",
         __func__, rfilt, ether_sprintf(dev->broadcast));
}

static void
ath_scan_end(struct ieee80211com *ic)
{
    struct net_device *dev = ic->ic_dev;
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    u_int32_t rfilt;

    sc->sc_scanning = 0;
    rfilt = ath_calcrxfilter(sc);
    ath_hal_setrxfilter(ah, rfilt);
    ath_hal_setassocid(ah, sc->sc_curbssid, sc->sc_curaid);

    DPRINTF(sc, ATH_DEBUG_STATE, "%s: RX filter 0x%x bssid %s aid 0x%x\n",
         __func__, rfilt, ether_sprintf(sc->sc_curbssid),
         sc->sc_curaid);
}

static void
ath_set_channel(struct ieee80211com *ic)
{
    struct net_device *dev = ic->ic_dev;
    struct ath_softc *sc = dev->priv;

    (void) ath_chan_set(sc, ic->ic_curchan);
    /*
     * If we are returning to our bss channel then mark state
     * so the next recv'd beacon's tsf will be used to sync the
     * beacon timers.  Note that since we only hear beacons in
     * sta/ibss mode this has no effect in other operating modes.
     */
    if (!sc->sc_scanning && ic->ic_curchan == ic->ic_bsschan)
        sc->sc_syncbeacon = 1;
}

static void
ath_set_coverageclass(struct ieee80211com *ic)
{
    struct ath_softc *sc = ic->ic_dev->priv;

    ath_hal_setcoverageclass(sc->sc_ah, ic->ic_coverageclass, 0);

    return;
}

static u_int
ath_mhz2ieee(struct ieee80211com *ic, u_int freq, u_int flags)
{
    struct ath_softc *sc = ic->ic_dev->priv;

    return (ath_hal_mhz2ieee(sc->sc_ah, freq, flags));
}

static int
ath_newstate(struct ieee80211vap *vap, enum ieee80211_state nstate, int arg)
{
    struct ath_vap *avp = ATH_VAP(vap);
    struct ieee80211com *ic = vap->iv_ic;
    struct net_device *dev = ic->ic_dev;
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    struct ieee80211_node *ni;
    int i, error, stamode;
    u_int32_t rfilt=0;
    static const HAL_LED_STATE leds[] = {
        HAL_LED_INIT,   /* IEEE80211_S_INIT */
        HAL_LED_SCAN,   /* IEEE80211_S_SCAN */
        HAL_LED_AUTH,   /* IEEE80211_S_AUTH */
        HAL_LED_ASSOC,  /* IEEE80211_S_ASSOC */
        HAL_LED_RUN,    /* IEEE80211_S_RUN */
    };

    DPRINTF(sc, ATH_DEBUG_STATE, "%s: %s: %s -> %s\n", __func__, dev->name,
        ieee80211_state_name[vap->iv_state],
        ieee80211_state_name[nstate]);

    del_timer(&sc->sc_cal_ch);      /* periodic calibration timer */
    ath_hal_setledstate(ah, leds[nstate]);  /* set LED */
    netif_stop_queue(dev);          /* before we do anything else */

    if (nstate == IEEE80211_S_INIT) {
        struct ieee80211vap *tmpvap;
        /*
         * if there is no vap left in RUN state
         * disable beacon interrupts.
         */
        TAILQ_FOREACH(tmpvap, &ic->ic_vaps, iv_next) {
            if (tmpvap != vap && tmpvap->iv_state == IEEE80211_S_RUN ) {
                break;
            }
        }
        if(!tmpvap) {
            sc->sc_imask &= ~(HAL_INT_SWBA | HAL_INT_BMISS);
            /*
             * Disable interrupts.
             */
            ath_hal_intrset(ah, sc->sc_imask &~ HAL_INT_GLOBAL);
            sc->sc_beacons=0;
        }
        /*
         * Notify the rate control algorithm.
         */
        ath_rate_newstate(vap, nstate);

#ifdef ATH_CWM
        /*
         * Notify CWM.
         */
        ath_cwm_newstate(vap, nstate);
#endif
        goto done;
    }
    ni = vap->iv_bss;

    rfilt = ath_calcrxfilter(sc);
    stamode = (vap->iv_opmode == IEEE80211_M_STA ||
           vap->iv_opmode == IEEE80211_M_IBSS ||
           vap->iv_opmode == IEEE80211_M_AHDEMO);
    if (stamode && nstate == IEEE80211_S_RUN) {
        sc->sc_curaid = ni->ni_associd;
        IEEE80211_ADDR_COPY(sc->sc_curbssid, ni->ni_bssid);
    } else
        sc->sc_curaid = 0;

    DPRINTF(sc, ATH_DEBUG_STATE, "%s: RX filter 0x%x bssid %s aid 0x%x\n",
         __func__, rfilt, ether_sprintf(sc->sc_curbssid),
         sc->sc_curaid);

    ath_hal_setrxfilter(ah, rfilt);
    if (stamode)
        ath_hal_setassocid(ah, sc->sc_curbssid, sc->sc_curaid);

    if ((vap->iv_opmode != IEEE80211_M_STA) &&
         (vap->iv_flags & IEEE80211_F_PRIVACY)) {
        for (i = 0; i < IEEE80211_WEP_NKID; i++)
            if (ath_hal_keyisvalid(ah, i))
                ath_hal_keysetmac(ah, i, ni->ni_bssid);
    }

    /*
     * Notify the rate control algorithm so rates
     * are setup should ath_beacon_alloc be called.
     */
    ath_rate_newstate(vap, nstate);

#ifdef ATH_CWM
    /*
     * Notify CWM.
     */
    ath_cwm_newstate(vap, nstate);
#endif

    if (vap->iv_opmode == IEEE80211_M_MONITOR) {
        /* nothing to do */;
    } else if (nstate == IEEE80211_S_RUN) {
        DPRINTF(sc, ATH_DEBUG_STATE,
            "%s(RUN): ic_flags=0x%08x iv=%d bssid=%s "
            "capinfo=0x%04x chan=%d\n"
             , __func__
             , vap->iv_flags
             , ni->ni_intval
             , ether_sprintf(ni->ni_bssid)
             , ni->ni_capinfo
             , ieee80211_chan2ieee(ic, ni->ni_chan));

        switch (vap->iv_opmode) {
        case IEEE80211_M_HOSTAP:
        case IEEE80211_M_IBSS:
            /*
             * Allocate and setup the beacon frame.
             *
             * Stop any previous beacon DMA.  This may be
             * necessary, for example, when an ibss merge
             * causes reconfiguration; there will be a state
             * transition from RUN->RUN that means we may
             * be called with beacon transmission active.
             */
            ath_hal_stoptxdma(ah, sc->sc_bhalq);

                /* Set default key index for static wep case */
            ni->ni_ath_defkeyindex = IEEE80211_INVAL_DEFKEY;
            if (((vap->iv_flags & IEEE80211_F_WPA) == 0) &&
                (ni->ni_authmode != IEEE80211_AUTH_8021X) &&
                (vap->iv_def_txkey != IEEE80211_KEYIX_NONE)) {
                            ni->ni_ath_defkeyindex = vap->iv_def_txkey;
            }

            error = ath_beacon_alloc(sc, ni);
            /*
             * if the turbo flags have changed, then beacon and turbo
             * need to be reconfigured.
             */
            if((sc->sc_dturbo && !(vap->iv_ath_cap & IEEE80211_ATHC_TURBOP)) ||
                (!sc->sc_dturbo && (vap->iv_ath_cap & IEEE80211_ATHC_TURBOP)))
                sc->sc_beacons = 0;
            if (error != 0)
                goto bad;
            break;
        case IEEE80211_M_STA:
#ifdef ATH_SUPERG_COMP
            /* have we negotiated compression? */
                if (!(vap->iv_ath_cap & ni->ni_ath_flags &
                    IEEE80211_NODE_COMP)) {
                ni->ni_ath_flags &= ~IEEE80211_NODE_COMP;
            }
#endif
            /*
             * Allocate a key cache slot to the station.
             */
            ath_setup_keycacheslot(sc, ni);
            /*
             * Record negotiated dynamic turbo state for
             * use by rate control modules.
             */
            sc->sc_dturbo =
                (ni->ni_ath_flags & IEEE80211_ATHC_TURBOP) != 0;
            break;
        default:
            break;
        }

        /*
         * Configure the beacon and sleep timers.
         */
        if(!sc->sc_beacons) {
            ath_beacon_config(sc, vap);
            sc->sc_beacons = 1;
        }

        /*
         * Reset rssi stats; maybe not the best place...
         */
        sc->sc_halstats.ns_avgbrssi = ATH_RSSI_DUMMY_MARKER;
        sc->sc_halstats.ns_avgrssi = ATH_RSSI_DUMMY_MARKER;
        sc->sc_halstats.ns_avgtxrssi = ATH_RSSI_DUMMY_MARKER;
    } else {
        /*
         *  XXXX
         * if it is SCAN state , disable Beacons.
         */
        if (nstate == IEEE80211_S_SCAN) {
            ath_hal_intrset(ah,
                            sc->sc_imask &~ (HAL_INT_SWBA | HAL_INT_BMISS));
            sc->sc_imask &= ~(HAL_INT_SWBA | HAL_INT_BMISS);
            sc->sc_beacons=0; /* need to reconfigure the beacons  when it moves to RUN */
        }
    }
done:
    /*
     * Invoke the parent method to complete the work.
     */
    error = avp->av_newstate(vap, nstate, arg);

    /*
     * Finally, start any timers.
     */
    if (nstate == IEEE80211_S_RUN) {
        /* start periodic recalibration timer */
        mod_timer(&sc->sc_cal_ch, jiffies + (ath_calinterval * HZ));
    }

#ifdef ATH_SUPERG_XR
    if(vap->iv_flags & IEEE80211_F_XR && nstate == IEEE80211_S_RUN && !sc->sc_xrgrppoll) {
        ath_grppoll_txq_setup(sc,HAL_TX_QUEUE_DATA,GRP_POLL_PERIOD_NO_XR_STA(sc));
        ath_grppoll_start(vap,sc->sc_xrpollcount);
        ath_hal_setrxfilter(ah, rfilt|HAL_RX_FILTER_XRPOLL);
    }
    if(vap->iv_flags & IEEE80211_F_XR && nstate == IEEE80211_S_INIT && sc->sc_xrgrppoll) {
        ath_grppoll_stop(vap);
    }
#endif
bad:
    netif_start_queue(dev);
    return error;
}

#ifdef ATH_SUPERG_COMP
/* Enable/Disable de-compression mask for given node.
 * The routine is invoked after addition or deletion of the
 * key.
 */
static void
ath_comp_set(struct ieee80211vap *vap, struct ieee80211_node *ni, int en)
{
    ath_setup_comp(ni, en);
    return;
}

/* Set up decompression engine for this node. */
static void
ath_setup_comp(struct ieee80211_node *ni, int enable)
{
#define IEEE80211_KEY_XR    (IEEE80211_KEY_XMIT | IEEE80211_KEY_RECV)
    struct ieee80211vap *vap = ni->ni_vap;
    struct ath_softc *sc = vap->iv_ic->ic_dev->priv;
    struct ath_node *an = ATH_NODE(ni);
    u_int16_t  keyindex;

    if (enable) {
        /* Have we negotiated compression? */
            if (!(ni->ni_ath_flags & IEEE80211_NODE_COMP)) {
            return;
        }

        /* No valid key? */
        if (ni->ni_ucastkey.wk_keyix == IEEE80211_KEYIX_NONE) {
            return;
        }

            /* Setup decompression mask.
         * For TKIP and split MIC case, recv. keyindex is at 32 offset
         * from tx key.
         */
        if ((ni->ni_wpa_ie != NULL) &&
            (ni->ni_rsn.rsn_ucastcipher == IEEE80211_CIPHER_TKIP) &&
            sc->sc_splitmic) {
            if ((ni->ni_ucastkey.wk_flags & IEEE80211_KEY_XR)
                            == IEEE80211_KEY_XR) {
                keyindex = ni->ni_ucastkey.wk_keyix + 32;
            } else {
                keyindex = ni->ni_ucastkey.wk_keyix;
            }
        } else {
            keyindex = ni->ni_ucastkey.wk_keyix +
                            ni->ni_rxkeyoff;
        }

            ath_hal_setdecompmask(sc->sc_ah, keyindex, 1);
            an->an_decomp_index = keyindex;
    } else {
        if (an->an_decomp_index != INVALID_DECOMP_INDEX) {
            ath_hal_setdecompmask(sc->sc_ah,
                        an->an_decomp_index, 0);
            an->an_decomp_index = INVALID_DECOMP_INDEX;
        }
    }

    return;
#undef IEEE80211_KEY_XR
}
#endif

/*
 * Allocate a key cache slot to the station so we can
 * setup a mapping from key index to node. The key cache
 * slot is needed for managing antenna state and for
 * compression when stations do not use crypto.  We do
 * it uniliaterally here; if crypto is employed this slot
 * will be reassigned.
 */
static void
ath_setup_stationkey(struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ath_softc *sc = vap->iv_ic->ic_dev->priv;
    u_int16_t keyix;

    keyix = ath_key_alloc(vap, &ni->ni_ucastkey);
    if (keyix == IEEE80211_KEYIX_NONE) {
        /*
         * Key cache is full; we'll fall back to doing
         * the more expensive lookup in software.  Note
         * this also means no h/w compression.
         */
        /* XXX msg+statistic */
        return;
    } else {
        ni->ni_ucastkey.wk_keyix = keyix;
        /* NB: this will create a pass-thru key entry */
        ath_keyset(sc, &ni->ni_ucastkey, ni->ni_macaddr, vap->iv_bss);

#ifdef ATH_SUPERG_COMP
        /* Enable de-compression logic */
        ath_setup_comp(ni, 1);
#endif
    }

    return;
}

/* Setup WEP key for the station if compression is negotiated.
 * When station and AP are using same default key index, use single key
 * cache entry for receive and transmit, else two key cache entries are
 * created. One for receive with MAC address of station and one for transmit
 * with NULL mac address. On receive key cache entry de-compression mask
 * is enabled.
 */
static void
ath_setup_stationwepkey(struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_key *ni_key;
    struct ieee80211_key tmpkey;
    struct ieee80211_key *rcv_key, *xmit_key;
    int    txkeyidx, rxkeyidx = IEEE80211_KEYIX_NONE, i;
    u_int8_t null_macaddr[IEEE80211_ADDR_LEN] = {0,0,0,0,0,0};

    KASSERT(ni->ni_ath_defkeyindex < IEEE80211_WEP_NKID,
        ("got invalid node key index 0x%x", ni->ni_ath_defkeyindex));
    KASSERT(vap->iv_def_txkey < IEEE80211_WEP_NKID,
        ("got invalid vap def key index 0x%x", vap->iv_def_txkey));

    /* Allocate a key slot first */
    if (!ieee80211_crypto_newkey(vap,
            IEEE80211_CIPHER_WEP,
            IEEE80211_KEY_XMIT|IEEE80211_KEY_RECV,
            &ni->ni_ucastkey)) {
        goto error;
    }

    txkeyidx = ni->ni_ucastkey.wk_keyix;
    xmit_key = &vap->iv_nw_keys[vap->iv_def_txkey];

    /* Do we need seperate rx key? */
    if (ni->ni_ath_defkeyindex != vap->iv_def_txkey) {
        ni->ni_ucastkey.wk_keyix = IEEE80211_KEYIX_NONE;
        if (!ieee80211_crypto_newkey(vap,
                IEEE80211_CIPHER_WEP,
                IEEE80211_KEY_XMIT|IEEE80211_KEY_RECV,
                &ni->ni_ucastkey)) {
            ni->ni_ucastkey.wk_keyix = txkeyidx;
            ieee80211_crypto_delkey(vap, &ni->ni_ucastkey, ni);
            goto error;
        }
        rxkeyidx = ni->ni_ucastkey.wk_keyix;
        ni->ni_ucastkey.wk_keyix = txkeyidx;

        rcv_key = &vap->iv_nw_keys[ni->ni_ath_defkeyindex];
    } else {
        rcv_key = xmit_key;
        rxkeyidx = txkeyidx;
    }

    /* Remember receive key offset */
    ni->ni_rxkeyoff = rxkeyidx - txkeyidx;

    /* Setup xmit key */
    ni_key = &ni->ni_ucastkey;
    if (rxkeyidx != txkeyidx) {
        ni_key->wk_flags = IEEE80211_KEY_XMIT;
    }
    else {
        ni_key->wk_flags = IEEE80211_KEY_XMIT|IEEE80211_KEY_RECV;
    }
        ni_key->wk_keylen = xmit_key->wk_keylen;
	for(i=0;i<IEEE80211_TID_SIZE;++i)
		ni_key->wk_keyrsc[i] = xmit_key->wk_keyrsc[i];
        ni_key->wk_keytsc = 0;
        memset(ni_key->wk_key, 0, sizeof(ni_key->wk_key));
        memcpy(ni_key->wk_key, xmit_key->wk_key, xmit_key->wk_keylen);
    ieee80211_crypto_setkey(vap, &ni->ni_ucastkey,
        (rxkeyidx == txkeyidx) ? ni->ni_macaddr:null_macaddr, ni);

    if (rxkeyidx != txkeyidx) {
        /* Setup recv key */
        ni_key = &tmpkey;
        ni_key->wk_keyix = rxkeyidx;
        ni_key->wk_flags = IEEE80211_KEY_RECV;
            ni_key->wk_keylen = rcv_key->wk_keylen;
	    for(i=0;i<IEEE80211_TID_SIZE;++i)
		ni_key->wk_keyrsc[i] = rcv_key->wk_keyrsc[i];
            ni_key->wk_keytsc = 0;
            ni_key->wk_cipher = rcv_key->wk_cipher;
            ni_key->wk_private = rcv_key->wk_private;
            memset(ni_key->wk_key, 0, sizeof(ni_key->wk_key));
            memcpy(ni_key->wk_key, rcv_key->wk_key, rcv_key->wk_keylen);
        ieee80211_crypto_setkey(vap, &tmpkey, ni->ni_macaddr, ni);
    }

    return;

error:
    ni->ni_ath_flags &= ~IEEE80211_NODE_COMP;
    return;
}

/* Create a keycache entry for given node in clearcase as well as static wep.
 * Handle compression state if required.
 * For non clearcase/static wep case, the key is plumbed by hostapd.
 */
static void
ath_setup_keycacheslot(struct ath_softc *sc, struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;

    if (ni->ni_ucastkey.wk_keyix != IEEE80211_KEYIX_NONE) {
        ieee80211_crypto_delkey(vap, &ni->ni_ucastkey, ni);
    }

    /* Only for clearcase and WEP case */
    if ((vap->iv_flags & IEEE80211_F_PRIVACY) == 0 ||
          (ni->ni_ath_defkeyindex != IEEE80211_INVAL_DEFKEY)) {

        if ((vap->iv_flags & IEEE80211_F_PRIVACY) == 0) {
            KASSERT(ni->ni_ucastkey.wk_keyix  \
                    == IEEE80211_KEYIX_NONE, \
                    ("new node with a ucast key already \
                  setup (keyix %u)",\
                      ni->ni_ucastkey.wk_keyix));
            /* NB: 5210 has no passthru/clr key support */
            if (sc->sc_hasclrkey)
                ath_setup_stationkey(ni);
        } else {
            ath_setup_stationwepkey(ni);
        }
    }

    return;
}

/*
 * Setup driver-specific state for a newly associated node.
 * Note that we're called also on a re-associate, the isnew
 * param tells us if this is the first time or not.
 */
static void
ath_newassoc(struct ieee80211_node *ni, int isnew)
{
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211vap *vap = ni->ni_vap;
    struct ath_softc *sc = ic->ic_dev->priv;
    u_int32_t capflag = 0;

	if ((ni->ni_chwidth == IEEE80211_CWM_WIDTH40) && 
		(ic->ic_cwm.cw_width == IEEE80211_CWM_WIDTH40))
	{
		capflag |=  ATH_RC_CW40_FLAG;
	}
    if (ni->ni_htcap & IEEE80211_HTCAP_C_SHORTGI40) {
        capflag |=  ATH_RC_SGI_FLAG;
    }
    if (ni->ni_flags & IEEE80211_NODE_HT) {
        capflag |=  ATH_RC_HT_FLAG | ATH_RC_DS_FLAG;
    }
    ath_rate_newassoc(sc, ATH_NODE(ni), isnew, capflag);

    /* are we supporting compression? */
        if (!(vap->iv_ath_cap & ni->ni_ath_flags & IEEE80211_NODE_COMP)) {
        ni->ni_ath_flags &= ~IEEE80211_NODE_COMP;
    }

        /* disable compression for TKIP */
        if ((ni->ni_ath_flags & IEEE80211_NODE_COMP) &&
        (ni->ni_wpa_ie != NULL) &&
            (ni->ni_rsn.rsn_ucastcipher == IEEE80211_CIPHER_TKIP)) {
                ni->ni_ath_flags &= ~IEEE80211_NODE_COMP;
    }

    ath_setup_keycacheslot(sc, ni);
#ifdef ATH_SUPERG_XR
    if(1) {
        struct ath_node *an = ATH_NODE(ni);
        if (ic->ic_ath_cap & an->an_node.ni_ath_flags & IEEE80211_ATHC_XR) {
            an->an_minffrate = ATH_MIN_FF_RATE;
        } else {
            an->an_minffrate = 0;
        }
        ath_grppoll_period_update(sc);
    }
#endif
}

static int
ath_getchannels(struct net_device *dev, u_int cc,
    HAL_BOOL outdoor, HAL_BOOL xchanmode)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;
    HAL_CHANNEL *chans;
    int i, nchan, chansToGet, match = 0;
    u_int16_t   mode;

    chans = kmalloc(IEEE80211_CHAN_MAX * sizeof(HAL_CHANNEL), GFP_KERNEL);
    if (chans == NULL) {
        printk("%s: unable to allocate channel table\n", dev->name);
        return ENOMEM;
    }

    /*
     * Couple of observations and issues:
     * 1) IEEE80211_CHAN_MAX is 255 and Owl upper layer code asserts
     * with higher number of channels. This typically happens with
     * DEBUG domain, which they will use for debugging. For now,
     * LIMITINGS THE MAX CHANS TO 60. This needs to be fixed. 60
     * channels is sufficient for 11ng.
     * 
     * 2) HAL indicates MODE-11A for Owl, which it does not support.
     * MODE-11A is indicated in HAL as a workaround for an exiting bug.
     * This would result in A-channels being present in channel list,
     * which is wrong. Instead of asking all modes, we are asking for
     * only 11ng mode. When Bug 20833 is fixed, this work around needs
     * be taken out. 
     *
     * 
     * 3) Each channel is represented with freq and mode(a/b/g). 
     * In case of 11n, we have 11n-HT20 and 11n-HT40, which regdomain
     * considers as mode.
     * All 11g channels are HT20 capable. HT40 flag indicates, that this
     * the centre freq for 40-MHz band. We use this info to extrapolate
     * and generate EXT channel info and set the flag. While "freq + mode"
     * is left alone for existing channels(a/b/g), for 11n, Owl upper
     * layer expects a 11n channel to occur once with all its capabilities.
     *
     *
     * 4) Upper layer code needs to interpret the EXT_U and EXT_L
     * bit flags to decide how to use a particular channel.
     *
     */
    mode = (force_11a_channels) ? HAL_MODE_11NA : HAL_MODE_11NG;
    chansToGet = 60;

    if (!ath_hal_init_channels(ah, chans, chansToGet, &nchan,
        ic->ic_regclassids, IEEE80211_REGCLASSIDS_MAX, &ic->ic_nregclass,
        cc, mode, outdoor, xchanmode, CHANNEL_HT40 | CHANNEL_HT20)) {
        u_int32_t rd;

        ath_hal_getregdomain(ah, &rd);
        printk("%s: unable to collect channel list from hal; "
            "regdomain likely %u country code %u\n",
            dev->name, rd, cc);
        kfree(chans);
        return EINVAL;
    }


    /*
     * Convert HAL channels to ieee80211 ones.
     */
    for (i = 0; i < nchan; i++) {
        HAL_CHANNEL *c = &chans[i];
        struct ieee80211_channel *ichan = &ic->ic_channels[i];

        ichan->ic_ieee = ath_hal_mhz2ieee(ah, c->channel,
                            c->channelFlags);
        ichan->ic_freq = c->channel;
        ichan->ic_flags = c->channelFlags;
        ichan->ic_maxregpower = c->maxRegTxPower;   /* dBm */
        ichan->ic_maxpower = c->maxTxPower / 4;     /* 1/4 dBm */
        ichan->ic_minpower = c->minTxPower / 4;     /* 1/4 dBm */
    }

    /* Generate CTL and EXT channel info for 11n channels */
    for(i = 0; i < nchan; i++) {
        struct ieee80211_channel *ichan = &ic->ic_channels[i], 
                                 *pCur;

        if(IEEE80211_IS_CHAN_HT40_CAPABLE(ichan)) {
            int j, checkChanFreqUpper, checkChanFreqLower;

            checkChanFreqUpper = ichan->ic_freq + 10;
            checkChanFreqLower = ichan->ic_freq - 10;

            pCur = ic->ic_channels;

            for(j = 0; j < nchan; j++, pCur++) {
                if (IEEE80211_IS_CHAN_5GHZ(pCur)) {
                    pCur->ic_flags |= (IEEE80211_CHAN_HT40U | IEEE80211_CHAN_HT40L);
                    continue;
                }

                if(IEEE80211_IS_CHAN_HT_CAPABLE(pCur)) {
                    if(pCur->ic_freq == checkChanFreqUpper)
                        pCur->ic_flags |= IEEE80211_CHAN_HT40U;
                    else if(pCur->ic_freq == checkChanFreqLower)
                        pCur->ic_flags |= IEEE80211_CHAN_HT40L; 
                }
            }

        }
    }


    /* Have one 11n entry per channel */
    for(i = 0; i < nchan; i++) {
        struct ieee80211_channel *pCur =  &ic->ic_channels[i],
                                 *pPrev=NULL, *pNext=NULL;
        int j;

        if(IEEE80211_IS_CHAN_HT40_CAPABLE(pCur)) {
            for(j = i-1; j > 0; j--) {
                pPrev = pCur - (i - j);

                if(pPrev->ic_freq == pCur->ic_freq &&
                        IEEE80211_IS_CHAN_HT20_CAPABLE(pPrev)) {
                    memset(pPrev, 0, sizeof(*pPrev));
                    match++;
                    break;
                }
            }

            for(j = i + 1; j < nchan; j++) {
                pNext = pCur + (j - i); 
                
                if(pNext->ic_freq == pCur->ic_freq &&
                        IEEE80211_IS_CHAN_HT20_CAPABLE(pPrev)) {
                    memset(pNext, 0, sizeof(*pNext));
                    match++;
                    break;
                }
            }
        }       
    }

    /* sort the list, again */
    sort(ic->ic_channels, nchan, sizeof(struct ieee80211_channel), chanCompare, NULL);
    nchan -= match;

    /* Move the list to head */
    for(i = 0; i <= nchan; i++){
        struct ieee80211_channel *pCur;
        pCur =  &ic->ic_channels[i];
        *pCur = *(pCur + match);
    }

    ic->ic_nchans = nchan;
    kfree(chans);
#define REG_DUMP_CHANNELS
#ifdef  REG_DUMP_CHANNELS
    ieee80211_announce_channels(ic);
#endif
    return 0;
}

int
chanCompare(const void * pLt, const void *pRt)
{

    return (((struct ieee80211_channel *)pLt)->ic_freq -
            ((struct ieee80211_channel *)pRt)->ic_freq );

}


static void
ath_led_done(unsigned long arg)
{
    struct ath_softc *sc = (struct ath_softc *) arg;

    sc->sc_blinking = 0;
}

/*
 * Turn the LED off: flip the pin and then set a timer so no
 * update will happen for the specified duration.
 */
static void
ath_led_off(unsigned long arg)
{
    struct ath_softc *sc = (struct ath_softc *) arg;

    ath_hal_gpioset(sc->sc_ah, sc->sc_ledpin, !sc->sc_ledon);
    sc->sc_ledtimer.function = ath_led_done;
    sc->sc_ledtimer.expires = jiffies + sc->sc_ledoff;
    add_timer(&sc->sc_ledtimer);
}

/*
 * Blink the LED according to the specified on/off times.
 */
static void
ath_led_blink(struct ath_softc *sc, int on, int off)
{
    DPRINTF(sc, ATH_DEBUG_LED, "%s: on %u off %u\n", __func__, on, off);
    ath_hal_gpioset(sc->sc_ah, sc->sc_ledpin, sc->sc_ledon);
    sc->sc_blinking = 1;
    sc->sc_ledoff = off;
    sc->sc_ledtimer.function = ath_led_off;
    sc->sc_ledtimer.expires = jiffies + on;
    add_timer(&sc->sc_ledtimer);
}

void
ath_led_event(struct ath_softc *sc, int event)
{

    sc->sc_ledevent = jiffies;  /* time of last event */
    if (sc->sc_blinking)        /* don't interrupt active blink */
        return;
    switch (event) {
    case ATH_LED_POLL:
        ath_led_blink(sc, sc->sc_hwmap[0].ledon,
            sc->sc_hwmap[0].ledoff);
        break;
    case ATH_LED_TX:
        if (sc->sc_txrate & 0x80) {
            ath_led_blink(sc, sc->sc_hthwmap[sc->sc_txrate & IEEE80211_RATE_VAL].ledon,
                sc->sc_hthwmap[sc->sc_txrate & IEEE80211_RATE_VAL].ledoff);
        } else {
            ath_led_blink(sc, sc->sc_hwmap[sc->sc_txrate].ledon,
                sc->sc_hwmap[sc->sc_txrate].ledoff);
        }
        break;
    case ATH_LED_RX:
        if (sc->sc_rxrate & 0x80) {
            ath_led_blink(sc, sc->sc_hthwmap[sc->sc_rxrate & IEEE80211_RATE_VAL].ledon,
                sc->sc_hthwmap[sc->sc_rxrate& IEEE80211_RATE_VAL].ledoff);
        } else {
            ath_led_blink(sc, sc->sc_hwmap[sc->sc_rxrate].ledon,
                sc->sc_hwmap[sc->sc_rxrate].ledoff);

        }
        break;
    }
}

static void
ath_update_txpow(struct ath_softc *sc)
{
    struct ieee80211com *ic = &sc->sc_ic;
    struct ieee80211vap *vap;
    struct ath_hal *ah = sc->sc_ah;
    u_int32_t txpow;

    if (sc->sc_curtxpow != ic->ic_txpowlimit) {
        ath_hal_settxpowlimit(ah, ic->ic_txpowlimit);
        /* read back in case value is clamped */
        ath_hal_gettxpowlimit(ah, &txpow);
        ic->ic_txpowlimit = sc->sc_curtxpow = txpow;
    }
    /*
     * Fetch max tx power level for status requests.
     */
    ath_hal_getmaxtxpow(sc->sc_ah, &txpow);
    /* XXX locking/move to net80211 */
    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
        vap->iv_bss->ni_txpower = txpow;
}

#ifdef ATH_SUPERG_XR
static int
ath_xr_rate_setup(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    struct ieee80211com *ic = &sc->sc_ic;
    const HAL_RATE_TABLE *rt;
    struct ieee80211_rateset *rs;
    int i, maxrates;
    sc->sc_xr_rates = ath_hal_getratetable(ah, HAL_MODE_XR);
    rt = sc->sc_xr_rates;
    if (rt == NULL)
        return 0;
    if (rt->rateCount > XR_NUM_SUP_RATES) {
        DPRINTF(sc, ATH_DEBUG_ANY,
            "%s: rate table too small (%u > %u)\n",
            __func__, rt->rateCount, IEEE80211_RATE_MAXSIZE);
        maxrates = IEEE80211_RATE_MAXSIZE;
    } else
        maxrates = rt->rateCount;
    rs = &ic->ic_sup_xr_rates;
    for (i = 0; i < maxrates; i++)
        rs->rs_rates[i] = rt->info[i].dot11Rate;
    rs->rs_nrates = maxrates;
    return 1;
}
#endif

/* Setup half/quarter rate table support */
static void
ath_setup_subrates(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    struct ieee80211com *ic = &sc->sc_ic;
    const HAL_RATE_TABLE *rt;
    struct ieee80211_rateset *rs;
    int i, maxrates;

    sc->sc_half_rates = ath_hal_getratetable(ah, HAL_MODE_11A_HALF_RATE);
    rt = sc->sc_half_rates;
    if (rt != NULL) {
        if (rt->rateCount > IEEE80211_RATE_MAXSIZE) {
            DPRINTF(sc, ATH_DEBUG_ANY,
                "%s: rate table too small (%u > %u)\n",
                   __func__, rt->rateCount, IEEE80211_RATE_MAXSIZE);
            maxrates = IEEE80211_RATE_MAXSIZE;
        } else
            maxrates = rt->rateCount;
        rs = &ic->ic_sup_half_rates;
        for (i = 0; i < maxrates; i++)
            rs->rs_rates[i] = rt->info[i].dot11Rate;
        rs->rs_nrates = maxrates;
    }

    sc->sc_quarter_rates = ath_hal_getratetable(ah,
                        HAL_MODE_11A_QUARTER_RATE);
    rt = sc->sc_quarter_rates;
    if (rt != NULL) {
        if (rt->rateCount > IEEE80211_RATE_MAXSIZE) {
            DPRINTF(sc, ATH_DEBUG_ANY,
                "%s: rate table too small (%u > %u)\n",
                   __func__, rt->rateCount, IEEE80211_RATE_MAXSIZE);
            maxrates = IEEE80211_RATE_MAXSIZE;
        } else
            maxrates = rt->rateCount;
        rs = &ic->ic_sup_quarter_rates;
        for (i = 0; i < maxrates; i++)
            rs->rs_rates[i] = rt->info[i].dot11Rate;
        rs->rs_nrates = maxrates;
    }
}

static int
ath_rate_setup(struct net_device *dev, u_int mode)
{
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    struct ieee80211com *ic = &sc->sc_ic;
    const HAL_RATE_TABLE *rt;
    struct ieee80211_rateset *rs;
    int i, maxrates, rix;

    switch (mode) {
    case IEEE80211_MODE_11A:
        sc->sc_rates[mode] = ath_hal_getratetable(ah, HAL_MODE_11A);
        break;
    case IEEE80211_MODE_11B:
        sc->sc_rates[mode] = ath_hal_getratetable(ah, HAL_MODE_11B);
        break;
    case IEEE80211_MODE_11G:
        sc->sc_rates[mode] = ath_hal_getratetable(ah, HAL_MODE_11G);
        break;
    case IEEE80211_MODE_TURBO_A:
        sc->sc_rates[mode] = ath_hal_getratetable(ah, HAL_MODE_TURBO);
        break;
    case IEEE80211_MODE_TURBO_G:
        sc->sc_rates[mode] = ath_hal_getratetable(ah, HAL_MODE_108G);
        break;
    case IEEE80211_MODE_11NA:
        sc->sc_rates[mode] = ath_hal_getratetable(ah, HAL_MODE_11NA);
        break;
    case IEEE80211_MODE_11NG:
        sc->sc_rates[mode] = ath_hal_getratetable(ah, HAL_MODE_11NG);
        break;
    default:
        DPRINTF(sc, ATH_DEBUG_ANY, "%s: invalid mode %u\n",
            __func__, mode);
        return 0;
    }
    rt = sc->sc_rates[mode];
    if (rt == NULL)
        return 0;
    if (rt->rateCount > IEEE80211_RATE_MAXSIZE) {
        DPRINTF(sc, ATH_DEBUG_ANY,
            "%s: rate table too small (%u > %u)\n",
            __func__, rt->rateCount, IEEE80211_RATE_MAXSIZE);
        maxrates = IEEE80211_RATE_MAXSIZE;
    } else
        maxrates = rt->rateCount;
    rs = &ic->ic_sup_rates[mode];
    rix = 0;
    for (i = 0; i < maxrates; i++) {
        if ((rt->info[i].phy == IEEE80211_T_HT))
            continue;
        rs->rs_rates[rix++] = rt->info[i].dot11Rate;
    }
    rs->rs_nrates = rix;
    rix = 0;
    if ((mode == IEEE80211_MODE_11NA) || (mode == IEEE80211_MODE_11NG)) {
        rs = &ic->ic_sup_ht_rates[mode];
        for (i = 0; i < maxrates; i++) {
            if (rt->info[i].phy == IEEE80211_T_HT) {
                rs->rs_rates[rix++] = rt->info[i].dot11Rate;
            }
        }
        rs->rs_nrates = rix;
    }
    return 1;
}

static void
ath_setcurmode(struct ath_softc *sc, enum ieee80211_phymode mode)
{
#define N(a)    (sizeof(a)/sizeof(a[0]))
    const HAL_RATE_TABLE *rt;
    int i, j;

    memset(sc->sc_htrixmap, 0xff, sizeof(sc->sc_htrixmap));
    memset(sc->sc_rixmap, 0xff, sizeof(sc->sc_rixmap));
    rt = sc->sc_rates[mode];
    KASSERT(rt != NULL, ("no h/w rate set for phy mode %u", mode));

    for (i = 0; i < rt->rateCount; i++) {
            sc->sc_rixmap[rt->info[i].rateCode] = i;
    }
    memset(sc->sc_hwmap, 0, sizeof(sc->sc_hwmap));
    for (i = 0; i < 32; i++) {
        sc->sc_hwmap[i].ledon = (500 * HZ) / 1000;
        sc->sc_hwmap[i].ledoff = (130 * HZ) / 1000;
    }
    for (i = 0; i < 32; i++) {
        sc->sc_hthwmap[i].ledon = (500 * HZ) / 1000;
        sc->sc_hthwmap[i].ledoff = (130 * HZ) / 1000;
    }
    for (i = 0; i < rt->rateCount; i++) {
        u_int8_t code = rt->info[i].rateCode & IEEE80211_RATE_VAL;

        if (rt->info[i].phy == IEEE80211_T_HT)
        {
            /* NB: on/off times from the Atheros NDIS driver, w/ permission */
            static const struct {
                u_int       rate;       /* tx/rx 802.11 rate */
                u_int16_t   timeOn;     /* LED on time (ms) */
                u_int16_t   timeOff;    /* LED off time (ms) */
            } htblinkrates[] = {
                {  15,  32,   8 },
                {  14,  36,   9 },
                {  13,  40,  10 },
                {  12,  44,  11 },
                {  11,  50,  13 },
                {  10,  57,  14 },
                {   9,  67,  16 },
                {   8,  80,  20 },
                {   7, 100,  25 },
                {   6, 133,  34 },
                {   5, 160,  40 },
                {   4, 200,  50 },
                {   3, 240,  58 },
                {   2, 267,  66 },
                {   1, 400, 100 },
                {   0, 500, 130 },
            };
            sc->sc_hthwmap[code].ieeerate =
                rt->info[i].dot11Rate & IEEE80211_RATE_VAL;
            if (rt->info[i].shortPreamble ||
                rt->info[i].phy == IEEE80211_T_OFDM)
                sc->sc_hthwmap[code].flags |= IEEE80211_RADIOTAP_F_SHORTPRE;
            /* setup blink rate table to avoid per-packet lookup */
            for (j = 0; j < N(htblinkrates)-1; j++)
                if (htblinkrates[j].rate == sc->sc_hthwmap[code].ieeerate)
                    break;
            /* NB: this uses the last entry if the rate isn't found */
            /* XXX beware of overlow */
            sc->sc_hthwmap[code].ledon = (htblinkrates[j].timeOn * HZ) / 1000;
            sc->sc_hthwmap[code].ledoff = (htblinkrates[j].timeOff * HZ) / 1000;
        } else {
            /* NB: on/off times from the Atheros NDIS driver, w/ permission */
            static const struct {
                u_int       rate;       /* tx/rx 802.11 rate */
                u_int16_t   timeOn;     /* LED on time (ms) */
                u_int16_t   timeOff;    /* LED off time (ms) */
            } blinkrates[] = {
                { 108,  40,  10 },
                {  96,  44,  11 },
                {  72,  50,  13 },
                {  48,  57,  14 },
                {  36,  67,  16 },
                {  24,  80,  20 },
                {  22, 100,  25 },
                {  18, 133,  34 },
                {  12, 160,  40 },
                {  10, 200,  50 },
                {   6, 240,  58 },
                {   4, 267,  66 },
                {   2, 400, 100 },
                {   0, 500, 130 },
            };
            sc->sc_hwmap[code].ieeerate =
                rt->info[i].dot11Rate & IEEE80211_RATE_VAL;
            if (rt->info[i].shortPreamble ||
                rt->info[i].phy == IEEE80211_T_OFDM)
                sc->sc_hwmap[code].flags |= IEEE80211_RADIOTAP_F_SHORTPRE;
            /* setup blink rate table to avoid per-packet lookup */
            for (j = 0; j < N(blinkrates)-1; j++)
                if (blinkrates[j].rate == sc->sc_hwmap[code].ieeerate)
                    break;
            /* NB: this uses the last entry if the rate isn't found */
            /* XXX beware of overlow */
            sc->sc_hwmap[code].ledon = (blinkrates[j].timeOn * HZ) / 1000;
            sc->sc_hwmap[code].ledoff = (blinkrates[j].timeOff * HZ) / 1000;
        }
    }
    sc->sc_currates = rt;
    sc->sc_curmode = mode;
    /*
     * All protection frames are transmited at 2Mb/s for
     * 11g, otherwise at 1Mb/s.
     * XXX select protection rate index from rate table.
     */
    sc->sc_protrix = (mode == IEEE80211_MODE_11G ? 1 : 0);
    /* rate index used to send mgt frames */
    sc->sc_minrateix = 0;
#undef N
}

#ifdef ATH_SUPERG_FF
static u_int32_t
athff_approx_txtime(struct ath_softc *sc, struct ath_node *an, struct sk_buff *skb)
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
    if (sc->sc_ic.ic_flags & IEEE80211_F_PRIVACY) {
        framelen += 24;
    }
    if (an->an_tx_ffbuf[skb->priority]) {
        framelen += an->an_tx_ffbuf[skb->priority]->bf_skb->len;
    }

    txtime = ath_hal_computetxtime(sc->sc_ah, sc->sc_currates, framelen,
                       an->an_prevdatarix, AH_FALSE);

    return txtime;
}
/*
 * Determine if a data frame may be aggregated via ff tunnelling.
 *
 *  NB: allowing EAPOL frames to be aggregated with other unicast traffic.
 *      Do 802.1x EAPOL frames proceed in the clear? Then they couldn't
 *      be aggregated with other types of frames when encryption is on?
 *
 *  NB: assumes lock on an_tx_ffbuf effectively held by txq lock mechanism.
 */
static int
athff_can_aggregate(struct ath_softc *sc, struct ether_header *eh,
            struct ath_node *an, struct sk_buff *skb, u_int16_t fragthreshold, int *flushq)
{
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_txq *txq = sc->sc_ac2q[skb->priority];
    struct ath_buf *ffbuf = an->an_tx_ffbuf[skb->priority];
    u_int32_t txoplimit;

    *flushq = AH_FALSE;

    if (fragthreshold < 2346)
        return AH_FALSE;

    if ((!ffbuf) && (txq->axq_depth < sc->sc_fftxqmin) ) {
        return AH_FALSE;
    }
    if (!(ic->ic_ath_cap & an->an_node.ni_ath_flags & IEEE80211_ATHC_FF)) {
        return AH_FALSE;
    }
    if (!(ic->ic_opmode == IEEE80211_M_STA || ic->ic_opmode == IEEE80211_M_HOSTAP)) {
        return AH_FALSE;
    }
    if ((ic->ic_opmode == IEEE80211_M_HOSTAP) && ETHER_IS_MULTICAST(eh->ether_dhost)) {
        return AH_FALSE;
    }

#ifdef ATH_SUPERG_XR
    if(sc->sc_currates->info[an->an_prevdatarix].rateKbps  < an->an_minffrate) {
        return AH_FALSE;
    }
#endif
    txoplimit = IEEE80211_TXOP_TO_US(
        ic->ic_wme.wme_chanParams.cap_wmeParams[skb->priority].wmep_txopLimit);
    if (txoplimit != 0 && athff_approx_txtime(sc, an, skb) > txoplimit) {
        DPRINTF(sc, ATH_DEBUG_XMIT | ATH_DEBUG_FF,
            "%s: FF TxOp violation\n", __func__);
        if (ffbuf) {
            *flushq = AH_TRUE;
        }
        return AH_FALSE;
    }

    return AH_TRUE;
}
#endif

#ifdef AR_DEBUG
static void
ath_printrxbuf(struct ath_buf *bf, int done)
{
    struct ath_desc *ds = bf->bf_desc;
    int i;

    printk("R (%p %p) %08x %08x %08x %08x ",
        ds, (struct ath_desc *)bf->bf_daddr,
        ds->ds_link, ds->ds_data,
        ds->ds_ctl0, ds->ds_ctl1);
    for (i = 0; i < sizeof(ds->ds_hw)/sizeof(ds->ds_hw[0]); i++) {
        printk("%08x ", ds->ds_hw[i]);
    }
}

static void
ath_printtxbuf(struct ath_buf *bf, int done)
{
    struct ath_desc *ds = bf->bf_desc;
    int i;

    printk("T (%p %p) %08x %08x %08x %08x ",
        ds, (struct ath_desc *)bf->bf_daddr,
        ds->ds_link, ds->ds_data,
        ds->ds_ctl0, ds->ds_ctl1);
    for (i = 0; i < sizeof(ds->ds_hw)/sizeof(ds->ds_hw[0]); i++) {
        printk("%08x ", ds->ds_hw[i]);
    }
    printk("%c\n", !done ? ' ' : (ds->ds_txstat.ts_status == 0) ? '*' : '!');
}
#endif /* AR_DEBUG */

/*
 * Return netdevice statistics.
 */
static struct net_device_stats *
ath_getstats(struct net_device *dev)
{
    struct ath_softc *sc = dev->priv;
    struct net_device_stats *stats = &sc->sc_devstats;

    /* update according to private statistics */
    stats->tx_errors = sc->sc_stats.ast_tx_xretries
             + sc->sc_stats.ast_tx_fifoerr
             + sc->sc_stats.ast_tx_filtered
             ;
    stats->tx_dropped = sc->sc_stats.ast_tx_nobuf
            + sc->sc_stats.ast_tx_encap
            + sc->sc_stats.ast_tx_nonode
            + sc->sc_stats.ast_tx_nobufmgt;
    stats->rx_errors = sc->sc_stats.ast_rx_fifoerr
            + sc->sc_stats.ast_rx_badcrypt
            + sc->sc_stats.ast_rx_badmic
            ;
    stats->rx_dropped = sc->sc_stats.ast_rx_tooshort;
    stats->rx_crc_errors = sc->sc_stats.ast_rx_crcerr;

    return stats;
}

static int
ath_set_mac_address(struct net_device *dev, void *addr)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    struct ath_hal *ah = sc->sc_ah;
    struct sockaddr *mac = addr;
    int error;

    if (netif_running(dev)) {
        DPRINTF(sc, ATH_DEBUG_ANY,
            "%s: cannot set address; device running\n", __func__);
        return -EBUSY;
    }
    DPRINTF(sc, ATH_DEBUG_ANY, "%s: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
        __func__,
        mac->sa_data[0], mac->sa_data[1], mac->sa_data[2],
        mac->sa_data[3], mac->sa_data[4], mac->sa_data[5]);

    ATH_LOCK(sc);
    /* XXX not right for multiple vap's */
    IEEE80211_ADDR_COPY(ic->ic_myaddr, mac->sa_data);
    IEEE80211_ADDR_COPY(dev->dev_addr, mac->sa_data);
    ath_hal_setmac(ah, dev->dev_addr);
    error = -ath_reset(dev);
    ATH_UNLOCK(sc);

    return error;
}

static int
ath_change_mtu(struct net_device *dev, int mtu)
{
    struct ath_softc *sc = dev->priv;
    int error;

    if (!(ATH_MIN_MTU < mtu && mtu <= ATH_MAX_MTU)) {
        DPRINTF(sc, ATH_DEBUG_ANY, "%s: invalid %d, min %u, max %u\n",
            __func__, mtu, ATH_MIN_MTU, ATH_MAX_MTU);
        return -EINVAL;
    }
    DPRINTF(sc, ATH_DEBUG_ANY, "%s: %d\n", __func__, mtu);

    ATH_LOCK(sc);
    dev->mtu = mtu;
    /* NB: the rx buffers may need to be reallocated */
    tasklet_disable(&sc->sc_rxtq);
    error = -ath_reset(dev);
    tasklet_enable(&sc->sc_rxtq);
    ATH_UNLOCK(sc);

    return error;
}

/*
 * Diagnostic interface to the HAL.  This is used by various
 * tools to do things like retrieve register contents for
 * debugging.  The mechanism is intentionally opaque so that
 * it can change frequently w/o concern for compatiblity.
 */
static int
ath_ioctl_diag(struct ath_softc *sc, struct ath_diag *ad)
{
    struct ath_hal *ah = sc->sc_ah;
    u_int id = ad->ad_id & ATH_DIAG_ID;
    void *indata = NULL;
    void *outdata = NULL;
    u_int32_t insize = ad->ad_in_size;
    u_int32_t outsize = ad->ad_out_size;
    int error = 0;

    if (ad->ad_id & ATH_DIAG_IN) {
        /*
         * Copy in data.
         */
        indata = kmalloc(insize, GFP_KERNEL);
        if (indata == NULL) {
            error = -ENOMEM;
            goto bad;
        }
        if (copy_from_user(indata, ad->ad_in_data, insize)) {
            error = -EFAULT;
            goto bad;
        }
    }
    if (ad->ad_id & ATH_DIAG_DYN) {
        /*
         * Allocate a buffer for the results (otherwise the HAL
         * returns a pointer to a buffer where we can read the
         * results).  Note that we depend on the HAL leaving this
         * pointer for us to use below in reclaiming the buffer;
         * may want to be more defensive.
         */
        outdata = kmalloc(outsize, GFP_KERNEL);
        if (outdata == NULL) {
            error = -ENOMEM;
            goto bad;
        }
    }
    if (ath_hal_getdiagstate(ah, id, indata, insize, &outdata, &outsize)) {
        if (outsize < ad->ad_out_size)
            ad->ad_out_size = outsize;
        if (outdata &&
             copy_to_user(ad->ad_out_data, outdata, ad->ad_out_size))
            error = -EFAULT;
    } else {
        error = -EINVAL;
    }
bad:
    if ((ad->ad_id & ATH_DIAG_IN) && indata != NULL)
        kfree(indata);
    if ((ad->ad_id & ATH_DIAG_DYN) && outdata != NULL)
        kfree(outdata);
    return error;
}

extern  int ath_ioctl_ethtool(struct ath_softc *sc, int cmd, void *addr);

static int
ath_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    struct ath_softc *sc = dev->priv;
    struct ieee80211com *ic = &sc->sc_ic;
    int error;

    ATH_LOCK(sc);
    switch (cmd) {
    case SIOCGATHSTATS:
        sc->sc_stats.ast_tx_packets = sc->sc_devstats.tx_packets;
        sc->sc_stats.ast_rx_packets = sc->sc_devstats.rx_packets;
        /*
         * TODO:
         * The ieee layer knows only about the combined RSSI. It is not (yet?)
         * mimo aware.
         */ 
        sc->sc_stats.ast_rx_rssi_combined = ieee80211_getrssi(ic);
        if (copy_to_user(ifr->ifr_data, &sc->sc_stats,
            sizeof (sc->sc_stats)))
            error = -EFAULT;
        else
            error = 0;
        break;
    case SIOCGATHSTATSCLR:
        {
            struct ath_stats __clr_stats = {0};
            sc->sc_stats = __clr_stats;
            error = 0;
        }
        break;
    case SIOCGATHDIAG:
        if (!capable(CAP_NET_ADMIN))
            error = -EPERM;
        else
            error = ath_ioctl_diag(sc, (struct ath_diag *) ifr);
        break;
    case SIOCGATHCWMINFO: 
    case SIOCGATHCWMDBG:
	error = ath_cwm_ioctl(sc, cmd, ifr->ifr_data);
	break;
    case SIOCETHTOOL:
        if (copy_from_user(&cmd, ifr->ifr_data, sizeof(cmd)))
            error = -EFAULT;
        else
            error = ath_ioctl_ethtool(sc, cmd, ifr->ifr_data);
        break;
    case SIOC80211IFCREATE:
        error = ieee80211_ioctl_create_vap(ic, ifr);
        break;
    default:
        error = -EINVAL;
        break;
    }
    ATH_UNLOCK(sc);
    return error;
}

#ifdef ATH_TX99_DIAG
static int
ath_rx99ctl(int mode,int val,struct ath_softc *sc)
{
    static void (*rx99ctl) (struct ath_softc *,int)=NULL;
    if(!mode) {
        if(rx99ctl) {
            inter_module_put("rx99ctl");
            rx99ctl=NULL;
        }
        return 0;
    }
    if(!rx99ctl) {
        rx99ctl = inter_module_get_request("rx99ctl","ath_tx99");
        if(!rx99ctl)
        {
            printk("Could not load the module ath_tx99 \n");

        }
    }
    if(rx99ctl) {
        rx99ctl(sc,val);
    }
    return 0;
}

static int
ath_tx99ctl(int mode,int val,struct ath_softc *sc)
{
    static void (*tx99ctl) (struct ath_softc *,int,ath_callback,ath_callback)=NULL;
    if(!mode) {
        if(tx99ctl) {
            inter_module_put("tx99ctl");
            tx99ctl=NULL;
        }
        return 0;
    }
    if(!tx99ctl) {
        tx99ctl = inter_module_get_request("tx99ctl","ath_tx99");
        if(!tx99ctl)
        {
            printk("Could not load the module ath_tx99 \n");

        }
    }
    if(tx99ctl) {
        if(sc->sc_txrx99.txpower == 0)
            sc->sc_txrx99.txpower=60;
        tx99ctl(sc,val,ath_draintxq,ath_stoprecv);
    }
    return 0;
}
#endif /* ATH_TX99_DIAG */

#ifdef CONFIG_SYSCTL
/*
 * Sysctls are split into ``static'' and ``dynamic'' tables.
 * The former are defined at module load time and are used
 * control parameters common to all devices.  The latter are
 * tied to particular device instances and come and go with
 * each device.  The split is currently a bit tenuous; many of
 * the static ones probably should be dynamic but having them
 * static (e.g. debug) means they can be set after a module is
 * loaded and before bringing up a device.  The alternative
 * is to add module parameters.
 */

/*
 * Dynamic (i.e. per-device) sysctls.  These are automatically
 * mirrored in /proc/sys.
 */
enum {
    ATH_SLOTTIME    = 1,
    ATH_ACKTIMEOUT  = 2,
    ATH_CTSTIMEOUT  = 3,
    ATH_SOFTLED = 4,
    ATH_LEDPIN  = 5,
    ATH_COUNTRYCODE = 6,
    ATH_REGDOMAIN   = 7,
    ATH_DEBUG   = 8,
    ATH_TXANTENNA   = 9,
    ATH_RXANTENNA   = 10,
    ATH_DIVERSITY   = 11,
    ATH_TXINTRPERIOD= 12,
    ATH_FFTXQMIN    = 18,
    ATH_TKIPMIC = 19,
    ATH_XR_POLL_PERIOD= 20,
    ATH_XR_POLL_COUNT = 21,
    ATH_GLOBALTXTIMEOUT = 22,
};

static int
ATH_SYSCTL_DECL(ath_sysctl_halparam, ctl, write, filp, buffer, lenp, ppos)
{
    struct ath_softc *sc = ctl->extra1;
    struct ath_hal *ah = sc->sc_ah;
    u_int val;
    int ret;

    ctl->data = &val;
    ctl->maxlen = sizeof(val);
    if (write) {
        ret = ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
                lenp, ppos);
        if (ret == 0) {
            switch (ctl->ctl_name) {
            case ATH_SLOTTIME:
                if (!ath_hal_setslottime(ah, val))
                    ret = -EINVAL;
                break;
            case ATH_ACKTIMEOUT:
                if (!ath_hal_setacktimeout(ah, val))
                    ret = -EINVAL;
                break;
            case ATH_CTSTIMEOUT:
                if (!ath_hal_setctstimeout(ah, val))
                    ret = -EINVAL;
                break;
            case ATH_SOFTLED:
                if (val != sc->sc_softled) {
                    if (val)
                        ath_hal_gpioCfgOutput(ah,
                            sc->sc_ledpin);
                    ath_hal_gpioset(ah, sc->sc_ledpin,!val);
                    sc->sc_softled = val;
                }
                break;
            case ATH_LEDPIN:
                sc->sc_ledpin = val;
                break;
            case ATH_DEBUG:
                sc->sc_debug = val;
                break;
            case ATH_TXANTENNA:
                /* XXX validate? */
                sc->sc_txantenna = val;
                break;
            case ATH_RXANTENNA:
                /* XXX validate? */
                ath_setdefantenna(sc, val);
                break;
            case ATH_DIVERSITY:
                /* XXX validate? */
                if (!sc->sc_hasdiversity)
                    return -EINVAL;
                sc->sc_diversity = val;
                ath_hal_setdiversity(ah, val);
                break;
            case ATH_TXINTRPERIOD:
                sc->sc_txintrperiod = val;
                break;
            case ATH_FFTXQMIN:
                sc->sc_fftxqmin = val;
                break;
            case ATH_TKIPMIC: {
                struct ieee80211com *ic = &sc->sc_ic;

                if (!ath_hal_hastkipmic(ah))
                    return -EINVAL;
                ath_hal_settkipmic(ah, val);
                if (val)
                    ic->ic_caps |= IEEE80211_C_TKIPMIC;
                else
                    ic->ic_caps &= ~IEEE80211_C_TKIPMIC;
                break;
            }
#ifdef ATH_SUPERG_XR
            case ATH_XR_POLL_PERIOD:
              if(val > XR_MAX_POLL_INTERVAL)
                val = XR_MAX_POLL_INTERVAL;
              else if(val < XR_MIN_POLL_INTERVAL)
                val = XR_MIN_POLL_INTERVAL;
              sc->sc_xrpollint = val;
              break;

            case ATH_XR_POLL_COUNT:
              if(val > XR_MAX_POLL_COUNT)
                val = XR_MAX_POLL_COUNT;
              else if(val < XR_MIN_POLL_COUNT)
                val = XR_MIN_POLL_COUNT;
              sc->sc_xrpollcount = val;
              break;
#endif
        case ATH_GLOBALTXTIMEOUT:
        if (!ath_hal_gttsupported(ah)) {
            return -EINVAL;
        }
                if (!ath_hal_setglobaltxtimeout(ah, val))
                    ret = -EINVAL;
                break;
            default:
                return -EINVAL;
            }
        }
    } else {
        switch (ctl->ctl_name) {
        case ATH_SLOTTIME:
            val = ath_hal_getslottime(ah);
            break;
        case ATH_ACKTIMEOUT:
            val = ath_hal_getacktimeout(ah);
            break;
        case ATH_CTSTIMEOUT:
            val = ath_hal_getctstimeout(ah);
            break;
        case ATH_SOFTLED:
            val = sc->sc_softled;
            break;
        case ATH_LEDPIN:
            val = sc->sc_ledpin;
            break;
        case ATH_COUNTRYCODE:
            ath_hal_getcountrycode(ah, &val);
            break;
        case ATH_REGDOMAIN:
            ath_hal_getregdomain(ah, &val);
            break;
        case ATH_DEBUG:
            val = sc->sc_debug;
            break;
        case ATH_TXANTENNA:
            val = sc->sc_txantenna;
            break;
        case ATH_RXANTENNA:
            val = ath_hal_getdefantenna(ah);
            break;
        case ATH_DIVERSITY:
            val = sc->sc_diversity;
            break;
        case ATH_TXINTRPERIOD:
            val = sc->sc_txintrperiod;
            break;
        case ATH_FFTXQMIN:
            val = sc->sc_fftxqmin;
            break;
        case ATH_TKIPMIC:
            val = ath_hal_gettkipmic(ah);
            break;
#ifdef ATH_SUPERG_XR
        case ATH_XR_POLL_PERIOD:
            val=sc->sc_xrpollint;
            break;

        case ATH_XR_POLL_COUNT:
            val=sc->sc_xrpollcount;
            break;
#endif
    case ATH_GLOBALTXTIMEOUT:
            if (!ath_hal_gttsupported(ah)) {
        return -EINVAL;
        }
            val = ath_hal_getglobaltxtimeout(ah);
            break;
        default:
            return -EINVAL;
        }
        ret = ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
                lenp, ppos);
    }
    return ret;
}

static  int mincalibrate = 1;           /* once a second */
static  int maxint = 0x7fffffff;        /* 32-bit big */

#define CTL_AUTO    -2  /* cannot be CTL_ANY or CTL_NONE */

static const ctl_table ath_sysctl_template[] = {
    { .ctl_name = ATH_SLOTTIME,
      .procname = "slottime",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_ACKTIMEOUT,
      .procname = "acktimeout",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_CTSTIMEOUT,
      .procname = "ctstimeout",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_SOFTLED,
      .procname = "softled",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_LEDPIN,
      .procname = "ledpin",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_COUNTRYCODE,
      .procname = "countrycode",
      .mode     = 0444,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_REGDOMAIN,
      .procname = "regdomain",
      .mode     = 0444,
      .proc_handler = ath_sysctl_halparam
    },
#ifdef AR_DEBUG
    { .ctl_name = ATH_DEBUG,
      .procname = "debug",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
#endif
    { .ctl_name = ATH_TXANTENNA,
      .procname = "txantenna",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_RXANTENNA,
      .procname = "rxantenna",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_DIVERSITY,
      .procname = "diversity",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_TXINTRPERIOD,
      .procname = "txintrperiod",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_FFTXQMIN,
      .procname = "fftxqmin",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_TKIPMIC,
      .procname = "tkipmic",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
#ifdef ATH_SUPERG_XR
    { .ctl_name = ATH_XR_POLL_PERIOD,
      .procname = "xrpollperiod",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { .ctl_name = ATH_XR_POLL_COUNT,
      .procname = "xrpollcount",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
#endif
       { .ctl_name  = ATH_GLOBALTXTIMEOUT,
      .procname = "globaltxtimeout",
      .mode     = 0644,
      .proc_handler = ath_sysctl_halparam
    },
    { 0 }
};

static void
ath_dynamic_sysctl_register(struct ath_softc *sc)
{
    int i, space;

    space = 5*sizeof(struct ctl_table) + sizeof(ath_sysctl_template);
    sc->sc_sysctls = kmalloc(space, GFP_KERNEL);
    if (sc->sc_sysctls == NULL) {
        printk("%s: no memory for sysctl table!\n", __func__);
        return;
    }

    /* setup the table */
    memset(sc->sc_sysctls, 0, space);
    sc->sc_sysctls[0].ctl_name = CTL_DEV;
    sc->sc_sysctls[0].procname = "dev";
    sc->sc_sysctls[0].mode = 0555;
    sc->sc_sysctls[0].child = &sc->sc_sysctls[2];
    /* [1] is NULL terminator */
    sc->sc_sysctls[2].ctl_name = CTL_AUTO;
    sc->sc_sysctls[2].procname = sc->sc_dev->name;
    sc->sc_sysctls[2].mode = 0555;
    sc->sc_sysctls[2].child = &sc->sc_sysctls[4];
    /* [3] is NULL terminator */
    /* copy in pre-defined data */
    memcpy(&sc->sc_sysctls[4], ath_sysctl_template,
        sizeof(ath_sysctl_template));

    /* add in dynamic data references */
    for (i = 4; sc->sc_sysctls[i].ctl_name; i++)
        if (sc->sc_sysctls[i].extra1 == NULL)
            sc->sc_sysctls[i].extra1 = sc;

    /* and register everything */
    sc->sc_sysctl_header = register_sysctl_table(sc->sc_sysctls, 1);
    if (!sc->sc_sysctl_header) {
        printk("%s: failed to register sysctls!\n", sc->sc_dev->name);
        kfree(sc->sc_sysctls);
        sc->sc_sysctls = NULL;
    }

    /* initialize values */
    sc->sc_debug = ath_debug;
    sc->sc_txantenna = 0;       /* default to auto-selection */
    sc->sc_txintrperiod = ATH_TXQ_INTR_PERIOD;
}

static void
ath_dynamic_sysctl_unregister(struct ath_softc *sc)
{
    if (sc->sc_sysctl_header) {
        unregister_sysctl_table(sc->sc_sysctl_header);
        sc->sc_sysctl_header = NULL;
    }
    if (sc->sc_sysctls) {
        kfree(sc->sc_sysctls);
        sc->sc_sysctls = NULL;
    }
}

/*
 * Announce various information on device/driver attach.
 */
static void
ath_announce(struct net_device *dev)
{
#define HAL_MODE_DUALBAND   (HAL_MODE_11A|HAL_MODE_11B)
    struct ath_softc *sc = dev->priv;
    struct ath_hal *ah = sc->sc_ah;
    u_int modes, cc;

    printk("%s: mac %d.%d phy %d.%d", dev->name,
        ah->ah_macVersion, ah->ah_macRev,
        ah->ah_phyRev >> 4, ah->ah_phyRev & 0xf);
    /*
     * Print radio revision(s).  We check the wireless modes
     * to avoid falsely printing revs for inoperable parts.
     * Dual-band radio revs are returned in the 5Ghz rev number.
     */
    ath_hal_getcountrycode(ah, &cc);
    modes = ath_hal_getwirelessmodes(ah, cc);
    if ((modes & HAL_MODE_DUALBAND) == HAL_MODE_DUALBAND) {
        if (ah->ah_analog5GhzRev && ah->ah_analog2GhzRev)
            printk(" 5ghz radio %d.%d 2ghz radio %d.%d",
                ah->ah_analog5GhzRev >> 4,
                ah->ah_analog5GhzRev & 0xf,
                ah->ah_analog2GhzRev >> 4,
                ah->ah_analog2GhzRev & 0xf);
        else
            printk(" radio %d.%d", ah->ah_analog5GhzRev >> 4,
                ah->ah_analog5GhzRev & 0xf);
    } else
        printk(" radio %d.%d", ah->ah_analog5GhzRev >> 4,
            ah->ah_analog5GhzRev & 0xf);
    printk("\n");
    if (1/*bootverbose*/) {
        int i;
        for (i = 0; i <= WME_AC_VO; i++) {
            struct ath_txq *txq = sc->sc_ac2q[i];
            printk("%s: Use hw queue %u for %s traffic\n",
                dev->name, txq->axq_qnum,
                ieee80211_wme_acnames[i]);
        }
        printk("%s: Use hw queue %u for CAB traffic\n", dev->name,
            sc->sc_cabq->axq_qnum);
        printk("%s: Use hw queue %u for beacons\n", dev->name,
            sc->sc_bhalq);
    }
#undef HAL_MODE_DUALBAND
}

/*
 * Static (i.e. global) sysctls.  Note that the hal sysctls
 * are located under ours by sharing the setting for DEV_ATH.
 */
enum {
    DEV_ATH     = 9,            /* XXX known by hal */
};

static ctl_table ath_static_sysctls[] = {
#ifdef AR_DEBUG
    { .ctl_name = CTL_AUTO,
       .procname    = "debug",
      .mode     = 0644,
      .data     = &ath_debug,
      .maxlen   = sizeof(ath_debug),
      .proc_handler = proc_dointvec
    },
#endif
    { .ctl_name = CTL_AUTO,
      .procname = "countrycode",
      .mode     = 0444,
      .data     = &ath_countrycode,
      .maxlen   = sizeof(ath_countrycode),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "regdomain",
      .mode     = 0444,
      .data     = &ath_regdomain,
      .maxlen   = sizeof(ath_regdomain),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "outdoor",
      .mode     = 0444,
      .data     = &ath_outdoor,
      .maxlen   = sizeof(ath_outdoor),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "xchanmode",
      .mode     = 0444,
      .data     = &ath_xchanmode,
      .maxlen   = sizeof(ath_xchanmode),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "calibrate",
      .mode     = 0644,
      .data     = &ath_calinterval,
      .maxlen   = sizeof(ath_calinterval),
      .extra1   = &mincalibrate,
      .extra2   = &maxint,
      .proc_handler = proc_dointvec_minmax
    },
    { .ctl_name = CTL_AUTO,
      .procname = "noreset",
      .mode     = 0644,
      .data     = &ath_noreset,
      .maxlen   = sizeof(ath_noreset),
      .proc_handler = proc_dointvec
    },
#ifdef CES_DEMO
    { .ctl_name = CTL_AUTO,
      .procname = "athrate0123ht",
      .mode     = 0644,
      .data     = &ath_htrate_0123,
      .maxlen   = sizeof(ath_htrate_0123),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "athrate0123legacy",
      .mode     = 0644,
      .data     = &ath_legacyrate_0123,
      .maxlen   = sizeof(ath_legacyrate_0123),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "athtries0123",
      .mode     = 0644,
      .data     = &ath_rate_tries_0123,
      .maxlen   = sizeof(ath_rate_tries_0123),
      .proc_handler = proc_dointvec
    },
#endif
#ifdef ATH_CHAINMASK_SELECT
    { .ctl_name = CTL_AUTO,
      .procname = "ath_cm_enable",
      .mode     = 0644,
      .data     = &ath_chainmask_sel_enable,
      .maxlen   = sizeof(ath_chainmask_sel_enable),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "ath_cm_up_rssi_thres",
      .mode     = 0644,
      .data     = &ath_chainmask_sel_up_rssi_thres,
      .maxlen   = sizeof(ath_chainmask_sel_up_rssi_thres),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "ath_cm_down_rssi_thres",
      .mode     = 0644,
      .data     = &ath_chainmask_sel_down_rssi_thres,
      .maxlen   = sizeof(ath_chainmask_sel_down_rssi_thres),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "ath_cm_timer_period",
      .mode     = 0644,
      .data     = &ath_chainmask_sel_period,
      .maxlen   = sizeof(ath_chainmask_sel_period),
      .proc_handler = proc_dointvec
    },
#endif
#ifdef ATH_FORCE_PPM
    { .ctl_name = CTL_AUTO,
      .procname = "forcePpmEnable",
      .mode     = 0644,
      .data     = &ath_force_ppm_enable,
      .maxlen   = sizeof(ath_force_ppm_enable),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "forcePpmPeriod",
      .mode     = 0644,
      .data     = &ath_force_ppm_period,
      .maxlen   = sizeof(ath_force_ppm_period),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "forcePpmRxWd",
      .mode     = 0644,
      .data     = &ath_force_ppm_wd,
      .maxlen   = sizeof(ath_force_ppm_wd),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "forcePpmTo",
      .mode     = 0644,
      .data     = &ath_force_ppm_to,
      .maxlen   = sizeof(ath_force_ppm_to),
      .proc_handler = proc_dointvec
    },
#endif /* ATH_FORCE_PPM */
    { .ctl_name = CTL_AUTO,
      .procname = "aggrProt",
      .mode     = 0644,
      .data     = &ath_aggr_prot,
      .maxlen   = sizeof(ath_aggr_prot),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "aggrProtDuration",
      .mode     = 0644,
      .data     = &ath_aggr_prot_duration,
      .maxlen   = sizeof(ath_aggr_prot_duration),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "aggrProtMax",
      .mode     = 0644,
      .data     = &ath_aggr_prot_max,
      .maxlen   = sizeof(ath_aggr_prot_max),
      .proc_handler = proc_dointvec
    },
    { 0 }
};

static ctl_table ath_ath_table[] = {
    { .ctl_name = DEV_ATH,
      .procname = "ath",
      .mode     = 0555,
      .child    = ath_static_sysctls
    }, { 0 }
};
static ctl_table ath_root_table[] = {
    { .ctl_name = CTL_DEV,
      .procname = "dev",
      .mode     = 0555,
      .child    = ath_ath_table
    }, { 0 }
};
static struct ctl_table_header *ath_sysctl_header;

void
ath_sysctl_register(void)
{
    static int initialized = 0;

    if (!initialized) {
        ath_sysctl_header =
            register_sysctl_table(ath_root_table, 1);
        initialized = 1;
    }
}

void
ath_sysctl_unregister(void)
{
    if (ath_sysctl_header)
        unregister_sysctl_table(ath_sysctl_header);
}

static HAL_BOOL
ath_fastcc_check(struct ath_softc *sc, struct ieee80211_channel *chan)
{
    struct ieee80211com *ic;
    struct ieee80211_channel *cur_chan;
    int cur_chan_rate, new_chan_rate;

    ic = &sc->sc_ic;
    cur_chan = ic->ic_curchan;

    KASSERT(ic, ("channel change on empty ic"));
    KASSERT(chan, ("channel change on NULL channel"));

    /*
     * fast channel change not configured
     */
    if (!(ic->ic_flags_ext & IEEE80211_FAST_CC))
        return AH_FALSE;

    /*
     * changing channels across modes
     */
    if (ieee80211_chan2mode(chan) != ieee80211_chan2mode(cur_chan))
        return AH_FALSE;

    /*
     * changing channels across 1/4-1/2 rate
     */
    cur_chan_rate = cur_chan->ic_flags &
                    (IEEE80211_CHAN_HALF | IEEE80211_CHAN_QUARTER);
    new_chan_rate = chan->ic_flags &
                    (IEEE80211_CHAN_HALF | IEEE80211_CHAN_QUARTER);
    if (cur_chan_rate != new_chan_rate)
        return AH_FALSE;

    return AH_TRUE;
}
#endif /* CONFIG_SYSCTL */
