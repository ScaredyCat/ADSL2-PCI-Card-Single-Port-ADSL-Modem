/*
 *	Forwarding decision
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: br_forward.c,v 1.4 2001/08/14 22:05:57 davem Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/skbuff.h>
#include <linux/if_bridge.h>
#include <linux/netfilter_bridge.h>
#include "br_private.h"

/* Santosh: For IGMP Proxy/Snooping */

#if defined(CONFIG_IFX_IGMP_PROXY) || defined(CONFIG_IFX_IGMP_PROXY_MODULE)
int (*mcast_hook_proxy_fwd)(struct sk_buff *, struct net_device *) = NULL;
#endif

#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
extern int brnf_filter_forward_enable;
extern int brnf_filter_local_out_enable;
extern int brnf_filter_post_routing_enable;
#endif
#if 0
#define DBPRINT printk
#else
#define DBPRINT(format, args...)
#endif

static inline int should_deliver(struct net_bridge_port *p, struct sk_buff *skb)
{
	if (skb->dev == p->dev ||
	    p->state != BR_STATE_FORWARDING)
		return 0;

#ifdef CONFIG_IFX_BR_WAN_ISOLATION 
        /*tc.chen : drop packet from interface nasX to nasX */
        if (skb->dev->name[0]=='n' && p->dev->name[0]=='n')
		return 0;
#endif
	return 1;
}

int br_dev_queue_push_xmit(struct sk_buff *skb)
{
#if defined(CONFIG_BRIDGE_NF_EBTABLES) || defined(CONFIG_BRIDGE_NF_EBTABLES_MODULE)
	nf_bridge_maybe_copy_header(skb);
#endif
	skb_push(skb, ETH_HLEN);

#ifdef CONFIG_IFX_BR_OPT  /* by MarsLin for performance tune while bridge mode!! */
        skb->dev->hard_start_xmit(skb, skb->dev);
#else
        dev_queue_xmit(skb);
#endif
	return 0;
}

#ifdef CONFIG_IFX_BR_OPT
int __bridge br_forward_finish(struct sk_buff *skb)
#else
int br_forward_finish(struct sk_buff *skb)
#endif
{
//060620:henryhsu Move vlan code from Amazon to Danube
#if defined(CONFIG_IFX_NFEXT_VBRIDGE) || defined(CONFIG_IFX_NFEXT_VBRIDGE_MODULE)
	/* by MarsLin, for V-Bridge Port Egress, POST_ROUTING */
	if ((ifx_vbridge_hook != NULL) && (ifx_vbridge_hook(&skb, skb->dev, 4) < 0))
		return -1;
#endif
#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
	if (!brnf_filter_post_routing_enable)
		return br_dev_queue_push_xmit(skb);
#endif
	NF_HOOK(PF_BRIDGE, NF_BR_POST_ROUTING, skb, NULL, skb->dev,
			br_dev_queue_push_xmit);

	return 0;
}

static void __br_deliver(struct net_bridge_port *to, struct sk_buff *skb)
{
	skb->dev = to->dev;
#ifndef CONFIG_IP_NF_TURBONAT
#ifdef CONFIG_NETFILTER_DEBUG
	//skb->nf_debug = 0;
#endif
#endif
#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
	if (!brnf_filter_local_out_enable)
		return br_forward_finish(skb);
#endif
	NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_OUT, skb, NULL, skb->dev,
			br_forward_finish);
}

#ifdef CONFIG_IFX_BR_OPT
static void __bridge __br_forward(struct net_bridge_port *to, struct sk_buff *skb)
#else
static void __br_forward(struct net_bridge_port *to, struct sk_buff *skb)
#endif
{
	struct net_device *indev;

	indev = skb->dev;
	skb->dev = to->dev;
	skb->ip_summed = CHECKSUM_NONE;

#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
	if (!brnf_filter_forward_enable)
		return br_forward_finish(skb);
#endif
	NF_HOOK(PF_BRIDGE, NF_BR_FORWARD, skb, indev, skb->dev,
			br_forward_finish);
}

