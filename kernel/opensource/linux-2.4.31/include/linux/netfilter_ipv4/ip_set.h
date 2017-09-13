#ifndef _IP_SET_H
#define _IP_SET_H

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

/*
 * A sockopt of such quality has hardly ever been seen before on the open
 * market!  This little beauty, hardly ever used: above 64, so it's
 * traditionally used for firewalling, not touched (even once!) by the
 * 2.0, 2.2 and 2.4 kernels!
 *
 * Comes with its own certificate of authenticity, valid anywhere in the
 * Free world!
 *
 * Rusty, 19.4.2000
 */
#define SO_IP_SET 		83

/* Directions: */
#define IPSET_SRC 		0x01
#define IPSET_DST		0x02
/* Inverse flag for matching: */
#define IPSET_MATCH_INV		0x04
/* Overwrite at adding a new entry: */
#define IPSET_ADD_OVERWRITE	0x08
/* Set typecodes: */
#define IPSET_TYPE_IP		0x10
#define IPSET_TYPE_PORT		0x20

/*FIXME: remove this */
/* #define CONFIG_IP_NF_SET_DEBUG */

/*
 * Heavily modify by Joakim Axelsson 08.03.2002
 * - Made it more modulebased
 *
 * Additional heavy modifications by Jozsef Kadlecsik 22.02.2004
 * - multilevel pools (sets)
 * - in order to "deal with" backward compatibility, renamed to ipset
 */

/* Used so that the kernel module and ipset-binary can match thier versions 
 */
#define IP_SET_PROTOCOL_VERSION 1

#define IP_SET_MAXNAMELEN 32	/* set names and set typenames */

/* The max level of the sets. 
 * Do not increase lightheartedly before eliminating
 * the recursive functions from ip_set.c.
 */
/* So many IPs can identify a set: */
#define IP_SET_SETIP_LEVELS	4
/* Max level of a set: */
#define IP_SET_LEVELS		(IP_SET_SETIP_LEVELS+1)

/* Lets work with our own typedef for representing an IP address.
 * We hope to make the code more portable, possibly to IPv6...
 *
 * The representation works in HOST byte order, because most set types
 * will perform arithmetic operations and compare operations.
 * 
 * For now the type is an uint32_t.
 *
 * We do not enforce, but assume that a set may not store more than 
 * 65536 entries.
 *
 * Make sure to ONLY use the functions when translating and parsing
 * in order to keep the host byte order and make it more portable:
 *  parse_ip()
 *  parse_mask()
 *  parse_ipandmask()
 *  ip_tostring()
 * (Joakim: where are they???)
 */

typedef uint32_t ip_set_ip_t;

/* SO_IP_SET operation constants, and their request struct types.
 */

/* IP_SET_REQ_BASE defines the first components of ANY request structure.
 * It is used for all SO_IP_SET calls, set or get.
 */
#define IP_SET_REQ_BASE \
	unsigned op; \
	int id \
	
struct ip_set_req_base {
	IP_SET_REQ_BASE;
};

struct ip_set_req_std {
	IP_SET_REQ_BASE;
	ip_set_ip_t ip[IP_SET_SETIP_LEVELS];
	u_int8_t level;
};

#define IP_SET_OP_CREATE	0x00000001	/* Create a new (empty) set */
struct ip_set_req_create {
	IP_SET_REQ_BASE;
	char name[IP_SET_MAXNAMELEN];
	char typename[IP_SET_LEVELS][IP_SET_MAXNAMELEN];
	u_int8_t levels;
};

#define IP_SET_OP_DESTROY	0x00000002	/* Remove a (empty) set */
/* Uses ip_set_req_std */

#define IP_SET_OP_CREATE_CHILD 	0x00000003	/* Create a new child set */
struct ip_set_req_sub {
	IP_SET_REQ_BASE;
	ip_set_ip_t ip[IP_SET_SETIP_LEVELS];
	u_int8_t level;
	u_int8_t childsets;
};

#define IP_SET_OP_FLUSH		0x00000004	/* Remove all IPs in a set */
/* Uses ip_set_req_sub */

#define IP_SET_OP_RENAME	0x00000005	/* Rename a set */
struct ip_set_req_rename {
	IP_SET_REQ_BASE;
	char newname[IP_SET_MAXNAMELEN];
};

#define IP_SET_OP_SWAP		0x00000006	/* Swap two sets */
struct ip_set_req_swap {
	IP_SET_REQ_BASE;
	int to;
};

#define IP_SET_OP_ADD_IP	0x00000007	/* Add an IP to a set */
/* Uses ip_set_req_std, with type specific addage */

#define IP_SET_OP_DEL_IP	0x00000008	/* Remove an IP from a set */
/* Uses ip_set_req_std, with type specific addage */

/* Test if an IP is in the set
 */
