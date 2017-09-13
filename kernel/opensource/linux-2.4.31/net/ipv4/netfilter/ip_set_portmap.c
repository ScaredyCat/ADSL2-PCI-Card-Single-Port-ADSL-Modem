/* Copyright 2004 Jozsef Kadlecsik (kadlec@blackhole.kfki.hu)
 *
 * Based on ip_set_ipmap.c
 *
 * This program is free software; you can redistribute it and/or modify   
 * it under the terms of the GNU General Public License as published by   
 * the Free Software Foundation; either version 2 of the License, or      
 * (at your option) any later version.                                    
 *                                                                         
 * This program is distributed in the hope that it will be useful,        
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
 * GNU General Public License for more details.                           
 *                                                                         
 * You should have received a copy of the GNU General Public License      
 * along with this program; if not, write to the Free Software            
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* Kernel module implementing a port set type as a bitmap */

#include <linux/module.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_set.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/softirq.h>
#include <linux/spinlock.h>

#include <net/ip.h>

#include <linux/netfilter_ipv4/ip_set_portmap.h>

static inline ip_set_ip_t
get_port(const struct sk_buff *skb, u_int32_t flags)
{
	struct iphdr *iph = skb->nh.iph;
	u_int16_t offset = ntohs(iph->frag_off) & IP_OFFSET;

	switch (iph->protocol) {
	case IPPROTO_TCP: {
		struct tcphdr *tcph = (struct tcphdr *)((u_int32_t *)iph + iph->ihl);
		
		/* See comments at tcp_match in ip_tables.c */
		if (offset != 0
		    || (offset == 0 
		    	&& (skb->len - iph->ihl * 4) < sizeof(struct tcphdr)))
			return INVALID_PORT;
	     	
	     	return ntohs(flags & IPSET_SRC ?
			     tcph->source : tcph->dest);
	    }
	case IPPROTO_UDP: {
		struct udphdr *udph = (struct udphdr *)((u_int32_t *)iph + iph->ihl);

		if (offset != 0
		    || (offset == 0 
		    	&& (skb->len - iph->ihl * 4) < sizeof(struct udphdr)))
			return INVALID_PORT;

	     	return ntohs(flags & IPSET_SRC ?
			     udph->source : udph->dest);
	    }
	default:
		return INVALID_PORT;
	}
}

static inline int
__testport(struct ip_set_private *private, ip_set_ip_t port, ip_set_ip_t *id)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;

	if (port < map->first_port || port > map->last_port)
		return -ERANGE;
		
	*id = port - map->first_port;
	return !!test_bit(*id, map->members);
}

static int
matchport(struct ip_set_private *private, ip_set_ip_t port, ip_set_ip_t *id)
{
	return __testport(private, port, id);
}

static int
testport(struct ip_set_private *private, const void *data, size_t size,
         ip_set_ip_t *id)
{
	struct ip_set_req_portmap *req = 
	    (struct ip_set_req_portmap *) data;

	if (size != sizeof(struct ip_set_req_portmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_portmap),
			      size);
		return -EINVAL;
	}
	return __testport(private, req->port, id);
}

static int
testport_kernel(struct ip_set_private *private, const struct sk_buff *skb,
		u_int32_t flags, ip_set_ip_t *id)
{
	ip_set_ip_t port;
	
	if (!(flags & IPSET_TYPE_PORT))
		return -EINVAL;

	port = get_port(skb, flags);
	DP("flags %u %s port %u",
		flags, 
		flags & IPSET_SRC ? "SRC" : "DST",
		port);
	
	if (port == INVALID_PORT)
		return -EINVAL;	

	return __testport(private, port, id);
}

static inline int
__addport(struct ip_set_private *private, ip_set_ip_t port, ip_set_ip_t *id)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;

	if (port < map->first_port || port > map->last_port)
		return -ERANGE;
	if (test_and_set_bit(port - map->first_port, map->members))
		return -EEXIST;
		
	*id = port - map->first_port;
	return 0;
}

static int
addport(struct ip_set_private *private, const void *data, size_t size,
        ip_set_ip_t *id)
{
	struct ip_set_req_portmap *req = 
	    (struct ip_set_req_portmap *) data;

	if (size != sizeof(struct ip_set_req_portmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_portmap),
			      size);
		return -EINVAL;
	}
	return __addport(private, req->port, id);
}

static int
addport_kernel(struct ip_set_private *private, const struct sk_buff *skb,
	       u_int32_t flags, ip_set_ip_t *id)
{
	ip_set_ip_t port;
	
	if (!(flags & IPSET_TYPE_PORT))
		return -EINVAL;
		  
	port = get_port(skb, flags);
	
	if (port == INVALID_PORT)
		return -EINVAL;

	return __addport(private, port, id);
}

