/*-
 * Copyright (c) 2001 Atsushi Onoe
 * Copyright (c) 2002-2005 Sam Leffler, Errno Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

__FBSDID("$FreeBSD: src/sys/net80211/ieee80211_input.c,v 1.28 2004/12/31 22:42:38 sam Exp $");

/*
 * IEEE 802.11 input handling.
 */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/if_vlan.h>

#include "if_llc.h"
#include "if_ethersubr.h"
#include "if_media.h"
#include "if_athproto.h"

#include <net80211/ieee80211_var.h>

#if !defined(__OPTIMIZE__)
#define htonl(x) __cpu_to_be32(x)
#define htons(x) __cpu_to_be16(x)
#define ntohl(x) __be32_to_cpu(x)
#define ntohs(x) __be16_to_cpu(x)
#endif
#ifdef IEEE80211_DEBUG

extern u_int32_t ath_htvendorie_enable;
extern u_int32_t ath_htoui;
extern u_int32_t ath_htcapid;
extern u_int32_t ath_htinfoid;
extern u_int32_t ath_htdupie_enable;
/*
 * Decide if a received management frame should be
 * printed when debugging is enabled.  This filters some
 * of the less interesting frames that come frequently
 * (e.g. beacons).
 */
static __inline int
doprint(struct ieee80211vap *vap, int subtype)
{
    switch (subtype) {
    case IEEE80211_FC0_SUBTYPE_BEACON:
        return (vap->iv_ic->ic_flags & IEEE80211_F_SCAN);
    case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
        return (vap->iv_opmode == IEEE80211_M_IBSS);
    }
    return 1;
}

/*
 * Emit a debug message about discarding a frame or information
 * element.  One format is for extracting the mac address from
 * the frame header; the other is for when a header is not
 * available or otherwise appropriate.
 */
#define IEEE80211_DISCARD(_vap, _m, _wh, _type, _fmt, ...) do {     \
    if ((_vap)->iv_debug & (_m))                    \
        ieee80211_discard_frame(_vap, _wh, _type, _fmt, __VA_ARGS__);\
} while (0)
#define IEEE80211_DISCARD_IE(_vap, _m, _wh, _type, _fmt, ...) do {  \
    if ((_vap)->iv_debug & (_m))                    \
        ieee80211_discard_ie(_vap, _wh, _type, _fmt, __VA_ARGS__);\
} while (0)
#define IEEE80211_DISCARD_MAC(_vap, _m, _mac, _type, _fmt, ...) do {    \
    if ((_vap)->iv_debug & (_m))                    \
        ieee80211_discard_mac(_vap, _mac, _type, _fmt, __VA_ARGS__);\
} while (0)

static const u_int8_t *ieee80211_getbssid(struct ieee80211vap *,
    const struct ieee80211_frame *);
static void ieee80211_discard_frame(struct ieee80211vap *,
    const struct ieee80211_frame *, const char *type, const char *fmt, ...);
static void ieee80211_discard_ie(struct ieee80211vap *,
    const struct ieee80211_frame *, const char *type, const char *fmt, ...);
static void ieee80211_discard_mac(struct ieee80211vap *,
    const u_int8_t mac[IEEE80211_ADDR_LEN], const char *type,
    const char *fmt, ...);
#else
#define IEEE80211_DISCARD(_vap, _m, _wh, _type, _fmt, ...)
#define IEEE80211_DISCARD_IE(_vap, _m, _wh, _type, _fmt, ...)
#define IEEE80211_DISCARD_MAC(_vap, _m, _mac, _type, _fmt, ...)
#endif /* IEEE80211_DEBUG */

static struct sk_buff *ieee80211_defrag(struct ieee80211_node *,
        struct sk_buff *, int);
static void ieee80211_deliver_data(struct ieee80211_node *, struct sk_buff *);
static struct sk_buff *ieee80211_decap(struct ieee80211vap *,
        struct sk_buff *, int);
static void ieee80211_send_error(struct ieee80211_node *, const u_int8_t *mac,
        int subtype, int arg);
static void ieee80211_recv_pspoll(struct ieee80211_node *, struct sk_buff *);

static void athff_decap(struct sk_buff *skb);
#ifdef USE_HEADERLEN_RESV
static unsigned short ath_eth_type_trans(struct sk_buff *skb, struct net_device *dev);
#endif

static int
ieee80211_amsdu_input(struct ieee80211_node *ni, struct sk_buff *skb);

/* Since the A-MSDU sub-frame header is same as the 802.3 header, 
 * conversion of sub-frame to dix frame is same as fast-frame decap */
#define amsdu_subframe_decap(_skb) athff_decap(_skb)

 
/*
 * Process a received frame.  The node associated with the sender
 * should be supplied.  If nothing was found in the node table then
 * the caller is assumed to supply a reference to ic_bss instead.
 * The RSSI and a timestamp are also supplied.  The RSSI data is used
 * during AP scanning to select a AP to associate with; it can have
 * any units so long as values have consistent units and higher values
 * mean ``better signal''.  The receive timestamp is currently not used
 * by the 802.11 layer.
 */
int
ieee80211_input(struct ieee80211_node *ni,
    struct sk_buff *skb, int rssi, u_int32_t rstamp)
{
#define HAS_SEQ(type)   ((type & 0x4) == 0)
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_node *ni_wds=NULL;
    struct net_device *dev = vap->iv_dev;
    struct ieee80211_frame *wh;
    struct ieee80211_key *key;
    struct ether_header *eh;
#ifdef ATH_SUPERG_FF
    struct llc *llc;
#endif
    int hdrspace;
    u_int8_t dir, type, subtype;
    u_int8_t *bssid;
    u_int16_t rxseq;
    int is_amsdu = 0;

    KASSERT(ni != NULL, ("null node"));
    ni->ni_inact = ni->ni_inact_reload;

    KASSERT(skb->len >= sizeof(struct ieee80211_frame_min),
        ("frame length too short: %u", skb->len));

    /* XXX adjust device in sk_buff? */

    type = -1;          /* undefined */
    /*
     * In monitor mode, send everything directly to bpf.
     * Also do not process frames w/o i_addr2 any further.
     * XXX may want to include the CRC
     */
    if (vap->iv_opmode == IEEE80211_M_MONITOR)
        goto out;

    if (skb->len < sizeof(struct ieee80211_frame_min)) {
        IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_ANY,
            ni->ni_macaddr, NULL,
            "too short (1): len %u", skb->len);
        vap->iv_stats.is_rx_tooshort++;
        goto out;
    }
    /*
     * Bit of a cheat here, we use a pointer for a 3-address
     * frame format but don't reference fields past outside
     * ieee80211_frame_min w/o first validating the data is
     * present.
     */
    wh = (struct ieee80211_frame *)skb->data;

    if ((wh->i_fc[0] & IEEE80211_FC0_VERSION_MASK) !=
        IEEE80211_FC0_VERSION_0) {
        IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_ANY,
            ni->ni_macaddr, NULL, "wrong version %x", wh->i_fc[0]);
        vap->iv_stats.is_rx_badversion++;
        goto err;
    }

    dir = wh->i_fc[1] & IEEE80211_FC1_DIR_MASK;
    type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
    subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;
    if ((ic->ic_flags & IEEE80211_F_SCAN) == 0) {
        switch (vap->iv_opmode) {
        case IEEE80211_M_STA:
            bssid = wh->i_addr2;
            if (!IEEE80211_ADDR_EQ(bssid, ni->ni_bssid)) {
                /* not interested in */
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                    bssid, NULL, "%s", "not to bss");
                vap->iv_stats.is_rx_wrongbss++;
                goto out;
            }
            break;
        case IEEE80211_M_IBSS:
        case IEEE80211_M_AHDEMO:
            if (dir != IEEE80211_FC1_DIR_NODS)
                bssid = wh->i_addr1;
            else if (type == IEEE80211_FC0_TYPE_CTL)
                bssid = wh->i_addr1;
            else {
                if (skb->len < sizeof(struct ieee80211_frame)) {
                    IEEE80211_DISCARD_MAC(vap,
                        IEEE80211_MSG_ANY, ni->ni_macaddr,
                        NULL, "too short (2): len %u",
                        skb->len);
                    vap->iv_stats.is_rx_tooshort++;
                    goto out;
                }
                bssid = wh->i_addr3;
            }
            if (type == IEEE80211_FC0_TYPE_DATA &&
                ni == vap->iv_bss) {
                /*
                 * Fake up a node for this newly discovered
                 * member of the IBSS.  This should probably
                 * done after an ACL check.
                 */
                ni = ieee80211_fakeup_adhoc_node(vap,
                        wh->i_addr2);
                if (ni == NULL) {
                    /* NB: stat kept for alloc failure */
                    goto err;
                }
            }
            break;
        case IEEE80211_M_HOSTAP:
            if (dir != IEEE80211_FC1_DIR_NODS)
                bssid = wh->i_addr1;
            else if (type == IEEE80211_FC0_TYPE_CTL)
                bssid = wh->i_addr1;
            else {
                if (skb->len < sizeof(struct ieee80211_frame)) {
                    IEEE80211_DISCARD_MAC(vap,
                        IEEE80211_MSG_ANY, ni->ni_macaddr,
                        NULL, "too short (2): len %u",
                        skb->len);
                    vap->iv_stats.is_rx_tooshort++;
                    goto out;
                }
                bssid = wh->i_addr3;
            }
            /*
             * Validate the bssid.
             * Let beacons get through though for 11g protection mode.
             */
#ifdef ATH_SUPERG_XR
            if (!IEEE80211_ADDR_EQ(bssid, vap->iv_bss->ni_bssid) &&
		!IEEE80211_ADDR_EQ(bssid, dev->broadcast) && 
		subtype != IEEE80211_FC0_SUBTYPE_BEACON) {
                /*
                 * allow MGT frames to vap->iv_xrvap.
                 * this will allow roaming between  XR and normal vaps
                 * without station dis associating from previous vap.
                 */
                if(!(vap->iv_xrvap &&
                     IEEE80211_ADDR_EQ(bssid, vap->iv_xrvap->iv_bss->ni_bssid) &&
                     type == IEEE80211_FC0_TYPE_MGT &&
                     ni != vap->iv_bss)) {
                    /* not interested in */
                    IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                                          bssid, NULL, "%s", "not to bss or xrbss");
                    vap->iv_stats.is_rx_wrongbss++;
                    goto out;
                }
            }
#else
            if (!IEEE80211_ADDR_EQ(bssid, vap->iv_bss->ni_bssid) &&
	        !IEEE80211_ADDR_EQ(bssid, dev->broadcast) &&
	        subtype != IEEE80211_FC0_SUBTYPE_BEACON) {
                /* not interested in */
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                    bssid, NULL, "%s", "not to bss");
                vap->iv_stats.is_rx_wrongbss++;
                goto out;
            }

#endif
            break;
        case IEEE80211_M_WDS:
            if (skb->len < sizeof(struct ieee80211_frame_addr4)) {
                IEEE80211_DISCARD_MAC(vap,
                    IEEE80211_MSG_ANY, ni->ni_macaddr,
                    NULL, "too short (3): len %u",
                    skb->len);
                vap->iv_stats.is_rx_tooshort++;
                goto out;
            }
            bssid = wh->i_addr1;
            if (!IEEE80211_ADDR_EQ(bssid, vap->iv_bss->ni_bssid) &&
                !IEEE80211_ADDR_EQ(bssid, dev->broadcast)) {
                /* not interested in */
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                    bssid, NULL, "%s", "not to bss");
                vap->iv_stats.is_rx_wrongbss++;
                goto out;
            }
            break;
        default:
            /* XXX catch bad values */
            goto out;
        }
        ni->ni_rssi = rssi;
        ni->ni_rstamp = rstamp;
        if (HAS_SEQ(type)) {
            u_int8_t tid;
            if (IEEE80211_QOS_HAS_SEQ(wh)) {
                tid = ((struct ieee80211_qosframe *)wh)->
                    i_qos[0] & IEEE80211_QOS_TID;
                if (tid >= WME_AC_VI)
                    ic->ic_wme.wme_hipri_traffic++;
                tid++;
            } else
                tid = 0;
            rxseq = le16toh(*(u_int16_t *)wh->i_seq);
            if ((wh->i_fc[1] & IEEE80211_FC1_RETRY) &&
                (rxseq == ni->ni_rxseqs[tid])) {
                /* duplicate, discard */
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                    bssid, "duplicate",
                    "seqno <%u,%u> fragno <%u,%u> tid %u",
                    rxseq >> IEEE80211_SEQ_SEQ_SHIFT,
                    ni->ni_rxseqs[tid] >>
                    IEEE80211_SEQ_SEQ_SHIFT,
                    rxseq & IEEE80211_SEQ_FRAG_MASK,
                    ni->ni_rxseqs[tid] &
                    IEEE80211_SEQ_FRAG_MASK,
                    tid);
                vap->iv_stats.is_rx_dup++;
                IEEE80211_NODE_STAT(ni, rx_dup);
                goto out;
            }
            ni->ni_rxseqs[tid] = rxseq;
        }
    }

    switch (type) {
    case IEEE80211_FC0_TYPE_DATA:
        hdrspace = ieee80211_hdrspace(ic, wh);
        if (skb->len < hdrspace) {
            IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
                wh, "data", "too short: len %u, expecting %u",
                skb->len, hdrspace);
            vap->iv_stats.is_rx_tooshort++;
            goto out;       /* XXX */
        }
        switch (vap->iv_opmode) {
        case IEEE80211_M_STA:
            if ((dir != IEEE80211_FC1_DIR_FROMDS) &&
                (!((vap->iv_flags_ext & IEEE80211_FEXT_WDS) && (dir == IEEE80211_FC1_DIR_DSTODS)))) {
                IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
                          wh, "data", "invalid dir 0x%x", dir);
                vap->iv_stats.is_rx_wrongdir++;
                goto out;
            }
            if ((dev->flags & IFF_MULTICAST) &&
                IEEE80211_IS_MULTICAST(wh->i_addr1) &&
                IEEE80211_ADDR_EQ(wh->i_addr3, vap->iv_myaddr)) {
                /*
                 * In IEEE802.11 network, multicast packet
                 * sent from me is broadcasted from AP.
                 * It should be silently discarded for
                 * SIMPLEX interface.
                 *
                 * NB: Linux has no IFF_ flag to indicate
                 *     if an interface is SIMPLEX or not;
                 *     so we always assume it to be true.
                 */
                IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                    wh, NULL, "%s", "multicast echo");
                vap->iv_stats.is_rx_mcastecho++;
                goto out;
            }
            break;
        case IEEE80211_M_IBSS:
        case IEEE80211_M_AHDEMO:
            if (dir != IEEE80211_FC1_DIR_NODS) {
                IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
                    wh, "data", "invalid dir 0x%x", dir);
                vap->iv_stats.is_rx_wrongdir++;
                goto out;
            }
            /* XXX no power-save support */
            break;
        case IEEE80211_M_HOSTAP:
            if ((dir != IEEE80211_FC1_DIR_TODS) &&
                (dir != IEEE80211_FC1_DIR_DSTODS)) {
                IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
                    wh, "data", "invalid dir 0x%x", dir);
                vap->iv_stats.is_rx_wrongdir++;
                goto out;
            }
            /* check if source STA is associated */
            if (ni == vap->iv_bss) {
                IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                    wh, "data", "%s", "unknown src");
                /* NB: caller deals with reference */
                    if  (vap->iv_state == IEEE80211_S_RUN) {
                   ieee80211_send_error(ni, wh->i_addr2,
                            IEEE80211_FC0_SUBTYPE_DEAUTH,
                            IEEE80211_REASON_NOT_AUTHED);
                }
                vap->iv_stats.is_rx_notassoc++;
                goto err;
            }
            if (ni->ni_associd == 0) {
                IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                    wh, "data", "%s", "unassoc src");
                IEEE80211_SEND_MGMT(ni,
                    IEEE80211_FC0_SUBTYPE_DISASSOC,
                    IEEE80211_REASON_NOT_ASSOCED);
                vap->iv_stats.is_rx_notassoc++;
                goto err;
            }
             /* If we're a 4 address packet, make sure we have an entry in
                the node table for the packet source address (addr4).  If not,
                add one */
             if (dir == IEEE80211_FC1_DIR_DSTODS) {
                 struct ieee80211_node_table *nt;
                 struct ieee80211_frame_addr4 *wh4;
                 if (!(vap->iv_flags_ext & IEEE80211_FEXT_WDS)) {
                     IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                               wh, "data", "%s", "4 addr not allowed");
                     goto err;
                 }
                 wh4 = (struct ieee80211_frame_addr4 *)skb->data;
                 nt = &ic->ic_sta;
                 ni_wds = ieee80211_find_wds_node(nt, wh4->i_addr4);
                 /* Last call increments ref count if !NULL */
                 if ((ni_wds != NULL) && (ni_wds != ni)) {
                     IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                               wh, "data", "%s", "unassoc src");
                     IEEE80211_SEND_MGMT(ni,
                                 IEEE80211_FC0_SUBTYPE_DISASSOC,
                                 IEEE80211_REASON_NOT_ASSOCED);
                     vap->iv_stats.is_rx_notassoc++;
                     ieee80211_free_node(ni_wds); /* Decr ref count */
                     goto err;
                 }
                 if (ni_wds == NULL) {
                     ieee80211_add_wds_addr(nt, ni, wh4->i_addr4);
                 }
                 ieee80211_free_node(ni_wds); /* Decr ref count */
             }

            /*
             * Check for power save state change.
             */
            if (((wh->i_fc[1] & IEEE80211_FC1_PWR_MGT) ^
                (ni->ni_flags & IEEE80211_NODE_PWR_MGT)))
                ieee80211_node_pwrsave(ni,
                    wh->i_fc[1] & IEEE80211_FC1_PWR_MGT);
            break;
        case IEEE80211_M_WDS:
            if (dir != IEEE80211_FC1_DIR_DSTODS) {
                IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
                    wh, "data", "invalid dir 0x%x", dir);
                vap->iv_stats.is_rx_wrongdir++;
                goto out;
            }
            break;
        default:
            /* XXX here to keep compiler happy */
            goto out;
        }

        /*
         * Handle privacy requirements.  Note that we
         * must not be preempted from here until after
         * we (potentially) call ieee80211_crypto_demic;
         * otherwise we may violate assumptions in the
         * crypto cipher modules used to do delayed update
         * of replay sequence numbers.
         */
        if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
            if ((vap->iv_flags & IEEE80211_F_PRIVACY) == 0) {
                /*
                 * Discard encrypted frames when privacy is off.
                 */
                IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                    wh, "WEP", "%s", "PRIVACY off");
                vap->iv_stats.is_rx_noprivacy++;
                IEEE80211_NODE_STAT(ni, rx_noprivacy);
                goto out;
            }
            key = ieee80211_crypto_decap(ni, skb, hdrspace);
            if (key == NULL) {
                /* NB: stats+msgs handled in crypto_decap */
                IEEE80211_NODE_STAT(ni, rx_wepfail);
                goto out;
            }
            wh = (struct ieee80211_frame *)skb->data;
            wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
        } else {
            key = NULL;
        }

        /*
         * Next up, any fragmentation.
         */
        if (!IEEE80211_IS_MULTICAST(wh->i_addr1)) {
            skb = ieee80211_defrag(ni, skb, hdrspace);
            if (skb == NULL) {
                /* Fragment dropped or frame not complete yet */
                goto out;
            }
        }
        
        if(subtype == IEEE80211_FC0_SUBTYPE_QOS) {
            is_amsdu = (dir != IEEE80211_FC1_DIR_DSTODS) ?
                (((struct ieee80211_qosframe *)wh)->i_qos[0] &
                    IEEE80211_QOS_AMSDU) :
                (((struct ieee80211_qosframe_addr4 *)wh)->i_qos[0] &
                    IEEE80211_QOS_AMSDU);
        }  

        wh = NULL;      /* no longer valid, catch any uses */

        /*
         * Next strip any MSDU crypto bits.
         */
        if (key != NULL &&
            !ieee80211_crypto_demic(vap, key, skb, hdrspace, 0)) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                ni->ni_macaddr, "data", "%s", "demic error");
            IEEE80211_NODE_STAT(ni, rx_demicfail);
            goto out;
        }

        /*
         * Finally, strip the 802.11 header.
         */
        skb = ieee80211_decap(vap, skb, hdrspace);
        if (skb == NULL) {
            /* don't count Null data frames as errors */
            if (subtype == IEEE80211_FC0_SUBTYPE_NODATA)
                goto out;
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                ni->ni_macaddr, "data", "%s", "decap error");
            vap->iv_stats.is_rx_decap++;
            IEEE80211_NODE_STAT(ni, rx_decap);
            goto err;
        }
        eh = (struct ether_header *) skb->data;
        if (!ieee80211_node_is_authorized(ni)) {
            /*
             * Deny any non-PAE frames received prior to
             * authorization.  For open/shared-key
             * authentication the port is mark authorized
             * after authentication completes.  For 802.1x
             * the port is not marked authorized by the
             * authenticator until the handshake has completed.
             */
            if (eh->ether_type != __constant_htons(ETHERTYPE_PAE)) {
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
                    eh->ether_shost, "data",
                    "unauthorized port: ether type 0x%x len %u",
                    eh->ether_type, skb->len);
                vap->iv_stats.is_rx_unauth++;
                IEEE80211_NODE_STAT(ni, rx_unauth);
                goto err;
            }
        } else {
            /*
             * When denying unencrypted frames, discard
             * any non-PAE frames received without encryption.
             */
            if ((vap->iv_flags & IEEE80211_F_DROPUNENC) &&
                key == NULL &&
                eh->ether_type != __constant_htons(ETHERTYPE_PAE)) {
                /*
                 * Drop unencrypted frames.
                 */
                vap->iv_stats.is_rx_unencrypted++;
                IEEE80211_NODE_STAT(ni, rx_unencrypted);
                goto out;
            }
        }
        vap->iv_devstats.rx_packets++;
        vap->iv_devstats.rx_bytes += skb->len;
        IEEE80211_NODE_STAT(ni, rx_data);
        IEEE80211_NODE_STAT_ADD(ni, rx_bytes, skb->len);
        ic->ic_lastdata = jiffies;

        if(is_amsdu) {
            return ieee80211_amsdu_input(ni, skb);
        }