#define IP_SET_OP_TEST_IP	0x00000009	/* Test an IP in a set */
struct ip_set_req_test {
	IP_SET_REQ_BASE;
	ip_set_ip_t ip[IP_SET_SETIP_LEVELS];
	u_int8_t level;
	int reply;		/* Test result */
};

#define IP_SET_OP_VERSION	0x00000010
struct ip_set_req_version {
	IP_SET_REQ_BASE;
	unsigned version;
};

/* List operations:
 * Size requests are sent by ip_set_req_list
 * except for LISTING.
 */
#define IP_SET_OP_LIST_HEADER_SIZE	0x00000101
#define IP_SET_OP_LIST_HEADER		0x00000102
#define IP_SET_OP_LIST_MEMBERS_SIZE	0x00000103
#define IP_SET_OP_LIST_MEMBERS		0x00000104
#define IP_SET_OP_LIST_CHILDSETS_SIZE	0x00000105
#define IP_SET_OP_LIST_CHILDSETS	0x00000106
struct ip_set_req_list {
	IP_SET_REQ_BASE;
	ip_set_ip_t ip[IP_SET_SETIP_LEVELS];
	u_int8_t level;
	size_t size;
};

#define IP_SET_OP_LISTING_SIZE   	0x00000107
#define IP_SET_OP_LISTING        	0x00000108

struct ip_set_req_listing_size {
	IP_SET_REQ_BASE;
	size_t size;
};

struct ip_set_req_listing {
	char name[IP_SET_MAXNAMELEN];
	char typename[IP_SET_LEVELS][IP_SET_MAXNAMELEN];
	u_int8_t levels;
	unsigned ref;
	int id;
};

/* Between the iptables(8) set extension modules and the kernel we
 * identify a set by its id.
 *
 * The GETSET_BYNAME call passes the name of a set to the kernel, and
 * the a valid set id is returned if the set is still exist.
 * The GETSET_BYID call passes the id a set to the kernel, and
 * the set name is returned if the set is still exist.
 */
#define IP_SET_OP_GETSET_BYNAME		0x00000011
struct ip_set_req_get {
	IP_SET_REQ_BASE;
	unsigned ref;
	char name[IP_SET_MAXNAMELEN];
};

#define IP_SET_OP_GETSET_BYID		0x00000012
/* Uses ip_set_req_get */

static inline int bitmap_bytes(ip_set_ip_t a, ip_set_ip_t b)
{
	return 4 * ((((b - a + 8) / 8) + 3) / 4);
}

#ifdef __KERNEL__

