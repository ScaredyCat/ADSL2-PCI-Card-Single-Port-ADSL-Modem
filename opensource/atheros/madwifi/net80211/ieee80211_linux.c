/*-
 * Copyright (c) 2003-2005 Sam Leffler, Errno Consulting
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
#define	EXPORT_SYMTAB
#endif

/*
 * IEEE 802.11 support (Linux-specific code)
 */
#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/sysctl.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>

#include <net/iw_handler.h>
#include <linux/wireless.h>
#include <linux/if_arp.h>		/* XXX for ARPHRD_* */

#include <asm/uaccess.h>

#include "if_media.h"
#include "if_ethersubr.h"

#include <net80211/ieee80211_var.h>

/*
 * Print a console message with the device name prepended.
 */
void
if_printf(struct net_device *dev, const char *fmt, ...)
{
	va_list ap;
	char buf[512];		/* XXX */

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	printk("%s: %s", dev->name, buf);
}

/*
 * Allocate and setup a management frame of the specified
 * size.  We return the sk_buff and a pointer to the start
 * of the contiguous data area that's been reserved based
 * on the packet length.  The data area is forced to 32-bit
 * alignment and the buffer length to a multiple of 4 bytes.
 * This is done mainly so beacon frames (that require this)
 * can use this interface too.
 */
struct sk_buff *
ieee80211_getmgtframe(u_int8_t **frm, u_int pktlen)
{
	const u_int align = sizeof(u_int32_t);
	struct ieee80211_cb *cb;
	struct sk_buff *skb;
	u_int len;

	len = roundup(sizeof(struct ieee80211_frame) + pktlen, 4);
	skb = dev_alloc_skb(len + align-1);
	if (skb != NULL) {
		u_int off = ((unsigned long) skb->data) % align;
		if (off != 0)
			skb_reserve(skb, align - off);

		cb = (struct ieee80211_cb *)skb->cb;
		cb->ni = NULL;
		cb->flags = 0;
		cb->next = NULL;

		skb_reserve(skb, sizeof(struct ieee80211_frame));
		*frm = skb_put(skb, pktlen);
	}
	return skb;
}

/*
 * Drain a queue of sk_buff's.
 */
void
__skb_queue_drain(struct sk_buff_head *q)
{
	struct sk_buff *skb;

	while ((skb = __skb_dequeue(q)) != NULL)
		dev_kfree_skb(skb);
}

#if IEEE80211_VLAN_TAG_USED
/*
 * VLAN support.
 */

/*
 * Register a vlan group.
 */
static void
ieee80211_vlan_register(struct net_device *dev, struct vlan_group *grp)
{
	struct ieee80211vap *vap = dev->priv;

	vap->iv_vlgrp = grp;
}

/*
 * Add an rx vlan identifier
 */
static void
ieee80211_vlan_add_vid(struct net_device *dev, unsigned short vid)
{
	struct ieee80211vap *vap = dev->priv;

	if (vap->iv_vlgrp != NULL)
		vap->iv_bss->ni_vlan = vid;
}

/*
 * Kill (i.e. delete) a vlan identifier.
 */
static void
ieee80211_vlan_kill_vid(struct net_device *dev, unsigned short vid)
{
	struct ieee80211vap *vap = dev->priv;

	if (vap->iv_vlgrp != NULL)
		vap->iv_vlgrp->vlan_devices[vid] = NULL;
}
#endif /* IEEE80211_VLAN_TAG_USED */

void
ieee80211_vlan_vattach(struct ieee80211vap *vap)
{
#if IEEE80211_VLAN_TAG_USED
	struct net_device *dev = vap->iv_dev;

	dev->features |= NETIF_F_HW_VLAN_TX | NETIF_F_HW_VLAN_RX |
			 NETIF_F_HW_VLAN_FILTER;
	dev->vlan_rx_register = ieee80211_vlan_register;
	dev->vlan_rx_add_vid = ieee80211_vlan_add_vid;
	dev->vlan_rx_kill_vid = ieee80211_vlan_kill_vid;
#endif /* IEEE80211_VLAN_TAG_USED */
}

void
ieee80211_vlan_vdetach(struct ieee80211vap *vap)
{
}

