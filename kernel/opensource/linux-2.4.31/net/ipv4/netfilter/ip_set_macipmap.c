/* Copyright 2000-2004 Joakim Axelsson (gozem@linux.nu)
 *                     Patrick Schaaf (bof@bof.de)
 *                     Martin Josefsson (gandalf@wlug.westbo.se)
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

/* Kernel module implementing an IP set type: the macipmap type */

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
#include <linux/if_ether.h>
#include <linux/vmalloc.h>

#include <linux/netfilter_ipv4/ip_set_macipmap.h>

static int
matchip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	struct ip_set_macipmap *map = (struct ip_set_macipmap *) private;
	struct ip_set_macip *table =
	    (struct ip_set_macip *) map->members;
	
	if (ip < map->first_ip || ip > map->last_ip)
		return -ERANGE;

	*id = ip - map->first_ip;
	return !!test_bit(IPSET_MACIP_ISSET, &table[*id].flags);
}

static int
testip(struct ip_set_private *private, const void *data, size_t size,
       ip_set_ip_t *id)
{
	struct ip_set_macipmap *map = (struct ip_set_macipmap *) private;
	struct ip_set_macip *table =
	    (struct ip_set_macip *) map->members;
	
	struct ip_set_req_macipmap *req =
	    (struct ip_set_req_macipmap *) data;

	if (size != sizeof(struct ip_set_req_macipmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_macipmap),
			      size);
		return -EINVAL;
	}

	if (req->ip < map->first_ip || req->ip > map->last_ip)
		return -ERANGE;

	*id = req->ip - map->first_ip;
	if (test_bit(IPSET_MACIP_ISSET, &table[*id].flags)) {
		/* Is mac pointer valid?
		 * If so, compare... */
		return (memcmp(req->ethernet,
			       &table[*id].ethernet,
			       ETH_ALEN) == 0);
	} else {
		return (map->flags & IPSET_MACIP_MATCHUNSET ? 1 : 0);
	}
}

static int
testip_kernel(struct ip_set_private *private, const struct sk_buff *skb,
	      u_int32_t flags, ip_set_ip_t *id)
{
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) private;
	struct ip_set_macip *table =
	    (struct ip_set_macip *) map->members;
	ip_set_ip_t ip;
	
	if (!(flags & IPSET_TYPE_IP))
		return -EINVAL;
		  
	ip = ntohl(flags & IPSET_SRC ? skb->nh.iph->saddr
				     : skb->nh.iph->daddr);

	if (ip < map->first_ip || ip > map->last_ip)
		return -ERANGE;

	*id = ip - map->first_ip;
	if (test_bit(IPSET_MACIP_ISSET, &table[*id].flags)) {
		/* Is mac pointer valid?
		 * If so, compare... */
		return (skb->mac.raw >= skb->head
			&& (skb->mac.raw + ETH_HLEN) <= skb->data
			&& (memcmp(skb->mac.ethernet->h_source,
				   &table[*id].ethernet,
				   ETH_ALEN) == 0));
	} else {
		return (map->flags & IPSET_MACIP_MATCHUNSET ? 1 : 0);
	}
}

/* returns 0 on success */
static inline int
__addip(struct ip_set_private *private, 
	ip_set_ip_t ip, unsigned char *ethernet, ip_set_ip_t *id)
{
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) private;
	struct ip_set_macip *table =
	    (struct ip_set_macip *) map->members;

	if (ip < map->first_ip || ip > map->last_ip)
		return -ERANGE;
	if (test_and_set_bit(IPSET_MACIP_ISSET, 
			     &table[ip - map->first_ip].flags))
		return -EEXIST;

	*id = ip - map->first_ip;
	memcpy(&table[*id].ethernet, ethernet, ETH_ALEN);
	return 0;
}

static int
addip(struct ip_set_private *private, const void *data, size_t size,
      ip_set_ip_t *id)
{
	struct ip_set_req_macipmap *req =
	    (struct ip_set_req_macipmap *) data;

	if (size != sizeof(struct ip_set_req_macipmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_macipmap),
			      size);
		return -EINVAL;
	}
	return __addip(private, req->ip, req->ethernet, id);
}

