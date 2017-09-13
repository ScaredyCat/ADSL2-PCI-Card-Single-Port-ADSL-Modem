/*
 * RTSP extension for TCP NAT alteration
 * (C) 2003 by Tom Marshall <tmarshall@real.com>
 * based on ip_nat_irc.c
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Module load syntax:
 *      insmod ip_nat_rtsp.o ports=port1,port2,...port<MAX_PORTS>
 *                           stunaddr=<address>
 *                           destaction=[auto|strip|none]
 *
 * If no ports are specified, the default will be port 554 only.
 *
 * stunaddr specifies the address used to detect that a client is using STUN.
 * If this address is seen in the destination parameter, it is assumed that
 * the client has already punched a UDP hole in the firewall, so we don't
 * mangle the client_port.  If none is specified, it is autodetected.  It
 * only needs to be set if you have multiple levels of NAT.  It should be
 * set to the external address that the STUN clients detect.  Note that in
 * this case, it will not be possible for clients to use UDP with servers
 * between the NATs.
 *
 * If no destaction is specified, auto is used.
 *   destaction=auto:  strip destination parameter if it is not stunaddr.
 *   destaction=strip: always strip destination parameter (not recommended).
 *   destaction=none:  do not touch destination parameter (not recommended).
 */

#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/kernel.h>
#include <net/tcp.h>
#include <linux/netfilter_ipv4/ip_nat.h>
#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>
#include <linux/netfilter_ipv4/ip_conntrack_rtsp.h>


#include <linux/netfilter_ipv4/lockhelp.h>
DECLARE_RWLOCK_EXTERN(ip_rtsp_rwlock);

#define ASSERT_READ_LOCK MUST_BE_READ_LOCKED(&ip_rtsp_rwlock);
#define ASSERT_WRITE_LOCK MUST_BE_WRITE_LOCKED(&ip_rtsp_rwlock);


#include <linux/inet.h>
#include <linux/ctype.h>
#define NF_NEED_STRNCASECMP
#define NF_NEED_STRTOU16
#include <linux/netfilter_helpers.h>
#define NF_NEED_MIME_NEXTLINE
#include <linux/netfilter_mime.h>
#include <linux/unistd.h>
#define IFX_RTSP_NAT_ALG
#undef IFX_RTSP_NAT_ALG

#ifdef IFX_RTSP_NAT_ALG
#include <linux/netfilter_ipv4/listhelp.h>
//#include "rtsp_alg.h"
#include "ifx_alg.h"
#endif


#define INFOP(args...) printk(KERN_INFO __FILE__ ":", __FUNCTION__ ":" args)

#undef RTSP_NAT_DEBUG

#ifdef RTSP_NAT_DEBUG
#define NDEBUG(format,args...) printk(" \n Neeraj " ": " __FILE__  " : " __FUNCTION__ " : " format,##args)
#else
#define NDEBUG(format,args...)
#endif

//#define IP_NF_RTSP_DEBUG

#ifdef IP_NF_RTSP_DEBUG
//#define DEBUGP(args...) printk(KERN_DEBUG __FILE__ ":" __FUNCTION__ ":" args);
#define DEBUGP(args...) printk(args)
#else
#define DEBUGP(args...)
#endif



#define MAX_PORTS       8
#define DSTACT_AUTO     0
#define DSTACT_STRIP    1
#define DSTACT_NONE     2


#ifndef IFX_RTSP_NAT_ALG

static int      ports[MAX_PORTS];
static int       num_ports = 0;

#endif


static char *    stunaddr = NULL;
static char *    destaction = NULL;
static u_int32_t extip = 0;
static int       dstact = 0;


MODULE_AUTHOR("Tom Marshall <tmarshall@real.com>");
MODULE_DESCRIPTION("RTSP network address translation module");
MODULE_LICENSE("GPL");

#ifdef MODULE_PARM

#ifndef IFX_RTSP_NAT_ALG

MODULE_PARM(ports, "1-" __MODULE_STRING(MAX_PORTS) "i");
MODULE_PARM_DESC(ports, "port numbers of RTSP servers");
MODULE_PARM(stunaddr, "s");
MODULE_PARM_DESC(stunaddr, "Address for detecting STUN");
MODULE_PARM(destaction, "s");
MODULE_PARM_DESC(destaction, "Action for destination parameter (auto/strip/none)");