void
ieee80211_notify_node_join(struct ieee80211_node *ni, int newassoc)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct net_device *dev = vap->iv_dev;
	union iwreq_data wreq;

	if (ni == vap->iv_bss) { 
		if (newassoc) 
			netif_carrier_on(dev);
		memset(&wreq, 0, sizeof(wreq));
		IEEE80211_ADDR_COPY(wreq.addr.sa_data, ni->ni_bssid);
		wreq.addr.sa_family = ARPHRD_ETHER;
#ifdef ATH_SUPERG_XR
	if(vap->iv_flags & IEEE80211_F_XR) {
		dev = vap->iv_xrvap->iv_dev;
	}
#endif
		wireless_send_event(dev, SIOCGIWAP, &wreq, NULL);
	} else {
		memset(&wreq, 0, sizeof(wreq));
		IEEE80211_ADDR_COPY(wreq.addr.sa_data, ni->ni_macaddr);
		wreq.addr.sa_family = ARPHRD_ETHER;
#ifdef ATH_SUPERG_XR
	if(vap->iv_flags & IEEE80211_F_XR) {
		dev = vap->iv_xrvap->iv_dev;
	}
#endif
		wireless_send_event(dev, IWEVREGISTERED, &wreq, NULL);
	}
}

void
ieee80211_notify_node_leave(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct net_device *dev = vap->iv_dev;
	union iwreq_data wreq;

	if (ni == vap->iv_bss) {
		netif_carrier_off(dev);
		memset(wreq.ap_addr.sa_data, 0, ETHER_ADDR_LEN);
		wreq.ap_addr.sa_family = ARPHRD_ETHER;
		wireless_send_event(dev, SIOCGIWAP, &wreq, NULL);
	} else {
		/* fire off wireless event station leaving */
		memset(&wreq, 0, sizeof(wreq));
		IEEE80211_ADDR_COPY(wreq.addr.sa_data, ni->ni_macaddr);
		wreq.addr.sa_family = ARPHRD_ETHER;
		wireless_send_event(dev, IWEVEXPIRED, &wreq, NULL);
	}
}

void
ieee80211_notify_scan_done(struct ieee80211vap *vap)
{
	struct net_device *dev = vap->iv_dev;
	union iwreq_data wreq;

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s\n", "notify scan done");

	/* dispatch wireless event indicating scan completed */
	wreq.data.length = 0;
	wreq.data.flags = 0;
	wireless_send_event(dev, SIOCGIWSCAN, &wreq, NULL);
}

void
ieee80211_notify_replay_failure(struct ieee80211vap *vap,
	const struct ieee80211_frame *wh, const struct ieee80211_key *k,
	u_int64_t rsc)
{
	static const char * tag = "MLME-REPLAYFAILURE.indication";
	struct net_device *dev = vap->iv_dev;
	union iwreq_data wrqu;
	char buf[128];

	IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_CRYPTO, wh->i_addr2,
	    "%s replay detected <keyix %d, rsc %llu >",
	    k->wk_cipher->ic_name, k->wk_keyix, rsc );

	/* TODO: needed parameters: count, keyid, key type, src address, TSC */
	snprintf(buf, sizeof(buf), "%s(keyid=%d %scast addr=%s)", tag,
	    k->wk_keyix, IEEE80211_IS_MULTICAST(wh->i_addr1) ?  "broad" : "uni",
	    ether_sprintf(wh->i_addr1));
	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = strlen(buf);
	wireless_send_event(dev, IWEVCUSTOM, &wrqu, buf);
}
EXPORT_SYMBOL(ieee80211_notify_replay_failure);

void
ieee80211_notify_michael_failure(struct ieee80211vap *vap,
	const struct ieee80211_frame *wh, u_int keyix)
{
	static const char *tag = "MLME-MICHAELMICFAILURE.indication";
	struct net_device *dev = vap->iv_dev;
	union iwreq_data wrqu;
	char buf[128];

	IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_CRYPTO, wh->i_addr2,
	    "Michael MIC verification failed <keyix %d>", keyix);
	vap->iv_stats.is_rx_tkipmic++;

	/* TODO: needed parameters: count, keyid, key type, src address, TSC */
	snprintf(buf, sizeof(buf), "%s(keyid=%d %scast addr=%s)", tag,
	    keyix, IEEE80211_IS_MULTICAST(wh->i_addr2) ?  "broad" : "uni",
	    ether_sprintf(wh->i_addr2));
	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = strlen(buf);
	wireless_send_event(dev, IWEVCUSTOM, &wrqu, buf);
}
EXPORT_SYMBOL(ieee80211_notify_michael_failure);

