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
 *
 * $FreeBSD: src/sys/net80211/ieee80211_node.h,v 1.12 2004/12/31 22:42:38 sam Exp $
 */
#ifndef _NET80211_IEEE80211_NODE_H_
#define _NET80211_IEEE80211_NODE_H_

#include <net80211/ieee80211_ioctl.h>		/* for ieee80211_nodestats */

/*
 * Each ieee80211com instance has a single timer that fires once a
 * second.  This is used to initiate various work depending on the
 * state of the instance: scanning (passive or active), ``transition''
 * (waiting for a response to a management frame when operating
 * as a station), and node inactivity processing (when operating
 * as an AP).  For inactivity processing each node has a timeout
 * set in it's ni_inact field that is decremented on each timeout
 * and the node is reclaimed when the counter goes to zero.  We
 * use different inactivity timeout values depending on whether
 * the node is associated and authorized (either by 802.1x or
 * open/shared key authentication) or associated but yet to be
 * authorized.  The latter timeout is shorter to more aggressively
 * reclaim nodes that leave part way through the 802.1x exchange.
 */
#define	IEEE80211_INACT_WAIT	15		/* inactivity interval (secs) */
#define	IEEE80211_INACT_INIT	(30/IEEE80211_INACT_WAIT)	/* initial */
#define	IEEE80211_INACT_AUTH	(180/IEEE80211_INACT_WAIT)	/* associated but not authorized */
#define	IEEE80211_INACT_RUN	(300/IEEE80211_INACT_WAIT)	/* authorized */
#define	IEEE80211_INACT_PROBE	(30/IEEE80211_INACT_WAIT)	/* probe */
#define	IEEE80211_INACT_SCAN	(300/IEEE80211_INACT_WAIT)	/* scanned */

#define	IEEE80211_TRANS_WAIT 	5		/* mgt frame tx timer (secs) */

#define	IEEE80211_NODE_HASHSIZE	32
/* simple hash is enough for variation of macaddr */
#define	IEEE80211_NODE_HASH(addr)	\
	(((const u_int8_t *)(addr))[IEEE80211_ADDR_LEN - 1] % \
		IEEE80211_NODE_HASHSIZE)

struct ieee80211_rsnparms {
	u_int8_t	rsn_mcastcipher;	/* mcast/group cipher */
	u_int8_t	rsn_mcastkeylen;	/* mcast key length */
	u_int8_t	rsn_ucastcipherset;	/* unicast cipher set */
	u_int8_t	rsn_ucastcipher;	/* selected unicast cipher */
	u_int8_t	rsn_ucastkeylen;	/* unicast key length */
	u_int8_t	rsn_keymgmtset;		/* key mangement algorithms */
	u_int8_t	rsn_keymgmt;		/* selected key mgmt algo */
	u_int16_t	rsn_caps;		/* capabilities */
};

struct ieee80211_node_table;
struct ieee80211com;
struct ieee80211vap;

/*
 * Node specific information.  Note that drivers are expected
 * to derive from this structure to add device-specific per-node
 * state.  This is done by overriding the ic_node_* methods in
 * the ieee80211com structure.
 */
struct ieee80211_node {
	struct ieee80211vap	*ni_vap;
	struct ieee80211com	*ni_ic;
	struct ieee80211_node_table *ni_table;
	TAILQ_ENTRY(ieee80211_node)	ni_list;
	LIST_ENTRY(ieee80211_node)	ni_hash;
	atomic_t		ni_refcnt;
	u_int			ni_scangen;	/* gen# for timeout scan */
	u_int8_t		ni_authmode;	/* authentication algorithm */
	u_int16_t		ni_flags;	/* special-purpose state */
#define	IEEE80211_NODE_AUTH	0x0001		/* authorized for data */
#define	IEEE80211_NODE_QOS	0x0002		/* QoS enabled */
#define	IEEE80211_NODE_ERP	0x0004		/* ERP enabled */
#define	IEEE80211_NODE_HT	0x0008		/* HT enabled */

/* NB: this must have the same value as IEEE80211_FC1_PWR_MGT */
#define	IEEE80211_NODE_PWR_MGT	0x0010		/* power save mode enabled */
#define	IEEE80211_NODE_AREF	0x0020		/* authentication ref held */
	u_int8_t		ni_ath_flags;	/* Atheros feature flags */
	/* NB: These must have the same values as IEEE80211_ATHC_* */
#define IEEE80211_NODE_TURBOP	0x0001		/* Turbo prime enable */
#define IEEE80211_NODE_COMP	0x0002		/* Compresssion enable */
#define IEEE80211_NODE_FF	0x0004          /* Fast Frame capable */
#define IEEE80211_NODE_XR	0x0008		/* Atheros WME enable */
#define IEEE80211_NODE_AR	0x0010		/* AR capable */
#define IEEE80211_NODE_BOOST	0x0080 
	u_int16_t               ni_ath_defkeyindex; /* Atheros def key index */
#define IEEE80211_INVAL_DEFKEY	0x7FFF
	u_int16_t		ni_associd;	/* assoc response */
	u_int16_t		ni_txpower;	/* current transmit power */
	u_int16_t		ni_vlan;	/* vlan tag */
	u_int32_t		*ni_challenge;	/* shared-key challenge */
	u_int8_t		*ni_wpa_ie;	/* captured WPA/RSN ie */
	u_int8_t		*ni_wme_ie;	/* captured WME ie */
	u_int8_t		*ni_ath_ie;	/* captured Atheros ie */
	u_int16_t		ni_txseqs[17];	/* tx seq per-tid */
	u_int16_t		ni_rxseqs[17];	/* rx seq previous per-tid*/
	u_int32_t		ni_rxfragstamp;	/* time stamp of last rx frag */
	struct sk_buff		*ni_rxfrag[3];	/* rx frag reassembly */
	struct ieee80211_rsnparms ni_rsn;	/* RSN/WPA parameters */
	struct ieee80211_key	ni_ucastkey;	/* unicast key */
	int			ni_rxkeyoff;    /* Receive key offset */