#endif

#endif

/* protects rtsp part of conntracks */
DECLARE_LOCK_EXTERN(ip_rtsp_lock);
// 6th july DECLARE_RWLOCK(ip_rtsp_rwlock);


#define SKIP_WSPACE(ptr,len,off) while(off < len && isspace(*(ptr+off))) { off++; }

#ifdef IFX_RTSP_NAT_ALG

#define BUF_LEN 80
static int Device_Open = 0;
static char Message[BUF_LEN];
static char *Message_Ptr;

struct list_head rtsp_registration_list;/* stores the registered port and protocols*/

/* initialize the list */
LIST_HEAD(rtsp_registration_list);

int rtsp_device_ioctl(struct inode *,struct file *,unsigned int ,unsigned long);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static int rtsp_device_release(struct inode *,struct file *);
#else
static void rtsp_device_release(struct inode *,struct file *);
#endif

static int rtsp_device_open(struct inode *,struct file *);

/* This structure will hold the functions to be called 
 * when a process does something to the device we 
 * created. Since a pointer to this structure is kept in 
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions. */
struct file_operations rtsp_Fops = {
llseek: NULL,   /* seek */
read: NULL, 
write: NULL,
readdir: NULL,   /* readdir */
poll: NULL,   /* select */
ioctl: rtsp_device_ioctl,   /* ioctl */
mmap: NULL,   /* mmap */
open: rtsp_device_open,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
flush: NULL,  /* flush */
#endif
release: rtsp_device_release  /* a.k.a. close */
};


#endif
/*** helper functions ***/

static void

get_skb_tcpdata(struct sk_buff* skb, char** pptcpdata, uint* ptcpdatalen)
{
	struct iphdr*   iph  = (struct iphdr*)skb->nh.iph;
	struct tcphdr*  tcph = (struct tcphdr*)((char*)iph + iph->ihl*4);

	*pptcpdata = (char*)tcph + tcph->doff*4;
	*ptcpdatalen = ((char*)skb->h.raw + skb->len) - *pptcpdata;
}

/*** nat functions ***/

/*
 * Mangle the "Transport:" header:
 *   - Replace all occurences of "client_port=<spec>"
 *   - Handle destination parameter
 *
 * In:
 *   ct, ctinfo = conntrack context
 *   pskb       = packet
 *   tranoff    = Transport header offset from TCP data
 *   tranlen    = Transport header length (incl. CRLF)
 *   rport_lo   = replacement low  port (host endian)
 *   rport_hi   = replacement high port (host endian)
 *
 * Returns packet size difference.
 *
 * Assumes that a complete transport header is present, ending with CR or LF
 */
static int
rtsp_mangle_tran(struct ip_conntrack* ct, enum ip_conntrack_info ctinfo,
		struct ip_conntrack_expect* exp,
		struct sk_buff** pskb, uint tranoff, uint tranlen)
{
	char*       ptcp;
	uint        tcplen;
	char*       ptran;
	char        rbuf1[16];      /* Replacement buffer (one port) */
	uint        rbuf1len;       /* Replacement len (one port) */
	char        rbufa[16];      /* Replacement buffer (all ports) */
	uint        rbufalen;       /* Replacement len (all ports) */
	u_int32_t   newip;
	u_int16_t   loport, hiport;
	uint        off = 0;
	uint        diff;           /* Number of bytes we removed */

	struct ip_ct_rtsp_expect* prtspexp = &exp->help.exp_rtsp_info;
	struct ip_conntrack_tuple t;

	char    szextaddr[15+1];
	uint    extaddrlen;
	int     is_stun;

	get_skb_tcpdata(*pskb, &ptcp, &tcplen);
	ptran = ptcp+tranoff;

	if (tranoff+tranlen > tcplen || tcplen-tranoff < tranlen ||
			tranlen < 10 || !iseol(ptran[tranlen-1]) ||
			nf_strncasecmp(ptran, "Transport:", 10) != 0)
	{
		INFOP("sanity check failed\n");
		return 0;
	}
	off += 10;
	SKIP_WSPACE(ptcp+tranoff, tranlen, off);

#ifdef CONFIG_IFX_ALG_QOS 
	if (exp->tuple.dst.u.udp.port == 0) {
		newip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
		t = exp->tuple;
		t.src.ip = newip;
	} else {
		newip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
		t = exp->tuple;
		t.dst.ip = newip;
	}
#else
	newip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
	t = exp->tuple;
	t.dst.ip = newip;
#endif