#ifdef ATH_SUPERG_FF
        /* check for FF */
        llc = (struct llc *) (skb->data + sizeof(struct ether_header));
        if (ntohs(llc->llc_snap.ether_type) == (u_int16_t)ATH_ETH_TYPE){
            struct sk_buff *skb1 = NULL;
            struct ether_header *eh_tmp;
            struct athl2p_tunnel_hdr *ath_hdr;
            int frame_len;

            /* NB: assumes linear (i.e., non-fragmented) skb */

            /* get to the tunneled headers */
            ath_hdr = (struct athl2p_tunnel_hdr *)
                skb_pull(skb, sizeof(struct ether_header) + LLC_SNAPFRAMELEN);
            /* ignore invalid frames */
            if(ath_hdr == NULL)
            {
                goto err;
            }

            /* only implementing FF now. drop all others. */
            if (ath_hdr->proto != ATH_L2TUNNEL_PROTO_FF) {
                IEEE80211_DISCARD_MAC(vap,
                    IEEE80211_MSG_SUPG | IEEE80211_MSG_INPUT,
                    eh->ether_shost, "fast-frame",
                    "bad atheros tunnel prot %u",
                    ath_hdr->proto);
                vap->iv_stats.is_rx_badathtnl++;
                goto err;
            }
            vap->iv_stats.is_rx_ffcnt++;

            /* move past the tunneled header, with alignment */
            skb_pull(skb, roundup(sizeof(struct athl2p_tunnel_hdr) - 2, 4) + 2);

            skb1 = skb_clone(skb, GFP_ATOMIC); /* XXX: GFP_ATOMIC is overkill? */
            eh_tmp = (struct ether_header *)skb->data;

            /* ether_type must be length*/
            frame_len = ntohs(eh_tmp->ether_type);

            /* we now have 802.3 MAC hdr followed by 802.2 LLC/SNAP. convert to DIX */
            athff_decap(skb);

            /* remove second frame from end of first */
            skb_trim(skb, sizeof(struct ether_header) + frame_len - LLC_SNAPFRAMELEN);

            /* prepare second tunneled frame */
            skb_pull(skb1, roundup(sizeof(struct ether_header) + frame_len, 4));
            eh_tmp = (struct ether_header *)skb1->data;
            frame_len = ntohs(eh_tmp->ether_type);
            athff_decap(skb1);

            /* deliver the frames */
            ieee80211_deliver_data(ni, skb);
            ieee80211_deliver_data(ni, skb1);
        }
        else {
            /* assume non-atheros llc type */
            ieee80211_deliver_data(ni, skb);
        }
#else /* !ATH_SUPERG_FF */
        ieee80211_deliver_data(ni, skb);
#endif
        return IEEE80211_FC0_TYPE_DATA;

    case IEEE80211_FC0_TYPE_MGT:
        IEEE80211_NODE_STAT(ni, rx_mgmt);
        if (dir != IEEE80211_FC1_DIR_NODS) {
            vap->iv_stats.is_rx_wrongdir++;
            goto err;
        }
        if (skb->len < sizeof(struct ieee80211_frame)) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_ANY,
                ni->ni_macaddr, "mgt", "too short: len %u",
                skb->len);
            vap->iv_stats.is_rx_tooshort++;
            goto out;
        }
#ifdef IEEE80211_DEBUG
        if ((ieee80211_msg_debug(vap) && doprint(vap, subtype)) ||
            ieee80211_msg_dumppkts(vap)) {
            ieee80211_note(vap, "received %s from %s rssi %d\n",
                ieee80211_mgt_subtype_name[subtype >>
                IEEE80211_FC0_SUBTYPE_SHIFT],
                ether_sprintf(wh->i_addr2), rssi);
        }
#endif
        if (wh->i_fc[1] & IEEE80211_FC1_WEP) {
            if (subtype != IEEE80211_FC0_SUBTYPE_AUTH) {
                /*
                 * Only shared key auth frames with a challenge
                 * should be encrypted, discard all others.
                 */
                IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                    wh, ieee80211_mgt_subtype_name[subtype >>
                    IEEE80211_FC0_SUBTYPE_SHIFT],
                    "%s", "WEP set but not permitted");
                vap->iv_stats.is_rx_mgtdiscard++; /* XXX */
                goto out;
            }
            if ((vap->iv_flags & IEEE80211_F_PRIVACY) == 0) {
                /*
                 * Discard encrypted frames when privacy is off.
                 */
                IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                    wh, "mgt", "%s", "WEP set but PRIVACY off");
                vap->iv_stats.is_rx_noprivacy++;
                goto out;
            }
            hdrspace = ieee80211_hdrspace(ic, wh);
            key = ieee80211_crypto_decap(ni, skb, hdrspace);
            if (key == NULL) {
                /* NB: stats+msgs handled in crypto_decap */
                goto out;
            }
            wh = (struct ieee80211_frame *)skb->data;
            wh->i_fc[1] &= ~IEEE80211_FC1_WEP;
        }
        ic->ic_recv_mgmt(ni, skb, subtype, rssi, rstamp);
        goto out;

    case IEEE80211_FC0_TYPE_CTL:
        IEEE80211_NODE_STAT(ni, rx_ctrl);
        vap->iv_stats.is_rx_ctl++;
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            switch (subtype) {
            case IEEE80211_FC0_SUBTYPE_PS_POLL:
                ieee80211_recv_pspoll(ni, skb);
                break;
            }
        }
        goto out;

    default:
        IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
            wh, NULL, "bad frame type 0x%x", type);
        /* should not come here */
        break;
    }
err:
    vap->iv_devstats.rx_errors++;
out:
    if (skb != NULL)
        dev_kfree_skb(skb);
    return type;
#undef HAS_SEQ
}
EXPORT_SYMBOL(ieee80211_input);

int
ieee80211_input_all(struct ieee80211com *ic,
    struct sk_buff *skb, int rssi, u_int32_t rstamp)
{
    struct ieee80211vap *vap;
    int type = -1;

    /* XXX locking */
    TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
        struct ieee80211_node *ni;
        struct sk_buff *skb1;

        if (TAILQ_NEXT(vap, iv_next) != NULL) {
            skb1 = skb_copy(skb, GFP_ATOMIC);
            if (skb1 == NULL) {
                /* XXX stat+msg */
                continue;
            }
        } else {
            skb1 = skb;
            skb = NULL;
        }
        ni = ieee80211_ref_node(vap->iv_bss);
        type = ieee80211_input(ni, skb1, rssi, rstamp);
        ieee80211_free_node(ni);
    }
    if (skb != NULL)        /* no vaps, reclaim skb */
        dev_kfree_skb(skb);
    return type;
}
EXPORT_SYMBOL(ieee80211_input_all);

/*
 * This function reassemble fragments using the skb of the 1st fragment,
 * if large enough. If not, a new skb is allocated to hold incoming
 * fragments.
 *
 * Fragments are copied at the end of the previous fragment.  A different
 * strategy could have been used, where a non-linear skb is allocated and
 * fragments attached to that skb.
 */
static struct sk_buff *
ieee80211_defrag(struct ieee80211_node *ni, struct sk_buff *skb, int hdrlen)
{
    struct ieee80211_frame *wh = (struct ieee80211_frame *) skb->data;
    u_int16_t rxseq, last_rxseq;
    u_int8_t fragno, last_fragno;
    u_int8_t more_frag = wh->i_fc[1] & IEEE80211_FC1_MORE_FRAG;

    rxseq = le16_to_cpu(*(u_int16_t *)wh->i_seq) >> IEEE80211_SEQ_SEQ_SHIFT;
    fragno = le16_to_cpu(*(u_int16_t *)wh->i_seq) & IEEE80211_SEQ_FRAG_MASK;

    /* Quick way out, if there's nothing to defragment */
    if (!more_frag && fragno == 0 && ni->ni_rxfrag[0] == NULL)
        return skb;

    /*
     * Remove frag to insure it doesn't get reaped by timer.
     */
    if (ni->ni_table == NULL) {
        /*
         * Should never happen.  If the node is orphaned (not in
         * the table) then input packets should not reach here.
         * Otherwise, a concurrent request that yanks the table
         * should be blocked by other interlocking and/or by first
         * shutting the driver down.  Regardless, be defensive
         * here and just bail
         */
        /* XXX need msg+stat */
        dev_kfree_skb(skb);
        return NULL;
    }

    /*
     * Use this lock to make sure ni->ni_rxfrag[0] is
     * not freed by the timer process while we use it.
     * XXX bogus
     */
    IEEE80211_NODE_LOCK(ni->ni_table);

    /*
     * Update the time stamp.  As a side effect, it
     * also makes sure that the timer will not change
     * ni->ni_rxfrag[0] for at least 1 second, or in
     * other words, for the remaining of this function.
     */
    ni->ni_rxfragstamp = jiffies;

    IEEE80211_NODE_UNLOCK(ni->ni_table);

    /*
     * Validate that fragment is in order and
     * related to the previous ones.
     */
    if (ni->ni_rxfrag[0]) {
        struct ieee80211_frame *lwh;

        lwh = (struct ieee80211_frame *) ni->ni_rxfrag[0]->data;
        last_rxseq = le16_to_cpu(*(u_int16_t *)lwh->i_seq) >>
            IEEE80211_SEQ_SEQ_SHIFT;
        last_fragno = le16_to_cpu(*(u_int16_t *)lwh->i_seq) &
            IEEE80211_SEQ_FRAG_MASK;
        if (rxseq != last_rxseq
            || fragno != last_fragno + 1
            || (!IEEE80211_ADDR_EQ(wh->i_addr1, lwh->i_addr1))
            || (!IEEE80211_ADDR_EQ(wh->i_addr2, lwh->i_addr2))
            || (ni->ni_rxfrag[0]->end - ni->ni_rxfrag[0]->tail <
                skb->len)) {
            /*
             * Unrelated fragment or no space for it,
             * clear current fragments
             */
            dev_kfree_skb(ni->ni_rxfrag[0]);
            ni->ni_rxfrag[0] = NULL;
        }
    }

    /* If this is the first fragment */
    if (ni->ni_rxfrag[0] == NULL && fragno == 0) {
        ni->ni_rxfrag[0] = skb;
        /* If more frags are coming */
        if (more_frag) {
            if (skb_is_nonlinear(skb)) {
                /*
                 * We need a continous buffer to
                 * assemble fragments
                 */
                ni->ni_rxfrag[0] = skb_copy(skb, GFP_ATOMIC);
                dev_kfree_skb(skb);
            }
            /*
             * Check that we have enough space to hold
             * incoming fragments
             * XXX 4-address/QoS frames?
             */
            else if (skb->end - skb->head < ni->ni_vap->iv_dev->mtu +
                 hdrlen) {
                ni->ni_rxfrag[0] = skb_copy_expand(skb, 0,
                                   (ni->ni_vap->iv_dev->mtu + hdrlen)
                                   - (skb->end - skb->head), GFP_ATOMIC);
                dev_kfree_skb(skb);
            }
        }
    } else {
        if (ni->ni_rxfrag[0]) {
            struct ieee80211_frame *lwh = (struct ieee80211_frame *)
                ni->ni_rxfrag[0]->data;

            /*
             * We know we have enough space to copy,
             * we've verified that before
             */
            /* Copy current fragment at end of previous one */
            memcpy(ni->ni_rxfrag[0]->tail,
                   skb->data + hdrlen, skb->len - hdrlen);
            /* Update tail and length */
            skb_put(ni->ni_rxfrag[0], skb->len - hdrlen);
            /* Keep a copy of last sequence and fragno */
            *(u_int16_t *) lwh->i_seq = *(u_int16_t *) wh->i_seq;
        }
        /* we're done with the fragment */
        dev_kfree_skb(skb);
    }

    if (more_frag) {
        /* More to come */
        skb = NULL;
    } else {
        /* Last fragment received, we're done! */
        skb = ni->ni_rxfrag[0];
        ni->ni_rxfrag[0] = NULL;
    }
    return skb;
}

static void
ieee80211_deliver_data(struct ieee80211_node *ni, struct sk_buff *skb)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct net_device *dev = vap->iv_dev;
    struct ether_header *eh = (struct ether_header *) skb->data;

#ifdef ATH_SUPERG_XR
    /*
     * if it is a XR vap, send the data to associated normal net
     * device. XR vap has a net device which is not registered with
     * OS.
     */
    if (vap->iv_flags & IEEE80211_F_XR) {
        dev = vap->iv_xrvap->iv_dev;
    }