	/* 11n Capabilities */
	u_int16_t 		ni_htcap;	/* HT capabilities */
	u_int16_t		ni_maxampdu;	/* maximum rx A-MPDU length */
	u_int8_t		ni_mpdudensity;	/* MPDU density */

	/* 11n information */
        enum ieee80211_cwm_width  ni_chwidth;	/* recommended tx channel width */
	u_int8_t		ni_newchwidth; 	/* channel width changed */

	/* hardware */
	u_int32_t		ni_rstamp;	/* recv timestamp */
	u_int8_t		ni_rssi;	/* recv ssi */

	/* header */
	u_int8_t		ni_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t		ni_bssid[IEEE80211_ADDR_LEN];

	/* beacon, probe response */
	union {
		u_int8_t	data[8];
		u_int64_t	tsf;
	} ni_tstamp;				/* from last rcv'd beacon */
	u_int16_t		ni_intval;	/* beacon interval */
	u_int16_t		ni_capinfo;	/* capabilities */
	u_int8_t		ni_esslen;
	u_int8_t		ni_essid[IEEE80211_NWID_LEN];
	struct ieee80211_rateset ni_rates;	/* negotiated rate set */
	struct ieee80211_rateset ni_htrates;	/* negotiated ht rate set */
	struct ieee80211_channel *ni_chan;
	u_int16_t		ni_fhdwell;	/* FH only */
	u_int8_t		ni_fhindex;	/* FH only */
	u_int8_t		ni_erp;		/* ERP from beacon/probe resp */
	u_int16_t		ni_timoff;	/* byte offset to TIM ie */

	/* others */
	struct sk_buff_head	ni_savedq;	/* packets queued for pspoll */
	u_int32_t		ni_pwrsavedrops;/* pspoll packets dropped */
	short			ni_inact;	/* inactivity mark count */
	short			ni_inact_reload;/* inactivity reload value */
	int			ni_txrate;	/* index to ni_rates[] */
	struct ieee80211_nodestats ni_stats;	/* per-node statistics */
	struct ieee80211vap	*ni_prev_vap;  /* previously associated vap */
};
MALLOC_DECLARE(M_80211_NODE);

#define	IEEE80211_NODE_AID(ni)	IEEE80211_AID(ni->ni_associd)

#define	IEEE80211_NODE_STAT(ni,stat)	(ni->ni_stats.ns_##stat++)
#define	IEEE80211_NODE_STAT_ADD(ni,stat,v)	(ni->ni_stats.ns_##stat += v)
#define	IEEE80211_NODE_STAT_SET(ni,stat,v)	(ni->ni_stats.ns_##stat = v)

static __inline struct ieee80211_node *
ieee80211_ref_node(struct ieee80211_node *ni)
{
	ieee80211_node_incref(ni);
	return ni;
}

static __inline void
ieee80211_unref_node(struct ieee80211_node **ni)
{
	ieee80211_node_decref(*ni);
	*ni = NULL;			/* guard against use */
}

void	ieee80211_node_attach(struct ieee80211com *);
void	ieee80211_node_detach(struct ieee80211com *);
void	ieee80211_node_vattach(struct ieee80211vap *);
void	ieee80211_node_latevattach(struct ieee80211vap *);
void	ieee80211_node_vdetach(struct ieee80211vap *);

static __inline int
ieee80211_node_is_authorized(const struct ieee80211_node *ni)
{
	return (ni->ni_flags & IEEE80211_NODE_AUTH);
}

void	ieee80211_node_authorize(struct ieee80211_node *);
void	ieee80211_node_unauthorize(struct ieee80211_node *);