	extaddrlen = extip ? sprintf(szextaddr, "%u.%u.%u.%u", NIPQUAD(extip))
		: sprintf(szextaddr, "%u.%u.%u.%u", NIPQUAD(newip));
	DEBUGP("stunaddr=%s (%s)\n", szextaddr, (extip?"forced":"auto"));

	rbuf1len = rbufalen = 0;
	switch (prtspexp->pbtype)
	{
		case pb_single:
			for (loport = prtspexp->loport; loport != 0; loport++) /* XXX: improper wrap? */
			{
#ifdef CONFIG_IFX_ALG_QOS 
				if (exp->tuple.dst.u.udp.port == 0) {
					t.src.u.udp.port = htons(loport);
				} else {
					t.dst.u.udp.port = htons(loport);
				}
#else
				t.dst.u.udp.port = htons(loport);
#endif
				if (ip_conntrack_change_expect(exp, &t) == 0)
				{
					DEBUGP("using port %hu\n", loport);
					break;
				}
			}
			if (loport != 0)
			{
				rbuf1len = sprintf(rbuf1, "%hu", loport);
				rbufalen = sprintf(rbufa, "%hu", loport);
			}
			break;
		case pb_range:
			for (loport = prtspexp->loport; loport != 0; loport += 2) /* XXX: improper wrap? */
			{
				t.dst.u.udp.port = htons(loport);
				if (ip_conntrack_change_expect(exp, &t) == 0)
				{
					hiport = loport + ~exp->mask.dst.u.udp.port;
					DEBUGP("using ports %hu-%hu\n", loport, hiport);
					break;
				}
			}
			if (loport != 0)
			{
				rbuf1len = sprintf(rbuf1, "%hu", loport);
				rbufalen = sprintf(rbufa, "%hu-%hu", loport, loport+1);
			}
			break;
		case pb_discon:
			for (loport = prtspexp->loport; loport != 0; loport++) /* XXX: improper wrap? */
			{
				t.dst.u.udp.port = htons(loport);
				if (ip_conntrack_change_expect(exp, &t) == 0)
				{
					DEBUGP("using port %hu (1 of 2)\n", loport);
					break;
				}
			}
			for (hiport = prtspexp->hiport; hiport != 0; hiport++) /* XXX: improper wrap? */
			{
				t.dst.u.udp.port = htons(hiport);
				if (ip_conntrack_change_expect(exp, &t) == 0)
				{
					DEBUGP("using port %hu (2 of 2)\n", hiport);
					break;
				}
			}
			if (loport != 0 && hiport != 0)
			{
				rbuf1len = sprintf(rbuf1, "%hu", loport);
				if (hiport == loport+1)
				{
					rbufalen = sprintf(rbufa, "%hu-%hu", loport, hiport);
				}
				else
				{
					rbufalen = sprintf(rbufa, "%hu/%hu", loport, hiport);
				}
			}
			break;
		default:
			/* oops */
	}

	if (rbuf1len == 0)
	{
		return 0;   /* cannot get replacement port(s) */
	}

#ifdef CONFIG_IFX_ALG_QOS