#endif

    /* perform as a bridge within the vap */
    /* XXX intra-vap bridging only */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP &&
        (vap->iv_flags & IEEE80211_F_NOBRIDGE) == 0) {
        struct sk_buff *skb1 = NULL;

        if (ETHER_IS_MULTICAST(eh->ether_dhost)) {
            skb1 = skb_copy(skb, GFP_ATOMIC);
        } else {
            /*
             * Check if destination is associated with the
             * same vap and authorized to receive traffic.
             * Beware of traffic destined for the vap itself;
             * sending it will not work; just let it be
             * delivered normally.
             */
            struct ieee80211_node *ni1 = ieee80211_find_node(
                &vap->iv_ic->ic_sta, eh->ether_dhost);
            if (ni1 != NULL) {
                if (ni1->ni_vap == vap &&
                    ieee80211_node_is_authorized(ni1) &&
                    ni1 != vap->iv_bss) {
                    skb1 = skb;
                    skb = NULL;
                }
                /* XXX statistic? */
                ieee80211_free_node(ni1);
            }
        }
        if (skb1 != NULL) {
            skb1->dev = dev;
            skb1->mac.raw = skb1->data;
            skb1->nh.raw = skb1->data +
                sizeof(struct ether_header);
            skb1->protocol = __constant_htons(ETH_P_802_2);
            /* XXX inser`t vlan tage before queue it? */
            dev_queue_xmit(skb1);
        }
    }

    if (skb != NULL) {
        skb->dev = dev;

#ifdef USE_HEADERLEN_RESV
        skb->protocol = ath_eth_type_trans(skb, dev);
#else
        skb->protocol = eth_type_trans(skb, dev);
#endif
        if (ni->ni_vlan != 0 && vap->iv_vlgrp != NULL) {
            /* attach vlan tag */
            vlan_hwaccel_receive_skb(skb, vap->iv_vlgrp, ni->ni_vlan);
        } else {
            netif_rx(skb);
        }
        dev->last_rx = jiffies;
    }
}

static struct sk_buff *
ieee80211_decap(struct ieee80211vap *vap, struct sk_buff *skb, int hdrlen)
{
    struct ieee80211_qosframe_addr4 wh; /* Max size address frames */
    struct ether_header *eh;
    struct llc *llc;
    u_short ether_type = 0;

    memcpy(&wh, skb->data, hdrlen); /* Only copy hdrlen over */
    llc = (struct llc *) skb_pull(skb, hdrlen);
    if (skb->len >= LLC_SNAPFRAMELEN &&
        llc->llc_dsap == LLC_SNAP_LSAP && llc->llc_ssap == LLC_SNAP_LSAP &&
        llc->llc_control == LLC_UI && llc->llc_snap.org_code[0] == 0 &&
        llc->llc_snap.org_code[1] == 0 && llc->llc_snap.org_code[2] == 0) {
        ether_type = llc->llc_un.type_snap.ether_type;
        skb_pull(skb, LLC_SNAPFRAMELEN);
        llc = NULL;
    }
    eh = (struct ether_header *) skb_push(skb, sizeof(struct ether_header));
    switch (wh.i_fc[1] & IEEE80211_FC1_DIR_MASK) {
    case IEEE80211_FC1_DIR_NODS:
        IEEE80211_ADDR_COPY(eh->ether_dhost, wh.i_addr1);
        IEEE80211_ADDR_COPY(eh->ether_shost, wh.i_addr2);
        break;
    case IEEE80211_FC1_DIR_TODS:
        IEEE80211_ADDR_COPY(eh->ether_dhost, wh.i_addr3);
        IEEE80211_ADDR_COPY(eh->ether_shost, wh.i_addr2);
        break;
    case IEEE80211_FC1_DIR_FROMDS:
        IEEE80211_ADDR_COPY(eh->ether_dhost, wh.i_addr1);
        IEEE80211_ADDR_COPY(eh->ether_shost, wh.i_addr3);
        break;
    case IEEE80211_FC1_DIR_DSTODS:
        IEEE80211_ADDR_COPY(eh->ether_dhost, wh.i_addr3);
        IEEE80211_ADDR_COPY(eh->ether_shost, wh.i_addr4);
        break;
    }
    if (!ALIGNED_POINTER(skb->data + sizeof(*eh), u_int32_t)) {
        struct sk_buff *n;

        /* XXX does this always work? */
        n = skb_copy(skb, GFP_ATOMIC);
        dev_kfree_skb(skb);
        if (n == NULL)
            return NULL;
        skb = n;
        eh = (struct ether_header *) skb->data;
    }
    if (llc != NULL)
        eh->ether_type = htons(skb->len - sizeof(*eh));
    else
        eh->ether_type = ether_type;
    return skb;
}

/*
 * Install received rate set information in the node's state block.
 */
int
ieee80211_setup_rates(struct ieee80211_node *ni,
    const u_int8_t *rates, const u_int8_t *xrates, int flags)
{
    struct ieee80211_rateset *rs = &ni->ni_rates;
    struct ieee80211_rateset rrs, *irs;
    struct ieee80211com *ic = ni->ni_ic;

    memset(&rrs, 0, sizeof(rrs));
    memset(rs, 0, sizeof(*rs));
    rrs.rs_nrates = rates[1];
    memcpy(rrs.rs_rates, rates + 2, rrs.rs_nrates);
    if (xrates != NULL) {
        u_int8_t nxrates;
        /*
         * Tack on 11g extended supported rate element.
         */
        nxrates = xrates[1];
        if (rrs.rs_nrates + nxrates > IEEE80211_RATE_MAXSIZE) {
            struct ieee80211vap *vap = ni->ni_vap;

            nxrates = IEEE80211_RATE_MAXSIZE - rrs.rs_nrates;
            IEEE80211_NOTE(vap, IEEE80211_MSG_XRATE, ni,
                "extended rate set too large;"
                " only using %u of %u rates",
                nxrates, xrates[1]);
            vap->iv_stats.is_rx_rstoobig++;
        }
        memcpy(rrs.rs_rates + rrs.rs_nrates, xrates+2, nxrates);
        rrs.rs_nrates += nxrates;
    }
    irs =  &ic->ic_sup_rates[ieee80211_chan2mode(ni->ni_chan)];
    if (flags & IEEE80211_F_DOSORT)
        ieee80211_sort_rate(&rrs);

    if (flags & IEEE80211_F_DOXSECT)
        ieee80211_xsect_rate(irs, &rrs, rs);
    else
        memcpy(rs, &rrs, sizeof(rrs));

    if (flags & IEEE80211_F_DOFRATE)
        if (!ieee80211_fixed_rate_check(ni, rs))
            return 0;

    if (flags & IEEE80211_F_DOBRS)
        if (!ieee80211_brs_rate_check(irs, rs))
            return 0;

    /* ieee80211_xsect_rate() called above alters the already sorted
     * rate set. So do the sorting again. */
    if (flags & IEEE80211_F_DOSORT)
        ieee80211_sort_rate(rs);

    return 1;
}

/*
 * Install received ht rate set information in the node's state block.
 */
int
ieee80211_setup_ht_rates(struct ieee80211_node *ni, u_int8_t *ie,
             int flags)
{
    struct ieee80211_ie_htcap_cmn *htcap = (struct ieee80211_ie_htcap_cmn *)ie;
    struct ieee80211_rateset *rs = &ni->ni_htrates;
    struct ieee80211_rateset rrs, *irs;
    struct ieee80211com *ic = ni->ni_ic;
    int i,j;

    memset(&rrs, 0, sizeof(rrs));
    memset(rs, 0, sizeof(*rs));
    j = 0;

    if(htcap != NULL) {
        for (i=0; i < IEEE80211_HT_RATE_SIZE; i++) {
            if (htcap->hc_mcsset[i/8] & (1<<(i%8))) {
                rrs.rs_rates[j++] = i;
            }
            if (j == IEEE80211_RATE_MAXSIZE) {
                IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_XRATE, ni,
                    "ht extended rate set too large;"
                    " only using %u rates",j);
                ni->ni_vap->iv_stats.is_rx_rstoobig++;
                break;
            }
        }
    }
    rrs.rs_nrates = j;
    irs =  &ic->ic_sup_ht_rates[ieee80211_chan2mode(ni->ni_chan)];
    if (flags & IEEE80211_F_DOSORT)
        ieee80211_sort_rate(&rrs);

    if (flags & IEEE80211_F_DOXSECT)
        ieee80211_xsect_rate(irs, &rrs, rs);
    else
        memcpy(rs, &rrs, sizeof(rrs));

    if (flags & IEEE80211_F_DOFRATE)
        if (!ieee80211_fixed_rate_check(ni, rs))
            return 0;

    if (flags & IEEE80211_F_DOBRS)
        if (!ieee80211_brs_rate_check(irs, rs))
            return 0;

    return 1;
}

void
ieee80211_setup_basic_ht_rates(struct ieee80211_node *ni, u_int8_t *ie)
{
#define RV(v)   ((v) & IEEE80211_RATE_VAL)
    struct ieee80211_ie_htinfo_cmn *htinfo = (struct ieee80211_ie_htinfo_cmn *)ie;
    struct ieee80211_rateset *rs = &ni->ni_htrates;
    int i,j;

    if (rs->rs_nrates) {
        for (i=0; i < IEEE80211_HT_RATE_SIZE; i++) {
            if (htinfo->hi_basicmcsset[i/8] & (1<<(i%8))) {
                for (j = 0; j < rs->rs_nrates; j++) {
                    if (RV(rs->rs_rates[j]) == i) {
                        rs->rs_rates[j] |= IEEE80211_RATE_BASIC;
                    }
                }
            }
        }
    } else {
        IEEE80211_NOTE(ni->ni_vap, IEEE80211_MSG_XRATE, ni,
            "ht rate set %s;", "empty");
    }
#undef RV
}

static void
ieee80211_auth_open(struct ieee80211_node *ni, struct ieee80211_frame *wh,
    int rssi, u_int32_t rstamp, u_int16_t seq, u_int16_t status)
{
    struct ieee80211vap *vap = ni->ni_vap;

    if (ni->ni_authmode == IEEE80211_AUTH_SHARED) {
        IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
            ni->ni_macaddr, "open auth",
            "bad sta auth mode %u", ni->ni_authmode);
        vap->iv_stats.is_rx_bad_auth++; /* XXX maybe a unique error? */
        if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
            /* XXX hack to workaround calling convention */

            /* XXX To send the frame to the requesting STA, we have to
             * create a node for the station that we're going to reject.
             * The node will be freed automatically */
            if (ni == vap->iv_bss) {
                ni = ieee80211_dup_bss(vap, wh->i_addr2);
                if (ni == NULL)
                    return;
            }
            IEEE80211_SEND_MGMT(ni,
                IEEE80211_FC0_SUBTYPE_AUTH,
                (seq + 1) | (IEEE80211_STATUS_ALG<<16));
            return;
        }
    }
    switch (vap->iv_opmode) {
    case IEEE80211_M_IBSS:
        if (vap->iv_state != IEEE80211_S_RUN ||
            seq != IEEE80211_AUTH_OPEN_REQUEST) {
            vap->iv_stats.is_rx_bad_auth++;
            return;
        }
        ieee80211_new_state(vap, IEEE80211_S_AUTH,
            wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
        break;

    case IEEE80211_M_AHDEMO:
    case IEEE80211_M_WDS:
        /* should not come here */
        break;

    case IEEE80211_M_HOSTAP:
        if (vap->iv_state != IEEE80211_S_RUN ||
            seq != IEEE80211_AUTH_OPEN_REQUEST) {
            vap->iv_stats.is_rx_bad_auth++;
            return;
        }
        /* always accept open authentication requests */
        if (ni == vap->iv_bss) {
            ni = ieee80211_dup_bss(vap, wh->i_addr2);
            if (ni == NULL)
                return;
        } else if ((ni->ni_flags & IEEE80211_NODE_AREF) == 0)
            (void) ieee80211_ref_node(ni);
        /*
         * Mark the node as referenced to reflect that it's
         * reference count has been bumped to insure it remains
         * after the transaction completes.
         */
        ni->ni_flags |= IEEE80211_NODE_AREF;

        IEEE80211_SEND_MGMT(ni, IEEE80211_FC0_SUBTYPE_AUTH, seq + 1);
        IEEE80211_NOTE(vap, IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH,
            ni, "station authenticated (%s)", "open");
        /*
         * When 802.1x is not in use mark the port
         * authorized at this point so traffic can flow.
         */
        if (ni->ni_authmode != IEEE80211_AUTH_8021X)
            ieee80211_node_authorize(ni);
        break;

    case IEEE80211_M_STA:
        if (vap->iv_state != IEEE80211_S_AUTH ||
            seq != IEEE80211_AUTH_OPEN_RESPONSE) {
            vap->iv_stats.is_rx_bad_auth++;
            return;
        }
        if (status != 0) {
            IEEE80211_NOTE(vap,
                IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                "open auth failed (reason %d)", status);
            vap->iv_stats.is_rx_auth_fail++;
            ieee80211_new_state(vap, IEEE80211_S_SCAN,
                IEEE80211_SCAN_FAIL_STATUS);
        } else
            ieee80211_new_state(vap, IEEE80211_S_ASSOC, 0);
        break;
    case IEEE80211_M_MONITOR:
        break;
    }
}

/*
 * Send a management frame error response to the specified
 * station.  If ni is associated with the station then use
 * it; otherwise allocate a temporary node suitable for
 * transmitting the frame and then free the reference so
 * it will go away as soon as the frame has been transmitted.
 */
static void
ieee80211_send_error(struct ieee80211_node *ni,
    const u_int8_t *mac, int subtype, int arg)
{
    struct ieee80211vap *vap = ni->ni_vap;
    int istmp;

    if (ni == vap->iv_bss) {
        ni = ieee80211_tmp_node(vap, mac);
        if (ni == NULL) {
            /* XXX msg */
            return;
        }
        istmp = 1;
    } else
        istmp = 0;
    IEEE80211_SEND_MGMT(ni, subtype, arg);
    if (istmp)
        ieee80211_free_node(ni);
}

static int
alloc_challenge(struct ieee80211_node *ni)
{
    if (ni->ni_challenge == NULL)
        MALLOC(ni->ni_challenge, u_int32_t*, IEEE80211_CHALLENGE_LEN,
            M_DEVBUF, M_NOWAIT);
    if (ni->ni_challenge == NULL) {
        IEEE80211_NOTE(ni->ni_vap,
            IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
            "%s", "shared key challenge alloc failed");
        /* XXX statistic */
    }
    return (ni->ni_challenge != NULL);
}

/* XXX TODO: add statistics */
static void
ieee80211_auth_shared(struct ieee80211_node *ni, struct ieee80211_frame *wh,
    u_int8_t *frm, u_int8_t *efrm, int rssi, u_int32_t rstamp,
    u_int16_t seq, u_int16_t status)
{
    struct ieee80211vap *vap = ni->ni_vap;
    u_int8_t *challenge;
    int allocbs, estatus;

    /*
     * NB: this can happen as we allow pre-shared key
     * authentication to be enabled w/o wep being turned
     * on so that configuration of these can be done
     * in any order.  It may be better to enforce the
     * ordering in which case this check would just be
     * for sanity/consistency.
     */
    estatus = 0;            /* NB: silence compiler */
    if ((vap->iv_flags & IEEE80211_F_PRIVACY) == 0) {
        IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
            ni->ni_macaddr, "shared key auth",
            "%s", " PRIVACY is disabled");
        estatus = IEEE80211_STATUS_ALG;
        goto bad;
    }
    /*
     * Pre-shared key authentication is evil; accept
     * it only if explicitly configured (it is supported
     * mainly for compatibility with clients like OS X).
     */
    if (ni->ni_authmode != IEEE80211_AUTH_AUTO &&
        ni->ni_authmode != IEEE80211_AUTH_SHARED) {
        IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
            ni->ni_macaddr, "shared key auth",
            "bad sta auth mode %u", ni->ni_authmode);
        vap->iv_stats.is_rx_bad_auth++; /* XXX maybe a unique error? */
        estatus = IEEE80211_STATUS_ALG;
        goto bad;
    }

    challenge = NULL;
    if (frm + 1 < efrm) {
        if ((frm[1] + 2) > (efrm - frm)) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                ni->ni_macaddr, "shared key auth",
                "ie %d/%d too long",
                frm[0], (frm[1] + 2) - (efrm - frm));
            vap->iv_stats.is_rx_bad_auth++;
            estatus = IEEE80211_STATUS_CHALLENGE;
            goto bad;
        }
        if (*frm == IEEE80211_ELEMID_CHALLENGE)
            challenge = frm;
        frm += frm[1] + 2;
    }
    switch (seq) {
    case IEEE80211_AUTH_SHARED_CHALLENGE:
    case IEEE80211_AUTH_SHARED_RESPONSE:
        if (challenge == NULL) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                ni->ni_macaddr, "shared key auth",
                "%s", "no challenge");
            vap->iv_stats.is_rx_bad_auth++;
            estatus = IEEE80211_STATUS_CHALLENGE;
            goto bad;
        }
        if (challenge[1] != IEEE80211_CHALLENGE_LEN) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                ni->ni_macaddr, "shared key auth",
                "bad challenge len %d", challenge[1]);
            vap->iv_stats.is_rx_bad_auth++;
            estatus = IEEE80211_STATUS_CHALLENGE;
            goto bad;
        }
    default:
        break;
    }
    switch (vap->iv_opmode) {
    case IEEE80211_M_MONITOR:
    case IEEE80211_M_AHDEMO:
    case IEEE80211_M_IBSS:
    case IEEE80211_M_WDS:
        IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
            ni->ni_macaddr, "shared key auth",
            "bad operating mode %u", vap->iv_opmode);
        return;
    case IEEE80211_M_HOSTAP:
        if (vap->iv_state != IEEE80211_S_RUN) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                ni->ni_macaddr, "shared key auth",
                "bad state %u", vap->iv_state);
            estatus = IEEE80211_STATUS_ALG; /* XXX */
            goto bad;
        }
        switch (seq) {
        case IEEE80211_AUTH_SHARED_REQUEST:
            if (ni == vap->iv_bss) {
                ni = ieee80211_dup_bss(vap, wh->i_addr2);
                if (ni == NULL) {
                    /* NB: no way to return an error */
                    return;
                }
                allocbs = 1;
            } else {
                if ((ni->ni_flags & IEEE80211_NODE_AREF) == 0)
                    (void) ieee80211_ref_node(ni);
                allocbs = 0;
            }
            /*
             * Mark the node as referenced to reflect that it's
             * reference count has been bumped to insure it remains
             * after the transaction completes.
             */
            ni->ni_flags |= IEEE80211_NODE_AREF;
            ni->ni_rssi = rssi;
            ni->ni_rstamp = rstamp;
            if (!alloc_challenge(ni)) {
                /* NB: don't return error so they rexmit */
                return;
            }
            get_random_bytes(ni->ni_challenge,
                IEEE80211_CHALLENGE_LEN);
            IEEE80211_NOTE(vap,
                IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                "shared key %sauth request", allocbs ? "" : "re");
            break;
        case IEEE80211_AUTH_SHARED_RESPONSE:
            if (ni == vap->iv_bss) {
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                    ni->ni_macaddr, "shared key response",
                    "%s", "unknown station");
                /* NB: don't send a response */
                return;
            }
            if (ni->ni_challenge == NULL) {
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                    ni->ni_macaddr, "shared key response",
                    "%s", "no challenge recorded");
                vap->iv_stats.is_rx_bad_auth++;
                estatus = IEEE80211_STATUS_CHALLENGE;
                goto bad;
            }
            if (memcmp(ni->ni_challenge, &challenge[2],
                       challenge[1]) != 0) {
                IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                    ni->ni_macaddr, "shared key response",
                    "%s", "challenge mismatch");
                vap->iv_stats.is_rx_auth_fail++;
                estatus = IEEE80211_STATUS_CHALLENGE;
                goto bad;
            }
            IEEE80211_NOTE(vap,
                IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH, ni,
                "station authenticated (%s)", "shared key");
            ieee80211_node_authorize(ni);
            break;
        default:
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_AUTH,
                ni->ni_macaddr, "shared key auth",
                "bad seq %d", seq);
            vap->iv_stats.is_rx_bad_auth++;
            estatus = IEEE80211_STATUS_SEQUENCE;
            goto bad;
        }
        IEEE80211_SEND_MGMT(ni, IEEE80211_FC0_SUBTYPE_AUTH, seq + 1);
        break;

    case IEEE80211_M_STA:
        if (vap->iv_state != IEEE80211_S_AUTH)
            return;
        switch (seq) {
        case IEEE80211_AUTH_SHARED_PASS:
            if (ni->ni_challenge != NULL) {
                FREE(ni->ni_challenge, M_DEVBUF);
                ni->ni_challenge = NULL;
            }
            if (status != 0) {
                IEEE80211_NOTE_MAC(vap,
                    IEEE80211_MSG_DEBUG | IEEE80211_MSG_AUTH,
                    ieee80211_getbssid(vap, wh),
                    "shared key auth failed (reason %d)",
                    status);
                vap->iv_stats.is_rx_auth_fail++;
                /* XXX IEEE80211_SCAN_FAIL_STATUS */
                goto bad;
            }
            ieee80211_new_state(vap, IEEE80211_S_ASSOC, 0);
            break;
        case IEEE80211_AUTH_SHARED_CHALLENGE:
            if (!alloc_challenge(ni))
                goto bad;
            /* XXX could optimize by passing recvd challenge */
            memcpy(ni->ni_challenge, &challenge[2], challenge[1]);
            IEEE80211_SEND_MGMT(ni,
                IEEE80211_FC0_SUBTYPE_AUTH, seq + 1);
            break;
        default:
            IEEE80211_DISCARD(vap, IEEE80211_MSG_AUTH,
                wh, "shared key auth", "bad seq %d", seq);
            vap->iv_stats.is_rx_bad_auth++;
            goto bad;
        }
        break;
    }
    return;
