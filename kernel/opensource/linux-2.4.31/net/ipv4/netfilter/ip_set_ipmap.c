/* Copyright 2000-2004 Joakim Axelsson (gozem@linux.nu)
 *                     Patrick Schaaf (bof@bof.de)
 *                     Jozsef Kadlecsik (kadlec@blackhole.kfki.hu)
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

/* Kernel module implementing an IP set type: the single bitmap type */

#include <linux/module.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_set.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/softirq.h>
#include <linux/spinlock.h>

#include <linux/netfilter_ipv4/ip_set_ipmap.h>

static inline ip_set_ip_t
ip_to_id(const struct ip_set_ipmap *map, ip_set_ip_t ip)
{
	return ((ip & map->netmask) - map->first_ip)/map->hosts;
}

static inline int
__testip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;
	
	if (ip < map->first_ip || ip > map->last_ip)
		return -ERANGE;

	*id = ip_to_id(map, ip);
	return !!test_bit(*id, map->members);
}

static int
matchip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	return __testip(private, ip, id);
}

static int
testip(struct ip_set_private *private, const void *data, size_t size,
       ip_set_ip_t *id)
{
	struct ip_set_req_ipmap *req = 
	    (struct ip_set_req_ipmap *) data;

	if (size != sizeof(struct ip_set_req_ipmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_ipmap),
			      size);
		return -EINVAL;
	}
	return __testip(private, req->ip, id);
}

static int
testip_kernel(struct ip_set_private *private, 
	      const struct sk_buff *skb,
	      u_int32_t flags,
	      ip_set_ip_t *id)
{
	if (!(flags & IPSET_TYPE_IP))
		return -EINVAL;
	
	DP("flags: %u (%s) ip %u.%u.%u.%u", flags,
	   flags & IPSET_SRC ? "SRC" : "DST",
	   NIPQUAD(skb->nh.iph->saddr));
	DP("flags: %u (%s) ip %u.%u.%u.%u", flags, 	  
	   flags & IPSET_SRC ? "SRC" : "DST",
	   NIPQUAD(skb->nh.iph->daddr));

	return __testip(private,
			ntohl(flags & IPSET_SRC ? skb->nh.iph->saddr 
						: skb->nh.iph->daddr),
			id);
}

static inline int
__addip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;

	if (ip < map->first_ip || ip > map->last_ip)
		return -ERANGE;

	*id = ip_to_id(map, ip);
	if (test_and_set_bit(*id, map->members))
		return -EEXIST;

	return 0;
}

static int
addip(struct ip_set_private *private, const void *data, size_t size,
      ip_set_ip_t *id)
{
	struct ip_set_req_ipmap *req = 
	    (struct ip_set_req_ipmap *) data;

	if (size != sizeof(struct ip_set_req_ipmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_ipmap),
			      size);
		return -EINVAL;
	}
	DP("%u.%u.%u.%u", NIPQUAD(req->ip));
	return __addip(private, req->ip, id);
}

static int
addip_kernel(struct ip_set_private *private, const struct sk_buff *skb,
	     u_int32_t flags, ip_set_ip_t *id)
{
	if (!(flags & IPSET_TYPE_IP))
		return -EINVAL;
		  
	return __addip(private,
		       ntohl(flags & IPSET_SRC ? skb->nh.iph->saddr 
					       : skb->nh.iph->daddr),
		       id);
}

static inline int 
__delip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;

	if (ip < map->first_ip || ip > map->last_ip)
		return -ERANGE;

	*id = ip_to_id(map, ip);
	if (!test_and_clear_bit(*id, map->members))
		return -EEXIST;
	
	return 0;
}

static int
delip(struct ip_set_private *private, const void *data, size_t size,
      ip_set_ip_t *id)
{
	struct ip_set_req_ipmap *req =
	    (struct ip_set_req_ipmap *) data;

	if (size != sizeof(struct ip_set_req_ipmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_ipmap),
			      size);
		return -EINVAL;
	}
	return __delip(private, req->ip, id);
}

static int
delip_kernel(struct ip_set_private *private, const struct sk_buff *skb,
	     u_int32_t flags, ip_set_ip_t *id)
{
	if (!(flags & IPSET_TYPE_IP))
		return -EINVAL;
		  
	return __delip(private,
		       ntohl(flags & IPSET_SRC ? skb->nh.iph->saddr 
					       : skb->nh.iph->daddr),
		       id);
}