	//Nirav : Check in case this is RTCP port i.e. odd port.
	if ((prtspexp->loport % 2) != 0) {
		DEBUGP("This is RTCP request to port %d\n",prtspexp->loport);
		return 0;
	} else 
		DEBUGP("This is NOT RTCP request to port %d\n",prtspexp->loport);
#endif
	/* Transport: tran;field;field=val,tran;field;field=val,... */
	while (off < tranlen)
	{
		uint        saveoff;
		const char* pparamend;
		uint        nextparamoff;

		pparamend = memchr(ptran+off, ',', tranlen-off);
		pparamend = (pparamend == NULL) ? ptran+tranlen : pparamend+1;
		nextparamoff = pparamend-ptcp;

		/*
		 * We pass over each param twice.  On the first pass, we look for a
		 * destination= field.  It is handled by the security policy.  If it
		 * is present, allowed, and equal to our external address, we assume
		 * that STUN is being used and we leave the client_port= field alone.
		 */
		is_stun = 0;
		saveoff = off;
		while (off < nextparamoff)
		{
			const char* pfieldend;
			uint        nextfieldoff;

			pfieldend = memchr(ptran+off, ';', nextparamoff-off);
			nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

			if (dstact != DSTACT_NONE && strncmp(ptran+off, "destination=", 12) == 0)
			{
				if (strncmp(ptran+off+12, szextaddr, extaddrlen) == 0)
				{
					is_stun = 1;
				}
				if (dstact == DSTACT_STRIP || (dstact == DSTACT_AUTO && !is_stun))
				{
					diff = nextfieldoff-off;
					if (!ip_nat_mangle_tcp_packet(pskb, ct, ctinfo,
								off, diff, NULL, 0))
					{
						/* mangle failed, all we can do is bail */
						return 0;
					}
					get_skb_tcpdata(*pskb, &ptcp, &tcplen);
					ptran = ptcp+tranoff;
					tranlen -= diff;
					nextparamoff -= diff;
					nextfieldoff -= diff;
				}
			}

			off = nextfieldoff;
		}
		if (is_stun)
		{
			continue;
		}
		off = saveoff;
		while (off < nextparamoff)
		{
			const char* pfieldend;
			uint        nextfieldoff;

			pfieldend = memchr(ptran+off, ';', nextparamoff-off);
			nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

			if (strncmp(ptran+off, "client_port=", 12) == 0)
			{
				u_int16_t   port;
				uint        numlen;
				uint        origoff;
				uint        origlen;
				char*       rbuf    = rbuf1;
				uint        rbuflen = rbuf1len;

				off += 12;
				origoff = (ptran-ptcp)+off;
				origlen = 0;
				numlen = nf_strtou16(ptran+off, &port);
				off += numlen;
				origlen += numlen;
				if (port != prtspexp->loport)
				{
					DEBUGP("multiple ports found, port %hu ignored\n", port);
				}
				else
				{
					if (ptran[off] == '-' || ptran[off] == '/')
					{
						off++;
						origlen++;
						numlen = nf_strtou16(ptran+off, &port);
						off += numlen;
						origlen += numlen;
						rbuf = rbufa;
						rbuflen = rbufalen;
					}

					/*
					 * note we cannot just memcpy() if the sizes are the same.
					 * the mangle function does skb resizing, checks for a
					 * cloned skb, and updates the checksums.
					 *
					 * parameter 4 below is offset from start of tcp data.
					 */
					diff = origlen-rbuflen;
					if (!ip_nat_mangle_tcp_packet(pskb, ct, ctinfo,
								origoff, origlen, rbuf, rbuflen))
					{
						/* mangle failed, all we can do is bail */
						return 0;
					}
					get_skb_tcpdata(*pskb, &ptcp, &tcplen);
					ptran = ptcp+tranoff;
					tranlen -= diff;
					nextparamoff -= diff;
					nextfieldoff -= diff;
				}
			}

			off = nextfieldoff;
		}

		off = nextparamoff;
	}

	return 1;
}

	static unsigned int
expected(struct sk_buff **pskb, uint hooknum, struct ip_conntrack* ct, struct ip_nat_info* info)
{
	struct ip_nat_multi_range mr;
	u_int32_t newdstip, newsrcip, newip;

	struct ip_conntrack *master = master_ct(ct);

	IP_NF_ASSERT(info);
	IP_NF_ASSERT(master);

	IP_NF_ASSERT(!(info->initialized & (1 << HOOK2MANIP(hooknum))));

#ifdef IFX_RTSP_NAT_ALG
	newdstip = master->tuplehash[exp->help.exp_rtsp_info.rtcp_dir].tuple.src.ip;
	newsrcip = ct->tuplehash[exp->help.exp_rtsp_info.rtcp_dir].tuple.src.ip;
	newip = (HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) ? newsrcip : newdstip;
#else
	newdstip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
	newsrcip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
	newip = (HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) ? newsrcip : newdstip;
#endif

	DEBUGP("newsrcip=%u.%u.%u.%u, newdstip=%u.%u.%u.%u, newip=%u.%u.%u.%u\n",
			NIPQUAD(newsrcip), NIPQUAD(newdstip), NIPQUAD(newip));

	mr.rangesize = 1;
	/* We don't want to manip the per-protocol, just the IPs. */
	mr.range[0].flags = IP_NAT_RANGE_MAP_IPS;
	mr.range[0].min_ip = mr.range[0].max_ip = newip;

	return ip_nat_setup_info(ct, &mr, hooknum);
}