/* called under bridge lock */
void br_deliver(struct net_bridge_port *to, struct sk_buff *skb)
{
	if (should_deliver(to, skb)) {
		__br_deliver(to, skb);
		return;
	}

	kfree_skb(skb);
}

/* called under bridge lock */
#ifdef CONFIG_IFX_BR_OPT
void __bridge br_forward(struct net_bridge_port *to, struct sk_buff *skb)
#else
void br_forward(struct net_bridge_port *to, struct sk_buff *skb)
#endif
{
	if (should_deliver(to, skb)) {
		__br_forward(to, skb);
		return;
	}

	kfree_skb(skb);
}

/* called under bridge lock */
static void br_flood(struct net_bridge *br, struct sk_buff *skb, int clone,
	void (*__packet_hook)(struct net_bridge_port *p, struct sk_buff *skb))
{
	struct net_bridge_port *p;
	struct net_bridge_port *prev;

	DBPRINT ("br_flood - enter, with skb from dev: %s dst: %u.%u.%u.%u \n", skb->dev->name, NIPQUAD(skb->nh.iph->daddr));

	if (clone) {
		struct sk_buff *skb2;

		if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
			br->statistics.tx_dropped++;
			return;
		}

		skb = skb2;
	}

	prev = NULL;

	p = br->port_list;
	while (p != NULL) {
		if (should_deliver(p, skb)) {
			if (prev != NULL) {
				struct sk_buff *skb2;

				if ((skb2 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
					br->statistics.tx_dropped++;
					kfree_skb(skb);
					return;
				}
		/* Santosh: hook for IGMP Proxy/Snooping */
#if defined(CONFIG_IFX_IGMP_PROXY) || defined(CONFIG_IFX_IGMP_PROXY_MODULE)
				if ((mcast_hook_proxy_fwd != NULL) && (skb2->mac.ethernet->h_dest[0] & 0x1)) {
					DBPRINT ("br_flood (1) - calling mcast_hook, with skb from dev: %s to dev: %s, dst: %u.%u.%u.%u\n", 
						skb2->dev->name ,prev->dev->name,  NIPQUAD(skb2->nh.iph->daddr));
				       if (mcast_hook_proxy_fwd(skb2,prev->dev) == 1) {
						__packet_hook(prev, skb2);
					} else {
						kfree_skb(skb2);
					}
				}else
				{
				    __packet_hook(prev, skb2); 
				}
#else
				    __packet_hook(prev, skb2); 
#endif //PROXY || SNOOPING
				
			}

			prev = p;
		}

		p = p->next;
	}

	if (prev != NULL) {
/* Santosh: hook for IGMP Proxy/Snooping */
#if defined(CONFIG_IFX_IGMP_PROXY) || defined(CONFIG_IFX_IGMP_PROXY_MODULE)
		if ((mcast_hook_proxy_fwd != NULL) && (skb->mac.ethernet->h_dest[0] & 0x1)) {
			DBPRINT ("br_flood (2) - calling mcast_hook, with skb from dev: %s to dev: %s, dst: %u.%u.%u.%u\n", 
				skb->dev->name ,prev->dev->name, NIPQUAD(skb->nh.iph->daddr));
	       if (mcast_hook_proxy_fwd(skb,prev->dev) == 1) {
				__packet_hook(prev, skb);
				return;
			} 
		   else {
				kfree_skb(skb);
				return;
			}
		}
#endif	

 		__packet_hook(prev, skb);
		return;
	}

	kfree_skb(skb);
}

/* called under bridge lock */
void br_flood_deliver(struct net_bridge *br, struct sk_buff *skb, int clone)
{
	br_flood(br, skb, clone, __br_deliver);
}

/* called under bridge lock */
void br_flood_forward(struct net_bridge *br, struct sk_buff *skb, int clone)
{
	br_flood(br, skb, clone, __br_forward);
}