bad:
    /*
     * Send an error response; but only when operating as an AP.
     */
    if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
        /* XXX hack to workaround calling convention */
        ieee80211_send_error(ni, wh->i_addr2,
            IEEE80211_FC0_SUBTYPE_AUTH,
            (seq + 1) | (estatus<<16));
    } else if (vap->iv_opmode == IEEE80211_M_STA) {
        /*
         * Kick the state machine.  This short-circuits
         * using the mgt frame timeout to trigger the
         * state transition.
         */
        if (vap->iv_state == IEEE80211_S_AUTH)
            ieee80211_new_state(vap, IEEE80211_S_SCAN, 0);
    }
}

/* Verify the existence and length of __elem or get out. */
#define IEEE80211_VERIFY_ELEMENT(__elem, __maxlen) do {         \
    if ((__elem) == NULL) {                     \
        IEEE80211_DISCARD(vap, IEEE80211_MSG_ELEMID,        \
            wh, ieee80211_mgt_subtype_name[subtype >>       \
            IEEE80211_FC0_SUBTYPE_SHIFT],           \
            "%s", "no " #__elem );              \
        vap->iv_stats.is_rx_elem_missing++;         \
        return;                         \
    }                               \
    if ((__elem)[1] > (__maxlen)) {                 \
        IEEE80211_DISCARD(vap, IEEE80211_MSG_ELEMID,        \
            wh, ieee80211_mgt_subtype_name[subtype >>       \
            IEEE80211_FC0_SUBTYPE_SHIFT],           \
            "bad " #__elem " len %d", (__elem)[1]);     \
        vap->iv_stats.is_rx_elem_toobig++;          \
        return;                         \
    }                               \
} while (0)

#define IEEE80211_VERIFY_LENGTH(_len, _minlen) do {         \
    if ((_len) < (_minlen)) {                   \
        IEEE80211_DISCARD(vap, IEEE80211_MSG_ELEMID,        \
            wh, ieee80211_mgt_subtype_name[subtype >>       \
            IEEE80211_FC0_SUBTYPE_SHIFT],           \
            "%s", "ie too short");              \
        vap->iv_stats.is_rx_elem_toosmall++;            \
        return;                         \
    }                               \
} while (0)

#ifdef IEEE80211_DEBUG
static void
ieee80211_ssid_mismatch(struct ieee80211vap *vap, const char *tag,
    u_int8_t mac[IEEE80211_ADDR_LEN], u_int8_t *ssid)
{
    printf("[%s] discard %s frame, ssid mismatch: ",
        ether_sprintf(mac), tag);
    ieee80211_print_essid(ssid + 2, ssid[1]);
    printf("\n");
}

#define IEEE80211_VERIFY_SSID(_ni, _ssid) do {              \
    if ((_ssid)[1] != 0 &&                      \
        ((_ssid)[1] != (_ni)->ni_esslen ||              \
        memcmp((_ssid) + 2, (_ni)->ni_essid, (_ssid)[1]) != 0)) {   \
        if (ieee80211_msg_input(vap))               \
            ieee80211_ssid_mismatch(vap,            \
                ieee80211_mgt_subtype_name[subtype >>   \
                IEEE80211_FC0_SUBTYPE_SHIFT],       \
                wh->i_addr2, _ssid);            \
        vap->iv_stats.is_rx_ssidmismatch++;         \
        return;                         \
    }                               \
} while (0)
#else /* !IEEE80211_DEBUG */
#define IEEE80211_VERIFY_SSID(_ni, _ssid) do {              \
    if ((_ssid)[1] != 0 &&                      \
        ((_ssid)[1] != (_ni)->ni_esslen ||              \
        memcmp((_ssid) + 2, (_ni)->ni_essid, (_ssid)[1]) != 0)) {   \
        vap->iv_stats.is_rx_ssidmismatch++;         \
        return;                         \
    }                               \
} while (0)
#endif /* !IEEE80211_DEBUG */

/* unalligned little endian access */
#define LE_READ_2(p)                    \
    ((u_int16_t)                    \
     ((((const u_int8_t *)(p))[0]      ) |      \
      (((const u_int8_t *)(p))[1] <<  8)))
#define LE_READ_4(p)                    \
    ((u_int32_t)                    \
     ((((const u_int8_t *)(p))[0]      ) |      \
      (((const u_int8_t *)(p))[1] <<  8) |      \
      (((const u_int8_t *)(p))[2] << 16) |      \
      (((const u_int8_t *)(p))[3] << 24)))

#define BE_READ_4(p)                    \
    ((u_int32_t)                    \
     ((((const u_int8_t *)(p))[0] << 24) |      \
      (((const u_int8_t *)(p))[1] << 16) |      \
      (((const u_int8_t *)(p))[2] <<  8) |      \
      (((const u_int8_t *)(p))[3]      )))

static int __inline
iswpaoui(const u_int8_t *frm)
{
    return frm[1] > 3 && LE_READ_4(frm+2) == ((WPA_OUI_TYPE<<24)|WPA_OUI);
}

static int __inline
iswmeoui(const u_int8_t *frm)
{
    return frm[1] > 3 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI);
}

static int __inline
iswmeparam(const u_int8_t *frm)
{
    return frm[1] > 5 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI) &&
        frm[6] == WME_PARAM_OUI_SUBTYPE;
}

static int __inline
iswmeinfo(const u_int8_t *frm)
{
    return frm[1] > 5 && LE_READ_4(frm+2) == ((WME_OUI_TYPE<<24)|WME_OUI) &&
        frm[6] == WME_INFO_OUI_SUBTYPE;
}

static int __inline
isatherosoui(const u_int8_t *frm)
{
    return frm[1] > 3 && LE_READ_4(frm+2) == ((ATH_OUI_TYPE<<24)|ATH_OUI);
}

static int __inline
ishtcap(const u_int8_t *frm)
{
    return frm[1] > 3 && BE_READ_4(frm+2) == ((ath_htoui<<8)|ath_htcapid);
}

static int __inline
ishtinfo(const u_int8_t *frm)
{
    return frm[1] > 3 && BE_READ_4(frm+2) == ((ath_htoui<<8)|ath_htinfoid);
}

/*
 * Convert a WPA cipher selector OUI to an internal
 * cipher algorithm.  Where appropriate we also
 * record any key length.
 */
static int
wpa_cipher(u_int8_t *sel, u_int8_t *keylen)
{
#define WPA_SEL(x)  (((x)<<24)|WPA_OUI)
    u_int32_t w = LE_READ_4(sel);

    switch (w) {
    case WPA_SEL(WPA_CSE_NULL):
        return IEEE80211_CIPHER_NONE;
    case WPA_SEL(WPA_CSE_WEP40):
        if (keylen)
            *keylen = 40 / NBBY;
        return IEEE80211_CIPHER_WEP;
    case WPA_SEL(WPA_CSE_WEP104):
        if (keylen)
            *keylen = 104 / NBBY;
        return IEEE80211_CIPHER_WEP;
    case WPA_SEL(WPA_CSE_TKIP):
        return IEEE80211_CIPHER_TKIP;
    case WPA_SEL(WPA_CSE_CCMP):
        return IEEE80211_CIPHER_AES_CCM;
    }
    return 32;      /* NB: so 1<< is discarded */
#undef WPA_SEL
}

/*
 * Convert a WPA key management/authentication algorithm
 * to an internal code.
 */
static int
wpa_keymgmt(u_int8_t *sel)
{
#define WPA_SEL(x)  (((x)<<24)|WPA_OUI)
    u_int32_t w = LE_READ_4(sel);

    switch (w) {
    case WPA_SEL(WPA_ASE_8021X_UNSPEC):
        return WPA_ASE_8021X_UNSPEC;
    case WPA_SEL(WPA_ASE_8021X_PSK):
        return WPA_ASE_8021X_PSK;
    case WPA_SEL(WPA_ASE_NONE):
        return WPA_ASE_NONE;
    }
    return 0;       /* NB: so is discarded */
#undef WPA_SEL
}

/*
 * Parse a WPA information element to collect parameters
 * and validate the parameters against what has been
 * configured for the system.
 */
static int
ieee80211_parse_wpa(struct ieee80211vap *vap, u_int8_t *frm,
    struct ieee80211_rsnparms *rsn, const struct ieee80211_frame *wh)
{
    u_int8_t len = frm[1];
    u_int32_t w;
    int n;

    /*
     * Check the length once for fixed parts: OUI, type,
     * version, mcast cipher, and 2 selector counts.
     * Other, variable-length data, must be checked separately.
     */
    KASSERT(vap->iv_flags & IEEE80211_F_WPA1,
        ("not WPA, flags 0x%x", vap->iv_flags));
    if (len < 14) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "WPA", "too short, len %u", len);
        return IEEE80211_REASON_IE_INVALID;
    }
    frm += 6, len -= 4;     /* NB: len is payload only */
    /* NB: iswapoui already validated the OUI and type */
    w = LE_READ_2(frm);
    if (w != WPA_VERSION) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "WPA", "bad version %u", w);
        return IEEE80211_REASON_IE_INVALID;
    }
    frm += 2, len -= 2;

    /* multicast/group cipher */
    w = wpa_cipher(frm, &rsn->rsn_mcastkeylen);
    if (w != rsn->rsn_mcastcipher) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "WPA", "mcast cipher mismatch; got %u, expected %u",
            w, rsn->rsn_mcastcipher);
        return IEEE80211_REASON_IE_INVALID;
    }
    frm += 4, len -= 4;

    /* unicast ciphers */
    n = LE_READ_2(frm);
    frm += 2, len -= 2;
    if (len < n*4+2) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "WPA", "ucast cipher data too short; len %u, n %u",
            len, n);
        return IEEE80211_REASON_IE_INVALID;
    }
    w = 0;
    for (; n > 0; n--) {
        w |= 1<<wpa_cipher(frm, &rsn->rsn_ucastkeylen);
        frm += 4, len -= 4;
    }
    w &= rsn->rsn_ucastcipherset;
    if (w == 0) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "WPA", "%s", "ucast cipher set empty");
        return IEEE80211_REASON_IE_INVALID;
    }
    if (w & (1<<IEEE80211_CIPHER_TKIP))
        rsn->rsn_ucastcipher = IEEE80211_CIPHER_TKIP;
    else
        rsn->rsn_ucastcipher = IEEE80211_CIPHER_AES_CCM;

    /* key management algorithms */
    n = LE_READ_2(frm);
    frm += 2, len -= 2;
    if (len < n*4) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "WPA", "key mgmt alg data too short; len %u, n %u",
            len, n);
        return IEEE80211_REASON_IE_INVALID;
    }
    w = 0;
    for (; n > 0; n--) {
        w |= wpa_keymgmt(frm);
        frm += 4, len -= 4;
    }
    w &= rsn->rsn_keymgmtset;
    if (w == 0) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "WPA", "%s", "no acceptable key mgmt alg");
        return IEEE80211_REASON_IE_INVALID;
    }
    if (w & WPA_ASE_8021X_UNSPEC)
        rsn->rsn_keymgmt = WPA_ASE_8021X_UNSPEC;
    else
        rsn->rsn_keymgmt = WPA_ASE_8021X_PSK;

    if (len > 2)        /* optional capabilities */
        rsn->rsn_caps = LE_READ_2(frm);

    return 0;
}

/*
 * Convert an RSN cipher selector OUI to an internal
 * cipher algorithm.  Where appropriate we also
 * record any key length.
 */
static int
rsn_cipher(u_int8_t *sel, u_int8_t *keylen)
{
#define RSN_SEL(x)  (((x)<<24)|RSN_OUI)
    u_int32_t w = LE_READ_4(sel);

    switch (w) {
    case RSN_SEL(RSN_CSE_NULL):
        return IEEE80211_CIPHER_NONE;
    case RSN_SEL(RSN_CSE_WEP40):
        if (keylen)
            *keylen = 40 / NBBY;
        return IEEE80211_CIPHER_WEP;
    case RSN_SEL(RSN_CSE_WEP104):
        if (keylen)
            *keylen = 104 / NBBY;
        return IEEE80211_CIPHER_WEP;
    case RSN_SEL(RSN_CSE_TKIP):
        return IEEE80211_CIPHER_TKIP;
    case RSN_SEL(RSN_CSE_CCMP):
        return IEEE80211_CIPHER_AES_CCM;
    case RSN_SEL(RSN_CSE_WRAP):
        return IEEE80211_CIPHER_AES_OCB;
    }
    return 32;      /* NB: so 1<< is discarded */
#undef WPA_SEL
}

/*
 * Convert an RSN key management/authentication algorithm
 * to an internal code.
 */
static int
rsn_keymgmt(u_int8_t *sel)
{
#define RSN_SEL(x)  (((x)<<24)|RSN_OUI)
    u_int32_t w = LE_READ_4(sel);

    switch (w) {
    case RSN_SEL(RSN_ASE_8021X_UNSPEC):
        return RSN_ASE_8021X_UNSPEC;
    case RSN_SEL(RSN_ASE_8021X_PSK):
        return RSN_ASE_8021X_PSK;
    case RSN_SEL(RSN_ASE_NONE):
        return RSN_ASE_NONE;
    }
    return 0;       /* NB: so is discarded */
#undef RSN_SEL
}

/*
 * Parse a WPA/RSN information element to collect parameters
 * and validate the parameters against what has been
 * configured for the system.
 */
static int
ieee80211_parse_rsn(struct ieee80211vap *vap, u_int8_t *frm,
    struct ieee80211_rsnparms *rsn, const struct ieee80211_frame *wh)
{
    u_int8_t len = frm[1];
    u_int32_t w;
    int n;