static uint
help_out(struct ip_conntrack* ct, enum ip_conntrack_info ctinfo,
		struct ip_conntrack_expect* exp, struct sk_buff** pskb)
{
	char*   ptcp;
	uint    tcplen;
	uint    hdrsoff;
	uint    hdrslen;
	uint    lineoff;
	uint    linelen;
	uint    off;

	struct iphdr* iph = (struct iphdr*)(*pskb)->nh.iph;
	struct tcphdr* tcph = (struct tcphdr*)((void*)iph + iph->ihl*4);

	struct ip_ct_rtsp_expect* prtspexp = &exp->help.exp_rtsp_info;

	get_skb_tcpdata(*pskb, &ptcp, &tcplen);

	hdrsoff = exp->seq - ntohl(tcph->seq);
	hdrslen = prtspexp->len;
	off = hdrsoff;

	while (nf_mime_nextline(ptcp, hdrsoff+hdrslen, &off, &lineoff, &linelen))
	{
		if (linelen == 0)
		{
			break;
		}
		if (off > hdrsoff+hdrslen)
		{
			INFOP("!! overrun !!");
			break;
		}
		DEBUGP("hdr: len=%u, %.*s", linelen, (int)linelen, ptcp+lineoff);

		if (nf_strncasecmp(ptcp+lineoff, "Transport:", 10) == 0)
		{
			uint oldtcplen = tcplen;
			if (!rtsp_mangle_tran(ct, ctinfo, exp, pskb, lineoff, linelen))
			{
				break;
			}
			get_skb_tcpdata(*pskb, &ptcp, &tcplen);
			hdrslen -= (oldtcplen-tcplen);
			off -= (oldtcplen-tcplen);
			lineoff -= (oldtcplen-tcplen);
			linelen -= (oldtcplen-tcplen);
			DEBUGP("rep: len=%u, %.*s", linelen, (int)linelen, ptcp+lineoff);
		}
	}


	return NF_ACCEPT;
}

static uint
help_in(struct ip_conntrack* ct, enum ip_conntrack_info ctinfo,
		struct ip_conntrack_expect* exp, struct sk_buff** pskb)
{
	/* XXX: unmangle */
	return NF_ACCEPT;
}

static uint
help(struct ip_conntrack* ct,
		struct ip_conntrack_expect* exp,
		struct ip_nat_info* info,
		enum ip_conntrack_info ctinfo,
		unsigned int hooknum,
		struct sk_buff** pskb)
{
	struct iphdr*  iph  = (struct iphdr*)(*pskb)->nh.iph;
	struct tcphdr* tcph = (struct tcphdr*)((char*)iph + iph->ihl * 4);
	uint datalen;
	int dir;
	struct ip_ct_rtsp_expect* ct_rtsp_info;
	int rc = NF_ACCEPT;


	if (ct == NULL || exp == NULL || info == NULL || pskb == NULL)
	{
		DEBUGP("!! null ptr (%p,%p,%p,%p) !!\n", ct, exp, info, pskb);
		return NF_ACCEPT;
	}

	ct_rtsp_info = &exp->help.exp_rtsp_info;

	/*
	 * Only mangle things once: original direction in POST_ROUTING
	 * and reply direction on PRE_ROUTING.
	 */
	dir = CTINFO2DIR(ctinfo);
	if (!((hooknum == NF_IP_POST_ROUTING && dir == IP_CT_DIR_ORIGINAL)
				|| (hooknum == NF_IP_PRE_ROUTING && dir == IP_CT_DIR_REPLY)))
	{
		DEBUGP("Not touching dir %s at hook %s\n",
				dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
				hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
				: hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
				: hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "???");
		return NF_ACCEPT;
	}
	DEBUGP("got beyond not touching\n");

	datalen = (*pskb)->len - iph->ihl * 4 - tcph->doff * 4;

