/* Copyright 2004 Jozsef Kadlecsik (kadlec@blackhole.kfki.hu)
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

/* Kernel module implementing an ip hash set */

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
#include <linux/vmalloc.h>

#include <net/ip.h>

#include <linux/netfilter_ipv4/ip_set_iphash.h>
#include <linux/netfilter_ipv4/ip_set_jhash.h>

static inline ip_set_ip_t
hash_ip(const struct ip_set_iphash *map, ip_set_ip_t ip)
{
	return (jhash_1word(ip & map->netmask, map->initval) % map->hashsize);
}

static inline int
__testip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;

	*id = hash_ip(map, ip);
	return (map->members[*id] == ip);
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
	struct ip_set_req_iphash *req = 
	    (struct ip_set_req_iphash *) data;

	if (size != sizeof(struct ip_set_req_iphash)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_iphash),
			      size);
		return -EINVAL;
	}
	return __testip(private, req->ip, id);
}

static int
testip_kernel(struct ip_set_private *private, const struct sk_buff *skb,
		u_int32_t flags, ip_set_ip_t *id)
{
	if (!(flags & IPSET_TYPE_IP))
		return -EINVAL;
		  
	return __testip(private,
			ntohl(flags & IPSET_SRC ? skb->nh.iph->saddr 
						: skb->nh.iph->daddr),
			id);
}

static inline int
__addip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id,
        u_int32_t flags)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;

	*id = hash_ip(map, ip);

	if (map->members[*id] == ip)
		return -EEXIST;

	if (map->members[*id] != 0 && !(flags & IPSET_ADD_OVERWRITE))
		return -EADDRINUSE;

	map->members[*id] = ip;
	return 0;
}

static int
addip(struct ip_set_private *private, const void *data, size_t size,
        ip_set_ip_t *id)
{
	struct ip_set_req_iphash *req = 
	    (struct ip_set_req_iphash *) data;

	if (size != sizeof(struct ip_set_req_iphash)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_iphash),
			      size);
		return -EINVAL;
	}
	return __addip(private, req->ip, id, req->flags);
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
		       id,
		       flags);
}

static inline int
__delip(struct ip_set_private *private, ip_set_ip_t ip, ip_set_ip_t *id)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;

	*id = hash_ip(map, ip);

	if (map->members[*id] == 0)
		return -EEXIST;

	if (map->members[*id] != ip)
		return -EADDRINUSE;

	map->members[*id] = 0;
	return 0;
}

static int
delip(struct ip_set_private *private, const void *data, size_t size,
        ip_set_ip_t *id)
{
	struct ip_set_req_iphash *req =
	    (struct ip_set_req_iphash *) data;

	if (size != sizeof(struct ip_set_req_iphash)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			      sizeof(struct ip_set_req_iphash),
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
	struct ip_set_req_iphash_create *req =
	    (struct ip_set_req_iphash_create *) data;
	struct ip_set_iphash *map;

	if (size != sizeof(struct ip_set_req_iphash_create)) {
		ip_set_printk("data length wrong (want %d, have %d)",
			       sizeof(struct ip_set_req_iphash_create),
			       size);
		return -EINVAL;
	}

	if (req->hashsize > MAX_RANGE) {
		ip_set_printk("hashsize too big (max %d)",
			       MAX_RANGE);
		return -ERANGE;
	}

	if (req->hashsize < 1) {
		ip_set_printk("hashsize too small");
		return -ERANGE;
	}

	map = kmalloc(sizeof(struct ip_set_iphash), GFP_KERNEL);
	if (!map) {
		DP("out of memory for %d bytes",
		   sizeof(struct ip_set_iphash));
		return -ENOMEM;
	}
	map->initval = req->initval;
	map->hashsize = req->hashsize;
	map->netmask = req->netmask;
	newbytes = map->hashsize * sizeof(ip_set_ip_t);
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
	struct ip_set_iphash *map = (struct ip_set_iphash *) *private;

	vfree(map->members);
	kfree(map);

	*private = NULL;
}

static void flush(struct ip_set_private *private)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;
	memset(map->members, 0, map->hashsize * sizeof(ip_set_ip_t));
}

static int list_header_size(const struct ip_set_private *private)
{
	return sizeof(struct ip_set_req_iphash_create);
}

static void list_header(const struct ip_set_private *private, void *data)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;
	struct ip_set_req_iphash_create *header =
	    (struct ip_set_req_iphash_create *) data;

	header->initval = map->initval;
	header->hashsize = map->hashsize;
	header->netmask = map->netmask;
}

static int list_members_size(const struct ip_set_private *private)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;

	return (map->hashsize * sizeof(ip_set_ip_t));
}

static void list_members(const struct ip_set_private *private, void *data)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;

	int bytes = map->hashsize * sizeof(ip_set_ip_t);

	memcpy(data, map->members, bytes);
}

static ip_set_ip_t sizeid(const struct ip_set_private *private)
{
	struct ip_set_iphash *map = (struct ip_set_iphash *) private;

	return (map->hashsize);
}

static struct ip_set_type ip_set_iphash = {
	.typename		= SETTYPE_NAME,
	.typecode		= IPSET_TYPE_IP,
	.protocol_version	= IP_SET_PROTOCOL_VERSION,
	.create			= &create,
	.destroy		= &destroy,
	.flush			= &flush,
	.reqsize		= sizeof(struct ip_set_req_iphash),
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
	return ip_set_register_set_type(&ip_set_iphash);
}

static void __exit fini(void)
{
	/* FIXME: possible race with ip_set_create() */
	ip_set_unregister_set_type(&ip_set_iphash);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
