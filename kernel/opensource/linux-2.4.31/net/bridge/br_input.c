/*
 *	Handle incoming frames
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: br_input.c,v 1.9.2.1 2001/12/24 04:50:05 davem Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_bridge.h>
#include <linux/netfilter_bridge.h>
#include "br_private.h"
#define BRIDGE_TAG 0x35564252	/* 5VBR */
#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
extern int brnf_filter_pre_routing_enable;
extern int brnf_filter_local_in_enable;
#endif

unsigned char bridge_ula[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

static int br_pass_frame_up_finish(struct sk_buff *skb)
{
#ifndef CONFIG_IP_NF_TURBONAT
#ifdef CONFIG_NETFILTER_DEBUG
	skb->nf_debug = 0;
#endif
#endif
	netif_rx(skb);

	return 0;
}

static void br_pass_frame_up(struct net_bridge *br, struct sk_buff *skb)
{
	struct net_device *indev;

//165064:henryhsu Move vlan code from Amazon to Danube
#if defined(CONFIG_IFX_NFEXT_VBRIDGE) || defined(CONFIG_IFX_NFEXT_VBRIDGE_MODULE)
	/* by MarsLin, for V-Bridge CUP Ingress, LOCAL_IN */
	if ((ifx_vbridge_hook != NULL) && (ifx_vbridge_hook(&skb, skb->dev, 1) < 0))
		return;
#endif
	br->statistics.rx_packets++;
	br->statistics.rx_bytes += skb->len;

	indev = skb->dev;
	skb->dev = &br->dev;
	skb->pkt_type = PACKET_HOST;
	skb_push(skb, ETH_HLEN);
	skb->protocol = eth_type_trans(skb, &br->dev);

#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
	if (!brnf_filter_local_in_enable)
		return br_pass_frame_up_finish(skb);
#endif
	NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_IN, skb, indev, NULL,
			br_pass_frame_up_finish);
}

#ifdef CONFIG_IFX_BR_OPT
int __bridge br_handle_frame_finish(struct sk_buff *skb)
#else
int br_handle_frame_finish(struct sk_buff *skb)
#endif
{
	struct net_bridge *br;
	unsigned char *dest;
	struct net_bridge_fdb_entry *dst;
	struct net_bridge_port *p;
	int passedup;

	dest = skb->mac.ethernet->h_dest;

	p = skb->dev->br_port;
	if (p == NULL)
		goto err_nolock;

	br = p->br;
	read_lock(&br->lock);
	if (skb->dev->br_port == NULL)
		goto err;

	passedup = 0;
	if (br->dev.flags & IFF_PROMISC) {
		struct sk_buff *skb2;

		skb2 = skb_clone(skb, GFP_ATOMIC);
		if (skb2 != NULL) {
			passedup = 1;
			br_pass_frame_up(br, skb2);
		}
	}

	if (dest[0] & 1) {
		br_flood_forward(br, skb, !passedup);
		if (!passedup)
			br_pass_frame_up(br, skb);
		goto out;
	}

	dst = br_fdb_get(br, dest);
	if (dst != NULL && dst->is_local) {
		if (!passedup)
			br_pass_frame_up(br, skb);
		else
			kfree_skb(skb);
		br_fdb_put(dst);
		goto out;
	}

	if (dst != NULL) {
		*(int*)(skb->cb)=BRIDGE_TAG;
		br_forward(dst->dst, skb);
		br_fdb_put(dst);
		goto out;
	}
	

	br_flood_forward(br, skb, 0);

out:
	read_unlock(&br->lock);
	return 0;

err:
	read_unlock(&br->lock);
err_nolock:
	kfree_skb(skb);
	return 0;
}

int br_handle_frame(struct sk_buff *skb)
{
	struct net_bridge *br;
	unsigned char *dest;
	struct net_bridge_port *p;

	dest = skb->mac.ethernet->h_dest;

	p = skb->dev->br_port;
	if (p == NULL)
		goto err_nolock;

	br = p->br;
	read_lock(&br->lock);
	if (skb->dev->br_port == NULL)
		goto err;

	if (!(br->dev.flags & IFF_UP) ||
	    p->state == BR_STATE_DISABLED)
		goto err;

	if (skb->mac.ethernet->h_source[0] & 1)
		goto err;

	if (p->state == BR_STATE_LEARNING ||
	    p->state == BR_STATE_FORWARDING)
		br_fdb_insert(br, p, skb->mac.ethernet->h_source, 0);

	if (br->stp_enabled &&
	    !memcmp(dest, bridge_ula, 5) &&
	    !(dest[5] & 0xF0))
		goto handle_special_frame;

	if (p->state == BR_STATE_FORWARDING) {

//060620:henryhsu Move vlan code from Amazon to Danube
#if defined(CONFIG_IFX_NFEXT_VBRIDGE) || defined(CONFIG_IFX_NFEXT_VBRIDGE_MODULE)
		/* by MarsLin, for V-Bridge Port Ingress, PRE_ROUTING */
		if ((ifx_vbridge_hook != NULL) && (ifx_vbridge_hook(&skb, skb->dev, 0) < 0))
		{	read_unlock(&br->lock);
			return -1;
		}
#endif

#if defined(CONFIG_BRIDGE_NF_EBTABLES) || defined(CONFIG_BRIDGE_NF_EBTABLES_MODULE)
		if (br_should_route_hook && br_should_route_hook(&skb)) {
			read_unlock(&br->lock);
			return -1;
		}

		if (!memcmp(p->br->dev.dev_addr, dest, ETH_ALEN))
			skb->pkt_type = PACKET_HOST;
#endif
#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
		if (!brnf_filter_pre_routing_enable)
			return br_handle_frame_finish(skb);
#endif
		NF_HOOK(PF_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			br_handle_frame_finish);
		read_unlock(&br->lock);
		return 0;
	}

err:
	read_unlock(&br->lock);
err_nolock:
	kfree_skb(skb);
	return 0;

handle_special_frame:
	if (!dest[5]) {
#if defined(CONFIG_IFX_NETFILTER_PROCFS) && defined(CONFIG_BRIDGE_NF_EBTABLES)
		if (!brnf_filter_local_in_enable)
			return br_stp_handle_bpdu(skb);
#endif
		NF_HOOK(PF_BRIDGE, NF_BR_LOCAL_IN, skb, skb->dev,NULL,
			br_stp_handle_bpdu);
		read_unlock(&br->lock);
		return 0;
	}

	read_unlock(&br->lock);
	kfree_skb(skb);
	return 0;
}