	LOCK_BH(&ip_rtsp_lock);
	/* Ensure the packet contains all of the marked data */
	if (!between(exp->seq + ct_rtsp_info->len,
				ntohl(tcph->seq), ntohl(tcph->seq) + datalen))
	{
		/* Partial retransmission?  Probably a hacker. */
		if (net_ratelimit())
		{
			INFOP("partial packet %u/%u in %u/%u\n",
					exp->seq, ct_rtsp_info->len, ntohl(tcph->seq), ntohl(tcph->seq) + datalen);
		}
		UNLOCK_BH(&ip_rtsp_lock);

		return NF_DROP;
	}

	switch (dir)
	{
		case IP_CT_DIR_ORIGINAL:
			rc = help_out(ct, ctinfo, exp, pskb);
			break;
		case IP_CT_DIR_REPLY:
			rc = help_in(ct, ctinfo, exp, pskb);
			break;
		default:
			/* oops */
	}
	UNLOCK_BH(&ip_rtsp_lock);


	return rc;
}

#ifndef IFX_RTSP_NAT_ALG
static struct ip_nat_helper ip_nat_rtsp_helpers[MAX_PORTS];
static char rtsp_names[MAX_PORTS][10];
#endif

/* This function is intentionally _NOT_ defined as  __exit */
	static void
fini(void)
{
#ifndef IFX_RTSP_NAT_ALG

	int i;

	for (i = 0; i < num_ports; i++)
	{
		DEBUGP("unregistering helper for port %d\n", ports[i]);
		ip_nat_helper_unregister(&ip_nat_rtsp_helpers[i]);
	}

#else

	struct list_head  *cur_item;
	struct list_head  *temp_item;
	struct rtsp_registration_data* cur_registration;
	int ret =0;

	list_for_each_safe(cur_item, temp_item, &rtsp_registration_list)
	{
		cur_registration = list_entry(cur_item, struct rtsp_registration_data, list);

		ip_nat_helper_unregister(cur_registration->helper);

		kfree(cur_registration->helper->name );

		kfree(cur_registration->helper);

		list_del(&(cur_registration->list));
		kfree(cur_registration);

		MOD_DEC_USE_COUNT;


	}

	ret = unregister_chrdev(MAJOR_NUM_RTSP, DEVICE_FILE_NAME_RTSP);
	/* If there's an error, report it */ 
	if (ret < 0)
		printk("Error in module_unregister_chrdev: %d\n", ret);

#endif

}

	static int __init
init(void)
{
	int ret = 0;

#ifndef IFX_RTSP_NAT_ALG
	int i;
	struct ip_nat_helper* hlpr;
	char* tmpname;
#endif

	printk("ip_nat_rtsp v" IP_NF_RTSP_VERSION " loading\n");

#ifndef IFX_RTSP_NAT_ALG

	if (ports[0] == 0)
	{
		ports[0] = RTSP_PORT;
	}

	for (i = 0; (i < MAX_PORTS) && ports[i] != 0; i++)
	{
		hlpr = &ip_nat_rtsp_helpers[i];
		memset(hlpr, 0, sizeof(struct ip_nat_helper));

		hlpr->tuple.dst.protonum = IPPROTO_TCP;
		hlpr->tuple.src.u.tcp.port = htons(ports[i]);
		hlpr->mask.src.u.tcp.port = 0xFFFF;
		hlpr->mask.dst.protonum = 0xFFFF;
		hlpr->help = help;
		hlpr->flags = 0;
		hlpr->me = THIS_MODULE;
		hlpr->expect = expected;

		tmpname = &rtsp_names[i][0];
		if (ports[i] == RTSP_PORT)
		{
			sprintf(tmpname, "rtsp");
		}
		else
		{
			sprintf(tmpname, "rtsp-%d", i);
		}
		hlpr->name = tmpname;

		DEBUGP("registering helper for port %d: name %s\n", ports[i], hlpr->name);
		ret = ip_nat_helper_register(hlpr);

		if (ret)
		{
			printk("ip_nat_rtsp: error registering helper for port %d\n", ports[i]);
			fini();
			return 1;
		}
		num_ports++;
	}


#else

	NDEBUG("Registering Rtsp_device\n");
	ret = register_chrdev(MAJOR_NUM_RTSP, DEVICE_FILE_NAME_RTSP, &rtsp_Fops);

	/* Negative values signify an error */
	if (ret < 0) 
	{
		printk (" %s failed with %d\n","Sorry, registering the character device ",ret);
	}
	else
	{
		printk(" SUCCESS %d MAJOR_NUM_RTSP ",MAJOR_NUM_RTSP);
	}


#endif

	if (stunaddr != NULL)
	{
		extip = in_aton(stunaddr);
	}
	if (destaction != NULL)
	{
		if (strcmp(destaction, "auto") == 0)
		{
			dstact = DSTACT_AUTO;
		}
		if (strcmp(destaction, "strip") == 0)
		{
			dstact = DSTACT_STRIP;
		}
		if (strcmp(destaction, "none") == 0)
		{
			dstact = DSTACT_NONE;
		}
	}
	return ret;
}