    /*
     * Check the length once for fixed parts:
     * version, mcast cipher, and 2 selector counts.
     * Other, variable-length data, must be checked separately.
     */
    KASSERT(vap->iv_flags & IEEE80211_F_WPA2,
        ("not RSN, flags 0x%x", vap->iv_flags));
    if (len < 10) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "RSN", "too short, len %u", len);
        return IEEE80211_REASON_IE_INVALID;
    }
    frm += 2;
    w = LE_READ_2(frm);
    if (w != RSN_VERSION) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "RSN", "bad version %u", w);
        return IEEE80211_REASON_IE_INVALID;
    }
    frm += 2, len -= 2;

    /* multicast/group cipher */
    w = rsn_cipher(frm, &rsn->rsn_mcastkeylen);
    if (w != rsn->rsn_mcastcipher) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "RSN", "mcast cipher mismatch; got %u, expected %u",
            w, rsn->rsn_mcastcipher);
        return IEEE80211_REASON_IE_INVALID;
    }
    frm += 4, len -= 4;

    /* unicast ciphers */
    n = LE_READ_2(frm);
    frm += 2, len -= 2;
    if (len < n*4+2) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "RSN", "ucast cipher data too short; len %u, n %u",
            len, n);
        return IEEE80211_REASON_IE_INVALID;
    }
    w = 0;
    for (; n > 0; n--) {
        w |= 1<<rsn_cipher(frm, &rsn->rsn_ucastkeylen);
        frm += 4, len -= 4;
    }
    w &= rsn->rsn_ucastcipherset;
    if (w == 0) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "RSN", "%s", "ucast cipher set empty");
        return IEEE80211_REASON_IE_INVALID;
    }
    if (w & (1<<IEEE80211_CIPHER_TKIP))
        rsn->rsn_ucastcipher = IEEE80211_CIPHER_TKIP;
    else
        rsn->rsn_ucastcipher = IEEE80211_CIPHER_AES_CCM;

    /* key management algorithms */
    n = LE_READ_2(frm);
    frm += 2, len -= 2;
    if (len < n*4) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "RSN", "key mgmt alg data too short; len %u, n %u",
            len, n);
        return IEEE80211_REASON_IE_INVALID;
    }
    w = 0;
    for (; n > 0; n--) {
        w |= rsn_keymgmt(frm);
        frm += 4, len -= 4;
    }
    w &= rsn->rsn_keymgmtset;
    if (w == 0) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WPA,
            wh, "RSN", "%s", "no acceptable key mgmt alg");
        return IEEE80211_REASON_IE_INVALID;
    }
    if (w & RSN_ASE_8021X_UNSPEC)
        rsn->rsn_keymgmt = RSN_ASE_8021X_UNSPEC;
    else
        rsn->rsn_keymgmt = RSN_ASE_8021X_PSK;

    /* optional RSN capabilities */
    if (len > 2)
        rsn->rsn_caps = LE_READ_2(frm);
    /* XXXPMKID */

    return 0;
}

void
ieee80211_saveie(u_int8_t **iep, const u_int8_t *ie)
{
    u_int ielen = ie[1]+2;
    /*
     * Record information element for later use.
     */
    if (*iep == NULL || (*iep)[1] != ie[1]) {
        if (*iep != NULL)
            FREE(*iep, M_DEVBUF);
        MALLOC(*iep, void*, ielen, M_DEVBUF, M_NOWAIT);
    }
    if (*iep != NULL)
        memcpy(*iep, ie, ielen);
}
EXPORT_SYMBOL(ieee80211_saveie);

static int
ieee80211_parse_wmeparams(struct ieee80211vap *vap, u_int8_t *frm,
    const struct ieee80211_frame *wh)
{
#define MS(_v, _f)  (((_v) & _f) >> _f##_S)
    struct ieee80211_wme_state *wme = &vap->iv_ic->ic_wme;
    u_int len = frm[1], qosinfo;
    int i;

    if (len < sizeof(struct ieee80211_wme_param)-2) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_WME,
            wh, "WME", "too short, len %u", len);
        return -1;
    }
    qosinfo = frm[__offsetof(struct ieee80211_wme_param, param_qosInfo)];
    qosinfo &= WME_QOSINFO_COUNT;
    /* XXX do proper check for wraparound */
    if (qosinfo == wme->wme_wmeChanParams.cap_info)
        return 0;
    frm += __offsetof(struct ieee80211_wme_param, params_acParams);
    for (i = 0; i < WME_NUM_AC; i++) {
        struct wmeParams *wmep =
            &wme->wme_wmeChanParams.cap_wmeParams[i];
        /* NB: ACI not used */
        wmep->wmep_acm = MS(frm[0], WME_PARAM_ACM);
        wmep->wmep_aifsn = MS(frm[0], WME_PARAM_AIFSN);
        wmep->wmep_logcwmin = MS(frm[1], WME_PARAM_LOGCWMIN);
        wmep->wmep_logcwmax = MS(frm[1], WME_PARAM_LOGCWMAX);
        wmep->wmep_txopLimit = LE_READ_2(frm+2);
        frm += 4;
    }
    wme->wme_wmeChanParams.cap_info = qosinfo;
    return 1;
#undef MS
}

static void
ieee80211_parse_athParams(struct ieee80211_node *ni, u_int8_t *ie)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = ni->ni_ic;
    struct ieee80211_ie_athAdvCap *athIe =
        (struct ieee80211_ie_athAdvCap *) ie;

    (void)vap;
    (void)ic;

    ni->ni_ath_flags = athIe->athAdvCap_capability;
    ni->ni_ath_defkeyindex = LE_READ_2(&athIe->athAdvCap_defKeyIndex);
#if 0
    /* NB: too noisy */
    IEEE80211_NOTE(vap, IEEE80211_MSG_SUPG, ni,
        "recv ath params: caps 0x%x flags 0x%x defkeyix %u",
        athIe->athAdvCap_capability, ni->ni_ath_flags,
        ni->ni_ath_defkeyindex);
#endif
#ifdef ATH_SUPERG_DYNTURBO
    if (IEEE80211_ATH_CAP(vap, ni, IEEE80211_ATHC_TURBOP)) {
        u_int32_t curflags, newflags;

        /*
         * Check for turbo mode switch.  Calculate flags
         * for the new mode and effect the switch.
         */
        newflags = curflags = ic->ic_bsschan->ic_flags;
        /* NB: ATHC_BOOST is not in ic_ath_cap, so get it from the ie */
        if (athIe->athAdvCap_capability & IEEE80211_ATHC_BOOST)
            newflags |= IEEE80211_CHAN_TURBO;
        else
            newflags &= ~IEEE80211_CHAN_TURBO;
        if (newflags != curflags)
            ieee80211_dturbo_switch(ic, newflags);
    }
#endif /* ATH_SUPERG_DYNTURBO */
}

void
ieee80211_saveath(struct ieee80211_node *ni, u_int8_t *ie)
{
    const struct ieee80211_ie_athAdvCap *athIe =
        (const struct ieee80211_ie_athAdvCap *) ie;

    ni->ni_ath_flags = athIe->athAdvCap_capability;
    ni->ni_ath_defkeyindex = LE_READ_2(&athIe->athAdvCap_defKeyIndex);
    ieee80211_saveie(&ni->ni_ath_ie, ie);
}

struct ieee80211_channel *
ieee80211_doth_findchan(struct ieee80211vap *vap, u_int8_t chan)
{
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_channel *c;
    int flags, freq;

    /* NB: try first to preserve turbo */
    flags = ic->ic_bsschan->ic_flags & IEEE80211_CHAN_ALL;
    freq = ieee80211_ieee2mhz(chan, 0);
    c = ieee80211_find_channel(ic, freq, flags);
    if (c == NULL)
        c = ieee80211_find_channel(ic, freq, 0);
    return c;
}

static int
ieee80211_parse_dothparams(struct ieee80211vap *vap, u_int8_t *frm,
    const struct ieee80211_frame *wh)
{
    struct ieee80211com *ic = vap->iv_ic;
    u_int len = frm[1];
    u_int8_t chan, tbtt;

    if (len < 4-2) {        /* XXX ie struct definition */
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_DOTH,
            wh, "channel switch", "too short, len %u", len);
        return -1;
    }
    chan = frm[3];
    if (isclr(ic->ic_chan_avail, chan)) {
        IEEE80211_DISCARD_IE(vap,
            IEEE80211_MSG_ELEMID | IEEE80211_MSG_DOTH,
            wh, "channel switch", "invalid channel %u", chan);
        return -1;
    }
    tbtt = frm[4];
    IEEE80211_DPRINTF(vap, IEEE80211_MSG_DOTH,
        "%s: channel switch to %d in %d tbtt\n", __func__, chan, tbtt);
    if (tbtt <= 1) {
        struct ieee80211_channel *c;

        IEEE80211_DPRINTF(vap, IEEE80211_MSG_DOTH,
            "%s: Channel switch to %d NOW!\n", __func__, chan);
#if 0
        /* XXX does not belong here? */
        /* XXX doesn't stop management frames */
        /* XXX who restarts the queue? */
        /* NB: for now, error here is non-catastrophic.
         *     in the future we may need to insure we
         *     stop xmit on this channel.
         */
        netif_stop_queue(ic->ic_dev);
#endif
        if ( (c = ieee80211_doth_findchan(vap, chan)) == NULL) {
            /* XXX something wrong */
            IEEE80211_DISCARD_IE(vap,
                                 IEEE80211_MSG_ELEMID | IEEE80211_MSG_DOTH,
                                 wh, "channel switch",
                                 "channel %u lookup failed", chan);
            return 0;
        }
        ic->ic_prevchan = ic->ic_curchan;
        ic->ic_curchan = ic->ic_bsschan = c;
        ic->ic_set_channel(ic);
        return 1;
    }
    return 0;
}

void
ieee80211_parse_htcap(struct ieee80211_node *ni, u_int8_t *ie)
{
    struct ieee80211_ie_htcap_cmn *htcap = (struct ieee80211_ie_htcap_cmn *)ie;
    struct ieee80211com 	  *ic = ni->ni_ic;

    ni->ni_htcap    = le16toh(htcap->hc_cap);

    if (ni->ni_htcap & IEEE80211_HTCAP_C_SHORTGI40) {
        ni->ni_htcap  = ni->ni_htcap & ((ic->ic_htflags & IEEE80211_HTF_SHORTGI) 
                                        ? ni->ni_htcap  : ~IEEE80211_HTCAP_C_SHORTGI40);
    }
    if (!(ni->ni_htcap & IEEE80211_HTCAP_C_CHWIDTH40)) {
        ni->ni_chwidth = IEEE80211_CWM_WIDTH20;
    }
    ni->ni_maxampdu = 1 << (IEEE80211_HTCAP_MAXRXAMPDU_FACTOR + htcap->hc_maxampdu);
    ni->ni_mpdudensity = htcap->hc_mpdudensity;

    ni->ni_flags |= IEEE80211_NODE_HT;
}

void
ieee80211_parse_htinfo(struct ieee80211_node *ni, u_int8_t *ie)
{
    struct ieee80211_ie_htinfo_cmn  *htinfo = (struct ieee80211_ie_htinfo_cmn *)ie;
    enum ieee80211_cwm_width    chwidth;

    chwidth = (htinfo->hi_txchwidth == IEEE80211_HTINFO_TXWIDTH_2040) ?
                IEEE80211_CWM_WIDTH40 : IEEE80211_CWM_WIDTH20;

    /* update node's recommended tx channel width */
    ni->ni_chwidth = chwidth;
}

/* XXX. Not the right place for such a definition */
struct l2_update_frame {
    struct ether_header eh;
    u8 dsap;
    u8 ssap;
    u8 control;
    u8 xid[3];
}  __packed;

static void
ieee80211_deliver_l2uf(struct ieee80211_node *ni)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct net_device *dev = vap->iv_dev;
    struct sk_buff *skb;
    struct l2_update_frame *l2uf;
    struct ether_header *eh;

    skb = dev_alloc_skb(sizeof(*l2uf));
    if (!skb) {
        printk("ieee80211_deliver_l2uf: no buf available\n");
        return;
    }
    skb_put(skb, sizeof(*l2uf));
    l2uf = (struct l2_update_frame *)(skb->data);
    eh = &l2uf->eh;
    /* dst: Broadcast address */
    IEEE80211_ADDR_COPY(eh->ether_dhost, dev->broadcast);
    /* src: associated STA */
    IEEE80211_ADDR_COPY(eh->ether_shost, ni->ni_macaddr);
    eh->ether_type = htons(skb->len - sizeof(*eh));

    l2uf->dsap = 0;
    l2uf->ssap = 0;
    l2uf->control = 0xf5;
    l2uf->xid[0] = 0x81;
    l2uf->xid[1] = 0x80;
    l2uf->xid[2] = 0x00;

    skb->dev = dev;

    /* ieee80211_deliver_data() called below pulls the eth header any way
     * so, do not pull the header again */
    //skb->protocol = eth_type_trans(skb, dev);

    skb->mac.raw = skb->data;
    ieee80211_deliver_data(ni, skb);
    return;
}