void ieee80211_notify_push_button(struct ieee80211vap *vap)
{
    static const char *tag = "PUSH-BUTTON.indication";
    struct net_device *dev = vap->iv_dev;
    union iwreq_data wrqu;
    char buf[128];

    snprintf(buf, sizeof(buf), "%s", tag);
    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = strlen(buf);
    printk ("sending push button event to application!\n"); 
    wireless_send_event(dev, IWEVCUSTOM, &wrqu, buf);
}
EXPORT_SYMBOL(ieee80211_notify_push_button);


int
ieee80211_load_module(const char *modname)
{
	request_module(modname);
	return 0;
}

#ifdef CONFIG_SYSCTL

#ifdef IEEE80211_DEBUG
static int
IEEE80211_SYSCTL_DECL(ieee80211_sysctl_debug, ctl, write, filp, buffer,
		lenp, ppos)
{
	struct ieee80211vap *vap = ctl->extra1;
	u_int val;
	int ret;

	ctl->data = &val;
	ctl->maxlen = sizeof(val);
	if (write) {
		ret = IEEE80211_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
				lenp, ppos);
		if (ret == 0)
			vap->iv_debug = val;
	} else {
		val = vap->iv_debug;
		ret = IEEE80211_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
				lenp, ppos);
	}
	return ret;
}
#endif /* IEEE80211_DEBUG */

#define	CTL_AUTO	-2	/* cannot be CTL_ANY or CTL_NONE */

static const ctl_table ieee80211_sysctl_template[] = {
#ifdef IEEE80211_DEBUG
	{ .ctl_name	= CTL_AUTO,
	  .procname	= "debug",
	  .mode		= 0644,
	  .proc_handler	= ieee80211_sysctl_debug
	},
#endif
	/* NB: must be last entry before NULL */
	{ .ctl_name	= CTL_AUTO,
	  .procname	= "%parent",
	  .maxlen	= IFNAMSIZ,
	  .mode		= 0444,
	  .proc_handler	= proc_dostring
	},
	{ 0 }
};

void
ieee80211_sysctl_vattach(struct ieee80211vap *vap)
{
	int i, space;

	space = 5*sizeof(struct ctl_table) + sizeof(ieee80211_sysctl_template);
	vap->iv_sysctls = kmalloc(space, GFP_KERNEL);
	if (vap->iv_sysctls == NULL) {
		printk("%s: no memory for sysctl table!\n", __func__);
		return;
	}

	/* setup the table */
	memset(vap->iv_sysctls, 0, space);
	vap->iv_sysctls[0].ctl_name = CTL_NET;
	vap->iv_sysctls[0].procname = "net";
	vap->iv_sysctls[0].mode = 0555;
	vap->iv_sysctls[0].child = &vap->iv_sysctls[2];
	/* [1] is NULL terminator */
	vap->iv_sysctls[2].ctl_name = CTL_AUTO;
	vap->iv_sysctls[2].procname = vap->iv_dev->name;/* XXX bad idea? */
	vap->iv_sysctls[2].mode = 0555;
	vap->iv_sysctls[2].child = &vap->iv_sysctls[4];
	/* [3] is NULL terminator */
	/* copy in pre-defined data */
	memcpy(&vap->iv_sysctls[4], ieee80211_sysctl_template,
		sizeof(ieee80211_sysctl_template));

	/* add in dynamic data references */
	for (i = 4; vap->iv_sysctls[i].ctl_name; i++)
		if (vap->iv_sysctls[i].extra1 == NULL)
			vap->iv_sysctls[i].extra1 = vap;

	/* tack on back-pointer to parent device */
	vap->iv_sysctls[i-1].data = vap->iv_ic->ic_dev->name;	/* XXX? */

	/* and register everything */
	vap->iv_sysctl_header = register_sysctl_table(vap->iv_sysctls, 1);
	if (!vap->iv_sysctl_header) {
		printk("%s: failed to register sysctls!\n", vap->iv_dev->name);
		kfree(vap->iv_sysctls);
		vap->iv_sysctls = NULL;
	}
}