#ifdef IFX_RTSP_NAT_ALG

static int rtsp_device_port_register(struct rtsp_params* rtspparam)
{

	int port = RTSP_PORT;
	struct ip_nat_helper *hlpr;
	struct rtsp_registration_data *rtsp_reg_data;
	int ret = 0;
	char * tmp_name;


	hlpr = kmalloc(sizeof(struct ip_nat_helper), GFP_ATOMIC);
	if (hlpr == NULL)
	{	
		NDEBUG("RTSP: Memory not available\n");
		return -1;
	}

	rtsp_reg_data = kmalloc(sizeof(struct rtsp_registration_data), GFP_ATOMIC);
	if(rtsp_reg_data == NULL)
	{
		NDEBUG("RTSP : hlpr rtsp_reg_data Memory not available\n");

		return -1;
	}

	tmp_name = kmalloc(20, GFP_ATOMIC);
	if(tmp_name == NULL)
	{
		NDEBUG("RTSP : tmp_name memeory not available \n");

		return -1;
	}


	memset(hlpr, 0, sizeof(struct ip_nat_helper));
	memset(rtsp_reg_data,0,sizeof(struct rtsp_registration_data));
	memset(tmp_name,'\0',20);


	NDEBUG("\n INIT_LIST_HEAD try init list rtsp_reg_data is %p",rtsp_reg_data);
	/* Init list head of the rtsp_registration_data*/
	INIT_LIST_HEAD(&(rtsp_reg_data->list));


	port = rtspparam->rtsp_port;


	hlpr->tuple.src.u.tcp.port = htons(port);
	hlpr->tuple.dst.protonum = IPPROTO_TCP;
	hlpr->mask.src.u.tcp.port = 0xFFFF;

	hlpr->list.next =NULL;
	hlpr->list.prev =NULL;

	hlpr->mask.dst.protonum = 0xFFFF;
	hlpr->help = help;
	hlpr->flags = 0;
	hlpr->me = THIS_MODULE;
	hlpr->expect = expected;


	if (port == RTSP_PORT)
	{
		sprintf(tmp_name , "rtsp");
	}
	else
	{
		sprintf(tmp_name , "rtsp-%d",port );
	}

	hlpr->name = tmp_name;


	NDEBUG(" Trying to register helper register ....port #%d and name is %s \n", port,hlpr->name);

	ret = ip_nat_helper_register(hlpr);

	NDEBUG(" \n register helper register ...........return %d\n", ret);


	if (ret < 0)
	{
		printk("ip_nat_helper_register: ERROR registering port %d\n", port);
		fini();	

		return -EBUSY;
	}

	MOD_INC_USE_COUNT;


	rtsp_reg_data->helper = hlpr;
	rtsp_reg_data->proto = rtspparam->ip_proto;
	rtsp_reg_data->port = port;

	list_prepend(&(rtsp_registration_list), &(rtsp_reg_data->list));


	return 0;	   
}

static int  get_reg_data(const struct rtsp_registration_data* data, int port, enum AlgControlProtocol proto)
{
	if((data->port == port)&&(data->proto == proto))
	{
		return 1;
	}
	else
	{

		return 0;
	}

}