void
ieee80211_recv_mgmt(struct ieee80211_node *ni, struct sk_buff *skb,
    int subtype, int rssi, u_int32_t rstamp)
{
#define ISPROBE(_st)    ((_st) == IEEE80211_FC0_SUBTYPE_PROBE_RESP)
#define ISREASSOC(_st)  ((_st) == IEEE80211_FC0_SUBTYPE_REASSOC_RESP)
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211com *ic = vap->iv_ic;
    struct ieee80211_frame *wh;
    u_int8_t *frm, *efrm;
    u_int8_t *ssid, *rates, *xrates, *wpa, *wme, *ath, *htcap;
    u_int8_t rate;
    int reassoc, resp, allocbs;

    wh = (struct ieee80211_frame *) skb->data;
    frm = (u_int8_t *)&wh[1];
    efrm = skb->data + skb->len;
    switch (subtype) {
    case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
    case IEEE80211_FC0_SUBTYPE_BEACON: {
        struct ieee80211_scanparams scan;
	int has_erp = 0;

        /*
         * We process beacon/probe response frames:
         *    o when scanning, or
         *    o station mode when associated (to collect state
         *      updates such as 802.11g slot time), or
         *    o adhoc mode (to discover neighbors), or
	 *    o ap mode in protection mode (beacons only)
         * Frames otherwise received are discarded.
         */
        if (!((ic->ic_flags & IEEE80211_F_SCAN) ||
	      ((subtype == IEEE80211_FC0_SUBTYPE_BEACON) &&
	       (vap->iv_opmode == IEEE80211_M_HOSTAP) &&
	       (ic->ic_protmode != IEEE80211_PROT_NONE)) ||
              (vap->iv_opmode == IEEE80211_M_STA && ni->ni_associd) ||
               vap->iv_opmode == IEEE80211_M_IBSS)) {
            vap->iv_stats.is_rx_mgtdiscard++;
            return;
        }
        /*
         * beacon/probe response frame format
         *  [8] time stamp
         *  [2] beacon interval
         *  [2] capability information
         *  [tlv] ssid
         *  [tlv] supported rates
         *  [tlv] country information
         *  [tlv] parameter set (FH/DS)
         *  [tlv] erp information
         *  [tlv] extended supported rates
         *  [tlv] WME
         *  [tlv] WPA or RSN
         *  [tlv] HT Capabilities
         *  [tlv] HT Information
                 *      [tlv] Atheros Advanced Capabilities
         */
        IEEE80211_VERIFY_LENGTH(efrm - frm, 12);
        memset(&scan, 0, sizeof(scan));
        scan.tstamp  = frm;             frm += 8;
        scan.bintval = le16toh(*(u_int16_t *)frm);  frm += 2;
        scan.capinfo = le16toh(*(u_int16_t *)frm);  frm += 2;
        scan.bchan = ieee80211_chan2ieee(ic, ic->ic_curchan);
        scan.chan = scan.bchan;

        while (frm < efrm) {
            switch (*frm) {
            case IEEE80211_ELEMID_SSID:
                scan.ssid = frm;
                break;
            case IEEE80211_ELEMID_RATES:
                scan.rates = frm;
                break;
            case IEEE80211_ELEMID_COUNTRY:
                scan.country = frm;
                break;
            case IEEE80211_ELEMID_FHPARMS:
                if (ic->ic_phytype == IEEE80211_T_FH) {
                    scan.fhdwell = LE_READ_2(&frm[2]);
                    scan.chan = IEEE80211_FH_CHAN(frm[4], frm[5]);
                    scan.fhindex = frm[6];
                }
                break;
            case IEEE80211_ELEMID_DSPARMS:
                /*
                 * XXX hack this since depending on phytype
                 * is problematic for multi-mode devices.
                 */
                if (ic->ic_phytype != IEEE80211_T_FH) {
                    scan.chan = frm[2];
                }
                break;
            case IEEE80211_ELEMID_TIM:
                /* XXX ATIM? */
                scan.tim = frm;
                scan.timoff = frm - skb->data;
                break;
            case IEEE80211_ELEMID_IBSSPARMS:
                break;
            case IEEE80211_ELEMID_XRATES:
                scan.xrates = frm;
                break;
            case IEEE80211_ELEMID_ERP:
                if (frm[1] != 1) {
                    IEEE80211_DISCARD_IE(vap,
                        IEEE80211_MSG_ELEMID, wh, "ERP",
                        "bad len %u", frm[1]);
                    vap->iv_stats.is_rx_elem_toobig++;
                    break;
                }
		has_erp = 1;
                scan.erp = frm[2];
                break;
            case IEEE80211_ELEMID_RSN:
                scan.wpa = frm;
                break;
            case IEEE80211_ELEMID_VENDOR:
                if (iswpaoui(frm))
                    scan.wpa = frm;
                else if (iswmeparam(frm) || iswmeinfo(frm))
                    scan.wme = frm;
                else if (isatherosoui(frm))
                    scan.ath = frm;
                else if (ath_htvendorie_enable) {
                    if (ishtcap(frm))
                        scan.htcap =
                            (u_int8_t *)&((struct vendor_ie_htcap *)frm)->hc_ie;
                    else if (ishtinfo(frm))
                        scan.htinfo = 
                            (u_int8_t *)&((struct vendor_ie_htinfo *)frm)->hi_ie;
                }
                break;
            case IEEE80211_ELEMID_CHANSWITCHANN:
                if (ic->ic_flags & IEEE80211_F_DOTH) {
                    scan.doth = frm;
                }
                break;
            case IEEE80211_ELEMID_HTCAP:
                if(!ath_htvendorie_enable)
                    scan.htcap =
                        (u_int8_t *)&((struct ieee80211_ie_htcap *)frm)->hc_ie;
                break;
            case IEEE80211_ELEMID_HTINFO:
                if(!ath_htvendorie_enable)
                    scan.htinfo = 
                        (u_int8_t *)&((struct ieee80211_ie_htinfo *)frm)->hi_ie;
                break;
            case IEEE80211_ELEMID_HTCAP_ANA:
                if(ath_htdupie_enable && (scan.htcap == NULL))
                    scan.htcap =
                        (u_int8_t *)&((struct ieee80211_ie_htcap *)frm)->hc_ie;
                break;
            case IEEE80211_ELEMID_HTINFO_ANA:
                if(ath_htdupie_enable && (scan.htinfo == NULL))
                    scan.htinfo = 
                        (u_int8_t *)&((struct ieee80211_ie_htinfo *)frm)->hi_ie;
                break;
            default:
                IEEE80211_DISCARD_IE(vap, IEEE80211_MSG_ELEMID,
                    wh, "unhandled",
                    "id %u, len %u", *frm, frm[1]);
                vap->iv_stats.is_rx_elem_unknown++;
                break;
            }
            frm += frm[1] + 2;
        }
        IEEE80211_VERIFY_ELEMENT(scan.rates, IEEE80211_RATE_MAXSIZE);
        IEEE80211_VERIFY_ELEMENT(scan.ssid, IEEE80211_NWID_LEN);

#if IEEE80211_CHAN_MAX < 255
        if (scan.chan > IEEE80211_CHAN_MAX) {
            IEEE80211_DISCARD(vap, IEEE80211_MSG_ELEMID,
                wh, ieee80211_mgt_subtype_name[subtype >>
                IEEE80211_FC0_SUBTYPE_SHIFT],
                "invalid channel %u", scan.chan);
            vap->iv_stats.is_rx_badchan++;
            return;
        }
#endif
        if (scan.chan != scan.bchan &&
            ic->ic_phytype != IEEE80211_T_FH) {
            /*
             * Frame was received on a channel different from the
             * one indicated in the DS params element id;
             * silently discard it.
             *
             * NB: this can happen due to signal leakage.
             *     But we should take it for FH phy because
             *     the rssi value should be correct even for
             *     different hop pattern in FH.
             */
            IEEE80211_DISCARD(vap, IEEE80211_MSG_ELEMID,
                wh, ieee80211_mgt_subtype_name[subtype >>
                IEEE80211_FC0_SUBTYPE_SHIFT],
                "for off-channel %u", scan.chan);
            vap->iv_stats.is_rx_chanmismatch++;
            return;
        }

        /*
         * Count frame now that we know it's to be processed.
         */
        if (subtype == IEEE80211_FC0_SUBTYPE_BEACON)
            IEEE80211_NODE_STAT(ni, rx_beacons);
        else
            IEEE80211_NODE_STAT(ni, rx_proberesp);

        /*
         * When operating in station mode, check for state updates.
         * Be careful to ignore beacons received while doing a
         * background scan.  We consider only 11g/WMM stuff right now.
         */
        if (vap->iv_opmode == IEEE80211_M_STA &&
            ni->ni_associd != 0 &&
            IEEE80211_ADDR_EQ(wh->i_addr2, ni->ni_bssid)) {
            /* record tsf of last beacon */
            memcpy(ni->ni_tstamp.data, scan.tstamp,
                sizeof(ni->ni_tstamp));
            if (ni->ni_erp != scan.erp) {
                IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                    "erp change: was 0x%x, now 0x%x",
                    ni->ni_erp, scan.erp);
                if (scan.erp & IEEE80211_ERP_USE_PROTECTION)
                    ic->ic_flags |= IEEE80211_F_USEPROT;
                else
                    ic->ic_flags &= ~IEEE80211_F_USEPROT;
                ni->ni_erp = scan.erp;
                /* XXX statistic */
            }
            if ((ni->ni_capinfo ^ scan.capinfo) & IEEE80211_CAPINFO_SHORT_SLOTTIME) {
                IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
                    "capabilities change: was 0x%x, now 0x%x",
                    ni->ni_capinfo, scan.capinfo);
                /*
                 * NB: we assume short preamble doesn't
                 *     change dynamically
                 */
                ieee80211_set_shortslottime(ic,
                    IEEE80211_IS_CHAN_A(ic->ic_bsschan) || IEEE80211_IS_CHAN_11NA(ic->ic_bsschan) ||
                    (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME));
                ni->ni_capinfo = scan.capinfo;
                /* XXX statistic */
            }
            if (scan.wme != NULL &&
                (ni->ni_flags & IEEE80211_NODE_QOS) &&
                ieee80211_parse_wmeparams(vap, scan.wme, wh) > 0)
                ieee80211_wme_updateparams(vap);
            if (scan.ath != NULL)
                ieee80211_parse_athParams(ni, scan.ath);
            if (scan.doth != NULL)
                ieee80211_parse_dothparams(vap, scan.doth, wh);
            /* 11n */
            if (scan.htcap) {
                ieee80211_parse_htcap(ni, scan.htcap);
            }
            if (scan.htinfo) {
                enum ieee80211_cwm_width ochwidth = ni->ni_chwidth;
                ieee80211_parse_htinfo(ni, scan.htinfo);

                /* check for AP changing channel width */
                if (ochwidth != ni->ni_chwidth) {
                    ni->ni_newchwidth = 1;
                    ic->ic_cwm.cw_width = ni->ni_chwidth;
                }
            }
            if (scan.tim != NULL) {
                /*
                 * Check the TIM. For now we drop out of
                 * power save mode for any reason.
                 */
                struct ieee80211_tim_ie *tim =
                    (struct ieee80211_tim_ie *) scan.tim;
                int aid = IEEE80211_AID(ni->ni_associd);
                int ix = aid / NBBY;
                int min = tim->tim_bitctl &~ 1;
                int max = tim->tim_len + min - 4;
                if ((tim->tim_bitctl&1) ||
                    (min <= ix && ix <= max &&
                     isset(tim->tim_bitmap - min, aid)))
                    ieee80211_sta_pwrsave(vap, 0);

                vap->iv_dtim_count = tim->tim_count;
            }
            /*
             * If scanning, pass the info to the scan module.
             * Otherwise, check if it's the right time to do
             * a background scan.  Background scanning must
             * be enabled and we must not be operating in the
             * turbo phase of dynamic turbo mode.  Then,
             * it's been a while since the last background
             * scan and if no data frames have come through
             * recently, kick off a scan.  Note that this
             * is the mechanism by which a background scan
             * is started _and_ continued each time we
             * return on-channel to receive a beacon from
             * our ap.
             */
            if (ic->ic_flags & IEEE80211_F_SCAN)
                ieee80211_add_scan(vap, &scan, wh,
                    subtype, rssi, rstamp);
            else if ((vap->iv_flags & IEEE80211_F_BGSCAN) &&
                !IEEE80211_IS_CHAN_DTURBO(ic->ic_curchan) &&
                time_after(jiffies,
                   ic->ic_lastscan + vap->iv_bgscanintvl) &&
                time_after(jiffies,
                   ic->ic_lastdata + vap->iv_bgscanidle))
                ieee80211_bg_scan(vap);
            return;
        }

        /* Update AP protection mode when in 11G mode */
        if ((vap->iv_opmode == IEEE80211_M_HOSTAP) &&
        	(IEEE80211_IS_CHAN_ANYG(ic->ic_curchan) || IEEE80211_IS_CHAN_11NG(ic->ic_curchan))) {
                /* Assume no ERP IE == 11b AP */
                if (!has_erp ||
                    (has_erp && (scan.erp & IEEE80211_ERP_NON_ERP_PRESENT))) {
                	if (!(ic->ic_flags & IEEE80211_F_USEPROT)) {
                        	IEEE80211_DPRINTF(vap,
                                	IEEE80211_MSG_INPUT,
                                        "setting protection bit "
                                        "(beacon from %s)\n",
                                        ether_sprintf(wh->i_addr2));
                                ic->ic_flags |= IEEE80211_F_USEPROT;
                                ic->ic_flags_ext |= IEEE80211_FEXT_ERPUPDATE;
                	}
        	}
        }

        /*
         * If scanning, just pass information to the scan module.
         */
        if (ic->ic_flags & IEEE80211_F_SCAN) {
            ieee80211_add_scan(vap, &scan, wh,
                subtype, rssi, rstamp);
            return;
        }
        if (scan.capinfo & IEEE80211_CAPINFO_IBSS) {
            if (!IEEE80211_ADDR_EQ(wh->i_addr2, ni->ni_macaddr)) {
                /*
                 * Create a new entry in the neighbor table.
                 */
                ni = ieee80211_add_neighbor(vap, wh, &scan);
            } else {
                /*
                 * Record tsf for potential resync.
                 */
                memcpy(ni->ni_tstamp.data, scan.tstamp,
                    sizeof(ni->ni_tstamp));
            }
            if (ni != NULL) {
                ni->ni_rssi = rssi;
                ni->ni_rstamp = rstamp;
            }
        }
        break;
    }

    case IEEE80211_FC0_SUBTYPE_PROBE_REQ:
        if (vap->iv_opmode == IEEE80211_M_STA ||
            vap->iv_state != IEEE80211_S_RUN) {
            vap->iv_stats.is_rx_mgtdiscard++;
            return;
        }
        if (IEEE80211_IS_MULTICAST(wh->i_addr2)) {
            /* frame must be directed */
            vap->iv_stats.is_rx_mgtdiscard++;   /* XXX stat */
            return;
        }

/*
 * XR vap does not process  probe requests.
 */
#ifdef ATH_SUPERG_XR
    if(vap->iv_flags & IEEE80211_F_XR )
        return;