static int create(struct ip_set_private **private, const void *data, size_t size)
{
	int newbytes;
	struct ip_set_req_ipmap_create *req =
	    (struct ip_set_req_ipmap_create *) data;
	struct ip_set_ipmap *map;

	if (size != sizeof(struct ip_set_req_ipmap_create)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_ipmap_create),
			      size);
		return -EINVAL;
	}

	DP("from 0x%08x to 0x%08x", req->from, req->to);

	if (req->from > req->to) {
		DP("bad ip range");
		return -EINVAL;
	}

	if (req->to - req->from > MAX_RANGE) {
		ip_set_printk("range too big (max %d addresses)",
			       MAX_RANGE);
		return -ERANGE;
	}

	map = kmalloc(sizeof(struct ip_set_ipmap), GFP_KERNEL);
	if (!map) {
		DP("out of memory for %d bytes",
		   sizeof(struct ip_set_ipmap));
		return -ENOMEM;
	}
	map->first_ip = req->from;
	map->last_ip = req->to;
	map->netmask = req->netmask;

	if (req->netmask == 0xFFFFFFFF) {
		map->hosts = 1;
		map->sizeid = map->last_ip - map->first_ip + 1;
	} else {
		unsigned int mask_bits, netmask_bits;
		ip_set_ip_t mask;
		
		map->first_ip &= map->netmask;	/* Should we better bark? */
		
		mask = range_to_mask(map->first_ip, map->last_ip, &mask_bits);
		netmask_bits = mask_to_bits(map->netmask);
		
		if (!mask || netmask_bits <= mask_bits)
			return -EINVAL;

		map->hosts = 2 << (32 - netmask_bits - 1);
		map->sizeid = 2 << (netmask_bits - mask_bits - 1);
	}
	newbytes = bitmap_bytes(0, map->sizeid - 1);
	DP("%x %x %i", map->first_ip, map->last_ip, newbytes);
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
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) *private;
	
	kfree(map->members);
	kfree(map);
	
	*private = NULL;
}

static void flush(struct ip_set_private *private)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;
	memset(map->members, 0, bitmap_bytes(0, map->sizeid - 1));
}

static int list_header_size(const struct ip_set_private *private)
{
	return sizeof(struct ip_set_req_ipmap_create);
}

static void list_header(const struct ip_set_private *private, void *data)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;
	struct ip_set_req_ipmap_create *header =
	    (struct ip_set_req_ipmap_create *) data;

	DP("list_header %x %x", map->first_ip, map->last_ip);

	header->from = map->first_ip;
	header->to = map->last_ip;
	header->netmask = map->netmask;
}

static int list_members_size(const struct ip_set_private *private)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;

	return bitmap_bytes(0, map->sizeid - 1);
}

static void list_members(const struct ip_set_private *private, void *data)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;

	int bytes = bitmap_bytes(0, map->sizeid - 1);

	memcpy(data, map->members, bytes);
}

static ip_set_ip_t sizeid(const struct ip_set_private *private)
{
	struct ip_set_ipmap *map = (struct ip_set_ipmap *) private;

	return (map->sizeid);
}

static struct ip_set_type ip_set_ipmap = {
	.typename		= SETTYPE_NAME,
	.typecode		= IPSET_TYPE_IP,
	.protocol_version	= IP_SET_PROTOCOL_VERSION,
	.create			= &create,
	.destroy		= &destroy,
	.flush			= &flush,
	.reqsize		= sizeof(struct ip_set_req_ipmap),
	.addip			= &addip,
	.addip_kernel		= &addip_kernel,
	.delip			= &delip,
	.delip_kernel		= &delip_kernel,
	.matchip		= &matchip,
	.testip			= &testip,
	.testip_kernel		= &testip_kernel,
	.list_header_size	= &list_header_size,
	.list_header		= &list_header,
	.list_members_size	= &list_members_size,
	.list_members		= &list_members,
	.sizeid			= &sizeid,
	.me			= THIS_MODULE,
};

static int __init init(void)
{
	return ip_set_register_set_type(&ip_set_ipmap);
}

static void __exit fini(void)
{
	/* FIXME: possible race with ip_set_create() */
	ip_set_unregister_set_type(&ip_set_ipmap);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