static int rtsp_device_port_deregister(struct rtsp_params* rtspparam)
{
	struct rtsp_registration_data* rtsp_reg_data;

	rtsp_reg_data = LIST_FIND(&rtsp_registration_list,
			get_reg_data,
			struct rtsp_registration_data *,
			rtspparam->rtsp_port, 
			rtspparam->ip_proto );

	NDEBUG("\n rtsp_reg_data is %p\n",rtsp_reg_data);

	if(rtsp_reg_data == NULL)
	{
		NDEBUG("RTSP: No registration for this port exists\n");
		return -1;
	}

	ip_nat_helper_unregister(rtsp_reg_data->helper);
	NDEBUG("RTSP: ip_nat_helper_unregister helper unregistered\n");

	NDEBUG("Trying to free rtsp_reg_data->helper->name");
	kfree(rtsp_reg_data->helper->name );

	NDEBUG(" Trying to free nat helper");
	kfree(rtsp_reg_data->helper);

	NDEBUG("RTSP: Freeing up rtsp_registration_data->list\n");
	list_del(&(rtsp_reg_data->list));

	NDEBUG(" Trying to free rtsp_reg_data");
	kfree(rtsp_reg_data);


	MOD_DEC_USE_COUNT;

	return 0;
}

/* This function is called whenever a process attempts 
 * to open the device file */
static int rtsp_device_open(struct inode *inode, 
		struct file *file)
{
#ifdef DEBUG
	printk ("device_open(%p)\n", file);
#endif

	/* We don't want to talk to two processes at the 
	 * same time */
	if (Device_Open)
	{
		return -EBUSY;
	}

	/* If this was a process, we would have had to be 
	 * more careful here, because one process might have 
	 * checked Device_Open right before the other one 
	 * tried to increment it. However, we're in the 
	 * kernel, so we're protected against context switches.
	 *
	 * This is NOT the right attitude to take, because we
	 * might be running on an SMP box, but we'll deal with
	 * SMP in a later chapter.
	 */ 

	Device_Open++;

	/* Initialize the message */
	Message_Ptr = Message;

	//  MOD_INC_USE_COUNT;

	return 0;
}

/* This function is called when a process closes the 
 * device file. It doesn't have a return value because 
 * it cannot fail. Regardless of what else happens, you 
 * should always be able to close a device (in 2.0, a 2.2
 * device file could be impossible to close).
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
static int rtsp_device_release(struct inode *inode, 
		struct file *file)
#else
static void rtsp_device_release(struct inode *inode, 
		struct file *file)
#endif
{
#ifdef DEBUG
	printk ("device_release(%p,%p)\n", inode, file);
#endif


	/* We're now ready for our next caller */
	Device_Open --;

	//  MOD_DEC_USE_COUNT;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)
	return 0;
#endif
}



int rtsp_device_ioctl(
		struct inode *inode,
		struct file *file,
		unsigned int ioctl_num,/* The number of the ioctl */
		unsigned long ioctl_param) /* The parameter to it */
{
	struct rtsp_params *rtspparam;
	int ret = 0;
	/* Switch according to the ioctl called */
	switch (ioctl_num) {
		//case IOCTL_SET_RTSP_MSG:
		//	break;

		case IOCTL_DEREGISTER_RTSP_PORT:
			NDEBUG("\n IOCTL_DEREGISTER_RTSP_PORT is called...........\n");
			rtspparam=(struct rtsp_params*)ioctl_param;
			NDEBUG("rtspparam.rtsp_port %d,rtspparam.ip_proto %d",rtspparam->rtsp_port,rtspparam->ip_proto);

			ret = rtsp_device_port_deregister(rtspparam);
			if (ret < 0 )
			{
				printk("\n ERROR : rtsp_device_port_deregister is failed \n");
				return -1;
			}
			break;
			//case IOCTL_GET_NTH_BYTE:
		case IOCTL_REGISTER_RTSP_PORT:
			NDEBUG("Received IOCTL_REGISTER_PORT\n");
			rtspparam = (struct rtsp_params*)ioctl_param;

			NDEBUG("Sip Port: [%d]\n",rtspparam->rtsp_port);
			if(rtspparam->ip_proto== IP_PROTO_TCP)
				NDEBUG("Sip Proto: TCP\n");
			else if(rtspparam->ip_proto== IP_PROTO_UDP)
				NDEBUG("Sip Proto: UDP\n");
			NDEBUG("rtspparam.rtsp_port %d,rtspparam.ip_proto %d",rtspparam->rtsp_port,rtspparam->ip_proto);

			ret = rtsp_device_port_register(rtspparam);
			if (ret < 0 )
			{
				printk("\n ERROR : rtsp_device_port_register is failed \n");
				return -1;
			}


			break;
	}

	return 0;
}

#endif


module_init(init);
module_exit(fini);