void
ieee80211_sysctl_vdetach(struct ieee80211vap *vap)
{
	if (vap->iv_sysctl_header) {
		unregister_sysctl_table(vap->iv_sysctl_header);
		vap->iv_sysctl_header = NULL;
	}
	if (vap->iv_sysctls) {
		kfree(vap->iv_sysctls);
		vap->iv_sysctls = NULL;
	}
}
#endif /* CONFIG_SYSCTL */

/*
 * Format an Ethernet MAC for printing.
 */
const char*
ether_sprintf(const u_int8_t *mac)
{
	static char etherbuf[18];
	snprintf(etherbuf, sizeof(etherbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return etherbuf;
}
EXPORT_SYMBOL(ether_sprintf);		/* XXX */

enum {
	DIDmsg_lnxind_wlansniffrm		= 0x00000044,
	DIDmsg_lnxind_wlansniffrm_hosttime	= 0x00010044,
	DIDmsg_lnxind_wlansniffrm_mactime	= 0x00020044,
	DIDmsg_lnxind_wlansniffrm_channel	= 0x00030044,
	DIDmsg_lnxind_wlansniffrm_rssi		= 0x00040044,
	DIDmsg_lnxind_wlansniffrm_sq		= 0x00050044,
	DIDmsg_lnxind_wlansniffrm_signal	= 0x00060044,
	DIDmsg_lnxind_wlansniffrm_noise		= 0x00070044,
	DIDmsg_lnxind_wlansniffrm_rate		= 0x00080044,
	DIDmsg_lnxind_wlansniffrm_istx		= 0x00090044,
	DIDmsg_lnxind_wlansniffrm_frmlen	= 0x000A0044
};
enum {
	P80211ENUM_msgitem_status_no_value	= 0x00
};
enum {
	P80211ENUM_truth_false			= 0x00
};

void
ieee80211_input_monitor(struct ieee80211com *ic, struct sk_buff *skb,
	u_int32_t mactime, u_int32_t rssi, u_int32_t signal, u_int32_t rate)
{
	static const wlan_ng_prism2_header template = {
		.msgcode	= DIDmsg_lnxind_wlansniffrm,
		.msglen		= sizeof(wlan_ng_prism2_header),

		.hosttime.did	= DIDmsg_lnxind_wlansniffrm_hosttime,
		.hosttime.len	= 4,

		.mactime.did	= DIDmsg_lnxind_wlansniffrm_mactime,
		.mactime.len	= 4,

		.istx.did	= DIDmsg_lnxind_wlansniffrm_istx,
		.istx.len	= 4,
		.istx.data	= P80211ENUM_truth_false,

		.frmlen.did	= DIDmsg_lnxind_wlansniffrm_frmlen,
		.frmlen.len	= 4,

		.channel.did	= DIDmsg_lnxind_wlansniffrm_channel,
		.channel.len	= 4,

		.rssi.did	= DIDmsg_lnxind_wlansniffrm_rssi,
		.rssi.status	= P80211ENUM_msgitem_status_no_value,
		.rssi.len	= 4,

		.signal.did	= DIDmsg_lnxind_wlansniffrm_signal,
		.signal.len	= 4,

		.rate.did	= DIDmsg_lnxind_wlansniffrm_rate,
		.rate.len	= 4,
	};
	struct ieee80211vap *vap, *next;
	wlan_ng_prism2_header *ph;

	ph = (wlan_ng_prism2_header *)
		skb_push(skb, sizeof(wlan_ng_prism2_header));
	*ph = template;

	ph->hosttime.data = jiffies;
	ph->mactime.data = mactime;
	ph->frmlen.data = skb->len - sizeof(wlan_ng_prism2_header);
	/* XXX no way to pass channel flag state */
	ph->channel.data = ieee80211_chan2ieee(ic, ic->ic_curchan);
	ph->rssi.data = rssi;
	ph->signal.data = signal;
	ph->rate.data = rate;

	/* XXX locking */
	for (vap = TAILQ_FIRST(&ic->ic_vaps); vap != NULL; vap = next) {
		struct sk_buff *skb1;
		struct net_device *dev;

		next = TAILQ_NEXT(vap, iv_next);
		if (vap->iv_opmode != IEEE80211_M_MONITOR ||
		    vap->iv_state != IEEE80211_S_RUN)
			continue;
		/* look ahead for another monitor mode vap */
		while (next != NULL && next->iv_opmode != IEEE80211_M_MONITOR)
			next = TAILQ_NEXT(next, iv_next);
		if (next != NULL) {
			skb1 = skb_copy(skb, GFP_ATOMIC);
			if (skb1 == NULL) {
				/* XXX stat+msg */
				continue;
			}
		} else {
			skb1 = skb;
			skb = NULL;
		}

		dev = vap->iv_dev;		/* NB: deliver to wlanX */

		ph = (wlan_ng_prism2_header *) skb1->data;
		strncpy(ph->devname, dev->name, sizeof(ph->devname));

		skb1->dev = dev;
		skb1->mac.raw = skb1->data;
		skb1->ip_summed = CHECKSUM_NONE;
		skb1->pkt_type = PACKET_OTHERHOST;
		skb1->protocol = __constant_htons(0x0019); /* ETH_P_80211_RAW */

		netif_rx(skb1);

		vap->iv_devstats.rx_packets++;
		vap->iv_devstats.rx_bytes += skb1->len;
	}
	if (skb != NULL)			/* no vaps, reclaim skb */
		dev_kfree_skb(skb);
}
EXPORT_SYMBOL(ieee80211_input_monitor);

/*
 * Module glue.
 */
#include "version.h"
static	char *version = WLAN_VERSION " (Atheros/multi-bss)";
static	char *dev_info = "wlan";

MODULE_AUTHOR("Errno Consulting, Sam Leffler");
MODULE_DESCRIPTION("802.11 wireless LAN protocol support");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif

extern	void ieee80211_auth_setup(void);

enum {
    DEV_ATH     = 9,            /* XXX known by hal */
};

u_int32_t ath_htvendorie_enable = 1;
u_int32_t ath_htoui             = 0x00904c;
u_int32_t ath_htcapid           = 51;
u_int32_t ath_htinfoid          = 52;
u_int32_t ath_htdupie_enable    = 0;

static ctl_table ath_wlan_sysctls[] = {
    { .ctl_name = CTL_AUTO,
      .procname = "htvendorieenable",
      .mode     = 0644,
      .data     = &ath_htvendorie_enable,
      .maxlen   = sizeof(ath_htvendorie_enable),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "htoui",
      .mode     = 0644,
      .data     = &ath_htoui,
      .maxlen   = sizeof(ath_htoui),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "htcapid",
      .mode     = 0644,
      .data     = &ath_htcapid,
      .maxlen   = sizeof(ath_htcapid),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "htinfoid",
      .mode     = 0644,
      .data     = &ath_htinfoid,
      .maxlen   = sizeof(ath_htinfoid),
      .proc_handler = proc_dointvec
    },
    { .ctl_name = CTL_AUTO,
      .procname = "htdupieenable",
      .mode     = 0644,
      .data     = &ath_htdupie_enable,
      .maxlen   = sizeof(ath_htdupie_enable),
      .proc_handler = proc_dointvec
    },
    { 0 }
};
static ctl_table ath_ath_table[] = {
    { .ctl_name = DEV_ATH,
      .procname = "ath",
      .mode     = 0555,
      .child    = ath_wlan_sysctls
    }, { 0 }
};
static ctl_table ath_root_table[] = {
    { .ctl_name = CTL_DEV,
      .procname = "dev",
      .mode     = 0555,
      .child    = ath_ath_table
    }, { 0 }
};
static struct ctl_table_header *ath_wlan_sysctl_header;

void
ath_wlan_sysctl_register(void)
{
    static int initialized = 0;

    if (!initialized) {
        ath_wlan_sysctl_header = register_sysctl_table(ath_root_table, 1);
        initialized = 1;
    }
}

static int __init
init_wlan(void)
{
	printk(KERN_INFO "%s: %s\n", dev_info, version);
	ath_wlan_sysctl_register();
	return 0;
}
module_init(init_wlan);

static void __exit
exit_wlan(void)
{
	printk(KERN_INFO "%s: driver unloaded\n", dev_info);
}
module_exit(exit_wlan);