#define ip_set_printk(format, args...) 			\
	do {							\
		printk("%s: %s: ", __FILE__, __FUNCTION__);	\
		printk(format "\n" , ## args);			\
	} while (0)

#if defined(CONFIG_IP_NF_SET_DEBUG) || defined(CONFIG_IP_NF_SET_DEBUG_MODULE)
#define CONFIG_IP_NF_SET_DEBUG
#define CONFIG_IP_NF_SET_DEBUG_MODULE

#define DP(format, args...) 					\
	do {							\
		printk("%s: %s (DBG): ", __FILE__, __FUNCTION__);\
		printk(format "\n" , ## args);			\
	} while (0)
#else
#define DP(format, args...)
#endif

/* Generic set type: */
struct ip_set_private {
	struct ip_set_private **childsets;	/* child sets */

	/* type speficic members */
};

/*
 * The ip_set_type_t definition - one per set type, e.g. "ipmap".
 *
 * Each individual set has a pointer, set->type, going to one
 * of these structures. Function pointers inside the structure implement
 * the real behaviour of the sets.
 *
 * If not mentioned differently, the implementation behind the function
 * pointers of a set_type, is expected to return 0 if ok, and a negative
 * errno (e.g. -EINVAL) on error.
 */
struct ip_set_type {
	struct list_head list;	/* next in list of set types */

	/* match IP in set - internally required
	 * return 0 if not in set, 1 if in set or
	 * negative errno if input was invalid
	 */
	int (*matchip) (struct ip_set_private *private,
			ip_set_ip_t ip,
			ip_set_ip_t *id);

	/* test for IP in set (kernel: iptables -m set --entry x)
	 * return 0 if not in set, 1 if in set.
	 */
	int (*testip_kernel) (struct ip_set_private *private,
			      const struct sk_buff * skb, 
			      u_int32_t flags,
			      ip_set_ip_t *id);

	/* test for IP in set (userspace: ipset -T set --entry x)
	 * return 0 if not in set, 1 if in set.
	 */
	int (*testip) (struct ip_set_private *private,
		       const void *data, size_t size,
		       ip_set_ip_t *id);

	/*
	 * Size of the data structure passed by when
	 * adding/deletin/testing an entry.
	 */
	size_t reqsize;

	/* Add IP into set (userspace: ipset -A set --entry x)
	 * Return -EEXIST if the address is already in the set,
	 * and -ERANGE if the address lies outside the set bounds.
	 * If the address was not already in the set, 0 is returned.
	 */
	int (*addip) (struct ip_set_private *private, 
		      const void *data, size_t size,
		      ip_set_ip_t *id);

	/* Add IP into set (kernel: iptables ... -j SET --entry x)
	 * Return -EEXIST if the address is already in the set,
	 * and -ERANGE if the address lies outside the set bounds.
	 * If the address was not already in the set, 0 is returned.
	 */
	int (*addip_kernel) (struct ip_set_private *private,
			     const struct sk_buff * skb, 
			     u_int32_t flags,
			     ip_set_ip_t *id);

	/* remove IP from set (userspace: ipset -D set --entry x)
	 * Return -EEXIST if the address is NOT in the set,
	 * and -ERANGE if the address lies outside the set bounds.
	 * If the address really was in the set, 0 is returned.
	 */
	int (*delip) (struct ip_set_private *private, 
		      const void *data, size_t size,
		      ip_set_ip_t *id);

	/* remove IP from set (kernel: iptables ... -j SET --entry x)
	 * Return -EEXIST if the address is NOT in the set,
	 * and -ERANGE if the address lies outside the set bounds.
	 * If the address really was in the set, 0 is returned.
	 */
	int (*delip_kernel) (struct ip_set_private *private,
			     const struct sk_buff * skb, 
			     u_int32_t flags,
			     ip_set_ip_t *id);

	/* new set creation - allocated type specific items
	 */
	int (*create) (struct ip_set_private **private,
		       const void *data, size_t size);

	/* set destruction - free type specific items
	 * There is no return value.
	 * Can be called only when child sets are destroyed.
	 */
	void (*destroy) (struct ip_set_private **private);

	/* set flushing - reset all bits in the set, or something similar.
	 * There is no return value.
	 */
	void (*flush) (struct ip_set_private *private);

	/* Listing: Get size needed for header
	 */
	int (*list_header_size) (const struct ip_set_private *private);

	/* Listing: Get the header
	 *
	 * Fill in the information in "data".
	 * This function is always run after list_header_size() under a 
	 * writelock on the set. Therefor is the length of "data" always 
	 * correct. 
	 */
	void (*list_header) (const struct ip_set_private *private, 
			     void *data);

	/* Listing: Get the size for the set members
	 */
	int (*list_members_size) (const struct ip_set_private *private);

	/* Listing: Get the set members
	 *
	 * Fill in the information in "data".
	 * This function is always run after list_member_size() under a 
	 * writelock on the set. Therefor is the length of "data" always 
	 * correct. 
	 */
	void (*list_members) (const struct ip_set_private *private,
			      void *data);

	/* Listing: set size in ids (first id is 0. Cannot change for a set).
	 */
	ip_set_ip_t (*sizeid) (const struct ip_set_private *private);

	/* Listing: Get the bitmap for the valid childsets
	 */
	void (*list_childsets) (const struct ip_set_private *private,
			        void *data);

	char typename[IP_SET_MAXNAMELEN];
	char typecode;
	int protocol_version;

	/* Set this to THIS_MODULE if you are a module, otherwise NULL */
	struct module *me;
};

extern int ip_set_register_set_type(struct ip_set_type *set_type);
extern void ip_set_unregister_set_type(struct ip_set_type *set_type);

/* A generic ipset */
struct ip_set {
	struct list_head list;		/* next in list of all sets */
	rwlock_t lock;			/* a lock for concurrency control */
	unsigned ref;			/* reference counter */
	unsigned subref;		/* reference counter at creating/destroying childsets */
	u_int8_t levels;		/* max levels of subsets */
	struct ip_set_type *type[IP_SET_LEVELS]; /* the set types */
	struct ip_set_private *private;	/* type specific data */
	char name[IP_SET_MAXNAMELEN];	/* the proper name of the set */
};

extern struct ip_set **ip_set_list;

/* register and unregister set pointer references */
extern struct ip_set *ip_set_get_byname(const char name[IP_SET_MAXNAMELEN],
					int *id);
extern struct ip_set *ip_set_get_byid(int id);
extern void ip_set_put(struct ip_set *set);

/* API for iptables set match, and SET target */
extern void ip_set_addip_kernel(struct ip_set *set,
				const struct sk_buff *skb,
				const u_int32_t *flags,
				u_int8_t set_level,
				u_int8_t ip_level);
extern void ip_set_delip_kernel(struct ip_set *set,
				const struct sk_buff *skb,
				const u_int32_t *flags,
				u_int8_t set_level,
				u_int8_t ip_level);
extern int ip_set_testip_kernel(struct ip_set *set,
				const struct sk_buff *skb,
				const u_int32_t *flags,
				u_int8_t set_level,
				u_int8_t ip_level);

#endif				/* __KERNEL__ */

#endif /*_IP_SET_H*/
