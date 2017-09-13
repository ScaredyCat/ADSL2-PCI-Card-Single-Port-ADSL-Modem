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
 * $FreeBSD: src/sys/net80211/ieee80211_proto.h,v 1.8 2004/12/31 22:42:38 sam Exp $
 */
#ifndef _NET80211_IEEE80211_PROTO_H_
#define _NET80211_IEEE80211_PROTO_H_

/*
 * 802.11 protocol implementation definitions.
 */

enum ieee80211_state {
	IEEE80211_S_INIT	= 0,	/* default state */
	IEEE80211_S_SCAN	= 1,	/* scanning */
	IEEE80211_S_JOIN	= 2,	/* join */
	IEEE80211_S_AUTH	= 3,	/* try to authenticate */
	IEEE80211_S_ASSOC	= 4,	/* try to assoc */
	IEEE80211_S_RUN		= 5,	/* associated */
};
#define	IEEE80211_S_MAX		(IEEE80211_S_RUN+1)

#define	IEEE80211_SEND_MGMT(_ni,_type,_arg) \
	((*(_ni)->ni_ic->ic_send_mgmt)(_ni, _type, _arg))

struct ieee80211_action_mgt_args {
	u_int8_t 	category;
	u_int8_t	action;
	u_int32_t	arg1;
	u_int32_t	arg2;
	u_int32_t	arg3;
};

extern	const char *ieee80211_mgt_subtype_name[];
extern	const char *ieee80211_ctl_subtype_name[];
extern	const char *ieee80211_state_name[IEEE80211_S_MAX];
extern	const char *ieee80211_wme_acnames[];
extern	const char *ieee80211_phymode_name[];

void	ieee80211_proto_attach(struct ieee80211com *);
void	ieee80211_proto_detach(struct ieee80211com *);
void	ieee80211_proto_vattach(struct ieee80211vap *);
void	ieee80211_proto_vdetach(struct ieee80211vap *);

struct ieee80211_node;
int	ieee80211_input(struct ieee80211_node *,
		struct sk_buff *, int, u_int32_t);
int	ieee80211_input_all(struct ieee80211com *,
		struct sk_buff *, int, u_int32_t);
int	ieee80211_setup_rates(struct ieee80211_node *,
		const u_int8_t *, const u_int8_t *, int);
int	ieee80211_setup_ht_rates(struct ieee80211_node *, u_int8_t *,int);
void	ieee80211_setup_basic_ht_rates(struct ieee80211_node *,u_int8_t *);
void	ieee80211_saveie(u_int8_t **, const u_int8_t *);
void	ieee80211_saveath(struct ieee80211_node *, u_int8_t *);
void    ieee80211_parse_htcap(struct ieee80211_node *ni, u_int8_t *ie);
void	ieee80211_parse_htinfo(struct ieee80211_node *ni, u_int8_t *ie);
void	ieee80211_recv_mgmt(struct ieee80211_node *, struct sk_buff *,
		int, int, u_int32_t);
void	ieee80211_sta_pwrsave(struct ieee80211vap *, int enable);
int	ieee80211_hardstart(struct sk_buff *, struct net_device *);
int	ieee80211_send_nulldata(struct ieee80211_node *);
int	ieee80211_send_mgmt(struct ieee80211_node *, int, int);
int	ieee80211_send_probereq(struct ieee80211_node *,
		const u_int8_t sa[IEEE80211_ADDR_LEN],
		const u_int8_t da[IEEE80211_ADDR_LEN],
		const u_int8_t bssid[IEEE80211_ADDR_LEN],
		const u_int8_t *ssid, size_t ssidlen,
		const void *optie, size_t optielen);
struct sk_buff *ieee80211_encap(struct ieee80211_node *, struct sk_buff *, int *);
void	ieee80211_pwrsave(struct ieee80211_node *, struct sk_buff *);