#endif
        /*
         * prreq frame format
         *  [tlv] ssid
         *  [tlv] supported rates
         *  [tlv] extended supported rates
                 *      [tlv] Atheros Advanced Capabilities
         */
        ssid = rates = xrates = ath = NULL;
        while (frm < efrm) {
            switch (*frm) {
            case IEEE80211_ELEMID_SSID:
                ssid = frm;
                break;
            case IEEE80211_ELEMID_RATES:
                rates = frm;
                break;
            case IEEE80211_ELEMID_XRATES:
                xrates = frm;
                break;
            case IEEE80211_ELEMID_VENDOR:
                if (isatherosoui(frm))
                ath = frm;
                /* XXX Atheros OUI support */
                break;
            }
            frm += frm[1] + 2;
        }
        IEEE80211_VERIFY_ELEMENT(rates, IEEE80211_RATE_MAXSIZE);
        IEEE80211_VERIFY_ELEMENT(ssid, IEEE80211_NWID_LEN);
        IEEE80211_VERIFY_SSID(vap->iv_bss, ssid);
        if ((vap->iv_flags & IEEE80211_F_HIDESSID) && ssid[1] == 0) {
            IEEE80211_DISCARD(vap, IEEE80211_MSG_INPUT,
                wh, ieee80211_mgt_subtype_name[subtype >>
                IEEE80211_FC0_SUBTYPE_SHIFT],
                "%s", "no ssid with ssid suppression enabled");
            vap->iv_stats.is_rx_ssidmismatch++; /*XXX*/
            return;
        }
        if (ni == vap->iv_bss) {
            if (vap->iv_opmode == IEEE80211_M_IBSS) {
                /*
                 * XXX Cannot tell if the sender is operating
                 * in ibss mode.  But we need a new node to
                 * send the response so blindly add them to the
                 * neighbor table.
                 */
                ni = ieee80211_fakeup_adhoc_node(vap,
                    wh->i_addr2);
            } else
                ni = ieee80211_tmp_node(vap, wh->i_addr2);
            if (ni == NULL)
                return;
            allocbs = 1;
        } else
            allocbs = 0;
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_INPUT, wh->i_addr2,
            "%s", "recv probe req");
        ni->ni_rssi = rssi;
        ni->ni_rstamp = rstamp;
        ieee80211_setup_rates(ni, rates, xrates,
                  IEEE80211_F_DOSORT | IEEE80211_F_DOXSECT |
                  IEEE80211_F_DOBRS  | IEEE80211_F_DOFRATE);
        IEEE80211_SEND_MGMT(ni,
            IEEE80211_FC0_SUBTYPE_PROBE_RESP, 0);
        if (allocbs && vap->iv_opmode != IEEE80211_M_IBSS) {
            /*
             * Temporary node created just to send a
             * response, reclaim immediately
             */
            ieee80211_free_node(ni);
        } else if (ath != NULL)
            ieee80211_saveath(ni, ath);
        break;

    case IEEE80211_FC0_SUBTYPE_AUTH: {
        u_int16_t algo, seq, status;
        /*
         * auth frame format
         *  [2] algorithm
         *  [2] sequence
         *  [2] status
         *  [tlv*] challenge
         */
        IEEE80211_VERIFY_LENGTH(efrm - frm, 6);
        algo   = le16toh(*(u_int16_t *)frm);
        seq    = le16toh(*(u_int16_t *)(frm + 2));
        status = le16toh(*(u_int16_t *)(frm + 4));
#ifdef ATH_SUPERG_XR
        if (!IEEE80211_ADDR_EQ(wh->i_addr3, vap->iv_bss->ni_bssid)) {
            /*
             * node roaming between XR and normal vaps.
             * this can only happen in AP mode. disaccociate from
             * previous vap first.
             */
            if(vap->iv_xrvap) {
                if(ni == vap->iv_bss) {
                    ni = vap->iv_xrvap->iv_bss;
                } else {
                    ieee80211_node_leave(ni);
                    ieee80211_node_reset(ni,vap->iv_xrvap);
                }
                vap = vap->iv_xrvap;
            } else {
                IEEE80211_DISCARD(vap, IEEE80211_MSG_AUTH,
                                  wh, "auth", "%s", "not to pier xr bssid");
                return;
            }
        }
#endif
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_AUTH, wh->i_addr2,
            "recv auth frame with algorithm %d seq %d", algo, seq);
        /*
         * Consult the ACL policy module if setup.
         */
        if (ic->ic_acl != NULL &&
            !ic->ic_acl->iac_check(vap, wh->i_addr2)) {
            IEEE80211_DISCARD(vap, IEEE80211_MSG_ACL,
                wh, "auth", "%s", "disallowed by ACL");
            vap->iv_stats.is_rx_acl++;
            return;
        }
        if (vap->iv_flags & IEEE80211_F_COUNTERM) {
            IEEE80211_DISCARD(vap,
                IEEE80211_MSG_AUTH | IEEE80211_MSG_CRYPTO,
                wh, "auth", "%s", "TKIP countermeasures enabled");
            vap->iv_stats.is_rx_auth_countermeasures++;
            if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
                ieee80211_send_error(ni, wh->i_addr2,
                    IEEE80211_FC0_SUBTYPE_AUTH,
                    IEEE80211_REASON_MIC_FAILURE);
            }
            return;
        }
        if (algo == IEEE80211_AUTH_ALG_SHARED)
            ieee80211_auth_shared(ni, wh, frm + 6, efrm, rssi,
                rstamp, seq, status);
        else if (algo == IEEE80211_AUTH_ALG_OPEN)
            ieee80211_auth_open(ni, wh, rssi, rstamp, seq, status);
        else {
            IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
                wh, "auth", "unsupported alg %d", algo);
            vap->iv_stats.is_rx_auth_unsupported++;
            if (vap->iv_opmode == IEEE80211_M_HOSTAP) {
                /* XXX not right */
                ieee80211_send_error(ni, wh->i_addr2,
                    IEEE80211_FC0_SUBTYPE_AUTH,
                    (seq+1) | (IEEE80211_STATUS_ALG<<16));
            }
            return;
        }
        break;
    }

    case IEEE80211_FC0_SUBTYPE_ASSOC_REQ:
    case IEEE80211_FC0_SUBTYPE_REASSOC_REQ: {
        u_int16_t capinfo, bintval;
        struct ieee80211_rsnparms rsn;
        u_int8_t reason;

        if (vap->iv_opmode != IEEE80211_M_HOSTAP ||
            vap->iv_state != IEEE80211_S_RUN) {
            vap->iv_stats.is_rx_mgtdiscard++;
            return;
        }

        if (subtype == IEEE80211_FC0_SUBTYPE_REASSOC_REQ) {
            reassoc = 1;
            resp = IEEE80211_FC0_SUBTYPE_REASSOC_RESP;
        } else {
            reassoc = 0;
            resp = IEEE80211_FC0_SUBTYPE_ASSOC_RESP;
        }

        /*
         * asreq frame format
         *  [2] capability information
         *  [2] listen interval
         *  [6*] current AP address (reassoc only)
         *  [tlv] ssid
         *  [tlv] supported rates
         *  [tlv] extended supported rates
         *  [tlv] wpa or RSN
         *      [tlv] WME
         *  [tlv] HT Capabilities
         *  [tlv] Atheros Advanced Capabilities
         */
        IEEE80211_VERIFY_LENGTH(efrm - frm, (reassoc ? 10 : 4));
        if (!IEEE80211_ADDR_EQ(wh->i_addr3, vap->iv_bss->ni_bssid)) {
            IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
                wh, ieee80211_mgt_subtype_name[subtype >>
                IEEE80211_FC0_SUBTYPE_SHIFT],
                "%s", "wrong bssid");
            vap->iv_stats.is_rx_assoc_bss++;
            return;
        }
        capinfo = le16toh(*(u_int16_t *)frm);   frm += 2;
        bintval = le16toh(*(u_int16_t *)frm);   frm += 2;
        if (reassoc)
            frm += 6;   /* ignore current AP info */
        ssid = rates = xrates = wpa = wme = ath = htcap = NULL;
        while (frm < efrm) {
            switch (*frm) {
            case IEEE80211_ELEMID_SSID:
                ssid = frm;
                break;
            case IEEE80211_ELEMID_RATES:
                rates = frm;
                break;
            case IEEE80211_ELEMID_XRATES:
                xrates = frm;
                break;
            /* XXX verify only one of RSN and WPA ie's? */
            case IEEE80211_ELEMID_RSN:
                wpa = frm;
                break;
            case IEEE80211_ELEMID_HTCAP:
                if(!ath_htvendorie_enable)
                    htcap =
                        (u_int8_t *)&((struct ieee80211_ie_htcap *)frm)->hc_ie;
                break;
            case IEEE80211_ELEMID_HTCAP_ANA:
                if(ath_htdupie_enable && (htcap == NULL))
                    htcap =
                        (u_int8_t *)&((struct ieee80211_ie_htcap *)frm)->hc_ie;
                break;
            case IEEE80211_ELEMID_VENDOR:
                if (iswpaoui(frm)) {
                    if (vap->iv_flags & IEEE80211_F_WPA1)
                        wpa = frm;
                } else if (iswmeinfo(frm))
                    wme = frm;
                else if (isatherosoui(frm))
                    ath = frm;
                else if(ath_htvendorie_enable && ishtcap(frm))
                    htcap = (u_int8_t *)&((struct vendor_ie_htcap *)frm)->hc_ie;
                break;
            }
            frm += frm[1] + 2;
        }
        IEEE80211_VERIFY_ELEMENT(rates, IEEE80211_RATE_MAXSIZE);
        IEEE80211_VERIFY_ELEMENT(ssid, IEEE80211_NWID_LEN);
        IEEE80211_VERIFY_SSID(vap->iv_bss, ssid);

        if (ni == vap->iv_bss) {
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ANY, wh->i_addr2,
                "deny %s request, sta not authenticated",
                reassoc ? "reassoc" : "assoc");
            ieee80211_send_error(ni, wh->i_addr2,
                IEEE80211_FC0_SUBTYPE_DEAUTH,
                IEEE80211_REASON_ASSOC_NOT_AUTHED);
            vap->iv_stats.is_rx_assoc_notauth++;
            return;
        }

        /* Assert right associstion security credentials */
        /* XXX Divy. Incomplete */
        if (wpa == NULL && (ic->ic_flags & IEEE80211_F_WPA)) {
            IEEE80211_DPRINTF(vap,
                IEEE80211_MSG_ASSOC | IEEE80211_MSG_WPA,
                "[%s] no WPA/RSN IE in association request\n",
                ether_sprintf(wh->i_addr2));
            IEEE80211_SEND_MGMT(ni,
                        IEEE80211_FC0_SUBTYPE_DEAUTH,
                        IEEE80211_REASON_RSN_REQUIRED);
            ieee80211_node_leave(ni);
            /* XXX distinguish WPA/RSN? */
            vap->iv_stats.is_rx_assoc_badwpaie++;
            return;
        }

        if (wpa != NULL) {
            /*
             * Parse WPA information element.  Note that
             * we initialize the param block from the node
             * state so that information in the IE overrides
             * our defaults.  The resulting parameters are
             * installed below after the association is assured.
             */
            rsn = ni->ni_rsn;
            if (wpa[0] != IEEE80211_ELEMID_RSN)
                reason = ieee80211_parse_wpa(vap, wpa, &rsn, wh);
            else
                reason = ieee80211_parse_rsn(vap, wpa, &rsn, wh);
            if (reason != 0) {
                IEEE80211_SEND_MGMT(ni,
                    IEEE80211_FC0_SUBTYPE_DEAUTH, reason);
                ieee80211_node_leave(ni);
                /* XXX distinguish WPA/RSN? */
                vap->iv_stats.is_rx_assoc_badwpaie++;
                return;
            }
            IEEE80211_NOTE_MAC(vap,
                IEEE80211_MSG_ASSOC | IEEE80211_MSG_WPA,
                wh->i_addr2,
                "%s ie: mc %u/%u uc %u/%u key %u caps 0x%x",
                wpa[0] != IEEE80211_ELEMID_RSN ?  "WPA" : "RSN",
                rsn.rsn_mcastcipher, rsn.rsn_mcastkeylen,
                rsn.rsn_ucastcipher, rsn.rsn_ucastkeylen,
                rsn.rsn_keymgmt, rsn.rsn_caps);
        }
        /* discard challenge after association */
        if (ni->ni_challenge != NULL) {
            FREE(ni->ni_challenge, M_DEVBUF);
            ni->ni_challenge = NULL;
        }
        /* 802.11 spec says to ignore station's privacy bit */
        if ((capinfo & IEEE80211_CAPINFO_ESS) == 0) {
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ANY, wh->i_addr2,
                "deny %s request, capability mismatch 0x%x",
                reassoc ? "reassoc" : "assoc", capinfo);
            IEEE80211_SEND_MGMT(ni, resp, IEEE80211_STATUS_CAPINFO);
            ieee80211_node_leave(ni);
            vap->iv_stats.is_rx_assoc_capmismatch++;
            return;
        }
        if (!ieee80211_setup_rates(ni, rates, xrates,
                  IEEE80211_F_DOSORT | IEEE80211_F_DOXSECT |
                  IEEE80211_F_DOBRS  | IEEE80211_F_DOFRATE))
        {
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ANY, wh->i_addr2,
                "deny %s request, rate set mismatch",
                reassoc ? "reassoc" : "assoc");
            IEEE80211_SEND_MGMT(ni, resp,
                IEEE80211_STATUS_BASIC_RATE);
            ieee80211_node_leave(ni);
            vap->iv_stats.is_rx_assoc_norate++;
            return;
        }
        ni->ni_rssi = rssi;
        ni->ni_rstamp = rstamp;
        ni->ni_intval = bintval;
        ni->ni_capinfo = capinfo;
        ni->ni_chan = ic->ic_curchan;
        ni->ni_fhdwell = vap->iv_bss->ni_fhdwell;
        ni->ni_fhindex = vap->iv_bss->ni_fhindex;
        if (wpa != NULL) {
            /*
             * Record WPA/RSN parameters for station, mark
             * node as using WPA and record information element
             * for applications that require it.
             */
            ni->ni_rsn = rsn;
            ieee80211_saveie(&ni->ni_wpa_ie, wpa);
        } else if (ni->ni_wpa_ie != NULL) {
            /*
             * Flush any state from a previous association.
             */
            FREE(ni->ni_wpa_ie, M_DEVBUF);
            ni->ni_wpa_ie = NULL;
        }
        if (wme != NULL) {
            /*
             * Record WME parameters for station, mark node
             * as capable of QoS and record information
             * element for applications that require it.
             */
            ieee80211_saveie(&ni->ni_wme_ie, wme);
            ni->ni_flags |= IEEE80211_NODE_QOS;
        } else if (ni->ni_wme_ie != NULL) {
            /*
             * Flush any state from a previous association.
             */
            FREE(ni->ni_wme_ie, M_DEVBUF);
            ni->ni_wme_ie = NULL;
            ni->ni_flags &= ~IEEE80211_NODE_QOS;
        }
        if (ath != NULL) {
            ieee80211_saveath(ni, ath);
        } else if (ni->ni_ath_ie != NULL) {
            /*
             * Flush any state from a previous association.
             */
            FREE(ni->ni_ath_ie, M_DEVBUF);
            ni->ni_ath_ie = NULL;
            ni->ni_ath_flags = 0;
        }

        /* 11n */
        if (IEEE80211_IS_CHAN_11N(ic->ic_curchan)) {
            if (htcap != NULL) {
                /* record capabilities, mark node as capable of HT */
                ieee80211_parse_htcap(ni, htcap);
            } else {
                /*
                 * Flush any state from a previous association.
                 */
                ni->ni_flags &= ~IEEE80211_NODE_HT;
                ni->ni_htcap = 0;
            }
            rate = ieee80211_setup_ht_rates(ni, htcap,
                IEEE80211_F_DOFRATE | IEEE80211_F_DOXSECT |
                IEEE80211_F_DOBRS);
            if (!rate) {
                IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ANY, wh->i_addr2,
                    "deny %s request, ht rate set mismatch",
                    reassoc ? "reassoc" : "assoc");
                IEEE80211_SEND_MGMT(ni, resp,
                    IEEE80211_STATUS_BASIC_RATE);
                ieee80211_node_leave(ni);
                vap->iv_stats.is_rx_assoc_norate++;
                return;
            }
        }
        /* Send TGf L2UF frame on behalf of newly associated station */
        ieee80211_deliver_l2uf(ni);
        ieee80211_node_join(ni, resp);
#ifdef ATH_SUPERG_XR

        if(ni->ni_prev_vap && ni->ni_vap != ni->ni_prev_vap &&
           ni->ni_vap->iv_ath_cap & IEEE80211_ATHC_XR) {
            /*
             * node moved between XR and normal vap.
             * move the data between  XR and normal vap.
             */
            ic->ic_node_move_data(ni);
            ni->ni_prev_vap = ni->ni_vap;
        }
#endif
        break;
    }

    case IEEE80211_FC0_SUBTYPE_ASSOC_RESP:
    case IEEE80211_FC0_SUBTYPE_REASSOC_RESP: {
        u_int16_t capinfo, associd;
        u_int16_t status;

        if (vap->iv_opmode != IEEE80211_M_STA ||
            vap->iv_state != IEEE80211_S_ASSOC) {
            vap->iv_stats.is_rx_mgtdiscard++;
            return;
        }

        /*
         * asresp frame format
         *  [2] capability information
         *  [2] status
         *  [2] association ID
         *  [tlv] supported rates
         *  [tlv] extended supported rates
         *  [tlv] WME
         */
        IEEE80211_VERIFY_LENGTH(efrm - frm, 6);
        ni = vap->iv_bss;
        capinfo = le16toh(*(u_int16_t *)frm);
        frm += 2;
        status = le16toh(*(u_int16_t *)frm);
        frm += 2;
        if (status != 0) {
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ASSOC,
                wh->i_addr2,
                "%sassoc failed (reason %d)",
                ISREASSOC(subtype) ?  "re" : "", status);
            vap->iv_stats.is_rx_auth_fail++;    /* XXX */
            ieee80211_new_state(vap, IEEE80211_S_SCAN,
                IEEE80211_SCAN_FAIL_STATUS);
            return;
        }
        associd = le16toh(*(u_int16_t *)frm);
        frm += 2;

        rates = xrates = wme = NULL;
        while (frm < efrm) {
            switch (*frm) {
            case IEEE80211_ELEMID_RATES:
                rates = frm;
                break;
            case IEEE80211_ELEMID_XRATES:
                xrates = frm;
                break;
            case IEEE80211_ELEMID_VENDOR:
                if (iswmeoui(frm))
                    wme = frm;
                break;
            }
            frm += frm[1] + 2;
        }

        IEEE80211_VERIFY_ELEMENT(rates, IEEE80211_RATE_MAXSIZE);
        if (!ieee80211_setup_rates(ni, rates, xrates,
                      IEEE80211_F_DOSORT | IEEE80211_F_DOXSECT |
                      IEEE80211_F_DOBRS  | IEEE80211_F_DOFRATE))
        {
            IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ASSOC,
                wh->i_addr2,
                "%sassoc failed (rate set mismatch)",
                ISREASSOC(subtype) ?  "re" : "");
            vap->iv_stats.is_rx_assoc_norate++;
            ieee80211_new_state(vap, IEEE80211_S_SCAN,
                IEEE80211_SCAN_FAIL_STATUS);
            return;
        }
        ni->ni_capinfo = capinfo;
        ni->ni_associd = associd;
        if (wme != NULL &&
            ieee80211_parse_wmeparams(vap, wme, wh) >= 0) {
            ni->ni_flags |= IEEE80211_NODE_QOS;
            ieee80211_wme_updateparams(vap);
        } else
            ni->ni_flags &= ~IEEE80211_NODE_QOS;
        /*
         * Configure state now that we are associated.
         *
         * XXX may need different/additional driver callbacks?
         */
        if (IEEE80211_IS_CHAN_A(ic->ic_curchan) || 
	    IEEE80211_IS_CHAN_11NA(ic->ic_curchan) ||
            (ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE)) {
            ic->ic_flags |= IEEE80211_F_SHPREAMBLE;
            ic->ic_flags &= ~IEEE80211_F_USEBARKER;
        } else {
            ic->ic_flags &= ~IEEE80211_F_SHPREAMBLE;
            ic->ic_flags |= IEEE80211_F_USEBARKER;
        }
        ieee80211_set_shortslottime(ic,
			IEEE80211_IS_CHAN_A(ic->ic_curchan) || 
			IEEE80211_IS_CHAN_11NA(ic->ic_curchan) ||
			(ni->ni_capinfo & IEEE80211_CAPINFO_SHORT_SLOTTIME));
        /*
         * Honor ERP protection.
         *
         * NB: ni_erp should zero for non-11g operation
         *     but we check the channel characteristics
         *     just in case.
         */
        if ((IEEE80211_IS_CHAN_ANYG(ic->ic_curchan) || 
            IEEE80211_IS_CHAN_11NG(ic->ic_curchan)) &&
            (ni->ni_erp & IEEE80211_ERP_USE_PROTECTION))
            ic->ic_flags |= IEEE80211_F_USEPROT;
        else
            ic->ic_flags &= ~IEEE80211_F_USEPROT;
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_ASSOC, wh->i_addr2,
            "%sassoc success: %s preamble, %s slot time%s%s%s%s%s%s%s",
            ISREASSOC(subtype) ? "re" : "",
            ic->ic_flags&IEEE80211_F_SHPREAMBLE ? "short" : "long",
            ic->ic_flags&IEEE80211_F_SHSLOT ? "short" : "long",
            ic->ic_flags&IEEE80211_F_USEPROT ? ", protection" : "",
            ni->ni_flags & IEEE80211_NODE_QOS ? ", QoS" : "",
            IEEE80211_ATH_CAP(vap, ni, IEEE80211_NODE_TURBOP) ?
            ", turbo" : "",
            IEEE80211_ATH_CAP(vap, ni, IEEE80211_NODE_COMP) ?
            ", compression" : "",
            IEEE80211_ATH_CAP(vap, ni, IEEE80211_NODE_FF) ?
            ", fast-frames" : "",
            IEEE80211_ATH_CAP(vap, ni, IEEE80211_NODE_XR) ? ", XR" : "",
            IEEE80211_ATH_CAP(vap, ni, IEEE80211_NODE_AR) ? ", AR" : ""
        );
        ieee80211_new_state(vap, IEEE80211_S_RUN, subtype);
        break;
    }

    case IEEE80211_FC0_SUBTYPE_DEAUTH: {
        u_int16_t reason;

        if (vap->iv_state == IEEE80211_S_SCAN) {
            vap->iv_stats.is_rx_mgtdiscard++;
            return;
        }
        /*
         * deauth frame format
         *  [2] reason
         */
        IEEE80211_VERIFY_LENGTH(efrm - frm, 2);
        reason = le16toh(*(u_int16_t *)frm);
        vap->iv_stats.is_rx_deauth++;
        IEEE80211_NODE_STAT(ni, rx_deauth);

        IEEE80211_NOTE(vap, IEEE80211_MSG_AUTH, ni,
            "recv deauthenticate (reason %d)", reason);
        switch (vap->iv_opmode) {
        case IEEE80211_M_STA:
            ieee80211_new_state(vap, IEEE80211_S_AUTH,
                wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK);
            break;
        case IEEE80211_M_HOSTAP:
            if (ni != vap->iv_bss)
                ieee80211_node_leave(ni);
            break;
        default:
            vap->iv_stats.is_rx_mgtdiscard++;
            break;
        }
        break;
    }

    case IEEE80211_FC0_SUBTYPE_DISASSOC: {
        u_int16_t reason;

        if (vap->iv_state != IEEE80211_S_RUN &&
            vap->iv_state != IEEE80211_S_ASSOC &&
            vap->iv_state != IEEE80211_S_AUTH) {
            vap->iv_stats.is_rx_mgtdiscard++;
            return;
        }
        /*
         * disassoc frame format
         *  [2] reason
         */
        IEEE80211_VERIFY_LENGTH(efrm - frm, 2);
        reason = le16toh(*(u_int16_t *)frm);
        vap->iv_stats.is_rx_disassoc++;
        IEEE80211_NODE_STAT(ni, rx_disassoc);

        IEEE80211_NOTE(vap, IEEE80211_MSG_ASSOC, ni,
            "recv disassociate (reason %d)", reason);
        switch (vap->iv_opmode) {
        case IEEE80211_M_STA:
            ieee80211_new_state(vap, IEEE80211_S_ASSOC, 0);
            break;
        case IEEE80211_M_HOSTAP:
            if (ni != vap->iv_bss)
                ieee80211_node_leave(ni);
            break;
        default:
            vap->iv_stats.is_rx_mgtdiscard++;
            break;
        }
        break;
    }

    case IEEE80211_FC0_SUBTYPE_ACTION: {
        struct ieee80211_action *ia;
        struct ieee80211_action_ht_txchwidth *iachwidth;
        enum ieee80211_cwm_width  chwidth;
	struct ieee80211_action_ba_addbarequest *addbarequest;
	struct ieee80211_action_ba_addbaresponse *addbaresponse;
	struct ieee80211_action_ba_delba *delba;
	struct ieee80211_ba_parameterset baparamset;
	struct ieee80211_ba_seqctrl      basequencectrl;
	struct ieee80211_delba_parameterset delbaparamset;
	struct ieee80211_action_mgt_args actionargs;
	u_int16_t                	 statuscode;
	u_int16_t                	 batimeout;
	u_int16_t                	 reasoncode;

        if (vap->iv_state != IEEE80211_S_RUN &&
            vap->iv_state != IEEE80211_S_ASSOC &&
            vap->iv_state != IEEE80211_S_AUTH) {
            vap->iv_stats.is_rx_mgtdiscard++;
            return;
        }

        IEEE80211_VERIFY_LENGTH(efrm - frm, sizeof(struct ieee80211_action));

        ia = (struct ieee80211_action *) frm;

        vap->iv_stats.is_rx_action++;
        IEEE80211_NODE_STAT(ni, rx_action);
        IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
            "%s: action mgt frame (cat %d, act %d)",__func__, ia->ia_category, ia->ia_action);

        switch (ia->ia_category) {
        case IEEE80211_ACTION_CAT_QOS:
            IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
                    "%s: QoS action mgt frames not supported", __func__);
            vap->iv_stats.is_rx_mgtdiscard++;
            break;
        case IEEE80211_ACTION_CAT_BA:
            switch (ia->ia_action) {
            case IEEE80211_ACTION_BA_ADDBA_REQUEST:
                IEEE80211_VERIFY_LENGTH(efrm - frm, sizeof(struct ieee80211_action_ba_addbarequest));

                addbarequest = (struct ieee80211_action_ba_addbarequest *) frm;

		*(u_int16_t *)&baparamset 	= le16toh(*(u_int16_t *)&addbarequest->rq_baparamset);
		batimeout  			= le16toh(addbarequest->rq_batimeout);
		*(u_int16_t *)&basequencectrl 	= le16toh(*(u_int16_t *)&addbarequest->rq_basequencectrl);

		IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
		    "%s: ADDBA request action mgt frame. TID %d, buffer size %d",
		     __func__, baparamset.tid, baparamset.buffersize);

		/* Process ADDBA request and save response in per TID data structure */
		ic->ic_addba_requestprocess(ic, ni, addbarequest->rq_dialogtoken,           
					    &baparamset, batimeout, basequencectrl);

		/* Send ADDBA response */
		actionargs.category	= IEEE80211_ACTION_CAT_BA;
		actionargs.action	= IEEE80211_ACTION_BA_ADDBA_RESPONSE;
		actionargs.arg1		= baparamset.tid;
		actionargs.arg2		= 0;
		actionargs.arg3		= 0;
		IEEE80211_SEND_MGMT(ni, IEEE80211_FC0_SUBTYPE_ACTION, (int) &actionargs);
                break;
            case IEEE80211_ACTION_BA_ADDBA_RESPONSE:
		    IEEE80211_VERIFY_LENGTH(efrm - frm, sizeof(struct ieee80211_action_ba_addbaresponse));

		    addbaresponse = (struct ieee80211_action_ba_addbaresponse *) frm;

		    statuscode 			= le16toh(addbaresponse->rs_statuscode);
		    *(u_int16_t *)&baparamset 	= le16toh(*(u_int16_t *)&addbaresponse->rs_baparamset);
		    batimeout  			= le16toh(addbaresponse->rs_batimeout);
		    ic->ic_addba_responseprocess(ic, ni, statuscode, &baparamset, batimeout);
		    IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
			"%s: ADDBA response action mgt frame. TID %d, buffer size %d",
			 __func__, baparamset.tid, baparamset.buffersize);
		    break;
            case IEEE80211_ACTION_BA_DELBA:
		    IEEE80211_VERIFY_LENGTH(efrm - frm, sizeof(struct ieee80211_action_ba_delba));

		    delba = (struct ieee80211_action_ba_delba *) frm;
		    *(u_int16_t *)&delbaparamset= le16toh(*(u_int16_t *)&delba->dl_delbaparamset);
		    reasoncode 			= le16toh(delba->dl_reasoncode);
		    ic->ic_delba_process(ic, ni, &delbaparamset, reasoncode);
		    IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
			"%s: DELBA action mgt frame. TID %d, initiator %d, reason code %d",
			 __func__, delbaparamset.tid, delbaparamset.initiator, reasoncode);
		    break;
            default:
                IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
                        "%s: invalid BA action mgt frame", __func__);
                vap->iv_stats.is_rx_mgtdiscard++;
                break;
            }
            break;
        case IEEE80211_ACTION_CAT_HT:
            switch (ia->ia_action) {
            case IEEE80211_ACTION_HT_TXCHWIDTH:
                IEEE80211_VERIFY_LENGTH(efrm - frm, sizeof(struct ieee80211_action_ht_txchwidth));

                iachwidth = (struct ieee80211_action_ht_txchwidth *) frm;
                chwidth = (iachwidth->at_chwidth == IEEE80211_A_HT_TXCHWIDTH_2040) ?
                            IEEE80211_CWM_WIDTH40 : IEEE80211_CWM_WIDTH20;

                /* Check for channel width change */
                if (chwidth != ni->ni_chwidth) {
                    ni->ni_newchwidth = 1;
                }

                /* update node's recommended tx channel width */
                ni->ni_chwidth = chwidth;

                IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
                    "%s: HT txchwidth action mgt frame. Width %d (%s)",
                     __func__, chwidth, ni->ni_newchwidth? "new" : "no change");

                break;
            case IEEE80211_ACTION_HT_MIMOPWRSAVE:
                IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
                        "%s: HT mimo pwr save action mgt frame not implemented", __func__);
                vap->iv_stats.is_rx_mgtdiscard++;
                break;
            default:
                IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
                        "%s: invalid HT action mgt frame", __func__);
                vap->iv_stats.is_rx_mgtdiscard++;
                break;
            }
            break;
        default:
            IEEE80211_NOTE(vap, IEEE80211_MSG_ACTION, ni,
                    "%s: action mgt frame has invalid category %d", __func__, ia->ia_category);
            vap->iv_stats.is_rx_mgtdiscard++;
            break;
        }
        break;
    }
    default:
        IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
             wh, "mgt", "subtype 0x%x not handled", subtype);
        vap->iv_stats.is_rx_badsubtype++;
        break;
    }