static int
addip_kernel(struct ip_set_private *private, const struct sk_buff *skb,
	     u_int32_t flags, ip_set_ip_t *id)
{
	ip_set_ip_t ip;
	
	if (!(flags & IPSET_TYPE_IP))
		return -EINVAL;
		  
	ip = ntohl(flags & IPSET_SRC ? skb->nh.iph->saddr
				     : skb->nh.iph->daddr);

	if (!(skb->mac.raw >= skb->head
	      && (skb->mac.raw + ETH_HLEN) <= skb->data))
		return -EINVAL;

	return __addip(private, ip, skb->mac.ethernet->h_source, id);
}

static inline int
__delip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) private;
	struct ip_set_macip *table =
	    (struct ip_set_macip *) map->members;

	if (ip < map->first_ip || ip > map->last_ip)
		return -ERANGE;
	if (!test_and_clear_bit(IPSET_MACIP_ISSET, 
				&table[ip - map->first_ip].flags))
		return -EEXIST;

	*id = ip - map->first_ip;
	return 0;
}

static int
delip(struct ip_set_private *private, const void *data, size_t size,
     ip_set_ip_t *id)
{
	struct ip_set_req_macipmap *req =
	    (struct ip_set_req_macipmap *) data;

	if (size != sizeof(struct ip_set_req_macipmap)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_macipmap),
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
	struct ip_set_req_macipmap_create *req =
	    (struct ip_set_req_macipmap_create *) data;
	struct ip_set_macipmap *map;

	if (size != sizeof(struct ip_set_req_macipmap_create)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_macipmap_create),
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

	map = kmalloc(sizeof(struct ip_set_macipmap), GFP_KERNEL);
	if (!map) {
		DP("out of memory for %d bytes",
		   sizeof(struct ip_set_macipmap));
		return -ENOMEM;
	}
	map->flags = req->flags;
	map->first_ip = req->from;
	map->last_ip = req->to;
	newbytes = (req->to - req->from + 1) * sizeof(struct ip_set_macip);
	map->members = vmalloc(newbytes);
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
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) *private;

	vfree(map->members);
	kfree(map);

	*private = NULL;
}

static void flush(struct ip_set_private *private)
{
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) private;
	memset(map->members, 0, (map->last_ip - map->first_ip)
	       * sizeof(struct ip_set_macip));
}

static int list_header_size(const struct ip_set_private *private)
{
	return sizeof(struct ip_set_req_macipmap_create);
}

static void list_header(const struct ip_set_private *private, void *data)
{
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) private;
	struct ip_set_req_macipmap_create *header =
	    (struct ip_set_req_macipmap_create *) data;

	DP("list_header %x %x %u", map->first_ip, map->last_ip,
	   map->flags);

	header->from = map->first_ip;
	header->to = map->last_ip;
	header->flags = map->flags;
}

static int list_members_size(const struct ip_set_private *private)
{
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) private;

	return (map->last_ip
		- map->first_ip + 1) * sizeof(struct ip_set_macip);
}

static void list_members(const struct ip_set_private *private, void *data)
{
	struct ip_set_macipmap *map =
	    (struct ip_set_macipmap *) private;

	int bytes = (map->last_ip - 
		     - map->first_ip + 1) * sizeof(struct ip_set_macip);

	memcpy(data, map->members, bytes);
}

static ip_set_ip_t sizeid(const struct ip_set_private *private)
{
	struct ip_set_macipmap *map = (struct ip_set_macipmap *) private;

	return (map->last_ip - map->first_ip + 1);
}

static struct ip_set_type ip_set_macipmap = {
	.typename		= SETTYPE_NAME,
	.typecode		= IPSET_TYPE_IP,
	.protocol_version	= IP_SET_PROTOCOL_VERSION,
	.create			= &create,
	.destroy		= &destroy,
	.flush			= &flush,
	.reqsize		= sizeof(struct ip_set_req_macipmap),
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
	return ip_set_register_set_type(&ip_set_macipmap);
}

static void __exit fini(void)
{
	/* FIXME: possible race with ip_set_create() */
	ip_set_unregister_set_type(&ip_set_macipmap);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