void	ieee80211_create_ibss(struct ieee80211vap*, struct ieee80211_channel *);
void	ieee80211_reset_bss(struct ieee80211vap *);
int	ieee80211_ibss_merge(struct ieee80211_node *);
struct ieee80211_scan_entry;
int	ieee80211_sta_join(struct ieee80211vap *,
		const struct ieee80211_scan_entry *);
void	ieee80211_sta_leave(struct ieee80211_node *);

struct ieee80211_wds_addr {
	LIST_ENTRY(ieee80211_wds_addr)	wds_hash;
	u_int8_t	wds_macaddr[IEEE80211_ADDR_LEN];
	struct ieee80211_node	*wds_ni;
};
	
/*
 * Table of ieee80211_node instances.  Each ieee80211com
 * has at least one for holding the scan candidates.
 * When operating as an access point or in ibss mode there
 * is a second table for associated stations or neighbors.
 */
struct ieee80211_node_table {
	struct ieee80211com	*nt_ic;		/* back reference */
	ieee80211_node_lock_t	nt_nodelock;	/* on node table */
	TAILQ_HEAD(, ieee80211_node) nt_node;	/* information of all nodes */
	ATH_LIST_HEAD(, ieee80211_node) nt_hash[IEEE80211_NODE_HASHSIZE];
	ATH_LIST_HEAD(, ieee80211_wds_addr) nt_wds_hash[IEEE80211_NODE_HASHSIZE];
	const char		*nt_name;	/* for debugging */
	ieee80211_scan_lock_t	nt_scanlock;	/* on nt_scangen */
	u_int			nt_scangen;	/* gen# for timeout scan */
	int			nt_inact_init;	/* initial node inact setting */
};

struct ieee80211_node *ieee80211_alloc_node(struct ieee80211_node_table *,
		struct ieee80211vap *, const u_int8_t *);
struct ieee80211_node *ieee80211_tmp_node(struct ieee80211vap *,
		const u_int8_t *);
struct ieee80211_node *ieee80211_dup_bss(struct ieee80211vap *,
		const u_int8_t *);
void ieee80211_node_reset(struct ieee80211_node *ni,
		 struct ieee80211vap *vap);
#ifdef IEEE80211_DEBUG_REFCNT
void	ieee80211_free_node_debug(struct ieee80211_node *,
		const char *func, int line);
struct ieee80211_node *ieee80211_find_node_debug(
		struct ieee80211_node_table *, const u_int8_t *,
		const char *func, int line);
struct ieee80211_node * ieee80211_find_rxnode_debug(
		struct ieee80211com *, const struct ieee80211_frame_min *,
		const char *func, int line);
struct ieee80211_node *ieee80211_find_txnode_debug(
		struct ieee80211vap *, const u_int8_t *,
		const char *func, int line);
#define	ieee80211_free_node(ni) \
	ieee80211_free_node_debug(ni, __func__, __LINE__)
#define	ieee80211_find_node(nt, mac) \
	ieee80211_find_node_debug(nt, mac, __func__, __LINE__)
#define	ieee80211_find_rxnode(nt, wh) \
	ieee80211_find_rxnode_debug(nt, wh, __func__, __LINE__)
#define	ieee80211_find_txnode(nt, mac) \
	ieee80211_find_txnode_debug(nt, mac, __func__, __LINE__)
#else
void	ieee80211_free_node(struct ieee80211_node *);

struct ieee80211_node *ieee80211_find_node(
		struct ieee80211_node_table *, const u_int8_t *);
struct ieee80211_node * ieee80211_find_rxnode(
		struct ieee80211com *, const struct ieee80211_frame_min *);
struct ieee80211_node *ieee80211_find_txnode(
		struct ieee80211vap *, const u_int8_t *);
#endif
int	ieee80211_add_wds_addr(struct ieee80211_node_table *,
			       struct ieee80211_node *, const u_int8_t *);
void	ieee80211_del_wds_node(struct ieee80211_node_table *,
			       struct ieee80211_node *);
struct ieee80211_node *ieee80211_find_wds_node(struct ieee80211_node_table *,
					       const u_int8_t *);

typedef void ieee80211_iter_func(void *, struct ieee80211_node *);
void	ieee80211_iterate_nodes(struct ieee80211_node_table *,
		ieee80211_iter_func *, void *);

void	ieee80211_dump_node(struct ieee80211_node_table *,
		struct ieee80211_node *);
void	ieee80211_dump_nodes(struct ieee80211_node_table *);

struct ieee80211_node *ieee80211_fakeup_adhoc_node(struct ieee80211vap *vap,
		const u_int8_t macaddr[]);
struct ieee80211_scanparams;
struct ieee80211_node *ieee80211_add_neighbor(struct ieee80211vap *,
		const struct ieee80211_frame *,
		const struct ieee80211_scanparams *);
void	ieee80211_node_join(struct ieee80211_node *, int);
void	ieee80211_node_leave(struct ieee80211_node *);
u_int8_t ieee80211_getrssi(struct ieee80211com *);
#endif /* _NET80211_IEEE80211_NODE_H_ */