#undef ISREASSOC
#undef ISPROBE
}
#undef IEEE80211_VERIFY_LENGTH
#undef IEEE80211_VERIFY_ELEMENT

/*
 * Process a received ps-poll frame.
 */
static void
ieee80211_recv_pspoll(struct ieee80211_node *ni, struct sk_buff *skb0)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ieee80211_frame_min *wh;
    struct sk_buff *skb;
    u_int16_t aid;
    int qlen;

    wh = (struct ieee80211_frame_min *)skb0->data;
    if (ni->ni_associd == 0) {
        IEEE80211_DISCARD(vap,
            IEEE80211_MSG_POWER | IEEE80211_MSG_DEBUG,
            (struct ieee80211_frame *) wh, "ps-poll",
            "%s", "unassociated station");
        vap->iv_stats.is_ps_unassoc++;
        IEEE80211_SEND_MGMT(ni, IEEE80211_FC0_SUBTYPE_DEAUTH,
            IEEE80211_REASON_NOT_ASSOCED);
        return;
    }

    aid = le16toh(*(u_int16_t *)wh->i_dur);
    if (aid != ni->ni_associd) {
        IEEE80211_DISCARD(vap,
            IEEE80211_MSG_POWER | IEEE80211_MSG_DEBUG,
            (struct ieee80211_frame *) wh, "ps-poll",
            "aid mismatch: sta aid 0x%x poll aid 0x%x",
            ni->ni_associd, aid);
        vap->iv_stats.is_ps_badaid++;
        IEEE80211_SEND_MGMT(ni, IEEE80211_FC0_SUBTYPE_DEAUTH,
            IEEE80211_REASON_NOT_ASSOCED);
        return;
    }

    /* Okay, take the first queued packet and put it out... */
    IEEE80211_NODE_SAVEQ_DEQUEUE(ni, skb, qlen);
    if (skb == NULL) {
        IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_POWER, wh->i_addr2,
            "%s", "recv ps-poll, but queue empty");
        ieee80211_send_nulldata(ieee80211_ref_node(ni));
        vap->iv_stats.is_ps_qempty++;   /* XXX node stat */
        if (vap->iv_set_tim != NULL)
            vap->iv_set_tim(ni, 0);     /* just in case */
        return;
    }
    /*
     * If there are more packets, set the more packets bit
     * in the packet dispatched to the station; otherwise
     * turn off the TIM bit.
     */
    if (qlen != 0) {
        IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni,
            "recv ps-poll, send packet, %u still queued", qlen);
        /*
         * NB: More-data bit will be set during encap.
         */
    } else {
        IEEE80211_NOTE(vap, IEEE80211_MSG_POWER, ni,
            "%s", "recv ps-poll, send packet, queue empty");
        if (vap->iv_set_tim != NULL)
            vap->iv_set_tim(ni, 0);
    }
    M_PWR_SAV_SET(skb);     /* bypass PS handling */
    skb->dev = vap->iv_dev;     /* XXX needed? */
    (void) dev_queue_xmit(skb); /* resubmit */
}

static void
athff_decap(struct sk_buff *skb)
{
    struct ether_header eh_src, *eh_dst;
    struct llc *llc;

    memcpy(&eh_src, skb->data, sizeof(struct ether_header));
    llc = (struct llc *) skb_pull(skb, sizeof(struct ether_header));
    eh_src.ether_type = llc->llc_un.type_snap.ether_type;
    skb_pull(skb, LLC_SNAPFRAMELEN);

    eh_dst = (struct ether_header *) skb_push(skb, sizeof(struct ether_header));
    memcpy(eh_dst, &eh_src, sizeof(struct ether_header));
}


/* 
 * Extracts sub-frames of an 802.11n A-MSDU and passes them up the stack. 
 * The sub-frame header is same as the 802.3 header, with the typeOrLen 
 * field indicating the length of the subframe.
 */
static int
ieee80211_amsdu_input(struct ieee80211_node *ni, struct sk_buff *skb)
{
    struct ieee80211vap *vap = ni->ni_vap;
    struct ether_header *subfrmhdr;
    int subfrm_len;

    /* Remove the ethernet header inserted by ieee80211_decap */
    skb_pull(skb, sizeof(struct ether_header));
                
    while (skb->len >= sizeof(struct ether_header)) {
        struct sk_buff *skb_subfrm;
	
        subfrmhdr = (struct ether_header *)skb->data;
        subfrm_len = ntohs(subfrmhdr->ether_type);

        if(subfrm_len < sizeof(LLC_SNAPFRAMELEN)) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT, 
                subfrmhdr->ether_shost, NULL, 
                "A-MSDU sub-frame too short: len %u", 
                subfrm_len);
            goto err_amsdu;
        }

        subfrm_len += sizeof(struct ether_header);

        if (skb->len == subfrm_len) {
            amsdu_subframe_decap(skb);
            ieee80211_deliver_data(ni, skb);
            return IEEE80211_FC0_TYPE_DATA;
        } 

        if(skb->len < roundup(subfrm_len, 4)) {
            IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT, 
                subfrmhdr->ether_shost, NULL, "Short A-MSDU: len %u", skb->len);
            goto err_amsdu;
        }

        skb_subfrm = skb_clone(skb, GFP_ATOMIC);
        skb_pull(skb, roundup(subfrm_len, 4));
                
        /* Convert the sub-frame into DIX frame */
        amsdu_subframe_decap(skb_subfrm);

        /* Remove any subsequent frames at the end of current sub-frame */
        skb_trim(skb_subfrm, subfrm_len - LLC_SNAPFRAMELEN);

        //subfrm_cnt++;
        ieee80211_deliver_data(ni, skb_subfrm);
    }
err_amsdu:
    /* Discard any malformed A-MSDU */
    dev_kfree_skb(skb);
    return IEEE80211_FC0_TYPE_DATA;
}

#ifdef USE_HEADERLEN_RESV
/*
 * The kernel version of this function alters the skb in a manner
 * inconsistent with dev->hard_header_len header reservation. This
 * is a rewrite of the portion of eth_type_trans() that we need.
 */
static unsigned short
ath_eth_type_trans(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr *eth;

    skb->mac.raw=skb->data;
    skb_pull(skb, ETH_HLEN);
    /*
     * NB: mac.ethernet is replaced in 2.6.9 by eth_hdr but
     *     since that's an inline and not a define there's
     *     no easy way to do this cleanly.
     */
    eth= (struct ethhdr *)skb->mac.raw;

    if(*eth->h_dest&1)
    {
        if(memcmp(eth->h_dest,dev->broadcast, ETH_ALEN)==0)
            skb->pkt_type=PACKET_BROADCAST;
        else
            skb->pkt_type=PACKET_MULTICAST;
    }

    /*
     *  This ALLMULTI check should be redundant by 1.4
     *  so don't forget to remove it.
     *
     *  Seems, you forgot to remove it. All silly devices
     *  seems to set IFF_PROMISC.
     */

    else if(1 /*dev->flags&IFF_PROMISC*/)
    {
        if(memcmp(eth->h_dest,dev->dev_addr, ETH_ALEN))
            skb->pkt_type=PACKET_OTHERHOST;
    }

    return eth->h_proto;
}
#endif

/*
 * Process a frame w/ hw detected MIC failure. 
 * The frame will be dropped in any case.
 */
void
ieee80211_check_mic(struct ieee80211_node *ni, struct sk_buff *skb)
{
	struct ieee80211vap *vap = ni->ni_vap;

	struct ieee80211_frame *wh;
	struct ieee80211_key *key;
	int hdrspace;
	struct ieee80211com *ic = vap->iv_ic;
	
	if (skb->len < sizeof(struct ieee80211_frame_min)) {
		IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_ANY,
		    ni->ni_macaddr, NULL,
		    "too short (1): len %u", skb->len);
		vap->iv_stats.is_rx_tooshort++;
		return;
	}

	wh = (struct ieee80211_frame *)skb->data;
		
	hdrspace = ieee80211_hdrspace(ic, wh);
	key = ieee80211_crypto_decap(ni, skb, hdrspace);
	if (key == NULL) {
		/* NB: stats+msgs handled in crypto_decap */
		IEEE80211_NODE_STAT(ni, rx_wepfail);
		return;
	}

	if (!ieee80211_crypto_demic(vap, key, skb, hdrspace, 1)) {
		IEEE80211_DISCARD_MAC(vap, IEEE80211_MSG_INPUT,
			ni->ni_macaddr, "data", "%s", "demic error");
			IEEE80211_NODE_STAT(ni, rx_demicfail);
	}
	return;
}
EXPORT_SYMBOL(ieee80211_check_mic);

#ifdef IEEE80211_DEBUG
/*
 * Debugging support.
 */

/*
 * Return the bssid of a frame.
 */
static const u_int8_t *
ieee80211_getbssid(struct ieee80211vap *vap, const struct ieee80211_frame *wh)
{
    if (vap->iv_opmode == IEEE80211_M_STA)
        return wh->i_addr2;
    if ((wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) != IEEE80211_FC1_DIR_NODS)
        return wh->i_addr1;
    if ((wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) == IEEE80211_FC0_SUBTYPE_PS_POLL)
        return wh->i_addr1;
    return wh->i_addr3;
}

void
ieee80211_note(struct ieee80211vap *vap, const char *fmt, ...)
{
    char buf[128];      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printk("%s: %s", vap->iv_dev->name, buf);   /* NB: no \n */
}
EXPORT_SYMBOL(ieee80211_note);

void
ieee80211_note_frame(struct ieee80211vap *vap,
    const struct ieee80211_frame *wh,
    const char *fmt, ...)
{
    char buf[128];      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printk("%s: [%s] %s\n", vap->iv_dev->name,
        ether_sprintf(ieee80211_getbssid(vap, wh)), buf);
}
EXPORT_SYMBOL(ieee80211_note_frame);

void
ieee80211_note_mac(struct ieee80211vap *vap,
    const u_int8_t mac[IEEE80211_ADDR_LEN],
    const char *fmt, ...)
{
    char buf[128];      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printk("%s: [%s] %s\n", vap->iv_dev->name, ether_sprintf(mac), buf);
}
EXPORT_SYMBOL(ieee80211_note_mac);

static void
ieee80211_discard_frame(struct ieee80211vap *vap,
    const struct ieee80211_frame *wh,
    const char *type, const char *fmt, ...)
{
    char buf[128];      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (type != NULL)
        printk("[%s:%s] discard %s frame, %s\n", vap->iv_dev->name,
            ether_sprintf(ieee80211_getbssid(vap, wh)), type, buf);
    else
        printk("[%s:%s] discard frame, %s\n", vap->iv_dev->name,
            ether_sprintf(ieee80211_getbssid(vap, wh)), buf);
}

static void
ieee80211_discard_ie(struct ieee80211vap *vap,
    const struct ieee80211_frame *wh,
    const char *type, const char *fmt, ...)
{
    char buf[128];      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (type != NULL)
        printk("[%s:%s] discard %s information element, %s\n",
            vap->iv_dev->name,
            ether_sprintf(ieee80211_getbssid(vap, wh)), type, buf);
    else
        printk("[%s:%s] discard information element, %s\n",
            vap->iv_dev->name,
            ether_sprintf(ieee80211_getbssid(vap, wh)), buf);
}

static void
ieee80211_discard_mac(struct ieee80211vap *vap,
    const u_int8_t mac[IEEE80211_ADDR_LEN],
    const char *type, const char *fmt, ...)
{
    char buf[128];      /* XXX */
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (type != NULL)
        printk("[%s:%s] discard %s frame, %s\n", vap->iv_dev->name,
            ether_sprintf(mac), type, buf);
    else
        printk("[%s:%s] discard frame, %s\n", vap->iv_dev->name,
            ether_sprintf(mac), buf);
}
#endif /* IEEE80211_DEBUG */