void	ieee80211_reset_erp(struct ieee80211com *, enum ieee80211_phymode);
void	ieee80211_set_shortslottime(struct ieee80211com *, int onoff);
int	ieee80211_iserp_rateset(struct ieee80211com *,
		struct ieee80211_rateset *);
void	ieee80211_set11gbasicrates(struct ieee80211_rateset *,
		enum ieee80211_phymode);
void	ieee80211_setpuregbasicrates(struct ieee80211_rateset *);
enum ieee80211_phymode ieee80211_get11gbasicrates(struct ieee80211_rateset *);
void	ieee80211_check_mic(struct ieee80211_node *, struct sk_buff *);

/*
 * Return the size of the 802.11 header for a management or data frame.
 */
static __inline int
ieee80211_hdrsize(const void *data)
{
	const struct ieee80211_frame *wh = data;
	int size = sizeof(struct ieee80211_frame);

	/* NB: we don't handle control frames */
	KASSERT((wh->i_fc[0]&IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_CTL,
		("%s: control frame", __func__));
	if ((wh->i_fc[1] & IEEE80211_FC1_DIR_MASK) == IEEE80211_FC1_DIR_DSTODS)
		size += IEEE80211_ADDR_LEN;
	if (IEEE80211_QOS_HAS_SEQ(wh))
		size += sizeof(u_int16_t);
	return size;
}

/*
 * Like ieee80211_hdrsize, but handles any type of frame.
 */
static __inline int
ieee80211_anyhdrsize(const void *data)
{
	const struct ieee80211_frame *wh = data;

	if ((wh->i_fc[0]&IEEE80211_FC0_TYPE_MASK) == IEEE80211_FC0_TYPE_CTL) {
		switch (wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK) {
		case IEEE80211_FC0_SUBTYPE_CTS:
		case IEEE80211_FC0_SUBTYPE_ACK:
			return sizeof(struct ieee80211_frame_ack);
		}
		return sizeof(struct ieee80211_frame_min);
	} else
		return ieee80211_hdrsize(data);
}

/*
 * Template for an in-kernel authenticator.  Authenticators
 * register with the protocol code and are typically loaded
 * as separate modules as needed.
 */
struct ieee80211_authenticator {
	const char *ia_name;		/* printable name */
	int	(*ia_attach)(struct ieee80211vap *);
	void	(*ia_detach)(struct ieee80211vap *);
	void	(*ia_node_join)(struct ieee80211_node *);
	void	(*ia_node_leave)(struct ieee80211_node *);
};
void	ieee80211_authenticator_register(int type,
		const struct ieee80211_authenticator *);
void	ieee80211_authenticator_unregister(int type);
const struct ieee80211_authenticator *ieee80211_authenticator_get(int auth);

struct eapolcom;
/*
 * Template for an in-kernel authenticator backend.  Backends
 * register with the protocol code and are typically loaded
 * as separate modules as needed.
 */
struct ieee80211_authenticator_backend {
	const char *iab_name;		/* printable name */
	int	(*iab_attach)(struct eapolcom *);
	void	(*iab_detach)(struct eapolcom *);
};
void	ieee80211_authenticator_backend_register(
		const struct ieee80211_authenticator_backend *);
void	ieee80211_authenticator_backend_unregister(
		const struct ieee80211_authenticator_backend *);
const struct ieee80211_authenticator_backend *
	ieee80211_authenticator_backend_get(const char *name);

/*
 * Template for an MAC ACL policy module.  Such modules
 * register with the protocol code and are passed the sender's
 * address of each received frame for validation.
 */
struct ieee80211_aclator {
	const char *iac_name;		/* printable name */
	int	(*iac_attach)(struct ieee80211com *);
	void	(*iac_detach)(struct ieee80211com *);
	int	(*iac_check)(struct ieee80211vap *,
			const u_int8_t mac[IEEE80211_ADDR_LEN]);
	int	(*iac_add)(struct ieee80211vap *,
			const u_int8_t mac[IEEE80211_ADDR_LEN]);
	int	(*iac_remove)(struct ieee80211vap *,
			const u_int8_t mac[IEEE80211_ADDR_LEN]);
	int	(*iac_flush)(struct ieee80211vap *);
	int	(*iac_setpolicy)(struct ieee80211vap *, int);
	int	(*iac_getpolicy)(struct ieee80211vap *);
};
void	ieee80211_aclator_register(const struct ieee80211_aclator *);
void	ieee80211_aclator_unregister(const struct ieee80211_aclator *);
const struct ieee80211_aclator *ieee80211_aclator_get(const char *name);

/* flags for ieee80211_fix_rate() */
#define	IEEE80211_F_DOSORT	0x00000001	/* sort rate list */
#define	IEEE80211_F_DOFRATE	0x00000002	/* use fixed rate */
#define	IEEE80211_F_DOXSECT	0x00000004	/* intersection of rates  */
#define	IEEE80211_F_DOBRS	0x00000008	/* check for basic rates */

void	ieee80211_sort_rate(struct ieee80211_rateset *);
void    ieee80211_xsect_rate(struct ieee80211_rateset *, 
		struct ieee80211_rateset *, struct ieee80211_rateset *);
int     ieee80211_brs_rate_check(struct ieee80211_rateset *, 
		struct ieee80211_rateset *);
int     ieee80211_fixed_rate_check(struct ieee80211_node *, 
		struct ieee80211_rateset *);
int 	ieee80211_fixed_htrate_check(struct ieee80211_node *, 
		struct ieee80211_rateset *);

struct wmeParams {
	u_int8_t	wmep_acm;		/* ACM parameter */
	u_int8_t	wmep_aifsn;		/* AIFSN parameters */
	u_int8_t	wmep_logcwmin;		/* cwmin in exponential form */
	u_int8_t	wmep_logcwmax;		/* cwmax in exponential form */
	u_int16_t	wmep_txopLimit;		/* txopLimit */
	u_int8_t        wmep_noackPolicy;       /* No-Ack Policy: 0=ack, 1=no-ack */
};

#define IEEE80211_EXPONENT_TO_VALUE(_exp)  (1 << (u_int32_t)(_exp)) - 1
#define IEEE80211_TXOP_TO_US(_txop)  (u_int32_t)(_txop) << 5
#define IEEE80211_US_TO_TXOP(_us)  (u_int16_t)((u_int32_t)(_us)) >> 5

struct chanAccParams{
	u_int8_t		cap_info;		 	/* ver. of the current param set */
	struct wmeParams	cap_wmeParams[WME_NUM_AC];	/*WME params for each access class */ 
};

struct ieee80211_wme_state {
	u_int	wme_flags;
#define	WME_F_AGGRMODE	0x00000001	/* STATUS: WME agressive mode */
	u_int	wme_hipri_traffic;	/* VI/VO frames in beacon interval */
	u_int	wme_hipri_switch_thresh;/* agressive mode switch thresh */
	u_int	wme_hipri_switch_hysteresis;/* agressive mode switch hysteresis */

	struct chanAccParams	wme_wmeChanParams;	/* configured WME parameters applied to itself*/
	struct chanAccParams	wme_wmeBssChanParams;	/* configured WME parameters broadcasted to STAs*/
	struct chanAccParams	wme_chanParams;		/* channel parameters applied to itself*/
	struct chanAccParams	wme_bssChanParams;	/* channel parameters broadcasted to STAs*/
	u_int8_t                wme_nonAggressiveMode;   /* don't use aggressive params and use WME params */

	/* update hardware tx params after wme state change */
	int	(*wme_update)(struct ieee80211com *);
};

void	ieee80211_wme_initparams(struct ieee80211vap *);
void	ieee80211_wme_initparams_locked(struct ieee80211vap *);
void	ieee80211_wme_updateparams(struct ieee80211vap *);
void	ieee80211_wme_updateparams_locked(struct ieee80211vap *);

int	ieee80211_open(struct net_device *);
int	ieee80211_init(struct net_device *, int force);
void	ieee80211_start_running(struct ieee80211com *);
int	ieee80211_stop(struct net_device *);
void	ieee80211_stop_running(struct ieee80211com *);
void	ieee80211_beacon_miss(struct ieee80211com *);
#ifdef ATH_SUPERG_DYNTURBO
void	ieee80211_dturbo_switch(struct ieee80211com *, int newflags);
#endif
#define	ieee80211_new_state(_vap, _state, _arg) \
	((_vap)->iv_newstate(_vap, _state, _arg))
void	ieee80211_print_essid(const u_int8_t *, int);
void	ieee80211_dump_pkt(struct ieee80211com *,
		const u_int8_t *, int, int, int);
struct sk_buff *ieee80211_getcfframe(struct ieee80211vap *vap, int type);

/*
 * Beacon frames constructed by ieee80211_beacon_alloc
 * have the following structure filled in so drivers
 * can update the frame later w/ minimal overhead.
 */
struct ieee80211_beacon_offsets {
	u_int16_t	*bo_caps;		/* capabilities */
	u_int8_t	*bo_tim;		/* start of atim/dtim */
	u_int8_t	*bo_wme;		/* start of WME parameters */
	u_int8_t	*bo_tim_trailer;	/* start of fixed-size tim trailer */
	u_int16_t	bo_tim_len;		/* atim/dtim length in bytes */
	u_int16_t	bo_tim_trailerlen;	/* trailer length in bytes */
	u_int8_t	*bo_chanswitch;		/* where channel switch IE will go */
	u_int8_t	*bo_ath_caps;		/* where ath caps is */
	u_int8_t	*bo_xr;			/* start of xr element */
	u_int8_t	*bo_erp;		/* start of ERP element */
    u_int8_t    *bo_appie_buf;  /* start of APP IE buf */
    u_int16_t   bo_appie_buf_len;
	u_int8_t	*bo_htinfo;		/* start of HT Info element */
	u_int16_t	bo_chanswitch_trailerlen;
};
struct sk_buff *ieee80211_beacon_alloc(struct ieee80211_node *,
		struct ieee80211_beacon_offsets *);
int	ieee80211_beacon_update(struct ieee80211_node *,
		struct ieee80211_beacon_offsets *, struct sk_buff *, int mcast);

/* XXX exposed 'cuz of beacon code botch */
int8_t	 *ieee80211_add_rates(u_int8_t *, const struct ieee80211_rateset *);
u_int8_t *ieee80211_add_xrates(u_int8_t *, const struct ieee80211_rateset *);
u_int8_t *ieee80211_add_wpa(u_int8_t *, struct ieee80211vap *);
u_int8_t *ieee80211_add_erp(u_int8_t *, struct ieee80211com *);
u_int8_t *ieee80211_add_athAdvCap(u_int8_t *, u_int8_t, u_int16_t);
u_int8_t *ieee80211_add_xr_param(u_int8_t *, struct ieee80211vap *);
u_int8_t *ieee80211_add_wme_param(u_int8_t *, struct ieee80211_wme_state *);
u_int8_t *ieee80211_add_country(u_int8_t *, struct ieee80211com *);
u_int8_t *ieee80211_add_country(u_int8_t *, struct ieee80211com *);
u_int8_t *ieee80211_add_athAdvCap(u_int8_t *, u_int8_t, u_int16_t);
u_int8_t *ieee80211_add_htcap(u_int8_t *, struct ieee80211_node *);
u_int8_t *ieee80211_add_htinfo(u_int8_t *, struct ieee80211_node *);

/*
 * Notification methods called from the 802.11 state machine.
 * Note that while these are defined here, their implementation
 * is OS-specific.
 */
void	ieee80211_notify_node_join(struct ieee80211_node *, int newassoc);
void	ieee80211_notify_node_leave(struct ieee80211_node *);
void	ieee80211_notify_scan_done(struct ieee80211vap *);
#endif /* _NET80211_IEEE80211_PROTO_H_ */