static inline int
__delport(struct ip_set_private *private, ip_set_ip_t port, ip_set_ip_t *id)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;

	if (port < map->first_port || port > map->last_port)
		return -ERANGE;
	if (!test_and_clear_bit(port - map->first_port, map->members))
		return -EEXIST;
		
	*id = port - map->first_port;
	return 0;
}

static int
delport(struct ip_set_private *private, const void *data, size_t size,
        ip_set_ip_t *id)
{
	struct ip_set_req_portmap *req =
	    (struct ip_set_req_portmap *) data;

	if (size != sizeof(struct ip_set_req_portmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_portmap),
			      size);
		return -EINVAL;
	}
	return __delport(private, req->port, id);
}

static int
delport_kernel(struct ip_set_private *private, const struct sk_buff *skb,
	       u_int32_t flags, ip_set_ip_t *id)
{
	ip_set_ip_t port;
	
	if (!(flags & IPSET_TYPE_PORT))
		return -EINVAL;
		  
	port  = get_port(skb, flags);
	
	if (port == INVALID_PORT)
		return -EINVAL;

	return __delport(private, port, id);
}

static int create(struct ip_set_private **private, const void *data, size_t size)
{
	int newbytes;
	struct ip_set_req_portmap_create *req =
	    (struct ip_set_req_portmap_create *) data;
	struct ip_set_portmap *map;

	if (size != sizeof(struct ip_set_req_portmap_create)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			       sizeof(struct ip_set_req_portmap_create),
			       size);
		return -EINVAL;
	}

	DP("from 0x%08x to 0x%08x", req->from, req->to);

	if (req->from > req->to) {
		DP("bad port range");
		return -EINVAL;
	}

	if (req->to - req->from > MAX_RANGE) {
		ip_set_printk("range too big (max %d ports)",
			       MAX_RANGE);
		return -ERANGE;
	}

	map = kmalloc(sizeof(struct ip_set_portmap), GFP_KERNEL);
	if (!map) {
		DP("out of memory for %d bytes",
		   sizeof(struct ip_set_portmap));
		return -ENOMEM;
	}
	map->first_port = req->from;
	map->last_port = req->to;
	newbytes = bitmap_bytes(req->from, req->to);
	map->members = kmalloc(newbytes, GFP_KERNEL);
	if (!map->members) {
		DP("out of memory for %d bytes", newbytes);
		kfree(map);
		return -ENOMEM;
	}
	memset(map->members, 0, newbytes);

	*private = (struct ip_set_private *) map;
	return 0;
}

static void destroy(struct ip_set_private **private)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) *private;

	kfree(map->members);
	kfree(map);

	*private = NULL;
}

static void flush(struct ip_set_private *private)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;
	memset(map->members, 0, bitmap_bytes(map->first_port, map->last_port));
}

static int list_header_size(const struct ip_set_private *private)
{
	return sizeof(struct ip_set_req_portmap_create);
}

static void list_header(const struct ip_set_private *private, void *data)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;
	struct ip_set_req_portmap_create *header =
	    (struct ip_set_req_portmap_create *) data;

	DP("list_header %x %x", map->first_port, map->last_port);

	header->from = map->first_port;
	header->to = map->last_port;
}

static int list_members_size(const struct ip_set_private *private)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;

	return bitmap_bytes(map->first_port, map->last_port);
}

static void list_members(const struct ip_set_private *private, void *data)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;

	int bytes = bitmap_bytes(map->first_port, map->last_port);

	memcpy(data, map->members, bytes);
}

static ip_set_ip_t sizeid(const struct ip_set_private *private)
{
	struct ip_set_portmap *map = (struct ip_set_portmap *) private;

	return (map->last_port - map->first_port + 1);
}

static struct ip_set_type ip_set_portmap = {
	.typename		= SETTYPE_NAME,
	.typecode		= IPSET_TYPE_PORT,
	.protocol_version	= IP_SET_PROTOCOL_VERSION,
	.create			= &create,
	.destroy		= &destroy,
	.flush			= &flush,
	.reqsize		= sizeof(struct ip_set_req_portmap),
	.addip			= &addport,
	.addip_kernel		= &addport_kernel,
	.delip			= &delport,
	.delip_kernel		= &delport_kernel,
	.matchip		= &matchport,
	.testip			= &testport,
	.testip_kernel		= &testport_kernel,
	.list_header_size	= &list_header_size,
	.list_header		= &list_header,
	.list_members_size	= &list_members_size,
	.list_members		= &list_members,
	.sizeid			= &sizeid,
	.me			= THIS_MODULE,
};

static int __init init(void)
{
	return ip_set_register_set_type(&ip_set_portmap);
}

static void __exit fini(void)
{
	/* FIXME: possible race with ip_set_create() */
	ip_set_unregister_set_type(&ip_set_portmap);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
